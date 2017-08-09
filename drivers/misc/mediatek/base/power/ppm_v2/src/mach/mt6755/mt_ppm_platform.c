/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include "mt_ppm_platform.h"
#include "mt_ppm_internal.h"


void ppm_update_req_by_pwr(enum ppm_power_state new_state, struct ppm_policy_req *req)
{
	ppm_cobra_update_limit(new_state, req);
}

int ppm_find_pwr_idx(struct ppm_cluster_status *cluster_status)
{
	struct ppm_pwr_idx_ref_tbl_data ref_tbl = ppm_get_pwr_idx_ref_tbl();
	unsigned int pwr_idx = 0;
	int i;

	for_each_ppm_clusters(i) {
		int core = cluster_status[i].core_num;
		int opp = cluster_status[i].freq_idx;

		if (core != 0 && opp >= 0 && opp < DVFS_OPP_NUM) {
			pwr_idx += (ref_tbl.pwr_idx_ref_tbl[i].core_total_power[opp] * core)
				+ ref_tbl.pwr_idx_ref_tbl[i].l2_power[opp];
		}

		ppm_ver("[%d] core = %d, opp = %d\n", i, core, opp);
	}

	if (!pwr_idx) {
		ppm_warn("@%s: pwr_idx is 0!\n", __func__);
		return -1; /* not found */
	}

	ppm_ver("@%s: pwr_idx = %d\n", __func__, pwr_idx);

	return pwr_idx;
}

int ppm_get_max_pwr_idx(void)
{
	struct ppm_cluster_status status[NR_PPM_CLUSTERS];
	int i;

	for_each_ppm_clusters(i) {
		status[i].core_num = get_cluster_max_cpu_core(i);
		status[i].freq_idx = get_cluster_max_cpufreq_idx(i);
	}

	return ppm_find_pwr_idx(status);
}

enum ppm_power_state ppm_judge_state_by_user_limit(enum ppm_power_state cur_state,
			struct ppm_userlimit_data user_limit)
{
	enum ppm_power_state new_state = PPM_POWER_STATE_NONE;
	int LL_core_min = user_limit.limit[0].min_core_num;
	int LL_core_max = user_limit.limit[0].max_core_num;
	int L_core_min = user_limit.limit[1].min_core_num;
	int L_core_max = user_limit.limit[1].max_core_num;
	int LL_freq_min = user_limit.limit[0].min_freq_idx;
	int L_freq_max = user_limit.limit[1].max_freq_idx;
	int sum = LL_core_min + L_core_min;

	ppm_ver("Judge: input --> [%s] (%d)(%d)(%d)(%d) [(%d)(%d)]\n",
		ppm_get_power_state_name(cur_state),
		LL_core_min, LL_core_max, L_core_min, L_core_max, LL_freq_min, L_freq_max);

	LL_core_max = (LL_core_max == -1) ? 4 : LL_core_max;
	L_core_max = (L_core_max == -1) ? 4 : L_core_max;

	/* need to check freq limit for cluster move/merge */
	if (cur_state == PPM_POWER_STATE_LL_ONLY || cur_state == PPM_POWER_STATE_L_ONLY) {
		struct ppm_power_state_data *state_info = ppm_get_power_state_info();

		LL_freq_min = (LL_freq_min == -1)
			? state_info[cur_state].cluster_limit->state_limit[0].min_cpufreq_idx
			: LL_freq_min;
		L_freq_max = (L_freq_max == -1)
			? state_info[cur_state].cluster_limit->state_limit[1].max_cpufreq_idx
			: L_freq_max;
		/* idx -> freq */
		LL_freq_min = (ppm_main_info.cluster_info[0].dvfs_tbl)
			? ppm_main_info.cluster_info[0].dvfs_tbl[LL_freq_min].frequency
			: get_cluster_min_cpufreq_idx(0);
		L_freq_max = (ppm_main_info.cluster_info[1].dvfs_tbl)
			? ppm_main_info.cluster_info[1].dvfs_tbl[L_freq_max].frequency
			: get_cluster_max_cpufreq_idx(1);
	}

	/* min_core <= 0: don't care */
	/* min_core > 0: turn on this cluster */
	/* max_core == 0: turn off this cluster */
	switch (cur_state) {
	case PPM_POWER_STATE_LL_ONLY:
		new_state = (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
			: (L_core_min <= 0 || L_core_max == 0) ? cur_state
			/* should not go to L only due to root cluster is fixed at LL */
			: (L_core_min > 0 && ppm_main_info.fixed_root_cluster == 0) ? PPM_POWER_STATE_4LL_L
			: (LL_core_min <= 0) ? PPM_POWER_STATE_L_ONLY
			: (LL_core_min == 4) ? PPM_POWER_STATE_4LL_L
			: (L_core_min == 4) ? PPM_POWER_STATE_4L_LL
			/* merge to L cluster */
			: (sum <= L_core_max && L_freq_max >= LL_freq_min) ? PPM_POWER_STATE_L_ONLY
			: PPM_POWER_STATE_NONE;
		break;
	case PPM_POWER_STATE_L_ONLY:
		new_state = (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
			: (LL_core_min <= 0 || LL_core_max == 0) ? cur_state
			/* keep current if for only LL min is set */
			: (LL_core_min > 0 && L_core_min == -1 && L_freq_max >= LL_freq_min) ? cur_state
			/* will return NONE if LL min is set but L min is not */
			: (L_core_min == 4) ? PPM_POWER_STATE_4L_LL
			: (LL_core_min == 4) ? PPM_POWER_STATE_4LL_L
			/* merge to L cluster */
			: (sum <= L_core_max && L_freq_max >= LL_freq_min) ? cur_state
			: PPM_POWER_STATE_NONE;
		break;
	case PPM_POWER_STATE_4LL_L:
		new_state = (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
			: (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
			: (L_core_min == 4) ? PPM_POWER_STATE_4L_LL
			: (LL_core_max == 4) ? cur_state
			: PPM_POWER_STATE_NONE;
		break;
	case PPM_POWER_STATE_4L_LL:
		new_state = (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
			: (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
			: (L_core_max == 4) ? cur_state
			: (L_core_max < 4 && LL_core_min == 4) ? PPM_POWER_STATE_4LL_L
			: PPM_POWER_STATE_NONE;
		break;
	default:
		break;
	}

	/* check root cluster is fixed or not */
	switch (ppm_main_info.fixed_root_cluster) {
	case 0:
		new_state = (new_state == PPM_POWER_STATE_L_ONLY) ? PPM_POWER_STATE_NONE
			: (new_state == PPM_POWER_STATE_4L_LL) ? PPM_POWER_STATE_4LL_L
			: new_state;
		break;
	case 1:
		new_state = (new_state == PPM_POWER_STATE_LL_ONLY) ? PPM_POWER_STATE_NONE
			: (new_state == PPM_POWER_STATE_4LL_L) ? PPM_POWER_STATE_4L_LL
			: new_state;
		break;
	default:
		break;
	}

	ppm_ver("Judge: output --> [%s]\n", ppm_get_power_state_name(new_state));

	return new_state;
}

/* modify request to fit cur_state */
void ppm_limit_check_for_user_limit(enum ppm_power_state cur_state, struct ppm_policy_req *req,
			struct ppm_userlimit_data user_limit)
{
	if (req && cur_state == PPM_POWER_STATE_L_ONLY) {
		unsigned int LL_min_core = req->limit[0].min_cpu_core;
		unsigned int L_min_core = req->limit[1].min_cpu_core;
		unsigned int sum = LL_min_core + L_min_core;
		unsigned int LL_min_freq, L_min_freq, L_max_freq;

		LL_min_freq = (ppm_main_info.cluster_info[0].dvfs_tbl)
			? ppm_main_info.cluster_info[0].dvfs_tbl[req->limit[0].min_cpufreq_idx].frequency
			: get_cluster_min_cpufreq_idx(0);
		L_min_freq = (ppm_main_info.cluster_info[1].dvfs_tbl)
			? ppm_main_info.cluster_info[1].dvfs_tbl[req->limit[1].min_cpufreq_idx].frequency
			: get_cluster_min_cpufreq_idx(1);
		L_max_freq = (ppm_main_info.cluster_info[1].dvfs_tbl)
			? ppm_main_info.cluster_info[1].dvfs_tbl[req->limit[1].max_cpufreq_idx].frequency
			: get_cluster_max_cpufreq_idx(1);

		if (LL_min_core > 0 && L_max_freq >= LL_min_freq) {
			/* user do not set L min so we just move LL core to L */
			if (user_limit.limit[1].min_core_num <= 0) {
				req->limit[0].min_cpu_core = 0;
				req->limit[1].min_cpu_core = LL_min_core;
				ppm_ver("Judge: move LL min core to L = %d\n", LL_min_core);
			} else if (sum <= 4) {
				req->limit[0].min_cpu_core = 0;
				req->limit[1].min_cpu_core = sum;
				ppm_ver("Judge: merge LL and L min core = %d\n", sum);
#ifdef PPM_IC_SEGMENT_CHECK
			} else if (ppm_main_info.fix_state_by_segment == PPM_POWER_STATE_L_ONLY) {
				req->limit[0].min_cpu_core = 0;
				req->limit[1].min_cpu_core = min(sum, get_cluster_max_cpu_core(1));
				ppm_ver("Judge: merge LL and L min core = %d(sum = %d)\n",
					req->limit[1].min_cpu_core, sum);
#endif
			} else {
				ppm_ver("Judge: cannot merge to L! LL min = %d, L min = %d\n",
					LL_min_core, L_min_core);
				/* check LL max core */
				if (req->limit[0].max_cpu_core < LL_min_core)
					req->limit[0].max_cpu_core = LL_min_core;

				return;
			}

			if (LL_min_freq > L_min_freq) {
				req->limit[1].min_cpufreq_idx =
					ppm_main_freq_to_idx(1, LL_min_freq, CPUFREQ_RELATION_L);
				ppm_ver("Judge: change L min freq idx to %d due to LL min freq = %d\n",
					req->limit[1].min_cpufreq_idx, LL_min_freq);
			}
		}
	}
}

/* to avoid build error since cpufreq does not provide no lock API for MT6755 */
#include "mt_cpufreq.h"
unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(unsigned int cluster)
{
	return mt_cpufreq_get_cur_phy_freq(cluster);
}

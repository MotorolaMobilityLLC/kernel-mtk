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
	int LL_core_min = user_limit.limit[PPM_CLUSTER_LL].min_core_num;
	int LL_core_max = user_limit.limit[PPM_CLUSTER_LL].max_core_num;
	int L_core_min = user_limit.limit[PPM_CLUSTER_L].min_core_num;
	int L_core_max = user_limit.limit[PPM_CLUSTER_L].max_core_num;
	int LL_freq_min = user_limit.limit[PPM_CLUSTER_LL].min_freq_idx;
	int L_freq_max = user_limit.limit[PPM_CLUSTER_L].max_freq_idx;
	int sum = LL_core_min + L_core_min;
	int B_core_min = user_limit.limit[PPM_CLUSTER_B].min_core_num;
	int B_core_max = user_limit.limit[PPM_CLUSTER_B].max_core_num;
	int root_cluster = ppm_main_info.fixed_root_cluster;

	ppm_ver("Judge: input --> [%s] (%d)(%d)(%d)(%d)(%d)(%d) [(%d)(%d)]\n",
		ppm_get_power_state_name(cur_state), LL_core_min, LL_core_max,
		 L_core_min, L_core_max, B_core_min, B_core_max, LL_freq_min, L_freq_max);

	LL_core_max = (LL_core_max == -1) ? get_cluster_max_cpu_core(PPM_CLUSTER_LL) : LL_core_max;
	L_core_max = (L_core_max == -1) ? get_cluster_max_cpu_core(PPM_CLUSTER_L) : L_core_max;

	/* need to check freq limit for cluster move/merge */
	if (cur_state == PPM_POWER_STATE_LL_ONLY || cur_state == PPM_POWER_STATE_L_ONLY) {
		struct ppm_power_state_data *state_info = ppm_get_power_state_info();

		LL_freq_min = (LL_freq_min == -1)
			? state_info[cur_state].cluster_limit->state_limit[PPM_CLUSTER_LL].min_cpufreq_idx
			: LL_freq_min;
		L_freq_max = (L_freq_max == -1)
			? state_info[cur_state].cluster_limit->state_limit[PPM_CLUSTER_L].max_cpufreq_idx
			: L_freq_max;
		/* idx -> freq */
		LL_freq_min = (ppm_main_info.cluster_info[PPM_CLUSTER_LL].dvfs_tbl)
			? ppm_main_info.cluster_info[PPM_CLUSTER_LL].dvfs_tbl[LL_freq_min].frequency
			: get_cluster_min_cpufreq_idx(PPM_CLUSTER_LL);
		L_freq_max = (ppm_main_info.cluster_info[PPM_CLUSTER_L].dvfs_tbl)
			? ppm_main_info.cluster_info[PPM_CLUSTER_L].dvfs_tbl[L_freq_max].frequency
			: get_cluster_max_cpufreq_idx(PPM_CLUSTER_L);
	}

	/* min_core <= 0: don't care */
	/* min_core > 0: turn on this cluster */
	/* max_core == 0: turn off this cluster */
	switch (cur_state) {
	case PPM_POWER_STATE_LL_ONLY:
		/* not force Big core on */
		if (B_core_min <= 0) {
			new_state = (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
				: (L_core_min <= 0 || L_core_max == 0) ? cur_state
				/* should not go to L only due to root cluster is fixed at LL */
				: (L_core_min > 0 && root_cluster == PPM_CLUSTER_LL) ? PPM_POWER_STATE_4LL_L
				: (LL_core_min <= 0) ? PPM_POWER_STATE_L_ONLY
				/* merge to L cluster */
				: (sum <= L_core_max && L_freq_max >= LL_freq_min) ? PPM_POWER_STATE_L_ONLY
				: PPM_POWER_STATE_4LL_L;
		} else {
			new_state = (LL_core_max == 0 && L_core_max == 0) ? PPM_POWER_STATE_NONE
				: (LL_core_max == 0 || (LL_core_min <= 0 && L_core_min > 0)) ? PPM_POWER_STATE_4L_LL
				: PPM_POWER_STATE_4LL_L;
		}
		break;
	case PPM_POWER_STATE_L_ONLY:
		/* not force Big core on */
		if (B_core_min <= 0) {
			new_state = (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
				: (LL_core_min <= 0 || LL_core_max == 0) ? cur_state
				/* keep current if for only LL min is set */
				: (LL_core_min > 0 && L_core_min == -1 && L_freq_max >= LL_freq_min) ? cur_state
				/* merge to L cluster */
				: (sum <= L_core_max && L_freq_max >= LL_freq_min) ? cur_state
				: PPM_POWER_STATE_4L_LL;
		} else {
			new_state = (LL_core_max == 0 && L_core_max == 0) ? PPM_POWER_STATE_NONE
				: (L_core_max == 0) ? PPM_POWER_STATE_4LL_L
				: PPM_POWER_STATE_4L_LL;
		}
		break;
	case PPM_POWER_STATE_4LL_L:
		/* force Big core off */
		if (B_core_max == 0) {
			new_state = (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
				: (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
				: (LL_core_min <= 0 && L_core_min > 0) ? PPM_POWER_STATE_4L_LL
				: cur_state;
		} else {
			new_state = (LL_core_max == 0 && L_core_max == 0) ? PPM_POWER_STATE_NONE
				: (LL_core_max == 0 || (LL_core_min <= 0 && L_core_min > 0)) ? PPM_POWER_STATE_4L_LL
				: cur_state;
		}
		break;
	case PPM_POWER_STATE_4L_LL:
		/* force Big core off */
		if (B_core_max == 0) {
			new_state = (LL_core_max == 0) ? PPM_POWER_STATE_L_ONLY
				: (L_core_max == 0) ? PPM_POWER_STATE_LL_ONLY
				: cur_state;
		} else {
			new_state = (LL_core_max == 0 && L_core_max == 0) ? PPM_POWER_STATE_NONE
				: (L_core_max == 0) ? PPM_POWER_STATE_4LL_L
				: cur_state;
		}
		break;
	default:
		break;
	}

	/* check root cluster is fixed or not */
	switch (root_cluster) {
	case PPM_CLUSTER_LL:
		new_state = (new_state == PPM_POWER_STATE_L_ONLY) ? PPM_POWER_STATE_NONE
			: (new_state == PPM_POWER_STATE_4L_LL) ? PPM_POWER_STATE_4LL_L
			: new_state;
		break;
	case PPM_CLUSTER_L:
		new_state = (new_state == PPM_POWER_STATE_LL_ONLY) ? PPM_POWER_STATE_NONE
			: (new_state == PPM_POWER_STATE_4LL_L) ? PPM_POWER_STATE_4L_LL
			: new_state;
		break;
	default:
		break;
	}

#ifdef PPM_DISABLE_CLUSTER_MIGRATION
	if (new_state == PPM_POWER_STATE_L_ONLY || new_state == PPM_POWER_STATE_4L_LL)
		new_state = PPM_POWER_STATE_4LL_L;
#endif

	ppm_ver("Judge: output --> [%s]\n", ppm_get_power_state_name(new_state));

	return new_state;
}

/* modify request to fit cur_state */
void ppm_limit_check_for_user_limit(enum ppm_power_state cur_state, struct ppm_policy_req *req,
			struct ppm_userlimit_data user_limit)
{
	if (req && ((cur_state == PPM_POWER_STATE_L_ONLY) || (cur_state == PPM_POWER_STATE_4L_LL))
		&& (req->limit[PPM_CLUSTER_L].max_cpu_core >= req->limit[PPM_CLUSTER_LL].min_cpu_core)) {
		unsigned int LL_min_core = req->limit[PPM_CLUSTER_LL].min_cpu_core;
		unsigned int L_min_core = req->limit[PPM_CLUSTER_L].min_cpu_core;
		unsigned int sum = LL_min_core + L_min_core;
		unsigned int LL_min_freq, L_min_freq, L_max_freq;

		LL_min_freq = (ppm_main_info.cluster_info[PPM_CLUSTER_LL].dvfs_tbl)
			? ppm_main_info.cluster_info[PPM_CLUSTER_LL]
				.dvfs_tbl[req->limit[PPM_CLUSTER_LL].min_cpufreq_idx].frequency
			: get_cluster_min_cpufreq_idx(PPM_CLUSTER_LL);
		L_min_freq = (ppm_main_info.cluster_info[PPM_CLUSTER_L].dvfs_tbl)
			? ppm_main_info.cluster_info[PPM_CLUSTER_L]
				.dvfs_tbl[req->limit[PPM_CLUSTER_L].min_cpufreq_idx].frequency
			: get_cluster_min_cpufreq_idx(PPM_CLUSTER_L);
		L_max_freq = (ppm_main_info.cluster_info[PPM_CLUSTER_L].dvfs_tbl)
			? ppm_main_info.cluster_info[PPM_CLUSTER_L]
				.dvfs_tbl[req->limit[PPM_CLUSTER_L].max_cpufreq_idx].frequency
			: get_cluster_max_cpufreq_idx(PPM_CLUSTER_L);

		if (LL_min_core > 0 && L_max_freq >= LL_min_freq) {
			/* user do not set L min so we just move LL core to L */
			if (user_limit.limit[PPM_CLUSTER_L].min_core_num <= 0) {
				req->limit[PPM_CLUSTER_LL].min_cpu_core = 0;
				req->limit[PPM_CLUSTER_L].min_cpu_core = LL_min_core;
				ppm_ver("Judge: move LL min core to L = %d\n", LL_min_core);
			} else if (sum <= get_cluster_max_cpu_core(PPM_CLUSTER_L)) {
				req->limit[PPM_CLUSTER_LL].min_cpu_core = 0;
				req->limit[PPM_CLUSTER_L].min_cpu_core = sum;
				ppm_ver("Judge: merge LL and L min core = %d\n", sum);
			} else {
				ppm_ver("Judge: cannot merge to L! LL min = %d, L min = %d\n",
					LL_min_core, L_min_core);
				/* check LL max core */
				if (req->limit[PPM_CLUSTER_LL].max_cpu_core < LL_min_core)
					req->limit[PPM_CLUSTER_LL].max_cpu_core = LL_min_core;

				return;
			}

			if (LL_min_freq > L_min_freq) {
				req->limit[PPM_CLUSTER_L].min_cpufreq_idx =
					ppm_main_freq_to_idx(PPM_CLUSTER_L, LL_min_freq, CPUFREQ_RELATION_L);
				ppm_ver("Judge: change L min freq idx to %d due to LL min freq = %d\n",
					req->limit[PPM_CLUSTER_L].min_cpufreq_idx, LL_min_freq);
			}
		}
	}
}


#if PPM_HW_OCP_SUPPORT
#include <linux/delay.h>
#include <linux/ktime.h>
#include "mt_idvfs.h"

static unsigned int max_power_except_big;
static unsigned int max_power_big;
static unsigned int max_power_1L;
/* set a margin (2B @ idx 14) to avoid big be throttled / un-throttled frequently */
static unsigned int big_on_margin;
#define MAX_OCP_TARGET_POWER	127000

static bool ppm_is_big_cluster_on(void)
{
	struct cpumask cpumask, cpu_online_cpumask;

	arch_get_cluster_cpus(&cpumask, PPM_CLUSTER_B);
	cpumask_and(&cpu_online_cpumask, &cpumask, cpu_online_mask);

	return (cpumask_weight(&cpu_online_cpumask) > 0) ? true : false;
}

/* return value is the remaining power budget for SW DLPT */
unsigned int ppm_set_ocp(unsigned int limited_power, unsigned int percentage)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	struct ppm_pwr_idx_ref_tbl_data ref_tbl = ppm_get_pwr_idx_ref_tbl();
	int i, ret = 0;
	unsigned int power_for_ocp = 0, power_for_tbl_lookup = 0;

	/* if max_power_except_big < limited_power, set (limited_power - max_power_except_big) to HW OCP */
	if (!max_power_except_big) {
		for_each_pwr_tbl_entry(i, power_table) {
			if (power_table.power_tbl[i].cluster_cfg[PPM_CLUSTER_B].core_num == 0) {
				max_power_except_big = (power_table.power_tbl[i].power_idx);
				break;
			}
		}

		max_power_big = power_table.power_tbl[0].power_idx - max_power_except_big;
		big_on_margin = (ref_tbl.pwr_idx_ref_tbl[PPM_CLUSTER_B].core_total_power[14] * 2)
				+ ref_tbl.pwr_idx_ref_tbl[PPM_CLUSTER_B].l2_power[14];
		max_power_1L = ref_tbl.pwr_idx_ref_tbl[PPM_CLUSTER_L].core_total_power[0]
				+ ref_tbl.pwr_idx_ref_tbl[PPM_CLUSTER_L].l2_power[0];

		ppm_info("max_power_big = %d, max_power_except_big = %d, big_on_margin = %d, max_power_1L = %d\n",
			max_power_big, max_power_except_big, big_on_margin, max_power_1L);
	}

	/* no need to set big OCP since big cluster is off */
	if (!ppm_is_big_cluster_on()) {
		/* no need to throttle big core since it is not powered on */
		if (limited_power > max_power_except_big
			&& limited_power < power_table.power_tbl[0].power_idx
			&& !ppm_is_need_to_check_tlp3_table(ppm_main_info.cur_power_state, limited_power)) {
			/* limited_power is over margin so big can be turned on! */
			if (limited_power > max_power_except_big + big_on_margin)
				return power_table.power_tbl[0].power_idx;
			else
				return max_power_except_big;
		} else
			return limited_power;
	}

	/* OCP is turned off by user, skip set ocp flow! */
	if (!ocp_status_get(PPM_CLUSTER_B))
		return limited_power;

	if (ppm_is_need_to_check_tlp3_table(ppm_main_info.cur_power_state, limited_power)) {
		/* We need big now! give all budget - max_power_1L power to OCP */
		power_for_ocp = (percentage)
			? ((limited_power - max_power_1L) * 100 + (percentage - 1)) / percentage
			: (limited_power - max_power_1L);
		power_for_ocp = (power_for_ocp >= max_power_big)
			? MAX_OCP_TARGET_POWER : power_for_ocp;
		power_for_tbl_lookup = limited_power;
	} else if (limited_power <= max_power_except_big) {
		/* disable HW OCP by setting budget to 0 */
		power_for_ocp = 0;
		power_for_tbl_lookup = limited_power;
	} else {
		/* pass remaining power to HW OCP */
		power_for_ocp = (percentage)
			? ((limited_power - max_power_except_big) * 100 + (percentage - 1)) / percentage
			: (limited_power - max_power_except_big);
		power_for_ocp = (power_for_ocp >= max_power_big)
			? MAX_OCP_TARGET_POWER : power_for_ocp;
		/* set to max power in power table to avoid big core being throttled */
		power_for_tbl_lookup = power_table.power_tbl[0].power_idx;
	}

	ppm_dbg(DLPT, "power_for_ocp = %d, power_for_tbl_lookup = %d\n",
		power_for_ocp, power_for_tbl_lookup);

	ret = BigOCPSetTarget(OCP_ALL, power_for_ocp);
	if (ret) {
		/* pass all limited power for tbl lookup if set ocp target failed */
		ppm_err("@%s: OCP set target(%d) failed, ret = %d\n", __func__, power_for_ocp, ret);
		return limited_power;
	}

	if (power_for_ocp) {
		int leakage, total, clkpct;

		BigOCPCapture(1, 1, 0, 15);
		BigOCPCaptureStatus(&leakage, &total, &clkpct);

		if (clkpct == 625)
			ppm_warn("OC triggered by OCP!!, budget = %d\n", power_for_ocp);
		else
			ppm_dbg(DLPT, "set budget = %d to OCP done, clkpct = %d\n", power_for_ocp, clkpct);
	}

	return power_for_tbl_lookup;
}
#endif

#if PPM_DLPT_ENHANCEMENT
#if PPM_HW_OCP_SUPPORT
static bool ppm_is_ocp_available(struct ppm_cluster_status *cluster_status, unsigned int cluster_num)
{
	int i;

	for (i = 0; i < cluster_num; i++) {
		/* OCP may disabled by user */
		if (cluster_status[i].core_num && !ocp_status_get(i))
			return false;
	}

	return true;
}

unsigned int ppm_calc_total_power_by_ocp(struct ppm_cluster_status *cluster_status, unsigned int cluster_num)
{
	int i;
	unsigned int budget = 0;
	unsigned long long total[NR_PPM_CLUSTERS];
	unsigned int freq[NR_PPM_CLUSTERS];
	unsigned int count[NR_PPM_CLUSTERS];
	ktime_t now, delta;

#if !PPM_USE_OCP_DPM
	return 0;
#endif

	if (!ppm_is_ocp_available(cluster_status, cluster_num))
		return 0;

	now = ktime_get();
	for (i = 0; i < cluster_num; i++) {
		if (!cluster_status[i].core_num)
			continue;

		freq[i] = ppm_main_info.cluster_info[i].dvfs_tbl[cluster_status[i].freq_idx].frequency / 1000;
		count[i] = (i == PPM_CLUSTER_B) ? 300 : get_cluster_min_cpufreq(i) * 7 / 10;
	}

	/* set LL/L Avg power first */
	for (i = 0; i < 2; i++) {
		if (!cluster_status[i].core_num)
			continue;

		LittleOCPDVFSSet(i, freq[i], cluster_status[i].volt);
		LittleOCPAvgPwr(i, 1, get_cluster_min_cpufreq(i) * 7 / 10);
	}

	/* get Big pwr if needed */
	if (cluster_status[PPM_CLUSTER_B].core_num) {
		total[PPM_CLUSTER_B] = BigOCPAvgPwrGet(count[PPM_CLUSTER_B]);
		if (!total[PPM_CLUSTER_B]) {
			ppm_warn("Big OCP capture failed!\n");
			return 0; /* error */
		}
	}

	/* get LL/L pwr */
	for (i = 0; i < 2; i++) {
		int retry = 50;

		if (!cluster_status[i].core_num)
			continue;

		do {
			total[i] = (unsigned long long)LittleOCPAvgPwrGet(i);
			if (total[i]) /* success */
				break;
			retry--;
			udelay(20);
		} while (retry > 0);
		if (!retry) {
			ppm_warn("%s OCP capture failed, total = %llu\n",
				(i == PPM_CLUSTER_LL) ? "LL" : "L", total[i]);
			return 0; /* error */
		}
	}
	delta = ktime_sub(ktime_get(), now);

	/* calculate total budget */
	for (i = 0; i < cluster_num; i++) {
		if (!cluster_status[i].core_num)
			continue;

		if (i == PPM_CLUSTER_B)
			budget += total[i];
		else
			budget += (total[i] * cluster_status[i].volt + (1000 - 1)) / 1000;

		ppm_dbg(DLPT, "ocp(%d):F/V/core/cnt/total = %d/%d/%d/%d/%llu\n", i, freq[i],
			cluster_status[i].volt, cluster_status[i].core_num, count[i], total[i]);
	}

	/* add MCUSYS power, use LL volt since it is shared buck */
	budget += (OCPMcusysPwrGet() * cluster_status[0].volt + (1000 - 1)) / 1000;

	ppm_dbg(DLPT, "ocp: total budget = %d, time = %lldus\n", budget, ktime_to_us(delta));

	return budget;
}
#endif

unsigned int ppm_calc_total_power(struct ppm_cluster_status *cluster_status,
				unsigned int cluster_num, unsigned int percentage)
{
	struct ppm_pwr_idx_ref_tbl_data ref_tbl = ppm_get_pwr_idx_ref_tbl();
	unsigned int dynamic, lkg, total, budget = 0;
	int i;
	ktime_t now, delta;

	for (i = 0; i < cluster_num; i++) {
		int core = cluster_status[i].core_num;
		int opp = cluster_status[i].freq_idx;

		if (core != 0 && opp >= 0 && opp < DVFS_OPP_NUM) {
			now = ktime_get();
			dynamic = ref_tbl.pwr_idx_ref_tbl[i].core_dynamic_power[opp];
			lkg = mt_cpufreq_get_leakage_mw(i + 1) / get_cluster_max_cpu_core(i);
			total = ((((dynamic * 100 + (percentage - 1)) / percentage) + lkg) * core)
				+ ref_tbl.pwr_idx_ref_tbl[i].l2_power[opp];
			budget += total;
			delta = ktime_sub(ktime_get(), now);

			ppm_dbg(DLPT, "(%d):OPP/V/core/Lkg/total = %d/%d/%d/%d/%d(time = %lldus)\n",
				i, opp, cluster_status[i].volt, cluster_status[i].core_num,
				lkg, total, ktime_to_us(delta));
		}
	}

	if (!budget) {
		ppm_warn("@%s: pwr_idx is 0!\n", __func__);
		return -1; /* not found */
	}

	ppm_dbg(DLPT, "@%s: total budget = %d\n", __func__, budget);

	return budget;
}
#endif



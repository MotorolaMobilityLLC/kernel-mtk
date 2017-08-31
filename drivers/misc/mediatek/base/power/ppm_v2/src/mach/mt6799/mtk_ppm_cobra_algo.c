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
#include <linux/cpumask.h>
#include <linux/io.h>

#include "mtk_ppm_platform.h"
#include "mtk_ppm_internal.h"
#include "mtk_unified_power.h"


struct ppm_cobra_data *cobra_tbl;
struct ppm_cobra_lookup cobra_lookup_data;

static int prev_max_cpufreq_idx[NR_PPM_CLUSTERS];
static int Core_limit[NR_PPM_CLUSTERS];
/* TODO: check idx mapping */
#if PPM_COBRA_NEED_OPP_MAPPING
static int fidx_mapping_tbl_shared_fy[COBRA_OPP_NUM] = {0, 3, 5, 7, 9, 10, 12, 14};
static int fidx_mapping_tbl_shared_fy_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 7, 7};
static int fidx_mapping_tbl_single_fy[COBRA_OPP_NUM] = {0, 4, 6, 8, 10, 12, 14, 15};
static int fidx_mapping_tbl_single_fy_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7};
static int fidx_mapping_tbl_shared_sb[COBRA_OPP_NUM] = {0, 3, 6, 8, 9, 11, 12, 14};
static int fidx_mapping_tbl_shared_sb_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 7};
static int fidx_mapping_tbl_single_sb[COBRA_OPP_NUM] = {0, 3, 6, 8, 9, 10, 12, 15};
static int fidx_mapping_tbl_single_sb_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 6, 7, 7, 7};

int *fidx_mapping_tbl_shared;
int *fidx_mapping_tbl_shared_r;
int *fidx_mapping_tbl_single;
int *fidx_mapping_tbl_single_r;
#endif

#define ACT_CORE(cluster)		(active_core[CLUSTER_ID_##cluster])
#define CORE_LIMIT(cluster)		(core_limit_tmp[CLUSTER_ID_##cluster])
#define PREV_FREQ_LIMIT(cluster)	(prev_max_cpufreq_idx[CLUSTER_ID_##cluster])


static unsigned int get_idx_in_pwr_tbl(enum ppm_cluster cluster)
{
	unsigned int idx = 0;

	if (cluster >= NR_PPM_CLUSTERS) {
		ppm_err("%s: Invalid input: cluster=%d\n", __func__, cluster);
		WARN_ON(1);
	}

	while (cluster)
		idx += get_cluster_max_cpu_core(--cluster);

	return idx;
}

static short get_delta_pwr_shared(unsigned int core_c1, unsigned int core_c2, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	unsigned int idx_c1, idx_c2;
	unsigned int cur_opp, prev_opp;
	unsigned short cur_pwr, prev_pwr;
	short delta_pwr;
#endif

	if (core_c1 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C1)
		|| core_c2 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C2)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SHARED_C1)) {
		ppm_err("%s: Invalid input: c1_core=%d, c2_core=%d, opp=%d\n",
			__func__, core_c1, core_c2, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core_c1 == 0 && core_c2 == 0)
		return 0;

	idx_c1 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C1);
	idx_c2 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C2);

#if PPM_COBRA_NEED_OPP_MAPPING
	cur_opp = fidx_mapping_tbl_shared[opp];
	prev_opp = fidx_mapping_tbl_shared[opp+1];
#else
	cur_opp = opp;
	prev_opp = opp + 1;
#endif
	cur_pwr = (core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].power_idx : 0;
	cur_pwr += (core_c2) ? cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-1][cur_opp].power_idx : 0;

	if (opp == COBRA_OPP_NUM - 1) {
		prev_pwr = (core_c2) ?
			((core_c2 > 1) ? (cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-2][cur_opp].power_idx +
			((core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].power_idx : 0))
			: ((core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].power_idx : 0))
			: ((core_c1 > 1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-2][cur_opp].power_idx : 0);
	} else {
		prev_pwr = (core_c2) ? cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-1][prev_opp].power_idx : 0;
		prev_pwr += (core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][prev_opp].power_idx : 0;
	}

	delta_pwr = cur_pwr - prev_pwr;

	return delta_pwr;
#else
	return cobra_tbl->delta_tbl_shared[core_c2][core_c1][opp].delta_pwr;
#endif
}

static short get_delta_perf_shared(unsigned int core_c1, unsigned int core_c2, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	unsigned int idx_c1, idx_c2;
	unsigned int cur_opp, prev_opp;
	unsigned short cur_perf, prev_perf;
	short delta_perf;
#endif

	if (core_c1 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C1)
		|| core_c2 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C2)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SHARED_C1)) {
		ppm_err("%s: Invalid input: c1_core=%d, c2_core=%d, opp=%d\n",
			__func__, core_c1, core_c2, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core_c1 == 0 && core_c2 == 0)
		return 0;

	idx_c1 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C1);
	idx_c2 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C2);

#if PPM_COBRA_NEED_OPP_MAPPING
	cur_opp = fidx_mapping_tbl_shared[opp];
	prev_opp = fidx_mapping_tbl_shared[opp+1];
#else
	cur_opp = opp;
	prev_opp = opp + 1;
#endif
	cur_perf = (core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].perf_idx : 0;
	cur_perf += (core_c2) ? cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-1][cur_opp].perf_idx : 0;

	if (opp == COBRA_OPP_NUM - 1) {
		prev_perf = (core_c2) ?
			((core_c2 > 1) ? (cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-2][cur_opp].perf_idx +
			((core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].perf_idx : 0))
			: ((core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][cur_opp].perf_idx : 0))
			: ((core_c1 > 1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-2][cur_opp].perf_idx : 0);
	} else {
		prev_perf = (core_c2) ? cobra_tbl->basic_pwr_tbl[idx_c2+core_c2-1][prev_opp].perf_idx : 0;
		prev_perf += (core_c1) ? cobra_tbl->basic_pwr_tbl[idx_c1+core_c1-1][prev_opp].perf_idx : 0;
	}

	delta_perf = cur_perf - prev_perf;

	return delta_perf;
#else
	return cobra_tbl->delta_tbl_shared[core_c2][core_c1][opp].delta_perf;
#endif
}

static short get_delta_eff_shared(unsigned int core_c1, unsigned int core_c2, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	short delta_pwr, delta_perf, delta_eff;
#endif

	if (core_c1 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C1)
		|| core_c2 > get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C2)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SHARED_C1)) {
		ppm_err("%s: Invalid input: c1_core=%d, c2_core=%d, opp=%d\n",
			__func__, core_c1, core_c2, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core_c1 == 0 && core_c2 == 0)
		return 0;

	delta_pwr = get_delta_pwr_shared(core_c1, core_c2, opp);
	delta_perf = get_delta_perf_shared(core_c1, core_c2, opp);

	if (opp == COBRA_OPP_NUM - 1)
		/* x10 to make it hard to turn off cores */
		delta_eff = (delta_perf * 1000) / delta_pwr;
	else
		delta_eff = (delta_perf * 100) / delta_pwr;

	return delta_eff;
#else
	return cobra_tbl->delta_tbl_shared[core_c2][core_c1][opp].delta_eff;
#endif
}

static short get_delta_pwr_single(unsigned int core, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	unsigned int idx, cur_opp, prev_opp;
	short delta_pwr;
#endif

	if (core > get_cluster_max_cpu_core(CLUSTER_ID_SINGLE)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SINGLE)) {
		ppm_err("%s: Invalid input: core=%d, opp=%d\n", __func__, core, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core == 0)
		return 0;

	idx = get_idx_in_pwr_tbl(CLUSTER_ID_SINGLE);

#if PPM_COBRA_NEED_OPP_MAPPING
	cur_opp = fidx_mapping_tbl_single[opp];
	prev_opp = fidx_mapping_tbl_single[opp+1];
#else
	cur_opp = opp;
	prev_opp = opp + 1;
#endif

	if (opp == COBRA_OPP_NUM - 1) {
		delta_pwr = (core == 1)
			? cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].power_idx
			: (cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].power_idx -
			cobra_tbl->basic_pwr_tbl[idx+core-2][cur_opp].power_idx);
	} else {
		delta_pwr = cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].power_idx -
			cobra_tbl->basic_pwr_tbl[idx+core-1][prev_opp].power_idx;
	}

	return delta_pwr;
#else
	return cobra_tbl->delta_tbl_single[core][opp].delta_pwr;
#endif
}

static short get_delta_perf_single(unsigned int core, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	unsigned int idx, cur_opp, prev_opp;
	short delta_perf;
#endif

	if (core > get_cluster_max_cpu_core(CLUSTER_ID_SINGLE)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SINGLE)) {
		ppm_err("%s: Invalid input: core=%d, opp=%d\n", __func__, core, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core == 0)
		return 0;

	idx = get_idx_in_pwr_tbl(CLUSTER_ID_SINGLE);

#if PPM_COBRA_NEED_OPP_MAPPING
	cur_opp = fidx_mapping_tbl_single[opp];
	prev_opp = fidx_mapping_tbl_single[opp+1];
#else
	cur_opp = opp;
	prev_opp = opp + 1;
#endif

	if (opp == COBRA_OPP_NUM - 1) {
		delta_perf = (core == 1)
			? cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].perf_idx
			: (cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].perf_idx -
			cobra_tbl->basic_pwr_tbl[idx+core-2][cur_opp].perf_idx);
	} else {
		delta_perf = cobra_tbl->basic_pwr_tbl[idx+core-1][cur_opp].perf_idx -
			cobra_tbl->basic_pwr_tbl[idx+core-1][prev_opp].perf_idx;
	}

	return delta_perf;
#else
	return cobra_tbl->delta_tbl_single[core][opp].delta_perf;
#endif
}

static short get_delta_eff_single(unsigned int core, unsigned int opp)
{
#if PPM_COBRA_RUNTIME_CALC_DELTA
	short delta_pwr, delta_perf, delta_eff;
#endif

	if (core > get_cluster_max_cpu_core(CLUSTER_ID_SINGLE)
		|| opp > get_cluster_min_cpufreq_idx(CLUSTER_ID_SINGLE)) {
		ppm_err("%s: Invalid input: core=%d, opp=%d\n", __func__, core, opp);
		WARN_ON(1);
	}

#if PPM_COBRA_RUNTIME_CALC_DELTA
	if (core == 0)
		return 0;

	delta_pwr = get_delta_pwr_single(core, opp);
	delta_perf = get_delta_perf_single(core, opp);

	if (opp == COBRA_OPP_NUM - 1)
		/* x10 to make it hard to turn off cores */
		delta_eff = (delta_perf * 1000) / delta_pwr;
	else
		delta_eff = (delta_perf * 100) / delta_pwr;

	return delta_eff;
#else
	return cobra_tbl->delta_tbl_single[core][opp].delta_eff;
#endif
}


void ppm_cobra_update_core_limit(unsigned int cluster, int limit)
{
	if (cluster >= NR_PPM_CLUSTERS) {
		ppm_err("%s: Invalid cluster id = %d\n", __func__, cluster);
		WARN_ON(1);
	}

	if (limit < 0 || limit > get_cluster_max_cpu_core(cluster)) {
		ppm_err("%s: Invalid core limit for cluster%d = %d\n", __func__, cluster, limit);
		WARN_ON(1);
	}

	Core_limit[cluster] = limit;
}

void ppm_cobra_update_freq_limit(unsigned int cluster, int limit)
{
	if (cluster >= NR_PPM_CLUSTERS) {
		ppm_err("%s: Invalid cluster id = %d\n", __func__, cluster);
		WARN_ON(1);
	}

	if (limit < 0 || limit > get_cluster_min_cpufreq_idx(cluster)) {
		ppm_err("%s: Invalid freq limit for cluster%d = %d\n", __func__, cluster, limit);
		WARN_ON(1);
	}

	prev_max_cpufreq_idx[cluster] = limit;
}

void ppm_cobra_update_limit(enum ppm_power_state new_state, void *user_req)
{
	struct ppm_power_state_data *state_info;
	struct ppm_policy_req *req;
	short power_budget;
	int opp[NR_PPM_CLUSTERS];
	int active_core[NR_PPM_CLUSTERS];
#if PPM_COBRA_USE_CORE_LIMIT
	int core_limit_tmp[NR_PPM_CLUSTERS];
#endif
	int i;
	struct cpumask cluster_cpu, online_cpu;
	short delta_power;
	int shared_cluster_select;
	/* Get power index of current OPP */
	short curr_power = 0;
	struct ppm_cluster_status cl_status[NR_PPM_CLUSTERS];
	int is_shared_freq_limited = 0;

	/* skip if DVFS is not ready (we cannot get current freq...) */
	if (!ppm_main_info.client_info[PPM_CLIENT_DVFS].limit_cb)
		return;

	if (!user_req)
		return;

	state_info = ppm_get_power_state_info();
	req = (struct ppm_policy_req *)user_req;
	power_budget = req->power_budget;

	if (power_budget >= ppm_get_max_pwr_idx())
		return;

	ppm_dbg(COBRA_ALGO, "[PREV]Core_Limit=%d%d%d, policy_limit=%d%d%d, state=%s\n",
			Core_limit[CLUSTER_ID_SHARED_C1],
			Core_limit[CLUSTER_ID_SHARED_C2],
			Core_limit[CLUSTER_ID_SINGLE],
			req->limit[CLUSTER_ID_SHARED_C1].max_cpu_core,
			req->limit[CLUSTER_ID_SHARED_C2].max_cpu_core,
			req->limit[CLUSTER_ID_SINGLE].max_cpu_core,
			ppm_get_power_state_name(new_state));

	for_each_ppm_clusters(i) {
		arch_get_cluster_cpus(&cluster_cpu, i);
		cpumask_and(&online_cpu, &cluster_cpu, cpu_online_mask);

		cl_status[i].core_num = cpumask_weight(&online_cpu);
		cl_status[i].volt = 0;	/* don't care */
		if (!cl_status[i].core_num)
			cl_status[i].freq_idx = -1;
		else
			cl_status[i].freq_idx = ppm_main_freq_to_idx(i,
					mt_cpufreq_get_cur_phy_freq_no_lock(i), CPUFREQ_RELATION_L);

		ppm_ver("[%d] core = %d, freq_idx = %d\n",
			i, cl_status[i].core_num, cl_status[i].freq_idx);
	}

#if PPM_COBRA_USE_CORE_LIMIT
	for_each_ppm_clusters(i) {
		if (cl_status[i].core_num > Core_limit[i])
			cl_status[i].core_num = Core_limit[i];
		if (req->limit[i].max_cpu_core < Core_limit[i])
			Core_limit[i] = req->limit[i].max_cpu_core;
	}
#endif

	/* TODO: check this! according to cluster migration design */
	if (cl_status[PPM_CLUSTER_LL].core_num == 0 && cl_status[PPM_CLUSTER_L].core_num == 0) {
		if (Core_limit[PPM_CLUSTER_LL] > 0) {
			cl_status[PPM_CLUSTER_LL].core_num = 1;
			cl_status[PPM_CLUSTER_LL].freq_idx = get_cluster_max_cpufreq_idx(PPM_CLUSTER_LL);
		} else {
			cl_status[PPM_CLUSTER_L].core_num = 1;
			cl_status[PPM_CLUSTER_L].freq_idx = get_cluster_max_cpufreq_idx(PPM_CLUSTER_L);
		}
	}

	/* use C2 (more powerful) frequency */
	if (cl_status[CLUSTER_ID_SHARED_C2].core_num > 0)
		cl_status[CLUSTER_ID_SHARED_C1].freq_idx = cl_status[CLUSTER_ID_SHARED_C2].freq_idx;

	/* check shared clusters' freq is limited or not */
	if (cl_status[CLUSTER_ID_SHARED_C1].freq_idx <= PREV_FREQ_LIMIT(SHARED_C1)
		|| cl_status[CLUSTER_ID_SHARED_C2].freq_idx <= PREV_FREQ_LIMIT(SHARED_C2))
		is_shared_freq_limited = 1;

	curr_power = ppm_find_pwr_idx(cl_status);
	if (curr_power < 0)
		curr_power = mt_ppm_thermal_get_max_power();
	delta_power = power_budget - curr_power;

	for_each_ppm_clusters(i) {
#if PPM_COBRA_NEED_OPP_MAPPING
		int *tbl = (i == CLUSTER_ID_SINGLE) ? fidx_mapping_tbl_single_r : fidx_mapping_tbl_shared_r;

		/* convert current OPP(frequency only) from 16 to 8 */
		opp[i] = (cl_status[i].freq_idx >= 0)
			? tbl[cl_status[i].freq_idx] : -1;
#else
		opp[i] = cl_status[i].freq_idx;
#endif

		/* Get Active Core number of each cluster */
		active_core[i] = (cl_status[i].core_num >= 0) ? cl_status[i].core_num : 0;

#if PPM_COBRA_USE_CORE_LIMIT
		core_limit_tmp[i] = Core_limit[i];
		req->limit[i].max_cpu_core = core_limit_tmp[i];
#endif
	}

	/* Which shared-cluster is active */
	shared_cluster_select = (ACT_CORE(SHARED_C2) > 0)
		? CLUSTER_ID_SHARED_C2
		: ((ACT_CORE(SHARED_C1) > 0) ? CLUSTER_ID_SHARED_C1 : CLUSTER_ID_SINGLE);

	ppm_dbg(COBRA_ALGO,
		"[IN](bgt/delta/cur)=(%d/%d/%d), (opp/act/c_lmt/f_lmt)=(%d,%d,%d/%d%d%d/%d%d%d/%d,%d,%d)\n",
				power_budget, delta_power, curr_power,
				opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2], opp[CLUSTER_ID_SINGLE],
				ACT_CORE(SHARED_C1), ACT_CORE(SHARED_C2), ACT_CORE(SINGLE),
				CORE_LIMIT(SHARED_C1), CORE_LIMIT(SHARED_C2), CORE_LIMIT(SINGLE),
				PREV_FREQ_LIMIT(SHARED_C1), PREV_FREQ_LIMIT(SHARED_C2), PREV_FREQ_LIMIT(SINGLE));

	/* increase ferquency limit */
	if (delta_power >= 0) {
		while (1) {
			int ChoosenCl = -1, MaxEff = 0, ChoosenPwr = 0;
			int target_delta_pwr, target_delta_eff;

			/* give remaining power to single-cluster if shared-cluster opp is 0 */
			/* TODO: check this! */
			if (opp[shared_cluster_select] == 0 && ACT_CORE(SINGLE) == 0) {
				target_delta_pwr = get_delta_pwr_single(1, COBRA_OPP_NUM-1);
				if (delta_power >= target_delta_pwr) {
					ACT_CORE(SINGLE) = 1;
					delta_power -= target_delta_pwr;
					opp[CLUSTER_ID_SINGLE] = COBRA_OPP_NUM - 1;
				}
			}

			/* Single-cluster */
			if (ACT_CORE(SINGLE) > 0 && opp[CLUSTER_ID_SINGLE] > 0) {
				target_delta_pwr = get_delta_pwr_single(ACT_CORE(SINGLE), opp[CLUSTER_ID_SINGLE]-1);
				if (delta_power >= target_delta_pwr) {
					MaxEff = get_delta_eff_single(ACT_CORE(SINGLE), opp[CLUSTER_ID_SINGLE]-1);
					ChoosenCl = CLUSTER_ID_SINGLE;
					ChoosenPwr = target_delta_pwr;
				}
			}

			/* Shared-cluster */
			if (shared_cluster_select != CLUSTER_ID_SINGLE && opp[shared_cluster_select] > 0
				&& (is_shared_freq_limited || opp[CLUSTER_ID_SINGLE] == 0 || ACT_CORE(SINGLE) == 0)) {
				target_delta_pwr = get_delta_pwr_shared(ACT_CORE(SHARED_C1), ACT_CORE(SHARED_C2),
					opp[shared_cluster_select]-1);
				target_delta_eff = get_delta_eff_shared(ACT_CORE(SHARED_C1), ACT_CORE(SHARED_C2),
					opp[shared_cluster_select]-1);
				if (delta_power >= target_delta_pwr && MaxEff <= target_delta_eff) {
					MaxEff = target_delta_eff;
					ChoosenCl = CLUSTER_ID_SHARED_C1;
					ChoosenPwr = target_delta_pwr;
				}
			}

			if (ChoosenCl != -1)
				goto prepare_next_round;

			/* exceed power budget or all active core is highest freq. */
#if PPM_COBRA_USE_CORE_LIMIT
			/* no enough budget */
			if (opp[shared_cluster_select] != 0)
				goto end;

			for_each_ppm_clusters(i) {
				int max_core = (new_state >= NR_PPM_POWER_STATE) ? get_cluster_max_cpu_core(i)
					: state_info[new_state].cluster_limit->state_limit[i].max_cpu_core;

				/* cannot turned on this cluster due to max_core limit of new_state is 0 */
				if (!max_core)
					continue;

				while (core_limit_tmp[i] < max_core) {
					target_delta_pwr = (i == CLUSTER_ID_SINGLE)
						? get_delta_pwr_single(CORE_LIMIT(SINGLE)+1, COBRA_OPP_NUM-1)
						: ((i == CLUSTER_ID_SHARED_C1)
						? get_delta_pwr_shared(CORE_LIMIT(SHARED_C1)+1,
							ACT_CORE(SHARED_C2), COBRA_OPP_NUM-1)
						: get_delta_pwr_shared(ACT_CORE(SHARED_C1),
							CORE_LIMIT(SHARED_C2)+1, COBRA_OPP_NUM-1));

					if (delta_power < target_delta_pwr)
						break;

					delta_power -= target_delta_pwr;
					req->limit[i].max_cpu_core = ++core_limit_tmp[i];
				}
			}

end:
#endif
			ppm_dbg(COBRA_ALGO, "[+]ChoosenCl=-1! delta=%d, (opp/c_lmt)=(%d,%d,%d/%d%d%d)\n",
				delta_power, opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2],
				opp[CLUSTER_ID_SINGLE], CORE_LIMIT(SHARED_C1),
				CORE_LIMIT(SHARED_C2), CORE_LIMIT(SINGLE));

			break;

prepare_next_round:
			/* if choose shared-cluster, change opp of active cluster only */
			if (ChoosenCl == CLUSTER_ID_SHARED_C1) {
				if (ACT_CORE(SHARED_C2) > 0)
					opp[CLUSTER_ID_SHARED_C2] -= 1;
				if (ACT_CORE(SHARED_C1) > 0)
					opp[CLUSTER_ID_SHARED_C1] -= 1;
			} else
				opp[ChoosenCl] -= 1;

			delta_power -= ChoosenPwr;

			ppm_dbg(COBRA_ALGO, "[+](delta/MaxEff/Cl/Pwr)=(%d,%d,%d,%d), opp=%d,%d,%d\n",
					delta_power, MaxEff, ChoosenCl, ChoosenPwr,
					opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2],
					opp[CLUSTER_ID_SINGLE]);
		}
	} else {
		while (delta_power < 0) {
			int ChoosenCl = -1;
			int MinEff = 10000;	/* should be bigger than max value of efficiency_* array */
			int ChoosenPwr = 0;
			int target_delta_eff;

			/* Single-cluster */
			if (ACT_CORE(SINGLE) > 0 && opp[CLUSTER_ID_SINGLE] < PPM_COBRA_MAX_FREQ_IDX) {
				MinEff = get_delta_eff_single(ACT_CORE(SINGLE), opp[CLUSTER_ID_SINGLE]);
				ChoosenCl = CLUSTER_ID_SINGLE;
				ChoosenPwr = get_delta_pwr_single(ACT_CORE(SINGLE), opp[CLUSTER_ID_SINGLE]);
			}

			/* Shared-cluster */
			if (shared_cluster_select != CLUSTER_ID_SINGLE
				&& opp[shared_cluster_select] < PPM_COBRA_MAX_FREQ_IDX) {
				target_delta_eff = get_delta_eff_shared(ACT_CORE(SHARED_C1),
							ACT_CORE(SHARED_C2), opp[shared_cluster_select]);
				if (MinEff > target_delta_eff) {
					MinEff = target_delta_eff;
					ChoosenCl = CLUSTER_ID_SHARED_C1;
					ChoosenPwr = get_delta_pwr_shared(ACT_CORE(SHARED_C1),
							ACT_CORE(SHARED_C2), opp[shared_cluster_select]);
				}
			}

			if (ChoosenCl == -1) {
				ppm_err("No lower OPP!(bgt/delta/cur)=(%d/%d/%d),(opp/act)=(%d,%d,%d/%d%d%d)\n",
					power_budget, delta_power, curr_power,
					opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2],
					opp[CLUSTER_ID_SINGLE], ACT_CORE(SHARED_C1),
					ACT_CORE(SHARED_C2), ACT_CORE(SINGLE));
				break;
			}

			/* if choose shared-cluster, change opp of active cluster only */
			if (ChoosenCl == CLUSTER_ID_SHARED_C1) {
				if (ACT_CORE(SHARED_C2) > 0)
					opp[CLUSTER_ID_SHARED_C2] += 1;
				if (ACT_CORE(SHARED_C1) > 0)
					opp[CLUSTER_ID_SHARED_C1] += 1;
			} else
				opp[ChoosenCl] += 1;

			/* Turned off core */
			/* TODO: check this! */
#if PPM_COBRA_USE_CORE_LIMIT
#if 0
			if (opp[PPM_CLUSTER_B] == PPM_COBRA_MAX_FREQ_IDX && core_limit_tmp[PPM_CLUSTER_B] > 0) {
				req->limit[PPM_CLUSTER_B].max_cpu_core = --ACT_CORE(SINGLE);
				opp[PPM_CLUSTER_B] = PPM_COBRA_MAX_FREQ_IDX - 1;
			} else if (opp[shared_cluster_select] == PPM_COBRA_MAX_FREQ_IDX) {
				if (ACT_CORE(SHARED_C2) > 1 || (ACT_CORE(SHARED_C1) > 0 && ACT_CORE(SHARED_C2) > 0))
					req->limit[PPM_CLUSTER_L].max_cpu_core = --ACT_CORE(SHARED_C2);
				else if (ACT_CORE(SHARED_C1) > 1)
					req->limit[PPM_CLUSTER_LL].max_cpu_core = --ACT_CORE(SHARED_C1);
				if (ACT_CORE(SHARED_C2) > 0)
					opp[PPM_CLUSTER_L] = PPM_COBRA_MAX_FREQ_IDX - 1;
				if (ACT_CORE(SHARED_C1) > 0)
					opp[PPM_CLUSTER_LL] = PPM_COBRA_MAX_FREQ_IDX - 1;
			}

#else
			for (i = NR_PPM_CLUSTERS-1; i >= 0; i--) {
				if (i == CLUSTER_ID_SINGLE
					&& opp[CLUSTER_ID_SINGLE] == PPM_COBRA_MAX_FREQ_IDX
					&& ACT_CORE(SINGLE) > 0) {
					req->limit[CLUSTER_ID_SINGLE].max_cpu_core = --ACT_CORE(SINGLE);
					opp[CLUSTER_ID_SINGLE] = PPM_COBRA_MAX_FREQ_IDX - 1;
					break;
				} else if (i != CLUSTER_ID_SINGLE
					&& opp[shared_cluster_select] == PPM_COBRA_MAX_FREQ_IDX) {

					if (ACT_CORE(SHARED_C2) > 1
						|| (ACT_CORE(SHARED_C1) > 0 && ACT_CORE(SHARED_C2) > 0))
						req->limit[CLUSTER_ID_SHARED_C2].max_cpu_core = --ACT_CORE(SHARED_C2);
					else if (ACT_CORE(SHARED_C1) > 1)
						req->limit[CLUSTER_ID_SHARED_C1].max_cpu_core = --ACT_CORE(SHARED_C1);

					if (ACT_CORE(SHARED_C2) > 0)
						opp[CLUSTER_ID_SHARED_C2] = PPM_COBRA_MAX_FREQ_IDX - 1;
					else
						/* all C2 core are off, check C1 in next round */
						shared_cluster_select = CLUSTER_ID_SHARED_C1;
					if (ACT_CORE(SHARED_C1) > 0)
						opp[CLUSTER_ID_SHARED_C1] = PPM_COBRA_MAX_FREQ_IDX - 1;

					break;
				}
			}
#endif
#endif

			delta_power += ChoosenPwr;
			curr_power -= ChoosenPwr;

			ppm_dbg(COBRA_ALGO, "[-](delta/MinEff/Cl/Pwr)=(%d,%d,%d,%d), (opp/act)=(%d,%d,%d/%d%d%d)\n",
					delta_power, MinEff, ChoosenCl, ChoosenPwr,
					opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2],
					opp[CLUSTER_ID_SINGLE], ACT_CORE(SHARED_C1),
					ACT_CORE(SHARED_C2), ACT_CORE(SINGLE));
		}
	}

	/* Set all frequency limit of the cluster */
	/* Set OPP of Cluser n to opp[n] */
	for_each_ppm_clusters(i) {
		if (i == CLUSTER_ID_SINGLE) {
			if (opp[i] >= 0 && ACT_CORE(SINGLE) > 0)
#if PPM_COBRA_NEED_OPP_MAPPING
				req->limit[i].max_cpufreq_idx = fidx_mapping_tbl_single[opp[i]];
#else
				req->limit[i].max_cpufreq_idx = opp[i];
#endif
			else
				req->limit[i].max_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
		} else
#if PPM_COBRA_NEED_OPP_MAPPING
			req->limit[i].max_cpufreq_idx =
				fidx_mapping_tbl_shared[opp[shared_cluster_select]];
#else
			req->limit[i].max_cpufreq_idx = opp[shared_cluster_select];
#endif
	}

	ppm_dbg(COBRA_ALGO, "[OUT]delta=%d, (opp/act/c_lmt/f_lmt)=(%d,%d,%d/%d%d%d/%d%d%d/%d,%d,%d)\n",
				delta_power, opp[CLUSTER_ID_SHARED_C1], opp[CLUSTER_ID_SHARED_C2],
				opp[CLUSTER_ID_SINGLE],	ACT_CORE(SHARED_C1), ACT_CORE(SHARED_C2), ACT_CORE(SINGLE),
				req->limit[CLUSTER_ID_SHARED_C1].max_cpu_core,
				req->limit[CLUSTER_ID_SHARED_C2].max_cpu_core,
				req->limit[CLUSTER_ID_SINGLE].max_cpu_core,
				req->limit[CLUSTER_ID_SHARED_C1].max_cpufreq_idx,
				req->limit[CLUSTER_ID_SHARED_C2].max_cpufreq_idx,
				req->limit[CLUSTER_ID_SINGLE].max_cpufreq_idx);

	/* error check */
	for_each_ppm_clusters(i) {
		if (req->limit[i].max_cpufreq_idx > req->limit[i].min_cpufreq_idx)
			req->limit[i].min_cpufreq_idx = req->limit[i].max_cpufreq_idx;
		if (req->limit[i].max_cpu_core < req->limit[i].min_cpu_core)
			req->limit[i].min_cpu_core = req->limit[i].max_cpu_core;
	}
}

void ppm_cobra_init(void)
{
	int i, j;
#if !PPM_COBRA_RUNTIME_CALC_DELTA
	int k;
#endif
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();

#if PPM_COBRA_NEED_OPP_MAPPING
	if (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) {
		fidx_mapping_tbl_shared = fidx_mapping_tbl_shared_sb;
		fidx_mapping_tbl_shared_r = fidx_mapping_tbl_shared_sb_r;
		fidx_mapping_tbl_single = fidx_mapping_tbl_single_sb;
		fidx_mapping_tbl_single_r = fidx_mapping_tbl_single_sb_r;
	} else {
		fidx_mapping_tbl_shared = fidx_mapping_tbl_shared_fy;
		fidx_mapping_tbl_shared_r = fidx_mapping_tbl_shared_fy_r;
		fidx_mapping_tbl_single = fidx_mapping_tbl_single_fy;
		fidx_mapping_tbl_single_r = fidx_mapping_tbl_single_fy_r;
	}
#endif

	for_each_ppm_clusters(i) {
		prev_max_cpufreq_idx[i] = 0;
		Core_limit[i] = get_cluster_max_cpu_core(i);
	}

	/* remap sram for cobra */
	cobra_tbl = ioremap_nocache(PPM_COBRA_TBL_SRAM_ADDR, PPM_COBRA_TBL_SRAM_SIZE);
	if (!cobra_tbl) {
		ppm_err("remap cobra_tbl failed!\n");
		WARN_ON(1);
		return;
	}

	ppm_info("addr of cobra_tbl = 0x%p, size = %lu\n", cobra_tbl, PPM_COBRA_TBL_SRAM_SIZE);
	memset_io((u8 *)cobra_tbl, 0x00, PPM_COBRA_TBL_SRAM_SIZE);

#ifdef CONFIG_MTK_UNIFY_POWER
	{
		unsigned int core, dyn, lkg, dyn_c, lkg_c, cap;

		/* generate basic power table */
		ppm_info("basic power table:\n");
		for (i = 0; i < TOTAL_CORE_NUM; i++) {
			for (j = 0; j < DVFS_OPP_NUM; j++) {
				core = (i % 4) + 1;
				dyn = upower_get_power(i/4, j, UPOWER_DYN);
				lkg = upower_get_power(i/4, j, UPOWER_LKG);
				dyn_c = upower_get_power(i/4+NR_PPM_CLUSTERS, j, UPOWER_DYN);
				lkg_c = upower_get_power(i/4+NR_PPM_CLUSTERS, j, UPOWER_LKG);
				cap = upower_get_power(i/4, j, UPOWER_CPU_STATES);

				cobra_tbl->basic_pwr_tbl[i][j].power_idx =
					((dyn + lkg) * core + (dyn_c + lkg_c)) / 1000;
				cobra_tbl->basic_pwr_tbl[i][j].perf_idx = cap * core;

				ppm_info("[%d][%d] = (%d, %d)\n", i, j,
					cobra_tbl->basic_pwr_tbl[i][j].power_idx,
					cobra_tbl->basic_pwr_tbl[i][j].perf_idx);
			}
		}
	}
#else
	for (i = 0; i < TOTAL_CORE_NUM; i++) {
		for (j = 0; j < DVFS_OPP_NUM; j++) {
			cobra_tbl->basic_pwr_tbl[i][j].power_idx = 0;
			cobra_tbl->basic_pwr_tbl[i][j].perf_idx = 0;
		}
	}
#endif

	/* decide min_pwr_idx and max_perf_idx for each state */
	for_each_ppm_power_state(i) {
		for_each_ppm_clusters(j) {
			int max_core = state_info[i].cluster_limit->state_limit[j].max_cpu_core;
			int min_core = state_info[i].cluster_limit->state_limit[j].min_cpu_core;
			int idx = get_idx_in_pwr_tbl(j);

			if (max_core > get_cluster_max_cpu_core(j) || min_core < get_cluster_min_cpu_core(j))
				continue;

			state_info[i].min_pwr_idx += (min_core)
				? cobra_tbl->basic_pwr_tbl[idx+min_core-1][DVFS_OPP_NUM-1].power_idx : 0;
			state_info[i].max_perf_idx += (max_core)
				? cobra_tbl->basic_pwr_tbl[idx+max_core-1][0].perf_idx : 0;
		}
		ppm_info("%s: min_pwr_idx = %d, max_perf_idx = %d\n", state_info[i].name,
			state_info[i].min_pwr_idx, state_info[i].max_perf_idx);
	}

#if !PPM_COBRA_RUNTIME_CALC_DELTA
	/* generate delta power and delta perf table for single(non-shared) buck cluster */
	ppm_info("Single-cluster delta table:\n");
	for (i = 0; i <= get_cluster_max_cpu_core(CLUSTER_ID_SINGLE); i++) {
		for (j = 0; j < COBRA_OPP_NUM; j++) {
			int idx = get_idx_in_pwr_tbl(CLUSTER_ID_SINGLE);
#if PPM_COBRA_NEED_OPP_MAPPING
			int opp = fidx_mapping_tbl_single[j];
#else
			int opp = j;
#endif
			int prev_opp;

			if (i == 0) {
				cobra_tbl->delta_tbl_single[i][j].delta_pwr = 0;
				cobra_tbl->delta_tbl_single[i][j].delta_perf = 0;
				cobra_tbl->delta_tbl_single[i][j].delta_eff = 0;

				ppm_info("[%d][%d] = (0, 0, 0)\n", i, j);
				continue;
			}

			if (j == COBRA_OPP_NUM - 1) {
				cobra_tbl->delta_tbl_single[i][j].delta_pwr = (i == 1)
					? cobra_tbl->basic_pwr_tbl[idx+i-1][opp].power_idx
					: (cobra_tbl->basic_pwr_tbl[idx+i-1][opp].power_idx -
					cobra_tbl->basic_pwr_tbl[idx+i-2][opp].power_idx);
				cobra_tbl->delta_tbl_single[i][j].delta_perf = (i == 1)
					? cobra_tbl->basic_pwr_tbl[idx+i-1][opp].perf_idx
					: (cobra_tbl->basic_pwr_tbl[idx+i-1][opp].perf_idx -
					cobra_tbl->basic_pwr_tbl[idx+i-2][opp].perf_idx);
				/* x10 to make it hard to turn off cores */
				cobra_tbl->delta_tbl_single[i][j].delta_eff =
					(cobra_tbl->delta_tbl_single[i][j].delta_perf * 1000) /
					cobra_tbl->delta_tbl_single[i][j].delta_pwr;
			} else {
#if PPM_COBRA_NEED_OPP_MAPPING
				prev_opp = fidx_mapping_tbl_single[j+1];
#else
				prev_opp = j+1;
#endif
				cobra_tbl->delta_tbl_single[i][j].delta_pwr =
					cobra_tbl->basic_pwr_tbl[idx+i-1][opp].power_idx -
					cobra_tbl->basic_pwr_tbl[idx+i-1][prev_opp].power_idx;
				cobra_tbl->delta_tbl_single[i][j].delta_perf =
					cobra_tbl->basic_pwr_tbl[idx+i-1][opp].perf_idx -
					cobra_tbl->basic_pwr_tbl[idx+i-1][prev_opp].perf_idx;
				cobra_tbl->delta_tbl_single[i][j].delta_eff =
					(cobra_tbl->delta_tbl_single[i][j].delta_perf * 100) /
					cobra_tbl->delta_tbl_single[i][j].delta_pwr;
			}

			ppm_info("[%d][%d] = (%d, %d, %d)\n", i, j,
				cobra_tbl->delta_tbl_single[i][j].delta_pwr,
				cobra_tbl->delta_tbl_single[i][j].delta_perf,
				cobra_tbl->delta_tbl_single[i][j].delta_eff);
		}
	}

	/* generate delta power and delta perf table for shared-buck clusters */
	ppm_info("Shared-cluster delta table:\n");
	for (i = 0; i <= get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C2); i++) {
		for (j = 0; j <= get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C1); j++) {
			for (k = 0; k < COBRA_OPP_NUM; k++) {
				int idx_c1 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C1);
				int idx_c2 = get_idx_in_pwr_tbl(CLUSTER_ID_SHARED_C2);
#if PPM_COBRA_NEED_OPP_MAPPING
				int opp = fidx_mapping_tbl_shared[k];
#else
				int opp = k;
#endif
				int cur_pwr, cur_perf, prev_pwr, prev_perf, prev_opp;

				if (i == 0 && j == 0) {
					cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr = 0;
					cobra_tbl->delta_tbl_shared[i][j][k].delta_perf = 0;
					cobra_tbl->delta_tbl_shared[i][j][k].delta_eff = 0;

					ppm_info("[%d][%d][%d] = (0, 0, 0)\n", i, j, k);
					continue;
				}

				cur_pwr = (i) ? cobra_tbl->basic_pwr_tbl[idx_c2+i-1][opp].power_idx : 0;
				cur_pwr += (j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].power_idx : 0;
				cur_perf = (i) ? cobra_tbl->basic_pwr_tbl[idx_c2+i-1][opp].perf_idx : 0;
				cur_perf += (j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].perf_idx : 0;

				if (k == COBRA_OPP_NUM - 1) {
					prev_pwr = (i) ?
						((i > 1) ? (cobra_tbl->basic_pwr_tbl[idx_c2+i-2][opp].power_idx +
						((j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].power_idx : 0))
						: ((j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].power_idx : 0))
						: ((j > 1) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-2][opp].power_idx : 0);
					prev_perf = (i) ?
						((i > 1) ? (cobra_tbl->basic_pwr_tbl[idx_c2+i-2][opp].perf_idx +
						((j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].perf_idx : 0))
						: ((j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][opp].perf_idx : 0))
						: ((j > 1) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-2][opp].perf_idx : 0);

					cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr = cur_pwr - prev_pwr;
					cobra_tbl->delta_tbl_shared[i][j][k].delta_perf = cur_perf - prev_perf;
					/* x10 to make it hard to turn off cores */
					cobra_tbl->delta_tbl_shared[i][j][k].delta_eff =
						(cobra_tbl->delta_tbl_shared[i][j][k].delta_perf * 1000) /
						cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr;
				} else {
#if PPM_COBRA_NEED_OPP_MAPPING
					prev_opp = fidx_mapping_tbl_shared[k+1];
#else
					prev_opp = k+1;
#endif
					prev_pwr = (i) ? cobra_tbl->basic_pwr_tbl[idx_c2+i-1][prev_opp].power_idx : 0;
					prev_pwr += (j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][prev_opp].power_idx : 0;
					prev_perf = (i) ? cobra_tbl->basic_pwr_tbl[idx_c2+i-1][prev_opp].perf_idx : 0;
					prev_perf += (j) ? cobra_tbl->basic_pwr_tbl[idx_c1+j-1][prev_opp].perf_idx : 0;

					cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr = cur_pwr - prev_pwr;
					cobra_tbl->delta_tbl_shared[i][j][k].delta_perf = cur_perf - prev_perf;
					cobra_tbl->delta_tbl_shared[i][j][k].delta_eff =
						(cobra_tbl->delta_tbl_shared[i][j][k].delta_perf * 100) /
						cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr;
				}

				ppm_info("[%d][%d][%d] = (%d, %d, %d)\n", i, j, k,
					cobra_tbl->delta_tbl_shared[i][j][k].delta_pwr,
					cobra_tbl->delta_tbl_shared[i][j][k].delta_perf,
					cobra_tbl->delta_tbl_shared[i][j][k].delta_eff);
			}
		}
	}
#endif

	ppm_info("COBRA init done!\n");
}


void ppm_cobra_dump_tbl(struct seq_file *m)
{
	int i, j, k;

	seq_puts(m, "\n=======================================\n");
	seq_puts(m, "basic power table (pwr, perf)");
	seq_puts(m, "\n=======================================\n");
	for (i = 0; i < TOTAL_CORE_NUM; i++) {
		for (j = 0; j < DVFS_OPP_NUM; j++) {
			seq_printf(m, "[%d][%d] = (%d, %d)\n", i, j,
				cobra_tbl->basic_pwr_tbl[i][j].power_idx,
				cobra_tbl->basic_pwr_tbl[i][j].perf_idx);
		}
	}

	if (ppm_debug > 0) {
		seq_puts(m, "\n=======================================================================\n");
		seq_puts(m, "Single(Non-shared) cluster delta table (delta_pwr, delta_perf, delta_eff)");
		seq_puts(m, "\n=======================================================================\n");
		for (i = 0; i <= get_cluster_max_cpu_core(CLUSTER_ID_SINGLE); i++) {
			for (j = 0; j < COBRA_OPP_NUM; j++) {
				seq_printf(m, "[%d][%d] = (%d, %d, %d)\n", i, j,
					get_delta_pwr_single(i, j),
					get_delta_perf_single(i, j),
					get_delta_eff_single(i, j));
			}
		}

		seq_puts(m, "\n===========================================================\n");
		seq_puts(m, "Shared cluster delta table (delta_pwr, delta_perf, delta_eff)");
		seq_puts(m, "\n===========================================================\n");
		for (i = 0; i <= get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C2); i++) {
			for (j = 0; j <= get_cluster_max_cpu_core(CLUSTER_ID_SHARED_C1); j++) {
				for (k = 0; k < COBRA_OPP_NUM; k++) {
					seq_printf(m, "[%d][%d][%d] = (%d, %d, %d)\n", i, j, k,
						get_delta_pwr_shared(j, i, k),
						get_delta_perf_shared(j, i, k),
						get_delta_eff_shared(j, i, k));
				}
			}
		}
	}
}

static unsigned int get_limit_opp_and_budget(void)
{
	unsigned int power = 0;
	int i, j, k, idx;

	for (i = 0; i <= get_cluster_min_cpufreq_idx(CLUSTER_ID_SHARED_C1); i++) {
		cobra_lookup_data.limit[CLUSTER_ID_SHARED_C1].opp = i;
		cobra_lookup_data.limit[CLUSTER_ID_SHARED_C2].opp = i;

		for (j = 0; j <= get_cluster_min_cpufreq_idx(CLUSTER_ID_SINGLE); j++) {
			cobra_lookup_data.limit[CLUSTER_ID_SINGLE].opp = j;

			for_each_ppm_clusters(k) {
				if (!cobra_lookup_data.limit[k].core)
					continue;

				idx = get_idx_in_pwr_tbl(k) + cobra_lookup_data.limit[k].core - 1;
				power += (k == CLUSTER_ID_SINGLE)
					? cobra_tbl->basic_pwr_tbl[idx][j].power_idx
					: cobra_tbl->basic_pwr_tbl[idx][i].power_idx;
			}

			if (power <= cobra_lookup_data.budget)
				return power;

			power = 0;
		}
	}

	return power;
}

static void ppm_cobra_lookup_by_budget(struct seq_file *m)
{
	int i, j, k;
	unsigned int power;

	seq_puts(m, "\n========================================================\n");
	seq_puts(m, "(LL_core, LL_opp, L_core, L_opp, B_core, B_opp) = power");
	seq_puts(m, "\n========================================================\n");

	seq_printf(m, "Input budget = %d\n\n", cobra_lookup_data.budget);

	for (i = get_cluster_max_cpu_core(PPM_CLUSTER_B); i >= 0; i--) {
		for (j = get_cluster_max_cpu_core(PPM_CLUSTER_L); j >= 0; j--) {
			for (k = get_cluster_max_cpu_core(PPM_CLUSTER_LL); k >= 0; k--) {
				if (!i && !j && !k)
					continue;

				cobra_lookup_data.limit[PPM_CLUSTER_LL].core = k;
				cobra_lookup_data.limit[PPM_CLUSTER_L].core = j;
				cobra_lookup_data.limit[PPM_CLUSTER_B].core = i;
				power = get_limit_opp_and_budget();

				if (power) {
					seq_printf(m, "(%d, %2d, %d, %2d, %d, %2d) = %4d\n",
						k, cobra_lookup_data.limit[PPM_CLUSTER_LL].opp,
						j, cobra_lookup_data.limit[PPM_CLUSTER_L].opp,
						i, cobra_lookup_data.limit[PPM_CLUSTER_B].opp,
						power);
				}
			}
		}
	}
}

static void ppm_cobra_lookup_by_limit(struct seq_file *m)
{
	unsigned int budget = 0, core, opp, i;

	for_each_ppm_clusters(i) {
		core = (cobra_lookup_data.limit[i].core > get_cluster_max_cpu_core(i))
			? get_cluster_max_cpu_core(i) : cobra_lookup_data.limit[i].core;
		opp = (cobra_lookup_data.limit[i].opp > get_cluster_min_cpufreq_idx(i))
			? get_cluster_min_cpufreq_idx(i) : cobra_lookup_data.limit[i].opp;

		if (core)
			budget += cobra_tbl->basic_pwr_tbl[4*i+core-1][opp].power_idx;

		seq_printf(m, "Cluster %d: core = %d, opp = %d\n", i, core, opp);
	}

	seq_printf(m, "Budget = %d mW\n", budget);
}

extern void ppm_cobra_lookup_get_result(struct seq_file *m, enum ppm_cobra_lookup_type type)
{
	switch (type) {
	case LOOKUP_BY_BUDGET:
		ppm_cobra_lookup_by_budget(m);
		break;
	case LOOKUP_BY_LIMIT:
		ppm_cobra_lookup_by_limit(m);
		break;
	default:
		ppm_err("Invalid lookup type %d\n", type);
		break;
	}
}

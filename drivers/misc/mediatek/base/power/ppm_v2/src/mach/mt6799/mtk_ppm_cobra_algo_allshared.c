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

#include "mtk_ppm_platform.h"
#include "mtk_ppm_internal.h"
#include "mtk_unified_power.h"
#include <linux/io.h>


struct ppm_cobra_data *cobra_tbl;
struct ppm_cobra_lookup cobra_lookup_data;

static int Core_limit[NR_PPM_CLUSTERS];
/* TODO: check idx mapping */
#if PPM_COBRA_NEED_OPP_MAPPING
static int fidx_mapping_tbl_fy[COBRA_OPP_NUM] = {0, 3, 5, 7, 9, 10, 12, 14};
static int fidx_mapping_tbl_fy_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 7, 7};
static int fidx_mapping_tbl_sb[COBRA_OPP_NUM] = {0, 3, 6, 8, 9, 11, 12, 14};
static int fidx_mapping_tbl_sb_r[DVFS_OPP_NUM] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 7};

int *fidx_mapping_tbl;
int *fidx_mapping_tbl_r;
#endif

#define ACT_CORE(cluster)	(active_core[PPM_CLUSTER_##cluster])
#define CORE_LIMIT(cluster)	(core_limit_tmp[PPM_CLUSTER_##cluster])


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

static short get_delta_pwr(unsigned int core_LL, unsigned int core_L,
			unsigned int core_B, unsigned int opp)
{
	unsigned int idx_LL, idx_L, idx_B;
	unsigned int cur_opp, prev_opp;
	unsigned short cur_pwr, prev_pwr;
	short delta_pwr;

	if (core_LL > get_cluster_max_cpu_core(PPM_CLUSTER_LL)
		|| core_L > get_cluster_max_cpu_core(PPM_CLUSTER_L)
		|| core_B > get_cluster_max_cpu_core(PPM_CLUSTER_B)
		|| opp > COBRA_OPP_NUM) {
		ppm_err("%s: Invalid input: core_LL=%d, core_L=%d, core_B=%d, opp=%d\n",
			__func__, core_LL, core_L, core_B, opp);
		return 0;
	}

	if (core_LL == 0 && core_L == 0 && core_B == 0)
		return 0;

	idx_LL = get_idx_in_pwr_tbl(PPM_CLUSTER_LL);
	idx_L = get_idx_in_pwr_tbl(PPM_CLUSTER_L);
	idx_B = get_idx_in_pwr_tbl(PPM_CLUSTER_B);

#if PPM_COBRA_NEED_OPP_MAPPING
	cur_opp = fidx_mapping_tbl[opp];
	prev_opp = fidx_mapping_tbl[opp+1];
#else
	cur_opp = opp;
	prev_opp = opp + 1;
#endif
	cur_pwr = (core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][cur_opp].power_idx : 0;
	cur_pwr += (core_L) ? cobra_tbl->basic_pwr_tbl[idx_L+core_L-1][cur_opp].power_idx : 0;
	cur_pwr += (core_B) ? cobra_tbl->basic_pwr_tbl[idx_B+core_B-1][cur_opp].power_idx : 0;

	if (opp == COBRA_OPP_NUM - 1) {
		switch (core_B) {
		case 0:
			prev_pwr = (core_L) ?
				((core_L > 1) ? (cobra_tbl->basic_pwr_tbl[idx_L+core_L-2][cur_opp].power_idx +
				((core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][cur_opp].power_idx : 0))
				: ((core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][cur_opp].power_idx : 0))
				: ((core_LL > 1) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-2][cur_opp].power_idx : 0);
			break;
		case 1:
			prev_pwr = (core_L) ? cobra_tbl->basic_pwr_tbl[idx_L+core_L-1][cur_opp].power_idx : 0;
			prev_pwr += (core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][cur_opp].power_idx : 0;
			break;
		case 2:
		default:
			prev_pwr = cobra_tbl->basic_pwr_tbl[idx_B+core_B-2][cur_opp].power_idx;
			prev_pwr += (core_L) ? cobra_tbl->basic_pwr_tbl[idx_L+core_L-1][cur_opp].power_idx : 0;
			prev_pwr += (core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][cur_opp].power_idx : 0;
			break;
		}
	} else {
		prev_pwr = (core_B) ? cobra_tbl->basic_pwr_tbl[idx_B+core_B-1][prev_opp].power_idx : 0;
		prev_pwr += (core_L) ? cobra_tbl->basic_pwr_tbl[idx_L+core_L-1][prev_opp].power_idx : 0;
		prev_pwr += (core_LL) ? cobra_tbl->basic_pwr_tbl[idx_LL+core_LL-1][prev_opp].power_idx : 0;
	}

	delta_pwr = cur_pwr - prev_pwr;

	return delta_pwr;
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
	/* NO NEED */
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
			Core_limit[PPM_CLUSTER_LL],
			Core_limit[PPM_CLUSTER_L],
			Core_limit[PPM_CLUSTER_B],
			req->limit[PPM_CLUSTER_LL].max_cpu_core,
			req->limit[PPM_CLUSTER_L].max_cpu_core,
			req->limit[PPM_CLUSTER_B].max_cpu_core,
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

	/* use most powerful frequency between shared clusters */
	if (cl_status[PPM_CLUSTER_B].core_num > 0) {
		cl_status[PPM_CLUSTER_L].freq_idx = cl_status[PPM_CLUSTER_B].freq_idx;
		cl_status[PPM_CLUSTER_LL].freq_idx = cl_status[PPM_CLUSTER_B].freq_idx;
	} else if (cl_status[PPM_CLUSTER_L].core_num > 0)
		cl_status[PPM_CLUSTER_LL].freq_idx = cl_status[PPM_CLUSTER_L].freq_idx;

	curr_power = ppm_find_pwr_idx(cl_status);
	if (curr_power < 0)
		curr_power = mt_ppm_thermal_get_max_power();
	delta_power = power_budget - curr_power;

	for_each_ppm_clusters(i) {
#if PPM_COBRA_NEED_OPP_MAPPING
		/* convert current OPP(frequency only) from 16 to 8 */
		opp[i] = (cl_status[i].freq_idx >= 0)
			? fidx_mapping_tbl_r[cl_status[i].freq_idx] : -1;
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
	shared_cluster_select = (ACT_CORE(B) > 0) ? PPM_CLUSTER_B
		: ((ACT_CORE(L) > 0) ? PPM_CLUSTER_L : PPM_CLUSTER_LL);

	ppm_dbg(COBRA_ALGO,
		"[IN](bgt/delta/cur)=(%d/%d/%d), (opp/act/c_lmt)=(%d,%d,%d/%d%d%d/%d%d%d)\n",
				power_budget, delta_power, curr_power,
				opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B],
				ACT_CORE(LL), ACT_CORE(L), ACT_CORE(B),
				CORE_LIMIT(LL), CORE_LIMIT(L), CORE_LIMIT(B));

	/* increase ferquency limit */
	if (delta_power >= 0) {
		while (1) {
			int ChoosenPwr = 0;
			int target_delta_pwr;

			/* Shared-cluster */
			if (opp[shared_cluster_select] > 0) {
				target_delta_pwr = get_delta_pwr(ACT_CORE(LL), ACT_CORE(L), ACT_CORE(B),
					opp[shared_cluster_select]-1);
				if (delta_power >= target_delta_pwr) {
					ChoosenPwr = target_delta_pwr;

					goto prepare_next_round;
				}
			}

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
					target_delta_pwr = (i == PPM_CLUSTER_LL)
						? get_delta_pwr(CORE_LIMIT(LL)+1, ACT_CORE(L),
							ACT_CORE(B), COBRA_OPP_NUM-1)
						: ((i == PPM_CLUSTER_L)
						? get_delta_pwr(ACT_CORE(LL), CORE_LIMIT(L)+1,
							ACT_CORE(B), COBRA_OPP_NUM-1)
						: get_delta_pwr(ACT_CORE(LL), ACT_CORE(L),
							CORE_LIMIT(B)+1, COBRA_OPP_NUM-1));

					if (delta_power < target_delta_pwr)
						break;

					delta_power -= target_delta_pwr;
					req->limit[i].max_cpu_core = ++core_limit_tmp[i];
				}
			}

end:
#endif
			ppm_dbg(COBRA_ALGO, "[+] No higher OPP! delta=%d, (opp/c_lmt)=(%d,%d,%d/%d%d%d)\n",
				delta_power, opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B],
				CORE_LIMIT(LL),	CORE_LIMIT(L), CORE_LIMIT(B));
			break;

prepare_next_round:
			for_each_ppm_clusters(i) {
				if (active_core[i] > 0)
					opp[i] -= 1;
			}
			delta_power -= ChoosenPwr;

			ppm_dbg(COBRA_ALGO, "[+](delta/Pwr)=(%d,%d), opp=%d,%d,%d\n",
					delta_power, ChoosenPwr,
					opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B]);
		}
	} else {
		while (delta_power < 0) {
			int ChoosenPwr = 0;

			/* Shared-cluster */
			if (opp[shared_cluster_select] < PPM_COBRA_MAX_FREQ_IDX) {
				ChoosenPwr = get_delta_pwr(ACT_CORE(LL), ACT_CORE(L),
						ACT_CORE(B), opp[shared_cluster_select]);

				for_each_ppm_clusters(i) {
					if (active_core[i] > 0)
						opp[i] += 1;
				}
			} else {
				ppm_err("No lower OPP!(bgt/delta/cur)=(%d/%d/%d),(opp/act)=(%d,%d,%d/%d%d%d)\n",
					power_budget, delta_power, curr_power,
					opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B],
					ACT_CORE(LL), ACT_CORE(L), ACT_CORE(B));
				break;
			}

			/* Turned off core */
#if PPM_COBRA_USE_CORE_LIMIT
			if (opp[shared_cluster_select] == PPM_COBRA_MAX_FREQ_IDX) {
				if (ACT_CORE(B) > 1 || ((ACT_CORE(LL) || ACT_CORE(L)) && ACT_CORE(B)))
					req->limit[PPM_CLUSTER_B].max_cpu_core = --ACT_CORE(B);
				else if (ACT_CORE(L) > 1 || (ACT_CORE(LL) && ACT_CORE(L)))
					req->limit[PPM_CLUSTER_L].max_cpu_core = --ACT_CORE(L);
				else if (ACT_CORE(LL) > 1)
					req->limit[PPM_CLUSTER_LL].max_cpu_core = --ACT_CORE(LL);

				for (i = NR_PPM_CLUSTERS-1; i >= 0; i--) {
					if (active_core[i] > 0)
						opp[i] = PPM_COBRA_MAX_FREQ_IDX - 1;
					else if (i != 0 && i == shared_cluster_select)
						shared_cluster_select = i - 1;
				}
			}
#endif

			delta_power += ChoosenPwr;
			curr_power -= ChoosenPwr;

			ppm_dbg(COBRA_ALGO, "[-](delta/Pwr)=(%d,%d), (opp/act)=(%d,%d,%d/%d%d%d)\n",
					delta_power, ChoosenPwr,
					opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B],
					ACT_CORE(LL), ACT_CORE(L), ACT_CORE(B));
		}
	}

	/* Set all frequency limit of the cluster */
	/* Set OPP of Cluser n to opp[n] */
	for_each_ppm_clusters(i) {
#if PPM_COBRA_NEED_OPP_MAPPING
		req->limit[i].max_cpufreq_idx =	fidx_mapping_tbl[opp[shared_cluster_select]];
#else
		req->limit[i].max_cpufreq_idx = opp[shared_cluster_select];
#endif
	}

	ppm_dbg(COBRA_ALGO, "[OUT]delta=%d, (opp/act/c_lmt/f_lmt)=(%d,%d,%d/%d%d%d/%d%d%d/%d,%d,%d)\n",
				delta_power, opp[PPM_CLUSTER_LL], opp[PPM_CLUSTER_L], opp[PPM_CLUSTER_B],
				ACT_CORE(LL), ACT_CORE(L), ACT_CORE(B),
				req->limit[PPM_CLUSTER_LL].max_cpu_core,
				req->limit[PPM_CLUSTER_L].max_cpu_core,
				req->limit[PPM_CLUSTER_B].max_cpu_core,
				req->limit[PPM_CLUSTER_LL].max_cpufreq_idx,
				req->limit[PPM_CLUSTER_L].max_cpufreq_idx,
				req->limit[PPM_CLUSTER_B].max_cpufreq_idx);

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
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();

#if PPM_COBRA_NEED_OPP_MAPPING
	if (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) {
		fidx_mapping_tbl = fidx_mapping_tbl_sb;
		fidx_mapping_tbl_r = fidx_mapping_tbl_sb_r;
	} else {
		fidx_mapping_tbl = fidx_mapping_tbl_fy;
		fidx_mapping_tbl_r = fidx_mapping_tbl_fy_r;
	}
#endif

	for_each_ppm_clusters(i)
		Core_limit[i] = get_cluster_max_cpu_core(i);

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

	ppm_info("COBRA init done!\n");
}


void ppm_cobra_dump_tbl(struct seq_file *m)
{
	int i, j, k, l;

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

	if (ppm_debug <= 0)
		return;

	seq_puts(m, "\n===========================================================\n");
	seq_puts(m, "Shared cluster delta table (delta_pwr)");
	seq_puts(m, "\n===========================================================\n");
	for (i = 0; i <= get_cluster_max_cpu_core(PPM_CLUSTER_B); i++) {
		for (j = 0; j <= get_cluster_max_cpu_core(PPM_CLUSTER_L); j++) {
			for (k = 0; k <= get_cluster_max_cpu_core(PPM_CLUSTER_LL); k++) {
				for (l = 0; l < COBRA_OPP_NUM; l++) {
					seq_printf(m, "[%d][%d][%d] OPP %d = (%d)\n", k, j, i, l,
						get_delta_pwr(k, j, i, l));
				}
			}
		}
	}
}

static unsigned int get_limit_opp_and_budget(void)
{
	unsigned int power = 0;
	int i, j, idx;

	for (i = 0; i <= get_cluster_min_cpufreq_idx(PPM_CLUSTER_LL); i++) {
		cobra_lookup_data.limit[PPM_CLUSTER_LL].opp = i;
		cobra_lookup_data.limit[PPM_CLUSTER_L].opp = i;
		cobra_lookup_data.limit[PPM_CLUSTER_B].opp = i;

		for_each_ppm_clusters(j) {
			if (!cobra_lookup_data.limit[j].core)
				continue;

			idx = get_idx_in_pwr_tbl(j) + cobra_lookup_data.limit[j].core - 1;
			power += cobra_tbl->basic_pwr_tbl[idx][i].power_idx;
		}

		if (power <= cobra_lookup_data.budget)
			return power;

		power = 0;
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

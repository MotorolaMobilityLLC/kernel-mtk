/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include "mt_cpufreq.h"
#include "mt_ppm_platform.h"
#include "mt_ppm_internal.h"

/*==============================================================*/
/* Macros							*/
/*==============================================================*/
#define PWR_STATE_TBLS(name, type) \
	.cluster_limit = &cluster_limit_##name,	\
	.pwr_sorted_tbl = &pwr_sorted_tbl_##name##_##type,	\
	.perf_sorted_tbl = &perf_sorted_tbl_##name##_##type,	\
	.transfer_by_pwr = &transfer_by_pwr_##name,	\
	.transfer_by_perf = &transfer_by_perf_##name,

#define LIMIT(fmin, fmax, cmin, cmax) {	\
	.min_cpufreq_idx = fmin,	\
	.max_cpufreq_idx = fmax,	\
	.min_cpu_core = cmin,	\
	.max_cpu_core = cmax,	\
}

#define STATE_LIMIT(name)	\
const struct ppm_state_cluster_limit_data cluster_limit_##name = {	\
	.state_limit = state_limit_##name,		\
	.size = ARRAY_SIZE(state_limit_##name),	\
}

#define SORT_TBL_ELEMENT(idx, val, advise) {	\
	.index = idx,	\
	.value = val,	\
	.advise_index = advise,	\
}

#define STATE_PWR_TBL(name, type)	\
const struct ppm_state_sorted_pwr_tbl_data pwr_sorted_tbl_##name##_##type = {	\
	.sorted_tbl = state_pwr_tbl_##name##_##type,		\
	.size = ARRAY_SIZE(state_pwr_tbl_##name##_##type),	\
}

#define STATE_PERF_TBL(name, type)	\
const struct ppm_state_sorted_pwr_tbl_data perf_sorted_tbl_##name##_##type = {	\
	.sorted_tbl = state_perf_tbl_##name##_##type,		\
	.size = ARRAY_SIZE(state_perf_tbl_##name##_##type),	\
}

#define TRANS_DATA(state, mask, rule, delta, hold, bond, f_hold, tlp) {	\
	.next_state = PPM_POWER_STATE_##state,	\
	.mode_mask = mask,	\
	.transition_rule = rule,	\
	.loading_delta = delta,	\
	.loading_hold_time = hold,	\
	.loading_hold_cnt = 0,	\
	.loading_bond = bond,	\
	.freq_hold_time = f_hold,	\
	.freq_hold_cnt = 0,		\
	.tlp_bond = tlp,	\
}

#define STATE_TRANSFER_DATA_PWR(name)	\
struct ppm_state_transfer_data transfer_by_pwr_##name = {	\
	.transition_data = state_pwr_transfer_##name,		\
	.size = ARRAY_SIZE(state_pwr_transfer_##name),	\
}

#define STATE_TRANSFER_DATA_PERF(name)	\
struct ppm_state_transfer_data transfer_by_perf_##name = {	\
	.transition_data = state_perf_transfer_##name,		\
	.size = ARRAY_SIZE(state_perf_transfer_##name),	\
}

/*==============================================================*/
/* Local Functions						*/
/*==============================================================*/
static bool ppm_trans_rule_LL_ONLY_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_LL_ONLY_to_4LL_L(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_L_ONLY_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_L_ONLY_to_4L_LL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_4LL_L_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_4L_LL_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);

/*==============================================================*/
/* Local Variables						*/
/*==============================================================*/
/* cluster limit for each power state */
static const struct ppm_cluster_limit state_limit_LL_ONLY[] = {
	[0] = LIMIT(7, 0, 1, 4),
	[1] = LIMIT(7, 0, 0, 0),
};
STATE_LIMIT(LL_ONLY);

static const struct ppm_cluster_limit state_limit_L_ONLY[] = {
	[0] = LIMIT(7, 0, 0, 0),
	[1] = LIMIT(5, 0, 1, 4),
};
STATE_LIMIT(L_ONLY);

static const struct ppm_cluster_limit state_limit_4LL_L[] = {
	[0] = LIMIT(7, 0, 4, 4),
	[1] = LIMIT(7, 0, 1, 4),
};
STATE_LIMIT(4LL_L);

static const struct ppm_cluster_limit state_limit_4L_LL[] = {
	[0] = LIMIT(7, 0, 1, 4),
	[1] = LIMIT(7, 0, 4, 4),
};
STATE_LIMIT(4L_LL);

/* state transfer data  by power/performance for each state */
static struct ppm_state_transfer state_pwr_transfer_LL_ONLY[] = {
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
};
STATE_TRANSFER_DATA_PWR(LL_ONLY);

static struct ppm_state_transfer state_perf_transfer_LL_ONLY[] = {
	TRANS_DATA(
		4LL_L,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_LL_ONLY_to_4LL_L,
		PPM_DEFAULT_DELTA,
		PPM_DEFAULT_HOLD_TIME,
		PPM_LOADING_UPPER,
		0,
		PPM_TLP_CRITERIA
		),
	TRANS_DATA(
		L_ONLY,
		PPM_MODE_MASK_JUST_MAKE_ONLY | PPM_MODE_MASK_PERFORMANCE_ONLY,
		ppm_trans_rule_LL_ONLY_to_L_ONLY,
		PPM_DEFAULT_DELTA,
		PPM_DEFAULT_HOLD_TIME,
		PPM_LOADING_UPPER,
		/* PPM_DEFAULT_FREQ_HOLD_TIME, */
		8,
		PPM_TLP_CRITERIA
		),
};
STATE_TRANSFER_DATA_PERF(LL_ONLY);

static struct ppm_state_transfer state_pwr_transfer_L_ONLY[] = {
	TRANS_DATA(
		LL_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_L_ONLY_to_LL_ONLY,
		0,
		0,
		0,
		PPM_DEFAULT_FREQ_HOLD_TIME,
		0
		),
};
STATE_TRANSFER_DATA_PWR(L_ONLY);

static struct ppm_state_transfer state_perf_transfer_L_ONLY[] = {
	TRANS_DATA(
		4L_LL,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_L_ONLY_to_4L_LL,
		PPM_DEFAULT_DELTA,
		PPM_DEFAULT_HOLD_TIME,
		PPM_LOADING_UPPER,
		0,
		0
		),
};
STATE_TRANSFER_DATA_PERF(L_ONLY);

static struct ppm_state_transfer state_pwr_transfer_4LL_L[] = {
	TRANS_DATA(
		LL_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_4LL_L_to_LL_ONLY,
		PPM_DEFAULT_DELTA,
		PPM_DEFAULT_HOLD_TIME,
		PPM_LOADING_UPPER,
		0,
		0
		),
};
STATE_TRANSFER_DATA_PWR(4LL_L);

static struct ppm_state_transfer state_perf_transfer_4LL_L[] = {
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
};
STATE_TRANSFER_DATA_PERF(4LL_L);

static struct ppm_state_transfer state_pwr_transfer_4L_LL[] = {
	TRANS_DATA(
		L_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_4L_LL_to_L_ONLY,
		PPM_DEFAULT_DELTA,
		PPM_DEFAULT_HOLD_TIME,
		PPM_LOADING_UPPER,
		0,
		0
		),
};
STATE_TRANSFER_DATA_PWR(4L_LL);

static struct ppm_state_transfer state_perf_transfer_4L_LL[] = {
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
};
STATE_TRANSFER_DATA_PERF(4L_LL);

/*==============================================================*/
/* Power Table auto-generated by script begin			*/
/* Note: DO NOT modify it manually!!				*/
/*==============================================================*/
#include "mt_ppm_power_table.h"
/*==============================================================*/
/* Power Table auto-generated by script	end			*/
/*==============================================================*/

/* default use FY table, may change to SB if needed */
const struct ppm_power_tbl_data power_table_FY = {
	.power_tbl = cpu_tlp_power_tbl_FY,
	.nr_power_tbl = ARRAY_SIZE(cpu_tlp_power_tbl_FY),
};

const struct ppm_power_tbl_data power_table_SB = {
	.power_tbl = cpu_tlp_power_tbl_SB,
	.nr_power_tbl = ARRAY_SIZE(cpu_tlp_power_tbl_SB),
};

/* PPM power state static data */
struct ppm_power_state_data pwr_state_info_FY[NR_PPM_POWER_STATE] = {
	[0] = {
		.name = __stringify(LL_ONLY),
		.state = PPM_POWER_STATE_LL_ONLY,
		PWR_STATE_TBLS(LL_ONLY, FY)
	},
	[1] = {
		.name = __stringify(L_ONLY),
		.state = PPM_POWER_STATE_L_ONLY,
		PWR_STATE_TBLS(L_ONLY, FY)
	},
	[2] = {
		.name = __stringify(4LL_L),
		.state = PPM_POWER_STATE_4LL_L,
		PWR_STATE_TBLS(4LL_L, FY)
	},
	[3] = {
		.name = __stringify(4L_LL),
		.state = PPM_POWER_STATE_4L_LL,
		PWR_STATE_TBLS(4L_LL, FY)
	},
};

struct ppm_power_state_data pwr_state_info_SB[NR_PPM_POWER_STATE] = {
	[0] = {
		.name = __stringify(LL_ONLY),
		.state = PPM_POWER_STATE_LL_ONLY,
		PWR_STATE_TBLS(LL_ONLY, SB)
	},
	[1] = {
		.name = __stringify(L_ONLY),
		.state = PPM_POWER_STATE_L_ONLY,
		PWR_STATE_TBLS(L_ONLY, SB)
	},
	[2] = {
		.name = __stringify(4LL_L),
		.state = PPM_POWER_STATE_4LL_L,
		PWR_STATE_TBLS(4LL_L, SB)
	},
	[3] = {
		.name = __stringify(4L_LL),
		.state = PPM_POWER_STATE_4L_LL,
		PWR_STATE_TBLS(4L_LL, SB)
	},
};

const unsigned int pwr_idx_search_prio[NR_PPM_POWER_STATE][NR_PPM_POWER_STATE] = {
	[PPM_POWER_STATE_LL_ONLY] = {PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_L_ONLY] = {PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_4LL_L] = {PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_4L_LL] = {PPM_POWER_STATE_L_ONLY, PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,},
};

const unsigned int perf_idx_search_prio[NR_PPM_POWER_STATE][NR_PPM_POWER_STATE] = {
	[PPM_POWER_STATE_LL_ONLY] = {PPM_POWER_STATE_L_ONLY, PPM_POWER_STATE_4LL_L, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_L_ONLY] = {PPM_POWER_STATE_4L_LL, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_4LL_L] = {PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_4L_LL] = {PPM_POWER_STATE_NONE,},
};

/*==============================================================*/
/* Local Function Implementation					*/
/*==============================================================*/
/* transition rules */
static bool ppm_trans_rule_LL_ONLY_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	unsigned int cur_freq_LL;

	/* keep in LL_ONLY state if root cluster is fixed at cluster 0 */
	if (ppm_main_info.fixed_root_cluster == 0)
		return false;

	/* keep in LL ONLY state if LCM is off */
	if (ppm_lcmoff_is_policy_activated())
		return false;

	/* check loading */
	if (data.ppm_cur_loads > (settings->loading_bond - settings->loading_delta)
		&& data.ppm_cur_tlp <= settings->tlp_bond) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	/* check freq */
	cur_freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LITTLE);	/* FIXME */
	ppm_dbg(HICA, "LL cur freq = %d\n", cur_freq_LL);

	if (cur_freq_LL >= get_cluster_max_cpufreq(0)) {
		settings->freq_hold_cnt++;
		if (settings->freq_hold_cnt >= settings->freq_hold_time)
			return true;
	} else
		settings->freq_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_LL_ONLY_to_4LL_L(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* check loading only */
	if (data.ppm_cur_loads > (settings->loading_bond - settings->loading_delta)
		&& data.ppm_cur_tlp > settings->tlp_bond) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_L_ONLY_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* check freq only */
	unsigned int cur_freq_L;

	/* keep in L_ONLY state if root cluster is fixed at cluster 1 */
	if (ppm_main_info.fixed_root_cluster == 1)
		return false;

	cur_freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_BIG); /* FIXME */
	ppm_dbg(HICA, "L cur freq = %d\n", cur_freq_L);

	if (cur_freq_L < get_cluster_max_cpufreq(0)) {
		settings->freq_hold_cnt++;
		if (settings->freq_hold_cnt >= settings->freq_hold_time)
			return true;
	} else
		settings->freq_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_L_ONLY_to_4L_LL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* check loading only */
	if (data.ppm_cur_loads > (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_4LL_L_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* check loading only */
	if (data.ppm_cur_loads <= (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_4L_LL_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* check loading only */
	if (data.ppm_cur_loads <= (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}


/*==============================================================*/
/* Global Function Implementation				*/
/*==============================================================*/
struct ppm_power_state_data *ppm_get_power_state_info(void)
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_FY)
		? pwr_state_info_FY : pwr_state_info_SB;
}

const struct ppm_power_tbl_data ppm_get_power_table(void)
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_FY)
		? power_table_FY : power_table_SB;
}

const char *ppm_get_power_state_name(enum ppm_power_state state)
{
	if (state >= NR_PPM_POWER_STATE)
		return "NONE";

	/* the state name is the same between FY and SB */
	return pwr_state_info_FY[state].name;
}

enum ppm_power_state ppm_change_state_with_fix_root_cluster(enum ppm_power_state cur_state, int cluster)
{
	enum ppm_power_state new_state = cur_state;

	switch (cluster) {
	case 0:
		if (cur_state == PPM_POWER_STATE_L_ONLY)
			new_state = PPM_POWER_STATE_LL_ONLY;
		else if (cur_state == PPM_POWER_STATE_4L_LL)
			new_state = PPM_POWER_STATE_4LL_L;
		break;
	case 1:
		if (cur_state == PPM_POWER_STATE_LL_ONLY)
			new_state = PPM_POWER_STATE_L_ONLY;
		else if (cur_state == PPM_POWER_STATE_4LL_L)
			new_state = PPM_POWER_STATE_4L_LL;
		break;
	default:
		break;
	}

	return new_state;
}

int ppm_get_table_idx_by_perf(enum ppm_power_state state, unsigned int perf_idx)
{
	int i;
	struct ppm_power_state_data *state_info;
	const struct ppm_state_sorted_pwr_tbl_data *tbl;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	if (state > NR_PPM_POWER_STATE || (perf_idx == -1)) {
		ppm_warn("Invalid argument: state = %d, pwr_idx = %d\n", state, perf_idx);
		return -1;
	}

	/* search whole tlp table */
	if (state == PPM_POWER_STATE_NONE) {
		for (i = 1; i < power_table.nr_power_tbl; i++) {
			if (power_table.power_tbl[i].perf_idx < perf_idx)
				return power_table.power_tbl[i-1].index;
		}
	} else {
		state_info = ppm_get_power_state_info();
		tbl = state_info[state].perf_sorted_tbl;

		/* return -1 (not found) if input is larger than max perf_idx in table */
		if (tbl->sorted_tbl[0].value < perf_idx)
			return -1;

		for (i = 1; i < tbl->size; i++) {
			if (tbl->sorted_tbl[i].value < perf_idx)
				return tbl->sorted_tbl[i-1].index;
		}
	}

	/* not found */
	return -1;
}

int ppm_get_table_idx_by_pwr(enum ppm_power_state state, unsigned int pwr_idx)
{
	int i;
	struct ppm_power_state_data *state_info;
	const struct ppm_state_sorted_pwr_tbl_data *tbl;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	if (state > NR_PPM_POWER_STATE || (pwr_idx == ~0)) {
		ppm_warn("Invalid argument: state = %d, pwr_idx = %d\n", state, pwr_idx);
		return -1;
	}

	/* search whole tlp table */
	if (state == PPM_POWER_STATE_NONE) {
		for_each_pwr_tbl_entry(i, power_table) {
			if (power_table.power_tbl[i].power_idx <= pwr_idx)
				return i;
		}
	} else {
		state_info = ppm_get_power_state_info();
		tbl = state_info[state].pwr_sorted_tbl;

		for (i = 0; i < tbl->size; i++) {
			if (tbl->sorted_tbl[i].value <= pwr_idx)
				return tbl->sorted_tbl[i].advise_index;
		}
	}

	/* not found */
	return -1;
}

enum ppm_power_state ppm_find_next_state(enum ppm_power_state state,
			unsigned int *level, enum power_state_search_policy policy)
{
	const unsigned int *tbl;
	enum ppm_power_state new_state;
	int keep_going;

	ppm_ver("@%s: state = %s, lv = %d\n", __func__, ppm_get_power_state_name(state), *level);

	if (state >= NR_PPM_POWER_STATE || *level >= NR_PPM_POWER_STATE)
		return PPM_POWER_STATE_NONE;

	tbl = (policy == PERFORMANCE) ? perf_idx_search_prio[state] : pwr_idx_search_prio[state];

	do {
		keep_going = 0;

		new_state = tbl[*level];

		ppm_ver("@%s: new_state = %s, lv = %d\n", __func__, ppm_get_power_state_name(new_state), *level);

		if (new_state == PPM_POWER_STATE_NONE)
			break;

		/* check fix root cluster setting */
		switch (ppm_main_info.fixed_root_cluster) {
		case 0:
			if (new_state == PPM_POWER_STATE_L_ONLY || new_state == PPM_POWER_STATE_4L_LL) {
				(*level)++;
				keep_going = 1;
			}
			break;
		case 1:
			if (new_state == PPM_POWER_STATE_LL_ONLY || new_state == PPM_POWER_STATE_4LL_L) {
				(*level)++;
				keep_going = 1;
			}
			break;
		default:
			break;
		}
	} while (keep_going);

	return new_state;
}

int ppm_find_pwr_idx(struct ppm_cluster_status *cluster_status)
{
	int i;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	int core_1 = cluster_status[0].core_num;
	int opp_1 = cluster_status[0].freq_idx;
	int core_2 = cluster_status[1].core_num;
	int opp_2 = cluster_status[1].freq_idx;

	ppm_dbg(DLPT, "@%s: core_1 = %d, opp_1 = %d, core_2 = %d, opp_2 = %d, volt_1 = %d, volt_2 = %d\n",
		__func__, core_1, opp_1, core_2, opp_2, cluster_status[0].volt, cluster_status[1].volt);

	opp_1 = (!core_1) ? -1 : opp_1;
	opp_2 = (!core_2) ? -1 : opp_2;

	/* sync opp to little one due to shared bulk*/
	if (opp_1 != -1 && opp_2 != -1) {
		opp_1 = MIN(opp_1, opp_2);
		opp_2 = MIN(opp_1, opp_2);
	}

	for_each_pwr_tbl_entry(i, power_table) {
		if (power_table.power_tbl[i].cluster_cfg[0].core_num == core_1
				&& power_table.power_tbl[i].cluster_cfg[0].opp_lv == opp_1
				&& power_table.power_tbl[i].cluster_cfg[1].core_num == core_2
				&& power_table.power_tbl[i].cluster_cfg[1].opp_lv == opp_2
			) {
				ppm_dbg(DLPT, "[index][power] = [%d][%d]\n",
					i, power_table.power_tbl[i].power_idx);
				return power_table.power_tbl[i].power_idx;
			}
	}

	ppm_dbg(DLPT, "@%s: power_idx not found!\n", __func__);

	/* return -1 if not found */
	return -1;
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
	int sum;

	ppm_ver("Judge: input --> [%s] (%d)(%d)(%d)(%d) [(%d)(%d)]\n",
		ppm_get_power_state_name(cur_state),
		LL_core_min, LL_core_max, L_core_min, L_core_max, LL_freq_min, L_freq_max);

	LL_core_max = (LL_core_max == -1) ? 4 : LL_core_max;
	L_core_max = (L_core_max == -1) ? 4 : L_core_max;
	LL_core_min = (LL_core_min == -1) ? 0 : LL_core_min;
	L_core_min = (L_core_min == -1) ? 0 : L_core_min;
	sum = LL_core_min + L_core_min;

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
			: (LL_core_min > 0 && L_core_min == -1 &&
				L_freq_max >= LL_freq_min && L_core_max >= LL_core_min) ? cur_state
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

unsigned int ppm_get_root_cluster_by_state(enum ppm_power_state cur_state)
{
	unsigned int root_cluster = 0;

	switch (cur_state) {
	case PPM_POWER_STATE_L_ONLY:
	case PPM_POWER_STATE_4L_LL:
		root_cluster = 1;
		break;
	case PPM_POWER_STATE_NONE:
		root_cluster = (ppm_main_info.fixed_root_cluster == -1) ? 0
				: (unsigned int)ppm_main_info.fixed_root_cluster;
		break;
	case PPM_POWER_STATE_LL_ONLY:
	case PPM_POWER_STATE_4LL_L:
	default:
		break;
	}

	return root_cluster;
}

#ifdef PPM_IC_SEGMENT_CHECK
enum ppm_power_state ppm_check_fix_state_by_segment(void)
{
	unsigned int segment = get_devinfo_with_index(21) & 0xFF;
	enum ppm_power_state fix_state = PPM_POWER_STATE_NONE;

	switch (segment) {
	case 0x43: /* fix L only */
	case 0x4B:
		fix_state = PPM_POWER_STATE_L_ONLY;
		break;
	default:
		break;
	}

	ppm_info("segment = 0x%x, fix_state = %s\n", segment, ppm_get_power_state_name(fix_state));

	return fix_state;
}
#endif


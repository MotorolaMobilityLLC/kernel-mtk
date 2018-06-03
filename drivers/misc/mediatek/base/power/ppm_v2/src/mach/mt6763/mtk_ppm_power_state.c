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
#include <trace/events/mtk_events.h>

#include "mtk_ppm_platform.h"
#include "mtk_ppm_internal.h"


/*==============================================================*/
/* Macros							*/
/*==============================================================*/
#define PWR_STATE_INFO(name, type) \
	.min_pwr_idx = 0,			\
	.max_perf_idx = ~0,			\
	.cluster_limit = &cluster_limit_##name,	\
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

#define TRANS_DATA(state, mask, rule, c_hold, c_bond, h_hold, h_l_bond, h_h_bond, f_hold) {	\
	.next_state = PPM_POWER_STATE_##state,	\
	.mode_mask = mask,	\
	.transition_rule = rule,	\
	.capacity_hold_time = c_hold,	\
	.capacity_hold_cnt = 0,		\
	.capacity_bond = c_bond,	\
	.bigtsk_hold_time = h_hold,	\
	.bigtsk_hold_cnt = 0,	\
	.bigtsk_l_bond = h_l_bond,	\
	.bigtsk_h_bond = h_h_bond,	\
	.freq_hold_time = f_hold,	\
	.freq_hold_cnt = 0,		\
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
static bool ppm_trans_rule_LL_ONLY_to_ALL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_L_ONLY_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_L_ONLY_to_ALL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_ALL_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);
static bool ppm_trans_rule_ALL_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings);

/*==============================================================*/
/* Local Variables						*/
/*==============================================================*/
/* cluster limit for each power state */
static const struct ppm_cluster_limit state_limit_LL_ONLY[] = {
	[0] = LIMIT(15, 0, 1, 4),
	[1] = LIMIT(15, 0, 0, 0),
};
STATE_LIMIT(LL_ONLY);

static const struct ppm_cluster_limit state_limit_L_ONLY[] = {
	[0] = LIMIT(15, 0, 0, 0),
	[1] = LIMIT(8, 0, 1, 4),
};
STATE_LIMIT(L_ONLY);

static const struct ppm_cluster_limit state_limit_ALL[] = {
	[0] = LIMIT(15, 0, 0, 4),
	[1] = LIMIT(15, 0, 0, 4),
};
STATE_LIMIT(ALL);

/* state transfer data  by power/performance for each state */
static struct ppm_state_transfer state_pwr_transfer_LL_ONLY[] = {
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0, 0),
};
STATE_TRANSFER_DATA_PWR(LL_ONLY);

static struct ppm_state_transfer state_perf_transfer_LL_ONLY[] = {
	TRANS_DATA(
		ALL,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_LL_ONLY_to_ALL,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_UP,
		PPM_DEFAULT_BIGTSK_TIME,
		2,
		4,
		0
		),
	TRANS_DATA(
		L_ONLY,
#ifdef PPM_DISABLE_CLUSTER_MIGRATION
		0,
#else
		PPM_MODE_MASK_JUST_MAKE_ONLY | PPM_MODE_MASK_PERFORMANCE_ONLY,
#endif
		ppm_trans_rule_LL_ONLY_to_L_ONLY,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_UP,
		PPM_DEFAULT_BIGTSK_TIME,
		2,
		4,
		0
		),
};
STATE_TRANSFER_DATA_PERF(LL_ONLY);

static struct ppm_state_transfer state_pwr_transfer_L_ONLY[] = {
	TRANS_DATA(
		LL_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_L_ONLY_to_LL_ONLY,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_DOWN,
		PPM_DEFAULT_BIGTSK_TIME,
		0,
		0,
		PPM_DEFAULT_FREQ_HOLD_TIME
		),
};
STATE_TRANSFER_DATA_PWR(L_ONLY);

static struct ppm_state_transfer state_perf_transfer_L_ONLY[] = {
	TRANS_DATA(
		ALL,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_L_ONLY_to_ALL,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_UP,
		PPM_DEFAULT_BIGTSK_TIME,
		2,
		4,
		0
		),
};
STATE_TRANSFER_DATA_PERF(L_ONLY);

static struct ppm_state_transfer state_pwr_transfer_ALL[] = {
	TRANS_DATA(
		LL_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_ALL_to_LL_ONLY,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_DOWN,
		PPM_DEFAULT_BIGTSK_TIME,
		0,
		0,
		0
		),
	TRANS_DATA(
		L_ONLY,
		PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_ALL_to_L_ONLY,
		PPM_DEFAULT_HOLD_TIME,
		PPM_CAPACITY_DOWN,
		PPM_DEFAULT_BIGTSK_TIME,
		2,
		4,
		0
		),
};
STATE_TRANSFER_DATA_PWR(ALL);

static struct ppm_state_transfer state_perf_transfer_ALL[] = {
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0, 0),
};
STATE_TRANSFER_DATA_PERF(ALL);

/* PPM power state static data */
struct ppm_power_state_data pwr_state_info_FY[NR_PPM_POWER_STATE] = {
	[0] = {
		.name = __stringify(LL_ONLY),
		.state = PPM_POWER_STATE_LL_ONLY,
		PWR_STATE_INFO(LL_ONLY, FY)
	},
	[1] = {
		.name = __stringify(L_ONLY),
		.state = PPM_POWER_STATE_L_ONLY,
		PWR_STATE_INFO(L_ONLY, FY)
	},
	[2] = {
		.name = __stringify(ALL),
		.state = PPM_POWER_STATE_ALL,
		PWR_STATE_INFO(ALL, FY)
	},
};

struct ppm_power_state_data pwr_state_info_SB[NR_PPM_POWER_STATE] = {
	[0] = {
		.name = __stringify(LL_ONLY),
		.state = PPM_POWER_STATE_LL_ONLY,
		PWR_STATE_INFO(LL_ONLY, SB)
	},
	[1] = {
		.name = __stringify(L_ONLY),
		.state = PPM_POWER_STATE_L_ONLY,
		PWR_STATE_INFO(L_ONLY, SB)
	},
	[2] = {
		.name = __stringify(ALL),
		.state = PPM_POWER_STATE_ALL,
		PWR_STATE_INFO(ALL, SB)
	},
};

const unsigned int pwr_idx_search_prio[NR_PPM_POWER_STATE][NR_PPM_POWER_STATE] = {
	[PPM_POWER_STATE_LL_ONLY] = {PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_L_ONLY] = {PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_ALL] = {PPM_POWER_STATE_NONE,},
};

const unsigned int perf_idx_search_prio[NR_PPM_POWER_STATE][NR_PPM_POWER_STATE] = {
	[PPM_POWER_STATE_LL_ONLY] = {PPM_POWER_STATE_L_ONLY, PPM_POWER_STATE_ALL, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_L_ONLY] = {PPM_POWER_STATE_ALL, PPM_POWER_STATE_NONE,},
	[PPM_POWER_STATE_ALL] = {PPM_POWER_STATE_NONE,},
};

/*==============================================================*/
/* Local Function Implementation				*/
/*==============================================================*/
/* transition rules */
static bool ppm_trans_rule_LL_ONLY_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	return false;
}

static bool ppm_trans_rule_LL_ONLY_to_ALL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	{
		unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_LL);

		if (heavy_task) {
			ppm_dbg(HICA, "Go to ALL due to LL heavy task = %d\n", heavy_task);
			trace_ppm_hica(
				ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
				ppm_get_power_state_name(PPM_POWER_STATE_ALL),
				-1, -1, -1, -1, heavy_task, -1, true);
			return true;
		}
	}
#endif

#if PPM_BIG_TASK_INDICATE_SUPPORT
	{
		unsigned int big_task_L = hps_get_bigtsk(PPM_CLUSTER_L);

		if (big_task_L) {
			settings->bigtsk_hold_cnt++;
			if (settings->bigtsk_hold_cnt >= settings->bigtsk_hold_time) {
				ppm_dbg(HICA, "Go to ALL due to L big task = %d/%d\n", big_task_L);
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					-1, -1, big_task_L, -1, -1, -1, true);
				return true;
			}
		} else
			settings->bigtsk_hold_cnt = 0;
	}
#endif

	{
		/* check capacity */
		unsigned long usage, capacity;

		if (sched_get_cluster_util(PPM_CLUSTER_LL, &usage, &capacity)) {
			ppm_err("Get cluster %d util failed\n", PPM_CLUSTER_LL);
			return false;
		}

		ppm_dbg(HICA, "Cluster %d usage = %ld, capacity = %ld\n",
			PPM_CLUSTER_LL, usage, capacity);

		if (usage >= capacity * settings->capacity_bond / 100) {
			settings->capacity_hold_cnt++;
			if (settings->capacity_hold_cnt >= settings->capacity_hold_time) {
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					usage, capacity, -1, -1, -1, -1, true);
				return true;
			}
		} else
			settings->capacity_hold_cnt = 0;

		trace_ppm_hica(
			ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
			ppm_get_power_state_name(PPM_POWER_STATE_ALL),
			usage, capacity, -1, -1, -1, -1, false);
	}

	return false;
}

static bool ppm_trans_rule_L_ONLY_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* keep in L_ONLY state if root cluster is fixed at L */
	if (ppm_main_info.fixed_root_cluster == PPM_CLUSTER_L)
		return false;

#if PPM_BIG_TASK_INDICATE_SUPPORT
	{
		unsigned int big_task_L = hps_get_bigtsk(PPM_CLUSTER_L);

		if (big_task_L) {
			ppm_dbg(HICA, "Stay in L_ONLY due to L big task = %d\n", big_task_L);
			trace_ppm_hica(
				ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
				ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
				-1, -1, big_task_L, -1, -1, -1, false);
			settings->capacity_hold_cnt = 0;
			settings->freq_hold_cnt = 0;
			return false;
		}
	}
#endif

	{
		/* check freq */
		unsigned int cur_freq_L = mt_cpufreq_get_cur_phy_freq_no_lock(MT_CPU_DVFS_L);

		ppm_dbg(HICA, "L cur freq = %d\n", cur_freq_L);

		if (cur_freq_L < get_cluster_max_cpufreq(PPM_CLUSTER_LL)) {
			settings->freq_hold_cnt++;
			if (settings->freq_hold_cnt >= settings->freq_hold_time) {
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					-1, -1, -1, -1, -1, cur_freq_L, true);
				return true;
			}
		} else
			settings->freq_hold_cnt = 0;
	}

	{
		/* check capacity */
		unsigned long usage_LL, usage_L, capacity_LL, capacity_L;

		if (sched_get_cluster_util(PPM_CLUSTER_LL, &usage_LL, &capacity_LL)) {
			ppm_err("Get cluster %d util failed\n", PPM_CLUSTER_LL);
			return false;
		}

		if (sched_get_cluster_util(PPM_CLUSTER_L, &usage_L, &capacity_L)) {
			ppm_err("Get cluster %d util failed\n", PPM_CLUSTER_L);
			return false;
		}

		ppm_dbg(HICA, "L usage = %ld, LL capacity = %ld\n", usage_L, capacity_LL);

		if (usage_L < capacity_LL * settings->capacity_bond / 100) {
			settings->capacity_hold_cnt++;
			if (settings->capacity_hold_cnt >= settings->capacity_hold_time) {
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					usage_L, capacity_LL, -1, -1, -1, -1, true);
				return true;
			}
		} else
			settings->capacity_hold_cnt = 0;

		trace_ppm_hica(
			ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
			ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
			usage_L, capacity_LL, -1, -1, -1, -1, false);
	}

	return false;
}

static bool ppm_trans_rule_L_ONLY_to_ALL(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	{
		unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_L);

		if (heavy_task) {
			ppm_dbg(HICA, "Go to ALL due to L heavy task = %d\n", heavy_task);
			trace_ppm_hica(
				ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
				ppm_get_power_state_name(PPM_POWER_STATE_ALL),
				-1, -1, -1, -1, heavy_task, -1, true);
			return true;
		}
	}
#endif

#if PPM_BIG_TASK_INDICATE_SUPPORT
	{
		unsigned int big_task_L = hps_get_bigtsk(PPM_CLUSTER_L);

		if (big_task_L > settings->bigtsk_h_bond) {
			settings->bigtsk_hold_cnt++;
			if (settings->bigtsk_hold_cnt >= settings->bigtsk_hold_time) {
				ppm_dbg(HICA, "Go to ALL due to L big task = %d\n", big_task_L);
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					-1, -1, big_task_L, -1, -1, -1, true);
				return true;
			}
		} else
			settings->bigtsk_hold_cnt = 0;
	}
#endif

	{
		/* check capacity */
		unsigned long usage, capacity;

		if (sched_get_cluster_util(PPM_CLUSTER_L, &usage, &capacity)) {
			ppm_err("Get cluster %d util failed\n", PPM_CLUSTER_L);
			return false;
		}

		ppm_dbg(HICA, "Cluster %d usage = %ld, capacity = %ld\n",
			PPM_CLUSTER_L, usage, capacity);

		if (usage >= capacity * settings->capacity_bond / 100) {
			settings->capacity_hold_cnt++;
			if (settings->capacity_hold_cnt >= settings->capacity_hold_time) {
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					usage, capacity, -1, -1, -1, -1, true);
				return true;
			}
		} else
			settings->capacity_hold_cnt = 0;

		trace_ppm_hica(
			ppm_get_power_state_name(PPM_POWER_STATE_L_ONLY),
			ppm_get_power_state_name(PPM_POWER_STATE_ALL),
			usage, capacity, -1, -1, -1, -1, false);
	}

	return false;
}

static bool ppm_trans_rule_ALL_to_LL_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	/* keep in ALL state if root cluster is fixed at L */
	if (ppm_main_info.fixed_root_cluster == PPM_CLUSTER_L)
		return false;

#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	{
		unsigned int heavy_task, i;

		for_each_ppm_clusters(i) {
			heavy_task = hps_get_hvytsk(i);
			if (heavy_task) {
				ppm_dbg(HICA, "Stay in ALL due to cluster%d heavy task = %d\n",
					i, heavy_task);
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					-1, -1, -1, -1, heavy_task, -1, false);
				settings->capacity_hold_cnt = 0;
				return false;
			}
		}
	}
#endif

#if PPM_BIG_TASK_INDICATE_SUPPORT
	{
		unsigned int big_task_L = hps_get_bigtsk(PPM_CLUSTER_L);

		if (big_task_L) {
			ppm_dbg(HICA, "Stay in ALL due to L big task = %d\n", big_task_L);
			trace_ppm_hica(
				ppm_get_power_state_name(PPM_POWER_STATE_ALL),
				ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
				-1, -1, big_task_L, -1, -1, -1, false);
			settings->capacity_hold_cnt = 0;
			return false;
		}
	}
#endif

	{
		/* check capacity */
		unsigned long usage, usage_total = 0, capacity = 0, dummy;
		unsigned int i;

		for_each_ppm_clusters(i) {
			if (sched_get_cluster_util(i, &usage, &dummy)) {
				ppm_err("Get cluster %d util failed\n", i);
				return false;
			}
			usage_total += usage;
			if (i == PPM_CLUSTER_LL)
				capacity = dummy;
		}
		ppm_dbg(HICA, "usage_total = %ld, LL capacity = %ld\n", usage_total, capacity);

		if (usage_total < capacity * settings->capacity_bond / 100) {
			settings->capacity_hold_cnt++;
			if (settings->capacity_hold_cnt >= settings->capacity_hold_time) {
				trace_ppm_hica(
					ppm_get_power_state_name(PPM_POWER_STATE_ALL),
					ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
					usage_total, capacity, -1, -1, -1, -1, true);
				return true;
			}
		} else
			settings->capacity_hold_cnt = 0;

		trace_ppm_hica(
			ppm_get_power_state_name(PPM_POWER_STATE_ALL),
			ppm_get_power_state_name(PPM_POWER_STATE_LL_ONLY),
			usage_total, capacity, -1, -1, -1, -1, false);
	}

	return false;
}

static bool ppm_trans_rule_ALL_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	return false;
}


/*==============================================================*/
/* Global Function Implementation				*/
/*==============================================================*/
struct ppm_power_state_data *ppm_get_power_state_info(void)
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? pwr_state_info_SB
		: pwr_state_info_FY;
}

const char *ppm_get_power_state_name(enum ppm_power_state state)
{
	if (state >= NR_PPM_POWER_STATE)
		return "NONE";

	/* the state name is the same for each segment */
	return pwr_state_info_FY[state].name;
}

enum ppm_power_state ppm_change_state_with_fix_root_cluster(enum ppm_power_state cur_state, int cluster)
{
	enum ppm_power_state new_state = cur_state;

	switch (cluster) {
	case PPM_CLUSTER_LL:
		if (cur_state == PPM_POWER_STATE_L_ONLY)
			new_state = PPM_POWER_STATE_LL_ONLY;
		break;
	case PPM_CLUSTER_L:
		if (cur_state == PPM_POWER_STATE_LL_ONLY)
			new_state = PPM_POWER_STATE_L_ONLY;
		break;
	default:
		break;
	}

	return new_state;
}

unsigned int ppm_get_root_cluster_by_state(enum ppm_power_state cur_state)
{
	unsigned int root_cluster = PPM_CLUSTER_LL;

	switch (cur_state) {
	case PPM_POWER_STATE_L_ONLY:
		root_cluster = PPM_CLUSTER_L;
		break;
	case PPM_POWER_STATE_NONE:
	case PPM_POWER_STATE_ALL:
		root_cluster = (ppm_main_info.fixed_root_cluster == -1) ? PPM_CLUSTER_LL
				: (unsigned int)ppm_main_info.fixed_root_cluster;
		break;
	case PPM_POWER_STATE_LL_ONLY:
	default:
		break;
	}

	return root_cluster;
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
		case PPM_CLUSTER_LL:
			if (new_state == PPM_POWER_STATE_L_ONLY) {
				(*level)++;
				keep_going = 1;
			}
			break;
		case PPM_CLUSTER_L:
			if (new_state == PPM_POWER_STATE_LL_ONLY) {
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


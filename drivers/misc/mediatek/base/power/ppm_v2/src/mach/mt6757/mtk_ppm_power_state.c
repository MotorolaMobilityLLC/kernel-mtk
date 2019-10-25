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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <trace/events/mtk_events.h>

#include "mtk_cpufreq.h"
#include "mtk_ppm_internal.h"
#include "mtk_ppm_platform.h"

#if PPM_HICA_VARIANT_SUPPORT
int __attribute__((weak))
sched_get_nr_overutil_avg(int cluster_id, int *l_avg, int *h_avg)
{
	return 0;
}
#endif

/*==============================================================*/
/* Macros							*/
/*==============================================================*/
#define PWR_STATE_INFO(name, type)                                             \
	.min_pwr_idx = 0, .max_perf_idx = ~0,                                  \
	.cluster_limit = &cluster_limit_##name,                                \
	.transfer_by_pwr = &transfer_by_pwr_##name,                            \
	.transfer_by_perf = &transfer_by_perf_##name,

#define LIMIT(fmin, fmax, cmin, cmax)                                          \
	{                                                                      \
		.min_cpufreq_idx = fmin, .max_cpufreq_idx = fmax,              \
		.min_cpu_core = cmin, .max_cpu_core = cmax,                    \
	}

#define STATE_LIMIT(name)                                                      \
	const struct ppm_state_cluster_limit_data cluster_limit_##name = {     \
	    .state_limit = state_limit_##name,                                 \
	    .size = ARRAY_SIZE(state_limit_##name),                            \
	}

#if PPM_HICA_VARIANT_SUPPORT
#define TRANS_DATA(state, mask, rule, delta, hold, bond, f_hold, tlp, oul,     \
		   ouh)                                                        \
	{                                                                      \
		.next_state = PPM_POWER_STATE_##state, .mode_mask = mask,      \
		.transition_rule = rule, .loading_delta = delta,               \
		.loading_hold_time = hold, .loading_hold_cnt = 0,              \
		.loading_bond = bond, .freq_hold_time = f_hold,                \
		.freq_hold_cnt = 0, .tlp_bond = tlp,                           \
		.overutil_l_hold_time = oul, .overutil_l_hold_cnt = 0,         \
		.overutil_h_hold_time = ouh, .overutil_h_hold_cnt = 0,         \
	}
#else
#define TRANS_DATA(state, mask, rule, delta, hold, bond, f_hold, tlp)          \
	{                                                                      \
		.next_state = PPM_POWER_STATE_##state, .mode_mask = mask,      \
		.transition_rule = rule, .loading_delta = delta,               \
		.loading_hold_time = hold, .loading_hold_cnt = 0,              \
		.loading_bond = bond, .freq_hold_time = f_hold,                \
		.freq_hold_cnt = 0, .tlp_bond = tlp,                           \
	}
#endif

#define STATE_TRANSFER_DATA_PWR(name)                                          \
	struct ppm_state_transfer_data transfer_by_pwr_##name = {              \
	    .transition_data = state_pwr_transfer_##name,                      \
	    .size = ARRAY_SIZE(state_pwr_transfer_##name),                     \
	}

#define STATE_TRANSFER_DATA_PERF(name)                                         \
	struct ppm_state_transfer_data transfer_by_perf_##name = {             \
	    .transition_data = state_perf_transfer_##name,                     \
	    .size = ARRAY_SIZE(state_perf_transfer_##name),                    \
	}

/*==============================================================*/
/* Local Functions						*/
/*==============================================================*/
static bool
ppm_trans_rule_LL_ONLY_to_L_ONLY(struct ppm_hica_algo_data data,
				 struct ppm_state_transfer *settings);
static bool
ppm_trans_rule_LL_ONLY_to_4LL_L(struct ppm_hica_algo_data data,
				struct ppm_state_transfer *settings);
static bool
ppm_trans_rule_L_ONLY_to_LL_ONLY(struct ppm_hica_algo_data data,
				 struct ppm_state_transfer *settings);
static bool ppm_trans_rule_L_ONLY_to_4L_LL(struct ppm_hica_algo_data data,
					   struct ppm_state_transfer *settings);
static bool
ppm_trans_rule_4LL_L_to_LL_ONLY(struct ppm_hica_algo_data data,
				struct ppm_state_transfer *settings);
static bool ppm_trans_rule_4L_LL_to_L_ONLY(struct ppm_hica_algo_data data,
					   struct ppm_state_transfer *settings);

/*==============================================================*/
/* Local Variables						*/
/*==============================================================*/
/* cluster limit for each power state */
static const struct ppm_cluster_limit state_limit_LL_ONLY[] = {
	[0] = LIMIT(15, 0, 1, 4), [1] = LIMIT(15, 0, 0, 0),
};
STATE_LIMIT(LL_ONLY);

static const struct ppm_cluster_limit state_limit_L_ONLY[] = {
	[0] = LIMIT(15, 0, 0, 0), [1] = LIMIT(9, 0, 1, 4),
};
STATE_LIMIT(L_ONLY);

static const struct ppm_cluster_limit state_limit_4LL_L[] = {
	[0] = LIMIT(15, 0, 4, 4), [1] = LIMIT(15, 0, 1, 4),
};
STATE_LIMIT(4LL_L);

static const struct ppm_cluster_limit state_limit_4L_LL[] = {
	[0] = LIMIT(15, 0, 1, 4), [1] = LIMIT(15, 0, 4, 4),
};
STATE_LIMIT(4L_LL);

/* state transfer data  by power/performance for each state */
static struct ppm_state_transfer state_pwr_transfer_LL_ONLY[] = {
#if PPM_HICA_VARIANT_SUPPORT
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0, 0, 0),
#else
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
#endif
};
STATE_TRANSFER_DATA_PWR(LL_ONLY);

static struct ppm_state_transfer state_perf_transfer_LL_ONLY[] = {
	TRANS_DATA(4LL_L, PPM_MODE_MASK_ALL_MODE,
		 ppm_trans_rule_LL_ONLY_to_4LL_L,
	       PPM_DEFAULT_DELTA, PPM_DEFAULT_HOLD_TIME, PPM_LOADING_UPPER, 0,
#if PPM_HICA_VARIANT_SUPPORT
	       PPM_TLP_CRITERIA, 0, 0
#else
	       PPM_TLP_CRITERIA
#endif
	       ),
	TRANS_DATA(L_ONLY,
	       PPM_MODE_MASK_JUST_MAKE_ONLY | PPM_MODE_MASK_PERFORMANCE_ONLY,
	       ppm_trans_rule_LL_ONLY_to_L_ONLY, PPM_DEFAULT_DELTA,
	       PPM_DEFAULT_HOLD_TIME, PPM_LOADING_UPPER,
	       /* PPM_DEFAULT_FREQ_HOLD_TIME, */
	       8,
#if PPM_HICA_VARIANT_SUPPORT
	       PPM_TLP_CRITERIA, 0, 8
#else
	       PPM_TLP_CRITERIA
#endif
	       ),
};
STATE_TRANSFER_DATA_PERF(LL_ONLY);

static struct ppm_state_transfer state_pwr_transfer_L_ONLY[] = {
	TRANS_DATA(LL_ONLY, PPM_MODE_MASK_ALL_MODE,
	       ppm_trans_rule_L_ONLY_to_LL_ONLY, 0, 0, 0,
	       PPM_DEFAULT_FREQ_HOLD_TIME,
#if PPM_HICA_VARIANT_SUPPORT
	       0, 4, 0
#else
	       0
#endif
	       ),
};
STATE_TRANSFER_DATA_PWR(L_ONLY);

static struct ppm_state_transfer state_perf_transfer_L_ONLY[] = {
	TRANS_DATA(4L_LL, PPM_MODE_MASK_ALL_MODE,
			 ppm_trans_rule_L_ONLY_to_4L_LL,
	       PPM_DEFAULT_DELTA, PPM_DEFAULT_HOLD_TIME, PPM_LOADING_UPPER, 0,
#if PPM_HICA_VARIANT_SUPPORT
	       0, 0, 0
#else
	       0
#endif
	       ),
};
STATE_TRANSFER_DATA_PERF(L_ONLY);

static struct ppm_state_transfer state_pwr_transfer_4LL_L[] = {
	TRANS_DATA(LL_ONLY, PPM_MODE_MASK_ALL_MODE,
			 ppm_trans_rule_4LL_L_to_LL_ONLY,
	       PPM_DEFAULT_DELTA, PPM_DEFAULT_HOLD_TIME, PPM_LOADING_UPPER, 0,
#if PPM_HICA_VARIANT_SUPPORT
	       0, 0, 0
#else
	       0
#endif
	       ),
};
STATE_TRANSFER_DATA_PWR(4LL_L);

static struct ppm_state_transfer state_perf_transfer_4LL_L[] = {
#if PPM_HICA_VARIANT_SUPPORT
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0, 0, 0),
#else
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
#endif
};
STATE_TRANSFER_DATA_PERF(4LL_L);

static struct ppm_state_transfer state_pwr_transfer_4L_LL[] = {
	TRANS_DATA(L_ONLY, PPM_MODE_MASK_ALL_MODE,
		ppm_trans_rule_4L_LL_to_L_ONLY,
	       PPM_DEFAULT_DELTA, PPM_DEFAULT_HOLD_TIME, PPM_LOADING_UPPER, 0,
#if PPM_HICA_VARIANT_SUPPORT
	       0, 0, 0
#else
	       0
#endif
	       ),
};
STATE_TRANSFER_DATA_PWR(4L_LL);

static struct ppm_state_transfer state_perf_transfer_4L_LL[] = {
#if PPM_HICA_VARIANT_SUPPORT
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0, 0, 0),
#else
	TRANS_DATA(NONE, 0, NULL, 0, 0, 0, 0, 0),
#endif
};
STATE_TRANSFER_DATA_PERF(4L_LL);

/* PPM power state static data */
struct ppm_power_state_data pwr_state_info_FY[NR_PPM_POWER_STATE] = {
	[0] = {.name = __stringify(LL_ONLY),
	       .state = PPM_POWER_STATE_LL_ONLY,
	       PWR_STATE_INFO(LL_ONLY, FY)},
	[1] = {.name = __stringify(L_ONLY),
	       .state = PPM_POWER_STATE_L_ONLY,
	       PWR_STATE_INFO(L_ONLY, FY)},
	[2] = {.name = __stringify(4LL_L),
	       .state = PPM_POWER_STATE_4LL_L,
	       PWR_STATE_INFO(4LL_L, FY)},
	[3] = {.name = __stringify(4L_LL),
	       .state = PPM_POWER_STATE_4L_LL,
	       PWR_STATE_INFO(4L_LL, FY)},
};

struct ppm_power_state_data pwr_state_info_SB[NR_PPM_POWER_STATE] = {
	[0] = {.name = __stringify(LL_ONLY),
	       .state = PPM_POWER_STATE_LL_ONLY,
	       PWR_STATE_INFO(LL_ONLY, SB)},
	[1] = {.name = __stringify(L_ONLY),
	       .state = PPM_POWER_STATE_L_ONLY,
	       PWR_STATE_INFO(L_ONLY, SB)},
	[2] = {.name = __stringify(4LL_L),
	       .state = PPM_POWER_STATE_4LL_L,
	       PWR_STATE_INFO(4LL_L, SB)},
	[3] = {.name = __stringify(4L_LL),
	       .state = PPM_POWER_STATE_4L_LL,
	       PWR_STATE_INFO(4L_LL, SB)},
};

const unsigned int pwr_idx_search_prio[NR_PPM_POWER_STATE]
					[NR_PPM_POWER_STATE] = {
	    [PPM_POWER_STATE_LL_ONLY] = {
		    PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_L_ONLY] = {
		    PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_4LL_L] = {
		    PPM_POWER_STATE_LL_ONLY, PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_4L_LL] = {
		    PPM_POWER_STATE_L_ONLY, PPM_POWER_STATE_LL_ONLY,
		    PPM_POWER_STATE_NONE,
		},
};

const unsigned int
	perf_idx_search_prio[NR_PPM_POWER_STATE][NR_PPM_POWER_STATE] = {
	    [PPM_POWER_STATE_LL_ONLY] = {
		    PPM_POWER_STATE_L_ONLY, PPM_POWER_STATE_4LL_L,
		    PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_L_ONLY] = {
		    PPM_POWER_STATE_4L_LL, PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_4LL_L] = {
		    PPM_POWER_STATE_NONE,
		},
	    [PPM_POWER_STATE_4L_LL] = {
		    PPM_POWER_STATE_NONE,
		},
};

/*==============================================================*/
/* Local Function Implementation					*/
/*==============================================================*/
/* transition rules */
static bool
ppm_trans_rule_LL_ONLY_to_L_ONLY(struct ppm_hica_algo_data data,
				 struct ppm_state_transfer *settings)
{
	unsigned int cur_freq_LL;
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_LL);
#endif
#if PPM_HICA_VARIANT_SUPPORT
	int overutil_l = 0;
	int overutil_h = 0;
#endif

	/* keep in LL_ONLY state if root cluster is fixed at cluster 0 */
	if (ppm_main_info.fixed_root_cluster == 0)
		return false;

	/* keep in LL ONLY state if LCM is off */
	if (ppm_lcmoff_is_policy_activated())
		return false;

/* check heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	if (heavy_task && data.ppm_cur_tlp <= settings->tlp_bond) {
		ppm_dbg(HICA, "LL heavy task = %d\n", heavy_task);
		return true;
	}
#endif

	/* check loading */
	if (data.ppm_cur_loads >
		(settings->loading_bond - settings->loading_delta) &&
	    data.ppm_cur_tlp <= settings->tlp_bond) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

#if PPM_HICA_VARIANT_SUPPORT
	/* HICA 1.75 */
	if (cur_hica_variant == 1) {
		sched_get_nr_overutil_avg(0, &overutil_l, &overutil_h);
		ppm_dbg(HICA, "LL overutil_l = %d, overutil_h = %d\n",
			overutil_l, overutil_h);
#ifdef _TRACE_
		trace_ppm_overutil_update(overutil_l, overutil_h);
#endif

		if (overutil_h > hica_overutil) {
			settings->overutil_h_hold_cnt++;
			if (settings->overutil_h_hold_cnt >=
			    settings->overutil_h_hold_time)
				return true;
		} else {
			settings->overutil_h_hold_cnt = 0;
		}

		return false;
	}
#endif
	/* check freq */
	cur_freq_LL = mt_cpufreq_get_cur_phy_freq_no_lock(MT_CPU_DVFS_LL);
	ppm_dbg(HICA, "LL cur freq = %d\n", cur_freq_LL);

	if (cur_freq_LL >= get_cluster_max_cpufreq(0)) {
		settings->freq_hold_cnt++;
		if (settings->freq_hold_cnt >= settings->freq_hold_time)
			return true;
	} else
		settings->freq_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_LL_ONLY_to_4LL_L(struct ppm_hica_algo_data data,
					    struct ppm_state_transfer *settings)
{
/* check heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_LL);

	if (heavy_task && data.ppm_cur_tlp > settings->tlp_bond) {
		ppm_dbg(HICA, "LL heavy task = %d\n", heavy_task);
		return true;
	}
#endif

#ifdef PPM_TURBO_CORE_SUPPORT
	/* unlock L if smart_detect_boost is enabled */
	if (ppm_main_info.smart_detect_boost && ppm_main_info.is_turbo_core) {
		ppm_dbg(HICA,
			"Go to 4LL_L due to smart detect boost is enabled\n");
		return true;
	}
#endif

	/* check loading only */
	if (data.ppm_cur_loads >
		(settings->loading_bond - settings->loading_delta) &&
	    data.ppm_cur_tlp > settings->tlp_bond) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool
ppm_trans_rule_L_ONLY_to_LL_ONLY(struct ppm_hica_algo_data data,
				 struct ppm_state_transfer *settings)
{
	/* check freq only */
	unsigned int cur_freq_L;
#if PPM_HICA_VARIANT_SUPPORT
	int overutil_l = 0;
	int overutil_h = 0;
#endif

	/* keep in L_ONLY state if root cluster is fixed at cluster 1 */
	if (ppm_main_info.fixed_root_cluster == 1)
		return false;

#if PPM_HICA_VARIANT_SUPPORT
	/* HICA 1.75 */
	if (cur_hica_variant == 1) {
		sched_get_nr_overutil_avg(1, &overutil_l, &overutil_h);
		ppm_dbg(HICA, "L overutil_l = %d, overutil_h = %d\n",
			overutil_l, overutil_h);
#ifdef _TRACE_
		trace_ppm_overutil_update(overutil_l, overutil_h);
#endif

		if (overutil_l <= hica_overutil) {
			settings->overutil_l_hold_cnt++;
			if (settings->overutil_l_hold_cnt >=
			    settings->overutil_l_hold_time)
				return true;
		} else {
			settings->overutil_l_hold_cnt = 0;
		}

		return false;
	}
#endif

	/* check freq */
	cur_freq_L = mt_cpufreq_get_cur_phy_freq_no_lock(MT_CPU_DVFS_L);
	ppm_dbg(HICA, "L cur freq = %d\n", cur_freq_L);

	if (cur_freq_L < get_cluster_max_cpufreq(0)) {
		settings->freq_hold_cnt++;
		if (settings->freq_hold_cnt >= settings->freq_hold_time)
			return true;
	} else
		settings->freq_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_L_ONLY_to_4L_LL(struct ppm_hica_algo_data data,
					   struct ppm_state_transfer *settings)
{
#ifdef PPM_TURBO_CORE_SUPPORT
	if (ppm_main_info.smart_detect_boost && ppm_main_info.is_turbo_core) {
		ppm_dbg(HICA,
			"Go to 4L_LL due to smart detect boost is enabled\n");
		return true;
	}
#endif

	/* check loading only */
	if (data.ppm_cur_loads >
	    (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_4LL_L_to_LL_ONLY(struct ppm_hica_algo_data data,
					    struct ppm_state_transfer *settings)
{
#ifdef PPM_TURBO_CORE_SUPPORT
	/* Keep L cluster on */
	if (ppm_main_info.smart_detect_boost && ppm_main_info.is_turbo_core) {
		ppm_dbg(HICA,
			"Stay in 4LL_L due to smart detect boost is enabled\n");
		return false;
	}
#endif

	/* check loading only */
	if (data.ppm_cur_loads <=
	    (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

static bool ppm_trans_rule_4L_LL_to_L_ONLY(struct ppm_hica_algo_data data,
					   struct ppm_state_transfer *settings)
{
#ifdef PPM_TURBO_CORE_SUPPORT
	if (ppm_main_info.smart_detect_boost && ppm_main_info.is_turbo_core) {
		ppm_dbg(HICA,
			"Stay in 4L_LL due to smart detect boost is enabled\n");
		return false;
	}
#endif

	/* check loading only */
	if (data.ppm_cur_loads <=
	    (settings->loading_bond - settings->loading_delta)) {
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
		   ? pwr_state_info_FY
		   : pwr_state_info_SB;
}

const char *ppm_get_power_state_name(enum ppm_power_state state)
{
	if (state >= NR_PPM_POWER_STATE)
		return "NONE";

	/* the state name is the same between FY and SB */
	return pwr_state_info_FY[state].name;
}

enum ppm_power_state
ppm_change_state_with_fix_root_cluster(enum ppm_power_state cur_state,
				       int cluster)
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

unsigned int ppm_get_root_cluster_by_state(enum ppm_power_state cur_state)
{
	unsigned int root_cluster = 0;

	switch (cur_state) {
	case PPM_POWER_STATE_L_ONLY:
	case PPM_POWER_STATE_4L_LL:
		root_cluster = 1;
		break;
	case PPM_POWER_STATE_NONE:
		root_cluster =
		    (ppm_main_info.fixed_root_cluster == -1)
			? 0
			: (unsigned int)ppm_main_info.fixed_root_cluster;
		break;
	case PPM_POWER_STATE_LL_ONLY:
	case PPM_POWER_STATE_4LL_L:
	default:
		break;
	}

	return root_cluster;
}

enum ppm_power_state ppm_find_next_state(enum ppm_power_state state,
					 unsigned int *level,
					 enum power_state_search_policy policy)
{
	const unsigned int *tbl;
	enum ppm_power_state new_state;
	int keep_going;

	ppm_ver("@%s: state = %s, lv = %d\n", __func__,
		ppm_get_power_state_name(state), *level);

	if (state >= NR_PPM_POWER_STATE || *level >= NR_PPM_POWER_STATE)
		return PPM_POWER_STATE_NONE;

	tbl = (policy == PERFORMANCE) ? perf_idx_search_prio[state]
				      : pwr_idx_search_prio[state];

	do {
		keep_going = 0;

		new_state = tbl[*level];

		ppm_ver("@%s: new_state = %s, lv = %d\n", __func__,
			ppm_get_power_state_name(new_state), *level);

		if (new_state == PPM_POWER_STATE_NONE)
			break;

		/* check fix root cluster setting */
		switch (ppm_main_info.fixed_root_cluster) {
		case 0:
			if (new_state == PPM_POWER_STATE_L_ONLY ||
			    new_state == PPM_POWER_STATE_4L_LL) {
				(*level)++;
				if ((*level) < NR_PPM_POWER_STATE)
					keep_going = 1;
			}
			break;
		case 1:
			if (new_state == PPM_POWER_STATE_LL_ONLY ||
			    new_state == PPM_POWER_STATE_4LL_L) {
				(*level)++;
				if ((*level) < NR_PPM_POWER_STATE)
					keep_going = 1;
			}
			break;
		default:
			break;
		}
	} while (keep_going);

	return new_state;
}

#ifdef PPM_IC_SEGMENT_CHECK
enum ppm_power_state ppm_check_fix_state_by_segment(void)
{
	unsigned int segment = get_devinfo_with_index(21) & 0xFF;
	enum ppm_power_state fix_state = PPM_POWER_STATE_NONE;

	switch (segment) {
	case 0x43: /* fix L only */
		fix_state = PPM_POWER_STATE_L_ONLY;
		break;
	default:
		break;
	}

	ppm_info("segment = 0x%x, fix_state = %s\n", segment,
		 ppm_get_power_state_name(fix_state));

	return fix_state;
}
#endif

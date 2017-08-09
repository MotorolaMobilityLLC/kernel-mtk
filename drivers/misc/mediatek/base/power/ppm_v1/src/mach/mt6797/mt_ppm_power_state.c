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

#ifdef PPM_POWER_TABLE_CALIBRATION
#define STATE_PWR_TBL(name, type)	\
struct ppm_state_sorted_pwr_tbl_data pwr_sorted_tbl_##name##_##type = {	\
	.sorted_tbl = state_pwr_tbl_##name##_##type,		\
	.size = ARRAY_SIZE(state_pwr_tbl_##name##_##type),	\
}

#define STATE_PERF_TBL(name, type)	\
struct ppm_state_sorted_pwr_tbl_data perf_sorted_tbl_##name##_##type = {	\
	.sorted_tbl = state_perf_tbl_##name##_##type,		\
	.size = ARRAY_SIZE(state_perf_tbl_##name##_##type),	\
}
#else
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
#endif

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
	[0] = LIMIT(15, 0, 1, 4),
	[1] = LIMIT(15, 0, 0, 0),
	[2] = LIMIT(15, 0, 0, 0),
};
STATE_LIMIT(LL_ONLY);

static const struct ppm_cluster_limit state_limit_L_ONLY[] = {
	[0] = LIMIT(15, 0, 0, 0),
	[1] = LIMIT(8, 0, 1, 4),
	[2] = LIMIT(15, 0, 0, 0),
};
STATE_LIMIT(L_ONLY);

static const struct ppm_cluster_limit state_limit_4LL_L[] = {
	[0] = LIMIT(15, 0, 1, 4),
	[1] = LIMIT(15, 0, 0, 4),
	[2] = LIMIT(15, 0, 0, 2),
};
STATE_LIMIT(4LL_L);

static const struct ppm_cluster_limit state_limit_4L_LL[] = {
	[0] = LIMIT(15, 0, 0, 4),
	[1] = LIMIT(15, 0, 1, 4),
	[2] = LIMIT(15, 0, 0, 2),
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
#ifdef DISABLE_CLUSTER_MIGRATION
		0,
#else
		PPM_MODE_MASK_JUST_MAKE_ONLY | PPM_MODE_MASK_PERFORMANCE_ONLY,
#endif
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
#ifdef PPM_POWER_TABLE_CALIBRATION
#include "mt_ppm_power_table_calibration.h"
#else
#include "mt_ppm_power_table.h"
#endif
/*==============================================================*/
/* Power Table auto-generated by script	end			*/
/*==============================================================*/

/* default use FY table, may change to SB if needed */
#ifdef PPM_POWER_TABLE_CALIBRATION
struct ppm_power_tbl_data power_table_FY = {
#else
const struct ppm_power_tbl_data power_table_FY = {
#endif
	.power_tbl = cpu_tlp_power_tbl_FY,
	.nr_power_tbl = ARRAY_SIZE(cpu_tlp_power_tbl_FY),
};

#ifdef PPM_POWER_TABLE_CALIBRATION
struct ppm_power_tbl_data power_table_SB = {
#else
const struct ppm_power_tbl_data power_table_SB = {
#endif
	.power_tbl = cpu_tlp_power_tbl_SB,
	.nr_power_tbl = ARRAY_SIZE(cpu_tlp_power_tbl_SB),
};

#ifdef PPM_POWER_TABLE_CALIBRATION
struct ppm_power_tbl_data power_table_MB = {
#else
const struct ppm_power_tbl_data power_table_MB = {
#endif
	.power_tbl = cpu_tlp_power_tbl_MB,
	.nr_power_tbl = ARRAY_SIZE(cpu_tlp_power_tbl_MB),
};

const struct ppm_pwr_idx_ref_tbl_data pwr_idx_ref_tbl_FY = {
	.pwr_idx_ref_tbl = cpu_pwr_idx_ref_tbl_FY,
	.nr_pwr_idx_ref_tbl = ARRAY_SIZE(cpu_pwr_idx_ref_tbl_FY),
};

const struct ppm_pwr_idx_ref_tbl_data pwr_idx_ref_tbl_SB = {
	.pwr_idx_ref_tbl = cpu_pwr_idx_ref_tbl_SB,
	.nr_pwr_idx_ref_tbl = ARRAY_SIZE(cpu_pwr_idx_ref_tbl_SB),
};

const struct ppm_pwr_idx_ref_tbl_data pwr_idx_ref_tbl_MB = {
	.pwr_idx_ref_tbl = cpu_pwr_idx_ref_tbl_MB,
	.nr_pwr_idx_ref_tbl = ARRAY_SIZE(cpu_pwr_idx_ref_tbl_MB),
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

struct ppm_power_state_data pwr_state_info_MB[NR_PPM_POWER_STATE] = {
	[0] = {
		.name = __stringify(LL_ONLY),
		.state = PPM_POWER_STATE_LL_ONLY,
		PWR_STATE_TBLS(LL_ONLY, MB)
	},
	[1] = {
		.name = __stringify(L_ONLY),
		.state = PPM_POWER_STATE_L_ONLY,
		PWR_STATE_TBLS(L_ONLY, MB)
	},
	[2] = {
		.name = __stringify(4LL_L),
		.state = PPM_POWER_STATE_4LL_L,
		PWR_STATE_TBLS(4LL_L, MB)
	},
	[3] = {
		.name = __stringify(4L_LL),
		.state = PPM_POWER_STATE_4L_LL,
		PWR_STATE_TBLS(4L_LL, MB)
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
/* Local Function Implementation				*/
/*==============================================================*/
/* transition rules */
static bool ppm_trans_rule_LL_ONLY_to_L_ONLY(
	struct ppm_hica_algo_data data, struct ppm_state_transfer *settings)
{
	unsigned int cur_freq_LL;

	/* keep in LL_ONLY state if root cluster is fixed at cluster 0 */
	if (ppm_main_info.fixed_root_cluster == PPM_CLUSTER_LL)
		return false;

	/* keep in LL ONLY state if LCM is off */
	if (ppm_lcmoff_is_policy_activated())
		return false;

	/* check heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	{
		unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_LL);

		if (heavy_task && data.ppm_cur_tlp <= settings->tlp_bond) {
			ppm_dbg(HICA, "LL heavy task = %d\n", heavy_task);
			return true;
		}
	}
#endif

	/* check loading */
	if (data.ppm_cur_loads > (settings->loading_bond - settings->loading_delta)
		&& data.ppm_cur_tlp <= settings->tlp_bond) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	/* check freq */
	cur_freq_LL = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_LL);	/* FIXME */
	ppm_dbg(HICA, "LL cur freq = %d\n", cur_freq_LL);

	if (cur_freq_LL >= get_cluster_max_cpufreq(PPM_CLUSTER_LL)) {
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
	/* check heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_LL);

	if (heavy_task && data.ppm_cur_tlp > settings->tlp_bond) {
		ppm_dbg(HICA, "LL heavy task = %d\n", heavy_task);
		return true;
	}
#endif

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
	unsigned int cur_freq_L;

	/* keep in L_ONLY state if root cluster is fixed at L */
	if (ppm_main_info.fixed_root_cluster == PPM_CLUSTER_L)
		return false;

	/* stay if L has heavy task, should be transferred to 4L+LL */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	{
		unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_L);

		if (heavy_task) {
			ppm_dbg(HICA, "L heavy task = %d\n", heavy_task);
			return false;
		}
	}
#endif

	cur_freq_L = mt_cpufreq_get_cur_phy_freq(MT_CPU_DVFS_L); /* FIXME */
	ppm_dbg(HICA, "L cur freq = %d\n", cur_freq_L);

	if (cur_freq_L < get_cluster_max_cpufreq(PPM_CLUSTER_LL)) {
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
	/* check heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task = hps_get_hvytsk(PPM_CLUSTER_L);

	if (heavy_task) {
		ppm_dbg(HICA, "L heavy task = %d\n", heavy_task);
		return true;
	}
#endif

	/* check loading */
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
	/* stay if LL/L/B has heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task;
	int i;

	for_each_ppm_clusters(i) {
		heavy_task = hps_get_hvytsk(i);
		if (heavy_task) {
			ppm_dbg(HICA, "Cluster%d heavy task = %d\n", i, heavy_task);
			return false;
		}
	}
#endif

	/* check loading */
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
	/* stay if LL/L/B has heavy task */
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
	unsigned int heavy_task;
	int i;

	for_each_ppm_clusters(i) {
		heavy_task = hps_get_hvytsk(i);
		if (heavy_task) {
			ppm_dbg(HICA, "Cluster%d heavy task = %d\n", i, heavy_task);
			return false;
		}
	}
#endif

	/* check loading */
	if (data.ppm_cur_loads <= (settings->loading_bond - settings->loading_delta)) {
		settings->loading_hold_cnt++;
		if (settings->loading_hold_cnt >= settings->loading_hold_time)
			return true;
	} else
		settings->loading_hold_cnt = 0;

	return false;
}

#ifdef PPM_POWER_TABLE_CALIBRATION
static struct ppm_pwr_idx_ref_tbl_data ppm_get_pwr_idx_ref_tbl(void)
#else
static const struct ppm_pwr_idx_ref_tbl_data ppm_get_pwr_idx_ref_tbl(void)
#endif
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? pwr_idx_ref_tbl_SB
		: (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_MB) ? pwr_idx_ref_tbl_MB
		: pwr_idx_ref_tbl_FY;
}

/*==============================================================*/
/* Global Function Implementation				*/
/*==============================================================*/
struct ppm_power_state_data *ppm_get_power_state_info(void)
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? pwr_state_info_SB
		: (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_MB) ? pwr_state_info_MB
		: pwr_state_info_FY;
}

#ifdef PPM_POWER_TABLE_CALIBRATION
struct ppm_power_tbl_data ppm_get_power_table(void)
#else
const struct ppm_power_tbl_data ppm_get_power_table(void)
#endif
{
	return (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? power_table_SB
		: (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_MB) ? power_table_MB
		: power_table_FY;
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
		else if (cur_state == PPM_POWER_STATE_4L_LL)
			new_state = PPM_POWER_STATE_4LL_L;
		break;
	case PPM_CLUSTER_L:
		if (cur_state == PPM_POWER_STATE_LL_ONLY)
			new_state = PPM_POWER_STATE_L_ONLY;
		else if (cur_state == PPM_POWER_STATE_4LL_L)
			new_state = PPM_POWER_STATE_4L_LL;
		break;
	/* We do not support to fix root cluster at B */
	case PPM_CLUSTER_B:
	default:
		break;
	}

	return new_state;
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
			if (new_state == PPM_POWER_STATE_L_ONLY || new_state == PPM_POWER_STATE_4L_LL) {
				(*level)++;
				keep_going = 1;
			}
			break;
		case PPM_CLUSTER_L:
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
		LL_freq_min = ppm_main_info.cluster_info[PPM_CLUSTER_LL].dvfs_tbl[LL_freq_min].frequency;
		L_freq_max = ppm_main_info.cluster_info[PPM_CLUSTER_L].dvfs_tbl[L_freq_max].frequency;
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

#ifdef DISABLE_CLUSTER_MIGRATION
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
	if (req && cur_state == PPM_POWER_STATE_L_ONLY) {
		unsigned int LL_min_core = req->limit[PPM_CLUSTER_LL].min_cpu_core;
		unsigned int L_min_core = req->limit[PPM_CLUSTER_L].min_cpu_core;
		unsigned int sum = LL_min_core + L_min_core;
		unsigned int LL_min_freq, L_min_freq, L_max_freq;

		LL_min_freq = ppm_main_info.cluster_info[PPM_CLUSTER_LL]
				.dvfs_tbl[req->limit[PPM_CLUSTER_LL].min_cpufreq_idx].frequency;
		L_min_freq = ppm_main_info.cluster_info[PPM_CLUSTER_L]
				.dvfs_tbl[req->limit[PPM_CLUSTER_L].min_cpufreq_idx].frequency;
		L_max_freq = ppm_main_info.cluster_info[PPM_CLUSTER_L]
				.dvfs_tbl[req->limit[PPM_CLUSTER_L].max_cpufreq_idx].frequency;

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

unsigned int ppm_get_root_cluster_by_state(enum ppm_power_state cur_state)
{
	unsigned int root_cluster = PPM_CLUSTER_LL;

	switch (cur_state) {
	case PPM_POWER_STATE_L_ONLY:
	case PPM_POWER_STATE_4L_LL:
		root_cluster = PPM_CLUSTER_L;
		break;
	case PPM_POWER_STATE_NONE:
		root_cluster = (ppm_main_info.fixed_root_cluster == -1) ? PPM_CLUSTER_LL
				: (unsigned int)ppm_main_info.fixed_root_cluster;
		break;
	case PPM_POWER_STATE_LL_ONLY:
	case PPM_POWER_STATE_4LL_L:
	default:
		break;
	}

	return root_cluster;
}

#if PPM_HW_OCP_SUPPORT
#include <linux/delay.h>
#include <linux/ktime.h>
#include "mt_idvfs.h"

static unsigned int max_power;
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
	int i, ret = 0;
	unsigned int power_for_ocp = 0, power_for_tbl_lookup = 0;

	/* no need to set big OCP since big cluster is powered off */
	if (!ppm_is_big_cluster_on())
		return limited_power;

	/* if max_power < limited_power, set (limited_power - max_power) to HW OCP */
	if (!max_power) {
		/* get max power budget for SW DLPT */
		for_each_pwr_tbl_entry(i, power_table) {
			if (power_table.power_tbl[i].cluster_cfg[PPM_CLUSTER_B].core_num == 0) {
				max_power = (power_table.power_tbl[i].power_idx);
				break;
			}
		}
		ppm_info("@%s: max_power = %d\n", __func__, max_power);
	}

	if (limited_power <= max_power) {
		/* disable HW OCP by setting budget to 0 */
		power_for_ocp = 0;
		power_for_tbl_lookup = limited_power;
	} else {
		/* pass remaining power to HW OCP */
		power_for_ocp = (percentage)
			? ((limited_power - max_power) * 100 + (percentage - 1)) / percentage
			: (limited_power - max_power);
		power_for_ocp = (power_for_ocp > MAX_OCP_TARGET_POWER)
			? MAX_OCP_TARGET_POWER : power_for_ocp;
		power_for_tbl_lookup = max_power;
	}

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
unsigned int ppm_calc_total_power(struct ppm_cluster_status *cluster_status,
				unsigned int cluster_num, unsigned int percentage)
{
	unsigned int budget = 0;

#if 0/* PPM_HW_OCP_SUPPORT */
	int i, ret;

	for (i = 0; i < cluster_num; i++) {
		/* read power meter for total power calculation */
		if (cluster_status[i].core_num) {
			unsigned int freq =
				ppm_main_info.cluster_info[i].dvfs_tbl[cluster_status[i].freq_idx].frequency;
			unsigned int count = (i == PPM_CLUSTER_B)
				? 30000 : get_cluster_min_cpufreq(i);
			unsigned long long leakage = 0, total = 0;
			ktime_t now, delta;

			if (i == PPM_CLUSTER_B) {
				now = ktime_get();
				total = BigOCPAvgPwrGet(count);
				if (!total) {
					ppm_warn("cluster %d OCP capture failed!\n", i);
					return 0; /* error */
				}
				budget += total;
				delta = ktime_sub(ktime_get(), now);
			} else {
				int retry = 20;

				now = ktime_get();
				LittleOCPDVFSSet(i, freq / 1000, cluster_status[i].volt);
				LittleOCPAvgPwr(i, 1, count);
				do {
					ret = LittleOCPAvgPwrGet(i, &leakage, &total);
					if (!ret && total) /* success */
						break;
					retry--;
					udelay(100);
				} while (retry > 0);
				if (!retry) {
					ppm_warn("cluster %d OCP capture failed, ret = %d, total = %llu\n",
						i, ret, total);
					return 0; /* error */
				}

#if 0	/* wait for mcusys power calculation formula */
				budget += (i == PPM_CLUSTER_LL)
					? ((207 + total) * cluster_status[i].volt + (1000 - 1)) / 1000
					: (total * cluster_status[i].volt + (1000 - 1)) / 1000;
#else
				total = (total * cluster_status[i].volt + (1000 - 1)) / 1000;
				budget += total;
#endif
				delta = ktime_sub(ktime_get(), now);
			}

			ppm_dbg(DLPT, "ocp(%d):F/V/core/cnt/Lkg/total = %d/%d/%d/%d/%llu/%llu(time = %lldus)\n",
				i, freq, cluster_status[i].volt, cluster_status[i].core_num, count,
				leakage, total, ktime_to_us(delta));
		}
	}
#else
	struct ppm_pwr_idx_ref_tbl_data ref_tbl = ppm_get_pwr_idx_ref_tbl();
	unsigned int dynamic, lkg, total;
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
#endif
	ppm_dbg(DLPT, "@%s: total budget = %d\n", __func__, budget);

	return budget;
}
#endif

#ifdef PPM_POWER_TABLE_CALIBRATION
static enum ppm_power_state ppm_judge_state_for_pwr_tbl(struct ppm_power_tbl tbl_data)
{
	if (tbl_data.cluster_cfg[PPM_CLUSTER_LL].core_num == 0)
		return PPM_POWER_STATE_L_ONLY;
	else if (tbl_data.cluster_cfg[PPM_CLUSTER_L].core_num == 0)
		return PPM_POWER_STATE_LL_ONLY;
	else if (tbl_data.cluster_cfg[PPM_CLUSTER_LL].core_num == get_cluster_max_cpu_core(PPM_CLUSTER_LL)
		&& tbl_data.cluster_cfg[PPM_CLUSTER_L].core_num == get_cluster_max_cpu_core(PPM_CLUSTER_L))
		return PPM_POWER_STATE_NONE; /* for both 4LL_L and 4L_LL*/
	else if (tbl_data.cluster_cfg[PPM_CLUSTER_LL].core_num == get_cluster_max_cpu_core(PPM_CLUSTER_LL))
		return PPM_POWER_STATE_4LL_L;
	else
		return PPM_POWER_STATE_4L_LL;
}

static void ppm_add_to_sorted_tbl(int pwr_tbl_idx, int perf_sort_tbl_idx,
				int pwr_sort_tbl_idx, enum ppm_power_state state)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	struct ppm_state_sorted_pwr_tbl_data *tbl;

	tbl = state_info[state].perf_sorted_tbl;
	tbl->sorted_tbl[perf_sort_tbl_idx].index = power_table.power_tbl[pwr_tbl_idx].index;
	tbl->sorted_tbl[perf_sort_tbl_idx].value = power_table.power_tbl[pwr_tbl_idx].perf_idx;
	tbl->sorted_tbl[perf_sort_tbl_idx].advise_index = 0xFF;

	tbl = state_info[state].pwr_sorted_tbl;
	tbl->sorted_tbl[pwr_sort_tbl_idx].index = power_table.power_tbl[pwr_tbl_idx].index;
	tbl->sorted_tbl[pwr_sort_tbl_idx].value = power_table.power_tbl[pwr_tbl_idx].power_idx;
	tbl->sorted_tbl[pwr_sort_tbl_idx].advise_index = 0xFF; /* calculate this later */
}

static void ppm_gen_sorted_table(void)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	struct ppm_power_state_data *state_info = ppm_get_power_state_info();
	struct ppm_state_sorted_pwr_tbl_data *perf_tbl, *pwr_tbl;
	enum ppm_power_state state;
	int index_tbl_perf[NR_PPM_POWER_STATE] = {0};
	int index_tbl_pwr[NR_PPM_POWER_STATE] = {0};
	int i, j, k;
	unsigned int idx_in_pwr_tbl, idx_in_perf_tbl;
	unsigned int tmp_idx, tmp_val;

	for_each_pwr_tbl_entry(i, power_table) {
		/* decide PPM state for current idx */
		state = ppm_judge_state_for_pwr_tbl(power_table.power_tbl[i]);

		/* append to corresponding sorted table */
		/* p.s. append to 4LL_L and 4L_LL if state is NONE! */
		if (state == PPM_POWER_STATE_NONE) {
			state = PPM_POWER_STATE_4LL_L;
			ppm_add_to_sorted_tbl(i, index_tbl_perf[state], index_tbl_pwr[state], state);
			index_tbl_perf[state]++;
			index_tbl_pwr[state]++;

			state = PPM_POWER_STATE_4L_LL;
			ppm_add_to_sorted_tbl(i, index_tbl_perf[state], index_tbl_pwr[state], state);
			index_tbl_perf[state]++;
			index_tbl_pwr[state]++;
		} else {
			ppm_add_to_sorted_tbl(i, index_tbl_perf[state], index_tbl_pwr[state], state);
			index_tbl_perf[state]++;
			index_tbl_pwr[state]++;
		}
	}

	/* sort perf tbl */
	for_each_ppm_power_state(i) {
		perf_tbl = state_info[i].perf_sorted_tbl;
		pwr_tbl = state_info[i].pwr_sorted_tbl;

		for (j = perf_tbl->size; j > 0; j--) {
			for (k = 1; k < j; k++) {
				if (perf_tbl->sorted_tbl[k - 1].value < perf_tbl->sorted_tbl[k].value) {
					/* ignore advise_index since it is unused */
					tmp_idx = perf_tbl->sorted_tbl[k - 1].index;
					tmp_val = perf_tbl->sorted_tbl[k - 1].value;

					perf_tbl->sorted_tbl[k - 1].index = perf_tbl->sorted_tbl[k].index;
					perf_tbl->sorted_tbl[k - 1].value = perf_tbl->sorted_tbl[k].value;

					perf_tbl->sorted_tbl[k].index = tmp_idx;
					perf_tbl->sorted_tbl[k].value = tmp_val;
				}
			}
		}

		/* Dump perf sorted table */
		ppm_info("perf sorted table for %s:\n", state_info[i].name);
		for (j = 0; j < perf_tbl->size; j++) {
			ppm_info("[%d, %d, %d]\n",
				perf_tbl->sorted_tbl[j].index,
				perf_tbl->sorted_tbl[j].value,
				perf_tbl->sorted_tbl[j].advise_index
				);

		}
		ppm_info("\n");

		/* found advise idx in pwr_sorted_tbl */
		for (j = 0; j < pwr_tbl->size; j++) {
			idx_in_pwr_tbl = pwr_tbl->sorted_tbl[j].index;

			/* find best perf_idx item
			   idx_in_pwr_tbl > idx_in_perf_tbl: lower pwr_idx but higher perf_idx
			   idx_in_pwr_tbl == idx_in_perf_tbl: itself is the best one
			*/
			for (k = 0; k < perf_tbl->size; k++) {
				idx_in_perf_tbl = perf_tbl->sorted_tbl[k].index;

				if (idx_in_perf_tbl >= idx_in_pwr_tbl) {
					pwr_tbl->sorted_tbl[j].advise_index = idx_in_perf_tbl;
					break;
				}
			}
		}

		/* Dump pwr sorted table */
		ppm_info("pwr sorted table for %s:\n", state_info[i].name);
		for (j = 0; j < pwr_tbl->size; j++) {
			ppm_info("[%d, %d, %d]\n",
				pwr_tbl->sorted_tbl[j].index,
				pwr_tbl->sorted_tbl[j].value,
				pwr_tbl->sorted_tbl[j].advise_index
				);
		}
		ppm_info("\n");
	}
}

static void ppm_main_pwr_tbl_cpy(struct ppm_power_tbl *dest, struct ppm_power_tbl *src)
{
	int i;

	dest->index = src->index;
	dest->perf_idx = src->perf_idx;
	dest->power_idx = src->power_idx;
	dest->power_idx_max = src->power_idx_max;
	dest->power_idx_min = src->power_idx_min;

	for_each_ppm_clusters(i) {
		dest->cluster_cfg[i].opp_lv = src->cluster_cfg[i].opp_lv;
		dest->cluster_cfg[i].core_num = src->cluster_cfg[i].core_num;
	}
}

static int __init ppm_main_pwr_tbl_calibration(void)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	struct ppm_pwr_idx_ref_tbl_data ref_tbl = ppm_get_pwr_idx_ref_tbl();
	int big_lkg_efuse = mt_spower_get_efuse_lkg(MT_SPOWER_CPUBIG);
	unsigned int lkg_ratio, diff = BIG_LKG_EFUSE_MAX - BIG_LKG_EFUSE_MIN;
	unsigned int i, j, p_max, p_min;
	char buf[64];
	char *ptr;

	lkg_ratio = (big_lkg_efuse >= BIG_LKG_EFUSE_MAX) ? 100
		: (big_lkg_efuse <= BIG_LKG_EFUSE_MIN) ? 0
		: ((big_lkg_efuse - BIG_LKG_EFUSE_MIN) * 100 + (diff - 1)) / diff;

	ppm_info("big_lkg_efuse = %d, lkg_ratio = %d\n", big_lkg_efuse, lkg_ratio);

	/* calibrate power_idx */
	for_each_pwr_tbl_entry(i, power_table) {
		p_max = power_table.power_tbl[i].power_idx_max;
		p_min = power_table.power_tbl[i].power_idx_min;
		power_table.power_tbl[i].power_idx = p_min + ((p_max - p_min) * lkg_ratio + (100 - 1)) / 100;
	}

	/* sort power table by power budget */
	for (i = power_table.nr_power_tbl; i > 0; i--) {
		for (j = 1; j < i; j++) {
			struct ppm_power_tbl *prev = &(power_table.power_tbl[j - 1]);
			struct ppm_power_tbl *cur = &(power_table.power_tbl[j]);

			if (prev->power_idx < cur->power_idx) {
				struct ppm_power_tbl tmp;

				ppm_main_pwr_tbl_cpy(&tmp, prev);
				ppm_main_pwr_tbl_cpy(prev, cur);
				ppm_main_pwr_tbl_cpy(cur, &tmp);
			}
		}
	}

	/* dump power table */
	ppm_info("PPM power table:\n");
	for_each_pwr_tbl_entry(i, power_table) {
		ptr = buf;
		memset(buf, 0, sizeof(buf));

		ptr += sprintf(ptr, "[%d] = ", power_table.power_tbl[i].index);

		for_each_ppm_clusters(j) {
			ptr += sprintf(ptr, "(%d, %d), ",
				power_table.power_tbl[i].cluster_cfg[j].opp_lv,
				power_table.power_tbl[i].cluster_cfg[j].core_num
			);
		}

		ptr += sprintf(ptr, "%d, %d",
			power_table.power_tbl[i].perf_idx,
			power_table.power_tbl[i].power_idx
		);

		ppm_info("%s\n", buf);
	}
	ppm_info("\n");

	/* generate pwr/perf sorted table for each state */
	ppm_gen_sorted_table();

	/* calibrate pwr_idx ref table */
	ppm_info("PPM pwr_idx ref table:\n");
	for_each_ppm_clusters(i) {
		for (j = 0; j < DVFS_OPP_NUM; j++) {
			/* calculate actual total power */
			p_max = ref_tbl.pwr_idx_ref_tbl[i].core_total_power_max[j];
			p_min = ref_tbl.pwr_idx_ref_tbl[i].core_total_power_min[j];
			ref_tbl.pwr_idx_ref_tbl[i].core_total_power[j] =
				p_min + ((p_max - p_min) * lkg_ratio + (100 - 1)) / 100;

			/* calculate actual L2 power */
			p_max = ref_tbl.pwr_idx_ref_tbl[i].l2_power_max[j];
			p_min = ref_tbl.pwr_idx_ref_tbl[i].l2_power_min[j];
			ref_tbl.pwr_idx_ref_tbl[i].l2_power[j] =
				p_min + ((p_max - p_min) * lkg_ratio + (100 - 1)) / 100;

			ppm_info("cluster %d(%d): (%d, %d)\n", i, j,
				ref_tbl.pwr_idx_ref_tbl[i].core_total_power[j],
				ref_tbl.pwr_idx_ref_tbl[i].l2_power[j]
				);
		}
	}

	return 0;
}

late_initcall(ppm_main_pwr_tbl_calibration);
#endif


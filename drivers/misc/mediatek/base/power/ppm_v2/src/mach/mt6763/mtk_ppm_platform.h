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


#ifndef __MT_PPM_PLATFORM_H__
#define __MT_PPM_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mach/mtk_ppm_api.h"
#include "mach/mtk_cpufreq_api.h"


/*==============================================================*/
/* Macros							*/
/*==============================================================*/
/* ppm driver update state to MET directly  0: turn off */
#define PPM_UPDATE_STATE_DIRECT_TO_MET  (1)

#define PPM_HEAVY_TASK_INDICATE_SUPPORT	(1)
#define PPM_BIG_TASK_INDICATE_SUPPORT	(1)
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
extern unsigned int hps_get_hvytsk(unsigned int cluster_id);
#endif
#if PPM_BIG_TASK_INDICATE_SUPPORT
extern unsigned int hps_get_bigtsk(unsigned int cluster_id);
#endif

#define PPM_HW_OCP_SUPPORT		(0)
#define PPM_DLPT_DEFAULT_MODE		(SW_MODE)
#define PPM_DLPT_ENHANCEMENT		(1)
#if PPM_DLPT_ENHANCEMENT
#define DYNAMIC_TABLE2REAL_PERCENTAGE	(58)
#endif

#define PPM_HICA_VARIANT_SUPPORT	(0)
#define PPM_HICA_2P0			(1)
#define PPM_CAPACITY_UP			(90)
#define PPM_CAPACITY_DOWN		(80)

#ifndef PPM_DISABLE_CLUSTER_MIGRATION
#define PPM_CLUSTER_MIGRATION_BOOST	(1)
#endif

/*#define PPM_DISABLE_BIG_FOR_LP_MODE	(1)*/
/*#define PPM_DISABLE_LL_ONLY		(1)*/
/* TODO: check this */
#define PPM_1LL_MIN_FREQ		(741000)

/* for COBRA algo */
#define PPM_COBRA_USE_CORE_LIMIT	(1)
#define PPM_COBRA_RUNTIME_CALC_DELTA	(0)
#if PPM_COBRA_USE_CORE_LIMIT
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM)
#else
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM-1)
#endif
#define COBRA_OPP_NUM	(DVFS_OPP_NUM)
#define TOTAL_CORE_NUM	(CORE_NUM_LL+CORE_NUM_L)
#define CORE_NUM_LL	(4)
#define CORE_NUM_L	(4)



/* other policy settings */
/* check these settings */
#define	LCMOFF_MIN_FREQ		(598000)
#define	PTPOD_FREQ_IDX_FY	(10)
#define	PTPOD_FREQ_IDX_SB	(10)
#define PWRTHRO_BAT_PER_MW	(600)
#define PWRTHRO_BAT_OC_MW	(600)
#define PWRTHRO_LOW_BAT_LV1_MW	(600)
#define PWRTHRO_LOW_BAT_LV2_MW	(600)

#define FREQ_LIMIT_FOR_KE	(800000)

#define DVFS_OPP_NUM		(16)

#define PPM_DEFAULT_HOLD_TIME		(4)
#define PPM_DEFAULT_BIGTSK_TIME		(1)
#define PPM_DEFAULT_FREQ_HOLD_TIME	(4)
#define PPM_DEFAULT_DELTA		(20)
#define PPM_LOADING_UPPER		(400)
#define PPM_TLP_CRITERIA		(400)

#define get_cluster_lcmoff_min_freq(id)		LCMOFF_MIN_FREQ	/* the same for each cluster */
#define get_cluster_ptpod_fix_freq_idx(id)						\
	((ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? PTPOD_FREQ_IDX_SB	\
	: PTPOD_FREQ_IDX_FY)
#define get_cluster_suspend_fix_freq(id)	\
	((id == 0) ? SUSPEND_FREQ_LL : SUSPEND_FREQ_L)
#define get_max_real_power_by_segment(seg)		\
	((ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? DLPT_MAX_REAL_POWER_SB	\
	: DLPT_MAX_REAL_POWER_FY)

/*==============================================================*/
/* Enum								*/
/*==============================================================*/
enum ppm_power_state {
	PPM_POWER_STATE_LL_ONLY = 0,
	PPM_POWER_STATE_L_ONLY,
	PPM_POWER_STATE_ALL,

	PPM_POWER_STATE_NONE,	/* HICA disabled */
	NR_PPM_POWER_STATE = PPM_POWER_STATE_NONE,
};

enum ppm_cluster {
	PPM_CLUSTER_LL = 0,
	PPM_CLUSTER_L,

	NR_PPM_CLUSTERS,
};

enum ppm_cobra_lookup_type {
	LOOKUP_BY_BUDGET,
	LOOKUP_BY_LIMIT,

	NR_PPM_COBRA_LOOKUP_TYPE,
};

/*==============================================================*/
/* Data Structures						*/
/*==============================================================*/
struct ppm_cobra_basic_pwr_data {
	unsigned short perf_idx;
	unsigned short power_idx;
};

struct ppm_cobra_delta_data {
	unsigned short delta_pwr;
};

struct ppm_cobra_data {
	struct ppm_cobra_basic_pwr_data basic_pwr_tbl[TOTAL_CORE_NUM][DVFS_OPP_NUM];
#if !PPM_COBRA_RUNTIME_CALC_DELTA
	struct ppm_cobra_delta_data delta_tbl_LxLL[CORE_NUM_L+1][CORE_NUM_LL+1][COBRA_OPP_NUM];
#endif
};

struct ppm_cobra_lookup {
	unsigned int budget;
	struct {
		unsigned int opp;
		unsigned int core;
	} limit[NR_PPM_CLUSTERS];
};

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/
extern struct ppm_cobra_data cobra_tbl;
extern struct ppm_cobra_lookup cobra_lookup_data;

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
#if PPM_DLPT_ENHANCEMENT
extern unsigned int ppm_calc_total_power(struct ppm_cluster_status *cluster_status,
					unsigned int cluster_num, unsigned int percentage);
#endif

extern int ppm_platform_init(void);

/* COBRA algo */
extern void ppm_cobra_update_core_limit(unsigned int cluster, int limit);
extern void ppm_cobra_update_freq_limit(unsigned int cluster, int limit);
extern void ppm_cobra_update_limit(enum ppm_power_state new_state, void *user_req);
extern void ppm_cobra_init(void);
extern void ppm_cobra_dump_tbl(struct seq_file *m);
extern void ppm_cobra_lookup_get_result(struct seq_file *m, enum ppm_cobra_lookup_type type);

extern int sched_get_cluster_util(int cluster_id, unsigned long *usage, unsigned long *capacity);

#ifdef __cplusplus
}
#endif

#endif


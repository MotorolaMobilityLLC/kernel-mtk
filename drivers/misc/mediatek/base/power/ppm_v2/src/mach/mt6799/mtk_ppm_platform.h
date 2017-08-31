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
#include "mtk_ocp.h"


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

#if OCP_FEATURE_ENABLED
/* Waiting for OCP HQA ready */
#define PPM_HW_OCP_SUPPORT		(0)
#define PPM_DLPT_DEFAULT_MODE		(SW_MODE)
#else
#define PPM_HW_OCP_SUPPORT		(0)
#define PPM_DLPT_DEFAULT_MODE		(SW_MODE)
#endif

#define PPM_DLPT_ENHANCEMENT		(1)
#if PPM_DLPT_ENHANCEMENT
#define DYNAMIC_TABLE2REAL_PERCENTAGE	(58)
#endif

#define PPM_HICA_VARIANT_SUPPORT	(0)
#define PPM_HICA_2P0			(1)
#define PPM_HICA_BIG_LIMIT_FREQ		(1500000)
#define PPM_CAPACITY_UP			(90)
#define PPM_CAPACITY_DOWN		(80)

#ifndef PPM_DISABLE_CLUSTER_MIGRATION
#define PPM_CLUSTER_MIGRATION_BOOST	(1)
#endif

#define PPM_DISABLE_BIG_FOR_LP_MODE	(1)
/*#define PPM_DISABLE_LL_ONLY		(1)*/
#define PPM_1LL_MIN_FREQ		(741000)

/* for COBRA algo */
#define PPM_COBRA_USE_CORE_LIMIT	(1)
#define PPM_COBRA_NEED_OPP_MAPPING	(0)
#define PPM_COBRA_TBL_SRAM_ADDR	(0x0012B800)
#define PPM_COBRA_TBL_SRAM_SIZE	(sizeof(struct ppm_cobra_basic_pwr_data)*TOTAL_CORE_NUM*COBRA_OPP_NUM)
#if PPM_COBRA_USE_CORE_LIMIT
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM)
#else
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM-1)
#endif
#if PPM_COBRA_NEED_OPP_MAPPING
#define COBRA_OPP_NUM	(8)
#else
#define COBRA_OPP_NUM	(DVFS_OPP_NUM)
#endif
#ifdef ALL_BUCK_SHARED
#define TOTAL_CORE_NUM	(CORE_NUM_LL+CORE_NUM_L+CORE_NUM_B)
#define CORE_NUM_LL	(4)
#define CORE_NUM_L	(4)
#define CORE_NUM_B	(2)
#else
#define PPM_COBRA_RUNTIME_CALC_DELTA	(1)
/* LL and B share buck */
#define CLUSTER_ID_SHARED_C1	(0)
#define CLUSTER_ID_SHARED_C2	(2)
#define CLUSTER_ID_SINGLE	(1)
#define CORE_NUM_SHARED_C1	(4)
#define CORE_NUM_SHARED_C2	(2)
#define CORE_NUM_SINGLE		(4)
#define TOTAL_CORE_NUM		(CORE_NUM_SHARED_C1+CORE_NUM_SHARED_C2+CORE_NUM_SINGLE)
#endif

/* online core to SSPM */
#define PPM_ONLINE_CORE_SRAM_ADDR	(PPM_COBRA_TBL_SRAM_ADDR + PPM_COBRA_TBL_SRAM_SIZE)

/* DLPT mode */
#define DLPT_MAX_REAL_POWER_FY	(12504)
#define DLPT_MAX_REAL_POWER_SB	(18381)

/* other policy settings */
#define	LCMOFF_MIN_FREQ		(598000)
#define	PTPOD_FREQ_IDX_FY	(0)
#define	PTPOD_FREQ_IDX_FY_B	(0)
#define	PTPOD_FREQ_IDX_SB	(0)
#define SUSPEND_FREQ_LL		(897000)
#define SUSPEND_FREQ_L		(1274000)
#define SUSPEND_FREQ_B		(1001000)
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
	: (id == PPM_CLUSTER_B) ? PTPOD_FREQ_IDX_FY_B					\
	: PTPOD_FREQ_IDX_FY)
#define get_cluster_suspend_fix_freq(id)		\
	((id == PPM_CLUSTER_LL) ? SUSPEND_FREQ_LL	\
	: (id == PPM_CLUSTER_L) ? SUSPEND_FREQ_L	\
	: SUSPEND_FREQ_B)
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
	PPM_CLUSTER_B,

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

#ifdef ALL_BUCK_SHARED
struct ppm_cobra_delta_data {
	unsigned short delta_pwr;
};

struct ppm_cobra_data {
	struct ppm_cobra_basic_pwr_data basic_pwr_tbl[TOTAL_CORE_NUM][DVFS_OPP_NUM];
};

#else
struct ppm_cobra_delta_data {
	unsigned short delta_pwr;
	unsigned short delta_perf;
	unsigned short delta_eff;
};

struct ppm_cobra_data {
	struct ppm_cobra_basic_pwr_data basic_pwr_tbl[TOTAL_CORE_NUM][DVFS_OPP_NUM];
#if !PPM_COBRA_RUNTIME_CALC_DELTA
	struct ppm_cobra_delta_data delta_tbl_shared[CORE_NUM_SHARED_C2+1][CORE_NUM_SHARED_C1+1][COBRA_OPP_NUM];
	struct ppm_cobra_delta_data delta_tbl_single[CORE_NUM_SINGLE+1][COBRA_OPP_NUM];
#endif
};
#endif

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
extern struct ppm_cobra_data *cobra_tbl;
extern struct ppm_cobra_lookup cobra_lookup_data;

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
#if PPM_HW_OCP_SUPPORT
extern unsigned int ppm_set_ocp(unsigned int limited_power, unsigned int percentage);
#endif
#if PPM_DLPT_ENHANCEMENT
#if PPM_HW_OCP_SUPPORT
extern unsigned int ppm_calc_total_power_by_ocp(struct ppm_cluster_status *cluster_status,
				unsigned int cluster_num);
#endif
extern unsigned int ppm_calc_total_power(struct ppm_cluster_status *cluster_status,
					unsigned int cluster_num, unsigned int percentage);
#endif

extern int ppm_platform_init(void);
extern void ppm_power_data_init(void);

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


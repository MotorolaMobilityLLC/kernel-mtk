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

#include "mach/mt_ppm_api.h"

/*==============================================================*/
/* Macros							*/
/*==============================================================*/
/* ppm driver update state to MET directly  0: turn off */
#define PPM_UPDATE_STATE_DIRECT_TO_MET  (1)

#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
#define PPM_IC_SEGMENT_CHECK		(1)
#define PPM_VPROC_5A_LIMIT_CHECK	(1)
#define PPM_5A_LIMIT_FREQ_IDX		(1)
#endif

#define PPM_HW_OCP_SUPPORT		(0)
#define PPM_DLPT_ENHANCEMENT		(0)

/* for COBRA algo */
#define PPM_COBRA_USE_CORE_LIMIT	(1)
#define PPM_COBRA_NEED_OPP_MAPPING	(0)
#define PPM_COBRA_RUNTIME_CALC_DELTA	(0)
#ifdef PPM_COBRA_USE_CORE_LIMIT
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM)
#else
#define PPM_COBRA_MAX_FREQ_IDX	(COBRA_OPP_NUM-1)
#endif
#define COBRA_OPP_NUM	(8)
#define TOTAL_CORE_NUM	(CORE_NUM_LL+CORE_NUM_L)
#define CORE_NUM_LL	(4)
#define CORE_NUM_L	(4)

/* DLPT */
#define PPM_DLPT_DEFAULT_MODE	(SW_MODE)
#define DLPT_MAX_REAL_POWER_FY	(3890)
#define DLPT_MAX_REAL_POWER_SB	(4992)

/* other policy settings */
#define	LCMOFF_MIN_FREQ		(598000)
#define	PTPOD_FREQ_IDX		(8)
#define SUSPEND_FREQ_LL		(689000)
#define SUSPEND_FREQ_L		(871000)
#define PWRTHRO_BAT_PER_MW	(1896)
#define PWRTHRO_BAT_OC_MW	(1160)
#define PWRTHRO_LOW_BAT_LV1_MW	(1896)
#define PWRTHRO_LOW_BAT_LV2_MW	(1160)

#define FREQ_LIMIT_FOR_KE	(800000)

#define NR_CLUSTERS		(2)
#define DVFS_OPP_NUM		(8)

#define PPM_DEFAULT_HOLD_TIME		(4)
#define PPM_DEFAULT_FREQ_HOLD_TIME	(4)
#define PPM_DEFAULT_DELTA		(20)
#define PPM_LOADING_UPPER		(400)
#define PPM_TLP_CRITERIA		(400)

#define get_cluster_lcmoff_min_freq(id)		LCMOFF_MIN_FREQ	/* the same for each cluster */
#define get_cluster_ptpod_fix_freq_idx(id)	PTPOD_FREQ_IDX	/* the same for each cluster */
#define get_cluster_suspend_fix_freq(id)	\
	((id == 0) ? SUSPEND_FREQ_LL : SUSPEND_FREQ_L)
#define get_max_real_power_by_segment(seg)	\
	((ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_SB) ? DLPT_MAX_REAL_POWER_SB	\
	: DLPT_MAX_REAL_POWER_FY)

/*==============================================================*/
/* Enum								*/
/*==============================================================*/
enum ppm_power_state {
	PPM_POWER_STATE_LL_ONLY = 0,
	PPM_POWER_STATE_L_ONLY,
	PPM_POWER_STATE_4LL_L,
	PPM_POWER_STATE_4L_LL,	/* Need this? */

	PPM_POWER_STATE_NONE,	/* HICA disabled */
	NR_PPM_POWER_STATE = PPM_POWER_STATE_NONE,
};

enum ppm_cluster {
	PPM_CLUSTER_LL = 0,
	PPM_CLUSTER_L,

	NR_PPM_CLUSTERS,
};

/*==============================================================*/
/* Data Structures						*/
/*==============================================================*/
struct ppm_pwr_idx_ref_tbl {
	const unsigned int core_dynamic_power_TT[DVFS_OPP_NUM];
	const unsigned int core_dynamic_power_FF[DVFS_OPP_NUM];
	unsigned int core_dynamic_power[DVFS_OPP_NUM];
	const unsigned int core_total_power_TT[DVFS_OPP_NUM];
	const unsigned int core_total_power_FF[DVFS_OPP_NUM];
	unsigned int core_total_power[DVFS_OPP_NUM];
	const unsigned int l2_power_TT[DVFS_OPP_NUM];
	const unsigned int l2_power_FF[DVFS_OPP_NUM];
	unsigned int l2_power[DVFS_OPP_NUM];
};

struct ppm_pwr_idx_ref_tbl_data {
	struct ppm_pwr_idx_ref_tbl *pwr_idx_ref_tbl;
	const unsigned int nr_pwr_idx_ref_tbl;
};

struct ppm_cobra_basic_pwr_data {
	unsigned int perf_idx;
	unsigned int power_idx;
};

struct ppm_cobra_delta_data {
	unsigned int delta_pwr;
#if 0
	unsigned int delta_perf;
	unsigned int delta_eff;
#endif
};

struct ppm_cobra_data {
	struct ppm_cobra_basic_pwr_data basic_pwr_tbl[TOTAL_CORE_NUM][DVFS_OPP_NUM];
#if !PPM_COBRA_RUNTIME_CALC_DELTA
	struct ppm_cobra_delta_data delta_tbl_LxLL[CORE_NUM_L+1][CORE_NUM_LL+1][COBRA_OPP_NUM];
#endif
};

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/
extern struct ppm_cobra_data cobra_tbl;

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
#ifdef PPM_IC_SEGMENT_CHECK
extern enum ppm_power_state ppm_check_fix_state_by_segment(void);
#endif

/* to avoid build error */
extern unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(unsigned int cluster);


extern struct ppm_pwr_idx_ref_tbl_data ppm_get_pwr_idx_ref_tbl(void);
extern int *ppm_get_perf_idx_ref_tbl(enum ppm_cluster cluster);
extern void ppm_power_data_init(void);

/* COBRA algo */
extern void ppm_cobra_update_core_limit(unsigned int cluster, int limit);
extern void ppm_cobra_update_freq_limit(unsigned int cluster, int limit);
extern void ppm_cobra_update_limit(enum ppm_power_state new_state, void *user_req);
extern void ppm_cobra_init(void);
extern void ppm_cobra_dump_tbl(struct seq_file *m);


#ifdef __cplusplus
}
#endif

#endif

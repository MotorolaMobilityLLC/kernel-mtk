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


#ifndef __MT_PPM_PLATFORM_H__
#define __MT_PPM_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mach/mt_ppm_api.h"
#include "mt_cpufreq.h"	/* for IDVFS option */

/*==============================================================*/
/* Macros							*/
/*==============================================================*/
/* ppm driver update state to MET directly  0: turn off */
#define PPM_UPDATE_STATE_DIRECT_TO_MET  (1)

#define PPM_HEAVY_TASK_INDICATE_SUPPORT	(1)
#if PPM_HEAVY_TASK_INDICATE_SUPPORT
extern unsigned int hps_get_hvytsk(unsigned int cluster_id);
#endif

#ifdef ENABLE_IDVFS
#include "mt_ocp.h"
#define PPM_HW_OCP_SUPPORT		(1)
#define PPM_USE_OCP_DPM			(0)
#else
#define PPM_HW_OCP_SUPPORT		(0)
#endif

#define PPM_DLPT_ENHANCEMENT		(1)
#if PPM_DLPT_ENHANCEMENT
#define DYNAMIC_TABLE2REAL_PERCENTAGE	(58)
#endif

/* #define PPM_POWER_TABLE_CALIBRATION	(1) */
#ifdef PPM_POWER_TABLE_CALIBRATION
#include "mt_static_power.h"
#define BIG_LKG_EFUSE_TT		(169)
#define BIG_LKG_EFUSE_FF		(421)
#endif

#define PPM_THERMAL_ENHANCEMENT		(1)
#ifdef PPM_THERMAL_ENHANCEMENT
#define LITTLE_MIN_FREQ_IDX		(10)
#define BIG_MIN_FREQ_IDX		(12)
#define SKIP_ADVISE_BOUNDARY		(300)
#endif

#define PPM_OUTPUT_TRANS_LOG_TO_UART	(1)

#ifndef PPM_DISABLE_CLUSTER_MIGRATION
#define PPM_CLUSTER_MIGRATION_BOOST	(1)
#endif

#define PPM_DEFAULT_LIMIT_B_FREQ	(1)
#ifdef PPM_DEFAULT_LIMIT_B_FREQ
#define PPM_HICA_BIG_LIMIT_FREQ	(1500000)
#endif

#define PPM_DISABLE_BIG_FOR_LP_MODE	(1)

#define PPM_USE_EFFICIENCY_TABLE	(1)
#ifdef PPM_USE_EFFICIENCY_TABLE
#define PPM_EFFICIENCY_TABLE_USE_CORE_LIMIT	(1)
#ifdef PPM_EFFICIENCY_TABLE_USE_CORE_LIMIT
#define PPM_EFFICIENCY_TABLE_MAX_FREQ_IDX 8
#else
#define PPM_EFFICIENCY_TABLE_MAX_FREQ_IDX 7
#endif
#endif

/* DLPT mode */
#define PPM_DLPT_DEFAULT_MODE	(HYBRID_MODE)
#define DLPT_MAX_REAL_POWER_FY	(12504)
#define DLPT_MAX_REAL_POWER_SB	(18381)
#define DLPT_MAX_REAL_POWER_MB	(12504)	/* TBD */

#define	LCMOFF_MIN_FREQ		(598000)
#define	PTPOD_FREQ_IDX_FY	(7)
#define	PTPOD_FREQ_IDX_FY_B	(8)
#define	PTPOD_FREQ_IDX_SB	(9)
#define SUSPEND_FREQ_LL		(897000)
#define SUSPEND_FREQ_L		(1274000)
#define SUSPEND_FREQ_B		(1001000)
#define PWRTHRO_BAT_PER_MW	(600)
#define PWRTHRO_BAT_OC_MW	(600)
#define PWRTHRO_LOW_BAT_LV1_MW	(600)
#define PWRTHRO_LOW_BAT_LV2_MW	(600)

#define DVFS_OPP_NUM		(16)

#define PPM_DEFAULT_HOLD_TIME		(4)
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
	: (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_MB) ? DLPT_MAX_REAL_POWER_MB	\
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
	PPM_CLUSTER_B,

	NR_PPM_CLUSTERS,
};

/*==============================================================*/
/* Data Structures						*/
/*==============================================================*/
#ifdef PPM_POWER_TABLE_CALIBRATION
struct ppm_power_tbl {
	unsigned int index;
	struct {
		int opp_lv;
		unsigned int core_num;
	} cluster_cfg[NR_PPM_CLUSTERS];
	unsigned int perf_idx;
	unsigned int power_idx_TT;
	unsigned int power_idx_FF;
	unsigned int power_idx;
};

struct ppm_power_tbl_data {
	struct ppm_power_tbl *power_tbl;
	unsigned int nr_power_tbl;
};

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
#else
struct ppm_power_tbl {
	const unsigned int index;
	struct {
		int opp_lv;
		unsigned int core_num;
	} const cluster_cfg[NR_PPM_CLUSTERS];
	const unsigned int perf_idx;
	const unsigned int power_idx;
};

struct ppm_power_tbl_data {
	const struct ppm_power_tbl *power_tbl;
	const unsigned int nr_power_tbl;
};

struct ppm_pwr_idx_ref_tbl {
	const unsigned int core_dynamic_power[DVFS_OPP_NUM];
	const unsigned int core_total_power[DVFS_OPP_NUM];
	const unsigned int l2_power[DVFS_OPP_NUM];
};

struct ppm_pwr_idx_ref_tbl_data {
	const struct ppm_pwr_idx_ref_tbl *pwr_idx_ref_tbl;
	const unsigned int nr_pwr_idx_ref_tbl;
};
#endif

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/

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


#ifdef __cplusplus
}
#endif

#endif


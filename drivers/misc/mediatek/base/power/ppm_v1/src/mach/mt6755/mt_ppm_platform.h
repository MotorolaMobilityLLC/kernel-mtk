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

/* DLPT */
#define PPM_DLPT_DEFAULT_MODE	(SW_MODE)
#define DLPT_MAX_REAL_POWER_FY	(3890)
#define DLPT_MAX_REAL_POWER_SB	(4992)
#define	LCMOFF_MIN_FREQ		(598000)
#define	PTPOD_FREQ_IDX		(3)
#define SUSPEND_FREQ_LL		(689000)
#define SUSPEND_FREQ_L		(871000)
#define PWRTHRO_BAT_PER_MW	(610)
#define PWRTHRO_BAT_OC_MW	(610)
#define PWRTHRO_LOW_BAT_LV1_MW	(610)
#define PWRTHRO_LOW_BAT_LV2_MW	(610)

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

/*==============================================================*/
/* Data Structures						*/
/*==============================================================*/
struct ppm_power_tbl {
	const unsigned int index;
	struct {
		int opp_lv;
		unsigned int core_num;
	} const cluster_cfg[NR_CLUSTERS];
	const unsigned int perf_idx;
	const unsigned int power_idx;
};

struct ppm_power_tbl_data {
	const struct ppm_power_tbl *power_tbl;
	const unsigned int nr_power_tbl;
};

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
#ifdef PPM_IC_SEGMENT_CHECK
extern enum ppm_power_state ppm_check_fix_state_by_segment(void);
#endif

#ifdef __cplusplus
}
#endif

#endif




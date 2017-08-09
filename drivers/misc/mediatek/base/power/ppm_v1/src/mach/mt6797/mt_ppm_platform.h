
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
#define PPM_HW_OCP_SUPPORT		(1)

/* DLPT mode */
#define PPM_DLPT_DEFAULT_MODE	(HYBRID_MODE)
#define DLPT_MAX_REAL_POWER_FY	(3890)		/* TODO: check this */
#define DLPT_MAX_REAL_POWER_SB	(4992)		/* TODO: check this */
#define PPM_OCP_MAX_POWER_RATIO	(120 / 100)

#define	LCMOFF_MIN_FREQ		(598000)	/* TODO: check this */
#define	PTPOD_FREQ_IDX		(3)		/* TODO: check this */
#define SUSPEND_FREQ_IDX_LL	(3)		/* TODO: check this */
#define SUSPEND_FREQ_IDX_L	(5)		/* TODO: check this */
#define PWRTHRO_BAT_PER_MW	(600)
#define PWRTHRO_BAT_OC_MW	(600)
#define PWRTHRO_LOW_BAT_LV1_MW	(600)
#define PWRTHRO_LOW_BAT_LV2_MW	(600)

#define DVFS_OPP_NUM		(8)

#define PPM_DEFAULT_HOLD_TIME		(4)
#define PPM_DEFAULT_FREQ_HOLD_TIME	(4)
#define PPM_DEFAULT_DELTA		(20)
#define PPM_LOADING_UPPER		(400)
#define PPM_TLP_CRITERIA		(400)

#define get_cluster_lcmoff_min_freq(id)		LCMOFF_MIN_FREQ	/* the same for each cluster */
#define get_cluster_ptpod_fix_freq_idx(id)	PTPOD_FREQ_IDX	/* the same for each cluster */
#define get_cluster_suspend_fix_freq_idx(id)	\
	((id == 0) ? SUSPEND_FREQ_IDX_LL : SUSPEND_FREQ_IDX_L)

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

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
unsigned int ppm_set_ocp(unsigned int limited_power);

#ifdef __cplusplus
}
#endif

#endif




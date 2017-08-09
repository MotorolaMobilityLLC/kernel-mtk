
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
#else
#define PPM_HW_OCP_SUPPORT		(0)
#endif
#define PPM_DLPT_ENHANCEMENT		(1)
#if PPM_DLPT_ENHANCEMENT
#define DYNAMIC_TABLE2REAL_PERCENTAGE	(58)
#endif

/* DLPT mode */
#define PPM_DLPT_DEFAULT_MODE	(HYBRID_MODE)
#define DLPT_MAX_REAL_POWER_FY	(12504)
#define DLPT_MAX_REAL_POWER_SB	(18381)

#define	LCMOFF_MIN_FREQ		(598000)	/* TODO: check this */
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

struct ppm_pwr_idx_ref_tbl {
	const unsigned int core_dynamic_power[DVFS_OPP_NUM];
	const unsigned int core_total_power[DVFS_OPP_NUM];
	const unsigned int l2_power[DVFS_OPP_NUM];
};

struct ppm_pwr_idx_ref_tbl_data {
	const struct ppm_pwr_idx_ref_tbl *pwr_idx_ref_tbl;
	const unsigned int nr_pwr_idx_ref_tbl;
};

/*==============================================================*/
/* Global Variables						*/
/*==============================================================*/

/*==============================================================*/
/* APIs								*/
/*==============================================================*/
extern unsigned int ppm_set_ocp(unsigned int limited_power, unsigned int percentage);
#if PPM_DLPT_ENHANCEMENT
extern unsigned int ppm_calc_total_power(struct ppm_cluster_status *cluster_status,
					unsigned int cluster_num, unsigned int percentage);
#endif

#ifdef __cplusplus
}
#endif

#endif


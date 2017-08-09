#ifndef _MT_VCORE_DVFS_3_
#define _MT_VCORE_DVFS_3_

#include <linux/kernel.h>

/*
 * Config and Parameter
 */
#define VCORE_1_P_25_UV		1250000
#define VCORE_1_P_15_UV		1150000

#define FDDR_S0_KHZ		1466000
#define FDDR_S1_KHZ		1313000

#if 0 /* SPEC_NO_FREQ */
/* CLK MUX */
#define FAXI_S0_KHZ		218000
#define FAXI_S1_KHZ		136000

/* PLL hopping */
#define FVENC_S0_KHZ		300000
#define FVENC_S1_KHZ		182000

/* CLK MUX */
#define FQTRHALF_S0_KHZ		54500		/* Faxi/4 */
#define FQTRHALF_S1_KHZ		68000		/* Faxi/2 */
#endif

/* Vcore 1.15 <=> trans1 <=> trans2 <=> Vcore 1.25 (SPM control) */
enum vcore_trans {
	TRANS1,
	TRANS2,
	NUM_TRANS
};

enum dvfs_kicker {
	/* SPEC define kicker */
	KIR_GPU,		/* 0 */
	KIR_MM,			/* 1 */
	KIR_EMIBW,		/* 2 */
	KIR_SDIO,		/* 3 */
	KIR_SYSFS,		/* 4 */
	NUM_KICKER,		/* 5 */

	/* internal kicker */
	KIR_SDIO_AUTOK,		/* 6 */
	KIR_LATE_INIT		/* 7 */
};

enum dvfs_opp {
	OPP_OFF = -1,
	OPP_0 = 0,		/* 0: Vcore 1.25, DDR 1466 or 1600 */
	OPP_1,			/* 1: Vcore 1.15, DDR 1313 */
	NUM_OPP
};

#define OPPI_PERF		OPP_0
#define OPPI_LOW_PWR		OPP_1
#define OPPI_UNREQ		OPP_OFF

enum dvfs_error {
	FAIL,			/* 0 */
	ERR_FEATURE_DISABLE,	/* 1 */
	ERR_SDIO_AUTOK,		/* 2 */
	ERR_OPP,		/* 3 */
	ERR_KICKER,		/* 4 */
	ERR_NO_CHANGE,		/* 5 */
	ERR_VCORE_DVS,		/* 6 */
	ERR_DDR_DFS,		/* 7 */
	ERR_VENCPLL_FH,		/* 8 */
	ERR_LATE_INIT_OPP	/* 9 */
};

/* for GPU, MM, EMIBW, SDIO, USB, SYSFS */
extern int vcorefs_request_dvfs_opp(enum dvfs_kicker kicker, enum dvfs_opp opp);

/* for SDIO autoK */
extern int vcorefs_sdio_lock_dvfs(bool is_online_tuning);
extern unsigned int vcorefs_sdio_get_vcore_nml(void);
extern int vcorefs_sdio_set_vcore_nml(unsigned int vcore_uv);
extern int vcorefs_sdio_unlock_dvfs(bool is_online_tuning);
extern bool vcorefs_sdio_need_multi_autok(void);

/* for External Control Function */
extern void vcorefs_list_kicker_request(void);
extern unsigned int vcorefs_get_curr_voltage(void);
extern int get_ddr_khz(void);
extern int get_ddr_khz_by_steps(unsigned int steps);
extern bool is_vcorefs_can_work(void);

/*
 * Macro and Inline
 */
#define VCORE_BASE_UV		600000
#define VCORE_STEP_UV		6250

#define VCORE_INVALID		0x80

#define vcore_uv_to_pmic(uv)	/* pmic >= uv */	\
	((((uv) - VCORE_BASE_UV) + (VCORE_STEP_UV - 1)) / VCORE_STEP_UV)

#define vcore_pmic_to_uv(pmic)	\
	(((pmic) * VCORE_STEP_UV) + VCORE_BASE_UV)

#endif

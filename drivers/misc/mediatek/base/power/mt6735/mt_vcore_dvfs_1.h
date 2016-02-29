#ifndef _MT_VCORE_DVFS_1_
#define _MT_VCORE_DVFS_1_

#include <linux/kernel.h>

/*
 * Config and Parameter
 */
#define VCORE_1_P_15_UV         1150000
#define VCORE_1_P_05_UV         1050000

#define FDDR_S0_KHZ             1280000		/* 6737: 1466000 */
#define FDDR_S1_KHZ             938000		/* 6737: 1313000 */

/* CLK MUX */
#define FAXI_S0_KHZ             218000
#define FAXI_S1_KHZ             136000

/* PLL hopping */
#define FVENC_S0_KHZ            300000
#define FVENC_S1_KHZ            166000

/* CLK MUX */
#define FQTRHALF_S0_KHZ         54500           /* Faxi/4 */
#define FQTRHALF_S1_KHZ         68000           /* Faxi/2 */

#define PMIC_VCORE_PDN_VOSEL_ON 0x618

/* Vcore 1.05 <=> trans1 <=> trans2 <=> Vcore 1.15 (SPM control) */
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
	KIR_USB,		/* 4 */
	KIR_SYSFS,		/* 5 */
	NUM_KICKER,		/* 6 */
	/* internal kicker */
	KIR_SDIO_AUTOK,		/* 7 */
	KIR_LATE_INIT		/* 8 */
};

enum dvfs_opp {
	OPP_OFF = -1,
	OPP_0 = 0,			/* 0: Vcore 1.15, DDR 1280 (6737: Vcore 1.15, DDR 1466) */
	OPP_1,				/* 1: Vcore 1.05, DDR 938  (6737: Vcore 1.05, DDR 1313) */
	NUM_OPP
};

#define OPPI_PERF		OPP_0
#define OPPI_LOW_PWR		OPP_1
#define OPPI_UNREQ		OPP_OFF

enum dvfs_error {
	PASS,			/* 0 */
	FAIL,			/* 1 */
	ERR_FEATURE_DISABLE,	/* 2 */
	ERR_SDIO_AUTOK,		/* 3 */
	ERR_OPP,		/* 4 */
	ERR_KICKER,		/* 5 */
	ERR_NO_CHANGE,		/* 6 */
	ERR_VCORE_DVS,		/* 7 */
	ERR_DDR_DFS,		/* 8 */
	ERR_VENCPLL_FH,		/* 9 */
	ERR_LATE_INIT_OPP	/* 10 */
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
extern unsigned int vcorefs_get_curr_voltage(void);
extern unsigned int get_ddr_khz(void);
extern unsigned int get_ddr_khz_by_steps(unsigned int step);
extern bool is_vcorefs_can_work(void);

/*
 * Macro and Inline
 */
#define VCORE_BASE_UV           600000
#define VCORE_STEP_UV           6250

#define VCORE_INVALID		0x80

#define vcore_uv_to_pmic(uv)	/* pmic >= uv */	\
	((((uv) - VCORE_BASE_UV) + (VCORE_STEP_UV - 1)) / VCORE_STEP_UV)

#define vcore_pmic_to_uv(pmic)	\
	(((pmic) * VCORE_STEP_UV) + VCORE_BASE_UV)

#endif


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

#define DEBUG 1
/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/suspend.h>
#include <linux/topology.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/mtk_devinfo.h>
#include <mt-plat/aee.h>

/* project includes */
#include <mt-plat/upmu_common.h>
#include <mt6311.h>
#include <mach/mtk_freqhopping.h>
#include <mach/mtk_ppm_api.h>
#include <mach/mtk_pbm.h>
#include <mach/mtk_thermal.h>
#include "mtk_static_power.h"

/* local includes */
#include "mtk_cpufreq.h"
#include "mtk_cpufreq_hybrid.h"

#define DCM_ENABLE	1
#ifdef DCM_ENABLE
#include "mtk_dcm.h"
#endif

/*=============================================================*/
/* Macro definition                                            */
/*=============================================================*/
static unsigned long apmixed_base	= 0x1000c000;
static unsigned long infracfg_ao_base	= 0x10001000;
static unsigned long topckgen_base	= 0x10000000;

#define APMIXED_NODE		"mediatek,apmixed"
#define INFRACFG_AO_NODE	"mediatek,infracfg_ao"
#define TOPCKGEN_NODE		"mediatek,topckgen"

#define ARMPLL_LL_CON1		(apmixed_base + 0x204)
#define ARMPLL_L_CON1		(apmixed_base + 0x214)
#define CCIPLL_CON1		(apmixed_base + 0x294)

#define TOP_CKMUXSEL		(infracfg_ao_base + 0x000)
#define TOPCKGEN_CKDIV1_BIG	(infracfg_ao_base + 0x024)
#define TOPCKGEN_CKDIV1_SML	(infracfg_ao_base + 0x028)
#define TOPCKGEN_CKDIV1_BUS	(infracfg_ao_base + 0x02c)

#define CLK_MISC_CFG_0		(topckgen_base + 0x104)

/*
 * CONFIG
 */
#ifdef CONFIG_HYBRID_CPU_DVFS
/**
 * 1: get voltage from DVFSP for performance
 * 2: get voltage from PMIC through I2C
 */
#ifdef __TRIAL_RUN__
static u32 enable_cpuhvfs = 2;
#else
static u32 enable_cpuhvfs = 1;
#endif

#define HWGOV_EN_FORCE		(1U << 0)
#define HWGOV_EN_GAME		(1U << 1)	/* 0x2 */
#define HWGOV_DIS_FORCE		(1U << 16)
#define HWGOV_DIS_GAME		(1U << 17)	/* 0x20000 */
#define HWGOV_EN_MASK		(0xffff << 0)

static u32 hwgov_en_map;

#ifdef CPUHVFS_HW_GOVERNOR
static u32 enable_hw_gov = 1;
#else
static u32 enable_hw_gov;
#endif

#endif	/* CONFIG_HYBRID_CPU_DVFS */

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) >= (b) ? (b) : (a))

#include <linux/time.h>
static ktime_t now[NR_SET_V_F];
static ktime_t delta[NR_SET_V_F];
static ktime_t max[NR_SET_V_F];

/* used @ set_cur_volt_extBuck() */
#define NORMAL_DIFF_VRSAM_VPROC		10000
#define MAX_DIFF_VSRAM_VPROC		30000
#define MIN_VSRAM_VOLT			85000
#define MAX_VSRAM_VOLT_FY		105000
#define MAX_VSRAM_VOLT_FY2		111875
#define MAX_VSRAM_VOLT_FY3		111875

static unsigned int max_vsram_volt = MAX_VSRAM_VOLT_FY;

/* PMIC/PLL settle time (us) */
#define PMIC_CMD_DELAY_TIME	5
#define MIN_PMIC_SETTLE_TIME	25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt) \
	(((new_volt) - (old_volt) + 1250 - 1) / 1250 + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt) \
	(((old_volt) - (new_volt)) * 2 / 625 + PMIC_CMD_DELAY_TIME)

#define PLL_SETTLE_TIME		20
#define POS_SETTLE_TIME		2

/* for DVFS OPP table LL/FY */
#define CPU_DVFS_FREQ0_LL_FY		1638000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1560000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1508000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1443000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1378000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1300000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1235000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1183000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1066000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		 949000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		 806000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		 676000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		 559000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		 442000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		 377000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		 221000		/* KHz */

/* for DVFS OPP table L/FY */
#define CPU_DVFS_FREQ0_L_FY		2340000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY		2262000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY		2197000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY		2132000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY		2067000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY		1963000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY		1872000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY		1794000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY		1638000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY		1456000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY		1248000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY		1040000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY		 897000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY		 754000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY		 533000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY		 442000		/* KHz */

/* for DVFS OPP table CCI/FY */
#define CPU_DVFS_FREQ0_CCI_FY		 988000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		 949000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		 910000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		 858000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		 819000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		 767000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		 741000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		 702000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		 650000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		 572000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		 494000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		 416000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		 351000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		 286000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		 234000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY	102500		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 99375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 96875		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 94375		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 91875		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 88750		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 86875		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 81250		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 77500		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 73750		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 70000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 67500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 65000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 62500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 60000		/* 10uV */

/* for DVFS OPP table LL/FY2 */
#define CPU_DVFS_FREQ0_LL_FY2		1638000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY2		1638000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY2		1560000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY2		1508000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY2		1443000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY2		1378000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY2		1300000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY2		1183000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY2		1066000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY2		 936000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY2		 806000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY2		 663000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY2		 559000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY2		 455000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY2		 390000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY2		 247000		/* KHz */

/* for DVFS OPP table L/FY2 */
#define CPU_DVFS_FREQ0_L_FY2		2340000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY2		2288000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY2		2236000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY2		2184000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY2		2132000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY2		2080000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY2		1976000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY2		1820000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY2		1664000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY2		1469000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY2		1261000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY2		1040000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY2		 910000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY2		 767000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY2		 572000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY2		 494000		/* KHz */

/* for DVFS OPP table CCI/FY2 */
#define CPU_DVFS_FREQ0_CCI_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY2		 988000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY2		 949000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY2		 910000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY2		 858000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY2		 819000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY2		 767000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY2		 702000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY2		 650000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY2		 572000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY2		 494000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY2		 416000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY2		 351000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY2		 286000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY2		 234000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY2		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY2	107500		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY2	102500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY2	 99375		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY2	 96875		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY2	 94375		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY2	 91875		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY2	 88750		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY2	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY2	 81250		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY2	 77500		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY2	 73750		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY2	 70000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY2	 67500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY2	 65000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY2	 62500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY2	 60000		/* 10uV */

/* for DVFS OPP table LL/FY3 */
#define CPU_DVFS_FREQ0_LL_FY3		1690000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY3		1664000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY3		1638000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY3		1560000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY3		1508000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY3		1378000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY3		1300000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY3		1183000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY3		1066000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY3		 936000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY3		 806000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY3		 663000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY3		 559000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY3		 455000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY3		 390000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY3		 247000		/* KHz */

/* for DVFS OPP table L/FY3 */
#define CPU_DVFS_FREQ0_L_FY3		2392000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY3		2340000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY3		2288000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY3		2236000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY3		2184000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY3		2080000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY3		1976000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY3		1820000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY3		1664000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY3		1469000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY3		1261000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY3		1040000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY3		 910000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY3		 767000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY3		 572000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY3		 494000		/* KHz */

/* for DVFS OPP table CCI/FY3 */
#define CPU_DVFS_FREQ0_CCI_FY3		1040000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY3		1014000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY3		 988000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY3		 949000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY3		 910000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY3		 819000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY3		 767000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY3		 702000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY3		 650000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY3		 572000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY3		 494000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY3		 416000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY3		 351000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY3		 286000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY3		 234000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY3		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY3	111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY3	107500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY3	102500		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY3	 99375		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY3	 96875		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY3	 91875		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY3	 88750		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY3	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY3	 81250		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY3	 77500		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY3	 73750		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY3	 70000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY3	 67500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY3	 65000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY3	 62500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY3	 60000		/* 10uV */

#define CPUFREQ_LAST_FREQ_LEVEL		CPU_DVFS_FREQ15_CCI_FY

/* LL 676M */
#define LL_1CORE_FLOOR_IDX	11

/*#define L_FLOOR_TRACK_LL	1*/

/*#define CCI_HALF_MAX_FREQ	1*/


/* Debugging */
#undef TAG
#define TAG     "[Power/cpufreq] "

#define cpufreq_err(fmt, args...)		\
	pr_err(TAG"[ERROR]"fmt, ##args)
#define cpufreq_warn(fmt, args...)		\
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define cpufreq_info(fmt, args...)		\
	pr_warn(TAG""fmt, ##args)
#define cpufreq_dbg(fmt, args...)		\
	pr_debug(TAG""fmt, ##args)
#define cpufreq_ver(fmt, args...)		\
	do {					\
		if (func_lv_mask)		\
			cpufreq_info(TAG""fmt, ##args);	\
	} while (0)

#define GEN_DB_ON(condition, fmt, args...)			\
({								\
	int _r = !!(condition);					\
	if (unlikely(_r))					\
		aee_kernel_exception("CPUDVFS", fmt, ##args);	\
	unlikely(_r);						\
})

static unsigned int func_lv_mask;
static unsigned int do_dvfs_stress_test;
static unsigned int dvfs_power_mode;

enum ppb_power_mode {
	DEFAULT_MODE,		/* normal mode */
	LOW_POWER_MODE,
	JUST_MAKE_MODE,
	PERFORMANCE_MODE,	/* sports mode */
	NUM_PPB_POWER_MODE
};

static const char *power_mode_str[NUM_PPB_POWER_MODE] = {
	"Default(Normal) mode",
	"Low Power mode",
	"Just Make mode",
	"Performance(Sports) mode"
};

static unsigned int ppb_hispeed_freq[NR_MT_CPU_DVFS][NUM_PPB_POWER_MODE] = {
	[MT_CPU_DVFS_LL][DEFAULT_MODE]		= 8,	/* OPP index */
	[MT_CPU_DVFS_LL][LOW_POWER_MODE]	= 12,
	[MT_CPU_DVFS_LL][JUST_MAKE_MODE]	= 10,
	[MT_CPU_DVFS_LL][PERFORMANCE_MODE]	= 0,

	[MT_CPU_DVFS_L][DEFAULT_MODE]		= 6,
	[MT_CPU_DVFS_L][LOW_POWER_MODE]		= 10,
	[MT_CPU_DVFS_L][JUST_MAKE_MODE]		= 8,
	[MT_CPU_DVFS_L][PERFORMANCE_MODE]	= 0,
};

/*
 * BIT Operation
 */
#define _BIT_(_bit_)                    (unsigned)(1 << (_bit_))
#define _BITS_(_bits_, _val_) \
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define _BITMASK_(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

/*
 * REG ACCESS
 */
#define cpufreq_read(addr)                  __raw_readl(IOMEM(addr))
#define cpufreq_write(addr, val)            mt_reg_sync_writel((val), ((void *)addr))
#define cpufreq_write_mask(addr, mask, val) \
cpufreq_write(addr, (cpufreq_read(addr) & ~(_BITMASK_(mask))) | _BITS_(mask, val))

/*
 * LOCK
 */
static DEFINE_MUTEX(cpufreq_mutex);
#define cpufreq_lock()		mutex_lock(&cpufreq_mutex)
#define cpufreq_unlock()	mutex_unlock(&cpufreq_mutex)

/*
 * EFUSE
 */
static int dvfs_disable_flag;

enum cpu_level {
	CPU_LEVEL_0,
	CPU_LEVEL_1,
	CPU_LEVEL_2,
	NUM_CPU_LEVEL
};

static unsigned int _mt_cpufreq_get_cpu_level(void)
{
	unsigned int lv = CPU_LEVEL_0;
	u32 segment = get_devinfo_with_index(30);

	cpufreq_ver("from efuse: segment = 0x%x\n", segment);

	switch (_GET_BITS_VAL_(7:5, segment)) {
	case 0:
		lv = CPU_LEVEL_0;
		break;
	case 1:
		lv = CPU_LEVEL_1;
		break;
	case 3:
	case 7:
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
		lv = CPU_LEVEL_2;
#else
		lv = CPU_LEVEL_1;
#endif
		break;
	}

#if 0
	/* get CPU clock-frequency from DT */
	{
		struct device_node *node = of_find_node_by_path("/cpus/cpu@100");
		unsigned int cpu_speed = 0;

		if (!of_property_read_u32(node, "clock-frequency", &cpu_speed))
			cpu_speed = cpu_speed / 1000 / 1000;	/* MHz */
		else {
			cpufreq_err
			    ("@%s: missing clock-frequency property, use default CPU level\n",
			     __func__);
			return CPU_LEVEL_0;
		}

		cpufreq_ver("CPU clock-frequency from DT = %d MHz\n", cpu_speed);
	}
#endif

	return lv;
}


/*
 * AEE (SRAM debug)
 */
enum cpu_dvfs_state {
	CPU_DVFS_LL_IS_DOING_DVFS = 0,
	CPU_DVFS_L_IS_DOING_DVFS,
	CPU_DVFS_CCI_IS_DOING_DVFS,
};

static void _mt_cpufreq_aee_init(void)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_vproc_big(0x0);
	aee_rr_rec_cpu_dvfs_vproc_little(0x0);
	aee_rr_rec_cpu_dvfs_oppidx(0x0);
	aee_rr_rec_cpu_dvfs_cci_oppidx(0x0);
	aee_rr_rec_cpu_dvfs_status(0x0);
	aee_rr_rec_cpu_dvfs_step(0x0);
	aee_rr_rec_cpu_dvfs_pbm_step(0x0);
	aee_rr_rec_cpu_dvfs_cb(0x0);
	aee_rr_rec_cpufreq_cb(0x0);
#endif
}

/*
 * PMIC_WRAP
 */
/* for Vsram */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define VOLT_TO_PMIC_VAL(volt)		(((volt) - 51875 + 625 - 1) / 625)
#define PMIC_VAL_TO_VOLT(val)		((val) * 625 + 51875)
#else
#define VOLT_TO_PMIC_VAL(volt)		(((volt) - 60000 + 625 - 1) / 625)
#define PMIC_VAL_TO_VOLT(val)		((val) * 625 + 60000)
#endif

/* for Vproc */
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define VOLT_TO_EXTBUCK_VAL(volt)	(((volt) - 40625 + 625 - 1) / 625)
#define EXTBUCK_VAL_TO_VOLT(val)	((val) * 625 + 40625)
#else
#define VOLT_TO_EXTBUCK_VAL(volt)	(((volt) - 60000 + 625 - 1) / 625)
#define EXTBUCK_VAL_TO_VOLT(val)	((val) * 625 + 60000)
#endif


/* cpu voltage sampler */
static cpuVoltsampler_func g_pCpuVoltSampler;

void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB)
{
	g_pCpuVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_setvolt_registerCB);

/*
 * mt_cpufreq driver
 */
#define FP(khz, pos, clk) {	\
	.target_f = khz,		\
	.vco_dds = khz*pos*clk,		\
	.pos_div = pos,			\
	.clk_div = clk,			\
}
struct mt_cpu_freq_method {
	const unsigned int target_f;
	const unsigned int vco_dds;
	const unsigned int pos_div;
	const unsigned int clk_div;
};

struct opp_idx_tbl {
	struct mt_cpu_dvfs *p;
	struct mt_cpu_freq_method *slot;
};

enum opp_idx_type {
	CUR_OPP_IDX = 0,
	TARGET_OPP_IDX = 1,

	NR_OPP_IDX,
};
static struct opp_idx_tbl opp_tbl_m[NR_OPP_IDX];

#define OP(khz, volt) {            \
	.cpufreq_khz = khz,             \
	.cpufreq_volt = volt,           \
	.cpufreq_volt_org = volt,       \
}

struct mt_cpu_freq_info {
	const unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;
	const unsigned int cpufreq_volt_org;
};

struct mt_cpu_dvfs {
	const char *name;
	const enum mt_cpu_dvfs_id id;
	unsigned int cpu_id;			/* for cpufreq */
	unsigned int cpu_level;
	struct mt_cpu_dvfs_ops *ops;
	struct cpufreq_policy *mt_policy;
	unsigned int *armpll_addr;		/* for armpll clock address */
	unsigned int *ckdiv_addr;		/* for clock divider address */
	int hopping_id;				/* for frequency hopping */
	struct mt_cpu_freq_method *freq_tbl;	/* Frequency table */
	struct mt_cpu_freq_info *opp_tbl;	/* OPP table */
	int nr_opp_tbl;				/* size for OPP table */
	int idx_opp_tbl;			/* current OPP idx */
	int idx_opp_ppm_base;			/* ppm update base */
	int idx_opp_ppm_limit;			/* ppm update limit */
	int armpll_is_available;		/* For CCI clock switch flag */
	int idx_normal_max_opp;			/* idx for normal max OPP */

	struct cpufreq_frequency_table *freq_tbl_for_cpufreq;	/* freq table for cpufreq */

	/* enable/disable DVFS function */
	bool dvfs_disable_by_suspend;
	bool dvfs_disable_by_procfs;
};

struct mt_cpu_dvfs_ops {
	/* for freq change (PLL/MUX) */
	unsigned int (*get_cur_phy_freq)(struct mt_cpu_dvfs *p);	/* return (physical) freq (KHz) */
	void (*set_cur_freq)(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz);	/* set freq  */

	/* for vproc change (extBuck) */
	unsigned int (*get_cur_volt)(struct mt_cpu_dvfs *p);	/* return volt (mV * 100) */
	int (*set_cur_volt)(struct mt_cpu_dvfs *p, unsigned int volt);	/* set volt (mv * 100) */

	/* for vsram change (LDO) */
	unsigned int (*get_cur_vsram)(struct mt_cpu_dvfs *p);	/* return volt (mV * 100) */
	int (*set_cur_vsram)(struct mt_cpu_dvfs *p, unsigned int volt);	/* set volt (mv * 100) */
	int (*set_sync_dcm)(unsigned int mhz);
};

/* for freq change (PLL/MUX) */
static unsigned int get_cur_phy_freq(struct mt_cpu_dvfs *p);
static void set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz);

/* for volt change (PMICWRAP/extBuck) */
static unsigned int get_cur_volt_extbuck(struct mt_cpu_dvfs *p);
static int set_cur_volt_extbuck(struct mt_cpu_dvfs *p, unsigned int volt);	/* volt: mv * 100 */
static unsigned int get_cur_volt_sram(struct mt_cpu_dvfs *p);
static int set_cur_volt_sram(struct mt_cpu_dvfs *p, unsigned int volt);		/* volt: mv * 100 */

/* CPU callback */
static int _mt_cpufreq_cpu_CB(struct notifier_block *nfb, unsigned long action, void *hcpu);

static struct mt_cpu_dvfs_ops dvfs_ops_LL = {
	.get_cur_phy_freq = get_cur_phy_freq,
	.set_cur_freq = set_cur_freq,
	.get_cur_volt = get_cur_volt_extbuck,
	.set_cur_volt = set_cur_volt_extbuck,
	.get_cur_vsram = get_cur_volt_sram,
	.set_cur_vsram = set_cur_volt_sram,
#ifdef DCM_ENABLE
	.set_sync_dcm = sync_dcm_set_mp0_freq,
#endif
};

static struct mt_cpu_dvfs_ops dvfs_ops_L = {
	.get_cur_phy_freq = get_cur_phy_freq,
	.set_cur_freq = set_cur_freq,
	.get_cur_volt = get_cur_volt_extbuck,
	.set_cur_volt = set_cur_volt_extbuck,
	.get_cur_vsram = get_cur_volt_sram,
	.set_cur_vsram = set_cur_volt_sram,
#ifdef DCM_ENABLE
	.set_sync_dcm = sync_dcm_set_mp1_freq,
#endif
};

static struct mt_cpu_dvfs_ops dvfs_ops_CCI = {
	.get_cur_phy_freq = get_cur_phy_freq,
	.set_cur_freq = set_cur_freq,
	.get_cur_volt = get_cur_volt_extbuck,
	.set_cur_volt = set_cur_volt_extbuck,
	.get_cur_vsram = get_cur_volt_sram,
	.set_cur_vsram = set_cur_volt_sram,
#ifdef DCM_ENABLE
	.set_sync_dcm = sync_dcm_set_cci_freq,
#endif
};

static struct mt_cpu_dvfs cpu_dvfs[] = {
	[MT_CPU_DVFS_LL] = {
				.name = __stringify(MT_CPU_DVFS_LL),
				.id = MT_CPU_DVFS_LL,
				.cpu_id = 0,
				.cpu_level = CPU_LEVEL_0,
				.idx_normal_max_opp = -1,
				.idx_opp_ppm_base = -1,
				.idx_opp_ppm_limit = -1,
				.ops = &dvfs_ops_LL,
				.hopping_id = FH_ARMPLL1_PLLID,
				},

	[MT_CPU_DVFS_L] = {
				.name = __stringify(MT_CPU_DVFS_L),
				.id = MT_CPU_DVFS_L,
				.cpu_id = 4,
				.cpu_level = CPU_LEVEL_0,
				.idx_normal_max_opp = -1,
				.idx_opp_ppm_base = -1,
				.idx_opp_ppm_limit = -1,
				.ops = &dvfs_ops_L,
				.hopping_id = FH_ARMPLL2_PLLID,
				},

	[MT_CPU_DVFS_CCI] = {
				.name = __stringify(MT_CPU_DVFS_CCI),
				.id = MT_CPU_DVFS_CCI,
				.cpu_id = 8,
				.cpu_level = CPU_LEVEL_0,
				.idx_normal_max_opp = -1,
				.idx_opp_ppm_base = -1,
				.idx_opp_ppm_limit = -1,
				.ops = &dvfs_ops_CCI,
				.hopping_id = FH_CCI_PLLID,
				},
};

#define for_each_cpu_dvfs(i, p)			for (i = 0, p = cpu_dvfs; i < NR_MT_CPU_DVFS; i++, p = &cpu_dvfs[i])
#define for_each_cpu_dvfs_only(i, p)	\
	for (i = 0, p = cpu_dvfs; (i < NR_MT_CPU_DVFS) && (i != MT_CPU_DVFS_CCI); i++, p = &cpu_dvfs[i])
#define cpu_dvfs_is(p, id)			(p == &cpu_dvfs[id])
#define cpu_dvfs_is_available(p)		(p->opp_tbl)
#define cpu_dvfs_get_name(p)			(p->name)

#define cpu_dvfs_get_cur_freq(p)		(p->opp_tbl[p->idx_opp_tbl].cpufreq_khz)
#define cpu_dvfs_get_freq_by_idx(p, idx)	(p->opp_tbl[idx].cpufreq_khz)
#define cpu_dvfs_get_freq_mhz(p, idx)		(p->opp_tbl[idx].cpufreq_khz / 1000)

#define cpu_dvfs_get_max_freq(p)		(p->opp_tbl[0].cpufreq_khz)
#define cpu_dvfs_get_normal_max_freq(p)		(p->opp_tbl[p->idx_normal_max_opp].cpufreq_khz)
#define cpu_dvfs_get_min_freq(p)		(p->opp_tbl[p->nr_opp_tbl - 1].cpufreq_khz)

#define cpu_dvfs_get_cur_volt(p)		(p->opp_tbl[p->idx_opp_tbl].cpufreq_volt)
#define cpu_dvfs_get_volt_by_idx(p, idx)	(p->opp_tbl[idx].cpufreq_volt)
#define cpu_dvfs_get_org_volt_by_idx(p, idx)	(p->opp_tbl[idx].cpufreq_volt_org)

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define cpu_dvfs_is_extbuck_valid()		(1)
#else
#define cpu_dvfs_is_extbuck_valid()		(is_ext_buck_exist() && is_ext_buck_sw_ready())
#endif

static struct mt_cpu_dvfs *id_to_cpu_dvfs(enum mt_cpu_dvfs_id id)
{
	return (id < NR_MT_CPU_DVFS) ? &cpu_dvfs[id] : NULL;
}

#ifdef CONFIG_HYBRID_CPU_DVFS
static inline unsigned int id_to_cluster(enum mt_cpu_dvfs_id id)
{
	if (id == MT_CPU_DVFS_LL)
		return CPU_CLUSTER_LL;
	if (id == MT_CPU_DVFS_L)
		return CPU_CLUSTER_L;
	if (id == MT_CPU_DVFS_CCI)
		return CPU_CLUSTER_CCI;

	WARN_ON(1);
	return NUM_CPU_CLUSTER;
}

static inline unsigned int cpu_dvfs_to_cluster(struct mt_cpu_dvfs *p)
{
	if (p == &cpu_dvfs[MT_CPU_DVFS_LL])
		return CPU_CLUSTER_LL;
	if (p == &cpu_dvfs[MT_CPU_DVFS_L])
		return CPU_CLUSTER_L;
	if (p == &cpu_dvfs[MT_CPU_DVFS_CCI])
		return CPU_CLUSTER_CCI;

	WARN_ON(1);
	return NUM_CPU_CLUSTER;
}

static inline struct mt_cpu_dvfs *cluster_to_cpu_dvfs(unsigned int cluster)
{
	if (cluster == CPU_CLUSTER_LL)
		return &cpu_dvfs[MT_CPU_DVFS_LL];
	if (cluster == CPU_CLUSTER_L)
		return &cpu_dvfs[MT_CPU_DVFS_L];
	if (cluster == CPU_CLUSTER_CCI)
		return &cpu_dvfs[MT_CPU_DVFS_CCI];

	WARN_ON(1);
	return NULL;
}
#endif

static void aee_record_cpu_dvfs_in(enum mt_cpu_dvfs_id id)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	if (id == MT_CPU_DVFS_LL)
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() |
					   (1 << CPU_DVFS_LL_IS_DOING_DVFS));
	else if (id == MT_CPU_DVFS_L)
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() |
					   (1 << CPU_DVFS_L_IS_DOING_DVFS));
	else	/* CCI */
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() |
					   (1 << CPU_DVFS_CCI_IS_DOING_DVFS));
#endif
}

static void aee_record_cpu_dvfs_out(enum mt_cpu_dvfs_id id)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	if (id == MT_CPU_DVFS_LL)
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() &
					   ~(1 << CPU_DVFS_LL_IS_DOING_DVFS));
	else if (id == MT_CPU_DVFS_L)
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() &
					   ~(1 << CPU_DVFS_L_IS_DOING_DVFS));
	else	/* CCI */
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() &
					   ~(1 << CPU_DVFS_CCI_IS_DOING_DVFS));
#endif
}

static void aee_record_cpu_dvfs_step(unsigned int step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_step((aee_rr_curr_cpu_dvfs_step() & 0xF0) | step);
#endif
}

static void aee_record_cpu_dvfs_pbm_step(unsigned int step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_pbm_step(step);
#endif
}

static void aee_record_cci_dvfs_step(unsigned int step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_step((aee_rr_curr_cpu_dvfs_step() & 0x0F) | (step << 4));
#endif
}

static void aee_record_cpu_dvfs_cb(unsigned int step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_cb(step);
#endif
}

void aee_record_cpufreq_cb(unsigned int step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpufreq_cb(step);
#endif
}

static void aee_record_cpu_volt(struct mt_cpu_dvfs *p, unsigned int volt)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_cpu_dvfs_vproc_little(VOLT_TO_EXTBUCK_VAL(volt));
#endif
}

static void aee_record_freq_idx(struct mt_cpu_dvfs *p, int idx)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	if (cpu_dvfs_is(p, MT_CPU_DVFS_LL))
		aee_rr_rec_cpu_dvfs_oppidx((aee_rr_curr_cpu_dvfs_oppidx() & 0xF0) | idx);
	else if (cpu_dvfs_is(p, MT_CPU_DVFS_L))
		aee_rr_rec_cpu_dvfs_oppidx((aee_rr_curr_cpu_dvfs_oppidx() & 0x0F) | (idx << 4));
	else
		aee_rr_rec_cpu_dvfs_cci_oppidx((aee_rr_curr_cpu_dvfs_cci_oppidx() & 0xF0) | idx);
#endif
}

static void notify_cpu_volt_sampler(struct mt_cpu_dvfs *p, unsigned int volt)
{
	unsigned int mv = volt / 100;

	if (!g_pCpuVoltSampler)
		return;

	g_pCpuVoltSampler(p->id, mv);
}


/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {	\
	OP(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg),	\
}

#define OPP_TBL2(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_1[] = {	\
	OP(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg /* NOTE */),	\
	OP(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg),	\
}

OPP_TBL(LL, FY, 0, 1);
OPP_TBL(L, FY, 0, 1);
OPP_TBL(CCI, FY, 0, 1);

OPP_TBL2(LL, FY2, 1, 1);
OPP_TBL(L, FY2, 1, 1);
OPP_TBL(CCI, FY2, 1, 1);

OPP_TBL(LL, FY3, 2, 1);
OPP_TBL(L, FY3, 2, 1);
OPP_TBL(CCI, FY3, 2, 1);

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_LL_e0[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_LL_FY,	1,	1),
	FP(CPU_DVFS_FREQ1_LL_FY,	1,	1),
	FP(CPU_DVFS_FREQ2_LL_FY,	1,	1),
	FP(CPU_DVFS_FREQ3_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ4_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ5_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ6_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ7_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ8_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ9_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ10_LL_FY,	2,	1),
	FP(CPU_DVFS_FREQ11_LL_FY,	2,	2),
	FP(CPU_DVFS_FREQ12_LL_FY,	2,	2),
	FP(CPU_DVFS_FREQ13_LL_FY,	2,	2),
	FP(CPU_DVFS_FREQ14_LL_FY,	2,	2),
	FP(CPU_DVFS_FREQ15_LL_FY,	2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e0[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ1_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ2_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ3_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ4_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ5_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ6_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ7_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ8_L_FY,		1,	1),
	FP(CPU_DVFS_FREQ9_L_FY,		2,	1),
	FP(CPU_DVFS_FREQ10_L_FY,	2,	1),
	FP(CPU_DVFS_FREQ11_L_FY,	2,	1),
	FP(CPU_DVFS_FREQ12_L_FY,	2,	1),
	FP(CPU_DVFS_FREQ13_L_FY,	2,	1),
	FP(CPU_DVFS_FREQ14_L_FY,	2,	2),
	FP(CPU_DVFS_FREQ15_L_FY,	2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e0[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ1_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ2_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ3_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ4_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ5_CCI_FY,	2,	1),
	FP(CPU_DVFS_FREQ6_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ7_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ8_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ9_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ10_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ11_CCI_FY,	2,	2),
	FP(CPU_DVFS_FREQ12_CCI_FY,	2,	4),
	FP(CPU_DVFS_FREQ13_CCI_FY,	2,	4),
	FP(CPU_DVFS_FREQ14_CCI_FY,	2,	4),
	FP(CPU_DVFS_FREQ15_CCI_FY,	2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e1[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_LL_FY2,	1,	1),
	FP(CPU_DVFS_FREQ1_LL_FY2,	1,	1),
	FP(CPU_DVFS_FREQ2_LL_FY2,	1,	1),
	FP(CPU_DVFS_FREQ3_LL_FY2,	1,	1),
	FP(CPU_DVFS_FREQ4_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ5_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ6_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ7_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ8_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ9_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ10_LL_FY2,	2,	1),
	FP(CPU_DVFS_FREQ11_LL_FY2,	2,	2),
	FP(CPU_DVFS_FREQ12_LL_FY2,	2,	2),
	FP(CPU_DVFS_FREQ13_LL_FY2,	2,	2),
	FP(CPU_DVFS_FREQ14_LL_FY2,	2,	2),
	FP(CPU_DVFS_FREQ15_LL_FY2,	2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e1[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ1_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ2_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ3_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ4_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ5_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ6_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ7_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ8_L_FY2,	1,	1),
	FP(CPU_DVFS_FREQ9_L_FY2,	2,	1),
	FP(CPU_DVFS_FREQ10_L_FY2,	2,	1),
	FP(CPU_DVFS_FREQ11_L_FY2,	2,	1),
	FP(CPU_DVFS_FREQ12_L_FY2,	2,	1),
	FP(CPU_DVFS_FREQ13_L_FY2,	2,	1),
	FP(CPU_DVFS_FREQ14_L_FY2,	2,	2),
	FP(CPU_DVFS_FREQ15_L_FY2,	2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e1[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ1_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ2_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ3_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ4_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ5_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ6_CCI_FY2,	2,	1),
	FP(CPU_DVFS_FREQ7_CCI_FY2,	2,	2),
	FP(CPU_DVFS_FREQ8_CCI_FY2,	2,	2),
	FP(CPU_DVFS_FREQ9_CCI_FY2,	2,	2),
	FP(CPU_DVFS_FREQ10_CCI_FY2,	2,	2),
	FP(CPU_DVFS_FREQ11_CCI_FY2,	2,	2),
	FP(CPU_DVFS_FREQ12_CCI_FY2,	2,	4),
	FP(CPU_DVFS_FREQ13_CCI_FY2,	2,	4),
	FP(CPU_DVFS_FREQ14_CCI_FY2,	2,	4),
	FP(CPU_DVFS_FREQ15_CCI_FY2,	2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e2[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_LL_FY3,	1,	1),
	FP(CPU_DVFS_FREQ1_LL_FY3,	1,	1),
	FP(CPU_DVFS_FREQ2_LL_FY3,	1,	1),
	FP(CPU_DVFS_FREQ3_LL_FY3,	1,	1),
	FP(CPU_DVFS_FREQ4_LL_FY3,	1,	1),
	FP(CPU_DVFS_FREQ5_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ6_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ7_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ8_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ9_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ10_LL_FY3,	2,	1),
	FP(CPU_DVFS_FREQ11_LL_FY3,	2,	2),
	FP(CPU_DVFS_FREQ12_LL_FY3,	2,	2),
	FP(CPU_DVFS_FREQ13_LL_FY3,	2,	2),
	FP(CPU_DVFS_FREQ14_LL_FY3,	2,	2),
	FP(CPU_DVFS_FREQ15_LL_FY3,	2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e2[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ1_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ2_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ3_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ4_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ5_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ6_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ7_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ8_L_FY3,	1,	1),
	FP(CPU_DVFS_FREQ9_L_FY3,	2,	1),
	FP(CPU_DVFS_FREQ10_L_FY3,	2,	1),
	FP(CPU_DVFS_FREQ11_L_FY3,	2,	1),
	FP(CPU_DVFS_FREQ12_L_FY3,	2,	1),
	FP(CPU_DVFS_FREQ13_L_FY3,	2,	1),
	FP(CPU_DVFS_FREQ14_L_FY3,	2,	2),
	FP(CPU_DVFS_FREQ15_L_FY3,	2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e2[] = {
	/* Target Frequency,		POS,	CLK */
	FP(CPU_DVFS_FREQ0_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ1_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ2_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ3_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ4_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ5_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ6_CCI_FY3,	2,	1),
	FP(CPU_DVFS_FREQ7_CCI_FY3,	2,	2),
	FP(CPU_DVFS_FREQ8_CCI_FY3,	2,	2),
	FP(CPU_DVFS_FREQ9_CCI_FY3,	2,	2),
	FP(CPU_DVFS_FREQ10_CCI_FY3,	2,	2),
	FP(CPU_DVFS_FREQ11_CCI_FY3,	2,	2),
	FP(CPU_DVFS_FREQ12_CCI_FY3,	2,	4),
	FP(CPU_DVFS_FREQ13_CCI_FY3,	2,	4),
	FP(CPU_DVFS_FREQ14_CCI_FY3,	2,	4),
	FP(CPU_DVFS_FREQ15_CCI_FY3,	2,	4),
};

struct opp_tbl_info {
	struct mt_cpu_freq_info *const opp_tbl;
	const int size;
	const enum dvfs_table_type type;
};

static struct opp_tbl_info opp_tbls_v12d[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0, ARRAY_SIZE(opp_tbl_LL_e0_0), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_1, ARRAY_SIZE(opp_tbl_LL_e1_1), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0, ARRAY_SIZE(opp_tbl_LL_e2_0), DVFS_TABLE_TYPE_FY },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0, ARRAY_SIZE(opp_tbl_L_e0_0), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0, ARRAY_SIZE(opp_tbl_L_e1_0), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_2] = { opp_tbl_L_e2_0, ARRAY_SIZE(opp_tbl_L_e2_0), DVFS_TABLE_TYPE_FY },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0, ARRAY_SIZE(opp_tbl_CCI_e0_0), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0, ARRAY_SIZE(opp_tbl_CCI_e1_0), DVFS_TABLE_TYPE_FY },
		[CPU_LEVEL_2] = { opp_tbl_CCI_e2_0, ARRAY_SIZE(opp_tbl_CCI_e2_0), DVFS_TABLE_TYPE_FY },
	},
};

struct opp_tbl_m_info {
	struct mt_cpu_freq_method *const opp_tbl_m;
	const int size;
};

static struct opp_tbl_m_info opp_tbls_m_v12d[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_e0, ARRAY_SIZE(opp_tbl_method_LL_e0) },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_e1, ARRAY_SIZE(opp_tbl_method_LL_e1) },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_e2, ARRAY_SIZE(opp_tbl_method_LL_e2) },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_e0, ARRAY_SIZE(opp_tbl_method_L_e0) },
		[CPU_LEVEL_1] = { opp_tbl_method_L_e1, ARRAY_SIZE(opp_tbl_method_L_e1) },
		[CPU_LEVEL_2] = { opp_tbl_method_L_e2, ARRAY_SIZE(opp_tbl_method_L_e2) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_e0, ARRAY_SIZE(opp_tbl_method_CCI_e0) },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_e1, ARRAY_SIZE(opp_tbl_method_CCI_e1) },
		[CPU_LEVEL_2] = { opp_tbl_method_CCI_e2, ARRAY_SIZE(opp_tbl_method_CCI_e2) },
	},
};

static struct notifier_block __refdata _mt_cpufreq_cpu_notifier = {
	.notifier_call = _mt_cpufreq_cpu_CB,
};

/* for freq change (PLL/MUX) */
static int _search_available_freq_idx(struct mt_cpu_dvfs *p, unsigned int target_khz,
				      unsigned int relation)
{
	int new_opp_idx = -1;
	int i;

	if (relation == CPUFREQ_RELATION_L) {
		for (i = (signed)(p->nr_opp_tbl - 1); i >= 0; i--) {
			if (cpu_dvfs_get_freq_by_idx(p, i) >= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	} else {		/* CPUFREQ_RELATION_H */
		for (i = 0; i < (signed)p->nr_opp_tbl; i++) {
			if (cpu_dvfs_get_freq_by_idx(p, i) <= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	}

	return new_opp_idx;
}

/* for PTP-OD */
static mt_cpufreq_set_ptbl_funcPTP mt_cpufreq_update_private_tbl;

void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB)
{
	mt_cpufreq_update_private_tbl = pCB;
}

static int _set_cur_volt_locked(struct mt_cpu_dvfs *p, unsigned int volt)	/* volt: mv * 100 */
{
	int ret = -1;

	if (!cpu_dvfs_is_available(p))
		return 0;

	/* set volt */
	ret = p->ops->set_cur_volt(p, volt);

	return ret;
}

static int _restore_default_volt(struct mt_cpu_dvfs *p, enum mt_cpu_dvfs_id id)
{
	int i;
	int ret = -1;
	unsigned int freq = 0;
	unsigned int volt = 0;
	int idx = 0;

	cpufreq_lock();

	/* restore to default volt */
	for (i = 0; i < p->nr_opp_tbl; i++)
		p->opp_tbl[i].cpufreq_volt = p->opp_tbl[i].cpufreq_volt_org;

	if (mt_cpufreq_update_private_tbl)
		mt_cpufreq_update_private_tbl(id, 1);

	freq = p->ops->get_cur_phy_freq(p);
	volt = p->ops->get_cur_volt(p);

	if (freq > cpu_dvfs_get_max_freq(p))
		idx = 0;
	else
		idx = _search_available_freq_idx(p, freq, CPUFREQ_RELATION_L);

	/* set volt */
	if (cpu_dvfs_get_volt_by_idx(p, idx) > volt)
		ret = _set_cur_volt_locked(p, cpu_dvfs_get_volt_by_idx(p, idx));

	cpufreq_unlock();

	return ret;
}

unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p || !cpu_dvfs_is_available(p) || idx >= p->nr_opp_tbl)
		return 0;

	return cpu_dvfs_get_freq_by_idx(p, idx);
}

int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl, int nr_volt_tbl)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	int i;
	int ret = -1;
	unsigned int freq = 0;
	unsigned int volt = 0;
	int idx = 0;

	if (p && (!cpu_dvfs_is_available(p) || p->nr_opp_tbl == 0))
		return 0;

	if (!p || !volt_tbl || nr_volt_tbl > p->nr_opp_tbl)
		return -EPERM;

	cpufreq_lock();

	/* update volt table */
	for (i = 0; i < nr_volt_tbl; i++)
		p->opp_tbl[i].cpufreq_volt = EXTBUCK_VAL_TO_VOLT(volt_tbl[i]);

	if (mt_cpufreq_update_private_tbl)
		mt_cpufreq_update_private_tbl(id, 0);

	freq = p->ops->get_cur_phy_freq(p);
	volt = p->ops->get_cur_volt(p);

	if (freq > cpu_dvfs_get_max_freq(p))
		idx = 0;
	else
		idx = _search_available_freq_idx(p, freq, CPUFREQ_RELATION_L);

	/* set volt */
	if (cpu_dvfs_get_volt_by_idx(p, idx) > volt)
		ret = _set_cur_volt_locked(p, cpu_dvfs_get_volt_by_idx(p, idx));

	cpufreq_unlock();

	return ret;
}

void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p || !cpu_dvfs_is_available(p))
		return;

	_restore_default_volt(p, id);
}
/* for PTP-OD End*/

/* for PBM */
unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id)
{
#ifdef CONFIG_THERMAL
	struct mt_cpu_dvfs *p;
	int temp;
	int mw = 0;
	int i;

	if (id == 0) {
		for_each_cpu_dvfs_only(i, p) {
			if (cpu_dvfs_is(p, MT_CPU_DVFS_LL) && p->armpll_is_available) {
				temp = tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000;
				mw += mt_spower_get_leakage(MT_SPOWER_CPULL, cpu_dvfs_get_cur_volt(p) / 100, temp);
			} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L) && p->armpll_is_available) {
				temp = tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000;
				mw += mt_spower_get_leakage(MT_SPOWER_CPUL, cpu_dvfs_get_cur_volt(p) / 100, temp);
			}
		}
	} else if (id > 0) {
		id = id - 1;
		p = id_to_cpu_dvfs(id);
		if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			temp = tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000;
			mw = mt_spower_get_leakage(MT_SPOWER_CPULL, cpu_dvfs_get_cur_volt(p) / 100, temp);
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L)) {
			temp = tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000;
			mw = mt_spower_get_leakage(MT_SPOWER_CPUL, cpu_dvfs_get_cur_volt(p) / 100, temp);
		}
	}

	return mw;
#else
	return 0;
#endif
}

static void _kick_PBM_by_cpu(struct mt_cpu_dvfs *p)
{
#ifndef DISABLE_PBM_FEATURE
	struct mt_cpu_dvfs *p_dvfs[NR_MT_CPU_DVFS];
	struct cpumask dvfs_cpumask[NR_MT_CPU_DVFS];
	struct cpumask cpu_online_cpumask[NR_MT_CPU_DVFS];
	struct ppm_cluster_status ppm_data[NR_MT_CPU_DVFS];
	int i;

	aee_record_cpu_dvfs_pbm_step(1);

	for_each_cpu_dvfs_only(i, p) {
		arch_get_cluster_cpus(&dvfs_cpumask[i], i);
		cpumask_and(&cpu_online_cpumask[i], &dvfs_cpumask[i], cpu_online_mask);

		p_dvfs[i] = id_to_cpu_dvfs(i);

		ppm_data[i].core_num = cpumask_weight(&cpu_online_cpumask[i]);
		ppm_data[i].freq_idx = p_dvfs[i]->idx_opp_tbl;
		ppm_data[i].volt = cpu_dvfs_get_cur_volt(p_dvfs[i]) / 100;

		cpufreq_ver("%d: core = %d, idx = %d, volt = %d\n",
			    i, ppm_data[i].core_num, ppm_data[i].freq_idx, ppm_data[i].volt);
	}

	aee_record_cpu_dvfs_pbm_step(3);

	mt_ppm_dlpt_kick_PBM(ppm_data, arch_get_nr_clusters());
#endif

	aee_record_cpu_dvfs_pbm_step(0);
}
/* for PBM End */

/* Frequency API */
static unsigned int _cpu_freq_calc(unsigned int con1, unsigned int ckdiv1)
{
	unsigned int freq;
	unsigned int posdiv;

	posdiv = _GET_BITS_VAL_(26:24, con1);

	con1 &= _BITMASK_(21:0);
	freq = ((con1 * 26) >> 14) * 1000;

	switch (posdiv) {
	case 0:
		break;
	case 1:
		freq = freq / 2;
		break;
	case 2:
		freq = freq / 4;
		break;
	case 3:
		freq = freq / 8;
		break;
	default:
		freq = freq / 16;
		break;
	};

	switch (ckdiv1) {
	case 8:
		break;
	case 9:
		freq = freq * 3 / 4;
		break;
	case 10:
		freq = freq * 2 / 4;
		break;
	case 11:
		freq = freq * 1 / 4;
		break;
	case 16:
		break;
	case 17:
		freq = freq * 4 / 5;
		break;
	case 18:
		freq = freq * 3 / 5;
		break;
	case 19:
		freq = freq * 2 / 5;
		break;
	case 20:
		freq = freq * 1 / 5;
		break;
	case 24:
		break;
	case 25:
		freq = freq * 5 / 6;
		break;
	case 26:
		freq = freq * 4 / 6;
		break;
	case 27:
		freq = freq * 3 / 6;
		break;
	case 28:
		freq = freq * 2 / 6;
		break;
	case 29:
		freq = freq * 1 / 6;
		break;
	default:
		break;
	}

	return freq;
}

static unsigned int get_cur_phy_freq(struct mt_cpu_dvfs *p)
{
	unsigned int con1;
	unsigned int ckdiv1;
	unsigned int cur_khz;

	con1 = cpufreq_read(p->armpll_addr);
	ckdiv1 = cpufreq_read(p->ckdiv_addr);
	ckdiv1 = _GET_BITS_VAL_(4:0, ckdiv1);

	cur_khz = _cpu_freq_calc(con1, ckdiv1);

	cpufreq_ver("@%s: cur_khz = %u, con1[0x%p] = 0x%x, ckdiv1_val = 0x%x\n",
		    __func__, cur_khz, p->armpll_addr, con1, ckdiv1);

	return cur_khz;
}

unsigned int mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned int freq;

	if (!p)
		return 0;

	cpufreq_lock();
	freq = p->ops->get_cur_phy_freq(p);
	cpufreq_unlock();

	return freq;
}

unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned int freq;

	if (!p)
		return 0;

	freq = cpu_dvfs_get_cur_freq(p);

	return freq;
}

static unsigned int _cpu_dds_calc(unsigned int khz)
{
	unsigned int dds;

	dds = ((khz / 1000) << 14) / 26;

	return dds;
}

static void _cpu_clock_switch(struct mt_cpu_dvfs *p, enum top_ckmuxsel sel)
{
	if (sel == TOP_CKMUXSEL_MAINPLL || sel == TOP_CKMUXSEL_UNIVPLL)
		cpufreq_write_mask(CLK_MISC_CFG_0, 5:4, 0x3);

	if (cpu_dvfs_is(p, MT_CPU_DVFS_LL))
		cpufreq_write_mask(TOP_CKMUXSEL, 9:8, sel);
	else if (cpu_dvfs_is(p, MT_CPU_DVFS_L))
		cpufreq_write_mask(TOP_CKMUXSEL, 5:4, sel);
	else	/* CCI */
		cpufreq_write_mask(TOP_CKMUXSEL, 13:12, sel);

	if (sel == TOP_CKMUXSEL_ARMPLL || sel == TOP_CKMUXSEL_CLKSQ)
		cpufreq_write_mask(CLK_MISC_CFG_0, 5:4, 0x0);
}

static enum top_ckmuxsel _get_cpu_clock_switch(struct mt_cpu_dvfs *p)
{
	unsigned int val;
	unsigned int mask;

	val = cpufreq_read(TOP_CKMUXSEL);

	mask = (cpu_dvfs_is(p, MT_CPU_DVFS_LL) ? _BITMASK_(9:8) :
		cpu_dvfs_is(p, MT_CPU_DVFS_L) ? _BITMASK_(5:4) : _BITMASK_(13:12));

	val &= mask;

	val = (cpu_dvfs_is(p, MT_CPU_DVFS_LL) ? val >> 8 :
	       cpu_dvfs_is(p, MT_CPU_DVFS_L) ? val >> 4 : val >> 12);

	return val;
}

int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p || sel >= NR_TOP_CKMUXSEL)
		return -1;

	_cpu_clock_switch(p, sel);

	return 0;
}

enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return -1;

	return _get_cpu_clock_switch(p);
}

static void adjust_armpll_dds(struct mt_cpu_dvfs *p, unsigned int vco, unsigned int pos_div)
{
	unsigned int dds;
	unsigned int val;

	dds = _GET_BITS_VAL_(21:0, _cpu_dds_calc(vco));

	val = cpufreq_read(p->armpll_addr) & ~(_BITMASK_(21:0));
	val |= dds;

	cpufreq_write(p->armpll_addr, val | _BIT_(31) /* CHG */);
	udelay(PLL_SETTLE_TIME);
}

static void adjust_posdiv(struct mt_cpu_dvfs *p, unsigned int pos_div)
{
	unsigned int sel;

#if 0	/* glitch-free */
	_cpu_clock_switch(p, TOP_CKMUXSEL_MAINPLL);
#endif

	sel = (pos_div == 1 ? 0 :
	       pos_div == 2 ? 1 :
	       pos_div == 4 ? 2 : 0);

	cpufreq_write_mask(p->armpll_addr, 26:24, sel);
	udelay(POS_SETTLE_TIME);

#if 0	/* glitch-free */
	_cpu_clock_switch(p, TOP_CKMUXSEL_ARMPLL);
#endif
}

static void adjust_clkdiv(struct mt_cpu_dvfs *p, unsigned int clk_div)
{
	unsigned int sel;

	sel = (clk_div == 1 ? 8 :
	       clk_div == 2 ? 10 :
	       clk_div == 4 ? 11 : 8);

	if (cpu_dvfs_is(p, MT_CPU_DVFS_LL))
		cpufreq_write_mask(TOPCKGEN_CKDIV1_SML, 4:0, sel);
	else if (cpu_dvfs_is(p, MT_CPU_DVFS_L))
		cpufreq_write_mask(TOPCKGEN_CKDIV1_BIG, 4:0, sel);
	else	/* CCI */
		cpufreq_write_mask(TOPCKGEN_CKDIV1_BUS, 4:0, sel);

	udelay(POS_SETTLE_TIME);
}

static int search_table_idx_by_freq(struct mt_cpu_dvfs *p, unsigned int freq)
{
	int i;
	int idx = -1;

	for (i = 0; i < p->nr_opp_tbl; i++) {
		if (p->freq_tbl[i].target_f == freq) {
			idx = i;
			break;
		}
	}

	if (idx == -1)
		idx = p->idx_opp_tbl;

	return idx;
}

static void set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz)
{
	int idx;
	unsigned int sel, cur_posdiv, cur_clkdiv, dds;

	/* CUR_OPP_IDX */
	sel = _GET_BITS_VAL_(26:24, cpufreq_read(p->armpll_addr));
	cur_posdiv = (sel == 0 ? 1 :
		      sel == 1 ? 2 :
		      sel == 2 ? 4 : 1);
	sel = _GET_BITS_VAL_(4:0, cpufreq_read(p->ckdiv_addr));
	cur_clkdiv = (sel == 8 ? 1 :
		      sel == 10 ? 2 :
		      sel == 11 ? 4 : 1);
	cpufreq_ver("[CUR_OPP_IDX][NAME][IDX][FREQ] => %s:%d:%d\n",
		    cpu_dvfs_get_name(p), p->idx_opp_tbl, cpu_dvfs_get_freq_by_idx(p, p->idx_opp_tbl));

	/* TARGET_OPP_IDX */
	opp_tbl_m[TARGET_OPP_IDX].p = p;
	idx = search_table_idx_by_freq(opp_tbl_m[TARGET_OPP_IDX].p, target_khz);
	opp_tbl_m[TARGET_OPP_IDX].slot = &p->freq_tbl[idx];
	cpufreq_ver("[TARGET_OPP_IDX][NAME][IDX][FREQ] => %s:%d:%d\n",
		    cpu_dvfs_get_name(p), idx, cpu_dvfs_get_freq_by_idx(p, idx));

	if (cur_khz == target_khz)
		return;

	if (do_dvfs_stress_test)
		cpufreq_dbg("%s: %s: cur_khz = %u(%d), target_khz = %u(%d), clkdiv = %u->%u\n",
			    __func__, cpu_dvfs_get_name(p), cur_khz, p->idx_opp_tbl, target_khz, idx,
			    cur_clkdiv, opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);

	aee_record_cpu_dvfs_step(4);

#ifdef DCM_ENABLE
	p->ops->set_sync_dcm(0);	/* clock won't be slow down */
#endif

	now[SET_FREQ] = ktime_get();

	aee_record_cpu_dvfs_step(5);

	/* post_div 1 -> 2 */
	if (cur_posdiv < opp_tbl_m[TARGET_OPP_IDX].slot->pos_div)
		adjust_posdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);

	aee_record_cpu_dvfs_step(6);

	/* armpll_div 1 -> 2 */
	if (cur_clkdiv < opp_tbl_m[TARGET_OPP_IDX].slot->clk_div)
		adjust_clkdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);

	if (p->hopping_id != -1) {	/* actual FHCTL */
		aee_record_cpu_dvfs_step(7);

		dds = _GET_BITS_VAL_(21:0, _cpu_dds_calc(opp_tbl_m[TARGET_OPP_IDX].slot->vco_dds));
		mt_dfs_armpll(p->hopping_id, dds);
	} else {
		aee_record_cpu_dvfs_step(8);

		adjust_armpll_dds(p, opp_tbl_m[TARGET_OPP_IDX].slot->vco_dds, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);
	}

	aee_record_cpu_dvfs_step(9);

	/* armpll_div 2 -> 1 */
	if (cur_clkdiv > opp_tbl_m[TARGET_OPP_IDX].slot->clk_div)
		adjust_clkdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->clk_div);

	aee_record_cpu_dvfs_step(10);

	/* post_div 2 -> 1 */
	if (cur_posdiv > opp_tbl_m[TARGET_OPP_IDX].slot->pos_div)
		adjust_posdiv(p, opp_tbl_m[TARGET_OPP_IDX].slot->pos_div);

	aee_record_cpu_dvfs_step(11);

	delta[SET_FREQ] = ktime_sub(ktime_get(), now[SET_FREQ]);
	if (ktime_to_us(delta[SET_FREQ]) > ktime_to_us(max[SET_FREQ]))
		max[SET_FREQ] = delta[SET_FREQ];

#ifdef DCM_ENABLE
	p->ops->set_sync_dcm(target_khz / 1000);
#endif
}

#ifdef CONFIG_HYBRID_CPU_DVFS
static void set_cur_freq_hybrid(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz)
{
	int r, index;
	unsigned int cluster, volt, volt_val;

	if (WARN_ON(!enable_cpuhvfs))
		return;

	cluster = cpu_dvfs_to_cluster(p);
	index = search_table_idx_by_freq(p, target_khz);

	r = cpuhvfs_set_target_opp(cluster, index, &volt_val);
	if (r) {
		cpufreq_err("cluster%u dvfs failed (%d): opp = %d, freq = %u (%u)\n",
			    cluster, r, index, target_khz, cur_khz);
		BUG();
	}

	volt = EXTBUCK_VAL_TO_VOLT(volt_val);
	aee_record_cpu_volt(p, volt);
	notify_cpu_volt_sampler(p, volt);
}
#endif

/* for vsram change */
static int set_cur_volt_sram(struct mt_cpu_dvfs *p, unsigned int volt)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	pmic_set_register_value(PMIC_RG_LDO_VSRAM_PROC_VOSEL, VOLT_TO_PMIC_VAL(volt));
#else
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON, VOLT_TO_PMIC_VAL(volt));
#endif

	return 0;
}

static unsigned int get_cur_volt_sram(struct mt_cpu_dvfs *p)
{
	unsigned int rdata = 0;
	unsigned int retry_cnt = 5;

	do {
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
		rdata = pmic_get_register_value(PMIC_RG_LDO_VSRAM_PROC_VOSEL);
#else
		rdata = pmic_get_register_value(MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON);
#endif
	} while (rdata > 0x7f && retry_cnt--);

	rdata = PMIC_VAL_TO_VOLT(rdata);
	/* cpufreq_ver("@%s: volt = %u\n", __func__, rdata); */

	return rdata;	/* mv * 100 */
}

/* for vproc change */
static unsigned int get_cur_volt_extbuck(struct mt_cpu_dvfs *p)
{
	unsigned int ret_volt = 0;	/* volt: mv * 100 */
	unsigned int retry_cnt = 5;

	/* For avoiding i2c violation during suspend */
	if (p->dvfs_disable_by_suspend)
		return cpu_dvfs_get_cur_volt(p);

	if (cpu_dvfs_is_extbuck_valid()) {
		do {
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
			unsigned int ret_val;

			ret_val = pmic_get_register_value(PMIC_RG_BUCK_VPROC11_VOSEL);
			if (ret_val > 0x7f) {
				ret_volt = 0;
#else
			unsigned char ret_val;

			if (mt6311_read_byte(MT6311_VDVFS11_CON13, &ret_val) <= 0) {
				cpufreq_err("%s(), fail to read ext buck volt\n", __func__);
				ret_volt = 0;
#endif
			} else {
				ret_volt = EXTBUCK_VAL_TO_VOLT(ret_val);
				/* cpufreq_ver("@%s: volt = %u\n", __func__, ret_volt); */
			}
		} while (ret_volt == 0 && retry_cnt--);
	} else {
		cpufreq_err("%s(), can not use ext buck!\n", __func__);
	}

	return ret_volt;
}

unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return 0;

	return p->ops->get_cur_volt(p);	/* mv * 100 */
}
EXPORT_SYMBOL(mt_cpufreq_get_cur_volt);

static unsigned int _calc_pmic_settle_time(unsigned int old_vproc, unsigned int old_vsram,
					   unsigned int new_vproc, unsigned int new_vsram)
{
	unsigned delay = 100;

	if (new_vproc == old_vproc && new_vsram == old_vsram)
		return 0;

	/* VPROC is UP */
	if (new_vproc >= old_vproc) {
		/* VSRAM is UP too, choose larger one to calculate settle time */
		if (new_vsram >= old_vsram)
			delay = MAX(PMIC_VOLT_UP_SETTLE_TIME(old_vsram, new_vsram),
				    PMIC_VOLT_UP_SETTLE_TIME(old_vproc, new_vproc));
		/* VSRAM is DOWN, it may happen at bootup stage */
		else
			delay = MAX(PMIC_VOLT_DOWN_SETTLE_TIME(old_vsram, new_vsram),
				    PMIC_VOLT_UP_SETTLE_TIME(old_vproc, new_vproc));
	}
	/* VPROC is DOWN */
	else {
		/* VSRAM is DOWN too, choose larger one to calculate settle time */
		if (old_vsram >= new_vsram)
			delay = MAX(PMIC_VOLT_DOWN_SETTLE_TIME(old_vsram, new_vsram),
				    PMIC_VOLT_DOWN_SETTLE_TIME(old_vproc, new_vproc));
		/* VSRAM is UP, it may happen at bootup stage */
		else
			delay = MAX(PMIC_VOLT_UP_SETTLE_TIME(old_vsram, new_vsram),
				    PMIC_VOLT_DOWN_SETTLE_TIME(old_vproc, new_vproc));
	}

	if (delay < MIN_PMIC_SETTLE_TIME)
		delay = MIN_PMIC_SETTLE_TIME;

	return delay;
}

static void dump_all_opp_table(void)
{
	int i;
	struct mt_cpu_dvfs *p;

	for_each_cpu_dvfs(i, p) {
		cpufreq_err("[%s/%u] available = %d, oppidx = %d (%u, %u)\n",
			    p->name, p->cpu_id, p->armpll_is_available, p->idx_opp_tbl,
			    cpu_dvfs_get_cur_freq(p), cpu_dvfs_get_cur_volt(p));

		if (i == MT_CPU_DVFS_CCI) {
			for (i = 0; i < p->nr_opp_tbl; i++) {
				cpufreq_err("%-2d (%u, %u)\n",
					    i, cpu_dvfs_get_freq_by_idx(p, i), cpu_dvfs_get_volt_by_idx(p, i));
			}
		}
	}
}

static inline void assert_volt_valid(int line, unsigned int volt, unsigned int cur_vsram, unsigned int cur_vproc,
				     unsigned int old_vsram, unsigned int old_vproc)
{
	if (unlikely(cur_vsram < cur_vproc ||
		     cur_vsram - cur_vproc > MAX_DIFF_VSRAM_VPROC)) {
		cpufreq_err("@%d, volt = %u, cur_vsram = %u (%u), cur_vproc = %u (%u)\n",
			    line, volt, cur_vsram, old_vsram, cur_vproc, old_vproc);
		BUG();
	}
}

static int set_cur_volt_extbuck(struct mt_cpu_dvfs *p, unsigned int volt)
{				/* volt: vproc (mv*100) */
	unsigned int cur_vsram;
	unsigned int cur_vproc;
	unsigned int delay_us;

	/* For avoiding i2c violation during suspend */
	if (p->dvfs_disable_by_suspend)
		return 0;

	aee_record_cpu_volt(p, volt);

	now[SET_VOLT] = ktime_get();

	cur_vsram = p->ops->get_cur_vsram(p);
	cur_vproc = p->ops->get_cur_volt(p);

	if (cur_vproc == 0 || !cpu_dvfs_is_extbuck_valid()) {
		cpufreq_err("@%s():%d, can not use ext buck!\n", __func__, __LINE__);
		return -1;
	}

	if (unlikely(!(cur_vsram >= cur_vproc &&
		       cur_vsram - cur_vproc <= MAX_DIFF_VSRAM_VPROC))) {
		aee_kernel_warning(TAG, "@%s():%d, cur_vsram = %u, cur_vproc = %u\n",
				   __func__, __LINE__, cur_vsram, cur_vproc);
		cur_vproc = cpu_dvfs_get_cur_volt(p);
		cur_vsram = cur_vproc + NORMAL_DIFF_VRSAM_VPROC;
	}

	/* UP */
	if (volt > cur_vproc) {
		unsigned int target_vsram = volt + NORMAL_DIFF_VRSAM_VPROC;
		unsigned int next_vsram;

		do {
			unsigned int old_vproc = cur_vproc;
			unsigned int old_vsram = cur_vsram;

			next_vsram = MIN((MAX_DIFF_VSRAM_VPROC - 2500) + cur_vproc, target_vsram);

			/* update vsram */
			cur_vsram = MAX(next_vsram, MIN_VSRAM_VOLT);

			if (cur_vsram > max_vsram_volt) {
				cur_vsram = max_vsram_volt;
				target_vsram = max_vsram_volt;	/* to end the loop */
			}

			assert_volt_valid(__LINE__, volt, cur_vsram, cur_vproc, old_vsram, old_vproc);

			/* update vsram */
			now[SET_VSRAM] = ktime_get();
			p->ops->set_cur_vsram(p, cur_vsram);
			delta[SET_VSRAM] = ktime_sub(ktime_get(), now[SET_VSRAM]);
			if (ktime_to_us(delta[SET_VSRAM]) > ktime_to_us(max[SET_VSRAM]))
				max[SET_VSRAM] = delta[SET_VSRAM];

			/* update vproc */
			if (next_vsram > max_vsram_volt)
				cur_vproc = volt;	/* Vsram was limited, set to target vproc directly */
			else
				cur_vproc = next_vsram - NORMAL_DIFF_VRSAM_VPROC;

			assert_volt_valid(__LINE__, volt, cur_vsram, cur_vproc, old_vsram, old_vproc);

			now[SET_VPROC] = ktime_get();
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
			pmic_set_register_value(PMIC_RG_BUCK_VPROC11_VOSEL, VOLT_TO_EXTBUCK_VAL(cur_vproc));
#else
			mt6311_set_vdvfs11_vosel_on(VOLT_TO_EXTBUCK_VAL(cur_vproc));
#endif
			delta[SET_VPROC] = ktime_sub(ktime_get(), now[SET_VPROC]);
			if (ktime_to_us(delta[SET_VPROC]) > ktime_to_us(max[SET_VPROC]))
				max[SET_VPROC] = delta[SET_VPROC];

			delay_us = _calc_pmic_settle_time(old_vproc, old_vsram, cur_vproc, cur_vsram);
			udelay(delay_us);

			cpufreq_ver("@%s():UP, old_vsram=%u, cur_vsram=%u, old_vproc=%u, cur_vproc=%u, delay=%u\n",
				    __func__, old_vsram, cur_vsram, old_vproc, cur_vproc, delay_us);
		} while (cur_vsram < target_vsram);
	}
	/* DOWN */
	else if (volt < cur_vproc) {
		unsigned int next_vproc;
		unsigned int next_vsram = cur_vproc + NORMAL_DIFF_VRSAM_VPROC;

		do {
			unsigned int old_vproc = cur_vproc;
			unsigned int old_vsram = cur_vsram;

			next_vproc = MAX(next_vsram - (MAX_DIFF_VSRAM_VPROC - 2500), volt);

			/* update vproc */
			cur_vproc = next_vproc;

			assert_volt_valid(__LINE__, volt, cur_vsram, cur_vproc, old_vsram, old_vproc);

			now[SET_VPROC] = ktime_get();
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
			pmic_set_register_value(PMIC_RG_BUCK_VPROC11_VOSEL, VOLT_TO_EXTBUCK_VAL(cur_vproc));
#else
			mt6311_set_vdvfs11_vosel_on(VOLT_TO_EXTBUCK_VAL(cur_vproc));
#endif
			delta[SET_VPROC] = ktime_sub(ktime_get(), now[SET_VPROC]);
			if (ktime_to_us(delta[SET_VPROC]) > ktime_to_us(max[SET_VPROC]))
				max[SET_VPROC] = delta[SET_VPROC];

			/* update vsram */
			next_vsram = cur_vproc + NORMAL_DIFF_VRSAM_VPROC;
			cur_vsram = MAX(next_vsram, MIN_VSRAM_VOLT);
			cur_vsram = MIN(cur_vsram, max_vsram_volt);

			assert_volt_valid(__LINE__, volt, cur_vsram, cur_vproc, old_vsram, old_vproc);

			now[SET_VSRAM] = ktime_get();
			p->ops->set_cur_vsram(p, cur_vsram);
			delta[SET_VSRAM] = ktime_sub(ktime_get(), now[SET_VSRAM]);
			if (ktime_to_us(delta[SET_VSRAM]) > ktime_to_us(max[SET_VSRAM]))
				max[SET_VSRAM] = delta[SET_VSRAM];

			delay_us = _calc_pmic_settle_time(old_vproc, old_vsram, cur_vproc, cur_vsram);
			udelay(delay_us);

			cpufreq_ver("@%s():DOWN, old_vsram=%u, cur_vsram=%u, old_vproc=%u, cur_vproc=%u, delay=%u\n",
				    __func__, old_vsram, cur_vsram, old_vproc, cur_vproc, delay_us);
		} while (cur_vproc > volt);
	}

	delta[SET_VOLT] = ktime_sub(ktime_get(), now[SET_VOLT]);
	if (ktime_to_us(delta[SET_VOLT]) > ktime_to_us(max[SET_VOLT]))
		max[SET_VOLT] = delta[SET_VOLT];

	notify_cpu_volt_sampler(p, volt);

	cpufreq_ver("DVFS: End @%s(): %s, cur_vsram = %u (%u), cur_vproc = %u (%u)\n",
		    __func__, cpu_dvfs_get_name(p), cur_vsram, p->ops->get_cur_vsram(p),
		    cur_vproc, p->ops->get_cur_volt(p));

	return 0;
}

#ifdef CONFIG_HYBRID_CPU_DVFS
static int set_cur_volt_hybrid(struct mt_cpu_dvfs *p, unsigned int volt)
{
	if (WARN_ON(!enable_cpuhvfs))
		return -EPERM;

	return 0;
}

static unsigned int get_cur_volt_hybrid(struct mt_cpu_dvfs *p)
{
	unsigned int volt, volt_val;

	if (WARN_ON(!enable_cpuhvfs))
		return 0;

	volt_val = cpuhvfs_get_curr_volt(cpu_dvfs_to_cluster(p));
	if (volt_val != UINT_MAX)
		volt = EXTBUCK_VAL_TO_VOLT(volt_val);
	else
		volt = get_cur_volt_extbuck(p);

	return volt;
}
#endif

/* cpufreq set (freq & volt) */
#ifdef CCI_HALF_MAX_FREQ
static unsigned int _search_available_volt(struct mt_cpu_dvfs *p, unsigned int target_khz)
{
	int i;

	/* search available voltage */
	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (target_khz <= cpu_dvfs_get_freq_by_idx(p, i))
			break;
	}

	BUG_ON(i < 0);		/* i.e. target_khz > p->opp_tbl[0].cpufreq_khz */

	return cpu_dvfs_get_volt_by_idx(p, i);	/* mv * 100 */
}
#endif

static int _cpufreq_set_locked_cci(unsigned int cur_cci_khz, unsigned int target_cci_khz, unsigned int target_cci_volt)
{
	int ret = -1;
	enum mt_cpu_dvfs_id id = MT_CPU_DVFS_CCI;
	struct mt_cpu_dvfs *p_cci;
	unsigned int cur_cci_volt;

	aee_record_cpu_dvfs_in(id);

	aee_record_cci_dvfs_step(1);

	/* This is for cci */
	p_cci = id_to_cpu_dvfs(id);

	if (cur_cci_khz != target_cci_khz)
		cpufreq_ver
		    ("@%s(), %s: cur_cci_khz = %d, target_cci_khz = %d\n",
		     __func__, cpu_dvfs_get_name(p_cci), cur_cci_khz, target_cci_khz);

	cur_cci_volt = p_cci->ops->get_cur_volt(p_cci);

	aee_record_cci_dvfs_step(2);

	cpufreq_ver
		    ("@%s(), %s: cur_cci_volt = %d, target_cci_volt = %d\n",
		     __func__, cpu_dvfs_get_name(p_cci), cur_cci_volt, target_cci_volt);

	/* Set cci voltage (UP) */
	if (target_cci_volt > cur_cci_volt) {
		ret = p_cci->ops->set_cur_volt(p_cci, target_cci_volt);
		if (ret)	/* set volt fail */
			goto out;
	}

	aee_record_cci_dvfs_step(3);

	/* set cci freq (UP/DOWN) */
	if (cur_cci_khz != target_cci_khz)
		p_cci->ops->set_cur_freq(p_cci, cur_cci_khz, target_cci_khz);

	aee_record_cci_dvfs_step(4);

	/* Set cci voltage (DOWN) */
	if (target_cci_volt != 0 && (target_cci_volt < cur_cci_volt)) {
		ret = p_cci->ops->set_cur_volt(p_cci, target_cci_volt);
		if (ret)	/* set volt fail */
			goto out;
	}

	aee_record_cci_dvfs_step(5);

	cpufreq_ver("@%s(): Vproc = %dmv, Vsram = %dmv, freq = %dKHz\n",
		    __func__,
		    (p_cci->ops->get_cur_volt(p_cci)) / 100,
		    (p_cci->ops->get_cur_vsram(p_cci) / 100), p_cci->ops->get_cur_phy_freq(p_cci));

out:
	aee_record_cci_dvfs_step(0);

	aee_record_cpu_dvfs_out(id);

	return ret;
}

static int _cpufreq_set_locked(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz,
			       struct cpufreq_policy *policy, unsigned int cur_cci_khz, unsigned int target_cci_khz,
				   unsigned int target_volt_vproc1, int log)
{
	int ret = -1;
	unsigned int target_volt;
	unsigned int cur_volt;
	struct cpufreq_freqs freqs;

	if (dvfs_disable_flag == 1)
		return 0;

	aee_record_cpu_dvfs_step(1);

	if (!policy) {
		cpufreq_err("Can't get policy of %s\n", cpu_dvfs_get_name(p));
		goto out;
	}

	cpufreq_ver("DVFS: _cpufreq_set_locked: target_vproc1 = %d\n", target_volt_vproc1);

	/* VPROC1 came from MCSI */
	target_volt = target_volt_vproc1;
	/* a little tricky here, no need to set volt at cci set_locked */
	target_volt_vproc1 = 0;

	if (cur_khz != target_khz) {
		if (log || do_dvfs_stress_test)
			cpufreq_dbg
				("@%s(), %s:(%d,%d): freq=%d, volt=%d, on=%d, cur=%d, cci(%d,%d)\n",
				 __func__, cpu_dvfs_get_name(p), p->idx_opp_ppm_base, p->idx_opp_ppm_limit,
				 target_khz, target_volt, num_online_cpus(), cur_khz,
				 cur_cci_khz, target_cci_khz);
		else
			cpufreq_ver
				("@%s(), %s:(%d,%d): freq=%d, volt=%d, on=%d, cur=%d, cci(%d,%d)\n",
				 __func__, cpu_dvfs_get_name(p), p->idx_opp_ppm_base, p->idx_opp_ppm_limit,
				 target_khz, target_volt, num_online_cpus(), cur_khz,
				 cur_cci_khz, target_cci_khz);
	}

#ifdef CONFIG_HYBRID_CPU_DVFS
	if (!enable_cpuhvfs) {
#endif
		/* set volt (UP) */
		cur_volt = p->ops->get_cur_volt(p);
		if (target_volt > cur_volt) {
			ret = p->ops->set_cur_volt(p, target_volt);

			if (ret)	/* set volt fail */
				goto out;
		}
#ifdef CONFIG_HYBRID_CPU_DVFS
	}
#endif

	aee_record_cpu_dvfs_step(2);

	freqs.old = cur_khz;
	freqs.new = target_khz;
	if (policy) {
		freqs.cpu = policy->cpu;
		cpufreq_freq_transition_begin(policy, &freqs);
	}

	aee_record_cpu_dvfs_step(3);

	/* set freq (UP/DOWN) */
	if (cur_khz != target_khz)
		p->ops->set_cur_freq(p, cur_khz, target_khz);

	aee_record_cpu_dvfs_step(12);

	if (policy)
		cpufreq_freq_transition_end(policy, &freqs, 0);

	aee_record_cpu_dvfs_step(13);

#ifdef CONFIG_HYBRID_CPU_DVFS
	if (!enable_cpuhvfs) {
#endif
		/* set cci freq/volt */
		_cpufreq_set_locked_cci(cur_cci_khz, target_cci_khz, target_volt_vproc1);

		aee_record_cpu_dvfs_step(14);

		/* set volt (DOWN) */
		cur_volt = p->ops->get_cur_volt(p);
		if (cur_volt > target_volt) {
			ret = p->ops->set_cur_volt(p, target_volt);

			if (ret)	/* set volt fail */
				goto out;
		}
#ifdef CONFIG_HYBRID_CPU_DVFS
	}
#endif

	aee_record_cpu_dvfs_step(15);

	cpufreq_ver("DVFS: @%s(): Vproc = %dmv, Vsram = %dmv, freq(%s) = %dKHz\n",
		    __func__,
		    (p->ops->get_cur_volt(p)) / 100,
		    (p->ops->get_cur_vsram(p) / 100), p->name, p->ops->get_cur_phy_freq(p));

	/* trigger exception if freq/volt not correct during stress */
	if (do_dvfs_stress_test && !p->dvfs_disable_by_suspend
#ifdef CONFIG_HYBRID_CPU_DVFS
	    && (!enable_cpuhvfs || cur_khz != target_khz)
#endif
	) {
		unsigned int volt = p->ops->get_cur_volt(p);
		unsigned int freq = p->ops->get_cur_phy_freq(p);

		if (volt < target_volt || freq != target_khz) {
			cpufreq_err("volt = %u, target_volt = %u, freq = %u, target_khz = %u\n",
				volt, target_volt, freq, target_khz);
			dump_all_opp_table();
			BUG();
		}
	}

	ret = 0;

out:
	aee_record_cpu_dvfs_step(0);

	return ret;
}

static unsigned int _calc_new_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx);
static unsigned int _calc_new_cci_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx,
	unsigned int *target_cci_volt);

static void _mt_cpufreq_set(struct cpufreq_policy *policy, enum mt_cpu_dvfs_id id, int new_opp_idx, int lock)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	unsigned int cur_freq;
	unsigned int target_freq;
	int new_cci_opp_idx;
	unsigned int cur_cci_freq;
	unsigned int target_cci_freq;

	enum mt_cpu_dvfs_id id_cci;
	struct mt_cpu_dvfs *p_cci;
	int ret = -1;
	unsigned int target_volt_vpro1 = 0;
	int log = 1;

#ifndef __TRIAL_RUN__
	if (!policy || !p->mt_policy)
#endif
		return;

	/* This is for cci */
	id_cci = MT_CPU_DVFS_CCI;
	p_cci = id_to_cpu_dvfs(id_cci);

	if (lock)
		cpufreq_lock();

	if (p->dvfs_disable_by_suspend || !p->armpll_is_available
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
	    || (enable_cpuhvfs && enable_hw_gov)
#endif
	) {
		if (lock)
			cpufreq_unlock();
		return;
	}

	/* get current idx here to avoid idx synchronization issue */
	if (new_opp_idx == -1) {
		policy = p->mt_policy;
		new_opp_idx = p->idx_opp_tbl;
		log = 0;
	}

	if (do_dvfs_stress_test) {
		new_opp_idx = prandom_u32() % p->nr_opp_tbl;

		if (new_opp_idx < p->idx_opp_ppm_limit)
			new_opp_idx = p->idx_opp_ppm_limit;
	} else
		new_opp_idx = _calc_new_opp_idx(p, new_opp_idx);

	if (abs(new_opp_idx - p->idx_opp_tbl) < 5 && new_opp_idx != 0 &&
		new_opp_idx != p->nr_opp_tbl - 1)
		log = 0;

	/* Get cci opp idx */
	new_cci_opp_idx = _calc_new_cci_opp_idx(p, new_opp_idx, &target_volt_vpro1);
	cur_cci_freq = p_cci->ops->get_cur_phy_freq(p_cci);
	target_cci_freq = cpu_dvfs_get_freq_by_idx(p_cci, new_cci_opp_idx);

	cur_freq = p->ops->get_cur_phy_freq(p);
	target_freq = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);

	now[SET_DVFS] = ktime_get();

	aee_record_cpu_dvfs_in(id);

	ret = _cpufreq_set_locked(p, cur_freq, target_freq, policy, cur_cci_freq, target_cci_freq,
		target_volt_vpro1, log);

	aee_record_cpu_dvfs_out(id);

	delta[SET_DVFS] = ktime_sub(ktime_get(), now[SET_DVFS]);
	if (ktime_to_us(delta[SET_DVFS]) > ktime_to_us(max[SET_DVFS]))
		max[SET_DVFS] = delta[SET_DVFS];

	p->idx_opp_tbl = new_opp_idx;
	p_cci->idx_opp_tbl = new_cci_opp_idx;

	aee_record_freq_idx(p, p->idx_opp_tbl);
	aee_record_freq_idx(p_cci, p_cci->idx_opp_tbl);

	if (lock)
		cpufreq_unlock();

	if (!ret && !p->dvfs_disable_by_suspend && lock)
		_kick_PBM_by_cpu(p);
}

static int _search_available_freq_idx_under_v(struct mt_cpu_dvfs *p, unsigned int volt)
{
	int i;

	/* search available voltage */
	for (i = 0; i < p->nr_opp_tbl; i++) {
		if (volt >= cpu_dvfs_get_volt_by_idx(p, i))
			break;
	}

	BUG_ON(i >= p->nr_opp_tbl);

	return i;
}

static int _cpufreq_dfs_locked(struct cpufreq_policy *policy, struct mt_cpu_dvfs *p,
			       unsigned int freq_idx, unsigned long action)
{
	int ret = 0;
	struct cpufreq_freqs freqs;

	freqs.old = cpu_dvfs_get_cur_freq(p);
	freqs.new = cpu_dvfs_get_freq_by_idx(p, freq_idx);
	if (policy) {
		freqs.cpu = policy->cpu;
		cpufreq_freq_transition_begin(policy, &freqs);
	}

#ifdef CONFIG_HYBRID_CPU_DVFS
	if (enable_cpuhvfs) {
		action &= ~CPU_TASKS_FROZEN;
		if (action == CPU_DOWN_PREPARE) {
			cpuhvfs_notify_cluster_off(cpu_dvfs_to_cluster(p));
			goto DFS_DONE;
		} else if (action == CPU_DOWN_FAILED) {
			cpuhvfs_notify_cluster_on(cpu_dvfs_to_cluster(p));
		}
	}
#endif

	if (cpu_dvfs_get_cur_freq(p) != cpu_dvfs_get_freq_by_idx(p, freq_idx))
		p->ops->set_cur_freq(p, cpu_dvfs_get_cur_freq(p)
			, cpu_dvfs_get_freq_by_idx(p, freq_idx));

#ifdef CONFIG_HYBRID_CPU_DVFS
DFS_DONE:
#endif

	if (policy)
		cpufreq_freq_transition_end(policy, &freqs, 0);
	cpufreq_ver("CB - notify %d to governor\n", freqs.new);

	p->idx_opp_tbl = freq_idx;
	aee_record_freq_idx(p, p->idx_opp_tbl);

	return ret;
}

#define SINGLE_CORE_BOUNDARY_CPU_NUM 1
static unsigned int num_online_cpus_delta;
static int _mt_cpufreq_cpu_CB(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;

	enum mt_cpu_dvfs_id cluster_id;
	struct mt_cpu_dvfs *p;
	struct mt_cpu_dvfs *p_cci;
	int new_cci_opp_idx;
	unsigned int target_volt_vpro1 = 0;

	unsigned int cur_volt = 0;
	int freq_idx;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask dvfs_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus;

	if (dvfs_disable_flag == 1)
		return NOTIFY_OK;

	p_cci = id_to_cpu_dvfs(MT_CPU_DVFS_CCI);

	aee_record_cpu_dvfs_cb(1);

	cluster_id = arch_get_cluster_id(cpu);
	arch_get_cluster_cpus(&dvfs_cpumask, cluster_id);
	cpumask_and(&cpu_online_cpumask, &dvfs_cpumask, cpu_online_mask);
	p = id_to_cpu_dvfs(cluster_id);

	cpufreq_ver("@%s():%d, cpu = %d, action = %lu, oppidx = %d, num_online_cpus = %d\n"
	, __func__, __LINE__, cpu, action, p->idx_opp_tbl, online_cpus);

	dev = get_cpu_device(cpu);

	/* Pull up frequency when single core */
	if (dev) {
		if (online_cpus == SINGLE_CORE_BOUNDARY_CPU_NUM && cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			switch (action) {
			case CPU_UP_PREPARE:
			case CPU_UP_PREPARE_FROZEN:
				num_online_cpus_delta = 1;
				break;
			case CPU_UP_CANCELED:
			case CPU_UP_CANCELED_FROZEN:
				num_online_cpus_delta = 0;
				break;
			}
		} else {
			switch (action) {
			case CPU_ONLINE:
			case CPU_ONLINE_FROZEN:
				num_online_cpus_delta = 0;
				break;
			}
		}
	}

	aee_record_cpu_dvfs_cb(2);

	if (dev) {
		switch (action & ~CPU_TASKS_FROZEN) {
		case CPU_ONLINE:
			cpus = cpumask_weight(&cpu_online_cpumask);
			cpufreq_ver("CPU_ONLINE -> cpus = %d\n", cpus);
			if (cpus == 1) {
				aee_record_cpu_dvfs_cb(3);

				cpufreq_ver("CPU_ONLINE first CPU of %s\n",
					cpu_dvfs_get_name(p));
				cpufreq_lock();
				p->armpll_is_available = 1;
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
				if (enable_cpuhvfs && enable_hw_gov)
					goto UNLOCK_OL;		/* bypass specific adjustment */
#endif
				if (action == CPU_ONLINE) {
					aee_record_cpu_dvfs_cb(5);

					cur_volt = p->ops->get_cur_volt(p);
					cpufreq_ver("CB - adjust the freq to V:%d  due to L/LL on\n", cur_volt);
					freq_idx = _search_available_freq_idx_under_v(p, cur_volt);
					freq_idx = MAX(freq_idx, _calc_new_opp_idx(p, freq_idx));
					_cpufreq_dfs_locked(p->mt_policy, p, freq_idx, action);
				}
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
UNLOCK_OL:
#endif
				cpufreq_unlock();
			}
			break;
		case CPU_DOWN_PREPARE:
			cpus = cpumask_weight(&cpu_online_cpumask);
			cpufreq_ver("CPU_DOWN_PREPARE -> cpus = %d\n", cpus);
			if (cpus == 1) {
				aee_record_cpu_dvfs_cb(6);

				cpufreq_ver("CPU_DOWN_PREPARE last CPU of %s\n",
					cpu_dvfs_get_name(p));
				cpufreq_lock();
				aee_record_cpu_dvfs_cb(7);

				_cpufreq_dfs_locked(NULL, p, p->nr_opp_tbl - 1, action);
#ifdef CONFIG_HYBRID_CPU_DVFS
				if (!enable_cpuhvfs) {
#endif
					new_cci_opp_idx = _calc_new_cci_opp_idx(p, p->idx_opp_tbl,
						&target_volt_vpro1);
					/* set cci freq/volt */
					_cpufreq_set_locked_cci(cpu_dvfs_get_cur_freq(p_cci),
						cpu_dvfs_get_freq_by_idx(p_cci, new_cci_opp_idx),
						target_volt_vpro1);
					p_cci->idx_opp_tbl = new_cci_opp_idx;
					aee_record_freq_idx(p_cci, p_cci->idx_opp_tbl);
#ifdef CONFIG_HYBRID_CPU_DVFS
				}
#endif
				p->armpll_is_available = 0;
				cpufreq_unlock();

				aee_record_cpu_dvfs_cb(11);
			}
			break;
		case CPU_DOWN_FAILED:
			cpus = cpumask_weight(&cpu_online_cpumask);
			cpufreq_ver("CPU_DOWN_FAILED -> cpus = %d\n", cpus);
			if (cpus == 1) {
				cpufreq_lock();
				p->armpll_is_available = 1;
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
				if (enable_cpuhvfs && enable_hw_gov)
					goto UNLOCK_DF;		/* bypass specific adjustment */
#endif
				if (action == CPU_DOWN_FAILED) {
					cur_volt = p->ops->get_cur_volt(p);
					freq_idx = _search_available_freq_idx_under_v(p, cur_volt);
					freq_idx = MAX(freq_idx, _calc_new_opp_idx(p, freq_idx));
					_cpufreq_dfs_locked(p->mt_policy, p, freq_idx, action);
				}
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
UNLOCK_DF:
#endif
				cpufreq_unlock();
			}
			break;
		}

		aee_record_cpu_dvfs_cb(12);

		/* Notify PBM after CPU on/off */
		if (action == CPU_ONLINE || action == CPU_ONLINE_FROZEN
			|| action == CPU_DEAD || action == CPU_DEAD_FROZEN) {

			aee_record_cpu_dvfs_cb(13);

			if (!p->dvfs_disable_by_suspend)
				_kick_PBM_by_cpu(p);
		}
	}

	cpufreq_ver("@%s():%d, num_online_cpus_delta = %d\n", __func__, __LINE__,
		    num_online_cpus_delta);

	cpufreq_ver("@%s():%d, cpu = %d, action = %lu, oppidx = %d, num_online_cpus = %d\n"
	, __func__, __LINE__, cpu, action, p->idx_opp_tbl, online_cpus);

	aee_record_cpu_dvfs_cb(0);

	return NOTIFY_OK;
}

static int _sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
{
	unsigned int freq;
	int i;

	freq = p->ops->get_cur_phy_freq(p);

	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (freq <= cpu_dvfs_get_freq_by_idx(p, i))
			break;
	}

	if (WARN(i < 0, "CURR_FREQ %u IS OVER OPP0 %u\n", freq, cpu_dvfs_get_max_freq(p)))
		i = 0;

	p->idx_opp_tbl = i;
	aee_record_freq_idx(p, p->idx_opp_tbl);

	return 0;
}

static int _mt_cpufreq_sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
{
	int ret = -1;

	if (cpu_dvfs_is_available(p))
		ret = _sync_opp_tbl_idx(p);

	cpufreq_info("%s freq = %d\n", cpu_dvfs_get_name(p), cpu_dvfs_get_cur_freq(p));

	return ret;
}

static enum mt_cpu_dvfs_id _get_cpu_dvfs_id(unsigned int cpu_id)
{
	int cluster_id;

	cluster_id = arch_get_cluster_id(cpu_id);

	return cluster_id;
}

static int _mt_cpufreq_setup_freqs_table(struct cpufreq_policy *policy,
					 struct mt_cpu_freq_info *freqs, int num)
{
	struct mt_cpu_dvfs *p;
	int ret = 0;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

	ret = cpufreq_frequency_table_cpuinfo(policy, p->freq_tbl_for_cpufreq);

	if (!ret)
		policy->freq_table = p->freq_tbl_for_cpufreq;

	cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));
	cpumask_copy(policy->related_cpus, policy->cpus);

	return 0;
}

static unsigned int _calc_new_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx)
{
	cpufreq_ver("new_opp_idx = %d, idx_opp_ppm_base = %d, idx_opp_ppm_limit = %d\n",
		new_opp_idx, p->idx_opp_ppm_base, p->idx_opp_ppm_limit);

#ifdef LL_1CORE_FLOOR_IDX
	if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
		unsigned int online_cpus = num_online_cpus() + num_online_cpus_delta;

		if (online_cpus == 1 && new_opp_idx > LL_1CORE_FLOOR_IDX)
			new_opp_idx = LL_1CORE_FLOOR_IDX;
	}
#endif

#ifdef L_FLOOR_TRACK_LL
	if (cpu_dvfs_is(p, MT_CPU_DVFS_L)) {
		struct mt_cpu_dvfs *p_ll = id_to_cpu_dvfs(MT_CPU_DVFS_LL);

		if (p_ll->armpll_is_available && new_opp_idx > p_ll->idx_opp_tbl)
			new_opp_idx = p_ll->idx_opp_tbl;
	}
#endif

	if ((p->idx_opp_ppm_limit != -1) && (new_opp_idx < p->idx_opp_ppm_limit))
		new_opp_idx = p->idx_opp_ppm_limit;

	if ((p->idx_opp_ppm_base != -1) && (new_opp_idx > p->idx_opp_ppm_base))
		new_opp_idx = p->idx_opp_ppm_base;

	if ((p->idx_opp_ppm_base == p->idx_opp_ppm_limit) && p->idx_opp_ppm_base != -1)
		new_opp_idx = p->idx_opp_ppm_base;

	return new_opp_idx;
}

static unsigned int _calc_new_cci_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx,
	unsigned int *target_cci_volt)
{
	/* LL, L */
	int freq_idx[NR_MT_CPU_DVFS - 1] = {-1};
	unsigned int volt[NR_MT_CPU_DVFS - 1] = {0};
	int i;

	struct mt_cpu_dvfs *pp;
	struct mt_cpu_dvfs *p_cci = id_to_cpu_dvfs(MT_CPU_DVFS_CCI);

	unsigned int target_cci_freq = 0;
	int new_cci_opp_idx;

	/* Fill the V and F to determine cci state*/
	/* MCSI Algorithm for cci target F */
	for_each_cpu_dvfs_only(i, pp) {
		if (pp->armpll_is_available || pp->idx_opp_tbl != pp->nr_opp_tbl - 1) {
			/* F */
			if (pp == p)
				freq_idx[i] = new_opp_idx;
			else
				freq_idx[i] = pp->idx_opp_tbl;

			target_cci_freq = MAX(target_cci_freq, cpu_dvfs_get_freq_by_idx(pp, freq_idx[i]));
			/* V */
			volt[i] = cpu_dvfs_get_volt_by_idx(pp, freq_idx[i]);
			cpufreq_ver("DVFS: MCSI: %s, freq = %d, volt = %d\n", cpu_dvfs_get_name(pp),
				cpu_dvfs_get_freq_by_idx(pp, freq_idx[i]), volt[i]);
		}
	}

#ifdef CCI_HALF_MAX_FREQ
	target_cci_freq = target_cci_freq / 2;
	if (target_cci_freq > cpu_dvfs_get_freq_by_idx(p_cci, 0)) {
		target_cci_freq = cpu_dvfs_get_freq_by_idx(p_cci, 0);
		*target_cci_volt = cpu_dvfs_get_volt_by_idx(p_cci, 0);
	} else
		*target_cci_volt = _search_available_volt(p_cci, target_cci_freq);

	/* Determine dominating voltage */
	*target_cci_volt = MAX(*target_cci_volt, MAX(volt[MT_CPU_DVFS_LL], volt[MT_CPU_DVFS_L]));
#else
	*target_cci_volt = MAX(volt[MT_CPU_DVFS_LL], volt[MT_CPU_DVFS_L]);
#endif
	cpufreq_ver("DVFS: MCSI: target_cci (F,V) = (%d, %d)\n", target_cci_freq, *target_cci_volt);

	new_cci_opp_idx = _search_available_freq_idx_under_v(p_cci, *target_cci_volt);

	cpufreq_ver("DVFS: MCSI: Final Result, target_cci_volt = %d, target_cci_freq = %d\n",
		*target_cci_volt, cpu_dvfs_get_freq_by_idx(p_cci, new_cci_opp_idx));

	return new_cci_opp_idx;
}

static void ppm_limit_callback(struct ppm_client_req req)
{
	struct ppm_client_req *ppm = (struct ppm_client_req *)&req;

	struct mt_cpu_dvfs *p;
	int ignore_ppm[NR_MT_CPU_DVFS] = {0};
	unsigned int i;
	int kick = 0;

	if (dvfs_disable_flag)
		return;

	cpufreq_ver("get feedback from PPM module\n");

	cpufreq_lock();
	for (i = 0; i < ppm->cluster_num; i++) {
		cpufreq_ver("[%d]:cluster_id = %d, cpu_id = %d, min_cpufreq_idx = %d, max_cpufreq_idx = %d\n",
			i, ppm->cpu_limit[i].cluster_id, ppm->cpu_limit[i].cpu_id,
			ppm->cpu_limit[i].min_cpufreq_idx, ppm->cpu_limit[i].max_cpufreq_idx);
		cpufreq_ver("has_advise_freq = %d, advise_cpufreq_idx = %d\n",
			ppm->cpu_limit[i].has_advise_freq, ppm->cpu_limit[i].advise_cpufreq_idx);

		p = id_to_cpu_dvfs(i);

		if (ppm->cpu_limit[i].has_advise_freq) {
			p->idx_opp_ppm_base = ppm->cpu_limit[i].advise_cpufreq_idx;
			p->idx_opp_ppm_limit = ppm->cpu_limit[i].advise_cpufreq_idx;
			if (p->idx_opp_tbl == ppm->cpu_limit[i].advise_cpufreq_idx) {
				cpufreq_ver("idx = %d, advise_cpufreq_idx = %d\n", p->idx_opp_tbl,
					ppm->cpu_limit[i].advise_cpufreq_idx);
				ignore_ppm[i] = 1;
			}
		} else {
			p->idx_opp_ppm_base = ppm->cpu_limit[i].min_cpufreq_idx;	/* ppm update base */
			p->idx_opp_ppm_limit = ppm->cpu_limit[i].max_cpufreq_idx;	/* ppm update limit */
			if ((p->idx_opp_tbl >= ppm->cpu_limit[i].max_cpufreq_idx)
				&& (p->idx_opp_tbl <= ppm->cpu_limit[i].min_cpufreq_idx)) {
				cpufreq_ver("idx = %d, idx_opp_ppm_base = %d, idx_opp_ppm_limit = %d\n",
					p->idx_opp_tbl, p->idx_opp_ppm_base, p->idx_opp_ppm_limit);
				ignore_ppm[i] = 1;
			}
		}
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
		if (enable_cpuhvfs && enable_hw_gov) {
			cpuhvfs_set_opp_limit(cpu_dvfs_to_cluster(p),
					      opp_limit_to_ceiling(p->idx_opp_ppm_limit),
					      opp_limit_to_floor(p->idx_opp_ppm_base));
			ignore_ppm[i] = 1;	/* no SW DVFS request */
		}
#endif
	}

	for_each_cpu_dvfs_only(i, p) {
		if (p->armpll_is_available && p->mt_policy->governor) {
			if (p->idx_opp_ppm_limit == -1)
				p->mt_policy->max = cpu_dvfs_get_max_freq(p);
			else
				p->mt_policy->max = cpu_dvfs_get_freq_by_idx(p, p->idx_opp_ppm_limit);

			if (p->idx_opp_ppm_base == -1)
				p->mt_policy->min = cpu_dvfs_get_min_freq(p);
			else
				p->mt_policy->min = cpu_dvfs_get_freq_by_idx(p, p->idx_opp_ppm_base);

			if (!ignore_ppm[i]) {
				_mt_cpufreq_set(p->mt_policy, i, -1, 0);
				kick = 1;
			}
		}
	}
	cpufreq_unlock();

	if (!p->dvfs_disable_by_suspend && kick)
		_kick_PBM_by_cpu(p);
}

/*
 * cpufreq driver
 */
int mt_cpufreq_get_ppb_state(void)
{
	return dvfs_power_mode;
}

unsigned int mt_cpufreq_ppb_hispeed_freq(unsigned int cpu, unsigned int mode)
{
	enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(cpu);

	if (id >= NR_MT_CPU_DVFS || mode >= NUM_PPB_POWER_MODE)
		return 0;

	return ppb_hispeed_freq[id][mode];	/* OPP index */
}

static int _mt_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct mt_cpu_dvfs *p;
	int ret;		/* cpufreq_frequency_table_verify() always return 0 */

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));
	if (!p)
		return -EINVAL;

	ret = cpufreq_frequency_table_verify(policy, p->freq_tbl_for_cpufreq);

	return ret;
}

static int _mt_cpufreq_target(struct cpufreq_policy *policy, unsigned int target_freq,
			      unsigned int relation)
{
	struct mt_cpu_dvfs *p;
	int ret;
	unsigned int new_opp_idx;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));
	if (!p)
		return -EINVAL;

	ret = cpufreq_frequency_table_target(policy, p->freq_tbl_for_cpufreq,
					     target_freq, relation, &new_opp_idx);
	if (ret || new_opp_idx >= p->nr_opp_tbl)
		return -EINVAL;

	if (dvfs_disable_flag || p->dvfs_disable_by_suspend || p->dvfs_disable_by_procfs)
		return -EPERM;

	_mt_cpufreq_set(policy, p->id, new_opp_idx, 1);

	return 0;
}

static int init_cci_status;
static int _mt_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = 0;

	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

	policy->cpuinfo.transition_latency = 1000;

	{
		enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(policy->cpu);
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
		unsigned int lv = _mt_cpufreq_get_cpu_level();
		struct opp_tbl_info *opp_tbl_info;
		struct opp_tbl_m_info *opp_tbl_m_info;
		struct opp_tbl_m_info *opp_tbl_m_cci_info;

		enum mt_cpu_dvfs_id id_cci;
		struct mt_cpu_dvfs *p_cci;

		/* This is for cci */
		id_cci = MT_CPU_DVFS_CCI;
		p_cci = id_to_cpu_dvfs(id_cci);

		if (WARN_ON(!p))
			return -EINVAL;

		cpufreq_ver("DVFS: _mt_cpufreq_init: %s(cpu_id = %d)\n", cpu_dvfs_get_name(p), p->cpu_id);

		opp_tbl_info = &opp_tbls_v12d[id][lv];

		p->cpu_level = lv;

		aee_record_cpufreq_cb(7);
		ret = _mt_cpufreq_setup_freqs_table(policy,
						    opp_tbl_info->opp_tbl, opp_tbl_info->size);

		policy->cpuinfo.max_freq = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->cpuinfo.min_freq = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		policy->cur = mt_cpufreq_get_cur_phy_freq(id);	/* use cur phy freq is better */
		aee_record_cpufreq_cb(8);
		policy->max = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->min = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		if (_mt_cpufreq_sync_opp_tbl_idx(p) >= 0)
			if (p->idx_normal_max_opp == -1)
				p->idx_normal_max_opp = p->idx_opp_tbl;

		aee_record_cpufreq_cb(9);

		opp_tbl_m_info = &opp_tbls_m_v12d[id][lv];

		p->freq_tbl = opp_tbl_m_info->opp_tbl_m;

		cpufreq_lock();
		if (!dvfs_disable_flag) {
			p->armpll_is_available = 1;
			p->mt_policy = policy;
		} else {
			p->armpll_is_available = 0;
			p->mt_policy = NULL;
		}
		aee_record_cpufreq_cb(10);
#ifdef CONFIG_HYBRID_CPU_DVFS
		if (enable_cpuhvfs)
			cpuhvfs_notify_cluster_on(cpu_dvfs_to_cluster(p));
#endif
		cpufreq_unlock();
		aee_record_cpufreq_cb(11);
		if (init_cci_status == 0) {
			/* init cci freq idx */
			if (_mt_cpufreq_sync_opp_tbl_idx(p_cci) >= 0)
				if (p_cci->idx_normal_max_opp == -1)
					p_cci->idx_normal_max_opp = p_cci->idx_opp_tbl;

			opp_tbl_m_cci_info = &opp_tbls_m_v12d[id_cci][lv];

			p_cci->freq_tbl = opp_tbl_m_cci_info->opp_tbl_m;
			p_cci->mt_policy = NULL;
			p_cci->armpll_is_available = 1;
			init_cci_status = 1;
		}
		/* Sync min/max for PPM */
		/* cpufreq_update_policy(policy->cpu); */
	}

	if (ret)
		cpufreq_err("failed to setup frequency table\n");

	return ret;
}

static int _mt_cpufreq_exit(struct cpufreq_policy *policy)
{
	return 0;
}

static unsigned int _mt_cpufreq_get(unsigned int cpu)
{
	struct mt_cpu_dvfs *p;

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(cpu));
	if (!p)
		return 0;

	return cpu_dvfs_get_cur_freq(p);
}

#ifdef CONFIG_HYBRID_CPU_DVFS
#include <linux/syscore_ops.h>
#include <linux/smp.h>

static void __set_cpuhvfs_init_sta(struct init_sta *sta)
{
	int i;
	unsigned int volt;
	struct mt_cpu_dvfs *p;

	for (i = 0; i < NUM_CPU_CLUSTER; i++) {
		p = cluster_to_cpu_dvfs(i);

		_sync_opp_tbl_idx(p);	/* find OPP with current frequency */

		volt = get_cur_volt_extbuck(p);
		BUG_ON(volt < EXTBUCK_VAL_TO_VOLT(0));

		sta->opp[i] = p->idx_opp_tbl;
		sta->freq[i] = p->ops->get_cur_phy_freq(p);
		sta->volt[i] = VOLT_TO_EXTBUCK_VAL(volt);
		sta->ceiling[i] = opp_limit_to_ceiling(p->idx_opp_ppm_limit);
		sta->floor[i] = opp_limit_to_floor(p->idx_opp_ppm_base);
		sta->is_on[i] = (p->armpll_is_available ? true : false);
	}
}

#ifdef CPUHVFS_HW_GOVERNOR
static void notify_cpuhvfs_change_cb(struct dvfs_log *log_box, int num_log)
{
	int i, j, dlog = 0;
	unsigned int tf_sum, t_diff, avg_f, volt = 0;
	unsigned int old_f[NUM_PHY_CLUSTER] = { 0 };
	unsigned int new_f[NUM_PHY_CLUSTER] = { 0 };
	struct mt_cpu_dvfs *p;
	struct cpufreq_freqs freqs;

	cpufreq_lock();
	if (!enable_hw_gov || num_log < 1) {
		cpufreq_unlock();
		return;
	}

	for (i = 0; i < NUM_PHY_CLUSTER; i++) {
		p = cluster_to_cpu_dvfs(i);

		if (!p->armpll_is_available)
			continue;

		if (num_log == 1) {
			j = log_box[0].opp[i];
		} else {
			tf_sum = 0;
			for (j = num_log - 1; j >= 1; j--) {
				tf_sum += (log_box[j].time - log_box[j - 1].time) *
					  cpu_dvfs_get_freq_mhz(p, log_box[j - 1].opp[i]);
			}

			t_diff = log_box[num_log - 1].time - log_box[0].time;
			avg_f = tf_sum / t_diff;

			/* find OPP_F >= AVG_F from the lowest frequency OPP */
			for (j = p->nr_opp_tbl - 1; j >= 0; j--) {
				if (cpu_dvfs_get_freq_mhz(p, j) >= avg_f)
					break;
			}

			if (j < 0) {
				cpufreq_err("tf_sum = %u, t_diff = %u, avg_f = %u, max_f = %u\n",
					    tf_sum, t_diff, avg_f, cpu_dvfs_get_freq_mhz(p, 0));
				j = 0;
			}
		}

		old_f[i] = cpu_dvfs_get_freq_mhz(p, p->idx_opp_tbl);
		new_f[i] = cpu_dvfs_get_freq_mhz(p, j);

		if (j == p->idx_opp_tbl)
			continue;

		if ((p->idx_opp_tbl >= 4 && p->idx_opp_tbl < p->nr_opp_tbl) &&
		    (j >= 0 && j < 4))		/* log filter */
			dlog = 1;

		freqs.old = old_f[i] * 1000;
		freqs.new = new_f[i] * 1000;
		cpufreq_freq_transition_begin(p->mt_policy, &freqs);

		p->idx_opp_tbl = j;
		aee_record_freq_idx(p, p->idx_opp_tbl);

		volt = cpu_dvfs_get_cur_volt(p);
		aee_record_cpu_volt(p, volt);
		notify_cpu_volt_sampler(p, volt);

		cpufreq_freq_transition_end(p->mt_policy, &freqs, 0);
	}
	cpufreq_unlock();

	if (dlog)
		cpufreq_dbg("new_freq = (%u %u) <- (%u %u) @ %08x\n",
			    new_f[0], new_f[1], old_f[0], old_f[1], log_box[0].time);

	if (!p->dvfs_disable_by_suspend && volt != 0 /* OPP changed */)
		_kick_PBM_by_cpu(p);
}
#endif

static int _mt_cpufreq_syscore_suspend(void)
{
	if (enable_cpuhvfs)
		return cpuhvfs_dvfsp_suspend();

	return 0;
}

static void _mt_cpufreq_syscore_resume(void)
{
	if (enable_cpuhvfs) {
		int i;
		enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(smp_processor_id());
		struct init_sta sta;
		struct mt_cpu_dvfs *p;

		for (i = 0; i < NUM_CPU_CLUSTER; i++) {
			p = cluster_to_cpu_dvfs(i);

			sta.opp[i] = OPP_AT_SUSPEND;
			sta.freq[i] = p->ops->get_cur_phy_freq(p);
			sta.volt[i] = VOLT_AT_SUSPEND;
			sta.vsram[i] = VSRAM_AT_SUSPEND;
			sta.ceiling[i] = CEILING_AT_SUSPEND;
			sta.floor[i] = FLOOR_AT_SUSPEND;
		}

		cpuhvfs_dvfsp_resume(id_to_cluster(id), &sta);
	}
}

static struct syscore_ops _mt_cpufreq_syscore_ops = {
	.suspend	= _mt_cpufreq_syscore_suspend,
	.resume		= _mt_cpufreq_syscore_resume,
};
#endif	/* CONFIG_HYBRID_CPU_DVFS */

static struct freq_attr *_mt_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver _mt_cpufreq_driver = {
	.flags = CPUFREQ_ASYNC_NOTIFICATION,
	.verify = _mt_cpufreq_verify,
	.target = _mt_cpufreq_target,
	.init = _mt_cpufreq_init,
	.exit = _mt_cpufreq_exit,
	.get = _mt_cpufreq_get,
	.name = "mt-cpufreq",
	.attr = _mt_cpufreq_attr,
};

/*
 * Platform driver
 */
static int
_mt_cpufreq_pm_callback(struct notifier_block *nb,
		unsigned long action, void *ptr)
{
	struct mt_cpu_dvfs *p;
	int i;

	switch (action) {

	case PM_SUSPEND_PREPARE:
		cpufreq_ver("PM_SUSPEND_PREPARE\n");
		cpufreq_lock();
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;
			p->dvfs_disable_by_suspend = true;
		}
		cpufreq_unlock();
		break;
	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
		cpufreq_ver("PM_POST_SUSPEND\n");
		cpufreq_lock();
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;
			p->dvfs_disable_by_suspend = false;
		}
		cpufreq_unlock();
		break;
	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

void mt_cpufreq_eem_resume(void)
{
	struct mt_cpu_dvfs *p;
	int i;

	cpufreq_lock();
	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;
		p->dvfs_disable_by_suspend = false;
	}
	cpufreq_unlock();
}

static int _mt_cpufreq_suspend(struct device *dev)
{
#if 0
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		p->dvfs_disable_by_suspend = true;
	}
#endif
	return 0;
}

static int _mt_cpufreq_resume(struct device *dev)
{
#if 0
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		p->dvfs_disable_by_suspend = false;
	}
#endif
	return 0;
}

static int _mt_cpufreq_pdrv_probe(struct platform_device *pdev)
{
	/* For table preparing*/
	unsigned int lv = _mt_cpufreq_get_cpu_level();
	struct mt_cpu_dvfs *p;
	int i, j;
	struct opp_tbl_info *opp_tbl_info;
	struct cpufreq_frequency_table *table;

	_mt_cpufreq_aee_init();

	switch (lv) {
	case CPU_LEVEL_0:
		max_vsram_volt = MAX_VSRAM_VOLT_FY;
		break;
	case CPU_LEVEL_1:
		max_vsram_volt = MAX_VSRAM_VOLT_FY2;
		break;
	case CPU_LEVEL_2:
		max_vsram_volt = MAX_VSRAM_VOLT_FY3;
		break;
	}

	/* Prepare OPP table for PPM in probe to avoid nested lock */
	for_each_cpu_dvfs(j, p) {
		if (cpu_dvfs_is(p, MT_CPU_DVFS_LL)) {
			p->armpll_addr = (unsigned int *)ARMPLL_LL_CON1;
			p->ckdiv_addr = (unsigned int *)TOPCKGEN_CKDIV1_SML;
		} else if (cpu_dvfs_is(p, MT_CPU_DVFS_L)) {
			p->armpll_addr = (unsigned int *)ARMPLL_L_CON1;
			p->ckdiv_addr = (unsigned int *)TOPCKGEN_CKDIV1_BIG;
		} else {	/* CCI */
			p->armpll_addr = (unsigned int *)CCIPLL_CON1;
			p->ckdiv_addr = (unsigned int *)TOPCKGEN_CKDIV1_BUS;
		}

		opp_tbl_info = &opp_tbls_v12d[j][lv];

		if (!p->freq_tbl_for_cpufreq) {
			table = kzalloc((opp_tbl_info->size + 1) * sizeof(*table), GFP_KERNEL);

			if (!table)
				return -ENOMEM;

			for (i = 0; i < opp_tbl_info->size; i++) {
				table[i].driver_data = i;
				table[i].frequency = opp_tbl_info->opp_tbl[i].cpufreq_khz;
			}

			table[opp_tbl_info->size].driver_data = i;
			table[opp_tbl_info->size].frequency = CPUFREQ_TABLE_END;

			p->opp_tbl = opp_tbl_info->opp_tbl;
			p->nr_opp_tbl = opp_tbl_info->size;
			p->freq_tbl_for_cpufreq = table;

			_sync_opp_tbl_idx(p);
		}

		if (j != MT_CPU_DVFS_CCI)
			mt_ppm_set_dvfs_table(p->cpu_id, p->freq_tbl_for_cpufreq, opp_tbl_info->size,
					      opp_tbl_info->type);
	}

#ifdef CONFIG_HYBRID_CPU_DVFS	/* before cpufreq_register_driver */
	if (enable_cpuhvfs) {
		int r;
		struct init_sta sta;

		__set_cpuhvfs_init_sta(&sta);

		r = cpuhvfs_kick_dvfsp_to_run(&sta);
		if (!r) {
			for_each_cpu_dvfs(i, p) {
				p->ops->set_cur_freq = set_cur_freq_hybrid;
				p->ops->set_cur_volt = set_cur_volt_hybrid;
				if (enable_cpuhvfs == 1)
					p->ops->get_cur_volt = get_cur_volt_hybrid;
			}

			register_syscore_ops(&_mt_cpufreq_syscore_ops);

#ifdef CPUHVFS_HW_GOVERNOR
			cpuhvfs_register_dvfs_notify(notify_cpuhvfs_change_cb);
#endif
		} else {
			enable_cpuhvfs = 0;
			enable_hw_gov = 0;
		}
	}
#endif

	cpufreq_register_driver(&_mt_cpufreq_driver);
	register_hotcpu_notifier(&_mt_cpufreq_cpu_notifier);
	mt_ppm_register_client(PPM_CLIENT_DVFS, &ppm_limit_callback);

	pm_notifier(_mt_cpufreq_pm_callback, 0);

	return 0;
}

static int _mt_cpufreq_pdrv_remove(struct platform_device *pdev)
{
	unregister_hotcpu_notifier(&_mt_cpufreq_cpu_notifier);
	cpufreq_unregister_driver(&_mt_cpufreq_driver);

	return 0;
}

static const struct dev_pm_ops _mt_cpufreq_pm_ops = {
	.suspend = _mt_cpufreq_suspend,
	.resume = _mt_cpufreq_resume,
	.freeze = _mt_cpufreq_suspend,
	.thaw = _mt_cpufreq_resume,
	.restore = _mt_cpufreq_resume,
};

struct platform_device _mt_cpufreq_pdev = {
	.name = "mt-cpufreq",
	.id = -1,
};

static struct platform_driver _mt_cpufreq_pdrv = {
	.probe = _mt_cpufreq_pdrv_probe,
	.remove = _mt_cpufreq_pdrv_remove,
	.driver = {
		   .name = "mt-cpufreq",
		   .pm = &_mt_cpufreq_pm_ops,
		   .owner = THIS_MODULE,
		   },
};

/*
* PROC
*/
static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}

/* cpufreq_debug */
static int cpufreq_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "cpufreq debug (log level) = %d\n", func_lv_mask);

	return 0;
}

static ssize_t cpufreq_debug_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	unsigned int dbg_lv;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &dbg_lv);
	if (rc < 0)
		cpufreq_err("echo dbg_lv (dec) > /proc/cpufreq/cpufreq_debug\n");
	else
		func_lv_mask = dbg_lv;

	free_page((unsigned long)buf);
	return count;
}

static int cpufreq_power_mode_proc_show(struct seq_file *m, void *v)
{
	unsigned int mode = dvfs_power_mode;

	seq_printf(m, "%s\n", mode < NUM_PPB_POWER_MODE ? power_mode_str[mode] : "Unknown");

	return 0;
}

static ssize_t cpufreq_power_mode_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	unsigned int mode;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &mode) && mode < NUM_PPB_POWER_MODE) {
		dvfs_power_mode = mode;
#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(CPUHVFS_HW_GOVERNOR)
		if (mode == DEFAULT_MODE)
			cpuhvfs_set_power_mode(POWER_NORMAL);
		else if (mode == PERFORMANCE_MODE)
			cpuhvfs_set_power_mode(POWER_SPORTS);
#endif
		cpufreq_dbg("%s start\n", power_mode_str[mode]);
	} else {
		cpufreq_err("echo 0/1/2/3 > /proc/cpufreq/cpufreq_power_mode\n");
	}

	free_page((unsigned long)buf);
	return count;
}

static int ppb_hispeed_freq_proc_show(struct seq_file *m, void *v)
{
	int i;
	unsigned int idx;
	struct mt_cpu_dvfs *p = m->private;

	for (i = 0; i < NUM_PPB_POWER_MODE; i++) {
		idx = ppb_hispeed_freq[p->id][i];
		seq_printf(m, "<%d>%s: %u (%u)\n", i, power_mode_str[i], idx, cpu_dvfs_get_freq_by_idx(p, idx));
	}

	return 0;
}

static ssize_t ppb_hispeed_freq_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *tok;
	unsigned int mode, idx = UINT_MAX;
	struct mt_cpu_dvfs *p = PDE_DATA(file_inode(file));

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	tok = strsep(&buf, " ");
	if (!tok || kstrtouint(tok, 10, &mode) || mode >= NUM_PPB_POWER_MODE)
		goto OUT;

	tok = strsep(&buf, " ");
	if (!tok || kstrtouint(tok, 10, &idx) || idx >= p->nr_opp_tbl)
		goto OUT;

	ppb_hispeed_freq[p->id][mode] = idx;

OUT:
	if (idx >= p->nr_opp_tbl)
		cpufreq_err("echo [0~3] [0~15] > /proc/cpufreq/%s/ppb_hispeed_freq\n", p->name);

	free_page((unsigned long)buf);
	return count;
}

static int cpufreq_stress_test_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", do_dvfs_stress_test);

	return 0;
}

static ssize_t cpufreq_stress_test_proc_write(struct file *file, const char __user *buffer,
					      size_t count, loff_t *pos)
{
	unsigned int do_stress;
	int rc;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;
	rc = kstrtoint(buf, 10, &do_stress);
	if (rc < 0)
		cpufreq_err("echo 0/1 > /proc/cpufreq/cpufreq_stress_test\n");
	else
		do_dvfs_stress_test = do_stress;

	free_page((unsigned long)buf);
	return count;
}

#ifdef CONFIG_HYBRID_CPU_DVFS
static int __switch_hwgov_on_off(unsigned int state)
{
#ifdef CPUHVFS_HW_GOVERNOR
	int i, r;
	struct init_sta sta;
	struct mt_cpu_dvfs *p;

	if (state) {
		if (enable_hw_gov)
			return 0;
		if (!enable_cpuhvfs)
			return -EPERM;

		for (i = 0; i < NUM_CPU_CLUSTER; i++) {
			p = cluster_to_cpu_dvfs(i);
			sta.ceiling[i] = opp_limit_to_ceiling(p->idx_opp_ppm_limit);
			sta.floor[i] = opp_limit_to_floor(p->idx_opp_ppm_base);
		}

		r = cpuhvfs_enable_hw_governor(&sta);
		if (r)
			return r;

		enable_hw_gov = 1;
	} else {
		if (!enable_hw_gov)
			return 0;

		r = cpuhvfs_disable_hw_governor(&sta);
		if (r)
			return r;

		for (i = 0; i < NUM_CPU_CLUSTER; i++) {
			p = cluster_to_cpu_dvfs(i);
			p->idx_opp_tbl = sta.opp[i];	/* sync OPP back */
			aee_record_freq_idx(p, p->idx_opp_tbl);
		}
		enable_hw_gov = 0;
	}

	return 0;
#else
	return state ? -EPERM : 0;
#endif
}

static int __switch_cpuhvfs_on_off(unsigned int state)
{
	int i, r;
	struct init_sta sta;
	struct mt_cpu_dvfs *p;

	if (state) {
		if (enable_cpuhvfs)
			return 0;

		__set_cpuhvfs_init_sta(&sta);

		r = cpuhvfs_restart_dvfsp_running(&sta);
		if (r)
			return r;

		for_each_cpu_dvfs(i, p) {
			p->ops->set_cur_freq = set_cur_freq_hybrid;
			p->ops->set_cur_volt = set_cur_volt_hybrid;
			if (state == 1)
				p->ops->get_cur_volt = get_cur_volt_hybrid;
		}
		enable_cpuhvfs = (state == 1 ? 1 : 2);
	} else {
		if (!enable_cpuhvfs)
			return 0;

		r = __switch_hwgov_on_off(0);
		if (r)
			return r;

		r = cpuhvfs_stop_dvfsp_running();
		if (r)
			return r;

		for_each_cpu_dvfs(i, p) {
			p->ops->set_cur_freq = set_cur_freq;
			p->ops->set_cur_volt = set_cur_volt_extbuck;
			p->ops->get_cur_volt = get_cur_volt_extbuck;
		}
		enable_cpuhvfs = 0;
	}

	return 0;
}

static int enable_cpuhvfs_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%u\n", enable_cpuhvfs);

	return 0;
}

static ssize_t enable_cpuhvfs_proc_write(struct file *file, const char __user *ubuf, size_t count,
					 loff_t *ppos)
{
	int r;
	unsigned int val;

	r = kstrtouint_from_user(ubuf, count, 0, &val);
	if (r)
		return -EINVAL;

	cpufreq_lock();
	__switch_cpuhvfs_on_off(val);
	cpufreq_unlock();

	return count;
}

static int enable_hw_gov_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%u (en_map 0x%x)\n", enable_hw_gov, hwgov_en_map);

	return 0;
}

static ssize_t enable_hw_gov_proc_write(struct file *file, const char __user *ubuf, size_t count,
					loff_t *ppos)
{
	int r;
	unsigned int val;

	r = kstrtouint_from_user(ubuf, count, 0, &val);
	if (r)
		return -EINVAL;

	switch (val) {
	case HWGOV_EN_GAME:
		cpufreq_lock();
		hwgov_en_map |= val;
		if (!(hwgov_en_map & HWGOV_DIS_FORCE))
			__switch_hwgov_on_off(1);
		cpufreq_unlock();
		break;
	case 1:		/* force on */
		cpufreq_lock();
		hwgov_en_map = (hwgov_en_map & ~HWGOV_DIS_FORCE) | HWGOV_EN_FORCE;
		__switch_hwgov_on_off(1);
		cpufreq_unlock();
		break;

	case HWGOV_DIS_GAME:
		cpufreq_lock();
		hwgov_en_map &= ~(val >> 16);
		if (!(hwgov_en_map & HWGOV_EN_MASK))
			__switch_hwgov_on_off(0);
		cpufreq_unlock();
		break;
	case 0:		/* force off */
		cpufreq_lock();
		hwgov_en_map = (hwgov_en_map & ~HWGOV_EN_FORCE) | HWGOV_DIS_FORCE;
		__switch_hwgov_on_off(0);
		cpufreq_unlock();
		break;
	}

	return count;
}
#endif	/* CONFIG_HYBRID_CPU_DVFS */

/* cpufreq_oppidx */
static int cpufreq_oppidx_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = m->private;
	int j;

	seq_printf(m, "[%s/%u]\n", p->name, p->cpu_id);
	seq_printf(m, "cpufreq_oppidx = %d\n", p->idx_opp_tbl);

	for (j = 0; j < p->nr_opp_tbl; j++) {
		seq_printf(m, "\t%-2d (%u, %u [%u])\n",
			      j, cpu_dvfs_get_freq_by_idx(p, j),
			      cpu_dvfs_get_volt_by_idx(p, j),
			      cpu_dvfs_get_org_volt_by_idx(p, j));
	}

	return 0;
}

static ssize_t cpufreq_oppidx_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = PDE_DATA(file_inode(file));
	int oppidx;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &oppidx);
	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo oppidx > /proc/cpufreq/%s/cpufreq_oppidx\n", p->name);
	} else {
		if (oppidx >= 0 && oppidx < p->nr_opp_tbl) {
			p->dvfs_disable_by_procfs = true;
			/*_mt_cpufreq_set(cpu_dvfs_is(p, MT_CPU_DVFS_LL) ? MT_CPU_DVFS_LL : MT_CPU_DVFS_L, oppidx);*/

#if defined(CONFIG_HYBRID_CPU_DVFS) && defined(__TRIAL_RUN__)
			cpuhvfs_set_target_opp(cpu_dvfs_to_cluster(p), oppidx, NULL);
#endif
		} else {
			p->dvfs_disable_by_procfs = false;
			cpufreq_err("echo oppidx > /proc/cpufreq/%s/cpufreq_oppidx\n", p->name);
		}
	}

	free_page((unsigned long)buf);

	return count;
}

/* cpufreq_freq */
static int cpufreq_freq_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = m->private;

	seq_printf(m, "%d KHz\n", p->ops->get_cur_phy_freq(p));

	return 0;
}

static ssize_t cpufreq_freq_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = PDE_DATA(file_inode(file));
	unsigned int cur_freq;
	int freq, i, found = 0;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &freq);
	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo khz > /proc/cpufreq/%s/cpufreq_freq\n", p->name);
	} else {
		if (freq < CPUFREQ_LAST_FREQ_LEVEL) {
			if (freq != 0)
				cpufreq_err("frequency should higher than %dKHz!\n",
					    CPUFREQ_LAST_FREQ_LEVEL);

			p->dvfs_disable_by_procfs = false;
		} else {
			for (i = 0; i < p->nr_opp_tbl; i++) {
				if (freq == p->opp_tbl[i].cpufreq_khz) {
					found = 1;
					break;
				}
			}

			if (found == 1) {
				p->dvfs_disable_by_procfs = true;
				cpufreq_lock();
				cur_freq = p->ops->get_cur_phy_freq(p);

				if (p->armpll_is_available && freq != cur_freq) {
					p->ops->set_cur_freq(p, cur_freq, freq);
					p->idx_opp_tbl = i;
					aee_record_freq_idx(p, p->idx_opp_tbl);
				} else {
					found = 0;
				}
				cpufreq_unlock();

				if (!p->dvfs_disable_by_suspend && found)
					_kick_PBM_by_cpu(p);
			} else {
				p->dvfs_disable_by_procfs = false;
				cpufreq_err("frequency %dKHz! is not found in CPU opp table\n",
					    freq);
			}
		}
	}

	free_page((unsigned long)buf);

	return count;
}

/* cpufreq_volt */
static int cpufreq_volt_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = m->private;

	cpufreq_lock();

	if (cpu_dvfs_is_extbuck_valid()) {
		seq_printf(m, "Vproc: %d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */
		seq_printf(m, "Vsram: %d mv\n", p->ops->get_cur_vsram(p) / 100);	/* mv */
	} else
		seq_printf(m, "%d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */

	cpufreq_unlock();

	return 0;
}

static ssize_t cpufreq_volt_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = PDE_DATA(file_inode(file));
	int mv;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;
	rc = kstrtoint(buf, 10, &mv);
	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo mv > /proc/cpufreq/%s/cpufreq_volt\n", p->name);
	} else {
		p->dvfs_disable_by_procfs = true;
		cpufreq_lock();
		_set_cur_volt_locked(p, mv * 100);
		cpufreq_unlock();
	}

	free_page((unsigned long)buf);

	return count;
}

/* cpufreq_time_profile */
static int cpufreq_dvfs_time_profile_proc_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < NR_SET_V_F; i++)
		seq_printf(m, "max[%d] = %lld us\n", i, ktime_to_us(max[i]));

	return 0;
}

static ssize_t cpufreq_dvfs_time_profile_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *pos)
{
	unsigned int temp;
	int rc;
	int i;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &temp);
	if (rc < 0)
		cpufreq_err("echo 1 > /proc/cpufreq/cpufreq_dvfs_time_profile\n");
	else {
		if (temp == 1) {
			for (i = 0; i < NR_SET_V_F; i++)
				max[i].tv64 = 0;
		}
	}
	free_page((unsigned long)buf);

	return count;
}

#define PROC_FOPS_RW(name)						\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner          = THIS_MODULE,					\
	.open           = name ## _proc_open,				\
	.read           = seq_read,					\
	.llseek         = seq_lseek,					\
	.release        = single_release,				\
	.write          = name ## _proc_write,				\
}

#define PROC_FOPS_RO(name)						\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner          = THIS_MODULE,					\
	.open           = name ## _proc_open,				\
	.read           = seq_read,					\
	.llseek         = seq_lseek,					\
	.release        = single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(cpufreq_debug);
PROC_FOPS_RW(cpufreq_stress_test);
PROC_FOPS_RW(cpufreq_power_mode);
PROC_FOPS_RW(ppb_hispeed_freq);

#ifdef CONFIG_HYBRID_CPU_DVFS
PROC_FOPS_RW(enable_cpuhvfs);
PROC_FOPS_RW(enable_hw_gov);
#endif
PROC_FOPS_RW(cpufreq_oppidx);
PROC_FOPS_RW(cpufreq_freq);
PROC_FOPS_RW(cpufreq_volt);
PROC_FOPS_RW(cpufreq_dvfs_time_profile);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	struct proc_dir_entry *cpu_dir = NULL;
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);
	int i, j;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(cpufreq_debug),
		PROC_ENTRY(cpufreq_stress_test),
		PROC_ENTRY(cpufreq_power_mode),
#ifdef CONFIG_HYBRID_CPU_DVFS
		PROC_ENTRY(enable_cpuhvfs),
		PROC_ENTRY(enable_hw_gov),
#endif
		PROC_ENTRY(cpufreq_dvfs_time_profile),
	};

	const struct pentry cpu_entries[] = {
		PROC_ENTRY(cpufreq_oppidx),
		PROC_ENTRY(cpufreq_freq),
		PROC_ENTRY(cpufreq_volt),
		PROC_ENTRY(ppb_hispeed_freq),
	};

	dir = proc_mkdir("cpufreq", NULL);

	if (!dir) {
		cpufreq_err("fail to create /proc/cpufreq @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			cpufreq_err("%s(), create /proc/cpufreq/%s failed\n", __func__,
				    entries[i].name);
	}

	for_each_cpu_dvfs(j, p) {
		cpu_dir = proc_mkdir(p->name, dir);

		if (!cpu_dir) {
			cpufreq_err("fail to create /proc/cpufreq/%s @ %s()\n", p->name, __func__);
			return -ENOMEM;
		}

		for (i = 0; i < ARRAY_SIZE(cpu_entries); i++) {
			if (!proc_create_data
			    (cpu_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, cpu_dir,
			     cpu_entries[i].fops, p))
				cpufreq_err("%s(), create /proc/cpufreq/%s/%s failed\n", __func__,
					    p->name, cpu_entries[i].name);
		}
	}

	return 0;
}

static int mt_cpufreq_dts_map(void)
{
	struct device_node *node;

	/* apmixed */
	node = of_find_compatible_node(NULL, NULL, APMIXED_NODE);
	if (GEN_DB_ON(!node, "APMIXED Not Found"))
		return -ENODEV;

	apmixed_base = (unsigned long)of_iomap(node, 0);
	if (GEN_DB_ON(!apmixed_base, "APMIXED Map Failed"))
		return -ENOMEM;

	/* infracfg_ao */
	node = of_find_compatible_node(NULL, NULL, INFRACFG_AO_NODE);
	if (GEN_DB_ON(!node, "INFRACFG Not Found"))
		return -ENODEV;

	infracfg_ao_base = (unsigned long)of_iomap(node, 0);
	if (GEN_DB_ON(!infracfg_ao_base, "INFRACFG Map Failed"))
		return -ENOMEM;

	/* topckgen */
	node = of_find_compatible_node(NULL, NULL, TOPCKGEN_NODE);
	if (GEN_DB_ON(!node, "TOPCKGEN Not Found"))
		return -ENODEV;

	topckgen_base = (unsigned long)of_iomap(node, 0);
	if (GEN_DB_ON(!topckgen_base, "TOPCKGEN Map Failed"))
		return -ENOMEM;

	return 0;
}

/*
* Module driver
*/
static int __init _mt_cpufreq_pdrv_init(void)
{
	int ret;
	struct cpumask cpu_mask;
	unsigned int cluster_num;
	int i;

	ret = mt_cpufreq_dts_map();
	if (ret)
		goto out;

	cluster_num = (unsigned int)arch_get_nr_clusters();
	for (i = 0; i < cluster_num; i++) {
		arch_get_cluster_cpus(&cpu_mask, i);
		cpu_dvfs[i].cpu_id = cpumask_first(&cpu_mask);
		cpufreq_dbg("cluster_id = %d, cluster_cpuid = %d\n", i, cpu_dvfs[i].cpu_id);
	}

#ifdef CONFIG_HYBRID_CPU_DVFS	/* before platform_driver_register */
	ret = cpuhvfs_module_init();
	if (ret || dvfs_disable_flag) {
		enable_cpuhvfs = 0;
		enable_hw_gov = 0;
	}
#endif

	/* init proc */
	ret = _create_procfs();
	if (ret)
		goto out;

	/* register platform device/driver */
	ret = platform_device_register(&_mt_cpufreq_pdev);

	if (ret) {
		cpufreq_err("fail to register cpufreq device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&_mt_cpufreq_pdrv);

	if (ret) {
		cpufreq_err("fail to register cpufreq driver @ %s()\n", __func__);
		platform_device_unregister(&_mt_cpufreq_pdev);
	}

out:
	return ret;
}

static void __exit _mt_cpufreq_pdrv_exit(void)
{
	platform_driver_unregister(&_mt_cpufreq_pdrv);
	platform_device_unregister(&_mt_cpufreq_pdev);
}

late_initcall(_mt_cpufreq_pdrv_init);
module_exit(_mt_cpufreq_pdrv_exit);

MODULE_DESCRIPTION("MediaTek CPU DVFS Driver v0.4");

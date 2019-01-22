/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/**
 * @file    mt_ptp.c
 * @brief   Driver for EEM
 *
 */

#define __MTK_EEM_C__

/* Early porting use that avoid to cause compiler error */
/* #define EARLY_PORTING */
#define EARLY_PORTING_GPU
#define EARLY_PORTING_VCOREFS
#define EARLY_PORTING_PPM
/* #define EARLY_PORTING_CPUDVFS */
#define EARLY_PORTING_THERMAL
/* #define UPDATE_TO_UPOWER*/
#define ITurbo			(0)
#define EEM_ENABLE		(0)
#define DUMP_DATA_TO_DE	(0)

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	#define EEM_ENABLE_TINYSYS_SSPM (0)
#else
	#define EEM_ENABLE_TINYSYS_SSPM (0)
#endif
/*=============================================================
* Include files
*=============================================================
*/

/* system includes */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/time.h>
#include <linux/slab.h>
#ifdef __KERNEL__
#include <linux/topology.h>
#endif

/* project includes */
/*
*#include "mach/irqs.h"
*#include "mach/mt_freqhopping.h"
*#include "mach/mt_rtc_hw.h"
*#include "mach/mtk_rtc_hal.h"
*/


#ifdef CONFIG_OF
	#include <linux/cpu.h>
	#include <linux/of.h>
	#include <linux/of_irq.h>
	#include <linux/of_address.h>
	#include <linux/of_fdt.h>
	#include <mt-plat/aee.h>
	#if defined(CONFIG_MTK_CLKMGR)
		#include <mach/mt_clkmgr.h>
	#else
		#include <linux/clk.h>
	#endif
#endif

/* local includes (kernel-4.4)*/
#ifdef __KERNEL__
	#include <mt-plat/mtk_chip.h>
	#include <mt-plat/mtk_gpio.h>
	#include "mt-plat/upmu_common.h"
	#include "mach/mtk_freqhopping.h"
	#include "mt-plat/mt6799/include/mach/mtk_thermal.h"
	#include "mt-plat/mt6799/include/mach/mtk_ppm_api.h"
	#include "mt-plat/mt6799/include/mach/mtk_cpufreq_api.h"
	#include "mtk_eem.h"
	#include "mtk_defeem.h"
	#include "mtk_spm_vcore_dvfs.h"
	#include "mtk_gpufreq.h"
	#include <mt-plat/mtk_devinfo.h>
	#include <regulator/consumer.h>
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6335)
		#include "include/pmic_regulator.h"
		#include "mtk_pmic_regulator.h"
	#endif

	#ifdef UPDATE_TO_UPOWER
	#include "unified_power.h"
	#endif

	#if 0
	#include  "mt-plat/elbrus/include/mach/mt_cpufreq_api.h"
	#include "mach/mt_ppm_api.h"
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
	#include "../../../pmic/include/mt6313/mt6313.h"
	#endif
	#endif

#else
	#include "mach/mt_cpufreq.h"
	#include "mach/mt_ptp.h"
	#include "mach/mt_ptpslt.h"
	#include "mach/mt_defptp.h"
	#include "kernel2ctp.h"
	#include "ptp.h"
	#include "upmu_common.h"
	/* #include "mt6311.h" */
	#ifndef EARLY_PORTING
		#include "gpu_dvfs.h"
		#include "thermal.h"
		/* #include "gpio_const.h" */
	#endif
#endif

#if EEM_ENABLE_TINYSYS_SSPM
	#include "sspm_ipi.h"
	#include <sspm_reservedmem_define.h>
#endif /* if EEM_ENABLE_TINYSYS_SSPM */

/***********************************
 *
 ***********************************/
#if !(EEM_ENABLE_TINYSYS_SSPM)

#if EEM_ENABLE
static unsigned int ctrl_EEM_Enable = 1;
#else
static unsigned int ctrl_EEM_Enable;
#endif /* EEM_ENABLE */

/* 0 : signed off volt
 * 1 : voltage bin volt
 */
/* static unsigned int ctrl_VCORE_Volt_Enable;*/

#if ITurbo
unsigned int ctrl_ITurbo = 0, ITurboRun;
static unsigned int ITurbo_offset[16];
#endif /* ITurbo */

static int isGPUDCBDETOverflow, is2LDCBDETOverflow, isLDCBDETOverflow, isCCIDCBDETOverflow;
static unsigned int checkEfuse;
/* static unsigned int ateVer; */

/* Global variable for I DVFS use */
unsigned int infoIdvfs;
struct eem_det;
struct eem_ctrl;
/* for setting pmic pwm mode and auto mode */
static struct regulator *eem_regulator_proc1;
static struct regulator *eem_regulator_proc2;
static struct regulator *eem_regulator_gpu;
#if 0 /* no record table */
u32 *recordRef;
#endif

static void eem_set_eem_volt(struct eem_det *det);
static void eem_restore_eem_volt(struct eem_det *det);
static unsigned int mt_eem_update_vcore_volt(unsigned int index, unsigned int newVolt);

#endif /* if !EEM_ENABLE_TINYSYS_SSPM */

/* both legacy and sspm will use these variables */
#if (defined(__KERNEL__) && !defined(CONFIG_MTK_CLKMGR)) && !defined(EARLY_PORTING)
	struct clk *clk_thermal;
	struct clk *clk_mfg_main, *clk_mfg_async, *clk_mfg, *clk_mfg_core3, *clk_mfg_core2,
				*clk_mfg_core1, *clk_mfg_core0;
#endif

#define VCORE_NR_FREQ 4
#define E1
#ifdef E1
/* SOC Voltage (10uv)*/
unsigned int vcore_opp[VCORE_NR_FREQ][4] = {
	{85000, 102500, 100000, 97500},/* 80000 */
	{85000, 97500, 95000, 92500}, /* 80000 */
	{85000, 87500, 85000, 82500}, /* 70000 */
	{75000, 87500, 85000, 82500}, /* 65000 */
	/* {75000, 87500, 85000, 82500} *//* 60000 */
};
#else
unsigned int vcore_opp[VCORE_NR_FREQ][4] = {
	{80000, 102500, 100000, 97500},/* 80000 */
	{75000, 97500, 95000, 92500}, /* 80000 */
	{70000, 87500, 85000, 82500}, /* 70000 */
	{65000, 87500, 85000, 82500}, /* 65000 */
	/* {75000, 87500, 85000, 82500} *//* 60000 */
};
#endif

unsigned int eem_vcore[VCORE_NR_FREQ];
unsigned int eem_vcore_index[VCORE_NR_FREQ] = {0};

static int eem_log_en;
/* static unsigned int informGpuEEMisReady;*/

/* Global variable for slow idle*/
volatile unsigned int ptp_data[3] = {0, 0, 0};
/*=============================================================
* Macro definition
*=============================================================
*/

/*
 * CONFIG (SW related)
 */
#define CONFIG_EEM_SHOWLOG	(0) /* to filter function log */

#define EN_ISR_LOG		(1) /* to enable eem_isr_info() */

#define SET_PMIC_VOLT	(1) /* apply PMIC voltage */

#define LOG_INTERVAL	(2LL * NSEC_PER_SEC)
#define DVT				(0)
#define EEM_BANK_SOC	(0) /* if DVT, enable this bank! */
#define NR_HW_RES		(10) /* only read efuse for 5 banks */
#define EFUSE_API		(0) /* to use get_devinfo_with_index() */

#if DVT
	#define NR_FREQ 8 /* opp table size currently use */
	#define NR_FREQ_GPU 8
	#define NR_FREQ_CPU 8
#else /* for trial run */
	#define NR_FREQ 16 /* opp table size currently use */
	#define NR_FREQ_GPU 6
	#define NR_FREQ_CPU 16
#endif

/* #define BINLEVEL_BYEFUSE */ /* default binlevel = 0 */

/* #if !(EEM_ENABLE_TINYSYS_SSPM) */
/*
 * 100 us, This is the EEM Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL	0x514

/*
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */

/* 1mV=>10uV */
/* EEM domain pmic*/
#define EEM_V_BASE	(40000)
#define EEM_STEP	(625)
#define BIG_EEM_BASE	(60000)
#define BIG_EEM_STEP	(625)

/* SRAM */
#define SRAM_BASE	(45000)
#define SRAM_STEP	(625)
#define EEM_2_RMIC(value)	(((((value) * EEM_STEP) + EEM_V_BASE - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define PMIC_2_RMIC(det, value)	\
	(((((value) * det->pmic_step) + det->pmic_base - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define VOLT_2_RMIC(volt)	(((((volt) - SRAM_BASE) + SRAM_STEP - 1) / SRAM_STEP) + 3)

/* for voltage bin vcore dvfs */
#define VCORE_PMIC_BASE	(40000)
#define VCORE_PMIC_STEP	(625)
#define VOLT_2_VCORE_PMIC(volt) ((((volt) - VCORE_PMIC_BASE) + VCORE_PMIC_STEP - 1) / VCORE_PMIC_STEP)
#define VCORE_PMIC_2_VOLT(pmic)	(((pmic) * VCORE_PMIC_STEP) + VCORE_PMIC_BASE)

/* BIG CPU */
/* #define BIG_CPU_PMIC_BASE	(45000) */
/* #define BIG_CPU_PMIC_STEP	(625) */

/* CPU */
#define CPU_PMIC_BASE		(45000)
#define CPU_PMIC_STEP		(625)
#define MAX_ITURBO_OFFSET	(2000)

/* GPU */
#define GPU_PMIC_BASE		(45000)
#define GPU_PMIC_STEP		(625)

/* common part: for big, cci, LL, L*/
#define VBOOT_VAL		(75000) /* eem domain: 0x38, volt domain: 0.75.v */
#define VMAX_VAL		(100000) /* eem domain: 0x60, volt domain: 1v*/
#define VMIN_VAL		(75000)  /* eem domain: 0x38, volt domain: 0.75v*/
#define VCO_VAL			(0x38)
#define DTHI_VAL		(0x01)		/* positive */
#define DTLO_VAL		(0xfe)		/* negative (2's compliment) */
#define DETMAX_VAL		(0xffff)	/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL	(0x555555)
#define AGEM_VAL		(0x0)
#define DVTFIXED_VAL	(0x7)
#define DCCONFIG_VAL	(0x555555)

/* different for GPU */
#define VMAX_VAL_GPU		(97300)
#define VCO_VAL_GPU			(0x20)
#define DVTFIXED_VAL_GPU	(0x6)

/* different for L */
#define VMAX_VAL_L		(88000)

/* different for SOC */
#define VMAX_VAL_SOC	(97300)

/* use in base_ops_mon_mode */
#define MTS_VAL	(0x1fb)
#define BTS_VAL	(0x6d1)

/* After ATE program version 5 */
/* #define VCO_VAL_AFTER_5		0x04 */
/* #define VCO_VAL_AFTER_5_BIG	0x32 */

/* #define VMAX_SRAM			(115000) */
/* #define VMIN_SRAM			(80000) */

#if !(EEM_ENABLE_TINYSYS_SSPM)
#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_eem_aee_init(void)
{
	aee_rr_rec_ptp_vboot(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_temp(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_status(0xFF);
}
#endif /* if defined(CONFIG_EEM_AEE_RR_REC) */
#endif /* if !(EEM_ENABLE_TINYSYS_SSPM) */
/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))

/*
 * LOG
 */
#ifdef __KERNEL__
#define EEM_TAG     "[EEM] "
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define eem_emerg(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_alert(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_crit(fmt, args...)      pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(ANDROID_LOG_WARN, EEM_TAG, fmt, ##args)
	#define eem_notice(fmt, args...)    pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_debug(fmt, args...)     pr_debug(ANDROID_LOG_DEBUG, EEM_TAG, fmt, ##args)
#else
	#define eem_emerg(fmt, args...)     pr_emerg(EEM_TAG fmt, ##args)
	#define eem_alert(fmt, args...)     pr_alert(EEM_TAG fmt, ##args)
	#define eem_crit(fmt, args...)      pr_crit(EEM_TAG fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(EEM_TAG fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(EEM_TAG fmt, ##args)
	#define eem_notice(fmt, args...)    pr_notice(EEM_TAG fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(EEM_TAG fmt, ##args)
	#define eem_debug(fmt, args...)     pr_err(EEM_TAG fmt, ##args) /* pr_debug(EEM_TAG fmt, ##args) */
#endif

	#if EN_ISR_LOG /* For Interrupt use */
		#define eem_isr_info(fmt, args...)  eem_debug(fmt, ##args)
	#else
		#define eem_isr_info(fmt, args...)
	#endif
#endif

#define FUNC_LV_MODULE          BIT(0)  /* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)  /* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)  /* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)  /* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)  /* mt_cpufreq driver help function   */


#if CONFIG_EEM_SHOWLOG
	static unsigned int func_lv_mask = (
		FUNC_LV_MODULE |
		FUNC_LV_CPUFREQ |
		FUNC_LV_API |
		FUNC_LV_LOCAL |
		FUNC_LV_HELP
		);
	#define FUNC_ENTER(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug(">> %s()\n", __func__); } while (0)
	#define FUNC_EXIT(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug("<< %s():%d\n", __func__, __LINE__); } while (0)
#else
	#define FUNC_ENTER(lv)
	#define FUNC_EXIT(lv)
#endif /* CONFIG_CPU_DVFS_SHOWLOG */

#if (!EEM_ENABLE_TINYSYS_SSPM)
/* Get time stmp to known the time period */
static unsigned long long eem_pTime_us, eem_cTime_us, eem_diff_us;
#define TIME_TH_US 3000
#define EEM_IS_TOO_LONG()   \
	do {	\
		eem_diff_us = eem_cTime_us - eem_pTime_us;	\
		if (eem_diff_us > TIME_TH_US) {                \
			pr_warn(EEM_TAG "caller_addr %p: %llu us\n", __builtin_return_address(0), eem_diff_us); \
		} else if (eem_diff_us < 0) {	\
			pr_warn(EEM_TAG "E: misuse caller_addr %p\n", __builtin_return_address(0)); \
		}	\
	} while (0)
#endif /* if (!EEM_ENABLE_TINYSYS_SSPM) */
/*
 * REG ACCESS
 */
#ifdef __KERNEL__
	#define eem_read(addr)	__raw_readl((void __iomem *)(addr))/*DRV_Reg32(addr)*/
	#define eem_read_field(addr, range)	\
		((eem_read(addr) & BITMASK(range)) >> LSB(range))
	#define eem_write(addr, val)	mt_reg_sync_writel(val, addr)
#endif
/**
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define eem_write_field(addr, range, val)	\
	eem_write(addr, (eem_read(addr) & ~BITMASK(range)) | BITS(range, val))

/**
 * Helper macros
 */

/* EEM detector is disabled by who */
enum {
	BY_PROCFS	= BIT(0),
	BY_INIT_ERROR	= BIT(1),
	BY_MON_ERROR	= BIT(2),
	BY_PROCFS_INIT2 = BIT(3),
};

#ifdef CONFIG_OF
void __iomem *eem_base;
static u32 eem_irq_number;
#endif
/**
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) for (det = eem_detectors; det < (eem_detectors + ARRAY_SIZE(eem_detectors)); det++)

/**
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the eem_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = eem_detectors,				\
	     ctrl = id_to_eem_ctrl(det->ctrl_id);		\
	     det < (eem_detectors + ARRAY_SIZE(eem_detectors)); \
	     det++,						\
	     ctrl = id_to_eem_ctrl(det->ctrl_id))

/**
 * iterate over list of controllers
 * @pos:	the eem_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) for (ctrl = eem_ctrls; ctrl < (eem_ctrls + ARRAY_SIZE(eem_ctrls)); ctrl++)

/**
 * Given a eem_det * in eem_detectors. Return the id.
 * @det:	pointer to a eem_det in eem_detectors
 */
#define det_to_id(det)	((det) - &eem_detectors[0])

/**
 * Given a eem_ctrl * in eem_ctrls. Return the id.
 * @det:	pointer to a eem_ctrl in eem_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &eem_ctrls[0])

/**
 * Check if a detector has a feature
 * @det:	pointer to a eem_det to be check
 * @feature:	enum eem_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

/*=============================================================
* Global opp table
*=============================================================
*/
#if !(EEM_ENABLE_TINYSYS_SSPM)

#if DVT
	unsigned int freq[NR_FREQ] = {0x64, 0x59, 0x4D, 0x3D, 0x35, 0x2C, 0x1E, 0x0E};
#endif /* DVT */

#if 0 /* no record table */
/*
*[25:21] dcmdiv
*[20:12] DDS
*[11:7] clkdiv; ARM PLL Divider
*[6:4] postdiv
*[3:0] CF index

*[31:14] iDVFS %(/2)
*[13:7] Vsram pmic value
*[6:0] Vproc pmic value
*/
/*For DA9210 PMIC (0.3 + (x * 0.01)) */
unsigned int fyTbl[][8] = {
/* dcmdiv, DDS, clk_div, post_div, CF_index, iDVFS, Vsram, Vproc */
	{0x13, 0x0D6, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x5A},/* 1.391 (LL) */
	{0x13, 0x0CE, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x58},/* 1.339 */
	{0x12, 0x0C6, 0x8, 0x0, 0xA, 0x0, 0x0F, 0x55},/* 1.287 */
	{0x11, 0x0BC, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x53},/* 1.222 */
	{0x0F, 0x158, 0x8, 0x1, 0x8, 0x0, 0x0F, 0x4F},/* 1.118 */
	{0x0F, 0x148, 0x8, 0x1, 0x7, 0x0, 0x0E, 0x4D},/* 1.066 */
	{0x0D, 0x124, 0x8, 0x1, 0x6, 0x0, 0x0C, 0x48},/* 0.949 */
	{0x0C, 0x114, 0x8, 0x1, 0x6, 0x0, 0x0B, 0x46},/* 0.897 */
	{0x0B, 0x0F8, 0x8, 0x1, 0x5, 0x0, 0x0A, 0x43},/* 0.806 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x4, 0x0, 0x09, 0x40},/* 0.715 */
	{0x08, 0x0C0, 0x8, 0x1, 0x3, 0x0, 0x07, 0x3C},/* 0.624 */
	{0x07, 0x0AC, 0x8, 0x1, 0x2, 0x0, 0x07, 0x3A},/* 0.559 */
	{0x06, 0x128, 0xa, 0x1, 0x2, 0x0, 0x07, 0x37},/* 0.481 */
	{0x05, 0x100, 0xa, 0x1, 0x1, 0x0, 0x07, 0x35},/* 0.416 */
	{0x04, 0x0D0, 0xa, 0x1, 0x0, 0x0, 0x07, 0x32},/* 0.338 */
	{0x03, 0x110, 0xb, 0x1, 0x0, 0x0, 0x07, 0x30},/* 0.221 */

	{0x1A, 0x11C, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x5A},/* 1.846 (L) */
	{0x19, 0x112, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x58},/* 1.781 */
	{0x18, 0x106, 0x8, 0x0, 0xD, 0x0, 0x0F, 0x55},/* 1.703 */
	{0x17, 0x0FA, 0x8, 0x0, 0xC, 0x0, 0x0F, 0x53},/* 1.625 */
	{0x15, 0x0E6, 0x8, 0x0, 0xB, 0x0, 0x0F, 0x4F},/* 1.495 */
	{0x14, 0x0DA, 0x8, 0x0, 0xA, 0x0, 0x0E, 0x4D},/* 1.417 */
	{0x12, 0x0C4, 0x8, 0x0, 0x9, 0x0, 0x0C, 0x48},/* 1.274 */
	{0x11, 0x0BA, 0x8, 0x0, 0x8, 0x0, 0x0B, 0x46},/* 1.209 */
	{0x0F, 0x14C, 0x8, 0x1, 0x8, 0x0, 0x0A, 0x43},/* 1.079 */
	{0x0D, 0x128, 0x8, 0x1, 0x6, 0x0, 0x09, 0x40},/* 0.962 */
	{0x0B, 0x100, 0x8, 0x1, 0x5, 0x0, 0x07, 0x3C},/* 0.832 */
	{0x0A, 0x0E4, 0x8, 0x1, 0x4, 0x0, 0x07, 0x3A},/* 0.741 */
	{0x09, 0x0C8, 0x8, 0x1, 0x3, 0x0, 0x07, 0x37},/* 0.650 */
	{0x07, 0x0AC, 0x8, 0x1, 0x2, 0x0, 0x07, 0x35},/* 0.559 */
	{0x06, 0x120, 0xa, 0x1, 0x1, 0x0, 0x07, 0x32},/* 0.468 */
	{0x04, 0x0C8, 0xa, 0x1, 0x0, 0x0, 0x07, 0x30},/* 0.325 */

	{0x0D, 0x11C, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x5A},/* 0.923 (CCI) */
	{0x0C, 0x110, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x58},/* 0.884 */
	{0x0C, 0x108, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x55},/* 0.858 */
	{0x0B, 0x0FC, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x53},/* 0.819 */
	{0x0A, 0x0E8, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x4F},/* 0.754 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x0, 0x0, 0x0E, 0x4D},/* 0.715 */
	{0x09, 0x0C4, 0x8, 0x1, 0x0, 0x0, 0x0C, 0x48},/* 0.637 */
	{0x08, 0x0BC, 0x8, 0x1, 0x0, 0x0, 0x0B, 0x46},/* 0.611 */
	{0x07, 0x148, 0xa, 0x1, 0x0, 0x0, 0x0A, 0x43},/* 0.533 */
	{0x06, 0x128, 0xa, 0x1, 0x0, 0x0, 0x09, 0x40},/* 0.481 */
	{0x05, 0x100, 0xa, 0x1, 0x0, 0x0, 0x07, 0x3C},/* 0.416 */
	{0x05, 0x0E8, 0xa, 0x1, 0x0, 0x0, 0x07, 0x3A},/* 0.377 */
	{0x04, 0x0C8, 0xa, 0x1, 0x0, 0x0, 0x07, 0x37},/* 0.325 */
	{0x04, 0x0B0, 0xa, 0x1, 0x0, 0x0, 0x07, 0x35},/* 0.286 */
	{0x03, 0x120, 0xb, 0x1, 0x0, 0x0, 0x07, 0x32},/* 0.234 */
	{0x02, 0x0D0, 0xb, 0x1, 0x0, 0x0, 0x07, 0x30},/* 0.169 */

	{0x1F, 0x15A, 0x8, 0x0, 0xF, 0x2CFAE, 0x0F, 0x58},/* 2.249 (BIG) */
	{0x1F, 0x152, 0x8, 0x0, 0xF, 0x2BF0A, 0x0F, 0x57},/* 2.197 */
	{0x1F, 0x14E, 0x8, 0x0, 0xF, 0x2B6B8, 0x0F, 0x56},/* 2.171 */
	{0x1E, 0x146, 0x8, 0x0, 0xF, 0x2A614, 0x0F, 0x54},/* 2.119 */
	{0x1D, 0x142, 0x8, 0x0, 0xF, 0x29DC3, 0x0F, 0x53},/* 2.093 */
	{0x1C, 0x132, 0x8, 0x0, 0xF, 0x27C7B, 0x0F, 0x51},/* 1.989 */
	{0x19, 0x112, 0x8, 0x0, 0xF, 0x239EC, 0x0E, 0x4D},/* 1.781 */
	{0x17, 0x102, 0x8, 0x0, 0xD, 0x218A4, 0x0D, 0x4B},/* 1.677 */
	{0x15, 0x0E6, 0x8, 0x0, 0xB, 0x1DE66, 0x0B, 0x46},/* 1.495 */
	{0x13, 0x0D4, 0x8, 0x0, 0xA, 0x1B8F6, 0x0B, 0x44},/* 1.378 */
	{0x11, 0x0C0, 0x8, 0x0, 0x9, 0x18F5C, 0x09, 0x41},/* 1.248 */
	{0x10, 0x15C, 0x8, 0x1, 0x8, 0x169EC, 0x09, 0x3F},/* 1.131 */
	{0x0E, 0x134, 0x8, 0x1, 0x7, 0x14052, 0x07, 0x3C},/* 1.001 */
	{0x0C, 0x104, 0x8, 0x1, 0x6, 0x10E66, 0x07, 0x3A},/* 0.845 */
	{0x09, 0x0D0, 0x8, 0x1, 0x4, 0xD852, 0x07, 0x37},/* 0.676 */
	{0x04, 0x09C, 0x1a, 0x1, 0x0, 0x6C29, 0x07, 0x32},/* 0.338 */
};

unsigned int sbTbl[][8] = {
	{0x16, 0x0EE, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x5A},/* 1.547 (LL)*/
	{0x15, 0x0E6, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x59},/* 1.495 */
	{0x14, 0x0DE, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x57},/* 1.443 */
	{0x13, 0x0D6, 0x8, 0x0, 0x8, 0x0, 0x0F, 0x55},/* 1.391 */
	{0x13, 0x0CE, 0x8, 0x0, 0x7, 0x0, 0x0F, 0x54},/* 1.339 */
	{0x12, 0x0C4, 0x8, 0x0, 0x7, 0x0, 0x0F, 0x53},/* 1.274 */
	{0x11, 0x0BC, 0x8, 0x0, 0x6, 0x0, 0x0F, 0x50},/* 1.222 */
	{0x0F, 0x158, 0x8, 0x1, 0x6, 0x0, 0x0E, 0x4D},/* 1.118 */
	{0x0E, 0x138, 0x8, 0x1, 0x5, 0x0, 0x0D, 0x4A},/* 1.014 */
	{0x0C, 0x114, 0x8, 0x1, 0x4, 0x0, 0x0B, 0x46},/* 0.897 */
	{0x0B, 0x0F8, 0x8, 0x1, 0x3, 0x0, 0x0A, 0x43},/* 0.806 */
	{0x0A, 0x0DC, 0x8, 0x1, 0x3, 0x0, 0x09, 0x40},/* 0.715 */
	{0x08, 0x0C0, 0x8, 0x1, 0x2, 0x0, 0x07, 0x3C},/* 0.624 */
	{0x06, 0x128, 0xa, 0x1, 0x2, 0x0, 0x07, 0x37},/* 0.481 */
	{0x04, 0x0D0, 0xa, 0x1, 0x0, 0x0, 0x07, 0x32},/* 0.338 */
	{0x03, 0x110, 0xb, 0x1, 0x0, 0x0, 0x07, 0x2F},/* 0.221 */

	{0x1C, 0x134, 0x8, 0x0, 0xF, 0x0, 0x0F, 0x5A},/* 2.002 (L) */
	{0x1B, 0x12C, 0x8, 0x0, 0xE, 0x0, 0x0F, 0x59},/* 1.95  */
	{0x1A, 0x122, 0x8, 0x0, 0xE, 0x0, 0x0F, 0x57},/* 1.885 */
	{0x1A, 0x118, 0x8, 0x0, 0xC, 0x0, 0x0F, 0x55},/* 1.82  */
	{0x19, 0x10E, 0x8, 0x0, 0xB, 0x0, 0x0F, 0x54},/* 1.755 */
	{0x18, 0x106, 0x8, 0x0, 0xB, 0x0, 0x0F, 0x53},/* 1.703 */
	{0x17, 0x0FA, 0x8, 0x0, 0x9, 0x0, 0x0F, 0x50},/* 1.625 */
	{0x15, 0x0E6, 0x8, 0x0, 0x8, 0x0, 0x0E, 0x4D},/* 1.495 */
	{0x13, 0x0D0, 0x8, 0x0, 0x7, 0x0, 0x0D, 0x4A},/* 1.352 */
	{0x11, 0x0BA, 0x8, 0x0, 0x6, 0x0, 0x0B, 0x46},/* 1.209 */
	{0x0F, 0x14C, 0x8, 0x1, 0x6, 0x0, 0x0A, 0x43},/* 1.079 */
	{0x0D, 0x128, 0x8, 0x1, 0x4, 0x0, 0x09, 0x40},/* 0.962 */
	{0x0B, 0x100, 0x8, 0x1, 0x3, 0x0, 0x07, 0x3C},/* 0.832 */
	{0x09, 0x0C8, 0x8, 0x1, 0x2, 0x0, 0x07, 0x37},/* 0.65  */
	{0x06, 0x120, 0xa, 0x1, 0x1, 0x0, 0x07, 0x32},/* 0.468 */
	{0x04, 0x0C8, 0xa, 0x1, 0x0, 0x0, 0x07, 0x2F},/* 0.325 */

	{0x0E, 0x130, 0x8, 0x1, 0x0, 0x0, 0x0F,	0x5A},/* 0.988 (CCI) */
	{0x0D, 0x12C, 0x8, 0x1, 0x0, 0x0, 0x0F,	0x59},/* 0.975 */
	{0x0D, 0x120, 0x8, 0x1, 0x0, 0x0, 0x0F,	0x57},/* 0.936 */
	{0x0D, 0x118, 0x8, 0x1, 0x0, 0x0, 0x0F,	0x55},/* 0.91  */
	{0x0C, 0x110, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x54},/* 0.884 */
	{0x0C, 0x104, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x53},/* 0.845 */
	{0x0B, 0x0FC, 0x8, 0x1, 0x0, 0x0, 0x0F, 0x50},/* 0.819 */
	{0x0A, 0x0E8, 0x8, 0x1, 0x0, 0x0, 0x0E, 0x4D},/* 0.754 */
	{0x09, 0x0D0, 0x8, 0x1, 0x0, 0x0, 0x0D, 0x4A},/* 0.676 */
	{0x08, 0x0BC, 0x8, 0x1, 0x0, 0x0, 0x0B, 0x46},/* 0.611 */
	{0x07, 0x148, 0xa, 0x1, 0x0, 0x0, 0x0A, 0x43},/* 0.533 */
	{0x06, 0x128, 0xa, 0x1, 0x0, 0x0, 0x09, 0x40},/* 0.481 */
	{0x05, 0x100, 0xa, 0x1, 0x0, 0x0, 0x07, 0x3C},/* 0.416 */
	{0x04, 0x0C8, 0xa, 0x1, 0x0, 0x0, 0x07,	0x37},/* 0.325 */
	{0x03, 0x120, 0xb, 0x1, 0x0, 0x0, 0x07,	0x32},/* 0.234 */
	{0x02, 0x0D0, 0xb, 0x1, 0x0, 0x0, 0x07,	0x2F},/* 0.169 */

	{0x1F, 0x17A, 0x8, 0x0, 0xF, 0x3123D, 0x0F, 0x58},/* 2.457 (BIG) */
	{0x1F, 0x170, 0x8, 0x0, 0xF, 0x2FD71, 0x0F, 0x57},/* 2.392 */
	{0x1F, 0x166, 0x8, 0x0, 0xF, 0x2E8A4, 0x0F, 0x56},/* 2.327 */
	{0x1F, 0x15C, 0x8, 0x0, 0xF, 0x2D3D7, 0x0F, 0x55},/* 2.262 */
	{0x1F, 0x156, 0x8, 0x0, 0xF, 0x2C75C, 0x0F, 0x54},/* 2.223 */
	{0x1E, 0x14C, 0x8, 0x0, 0xF, 0x2B28F, 0x0F, 0x53},/* 2.158 */
	{0x1D, 0x142, 0x8, 0x0, 0xF, 0x29DC3, 0x0F, 0x51},/* 2.093 */
	{0x1A, 0x122, 0x8, 0x0, 0xE, 0x25B33, 0x0F, 0x4E},/* 1.885 */
	{0x17, 0x102, 0x8, 0x0, 0xA, 0x218A4, 0x0D, 0x4A},/* 1.677 */
	{0x15, 0x0E6, 0x8, 0x0, 0x8, 0x1DE66, 0x0B, 0x46},/* 1.495 */
	{0x13, 0x0D4, 0x8, 0x0, 0x8, 0x1B8F6, 0x0B, 0x44},/* 1.378 */
	{0x10, 0x15C, 0x8, 0x1, 0x6, 0x169EC, 0x09, 0x3F},/* 1.131 */
	{0x0E, 0x134, 0x8, 0x1, 0x5, 0x14052, 0x07, 0x3C},/* 1.001 */
	{0x0C, 0x104, 0x8, 0x1, 0x4, 0x10E66, 0x07, 0x3A},/* 0.845 */
	{0x09, 0x0D0, 0x8, 0x1, 0x3, 0xD852, 0x07, 0x37},/* 0.676 */
	{0x04, 0x09C, 0x1a, 0x1, 0x0, 0x6C29, 0x07, 0x32},/* 0.338 */
};
/* gpu */
unsigned int gpuFy[16] = {0x29, 0x29, 0x26, 0x26, 0x22, 0x22, 0x1F, 0x1F,
			0x1B, 0x1B, 0x18, 0x18, 0x14, 0x14, 0x10, 0x10};

unsigned int gpuSb[16] = {0x29, 0x29, 0x24, 0x24, 0x21, 0x21, 0x1E, 0x1E,
			0x1A, 0x1A, 0x18, 0x18, 0x14, 0x14, 0x10, 0x10};
#endif /*if 0*/

unsigned int gpuOutput[NR_FREQ]; /* (8)final volt table to gpu */
static unsigned int record_tbl_locked[NR_FREQ]; /* table used to apply to dvfs at final */

/*static unsigned int *recordTbl;*//* no record table */
/*static unsigned int *gpuTbl;*/ /* used to record gpu volt table */
#ifdef BINLEVEL_BYEFUSE
static unsigned int cpu_speed;
#endif

#else /* if EEM_ENABLE_TINYSYS_SSPM */
/* #define EEM_LOG_TIMER*/
#define NR_FREQ 8 /* for sspm struct eem_det */
phys_addr_t eem_log_phy_addr, eem_log_virt_addr;
uint32_t eem_log_size;

#endif /* EEM_ENABLE_TINYSYS_SSPM */

/*=============================================================
* Local type definition
*=============================================================
*/
/*
 * CONFIG (CHIP related)
 * EEMCORESEL.APBSEL
 */

enum eem_phase {
	EEM_PHASE_INIT01,
	EEM_PHASE_INIT02,
	EEM_PHASE_MON,

	NR_EEM_PHASE,
};

enum {
	EEM_VOLT_NONE    = 0,
	EEM_VOLT_UPDATE  = BIT(0),
	EEM_VOLT_RESTORE = BIT(1),
};

#if EEM_ENABLE_TINYSYS_SSPM
enum {
	IPI_EEM_INIT,
	IPI_EEM_PROBE,
	IPI_EEM_DEBUG_PROC_SHOW,
	IPI_EEM_DEBUG_PROC_WRITE,
	IPI_EEM_VCORE_INIT,
	IPI_EEM_VCORE_UPDATE_PROC_WRITE,
	IPI_EEM_CUR_VOLT_PROC_SHOW,

#ifdef EEM_LOG_TIMER
	IPI_EEM_LOGEN_PROC_SHOW,
	IPI_EEM_LOGEN_PROC_WRITE,
#endif
	/* IPI_EEM_VCORE_GET_VOLT,*/
	/* IPI_EEM_GPU_DVFS_GET_STATUS,*/
	/* IPI_EEM_OFFSET_PROC_SHOW, */
	/* IPI_EEM_OFFSET_PROC_WRITE, */
	NR_EEM_IPI,
};

struct eem_ipi_data {
	unsigned int cmd;
	union {
		struct {
			unsigned int arg[3];
		} data;
	} u;
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
struct eem_ctrl {
	const char *name;
	enum eem_det_id det_id;
	/* struct completion init_done; */
	/* atomic_t in_init; */
#ifdef __KERNEL__
	/* for voltage setting thread */
	wait_queue_head_t wq;
#endif
	int volt_update;
	struct task_struct *thread;
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
struct eem_det_ops {

	/* interface to EEM */
	void (*enable)(struct eem_det *det, int reason);
	void (*disable)(struct eem_det *det, int reason);
	void (*disable_locked)(struct eem_det *det, int reason);
	void (*switch_bank)(struct eem_det *det, enum eem_phase phase);

	int (*init01)(struct eem_det *det);
	int (*init02)(struct eem_det *det);
	int (*mon_mode)(struct eem_det *det);

	int (*get_status)(struct eem_det *det);
	void (*dump_status)(struct eem_det *det);

	void (*set_phase)(struct eem_det *det, enum eem_phase phase);

	/* interface to thermal */
	int (*get_temp)(struct eem_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct eem_det *det);
	int (*set_volt)(struct eem_det *det);
	void (*restore_default_volt)(struct eem_det *det);
	void (*get_freq_table)(struct eem_det *det);
	void (*get_orig_volt_table)(struct eem_det *det);

	/* interface to PMIC */
	int (*volt_2_pmic)(struct eem_det *det, int volt);
	int (*volt_2_eem)(struct eem_det *det, int volt);
	int (*pmic_2_volt)(struct eem_det *det, int pmic_val);
	int (*eem_2_pmic)(struct eem_det *det, int eev_val);
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */

enum eem_features {
	FEA_INIT01	= BIT(EEM_PHASE_INIT01),
	FEA_INIT02	= BIT(EEM_PHASE_INIT02),
	FEA_MON		= BIT(EEM_PHASE_MON),
};

#if !(EEM_ENABLE_TINYSYS_SSPM)
struct eem_det {
	const char *name;
	struct eem_det_ops *ops;
	int status;			/* TODO: enable/disable */
	int features;		/* enum eem_features */
	enum eem_ctrl_id ctrl_id;

	/* devinfo */
	unsigned int EEMINITEN;
	unsigned int EEMMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant */
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	/* Generated by EEM init01. Used in EEM init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for PMIC */
	unsigned int eem_v_base;
	unsigned int eem_step;
	unsigned int pmic_base;
	unsigned int pmic_step;

	/* for debug */
	unsigned int dcvalues[NR_EEM_PHASE];

	unsigned int freqpct30[NR_EEM_PHASE];
	unsigned int eem_26c[NR_EEM_PHASE];
	unsigned int vop30[NR_EEM_PHASE];
	unsigned int eem_eemEn[NR_EEM_PHASE];
	#if 0 /* no record table */
	u32 *recordRef;
	#endif
	#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_EEM_PHASE];
	#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;
	unsigned int t250;
	/* dvfs */
	unsigned int num_freq_tbl; /* could be got @ the same time with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency used to calculate percentage */
	unsigned char freq_tbl[NR_FREQ]; /* percentage to maximum freq */

	unsigned int volt_tbl_orig[NR_FREQ]; /* orig volt table for restoreing to dvfs*/
	unsigned int volt_tbl[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_init2[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_pmic[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_bin[NR_FREQ]; /* pmic value */
	int volt_offset;
	int pi_offset;

	unsigned int disabled; /* Disabled by error or sysfs */
	unsigned char set_volt_to_upower; /* only when init2, eem need to set volt to upower */
};
#else /* if has SSPM, strut eem_det just need to record dump data */

struct eem_det {
	const char *name;
	int status;			/* TODO: enable/disable */
	int features;		/* enum eem_features */
	enum eem_ctrl_id ctrl_id;

	/* for debug */
#if 0
	unsigned int dcvalues;

	unsigned int freqpct30;
	unsigned int eem_26c;
	unsigned int vop30;
	unsigned int eem_eemEn;
	u32 *recordRef;
	#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)];
	#endif
#endif
	/* dvfs */
	unsigned int num_freq_tbl; /* could be got @ the same time with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency used to calculate percentage */
	unsigned char freq_tbl[NR_FREQ]; /* percentage to maximum freq */

	unsigned char volt_tbl[NR_FREQ]; /* pmic value */
	unsigned char volt_tbl_init2[NR_FREQ]; /* pmic value */
	unsigned char volt_tbl_pmic[NR_FREQ]; /* pmic value */
	/* unsigned int volt_tbl_bin[NR_FREQ]; *//* pmic value */
	int volt_offset;
	int pi_offset;

	unsigned int disabled; /* Disabled by error or sysfs */
};

struct eem_log_det {
	enum eem_ctrl_id ctrl_id;
	unsigned char num_freq_tbl;
	unsigned int dcvalues[NR_EEM_PHASE];
	unsigned int freqpct30[NR_EEM_PHASE];
	unsigned int eem_26c[NR_EEM_PHASE];
	unsigned int vop30[NR_EEM_PHASE];
	unsigned int eem_eemEn[NR_EEM_PHASE];
	unsigned int temp;

	/* orig volt table for restoreing to dvfs*/
	unsigned char volt_tbl_orig[NR_FREQ];
	unsigned char volt_tbl_init2[NR_FREQ];
	unsigned char volt_tbl[NR_FREQ];
	unsigned char volt_tbl_pmic[NR_FREQ];
	unsigned char freq_tbl[NR_FREQ];
	unsigned char lock;
#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_EEM_PHASE];
#endif
};

struct eem_log {
	unsigned int hw_res[NR_HW_RES];
	unsigned int eem_vcore_pmic[VCORE_NR_FREQ];
	struct eem_log_det det_log[NR_EEM_DET];
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
struct eem_devinfo {
	/* M_HW_RES0 0x102A0580 */
	unsigned int BIG_BDES:8;
	unsigned int BIG_MDES:8;
	unsigned int BIG_DCBDET:8;
	unsigned int BIG_DCMDET:8;

	/* M_HW_RES1 0x102A0584 */
	unsigned int BIG_INITEN:1;
	unsigned int BIG_MONEN:1;
	unsigned int BIG_DVFS_LOW:2;
	unsigned int BIG_TURBO:1;
	unsigned int BIG_SPEC:3;
	unsigned int BIG_LEAKAGE:8;
	unsigned int BIG_MTDES:8;
	unsigned int BIG_AGEDELTA:8;

	/* M_HW_RES2 0x102A0588 */
	unsigned int CCI_BDES:8;
	unsigned int CCI_MDES:8;
	unsigned int CCI_DCBDET:8;
	unsigned int CCI_DCMDET:8;

	/* M_HW_RES3 0x102A058C */
	unsigned int CCI_INITEN:1;
	unsigned int CCI_MONEN:1;
	unsigned int CCI_DVFS_LOW:2;
	unsigned int CCI_TURBO:1;
	unsigned int CCI_SPEC:3;
	unsigned int CCI_LEAKAGE:8;
	unsigned int CCI_MTDES:8;
	unsigned int CCI_AGEDELTA:8;

	/* M_HW_RES4 0x102A0590 */
	unsigned int GPU_BDES:8;
	unsigned int GPU_MDES:8;
	unsigned int GPU_DCBDET:8;
	unsigned int GPU_DCMDET:8;

	/* M_HW_RES5 0x102A0594 */
	unsigned int GPU_INITEN:1;
	unsigned int GPU_MONEN:1;
	unsigned int GPU_DVFS_LOW:2;
	unsigned int GPU_TURBO:1;
	unsigned int GPU_SPEC:3;
	unsigned int GPU_LEAKAGE:8;
	unsigned int GPU_MTDES:8;
	unsigned int GPU_AGEDELTA:8;

	/* M_HW_RES6 0x102A0598 */
	unsigned int CPU_2L_BDES:8;
	unsigned int CPU_2L_MDES:8;
	unsigned int CPU_2L_DCBDET:8;
	unsigned int CPU_2L_DCMDET:8;

	/* M_HW_RES7 0x102A059C */
	unsigned int CPU_2L_INITEN:1;
	unsigned int CPU_2L_MONEN:1;
	unsigned int CPU_2L_DVFS_LOW:2;
	unsigned int CPU_2L_TURBO:1;
	unsigned int CPU_2L_SPEC:3;
	unsigned int CPU_2L_LEAKAGE:8;
	unsigned int CPU_2L_MTDES:8;
	unsigned int CPU_2L_AGEDELTA:8;

	/* M_HW_RES8 0x102A05A0 */
	unsigned int CPU_L_BDES:8;
	unsigned int CPU_L_MDES:8;
	unsigned int CPU_L_DCBDET:8;
	unsigned int CPU_L_DCMDET:8;

	/* M_HW_RES8 0x102A05A4 */
	unsigned int CPU_L_INITEN:1;
	unsigned int CPU_L_MONEN:1;
	unsigned int CPU_L_DVFS_LOW:2;
	unsigned int CPU_L_TURBO:1;
	unsigned int CPU_L_SPEC:3;
	unsigned int CPU_L_LEAKAGE:8;
	unsigned int CPU_L_MTDES:8;
	unsigned int CPU_L_AGEDELTA:8;
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */
/*=============================================================
*Local variable definition
*=============================================================
*/
static DEFINE_SPINLOCK(eem_spinlock);
#if !(EEM_ENABLE_TINYSYS_SSPM)
#ifdef __KERNEL__
/*
 * lock
 */
static DEFINE_MUTEX(record_mutex);

#if ITurbo/* irene */
/* CPU callback */
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,
					unsigned long action, void *hcpu);
static struct notifier_block __refdata _mt_eem_cpu_notifier = {
	.notifier_call = _mt_eem_cpu_CB,
};
#endif /* if ITurbo */

#endif /*ifdef __KERNEL */

#endif /* EEM_ENABLE_TINYSYS_SSPM */

/**
 * EEM controllers
 */
#if !(EEM_ENABLE_TINYSYS_SSPM)
struct eem_ctrl eem_ctrls[NR_EEM_CTRL] = {
	[EEM_CTRL_BIG] = {
		.name = __stringify(EEM_CTRL_BIG),
		.det_id = EEM_DET_BIG,
	},

	[EEM_CTRL_CCI] = {
		.name = __stringify(EEM_CTRL_CCI),
		.det_id = EEM_DET_CCI,
	},

	[EEM_CTRL_GPU] = {
		.name = __stringify(EEM_CTRL_GPU),
		.det_id = EEM_DET_GPU,
	},

	[EEM_CTRL_2L] = {
		.name = __stringify(EEM_CTRL_2L),
		.det_id = EEM_DET_2L,
	},

	[EEM_CTRL_L] = {
		.name = __stringify(EEM_CTRL_L),
		.det_id = EEM_DET_L,
	},
	#if EEM_BANK_SOC
	[EEM_CTRL_SOC] = {
		.name = __stringify(EEM_CTRL_SOC),
		.det_id = EEM_DET_SOC,
	}
	#endif

};
#endif /* if !EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
/*
 * operation of EEM detectors
 */
static void mt_ptp_lock(unsigned long *flags);
static void mt_ptp_unlock(unsigned long *flags);
static void base_ops_enable(struct eem_det *det, int reason);
static void base_ops_disable(struct eem_det *det, int reason);
static void base_ops_disable_locked(struct eem_det *det, int reason);
static void base_ops_switch_bank(struct eem_det *det, enum eem_phase phase);

static int base_ops_init01(struct eem_det *det);
static int base_ops_init02(struct eem_det *det);
static int base_ops_mon_mode(struct eem_det *det);

static int base_ops_get_status(struct eem_det *det);
static void base_ops_dump_status(struct eem_det *det);

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase);
static int base_ops_get_temp(struct eem_det *det);
static int base_ops_get_volt(struct eem_det *det);
static int base_ops_set_volt(struct eem_det *det);
static void base_ops_restore_default_volt(struct eem_det *det);
static void base_ops_get_freq_table(struct eem_det *det);
static void base_ops_get_orig_volt_table(struct eem_det *det);

static int base_ops_volt_2_pmic(struct eem_det *det, int volt); /* PMIC interface */
static int base_ops_volt_2_eem(struct eem_det *det, int volt);
static int base_ops_pmic_2_volt(struct eem_det *det, int pmic_val);
static int base_ops_eem_2_pmic(struct eem_det *det, int eev_val);

static int get_volt_cpu(struct eem_det *det);
static int set_volt_cpu(struct eem_det *det);
static void restore_default_volt_cpu(struct eem_det *det);
static void get_freq_table_cpu(struct eem_det *det);
static void get_orig_volt_table_cpu(struct eem_det *det);

static int get_volt_gpu(struct eem_det *det);
static int set_volt_gpu(struct eem_det *det);
static void restore_default_volt_gpu(struct eem_det *det);
static void get_freq_table_gpu(struct eem_det *det);
#if 0
static int get_volt_lte(struct eem_det *det);
static int set_volt_lte(struct eem_det *det);
static void restore_default_volt_lte(struct eem_det *det);
static int get_volt_vcore(struct eem_det *det);
#endif

#if EEM_BANK_SOC
static void get_freq_table_vcore(struct eem_det *det);
#endif

#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det);
#endif

/*static void switch_to_vcore_ao(struct eem_det *det);*/
/*static void switch_to_vcore_pdn(struct eem_det *det);*/

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct eem_det_ops eem_det_base_ops = {
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_table),
	BASE_OP(get_orig_volt_table),

	BASE_OP(volt_2_pmic),
	BASE_OP(volt_2_eem),
	BASE_OP(pmic_2_volt),
	BASE_OP(eem_2_pmic),
};

static struct eem_det_ops big_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
	.get_orig_volt_table = get_orig_volt_table_cpu,
};

static struct eem_det_ops gpu_det_ops = {
	.get_volt		= get_volt_gpu,
	.set_volt		= set_volt_gpu,
	.restore_default_volt	= restore_default_volt_gpu,
	.get_freq_table		= get_freq_table_gpu,
};

#if EEM_BANK_SOC
static struct eem_det_ops soc_det_ops = {
	.get_volt		= NULL,
	.set_volt		= NULL,
	.restore_default_volt	= NULL,
	.get_freq_table		= get_freq_table_vcore,
};
#endif

static struct eem_det_ops little_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
	.get_orig_volt_table = get_orig_volt_table_cpu,
};

static struct eem_det_ops dual_little_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
	.get_orig_volt_table = get_orig_volt_table_cpu,
};

static struct eem_det_ops cci_det_ops = {
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
	.get_orig_volt_table = get_orig_volt_table_cpu,
};

#if 0
static struct eem_det_ops lte_det_ops = {
	.get_volt		= get_volt_lte,
	.set_volt		= set_volt_lte,
	.restore_default_volt	= restore_default_volt_lte,
	.get_freq_table		= NULL,
};
#endif
#endif /* if !EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
static struct eem_det eem_detectors[NR_EEM_DET] = {
	[EEM_DET_BIG] = {
		.name		= __stringify(EEM_DET_BIG),
		.ops		= &big_det_ops,
		.ctrl_id	= EEM_CTRL_BIG,
#ifdef CONFIG_BIG_OFF
		.features	= 0,
#else
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
#endif
		.max_freq_khz	= 2548000,/* 2496Mhz */
		.VBOOT		= VBOOT_VAL, /* 10uV */
		.volt_offset	= 0,
		.eem_v_base	= BIG_EEM_BASE,
		.eem_step	= BIG_EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	[EEM_DET_CCI] = {
		.name		= __stringify(EEM_DET_CCI),
		.ops		= &cci_det_ops,
		.ctrl_id	= EEM_CTRL_CCI,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1391000,/* 1118 MHz */
		.VBOOT		= VBOOT_VAL, /* 1.0v: 0x30 */
		.volt_offset = 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	[EEM_DET_GPU] = {
		.name		= __stringify(EEM_DET_GPU),
		.ops		= &gpu_det_ops,
		.ctrl_id	= EEM_CTRL_GPU,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 850000,/* 850 MHz */
		.VBOOT		= VBOOT_VAL, /* 1.0006V: 100060 (Everest)  */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step	= EEM_STEP,
		.pmic_base	= GPU_PMIC_BASE,
		.pmic_step	= GPU_PMIC_STEP,
	},

	[EEM_DET_2L] = {
		.name		= __stringify(EEM_DET_2L),
		.ops		= &dual_little_det_ops,
		.ctrl_id	= EEM_CTRL_2L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1638000,/* 1599 MHz */
		.VBOOT		= VBOOT_VAL, /* 1.0v: 0x30 */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	[EEM_DET_L] = {
		.name		= __stringify(EEM_DET_L),
		.ops		= &little_det_ops,
		.ctrl_id	= EEM_CTRL_L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2340000,/* 2249 MHz */
		.VBOOT		= VBOOT_VAL, /* 1.0v: 0x30 */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
	},

	#if EEM_BANK_SOC
	[EEM_DET_SOC] = {
		.name		= __stringify(EEM_DET_SOC),
		.ops		= &soc_det_ops,
		.ctrl_id	= EEM_CTRL_SOC,
		.features	= FEA_INIT01 | FEA_INIT02,
		.max_freq_khz	= 100,/* 800 MHz */
		.VBOOT		= VBOOT_VAL, /* 1.0v */
		.volt_offset	= 0,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= SRAM_BASE,
		.pmic_step	= SRAM_STEP,
	}
	#endif

};
#else /*if EEM_ENABLE_TINYSYS_SSPM*/

static struct eem_det eem_detectors[NR_EEM_DET] = {
	[EEM_DET_BIG] = {
		.name		= __stringify(EEM_DET_BIG),
		.ctrl_id	= EEM_CTRL_BIG,
#ifdef CONFIG_BIG_OFF
		.features	= 0,
#else
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
#endif
		.max_freq_khz	= 2496000,/* 2496Mhz */
		.volt_offset	= 0,
	},

	[EEM_DET_CCI] = {
		.name		= __stringify(EEM_DET_CCI),
		.ctrl_id	= EEM_CTRL_CCI,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1118000,/* 1118 MHz */
		.volt_offset = 0,
	},

	[EEM_DET_GPU] = {
		.name		= __stringify(EEM_DET_GPU),
		.ctrl_id	= EEM_CTRL_GPU,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 850000,/* 850 MHz */
		.volt_offset	= 0,
	},

	[EEM_DET_2L] = {
		.name		= __stringify(EEM_DET_2L),
		.ctrl_id	= EEM_CTRL_2L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1599000,/* 1599 MHz */
		.volt_offset	= 0,
	},

	[EEM_DET_L] = {
		.name		= __stringify(EEM_DET_L),
		.ctrl_id	= EEM_CTRL_L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2249000,/* 2249 MHz */
		.volt_offset	= 0,
	},

	#if EEM_BANK_SOC
	[EEM_DET_SOC] = {
		.name		= __stringify(EEM_DET_SOC),
		.ctrl_id	= EEM_CTRL_SOC,
		.features	= FEA_INIT01 | FEA_INIT02,
		.max_freq_khz	= 800000,/* 800 MHz */
		.volt_offset	= 0,
	}
	#endif
};
#endif /* EEM_ENABLE_TINYSYS_SSPM */

#if !(EEM_ENABLE_TINYSYS_SSPM)
static struct eem_devinfo eem_devinfo;

#ifdef __KERNEL__
/**
 * timer for log
 */
static struct hrtimer eem_log_timer;
#endif
#endif /* EEM_ENABLE_TINYSYS_SSPM */
/*=============================================================
* Local function definition
*=============================================================
*/
#if !(EEM_ENABLE_TINYSYS_SSPM)
#ifndef EARLY_PORTING_THERMAL
int __attribute__((weak))
tscpu_get_temp_by_bank(thermal_bank_name ts_bank)
{
	eem_error("cannot find %s (thermal has not ready yet!)\n", __func__);
	return 0;
}
#endif

static struct eem_det *id_to_eem_det(enum eem_det_id id)
{
	if (likely(id < NR_EEM_DET))
		return &eem_detectors[id];
	else
		return NULL;
}

static struct eem_ctrl *id_to_eem_ctrl(enum eem_ctrl_id id)
{
	if (likely(id < NR_EEM_CTRL))
		return &eem_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct eem_det *det, int reason)
{
	/* FIXME: UNDER CONSTRUCTION */
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct eem_det *det, enum eem_phase phase)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/* 803f0000 + det->ctrl_id = enable ctrl's swcg clock */
	/* 003f0000 + det->ctrl_id = disable ctrl's swcg clock */
	if (phase == EEM_PHASE_INIT01)
		eem_write(EEMCORESEL, ((eem_read(EEMCORESEL) & ~BITMASK(2:0)) |
								BITS(2:0, det->ctrl_id) | 0x803f0000));
	else
		eem_write(EEMCORESEL, (((eem_read(EEMCORESEL) & ~BITMASK(2:0)) |
								BITS(2:0, det->ctrl_id)) & 0x0fffffff));

	/* eem_write_field(EEMCORESEL, 2:0, det->ctrl_id);*/
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct eem_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	switch (reason) {
	case BY_MON_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS_INIT2: /* 2 */
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		#ifdef UPDATE_TO_UPOWER
		det->set_volt_to_upower = 0;
		#endif
		eem_set_eem_volt(det);
		det->disabled |= reason;
		eem_debug("det->disabled=%x", det->disabled);
		break;

	case BY_INIT_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS: /* 1 */
		det->disabled |= reason;
		eem_debug("det->disabled=%x", det->disabled);
		/* restore default DVFS table (PMIC) */
		eem_restore_eem_volt(det);
		break;

	default:
		eem_debug("det->disabled=%x", det->disabled);
		det->disabled &= ~BY_PROCFS;
		det->disabled &= ~BY_PROCFS_INIT2;
		eem_debug("det->disabled=%x", det->disabled);
		eem_set_eem_volt(det);
		break;
	}

	eem_debug("Disable EEM[%s] done. reason=[%d]\n", det->name, det->disabled);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct eem_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det, NR_EEM_PHASE);
	det->ops->disable_locked(det, reason);
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct eem_det *det)
{
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		eem_debug("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	#if 0
	if (det->disabled & BY_PROCFS) {
		eem_debug("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	#endif

	/* atomic_inc(&ctrl->in_init); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		eem_debug("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_INIT_ERROR) {
		eem_error("[%s] Disabled by INIT_ERROR\n", ((char *)(det->name) + 8));
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	eem_debug("DCV = 0x%08X, AGEV = 0x%08X\n", det->DCVOFFSETIN, det->AGEVOFFSETIN);

	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct eem_det *det)
{
	#ifndef EARLY_PORTING_THERMAL
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;
	#endif

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		eem_debug("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_INIT_ERROR) {
		eem_error("[%s] Disabled BY_INIT_ERROR\n", ((char *)(det->name) + 8));
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	#ifndef EARLY_PORTING_THERMAL
		#if DVT
		det->MTS = MTS_VAL;
		det->BTS = BTS_VAL;
		#else
		ts_bank = det->ctrl_id;
		get_thermal_slope_intercept(&ts_info, ts_bank);
		det->MTS = ts_info.ts_MTS;
		det->BTS = ts_info.ts_BTS;
		#endif
	#else
		det->MTS =  MTS_VAL; /* orig: 0x2cf, (2048 * TS_SLOPE) + 2048; */
		det->BTS =  BTS_VAL; /* orig: 0x80E, 4 * TS_INTERCEPT; */
	#endif

	eem_debug("[base_ops_mon_mode] Bk = %d, MTS = %d, BTS = %d\n",
				det->ctrl_id, det->MTS, det->BTS);
	#if 0
	if ((det->EEMINITEN == 0x0) || (det->EEMMONEN == 0x0)) {
		eem_error("EEMINITEN = 0x%08X, EEMMONEN = 0x%08X\n", det->EEMINITEN, det->EEMMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}
	#endif
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct eem_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det, NR_EEM_PHASE);
	status = (eem_read(EEMEN) != 0) ? 1 : 0;
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct eem_det *det)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_HELP);

	eem_isr_info("[%s]\n",			det->name);

	eem_isr_info("EEMINITEN = 0x%08X\n",	det->EEMINITEN);
	eem_isr_info("EEMMONEN = 0x%08X\n",	det->EEMMONEN);
	eem_isr_info("MDES = 0x%08X\n",		det->MDES);
	eem_isr_info("BDES = 0x%08X\n",		det->BDES);
	eem_isr_info("DCMDET = 0x%08X\n",	det->DCMDET);

	eem_isr_info("DCCONFIG = 0x%08X\n",	det->DCCONFIG);
	eem_isr_info("DCBDET = 0x%08X\n",	det->DCBDET);

	eem_isr_info("AGECONFIG = 0x%08X\n",	det->AGECONFIG);
	eem_isr_info("AGEM = 0x%08X\n",		det->AGEM);

	eem_isr_info("AGEDELTA = 0x%08X\n",	det->AGEDELTA);
	eem_isr_info("DVTFIXED = 0x%08X\n",	det->DVTFIXED);
	eem_isr_info("MTDES = 0x%08X\n",	det->MTDES);
	eem_isr_info("VCO = 0x%08X\n",		det->VCO);

	eem_isr_info("DETWINDOW = 0x%08X\n",	det->DETWINDOW);
	eem_isr_info("VMAX = 0x%08X\n",		det->VMAX);
	eem_isr_info("VMIN = 0x%08X\n",		det->VMIN);
	eem_isr_info("DTHI = 0x%08X\n",		det->DTHI);
	eem_isr_info("DTLO = 0x%08X\n",		det->DTLO);
	eem_isr_info("VBOOT = 0x%08X\n",	det->VBOOT);
	eem_isr_info("DETMAX = 0x%08X\n",	det->DETMAX);

	eem_isr_info("DCVOFFSETIN = 0x%08X\n",	det->DCVOFFSETIN);
	eem_isr_info("AGEVOFFSETIN = 0x%08X\n",	det->AGEVOFFSETIN);

	eem_isr_info("MTS = 0x%08X\n",		det->MTS);
	eem_isr_info("BTS = 0x%08X\n",		det->BTS);

	eem_isr_info("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("freq_tbl[%d] = %d\n", i, det->freq_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_init2[%d] = %d\n", i, det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase)
{
	unsigned int i, filter, val;
	/* unsigned long flags; */

	FUNC_ENTER(FUNC_LV_HELP);

	/* mt_ptp_lock(&flags); */

	det->ops->switch_bank(det, phase);

	/* config EEM register */
	eem_write(EEM_DESCHAR,
		  ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	eem_write(EEM_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	eem_write(EEM_DETCHAR,
		  ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	eem_write(EEM_AGECHAR,
		  ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	eem_write(EEM_DCCONFIG, det->DCCONFIG);
	eem_write(EEM_AGECONFIG, det->AGECONFIG);

	if (phase == EEM_PHASE_MON)
		eem_write(EEM_TSCALCS,
			  ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		eem_write(EEM_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		eem_write(EEM_RUNCONFIG, val);
	}

	eem_write(EEM_FREQPCT30,
		  ((det->freq_tbl[3 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[2 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[1 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  (det->freq_tbl[0] & 0xff));
	eem_write(EEM_FREQPCT74,
		  ((det->freq_tbl[7 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[6 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[5 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  ((det->freq_tbl[4 * (NR_FREQ / 8)]) & 0xff));

	eem_write(EEM_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000)	|
		  ((det->VMIN << 16) & 0xff0000)	|
		  ((det->DTHI << 8) & 0xff00)		|
		  (det->DTLO & 0xff));
	/* eem_write(EEM_LIMITVALS, 0xFF0001FE); */
	eem_write(EEM_VBOOT, (((det->VBOOT) & 0xff)));
	eem_write(EEM_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	eem_write(EEMCONFIG, (((det->DETMAX) & 0xffff)));

	/* eem ctrl choose thermal sensors */
	switch (det->ctrl_id) {
	case EEM_CTRL_BIG:
		eem_write(EEM_CTL0, (0x1 | (0 << 16)));
		break;
	case EEM_CTRL_CCI:
		eem_write(EEM_CTL0, (0x7 | (1 << 24) | (0 << 20) | (4 << 16)));
		break;
	case EEM_CTRL_GPU:
		eem_write(EEM_CTL0, (0x1 | (5 << 16)));
		break;
	case EEM_CTRL_2L:
		eem_write(EEM_CTL0, (0x1 | (4 << 16)));
		break;
	case EEM_CTRL_L:
		eem_write(EEM_CTL0, (0x1 | (1 << 16)));
		break;
	case EEM_CTRL_SOC:
		eem_write(EEM_CTL0, (0x3 | (8 << 20) | (6 << 16)));
		break;
	default:
		eem_error("unknown ctrl try to set therm sensors\n");
		break;
	}
	/* clear all pending EEM interrupt & config EEMINTEN */
	eem_write(EEMINTSTS, 0xffffffff);

#if 0
	/*fix DCBDET overflow issue */
	if (((det_to_id(det) == EEM_DET_GPU) && isGPUDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_2L) && is2LDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_L) && isLDCBDETOverflow) ||
		((det_to_id(det) == EEM_DET_CCI) && isCCIDCBDETOverflow) ||
		(ateVer > 5)) {
		eem_write(EEM_CHKSHIFT, (eem_read(EEM_CHKSHIFT) & ~0x0F) | 0x07); /* 0x07 = DCBDETOFF */
	}
#endif

	eem_debug(" %s set phase = %d\n", ((char *)(det->name) + 8), phase);
	switch (phase) {
	case EEM_PHASE_INIT01:
		eem_write(EEMINTEN, 0x00005f01);
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000001);
		break;

	case EEM_PHASE_INIT02:
		eem_write(EEMINTEN, 0x00005f01);
		eem_write(EEM_INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			  (det->DCVOFFSETIN & 0xffff));
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000005);
		break;

	case EEM_PHASE_MON:
		eem_write(EEMINTEN, 0x00FF0000);
		/* enable EEM monitor mode */
		eem_write(EEMEN, 0x00000002);
		break;

	default:
		WARN_ON(1); /*BUG()*/
		break;
	}

	#if defined(__MTK_SLT_)
		mdelay(200);
	#endif
	/* mt_ptp_unlock(&flags); */

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct eem_det *det)
{
#if defined(CONFIG_THERMAL) && !defined(EARLY_PORTING_THERMAL)
	thermal_bank_name ts_bank;
	ts_bank = det_to_id(det);
	#ifdef __KERNEL__
		return tscpu_get_temp_by_bank(ts_bank);
	#else
		/*
		*thermal_init();
		*udelay(1000);
		*/
		return mtktscpu_get_hw_temp();
	#endif

#else
	return 0;
#endif
}

static int base_ops_get_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_table(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_tbl[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_orig_volt_table(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

/* for ISL191302 */
static int base_ops_volt_2_pmic(struct eem_det *det, int volt)
{
#if 0
	if (det_to_id(det) == EEM_DET_SOC)
		eem_debug("[%s][%s] volt = %d, pmic = %x\n",
			__func__,
			((char *)(det->name) + 8),
			volt,
			(((volt) - det->pmic_base + det->pmic_step - 1) / det->pmic_step)
			);
	else
		eem_debug("[%s][%s] volt = %d, pmic = %x\n",
			__func__,
			((char *)(det->name) + 8),
			volt,
			(((volt * 1024) + (1231000 - 1)) / 1231000);
			);
#endif
	if (det_to_id(det) == EEM_DET_SOC)
		return (((volt) - det->pmic_base + det->pmic_step - 1) / det->pmic_step);
	else
		return (((volt * 1024) + (1231000 - 1)) / 1231000);
}

static int base_ops_volt_2_eem(struct eem_det *det, int volt)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/*
	*eem_debug("[%s][%s] volt = %d, eem = %x\n",
	*	__func__,
	*	((char *)(det->name) + 8),
	*	volt,
	*	(((volt) - det->eem_v_base + det->eem_step - 1) / det->eem_step)
	*	);
	*/
	FUNC_EXIT(FUNC_LV_HELP);
	return (((volt) - det->eem_v_base + det->eem_step - 1) / det->eem_step);

}

/* for ISL191302 */
static int base_ops_pmic_2_volt(struct eem_det *det, int pmic_val)
{
#if 0
	if (det_to_id(det) == EEM_DET_SOC)
		eem_debug("[%s][%s] pmic = %x, volt = %d\n",
			__func__,
			((char *)(det->name) + 8),
			pmic_val,
			(((pmic_val) * det->pmic_step) + det->pmic_base),
			);
	else
		eem_debug("[%s][%s] pmic = %x, volt = %d\n",
			__func__,
			((char *)(det->name) + 8),
			pmic_val,
			(((pmic_val * 1231000) + (1024 - 1)) / 1024);
			);
#endif
	if (det_to_id(det) == EEM_DET_SOC)
		return (((pmic_val) * det->pmic_step) + det->pmic_base);
	else
		return (((pmic_val * 1231000) + (1024 - 1)) / 1024);
}

static int base_ops_eem_2_pmic(struct eem_det *det, int eem_val)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	if (det_to_id(det) == EEM_DET_SOC)
		return (
				(((eem_val) * det->eem_step) + det->eem_v_base -
				det->pmic_base + det->pmic_step - 1
				) / det->pmic_step);
	else
		return (
				((((eem_val) * det->eem_step) + det->eem_v_base) *
				1024 + (1231000 - 1)
				) / 1231000);
}
#if 0 /* no record table */
static void record(struct eem_det *det)
{
	int i;
	unsigned int vSram, vWrite;

	FUNC_ENTER(FUNC_LV_HELP);
	#ifdef EARLY_PORTING
	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vSram = clamp(
				(unsigned int)(record_tbl_locked[i] + 0xA),
				(unsigned int)(det->ops->volt_2_pmic(det, VMIN_SRAM)),
				(unsigned int)(det->ops->volt_2_pmic(det, VMAX_SRAM)));

		vWrite = det->recordRef[i*2];
		vWrite = (vWrite & (~0x3FFF)) |
			((((PMIC_2_RMIC(det, vSram) & 0x7F) << 7) | (record_tbl_locked[i] & 0x7F)) & 0x3fff);

		det->recordRef[i*2] = vWrite;
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
	#endif
	FUNC_EXIT(FUNC_LV_HELP);

}

static void restore_record(struct eem_det *det)
{

	int i;
	unsigned int vWrite;

	FUNC_ENTER(FUNC_LV_HELP);

	#ifdef EARLY_PORTING
	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vWrite = det->recordRef[i*2];
		if (det_to_id(det) == EEM_DET_2L)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i * 8) + 6) & 0x7F) << 7) |
				(*(recordTbl + (i * 8) + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_L)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 16) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 16) * 8 + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_CCI)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 32) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 32) * 8 + 7) & 0x7F));
		else if (det_to_id(det) == EEM_DET_BIG)
			vWrite = (vWrite & (~0x3FFF)) |
				(((*(recordTbl + (i + 48) * 8 + 6) & 0x7F) << 7) |
				(*(recordTbl + (i + 48) * 8 + 7) & 0x7F));
		det->recordRef[i*2] = vWrite;
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
	#endif
}
#endif /* no record table */

/* Will return 10uV */
static int get_volt_cpu(struct eem_det *det)
{
	unsigned int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	#if 0
	/* unit mv * 100 = 10uv */  /* I-Chang */
	switch (det_to_id(det)) {
	case EEM_DET_BIG:
		value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_B);
		break;

	case EEM_DET_L:
		value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L);
		break;

	case EEM_DET_2L:
		value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL);
		break;

	case EEM_DET_CCI:
		value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_CCI);
		break;

	default:
		value = 0;
		break;
	}
	#endif
	#if 0
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
	if (det_to_id(det) == EEM_DET_L)
		value = mt6313_get_voltage(VDVFS13)/10; /* L */
	else
		value = mt6313_get_voltage(VDVFS11)/10; /* B/LL/CCI*/
	#endif
	#endif

	/* return voltage in uV, so transfter to 10uV by /10 */
	if (det_to_id(det) == EEM_DET_L)
		value = regulator_get_voltage(eem_regulator_proc2)/10; /* L */
	else
		value = regulator_get_voltage(eem_regulator_proc1)/10; /* B/LL/CCI*/

	FUNC_EXIT(FUNC_LV_HELP);
	return value;
}
#if 0 /* no record table*/
static void mt_cpufreq_set_ptbl_funcEEM(enum mt_cpu_dvfs_id id, int restore)
{
	struct eem_det *det = NULL;

	switch (id) {
	case MT_CPU_DVFS_B:
		det = id_to_eem_det(EEM_DET_BIG);
		break;

	case MT_CPU_DVFS_L:
		det = id_to_eem_det(EEM_DET_L);
		break;

	case MT_CPU_DVFS_LL:
		det = id_to_eem_det(EEM_DET_2L);
		break;

	case MT_CPU_DVFS_CCI:
		det = id_to_eem_det(EEM_DET_CCI);
		break;

	default:
		break;
	}

	if (det) {
		if (restore)
			restore_record(det);
		else
			record(det);
	}
}
#endif
/* volt_tbl_pmic is convert from 10uV */
static int set_volt_cpu(struct eem_det *det)
{
	int value = 0;

	eem_debug("set_volt_to_cpu!\n");
	FUNC_ENTER(FUNC_LV_HELP);

	/* eem_debug("init02_vop_30 = 0x%x\n", det->vop30[EEM_PHASE_INIT02]); */
	#ifndef EARLY_PORTING_CPUDVFS
	#ifdef __KERNEL__
	mutex_lock(&record_mutex);
	#endif
	for (value = 0; value < NR_FREQ; value++)
		record_tbl_locked[value] = det->volt_tbl_pmic[value];

	switch (det_to_id(det)) {
	case EEM_DET_BIG:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_B, record_tbl_locked, det->num_freq_tbl);
		break;

	case EEM_DET_L:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_L, record_tbl_locked, det->num_freq_tbl);
		break;

	case EEM_DET_2L:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_LL, record_tbl_locked, det->num_freq_tbl);
		break;

	case EEM_DET_CCI:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_CCI, record_tbl_locked, det->num_freq_tbl);
		break;

	default:
		value = 0;
		break;
	}
	#ifdef __KERNEL__
	mutex_unlock(&record_mutex);
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
	#endif
	return value;

}

static void restore_default_volt_cpu(struct eem_det *det)
{
	#ifndef EARLY_PORTING_CPUDVFS
	int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	switch (det_to_id(det)) {
	case EEM_DET_BIG:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_B, det->volt_tbl_orig, det->num_freq_tbl);
		break;

	case EEM_DET_L:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_L, det->volt_tbl_orig, det->num_freq_tbl);
		break;

	case EEM_DET_2L:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_LL, det->volt_tbl_orig, det->num_freq_tbl);
		break;

	case EEM_DET_CCI:
		value = mt_cpufreq_update_volt(MT_CPU_DVFS_CCI, det->volt_tbl_orig, det->num_freq_tbl);
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
	#endif
}

static void get_freq_table_cpu(struct eem_det *det)
{
	int i = 0;
	#if !(DVT)
	/* unsigned int binLevel = 0;*/
	/* Frequency gethering stopped from DVFS */
	enum mt_cpu_dvfs_id cpu_id;
	enum eem_det_id det_id = det_to_id(det);
	#endif

	FUNC_ENTER(FUNC_LV_HELP);

	#if DVT
	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = freq[i];
		if (det->freq_tbl[i] == 0)
			break;
	}
	#else
	cpu_id = (det_id == EEM_DET_BIG) ? MT_CPU_DVFS_B :
			(det_id == EEM_DET_CCI) ? MT_CPU_DVFS_CCI :
			(det_id == EEM_DET_2L) ? MT_CPU_DVFS_LL :
			MT_CPU_DVFS_L;

	det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu_id, 0);
	for (i = 0; i < NR_FREQ_CPU; i++) {
		det->freq_tbl[i] = PERCENT(mt_cpufreq_get_freq_by_idx(cpu_id, i), det->max_freq_khz);
		/*eem_debug("freq[%d]=%d, det->max_freq_khz=%d, freq_tbl[%d]=%d 0x%0x\n",
		*			i, freq[i], det->max_freq_khz, i, det->freq_tbl[i], det->freq_tbl[i]);
		*/
		if (det->freq_tbl[i] == 0)
			break;
	}
	#endif

	det->num_freq_tbl = i;

	eem_debug("[%s] freq_num:%d, max_freq=%d\n", det->name+8, det->num_freq_tbl, det->max_freq_khz);
	/*for (i = 0; i < NR_FREQ; i++)
	*	eem_debug("%d\n", det->freq_tbl[i]);
	*/
	FUNC_EXIT(FUNC_LV_HELP);
}

/* get original volt from cpu dvfs, and apply this table to dvfs
*   when ptp need to restore volt
*/
static void get_orig_volt_table_cpu(struct eem_det *det)
{
	int i = 0;
	unsigned int det_id = det_to_id(det);

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < det->num_freq_tbl; i++) {
		det->volt_tbl_orig[i] =
			((det_id == EEM_DET_BIG) ? mt_cpufreq_get_volt_by_idx(MT_CPU_DVFS_B, i) :
			(det_id == EEM_DET_2L) ? mt_cpufreq_get_volt_by_idx(MT_CPU_DVFS_LL, i) :
			(det_id == EEM_DET_L) ?  mt_cpufreq_get_volt_by_idx(MT_CPU_DVFS_L, i) :
			mt_cpufreq_get_volt_by_idx(MT_CPU_DVFS_CCI, i));
		/* eem_debug("volt_tbl_orig[%d] = %d\n", i, det->volt_tbl_orig[i]);*/
	}
	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	/* eem_debug("get_volt_gpu=%d\n",mt_gpufreq_get_cur_volt()); */
	#ifdef EARLY_PORTING_GPU
	return 0;
	#else
		#if defined(__MTK_SLT_)
			return gpu_dvfs_get_cur_volt();
		#else
			return mt_gpufreq_get_cur_volt()/10; /* unit  mv * 100 = 10uv */
		#endif
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static int set_volt_gpu(struct eem_det *det)
{
	int i;
	unsigned int output[NR_FREQ_GPU];
	FUNC_ENTER(FUNC_LV_HELP);

	/*
	*eem_error("set_volt_gpu= ");
	*for (i = 0; i < 15; i++)
	*	eem_error("%x(%d), ", det->volt_tbl_pmic[i], det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	*eem_error("%x\n", det->volt_tbl_pmic[i]);
	*/
	for (i = 0; i < det->num_freq_tbl; i++)
		output[i] = det->volt_tbl_pmic[i];

	#ifdef EARLY_PORTING_GPU
		return 0;
	#else
		return mt_gpufreq_update_volt(output, NR_FREQ_GPU);
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void restore_default_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	#ifndef EARLY_PORTING_GPU
	mt_gpufreq_restore_default_volt();
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_gpu(struct eem_det *det)
{
	int i = 0;

	memset(det->freq_tbl, '0', sizeof(det->freq_tbl));

	FUNC_ENTER(FUNC_LV_HELP);

	#if DVT
	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = freq[i];
		if (det->freq_tbl[i] == 0)
			break;
	}
	#else
	#ifndef EARLY_PORTING_GPU
	det->max_freq_khz = mt_gpufreq_get_freq_by_idx(0);
	for (i = 0; i < NR_FREQ_GPU; i++)
		det->freq_tbl[i] = PERCENT(mt_gpufreq_get_freq_by_idx(i), det->max_freq_khz);
	#endif
	#endif

	det->num_freq_tbl = i;

	eem_debug("[%s] freq num:%d\n", det->name+8, det->num_freq_tbl);
	/*
	*for (i = 0; i < NR_FREQ; i++)
	*	eem_debug("%d\n", det->freq_tbl[i]);
	*/
	FUNC_EXIT(FUNC_LV_HELP);
}

#ifdef __KERNEL__
static long long eem_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}
#endif

#if 0
static int set_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	return mt_cpufreq_set_lte_volt(det->volt_tbl_init2[0]);
#else
	return 0;
	/* return dvfs_set_vlte(det->volt_tbl_bin[0]); */
	/* return dvfs_set_vlte(det->volt_tbl_init2[0]); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	return mt_get_cur_volt_lte(); /* unit mv * 100 = 10uv */
	FUNC_EXIT(FUNC_LV_HELP);
}

static void restore_default_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	if (mt_get_chip_hw_code() == 0x337)
		mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 105000));
#else
	/* dvfs_set_vlte(det->ops->volt_2_pmic(det, 105000)); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void switch_to_vcore_ao(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_AO);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[PTP_CTRL_VCORE].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void switch_to_vcore_pdn(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL, SEL_VCORE_PDN);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[EEM_CTRL_SOC].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	#if defined(__MTK_SLT_)
		#if defined(SLT_VMAX)
			det->volt_tbl_init2[0] = det->ops->volt_2_pmic(det, 131000);
			eem_debug("HV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#elif defined(SLT_VMIN)
			det->volt_tbl_init2[0] = clamp(det->volt_tbl_init2[0] - 0xB,
				det->ops->eem_2_pmic(det, det->VMIN), det->ops->eem_2_pmic(det, det->VMAX));
			eem_debug("LV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#else
			eem_debug("NV VCORE voltage EEM to 0x%x\n", det->volt_tbl_init2[0]);
		#endif
	#else
		eem_debug("VCORE EEM voltage to 0x%x\n", det->volt_tbl_init2[0]);
	#endif

	/* return mt_set_cur_volt_vcore_pdn(det->volt_tbl_pmic[0]); */  /* unit = 10 uv */
	#ifdef EARLY_PORTING
		return 0;
	#else
		return dvfs_set_vcore(det->volt_tbl_init2[0]);
	#endif
}
#endif

static int get_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	#ifdef EARLY_PORTING
	return 0;
	#else
	eem_debug("get_volt_vcore=%d\n", mt_get_cur_volt_vcore_ao());
	return mt_get_cur_volt_vcore_ao(); /* unit = 10 uv */
	#endif
}
#endif

/* for DVT */
#if EEM_BANK_SOC
static void get_freq_table_vcore(struct eem_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = freq[i];
		eem_debug("Timer Bank = %d, freq_percent = (%d)\n", det->ctrl_id, det->freq_tbl[i]);
		if (det->freq_tbl[i] == 0)
			break;
	}

	eem_debug("NR_FREQ=%d\n", i);
	det->num_freq_tbl = i;
	FUNC_EXIT(FUNC_LV_HELP);

}
#endif

/*=============================================================
* Global function definition
*=============================================================
*/
static void mt_ptp_lock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
	/* FIXME: lock with MD32 */
	/* get_md32_semaphore(SEMAPHORE_PTP); */
#ifdef __KERNEL__
	spin_lock_irqsave(&eem_spinlock, *flags);
	eem_pTime_us = eem_get_current_time_us();
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_lock);
#endif

void mt_ptp_unlock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
#ifdef __KERNEL__
	eem_cTime_us = eem_get_current_time_us();
	EEM_IS_TOO_LONG();
	spin_unlock_irqrestore(&eem_spinlock, *flags);
	/* FIXME: lock with MD32 */
	/* release_md32_semaphore(SEMAPHORE_PTP); */
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_unlock);
#endif

#if 0
int mt_ptp_idle_can_enter(void)
{
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_ctrl(ctrl) {
		if (atomic_read(&ctrl->in_init)) {
			FUNC_EXIT(FUNC_LV_HELP);
			return 0;
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 1;
}
EXPORT_SYMBOL(mt_ptp_idle_can_enter);
#endif

#ifdef __KERNEL__
/*
 * timer for log
 */
static enum hrtimer_restart eem_log_timer_func(struct hrtimer *timer)
{
	struct eem_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		if (det->ctrl_id == EEM_CTRL_SOC)
			continue;

		eem_error("Timer Bk=%d (%d)(%d, %d, %d, %d, %d, %d, %d, %d)(0x%x)\n",
			det->ctrl_id,
			det->ops->get_temp(det),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[0]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[1]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[2]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[3]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[4]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[5]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[6]),
			det->ops->pmic_2_volt(det, det->volt_tbl_pmic[7]),
			det->t250);
		#if ITurbo
		eem_error("sts(%d), It(%d)\n",
			det->ops->get_status(det),
			ITurboRun
			);
		#endif

		#if 0
		det->freq_tbl[0],
		det->freq_tbl[1],
		det->freq_tbl[2],
		det->freq_tbl[3],
		det->freq_tbl[4],
		det->freq_tbl[5],
		det->freq_tbl[6],
		det->freq_tbl[7],
		det->dcvalues[3],
		det->freqpct30[3],
		det->eem_26c[3],
		det->vop30[3]
		#endif
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#endif
/*
 * Thread for voltage setting
 */
static int eem_volt_thread_handler(void *data)
{
	struct eem_ctrl *ctrl = (struct eem_ctrl *)data;
	struct eem_det *det = id_to_eem_det(ctrl->det_id);

	FUNC_ENTER(FUNC_LV_HELP);
	#ifdef __KERNEL__
	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt) {
		/* update set volt status for this bank */
		#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			int temp = -1;

			switch (det->ctrl_id) {
			case EEM_CTRL_BIG:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_BIG_IS_SET_VOLT));
				temp = EEM_CPU_BIG_IS_SET_VOLT;
				break;

			case EEM_CTRL_GPU:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_GPU_IS_SET_VOLT));
				temp = EEM_GPU_IS_SET_VOLT;
				break;

			case EEM_CTRL_L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_2L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_2_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_2_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_CCI:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_CCI_IS_SET_VOLT));
				temp = EEM_CPU_CCI_IS_SET_VOLT;
				break;

			default:
				break;
			}
		#endif
		det->ops->set_volt(det);
		eem_error("B=%d,T=%d,DC=%x,V30=%x,F30=%x,sts=%x,250=%x\n",
		det->ctrl_id,
		det->ops->get_temp(det),
		det->dcvalues[EEM_PHASE_INIT01],
		det->vop30[EEM_PHASE_MON],
		det->freqpct30[EEM_PHASE_MON],
		det->ops->get_status(det),
		det->t250);

		/* clear out set volt status for this bank */
		#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			if (temp >= EEM_CPU_BIG_IS_SET_VOLT)
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() & ~(1 << temp));
		#endif
		}
		if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = EEM_VOLT_NONE;

	} while (!kthread_should_stop());
	#endif
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct eem_det *det)
{
	/*
	 * Inherit ops from eem_det_base_ops if ops in det is NULL
	 */
	FUNC_ENTER(FUNC_LV_HELP);

	#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = eem_det_base_ops.func;	\
		} while (0)

	INIT_OP(det->ops, disable);
	INIT_OP(det->ops, disable_locked);
	INIT_OP(det->ops, switch_bank);
	INIT_OP(det->ops, init01);
	INIT_OP(det->ops, init02);
	INIT_OP(det->ops, mon_mode);
	INIT_OP(det->ops, get_status);
	INIT_OP(det->ops, dump_status);
	INIT_OP(det->ops, set_phase);
	INIT_OP(det->ops, get_temp);
	INIT_OP(det->ops, get_volt);
	INIT_OP(det->ops, set_volt);
	INIT_OP(det->ops, restore_default_volt);
	INIT_OP(det->ops, get_freq_table);
	INIT_OP(det->ops, volt_2_pmic);
	INIT_OP(det->ops, volt_2_eem);
	INIT_OP(det->ops, pmic_2_volt);
	INIT_OP(det->ops, eem_2_pmic);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_ctrl(struct eem_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/* init_completion(&ctrl->init_done); */
	/* atomic_set(&ctrl->in_init, 0); */
	#ifdef __KERNEL__
	/* if(HAS_FEATURE(id_to_eem_det(ctrl->det_id), FEA_MON)) { */
	/* TODO: FIXME, why doesn't work <-XXX */
	if (1) {
		init_waitqueue_head(&ctrl->wq);
		ctrl->thread = kthread_run(eem_volt_thread_handler, ctrl, ctrl->name);

		if (IS_ERR(ctrl->thread))
			eem_error("Create %s thread failed: %ld\n", ctrl->name, PTR_ERR(ctrl->thread));
	}
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_det(struct eem_det *det, struct eem_devinfo *devinfo)
{
	enum eem_det_id det_id = det_to_id(det);
	/* unsigned int binLevel; */

	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("eem_init_det: det=%s, id=%d\n", ((char *)(det->name) + 8), det_id);

	inherit_base_det(det);

	/* init with devinfo, init with constant */
	det->DETWINDOW = DETWINDOW_VAL;

	det->VCO = VCO_VAL;
	det->DTHI = DTHI_VAL;
	det->DTLO = DTLO_VAL;
	det->DETMAX = DETMAX_VAL;
	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM = AGEM_VAL;
	det->DVTFIXED = DVTFIXED_VAL;
	det->DCCONFIG = DCCONFIG_VAL;

	#if 0
	if (ateVer > 5)
		det->VCO = VCO_VAL_AFTER_5;
	else
	#endif

	#if 0
	#ifdef BINLEVEL_BYEFUSE
		#ifdef __KERNEL__
		binLevel = GET_BITS_VAL(3:0, get_devinfo_with_index(22));
		#else
		binLevel = GET_BITS_VAL(3:0, eem_read(0x1020671C));
		#endif
	#else
	binLevel = 0;
	#endif

	if ((binLevel == 0) || (binLevel == 3))
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE);
	else if ((binLevel == 1) || (binLevel == 6))
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE_SB);
	else if ((binLevel == 2) || (binLevel == 7))
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE);
	else
		det->VMIN       = det->ops->volt_2_eem(det, VMIN_VAL_LITTLE);
	/*
	*if (NULL != det->ops->get_volt) {
	*	det->VBOOT = det->ops->get_volt(det);
	*	eem_debug("@%s, %s-VBOOT = %d\n",
	*		__func__, ((char *)(det->name) + 8), det->VBOOT);
	*}
	*/
	#endif

	switch (det_id) {
	case EEM_DET_BIG:
		det->MDES	= devinfo->BIG_MDES;
		det->BDES	= devinfo->BIG_BDES;
		det->DCMDET	= devinfo->BIG_DCMDET;
		det->DCBDET	= devinfo->BIG_DCBDET;
		det->EEMINITEN	= devinfo->BIG_INITEN;
		det->EEMMONEN	= devinfo->BIG_MONEN;
		det->VBOOT	= det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		#if 0
		if (ateVer > 5)
			det->VCO = VCO_VAL_AFTER_5_BIG;
		else
			det->VCO = VCO_VAL_BIG;

		det->recordRef	= recordRef + 108;
		/*
		*int i;
		*	for (int i = 0; i < NR_FREQ; i++)
		*		eem_debug("@(SRAM)%s----->(%s), = 0x%08x\n",
		*				__func__,
		*				det->name,
		*				*(det->recordRef + (i * 2)));
		*/
		#endif
		break;

	case EEM_DET_GPU:
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->EEMINITEN	= devinfo->GPU_INITEN;
		det->EEMMONEN	= devinfo->GPU_MONEN;
		det->VBOOT	= det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_GPU);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		/* override default setting */
		det->DVTFIXED   = DVTFIXED_VAL_GPU;
		det->VCO	= VCO_VAL_GPU;
		break;

	case EEM_DET_L:
		det->MDES	= devinfo->CPU_L_MDES;
		det->BDES	= devinfo->CPU_L_BDES;
		det->DCMDET	= devinfo->CPU_L_DCMDET;
		det->DCBDET	= devinfo->CPU_L_DCBDET;
		det->EEMINITEN	= devinfo->CPU_L_INITEN;
		det->EEMMONEN	= devinfo->CPU_L_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_L);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		#if 0
		det->recordRef	= recordRef + 36;
		int i;

		for (int i = 0; i < NR_FREQ; i++)
			eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	case EEM_DET_2L:
		det->MDES	= devinfo->CPU_2L_MDES;
		det->BDES	= devinfo->CPU_2L_BDES;
		det->DCMDET	= devinfo->CPU_2L_DCMDET;
		det->DCBDET	= devinfo->CPU_2L_DCBDET;
		det->EEMINITEN	= devinfo->CPU_2L_INITEN;
		det->EEMMONEN	= devinfo->CPU_2L_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		#if 0
		det->recordRef	= recordRef;
		int i;

		for (int i = 0; i < NR_FREQ; i++)
			eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

	case EEM_DET_CCI:
		det->MDES	= devinfo->CCI_MDES;
		det->BDES	= devinfo->CCI_BDES;
		det->DCMDET	= devinfo->CCI_DCMDET;
		det->DCBDET	= devinfo->CCI_DCBDET;
		det->EEMINITEN	= devinfo->CCI_INITEN;
		det->EEMMONEN	= devinfo->CCI_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		#if 0
		det->recordRef	= recordRef + 72;
		int i;

		for (int i = 0; i < NR_FREQ; i++)
			eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(det->recordRef + (i * 2)));
		#endif
		break;

     /* for DVT SOC input values are the same as CCI*/
    #if EEM_BANK_SOC
	case EEM_DET_SOC:
		det->MDES	= devinfo->CCI_MDES;
		det->BDES	= devinfo->CCI_BDES;
		det->DCMDET	= devinfo->CCI_DCMDET;
		det->DCBDET	= devinfo->CCI_DCBDET;
		det->EEMINITEN	= devinfo->CCI_INITEN;
		det->EEMMONEN	= devinfo->CCI_MONEN;
		det->VBOOT      = det->ops->volt_2_eem(det, det->VBOOT);
		det->VMAX	= det->ops->volt_2_eem(det, VMAX_VAL_SOC);
		det->VMIN	= det->ops->volt_2_eem(det, VMIN_VAL);
		break;
	#endif
	default:
		eem_debug("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	switch (det->ctrl_id) {
	case EEM_CTRL_BIG:
		det->AGEDELTA	= devinfo->BIG_AGEDELTA;
		det->MTDES	= devinfo->BIG_MTDES;
		break;

	case EEM_CTRL_GPU:
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
		break;

	case EEM_CTRL_L:
		det->AGEDELTA	= devinfo->CPU_L_AGEDELTA;
		det->MTDES	= devinfo->CPU_L_MTDES;
		break;

	case EEM_CTRL_2L:
		det->AGEDELTA	= devinfo->CPU_2L_AGEDELTA;
		det->MTDES	= devinfo->CPU_2L_MTDES;
		break;

	case EEM_CTRL_CCI:
		det->AGEDELTA	= devinfo->CCI_AGEDELTA;
		det->MTDES	= devinfo->CCI_MTDES;
		break;

    #if EEM_BANK_SOC
	case EEM_CTRL_SOC: /* init soc det info */
		det->AGEDELTA	= devinfo->CCI_AGEDELTA;
		det->MTDES	= devinfo->CCI_MTDES;
		break;
    #endif

	default:
		eem_debug("[%s]: Unknown ctrl_id %d\n", __func__, det->ctrl_id);
		break;
	}

	memset(det->volt_tbl, 0, sizeof(det->volt_tbl));
	memset(det->volt_tbl_pmic, 0, sizeof(det->volt_tbl_pmic));
	memset(record_tbl_locked, 0, sizeof(record_tbl_locked));

	/* get DVFS frequency table */
	det->ops->get_freq_table(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

#ifdef UPDATE_TO_UPOWER
static enum upower_bank transfer_ptp_to_upower_bank(unsigned int det_id)
{
	enum upower_bank bank;

	switch (det_id) {
	case EEM_DET_BIG:
		bank = UPOWER_BANK_B;
		break;
	case EEM_DET_2L:
		bank = UPOWER_BANK_LL;
		break;
	case EEM_DET_L:
		bank = UPOWER_BANK_L;
		break;
	case EEM_DET_CCI:
		bank = UPOWER_BANK_CCI;
		break;
	default:
		bank = NR_UPOWER_BANK;
		break;
	}
	return bank;
}

static void eem_update_init2_volt_to_upower(struct eem_det *det, unsigned int *pmic_volt)
{
	unsigned int *volt_tbl;
	enum upower_bank bank;
	int i;

	volt_tbl = kmalloc((sizeof(unsigned int) * (det->num_freq_tbl)), GFP_KERNEL);
	for (i = 0; i < det->num_freq_tbl; i++)
		volt_tbl[i] = det->ops->pmic_2_volt(det, pmic_volt[i]);

	bank = transfer_ptp_to_upower_bank(det_to_id(det));
	if (bank < NR_UPOWER_BANK) {
		upower_update_volt_by_eem(bank, volt_tbl, det->num_freq_tbl);
		eem_debug("update init2 volt to upower (eem bank %ld upower bank %d)\n", det_to_id(det), bank);
	}
	kfree(volt_tbl);

}
#endif

#if defined(__MTK_SLT_) && (defined(SLT_VMAX) || defined(SLT_VMIN))
	#define EEM_PMIC_LV_HV_OFFSET (0x3)
	#define EEM_PMIC_NV_OFFSET (0xB)
#endif
static void eem_set_eem_volt(struct eem_det *det)
{
#if SET_PMIC_VOLT
	unsigned i;
	int cur_temp, low_temp_offset;
	struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);
    #if ITurbo
	ITurboRunSet = 0;
    #endif
	FUNC_ENTER(FUNC_LV_HELP);

	cur_temp = det->ops->get_temp(det);

	#ifdef UPDATE_TO_UPOWER
	upower_update_degree_by_eem(transfer_ptp_to_upower_bank(det_to_id(det)), cur_temp/1000);
	#endif

	/* eem_debug("eem_set_eem_volt cur_temp = %d\n", cur_temp); */
	/* 6250 * 10uV = 62.5mv */
	if (cur_temp <= 33000)
		low_temp_offset = det->ops->volt_2_eem(det, 6250 + det->eem_v_base);
	else
		low_temp_offset = 0;

	ctrl->volt_update |= EEM_VOLT_UPDATE;

	#if ITurbo
	if (cur_temp <= 33000) {
		if ((det->ctrl_id == EEM_CTRL_L) && (ITurboRun == 1))
			ITurboRunSet = 0;
	} else {
		if ((det->ctrl_id == EEM_CTRL_L) && (ITurboRun == 1))
			ITurboRunSet = 0;
	}
	#endif
	/* for debugging */
	eem_debug("volt_offset, low_temp_offset= %d, %d\n", det->volt_offset, low_temp_offset);
	/* eem_debug("pi_offset = %d\n", det->pi_offset);*/
	/* eem_debug("det->vmin = %d\n", det->VMIN); */
	/* eem_debug("det->vmax = %d\n", det->VMAX); */

	/*
	*eem_error("Temp=(%d) ITR=(%d), ITRS=(%d)\n", cur_temp, ITurboRun, ITurboRunSet);
	*eem_debug("ctrl->volt_update |= EEM_VOLT_UPDATE\n");
	*/

	/* scale of det->volt_offset must equal 10uV */
	/* if has record table, min with record table of each cpu */
	for (i = 0; i < det->num_freq_tbl; i++) {
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			det->volt_tbl_pmic[i] =
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det, (det->volt_tbl[i] + det->volt_offset +
									low_temp_offset + det->pi_offset)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)));
			break;

		case EEM_CTRL_GPU:
			det->volt_tbl_pmic[i] =
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det, (det->volt_tbl[i] + det->volt_offset + low_temp_offset)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)));
			break;

		case EEM_CTRL_L:
			#if ITurbo
			if (ITurboRunSet == 1) {
				ITurbo_offset[i] = det->ops->volt_2_eem(det, MAX_ITURBO_OFFSET + det->eem_v_base) *
						(det->volt_tbl[i] - det->volt_tbl[NR_FREQ-1]) /
						(det->volt_tbl[0] - det->volt_tbl[NR_FREQ-1]);
			} else
				ITurbo_offset[i] = 0;
			#endif

			det->volt_tbl_pmic[i] =
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det,
					(det->volt_tbl[i] + det->volt_offset + low_temp_offset
					#if ITurbo
					- ITurbo_offset[i]
					#endif
				))+det->pi_offset,
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)));

			#if 0
			if (eem_log_en)
				eem_error("L->hw_v[%d]=0x%X, V(%d)L(%d)I(%d)P(%d) volt_tbl_pmic[%d]=0x%X (%d)\n",
					i, det->volt_tbl[i],
					det->volt_offset, low_temp_offset, ITurbo_offset[i], det->pi_offset,
					i, det->volt_tbl_pmic[i], det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
			#endif
			break;

		case EEM_CTRL_2L:
			det->volt_tbl_pmic[i] =
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det,
					(det->volt_tbl[i] + det->volt_offset + low_temp_offset)) + det->pi_offset,
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)));
			break;

		case EEM_CTRL_CCI:
			det->volt_tbl_pmic[i] =
			(unsigned int)(clamp(
				det->ops->eem_2_pmic(det, (det->volt_tbl[i] + det->volt_offset + low_temp_offset)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)));
			break;

		default:
			break;
		}

		eem_debug("[%s].volt_tbl[%d] = 0x%X ----- volt_tbl_pmic[%d] = 0x%X (%d)\n",
			det->name,
			i, det->volt_tbl[i],
			i, det->volt_tbl_pmic[i], det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	}


	#ifdef UPDATE_TO_UPOWER
	/* only when set_volt_to_upower == 0, the volt will be apply to upower */
	if (det->set_volt_to_upower == 0) {
		eem_update_init2_volt_to_upower(det, det->volt_tbl_pmic);
		det->set_volt_to_upower = 1;
	}
	#endif

	#if defined(__MTK_SLT_)
		#if defined(SLT_VMAX)
		det->volt_tbl_pmic[0] =
			clamp(
				det->ops->eem_2_pmic(det,
					((det->volt_tbl[0] + det->volt_offset + low_temp_offset) -
						EEM_PMIC_NV_OFFSET +
						EEM_PMIC_LV_HV_OFFSET)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)
			);
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#elif defined(SLT_VMIN)
		det->volt_tbl_pmic[0] =
			clamp(
				det->ops->eem_2_pmic(det,
					((det->volt_tbl[0] + det->volt_offset + low_temp_offset) -
						EEM_PMIC_NV_OFFSET -
						EEM_PMIC_LV_HV_OFFSET)),
				det->ops->eem_2_pmic(det, det->VMIN),
				det->ops->eem_2_pmic(det, det->VMAX)
			);
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#else
		/* det->volt_tbl_pmic[0] = det->volt_tbl_pmic[0] - EEM_PMIC_NV_OFFSET; */
		eem_debug("NV VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
		#endif
	#endif

	#ifdef __KERNEL__
		if ((0 == (det->disabled % 2)) && (0 == (det->disabled & BY_PROCFS_INIT2)))
			wake_up_interruptible(&ctrl->wq);
		else
			eem_error("Disabled by [%d]\n", det->disabled);
	#else
		#if defined(__MTK_SLT_)
			if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
				det->ops->set_volt(det);

			if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
				det->ops->restore_default_volt(det);

			ctrl->volt_update = EEM_VOLT_NONE;
		#endif
	#endif
#endif

	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_restore_eem_volt(struct eem_det *det)
{
	#if SET_PMIC_VOLT
		struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

		ctrl->volt_update |= EEM_VOLT_RESTORE;
		#ifdef __KERNEL__
			wake_up_interruptible(&ctrl->wq);
		#else
			#if defined(__MTK_SLT_)
			if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
				det->ops->set_volt(det);

			if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
				det->ops->restore_default_volt(det);

			ctrl->volt_update = EEM_VOLT_NONE;
			#endif
		#endif
	#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

#if 0
static void mt_eem_reg_dump_locked(void)
{
#ifndef CONFIG_ARM64
	unsigned int addr;

	for (addr = (unsigned int)EEM_DESCHAR; addr <= (unsigned int)EEM_SMSTATE1; addr += 4)
		eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);

	addr = (unsigned int)EEMCORESEL;
	eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);
#else
	unsigned long addr;

	for (addr = (unsigned long)EEM_DESCHAR; addr <= (unsigned long)EEM_SMSTATE1; addr += 4)
		eem_isr_info("0x %lu = 0x %lu\n", addr, *(volatile unsigned long *)addr);

	addr = (unsigned long)EEMCORESEL;
	eem_isr_info("0x %lu = 0x %lu\n", addr, *(volatile unsigned long *)addr);
#endif
}
#endif

static inline void handle_init01_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init1 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT01]		= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_INIT01]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT01]		= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_INIT01]		= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_INIT01]	= eem_read(EEMEN);

	/*
	*#if defined(__MTK_SLT_)
	*eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT01]);
	*eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_INIT01]);
	*eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT01]);
	*eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_INIT01]);
	*eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_INIT01]);i
	*#endif
	*/
	#if DUMP_DATA_TO_DE
	{
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][EEM_PHASE_INIT01] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
			#if 0
			#ifdef __KERNEL__
			eem_isr_info("0x%lx = 0x%08x\n",
			#else
			eem_isr_info("0x%X = 0x%X\n",
			#endif
				(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
				det->reg_dump_data[i][EEM_PHASE_INIT01]
				);
			#endif
		}
	}
	#endif
	/*
	 * Read & store 16 bit values EEM_DCVALUES.DCVOFFSET and
	 * EEM_AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	det->DCVOFFSETIN = ~(eem_read(EEM_DCVALUES) & 0xffff) + 1; /* hw bug, workaround */
	det->AGEVOFFSETIN = eem_read(EEM_AGEVALUES) & 0xffff;

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);
	/* det->ops->init02(det); */

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static unsigned int interpolate(unsigned int y1, unsigned int y0,
	unsigned int x1, unsigned int x0, unsigned int ym)
{
	unsigned int ratio, result;

	ratio = (((y1 - y0) * 100) + (x1 - x0 - 1)) / (x1 - x0);
	result =  (x1 - ((((y1 - ym) * 10000) + ratio - 1) / ratio) / 100);
	/*
	*eem_debug("y1(%d), y0(%d), x1(%d), x0(%d), ym(%d), ratio(%d), rtn(%d)\n",
	*	y1, y0, x1, x0, ym, ratio, result);
	*/

	return result;
}

#if defined(__MTK_SLT_)
int cpu_in_mon;
#endif
static inline void handle_init02_isr(struct eem_det *det)
{
	unsigned int temp, i, j;
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init2 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT02]		= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_INIT02]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT02]		= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_INIT02]	= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_INIT02]	= eem_read(EEMEN);

	#if 0
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT02]);
	eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT02]);
	eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_INIT02]);
	#endif
	#endif

	#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_INIT02] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
		#if 0
		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_INIT02]
			);
		#endif
	}
	#endif

		temp = eem_read(EEM_VOP30);
		/* eem_debug("init02 read(EEM_VOP30) = 0x%08X\n", temp); */
		/* EEM_VOP30=>pmic value */
		det->volt_tbl[0] = (temp & 0xff);
		det->volt_tbl[1 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
		det->volt_tbl[2 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
		det->volt_tbl[3 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

		temp = eem_read(EEM_VOP74);
		/* eem_debug("init02 read(EEM_VOP74) = 0x%08X\n", temp); */
		/* EEM_VOP74=>pmic value */
		det->volt_tbl[4 * (NR_FREQ / 8)] = (temp & 0xff);
		det->volt_tbl[5 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
		det->volt_tbl[6 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
		det->volt_tbl[7 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

		if (NR_FREQ > 8) {
			for (i = 0; i < 8; i++) {
				for (j = 1; j < (NR_FREQ / 8); j++) {
					if (i < 7) {
						det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
							interpolate(
								det->freq_tbl[(i * (NR_FREQ / 8))],
								det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
								det->volt_tbl[(i * (NR_FREQ / 8))],
								det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
								det->freq_tbl[(i * (NR_FREQ / 8)) + j]
							);
					} else {
						det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						clamp(
							interpolate(
								det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
								det->freq_tbl[((i) * (NR_FREQ / 8))],
								det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
								det->volt_tbl[((i) * (NR_FREQ / 8))],
								det->freq_tbl[(i * (NR_FREQ / 8)) + j]
							),
							det->VMIN,
							det->VMAX
						);
					}
				}
			}
		} /* if (NR_FREQ > 8)*/

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
		#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_big_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_big_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_big_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_big_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		default:
			break;
		}
		#endif
		#if 0
		eem_error("init02_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i, det->volt_tbl[i], det->ops->pmic_2_volt(det, det->volt_tbl[i]));

		if (NR_FREQ > 8) {
			eem_error("init02_[%s].volt_tbl[%d] = 0x%X (%d)\n",
			det->name, i+1, det->volt_tbl[i+1], det->ops->pmic_2_volt(det, det->volt_tbl[i+1]));
		}
		#endif
	}
	#if defined(__MTK_SLT_)
	if (det->ctrl_id <= EEM_CTRL_CCI)
		cpu_in_mon = 0;
	#endif
	eem_set_eem_volt(det);

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);

	/* atomic_dec(&ctrl->in_init); */
	/* complete(&ctrl->init_done); */
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	eem_error("====================================================\n");
	eem_error("EEM init err: EEMEN(%p) = 0x%X, EEMINTSTS(%p) = 0x%X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_error("EEM_SMSTATE0 (%p) = 0x%X\n",
		     EEM_SMSTATE0, eem_read(EEM_SMSTATE0));
	eem_error("EEM_SMSTATE1 (%p) = 0x%X\n",
		     EEM_SMSTATE1, eem_read(EEM_SMSTATE1));
	eem_error("====================================================\n");

#if 0
TODO: FIXME
	{
		struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

		atomic_dec(&ctrl->in_init);
		complete(&ctrl->init_done);
	}
TODO: FIXME
#endif

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n",
		__func__,
		__LINE__,
		det->name,
		det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct eem_det *det)
{
	unsigned int temp, i, j;
	#if defined(CONFIG_THERMAL) && defined(EARLY_PORTING_THERMAL)
	unsigned long long temp_long;
	unsigned long long temp_cur = (unsigned long long)aee_rr_curr_ptp_temp();
	#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = mon %s-isr\n", ((char *)(det->name) + 8));

	#if defined(CONFIG_THERMAL) && !defined(EARLY_PORTING_THERMAL)
	eem_isr_info("B_temp=%d, CCI_temp=%d, G_temp=%d, LL_temp=%d, L_temp=%d\n",
		tscpu_get_temp_by_bank(THERMAL_BANK0),
		tscpu_get_temp_by_bank(THERMAL_BANK1),
		tscpu_get_temp_by_bank(THERMAL_BANK2),
		tscpu_get_temp_by_bank(THERMAL_BANK3),
		tscpu_get_temp_by_bank(THERMAL_BANK4)
		);
	#endif

	#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING_THERMAL)
	switch (det->ctrl_id) {
	case EEM_CTRL_L:
		#ifdef CONFIG_THERMAL
		temp_long = (unsigned long long)tscpu_get_temp_by_bank(THERMAL_BANK4)/1000;
		if (temp_long != 0) {
			aee_rr_rec_ptp_temp(temp_long << (8 * EEM_CPU_LITTLE_IS_SET_VOLT) |
			(temp_cur & ~((unsigned long long)0xFF << (8 * EEM_CPU_LITTLE_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_GPU:
		#ifdef CONFIG_THERMAL
		temp_long = (unsigned long long)tscpu_get_temp_by_bank(THERMAL_BANK2)/1000;
		if (temp_long != 0) {
			aee_rr_rec_ptp_temp(temp_long << (8 * EEM_GPU_IS_SET_VOLT) |
			(temp_cur & ~((unsigned long long)0xFF << (8 * EEM_GPU_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_BIG:
		#ifdef CONFIG_THERMAL
		temp_long = (unsigned long long)tscpu_get_temp_by_bank(THERMAL_BANK0)/1000;
		if (temp_long != 0) {
			aee_rr_rec_ptp_temp(temp_long << (8 * EEM_CPU_BIG_IS_SET_VOLT) |
			(temp_cur & ~((unsigned long long)0xFF << (8 * EEM_CPU_BIG_IS_SET_VOLT))));
		}
		#endif
		break;
	case EEM_CTRL_2L:
		#ifdef CONFIG_THERMAL
		temp_long = (unsigned long long)tscpu_get_temp_by_bank(THERMAL_BANK3)/1000;
		if (temp_long != 0) {
			aee_rr_rec_ptp_temp(temp_long << (8 * EEM_CPU_2_LITTLE_IS_SET_VOLT) |
			(temp_cur & ~((unsigned long long)0xFF << (8 * EEM_CPU_2_LITTLE_IS_SET_VOLT))));
		}
		#endif
		break;
	case EEM_CTRL_CCI:
		#ifdef CONFIG_THERMAL
		temp_long = (unsigned long long)tscpu_get_temp_by_bank(THERMAL_BANK1)/1000;
		if (temp_long != 0) {
			aee_rr_rec_ptp_temp(temp_long << (8 * EEM_CPU_CCI_IS_SET_VOLT)|
			(temp_cur & ~((unsigned long long)0xFF << (8 * EEM_CPU_CCI_IS_SET_VOLT))));
		}
		#endif
		break;

	default:
		break;
	}
	#endif

	det->dcvalues[EEM_PHASE_MON]	= eem_read(EEM_DCVALUES);
	det->freqpct30[EEM_PHASE_MON]	= eem_read(EEM_FREQPCT30);
	det->eem_26c[EEM_PHASE_MON]	= eem_read(EEMINTEN + 0x10);
	det->vop30[EEM_PHASE_MON]	= eem_read(EEM_VOP30);
	det->eem_eemEn[EEM_PHASE_MON]	= eem_read(EEMEN);

	#if 0
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_MON]);
	eem_debug("CTP - freqpct30 218 = 0x%x\n", det->freqpct30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_MON]);
	eem_debug("CTP - vop30 248 = 0x%x\n", det->vop30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_eemEn 238 = 0x%x\n", det->eem_eemEn[EEM_PHASE_MON]);
	#endif
	#endif

	#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_MON] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
		#if 0
		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_MON]
			);
		#endif
	}
	#endif

	/* check if thermal sensor init completed? */
	det->t250 = eem_read(TEMP);

	/* 0x64 mappint to 100 + 25 = 125C,
	*   0xB2 mapping to 178 - 128 = 50, -50 + 25 = -25C
	*/
	if (((det->t250 & 0xff)  > 0x64) && ((det->t250  & 0xff) < 0xB2)) {
		eem_error("Temperature to high ----- (%d) degree !!\n", det->t250 + 25);
		goto out;
	}

		temp = eem_read(EEM_VOP30);
		/* eem_debug("mon eem_read(EEM_VOP30) = 0x%08X\n", temp); */
		/* EEM_VOP30=>pmic value */
		det->volt_tbl[0] = (temp & 0xff);
		det->volt_tbl[1 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
		det->volt_tbl[2 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
		det->volt_tbl[3 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

		temp = eem_read(EEM_VOP74);
		/* eem_debug("mon eem_read(EEM_VOP74) = 0x%08X\n", temp); */
		/* EEM_VOP74=>pmic value */
		det->volt_tbl[4 * (NR_FREQ / 8)] = (temp & 0xff);
		det->volt_tbl[5 * (NR_FREQ / 8)] = (temp >> 8)  & 0xff;
		det->volt_tbl[6 * (NR_FREQ / 8)] = (temp >> 16) & 0xff;
		det->volt_tbl[7 * (NR_FREQ / 8)] = (temp >> 24) & 0xff;

		if (NR_FREQ > 8) {
			for (i = 0; i < 8; i++) {
				for (j = 1; j < (NR_FREQ / 8); j++) {
					if (i < 7) {
						det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
							interpolate(
								det->freq_tbl[(i * (NR_FREQ / 8))],
								det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
								det->volt_tbl[(i * (NR_FREQ / 8))],
								det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
								det->freq_tbl[(i * (NR_FREQ / 8)) + j]
							);
					} else {
						det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
							clamp(
								interpolate(
									det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
									det->freq_tbl[((i) * (NR_FREQ / 8))],
									det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
									det->volt_tbl[((i) * (NR_FREQ / 8))],
									det->freq_tbl[(i * (NR_FREQ / 8)) + j]
								),
								det->VMIN,
								det->VMAX
							);
					}
				}
			}
		} /* if (NR_FREQ > 8) */
	for (i = 0; i < NR_FREQ; i++) {
		#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_BIG:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_big_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_big_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_big_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_big_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		default:
			break;
		}
		#endif
		/*
		*eem_error("mon_[%s].volt_tbl[%d] = 0x%X (%d)\n",
		*	det->name, i, det->volt_tbl[i], det->ops->pmic_2_volt(det, det->volt_tbl[i]));

		*if (NR_FREQ > 8) {
		*	eem_isr_info("mon_[%s].volt_tbl[%d] = 0x%X (%d)\n",
		*	det->name, i+1, det->volt_tbl[i+1], det->ops->pmic_2_volt(det, det->volt_tbl[i+1]));
		*}
		*/
	}
	eem_isr_info("ISR : EEM_TEMPSPARE1 = 0x%08X\n", eem_read(EEM_TEMPSPARE1));

	#if defined(__MTK_SLT_)
		if ((cpu_in_mon == 1) && (det->ctrl_id <= EEM_CTRL_CCI))
			eem_isr_info("[%s] Won't do CPU eem_set_eem_volt\n", ((char *)(det->name) + 8));
		else {
			if (det->ctrl_id <= EEM_CTRL_CCI) {
				eem_isr_info("[%s] Do CPU eem_set_eem_volt\n", ((char *)(det->name) + 8));
				cpu_in_mon = 1;
			}
			eem_set_eem_volt(det);
		}
	#else
		eem_set_eem_volt(det);

		if (det->ctrl_id == EEM_CTRL_BIG)
			infoIdvfs = 0xff;

		#if ITurbo
		if (det->ctrl_id == EEM_CTRL_L)
			ctrl_ITurbo = (ctrl_ITurbo == 0) ? 0 : 2;
		#endif

	#endif

out:
	/* Clear EEM INIT interrupt EEMINTSTS = 0x00ff0000 */
	eem_write(EEMINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct eem_det *det)
{
	#if DUMP_DATA_TO_DE
		unsigned int i;
	#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

	/* EEM Monitor mode error handler */
	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMCORESEL(%p) = 0x%08X, EEM_THERMINTST(%p) = 0x%08X, EEMODINTST(%p) = 0x%08X",
		     EEMCORESEL, eem_read(EEMCORESEL),
		     EEM_THERMINTST, eem_read(EEM_THERMINTST),
		     EEMODINTST, eem_read(EEMODINTST));
	eem_error(" EEMINTSTSRAW(%p) = 0x%08X, EEMINTEN(%p) = 0x%08X\n",
		     EEMINTSTSRAW, eem_read(EEMINTSTSRAW),
		     EEMINTEN, eem_read(EEMINTEN));
	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMEN(%p) = 0x%08X, EEMINTSTS(%p) = 0x%08X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_error("EEM_SMSTATE0 (%p) = 0x%08X\n",
		     EEM_SMSTATE0, eem_read(EEM_SMSTATE0));
	eem_error("EEM_SMSTATE1 (%p) = 0x%08X\n",
		     EEM_SMSTATE1, eem_read(EEM_SMSTATE1));
	eem_error("TEMP (%p) = 0x%08X\n",
		     TEMP, eem_read(TEMP));
	eem_error("EEM_TEMPMSR0 (%p) = 0x%08X\n",
		     EEM_TEMPMSR0, eem_read(EEM_TEMPMSR0));
	eem_error("EEM_TEMPMSR1 (%p) = 0x%08X\n",
		     EEM_TEMPMSR1, eem_read(EEM_TEMPMSR1));
	eem_error("EEM_TEMPMSR2 (%p) = 0x%08X\n",
		     EEM_TEMPMSR2, eem_read(EEM_TEMPMSR2));
	eem_error("EEM_TEMPMONCTL0 (%p) = 0x%08X\n",
		     EEM_TEMPMONCTL0, eem_read(EEM_TEMPMONCTL0));
	eem_error("EEM_TEMPMSRCTL1 (%p) = 0x%08X\n",
		     EEM_TEMPMSRCTL1, eem_read(EEM_TEMPMSRCTL1));
	eem_error("====================================================\n");

	#if DUMP_DATA_TO_DE
		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][EEM_PHASE_MON] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
			eem_error("0x%lx = 0x%08x\n",
				(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
				det->reg_dump_data[i][EEM_PHASE_MON]
				);
		}
	#endif

	eem_error("====================================================\n");
	eem_error("EEM mon err: EEMCORESEL(%p) = 0x%08X, EEM_THERMINTST(%p) = 0x%08X, EEMODINTST(%p) = 0x%08X",
		     EEMCORESEL, eem_read(EEMCORESEL),
		     EEM_THERMINTST, eem_read(EEM_THERMINTST),
		     EEMODINTST, eem_read(EEMODINTST));
	eem_error(" EEMINTSTSRAW(%p) = 0x%08X, EEMINTEN(%p) = 0x%08X\n",
		     EEMINTSTSRAW, eem_read(EEMINTSTSRAW),
		     EEMINTEN, eem_read(EEMINTEN));
	eem_error("====================================================\n");

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n", __func__, __LINE__, det->name, det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void eem_isr_handler(struct eem_det *det)
{
	unsigned int eemintsts, eemen;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eemintsts = eem_read(EEMINTSTS);
	eemen = eem_read(EEMEN);

	/* eem_debug("[eem_isr_handler]Bk_No = %d %s-isr, 0x%X, 0x%X\n",
	*	det->ctrl_id, ((char *)(det->name) + 8), eemintsts, eemen);
	*/

	if (eemintsts == 0x1) { /* EEM init1 or init2 */
		if ((eemen & 0x7) == 0x1) /* EEM init1 */
			handle_init01_isr(det);
		else if ((eemen & 0x7) == 0x5) /* EEM init2 */
			handle_init02_isr(det);
		else {
			/* error : init1 or init2 */
			handle_init_err_isr(det);
		}
	} else if ((eemintsts & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else { /* EEM error handler */
		/* init 1  || init 2 error handler */
		if (((eemen & 0x7) == 0x1) || ((eemen & 0x7) == 0x5))
			handle_init_err_isr(det);
		else /* EEM Monitor mode error handler */
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#ifdef __KERNEL__
static irqreturn_t eem_isr(int irq, void *dev_id)
#else
int ptp_isr(void)
#endif
{
	unsigned long flags;
	struct eem_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	/* mt_ptp_lock(&flags); */

	#if 0
	if (!(BIT(EEM_CTRL_VCORE) & eem_read(EEMODINTST))) {
		switch (eem_read_field(PERI_VCORE_PTPOD_CON0, VCORE_PTPODSEL)) {
		case SEL_VCORE_AO:
			det = &eem_detectors[PTP_DET_VCORE_AO];
			break;

		case SEL_VCORE_PDN:
			det = &eem_detectors[PTP_DET_VCORE_PDN];
			break;
		}

		if (likely(det)) {
			det->ops->switch_bank(det, NR_EEM_PHASE);
			eem_isr_handler(det);
		}
	}
	#endif

	for (i = 0; i < NR_EEM_CTRL; i++) {

	#if 0 /* already use EEM_BANK_SOC */
		#if !(DVT) /* DVT will test EEM_CTRL_SOC */
		if (i == EEM_CTRL_SOC)
			continue;
		#endif
	#endif

		mt_ptp_lock(&flags);
		/* TODO: FIXME, it is better to link i @ struct eem_det */
		if ((BIT(i) & eem_read(EEMODINTST))) {
			mt_ptp_unlock(&flags);
			continue;
		}

		det = &eem_detectors[i];

		det->ops->switch_bank(det, NR_EEM_PHASE);

		/*mt_eem_reg_dump_locked(); */

		eem_isr_handler(det);
		mt_ptp_unlock(&flags);
	}

	/* mt_ptp_unlock(&flags); */

	FUNC_EXIT(FUNC_LV_MODULE);
#ifdef __KERNEL__
	return IRQ_HANDLED;
#else
	return 0;
#endif
}
#if ITurbo
#ifdef __KERNEL__
#define ITURBO_CPU_NUM 1
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,
					unsigned long action, void *hcpu)
{
	unsigned long flags;
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	struct eem_det *det;
	enum mt_eem_cpu_id cluster_id;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask eem_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus, big_cpus;

	if (ctrl_ITurbo < 2) {
		eem_debug("Default I-Turbo off (%d) !!", ctrl_ITurbo);
		return NOTIFY_OK;
	}
	eem_debug("I-Turbo start to run (%d) !!", ctrl_ITurbo);

	/* Current active CPU is belong which cluster */
	cluster_id = arch_get_cluster_id(cpu);

	/* How many active CPU in this cluster, present by bit mask
	*	ex:	B	L	LL
	*		00	1111	0000
	*/
	arch_get_cluster_cpus(&eem_cpumask, cluster_id);

	/* How many active CPU online in this cluster, present by number */
	cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
	cpus = cpumask_weight(&cpu_online_cpumask);

	if (eem_log_en)
		eem_error("@%s():%d, cpu = %d, act = %lu, on_cpus = %d, clst = %d, clst_cpu = %d\n"
		, __func__, __LINE__, cpu, action, online_cpus, cluster_id, cpus);

	dev = get_cpu_device(cpu);

	if (dev) {
		det = id_to_eem_det(EEM_DET_BIG);
		arch_get_cluster_cpus(&eem_cpumask, MT_EEM_CPU_B);
		cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
		big_cpus = cpumask_weight(&cpu_online_cpumask);

		switch (action) {
		case CPU_POST_DEAD:
			if ((ITurboRun == 0) && (big_cpus == ITURBO_CPU_NUM) && (cluster_id == MT_EEM_CPU_B)) {
				if (eem_log_en)
					eem_debug("Turbo(1) DEAD (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 1;
				/* Revise BIG private table */
				/* [25:21]=dcmdiv, [20:12]=DDS, [11:7]=clkdiv, [6:4]=postdiv, [3:0]=CFindex */
#if 0 /* no record table */
				det->recordRef[0] =
					((*(recordTbl + (48 * 8) + 0) & 0x1F) << 21) |
					((*(recordTbl + (48 * 8) + 1) & 0x1FF) << 12) |
					((*(recordTbl + (48 * 8) + 2) & 0x1F) << 7) |
					((*(recordTbl + (48 * 8) + 3) & 0x7) << 4) |
					(*(recordTbl + (48 * 8) + 4) & 0xF);
#endif
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! DEAD (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;

		case CPU_DOWN_PREPARE:
			if ((ITurboRun == 1) && (big_cpus == ITURBO_CPU_NUM) && (cluster_id == MT_EEM_CPU_B)) {
				if (eem_log_en)
					eem_error("Turbo(0) DP (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 0;
				/* Restore BIG private table */
#if 0 /* no record table */
				det->recordRef[0] =
					((*(recordTbl + (48 * 8) + 0) & 0x1F) << 21) |
					((*(recordTbl + (48 * 8) + 1) & 0x1FF) << 12) |
					((*(recordTbl + (48 * 8) + 2) & 0x1F) << 7) |
					((*(recordTbl + (48 * 8) + 3) & 0x7) << 4) |
					(*(recordTbl + (48 * 8) + 4) & 0xF);
#endif
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! DP (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;

		case CPU_UP_PREPARE:
			if ((ITurboRun == 0) && (cpu == 8) && (big_cpus == 0)) {
				eem_error("Turbo(1) UP (%d), BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 1;
				/* Revise BIG private table */
#if 0 /* no record table*/
				det->recordRef[0] =
					((*(recordTbl + (48 * 8) + 0) & 0x1F) << 21) |
					((*(recordTbl + (48 * 8) + 1) & 0x1FF) << 12) |
					((*(recordTbl + (48 * 8) + 2) & 0x1F) << 7) |
					((*(recordTbl + (48 * 8) + 3) & 0x7) << 4) |
					(*(recordTbl + (48 * 8) + 4) & 0xF);
#endif
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
			} else if ((ITurboRun == 1) && (big_cpus == ITURBO_CPU_NUM) && (cluster_id == MT_EEM_CPU_B)) {
				if (eem_log_en)
					eem_error("Turbo(0) UP (%d), BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 0;
				/* Restore BIG private table */
#if 0 /* no record table */
				det->recordRef[0] =
					((*(recordTbl + (48 * 8) + 0) & 0x1F) << 21) |
					((*(recordTbl + (48 * 8) + 1) & 0x1FF) << 12) |
					((*(recordTbl + (48 * 8) + 2) & 0x1F) << 7) |
					((*(recordTbl + (48 * 8) + 3) & 0x7) << 4) |
					(*(recordTbl + (48 * 8) + 4) & 0xF);
#endif
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! UP (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;
		}
	}
	return NOTIFY_OK;
}
#endif /* ifdef __KERNEL__*/
#endif /* if ITurbo */

void eem_init02(void)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_INIT02)) {
			unsigned long flag;

			mt_ptp_lock(&flag);
			det->ops->init02(det);
			mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void eem_init01(void)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		unsigned long flag;
		unsigned int vboot = 0;

		if (HAS_FEATURE(det, FEA_INIT01)) {
			if (det->ops->get_volt != NULL) {
				vboot = det->ops->volt_2_eem(det, det->ops->get_volt(det));

				eem_debug("[%s]get_volt=%d\n", ((char *)(det->name) + 8),
							det->ops->get_volt(det));

				#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* irene */
					aee_rr_rec_ptp_vboot(
						((unsigned long long)(vboot) << (8 * det->ctrl_id)) |
						(aee_rr_curr_ptp_vboot() & ~
							((unsigned long long)(0xFF) << (8 * det->ctrl_id))
						)
					);
				#endif
			}

			eem_error("%s, vboot = %d, VBOOT = %d\n", ((char *)(det->name) + 8), vboot, det->VBOOT);

			#ifndef EARLY_PORTING
				#ifdef __KERNEL__
				if (vboot != det->VBOOT) {
					eem_error("@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
							__func__, __LINE__, det->name, vboot, det->VBOOT);
					/*
					*aee_kernel_warning("mt_eem",
					*	"@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
					*	__func__, __LINE__, det->name, vboot, det->VBOOT);
					*/
				}
				WARN_ON(vboot != det->VBOOT); /* BUG_ON(vboot != det->VBOOT);*/
				#endif
			#endif

			mt_ptp_lock(&flag); /* <-XXX */
			det->ops->init01(det);
			mt_ptp_unlock(&flag); /* <-XXX */

			/*
			 * VCORE_AO and VCORE_PDN use the same controller.
			 * Wait until VCORE_AO init01 and init02 done
			 */

			/*
			*if (atomic_read(&ctrl->in_init)) {
			*	TODO: Use workqueue to avoid blocking
			*	wait_for_completion(&ctrl->init_done);
			*}
			*/
		}
	}

	/* Fixme: set non PWM mode for mt6313 */
	#ifndef EARLY_PORTING
		#if 0
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
		mt6313_set_mode(VDVFS11, 0); /* set non PWM mode for Big, LL, CCI */
		mt6313_set_mode(VDVFS13, 0); /* set non PWM mode for L*/
		mt6313_set_mode(VDVFS14, 0); /* set non PWM mode for GPU*/
		eem_debug("mt6313 set non pwm mode\n");
		#endif
		#endif
		regulator_set_mode(eem_regulator_proc1, REGULATOR_MODE_NORMAL);
		regulator_set_mode(eem_regulator_proc2, REGULATOR_MODE_NORMAL);
		regulator_set_mode(eem_regulator_gpu, REGULATOR_MODE_NORMAL);
		eem_debug("pmic set non pwm mode\n");

		#if DVT /* if DVT, set non pwm mode for soc*/
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6335)
			buck_set_mode(VCORE, 0);
			#endif
		#endif /* if DVT */
	#endif /* ifndef EARLY_PORTING */

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			#if 0
			#if !defined(CONFIG_MTK_CLKMGR)
				clk_disable_unprepare(clk_mfg_main); /* Disable MFGCFG_BG3D */
				clk_disable_unprepare(clk_mfg_core0); /* Disable MFG_CORE0 */
				clk_disable_unprepare(clk_mfg_core1); /* Disable MFG_CORE1 */
				clk_disable_unprepare(clk_mfg_core2); /* Disable MFG_CORE2 */
				clk_disable_unprepare(clk_mfg_core3); /* Disable MFG_CORE3 */
				clk_disable_unprepare(clk_mfg); /* Disable MFG */
				clk_disable_unprepare(clk_mfg_async); /* Disable MFG_ASYNC */
				/*clk_disable_unprepare(clk_thermal);*/  /* bypass this due to crash*/
				eem_debug("gpu clock disable success(thermal clock non disable)\n");
			#endif
			#endif

			#ifndef EARLY_PORTING_GPU
			mt_gpufreq_enable_by_ptpod(); /* enable gpu DVFS */
			#endif

			#ifndef EARLY_PORTING_PPM
			mt_ppm_ptpod_policy_deactivate();
			#endif

			/* call hotplug api to disable L bulk */

			/* enable frequency hopping (main PLL) */
			/* mt_fh_popod_restore(); */
		#endif
	#endif

	/* informGpuEEMisReady = 1;*/

	/* This patch is waiting for whole bank finish the init01 then go
	 * next. Due to LL/L use same bulk PMIC, LL voltage table change
	 * will impact L to process init01 stage, because L require a
	 * stable 1V for init01.
	*/
	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((det->ctrl_id == EEM_CTRL_BIG) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_BIG);
			else if ((det->ctrl_id == EEM_CTRL_GPU) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_GPU);
			else if ((det->ctrl_id == EEM_CTRL_L) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_L);
			else if ((det->ctrl_id == EEM_CTRL_2L) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_2L);
			else if ((det->ctrl_id == EEM_CTRL_CCI) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_CCI);
		}
		/* current banks: 00 0101 1111 */
		if ((out == 0x5F) || (timeout == 30)) { /* 0x3B==out */
			eem_debug("init01 finish time is %d, out=0x%x\n", timeout, out);
			break;
		}
		udelay(100);
		timeout++;
	}
	eem_init02();
	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_EEM
#if 0
static char *readline(struct file *fp)
{
#define BUFSIZE 1024
	static char buf[BUFSIZE];
	static int buf_end;
	static int line_start;
	static int line_end;
	char *ret;

	FUNC_ENTER(FUNC_LV_HELP);
empty:

	if (line_start >= buf_end) {
		line_start = 0;
		buf_end = fp->f_op->read(fp, &buf[line_end], sizeof(buf) - line_end, &fp->f_pos);

		if (buf_end == 0) {
			line_end = 0;
			return NULL;
		}

		buf_end += line_end;
	}

	while (buf[line_end] != '\n') {
		line_end++;

		if (line_end >= buf_end) {
			memcpy(&buf[0], &buf[line_start], buf_end - line_start);
			line_end = buf_end - line_start;
			line_start = buf_end;
			goto empty;
		}
	}

	buf[line_end] = '\0';
	ret = &buf[line_start];
	line_start = line_end + 1;

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif
/* leakage */
unsigned int leakage_core;
unsigned int leakage_gpu;
unsigned int leakage_sram2;
unsigned int leakage_sram1;

/* get efuse content for bit, cci, gpu , ll, l and fill in eem_devinfo*/
void get_devinfo(struct eem_devinfo *p)
{
	int *val = (int *)p;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);
	checkEfuse = 1;

/* if not for DVT & has efuse content, use get_devinfo_with_index get efuse content*/
/* else use fake efuse */
#if !DVT && EFUSE_API
	#if defined(__KERNEL__)
		val[0] = get_devinfo_with_index(50);
		val[1] = get_devinfo_with_index(51);
		val[2] = get_devinfo_with_index(52);
		val[3] = get_devinfo_with_index(53);
		val[4] = get_devinfo_with_index(56);
		val[5] = get_devinfo_with_index(57);
		val[6] = get_devinfo_with_index(58);
		val[7] = get_devinfo_with_index(59);
		val[8] = get_devinfo_with_index(60);
		val[9] = get_devinfo_with_index(61);
		#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* irene */
			aee_rr_rec_ptp_60((unsigned int)val[0]);
			aee_rr_rec_ptp_64((unsigned int)val[1]);
			aee_rr_rec_ptp_68((unsigned int)val[2]);
			aee_rr_rec_ptp_6C((unsigned int)val[3]);
			aee_rr_rec_ptp_78((unsigned int)val[4]);
			aee_rr_rec_ptp_7C((unsigned int)val[5]);
			aee_rr_rec_ptp_80((unsigned int)val[6]);
			aee_rr_rec_ptp_84((unsigned int)val[7]);
			aee_rr_rec_ptp_88((unsigned int)val[8]);
			aee_rr_rec_ptp_8C((unsigned int)val[9]);
		#endif
	#else
		val[0] = eem_read(0x102A0580); /* M_HW_RES0 */
		val[1] = eem_read(0x102A0584); /* M_HW_RES1 */
		val[2] = eem_read(0x102A0588); /* M_HW_RES2 */
		val[3] = eem_read(0x102A058C); /* M_HW_RES3 */
		val[4] = eem_read(0x102A0590); /* M_HW_RES4 */
		val[5] = eem_read(0x102A0594); /* M_HW_RES5 */
		val[6] = eem_read(0x102A0598); /* M_HW_RES6 */
		val[7] = eem_read(0x102A059C); /* M_HW_RES7 */
		val[8] = eem_read(0x102A05A0); /* M_HW_RES8 */
		val[9] = eem_read(0x102A05A4); /* M_HW_RES9 */
	#endif
#else
	/* test pattern (irene)*/
	val[0] = 0x10BD3C1B;
	val[1] = 0x005500C0;
	val[2] = 0x10BD3C1B;
	val[3] = 0x005500C0;
	val[4] = 0x10BD3C1B;
	val[5] = 0x005500C0;
	val[6] = 0x10BD3C1B;
	val[7] = 0x005500C0;
	val[8] = 0x10BD3C1B;
	val[9] = 0x005500C0;
#endif
	eem_debug("M_HW_RES0 = 0x%08X\n", val[0]);
	eem_debug("M_HW_RES1 = 0x%08X\n", val[1]);
	eem_debug("M_HW_RES2 = 0x%08X\n", val[2]);
	eem_debug("M_HW_RES3 = 0x%08X\n", val[3]);
	eem_debug("M_HW_RES4 = 0x%08X\n", val[4]);
	eem_debug("M_HW_RES5 = 0x%08X\n", val[5]);
	eem_debug("M_HW_RES6 = 0x%08X\n", val[6]);
	eem_debug("M_HW_RES7 = 0x%08X\n", val[7]);
	eem_debug("M_HW_RES8 = 0x%08X\n", val[8]);
	eem_debug("M_HW_RES9 = 0x%08X\n", val[9]);

	for (i = 0; i < NR_HW_RES; i++) {
		if (val[i] == 0) {
			ctrl_EEM_Enable = 0;
			infoIdvfs = 0x55;
			checkEfuse = 0;
			eem_error("No EEM EFUSE available, will turn off EEM (val[%d] !!\n", i);
			break;
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
}

static int eem_probe(struct platform_device *pdev)
{
	int ret;
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	/* unsigned int code = mt_get_chip_hw_code(); */

	FUNC_ENTER(FUNC_LV_MODULE);

	/* thermal and gpu may enable their clock by themselves */
	#ifdef __KERNEL__
		#if 0
		#if !defined(CONFIG_MTK_CLKMGR) && !defined(EARLY_PORTING)
			/* enable thermal CG */
			clk_thermal = devm_clk_get(&pdev->dev, "therm-eem");
			if (IS_ERR(clk_thermal)) {
				eem_error("cannot get thermal clock\n");
				return PTR_ERR(clk_thermal);
			}

			/* get GPU mtcomose */
			clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
			if (IS_ERR(clk_mfg_async)) {
				eem_error("cannot get mtcmos-mfg-async\n");
				return PTR_ERR(clk_mfg_async);
			}

			clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
			if (IS_ERR(clk_mfg)) {
				eem_error("cannot get mtcmos-mfg\n");
				return PTR_ERR(clk_mfg);
			}

			clk_mfg_core3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core3");
			if (IS_ERR(clk_mfg_core3)) {
				eem_error("cannot get mtcmos-mfg_core3\n");
				return PTR_ERR(clk_mfg_core3);
			}

			clk_mfg_core2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core2");
			if (IS_ERR(clk_mfg_core2)) {
				eem_error("cannot get mtcmos-mfg-core2\n");
				return PTR_ERR(clk_mfg_core2);
			}

			clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
			if (IS_ERR(clk_mfg_core1)) {
				eem_error("cannot get mtcmos-mfg-core1\n");
				return PTR_ERR(clk_mfg_core1);
			}

			clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
			if (IS_ERR(clk_mfg_core0)) {
				eem_error("cannot get mtcmos-mfg-core0\n");
				return PTR_ERR(clk_mfg_core0);
			}

			/* get GPU clock */
			clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");
			if (IS_ERR(clk_mfg_main)) {
				eem_error("cannot get mfg-main\n");
				return PTR_ERR(clk_mfg_main);
			}
			/*
			*eem_debug("thmal=%p, gpu_clk=%p, gpu_mtcmos=%p",
			*	clk_thermal,
			*	clk_mfg_main,
			*	clk_mfg_async);
			*/
		#endif
		#endif /* #if 0 */

		/* set EEM IRQ */
		ret = request_irq(eem_irq_number, eem_isr, IRQF_TRIGGER_LOW, "eem", NULL);
		if (ret) {
			eem_error("EEM IRQ register failed (%d)\n", ret);
			WARN_ON(1);
		}
		eem_debug("Set EEM IRQ OK.\n");
	#endif

	#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
		_mt_eem_aee_init();
	#endif

	for_each_ctrl(ctrl)
		eem_init_ctrl(ctrl);

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			/* disable frequency hopping (main PLL) */
			/* mt_fh_popod_save(); */

			/* call hotplug api to enable L bulk */

			/* disable DVFS and set vproc = 1v (LITTLE = 689 MHz)(BIG = 1196 MHz) */
			#ifndef EARLY_PORTING_PPM
			mt_ppm_ptpod_policy_activate();
			#endif

			#ifndef EARLY_PORTING_GPU
			mt_gpufreq_disable_by_ptpod();
			#endif

			#if 0
			#if !defined(CONFIG_MTK_CLKMGR)
				eem_debug("enable clk_thermal...!\n");
				ret = clk_prepare_enable(clk_thermal); /* Thermal clock enable */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling THERMAL\n");

				ret = clk_prepare_enable(clk_mfg_async); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_async\n");

				ret = clk_prepare_enable(clk_mfg); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg\n");

				ret = clk_prepare_enable(clk_mfg_core3); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core3\n");

				ret = clk_prepare_enable(clk_mfg_core2); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core2\n");

				ret = clk_prepare_enable(clk_mfg_core1); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core1\n");

				ret = clk_prepare_enable(clk_mfg_core0); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core0\n");

				ret = clk_prepare_enable(clk_mfg_main); /* GPU CLOCK */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_main\n");
			#endif /* if !defined CONFIG_MTK_CLKMGR */
			#endif /* if 0 */
		#endif/* ifndef early porting*/
	#endif/*ifdef __KERNEL__*/

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
		/*
		*extern unsigned int ckgen_meter(int val);
		*eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
		*	__func__, ckgen_meter(1), ckgen_meter(2));
		*/
	#endif

	/* Fixme: set PWM mode for mt6313 */
	#ifndef EARLY_PORTING
		#if 0
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
		eem_debug("mt6313 set pwm mode\n");
		mt6313_set_mode(VDVFS11, 1); /* set PWM mode for Big, LL, CCI*/
		mt6313_set_mode(VDVFS13, 1); /* set PWM mode for L*/
		mt6313_set_mode(VDVFS14, 1); /* set PWM mode for GPU*/
		#endif
		#endif
		/* get regulator reference */
		/* proc1: big, cci, ll */
		eem_regulator_proc1 = regulator_get(&pdev->dev, "ext_buck_proc1");
		if (eem_regulator_proc1 == NULL) {
			eem_error("regulotor_proc1 error\n");
			return -EINVAL;
		}
		/* proc2: l */
		eem_regulator_proc2 = regulator_get(&pdev->dev, "ext_buck_proc2");
		if (eem_regulator_proc2 == NULL) {
			eem_error("regulotor_proc2 error\n");
			return -EINVAL;
		}
		eem_regulator_gpu = regulator_get(&pdev->dev, "ext_buck_gpu");
		if (eem_regulator_gpu == NULL) {
			eem_error("regulotor_gpu error\n");
			return -EINVAL;
		}
		/* set pwm mode for each buck */
		eem_debug("pmic set pwm mode\n");
		regulator_set_mode(eem_regulator_proc1, REGULATOR_MODE_FAST);
		regulator_set_mode(eem_regulator_proc2, REGULATOR_MODE_FAST);
		regulator_set_mode(eem_regulator_gpu, REGULATOR_MODE_FAST);

		#if DVT /* if DVT, set pwm mode for soc to run ptp soc det */
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6335)
			buck_set_mode(VCORE, 1); /* 1: Force PWM mdoe */
			#endif
		#endif /* if DVT */
	#endif /* ifndef EARLY_PORTING */

	/* for slow idle */
	ptp_data[0] = 0xffffffff;

	for_each_det(det)
		eem_init_det(det, &eem_devinfo);

	/* get original volt from cpu dvfs before init01*/
	for_each_det(det) {
		if ((det->ctrl_id != EEM_CTRL_SOC) && (det->ctrl_id != EEM_CTRL_GPU))
			det->ops->get_orig_volt_table(det);
	}
	eem_debug("get orig cpu volt table done!\n");

	#ifdef __KERNEL__
	/* mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcEEM);*/ /* no record table */
	eem_init01();
	#endif
	ptp_data[0] = 0;

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
		/*
		*unsigned int ckgen_meter(int val);
		*eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
		*	__func__,
		*	ckgen_meter(1),
		*	ckgen_meter(2));
		*/
	#endif

    #if ITurbo
	register_hotcpu_notifier(&_mt_eem_cpu_notifier);
	#endif

	eem_debug("eem_probe ok\n");
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	*kthread_stop(eem_volt_thread);
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_resume(struct platform_device *pdev)
{
	#if 0
	eem_volt_thread = kthread_run(eem_volt_thread_handler, 0, "eem volt");
	if (IS_ERR(eem_volt_thread))
		eem_debug("[%s]: failed to create eem volt thread\n", __func__);
	#endif

	FUNC_ENTER(FUNC_LV_MODULE);

#if 0
	#ifdef __KERNEL__
	mt_cpufreq_eem_resume();
	#endif
#endif

	eem_init02();
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_eem_of_match[] = {
	{ .compatible = "mediatek,eem_fsm", },
	{},
};
#endif

static struct platform_driver eem_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = eem_probe,
	.suspend    = eem_suspend,
	.resume     = eem_resume,
	.driver     = {
		.name   = "mt-eem",
#ifdef CONFIG_OF
		.of_match_table = mt_eem_of_match,
#endif
	},
};

#ifndef __KERNEL__
/*
 * For CTP
 */
int no_efuse;

int can_not_read_efuse(void)
{
	return no_efuse;
}

int mt_ptp_probe(void)
{
	eem_debug("CTP - In mt_eem_probe\n");
	no_efuse = eem_init();

#if defined(__MTK_SLT_)
	if (no_efuse == -1)
		return 0;
#endif
	eem_probe(NULL);
	return 0;
}

void eem_init01_ctp(unsigned int id)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_INIT01)) {
			unsigned long flag;

			eem_debug("CTP - (%d:%d)\n",
				det->ops->volt_2_eem(det, det->ops->get_volt(det)),
				det->VBOOT);

			mt_ptp_lock(&flag);
			det->ops->init01(det);
			mt_ptp_unlock(&flag);
		}
	}

	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((det->ctrl_id == EEM_CTRL_BIG) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_BIG);
			else if ((det->ctrl_id == EEM_CTRL_GPU) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_GPU);
			else if ((det->ctrl_id == EEM_CTRL_L) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_L);
			else if ((det->ctrl_id == EEM_CTRL_2L) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_2L);
			else if ((det->ctrl_id == EEM_CTRL_CCI) && (det->eem_eemEn[EEM_PHASE_INIT01] == 1))
				out |= BIT(EEM_CTRL_CCI);
		}
		if ((out == 0x1F) || (timeout == 1000)) {
			eem_error("init01 finish time is %d\n", timeout);
			break;
		}
		udelay(100);
		timeout++;
	}
	/*
	*thermal_init();
	*udelay(500);
	*/
	eem_init02();

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init01_ptp(int id)
{
	eem_init01_ctp(id);
}

#endif /* ifndef __KERNEL__ */

#ifdef CONFIG_PROC_FS
int mt_eem_opp_num(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);
	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_eem_opp_num);

void mt_eem_opp_freq(enum eem_det_id id, unsigned int *freq)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	for (i = 0; i < det->num_freq_tbl; i++)
		freq[i] = det->freq_tbl[i];

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_freq);

void mt_eem_opp_status(enum eem_det_id id, unsigned int *temp, unsigned int *volt)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

#if defined(__KERNEL__) && defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	*temp = tscpu_get_temp_by_bank(id);
#else
	*temp = 0;
#endif
	for (i = 0; i < det->num_freq_tbl; i++)
		volt[i] = det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_status);

/***************************
* return current EEM stauts
****************************/
int mt_eem_status(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);

	WARN_ON(!det); /*BUG_ON(!det);*/
	WARN_ON(!det->ops); /*BUG_ON(!det->ops);*/
	WARN_ON(!det->ops->get_status); /* BUG_ON(!det->ops->get_status);*/

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}

/**
 * ===============================================
 * PROCFS interface for debugging
 * ===============================================
 */

/*
 * show current EEM stauts
 */
static int eem_debug_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	/* FIXME: EEMEN sometimes is disabled temp */
	seq_printf(m, "[%s] %s (%d)\n",
		   ((char *)(det->name) + 8),
		   det->disabled ? "disabled" : "enable",
		   det->ops->get_status(det)
		   );

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM status by procfs interface
 */
static ssize_t eem_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;

		if (enabled == 0)
			/* det->ops->enable(det, BY_PROCFS); */
			det->ops->disable(det, 0);
		else if (enabled == 1)
			det->ops->disable(det, BY_PROCFS);
		else if (enabled == 2)
			det->ops->disable(det, BY_PROCFS_INIT2);
	} else
		ret = -EINVAL;

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current EEM data
 */
static int eem_dump_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det;
	/* int *val = (int *)&eem_devinfo; */
	int i, k;
	#if DUMP_DATA_TO_DE
		int j;
	#endif

	FUNC_ENTER(FUNC_LV_HELP);
	#if 0
	eem_detectors[EEM_DET_BIG].ops->dump_status(&eem_detectors[EEM_DET_BIG]);
	eem_detectors[EEM_DET_L].ops->dump_status(&eem_detectors[EEM_DET_L]);
	seq_printf(m, "det->EEMMONEN= 0x%08X,det->EEMINITEN= 0x%08X\n", det->EEMMONEN, det->EEMINITEN);
	seq_printf(m, "leakage_core\t= %d\n"
			"leakage_gpu\t= %d\n"
			"leakage_little\t= %d\n"
			"leakage_big\t= %d\n",
			leakage_core,
			leakage_gpu,
			leakage_sram2,
			leakage_sram1
			);
	#endif

	/*
	*for (i = 0; i < NR_HW_RES; i++)
	*	seq_printf(m, "M_HW_RES%d\t= 0x%08X\n", i, get_devinfo_with_index(50 + i));
	*/

	for_each_det(det) {
		if (det->ctrl_id == EEM_CTRL_SOC)
			continue;
		for (i = EEM_PHASE_INIT01; i < NR_EEM_PHASE; i++) {
			seq_printf(m, "Bank_number = %d\n", det->ctrl_id);
			if (i < EEM_PHASE_MON)
				seq_printf(m, "mode = init%d\n", i+1);
			else
				seq_puts(m, "mode = mon\n");
			if (eem_log_en) {
				seq_printf(m, "0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
					det->dcvalues[i],
					det->freqpct30[i],
					det->eem_26c[i],
					det->vop30[i],
					det->eem_eemEn[i]
				);

				if (det->eem_eemEn[i] == 0x5) {
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d) - (",
					det->ctrl_id, det->ops->get_temp(det));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ",
						det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k]));
					seq_printf(m, "%d) - (", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k]));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ", det->freq_tbl[k]);
					seq_printf(m, "%d)\n", det->freq_tbl[k]);
				}
			} /* if (eem_log_en) */
			#if DUMP_DATA_TO_DE
			for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
				seq_printf(m, "0x%08lx = 0x%08x\n",
					(unsigned long)EEM_BASEADDR + reg_dump_addr_off[j],
					det->reg_dump_data[j][i]
					);
			#endif
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
/*
 * show current voltage
 */
static int eem_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;
	u32 rdata = 0, i;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "EEM[%s] read current voltage fail\n", det->name);

	if ((det->ctrl_id != EEM_CTRL_GPU) && (det->ctrl_id != EEM_CTRL_SOC)) {
		for (i = 0; i < det->num_freq_tbl; i++)
			seq_printf(m, "EEM_HW, det->volt_tbl[%d] = [%x], det->volt_tbl_pmic[%d] = [%x]\n",
			i, det->volt_tbl[i], i, det->volt_tbl_pmic[i]);
		#if 0 /* no record table */
		for (i = 0; i < NR_FREQ; i++) {
			seq_printf(m, "(iDVFS, 0x%x)(Vs, 0x%x) (Vp, 0x%x, %d) (F_Setting)(%x, %x, %x, %x, %x)\n",
				(det->recordRef[i*2] >> 14) & 0x3FFFF,
				(det->recordRef[i*2] >> 7) & 0x7F,
				det->recordRef[i*2] & 0x7F,
				det->ops->pmic_2_volt(det, (det->recordRef[i*2] & 0x7F)),
				(det->recordRef[i*2+1] >> 21) & 0x1F,
				(det->recordRef[i*2+1] >> 12) & 0x1FF,
				(det->recordRef[i*2+1] >> 7) & 0x1F,
				(det->recordRef[i*2+1] >> 4) & 0x7,
				det->recordRef[i*2+1] & 0xF
				);
		}
		#endif
	}
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
#if 0 /* no record table */
/*
 * set secure DVFS by procfs
 */
static ssize_t eem_cur_volt_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int voltValue = 0, voltProc = 0, voltSram = 0, voltPmic = 0, index = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	/* if (!kstrtoint(buf, 10, &voltValue)) { */
	if (sscanf(buf, "%u %u", &voltValue, &index) == 2) {
		if ((det->ctrl_id != EEM_CTRL_GPU) && (det->ctrl_id != EEM_CTRL_SOC)) {
			ret = 0;
			det->recordRef[NR_FREQ * 2] = 0x00000000;
			mb(); /* SRAM writing */
			voltPmic = det->ops->volt_2_pmic(det, voltValue);
			if (det->ctrl_id == EEM_CTRL_BIG)
				voltProc = clamp(
				(unsigned int)voltPmic,
				(unsigned int)det->VMIN,
				(unsigned int)det->VMAX);
			else
				voltProc = clamp(
				(unsigned int)voltPmic,
				(unsigned int)(det->ops->eem_2_pmic(det, det->VMIN)),
				(unsigned int)(det->ops->eem_2_pmic(det, det->VMAX)));

			voltPmic = det->ops->volt_2_pmic(det, voltValue + 10000);
			voltSram = clamp(
				(unsigned int)(voltPmic),
				(unsigned int)(det->ops->volt_2_pmic(det, VMIN_SRAM)),
				(unsigned int)(det->ops->volt_2_pmic(det, VMAX_SRAM)));

			/* for (i = 0; i < NR_FREQ; i++) */
			det->recordRef[index * 2] = (det->recordRef[index * 2]  & (~0x3FFF)) |
				(((PMIC_2_RMIC(det, voltSram) & 0x7F) << 7) | (voltProc & 0x7F));

			det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
			mb(); /* SRAM writing */
		}
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! voltage = (80000 ~ 115500), index = (0 ~ 15)\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif /* no record table */
/*
 * show current EEM status
 */
static int eem_status_proc_show(struct seq_file *m, void *v)
{
	int i;
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "bank = %d, (%d) - (",
		   det->ctrl_id, det->ops->get_temp(det));
	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	seq_printf(m, "%d) - (", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));

	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->freq_tbl[i]);
	seq_printf(m, "%d)\n", det->freq_tbl[i]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
/*
 * set EEM log enable by procfs interface
 */

static int eem_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", eem_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (kstrtoint(buf, 10, &eem_log_en)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (eem_log_en) {
	case 0:
		eem_debug("eem log disabled.\n");
		hrtimer_cancel(&eem_log_timer);
		break;

	case 1:
		eem_debug("eem log enabled.\n");
		hrtimer_start(&eem_log_timer, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show EEM offset
 */
static int eem_offset_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "%d\n", det->volt_offset);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM offset by procfs
 */
static ssize_t eem_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &offset)) {
		ret = 0;
		det->volt_offset = offset;
		mt_ptp_lock(&flags);
		eem_set_eem_volt(det);
		mt_ptp_unlock(&flags);
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * set EEM ITurbo enable by procfs interface
 */
#if ITurbo
static int eem_iturbo_en_proc_show(struct seq_file *m, void *v)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%s\n", ((ctrl_ITurbo) ? "Enable" : "Disable"));
	for (i = 0; i < NR_FREQ; i++)
		seq_printf(m, "ITurbo_offset[%d] = %d (0.00625 per step)\n", i, ITurbo_offset[i]);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_iturbo_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	unsigned int value;
	char *buf = (char *) __get_free_page(GFP_USER);

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (kstrtoint(buf, 10, &value)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (value) {
	case 0:
		eem_debug("eem ITurbo disabled.\n");
		ctrl_ITurbo = 0;
		break;

	case 1:
		eem_debug("eem ITurbo enabled.\n");
		ctrl_ITurbo = 2;
		break;

	default:
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif /* if ITurbo */

static int eem_vcore_volt_proc_show(struct seq_file *m, void *v)
{
	unsigned int i = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	for (i = 0; i < VCORE_NR_FREQ; i++) {
		/* transfer 10uv to uv before showing*/
		seq_printf(m, "%d ", VCORE_PMIC_2_VOLT(get_vcore_ptp_volt(i)) * 10);
		eem_debug("eem_vcore %d\n", get_vcore_ptp_volt(i) * 10);
	}
	seq_puts(m, "\n");

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_vcore_volt_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int index = 0;
	unsigned int newVolt = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (sscanf(buf, "%u %u", &index, &newVolt) == 2) {
		/* transfer uv to 10uv */
		if (newVolt > 10)
			newVolt = newVolt / 10;

		ret = mt_eem_update_vcore_volt(index, newVolt);
		if (ret == 1) {
			ret = -EINVAL;
			if (index == 0)
				eem_error("volt should be set larger than index %d\n", index+1);
			else if (index == VCORE_NR_FREQ-1)
				eem_error("volt should be set smaller than index %d\n", index-1);
			else
				eem_error("volt should be set between index %d - %d\n", index-1, index+1);
		}
	} else {
		ret = -EINVAL;
		eem_error("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#if 0
static int eem_vcore_enable_proc_show(struct seq_file *m, void *v)
{

	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", ctrl_VCORE_Volt_Enable);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_vcore_enable_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int enable = 0;
	int i = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (enable) {
	case 0:
		eem_debug("vcore eem disabled! Restore to signed off volt.\n");
		ctrl_VCORE_Volt_Enable = 0;
		/* restore vore volt to signed off volt */
		for (i = 0; i < VCORE_NR_FREQ; i++)
			mt_eem_update_vcore_volt(i, vcore_opp[i][0]);
		break;

	case 1:
		eem_debug("vcore eem enabled. Reset to voltage bin volt\n");
		ctrl_VCORE_Volt_Enable = 1;
		/* reset vcore volt to voltage bin value */
		for (i = 0; i < VCORE_NR_FREQ; i++)
			mt_eem_update_vcore_volt(i, vcore_opp[i][eem_vcore_index[i]]);
		/* bottom up compare each volt to ensure each opp is in descending order */
		for (i = VCORE_NR_FREQ - 2; i >= 0; i--) {
			eem_debug("i=%d\n", i);
			eem_vcore[i] = (eem_vcore[i] < eem_vcore[i+1]) ? eem_vcore[i+1] : eem_vcore[i];
			eem_debug("final eem_vcore[%d]=%d\n", i, eem_vcore[i]);
		}
		break;

	default:
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(eem_debug);
PROC_FOPS_RO(eem_status);
PROC_FOPS_RO(eem_cur_volt);
PROC_FOPS_RW(eem_offset);
PROC_FOPS_RO(eem_dump);
PROC_FOPS_RW(eem_log_en);
PROC_FOPS_RW(eem_vcore_volt);
/* PROC_FOPS_RW(eem_vcore_enable);*/

#if ITurbo
PROC_FOPS_RW(eem_iturbo_en);
#endif

static int create_procfs(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct eem_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(eem_debug),
		PROC_ENTRY(eem_status),
		PROC_ENTRY(eem_cur_volt),
		PROC_ENTRY(eem_offset),
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_dump),
		PROC_ENTRY(eem_log_en),
		PROC_ENTRY(eem_vcore_volt),
		/* PROC_ENTRY(eem_vcore_enable),*/
		#if ITurbo
		PROC_ENTRY(eem_iturbo_en),
		#endif
	};

	FUNC_ENTER(FUNC_LV_HELP);
	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_error("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, eem_dir, eem_entries[i].fops)) {
			eem_error("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		if (det->ctrl_id == EEM_CTRL_SOC)
			continue;
		det_dir = proc_mkdir(det->name, eem_dir);

		if (!det_dir) {
			eem_debug("[%s]: mkdir /proc/eem/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data(det_entries[i].name,
					S_IRUGO | S_IWUSR | S_IWGRP,
					det_dir,
					det_entries[i].fops, det)) {
				eem_debug("[%s]: create /proc/eem/%s/%s failed\n", __func__,
					det->name, det_entries[i].name);
					FUNC_EXIT(FUNC_LV_HELP);
					return -3;
			}
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}

static int create_procfs_vcore(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_vcore_volt),
	};

	FUNC_ENTER(FUNC_LV_HELP);
	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_error("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP,
			eem_dir, eem_entries[i].fops)) {
			eem_error("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}

#endif /* CONFIG_PROC_FS */

#if 0
#define VLTE_BIN0_VMAX (121000)
#define VLTE_BIN0_VMIN (104500)
#define VLTE_BIN0_VNRL (110625)

#define VLTE_BIN1_VMAX (121000)
#define VLTE_BIN1_VMIN (102500)
#define VLTE_BIN1_VNRL (105000)

#define VLTE_BIN2_VMAX (121000)
#define VLTE_BIN2_VMIN (97500)
#define VLTE_BIN2_VNRL (100000)

#define VSOC_BIN0_VMAX (126500)
#define VSOC_BIN0_VMIN (109000)
#define VSOC_BIN0_VNRL (115000)

#define VSOC_BIN1_VMAX (126500)
#define VSOC_BIN1_VMIN (107500)
#define VSOC_BIN1_VNRL (110000)

#define VSOC_BIN2_VMAX (126500)
#define VSOC_BIN2_VMIN (102500)
#define VSOC_BIN2_VNRL (105000)

#if defined(SLT_VMAX)
#define VLTE_BIN(x) VLTE_BIN##x##_VMAX
#define VSOC_BIN(x) VSOC_BIN##x##_VMAX
#elif defined(SLT_VMIN)
#define VLTE_BIN(x) VLTE_BIN##x##_VMIN
#define VSOC_BIN(x) VSOC_BIN##x##_VMIN
#else
#define VLTE_BIN(x) VLTE_BIN##x##_VNRL
#define VSOC_BIN(x) VSOC_BIN##x##_VNRL
#endif

void process_voltage_bin(struct eem_devinfo *devinfo)
{
	switch (devinfo->LTE_VOLTBIN) {
	case 0:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, VLTE_BIN(0)); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, VLTE_BIN(0)));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(0)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(0)/100));
		break;
	case 1:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, 105000); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 105000));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(1)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(1)/100));
		break;
	case 2:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = det->ops->volt_2_pmic(det, 100000); */
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 100000));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, VLTE_BIN(2)));
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(2)/100));
		break;
	default:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				mt_cpufreq_set_lte_volt(det->ops->volt_2_pmic(det, 110625));
			#else
				dvfs_set_vlte(det->ops->volt_2_pmic(det, 110625));
			#endif
		#endif
		eem_debug("VLTE voltage bin to 1.10625V\n");
		break;
	};
	/* mt_cpufreq_set_lte_volt(det->volt_tbl_bin[0]); */

	switch (devinfo->SOC_VOLTBIN) {
	case 0:
#ifdef __MTK_SLT_
		if (!slt_is_low_vcore()) {
			dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(0)));
			eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
		} else {
			dvfs_set_vcore(det->ops->volt_2_pmic(det, 105000));
			eem_debug("VCORE voltage bin to %dmV\n", (105000/100));
		}
#else
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(0)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
#endif
		break;
	case 1:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(1)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(1)/100));
		break;
	case 2:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, VSOC_BIN(2)));
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(2)/100));
		break;
	default:
		dvfs_set_vcore(det->ops->volt_2_pmic(det, 105000));
		eem_debug("VCORE voltage bin to 1.05V\n");
		break;
	};
}
#endif

/* return vcore pmic value to vcore dvfs(pmic) */
unsigned int get_vcore_ptp_volt(unsigned int seg)
{
	unsigned int ret = 0;

	if (seg < VCORE_NR_FREQ)
		ret = eem_vcore[seg];
	else
		eem_error("VCORE: wrong segment\n");

	return ret;
}
/* update vcore voltage by index and new volt(10uv) */
static unsigned int  mt_eem_update_vcore_volt(unsigned int index, unsigned int newVolt)
{
	unsigned int ret = 0;
	unsigned int volt_pmic = VOLT_2_VCORE_PMIC(newVolt);

	FUNC_ENTER(FUNC_LV_MODULE);
	eem_debug("newVolt/pmic=%d/%d, eem_vcore[%d] = %d\n", newVolt, volt_pmic, index, volt_pmic);
	/* transfer volt to pmic value */
	if ((index == 0) && (volt_pmic >= eem_vcore[index+1]))
		eem_vcore[index] = volt_pmic;
	else if ((index == VCORE_NR_FREQ-1) && (volt_pmic <= eem_vcore[index-1]))
		eem_vcore[index] = volt_pmic;
	else if ((index > 0) && (index < VCORE_NR_FREQ-1) &&
			(volt_pmic <= eem_vcore[index-1]) &&
			(volt_pmic >= eem_vcore[index+1]))
		eem_vcore[index] = volt_pmic;
	else
		ret = 1;

	#ifndef EARLY_PORTING_VCOREFS
	ret = spm_vcorefs_pwarp_cmd();
	#endif

	FUNC_EXIT(FUNC_LV_MODULE);
	return ret;
}

#if 0
void eem_set_pi_offset(enum eem_ctrl_id id, int step)
{
	struct eem_det *det = id_to_eem_det(id);

	det->pi_offset = step;

#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)) /* irene */
	aee_rr_rec_eem_pi_offset(step);
#endif
}
#endif

unsigned int get_efuse_status(void)
{
	return checkEfuse;
}

#if 0 /* gpu dvfs no need this anymore */
unsigned int get_eem_status_for_gpu(void)
{
	/* return informGpuEEMisReady;*/
	return 0;
}
#endif

void eem_efuse_calibration(struct eem_devinfo *devinfo)
{
	int temp;

	eem_error("%s\n", __func__);

	if (devinfo->GPU_DCBDET >= 128) {
		eem_error("GPU_DCBDET = %d which is correct\n", devinfo->GPU_DCBDET);
	} else {
		isGPUDCBDETOverflow = 1;
		temp = devinfo->GPU_DCBDET;
		eem_error("GPU_DCBDET = 0x%x, (%d), (%d), (%d)\n", devinfo->GPU_DCBDET, temp, temp-256, (temp-256)/2);
		devinfo->GPU_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("GPU_DCBDET = 0x%x, (%d)\n",
			devinfo->GPU_DCBDET,
			devinfo->GPU_DCBDET);
	}

	if (devinfo->CPU_2L_DCBDET >= 128) {
		eem_error("CPU_2L_DCBDET = %d which is correct\n", devinfo->CPU_2L_DCBDET);
	} else {
		is2LDCBDETOverflow = 1;
		temp = devinfo->CPU_2L_DCBDET;
		devinfo->CPU_2L_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CPU_2L_DCBDET = 0x%x, (%d)\n",
			devinfo->CPU_2L_DCBDET,
			devinfo->CPU_2L_DCBDET);
	}

	if (devinfo->CPU_L_DCBDET >= 128) {
		eem_error("CPU_L_DCBDET = %d which is correct\n", devinfo->CPU_L_DCBDET);
	} else {
		isLDCBDETOverflow = 1;
		temp = devinfo->CPU_L_DCBDET;
		devinfo->CPU_L_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CPU_L_DCBDET = 0x%x, (%d)\n",
			devinfo->CPU_L_DCBDET,
			devinfo->CPU_L_DCBDET);
	}

	if (devinfo->CCI_DCBDET >= 128) {
		eem_error("CPU_CCI_DCBDET = %d which is correct\n", devinfo->CCI_DCBDET);
	} else {
		isCCIDCBDETOverflow = 1;
		temp = devinfo->CCI_DCBDET;
		devinfo->CCI_DCBDET = (unsigned char)((temp - 256) / 2);
		eem_error("CCI_DCBDET = 0x%x, (%d)\n",
			devinfo->CCI_DCBDET,
			devinfo->CCI_DCBDET);
	}
}

#ifdef __KERNEL__
static int __init dt_get_ptp_devinfo(unsigned long node, const char *uname, int depth, void *data)
{
	#if 0
	struct devinfo_ptp_tag *tags;
	unsigned int size = 0;

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	tags = (struct devinfo_ptp_tag *) of_get_flat_dt_prop(node, "atag,ptp", &size);

	if (tags) {
		vcore0 = tags->volt0;
		vcore1 = tags->volt1;
		vcore2 = tags->volt2;
		eem_debug("[EEM][VCORE] - Kernel Got from DT (0x%0X, 0x%0X, 0x%0X)\n",
			vcore0, vcore1, vcore2);
	}
	#endif
	unsigned int soc_efuse;
	int i = 0;

	FUNC_ENTER(FUNC_LV_MODULE);

	/* Read EFUSE */
	/* soc_efuse = get_devinfo_with_index(54);*/
	soc_efuse = 0;
	eem_error("[VCORE] - Kernel Got efuse 0x%0X\n", soc_efuse);
	eem_vcore_index[0] = (soc_efuse >> 4) & 0x03;
	eem_vcore_index[1] = (soc_efuse >> 2) & 0x03;
	eem_vcore_index[2] = soc_efuse & 0x03;
	eem_vcore_index[3] = 0;

	for (i = 0; i < VCORE_NR_FREQ; i++)
		/* eem_vcore[i] = (vcore_opp[i][eem_vcore_index[i]] - 60000 + 625 - 1) / 625; */
		eem_vcore[i] = VOLT_2_VCORE_PMIC(vcore_opp[i][eem_vcore_index[i]]);

	/* bottom up compare each volt to ensure each opp is in descending order */
	for (i = VCORE_NR_FREQ - 2; i >= 0; i--)
		eem_vcore[i] = (eem_vcore[i] < eem_vcore[i+1]) ? eem_vcore[i+1] : eem_vcore[i];

	/*vcore1 = (vcore1 < vcore2) ? vcore2 : vcore1;*/
	/*vcore0 = (vcore0 < vcore1) ? vcore1 : vcore0;*/

	eem_error("Got vcore volt(pmic): %d %d %d %d\n",
				eem_vcore[0], eem_vcore[1], eem_vcore[2], eem_vcore[3]);

	FUNC_EXIT(FUNC_LV_MODULE);
	return 1;
}

unsigned int mt_eem_vcorefs_set_volt(void)
{
	int ret = 0;

	#ifndef EARLY_PORTING_VCOREFS
	ret = spm_vcorefs_pwarp_cmd();
	#endif

	return ret;
}
#endif /* ifdef KERNEL */

#ifdef __KERNEL__
static int __init vcore_ptp_init(void)
{
	of_scan_flat_dt(dt_get_ptp_devinfo, NULL);

	return 0;
}
#endif
/*
 * Module driver
 */

#ifdef __KERNEL__
static int new_eem_val = 1; /* default no change */
static int  __init fn_change_eem_status(char *str)
{
	int new_set_val;

	eem_debug("fn_change_eem_status\n");
	if (get_option(&str, &new_set_val)) {
		new_eem_val = new_set_val;
		eem_debug("new_eem_val = %d\n", new_eem_val);
		return 0;
	}
	return -EINVAL;
}
early_param("eemen", fn_change_eem_status);
#endif

#ifdef __KERNEL__
static int __init eem_init(void)
#else /* for CTP */
int __init eem_init(void)
#endif
{
	int err = 0;

	#ifdef __KERNEL__
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,eem_fsm");
	if (node) {
		/* Setup IO addresses */
		eem_base = of_iomap(node, 0);
		/* eem_debug("[EEM] eem_base = 0x%p\n", eem_base);*/
	}

	/*get eem irq num (215)*/
	eem_irq_number = irq_of_parse_and_map(node, 0);
	eem_debug("[THERM_CTRL] eem_irq_number=%d\n", eem_irq_number);
	if (!eem_irq_number) {
		eem_debug("[EEM] get irqnr failed=0x%x\n", eem_irq_number);
		return 0;
	}
	eem_debug("[EEM] new_eem_val=%d, ctrl_EEM_Enable=%d\n", new_eem_val, ctrl_EEM_Enable);
	#endif

	get_devinfo(&eem_devinfo);

	#if 0
	#ifdef __KERNEL__
		ateVer = GET_BITS_VAL(7:4, get_devinfo_with_index(61));
	#else
		ateVer = GET_BITS_VAL(7:4, eem_read(0x1020698C));
	#endif
	if (ateVer <= 5)
		eem_efuse_calibration(&eem_devinfo);
	#endif

	#if 0 /* preloader params to control ptp, none use now*/
	#ifdef __KERNEL__
	if (new_eem_val == 0) {
		ctrl_EEM_Enable = 0;
		eem_debug("Disable EEM by kernel config\n");
	}
	#endif
	#endif

	/* process_voltage_bin(&eem_devinfo); */ /* LTE voltage bin use I-Chang */
	if (ctrl_EEM_Enable == 0) {
		/* informGpuEEMisReady = 1; */
		eem_error("ctrl_EEM_Enable = 0x%X\n", ctrl_EEM_Enable);
		create_procfs_vcore();
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

    #if ITurbo
	/* Read E-Fuse to control ITurbo mode */
	ctrl_ITurbo = eem_devinfo.BIG_TURBO;
	#endif

	#ifdef __KERNEL__
	/* init timer for log / volt */
	hrtimer_init(&eem_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	eem_log_timer.function = eem_log_timer_func;

	create_procfs();
	#endif

	/*
	 * reg platform device driver
	 */
	err = platform_driver_register(&eem_driver);

	if (err) {
		eem_debug("EEM driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	return 0;
}

static void __exit eem_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	eem_debug("eem de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}

#ifdef __KERNEL__
/* module_init(eem_conf); */ /*no record table*/
arch_initcall(vcore_ptp_init); /* I-Chang */
late_initcall(eem_init);
#endif
#endif /* EN_EEM */

#else /*if EEM_ENABLE_TINYSYS_SSPM */
struct eem_log *eem_logs;
#define EEM_IPI_SEND_DATA_LEN 4 /* size of cmd and args = 4 slot */
/* ipi_send() return code
 * IPI_DONE 0
 * IPI_RETURN 1
 * IPI_BUSY -1
 * IPI_TIMEOUT_AVL -2
 * IPI_TIMEOUT_ACK -3
 * IPI_MODULE_ID_ERROR -4
 * IPI_HW_ERROR -5
 */
static unsigned int eem_to_sspm(unsigned int cmd, struct eem_ipi_data *eem_data)
{
	unsigned int ackData;
	unsigned int len = EEM_IPI_SEND_DATA_LEN;
	unsigned int ret;
	#define IPI_ID IPI_ID_PTPOD /* IPI_ID_VCORE_DVFS */
	FUNC_ENTER(FUNC_LV_MODULE);
	switch (cmd) {
	case IPI_EEM_INIT:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_INIT) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

	case IPI_EEM_PROBE:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_PROBE) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

#ifdef EEM_VCORE_IN_SSPM
	case IPI_EEM_VCORE_GET_VOLT:
			eem_data->cmd = cmd;
			/* defined at sspm_ipi.h, id is difined at elbrus/sspm_ipi_pin.h */
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_VCORE_GET_VOLT) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;
#endif

#if 0
	case IPI_EEM_GPU_DVFS_GET_STATUS:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_GPU_DVFS_GET_STATUS) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;
#endif

	case IPI_EEM_DEBUG_PROC_SHOW:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_DEBUG_PROC_SHOW) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);

			break;

	case IPI_EEM_DEBUG_PROC_WRITE:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_DEBUG_PROC_WRITE) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

#ifdef EEM_LOG_TIMER
	case IPI_EEM_LOGEN_PROC_SHOW:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_LOGEN_PROC_SHOW) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			eem_debug("eem_to_sspm (logen): ret %d\n", ackData);
			break;

	case IPI_EEM_LOGEN_PROC_WRITE:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_LOGEN_PROC_WRITE) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;
#endif

#if 0
	case IPI_EEM_OFFSET_PROC_SHOW:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_OFFSET_PROC_SHOW) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

	case IPI_EEM_OFFSET_PROC_WRITE:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_OFFSET_PROC_WRITE) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;
#endif

	case IPI_EEM_VCORE_INIT:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len,
&ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_VCORE_INIT) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;
	case IPI_EEM_VCORE_UPDATE_PROC_WRITE:
			/* ackData will be 0 if set newVolt successfully*/
			/* ackData will be 1 if newVolt is illegal */
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_VCORE_UPDATE_PROC_WRITE) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

	case IPI_EEM_CUR_VOLT_PROC_SHOW:
			eem_data->cmd = cmd;
			ret = sspm_ipi_send_sync(IPI_ID, IPI_OPT_LOCK_POLLING, eem_data, len, &ackData);
			if (ret != 0)
				eem_error("sspm_ipi_send_sync error(IPI_EEM_CUR_VOLT_PROC_SHOW) ret:%d - %d\n",
							ret, ackData);
			else if (ackData < 0)
				eem_error("cmd(%d) return error(%d)\n", cmd, ackData);
			break;

	default:
			eem_error("cmd(%d) wrong!!\n", cmd);
			break;
	}

	FUNC_EXIT(FUNC_LV_MODULE);
	return ackData;
}

void mt_ptp_lock(unsigned long *flags)
{
#if 0
	/* FUNC_ENTER(FUNC_LV_HELP); */
	/* FIXME: lock with MD32 */
	/* get_md32_semaphore(SEMAPHORE_PTP); */
#ifdef __KERNEL__
	spin_lock_irqsave(&eem_spinlock, *flags);
	eem_pTime_us = eem_get_current_time_us();
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
#endif
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_lock);
#endif

void mt_ptp_unlock(unsigned long *flags)
{
#if 0
	/* FUNC_ENTER(FUNC_LV_HELP); */
#ifdef __KERNEL__
	eem_cTime_us = eem_get_current_time_us();
	EEM_IS_TOO_LONG();
	spin_unlock_irqrestore(&eem_spinlock, *flags);
	/* FIXME: lock with MD32 */
	/* release_md32_semaphore(SEMAPHORE_PTP); */
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
#endif
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_unlock);
#endif
/**
 * ===============================================
 * PROCFS interface for debugging
 * ===============================================
 */

#ifdef CONFIG_PROC_FS
/*
 * show current EEM stauts
 */
static int eem_debug_proc_show(struct seq_file *m, void *v)
{
	struct eem_ipi_data eem_data;
	int ret = 0;
	struct eem_det *det = (struct eem_det *)m->private;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_MODULE);
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = det_to_id(det);
	/* return det EEMEN by det->ops->get_status */
	ret = eem_to_sspm(IPI_EEM_DEBUG_PROC_SHOW, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

	seq_printf(m, "[%s] %s (0x%X)\n",
		   ((char *)(det->name) + 8),
		   det->disabled ? "disabled" : "enable",
		   ret
		   );
	FUNC_EXIT(FUNC_LV_MODULE);
	return 0;

}

/*
 * set EEM status by procfs interface
 * 0 : enable det
 * 1 : disabled eem
 * 2 : disabled mon mode, restore to init2 volt
 */
static ssize_t eem_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{

	int ret = 0;
	int enabled = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));
	struct eem_ipi_data eem_data;
	int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;
		spin_lock_irqsave(&eem_spinlock, flags);
		memset(&eem_data, 0, sizeof(struct eem_ipi_data));
		eem_data.u.data.arg[0] = det_to_id(det);
		eem_data.u.data.arg[1] = enabled;
		ipi_ret = eem_to_sspm(IPI_EEM_DEBUG_PROC_WRITE, &eem_data);
		spin_unlock_irqrestore(&eem_spinlock, flags);
	} else
		ret = -EINVAL;

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;

}
/*
 * show current voltage
 */
#define PMIC_TO_VOLT(val) ((val * 625) + 45000)
static int eem_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;
	u32 i;
	unsigned long flags;
	struct eem_ipi_data eem_data;
	unsigned char num_freq_tbl;
	unsigned char *volt_tbl;
	unsigned char *volt_tbl_pmic;
	unsigned int locklimit = 0, ret;
	unsigned char lock;

	FUNC_ENTER(FUNC_LV_HELP);

	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = det_to_id(det);
	/* return det current volt by det->ops->get_volt */
	ret = eem_to_sspm(IPI_EEM_CUR_VOLT_PROC_SHOW, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

	/* rdata = det->ops->get_volt(det); */
	while (1) {
		lock = eem_logs->det_log[det->ctrl_id].lock;
		locklimit++;
		mdelay(5); /* wait 5 ms */
		eem_debug("det(%d) lock=0x%X\n", det->ctrl_id, lock);
		/* if lock or access less than 5 times, read the lock again until unlock */
		if ((lock & 0x1) && (locklimit < 5))
			continue;

		num_freq_tbl = eem_logs->det_log[det->ctrl_id].num_freq_tbl;
		volt_tbl = kmalloc((sizeof(unsigned char) * num_freq_tbl), GFP_KERNEL);
		volt_tbl_pmic = kmalloc((sizeof(unsigned char) * num_freq_tbl), GFP_KERNEL);

		if (det->ctrl_id != EEM_CTRL_SOC) {
			for (i = 0; i < num_freq_tbl; i++) {
				volt_tbl[i] = eem_logs->det_log[det->ctrl_id].volt_tbl[i];
				volt_tbl_pmic[i] = eem_logs->det_log[det->ctrl_id].volt_tbl_pmic[i];
			}
		}

		lock = eem_logs->det_log[det->ctrl_id].lock;
		eem_debug("det(%d) lock=0x%X\n", det->ctrl_id, lock);
		if ((lock & 0x1) && (locklimit < 5))
			continue; /* if lock, read dram again */
		else
			break; /* if unlock, break out while loop, read next det*/
	}

	if (ret != 0)
		seq_printf(m, "current volt:%d\n", ret);
	else
		seq_printf(m, "EEM[%s] read current voltage fail\n", det->name);

	if (det->ctrl_id != EEM_CTRL_SOC) {
		for (i = 0; i < num_freq_tbl; i++)
			seq_printf(m, "det->volt_tbl[%d] =0x%X, det->volt_tbl_pmic[%d]=0x%X(%d)\n",
			i, volt_tbl[i],
			i, volt_tbl_pmic[i], PMIC_TO_VOLT(volt_tbl_pmic[i]));
	}

	kfree(volt_tbl);
	kfree(volt_tbl_pmic);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current EEM data
 */
static int eem_dump_proc_show(struct seq_file *m, void *v)
{

	struct eem_det *det;
	/* int *val = (int *)&eem_devinfo; */
	int i, k;
	unsigned char lock;
	unsigned int locklimit = 0;
	#if DUMP_DATA_TO_DE
	int j;
	#endif
	/* unsigned int ipi_ret; */

	FUNC_ENTER(FUNC_LV_HELP);
	/*
	*eem_detectors[EEM_DET_BIG].ops->dump_status(&eem_detectors[EEM_DET_BIG]);
	*eem_detectors[EEM_DET_L].ops->dump_status(&eem_detectors[EEM_DET_L]);
	*seq_printf(m, "det->EEMMONEN= 0x%08X,det->EEMINITEN= 0x%08X\n", det->EEMMONEN, det->EEMINITEN);
	*seq_printf(m, "leakage_core\t= %d\n"
	*		"leakage_gpu\t= %d\n"
	*		"leakage_little\t= %d\n"
	*		"leakage_big\t= %d\n",
	*		leakage_core,
	*		leakage_gpu,
	*		leakage_sram2,
	*		leakage_sram1
	*		);
	*/

	for (i = 0; i < NR_HW_RES-2; i++) {
		if (eem_logs->hw_res[i]) {
			seq_printf(m, "M_HW_RES%d\t=0x%08X\n", i, eem_logs->hw_res[i]);
			eem_debug("M_HW_RES%d\t=0x%08X\n", i, eem_logs->hw_res[i]);
		} else {
			eem_debug("M_HW_RES error\n");
		}
	}

	for_each_det(det) {
		/* while (1) {*/
		lock = eem_logs->det_log[det->ctrl_id].lock;
		locklimit++;
		eem_debug("det(%d) lock=%d\n", det->ctrl_id, lock);

		/* if lock, go to while loop, read the lock again until unlock */
		if (lock && (locklimit < 5))
			continue;
		det->num_freq_tbl = eem_logs->det_log[det->ctrl_id].num_freq_tbl;
		if (det->ctrl_id == EEM_CTRL_SOC)
			break; /* break out while loop, read next det */
		for (i = EEM_PHASE_INIT01; i < NR_EEM_PHASE; i++) {
			/* seq_printf(m, "Bank_number = %d\n", det->ctrl_id); */
			seq_printf(m, "Bank_number = %d\n", det->ctrl_id);
			if (i < EEM_PHASE_MON)
				seq_printf(m, "mode = init%d\n", i+1);
			else
				seq_puts(m, "mode = mon\n");

			if (eem_log_en) {
				seq_printf(m, "0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
					eem_logs->det_log[det->ctrl_id].dcvalues[i],
					eem_logs->det_log[det->ctrl_id].freqpct30[i],
					eem_logs->det_log[det->ctrl_id].eem_26c[i],
					eem_logs->det_log[det->ctrl_id].vop30[i],
					eem_logs->det_log[det->ctrl_id].eem_eemEn[i]
					);

				eem_debug("0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
					eem_logs->det_log[det->ctrl_id].dcvalues[i],
					eem_logs->det_log[det->ctrl_id].freqpct30[i],
					eem_logs->det_log[det->ctrl_id].eem_26c[i],
					eem_logs->det_log[det->ctrl_id].vop30[i],
					eem_logs->det_log[det->ctrl_id].eem_eemEn[i]
					);
				if (eem_logs->det_log[det->ctrl_id].eem_eemEn[i] == 0x5) {
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d) - (",
					/* det->ctrl_id, det->ops->get_temp(det)); */
					det->ctrl_id, eem_logs->det_log[det->ctrl_id].temp);

					eem_debug("det num_freq_tbl=%d\n",
						eem_logs->det_log[det->ctrl_id].num_freq_tbl);

					for (k = 0; k < det->num_freq_tbl - 1; k++) {
						seq_printf(m, "%d, ", /* %d */
						/* det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k])); */
						eem_logs->det_log[det->ctrl_id].volt_tbl_init2[k]);
					}
					seq_printf(m, "%d) - (",
						eem_logs->det_log[det->ctrl_id].volt_tbl_init2[k]);

					for (k = 0; k < det->num_freq_tbl - 1; k++) {
						seq_printf(m, "%d, ",
							eem_logs->det_log[det->ctrl_id].freq_tbl[k]);
					}
					seq_printf(m, "%d)\n", eem_logs->det_log[det->ctrl_id].freq_tbl[k]);
				}
				#if 0
				if (eem_logs->det_log[det->ctrl_id].eem_eemEn[i] == 0x2) {
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d)\n",
					/* det->ctrl_id, det->ops->get_temp(det)); */
					det->ctrl_id, eem_logs->det_log[det->ctrl_id].temp);
					seq_puts(m, "volt_tbl = ");
					for (k = 0; k < det->num_freq_tbl; k++) {
						seq_printf(m, "%d, ", /* %d */
						/* det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k])); */
						eem_logs->det_log[det->ctrl_id].volt_tbl_mon[k]);
					}
					seq_puts(m, "\n");
					seq_puts(m, "volt_tbl_pmic = ");
					for (k = 0; k < det->num_freq_tbl; k++) {
						seq_printf(m, "%d, ", /* %d */
						/* det->ops->pmic_2_volt(det, det->volt_tbl_pmic[k])); */
						eem_logs->det_log[det->ctrl_id].volt_tbl_pmic[k]);
					}
					seq_puts(m, "\n");
				}
				#endif
			} /* if (eem_log_en)*/
			#if DUMP_DATA_TO_DE
			for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
				seq_printf(m, "0x%08lx = 0x%08x\n",
					(unsigned long)EEM_BASEADDR + reg_dump_addr_off[j],
					/* det->reg_dump_data[j][i] */
					eem_logs->det_log[det->ctrl_id].reg_dump_data[j][i]
					);
			#endif
		} /* for init1 to mon*/
		lock = eem_logs->det_log[det->ctrl_id].lock;
		eem_debug("det(%d) lock=%d\n", det->ctrl_id, lock);
		if (lock && (locklimit < 5))
			continue; /* if lock, read dram again */
		else
			break; /* if unlock, break out while loop, read next det*/
	/*};*/
	} /* for_each_det */
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
/*
 * show current EEM status
 */
#if 0
static int eem_status_proc_show(struct seq_file *m, void *v)
{

	int i;
	struct eem_det *det = (struct eem_det *)m->private;
	struct eem_ipi_data eem_data;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "bank = %d, (%d) - (",
		   det->ctrl_id, det->ops->get_temp(det));
	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));
	seq_printf(m, "%d) - (", det->ops->pmic_2_volt(det, det->volt_tbl_pmic[i]));

	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->freq_tbl[i]);
	seq_printf(m, "%d)\n", det->freq_tbl[i]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
#endif

/*
 * set EEM log enable by procfs interface
 */
#ifdef EEM_LOG_TIMER
static int eem_log_en_proc_show(struct seq_file *m, void *v)
{
	struct eem_ipi_data eem_data;
	unsigned int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	ipi_ret = eem_to_sspm(IPI_EEM_LOGEN_PROC_SHOW, &eem_data);
	seq_printf(m, "%d\n", ipi_ret);
	spin_unlock_irqrestore(&eem_spinlock, flags);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct eem_ipi_data eem_data;
	unsigned int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (kstrtoint(buf, 10, &eem_log_en)) {
		eem_error("bad argument! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = eem_log_en;
	ipi_ret = eem_to_sspm(IPI_EEM_LOGEN_PROC_WRITE, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);
out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif /*ifdef EEM_LOG_TIMER */

/*
 * show EEM offset
 */
#if 0
static int eem_offset_proc_show(struct seq_file *m, void *v)
{

	struct eem_det *det = (struct eem_det *)m->private;
	struct eem_ipi_data eem_data;
	unsigned int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = det_to_id(det);
	ipi_ret = eem_to_sspm(IPI_EEM_OFFSET_PROC_SHOW, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);
	seq_printf(m, "%d\n", ipi_ret);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM offset by procfs
 */
static ssize_t eem_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));
	unsigned int ipi_ret = 0;
	struct eem_ipi_data eem_data;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &offset)) {
		ret = 0;
		spin_lock_irqsave(&eem_spinlock, flags);
		memset(&eem_data, 0, sizeof(struct eem_ipi_data));
		eem_data.u.data.arg[0] = det_to_id(det);
		eem_data.u.data.arg[1] = offset;
		ipi_ret = eem_to_sspm(IPI_EEM_OFFSET_PROC_WRITE, &eem_data);
		spin_unlock_irqrestore(&eem_spinlock, flags);
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif

static int eem_vcore_volt_proc_show(struct seq_file *m, void *v)
{

	unsigned int i = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < VCORE_NR_FREQ; i++)
		seq_printf(m, "%d ", VCORE_PMIC_2_VOLT(get_vcore_ptp_volt(i)) * 10);
	seq_puts(m, "\n");

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

unsigned int check_vcore_volt_boundary(unsigned int index, unsigned int newVolt)
{
	unsigned int volt_pmic = VOLT_2_VCORE_PMIC(newVolt);

	if ((index == 0) && (volt_pmic >= eem_vcore[index+1])) {
		return 0;
	} else if ((index == VCORE_NR_FREQ-1) && (volt_pmic <= eem_vcore[index-1])) {
		return 0;
	} else if ((index > 0) && (index < VCORE_NR_FREQ-1) &&
				(volt_pmic <= eem_vcore[index-1]) &&
				(volt_pmic >= eem_vcore[index+1])) {
		return 0;
	} else {
		return -EINVAL;
	}
}

static ssize_t eem_vcore_volt_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int index = 0;
	unsigned int newVolt = 0;
	struct eem_ipi_data eem_data;
	int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (sscanf(buf, "%u %u", &index, &newVolt) == 2) {
		ret = 0;
		/* transfer uv to 10uv */
		if (newVolt > 10)
			newVolt = newVolt / 10;
		ret = check_vcore_volt_boundary(index, newVolt);
		if (ret == -EINVAL) {
			if (index == 0)
				eem_debug("volt should be set larger than index %d\n", index+1);
			else if (index == 4) /* VCORE_NR_FREQ = 5 */
				eem_debug("volt should be set smaller than index %d\n", index-1);
			else
				eem_debug("volt should be set between index %d - %d\n", index-1, index+1);
		} else {
			eem_error("set eem_vcore[%d]=%d\n", index, newVolt);
			spin_lock_irqsave(&eem_spinlock, flags);
			memset(&eem_data, 0, sizeof(struct eem_ipi_data));
			eem_data.u.data.arg[0] = index;
			eem_data.u.data.arg[1] = newVolt;
			ipi_ret = eem_to_sspm(IPI_EEM_VCORE_UPDATE_PROC_WRITE, &eem_data);
			spin_unlock_irqrestore(&eem_spinlock, flags);
			/* retrieve entry */
			eem_vcore[index] = eem_logs->eem_vcore_pmic[index];
		}
	} else {
		ret = -EINVAL;
		eem_debug("bad argument!! argument should be \"0\"1\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#if 0
static int eem_vcore_enable_proc_show(struct seq_file *m, void *v)
{
	struct eem_ipi_data eem_data;
	unsigned int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	/* eem_data.u.data.arg[0] = det_to_id(det); */
	/* return ctrl_VCORE_Volt_Enable from sspm */
	ipi_ret = eem_to_sspm(IPI_EEM_VCORE_ENABLE_PROC_SHOW, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);
	seq_printf(m, "%d\n", ipi_ret);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_vcore_enable_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{

	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int enable = 0;
	/* unsigned int i = 0; */
	struct eem_ipi_data eem_data;
	unsigned int ipi_ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = enable;
	ipi_ret = eem_to_sspm(IPI_EEM_VCORE_ENABLE_PROC_WRITE, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}
#endif

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(eem_debug);
PROC_FOPS_RW(eem_vcore_volt);
/* PROC_FOPS_RO(eem_status); */
PROC_FOPS_RO(eem_cur_volt);
/* PROC_FOPS_RW(eem_offset); */
PROC_FOPS_RO(eem_dump);
/* PROC_FOPS_RW(eem_log_en);*/
/* PROC_FOPS_RW(eem_vcore_enable); */

static int create_procfs(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct eem_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(eem_debug),
		/* PROC_ENTRY(eem_status),*/
		PROC_ENTRY(eem_cur_volt),
		/* PROC_ENTRY(eem_offset),*/
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_vcore_volt),
		PROC_ENTRY(eem_dump),
		#ifdef EEM_LOG_TIMER
		PROC_ENTRY(eem_log_en),
		#endif
	};

	FUNC_ENTER(FUNC_LV_HELP);
	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_error("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, eem_dir, eem_entries[i].fops)) {
			eem_error("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		if (det->ctrl_id == EEM_CTRL_SOC)
			continue;
		det_dir = proc_mkdir(det->name, eem_dir);

		if (!det_dir) {
			eem_debug("[%s]: mkdir /proc/eem/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data(det_entries[i].name,
					S_IRUGO | S_IWUSR | S_IWGRP,
					det_dir,
					det_entries[i].fops, det)) {
				eem_debug("[%s]: create /proc/eem/%s/%s failed\n", __func__,
						det->name, det_entries[i].name);
						FUNC_EXIT(FUNC_LV_HELP);
						return -3;
			}
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
static int create_procfs_vcore(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_vcore_volt),
	};

	FUNC_ENTER(FUNC_LV_HELP);
	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_error("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP,
			eem_dir, eem_entries[i].fops)) {
			eem_error("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
#endif /* CONFIG_PROC_FS */

/* done at arch_init */
static int __init vcore_ptp_init(void)
{
	int i = 0;
	unsigned int soc_efuse;

	eem_log_phy_addr = sspm_reserve_mem_get_phys(PTPOD_MEM_ID);
	eem_log_virt_addr = sspm_reserve_mem_get_virt(PTPOD_MEM_ID);
	eem_log_size = sspm_reserve_mem_get_size(PTPOD_MEM_ID);
	eem_logs = (struct eem_log *)eem_log_virt_addr;
	/* soc_efuse = get_devinfo_with_index(54);*/
	soc_efuse = 0; /* no efuse value, so set soc_efuse to 0 */
	eem_error("[VCORE] - Kernel Got efuse 0x%0X\n", soc_efuse);
	eem_vcore_index[0] = (soc_efuse >> 4) & 0x03;
	eem_vcore_index[1] = (soc_efuse >> 2) & 0x03;
	eem_vcore_index[2] = soc_efuse & 0x03;
	eem_vcore_index[3] = 0;
	/* eem_vcore_index[4] = 0; */

	for (i = 0; i < VCORE_NR_FREQ; i++)
		eem_vcore[i] = VOLT_2_VCORE_PMIC(vcore_opp[i][eem_vcore_index[i]]);

	/* bottom up compare each volt to ensure each opp is in descending order */
	for (i = VCORE_NR_FREQ - 2; i >= 0; i--)
		eem_vcore[i] = (eem_vcore[i] < eem_vcore[i+1]) ? eem_vcore[i+1] : eem_vcore[i];

	for (i = 0; i < VCORE_NR_FREQ; i++)
		eem_logs->eem_vcore_pmic[i] = eem_vcore[i];

	eem_error("Got vcore volt(pmic): %d %d %d %d\n",
				eem_logs->eem_vcore_pmic[0], eem_logs->eem_vcore_pmic[1],
				eem_logs->eem_vcore_pmic[2], eem_logs->eem_vcore_pmic[3]);

	return 0;
}

/* return ack from sspm spm_vcorefs_pwarp_cmd */
unsigned int mt_eem_vcorefs_set_volt(void)
{
	unsigned long flags;
	struct eem_ipi_data eem_data;
	int ret = 0;

	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	ret = eem_to_sspm(IPI_EEM_VCORE_INIT, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);
	return ret;
}

/* still need to decide vcore0-5 in vcore_ptp_init */
unsigned int get_vcore_ptp_volt(unsigned int seg)
{
#if 0
	struct eem_ipi_data eem_data;
	int ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_MODULE);
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = seg;
	ret = eem_to_sspm(IPI_EEM_VCORE_GET_VOLT, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

	FUNC_EXIT(FUNC_LV_MODULE);
	return ret;
#endif
	eem_debug("get_vcore_ptp_volt: %d\n", eem_vcore[seg]);
	return eem_vcore[seg];
}

#if 0 /* gpu dvfs no need this anymore */
unsigned int get_eem_status_for_gpu(void)
{
	struct eem_ipi_data eem_data;
	int ret = 0;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_MODULE);
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = 0;
	ret = eem_to_sspm(IPI_EEM_GPU_DVFS_GET_STATUS, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

	FUNC_EXIT(FUNC_LV_MODULE);
	return ret;
}
#endif

static void eem_init_postprocess(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef __KERNEL__
	#ifndef EARLY_PORTING

		#if !defined(CONFIG_MTK_CLKMGR)
			clk_disable_unprepare(clk_mfg_main); /* Disable GPU MTCMOSE */
			clk_disable_unprepare(clk_mfg_core0); /* Disable GPU clock */
			clk_disable_unprepare(clk_mfg_core1); /* Disable GPU clock */
			clk_disable_unprepare(clk_mfg_core2); /* Disable GPU clock */
			clk_disable_unprepare(clk_mfg_core3); /* Disable GPU clock */
			clk_disable_unprepare(clk_mfg); /* Disable GPU clock */
			clk_disable_unprepare(clk_mfg_async); /* Disable GPU MTCMOSE */
			/*clk_disable_unprepare(clk_thermal);*/  /* bypass this due to crash*/
		#endif

		#ifndef CONFIG_BIG_OFF
		if (cpu_online(8))
			cpu_down(8);
		#endif
			/* enable frequency hopping (main PLL) */
			/* mt_fh_popod_restore(); */
	#endif /* EARLY_PORTING */
#endif /* ifdef __KERNEL__ */
	/* must be set after gpu dvfs is enabled */
	/* informGpuEEMisReady = 1; */
	ptp_data[0] = 0;

	FUNC_EXIT(FUNC_LV_MODULE);

}
/* #define EFUSE_API */
static void get_devinfo_to_log(void)
{

	FUNC_ENTER(FUNC_LV_HELP);
#if 0
	#if EFUSE_API /* if defined(__KERNEL__)*/
		eem_logs->hw_res[0] = get_devinfo_with_index(50);
		eem_logs->hw_res[1] = get_devinfo_with_index(51);
		eem_logs->hw_res[2] = get_devinfo_with_index(52);
		eem_logs->hw_res[3] = get_devinfo_with_index(53);
		eem_logs->hw_res[4] = get_devinfo_with_index(56);
		eem_logs->hw_res[5] = get_devinfo_with_index(57);
		eem_logs->hw_res[6] = get_devinfo_with_index(58);
		eem_logs->hw_res[7] = get_devinfo_with_index(59);
		eem_logs->hw_res[8] = get_devinfo_with_index(60);
		eem_logs->hw_res[9] = get_devinfo_with_index(61);
		eem_logs->hw_res[10] = get_devinfo_with_index(65);
		/* soc efuse */
		eem_logs->hw_res[11] = get_devinfo_with_index(54);
		/* binLevel */
		eem_logs->hw_res[12] = get_devinfo_with_index(22);

		#if defined(CONFIG_EEM_AEE_RR_REC) /* irene */
			aee_rr_rec_ptp_60((unsigned int)eem_logs->hw_res[0]);
			aee_rr_rec_ptp_64((unsigned int)eem_logs->hw_res[1]);
			aee_rr_rec_ptp_68((unsigned int)eem_logs->hw_res[2]);
			aee_rr_rec_ptp_6C((unsigned int)eem_logs->hw_res[3]);
			aee_rr_rec_ptp_78((unsigned int)eem_logs->hw_res[4]);
			aee_rr_rec_ptp_7C((unsigned int)eem_logs->hw_res[5]);
			aee_rr_rec_ptp_80((unsigned int)eem_logs->hw_res[6]);
			aee_rr_rec_ptp_84((unsigned int)eem_logs->hw_res[7]);
			aee_rr_rec_ptp_88((unsigned int)eem_logs->hw_res[8]);
			aee_rr_rec_ptp_8C((unsigned int)eem_logs->hw_res[9]);
			aee_rr_rec_ptp_9C((unsigned int)eem_logs->hw_res[10]);
		#endif
	#else
		eem_logs->hw_res[0] = eem_read(0x11D10580); /* M_HW_RES0 */
		eem_logs->hw_res[1] = eem_read(0x11D10584); /* M_HW_RES1 */
		eem_logs->hw_res[2] = eem_read(0x11D10588); /* M_HW_RES2 */
		eem_logs->hw_res[3] = eem_read(0x11D1058C); /* M_HW_RES3 */
		eem_logs->hw_res[4] = eem_read(0x11D10590); /* M_HW_RES6 */
		eem_logs->hw_res[5] = eem_read(0x11D10594); /* M_HW_RES7 */
		eem_logs->hw_res[6] = eem_read(0x11D10598); /* M_HW_RES8 */
		eem_logs->hw_res[7] = eem_read(0x11D1059C); /* M_HW_RES9 */
		eem_logs->hw_res[8] = eem_read(0x11D105A0); /* M_HW_RES10 */
		eem_logs->hw_res[9] = eem_read(0x11D105A4); /* M_HW_RES11 */
		eem_logs->hw_res[10] = eem_read(0x11D105A8); /* M_HW_RES15 */
		/* soc efuse */
		eem_logs->hw_res[11] = 0x0;
		/* binLevel */
		eem_logs->hw_res[12] = eem_read(0x1020671C);
	#endif/* if EFUSE_API */
#else/* (using fake efuse on Elbrus) */
	/* test pattern (irene)*/
	eem_logs->hw_res[0] = 0xFFFFFFFF; /* 0x17F75060 */
	eem_logs->hw_res[1] = 0x10BD3C1B; /* 0x00540003 */
	eem_logs->hw_res[2] = 0x005500C0; /* 0x187E3D12 */
	eem_logs->hw_res[3] = 0x10BD3C1B; /* 0x00560003 */
	eem_logs->hw_res[4] = 0x005500C0; /* 0x187E6103 */
	eem_logs->hw_res[5] = 0x10BD3C1B; /* 0x00450013 */
	eem_logs->hw_res[6] = 0x005500C0; /* 0x187E5E06 */
	eem_logs->hw_res[7] = 0x10BD3C1B; /* 0x00450003 */
	eem_logs->hw_res[8] = 0x005500C0; /* 0x187E6004;*/
	eem_logs->hw_res[9] = 0x10BD3C1B; /* 0x00450010 */
	eem_logs->hw_res[10] = 0x005500C0; /* 0x0070004E */
	/* for soc efuse */
	eem_logs->hw_res[11] = 0x0;

	/* for binlevel */
	eem_logs->hw_res[12] = 0x0;
#endif
/*
*	eem_debug("M_HW_RES0 = 0x%08X\n", val[0]);
*	eem_debug("M_HW_RES1 = 0x%08X\n", val[1]);
*	eem_debug("M_HW_RES2 = 0x%08X\n", val[2]);
*	eem_debug("M_HW_RES3 = 0x%08X\n", val[3]);
*	eem_debug("M_HW_RES6 = 0x%08X\n", val[4]);
*	eem_debug("M_HW_RES7 = 0x%08X\n", val[5]);
*	eem_debug("M_HW_RES8 = 0x%08X\n", val[6]);
*	eem_debug("M_HW_RES9 = 0x%08X\n", val[7]);
*	eem_debug("M_HW_RES10 = 0x%08X\n", val[8]);
*	eem_debug("M_HW_RES11 = 0x%08X\n", val[9]);
*	eem_debug("M_HW_RES15 = 0x%08X\n", val[10]);
*/
#if 0 /* if efuse ready, turn on this code */
	for (i = 0; i < NR_HW_RES-2; i++) {
		if (eem_logs->hw_res[i] == 0) {
			/* ctrl_EEM_Enable = 0; */
			/* infoIdvfs = 0x55; */
			checkEfuse = 0;
			eem_error("No EEM EFUSE available, will turn off EEM (val[%d] !!\n", i);
			break;
		}
	}
#endif
	/*
	*if (i < 12) {
	*	val[0] = 0x1410595E;
	*	val[1] = 0x004C0003;
	*	val[2] = 0x14C0570C;
	*	val[3] = 0x004B0003;
	*	val[4] = 0x14C06B01;
	*	val[5] = 0x003F0003;
	*	val[6] = 0x14C06B05;
	*	val[7] = 0x003F0303;
	*	val[8] = 0x14C06804;
	*	val[9] = 0x003F0000;
	*	val[10] = 0x00000047;
	*	val[11] = 0x0D2B365A;
	*}
	*/
	FUNC_EXIT(FUNC_LV_HELP);
}

static int eem_probe(struct platform_device *pdev)
{
	int ret;
	struct eem_ipi_data eem_data;
	unsigned long flags;
	/* unsigned int code = mt_get_chip_hw_code(); */

	FUNC_ENTER(FUNC_LV_MODULE);

	#ifdef __KERNEL__
		#if !defined(CONFIG_MTK_CLKMGR) && !defined(EARLY_PORTING)
			/* enable thermal CG */
			clk_thermal = devm_clk_get(&pdev->dev, "therm-eem");
			if (IS_ERR(clk_thermal)) {
				eem_error("cannot get thermal clock\n");
				return PTR_ERR(clk_thermal);
			}

			/* get GPU mtcomose */
			clk_mfg_async = devm_clk_get(&pdev->dev, "mtcmos-mfg-async");
			if (IS_ERR(clk_mfg_async)) {
				eem_error("cannot get mtcmos-mfg-async\n");
				return PTR_ERR(clk_mfg_async);
			}

			clk_mfg = devm_clk_get(&pdev->dev, "mtcmos-mfg");
			if (IS_ERR(clk_mfg)) {
				eem_error("cannot get mtcmos-mfg\n");
				return PTR_ERR(clk_mfg);
			}

			clk_mfg_core3 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core3");
			if (IS_ERR(clk_mfg_core3)) {
				eem_error("cannot get mtcmos-mfg_core3\n");
				return PTR_ERR(clk_mfg_core3);
			}

			clk_mfg_core2 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core2");
			if (IS_ERR(clk_mfg_core2)) {
				eem_error("cannot get mtcmos-mfg-core2\n");
				return PTR_ERR(clk_mfg_core2);
			}

			clk_mfg_core1 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core1");
			if (IS_ERR(clk_mfg_core1)) {
				eem_error("cannot get mtcmos-mfg-core1\n");
				return PTR_ERR(clk_mfg_core1);
			}

			clk_mfg_core0 = devm_clk_get(&pdev->dev, "mtcmos-mfg-core0");
			if (IS_ERR(clk_mfg_core0)) {
				eem_error("cannot get mtcmos-mfg-core0\n");
				return PTR_ERR(clk_mfg_core0);
			}

			/* get GPU clock */
			clk_mfg_main = devm_clk_get(&pdev->dev, "mfg-main");
			if (IS_ERR(clk_mfg_main)) {
				eem_error("cannot get mfg-main\n");
				return PTR_ERR(clk_mfg_main);
			}
			/*
			*eem_debug("thmal=%p, gpu_clk=%p, gpu_mtcmos=%p",
			*	clk_thermal,
			*	clk_mfg_main,
			*	clk_mfg_async);
			*/
		#endif
	#endif

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			/* disable frequency hopping (main PLL) */
			/* mt_fh_popod_save(); */

			/* call hotplug api to enable L bulk */

			#if !defined(CONFIG_MTK_CLKMGR)
				eem_debug("not defined CONFIG_MTK_CLKMGR, enable clk_thermal...!\n");
				ret = clk_prepare_enable(clk_thermal); /* Thermal clock enable */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling THERMAL\n");

				ret = clk_prepare_enable(clk_mfg_async); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_async\n");

				ret = clk_prepare_enable(clk_mfg); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg\n");

				ret = clk_prepare_enable(clk_mfg_core3); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core3\n");

				ret = clk_prepare_enable(clk_mfg_core2); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core2\n");

				ret = clk_prepare_enable(clk_mfg_core1); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core1\n");

				ret = clk_prepare_enable(clk_mfg_core0); /* gpu mtcmos enable*/
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_core0\n");

				ret = clk_prepare_enable(clk_mfg_main); /* GPU CLOCK */
				if (ret)
					eem_error("clk_prepare_enable failed when enabling clk_mfg_main\n");

			#endif /* if !defined CONFIG_MTK_CLKMGR */
		#endif/* ifndef early porting*/
	#endif/*ifdef __KERNEL__*/

	/* for slow idle */
	ptp_data[0] = 0xffffffff;
	spin_lock_irqsave(&eem_spinlock, flags);
	memset(&eem_data, 0, sizeof(struct eem_ipi_data));
	eem_data.u.data.arg[0] = 0;
	ret = eem_to_sspm(IPI_EEM_PROBE, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);
	eem_init_postprocess();

	eem_debug("eem_probe ok\n");
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
static int eem_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	*kthread_stop(eem_volt_thread);
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
/* tell sspm to call eem_init02 */
static int eem_resume(struct platform_device *pdev)
{
	/*
	* eem_volt_thread = kthread_run(eem_volt_thread_handler, 0, "eem volt");
	*if (IS_ERR(eem_volt_thread))
	*{
	*  eem_debug("[%s]: failed to create eem volt thread\n", __func__);
	*}
	*/

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id mt_eem_of_match[] = {
	{ .compatible = "mediatek,eem_fsm", },
	{},
};
#endif

static struct platform_driver eem_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = eem_probe,
	.suspend    = eem_suspend,
	.resume     = eem_resume,
	.driver     = {
		.name   = "mt-eem",
#ifdef CONFIG_OF
		.of_match_table = mt_eem_of_match,
#endif
	},
};
static int __init eem_init(void)
{
	int err = 0;
	struct eem_ipi_data eem_data;
	int ret = 0;
	struct device_node *node = NULL;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_MODULE);
	/* for testing */

	node = of_find_compatible_node(NULL, NULL, "mediatek,eem_fsm");

	if (node) {
		/* Setup IO addresses */
		eem_base = of_iomap(node, 0);
		eem_debug("[EEM] eem_base = 0x%p\n", eem_base);
	}
	eem_irq_number = irq_of_parse_and_map(node, 0);
	eem_debug("[THERM_CTRL] eem_irq_number=%d\n", eem_irq_number);
	if (!eem_irq_number) {
		eem_debug("[EEM] get irqnr failed=0x%x\n", eem_irq_number);
		return 0;
	}

	memset(&eem_data, 0, sizeof(struct eem_ipi_data));

	eem_data.u.data.arg[0] = eem_log_phy_addr;
	eem_data.u.data.arg[1] = eem_log_size;
	/* read efuse data to dram log */
	get_devinfo_to_log();

	/* ret 1: eem disabled at sspm
	 * ret 0: eem enabled at sspm
	 */
	spin_lock_irqsave(&eem_spinlock, flags);
	ret = eem_to_sspm(IPI_EEM_INIT, &eem_data);
	spin_unlock_irqrestore(&eem_spinlock, flags);

	if (ret > 0) {
		eem_error("EEM disabld\n");
		/* informGpuEEMisReady = 1;*/
		create_procfs_vcore();
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

#ifdef __KERNEL__
	create_procfs();
#endif
	/* reg platform device driver */
	err = platform_driver_register(&eem_driver);

	if (err) {
		eem_debug("EEM driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;

}

static void __exit eem_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	eem_debug("eem de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}

#ifdef __KERNEL__
arch_initcall(vcore_ptp_init); /* I-Chang */
late_initcall(eem_init);
#endif
#endif/* EEM_ENABLE_TINYSYS_SSPM */

MODULE_DESCRIPTION("MediaTek EEM Driver v0.3");
MODULE_LICENSE("GPL");
#ifdef EARLY_PORTING
	#undef EARLY_PORTING
#endif

#undef __MT_EEM_C__

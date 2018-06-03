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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <linux/uaccess.h>

#include "mtk_clkmgr.h"

#include "mtk_gpufreq.h"
/*
* #include "mtk_static_power.h"
*/
#include "mt-plat/upmu_common.h"
#include "mt-plat/sync_write.h"
#include "mt-plat/mtk_pmic_wrap.h"

#include "mach/mtk_fhreg.h"
#include "mach/mtk_freqhopping.h"

static void __iomem *g_apmixed_base;

/* Change name of REG_FH_PLL6_CON0 to REG_GPUPLL_CON0 in MT6763 */
#define GPUPLL_CON0 (REG_GPUPLL_CON0)
#define GPUPLL_CON1 (REG_GPUPLL_CON1)

/* Use VOLT_SET_READY for regulator operation debug*/
#define VOLT_SET_READY

/* TODO: check this! */
/* #include "mach/mt_static_power.h" */
#include "mach/mtk_thermal.h"
#include "mach/upmu_sw.h"
#include "mach/upmu_hw.h"
#include "mach/mtk_pbm.h"

#include <linux/regulator/consumer.h>
#include <mtk_vcorefs_manager.h>
#include "mtk_devinfo.h"

/* #define BRING_UP */

#define DRIVER_NOT_READY	-1

#ifndef MTK_UNREFERENCED_PARAMETER
#define MTK_UNREFERENCED_PARAMETER(param) ((void)(param))
#endif

/**************************************************
 * ENABLE SSPM mode
 **************************************************/

#ifndef MTK_SSPM
/* #define STATIC_PWR_READY2USE */
#else
#include "sspm_ipi.h"
#include "mtk_gpufreq_ipi.h"
#endif

/*
 * CONFIG
 */
/**************************************************
 * Define low battery voltage support
 ***************************************************/
#define MT_GPUFREQ_LOW_BATT_VOLT_PROTECT

/**************************************************
 * Define low battery volume support
 ***************************************************/
#define MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT

/**************************************************
 * Define oc support
 ***************************************************/
#define MT_GPUFREQ_OC_PROTECT

/**************************************************
 * GPU DVFS input boost feature
 ***************************************************/
#define MT_GPUFREQ_INPUT_BOOST

/***************************
 * Define for dynamic power table update
 ****************************/
#define MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE

/***************************
 * Define for random test
 ****************************/
/* #define MT_GPU_DVFS_RANDOM_TEST */

/***************************
 * Define for SRAM debugging
 ****************************/
#ifdef CONFIG_MTK_RAM_CONSOLE
#define MT_GPUFREQ_AEE_RR_REC
#endif

/*
 * Define how to set up VPROC by
 * PMIC_WRAP
 * PMIC
 * external IC
 */
/* #define VPROC_SET_BY_PMIC_WRAP */
#define VPROC_SET_BY_PMIC
/* #define VPROC_SET_BY_EXTIC */

#ifdef VPROC_SET_BY_EXTIC
#include "fan53555.h"
#endif
/**************************************************
 * Define register write function
 ***************************************************/
#define mt_gpufreq_reg_write(val, addr)		mt_reg_sync_writel((val), ((void *)addr))

/***************************
 * Operate Point Definition
 ****************************/
#define GPUOP(khz, volt, vsram, idx)	   \
{						   \
	.gpufreq_khz = khz,	 \
	.gpufreq_volt = volt,	\
	.gpufreq_vsram = vsram,	  \
	.gpufreq_idx = idx,	  \
}

/*************
 * For MT6763+
 *************/
#define VPROC 1
#define VSRAM 2

/**************************
 * GPU DVFS OPP table setting
 ***************************/

/* MT6763+ */
#define GPU_DVFS_FREQ0	 (950000)	/* KHz */
#define GPU_DVFS_FREQ1	 (927000)	/* KHz */
#define GPU_DVFS_FREQ2	 (905000)	/* KHz */
#define GPU_DVFS_FREQ3	 (871000)	/* KHz */
#define GPU_DVFS_FREQ4	 (848000)	/* KHz */
#define GPU_DVFS_FREQ5	 (826000)	/* KHz */
#define GPU_DVFS_FREQ6	 (792000)	/* KHz */
#define GPU_DVFS_FREQ7	 (770000)	/* KHz */
#define GPU_DVFS_FREQ8	 (725000)	/* KHz */
#define GPU_DVFS_FREQ9	 (680000)	/* KHz */
#define GPU_DVFS_FREQ10	  (650000)	 /* KHz */
#define GPU_DVFS_FREQ11	  (605000)	 /* KHz */
#define GPU_DVFS_FREQ12	  (560000)	 /* KHz */
#define GPU_DVFS_FREQ13	  (512000)	 /* KHz */
#define GPU_DVFS_FREQ14	  (442000)	 /* KHz */
#define GPU_DVFS_FREQ15	  (390000)	 /* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL	(GPU_DVFS_FREQ15)

#define GPU_DVFS_VOLT0	 (90000)	/* mV x 100 */
#define GPU_DVFS_VOLT1	 (88750)	/* mV x 100 */
#define GPU_DVFS_VOLT2	 (87500)	/* mV x 100 */
#define GPU_DVFS_VOLT3	 (85625)	/* mV x 100 */
#define GPU_DVFS_VOLT4	 (84375)	/* mV x 100 */
#define GPU_DVFS_VOLT5	 (83125)	/* mV x 100 */
#define GPU_DVFS_VOLT6	 (81250)	/* mV x 100 */
#define GPU_DVFS_VOLT7	 (80000)	/* mV x 100 */
#define GPU_DVFS_VOLT8	 (78125)	/* mV x 100 */
#define GPU_DVFS_VOLT9	 (76250)	/* mV x 100 */
#define GPU_DVFS_VOLT10	  (75000)	/* mV x 100 */
#define GPU_DVFS_VOLT11	  (73125)	/* mV x 100 */
#define GPU_DVFS_VOLT12	  (71250)	/* mV x 100 */
#define GPU_DVFS_VOLT13	  (69375)	/* mV x 100 */
#define GPU_DVFS_VOLT14	  (66875)	/* mV x 100 */
#define GPU_DVFS_VOLT15	  (65000)	/* mV x 100 */

#define GPU_DVFS_VSRAM0	  (100000)	/* mV x 100 */
#define GPU_DVFS_VSRAM1	  (98750)	/* mV x 100 */
#define GPU_DVFS_VSRAM2	  (97500)	/* mV x 100 */
#define GPU_DVFS_VSRAM3	  (95625)	/* mV x 100 */
#define GPU_DVFS_VSRAM4	  (94375)	/* mV x 100 */
#define GPU_DVFS_VSRAM5	  (93125)	/* mV x 100 */
#define GPU_DVFS_VSRAM6	  (91250)	/* mV x 100 */
#define GPU_DVFS_VSRAM7	  (90000)	/* mV x 100 */
#define GPU_DVFS_VSRAM8	  (88125)	/* mV x 100 */
#define GPU_DVFS_VSRAM9	  (86250)	/* mV x 100 */
#define GPU_DVFS_VSRAM10   (85000)	/* mV x 100 */
#define GPU_DVFS_VSRAM11   (83125)	/* mV x 100 */
#define GPU_DVFS_VSRAM12   (81250)	/* mV x 100 */
#define GPU_DVFS_VSRAM13   (80000)	/* mV x 100 */
#define GPU_DVFS_VSRAM14   (80000)	/* mV x 100 */
#define GPU_DVFS_VSRAM15   (80000)	/* mV x 100 */

#define GPU_DVFS_MAX_FREQ	GPU_DVFS_FREQ0 /* KHz */

#define GPU_DVFS_PTPOD_DISABLE_VOLT	GPU_DVFS_VOLT7

#define UNIVPLL_FREQ	(416000) /* KHz */

/* MT6763T Use VCORE DVFS and need on/off VCORE & frequency control only */
#define GPU_DVFS_FREQ0_6763T	 (800000)	/* KHz */
#define GPU_DVFS_FREQ1_6763T	 (530000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL_6763T	(GPU_DVFS_FREQ1_6763T)

#define GPU_DVFS_VOLT0_6763T	 (80000)	/* mV x 100 */
#define GPU_DVFS_VOLT1_6763T	 (70000)	/* mV x 100 */

#define GPU_DVFS_VSRAM0_6763T	  (90000)	/* mV x 100 */
#define GPU_DVFS_VSRAM1_6763T	  (80000)	/* mV x 100 */

#define GPU_DVFS_PTPOD_DISABLE_VOLT_MT6763T	GPU_DVFS_VOLT0_6763T

/* MT6763 Use VCORE DVFS and need on/off VCORE & frequency control only */
#define GPU_DVFS_FREQ0_6763	 (700000)	/* KHz */
#define GPU_DVFS_FREQ1_6763	 (530000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL_6763	(GPU_DVFS_FREQ1_6763)

#define GPU_DVFS_VOLT0_6763	 (80000)	/* mV x 100 */
#define GPU_DVFS_VOLT1_6763	 (70000)	/* mV x 100 */

#define GPU_DVFS_VSRAM0_6763	(90000)	/* mV x 100 */
#define GPU_DVFS_VSRAM1_6763	(80000)	/* mV x 100 */

#define GPU_DVFS_PTPOD_DISABLE_VOLT_MT6763	GPU_DVFS_VOLT0_6763

/*****************************************
 * PMIC settle time (us), should not be changed
 ******************************************/
#define PMIC_MAX_VSRAM GPU_DVFS_VSRAM0
#define PMIC_MIN_VPROC GPU_DVFS_VOLT15
#define PMIC_MAX_VPROC GPU_DVFS_VOLT0

#define PMIC_MIN_VCORE_6763T GPU_DVFS_VOLT1_6763T
#define PMIC_MAX_VCORE_6763T GPU_DVFS_VOLT0_6763T
#define PMIC_MIN_VCORE_6763 GPU_DVFS_VOLT1_6763
#define PMIC_MAX_VCORE_6763 GPU_DVFS_VOLT0_6763

/* mt6763 us * 100 BEGIN */
#define DELAY_FACTOR 625
#define HALF_DELAY_FACTOR 312
#define BUCK_INIT_US 750
#define VPROC_ENABLE_TIME_US 240 /* spec is 220, but more time means more safety */
#define VSRAM_GPU_ENABLE_TIME_US 120 /* spec is 110, but more time means more safety */

/* mt6763 END */


#define PMIC_CMD_DELAY_TIME	 5
#define MIN_PMIC_SETTLE_TIME	25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)	\
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)	\
	(((((old_volt) - (new_volt)) * 2)  / 625) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_ON_OFF_DELAY_US	   (200)
#define INVALID_SLEW_RATE	(0)

/* efuse */
#define GPUFREQ_EFUSE_INDEX		(8)
#define EFUSE_MFG_SPD_BOND_SHIFT	(8)
#define EFUSE_MFG_SPD_BOND_MASK		(0xF)
#define FUNC_CODE_EFUSE_INDEX		(22)
/*
 * LOG
 */
#define TAG	 "[Power/gpufreq] "

#define gpufreq_err(fmt, args...)	   \
	pr_err(TAG"[ERROR]"fmt, ##args)
#define gpufreq_warn(fmt, args...) \
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define gpufreq_info(fmt, args...)	\
	pr_warn(TAG""fmt, ##args)
#define gpufreq_dbg(fmt, args...)	   \
	do {								\
		if (mt_gpufreq_debug)		   \
			pr_warn(TAG""fmt, ##args);	 \
	} while (0)
#define gpufreq_ver(fmt, args...)	   \
	do {								\
		if (mt_gpufreq_debug)		   \
			pr_debug(TAG""fmt, ##args);	\
	} while (0)


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mt_gpufreq_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 200,
	.suspend = NULL,
	.resume = NULL,
};
#endif

static sampler_func g_pFreqSampler;
static sampler_func g_pVoltSampler;

static gpufreq_power_limit_notify g_pGpufreq_power_limit_notify;
#ifdef MT_GPUFREQ_INPUT_BOOST
static gpufreq_input_boost_notify g_pGpufreq_input_boost_notify;
#endif
static gpufreq_ptpod_update_notify g_pGpufreq_ptpod_update_notify;

/* Efuse Device ID */
typedef enum {
	C_MT6763TT = 0,
	C_MT6763T,
	C_MT6763,
} chip_ip_table;

chip_ip_table device_id;


/***************************
 * GPU DVFS OPP Table
 ****************************/
/* Full Yield */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, GPU_DVFS_VSRAM0, 0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1, GPU_DVFS_VSRAM1, 1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT2, GPU_DVFS_VSRAM2, 2),
	GPUOP(GPU_DVFS_FREQ3, GPU_DVFS_VOLT3, GPU_DVFS_VSRAM3, 3),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT4, GPU_DVFS_VSRAM4, 4),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT5, GPU_DVFS_VSRAM5, 5),
	GPUOP(GPU_DVFS_FREQ6, GPU_DVFS_VOLT6, GPU_DVFS_VSRAM6, 6),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT7, GPU_DVFS_VSRAM7, 7),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT8, GPU_DVFS_VSRAM8, 8),
	GPUOP(GPU_DVFS_FREQ9, GPU_DVFS_VOLT9, GPU_DVFS_VSRAM9, 9),
	GPUOP(GPU_DVFS_FREQ10, GPU_DVFS_VOLT10, GPU_DVFS_VSRAM10, 10),
	GPUOP(GPU_DVFS_FREQ11, GPU_DVFS_VOLT11, GPU_DVFS_VSRAM11, 11),
	GPUOP(GPU_DVFS_FREQ12, GPU_DVFS_VOLT12, GPU_DVFS_VSRAM12, 12),
	GPUOP(GPU_DVFS_FREQ13, GPU_DVFS_VOLT13, GPU_DVFS_VSRAM13, 13),
	GPUOP(GPU_DVFS_FREQ14, GPU_DVFS_VOLT14, GPU_DVFS_VSRAM14, 14),
	GPUOP(GPU_DVFS_FREQ15, GPU_DVFS_VOLT15, GPU_DVFS_VSRAM15, 15),
};

static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_1[] = {
	GPUOP(GPU_DVFS_FREQ0_6763T, GPU_DVFS_VOLT0_6763T, GPU_DVFS_VSRAM0_6763T, 0),
	GPUOP(GPU_DVFS_FREQ1_6763T, GPU_DVFS_VOLT1_6763T, GPU_DVFS_VSRAM1_6763T, 1),
};

static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_2[] = {
	GPUOP(GPU_DVFS_FREQ0_6763, GPU_DVFS_VOLT0_6763, GPU_DVFS_VSRAM0_6763, 0),
	GPUOP(GPU_DVFS_FREQ1_6763, GPU_DVFS_VOLT1_6763, GPU_DVFS_VSRAM1_6763, 1),
};

/*
 * AEE (SRAM debug)
 */
#ifdef MT_GPUFREQ_AEE_RR_REC
enum gpu_dvfs_state {
	GPU_DVFS_IS_DOING_DVFS = 0,
	GPU_DVFS_IS_VPROC_ENABLED,
};

static void _mt_gpufreq_aee_init(void)
{
	aee_rr_rec_gpu_dvfs_vgpu(0xFF);
	aee_rr_rec_gpu_dvfs_oppidx(0xFF);
	aee_rr_rec_gpu_dvfs_status(0xFC);
}
#endif

/**************************
 * enable GPU DVFS count
 ***************************/
static int g_gpufreq_dvfs_disable_count;

static unsigned int g_cur_gpu_freq = GPU_DVFS_FREQ0;	/* initial value */
static unsigned int g_cur_gpu_volt = GPU_DVFS_VOLT0;	/* initial value */
static unsigned int g_cur_gpu_idx = 0xFF;
static unsigned int g_cur_gpu_OPPidx = 0xFF;
static unsigned int g_vproc_sfchg_rrate = 3; /* just a default value */
static unsigned int g_vproc_sfchg_frate = 3; /* just a default value */
static unsigned int g_vsram_sfchg_rrate = 3; /* just a default value */
static unsigned int g_vsram_sfchg_frate = 3; /* just a default value */


static unsigned int g_cur_freq_init_keep;

static bool mt_gpufreq_ready;

/* In default settiing, freq_table[0] is max frequency, freq_table[num-1] is min frequency,*/
static unsigned int g_gpufreq_max_id;

/* If not limited, it should be set to freq_table[0] (MAX frequency) */
static unsigned int g_limited_max_id;
static unsigned int g_limited_min_id;

static bool mt_gpufreq_debug;
static bool mt_gpufreq_pause;
static bool mt_gpufreq_keep_max_frequency_state;
static bool mt_gpufreq_keep_opp_frequency_state;
#ifndef MTK_SSPM
static unsigned int mt_gpufreq_keep_opp_frequency;
#endif
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_volt_state;
static unsigned int mt_gpufreq_fixed_frequency;
static unsigned int mt_gpufreq_fixed_voltage;

static unsigned int mt_gpufreq_volt_enable;

static unsigned int mt_gpufreq_volt_enable_state;
static unsigned int mt_gpufreq_volt_lpm_state;
#ifdef MT_GPUFREQ_INPUT_BOOST
static unsigned int mt_gpufreq_input_boost_state = 1;
#endif
static bool g_limited_thermal_ignore_state;
#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
static bool g_limited_low_batt_volt_ignore_state;
#endif
#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
static bool g_limited_low_batt_volume_ignore_state;
#endif
#ifdef MT_GPUFREQ_OC_PROTECT
static bool g_limited_oc_ignore_state;
#endif

static bool mt_gpufreq_opp_max_frequency_state;
static unsigned int mt_gpufreq_opp_max_frequency;
static unsigned int mt_gpufreq_opp_max_index;

static unsigned int mt_gpufreq_dvfs_table_type;
#ifndef MTK_SSPM
static unsigned int mt_gpufreq_dvfs_table_cid;
#endif

/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);
static DEFINE_MUTEX(mt_gpufreq_power_lock);

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;
static struct mt_gpufreq_power_table_info *mt_gpufreqs_power;
#ifndef MTK_SSPM
static struct mt_gpufreq_clk_t *mt_gpufreq_clk;
#endif
static struct mt_gpufreq_pmic_t *mt_gpufreq_pmic;


/* static struct mt_gpufreq_power_table_info *mt_gpufreqs_default_power; */

static bool mt_gpufreq_ptpod_disable;
static int mt_gpufreq_ptpod_disable_idx;

static void mt_gpufreq_clock_switch(unsigned int freq_new);
static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new);
static void mt_gpufreq_volt_switch_vcore(unsigned int volt_old, unsigned int volt_new);
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new);
static unsigned int _mt_gpufreq_get_cur_volt(void);
static unsigned int _mt_gpufreq_get_cur_freq(void);
static void _mt_gpufreq_kick_pbm(int enable);
static int _mt_gpufreq_set_lpm_status(unsigned int unit, unsigned int enable_lpm);


#ifndef DISABLE_PBM_FEATURE
static bool g_limited_pbm_ignore_state;
static unsigned int mt_gpufreq_pbm_limited_gpu_power;	/* PBM limit power */
static unsigned int mt_gpufreq_pbm_limited_index;	/* Limited frequency index for PBM */
#ifdef MTK_SSPM
uint32_t g_sspm_cur_volt;
uint32_t g_sspm_power;
#endif
#define GPU_OFF_SETTLE_TIME_MS		(100)
struct delayed_work notify_pbm_gpuoff_work;
#endif

/* SSPM reserved memory */
/* #ifdef MTK_SSPM */
phys_addr_t gpu_fdvfs_phy_addr, gpu_fdvfs_virt_addr;
uint32_t gpu_dvfs_mem_size;

/* #endif */


int __attribute__ ((weak))
get_immediate_gpu_wrap(void)
{
	pr_err("get_immediate_gpu_wrap doesn't exist");
	return 0;
}

/*************************************************************************************
 * Get Post DIV value
 *
 * @breif
 * VCO needs proper post div value to get corresponding dds value to adjust PLL value
 * e.g: VCO range is 1.5GHz - 3.8GHz
 *	  required frequency is 900MHz, so pos div could be 2(1.8/2), 4(3.6/4), 8(X), 16(X)
 * It may have special requiremt by DE in different efuse value
 * e.g: In Olympus, efuse value(3'b001) ,VCO range is 1.5GHz - 3.8GHz
 *	  375MHz - 900MHz, It can only use post div 4, no pos div 2
 *
 * @param[in] freq: required frequency
 * @param[in] efuse: efuse value
 * return corresponding post div by freq and efuse
 **************************************************************************************/
static enum post_div_order_enum _get_post_div_order(unsigned int freq, unsigned int efuse)
{
	/* [Olympus]VCO range: 1.5G - 3.8GHz by div 1/2/4/8/16
	*
	*	PLL range: 125MHz - 3.8GHz
	*	Version(eFuse Value)			Info	 POSTDIV		   FOUT
	*	Free Version(efuse 3'b000)	 No limit  1/2/4/8/16  125MHz - 3800MHz
	*/
	enum post_div_order_enum post_div_order = POST_DIV4;

	post_div_order = POST_DIV4;

	gpufreq_dbg("@%s: freq = %d efuse = %d post_div_order = %d\n", __func__, freq, efuse, post_div_order);
	return post_div_order;
}

#ifdef MTK_SSPM
int mt_gpufreq_ap2sspm(unsigned int eCMD, unsigned int arg0, unsigned int arg1)
{
	gdvfs_data_t gdvfs_d;
	/*int md32Ret = -1;*/
	/*int apDebug = -1;*/

	gdvfs_d.cmd = eCMD;
	gdvfs_d.u.set_fv.arg[0] = arg0;
	gdvfs_d.u.set_fv.arg[1] = arg1;

	if (mt_gpufreq_debug)
		gpufreq_info("%s: calling cmd =(%d) arg0 = (%d) arg1 = (%d)\n", __func__, eCMD, arg0, arg1);
	/*
	 *	sspm_ipi_send_sync( $IPI_ID, $IPI_OPT, $sending_message, $IPI_ID_size, $pointer_to_save_md32return);
	 *	$IPI_ID_size == 0 -> use registed default size according to $IPI_ID
	 */
#if 0
	apDebug = sspm_ipi_send_sync(IPI_ID_GPU_DVFS, IPI_OPT_POLLING, &gdvfs_d, 0, &md32Ret, 1);
	/* apDebug = sspm_ipi_send_sync(IPI_ID_GPU_DVFS, IPI_OPT_WAIT, &gdvfs_d, 0, &md32Ret, 1); */

	if (apDebug < 0 || md32Ret < 0) {
		dump_stack();
		gpufreq_err("%s: Error when calling cmd =(%d) arg0 = (%d) arg1 = (%d)\n", __func__, eCMD, arg0, arg1);

		if (apDebug < 0)
			gpufreq_err("%s: AP side err (%d)\n", __func__, apDebug);
		if (md32Ret < 0)
			gpufreq_err("%s: SSPM err (%d)\n", __func__, md32Ret);

		md32Ret = -1;
	}
	return md32Ret;
#endif
	return 0;
}
#endif

#ifdef MT_GPUFREQ_INPUT_BOOST
static struct task_struct *mt_gpufreq_up_task;

static int mt_gpufreq_input_boost_task(void *data)
{
	while (1) {
		gpufreq_dbg("@%s: begin\n", __func__);

		if (g_pGpufreq_input_boost_notify != NULL) {
			gpufreq_dbg("@%s: g_pGpufreq_input_boost_notify\n", __func__);
			g_pGpufreq_input_boost_notify(g_gpufreq_max_id);
		}

		gpufreq_dbg("@%s: end\n", __func__);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;
	}

	return 0;
}


/*************************************************************************************
 * Input boost
 **************************************************************************************/
static void mt_gpufreq_input_event(struct input_handle *handle, unsigned int type,
				   unsigned int code, int value)
{
	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1)
		&& (mt_gpufreq_input_boost_state == 1)) {
		gpufreq_dbg("@%s: accept.\n", __func__);

		wake_up_process(mt_gpufreq_up_task);
	}
}

static int mt_gpufreq_input_connect(struct input_handler *handler,
					struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "gpufreq_ib";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void mt_gpufreq_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id mt_gpufreq_ids[] = {
	{.driver_info = 1},
	{},
};

static struct input_handler mt_gpufreq_input_handler = {
	.event = mt_gpufreq_input_event,
	.connect = mt_gpufreq_input_connect,
	.disconnect = mt_gpufreq_input_disconnect,
	.name = "gpufreq_ib",
	.id_table = mt_gpufreq_ids,
};
#endif /* mt_gpufreq_input_handler  */

/*
 * Power table calculation
 */
static void mt_gpufreq_power_calculation(unsigned int idx, unsigned int freq,
					  unsigned int volt, unsigned int temp)
{
#define GPU_ACT_REF_POWER		845	/* mW  */
#define GPU_ACT_REF_FREQ		850000	/* KHz */
#define GPU_ACT_REF_VOLT		90000	/* mV x 100 */

	unsigned int p_total = 0, p_dynamic = 0, ref_freq = 0, ref_volt = 0;
	int p_leakage = 0;

	p_dynamic = GPU_ACT_REF_POWER;
	ref_freq = GPU_ACT_REF_FREQ;
	ref_volt = GPU_ACT_REF_VOLT;

	p_dynamic = p_dynamic *
		((freq * 100) / ref_freq) *
		((volt * 100) / ref_volt) * ((volt * 100) / ref_volt) / (100 * 100 * 100);

#ifdef STATIC_PWR_READY2USE
	p_leakage =
		mt_spower_get_leakage(MT_SPOWER_GPU, (volt / 100), temp);
	if (!mt_gpufreq_volt_enable_state || p_leakage < 0)
		p_leakage = 0;
#else
	p_leakage = 71;
#endif

	p_total = p_dynamic + p_leakage;

	gpufreq_ver("%d: p_dynamic = %d, p_leakage = %d, p_total = %d, temp = %d\n",
			idx, p_dynamic, p_leakage, p_total, temp);

	mt_gpufreqs_power[idx].gpufreq_power = p_total;
}

/**************************************
 * Random seed generated for test
 ***************************************/
#ifdef MT_GPU_DVFS_RANDOM_TEST
static int mt_gpufreq_idx_get(int num)
{
	int random = 0, mult = 0, idx;

	random = jiffies & 0xF;

	while (1) {
		if ((mult * num) >= random) {
			idx = (mult * num) - random;
			break;
		}
		mult++;
	}
	return idx;
}
#endif


/* Set frequency and voltage at driver probe function */
#ifndef MTK_SSPM
static void mt_gpufreq_set_initial(void)
{
	unsigned int cur_volt = 0, cur_freq = 0;
	/*int i = 0;*/

	mutex_lock(&mt_gpufreq_lock);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif
	gpufreq_dbg("[Figo] Into mt_gpufreq_set_initial");
	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();

	/*
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_volt >= mt_gpufreqs[i].gpufreq_volt) {
			mt_gpufreq_set(cur_freq, mt_gpufreqs[i].gpufreq_khz,
				   cur_volt, mt_gpufreqs[i].gpufreq_volt);
			g_cur_gpu_OPPidx = i;
			gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
			_mt_gpufreq_kick_pbm(1);
			break;
		}
	}
	*/

	/* Not found, set to LPM */
	/*
	if (i == mt_gpufreqs_num) {
		gpufreq_err
			("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
			 cur_volt / 100);
		g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
		mt_gpufreq_set(cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
				   cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
	}
	*/

	g_cur_gpu_OPPidx = 0;
	mt_gpufreq_set(cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
				   cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

	g_cur_gpu_freq = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz;
	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
	g_cur_gpu_idx = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_idx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
}
#endif /* MTK_SSPM */


#ifndef DISABLE_PBM_FEATURE
static void mt_gpufreq_notify_pbm_gpuoff(struct work_struct *work)
{
	mutex_lock(&mt_gpufreq_lock);
	if (!mt_gpufreq_volt_enable_state)
		_mt_gpufreq_kick_pbm(0);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif


/* Set VPROC/VSRAM enable/disable when GPU clock be switched on/off */
unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	int ret = 0;
#ifdef MTK_SSPM
	/*
	* USE SSPM to switch VPROC
	*/
	ret =  mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, enable);

#else
	unsigned int reg_val = 0;

	mutex_lock(&mt_gpufreq_lock);

#ifdef VPROC_SET_BY_PMIC
	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		ret = DRIVER_NOT_READY;
		goto SET_EXIT;
	}

	if (device_id == C_MT6763TT) {
		if (enable == mt_gpufreq_volt_enable_state) {
			ret = 0;
			/* check if really consist */
			if (enable == 1) {
				/* check VSRAM first */
				ret = regulator_is_enabled(mt_gpufreq_pmic->reg_vsram);
				if (ret == 0)
					gpufreq_err("GPU VSRAM is OFF!!!\n");
				else if (ret < 0)
					gpufreq_err("FATAL: GPU VSRAM regulator error !!!\n");
				else {
					ret = regulator_is_enabled(mt_gpufreq_pmic->reg_vproc);
					if (ret == 0)
						gpufreq_err("VPROC is OFF!!!\n");
					else if (ret < 0)
						gpufreq_err("FATAL: VPROC regulator error !!!\n");
				}
			} else {
				/* check VPROC first */
				ret = regulator_is_enabled(mt_gpufreq_pmic->reg_vproc);
				if (ret > 0)
					gpufreq_err("VPROC is ON!!!\n");
				else if (ret < 0)
					gpufreq_err("FATAL: VPROC regulator error !!!\n");
				else {
					ret = regulator_is_enabled(mt_gpufreq_pmic->reg_vsram);
					if (ret > 0) {
						gpufreq_err("GPU VSRAM is ON!!!\n");
						ret = 0; /* get a chance to set off */
					} else if (ret < 0)
						gpufreq_err("FATAL: VPROC regulator error !!!\n");
				}
			}

			if (ret != 0)
				goto SET_EXIT;
		}

		if (mt_gpufreq_ptpod_disable == true) {
			if (enable == BUCK_OFF) {
				gpufreq_info("mt_gpufreq_ptpod_disable == true\n");
				ret = DRIVER_NOT_READY;
				goto SET_EXIT;
			}
		}

		if (enable == BUCK_ON) {
			/* Enable VSRAM */
			/*gpufreq_warn("start enable VSRAM...\n");*/
			ret = regulator_enable(mt_gpufreq_pmic->reg_vsram);
			if (ret) {
				gpufreq_err("Enable VSRAM failed! (%d)", ret);
				goto SET_EXIT;
			}
			udelay(VSRAM_GPU_ENABLE_TIME_US);

			/* Enable VPROC */
			/*gpufreq_warn("start enable VPROC...\n");*/
			ret = regulator_enable(mt_gpufreq_pmic->reg_vproc);
			if (ret) {
				gpufreq_err("Enable VPROC failed! (%d)", ret);
				goto SET_EXIT;
			}
			udelay(VPROC_ENABLE_TIME_US);

		} else {
			/* Try Disable VPROC */
			do {
				/*gpufreq_warn("start disable VPROC...\n");*/
				ret = regulator_disable(mt_gpufreq_pmic->reg_vproc);
				if (ret) {
					gpufreq_err("Disable VPROC failed! (%d)", ret);
					goto SET_EXIT;
				}
				/*udelay(VPROC_ENABLE_TIME_US);*/

				/*gpufreq_warn("start disable VSRAM...\n");*/
				ret = regulator_disable(mt_gpufreq_pmic->reg_vsram);
				if (ret) {
					gpufreq_err("Disable VSRAM failed! (%d)", ret);
					goto SET_EXIT;
				}
				/*udelay(VSRAM_GPU_ENABLE_TIME_US);*/

				if (regulator_is_enabled(mt_gpufreq_pmic->reg_vproc) == 0 &&
					regulator_is_enabled(mt_gpufreq_pmic->reg_vsram) == 0) {
					enable = BUCK_OFF;
				}
			} while (enable == BUCK_ENFORCE_OFF);
		}

#ifdef MT_GPUFREQ_AEE_RR_REC
		if (enable == 1)
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
						   (1 << GPU_DVFS_IS_VPROC_ENABLED));
		else
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
						   ~(1 << GPU_DVFS_IS_VPROC_ENABLED));
#endif
		reg_val = regulator_is_enabled(mt_gpufreq_pmic->reg_vproc);
	} else {
		if (enable == BUCK_ON)
			ret = vcorefs_request_dvfs_opp(KIR_GPU, OPP_0);
		else
			ret = vcorefs_request_dvfs_opp(KIR_GPU, OPP_UNREQ);
#ifdef VCORE_DVFS_READY
		if (ret != 0)
			gpufreq_err("@%s: VCORE LPM status set %d failed! (%d)\n", __func__, enable_lpm, ret);
#endif
	}
#endif
	/* Error checking */
	if (enable == 1 && reg_val == 0) {
		/* VPROC enable fail, dump info and trigger BUG() */
		gpufreq_err("@%s: enable = %x, reg_val = %d\n", __func__, enable, reg_val);
	}
#endif	/* MTK_SSPM */

#ifndef DISABLE_PBM_FEATURE
	if (enable == 1) {
		if (delayed_work_pending(&notify_pbm_gpuoff_work))
			cancel_delayed_work(&notify_pbm_gpuoff_work);
		else
			_mt_gpufreq_kick_pbm(1);
	} else {
		schedule_delayed_work(&notify_pbm_gpuoff_work, msecs_to_jiffies(GPU_OFF_SETTLE_TIME_MS));
	}

#endif

#ifndef MTK_SSPM
	mt_gpufreq_volt_enable_state = enable;
SET_EXIT:
	mutex_unlock(&mt_gpufreq_lock);
#endif

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);

static int _mt_gpufreq_set_lpm_status(unsigned int unit, unsigned int enable_lpm)
{
	int ret = 0;

	switch (unit)	{
	case VPROC:
		if (enable_lpm) /* Set to LPM */
			ret = pmic_config_interface(PMIC_RG_BUCK_VPROC_LP_ADDR, 1,
				PMIC_RG_BUCK_VPROC_LP_MASK, PMIC_RG_BUCK_VPROC_LP_SHIFT);
		else /* Back to normal */
			ret = pmic_config_interface(PMIC_RG_BUCK_VPROC_LP_ADDR, 0,
				PMIC_RG_BUCK_VPROC_LP_MASK, PMIC_RG_BUCK_VPROC_LP_SHIFT);
		if (ret != 0)
			gpufreq_err("@%s: Set VPROC LPM status %d failed! (%d)", __func__, enable_lpm, ret);
		break;
	case VSRAM:
		if (enable_lpm) /* Set to LPM */
			ret = pmic_config_interface(PMIC_RG_LDO_VSRAM_GPU_LP_ADDR, 1,
				PMIC_RG_LDO_VSRAM_GPU_LP_MASK, PMIC_RG_LDO_VSRAM_GPU_LP_SHIFT);
		else /* Back to normal */
			ret = pmic_config_interface(PMIC_RG_LDO_VSRAM_GPU_LP_ADDR, 0,
				PMIC_RG_LDO_VSRAM_GPU_LP_MASK, PMIC_RG_LDO_VSRAM_GPU_LP_SHIFT);
		if (ret != 0)
			gpufreq_err("@%s: Set VSRAM LPM status %d failed! (%d)", __func__, enable_lpm, ret);
		break;
	default:
		gpufreq_err("@%s: Unknown LPM unit!", __func__);
	}
	return ret;
}


/* Set VPROC enable/disable when GPU clock be switched on/off */
unsigned int mt_gpufreq_voltage_lpm_set(unsigned int enable_lpm)
{
	int ret = 0;

#ifdef MTK_SSPM
		/*
		* USE SSPM to switch VPROC
		*/
		ret =  mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, enable);
#else
		mutex_lock(&mt_gpufreq_lock);

#ifdef VPROC_SET_BY_PMIC
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		ret = DRIVER_NOT_READY;
		goto SET_LPM_EXIT;
	}

	if (device_id == C_MT6763TT) {
		if (enable_lpm && !mt_gpufreq_ptpod_disable) {
			ret = _mt_gpufreq_set_lpm_status(VPROC, 1);
			if (ret != 0)
				goto SET_LPM_EXIT;
			ret = _mt_gpufreq_set_lpm_status(VSRAM, 1);
			if (ret != 0)
				goto SET_LPM_EXIT;
		} else {
			ret = _mt_gpufreq_set_lpm_status(VSRAM, 0);
			if (ret != 0)
				goto SET_LPM_EXIT;
			ret = _mt_gpufreq_set_lpm_status(VPROC, 0);
			if (ret != 0)
				goto SET_LPM_EXIT;
		}
	} else {
		if (enable_lpm && !mt_gpufreq_ptpod_disable)
			vcorefs_request_dvfs_opp(KIR_GPU, OPP_UNREQ);
		else
			vcorefs_request_dvfs_opp(KIR_GPU, OPP_0);
	}
#endif
#endif /* MTK_SSPM */

#ifndef MTK_SSPM
	if (enable_lpm) {
		mt_gpufreq_volt_lpm_state = 1;
		mt_gpufreq_volt_enable_state = 0;
	} else {
		mt_gpufreq_volt_lpm_state = 0;
		mt_gpufreq_volt_enable_state = 1;
	}
SET_LPM_EXIT:
	mutex_unlock(&mt_gpufreq_lock);
#endif

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_lpm_set);

/************************************************
 * DVFS enable API for PTPOD
 *************************************************/

void mt_gpufreq_enable_by_ptpod(void)
{
#ifdef VOLT_SET_READY
	gpufreq_dbg("[Figo] Power off by mt_gpufreq_enable_by_ptpod\n");
	mt_gpufreq_voltage_enable_set(0);
#endif

	mt_gpufreq_ptpod_disable = false;
	gpufreq_info("mt_gpufreq enabled by ptpod\n");
}
EXPORT_SYMBOL(mt_gpufreq_enable_by_ptpod);

/************************************************
 * DVFS disable API for PTPOD
 *************************************************/
void mt_gpufreq_disable_by_ptpod(void)
{
	int i = 0, target_idx = 0;

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	mt_gpufreq_ptpod_disable = true;
	mt_gpufreq_voltage_lpm_set(0);
	gpufreq_info("mt_gpufreq disabled by ptpod\n");

	for (i = 0; i < mt_gpufreqs_num; i++) {
		target_idx = i;
		if (mt_gpufreqs_default[i].gpufreq_volt <= GPU_DVFS_PTPOD_DISABLE_VOLT)
			break;
	}

	mt_gpufreq_ptpod_disable_idx = target_idx;

#ifdef VOLT_SET_READY
	gpufreq_dbg("[Figo] Power on by mt_gpufreq_disable_by_ptpod\n");
	mt_gpufreq_voltage_enable_set(1);
#endif
	mt_gpufreq_target(target_idx);
}
EXPORT_SYMBOL(mt_gpufreq_disable_by_ptpod);

/************************************************
 * API to switch back default voltage setting for GPU PTPOD disabled
 *************************************************/
void mt_gpufreq_restore_default_volt(void)
{
	int i;

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < mt_gpufreqs_num; i++) {
		mt_gpufreqs[i].gpufreq_volt = mt_gpufreqs_default[i].gpufreq_volt;
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n", __func__, i,
				mt_gpufreqs[i].gpufreq_volt);
	}

	if (device_id == C_MT6763TT) /* Use VPROC for power Control */
		mt_gpufreq_volt_switch(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
	else /* Use VCORE for power Control */
		mt_gpufreq_volt_switch_vcore(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

	mutex_unlock(&mt_gpufreq_lock);
}
EXPORT_SYMBOL(mt_gpufreq_restore_default_volt);

/* Set voltage because PTP-OD modified voltage table by PMIC wrapper */
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
	int i;			/* , idx; */
	/* unsigned long flags; */

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return DRIVER_NOT_READY;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < array_size; i++) {
		mt_gpufreqs[i].gpufreq_volt = pmic_volt[i];
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %d\n", __func__, i,
				mt_gpufreqs[i].gpufreq_volt);
	}

	if (device_id == C_MT6763TT) /* Use VPROC for power Control */
		mt_gpufreq_volt_switch(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
	else /* Use VCORE for power Control */
		mt_gpufreq_volt_switch_vcore(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
	if (g_pGpufreq_ptpod_update_notify != NULL)
		g_pGpufreq_ptpod_update_notify();
	mutex_unlock(&mt_gpufreq_lock);

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt);

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
	return mt_gpufreqs_num;
}
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return DRIVER_NOT_READY;
	}

	if (idx < mt_gpufreqs_num) {
		gpufreq_dbg("@%s: idx = %d, frequency= %d\n", __func__, idx,
				mt_gpufreqs[idx].gpufreq_khz);
		return mt_gpufreqs[idx].gpufreq_khz;
	}

	gpufreq_dbg("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_freq_by_idx);

int mt_gpufreq_get_idx_by_target_freq(unsigned int target_freq)
{
	int i = mt_gpufreqs_num - 1;

	gpufreq_err("@%s: freq = %u\n", __func__, target_freq);
	while (i >= 0) {
		if (mt_gpufreqs[i--].gpufreq_khz >= target_freq)
			goto TARGET_EXIT;
	}

TARGET_EXIT:
	gpufreq_err("@%s: idx = %u\n", __func__, i+1);
	return i+1;
}

unsigned int mt_gpufreq_get_volt_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return DRIVER_NOT_READY;
	}

	if (idx < mt_gpufreqs_num) {
		gpufreq_dbg("@%s: idx = %d, voltage= %d\n", __func__, idx,
				mt_gpufreqs[idx].gpufreq_volt);
		return mt_gpufreqs[idx].gpufreq_volt;
	}

	gpufreq_dbg("@%s: idx = %d, NOT found! return 0!\n", __func__, idx);
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_get_volt_by_idx);

#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
static void mt_update_gpufreqs_power_table(void)
{
	int i = 0;
#ifdef MTK_SSPM
	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < mt_gpufreqs_num; i++) {
		mt_gpufreqs[i].gpufreq_volt =
			mt_gpufreqs_power[i].gpufreq_volt = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_VOLT);
		mt_gpufreqs_power[i].gpufreq_power = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_POWER);

		gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_khz);
		gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_volt);
		gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_power);
	}

	mutex_unlock(&mt_gpufreq_lock);
#else
	int temp = 0;
	unsigned int freq = 0, volt = 0;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready\n", __func__);
		return;
	}

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	gpufreq_dbg("@%s, temp = %d\n", __func__, temp);

	mutex_lock(&mt_gpufreq_lock);

	if ((temp >= -20) && (temp <= 125)) {
		for (i = 0; i < mt_gpufreqs_num; i++) {
			freq = mt_gpufreqs_power[i].gpufreq_khz;
			volt = mt_gpufreqs_power[i].gpufreq_volt;

			mt_gpufreq_power_calculation(i, freq, volt, temp);

			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_khz);
			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_volt);
			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_power);
		}
	} else
		gpufreq_err("@%s: temp < 0 or temp > 125, NOT update power table!\n", __func__);

	mutex_unlock(&mt_gpufreq_lock);
#endif
}
#endif

static void mt_setup_gpufreqs_power_table(int num)
{
	int i = 0, temp = 0;

	mt_gpufreqs_power = kzalloc((num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mt_gpufreqs_power == NULL)
		return;
#ifdef MTK_SSPM
	MTK_UNREFERENCED_PARAMETER(temp);

	for (i = 0; i < num ; i++) {
		mt_gpufreqs_power[i].gpufreq_khz = mt_gpufreqs[i].gpufreq_khz;
		mt_gpufreqs_power[i].gpufreq_volt = mt_gpufreqs[i].gpufreq_volt;
		mt_gpufreqs_power[i].gpufreq_power = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_POWER);
	}

#else
#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	gpufreq_dbg("@%s: temp = %d\n", __func__, temp);

	if ((temp < -20) || (temp > 125)) {
		gpufreq_dbg("@%s: temp < 0 or temp > 125!\n", __func__);
		temp = 65;
	}

	for (i = 0; i < num; i++) {
		/* fill-in freq and volt in power table */
		mt_gpufreqs_power[i].gpufreq_khz = mt_gpufreqs[i].gpufreq_khz;
		mt_gpufreqs_power[i].gpufreq_volt = mt_gpufreqs[i].gpufreq_volt;

		mt_gpufreq_power_calculation(i,
						 mt_gpufreqs_power[i].gpufreq_khz,
						 mt_gpufreqs_power[i].gpufreq_volt,
						 temp);

		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_khz = %u\n", i,
				 mt_gpufreqs_power[i].gpufreq_khz);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_volt = %u\n", i,
				 mt_gpufreqs_power[i].gpufreq_volt);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_power = %u\n", i,
				 mt_gpufreqs_power[i].gpufreq_power);
	}
#endif

#ifdef CONFIG_THERMAL
	mtk_gpufreq_register(mt_gpufreqs_power, num);
#endif
}

/***********************************************
 * register frequency table to gpufreq subsystem
 ************************************************/
static int mt_setup_gpufreqs_table(struct mt_gpufreq_table_info *freqs, int num)
{
	int i = 0;

	mt_gpufreqs = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	mt_gpufreqs_default = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	if (mt_gpufreqs == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mt_gpufreqs[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs[i].gpufreq_volt = freqs[i].gpufreq_volt;
		mt_gpufreqs[i].gpufreq_vsram = freqs[i].gpufreq_vsram;
		mt_gpufreqs[i].gpufreq_idx = freqs[i].gpufreq_idx;

		mt_gpufreqs_default[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs_default[i].gpufreq_volt = freqs[i].gpufreq_volt;
		mt_gpufreqs_default[i].gpufreq_vsram = freqs[i].gpufreq_vsram;
		mt_gpufreqs_default[i].gpufreq_idx = freqs[i].gpufreq_idx;

		gpufreq_dbg("freqs[%d].gpufreq_khz = %u\n", i, freqs[i].gpufreq_khz);
		gpufreq_dbg("freqs[%d].gpufreq_volt = %u\n", i, freqs[i].gpufreq_volt);
		gpufreq_dbg("freqs[%d].gpufreq_vsram = %u\n", i, freqs[i].gpufreq_vsram);
		gpufreq_dbg("freqs[%d].gpufreq_idx = %u\n", i, freqs[i].gpufreq_idx);
	}

	mt_gpufreqs_num = num;

	g_limited_max_id = 0;
	g_limited_min_id = mt_gpufreqs_num - 1;

	gpufreq_info("@%s: g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", __func__, g_cur_gpu_freq,
			 g_cur_gpu_volt);

	mt_setup_gpufreqs_power_table(num);

	return 0;
}

/**************************************
 * check if maximum frequency is needed
 ***************************************/
static int mt_gpufreq_keep_max_freq(unsigned int freq_old, unsigned int freq_new)
{
	if (mt_gpufreq_keep_max_frequency_state == true)
		return 1;

	return 0;
}

/*****************************
 * set GPU DVFS status
 ******************************/
int mt_gpufreq_state_set(int enabled)
{
	if (enabled) {
		if (!mt_gpufreq_pause) {
			gpufreq_dbg("gpufreq already enabled\n");
			return 0;
		}

		/*****************
		 * enable GPU DVFS
		 ******************/
		g_gpufreq_dvfs_disable_count--;
		gpufreq_dbg("enable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
				g_gpufreq_dvfs_disable_count);

		/***********************************************
		 * enable DVFS if no any module still disable it
		 ************************************************/
		if (g_gpufreq_dvfs_disable_count <= 0)
			mt_gpufreq_pause = false;
		else
			gpufreq_warn("someone still disable gpufreq, cannot enable it\n");
	} else {
		/******************
		 * disable GPU DVFS
		 *******************/
		g_gpufreq_dvfs_disable_count++;
		gpufreq_dbg("disable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
				g_gpufreq_dvfs_disable_count);

		if (mt_gpufreq_pause) {
			gpufreq_dbg("gpufreq already disabled\n");
			return 0;
		}

		mt_gpufreq_pause = true;
	}

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_state_set);


static unsigned int mt_gpufreq_dds_calc(unsigned int freq_khz, enum post_div_order_enum post_div_order)
{
	unsigned int dds = 0;

	gpufreq_dbg("@%s: request freq = %d, div_order = %d\n",
			__func__, freq_khz, post_div_order);
	/*
	 * Below freq range is Austin @ mt6763 only:
	 *
	 *	dds formula is consistent with Elbrus
	 *
	 */
	if ((freq_khz >= GPU_DVFS_FREQ15) && (freq_khz <= GPU_DVFS_FREQ0)) {
		dds = (((freq_khz / TO_MHz_HEAD * (1 << post_div_order)) << DDS_SHIFT)
			/ GPUPLL_FIN + ROUNDING_VALUE) / TO_MHz_TAIL;
	} else {
		gpufreq_err("@%s: target freq_khz(%d) out of range!\n", __func__, freq_khz);
		/* WARN_ON(); */
	}

	return dds;
}

#ifndef MTK_SSPM
#if 0 /* Need to switch pll in 6763 series */
static void gpu_dvfs_switch_to_univpll(bool on)
{
	clk_prepare_enable(mt_gpufreq_clk->clk_mux);

	if (on) {
		clk_set_parent(mt_gpufreq_clk->clk_mux, mt_gpufreq_clk->clk_sub_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
	} else {
		clk_set_parent(mt_gpufreq_clk->clk_mux, mt_gpufreq_clk->clk_main_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
	}
}
#endif

static void mt_gpufreq_clock_switch_transient(unsigned int freq_new,  enum post_div_order_enum post_div_order)
{
	unsigned int cur_volt;
	unsigned int cur_freq;
	unsigned int dds;

	/*	GPUPLL_CON1: 31: MMPLL_SDM_PCW_CHG
	*			  26-24: GPUPLL_POSDIV
	*				  000: /1
	*				  001: /2
	*				  010: /4
	*				  011: /8
	*				  100: /18
	*			  21-0:MMPLL_SDM_PCW
	*/

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();
	dds = mt_gpufreq_dds_calc(freq_new, post_div_order);
	gpufreq_dbg("@%s: request GPU dds = 0x%x, cur_volt = %d, cur_freq = %d\n",
			__func__, dds, cur_volt, cur_freq);

	gpufreq_dbg("@%s: request GPUPLL_CON1 = 0x%x\n",
			__func__, DRV_Reg32(GPUPLL_CON1));

	/* Step2. Modify gpupll_ck */
	DRV_WriteReg32(GPUPLL_CON1, (0x80000000) | (post_div_order << POST_DIV_SHIFT) | dds);
	DRV_WriteReg32(GPUPLL_CON0, 0x00000181);
	udelay(20);

}
#endif
/* static void _mt_gpufreq_set_cur_freq(unsigned int freq_new) */
static void mt_gpufreq_clock_switch(unsigned int freq_new)
{
#ifndef MTK_SSPM
	unsigned int mfgpll;

	if (mt_gpufreq_dvfs_table_cid == 0)
		mt_gpufreq_clock_switch_transient(freq_new, _get_post_div_order(freq_new, mt_gpufreq_dvfs_table_cid));
	else
		gpufreq_err("@%s: efuse number type(%d)\n", __func__, mt_gpufreq_dvfs_table_cid);

	mfgpll = DRV_Reg32(GPUPLL_CON1);
	gpufreq_dbg("@%s: freq_new = %d, mfgpll = 0x%x\n", __func__, freq_new, mfgpll);
#endif
	if (g_pFreqSampler != NULL)
		g_pFreqSampler(freq_new);

}

static int mt_gpufreq_get_idx_by_target_volt(unsigned int target_volt)
{
	int i = mt_gpufreqs_num - 1;

	while (i >= 0) {
		if (mt_gpufreqs[i--].gpufreq_volt >= target_volt)
			goto TARGET_EXIT;
	}

TARGET_EXIT:
	return i+1;
}

#ifndef MTK_SSPM
static unsigned int _calculate_vproc_sfchg_rate(bool isRising)
{
	unsigned int sfchg_rate_reg;
	unsigned int sfchg_rate_vproc;

	/* [MT6763] RG_LDO_VSRAM_PROC_SFCHG_RRATE (rising rate) and RG_LDO_VSRAM_PROC_SFCHG_FRATE (falling rate)
	*	4:	0.19us
	*	8:	0.34us
	*	11:	0.46us
	*	17:	0.69us
	*	23:	0.92us
	*	25:	1us
	*/
	if (isRising) {
		pmic_read_interface(MT6356_BUCK_VPROC_CFG0, &sfchg_rate_reg,
		PMIC_RG_BUCK_VPROC_SFCHG_RRATE_MASK, PMIC_RG_BUCK_VPROC_SFCHG_RRATE_SHIFT);
	} else {
		pmic_read_interface(MT6356_BUCK_VPROC_CFG0, &sfchg_rate_reg,
		PMIC_RG_BUCK_VPROC_SFCHG_FRATE_MASK, PMIC_RG_BUCK_VPROC_SFCHG_FRATE_SHIFT);
	}

	sfchg_rate_vproc = 1;

	gpufreq_dbg("@%s: isRising(%d), sfchg_rate_vproc = %d, sfchg_rate_reg = %x\n",
		__func__, isRising, sfchg_rate_vproc, sfchg_rate_reg);

	return sfchg_rate_vproc;
}
static unsigned int _calculate_vsram_sfchg_rate(bool isRising)
{
	unsigned int sfchg_rate_reg;
	unsigned int sfchg_rate_vsram;

	/* [MT6763] RG_LDO_VSRAM_GPU_SFCHG_RRATE and RG_LDO_VSRAM_GPU_SFCHG_FRATE
	*	4:	0.19us
	*	8:	0.34us
	*	11:	0.46us
	*	17:	0.69us
	*	23:	0.92us
	*	25:	1us
	*/

	if (isRising) {
		pmic_read_interface(MT6356_LDO_VSRAM_GPU_CFG0, &sfchg_rate_reg,
		PMIC_RG_LDO_VSRAM_GPU_SFCHG_RRATE_MASK, PMIC_RG_LDO_VSRAM_GPU_SFCHG_RRATE_SHIFT);
		sfchg_rate_vsram = 1;
	} else {
		pmic_read_interface(MT6356_LDO_VSRAM_GPU_CFG0, &sfchg_rate_reg,
		PMIC_RG_LDO_VSRAM_GPU_SFCHG_FRATE_MASK, PMIC_RG_LDO_VSRAM_GPU_SFCHG_FRATE_SHIFT);
		sfchg_rate_vsram = 1;
	}

	gpufreq_dbg("@%s: isRising(%d), sfchg_rate_vsram = %d, sfchg_rate_reg = %x\n",
		__func__, isRising, sfchg_rate_vsram, sfchg_rate_reg);

	return sfchg_rate_vsram;
}

#endif

static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new)
{
	unsigned int target_vsram;
	unsigned int target_old_vsram;
	unsigned int max_diff;
	unsigned int delay_unit_us;
	unsigned int sfchg_rate_vproc;
	unsigned int sfchg_rate_vsram;
	int i;

	gpufreq_dbg("@%s: volt_new = %d\n", __func__, volt_new);

	i = mt_gpufreq_get_idx_by_target_volt(volt_new);
	target_vsram = mt_gpufreqs[i].gpufreq_vsram;

	i = mt_gpufreq_get_idx_by_target_volt(volt_old);
	target_old_vsram = mt_gpufreqs[i].gpufreq_vsram;

#ifdef VPROC_SET_BY_PMIC

	if (volt_new > volt_old) {
		/* rising rate */
		sfchg_rate_vsram = g_vsram_sfchg_rrate;
		sfchg_rate_vproc = g_vproc_sfchg_rrate;

		/* VSRAM */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vsram,
							target_vsram*10, (PMIC_MAX_VSRAM*10) + 125);
		max_diff = target_vsram - target_old_vsram;
		delay_unit_us = (max_diff / DELAY_FACTOR) + 1;
		gpufreq_dbg("1. udelay us(%d): delay_unit_us(%d) x sfchg_rate_vsram(%d)\n",
					delay_unit_us * sfchg_rate_vsram, delay_unit_us, sfchg_rate_vsram);
		udelay(delay_unit_us * sfchg_rate_vsram);

		/* VPROC */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vproc,
							volt_new*10, (PMIC_MAX_VPROC*10) + 125);
		max_diff = volt_new - volt_old;
		delay_unit_us = (max_diff / DELAY_FACTOR) + 1;
		gpufreq_dbg("2. udelay us(%d): delay_unit_us(%d) x sfchg_rate_vproc(%d)\n",
					delay_unit_us * sfchg_rate_vproc, delay_unit_us, sfchg_rate_vproc);
		udelay(delay_unit_us * sfchg_rate_vproc);

	} else {
		/* falling rate */
		sfchg_rate_vsram = g_vsram_sfchg_frate;
		sfchg_rate_vproc = g_vproc_sfchg_frate;

		/* VPROC */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vproc,
							volt_new*10, (PMIC_MAX_VPROC*10) + 125);
		max_diff = volt_old - volt_new;
		delay_unit_us = (max_diff / DELAY_FACTOR) + 1;
		gpufreq_dbg("3. udelay us(%d): delay_unit_us(%d) x sfchg_rate_vproc(%d)\n",
					delay_unit_us * sfchg_rate_vproc, delay_unit_us, sfchg_rate_vproc);
		udelay(delay_unit_us * sfchg_rate_vproc);

		/* VSRAM */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vsram,
							target_vsram*10, (PMIC_MAX_VSRAM*10) + 125);
		max_diff = target_old_vsram - target_vsram;
		delay_unit_us = (max_diff / DELAY_FACTOR) + 1;
		gpufreq_dbg("4. udelay us(%d): delay_unit_us(%d) x sfchg_rate_vsram(%d)\n",
					delay_unit_us * sfchg_rate_vsram, delay_unit_us, sfchg_rate_vsram);
		udelay(delay_unit_us * sfchg_rate_vsram);
	}

#endif
	if (g_pVoltSampler != NULL)
		g_pVoltSampler(volt_new);
}

static void mt_gpufreq_volt_switch_vcore(unsigned int volt_old, unsigned int volt_new)
{
	if (volt_new > volt_old) {
		vcorefs_request_dvfs_opp(KIR_GPU, OPP_0);
	} else if (volt_new < volt_old) {
		vcorefs_request_dvfs_opp(KIR_GPU, OPP_UNREQ);
	}
}

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
	unsigned long mfgpll = 0;
	unsigned int post_div_order = 0;
	unsigned int freq_khz = 0;
#ifdef MTK_SSPM
	freq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_FREQ, 0);
#else
	unsigned long dds;

	mfgpll = DRV_Reg32(GPUPLL_CON1);
	post_div_order = (mfgpll & (0x7 << POST_DIV_SHIFT)) >> POST_DIV_SHIFT;

	/*
	 *	become dds
	 */
	dds = mfgpll & (0x3FFFFF);

	freq_khz = (((dds * TO_MHz_TAIL + ROUNDING_VALUE)*GPUPLL_FIN) >> DDS_SHIFT)
		/ (1 << post_div_order) *  TO_MHz_HEAD;

		if (freq_khz < DIV4_MIN_FREQ || freq_khz > DIV4_MAX_FREQ)
			gpufreq_err("Invalid out-of-bound freq %d KHz, mfgpll value = 0x%lx\n",
				freq_khz, mfgpll);
		else
#endif
			gpufreq_dbg("mfgpll = 0x%lx, freq = %d KHz, div_order = %d\n",
				mfgpll, freq_khz, post_div_order);

	return freq_khz;		/* KHz */
}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{
	unsigned int gpu_volt = 0;
#ifdef MTK_SSPM
	gpu_volt = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_VOLT, 0);
#else
	if (device_id == C_MT6763TT) {
		/* WARRNING: regulator_get_voltage prints uV */
		gpu_volt = regulator_get_voltage(mt_gpufreq_pmic->reg_vproc) / 10;
		gpufreq_dbg("gpu_dvfs_get_cur_volt:[PMIC] volt = %d\n", gpu_volt);
	} else {
		gpu_volt = regulator_get_voltage(mt_gpufreq_pmic->reg_vcore) / 10;
		gpufreq_dbg("gpu_dvfs_get_cur_volt:[PMIC] volt = %d\n", gpu_volt);
	}
#endif

	return gpu_volt;
}

static void _mt_gpufreq_kick_pbm(int enable)
{
#ifndef DISABLE_PBM_FEATURE
	static unsigned int power;
	static unsigned int cur_volt;
#ifdef MTK_SSPM
	if (enable == 2) { /* Kick from SSPM */
		kicker_pbm_by_gpu(true, g_sspm_power, g_sspm_cur_volt / 100);
		cur_volt = g_sspm_cur_volt;
		power = g_sspm_power;
	} else if (enable == 1) { /* VPROC power on */
		kicker_pbm_by_gpu(true, power, cur_volt / 100);
	} else {
		kicker_pbm_by_gpu(false, 0, cur_volt / 100);
	}
#else
	int i;
	int tmp_idx = -1;
	unsigned int found = 0;
	unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

	cur_volt = _mt_gpufreq_get_cur_volt();

	if (enable) {
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs_power[i].gpufreq_khz == cur_freq) {
				/* record idx since current voltage may not in DVFS table */
				tmp_idx = i;

				if (mt_gpufreqs_power[i].gpufreq_volt == cur_volt) {
					power = mt_gpufreqs_power[i].gpufreq_power;
					found = 1;
					kicker_pbm_by_gpu(true, power, cur_volt / 100);
					gpufreq_dbg
						("@%s: request GPU power = %d, cur_volt = %d, cur_freq = %d\n",
						 __func__, power, cur_volt / 100, cur_freq);
					return;
				}
			}
		}

		if (!found) {
			gpufreq_dbg("@%s: tmp_idx = %d\n", __func__, tmp_idx);

			if (tmp_idx != -1 && tmp_idx < mt_gpufreqs_num) {
				/* use freq to found corresponding power budget */
				power = mt_gpufreqs_power[tmp_idx].gpufreq_power;
				kicker_pbm_by_gpu(true, power, cur_volt / 100);
				gpufreq_dbg
					("@%s: request GPU power = %d, cur_volt = %d, cur_freq = %d\n",
					 __func__, power, cur_volt / 100, cur_freq);
			} else {
				gpufreq_warn("@%s: Cannot found request power in power table!\n",
						 __func__);
				gpufreq_warn("cur_freq = %dKHz, cur_volt = %dmV\n", cur_freq,
						 cur_volt / 100);
			}
		}
	} else {
		kicker_pbm_by_gpu(false, 0, cur_volt / 100);
	}
#endif
#endif
}

/*****************************************
 * frequency ramp up and ramp down handler
 ******************************************/
/***********************************************************
 * [note]
 * 1. frequency ramp up need to wait voltage settle
 * 2. frequency ramp down do not need to wait voltage settle
 ************************************************************/
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new)
{
	if (freq_new > freq_old) {
		if (device_id == C_MT6763TT)
			mt_gpufreq_volt_switch(volt_old, volt_new);
		else
			mt_gpufreq_volt_switch_vcore(volt_old, volt_new);
		mt_gpufreq_clock_switch(freq_new);
	} else {
		mt_gpufreq_clock_switch(freq_new);
		if (device_id == C_MT6763TT)
			mt_gpufreq_volt_switch(volt_old, volt_new);
		else
			mt_gpufreq_volt_switch_vcore(volt_old, volt_new);
	}

	g_cur_gpu_freq = freq_new;
	g_cur_gpu_volt = volt_new;

	_mt_gpufreq_kick_pbm(1);
}

/**********************************
 * gpufreq target callback function
 ***********************************/
/*************************************************
 * [note]
 * 1. handle frequency change request
 * 2. call mt_gpufreq_set to set target frequency
 **************************************************/
unsigned int mt_gpufreq_target(unsigned int idx)
{
#ifdef MTK_SSPM
	if (idx > (mt_gpufreqs_num - 1)) {
		gpufreq_err("@%s: idx out of range! idx = %d\n", __func__, idx);
		return -1;
	}

	if (mt_gpufreq_pause == false)
		return mt_gpufreq_ap2sspm(IPI_GPU_DVFS_TARGET_FREQ_IDX, idx, SUB_CMD_MOD_FREQ);

	gpufreq_warn("GPU DVFS pause!\n");

	return -1;
#else
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_idx, target_OPPidx;

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	return 0;
#endif

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_pause == true) {
		gpufreq_warn("GPU DVFS pause!\n");
		mutex_unlock(&mt_gpufreq_lock);
		return DRIVER_NOT_READY;
	}

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("GPU DVFS not ready!\n");
		mutex_unlock(&mt_gpufreq_lock);
		return DRIVER_NOT_READY;
	}

	if (mt_gpufreq_volt_enable_state == 0) {
		gpufreq_dbg("mt_gpufreq_volt_enable_state == 0! return\n");
		mutex_unlock(&mt_gpufreq_lock);
		return DRIVER_NOT_READY;
	}
#ifdef MT_GPU_DVFS_RANDOM_TEST
	idx = mt_gpufreq_idx_get(5);
	gpufreq_dbg("@%s: random test index is %d !\n", __func__, idx);
#endif

	if (idx > (mt_gpufreqs_num - 1)) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_err("@%s: idx out of range! idx = %d\n", __func__, idx);
		return -1;
	}

	/**********************************
	 * look up for the target GPU OPP
	 ***********************************/
	target_freq = mt_gpufreqs[idx].gpufreq_khz;
	target_volt = mt_gpufreqs[idx].gpufreq_volt;
	target_idx = mt_gpufreqs[idx].gpufreq_idx;
	target_OPPidx = idx;

	gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n", __func__, target_freq,
			target_OPPidx);

	/**********************************
	 * Check if need to keep max frequency
	 ***********************************/
	if (mt_gpufreq_keep_max_freq(g_cur_gpu_freq, target_freq)) {
		target_freq = mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz;
		target_volt = mt_gpufreqs[g_gpufreq_max_id].gpufreq_volt;
		target_idx = mt_gpufreqs[g_gpufreq_max_id].gpufreq_idx;
		target_OPPidx = g_gpufreq_max_id;
		gpufreq_dbg("Keep MAX frequency %d !\n", target_freq);
	}

	/************************************************
	 * If /proc command keep opp frequency.
	 *************************************************/
	if (mt_gpufreq_keep_opp_frequency_state == true) {
		target_freq = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
		target_volt = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
		target_idx = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_idx;
		target_OPPidx = mt_gpufreq_keep_opp_index;
		gpufreq_dbg("Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n", target_freq,
				target_volt, target_OPPidx);
	}

	/************************************************
	 * If /proc command fix the frequency.
	 *************************************************/
	if (mt_gpufreq_fixed_freq_volt_state == true) {
		target_freq = mt_gpufreq_fixed_frequency;
		target_volt = mt_gpufreq_fixed_voltage;
		target_idx = 0;
		target_OPPidx = 0;
		gpufreq_dbg("Fixed! fixed frequency %d, fixed voltage %d\n", target_freq,
				target_volt);
	}

	/************************************************
	 * If /proc command keep opp max frequency.
	 *************************************************/
	if (mt_gpufreq_opp_max_frequency_state == true) {
		if (target_freq > mt_gpufreq_opp_max_frequency) {
			target_freq = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
			target_idx = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_idx;
			target_OPPidx = mt_gpufreq_opp_max_index;

			gpufreq_dbg
				("opp max freq! opp max frequency %d, opp max voltage %d, opp max idx %d\n",
				 target_freq, target_volt, target_OPPidx);
		}
	}

	/************************************************
	 * PBM limit
	 *************************************************/
#ifndef DISABLE_PBM_FEATURE
	if (mt_gpufreq_pbm_limited_index != 0) {
		if (target_freq > mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_khz) {
			/*********************************************
			* target_freq > limited_freq, need to adjust
			**********************************************/
			target_freq = mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_volt;
			target_OPPidx = mt_gpufreq_pbm_limited_index;
			gpufreq_dbg("Limit! Thermal/Power limit gpu frequency %d\n",
					mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_khz);
		}
	}
#endif

	/************************************************
	 * Thermal/Power limit
	 *************************************************/
	if (g_limited_max_id != 0) {
		if (target_freq > mt_gpufreqs[g_limited_max_id].gpufreq_khz) {
			/*********************************************
			 * target_freq > limited_freq, need to adjust
			 **********************************************/
			target_freq = mt_gpufreqs[g_limited_max_id].gpufreq_khz;
			target_volt = mt_gpufreqs[g_limited_max_id].gpufreq_volt;
			target_idx = mt_gpufreqs[g_limited_max_id].gpufreq_idx;
			target_OPPidx = g_limited_max_id;
			gpufreq_info("Limit! Thermal/Power limit gpu frequency %d\n",
					 mt_gpufreqs[g_limited_max_id].gpufreq_khz);
		}
	}

	/************************************************
	 * DVFS keep at max freq when PTPOD initial
	 *************************************************/
	if (mt_gpufreq_ptpod_disable == true) {
#if 1
		target_freq = mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_khz;
		target_volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
		target_idx = mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_idx;
		target_OPPidx = mt_gpufreq_ptpod_disable_idx;
		gpufreq_dbg("PTPOD disable dvfs, mt_gpufreq_ptpod_disable_idx = %d\n",
				mt_gpufreq_ptpod_disable_idx);
#else
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_dbg("PTPOD disable dvfs, return\n");
		return 0;
#endif
	}

	/************************************************
	 * target frequency == current frequency, skip it
	 *************************************************/
	if (g_cur_gpu_freq == target_freq && g_cur_gpu_volt == target_volt) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due to same frequency\n",
				g_cur_gpu_freq, target_freq);
		return 0;
	}

	gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n", g_cur_gpu_freq,
			target_freq);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
	aee_rr_rec_gpu_dvfs_oppidx(target_OPPidx);
#endif

	/******************************
	 * set to the target frequency
	 *******************************/
	mt_gpufreq_set(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt);

	g_cur_gpu_idx = target_idx;
	g_cur_gpu_OPPidx = target_OPPidx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
#endif
	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_target);


/********************************************
 *	   POWER LIMIT RELATED
 ********************************************/
enum {
	IDX_THERMAL_LIMITED,
	IDX_LOW_BATT_VOLT_LIMITED,
	IDX_LOW_BATT_VOLUME_LIMITED,
	IDX_OC_LIMITED,

	NR_IDX_POWER_LIMITED,
};

/* NO need to throttle when OC */
#ifdef MT_GPUFREQ_OC_PROTECT
static unsigned int mt_gpufreq_oc_level;

#define MT_GPUFREQ_OC_LIMIT_FREQ_1		GPU_DVFS_FREQ14		/* < 485 MHz */
#define MT_GPUFREQ_OC_LIMIT_FREQ_1_6763T	GPU_DVFS_FREQ1_6763T	/* 530 MHz */
#define MT_GPUFREQ_OC_LIMIT_FREQ_1_6763		GPU_DVFS_FREQ1_6763	/* 530 MHz */
static unsigned int mt_gpufreq_oc_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_oc_limited_index_1;
static unsigned int mt_gpufreq_oc_limited_index;	/* Limited frequency index for oc */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
static unsigned int mt_gpufreq_low_battery_volume;

#define MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1	 GPU_DVFS_FREQ0
static unsigned int mt_gpufreq_low_bat_volume_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volume_limited_index_1;
static unsigned int mt_gpufreq_low_batt_volume_limited_index;	/* Limited frequency index for low battery volume */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
static unsigned int mt_gpufreq_low_battery_level;

#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1		GPU_DVFS_FREQ0	/* no need to throttle when LV1 */
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2		GPU_DVFS_FREQ14	/* 485 MHz */
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2_6763T	GPU_DVFS_FREQ1_6763T	/* 530 MHz */
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2_6763	GPU_DVFS_FREQ1_6763	/* 530 MHz */
static unsigned int mt_gpufreq_low_bat_volt_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volt_limited_index_1;
static unsigned int mt_gpufreq_low_bat_volt_limited_index_2;
static unsigned int mt_gpufreq_low_batt_volt_limited_index;	/* Limited frequency index for low battery voltage */
#endif

static unsigned int mt_gpufreq_thermal_limited_gpu_power;	/* thermal limit power */
static unsigned int mt_gpufreq_prev_thermal_limited_freq;	/* thermal limited freq */
/* limit frequency index array */
static unsigned int mt_gpufreq_power_limited_index_array[NR_IDX_POWER_LIMITED] = { 0 };
bool mt_gpufreq_power_limited_ignore[NR_IDX_POWER_LIMITED] = { false };

/************************************************
 * frequency adjust interface for thermal protect
 *************************************************/
/******************************************************
 * parameter: target power
 *******************************************************/
static int mt_gpufreq_power_throttle_protect(void)
{
	int ret = 0;
	int i = 0;
	unsigned int limited_index = 0;

	/* Check lowest frequency in all limitation */
	for (i = 0; i < NR_IDX_POWER_LIMITED; i++) {
		if (mt_gpufreq_power_limited_index_array[i] != 0 && limited_index == 0)
			limited_index = mt_gpufreq_power_limited_index_array[i];
		else if (mt_gpufreq_power_limited_index_array[i] != 0 && limited_index != 0) {
			if (mt_gpufreq_power_limited_index_array[i] > limited_index)
				limited_index = mt_gpufreq_power_limited_index_array[i];
		}
	}

	g_limited_max_id = limited_index;

	if (g_pGpufreq_power_limit_notify != NULL)
		g_pGpufreq_power_limit_notify(g_limited_max_id);

	return ret;
}

#ifdef MT_GPUFREQ_OC_PROTECT
/************************************************
 * GPU frequency adjust interface for oc protect
 *************************************************/
static void mt_gpufreq_oc_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);

	mt_gpufreq_power_limited_index_array[IDX_OC_LIMITED] = limited_index;
	mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

void mt_gpufreq_oc_callback(BATTERY_OC_LEVEL oc_level)
{
	gpufreq_dbg("@%s: oc_level = %d\n", __func__, oc_level);

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	if (g_limited_oc_ignore_state == true) {
		gpufreq_info("@%s: g_limited_oc_ignore_state == true!\n", __func__);
		return;
	}

	mt_gpufreq_oc_level = oc_level;

	/* BATTERY_OC_LEVEL_1: >= 5.5A	*/
	if (oc_level == BATTERY_OC_LEVEL_1) {
		if (mt_gpufreq_oc_limited_index != mt_gpufreq_oc_limited_index_1) {
			mt_gpufreq_oc_limited_index = mt_gpufreq_oc_limited_index_1;
			mt_gpufreq_oc_protect(mt_gpufreq_oc_limited_index_1);	/* Limit GPU 396.5Mhz */
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_oc_limited_index != mt_gpufreq_oc_limited_index_0) {
			mt_gpufreq_oc_limited_index = mt_gpufreq_oc_limited_index_0;
			mt_gpufreq_oc_protect(mt_gpufreq_oc_limited_index_0);	/* Unlimit */
		}
	}
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
/************************************************
 * GPU frequency adjust interface for low bat_volume protect
 *************************************************/
static void mt_gpufreq_low_batt_volume_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);

	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLUME_LIMITED] = limited_index;
	mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

void mt_gpufreq_low_batt_volume_callback(BATTERY_PERCENT_LEVEL low_battery_volume)
{
	gpufreq_dbg("@%s: low_battery_volume = %d\n", __func__, low_battery_volume);

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	if (g_limited_low_batt_volume_ignore_state == true) {
		gpufreq_info("@%s: g_limited_low_batt_volume_ignore_state == true!\n", __func__);
		return;
	}

	mt_gpufreq_low_battery_volume = low_battery_volume;

	/* LOW_BATTERY_VOLUME_1: <= 15%, LOW_BATTERY_VOLUME_0: >15% */
	if (low_battery_volume == BATTERY_PERCENT_LEVEL_1) {
		if (mt_gpufreq_low_batt_volume_limited_index !=
			mt_gpufreq_low_bat_volume_limited_index_1) {
			mt_gpufreq_low_batt_volume_limited_index =
				mt_gpufreq_low_bat_volume_limited_index_1;

			/* Unlimited */
			mt_gpufreq_low_batt_volume_protect(mt_gpufreq_low_bat_volume_limited_index_1);
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_low_batt_volume_limited_index !=
			mt_gpufreq_low_bat_volume_limited_index_0) {
			mt_gpufreq_low_batt_volume_limited_index =
				mt_gpufreq_low_bat_volume_limited_index_0;
			mt_gpufreq_low_batt_volume_protect(mt_gpufreq_low_bat_volume_limited_index_0);	/* Unlimit */
		}
	}
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
/************************************************
 * GPU frequency adjust interface for low bat_volt protect
 *************************************************/
static void mt_gpufreq_low_batt_volt_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);
	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLT_LIMITED] = limited_index;
	mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

/******************************************************
 * parameter: low_battery_level
 *******************************************************/
void mt_gpufreq_low_batt_volt_callback(LOW_BATTERY_LEVEL low_battery_level)
{
	gpufreq_dbg("@%s: low_battery_level = %d\n", __func__, low_battery_level);

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		return;
	}

	if (g_limited_low_batt_volt_ignore_state == true) {
		gpufreq_info("@%s: g_limited_low_batt_volt_ignore_state == true!\n", __func__);
		return;
	}

	mt_gpufreq_low_battery_level = low_battery_level;

	/* is_low_battery=1:need limit HW, is_low_battery=0:no limit */
	/* 3.25V HW issue int and is_low_battery=1,
	 * 3.0V HW issue int and is_low_battery=2,
	 * 3.5V HW issue int and is_low_battery=0
	 */

	/* no need to throttle when LV1 */
#if 0
	if (low_battery_level == LOW_BATTERY_LEVEL_1) {
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_1) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_1;
			/* Limit GPU 416Mhz */
			mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_1);
		}
	} else
#endif
	if (low_battery_level == LOW_BATTERY_LEVEL_2) {
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_2) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_2;
			/* Limit GPU 400Mhz */
			mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_2);
		}
	} else {		/* unlimit gpu */
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_0) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_0;
			/* Unlimit */
			mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_0);
		}
	}
}
#endif

/************************************************
 * frequency adjust interface for thermal protect
 *************************************************/
/******************************************************
 * parameter: target power
 *******************************************************/
static unsigned int _mt_gpufreq_get_limited_freq(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;
	unsigned int found = 0;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs_power[i].gpufreq_power <= limited_power) {
			limited_freq = mt_gpufreqs_power[i].gpufreq_khz;
			found = 1;
			break;
		}
	}

	/* not found */
	if (!found)
		limited_freq = mt_gpufreqs_power[mt_gpufreqs_num - 1].gpufreq_khz;

	gpufreq_dbg("@%s: limited_freq = %d\n", __func__, limited_freq);

	return limited_freq;
}

void mt_gpufreq_thermal_protect(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (mt_gpufreqs_num == 0) {
		gpufreq_warn("@%s: mt_gpufreqs_num == 0!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (g_limited_thermal_ignore_state == true) {
		gpufreq_info("@%s: g_limited_thermal_ignore_state == true!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	mt_gpufreq_thermal_limited_gpu_power = limited_power;

#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
	mt_update_gpufreqs_power_table();
#endif

	if (limited_power == 0)
		mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED] = 0;
	else {
		limited_freq = _mt_gpufreq_get_limited_freq(limited_power);

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz <= limited_freq) {
				mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED] = i;
				break;
			}
		}
	}

	if (mt_gpufreq_prev_thermal_limited_freq != limited_freq) {
		mt_gpufreq_prev_thermal_limited_freq = limited_freq;
		mt_gpufreq_power_throttle_protect();
		if (limited_freq < GPU_DVFS_FREQ6)
			gpufreq_info("@%s: p %u f %u i %u\n", __func__, limited_power, limited_freq,
				mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED]);
	}

	mutex_unlock(&mt_gpufreq_power_lock);
}
EXPORT_SYMBOL(mt_gpufreq_thermal_protect);

/* for thermal to update power budget */
unsigned int mt_gpufreq_get_max_power(void)
{
	if (!mt_gpufreqs_power)
		return 0;
	else
		return mt_gpufreqs_power[0].gpufreq_power;
}

/* for thermal to update power budget */
unsigned int mt_gpufreq_get_min_power(void)
{
	if (!mt_gpufreqs_power)
		return 0;
	else
		return mt_gpufreqs_power[mt_gpufreqs_num - 1].gpufreq_power;
}

void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power)
{
#ifndef DISABLE_PBM_FEATURE
#ifdef MTK_SSPM
	if (g_limited_pbm_ignore_state == false) {
		if (limited_power == mt_gpufreq_pbm_limited_gpu_power) {
			gpufreq_dbg("@%s: limited_power(%d mW) not changed, skip it!\n",
					__func__, limited_power);
		} else {
			if (!mt_gpufreq_pause) {
				if (mt_gpufreq_debug)
					gpufreq_info("@%s: limited_power(%d mW) changed\n",
						__func__, limited_power);
				mt_gpufreq_pbm_limited_index =
				mt_gpufreq_ap2sspm(IPI_GPU_DVFS_AP_LIMITED_FACTOR,
				LIMIED_FACTOR_PBM, limited_power);
				mt_gpufreq_pbm_limited_gpu_power = limited_power;
			} else {
				if (mt_gpufreq_debug)
					gpufreq_info("@%s: GPU DVFS pause!\n", __func__);
			}
		}
	} else {
		gpufreq_info("@%s: g_limited_pbm_ignore_state == true!\n", __func__);
	}
#else
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		/*gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);*/
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (mt_gpufreqs_num == 0) {
		gpufreq_warn("@%s: mt_gpufreqs_num == 0!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (g_limited_pbm_ignore_state == true) {
		gpufreq_info("@%s: g_limited_pbm_ignorepbm_ignore_state == true!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (limited_power == mt_gpufreq_pbm_limited_gpu_power) {
		gpufreq_dbg("@%s: limited_power(%d mW) not changed, skip it!\n",
				__func__, limited_power);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	mt_gpufreq_pbm_limited_gpu_power = limited_power;

	gpufreq_dbg("@%s: limited_power = %d\n", __func__, limited_power);

#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
	mt_update_gpufreqs_power_table();	/* TODO: need to check overhead? */
#endif

	if (limited_power == 0)
		mt_gpufreq_pbm_limited_index = 0;
	else {
		limited_freq = _mt_gpufreq_get_limited_freq(limited_power);

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz <= limited_freq) {
				mt_gpufreq_pbm_limited_index = i;
				break;
			}
		}
	}

	gpufreq_dbg("PBM limit frequency upper bound to id = %d\n", mt_gpufreq_pbm_limited_index);

	if (g_pGpufreq_power_limit_notify != NULL)
		g_pGpufreq_power_limit_notify(mt_gpufreq_pbm_limited_index);

	mutex_unlock(&mt_gpufreq_power_lock);
#endif
#endif
}

unsigned int mt_gpufreq_get_leakage_mw(void)
{
#ifndef DISABLE_PBM_FEATURE
	int temp = 0;
#ifdef STATIC_PWR_READY2USE
	unsigned int cur_vcore = _mt_gpufreq_get_cur_volt() / 100;
	int leak_power;
#endif

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

#ifdef STATIC_PWR_READY2USE
	leak_power = mt_spower_get_leakage(MT_SPOWER_GPU, cur_vcore, temp);
	if (mt_gpufreq_volt_enable_state && leak_power > 0)
		return leak_power;
	else
		return 0;
#else
	return 130;
#endif

#else /* DISABLE_PBM_FEATURE */
	return 0;
#endif
}

/************************************************
 * return current GPU thermal limit index
 *************************************************/
unsigned int mt_gpufreq_get_thermal_limit_index(void)
{
	gpufreq_dbg("current GPU thermal limit index is %d\n", g_limited_max_id);
	return g_limited_max_id;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_index);

/************************************************
 * return current GPU thermal limit frequency
 *************************************************/
unsigned int mt_gpufreq_get_thermal_limit_freq(void)
{
#ifdef MTK_SSPM
	unsigned int gpufreq_khz;

	gpufreq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, g_limited_max_id, QUERY_FREQ);
	gpufreq_dbg("current GPU thermal limit freq is %d MHz\n", gpufreq_khz / 1000);
	return gpufreq_khz;
#else
	gpufreq_dbg("current GPU thermal limit freq is %d MHz\n",
			mt_gpufreqs[g_limited_max_id].gpufreq_khz / 1000);
	return mt_gpufreqs[g_limited_max_id].gpufreq_khz;
#endif
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_freq);

/************************************************
 * return current GPU frequency index
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq_index(void)
{
#ifdef MTK_SSPM
	g_cur_gpu_OPPidx
		= g_cur_gpu_idx = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_OPPIDX, 0);
#endif
	gpufreq_dbg("current GPU frequency OPP index is %d\n", g_cur_gpu_OPPidx);
	return g_cur_gpu_OPPidx;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/************************************************
 * return current GPU frequency
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq(void)
{
#ifdef MTK_SSPM
	return _mt_gpufreq_get_cur_freq();
#else
	gpufreq_dbg("current GPU frequency is %d MHz\n", g_cur_gpu_freq / 1000);
	return g_cur_gpu_freq;
#endif
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/************************************************
 * return current GPU voltage
 *************************************************/
unsigned int mt_gpufreq_get_cur_volt(void)
{
#if 0
	return g_cur_gpu_volt;
#else
	return _mt_gpufreq_get_cur_volt();
#endif
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_volt);

/************************************************
 * register / unregister GPU input boost notifiction CB
 *************************************************/
void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	g_pGpufreq_input_boost_notify = pCB;
#endif
}
EXPORT_SYMBOL(mt_gpufreq_input_boost_notify_registerCB);

/************************************************
 * register / unregister GPU power limit notifiction CB
 *************************************************/
void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB)
{
	g_pGpufreq_power_limit_notify = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_power_limit_notify_registerCB);

/************************************************
 * register / unregister ptpod update GPU volt CB
 *************************************************/
void mt_gpufreq_update_volt_registerCB(gpufreq_ptpod_update_notify pCB)
{
	g_pGpufreq_ptpod_update_notify = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt_registerCB);

/************************************************
 * register / unregister set GPU freq CB
 *************************************************/
void mt_gpufreq_setfreq_registerCB(sampler_func pCB)
{
	g_pFreqSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setfreq_registerCB);

/************************************************
 * register / unregister set GPU volt CB
 *************************************************/
void mt_gpufreq_setvolt_registerCB(sampler_func pCB)
{
	g_pVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_gpufreq_setvolt_registerCB);

#ifdef CONFIG_HAS_EARLYSUSPEND
/*********************************
 * early suspend callback function
 **********************************/
void mt_gpufreq_early_suspend(struct early_suspend *h)
{
	/* mt_gpufreq_state_set(0); */

}

/*******************************
 * late resume callback function
 ********************************/
void mt_gpufreq_late_resume(struct early_suspend *h)
{
	/* mt_gpufreq_check_freq_and_set_pll(); */

	/* mt_gpufreq_state_set(1); */
}
#endif

static int mt_gpufreq_pm_restore_early(struct device *dev)
{
	int i = 0;
	int found = 0;

	g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (g_cur_gpu_freq == mt_gpufreqs[i].gpufreq_khz) {
			g_cur_gpu_idx = mt_gpufreqs[i].gpufreq_idx;
			g_cur_gpu_volt = mt_gpufreqs[i].gpufreq_volt;
			g_cur_gpu_OPPidx = i;
			found = 1;
			gpufreq_dbg("match g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);
			break;
		}
	}

	if (found == 0) {
		g_cur_gpu_idx = mt_gpufreqs[0].gpufreq_idx;
		g_cur_gpu_volt = mt_gpufreqs[0].gpufreq_volt;
		g_cur_gpu_OPPidx = 0;
		gpufreq_err("gpu freq not found, set parameter to max freq\n");
	}

	gpufreq_dbg("GPU freq SW/HW: %d/%d\n", g_cur_gpu_freq, _mt_gpufreq_get_cur_freq());
	gpufreq_dbg("g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_gpufreq_of_match[] = {
	{.compatible = "mediatek,mt6763-gpufreq",},
	{ /* sentinel */ },
};
#endif
MODULE_DEVICE_TABLE(of, mt_gpufreq_of_match);

#ifdef MTK_SSPM
struct task_struct *g_kt;
struct ipi_action gdvfs_act;
uint32_t gdvfs_rev_buf[3];
/* ipi_action_t ptpod_act; */

static int k_gpu_dvfs_host_recv_thread(void *arg)
{
	while (1) {
		sspm_ipi_recv_wait(oSPEED_DEV_ID_GPU_DVFS);

/*		gpufreq_err("@%s: Recv from sspm %d, %d, %d\n", __func__, gdvfs_rev_buf[0],
*		gdvfs_rev_buf[1], gdvfs_rev_buf[2]);
*/
		switch (gdvfs_rev_buf[0]) {
#ifndef DISABLE_PBM_FEATURE
		case IPI_GPU_DVFS_SSPM_KICK_PBM:
			g_sspm_cur_volt = gdvfs_rev_buf[1];
			g_sspm_power = gdvfs_rev_buf[2];
			_mt_gpufreq_kick_pbm(2);
			break;
#endif
		}

	}
	return 0;
}

static void mt_gpufreq_sspm_recv_daemon_init(void)
{
	int ret = 0;

	gdvfs_act.data = gdvfs_rev_buf;
	ret = sspm_ipi_recv_registration(oSPEED_DEV_ID_GPU_DVFS, &gdvfs_act);

	if (ret) {
		gpufreq_err("@%s: sspm_ipi_recv_registration failed (%d)\n", __func__, ret);
		return;
	}

	g_kt = kthread_create(k_gpu_dvfs_host_recv_thread, &gdvfs_act, "gpu_dvfs_host_recv");

	if (IS_ERR(g_kt)) {
		gpufreq_err("@%s: kthread_create failed (%ld)\n", __func__, PTR_ERR(g_kt));
		return;
	}
	wake_up_process(g_kt);
}
#endif

void mt_gpufreq_set_loading(unsigned int gpu_loading)
{
#ifdef MTK_SSPM
	mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_GPU_LOADING, gpu_loading);
#endif
}

static int mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
	struct device_node *apmixed_node;
	int ret;
	int i = 0;
	int mt_gpufreq_low_bat_volt_limit_freq_2;
	int mt_gpufreq_oc_limit_freq_1;
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
#endif

#if defined(CONFIG_OF) && !defined(MTK_SSPM)
	struct device_node *node;

	node = of_find_matching_node(NULL, mt_gpufreq_of_match);
	if (!node)
		gpufreq_err("@%s: find GPU node failed\n", __func__);

	mt_gpufreq_clk = kzalloc(sizeof(struct mt_gpufreq_clk_t), GFP_KERNEL);
	if (mt_gpufreq_clk == NULL)
		return -ENOMEM;

	mt_gpufreq_clk->clk_mux = devm_clk_get(&pdev->dev, "clk_mux");
	if (IS_ERR(mt_gpufreq_clk->clk_mux)) {
		dev_err(&pdev->dev, "cannot get clock mux\n");
		return PTR_ERR(mt_gpufreq_clk->clk_mux);
	}

	mt_gpufreq_clk->clk_main_parent = devm_clk_get(&pdev->dev, "clk_main_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_main_parent)) {
		dev_err(&pdev->dev, "cannot get main clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_main_parent);
	}
	mt_gpufreq_clk->clk_sub_parent = devm_clk_get(&pdev->dev, "clk_sub_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_sub_parent)) {
		dev_err(&pdev->dev, "cannot get sub clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_sub_parent);
	}

	/* alloc PMIC regulator */
	if (device_id == C_MT6763TT) {
		mt_gpufreq_pmic = kzalloc(sizeof(struct mt_gpufreq_pmic_t), GFP_KERNEL);
		if (mt_gpufreq_pmic == NULL)
			return -ENOMEM;

		mt_gpufreq_pmic->reg_vproc = regulator_get(&pdev->dev, "vproc");
		if (IS_ERR(mt_gpufreq_pmic->reg_vproc)) {
			dev_err(&pdev->dev, "cannot get vproc\n");
			return PTR_ERR(mt_gpufreq_pmic->reg_vproc);
		}

		mt_gpufreq_pmic->reg_vsram = regulator_get(&pdev->dev, "vsram_gpu");
		if (IS_ERR(mt_gpufreq_pmic->reg_vsram)) {
			dev_err(&pdev->dev, "cannot get vsram_gpu\n");
			return PTR_ERR(mt_gpufreq_pmic->reg_vsram);
		}
	} else {
		mt_gpufreq_pmic = kzalloc(sizeof(struct mt_gpufreq_pmic_t), GFP_KERNEL);
		mt_gpufreq_pmic->reg_vcore = regulator_get(&pdev->dev, "vcore");
		if (IS_ERR(mt_gpufreq_pmic->reg_vcore)) {
			dev_err(&pdev->dev, "cannot get vcore\n");
			return PTR_ERR(mt_gpufreq_pmic->reg_vcore);
		}
	}

#endif

	if (is_ext_buck_exist()) {
		gpufreq_info("@%s: I am 6763TT (%x)\n", __func__, get_devinfo_with_index(30));
		device_id = C_MT6763TT;
	} else if (get_devinfo_with_index(30) == 0x10) {
		gpufreq_info("@%s: I am 6763 (%x)\n", __func__, get_devinfo_with_index(30));
		device_id = C_MT6763;
	} else if (get_devinfo_with_index(30) == 0x0 || get_devinfo_with_index(30) == 0x20) {
		gpufreq_info("@%s: I am 6763T (%x)\n", __func__, get_devinfo_with_index(30));
		device_id = C_MT6763T;
	} else {
		gpufreq_err("@%s: Wrong Divice ID (%x)\n", __func__, get_devinfo_with_index(30));
	}

	mt_gpufreq_dvfs_table_type = device_id;

#ifdef CONFIG_HAS_EARLYSUSPEND
	mt_gpufreq_early_suspend_handler.suspend = mt_gpufreq_early_suspend;
	mt_gpufreq_early_suspend_handler.resume = mt_gpufreq_late_resume;
	register_early_suspend(&mt_gpufreq_early_suspend_handler);
#endif


	/**********************
	 * Initial leackage power usage
	 ***********************/
#ifdef STATIC_PWR_READY2USE
	mt_spower_init();
#endif

	/**********************
	 * Initial SRAM debugging ptr
	 ***********************/
#ifdef MT_GPUFREQ_AEE_RR_REC
	_mt_gpufreq_aee_init();
#endif

	/**********************
	 * setup gpufreq table
	 ***********************/
#ifndef MTK_SSPM
	gpufreq_info("setup gpufreqs table\n");

	if (mt_gpufreq_dvfs_table_type == C_MT6763TT)
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
	else if (mt_gpufreq_dvfs_table_type == C_MT6763T)
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_1,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_1));
	else
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_2,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_2));
	gpufreq_dbg("[Figo] OPP Table Set Up Done (%d)", mt_gpufreq_dvfs_table_type);

	/**********************
	 * setup PMIC init value
	 ***********************/
#ifdef VPROC_SET_BY_PMIC
	if (device_id == C_MT6763TT) {
		g_vproc_sfchg_rrate = _calculate_vproc_sfchg_rate(true); /* rising rate */
		g_vproc_sfchg_frate = _calculate_vproc_sfchg_rate(false); /* falling rate */
		g_vsram_sfchg_rrate = _calculate_vsram_sfchg_rate(true); /* rising rate */
		g_vsram_sfchg_frate = _calculate_vsram_sfchg_rate(false); /* falling rate */

		/* VSRAM */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vsram,
			PMIC_MAX_VSRAM*10, PMIC_MAX_VSRAM*10 + 125);

		/* VPROC */
		regulator_set_voltage(mt_gpufreq_pmic->reg_vproc,
			PMIC_MAX_VPROC*10, PMIC_MAX_VPROC*10 + 125);

		/* Enable VSRAM */
		if (!regulator_is_enabled(mt_gpufreq_pmic->reg_vsram)) {
			ret = regulator_enable(mt_gpufreq_pmic->reg_vsram);
			if (ret) {
				gpufreq_err("Enable VSRAM failed! (%d)", ret);
				return ret;
			}
			udelay(VSRAM_GPU_ENABLE_TIME_US);
		} else {
			ret = regulator_enable(mt_gpufreq_pmic->reg_vsram);
			if (ret)
				gpufreq_err("Enable VSRAM failed! (%d)", ret);
		}

		/* Enable VPROC */
		if (!regulator_is_enabled(mt_gpufreq_pmic->reg_vproc)) {
			ret = regulator_enable(mt_gpufreq_pmic->reg_vproc);
			if (ret) {
				gpufreq_err("Enable VPROC failed! (%d)", ret);
				return ret;
			}
			udelay(VPROC_ENABLE_TIME_US);
		} else {
			ret = regulator_enable(mt_gpufreq_pmic->reg_vproc);
			if (ret)
				gpufreq_err("Enable VPROC failed! (%d)", ret);
		}

#ifdef MT_GPUFREQ_AEE_RR_REC
		aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_VPROC_ENABLED));
#endif

		gpufreq_info("VSRAM Enabled (%d) %d mV\n",
			regulator_is_enabled(mt_gpufreq_pmic->reg_vsram),
			regulator_get_voltage(mt_gpufreq_pmic->reg_vsram)/1000);
		gpufreq_info("VPROC Enabled (%d) %d mV\n",
			regulator_is_enabled(mt_gpufreq_pmic->reg_vproc),
			regulator_get_voltage(mt_gpufreq_pmic->reg_vproc)/1000);
	}
#endif
	gpufreq_dbg("[Figo] Skip regulator init");
	mt_gpufreq_volt_enable_state = 1;
	mt_gpufreq_volt_lpm_state = 0;
	gpufreq_dbg("[Figo] power init done");

	/* init PLL
	 *	to get PLL register
	 */

	/* Init APMIXED base address */
	apmixed_node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	g_apmixed_base = of_iomap(apmixed_node, 0);
	if (!g_apmixed_base) {
		gpufreq_err("Error, APMIXED iomap failed");
		return -ENOENT;
	}
#else
	MTK_UNREFERENCED_PARAMETER(apmixed_node);
	MTK_UNREFERENCED_PARAMETER(ret);
	MTK_UNREFERENCED_PARAMETER(g_apmixed_base);
#endif /* !MTK_SSPM */
	g_cur_freq_init_keep = g_cur_gpu_freq;

	/**********************
	 * setup initial frequency
	 ***********************/
	gpufreq_dbg("[Figo] setup initial frequency");
#ifndef MTK_SSPM
	mt_gpufreq_set_initial();
#endif

	gpufreq_info("GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
	gpufreq_info("Current Vproc = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
	gpufreq_info("g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", g_cur_gpu_freq, g_cur_gpu_volt);
	gpufreq_info("g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n", g_cur_gpu_idx,
			 g_cur_gpu_OPPidx);

	mt_gpufreq_ready = true;


#ifdef MT_GPUFREQ_INPUT_BOOST
	mt_gpufreq_up_task =
		kthread_create(mt_gpufreq_input_boost_task, NULL, "mt_gpufreq_input_boost_task");
	if (IS_ERR(mt_gpufreq_up_task))
		return PTR_ERR(mt_gpufreq_up_task);

	sched_setscheduler_nocheck(mt_gpufreq_up_task, SCHED_FIFO, &param);
	get_task_struct(mt_gpufreq_up_task);

	rc = input_register_handler(&mt_gpufreq_input_handler);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1) {
			mt_gpufreq_low_bat_volt_limited_index_1 = i;
			break;
		}
	}

	if (device_id == C_MT6763TT) {
		mt_gpufreq_low_bat_volt_limit_freq_2 =
			MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2;
	} else if (device_id == C_MT6763T) {
		mt_gpufreq_low_bat_volt_limit_freq_2 =
			MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2_6763T;
	} else {
		mt_gpufreq_low_bat_volt_limit_freq_2 =
			MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2_6763;
	}

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz == mt_gpufreq_low_bat_volt_limit_freq_2) {
			mt_gpufreq_low_bat_volt_limited_index_2 = i;
			break;
		}
	}

	register_low_battery_notify(&mt_gpufreq_low_batt_volt_callback, LOW_BATTERY_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1) {
			mt_gpufreq_low_bat_volume_limited_index_1 = i;
			break;
		}
	}

	register_battery_percent_notify(&mt_gpufreq_low_batt_volume_callback,
					BATTERY_PERCENT_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_OC_PROTECT
	if (device_id == C_MT6763TT) {
		mt_gpufreq_oc_limit_freq_1 = MT_GPUFREQ_OC_LIMIT_FREQ_1;
	} else if (device_id == C_MT6763T) {
		mt_gpufreq_oc_limit_freq_1 = MT_GPUFREQ_OC_LIMIT_FREQ_1_6763T;
	} else {
		mt_gpufreq_oc_limit_freq_1 = MT_GPUFREQ_OC_LIMIT_FREQ_1_6763;
	}

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz == mt_gpufreq_oc_limit_freq_1) {
			mt_gpufreq_oc_limited_index_1 = i;
			break;
		}
	}

	register_battery_oc_notify(&mt_gpufreq_oc_callback, BATTERY_OC_PRIO_GPU);
#endif

#ifndef MTK_SSPM
#ifndef DISABLE_PBM_FEATURE
	INIT_DEFERRABLE_WORK(&notify_pbm_gpuoff_work, mt_gpufreq_notify_pbm_gpuoff);
#endif
#endif

	return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int mt_gpufreq_pdrv_remove(struct platform_device *pdev)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	input_unregister_handler(&mt_gpufreq_input_handler);

	kthread_stop(mt_gpufreq_up_task);
	put_task_struct(mt_gpufreq_up_task);
#endif

	return 0;
}

static const struct dev_pm_ops mt_gpufreq_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.restore_early = mt_gpufreq_pm_restore_early,
};
#if 0
struct platform_device mt_gpufreq_pdev = {
	.name = "mt-gpufreq",
	.id = -1,
};
#endif
static struct platform_driver mt_gpufreq_pdrv = {
	.probe = mt_gpufreq_pdrv_probe,
	.remove = mt_gpufreq_pdrv_remove,
	.driver = {
		   .name = "gpufreq",
		   .pm = &mt_gpufreq_pm_ops,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_gpufreq_of_match,
#endif
		   },
};


#ifdef CONFIG_PROC_FS
/* #if 0 */
/*
 * PROC
 */

/***************************
 * show current debug status
 ****************************/
static int mt_gpufreq_debug_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_debug)
		seq_puts(m, "gpufreq debug enabled\n");
	else
		seq_puts(m, "gpufreq debug disabled\n");

	return 0;
}

/***********************
 * enable debug message
 ************************/
static ssize_t mt_gpufreq_debug_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &debug) == 0) {
		if (debug == 0)
			mt_gpufreq_debug = 0;
		else if (debug == 1)
			mt_gpufreq_debug = 1;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

	return count;
}

#ifdef MT_GPUFREQ_OC_PROTECT
/****************************
 * show current limited by low batt volume
 *****************************/
static int mt_gpufreq_limited_oc_ignore_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	seq_printf(m, "oc_limited_id = %d, g_limited_oc_ignore_state = %d\n",
		   mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_LMITED_FACTOR, IDX_OC_LIMITED),
		   g_limited_oc_ignore_state);
#else
	seq_printf(m, "g_limited_max_id = %d, g_limited_oc_ignore_state = %d\n", g_limited_max_id,
		   g_limited_oc_ignore_state);
#endif
	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_oc_ignore_proc_write(struct file *file,
							   const char __user *buffer, size_t count,
							   loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1) {
			g_limited_oc_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_OC_LIMITED << 1 | 0x1));
		} else if (ignore == 0) {
			g_limited_oc_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_OC_LIMITED << 1));
		} else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#else
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_oc_ignore_state = true;
		else if (ignore == 0)
			g_limited_oc_ignore_state = false;
		else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#endif
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	mt_gpufreq_power_limited_ignore[IDX_OC_LIMITED] = g_limited_oc_ignore_state;
	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
/****************************
 * show current limited by low batt volume
 *****************************/
static int mt_gpufreq_limited_low_batt_volume_ignore_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	seq_printf(m, "low_batt_volume_limited_id = %d, g_limited_low_batt_volume_ignore_state = %d\n",
		   mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_LMITED_FACTOR, IDX_LOW_BATT_VOLUME_LIMITED),
		   g_limited_low_batt_volume_ignore_state);
#else
	seq_printf(m, "g_limited_max_id = %d, g_limited_low_batt_volume_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volume_ignore_state);
#endif
	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_low_batt_volume_ignore_proc_write(struct file *file,
									const char __user *buffer,
									size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1) {
			g_limited_low_batt_volume_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_LOW_BATT_VOLUME_LIMITED << 1 | 0x1));
		} else if (ignore == 0) {
			g_limited_low_batt_volume_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_LOW_BATT_VOLUME_LIMITED << 1));
		} else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#else
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_low_batt_volume_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volume_ignore_state = false;
		else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#endif
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	mt_gpufreq_power_limited_ignore[IDX_LOW_BATT_VOLUME_LIMITED] = g_limited_low_batt_volume_ignore_state;
	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
/****************************
 * show current limited by low batt volt
 *****************************/
static int mt_gpufreq_limited_low_batt_volt_ignore_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	seq_printf(m, "low_batt_volt_limited_id = %d, g_limited_low_batt_volt_ignore_state = %d\n",
		   mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_LMITED_FACTOR, IDX_LOW_BATT_VOLT_LIMITED),
		   g_limited_low_batt_volt_ignore_state);
#else
	seq_printf(m, "g_limited_max_id = %d, g_limited_low_batt_volt_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volt_ignore_state);
#endif

	return 0;
}

/**********************************
 * limited for low batt volt protect
 ***********************************/
static ssize_t mt_gpufreq_limited_low_batt_volt_ignore_proc_write(struct file *file,
								  const char __user *buffer,
								  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1) {
			g_limited_low_batt_volt_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_LOW_BATT_VOLT_LIMITED << 1 | 0x1));
		} else if (ignore == 0) {
			g_limited_low_batt_volt_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_LOW_BATT_VOLT_LIMITED << 1));
		} else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#else
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_low_batt_volt_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volt_ignore_state = false;
		else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#endif
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	mt_gpufreq_power_limited_ignore[IDX_LOW_BATT_VOLT_LIMITED] = g_limited_low_batt_volt_ignore_state;
	return count;
}
#endif

/****************************
 * show current limited by thermal
 *****************************/
static int mt_gpufreq_limited_thermal_ignore_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	seq_printf(m, "thermal_limited_id = %d, g_limited_thermal_ignore_state = %d\n",
		   mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_LMITED_FACTOR, IDX_THERMAL_LIMITED),
		   g_limited_thermal_ignore_state);
#else
	seq_printf(m, "g_limited_max_id = %d, g_limited_thermal_ignore_state = %d\n",
		   g_limited_max_id, g_limited_thermal_ignore_state);
#endif

	return 0;
}

/**********************************
 * limited for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_thermal_ignore_proc_write(struct file *file,
								const char __user *buffer,
								size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1) {
			g_limited_thermal_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_IGNORE_LMITED_FACTOR,
			(IDX_THERMAL_LIMITED << 1 | 0x1));
		} else if (ignore == 0) {
			g_limited_thermal_ignore_state =
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP,
			SET_IGNORE_LMITED_FACTOR, (IDX_THERMAL_LIMITED << 1));
		} else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#else
	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_thermal_ignore_state = true;
		else if (ignore == 0)
			g_limited_thermal_ignore_state = false;
		else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
#endif
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	mt_gpufreq_power_limited_ignore[IDX_THERMAL_LIMITED] = g_limited_thermal_ignore_state;
	return count;
}

#ifndef DISABLE_PBM_FEATURE
/****************************
 * show current limited by PBM
 *****************************/
static int mt_gpufreq_limited_pbm_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "mt_gpufreq_pbm_limited_index = %d, g_limited_pbm_ignore_state = %d\n",
		mt_gpufreq_pbm_limited_index, g_limited_pbm_ignore_state);

	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_pbm_ignore_proc_write(struct file *file,
							const char __user *buffer, size_t count,
							loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1) {
#ifdef MTK_SSPM
			/* Set as free power: 0*/
			mt_gpufreq_pbm_limited_index =
				mt_gpufreq_ap2sspm(IPI_GPU_DVFS_AP_LIMITED_FACTOR, LIMIED_FACTOR_PBM, 0);
			mt_gpufreq_pbm_limited_gpu_power = 0;
#endif
			g_limited_pbm_ignore_state = true;
		} else if (ignore == 0)
			g_limited_pbm_ignore_state = false;
		else
			gpufreq_warn
				("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}
#endif

/****************************
 * show current limited power
 *****************************/
static int mt_gpufreq_limited_power_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	unsigned int gpufreq_khz;

	gpufreq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, g_limited_max_id, QUERY_FREQ);

	seq_printf(m, "g_limited_max_id = %d, limit frequency = %d\n",
		   g_limited_max_id, gpufreq_khz);
#else
	seq_printf(m, "g_limited_max_id = %d, limit frequency = %d\n",
		   g_limited_max_id, mt_gpufreqs[g_limited_max_id].gpufreq_khz);
#endif
	return 0;
}

/**********************************
 * limited power for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_power_proc_write(struct file *file,
						   const char __user *buffer,
						   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int power = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &power) == 0)
#ifdef MTK_SSPM
		g_limited_max_id = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_THROTTLE_POWER, power);
#else
		mt_gpufreq_thermal_protect(power);
#endif
	else
		gpufreq_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}

/****************************
 * show current limited power by PBM
 *****************************/
#ifndef DISABLE_PBM_FEATURE
static int mt_gpufreq_limited_by_pbm_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "pbm_limited_power = %d, limit index = %d\n",
		   mt_gpufreq_pbm_limited_gpu_power, mt_gpufreq_pbm_limited_index);

	return 0;
}

/**********************************
 * limited power for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_by_pbm_proc_write(struct file *file, const char __user *buffer,
							size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int power = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &power) == 0)
		mt_gpufreq_set_power_limit_by_pbm(power);
	else
		gpufreq_warn("bad argument!! please provide the maximum limited power\n");

	return count;
}
#endif

/******************************
 * show current GPU DVFS stauts
 *******************************/
static int mt_gpufreq_state_proc_show(struct seq_file *m, void *v)
{
	if (!mt_gpufreq_pause)
		seq_puts(m, "GPU DVFS enabled\n");
	else
		seq_puts(m, "GPU DVFS disabled\n");

	return 0;
}

/****************************************
 * set GPU DVFS stauts by sysfs interface
 *****************************************/
static ssize_t mt_gpufreq_state_proc_write(struct file *file,
					   const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int enabled = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM
	if (kstrtoint(desc, 0, &enabled) == 0) {
		if (enabled == 1) {
			len = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, GPU_DVFS_MAX_FREQ, SUB_CMD_REL_FREQ);
		} else if (enabled == 0) {
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, 1);
			/*
			 *	release locked-freq before lock down
			 */
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, GPU_DVFS_MAX_FREQ, SUB_CMD_REL_FREQ);
			len = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, GPU_DVFS_MAX_FREQ, SUB_CMD_FIX_FREQ);
		} else {
			gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");
			return count;
		}

		if (len < 0)
			return count;

		mt_gpufreq_pause = (enabled) ? 0 : 1;
#else
	if (kstrtoint(desc, 0, &enabled) == 0) {
		if (enabled == 1) {
			mt_gpufreq_keep_max_frequency_state = false;
			mt_gpufreq_state_set(1);
		} else if (enabled == 0) {
			/* Keep MAX frequency when GPU DVFS disabled. */
			mt_gpufreq_keep_max_frequency_state = true;
#ifdef VOLT_SET_READY
			gpufreq_dbg("[Figo] Power on by mt_gpufreq_state_proc_write\n");
			mt_gpufreq_voltage_enable_set(1);
#endif
			mt_gpufreq_target(g_gpufreq_max_id);
			mt_gpufreq_state_set(0);
		} else
			gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");
#endif
	} else
		gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");

	return count;
}

/********************
 * show GPU OPP table
 *********************/
static int mt_gpufreq_opp_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
#ifdef MTK_SSPM
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;

	for (i = 0; i < mt_gpufreqs_num; i++) {

		gpufreq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_FREQ);
		gpufreq_volt = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_VOLT);

		seq_printf(m, "[%d] ", i);
		seq_printf(m, "freq = %d, ", gpufreq_khz);
		seq_printf(m, "volt = %d, ", gpufreq_volt);
		seq_printf(m, "idx = %d\n", i);
	}
#else
	for (i = 0; i < mt_gpufreqs_num; i++) {
		seq_printf(m, "[%d] ", i);
		seq_printf(m, "freq = %d, ", mt_gpufreqs[i].gpufreq_khz);
		seq_printf(m, "volt = %d, ", mt_gpufreqs[i].gpufreq_volt);
		seq_printf(m, "idx = %d\n", mt_gpufreqs[i].gpufreq_idx);
	}
#endif

	return 0;
}

/********************
 * show GPU power table
 *********************/
static int mt_gpufreq_power_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
#ifdef MTK_SSPM
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_power;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		gpufreq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_FREQ);
		gpufreq_volt = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_VOLT);
		gpufreq_power = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_POWER);

		seq_printf(m, "[%d] ", i);
		seq_printf(m, "freq = %d, ", gpufreq_khz);
		seq_printf(m, "volt = %d, ", gpufreq_volt);
		seq_printf(m, "power = %d\n", gpufreq_power);
	}
#else
	for (i = 0; i < mt_gpufreqs_num; i++) {
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_khz);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_volt);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_power);
	}
#endif
	return 0;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_keep_opp_frequency_state) {
		seq_puts(m, "gpufreq keep opp frequency enabled\n");
		seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq keep opp frequency disabled\n");
	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_freq_proc_write(struct file *file, const char __user *buffer,
						  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int fixed_freq = 0;
#ifndef MTK_SSPM
	int i = 0;
	int found = 0;
#endif

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';
#ifdef MTK_SSPM
	if (kstrtoint(desc, 0, &fixed_freq) == 0) {
		if (fixed_freq == 0) {
			mt_gpufreq_keep_opp_frequency_state = false;
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, GPU_DVFS_MAX_FREQ, SUB_CMD_REL_FREQ);
				mt_gpufreq_pause = 0;
		} else {
			mt_gpufreq_keep_opp_frequency_state = true;
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, 1);
			/*
			 *	release locked-freq before lock down
			 */
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, fixed_freq, SUB_CMD_REL_FREQ);
			if (mt_gpufreq_ap2sspm(IPI_GPU_DVFS_SET_FREQ, fixed_freq, SUB_CMD_FIX_FREQ) == 0) {
				mt_gpufreq_pause = 1;
				mt_gpufreq_keep_opp_index = mt_gpufreq_get_idx_by_target_freq(fixed_freq);
			}
		}
	} else
#else
	if (kstrtoint(desc, 0, &fixed_freq) == 0) {
		if (fixed_freq == 0) {
			mt_gpufreq_keep_opp_frequency_state = false;
		} else {
			for (i = 0; i < mt_gpufreqs_num; i++) {
				if (fixed_freq == mt_gpufreqs[i].gpufreq_khz) {
					mt_gpufreq_keep_opp_index = i;
					found = 1;
					break;
				}
			}

			if (found == 1) {
				mt_gpufreq_keep_opp_frequency_state = true;
				mt_gpufreq_keep_opp_frequency = fixed_freq;

#ifdef VOLT_SET_READY
			gpufreq_dbg("[Figo] Power on by mt_gpufreq_opp_freq_proc_write\n");
			mt_gpufreq_voltage_enable_set(1);
#endif
			mt_gpufreq_target(mt_gpufreq_keep_opp_index);
			}

		}
	} else
#endif	 /* MTK_SSPM */
		gpufreq_warn("bad argument!! please provide the fixed frequency\n");

	return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_max_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_opp_max_frequency_state) {
		seq_puts(m, "gpufreq opp max frequency enabled\n");
		seq_printf(m, "freq = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n", mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq opp max frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_max_freq_proc_write(struct file *file, const char __user *buffer,
						  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int max_freq = 0;
#ifndef MTK_SSPM
	int i = 0;
	int found = 0;
#endif

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM

	if (kstrtoint(desc, 0, &max_freq) == 0) {
		if (max_freq == 0) {
			mt_gpufreq_opp_max_frequency_state = false;
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_UPBOUND_FREQ, GPU_DVFS_MAX_FREQ);
		} else {
			mt_gpufreq_opp_max_frequency_state = true;
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, 1);
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_UPBOUND_FREQ, max_freq);
		}
	} else
#else
	if (kstrtoint(desc, 0, &max_freq) == 0) {
		if (max_freq == 0) {
			mt_gpufreq_opp_max_frequency_state = false;
		} else {
			for (i = 0; i < mt_gpufreqs_num; i++) {
				if (mt_gpufreqs[i].gpufreq_khz <= max_freq) {
					mt_gpufreq_opp_max_index = i;
					found = 1;
					break;
				}
			}

			if (found == 1) {
				mt_gpufreq_opp_max_frequency_state = true;
				mt_gpufreq_opp_max_frequency =
					mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
#ifdef VOLT_SET_READY
				gpufreq_dbg("[Figo] Power on by mt_gpufreq_opp_max_freq_proc_write\n");
				mt_gpufreq_voltage_enable_set(1);
#endif
				mt_gpufreq_target(mt_gpufreq_opp_max_index);
			}
		}
	} else
#endif
		gpufreq_warn("bad argument!! please provide the maximum limited frequency\n");

	return count;
}

/********************
 * show variable dump
 *********************/
static int mt_gpufreq_var_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
#ifdef MTK_SSPM
	unsigned int limited_id;

	seq_puts(m, "DVFS_GPU [SSPM]\n");
#else
	seq_puts(m, "DVFS_GPU [legacy]\n");
#endif

#ifdef MTK_SSPM
	g_cur_gpu_idx = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_OPPIDX, 0);
	seq_printf(m, "g_cur_gpu_idx = %d\n", g_cur_gpu_idx);

	/* This print current freq/volt according to oppindx on opptab */
	seq_printf(m, "g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n",
		mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, g_cur_gpu_idx, QUERY_FREQ),
		mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, g_cur_gpu_idx, QUERY_VOLT));

	g_limited_max_id = 0;
	for (i = 0; i < NR_IDX_POWER_LIMITED; i++) {
		limited_id = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_LMITED_FACTOR, i);
		mt_gpufreq_power_limited_index_array[i] = limited_id;

		if (limited_id > g_limited_max_id) {
			if (mt_gpufreq_power_limited_ignore[i] == false)
			g_limited_max_id = limited_id;
		}

		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d %s\n", i,
			   mt_gpufreq_power_limited_index_array[i], mt_gpufreq_power_limited_ignore[i]?"(ignore)":"");
	}
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);
	/*seq_printf(m, "mt_gpufreq_pbm_limited_index = %u\n", mt_gpufreq_pbm_limited_index);*/

	/* This print real freq from PLL */
	 seq_printf(m, "_mt_gpufreq_get_cur_freq = %d\n",
		 mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_FREQ, 0));
	/* This print real volt from PMIC */
	 seq_printf(m, "_mt_gpufreq_get_cur_volt = %d\n",
		 mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_CUR_VOLT, 0));

	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n",
		 mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_VOLT_ENABLED, 0));

	seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n", mt_gpufreq_dvfs_table_type);

	/* FIX-ME:
	*	seq_printf(m, "mt_gpufreq_ptpod_disable_idx = %d\n", mt_gpufreq_ptpod_disable_idx);
	*/
	/*seq_printf(m, "mt_gpufreq_pbm_limited_gpu_power = %u\n", mt_gpufreq_pbm_limited_gpu_power);*/
#else
	seq_printf(m, "g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", mt_gpufreq_get_cur_freq(),
		   mt_gpufreq_get_cur_volt());
	seq_printf(m, "g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n", g_cur_gpu_idx,
		   g_cur_gpu_OPPidx);
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);

	for (i = 0; i < NR_IDX_POWER_LIMITED; i++)
		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			   mt_gpufreq_power_limited_index_array[i]);

	seq_printf(m, "_mt_gpufreq_get_cur_freq = %d, mt_get_ckgen_freq = %d KHz\n",
		_mt_gpufreq_get_cur_freq(), mt_get_ckgen_freq(7));
	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n", mt_gpufreq_volt_enable_state);
	seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n", mt_gpufreq_dvfs_table_type);
	seq_printf(m, "mt_gpufreq_ptpod_disable_idx = %d\n", mt_gpufreq_ptpod_disable_idx);
#endif

	return 0;
}

/***************************
 * show current voltage enable status
 ****************************/
static int mt_gpufreq_volt_enable_proc_show(struct seq_file *m, void *v)
{
#ifdef MTK_SSPM
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_volt_enable);
	if (mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, QUERY_VOLT_ENABLED, 0))
#else
	if (mt_gpufreq_volt_enable)
#endif
		seq_puts(m, "gpufreq voltage enabled\n");
	else
		seq_puts(m, "gpufreq voltage disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_volt_enable_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int enable = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MTK_SSPM
	if (kstrtoint(desc, 0, &enable) == 0) {
		if (enable == 0)
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, 0);
		else if (enable == 1)
			mt_gpufreq_ap2sspm(IPI_GPU_DVFS_STATUS_OP, SET_VOLT_SWITCH, 1);
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
#else
	if (kstrtoint(desc, 0, &enable) == 0) {
		if (enable == 0) {
#ifdef VOLT_SET_READY
			gpufreq_dbg("[Figo] Power off by mt_gpufreq_state_proc_write\n");
			mt_gpufreq_voltage_enable_set(0);
#endif
			mt_gpufreq_volt_enable = false;
		} else if (enable == 1) {
#ifdef VOLT_SET_READY
			gpufreq_dbg("[Figo] Power on by mt_gpufreq_state_proc_write\n");
			mt_gpufreq_voltage_enable_set(1);
#endif
			mt_gpufreq_volt_enable = true;
		} else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
#endif
	return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_fixed_freq_volt_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_fixed_freq_volt_state) {
		seq_puts(m, "gpufreq fixed frequency enabled\n");
		seq_printf(m, "fixed frequency = %d\n", mt_gpufreq_fixed_frequency);
		seq_printf(m, "fixed voltage = %d\n", mt_gpufreq_fixed_voltage);
	} else
		seq_puts(m, "gpufreq fixed frequency disabled\n");
	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static void _mt_gpufreq_fixed_freq(int fixed_freq)
{
	unsigned int last_freq_level = 0;

	if (device_id == C_MT6763TT)
		last_freq_level = GPUFREQ_LAST_FREQ_LEVEL;
	else if (device_id == C_MT6763T)
		last_freq_level = GPUFREQ_LAST_FREQ_LEVEL_6763T;
	else if (device_id == C_MT6763)
		last_freq_level = GPUFREQ_LAST_FREQ_LEVEL_6763;

	/* freq (KHz) */
	if ((fixed_freq >= GPUFREQ_LAST_FREQ_LEVEL)
		&& (fixed_freq <= GPU_DVFS_FREQ0)) {
		gpufreq_dbg("@ %s, mt_gpufreq_clock_switch1 fix frq = %d, fix volt = %d, volt = %d\n",
			__func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_fixed_freq_volt_state = true;
		mt_gpufreq_fixed_frequency = fixed_freq;
		mt_gpufreq_fixed_voltage = g_cur_gpu_volt;
#ifdef VOLT_SET_READY
		gpufreq_dbg("[Figo] Power on by _mt_gpufreq_fixed_freq\n");
		mt_gpufreq_voltage_enable_set(1);
#endif
		gpufreq_dbg("@ %s, mt_gpufreq_clock_switch2 fix frq = %d, fix volt = %d, volt = %d\n",
			__func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_clock_switch(mt_gpufreq_fixed_frequency);
		g_cur_gpu_freq = mt_gpufreq_fixed_frequency;
	}
}

static void _mt_gpufreq_fixed_volt(int fixed_volt)
{
	unsigned int max_volt = 0;
	unsigned int min_volt = 0;


	if (device_id == C_MT6763TT) {
		max_volt = PMIC_MAX_VPROC;
		min_volt = PMIC_MIN_VPROC;
	} else if (device_id == C_MT6763T) {
		max_volt = PMIC_MAX_VCORE_6763T;
		min_volt = PMIC_MIN_VCORE_6763T;
	} else if (device_id == C_MT6763) {
		max_volt = PMIC_MAX_VCORE_6763;
		min_volt = PMIC_MIN_VCORE_6763;
	}

	/* volt (mV) */
#ifdef VPROC_SET_BY_PMIC
	if (fixed_volt >= (min_volt / 100) &&
		fixed_volt <= (max_volt / 100)) {
#endif
		gpufreq_dbg("@ %s, mt_gpufreq_volt_switch1 fix frq = %d, fix volt = %d, volt = %d\n",
			__func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_fixed_freq_volt_state = true;
		mt_gpufreq_fixed_frequency = g_cur_gpu_freq;
		mt_gpufreq_fixed_voltage = fixed_volt * 100;
#ifdef VOLT_SET_READY
		gpufreq_dbg("[Figo] Power on by _mt_gpufreq_fixed_volt\n");
		mt_gpufreq_voltage_enable_set(1);
#endif
		gpufreq_dbg("@ %s, mt_gpufreq_volt_switch2 fix frq = %d, fix volt = %d, volt = %d\n",
			__func__, mt_gpufreq_fixed_frequency, mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		if (device_id == C_MT6763TT)
			mt_gpufreq_volt_switch(g_cur_gpu_volt, mt_gpufreq_fixed_voltage);
		else
			mt_gpufreq_volt_switch_vcore(g_cur_gpu_volt, mt_gpufreq_fixed_voltage);
		g_cur_gpu_volt = mt_gpufreq_fixed_voltage;
	}
}

static ssize_t mt_gpufreq_fixed_freq_volt_proc_write(struct file *file, const char __user *buffer,
							 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int fixed_freq = 0;
	int fixed_volt = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf(desc, "%d %d", &fixed_freq, &fixed_volt) == 2) {
		if ((fixed_freq == 0) && (fixed_volt == 0)) {
			mt_gpufreq_fixed_freq_volt_state = false;
			mt_gpufreq_fixed_frequency = 0;
			mt_gpufreq_fixed_voltage = 0;
		} else {
			g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();
			if (fixed_freq > g_cur_gpu_freq) {
				_mt_gpufreq_fixed_volt(fixed_volt);
				_mt_gpufreq_fixed_freq(fixed_freq);
			} else {
				_mt_gpufreq_fixed_freq(fixed_freq);
				_mt_gpufreq_fixed_volt(fixed_volt);
			}
		}
	} else
		gpufreq_warn("bad argument!! should be [enable fixed_freq fixed_volt]\n");

	return count;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
/*****************************
 * show current input boost status
 ******************************/
static int mt_gpufreq_input_boost_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_input_boost_state == 1)
		seq_puts(m, "gpufreq input boost is enabled\n");
	else
		seq_puts(m, "gpufreq input boost is disabled\n");

	return 0;
}

/***************************
 * enable/disable input boost
 ****************************/
static ssize_t mt_gpufreq_input_boost_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int debug = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &debug) == 0) {
		if (debug == 0)
			mt_gpufreq_input_boost_state = 0;
		else if (debug == 1)
			mt_gpufreq_input_boost_state = 1;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

	return count;
}

/***************************
 * show lowpower frequency opp enable status
 ****************************/
static int mt_gpufreq_lpt_enable_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "not implemented\n");

	return 0;
}

/***********************
 * enable lowpower frequency opp
 ************************/
static ssize_t mt_gpufreq_lpt_enable_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *data)
{

	gpufreq_warn("not implemented\n");
#if 0
	char desc[32];
	int len = 0;

	int enable = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &enable) == 0) {
		if (enable == 0)
			mt_gpufreq_low_power_test_enable = false;
		else if (enable == 1)
			mt_gpufreq_low_power_test_enable = true;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
#endif

	return count;
}

#endif

#define PROC_FOPS_RW(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,				\
	.write		  = mt_ ## name ## _proc_write,				\
}

#define PROC_FOPS_RO(name)							\
	static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations mt_ ## name ## _proc_fops = {		\
	.owner		  = THIS_MODULE,				\
	.open		   = mt_ ## name ## _proc_open,	\
	.read		   = seq_read,					\
	.llseek		 = seq_lseek,					\
	.release		= single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &mt_ ## name ## _proc_fops}

PROC_FOPS_RW(gpufreq_debug);
PROC_FOPS_RW(gpufreq_limited_power);
#ifdef MT_GPUFREQ_OC_PROTECT
PROC_FOPS_RW(gpufreq_limited_oc_ignore);
#endif
#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
PROC_FOPS_RW(gpufreq_limited_low_batt_volume_ignore);
#endif
#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
PROC_FOPS_RW(gpufreq_limited_low_batt_volt_ignore);
#endif
PROC_FOPS_RW(gpufreq_limited_thermal_ignore);
#ifndef DISABLE_PBM_FEATURE
PROC_FOPS_RW(gpufreq_limited_pbm_ignore);
PROC_FOPS_RW(gpufreq_limited_by_pbm);
#endif
PROC_FOPS_RW(gpufreq_state);
PROC_FOPS_RO(gpufreq_opp_dump);
PROC_FOPS_RO(gpufreq_power_dump);
PROC_FOPS_RW(gpufreq_opp_freq);
PROC_FOPS_RW(gpufreq_opp_max_freq);
PROC_FOPS_RO(gpufreq_var_dump);
PROC_FOPS_RW(gpufreq_volt_enable);
PROC_FOPS_RW(gpufreq_fixed_freq_volt);
#ifdef MT_GPUFREQ_INPUT_BOOST
PROC_FOPS_RW(gpufreq_input_boost);
#endif
PROC_FOPS_RW(gpufreq_lpt_enable);

static int mt_gpufreq_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(gpufreq_debug),
		PROC_ENTRY(gpufreq_limited_power),
#ifdef MT_GPUFREQ_OC_PROTECT
		PROC_ENTRY(gpufreq_limited_oc_ignore),
#endif
#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
		PROC_ENTRY(gpufreq_limited_low_batt_volume_ignore),
#endif
#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
		PROC_ENTRY(gpufreq_limited_low_batt_volt_ignore),
#endif
		PROC_ENTRY(gpufreq_limited_thermal_ignore),
#ifndef DISABLE_PBM_FEATURE
		PROC_ENTRY(gpufreq_limited_pbm_ignore),
		PROC_ENTRY(gpufreq_limited_by_pbm),
#endif
		PROC_ENTRY(gpufreq_state),
		PROC_ENTRY(gpufreq_opp_dump),
		PROC_ENTRY(gpufreq_power_dump),
		PROC_ENTRY(gpufreq_opp_freq),
		PROC_ENTRY(gpufreq_opp_max_freq),
		PROC_ENTRY(gpufreq_var_dump),
		PROC_ENTRY(gpufreq_volt_enable),
		PROC_ENTRY(gpufreq_fixed_freq_volt),
#ifdef MT_GPUFREQ_INPUT_BOOST
		PROC_ENTRY(gpufreq_input_boost),
#endif
		PROC_ENTRY(gpufreq_lpt_enable),
	};


	dir = proc_mkdir("gpufreq", NULL);

	if (!dir) {
		gpufreq_err("fail to create /proc/gpufreq @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
			(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			gpufreq_err("@%s: create /proc/gpufreq/%s failed\n", __func__,
					entries[i].name);
	}

	return 0;
}
#endif				/* CONFIG_PROC_FS */

/**********************************
 * mediatek gpufreq initialization
 ***********************************/
 #if 1
static int __init mt_gpufreq_init(void)
{
#ifdef MTK_SSPM
	int i;
	struct mt_gpufreq_table_info *freqs = NULL;
#endif
	int ret = 0;

#ifdef BRING_UP
	/* Skip driver init in bring up stage */
	return 0;
#endif

/*
	if (!is_ext_buck_exist())
		return 0;
*/

	gpufreq_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_gpufreq_create_procfs())
		goto out;

#endif				/* CONFIG_PROC_FS */

#ifdef MTK_SSPM
	/* Elbrus has only one type*/
	mt_gpufreq_dvfs_table_type = 0;
	gpufreq_info("Init GPU DVFS on SSPM\n");

#ifndef DISABLE_PBM_FEATURE
		INIT_DEFERRABLE_WORK(&notify_pbm_gpuoff_work, mt_gpufreq_notify_pbm_gpuoff);
#endif /* DISABLE_PBM_FEATURE */

	mt_gpufreqs_num = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_INIT, mt_gpufreq_dvfs_table_type, 0);
	if (mt_gpufreqs_num <= 0) {
		gpufreq_err("Init GPU DVFS FAILED err (%d)\n", mt_gpufreqs_num);
		return mt_gpufreqs_num;
	}

	/* AP side freq table init */
	freqs = kzalloc((mt_gpufreqs_num) * sizeof(*freqs), GFP_KERNEL);

	/* Copy cache freq table from SSPM */
	for (i = 0; i < mt_gpufreqs_num; i++)	{
		freqs[i].gpufreq_khz = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_FREQ);
		freqs[i].gpufreq_volt = mt_gpufreq_ap2sspm(IPI_GPU_DVFS_OPPIDX_INFO, i, QUERY_VOLT);
		freqs[i].gpufreq_idx = i;
	}
	mt_setup_gpufreqs_table(freqs, mt_gpufreqs_num);

	kfree(freqs);

	mt_gpufreq_sspm_recv_daemon_init();

	mt_gpufreq_ready = true;
	MTK_UNREFERENCED_PARAMETER(mt_setup_gpufreqs_power_table);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_opp_tbl_e1_0);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_opp_tbl_e1_1);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_opp_tbl_e1_2);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_keep_opp_index);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_volt_enable_state);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_opp_max_frequency);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_set);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_keep_max_freq);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_power_calculation);
	MTK_UNREFERENCED_PARAMETER(_get_post_div_order);
	MTK_UNREFERENCED_PARAMETER(mt_gpufreq_dds_calc);
#endif

	/* register platform device/driver */
#if !defined(CONFIG_OF)
	ret = platform_device_register(&mt_gpufreq_pdev);
	if (ret) {
		gpufreq_err("fail to register gpufreq device @ %s()\n", __func__);
		goto out;
	}
#endif
	ret = platform_driver_register(&mt_gpufreq_pdrv);
	if (ret) {
		gpufreq_err("fail to register gpufreq driver @ %s()\n", __func__);
#if !defined(CONFIG_OF)
		platform_device_unregister(&mt_gpufreq_pdev);
#endif
	}

out:
	return ret;
}
#endif
static void __exit mt_gpufreq_exit(void)
{
	platform_driver_unregister(&mt_gpufreq_pdrv);
#if !defined(CONFIG_OF)
	platform_device_unregister(&mt_gpufreq_pdev);
#endif
}

module_init(mt_gpufreq_init);
module_exit(mt_gpufreq_exit);

MODULE_DESCRIPTION("MediaTek GPU Frequency Scaling driver");
MODULE_LICENSE("GPL");



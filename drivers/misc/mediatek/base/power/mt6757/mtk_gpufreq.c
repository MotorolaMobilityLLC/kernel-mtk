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
#include "mtk_cpufreq.h"
#include "mtk_gpufreq.h"
#include "mtk_static_power.h"
#include "mt-plat/upmu_common.h"
#include "mt-plat/sync_write.h"
#include "mach/mtk_pmic_wrap.h"
#include "mt-plat/mtk_devinfo.h"

#include "mt_freqhopping.h"
/* TODO: check this! */
/* #include "mach/mt_static_power.h" */
#include "mach/mtk_thermal.h"
#include "mach/upmu_sw.h"
#include "mach/upmu_hw.h"
#include "mach/mtk_pbm.h"

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#include <linux/regulator/consumer.h>
#endif
/*#define BRING_UP*/

#define DRIVER_NOT_READY -1
#define STATIC_PWR_READY2USE
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
 * Define how to set up VGPU by
 * PMIC_WRAP
 * PMIC
 * external IC
 */
/* #define VGPU_SET_BY_PMIC_WRAP */
#define VGPU_SET_BY_PMIC
/* #define VGPU_SET_BY_EXTIC */

#if defined(VGPU_SET_BY_PMIC)
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
#define VGPU_SET_BY_PMIC_MT6355
/* #define VGPU_SET_BY_PMIC_MT6355_VSRAM */
#else
#define VGPU_SET_BY_PMIC_MT6351
#endif
#endif
#ifdef VGPU_SET_BY_EXTIC
#include "fan53555.h"
#endif

/**************************************************
 * Define register write function
 ***************************************************/
#define mt_gpufreq_reg_write(val, addr)                                        \
	mt_reg_sync_writel((val), ((void *)addr))

/***************************
 * Operate Point Definition
 ****************************/
#define GPUOP(khz, volt, idx)	   \
{						   \
	.gpufreq_khz = khz,	 \
	.gpufreq_volt = volt,   \
	.gpufreq_idx = idx,   \
}

/**************************
 * GPU DVFS OPP table setting
 ***************************/

#define GPU_DVFS_FREQ0 (900000) /* KHz */
#define GPU_DVFS_FREQ1 (785000) /* KHz */
#define GPU_DVFS_FREQ2 (720000) /* KHz */
#define GPU_DVFS_FREQ3 (600000) /* KHz */
#define GPU_DVFS_FREQ4 (485000) /* KHz */
#define GPU_DVFS_FREQ5 (390000) /* KHz */
#define GPU_DVFS_FREQ6 (310000) /* KHz */
#define GPU_DVFS_FREQ7 (270000) /* KHz */

#define GPU_DVFS_FREQ0_KIBO (1000000) /* KHz */
#define GPU_DVFS_FREQ1_KIBO (915000)
#define GPU_DVFS_FREQ2_KIBO (830000)
#define GPU_DVFS_FREQ3_KIBO (650000)
#define GPU_DVFS_FREQ4_KIBO (485000)
#define GPU_DVFS_FREQ5_KIBO (390000)
#define GPU_DVFS_FREQ6_KIBO (310000)
#define GPU_DVFS_FREQ7_KIBO (270000)

#define GPUFREQ_LAST_FREQ_LEVEL (GPU_DVFS_FREQ7)

#define GPU_DVFS_VOLT0 (90000) /* mV x 100 */
#define GPU_DVFS_VOLT1 (85000) /* mV x 100 */
#define GPU_DVFS_VOLT2 (80000) /* mV x 100 */
#define GPU_DVFS_VOLT3 (75000) /* mV x 100 */
#define GPU_DVFS_VOLT4 (70000) /* mV x 100 */
#define GPU_DVFS_VOLT5 (65000) /* mV x 100 */
#define GPU_DVFS_VOLT6 (62500) /* mv x 100 */
#define GPU_DVFS_VOLT7 (61250) /* mV x 100 */

#define GPU_DVFS_PTPOD_DISABLE_VOLT GPU_DVFS_VOLT1

#define UNIVPLL_FREQ (416000) /* KHz */

/*****************************************
 * PMIC settle time (us), should not be changed
 ******************************************/
#ifdef VGPU_SET_BY_PMIC_MT6355
#ifdef VGPU_SET_BY_PMIC_MT6355_VSRAM
#define PMIC_MAX_VSRAM_VGPU GPU_DVFS_VSRAM0
#endif
#define GPU_DVFS_VOLT_MIN GPU_DVFS_FREQ7_KIBO
#define GPU_DVFS_VOLT_MAX GPU_DVFS_FREQ0_KIBO
#endif
#ifdef VGPU_SET_BY_PMIC /* 6351 ?*/
#define PMIC_CMD_DELAY_TIME 5
#define MIN_PMIC_SETTLE_TIME 25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)                           \
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)                         \
	(((((old_volt) - (new_volt)) * 2) / 625) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_ON_OFF_DELAY_US (200)
#define INVALID_SLEW_RATE (0)
/* #define GPU_DVFS_PMIC_SETTLE_TIME (40) // us */

#if defined VGPU_SET_BY_PMIC_MT6351
#define PMIC_BUCK_VGPU_VOSEL_ON MT6351_PMIC_BUCK_VGPU_VOSEL_ON
#define PMIC_ADDR_VGPU_VOSEL_ON MT6351_PMIC_BUCK_VGPU_VOSEL_ON_ADDR
#define PMIC_ADDR_VGPU_VOSEL_ON_MASK MT6351_PMIC_BUCK_VGPU_VOSEL_ON_MASK
#define PMIC_ADDR_VGPU_VOSEL_ON_SHIFT MT6351_PMIC_BUCK_VGPU_VOSEL_ON_SHIFT
#define PMIC_ADDR_VGPU_VOSEL_CTRL MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_ADDR
#define PMIC_ADDR_VGPU_VOSEL_CTRL_MASK MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_MASK
#define PMIC_ADDR_VGPU_VOSEL_CTRL_SHIFT MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_SHIFT
#define PMIC_ADDR_VGPU_EN MT6351_PMIC_BUCK_VGPU_EN_ADDR
#define PMIC_ADDR_VGPU_EN_MASK MT6351_PMIC_BUCK_VGPU_EN_MASK
#define PMIC_ADDR_VGPU_EN_SHIFT MT6351_PMIC_BUCK_VGPU_EN_SHIFT
#define PMIC_ADDR_VGPU_EN_CTRL MT6351_PMIC_BUCK_VGPU_EN_CTRL_ADDR
#define PMIC_ADDR_VGPU_EN_CTRL_MASK MT6351_PMIC_BUCK_VGPU_EN_CTRL_MASK
#define PMIC_ADDR_VGPU_EN_CTRL_SHIFT MT6351_PMIC_BUCK_VGPU_EN_CTRL_SHIFT
#endif

#elif defined(VGPU_SET_BY_EXTIC)
#define GPU_LDO_BASE 0x10001000
#define EXTIC_VSEL0 0x0 /* [0]     */
#define EXTIC_BUCK_EN0_MASK 0x1
#define EXTIC_BUCK_EN0_SHIFT 0x7
#define EXTIC_VSEL1 0x1 /* [0]     */
#define EXTIC_BUCK_EN1_MASK 0x1
#define EXTIC_BUCK_EN1_SHIFT 0x7
#define EXTIC_VGPU_CTRL 0x2
#define EXTIC_VGPU_SLEW_MASK 0x7
#define EXTIC_VGPU_SLEW_SHIFT 0x4

#define EXTIC_VOLT_ON_OFF_DELAY_US		350
#define EXTIC_VOLT_STEP			12826	/* 12.826mV per step */
#define EXTIC_SLEW_STEP			100	/* 10.000mV per step */
#define EXTIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt, slew_rate)	\
	(((((new_volt) - (old_volt)) * \
	EXTIC_SLEW_STEP) /  EXTIC_VOLT_STEP) / (slew_rate))	/* us */
#define EXTIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt, slew_rate)	\
	(((((old_volt) - (new_volt)) * \
	EXTIC_SLEW_STEP) /  EXTIC_VOLT_STEP) / (slew_rate))	/* us */
#endif
/* efuse */
#define GPUFREQ_EFUSE_INDEX (8)
#define EFUSE_MFG_SPD_BOND_SHIFT (8)
#define EFUSE_MFG_SPD_BOND_MASK (0xF)
#define FUNC_CODE_EFUSE_INDEX (22)
/*
 * LOG
 */
#define TAG "[Power/gpufreq] "

#define gpufreq_err(fmt, args...)	   \
	pr_debug(TAG"[ERROR]"fmt, ##args)
#define gpufreq_warn(fmt, args...)	\
	pr_debug(TAG"[WARNING]"fmt, ##args)
#define gpufreq_info(fmt, args...)	\
	pr_info(TAG""fmt, ##args)
#define gpufreq_dbg(fmt, args...)	   \
	do {								\
		if (mt_gpufreq_debug)		   \
			pr_debug(TAG""fmt, ##args);	 \
	} while (0)
/*#define gpufreq_dbg gpufreq_warn*/
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


/*****************
 * input boost notification
 ******************/
typedef void (*gpufreq_input_boost_notify)(unsigned int);

static gpufreq_power_limit_notify g_pGpufreq_power_limit_notify;
#ifdef MT_GPUFREQ_INPUT_BOOST
static gpufreq_input_boost_notify g_pGpufreq_input_boost_notify;
#endif
static gpufreq_ptpod_update_notify g_pGpufreq_ptpod_update_notify;

/***************************
 * GPU DVFS OPP Table
 ****************************/
/* Full Yield */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1, 1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT2, 2),
	GPUOP(GPU_DVFS_FREQ3, GPU_DVFS_VOLT3, 3),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT4, 4),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT5, 5),
	GPUOP(GPU_DVFS_FREQ6, GPU_DVFS_VOLT6, 6),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT7, 7),
};
static struct mt_gpufreq_table_info mt_gpufreq_opp_kibo_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0_KIBO, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ1_KIBO, GPU_DVFS_VOLT1, 1),
	GPUOP(GPU_DVFS_FREQ2_KIBO, GPU_DVFS_VOLT2, 2),
	GPUOP(GPU_DVFS_FREQ3_KIBO, GPU_DVFS_VOLT3, 3),
	GPUOP(GPU_DVFS_FREQ4_KIBO, GPU_DVFS_VOLT4, 4),
	GPUOP(GPU_DVFS_FREQ5_KIBO, GPU_DVFS_VOLT5, 5),
	GPUOP(GPU_DVFS_FREQ6_KIBO, GPU_DVFS_VOLT6, 6),
	GPUOP(GPU_DVFS_FREQ7_KIBO, GPU_DVFS_VOLT7, 7),
};
/*
 * AEE (SRAM debug)
 */
#ifdef MT_GPUFREQ_AEE_RR_REC
enum gpu_dvfs_state {
	GPU_DVFS_IS_DOING_DVFS = 0,
	GPU_DVFS_IS_VGPU_ENABLED,
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

static unsigned int g_cur_gpu_freq = 520000; /* initial value */
static unsigned int g_cur_gpu_volt = 100000; /* initial value */
static unsigned int g_cur_gpu_idx = 0xFF;
static unsigned int g_cur_gpu_OPPidx = 0xFF;
/* voltage max rising slew rate */
static unsigned int g_max_slew_rate = INVALID_SLEW_RATE;

static unsigned int g_cur_freq_init_keep;

static bool mt_gpufreq_ready;

/* In default settiing, freq_table[0] is max frequency, freq_table[num-1] is min
 * frequency,
 */
static unsigned int g_gpufreq_max_id;

/* If not limited, it should be set to freq_table[0] (MAX frequency) */
static unsigned int g_limited_max_id;
static unsigned int g_limited_min_id;

static bool mt_gpufreq_debug;
static bool mt_gpufreq_pause;
static bool mt_gpufreq_keep_max_frequency_state;
static bool mt_gpufreq_keep_opp_frequency_state;
#if 1
static unsigned int mt_gpufreq_keep_opp_frequency;
#endif
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_volt_state;
static unsigned int mt_gpufreq_fixed_frequency;
static unsigned int mt_gpufreq_fixed_voltage;

#if 1
static unsigned int mt_gpufreq_volt_enable;
#endif
static unsigned int mt_gpufreq_volt_enable_state;
#ifdef MT_GPUFREQ_INPUT_BOOST
static unsigned int mt_gpufreq_input_boost_state = 1;
#endif
/* static bool g_limited_power_ignore_state = false; */
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
static unsigned int mt_gpufreq_dvfs_table_cid;

/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);
static DEFINE_MUTEX(mt_gpufreq_power_lock);

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;
static struct mt_gpufreq_power_table_info *mt_gpufreqs_power;
static struct mt_gpufreq_clk_t *mt_gpufreq_clk;

#ifdef VGPU_SET_BY_PMIC_MT6355
static struct mt_gpufreq_pmic_t *mt_gpufreq_pmic;
#endif
/* static struct mt_gpufreq_power_table_info *mt_gpufreqs_default_power; */

static bool mt_gpufreq_ptpod_disable;
static int mt_gpufreq_ptpod_disable_idx;

static void mt_gpufreq_clock_switch(unsigned int freq_new);
static void mt_gpufreq_volt_switch(unsigned int volt_old,
				   unsigned int volt_new);
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new,
			   bool enable_kick_pbm);
static unsigned int _mt_gpufreq_get_cur_volt(void);
static unsigned int _mt_gpufreq_get_cur_freq(void);
static void _mt_gpufreq_kick_pbm(int enable);

#ifndef DISABLE_PBM_FEATURE
static bool g_limited_pbm_ignore_state;
static unsigned int mt_gpufreq_pbm_limited_gpu_power; /* PBM limit power */
static unsigned int
	mt_gpufreq_pbm_limited_index; /* Limited frequency index for PBM */
#define GPU_OFF_SETTLE_TIME_MS (100)
struct delayed_work notify_pbm_gpuoff_work;
#endif

int __attribute__ ((weak))
get_immediate_gpu_wrap(void)
{
	gpufreq_err("get_immediate_gpu_wrap doesn't exist");
	return 0;
}

/*******************************************************************************
 * Get Slew rate
 *
 * @breif
 * Calculate maximum slew rate
 * e.g: voltage range is 0.6V - 0.9V
 *      it will return slew rate from 0.6 to 0.9V
 *
 * return maximum slew rate, or return 0 if failed
 *******************************************************************************
 */
static unsigned int mt_gpufreq_volt_to_pmic_wrap(unsigned int volt);
static unsigned int _calculate_slew_rate(void)
{
	unsigned int sfchg_rrate, sfchg_rrate_reg;
	unsigned int max_slew_rate;
	const unsigned int max_voltage_range = mt_gpufreqs[0].gpufreq_volt;
	const unsigned int max_voltage_range_steps =
				mt_gpufreq_volt_to_pmic_wrap(max_voltage_range);

/* [MT6757]BUCK_VGPU_SFCHG_RRATE
 *
 *	4:  0.56us
 *	8:  1.0us
 *	11: 1.33us
 *	17: 2.0us
 *	23: 2.67us
 */
#ifdef VGPU_SET_BY_PMIC_MT6351
	pmic_read_interface(MT6351_PMIC_BUCK_VGPU_SFCHG_RRATE_ADDR,
			    &sfchg_rrate_reg,
			    MT6351_PMIC_BUCK_VGPU_SFCHG_RRATE_MASK,
			    MT6351_PMIC_BUCK_VGPU_SFCHG_RRATE_SHIFT);
#else
	pmic_read_interface(PMIC_RG_BUCK_VGPU_SFCHG_RRATE_ADDR,
			    &sfchg_rrate_reg,
			    PMIC_RG_BUCK_VGPU_SFCHG_RRATE_MASK,
			    PMIC_RG_BUCK_VGPU_SFCHG_RRATE_SHIFT);
#endif
	switch (sfchg_rrate_reg) {
	case 4:
	case 8:
		sfchg_rrate = 1;
		break;
	case 11:
	case 17:
		sfchg_rrate = 2;
		break;
	case 23:
	default:
		sfchg_rrate = 3;
		break;
	}

	max_slew_rate = max_voltage_range_steps * sfchg_rrate;
	gpufreq_dbg("@%s: slew_rate = %d sfchg_rrate = %d sfchg_rrate_reg = %x max_voltage[%d] steps = %d\n",
		    __func__, max_slew_rate, sfchg_rrate, sfchg_rrate_reg,
		    max_voltage_range, max_voltage_range_steps);

	return max_slew_rate;
}

/*******************************************************************************
 * Get Post DIV value
 *
 * @breif
 * VCO needs proper post div value to get corresponding dds value to adjust PLL
 *value
 * e.g: VCO range is 1.5GHz - 3.8GHz
 *      required frequency is 900MHz, so pos div could be 2(1.8/2), 4(3.6/4),
 *8(X), 16(X)
 * It may have special requiremt by DE in different efuse value
 * e.g: In MT6757, efuse value(3'b001) ,VCO range is 1.5GHz - 3.8GHz
 *      375MHz - 900MHz, It only can use post div 4, no pos div 2
 *
 * @param[in] freq: required frequency
 * @param[in] efuse: efuse value
 * return corresponding post div by freq and efuse
 *******************************************************************************
 */
static enum post_div_enum _get_post_div_value(unsigned int freq,
					      unsigned int efuse)
{
	/* [MT6757]VCO range: 1.5G - 3.8GHz by div 1/2/4/8/16
	 *
	 *	PLL range: 125MHz - 3.8GHz
	 *	Version(eFuse Value)			Info	 POSTDIV
	 *FOUT
	 *	Free Version(efuse 3'b000)     No limit  1/2/4/8/16  125MHz -
	 *3800MHz
	 */
	enum post_div_enum post_div = POST_DIV4;

	if (efuse == 0) {
		if (freq >= DIV4_MAX_FREQ)
			post_div = POST_DIV4;
		else if (freq < DIV4_MIN_FREQ)
			post_div = POST_DIV8;
	}

	gpufreq_dbg("@%s: freq = %d efuse = %d post_div = %d\n", __func__, freq,
		    efuse, post_div);
	return post_div;
}

/*******************************************************************************
 * Check GPU DVFS Efuse
 *******************************************************************************
 */
static unsigned int mt_gpufreq_get_dvfs_table_type(void)
{
	unsigned int gpu_speed_bounding = 0;
	unsigned int type = 0;
	unsigned int func_code_0, func_code_1;

	func_code_0 =
	    (get_devinfo_with_index(FUNC_CODE_EFUSE_INDEX) >> 24) & 0xf;
	func_code_1 = get_devinfo_with_index(FUNC_CODE_EFUSE_INDEX) & 0xf;

	gpufreq_info(
	    "from efuse: function code 0 = 0x%x, function code 1 = 0x%x\n",
	    func_code_0, func_code_1);

	if (func_code_1 == 0)
		type = 0;
	else if (func_code_1 == 1)
		type = 1;
	else /* if (func_code_1 == 2) */
		type = 2;
	gpu_speed_bounding = (get_devinfo_with_index(GPUFREQ_EFUSE_INDEX) >>
			      EFUSE_MFG_SPD_BOND_SHIFT) &
			     EFUSE_MFG_SPD_BOND_MASK;
	gpufreq_info("GPU frequency bounding from efuse = %x\n",
		     gpu_speed_bounding);

	/* No efuse or free run? use clock-frequency from device tree to
	 * determine GPU table type!
	 */
	if (gpu_speed_bounding == 0) {
#ifdef CONFIG_OF
		static const struct of_device_id gpu_ids[] = {
		    {.compatible = "arm,malit6xx"},
		    {.compatible = "arm,mali-midgard"},
		    {/* sentinel */} };
		struct device_node *node;
		unsigned int gpu_speed = 0;

		node = of_find_matching_node(NULL, gpu_ids);
		if (!node) {
			gpufreq_err("@%s: find GPU node failed\n", __func__);
			gpu_speed = 700; /* default speed */
		} else {
			if (!of_property_read_u32(node, "clock-frequency",
						  &gpu_speed)) {
				gpu_speed = gpu_speed / 1000 / 1000; /* MHz */
			} else {
				gpufreq_err("@%s: missing clock-frequency "
					    "property, use default GPU level\n",
					    __func__);
				gpu_speed = 700; /* default speed */
			}
		}
		gpufreq_info("GPU clock-frequency from DT = %d MHz\n",
			     gpu_speed);

		if (gpu_speed >= 700)
			type = 0; /* 700M */
		else if (gpu_speed >= 600)
			type = 1; /* 600M */
#else
		gpufreq_err("@%s: Cannot get GPU speed from DT!\n", __func__);
		type = 0;
#endif
	}

	switch (gpu_speed_bounding) {
	case 0:
		mt_gpufreq_dvfs_table_cid = 0; /* free run */
		break;
	case 1:
		type = 1; /* 800M */
	case 2:
		type = 0; /* 700M */
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	default:
		type = 2; /* 600M */
		break;
	}

	return type;
}

/*******************************************************************************
 * Check GPU DVFS Efuse for chipset
 * 0: MT6757, 1: kibo, 3:MT6757p, 7: MT6757pdual
 *******************************************************************************
 */
static unsigned int mt_gpufreq_get_chip_efuse(void)
{
	return (get_devinfo_with_index(30) & 0x000000E0) >> 5;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
static struct task_struct *mt_gpufreq_up_task;

static int mt_gpufreq_input_boost_task(void *data)
{
	while (1) {
		gpufreq_dbg("@%s: begin\n", __func__);

		if (g_pGpufreq_input_boost_notify != NULL) {
			gpufreq_dbg("@%s: g_pGpufreq_input_boost_notify\n",
				    __func__);
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

/*******************************************************************************
 * Input boost
 *******************************************************************************
 */
static void mt_gpufreq_input_event(struct input_handle *handle,
				   unsigned int type, unsigned int code,
				   int value)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1) &&
	    (mt_gpufreq_input_boost_state == 1)) {
		gpufreq_dbg("@%s: accept.\n", __func__);

		wake_up_process(mt_gpufreq_up_task);
	}
}

static int mt_gpufreq_input_connect(struct input_handler *handler,
				    struct input_dev *dev,
				    const struct input_device_id *id)
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
	{.driver_info = 1}, {},
};

static struct input_handler mt_gpufreq_input_handler = {
	.event = mt_gpufreq_input_event,
	.connect = mt_gpufreq_input_connect,
	.disconnect = mt_gpufreq_input_disconnect,
	.name = "gpufreq_ib",
	.id_table = mt_gpufreq_ids,
};
#endif

/*
 * Power table calculation
 */
static void mt_gpufreq_power_calculation(unsigned int idx, unsigned int freq,
					 unsigned int volt, unsigned int temp)
{
#define GPU_ACT_REF_POWER 845   /* mW  */
#define GPU_ACT_REF_FREQ 850000 /* KHz */
#define GPU_ACT_REF_VOLT 90000  /* mV x 100 */

	unsigned int p_total = 0, p_dynamic = 0, ref_freq = 0, ref_volt = 0;
	int p_leakage = 0;

	p_dynamic = GPU_ACT_REF_POWER;
	ref_freq = GPU_ACT_REF_FREQ;
	ref_volt = GPU_ACT_REF_VOLT;

	p_dynamic = p_dynamic * ((freq * 100) / ref_freq) *
		    ((volt * 100) / ref_volt) * ((volt * 100) / ref_volt) /
		    (100 * 100 * 100);

#ifdef STATIC_PWR_READY2USE
	p_leakage = mt_spower_get_leakage(MT_SPOWER_GPU, (volt / 100), temp);
	if (!mt_gpufreq_volt_enable_state || p_leakage < 0)
		p_leakage = 0;
#else
	p_leakage = 71;
#endif

	p_total = p_dynamic + p_leakage;

	gpufreq_ver(
	    "%d: p_dynamic = %d, p_leakage = %d, p_total = %d, temp = %d\n",
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

/**************************************
 * Convert pmic wrap register to voltage
 ***************************************/
static unsigned int mt_gpufreq_pmic_wrap_to_volt(unsigned int pmic_wrap_value)
{
	unsigned int volt = 0;
#ifdef VGPU_SET_BY_EXTIC
	volt = ((pmic_wrap_value * 12826) + 603000) / 10;
	/* 1.39375V */
	if (volt > 141150) {
		gpufreq_err("@%s: volt > 1.411v!\n", __func__);
		return 141100;
	}
#endif
#ifdef VGPU_SET_BY_PMIC_MT6351
	volt = (pmic_wrap_value * 625) + 60000;

	gpufreq_dbg("@%s: volt = %d\n", __func__, volt);

#endif
#ifdef VGPU_SET_BY_PMIC_MT6355
	/* kibo plus: VOUT = 0.40625V + 6.25mV * VOSEL */
	volt = (pmic_wrap_value * 625) + 40625;
#endif

	/* SW upper bound 1.39375V, HW has another upper bound protect */
	if (volt > 139375) {
		gpufreq_err("@%s: volt > 1.39375v!\n", __func__);
		return 139375;
	}
	return volt;
}

/**************************************
 * Convert voltage to pmic wrap register
 ***************************************/
static unsigned int mt_gpufreq_volt_to_pmic_wrap(unsigned int volt)
{
	unsigned int reg_val = 0;

#ifdef VGPU_SET_BY_PMIC_MT6351
	reg_val = (volt - 60000) / 625;

	gpufreq_dbg("@%s: reg_val = %d\n", __func__, reg_val);
#endif
#ifdef VGPU_SET_BY_PMIC_MT6355
	reg_val = (volt - 40625) / 625;
	gpufreq_dbg("@%s: reg_val = %d\n", __func__, reg_val);
#endif

	if (reg_val > 0x7F) {
		gpufreq_err("@%s: reg_val > 0x7F!\n", __func__);
		return 0x7F;
	}

	return reg_val;
}

/* Set frequency and voltage at driver probe function */
static void mt_gpufreq_set_initial(void)
{
	unsigned int cur_volt = 0, cur_freq = 0;
	int i = 0;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
				   (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_volt >= mt_gpufreqs[i].gpufreq_volt) {
			mt_gpufreq_set(cur_freq, mt_gpufreqs[i].gpufreq_khz,
				       cur_volt, mt_gpufreqs[i].gpufreq_volt,
				       true);
			g_cur_gpu_OPPidx = i;
			gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
			_mt_gpufreq_kick_pbm(1);
			break;
		}
	}

	/* Not found, set to LPM */
	if (i == mt_gpufreqs_num) {
		gpufreq_err("Set to LPM since GPU idx not found according to "
			    "current Vcore = %d mV\n",
			    cur_volt / 100);
		g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
		mt_gpufreq_set(
		    cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
		    cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt, true);
	}

	g_cur_gpu_freq = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz;
	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
	g_cur_gpu_idx = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_idx;

	g_max_slew_rate = _calculate_slew_rate();

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
				   ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
}

static unsigned int mt_gpufreq_calc_pmic_settle_time(unsigned int volt_old,
						     unsigned int volt_new)
{
	unsigned int delay = 100;
#if defined(VGPU_SET_BY_EXTIC)
	unsigned char reg_val;
	unsigned int slew_rate;
#endif

	if (volt_new == volt_old) {
		return 0;
	} else if (volt_new > volt_old) {
#ifdef VGPU_SET_BY_PMIC
		delay = PMIC_VOLT_UP_SETTLE_TIME(volt_old, volt_new);
#elif defined(VGPU_SET_BY_EXTIC)
		fan53555_read_byte(0x2, &reg_val);
		slew_rate = 1 << (6 - ((reg_val >> 4) & (0x7)));
		delay =
		    EXTIC_VOLT_UP_SETTLE_TIME(volt_old, volt_new, slew_rate);
#endif
	} else {
#ifdef VGPU_SET_BY_PMIC
		delay = PMIC_VOLT_DOWN_SETTLE_TIME(volt_old, volt_new);
#elif defined(VGPU_SET_BY_EXTIC)
		fan53555_read_byte(0x2, &reg_val);
		slew_rate = 1 << (6 - ((reg_val >> 4) & (0x7)));
		delay =
		    EXTIC_VOLT_DOWN_SETTLE_TIME(volt_old, volt_new, slew_rate);
#endif
	}
#ifdef VGPU_SET_BY_PMIC
	if (delay < MIN_PMIC_SETTLE_TIME)
		delay = MIN_PMIC_SETTLE_TIME;
#endif

	return delay;
}

#ifndef DISABLE_PBM_FEATURE
static void mt_gpufreq_notify_pbm_gpuoff(struct work_struct *work)
{
	mutex_lock(&mt_gpufreq_lock);
	if (!mt_gpufreq_volt_enable_state)
		_mt_gpufreq_kick_pbm(0);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif

/* Set VGPU enable/disable when GPU clock be switched on/off */
unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	int ret = 0;
#ifdef VGPU_SET_BY_EXTIC
	unsigned char reg_val = 0;
#else
	unsigned int reg_val = 0;
#endif
#if 0 /* from clk-mtk.h for CCF bring up */
	if (enable == 0) {
		gpufreq_dbg(
		"@%s, VGPU should keep on since MTCMOS cannot be turned off!\n",
		__func__);
		return DRIVER_NOT_READY;
	}
#endif

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		ret = DRIVER_NOT_READY;
		goto exit;
	}

	if (enable == mt_gpufreq_volt_enable_state) {
		ret = 0;
		goto exit;
	}

	if (mt_gpufreq_ptpod_disable == true) {
		if (enable == 0) {
			gpufreq_info("mt_gpufreq_ptpod_disable == true\n");
			ret = DRIVER_NOT_READY;
			goto exit;
		}
	}

	if (enable == 1) {
/* Enable VGPU */
#if defined(VGPU_SET_BY_PMIC_MT6355)
		ret = regulator_enable(mt_gpufreq_pmic->reg_vsram);
		if (ret) {
			gpufreq_err("Enable VSRAM_VGPU failed! (%d)", ret);
			goto exit;
		}
		ret = regulator_enable(mt_gpufreq_pmic->reg_vgpu);
		if (ret) {
			gpufreq_err("Enable VGPU failed! (%d)", ret);
			goto exit;
		}
#elif defined(VGPU_SET_BY_PMIC_MT6351)
		pmic_config_interface(
		    PMIC_ADDR_VGPU_EN, 0x1, PMIC_ADDR_VGPU_EN_MASK,
		    PMIC_ADDR_VGPU_EN_SHIFT); /* Set VGPU_EN[0] to 1 */
#elif defined(VGPU_SET_BY_EXTIC)
		fan53555_config_interface(EXTIC_VSEL0, 0x1, EXTIC_BUCK_EN0_MASK,
					  EXTIC_BUCK_EN0_SHIFT);
#endif
	} else {
/* Disable VGPU */
#if defined(VGPU_SET_BY_PMIC_MT6355)
		ret = regulator_disable(mt_gpufreq_pmic->reg_vgpu);
		if (ret) {
			gpufreq_err("Disable VGPU failed! (%d)", ret);
			goto exit;
		}
		ret = regulator_disable(mt_gpufreq_pmic->reg_vsram);
		if (ret) {
			gpufreq_err("Disable VSRAM_VGPU failed! (%d)", ret);
			goto exit;
		}
#elif defined(VGPU_SET_BY_PMIC_MT6351)
		pmic_config_interface(
		    PMIC_ADDR_VGPU_EN, 0x0, PMIC_ADDR_VGPU_EN_MASK,
		    PMIC_ADDR_VGPU_EN_SHIFT); /* Set VGPU_EN[0] to 0 */
#elif defined(VGPU_SET_BY_EXTIC)
		fan53555_config_interface(EXTIC_VSEL0, 0x0, EXTIC_BUCK_EN0_MASK,
					  EXTIC_BUCK_EN0_SHIFT);
#endif
	}
#if defined(VGPU_SET_BY_PMIC_MT6355)
	/* todo: need to check delay*/
	gpufreq_dbg("@%s: enable = %x\n", __func__, enable);
	udelay(PMIC_VOLT_ON_OFF_DELAY_US);
#elif defined(VGPU_SET_BY_PMIC_MT6351)
	/* (g_cur_gpu_volt / 1250) + 26; */
	/* delay = mt_gpufreq_calc_pmic_settle_time(0, g_cur_gpu_volt); */

	gpufreq_dbg("@%s: enable = %x\n", __func__, enable);

	udelay(PMIC_VOLT_ON_OFF_DELAY_US);
#elif defined(VGPU_SET_BY_EXTIC)
	udelay(EXTIC_VOLT_ON_OFF_DELAY_US);
#endif

#ifdef MT_GPUFREQ_AEE_RR_REC
	if (enable == 1)
		aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
					   (1 << GPU_DVFS_IS_VGPU_ENABLED));
	else
		aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
					   ~(1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif
#if defined(VGPU_SET_BY_PMIC_MT6355)
	reg_val = regulator_is_enabled(mt_gpufreq_pmic->reg_vgpu);
#elif defined(VGPU_SET_BY_PMIC_MT6351)
	pmic_read_interface(PMIC_ADDR_VGPU_EN, &reg_val, PMIC_ADDR_VGPU_EN_MASK,
			    PMIC_ADDR_VGPU_EN_SHIFT);
#elif defined(VGPU_SET_BY_EXTIC)
	fan53555_read_interface(EXTIC_VSEL0, &reg_val, EXTIC_BUCK_EN0_MASK,
				EXTIC_BUCK_EN0_SHIFT);
#endif
	/* Error checking */
	if (enable == 1 && reg_val == 0) {
		/* VGPU enable fail, dump info and trigger BUG() */
		int i = 0;

		gpufreq_err("@%s: enable = %x, reg_val = %d\n", __func__,
			    enable, reg_val);

		/* read PMIC chip id via PMIC wrapper */
		for (i = 0; i < 10; i++) {
#ifdef VGPU_SET_BY_EXTIC
			fan53555_read_interface(EXTIC_VSEL0, &reg_val,
						EXTIC_BUCK_EN0_MASK,
						EXTIC_BUCK_EN0_SHIFT);
			gpufreq_err("@%s: i2c num = 0x%x, fan53555 sw ready is "
				    "%d, reg_val = 0x%x\n",
				    __func__, get_fan53555_i2c_ch_num(),
				    is_fan53555_sw_ready(), reg_val);
#elif defined(VGPU_SET_BY_PMIC_MT6351)
			pwrap_read(0x200, &reg_val);
			gpufreq_err("@%s: PMIC CID via pwap = 0x%x\n", __func__,
				    reg_val);
#elif defined(VGPU_SET_BY_PMIC_MT6355)
			/*TODO: 6355 dump fail info*/
			pwrap_read(0x200, &reg_val);
			gpufreq_err("@%s: PMIC CID via pwap = 0x%x\n", __func__,
				    reg_val);
#endif
		}
	}

#ifndef DISABLE_PBM_FEATURE
	if (enable == 1) {
		if (delayed_work_pending(&notify_pbm_gpuoff_work))
			cancel_delayed_work(&notify_pbm_gpuoff_work);
		else
			_mt_gpufreq_kick_pbm(1);
	} else {
		schedule_delayed_work(&notify_pbm_gpuoff_work,
				      msecs_to_jiffies(GPU_OFF_SETTLE_TIME_MS));
	}

#endif

	mt_gpufreq_volt_enable_state = enable;
exit:
	mutex_unlock(&mt_gpufreq_lock);

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);

/************************************************
 * DVFS enable API for PTPOD
 *************************************************/

void mt_gpufreq_enable_by_ptpod(void)
{
	mt_gpufreq_voltage_enable_set(0);
#ifdef MTK_GPU_SPM
	if (mt_gpufreq_ptpod_disable)
		mtk_gpu_spm_resume();
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

#ifdef MTK_GPU_SPM
	mtk_gpu_spm_pause();

	g_cur_gpu_volt = _mt_gpufreq_get_cur_volt();
	g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();
#endif

	mt_gpufreq_ptpod_disable = true;
	gpufreq_info("mt_gpufreq disabled by ptpod\n");

	for (i = 0; i < mt_gpufreqs_num; i++) {
		/* VBoot = 0.85v for PTPOD */
		target_idx = i;
		if (mt_gpufreqs_default[i].gpufreq_volt <=
		    GPU_DVFS_PTPOD_DISABLE_VOLT)
			break;
	}

	mt_gpufreq_ptpod_disable_idx = target_idx;

	mt_gpufreq_voltage_enable_set(1);
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < mt_gpufreqs_num; i++) {
		mt_gpufreqs[i].gpufreq_volt =
		    mt_gpufreqs_default[i].gpufreq_volt;
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n",
			    __func__, i, mt_gpufreqs[i].gpufreq_volt);
	}

#ifndef MTK_GPU_SPM
	mt_gpufreq_volt_switch(g_cur_gpu_volt,
			       mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
#endif

	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

	mutex_unlock(&mt_gpufreq_lock);
}
EXPORT_SYMBOL(mt_gpufreq_restore_default_volt);

/* Set voltage because PTP-OD modified voltage table by PMIC wrapper */
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[],
				    unsigned int array_size)
{

	int i; /* , idx; */
	/* unsigned long flags; */
	unsigned volt = 0;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return DRIVER_NOT_READY;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < array_size; i++) {
		volt = mt_gpufreq_pmic_wrap_to_volt(pmic_volt[i]);
		mt_gpufreqs[i].gpufreq_volt = volt;
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n",
			    __func__, i, volt);
	}

#ifndef MTK_GPU_SPM
	mt_gpufreq_volt_switch(g_cur_gpu_volt,
			       mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
#endif

	g_max_slew_rate = _calculate_slew_rate();
	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
	if (g_pGpufreq_ptpod_update_notify != NULL)
		g_pGpufreq_ptpod_update_notify();
	mutex_unlock(&mt_gpufreq_lock);

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_update_volt);

unsigned int mt_gpufreq_get_dvfs_table_num(void) { return mt_gpufreqs_num; }
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
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

unsigned int mt_gpufreq_get_volt_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
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
	int i = 0, temp = 0;
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

			gpufreq_ver(
			    "update mt_gpufreqs_power[%d].gpufreq_khz = %d\n",
			    i, mt_gpufreqs_power[i].gpufreq_khz);
			gpufreq_ver(
			    "update mt_gpufreqs_power[%d].gpufreq_volt = %d\n",
			    i, mt_gpufreqs_power[i].gpufreq_volt);
			gpufreq_ver(
			    "update mt_gpufreqs_power[%d].gpufreq_power = %d\n",
			    i, mt_gpufreqs_power[i].gpufreq_power);
		}
	} else
		gpufreq_err(
		    "@%s: temp < 0 or temp > 125, NOT update power table!\n",
		    __func__);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif

static void mt_setup_gpufreqs_power_table(int num)
{
	int i = 0, temp = 0;

	mt_gpufreqs_power = kzalloc(
	    (num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mt_gpufreqs_power == NULL)
		return;

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

		mt_gpufreq_power_calculation(
		    i, mt_gpufreqs_power[i].gpufreq_khz,
		    mt_gpufreqs_power[i].gpufreq_volt, temp);

		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_khz = %u\n", i,
			     mt_gpufreqs_power[i].gpufreq_khz);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_volt = %u\n", i,
			     mt_gpufreqs_power[i].gpufreq_volt);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_power = %u\n", i,
			     mt_gpufreqs_power[i].gpufreq_power);
	}

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
	gpufreq_dbg("%s, num = %d\n", __func__, num);

	mt_gpufreqs = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	mt_gpufreqs_default = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	if (mt_gpufreqs == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mt_gpufreqs[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs[i].gpufreq_volt = freqs[i].gpufreq_volt;
		mt_gpufreqs[i].gpufreq_idx = freqs[i].gpufreq_idx;

		mt_gpufreqs_default[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs_default[i].gpufreq_volt = freqs[i].gpufreq_volt;
		mt_gpufreqs_default[i].gpufreq_idx = freqs[i].gpufreq_idx;

		gpufreq_dbg("freqs[%d].gpufreq_khz = %u\n", i,
			    freqs[i].gpufreq_khz);
		gpufreq_dbg("freqs[%d].gpufreq_volt = %u\n", i,
			    freqs[i].gpufreq_volt);
		gpufreq_dbg("freqs[%d].gpufreq_idx = %u\n", i,
			    freqs[i].gpufreq_idx);
	}

	mt_gpufreqs_num = num;

	g_limited_max_id = 0;
	g_limited_min_id = mt_gpufreqs_num - 1;

	gpufreq_info("@%s: g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n",
		     __func__, g_cur_gpu_freq, g_cur_gpu_volt);

	mt_setup_gpufreqs_power_table(num);

	return 0;
}

/**************************************
 * check if maximum frequency is needed
 ***************************************/
static int mt_gpufreq_keep_max_freq(unsigned int freq_old,
				    unsigned int freq_new)
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
		gpufreq_dbg(
		    "enable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
		    g_gpufreq_dvfs_disable_count);

		/***********************************************
		 * enable DVFS if no any module still disable it
		 ************************************************/
		if (g_gpufreq_dvfs_disable_count <= 0)
			mt_gpufreq_pause = false;
		else
			gpufreq_warn("someone still disable gpufreq, cannot "
				     "enable it\n");
	} else {
		/******************
		 * disable GPU DVFS
		 *******************/
		g_gpufreq_dvfs_disable_count++;
		gpufreq_dbg(
		    "disable GPU DVFS: g_gpufreq_dvfs_disable_count = %d\n",
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

static unsigned int mt_gpufreq_dds_calc(unsigned int khz,
					enum post_div_enum post_div)
{
	unsigned int dds = 0;

	gpufreq_dbg("@%s: request freq = %d, div = %d\n", __func__, khz,
		    post_div);
	if ((khz >= 125000) && (khz <= 3800000)) {
		dds = (((khz / 100 * post_div * 2) * 16384) / 26 + 5) / 10;
		/* todo: need to check thra range for MT6757p */
	} else {
		gpufreq_err("@%s: target khz(%d) out of range!\n", __func__,
			    khz);
		/* WARN_ON(); */
	}

	return dds;
}

static void gpu_dvfs_switch_to_univpll(bool on)
{
	int ret;

	ret = clk_prepare_enable(mt_gpufreq_clk->clk_mux);
	if (ret < 0)
		gpufreq_err("@%s: clk_prepare_enable fail\n", __func__);

	if (on) {
#if 0
		/* Step1. Select to UNIVPLL_D3 416MHz   */
		DRV_ClrReg32(CLK_CFG_1, 0x3 << 24);	/* mask */
		DRV_SetReg32(CLK_CFG_1, 0x3 << 24);	/* [25:24] = clk_mfg_sel 2'b3 is UNIVPLL_D3 416MHz */
		DRV_SetReg32(CLK_CFG_UPDATE, 0x1 << 7); /* [7] = mfg_ck_update */
#else
		clk_set_parent(mt_gpufreq_clk->clk_mux,
			       mt_gpufreq_clk->clk_sub_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
#endif
	} else {
#if 0
		/* Step3. Select back to MFGPLL */
		DRV_ClrReg32(CLK_CFG_1, 0x3 << 24);	/* mask */
		DRV_SetReg32(CLK_CFG_1, 0x1 << 24);	/*  [25:24] = clk_mfg_sel 2'b2 is MFGPLL   */
		DRV_SetReg32(CLK_CFG_UPDATE, 0x1 << 7); /* [7] = mfg_ck_update */
#else
		clk_set_parent(mt_gpufreq_clk->clk_mux,
			       mt_gpufreq_clk->clk_main_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
#endif
	}

	gpufreq_dbg("@%s: on = %d, CLK_CFG_1 = 0x%x, CLK_CFG_UPDATE = 0x%x\n",
		    __func__, on, DRV_Reg32(CLK_CFG_1),
		    DRV_Reg32(CLK_CFG_UPDATE));
}

static void mt_gpufreq_clock_switch_transient(unsigned int freq_new,
					      enum post_div_enum post_div)
{
	unsigned int cur_volt;
	unsigned int cur_freq;
	unsigned int univ_volt = mt_gpufreqs[0].gpufreq_volt;
	unsigned int dds;
	unsigned int i;
	unsigned int found;
	unsigned int tmp_idx;

	/*	MMPLL_CON1: 31: MMPLL_SDM_PCW_CHG
	 *			  26-24: MMPLL_POSDIV
	 *				  000: /1
	 *				  001: /2
	 *				  010: /4
	 *				  011: /8
	 *				  100: /18
	 *			  21-0:MMPLL_SDM_PCW
	 */

	found = 0;
	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();
	dds = mt_gpufreq_dds_calc(freq_new, 1 << post_div);
	gpufreq_dbg(
	    "@%s: request GPU dds = 0x%x, cur_volt = %d, cur_freq = %d\n",
	    __func__, dds, cur_volt, cur_freq);
	gpufreq_dbg("@%s: request MMPLL_CON1 = 0x%x\n", __func__,
		    DRV_Reg32(MMPLL_CON1));
	if ((DRV_Reg32(MMPLL_CON1) & (0x7 << 24)) != ((post_div + 1) << 24)) {
		gpufreq_dbg("@%s: switch to univ pll\n", __func__);
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz < UNIVPLL_FREQ) {
				/* record idx since current voltage may not in
				 * DVFS table
				 */
				tmp_idx = i - 1;
				univ_volt = mt_gpufreqs[tmp_idx].gpufreq_volt;
				found = 1;
				gpufreq_dbg("@%s: request GPU univ_volt = %d\n",
					    __func__, univ_volt);
				break;
			}
		}

		if (found == 1) {
			if (cur_volt < univ_volt)
				mt_gpufreq_volt_switch(cur_volt, univ_volt);

			/* Step1. Select to UNIVPLL_D3 416MHz   */
			gpu_dvfs_switch_to_univpll(true);
			/* Step2. Modify MFGPLL POSTDIV */
			DRV_WriteReg32(MMPLL_CON1, (0x80000000) |
						       ((post_div + 1) << 24) |
						       dds);
			udelay(1);
			/* Step3. Select back to MFGPLL */
			gpu_dvfs_switch_to_univpll(false);
			if (cur_volt != univ_volt)
				mt_gpufreq_volt_switch(univ_volt, cur_volt);
			DRV_SetReg32(MMPLL_CON1, 0x80000000);
			udelay(20);
		} else {
			gpufreq_dbg(
			    "@%s: request GPU univ_volt not found, khz = %d, "
			    "volt = %d\n",
			    __func__,
			    mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz,
			    mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_volt);
			/* WARN_ON(); */
		}
	} else {
		gpufreq_dbg("@%s: no switch to univ pll\n", __func__);
		DRV_WriteReg32(MMPLL_CON1,
			       0x80000000 | ((post_div + 1) << 24) | dds);
	}
}

/* static void _mt_gpufreq_set_cur_freq(unsigned int freq_new) */
static void mt_gpufreq_clock_switch(unsigned int freq_new)
{
	unsigned int mfgpll;

	if (mt_gpufreq_dvfs_table_cid == 0)
		mt_gpufreq_clock_switch_transient(
		    freq_new,
		    _get_post_div_value(freq_new, mt_gpufreq_dvfs_table_cid));
	else
		gpufreq_err("@%s: efuse number type(%d)\n", __func__,
			    mt_gpufreq_dvfs_table_cid);

	mfgpll = DRV_Reg32(MMPLL_CON1);
	gpufreq_dbg("@%s: freq_new = %d, mfgpll = 0x%x\n", __func__, freq_new,
		    mfgpll);

	if (g_pFreqSampler != NULL)
		g_pFreqSampler(freq_new);
}

#ifdef VGPU_SET_BY_EXTIC
static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new)
{
	unsigned int delay = 0;

	gpufreq_dbg("@%s: volt_new = %d\n", __func__, volt_new);

	/* set target volt */
	gpu_volt = volt_new / 100;
	if (volt_new % 100 != 0)
		gpu_volt++;

	fan53555_vosel(gpu_volt);
	gpufreq_dbg("gpu_dvfs_set_cur_volt: request volt = %d, gpu volt = %d\n",
		    volt_new, gpu_volt * 100);

	if (volt_new > volt_old) {
		delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new);
		gpufreq_dbg("@%s: delay = %d slew_rate = %d\n", __func__, delay,
			    g_max_slew_rate);
		udelay((g_max_slew_rate != INVALID_SLEW_RATE) ? g_max_slew_rate
							      : delay);
	}
	if (g_pVoltSampler != NULL)
		g_pVoltSampler(volt_new);
}
#endif /* end of mt_gpufreq_volt_switch EXTIC */

#ifdef VGPU_SET_BY_PMIC_MT6351
static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new)
{
	unsigned int reg_val = 0;
	unsigned int delay = 0;

	/* unsigned int RegValGet = 0; */
	gpufreq_dbg("@%s: volt_new = %d\n", __func__, volt_new);

	/* mt_gpufreq_reg_write(0x02B0, PMIC_WRAP_DVFS_ADR2); */

	reg_val = mt_gpufreq_volt_to_pmic_wrap(volt_new);
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_vgpu(reg_val);
#endif

#ifdef VGPU_SET_BY_PMIC
	/* Set VGPU_VOSEL_CTRL[1] to HW control */
	pmic_config_interface(PMIC_ADDR_VGPU_VOSEL_ON, reg_val,
			      PMIC_ADDR_VGPU_VOSEL_ON_MASK,
			      PMIC_ADDR_VGPU_VOSEL_ON_SHIFT);
#elif defined(VGPU_SET_BY_PMIC_WRAP)
#if 1
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VGPU, reg_val);
	mt_cpufreq_apply_pmic_cmd(IDX_NM_VGPU);

/* pmic_read_interface(0x02B0, &RegValGet, 0x7F, 0x0); // Get VDVFS13_EN[0] */
/* gpufreq_dbg("0x02B0 = %d\n", RegValGet); */
#else
	mt_gpufreq_reg_write(reg_val, PMIC_WRAP_DVFS_WDATA2);

	spm_dvfs_ctrl_volt(GPU_DVFS_CTRL_VOLT);
#endif
#endif

	if (volt_new > volt_old) {
		delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new);
		gpufreq_dbg("@%s: delay = %d slew_rate = %d\n", __func__, delay,
			    g_max_slew_rate);
		udelay((g_max_slew_rate != INVALID_SLEW_RATE) ? g_max_slew_rate
							      : delay);
	}

#if 0
	/* Control VSRAM_MD */
	{
		const unsigned int VSRAM_MD_Volt = GPU_DVFS_VOLT0; /* 0.9V */
		unsigned int vsram_md_target_volt = 0;
		unsigned int setSRAM_MD_Volt = 0;

		if (volt_new == GPU_DVFS_VOLT0 && volt_old != GPU_DVFS_VOLT0) {
			vsram_md_target_volt = volt_new + 10000; /* VSRAM_MD = VGPU + 0.1V */
			setSRAM_MD_Volt = 1;
		} else if (volt_old == GPU_DVFS_VOLT0 && volt_new != GPU_DVFS_VOLT0) {
			vsram_md_target_volt = VSRAM_MD_Volt;
			setSRAM_MD_Volt = 1;
		}

		if (setSRAM_MD_Volt) {
			gpufreq_dbg("@%s: sram volt = %d\n", __func__, vsram_md_target_volt);
			reg_val = mt_gpufreq_volt_to_pmic_wrap(vsram_md_target_volt);
			pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_ON, reg_val);
			pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL, reg_val);
		}
	}
#endif

	if (g_pVoltSampler != NULL)
		g_pVoltSampler(volt_new);
}
#endif /* end of mt_gpufreq_volt_switch 6351 */

#ifdef VGPU_SET_BY_PMIC_MT6355
static void mt_gpufreq_volt_switch(unsigned int volt_old, unsigned int volt_new)
{
#ifdef VGPU_SET_BY_PMIC_MT6355_VSRAM
	unsigned int target_vsram;
	unsigned int target_old_vsram;
	unsigned int target_idx;
#endif
	unsigned int delay = 0;

	gpufreq_dbg("@%s: volt_new = %d\n", __func__, volt_new);

#ifdef VGPU_SET_BY_PMIC_MT6355_VSRAM
	target_idx = mt_gpufreq_get_idx_by_target_volt(volt_new);
	target_vsram = mt_gpufreqs[i].gpufreq_vsram;
	target_idx = mt_gpufreq_get_idx_by_target_volt(volt_old);
	target_old_vsram = mt_gpufreqs[i].gpufreq_vsram;
#endif

	if (volt_new > volt_old)
		regulator_set_voltage(mt_gpufreq_pmic->reg_vgpu, volt_new * 10,
				      GPU_DVFS_VOLT_MAX * 10);
	else
		regulator_set_voltage(mt_gpufreq_pmic->reg_vgpu, volt_new * 10,
				      volt_old * 10);

	if (volt_new > volt_old) {
		delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new);
		gpufreq_dbg("@%s: delay = %d slew_rate = %d\n", __func__, delay,
			    g_max_slew_rate);
		udelay((g_max_slew_rate != INVALID_SLEW_RATE) ? g_max_slew_rate
							      : delay);
	}

	if (g_pVoltSampler != NULL)
		g_pVoltSampler(volt_new);
}
#endif /* end of mt_gpufreq_volt_switch 6355 */

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
	unsigned long mfgpll = 0;
	unsigned int post_div = 0;
	unsigned int freq = 0;

	mfgpll = DRV_Reg32(MMPLL_CON1) & ~0xffc00000;
	post_div = (DRV_Reg32(MMPLL_CON1) & (0x3 << 24)) >> 24;
	if ((mfgpll >= 0x10000) && (mfgpll <= 0x3fffff)) {
		/* re-write formula to avoid calculation overflow for 32 bit
		 * system
		 */
		/* freq = (((mfgpll * 100 * 26  >> 14) / (1 << post_div) + 5) /
		 * 10) * 100;
		 */
		freq =
		    (((mfgpll * 100 * 13 >> 13) / (1 << post_div) + 5) / 10) *
		    100;
	} else {
		gpufreq_err("Invalid mfgpll value = %ld\n", mfgpll);
		/* WARN_ON(); */
	}

	gpufreq_dbg("mfgpll = %ld, freq = %d div = %d\n", mfgpll, freq,
		    post_div);

	return freq; /* KHz */
}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{

	unsigned int gpu_volt = 0;
#ifdef VGPU_SET_BY_EXTIC
	unsigned int en = 0;
	unsigned char reg_val = 0;

	gpufreq_dbg("@%s\n", __func__);
	fan53555_read_byte(0x0, &reg_val);
	en = reg_val >> 7;
	if (en) { /* enabled i.e. not 0mv */
		fan53555_read_byte(0x0, &reg_val);
		gpu_volt = ((reg_val & 0x3f) * 12826 + 603000) / 10;
		gpufreq_dbg("gpu_dvfs_get_cur_volt:[EXTIC] pmic_val = 0x%x, "
			    "volt = %d\n",
			    reg_val, gpu_volt);
	} else {
		gpufreq_dbg("gpu_dvfs_get_cur_volt: VGPU not enabled!\n");
		gpu_volt = 0;
	}
#elif defined(VGPU_SET_BY_PMIC_MT6351)
	gpu_volt = mt_gpufreq_pmic_wrap_to_volt(
	    pmic_get_register_value(PMIC_BUCK_VGPU_VOSEL_ON));
	gpufreq_dbg("gpu_dvfs_get_cur_volt:[PMIC] volt = %d\n", gpu_volt);
#elif defined(VGPU_SET_BY_PMIC_MT6355)
	/* Notice: regulator_get_voltage prints uV */
	gpu_volt = regulator_get_voltage(mt_gpufreq_pmic->reg_vgpu) / 10;
	gpufreq_dbg("gpu_dvfs_get_cur_volt:[PMIC] volt = %d\n", gpu_volt);
#else
	gpufreq_dbg("gpu_dvfs_get_cur_volt:[WARN]  no tran value\n");
#endif

	return gpu_volt;
}

static void _mt_gpufreq_kick_pbm(int enable)
{
#ifndef DISABLE_PBM_FEATURE
	int i;
	int tmp_idx = -1;
	unsigned int found = 0;
	unsigned int power;
	unsigned int cur_volt = _mt_gpufreq_get_cur_volt();
	unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

	if (enable) {
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs_power[i].gpufreq_khz == cur_freq) {
				/* record idx since current voltage may not in
				 * DVFS table
				 */
				tmp_idx = i;

				if (mt_gpufreqs_power[i].gpufreq_volt ==
				    cur_volt) {
					power =
					    mt_gpufreqs_power[i].gpufreq_power;
					found = 1;
					kicker_pbm_by_gpu(true, power,
							  cur_volt / 100);
					gpufreq_dbg("@%s: request GPU power = %d, cur_volt = %d, cur_freq = %d\n",
						    __func__, power,
						    cur_volt / 100, cur_freq);
					return;
				}
			}
		}

		if (!found) {
			gpufreq_dbg("@%s: tmp_idx = %d\n", __func__, tmp_idx);

			if (tmp_idx != -1 && tmp_idx < mt_gpufreqs_num) {
				/* use freq to found corresponding power budget
				 */
				power =
				    mt_gpufreqs_power[tmp_idx].gpufreq_power;
				kicker_pbm_by_gpu(true, power, cur_volt / 100);
				gpufreq_dbg("@%s: request GPU power = %d, "
					    "cur_volt = %d, cur_freq = %d\n",
					    __func__, power, cur_volt / 100,
					    cur_freq);
			} else {
				gpufreq_warn("@%s: Cannot found request power "
					     "in power table!\n",
					     __func__);
				gpufreq_warn(
				    "cur_freq = %dKHz, cur_volt = %dmV\n",
				    cur_freq, cur_volt / 100);
			}
		}
	} else {
		kicker_pbm_by_gpu(false, 0, cur_volt / 100);
	}
#endif
}

/*****************************************
 * frequency ramp up and ramp down handler
 ******************************************/
/***********************************************************
 * [note]
 * 1. frequency ramp up need to wait voltage settle
 * 2. frequency ramp down do not need to wait voltage settle
 * 3. Parameter "bool enable_kick_pbm": Special for MT6757p,
	   found an issue that when GPU power off, driver will kick PBM to lower
 resource required
 ************************************************************/
static void mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new,
			   bool enable_kick_pbm)
{
	if (freq_new > freq_old) {
		/* if(volt_old != volt_new) // ??? */
		/* { */
		mt_gpufreq_volt_switch(volt_old, volt_new);
		/* } */

		mt_gpufreq_clock_switch(freq_new);
	} else {
		mt_gpufreq_clock_switch(freq_new);

		/* if(volt_old != volt_new) */
		/* { */
		mt_gpufreq_volt_switch(volt_old, volt_new);
		/* } */
	}

	g_cur_gpu_freq = freq_new;
	g_cur_gpu_volt = volt_new;

	/*Special for MT6757p*/
	if (enable_kick_pbm == true)
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
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_idx, target_OPPidx;

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	return 0;
#endif

	mutex_lock(&mt_gpufreq_lock);

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

	gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n", __func__,
		    target_freq, target_OPPidx);

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
		target_freq =
		    mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
		target_volt =
		    mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
		target_idx = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_idx;
		target_OPPidx = mt_gpufreq_keep_opp_index;
		gpufreq_dbg(
		    "Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n",
		    target_freq, target_volt, target_OPPidx);
	}

	/************************************************
	 * If /proc command fix the frequency.
	 *************************************************/
	if (mt_gpufreq_fixed_freq_volt_state == true) {
		target_freq = mt_gpufreq_fixed_frequency;
		target_volt = mt_gpufreq_fixed_voltage;
		target_idx = 0;
		target_OPPidx = 0;
		gpufreq_dbg("Fixed! fixed frequency %d, fixed voltage %d\n",
			    target_freq, target_volt);
	}

	/************************************************
	 * If /proc command keep opp max frequency.
	 *************************************************/
	if (mt_gpufreq_opp_max_frequency_state == true) {
		if (target_freq > mt_gpufreq_opp_max_frequency) {
			target_freq =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
			target_volt =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
			target_idx =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_idx;
			target_OPPidx = mt_gpufreq_opp_max_index;

			gpufreq_dbg("opp max freq! opp max frequency %d, opp "
				    "max voltage %d, opp max idx %d\n",
				    target_freq, target_volt, target_OPPidx);
		}
	}

/************************************************
 * PBM limit
 *************************************************/
#ifndef DISABLE_PBM_FEATURE
	if (mt_gpufreq_pbm_limited_index != 0) {
		if (target_freq >
		    mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_khz) {
			/*********************************************
			 * target_freq > limited_freq, need to adjust
			 **********************************************/
			target_freq = mt_gpufreqs[mt_gpufreq_pbm_limited_index]
					  .gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_pbm_limited_index]
					  .gpufreq_volt;
			target_OPPidx = mt_gpufreq_pbm_limited_index;
			gpufreq_dbg(
			    "Limit! Thermal/Power limit gpu frequency %d\n",
			    mt_gpufreqs[mt_gpufreq_pbm_limited_index]
				.gpufreq_khz);
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
			target_volt =
			    mt_gpufreqs[g_limited_max_id].gpufreq_volt;
			target_idx = mt_gpufreqs[g_limited_max_id].gpufreq_idx;
			target_OPPidx = g_limited_max_id;
			gpufreq_info(
			    "Limit! Thermal/Power limit gpu frequency %d\n",
			    mt_gpufreqs[g_limited_max_id].gpufreq_khz);
		}
	}

	/************************************************
	 * DVFS keep at max freq when PTPOD initial
	 *************************************************/
	if (mt_gpufreq_ptpod_disable == true) {
#if 1
		target_freq =
		    mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_khz;
		target_volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
		target_idx =
		    mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_idx;
		target_OPPidx = mt_gpufreq_ptpod_disable_idx;
		gpufreq_dbg(
		    "PTPOD disable dvfs, mt_gpufreq_ptpod_disable_idx = %d\n",
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
		gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due "
			    "to same frequency\n",
			    g_cur_gpu_freq, target_freq);
		return 0;
	}

	gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n",
		    g_cur_gpu_freq, target_freq);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
				   (1 << GPU_DVFS_IS_DOING_DVFS));
	aee_rr_rec_gpu_dvfs_oppidx(target_OPPidx);
#endif

	/******************************
	 * set to the target frequency
	 *******************************/
	mt_gpufreq_set(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt,
		       true); /*Special for MT6757p*/

	g_cur_gpu_idx = target_idx;
	g_cur_gpu_OPPidx = target_OPPidx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
				   ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_target);

/*Special for MT6757p, found an issue that when GPU power off, driver will kick
 * PBM to lower resource required
 */
unsigned int mt_gpufreq_target_skip_kick_pbm(unsigned int idx)
{
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_idx, target_OPPidx;

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	return 0;
#endif

	mutex_lock(&mt_gpufreq_lock);

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

	gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n", __func__,
		    target_freq, target_OPPidx);

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
		target_freq =
		    mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
		target_volt =
		    mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
		target_idx = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_idx;
		target_OPPidx = mt_gpufreq_keep_opp_index;
		gpufreq_dbg(
		    "Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n",
		    target_freq, target_volt, target_OPPidx);
	}

	/************************************************
	 * If /proc command fix the frequency.
	 *************************************************/
	if (mt_gpufreq_fixed_freq_volt_state == true) {
		target_freq = mt_gpufreq_fixed_frequency;
		target_volt = mt_gpufreq_fixed_voltage;
		target_idx = 0;
		target_OPPidx = 0;
		gpufreq_dbg("Fixed! fixed frequency %d, fixed voltage %d\n",
			    target_freq, target_volt);
	}

	/************************************************
	 * If /proc command keep opp max frequency.
	 *************************************************/
	if (mt_gpufreq_opp_max_frequency_state == true) {
		if (target_freq > mt_gpufreq_opp_max_frequency) {
			target_freq =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
			target_volt =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
			target_idx =
			    mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_idx;
			target_OPPidx = mt_gpufreq_opp_max_index;

			gpufreq_dbg("opp max freq! opp max frequency %d, opp "
				    "max voltage %d, opp max idx %d\n",
				    target_freq, target_volt, target_OPPidx);
		}
	}

/************************************************
 * PBM limit
 *************************************************/
#ifndef DISABLE_PBM_FEATURE
	if (mt_gpufreq_pbm_limited_index != 0) {
		if (target_freq >
		    mt_gpufreqs[mt_gpufreq_pbm_limited_index].gpufreq_khz) {
			/*********************************************
			 * target_freq > limited_freq, need to adjust
			 **********************************************/
			target_freq = mt_gpufreqs[mt_gpufreq_pbm_limited_index]
					  .gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_pbm_limited_index]
					  .gpufreq_volt;
			target_OPPidx = mt_gpufreq_pbm_limited_index;
			gpufreq_dbg(
			    "Limit! Thermal/Power limit gpu frequency %d\n",
			    mt_gpufreqs[mt_gpufreq_pbm_limited_index]
				.gpufreq_khz);
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
			target_volt =
			    mt_gpufreqs[g_limited_max_id].gpufreq_volt;
			target_idx = mt_gpufreqs[g_limited_max_id].gpufreq_idx;
			target_OPPidx = g_limited_max_id;
			gpufreq_info(
			    "Limit! Thermal/Power limit gpu frequency %d\n",
			    mt_gpufreqs[g_limited_max_id].gpufreq_khz);
		}
	}

	/************************************************
	 * DVFS keep at max freq when PTPOD initial
	 *************************************************/
	if (mt_gpufreq_ptpod_disable == true) {
#if 1
		target_freq =
		    mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_khz;
		target_volt = GPU_DVFS_PTPOD_DISABLE_VOLT;
		target_idx =
		    mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_idx;
		target_OPPidx = mt_gpufreq_ptpod_disable_idx;
		gpufreq_dbg(
		    "PTPOD disable dvfs, mt_gpufreq_ptpod_disable_idx = %d\n",
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
		gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due "
			    "to same frequency\n",
			    g_cur_gpu_freq, target_freq);
		return 0;
	}

	gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n",
		    g_cur_gpu_freq, target_freq);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
				   (1 << GPU_DVFS_IS_DOING_DVFS));
	aee_rr_rec_gpu_dvfs_oppidx(target_OPPidx);
#endif

	/******************************
	 * set to the target frequency
	 *******************************/
	mt_gpufreq_set(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt,
		       false); /*Special for MT6757p*/

	g_cur_gpu_idx = target_idx;
	g_cur_gpu_OPPidx = target_OPPidx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
				   ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);

	return 0;
}
EXPORT_SYMBOL(mt_gpufreq_target_skip_kick_pbm);

/********************************************
 *	   POWER LIMIT RELATED
 ********************************************/
enum { IDX_THERMAL_LIMITED,
	   IDX_LOW_BATT_VOLT_LIMITED,
	   IDX_LOW_BATT_VOLUME_LIMITED,
	   IDX_OC_LIMITED,

	   NR_IDX_POWER_LIMITED,
};

/* NO need to throttle when OC */
#ifdef MT_GPUFREQ_OC_PROTECT
static unsigned int mt_gpufreq_oc_level;

#define MT_GPUFREQ_OC_LIMIT_FREQ_1                                             \
	GPU_DVFS_FREQ4 /* no need to throttle when OC */
static unsigned int
	mt_gpufreq_oc_limited_index_0; /* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_oc_limited_index_1;
static unsigned int
	mt_gpufreq_oc_limited_index; /* Limited frequency index for oc */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
static unsigned int mt_gpufreq_low_battery_volume;

static unsigned int mt_gpufreq_low_bat_volume_limited_freq_1;
static unsigned int mt_gpufreq_low_bat_volume_limited_index_0;
/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volume_limited_index_1;
static unsigned int
	mt_gpufreq_low_batt_volume_limited_index;
/* Limited frequency index for low battery volume */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
static unsigned int mt_gpufreq_low_battery_level;

static unsigned int
	mt_gpufreq_low_bat_volt_limited_freq_1;
/* no need to throttle when LV1 */
static unsigned int mt_gpufreq_low_bat_volt_limited_freq_2;
static unsigned int
	mt_gpufreq_low_bat_volt_limited_index_0;
/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volt_limited_index_1;
static unsigned int mt_gpufreq_low_bat_volt_limited_index_2;
static unsigned int
	mt_gpufreq_low_batt_volt_limited_index;
/* Limited frequency index for low battery voltage */
#endif

static unsigned int
	mt_gpufreq_thermal_limited_gpu_power; /* thermal limit power */
static unsigned int
	mt_gpufreq_prev_thermal_limited_freq; /* thermal limited freq */
/* limit frequency index array */
static unsigned int mt_gpufreq_power_limited_index_array[NR_IDX_POWER_LIMITED] = {0};

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
		if (mt_gpufreq_power_limited_index_array[i] != 0 &&
		    limited_index == 0)
			limited_index = mt_gpufreq_power_limited_index_array[i];
		else if (mt_gpufreq_power_limited_index_array[i] != 0 &&
			 limited_index != 0) {
			if (mt_gpufreq_power_limited_index_array[i] >
			    limited_index)
				limited_index =
				    mt_gpufreq_power_limited_index_array[i];
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if (g_limited_oc_ignore_state == true) {
		gpufreq_info("@%s: g_limited_oc_ignore_state == true!\n",
			     __func__);
		return;
	}

	mt_gpufreq_oc_level = oc_level;

	/* BATTERY_OC_LEVEL_1: >= 5.5A  */
	if (oc_level == BATTERY_OC_LEVEL_1) {
		if (mt_gpufreq_oc_limited_index !=
		    mt_gpufreq_oc_limited_index_1) {
			mt_gpufreq_oc_limited_index =
			    mt_gpufreq_oc_limited_index_1;
			mt_gpufreq_oc_protect(
			    mt_gpufreq_oc_limited_index_1);
		  /* Limit GPU 396.5Mhz */
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_oc_limited_index !=
		    mt_gpufreq_oc_limited_index_0) {
			mt_gpufreq_oc_limited_index =
			    mt_gpufreq_oc_limited_index_0;
			mt_gpufreq_oc_protect(
			    mt_gpufreq_oc_limited_index_0); /* Unlimit */
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

	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLUME_LIMITED] =
	    limited_index;
	mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

void mt_gpufreq_low_batt_volume_callback(
	BATTERY_PERCENT_LEVEL low_battery_volume)
{
	gpufreq_dbg("@%s: low_battery_volume = %d\n", __func__,
		    low_battery_volume);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if (g_limited_low_batt_volume_ignore_state == true) {
		gpufreq_info(
		    "@%s: g_limited_low_batt_volume_ignore_state == true!\n",
		    __func__);
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
			mt_gpufreq_low_batt_volume_protect(
			    mt_gpufreq_low_bat_volume_limited_index_1);
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_low_batt_volume_limited_index !=
		    mt_gpufreq_low_bat_volume_limited_index_0) {
			mt_gpufreq_low_batt_volume_limited_index =
			    mt_gpufreq_low_bat_volume_limited_index_0;
			mt_gpufreq_low_batt_volume_protect(
			    mt_gpufreq_low_bat_volume_limited_index_0);
		  /* Unlimit */
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
	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLT_LIMITED] =
	    limited_index;
	mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

/******************************************************
 * parameter: low_battery_level
 *******************************************************/
void mt_gpufreq_low_batt_volt_callback(LOW_BATTERY_LEVEL low_battery_level)
{
	gpufreq_dbg("@%s: low_battery_level = %d\n", __func__,
		    low_battery_level);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if (g_limited_low_batt_volt_ignore_state == true) {
		gpufreq_info(
		    "@%s: g_limited_low_batt_volt_ignore_state == true!\n",
		    __func__);
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
			mt_gpufreq_low_batt_volt_protect(
			  mt_gpufreq_low_bat_volt_limited_index_1);
		}
	} else
#endif
	if (low_battery_level == LOW_BATTERY_LEVEL_2) {
		if (mt_gpufreq_low_batt_volt_limited_index !=
		    mt_gpufreq_low_bat_volt_limited_index_2) {
			mt_gpufreq_low_batt_volt_limited_index =
			    mt_gpufreq_low_bat_volt_limited_index_2;
			/* Limit GPU 400Mhz */
			mt_gpufreq_low_batt_volt_protect(
			    mt_gpufreq_low_bat_volt_limited_index_2);
		}
	} else { /* unlimit gpu */
		if (mt_gpufreq_low_batt_volt_limited_index !=
		    mt_gpufreq_low_bat_volt_limited_index_0) {
			mt_gpufreq_low_batt_volt_limited_index =
			    mt_gpufreq_low_bat_volt_limited_index_0;
			/* Unlimit */
			mt_gpufreq_low_batt_volt_protect(
			    mt_gpufreq_low_bat_volt_limited_index_0);
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
		limited_freq =
		    mt_gpufreqs_power[mt_gpufreqs_num - 1].gpufreq_khz;

	gpufreq_dbg("@%s: limited_freq = %d\n", __func__, limited_freq);

	return limited_freq;
}

void mt_gpufreq_thermal_protect(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (mt_gpufreqs_num == 0) {
		gpufreq_warn("@%s: mt_gpufreqs_num == 0!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (g_limited_thermal_ignore_state == true) {
		gpufreq_info("@%s: g_limited_thermal_ignore_state == true!\n",
			     __func__);
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
				mt_gpufreq_power_limited_index_array
				    [IDX_THERMAL_LIMITED] = i;
				break;
			}
		}
	}

	if (mt_gpufreq_prev_thermal_limited_freq != limited_freq) {
		mt_gpufreq_prev_thermal_limited_freq = limited_freq;
		mt_gpufreq_power_throttle_protect();
		if (limited_freq < GPU_DVFS_FREQ6)
			gpufreq_info("@%s: p %u f %u i %u\n", __func__,
				     limited_power, limited_freq,
				     mt_gpufreq_power_limited_index_array
					 [IDX_THERMAL_LIMITED]);
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
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (mt_gpufreqs_num == 0) {
		gpufreq_warn("@%s: mt_gpufreqs_num == 0!\n", __func__);
		mutex_unlock(&mt_gpufreq_power_lock);
		return;
	}

	if (g_limited_pbm_ignore_state == true) {
		gpufreq_info("@%s: g_limited_pbm_ignore_state == true!\n",
			     __func__);
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
	mt_update_gpufreqs_power_table(); /* TODO: need to check overhead? */
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

	gpufreq_dbg("PBM limit frequency upper bound to id = %d\n",
		    mt_gpufreq_pbm_limited_index);

	if (g_pGpufreq_power_limit_notify != NULL)
		g_pGpufreq_power_limit_notify(mt_gpufreq_pbm_limited_index);

	mutex_unlock(&mt_gpufreq_power_lock);
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
	gpufreq_dbg("current GPU thermal limit index is %d\n",
		    g_limited_max_id);
	return g_limited_max_id;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_index);

/************************************************
 * return current GPU thermal limit frequency
 *************************************************/
unsigned int mt_gpufreq_get_thermal_limit_freq(void)
{
	gpufreq_dbg("current GPU thermal limit freq is %d MHz\n",
		    mt_gpufreqs[g_limited_max_id].gpufreq_khz / 1000);
	return mt_gpufreqs[g_limited_max_id].gpufreq_khz;
}
EXPORT_SYMBOL(mt_gpufreq_get_thermal_limit_freq);

/************************************************
 * return current GPU frequency index
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq_index(void)
{
	gpufreq_dbg("current GPU frequency OPP index is %d\n",
		    g_cur_gpu_OPPidx);
	return g_cur_gpu_OPPidx;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/************************************************
 * return current GPU frequency
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq(void)
{
#ifdef MTK_GPU_SPM
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
void mt_gpufreq_setfreq_registerCB(sampler_func pCB) { g_pFreqSampler = pCB; }
EXPORT_SYMBOL(mt_gpufreq_setfreq_registerCB);

/************************************************
 * register / unregister set GPU volt CB
 *************************************************/
void mt_gpufreq_setvolt_registerCB(sampler_func pCB) { g_pVoltSampler = pCB; }
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
			gpufreq_dbg("match g_cur_gpu_OPPidx: %d\n",
				    g_cur_gpu_OPPidx);
			break;
		}
	}

	if (found == 0) {
		g_cur_gpu_idx = mt_gpufreqs[0].gpufreq_idx;
		g_cur_gpu_volt = mt_gpufreqs[0].gpufreq_volt;
		g_cur_gpu_OPPidx = 0;
		gpufreq_err("gpu freq not found, set parameter to max freq\n");
	}

	gpufreq_dbg("GPU freq SW/HW: %d/%d\n", g_cur_gpu_freq,
		    _mt_gpufreq_get_cur_freq());
	gpufreq_dbg("g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);

	return 0;
}

int mt_gpufreq_fan53555_init(void)
{
#if defined(VGPU_SET_BY_EXTIC)
	unsigned char reg_val = 0;
	static void *g_ldo_base;

	gpufreq_info("@%s\n", __func__);
	/* enable: // VGPU_SRAM */
	g_ldo_base = ioremap_nocache(GPU_LDO_BASE, 0x1000);
	gpufreq_info("@%s 2\n", __func__);
#if 1
	DRV_WriteReg32(g_ldo_base + 0xfc0, 0x0f0f0f0f);
	DRV_WriteReg32(g_ldo_base + 0xfc4, 0x0f0f0f0f);
	DRV_WriteReg32(g_ldo_base + 0xfbc, 0xff);

	/* enable VGPU power */
	fan53555_config_interface(EXTIC_VSEL0, 0x1, EXTIC_BUCK_EN0_MASK,
				  EXTIC_BUCK_EN0_SHIFT);
	/* set slew rate to 32mv/us */
	fan53555_config_interface(EXTIC_VGPU_CTRL, 0x11, EXTIC_VGPU_SLEW_MASK,
				  EXTIC_VGPU_SLEW_SHIFT);

#if 0
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

	mt_gpufreq_volt_enable_state = 1;
	gpufreq_info(" check VGPU_EN\n");
	fan53555_read_interface(EXTIC_VSEL0, &reg_val, EXTIC_BUCK_EN0_MASK,
				EXTIC_BUCK_EN0_SHIFT);
	gpufreq_info("VGPU_EN = %d\n", reg_val);
	fan53555_read_interface(EXTIC_VGPU_CTRL, &reg_val, EXTIC_VGPU_SLEW_MASK,
				EXTIC_VGPU_SLEW_SHIFT);
#endif
	gpufreq_info("VGPU_SLEW = %d\n", reg_val);

	/**********************
	 * setup initial frequency
	 ***********************/
	mt_gpufreq_set_initial();

	gpufreq_info("GPU current frequency = %dKHz\n",
		     _mt_gpufreq_get_cur_freq());
	gpufreq_info("GPU current volt = %dmV\n",
		     _mt_gpufreq_get_cur_volt() / 100);
	gpufreq_info("g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n",
		     g_cur_gpu_freq, g_cur_gpu_volt);
	gpufreq_info("g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n",
		     g_cur_gpu_idx, g_cur_gpu_OPPidx);

	mt_gpufreq_ready = true;
#endif
	return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id mt_gpufreq_of_match[] = {
	{
	.compatible = "mediatek,mt6757-gpufreq",
	},
	{/* sentinel */},
};
#endif
MODULE_DEVICE_TABLE(of, mt_gpufreq_of_match);

static int mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
#if defined(VGPU_SET_BY_PMIC_MT6355)
	int ret = 0;
#endif

#if defined(VGPU_SET_BY_PMIC_MT6351)
	unsigned int reg_val = 0;
#endif
	int i = 0;
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};
#endif

#ifdef CONFIG_OF
	struct device_node *node;
#endif

#ifdef CONFIG_OF
	node = of_find_matching_node(NULL, mt_gpufreq_of_match);
	if (!node)
		gpufreq_err("@%s: find GPU node failed\n", __func__);

	/* mt_gpufreq_clk if used for gpu_dvfs_switch_to_univpll */
	mt_gpufreq_clk = kzalloc(sizeof(struct mt_gpufreq_clk_t), GFP_KERNEL);
	if (mt_gpufreq_clk == NULL)
		return -ENOMEM;
#if 1
	mt_gpufreq_clk->clk_mux = devm_clk_get(&pdev->dev, "clk_mux");
	if (IS_ERR(mt_gpufreq_clk->clk_mux)) {
		dev_dbg(&pdev->dev, "cannot get clock mux\n");
		return PTR_ERR(mt_gpufreq_clk->clk_mux);
	}
#endif
	mt_gpufreq_clk->clk_main_parent =
	    devm_clk_get(&pdev->dev, "clk_main_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_main_parent)) {
		dev_dbg(&pdev->dev, "cannot get main clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_main_parent);
	}
	mt_gpufreq_clk->clk_sub_parent =
	    devm_clk_get(&pdev->dev, "clk_sub_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_sub_parent)) {
		dev_dbg(&pdev->dev, "cannot get sub clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_sub_parent);
	}

#if defined(VGPU_SET_BY_PMIC_MT6355)
	/* alloc PMIC regulator */
	mt_gpufreq_pmic = kzalloc(sizeof(struct mt_gpufreq_pmic_t), GFP_KERNEL);
	if (mt_gpufreq_pmic == NULL)
		return -ENOMEM;
	mt_gpufreq_pmic->reg_vgpu = regulator_get(&pdev->dev, "vgpu");
	if (IS_ERR(mt_gpufreq_pmic->reg_vgpu)) {
		gpufreq_err("@%s: , cannot get vgpu\n",
			    __func__);
		return PTR_ERR(mt_gpufreq_pmic->reg_vgpu);
	}

#ifdef VGPU_SET_BY_PMIC_MT6355_VSRAM
	mt_gpufreq_pmic->reg_vsram = regulator_get(&pdev->dev, "vsram_vgpu");
	if (IS_ERR(mt_gpufreq_pmic->reg_vsram)) {
		gpufreq_err(
		    "@%s: , cannot get vsram_vgpu\n",
		    __func__);
		return PTR_ERR(mt_gpufreq_pmic->reg_vsram);
	}
#endif
#endif
#endif /* end of 6355 part */

	mt_gpufreq_dvfs_table_type = mt_gpufreq_get_dvfs_table_type();

#ifdef CONFIG_HAS_EARLYSUSPEND
	mt_gpufreq_early_suspend_handler.suspend = mt_gpufreq_early_suspend;
	mt_gpufreq_early_suspend_handler.resume = mt_gpufreq_late_resume;
	register_early_suspend(&mt_gpufreq_early_suspend_handler);
#endif

#ifdef VGPU_SET_BY_PMIC_WRAP
	/**********************
	 * setup PMIC wrap setting
	 ***********************/
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);
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
	if (mt_gpufreq_get_chip_efuse() == 0) {
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
		gpufreq_dbg("@%s: Set OPP table as MT6757\n", __func__);
	} else {
		mt_setup_gpufreqs_table(
		    mt_gpufreq_opp_kibo_tbl_e1_0,
		    ARRAY_SIZE(mt_gpufreq_opp_kibo_tbl_e1_0));
		gpufreq_dbg("@%s: Set OPP table as MT6757P\n", __func__);
	}

/**********************
 * setup PMIC init value
 ***********************/
#ifdef VGPU_SET_BY_PMIC_MT6355
	/* Enable VGPU */
	ret = regulator_is_enabled(mt_gpufreq_pmic->reg_vgpu);
	if (!ret) { /* if ret = 0, meens pmic haven't enable */
		ret = regulator_enable(mt_gpufreq_pmic->reg_vgpu);
		if (ret) {
			gpufreq_err("Enable VGPU failed! (%d)\n", ret);
			return ret;
		}
	}
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
				   (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif
	mt_gpufreq_volt_enable_state = 1;

	gpufreq_info("VGPU Enabled (%d) %d mV\n",
		     regulator_is_enabled(mt_gpufreq_pmic->reg_vgpu),
		     _mt_gpufreq_get_cur_volt());
#ifdef VGPU_SET_BY_PMIC_MT6355_VSRAM
	gpufreq_info("VSRAM_VGPU Enabled (%d) %d mV\n",
		     regulator_is_enabled(mt_gpufreq_pmic->reg_vsram),
		     regulator_get_voltage(mt_gpufreq_pmic->reg_vsram) / 10);
#endif
#endif /* end of MT6355 part*/

#ifdef VGPU_SET_BY_PMIC_MT6351
	/* Set VGPU_VOSEL_CTRL[1] to HW control */
	pmic_config_interface(PMIC_ADDR_VGPU_VOSEL_CTRL, 0x1,
			      PMIC_ADDR_VGPU_VOSEL_CTRL_MASK,
			      PMIC_ADDR_VGPU_VOSEL_CTRL_SHIFT);
	/* Set VGPU_EN_CTRL[0] SW control to 0 */
	pmic_config_interface(PMIC_ADDR_VGPU_EN_CTRL, 0x0,
			      PMIC_ADDR_VGPU_EN_CTRL_MASK,
			      PMIC_ADDR_VGPU_EN_CTRL_SHIFT);
	/* Set VGPU_EN[0] to 1 */
	pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x1, PMIC_ADDR_VGPU_EN_MASK,
			      PMIC_ADDR_VGPU_EN_SHIFT);
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
				   (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

	mt_gpufreq_volt_enable_state = 1;

	/* Get VGPU_VOSEL_CTRL[1] */
	pmic_read_interface(PMIC_ADDR_VGPU_VOSEL_CTRL, &reg_val,
			    PMIC_ADDR_VGPU_VOSEL_CTRL_MASK,
			    PMIC_ADDR_VGPU_VOSEL_CTRL_SHIFT);
	gpufreq_dbg("VGPU_VOSEL_CTRL[1] = %d\n", reg_val);
	/* Get VGPU_EN_CTRL[0] */
	pmic_read_interface(PMIC_ADDR_VGPU_EN_CTRL, &reg_val,
			    PMIC_ADDR_VGPU_EN_CTRL_MASK,
			    PMIC_ADDR_VGPU_EN_CTRL_SHIFT);
	gpufreq_dbg("VGPU_EN_CTRL[0] = %d\n", reg_val);
	/* Get VGPU_EN[0] */
	pmic_read_interface(PMIC_ADDR_VGPU_EN, &reg_val, PMIC_ADDR_VGPU_EN_MASK,
			    PMIC_ADDR_VGPU_EN_SHIFT);
	gpufreq_dbg("VGPU_EN[0] = %d\n", reg_val);
#endif /* end of MT6351 part*/

	g_cur_freq_init_keep = g_cur_gpu_freq;

#ifdef VGPU_SET_BY_PMIC_WRAP
#if 1
	/**********************
	 * PMIC wrap setting for gpu default volt value
	 ***********************/
	reg_val = mt_gpufreq_volt_to_pmic_wrap(mt_gpufreqs[0].gpufreq_volt);
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VGPU, reg_val);
#else
	mt_gpufreq_reg_write(0x02B0, PMIC_WRAP_DVFS_ADR2);

	reg_val = mt_gpufreq_volt_to_pmic_wrap(mt_gpufreqs[0].gpufreq_volt);
	mt_gpufreq_reg_write(reg_val, PMIC_WRAP_DVFS_WDATA2); /* 1.125V */
#endif
#endif

#if defined(VGPU_SET_BY_EXTIC)
	/* Make sure the power control is ready */
	if (!is_fan53555_exist() || !is_fan53555_sw_ready()) {
		gpufreq_dbg("fan53555 is not ready yet\n");
		return -EPROBE_DEFER;
	}
	mt_gpufreq_fan53555_init();
#endif
#if defined(VGPU_SET_BY_PMIC_MT6355) || defined(VGPU_SET_BY_PMIC_MT6351)
	/**********************
	 * setup initial frequency
	 ***********************/
	mt_gpufreq_set_initial();

	gpufreq_info("GPU current frequency = %dKHz\n",
		     _mt_gpufreq_get_cur_freq());
	gpufreq_info("Current Vcore = %dmV\n",
		     _mt_gpufreq_get_cur_volt() / 100);
	gpufreq_info("g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n",
		     g_cur_gpu_freq, g_cur_gpu_volt);
	gpufreq_info("g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n",
		     g_cur_gpu_idx, g_cur_gpu_OPPidx);

	mt_gpufreq_ready = true;
#endif

#ifdef MT_GPUFREQ_INPUT_BOOST
	mt_gpufreq_up_task = kthread_create(mt_gpufreq_input_boost_task, NULL,
					    "mt_gpufreq_input_boost_task");
	if (IS_ERR(mt_gpufreq_up_task))
		return PTR_ERR(mt_gpufreq_up_task);

	sched_setscheduler_nocheck(mt_gpufreq_up_task, SCHED_FIFO, &param);
	get_task_struct(mt_gpufreq_up_task);

	rc = input_register_handler(&mt_gpufreq_input_handler);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
	if (mt_gpufreq_get_chip_efuse() == 0) {
		mt_gpufreq_low_bat_volt_limited_freq_1 = GPU_DVFS_FREQ0;
		mt_gpufreq_low_bat_volt_limited_freq_2 = GPU_DVFS_FREQ4;
	} else {
		mt_gpufreq_low_bat_volt_limited_freq_1 = GPU_DVFS_FREQ0_KIBO;
		mt_gpufreq_low_bat_volt_limited_freq_2 = GPU_DVFS_FREQ4_KIBO;
	}

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz ==
		    mt_gpufreq_low_bat_volt_limited_freq_1) {
			mt_gpufreq_low_bat_volt_limited_index_1 = i;
			break;
		}
	}

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz ==
		    mt_gpufreq_low_bat_volt_limited_freq_2) {
			mt_gpufreq_low_bat_volt_limited_index_2 = i;
			break;
		}
	}

	register_low_battery_notify(&mt_gpufreq_low_batt_volt_callback,
				    LOW_BATTERY_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
	if (mt_gpufreq_get_chip_efuse() == 0)
		mt_gpufreq_low_bat_volume_limited_freq_1 = GPU_DVFS_FREQ0;
	else
		mt_gpufreq_low_bat_volume_limited_freq_1 = GPU_DVFS_FREQ0_KIBO;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz ==
		    mt_gpufreq_low_bat_volume_limited_freq_1) {
			mt_gpufreq_low_bat_volume_limited_index_1 = i;
			break;
		}
	}

	register_battery_percent_notify(&mt_gpufreq_low_batt_volume_callback,
					BATTERY_PERCENT_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_OC_PROTECT
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_OC_LIMIT_FREQ_1) {
			mt_gpufreq_oc_limited_index_1 = i;
			break;
		}
	}

	register_battery_oc_notify(&mt_gpufreq_oc_callback,
				   BATTERY_OC_PRIO_GPU);
#endif

#ifndef DISABLE_PBM_FEATURE
	INIT_DEFERRABLE_WORK(&notify_pbm_gpuoff_work,
			     mt_gpufreq_notify_pbm_gpuoff);
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
static ssize_t mt_gpufreq_debug_proc_write(struct file *file,
					   const char __user *buffer,
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
			gpufreq_warn("bad argument!! should be 0 or 1 [0: "
				     "disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: "
			     "enable]\n");

	return count;
}

#ifdef MT_GPUFREQ_OC_PROTECT
/****************************
 * show current limited by low batt volume
 *****************************/
static int mt_gpufreq_limited_oc_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_oc_ignore_state = %d\n",
		   g_limited_max_id, g_limited_oc_ignore_state);

	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_oc_ignore_proc_write(
	struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;
	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_oc_ignore_state = true;
		else if (ignore == 0)
			g_limited_oc_ignore_state = false;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: not "
				     "ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, "
			     "1: ignore]\n");

	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
/****************************
 * show current limited by low batt volume
 *****************************/
static int
mt_gpufreq_limited_low_batt_volume_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, "
		      "g_limited_low_batt_volume_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volume_ignore_state);

	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_low_batt_volume_ignore_proc_write(
	struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_low_batt_volume_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volume_ignore_state = false;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: not "
				     "ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, "
			     "1: ignore]\n");

	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
/****************************
 * show current limited by low batt volt
 *****************************/
static int mt_gpufreq_limited_low_batt_volt_ignore_proc_show(struct seq_file *m,
							     void *v)
{
	seq_printf(m, "g_limited_max_id = %d, "
		      "g_limited_low_batt_volt_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volt_ignore_state);

	return 0;
}

/**********************************
 * limited for low batt volt protect
 ***********************************/
static ssize_t mt_gpufreq_limited_low_batt_volt_ignore_proc_write(
	struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_low_batt_volt_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volt_ignore_state = false;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: not "
				     "ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, "
			     "1: ignore]\n");

	return count;
}
#endif

/****************************
 * show current limited by thermal
 *****************************/
static int mt_gpufreq_limited_thermal_ignore_proc_show(struct seq_file *m,
						       void *v)
{
	seq_printf(
	    m, "g_limited_max_id = %d, g_limited_thermal_ignore_state = %d\n",
	    g_limited_max_id, g_limited_thermal_ignore_state);

	return 0;
}

/**********************************
 * limited for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_thermal_ignore_proc_write(
	struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_thermal_ignore_state = true;
		else if (ignore == 0)
			g_limited_thermal_ignore_state = false;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: not "
				     "ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, "
			     "1: ignore]\n");

	return count;
}

#ifndef DISABLE_PBM_FEATURE
/****************************
 * show current limited by PBM
 *****************************/
static int mt_gpufreq_limited_pbm_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_oc_ignore_state = %d\n",
		   g_limited_max_id, g_limited_pbm_ignore_state);

	return 0;
}

/**********************************
 * limited for low batt volume protect
 ***********************************/
static ssize_t mt_gpufreq_limited_pbm_ignore_proc_write(
	struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	unsigned int ignore = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtouint(desc, 0, &ignore) == 0) {
		if (ignore == 1)
			g_limited_pbm_ignore_state = true;
		else if (ignore == 0)
			g_limited_pbm_ignore_state = false;
		else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: not "
				     "ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, "
			     "1: ignore]\n");

	return count;
}
#endif

/****************************
 * show current limited power
 *****************************/
static int mt_gpufreq_limited_power_proc_show(struct seq_file *m, void *v)
{

	seq_printf(m, "g_limited_max_id = %d, limit frequency = %d\n",
		   g_limited_max_id, mt_gpufreqs[g_limited_max_id].gpufreq_khz);

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
		mt_gpufreq_thermal_protect(power);
	else
		gpufreq_warn("bad argument!! please provide the maximum "
			     "limited power\n");

	return count;
}

/****************************
 * show current limited power by PBM
 *****************************/
#ifndef DISABLE_PBM_FEATURE
static int mt_gpufreq_limited_by_pbm_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "pbm_limited_power = %d, limit index = %d\n",
		   mt_gpufreq_pbm_limited_gpu_power,
		   mt_gpufreq_pbm_limited_index);

	return 0;
}

/**********************************
 * limited power for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_by_pbm_proc_write(struct file *file,
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
		mt_gpufreq_set_power_limit_by_pbm(power);
	else
		gpufreq_warn("bad argument!! please provide the maximum "
			     "limited power\n");

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
					   const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int enabled = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &enabled) == 0) {
		if (enabled == 1) {
			mt_gpufreq_keep_max_frequency_state = false;
			mt_gpufreq_state_set(1);
		} else if (enabled == 0) {
			/* Keep MAX frequency when GPU DVFS disabled. */
			mt_gpufreq_keep_max_frequency_state = true;
			mt_gpufreq_voltage_enable_set(1);
			mt_gpufreq_target(g_gpufreq_max_id);
			mt_gpufreq_state_set(0);
		} else
			gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");
	} else
		gpufreq_warn(
		    "bad argument!! argument should be \"1\" or \"0\"\n");

	return count;
}

/********************
 * show GPU OPP table
 *********************/
static int mt_gpufreq_opp_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		seq_printf(m, "[%d] ", i);
		seq_printf(m, "freq = %d, ", mt_gpufreqs[i].gpufreq_khz);
		seq_printf(m, "volt = %d, ", mt_gpufreqs[i].gpufreq_volt);
		seq_printf(m, "idx = %d\n", mt_gpufreqs[i].gpufreq_idx);
	}

	return 0;
}

/********************
 * show GPU power table
 *********************/
static int mt_gpufreq_power_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < mt_gpufreqs_num; i++) {
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_khz);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_volt);
		seq_printf(m, "mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
			   mt_gpufreqs_power[i].gpufreq_power);
	}

	return 0;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_keep_opp_frequency_state) {
		seq_puts(m, "gpufreq keep opp frequency enabled\n");
		seq_printf(m, "freq = %d\n",
			   mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n",
			   mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq keep opp frequency disabled\n");
	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_freq_proc_write(struct file *file,
					      const char __user *buffer,
					      size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int i = 0;
	int fixed_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &fixed_freq) == 0) {
		if (fixed_freq == 0) {
			mt_gpufreq_keep_opp_frequency_state = false;
#ifdef MTK_GPU_SPM
			mtk_gpu_spm_reset_fix();
#endif
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

#ifndef MTK_GPU_SPM
				mt_gpufreq_voltage_enable_set(1);
				mt_gpufreq_target(mt_gpufreq_keep_opp_index);
#else
				mtk_gpu_spm_fix_by_idx(
				    mt_gpufreq_keep_opp_index);
#endif
			}
		}
	} else
		gpufreq_warn(
		    "bad argument!! please provide the fixed frequency\n");

	return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_opp_max_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_opp_max_frequency_state) {
		seq_puts(m, "gpufreq opp max frequency enabled\n");
		seq_printf(m, "freq = %d\n",
			   mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz);
		seq_printf(m, "volt = %d\n",
			   mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt);
	} else
		seq_puts(m, "gpufreq opp max frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_opp_max_freq_proc_write(struct file *file,
						  const char __user *buffer,
						  size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int i = 0;
	int max_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

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
				    mt_gpufreqs[mt_gpufreq_opp_max_index]
					.gpufreq_khz;

				mt_gpufreq_voltage_enable_set(1);
				mt_gpufreq_target(mt_gpufreq_opp_max_index);
			}
		}
	} else
		gpufreq_warn("bad argument!! please provide the maximum limited frequency\n");

	return count;
}

/********************
 * show variable dump
 *********************/
static int mt_gpufreq_var_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

#ifdef MTK_GPU_SPM
	seq_puts(m, "DVFS_GPU SPM is on\n");
#endif
	seq_printf(m, "g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n",
		   mt_gpufreq_get_cur_freq(), mt_gpufreq_get_cur_volt());
	seq_printf(m, "g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n",
		   g_cur_gpu_idx, g_cur_gpu_OPPidx);
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);

	for (i = 0; i < NR_IDX_POWER_LIMITED; i++)
		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d\n",
			   i, mt_gpufreq_power_limited_index_array[i]);

	seq_printf(m, "_mt_gpufreq_get_cur_freq = %d\n",
		   _mt_gpufreq_get_cur_freq());
	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n",
		   mt_gpufreq_volt_enable_state);
	seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n",
		   mt_gpufreq_dvfs_table_type);
	seq_printf(m, "mt_gpufreq_ptpod_disable_idx = %d\n",
		   mt_gpufreq_ptpod_disable_idx);
	seq_printf(m, "chipset: 0x%x\n", mt_gpufreq_get_chip_efuse());

	return 0;
}

/***************************
 * show current voltage enable status
 ****************************/
static int mt_gpufreq_volt_enable_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_volt_enable)
		seq_puts(m, "gpufreq voltage enabled\n");
	else
		seq_puts(m, "gpufreq voltage disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_volt_enable_proc_write(struct file *file,
						 const char __user *buffer,
						 size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int enable = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 0, &enable) == 0) {
		if (enable == 0) {
			mt_gpufreq_voltage_enable_set(0);
			mt_gpufreq_volt_enable = false;
		} else if (enable == 1) {
			mt_gpufreq_voltage_enable_set(1);
			mt_gpufreq_volt_enable = true;
		} else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: "
				     "disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: "
			     "enable]\n");

	return count;
}

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_fixed_freq_volt_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_fixed_freq_volt_state) {
		seq_puts(m, "gpufreq fixed frequency enabled\n");
		seq_printf(m, "fixed frequency = %d\n",
			   mt_gpufreq_fixed_frequency);
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
	/* freq (KHz) */
	if ((fixed_freq >= GPUFREQ_LAST_FREQ_LEVEL) &&
	    (fixed_freq <= GPU_DVFS_FREQ0)) {
		gpufreq_dbg("@ %s, mt_gpufreq_clock_switch1 fix frq = %d, fix "
			    "volt = %d, volt = %d\n",
			    __func__, mt_gpufreq_fixed_frequency,
			    mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_fixed_freq_volt_state = true;
		mt_gpufreq_fixed_frequency = fixed_freq;
		mt_gpufreq_fixed_voltage = g_cur_gpu_volt;
		mt_gpufreq_voltage_enable_set(1);
		gpufreq_dbg("@ %s, mt_gpufreq_clock_switch2 fix frq = %d, fix "
			    "volt = %d, volt = %d\n",
			    __func__, mt_gpufreq_fixed_frequency,
			    mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_clock_switch(mt_gpufreq_fixed_frequency);
		g_cur_gpu_freq = mt_gpufreq_fixed_frequency;
	}
}

static void _mt_gpufreq_fixed_volt(int fixed_volt)
{
/* volt (mV) */
#if defined VGPU_SET_BY_EXTIC
	if (fixed_volt >= (mt_gpufreq_pmic_wrap_to_volt(0x0) / 100) &&
	    fixed_volt <= (mt_gpufreq_pmic_wrap_to_volt(0x3F) / 100)) {
#elif defined VGPU_SET_BY_PMIC_MT6351
	if (fixed_volt >= (mt_gpufreq_pmic_wrap_to_volt(0x0) / 100) &&
	    fixed_volt <= (mt_gpufreq_pmic_wrap_to_volt(0x7F) / 100)) {
#elif defined VGPU_SET_BY_PMIC_MT6355
	if (fixed_volt >= (GPU_DVFS_VOLT_MIN / 100) &&
	    fixed_volt <= (GPU_DVFS_VOLT_MAX / 100)) {
#endif
		gpufreq_dbg("@ %s, mt_gpufreq_volt_switch1 fix frq = %d, fix "
			    "volt = %d, volt = %d\n",
			    __func__, mt_gpufreq_fixed_frequency,
			    mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_fixed_freq_volt_state = true;
		mt_gpufreq_fixed_frequency = g_cur_gpu_freq;
		mt_gpufreq_fixed_voltage = fixed_volt * 100;
		mt_gpufreq_voltage_enable_set(1);
		gpufreq_dbg("@ %s, mt_gpufreq_volt_switch2 fix frq = %d, fix "
			    "volt = %d, volt = %d\n",
			    __func__, mt_gpufreq_fixed_frequency,
			    mt_gpufreq_fixed_voltage, g_cur_gpu_volt);
		mt_gpufreq_volt_switch(g_cur_gpu_volt,
				       mt_gpufreq_fixed_voltage);
		g_cur_gpu_volt = mt_gpufreq_fixed_voltage;
	}
}

static ssize_t mt_gpufreq_fixed_freq_volt_proc_write(struct file *file,
						     const char __user *buffer,
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
#ifdef MTK_GPU_SPM
			mtk_gpu_spm_reset_fix();
#endif
		} else {
			g_cur_gpu_freq = _mt_gpufreq_get_cur_freq();
#ifndef MTK_GPU_SPM
			if (fixed_freq > g_cur_gpu_freq) {
				_mt_gpufreq_fixed_volt(fixed_volt);
				_mt_gpufreq_fixed_freq(fixed_freq);
			} else {
				_mt_gpufreq_fixed_freq(fixed_freq);
				_mt_gpufreq_fixed_volt(fixed_volt);
			}
#else
			if (0) {
				_mt_gpufreq_fixed_volt(fixed_volt);
				_mt_gpufreq_fixed_freq(fixed_freq);
			}
			{
				int i, found;

				for (i = 0; i < mt_gpufreqs_num; i++) {
					if (fixed_freq ==
					    mt_gpufreqs[i].gpufreq_khz) {
						mt_gpufreq_keep_opp_index = i;
						found = 1;
						break;
					}
				}
				mt_gpufreq_fixed_frequency = fixed_freq;
				mt_gpufreq_fixed_voltage = fixed_volt * 100;
				mtk_gpu_spm_fix_by_idx(
				    mt_gpufreq_keep_opp_index);
			}
#endif
		}
	} else
		gpufreq_warn("bad argument!! should be [enable fixed_freq "
			     "fixed_volt]\n");

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
static ssize_t mt_gpufreq_input_boost_proc_write(struct file *file,
						 const char __user *buffer,
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
			gpufreq_warn("bad argument!! should be 0 or 1 [0: "
				     "disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: "
			     "enable]\n");

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
static ssize_t mt_gpufreq_lpt_enable_proc_write(struct file *file,
						const char __user *buffer,
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

#define PROC_FOPS_RW(name)                                                     \
	static int mt_##name##_proc_open(struct inode *inode,                  \
					 struct file *file)                    \
	{                                                                      \
		return single_open(file, mt_##name##_proc_show,                \
				   PDE_DATA(inode));                           \
	}                                                                      \
	static const struct file_operations mt_##name##_proc_fops = {          \
		.owner = THIS_MODULE,                                          \
		.open = mt_##name##_proc_open,                                 \
		.read = seq_read,                                              \
		.llseek = seq_lseek,                                           \
		.release = single_release,                                     \
		.write = mt_##name##_proc_write,                               \
	}

#define PROC_FOPS_RO(name)                                                     \
	static int mt_##name##_proc_open(struct inode *inode,                  \
					 struct file *file)                    \
	{                                                                      \
		return single_open(file, mt_##name##_proc_show,                \
				   PDE_DATA(inode));                           \
	}                                                                      \
	static const struct file_operations mt_##name##_proc_fops = {          \
		.owner = THIS_MODULE,                                          \
		.open = mt_##name##_proc_open,                                 \
		.read = seq_read,                                              \
		.llseek = seq_lseek,                                           \
		.release = single_release,                                     \
	}

#define PROC_ENTRY(name)                                                       \
	{                                                                      \
		__stringify(name), &mt_##name##_proc_fops                      \
	}

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
		if (!proc_create(entries[i].name, 0664,
				 dir, entries[i].fops))
			gpufreq_err("@%s: create /proc/gpufreq/%s failed\n",
				    __func__, entries[i].name);
	}

	return 0;
}
#endif /* CONFIG_PROC_FS */

/**********************************
 * mediatek gpufreq initialization
 ***********************************/
#if 1
static int __init mt_gpufreq_init(void)
{
	int ret = 0;

#ifdef BRING_UP
	/* Skip driver init in bring up stage */
	gpufreq_err("@%s: ED2016: GPU DVFS bring up bypass\n", __func__);
	return 0;
#endif
	gpufreq_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_gpufreq_create_procfs())
		goto out;

#endif /* CONFIG_PROC_FS */

#if 0
	clk_cfg_0 = DRV_Reg32(CLK_CFG_0);
	clk_cfg_0 = (clk_cfg_0 & 0x00070000) >> 16;

	gpufreq_dbg("clk_cfg_0 = %d\n", clk_cfg_0);

	switch (clk_cfg_0) {
	case 0x5:		/* 476Mhz */
		g_cur_freq = GPU_MMPLL_D3;
		break;
	case 0x2:		/* 403Mhz */
		g_cur_freq = GPU_SYSPLL_D2;
		break;
	case 0x6:		/* 357Mhz */
		g_cur_freq = GPU_MMPLL_D4;
		break;
	case 0x4:		/* 312Mhz */
		g_cur_freq = GPU_UNIVPLL1_D2;
		break;
	case 0x7:		/* 286Mhz */
		g_cur_freq = GPU_MMPLL_D5;
		break;
	case 0x3:		/* 268Mhz */
		g_cur_freq = GPU_SYSPLL_D3;
		break;
	case 0x1:		/* 238Mhz */
		g_cur_freq = GPU_MMPLL_D6;
		break;
	case 0x0:		/* 156Mhz */
		g_cur_freq = GPU_UNIVPLL1_D4;
		break;
	default:
		break;
	}


	g_cur_freq_init_keep = g_cur_gpu_freq;
	gpufreq_dbg("g_cur_freq_init_keep = %d\n", g_cur_freq_init_keep);
#endif

/* register platform device/driver */
#if !defined(CONFIG_OF)
	ret = platform_device_register(&mt_gpufreq_pdev);
	if (ret) {
		gpufreq_err("fail to register gpufreq device @ %s()\n",
			    __func__);
		goto out;
	}
#endif
	ret = platform_driver_register(&mt_gpufreq_pdrv);
	if (ret) {
		gpufreq_err("fail to register gpufreq driver @ %s()\n",
			    __func__);
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

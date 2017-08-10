/*
 * Copyright (C) 2015-2016 MediaTek Inc.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
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
#include <linux/clk.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

/* #include <asm/system.h> */
#include <asm/uaccess.h>

/* #include "mach/mt_clkmgr.h" */
#include "mt_cpufreq.h"
#include "mt_gpufreq.h"
#include "mt-plat/upmu_common.h"
#include "mt-plat/sync_write.h"
#include "mach/mt_pmic_wrap.h"

#include "mach/mt_freqhopping.h"

#define STATIC_PWR_READY2USE
#ifdef STATIC_PWR_READY2USE
#include "mt_static_power.h"
#endif

#include "mach/mt_thermal.h"

/* DLPT */
#include "mach/mt_pbm.h"

#include <mt_spm.h>

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
/* #define MT_GPUFREQ_OC_PROTECT */

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

#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
#include "mt_vcorefs_manager.h"

#define MT_GPUFREQ_USE_BUCK_MT6353
#endif

/*
 * Define how to set up VGPU by
 * PMIC_WRAP
 * PMIC
 */

/**************************************************
 * Define register write function
 ***************************************************/
#define mt_gpufreq_reg_write(val, addr)		mt_reg_sync_writel((val), ((void *)addr))

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
#define GPU_DVFS_FREQT	 (806000)	/* KHz */
#define GPU_DVFS_FREQ0	 (728000)	/* KHz */
#define GPU_DVFS_FREQ0_1 (676000)	/* KHz */
#define GPU_DVFS_FREQ1	 (650000)	/* KHz */
#define GPU_DVFS_FREQ2	 (598000)	/* KHz */
#define GPU_DVFS_FREQ3	 (546000)	/* KHz */
#define GPU_DVFS_FREQ4	 (520000)	/* KHz */
#define GPU_DVFS_FREQ5	 (468000)	/* KHz */
#define GPU_DVFS_FREQ6	 (429000)	/* KHz */
#define GPU_DVFS_FREQ7	 (390000)	/* KHz */
#define GPU_DVFS_FREQ8	 (351000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL	(GPU_DVFS_FREQ8)

#define GPU_DVFS_VOLT0	 (112500)	/* mV x 100 */
#define GPU_DVFS_VOLT1	 (100000)	/* mV x 100 */
#define GPU_DVFS_VOLT2	 (93125)	/* mV x 100 */
#define GPU_DVFS_VOLT3	 (90000)	/* mV x 100 */

#define GPU_DVFS_CTRL_VOLT	 (2)

#define GPU_DVFS_PTPOD_DISABLE_VOLT	GPU_DVFS_VOLT1

#ifndef MT_GPUFREQ_USE_BUCK_MT6353
/*****************************************
 * PMIC settle time (us), should not be changed
 ******************************************/
#define PMIC_CMD_DELAY_TIME	 5
#define MIN_PMIC_SETTLE_TIME	25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)	\
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)	\
	(((((old_volt) - (new_volt)) * 2)  / 625) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_ON_OFF_DELAY_US       (400)
/* #define GPU_DVFS_PMIC_SETTLE_TIME (40) // us */

#define PMIC_BUCK_VGPU_VOSEL_ON		MT6351_PMIC_BUCK_VGPU_VOSEL_ON
#define PMIC_ADDR_VGPU_VOSEL_ON		MT6351_PMIC_BUCK_VGPU_VOSEL_ON_ADDR
#define PMIC_ADDR_VGPU_VOSEL_ON_MASK	MT6351_PMIC_BUCK_VGPU_VOSEL_ON_MASK
#define PMIC_ADDR_VGPU_VOSEL_ON_SHIFT	MT6351_PMIC_BUCK_VGPU_VOSEL_ON_SHIFT
#define PMIC_ADDR_VGPU_VOSEL_CTRL	MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_ADDR
#define PMIC_ADDR_VGPU_VOSEL_CTRL_MASK	MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_MASK
#define PMIC_ADDR_VGPU_VOSEL_CTRL_SHIFT	MT6351_PMIC_BUCK_VGPU_VOSEL_CTRL_SHIFT
#define PMIC_ADDR_VGPU_EN		MT6351_PMIC_BUCK_VGPU_EN_ADDR
#define PMIC_ADDR_VGPU_EN_MASK		MT6351_PMIC_BUCK_VGPU_EN_MASK
#define PMIC_ADDR_VGPU_EN_SHIFT		MT6351_PMIC_BUCK_VGPU_EN_SHIFT
#define PMIC_ADDR_VGPU_EN_CTRL		MT6351_PMIC_BUCK_VGPU_EN_CTRL_ADDR
#define PMIC_ADDR_VGPU_EN_CTRL_MASK	MT6351_PMIC_BUCK_VGPU_EN_CTRL_MASK
#define PMIC_ADDR_VGPU_EN_CTRL_SHIFT	MT6351_PMIC_BUCK_VGPU_EN_CTRL_SHIFT

#else /* MT_GPUFREQ_USE_BUCK_MT6353 - for Jade Minus and Rosa */

/*****************************************
 * PMIC settle time (us), should not be changed
 ******************************************/
#define PMIC_CMD_DELAY_TIME	 5
#define MIN_PMIC_SETTLE_TIME	25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)	\
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)	\
	(((((old_volt) - (new_volt)) * 2)  / 625) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_ON_OFF_DELAY_US       (400)
/* #define GPU_DVFS_PMIC_SETTLE_TIME (40) // us */

#define PMIC_BUCK_VPROC_VOSEL_ON			PMIC_BUCK_VPROC_VOSEL_ON
#define PMIC_ADDR_VPROC_VOSEL_ON			PMIC_BUCK_VPROC_VOSEL_ON_ADDR
#define PMIC_ADDR_VPROC_VOSEL_ON_MASK		PMIC_BUCK_VPROC_VOSEL_ON_MASK
#define PMIC_ADDR_VPROC_VOSEL_ON_SHIFT		PMIC_BUCK_VPROC_VOSEL_ON_SHIFT
#define PMIC_ADDR_VPROC_VOSEL_CTRL			PMIC_BUCK_VPROC_VOSEL_CTRL_ADDR
#define PMIC_ADDR_VPROC_VOSEL_CTRL_MASK		PMIC_BUCK_VPROC_VOSEL_CTRL_MASK
#define PMIC_ADDR_VPROC_VOSEL_CTRL_SHIFT	PMIC_BUCK_VPROC_VOSEL_CTRL_SHIFT
#define PMIC_ADDR_VPROC_EN					PMIC_BUCK_VPROC_EN_ADDR
#define PMIC_ADDR_VPROC_EN_MASK				PMIC_BUCK_VPROC_EN_MASK
#define PMIC_ADDR_VPROC_EN_SHIFT			PMIC_BUCK_VPROC_EN_SHIFT
#define PMIC_ADDR_VPROC_EN_CTRL				PMIC_BUCK_VPROC_EN_CTRL_ADDR
#define PMIC_ADDR_VPROC_EN_CTRL_MASK		PMIC_BUCK_VPROC_EN_CTRL_MASK
#define PMIC_ADDR_VPROC_EN_CTRL_SHIFT		PMIC_BUCK_VPROC_EN_CTRL_SHIFT

#endif /* !MT_GPUFREQ_USE_BUCK_MT6353 */

/* efuse */
#define GPUFREQ_EFUSE_INDEX	 (4)
#define EFUSE_MFG_SPD_BOND_SHIFT	(27)
#define EFUSE_MFG_SPD_BOND_MASK		(0xF)

#define MFG_PWR_STA_MASK	BIT(4)

/*
 * LOG
 */
#define TAG	 "[gpufreq] "

#define gpufreq_err(fmt, args...)	   \
	pr_err(TAG"[ERROR]"fmt, ##args)
#define gpufreq_warn(fmt, args...)	  \
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define gpufreq_info(fmt, args...)	  \
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


static sampler_func g_pFreqSampler;
static sampler_func g_pVoltSampler;

static gpufreq_power_limit_notify g_pGpufreq_power_limit_notify;
#ifdef MT_GPUFREQ_INPUT_BOOST
static gpufreq_input_boost_notify g_pGpufreq_input_boost_notify;
#endif

#if defined(CONFIG_OF)
static unsigned long apmixed_base;

#define APMIXED_NODE		"mediatek,apmixed"
#define APMIXED_BASE		(apmixed_base) /* 0x1000C000 */

#define GPUFREQ_NODE		"mediatek,mt6755-gpufreq"
static const struct of_device_id mt_gpufreq_of_match[] = {
	{.compatible = GPUFREQ_NODE,},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, mt_gpufreq_of_match);
#else /* !defined(CONFIG_OF) */
#define APMIXED_BASE		0x1000C000
#endif

#define APMIXED_MMPLL_CON1 (APMIXED_BASE + 0x244) /* 0x1000C244 */

/***************************
 * GPU DVFS OPP Table
 ****************************/
/* Segment1: Free */
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0, 1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT0, 2),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 3),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT1, 4),
	GPUOP(GPU_DVFS_FREQ6, GPU_DVFS_VOLT1, 5),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT1, 6),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT2, 7),
};

/* Segment2: 550M */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_1[] = {
	GPUOP(GPU_DVFS_FREQ3, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 1),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT1, 2),
	GPUOP(GPU_DVFS_FREQ6, GPU_DVFS_VOLT1, 3),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT1, 4),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT2, 5),
};

/* MT6755T: 800M Turbo */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_t[] = {
	GPUOP(GPU_DVFS_FREQT, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, 1),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0, 2),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT0, 3),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 4),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT1, 5),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT1, 6),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT2, 7),
};
#else
/* For E2 segment, GPU is sourced from Vcore */
/* E2 Segment1: 680M */
/* MT6750S */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e2_0[] = {
	GPUOP(GPU_DVFS_FREQ0_1, GPU_DVFS_VOLT1, 0),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 1),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT3, 2),
};

/* Segment2: 520M */
/* MT6750N */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e2_1[] = {
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 0),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT3, 1),
};

/* Segment3: 350M */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e2_2[] = {
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT1, 0),
};

/* fake Segment2: 520M */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e2_3[] = {
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 0),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT3, 1),
};

/* fake Segment3: 350M */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e2_4[] = {
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT3, 0),
};

/* MT6755S: 800M */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_t[] = {
	GPUOP(GPU_DVFS_FREQT, GPU_DVFS_VOLT0, 0),
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0, 1),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0, 2),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT0, 3),
	GPUOP(GPU_DVFS_FREQ4, GPU_DVFS_VOLT1, 4),
	GPUOP(GPU_DVFS_FREQ5, GPU_DVFS_VOLT1, 5),
	GPUOP(GPU_DVFS_FREQ7, GPU_DVFS_VOLT1, 6),
	GPUOP(GPU_DVFS_FREQ8, GPU_DVFS_VOLT2, 7),
};
#endif

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

static unsigned int g_cur_gpu_freq = 520000;	/* initial value */
static unsigned int g_cur_gpu_volt = 100000;	/* initial value */
static unsigned int g_cur_gpu_idx = 0xFF;
static unsigned int g_cur_gpu_OPPidx = 0xFF;

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
static unsigned int mt_gpufreq_keep_opp_frequency;
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_volt_state;
static unsigned int mt_gpufreq_fixed_frequency;
static unsigned int mt_gpufreq_fixed_voltage;

static unsigned int mt_gpufreq_volt_enable;
static unsigned int mt_gpufreq_volt_enable_state;
static bool mt_gpufreq_keep_volt_enable;
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
static unsigned int mt_gpufreq_dvfs_mmpll_spd_bond;

/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);
static DEFINE_MUTEX(mt_gpufreq_power_lock);

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;
static struct mt_gpufreq_power_table_info *mt_gpufreqs_power;
static struct mt_gpufreq_clk_t *mt_gpufreq_clk;
/* static struct mt_gpufreq_power_table_info *mt_gpufreqs_default_power; */

static bool mt_gpufreq_ptpod_disable;

static int mt_gpufreq_ptpod_disable_idx;

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_POLLING_TIMER
static int mt_gpufreq_low_batt_volume_timer_flag;
static DECLARE_WAIT_QUEUE_HEAD(mt_gpufreq_low_batt_volume_timer_waiter);
static struct hrtimer mt_gpufreq_low_batt_volume_timer;
static int mt_gpufreq_low_batt_volume_period_s = 1;
static int mt_gpufreq_low_batt_volume_period_ns;
struct task_struct *mt_gpufreq_low_batt_volume_thread = NULL;
#endif

static void mt_gpufreq_clock_switch(unsigned int freq_new);
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
/* voltage/freq set interfaces for vcore*/
static int mt_gpufreq_volt_switch_vcore(unsigned int new_oppidx);
static int mt_gpufreq_set_vcore(unsigned int freq_old, unsigned int freq_new,
			unsigned int new_oppidx);
/* voltage/freq set interfaces for pmic*/
static void mt_gpufreq_volt_switch_pmic(unsigned int volt_old, unsigned int volt_new);
static void mt_gpufreq_set_pmic(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new);
#else
/* voltage/freq set interfaces for pmic*/
static void mt_gpufreq_volt_switch_pmic(unsigned int volt_old, unsigned int volt_new);
static void mt_gpufreq_set_pmic(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new);
#endif
static unsigned int _mt_gpufreq_get_cur_volt(void);
static unsigned int _mt_gpufreq_get_cur_freq(void);
static void _mt_gpufreq_kick_pbm(int enable);

#ifndef DISABLE_PBM_FEATURE
static bool g_limited_pbm_ignore_state;
static unsigned int mt_gpufreq_pbm_limited_gpu_power;	/* PBM limit power */
static unsigned int mt_gpufreq_pbm_limited_index;	/* Limited frequency index for PBM */
#define GPU_OFF_SETTLE_TIME_MS		(100)
struct delayed_work notify_pbm_gpuoff_work;
#endif

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
static int g_last_gpu_dvs_result = 0x7F;
static unsigned int fake_segment;
#endif
static int g_is_rosa;
static unsigned int segment;

/* weak function declaration */
int __attribute__((weak)) get_immediate_gpu_wrap(void)
{
	return 40000;
}

int __attribute__((weak)) mtk_gpufreq_register(struct mt_gpufreq_power_table_info *freqs, int num)
{
	return 0;
}

void __attribute__((weak)) kicker_pbm_by_gpu(bool status, unsigned int loading, int voltage)
{
}

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_POLLING_TIMER
enum hrtimer_restart mt_gpufreq_low_batt_volume_timer_func(struct hrtimer *timer)
{
	mt_gpufreq_low_batt_volume_timer_flag = 1;
	wake_up_interruptible(&mt_gpufreq_low_batt_volume_timer_waiter);
	return HRTIMER_NORESTART;
}

int mt_gpufreq_low_batt_volume_thread_handler(void *unused)
{
	do {
		ktime_t ktime =
		    ktime_set(mt_gpufreq_low_batt_volume_period_s,
			      mt_gpufreq_low_batt_volume_period_ns);

		wait_event_interruptible(mt_gpufreq_low_batt_volume_timer_waiter,
					 mt_gpufreq_low_batt_volume_timer_flag != 0);
		mt_gpufreq_low_batt_volume_timer_flag = 0;

		gpufreq_dbg("@%s: begin\n", __func__);
		mt_gpufreq_low_batt_volume_check();

		hrtimer_start(&mt_gpufreq_low_batt_volume_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;
}

void mt_gpufreq_cancel_low_batt_volume_timer(void)
{
	hrtimer_cancel(&mt_gpufreq_low_batt_volume_timer);
}
EXPORT_SYMBOL(mt_gpufreq_cancel_low_batt_volume_timer);

void mt_gpufreq_start_low_batt_volume_timer(void)
{
	ktime_t ktime =
	    ktime_set(mt_gpufreq_low_batt_volume_period_s, mt_gpufreq_low_batt_volume_period_ns);
	hrtimer_start(&mt_gpufreq_low_batt_volume_timer, ktime, HRTIMER_MODE_REL);
}
EXPORT_SYMBOL(mt_gpufreq_start_low_batt_volume_timer);
#endif


/*************************************************************************************
 * Check GPU DVFS Efuse
 **************************************************************************************/
static unsigned int mt_gpufreq_get_dvfs_table_type(void)
{
	unsigned int type = 0;

#ifdef CONFIG_OF
	static const struct of_device_id gpu_ids[] = {
		{.compatible = "arm,malit6xx"},
		{.compatible = "arm,mali-midgard"},
		{ /* sentinel */ }
	};
	struct device_node *node;
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
	unsigned int gpu_speed = 0;
#endif
#endif

#ifdef CONFIG_ARCH_MT6755_TURBO
	gpufreq_info("check if GPU freq can be turbo by segment=0x%x\n", segment);
	if ((segment & 0xF0) == 0x20) {
		gpufreq_info("6755S or 6755T chip, segment=0x%x\n", segment);
		return 3;
	}
#endif

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
#ifdef CONFIG_OF
	node = of_find_matching_node(NULL, gpu_ids);
	if (!node)
		gpufreq_info("@%s: no GPU node found\n", __func__);
	else {
		if (!of_property_read_u32(node, "fake-segment", &fake_segment))
			gpufreq_info("@%s: fake_segment=%d\n", __func__, fake_segment);
		else
			gpufreq_info("@%s: no fake-segment property\n", __func__);
	}
#endif

	gpufreq_info("Segment code = 0x%x\n", segment);

	switch (segment) {
	case 0x82:
	case 0x86:
		type = 3; /* 6755s */
		break;
	case 0x42:
	case 0x46:
	case 0xC2:
	case 0xC6:
		type = 1;
		break;
	case 0x43:
	case 0x4B:
		type = 2;
		break;
	case 0x41:
	case 0x45:
	case 0xC1:
	case 0xC5:
	default:
		type = 0;
		break;
	}

	return type;
#else /* !MT_GPUFREQ_USE_BUCK_MT6353 */

	mt_gpufreq_dvfs_mmpll_spd_bond = (get_devinfo_with_index(GPUFREQ_EFUSE_INDEX) >>
			      EFUSE_MFG_SPD_BOND_SHIFT) & EFUSE_MFG_SPD_BOND_MASK;
	gpufreq_info("GPU frequency bounding from efuse = 0x%x\n", mt_gpufreq_dvfs_mmpll_spd_bond);

	/* No efuse or free run? use clock-frequency from device tree to determine GPU table type! */
	if (mt_gpufreq_dvfs_mmpll_spd_bond == 0) {
		/* Check segment_code if no efuse */
		if ((segment == 0x1) || (segment == 0x3) || (segment == 0x5) ||
		    (segment == 0x7)) {
			gpufreq_info("Set as 6755M since segment=0x%x\n", segment);
			return 1;	/* Segment2: 550M */
		}

#ifdef CONFIG_OF
		node = of_find_matching_node(NULL, gpu_ids);
		if (!node) {
			gpufreq_err("@%s: find GPU node failed\n", __func__);
			gpu_speed = 700;	/* default speed */
		} else {
			if (!of_property_read_u32(node, "clock-frequency", &gpu_speed)) {
				gpu_speed = gpu_speed / 1000 / 1000;	/* MHz */
			} else {
				gpufreq_err
				    ("@%s: missing clock-frequency property, use default GPU level\n",
				     __func__);
				gpu_speed = 700;	/* default speed */
			}
		}
		gpufreq_info("GPU clock-frequency from DT = %d MHz\n", gpu_speed);

		if (gpu_speed >= 700)
			type = 0;	/* Segment1: Free */
		else if (gpu_speed >= 550)
			type = 1;	/* Segment2: 550M */
#else
		gpufreq_err("@%s: Cannot get GPU speed from DT!\n", __func__);
		type = 0;
#endif
		return type;
	}

	switch (mt_gpufreq_dvfs_mmpll_spd_bond) {
	case 1:
	case 2:
		type = 0;	/* Segment1: Free */
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
		type = 1;	/* Segment2: 550M */
		break;
	}

	return type;
#endif
}

#ifdef MT_GPUFREQ_INPUT_BOOST
static struct task_struct *mt_gpufreq_up_task;

static int mt_gpufreq_input_boost_task(void *data)
{
	while (1) {
		gpufreq_dbg("@%s: begin\n", __func__);

		if (NULL != g_pGpufreq_input_boost_notify) {
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

static void mt_gpufreq_input_event(struct input_handle *handle, unsigned int type,
				   unsigned int code, int value)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1)
	    && (mt_gpufreq_input_boost_state == 1)) {
		gpufreq_dbg("@%s: accept.\n", __func__);

		/* if ((g_cur_gpu_freq < mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz) &&
		 * (g_cur_gpu_freq < mt_gpufreqs[g_limited_max_id].gpufreq_khz)) */
		/* { */
		wake_up_process(mt_gpufreq_up_task);
		/* } */
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
#endif

/*
 * Power table calculation
 */
static void mt_gpufreq_power_calculation(unsigned int idx, unsigned int freq,
					  unsigned int volt, unsigned int temp)
{
#define GPU_ACT_REF_POWER		1025	/* mW  */
#define GPU_ACT_REF_FREQ		728000	/* KHz */
#define GPU_ACT_REF_VOLT		100000	/* mV x 100 */

	unsigned int p_total = 0, p_dynamic = 0, p_leakage = 0, ref_freq = 0, ref_volt = 0;

	p_dynamic = GPU_ACT_REF_POWER;
	ref_freq = GPU_ACT_REF_FREQ;
	ref_volt = GPU_ACT_REF_VOLT;

	p_dynamic = p_dynamic *
	    ((freq * 100) / ref_freq) *
	    ((volt * 100) / ref_volt) * ((volt * 100) / ref_volt) / (100 * 100 * 100);

#ifdef STATIC_PWR_READY2USE
	p_leakage =
	    mt_spower_get_leakage(MT_SPOWER_GPU, (volt / 100), temp);
#else
	p_leakage = 130;
#endif

	p_total = p_dynamic + p_leakage;

	gpufreq_ver("%d: p_dynamic = %d, p_leakage = %d, p_total = %d\n",
		    idx, p_dynamic, p_leakage, p_total);

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

	volt = (pmic_wrap_value * 625) + 60000;

	gpufreq_dbg("@%s: volt = %d\n", __func__, volt);

	/* 1.39375V */
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

	reg_val = (volt - 60000) / 625;

	gpufreq_dbg("@%s: reg_val = %d\n", __func__, reg_val);

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

	mutex_lock(&mt_gpufreq_lock);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment != 0x82 && segment != 0x86) {
		/* slow down gpu freq for MT6738 since default freq is 520MHz */
		if (cur_freq > mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz)
			mt_gpufreq_clock_switch(mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz);

		/* keep in default freq since Vcore DVFS is not ready yet */
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (cur_freq == mt_gpufreqs[i].gpufreq_khz) {
				g_cur_gpu_OPPidx = i;
				gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
				_mt_gpufreq_kick_pbm(1);
				break;
			}
		}
	} else {
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
				mt_gpufreq_clock_switch(mt_gpufreqs[i].gpufreq_khz);
				g_cur_gpu_OPPidx = i;
				gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
				_mt_gpufreq_kick_pbm(1);
				break;
			}
		}

		/* Not found, set to LPM */
		if (i == mt_gpufreqs_num) {
			gpufreq_err
			    ("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
			     cur_volt / 100);
			g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
			mt_gpufreq_set_pmic(cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
				       cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
		}
	}
#else
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
			mt_gpufreq_clock_switch(mt_gpufreqs[i].gpufreq_khz);
			g_cur_gpu_OPPidx = i;
			gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
			_mt_gpufreq_kick_pbm(1);
			break;
		}
	}

	/* Not found, set to LPM */
	if (i == mt_gpufreqs_num) {
		gpufreq_err
		    ("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
		     cur_volt / 100);
		g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
		mt_gpufreq_set_pmic(cur_freq, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz,
			       cur_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);
	}
#endif

	g_cur_gpu_freq = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz;
	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
	g_cur_gpu_idx = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_idx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
}

static unsigned int mt_gpufreq_calc_pmic_settle_time(unsigned int volt_old, unsigned int volt_new)
{
	unsigned int delay = 100;

	if (volt_new == volt_old)
		return 0;
	else if (volt_new > volt_old)
		delay = PMIC_VOLT_UP_SETTLE_TIME(volt_old, volt_new);
	else
		delay = PMIC_VOLT_DOWN_SETTLE_TIME(volt_old, volt_new);

	if (delay < MIN_PMIC_SETTLE_TIME)
		delay = MIN_PMIC_SETTLE_TIME;

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

	if (enable == 0)
		if ((DRV_Reg32(PWR_STATUS) & MFG_PWR_STA_MASK) ||
				(DRV_Reg32(PWR_STATUS_2ND) & MFG_PWR_STA_MASK)) {
			gpufreq_dbg("@%s, VGPU should keep on since MTCMOS cannot be turned off!\n",
				    __func__);
			return -ENOSYS;
		}

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		ret = -ENOSYS;
		goto exit;
	}

	if (enable == mt_gpufreq_volt_enable_state) {
		ret = 0;
		goto exit;
	}

	if (enable == 0) {
		if (mt_gpufreq_ptpod_disable) {
			gpufreq_info("mt_gpufreq_ptpod_disable is true\n");
			ret = -ENOSYS;
			goto exit;
		}
		if (mt_gpufreq_keep_volt_enable) {
			gpufreq_dbg("mt_gpufreq_keep_volt_enable is true\n");
			ret = -ENOSYS;
			goto exit;
		}
	}

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment != 0x82 && segment != 0x86) {
		if ((is_vcorefs_can_work() < 0) ||
		    (mt_gpufreq_dvfs_table_type == 1 && fake_segment != 2) ||
		    (mt_gpufreq_dvfs_table_type == 2))
			goto end;

		if (enable == 0) {
			/* no need to unreq if current freq is not over GPU_DVFS_FREQ8 */
			if (mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_khz > GPU_DVFS_FREQ8)
				vcorefs_request_dvfs_opp(KIR_GPU, OPPI_UNREQ);
		} else {
			unsigned int cur_volt = _mt_gpufreq_get_cur_volt();
			int need_kick_pbm = 0;

			/* Vcore changed, need to kick PBM */
			if (cur_volt != mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt)
				need_kick_pbm = 1;

			/* Check need to raise Vcore or not */
			if (cur_volt >= mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt)
				goto done;

			ret = mt_gpufreq_volt_switch_vcore(g_cur_gpu_OPPidx);

			if (ret) {
				unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

				gpufreq_err("@%s: Set Vcore to %dmV failed! ret = %d, cur_freq = %d\n",
						__func__,
						mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt / 100,
						ret,
						cur_freq
				);

				/* Raise Vcore failed, set GPU freq to corresponding LV */
				if (cur_volt < mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt) {
					unsigned int i = 0;

					while (cur_volt != mt_gpufreqs[i].gpufreq_volt && i < mt_gpufreqs_num)
						i++;
					if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
						mt_gpufreq_clock_switch(mt_gpufreqs[i].gpufreq_khz);
						g_cur_gpu_OPPidx = i;
					}

					if (i == mt_gpufreqs_num) {
						gpufreq_err("@%s: Volt not found, set to lowest freq!\n",
								__func__);
						mt_gpufreq_clock_switch(
							mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz);
						g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
					}
				}
			}
done:
			if (need_kick_pbm)
				_mt_gpufreq_kick_pbm(1);
		}
	} else {
		unsigned int delay = 0;
		unsigned short rg_buck_gpu_en = 0, rg_da_qi_gpu_en = 0;

		if (enable == 1)
			pmic_config_interface(PMIC_ADDR_VPROC_EN, 0x1,
					      PMIC_ADDR_VPROC_EN_MASK,
					      PMIC_ADDR_VPROC_EN_SHIFT);	/* Set VGPU_EN[0] to 1 */
		else
			pmic_config_interface(PMIC_ADDR_VPROC_EN, 0x0,
					      PMIC_ADDR_VPROC_EN_MASK,
					      PMIC_ADDR_VPROC_EN_SHIFT);	/* Set VGPU_EN[0] to 0 */

		/* (g_cur_gpu_volt / 1250) + 26; */
		/* delay = mt_gpufreq_calc_pmic_settle_time(0, g_cur_gpu_volt); */
		delay = PMIC_VOLT_ON_OFF_DELAY_US;
		gpufreq_dbg("@%s: enable = %x, delay = %d\n", __func__, enable, delay);
		udelay(delay);
end:

#ifdef MT_GPUFREQ_AEE_RR_REC
		if (enable == 1)
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
						   (1 << GPU_DVFS_IS_VGPU_ENABLED));
		else
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
						   ~(1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

		rg_buck_gpu_en = pmic_get_register_value(PMIC_BUCK_VPROC_EN);
		rg_da_qi_gpu_en = pmic_get_register_value(PMIC_DA_QI_VPROC_EN);

		/* Error checking */
		if (enable == 1 && (rg_buck_gpu_en == 0 || rg_da_qi_gpu_en == 0)) {
			/* VGPU enable fail, dump info and trigger BUG() */
			int i = 0;

			gpufreq_err("@%s: enable=%x, delay=%d, buck_gpu_en=%u, da_qi_gpu_en=%u\n",
				    __func__, enable, delay, rg_buck_gpu_en,
				    rg_da_qi_gpu_en);

			/* read PMIC chip id via PMIC wrapper */
			for (i = 0; i < 10; i++) {
				gpufreq_err("@%s: PMIC_HWCID=0x%x\n", __func__,
					    pmic_get_register_value(PMIC_HWCID));
			}

			BUG();
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
	}
#else /* !MT_GPUFREQ_USE_BUCK_MT6353 */
	{
		unsigned int delay = 0;
		unsigned short rg_buck_gpu_en = 0, rg_da_qi_gpu_en = 0;

		if (enable == 1)
			pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x1,
					      PMIC_ADDR_VGPU_EN_MASK,
					      PMIC_ADDR_VGPU_EN_SHIFT);	/* Set VGPU_EN[0] to 1 */
		else
			pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x0,
					      PMIC_ADDR_VGPU_EN_MASK,
					      PMIC_ADDR_VGPU_EN_SHIFT);	/* Set VGPU_EN[0] to 0 */

		/* (g_cur_gpu_volt / 1250) + 26; */
		/* delay = mt_gpufreq_calc_pmic_settle_time(0, g_cur_gpu_volt); */
		delay = PMIC_VOLT_ON_OFF_DELAY_US;
		gpufreq_dbg("@%s: enable = %x, delay = %d\n", __func__, enable, delay);
		udelay(delay);

#ifdef MT_GPUFREQ_AEE_RR_REC
		if (enable == 1)
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() |
						   (1 << GPU_DVFS_IS_VGPU_ENABLED));
		else
			aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() &
						   ~(1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

		rg_buck_gpu_en = pmic_get_register_value(PMIC_BUCK_VGPU_EN);
		rg_da_qi_gpu_en = pmic_get_register_value(PMIC_DA_QI_VGPU_EN);

		/* Error checking */
		if (enable == 1 && (rg_buck_gpu_en == 0 || rg_da_qi_gpu_en == 0)) {
			/* VGPU enable fail, dump info and trigger BUG() */
			int i = 0;

			gpufreq_err("@%s: enable=%x, delay=%d, buck_gpu_en=%u, da_qi_gpu_en=%u\n",
				    __func__, enable, delay, rg_buck_gpu_en,
				    rg_da_qi_gpu_en);

			/* read PMIC chip id via PMIC wrapper */
			for (i = 0; i < 10; i++) {
				gpufreq_err("@%s: PMIC_HWCID=0x%x\n", __func__,
					    pmic_get_register_value(PMIC_HWCID));
			}

			BUG();
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
	}
#endif /* MT_GPUFREQ_USE_BUCK_MT6353 */

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
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
	mt_gpufreq_ptpod_disable = false;
	gpufreq_info("mt_gpufreq enabled by ptpod\n");
#else
	if (segment == 0x82 || segment == 0x86) {
		mt_gpufreq_ptpod_disable = false;
		gpufreq_info("mt_gpufreq enabled by ptpod\n");
	}
#endif
}
EXPORT_SYMBOL(mt_gpufreq_enable_by_ptpod);

/************************************************
 * DVFS disable API for PTPOD
 *************************************************/
void mt_gpufreq_disable_by_ptpod(void)
{
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
	int i = 0, volt_level_reached = 0, target_idx = 0;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	mt_gpufreq_ptpod_disable = true;
	gpufreq_info("mt_gpufreq disabled by ptpod\n");

	for (i = 0; i < mt_gpufreqs_num; i++) {
		/* VBoot = 1v for PTPOD */
		if (mt_gpufreqs_default[i].gpufreq_volt == GPU_DVFS_PTPOD_DISABLE_VOLT) {
			volt_level_reached = 1;
			if (i == (mt_gpufreqs_num - 1)) {
				target_idx = i;
				break;
			}
		} else {
			if (volt_level_reached == 1) {
				target_idx = i - 1;
				break;
			}
		}
	}

	mt_gpufreq_ptpod_disable_idx = target_idx;

	mt_gpufreq_voltage_enable_set(1);
	mt_gpufreq_target(target_idx);
#else
	if (segment == 0x82 || segment == 0x86) {
		int i = 0, volt_level_reached = 0, target_idx = 0;

		if (mt_gpufreq_ready == false) {
			gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
			return;
		}

		mt_gpufreq_ptpod_disable = true;
		gpufreq_info("mt_gpufreq disabled by ptpod\n");

		for (i = 0; i < mt_gpufreqs_num; i++) {
			/* VBoot = 1v for PTPOD */
			if (mt_gpufreqs_default[i].gpufreq_volt == GPU_DVFS_PTPOD_DISABLE_VOLT) {
				volt_level_reached = 1;
				if (i == (mt_gpufreqs_num - 1)) {
					target_idx = i;
					break;
				}
			} else {
				if (volt_level_reached == 1) {
					target_idx = i - 1;
					break;
				}
			}
		}

		mt_gpufreq_ptpod_disable_idx = target_idx;

		mt_gpufreq_voltage_enable_set(1);
		mt_gpufreq_target(target_idx);
	}
#endif
}
EXPORT_SYMBOL(mt_gpufreq_disable_by_ptpod);

/************************************************
 * API to switch back default voltage setting for GPU PTPOD disabled
 *************************************************/
void mt_gpufreq_restore_default_volt(void)
{
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
	int i;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < mt_gpufreqs_num; i++) {
		mt_gpufreqs[i].gpufreq_volt = mt_gpufreqs_default[i].gpufreq_volt;
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n", __func__, i,
			    mt_gpufreqs[i].gpufreq_volt);
	}

	mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

	mutex_unlock(&mt_gpufreq_lock);
#else
	if (segment == 0x82 || segment == 0x86) {
		int i;

		if (mt_gpufreq_ready == false) {
			gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
			return;
		}

		mutex_lock(&mt_gpufreq_lock);

		for (i = 0; i < mt_gpufreqs_num; i++) {
			mt_gpufreqs[i].gpufreq_volt = mt_gpufreqs_default[i].gpufreq_volt;
			gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n", __func__, i,
				    mt_gpufreqs[i].gpufreq_volt);
		}

		mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

		g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

		mutex_unlock(&mt_gpufreq_lock);
	}
#endif
}
EXPORT_SYMBOL(mt_gpufreq_restore_default_volt);

/* Set voltage because PTP-OD modified voltage table by PMIC wrapper */
unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size)
{
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment == 0x82 || segment == 0x86) {
		int i;			/* , idx; */
		/* unsigned long flags; */
		unsigned volt = 0;

		if (mt_gpufreq_ready == false) {
			gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
			return -ENOSYS;
		}

		mutex_lock(&mt_gpufreq_lock);

		for (i = 0; i < array_size; i++) {
			volt = mt_gpufreq_pmic_wrap_to_volt(pmic_volt[i]);
			mt_gpufreqs[i].gpufreq_volt = volt;
			gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n", __func__, i,
				    mt_gpufreqs[i].gpufreq_volt);
		}

		mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

		g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

		mutex_unlock(&mt_gpufreq_lock);
	}
	return 0;
#else
	int i;			/* , idx; */
	/* unsigned long flags; */
	unsigned volt = 0;

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return -ENOSYS;
	}

	mutex_lock(&mt_gpufreq_lock);

	for (i = 0; i < array_size; i++) {
		volt = mt_gpufreq_pmic_wrap_to_volt(pmic_volt[i]);
		mt_gpufreqs[i].gpufreq_volt = volt;
		gpufreq_dbg("@%s: mt_gpufreqs[%d].gpufreq_volt = %x\n", __func__, i,
			    mt_gpufreqs[i].gpufreq_volt);
	}

	mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt);

	g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;

	mutex_unlock(&mt_gpufreq_lock);

	return 0;
#endif
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
		return -ENOSYS;
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

	if ((temp > 0) && (temp < 125)) {
		for (i = 0; i < mt_gpufreqs_num; i++) {
			freq = mt_gpufreqs_power[i].gpufreq_khz;
			volt = mt_gpufreqs_power[i].gpufreq_volt;

			mt_gpufreq_power_calculation(i, freq, volt, temp);

			gpufreq_ver("update gpufreqs_power[%d]: khz=%u, volt=%u, power=%u\n",
				     i, mt_gpufreqs_power[i].gpufreq_khz,
				     mt_gpufreqs_power[i].gpufreq_volt,
				     mt_gpufreqs_power[i].gpufreq_power);
		}
	} else
		gpufreq_err("@%s: temp < 0 or temp > 125, NOT update power table!\n", __func__);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif

static void mt_setup_gpufreqs_power_table(int num)
{
	int i = 0, temp = 0;

	mt_gpufreqs_power = kzalloc((num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mt_gpufreqs_power == NULL)
		return;

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	gpufreq_dbg("@%s: temp = %d\n", __func__, temp);

	if ((temp < 0) || (temp > 125)) {
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

		gpufreq_info("gpufreqs_power[%d]: khz=%u, volt=%u, power=%u\n",
			     i, mt_gpufreqs_power[i].gpufreq_khz,
			     mt_gpufreqs_power[i].gpufreq_volt,
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

		gpufreq_dbg("freqs[%d]:khz=%u, volt=%u, idx=%u\n", i,
			    freqs[i].gpufreq_khz, freqs[i].gpufreq_volt,
			    freqs[i].gpufreq_idx);
	}

	mt_gpufreqs_num = num;

	g_limited_max_id = 0;
	g_limited_min_id = mt_gpufreqs_num - 1;

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

#if 1
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
#endif

static unsigned int mt_gpufreq_dds_calc(unsigned int khz, enum post_div_enum post_div)
{
	unsigned int dds = 0;

	gpufreq_dbg("@%s: request freq = %d, div = %d\n", __func__, khz, post_div);

	if ((khz >= 125000) && (khz <= 3000000)) {
		dds = (((khz / 100 * post_div*2) * 16384) / 26 + 5) / 10;
	} else {
		gpufreq_err("@%s: target khz(%d) out of range!\n", __func__, khz);
		BUG();
	}

	return dds;
}

static void gpu_dvfs_switch_to_syspll(bool on)
{
	clk_prepare_enable(mt_gpufreq_clk->clk_mux);
	if (on) {
		clk_set_parent(mt_gpufreq_clk->clk_mux, mt_gpufreq_clk->clk_sub_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
	} else {
		clk_set_parent(mt_gpufreq_clk->clk_mux, mt_gpufreq_clk->clk_main_parent);
		clk_disable_unprepare(mt_gpufreq_clk->clk_mux);
	}

	/* gpufreq_dbg("@%s: on = %d, CLK_CFG_1 = 0x%x, CLK_CFG_UPDATE = 0x%x\n",
			__func__, on, DRV_Reg32(CLK_CFG_1), DRV_Reg32(CLK_CFG_UPDATE)); */
}

static void mt_gpufreq_clock_switch_transient(unsigned int freq_new,  enum post_div_enum post_div)
{
#define SYSPLL_D3_FREQ	(364000)

	unsigned int cur_volt, cur_freq, dds, i, found = 0;
	unsigned int new_oppidx = g_cur_gpu_OPPidx;
	unsigned int tmp_idx = g_cur_gpu_OPPidx;
	unsigned int syspll_volt = mt_gpufreqs[0].gpufreq_volt;

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (freq_new == mt_gpufreqs[i].gpufreq_khz) {
			new_oppidx = i;
			break;
		}
	}
	dds = mt_gpufreq_dds_calc(freq_new, 1 << post_div);
	gpufreq_dbg("@%s: request GPU dds=0x%x, cur_volt=%u, cur_freq=%u, new_oppidx=%u\n",
			__func__, dds, cur_volt, cur_freq, new_oppidx);
	gpufreq_dbg("@%s: request APMIXED_MMPLL_CON1 = 0x%x\n",
			__func__, DRV_Reg32(APMIXED_MMPLL_CON1));
	if ((DRV_Reg32(APMIXED_MMPLL_CON1) & (0x7 << 24)) != ((post_div+1) << 24)) {
		gpufreq_dbg("@%s: switch to syspll\n", __func__);
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz < SYSPLL_D3_FREQ) {
				/* record idx since current voltage may not in DVFS table */
				if (i >= 1)
					tmp_idx = i - 1;
				else
					break;
				syspll_volt = mt_gpufreqs[tmp_idx].gpufreq_volt;
				found = 1;
				gpufreq_dbg("@%s: request GPU syspll_volt = %d\n",
					__func__, syspll_volt);
				break;
			}
		}

		if (found == 1) {
			/* ramp up volt for SYSPLL */
			if (cur_volt < syspll_volt) {
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
				mt_gpufreq_volt_switch_pmic(cur_volt, syspll_volt);
#else
				if (segment != 0x82 || segment != 0x86)
					mt_gpufreq_volt_switch_vcore(tmp_idx);
				else
					mt_gpufreq_volt_switch_pmic(cur_volt, syspll_volt);
#endif
			}

			/* Step1. Select to SYSPLL_D3 364MHz   */
			gpu_dvfs_switch_to_syspll(true);
			/* Step2. Modify APMIXED_MMPLL_CON1 */
			DRV_WriteReg32(APMIXED_MMPLL_CON1, (0x80000000) | ((post_div+1) << 24) | dds);
			/* Step3. Select back to MMPLL */
			gpu_dvfs_switch_to_syspll(false);
			DRV_SetReg32(APMIXED_MMPLL_CON1, 0x80000000);

			/* restore to cur_volt */
			if (cur_volt < syspll_volt) {
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
				mt_gpufreq_volt_switch_pmic(syspll_volt, cur_volt);
#else
				if (segment != 0x82 || segment != 0x86)
					mt_gpufreq_volt_switch_vcore(new_oppidx);
				else
					mt_gpufreq_volt_switch_pmic(syspll_volt, cur_volt);
#endif
			}
		} else {
			gpufreq_dbg("@%s: request GPU syspll_volt not found, khz = %d, volt = %d\n",
				__func__, mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz,
				mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_volt);
			BUG();
		}
	} else {
		gpufreq_dbg("@%s: no need to switch to SYSPLL\n", __func__);
		DRV_WriteReg32(APMIXED_MMPLL_CON1, 0x80000000 | ((post_div+1) << 24) | dds);
	}
}

static void mt_gpufreq_clock_switch(unsigned int freq_new)
{
	unsigned int dds;

	/* need clk switch if max freq is over 750M */
	if (mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz > 750000) {
		if (freq_new > 750000)
			mt_gpufreq_clock_switch_transient(freq_new, POST_DIV2);
		else if (freq_new < 250000)
			mt_gpufreq_clock_switch_transient(freq_new, POST_DIV8);
		else
			mt_gpufreq_clock_switch_transient(freq_new, POST_DIV4);
	} else {
		dds = mt_gpufreq_dds_calc(freq_new, 1 << POST_DIV4);
		mt_dfs_mmpll(dds);
		gpufreq_dbg("@%s: freq_new = %d, dds = 0x%x\n", __func__, freq_new, dds);
	}

	if (NULL != g_pFreqSampler)
		g_pFreqSampler(freq_new);

}

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
static int mt_gpufreq_volt_switch_vcore(unsigned int new_oppidx)
{
	gpufreq_dbg("@%s: new_oppidx = %d\n", __func__, new_oppidx);

	if ((is_vcorefs_can_work() < 0) ||
	    (mt_gpufreq_dvfs_table_type == 1 && fake_segment != 2) ||
	    (mt_gpufreq_dvfs_table_type == 1 && g_is_rosa) ||
	    (mt_gpufreq_dvfs_table_type == 2)) /* Segment2 & 3: use Vcore2 */
		return 0;

	BUG_ON(new_oppidx >= mt_gpufreqs_num);

	switch (mt_gpufreqs[new_oppidx].gpufreq_khz) {
	case GPU_DVFS_FREQ0_1:
	case GPU_DVFS_FREQ4:
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_PERF);
		break;
	case GPU_DVFS_FREQ8:
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_UNREQ);
		break;
	default:
		gpufreq_err("@%s: freq not found\n", __func__);
		g_last_gpu_dvs_result = 0x7F;
		break;
	}

	if (g_last_gpu_dvs_result == 0) {
		unsigned int volt_new = mt_gpufreq_get_cur_volt();

#ifdef MT_GPUFREQ_AEE_RR_REC
		aee_rr_rec_gpu_dvfs_vgpu(mt_gpufreq_volt_to_pmic_wrap(volt_new));
#endif

		if (NULL != g_pVoltSampler)
			g_pVoltSampler(volt_new);
	} else
		gpufreq_warn("@%s: GPU DVS failed, ret = %d\n", __func__, g_last_gpu_dvs_result);

	return g_last_gpu_dvs_result;
}
static void mt_gpufreq_volt_switch_pmic(unsigned int volt_old, unsigned int volt_new)
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

	/* Set VGPU_VOSEL_CTRL[1] to HW control */
	pmic_config_interface(PMIC_ADDR_VPROC_VOSEL_ON, reg_val,
			      PMIC_ADDR_VPROC_VOSEL_ON_MASK,
			      PMIC_ADDR_VPROC_VOSEL_ON_SHIFT);

	if (volt_new > volt_old) {
		delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new);
		gpufreq_dbg("@%s: delay = %d\n", __func__, delay);
		udelay(delay);
	}

	if (NULL != g_pVoltSampler)
		g_pVoltSampler(volt_new);
}
#else /* !MT_GPUFREQ_USE_BUCK_MT6353 */
static void mt_gpufreq_volt_switch_pmic(unsigned int volt_old, unsigned int volt_new)
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

	/* Set VGPU_VOSEL_CTRL[1] to HW control */
	pmic_config_interface(PMIC_ADDR_VGPU_VOSEL_ON, reg_val,
			      PMIC_ADDR_VGPU_VOSEL_ON_MASK,
			      PMIC_ADDR_VGPU_VOSEL_ON_SHIFT);

	if (volt_new > volt_old) {
		delay = mt_gpufreq_calc_pmic_settle_time(volt_old, volt_new);
		gpufreq_dbg("@%s: delay = %d\n", __func__, delay);
		udelay(delay);
	}

	if (NULL != g_pVoltSampler)
		g_pVoltSampler(volt_new);
}
#endif

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
#if 1
	unsigned int mmpll = 0;
	unsigned int freq = 0;
	unsigned int div = 0;
	unsigned int n_info = 0;

	mmpll = DRV_Reg32(APMIXED_MMPLL_CON1) & ~0x80000000;
	div = 1 << ((mmpll & (0x7 << 24)) >> 24);
	n_info = (mmpll & 0x1fffff) >> 14;
	freq = n_info * 26000 / div;

	gpufreq_dbg("mmpll = 0x%x, freq = %d\n", mmpll, freq);

	return freq;		/* KHz */
#else
	unsigned int gpu_freq = mt_get_mfgclk_freq();
	/* check freq meter */
	gpufreq_dbg("g_cur_gpu_freq = %d KHz, Meter = %d KHz\n", g_cur_gpu_freq, gpu_freq);
	return gpu_freq;
#endif
}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment == 0x82 || segment == 0x86)
		return mt_gpufreq_pmic_wrap_to_volt(pmic_get_register_value(PMIC_BUCK_VPROC_VOSEL_ON));
	else if ((mt_gpufreq_dvfs_table_type == 0) ||  /* Segment1: 680M use Vcore1 */
	    (fake_segment != 0))
		return vcorefs_get_curr_vcore() / 10;
	else /* Segment2 & 3: use Vcore2 */
		return GPU_DVFS_VOLT1;
#else
	return mt_gpufreq_pmic_wrap_to_volt(pmic_get_register_value(PMIC_BUCK_VGPU_VOSEL_ON));
#endif
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
}

/*****************************************
 * frequency ramp up and ramp down handler
 ******************************************/
/***********************************************************
 * [note]
 * 1. frequency ramp up need to wait voltage settle
 * 2. frequency ramp down do not need to wait voltage settle
 ************************************************************/
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
static int mt_gpufreq_set_vcore(unsigned int freq_old, unsigned int freq_new, unsigned int new_oppidx)
{
	int ret = 0;

	if (freq_new > freq_old) {
		if (is_vcorefs_can_work() >= 0) {
			ret = mt_gpufreq_volt_switch_vcore(new_oppidx);

			/* Do DFS only when Vcore was set to HPM successfully */
			if (ret)
				return ret;
		}
		mt_gpufreq_clock_switch(freq_new);
	} else {
		mt_gpufreq_clock_switch(freq_new);
		if (is_vcorefs_can_work() >= 0)
			mt_gpufreq_volt_switch_vcore(new_oppidx);
	}

	_mt_gpufreq_kick_pbm(1);

	return 0;
}
static void mt_gpufreq_set_pmic(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new)
{
	if (freq_new > freq_old) {
		/* if(volt_old != volt_new) // ??? */
		/* { */
		mt_gpufreq_volt_switch_pmic(volt_old, volt_new);
		/* } */

		mt_gpufreq_clock_switch(freq_new);
	} else {
		mt_gpufreq_clock_switch(freq_new);

		/* if(volt_old != volt_new) */
		/* { */
		mt_gpufreq_volt_switch_pmic(volt_old, volt_new);
		/* } */
	}

	g_cur_gpu_freq = freq_new;
	g_cur_gpu_volt = volt_new;

	_mt_gpufreq_kick_pbm(1);
}
#else /* !MT_GPUFREQ_USE_BUCK_MT6353 */
static void mt_gpufreq_set_pmic(unsigned int freq_old, unsigned int freq_new,
			   unsigned int volt_old, unsigned int volt_new)
{
	if (freq_new > freq_old) {
		/* if(volt_old != volt_new) // ??? */
		/* { */
		mt_gpufreq_volt_switch_pmic(volt_old, volt_new);
		/* } */

		mt_gpufreq_clock_switch(freq_new);
	} else {
		mt_gpufreq_clock_switch(freq_new);

		/* if(volt_old != volt_new) */
		/* { */
		mt_gpufreq_volt_switch_pmic(volt_old, volt_new);
		/* } */
	}

	g_cur_gpu_freq = freq_new;
	g_cur_gpu_volt = volt_new;

	_mt_gpufreq_kick_pbm(1);
}
#endif

/**********************************
 * gpufreq target callback function
 ***********************************/
/*************************************************
 * [note]
 * 1. handle frequency change request
 * 2. call mt_gpufreq_set_pmic/vcore to set target frequency
 **************************************************/
unsigned int mt_gpufreq_target(unsigned int idx)
{
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_idx, target_OPPidx;

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_warn("GPU DVFS not ready!\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (mt_gpufreq_volt_enable_state == 0) {
		gpufreq_dbg("mt_gpufreq_volt_enable_state == 0! return\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
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

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	/* only 1 OPP, no need to DVFS */
	if ((mt_gpufreq_dvfs_table_type == 2) || (fake_segment == 3)) {
		mutex_unlock(&mt_gpufreq_lock);
		return 0;
	}
#endif

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
#if 0
	/****************************************************
	 * If need to raise frequency, raise to max frequency
	 *****************************************************/
	if (target_freq > g_cur_freq) {
		target_freq = mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz;
		target_volt = mt_gpufreqs[g_gpufreq_max_id].gpufreq_volt;
		gpufreq_dbg("Need to raise frequency, raise to MAX frequency %d !\n", target_freq);
	}
#endif

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
		target_volt = mt_gpufreqs[mt_gpufreq_ptpod_disable_idx].gpufreq_volt;
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
	if (g_cur_gpu_freq == target_freq) {
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
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment != 0x82 && segment != 0x86) {
		if (!mt_gpufreq_set_vcore(g_cur_gpu_freq, target_freq, target_OPPidx)) {
			g_cur_gpu_idx = target_idx;
			g_cur_gpu_OPPidx = target_OPPidx;
			g_cur_gpu_freq = target_freq;
			g_cur_gpu_volt = mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt;
		}
	} else {
		mt_gpufreq_set_pmic(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt);

		g_cur_gpu_idx = target_idx;
		g_cur_gpu_OPPidx = target_OPPidx;
	}
#else
	mt_gpufreq_set_pmic(g_cur_gpu_freq, target_freq, g_cur_gpu_volt, target_volt);

	g_cur_gpu_idx = target_idx;
	g_cur_gpu_OPPidx = target_OPPidx;
#endif

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);

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

#define MT_GPUFREQ_OC_LIMIT_FREQ_1	 GPU_DVFS_FREQ0	/* no need to throttle when OC */
static unsigned int mt_gpufreq_oc_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_oc_limited_index_1;
static unsigned int mt_gpufreq_oc_limited_index;	/* Limited frequency index for oc */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
static unsigned int mt_gpufreq_low_battery_volume;

#define MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1	 GPU_DVFS_FREQ7
static unsigned int mt_gpufreq_low_bat_volume_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volume_limited_index_1;
static unsigned int mt_gpufreq_low_batt_volume_limited_index;	/* Limited frequency index for low battery volume */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
static unsigned int mt_gpufreq_low_battery_level;

#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1	 GPU_DVFS_FREQ0	/* no need to throttle when LV1 */
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2	 GPU_DVFS_FREQ7
static unsigned int mt_gpufreq_low_bat_volt_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volt_limited_index_1;
static unsigned int mt_gpufreq_low_bat_volt_limited_index_2;
static unsigned int mt_gpufreq_low_batt_volt_limited_index;	/* Limited frequency index for low battery voltage */
#endif

static unsigned int mt_gpufreq_thermal_limited_gpu_power;	/* thermal limit power */
/* limit frequency index array */
static unsigned int mt_gpufreq_power_limited_index_array[NR_IDX_POWER_LIMITED] = { 0 };

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

		gpufreq_dbg("mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			    mt_gpufreq_power_limited_index_array[i]);
	}

	g_limited_max_id = limited_index;
	gpufreq_dbg("Final limit frequency upper bound to id = %d, frequency = %d\n",
		     g_limited_max_id, mt_gpufreqs[g_limited_max_id].gpufreq_khz);

	if (NULL != g_pGpufreq_power_limit_notify)
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
		gpufreq_info("@%s: g_limited_oc_ignore_state == true!\n", __func__);
		return;
	}

	mt_gpufreq_oc_level = oc_level;

	/* BATTERY_OC_LEVEL_1: >= 7A, BATTERY_OC_LEVEL_0: < 7A */
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
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
			/* Limit GPU 400Mhz */
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
		gpufreq_warn("@%s: GPU DVFS not ready!\n", __func__);
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
	 * 3.5V HW issue int and is_low_battery=0 */
#if 0				/* no need to throttle when LV1 */
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

	gpufreq_dbg("Thermal limit frequency upper bound to id=%d, limited_power=%d\n",
		     mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED],
		     limited_power);

	mt_gpufreq_power_throttle_protect();

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
		gpufreq_info("@%s: g_limited_pbm_ignore_state == true!\n", __func__);
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

	if (NULL != g_pGpufreq_power_limit_notify)
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
#endif

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

#ifdef STATIC_PWR_READY2USE
	return mt_spower_get_leakage(MT_SPOWER_GPU, cur_vcore, temp);
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
	gpufreq_dbg("current GPU frequency OPP index is %d\n", g_cur_gpu_OPPidx);
	return g_cur_gpu_OPPidx;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq_index);

/************************************************
 * return current GPU frequency
 *************************************************/
unsigned int mt_gpufreq_get_cur_freq(void)
{
	gpufreq_dbg("current GPU frequency is %d MHz\n", g_cur_gpu_freq / 1000);
	return g_cur_gpu_freq;
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/************************************************
 * return current GPU voltage
 *************************************************/
unsigned int mt_gpufreq_get_cur_volt(void)
{
	return g_cur_gpu_volt;
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

	gpufreq_dbg("GPU freq SW/HW: %u/%u, g_cur_gpu_OPPidx: %d\n",
		    g_cur_gpu_freq, _mt_gpufreq_get_cur_freq(),
		    g_cur_gpu_OPPidx);

	return 0;
}

#if defined(CONFIG_OF)
static int mt_gpufreq_dts_map(struct platform_device *pdev)
{
	struct device_node *node;

	/* MFGCLK */
	node = of_find_matching_node(NULL, mt_gpufreq_of_match);
	if (!node) {
		gpufreq_err("@%s: find GPUFREQ node failed\n", __func__);
		BUG();
	}
	mt_gpufreq_clk = kzalloc(sizeof(struct mt_gpufreq_clk_t), GFP_KERNEL);
	if (NULL == mt_gpufreq_clk)
		return -ENOMEM;
	mt_gpufreq_clk->clk_mux = devm_clk_get(&pdev->dev, "clk_mux");
	if (IS_ERR(mt_gpufreq_clk->clk_mux)) {
		gpufreq_err("cannot get clock mux\n");
		return PTR_ERR(mt_gpufreq_clk->clk_mux);
	}
	mt_gpufreq_clk->clk_main_parent = devm_clk_get(&pdev->dev, "clk_main_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_main_parent)) {
		gpufreq_err("cannot get main clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_main_parent);
	}
	mt_gpufreq_clk->clk_sub_parent = devm_clk_get(&pdev->dev, "clk_sub_parent");
	if (IS_ERR(mt_gpufreq_clk->clk_sub_parent)) {
		gpufreq_err("cannot get sub clock parent\n");
		return PTR_ERR(mt_gpufreq_clk->clk_sub_parent);
	}

	/* apmixed */
	node = of_find_compatible_node(NULL, NULL, APMIXED_NODE);
	if (!node) {
		gpufreq_err("error: cannot find node " APMIXED_NODE);
		BUG();
	}
	apmixed_base = (unsigned long)of_iomap(node, 0);
	if (!apmixed_base) {
		gpufreq_err("error: cannot iomap " APMIXED_NODE);
		BUG();
	}

	return 0;
}
#else
static int mt_gpufreq_dts_map(struct platform_device *pdev)
{
	return 0;
}
#endif

static int mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
	unsigned int reg_val = 0;
	int i = 0;
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
#endif

	segment = get_devinfo_with_index(21) & 0xFF;
	g_is_rosa = (get_devinfo_with_index(21) & 0x00008000) >> 15;

	mt_gpufreq_dts_map(pdev);

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_POLLING_TIMER
	ktime_t ktime =
	    ktime_set(mt_gpufreq_low_batt_volume_period_s, mt_gpufreq_low_batt_volume_period_ns);
	hrtimer_init(&mt_gpufreq_low_batt_volume_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mt_gpufreq_low_batt_volume_timer.function = mt_gpufreq_low_batt_volume_timer_func;
#endif

	mt_gpufreq_dvfs_table_type = mt_gpufreq_get_dvfs_table_type();
	gpufreq_info("mt_gpufreq_dvfs_table_type: %u\n", mt_gpufreq_dvfs_table_type);

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
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (mt_gpufreq_dvfs_table_type == 0)	/* Segment1: 680M */
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_0));
	else if (mt_gpufreq_dvfs_table_type == 1) {
		if (fake_segment == 0) /* Segment2: 520M */
			mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_1,
						ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_1));
		else if (fake_segment == 2) /* fake Segment2: max 520M, flavor:_50 */
			mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_3,
						ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_3));
		else /* fake Segment2: max 350M, flavor:_50_k2_720p */
			mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_4,
						ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_4));
	} else if (mt_gpufreq_dvfs_table_type == 2) {
		if (fake_segment == 0) /* Segment3: 350M */
			mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_2,
						ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_2));
		else /* fake Segment3: max 350M, flavor:_38 */
			mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_4,
						ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_4));
	} else if (mt_gpufreq_dvfs_table_type == 3) {
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_t,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_t));
	} else
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e2_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e2_0));
#else /* !MT_GPUFREQ_USE_BUCK_MT6353 */
	if (mt_gpufreq_dvfs_table_type == 0)	/* Segment1: Free */
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
	else if (mt_gpufreq_dvfs_table_type == 1)	/* Segment2: 550M */
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_1,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_1));
	else if (mt_gpufreq_dvfs_table_type == 3)	/* 800M for turbo segment */
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_t,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_t));
	else
		mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0,
					ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
#endif

	/**********************
	 * setup PMIC init value
	 ***********************/
#ifndef MT_GPUFREQ_USE_BUCK_MT6353
	/* Set VGPU_VOSEL_CTRL[1] to HW control */
	pmic_config_interface(PMIC_ADDR_VGPU_VOSEL_CTRL, 0x1,
			      PMIC_ADDR_VGPU_VOSEL_CTRL_MASK,
			      PMIC_ADDR_VGPU_VOSEL_CTRL_SHIFT);
	/* Set VGPU_EN_CTRL[0] SW control to 0 */
	pmic_config_interface(PMIC_ADDR_VGPU_EN_CTRL, 0x0,
			      PMIC_ADDR_VGPU_EN_CTRL_MASK,
			      PMIC_ADDR_VGPU_EN_CTRL_SHIFT);
	/* Set VGPU_EN[0] to 1 */
	pmic_config_interface(PMIC_ADDR_VGPU_EN, 0x1,
			      PMIC_ADDR_VGPU_EN_MASK,
			      PMIC_ADDR_VGPU_EN_SHIFT);
#else
	if (segment == 0x82 || segment == 0x86) {
		/* Set VGPU_VOSEL_CTRL[1] to HW control */
		pmic_config_interface(PMIC_ADDR_VPROC_VOSEL_CTRL, 0x1,
				      PMIC_ADDR_VPROC_VOSEL_CTRL_MASK,
				      PMIC_ADDR_VPROC_VOSEL_CTRL_SHIFT);
		/* Set VGPU_EN_CTRL[0] SW control to 0 */
		pmic_config_interface(PMIC_ADDR_VPROC_EN_CTRL, 0x0,
				      PMIC_ADDR_VPROC_EN_CTRL_MASK,
				      PMIC_ADDR_VPROC_EN_CTRL_SHIFT);
		/* Set VGPU_EN[0] to 1 */
		pmic_config_interface(PMIC_ADDR_VPROC_EN, 0x1,
				      PMIC_ADDR_VPROC_EN_MASK,
				      PMIC_ADDR_VPROC_EN_SHIFT);
	}
#endif

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

	mt_gpufreq_volt_enable_state = 1;

#ifndef MT_GPUFREQ_USE_BUCK_MT6353
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
	pmic_read_interface(PMIC_ADDR_VGPU_EN, &reg_val,
			    PMIC_ADDR_VGPU_EN_MASK,
			    PMIC_ADDR_VGPU_EN_SHIFT);
	gpufreq_dbg("VGPU_EN[0] = %d\n", reg_val);
#else
	if (segment != 0x82 && segment != 0x86) {
		reg_val = 0;
	} else {
		/* Get VGPU_VOSEL_CTRL[1] */
		pmic_read_interface(PMIC_ADDR_VPROC_VOSEL_CTRL, &reg_val,
				    PMIC_ADDR_VPROC_VOSEL_CTRL_MASK,
				    PMIC_ADDR_VPROC_VOSEL_CTRL_SHIFT);
		gpufreq_dbg("VGPU_VOSEL_CTRL[1] = %d\n", reg_val);
		/* Get VGPU_EN_CTRL[0] */
		pmic_read_interface(PMIC_ADDR_VPROC_EN_CTRL, &reg_val,
				    PMIC_ADDR_VPROC_EN_CTRL_MASK,
				    PMIC_ADDR_VPROC_EN_CTRL_SHIFT);
		gpufreq_dbg("VGPU_EN_CTRL[0] = %d\n", reg_val);
		/* Get VGPU_EN[0] */
		pmic_read_interface(PMIC_ADDR_VPROC_EN, &reg_val,
				    PMIC_ADDR_VPROC_EN_MASK,
				    PMIC_ADDR_VPROC_EN_SHIFT);
		gpufreq_dbg("VGPU_EN[0] = %d\n", reg_val);
	}
#endif

	g_cur_freq_init_keep = g_cur_gpu_freq;

	/**********************
	 * setup initial frequency
	 ***********************/
	mt_gpufreq_set_initial();

	gpufreq_info("GPU current F=%u/%uKHz, V=%d/%dmV, cur_idx=%d, cur_OPPidx=%d\n",
		     _mt_gpufreq_get_cur_freq(), g_cur_gpu_freq,
		     _mt_gpufreq_get_cur_volt() / 100, g_cur_gpu_volt / 100,
		     g_cur_gpu_idx, g_cur_gpu_OPPidx);

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
		if (mt_gpufreqs[i].gpufreq_khz <= MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1) {
			mt_gpufreq_low_bat_volt_limited_index_1 = i;
			break;
		}
	}

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz <= MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2) {
			mt_gpufreq_low_bat_volt_limited_index_2 = i;
			break;
		}
	}

	register_low_battery_notify(&mt_gpufreq_low_batt_volt_callback, LOW_BATTERY_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz <= MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1) {
			mt_gpufreq_low_bat_volume_limited_index_1 = i;
			break;
		}
	}

	register_battery_percent_notify(&mt_gpufreq_low_batt_volume_callback,
					BATTERY_PERCENT_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_OC_PROTECT
	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (mt_gpufreqs[i].gpufreq_khz <= MT_GPUFREQ_OC_LIMIT_FREQ_1) {
			mt_gpufreq_oc_limited_index_1 = i;
			break;
		}
	}

	register_battery_oc_notify(&mt_gpufreq_oc_callback, BATTERY_OC_PRIO_GPU);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_POLLING_TIMER
	mt_gpufreq_low_batt_volume_thread = kthread_run(mt_gpufreq_low_batt_volume_thread_handler,
							0, "gpufreq low batt volume");
	if (IS_ERR(mt_gpufreq_low_batt_volume_thread))
		gpufreq_err("failed to create gpufreq_low_batt_volume thread\n");

	hrtimer_start(&mt_gpufreq_low_batt_volume_timer, ktime, HRTIMER_MODE_REL);
#endif

#ifndef DISABLE_PBM_FEATURE
	INIT_DEFERRABLE_WORK(&notify_pbm_gpuoff_work, mt_gpufreq_notify_pbm_gpuoff);
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

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_POLLING_TIMER
	kthread_stop(mt_gpufreq_low_batt_volume_thread);
	hrtimer_cancel(&mt_gpufreq_low_batt_volume_timer);
#endif

	return 0;
}


static const struct dev_pm_ops mt_gpufreq_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.restore_early = mt_gpufreq_pm_restore_early,
};

struct platform_device mt_gpufreq_pdev = {
	.name = "mt-gpufreq",
	.id = -1,
};

static struct platform_driver mt_gpufreq_pdrv = {
	.probe = mt_gpufreq_pdrv_probe,
	.remove = mt_gpufreq_pdrv_remove,
	.driver = {
		   .name = "mt-gpufreq",
		   .pm = &mt_gpufreq_pm_ops,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = mt_gpufreq_of_match,
#endif
		   },
};


#ifdef CONFIG_PROC_FS
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

	if (!kstrtoint(desc, 10, &debug)) {
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
	seq_printf(m, "g_limited_max_id = %d, g_limited_oc_ignore_state = %d\n", g_limited_max_id,
		   g_limited_oc_ignore_state);

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

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_oc_ignore_state = true;
		else if (ignore == 0)
			g_limited_oc_ignore_state = false;
		else
			gpufreq_warn
			    ("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
/****************************
 * show current limited by low batt volume
 *****************************/
static int mt_gpufreq_limited_low_batt_volume_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_low_batt_volume_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volume_ignore_state);

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

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_low_batt_volume_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volume_ignore_state = false;
		else
			gpufreq_warn
			    ("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
/****************************
 * show current limited by low batt volt
 *****************************/
static int mt_gpufreq_limited_low_batt_volt_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_low_batt_volt_ignore_state = %d\n",
		   g_limited_max_id, g_limited_low_batt_volt_ignore_state);

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

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_low_batt_volt_ignore_state = true;
		else if (ignore == 0)
			g_limited_low_batt_volt_ignore_state = false;
		else
			gpufreq_warn
			    ("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}
#endif

/****************************
 * show current limited by thermal
 *****************************/
static int mt_gpufreq_limited_thermal_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_thermal_ignore_state = %d\n",
		   g_limited_max_id, g_limited_thermal_ignore_state);

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

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_thermal_ignore_state = true;
		else if (ignore == 0)
			g_limited_thermal_ignore_state = false;
		else
			gpufreq_warn
			    ("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: not ignore, 1: ignore]\n");

	return count;
}

#ifndef DISABLE_PBM_FEATURE
/****************************
 * show current limited by PBM
 *****************************/
static int mt_gpufreq_limited_pbm_ignore_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "g_limited_max_id = %d, g_limited_oc_ignore_state = %d\n", g_limited_max_id,
		   g_limited_pbm_ignore_state);

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

	if (!kstrtouint(desc, 10, &ignore)) {
		if (ignore == 1)
			g_limited_pbm_ignore_state = true;
		else if (ignore == 0)
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

	if (!kstrtouint(desc, 10, &power))
		mt_gpufreq_thermal_protect(power);
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

	if (!kstrtouint(desc, 10, &power))
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

	if (!kstrtoint(desc, 10, &enabled)) {
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
		gpufreq_warn("bad argument!! argument should be \"1\" or \"0\"\n");

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

#if 0
		for (j = 0; j < ARRAY_SIZE(mt_gpufreqs_golden_power); j++) {
			if (mt_gpufreqs_golden_power[j].gpufreq_khz == mt_gpufreqs[i].gpufreq_khz) {
				p += sprintf(p, "power = %d\n",
					     mt_gpufreqs_golden_power[j].gpufreq_power);
				break;
			}
		}
#endif
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

	int i = 0;
	int fixed_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &fixed_freq)) {
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

				mt_gpufreq_voltage_enable_set(1);
				mt_gpufreq_target(mt_gpufreq_keep_opp_index);
			}

		}
	} else
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

	int i = 0;
	int max_freq = 0;
	int found = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &max_freq)) {
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

	seq_printf(m, "g_cur_gpu_freq = %d, g_cur_gpu_volt = %d\n", mt_gpufreq_get_cur_freq(),
		   mt_gpufreq_get_cur_volt());
	seq_printf(m, "g_cur_gpu_idx = %d, g_cur_gpu_OPPidx = %d\n", g_cur_gpu_idx,
		   g_cur_gpu_OPPidx);
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);

	for (i = 0; i < NR_IDX_POWER_LIMITED; i++)
		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			   mt_gpufreq_power_limited_index_array[i]);

	seq_printf(m, "_mt_gpufreq_get_cur_freq = %u KHz\n", _mt_gpufreq_get_cur_freq());
	seq_printf(m, "_mt_gpufreq_get_cur_volt = %u mV\n", _mt_gpufreq_get_cur_volt() / 100);
	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n", mt_gpufreq_volt_enable_state);
	seq_printf(m, "dvfs_table_type=%d, mmpll_spd_bond=%d, segment_code=0x%x\n",
		   mt_gpufreq_dvfs_table_type, mt_gpufreq_dvfs_mmpll_spd_bond,
		   (get_devinfo_with_index(21) & 0xFF));
	seq_printf(m, "mt_gpufreq_ptpod_disable_idx = %d\n", mt_gpufreq_ptpod_disable_idx);
#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment != 0x82 && segment != 0x86) {
		seq_printf(m, "fake_segment=%u, is_vcorefs_can_work=%d\n",
			fake_segment, is_vcorefs_can_work());
	}
#endif

	return 0;
}

/***************************
 * show current voltage enable status
 ****************************/
static int mt_gpufreq_volt_enable_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_volt_enable == 1)
		seq_puts(m, "gpufreq voltage is enabled\n");
	else if (mt_gpufreq_volt_enable == 2)
		seq_puts(m, "gpufreq voltage is always enabled\n");
	else
		seq_puts(m, "gpufreq voltage is disabled\n");

	return 0;
}

/*
 * control current vgpu buck
 */
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

	if (!kstrtoint(desc, 10, &enable)) {
		if (enable == 0) {
			mt_gpufreq_volt_enable = 0;
			mt_gpufreq_keep_volt_enable = false;
			mt_gpufreq_voltage_enable_set(0);
		} else if (enable == 1) {
			mt_gpufreq_volt_enable = 1;
			mt_gpufreq_keep_volt_enable = false;
			mt_gpufreq_voltage_enable_set(1);
		} else if (enable == 2) {
			mt_gpufreq_volt_enable = 2;
			mt_gpufreq_keep_volt_enable = true;
			mt_gpufreq_voltage_enable_set(1);
		} else
			gpufreq_warn("bad argument! should be 0/1/2 [0: disable, 1: enable, 2: always on]\n");
	} else
		gpufreq_warn("bad argument! should be 0/1/2 [0: disable, 1: enable, 2: always on]\n");

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
static ssize_t mt_gpufreq_fixed_freq_volt_proc_write(struct file *file, const char __user *buffer,
						     size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

#ifdef MT_GPUFREQ_USE_BUCK_MT6353
	if (segment != 0x82 && segment != 0x86)
		gpufreq_warn("not support fix volt since GPU is sourced from Vcore!\n");
	else {
		int fixed_freq = 0;
		int fixed_volt = 0;

		if (sscanf(desc, "%d %d", &fixed_freq, &fixed_volt) == 2) {
			if ((fixed_freq == 0) && (fixed_volt == 0)) {
				mt_gpufreq_fixed_freq_volt_state = false;
				mt_gpufreq_fixed_frequency = 0;
				mt_gpufreq_fixed_voltage = 0;
			} else {
				/* freq (KHz) */
				if ((fixed_freq >= GPUFREQ_LAST_FREQ_LEVEL)
				    && (fixed_freq <= GPU_DVFS_FREQT)) {
					mt_gpufreq_fixed_freq_volt_state = true;
					mt_gpufreq_fixed_frequency = fixed_freq;
					mt_gpufreq_fixed_voltage = g_cur_gpu_volt;
					mt_gpufreq_voltage_enable_set(1);
					mt_gpufreq_clock_switch(mt_gpufreq_fixed_frequency);
					g_cur_gpu_freq = mt_gpufreq_fixed_frequency;
				}
				/* volt (mV) */
				if (fixed_volt >= (mt_gpufreq_pmic_wrap_to_volt(0x0) / 100) &&
				    fixed_volt <= (mt_gpufreq_pmic_wrap_to_volt(0x7F) / 100)) {
					mt_gpufreq_fixed_freq_volt_state = true;
					mt_gpufreq_fixed_frequency = g_cur_gpu_freq;
					mt_gpufreq_fixed_voltage = fixed_volt * 100;
					mt_gpufreq_voltage_enable_set(1);
					mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreq_fixed_voltage);
					g_cur_gpu_volt = mt_gpufreq_fixed_voltage;
				}
			}
		} else
			gpufreq_warn("bad argument!! should be [enable fixed_freq fixed_volt]\n");
	}
#else
	{
		int fixed_freq = 0;
		int fixed_volt = 0;

		if (sscanf(desc, "%d %d", &fixed_freq, &fixed_volt) == 2) {
			if ((fixed_freq == 0) && (fixed_volt == 0)) {
				mt_gpufreq_fixed_freq_volt_state = false;
				mt_gpufreq_fixed_frequency = 0;
				mt_gpufreq_fixed_voltage = 0;
			} else {
				/* freq (KHz) */
				if ((fixed_freq >= GPUFREQ_LAST_FREQ_LEVEL)
				    && (fixed_freq <= GPU_DVFS_FREQT)) {
					mt_gpufreq_fixed_freq_volt_state = true;
					mt_gpufreq_fixed_frequency = fixed_freq;
					mt_gpufreq_fixed_voltage = g_cur_gpu_volt;
					mt_gpufreq_voltage_enable_set(1);
					mt_gpufreq_clock_switch(mt_gpufreq_fixed_frequency);
					g_cur_gpu_freq = mt_gpufreq_fixed_frequency;
				}
				/* volt (mV) */
				if (fixed_volt >= (mt_gpufreq_pmic_wrap_to_volt(0x0) / 100) &&
				    fixed_volt <= (mt_gpufreq_pmic_wrap_to_volt(0x7F) / 100)) {
					mt_gpufreq_fixed_freq_volt_state = true;
					mt_gpufreq_fixed_frequency = g_cur_gpu_freq;
					mt_gpufreq_fixed_voltage = fixed_volt * 100;
					mt_gpufreq_voltage_enable_set(1);
					mt_gpufreq_volt_switch_pmic(g_cur_gpu_volt, mt_gpufreq_fixed_voltage);
					g_cur_gpu_volt = mt_gpufreq_fixed_voltage;
				}
			}
		} else
			gpufreq_warn("bad argument!! should be [enable fixed_freq fixed_volt]\n");

	}
#endif
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

	if (!kstrtoint(desc, 10, &debug)) {
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
static int __init mt_gpufreq_init(void)
{
	int ret = 0;

	gpufreq_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_gpufreq_create_procfs())
		goto out;

#endif				/* CONFIG_PROC_FS */

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

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include <asm/uaccess.h>
#include <asm/io.h>

#include "mach/mt_clkmgr.h"
#include "mach/mt_freqhopping.h"
#include "mt_cpufreq.h"
#include "mt_gpufreq.h"
#include "mt-plat/sync_write.h"
#include "mt_static_power.h"
#include "mach/mt_thermal.h"
#include "mt-plat/upmu_common.h"
#include "mach/upmu_sw.h"
#include "mach/upmu_hw.h"
#include "mach/mt_pbm.h"
#include "mt_vcore_dvfs.h"
#include "mt_ptp.h"
#include "mt-plat/mt_devinfo.h"


/**************************************************
 * Define register write function
 ***************************************************/
#define mt_gpufreq_reg_write(val, addr)		mt_reg_sync_writel((val), ((void *)addr))

#define can_do_gpu_dvs()	(g_is_vcorefs_on == 1)

/***************************
 * Operate Point Definition
 ****************************/
#define GPUOP(khz, volt)	\
{						   \
	.gpufreq_khz = khz,	\
	.gpufreq_volt = volt,	\
}

/**************************
 * GPU DVFS OPP table setting
 ***************************/
#ifdef CONFIG_ARCH_MT6753
#define GPU_DVFS_FREQ0	 (598000)	/* KHz */
#define GPU_DVFS_FREQ1	 (448500)	/* KHz */
#define GPU_DVFS_FREQ2	 (299000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL	(GPU_DVFS_FREQ2)

#define GPU_DVFS_VOLT0	 (125000)	/* mV x 100 */
#define GPU_DVFS_VOLT1	 (115000)	/* mV x 100 */
#elif defined(CONFIG_ARCH_MT6735M)
#define GPU_DVFS_FREQ0_0   (650000)	/* KHz */
#define GPU_DVFS_FREQ0	 (549250)	/* KHz */
#define GPU_DVFS_FREQ1	 (448500)	/* KHz */
#define GPU_DVFS_FREQ2	 (299000)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL	(GPU_DVFS_FREQ2)

#define GPU_DVFS_VOLT0	 (125000)	/* mV x 100 */
#define GPU_DVFS_VOLT1	 (115000)	/* mV x 100 */
#define GPU_DVFS_VOLT2	 (105000)	/* mV x 100 */
#else
#define GPU_DVFS_FREQ0	 (598000)	/* KHz */
#define GPU_DVFS_FREQ0_1   (549250)	/* KHz */
#define GPU_DVFS_FREQ1	 (448500)	/* KHz */
#define GPU_DVFS_FREQ2	 (279500)	/* KHz */
#define GPUFREQ_LAST_FREQ_LEVEL	(GPU_DVFS_FREQ2)

#define GPU_DVFS_VOLT0	 (115000)	/* mV x 100 */
#define GPU_DVFS_VOLT1	 (105000)	/* mV x 100 */
#endif

/* efuse */
#define GPUFREQ_EFUSE_INDEX		 (3)
#define GPU_DEFAULT_MAX_FREQ_MHZ	(450)
#define GPU_DEFAULT_TYPE			(1)

/*
 * LOG
 */
#define TAG	 "[Power/gpufreq] "

#define gpufreq_err(fmt, args...)	   \
	pr_err(TAG"[ERROR]"fmt, ##args)
#define gpufreq_warn(fmt, args...)	  \
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define gpufreq_info(fmt, args...)	  \
	pr_warn(TAG""fmt, ##args)
#define gpufreq_dbg(fmt, args...)	   \
	do {								\
		if (mt_gpufreq_debug)		   \
			gpufreq_info(fmt, ##args);	 \
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

struct mt_gpufreq_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
};

/**************************
 * enable GPU DVFS count
 ***************************/
static int g_gpufreq_dvfs_disable_count;

static unsigned int g_cur_gpu_OPPidx = 0xFF;

static unsigned int mt_gpufreqs_num;
static struct mt_gpufreq_table_info *mt_gpufreqs;
static struct mt_gpufreq_table_info *mt_gpufreqs_default;

/***************************
 * GPU DVFS OPP Table
 ****************************/
#ifdef CONFIG_ARCH_MT6753
/* LV0: 600MHz */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* LV1: 450MHz */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_1[] = {
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};
#elif defined(CONFIG_ARCH_MT6735M)
/* LV0: add 550MHz for HD */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT2),
};

/* LV1: 450MHz for HPM */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_1[] = {
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT2),
};

/* LV2: 650MHz */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_2[] = {
	GPUOP(GPU_DVFS_FREQ0_0, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT1),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT2),
};
#else
/* LV0: 600MHz for HPM */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_0[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* LV1: 450MHz for HPM */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_1[] = {
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* LV2: 600MHz with 3 OPP lv */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_2[] = {
	GPUOP(GPU_DVFS_FREQ0, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};

/* LV3: 600MHz with 3 OPP lv */
static struct mt_gpufreq_table_info mt_gpufreq_opp_tbl_e1_3[] = {
	GPUOP(GPU_DVFS_FREQ0_1, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ1, GPU_DVFS_VOLT0),
	GPUOP(GPU_DVFS_FREQ2, GPU_DVFS_VOLT1),
};
#endif

static bool mt_gpufreq_ready;

/* In default settiing, freq_table[0] is max frequency, freq_table[num-1] is min frequency,*/
const unsigned int g_gpufreq_max_id = 0;
#if defined(CONFIG_ARCH_MT6735M) && defined(MT_GPUFREQ_VCOREFS_ENABLED)
static const unsigned int g_gpufreq_max_id_vcorefs_off = 1;
#endif

static int g_last_gpu_dvs_result = 0x7F;
static int g_is_vcorefs_on = 0xFF;

/* If not limited, it should be set to freq_table[0] (MAX frequency) */
static unsigned int g_limited_max_id;
static unsigned int g_limited_min_id;

static bool mt_gpufreq_debug;
static bool mt_gpufreq_pause;
static bool mt_gpufreq_keep_max_frequency_state;
static bool mt_gpufreq_keep_opp_frequency_state;
static unsigned int mt_gpufreq_keep_opp_frequency;
static unsigned int mt_gpufreq_keep_opp_index;
static bool mt_gpufreq_fixed_freq_state;
static unsigned int mt_gpufreq_fixed_frequency;

static unsigned int mt_gpufreq_volt_enable_state;
#ifdef MT_GPUFREQ_INPUT_BOOST
static unsigned int mt_gpufreq_input_boost_state = 1;
#endif


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


#ifdef MT_GPUFREQ_OC_PROTECT
static bool g_limited_oc_ignore_state;
static unsigned int mt_gpufreq_oc_level;

#define MT_GPUFREQ_OC_LIMIT_FREQ_1	 GPU_DVFS_FREQ3
static unsigned int mt_gpufreq_oc_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_oc_limited_index_1;
static unsigned int mt_gpufreq_oc_limited_index;	/* Limited frequency index for oc */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
static bool g_limited_low_batt_volume_ignore_state;
static unsigned int mt_gpufreq_low_battery_volume;

#define MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1	 GPU_DVFS_FREQ2
static unsigned int mt_gpufreq_low_bat_volume_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volume_limited_index_1;
static unsigned int mt_gpufreq_low_batt_volume_limited_index;	/* Limited frequency index for low battery volume */
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
static bool g_limited_low_batt_volt_ignore_state;
static unsigned int mt_gpufreq_low_battery_level;

#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1	 GPU_DVFS_FREQ2
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2	 GPU_DVFS_FREQ2
static unsigned int mt_gpufreq_low_bat_volt_limited_index_0;	/* unlimit frequency, index = 0. */
static unsigned int mt_gpufreq_low_bat_volt_limited_index_1;
static unsigned int mt_gpufreq_low_bat_volt_limited_index_2;
static unsigned int mt_gpufreq_low_batt_volt_limited_index;	/* Limited frequency index for low battery voltage */
#endif

/* static bool g_limited_power_ignore_state; */
static bool g_limited_thermal_ignore_state;
static unsigned int mt_gpufreq_thermal_limited_gpu_power;	/* thermal limit power */

#ifndef DISABLE_PBM_FEATURE
static bool g_limited_pbm_ignore_state;
static unsigned int mt_gpufreq_pbm_limited_gpu_power;	/* PBM limit power */
static unsigned int mt_gpufreq_pbm_limited_index;	/* Limited frequency index for PBM */
#endif

/* limit frequency index array */
static unsigned int mt_gpufreq_power_limited_index_array[NR_IDX_POWER_LIMITED] = { 0 };

static bool mt_gpufreq_opp_max_frequency_state;
static unsigned int mt_gpufreq_opp_max_frequency;
static unsigned int mt_gpufreq_opp_max_index;

static unsigned int mt_gpufreq_dvfs_table_type;
static unsigned int mt_gpufreq_dvfs_mmpll_spd_bond;

/* static DEFINE_SPINLOCK(mt_gpufreq_lock); */
static DEFINE_MUTEX(mt_gpufreq_lock);
static DEFINE_MUTEX(mt_gpufreq_power_lock);

static struct mt_gpufreq_power_table_info *mt_gpufreqs_power;
static unsigned int mt_gpufreqs_power_num;	/* run-time decided */


#ifdef MT_GPUFREQ_AEE_RR_REC
enum gpu_dvfs_state {
	GPU_DVFS_IS_DOING_DVFS = 0,
	GPU_DVFS_IS_VGPU_ENABLED,	/* always on */
};

static void _mt_gpufreq_aee_init(void)
{
	aee_rr_rec_gpu_dvfs_vgpu(0xFF);	/* no used */
	aee_rr_rec_gpu_dvfs_oppidx(0xFF);
	aee_rr_rec_gpu_dvfs_status(0xFE);
}
#endif


/**************************************
 * Efuse
 ***************************************/
static unsigned int _mt_gpufreq_get_dvfs_table_type(void)
{
	unsigned int type = 0;
	unsigned int gpu_550m_enable = is_have_550();
	unsigned int gpu_speed = 0;
#ifdef CONFIG_OF
	static const struct of_device_id gpu_ids[] = {
		{ .compatible = "arm,malit6xx" },
		{ .compatible = "arm,mali-midgard" },
		{ /* sentinel */ }
	};
	struct device_node *node;

	node = of_find_matching_node(NULL, gpu_ids);
	if (!node) {
		gpufreq_err("@%s: find GPU node failed\n", __func__);
		gpu_speed = GPU_DEFAULT_MAX_FREQ_MHZ;	// default speed
	} else {
		if (!of_property_read_u32(node, "clock-frequency", &gpu_speed)) {
			gpu_speed = gpu_speed / 1000 / 1000;	// MHz
		} else {
			gpufreq_err("@%s: missing clock-frequency property, use default GPU level\n", __func__);
			gpu_speed = GPU_DEFAULT_MAX_FREQ_MHZ;	// default speed
		}
	}
	gpufreq_info("GPU clock-frequency from DT = %d MHz\n", gpu_speed);
#endif

	mt_gpufreq_dvfs_mmpll_spd_bond = (get_devinfo_with_index(GPUFREQ_EFUSE_INDEX) >> 12) & 0x7;

	gpufreq_info("@%s: mmpll_spd_bond = 0x%x\n", __func__, mt_gpufreq_dvfs_mmpll_spd_bond);
	gpufreq_info("@%s: gpu_550m_enable = %d\n", __func__, gpu_550m_enable);

#ifdef CONFIG_ARCH_MT6753
	{
		unsigned int efuse_spare2 = (get_devinfo_with_index(5) >> 20) & 0x3;

		gpufreq_info("@%s: efuse_spare2 = 0x%x\n", __func__, efuse_spare2);

		/* 6753T check, spare2[21:20] should be 0x3 */
		if (mt_gpufreq_dvfs_mmpll_spd_bond == 0 && efuse_spare2 == 3)
			return 0;
	}
#else
	{
		unsigned int segment_code = (get_devinfo_with_index(47) >> 25) & 0x7F;

		gpufreq_info("@%s: segment_code = 0x%x\n", __func__, segment_code);

#ifdef CONFIG_ARCH_MT6735
		if (gpu_speed == 550)
			return 3;	/* SW config 37T to 35M+ */
#endif

		switch (segment_code) {
		case 0x41:
		case 0x42:
		case 0x43:
			return 2;	/* 37T: 600M with 3 OPP*/
		case 0x49:
			return 1;	/* 37M: 450M */
		case 0x4A:
		case 0x4B:
			return 0;	/* 35M+: 550M */
		case 0x51:
			return 0;	/* 37: 600M */
		case 0x52:
		case 0x53:
			if (gpu_speed == 550)
				return 0;	/* SW config to 35M+ */
			else
				return 2;	/* 35P+ 650M */
		default:
			break;
		}
	}
#endif

#ifdef CONFIG_ARCH_MT6735M
#ifndef MT_GPUFREQ_VCOREFS_ENABLED
	return 1;	/* no 550MHz if Vcore DVFS not built-in */
#endif

	/* for 35P */
	if (mt_gpufreq_dvfs_mmpll_spd_bond == 0 && gpu_550m_enable != 0)
		return 0;
#endif

	/* No efuse,  use clock-frequency from device tree to determine GPU table type! */
	if (mt_gpufreq_dvfs_mmpll_spd_bond == 0) {
		if (gpu_speed > GPU_DEFAULT_MAX_FREQ_MHZ)
			type = 0;	/* 600M */
		else
			type = GPU_DEFAULT_TYPE;

		return type;
	}

	switch (mt_gpufreq_dvfs_mmpll_spd_bond) {
	case 1:
	case 2:
#ifdef CONFIG_ARCH_MT6735M
	case 3:	/* 600M */
	case 4:	/* 550M */
		if (gpu_550m_enable)
			type = 0;
		else
			type = GPU_DEFAULT_TYPE;
		break;
#else
	case 3:	/* 600M */
		type = 0;
		break;
	case 4:	/* 550M */
		type = GPU_DEFAULT_TYPE;
		break;
#endif
	case 5:
	case 6:
	case 7:
	default:
		type = GPU_DEFAULT_TYPE;
		break;
	}

	return type;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
static struct task_struct *mt_gpufreq_up_task;

static int _mt_gpufreq_input_boost_task(void *data)
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


/**************************************
 * Input boost
 ***************************************/
static void _mt_gpufreq_input_event(struct input_handle *handle, unsigned int type,
					unsigned int code, int value)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if ((type == EV_KEY) && (code == BTN_TOUCH) && (value == 1)
		&& (mt_gpufreq_input_boost_state == 1)) {
		gpufreq_dbg("@%s: accept.\n", __func__);

		wake_up_process(mt_gpufreq_up_task);
	}
}

static int _mt_gpufreq_input_connect(struct input_handler *handler,
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

static void _mt_gpufreq_input_disconnect(struct input_handle *handle)
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
	.event = _mt_gpufreq_input_event,
	.connect = _mt_gpufreq_input_connect,
	.disconnect = _mt_gpufreq_input_disconnect,
	.name = "gpufreq_ib",
	.id_table = mt_gpufreq_ids,
};
#endif

/**************************************
 * Random seed generated for test
 ***************************************/
#ifdef MT_GPU_DVFS_RANDOM_TEST
static int _mt_gpufreq_idx_get(int num)
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
static unsigned int _mt_gpufreq_pmic_wrap_to_volt(unsigned int pmic_wrap_value)
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
#if defined(MT_GPUFREQ_AEE_RR_REC) || defined(MT_GPUFREQ_PERFORMANCE_TEST)
static unsigned int _mt_gpufreq_volt_to_pmic_wrap(unsigned int volt)
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
#endif

/**************************************
 * Power table calculation
 ***************************************/
static void _mt_gpufreq_power_calculation(unsigned int idx, unsigned int freq, unsigned int volt,
					  unsigned int temp)
{
#ifdef CONFIG_ARCH_MT6753
#define GPU_ACT_REF_POWER		1088	/* mW  */
#elif defined(CONFIG_ARCH_MT6735M)
#define GPU_ACT_REF_POWER		360		/* mW  */
#else
#define GPU_ACT_REF_POWER		720		/* mW  */
#endif
#define GPU_ACT_REF_FREQ		450000	/* KHz */
#define GPU_ACT_REF_VOLT		115000	/* mV x 100 */

	unsigned int p_total = 0, p_dynamic = 0, p_leakage = 0, ref_freq = 0, ref_volt = 0;

	p_dynamic = GPU_ACT_REF_POWER;
	ref_freq = GPU_ACT_REF_FREQ;
	ref_volt = GPU_ACT_REF_VOLT;

	p_dynamic = p_dynamic *
		((freq * 100) / ref_freq) *
		((volt * 100) / ref_volt) * ((volt * 100) / ref_volt) / (100 * 100 * 100);

	p_leakage = mt_spower_get_leakage(MT_SPOWER_VCORE, (volt / 100), temp);

	p_total = p_dynamic + p_leakage;

	gpufreq_ver("%d: p_dynamic = %d, p_leakage = %d, p_total = %d\n",
				idx, p_dynamic, p_leakage, p_total);

	mt_gpufreqs_power[idx].gpufreq_power = p_total;
}


#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
static void _mt_update_gpufreqs_power_table(void)
{
	int i = 0, temp = 0;
	unsigned int freq, volt;

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready\n", __func__);
		return;
	}

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif
	gpufreq_ver("@%s, temp = %d\n", __func__, temp);

	mutex_lock(&mt_gpufreq_lock);

	if ((temp > 0) && (temp < 125)) {
		for (i = 0; i < mt_gpufreqs_power_num; i++) {
			freq = mt_gpufreqs_power[i].gpufreq_khz;
			volt = mt_gpufreqs_power[i].gpufreq_volt;

			_mt_gpufreq_power_calculation(i, freq, volt, temp);

			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_khz = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_khz);
			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_volt = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_volt);
			gpufreq_ver("update mt_gpufreqs_power[%d].gpufreq_power = %d\n", i,
					mt_gpufreqs_power[i].gpufreq_power);
		}
	} else
		gpufreq_warn("@%s: temp < 0 or temp > 125, NOT update power table!\n", __func__);

	mutex_unlock(&mt_gpufreq_lock);
}
#endif

static void _mt_setup_gpufreqs_power_table(int num)
{
	int i, j, k, temp = 0;
	unsigned int freq, volt;

	mt_gpufreqs_power = kzalloc((num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mt_gpufreqs_power == NULL) {
		/* gpufreq_err("GPU power table memory allocation fail\n"); */
		return;
	}

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	gpufreq_info("@%s: temp = %d\n", __func__, temp);

	if ((temp < 0) || (temp > 125)) {
		gpufreq_warn("@%s: temp < 0 or temp > 125!\n", __func__);
		temp = 65;
	}

	/* fill-in freq and volt in power table */
	for (i = 0, k = 0; i < mt_gpufreqs_num; i++) {	/* freq */
		for (j = 0; j <= i; j++) {	/* volt */
			mt_gpufreqs_power[k].gpufreq_khz = mt_gpufreqs[i].gpufreq_khz;
			mt_gpufreqs_power[k].gpufreq_volt = mt_gpufreqs[j].gpufreq_volt;
			k++;

			if (k == num)
				break;
		}

		if (k == num)
			break;
	}

	for (i = 0; i < num; i++) {
		freq = mt_gpufreqs_power[i].gpufreq_khz;
		volt = mt_gpufreqs_power[i].gpufreq_volt;

		_mt_gpufreq_power_calculation(i, freq, volt, temp);

		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_khz = %u\n", i, freq);
		gpufreq_info("mt_gpufreqs_power[%d].gpufreq_volt = %u\n", i, volt);
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
static int _mt_setup_gpufreqs_table(struct mt_gpufreq_table_info *freqs, int num)
{
	int i = 0;

	mt_gpufreqs = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	mt_gpufreqs_default = kzalloc((num) * sizeof(*freqs), GFP_KERNEL);
	if (mt_gpufreqs == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mt_gpufreqs[i].gpufreq_khz = freqs[i].gpufreq_khz;
		/* Apply PTPOD result */
		mt_gpufreqs[i].gpufreq_volt =
			_mt_gpufreq_pmic_wrap_to_volt(get_vcore_ptp_volt(freqs[i].gpufreq_volt * 10));

		mt_gpufreqs_default[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mt_gpufreqs_default[i].gpufreq_volt = freqs[i].gpufreq_volt;

		gpufreq_info("mt_gpufreqs[%d].gpufreq_khz = %u\n", i, mt_gpufreqs[i].gpufreq_khz);
		gpufreq_info("mt_gpufreqs[%d].gpufreq_volt = %u\n", i, mt_gpufreqs[i].gpufreq_volt);
	}

	mt_gpufreqs_num = num;

	g_limited_max_id = 0;
	g_limited_min_id = mt_gpufreqs_num - 1;

	/* setup power table */
	mt_gpufreqs_power_num = (mt_gpufreqs_num) * (mt_gpufreqs_num + 1) / 2;
	_mt_setup_gpufreqs_power_table(mt_gpufreqs_power_num);

	return 0;
}

/*****************************
 * set GPU DVFS status
 ******************************/
static int _mt_gpufreq_state_set(int enabled)
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

static unsigned int _mt_gpufreq_dds_calc(unsigned int khz)
{
	unsigned int dds = 0;

	if ((khz >= 250250) && (khz <= 747500))
		dds = ((khz * 4 / 1000) * 8192) / 13;
	else {
		gpufreq_err("@%s: target khz(%d) out of range!\n", __func__, khz);
		BUG();
	}

	return dds;
}

static void _mt_gpufreq_set_cur_freq(unsigned int freq_new)
{
	unsigned int dds = _mt_gpufreq_dds_calc(freq_new);

	mt_dfs_mmpll(dds);

	gpufreq_dbg("@%s: freq_new = %d, dds = 0x%x\n", __func__, freq_new, dds);

	if (NULL != g_pFreqSampler)
		g_pFreqSampler(freq_new);
}

static int _mt_gpufreq_set_cur_volt(unsigned int new_oppidx)
{
	gpufreq_dbg("@%s: new_oppidx = %d\n", __func__, new_oppidx);

	BUG_ON(new_oppidx >= mt_gpufreqs_num);

#ifdef MT_GPUFREQ_VCOREFS_ENABLED
	switch (mt_gpufreqs[new_oppidx].gpufreq_khz) {
#ifdef CONFIG_ARCH_MT6735M
	case GPU_DVFS_FREQ0_0:
	case GPU_DVFS_FREQ0:
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_PERF_ULTRA);
#else
	case GPU_DVFS_FREQ0:
#ifdef CONFIG_ARCH_MT6735
	case GPU_DVFS_FREQ0_1:
#endif
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_PERF);
#endif
		break;
	case GPU_DVFS_FREQ1:
#ifdef CONFIG_ARCH_MT6753
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_LOW_PWR);
#else
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_PERF);
#endif
		break;
	case GPU_DVFS_FREQ2:
		g_last_gpu_dvs_result = vcorefs_request_dvfs_opp(KIR_GPU, OPPI_LOW_PWR);
		break;
	default:
		gpufreq_err("@%s: freq not found\n", __func__);
		g_last_gpu_dvs_result = 0x7F;
		break;
	}
#endif

	if (g_last_gpu_dvs_result == 0) {
		unsigned int volt_new = mt_gpufreq_get_cur_volt();

#ifdef MT_GPUFREQ_AEE_RR_REC
		aee_rr_rec_gpu_dvfs_vgpu(_mt_gpufreq_volt_to_pmic_wrap(volt_new));
#endif

		if (NULL != g_pVoltSampler)
			g_pVoltSampler(volt_new);
	} else
		gpufreq_warn("@%s: GPU DVS failed, ret = %d\n", __func__, g_last_gpu_dvs_result);

	return g_last_gpu_dvs_result;
}

static unsigned int _mt_gpufreq_get_cur_freq(void)
{
	unsigned int mmpll = 0;
	unsigned int freq = 0;

	mmpll = __raw_readl(MMPLL_CON1) & ~0x80000000;

	if ((mmpll >= 0x0209A000) && (mmpll <= 0x021CC000)) {
		freq = 0x0209A000;
		freq = 250250 + (((mmpll - freq) / 0x2000) * 3250);
	} else if ((mmpll >= 0x010E6000) && (mmpll <= 0x010F4000)) {
		freq = 0x010E6000;
		freq = 747500 + (((mmpll - freq) / 0x2000) * 6500);
	} else {
		gpufreq_err("Invalid mmpll value = 0x%x\n", mmpll);
		BUG();
	}

	gpufreq_dbg("mmpll = 0x%x, freq = %d\n", mmpll, freq);

	return freq;	/* KHz */
}

static unsigned int _mt_gpufreq_get_cur_volt(void)
{
#ifdef MT_GPUFREQ_VCOREFS_ENABLED
	g_is_vcorefs_on = is_vcorefs_can_work();

	if (!can_do_gpu_dvs())
		return _mt_gpufreq_pmic_wrap_to_volt(pmic_get_register_value(PMIC_VCORE1_VOSEL_ON));
	else
		return vcorefs_get_curr_voltage() / 10;
#else
	return _mt_gpufreq_pmic_wrap_to_volt(pmic_get_register_value(PMIC_VCORE1_VOSEL_ON));
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
		for (i = 0; i < mt_gpufreqs_power_num; i++) {
			if (mt_gpufreqs_power[i].gpufreq_khz == cur_freq) {
				/* record idx since current voltage may not in DVFS table */
				tmp_idx = i;

				if (mt_gpufreqs_power[i].gpufreq_volt == cur_volt) {
					power = mt_gpufreqs_power[i].gpufreq_power;
					found = 1;
					kicker_pbm_by_gpu(true, power, cur_volt / 100);
					gpufreq_dbg("@%s: request GPU power = %d, cur_volt = %d, cur_freq = %d\n",
						 __func__, power, cur_volt / 100, cur_freq);
					return;
				}
			}
		}

		if (!found) {
			gpufreq_dbg("@%s: tmp_idx = %d\n", __func__, tmp_idx);

			if (tmp_idx != -1 && tmp_idx < mt_gpufreqs_power_num) {
				/* use freq to found corresponding power budget */
				power = mt_gpufreqs_power[tmp_idx].gpufreq_power;
				kicker_pbm_by_gpu(true, power, cur_volt / 100);
				gpufreq_dbg("@%s: request GPU power = %d, cur_volt = %d, cur_freq = %d\n",
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
static int _mt_gpufreq_set(unsigned int freq_old, unsigned int freq_new, unsigned int new_oppidx)
{
	int ret = 0;

#ifdef MT_GPUFREQ_VCOREFS_ENABLED
	g_is_vcorefs_on = is_vcorefs_can_work();
#endif

	if (freq_new > freq_old) {
		/* TODO: need to handle fix LPM case?? */
		if (can_do_gpu_dvs()) {
			ret = _mt_gpufreq_set_cur_volt(new_oppidx);

			/* Do DFS only when Vcore was set to HPM successfully */
			if (ret)
				return ret;
		}
		_mt_gpufreq_set_cur_freq(freq_new);
	} else {
		_mt_gpufreq_set_cur_freq(freq_new);
		if (can_do_gpu_dvs())
			_mt_gpufreq_set_cur_volt(new_oppidx);
	}

	_mt_gpufreq_kick_pbm(1);

	return 0;
}

/* Set frequency and voltage at driver probe function */
static void _mt_gpufreq_set_initial(void)
{
	unsigned int cur_volt = 0, cur_freq = 0;
	int i = 0;

	mutex_lock(&mt_gpufreq_lock);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	cur_volt = _mt_gpufreq_get_cur_volt();
	cur_freq = _mt_gpufreq_get_cur_freq();

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
			_mt_gpufreq_set_cur_freq(mt_gpufreqs[i].gpufreq_khz);
			g_cur_gpu_OPPidx = i;
			gpufreq_dbg("init_idx = %d\n", g_cur_gpu_OPPidx);
			_mt_gpufreq_kick_pbm(1);
			break;
		}
	}

	/* Not found, set to LPM */
	if (i == mt_gpufreqs_num) {
		gpufreq_err("Set to LPM since GPU idx not found according to current Vcore = %d mV\n",
			 cur_volt / 100);
		g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
		_mt_gpufreq_set(cur_freq, mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz,
				g_cur_gpu_OPPidx);
	}
#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);
}

/************************************************
 * frequency adjust interface for thermal protect
 *************************************************/
/******************************************************
 * parameter: target power
 *******************************************************/
static int _mt_gpufreq_power_throttle_protect(void)
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
static void _mt_gpufreq_oc_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);

	mt_gpufreq_power_limited_index_array[IDX_OC_LIMITED] = limited_index;
	_mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

void mt_gpufreq_oc_callback(BATTERY_OC_LEVEL oc_level)
{
	gpufreq_dbg("@%s: oc_level = %d\n", __func__, oc_level);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
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
			_mt_gpufreq_oc_protect(mt_gpufreq_oc_limited_index_1);
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_oc_limited_index != mt_gpufreq_oc_limited_index_0) {
			mt_gpufreq_oc_limited_index = mt_gpufreq_oc_limited_index_0;
			_mt_gpufreq_oc_protect(mt_gpufreq_oc_limited_index_0);
		}
	}
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
/************************************************
 * GPU frequency adjust interface for low bat_volume protect
 *************************************************/
static void _mt_gpufreq_low_batt_volume_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);

	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLUME_LIMITED] = limited_index;
	_mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

void mt_gpufreq_low_batt_volume_callback(BATTERY_PERCENT_LEVEL low_battery_volume)
{
	gpufreq_dbg("@%s: low_battery_volume = %d\n", __func__, low_battery_volume);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
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
			_mt_gpufreq_low_batt_volume_protect(mt_gpufreq_low_bat_volume_limited_index_1);
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_low_batt_volume_limited_index !=
			mt_gpufreq_low_bat_volume_limited_index_0) {
			mt_gpufreq_low_batt_volume_limited_index =
				mt_gpufreq_low_bat_volume_limited_index_0;
			_mt_gpufreq_low_batt_volume_protect(mt_gpufreq_low_bat_volume_limited_index_0);
		}
	}
}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
/************************************************
 * GPU frequency adjust interface for low bat_volt protect
 *************************************************/
static void _mt_gpufreq_low_batt_volt_protect(unsigned int limited_index)
{
	mutex_lock(&mt_gpufreq_power_lock);

	gpufreq_dbg("@%s: limited_index = %d\n", __func__, limited_index);
	mt_gpufreq_power_limited_index_array[IDX_LOW_BATT_VOLT_LIMITED] = limited_index;
	_mt_gpufreq_power_throttle_protect();

	mutex_unlock(&mt_gpufreq_power_lock);
}

/******************************************************
 * parameter: low_battery_level
 *******************************************************/
void mt_gpufreq_low_batt_volt_callback(LOW_BATTERY_LEVEL low_battery_level)
{
	gpufreq_dbg("@%s: low_battery_level = %d\n", __func__, low_battery_level);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
		return;
	}

	if (g_limited_low_batt_volt_ignore_state == true) {
		gpufreq_info("@%s: g_limited_low_batt_volt_ignore_state == true!\n", __func__);
		return;
	}

	mt_gpufreq_low_battery_level = low_battery_level;

#if 0	/* no need to throttle when LV1 */
	if (low_battery_level == LOW_BATTERY_LEVEL_1) {
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_1) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_1;
			_mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_1);
		}
	} else
#endif
	if (low_battery_level == LOW_BATTERY_LEVEL_2) {
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_2) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_2;
			_mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_2);
		}
	}
	/* unlimit gpu */
	else {
		if (mt_gpufreq_low_batt_volt_limited_index !=
			mt_gpufreq_low_bat_volt_limited_index_0) {
			mt_gpufreq_low_batt_volt_limited_index =
				mt_gpufreq_low_bat_volt_limited_index_0;
			_mt_gpufreq_low_batt_volt_protect(mt_gpufreq_low_bat_volt_limited_index_0);
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

	for (i = 0; i < mt_gpufreqs_power_num; i++) {
		if (mt_gpufreqs_power[i].gpufreq_power <= limited_power) {
			limited_freq = mt_gpufreqs_power[i].gpufreq_khz;
			found = 1;
			break;
		}
	}

	/* not found */
	if (!found)
		limited_freq = mt_gpufreqs_power[mt_gpufreqs_power_num - 1].gpufreq_khz;

	gpufreq_dbg("@%s: limited_freq = %d\n", __func__, limited_freq);

	return limited_freq;
}

void mt_gpufreq_thermal_protect(unsigned int limited_power)
{
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
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

	gpufreq_dbg("@%s: limited_power = %d\n", __func__, limited_power);

#ifdef MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
	_mt_update_gpufreqs_power_table();
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

	gpufreq_dbg("Thermal limit frequency upper bound to id = %d\n",
			mt_gpufreq_power_limited_index_array[IDX_THERMAL_LIMITED]);

	_mt_gpufreq_power_throttle_protect();

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
		return mt_gpufreqs_power[mt_gpufreqs_power_num - 1].gpufreq_power;
}

void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power)
{
#ifndef DISABLE_PBM_FEATURE
	int i = 0;
	unsigned int limited_freq = 0;

	mutex_lock(&mt_gpufreq_power_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
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
	_mt_update_gpufreqs_power_table();	/* TODO: need to check overhead? */
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
	unsigned int cur_vcore = _mt_gpufreq_get_cur_volt() / 100;

#ifdef CONFIG_THERMAL
	temp = get_immediate_gpu_wrap() / 1000;
#else
	temp = 40;
#endif

	return mt_spower_get_leakage(MT_SPOWER_VCORE, cur_vcore, temp);
#else
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

/**********************************
 * gpufreq target callback function
 ***********************************/
/*************************************************
 * [note]
 * 1. handle frequency change request
 * 2. call mt_gpufreq_set to set target frequency
 **************************************************/
int mt_gpufreq_target(unsigned int idx)
{
	/* unsigned long flags; */
	unsigned int target_freq, target_volt, target_OPPidx;
	unsigned int cur_freq;
	int ret = 0;

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	return 0;
#endif

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("GPU DVFS not ready!\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (mt_gpufreq_volt_enable_state == 0) {
		gpufreq_dbg("mt_gpufreq_volt_enable_state == 0! return\n");
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}
#ifdef MT_GPU_DVFS_RANDOM_TEST
	idx = _mt_gpufreq_idx_get(5);
	gpufreq_dbg("@%s: random test index is %d !\n", __func__, idx);
#endif

	if (idx > (mt_gpufreqs_num - 1)) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_err("@%s: idx out of range! idx = %d\n", __func__, idx);
		return -1;
	}

	/************************************************
	 * If /proc command fix the frequency.
	 *************************************************/
	if (mt_gpufreq_fixed_freq_state == true) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_info("@%s: GPU freq is fixed by user! fixed_freq = %dKHz\n", __func__,
				 mt_gpufreq_fixed_frequency);
		return 0;
	}

	/**********************************
	 * look up for the target GPU OPP
	 ***********************************/
	target_freq = mt_gpufreqs[idx].gpufreq_khz;
	target_volt = mt_gpufreqs[idx].gpufreq_volt;
	target_OPPidx = idx;

	gpufreq_dbg("@%s: begin, receive freq: %d, OPPidx: %d\n",
			__func__, target_freq, target_OPPidx);

	/**********************************
	 * Check if need to keep max frequency
	 ***********************************/
	if (mt_gpufreq_keep_max_frequency_state == true) {
		target_freq = mt_gpufreqs[g_gpufreq_max_id].gpufreq_khz;
		target_volt = mt_gpufreqs[g_gpufreq_max_id].gpufreq_volt;
		target_OPPidx = g_gpufreq_max_id;
		gpufreq_dbg("Keep MAX frequency %d !\n", target_freq);
	}

	/************************************************
	 * If /proc command keep opp frequency.
	 *************************************************/
	if (mt_gpufreq_keep_opp_frequency_state == true) {
		target_freq = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_khz;
		target_volt = mt_gpufreqs[mt_gpufreq_keep_opp_index].gpufreq_volt;
		target_OPPidx = mt_gpufreq_keep_opp_index;
		gpufreq_dbg("Keep opp! opp frequency %d, opp voltage %d, opp idx %d\n",
				target_freq, target_volt, target_OPPidx);
	}

	/************************************************
	 * If /proc command keep opp max frequency.
	 *************************************************/
	if (mt_gpufreq_opp_max_frequency_state == true) {
		if (target_freq > mt_gpufreq_opp_max_frequency) {
			target_freq = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_khz;
			target_volt = mt_gpufreqs[mt_gpufreq_opp_max_index].gpufreq_volt;
			target_OPPidx = mt_gpufreq_opp_max_index;

			gpufreq_dbg("opp max freq! opp max frequency %d, opp max voltage %d, opp max idx %d\n",
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
			target_OPPidx = g_limited_max_id;
			gpufreq_dbg("Limit! Thermal/Power limit gpu frequency %d\n",
					mt_gpufreqs[g_limited_max_id].gpufreq_khz);
		}
	}
#if defined(CONFIG_ARCH_MT6735M) && defined(MT_GPUFREQ_VCOREFS_ENABLED)
	/************************************************
	* If 550MHz is exist, mask it when vcorefs is disabled
	*************************************************/
	g_is_vcorefs_on = is_vcorefs_can_work();
	if ((!can_do_gpu_dvs()) && (mt_gpufreq_dvfs_table_type == 0 || mt_gpufreq_dvfs_table_type == 2)) {
		if (target_freq > mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_khz) {
			/*********************************************
			* target_freq > limited_freq, need to adjust
			**********************************************/
			target_freq = mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_khz;
			target_volt = mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_volt;
			target_OPPidx = g_gpufreq_max_id_vcorefs_off;
			gpufreq_dbg("Limit! Limit gpu frequency %d due to Vcore DVFS disabled\n",
					mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_khz);
		}
	}
#endif

	/************************************************
	* target frequency == current frequency, skip it
	*************************************************/
	cur_freq = _mt_gpufreq_get_cur_freq();
	if (cur_freq == target_freq) {
		mutex_unlock(&mt_gpufreq_lock);
		gpufreq_dbg("GPU frequency from %d KHz to %d KHz (skipped) due to same frequency\n",
				cur_freq, target_freq);
		return 0;
	}

	gpufreq_dbg("GPU current frequency %d KHz, target frequency %d KHz\n",
			cur_freq, target_freq);

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	/******************************
	* set to the target frequency
	*******************************/
	ret = _mt_gpufreq_set(cur_freq, target_freq, target_OPPidx);
	if (!ret)
		g_cur_gpu_OPPidx = target_OPPidx;

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_oppidx(g_cur_gpu_OPPidx);
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() & ~(1 << GPU_DVFS_IS_DOING_DVFS));
#endif

	mutex_unlock(&mt_gpufreq_lock);

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_target);

/* Set VGPU enable/disable when GPU clock be switched on/off */
int mt_gpufreq_voltage_enable_set(unsigned int enable)
{
	int ret = 0;

	gpufreq_dbg("@%s: enable = %x\n", __func__, enable);

	mutex_lock(&mt_gpufreq_lock);

	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
		mutex_unlock(&mt_gpufreq_lock);
		return -ENOSYS;
	}

	if (enable == mt_gpufreq_volt_enable_state) {
		mutex_unlock(&mt_gpufreq_lock);
		return 0;
	}
#ifdef MT_GPUFREQ_VCOREFS_ENABLED
	g_is_vcorefs_on = is_vcorefs_can_work();

	if (!can_do_gpu_dvs())
		goto done;

	if (enable == 0)
		vcorefs_request_dvfs_opp(KIR_GPU, OPPI_UNREQ);
	else {
		unsigned int cur_volt = _mt_gpufreq_get_cur_volt();
		int need_kick_pbm = 0;

		if (cur_volt != mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt)
			need_kick_pbm = 1;	/* Vcore changed, need to kick PBM */

		ret = _mt_gpufreq_set_cur_volt(g_cur_gpu_OPPidx);

		if (ret) {
			unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

			gpufreq_err("@%s: Set Vcore to %dmV failed! ret = 0x%x, cur_freq = %d\n",
					__func__,
					mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt / 100,
					ret,
					cur_freq
			);

			/* Raise Vcore failed, set GPU freq to corresponding LV */
			if (cur_volt < mt_gpufreqs[g_cur_gpu_OPPidx].gpufreq_volt) {
				unsigned int i;

				for (i = 0; i < mt_gpufreqs_num; i++) {
					if (cur_volt == mt_gpufreqs[i].gpufreq_volt) {
						_mt_gpufreq_set_cur_freq(mt_gpufreqs[i].gpufreq_khz);
						g_cur_gpu_OPPidx = i;
					}
				}

				if (i == mt_gpufreqs_num) {
					gpufreq_err("@%s: Volt not found, set to lowest freq!\n",
							__func__);
					_mt_gpufreq_set_cur_freq(
						mt_gpufreqs[mt_gpufreqs_num - 1].gpufreq_khz);
					g_cur_gpu_OPPidx = mt_gpufreqs_num - 1;
				}
			}
		}

		if (need_kick_pbm)
			_mt_gpufreq_kick_pbm(1);
	}

done:
#endif
	mt_gpufreq_volt_enable_state = enable;

	mutex_unlock(&mt_gpufreq_lock);

#if 0
	/* Notify PBM only when VGPU switch on/off */
	if (enable == 1 && state_orig == 0)
		_mt_gpufreq_kick_pbm(1);
	else if (enable == 0 && state_orig == 1)
		_mt_gpufreq_kick_pbm(0);
#endif

	return ret;
}
EXPORT_SYMBOL(mt_gpufreq_voltage_enable_set);

unsigned int mt_gpufreq_get_dvfs_table_num(void)
{
	return mt_gpufreqs_num;
}
EXPORT_SYMBOL(mt_gpufreq_get_dvfs_table_num);

unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx)
{
	if (mt_gpufreq_ready == false) {
		gpufreq_dbg("@%s: GPU DVFS not ready!\n", __func__);
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
	return _mt_gpufreq_get_cur_freq();
}
EXPORT_SYMBOL(mt_gpufreq_get_cur_freq);

/************************************************
 * return current GPU voltage
 *************************************************/
unsigned int mt_gpufreq_get_cur_volt(void)
{
	return _mt_gpufreq_get_cur_volt();
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

static int _mt_gpufreq_pm_restore_early(struct device *dev)
{
	int i = 0;
	int found = 0;
	unsigned int cur_freq = _mt_gpufreq_get_cur_freq();

	for (i = 0; i < mt_gpufreqs_num; i++) {
		if (cur_freq == mt_gpufreqs[i].gpufreq_khz) {
			g_cur_gpu_OPPidx = i;
			found = 1;
			gpufreq_dbg("match g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);
			break;
		}
	}

	if (found == 0) {
		g_cur_gpu_OPPidx = 0;
		gpufreq_err("gpu freq not found, set parameter to max freq\n");
	}

	gpufreq_dbg("GPU freq: %d\n", cur_freq);
	gpufreq_dbg("g_cur_gpu_OPPidx: %d\n", g_cur_gpu_OPPidx);

	return 0;
}

static int _mt_gpufreq_pdrv_probe(struct platform_device *pdev)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	int rc;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1 };
#endif

#ifdef MT_GPUFREQ_PERFORMANCE_TEST
	/* fix at max freq */
	pmic_set_register_value(PMIC_VCORE1_VOSEL_ON, _mt_gpufreq_volt_to_pmic_wrap(115000));
	udelay(300);
	_mt_gpufreq_set_cur_freq(448500);

	return 0;
#endif

	/**********************
	* Initial leackage power usage
	***********************/
	mt_spower_init();

	/**********************
	* Initial SRAM debugging ptr
	***********************/
#ifdef MT_GPUFREQ_AEE_RR_REC
	_mt_gpufreq_aee_init();
#endif

	/**********************
	* setup gpufreq table
	***********************/
	gpufreq_info("setup gpufreqs table\n");

	mt_gpufreq_dvfs_table_type = _mt_gpufreq_get_dvfs_table_type();
	if (mt_gpufreq_dvfs_table_type == 0)	/* 600M for 6735/6753, 550M for 6735M */
		_mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_0, ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_0));
	else if (mt_gpufreq_dvfs_table_type == 1)	/* 450M */
		_mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_1, ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_1));
#ifndef CONFIG_ARCH_MT6753
	else if (mt_gpufreq_dvfs_table_type == 2)    /* for 35+/37+ */
		_mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_2, ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_2));
#endif
#ifdef CONFIG_ARCH_MT6735
	else if (mt_gpufreq_dvfs_table_type == 3)    /* for 37+ simulate 35+ */
		_mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_3, ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_3));
#endif
	else
		_mt_setup_gpufreqs_table(mt_gpufreq_opp_tbl_e1_1, ARRAY_SIZE(mt_gpufreq_opp_tbl_e1_1));

#ifdef MT_GPUFREQ_AEE_RR_REC
	aee_rr_rec_gpu_dvfs_status(aee_rr_curr_gpu_dvfs_status() | (1 << GPU_DVFS_IS_VGPU_ENABLED));
#endif

	mt_gpufreq_volt_enable_state = 1;

	/**********************
	 * setup initial frequency
	 ***********************/
	_mt_gpufreq_set_initial();

	gpufreq_info("GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
	gpufreq_info("Current Vcore = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
	gpufreq_info("g_cur_gpu_OPPidx = %d\n", g_cur_gpu_OPPidx);

	mt_gpufreq_ready = true;

#ifdef MT_GPUFREQ_INPUT_BOOST
	mt_gpufreq_up_task =
		kthread_create(_mt_gpufreq_input_boost_task, NULL, "mt_gpufreq_input_boost_task");
	if (IS_ERR(mt_gpufreq_up_task))
		return PTR_ERR(mt_gpufreq_up_task);

	sched_setscheduler_nocheck(mt_gpufreq_up_task, SCHED_FIFO, &param);
	get_task_struct(mt_gpufreq_up_task);

	rc = input_register_handler(&mt_gpufreq_input_handler);
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
	{
		int i;

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1) {
				mt_gpufreq_low_bat_volt_limited_index_1 = i;
				break;
			}
		}
		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2) {
				mt_gpufreq_low_bat_volt_limited_index_2 = i;
				break;
			}
		}
		register_low_battery_notify(&mt_gpufreq_low_batt_volt_callback,
						LOW_BATTERY_PRIO_GPU);
	}
#endif

#ifdef MT_GPUFREQ_LOW_BATT_VOLUME_PROTECT
	{
		int i;

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_LOW_BATT_VOLUME_LIMIT_FREQ_1) {
				mt_gpufreq_low_bat_volume_limited_index_1 = i;
				break;
			}
		}
		register_battery_percent_notify(&mt_gpufreq_low_batt_volume_callback,
						BATTERY_PERCENT_PRIO_GPU);
	}
#endif

#ifdef MT_GPUFREQ_OC_PROTECT
	{
		int i;

		for (i = 0; i < mt_gpufreqs_num; i++) {
			if (mt_gpufreqs[i].gpufreq_khz == MT_GPUFREQ_OC_LIMIT_FREQ_1) {
				mt_gpufreq_oc_limited_index_1 = i;
				break;
			}
		}
		register_battery_oc_notify(&mt_gpufreq_oc_callback, BATTERY_OC_PRIO_GPU);
	}
#endif

	return 0;
}

/***************************************
 * this function should never be called
 ****************************************/
static int _mt_gpufreq_pdrv_remove(struct platform_device *pdev)
{
#ifdef MT_GPUFREQ_INPUT_BOOST
	input_unregister_handler(&mt_gpufreq_input_handler);

	kthread_stop(mt_gpufreq_up_task);
	put_task_struct(mt_gpufreq_up_task);
#endif

	return 0;
}


const struct dev_pm_ops mt_gpufreq_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
	.restore_early = _mt_gpufreq_pm_restore_early,
};

struct platform_device mt_gpufreq_pdev = {
	.name = "mt-gpufreq",
	.id = -1,
};

static struct platform_driver mt_gpufreq_pdrv = {
	.probe = _mt_gpufreq_pdrv_probe,
	.remove = _mt_gpufreq_pdrv_remove,
	.driver = {
		.name = "mt-gpufreq",
		.pm = &mt_gpufreq_pm_ops,
		.owner = THIS_MODULE,
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

	seq_printf(m, "g_limited_max_id = %d, limit frequency = %d\n", g_limited_max_id,
		   mt_gpufreqs[g_limited_max_id].gpufreq_khz);

	return 0;
}

/**********************************
 * limited power for thermal protect
 ***********************************/
static ssize_t mt_gpufreq_limited_power_proc_write(struct file *file, const char __user *buffer,
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
static ssize_t mt_gpufreq_state_proc_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
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
			_mt_gpufreq_state_set(1);
		} else if (enabled == 0) {
			/* Keep MAX frequency when GPU DVFS disabled. */
			mt_gpufreq_keep_max_frequency_state = true;
			mt_gpufreq_voltage_enable_set(1);
			mt_gpufreq_target(g_gpufreq_max_id);
			_mt_gpufreq_state_set(0);
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
		seq_printf(m, "volt = %d\n", mt_gpufreqs[i].gpufreq_volt);
	}

	return 0;
}

/********************
 * show GPU power table
 *********************/
static int mt_gpufreq_power_dump_proc_show(struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < mt_gpufreqs_power_num; i++) {
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
#if defined(CONFIG_ARCH_MT6735M) && defined(MT_GPUFREQ_VCOREFS_ENABLED)
			/************************************************
			* If 550MHz is exist, mask it when vcorefs is disabled
			*************************************************/
			g_is_vcorefs_on = is_vcorefs_can_work();
			if ((!can_do_gpu_dvs()) && (mt_gpufreq_dvfs_table_type == 0 || mt_gpufreq_dvfs_table_type == 2)) {
				unsigned int limited_freq =
					mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_khz;

				if (fixed_freq > limited_freq) {
					gpufreq_warn("bad argument!! OPP freq cannot higher than %dKHz since Vcore DVFS is disabled\n",
						 limited_freq);
					return count;
				}
			}
#endif

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
#if defined(CONFIG_ARCH_MT6735M) && defined(MT_GPUFREQ_VCOREFS_ENABLED)
			/************************************************
			* If 550MHz is exist, mask it when vcorefs is disabled
			*************************************************/
			g_is_vcorefs_on = is_vcorefs_can_work();
			if ((!can_do_gpu_dvs()) && mt_gpufreq_dvfs_table_type == 0) {
				unsigned int limited_freq =
					mt_gpufreqs[g_gpufreq_max_id_vcorefs_off].gpufreq_khz;

				if (max_freq > limited_freq) {
					gpufreq_warn("bad argument!! max_freq cannot higher than %dKHz since Vcore DVFS is disabled\n",
						 limited_freq);
					return count;
				}
			}
#endif
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

	seq_printf(m, "GPU current frequency = %dKHz\n", _mt_gpufreq_get_cur_freq());
	seq_printf(m, "Current Vcore = %dmV\n", _mt_gpufreq_get_cur_volt() / 100);
	seq_printf(m, "g_cur_gpu_OPPidx = %d\n", g_cur_gpu_OPPidx);
	seq_printf(m, "g_last_gpu_dvs_result = %d (0:success, 127:not enabled, else:error)\n",
		   g_last_gpu_dvs_result);
	seq_printf(m, "g_is_vcorefs_on = %d (0:disable, 1:enable, 255:not build-in)\n",
		   g_is_vcorefs_on);
	seq_printf(m, "g_limited_max_id = %d\n", g_limited_max_id);

	for (i = 0; i < NR_IDX_POWER_LIMITED; i++)
		seq_printf(m, "mt_gpufreq_power_limited_index_array[%d] = %d\n", i,
			   mt_gpufreq_power_limited_index_array[i]);

	seq_printf(m, "mt_gpufreq_volt_enable_state = %d\n", mt_gpufreq_volt_enable_state);
	seq_printf(m, "mt_gpufreq_fixed_freq_state = %d\n", mt_gpufreq_fixed_freq_state);
	seq_printf(m, "mt_gpufreq_dvfs_table_type = %d\n", mt_gpufreq_dvfs_table_type);
	seq_printf(m, "mt_gpufreq_dvfs_mmpll_spd_bond = %d\n", mt_gpufreq_dvfs_mmpll_spd_bond);
	seq_printf(m, "is_have_550MHz = %d\n", is_have_550());

	return 0;
}

#if 0
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
			mt_gpufreq_voltage_enable_set(0);
			mt_gpufreq_volt_enable = false;
		} else if (enable == 1) {
			mt_gpufreq_voltage_enable_set(1);
			mt_gpufreq_volt_enable = true;
		} else
			gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		gpufreq_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");

	return count;
}
#endif

/***************************
 * show current specific frequency status
 ****************************/
static int mt_gpufreq_fixed_freq_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_fixed_freq_state) {
		seq_puts(m, "gpufreq fixed frequency enabled\n");
		seq_printf(m, "fixed frequency = %d\n", mt_gpufreq_fixed_frequency);
	} else
		seq_puts(m, "gpufreq fixed frequency disabled\n");

	return 0;
}

/***********************
 * enable specific frequency
 ************************/
static ssize_t mt_gpufreq_fixed_freq_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *data)
{
	char desc[32];
	int len = 0;

	int fixed_freq = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (!kstrtoint(desc, 10, &fixed_freq)) {
		mutex_lock(&mt_gpufreq_lock);
		if (fixed_freq == 0) {
			mt_gpufreq_fixed_freq_state = false;
			mt_gpufreq_fixed_frequency = 0;
		} else {
			_mt_gpufreq_set_cur_freq(fixed_freq);
			mt_gpufreq_fixed_freq_state = true;
			mt_gpufreq_fixed_frequency = fixed_freq;
			g_cur_gpu_OPPidx = 0;	/* keep Max Vcore */
		}
		mutex_unlock(&mt_gpufreq_lock);
	} else
		gpufreq_warn("bad argument!! should be [enable fixed_freq(KHz)]\n");

	return count;
}

#ifdef MT_GPUFREQ_INPUT_BOOST
/*****************************
 * show current input boost status
 ******************************/
static int mt_gpufreq_input_boost_proc_show(struct seq_file *m, void *v)
{
	if (mt_gpufreq_input_boost_state == 1)
		seq_puts(m, "gpufreq debug enabled\n");
	else
		seq_puts(m, "gpufreq debug disabled\n");

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
/* PROC_FOPS_RW(gpufreq_volt_enable); */
PROC_FOPS_RW(gpufreq_fixed_freq);
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
		/* PROC_ENTRY(gpufreq_volt_enable), */
		PROC_ENTRY(gpufreq_fixed_freq),
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
			gpufreq_err("@%s: create /proc/gpufreq/%s failed\n",
					__func__, entries[i].name);
	}

	return 0;
}
#endif	/* CONFIG_PROC_FS */

/**********************************
 * mediatek gpufreq initialization
 ***********************************/
static int __init _mt_gpufreq_init(void)
{
	int ret = 0;

	gpufreq_info("@%s\n", __func__);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (mt_gpufreq_create_procfs())
		goto out;

#endif	/* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&mt_gpufreq_pdev);
	if (ret) {
		gpufreq_err("fail to register gpufreq device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&mt_gpufreq_pdrv);
	if (ret) {
		gpufreq_err("fail to register gpufreq driver @ %s()\n", __func__);
		platform_device_unregister(&mt_gpufreq_pdev);
	}

out:
	return ret;
}

static void __exit _mt_gpufreq_exit(void)
{
	platform_driver_unregister(&mt_gpufreq_pdrv);
	platform_device_unregister(&mt_gpufreq_pdev);
}
module_init(_mt_gpufreq_init);
module_exit(_mt_gpufreq_exit);

MODULE_DESCRIPTION("MediaTek GPU Frequency Scaling driver");
MODULE_LICENSE("GPL");

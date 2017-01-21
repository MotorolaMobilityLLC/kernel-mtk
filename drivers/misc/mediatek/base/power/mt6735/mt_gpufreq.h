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

#ifndef _MT_GPUFREQ_H
#define _MT_GPUFREQ_H

#include <linux/module.h>

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
 * VCORE DVFS option
 ***************************************************/
#define MT_GPUFREQ_VCOREFS_ENABLED

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
 * Define for performance test
 ****************************/
/* #define MT_GPUFREQ_PERFORMANCE_TEST */

/***************************
 * Define for SRAM debugging
 ****************************/
#ifdef CONFIG_MTK_RAM_CONSOLE
#define MT_GPUFREQ_AEE_RR_REC
#endif

/**********************************
 * Power table struct for thermal
 **********************************/
struct mt_gpufreq_power_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_power;
};


/******************************
 * Forward ref
 *******************************/
extern u32 get_devinfo_with_index(u32 index);
#ifdef MT_GPUFREQ_AEE_RR_REC
extern void aee_rr_rec_gpu_dvfs_vgpu(u8 val);
extern void aee_rr_rec_gpu_dvfs_oppidx(u8 val);
extern void aee_rr_rec_gpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_gpu_dvfs_status(void);
#endif

/*****************
 * extern function
 ******************/
/* extern int mt_gpufreq_state_set(int enabled); */
extern unsigned int mt_gpufreq_get_cur_freq_index(void);
extern unsigned int mt_gpufreq_get_cur_freq(void);
extern unsigned int mt_gpufreq_get_cur_volt(void);
extern unsigned int mt_gpufreq_get_dvfs_table_num(void);
extern int mt_gpufreq_target(unsigned int idx);
extern int mt_gpufreq_voltage_enable_set(unsigned int enable);
extern unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx);
extern void mt_gpufreq_thermal_protect(unsigned int limited_power);
extern unsigned int mt_gpufreq_get_max_power(void);
extern unsigned int mt_gpufreq_get_min_power(void);
extern unsigned int mt_gpufreq_get_thermal_limit_index(void);
extern unsigned int mt_gpufreq_get_thermal_limit_freq(void);
extern void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power);
extern unsigned int mt_gpufreq_get_leakage_mw(void);

/*****************
 * power limit notification
 ******************/
typedef void (*gpufreq_power_limit_notify) (unsigned int);
extern void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB);

/*****************
 * input boost notification
 ******************/
typedef void (*gpufreq_input_boost_notify) (unsigned int);
extern void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB);

/*****************
 * profiling purpose
 ******************/
typedef void (*sampler_func) (unsigned int);
extern void mt_gpufreq_setfreq_registerCB(sampler_func pCB);
extern void mt_gpufreq_setvolt_registerCB(sampler_func pCB);

#endif

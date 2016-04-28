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
 * Clock Mux Register
 */
/*#define CLK26CALI       (0xF00001C0) // FIX ME, No this register
#define CLK_MISC_CFG_0  (0xF0000210)
#define CLK_MISC_CFG_1  (0xF0000214)
#define CLK26CALI_0     (0xF0000220)
#define CLK26CALI_1     (0xF0000224)
#define CLK26CALI_2     (0xF0000228)
#define MBIST_CFG_0     (0xF0000308)
#define MBIST_CFG_1     (0xF000030C)
#define MBIST_CFG_2     (0xF0000310)
#define MBIST_CFG_3     (0xF0000314)
*/


#if 0
/*
 * PMIC Wrapper DVFS Register
 */
#define PWRAP_BASE              (0xF000D000)
#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE + 0xE8) /**/
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE + 0xEC) /**/
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE + 0xF0) /**/
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE + 0xF4) /**/
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE + 0xF8) /**/
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE + 0xFC) /**/
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE + 0x100) /**/
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE + 0x104) /**/
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE + 0x108) /**/
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE + 0x10C) /**/
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE + 0x110) /**/
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE + 0x114) /**/
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE + 0x118) /**/
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE + 0x11C) /**/
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE + 0x120) /**/
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE + 0x124) /**/
#endif

enum post_div_enum {
	POST_DIV2 = 0,
	POST_DIV4,
	POST_DIV8,
};

struct mt_gpufreq_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_idx;
};

struct mt_gpufreq_power_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_power;
};

struct mt_gpufreq_clk_t {
	struct clk *clk_mux;          /* main clock for mfg setting */
	struct clk *clk_main_parent;	 /* substitution clock for mfg transient mux setting */
	struct clk *clk_sub_parent;	 /* substitution clock for mfg transient parent setting */
};


/*
 * extern function
 */
extern int mt_gpufreq_state_set(int enabled);
extern void mt_gpufreq_thermal_protect(unsigned int limited_power);
extern unsigned int mt_gpufreq_get_max_power(void);
extern unsigned int mt_gpufreq_get_min_power(void);
extern unsigned int mt_gpufreq_get_cur_freq_index(void);
extern unsigned int mt_gpufreq_get_cur_freq(void);
extern unsigned int mt_gpufreq_get_cur_volt(void);
extern unsigned int mt_gpufreq_get_dvfs_table_num(void);
extern unsigned int mt_gpufreq_target(unsigned int idx);
extern unsigned int mt_gpufreq_voltage_enable_set(unsigned int enable);
extern unsigned int mt_gpufreq_update_volt(unsigned int pmic_volt[], unsigned int array_size);
extern unsigned int mt_gpufreq_get_freq_by_idx(unsigned int idx);
extern void mt_gpufreq_restore_default_volt(void);
extern void mt_gpufreq_enable_by_ptpod(void);
extern void mt_gpufreq_disable_by_ptpod(void);
extern unsigned int mt_gpufreq_get_thermal_limit_index(void);
extern unsigned int mt_gpufreq_get_thermal_limit_freq(void);
extern void mt_gpufreq_set_power_limit_by_pbm(unsigned int limited_power);
extern unsigned int mt_gpufreq_get_leakage_mw(void);

extern unsigned int mt_get_mfgclk_freq(void);	/* Freq Meter API */
extern u32 get_devinfo_with_index(u32 index);

/* #ifdef MT_GPUFREQ_AEE_RR_REC */
extern void aee_rr_rec_gpu_dvfs_vgpu(u8 val);
extern void aee_rr_rec_gpu_dvfs_oppidx(u8 val);
extern void aee_rr_rec_gpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_gpu_dvfs_status(void);
/* #endif */

/*
 * power limit notification
 */
typedef void (*gpufreq_power_limit_notify)(unsigned int);
extern void mt_gpufreq_power_limit_notify_registerCB(gpufreq_power_limit_notify pCB);

/*
 * input boost notification
 */
typedef void (*gpufreq_input_boost_notify)(unsigned int);
extern void mt_gpufreq_input_boost_notify_registerCB(gpufreq_input_boost_notify pCB);

/*
 * profiling purpose
 */
typedef void (*sampler_func)(unsigned int);
extern void mt_gpufreq_setfreq_registerCB(sampler_func pCB);
extern void mt_gpufreq_setvolt_registerCB(sampler_func pCB);

#endif

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

#ifndef __MTK_CPUFREQ_H__
#define __MTK_CPUFREQ_H__

#include <linux/kernel.h>

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LL,
	MT_CPU_DVFS_L,
	MT_CPU_DVFS_CCI,

	NR_MT_CPU_DVFS,
};

enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_MAINPLL = 2,
	TOP_CKMUXSEL_UNIVPLL = 3,

	NR_TOP_CKMUXSEL,
};

enum dvfs_time_profile {
	SET_DVFS = 0,
	SET_FREQ = 1,
	SET_VOLT = 2,
	SET_VPROC = 3,
	SET_VSRAM = 4,
	SET_DELAY = 5,

	NR_SET_V_F,
};

typedef void (*cpuVoltsampler_func)(enum mt_cpu_dvfs_id, unsigned int mv);
typedef void (*mt_cpufreq_set_ptbl_funcPTP)(enum mt_cpu_dvfs_id id,
					    int restore);

/* PMIC */
extern int is_ext_buck_sw_ready(void);
extern int is_ext_buck_exist(void);
extern unsigned int mt6311_read_byte(unsigned char cmd,
				     unsigned char *returnData);

extern u32 get_devinfo_with_index(u32 index);

/* SRAM debugging*/
extern void aee_rr_rec_cpu_dvfs_vproc_big(u8 val);
extern void aee_rr_rec_cpu_dvfs_vproc_little(u8 val);
extern void aee_rr_rec_cpu_dvfs_oppidx(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_oppidx(void);
extern void aee_rr_rec_cpu_dvfs_cci_oppidx(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_cci_oppidx(void);
extern void aee_rr_rec_cpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_status(void);
extern void aee_rr_rec_cpu_dvfs_step(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_step(void);
extern void aee_rr_rec_cpu_dvfs_pbm_step(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_pbm_step(void);
extern void aee_rr_rec_cpu_dvfs_cb(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_cb(void);
extern void aee_rr_rec_cpufreq_cb(u8 val);
extern u8 aee_rr_curr_cpufreq_cb(void);

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id,
				  unsigned int *volt_tbl, int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_eem_resume(void);
extern void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB);

/* PBM */
extern unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id);

/* PPB */
#ifdef CONFIG_FPGA_EARLY_PORTING
static inline int mt_cpufreq_get_ppb_state(void) { return 0; }
static inline unsigned int mt_cpufreq_ppb_hispeed_freq(unsigned int cpu,
						       unsigned int mode)
{
	return 0;
}
#else
extern int mt_cpufreq_get_ppb_state(void);
extern unsigned int mt_cpufreq_ppb_hispeed_freq(unsigned int cpu,
						unsigned int mode);
#endif

/* PPM */
extern unsigned int mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id);

/* Generic */
extern int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id,
				   enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);

/* CPUFREQ */
extern void aee_record_cpufreq_cb(unsigned int step);

#endif /* __MTK_CPUFREQ_H__ */

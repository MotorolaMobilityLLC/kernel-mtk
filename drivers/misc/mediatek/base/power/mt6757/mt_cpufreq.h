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

/*
 * @file mt_cpufreq.h
 * @brief CPU DVFS driver interface
 */

#ifndef __MT_CPUFREQ_H__
#define __MT_CPUFREQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_IDVFS 1
extern int disable_idvfs_flag;

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LL,
	MT_CPU_DVFS_L,
	MT_CPU_DVFS_B,
	MT_CPU_DVFS_CCI,

	NR_MT_CPU_DVFS,
};

/* 3 => MAIN */
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

typedef void (*cpuVoltsampler_func) (enum mt_cpu_dvfs_id, unsigned int mv);

extern u32 get_devinfo_with_index(u32 index);
extern void (*cpufreq_freq_check)(enum mt_cpu_dvfs_id id);

/* Freq Meter API */
#ifdef __KERNEL__
extern unsigned int mt_get_cpu_freq(void);
#endif

/* #ifdef CONFIG_CPU_DVFS_AEE_RR_REC */
#if 1
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
#endif

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
				  int nr_volt_tbl);
extern int mt_cpufreq_update_volt_b(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
				  int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
typedef void (*mt_cpufreq_set_ptbl_funcPTP)(enum mt_cpu_dvfs_id id, int restore);
extern void mt_cpufreq_eem_resume(void);
extern void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB);

/* PBM */
extern unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id);

/* PPB */
extern int mt_cpufreq_get_ppb_state(void);

/* PPM */
extern unsigned int mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_org_volt(enum mt_cpu_dvfs_id id, int idx);

/* Generic */
extern int mt_cpufreq_state_set(int enabled);
extern int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
extern bool mt_cpufreq_earlysuspend_status_get(void);
extern unsigned int mt_get_cpu_freq(void);

/* CPUFREQ */
extern void aee_record_cpufreq_cb(unsigned int step);

#ifdef __cplusplus
}
#endif
#endif

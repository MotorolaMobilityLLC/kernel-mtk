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
struct mt_cpu_power_tbl {
	unsigned int ncpu_big;
	unsigned int khz_big;
	unsigned int ncpu_little;
	unsigned int khz_little;
	unsigned int performance;
	unsigned int power;
};

struct mt_cpu_tlp_power_info {
	struct mt_cpu_power_tbl *power_tbl;
	unsigned int nr_power_table;
};

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LITTLE,
	MT_CPU_DVFS_BIG,

	NR_MT_CPU_DVFS,
};

enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_MAINPLL = 2,
	TOP_CKMUXSEL_UNIVPLL = 3,

	NR_TOP_CKMUXSEL,
};

enum top_ckmuxsel_cci {
	TOP_CKMUXSEL_CLKSQ_CCI = 0,
	TOP_CKMUXSEL_ARMPLL_L = 1,
	TOP_CKMUXSEL_ARMPLL_LL = 2,
	TOP_CKMUXSEL_CLKSQ_CCI_2 = 3,

	NR_TOP_CKMUXSEL_CCI,
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

/* PMIC */
extern int is_ext_buck_sw_ready(void);
extern int is_ext_buck_exist(void);
extern void mt6311_set_vdvfs11_vosel(unsigned char val);
extern void mt6311_set_vdvfs11_vosel_on(unsigned char val);
extern void mt6311_set_vdvfs11_vosel_ctrl(unsigned char val);
extern unsigned int mt6311_read_byte(unsigned char cmd, unsigned char *returnData);
extern unsigned int mt6311_get_chip_id(void);

extern u32 get_devinfo_with_index(u32 index);
extern void (*cpufreq_freq_check)(enum mt_cpu_dvfs_id id);

/* PMIC WRAP */
#ifdef CONFIG_OF
extern void __iomem *pwrap_base;
#define PWRAP_BASE_ADDR     ((unsigned long)pwrap_base)
#endif

/* #ifdef CONFIG_CPU_DVFS_AEE_RR_REC */
#if 1
/* SRAM debugging*/
extern void aee_rr_rec_cpu_dvfs_vproc_big(u8 val);
extern void aee_rr_rec_cpu_dvfs_vproc_little(u8 val);
extern void aee_rr_rec_cpu_dvfs_oppidx(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_oppidx(void);
extern void aee_rr_rec_cpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_cpu_dvfs_status(void);
#endif

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
				  int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
typedef void (*mt_cpufreq_set_ptbl_funcPTP)(enum mt_cpu_dvfs_id id, int restore);
extern void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB);

/* Thermal */
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);

/* PBM */
extern unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_phy_volt(enum mt_cpu_dvfs_id id);

/* PPB */
extern int mt_cpufreq_get_ppb_state(void);

/* L hispeed */
extern int mt_cpufreq_get_chip_id_38(void);

/* DCM */
extern int sync_dcm_set_cci_freq(unsigned int cci_hz);
extern int sync_dcm_set_mp0_freq(unsigned int mp0_hz);
extern int sync_dcm_set_mp1_freq(unsigned int mp1_hz);

/* Generic */
extern int mt_cpufreq_state_set(int enabled);
extern int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
extern bool mt_cpufreq_earlysuspend_status_get(void);

#ifdef __cplusplus
}
#endif
#endif

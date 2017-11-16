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

/**
 * @file mt_cpufreq.h
 * @brief CPU DVFS driver interface
 */

#ifndef __MT_CPUFREQ_H__
#define __MT_CPUFREQ_H__

#ifdef __cplusplus
extern "C" {
#endif

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LITTLE,
	NR_MT_CPU_DVFS,
};

enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_UNIVPLL = 2,
	TOP_CKMUXSEL_MAINPLL = 3,

	NR_TOP_CKMUXSEL,
};

/*
 * PMIC_WRAP
 */

/* Phase */
enum pmic_wrap_phase_id {
	PMIC_WRAP_PHASE_NORMAL,
	PMIC_WRAP_PHASE_DEEPIDLE,

	NR_PMIC_WRAP_PHASE,
};

/* IDX mapping */
enum {
	IDX_NM_VCORE,

	NR_IDX_NM,
};

enum {
	IDX_DI_VCORE_NORMAL,
	IDX_DI_VCORE_SLEEP,

	NR_IDX_DI,
};
typedef void (*cpuVoltsampler_func) (enum mt_cpu_dvfs_id, unsigned int mv);

/* PMIC WRAP ADDR */
#ifdef CONFIG_OF
extern void __iomem *pwrap_base;
#define PWRAP_BASE_ADDR     ((unsigned int)pwrap_base)
#else
#define PWRAP_BASE 0x0
#define PWRAP_BASE_ADDR     PWRAP_BASE
#endif

/* PMIC WRAP */
extern s32 pwrap_read(u32 adr, u32 *rdata);
extern void mt_cpufreq_set_pmic_phase(enum pmic_wrap_phase_id phase);
extern void mt_cpufreq_set_pmic_cmd(enum pmic_wrap_phase_id phase, int idx,
						    unsigned int cmd_wdata);
extern void mt_cpufreq_apply_pmic_cmd(int idx);

/* PTP-OD */
extern unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx);
extern int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl,
						  int nr_volt_tbl);
extern void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_enable_by_ptpod(enum mt_cpu_dvfs_id id);
extern unsigned int mt_cpufreq_disable_by_ptpod(enum mt_cpu_dvfs_id id);

/* Thermal */
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);

/* SDIO */
extern void mt_vcore_dvfs_disable_by_sdio(unsigned int type, bool disabled);
extern void mt_vcore_dvfs_volt_set_by_sdio(unsigned int volt);
extern unsigned int mt_vcore_dvfs_volt_get_by_sdio(void);

extern unsigned int mt_get_cur_volt_vcore_ao(void);

/* Generic */
extern int mt_cpufreq_state_set(int enabled);
extern int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel);
extern enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id);
extern void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB);
extern bool mt_cpufreq_earlysuspend_status_get(void);

extern u32 get_devinfo_with_index(u32 index);
extern void (*cpufreq_freq_check)(enum mt_cpu_dvfs_id id);

#ifdef __cplusplus
}
#endif
#endif

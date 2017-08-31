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
#ifndef _MT_CPUFREQ_HYBRID_
#define _MT_CPUFREQ_HYBRID_

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#define CONFIG_HYBRID_CPU_DVFS 1
#endif

struct cpu_dvfs_log {
	unsigned int time_stamp_l_log:32;
	unsigned int time_stamp_h_log:32;

	struct {
		unsigned int vproc_log:10;
		unsigned int opp_idx_log:4;
		unsigned int sche_idx_log:5;
		unsigned int wfi_idx_log:4;
		unsigned int vsram_log:9;
	} cluster_opp_cfg[NR_MT_CPU_DVFS];

	unsigned int pause_bit:2;
	unsigned int ll_en:2;
	unsigned int l_ll_limit:4;
	unsigned int h_ll_limit:4;
	unsigned int l_en:2;
	unsigned int l_l_limit:4;
	unsigned int h_l_limit:4;
	unsigned int b_en:2;
	unsigned int l_b_limit:4;
	unsigned int h_b_limit:4;
};

struct cpu_dvfs_log_box {
	unsigned long long time_stamp;
	struct {
		unsigned int freq_idx;
		unsigned int limit_idx;
		unsigned int base_idx;
	} cluster_opp_cfg[NR_MT_CPU_DVFS];
};

/* Parameter Enum */
enum cpu_dvfs_ipi_type {
	IPI_DVFS_INIT_PTBL,
	IPI_DVFS_INIT,
	IPI_SET_DVFS,
	IPI_SET_CLUSTER_ON_OFF,
	/* IPI_SET_VOLT, */
	IPI_SET_FREQ,
	IPI_GET_VOLT,
	IPI_GET_FREQ,
	IPI_PAUSE_DVFS,
	IPI_TURBO_MODE,

	NR_DVFS_IPI,
};

typedef struct cdvfs_data {
	unsigned int cmd;
	union {
		struct {
			unsigned int arg[3];
		} set_fv;
	} u;
} cdvfs_data_t;

int cpuhvfs_module_init(void);
int cpuhvfs_set_init_sta(void);
int cpuhvfs_set_mix_max(int cluster_id, int base, int limit);
int cpuhvfs_set_cluster_on_off(int cluster_id, int state);
int cpuhvfs_set_dvfs(int cluster_id, unsigned int freq);
int cpuhvfs_set_volt(int cluster_id, unsigned int volt);
int cpuhvfs_set_freq(int cluster_id, unsigned int freq);
int cpuhvfs_get_volt(int buck_id);
int cpuhvfs_get_freq(int pll_id);
int cpuhvfs_set_turbo_mode(int turbo_mode, int freq_step, int volt_step);
int dvfs_to_sspm_command(u32 cmd, struct cdvfs_data *cdvfs_d);
#endif

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

#define SRAM_BASE_ADDR 0x00100000
struct cpu_dvfs_log {
	unsigned int time_stamp_l_log:32;
	unsigned int time_stamp_h_log:32;

	struct {
		unsigned int vproc_log:7;
		unsigned int opp_idx_log:4;
		unsigned int wfi_log:15;
		unsigned int vsram_log:6;
	} cluster_opp_cfg[NR_MT_CPU_DVFS];

	unsigned int pause_bit:2;
	unsigned int ll_en:2;
	unsigned int l_ll_limit:4;
	unsigned int h_ll_limit:4;
	unsigned int l_en:2;
	unsigned int h_l_limit:4;
	unsigned int l_l_limit:4;
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

#define PAUSE_NO_ACTION 0
#define PAUSE_IDLE 1
#define UNPAUSE 2
#define PAUSE_SUSPEND_LL 3
#define PAUSE_SUSPEND_L 4

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

typedef void (*dvfs_notify_t)(struct cpu_dvfs_log_box *log_box, int num_log);
#ifdef CONFIG_HYBRID_CPU_DVFS
extern int cpuhvfs_pause_dvfsp_running(void);
extern void cpuhvfs_unpause_dvfsp_to_run(void);
#if 0
extern int cpuhvfs_dvfsp_suspend(void);
extern void cpuhvfs_dvfsp_resume(unsigned int on_cluster);
#endif
/* #ifdef CPUHVFS_HW_GOVERNOR */
extern void cpuhvfs_register_dvfs_notify(dvfs_notify_t callback);
/* #endif */
#else
static inline int cpuhvfs_pause_dvfsp_running(void)	{ return 0; }
static inline void cpuhvfs_unpause_dvfsp_to_run(void)	{}
#if 0
static inline int cpuhvfs_dvfsp_suspend(void)		{ return 0; }
static inline void cpuhvfs_dvfsp_resume(unsigned int on_cluster)	{}
#endif
static inline void cpuhvfs_register_dvfs_notify(dvfs_notify_t callback)	{}
#endif

#endif

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

#ifndef __MTK_CM_MGR_H__
#define __MTK_CM_MGR_H__

#include <mtk_cm_mgr_platform.h>
#include <mtk_cm_mgr_platform_reg.h>

#include <linux/kernel.h>
#include <mtk_cpufreq_internal.h>

struct cm_mgr_data {
	unsigned int cmd;
	unsigned int arg;
};

#define CM_MGR_D_LEN		(2)
#define IPI_CM_MGR_INIT 0
#define IPI_CM_MGR_ENABLE 1
#define IPI_CM_MGR_OPP_ENABLE 2
#define IPI_CM_MGR_SSPM_ENABLE 3
#define IPI_CM_MGR_BLANK 4
#define IPI_CM_MGR_DISABLE_FB 5
#define IPI_CM_MGR_DRAM_TYPE 6
#define IPI_CM_MGR_CPU_POWER_RATIO_UP 7
#define IPI_CM_MGR_CPU_POWER_RATIO_DOWN 8
#define IPI_CM_MGR_VCORE_POWER_RATIO_UP 9
#define IPI_CM_MGR_VCORE_POWER_RATIO_DOWN 10
#define IPI_CM_MGR_DEBOUNCE_UP 11
#define IPI_CM_MGR_DEBOUNCE_DOWN 12
#define IPI_CM_MGR_GPU_ENABLE 13
#define IPI_CM_MGR_GPU_POWER_RATIO_UP 14
#define IPI_CM_MGR_GPU_POWER_RATIO_DOWN 15

extern spinlock_t cm_mgr_lock;
extern int cm_mgr_gpu_enable;
extern int cm_mgr_disable_fb;
extern int cm_mgr_blank_status;
extern int vcore_power_ratio_up[CM_MGR_EMI_OPP];
extern int vcore_power_ratio_down[CM_MGR_EMI_OPP];
extern int cm_mgr_enable;

extern void check_cm_mgr_status_internal(void);
extern void check_cm_mgr_status(unsigned int cluster, unsigned int freq);
extern int cm_mgr_to_sspm_command(u32 cmd, int val);

enum mt_cpu_dvfs_id;

extern unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id);

extern void sched_get_percpu_load2(int cpu, bool reset,
		unsigned int *rel_load, unsigned int *abs_load);

__attribute__((weak))
int dvfsrc_get_bw(int type) {return 0; };

#endif	/* __MTK_CM_MGR_H__ */

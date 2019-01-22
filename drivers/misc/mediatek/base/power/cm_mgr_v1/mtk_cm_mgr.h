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

#if defined(CONFIG_MACH_MT6758)
#include <mtk_cm_mgr_mt6758.h>
#elif defined(CONFIG_MACH_MT6771)
#include <mtk_cm_mgr_mt6771.h>
#include <mtk_cm_mgr_mt6771_reg.h>
#elif defined(CONFIG_MACH_MT6775)
#include <mtk_cm_mgr_mt6775.h>
#include <mtk_cm_mgr_mt6775_reg.h>
#endif

#include <linux/kernel.h>
#include <mtk_cpufreq_internal.h>

extern spinlock_t cm_mgr_lock;
extern int cm_mgr_disable_fb;
extern int cm_mgr_blank_status;
extern int vcore_power_ratio_up[CM_MGR_EMI_OPP];
extern int vcore_power_ratio_down[CM_MGR_EMI_OPP];
extern int cm_mgr_enable;

extern void check_cm_mgr_status_internal(void);
extern void check_cm_mgr_status(unsigned int cluster, unsigned int freq);

enum mt_cpu_dvfs_id;

extern unsigned int mt_cpufreq_get_cur_phy_freq_no_lock(enum mt_cpu_dvfs_id id);

extern void sched_get_percpu_load2(int cpu, bool reset, unsigned int *rel_load, unsigned int *abs_load);

__attribute__((weak))
void dvfsrc_set_power_model_ddr_request(unsigned int level) {};

#endif	/* __MTK_CM_MGR_H__ */

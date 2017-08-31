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
#ifndef __MT_CPUFREQ_DEBUG_H__
#define __MT_CPUFREQ_DEBUG_H__

#ifdef CONFIG_MTK_RAM_CONSOLE
#define CONFIG_CPU_DVFS_AEE_RR_REC 1
#endif

enum cpu_dvfs_state {
	CPU_DVFS_LL_IS_DOING_DVFS = 0,
	CPU_DVFS_L_IS_DOING_DVFS,
	CPU_DVFS_B_IS_DOING_DVFS,
	CPU_DVFS_CCI_IS_DOING_DVFS,
};

extern void aee_record_cpu_dvfs_in(struct mt_cpu_dvfs *p);
extern void aee_record_cpu_dvfs_out(struct mt_cpu_dvfs *p);
extern void aee_record_cpu_dvfs_step(unsigned int step);
extern void aee_record_cci_dvfs_step(unsigned int step);
extern void aee_record_cpu_dvfs_cb(unsigned int step);
extern void aee_record_cpu_volt(struct mt_cpu_dvfs *p, unsigned int volt);
extern void aee_record_freq_idx(struct mt_cpu_dvfs *p, int idx);
extern void _mt_cpufreq_aee_init(void);
#endif

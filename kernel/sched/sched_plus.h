/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifdef CONFIG_SMP
extern unsigned long arch_scale_get_max_freq(int cpu);
extern unsigned long arch_scale_get_min_freq(int cpu);
#endif

extern int stune_task_threshold;
#ifdef CONFIG_SCHED_TUNE
extern bool global_negative_flag;

void show_ste_info(void);
void show_pwr_info(void);
#endif

#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
extern int l_plus_cpu;
#endif

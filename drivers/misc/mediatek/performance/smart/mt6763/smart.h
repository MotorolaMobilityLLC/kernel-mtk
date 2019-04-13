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

#ifndef _SMART_H_
#define _SMART_H_

/*-----------------------------------------------*/
/* prototype */
/*-----------------------------------------------*/

extern void sched_max_util_task(int *cpu, int *pid, int *util, int *boost);
extern void sched_big_task_nr(int *L_nr, int *B_nr);
extern unsigned int
sched_get_nr_heavy_task_by_threshold(int cluster_id, unsigned int threshold);
extern int sched_get_nr_heavy_running_avg(int cid, int *avg);
extern void sched_get_percpu_load2(int cpu, bool reset, unsigned int *rel_load,
				   unsigned int *abs_load);
extern int get_avg_heavy_task_threshold(void);
extern int get_heavy_task_threshold(void);
extern int sched_get_nr_running_avg(int *avg, int *iowait_avg);

#ifdef CONFIG_MTK_SCHED_RQAVG_KS
extern int sched_get_cluster_util(int id, unsigned long *util,
				  unsigned long *cap);
#endif

extern int smart_enter_turbo_mode(void);

#endif

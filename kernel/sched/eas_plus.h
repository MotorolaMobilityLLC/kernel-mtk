/*
 * Copyright (C) 2018 MediaTek Inc.
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

/*
 * Add a system-wide over-utilization indicator which
 * is updated in load-balance.
 */
static bool system_overutil;
extern int cpu_eff_tp;

inline bool system_overutilized(int cpu);

static inline unsigned long task_util(struct task_struct *p);
static bool is_intra_domain(int prev, int target);
static int select_max_spare_capacity(struct task_struct *p, int target);
static int init_cpu_info(void);
static unsigned int aggressive_idle_pull(int this_cpu);
bool idle_lb_enhance(struct task_struct *p, int cpu);
static int is_tiny_task(struct task_struct *p);
static int
__select_idle_sibling(struct task_struct *p, int prev_cpu, int new_cpu);

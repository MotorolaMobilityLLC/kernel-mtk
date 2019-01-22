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


/* turning_point:
 *	to help identify if "more-core at low freq" is suffered
 *	in share-bucked when the high OPP is triggered by heavy task.
 *  watershed:
 *	to quantify what ultra-low loading is "less-cores" prefer.
*/

#define DEFAULT_TURNINGPOINT 65
#define DEFAULT_WATERSHED 236

#if defined(CONFIG_MACH_MT6799)
/* LL: CA35 */
#define DEFAULT_SODI_LIMIT 400
#else
/* LL: CA53 */
#define DEFAULT_SODI_LIMIT 200
#endif

extern int stune_task_threshold;

struct power_tuning_t {
	int turning_point; /* max=100, default: 65% capacity */
	int watershed; /* max=1023 */
};


extern struct power_tuning_t *get_eas_power_setting(void);

/* Game Hint */
extern void (*ged_kpi_set_game_hint_value_fp)(int is_game_mode);

/* Stune */
#ifdef CONFIG_CGROUP_SCHEDTUNE
extern int group_boost_read(int group_idx);
#else
static int group_boost_read(int group_idx) { return 0; }
#endif

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
extern int dbg_id;
#endif

#ifdef CONFIG_MTK_SCHED_VIP_TASKS
extern void store_vip(int pid);
extern int get_vip_pid(int idx);
extern int get_vip_ref_count(void);
#endif

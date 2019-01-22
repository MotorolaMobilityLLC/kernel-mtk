/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FBT_USEDEXT_H__
#define __FBT_USEDEXT_H__


/* GED extern */
extern void (*ged_kpi_set_game_hint_value_fp_fbt)(int is_game_mode);
extern void (*ged_kpi_cpu_boost_fp_fbt)(
	long long t_cpu_cur,
	long long t_cpu_target,
	unsigned long long t_cpu_slptime,
	unsigned int target_fps);

extern int (*ged_kpi_push_game_frame_time_fp_fbt)(
	int pid,
	unsigned long long last_TimeStamp,
	unsigned long long curr_TimeStamp,
	unsigned long long *pRunningTime,
	unsigned long long *pSleepTime);

extern int (*ged_kpi_push_app_self_fc_fp_fbt)(int is_game_control_frame_rate, int pid);

extern void (*ged_kpi_cpu_boost_check_01)(
	int gx_game_mode,
	int gx_force_cpu_boost,
	int enable_cpu_boost,
	int ismainhead);

extern void (*ged_vsync_notifier_fp)(void);

/*  */
extern void (*cpufreq_notifier_fp)(int, unsigned long);

extern void (*update_lppcap)(unsigned int);

extern void (*set_fps)(unsigned int);

extern int (*fbt_notifier_dfrc_fp_fbt_cpu)(unsigned int DFRC_fpt_limit);

void fbt_cpu_set_game_hint_cb(int game);

#endif

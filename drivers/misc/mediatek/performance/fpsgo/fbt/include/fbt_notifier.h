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

#ifndef __FBT_NOTIFIER_H__
#define __FBT_NOTIFIER_H__

extern void fbt_notifier_exit(void);
extern int fbt_notifier_init(void);

/* for FBT CPU */
int fbt_notifier_push_cpu_frame_time(
	unsigned long long Q2Q_time,
	unsigned long long Runnging_time);

int fbt_notifier_push_cpu_capability(
	unsigned int curr_cap,
	unsigned int max_cap,
	unsigned int target_fps);

void fbt_notifier_push_game_mode(int is_game_mode);
void fbt_notifier_push_benchmark_hint(int is_benchmark);

/* for DFRC */
int fbt_notifier_push_dfrc_fps_limit(unsigned int fps_limit);

#endif

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

#ifndef STABLIZER_H
#define STABLIZER_H

#include <fpsgo_common.h>
#include <trace/events/fpsgo.h>

extern int (*fbt_notifier_cpu_frame_time_fps_stabilizer)(
	unsigned long long Q2Q_time,
	unsigned long long Runnging_time,
	unsigned int Curr_cap,
	unsigned int Max_cap,
	unsigned int Target_fps);
extern void (*ged_kpi_output_gfx_info_fp)(long long t_gpu, unsigned int cur_freq, unsigned int cur_max_freq);
extern void (*display_time_fps_stablizer)(unsigned long long ts);
extern void fbt_cpu_vag_set_fps(unsigned int fps);
extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);

#endif


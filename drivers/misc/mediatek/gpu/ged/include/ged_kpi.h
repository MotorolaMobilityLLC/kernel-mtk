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

#ifndef __GED_KPI_H__
#define __GED_KPI_H__

#include "ged_type.h"

GED_ERROR ged_kpi_queue_buffer_ts(int pid,
								unsigned long long ullWdnd,
								int i32FrameID,
								int fence,
								int QedBuffer_length,
								int isSF);
GED_ERROR ged_kpi_acquire_buffer_ts(int pid, unsigned long long ullWdnd, int i32FrameID);
GED_ERROR ged_kpi_sw_vsync(void);
GED_ERROR ged_kpi_hw_vsync(void);
int ged_kpi_get_uncompleted_count(void);

unsigned int ged_kpi_get_cur_fps(void);
unsigned int ged_kpi_get_cur_avg_cpu_time(void);
unsigned int ged_kpi_get_cur_avg_gpu_time(void);
unsigned int ged_kpi_get_cur_avg_response_time(void);
unsigned int ged_kpi_get_cur_avg_gpu_remained_time(void);
unsigned int ged_kpi_get_cur_avg_cpu_remained_time(void);
unsigned int ged_kpi_get_cur_avg_gpu_freq(void);
void ged_kpi_get_latest_perf_state(long long *t_cpu_remained,
									long long *t_gpu_remained,
									long *t_cpu_target,
									long *t_gpu_target);
GED_BOOL ged_kpi_set_target_fps(unsigned int target_fps, int mode);

GED_ERROR ged_kpi_system_init(void);
void ged_kpi_system_exit(void);
bool ged_kpi_set_cpu_remained_time(long long t_cpu_remained, int QedBuffer_length);
bool ged_kpi_set_gpu_dvfs_hint(int t_gpu_target, int t_gpu_cur);
void ged_kpi_set_game_hint(int mode);

extern int boost_value_for_GED_idx(int group_idx, int boost_value);
extern int linear_real_boost(int linear_boost);
#endif

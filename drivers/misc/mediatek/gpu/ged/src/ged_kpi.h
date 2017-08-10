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

GED_ERROR ged_kpi_gpu_ts(int tid, unsigned long long ullWdnd, int i32FrameID, int fence);
GED_ERROR ged_kpi_sw_vsync(void);
GED_ERROR ged_kpi_hw_vsync(void);
int ged_kpi_get_uncompleted_count(void);

unsigned int ged_kpi_get_fps(void);
unsigned int ged_kpi_get_avg_cpu_time(void);
unsigned int ged_kpi_get_avg_gpu_time(void);
unsigned int ged_kpi_get_avg_response_time(void);
unsigned int ged_kpi_get_avg_wait_4_hw_vsync_time(void);
unsigned int ged_kpi_get_avg_gpu_freq(void);

GED_ERROR ged_kpi_system_init(void);
void ged_kpi_system_exit(void);

#endif

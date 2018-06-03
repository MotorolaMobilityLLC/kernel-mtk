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

#ifndef __GED_FRAME_SYNC_H__
#define __GED_FRAME_SYNC_H__

#include "ged_type.h"

#define GED_FRR_TABLE_SIZE 8
#define GED_FRR_TABLE_FOR_ALL_PID -1
#define GED_FRR_TABLE_FOR_ALL_CID 0xFFFFFFFF

#define GED_FRR_TABLE_ZERO 0
#define GED_FRR_TABLE_LOWER_BOUND_FPS 30
#define GED_FRR_TABLE_UPPER_BOUND_FPS 60

GED_ERROR ged_frr_system_init(void);

GED_ERROR ged_frr_system_exit(void);

GED_ERROR ged_frr_table_set_fps(int targetPid, uint64_t targetCid, int targetFps);

int ged_frr_table_get_fps(int targetPid, uint64_t targetCid);

GED_ERROR ged_frr_wait_hw_vsync(void);
#endif

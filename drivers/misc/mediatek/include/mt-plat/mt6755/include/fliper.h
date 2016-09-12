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

#define LPM_MAX_BW 5200
#define HPM_MAX_BW 7200
#define THRESHOLD_SCALE 64

#define CG_LPM_BW_THRESHOLD 2000
#define CG_HPM_BW_THRESHOLD 1700
#define TOTAL_LPM_BW_THRESHOLD 4680
#define TOTAL_HPM_BW_THRESHOLD 3600
#define BW_THRESHOLD_MAX 9000
#define BW_THRESHOLD_MIN 100
#define CG_DEFAULT_LPM 2000
#define CG_DEFAULT_HPM 1700
#define CG_JUST_MAKE_LPM 3000
#define CG_JUST_MAKE_HPM 2700
#define CG_PERFORMANCE_LPM 1800
#define CG_PERFORMANCE_HPM 1500
enum {
	Default = 0,
	Low_Power_Mode = 1,
	Just_Make_Mode = 2,
	Performance_Mode = 3,
};


void enable_cg_fliper(int);
void enable_total_fliper(int);
int cg_set_threshold(int, int);
int total_set_threshold(int, int);
int cg_restore_threshold(void);
int total_restore_threshold(void);


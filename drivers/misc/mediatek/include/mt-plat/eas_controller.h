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

#define CGROUP_FG 1
#define EAS_KIR_PERF 0
#define EAS_KIR_FBC 1
#define EAS_MAX_KIR 2

extern int boost_value_for_GED_idx(int, int);
int perfmgr_kick_fg_boost(int kicker, int value);
void init_perfmgr_eas_controller(void);

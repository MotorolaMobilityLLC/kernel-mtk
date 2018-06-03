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

#ifndef __FBT_CPU_PLATFORM_H__
#define __FBT_CPU_PLATFORM_H__

#include "fbt_cpu.h"

enum fbt_cpu_freq_bound_type {
	FBT_CPU_FREQ_BOUND,
	FBT_CPU_RESCUE_BOUND,
};

void fbt_set_boost_value(int cluster, unsigned int base_blc);
void fbt_init_cpuset_freq_bound_table(void);

extern int cluster_num;
extern int cluster_freq_bound[MAX_FREQ_BOUND_NUM];
extern int cluster_rescue_bound[MAX_FREQ_BOUND_NUM];

#endif

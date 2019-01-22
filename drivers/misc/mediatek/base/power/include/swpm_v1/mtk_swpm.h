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
 */

#ifndef __MTK_SWPM_H__
#define __MTK_SWPM_H__

enum swpm_type {
	SWPM_CPU = 0,
	SWPM_GPU,
	SWPM_CORE,
	SWPM_MEM,

	NR_SWPM_TYPE,
};

extern unsigned int swpm_get_avg_power(enum swpm_type type,
				unsigned int avg_window);
#if 0 /* TODO: need this? */
extern void swpm_start_pmu(int cpu);
extern void swpm_stop_pmu(int cpu);
#endif

#endif /* __MTK_SWPM_H__ */


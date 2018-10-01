/*
 * (C) COPYRIGHT 2012-2013, 2015-2016 ARM Limited. All rights reserved.
 */





#ifndef _KBASE_CPU_VEXPRESS_H_
#define _KBASE_CPU_VEXPRESS_H_

/**
 * Versatile Express implementation of @ref kbase_cpu_clk_speed_func.
 */
int kbase_get_vexpress_cpu_clock_speed(u32 *cpu_clock);

/**
 * Get the minimum GPU frequency for the attached logic tile
 */
u32 kbase_get_platform_min_freq(void);

/**
 * Get the maximum GPU frequency for the attached logic tile
 */
u32 kbase_get_platform_max_freq(void);

#endif				/* _KBASE_CPU_VEXPRESS_H_ */

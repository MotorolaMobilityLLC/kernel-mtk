/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

/**********************************************
 * unified_power_internal.h
 * This header file includes:
 * 1. Externs of time profiling related APIs
 **********************************************/
#ifndef UNIFIED_POWER_INTERNAL_H
#define UNIFIED_POWER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/ktime.h>

#ifdef UPOWER_RCU_LOCK
extern void upower_read_lock(void);
extern void upower_read_unlock(void);
#endif

#ifdef UPOWER_PROFILE_API_TIME
enum {
	GET_PWR,
	GET_TBL_PTR,
	UPDATE_TBL_PTR,

	TEST_NUM
};
extern void upower_get_start_time_us(unsigned int type);
extern void upower_get_diff_time_us(unsigned int type);
extern void print_diff_results(unsigned int type);
#endif

#ifdef __cplusplus
}
#endif

#endif

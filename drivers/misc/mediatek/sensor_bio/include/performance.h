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

#ifndef __PERFORMANCE_H__
#define __PERFORMANCE_H__

#include <hwmsensor.h>
#include <linux/types.h>

enum SENSOR_STATUS {
	DATA_REPORT,
};

struct time_records {
	u64 sum_kernel_time;
	u16 count;
};

#define LIMIT 1000

/* #define DEBUG_PERFORMANCE */

#ifdef DEBUG_PERFORMANCE
static struct time_records record;
static inline void mark_timestamp(u8 sensor_type, enum SENSOR_STATUS status, u64 current_time,
				  u64 event_time)
{
	if (status == DATA_REPORT) {
		record.sum_kernel_time += current_time - event_time;
		record.count++;
		if (record.count == LIMIT) {
			pr_warn("sensor[%d] ====> kernel report time:%lld\n", sensor_type,
				record.sum_kernel_time / LIMIT);
			record.sum_kernel_time = 0;
			record.count = 0;
		}
	}
}
#else
#define mark_timestamp(A, B, C, D)
#endif

#endif

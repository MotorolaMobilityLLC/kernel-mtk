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

#include <linux/kernel.h>
#include <linux/module.h>
#include "ged.h"
#include "ged_kpi.h"

void ged_notification(GED_NOTIFICATION_TYPE eType)
{
	switch (eType) {
	case GED_NOTIFICATION_TYPE_SW_VSYNC:
		ged_kpi_sw_vsync();
		break;
	case GED_NOTIFICATION_TYPE_HW_VSYNC_PRIMARY_DISPLAY:
		ged_kpi_hw_vsync();
		break;
	}
}
EXPORT_SYMBOL(ged_notification);
int ged_set_target_fps(unsigned int target_fps, int mode)
{
	if (ged_kpi_set_target_fps(target_fps, mode) == GED_TRUE)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(ged_set_target_fps);
void ged_get_latest_perf_state(long long *t_cpu_remained,
								long long *t_gpu_remained,
								long *t_cpu_target,
								long *t_gpu_target)
{
	ged_kpi_get_latest_perf_state(t_cpu_remained, t_gpu_remained, t_cpu_target, t_gpu_target);
}
unsigned int ged_get_cur_fps(void)
{
	return ged_kpi_get_cur_fps();
}

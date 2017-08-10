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
	switch(eType)
	{
	case GED_NOTIFICATION_TYPE_SW_VSYNC:
		ged_kpi_sw_vsync();
		break;
	case GED_NOTIFICATION_TYPE_HW_VSYNC_PRIMARY_DISPLAY:		
		ged_kpi_hw_vsync();
		break;
	}
}
EXPORT_SYMBOL(ged_notification);


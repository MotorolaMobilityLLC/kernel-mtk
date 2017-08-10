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

#ifndef __GED_H__
#define __GED_H__

#include "ged_type.h"

typedef enum GED_NOTIFICATION_TYPE_TAG
{
	GED_NOTIFICATION_TYPE_SW_VSYNC,
	GED_NOTIFICATION_TYPE_HW_VSYNC_PRIMARY_DISPLAY,
} GED_NOTIFICATION_TYPE;

void ged_notification(GED_NOTIFICATION_TYPE eType);

#endif

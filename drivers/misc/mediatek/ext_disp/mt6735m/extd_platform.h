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
#ifndef __EXTD_PLATFORM_H__
#define __EXTD_PLATFORM_H__

#include "ddp_hal.h"
#include "disp_drv_platform.h"


#define MAX_SESSION_COUNT		5

/* #define MTK_LCD_HW_3D_SUPPORT */
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))


#define MTK_EXT_DISP_OVERLAY_SUPPORT

/* /#define EXTD_DBG_USE_INNER_BUF */

/*#define HW_OVERLAY_COUNT  4*/
#define EXTD_OVERLAY_CNT  0
#define HW_DPI_VSYNC_SUPPORT 0

#define DISP_MODULE_RDMA DISP_MODULE_RDMA1
#endif

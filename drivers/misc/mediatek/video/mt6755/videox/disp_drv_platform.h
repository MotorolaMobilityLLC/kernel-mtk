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

#ifndef __DISP_DRV_PLATFORM_H__
#define __DISP_DRV_PLATFORM_H__

#include <linux/dma-mapping.h>
#include "m4u.h"
/*#include <mach/mt_reg_base.h>*/
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif

#include <mt-plat/sync_write.h>
/*#include <board-custom.h>*/
#include "disp_assert_layer.h"

#include "ddp_hal.h"
#include "ddp_drv.h"
#include "ddp_path.h"
#include "ddp_rdma.h"

/* #include <mach/mt6585_pwm.h> */
/* #include <mach/boot.h> */

#define ALIGN_TO(x, n)  (((x) + ((n) - 1)) & ~((n) - 1))

#define MTK_FB_ALIGNMENT 32   /*Hardware 3D */
/*#define MTK_FB_ALIGNMENT 1	// SW 3D */
#define MTK_FB_ION_SUPPORT
#define VIDEO_LAYER_COUNT            (3)
/* #define HW_OVERLAY_COUNT                  (4) */

#define PRIMARY_SESSION_INPUT_LAYER_COUNT			(8)
#define EXTERNAL_SESSION_INPUT_LAYER_COUNT			(4)
#define MEMORY_SESSION_INPUT_LAYER_COUNT			(4)
#define DISP_SESSION_OVL_TIMELINE_ID(x)				(x)

/* Display HW Capabilities */
#define DISP_HW_MODE_CAP DISP_OUTPUT_CAP_SWITCHABLE
#define DISP_HW_PASS_MODE DISP_OUTPUT_CAP_SINGLE_PASS
#define DISP_HW_MAX_LAYER 4

typedef enum {
	DISP_SESSION_OUTPUT_TIMELINE_ID = PRIMARY_SESSION_INPUT_LAYER_COUNT,
	DISP_SESSION_PRESENT_TIMELINE_ID,
	DISP_SESSION_OUTPUT_INTERFACE_TIMELINE_ID,
	DISP_SESSION_TIMELINE_COUNT,
} DISP_SESSION_ENUM;

#define MAX_SESSION_COUNT					5

/* macros for display path hardware */
#define DISP_HW_HRT_LYAERS_FOR_LOW_POWER	4
#define DISP_HW_HRT_LYAERS_FOR_HI_PERF		6
#define DISP_HW_HRT_720P_LYAERS_FOR_HI_PERF	6

#define DISP_HW_HRT_PERF_FOR_LCM_BIG	    4
#define DISP_HW_HRT_PERF_FOR_LCM_BIG_MAX	6
#define DISP_HW_HRT_PERF_FOR_LCM_SMALL    6
#define DISP_HW_HRT_PERF_LCM_AREA_THRESHOLD   (1280 * 800)

/* Other platform-dependent features */
#define DISP_PATH_DELAYED_TRIGGER_33ms_SUPPORT

#endif				/* __DISP_DRV_PLATFORM_H__ */

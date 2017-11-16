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
#include <linux/types.h>
/* #include <mach/mt_gpio.h> */
#include "m4u.h"
/* #include <mach/mt6585_pwm.h> */
/* #include <mach/mt_reg_base.h> */
#include <mach/mt_clkmgr.h>
/* #include <mach/mt_irq.h> */
/* #include <mach/boot.h> */
/* #include <board-custom.h> */
#include "ddp_hal.h"
#include "ddp_drv.h"
#include "ddp_path.h"

#include <mt-plat/sync_write.h>

/* #define OVL_CASCADE_SUPPORT */
/* sync with define in ddp_ovl.h */
#ifdef OVL_CASCADE_SUPPORT
#define MAX_INPUT_CONFIG		8
#else
#define MAX_INPUT_CONFIG		4
#endif

#define M4U_PROFILE
/* #define MTKFB_NO_M4U */
/* #define MTK_LCD_HW_3D_SUPPORT */
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))
#define MTK_FB_ALIGNMENT 32
#define MTK_FB_START_DSI_ISR
#define MTK_FB_OVERLAY_SUPPORT
#define MTK_FB_SYNC_SUPPORT
#define MTK_FB_ION_SUPPORT
#define HW_OVERLAY_COUNT                 (4)
#define RESERVED_LAYER_COUNT             (2)
#define VIDEO_LAYER_COUNT                (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)

#define DFO_USE_NEW_API
#define MTK_FB_DFO_DISABLE

/**
 * Support manual control MUTEX HW_DCM
 */
#define MANUAL_MUTEX_HW_DCM

/* new macro definition for display driver platform dependency options */
#define PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER		(4)
#define PRIMARY_DISPLAY_HW_OVERLAY_ENGINE_COUNT		(1)
#define PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT	(1) /* if use 2 ovl 2 times in/out, this count could be 1.75 */
#define PRIMARY_DISPLAY_SESSION_LAYER_COUNT	\
	(PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER*PRIMARY_DISPLAY_HW_OVERLAY_CASCADE_COUNT)

#define EXTERNAL_DISPLAY_SESSION_LAYER_COUNT			(PRIMARY_DISPLAY_HW_OVERLAY_LAYER_NUMBER)

#define DISP_SESSION_OVL_TIMELINE_ID(x)		(x)

/* Display HW Capabilities */
#define DISP_HW_MODE_CAP DISP_OUTPUT_CAP_SWITCHABLE
#define DISP_HW_PASS_MODE DISP_OUTPUT_CAP_SINGLE_PASS
#define DISP_HW_MAX_LAYER 4

typedef enum {
	DISP_SESSION_OUTPUT_TIMELINE_ID = PRIMARY_DISPLAY_SESSION_LAYER_COUNT,
	DISP_SESSION_PRESENT_TIMELINE_ID,
	DISP_SESSION_OUTPUT_INTERFACE_TIMELINE_ID,
	DISP_SESSION_TIMELINE_COUNT,
} DISP_SESSION_ENUM;

#define MAX_SESSION_COUNT					5
/* #define DISP_SWITCH_DST_MODE */

/**
 * DISP_NO_MT_BOOT: option for mt_boot
 */
#define DISP_NO_MT_BOOT

/**
 * DISP_NO_AEE
 */
#define DISP_NO_AEE

#endif /* __DISP_DRV_PLATFORM_H__ */

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

#ifndef __MTK_HRT_H__
#define __MTK_HRT_H__

#include "disp_session.h"
#include "disp_lcm.h"
#include "disp_drv_log.h"
#include "primary_display.h"
#include "disp_drv_platform.h"
#include "display_recorder.h"

#define FHDP_H	2160 /* 9:18, 1080:2160 */
#define HDP_H	1440 /* 9:18, 720:1440 */

/* 60 fps */
#define WQHD_LARB_LOWER_BOUND	3
#define WQHD_LARB_UPPER_BOUND	4
#define WQHD_EMI_EXTREME_LOWER_BOUND 2
#define WQHD_EMI_LOWER_BOUND	4
#define WQHD_EMI_UPPER_BOUND	6

#define FHDP_LARB_LOWER_BOUND	6
#define FHDP_LARB_UPPER_BOUND	8
#define FHDP_EMI_EXTREME_LOWER_BOUND 3
#define FHDP_EMI_LOWER_BOUND	8
#define FHDP_EMI_UPPER_BOUND	10

#define HDP_LARB_LOWER_BOUND	13
#define HDP_LARB_UPPER_BOUND	18
#define HDP_EMI_LOWER_BOUND	18
#define HDP_EMI_UPPER_BOUND	22

/* 120 fps */
#define OD_LARB_LOWER_BOUND	2
#define OD_LARB_UPPER_BOUND	3
#define OD_EMI_LOWER_BOUND	3
#define OD_EMI_UPPER_BOUND	4

#define PRIMARY_OVL_LAYER_NUM PRIMARY_SESSION_INPUT_LAYER_COUNT
#define SECONDARY_OVL_LAYER_NUM EXTERNAL_SESSION_INPUT_LAYER_COUNT

#ifdef CONFIG_MTK_DISPLAY_120HZ_SUPPORT
#define HRT_LEVEL(id) ((id)&0xff)
#define HRT_FPS(id) (((id)>>8)&0xff)
#define MAKE_HRT_NUM(fps, level) (unsigned int)((fps)<<8 | (level))
#endif

/* #define HRT_DEBUG */

int dispsys_hrt_calc(disp_layer_info *disp_info);

typedef struct hrt_sort_entry_t {
	struct hrt_sort_entry_t *head, *tail;
	layer_config *layer_info;
	int key;
	int overlap_w;
} hrt_sort_entry;

typedef enum {
	HRT_LEVEL_EXTREME_LOW = 0,
	HRT_LEVEL_LOW,
	HRT_LEVEL_HIGH,
	HRT_OVER_LIMIT,
} HRT_LEVEL;

typedef enum {
	HRT_PRIMARY = 0,
	HRT_SECONDARY,
} HRT_DISP_TYPE;

extern int hdmi_get_dev_info(int is_sf, void *info);

#endif

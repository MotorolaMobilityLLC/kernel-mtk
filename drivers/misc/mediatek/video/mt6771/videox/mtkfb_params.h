/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef __H_MTKFB_PARAMS__
#define __H_MTKFB_PARAMS__

/* HBM implementation is different, depending on display and backlight hardware
 * design, which is classified into the following types:
 * HBM_TYPE_OLED: OLED panel, HBM is controlled by DSI register only, which
 *     is independent on brightness.
 * HBM_TYPE_LCD_DCS_WLED: LCD panel, HBM is controlled by DSI register, and
 *     brightness is decided by WLED IC on I2C/SPI bus.
 * HBM_TYPE_LCD_DCS_ONLY: LCD panel, brightness/HBM is controlled by DSI
 *     register only.
 * HBM_TYPE_LCD_WLED_ONLY: LCD panel, brightness/HBM is controlled by WLED
 *     IC only.
 *
 * Note: brightness must be at maximum while enabling HBM for all LCD panels
 */
#define HBM_TYPE_OLED	0
#define HBM_TYPE_LCD_DCS_WLED	1
#define HBM_TYPE_LCD_DCS_ONLY	2
#define HBM_TYPE_LCD_WLED_ONLY	3

enum hbm_state {
	HBM_OFF_STATE = 0,
	HBM_ON_STATE,
	HBM_STATE_NUM
};

enum cabc_mode {
	CABC_DIS_MODE = 0,
	CABC_UI_MODE,
	CABC_ST_MODE,
	CABC_MV_MODE,
	CABC_MODE_NUM
};

enum panel_param_id {
	PARAM_HBM_ID = 0,
	PARAM_CABC_ID,
	PARAM_ID_NUM
};

#endif

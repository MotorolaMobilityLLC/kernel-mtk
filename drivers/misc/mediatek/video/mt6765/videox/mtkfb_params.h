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

#ifndef __MTKFB_PARAMS_H__
#define __MTKFB_PARAMS_H__

typedef enum {
    PARAM_HBM = 0,
    PARAM_CABC,
    PARAM_MAX_NUM
} paramId_t;

enum hbm_mode {
	HBM_MODE_GPIO = 0,
	HBM_MODE_DCS_ONLY,
	HBM_MODE_DCS_GPIO,
};

enum hbm_state {
	HBM_OFF_STATE = 0,
	HBM_ON_STATE,
	HBM_STATE_NUM
};

enum cabc_mode {
	CABC_UI_MODE = 0,
	CABC_MV_MODE,
	CABC_DIS_MODE,
	CABC_MODE_NUM
};

#endif /* __MTKFB_PARAMS_H__ */

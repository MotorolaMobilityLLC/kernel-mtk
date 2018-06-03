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

#ifndef __IMGSENSOR_COMMON_H__
#define __IMGSENSOR_COMMON_H__

#include <linux/types.h>

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PREFIX "[imgsensor]"

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG(fmt, arg...)  pr_debug(PREFIX fmt, ##arg)
#define PK_ERR(fmt, arg...)  pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...) pr_debug(PREFIX fmt, ##arg)
#else
#define PK_DBG(fmt, arg...)
#define PK_ERR(fmt, arg...)  pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...) pr_debug(PREFIX fmt, ##arg)
#endif

typedef enum {
	IMGSENSOR_HW_PIN_NONE = 0,
	IMGSENSOR_HW_PIN_PDN,
	IMGSENSOR_HW_PIN_RST,
	IMGSENSOR_HW_PIN_AVDD,
	IMGSENSOR_HW_PIN_DVDD,
	IMGSENSOR_HW_PIN_DOVDD,
	IMGSENSOR_HW_PIN_AFVDD,

	IMGSENSOR_HW_PIN_MCLK1,
	IMGSENSOR_HW_PIN_MCLK2,
	IMGSENSOR_HW_PIN_MCLK3,
	IMGSENSOR_HW_PIN_MCLK4,

	IMGSENSOR_HW_PIN_MAX_NUM,
	IMGSENSOR_HW_PIN_UNDEF = -1
} IMGSENSOR_HW_PIN;

typedef enum {
	IMGSENSOR_HW_PIN_STATE_LEVEL_0,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1000,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1100,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1200,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1210,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1220,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1500,
	IMGSENSOR_HW_PIN_STATE_LEVEL_1800,
	IMGSENSOR_HW_PIN_STATE_LEVEL_2800,
	IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,

	IMGSENSOR_HW_PIN_STATE_NONE = -1
} IMGSENSOR_HW_PIN_STATE;

typedef enum {
	IMGSENSOR_RETURN_SUCCESS = 0,
	IMGSENSOR_RETURN_ERROR   = -1,
} IMGSENSOR_RETURN;

typedef enum {
	IMGSENSOR_SENSOR_IDX_MAIN,
	IMGSENSOR_SENSOR_IDX_SUB,
	IMGSENSOR_SENSOR_IDX_MAIN2,
	IMGSENSOR_SENSOR_IDX_SUB2,
	IMGSENSOR_SENSOR_IDX_MAX_NUM,
	IMGSENSOR_SENSOR_IDX_NONE = -1
} IMGSENSOR_SENSOR_IDX;

#define IMGSENSOR_HW_POWER_INFO_MAX	12
#define IMGSENSOR_HW_SENSOR_MAX_NUM	8

typedef enum {
	IMGSENSOR_HW_POWER_STATUS_OFF,
	IMGSENSOR_HW_POWER_STATUS_ON
} IMGSENSOR_HW_POWER_STATUS;

typedef enum {
	IMGSENSOR_HW_ID_MCLK,
	IMGSENSOR_HW_ID_REGULATOR,
	IMGSENSOR_HW_ID_MT6306,

	IMGSENSOR_HW_ID_MAX_NUM,
	IMGSENSOR_HW_ID_NONE = -1
} IMGSENSOR_HW_ID;

typedef struct {
	void              *pinstance;
	IMGSENSOR_RETURN (*init)(void *);
	IMGSENSOR_RETURN (*set) (void *,
	                         IMGSENSOR_SENSOR_IDX,
	                         IMGSENSOR_HW_PIN,
	                         IMGSENSOR_HW_PIN_STATE);
	IMGSENSOR_HW_ID    id;
} IMGSENSOR_HW_DEVICE;

typedef struct {
	IMGSENSOR_HW_ID  id;
	IMGSENSOR_HW_PIN pin;
} IMGSENSOR_HW_CUSTOM_POWER_INFO;

typedef struct {
	IMGSENSOR_SENSOR_IDX           sensor_idx;
	IMGSENSOR_HW_CUSTOM_POWER_INFO pwr_info[IMGSENSOR_HW_POWER_INFO_MAX];
} IMGSENSOR_HW_CUSTOM_POWER_CFG;

typedef struct {
	IMGSENSOR_HW_PIN       pin;
	IMGSENSOR_HW_PIN_STATE pin_state_on;
	u32 pin_on_delay;
	IMGSENSOR_HW_PIN_STATE pin_state_off;
	u32 pin_off_delay;
} IMGSENSOR_HW_POWER_INFO;

typedef struct {
	char                   *sensor_name;
	IMGSENSOR_HW_POWER_INFO pwr_info[IMGSENSOR_HW_POWER_INFO_MAX];
} IMGSENSOR_HW_SENSOR_POWER_SEQ;

typedef struct {
	IMGSENSOR_SENSOR_IDX    sensor_idx;
	IMGSENSOR_HW_POWER_INFO pwr_info[IMGSENSOR_HW_POWER_INFO_MAX];
} IMGSENSOR_HW_PLATFORM_POWER_SEQ;

typedef IMGSENSOR_RETURN (*imgsensor_hw_open)(IMGSENSOR_HW_DEVICE **);

#endif


/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __IMGSENSOR_HW_H__
#define __IMGSENSOR_HW_H__

#include "imgsensor_sensor.h"
#include "imgsensor_custom.h"
#include "imgsensor_common.h"

enum IMGSENSOR_HW_POWER_STATUS {
	IMGSENSOR_HW_POWER_STATUS_OFF,
	IMGSENSOR_HW_POWER_STATUS_ON
};

struct IMGSENSOR_HW_SENSOR_POWER {
	struct IMGSENSOR_HW_POWER_INFO *ppwr_info;
	enum   IMGSENSOR_HW_ID          id[IMGSENSOR_HW_PIN_MAX_NUM];
};

struct IMGSENSOR_HW {
	struct IMGSENSOR_HW_DEVICE      *pdev[IMGSENSOR_HW_ID_MAX_NUM];
	struct IMGSENSOR_HW_SENSOR_POWER sensor_pwr[IMGSENSOR_SENSOR_IDX_MAX_NUM];
};

enum IMGSENSOR_RETURN imgsensor_hw_init(struct IMGSENSOR_HW *phw);
enum IMGSENSOR_RETURN imgsensor_hw_power(
	struct IMGSENSOR_HW *phw,
	struct IMGSENSOR_SENSOR *psensor,
	char *curr_sensor_name,
	enum IMGSENSOR_HW_POWER_STATUS pwr_status);

#if 0
enum IMGSENSOR_RETURN imgsensor_hw_power_add_sequence(struct IMGSENSOR_HW_POWER_SEQ *ppwr_seq);
#endif

#endif


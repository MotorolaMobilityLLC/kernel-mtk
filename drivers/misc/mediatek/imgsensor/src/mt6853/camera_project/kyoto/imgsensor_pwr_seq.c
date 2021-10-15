/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "kd_imgsensor.h"

#include "imgsensor_hw.h"
#include "imgsensor_cfg_table.h"

struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
#if defined(MOT_S5KHM2_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_S5KHM2_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(MOT_S5KHM2QTECH_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_S5KHM2QTECH_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(MOT_OV32B40_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_OV32B40_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{AVDD, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_High, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(MOT_OV02B1B_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_OV02B1B_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_High, 11},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(MOT_S5K4H7_MIPI_RAW)
	{
		SENSOR_DRVNAME_MOT_S5K4H7_MIPI_RAW,
		{
			{RST, Vol_Low, 1},
			{SensorMCLK, Vol_High, 1},
			{AVDD, Vol_High, 1},
			{DVDD, Vol_High, 1},
			{DOVDD, Vol_1800, 1},
			{RST, Vol_High, 5},
		},
	},
#endif
	/* add new sensor before this line */
	{NULL,},
};


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

#include "regulator/regulator.h"
#include "gpio/gpio.h"
/*#include "mt6306/mt6306.h"*/
#include "mclk/mclk.h"



#include "imgsensor_cfg_table.h"

enum IMGSENSOR_RETURN
	(*hw_open[IMGSENSOR_HW_ID_MAX_NUM])(struct IMGSENSOR_HW_DEVICE **) = {
	imgsensor_hw_regulator_open,
	imgsensor_hw_gpio_open,
	/*imgsensor_hw_mt6306_open,*/
	imgsensor_hw_mclk_open
};

struct IMGSENSOR_HW_CFG imgsensor_custom_config[] = {
	{
		IMGSENSOR_SENSOR_IDX_MAIN,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD},
#ifdef CAMERA_CHROMATIX_NOISE
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD_CN},
#endif
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
#ifdef CAMERA_CHROMATIX_NOISE
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD_CN},
#endif
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
#ifdef MIPI_SWITCH
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
#endif
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
#ifdef CAMERA_CHROMATIX_NOISE
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD_CN},
#endif
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
#ifdef MIPI_SWITCH
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
#endif
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB2,
		IMGSENSOR_I2C_DEV_3,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
#ifdef CAMERA_CHROMATIX_NOISE
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD_CN},
#endif
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN3,
		IMGSENSOR_I2C_DEV_3,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
#ifdef CAMERA_CHROMATIX_NOISE
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD_CN},
#endif
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},

	{IMGSENSOR_SENSOR_IDX_NONE}
};

struct IMGSENSOR_HW_POWER_SEQ platform_power_sequence[] = {
#ifdef MIPI_SWITCH
	{
		PLATFORM_POWER_SEQ_NAME,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
		},
		IMGSENSOR_SENSOR_IDX_SUB,
	},
	{
		PLATFORM_POWER_SEQ_NAME,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
		},
		IMGSENSOR_SENSOR_IDX_SUB2,
	},
#endif

	{NULL}
};

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
/* Add Corfu start */
#if defined(MOT_COFUL_S5KJN1_QTECH)
        {
                SENSOR_DRVNAME_MOT_COFUL_S5KJN1_QTECH,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DOVDD, Vol_1800, 1},
                        {DVDD, Vol_1100, 1},
                        {AVDD, Vol_2800, 1},
#ifdef CAMERA_CHROMATIX_NOISE
                        {AVDDCN, Vol_1800, 1},
#endif
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_CORFU_HI1336_OFILM)
        {
                SENSOR_DRVNAME_MOT_CORFU_HI1336_OFILM,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DOVDD, Vol_1800, 1},
                        {AVDD, Vol_2800, 1},
#ifdef CAMERA_CHROMATIX_NOISE
                        {AVDDCN, Vol_1800, 1},
#endif
                        {DVDD, Vol_1100, 1},
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_CORFU_HI1336_OFILM_DOE)
        {
                SENSOR_DRVNAME_MOT_CORFU_HI1336_OFILM_DOE,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DOVDD, Vol_1800, 1},
                        {AVDD, Vol_2800, 1},
#ifdef CAMERA_CHROMATIX_NOISE
                        {AVDDCN, Vol_1800, 1},
#endif
                        {DVDD, Vol_1100, 1},
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_COFUL_S5K4H7_QTECH)
        {
                SENSOR_DRVNAME_MOT_COFUL_S5K4H7_QTECH,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DVDD, Vol_1200, 1},
                        {AVDD, Vol_2800, 1},
#ifdef CAMERA_CHROMATIX_NOISE
                        {AVDDCN, Vol_1800, 1},
#endif
                        {DOVDD, Vol_1800, 1},
                        {RST, Vol_High, 5}
                },
        },
#endif
#if defined(MOT_COFUL_GC02M1_TSP)
        {
                SENSOR_DRVNAME_MOT_COFUL_GC02M1_TSP,
                {
                        {RST, Vol_Low, 1},
                        {SensorMCLK, Vol_High, 1},
                        {DVDD, Vol_High, 1},
                        {DOVDD, Vol_1800, 1},
                        {AVDD, Vol_2800, 1},
#ifdef CAMERA_CHROMATIX_NOISE
                        {AVDDCN, Vol_1800, 1},
#endif
                        {RST, Vol_High, 5}
                },
        },
#endif
/* Add Corfu end */
	/* add new sensor before this line */
	{NULL,},
};


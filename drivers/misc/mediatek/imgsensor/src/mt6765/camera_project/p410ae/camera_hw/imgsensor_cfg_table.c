// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AFVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_EN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB2,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_EN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN3,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
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
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
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
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
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
#if defined(S5KJN1_QT_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5KJN1_QT_P410AE_MIPI_RAW,
		{
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 1},
			{AVDD, Vol_2800, 1},
			{AFVDD, Vol_2800, 5},
			{RST, Vol_High, 1},
			{SensorMCLK, Vol_High, 5},
		},
	},
#endif
#if defined(HI1336_QTECH_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI1336_QTECH_P410AE_MIPI_RAW,
		{
			{RST, Vol_Low, 3},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1100, 3},
			{AFVDD, Vol_2800, 1},
			{SensorMCLK, Vol_High, 1},
			{RST, Vol_High, 10},
		},
	},
#endif
#if defined(HI1634_QT_P411BE_MIPI_RAW)
        {
                SENSOR_DRVNAME_HI1634_QT_P411BE_MIPI_RAW,
                {
                   {RST, Vol_Low, 3},
                   {AVDD,Vol_2800,1},
                   {DOVDD, Vol_1800, 1},
                   {DVDD, Vol_1100, 3},
                   {AFVDD, Vol_2800, 1},
                   {SensorMCLK,Vol_High,1},
                   {RST, Vol_High, 10},
                },
        },
#endif
#if defined(S5K4H7SUB_QT_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4H7SUB_QT_P410AE_MIPI_RAW,
		{
		   {SensorMCLK, Vol_High, 1},
		   {RST, Vol_Low, 1},
		   //{PDN, Vol_Low, 1},
		   {DOVDD, Vol_1800, 1},
		   //{DVDD, Vol_1200, 2},
		   {DVDD, Vol_1200, 5},
		   {AVDD, Vol_2800, 5},
		   {RST, Vol_High, 3},
		   //{PDN, Vol_High, 3},
		},
	},
#endif
#if defined(OV16A1Q_QT_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16A1Q_QT_P410AE_MIPI_RAW,
		{
		   {RST, Vol_Low, 1},
		   {SensorMCLK, Vol_High, 1},
		   {DOVDD, Vol_1800, 1},
		   {DVDD, Vol_1200, 1},
		   {AVDD, Vol_2800, 1},
		   {RST, Vol_High, 5},
		},
	},
#endif
#if defined(HI1634_QT_P410AE_MIPI_RAW)
        {
                SENSOR_DRVNAME_HI1634_QT_P410AE_MIPI_RAW,
                {
                   {RST, Vol_Low, 3},
                   {DOVDD, Vol_1800, 1},
                   {AVDD, Vol_2800, 1},
                   {DVDD, Vol_1100, 3},
                   {AFVDD, Vol_2800, 1},
                   {SensorMCLK,Vol_High,1},
                   {RST, Vol_High, 10},
                },
        },
#endif
#if defined(S5K4H7WIDE_QT_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4H7WIDE_QT_P410AE_MIPI_RAW,
		{
		   {SensorMCLK, Vol_High, 1},
		   {RST, Vol_Low, 1},
		   //{PDN, Vol_Low, 1},
		   //{DVDD, Vol_1200, 2},
		   {AVDD, Vol_2800, 5},
		   {DVDD, Vol_1200, 5},
		   {DOVDD, Vol_1800, 2},
		   {RST, Vol_High, 3},
		   //{PDN, Vol_High, 3},
		   //{SensorMCLK, Vol_High, 1},

		},
	},
#endif
#if defined(GC2M1B_TSP_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC2M1B_TSP_P410AE_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{SensorMCLK, Vol_High, 1},
		},
	},
#endif
#if defined(GC02M1_TSP_P410AE_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC02M1_TSP_P410AE_MIPI_RAW,
		{
			{PDN, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{SensorMCLK, Vol_High, 1},
		},
	},
#endif
	/* add new sensor before this line */
	{NULL,},
};


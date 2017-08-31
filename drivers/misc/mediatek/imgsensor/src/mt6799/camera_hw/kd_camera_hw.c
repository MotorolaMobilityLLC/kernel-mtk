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

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <asm/atomic.h>

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include "kd_camera_hw.h"

/* kernel standard for PMIC*/
#if !defined(CONFIG_MTK_LEGACY)
/* PMIC */
#include <linux/regulator/consumer.h>
#endif

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG(fmt, arg...)			pr_debug(PFX fmt, ##arg)
#define PK_ERR(fmt, arg...)         pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...) 		pr_debug(PFX fmt, ##arg)
#else
#define PK_DBG(fmt, arg...)
#define PK_ERR(fmt, arg...)			pr_err(fmt, ##arg)
#define PK_INFO(fmt, arg...)		pr_debug(PFX fmt, ##arg)
#endif


#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3


#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4



u32 pinSet[3][8] = {
	/* for main sensor */
	{
		CAMERA_CMRST_PIN,
		CAMERA_CMRST_PIN_M_GPIO,	/* mode */
		GPIO_OUT_ONE,		/* ON state */
		GPIO_OUT_ZERO,		/* OFF state */
		CAMERA_CMPDN_PIN,
		CAMERA_CMPDN_PIN_M_GPIO,
		GPIO_OUT_ONE,
		GPIO_OUT_ZERO,
	},
	/* for sub sensor */
	{
		CAMERA_CMRST1_PIN,
		CAMERA_CMRST1_PIN_M_GPIO,
		GPIO_OUT_ONE,
		GPIO_OUT_ZERO,
		CAMERA_CMPDN1_PIN,
		CAMERA_CMPDN1_PIN_M_GPIO,
		GPIO_OUT_ONE,
		GPIO_OUT_ZERO,
	},
	/* for Main2 sensor */
	{
		CAMERA_CMRST2_PIN,
		CAMERA_CMRST2_PIN_M_GPIO,
		GPIO_OUT_ONE,
		GPIO_OUT_ZERO,
		CAMERA_CMPDN2_PIN,
		CAMERA_CMPDN2_PIN_M_GPIO,
		GPIO_OUT_ONE,
		GPIO_OUT_ZERO,
	},
};

#ifndef CONFIG_MTK_LEGACY
#define CUST_AVDD AVDD - AVDD
#define CUST_DVDD DVDD - AVDD
#define CUST_DOVDD DOVDD - AVDD
#define CUST_AFVDD AFVDD - AVDD
#define CUST_SUB_AVDD SUB_AVDD - AVDD
#define CUST_SUB_DVDD SUB_DVDD - AVDD
#define CUST_SUB_DOVDD SUB_DOVDD - AVDD
#define CUST_MAIN2_AVDD MAIN2_AVDD - AVDD
#define CUST_MAIN2_DVDD MAIN2_DVDD - AVDD
#define CUST_MAIN2_DOVDD MAIN2_DOVDD - AVDD

#endif


PowerCustInfo PowerCustList[] = {
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for AVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for DVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for DOVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for AFVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for SUB_AVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for SUB_DVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for SUB_DOVDD; */
	{GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_AVDD; */
	{GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_DVDD; */
	{GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_DOVDD; */
	/* {GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_Low}, */
};

PowerSequence PowerOnList[] = {
#if defined(IMX398_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX398_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV23850_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV23850_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX386_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX386_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif

#if defined(IMX338_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX338_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K4E6_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4E6_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K3P8SP_MIPI_RAW)
	  {SENSOR_DRVNAME_S5K3P8SP_MIPI_RAW,
		  {
			  {SensorMCLK, Vol_High, 0},
			  {DOVDD, Vol_1800, 0},
			  {AVDD, Vol_2800, 0},
			  {DVDD, Vol_1000, 0},
			  {AFVDD, Vol_2800, 5},
			  {PDN, Vol_Low, 4},
			  {PDN, Vol_High, 0},
			  {RST, Vol_Low, 1},
			  {RST, Vol_High, 0},
		  },
	  },
#endif
#if defined(S5K3M2_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M2_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K3P3SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K5E2YA_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K4ECGX_MIPI_YUV)
	{
		SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV,
		{
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV16880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K2P8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX258_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX258_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX377_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX377_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV8858_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8858_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K2X8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2X8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX214_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX214_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX214_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX214_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX230_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX230_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K3L8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX362_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX362_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K2L7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2L7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 3},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX318_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX318_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV8865_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8865_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(IMX219_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX219_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1000, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(S5K3M3_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M3_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW_2)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW_2,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5},
			{VDD_None, Vol_Low, 0}
		},
	},
#endif
	/* add new sensor before this line */
	{NULL,},
};


#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1220 1220000
#define VOL_1000 1000000

/* GPIO Pin control*/
struct platform_device *cam_plt_dev = NULL;
struct pinctrl *camctrl = NULL;
struct pinctrl_state *cam0_pnd_h = NULL;/* main cam */
struct pinctrl_state *cam0_pnd_l = NULL;
struct pinctrl_state *cam0_rst_h = NULL;
struct pinctrl_state *cam0_rst_l = NULL;
struct pinctrl_state *cam1_pnd_h = NULL;/* sub cam */
struct pinctrl_state *cam1_pnd_l = NULL;
struct pinctrl_state *cam1_rst_h = NULL;
struct pinctrl_state *cam1_rst_l = NULL;
struct pinctrl_state *cam2_pnd_h = NULL;/* main2 cam */
struct pinctrl_state *cam2_pnd_l = NULL;
struct pinctrl_state *cam2_rst_h = NULL;
struct pinctrl_state *cam2_rst_l = NULL;
struct pinctrl_state *cam_ldo_vcama_h = NULL;/* for AVDD */
struct pinctrl_state *cam_ldo_vcama_l = NULL;
struct pinctrl_state *cam_ldo_vcamd_h = NULL;/* for DVDD */
struct pinctrl_state *cam_ldo_vcamd_l = NULL;
struct pinctrl_state *cam_ldo_vcamio_h = NULL;/* for DOVDD */
struct pinctrl_state *cam_ldo_vcamio_l = NULL;
struct pinctrl_state *cam_ldo_vcamaf_h = NULL;/* for AFVDD */
struct pinctrl_state *cam_ldo_vcamaf_l = NULL;
struct pinctrl_state *cam_ldo_sub_vcamd_h = NULL;/* for SUB_DVDD */
struct pinctrl_state *cam_ldo_sub_vcamd_l = NULL;
struct pinctrl_state *cam_ldo_main2_vcamd_h = NULL;/* for MAIN2_DVDD */
struct pinctrl_state *cam_ldo_main2_vcamd_l = NULL;
struct pinctrl_state *cam_mipi_switch_en_h = NULL;/* for mipi switch enable */
struct pinctrl_state *cam_mipi_switch_en_l = NULL;
struct pinctrl_state *cam_mipi_switch_sel_h = NULL;/* for mipi switch select */
struct pinctrl_state *cam_mipi_switch_sel_l = NULL;
int has_mipi_switch = 0;

int mtkcam_gpio_init(struct platform_device *pdev)
{
	int ret = 0;
	has_mipi_switch = 1;

	camctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(camctrl)) {
		dev_err(&pdev->dev, "Cannot find camera pinctrl!");
		ret = PTR_ERR(camctrl);
	}
	/*Cam0 Power/Rst Ping initialization */
	cam0_pnd_h = pinctrl_lookup_state(camctrl, "cam0_pnd1");
	if (IS_ERR(cam0_pnd_h)) {
		ret = PTR_ERR(cam0_pnd_h);
		PK_ERR("%s : pinctrl err, cam0_pnd_h\n", __func__);
	}

	cam0_pnd_l = pinctrl_lookup_state(camctrl, "cam0_pnd0");
	if (IS_ERR(cam0_pnd_l)) {
		ret = PTR_ERR(cam0_pnd_l);
		PK_ERR("%s : pinctrl err, cam0_pnd_l\n", __func__);
	}


	cam0_rst_h = pinctrl_lookup_state(camctrl, "cam0_rst1");
	if (IS_ERR(cam0_rst_h)) {
		ret = PTR_ERR(cam0_rst_h);
		PK_ERR("%s : pinctrl err, cam0_rst_h\n", __func__);
	}

	cam0_rst_l = pinctrl_lookup_state(camctrl, "cam0_rst0");
	if (IS_ERR(cam0_rst_l)) {
		ret = PTR_ERR(cam0_rst_l);
		PK_ERR("%s : pinctrl err, cam0_rst_l\n", __func__);
	}

	/*Cam1 Power/Rst Ping initialization */
	cam1_pnd_h = pinctrl_lookup_state(camctrl, "cam1_pnd1");
	if (IS_ERR(cam1_pnd_h)) {
		ret = PTR_ERR(cam1_pnd_h);
		PK_ERR("%s : pinctrl err, cam1_pnd_h\n", __func__);
	}

	cam1_pnd_l = pinctrl_lookup_state(camctrl, "cam1_pnd0");
	if (IS_ERR(cam1_pnd_l)) {
		ret = PTR_ERR(cam1_pnd_l);
		PK_ERR("%s : pinctrl err, cam1_pnd_l\n", __func__);
	}


	cam1_rst_h = pinctrl_lookup_state(camctrl, "cam1_rst1");
	if (IS_ERR(cam1_rst_h)) {
		ret = PTR_ERR(cam1_rst_h);
		PK_ERR("%s : pinctrl err, cam1_rst_h\n", __func__);
	}


	cam1_rst_l = pinctrl_lookup_state(camctrl, "cam1_rst0");
	if (IS_ERR(cam1_rst_l)) {
		ret = PTR_ERR(cam1_rst_l);
		PK_ERR("%s : pinctrl err, cam1_rst_l\n", __func__);
	}
	/*Cam2 Power/Rst Ping initialization */
	cam2_pnd_h = pinctrl_lookup_state(camctrl, "cam2_pnd1");
	if (IS_ERR(cam2_pnd_h)) {
		ret = PTR_ERR(cam2_pnd_h);
		PK_ERR("%s : pinctrl err, cam2_pnd_h\n", __func__);
	}

	cam2_pnd_l = pinctrl_lookup_state(camctrl, "cam2_pnd0");
	if (IS_ERR(cam2_pnd_l)) {
		ret = PTR_ERR(cam2_pnd_l);
		PK_ERR("%s : pinctrl err, cam2_pnd_l\n", __func__);
	}


	cam2_rst_h = pinctrl_lookup_state(camctrl, "cam2_rst1");
	if (IS_ERR(cam2_rst_h)) {
		ret = PTR_ERR(cam2_rst_h);
		PK_ERR("%s : pinctrl err, cam2_rst_h\n", __func__);
	}


	cam2_rst_l = pinctrl_lookup_state(camctrl, "cam2_rst0");
	if (IS_ERR(cam2_rst_l)) {
		ret = PTR_ERR(cam2_rst_l);
		PK_ERR("%s : pinctrl err, cam2_rst_l\n", __func__);
	}

	/*externel LDO enable */
	/*GPIO 253*/
	cam_ldo_vcama_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcama_1");
	if (IS_ERR(cam_ldo_vcama_h)) {
		ret = PTR_ERR(cam_ldo_vcama_h);
		PK_ERR("%s : pinctrl err, cam_ldo_vcama_h\n", __func__);
	}

	cam_ldo_vcama_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcama_0");
	if (IS_ERR(cam_ldo_vcama_l)) {
		ret = PTR_ERR(cam_ldo_vcama_l);
		PK_ERR("%s : pinctrl err, cam_ldo_vcama_l\n", __func__);
	}

	cam_ldo_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamd_1");
	if (IS_ERR(cam_ldo_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamd_h\n", __func__);
	}

	cam_ldo_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamd_0");
	if (IS_ERR(cam_ldo_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamd_l\n", __func__);
	}

	cam_ldo_vcamio_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamio_1");
	if (IS_ERR(cam_ldo_vcamio_h)) {
		ret = PTR_ERR(cam_ldo_vcamio_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamio_h\n", __func__);
	}

	cam_ldo_vcamio_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamio_0");
	if (IS_ERR(cam_ldo_vcamio_l)) {
		ret = PTR_ERR(cam_ldo_vcamio_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamio_l\n", __func__);
	}

	cam_ldo_vcamaf_h = pinctrl_lookup_state(camctrl, "cam_ldo_vcamaf_1");
	if (IS_ERR(cam_ldo_vcamaf_h)) {
		ret = PTR_ERR(cam_ldo_vcamaf_h);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamaf_h\n", __func__);
	}

	cam_ldo_vcamaf_l = pinctrl_lookup_state(camctrl, "cam_ldo_vcamaf_0");
	if (IS_ERR(cam_ldo_vcamaf_l)) {
		ret = PTR_ERR(cam_ldo_vcamaf_l);
		PK_DBG("%s : pinctrl err, cam_ldo_vcamaf_l\n", __func__);
	}

	cam_ldo_sub_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_sub_vcamd_1");
	if (IS_ERR(cam_ldo_sub_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_sub_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_sub_vcamd_h\n", __func__);
	}

	cam_ldo_sub_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_sub_vcamd_0");
	if (IS_ERR(cam_ldo_sub_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_sub_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_sub_vcamd_l\n", __func__);
	}

	cam_ldo_main2_vcamd_h = pinctrl_lookup_state(camctrl, "cam_ldo_main2_vcamd_1");
	if (IS_ERR(cam_ldo_main2_vcamd_h)) {
		ret = PTR_ERR(cam_ldo_main2_vcamd_h);
		PK_DBG("%s : pinctrl err, cam_ldo_main2_vcamd_h\n", __func__);
	}

	cam_ldo_main2_vcamd_l = pinctrl_lookup_state(camctrl, "cam_ldo_main2_vcamd_0");
	if (IS_ERR(cam_ldo_main2_vcamd_l)) {
		ret = PTR_ERR(cam_ldo_main2_vcamd_l);
		PK_DBG("%s : pinctrl err, cam_ldo_main2_vcamd_l\n", __func__);
	}

	cam_mipi_switch_en_h = pinctrl_lookup_state(camctrl, "cam_mipi_switch_en_1");
	if (IS_ERR(cam_mipi_switch_en_h)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_en_h);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_en_h\n", __func__);
	}

	cam_mipi_switch_en_l = pinctrl_lookup_state(camctrl, "cam_mipi_switch_en_0");
	if (IS_ERR(cam_mipi_switch_en_l)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_en_l);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_en_l\n", __func__);
	}
	cam_mipi_switch_sel_h = pinctrl_lookup_state(camctrl, "cam_mipi_switch_sel_1");
	if (IS_ERR(cam_mipi_switch_sel_h)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_sel_h);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_sel_h\n", __func__);
	}

	cam_mipi_switch_sel_l = pinctrl_lookup_state(camctrl, "cam_mipi_switch_sel_0");
	if (IS_ERR(cam_mipi_switch_sel_l)) {
		has_mipi_switch = 0;
		ret = PTR_ERR(cam_mipi_switch_sel_l);
		PK_DBG("%s : pinctrl err, cam_mipi_switch_sel_l\n", __func__);
	}
	return ret;
}

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val)
{
	int ret = 0;
//	static signed int mAVDD_usercounter = 0;
	static signed int mDVDD_usercounter = 0;

	if (IS_ERR(camctrl)) {
		return -1;
	}

	switch (PwrType) {
	case RST:
		if (PinIdx == 0) {
			if (Val == 0 && !IS_ERR(cam0_rst_l))
				pinctrl_select_state(camctrl, cam0_rst_l);
			else if (Val == 1 && !IS_ERR(cam0_rst_h))
				pinctrl_select_state(camctrl, cam0_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		} else if (PinIdx == 1) {
			if (Val == 0 && !IS_ERR(cam1_rst_l))
				pinctrl_select_state(camctrl, cam1_rst_l);
			else if (Val == 1 && !IS_ERR(cam1_rst_h))
				pinctrl_select_state(camctrl, cam1_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		} else {
			if (Val == 0 && !IS_ERR(cam2_rst_l))
				pinctrl_select_state(camctrl, cam2_rst_l);
			else if (Val == 1 && !IS_ERR(cam2_rst_h))
				pinctrl_select_state(camctrl, cam2_rst_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, RST\n", __func__,PinIdx ,Val);
		}
		break;
	case PDN:
		if (PinIdx == 0) {
			if (Val == 0 && !IS_ERR(cam0_pnd_l))
				pinctrl_select_state(camctrl, cam0_pnd_l);
			else if (Val == 1 && !IS_ERR(cam0_pnd_h))
				pinctrl_select_state(camctrl, cam0_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		} else if (PinIdx == 1) {
			if (Val == 0 && !IS_ERR(cam1_pnd_l))
				pinctrl_select_state(camctrl, cam1_pnd_l);
			else if (Val == 1 && !IS_ERR(cam1_pnd_h))
				pinctrl_select_state(camctrl, cam1_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		} else {
			if (Val == 0 && !IS_ERR(cam2_pnd_l))
				pinctrl_select_state(camctrl, cam2_pnd_l);
			else if (Val == 1 && !IS_ERR(cam2_pnd_h))
				pinctrl_select_state(camctrl, cam2_pnd_h);
			else
				PK_ERR("%s : pinctrl err, PinIdx %d, Val %d, PDN\n", __func__,PinIdx ,Val);
		}
		break;
	case MAIN2_DVDD:
	case SUB_AVDD:
	case MAIN2_AVDD:
		/*SUB_DVDD & SUB_AVDD MAIN2_AVDD use same cotrol GPIO */
		PK_DBG("mDVDD_usercounter(%d)\n",mDVDD_usercounter);
		if (Val == 0 && !IS_ERR(cam_ldo_vcamd_l)){
			mDVDD_usercounter --;
			if(mDVDD_usercounter <= 0)
			{
				if(mDVDD_usercounter < 0)
					PK_ERR("Please check AVDD pin control\n");

				mDVDD_usercounter = 0;
				pinctrl_select_state(camctrl, cam_ldo_vcamd_l);
			}

		}
		else if (Val == 1 && !IS_ERR(cam_ldo_vcamd_h)){
			mDVDD_usercounter ++;
			pinctrl_select_state(camctrl, cam_ldo_vcamd_h);
		}
		break;
	case DVDD:
	case DOVDD:
	case AFVDD:
	default:
		PK_ERR("PwrType(%d) is invalid !!\n", PwrType);
		break;
	};

	PK_DBG("PinIdx(%d) PwrType(%d) val(%d)\n", PinIdx, PwrType, Val);

	return ret;
}

BOOL hwpoweron(PowerInformation *pPowerInformation, int pinSetIdx)
{
	PowerCustInfo *pPowerCust;

	if (pPowerInformation->PowerType == AVDD) {
		if (pinSetIdx == 2) {
			pPowerCust = &PowerCustList[CUST_MAIN2_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(MAIN2_AVDD, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_AVDD, pPowerCust->Voltage)) {
					PK_INFO("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		} else if(pinSetIdx == 1){
			pPowerCust = &PowerCustList[CUST_SUB_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(SUB_AVDD, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_AVDD, pPowerCust->Voltage)) {
					PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		} else{
			pPowerCust = &PowerCustList[CUST_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(pPowerInformation->PowerType, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, AVDD, pPowerCust->Voltage)) {
					PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pPowerInformation->PowerType == DVDD) {
		if (pinSetIdx == 2) {
			pPowerCust = &PowerCustList[CUST_MAIN2_DVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Main2 camera VCAM_D power on");
				/*vcamd: unsupportable voltage range: 1500000-1210000uV*/
				if (pPowerInformation->Voltage == Vol_1200) {
					pPowerInformation->Voltage = Vol_1210;
					//PK_INFO("[CAMERA SENSOR] Main2 camera VCAM_D power 1.2V to 1.21V\n");
				}
				if (TRUE != _hwPowerOn(MAIN2_DVDD, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, pPowerCust->Voltage)) {
					PK_ERR("[CAMERA CUST_MAIN2_DVDD] set gpio failed!!\n");
				}
			}
		} else if (pinSetIdx == 1) {
			pPowerCust = &PowerCustList[CUST_SUB_DVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power on");
				if (pPowerInformation->Voltage == Vol_1200) {
					pPowerInformation->Voltage = Vol_1210;
					PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power 1.2V to 1.21V\n");
				}
				if (TRUE != _hwPowerOn(SUB_DVDD, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, pPowerCust->Voltage)) {
					PK_ERR("[CAMERA CUST_SUB_DVDD] set gpio failed!!\n");
				}
			}
		} else {
			pPowerCust = &PowerCustList[CUST_DVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Main camera VCAM_D power on");
				//if (pPowerInformation->Voltage == Vol_1200) {
				//	pPowerInformation->Voltage = Vol_1220;
					//PK_INFO("[CAMERA SENSOR] Sub camera VCAM_D power 1.2V to 1.21V\n");
				//}
				if (TRUE != _hwPowerOn(DVDD, pPowerInformation->Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, DVDD, pPowerCust->Voltage)) {
					PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pPowerInformation->PowerType == DOVDD) {
		pPowerCust = &PowerCustList[CUST_DOVDD];

		if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pPowerInformation->PowerType, pPowerInformation->Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, DOVDD, pPowerCust->Voltage)) {
				PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");
			}

		}
	} else if (pPowerInformation->PowerType == AFVDD) {
		pPowerCust = &PowerCustList[CUST_AFVDD];

		//PK_INFO("[CAMERA SENSOR] Skip AFVDD setting\n");
		if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pPowerInformation->PowerType, pPowerInformation->Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AFVDD, pPowerCust->Voltage)) {
				PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");
			}
		}
	} else if (pPowerInformation->PowerType == PDN) {
		/* PK_INFO("hwPowerOn: PDN %d\n", pPowerInformation->Voltage); */
		/*mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);*/
		if (pPowerInformation->Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}
	} else if (pPowerInformation->PowerType == RST) {
		/*mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);*/
		if (pPowerInformation->Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}

	} else if (pPowerInformation->PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			/* PK_INFO("Sensor MCLK1 On"); */
			ISP_MCLK1_EN(TRUE);
		} else if (pinSetIdx == 1) {
			/* PK_INFO("Sensor MCLK2 On"); */
			ISP_MCLK2_EN(TRUE);
		} else if (pinSetIdx == 2) {
			/* PK_INFO("Sensor MCLK3 On"); */
			ISP_MCLK3_EN(TRUE);
		} else {
			/* PK_INFO("Sensor MCLK1 On"); */
			ISP_MCLK1_EN(TRUE);
		}
	} else {
	}
	if (pPowerInformation->Delay)
		mdelay(pPowerInformation->Delay);
	return TRUE;
}


BOOL hwpowerdown(PowerInformation *pPowerInformation, int pinSetIdx)
{
	PowerCustInfo *pPowerCust;

	if (pPowerInformation->PowerType == AVDD) {
		if (pinSetIdx == 2) {
			pPowerCust = &PowerCustList[CUST_MAIN2_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(MAIN2_AVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to diable analog power2\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_AVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		} else if(pinSetIdx == 1) {
			pPowerCust = &PowerCustList[CUST_SUB_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(SUB_AVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to diable analog power1\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_AVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		} else{
			pPowerCust = &PowerCustList[CUST_AVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(AVDD)) {
					PK_ERR("[CAMERA SENSOR]Fail to diable analog power0\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, AVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pPowerInformation->PowerType == DVDD) {
		if (pinSetIdx == 2) {
			pPowerCust = &PowerCustList[CUST_MAIN2_DVDD];

			if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(MAIN2_DVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to disable digital power2\n");
					return FALSE;
				} else {
					if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_MAIN2_DVDD] set gpio failed!!\n");
					}
				}
			} else if (pinSetIdx == 1) {
				pPowerCust = &PowerCustList[CUST_SUB_DVDD];

				if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
					if (TRUE != _hwPowerDown(SUB_DVDD)) {
						PK_ERR("[CAMERA SENSOR] Fail to disable digital power1\n");
						return FALSE;
					}
				} else {
					if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_SUB_DVDD] set gpio failed!!\n");
					}
				}
			} else {
				pPowerCust = &PowerCustList[CUST_DVDD];

				if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
					if (TRUE != _hwPowerDown(DVDD)) {
						PK_ERR("[CAMERA SENSOR] Fail to disable digital power0\n");
						return FALSE;
					}
				} else {
					if (mtkcam_gpio_set(pinSetIdx, DVDD, 1 - pPowerCust->Voltage)) {
						PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
					}
				}
			}
		}
	} else if (pPowerInformation->PowerType == DOVDD) {
		pPowerCust = &PowerCustList[CUST_DOVDD];

		if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(pPowerInformation->PowerType)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable io power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, DOVDD, 1 - pPowerCust->Voltage)) {
				PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pPowerInformation->PowerType == AFVDD) {
		pPowerCust = &PowerCustList[CUST_AFVDD];

		if (pPowerCust->Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(pPowerInformation->PowerType)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable af power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AFVDD, 1 - pPowerCust->Voltage)) {
				PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pPowerInformation->PowerType == PDN) {
		/*PK_INFO("hwPowerDown: PDN %d\n", pPowerInformation->Voltage);*/
		if (pPowerInformation->Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}
	} else if (pPowerInformation->PowerType == RST) {
		/*PK_INFO("hwPowerDown: RST %d\n", pPowerInformation->Voltage);*/
		if (pPowerInformation->Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}
	} else if (pPowerInformation->PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			ISP_MCLK1_EN(FALSE);
		} else if (pinSetIdx == 1) {
			ISP_MCLK2_EN(FALSE);
		} else if (pinSetIdx == 2) {
			ISP_MCLK3_EN(FALSE);
		} else {
			ISP_MCLK1_EN(FALSE);
		}
	} else {
	}
	return TRUE;
}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On,
		       char *mode_name)
{
	PowerSequence       *pPowerSequence;
	PowerInformation    *pPowerInformation;
	BOOL               (*pPowerSequenceFunction)(PowerInformation *, int);
	int                  pinSetIdx;

#if defined(CONFIG_IMGSENSOR_MAIN) || defined(CONFIG_IMGSENSOR_SUB) || defined(CONFIG_IMGSENSOR_MAIN2)
#define STRINGIZE(sensorName)       #sensorName
#define SENSOR_NAME(stringizedName) STRINGIZE(stringizedName)
	char *pSensorNameConfig;

#ifdef CONFIG_IMGSENSOR_MAIN
	pSensorNameConfig = SENSOR_NAME(CONFIG_IMGSENSOR_MAIN);
	if(SensorIdx == DUAL_CAMERA_MAIN_SENSOR && strncmp(currSensorName, ++pSensorNameConfig, sizeof(SENSOR_NAME(CONFIG_IMGSENSOR_MAIN)) - 3))
		return 0;
#endif
#ifdef CONFIG_IMGSENSOR_SUB
	pSensorNameConfig = SENSOR_NAME(CONFIG_IMGSENSOR_SUB);
	if(SensorIdx == DUAL_CAMERA_SUB_SENSOR && strncmp(currSensorName, ++pSensorNameConfig, sizeof(SENSOR_NAME(CONFIG_IMGSENSOR_SUB)) - 3))
		return 0;
#endif
#ifdef CONFIG_IMGSENSOR_MAIN2
	pSensorNameConfig = SENSOR_NAME(CONFIG_IMGSENSOR_MAIN2);
	if(SensorIdx == DUAL_CAMERA_MAIN_2_SENSOR && strncmp(currSensorName, ++pSensorNameConfig, sizeof(SENSOR_NAME(CONFIG_IMGSENSOR_MAIN2)) - 3))
		return 0;
#endif
#endif

	PK_INFO("PowerOn:SensorName=%s, sensorIdx:%d, ON/OFF:%d\n", currSensorName, SensorIdx, On);

	pinSetIdx = (SensorIdx == DUAL_CAMERA_MAIN_SENSOR) ? 0 : (SensorIdx == DUAL_CAMERA_SUB_SENSOR) ? 1 : 2;

	if(On) {
	    /* MIPI SWITCH */
		if(has_mipi_switch){
			if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
				pinctrl_select_state(camctrl, cam_mipi_switch_en_l);
				pinctrl_select_state(camctrl, cam_mipi_switch_sel_h);
			} else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
				pinctrl_select_state(camctrl, cam_mipi_switch_en_l);
				pinctrl_select_state(camctrl, cam_mipi_switch_sel_l);
			}
		}

		pPowerSequenceFunction = hwpoweron;
	} else{
		if(has_mipi_switch){
			if ((DUAL_CAMERA_SUB_SENSOR == SensorIdx) || (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx)) {
				pinctrl_select_state(camctrl, cam_mipi_switch_en_h);
			}
		}

		pPowerSequenceFunction = hwpowerdown;
	}

	//pPowerSequenceFunction = (On) ? hwpoweron : hwpowerdown;
	pPowerSequence         = PowerOnList;

	while(pPowerSequence->SensorName) {
		if (!strcmp(pPowerSequence->SensorName, currSensorName))
		{
			pPowerInformation = pPowerSequence->PowerInfo;
			while(pPowerInformation->PowerType != VDD_None &&
				  pPowerSequenceFunction(pPowerInformation, pinSetIdx) &&
				  (pPowerInformation - pPowerSequence->PowerInfo) < CAMERA_HW_POWER_INFO_MAX * sizeof(PowerInformation)) {
				pPowerInformation++;
			}
		}
		pPowerSequence++;
	}

	return 0;//(pPowerInformation->PowerType == VDD_None) ? 0 : -EIO;
}

static int __init kd_camera_hw_init(void)
{
	return 0;
}

#if !defined(CONFIG_MTK_LEGACY)
/*PMIC*/
struct regulator *regVCAMA = NULL;
struct regulator *regVCAMD = NULL;
struct regulator *regVCAMIO = NULL;
struct regulator *regVCAMAF = NULL;
struct regulator *regSubVCAMA = NULL;
struct regulator *regSubVCAMD = NULL;
struct regulator *regSubVCAMIO = NULL;
struct regulator *regMain2VCAMA = NULL;
struct regulator *regMain2VCAMD = NULL;
struct regulator *regMain2VCAMIO = NULL;

extern struct device *sensor_device;

bool Get_Cam_Regulator(void)
{
	struct device_node *node = sensor_device->of_node;

	if ((sensor_device->of_node = of_find_compatible_node(NULL, NULL, "mediatek,camera_hw")) == NULL){
		PK_ERR("regulator get cust camera node failed!\n");
		sensor_device->of_node = node;

		return FALSE;
	}

	/* name = of_get_property(node, "MAIN_CAMERA_POWER_A", NULL); */
	regSubVCAMD = regulator_get(sensor_device, "vcamd_sub"); /*check customer definition*/
	if (regSubVCAMD == NULL) {
	    if (regVCAMA == NULL) {
		    regVCAMA = regulator_get(sensor_device, "vcama");
	    }
	    if (regVCAMD == NULL) {
		    regVCAMD = regulator_get(sensor_device, "vcamd");
	    }
	    if (regVCAMIO == NULL) {
		    regVCAMIO = regulator_get(sensor_device, "vcamio");
		}
	    if (regVCAMAF == NULL) {
		    regVCAMAF = regulator_get(sensor_device, "vcamaf");
	    }
	} else{
		PK_DBG("Camera customer regulator!\n");
	    if (regVCAMA == NULL) {
		    regVCAMA = regulator_get(sensor_device, "vcama1");
	    }
		if (regSubVCAMA == NULL) {
		    regSubVCAMA = regulator_get(sensor_device, "vcama2");
	    }
		if (regMain2VCAMA == NULL) {
		    regMain2VCAMA = regulator_get(sensor_device, "vcama2");
	    }
	    if (regVCAMD == NULL) {
		    regVCAMD = regulator_get(sensor_device, "vcamd1");
	    }
		if (regSubVCAMD == NULL) {
		    regSubVCAMD = regulator_get(sensor_device, "vcamd2");
	    }
		if (regMain2VCAMD == NULL) {
		    regMain2VCAMD = regulator_get(sensor_device, "vcamd2");
	    }
	    if (regVCAMIO == NULL) {
		    regVCAMIO = regulator_get(sensor_device, "vcamio");
	    }
	    if (regSubVCAMIO == NULL) {
		    regSubVCAMIO = regulator_get(sensor_device, "vcamio");
	    }
		if (regMain2VCAMIO == NULL) {
		    regMain2VCAMIO = regulator_get(sensor_device, "vcamio");
	    }
	    if (regVCAMAF == NULL) {
		    regVCAMAF = regulator_get(sensor_device, "vcamaf");
	    }
	}
	sensor_device->of_node = node;

	return TRUE;
}


bool _hwPower(PowerType type, Voltage powerVolt, bool on)
{
    bool ret = FALSE;
	struct regulator *reg = NULL;
	PK_DBG("[_hwPower]powertype:%d\n", type);

	if (type == AVDD) {
		reg = regVCAMA;
	} else if (type == DVDD) {
		reg = regVCAMD;
	} else if (type == DOVDD) {
		reg = regVCAMIO;
	} else if (type == AFVDD) {
		reg = regVCAMAF;
	} else if (type == SUB_AVDD) {
		reg = regSubVCAMA;
	} else if (type == SUB_DVDD) {
		reg = regSubVCAMD;
	} else if (type == SUB_DOVDD) {
		reg = regSubVCAMIO;
	} else if (type == MAIN2_AVDD) {
		reg = regMain2VCAMA;
	} else if (type == MAIN2_DVDD) {
		reg = regMain2VCAMD;
	} else if (type == MAIN2_DOVDD) {
		reg = regMain2VCAMIO;
	} else {
		return ret;
	}

	if (IS_ERR(reg)) {
		PK_DBG("[_hwPower]%d fail to power ON/OFF due to regVCAM == NULL\n", type);
		return ret;
    }

	if(on) {
		if (regulator_set_voltage(reg , powerVolt, powerVolt)) {
			PK_ERR("[_hwPower]fail to regulator_set_voltage, powertype:%d powerId:%d\n", type, powerVolt);
			//return ret;
		}
		if (regulator_enable(reg)) {
			PK_DBG("[_hwPower]fail to regulator_enable, powertype:%d powerId:%d\n", type, powerVolt);
			return ret;
		}
 	} else {
		if (regulator_is_enabled(reg)) {
			PK_DBG("[_hwPower]%d is enabled\n", type);
		}
		if (regulator_disable(reg)) {
			PK_DBG("[_hwPower]fail to regulator_disable, powertype: %d\n\n", type);
			return ret;
		}
	}
	ret = true;

    return ret;
}
#endif


arch_initcall(kd_camera_hw_init);
EXPORT_SYMBOL(kdCISModulePowerOn);

/* !-- */
/*  */

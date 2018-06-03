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

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include "kd_camera_hw.h"
/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"

//#define DEBUG_CAMERA_HW_K
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


extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);

u32 pinSetIdx = 0;		/* default main sensor */
u32 pinSet[3][8] = {
	/* for main sensor */
	{CAMERA_CMRST_PIN,
	 CAMERA_CMRST_PIN_M_GPIO,	/* mode */
	 GPIO_OUT_ONE,		/* ON state */
	 GPIO_OUT_ZERO,		/* OFF state */
	 CAMERA_CMPDN_PIN,
	 CAMERA_CMPDN_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 },
	/* for sub sensor */
	{CAMERA_CMRST1_PIN,
	 CAMERA_CMRST1_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 CAMERA_CMPDN1_PIN,
	 CAMERA_CMPDN1_PIN_M_GPIO,
	 GPIO_OUT_ONE,
	 GPIO_OUT_ZERO,
	 },
	/* for Main2 sensor */
	{CAMERA_CMRST2_PIN,
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


PowerCust PowerCustList = {
	{
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for AVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for DVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for DOVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for AFVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for SUB_AVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_Low},	/* for SUB_DVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for SUB_DOVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_AVDD; */
	 {GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_DVDD; */
	 {GPIO_UNSUPPORTED, GPIO_MODE_GPIO, Vol_High},	/* for MAIN2_DOVDD; */
/* {GPIO_SUPPORTED, GPIO_MODE_GPIO, Vol_Low}, */
	 }
};



PowerUp PowerOnList = {
	{
	  {SENSOR_DRVNAME_IMX338_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 0},
	   {DOVDD, Vol_1800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 0},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 1}
	   },
	  },
	  {SENSOR_DRVNAME_S5K3M2_MIPI_RAW,
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
	   },
	  },
	  {SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
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
	   },
	  },
	 {SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
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
	   },
	  },
	 {SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
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
	   },
	  },
	  {SENSOR_DRVNAME_IMX258_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
	   {SENSOR_DRVNAME_OV8858_MIPI_RAW,
		{
		 {SensorMCLK, Vol_High, 0},
		 {PDN, Vol_Low, 0},
		 {RST, Vol_Low, 0},
		 {DOVDD, Vol_1800, 1},
		 {AVDD, Vol_2800, 1},
		 {DVDD, Vol_1200, 5},
		 {AFVDD, Vol_2800, 1},
		 {PDN, Vol_High, 1},
		 {RST, Vol_High, 2}
		 },
		},
	  {SENSOR_DRVNAME_S5K2X8_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
	  {SENSOR_DRVNAME_IMX214_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1000, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
	  {SENSOR_DRVNAME_IMX214_MIPI_MONO,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1000, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
	  {SENSOR_DRVNAME_IMX230_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1200, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
	  {SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#if defined(IMX362_MIPI_RAW)
	  {SENSOR_DRVNAME_IMX362_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 1},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
	  {SENSOR_DRVNAME_S5K2L7_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 0},
	   {RST, Vol_Low, 0},
	   {DOVDD, Vol_1800, 0},
	   {AVDD, Vol_2800, 0},
	   {DVDD, Vol_1200, 0},
	   {AFVDD, Vol_2800, 3},
	   {PDN, Vol_High, 0},
	   {RST, Vol_High, 5}
	   },
	  },
	  {SENSOR_DRVNAME_IMX318_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1200, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#if defined(OV8865_MIPI_RAW)
	  {SENSOR_DRVNAME_OV8865_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {PDN, Vol_Low, 5},
	   {RST, Vol_Low, 5},
	   {DOVDD, Vol_1800, 5},
	   {AVDD, Vol_2800, 5},
	   {DVDD, Vol_1200, 5},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_High, 5},
	   {RST, Vol_High, 5}
	   },
	  },
#endif
#if defined(IMX219_MIPI_RAW)
	  {SENSOR_DRVNAME_IMX219_MIPI_RAW,
	  {
	   {SensorMCLK, Vol_High, 0},
	   {AVDD, Vol_2800, 10},
	   {DOVDD, Vol_1800, 10},
	   {DVDD, Vol_1000, 10},
	   {AFVDD, Vol_2800, 5},
	   {PDN, Vol_Low, 0},
	   {PDN, Vol_High, 0},
	   {RST, Vol_Low, 0},
	   {RST, Vol_High, 0}
	   },
	  },
#endif
#if defined(OV5670_MIPI_RAW)
	   {SENSOR_DRVNAME_OV5670_MIPI_RAW,
	   {
		{SensorMCLK, Vol_High, 0},
		{PDN, Vol_Low, 5},
		{RST, Vol_Low, 5},
		{DOVDD, Vol_1800, 5},
		{AVDD, Vol_2800, 5},
		{DVDD, Vol_1200, 5},
		{AFVDD, Vol_2800, 5},
		{PDN, Vol_High, 5},
		{RST, Vol_High, 5}
		},
	   },
#endif
#if defined(OV5670_MIPI_RAW_2)
			{SENSOR_DRVNAME_OV5670_MIPI_RAW_2,
			{
			 {SensorMCLK, Vol_High, 0},
			 {PDN, Vol_Low, 5},
			 {RST, Vol_Low, 5},
			 {DOVDD, Vol_1800, 5},
			 {AVDD, Vol_2800, 5},
			 {DVDD, Vol_1200, 5},
			 {AFVDD, Vol_2800, 5},
			 {PDN, Vol_High, 5},
			 {RST, Vol_High, 5}
			 },
			},
#endif
	 /* add new sensor before this line */
	 {NULL,},
	 }
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

BOOL hwpoweron(PowerInformation pwInfo, char *mode_name)
{
	if (pwInfo.PowerType == AVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(MAIN2_AVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_AVDD, PowerCustList.PowerCustInfo[CUST_MAIN2_AVDD].Voltage)) {
						PK_INFO("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
		else if(pinSetIdx == 1){
			if (PowerCustList.PowerCustInfo[CUST_SUB_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(SUB_AVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_AVDD, PowerCustList.PowerCustInfo[CUST_SUB_AVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
		else{
			if (PowerCustList.PowerCustInfo[CUST_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, AVDD, PowerCustList.PowerCustInfo[CUST_AVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pwInfo.PowerType == DVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power on");
				/*vcamd: unsupportable voltage range: 1500000-1210000uV*/
				if (pwInfo.Voltage == Vol_1200) {
					pwInfo.Voltage = Vol_1220;
					//PK_INFO("[CAMERA SENSOR] Main2 camera VCAM_D power 1.2V to 1.21V\n");
				}
				if (TRUE != _hwPowerOn(MAIN2_DVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_MAIN2_DVDD] set gpio failed!!\n");
				}
			}
		} else if (pinSetIdx == 1) {
			if (PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power on");
				if (pwInfo.Voltage == Vol_1200) {
					pwInfo.Voltage = Vol_1220;
					PK_DBG("[CAMERA SENSOR] Sub camera VCAM_D power 1.2V to 1.21V\n");
				}
				if (TRUE != _hwPowerOn(SUB_DVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_SUB_DVDD] set gpio failed!!\n");
				}
			}
		} else {
			if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				PK_DBG("[CAMERA SENSOR] Main camera VCAM_D power on");
				//if (pwInfo.Voltage == Vol_1200) {
				//	pwInfo.Voltage = Vol_1220;
					//PK_INFO("[CAMERA SENSOR] Sub camera VCAM_D power 1.2V to 1.21V\n");
				//}
				if (TRUE != _hwPowerOn(DVDD, pwInfo.Voltage)) {
					PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, DVDD, PowerCustList.PowerCustInfo[CUST_DVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
				}
			}
		}
	} else if (pwInfo.PowerType == DOVDD) {
		if (PowerCustList.PowerCustInfo[CUST_DOVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, DOVDD, PowerCustList.PowerCustInfo[CUST_DOVDD].Voltage)) {
					PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");
			}

		}
	} else if (pwInfo.PowerType == AFVDD) {
		//PK_INFO("[CAMERA SENSOR] Skip AFVDD setting\n");
		if (PowerCustList.PowerCustInfo[CUST_AFVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerOn(pwInfo.PowerType, pwInfo.Voltage)) {
				PK_ERR("[CAMERA SENSOR] Fail to enable digital power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AFVDD, PowerCustList.PowerCustInfo[CUST_AFVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");
			}
		}
	} else if (pwInfo.PowerType == PDN) {
		/* PK_INFO("hwPowerOn: PDN %d\n", pwInfo.Voltage); */
		/*mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);*/
		if (pwInfo.Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}
	} else if (pwInfo.PowerType == RST) {
		/*mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);*/
		if (pwInfo.Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}

	} else if (pwInfo.PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			/* PK_INFO("Sensor MCLK1 On"); */
			ISP_MCLK1_EN(TRUE);
		} else if (pinSetIdx == 1) {
			/* PK_INFO("Sensor MCLK2 On"); */
			ISP_MCLK2_EN(TRUE);
		} else if (pinSetIdx == 2) {
			/* PK_INFO("Sensor MCLK2 On"); */
			ISP_MCLK2_EN(TRUE);
		} else {
			/* PK_INFO("Sensor MCLK3 On"); */
			ISP_MCLK1_EN(TRUE);
		}
	} else {
	}
	if (pwInfo.Delay > 0)
		mdelay(pwInfo.Delay);
	return TRUE;
}



BOOL hwpowerdown(PowerInformation pwInfo, char *mode_name)
{
	if (pwInfo.PowerType == AVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(MAIN2_AVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to diable analog power2\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, MAIN2_AVDD, 1-PowerCustList.PowerCustInfo[CUST_MAIN2_AVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
		else if(pinSetIdx == 1){
			if (PowerCustList.PowerCustInfo[CUST_SUB_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(SUB_AVDD)) {
					PK_ERR("[CAMERA SENSOR] Fail to diable analog power1\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, SUB_AVDD, 1-PowerCustList.PowerCustInfo[CUST_SUB_AVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}
		else{
			if (PowerCustList.PowerCustInfo[CUST_AVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(AVDD)) {
					PK_ERR("[CAMERA SENSOR]Fail to diable analog power0\n");
					return FALSE;
				}
			} else {
				if (mtkcam_gpio_set(pinSetIdx, AVDD, 1-PowerCustList.PowerCustInfo[CUST_AVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");
				}
			}
		}


	} else if (pwInfo.PowerType == DVDD) {
		if (pinSetIdx == 2) {
			if (PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
				if (TRUE != _hwPowerDown(MAIN2_DVDD)) {
						PK_ERR("[CAMERA SENSOR] Fail to disable digital power2\n");
						return FALSE;
					}
				} else {
					if (mtkcam_gpio_set(pinSetIdx, MAIN2_DVDD, 1-PowerCustList.PowerCustInfo[CUST_MAIN2_DVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_MAIN2_DVDD] set gpio failed!!\n");
					}
				}
			} else if (pinSetIdx == 1) {
				if (PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
					if (TRUE != _hwPowerDown(SUB_DVDD)) {
						PK_ERR("[CAMERA SENSOR] Fail to disable digital power1\n");
						return FALSE;
					}
				} else {
					if (mtkcam_gpio_set(pinSetIdx, SUB_DVDD, 1-PowerCustList.PowerCustInfo[CUST_SUB_DVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_SUB_DVDD] set gpio failed!!\n");
					}
				}
			} else {
				if (PowerCustList.PowerCustInfo[CUST_DVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
					if (TRUE != _hwPowerDown(DVDD)) {
						PK_ERR("[CAMERA SENSOR] Fail to disable digital power0\n");
						return FALSE;
					}
				} else {
					if (mtkcam_gpio_set(pinSetIdx, DVDD, 1-PowerCustList.PowerCustInfo[CUST_DVDD].Voltage)) {
						PK_ERR("[CAMERA CUST_DVDD] set gpio failed!!\n");
					}
				}
			}
	} else if (pwInfo.PowerType == DOVDD) {
		if (PowerCustList.PowerCustInfo[CUST_DOVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(pwInfo.PowerType)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable io power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, DOVDD, 1-PowerCustList.PowerCustInfo[CUST_DOVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_AVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pwInfo.PowerType == AFVDD) {
		if (PowerCustList.PowerCustInfo[CUST_AFVDD].Gpio_Pin == GPIO_UNSUPPORTED) {
			if (TRUE != _hwPowerDown(pwInfo.PowerType)) {
				PK_ERR("[CAMERA SENSOR] Fail to disable af power\n");
				return FALSE;
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, AFVDD, 1-PowerCustList.PowerCustInfo[CUST_AFVDD].Voltage)) {
				PK_ERR("[CAMERA CUST_DOVDD] set gpio failed!!\n");/* 1-voltage for reverse*/
			}
		}
	} else if (pwInfo.PowerType == PDN) {
		/*PK_INFO("hwPowerDown: PDN %d\n", pwInfo.Voltage);*/
		if (pwInfo.Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, PDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}
	} else if (pwInfo.PowerType == RST) {
		/*PK_INFO("hwPowerDown: RST %d\n", pwInfo.Voltage);*/
		if (pwInfo.Voltage == Vol_High) {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		} else {
			if (mtkcam_gpio_set(pinSetIdx, RST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {

				PK_ERR("[CAMERA SENSOR] set gpio failed!!\n");
			}
		}

	} else if (pwInfo.PowerType == SensorMCLK) {
		if (pinSetIdx == 0) {
			ISP_MCLK1_EN(FALSE);
		} else if (pinSetIdx == 1) {
			ISP_MCLK2_EN(FALSE);
		} else if (pinSetIdx == 2) {
			ISP_MCLK2_EN(FALSE);
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

	int pwListIdx, pwIdx;
	BOOL sensorInPowerList = KAL_FALSE;

	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) {
		pinSetIdx = 0;
	} else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
		pinSetIdx = 1;
	} else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
		pinSetIdx = 2;
	}

	if (currSensorName && (0 == strcmp(currSensorName, "ov5670mipiraw")))
	{
		if(pinSetIdx == 1)
		{
			goto _kdCISModulePowerOn_exit_;
		}
	}

	/* power ON */
	if (On) {
		PK_INFO("PowerOn:SensorName=%s, pinSetIdx=%d, sensorIdx:%d\n", currSensorName, pinSetIdx, SensorIdx);
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

		for (pwListIdx = 0; pwListIdx < 16; pwListIdx++) {
			if (currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName != NULL)
			    && (0 == strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,currSensorName))) {
				//PK_INFO("sensorIdx:%d\n", SensorIdx);

				sensorInPowerList = KAL_TRUE;

				for (pwIdx = 0; pwIdx < 10; pwIdx++) {
					PK_DBG("PowerType:%d\n", PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType);
					if (PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].
					    PowerType != VDD_None) {
						if (hwpoweron(PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx], mode_name) == FALSE)
							goto _kdCISModulePowerOn_exit_;
					} else {
						PK_DBG("pwIdx=%d\n", pwIdx);
						break;
					}
				}
				break;
			} else if (PowerOnList.PowerSeq[pwListIdx].SensorName == NULL) {
				break;
			} else {
			}
		}
#if 0
		/* Temp solution: default power on/off sequence */
		if (KAL_FALSE == sensorInPowerList) {
			PK_INFO("Default power on sequence");

			if (pinSetIdx == 0) {
				ISP_MCLK1_EN(1);
			} else if (pinSetIdx == 1) {
				ISP_MCLK2_EN(1);
			}
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_INFO("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_INFO("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_INFO("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif
				} else {
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}
			/* VCAM_IO */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800*1000, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %d\n",
				     CAMERA_POWER_VCAM_D2);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800*1000, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800*1000, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d\n",
				     CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800*1000, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %d\n",
				     CAMERA_POWER_VCAM_A2);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);

			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_INFO("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_INFO("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_INFO("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			mdelay(1);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif

				} else {
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}




		}
		/*
		   if(pinSetIdx==0)
		   for(;;)
		   {}
		 */
		/*
		   if(pinSetIdx==1)
		   for(;;)
		   {}
		 */
 #endif
	} else {
	    /* power OFF */
		PK_INFO("PowerOFF:pinSetIdx=%d, sensorIdx:%d\n", pinSetIdx, SensorIdx);
		if(has_mipi_switch){
			if ((DUAL_CAMERA_SUB_SENSOR == SensorIdx) || (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx)) {
				pinctrl_select_state(camctrl, cam_mipi_switch_en_h);
			}
		}

		for (pwListIdx = 0; pwListIdx < 16; pwListIdx++) {
			if (currSensorName && (PowerOnList.PowerSeq[pwListIdx].SensorName != NULL)
			    && (0 ==
				strcmp(PowerOnList.PowerSeq[pwListIdx].SensorName,
				       currSensorName))) {
				/*PK_INFO("kdCISModulePowerOn get in---\n");*/
				//PK_INFO("sensorIdx:%d\n", SensorIdx);

				sensorInPowerList = KAL_TRUE;

				for (pwIdx = 9; pwIdx >= 0; pwIdx--) {
					PK_DBG("PowerType:%d\n", PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].PowerType);
					if (PowerOnList.PowerSeq[pwListIdx].PowerInfo[pwIdx].
					    PowerType != VDD_None) {
						if (hwpowerdown
						    (PowerOnList.PowerSeq[pwListIdx].
						     PowerInfo[pwIdx], mode_name) == FALSE)
							goto _kdCISModulePowerOn_exit_;
						if (pwIdx > 0) {
							if (PowerOnList.PowerSeq[pwListIdx].
							    PowerInfo[pwIdx - 1].Delay > 0)
								mdelay(PowerOnList.
								       PowerSeq[pwListIdx].
								       PowerInfo[pwIdx - 1].Delay);
						}
					} else {
						PK_DBG("pwIdx=%d\n", pwIdx);
					}
				}
			} else if (PowerOnList.PowerSeq[pwListIdx].SensorName == NULL) {
				break;
			} else {
			}
		}
#if 0
		/* Temp solution: default power on/off sequence */
		if (KAL_FALSE == sensorInPowerList) {
			PK_INFO("Default power off sequence");

			if (pinSetIdx == 0) {
				ISP_MCLK1_EN(0);
			} else if (pinSetIdx == 1) {
				ISP_MCLK2_EN(0);
			}
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_INFO("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_INFO("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_INFO("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}


			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (0 == pinSetIdx) {
#ifndef CONFIG_MTK_MT6306_SUPPORT
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#else
					if (mt6306_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt6306_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
#endif
				} else {
					if (mt_set_gpio_mode
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
					}
					if (mt_set_gpio_dir
					    (pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
					}
					if (mt_set_gpio_out
					    (pinSet[pinSetIdx][IDX_PS_CMRST],
					     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
						PK_INFO
						    ("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
					}
				}
			}


			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %d\n",
				     CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= (%d)\n",
				     CAMERA_POWER_VCAM_A);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %d\n",
				     CAMERA_POWER_VCAM_D2);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2, mode_name)) {
				PK_INFO
				    ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %d\n",
				     CAMERA_POWER_VCAM_A2);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}







		}
#endif
	}			/*  */

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;
}
static int __init kd_camera_hw_init(void)
{
        return 0;
}

arch_initcall(kd_camera_hw_init);
EXPORT_SYMBOL(kdCISModulePowerOn);

/* !-- */
/*  */

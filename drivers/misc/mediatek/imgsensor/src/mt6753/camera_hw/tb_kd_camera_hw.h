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
#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_

#include <linux/types.h>

// #warning "Compiled tb6735p1_64 kd_camera_hw.h file..."

#if defined CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>

#if 0//def MTK_MT6306_SUPPORT
#include <mach/dcl_sim_gpio.h>
#endif

#include <mach/mt_pm_ldo.h>
#include "pmic_drv.h"

//
//Analog 
#define CAMERA_POWER_VCAM_A         PMIC_APP_MAIN_CAMERA_POWER_A
//Digital 
#define CAMERA_POWER_VCAM_D         PMIC_APP_MAIN_CAMERA_POWER_D
//AF 
#define CAMERA_POWER_VCAM_AF        PMIC_APP_MAIN_CAMERA_POWER_AF
//digital io
#define CAMERA_POWER_VCAM_IO        PMIC_APP_MAIN_CAMERA_POWER_IO
//Digital for Sub
#define SUB_CAMERA_POWER_VCAM_D     PMIC_APP_SUB_CAMERA_POWER_D


//FIXME, should defined in DCT tool 

//Main sensor
#if 0//def MTK_MT6306_SUPPORT
#warning "MTK_MT6306_SUPPORT is defined...notice kd_camera_hw.h"
#endif

#define CAMERA_CMRST_PIN            GPIO_CAMERA_CMRST_PIN 
//#define CAMERA_CMRST_PIN            GPIO44
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_CMRST_PIN_M_GPIO


#define CAMERA_CMPDN_PIN            GPIO_CAMERA_CMPDN_PIN    
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_CMPDN_PIN_M_GPIO 
 
//FRONT sensor
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_CMRST1_PIN 
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_CMRST1_PIN_M_GPIO 

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_CMPDN1_PIN 
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_CMPDN1_PIN_M_GPIO 

// Define I2C Bus Num
#define SUPPORT_I2C_BUS_NUM1        0
#define SUPPORT_I2C_BUS_NUM2        0

#else
	
#define CAMERA_CMRST_PIN            0
#define CAMERA_CMRST_PIN_M_GPIO     0

#define CAMERA_CMPDN_PIN            0
#define CAMERA_CMPDN_PIN_M_GPIO     0

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           0
#define CAMERA_CMRST1_PIN_M_GPIO    0

#define CAMERA_CMPDN1_PIN           0
#define CAMERA_CMPDN1_PIN_M_GPIO    0

#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0

#define SUPPORT_I2C_BUS_NUM1        0
#define SUPPORT_I2C_BUS_NUM2        0


#endif /* End of #if defined CONFIG_MTK_LEGACY */


typedef enum KD_REGULATOR_TYPE_TAG {
	VCAMA,
	VCAMD,
	VCAMIO,
	VCAMAF,
} KD_REGULATOR_TYPE_T;

typedef enum {
	CAMPDN,
	CAMRST,
	CAM1PDN,
	CAM1RST,
	CAMLDO
} CAMPowerType;

extern void ISP_MCLK1_EN(bool En);
extern void ISP_MCLK2_EN(bool En);
extern void ISP_MCLK3_EN(bool En);

extern bool _hwPowerDown(KD_REGULATOR_TYPE_T type);
extern bool _hwPowerOn(KD_REGULATOR_TYPE_T type, int powerVolt);

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val);
int mtkcam_gpio_init(struct platform_device *pdev);

#endif

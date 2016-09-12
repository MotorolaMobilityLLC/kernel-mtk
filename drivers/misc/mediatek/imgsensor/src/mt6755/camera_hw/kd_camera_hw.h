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

#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_

#include <linux/types.h>
#include "kd_camera_typedef.h"

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>

#ifdef CONFIG_MTK_MT6306_SUPPORT
#include <mach/dcl_sim_gpio.h>
#endif


#include <mach/mt_pm_ldo.h>
#include <cust_i2c.h>
#include "pmic_drv.h"



/*  */
/* Power */
#define CAMERA_POWER_VCAM_A PMIC_APP_MAIN_CAMERA_POWER_A
#define CAMERA_POWER_VCAM_D PMIC_APP_MAIN_CAMERA_POWER_D
#define CAMERA_POWER_VCAM_A2 PMIC_APP_MAIN_CAMERA_POWER_AF
#define CAMERA_POWER_VCAM_D2 PMIC_APP_MAIN_CAMERA_POWER_IO

#define SUB_CAMERA_POWER_VCAM_D     PMIC_APP_SUB_CAMERA_POWER_D


/* FIXME, should defined in DCT tool */

/* Main sensor */
#ifdef CONFIG_MTK_MT6306_SUPPORT
    /* Common phone's reset pin uses extension GPIO10 of mt6306 */
#define CAMERA_CMRST_PIN            GPIO10
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_INVALID
#else
#ifdef GPIO_CAMERA_CMRST_PIN
#define CAMERA_CMRST_PIN            GPIO_CAMERA_CMRST_PIN
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_CMRST_PIN_M_GPIO
#else				/* For the case: both GPIO_CAMERA_CMRST_PIN & CONFIG_MTK_MT6306_SUPPORT is undefined */
#define CAMERA_CMRST_PIN            GPIO_CAMERA_INVALID
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_INVALID
#endif
#endif


#define CAMERA_CMPDN_PIN            GPIO_CAMERA_CMPDN_PIN
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_CMPDN_PIN_M_GPIO

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_CMRST1_PIN
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_CMRST1_PIN_M_GPIO

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_CMPDN1_PIN
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_CMPDN1_PIN_M_GPIO



/* Define I2C Bus Num */
#define SUPPORT_I2C_BUS_NUM1        I2C_CAMERA_MAIN_CHANNEL
#define SUPPORT_I2C_BUS_NUM2        I2C_CAMERA_SUB_CHANNEL


typedef enum {
	VDD_None,
	PDN,
	RST,
	SensorMCLK,
	AVDD = CAMERA_POWER_VCAM_A,
	DVDD = CAMERA_POWER_VCAM_D,
	DOVDD = CAMERA_POWER_VCAM_D2,
	AFVDD = CAMERA_POWER_VCAM_A2
} PowerType;

typedef enum {
	Vol_Low = 0,
	Vol_High = 1,
	Vol_900 = VOL_0900,
	Vol_1000 = VOL_1000,
	Vol_1100 = VOL_1100,
	Vol_1200 = VOL_1200,
	Vol_1220 = VOL_1220,
	Vol_1300 = VOL_1300,
	Vol_1350 = VOL_1350,
	Vol_1500 = VOL_1500,
	Vol_1800 = VOL_1800,
	Vol_2000 = VOL_2000,
	Vol_2100 = VOL_2100,
	Vol_2500 = VOL_2500,
	Vol_2800 = VOL_2800,
	Vol_3000 = VOL_3000,
	Vol_3300 = VOL_3300,
	Vol_3400 = VOL_3400,
	Vol_3500 = VOL_3500,
	Vol_3600 = VOL_3600
} Voltage;
#else

#define VOL2800 2800000
#define VOL1800 1800000
#define VOL1500 1500000
#define VOL1200 1200000
#define VOL1220 1220000
#define VOL1000 1000000
#define VOL1100 1100000

typedef enum {
	VDD_None,
	PDN,
	RST,
	SensorMCLK,
	AVDD,
	DVDD,
	DOVDD,
	AFVDD,
	SUB_DVDD,
	MAIN2_DVDD,
} PowerType;

typedef enum {
	Vol_Low = 0,
	Vol_High = 1,
	Vol_1000 = VOL1000,
	Vol_1100 = VOL1100,
	Vol_1200 = VOL1200,
	Vol_1220 = VOL1220,
	Vol_1500 = VOL1500,
	Vol_1800 = VOL1800,
	Vol_2800 = VOL2800,
} Voltage;
#define CAMERA_CMRST_PIN            0
#define CAMERA_CMRST_PIN_M_GPIO     0

#define CAMERA_CMPDN_PIN            0
#define CAMERA_CMPDN_PIN_M_GPIO     0

/* FRONT sensor */
#define CAMERA_CMRST1_PIN           0
#define CAMERA_CMRST1_PIN_M_GPIO    0

#define CAMERA_CMPDN1_PIN           0
#define CAMERA_CMPDN1_PIN_M_GPIO    0

#define CAMERA_CMRST2_PIN           0
#define CAMERA_CMRST2_PIN_M_GPIO    0
#define CAMERA_CMPDN2_PIN           0
#define CAMERA_CMPDN2_PIN_M_GPIO    0

#define GPIO_OUT_ONE 1
#define GPIO_OUT_ZERO 0
#define GPIO_UNSUPPORTED 0xff
#define GPIO_SUPPORTED 0
#define GPIO_MODE_GPIO 0

#endif


typedef struct {
	PowerType PowerType;
	Voltage Voltage;
	u32 Delay;
} PowerInformation;


typedef struct {
	char *SensorName;
	PowerInformation PowerInfo[12];
} PowerSequence;

typedef struct {
	PowerSequence PowerSeq[16];
} PowerUp;

typedef struct {
	u32 Gpio_Pin;
	u32 Gpio_Mode;
	Voltage Voltage;
} PowerCustInfo;

typedef struct {
	PowerCustInfo PowerCustInfo[6];
} PowerCust;

extern bool _hwPowerDown(PowerType type);
extern bool _hwPowerOn(PowerType type, int powerVolt);

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val);
int mtkcam_gpio_init(struct platform_device *pdev);


#endif

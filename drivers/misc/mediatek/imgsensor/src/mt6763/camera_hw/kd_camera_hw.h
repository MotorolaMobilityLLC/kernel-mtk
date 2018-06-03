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
#include "kd_camera_typedef.h"

#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_

#ifndef SUPPORT_I2C_BUS_NUM1
    #define SUPPORT_I2C_BUS_NUM1        2
#endif
#ifndef SUPPORT_I2C_BUS_NUM2
    #define SUPPORT_I2C_BUS_NUM2        3
#endif

#ifndef SUPPORT_I2C_BUS_NUM3
    #define SUPPORT_I2C_BUS_NUM3        SUPPORT_I2C_BUS_NUM2
#endif


#define VOL2800 2800000
#define VOL2500 2500000
#define VOL1800 1800000
#define VOL1500 1500000
#define VOL1200 1200000
#define VOL1210 1210000
#define VOL1220 1220000
#define VOL1000 1000000
#define VOL1100 1100000
#define VOL1050 1050000



typedef enum {
	VDD_None,
	PDN,
	RST,
	SensorMCLK,
	AVDD,
	DVDD,
	DOVDD,
	AFVDD,
	SUB_AVDD,
	SUB_DVDD,
	SUB_DOVDD,
	MAIN2_AVDD,
	MAIN2_DVDD,
	MAIN2_DOVDD,
} PowerType;

typedef enum {
	Vol_Low = 0,
	Vol_High = 1,
	Vol_1000 = VOL1000,
	Vol_1050 = VOL1050,
	Vol_1100 = VOL1100,
	Vol_1200 = VOL1200,
	Vol_1210 = VOL1210,
	Vol_1220 = VOL1220,
	Vol_1500 = VOL1500,
	Vol_1800 = VOL1800,
	Vol_2500 = VOL2500,
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

/* Main2 sensor */
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
	PowerCustInfo PowerCustInfo[10];
} PowerCust;

extern bool _hwPowerDown(PowerType type);
extern bool _hwPowerOn(PowerType type, int powerVolt);
extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);
extern void ISP_MCLK4_EN(BOOL En);
extern unsigned int mt_get_ckgen_freq(int ID);
extern void mipic_26m_en(int en);
extern unsigned int mt_get_abist_freq(unsigned int ID);

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val);
int mtkcam_gpio_init(struct platform_device *pdev);



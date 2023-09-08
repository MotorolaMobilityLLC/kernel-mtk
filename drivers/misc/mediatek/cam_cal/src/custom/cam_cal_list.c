// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include "cam_cal_list.h"
#include "eeprom_i2c_common_driver.h"
#include "eeprom_i2c_custom_driver.h"
#include "kd_imgsensor.h"

#define MAX_EEPROM_SIZE_32K 0x8000
#define MAX_EEPROM_SIZE_16K 0x4000
#if defined(CONFIG_MTK_VICKY_EEPROM_PROJECT)
extern unsigned int mot_s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
extern unsigned int mot_ov02b10_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
#endif
#if defined(CONFIG_MOT_DEVONF_CAMERA_PROJECT)
extern unsigned int mot_devonf_s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
#endif
#ifdef CONFIG_MOT_DEVONN_CAMERA_PROJECT
extern unsigned int mot_ov02b10_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
extern unsigned int mot_gc02m1_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
#endif
#if defined(CONFIG_MOT_CANCUNF_CAMERA_PROJECT)
extern unsigned int mot_cancunf_s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
extern unsigned int mot_cancunf_sc202_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
#endif
#if defined(CONFIG_MOT_CANCUNN_CAMERA_PROJECT)
extern unsigned int mot_cancunn_s5k4h7_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size);
#endif
struct stCAM_CAL_LIST_STRUCT g_camCalList[] = {
#if defined(CONFIG_MOT_DEVONN_CAMERA_PROJECT)
	{MOT_DEVONN_S5KJN1_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_DEVONN_HI1634Q_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_DEVONN_GC02M1_SENSOR_ID, 0x6E, mot_gc02m1_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_DEVONN_OV02B10_SENSOR_ID, 0x78, mot_ov02b10_read_region, MAX_EEPROM_SIZE_16K},  // otp
#elif defined(CONFIG_MOT_DEVONF_CAMERA_PROJECT)
	{MOT_DEVONF_OV50A_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_DEVONF_HI1634Q_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_DEVONF_S5K4H7_SENSOR_ID, 0x5A, mot_devonf_s5k4h7_read_region},  // otp
#elif defined(CONFIG_MOT_CANCUNF_CAMERA_PROJECT)
	{MOT_CANCUNF_OV50D_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNF_S5KJNS_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNF_S5KJNS_VCM_YOVA_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNF_HI1634Q_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNF_S5K3P9SP04_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNF_S5K4H7_SENSOR_ID, 0x5A, mot_cancunf_s5k4h7_read_region},
	{MOT_CANCUNF_SC202_SENSOR_ID, 0x6C, mot_cancunf_sc202_read_region},  // otp
#elif defined(CONFIG_MOT_CANCUNN_CAMERA_PROJECT)
	{MOT_CANCUNN_OV50D_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNN_HI1634Q_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_CANCUNN_S5K4H7_SENSOR_ID, 0x5A, mot_cancunn_s5k4h7_read_region},  //otp
#else
	/*Below is commom sensor */
	{HI1339_SENSOR_ID, 0xB0, Common_read_region},
	{OV13B10LZ_SENSOR_ID, 0xB0, Common_read_region},
	{GC5035_SENSOR_ID,  0x7E, Common_read_region},
	{HI1339SUBOFILM_SENSOR_ID, 0xA2, Common_read_region},
	{HI1339SUBTXD_SENSOR_ID, 0xA2, Common_read_region},
	//Begin: Add EEPROM for Vicky
	{MOT_VICKY_S5KHM6_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_VICKY_S5KGM1_SENSOR_ID, 0xB0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{MOT_VICKY_HI1634Q_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
#if defined(CONFIG_MTK_VICKY_EEPROM_PROJECT)
	{MOT_VICKY_S5K4H7_SENSOR_ID, 0x5A, mot_s5k4h7_read_region},  // otp
	{MOT_VICKY_OV02B10_SENSOR_ID, 0x78, mot_ov02b10_read_region},  // otp
#endif
	{OV48B_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{IMX766_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_32K},
	{IMX766DUAL_SENSOR_ID, 0xA2, Common_read_region, MAX_EEPROM_SIZE_16K},
	{S5K3P9SP_SENSOR_ID, 0xA0, Common_read_region},
	{IMX481_SENSOR_ID, 0xA2, Common_read_region},
	{IMX586_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{IMX576_SENSOR_ID, 0xA2, Common_read_region},
	{IMX519_SENSOR_ID, 0xA0, Common_read_region},
	{S5K3M5SX_SENSOR_ID, 0xA0, Common_read_region, MAX_EEPROM_SIZE_16K},
	{IMX350_SENSOR_ID, 0xA0, Common_read_region},
	{IMX499_SENSOR_ID, 0xA0, Common_read_region},
#endif
	/*  ADD before this line */
	{0, 0, 0}       /*end of list */
};

unsigned int cam_cal_get_sensor_list(
	struct stCAM_CAL_LIST_STRUCT **ppCamcalList)
{
	if (ppCamcalList == NULL)
		return 1;

	*ppCamcalList = &g_camCalList[0];
	return 0;
}



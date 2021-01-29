/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef __SAIPAN_CXT_GC02M1_OTP_H__
#define __SAIPAN_CXT_GC02M1_OTP_H__
#endif

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"
#include "cam_cal_define.h"
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/unistd.h>

#define EEPROM_P24C64F_ID 0xA4


kal_uint16 read_eeprom(kal_uint32 addr);

static struct stCAM_CAL_DATAINFO_STRUCT st_rear_saipan_cxt_gc02m1_eeprom_data ={
	.sensorID= SAIPAN_CXT_GC02M1_SENSOR_ID,
	.deviceID = 0x10,
	.dataLength = 0x0770,
	.sensorVendorid = 0x16020000,
	.vendorByte = {1,2,3,4},
	.dataBuffer = NULL,
};
static struct stCAM_CAL_CHECKSUM_STRUCT st_rear_saipan_cxt_gc02m1_Checksum[8] =
{
	{MODULE_ITEM,0x0000,0x0000,0x0007,0x0008,0x55},
	{AWB_ITEM,0x0009,0x0009,0x0019,0x001A,0x55},
	{LSC_ITEM,0x001B,0x001B,0x0767,0x0768,0x55},
	{TOTAL_ITEM,0x0000,0x0000,0x0768,0x0769,0x55},
	{MAX_ITEM,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x55},  // this line must haved
};
extern int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData,
	struct stCAM_CAL_CHECKSUM_STRUCT* checkData);
extern int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData);

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);

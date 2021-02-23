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

#ifndef __SAIPAN_QTECH_HI4821Q_OTP_H__
#define __SAIPAN_QTECH_HI4821Q_OTP_H__

#include "kd_camera_typedef.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"
#include "cam_cal_define.h"
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/unistd.h>

#define EEPROM_BL24SA64D_ID 0xA0
#define SAIPAN_QTECH_HI4821Q_XGC_QGC_PGC_CALIB 0
#define SAIPAN_QTECH_HI4821Q_OTP_DUMP 0

#define SAIPAN_QTECH_HI4821Q_OTP_ENABLE 1

#if SAIPAN_QTECH_HI4821Q_OTP_ENABLE
kal_uint16 saipan_qtech_hi4821q_read_eeprom(kal_uint32 addr);
#endif

#if SAIPAN_QTECH_HI4821Q_XGC_QGC_PGC_CALIB
//  OTP information setting
#define XGC_BLOCK_X  9
#define XGC_BLOCK_Y  7
#define QGC_BLOCK_X  9
#define QGC_BLOCK_Y  7
#define PGC_BLOCK_X  13
#define PGC_BLOCK_Y  11

// SRAM Information
#define SRAM_XGC_START_ADDR_48M     0x43F0
#define SRAM_QGC_START_ADDR_48M     0x4000
#define SRAM_PGC_START_ADDR_48M     0x4000  //0x8980 (2020.9.14)

u8* pgc_data_buffer = NULL;
u8* qgc_data_buffer = NULL;
u8* xgc_data_buffer = NULL;

#define PGC_DATA_SIZE 572
#define QGC_DATA_SIZE 1008
#define XGC_DATA_SIZE 693

#if SAIPAN_QTECH_HI4821Q_OTP_DUMP
extern void dumpEEPROMData(int u4Length,u8* pu1Params);
#endif
#endif

#if SAIPAN_QTECH_HI4821Q_XGC_QGC_PGC_CALIB
static void apply_sensor_Cali(void);
#endif

#if SAIPAN_QTECH_HI4821Q_OTP_ENABLE
static struct stCAM_CAL_DATAINFO_STRUCT st_rear_saipan_qtech_hi4821q_eeprom_data ={
	.sensorID= SAIPAN_QTECH_HI4821Q_SENSOR_ID,
	.deviceID = 0x01,
	.dataLength = 0x1638,
	.sensorVendorid = 0x0B000000,
	.vendorByte = {1,2,3,4},
	.dataBuffer = NULL,
};

static struct stCAM_CAL_CHECKSUM_STRUCT st_rear_saipan_qtech_hi4821q_Checksum[11] =
{
	{MODULE_ITEM,0x0000,0x0000,0x0007,0x0008,0x55},
	{AWB_ITEM,0x0009,0x0009,0x0019,0x001A,0x55},
	{AF_ITEM,0x001B,0x001B,0x0020,0x0021,0x55},
	{LSC_ITEM,0x0022,0x0022,0x076E,0x076F,0x55},
	{PDAF_ITEM,0x0770,0x0770,0x0960,0x0961,0x55},
	{PDAF_PROC2_ITEM,0x0962,0x0962,0x0D4E,0x0D4F,0x55},
	{SAIPAN_QTECH_HI4821Q_XGC,0x0D50,0x0D50,0x1005,0x1006,0x55},
	{SAIPAN_QTECH_HI4821Q_QGC,0x1007,0x1007,0x13F7,0x13F8,0x55},
	{SAIPAN_QTECH_HI4821Q_PGC,0x13F9,0x13F9,0x1635,0x1636,0x55},
	{TOTAL_ITEM,0x0000,0x0000,0x1636,0x1637,0x55},
	{MAX_ITEM,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x55},  // this line must haved
};

extern int imgSensorReadEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData,
	struct stCAM_CAL_CHECKSUM_STRUCT* checkData);
extern int imgSensorSetEepromData(struct stCAM_CAL_DATAINFO_STRUCT* pData);

#endif

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);

#endif

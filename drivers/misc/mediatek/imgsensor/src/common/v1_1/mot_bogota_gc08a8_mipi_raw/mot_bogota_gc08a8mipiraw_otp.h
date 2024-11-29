/*
 * Copyright (C) 2021 lucas (guoqiang8@lenovo.com).
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

#ifndef _MOT_BOGOTA_GC08A8_MIPI_RAW_OTP_H_
#define _MOT_BOGOTA_GC08A8_MIPI_RAW_OTP_H_

/*begin 20220402 add for otp check*/

#define MODULE_GROUP_FLAG 0x15A0
#define AWB_GROUP_FLAG 0x16D8
#define LSC_GROUP_FLAG 0x17B0

#define GROUP1_MODULE_INFO_FLAG 0x15A8
#define GROUP1_AWB_INFO_FLAG 0x16E0
#define GROUP1_LSC_INFO_FLAG 0x17B8

#define GROUP2_MODULE_INFO_FLAG 0x1640
#define GROUP2_AWB_INFO_FLAG 0x1748
#define GROUP2_LSC_INFO_FLAG 0x5220

#define MODULE_LENGTH 19
#define AWB_LENGTH 13
#define LSC_LENGTH 1869

struct mot_bogota_gc08a8_otp_t {
	kal_uint8  module_flag;
	kal_uint8  module_param[18];
	kal_uint8  moduleChksum;
	kal_uint8  awb_flag;
	kal_uint8  awb_param[12];
	kal_uint8  awbChksum;
	kal_uint8  lsc_flag;
	kal_uint8  lsc_param[1868];
	kal_uint8  lscChksum;
};

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

struct BOGOTA_GC08A8_eeprom_t {
	uint8_t lens_id;
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
#endif

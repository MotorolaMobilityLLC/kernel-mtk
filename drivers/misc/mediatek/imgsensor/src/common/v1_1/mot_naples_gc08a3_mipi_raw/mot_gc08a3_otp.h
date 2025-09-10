/*
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

#ifndef _MOT_NAPLES_GC08A3_MIPI_RAW_OTP_H_
#define _MOT_NAPLES_GC08A3_MIPI_RAW_OTP_H_

/*begin 20220402 add for otp check*/

#define MODULE_GROUP_FLAG 0x15A0

#define GROUP1_MODULE_INFO 0x15A8
#define GROUP1_AWB_FLAG 0x16E0
#define GROUP1_AWB_INFO 0x16E8
#define GROUP1_LSC_FLAG 0x1770
#define GROUP1_LSC_INFO 0x1778
#define GROUP1_MODULE_CRC 0x16d0

#define GROUP2_MODULE_INFO 0x51e8
#define GROUP2_AWB_FLAG 0x5320
#define GROUP2_AWB_INFO 0x5328
#define GROUP2_LSC_FLAG 0x53b0
#define GROUP2_LSC_INFO 0x53b8
#define GROUP2_MODULE_CRC 0x5310

#define GROUP3_MODULE_INFO 0x8e28
#define GROUP3_AWB_FLAG 0x8f60
#define GROUP3_AWB_INFO 0x8f68
#define GROUP3_LSC_FLAG 0x8ff0
#define GROUP3_LSC_INFO 0x8ff8
#define GROUP3_MODULE_CRC 0x8f50

#define MODULE_LENGTH 37
#define AWB_LENGTH 16
#define LSC_LENGTH 1868
#define LSC_OFFSET 58
#define ALL_DATA_SIZE 1928


struct mot_naples_gc08a3_otp_t {
	kal_uint8  module_param[37];
	kal_uint8  moduleChksum[2];
	kal_uint8  awb_flag;
	kal_uint8  awb_param[16];
	kal_uint8  awbChksum;
	kal_uint8  lsc_flag;
	kal_uint8  lsc_param[1868];
	kal_uint8  lscChksum;
	kal_uint8  allDataChksum;
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
#endif
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

#ifndef _MOT_SYDNEY_GC08A3_UW_MIPI_RAW_OTP_H_
#define _MOT_SYDNEY_GC08A3_UW_MIPI_RAW_OTP_H_

/*begin 20251206 add for otp check*/

#define UW_MODULE_GROUP_FLAG 0x15A0

#define UW_GROUP1_MODULE_INFO 0x15A8
#define UW_GROUP1_AWB_FLAG 0x1638
#define UW_GROUP1_AWB_INFO 0x1640
#define UW_GROUP1_LSC_FLAG 0x16C8
#define UW_GROUP1_LSC_INFO 0x16D0
#define UW_GROUP1_MODULE_CRC 0x1630

#define UW_GROUP2_MODULE_INFO 0x51D8
#define UW_GROUP2_AWB_FLAG 0x5268
#define UW_GROUP2_AWB_INFO 0x5270
#define UW_GROUP2_LSC_FLAG 0x52F8
#define UW_GROUP2_LSC_INFO 0x5300
#define UW_GROUP2_MODULE_CRC 0x5260

#define UW_GROUP3_MODULE_INFO 0x8e08
#define UW_GROUP3_AWB_FLAG 0x8e98
#define UW_GROUP3_AWB_INFO 0x8ea0
#define UW_GROUP3_LSC_FLAG 0x8f28
#define UW_GROUP3_LSC_INFO 0x8f30
#define UW_GROUP3_MODULE_CRC 0x8e90

#define UW_MODULE_LENGTH 17
#define UW_AWB_LENGTH 16
#define UW_LSC_LENGTH 1868
#define UW_LSC_OFFSET 0
#define UW_ALL_DATA_SIZE 1926


struct mot_sydney_gc08a3_uw_otp_t {
	kal_uint8  module_param[UW_MODULE_LENGTH];
	kal_uint8  moduleChksum;
	kal_uint8  awb_flag;
	kal_uint8  awb_param[UW_AWB_LENGTH];
	kal_uint8  awbChksum;
	kal_uint8  lsc_flag;
	kal_uint8  lsc_param[UW_LSC_LENGTH];
	kal_uint8  lscChksum;
	//kal_uint8  allDataChksum;
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
	u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
#endif
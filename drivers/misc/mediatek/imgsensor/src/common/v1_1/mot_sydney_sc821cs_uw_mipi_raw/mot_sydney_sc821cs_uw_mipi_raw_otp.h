/*
 * Copyright (C) 2018 MediaTek Inc.
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
#include "kd_camera_typedef.h"

#include <linux/i2c.h>
/*MODULE*/
#define OTP_MODULE_FLAG 0x827A
#define MODULE_GROUP_INFO_PAGE 2
#define MODULE_GROUP1_INFO_ADDR 0x827B
#define MODULE_GROUP1_CHECKSUM 0x82A0

#define MODULE_GROUP2_INFO_PAGE 7
#define MODULE_GROUP2_INFO_ADDR 0x8C7B
#define MODULE_GROUP2_CHECKSUM 0x8CA0

#define MODULE_INFO_LENGTH 37

/*awb*/
#define OTP_AWB_GROUP1_FLAG    0x82A2
#define AWB_GROUP1_INFO_PAGE 2
#define AWB_GROUP1_INFO_ADDR 0x82A3
#define AWB_GROUP1_CHECKSUM 0x82B3

#define OTP_AWB_GROUP2_FLAG    0x8CA2
#define AWB_GROUP2_INFO_PAGE 7
#define AWB_GROUP2_INFO_ADDR 0x8CA3
#define AWB_GROUP2_CHECKSUM 0x8CB3

#define AWB_INFO_LENGTH 16

/*lsc group 1*/
#define LSC_INFO_LENGTH 1868

#define OTP_LSC_GROUP1_FLAG    0x82B4

#define LSC_GROUP1_PART1_INFO_PAGE 2
#define LSC_GROUP1_PART1_INFO_ADDR 0x82B5
#define LSC_GROUP1_PART1_INFO_LENGTH 331

#define LSC_GROUP1_PART2_INFO_PAGE 3
#define LSC_GROUP1_PART2_INFO_ADDR 0x847A
#define LSC_GROUP1_PART2_INFO_LENGTH 390

#define LSC_GROUP1_PART3_INFO_PAGE 4
#define LSC_GROUP1_PART3_INFO_ADDR 0x867A
#define LSC_GROUP1_PART3_INFO_LENGTH 390

#define LSC_GROUP1_PART4_INFO_PAGE 5
#define LSC_GROUP1_PART4_INFO_ADDR 0x887A
#define LSC_GROUP1_PART4_INFO_LENGTH 390

#define LSC_GROUP1_PART5_INFO_PAGE 6
#define LSC_GROUP1_PART5_INFO_ADDR 0x8A7A
#define LSC_GROUP1_PART5_INFO_LENGTH 367

#define LSC_GROUP1_CHECKSUM 0x8BE9
#define TOTAL_GROUP1_INFO_PAGE 6
#define TOTAL_GROUP1_CHECKSUM 0x8BEA

/*lsc group 2*/
#define OTP_LSC_GROUP2_FLAG    0x8CB4

#define LSC_GROUP2_PART1_INFO_PAGE 7
#define LSC_GROUP2_PART1_INFO_ADDR 0x8CB5
#define LSC_GROUP2_PART1_INFO_LENGTH 331

#define LSC_GROUP2_PART2_INFO_PAGE 8
#define LSC_GROUP2_PART2_INFO_ADDR 0x8E7A
#define LSC_GROUP2_PART2_INFO_LENGTH 390

#define LSC_GROUP2_PART3_INFO_PAGE 9
#define LSC_GROUP2_PART3_INFO_ADDR 0x907A
#define LSC_GROUP2_PART3_INFO_LENGTH 390

#define LSC_GROUP2_PART4_INFO_PAGE 10
#define LSC_GROUP2_PART4_INFO_ADDR 0x927A
#define LSC_GROUP2_PART4_INFO_LENGTH 390

#define LSC_GROUP2_PART5_INFO_PAGE 11
#define LSC_GROUP2_PART5_INFO_ADDR 0x947A
#define LSC_GROUP2_PART5_INFO_LENGTH 367

#define LSC_GROUP2_CHECKSUM 0x95E9

#define TOTAL_GROUP2_INFO_PAGE 11
#define TOTAL_GROUP2_CHECKSUM 0x95EA

#define ALL_DATA_SIZE 1928
extern struct mot_sydney_sc821cs_uw_otp_t mot_sydney_sc821cs_uw_otp_info;

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

struct SYDNEY_SC821CS_UW_eeprom_t{
	uint8_t lens_id;
};

struct mot_sydney_sc821cs_uw_otp_t {
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
		u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
unsigned int sc821cs_uw_read_region(struct i2c_client *client,
				unsigned int addr,
				unsigned char *data,
				unsigned int size);

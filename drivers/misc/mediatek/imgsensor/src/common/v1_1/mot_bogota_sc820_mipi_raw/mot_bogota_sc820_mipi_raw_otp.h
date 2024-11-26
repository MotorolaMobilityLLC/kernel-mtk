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
#define MODULE_GROUP1_CHECKSUM 0x8291

#define MODULE_GROUP2_INFO_ADDR 0x8292
#define MODULE_GROUP2_CHECKSUM 0x82A8

#define MODULE_INFO_LENGTH 22

/*awb*/
#define OTP_AWB_FLAG    0x82A9
#define AWB_GROUP_INFO_PAGE 2

#define AWB_GROUP1_INFO_ADDR 0x82AA
#define AWB_GROUP1_CHECKSUM 0x82C1

#define AWB_GROUP2_INFO_ADDR 0x82C2
#define AWB_GROUP2_CHECKSUM 0x82D9

#define AWB_INFO_LENGTH 23

/*OC*/
#define OTP_OC_FLAG     0x82DA
#define OC_GROUP_INFO_PAGE 2

#define OC_GROUP1_INFO_ADDR 0x82DB
#define OC_GROUP1_CHECKSUM  0x82EC

#define OC_GROUP2_INFO_ADDR 0x82ED
#define OC_GROUP2_CHECKSUM  0X82FE

#define OC_INFO_LENGTH 17

/*lsc*/
#define OTP_LSC_FLAG    0x82FF

#define LSC_GROUP1_PART1_INFO_PAGE 2
#define LSC_GROUP1_PART1_INFO_ADDR 0x8300
#define LSC_GROUP1_PART1_INFO_LENGTH 256

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
#define LSC_GROUP1_PART5_INFO_LENGTH 390

#define LSC_GROUP1_PART6_INFO_PAGE 7
#define LSC_GROUP1_PART6_INFO_ADDR 0x8C7A
#define LSC_GROUP1_PART6_INFO_LENGTH 52

#define LSC_GROUP1_CHECKSUM 0x8CAE

#define LSC_GROUP2_PART1_INFO_PAGE 7
#define LSC_GROUP2_PART1_INFO_ADDR 0x8CAF
#define LSC_GROUP2_PART1_INFO_LENGTH 337

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
#define LSC_GROUP2_PART5_INFO_LENGTH 361

#define LSC_GROUP2_CHECKSUM 0x95E3

#define LSC_INFO_LENGTH 1868

extern struct mot_bogota_sc820_otp_t mot_bogota_sc820_otp_info;

typedef enum {
	NO_ERRORS,
	CRC_FAILURE,
	LIMIT_FAILURE
} calibration_status_t;

struct BOGOTA_SC820_eeprom_t{
	uint8_t lens_id;
};

struct mot_bogota_sc820_otp_t {
    u8  module_flag;
    u8  module_param[22]; //u8  module_param[9];
    u8  module_checksum;
    u8  awb_param[23];
    u8  awb_checksum;
    u8  lsc_param[1868];
    u8  lsc_checksum;
};

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
		u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
unsigned int mot_bogota_sc820_read_region(struct i2c_client *client,
				unsigned int addr,
				unsigned char *data,
				unsigned int size);

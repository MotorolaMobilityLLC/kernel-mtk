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

#ifndef __MOT_MILAN_GC02M1_OTP_H__
#define __MOT_MILAN_GC02M1_OTP_H__

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
		       u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);

#define EEPROM_DATA_PATH "/data/vendor/camera_dump/mot_gc02m1_eeprom.bin"
#define SERIAL_DATA_PATH "/data/vendor/camera_dump/serial_number_depth.bin"
#define DUMP_DEPTH_SERIAL_NUMBER_SIZE 3

unsigned int gc02m1_read_all_data(mot_calibration_info_t * pOtpCalInfo);

typedef struct gc02m1_otp_data {
	u8 program_flag;
	u8 manufacturer_id[2];
	u8 manufacture_date[3];
	u8 serial_number[3];
} gc02m1_otp_data_t;

#endif


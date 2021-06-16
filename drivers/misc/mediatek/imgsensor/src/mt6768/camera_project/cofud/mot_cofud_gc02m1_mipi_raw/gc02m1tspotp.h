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

#ifndef __GC02M1TSP_OTP_H__
#define __GC02M1TSP_OTP_H__

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData,
		       u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);

bool GC02M1TSP_otp_update(void);
unsigned int gc02m1_read_all_data(mot_calibration_info_t * pOtpCalInfo);

typedef struct gc02m1_group_data {
	u8 wb_flag;
	u8 wb_data[6];
	u8 wb_checksum[2];
} gc02m1_group_data_t;

typedef struct gc02m1_otp {
	struct gc02m1_group_data group_data[3];
	u8 mtk_info[3][14];
} gc02m1_otp_t;

#endif

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
#ifndef _OV02B10MIPI_OTP_H
#define _OV02B10MIPI_OTP_H

#define OV02B10_OTP_SIZE 31

#define DEVONN_OV02B10_OTP_CRC_AWB_GROUP1_CAL_SIZE 7
#define DEVONN_OV02B10_OTP_CRC_AWB_GROUP2_CAL_SIZE 6
#define OV02B10_AWB_DATA_SIZE 15
#define OV02B10_SERIAL_NUM_SIZE 16

typedef enum {
    NO_ERRORS,
    CRC_FAILURE,
    LIMIT_FAILURE
} calibration_status_t;

typedef struct {
        calibration_status_t mnf_status;
        calibration_status_t af_status;
        calibration_status_t awb_status;
        calibration_status_t lsc_status;
        calibration_status_t pdaf_status;
        calibration_status_t dual_status;
} ov02b10_calibration_status_t;

extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData,u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern int iReadRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing);
extern int iWriteRegI2CTiming(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId, u16 timing);
extern void kdSetI2CSpeed(u16 i2cSpeed);
extern int iReadReg(u16 a_u2Addr, u8 *a_puBuff, u16 i2cId);
extern int iWriteReg(u16 a_u2Addr, u32 a_u4Data, u32 a_u4Bytes, u16 i2cId);
extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing);
int ov02b10_read_data_from_otp(void);
void DEVONN_OV02B10_eeprom_format_calibration_data(void *data, ov02b10_calibration_status_t *ov02b10_cal_info);
#endif


/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include "imgsensor_i2c.h"

#ifdef IMGSENSOR_LEGACY_COMPAT
extern struct IMGSENSOR_I2C_CFG *pgi2c_cfg_legacy;

void kdSetI2CSpeed(u16 i2cSpeed)
{

}

int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId)
{
	return imgsensor_i2c_read(pgi2c_cfg_legacy, a_pSendData, a_sizeSendData, a_pRecvData, a_sizeRecvData, i2cId, IMGSENSOR_I2C_SPEED);
}

int iReadRegI2CTiming(u8 *a_pSendData , u16 a_sizeSendData, u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId, u16 timing)
{
	return imgsensor_i2c_read(pgi2c_cfg_legacy, a_pSendData, a_sizeSendData, a_pRecvData, a_sizeRecvData, i2cId, timing * 1000);
}

int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId)
{
	return imgsensor_i2c_write(pgi2c_cfg_legacy, a_pSendData, a_sizeSendData, a_sizeSendData, i2cId, IMGSENSOR_I2C_SPEED);
}

int iWriteRegI2CTiming(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId, u16 timing)
{
	return imgsensor_i2c_write(pgi2c_cfg_legacy, a_pSendData, a_sizeSendData, a_sizeSendData, i2cId, timing * 1000);
}

int iMultiWriteReg(u8 *pData, u16 lens, u16 i2cId)
{
	return imgsensor_i2c_write(pgi2c_cfg_legacy, pData, lens, lens, i2cId, IMGSENSOR_I2C_SPEED);
}

int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length, u16 timing)
{
	return imgsensor_i2c_write(pgi2c_cfg_legacy, pData, bytes, transfer_length, i2cId, timing * 1000);
}

int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId)
{
    return iBurstWriteReg_multi(pData, bytes, i2cId, bytes, IMGSENSOR_I2C_SPEED / 1000);
}
#endif

void ISP_MCLK1_EN(BOOL En)
{
}
void ISP_MCLK2_EN(BOOL En)
{
}
void ISP_MCLK3_EN(BOOL En)
{
}
void ISP_MCLK4_EN(BOOL En)
{
}


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

#ifndef __IMGSENSOR_LEGACY_H__
#define __IMGSENSOR_LEGACY_H__

#include "imgsensor_sensor.h"
#include "imgsensor_i2c.h"

static SENSOR_FUNCTION_STRUCT *psensor_func;

#define kdSetI2CSpeed(i2cSpeed)

#define iReadRegI2C(a_pSendData, a_sizeSendData, a_pRecvData, a_sizeRecvData, i2cId)\
	imgsensor_i2c_read(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (a_pSendData), (a_sizeSendData), (a_pRecvData), (a_sizeRecvData), (i2cId), IMGSENSOR_I2C_SPEED)

#define iReadRegI2CTiming(a_pSendData, a_sizeSendData, a_pRecvData, a_sizeRecvData, i2cId, timing)\
	imgsensor_i2c_read(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (a_pSendData), (a_sizeSendData), (a_pRecvData), (a_sizeRecvData), (i2cId), (timing))

#define iWriteRegI2C(a_pSendData, a_sizeSendData, i2cId)\
	imgsensor_i2c_write(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (a_pSendData), (a_sizeSendData), (a_sizeSendData), (i2cId), IMGSENSOR_I2C_SPEED)

#define iWriteRegI2CTiming(a_pSendData, a_sizeSendData, i2cId, timing)\
	imgsensor_i2c_write(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (a_pSendData), (a_sizeSendData), (a_sizeSendData), (i2cId), (timing))

#define iBurstWriteReg(pData, bytes, i2cId)\
	imgsensor_i2c_write(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (pData), (bytes), (bytes), (i2cId), IMGSENSOR_I2C_SPEED)

#define iBurstWriteReg_multi(pData, bytes, i2cId, transfer_length, timing)\
	imgsensor_i2c_write(&((struct IMGSENSOR_SENSOR_INST *)psensor_func->psensor_inst)->i2c_cfg, (pData), (bytes), (transfer_length), (i2cId), (timing))

#define imgsensor_legacy_init(sensor_func)\
do {\
	psensor_func       = sensor_func;\
	psensor_func->arch = IMGSENSOR_ARCH_V2;\
} while(0)

#endif


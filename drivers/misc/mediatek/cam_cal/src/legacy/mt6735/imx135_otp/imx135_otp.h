/*
 * Copyright (C) 2016 MediaTek Inc.
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
#ifndef __CAM_CAL_H
#define __CAM_CAL_H

#define CAM_CAL_DEV_MAJOR_NUMBER 226

/* CAM_CAL READ/WRITE ID */
#define IMX135_OTP_DEVICE_ID                                                    0x20/* slave id of imx135 */
/* #define I2C_UNIT_SIZE                                  1 //in byte */
/* #define OTP_START_ADDR                            0x0A04 */
/* #define OTP_SIZE                                      24 */

extern kal_uint16 read_cmos_sensor(kal_uint32 addr);
extern void write_cmos_sensor(kal_uint32 addr, kal_uint32 para);
extern void write_cmos_sensor_16(kal_uint16 addr, kal_uint16 para);
extern void write_cmos_sensor_8(kal_uint16 addr, kal_uint8 para);

#endif /* __CAM_CAL_H */


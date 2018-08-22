/*
 * mmc3630x.h - Definitions for mmc3630x magnetic sensor chip.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __MMC3630x_H__
#define __MMC3630x_H__

#include <linux/ioctl.h>

#define CALIBRATION_DATA_SIZE	12
#define SENSOR_DATA_SIZE	9
#ifndef TRUE
	#define TRUE		1
#endif
#define MMC3630x_I2C_ADDR	0x30	/* 8-bit  0x60 */
#define MMC3630x_DEVICE_ID	0x0A

/* MMC3630x register address */
#define MMC3630X_REG_CTRL		0x08
#define MMC3630X_REG_BITS		0x09
#define MMC3630X_REG_DATA		0x00
#define MMC3630X_REG_DS			0x07
#define MMC3630X_REG_PRODUCTID		0x2F

/* MMC3630x control bit */
#define MMC3630X_CTRL_TM		0x01
#define MMC3630X_CTRL_CM		0x02
#define MMC3630X_CTRL_50HZ		0x00
#define MMC3630X_CTRL_25HZ		0x04
#define MMC3630X_CTRL_12HZ		0x08
#define MMC3630X_CTRL_SET	        0x08
#define MMC3630X_CTRL_RESET             0x10
#define MMC3630X_CTRL_REFILL            0x20

#define MMC3630X_MAG_REG_ADDR_OTP_GATE0			0x0F
#define MMC3630X_MAG_REG_ADDR_OTP_GATE1			0x12
#define MMC3630X_MAG_REG_ADDR_OTP_GATE2			0x13
#define MMC3630X_MAG_REG_ADDR_OTP				0x2A
#define MMC3630X_MAG_REG_ADDR_CTRL2				0x0A

#define MMC3630X_MAG_OTP_OPEN0					0xE1
#define MMC3630X_MAG_OTP_OPEN1					0x11
#define MMC3630X_MAG_OTP_OPEN2					0x80
#define MMC3630X_MAG_OTP_CLOSE					0x00

#define MMC3630X_BITS_SLOW_16            0x00
#define MMC3630X_BITS_FAST_16            0x01
#define MMC3630X_BITS_14                 0x02

/* conversion of magnetic data to uT units */
/* 32768 = 1Guass = 100 uT */
/* 100 / 32768 = 25 / 8192 */
/* 65536 = 360Degree */
/* 360 / 65536 = 45 / 8192 */


/* #define CONVERT_M_DIV			8192 */
#define CONVERT_M_DIV			1024000

/* sensitivity 1024 count = 1 Guass = 100uT */

#define MMC3630X_OFFSET_X		32768
#define MMC3630X_OFFSET_Y		32768
#define MMC3630X_OFFSET_Z		32768
#define MMC3630X_SENSITIVITY_X		1024
#define MMC3630X_SENSITIVITY_Y		1024
#define MMC3630X_SENSITIVITY_Z		1024

#endif /* __MMC3630x_H__ */


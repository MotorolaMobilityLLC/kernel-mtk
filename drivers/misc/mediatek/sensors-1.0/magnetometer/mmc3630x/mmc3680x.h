/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2017
 *****************************************************************************/
 
/*
 * mmc3680x.h - Definitions for mmc3680x magnetic sensor chip.
 */
 
 
#ifndef __MMC3680x_H__
#define __MMC3680x_H__

#include <linux/ioctl.h>

#define CALIBRATION_DATA_SIZE	12
#define SENSOR_DATA_SIZE 	9
#ifndef TRUE
	#define TRUE 		1
#endif
#define MMC3680x_I2C_ADDR	0x30	// 8-bit  0x60
#define MMC3680x_DEVICE_ID	0x0A

/* MMC3680x register address */
#define MMC3680X_REG_CTRL		0x08
#define MMC3680X_REG_BITS		0x09
#define MMC3680X_REG_DATA		0x00
#define MMC3680X_REG_DS			0x07
#define MMC3680X_REG_PRODUCTID		0x2F

/* MMC3680x control bit */
#define MMC3680X_CTRL_TM		0x01
#define MMC3680X_CTRL_CM		0x02
#define MMC3680X_CTRL_50HZ		0x00
#define MMC3680X_CTRL_25HZ		0x04
#define MMC3680X_CTRL_12HZ		0x08
#define MMC3680X_CTRL_SET  	        0x08
#define MMC3680X_CTRL_RESET             0x10
#define MMC3680X_CTRL_REFILL            0x20

#define MMC3680X_MAG_REG_ADDR_OTP_GATE0        		0x0F
#define MMC3680X_MAG_REG_ADDR_OTP_GATE1        		0x12
#define MMC3680X_MAG_REG_ADDR_OTP_GATE2        		0x13
#define MMC3680X_MAG_REG_ADDR_OTP         			0x2A
#define MMC3680X_MAG_REG_ADDR_CTRL2        			0x0A

#define MMC3680X_MAG_OTP_OPEN0  					0xE1
#define MMC3680X_MAG_OTP_OPEN1  					0x11
#define MMC3680X_MAG_OTP_OPEN2  					0x80
#define MMC3680X_MAG_OTP_CLOSE  					0x00

#define MMC3680X_BITS_SLOW_16            0x00
#define MMC3680X_BITS_FAST_16            0x01
#define MMC3680X_BITS_14                 0x02

// conversion of magnetic data to uT units
// 32768 = 1Guass = 100 uT
// 100 / 32768 = 25 / 8192
// 65536 = 360Degree
// 360 / 65536 = 45 / 8192


//#define CONVERT_M_DIV			8192
#define CONVERT_M_DIV			1024000

// sensitivity 1024 count = 1 Guass = 100uT

#define MMC3680X_OFFSET_X		32768
#define MMC3680X_OFFSET_Y		32768
#define MMC3680X_OFFSET_Z		32768
#define MMC3680X_SENSITIVITY_X		1024
#define MMC3680X_SENSITIVITY_Y		1024
#define MMC3680X_SENSITIVITY_Z		1024


#if 0
#define ECOMPASS_IOC_SET_YPR            	_IOW(MSENSOR, 0x21, int[CALIBRATION_DATA_SIZE])
#define COMPAT_ECOMPASS_IOC_SET_YPR            _IOW(MSENSOR, 0x21, compat_int_t[CALIBRATION_DATA_SIZE])
#endif
#define COMPAT_MMC3680X_IOC_READ_REG           _IOWR(MSENSOR, 0x32, unsigned char)
#define COMPAT_MMC3680X_IOC_WRITE_REG          _IOW(MSENSOR, 0x33, unsigned char[2])
#define COMPAT_MMC3680X_IOC_READ_REGS          _IOWR(MSENSOR, 0x34, unsigned char[10])
#define MMC3680X_IOC_READ_REG		    _IOWR(MSENSOR, 0x23, unsigned char)
#define MMC3680X_IOC_WRITE_REG		    _IOW(MSENSOR,  0x24, unsigned char[2])
#define MMC3680X_IOC_READ_REGS		    _IOWR(MSENSOR, 0x25, unsigned char[10])

#endif /* __MMC3680x_H__ */


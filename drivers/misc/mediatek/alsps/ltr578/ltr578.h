
/* Lite-On LTR-578ALS Linux Driver
 *
 * Copyright (C) 2015 Lite-On Technology Corp (Singapore)
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _LTR578_H
#define _LTR578_H

#include <linux/ioctl.h>
/* LTR-578 Registers */
#define LTR578_MAIN_CTRL		0x00
#define LTR578_PS_LED			0x01
#define LTR578_PS_PULSES		0x02
#define LTR578_PS_MEAS_RATE		0x03
#define LTR578_ALS_MEAS_RATE	0x04
#define LTR578_ALS_GAIN			0x05


#define LTR578_INT_CFG			0x19
#define LTR578_INT_PST			0x1A
#define LTR578_PS_THRES_UP_0	0x1B
#define LTR578_PS_THRES_UP_1	0x1C
#define LTR578_PS_THRES_LOW_0	0x1D
#define LTR578_PS_THRES_LOW_1	0x1E

#define LTR578_PS_CAN_0			0x1F
#define LTR578_PS_CAN_1			0x20
#define LTR578_ALS_THRES_UP_0	0x21
#define LTR578_ALS_THRES_UP_1	0x22
#define LTR578_ALS_THRES_UP_2	0x23
#define LTR578_ALS_THRES_LOW_0	0x24
#define LTR578_ALS_THRES_LOW_1	0x25
#define LTR578_ALS_THRES_LOW_2	0x26



/* 578's Read Only Registers */
#define LTR578_PART_ID			0x06
#define LTR578_MAIN_STATUS		0x07
#define LTR578_PS_DATA_0		0x08
#define LTR578_PS_DATA_1		0x09
#define LTR578_ALS_DATA_0		0x0D
#define LTR578_ALS_DATA_1		0x0E
#define LTR578_ALS_DATA_2		0x0F


/* Basic Operating Modes */
#define MODE_ALS_Range1		0x00  /* /for als gain x1 */
#define MODE_ALS_Range3		0x01  /* /for als  gain x3 */
#define MODE_ALS_Range6		0x02  /* /for als  gain x6 */
#define MODE_ALS_Range9		0x03  /* /for als gain x9 */
#define MODE_ALS_Range18	0x04  /* /for als gain x18 */

#define ALS_RANGE_1		1
#define ALS_RANGE_3	3
#define ALS_RANGE_6	6
#define ALS_RANGE_9	9
#define ALS_RANGE_18	18

#define ALS_RESO_MEAS	0x22
/*
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR578_IOCTL_MAGIC      'c'

/* IOCTLs for LTR578 device */
#define LTR578_IOCTL_PS_ENABLE		_IOW(LTR578_IOCTL_MAGIC, 0, char *)
#define LTR578_IOCTL_ALS_ENABLE		_IOW(LTR578_IOCTL_MAGIC, 1, char *)
#define LTR578_IOCTL_READ_PS_DATA	_IOR(LTR578_IOCTL_MAGIC, 2, char *)
#define LTR578_IOCTL_READ_PS_INT	_IOR(LTR578_IOCTL_MAGIC, 3, char *)
#define LTR578_IOCTL_READ_ALS_DATA	_IOR(LTR578_IOCTL_MAGIC, 4, char *)
#define LTR578_IOCTL_READ_ALS_INT	_IOR(LTR578_IOCTL_MAGIC, 5, char *)


/* Power On response time in ms */
#define PON_DELAY	600
#define WAKEUP_DELAY	10

#define LTR578_SUCCESS						0
#define LTR578_ERR_I2C						-1
#define LTR578_ERR_STATUS					-3
#define LTR578_ERR_SETUP_FAILURE			-4
#define LTR578_ERR_GETGSENSORDATA			-5
#define LTR578_ERR_IDENTIFICATION			-6




/* Interrupt vector number to use when probing IRQ number.
 * User changeable depending on sys interrupt.
 * For IRQ numbers used, see /proc/interrupts.
 */
#define GPIO_INT_NO	32

#endif

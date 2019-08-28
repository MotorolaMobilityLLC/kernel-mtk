/* 
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
/*
 * Definitions for LTR577 als/ps sensor chip.
 */
#ifndef _LTR577_H_
#define _LTR577_H_

#include <linux/ioctl.h>
/* LTR-577 Registers */
#define LTR577_MAIN_CTRL		0x00
#define LTR577_PS_LED			0x01
#define LTR577_PS_PULSES		0x02
#define LTR577_PS_MEAS_RATE		0x03
#define LTR577_ALS_MEAS_RATE	0x04
#define LTR577_ALS_GAIN			0x05

#define LTR577_INT_CFG			0x19
#define LTR577_INT_PST 			0x1A
#define LTR577_PS_THRES_UP_0	0x1B
#define LTR577_PS_THRES_UP_1	0x1C
#define LTR577_PS_THRES_LOW_0	0x1D
#define LTR577_PS_THRES_LOW_1	0x1E
#define LTR577_PS_CAN_0			0x1F
#define LTR577_PS_CAN_1			0x20
#define LTR577_ALS_THRES_UP_0	0x21
#define LTR577_ALS_THRES_UP_1	0x22
#define LTR577_ALS_THRES_UP_2	0x23
#define LTR577_ALS_THRES_LOW_0	0x24
#define LTR577_ALS_THRES_LOW_1	0x25
#define LTR577_ALS_THRES_LOW_2	0x26

/* 577's Read Only Registers */
#define LTR577_PART_ID			0x06
#define LTR577_MAIN_STATUS		0x07
#define LTR577_PS_DATA_0		0x08
#define LTR577_PS_DATA_1		0x09
#define LTR577_CLEAR_DATA_0		0x0A
#define LTR577_CLEAR_DATA_1		0x0B
#define LTR577_CLEAR_DATA_2		0x0C
#define LTR577_ALS_DATA_0		0x0D
#define LTR577_ALS_DATA_1		0x0E
#define LTR577_ALS_DATA_2		0x0F

/* Basic Operating Modes */
#define MODE_ALS_Range1			0x00  ///for als gain x1
#define MODE_ALS_Range3			0x01  ///for als gain x3
#define MODE_ALS_Range6			0x02  ///for als gain x6
#define MODE_ALS_Range9			0x03  ///for als gain x9
#define MODE_ALS_Range18		0x04  ///for als gain x18

#define ALS_RANGE_1				1
#define ALS_RANGE_3 			3
#define ALS_RANGE_6 			6
#define ALS_RANGE_9 			9
#define ALS_RANGE_18			18

#define ALS_RESO_MEAS			0x22

#define ALS_WIN_FACTOR			1
#define ALS_WIN_FACTOR2			5
#define ALS_USE_CLEAR_DATA		0
/* 
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR577_IOCTL_MAGIC      'c'

/* IOCTLs for LTR577 device */
#define LTR577_IOCTL_PS_ENABLE		_IOW(LTR577_IOCTL_MAGIC, 0, char *)
#define LTR577_IOCTL_ALS_ENABLE		_IOW(LTR577_IOCTL_MAGIC, 1, char *)
#define LTR577_IOCTL_READ_PS_DATA	_IOR(LTR577_IOCTL_MAGIC, 2, char *)
#define LTR577_IOCTL_READ_PS_INT	_IOR(LTR577_IOCTL_MAGIC, 3, char *)
#define LTR577_IOCTL_READ_ALS_DATA	_IOR(LTR577_IOCTL_MAGIC, 4, char *)
#define LTR577_IOCTL_READ_ALS_INT	_IOR(LTR577_IOCTL_MAGIC, 5, char *)

/* Power On response time in ms */
#define PON_DELAY		50
#define WAKEUP_DELAY	10

#define LTR577_SUCCESS						 0
#define LTR577_ERR_I2C						-1
#define LTR577_ERR_STATUS					-3
#define LTR577_ERR_SETUP_FAILURE			-4
#define LTR577_ERR_GETGSENSORDATA			-5
#define LTR577_ERR_IDENTIFICATION			-6

/* Interrupt vector number to use when probing IRQ number.
 * User changeable depending on sys interrupt.
 * For IRQ numbers used, see /proc/interrupts.
 */
#define GPIO_INT_NO	32

extern struct platform_device *get_alsps_platformdev(void);

#endif

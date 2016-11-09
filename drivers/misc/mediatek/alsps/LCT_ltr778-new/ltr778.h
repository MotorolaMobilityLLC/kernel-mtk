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
 * Definitions for LTR778 als/ps sensor chip.
 */
#ifndef _LTR778_H_
#define _LTR778_H_

#include <linux/ioctl.h>

/* LTR778 Registers */
#define LTR778_ALS_CONTR			0x80
#define LTR778_PS_CONTR				0x81
#define LTR778_PS_LED				0x82
#define LTR778_PS_PULSES			0x83
#define LTR778_PS_MEAS_RATE			0x84
#define LTR778_ALS_MEAS_RATE		0x85
#define LTR778_PART_ID				0x86
#define LTR778_MANUFACTURER_ID		0x87
#define LTR778_ALS_DATA_CH1_0		0x88
#define LTR778_ALS_DATA_CH1_1		0x89
#define LTR778_ALS_DATA_CH0_0		0x8A
#define LTR778_ALS_DATA_CH0_1		0x8B
#define LTR778_ALS_DARK_DATA_CH1_0	0x8C
#define LTR778_ALS_DARK_DATA_CH1_1	0x8D
#define LTR778_ALS_DARK_DATA_CH0_0	0x8E
#define LTR778_ALS_DARK_DATA_CH0_1	0x8F
#define LTR778_ALS_PS_STATUS		0x90
#define LTR778_PS_DATA_0			0x91
#define LTR778_PS_DATA_1			0x92
#define LTR778_INTERRUPT			0x93
#define LTR778_INTERRUPT_PRST		0x94
#define LTR778_PS_THRES_UP_0		0x95
#define LTR778_PS_THRES_UP_1		0x96
#define LTR778_PS_THRES_LOW_0		0x97
#define LTR778_PS_THRES_LOW_1		0x98
#define LTR778_ALS_THRES_UP_0		0x9B
#define LTR778_ALS_THRES_UP_1		0x9C
#define LTR778_ALS_THRES_LOW_0		0x9D
#define LTR778_ALS_THRES_LOW_1		0x9E
#define LTR778_FAULT_DET_SETTING	0x9F
#define LTR778_FAULT_DET_STATUS		0xA0
/* LTR778 Registers */

/* Basic Operating Modes */
#define MODE_ON_Reset			0x02  ///for als reset

#define MODE_ALS_Range1			0x00  ///for als gain x1
#define MODE_ALS_Range2			0x04  ///for als  gain x4
#define MODE_ALS_Range3			0x08  ///for als  gain x16
#define MODE_ALS_Range4			0x0C  ///for als gain x64
#define MODE_ALS_Range5			0x10  ///for als gain x128
#define MODE_ALS_Range6			0x14  ///for als gain x256

#define ALS_RANGE_1				1
#define ALS_RANGE_4 			4
#define ALS_RANGE_16 			16
#define ALS_RANGE_64 			64
#define ALS_RANGE_128			128
#define ALS_RANGE_256 			256

/* Power On response time in ms */
#define PON_DELAY				50
#define WAKEUP_DELAY			10

#define WIN_FACTOR 				1

#define LTR778_SUCCESS						 0
#define LTR778_ERR_I2C						-1
#define LTR778_ERR_STATUS					-3
#define LTR778_ERR_SETUP_FAILURE			-4
#define LTR778_ERR_GETGSENSORDATA			-5
#define LTR778_ERR_IDENTIFICATION			-6

extern struct platform_device *get_alsps_platformdev(void);

#endif
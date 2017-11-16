/*Transsion Top Secret*/
/* 
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
/*
 * Definitions for ltr578 als/ps sensor chip.
 */



#ifndef __LTR578_H__
#define __LTR578_H__

#include <linux/ioctl.h>

extern int ltr578_CMM_PPCOUNT_VALUE;
extern int ltr578_CMM_CONTROL_VALUE;
extern int ZOOM_TIME;


/*REG address*/
#define APS_RW_MAIN_CTRL            0x00 //ALS, PS operation mode control, SW reset
#define APS_RW_PS_LED				0x01 // PS LED setting
#define APS_RW_PS_N_PULSES			0x02 // PS number of pulses
#define APS_RW_PS_MEAS_RATE			0x03 // PS measurement rate in active mode
#define APS_RW_ALS_MEAS_RATE		0x04 // ALS measurement rate in active mode
#define APS_RW_ALS_GAIN				0x05 // ALS analog Gain	
#define APS_RO_PART_ID				0x06 // Part Number ID and Revision ID
#define APS_RO_MAIN_STATUS			0x07 // Power-On status, Interrupt status, Data status
#define APS_RO_PS_DATA_0			0x08 // PS measurement data, lower byte
#define APS_RO_PS_DATA_1			0x09 // PS measurement data, upper byte
#define LTR578_ALS_DATA_0			0x0D
#define LTR578_ALS_DATA_1			0x0E
#define LTR578_ALS_DATA_2			0x0F
#define APS_RW_INTERRUPT			0x19 // Interrupt settings
#define APS_RW_INTERRUPT_PERSIST	0x1A // ALS / PS Interrupt persist setting
#define APS_RW_PS_THRES_UP_0		0x1B // PS interrupt upper threshold, lower byte
#define APS_RW_PS_THRES_UP_1		0x1C // PS interrupt upper threshold, upper byte
#define APS_RW_PS_THRES_LOW_0		0x1D // PS interrupt lower threshold, lower byte
#define APS_RW_PS_THRES_LOW_1		0x1E // PS interrupt lower threshold, upper byte
#define LTR578_PS_CAN_0			    0x1F
#define LTR578_PS_CAN_1			    0x20
#define LTR578_ALS_THRES_UP_0	    0x21
#define LTR578_ALS_THRES_UP_1	    0x22
#define LTR578_ALS_THRES_UP_2	    0x23
#define LTR578_ALS_THRES_LOW_0	    0x24
#define LTR578_ALS_THRES_LOW_1	    0x25
#define LTR578_ALS_THRES_LOW_2	    0x26

#define MODE_ALS_ON_Range1		(0x1 << 3)
#define MODE_ALS_ON_Range2		(0x0 << 3)
#define MODE_ALS_StdBy			0x00

#define MODE_PS_ON_Gain1		(0x0 << 2)
#define MODE_PS_ON_Gain4		(0x1 << 2)
#define MODE_PS_ON_Gain8		(0x2 << 2)
#define MODE_PS_ON_Gain16		(0x3 << 2)
#define MODE_PS_StdBy			0x00

/* Basic Operating Modes */
#define MODE_ALS_Range1		0x00  ///for als gain x1
#define MODE_ALS_Range3		0x01  ///for als  gain x3
#define MODE_ALS_Range6		0x02  ///for als  gain x6
#define MODE_ALS_Range9		0x03  ///for als gain x9
#define MODE_ALS_Range18	0x04  ///for als gain x18

#define ALS_RANGE_1		1
#define ALS_RANGE_3 	3
#define ALS_RANGE_6 	6
#define ALS_RANGE_9 	9
#define ALS_RANGE_18	18

#define PS_RANGE1			1
#define PS_RANGE2			2
#define PS_RANGE4			4
#define PS_RANGE8			8

#define ALS_RANGE1_320			1
#define ALS_RANGE2_64K			2

/* Power On response time in ms */
#define PON_DELAY			600
#define WAKEUP_DELAY			10


#define ltr578_SUCCESS						0
#define ltr578_ERR_I2C						-1
#define ltr578_ERR_STATUS					-3
#define ltr578_ERR_SETUP_FAILURE			-4
#define ltr578_ERR_GETGSENSORDATA			-5
#define ltr578_ERR_IDENTIFICATION			-6



#endif



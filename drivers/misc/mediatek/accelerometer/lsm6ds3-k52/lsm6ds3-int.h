/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/* lsm6ds3.h
 *
 * (C) Copyright 2008 
 * MediaTek <www.mediatek.com>
 *
 * mpu300 head file for MT65xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef L3M6DS3_H
#define L3M6DS3_H
	 
#include <linux/ioctl.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <hwmsen_helper.h>

	 
#define LSM6DS3_I2C_SLAVE_ADDR		0xD0
#define LSM6DS3_FIXED_DEVID			0x69


/* LSM6DS3 Register Map  (Please refer to LSM6DS3 Specifications) */
/*lenovo-sw caoyi1 modify 20150325 begin*/
#define LSM6DS3_FUNC_CFG_ACCESS  0x01
//#define LSM6DS3_FUNC_CFG_ACCESS  0x00
/*lenovo-sw caoyi1 modify 20150325 end*/
#define LSM6DS3_RAM_ACCESS  	0X01
#define LSM6DS3_SENSOR_SYNC_TIME_FRAME 0X04

/*FIFO control register*/
#define LSM6DS3_FIFO_CTRL1 0x06
#define LSM6DS3_FIFO_CTRL2 0x07
#define LSM6DS3_FIFO_CTRL3 0x08
#define LSM6DS3_FIFO_CTRL4 0x09
#define LSM6DS3_FIFO_CTRL5 0x0a

/*Orientation configuration*/
#define LSM6DS3_ORIENT_CFG_G 0x0B

/*Interrupt control*/
#define LSM6DS3_INT1_CTRL 0x0D
#define LSM6DS3_INT2_CTRL 0x0E

#define LSM6DS3_WHO_AM_I 0x0F

/*Acc and Gyro control registers*/
#define LSM6DS3_CTRL1_XL 0x10
#define LSM6DS3_CTRL2_G 0x11
#define LSM6DS3_CTRL3_C	0x12
#define LSM6DS3_CTRL4_C	0x13
#define LSM6DS3_CTRL5_C	0x14
#define LSM6DS3_CTRL6_C	0x15
#define LSM6DS3_CTRL7_G	0x16
#define LSM6DS3_CTRL8_XL	0x17
#define LSM6DS3_CTRL9_XL	0x18
#define LSM6DS3_CTRL10_C	0x19

#define LSM6DS3_MASTER_CONFIG 0x1A
#define LSM6DS3_WAKE_UP_SRC 0x1B
#define LSM6DS3_TAP_SRC 0x1C
#define LSM6DS3_D6D_SRC	0x1D
#define LSM6DS3_STATUS_REG 0x1E

/*Output register*/
#define LSM6DS3_OUT_TEMP_L 0x20
#define LSM6DS3_OUT_TEMP_H 0x21
#define LSM6DS3_OUTX_L_G   0x22
#define LSM6DS3_OUTX_H_G   0x23
#define LSM6DS3_OUTY_L_G   0x24
#define LSM6DS3_OUTY_H_G   0x25
#define LSM6DS3_OUTZ_L_G   0x26
#define LSM6DS3_OUTZ_H_G   0x27

#define LSM6DS3_OUTX_L_XL   0x28
#define LSM6DS3_OUTX_H_XL   0x29
#define LSM6DS3_OUTY_L_XL   0x2A
#define LSM6DS3_OUTY_H_XL   0x2B
#define LSM6DS3_OUTZ_L_XL   0x2C
#define LSM6DS3_OUTZ_H_XL   0x2D

/*Sensor Hub registers*/
#define LSM6DS3_SENSORHUB1_REG 0x2E
#define LSM6DS3_SENSORHUB2_REG 0x2F
#define LSM6DS3_SENSORHUB3_REG 0x30
#define LSM6DS3_SENSORHUB4_REG 0x31
#define LSM6DS3_SENSORHUB5_REG 0x32
#define LSM6DS3_SENSORHUB6_REG 0x33
#define LSM6DS3_SENSORHUB7_REG 0x34
#define LSM6DS3_SENSORHUB8_REG 0x35
#define LSM6DS3_SENSORHUB9_REG 0x36
#define LSM6DS3_SENSORHUB10_REG 0x37
#define LSM6DS3_SENSORHUB11_REG 0x38
#define LSM6DS3_SENSORHUB12_REG 0x39

#define LSM6DS3_SENSORHUB13_REG 0x4D
#define LSM6DS3_SENSORHUB14_REG 0x4E
#define LSM6DS3_SENSORHUB15_REG 0x4F
#define LSM6DS3_SENSORHUB16_REG 0x50
#define LSM6DS3_SENSORHUB17_REG 0x51
#define LSM6DS3_SENSORHUB18_REG 0x52

/*FIFO status registers*/
#define LSM6DS3_FIFO_STATUS1 0x3A
#define LSM6DS3_FIFO_STATUS2 0x3B
#define LSM6DS3_FIFO_STATUS3 0x3C
#define LSM6DS3_FIFO_STATUS4 0x3D

#define LSM6DS3_FIFO_DATA_OUT_L 0x3E
#define LSM6DS3_FIFO_DATA_OUT_H 0x3F

#define LSM6DS3_TIMESTAMP0_REG 0x40
#define LSM6DS3_TIMESTAMP1_REG 0x41
#define LSM6DS3_TIMESTAMP2_REG 0x42

#define LSM6DS3_STEP_TIMESTAMP_L 0x49
#define LSM6DS3_STEP_TIMESTAMP_H 0x4A

#define LSM6DS3_STEP_COUNTER_L	0x4B
#define LSM6DS3_STEP_COUNTER_H	0x4C

#define LSM6DS3_FUNC_SRC 	0x53
#define LSM6DS3_TAP_CFG	0x58
#define LSM6DS3_TAP_THS_6D 0x59
#define LSM6DS3_INT_DUR2	0x5A
#define LSM6DS3_WAKE_UP_THS	0x5B
#define LSM6DS3_WAKE_UP_DUR 0x5C

#define LSM6DS3_FREE_FALL 0x5D
#define LSM6DS3_MD1_CFG 0x5E
#define LSM6DS3_MD2_CFG 0x5F

/*Output Raw data*/
#define LSM6DS3_OUT_MAG_RAW_X_L 0x66
#define LSM6DS3_OUT_MAG_RAW_X_H 0x67
#define LSM6DS3_OUT_MAG_RAW_Y_L 0x68
#define LSM6DS3_OUT_MAG_RAW_Y_H 0x69
#define LSM6DS3_OUT_MAG_RAW_Z_L 0x6A
#define LSM6DS3_OUT_MAG_RAW_Z_H 0x6B

/*Embedded function register*/
#define LSM6DS3_SLV0_ADD 0x02
#define LSM6DS3_SLV0_SUBADD 0x03
#define LSM6DS3_SLAVE0_CONFIG 0x04

#define LSM6DS3_SLV1_ADD 0x05
#define LSM6DS3_SLV1_SUBADD 0x06
#define LSM6DS3_SLAVE1_CONFIG 0x07

#define LSM6DS3_SLV2_ADD 0x08
#define LSM6DS3_SLV2_SUBADD 0x09
#define LSM6DS3_SLAVE2_CONFIG 0x0a

#define LSM6DS3_SLV3_ADD 0x0b
#define LSM6DS3_SLV3_SUBADD 0x0c
#define LSM6DS3_SLAVE3_CONFIG 0x0d

#define LSM6DS3_DATAWRITE_SRC_MODE_SUB_SLV0 0x0e
#define LSM6DS3_SM_THS 0x13
#define LSM6DS3_STEP_COUNT_DELTA 0x15
#define LSM6DS3_MAG_SI_XX 0x24
#define LSM6DS3_MAG_SI_XY 0x25
#define LSM6DS3_MAG_SI_XZ 0x26
#define LSM6DS3_MAG_SI_YX 0x27
#define LSM6DS3_MAG_SI_YY 0x28
#define LSM6DS3_MAG_SI_YZ 0x29
#define LSM6DS3_MAG_SI_ZX 0x2a
#define LSM6DS3_MAG_SI_ZY 0x2b
#define LSM6DS3_MAG_SI_ZZ 0x2c

#define LSM6DS3_MAG_OFFX_L 0x2D
#define LSM6DS3_MAG_OFFX_H 0x2E
#define LSM6DS3_MAG_OFFY_L 0x2F
#define LSM6DS3_MAG_OFFY_H 0x30
#define LSM6DS3_MAG_OFFZ_L 0x31
#define LSM6DS3_MAG_OFFZ_H 0x32




/*LSM6DS3 Register Bit definitions*/
#define LSM6DS3_ACC_RANGE_MASK 0x0C
#define LSM6DS3_ACC_RANGE_2g		    0x00
#define LSM6DS3_ACC_RANGE_4g		    0x08
#define LSM6DS3_ACC_RANGE_8g		    0x0C
#define LSM6DS3_ACC_RANGE_16g		    0x04

#define LSM6DS3_ACC_ODR_MASK	0xF0
#define LSM6DS3_ACC_ODR_POWER_DOWN		0x00
#define LSM6DS3_ACC_ODR_13HZ		    0x10
#define LSM6DS3_ACC_ODR_26HZ		    0x20
#define LSM6DS3_ACC_ODR_52HZ		    0x30
#define LSM6DS3_ACC_ODR_104HZ		    0x40
#define LSM6DS3_ACC_ODR_208HZ		    0x50
#define LSM6DS3_ACC_ODR_416HZ		    0x60
#define LSM6DS3_ACC_ODR_833HZ		    0x70
#define LSM6DS3_ACC_ODR_1660HZ		    0x80
#define LSM6DS3_ACC_ODR_3330HZ		    0x90
#define LSM6DS3_ACC_ODR_6660HZ		    0xA0

#define LSM6DS3_GYRO_RANGE_MASK	0x0E
#define LSM6DS3_GYRO_RANGE_125DPS	0x02
#define LSM6DS3_GYRO_RANGE_245DPS	0x00
#define LSM6DS3_GYRO_RANGE_500DPS	0x04
#define LSM6DS3_GYRO_RANGE_1000DPS	0x08
#define LSM6DS3_GYRO_RANGE_2000DPS	0x0c

#define LSM6DS3_GYRO_ODR_MASK	0xf0
#define LSM6DS3_GYRO_ODR_POWER_DOWN	0x00
#define LSM6DS3_GYRO_ODR_13HZ		0x10
#define LSM6DS3_GYRO_ODR_26HZ		0x20
#define LSM6DS3_GYRO_ODR_52HZ		0x30
#define LSM6DS3_GYRO_ODR_104HZ		0x40
#define LSM6DS3_GYRO_ODR_208HZ		0x50
#define LSM6DS3_GYRO_ODR_416HZ		0x60
#define LSM6DS3_GYRO_ODR_833HZ		0x70
#define LSM6DS3_GYRO_ODR_1660HZ		0x80

#define AUTO_INCREMENT 0x80
#define LSM6DS3_CTRL3_C_IFINC 0x04


#define LSM6DS3_ACC_SENSITIVITY_2G		61	/*ug/LSB*/
#define LSM6DS3_ACC_SENSITIVITY_4G		122	/*ug/LSB*/
#define LSM6DS3_ACC_SENSITIVITY_8G		244	/*ug/LSB*/
#define LSM6DS3_ACC_SENSITIVITY_16G		488	/*ug/LSB*/


#define LSM6DS3_ACC_ENABLE_AXIS_MASK 0X38
#define LSM6DS3_ACC_ENABLE_AXIS_X 0x08
#define LSM6DS3_ACC_ENABLE_AXIS_Y 0x10
#define LSM6DS3_ACC_ENABLE_AXIS_Z 0x20



#define LSM6DS3_GYRO_SENSITIVITY_125DPS		4375	/*udps/LSB*/
#define LSM6DS3_GYRO_SENSITIVITY_245DPS		8750	/*udps/LSB*/
#define LSM6DS3_GYRO_SENSITIVITY_500DPS		17500	/*udps/LSB*/
#define LSM6DS3_GYRO_SENSITIVITY_1000DPS	35000	/*udps/LSB*/
/*lenovo-sw caoyi1 modify fix gyro data vilation 20150608 begin*/
//#define LSM6DS3_GYRO_SENSITIVITY_2000DPS	70000	/*udps/LSB*/
#define LSM6DS3_GYRO_SENSITIVITY_2000DPS	70	/*mdps/LSB*/
/*lenovo-sw caoyi1 modify fix gyro data vilation 20150608 end*/

#define LSM6DS3_GYRO_ENABLE_AXIS_MASK 0x38
#define LSM6DS3_GYRO_ENABLE_AXIS_X 0x08
#define LSM6DS3_GYRO_ENABLE_AXIS_Y 0x10
#define LSM6DS3_GYRO_ENABLE_AXIS_Z 0x20

#define LSM6DS3_ACC_GYRO_FUNC_EN_MASK  	0x04

#define LSM6DS3_TIMER_EN 0x80
typedef enum {
  	LSM6DS3_TIMER_EN_DISABLED 		        = 0x00,
  	LSM6DS3_TIMER_EN_ENABLED 		        = 0x80,
} LSM6DS3_TIMER_EN_t;

#define LSM6DS3_PEDO_EN_MASK 0x40
typedef enum {
  	LSM6DS3_ACC_GYRO_PEDO_EN_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_PEDO_EN_ENABLED 		 =0x40,
} LSM6DS3_ACC_GYRO_PEDO_EN_t;

#define LSM6DS3_TILT_EN_MASK 0x20

typedef enum {
  	LSM6DS3_ACC_GYRO_TILT_EN_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_TILT_EN_ENABLED 		 =0x20,
} LSM6DS3_ACC_GYRO_TILT_EN_t;

typedef enum {
  	LSM6DS3_ACC_GYRO_INT1 		 =0,
  	LSM6DS3_ACC_GYRO_INT2 		 =1,
} LSM6DS3_ACC_GYRO_ROUNT_INT_t;
typedef enum {
  	LSM6DS3_ACC_GYRO_FUNC_EN_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED 		 =0x04,
} LSM6DS3_ACC_GYRO_FUNC_EN_t;

typedef enum {
  	LSM6DS3_ACC_GYRO_INT_TILT_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_INT_TILT_ENABLED 		 =0x02,
} LSM6DS3_ACC_GYRO_INT2_TILT_t;

#define  	LSM6DS3_ACC_GYRO_INT_TILT_MASK  	0x02
typedef enum {
  	LSM6DS3_ACC_GYRO_TILT_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_TILT_ENABLED 		 =0x02,
} LSM6DS3_ACC_GYRO_TILT_t;

#define LSM6DS3_ACC_GYRO_TILT_MASK  	0x02

typedef enum {
  	LSM6DS3_ACC_GYRO_INT_SIGN_MOT_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_INT_SIGN_MOT_ENABLED 		 =0x40,
} LSM6DS3_ACC_GYRO_INT_SIGN_MOT_t;

#define  	LSM6DS3_ACC_GYRO_INT_SIGN_MOT_MASK  	0x40

typedef enum {
  	LSM6DS3_ACC_GYRO_SIGN_MOT_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_SIGN_MOT_ENABLED 		 =0x01,
} LSM6DS3_ACC_GYRO_SIGN_MOT_t;

#define  	LSM6DS3_ACC_GYRO_SIGN_MOT_MASK  	0x01

typedef enum {
  	LSM6DS3_ACC_GYRO_RAM_PAGE_DISABLED 		 =0x00,
  	LSM6DS3_ACC_GYRO_RAM_PAGE_ENABLED 		 =0x80,
} LSM6DS3_ACC_GYRO_RAM_PAGE_t;

#define LSM6DS3_RAM_PAGE_MASK  	0x80
#define LSM6DS3_CONFIG_PEDO_THS_MIN          0x0F

typedef enum {
  	LSM6DS3_ACC_GYRO_PEDO_RST_STEP_DISABLED 	 =0x00,
  	LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED 		 =0x02,
} LSM6DS3_ACC_GYRO_PEDO_RST_STEP_t;

#define LSM6DS3_PEDO_RST_STEP_MASK  0x02

typedef enum {
  	LSM6DS3_ACC_GYRO_INT_ACTIVE_HIGH 	 =0x00,
  	LSM6DS3_ACC_GYRO_INT_ACTIVE_LOW		 =0x20,
} LSM6DS3_ACC_GYRO_INT_ACTIVE_t;

#define LSM6DS3_ACC_GYRO_INT_ACTIVE_MASK 0x20

typedef enum {
  	LSM6DS3_ACC_GYRO_INT_LATCH 	  =0x01,
  	LSM6DS3_ACC_GYRO_INT_NO_LATCH	 =0x00,
} LSM6DS3_ACC_GYRO_INT_LATCH_CTL_t;

#define LSM6DS3_ACC_GYRO_INT_LATCH_CTL_MASK 0x01

typedef enum {
  	LSM6DS3_ACC_GYRO_SLOPE_FDS_DISABLE_HF_ENABLE 	  =0x10,
  	LSM6DS3_ACC_GYRO_SLOPE_FDS_ENABLE_HF_DISABLE	  =0x00,
} LSM6DS3_ACC_GYRO_SLOPE_FDS_t;

#define LSM6DS3_ACC_GYRO_SLOPE_FDS_MASK 0x10

typedef enum {
  	LSM6DS3_ACC_GYRO_MD1_WU_CFG_ENABLE 	  =0x20,
  	LSM6DS3_ACC_GYRO_MD1_WU_CFG_DISABLE	  =0x00,
} LSM6DS3_ACC_GYRO_MD1_WU_CFG_t;

#define LSM6DS3_ACC_GYRO_MD1_WU_CFG 0x20

#define LSM6DS3_ACC_WU_TH_MASK 0x3F
#define LSM6DS3_ACC_WU_DUR_MASK 0x60

#define LSM6DS3_SIGNICANT_MOTION_INT_WU_STATUS 0x0F


/* interrupt src defines of FUNC_SRC reg */
#define LSM6DS3_STEP_COUNT_DELTA_IA 0x80
#define LSM6DS3_SIGNICANT_MOTION_INT_STATUS 0x40
#define LSM6DS3_TILT_INT_STATUS 0x20
#define LSM6DS3_STEP_DETECT_INT_STATUS 0x10
#define LSM6DS3_STEP_OVERFLOW 0x08
/* interrupt src defines of FIFO_STATUS2 reg */
#define LSM6DS3_FIFO_DATA_AVL 0x80
#define LSM6DS3_FIFO_OVER_RUN 0x40
#define LSM6DS3_FIFO_FULL 0x20
#define LSM6DS3_FIFO_EMPTY 0x10



#define LSM6DS3_SUCCESS		       0
#define LSM6DS3_ERR_I2C		      -1
#define LSM6DS3_ERR_STATUS			  -3
#define LSM6DS3_ERR_SETUP_FAILURE	  -4
#define LSM6DS3_ERR_GETGSENSORDATA  -5
#define LSM6DS3_ERR_IDENTIFICATION	  -6

#define LSM6DS3_BUFSIZE 60

/*------------------------------------------------------------------*/

// 1 rad = 180/PI degree, L3G4200D_OUT_MAGNIFY = 131,
// 180*131/PI = 7506
#define DEGREE_TO_RAD	180*1000000/PI//7506  // fenggy mask
//#define DEGREE_TO_RAD 819	 
#endif //L3M6DS3_H


/*----------------------------------------------------------------------------*/
typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
    ADX_TRC_CALI	= 0X08,
    ADX_TRC_INFO	= 0X10,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    ACCEL_TRC_FILTER  = 0x01,
    ACCEL_TRC_RAWDATA = 0x02,
    ACCEL_TRC_IOCTL   = 0x04,
    ACCEL_TRC_CALI	= 0X08,
    ACCEL_TRC_INFO	= 0X10,
    ACCEL_TRC_DATA	= 0X20,
} ACCEL_TRC;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* fifo & interrupt func */
#define LSM6DS3_EN_BIT				0x01
#define LSM6DS3_DIS_BIT				0x00
#define LSM6DS3_FIFO_MODE_ADDR			0x0a
#define LSM6DS3_FIFO_MODE_MASK			0x07
#define LSM6DS3_FIFO_MODE_BYPASS		0x00
#define LSM6DS3_FIFO_MODE_CONTINUOS		0x06
#define LSM6DS3_FIFO_THRESHOLD_IRQ_MASK	0x08
#define LSM6DS3_FIFO_ODR_ADDR			0x0a
#define LSM6DS3_FIFO_ODR_MASK			0x78
#define LSM6DS3_FIFO_ODR_MAX			0x07
#define LSM6DS3_FIFO_ODR_MAX_HZ			800
#define LSM6DS3_FIFO_ODR_OFF			0x00
#define LSM6DS3_FIFO_CTRL3_ADDR			0x08
#define LSM6DS3_FIFO_ACCEL_DECIMATOR_MASK	0x07
#define LSM6DS3_FIFO_GYRO_DECIMATOR_MASK	0x38
#define LSM6DS3_FIFO_CTRL4_ADDR			0x09
#define LSM6DS3_FIFO_STEP_C_DECIMATOR_MASK	0x38
#define LSM6DS3_FIFO_THR_L_ADDR			0x06
#define LSM6DS3_FIFO_THR_H_ADDR			0x07
#define LSM6DS3_FIFO_THR_H_MASK			0x0f
#define LSM6DS3_FIFO_THR_IRQ_MASK		0x08
#define LSM6DS3_FIFO_PEDO_E_ADDR		0x07
#define LSM6DS3_FIFO_PEDO_E_MASK		0xC0
#define LSM6DS3_FIFO_STEP_C_FREQ		25

#define LSM6DS3_FIFO_DIFF_L			0x3a
#define LSM6DS3_FIFO_DIFF_MASK			0x0fff
#define LSM6DS3_FIFO_ELEMENT_LEN_BYTE		6
#define LSM6DS3_FIFO_BYTE_FOR_CHANNEL		2
#define LSM6DS3_FIFO_DATA_OVR_2REGS		0x4000
#define LSM6DS3_FIFO_DATA_OVR			0x40
#define LSM6DS3_MAX_FIFO_SIZE			(8 * 1024)
#define LSM6DS3_MAX_FIFO_LENGHT			(LSM6DS3_MAX_FIFO_SIZE / \
						LSM6DS3_FIFO_ELEMENT_LEN_BYTE)
#ifndef MAX
#define MAX(a, b)				(((a) > (b)) ? (a) : (b))
#endif
	
#ifndef MIN
#define MIN(a, b)				(((a) < (b)) ? (a) : (b))
#endif


enum {
	LSM6DS3_ACCEL_IDX = 0,
	LSM6DS3_GYRO_IDX,
	LSM6DS3_SIGN_MOTION_IDX,
	LSM6DS3_STEP_COUNTER_IDX,
	LSM6DS3_STEP_DETECTOR_IDX,
	LSM6DS3_TILT_IDX,
	LSM6DS3_SENSORS_NUMB,
};


enum fifo_mode {
	BYPASS = 0,
	CONTINUOS,
};

struct lsm6ds3_sensor_data {
	const char* name;
	s64 timestamp;
	u8 enabled;
	u32 c_odr;
	u32 c_gain;
	u8 sindex;
	u8 sample_to_discard;

	u16 fifo_length;
	u8 sample_in_pattern;
	s64 deltatime;

};

struct lsm6ds3_i2c_data;

struct lsm6ds3_transfer_function {
	int (*write) (struct lsm6ds3_i2c_data *cdata, u8 reg_addr, int len, u8 *data, bool b_lock);
	int (*read) (struct lsm6ds3_i2c_data *cdata, u8 reg_addr, int len, u8 *data, bool b_lock);
};

#define LSM6DS3_AXIS_X          0
#define LSM6DS3_AXIS_Y          1
#define LSM6DS3_AXIS_Z          2
#define LSM6DS3_ACC_AXES_NUM        3
#define LSM6DS3_GYRO_AXES_NUM       3

struct lsm6ds3_i2c_data {
    struct i2c_client *client;
	struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    atomic_t 				layout;
    /*misc*/
    struct work_struct	eint_work;				
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
    atomic_t				filter;
/*lenovo-sw caoyi modify for eint API 20151021 begin*/
    struct device_node *irq_node;
    int		irq;
    struct device_node *irq_node2;
    int		irq2;
/*lenovo-sw caoyi modify for eint API 20151021 end*/
    s16                     cali_sw[LSM6DS3_GYRO_AXES_NUM+1];

    /*data*/
	s8                      offset[LSM6DS3_ACC_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[LSM6DS3_ACC_AXES_NUM+1];
	
	int 					sensitivity;
	int 					sample_rate;

/*lenovo-sw caoyi1 add for selftest function 20150527 begin*/
    u8 resume_LSM6DS3_CTRL1_XL;
    u8 resume_LSM6DS3_CTRL2_G;
    u8 resume_LSM6DS3_CTRL3_C;
    u8 resume_LSM6DS3_CTRL4_C;
    u8 resume_LSM6DS3_CTRL5_C;
    u8 resume_LSM6DS3_CTRL6_C;
    u8 resume_LSM6DS3_CTRL7_G;
    u8 resume_LSM6DS3_CTRL8_XL;
    u8 resume_LSM6DS3_CTRL9_XL;
    u8 resume_LSM6DS3_CTRL10_C;
/*lenovo-sw caoyi1 add for selftest function 20150527 end*/

/* step counter func support  --add by liaoxl.lenovo 7.28.2015 start */
	struct lsm6ds3_sensor_data sensors[LSM6DS3_SENSORS_NUMB];
//	struct mutex bank_registers_lock;
//	struct mutex fifo_lock;
	u16 fifo_data_size;
	u8 *fifo_data_buffer;
	u16 fifo_threshold;
	/* for 16bit size limit of step count register */
	u16 steps_c;
	u8  step_c_delay;
	/* for interrupt usage */
	atomic_t    int1_request_num;
	atomic_t    int2_request_num;
	/* step counter overflow interrupt triggered numbers */
	volatile u32 overflow;
	volatile u32 bounce_steps;
	volatile int bounce_count;
	volatile u16 last_bounce_step;
	volatile int boot_deb;
	atomic_t             reset_sc;

	const struct lsm6ds3_transfer_function *tf;
/* step counter func support  --add by liaoxl.lenovo 7.28.2015  end */

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};



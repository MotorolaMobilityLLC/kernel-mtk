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

/* LSM6DSMSTR_ACC motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */

#ifndef LSM6DSMSTR_ACCGYRO_H
#define LSM6DSMSTR_ACCGYRO_H

#include <linux/ioctl.h>

#define LSM6DSMSTR_ACCELGYRO_DEV_NAME		"lsm6dsmtr_accelgyro"
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               pr_debug(GSE_TAG"%s\n", __func__)
#define GSE_LOG(fmt, args...)    pr_debug(GSE_TAG fmt, ##args)

#define ACC_TAG						"<ACCELEROMETER> "
#define ACC_PR_ERR(fmt, args...)		pr_err(ACC_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define ACC_LOG(fmt, args...)		pr_debug(ACC_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define ACC_VER(fmt, args...)		pr_debug(ACC_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define ACC_FUN(f)               pr_debug(ACC_TAG"%s\n", __func__)

#define LSM6DSMSTR_GET_BITSLICE(regvar, mask)\
			((regvar & mask) >> __ffs(mask))

#define LSM6DSMSTR_SET_BITSLICE(regvar, mask, val)\
		((regvar & ~mask) | \
		((val<<__ffs(mask))&mask))


#define LSM6DSMSTR_BUS_WRITE_FUNC(dev_addr, reg_addr, reg_data, wr_len)\
				bus_write(dev_addr, reg_addr, reg_data, wr_len)
#define LSM6DSMSTR_BUS_READ_FUNC(dev_addr, reg_addr, reg_data, r_len)\
				bus_read(dev_addr, reg_addr, reg_data, r_len)
#define LSM6DSMSTR_WR_FUNC_PTR s8 (*bus_write)(u8, u8,\
u8 *, u8)
#define LSM6DSMSTR_RD_FUNC_PTR s8 (*bus_read)(u8,\
				u8, u8 *, u8)
#define LSM6DSMSTR_BRD_FUNC_PTR s8 \
	(*burst_read)(u8, u8, u8 *, u32)

#define BMI_INT_LEVEL	0
#define BMI_INT_EDGE        1


#define WORK_DELAY_DEFAULT	(200)
#define ABSMIN				(-32768)
#define ABSMAX				(32767)
/* LSM6DSM start*/

/*
*  register mask bit
*/
#define LSM6DSM_REG_FUNC_CFG_ACCESS_MASK_FUNC_CFG_EN        0x80
#define LSM6DSM_REG_CTRL1_XL_MASK_FS_XL			            0x0C
#define LSM6DSM_REG_CTRL1_XL_MASK_ODR_XL		            0xF0
#define LSM6DSM_REG_CTRL2_G_MASK_FS_G			            0x0C
#define LSM6DSM_REG_CTRL2_G_MASK_ODR_G			            0xF0
#define LSM6DSM_REG_CTRL2_G_MASK_ODR_G			            0xF0
#define LSM6DSM_REG_CTRL10_C_MASK_FUNC_EN                   0x04
#define LSM6DSM_REG_CTRL10_C_MASK_PEDO_RST_STEP             0x02
#define LSM6DSM_REG_TAP_CFG_MASK_PEDO_EN                    0x40

/* LSM6DSM Register Map  (Please refer to LSM6DSM Specifications) */
#define LSM6DSM_REG_FUNC_CFG_ACCESS     0x01
#define LSM6DSM_REG_INT1_CTRL           0X0D
#define LSM6DSM_REG_INT2_CTRL           0X0E
#define LSM6DSM_REG_WHO_AM_I			0x0F
#define LSM6DSM_REG_CTRL1_XL			0x10
#define LSM6DSMSTR_USER_ACC_RANGE LSM6DSM_REG_CTRL1_XL

#define LSM6DSM_REG_CTRL2_G				0x11
#define LSM6DSM_REG_CTRL3_C				0x12
#define LSM6DSM_REG_CTRL4_C				0x13
#define LSM6DSM_REG_CTRL5_C				0x14
#define LSM6DSM_REG_CTRL6_C				0x15
#define LSM6DSM_REG_CTRL7_G				0x16
#define LSM6DSM_REG_CTRL8_XL		    0x17
#define LSM6DSM_REG_CTRL9_XL		    0x18
#define LSM6DSM_REG_CTRL10_C			0x19

#define LSM6DSM_REG_OUTX_L_G			0x22
#define LSM6DSM_REG_OUTX_H_G			0x23
#define LSM6DSM_REG_OUTY_L_G			0x24
#define LSM6DSM_REG_OUTY_H_G			0x25
#define LSM6DSM_REG_OUTZ_L_G			0x26
#define LSM6DSM_REG_OUTZ_H_G			0x27

#define LSM6DSM_REG_OUTX_L_XL			0x28
#define LSM6DSM_REG_OUTX_H_XL			0x29
#define LSM6DSM_REG_OUTY_L_XL			0x2A
#define LSM6DSM_REG_OUTY_H_XL			0x2B
#define LSM6DSM_REG_OUTZ_L_XL			0x2C
#define LSM6DSM_REG_OUTZ_H_XL			0x2D
#define LSM6DSM_REG_STEP_COUNTER_L      0x4B
#define LSM6DSM_REG_STEP_COUNTER_H      0x4C
#define LSM6DSM_REG_FUNC_SRC            0x53
#define LSM6DSM_REG_TAP_CFG             0x58
#define LSM6DSM_REG_MD1_CFG             0x5E

/*
*  register value
*/
#define LSM6DS3_FIXED_DEVID			    0x69
#define LSM6DSL_FIXED_DEVID			    0x6A
#define LSM6DSM_FIXED_DEVID			    0x6A

#define LSM6DSM_REG_FUNC_CFG_ACCESS_ENABLE                  0x80
#define LSM6DSM_REG_FUNC_CFG_ACCESS_DISABLE                 0x00

#define LSM6DSM_REG_CTRL1_XL_ODR_208HZ			0x05
#define LSM6DSM_REG_CTRL1_XL_ODR_104HZ			0x04
#define LSM6DSM_REG_CTRL1_XL_ODR_52HZ			0x03
#define LSM6DSM_REG_CTRL1_XL_ODR_26HZ			0x02
#define LSM6DSM_REG_CTRL1_XL_ODR_13HZ			0x01
#define LSM6DSM_REG_CTRL1_XL_ODR_0HZ			0x00

#define LSM6DSM_REG_CTRL1_XL_FS_2G			                0x00
#define LSM6DSM_REG_CTRL1_XL_FS_4G			                0x02
#define LSM6DSM_REG_CTRL1_XL_FS_8G			                0x03
#define LSM6DSM_REG_CTRL1_XL_FS_16G			                0x01


#define LSM6DSM_REG_CTRL2_G_ODR_208HZ			            0x05
#define LSM6DSM_REG_CTRL2_G_ODR_104HZ			            0x04
#define LSM6DSM_REG_CTRL2_G_ODR_52HZ			            0x03
#define LSM6DSM_REG_CTRL2_G_ODR_26HZ			            0x03
#define LSM6DSM_REG_CTRL2_G_ODR_0HZ				            0x00

#define LSM6DSM_REG_CTRL2_G_FS_245DPS			            0x00
#define LSM6DSM_REG_CTRL2_G_FS_500DPS			            0x01
#define LSM6DSM_REG_CTRL2_G_FS_1000DPS			            0x02
#define LSM6DSM_REG_CTRL2_G_FS_2000DPS			            0x03

#define LSM6DSM_REG_CTRL10_C_FUNC_ENABLE                    0x04
#define LSM6DSM_REG_CTRL10_C_FUNC_DISABLE                   0x00
#define LSM6DSM_REG_CTRL10_C_FUNC_PEDO_RST_STEP             0x02

#define LSM6DSM_REG_TAP_CFG_PEDO_ENABLE                     0x40
#define LSM6DSM_REG_TAP_CFG_PEDO_DISABLE                    0x00

/*
*  return value
*/
#define LSM6DSM_SUCCESS					    0
#define LSM6DSM_ERR_I2C					    -1
#define LSM6DSM_ERR_STATUS					-3
#define LSM6DSM_ERR_SETUP_FAILURE			-4
#define LSM6DSM_ERR_GETGSENSORDATA			-5
#define LSM6DSM_ERR_IDENTIFICATION			-6

#define LSM6DSM_BUFSIZE			256

/* LSM6DSM end*/

#define LSM6DSMSTR_GEN_READ_WRITE_DELAY     (1)
#define LSM6DSMSTR_SEC_INTERFACE_GEN_READ_WRITE_DELAY    (5)

#define LSM6DSMSTR_MDELAY_DATA_TYPE		u32
#define LSM6DSMSTR_ACC_SUCCESS	0
#define LSM6DSMSTR_ACC_ERR_SPI	-1

#define BMI_DELAY_MIN		(1)

#define LSM6DSMSTR_BUFSIZE		256

#define LSM6DSMSTR_RETURN_FUNCTION_TYPE			s8
#define LSM6DSMSTR_NULL							(0)
#define E_LSM6DSMSTR_NULL_PTR					((s8)-127)
#define E_LSM6DSMSTR_COMM_RES					((s8)-1)
#define E_LSM6DSMSTR_OUT_OF_RANGE				((s8)-2)
#define E_LSM6DSMSTR_BUSY						((s8)-3)
#define	SUCCESS								((u8)0)
#define	ERROR								((s8)-1)
#define LSM6DSMSTR_INIT_VALUE					(0)
#define LSM6DSMSTR_GEN_READ_WRITE_DATA_LENGTH	(1)

#define C_MAX_FIR_LENGTH			(32)
#define REG_MAX0					0x24
#define REG_MAX1					0x56
#define LSM6DSMSTR_ACC_AXIS_X		0
#define LSM6DSMSTR_ACC_AXIS_Y		1
#define LSM6DSMSTR_ACC_AXIS_Z		2
#define LSM6DSMSTR_ACC_AXES_NUM		3
#define LSM6DSMSTR_ACC_DATA_LEN		6

#define LSM6DSMSTR_ACC_MODE_NORMAL      0
#define LSM6DSMSTR_ACC_MODE_LOWPOWER    1
#define LSM6DSMSTR_ACC_MODE_SUSPEND     2
#define LSM6DSMSTR_SHIFT_8_POSITION     8
#define LSM6DSMSTR_FS_125_LSB               2624
#define LSM6DSMSTR_FS_250_LSB               1312
#define LSM6DSMSTR_FS_500_LSB               656
#define LSM6DSMSTR_FS_1000_LSB              328
#define LSM6DSMSTR_FS_2000_LSB              164

#define LSM6DSMSTR_GYRO_ODR_RESERVED	0x00

struct lsm6dsmtr_t {
	u8 chip_id;
	   /**< chip id of LSM6DSMSTR */
	u8 dev_addr;
	    /**< device address of LSM6DSMSTR */
	s8 mag_manual_enable;
		     /**< used for check the mag manual/auto mode status */
	 LSM6DSMSTR_WR_FUNC_PTR;
		   /**< bus write function pointer */
	 LSM6DSMSTR_RD_FUNC_PTR;
		   /**< bus read function pointer */
	 LSM6DSMSTR_BRD_FUNC_PTR;
		    /**< burst write function pointer */
	void (*delay_msec)(LSM6DSMSTR_MDELAY_DATA_TYPE);
					    /**< delay function pointer */
};

struct odr_t {
	u8 acc_odr;
	u8 gyro_odr;
};

struct lsm6dsmtracc_t {
	union {
		int16_t v[3];
		struct {
			int16_t x;
			int16_t y;
			int16_t z;
		};
	};
};

struct lsm6dsmtrgyro_t {
	union {
		int16_t v[3];
		struct {
			int16_t x;
			int16_t y;
			int16_t z;
		};
	};
};

/*! bmi sensor support type*/
enum BMI_SENSOR_TYPE {
	BMI_ACC_SENSOR,
	BMI_GYRO_SENSOR,
	BMI_MAG_SENSOR,
	BMI_SENSOR_TYPE_MAX
};

/* gyro */
#define BMG_DRIVER_VERSION "V1.0"
#define BMG_AXIS_X				0
#define BMG_AXIS_Y				1
#define BMG_AXIS_Z				2
#define BMG_AXES_NUM			3
#define BMG_DATA_LEN			6
#define BMG_BUFSIZE				128

#define MAX_SENSOR_NAME			(32)

#define BMG_GET_BITSLICE(regvar, mask)\
	((regvar & mask) >> mask)
#define BMG_SET_BITSLICE(regvar, mask, val)\
			((regvar & ~mask) | \
			((val<<__ffs(mask))&mask))

#define DEGREE_TO_RAD				7506
#define SW_CALIBRATION
#define BMG_DEV_NAME				"lsm6dsmtr_gyro"
#define UNKNOWN_DEV					"unknown sensor"
#define LSM6DSMSTR_GYRO_I2C_ADDRESS		0x66

enum BMI_GYRO_PM_TYPE {
	BMI_GYRO_PM_NORMAL = 0,
	BMI_GYRO_PM_FAST_START,
	BMI_GYRO_PM_SUSPEND,
	BMI_GYRO_PM_MAX
};

#ifdef CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR
extern int lsm6dsmtr_step_notify(enum STEP_NOTIFY_TYPE);

extern int BMP160_spi_read_bytes(struct spi_device *spi, u16 addr, u8 *rx_buf, u32 data_len);
extern int BMP160_spi_write_bytes(struct spi_device *spi, u16 addr, u8 *tx_buf, u32 data_len);

int lsm6dsm_write_with_mask(struct lsm6dsm_data *cdata, u8 reg_addr, u8 mask, u8 data);

#endif
#endif

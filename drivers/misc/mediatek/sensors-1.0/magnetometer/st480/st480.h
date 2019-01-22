/*****************************************************************************
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
 *
 * Magnetometer Sensor Driver
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
 *
 *****************************************************************************/

/*
 * Definitions for senodia compass chip.
 */
#ifndef MAGNETIC_H
#define MAGNETIC_H

#include <linux/ioctl.h>	/* For IOCTL macros */
/*
 * Self test
 */
#define SENSOR_AUTO_TEST 0

#if SENSOR_AUTO_TEST
#include <linux/kthread.h>
#endif

/*
 * I2C name
 */
#define ST480_I2C_NAME "st480"

/*
 * register shift
 */
#define ST480_REG_DRR_SHIFT 2
#define ST480_DEVICE_ID 0x7c

/* conversion of magnetic data (for AK8963) to uT units */
/* #define CONVERT_M                   (1.0f*0.06f) */
/* conversion of orientation data to degree units */
/* #define CONVERT_O                   (1.0f/64.0f) */

#define CONVERT_M			1
#define CONVERT_M_DIV		1	/* 6/100 = CONVERT_M */
#define CONVERT_O			1
#define CONVERT_O_DIV		1	/* 1/64 = CONVERT_O */

/*
 * register
 */
#define SINGLE_MEASUREMENT_MODE_CMD 0x3E
#define READ_MEASUREMENT_CMD 0x4E
#define WRITE_REGISTER_CMD 0x60
#define READ_REGISTER_CMD 0x50
#define EXIT_REGISTER_CMD 0x80
#define MEMORY_RECALL_CMD 0xD0
#define MEMORY_STORE_CMD 0xE0
#define RESET_CMD 0xF0

#define CALIBRATION_REG (0x02 << ST480_REG_DRR_SHIFT)
#define CALIBRATION_DATA_LOW 0x1C
#define CALIBRATION_DATA_HIGH 0x00

#define ONE_INIT_DATA_LOW 0x7C
#define ONE_INIT_DATA_HIGH 0x00
#define ONE_INIT_REG (0x00 << ST480_REG_DRR_SHIFT)

#define TWO_INIT_DATA_LOW 0x00
#define TWO_INIT_DATA_HIGH 0x00
#define TWO_INIT_REG (0x02 << ST480_REG_DRR_SHIFT)

/*
 * Miscellaneous set.
 */
#define SENSOR_DATA_SIZE 6
#define MAX_FAILURE_COUNT 3
#define ST480_DEFAULT_DELAY   27


/*
 *  IC select
 */
#define SENSOR_SIZE_3X3_QFN	0
#define SENSOR_SIZE_2X2_BGA	0
#define SENSOR_SIZE_1_6X1_6_LGA	0
#define SENSOR_SIZE_1_6X1_6_BGA	0
#define ST480_SIZE_1_2X1_2 1

/*
 *  IC position
 */
#define CONFIG_ST480_BOARD_LOCATION_FRONT
#define CONFIG_ST480_BOARD_LOCATION_FRONT_DEGREE_90



extern struct mag_hw *st480_get_cust_mag_hw(void);


struct SensorData {
	rwlock_t datalock;
	rwlock_t ctrllock;
	int controldata[10];
	unsigned int debug;
	int yaw;
	int roll;
	int pitch;
	int nmx;
	int nmy;
	int nmz;
	int mag_status;
};
#endif

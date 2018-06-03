/* BMA2XX motion sensor driver
 *
 * (C) Copyright 2008
 * MediaTek <www.mediatek.com>
 *
 * BMA2XX driver for MT6516
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
 */
#ifndef BMA2XX_H
#define BMA2XX_H

#include <linux/ioctl.h>

/*
************************************************************
| sensor | chip id | bit number |     7-bit i2c address      |
-------------------------------------------------------------|
| bma220 |  0xDD   |     6      |0x0B(CSB:low),0x0A(CSB:high)|
-------------------------------------------------------------|
| bma222 |  0x03   |     8      |0x08(SDO:low),0x09(SDO:high)|
-------------------------------------------------------------|
| bma222e|  0xF8   |     8      |0x18(SDO:low),0x19(SDO:high)|
-------------------------------------------------------------|
| bma250 |  0x03   |     10     |0x18(SDO:low),0x19(SDO:high)|
-------------------------------------------------------------|
| bma250e|  0xF9   |     10     |0x18(SDO:low),0x19(SDO:high)|
-------------------------------------------------------------|
| bma255 |  0xFA   |     12     |0x18(SDO:low),0x19(SDO:high)|
-------------------------------------------------------------|
| bma280 |  0xFB   |     14     |0x18(SDO:low),0x19(SDO:high)|
*************************************************************/

	#define BMA2XX_I2C_SLAVE_WRITE_ADDR		0x10

	 /* BMA2XX Register Map  (Please refer to BMA2XX Specifications) */
	#define BMA2XX_REG_DEVID				0x00
	#define BMA2XX_FIXED_DEVID			0xFA
	#define BMA2XX_REG_OFSX				0x16
	#define BMA2XX_REG_OFSX_HIGH			0x1A
	#define BMA2XX_REG_BW_RATE			0x10
	#define BMA2XX_BW_MASK				0x1f
	#define BMA2XX_BW_200HZ				0x0d
	#define BMA2XX_BW_100HZ				0x0c
	#define BMA2XX_BW_50HZ				0x0b
	#define BMA2XX_BW_25HZ				0x0a
	#define BMA2XX_REG_POWER_CTL		0x11
	#define BMA2XX_REG_DATA_FORMAT		0x0f
	#define BMA2XX_RANGE_MASK			0x0f
	#define BMA2XX_RANGE_2G				0x03
	#define BMA2XX_RANGE_4G				0x05
	#define BMA2XX_RANGE_8G				0x08
	#define BMA2XX_REG_DATAXLOW			0x02
	#define BMA2XX_REG_DATA_RESOLUTION	0x14
	#define BMA2XX_MEASURE_MODE			0x80
	#define BMA2XX_SELF_TEST				0x32
	#define BMA2XX_SELF_TEST_AXIS_X		0x01
	#define BMA2XX_SELF_TEST_AXIS_Y		0x02
	#define BMA2XX_SELF_TEST_AXIS_Z		0x03
	#define BMA2XX_SELF_TEST_POSITIVE	0x00
	#define BMA2XX_SELF_TEST_NEGATIVE	0x04
	#define BMA2XX_INT_REG_1				0x16
	#define BMA2XX_INT_REG_2				0x17


#define BMA2XX_SUCCESS						0
#define BMA2XX_ERR_I2C						-1
#define BMA2XX_ERR_STATUS					-3
#define BMA2XX_ERR_SETUP_FAILURE			-4
#define BMA2XX_ERR_GETGSENSORDATA			-5
#define BMA2XX_ERR_IDENTIFICATION			-6



#define BMA2XX_BUFSIZE				256

#define BMA2XX_AXES_NUM        3

/*----------------------------------------------------------------------------*/
enum CUST_ACTION {
	BMA2XX_CUST_ACTION_SET_CUST = 1,
	BMA2XX_CUST_ACTION_SET_CALI,
	BMA2XX_CUST_ACTION_RESET_CALI
};
/*----------------------------------------------------------------------------*/
struct BMA2XX_CUST {
	uint16_t    action;
};
/*----------------------------------------------------------------------------*/
struct  BMA2XX_SET_CUST {
	uint16_t    action;
	uint16_t    part;
	int32_t     data[0];
};
/*----------------------------------------------------------------------------*/
struct BMA2XX_SET_CALI {
	uint16_t    action;
	int32_t     data[BMA2XX_AXES_NUM];
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
union BMA2XX_CUST_DATA {
	uint32_t                data[10];
	struct BMA2XX_CUST         cust;
	struct BMA2XX_SET_CUST     setCust;
	struct BMA2XX_SET_CALI     setCali;
	struct BMA2XX_CUST   resetCali;
};
/*----------------------------------------------------------------------------*/

#define BMA255_CHIP_ID		0XFA
#define BMA250E_CHIP_ID		0XF9
#define BMA222E_CHIP_ID		0XF8
#define BMA280_CHIP_ID		0XFB

#define BMA222E_TYPE		0
#define BMA250E_TYPE		1
#define BMA255_TYPE		2
#define BMA280_TYPE		3
#define UNSUPPORT_TYPE		-1

struct bma2x2_type_map_t {
	/*! bma2x2 sensor chip id */
	uint16_t chip_id;

	/*! bma2x2 sensor type */
	int16_t sensor_type;

	/*! bma2x2 sensor name */
	const char *sensor_name;
};
#endif


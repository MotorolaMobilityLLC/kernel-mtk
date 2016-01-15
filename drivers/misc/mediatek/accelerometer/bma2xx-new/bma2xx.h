/* BOSCH Accelerometer Sensor Driver Header File
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

#ifndef BMA2XX_H
#define BMA2XX_H

#include <linux/ioctl.h>
/*************************************************************
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
/*
 * configuration
*/
#define BMA_DRIVER_VERSION "V1.2"
#define BMA_I2C_ADDRESS(SENSOR_NAME, PIN_STATUS) \
	SENSOR_NAME##_I2C_ADDRESS_##PIN_STATUS

/* apply low pass filter on output */
/* #define CONFIG_BMA_LOWPASS */
#define SW_CALIBRATION

#define BMA_AXIS_X				0
#define BMA_AXIS_Y				1
#define BMA_AXIS_Z				2
#define BMA_AXES_NUM				3
#define BMA_DATA_LEN				6
#define BMA_DEV_NAME				"bma2xx"

#define C_MAX_FIR_LENGTH			(32)
#define MAX_SENSOR_NAME				(32)

/* common definition */
#define BMA_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define BMA_CHIP_ID_REG				0x00
#define BMA_BUFSIZE				128

/*********************************[BMA220]*************************************/
/* chip id */
#define BMA220_CHIP_ID				0xDD

/* i2c address */
#define BMA220_I2C_ADDRESS_PIN_HIGH		0x0A
#define BMA220_I2C_ADDRESS_PIN_LOW		0x0B

/* bandwidth */
#define BMA220_BANDWIDTH_50HZ			0x05
#define BMA220_BANDWIDTH_75HZ			0x04
#define BMA220_BANDWIDTH_150HZ			0x03
#define BMA220_BANDWIDTH_250HZ			0x02
#define BMA220_BANDWIDTH_600HZ			0x01
#define BMA220_BANDWIDTH_1000HZ			0x00

#define BMA220_BANDWIDTH_CONFIG_REG		0x20

#define BMA220_SC_FILT_CONFIG__POS		0
#define BMA220_SC_FILT_CONFIG__MSK		0x0F
#define BMA220_SC_FILT_CONFIG__LEN		4
#define BMA220_SC_FILT_CONFIG__REG		BMA220_BANDWIDTH_CONFIG_REG

/* range */
#define BMA220_RANGE_2G				0
#define BMA220_RANGE_4G				1
#define BMA220_RANGE_8G				2

#define BMA220_RANGE_SELFTEST_REG		0x22

#define BMA220_RANGE__POS			0
#define BMA220_RANGE__MSK			0x03
#define BMA220_RANGE__LEN			2
#define BMA220_RANGE__REG			BMA220_RANGE_SELFTEST_REG

/* power mode */
#define	BMA220_SUSPEND_MODE			0
#define BMA220_NORMAL_MODE			1

#define BMA220_SUSPEND_MODE_REG			0x30

#define BMA220_SUSPEND__POS			0
#define BMA220_SUSPEND__MSK			0xFF
#define BMA220_SUSPEND__LEN			8
#define BMA220_SUSPEND__REG			BMA220_SUSPEND_MODE_REG

/* data */
#define BMA220_REG_DATAXLOW			0x04

/*********************************[BMA222]*************************************/
/* chip id */
#define BMA222_BMA250_CHIP_ID			0x03

/* i2c address */
#define BMA222_I2C_ADDRESS_PIN_HIGH		0x09
#define BMA222_I2C_ADDRESS_PIN_LOW		0x08

/* bandwidth */
#define BMA222_BW_7_81HZ			0x08
#define BMA222_BW_15_63HZ			0x09
#define BMA222_BW_31_25HZ			0x0A
#define BMA222_BW_62_50HZ			0x0B
#define BMA222_BW_125HZ				0x0C
#define BMA222_BW_250HZ				0x0D
#define BMA222_BW_500HZ				0x0E
#define BMA222_BW_1000HZ			0x0F

#define BMA222_BW_SEL_REG			0x10

#define BMA222_BANDWIDTH__POS			0
#define BMA222_BANDWIDTH__LEN			5
#define BMA222_BANDWIDTH__MSK			0x1F
#define BMA222_BANDWIDTH__REG			BMA222_BW_SEL_REG

/* range */
#define BMA222_RANGE_2G				3
#define BMA222_RANGE_4G				5
#define BMA222_RANGE_8G				8

#define BMA222_RANGE_SEL_REG			0x0F

#define BMA222_RANGE_SEL__POS			0
#define BMA222_RANGE_SEL__LEN			4
#define BMA222_RANGE_SEL__MSK			0x0F
#define BMA222_RANGE_SEL__REG			BMA222_RANGE_SEL_REG

/* power mode */
#define BMA222_MODE_SUSPEND			1
#define BMA222_MODE_NORMAL			0

#define BMA222_MODE_CTRL_REG			0x11

#define BMA222_EN_LOW_POWER__POS		6
#define BMA222_EN_LOW_POWER__LEN		1
#define BMA222_EN_LOW_POWER__MSK		0x40
#define BMA222_EN_LOW_POWER__REG		BMA222_MODE_CTRL_REG

#define BMA222_EN_SUSPEND__POS			7
#define BMA222_EN_SUSPEND__LEN			1
#define BMA222_EN_SUSPEND__MSK			0x80
#define BMA222_EN_SUSPEND__REG			BMA222_MODE_CTRL_REG

/* data */
#define BMA222_REG_DATAXLOW			0x02

/*********************************[BMA222E]************************************/
/* chip id */
#define BMA222E_CHIP_ID				0xF8

/* i2c address */
#define BMA2X2_I2C_ADDRESS_PIN_HIGH		0x19//0x48//0x3e//0xbe//0xc8//0x19
#define BMA2X2_I2C_ADDRESS_PIN_LOW		0x18

#define BMA222E_I2C_ADDRESS_PIN_HIGH		0x19
#define BMA222E_I2C_ADDRESS_PIN_LOW		0x18

/* bandwidth */
#define BMA2X2_BW_7_81HZ			0x08
#define BMA2X2_BW_15_63HZ			0x09
#define BMA2X2_BW_31_25HZ			0x0A
#define BMA2X2_BW_62_50HZ			0x0B
#define BMA2X2_BW_125HZ				0x0C
#define BMA2X2_BW_250HZ				0x0D
#define BMA2X2_BW_500HZ				0x0E
#define BMA2X2_BW_1000HZ			0x0F

#define BMA2X2_BW_SEL_REG			0x10

#define BMA2X2_BANDWIDTH__POS			0
#define BMA2X2_BANDWIDTH__LEN			5
#define BMA2X2_BANDWIDTH__MSK			0x1F
#define BMA2X2_BANDWIDTH__REG			BMA2X2_BW_SEL_REG

/* range */
#define BMA2X2_RANGE_2G				3
#define BMA2X2_RANGE_4G				5
#define BMA2X2_RANGE_8G				8
#define BMA2X2_RANGE_16G			12

#define BMA2X2_RANGE_SEL_REG			0x0F

#define BMA2X2_RANGE_SEL__POS			0
#define BMA2X2_RANGE_SEL__LEN			4
#define BMA2X2_RANGE_SEL__MSK			0x0F
#define BMA2X2_RANGE_SEL__REG			BMA2X2_RANGE_SEL_REG

/* power mode */
#define BMA2X2_MODE_NORMAL			0
#define BMA2X2_MODE_SUSPEND			4

#define BMA2X2_MODE_CTRL_REG			0x11

#define BMA2X2_MODE_CTRL__POS			5
#define BMA2X2_MODE_CTRL__LEN			3
#define BMA2X2_MODE_CTRL__MSK			0xE0
#define BMA2X2_MODE_CTRL__REG			BMA2X2_MODE_CTRL_REG

#define BMA2X2_LOW_NOISE_CTRL_REG		0x12

#define BMA2X2_LOW_POWER_MODE__POS		6
#define BMA2X2_LOW_POWER_MODE__LEN		1
#define BMA2X2_LOW_POWER_MODE__MSK		0x40
#define BMA2X2_LOW_POWER_MODE__REG		BMA2X2_LOW_NOISE_CTRL_REG

/* data */
#define BMA2X2_REG_DATAXLOW			0x02

/*********************************[BMA250]*************************************/
/* chip id */
/* bma250 chip id is same as bma222 */

/* i2c address */
#define BMA250_I2C_ADDRESS_PIN_HIGH		0x19
#define BMA250_I2C_ADDRESS_PIN_LOW		0x18

/* bandwidth */
#define BMA250_BW_7_81HZ			0x08
#define BMA250_BW_15_63HZ			0x09
#define BMA250_BW_31_25HZ			0x0A
#define BMA250_BW_62_50HZ			0x0B
#define BMA250_BW_125HZ				0x0C
#define BMA250_BW_250HZ				0x0D
#define BMA250_BW_500HZ				0x0E
#define BMA250_BW_1000HZ			0x0F

#define BMA250_BW_SEL_REG			0x10

#define BMA250_BANDWIDTH__POS			0
#define BMA250_BANDWIDTH__LEN			5
#define BMA250_BANDWIDTH__MSK			0x1F
#define BMA250_BANDWIDTH__REG			BMA250_BW_SEL_REG

/* range */
#define BMA250_RANGE_2G				3
#define BMA250_RANGE_4G				5
#define BMA250_RANGE_8G				8

#define BMA250_RANGE_SEL_REG			0x0F

#define BMA250_RANGE_SEL__POS			0
#define BMA250_RANGE_SEL__LEN			4
#define BMA250_RANGE_SEL__MSK			0x0F
#define BMA250_RANGE_SEL__REG			BMA250_RANGE_SEL_REG

/* power mode */
#define BMA250_MODE_NORMAL			0
#define BMA250_MODE_SUSPEND			1

#define BMA250_MODE_CTRL_REG			0x11

#define BMA250_EN_LOW_POWER__POS		6
#define BMA250_EN_LOW_POWER__LEN		1
#define BMA250_EN_LOW_POWER__MSK		0x40
#define BMA250_EN_LOW_POWER__REG		BMA250_MODE_CTRL_REG

#define BMA250_EN_SUSPEND__POS			7
#define BMA250_EN_SUSPEND__LEN			1
#define BMA250_EN_SUSPEND__MSK			0x80
#define BMA250_EN_SUSPEND__REG			BMA250_MODE_CTRL_REG

/* data */
#define BMA250_REG_DATAXLOW			0x02

/*********************************[BMA250E]************************************/
/* chip id */
#define BMA250E_CHIP_ID				0xF9

/* i2c address */
#define BMA250E_I2C_ADDRESS_PIN_HIGH		0x19
#define BMA250E_I2C_ADDRESS_PIN_LOW		0x18

/* bandwidth */
/*bma250e bandwidth is same as bma222e */

/* range */
/*bma250e range is same as bma222e */

/* power mode */
/*bma250e powermode is same as bma222e */

/* data */
/*bma250e data register is same as bma222e */

/*********************************[BMA255]*************************************/
/* chip id */
#define BMA255_CHIP_ID				0xFA

/* i2c address */
#define BMA255_I2C_ADDRESS_PIN_HIGH		0x19
#define BMA255_I2C_ADDRESS_PIN_LOW		0x18

/* bandwidth */
/*bma255 bandwidth is same as bma222e */

/* range */
/*bma255 range is same as bma222e */

/* power mode */
/*bma255 powermode is same as bma222e */

/* data */
/*bma255 data register is same as bma222e */

/*********************************[BMA280]*************************************/
/* chip id */
#define BMA280_CHIP_ID				0xFB

/* i2c address */
#define BMA280_I2C_ADDRESS_PIN_HIGH		0x19
#define BMA280_I2C_ADDRESS_PIN_LOW		0x18

/* bandwidth */
/*
*bma280 bandwidth is same as bma222e except 1000Hz,
*bma280 doesn't support 1000Hz bandwidth.
*/

/* range */
/*bma280 range is same as bma222e */

/* power mode */
/*bma280 powermode is same as bma222e */

/* data */
/*bma280 data register is same as bma222e */
#define BMA222_SUCCESS						0

#endif/* BMA2XX_H */

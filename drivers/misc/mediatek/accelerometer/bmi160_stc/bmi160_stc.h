/* BOSCH STEP COUNTER Sensor Driver Header File
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

#ifndef BMI160_STC_H
#define BMI160_STC_H

#include <linux/ioctl.h>

#define STC_DRIVER_VERSION "V1.0"

/*according i2c_mt6516.c */
#define C_I2C_FIFO_SIZE         8
#define C_MAX_FIR_LENGTH		(32)
#define MAX_SENSOR_NAME			(32)
#define BMG_BUFSIZE				128
#define E_BMI160_OUT_OF_RANGE	((s8)-2)

/* common definition */
#define BMI160_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMI160_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

#define STC_DEV_NAME				"step_counter"
#define BMI160_GYRO_I2C_ADDRESS		0x66
#define BMI160_I2C_ADDR				0x68
#define	SUCCESS						((u8)0)
#define	ERROR						((s8)-1)
#define	ENABLE						(1)
#define	DISABLE						(0)
#define BMI160_USER_CHIP_ID_ADDR		0x00
#define BMI160_USER_STEP_COUNT_0_ADDR	(0X78)
#define	BMI160_STEP_COUNT_LSB_BYTE    	(0)
#define	BMI160_STEP_COUNT_MSB_BYTE    	(1)
#define BMI160_SHIFT_BIT_POSITION_BY_08_BITS     (8)
#define	BMI160_STEP_COUNT_LSB_BYTE    	(0)
#define BMI160_STEP_COUNTER_LENGTH	 	(2)
#define BMI160_GEN_READ_WRITE_DATA_LENGTH	(1)
/***************************************************/
/**\name STEP COUNTER CONFIGURATION REGISTERS*/
/******************************************************/
#define BMI160_USER_STEP_CONFIG_0_ADDR			(0X7A)
#define BMI160_USER_STEP_CONFIG_1_ADDR			(0X7B)
/**************************************************************/
/**\name	STEP COUNTER ENABLE LENGTH, POSITION AND MASK*/
/**************************************************************/
/* STEP_CONFIG_1  Description - Reg Addr --> 0x7B, Bit -->  0 to 2 */
#define BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__POS		(3)
#define BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__LEN		(1)
#define BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__MSK		(0x08)
#define BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__REG	\
(BMI160_USER_STEP_CONFIG_1_ADDR)
/**************************************************************/
/**\name	STEP COUNTER LENGTH, POSITION AND MASK*/
/**************************************************************/
/* STEP_CNT_0  Description - Reg Addr --> 0x78, Bit -->  0 to 7 */
#define BMI160_USER_STEP_COUNT_LSB__POS               (0)
#define BMI160_USER_STEP_COUNT_LSB__LEN               (7)
#define BMI160_USER_STEP_COUNT_LSB__MSK               (0xFF)
#define BMI160_USER_STEP_COUNT_LSB__REG	 (BMI160_USER_STEP_COUNT_0_ADDR)

/* USER DATA REGISTERS DEFINITION START */
/* Chip ID Description - Reg Addr --> 0x00, Bit --> 0...7 */
#define BMI160_USER_CHIP_ID__POS             0
#define BMI160_USER_CHIP_ID__MSK            0xFF
#define BMI160_USER_CHIP_ID__LEN             8
#define BMI160_USER_CHIP_ID__REG             BMI160_USER_CHIP_ID_ADDR

#define BMI160_USER_ACC_CONF_ADDR			0X40
/* Acc_Conf Description - Reg Addr --> 0x40, Bit --> 0...3 */
#define BMI160_USER_ACC_CONF_ODR__POS       0
#define BMI160_USER_ACC_CONF_ODR__LEN       4
#define BMI160_USER_ACC_CONF_ODR__MSK       0x0F
#define BMI160_USER_ACC_CONF_ODR__REG		BMI160_USER_ACC_CONF_ADDR


#define SENSOR_CHIP_ID_BMI (0xD0)
#define SENSOR_CHIP_ID_BMI_C2 (0xD1)
#define SENSOR_CHIP_ID_BMI_C3 (0xD3)

#define SENSOR_CHIP_REV_ID_BMI (0x00)

/* BMI160 Accel ODR */
#define BMI160_ACCEL_ODR_RESERVED       0x00
#define BMI160_ACCEL_ODR_0_78HZ         0x01
#define BMI160_ACCEL_ODR_1_56HZ         0x02
#define BMI160_ACCEL_ODR_3_12HZ         0x03
#define BMI160_ACCEL_ODR_6_25HZ         0x04
#define BMI160_ACCEL_ODR_12_5HZ         0x05
#define BMI160_ACCEL_ODR_25HZ           0x06
#define BMI160_ACCEL_ODR_50HZ           0x07
#define BMI160_ACCEL_ODR_100HZ          0x08
#define BMI160_ACCEL_ODR_200HZ          0x09
#define BMI160_ACCEL_ODR_400HZ          0x0A
#define BMI160_ACCEL_ODR_800HZ          0x0B
#define BMI160_ACCEL_ODR_1600HZ         0x0C
#define BMI160_ACCEL_ODR_RESERVED0      0x0D
#define BMI160_ACCEL_ODR_RESERVED1      0x0E
#define BMI160_ACCEL_ODR_RESERVED2      0x0F
#define C_BMI160_FOURTEEN_U8X           ((u8)14)
#define BMI160_USER_GYR_RANGE_ADDR      0X43
#define BMI160_CMD_COMMANDS_ADDR		0X7E
/* Command description address - Reg Addr --> 0x7E, Bit -->  0....7 */
#define BMI160_CMD_COMMANDS__POS        0
#define BMI160_CMD_COMMANDS__LEN        8
#define BMI160_CMD_COMMANDS__MSK        0xFF
#define BMI160_CMD_COMMANDS__REG	 	BMI160_CMD_COMMANDS_ADDR

#define CMD_PMU_ACC_SUSPEND           0x10
#define CMD_PMU_ACC_NORMAL            0x11
#define CMD_PMU_GYRO_SUSPEND          0x14
#define CMD_PMU_GYRO_NORMAL           0x15
#define CMD_PMU_GYRO_FASTSTART        0x17

#define BMI160_SHIFT_1_POSITION       1
#define BMI160_SHIFT_2_POSITION       2
#define BMI160_SHIFT_3_POSITION       3
#define BMI160_SHIFT_4_POSITION       4
#define BMI160_SHIFT_5_POSITION       5
#define BMI160_SHIFT_6_POSITION       6
#define BMI160_SHIFT_7_POSITION       7
#define BMI160_SHIFT_8_POSITION       8
#define BMI160_SHIFT_12_POSITION      12
#define BMI160_SHIFT_16_POSITION      16

#endif/* BMI160_STC_H */

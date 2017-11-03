/*
 * ICM206XX sensor driver
 * Copyright (C) 2016 Invensense, Inc.
 *
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

#ifndef ICM20608D_REGISTER_H
#define ICM20608D_REGISTER_H

/*=======================================================================================*/
/* ICM20608D Register Map								 */
/* Acc, Gyro and step counter has same Register map					 */
/*=======================================================================================*/

#define ICM206XX_REG_SELF_TEST_X_GYRO		0x00

#define ICM206XX_REG_SELF_TEST_X_ACC		0x0D

#define ICM206XX_REG_SAMRT_DIV			0x19
#define ICM206XX_REG_CFG			0x1A

#define ICM206XX_REG_GYRO_CFG			0x1B
#define ICM206XX_REG_ACCEL_CFG			0x1C
#define ICM206XX_REG_ACCEL_CFG2			0x1D

#define ICM206XX_REG_ACCEL_XH			0x3B
#define ICM206XX_REG_GYRO_XH			0x43

#define ICM206XX_REG_RESET			0x68
#define ICM206XX_REG_USER_CTL			0x6A
#define ICM206XX_REG_PWR_CTL			0x6B
#define ICM206XX_REG_PWR_CTL2			0x6C

#define ICM206XX_REG_MEM_BANK_SEL		0x6D
#define ICM206XX_REG_MEM_START_ADDR		0x6E
#define ICM206XX_REG_MEM_R_W			0x6F
#define ICM206XX_REG_PRGM_START_ADDRH		0x70

#define ICM206XX_REG_WHO_AM_I			0x75

/*----------------------------------------------------------------------------*/

#define ICM206XX_BIT_DEVICE_RESET		0x80
#define ICM206XX_BIT_SLEEP			0x40	/* enable low power sleep mode */

#define ICM206XX_BIT_ACCEL_FSR			0x18	/* b' 00011000 */
#define ICM206XX_BIT_GYRO_FSR			0x18

#define ICM206XX_BIT_ACCEL_FILTER		0x07	/* b' 00000111 */
#define ICM206XX_BIT_GYRO_FILTER		0x07

#define ICM206XX_BIT_ACCEL_SELFTEST		0xE0
#define ICM206XX_BIT_GYRO_SELFTEST		0xE0

#define ICM206XX_BIT_ACCEL_STANDBY		0x38
#define ICM206XX_BIT_GYRO_STANDBY		0x07

#define ICM206XX_ACCEL_FS_SEL			0x03	/* ICM206XX_REG_ACCEL_CFG[4:3] */
#define ICM206XX_GYRO_FS_SEL			0x03	/* ICM206XX_REG_GYRO_CFG[4:3] */


/*----------------------------------------------------------------------------*/

#define ICM206XX_INTERNAL_SAMPLE_RATE		1000

/*----------------------------------------------------------------------------*/

#define ICM20608D_WHO_AM_I			0xAE

/*----------------------------------------------------------------------------*/

#endif /* ICM20608D_REGISTER_H */


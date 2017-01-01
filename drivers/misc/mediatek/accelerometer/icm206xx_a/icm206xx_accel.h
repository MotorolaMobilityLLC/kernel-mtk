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

#ifndef ICM206XX_ACCEL_H
#define ICM206XX_ACCEL_H

/*----------------------------------------------------------------------------*/
/* 1 G = 9.80665f */
/* +/-2G = 4G(-2G ~ 2G) 0 ~ 65535 1G = 16384 0.06mg = 1 */
/* +/-4G = 8G(-4G ~ 4G) 0 ~ 65535 1G = 8192 0.12mg = 1 */
/* +/-8G = 8G(-8G ~ 8G) 0 ~ 65535 1G = 4096 0.24mg = 1 */
/* +/-16G = 8G(-16G ~ 16G) 0 ~ 65535 1G = 2046 0.49mg = 1 */

#define ICM206XX_ACCEL_MAX_SENSITIVITY			16384		/* Max: ICM206XX_ACC_RANGE_2G */
#define ICM206XX_ACCEL_DEFAULT_SENSITIVITY		8192		/* Default: ICM206XX_ACC_RANGE_4G */

/* GRAVITY_EARTH_1000 is defined as 9807 */
						/* Accel div 1000 */
/*----------------------------------------------------------------------------*/
enum icm206xx_accel_fsr {
	ICM206XX_ACCEL_RANGE_2G = 0,			/* +/-2G */
	ICM206XX_ACCEL_RANGE_4G = 1,			/* +/-4G by default */
	ICM206XX_ACCEL_RANGE_8G = 2,			/* +/-8G */
	ICM206XX_ACCEL_RANGE_16G = 3			/* +/-16G */
};

enum icm206xx_accel_filter {
	ICM206XX_ACCEL_DLPF_0 = 0,			/* Bandwidth 460hz */
	ICM206XX_ACCEL_DLPF_1 = 1,			/* Bandwidth 184hz by default */
	ICM206XX_ACCEL_DLPF_2 = 2,			/* Bandwidth 92hz */
	ICM206XX_ACCEL_DLPF_3 = 3,			/* Bandwidth 41hz */
	ICM206XX_ACCEL_DLPF_4 = 4,			/* Bandwidth 20hz */
	ICM206XX_ACCEL_DLPF_5 = 5,			/* Bandwidth 10hz */
	ICM206XX_ACCEL_DLPF_6 = 6,			/* Bandwidth 5hz */
	ICM206XX_ACCEL_DLPF_7 = 7			/* Bandwidth 460hz */

};
/*----------------------------------------------------------------------------*/
#endif /* ICM206XX_ACCEL_H */


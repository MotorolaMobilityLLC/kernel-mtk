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

#ifndef ICM206XX_GYRO_H
#define ICM206XX_GYRO_H

/*----------------------------------------------------------------------------*/
/*

FULL_DATA_RANGE	65536 // 2^16

FULL_SCALE_RANGE1	500   // -250 ~ +250
FULL_SCALE_RANGE2	1000   // -500 ~ +500
FULL_SCALE_RANGE3	2000   // -1000 ~ +1000
FULL_SCALE_RANGE4	4000   // -2000 ~ +2000

SENSITIVITY		FULL_DATA_RANGE / FULL_SCALE_RANGE
!! SENSITIVITY accepted as same as SENSITIVITY_SCALE_FACTOR

Acquired Data (represented in FULL_DATA_RANGE) / SENSITIVITY_SCALE_FACTOR = Acquired DPS

Calculation of Current Sensitivy with FSR value (0 .. 3)
Current Sensitivity --> Max Sensitivity >> FSR Value

Radian to Degree Factor --> 180 / PI  : x radian = x * 180 / PI degree
Applying SENSITIVITY to above Radian to Degree Factor =
	(180 / PI) * SENSITIVITY
ex) SENSITIVITY = 131
	(180 / PI) * 131 = 7506
	1 degree / 7506 = x radian

Measured data always scaled to MAX_SENSITIVITY,
so Degree to Radian Factor always be 7506 and it is a divider to measured data.


Orientation Translation
Mapping Sensor Orientation into Device (Phone) Orientation
Used when reading sensor data and calibration value
Device[AXIS] = Sensor[Converted AXIS] * Sign[AXIS]

Mapping Device (Phone) Orientation into Sensor Orientation
Used when writing calibration value
Sensor[Converted AXIS] = Device[AXIS] * Sign[AXIS]
*/

#define ICM206XX_GYRO_MAX_SENSITIVITY	131		/* Max: ICM206XX_GYRO_RANGE_250DPS */
#define ICM206XX_GYRO_DEFAULT_SENSITIVITY	33	/* Default: ICM206XX_GYRO_RANGE_1000DPS */

#define DEGREE_TO_RAD    7506				/* The value calculated with MAX_SENSITIVITY */
							/* Gyro div */
/*----------------------------------------------------------------------------*/
enum icm206xx_gyro_fsr {
	ICM206XX_GYRO_RANGE_250DPS = 0,		/* +/-250dps */
	ICM206XX_GYRO_RANGE_500DPS = 1,		/* +/-500dps */
	ICM206XX_GYRO_RANGE_1000DPS = 2,		/* +/-1000dps by default */
	ICM206XX_GYRO_RANGE_2000DPS = 3			/* +/-2000dps */
};

enum icm206xx_gyro_filter {
	ICM206XX_GYRO_DLPF_0 = 0,			/* Bandwidth 250hz */
	ICM206XX_GYRO_DLPF_1 = 1,			/* Bandwidth 184hz by default */
	ICM206XX_GYRO_DLPF_2 = 2,			/* Bandwidth 92hz */
	ICM206XX_GYRO_DLPF_3 = 3,			/* Bandwidth 41hz */
	ICM206XX_GYRO_DLPF_4 = 4,			/* Bandwidth 20hz */
	ICM206XX_GYRO_DLPF_5 = 5,			/* Bandwidth 10hz */
	ICM206XX_GYRO_DLPF_6 = 6,			/* Bandwidth 5hz */
	ICM206XX_GYRO_DLPF_7 = 7			/* Bandwidth 460hz */

};
/*----------------------------------------------------------------------------*/
#endif /* ICM206XX_GYRO_H */


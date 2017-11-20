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

#ifndef ICM206XX_SHARE_H
#define ICM206XX_SHARE_H

/*=======================================================================================*/
/* ICM206XX Sensor Shared Definition - Accel, Gyro and Step Counter			 */
/*=======================================================================================*/

#define ICM206XX_SUCCESS			0
#define ICM206XX_ERR_I2C			-1
#define ICM206XX_ERR_INVALID_PARAM		-2
#define ICM206XX_ERR_STATUS			-3
#define ICM206XX_ERR_SETUP_FAILURE		-4

/*----------------------------------------------------------------------------*/

#define ICM206XX_AXIS_X				0
#define ICM206XX_AXIS_Y				1
#define ICM206XX_AXIS_Z				2
#define ICM206XX_AXIS_NUM			3

#define ICM206XX_DATA_LEN			6

#define ICM206XX_BUFSIZE			60

/*----------------------------------------------------------------------------*/
enum icm206xx_sensor_type {
	ICM206XX_SENSOR_TYPE_ACC,
	ICM206XX_SENSOR_TYPE_GYRO,
	ICM206XX_SENSOR_TYPE_STEP_COUNTER,
	ICM206XX_SENSOR_TYPE_FLOOR_COUNTER,
	ICM206XX_SENSOR_TYPE_MAX
};

/*----------------------------------------------------------------------------*/

/* To enable selftest, activate this feature */
/* #define ICM206XX_SELFTEST */

#ifdef ICM206XX_SELFTEST

#define ICM206XX_ST_SAMPLE_COUNT		200
#define ICM206XX_ST_PRECISION			1000

#define ICM206XX_ST_ACCEL_DELTA_MIN		50
#define ICM206XX_ST_ACCEL_DELTA_MAX		150
#define ICM206XX_ST_ACCEL_AL_MIN		225
#define ICM206XX_ST_ACCEL_AL_MAX		675

#define ICM206XX_ST_GYRO_DELTA			50
#define ICM206XX_ST_GYRO_AL			60
#define ICM206XX_ST_GYRO_OFFSET_MAX		20

/* lookup table: convert selft test code in register to otp value */
/* ST_OTP = (2620 / 2^FS) * 1.01^(ST_Code -1) (LSB) */
static const u16 st_otp_lookup_tbl[256] = {

	2620, 2646, 2672, 2699, 2726, 2753, 2781, 2808,
	2837, 2865, 2894, 2923, 2952, 2981, 3011, 3041,
	3072, 3102, 3133, 3165, 3196, 3228, 3261, 3293,
	3326, 3359, 3393, 3427, 3461, 3496, 3531, 3566,
	3602, 3638, 3674, 3711, 3748, 3786, 3823, 3862,
	3900, 3939, 3979, 4019, 4059, 4099, 4140, 4182,
	4224, 4266, 4308, 4352, 4395, 4439, 4483, 4528,
	4574, 4619, 4665, 4712, 4759, 4807, 4855, 4903,
	4953, 5002, 5052, 5103, 5154, 5205, 5257, 5310,
	5363, 5417, 5471, 5525, 5581, 5636, 5693, 5750,
	5807, 5865, 5924, 5983, 6043, 6104, 6165, 6226,
	6289, 6351, 6415, 6479, 6544, 6609, 6675, 6742,
	6810, 6878, 6946, 7016, 7086, 7157, 7229, 7301,
	7374, 7448, 7522, 7597, 7673, 7750, 7828, 7906,
	7985, 8065, 8145, 8227, 8309, 8392, 8476, 8561,
	8647, 8733, 8820, 8909, 8998, 9088, 9178, 9270,
	9363, 9457, 9551, 9647, 9743, 9841, 9939, 10038,
	10139, 10240, 10343, 10446, 10550, 10656, 10763, 10870,
	10979, 11089, 11200, 11312, 11425, 11539, 11654, 11771,
	11889, 12008, 12128, 12249, 12371, 12495, 12620, 12746,
	12874, 13002, 13132, 13264, 13396, 13530, 13666, 13802,
	13940, 14080, 14221, 14363, 14506, 14652, 14798, 14946,
	15096, 15247, 15399, 15553, 15709, 15866, 16024, 16184,
	16346, 16510, 16675, 16842, 17010, 17180, 17352, 17526,
	17701, 17878, 18057, 18237, 18420, 18604, 18790, 18978,
	19167, 19359, 19553, 19748, 19946, 20145, 20347, 20550,
	20756, 20963, 21173, 21385, 21598, 21814, 22033, 22253,
	22475, 22700, 22927, 23156, 23388, 23622, 23858, 24097,
	24338, 24581, 24827, 25075, 25326, 25579, 25835, 26093,
	26354, 26618, 26884, 27153, 27424, 27699, 27976, 28255,
	28538, 28823, 29112, 29403, 29697, 29994, 30294, 30597,
	30903, 31212, 31524, 31839, 32157, 32479, 32804
};
#endif

/*----------------------------------------------------------------------------*/

extern int icm206xx_share_i2c_read_register(u8 addr, u8 *data, u8 len);
extern int icm206xx_share_i2c_write_register(u8 addr, u8 *data, u8 len);
extern int icm206xx_share_i2c_read_memory(u16 mem_addr, u32 len, u8 *data);
extern int icm206xx_share_i2c_write_memory(u16 mem_addr, u32 len, u8 const *data);

extern int icm206xx_share_ChipSoftReset(void);
extern int icm206xx_share_SetPowerMode(int sensor_type, bool enable);
extern int icm206xx_share_EnableSensor(int sensor_type, bool enable);

extern int icm206xx_share_ReadChipInfo(char *buf, int bufsize);
extern int icm206xx_share_SetSampleRate(int sensor_type, u64 delay_ns, bool force_1khz);

/*----------------------------------------------------------------------------*/

#endif /* ICM206XX_SHARE_H */


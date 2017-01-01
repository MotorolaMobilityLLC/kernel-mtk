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

#include <linux/ioctl.h>
#include <hwmsensor.h>
#include "cust_acc.h"
#include "accel.h"
#include "icm206xx_register_20608D.h"
#include "icm206xx_share.h"
#include "icm206xx_accel.h"

#define ICM206XX_ACCEL_DEV_NAME	"ICM206XX_ACCEL"

struct icm206xx_accel_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert cvt;

	/*misc */
	atomic_t trace;
	atomic_t suspend;
	atomic_t selftest;

	/*data */
	s16 cali_sw[ICM206XX_AXIS_NUM + 1];
	s16 data[ICM206XX_AXIS_NUM + 1];

	u8 offset[ICM206XX_AXIS_NUM + 1];
};

static int icm206xx_accel_init_flag = -1;

struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
struct i2c_client *icm206xx_accel_i2c_client;
static struct icm206xx_accel_i2c_data *obj_i2c_data;

#ifdef ICM206XX_SELFTEST
static char selftestRes[8] = { 0 };
#endif

static int g_icm206xx_accel_sensitivity = ICM206XX_ACCEL_DEFAULT_SENSITIVITY;	/* +/-4G as Default */

static int icm206xx_accel_local_init(void);
static int icm206xx_accel_remove(void);
static struct acc_init_info icm206xx_accel_init_info = {
	.name = ICM206XX_ACCEL_DEV_NAME,
	.init = icm206xx_accel_local_init,
	.uninit = icm206xx_accel_remove,
};

/*=======================================================================================*/
/* Debug Section									 */
/*=======================================================================================*/

#define DEBUG_MAX_CHECK_COUNT		32
#define DEBUG_MAX_IOCTL_CMD_COUNT	168
#define DEBUG_MAX_LOG_STRING_LENGTH	512
#define DEBUG_IOCTL_CMD_DEFAULT		0xFF

#define DEBUG_PRINT_ELEMENT_COUNT	8
#define DEBUG_PRINT_CHAR_COUNT_PER_LINE 24

struct icm206xx_debug_data {
	int nTestCount[DEBUG_MAX_CHECK_COUNT];
	int nUsedTestCount;

	unsigned char ucCmdLog[DEBUG_MAX_IOCTL_CMD_COUNT];
	int nLogIndex;
	unsigned char ucPrevCmd;

	int nDriverState;

	char strDebugDump[DEBUG_MAX_LOG_STRING_LENGTH];
	int nStrIndex;
};

struct icm206xx_debug_data icm206xx_accel_debug;

static void initDebugData(void)
{
	memset(&icm206xx_accel_debug, 0x00, sizeof(icm206xx_accel_debug));
	icm206xx_accel_debug.ucPrevCmd = DEBUG_IOCTL_CMD_DEFAULT;
	memset(&(icm206xx_accel_debug.ucCmdLog[0]), DEBUG_IOCTL_CMD_DEFAULT, DEBUG_MAX_IOCTL_CMD_COUNT);
}

static void addCmdLog(unsigned int cmd)
{
	unsigned char converted_cmd = DEBUG_IOCTL_CMD_DEFAULT;

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		converted_cmd = 0;
		break;
	case GSENSOR_IOCTL_READ_CHIPINFO:
		converted_cmd = 1;
		break;
	case GSENSOR_IOCTL_READ_SENSORDATA:
		converted_cmd = 2;
		break;
	case GSENSOR_IOCTL_READ_OFFSET:
		converted_cmd = 3;
		break;
	case GSENSOR_IOCTL_READ_GAIN:
		converted_cmd = 4;
		break;
	case GSENSOR_IOCTL_READ_RAW_DATA:
		converted_cmd = 5;
		break;
	case GSENSOR_IOCTL_SET_CALI:
		converted_cmd = 6;
		break;
	case GSENSOR_IOCTL_GET_CALI:
		converted_cmd = 7;
		break;
	case GSENSOR_IOCTL_CLR_CALI:
		converted_cmd = 8;
		break;
	}

	icm206xx_accel_debug.nDriverState = 0;

	if ((icm206xx_accel_debug.nLogIndex < DEBUG_MAX_IOCTL_CMD_COUNT) && (converted_cmd != DEBUG_IOCTL_CMD_DEFAULT)) {
		icm206xx_accel_debug.ucCmdLog[icm206xx_accel_debug.nLogIndex++] = converted_cmd;
		icm206xx_accel_debug.ucPrevCmd = converted_cmd;
	}
}

static void printCmdLog(void)
{
	int i;
	int nRowCount = 0;

	nRowCount = (int)(icm206xx_accel_debug.nLogIndex / 8);

	for (i = 0; i < nRowCount + 1; i++) {
		sprintf(&icm206xx_accel_debug.strDebugDump[i * 24], "%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
			icm206xx_accel_debug.ucCmdLog[i * 8 + 0],
			icm206xx_accel_debug.ucCmdLog[i * 8 + 1],
			icm206xx_accel_debug.ucCmdLog[i * 8 + 2],
			icm206xx_accel_debug.ucCmdLog[i * 8 + 3],
			icm206xx_accel_debug.ucCmdLog[i * 8 + 4],
			icm206xx_accel_debug.ucCmdLog[i * 8 + 5], icm206xx_accel_debug.ucCmdLog[i * 8 + 6], icm206xx_accel_debug.ucCmdLog[i * 8 + 7]);
	}

	ACC_LOG("ACCEL_IOCTL_CMD_LOG :\n%s\t%d\n", icm206xx_accel_debug.strDebugDump, icm206xx_accel_debug.nLogIndex);
}

/*=======================================================================================*/
/* Vendor Specific Functions Section							 */
/*=======================================================================================*/

static int icm206xx_accel_SetFullScale(struct i2c_client *client, u8 accel_fsr)
{
	u8 databuf[2] = { 0 };
	int res = 0;

	if (accel_fsr < ICM206XX_ACCEL_RANGE_2G || accel_fsr > ICM206XX_ACCEL_RANGE_16G) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_CFG, databuf, 1);
	if (res < 0) {
		ACC_ERR("read fsr register err!\n");
		return ICM206XX_ERR_I2C;
	}

	databuf[0] &= ~ICM206XX_BIT_ACCEL_FSR;	/* clear FSR bit */
	databuf[0] |= accel_fsr << ICM206XX_ACCEL_FS_SEL;

	g_icm206xx_accel_sensitivity = ICM206XX_ACCEL_MAX_SENSITIVITY >> accel_fsr;

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_ACCEL_CFG, databuf, 1);
	if (res < 0) {
		ACC_ERR("write fsr register err!\n");
		return ICM206XX_ERR_I2C;
	}

/*
	databuf[0] = 0;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_CFG, databuf, 1);
	if (res < 0) {
		ACC_ERR("Read fsr register err!\n");
		return ICM206XX_ERR_I2C;
	}

	ACC_LOG("read fsr: 0x%x\n", databuf[0]);
*/
	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_SetFilter(struct i2c_client *client, u8 accel_filter)
{
	u8 databuf[2] = { 0 };
	int res = 0;

	if (accel_filter < ICM206XX_ACCEL_DLPF_0 || accel_filter > ICM206XX_ACCEL_DLPF_7) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_CFG2, databuf, 1);
	if (res < 0) {
		ACC_ERR("read filter register err!\n");
		return ICM206XX_ERR_I2C;
	}

	databuf[0] &= ~ICM206XX_BIT_ACCEL_FILTER;	/* clear Filter bit */
	databuf[0] |= accel_filter;

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_ACCEL_CFG2, databuf, 1);
	res = 0;		/* Taly!!! : Remove this line on actual platform */
	/* i2c write return fail on MTK evb, but value is written properly */
	if (res < 0) {
		ACC_ERR("write filter register err!\n");
		return ICM206XX_ERR_I2C;
	}

/*
	databuf[0] = 0;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_CFG2, databuf, 1);
	if (res < 0) {
		ACC_ERR("Read filter register err!\n");
		return ICM206XX_ERR_I2C;
	}

	ACC_LOG("read filter: 0x%x\n", databuf[0]);
*/

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadSensorDataDirect(struct i2c_client *client, s16 data[ICM206XX_AXIS_NUM])
{
	char databuf[6];
	int i;
	int res = 0;

	if (client == NULL)
		return ICM206XX_ERR_INVALID_PARAM;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_XH, databuf, ICM206XX_DATA_LEN);
	if (res < 0) {
		ACC_ERR("read accelerometer data error\n");
		return ICM206XX_ERR_I2C;
	}

	for (i = 0; i < ICM206XX_AXIS_NUM; i++)
		data[i] = ((s16) ((databuf[i * 2] << 8) | (databuf[i * 2 + 1])));	/* convert 8-bit to 16-bit */

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	char databuf[6];
	int data[3];
	int res = 0;
	int i = 0;
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);

	if (client == NULL)
		return ICM206XX_ERR_INVALID_PARAM;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_XH, databuf, ICM206XX_DATA_LEN);
	if (res < 0) {
		ACC_ERR("read accelerometer data error\n");
		return ICM206XX_ERR_I2C;
	}

	/* convert 8-bit to 16-bit */
	/* This part must be done completely before any translation (Orientation Translation, Unit Translation) */
	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		obj->data[i] = ((s16) ((databuf[i * 2] << 8) | (databuf[i * 2 + 1])));

		/* Add Cali */
		obj->data[i] += obj->cali_sw[i];
	}

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/* Orientation Translation Lower (sensor) --> Upper (Device) */
		data[i] = obj->cvt.sign[i] * obj->data[obj->cvt.map[i]];

		/* Unit Translation */
		data[i] = data[i] * GRAVITY_EARTH_1000 / g_icm206xx_accel_sensitivity;
	}

	sprintf(buf, "%04x %04x %04x", data[ICM206XX_AXIS_X], data[ICM206XX_AXIS_Y], data[ICM206XX_AXIS_Z]);

	if (atomic_read(&obj->trace)) {
		ACC_LOG("Accelerometer Data - %04x %04x %04x\n", data[ICM206XX_AXIS_X], data[ICM206XX_AXIS_Y], data[ICM206XX_AXIS_Z]);
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadOffsetData(struct i2c_client *client, char *buf)
{
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);

	if (client == NULL)
		return ICM206XX_ERR_INVALID_PARAM;

	/* offset value [0, 0, 0] */
	sprintf(buf, "%04x %04x %04x", obj->offset[ICM206XX_AXIS_X], obj->offset[ICM206XX_AXIS_Y], obj->offset[ICM206XX_AXIS_Z]);

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadGain(struct i2c_client *client, struct GSENSOR_VECTOR3D *gsensor_gain)
{
	if (client == NULL)
		return ICM206XX_ERR_INVALID_PARAM;

	gsensor_gain->x = gsensor_gain->y = gsensor_gain->z = g_icm206xx_accel_sensitivity;

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	s16 data[ICM206XX_AXIS_NUM] = { 0, 0, 0 };

	res = icm206xx_accel_ReadSensorDataDirect(client, data);
	if (res < 0) {
		ACC_ERR("read accelerometer raw data  error\n");
		return ICM206XX_ERR_I2C;
	}

	/* Sensor Raw Data direct read from sensor register     */
	/* No orientation translation, No unit Translation      */
	sprintf(buf, "%04x %04x %04x", data[ICM206XX_AXIS_X], data[ICM206XX_AXIS_Y], data[ICM206XX_AXIS_Z]);

	return ICM206XX_SUCCESS;
}

/*----------------------------------------------------------------------------*/
static int icm206xx_accel_ResetCalibration(struct i2c_client *client)
{
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));

	return ICM206XX_SUCCESS;
}

static int icm206xx_accel_ReadCalibration(struct i2c_client *client, struct SENSOR_DATA *sensor_data)
{
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);
	int i;
	int res = 0;
	int dat[ICM206XX_AXIS_NUM];

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		dat[i] = obj->cvt.sign[i] * obj->cali_sw[obj->cvt.map[i]];
	}

	/* lower layer (sensor) unit Translation into Upper layer (HAL) unit */
	sensor_data->x = dat[ICM206XX_AXIS_X] * GRAVITY_EARTH_1000 / g_icm206xx_accel_sensitivity;
	sensor_data->y = dat[ICM206XX_AXIS_Y] * GRAVITY_EARTH_1000 / g_icm206xx_accel_sensitivity;
	sensor_data->z = dat[ICM206XX_AXIS_Z] * GRAVITY_EARTH_1000 / g_icm206xx_accel_sensitivity;

	if (atomic_read(&obj->trace)) {
		ACC_LOG("Accel ReadCalibration:[sensor_data:%5d %5d %5d][cali_sw:%5d %5d %5d]\n",
			sensor_data->x, sensor_data->y, sensor_data->z,
			obj->cali_sw[ICM206XX_AXIS_X], obj->cali_sw[ICM206XX_AXIS_Y], obj->cali_sw[ICM206XX_AXIS_Z]);
	}

	return res;
}

static int icm206xx_accel_WriteCalibration(struct i2c_client *client, struct SENSOR_DATA *sensor_data)
{
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);
	int cali[ICM206XX_AXIS_NUM];
	int res = 0;
	int i;

	/* Upper layer (HAL) unit Translation into lower layer (sensor) unit */
	cali[ICM206XX_AXIS_X] = sensor_data->x * g_icm206xx_accel_sensitivity / GRAVITY_EARTH_1000;
	cali[ICM206XX_AXIS_Y] = sensor_data->y * g_icm206xx_accel_sensitivity / GRAVITY_EARTH_1000;
	cali[ICM206XX_AXIS_Z] = sensor_data->z * g_icm206xx_accel_sensitivity / GRAVITY_EARTH_1000;

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/* Orientation Translation */
		obj->cali_sw[obj->cvt.map[i]] += obj->cvt.sign[i] * cali[i];
	}

	if (atomic_read(&obj->trace)) {
		ACC_LOG("Accel writeCalibration:[sensor_data:%5d %5d %5d][cali_sw:%5d %5d %5d]\n",
			sensor_data->x, sensor_data->y, sensor_data->z,
			obj->cali_sw[ICM206XX_AXIS_X], obj->cali_sw[ICM206XX_AXIS_Y], obj->cali_sw[ICM206XX_AXIS_Z]);
	}

	return res;
}

/*----------------------------------------------------------------------------*/
#ifdef ICM206XX_SELFTEST
static int icm206xx_accel_InitSelfTest(struct i2c_client *client)
{
	/* -------------------------------------------------------------------------------- */
	/* Configuration Setting on chip to do selftest */
	/* -------------------------------------------------------------------------------- */

	int res = 0;

	/* softreset */
	res = icm206xx_share_ChipSoftReset();
	if (res != ICM206XX_SUCCESS)
		return res;

	/* setpowermode(true) --> exit sleep */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_ACC, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* fsr : ICM206XX_ACCEL_RANGE_2G */
	res = icm206xx_accel_SetFullScale(client, ICM206XX_ACCEL_RANGE_2G);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* filter : ICM206XX_ACCEL_DLPF_2 */
	res = icm206xx_accel_SetFilter(client, ICM206XX_ACCEL_DLPF_2);
	res = 0;
	if (res != ICM206XX_SUCCESS)
		return res;

	/* odr : 1000hz (1kHz) */
	res = icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_ACC, 1000000, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set enable sensor */
	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_ACC, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	return res;
}

static int icm206xx_accel_CalcAvgWithSamples(struct i2c_client *client, int avg[3], int count)
{
	int res = 0;
	int i, nAxis;
	s16 sensor_data[ICM206XX_AXIS_NUM];
	s32 sum[ICM206XX_AXIS_NUM] = { 0, 0, 0 };

	for (i = 0; i < count; i++) {
		res = icm206xx_accel_ReadSensorDataDirect(client, sensor_data);
		if (res) {
			ACC_ERR("read data fail: %d\n", res);
			return ICM206XX_ERR_STATUS;
		}

		for (nAxis = 0; nAxis < ICM206XX_AXIS_NUM; nAxis++)
			sum[nAxis] += sensor_data[nAxis];
		/* Data register updated @1khz */
		mdelay(1);
	}

	for (nAxis = 0; nAxis < ICM206XX_AXIS_NUM; nAxis++)
		avg[nAxis] = (int)(sum[nAxis] / count) * ICM206XX_ST_PRECISION;

	return res;
}

static bool icm206xx_accel_DoSelfTest(struct i2c_client *client)
{
	int res = 0;
	int i;
	int acc_ST_on[ICM206XX_AXIS_NUM], acc_ST_off[ICM206XX_AXIS_NUM];
	int acc_ST_response[ICM206XX_AXIS_NUM];
	u8 st_code[ICM206XX_AXIS_NUM];	/* index of otp_lookup_tbl */
	u16 st_otp[ICM206XX_AXIS_NUM];
	bool otp_value_has_zero = false;
	bool test_result = true;
	u8 databuf[2] = { 0 };

	/* -------------------------------------------------------------------------------- */
	/* Acquire OTP value from OTP Lookup Table */
	/* -------------------------------------------------------------------------------- */

	/* read st_code */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_SELF_TEST_X_ACC, &(st_code[0]), 3);
	if (res) {
		ACC_ERR("Read data fail: %d\n", res);
		return false;
	}

	ACC_LOG("st_code: %02x, %02x, %02x\n", st_code[0], st_code[1], st_code[2]);

	/* lookup OTP value with st_code */
	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		if (st_code[i] != 0)
			st_otp[i] = st_otp_lookup_tbl[st_code[i] - 1];
		else {
			st_otp[i] = 0;
			otp_value_has_zero = true;
		}
	}

	/* -------------------------------------------------------------------------------- */
	/* Read sensor data and calculate average values from it */
	/* -------------------------------------------------------------------------------- */

	/* Read 200 Samples with selftest off */
	res = icm206xx_accel_CalcAvgWithSamples(client, acc_ST_off, ICM206XX_ST_SAMPLE_COUNT);
	if (res) {
		ACC_ERR("Read data fail: %d\n", res);
		return false;
	}

	/* set selftest on */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_ACCEL_CFG, databuf, 1);
	databuf[0] |= ICM206XX_BIT_ACCEL_SELFTEST;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_ACCEL_CFG, databuf, 1);

	/* Wait 20ms for oscillations to stabilize */
	mdelay(20);

	/* Read 200 Samples with selftest on */
	res = icm206xx_accel_CalcAvgWithSamples(client, acc_ST_on, ICM206XX_ST_SAMPLE_COUNT);
	if (res) {
		ACC_ERR("Read data fail: %d\n", res);
		return false;
	}

	/* -------------------------------------------------------------------------------- */
	/* Compare calculated value with INVN OTP value to judge Success or Fail */
	/* -------------------------------------------------------------------------------- */

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		acc_ST_response[i] = abs(acc_ST_on[i] - acc_ST_off[i]);

		if (otp_value_has_zero == false) {
			ACC_LOG("Selftest result A : [i:%d][%d][%d][%d]\n",
				i,
				acc_ST_response[i],
				(st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_ACCEL_DELTA_MIN / 100),
				(st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_ACCEL_DELTA_MAX / 100));

			if ((acc_ST_response[i] < (st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_ACCEL_DELTA_MIN / 100)) ||
			    (acc_ST_response[i] > (st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_ACCEL_DELTA_MAX / 100)))
				test_result = false;
		} else {
			ACC_LOG("Selftest result B: [i:%d][%d][%d][%d]\n",
				i,
				acc_ST_response[i],
				(ICM206XX_ST_ACCEL_AL_MIN * 16384 / 1000 * ICM206XX_ST_PRECISION),
				(ICM206XX_ST_ACCEL_AL_MAX * 16384 / 1000 * ICM206XX_ST_PRECISION));

			if ((acc_ST_response[i] < (ICM206XX_ST_ACCEL_AL_MIN * 16384 / 1000 * ICM206XX_ST_PRECISION)) ||
			    (acc_ST_response[i] > (ICM206XX_ST_ACCEL_AL_MAX * 16384 / 1000 * ICM206XX_ST_PRECISION)))
				test_result = false;
		}

		ACC_LOG("Selftest result:%d [i:%d][acc_ST_response:%d][acc_ST_on:%d][acc_ST_off:%d]\n",
			test_result, i, acc_ST_response[i], acc_ST_on[i], acc_ST_off[i]);

	}

	return test_result;
}
#endif
/*----------------------------------------------------------------------------*/
static int icm206xx_accel_init_client(struct i2c_client *client, bool enable)
{
	int res = 0;

	/* Exit sleep mode */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_ACC, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set fsr ICM206XX_ACCEL_RANGE_4G as default */
	res = icm206xx_accel_SetFullScale(client, ICM206XX_ACCEL_RANGE_4G);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set filter ICM206XX_ACCEL_DLPF_1 as default */
	res = icm206xx_accel_SetFilter(client, ICM206XX_ACCEL_DLPF_1);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set 5ms(200hz) sample rate */
	res = icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_ACC, 5000000, false);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Disable sensor - standby mode for accelerometer */
	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_ACC, enable);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set power mode - sleep or normal */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_ACC, enable);
	if (res != ICM206XX_SUCCESS) {
		return res;
	}

	ACC_LOG("icm206xx_accel_init_client OK!\n");

	return ICM206XX_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/* For  driver get cust info */
struct acc_hw *get_cust_acc(void)
{
	return &accel_cust;
}

/* accelerometer power control function */
static void icm206xx_accel_power(struct acc_hw *hw, unsigned int on)
{
/* Usually, it do nothing in the power function. Because the power of sensor is always on. */
}

/*=======================================================================================*/
/* Driver Attribute Section								 */
/*=======================================================================================*/

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ICM206XX_BUFSIZE];

	icm206xx_share_ReadChipInfo(strbuf, ICM206XX_BUFSIZE);

	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = icm206xx_accel_i2c_client;
	char strbuf[ICM206XX_BUFSIZE];

	if (NULL == client) {
		ACC_ERR("i2c client is null!!\n");
		return 0;
	}

	icm206xx_accel_ReadSensorData(client, strbuf, ICM206XX_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (0 == kstrtoint(buf, 16, &trace))
		atomic_set(&obj->trace, trace);
	else
		ACC_ERR("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw)
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d %d (%d %d)\n",
				obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");

	return len;
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	ACC_LOG("[%s] default direction: %d\n", __func__, obj->hw->direction);

	len = snprintf(buf, PAGE_SIZE, "default direction = %d\n", obj->hw->direction);

	return len;
}

static ssize_t store_chip_orientation(struct device_driver *ddri, const char *buf, size_t tCount)
{
	int nDirection = 0;
	int res = 0;
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = kstrtoint(buf, 10, &nDirection);
	if (res != 0) {
		if (hwmsen_get_convert(nDirection, &obj->cvt))
			ACC_ERR("ERR: fail to set direction\n");
	}

	ACC_LOG("[%s] set direction: %d\n", __func__, nDirection);

	return tCount;
}

static ssize_t show_register_dump(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct icm206xx_accel_i2c_data *obj = obj_i2c_data;
	int i;
	u8 databuf[2] = { 0 };
	int res = 0;

	if (obj == NULL) {
		ACC_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	len += snprintf(buf + len, PAGE_SIZE, "Register Dump\n");

	for (i = 0; i < 0x7F; i++) {
		/* Don't read memory r/w register */
		if (i == ICM206XX_REG_MEM_R_W)
			databuf[0] = 0;
		else {
			res = icm206xx_share_i2c_read_register(i, databuf, 1);
			if (res < 0) {
				ACC_ERR("read register err!\n");
				return 0;
			}
		}
		len += snprintf(buf + len, PAGE_SIZE, "0x%02x: 0x%02x\n", i, databuf[0]);
	}

	return len;
}

#ifdef ICM206XX_SELFTEST
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = icm206xx_accel_i2c_client;

	if (NULL == client) {
		ACC_ERR("i2c client is null!!\n");
		return 0;
	}

	return snprintf(buf, 8, "%s\n", selftestRes);
}

static ssize_t store_selftest_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = icm206xx_accel_i2c_client;
	int num;
	int res = 0;

	/* check parameter values to run selftest */
	res = kstrtoint(buf, 10, &num);
	if (res != 0) {
		ACC_ERR("parse number fail\n");
		return count;
	} else if (num == 0) {
		ACC_ERR("invalid data count\n");
		return count;
	}

	/* run selftest */
	res = icm206xx_accel_InitSelfTest(client);

	if (icm206xx_accel_DoSelfTest(client) == true) {
		strcpy(selftestRes, "y");
		ACC_LOG("ACCEL SELFTEST : PASS\n");
	} else {
		strcpy(selftestRes, "n");
		ACC_LOG("ACCEL SELFTEST : FAIL\n");
	}

	/* Taly!!! : Selftest is considered to be called only in factory mode */
	/* In general mode, the condition before selftest will not be recovered and sensor will not be in sleep mode */
	res = icm206xx_accel_init_client(client, true);

	return count;
}
#endif

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);
static DRIVER_ATTR(regdump, S_IRUGO, show_register_dump, NULL);
#ifdef ICM206XX_SELFTEST
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO, show_selftest_value, store_selftest_value);
#endif

static struct driver_attribute *icm206xx_accel_attr_list[] = {
	&driver_attr_chipinfo,	/* chip information */
	&driver_attr_sensordata,	/* dump sensor data */
	&driver_attr_trace,	/* trace log */
	&driver_attr_status,	/* chip status */
	&driver_attr_orientation,	/* chip orientation information */
	&driver_attr_regdump,	/* register dump */
#ifdef ICM206XX_SELFTEST
	&driver_attr_selftest,	/* run selftest when store, report selftest result when show */
#endif
};

/*----------------------------------------------------------------------------*/
static int icm206xx_accel_create_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_accel_attr_list) / sizeof(icm206xx_accel_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		res = driver_create_file(driver, icm206xx_accel_attr_list[idx]);
		if (0 != res) {
			ACC_ERR("driver_create_file (%s) = %d\n", icm206xx_accel_attr_list[idx]->attr.name, res);
			break;
		}
	}
	return res;
}

static int icm206xx_accel_delete_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_accel_attr_list) / sizeof(icm206xx_accel_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, icm206xx_accel_attr_list[idx]);

	return res;
}

/*=======================================================================================*/
/* Misc - Factory Mode (IOCTL) Device Driver Section					 */
/*=======================================================================================*/

static int icm206xx_accel_open(struct inode *inode, struct file *file)
{
	file->private_data = icm206xx_accel_i2c_client;

	if (file->private_data == NULL) {
		ACC_ERR("null pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}

static int icm206xx_accel_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long icm206xx_accel_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct icm206xx_accel_i2c_data *obj = (struct icm206xx_accel_i2c_data *)i2c_get_clientdata(client);
	char strbuf[ICM206XX_BUFSIZE] = { 0 };
	void __user *data;
	long res = 0;
	struct SENSOR_DATA sensor_data;
	static struct GSENSOR_VECTOR3D gsensor_gain;

	if (_IOC_DIR(cmd) & _IOC_READ)
		res = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		res = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (res) {
		ACC_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	addCmdLog(cmd);
	printCmdLog();

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		icm206xx_share_ChipSoftReset();
		icm206xx_accel_init_client(client, true);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_share_ReadChipInfo(strbuf, ICM206XX_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			res = -EINVAL;
			break;
		}

		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_accel_ReadSensorData(client, strbuf, ICM206XX_BUFSIZE);
		if (copy_to_user(data, strbuf, sizeof(strbuf))) {
			res = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_OFFSET:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_accel_ReadOffsetData(client, strbuf);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			res = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_accel_ReadGain(client, &gsensor_gain);
		if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
			res = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_accel_ReadRawData(client, strbuf);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			res = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_SET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
			res = -EFAULT;
			break;
		}

		if (atomic_read(&obj->suspend)) {
			ACC_ERR("Perform calibration in suspend state!!\n");
			res = -EINVAL;
		} else {
			ACC_LOG("accel set cali:[%5d %5d %5d]\n", sensor_data.x, sensor_data.y, sensor_data.z);

			res = icm206xx_accel_WriteCalibration(client, &sensor_data);
		}
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}
		res = icm206xx_accel_ReadCalibration(client, &sensor_data);
		if (res)
			break;

		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			res = -EFAULT;
			break;
		}

		break;

	case GSENSOR_IOCTL_CLR_CALI:
		res = icm206xx_accel_ResetCalibration(client);
		break;

	default:
		ACC_ERR("unknown IOCTL: 0x%08x\n", cmd);
		res = -ENOIOCTLCMD;
	}

	return res;
}

#ifdef CONFIG_COMPAT
static long icm206xx_accel_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long res = 0;
	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		ACC_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}

	switch (cmd) {
	case COMPAT_GSENSOR_IOCTL_INIT:
	case COMPAT_GSENSOR_IOCTL_READ_CHIPINFO:
	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
	case COMPAT_GSENSOR_IOCTL_READ_OFFSET:
	case COMPAT_GSENSOR_IOCTL_READ_GAIN:
	case COMPAT_GSENSOR_IOCTL_READ_RAW_DATA:
	case COMPAT_GSENSOR_IOCTL_SET_CALI:
	case COMPAT_GSENSOR_IOCTL_GET_CALI:
	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			ACC_ERR("invalid argument.");
			return -EINVAL;
		}

		res = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
		break;

	default:
		ACC_ERR("%s not supported = 0x%04x", __func__, cmd);
		return -ENOIOCTLCMD;
	}

	return res;
}
#endif

static const struct file_operations icm206xx_accel_fops = {
	.open = icm206xx_accel_open,
	.release = icm206xx_accel_release,
	.unlocked_ioctl = icm206xx_accel_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = icm206xx_accel_compat_ioctl,
#endif
};

static struct miscdevice icm206xx_accel_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &icm206xx_accel_fops,
};

/*=======================================================================================*/
/* Misc - I2C HAL Support Section							 */
/*=======================================================================================*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int icm206xx_accel_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int icm206xx_accel_enable_nodata(int en)
{
	int res = 0;
	bool power = false;

	if (1 == en) {
		power = true;

	}
	if (0 == en) {
		power = false;
		icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_ACC, 0, false);
	}

	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_ACC, power);
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_ACC, power);

	if (res != ICM206XX_SUCCESS) {
		ACC_LOG("fail!\n");
		return -1;
	}

	ACC_LOG("icm206xx_accel_enable_nodata OK!\n");

	return 0;
}

static int icm206xx_accel_set_delay(u64 ns)
{
	ACC_LOG("%s is called [ns:%lld]\n", __func__, ns);

	icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_ACC, ns, false);

	return 0;
}

static int icm206xx_accel_get_data(int *x, int *y, int *z, int *status)
{
	char buff[ICM206XX_BUFSIZE];
	int res = 0;

	icm206xx_accel_ReadSensorData(obj_i2c_data->client, buff, ICM206XX_BUFSIZE);

	res = sscanf(buff, "%x %x %x", x, y, z);

	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

/*=======================================================================================*/
/* I2C Device Driver Section								 */
/*=======================================================================================*/

static const struct i2c_device_id icm206xx_accel_i2c_id[] = { {ICM206XX_ACCEL_DEV_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif

static int icm206xx_accel_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct icm206xx_accel_i2c_data *obj;
	struct acc_control_path ctl = { 0 };
	struct acc_data_path data = { 0 };
	int res = 0;

	ACC_LOG();
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		res = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct icm206xx_accel_i2c_data));

	/* hwmsen_get_convert() depend on the direction value */
	obj->hw = hw;
	res = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (res) {
		ACC_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

	icm206xx_accel_i2c_client = new_client;

	/* Soft reset is called only in accelerometer init */
	/* Do not call soft reset in gyro and step_count init */
	res = icm206xx_share_ChipSoftReset();
	if (res != ICM206XX_SUCCESS)
		goto exit_init_failed;

	res = icm206xx_accel_init_client(new_client, false);
	if (res)
		goto exit_init_failed;

	/* misc_register() for factory mode, engineer mode and so on */
	res = misc_register(&icm206xx_accel_device);
	if (res) {
		ACC_ERR("icm206xx_accel_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}

	/* Crate platform_driver attribute */
	res = icm206xx_accel_create_attr(&(icm206xx_accel_init_info.platform_diver_addr->driver));
	if (res) {
		ACC_ERR("icm206xx_accel create attribute err = %d\n", res);
		goto exit_create_attr_failed;
	}

	/*-----------------------------------------------------------*/
	/* Fill and Register the acc_control_path and acc_data_path */
	/*-----------------------------------------------------------*/

	/* Fill the acc_control_path */
	ctl.is_use_common_factory = false;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	ctl.open_report_data = icm206xx_accel_open_report_data;	/* function pointer */
	ctl.enable_nodata = icm206xx_accel_enable_nodata;	/* function pointer */
	ctl.set_delay = icm206xx_accel_set_delay;	/* function pointer */

	/* Register the acc_control_path */
	res = acc_register_control_path(&ctl);
	if (res) {
		ACC_ERR("register accel control path err\n");
		goto exit_kfree;
	}

	/* Fill the acc_data_path */
	data.get_data = icm206xx_accel_get_data;	/* function pointer */
	data.vender_div = 1000;

	/* Register the acc_data_path */
	res = acc_register_data_path(&data);
	if (res) {
		ACC_ERR("register accel_data_path fail = %d\n", res);
		goto exit_kfree;
	}

	/* Set init_flag = 0 and return */
	icm206xx_accel_init_flag = 0;

	ACC_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&icm206xx_accel_device);
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
exit:
	kfree(obj);
	obj = NULL;
	icm206xx_accel_init_flag = -1;
	ACC_ERR("%s: err = %d\n", __func__, res);
	return res;
}

static int icm206xx_accel_i2c_remove(struct i2c_client *client)
{
	int res = 0;

	res = icm206xx_accel_delete_attr(&(icm206xx_accel_init_info.platform_diver_addr->driver));
	if (res)
		ACC_ERR("icm206xx_accel_delete_attr fail: %d\n", res);

	res = misc_deregister(&icm206xx_accel_device);
	if (res)
		ACC_ERR("misc_deregister fail: %d\n", res);

	res = hwmsen_detach(ID_ACCELEROMETER);
	if (res)
		ACC_ERR("hwmsen_detach fail: %d\n", res);

	icm206xx_accel_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static int icm206xx_accel_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, ICM206XX_ACCEL_DEV_NAME);
	return 0;
}

static int icm206xx_accel_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	int res = 0;
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			ACC_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);

		res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_ACC, false);
		if (res < 0) {
			ACC_ERR("write power control fail!\n");
			return res;
		}

		icm206xx_accel_power(obj->hw, 0);
		ACC_LOG("icm206xx_accel suspend ok\n");

	}
	return res;
}

static int icm206xx_accel_i2c_resume(struct i2c_client *client)
{
	struct icm206xx_accel_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	if (obj == NULL) {
		ACC_ERR("null pointer!!\n");
		return -EINVAL;
	}

	icm206xx_accel_power(obj->hw, 1);
	res = icm206xx_accel_init_client(client, false);

	if (res) {
		ACC_ERR("initialize client fail!!\n");
		return res;
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}

static struct i2c_driver icm206xx_accel_i2c_driver = {
	.driver = {
		   .name = ICM206XX_ACCEL_DEV_NAME,
#ifdef CONFIG_OF
		   .of_match_table = accel_of_match,
#endif
		   },
	.probe = icm206xx_accel_i2c_probe,
	.remove = icm206xx_accel_i2c_remove,
	.detect = icm206xx_accel_i2c_detect,
	.suspend = icm206xx_accel_i2c_suspend,
	.resume = icm206xx_accel_i2c_resume,
	.id_table = icm206xx_accel_i2c_id,
};

/*=======================================================================================*/
/* Kernel Module Section								 */
/*=======================================================================================*/

static int icm206xx_accel_remove(void)
{
	icm206xx_accel_power(hw, 0);

	i2c_del_driver(&icm206xx_accel_i2c_driver);

	return 0;
}

static int icm206xx_accel_local_init(void)
{
	icm206xx_accel_power(hw, 1);

	initDebugData();

	if (i2c_add_driver(&icm206xx_accel_i2c_driver)) {
		ACC_ERR("add driver error\n");
		return -1;
	}

	if (-1 == icm206xx_accel_init_flag)
		return -1;
	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init icm206xx_accel_init(void)
{
	const char *name = "mediatek,icm206xx_accel";

	hw = get_accel_dts_func(name, hw);
	if (!hw)
		ACC_ERR("get dts info fail\n");

	acc_driver_add(&icm206xx_accel_init_info);

	return 0;
}

static void __exit icm206xx_accel_exit(void)
{

}

/*----------------------------------------------------------------------------*/
module_init(icm206xx_accel_init);
module_exit(icm206xx_accel_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("icm206xx accelerometer driver");

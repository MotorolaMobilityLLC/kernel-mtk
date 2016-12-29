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
#include "cust_gyro.h"
#include "gyroscope.h"
#include "../../accelerometer/icm206xx_a/icm206xx_register_20608D.h"
#include "../../accelerometer/icm206xx_a/icm206xx_share.h"
#include "icm206xx_gyro.h"

#define ICM206XX_GYRO_DEV_NAME		"ICM206XX_GYRO"

struct icm206xx_gyro_i2c_data {
	struct i2c_client	*client;
	struct gyro_hw		*hw;
	struct hwmsen_convert   cvt;

	/*misc*/
	atomic_t		trace;
	atomic_t		suspend;
	atomic_t		selftest;

	/*data*/
	s16			cali_sw[ICM206XX_AXIS_NUM+1];
	s16			data[ICM206XX_AXIS_NUM+1];
};

static int icm206xx_gyro_init_flag =  -1;

static struct i2c_client *icm206xx_gyro_i2c_client;
static struct icm206xx_gyro_i2c_data *obj_i2c_data;

#ifdef ICM206XX_SELFTEST
static char selftestRes[8] = { 0 };
#endif

static int g_icm206xx_gyro_sensitivity = ICM206XX_GYRO_DEFAULT_SENSITIVITY;	/* +/-1000DPS as default */

struct gyro_hw gyro_cust;
static struct gyro_hw *hw = &gyro_cust;

static int icm206xx_gyro_local_init(struct platform_device *pdev);
static int icm206xx_gyro_remove(void);
static struct gyro_init_info icm206xx_gyro_init_info = {
	.name = ICM206XX_GYRO_DEV_NAME,
	.init = icm206xx_gyro_local_init,
	.uninit = icm206xx_gyro_remove,
};

/*=======================================================================================*/
/* I2C Primitive Functions Section							 */
/*=======================================================================================*/

/* Share i2c function with Accelerometer		*/
/* Function is defined in accelerometer/icm206xx_a	*/

/*=======================================================================================*/
/* Vendor Specific Functions Section							 */
/*=======================================================================================*/

static int icm206xx_gyro_SetFullScale(struct i2c_client *client, u8 gyro_fsr)
{
	u8 databuf[2] = {0};
	int res = 0;

	if (gyro_fsr < ICM206XX_GYRO_RANGE_250DPS || gyro_fsr > ICM206XX_GYRO_RANGE_2000DPS) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_GYRO_CFG, databuf, 1);
	if (res < 0) {
		GYRO_ERR("read fsr register err!\n");
		return ICM206XX_ERR_I2C;
	}

	databuf[0] &= ~ICM206XX_BIT_GYRO_FSR;		/* clear FSR bit */
	databuf[0] = gyro_fsr << ICM206XX_GYRO_FS_SEL;

	g_icm206xx_gyro_sensitivity = (ICM206XX_GYRO_MAX_SENSITIVITY >> gyro_fsr) + 1;

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_GYRO_CFG, databuf, 1);
	if (res < 0) {
		GYRO_ERR("write fsr register err!\n");
		return ICM206XX_ERR_I2C;
	}

	return ICM206XX_SUCCESS;
}


static int icm206xx_gyro_SetFilter(struct i2c_client *client, u8 gyro_filter)
{
	u8 databuf[2] = {0};
	int res = 0;

	if (gyro_filter < ICM206XX_GYRO_DLPF_0 || gyro_filter > ICM206XX_GYRO_DLPF_7) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_CFG, databuf, 1);
	if (res < 0) {
		GYRO_ERR("read filter register err!\n");
		return ICM206XX_ERR_I2C;
	}

	databuf[0] &= ~ICM206XX_BIT_GYRO_FILTER;	/* clear Filter bit */
	databuf[0] |= gyro_filter;

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_CFG, databuf, 1);
	res = 0;	/* Taly!!! : Remove this line on actual platform */
			/* i2c write return fail on MTK evb, but value is written properly */
	if (res < 0) {
		GYRO_ERR("write filter register err!\n");
		return ICM206XX_ERR_I2C;
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_gyro_ReadSensorDataDirect
(struct i2c_client *client, s16 data[ICM206XX_AXIS_NUM])
{
	char databuf[6];
	int i;
	int res = 0;

	if (client == NULL)
		return ICM206XX_ERR_INVALID_PARAM;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_GYRO_XH, databuf, ICM206XX_DATA_LEN);
	if (res < 0) {
		GYRO_ERR("ICM206XX read gyro data  error\n");
		return ICM206XX_ERR_I2C;
	}

	for (i = 0; i < ICM206XX_AXIS_NUM; i++)
		data[i] = ((s16)((databuf[i * 2] << 8) | (databuf[i * 2 + 1]))); /* convert 8-bit to 16-bit */

	return ICM206XX_SUCCESS;
}

static int icm206xx_gyro_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	char databuf[6];
	int  data[3];
	int  res = 0;
	int  i = 0;
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_GYRO_XH, databuf, ICM206XX_DATA_LEN);
	if (res < 0) {
		GYRO_ERR("read gyroscope data error\n");
		return ICM206XX_ERR_I2C;
	}

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/* convert 8-bit to 16-bit */
		obj->data[i] = ((s16)((databuf[i * 2] << 8) | (databuf[i * 2 + 1]))); /* DPS */

		/* Add Cali */
		obj->data[i] += obj->cali_sw[i];
	}

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/*Orientation Translation Lower (sensor) --> Upper (Device) */
		data[i] = obj->cvt.sign[i] * obj->data[obj->cvt.map[i]];

		/* 1. Apply SENSITIVITY_SCALE_FACTOR calculated from SetFullScale	(DPS) */
		/* 2. Translated it to MAX SENSITIVITY_SCALE_FACTOR			(DPS) */
		data[i] = data[i] * ICM206XX_GYRO_MAX_SENSITIVITY / g_icm206xx_gyro_sensitivity;
	}

	sprintf(buf, "%04x %04x %04x",	data[ICM206XX_AXIS_X],
					data[ICM206XX_AXIS_Y],
					data[ICM206XX_AXIS_Z]);

	if (atomic_read(&obj->trace)) {
		GYRO_LOG("Gyro Data - %04x %04x %04x\n",
					data[ICM206XX_AXIS_X],
					data[ICM206XX_AXIS_Y],
					data[ICM206XX_AXIS_Z]);
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_gyro_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	s16 data[ICM206XX_AXIS_NUM] = { 0, 0, 0 };

	res = icm206xx_gyro_ReadSensorDataDirect(client, data);
	if (res < 0) {
		GYRO_ERR("read gyroscope raw data  error\n");
		return ICM206XX_ERR_I2C;
	}

	/* Sensor Raw Data direct read from sensor register	*/
	/* No orientation translation, No unit Translation	*/
	sprintf(buf, "%04x %04x %04x",	data[ICM206XX_AXIS_X],
					data[ICM206XX_AXIS_Y],
					data[ICM206XX_AXIS_Z]);

	return ICM206XX_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int icm206xx_gyro_ResetCalibration(struct i2c_client *client)
{
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return ICM206XX_SUCCESS;
}

static int icm206xx_gyro_ReadCalibration(struct i2c_client *client, struct SENSOR_DATA *sensor_data)
{
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);
	int cali[ICM206XX_AXIS_NUM];
	int i;

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/* remap to device orientation */
		cali[i] = obj->cvt.sign[i]*obj->cali_sw[obj->cvt.map[i]];
	}

	sensor_data->x = cali[ICM206XX_AXIS_X] * ICM206XX_GYRO_MAX_SENSITIVITY / g_icm206xx_gyro_sensitivity;
	sensor_data->y = cali[ICM206XX_AXIS_Y] * ICM206XX_GYRO_MAX_SENSITIVITY / g_icm206xx_gyro_sensitivity;
	sensor_data->z = cali[ICM206XX_AXIS_Z] * ICM206XX_GYRO_MAX_SENSITIVITY / g_icm206xx_gyro_sensitivity;

	if (atomic_read(&obj->trace)) {
		GYRO_LOG("gyro ReadCalibration:[sensor_data:%5d %5d %5d][cali_sw:%5d %5d %5d]\n",
				sensor_data->x, sensor_data->y, sensor_data->z,
				obj->cali_sw[ICM206XX_AXIS_X],
				obj->cali_sw[ICM206XX_AXIS_Y],
				obj->cali_sw[ICM206XX_AXIS_Z]);
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_gyro_WriteCalibration(struct i2c_client *client, struct SENSOR_DATA *sensor_data)
{
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);
	int cali[ICM206XX_AXIS_NUM];
	int i;

	cali[ICM206XX_AXIS_X] = sensor_data->x * g_icm206xx_gyro_sensitivity / ICM206XX_GYRO_MAX_SENSITIVITY;
	cali[ICM206XX_AXIS_Y] = sensor_data->y * g_icm206xx_gyro_sensitivity / ICM206XX_GYRO_MAX_SENSITIVITY;
	cali[ICM206XX_AXIS_Z] = sensor_data->z * g_icm206xx_gyro_sensitivity / ICM206XX_GYRO_MAX_SENSITIVITY;

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		/* read cali_sw with remap */
		/* (Orientation Translation : Upper (Device) Layer --> Lower (Chip) Layer) and sign information */
		obj->cali_sw[obj->cvt.map[i]] += obj->cvt.sign[i] * cali[i];
	}

	if (atomic_read(&obj->trace)) {
		GYRO_LOG("gyro writeCalibration:[sensor_data:%5d %5d %5d][cali_sw:%5d %5d %5d]\n",
				sensor_data->x, sensor_data->y, sensor_data->z,
				obj->cali_sw[ICM206XX_AXIS_X],
				obj->cali_sw[ICM206XX_AXIS_Y],
				obj->cali_sw[ICM206XX_AXIS_Z]);
	}

	return ICM206XX_SUCCESS;

}
/*----------------------------------------------------------------------------*/
#ifdef ICM206XX_SELFTEST
static int  icm206xx_gyro_InitSelfTest(struct i2c_client *client)
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
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_GYRO, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* fsr : ICM206XX_GYRO_RANGE_250DPS */
	res = icm206xx_gyro_SetFullScale(client, ICM206XX_GYRO_RANGE_250DPS);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* filter : ICM206XX_GYRO_DLPF_2 */
	res = icm206xx_gyro_SetFilter(client, ICM206XX_GYRO_DLPF_2);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* odr : 1000hz (1kHz) */
	res = icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_GYRO, 1000000, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set enable sensor */
	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_GYRO, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	return res;
}

static int icm206xx_gyro_CalcAvgWithSamples(struct i2c_client *client, int avg[3], int count)
{
	int res = 0;
	int i, nAxis;
	s16 sensor_data[ICM206XX_AXIS_NUM];
	s32 sum[ICM206XX_AXIS_NUM] = { 0, 0, 0 };

	for (i = 0; i < count; i++) {
		res = icm206xx_gyro_ReadSensorDataDirect(client, sensor_data);
		if (res) {
			GYRO_ERR("read data fail: %d\n", res);
			return ICM206XX_ERR_STATUS;
		}

		for (nAxis = 0; nAxis < ICM206XX_AXIS_NUM; nAxis++)
			sum[nAxis] += sensor_data[nAxis];

		/* Data register updated @1khz */
		mdelay(1);
	}

	for (nAxis = 0; nAxis < ICM206XX_AXIS_NUM; nAxis++)
		avg[nAxis] = (int)(sum[nAxis] / count) * ICM206XX_ST_PRECISION;

	return ICM206XX_SUCCESS;
}

static bool icm206xx_gyro_DoSelfTest(struct i2c_client *client)
{
	int res = 0;
	int i;
	int gyro_ST_on[ICM206XX_AXIS_NUM], gyro_ST_off[ICM206XX_AXIS_NUM];
	int gyro_ST_response[ICM206XX_AXIS_NUM];
	u8  st_code[ICM206XX_AXIS_NUM]; /* index of otp_lookup_tbl */
	u16 st_otp[ICM206XX_AXIS_NUM];
	bool otp_value_has_zero = false;
	bool test_result = true;
	u8 databuf[2] = {0};

	/* -------------------------------------------------------------------------------- */
	/* Acquire OTP value from OTP Lookup Table */
	/* -------------------------------------------------------------------------------- */

	/* read st_code */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_SELF_TEST_X_GYRO, &(st_code[0]), 3);
	if (res) {
		GYRO_ERR("read data fail: %d\n", res);
		return false;
	}

	GYRO_LOG("st_code: %02x, %02x, %02x\n", st_code[0], st_code[1], st_code[2]);

	/* lookup OTP value with st_code */
	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		if (st_code[i] != 0)
			st_otp[i] = st_otp_lookup_tbl[st_code[i] - 1];
		else{
			st_otp[i] = 0;
			otp_value_has_zero = true;
		}
	}

	/* -------------------------------------------------------------------------------- */
	/* Read sensor data and calculate average values from it */
	/* -------------------------------------------------------------------------------- */

	/* Read 200 Samples with selftest off */
	res = icm206xx_gyro_CalcAvgWithSamples(client, gyro_ST_off, ICM206XX_ST_SAMPLE_COUNT);
	if (res) {
		GYRO_ERR("read data fail: %d\n", res);
		return false;
	}

	/* set selftest on */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_GYRO_CFG, databuf, 1);
	databuf[0] |= ICM206XX_BIT_GYRO_SELFTEST;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_GYRO_CFG, databuf, 1);

	/* Wait 20ms for oscillations to stabilize */
	mdelay(20);

	/* Read 200 Samples with selftest on */
	res = icm206xx_gyro_CalcAvgWithSamples(client, gyro_ST_on, ICM206XX_ST_SAMPLE_COUNT);
	if (res) {
		GYRO_ERR("read data fail: %d\n", res);
		return false;
	}

	/* -------------------------------------------------------------------------------- */
	/* Compare calculated value with INVN OTP value to judge Success or Fail */
	/* -------------------------------------------------------------------------------- */

	for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
		gyro_ST_response[i] = abs(gyro_ST_on[i] - gyro_ST_off[i]);

		if (otp_value_has_zero == false) {
			GYRO_LOG("Selftest result A : [i:%d][%d][%d]\n",
				i,
				gyro_ST_response[i],
				(int)(st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_GYRO_DELTA / 100));

			if (gyro_ST_response[i] < (st_otp[i] * ICM206XX_ST_PRECISION * ICM206XX_ST_GYRO_DELTA / 100))
				test_result = false;
		} else{
			GYRO_LOG("Selftest result B: [i:%d][%d][%d]\n",
				i,
				gyro_ST_response[i],
				(int)(ICM206XX_ST_GYRO_AL * 32768 / 250 * ICM206XX_ST_PRECISION));

			if (gyro_ST_response[i] < (ICM206XX_ST_GYRO_AL * 32768 / 250 * ICM206XX_ST_PRECISION))
				test_result = false;
		}
	}

	if (test_result == true) {
		for (i = 0; i < ICM206XX_AXIS_NUM; i++) {
			GYRO_LOG("Selftest result C: [i:%d][%d][%d]\n",
				i,
				(int)abs(gyro_ST_off[i]),
				(int)(ICM206XX_ST_GYRO_OFFSET_MAX * 32768 / 250 * ICM206XX_ST_PRECISION));

			if (abs(gyro_ST_off[i]) > (ICM206XX_ST_GYRO_OFFSET_MAX * 32768 / 250 * ICM206XX_ST_PRECISION))
				test_result = false;
		}
	}

	GYRO_LOG("Selftest result : %d\n", test_result);

	return test_result;
}
#endif
/*----------------------------------------------------------------------------*/
static int icm206xx_gyro_init_client(struct i2c_client *client, bool enable)
{
	int res = 0;

	/* Exit sleep mode */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_GYRO, true);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set fsr +/-1000 dps as default */
	res = icm206xx_gyro_SetFullScale(client, ICM206XX_GYRO_RANGE_1000DPS);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set filter ICM206XX_GYRO_DLPF_1 as default */
	res = icm206xx_gyro_SetFilter(client, ICM206XX_GYRO_DLPF_1);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set 5ms(200hz) sample rate */
	res = icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_GYRO, 5000000, false);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Disable sensor - standby mode for gyroscope */
	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_GYRO, enable);
	if (res != ICM206XX_SUCCESS)
		return res;

	/* Set power mode - sleep or normal */
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_GYRO, enable);
	if (res != ICM206XX_SUCCESS)
		return res;

	GYRO_LOG("icm206xx_gyro_init_client OK!\n");

	return ICM206XX_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/* For  driver get cust info */
struct gyro_hw *get_cust_gyro(void)
{
	return &gyro_cust;
}

/* gyroscope power control function */
static void icm206xx_gyro_power(struct gyro_hw *hw, unsigned int on)
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
	char strbuf[ICM206XX_BUFSIZE];
	struct i2c_client *client = icm206xx_gyro_i2c_client;

	if (NULL == client) {
		GYRO_ERR("i2c client is null!!\n");
		return 0;
	}

	icm206xx_gyro_ReadSensorData(client, strbuf, ICM206XX_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	struct icm206xx_gyro_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct icm206xx_gyro_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (0 == kstrtoint(buf, 16, &trace))
		atomic_set(&obj->trace, trace);
	else
		GYRO_ERR("invalid content: '%s', length = %zu\n", buf, count);


	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct icm206xx_gyro_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw)
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n",
			obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	return len;
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct icm206xx_gyro_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	GYRO_LOG("[%s] default direction: %d\n", __func__, obj->hw->direction);

	len = snprintf(buf, PAGE_SIZE, "default direction = %d\n", obj->hw->direction);

	return len;
}

static ssize_t store_chip_orientation(struct device_driver *ddri, const char *buf, size_t tCount)
{
	int nDirection = 0;
	int res = 0;
	struct icm206xx_gyro_i2c_data   *obj = obj_i2c_data;

	if (obj == NULL) {
		GYRO_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = kstrtoint(buf, 10, &nDirection);
	if (res != 0) {
		if (hwmsen_get_convert(nDirection, &obj->cvt))
			GYRO_ERR("ERR: fail to set direction\n");
	}

	GYRO_LOG("[%s] set direction: %d\n", __func__, nDirection);

	return tCount;
}

#ifdef ICM206XX_SELFTEST
static ssize_t show_selftest_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = icm206xx_gyro_i2c_client;

	if (NULL == client) {
		GYRO_ERR("i2c client is null!!\n");
		return 0;
	}

	return snprintf(buf, 8, "%s\n", selftestRes);
}

static ssize_t store_selftest_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = icm206xx_gyro_i2c_client;
	int num;
	int res = 0;

	/* check parameter values to run selftest */
	res = kstrtoint(buf, 10, &num);
	if (res != 0) {
		GYRO_ERR("parse number fail\n");
		return count;
	} else if (num == 0) {
		GYRO_ERR("invalid data count\n");
		return count;
	}

	/* run selftest */
	res = icm206xx_gyro_InitSelfTest(client);

	if (icm206xx_gyro_DoSelfTest(client) == true) {
		strcpy(selftestRes, "y");
		GYRO_LOG("GYRO SELFTEST : PASS\n");
	} else{
		strcpy(selftestRes, "n");
		GYRO_LOG("GYRO SELFTEST : FAIL\n");
	}

	/* Taly!!! : Selftest is considered to be called only in factory mode */
	/* In general mode, the condition before selftest will not be recovered and sensor will not be in sleep mode */
	res = icm206xx_gyro_init_client(client, true);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(trace,  S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status,  S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO, show_chip_orientation, store_chip_orientation);
#ifdef ICM206XX_SELFTEST
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO, show_selftest_value, store_selftest_value);
#endif

static struct driver_attribute *icm206xx_gyro_attr_list[] = {
	&driver_attr_chipinfo,			/* chip information - whoami */
	&driver_attr_sensordata,		/* dump sensor data */
	&driver_attr_trace,			/* trace log */
	&driver_attr_status,			/* chip status */
	&driver_attr_orientation,		/* chip orientation information */
#ifdef ICM206XX_SELFTEST
	&driver_attr_selftest,			/* run selftest when store, report selftest result when show */
#endif
};
/*----------------------------------------------------------------------------*/
static int icm206xx_gyro_create_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_gyro_attr_list)/sizeof(icm206xx_gyro_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		res = driver_create_file(driver, icm206xx_gyro_attr_list[idx]);
		if (0 != res) {
			GYRO_ERR("driver_create_file (%s) = %d\n", icm206xx_gyro_attr_list[idx]->attr.name, res);
			break;
		}
	}
	return res;
}

static int icm206xx_gyro_delete_attr(struct device_driver *driver)
{
	int idx;
	int res = 0;
	int num = (int)(sizeof(icm206xx_gyro_attr_list)/sizeof(icm206xx_gyro_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, icm206xx_gyro_attr_list[idx]);

	return res;
}

/*=======================================================================================*/
/* Misc - Factory Mode (IOCTL) Device Driver Section					 */
/*=======================================================================================*/

static int icm206xx_gyro_open(struct inode *inode, struct file *file)
{
	file->private_data = icm206xx_gyro_i2c_client;

	if (file->private_data == NULL) {
		GYRO_ERR("null pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}

static int icm206xx_gyro_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long icm206xx_gyro_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	char strbuf[ICM206XX_BUFSIZE] = {0};
	void __user *data;
	long res = 0;
	int copy_cnt = 0;
	struct SENSOR_DATA sensor_data;
	int smtRes = 0;

	if (_IOC_DIR(cmd) & _IOC_READ)
		res = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		res = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (res) {
		GYRO_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GYROSCOPE_IOCTL_INIT:
		icm206xx_gyro_init_client(client, true);
		break;

	case GYROSCOPE_IOCTL_SMT_DATA:
		data = (void __user *) arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		GYRO_LOG("ioctl smtRes: %d!\n", smtRes);
		copy_cnt = copy_to_user(data, &smtRes,  sizeof(smtRes));

		if (copy_cnt) {
			res = -EFAULT;
			GYRO_ERR("copy gyro data to user failed!\n");
		}
		GYRO_LOG("copy gyro data to user OK: %d!\n", copy_cnt);
		break;

	case GYROSCOPE_IOCTL_READ_SENSORDATA:
		data = (void __user *) arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_gyro_ReadSensorData(client, strbuf, ICM206XX_BUFSIZE);
		if (copy_to_user(data, strbuf, sizeof(strbuf))) {
			res = -EFAULT;
			break;
		}
		break;

	case GYROSCOPE_IOCTL_SET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}
		if (copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			res = -EFAULT;
		else {
			GYRO_LOG("gyro set cali:[%5d %5d %5d]\n",
				sensor_data.x, sensor_data.y, sensor_data.z);

			res = icm206xx_gyro_WriteCalibration(client, &sensor_data);
		}
		break;

	case GYROSCOPE_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}
		res = icm206xx_gyro_ReadCalibration(client, &sensor_data);

		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			res = -EFAULT;
			break;
		}
		break;

	case GYROSCOPE_IOCTL_CLR_CALI:
		res = icm206xx_gyro_ResetCalibration(client);
		break;

	case GYROSCOPE_IOCTL_READ_SENSORDATA_RAW:
		data = (void __user *)arg;
		if (data == NULL) {
			res = -EINVAL;
			break;
		}

		icm206xx_gyro_ReadRawData(client, strbuf);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			res = -EFAULT;
			break;
		}
		break;
/*	case GYROSCOPE_IOCTL_READ_TEMPERATURE:		*/
/*	case GYROSCOPE_IOCTL_GET_POWER_STATUS:		*/

	default:
		GYRO_ERR("unknown IOCTL: 0x%08x\n", cmd);
		res = -ENOIOCTLCMD;
	}

	return res;
}

#ifdef CONFIG_COMPAT
static long icm206xx_gyro_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long res = 0;
	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl) {
		GYRO_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}

	switch (cmd) {
	case COMPAT_GYROSCOPE_IOCTL_INIT:
	case COMPAT_GYROSCOPE_IOCTL_SMT_DATA:
	case COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA:
	case COMPAT_GYROSCOPE_IOCTL_SET_CALI:
	case COMPAT_GYROSCOPE_IOCTL_GET_CALI:
	case COMPAT_GYROSCOPE_IOCTL_CLR_CALI:
	case COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA_RAW:
	case COMPAT_GYROSCOPE_IOCTL_READ_TEMPERATURE:
	case COMPAT_GYROSCOPE_IOCTL_GET_POWER_STATUS:
		if (arg32 == NULL) {
			GYRO_ERR("invalid argument.");
			return -EINVAL;
		}

		res = file->f_op->unlocked_ioctl(file, cmd, (unsigned long)arg32);
		break;
	default:
		GYRO_ERR("%s not supported = 0x%04x\n", __func__, cmd);
		res = -ENOIOCTLCMD;
		break;
	}
	return res;
}
#endif

static const struct file_operations icm206xx_gyro_fops = {
	.open		= icm206xx_gyro_open,
	.release	= icm206xx_gyro_release,
	.unlocked_ioctl	= icm206xx_gyro_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= icm206xx_gyro_compat_ioctl,
#endif
};

static struct miscdevice icm206xx_gyro_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "gyroscope",
	.fops		= &icm206xx_gyro_fops,
};

/*=======================================================================================*/
/* Misc - I2C HAL Support Section							 */
/*=======================================================================================*/

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int icm206xx_gyro_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */
static int icm206xx_gyro_enable_nodata(int en)
{
	int res = 0;
	bool power = false;

	if (1 == en)
		power = true;
	else{
		power = false;
		icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_GYRO, 0, false);
	}

	res = icm206xx_share_EnableSensor(ICM206XX_SENSOR_TYPE_GYRO, power);
	res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_GYRO, power);

	if (res != ICM206XX_SUCCESS) {
		GYRO_ERR("icm206xx_gyro_SetPowerMode fail!\n");
		return -1;
	}
	GYRO_LOG("icm206xx_gyro_enable_nodata OK!\n");

	return 0;
}

static int icm206xx_gyro_set_delay(u64 ns)
{
	icm206xx_share_SetSampleRate(ICM206XX_SENSOR_TYPE_GYRO, ns, false);

	return 0;
}

static int icm206xx_gyro_get_data(int *x , int *y, int *z, int *status)
{
	char buff[ICM206XX_BUFSIZE];
	int res = 0;

	icm206xx_gyro_ReadSensorData(obj_i2c_data->client, buff, ICM206XX_BUFSIZE); /* degree / second */
	res = sscanf(buff, "%x %x %x", x, y, z);

	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

/*=======================================================================================*/
/* I2C Device Driver Section								 */
/*=======================================================================================*/

static const struct i2c_device_id icm206xx_gyro_i2c_id[] = {{ICM206XX_GYRO_DEV_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id gyro_of_match[] = {
	{.compatible = "mediatek,gyro"},
	{},
};
#endif

static int icm206xx_gyro_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct icm206xx_gyro_i2c_data *obj;
	struct gyro_control_path ctl = {0};
	struct gyro_data_path data = {0};
	int res = 0;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		res = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct icm206xx_gyro_i2c_data));

	/* hwmsen_get_convert() depend on the direction value */
	obj->hw = hw;
	res = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (res) {
		GYRO_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	if (0 != obj->hw->addr) {
		client->addr = obj->hw->addr >> 1;
		GYRO_LOG("gyro_use_i2c_addr: %x\n", client->addr);
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

	icm206xx_gyro_i2c_client = new_client;

	res = icm206xx_gyro_init_client(new_client, false);
	if (res)
		goto exit_init_failed;

	/* misc_register() for factory mode, engineer mode and so on */
	res = misc_register(&icm206xx_gyro_device);
	if (res) {
		GYRO_ERR("icm206xx_gyro_device misc register failed!\n");
		goto exit_misc_device_register_failed;
	}

	res = icm206xx_gyro_create_attr(&(icm206xx_gyro_init_info.platform_diver_addr->driver));
	if (res) {
		GYRO_ERR("icm206xx_g create attribute err = %d\n", res);
		goto exit_create_attr_failed;
	}

	ctl.is_use_common_factory = false;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	ctl.open_report_data = icm206xx_gyro_open_report_data;
	ctl.enable_nodata = icm206xx_gyro_enable_nodata;
	ctl.set_delay  = icm206xx_gyro_set_delay;

	res = gyro_register_control_path(&ctl);
	if (res) {
		GYRO_ERR("register gyro control path err\n");
		goto exit_kfree;
	}

	data.get_data = icm206xx_gyro_get_data;
	data.vender_div = DEGREE_TO_RAD;

	res = gyro_register_data_path(&data);
	if (res) {
		GYRO_ERR("register gyro_data_path fail = %d\n", res);
		goto exit_kfree;
	}

	icm206xx_gyro_init_flag = 0;

	GYRO_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&icm206xx_gyro_device);
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
exit:
	kfree(obj);
	obj = NULL;
	icm206xx_gyro_init_flag =  -1;
	GYRO_ERR("%s: err = %d\n", __func__, res);
	return res;
}

static int icm206xx_gyro_i2c_remove(struct i2c_client *client)
{
	int res = 0;

	res = icm206xx_gyro_delete_attr(&(icm206xx_gyro_init_info.platform_diver_addr->driver));
	if (res)
		GYRO_ERR("icm206xx_gyro_delete_attr fail: %d\n", res);

	res = misc_deregister(&icm206xx_gyro_device);
	if (res)
		GYRO_ERR("misc_deregister fail: %d\n", res);

	icm206xx_gyro_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static int icm206xx_gyro_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, ICM206XX_GYRO_DEV_NAME);
	return 0;
}

static int icm206xx_gyro_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	int res = 0;
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GYRO_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);

		res = icm206xx_share_SetPowerMode(ICM206XX_SENSOR_TYPE_GYRO, false);
		if (res < 0)
			return res;

	}
	return res;
}

static int icm206xx_gyro_i2c_resume(struct i2c_client *client)
{
	struct icm206xx_gyro_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	if (obj == NULL) {
		GYRO_ERR("null pointer!!\n");
		return -EINVAL;
	}

	icm206xx_gyro_power(obj->hw, 1);
	res = icm206xx_gyro_init_client(client, false);
	if (res) {
		GYRO_ERR("initialize client fail!!\n");
		return res;
	}
	atomic_set(&obj->suspend, 0);

	return 0;
}

static struct i2c_driver icm206xx_gyro_i2c_driver = {
	.driver = {
	.name		   = ICM206XX_GYRO_DEV_NAME,
#ifdef CONFIG_OF
	.of_match_table = gyro_of_match,
#endif
	},
	.probe			= icm206xx_gyro_i2c_probe,
	.remove			= icm206xx_gyro_i2c_remove,
	.detect			= icm206xx_gyro_i2c_detect,
	.suspend		= icm206xx_gyro_i2c_suspend,
	.resume			= icm206xx_gyro_i2c_resume,
	.id_table		= icm206xx_gyro_i2c_id,
};

/*=======================================================================================*/
/* Kernel Module Section								 */
/*=======================================================================================*/

static int icm206xx_gyro_remove(void)
{
	icm206xx_gyro_power(hw, 0);

	i2c_del_driver(&icm206xx_gyro_i2c_driver);

	return 0;
}

static int icm206xx_gyro_local_init(struct platform_device *pdev)
{
	icm206xx_gyro_power(hw, 1);

	if (i2c_add_driver(&icm206xx_gyro_i2c_driver)) {
		GYRO_ERR("add driver error\n");
		return -1;
	}

	if (-1 == icm206xx_gyro_init_flag)
		return -1;
	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init icm206xx_gyro_init(void)
{
	const char *name = "mediatek,icm206xx_gyro";

	hw = get_gyro_dts_func(name, hw);
	if (!hw)
		GYRO_ERR("get dts info fail\n");

	gyro_driver_add(&icm206xx_gyro_init_info);

	return 0;
}

static void __exit icm206xx_gyro_exit(void)
{

}

/*----------------------------------------------------------------------------*/
module_init(icm206xx_gyro_init);
module_exit(icm206xx_gyro_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("icm206xx gyroscope driver");

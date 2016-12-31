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

#include <hwmsensor.h>
#include "cust_acc.h"
#include "accel.h"
#include "icm206xx_register_20608D.h"
#include "icm206xx_share.h"

/*=======================================================================================*/
/* Shared I2C Primitive Functions Section						 */
/*=======================================================================================*/

extern struct i2c_client *icm206xx_accel_i2c_client;

static DEFINE_MUTEX(icm206xx_accel_i2c_mutex);

static int icm206xx_i2c_read_register(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	int res = 0;
	struct i2c_msg msgs[2] = {{0}, {0} };

	mutex_lock(&icm206xx_accel_i2c_mutex);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client) {
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	} else if (len > C_I2C_FIFO_SIZE) {
		ACC_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	}
	res = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (res != 2) {
		ACC_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, res);
		res = -EIO;
	} else
		res = 0;

	mutex_unlock(&icm206xx_accel_i2c_mutex);
	return res;
}

static int icm206xx_i2c_write_register(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{   /*because address also occupies one byte, the maximum length for write is 7 bytes*/
	int idx, num;
	int res = 0;
	char buf[C_I2C_FIFO_SIZE];

	mutex_lock(&icm206xx_accel_i2c_mutex);
	if (!client) {
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	} else if (len >= C_I2C_FIFO_SIZE) {
		ACC_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	}

	num = 0;
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	res = i2c_master_send(client, buf, num);
	if (res < 0) {
		ACC_ERR("send command error!!\n");
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EFAULT;
	} else
		res = 0;
	mutex_unlock(&icm206xx_accel_i2c_mutex);

	return res;
}

static int icm206xx_i2c_write_memory(struct i2c_client *client, u16 mem_addr,
						u32 len, u8 const *data)
{
	u8 bank[2];
	u8 addr[2];
	u8 buf[513];

	struct i2c_msg msgs[3];
	int res = 0;

	if (len >= (sizeof(buf) - 1))
		return -ENOMEM;

	mutex_lock(&icm206xx_accel_i2c_mutex);

	bank[0] = ICM206XX_REG_MEM_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = ICM206XX_REG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf[0] = ICM206XX_REG_MEM_R_W;
	memcpy(buf + 1, data, len);

	/* write message */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = client->addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = client->addr;
	msgs[2].flags = 0;
	msgs[2].buf = (u8 *) buf;
	msgs[2].len = len + 1;

	if (!client) {
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	}

	res = i2c_transfer(client->adapter, msgs, 3);
	if (res != 3) {
		ACC_ERR("i2c_transfer error: (%d %p %d) %d\n", client->addr, data, len, res);
		res = -EIO;
	} else
		res = 0;

	mutex_unlock(&icm206xx_accel_i2c_mutex);

	return res;
}

static int icm206xx_i2c_read_memory(struct i2c_client *client, u16 mem_addr,
						u32 len, u8 *data)
{
	u8 bank[2];
	u8 addr[2];
	u8 buf;

	struct i2c_msg msgs[4];
	int res = 0;

	mutex_lock(&icm206xx_accel_i2c_mutex);

	bank[0] = ICM206XX_REG_MEM_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = ICM206XX_REG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf = ICM206XX_REG_MEM_R_W;

	/* write message */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = client->addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = client->addr;
	msgs[2].flags = 0;
	msgs[2].buf = &buf;
	msgs[2].len = 1;

	msgs[3].addr = client->addr;
	msgs[3].flags = I2C_M_RD;
	msgs[3].buf = data;
	msgs[3].len = len;

	if (!client) {
		mutex_unlock(&icm206xx_accel_i2c_mutex);
		return -EINVAL;
	}

	res = i2c_transfer(client->adapter, msgs, 4);
	if (res != 4) {
		ACC_ERR("i2c_transfer error: (%d %p %d) %d\n", client->addr, data, len, res);
		res = -EIO;
	} else
		res = 0;

	mutex_unlock(&icm206xx_accel_i2c_mutex);

	return res;
}

/*=======================================================================================*/
/* Shared Vendor Specific Functions Section						 */
/*=======================================================================================*/

struct icm206xx_sensor_status_info {
	bool sensor_power;
	int sample_rate;
};

static struct icm206xx_sensor_status_info icm206xx_all_sensor_status_info[ICM206XX_SENSOR_TYPE_MAX];

static int icm206xx_ChipSoftReset(struct i2c_client *client)
{
	u8 databuf[10];
	int res = 0;
	int i;

	memset(databuf, 0, sizeof(u8) * 10);

	/* read */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_PWR_CTL, databuf, 0x1);
	if (res < 0) {
		ACC_ERR("read power ctl register err!\n");
		return ICM206XX_ERR_I2C;
	}

	/* set device_reset bit to do soft reset */
	databuf[0] |= ICM206XX_BIT_DEVICE_RESET;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_PWR_CTL, databuf, 0x1);
	if (res < 0) {
		ACC_ERR("write power ctl register err!\n");
		return ICM206XX_ERR_I2C;
	}

	mdelay(100);

	/* sensor status rset */
	for (i = 0; i < ICM206XX_SENSOR_TYPE_MAX; i++) {
		icm206xx_all_sensor_status_info[i].sensor_power = false;
		icm206xx_all_sensor_status_info[i].sample_rate = 0;
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_SetPowerMode(struct i2c_client *client, int sensor_type, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0;
	int i;

	if (sensor_type >= ICM206XX_SENSOR_TYPE_MAX) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	icm206xx_all_sensor_status_info[sensor_type].sensor_power = enable;

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_PWR_CTL, databuf, 1);
	if (res < 0) {
		ACC_ERR("read power ctl register err!\n");
		return ICM206XX_ERR_I2C;
	}

	databuf[0] &= ~ICM206XX_BIT_SLEEP;
	if (enable == false) {
		/* Check power status of all sensors */
		for (i = 0; i < ICM206XX_SENSOR_TYPE_MAX; i++) {
			if (icm206xx_all_sensor_status_info[i].sensor_power == true)
				break;
		}

		/* Go to sleep mode when all sensors are disabled */
		if (i == ICM206XX_SENSOR_TYPE_MAX) {
			databuf[0] |= ICM206XX_BIT_SLEEP;
		}
	}

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_PWR_CTL, databuf, 1);
	if (res < 0) {
		ACC_ERR("set power mode failed!\n");
		return ICM206XX_ERR_I2C;
	}

	mdelay(5);

	ACC_LOG("set power mode ok %d!\n", enable);

	return ICM206XX_SUCCESS;
}

static int icm206xx_EnableSensor(struct i2c_client *client, int sensor_type, bool enable)
{
	u8 databuf[2] = {0};
	int res = 0;

	if (sensor_type >= ICM206XX_SENSOR_TYPE_MAX) {
		return ICM206XX_ERR_INVALID_PARAM;
	}

	res = icm206xx_share_i2c_read_register(ICM206XX_REG_PWR_CTL2, databuf, 1);
	if (res < 0) {
		ACC_ERR("read power ctl2 register err!\n");
		return ICM206XX_ERR_I2C;
	}

	if (enable == true) {
		if (sensor_type == ICM206XX_SENSOR_TYPE_ACC || sensor_type == ICM206XX_SENSOR_TYPE_STEP_COUNTER) {
			databuf[0] &= ~ICM206XX_BIT_ACCEL_STANDBY;

		} else{
			databuf[0] &= ~ICM206XX_BIT_GYRO_STANDBY;
		}
	} else{
		if (sensor_type == ICM206XX_SENSOR_TYPE_ACC) {
			if (icm206xx_all_sensor_status_info[ICM206XX_SENSOR_TYPE_STEP_COUNTER].sensor_power == false)
				databuf[0] |= ICM206XX_BIT_ACCEL_STANDBY;
		} else if (sensor_type == ICM206XX_SENSOR_TYPE_STEP_COUNTER) {
			if (icm206xx_all_sensor_status_info[ICM206XX_SENSOR_TYPE_ACC].sensor_power == false)
				databuf[0] |= ICM206XX_BIT_ACCEL_STANDBY;
		} else{
			databuf[0] |= ICM206XX_BIT_GYRO_STANDBY;
		}
	}

	res = icm206xx_share_i2c_write_register(ICM206XX_REG_PWR_CTL2, databuf, 1);
	if (res < 0) {
		ACC_ERR("set power ctl2 failed!\n");
		return ICM206XX_ERR_I2C;
	}

	mdelay(50);

	return ICM206XX_SUCCESS;
}

static int icm206xx_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[2] = {0};
	int res = 0;

	if ((NULL == buf) || (bufsize <= 30))
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}

	res = icm206xx_i2c_read_register(client, ICM206XX_REG_WHO_AM_I, databuf, 1);
	if (res < 0) {
		ACC_ERR("read who_am_i register err!\n");
		return ICM206XX_ERR_I2C;
	}

	switch (databuf[0]) {
	case ICM20608D_WHO_AM_I:
		sprintf(buf, "ICM20608D [0x%x]", databuf[0]);
		break;
	default:
		sprintf(buf, "Unknown Sensor [0x%x]", databuf[0]);
		break;
	}

	return ICM206XX_SUCCESS;
}

static int icm206xx_SetSampleRate(struct i2c_client *client, int sensor_type, u64 delay_ns, bool force_1khz)
{
	u8 databuf[2] = {0};
	int sample_rate = 0;
	int rate_div = 0;
	int res = 0;
	int i, highest_sample_rate = 0;

	/* ---------------------------------------------------- */
	/* Normal  : 200ms (200,000,000ns)	: 5hz		*/
	/* UI	   : 66ms (66,670,000ns)	: 15hz		*/
	/* Game    : 20ms (20,000,000ns)	: 50hz		*/
	/* Fastest : 5ms (5,000,000ns)	: 200hz		*/
	/*	Actual Delay of Accel : 10ms (100hz)		*/
	/*	Actual Delay of Gyro : 2ms (500hz)		*/
	/* ---------------------------------------------------- */
	sample_rate = (int)(delay_ns / 1000);		/* ns to us */
	sample_rate = (int)(sample_rate / 1000);	/* us to ms */

	sample_rate = (int)(1000 / sample_rate);	/* ns to hz */

	/* sample rate: 5hz to 200hz; */
	/* when force_1khz is true, it means self test mode is running at 1khz */
	if ((sample_rate > 200) && (force_1khz == false))
		sample_rate = 200;
	else if (sample_rate < 5)
		sample_rate = 5;

	if (icm206xx_all_sensor_status_info[sensor_type].sample_rate == sample_rate)
		return ICM206XX_SUCCESS;

	icm206xx_all_sensor_status_info[sensor_type].sample_rate = sample_rate;

	/* Check sample rate of enabled sensors */
	for (i = 0; i < ICM206XX_SENSOR_TYPE_MAX; i++) {
		if (icm206xx_all_sensor_status_info[i].sensor_power == true) {
			if (highest_sample_rate < icm206xx_all_sensor_status_info[i].sample_rate)
				highest_sample_rate = icm206xx_all_sensor_status_info[i].sample_rate;
		}
	}

	/* Change sample rate */
	rate_div = ICM206XX_INTERNAL_SAMPLE_RATE / highest_sample_rate - 1;

	databuf[0] = rate_div;
	res = icm206xx_share_i2c_write_register(ICM206XX_REG_SAMRT_DIV, databuf, 1);
	if (res < 0) {
		ACC_ERR("write sample rate register err!\n");
		return ICM206XX_ERR_I2C;
	}

	/* read sample div after written for test */
	res = icm206xx_share_i2c_read_register(ICM206XX_REG_SAMRT_DIV, databuf, 1);
	if (res < 0) {
		ACC_ERR("read accel sample rate register err!\n");
		return ICM206XX_ERR_I2C;
	}

	ACC_LOG("read accel sample rate: 0x%x\n", databuf[0]);

	return ICM206XX_SUCCESS;
}

/*=======================================================================================*/
/* Export Symbols Section								 */
/* -- To make this module as an Entry module to access other INVN sensors such as Gyro	 */
/*=======================================================================================*/

int icm206xx_share_i2c_read_register(u8 addr, u8 *data, u8 len)
{
	return icm206xx_i2c_read_register(icm206xx_accel_i2c_client, addr, data, len);
}
EXPORT_SYMBOL(icm206xx_share_i2c_read_register);

int icm206xx_share_i2c_write_register(u8 addr, u8 *data, u8 len)
{
	return icm206xx_i2c_write_register(icm206xx_accel_i2c_client, addr, data, len);
}
EXPORT_SYMBOL(icm206xx_share_i2c_write_register);

int icm206xx_share_i2c_read_memory(u16 mem_addr, u32 len, u8 *data)
{
	return icm206xx_i2c_read_memory(icm206xx_accel_i2c_client, mem_addr, len, data);
}
EXPORT_SYMBOL(icm206xx_share_i2c_read_memory);

int icm206xx_share_i2c_write_memory(u16 mem_addr, u32 len, u8 const *data)
{
	return icm206xx_i2c_write_memory(icm206xx_accel_i2c_client, mem_addr, len, data);
}
EXPORT_SYMBOL(icm206xx_share_i2c_write_memory);

/*----------------------------------------------------------------------------*/
int icm206xx_share_ChipSoftReset(void)
{
	return icm206xx_ChipSoftReset(icm206xx_accel_i2c_client);
}
EXPORT_SYMBOL(icm206xx_share_ChipSoftReset);

int icm206xx_share_SetPowerMode(int sensor_type, bool enable)
{
	return icm206xx_SetPowerMode(icm206xx_accel_i2c_client, sensor_type, enable);
}
EXPORT_SYMBOL(icm206xx_share_SetPowerMode);

int icm206xx_share_EnableSensor(int sensor_type, bool enable)
{
	return icm206xx_EnableSensor(icm206xx_accel_i2c_client, sensor_type, enable);
}
EXPORT_SYMBOL(icm206xx_share_EnableSensor);

int icm206xx_share_ReadChipInfo(char *buf, int bufsize)
{
	return icm206xx_ReadChipInfo(icm206xx_accel_i2c_client, buf, bufsize);
}
EXPORT_SYMBOL(icm206xx_share_ReadChipInfo);

int icm206xx_share_SetSampleRate(int sensor_type, u64 delay_ns, bool force_1khz)
{
	return icm206xx_SetSampleRate(icm206xx_accel_i2c_client, sensor_type, delay_ns, force_1khz);
}
EXPORT_SYMBOL(icm206xx_share_SetSampleRate);
/*----------------------------------------------------------------------------*/



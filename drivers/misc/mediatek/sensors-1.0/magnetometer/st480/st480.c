/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Magnetometer Sensor Driver
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
 *
 *****************************************************************************/

#include "cust_mag.h"
#include "mag.h"
#include "st480.h"

/*----------------------------------------------------------------------------*/
#define MSE_TAG                  "MSENSOR"
#define MSE_FUN(f)               pr_debug(MSE_TAG"%s\n", __func__)
#define MSE_ERR(fmt, args...)    pr_debug(MSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define MSE_LOG(fmt, args...)    pr_debug(MSE_TAG fmt, ##args)

#define MSE_VER(fmt, args...)   ((void)0)

#define POWER_NONE_MACRO MT65XX_POWER_NONE
static int st480_init_flag;

/*----------------------------------------------------------------------------*/
#define ST480_AXIS_X            0
#define ST480_AXIS_Y            1
#define ST480_AXIS_Z            2
#define ST480_AXES_NUM          3

#define ST480_BUFSIZE          256

/* Maintain  cust info here */
struct mag_hw mag_cust;
static struct mag_hw *hw = &mag_cust;

/* For  driver get cust info */
struct mag_hw *get_cust_mag(void)
{
	return &mag_cust;
}

/*----------------------------------------------------------------------------*/
static struct platform_driver st480_sensor_driver;


static DECLARE_WAIT_QUEUE_HEAD(open_wq);

struct st480_data {
	struct i2c_client *client;
	struct mag_hw *hw;
	struct hwmsen_convert cvt;
	atomic_t layout;
	atomic_t trace;
};

struct SensorData st480sensordata;

static struct st480_data *st480;

static atomic_t m_flag;
static atomic_t o_flag;
static atomic_t open_flag;

static atomic_t dev_open_count;

static short st480d_delay = ST480_DEFAULT_DELAY;

struct mag_3 {
	s16 mag_x, mag_y, mag_z;
};
static struct mag_3 mag;
static int st480_local_init(void);
static int st480_remove(void);

static struct mag_init_info st480_init_info = {
	.name = ST480_I2C_NAME,
	.init = st480_local_init,
	.uninit = st480_remove,
};


/*----------------------------------------------------------------------------*/
static void st480_power(struct mag_hw *hw, unsigned int on)
{
#if 0
	static unsigned int power_on;

	if (hw->power_id != POWER_NONE_MACRO) {
		MSE_LOG("power %s\n", on ? "on" : "off");
		if (power_on == on)
			MSE_LOG("ignore power control: %d\n", on);
		else if (on) {
			if (!hwPowerOn(hw->power_id, hw->power_vol, "st480"))
				MSE_ERR("power on fails!!\n");
		} else {
			if (!hwPowerDown(hw->power_id, "st480"))
				MSE_ERR("power off fail!!\n");
		}
	}
	power_on = on;
#endif
}

/*----------------------------------------------------------------------------*/

static int st480_GetOpenStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
	return atomic_read(&open_flag);
}


/*
 * st480 i2c transfer
 * read/write
 */
static int st480_i2c_transfer_data(struct i2c_client *client, int len, char *buf, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = len,
		 .buf = buf,
		 },
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = buf,
		 },
	};

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		MSE_ERR("megnetic_i2c_read_data: transfer error\n");
		return -EIO;
	} else
		return 0;
}

/*
 * Device detect and init
 *
 */
static int client_init(struct i2c_client *client)
{
	int ret;
	unsigned char buf[5];
	/* unsigned char data[1]; */

	ret = 0;

	MSE_FUN();
	memset(buf, 0, 5);
	/* memset(data, 0, 1); */

	buf[0] = READ_REGISTER_CMD;
	buf[1] = 0x00;

	while (st480_i2c_transfer_data(client, 2, buf, 3) != 0) {
		ret++;
		 msleep(20);
		if (st480_i2c_transfer_data(client, 2, buf, 3) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			return -EIO;
	}

	/* if(buf[2] != ST480_DEVICE_ID) {
	* MSE_ERR("st480 ic not exist!");
	* return -ENODEV;
	* }
	*/

	MSE_ERR("st480 device id is %x\n", buf[2]);

	/* init register step 1 */
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = ONE_INIT_DATA_HIGH;
	buf[2] = ONE_INIT_DATA_LOW;
	buf[3] = ONE_INIT_REG;
	ret = 0;
	while (st480_i2c_transfer_data(client, 4, buf, 1) != 0) {
		ret++;
		msleep(20);
		if (st480_i2c_transfer_data(client, 4, buf, 1) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			return -EIO;
	}

	/* init register step 2 */
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = TWO_INIT_DATA_HIGH;
	buf[2] = TWO_INIT_DATA_LOW;
	buf[3] = TWO_INIT_REG;
	ret = 0;
	while (st480_i2c_transfer_data(client, 4, buf, 1) != 0) {
		ret++;
		msleep(20);
		if (st480_i2c_transfer_data(client, 4, buf, 1) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			return -EIO;
	}

	/* set calibration register */
	buf[0] = WRITE_REGISTER_CMD;
	buf[1] = CALIBRATION_DATA_HIGH;
	buf[2] = CALIBRATION_DATA_LOW;
	buf[3] = CALIBRATION_REG;
	ret = 0;
	while (st480_i2c_transfer_data(client, 4, buf, 1) != 0) {
		ret++;
		msleep(20);
		if (st480_i2c_transfer_data(client, 4, buf, 1) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			return -EIO;
	}

	/* set mode config */
	buf[0] = SINGLE_MEASUREMENT_MODE_CMD;
	ret = 0;
	while (st480_i2c_transfer_data(client, 1, buf, 1) != 0) {
		ret++;
		msleep(20);
		if (st480_i2c_transfer_data(client, 1, buf, 1) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			return -EIO;
	}

	return 0;
}

static void st480_work_func(void)
{
	char buffer[7];
	int ret;
	int mag_tmp[ST480_AXES_NUM];

	memset(buffer, 0, 7);

	buffer[0] = READ_MEASUREMENT_CMD;
	ret = 0;
	while (st480_i2c_transfer_data(st480->client, 1, buffer, 7) != 0) {
		ret++;

		if (st480_i2c_transfer_data(st480->client, 1, buffer, 7) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			break;
	}

	if (!((buffer[0] >> 4) & 0X01)) {
		mag.mag_x = ((buffer[1] << 8) | buffer[2]);
		mag.mag_y = ((buffer[3] << 8) | buffer[4]);
		mag.mag_z = ((buffer[5] << 8) | buffer[6]);

		/* MSE_ERR("st480 raw data: x = %d, y = %d, z = %d\n",mag.mag_x,mag.mag_y,mag.mag_z); */
	}
	mag_tmp[st480->cvt.map[ST480_AXIS_X]] = st480->cvt.sign[ST480_AXIS_X] * mag.mag_x;
	mag_tmp[st480->cvt.map[ST480_AXIS_Y]] = st480->cvt.sign[ST480_AXIS_Y] * mag.mag_y;
	mag_tmp[st480->cvt.map[ST480_AXIS_Z]] = st480->cvt.sign[ST480_AXIS_Z] * mag.mag_z;

	mag.mag_x = mag_tmp[0];
	mag.mag_y = mag_tmp[1];
	mag.mag_z = mag_tmp[2];
	buffer[0] = SINGLE_MEASUREMENT_MODE_CMD;
	ret = 0;
	while (st480_i2c_transfer_data(st480->client, 1, buffer, 1) != 0) {
		ret++;
		msleep(20);
		if (st480_i2c_transfer_data(st480->client, 1, buffer, 1) == 0)
			break;
		if (ret > MAX_FAILURE_COUNT)
			break;
	}
}

/*----------------------------------------------------------------------------*/
static int st480_ReadChipInfo(char *buf, int bufsize)
{
	if ((!buf) || (bufsize <= 30))
		return -1;
	if (!(st480->client)) {
		*buf = 0;
		return -2;
	}

	sprintf(buf, "st480");
	return 0;
}

/*----------------------------------------------------------------------------*/
static int st480_ReadPostureData(char *buf, int bufsize)
{
	if ((!buf) || (bufsize <= 80))
		return -1;

	read_lock(&st480sensordata.datalock);
	sprintf(buf, "%d %d %d %d", st480sensordata.yaw, st480sensordata.pitch,
		st480sensordata.roll, st480sensordata.mag_status);
	read_unlock(&st480sensordata.datalock);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int st480_ReadCaliData(char *buf, int bufsize)
{
	if ((!buf) || (bufsize <= 80))
		return -1;

	read_lock(&st480sensordata.datalock);
	sprintf(buf, "%d %d %d %d ", st480sensordata.nmx, st480sensordata.nmy,
		st480sensordata.nmz, st480sensordata.mag_status);
	read_unlock(&st480sensordata.datalock);

	return 0;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
	char strbuf[256];

	sprintf(strbuf, "st480d");
	return sprintf(buf, "%s", strbuf);
}


static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[256];

	st480_ReadChipInfo(strbuf, 256);
	return sprintf(buf, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];

	st480_work_func();
	sprintf(strbuf, "%04x %04x %04x", mag.mag_x, mag.mag_y, mag.mag_z);
	/* st480_ReadSensorData(strbuf, ST480_BUFSIZE); */
	return sprintf(buf, "%s\n", strbuf);
}


/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];

	st480_ReadPostureData(strbuf, ST480_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_calidata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[ST480_BUFSIZE];

	st480_ReadCaliData(strbuf, ST480_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct st480_data *data = st480;

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		       data->hw->direction, atomic_read(&data->layout), data->cvt.sign[0],
		       data->cvt.sign[1], data->cvt.sign[2], data->cvt.map[0], data->cvt.map[1],
		       data->cvt.map[2]);
}

/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct st480_data *data = st480;
	int layout = 0;

	if (sscanf(buf, "0x%x", &layout) == 1) {
		atomic_set(&data->layout, layout);
		if (!hwmsen_get_convert(layout, &data->cvt))
			MSE_ERR("HWMSEN_GET_CONVERT function OK!\r\n");
		else if (!hwmsen_get_convert(data->hw->direction, &data->cvt))
			MSE_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		else {
			MSE_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	} else {
		MSE_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	struct st480_data *data = st480;
	ssize_t len = 0;

	if (data->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d %d (%d %d)\n",
				data->hw->i2c_num, data->hw->direction, data->hw->power_id,
				data->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "OPEN: %d\n", atomic_read(&dev_open_count));
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct st480_data *obj = st480;

	if (obj == NULL) {
		MSE_ERR("ST480_i2c_data is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct st480_data *obj = st480;
	int trace;

	if (obj == NULL) {
		MSE_ERR("st480_i2c_data is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->trace, trace);
		/* MSE_ERR("invalid content: '%s', length = %d\n", buf, count); */

	return count;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(daemon, S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DRIVER_ATTR(calidata, S_IRUGO, show_calidata_value, NULL);
static DRIVER_ATTR(layout, S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(trace, S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *st480_attr_list[] = {
	&driver_attr_daemon,
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_posturedata,
	&driver_attr_calidata,
	&driver_attr_layout,
	&driver_attr_status,
	&driver_attr_trace,
};

/*----------------------------------------------------------------------------*/
static int st480_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(st480_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, st480_attr_list[idx]);
		if (err) {
			MSE_ERR("driver_create_file (%s) = %d\n", st480_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int st480_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(st480_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, st480_attr_list[idx]);

	return err;
}


static int st480d_open(struct inode *inode, struct file *file)
{
	MSE_FUN();
	return nonseekable_open(inode, file);
}

static int st480d_release(struct inode *inode, struct file *file)
{
	MSE_FUN();
	return 0;
}

int factory_mode;
static long st480d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int status = 0;
	int calidata[4];
	int valuebuf[4];
	int retval = 0;
	int enable = 0;
	char buff[ST480_BUFSIZE];

	/* MSE_LOG("st480d:[%x]%s\n",cmd,cmd==MSENSOR_IOCTL_SENSOR_ENABLE?"MSENSOR_IOCTL_SENSOR_ENABLE":
	* cmd==MSENSOR_IOCTL_READ_SENSORDATA?"MSENSOR_IOCTL_READ_SENSORDATA":
	* cmd==CKT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA?"CKT_MSENSOR_IOCTL_READ_FACTORY_SENSORDATA":
	* cmd==MSENSOR_IOCTL_SET_POSTURE?"MSENSOR_IOCTL_SET_POSTURE":
	* cmd==MSENSOR_IOCTL_SET_CALIDATA?"MSENSOR_IOCTL_SET_CALIDATA":
	* cmd==IOCTL_SENSOR_GET_COMPASS_FLAG?"IOCTL_SENSOR_GET_COMPASS_FLAG":
	* cmd==IOCTL_SENSOR_GET_COMPASS_DELAY?"IOCTL_SENSOR_GET_COMPASS_DELAY":
	* "default");
	*/

	switch (cmd) {
	case MSENSOR_IOCTL_SENSOR_ENABLE:
		if (argp == NULL) {
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}
		if (copy_from_user(&enable, argp, sizeof(enable))) {
			MSE_ERR("copy_from_user failed.");
			return -EFAULT;
		}
		MSE_ERR("MSENSOR_IOCTL_SENSOR_ENABLE enable=%d!\r\n", enable);
		read_lock(&st480sensordata.ctrllock);
		if (enable == 1) {
			atomic_set(&o_flag, 1);
			atomic_set(&open_flag, 1);
		} else {
			atomic_set(&o_flag, 0);
			if (atomic_read(&m_flag) == 0)
				atomic_set(&open_flag, 0);
		}
		wake_up(&open_wq);
		read_unlock(&st480sensordata.ctrllock);

		break;

	case MSENSOR_IOCTL_READ_SENSORDATA:
		st480_work_func();
		MSE_ERR("read msensor data: %d, %d, %d\n", mag.mag_x, mag.mag_y, mag.mag_z);
		if (factory_mode == 1) {
			MSE_ERR("factory_mode\n");
			sprintf(buff, "%04x %04x %04x", mag.mag_x, mag.mag_y, mag.mag_z);
			if (copy_to_user(argp, buff, strlen(buff) + 1)) {
				MSE_ERR("copy to user error.\n");
				retval = -EFAULT;
				goto err_out;
			}
			factory_mode = 0;
		} else {
			if (copy_to_user(argp, (void *)&mag, sizeof(mag)) != 0) {
				MSE_ERR("copy to user error.\n");
				retval = -EFAULT;
				goto err_out;

			}
		}
		break;

		/* add by sen.luo */
	case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
		factory_mode = 1;
		st480_work_func();
		sprintf(buff, "%04x %04x %04x %04x %04x", mag.mag_x, mag.mag_y, mag.mag_z, 1, 1);
		if (copy_to_user(argp, buff, strlen(buff) + 1)) {
			MSE_ERR("copy to user error.\n");
			retval = -EFAULT;
			goto err_out;
		}
		break;
		/* end */

	case MSENSOR_IOCTL_SET_POSTURE:
		MSE_ERR("st480 MSENSOR_IOCTL_SET_POSTURE\n");
		if (argp == NULL) {
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}

		if (copy_from_user(&valuebuf, argp, sizeof(valuebuf))) {
			retval = -EFAULT;
			goto err_out;
		}

		write_lock(&st480sensordata.datalock);
		st480sensordata.yaw = valuebuf[0];
		st480sensordata.pitch = valuebuf[1];
		st480sensordata.roll = valuebuf[2];
		st480sensordata.mag_status = valuebuf[3];
		write_unlock(&st480sensordata.datalock);
		MSE_ERR("st480 yaw = %d, pitch = %d, roll = %d\n", st480sensordata.yaw,
			st480sensordata.pitch, st480sensordata.roll);
		break;

	case MSENSOR_IOCTL_SET_CALIDATA:
		MSE_ERR("st480 MSENSOR_IOCTL_SET_CALIDATA\n");
		/* argp = (void __user *) arg; */
		if (argp == NULL) {
			MSE_ERR("IO parameter pointer is NULL!\r\n");
			break;
		}
		if (copy_from_user(&calidata, argp, sizeof(calidata))) {
			retval = -EFAULT;
			goto err_out;
		}

		write_lock(&st480sensordata.datalock);
		st480sensordata.nmx = calidata[0];
		st480sensordata.nmy = calidata[1];
		st480sensordata.nmz = calidata[2];

		st480sensordata.mag_status = calidata[3];
		MSE_ERR("st480 nmx = %d nmy = %d nmz = %d\n", calidata[0], calidata[1],
			calidata[2]);
		write_unlock(&st480sensordata.datalock);
		break;

	case IOCTL_SENSOR_GET_COMPASS_FLAG:
		status = st480_GetOpenStatus();
		if (copy_to_user(argp, &status, sizeof(status))) {
			MSE_ERR("copy_to_user failed.");
			retval = -EFAULT;
			goto err_out;
		}
		break;


	case IOCTL_SENSOR_GET_COMPASS_DELAY:
		if (copy_to_user(argp, (void *)&st480d_delay, sizeof(st480d_delay)) != 0) {
			MSE_ERR("copy to user error.\n");
			retval = -EFAULT;
			goto err_out;
		}
		break;
	}
	return 0;

err_out:
	return retval;
}

#ifdef CONFIG_COMPAT
static long st480_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case MSENSOR_IOCTL_SENSOR_ENABLE:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_SENSOR_ENABLE,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("MSENSOR_IOCTL_SENSOR_ENABLE unlocked_ioctl failed."); */
			return ret;
		}
		break;

	case MSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_READ_SENSORDATA,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("MSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed."); */
			return ret;
		}
		break;

		/* add by sen.luo */
	case MSENSOR_IOCTL_READ_FACTORY_SENSORDATA:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_READ_FACTORY_SENSORDATA,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("MSENSOR_IOCTL_READ_FACTORY_SENSORDATA unlocked_ioctl failed."); */
			return ret;
		}
		break;

	case MSENSOR_IOCTL_SET_POSTURE:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_SET_POSTURE,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("MSENSOR_IOCTL_SET_POSTURE unlocked_ioctl failed."); */
			return ret;
		}
		break;

	case MSENSOR_IOCTL_SET_CALIDATA:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, MSENSOR_IOCTL_SET_CALIDATA,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("MSENSOR_IOCTL_SET_CALIDATA unlocked_ioctl failed."); */
			return ret;
		}
		break;

	case IOCTL_SENSOR_GET_COMPASS_FLAG:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, IOCTL_SENSOR_GET_COMPASS_FLAG,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("IOCTL_SENSOR_GET_COMPASS_FLAG unlocked_ioctl failed."); */
			return ret;
		}
		break;

	case IOCTL_SENSOR_GET_COMPASS_DELAY:
		if (arg32 == NULL) {
			/* MAGN_LOG("invalid argument."); */
			return -EINVAL;
		}

		ret = file->f_op->unlocked_ioctl(file, IOCTL_SENSOR_GET_COMPASS_DELAY,
						 (unsigned long)arg32);
		if (ret) {
			/* MAGN_LOG("IOCTL_SENSOR_GET_COMPASS_DELAY unlocked_ioctl failed."); */
			return ret;
		}
		break;

	default:
		/* MSE_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd); */
		return -ENOIOCTLCMD;
		/* break; */
	}
	return 0;
}
#endif


static const struct file_operations st480d_fops = {
	.open = st480d_open,
	.release = st480d_release,
	.unlocked_ioctl = st480d_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = st480_compat_ioctl,
#endif
};

static struct miscdevice st480d_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msensor",
	.fops = &st480d_fops,
};

int st480_operate(void *self, uint32_t command, void *buff_in, int size_in,
		  void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data *msensor_data;

	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			MSE_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value < 10)
				value = 10;
			st480d_delay = value;
		}
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			MSE_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			read_lock(&st480sensordata.ctrllock);

			if (value == 1) {
				atomic_set(&m_flag, 1);
				atomic_set(&open_flag, 1);
			} else {
				atomic_set(&m_flag, 0);
				if (atomic_read(&o_flag) == 0)
					atomic_set(&open_flag, 0);
			}
			wake_up(&open_wq);
			read_unlock(&st480sensordata.ctrllock);
			/* TODO: turn device into standby or normal mode */
		}
		break;

	case SENSOR_GET_DATA:
		MSE_ERR("%s sensor_get_data\n", __func__);
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			MSE_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			msensor_data = (struct hwm_sensor_data *)buff_out;

			msensor_data->values[0] = st480sensordata.nmx;
			msensor_data->values[1] = st480sensordata.nmy;
			msensor_data->values[2] = st480sensordata.nmz;
			msensor_data->status = st480sensordata.mag_status;
			msensor_data->value_divide = 1000;
			MSE_ERR("%s, SENSOR_GET_DATA, data:%d,%d,%d\n", __func__,
				msensor_data->values[0], msensor_data->values[1],
				msensor_data->values[2]);
		}
		break;
	default:
		MSE_ERR("st480 operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}

	return err;
}

int st480_orientation_operate(void *self, uint32_t command, void *buff_in, int size_in,
			      void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data *osensor_data;

	switch (command) {
	case SENSOR_DELAY:

		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			MSE_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value < 10)
				value = 10;
			st480d_delay = value;
		}
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			MSE_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			read_lock(&st480sensordata.ctrllock);
			if (value == 1) {
				atomic_set(&o_flag, 1);
				atomic_set(&open_flag, 1);
			} else {
				atomic_set(&o_flag, 0);
				if (atomic_read(&m_flag) == 0)
					atomic_set(&open_flag, 0);
			}
			wake_up(&open_wq);
			read_unlock(&st480sensordata.ctrllock);
		}
		break;

	case SENSOR_GET_DATA:
		MSE_ERR("%s sensor_get_data\n", __func__);
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			MSE_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			osensor_data = (struct hwm_sensor_data *)buff_out;

			osensor_data->values[0] = st480sensordata.yaw;
			osensor_data->values[1] = st480sensordata.pitch;
			osensor_data->values[2] = st480sensordata.roll;
			osensor_data->status = st480sensordata.mag_status;
			osensor_data->value_divide = 1000;
			MSE_ERR("%s, SENSOR_GET_DATA, data:%d,%d,%d\n", __func__,
				osensor_data->values[0], osensor_data->values[1],
				osensor_data->values[2]);
		}
		break;
	default:
		MSE_ERR("st480d operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}

	return err;
}

static int st480_m_enable(int en)
{
	int value = 0;
	int err = 0;

	value = en;
	/* factory_mode = 1; */
	if (value == 1) {
		atomic_set(&m_flag, 1);
		atomic_set(&open_flag, 1);
	} else {
		atomic_set(&m_flag, 0);
		if (atomic_read(&o_flag) == 0)
			atomic_set(&open_flag, 0);
	}
	wake_up(&open_wq);
	return err;
}

static int st480_m_set_delay(u64 ns)
{
	int value = 0;

	value = (int)ns / 1000 / 1000;	/* convert into ms */
	if (value < ST480_DEFAULT_DELAY)
		value = ST480_DEFAULT_DELAY;
	st480d_delay = value;
	return 0;
}

static int st480_m_open_report_data(int open)
{
	return 0;
}

static int st480_m_get_data(int *x, int *y, int *z, int *status)
{
	/* msensor_data = (hwm_sensor_data *)buff_out; */
	*x = st480sensordata.nmx / 1000;
	*y = st480sensordata.nmy / 1000;
	*z = st480sensordata.nmz / 1000;
	*status = st480sensordata.mag_status;

	MSE_ERR("%s, data:%d,%d,%d\n", __func__, *x, *y, *z);

	return 0;
}

#if SENSOR_AUTO_TEST
static int sensor_test_read(void)
{
	st480_work_func();
	return 0;
}

static int auto_test_read(void *unused)
{
	while (1) {
		sensor_test_read();
		msleep(200);
	}
	return 0;
}
#endif

static int st480_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	struct mag_control_path ctl = { 0 };
	struct mag_data_path mag_data = { 0 };

	MSE_FUN();
	err = get_mag_dts_func(client->dev.of_node, hw);
	if (err) {
		MSE_ERR("get dts info fail\n");
		err = -EFAULT;
		goto exit1;
	}

#if SENSOR_AUTO_TEST
	struct task_struct *thread;
#endif

	/* Allocate memory for driver data */
	st480 = kzalloc(sizeof(struct st480_data), GFP_KERNEL);
	if (!st480) {
		MSE_ERR("SENODIA st480_i2c_probe: memory allocation failed.\n");
		err = -ENOMEM;
		goto exit1;
	}

	memset(st480, 0, sizeof(struct st480_data));

	st480->hw = hw;
	err = hwmsen_get_convert(st480->hw->direction, &st480->cvt);
	if (err) {
		MSE_ERR("invalid direction: %d\n", st480->hw->direction);
		goto exit1;
	}

	atomic_set(&st480->layout, st480->hw->direction);
	atomic_set(&st480->trace, 0);
	init_waitqueue_head(&open_wq);

	client->addr = st480->hw->i2c_addr[0];
	st480->client = client;
	i2c_set_clientdata(client, st480);


	err = client_init(st480->client);
	if (err != 0) {
		MSE_ERR("st480 setup error!\n");
		goto exit2;
	}
	err = st480_create_attr(&(st480_init_info.platform_diver_addr->driver));
	if (err) {
		MSE_ERR("create attribute err = %d\n", err);
		goto exit3;
	}


	err = misc_register(&st480d_device);
	if (err) {
		MSE_ERR("SENODIA st480_i2c_probe: st480d device register failed\n");
		goto exit3;
	}

	ctl.is_use_common_factory = false;
	ctl.enable = st480_m_enable;
	ctl.set_delay = st480_m_set_delay;
	ctl.open_report_data = st480_m_open_report_data;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = st480->hw->is_batch_supported;

	err = mag_register_control_path(&ctl);

	if (err) {
		MSE_ERR("register mag control path err\n");
		goto exit2;
	}

	mag_data.div = CONVERT_M_DIV;
	mag_data.get_data = st480_m_get_data;

	err = mag_register_data_path(&mag_data);
	if (err) {
		MSE_ERR("register data control path err\n");
		goto exit2;
	}

	/* As default, report all information */
	atomic_set(&m_flag, 0);
	atomic_set(&o_flag, 0);
	atomic_set(&open_flag, 0);

#if SENSOR_AUTO_TEST
	thread = kthread_run(auto_test_read, NULL, "st480_read_test");
#endif
	st480_init_flag = 1;

	MSE_ERR("st480 compass probed successfully.");
	return 0;

	misc_deregister(&st480d_device);
exit3:
exit2:
	kfree(st480);
exit1:
	st480_init_flag = -1;
	return err;

}

static int st480_i2c_remove(struct i2c_client *client)
{
	int err;

	MSE_FUN();
	err = st480_delete_attr(&st480_sensor_driver.driver);
	if (err)
		MSE_ERR("st480_delete_attr fail: %d\n", err);
	i2c_unregister_device(st480->client);
	kfree(i2c_get_clientdata(st480->client));
	misc_deregister(&st480d_device);

	st480 = NULL;

	MSE_LOG("st480 i2c successfully removed.");
	return 0;
}

static const struct i2c_device_id st480_i2c_id[] = { {ST480_I2C_NAME, 0}, {} };

#ifdef CONFIG_OF
static const struct of_device_id mag_of_match[] = {
	{.compatible = "mediatek,msensor"},
	{},
};
#endif

static struct i2c_driver st480_i2c_driver = {
	.driver = {
		   .name = ST480_I2C_NAME,
#ifdef CONFIG_OF
		   .of_match_table = mag_of_match,
#endif
		   },
	.probe = st480_i2c_probe,
	.remove = st480_i2c_remove,
	.id_table = st480_i2c_id,
};

static int st480_probe(struct platform_device *pdev)
{

	MSE_FUN();

	st480_power(hw, 1);

	rwlock_init(&st480sensordata.ctrllock);
	rwlock_init(&st480sensordata.datalock);

	atomic_set(&dev_open_count, 0);

	if (i2c_add_driver(&st480_i2c_driver)) {
		MSE_ERR("add driver error\n");
		return -1;
	}
	return 0;
}

static int st480_remove(void)
{
	atomic_set(&dev_open_count, 0);
	i2c_del_driver(&st480_i2c_driver);
	return 0;
}

static int st480_sensor_remove(struct platform_device *pdev)
{
	atomic_set(&dev_open_count, 0);
	i2c_del_driver(&st480_i2c_driver);
	return 0;
}

static struct platform_driver st480_sensor_driver = {
	.probe = st480_probe,
	.remove = st480_sensor_remove,
	.driver = {
		   .name = "msensor",
		   }
};

static int st480_local_init(void)
{

	/* const char *name = "mediatek,st480_auto"; */
	/* struct mag_hw *hw = get_mag_dts_func(name, hw); */
	MSE_FUN();

	st480_power(hw, 1);
	if (i2c_add_driver(&st480_i2c_driver)) {
		MSE_ERR("i2c_add_driver error\n");
		return -1;
	}
	if (st480_init_flag == -1)
		return -1;
	return 0;
}

static int __init st480_init(void)
{
	mag_driver_add(&st480_init_info);
	return 0;
}

static void __exit st480_exit(void)
{
	MSE_FUN();

	/* platform_driver_unregister(&st480_sensor_driver); */
}
module_init(st480_init);
module_exit(st480_exit);

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("Senodia ST480 linux driver for MTK");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

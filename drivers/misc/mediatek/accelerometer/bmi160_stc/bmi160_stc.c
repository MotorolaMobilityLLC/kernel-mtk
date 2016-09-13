/* BOSCH STEP COUNTER Sensor Driver
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

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <sensors_io.h>
#include <cust_acc.h>
#include <accel.h>
#include <step_counter.h>
#include "bmi160_stc.h"

extern struct i2c_client *bmi160_acc_i2c_client;
/*Lenovo-sw weimh1 add 2016-7-06 begin: */
static int step_c_enable_nodata(int en);
static int bmi160_stc_clr(struct i2c_client *client);
/*Lenovo-sw weimh1 add 2016-7-06 end*/
struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;
#define COMPATIABLE_NAME "mediatek,m_step_c_pl"
/* sensor type */
enum SENSOR_TYPE_ENUM {
	BMI160_STC_TYPE = 0x0,
	INVALID_TYPE = 0xff
};

/* power mode */
enum STC_POWERMODE_ENUM {
	STC_SUSPEND_MODE = 0x0,
	STC_NORMAL_MODE,
	STC_LOWPOWER_MODE,
	STC_UNDEFINED_POWERMODE = 0xff
};

struct pedometer_data_t {
	u8 wkar_step_detector_status;
	u_int32_t last_step_counter_value;
};

/* bmg i2c client data */
struct step_c_i2c_data {
	struct i2c_client *client;
	u8 sensor_name[MAX_SENSOR_NAME];
	enum SENSOR_TYPE_ENUM sensor_type;
	enum STC_POWERMODE_ENUM power_mode;
	int datarate;
	struct mutex lock;
	atomic_t	trace;
	atomic_t	suspend;
	atomic_t	filter;
	struct pedometer_data_t pedo_data;
};

static bool step_enable_status = false;
static int step_c_init_flag =-1; // 0<==>OK -1 <==> fail
static struct i2c_driver step_c_i2c_driver;
static struct step_c_i2c_data *obj_i2c_data;
static int step_c_set_datarate(struct i2c_client *client,
		int datarate);
static int step_c_set_powermode(struct i2c_client *client,
		enum STC_POWERMODE_ENUM power_mode);
static const struct i2c_device_id step_c_i2c_id[] = {
	{STC_DEV_NAME, 0},
	{}
};

static int stc_i2c_read_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	u8 beg = addr;
	int err;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &beg
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = len,	.buf = data,
		}
	};
	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		STEP_C_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}
	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		STEP_C_ERR("i2c_transfer error: (%d %p %d) %d\n",
			addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;
	}
	return err;
}

static int stc_i2c_write_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	int err, idx = 0, num = 0;
	char buf[C_I2C_FIFO_SIZE];

	if (!client)
		return -EINVAL;
	else if (len >= C_I2C_FIFO_SIZE) {
		STEP_C_ERR("length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		STEP_C_ERR("send command error!!\n");
		return -EFAULT;
	} else {
		err = 0;
	}
	return err;
}

static int step_c_get_chip_type(struct i2c_client *client)
{
	int err = 0;
	u8 chip_id = 0;
	struct step_c_i2c_data *obj = obj_i2c_data;
	STEP_C_FUN();

	err = stc_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, &chip_id, 1);
	mdelay(20);
	if (err != 0)
		return err;

	switch (chip_id) {
	case SENSOR_CHIP_ID_BMI:
	case SENSOR_CHIP_ID_BMI_C2:
	case SENSOR_CHIP_ID_BMI_C3:
		obj->sensor_type = BMI160_STC_TYPE;
		strcpy(obj->sensor_name, "step_counter");
		break;
	default:
		obj->sensor_type = INVALID_TYPE;
		strcpy(obj->sensor_name, "unknown sensor");
		break;
	}

	STEP_C_LOG("[%s]chip id = %#x, sensor name = %s\n",
	__func__, chip_id, obj->sensor_name);

	if (obj->sensor_type == INVALID_TYPE) {
		STEP_C_ERR("unknown sensor.\n");
		return -1;
	}
	return 0;
}


/*!
 *	@brief This API write enable step counter
 *	from the register 0x7B bit 3
 *
 *
 *  @param v_step_counter_u8 : The value of step counter enable
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 */
int step_counter_enable(struct i2c_client *client, u8 v_step_counter_u8)
{
	int com_rslt = -1;
	u8 v_data_u8 = 0;
//	u8 v_data_conf1 = 0x2D;
	u8 v_data2_u8 = 0;
	u8 v_data3_u8 = 0;

#if 0
	mdelay(30);
	com_rslt = stc_i2c_write_block(client,
					BMI160_USER_STEP_CONFIG_0_ADDR,
					&v_data_conf1, BMI160_GEN_READ_WRITE_DATA_LENGTH);

	mdelay(30);
	com_rslt += stc_i2c_write_block(client,
					BMI160_USER_STEP_CONFIG_1_ADDR,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
#endif

	if (v_step_counter_u8 <= 1) {
		com_rslt = stc_i2c_read_block(client,
				BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__REG,
				&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);

		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
					BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE,
					v_step_counter_u8);
			com_rslt += stc_i2c_write_block(client,
				BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__REG,
				&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
	} else {
		com_rslt = E_BMI160_OUT_OF_RANGE;
	}

	mdelay(30);
	com_rslt = stc_i2c_read_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &v_data3_u8, 1);
	mdelay(30);
	com_rslt = stc_i2c_read_block(client,
					BMI160_USER_STEP_CONFIG_0_ADDR,
					&v_data2_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	mdelay(30);
	
	STEP_C_LOG("ACC_CONF=0x%x, STEP_CONF[0]=0x%x, STEP_CONF[1]=0x%x\n", v_data3_u8, v_data2_u8, v_data_u8);
	
	return com_rslt;
}

static int step_c_set_powermode(struct i2c_client *client,
		enum STC_POWERMODE_ENUM power_mode)
{
	int err = 0;
	u8 actual_power_mode = 0;
	u8 acc_conf = 0;
	u8 acc_us =0;
	u8 bandwidth = BMI160_ACCEL_OSR4_AVG1;
	int datarate = BMI160_ACCEL_ODR_200HZ;
	struct step_c_i2c_data *obj = obj_i2c_data;

	STEP_C_LOG("[%s] power_mode = %d, old power_mode = %d\n",
	__func__, power_mode, obj->power_mode);

	if (power_mode == obj->power_mode) {
		STEP_C_ERR("power mode is the newest!\n");
		return 0;
	}
	mutex_lock(&obj->lock);
	/*Lenovo-sw weimh1 add 2016-7-7:add low power mode*/
	if (power_mode == STC_SUSPEND_MODE) {
		actual_power_mode = CMD_PMU_ACC_SUSPEND;
	} else if (power_mode == STC_LOWPOWER_MODE) {
		datarate = BMI160_ACCEL_ODR_100HZ;
		actual_power_mode = CMD_PMU_ACC_LOWPOWER;
		acc_us = 1;
		bandwidth = 0x02;//BMI160_ACCEL_OSR2_AVG2;
	} else {
		datarate = BMI160_ACCEL_ODR_200HZ;
		actual_power_mode = CMD_PMU_ACC_NORMAL;
		acc_us = 0;
		bandwidth = BMI160_ACCEL_OSR4_AVG1;
	}

	/*Lenovo-sw weimh1 add 2016-8-17 begin:config acc_us*/
	err = stc_i2c_read_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &acc_conf, 1);
	mdelay(30);
	acc_conf = BMI160_SET_BITSLICE(acc_conf,
			BMI160_USER_ACC_CONF_ACC_BWP, bandwidth);
	acc_conf = BMI160_SET_BITSLICE(acc_conf,
			BMI160_USER_ACC_CONF_ACC_UNDER_SAMPLING, acc_us);
	err += stc_i2c_write_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &acc_conf, 1);
	mdelay(30);
	/*Lenovo-sw weimh1 add 2016-8-17 end*/

	err = stc_i2c_write_block(client,
		BMI160_CMD_COMMANDS__REG, &actual_power_mode, 1);
	mdelay(30);
	if (err < 0) {
		STEP_C_ERR("set power mode failed, err = %d, sensor name = %s\n",
				err, obj->sensor_name);
		mutex_unlock(&obj->lock);
		return err;
	}
	obj->power_mode = power_mode;

	mutex_unlock(&obj->lock);

	err = step_c_set_datarate(client, datarate);
	if (err < 0) {
		STEP_C_ERR("%s,set bandwidth failed, err = %d\n", __func__, err);
		mutex_unlock(&obj->lock);
		return err;
	}

	err = stc_i2c_read_block(client,
		BMI160_USER_ACC_CONF_ODR__REG, &acc_conf, 1);
	mdelay(30);
	STEP_C_LOG("%s acc_conf=0x%x!\n", __func__, acc_conf);

	return err;
}

static int step_c_set_datarate(struct i2c_client *client,
		int datarate)
{
	int err = 0;
	u8 data = 0;
	struct step_c_i2c_data *obj = obj_i2c_data;
	STEP_C_LOG("[%s] datarate = %d, old datarate = %d\n",
	__func__, datarate, obj->datarate);

	if (datarate == obj->datarate) {
		STEP_C_ERR("invalid data rate.\n");
		return 0;
	}
	mutex_lock(&obj->lock);
	if (obj->sensor_type == BMI160_STC_TYPE) {
		err = stc_i2c_read_block(client,
			BMI160_USER_ACC_CONF_ODR__REG, &data, 1);
		data = BMI160_SET_BITSLICE(data,
			BMI160_USER_ACC_CONF_ODR, datarate);
		err += stc_i2c_write_block(client,
			BMI160_USER_ACC_CONF_ODR__REG, &data, 1);
	}
	if (err < 0) {
		STEP_C_ERR("set data rate failed.\n");
	}
	else {
		obj->datarate = datarate;
	}
	mutex_unlock(&obj->lock);
	return err;
}


static int step_c_init_client(struct i2c_client *client)
{
	int err = 0;
	STEP_C_FUN();
	err = step_c_get_chip_type(client);
	if (err < 0) {
		STEP_C_ERR("get chip type failed, err = %d\n", err);
		return err;
	}
	err = step_c_set_datarate(client, BMI160_ACCEL_ODR_200HZ);
	if (err < 0) {
		STEP_C_ERR("set bandwidth failed, err = %d\n", err);
		return err;
	}
	err = step_c_set_powermode(client,
		(enum STC_POWERMODE_ENUM)STC_LOWPOWER_MODE);
	if (err < 0) {
		STEP_C_ERR("set power mode failed, err = %d\n", err);
		return err;
	}
	err = step_counter_enable(obj_i2c_data->client, ENABLE);
	if (err < 0) {
		STEP_C_ERR("set step counter enable failed, err = %d\n", err);
		return err;
	}
	
	return 0;
}

static int step_c_open(struct inode *inode, struct file *file)
{
	file->private_data = obj_i2c_data;
	if (file->private_data == NULL) {
		STEP_C_ERR("file->private_data == NULL.\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int step_c_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long stc_c_unlocked_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	return 0;
}

static const struct file_operations step_c_fops = {
	.owner = THIS_MODULE,
	.open = step_c_open,
	.release = step_c_release,
	.unlocked_ioctl = stc_c_unlocked_ioctl,
};

static struct miscdevice step_c_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "step_counter",
	.fops = &step_c_fops,
};

static int step_c_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct step_c_i2c_data *obj = obj_i2c_data;
	int err = 0;
	STEP_C_FUN();
	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			STEP_C_ERR("null pointer.\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);

		err = step_c_set_powermode(obj->client, STC_LOWPOWER_MODE);
		if (err) {
			STEP_C_ERR("step counter set suspend mode failed, err = %d\n",
				err);
			return err;
		}
		err = step_counter_enable(obj_i2c_data->client, ENABLE);
		if (err < 0) {
			STEP_C_ERR("set step counter enable failed, err = %d\n", err);
			return err;
		}

		/*Lenovo-sw weimh1 add 2016-8-17 begin: report data when enable*/
		step_notify(TYPE_STEP_SUSPEND);
		/*Lenovo-sw weimh1 add 2016-8-17 end*/
	}
	return err;
}

static int step_c_resume(struct i2c_client *client)
{
	struct step_c_i2c_data *obj = obj_i2c_data;
	int err;

	STEP_C_FUN();
	
	if (obj == NULL) {
		STEP_C_ERR("null pointer\n");
		return -EINVAL;
	}

#if 0
	/*Lenovo-sw weimh1 mod 2016-5-23 begin:use obj->client instead of client*/
	err = step_c_init_client(obj->client);
	/*Lenovo-sw weimh1 mod 2016-5-23 */
#else
	if (step_enable_status == true) {
		err = step_c_set_powermode(obj->client,
				(enum STC_POWERMODE_ENUM)STC_NORMAL_MODE);
		/*Lenovo-sw weimh1 add 2016-8-17 begin: report data when enable*/
		step_notify(TYPE_STEP_COUNTER);
		/*Lenovo-sw weimh1 add 2016-8-17 end*/
	}
	else
		err = step_c_set_powermode(obj->client,
				(enum STC_POWERMODE_ENUM)STC_LOWPOWER_MODE);

	err = step_counter_enable(obj->client, ENABLE);
	if (err < 0) {
		STEP_C_ERR("set step counter enable failed, err = %d\n", err);
		return err;
	}
#endif
	if (err) {
		STEP_C_ERR("initialize client failed, err = %d\n", err);
		return err;
	}
	atomic_set(&obj->suspend, 0);
	
	return 0;
}

static int step_c_i2c_detect(struct i2c_client *client,
		struct i2c_board_info *info)
{
	strcpy(info->type, STC_DEV_NAME);
	return 0;
}

static int step_c_open_report_data(int open)
{
	return 0;
}

static int step_c_enable_nodata(int en)
{
	int err = 0;
	if (ENABLE == en) {
		step_enable_status = true;
		err = step_c_set_powermode(obj_i2c_data->client,
				(enum STC_POWERMODE_ENUM)STC_NORMAL_MODE);
		err += step_counter_enable(obj_i2c_data->client, ENABLE);
		
		/*Lenovo-sw weimh1 add 2016-8-17 begin: report data when enable*/
		step_notify(TYPE_STEP_COUNTER);
		/*Lenovo-sw weimh1 add 2016-8-17 end*/
	}
	else {
		step_enable_status = false;
		err = step_c_set_powermode(obj_i2c_data->client,
				(enum STC_POWERMODE_ENUM)STC_LOWPOWER_MODE);
		err += step_counter_enable(obj_i2c_data->client, ENABLE);

		/*Lenovo-sw weimh1 add 2016-9-13 begin: report data when enable*/
		step_notify(TYPE_STEP_SUSPEND);
		/*Lenovo-sw weimh1 add 2016-9-13 end*/
	}
	
	if(err < 0) {
		STEP_C_ERR("step counter enable failed.\n");
		return err;
	}

	STEP_C_LOG("step counter enable success.\n");
	
	return 0;
}

static int step_c_set_delay(u64 ns)
{
	int err;
	int sample_delay = 0;
	int value = (int)ns/1000/1000 ;
	struct step_c_i2c_data *obj = obj_i2c_data;
	if(value <= 5) {
		sample_delay = BMI160_ACCEL_ODR_200HZ;
	}
	else if(value <= 10) {
		sample_delay = BMI160_ACCEL_ODR_100HZ;
	}
	else if(value <= 20) {
		sample_delay = BMI160_ACCEL_ODR_50HZ;
	}
	else if(value <= 40) {
		sample_delay = BMI160_ACCEL_ODR_25HZ;
	}
	else {
		sample_delay = BMI160_ACCEL_ODR_100HZ;
	}
	STEP_C_LOG("sensor delay command: %d, sample_delay = %d\n",
			value, sample_delay);
	err = step_c_set_datarate(obj->client, sample_delay);
	if (err < 0)
		STEP_C_ERR("set delay parameter error\n");
	return 0;
}

static int step_c_enable_significant(int open)
{
	return 0;
}

static int step_c_enable_step_detect(int open)
{
	return 0;
}

static int step_c_read_counter(struct i2c_client *client, u16 *v_step_cnt_s16)
{
	int com_rslt = -1;
	u8 a_data_u8r[2] = {0, 0};
	
	com_rslt = stc_i2c_read_block(client, BMI160_USER_STEP_COUNT_LSB__REG,
					a_data_u8r, BMI160_STEP_COUNTER_LENGTH);
	*v_step_cnt_s16 = (u16)
		((((u32)(a_data_u8r[BMI160_STEP_COUNT_MSB_BYTE]))
		<< BMI160_SHIFT_BIT_POSITION_BY_08_BITS)
		| (a_data_u8r[BMI160_STEP_COUNT_LSB_BYTE]));
	mdelay(30);
	STEP_C_LOG("step counter = %d.\n", (*v_step_cnt_s16));
	return com_rslt;
}

static int bmi160_stc_clr(struct i2c_client *client)
{
	int comres = 0;
	u8 cmd_mode = CMD_RESET_STEPCOUNTER;

	STEP_C_LOG("current cmd = 0x%x.\n", cmd_mode);
	comres = stc_i2c_write_block(client,
		BMI160_CMD_COMMANDS__REG, &cmd_mode, 1);
	mdelay(30);
	if (comres< 0) {
		STEP_C_ERR("step_cnt_clr, err = %d,\n", comres);
	}

	return comres;
}

static int bmi160_stc_get_mode(struct i2c_client *client, u8 *mode)
{
	int comres = 0;
	u8 v_data_u8r = 0;
	comres = stc_i2c_read_block(client,
			BMI160_USER_ACC_PMU_STATUS__REG, &v_data_u8r, 1);
	*mode = BMI160_GET_BITSLICE(v_data_u8r,
			BMI160_USER_ACC_PMU_STATUS);
	mdelay(30);
	return comres;
}

static int get_step_counter_enable(struct i2c_client *client, u8 *v_step_counter_u8)
{
	int com_rslt  = 0;
	u8 v_data_u8 = 0;

	com_rslt = stc_i2c_read_block(client,
			BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE__REG,
			&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	*v_step_counter_u8 = BMI160_GET_BITSLICE(v_data_u8,
			BMI160_USER_STEP_CONFIG_1_STEP_COUNT_ENABLE);
	mdelay(30);
	return com_rslt;
}

static int step_c_get_data(u64 *value, int *status)
{
	u16 data;
	int err;
	static u16 last_stc_value;
	u8 acc_mode;
	u8 stc_enable;
	struct step_c_i2c_data *obj = obj_i2c_data;

	err = step_c_read_counter(obj->client, &data);
	if (0 == data) {
		err = bmi160_stc_get_mode(obj->client, &acc_mode);
		if (err < 0) {
			STEP_C_ERR("bmi160_acc_get_mode failed.\n");
		}
		STEP_C_LOG("step_c_get_data acc mode = %d.\n", acc_mode);
		err = get_step_counter_enable(obj->client, &stc_enable);
		if (err < 0) {
			STEP_C_ERR("get_step_counter_enable failed.\n");
		}
		STEP_C_LOG("step_c_get_data stc enable = %d.\n", stc_enable);
		if (acc_mode != 0 && stc_enable == 1) {
			data = last_stc_value;
		}
	}

	mdelay(30);
	if (err < 0) {
		STEP_C_ERR("read step count fail.\n");
		return err;
	}

	if (data >= last_stc_value) {
		obj->pedo_data.last_step_counter_value += (
			data - last_stc_value);
		last_stc_value = data;
	} else {
		if (data > 0)
			last_stc_value = data;
	}

	*value = (int)obj->pedo_data.last_step_counter_value;
	*status = 1;
	STEP_C_LOG("step_c_get_data = %d.\n", (int)(*value));
	return err;
}

static int stc_get_data_significant(u64 *value, int *status)
{
	return 0;
}

static int stc_get_data_step_d(u64 *value, int *status)
{
	return 0;
}

static int step_c_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct step_c_i2c_data *obj;
	struct step_c_control_path ctl = {0};
	struct step_c_data_path data = {0};
	int err = 0;
	
	STEP_C_FUN();
	
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	obj_i2c_data = obj;
	obj->client = bmi160_acc_i2c_client;
	i2c_set_clientdata(obj->client, obj);
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	obj->power_mode = STC_UNDEFINED_POWERMODE;
	obj->datarate = BMI160_ACCEL_ODR_RESERVED;
	mutex_init(&obj->lock);
	err = step_c_init_client(obj->client);
	if (err)
		goto exit_init_client_failed;

	err = misc_register(&step_c_device);
	if (err) {
		STEP_C_ERR("misc device register failed, err = %d\n", err);
		goto exit_misc_device_register_failed;
	}
	ctl.open_report_data= step_c_open_report_data;
	ctl.enable_nodata = step_c_enable_nodata;
	ctl.set_delay  = step_c_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = false;
	ctl.is_report_input_direct = false;
	ctl.enable_significant = step_c_enable_significant;
	ctl.enable_step_detect = step_c_enable_step_detect;
	err = step_c_register_control_path(&ctl);
	if(err) {
		STEP_C_ERR("register step_counter control path err.\n");
		goto exit_create_attr_failed;
	}

	data.get_data = step_c_get_data;
	data.vender_div = 1;
	data.get_data_significant = stc_get_data_significant;
	data.get_data_step_d = stc_get_data_step_d;
	err = step_c_register_data_path(&data);
	if(err) {
		STEP_C_ERR("step_c_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	step_c_init_flag =0;
	bmi160_stc_clr(obj->client);
	STEP_C_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&step_c_device);
exit_misc_device_register_failed:
exit_init_client_failed:
	kfree(obj);
exit:
	step_c_init_flag =-1;
	STEP_C_ERR("err = %d\n", err);
	return err;
}

static int step_c_i2c_remove(struct i2c_client *client)
{
	int err = 0;
	err = misc_deregister(&step_c_device);
	if (err)
		STEP_C_ERR("misc_deregister failed, err = %d\n", err);
	obj_i2c_data = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id step_c_of_match[] = {
	{.compatible = "mediatek,bmi120_new"},
	{},
};
#endif

static struct i2c_driver step_c_i2c_driver = {
	.driver = {
		.name = STC_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = step_c_of_match,
#endif
	},
	.probe = step_c_i2c_probe,
	.remove	= step_c_i2c_remove,
	.detect	= step_c_i2c_detect,
	.suspend = step_c_suspend,
	.resume = step_c_resume,
	.id_table = step_c_i2c_id,
};

static int bmi160_stc_remove(void)
{
    STEP_C_FUN();
		
    i2c_del_driver(&step_c_i2c_driver);
	
    return 0;
}

/*----------------------------------------------------------------------------*/
static int bmi160_stc_local_init(void)
{
	STEP_C_LOG("step counter init.\n");
	
	if (i2c_add_driver(&step_c_i2c_driver)) {
		STEP_C_ERR("add driver error.\n");
		return -1;
	}
	
	if (-1 == step_c_init_flag) {
	    return -1;
	}
	
	return 0;
}

static struct step_c_init_info bmi160_stc_init_info = {
	.name = "step_counter",
	.init = bmi160_stc_local_init,
	.uninit = bmi160_stc_remove,
};

static int __init stc_init(void)
{
	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);
	if (!hw) {
		STEP_C_ERR("get dts info fail\n");
		return 0;
	}
	
	STEP_C_LOG("%s: bosch step counter driver version: %s\n",
	__func__, STC_DRIVER_VERSION);
	step_c_driver_add(&bmi160_stc_init_info);
	
	return 0;
}

static void __exit stc_exit(void)
{
	STEP_C_FUN();
}

module_init(stc_init);
module_exit(stc_exit);

MODULE_LICENSE("GPLv2");
MODULE_DESCRIPTION("STEP COUNTER I2C Driver");
MODULE_AUTHOR("BOSCH");
MODULE_VERSION(STC_DRIVER_VERSION);

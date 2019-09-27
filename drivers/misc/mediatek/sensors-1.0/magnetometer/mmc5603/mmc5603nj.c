/*
 *
 * mmc5603x.c - mmc5603x magnetic sensor chip driver.
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
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <hwmsensor.h>
#include <sensors_io.h>
#include <linux/types.h>

#include <hwmsen_helper.h>

#include <cust_mag.h>
#include "mmc5603nj.h"

#include "mag.h"

/*-------------------------MT6516&MT6573 define-------------------------------*/
#define POWER_NONE_MACRO MT65XX_POWER_NONE

/*----------------------------------------------------------------------------*/
#define MMC5603NJ_DEV_NAME "mmc5603nj"
#define DRIVER_VERSION "1.00.20167"
/*----------------------------------------------------------------------------*/
#define MEMSIC_DEBUG_ON 1
#define MEMSIC_DEBUG_FUNC_ON 1
/* Log define */
#define MEMSIC_INFO(fmt, arg...) pr_err("[MMC5603NJ][INFO]: "fmt, ##arg)
#define MEMSIC_ERR(fmt, arg...) pr_err("[MMC5603NJ][ERROR][%d]: "fmt, __LINE__, ##arg)
#define MEMSIC_DEBUG(fmt, arg...)                                           \
	do                                                                      \
	{                                                                       \
		if (MEMSIC_DEBUG_ON)                                                \
			pr_err("[MMC5603NJ][DEBUG][%d]: "fmt, __LINE__, ##arg); \
	} while (0)
#define MEMSIC_DEBUG_FUNC()                                                       \
	do                                                                            \
	{                                                                             \
		if (MEMSIC_DEBUG_FUNC_ON)                                                 \
			pr_err("[MMC5603NJ][FUNC]:  %s@Line:%d\n", __func__, __LINE__); \
	} while (0)


#define MMC5603NJ_DEFAULT_DELAY 100
#define MMC5603NJ_BUFSIZE 0x50

#define MMC5603NJ_RETRY_COUNT 3

/* Define Delay time */
#define MMC5603NJ_DELAY_TM 10  /* ms */
#define MMC5603NJ_DELAY_STDN 1 /* ms */

static struct i2c_client *this_client = NULL;

// calibration msensor and orientation data
//static int sensor_data[CALIBRATION_DATA_SIZE];
static struct mutex sensor_data_mutex;
static struct mutex read_i2c_xyz;

/* static DECLARE_WAIT_QUEUE_HEAD(data_ready_wq); */
static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static int mmcd_delay = MMC5603NJ_DEFAULT_DELAY;

static atomic_t open_flag = ATOMIC_INIT(0);
static atomic_t m_flag = ATOMIC_INIT(0);
static atomic_t o_flag = ATOMIC_INIT(0);

static const struct i2c_device_id mmc5603nj_i2c_id[] = {{MMC5603NJ_DEV_NAME, 0}, {}};

#define MMC5603X_MODE_SWITCH
uint8_t st_thd_reg[3]; // save strim data from reg 0x27-0x29
enum Read_Mode
{
	CONTINUOUS_MODE_AUTO_SR = 1,
	SINGLE_MODE = 2
};
enum Read_Mode MMC5603_Read_mode = CONTINUOUS_MODE_AUTO_SR;

/* Maintain  cust info here */

/*----------------------------------------------------------------------------*/
static int mmc5603nj_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mmc5603nj_i2c_remove(struct i2c_client *client);
static int mmc5603nj_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
//static int mmc5603nj_suspend(struct i2c_client *client, pm_message_t msg);
//static int mmc5603nj_resume(struct i2c_client *client);
static int mmc5603nj_local_init(void);
static int mmc5603nj_remove(void);
static int mmc5603nj_enable_sensor(void);
static int mmc5603nj_disable_sensor(void);

typedef enum
{
	MEMSIC_FUN_DEBUG = 0x01,
	MEMSIC_DATA_DEBUG = 0X02,
	MEMSIC_HWM_DEBUG = 0X04,
	MEMSIC_CTR_DEBUG = 0X08,
	MEMSIC_I2C_DEBUG = 0x10,
} MMC_TRC;

/*----------------------------------------------------------------------------*/
struct mmc5603nj_i2c_data
{
	struct i2c_client *client;
	struct mag_hw hw;
	atomic_t layout;
	atomic_t trace;
	struct hwmsen_convert cvt;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_drv;
#endif
};

static int mmc5603nj_init_flag; /* 0<==>OK -1 <==> fail */

static struct mag_init_info mmc5603nj_init_info = {
	.name = "mmc5603nj",
	.init = mmc5603nj_local_init,
	.uninit = mmc5603nj_remove,
};

#ifdef CONFIG_OF
static const struct of_device_id mag_of_match[] = {
	{.compatible = "mediatek,mmc5603"},
	{},
};
#endif

//#ifdef CONFIG_PM_SLEEP
//static const struct dev_pm_ops mmc5603nj_pm_ops = {
//	SET_SYSTEM_SLEEP_PM_OPS(mmc5603nj_suspend, mmc5603nj_resume)
//};
//#endif
static struct i2c_driver mmc5603nj_i2c_driver = {
	.driver = {
		.name = MMC5603NJ_DEV_NAME,
//#ifdef CONFIG_PM_SLEEP
//		.pm    = &mmc5603nj_pm_ops,
//#endif
#ifdef CONFIG_OF
		.of_match_table = mag_of_match,
#endif
	},
	.probe = mmc5603nj_i2c_probe,
	.remove = mmc5603nj_i2c_remove,
	.detect = mmc5603nj_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
//.suspend    = mmc5603nj_suspend,
//.resume     = mmc5603nj_resume,
#endif
	.id_table = mmc5603nj_i2c_id,
};

static atomic_t dev_open_count;

static DEFINE_MUTEX(mmc5603nj_i2c_mutex);

#ifndef CONFIG_MTK_I2C_EXTENSION
int mag_read_byte(struct i2c_client *client, u8 addr, u8 *data)
{
	u8 beg = addr;
	int err;
	struct i2c_msg msgs[2] = {
		{.addr = client->addr, .flags = 0, .len = 1, .buf = &beg},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = data,
		}};

	if (!client)
		return -EINVAL;

	err = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (err != 2)
	{
		MEMSIC_ERR("i2c_transfer error: (%d %p) %d\n", addr, data, err);
		err = -EIO;
	}

	err = 0;

	return err;
}
static int mag_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err = 0;
	u8 beg = addr;
	struct i2c_msg msgs[2] = {{0}, {0}};

	if (len == 1)
		return mag_read_byte(client, addr, data);

	mutex_lock(&mmc5603nj_i2c_mutex);
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client)
	{
		mutex_unlock(&mmc5603nj_i2c_mutex);
		MEMSIC_ERR("Client is Empty\n");
		return -EINVAL;
	}
	else if (len > C_I2C_FIFO_SIZE)
	{
		mutex_unlock(&mmc5603nj_i2c_mutex);
		MEMSIC_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (err != 2)
	{
		MEMSIC_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	}
	else
	{
		err = 0;
	}
	mutex_unlock(&mmc5603nj_i2c_mutex);

	return err;
}

static int mag_i2c_write_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	int err = 0, idx = 0, num = 0;
	char buf[C_I2C_FIFO_SIZE];

	mutex_lock(&mmc5603nj_i2c_mutex);
	if (!client)
	{
		mutex_unlock(&mmc5603nj_i2c_mutex);
		MEMSIC_ERR("Client is Empty\n");
		return -EINVAL;
	}
	else if (len >= C_I2C_FIFO_SIZE)
	{
		mutex_unlock(&mmc5603nj_i2c_mutex);
		MEMSIC_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0)
	{
		mutex_unlock(&mmc5603nj_i2c_mutex);
		MEMSIC_ERR("send command error!!\n");
		return -EFAULT;
	}
	mutex_unlock(&mmc5603nj_i2c_mutex);
	return err;
}
#endif

static int I2C_RxData(char *rxData, int length)
{
#ifndef CONFIG_MTK_I2C_EXTENSION
	struct i2c_client *client = this_client;
	int res = 0;
	char addr = rxData[0];

	if ((rxData == NULL) || (length < 1))
	{
		MEMSIC_ERR("Invalid param\n");
		return -EINVAL;
	}
	res = mag_i2c_read_block(client, addr, rxData, length);
	if (res < 0)
	{
		MEMSIC_ERR("mag_i2c_read_block error\n");
		return -1;
	}
	return 0;
#else
	uint8_t loop_i = 0;

	int i;
	struct i2c_client *client = this_client;
	struct mmc5603nj_i2c_data *data = i2c_get_clientdata(client);
	char addr = rxData[0];

	/* Caller should check parameter validity.*/

	if ((rxData == NULL) || (length < 1))
	{
		MEMSIC_ERR("Invalid param\n");
		return -EINVAL;
	}

	for (loop_i = 0; loop_i < MMC5603NJ_RETRY_COUNT; loop_i++)
	{
		this_client->addr = (this_client->addr & I2C_MASK_FLAG) | I2C_WR_FLAG;
		if (i2c_master_send(this_client, (const char *)rxData, ((length << 0X08) | 0X01)))
			break;

		MEMSIC_DEBUG("I2C_RxData delay!\n");
		mdelay(10);
	}

	if (loop_i >= MMC5603NJ_RETRY_COUNT)
	{
		MEMSIC_ERR("%s retry over %d\n", __func__, MMC5603NJ_RETRY_COUNT);
		return -EIO;
	}

	if (atomic_read(&data->trace) == MEMSIC_I2C_DEBUG)
	{
		MEMSIC_INFO("RxData: len=%02x, addr=%02x\n  data=", length, addr);
		for (i = 0; i < length; i++)
			MEMSIC_INFO(" %02x", rxData[i]);

		MEMSIC_INFO("\n");
	}

	return 0;
#endif
}

static int I2C_TxData(char *txData, int length)
{
#ifndef CONFIG_MTK_I2C_EXTENSION
	struct i2c_client *client = this_client;
	int res = 0;
	char addr = txData[0];
	u8 *buff = &txData[1];

	if ((txData == NULL) || (length < 2))
	{
		MEMSIC_ERR("Invalid param\n");
		return -EINVAL;
	}
	res = mag_i2c_write_block(client, addr, buff, (length - 1));
	if (res < 0)
		return -1;
	return 0;
#else
	uint8_t loop_i;
	int i;
	struct i2c_client *client = this_client;
	struct mmc5603nj_i2c_data *data = i2c_get_clientdata(client);

	/* Caller should check parameter validity.*/
	if ((txData == NULL) || (length < 2))
		return -EINVAL;

	this_client->addr = this_client->addr & I2C_MASK_FLAG;
	for (loop_i = 0; loop_i < MMC5603NJ_RETRY_COUNT; loop_i++)
	{
		if (i2c_master_send(this_client, (const char *)txData, length) > 0)
			break;

		MEMSIC_DEBUG("I2C_TxData delay!\n");
		mdelay(10);
	}

	if (loop_i >= MMC5603NJ_RETRY_COUNT)
	{
		MEMSIC_ERR("%s retry over %d\n", __func__, MMC5603NJ_RETRY_COUNT);
		return -EIO;
	}

	if (atomic_read(&data->trace) == MEMSIC_I2C_DEBUG)
	{
		MEMSIC_INFO("TxData: len=%02x, addr=%02x\n  data=", length, txData[0]);
		for (i = 0; i < (length - 1); i++)
			MEMSIC_INFO(" %02x", txData[i + 1]);

		MEMSIC_INFO("\n");
	}

	return 0;
#endif
}

static int ECS_ReadXYZData(int *vec, int size)
{
	unsigned char data[6] = {0, 0, 0, 0, 0, 0};
	struct i2c_client *client = this_client;
	struct mmc5603nj_i2c_data *clientdata = i2c_get_clientdata(client);

	if (size < 3)
	{
		MEMSIC_ERR("Invalid size value\n");
		return -1;
	}
	mutex_lock(&read_i2c_xyz);

	data[0] = MMC5603NJ_REG_DATA;
	if (I2C_RxData(data, 6) < 0)
	{
		mutex_unlock(&read_i2c_xyz);
		MEMSIC_ERR("i2c rx data failed func: %s  line: %d\n", __func__, __LINE__);
		return -EFAULT;
	}

	vec[0] = (uint16_t)(data[0] << 8 | data[1]);
	vec[1] = (uint16_t)(data[2] << 8 | data[3]);
	vec[2] = (uint16_t)(data[4] << 8 | data[5]);

	if (atomic_read(&clientdata->trace) == MEMSIC_DATA_DEBUG)
	{
		MEMSIC_INFO("[X - %04x] [Y - %04x] [Z - %04x]\n", vec[0], vec[1], vec[2]);
	}

	mutex_unlock(&read_i2c_xyz);
	return 0;
}
#ifdef MMC5603X_MODE_SWITCH

static int mmc5603x_get_trim_data(void)
{
	uint8_t reg_value[3];
	int16_t st_thr_data[3] = {0};
	int16_t st_thr_new[3] = {0};

	int16_t st_thd[3] = {0};
	int i = 0;

	reg_value[0] = MMC5603NJ_REG_ST_X_VAL;
	if (I2C_RxData(reg_value, 3) < 0)
	{

		MEMSIC_ERR("i2c rx data failed func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}

	for (i = 0; i < 3; i++)
	{
		st_thr_data[i] = (int16_t)(reg_value[i] - 128) * 32;
		if (st_thr_data[i] < 0)
			st_thr_data[i] = -st_thr_data[i];
		st_thr_new[i] = st_thr_data[i] - st_thr_data[i] / 5;

		st_thd[i] = st_thr_new[i] / 8;
		if (st_thd[i] > 255)
			st_thd_reg[i] = 0xFF;
		else
			st_thd_reg[i] = (uint8_t)st_thd[i];
	}
	return 0;
}
/*********************************************************************************
* decription: Auto self-test
*********************************************************************************/
static int mmc5603x_auto_selftest(void)
{
	unsigned char reg[2] = {0};
	char reg_status[2] = {0};
	/* Write threshold into the reg 0x1E-0x20 */
	reg[0] = st_thd_reg[0];
	reg[1] = MMC5603NJ_REG_X_THD;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
	}

	reg[0] = st_thd_reg[1];
	reg[1] = MMC5603NJ_REG_Y_THD;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
	}

	reg[0] = st_thd_reg[2];
	reg[1] = MMC5603NJ_REG_Z_THD;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
	}

	/* Write 0x40 to register 0x08, set Auto_st_en bit high */

	reg[0] = MMC5603NJ_CMD_AUTO_ST_EN;
	reg[1] = MMC5603NJ_REG_CTRL0;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
	}
	/* Delay 15ms to finish the selftest process */
	msleep(15);

	/* Read Sat_sensor bit */

	reg_status[0] = MMC5603NJ_REG_STATUS1;
	I2C_RxData(reg_status, 1);

	if ((reg_status[0] & MMC5603NJ_SAT_SENSOR))
	{
		return -1;
	}
	return 1;
}
/*********************************************************************************
* decription: SET operation 
*********************************************************************************/
static void mmc5603x_set(void)
{
	unsigned char reg[2] = {0};
	/* Write 0x08 to register 0x1B, set SET bit high */
	reg[0] = MMC5603NJ_CMD_SET;
	reg[1] = MMC5603NJ_REG_CTRL0;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
	}
	/* Delay at least 1ms to finish the SET operation */
	msleep(1);
	return;
}

/*********************************************************************************
* decription: Do selftest operation periodically
*********************************************************************************/
static int mmc5603x_saturation_checking(void)
{

	int ret = 1;
	unsigned char reg[2] = {0};

	/* If sampling rate is 50Hz, then do saturation checking every 250 loops, i.e. 5 seconds */
	static int NumOfSamples = 250;
	static int cnt = 0;

	if ((cnt++) >= NumOfSamples)
	{
		cnt = 0;
		ret = mmc5603x_auto_selftest();
		if (ret == -1)
		{
			/* Sensor is saturated, need to do SET operation */
			mmc5603x_set();
		}

		/* Do TM_M after selftest operation */
		reg[0] = MMC5603NJ_CMD_TMM;
		reg[1] = MMC5603NJ_REG_CTRL0;
		if (I2C_TxData(reg, 2) < 0)
		{
			MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		}
		msleep(8); //Delay 8ms to finish the TM_M operatio
	}

	return ret;
}

/**
 * mmc5603_read_mode_switch.
 *
 * @param  raw_data_ut    Triaxial raw data the unit is ut
 *
 * @return  none 
 */

static void mmc5603x_read_mode_switch(int raw_data_ut[])
{
	unsigned char reg[2] = {0};

	if (MMC5603_Read_mode == CONTINUOUS_MODE_AUTO_SR)
	{
		/* If X or Y axis output exceed 10 Gauss, then switch to single mode */
		if ((raw_data_ut[0] > 10000) || (raw_data_ut[0] < -10000) || (raw_data_ut[1] > 10000) || (raw_data_ut[1] < -10000))
		{

			MMC5603_Read_mode = SINGLE_MODE;
			/* Disable continuous mode */
			reg[0] = 0x00;
			reg[1] = MMC5603NJ_REG_CTRL2;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}
			msleep(15); //Delay 15ms to finish the last sampling

			/* Do SET operation */
			reg[0] = MMC5603NJ_CMD_SET;
			reg[1] = MMC5603NJ_REG_CTRL0;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}

			msleep(1); //Delay 1ms to finish the SET operation

			/* Do TM_M before next data reading */
			reg[0] = MMC5603NJ_CMD_TMM;
			reg[1] = MMC5603NJ_REG_CTRL0;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}
			msleep(8); //Delay 8ms to finish the TM_M operation
		}
	}
	else if (MMC5603_Read_mode == SINGLE_MODE)
	{
		/* If both of X and Y axis output less than 8 Gauss, then switch to continuous mode with Auto_SR */
		if ((raw_data_ut[0] < 8000) && (raw_data_ut[0] > -8000) && (raw_data_ut[1] < 8000) && (raw_data_ut[1] > -8000))
		{

			MMC5603_Read_mode = CONTINUOUS_MODE_AUTO_SR;

			/* Enable continuous mode with Auto_SR */
			reg[0] = MMC5603NJ_CMD_CMM_FREQ_EN | MMC5603NJ_CMD_AUTO_SR_EN;
			reg[1] = MMC5603NJ_REG_CTRL0;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}

			reg[0] = MMC5603NJ_CMD_CMM_EN;
			reg[1] = MMC5603NJ_REG_CTRL2;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}
		}
		else
		{
			/* Sensor checking */
			mmc5603x_saturation_checking();

			/* Do TM_M before next data reading */
			reg[0] = MMC5603NJ_CMD_TMM;
			reg[1] = MMC5603NJ_REG_CTRL0;
			if (I2C_TxData(reg, 2) < 0)
			{
				MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
			}
		}
	}
}
#endif

static int ECS_GetRawData(int data[3])
{
	int err = 0;
	int data_temp[3] = {0};
	struct mmc5603nj_i2c_data *obj = i2c_get_clientdata(this_client);

	if (obj == NULL)
	{
		MEMSIC_ERR("mmc5603nj_i2c_data is null!!\n");
		return 0;
	}
	err = ECS_ReadXYZData(data, 3);
	if (err != 0)
	{
		MEMSIC_ERR("MMC5603NJ_IOC_TM failed\n");
		return -1;
	}

	// sensitivity 1024 count = 1 Guass = 1000mg
	data_temp[0] = (data[0] - MMC5603NJ_OFFSET) * 1000/ MMC5603NJ_SENSITIVITY;
	data_temp[1] = (data[1] - MMC5603NJ_OFFSET) * 1000/ MMC5603NJ_SENSITIVITY;
	data_temp[2] = (data[2] - MMC5603NJ_OFFSET) * 1000/ MMC5603NJ_SENSITIVITY;
	MEMSIC_INFO("coridate before %d %d %d\n", data_temp[0], data_temp[1], data_temp[2]);

#ifdef MMC5603X_MODE_SWITCH
	mmc5603x_read_mode_switch(data_temp);
#endif

	data[obj->cvt.map[0]] = obj->cvt.sign[0] * data_temp[0];
	data[obj->cvt.map[1]] = obj->cvt.sign[1] * data_temp[1];
	data[obj->cvt.map[2]] = obj->cvt.sign[2] * data_temp[2];
	MEMSIC_INFO("coridate after %d %d %d\n", data[0], data[1], data[2]);

	return err;
}

static int mmc5603nj_ReadChipInfo(char *buf, int bufsize)
{
	if ((!buf) || (bufsize <= MMC5603NJ_BUFSIZE - 1))
	{
		MEMSIC_ERR("Invalid buff size\n");
		return -1;
	}
	if (!this_client)
	{
		*buf = 0;
		MEMSIC_ERR("Invalid client\n");
		return -2;
	}

	sprintf(buf, "mmc5603nj Chip");
	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[MMC5603NJ_BUFSIZE];
	mmc5603nj_ReadChipInfo(strbuf, MMC5603NJ_BUFSIZE);
	return sprintf(buf, "%s\n", strbuf);
}

static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{

	int sensordata[3];
	char strbuf[MMC5603NJ_BUFSIZE];

	ECS_GetRawData(sensordata);

	sprintf(strbuf, "%d %d %d\n", sensordata[0], sensordata[1], sensordata[2]);

	return sprintf(buf, "%s\n", strbuf);
}

static ssize_t show_layout_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = this_client;
	struct mmc5603nj_i2c_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
				   data->hw.direction, atomic_read(&data->layout), data->cvt.sign[0], data->cvt.sign[1],
				   data->cvt.sign[2], data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]);
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = this_client;
	struct mmc5603nj_i2c_data *data = i2c_get_clientdata(client);
	int layout = 0;
	int ret = 0;

	ret = kstrtoint(buf, 10, &layout);
	if (ret == 0)
	{
		atomic_set(&data->layout, layout);
		if (!hwmsen_get_convert(layout, &data->cvt))
		{
			MEMSIC_ERR("hwmsen_get_convert function error!\n");
		}
		else if (!hwmsen_get_convert(data->hw.direction, &data->cvt))
		{
			MEMSIC_ERR("invalid layout: %d, restore to %d\n", layout, data->hw.direction);
		}
		else
		{
			MEMSIC_ERR("invalid layout: (%d, %d)\n", layout, data->hw.direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	}
	else
	{
		MEMSIC_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct mmc5603nj_i2c_data *obj = i2c_get_clientdata(this_client);
	if (NULL == obj)
	{
		MEMSIC_ERR("mmc5603nj_i2c_data is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct mmc5603nj_i2c_data *obj = i2c_get_clientdata(this_client);
	int trace;
	if (NULL == obj)
	{
		MEMSIC_ERR("mmc5603nj_i2c_data is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
	{
		atomic_set(&obj->trace, trace);
	}
	else
	{
		MEMSIC_ERR("invalid content: '%s', length = %d\n", buf, count);
	}

	return count;
}

static ssize_t show_daemon_name(struct device_driver *ddri, char *buf)
{
	char strbuf[MMC5603NJ_BUFSIZE];
	sprintf(strbuf, "memsicd5603nj");
	return sprintf(buf, "%s", strbuf);
}

static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	u8 uData = 0;
	struct mmc5603nj_i2c_data *obj = i2c_get_clientdata(this_client);

	if (obj == NULL)
	{
		MEMSIC_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	uData = atomic_read(&m_flag);

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", uData);
	return res;
}

static ssize_t show_selftest(struct device_driver *ddri, char *buf)
{
	return 1; //TODO
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(daemon, S_IRUGO, show_daemon_name, NULL);
static DRIVER_ATTR(chipinfo, S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(layout, S_IRUGO | S_IWUSR, show_layout_value, store_layout_value);
static DRIVER_ATTR(trace, S_IRUGO | S_IWUSR, show_trace_value, store_trace_value);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status, NULL);
static DRIVER_ATTR(selftest, S_IRUGO, show_selftest, NULL);

static struct driver_attribute *mmc5603nj_attr_list[] = {
	&driver_attr_daemon,
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_layout,
	&driver_attr_trace,
	&driver_attr_powerstatus,
	&driver_attr_selftest,
};

static int mmc5603nj_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(mmc5603nj_attr_list) / sizeof(mmc5603nj_attr_list[0]));
	if (driver == NULL)
	{
		return -EINVAL;
	}
	for (idx = 0; idx < num; idx++)
	{
		if ((err = driver_create_file(driver, mmc5603nj_attr_list[idx])))
		{
			MEMSIC_ERR("driver_create_file (%s) = %d\n", mmc5603nj_attr_list[idx]->attr.name, err);
			break;
		}
	}

	return err;
}

static int mmc5603nj_delete_attr(struct device_driver *driver)
{
	int idx;
	int num = (int)(sizeof(mmc5603nj_attr_list) / sizeof(mmc5603nj_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}
	for (idx = 0; idx < num; idx++)
	{
		driver_remove_file(driver, mmc5603nj_attr_list[idx]);
	}

	return 0;
}

#if 0
/*----------------------------------------------------------------------------*/
static int mmc5603nj_suspend(struct device *dev)
{
	return 0;
}
static int mmc5603nj_resume(struct device *dev)
{
	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
static int mmc5603nj_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, MMC5603NJ_DEV_NAME);
	return 0;
}

static int check_device(void)
{
	char product_id[2] = {0};

	product_id[0] = MMC5603NJ_REG_PRODUCTID;
	if (I2C_RxData(product_id, 1) < 0)
	{
		MEMSIC_ERR("[mmc5603nj] read id fail\n");
		//read again
		I2C_RxData(product_id, 1);
	}

	MEMSIC_INFO("[mmc5603nj] product_id[0] = %d\n", product_id[0]);
	if (product_id[0] != MMC5603NJ_PRODUCT_ID)
	{
		MEMSIC_ERR("Got memsic mmc5603nj id failed");
		return -1;
	}
	return 0;
}

static int mmc5603nj_enable(int en)
{
	int value = 0;
	int err = 0;
	value = en;

	if (value == 1)
	{
		atomic_set(&m_flag, 1);
		atomic_set(&open_flag, 1);
	}
	else
	{
		atomic_set(&m_flag, 0);
		if (atomic_read(&o_flag) == 0)
		{
			atomic_set(&open_flag, 0);
		}
	}

	MEMSIC_INFO(" m sensor enablea val = %d, open_flag = %d\n", en, atomic_read(&open_flag));
	if (atomic_read(&open_flag))
	{
		mmc5603nj_enable_sensor();
	}
	else
	{
		mmc5603nj_disable_sensor();
	}
	wake_up(&open_wq);
	return err;
}

static int mmc5603nj_set_delay(u64 ns)
{
	int value = 0;
	value = (int)ns / 1000 / 1000;
	if (value <= 10)
	{
		mmcd_delay = 10;
	}
	else
	{
		mmcd_delay = value;
	}

	return 0;
}

static int mmc5603nj_open_report_data(int open)
{
	return 0;
}

static int mmc5603nj_get_data(int *x, int *y, int *z, int *status)
{
	int data[3] = {0};

	ECS_GetRawData(data);
    //x,y,z is mg, the algorithm interface is divided by 1000  transform to gauss
	*x = data[0] * 100;
	*y = data[1] * 100;
	*z = data[2] * 100;
	*status = 3;
	MEMSIC_INFO("memsic 5603 data x/y/z/status=%d %d %d %d\n", *x, *y, *z, *status);
	return 0;
}

static int mmc5603nj_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int value = 0;

	value = (int)samplingPeriodNs / 1000 / 1000;

	if (value <= 10)
		mmcd_delay = 10;
	else
		mmcd_delay = value;
	return 0;
}

static int mmc5603nj_flush(void)
{
	return mag_flush_report();
}

static int mmc5603nj_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err;

	err = mmc5603nj_enable(enabledisable == true ? 1 : 0);
	if (err)
	{
		MEMSIC_ERR("%s enable sensor failed!\n", __func__);
		return -1;
	}
	err = mmc5603nj_batch(0, sample_periods_ms * 1000000, 0);
	if (err)
	{
		MEMSIC_ERR("%s enable set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}
static int mmc5603nj_factory_get_data(int32_t data[3], int *status)
{
	/* get raw data */
        int32_t data_temp[3] = {0};
        mmc5603nj_get_data(&data_temp[0], &data_temp[1], &data_temp[2], status);
        data[0] = data_temp[0] / 1000;
        data[1] = data_temp[1] / 1000;
        data[2] = data_temp[2] / 1000;
	return 0;
}
static int mmc5603nj_factory_get_raw_data(int32_t data[3])
{
	MEMSIC_INFO("do not support mmc5603nj_factory_get_raw_data!\n");
	return 0;
}
static int mmc5603nj_factory_enable_calibration(void)
{
	return 0;
}
static int mmc5603nj_factory_clear_cali(void)
{
	return 0;
}
static int mmc5603nj_factory_set_cali(int32_t data[3])
{
	return 0;
}
static int mmc5603nj_factory_get_cali(int32_t data[3])
{
	return 0;
}
static int mmc5603nj_factory_do_self_test(void)
{
	return 0;
}

static struct mag_factory_fops mmc5603nj_factory_fops = {
	.enable_sensor = mmc5603nj_factory_enable_sensor,
	.get_data = mmc5603nj_factory_get_data,
	.get_raw_data = mmc5603nj_factory_get_raw_data,
	.enable_calibration = mmc5603nj_factory_enable_calibration,
	.clear_cali = mmc5603nj_factory_clear_cali,
	.set_cali = mmc5603nj_factory_set_cali,
	.get_cali = mmc5603nj_factory_get_cali,
	.do_self_test = mmc5603nj_factory_do_self_test,
};

static struct mag_factory_public mmc5603nj_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &mmc5603nj_factory_fops,
};

static int mmc5603nj_enable_sensor(void)
{
	unsigned char reg[2] = {0};

	MMC5603_Read_mode = CONTINUOUS_MODE_AUTO_SR;

	reg[0] = MMC5603NJ_REG_CTRL1;
	reg[1] = MMC5603NJ_CMD_BW_01;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}
	reg[0] = MMC5603NJ_REG_ODR;
	reg[1] = MMC5603NJ_SAMPLE_RATE + MMC5603NJ_SAMPLE_RATE / 10;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}
	reg[0] = MMC5603NJ_REG_CTRL0;
	reg[1] = MMC5603NJ_CMD_AUTO_SR;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}
	/*turn on continuous mode*/
	reg[0] = MMC5603NJ_REG_CTRL2;
	reg[1] = MMC5603NJ_CMD_CM;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}
	msleep(20);
	MEMSIC_INFO("mmc5603 enable sensor\n");
	
	return 0;
}
static int mmc5603nj_disable_sensor(void)
{
	unsigned char reg[2] = {0};
	/*disable  continuous mode*/
	reg[0] = MMC5603NJ_REG_CTRL2;
	reg[1] = 0;
	if (I2C_TxData(reg, 2) < 0)
	{
		MEMSIC_ERR("i2c transfer error func: %s  line: %d\n", __func__, __LINE__);
		return -1;
	}
	msleep(20);
	MEMSIC_INFO("mmc5603 disaable sensor\n");
	
	return 0;
}

static int mmc5603nj_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct mmc5603nj_i2c_data *data;
	int err = 0;
	struct mag_control_path ctl = {0};
	struct mag_data_path mag_data = {0};

	MEMSIC_INFO("%s: enter probe,driver version=%s\n", __func__, DRIVER_VERSION);

	client->addr = 0x30;
	data = kmalloc(sizeof(struct mmc5603nj_i2c_data), GFP_KERNEL);
	if (!data)
	{
		err = -ENOMEM;
		MEMSIC_ERR("Allocate memory to struct failed\n");
		goto exit;
	}
	memset(data, 0, sizeof(struct mmc5603nj_i2c_data));

	err = get_mag_dts_func(client->dev.of_node, &data->hw);
	if (err < 0)
	{
		MEMSIC_ERR("get dts info fail\n");
		err = -EFAULT;
		goto exit;
	}
	MEMSIC_INFO("direction =%d\n", data->hw.direction);

	err = hwmsen_get_convert(data->hw.direction, &data->cvt);
	if (err)
	{
		MEMSIC_ERR("invalid direction: %d\n", data->hw.direction);
		goto exit;
	}

	atomic_set(&data->layout, data->hw.direction);
	atomic_set(&data->trace, 0);

	mutex_init(&sensor_data_mutex);
	mutex_init(&read_i2c_xyz);

	/*init_waitqueue_head(&data_ready_wq);*/
	init_waitqueue_head(&open_wq);

	data->client = client;
	new_client = data->client;
	i2c_set_clientdata(new_client, data);

	this_client = new_client;
	usleep_range(10000, 11000);

	err = check_device();
	if (err < 0)
	{
		MEMSIC_ERR("mmc5603x probe: check device connect error\n");
		goto exit_kfree;
	}

	
	err = mmc5603x_get_trim_data();
	if (err < 0)
	{
		MEMSIC_ERR("mmc5603x probe: get the trime data error\n");
		goto exit_kfree;
	}

	/* Register sysfs attribute */
	err = mmc5603nj_create_attr(&(mmc5603nj_init_info.platform_diver_addr->driver));
	if (err)
	{
		MEMSIC_ERR("create attribute err = %d\n", err);
		goto exit_sysfs_create_group_failed;
	}

	err = mag_factory_device_register(&mmc5603nj_factory_device);
	if (err)
	{
		MEMSIC_ERR("misc device register failed, err = %d\n", err);
		goto exit_factory_device_register_failed;
	}
	ctl.is_use_common_factory = false;
	ctl.enable = mmc5603nj_enable;
	ctl.set_delay = mmc5603nj_set_delay;
	ctl.open_report_data = mmc5603nj_open_report_data;
	ctl.batch = mmc5603nj_batch;
	ctl.flush = mmc5603nj_flush;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = data->hw.is_batch_supported;
	strlcpy(ctl.libinfo.libname, "memsic", sizeof(ctl.libinfo.libname));

	err = mag_register_control_path(&ctl);
	if (err)
	{
		MEMSIC_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

	mag_data.div = CONVERT_M_DIV;
	mag_data.get_data = mmc5603nj_get_data;

	err = mag_register_data_path(&mag_data);
	if (err)
	{
		MEMSIC_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

	MEMSIC_INFO("mmc5603NJ IIC probe successful !");

	mmc5603nj_init_flag = 1;
	return 0;

exit_sysfs_create_group_failed:
exit_factory_device_register_failed:

exit_kfree:
	kfree(data);
exit:
	MEMSIC_ERR("%s: err = %d\n", __func__, err);
	mmc5603nj_init_flag = -1;
	return err;
}
/*----------------------------------------------------------------------------*/
static int mmc5603nj_i2c_remove(struct i2c_client *client)
{
	int err;

	if ((err = mmc5603nj_delete_attr(&(mmc5603nj_init_info.platform_diver_addr->driver))))
	{
		MEMSIC_ERR("mmc5603nj_delete_attr fail: %d\n", err);
	}

	this_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/

static int mmc5603nj_local_init(void)
{

	atomic_set(&dev_open_count, 0);

	if (i2c_add_driver(&mmc5603nj_i2c_driver))
	{
		MEMSIC_ERR("add driver error\n");
		return -1;
	}
	if (-1 == mmc5603nj_init_flag)
	{
		MEMSIC_ERR("mmc5603nj init failed\n");
		return -1;
	}

	return 0;
}

static int mmc5603nj_remove(void)
{
	atomic_set(&dev_open_count, 0);
	i2c_del_driver(&mmc5603nj_i2c_driver);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int __init mmc5603nj_init(void)
{

	mag_driver_add(&mmc5603nj_init_info);
	return 0;
}

static void __exit mmc5603nj_exit(void)
{
}
module_init(mmc5603nj_init);
module_exit(mmc5603nj_exit);

MODULE_AUTHOR("MEMSIC AE TEAM");
MODULE_DESCRIPTION("MEMSIC MMC5603 Magnetic Sensor Chip Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

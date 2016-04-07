/* BOSCH Accelerometer Sensor Driver
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
 * History: V1.0 --- [2013.01.10]Driver creation
 *          V1.1 --- [2013.02.18]Remove power mode setting in resume function.
 *          V1.2 --- [2013.03.14]Instead late_resume, use resume to make sure
 *                               driver resume is ealier than processes resume.
 */
/*
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <linux/mutex.h>
#include <linux/module.h>
*/
#ifdef MT6516
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#endif

#ifdef MT6573
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#endif

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif

#ifdef MT6577
#include <mach/mt6577_devs.h>
#include <mach/mt6577_typedefs.h>
#include <mach/mt6577_gpio.h>
#include <mach/mt6577_pm_ldo.h>
#endif

#if defined(MT6573) || defined(MT6575) || defined(MT6577)
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#elif defined(MT6516)
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif

/*
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
*/
//extern struct acc_hw* bma222_get_cust_acc_hw(void);

#include <cust_acc.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include "bma2xx.h"
#include <accel.h>

//#define POWER_NONE_MACRO MT65XX_POWER_NONE
/* sensor type */
enum SENSOR_TYPE_ENUM {
	BMA220_TYPE = 0x0,
	BMA222_TYPE,
	BMA222E_TYPE,
	BMA250_TYPE,
	BMA250E_TYPE,
	BMA255_TYPE,
	BMA280_TYPE,

	INVALID_TYPE = 0xff
};

/*
*data rate [=2*bandwidth for bosch accelerometer]
*SENSOR_DELAY_NORMAL: 200ms
*SENSOR_DELAY_UI: 60ms
*SENSOR_DELAY_GAME: 20ms
*SENSOR_DELAY_FASTEST: 0ms
*boundary defined by daemon
*/
enum BMA_DATARATE_ENUM {
	BMA_DATARATE_10HZ = 0x0,
	BMA_DATARATE_25HZ,
	BMA_DATARATE_50HZ,
	BMA_DATARATE_100HZ,
	BMA_DATARATE_200HZ,

	BMA_UNDEFINED_DATARATE = 0xff
};

/* range */
enum BMA_RANGE_ENUM {
	BMA_RANGE_2G = 0x0, /* +/- 2G */
	BMA_RANGE_4G,		/* +/- 4G */
	BMA_RANGE_8G,		/* +/- 8G */

	BMA_UNDEFINED_RANGE = 0xff
};

/* power mode */
enum BMA_POWERMODE_ENUM {
	BMA_SUSPEND_MODE = 0x0,
	BMA_NORMAL_MODE,

	BMA_UNDEFINED_POWERMODE = 0xff
};

/* debug infomation flags */
enum ADX_TRC {
	ADX_TRC_FILTER  = 0x01,
	ADX_TRC_RAWDATA = 0x02,
	ADX_TRC_IOCTL   = 0x04,
	ADX_TRC_CALI	= 0x08,
	ADX_TRC_INFO	= 0x10,
};

/* s/w data filter */
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMA_AXES_NUM];
	int sum[BMA_AXES_NUM];
	int num;
	int idx;
};

/* bma i2c client data */
struct bma_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert   cvt;

	/* sensor info */
	u8 sensor_name[MAX_SENSOR_NAME];
	enum SENSOR_TYPE_ENUM sensor_type;
	enum BMA_POWERMODE_ENUM power_mode;
	enum BMA_RANGE_ENUM range;
	enum BMA_DATARATE_ENUM datarate;
	/* sensitivity = 2^bitnum/range [+/-2G = 4; +/-4G = 8; +/-8G = 16 ] */
	u16 sensitivity;

    /*misc*/
	struct mutex lock;
	atomic_t	trace;
	atomic_t	suspend;
	atomic_t	filter;
	s16	cali_sw[BMA_AXES_NUM+1];/* unmapped axis value */

	/* hw offset */
	s8	offset[BMA_AXES_NUM+1];/*+1: for 4-byte alignment*/

#if defined(CONFIG_BMA_LOWPASS)
	atomic_t	firlen;
	atomic_t	fir_en;
	struct data_filter	fir;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif   //wangxiqiang 
};

/* log macro */
#define GSE_TAG                  "[gsensor] "
#define GSE_FUN(f)               printk(KERN_ERR GSE_TAG"%s,line = %d\n", __func__,__LINE__)
#define GSE_ERR(fmt, args...) \
	printk(KERN_ERR GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    printk(KERN_ERR GSE_TAG fmt, ##args)

static struct platform_driver bma_gsensor_driver;
static struct i2c_driver bma_i2c_driver;
static struct bma_i2c_data *obj_i2c_data;
static int bma_set_powermode(struct i2c_client *client,
		enum BMA_POWERMODE_ENUM power_mode);
static const struct i2c_device_id bma_i2c_id[] = {
	{BMA_DEV_NAME, 0},
	{}
};

/*
* 1st param option: BMA220, BMA222, BMA250, BMA2X2
* 2st param option: PIN_HIGH, PIN_LOW
* Note: When select BMA2X2, it support
*		BMA222E, BMA250E, BMA255, BMA280 automatically.
*/
//static struct i2c_board_info __initdata bma_i2c_info = {
//I2C_BOARD_INFO(BMA_DEV_NAME, BMA_I2C_ADDRESS(BMA2X2, PIN_LOW))
//};

#if 1//wangxiqiang modify 
static DEFINE_MUTEX(gsensor_scp_en_mutex);
static int bma_gsensor_local_init(void);
static int bma_gsensor_remove(void);  //wangxiqiang
static int bma_gsensor_set_delay(u64 ns);
static int bma_gsensor_init_flag = -1; // 0<==>OK -1 <==> fail
//static int sensor_suspend = 0;
static struct acc_init_info bma_init_info = {
    .name = BMA_DEV_NAME,
    .init = bma_gsensor_local_init,
    .uninit = bma_gsensor_remove,
};
#endif



static struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;


/* I2C operation functions */
static int bma_i2c_read_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	u8 beg = addr;
	struct i2c_msg msgs[2] = {
		{
			.addr = client->addr,	.flags = 0,
			.len = 1,	.buf = &beg
		},
		{
			.addr = client->addr,	.flags = I2C_M_RD,
			.len = len,	.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n",
			addr, data, len, err);
		err = -EIO;
	} else {
		err = 0;/*no error*/
	}
	return err;
}

static int bma_i2c_write_block(struct i2c_client *client, u8 addr,
				u8 *data, u8 len)
{
	/*
	*because address also occupies one byte,
	*the maximum length for write is 7 bytes
	*/
	int err, idx = 0, num = 0;
	char buf[C_I2C_FIFO_SIZE];

	if (!client)
		return -EINVAL;
	else if (len >= C_I2C_FIFO_SIZE) {
		GSE_ERR("length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		return -EINVAL;
	}

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	if (err < 0) {
		GSE_ERR("send command error!!\n");
		return -EFAULT;
	} else {
		err = 0;/*no error*/
	}
	return err;
}

static void bma_power(struct acc_hw *hw, unsigned int on)
{
#if 0
	static unsigned int power_on;

	if (hw->power_id != POWER_NONE_MACRO) {/* have externel LDO */
		GSE_LOG("power %s\n", on ? "on" : "off");
		if (power_on == on) {/* power status not change */
			GSE_LOG("ignore power control: %d\n", on);
		} else if (on) {/* power on */
			if (!hwPowerOn(hw->power_id, hw->power_vol,
				BMA_DEV_NAME)) {
				GSE_ERR("power on failed\n");
			}
		} else {/* power off */
			if (!hwPowerDown(hw->power_id, BMA_DEV_NAME))
				GSE_ERR("power off failed\n");
		}
	}
	power_on = on;
#endif 
}

static int bma_read_raw_data(struct i2c_client *client, s16 data[BMA_AXES_NUM])
{
	struct bma_i2c_data *priv = i2c_get_clientdata(client);
	s8 buf[BMA_DATA_LEN] = {0};
	int err = 0;

	if (priv->power_mode == BMA_SUSPEND_MODE) {
		err = bma_set_powermode(client,
			(enum BMA_POWERMODE_ENUM)BMA_NORMAL_MODE);
		if (err < 0) {
			GSE_ERR("set power mode failed, err = %d\n", err);
			return err;
		}
	}

	memset(data, 0, sizeof(s16)*BMA_AXES_NUM);

	if (NULL == client) {
		err = -EINVAL;
		return err;
	}

	if (priv->sensor_type == BMA220_TYPE) {/* BMA220 */
		err = bma_i2c_read_block(client, BMA220_REG_DATAXLOW, buf, 3);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
				priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] = ((s16)buf[BMA_AXIS_X]) >> 2;
		data[BMA_AXIS_Y] = ((s16)buf[BMA_AXIS_Y]) >> 2;
		data[BMA_AXIS_Z] = ((s16)buf[BMA_AXIS_Z]) >> 2;
		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][8bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z],
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z]);
			GSE_LOG("[%s][6bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z],
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA222_TYPE) {/* BMA222 */
		err = bma_i2c_read_block(client, BMA222_REG_DATAXLOW, buf, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] = (s16)buf[BMA_AXIS_X*2 + 1];
		data[BMA_AXIS_Y] = (s16)buf[BMA_AXIS_Y*2 + 1];
		data[BMA_AXIS_Z] = (s16)buf[BMA_AXIS_Z*2 + 1];
		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][8bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z],
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA222E_TYPE) {/* BMA222E */
		err = bma_i2c_read_block(client, BMA2X2_REG_DATAXLOW, buf, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] = (s16)buf[BMA_AXIS_X*2 + 1];
		data[BMA_AXIS_Y] = (s16)buf[BMA_AXIS_Y*2 + 1];
		data[BMA_AXIS_Z] = (s16)buf[BMA_AXIS_Z*2 + 1];
		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][8bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z],
			(s16)buf[BMA_AXIS_X],
			(s16)buf[BMA_AXIS_Y],
			(s16)buf[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA250_TYPE) {/* BMA250 */
		u8 buf_tmp[BMA_DATA_LEN] = {0};
		err = bma_i2c_read_block(client, BMA250_REG_DATAXLOW,
				buf_tmp, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] =
			(s16)(((u16)buf_tmp[1]) << 8 | buf_tmp[0]) >> 6;
		data[BMA_AXIS_Y] =
			(s16)(((u16)buf_tmp[3]) << 8 | buf_tmp[2]) >> 6;
		data[BMA_AXIS_Z] =
			(s16)(((u16)buf_tmp[5]) << 8 | buf_tmp[4]) >> 6;

		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][10bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z],
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA250E_TYPE) {/* BMA250E */
		u8 buf_tmp[BMA_DATA_LEN] = {0};
		err = bma_i2c_read_block(client, BMA2X2_REG_DATAXLOW,
				buf_tmp, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] =
			(s16)(((u16)buf_tmp[1]) << 8 | buf_tmp[0]) >> 6;
		data[BMA_AXIS_Y] =
			(s16)(((u16)buf_tmp[3]) << 8 | buf_tmp[2]) >> 6;
		data[BMA_AXIS_Z] =
			(s16)(((u16)buf_tmp[5]) << 8 | buf_tmp[4]) >> 6;

		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][10bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z],
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA255_TYPE) {/* BMA255 */
		u8 buf_tmp[BMA_DATA_LEN] = {0};
		err = bma_i2c_read_block(client, BMA2X2_REG_DATAXLOW,
				buf_tmp, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] =
			(s16)(((u16)buf_tmp[1]) << 8 | buf_tmp[0]) >> 4;
		data[BMA_AXIS_Y] =
			(s16)(((u16)buf_tmp[3]) << 8 | buf_tmp[2]) >> 4;
		data[BMA_AXIS_Z] =
			(s16)(((u16)buf_tmp[5]) << 8 | buf_tmp[4]) >> 4;

		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][12bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z],
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z]);
		}
	} else if (priv->sensor_type == BMA280_TYPE) {/* BMA280 */
		u8 buf_tmp[BMA_DATA_LEN] = {0};
		err = bma_i2c_read_block(client, BMA2X2_REG_DATAXLOW,
				buf_tmp, 6);
		if (err) {
			GSE_ERR("[%s]read raw data failed, err = %d\n",
			priv->sensor_name, err);
			return err;
		}
		data[BMA_AXIS_X] =
			(s16)(((u16)buf_tmp[1]) << 8 | buf_tmp[0]) >> 2;
		data[BMA_AXIS_Y] =
			(s16)(((u16)buf_tmp[3]) << 8 | buf_tmp[2]) >> 2;
		data[BMA_AXIS_Z] =
			(s16)(((u16)buf_tmp[5]) << 8 | buf_tmp[4]) >> 2;

		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%s][14bit raw]"
			"[%08X %08X %08X] => [%5d %5d %5d]\n",
			priv->sensor_name,
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z],
			data[BMA_AXIS_X],
			data[BMA_AXIS_Y],
			data[BMA_AXIS_Z]);
		}
	}

#ifdef CONFIG_BMA_LOWPASS
/*
*Example: firlen = 16, filter buffer = [0] ... [15],
*when 17th data come, replace [0] with this new data.
*Then, average this filter buffer and report average value to upper layer.
*/
	if (atomic_read(&priv->filter)) {
		if (atomic_read(&priv->fir_en) &&
				 !atomic_read(&priv->suspend)) {
			int idx, firlen = atomic_read(&priv->firlen);
			if (priv->fir.num < firlen) {
				priv->fir.raw[priv->fir.num][BMA_AXIS_X] =
						data[BMA_AXIS_X];
				priv->fir.raw[priv->fir.num][BMA_AXIS_Y] =
						data[BMA_AXIS_Y];
				priv->fir.raw[priv->fir.num][BMA_AXIS_Z] =
						data[BMA_AXIS_Z];
				priv->fir.sum[BMA_AXIS_X] += data[BMA_AXIS_X];
				priv->fir.sum[BMA_AXIS_Y] += data[BMA_AXIS_Y];
				priv->fir.sum[BMA_AXIS_Z] += data[BMA_AXIS_Z];
				if (atomic_read(&priv->trace)&ADX_TRC_FILTER) {
					GSE_LOG("add [%2d]"
					"[%5d %5d %5d] => [%5d %5d %5d]\n",
					priv->fir.num,
					priv->fir.raw
					[priv->fir.num][BMA_AXIS_X],
					priv->fir.raw
					[priv->fir.num][BMA_AXIS_Y],
					priv->fir.raw
					[priv->fir.num][BMA_AXIS_Z],
					priv->fir.sum[BMA_AXIS_X],
					priv->fir.sum[BMA_AXIS_Y],
					priv->fir.sum[BMA_AXIS_Z]);
				}
				priv->fir.num++;
				priv->fir.idx++;
			} else {
				idx = priv->fir.idx % firlen;
				priv->fir.sum[BMA_AXIS_X] -=
					priv->fir.raw[idx][BMA_AXIS_X];
				priv->fir.sum[BMA_AXIS_Y] -=
					priv->fir.raw[idx][BMA_AXIS_Y];
				priv->fir.sum[BMA_AXIS_Z] -=
					priv->fir.raw[idx][BMA_AXIS_Z];
				priv->fir.raw[idx][BMA_AXIS_X] =
					data[BMA_AXIS_X];
				priv->fir.raw[idx][BMA_AXIS_Y] =
					data[BMA_AXIS_Y];
				priv->fir.raw[idx][BMA_AXIS_Z] =
					data[BMA_AXIS_Z];
				priv->fir.sum[BMA_AXIS_X] +=
					data[BMA_AXIS_X];
				priv->fir.sum[BMA_AXIS_Y] +=
					data[BMA_AXIS_Y];
				priv->fir.sum[BMA_AXIS_Z] +=
					data[BMA_AXIS_Z];
				priv->fir.idx++;
				data[BMA_AXIS_X] =
					priv->fir.sum[BMA_AXIS_X]/firlen;
				data[BMA_AXIS_Y] =
					priv->fir.sum[BMA_AXIS_Y]/firlen;
				data[BMA_AXIS_Z] =
					priv->fir.sum[BMA_AXIS_Z]/firlen;
				if (atomic_read(&priv->trace)&ADX_TRC_FILTER) {
					GSE_LOG("add [%2d]"
					"[%5d %5d %5d] =>"
					"[%5d %5d %5d] : [%5d %5d %5d]\n", idx,
					priv->fir.raw[idx][BMA_AXIS_X],
					priv->fir.raw[idx][BMA_AXIS_Y],
					priv->fir.raw[idx][BMA_AXIS_Z],
					priv->fir.sum[BMA_AXIS_X],
					priv->fir.sum[BMA_AXIS_Y],
					priv->fir.sum[BMA_AXIS_Z],
					data[BMA_AXIS_X],
					data[BMA_AXIS_Y],
					data[BMA_AXIS_Z]);
				}
			}
		}
	}
#endif
	return err;
}

#if 0
/* get hardware offset value from chip register */
static int bma_get_hw_offset(struct i2c_client *client,
		s8 offset[BMA_AXES_NUM + 1])
{
	int err = 0;

	/* HW calibration is under construction */
	GSE_LOG("hw offset x=%x, y=%x, z=%x\n",
	offset[BMA_AXIS_X], offset[BMA_AXIS_Y], offset[BMA_AXIS_Z]);

	return err;
}

/* set hardware offset value to chip register*/
static int bma_set_hw_offset(struct i2c_client *client,
		s8 offset[BMA_AXES_NUM + 1])
{
	int err = 0;

	/* HW calibration is under construction */
	GSE_LOG("hw offset x=%x, y=%x, z=%x\n",
	offset[BMA_AXIS_X], offset[BMA_AXIS_Y], offset[BMA_AXIS_Z]);

	return err;
}
#endif 
static int bma_reset_calibration(struct i2c_client *client)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

#ifdef SW_CALIBRATION

#else
	err = bma_set_hw_offset(client, obj->offset);
	if (err) {
		GSE_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}

static int bma_read_calibration(struct i2c_client *client,
	int act[BMA_AXES_NUM], int raw[BMA_AXES_NUM])
{
	/*
	*raw: the raw calibration data, unmapped;
	*act: the actual calibration data, mapped
	*/
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int mul;

	#ifdef SW_CALIBRATION
	/* only sw calibration, disable hw calibration */
	mul = 0;
	#else
	err = bma_get_hw_offset(client, obj->offset);
	if (err) {
		GSE_ERR("read hw offset failed, %d\n", err);
		return err;
	}
	mul = 1; /* mul = sensor sensitivity / offset sensitivity */
	#endif

	raw[BMA_AXIS_X] =
		obj->offset[BMA_AXIS_X]*mul + obj->cali_sw[BMA_AXIS_X];
	raw[BMA_AXIS_Y] =
		obj->offset[BMA_AXIS_Y]*mul + obj->cali_sw[BMA_AXIS_Y];
	raw[BMA_AXIS_Z] =
		obj->offset[BMA_AXIS_Z]*mul + obj->cali_sw[BMA_AXIS_Z];

	act[obj->cvt.map[BMA_AXIS_X]] =
		obj->cvt.sign[BMA_AXIS_X]*raw[BMA_AXIS_X];
	act[obj->cvt.map[BMA_AXIS_Y]] =
		obj->cvt.sign[BMA_AXIS_Y]*raw[BMA_AXIS_Y];
	act[obj->cvt.map[BMA_AXIS_Z]] =
		obj->cvt.sign[BMA_AXIS_Z]*raw[BMA_AXIS_Z];

	return err;
}

static int bma_write_calibration(struct i2c_client *client,
	int dat[BMA_AXES_NUM])
{
	/* dat array : Android coordinate system, mapped,unit:LSB */
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[BMA_AXES_NUM], raw[BMA_AXES_NUM];

	/*offset will be updated in obj->offset */
	err = bma_read_calibration(client, cali, raw);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG("OLD OFFSET:"
	"unmapped raw offset(%+3d %+3d %+3d),"
	"unmapped hw offset(%+3d %+3d %+3d),"
	"unmapped cali_sw (%+3d %+3d %+3d)\n",
	raw[BMA_AXIS_X], raw[BMA_AXIS_Y], raw[BMA_AXIS_Z],
	obj->offset[BMA_AXIS_X],
	obj->offset[BMA_AXIS_Y],
	obj->offset[BMA_AXIS_Z],
	obj->cali_sw[BMA_AXIS_X],
	obj->cali_sw[BMA_AXIS_Y],
	obj->cali_sw[BMA_AXIS_Z]);

	/* calculate the real offset expected by caller */
	cali[BMA_AXIS_X] += dat[BMA_AXIS_X];
	cali[BMA_AXIS_Y] += dat[BMA_AXIS_Y];
	cali[BMA_AXIS_Z] += dat[BMA_AXIS_Z];

	GSE_LOG("UPDATE: add mapped data(%+3d %+3d %+3d)\n",
		dat[BMA_AXIS_X], dat[BMA_AXIS_Y], dat[BMA_AXIS_Z]);

#ifdef SW_CALIBRATION
	/* obj->cali_sw array : chip coordinate system, unmapped,unit:LSB */
	obj->cali_sw[BMA_AXIS_X] =
		obj->cvt.sign[BMA_AXIS_X]*(cali[obj->cvt.map[BMA_AXIS_X]]);
	obj->cali_sw[BMA_AXIS_Y] =
		obj->cvt.sign[BMA_AXIS_Y]*(cali[obj->cvt.map[BMA_AXIS_Y]]);
	obj->cali_sw[BMA_AXIS_Z] =
		obj->cvt.sign[BMA_AXIS_Z]*(cali[obj->cvt.map[BMA_AXIS_Z]]);
#else
	int divisor = 1; /* divisor = sensor sensitivity / offset sensitivity */
	obj->offset[BMA_AXIS_X] = (s8)(obj->cvt.sign[BMA_AXIS_X]*
		(cali[obj->cvt.map[BMA_AXIS_X]])/(divisor));
	obj->offset[BMA_AXIS_Y] = (s8)(obj->cvt.sign[BMA_AXIS_Y]*
		(cali[obj->cvt.map[BMA_AXIS_Y]])/(divisor));
	obj->offset[BMA_AXIS_Z] = (s8)(obj->cvt.sign[BMA_AXIS_Z]*
		(cali[obj->cvt.map[BMA_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMA_AXIS_X] = obj->cvt.sign[BMA_AXIS_X]*
		(cali[obj->cvt.map[BMA_AXIS_X]])%(divisor);
	obj->cali_sw[BMA_AXIS_Y] = obj->cvt.sign[BMA_AXIS_Y]*
		(cali[obj->cvt.map[BMA_AXIS_Y]])%(divisor);
	obj->cali_sw[BMA_AXIS_Z] = obj->cvt.sign[BMA_AXIS_Z]*
		(cali[obj->cvt.map[BMA_AXIS_Z]])%(divisor);

	GSE_LOG("NEW OFFSET:"
	"unmapped raw offset(%+3d %+3d %+3d),"
	"unmapped hw offset(%+3d %+3d %+3d),"
	"unmapped cali_sw(%+3d %+3d %+3d)\n",
	obj->offset[BMA_AXIS_X]*divisor + obj->cali_sw[BMA_AXIS_X],
	obj->offset[BMA_AXIS_Y]*divisor + obj->cali_sw[BMA_AXIS_Y],
	obj->offset[BMA_AXIS_Z]*divisor + obj->cali_sw[BMA_AXIS_Z],
	obj->offset[BMA_AXIS_X],
	obj->offset[BMA_AXIS_Y],
	obj->offset[BMA_AXIS_Z],
	obj->cali_sw[BMA_AXIS_X],
	obj->cali_sw[BMA_AXIS_Y],
	obj->cali_sw[BMA_AXIS_Z]);

	/* HW calibration is under construction */
	err = bma_set_hw_offset(client, obj->offset);
	if (err) {
		GSE_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif

	return err;
}

/* get chip type */
static int bma_get_chip_type(struct i2c_client *client)
{
	int err = 0;
	u8 chip_id = 0;
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	GSE_FUN(f);
#if 0//wangxiqiang 
int i = 0;
int res = 0;
u8 databuf[2]={0};
memset(databuf, 0, sizeof(u8)*2);    
databuf[0] = BMA_CHIP_ID_REG;  
for(i=200;i<=255;i++)  
{
client->addr = 0x00 +i;
//GSE_ERR("wangxiqiang 600 i2c succeed addr : %x i= %d\n",client->addr,i);
res = i2c_master_send(client, databuf, 0x1);
GSE_ERR("wangxiqiang 600  addr : %x i= %d\n",client->addr,i);
if(res >0)
{
	GSE_ERR("wangxiqiang i2c succeed addr : %x i= %d\n",client->addr,i);
}
else
{
	GSE_ERR("wangxiqiang i2c err i = %d\n",i);
}
}
#endif

	/* twice */
	err = bma_i2c_read_block(client, BMA_CHIP_ID_REG, &chip_id, 0x01);
	err = bma_i2c_read_block(client, BMA_CHIP_ID_REG, &chip_id, 0x01);
	if (err != 0)
		return err;

	switch (chip_id) {
	case BMA220_CHIP_ID:
		obj->sensor_type = BMA220_TYPE;
		strcpy(obj->sensor_name, "bma220");
		break;
	/* same chip id, distinguish them by i2c address */
	case BMA222_BMA250_CHIP_ID:
		if ((client->addr == BMA222_I2C_ADDRESS_PIN_HIGH)
		|| (client->addr == BMA222_I2C_ADDRESS_PIN_LOW)) {
			obj->sensor_type = BMA222_TYPE;
			strcpy(obj->sensor_name, "bma222");
		} else {
			obj->sensor_type = BMA250_TYPE;
			strcpy(obj->sensor_name, "bma250");
		}
		break;
	case BMA222E_CHIP_ID:
		obj->sensor_type = BMA222E_TYPE;
		strcpy(obj->sensor_name, "bma222e");
		break;
	case BMA250E_CHIP_ID:
		obj->sensor_type = BMA250E_TYPE;
		strcpy(obj->sensor_name, "bma250e");
		break;
	case BMA255_CHIP_ID:
		obj->sensor_type = BMA255_TYPE;
		strcpy(obj->sensor_name, "bma255");
		break;
	case BMA280_CHIP_ID:
		obj->sensor_type = BMA280_TYPE;
		strcpy(obj->sensor_name, "bma280");
		break;
	default:
		obj->sensor_type = INVALID_TYPE;
		strcpy(obj->sensor_name, "unknown sensor");
		break;
	}

	GSE_LOG("[%s]chip id = %#x, sensor name = %s\n",
	__func__, chip_id, obj->sensor_name);

	if (obj->sensor_type == INVALID_TYPE) {
		GSE_ERR("unknown gsensor\n");
		return -1;
	}
	return 0;
}

/* set power mode */
static int bma_set_powermode(struct i2c_client *client,
		enum BMA_POWERMODE_ENUM power_mode)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	u8 err = 0, data = 0, actual_power_mode = 0;

	GSE_LOG("[%s] power_mode = %d, old power_mode = %d\n",
	__func__, power_mode, obj->power_mode);

	if (power_mode == obj->power_mode)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMA220_TYPE) {/* BMA220 */
		if (power_mode == BMA_SUSPEND_MODE)
			actual_power_mode = BMA220_SUSPEND_MODE;
		else if (power_mode == BMA_NORMAL_MODE)
			actual_power_mode = BMA220_NORMAL_MODE;
		else {
			err = -EINVAL;
			GSE_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}
		/*
		*read suspend to toggle between suspend
		*and normal operation mode
		*/
		err = bma_i2c_read_block(client, BMA220_SUSPEND__REG, &data, 1);
		/* post-write access timing constraints */
		mdelay(1);
	} else if (obj->sensor_type == BMA222_TYPE) {/* BMA222 */
		if (power_mode == BMA_SUSPEND_MODE) {
			actual_power_mode = BMA222_MODE_SUSPEND;
		} else if (power_mode == BMA_NORMAL_MODE) {
			actual_power_mode = BMA222_MODE_NORMAL;
		} else {
			err = -EINVAL;
			GSE_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}
		err = bma_i2c_read_block(client,
			BMA222_EN_LOW_POWER__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA222_EN_LOW_POWER, 0);/* disable low power mode */
		data = BMA_SET_BITSLICE(data,
			BMA222_EN_SUSPEND, actual_power_mode);
		err += bma_i2c_write_block(client,
			BMA222_EN_LOW_POWER__REG, &data, 1);
	} else if ((obj->sensor_type == BMA222E_TYPE)
		|| (obj->sensor_type == BMA250E_TYPE)
		|| (obj->sensor_type == BMA255_TYPE)
		|| (obj->sensor_type == BMA280_TYPE)) {
		/* BMA222E, BMA250E, BMA255, BMA280 */
		u8 data1 = 0, data2 = 0;
		if (power_mode == BMA_SUSPEND_MODE) {
			actual_power_mode = BMA2X2_MODE_SUSPEND;
		} else if (power_mode == BMA_NORMAL_MODE) {
			actual_power_mode = BMA2X2_MODE_NORMAL;
		} else {
			err = -EINVAL;
			GSE_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_MODE_CTRL_REG, &data1, 1);
		err += bma_i2c_read_block(client,
			BMA2X2_LOW_NOISE_CTRL_REG, &data2, 1);

		switch (actual_power_mode) {
		case BMA2X2_MODE_NORMAL:
			data1 = BMA_SET_BITSLICE(data1,
				BMA2X2_MODE_CTRL, BMA2X2_MODE_NORMAL);
			data2 = BMA_SET_BITSLICE(data2,
				BMA2X2_LOW_POWER_MODE, 0);
			err += bma_i2c_write_block(client,
				BMA2X2_MODE_CTRL_REG, &data1, 1);
			mdelay(1);
			err += bma_i2c_write_block(client,
				BMA2X2_LOW_NOISE_CTRL_REG, &data2, 1);
			mdelay(1);
		break;
		case BMA2X2_MODE_SUSPEND:
			data1 = BMA_SET_BITSLICE(data1,
				BMA2X2_MODE_CTRL, BMA2X2_MODE_SUSPEND);
			data2 = BMA_SET_BITSLICE(data2,
				BMA2X2_LOW_POWER_MODE, 0);
			err += bma_i2c_write_block(client,
				BMA2X2_LOW_NOISE_CTRL_REG, &data2, 1);
			mdelay(1);
			err += bma_i2c_write_block(client,
				BMA2X2_MODE_CTRL_REG, &data1, 1);
			mdelay(1);
		break;
		}
	} else if (obj->sensor_type == BMA250_TYPE) {/* BMA250 */
		if (power_mode == BMA_SUSPEND_MODE) {
			actual_power_mode = BMA250_MODE_SUSPEND;
		} else if (power_mode == BMA_NORMAL_MODE) {
			actual_power_mode = BMA250_MODE_NORMAL;
		} else {
			err = -EINVAL;
			GSE_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}
		err = bma_i2c_read_block(client,
			BMA250_EN_LOW_POWER__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA250_EN_LOW_POWER, 0);/* disable low power mode */
		data = BMA_SET_BITSLICE(data,
			BMA250_EN_SUSPEND, actual_power_mode);
		err += bma_i2c_write_block(client,
			BMA250_EN_LOW_POWER__REG, &data, 1);
	}

	if (err < 0)
		GSE_ERR("set power mode failed, err = %d, sensor name = %s\n",
			err, obj->sensor_name);
	else
		obj->power_mode = power_mode;

	mutex_unlock(&obj->lock);
	return err;
}

static int bma_set_range(struct i2c_client *client, enum BMA_RANGE_ENUM range)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	u8 err = 0, data = 0, actual_range = 0;

	GSE_LOG("[%s] range = %d, old range = %d\n",
	__func__, range, obj->range);

	if (range == obj->range)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMA220_TYPE) {/* BMA220 */
		if (range == BMA_RANGE_2G)
			actual_range = BMA220_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA220_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA220_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client, BMA220_RANGE__REG, &data, 1);
		data = BMA_SET_BITSLICE(data, BMA220_RANGE, actual_range);
		err += bma_i2c_write_block(client, BMA220_RANGE__REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 6bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 16;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 8;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 4;
			break;
			default:
				obj->sensitivity = 16;
			break;
			}
		}
	} else if (obj->sensor_type == BMA222_TYPE) {/* BMA222 */
		if (range == BMA_RANGE_2G)
			actual_range = BMA222_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA222_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA222_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA222_RANGE_SEL__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA222_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA222_RANGE_SEL__REG, &data, 1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 8bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 64;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 32;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 16;
			break;
			default:
				obj->sensitivity = 64;
			break;
			}
		}
	} else if (obj->sensor_type == BMA222E_TYPE) {/* BMA222E */
		if (range == BMA_RANGE_2G)
			actual_range = BMA2X2_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA2X2_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA2X2_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA2X2_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 8bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 64;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 32;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 16;
			break;
			default:
				obj->sensitivity = 64;
			break;
			}
		}
	} else if (obj->sensor_type == BMA250_TYPE) {/* BMA250 */
		if (range == BMA_RANGE_2G)
			actual_range = BMA250_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA250_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA250_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA250_RANGE_SEL__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA250_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA250_RANGE_SEL__REG, &data, 1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 10bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 256;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 128;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 64;
			break;
			default:
				obj->sensitivity = 256;
			break;
			}
		}
	} else if (obj->sensor_type == BMA250E_TYPE) {/* BMA250E */
		if (range == BMA_RANGE_2G)
			actual_range = BMA2X2_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA2X2_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA2X2_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA2X2_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 10bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 256;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 128;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 64;
			break;
			default:
				obj->sensitivity = 256;
			break;
			}
		}
	} else if (obj->sensor_type == BMA255_TYPE) {/* BMA255 */
		if (range == BMA_RANGE_2G)
			actual_range = BMA2X2_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA2X2_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA2X2_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA2X2_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 12bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 1024;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 512;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 256;
			break;
			default:
				obj->sensitivity = 1024;
			break;
			}
		}
	} else if (obj->sensor_type == BMA280_TYPE) {/* BMA280 */
		if (range == BMA_RANGE_2G)
			actual_range = BMA2X2_RANGE_2G;
		else if (range == BMA_RANGE_4G)
			actual_range = BMA2X2_RANGE_4G;
		else if (range == BMA_RANGE_8G)
			actual_range = BMA2X2_RANGE_8G;
		else {
			err = -EINVAL;
			GSE_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA2X2_RANGE_SEL, actual_range);
		err += bma_i2c_write_block(client,
			BMA2X2_RANGE_SEL_REG, &data, 1);
		mdelay(1);

		if (err < 0)
			GSE_ERR("set range failed,"
			"err = %d, sensor name = %s\n", err, obj->sensor_name);
		else {
			obj->range = range;
			/* bitnum: 14bit */
			switch (range) {
			case BMA_RANGE_2G:
				obj->sensitivity = 4096;
			break;
			case BMA_RANGE_4G:
				obj->sensitivity = 2048;
			break;
			case BMA_RANGE_8G:
				obj->sensitivity = 1024;
			break;
			default:
				obj->sensitivity = 4096;
			break;
			}
		}
	}

	mutex_unlock(&obj->lock);
	return err;
}

static int bma_set_datarate(struct i2c_client *client,
		enum BMA_DATARATE_ENUM datarate)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	u8 err = 0, data = 0, bandwidth = 0;

	GSE_LOG("[%s] datarate = %d, old datarate = %d\n",
	__func__, datarate, obj->datarate);

	if (datarate == obj->datarate)
		return 0;

	mutex_lock(&obj->lock);

	if (obj->sensor_type == BMA220_TYPE) {/* BMA220 */
		if ((datarate == BMA_DATARATE_10HZ)
		|| (datarate == BMA_DATARATE_25HZ)
		|| (datarate == BMA_DATARATE_50HZ))
			bandwidth = BMA220_BANDWIDTH_50HZ;
		else if (datarate == BMA_DATARATE_100HZ)
			bandwidth = BMA220_BANDWIDTH_75HZ;
		else if (datarate == BMA_DATARATE_200HZ)
			bandwidth = BMA220_BANDWIDTH_150HZ;
		else {
			err = -EINVAL;
			GSE_ERR("invalid datarate = %d\n", datarate);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA220_SC_FILT_CONFIG__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA220_SC_FILT_CONFIG, bandwidth);
		err += bma_i2c_write_block(client,
			BMA220_SC_FILT_CONFIG__REG, &data, 1);
		mdelay(1);
	} else if (obj->sensor_type == BMA222_TYPE) {/* BMA222 */
		if (datarate == BMA_DATARATE_10HZ)
			bandwidth = BMA222_BW_7_81HZ;
		else if (datarate == BMA_DATARATE_25HZ)
			bandwidth = BMA222_BW_15_63HZ;
		else if (datarate == BMA_DATARATE_50HZ)
			bandwidth = BMA222_BW_31_25HZ;
		else if (datarate == BMA_DATARATE_100HZ)
			bandwidth = BMA222_BW_62_50HZ;
		else if (datarate == BMA_DATARATE_200HZ)
			bandwidth = BMA222_BW_125HZ;
		else {
			err = -EINVAL;
			GSE_ERR("invalid datarate = %d\n", datarate);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA222_BANDWIDTH__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA222_BANDWIDTH, bandwidth);
		err += bma_i2c_write_block(client,
			BMA222_BANDWIDTH__REG, &data, 1);
	} else if ((obj->sensor_type == BMA222E_TYPE)
		|| (obj->sensor_type == BMA250E_TYPE)
		|| (obj->sensor_type == BMA255_TYPE)
		|| (obj->sensor_type == BMA280_TYPE)) {
		/* BMA222E, BMA250E, BMA255, BMA280 */
		if (datarate == BMA_DATARATE_10HZ)
			bandwidth = BMA2X2_BW_7_81HZ;
		else if (datarate == BMA_DATARATE_25HZ)
			bandwidth = BMA2X2_BW_15_63HZ;
		else if (datarate == BMA_DATARATE_50HZ)
			bandwidth = BMA2X2_BW_31_25HZ;
		else if (datarate == BMA_DATARATE_100HZ)
			bandwidth = BMA2X2_BW_62_50HZ;
		else if (datarate == BMA_DATARATE_200HZ)
			bandwidth = BMA2X2_BW_125HZ;
		else {
			err = -EINVAL;
			GSE_ERR("invalid datarate = %d\n", datarate);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA2X2_BANDWIDTH__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA2X2_BANDWIDTH, bandwidth);
		err += bma_i2c_write_block(client,
			BMA2X2_BANDWIDTH__REG, &data, 1);
		mdelay(1);
	} else if (obj->sensor_type == BMA250_TYPE) {/* BMA250 */
		if (datarate == BMA_DATARATE_10HZ)
			bandwidth = BMA250_BW_7_81HZ;
		else if (datarate == BMA_DATARATE_25HZ)
			bandwidth = BMA250_BW_15_63HZ;
		else if (datarate == BMA_DATARATE_50HZ)
			bandwidth = BMA250_BW_31_25HZ;
		else if (datarate == BMA_DATARATE_100HZ)
			bandwidth = BMA250_BW_62_50HZ;
		else if (datarate == BMA_DATARATE_200HZ)
			bandwidth = BMA250_BW_125HZ;
		else {
			err = -EINVAL;
			GSE_ERR("invalid datarate = %d\n", datarate);
			mutex_unlock(&obj->lock);
			return err;
		}

		err = bma_i2c_read_block(client,
			BMA250_BANDWIDTH__REG, &data, 1);
		data = BMA_SET_BITSLICE(data,
			BMA250_BANDWIDTH, bandwidth);
		err += bma_i2c_write_block(client,
			BMA250_BANDWIDTH__REG, &data, 1);
	}

	if (err < 0)
		GSE_ERR("set bandwidth failed,"
		"err = %d,sensor type = %d\n", err, obj->sensor_type);
	else
		obj->datarate = datarate;

	mutex_unlock(&obj->lock);
	return err;
}

/* bma setting initialization */
static int bma_init_client(struct i2c_client *client, int reset_cali)
{
#ifdef CONFIG_BMA_LOWPASS
	struct bma_i2c_data *obj =
		(struct bma_i2c_data *)i2c_get_clientdata(client);
#endif
	int err = 0;
	GSE_FUN(f);

	err = bma_get_chip_type(client);
	if (err < 0) {
		GSE_ERR("get chip type failed, err = %d\n", err);
		return err;
	}

	err = bma_set_datarate(client,
		(enum BMA_DATARATE_ENUM)BMA_DATARATE_10HZ);
	if (err < 0) {
		GSE_ERR("set bandwidth failed, err = %d\n", err);
		return err;
	}

	err = bma_set_range(client, (enum BMA_RANGE_ENUM)BMA_RANGE_2G);
	if (err < 0) {
		GSE_ERR("set range failed, err = %d\n", err);
		return err;
	}

	err = bma_set_powermode(client,
		(enum BMA_POWERMODE_ENUM)BMA_SUSPEND_MODE);
	if (err < 0) {
		GSE_ERR("set power mode failed, err = %d\n", err);
		return err;
	}

	if (0 != reset_cali) {
		/*reset calibration only in power on*/
		err = bma_reset_calibration(client);
		if (err < 0) {
			GSE_ERR("reset calibration failed, err = %d\n", err);
			return err;
		}
	}

#ifdef CONFIG_BMA_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	return 0;
}

/*
*Returns compensated and mapped value. unit is :1000*m/s^2
*/
static int bma_read_sensor_data(struct i2c_client *client,
		char *buf, int bufsize)
{
	struct bma_i2c_data *obj =
		(struct bma_i2c_data *)i2c_get_clientdata(client);
	s16 databuf[BMA_AXES_NUM];
	int acc[BMA_AXES_NUM];
	int err = 0;

	memset(databuf, 0, sizeof(s16)*BMA_AXES_NUM);
	memset(acc, 0, sizeof(int)*BMA_AXES_NUM);

	if (NULL == buf)
		return -1;

	if (NULL == client) {
		*buf = 0;
		return -2;
	}
/*
	if(sensor_suspend == 1)
	{
		//GSE_LOG("sensor in suspend read not data!\n");
		return 0;
	}
*/
	err = bma_read_raw_data(client, databuf);
	if (err) {
		GSE_ERR("bma read raw data failed, err = %d\n", err);
		return -3;
	} else {
		/* compensate data */
		databuf[BMA_AXIS_X] += obj->cali_sw[BMA_AXIS_X];
		databuf[BMA_AXIS_Y] += obj->cali_sw[BMA_AXIS_Y];
		databuf[BMA_AXIS_Z] += obj->cali_sw[BMA_AXIS_Z];

		/* remap coordinate */
		acc[obj->cvt.map[BMA_AXIS_X]] =
			obj->cvt.sign[BMA_AXIS_X]*databuf[BMA_AXIS_X];
		acc[obj->cvt.map[BMA_AXIS_Y]] =
			obj->cvt.sign[BMA_AXIS_Y]*databuf[BMA_AXIS_Y];
		acc[obj->cvt.map[BMA_AXIS_Z]] =
			obj->cvt.sign[BMA_AXIS_Z]*databuf[BMA_AXIS_Z];

		/* convert: LSB -> m/s^2 */
		acc[BMA_AXIS_X] = acc[BMA_AXIS_X] *
			GRAVITY_EARTH_1000 / obj->sensitivity;
		acc[BMA_AXIS_Y] = acc[BMA_AXIS_Y] *
			GRAVITY_EARTH_1000 / obj->sensitivity;
		acc[BMA_AXIS_Z] = acc[BMA_AXIS_Z] *
			GRAVITY_EARTH_1000 / obj->sensitivity;

		sprintf(buf, "%04x %04x %04x",
			acc[BMA_AXIS_X], acc[BMA_AXIS_Y], acc[BMA_AXIS_Z]);
		if (atomic_read(&obj->trace) & ADX_TRC_IOCTL)
			GSE_LOG("gsensor data: %s\n", buf);
	}

	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct bma_i2c_data *obj = obj_i2c_data;

	if (NULL == obj) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", obj->sensor_name);
}

/*
* sensor data format is hex, unit:1000*m/s^2
*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	char strbuf[BMA_BUFSIZE] = "";

	if (NULL == obj) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	bma_read_sensor_data(obj->client, strbuf, BMA_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	int err, len = 0, mul;
	int act[BMA_AXES_NUM], raw[BMA_AXES_NUM];

	if (NULL == obj) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	err = bma_read_calibration(obj->client, act, raw);
	if (err)
		return -EINVAL;
	else {
		mul = 1; /* mul = sensor sensitivity / offset sensitivity */
		len += snprintf(buf+len, PAGE_SIZE-len,
		"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
		mul,
		obj->offset[BMA_AXIS_X],
		obj->offset[BMA_AXIS_Y],
		obj->offset[BMA_AXIS_Z],
		obj->offset[BMA_AXIS_X],
		obj->offset[BMA_AXIS_Y],
		obj->offset[BMA_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len,
		"[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		obj->cali_sw[BMA_AXIS_X],
		obj->cali_sw[BMA_AXIS_Y],
		obj->cali_sw[BMA_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len,
		"[ALL]unmapped(%+3d, %+3d, %+3d), mapped(%+3d, %+3d, %+3d)\n",
		raw[BMA_AXIS_X], raw[BMA_AXIS_Y], raw[BMA_AXIS_Z],
		act[BMA_AXIS_X], act[BMA_AXIS_Y], act[BMA_AXIS_Z]);

		return len;
	}
}

/*
*unit:mapped LSB
*Example:
*	if only force +1 LSB to android z-axis via s/w calibration,
		type command in terminal:
*		>echo 0x0 0x0 0x1 > cali
*	if only force -1(32 bit hex is 0xFFFFFFFF) LSB, type:
*               >echo 0x0 0x0 0xFFFFFFFF > cali
*/
static ssize_t store_cali_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	int err = 0;
	int dat[BMA_AXES_NUM];

	if (NULL == obj) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	if (!strncmp(buf, "rst", 3)) {
		err = bma_reset_calibration(obj->client);
		if (err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (BMA_AXES_NUM == sscanf(buf, "0x%02X 0x%02X 0x%02X",
		&dat[BMA_AXIS_X], &dat[BMA_AXIS_Y], &dat[BMA_AXIS_Z])) {
		err = bma_write_calibration(obj->client, dat);
		if (err) {
			GSE_ERR("bma write calibration failed, err = %d\n",
				err);
		}
	} else {
		GSE_ERR("invalid format\n");
	}

	return count;
}

static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMA_LOWPASS
	struct i2c_client *client = bma222_i2c_client;
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GSE_LOG("[%5d %5d %5d]\n",
			obj->fir.raw[idx][BMA_AXIS_X],
			obj->fir.raw[idx][BMA_AXIS_Y],
			obj->fir.raw[idx][BMA_AXIS_Z]);
		}

		GSE_LOG("sum = [%5d %5d %5d]\n",
			obj->fir.sum[BMA_AXIS_X],
			obj->fir.sum[BMA_AXIS_Y],
			obj->fir.sum[BMA_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n",
			obj->fir.sum[BMA_AXIS_X]/len,
			obj->fir.sum[BMA_AXIS_Y]/len,
			obj->fir.sum[BMA_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

static ssize_t store_firlen_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
#ifdef CONFIG_BMA_LOWPASS
	struct i2c_client *client = bma222_i2c_client;
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if (1 != sscanf(buf, "%d", &firlen)) {
		GSE_ERR("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GSE_ERR("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->firlen, firlen);
		if (NULL == firlen) {
			atomic_set(&obj->fir_en, 0);
		} else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif

	return count;
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct bma_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace))
		atomic_set(&obj->trace, trace);
	else
	#ifdef CONFIG_ARM64
		GSE_ERR("invalid content: '%s', length = %ld\n", buf, count);
	#else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, count);
	#endif
	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	if (obj->hw)
		len += snprintf(buf+len, PAGE_SIZE-len,
		"CUST: %d %d (%d %d)\n",
		obj->hw->i2c_num, obj->hw->direction,
		obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	len += snprintf(buf+len, PAGE_SIZE-len, "i2c addr:%#x,ver:%s\n",
		obj->client->addr, BMA_DRIVER_VERSION);

	return len;
}

static ssize_t show_power_mode_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%s mode\n",
		obj->power_mode == BMA_NORMAL_MODE ? "normal" : "suspend");

	return len;
}

static ssize_t store_power_mode_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	unsigned long power_mode;
	int err;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &power_mode);

	if (err == 0) {
		err = bma_set_powermode(obj->client,
			(enum BMA_POWERMODE_ENUM)(!!(power_mode)));
		if (err)
			return err;
		return count;
	}
	return err;
}

static ssize_t show_range_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->range);

	return len;
}

static ssize_t store_range_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	unsigned long range;
	int err;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &range);

	if (err == 0) {
		if ((range == BMA_RANGE_2G)
		|| (range == BMA_RANGE_4G)
		|| (range == BMA_RANGE_8G)) {
			err = bma_set_range(obj->client, range);
			if (err)
				return err;
			return count;
		}
	}
	return err;
}

static ssize_t show_datarate_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->datarate);

	return len;
}

static ssize_t store_datarate_value(struct device_driver *ddri,
		const char *buf, size_t count)
{
	struct bma_i2c_data *obj = obj_i2c_data;
	unsigned long datarate;
	int err;

	if (obj == NULL) {
		GSE_ERR("bma i2c data pointer is null\n");
		return 0;
	}

	err = kstrtoul(buf, 10, &datarate);

	if (err == 0) {
		if ((datarate == BMA_DATARATE_10HZ)
		|| (datarate == BMA_DATARATE_25HZ)
		|| (datarate == BMA_DATARATE_50HZ)
		|| (datarate == BMA_DATARATE_100HZ)
		|| (datarate == BMA_DATARATE_200HZ)) {
			err = bma_set_datarate(obj->client, datarate);
			if (err)
				return err;
			return count;
		}
	}
	return err;
}

static ssize_t show_selftest(struct device_driver *ddri, char *buf)
{
      struct bma_i2c_data *obj = obj_i2c_data;
      ssize_t len = 0;
      bool result;

      if(!strcmp(obj->sensor_name, "unknown sensor"))
      {
            result = false;
      }
      else
      {
            result = true;
      }
      GSE_LOG("[%s]: result=%d \r\n", __func__, result);

      len += snprintf(buf + len, PAGE_SIZE - len, "%d", result);
      return len;

}

static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO,
		show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO,
		show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powermode, S_IWUSR | S_IRUGO,
		show_power_mode_value, store_power_mode_value);
static DRIVER_ATTR(range, S_IWUSR | S_IRUGO,
		show_range_value, store_range_value);
static DRIVER_ATTR(datarate, S_IWUSR | S_IRUGO,
		show_datarate_value, store_datarate_value);
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO,
		show_selftest,           NULL                  );

static struct driver_attribute *bma_attr_list[] = {
	/* chip information */
	&driver_attr_chipinfo,
	/* dump sensor data */
	&driver_attr_sensordata,
	/* show calibration data */
	&driver_attr_cali,
	/* filter length: 0: disable, others: enable */
	&driver_attr_firlen,
	/* trace flag */
	&driver_attr_trace,
	/* get hw configuration */
	&driver_attr_status,
	/* get power mode */
	&driver_attr_powermode,
	/* get range */
	&driver_attr_range,
	/* get data rate */
	&driver_attr_datarate,
	&driver_attr_selftest,
};

static int bma_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma_attr_list)/sizeof(bma_attr_list[0]));
	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bma_attr_list[idx]);
		if (err) {
			GSE_ERR("driver_create_file (%s) = %d\n",
				bma_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int bma_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma_attr_list)/sizeof(bma_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bma_attr_list[idx]);

	return err;
}

int bma_gsensor_operate(void *self, uint32_t command, void *buff_in, int size_in,
		void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	struct bma_i2c_data *priv = (struct bma_i2c_data *)self;
	struct hwm_sensor_data *gsensor_data;
	char buff[BMA_BUFSIZE];

	switch (command) {
	case SENSOR_DELAY:
	if ((buff_in == NULL) || (size_in < sizeof(int))) {
		GSE_ERR("set delay parameter error\n");
		err = -EINVAL;
	} else {
		value = *(int *)buff_in;

		if (value >= 100)
			sample_delay = BMA_DATARATE_10HZ;
		else if ((value < 100) && (value >= 40))
			sample_delay = BMA_DATARATE_25HZ;
		else if ((value < 40) && (value >= 20))
			sample_delay = BMA_DATARATE_50HZ;
		else if ((value < 20) && (value >= 10))
			sample_delay = BMA_DATARATE_100HZ;
		else
			sample_delay = BMA_DATARATE_200HZ;

		GSE_LOG("sensor delay command: %d, sample_delay = %d\n",
			value, sample_delay);

		err = bma_set_datarate(priv->client, sample_delay);
		if (err < 0)
			GSE_ERR("set delay parameter error\n");

		if (value >= 40)
			atomic_set(&priv->filter, 0);
		else {
		#if defined(CONFIG_BMA_LOWPASS)
			priv->fir.num = 0;
			priv->fir.idx = 0;
			priv->fir.sum[BMA_AXIS_X] = 0;
			priv->fir.sum[BMA_AXIS_Y] = 0;
			priv->fir.sum[BMA_AXIS_Z] = 0;
			atomic_set(&priv->filter, 1);
		#endif
		}
	}
	break;
	case SENSOR_ENABLE:
	if ((buff_in == NULL) || (size_in < sizeof(int))) {
		GSE_ERR("enable sensor parameter error\n");
		err = -EINVAL;
	} else {
		/* value:[0--->suspend, 1--->normal] */
		value = *(int *)buff_in;
		GSE_LOG("sensor enable/disable command: %s\n",
			value ? "enable" : "disable");

		err = bma_set_powermode(priv->client,
			(enum BMA_POWERMODE_ENUM)(!!value));
		if (err)
			GSE_ERR("set power mode failed, err = %d\n", err);
	}
	break;
	case SENSOR_GET_DATA:
	if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
		GSE_ERR("get sensor data parameter error\n");
		err = -EINVAL;
	} else {
		gsensor_data = (struct hwm_sensor_data *)buff_out;
		bma_read_sensor_data(priv->client, buff, BMA_BUFSIZE);
		sscanf(buff, "%x %x %x", &gsensor_data->values[0],
			&gsensor_data->values[1], &gsensor_data->values[2]);
		gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
		gsensor_data->value_divide = 1000;
	}
	break;
	default:
	GSE_ERR("gsensor operate function no this parameter %d\n", command);
	err = -1;
	break;
	}

	return err;
}

static int bma_open(struct inode *inode, struct file *file)
{
	file->private_data = obj_i2c_data;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int bma_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}


#ifdef CONFIG_COMPAT
static long bma_compat_ioctl(struct file *file, unsigned int cmd,
       unsigned long arg)
{
    long err = 0;

	void __user *arg64 = compat_ptr(arg);
//	GSE_ERR("GSENSOR_IOCTL_SET_CALI2071\n");//wangxiqiang
	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;
	
    switch (cmd)
    {
        case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
            if (arg64 == NULL)
            {
                err = -EINVAL;
                break;    
            }
//		GSE_ERR("GSENSOR_IOCTL_SET_CALI2083\n");//wangxiqiang
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg64);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA unlocked_ioctl failed.");
		        return err;
		    }
        break;
        case COMPAT_GSENSOR_IOCTL_SET_CALI:
            if (arg64 == NULL)
            {
                err = -EINVAL;
                break;    
            }
//		GSE_ERR("GSENSOR_IOCTL_SET_CALI2096\n");//wangxiqiang
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_SET_CALI, (unsigned long)arg64);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_SET_CALI unlocked_ioctl failed.");
		        return err;
		    }
        break;
        case COMPAT_GSENSOR_IOCTL_GET_CALI:
            if (arg64 == NULL)
            {
                err = -EINVAL;
                break;    
            }
//		GSE_ERR("GSENSOR_IOCTL_SET_CALI2109\n");//wangxiqiang
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_GET_CALI, (unsigned long)arg64);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_GET_CALI unlocked_ioctl failed.");
		        return err;
		    }
        break;
        case COMPAT_GSENSOR_IOCTL_CLR_CALI:
            if (arg64 == NULL)
            {
                err = -EINVAL;
                break;    
            }
//		GSE_ERR("GSENSOR_IOCTL_SET_CALI2122\n");//wangxiqiang
		    err = file->f_op->unlocked_ioctl(file, GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg64);
		    if (err){
		        GSE_ERR("GSENSOR_IOCTL_CLR_CALI unlocked_ioctl failed.");
		        return err;
		    }
        break;

        default:
            GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
        break;

    }

    return err;
}
#endif


static long bma_unlocked_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct bma_i2c_data *obj = (struct bma_i2c_data *)file->private_data;
	struct i2c_client *client = obj->client;
	char strbuf[BMA_BUFSIZE] = "";
	int raw_offset[BMA_BUFSIZE] = {0};
	s16 raw_data[BMA_AXES_NUM] = {0};
	void __user *data;
	struct SENSOR_DATA sensor_data;
	struct GSENSOR_VECTOR3D gsensor_gain;
	long err = 0;
	int cali[BMA_AXES_NUM];

	if (obj == NULL)
		return -EFAULT;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
			(void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
			(void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GSE_ERR("access error: %08x, (%2d, %2d)\n",
			cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
	bma_init_client(client, 0);
	err = bma_set_powermode(client, BMA_NORMAL_MODE);
	if (err) {
		err = -EFAULT;
		break;
	}
	break;
	case GSENSOR_IOCTL_READ_CHIPINFO:
	data = (void __user *) arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}

	strcpy(strbuf, obj->sensor_name);
	if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
		err = -EFAULT;
		break;
	}
	break;
	case GSENSOR_IOCTL_READ_SENSORDATA:
	data = (void __user *) arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}

	bma_read_sensor_data(client, strbuf, BMA_BUFSIZE);
	if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
		err = -EFAULT;
		break;
	}
	break;
	case GSENSOR_IOCTL_READ_GAIN:
	data = (void __user *) arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->sensitivity;
	if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
		err = -EFAULT;
		break;
	}
	break;
	case GSENSOR_IOCTL_READ_RAW_DATA:
	data = (void __user *) arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}

	err = bma_read_raw_data(client, raw_data);
	if (err) {
		err = -EFAULT;
		break;
	}
	sprintf(strbuf, "%04x %04x %04x",
			raw_data[BMA_AXIS_X],
			raw_data[BMA_AXIS_X],
			raw_data[BMA_AXIS_X]);

	if (copy_to_user(data, &strbuf, strlen(strbuf) + 1)) {
		err = -EFAULT;
		break;
	}
	break;
	case GSENSOR_IOCTL_SET_CALI:
	/* data unit is m/s^2 */
	data = (void __user *)arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}
	if (copy_from_user(&sensor_data, data, sizeof(sensor_data))) {
		err = -EFAULT;
		break;
	}
	if (atomic_read(&obj->suspend)) {
		GSE_ERR("perform calibration in suspend mode\n");
		err = -EINVAL;
	} else {
		/* convert: m/s^2 -> LSB */
		cali[BMA_AXIS_X] = sensor_data.x *
			obj->sensitivity / GRAVITY_EARTH_1000;
		cali[BMA_AXIS_Y] = sensor_data.y *
			obj->sensitivity / GRAVITY_EARTH_1000;
		cali[BMA_AXIS_Z] = sensor_data.z *
			obj->sensitivity / GRAVITY_EARTH_1000;
		err = bma_write_calibration(client, cali);
	}
	break;
	case GSENSOR_IOCTL_CLR_CALI:
	err = bma_reset_calibration(client);
	break;
	case GSENSOR_IOCTL_GET_CALI:
	data = (void __user *)arg;
	if (data == NULL) {
		err = -EINVAL;
		break;
	}
	err = bma_read_calibration(client, cali, raw_offset);
	if (err)
		break;

	sensor_data.x = cali[BMA_AXIS_X] *
		GRAVITY_EARTH_1000 / obj->sensitivity;
	sensor_data.y = cali[BMA_AXIS_Y] *
		GRAVITY_EARTH_1000 / obj->sensitivity;
	sensor_data.z = cali[BMA_AXIS_Z] *
		GRAVITY_EARTH_1000 / obj->sensitivity;
	if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
		err = -EFAULT;
		break;
	}
	break;
	default:
	GSE_ERR("unknown IOCTL: 0x%08x\n", cmd);
	err = -ENOIOCTLCMD;
	break;
	}

	return err;
}

static const struct file_operations bma_fops = {
	.owner = THIS_MODULE,
	.open = bma_open,
	.release = bma_release,
	.unlocked_ioctl = bma_unlocked_ioctl,
	#ifdef CONFIG_COMPAT
        .compat_ioctl  =   bma_compat_ioctl
	#endif
};

static struct miscdevice bma_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &bma_fops,
};

#ifndef CONFIG_HAS_EARLYSUSPEND 
static int bma_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	GSE_FUN();

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GSE_ERR("null pointer\n");
			return -EINVAL;
		}

		atomic_set(&obj->suspend, 1);
		err = bma_set_powermode(obj->client, BMA_SUSPEND_MODE);
		if (err) {
			GSE_ERR("bma set suspend mode failed, err = %d\n", err);
			return err;
		}
		bma_power(obj->hw, 0);
	}
	return err;
}

static int bma_resume(struct i2c_client *client)
{
	struct bma_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer\n");
		return -EINVAL;
	}

	bma_power(obj->hw, 1);
	err = bma_init_client(client, 0);
	if (err) {
		GSE_ERR("initialize client failed, err = %d\n", err);
		return err;
	}

	atomic_set(&obj->suspend, 0);
	return 0;
}
#else
static void bma_early_suspend(struct early_suspend *h) 
{
	struct bma_i2c_data *obj = container_of(h, struct bma_i2c_data, early_drv);   
	int err;
	GSE_FUN();    

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1); 
	if(err = bma_set_powermode(obj->client, false))
	{
		GSE_ERR("write power control fail!!\n");
		return;
	}

	sensor_suspend = 1;
	
	bma_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bma_late_resume(struct early_suspend *h)
{
	struct bma_i2c_data *obj = container_of(h, struct bma_i2c_data, early_drv);         
	int err;
	GSE_FUN();

	if(obj == NULL)
	{
		GSE_ERR("null pointer!!\n");
		return;
	}

	bma_power(obj->hw, 1);
	if(err = bma_init_client(obj->client, 0))
	{
		GSE_ERR("initialize client fail!!\n");
		return;        
	}
	sensor_suspend = 0;
	atomic_set(&obj->suspend, 0);    
}


#endif

////////////////////////////////////////wangxiqiang modify 
// if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL
static int bma_gsensor_open_report_data(int open)
{
	//should queuq work to report event if  is_report_input_direct=true
	return 0;
}
/*----------------------------------------------------------------------------*/
// if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL
static int bma_gsensor_enable_nodata(int en)
{
    	int res =0;
	int retry = 0;
	bool power=false;
	
	if(1==en)
	{
		power=true;
	}
	if(0==en)
	{
		power =false;
	}

	for(retry = 0; retry < 3; retry++){
		res = bma_set_powermode(obj_i2c_data->client, power);
		if(res == 0)
		{
			GSE_LOG("bma_set_powermode done\n");
			break;
		}
		GSE_LOG("bma_set_powermode fail\n");
	}

	
	if(res != BMA222_SUCCESS)
	{
		GSE_LOG("bma_set_powermode fail!\n");
		return -1;
	}
	GSE_LOG("bma222_enable_nodata OK!\n");
	return 0;
}
/*----------------------------------------------------------------------------*/
/*
static DEFINE_MUTEX(read_lock);
static int bma220_read_reg_value(u8 regnum)
{
	u8 regvalue[1], buffer[1];
	int res = 0;
	mutex_lock(&read_lock);
	buffer[0] = regnum;
	printk("wangxiqiang show_reg_value 1184\n");
	res = i2c_master_send(obj_i2c_data->client,buffer,0x01);
	if(res <= 0){
		mutex_unlock(&read_lock);
		printk("wangxiqiang read reg send res = %d\n",res);
		return res;
	}
	printk("wangxiqiang show_reg_value 1191\n");
	res = i2c_master_recv(obj_i2c_data->client,regvalue,0x01);
	if(res <= 0){
		mutex_unlock(&read_lock);
		printk("wangxiqiang read reg recv res = %d\n",res);
		return res;
	}

	mutex_unlock(&read_lock);
	printk("wangxiqiang show_reg_value 1200 regvalue = 0x%04X\n",regvalue[0]);
	return regvalue[0];
	
}
*/

static int bma_gsensor_set_delay(u64 ns)
{
    int err = 0;
    int value;
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
#else//#ifdef CUSTOM_KERNEL_SENSORHUB
	//int sample_delay;
#endif//#ifdef CUSTOM_KERNEL_SENSORHUB

    value = (int)ns/1000/1000;

#ifdef CUSTOM_KERNEL_SENSORHUB
    req.set_delay_req.sensorType = ID_ACCELEROMETER;
    req.set_delay_req.action = SENSOR_HUB_SET_DELAY;
    req.set_delay_req.delay = value;
    len = sizeof(req.activate_req);
    err = SCP_sensorHub_req_send(&req, &len, 1);
    if (err)
    {
        GSE_ERR("SCP_sensorHub_req_send!\n");
        return err;
    }
#else//#ifdef CUSTOM_KERNEL_SENSORHUB    
/*
	if(value <= 5)
	{
		sample_delay = BMA220_BW_200HZ;
	}
	else if(value <= 10)
	{
		sample_delay = BMA220_BW_100HZ;
	}
	else
	{
		//sample_delay = BMA220_BW_100HZ;
		sample_delay = BMA220_BW_25HZ;//wangxiqiang 
	}
*/
	mutex_lock(&gsensor_scp_en_mutex);
	err = bma_set_datarate(obj_i2c_data->client,
		(enum BMA_DATARATE_ENUM)BMA_DATARATE_10HZ);
	//if (err < 0) {
	//	GSE_ERR("set bandwidth failed, err = %d\n", err);
	//	return err;
	//}

	//err = BMA220_SetBWRate(obj_i2c_data->client, sample_delay);
	mutex_unlock(&gsensor_scp_en_mutex);
	if(err != BMA222_SUCCESS ) //0x2C->BW=100Hz
	{
		GSE_ERR("Set delay parameter error!\n");
        return -1;
	}

	if(value >= 50)
	{
		atomic_set(&obj_i2c_data->filter, 0);
	}
	else
	{	
	#if defined(CONFIG_BMA220_LOWPASS)
		priv->fir.num = 0;
		priv->fir.idx = 0;
		priv->fir.sum[BMA220_AXIS_X] = 0;
		priv->fir.sum[BMA220_AXIS_Y] = 0;
		priv->fir.sum[BMA220_AXIS_Z] = 0;
		atomic_set(&priv->filter, 1);
	#endif
	}
#endif//#ifdef CUSTOM_KERNEL_SENSORHUB
   // bma220_read_reg_value(0x20);
    GSE_LOG("gsensor_set_delay (%d)\n",value);

	return 0;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int bma_gsensor_get_data(int* x ,int* y,int* z, int* status)
{
#ifdef CUSTOM_KERNEL_SENSORHUB
    SCP_SENSOR_HUB_DATA req;
    int len;
	int err = 0;
#else
	char buff[BMA_BUFSIZE];
#endif
GSE_FUN();
#ifdef CUSTOM_KERNEL_SENSORHUB
    req.get_data_req.sensorType = ID_ACCELEROMETER;
    req.get_data_req.action = SENSOR_HUB_GET_DATA;
	len = sizeof(req.get_data_req);
	err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err)
	{
	    GSE_ERR("SCP_sensorHub_req_send!\n");
		return err;
	}
	
	if (ID_ACCELEROMETER != req.get_data_rsp.sensorType ||
		SENSOR_HUB_GET_DATA != req.get_data_rsp.action ||
		0 != req.get_data_rsp.errCode)
	{
	    GSE_ERR("error : %d\n", req.get_data_rsp.errCode);
		return req.get_data_rsp.errCode;
	}

	*x = req.get_data_rsp.int16_Data[0];
	*y = req.get_data_rsp.int16_Data[1];
	*z = req.get_data_rsp.int16_Data[2];
	GSE_ERR("x = %d, y = %d, z = %d\n", *x, *y, *z);

	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
#else
    mutex_lock(&gsensor_scp_en_mutex);
	bma_read_sensor_data(obj_i2c_data->client, buff, BMA_BUFSIZE);
	mutex_unlock(&gsensor_scp_en_mutex);
	sscanf(buff, "%x %x %x", x, y, z);				
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
  #endif
	return 0;
}
////////////////////////////////////////
#if 0
static int bma_i2c_detect(struct i2c_client *client,
		struct i2c_board_info *info)
{
	strcpy(info->type, BMA_DEV_NAME);
	return 0;
}

#endif 

static int bma_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct bma_i2c_data *obj;
	//struct hwmsen_object sobj;
    struct acc_control_path ctl={0}; //wangxiqiang modify 
    struct acc_data_path data={0};
	int err = 0;
	GSE_FUN();

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	GSE_FUN();
	if (!obj) {
		err = -ENOMEM;
	GSE_FUN();
		goto exit;
	}
	GSE_FUN();
	memset(obj, 0, sizeof(struct bma_i2c_data));
	GSE_FUN();	
	//obj->hw = bma222_get_cust_acc_hw();
	obj->hw = hw;
	GSE_FUN();	
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	GSE_FUN();
	if (err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		GSE_FUN();
		goto exit_hwmsen_get_convert_failed;
	}
	GSE_FUN();
	obj_i2c_data = obj;
GSE_FUN();
	obj->client = client;
GSE_FUN();
	i2c_set_clientdata(client, obj);
GSE_FUN();
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	obj->power_mode = BMA_UNDEFINED_POWERMODE;
	obj->range = BMA_UNDEFINED_RANGE;
	obj->datarate = BMA_UNDEFINED_DATARATE;
	mutex_init(&obj->lock);

#ifdef CONFIG_BMA_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);
#endif

	err = bma_init_client(client, 1);
	if (err)
		goto exit_init_client_failed;

	err = misc_register(&bma_device);
	if (err) {
		GSE_ERR("misc device register failed, err = %d\n", err);
		goto exit_misc_device_register_failed;
	}


	if((err = bma_create_attr(&(bma_init_info.platform_diver_addr->driver))))
	{
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
/*
	err = bma_create_attr(&bma_gsensor_driver.driver);
	if (err) {
		GSE_ERR("create attribute failed, err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
	sobj.polling = 1;
	sobj.sensor_operate = gsensor_operate;

	err = hwmsen_attach(ID_ACCELEROMETER, &sobj);
	if (err) {
		GSE_ERR("hwmsen attach failed, err = %d\n", err);
		goto exit_hwmsen_attach_failed;
	}
*/

    ctl.open_report_data= bma_gsensor_open_report_data;
    ctl.enable_nodata = bma_gsensor_enable_nodata;
    ctl.set_delay  = bma_gsensor_set_delay;
    ctl.is_report_input_direct = false;

#ifdef CUSTOM_KERNEL_SENSORHUB
    ctl.is_support_batch = obj->hw->is_batch_supported;
#else
    ctl.is_support_batch = false;
#endif
    err = acc_register_control_path(&ctl);
    if(err)
    {
        GSE_ERR("register acc control path err\n");
        goto exit_kfree;
    }
    data.get_data = bma_gsensor_get_data;
    data.vender_div = 1000;
    err = acc_register_data_path(&data);
    if(err)
    {
        GSE_ERR("register acc data path err\n");
        goto exit_kfree;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND

	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = bma_early_suspend,
	obj->early_drv.resume   = bma_late_resume,    
	register_early_suspend(&obj->early_drv);
#endif 
/*
    err = batch_register_support_info(ID_ACCELEROMETER,ctl.is_support_batch, 1000, 0);
    if(err)
    {
	    GSE_ERR("register gsensor batch support err = %d\n", err);
	     goto exit_create_attr_failed;
     }
*/

	bma_gsensor_init_flag = 0;
	GSE_LOG("%s: OK\n", __func__);
	return 0;

//exit_hwmsen_attach_failed:
	//bma_delete_attr(&bma_gsensor_driver.driver);
exit_create_attr_failed:
	misc_deregister(&bma_device);
exit_misc_device_register_failed:
exit_init_client_failed:
exit_hwmsen_get_convert_failed:
exit_kfree:
	kfree(obj);
exit:
	bma_gsensor_init_flag = -1;
	GSE_ERR("err = %d\n", err);
	return err;
}


#if 1//wangxiqiang modify 
//////////////////////////////////////

static int  bma_gsensor_local_init(void)  //wangxiqiang
{
    //struct acc_hw *hw = get_cust_acc_hw(); bma222e_get_cust_acc_hw
	//struct acc_hw *hw = bma222_get_cust_acc_hw();
        GSE_FUN();
	
	bma_power(hw, 1);
	GSE_LOG("wangxiqiang 1938" );
	if(i2c_add_driver(&bma_i2c_driver))
	{
		GSE_ERR("add driver error\n");
		return -1;
	}
	GSE_LOG("wangxiqiang 1934" );
	if(-1 == bma_gsensor_init_flag)
	{
	   return -1;
	}
	printk("fwq loccal init---\n");
	return 0;
}

static int bma_gsensor_remove()
{
    //struct acc_hw *hw = get_cust_acc_hw();
	//struct acc_hw *hw = bma222_get_cust_acc_hw();
    GSE_FUN();    
    bma_power(hw, 0);    
    i2c_del_driver(&bma_i2c_driver);
    return 0;
}
#endif 
///////////////////////
static int bma_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = hwmsen_detach(ID_ACCELEROMETER);
	if (err)
		GSE_ERR("hwmsen_detach failed, err = %d\n", err);

	err = bma_delete_attr(&bma_gsensor_driver.driver);
	if (err)
		GSE_ERR("bma_delete_attr failed, err = %d\n", err);

	err = misc_deregister(&bma_device);
	if (err)
		GSE_ERR("misc_deregister failed, err = %d\n", err);

	obj_i2c_data = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static int bma_probe(struct platform_device *pdev)
{
	//struct acc_hw *hw = bma222_get_cust_acc_hw();
	GSE_FUN();

	bma_power(hw, 1);
	if (i2c_add_driver(&bma_i2c_driver)) {
		GSE_ERR("add i2c driver failed\n");
		return -1;
	}

	return 0;
}

static int bma_remove(struct platform_device *pdev)
{
//	struct acc_hw *hw = bma222_get_cust_acc_hw();
	GSE_FUN();

	bma_power(hw, 0);
	i2c_del_driver(&bma_i2c_driver);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif


static struct i2c_driver bma_i2c_driver = {
	.driver = {
		.name = BMA_DEV_NAME,
	#ifdef CONFIG_OF
	.of_match_table = accel_of_match,
	#endif 
	},
	.probe = bma_i2c_probe,
	.remove	= bma_i2c_remove,
	//.detect	= bma_i2c_detect,
#if !defined(CONFIG_HAS_EARLYSUSPEND) 
	.suspend = bma_suspend,
	.resume = bma_resume,
#endif
	.id_table = bma_i2c_id,
};

static struct platform_driver bma_gsensor_driver = {
	.probe      = bma_probe,
	.remove     = bma_remove,
	.driver     = {
		.name   = "gsensor",
		.owner  = THIS_MODULE,
	}
};

static int __init bma_init(void)
{
	const char *name = "mediatek,bma2xx";	
	//struct acc_hw *hw = bma222_get_cust_acc_hw();
	GSE_FUN();
	GSE_LOG("%s: bosch accelerometer driver version: %s\n",
	__func__, BMA_DRIVER_VERSION);

	//i2c_register_board_info(hw->i2c_num, &bma_i2c_info, 1);
	
	hw = get_accel_dts_func(name, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");
#if  1//wangxiqiang modify 
 	acc_driver_add(&bma_init_info);
#else
	if (platform_driver_register(&bma_gsensor_driver)) {
		GSE_ERR("register gsensor platform driver failed\n");
		return -ENODEV;
	}
#endif

	return 0;
}

static void __exit bma_exit(void)
{
	GSE_FUN();
#if 0  //wangxiqiang modify 
	platform_driver_unregister(&bma_gsensor_driver);
#endif
}

module_init(bma_init);
module_exit(bma_exit);

MODULE_LICENSE("GPLv2");
MODULE_DESCRIPTION("BMA I2C Driver");
MODULE_AUTHOR("deliang.tao@bosch-sensortec.com");
MODULE_VERSION(BMA_DRIVER_VERSION);

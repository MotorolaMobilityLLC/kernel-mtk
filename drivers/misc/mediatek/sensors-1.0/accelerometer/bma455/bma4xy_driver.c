/* BOSCH BMA4XY ACC Sensor Driver
 * (C) Copyright 2011~2017 Bosch Sensortec GmbH All Rights Reserved
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 * VERSION: v0.0.2.5
 * Date: 2017/05/08
 */
#define DRIVER_VERSION "0.0.2.5"
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/kobject.h>
#include <linux/atomic.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/uaccess.h>
#include <cust_acc.h>
#include <accel.h>
#include <step_counter.h>
#include "bma4xy_driver.h"
//#include <linux/hwmsensor.h>
//#include <linux/hwmsen_dev.h>
//#include <linux/sensors_io.h>
//#include <linux/hwmsen_helper.h>
//#include <linux/batch.h>
#include <wake_gesture.h>

#define SW_CALIBRATION
#define BMA4XY_DEV_NAME        "bma455_acc"
static struct i2c_client *bma4xy_i2c_client;
static const struct i2c_device_id bma4xy_i2c_id[] = {
	{BMA4XY_DEV_NAME, 0},
	{} };
#define COMPATIABLE_NAME "mediatek,bma455"
#if !defined(BMA420) && !defined(BMA456)
#define BMA4_STEP_COUNTER 1
#endif
#if defined(BMA422) || defined(BMA455) || defined(BMA424)
#define BMA4_WAKEUP 1
#endif
static int bma4xy_i2c_probe(
struct i2c_client *client, const struct i2c_device_id *id);
#if !defined(BMA424SC) && !defined(BMA424)
//static struct i2c_board_info __initdata i2c_bma4xy = {
//	I2C_BOARD_INFO(BMA4XY_DEV_NAME, 0x19)};//0x18
#else
static struct i2c_board_info __initdata i2c_bma4xy = {
	I2C_BOARD_INFO(BMA4XY_DEV_NAME, 0x19)};
#endif
static int bma4xy_i2c_remove(struct i2c_client *client);
static int bma4xy_i2c_suspend(struct device *dev);
static int bma4xy_i2c_resume(struct device *dev);
static int gsensor_local_init(void);
static int gsensor_remove(void);
static int gsensor_set_delay(u64 ns);

/*----------------------------------------------------------------------------*/

#if defined(BMA420) || defined(BMA421) || defined(BMA421L) || defined(BMA422)
static struct data_resolution bma4xy_acc_data_resolution[1] = {
	{{1, 95}, 512},	/*+/-4g  in 12-bit resolution:  1.95 mg/LSB*/
};
static struct data_resolution bma4xy_acc_offset_resolution = {{1, 95}, 512};
#elif defined(BMA455) || defined(BMA424) || defined(BMA424SC) || defined(BMA455N)
static struct data_resolution bma4xy_acc_data_resolution[1] = {
	{{0, 12}, 8192},	/*+/-4g  in 16-bit resolution:  0.12 mg/LSB*/
};
static struct data_resolution bma4xy_acc_offset_resolution = {{0, 12}, 8192};
#endif

#if DEBUG
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               pr_err(GSE_TAG"%s\n", __func__)
#define GSE_ERR(fmt, args...) \
pr_err(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    pr_err(GSE_TAG fmt, ##args)
#else
#define GSE_TAG
#define GSE_FUN(f)
#define GSE_ERR(fmt, args...)
#define GSE_LOG(fmt, args...)
#endif

static bool enable_status;
static struct acc_hw bma4_accel_cust;
static struct acc_hw *hw = &bma4_accel_cust;
static struct bma4xy_client_data *bma4_obj_i2c_data;
static bool sensor_power = true;
static int sensor_suspend;
static struct GSENSOR_VECTOR3D gsensor_gain;
static DEFINE_MUTEX(gsensor_mutex);
static int gsensor_init_flag = -1;	/* 0<==>OK -1 <==> fail */
#define ERRNUM1 -1
#define ERRNUM2 -2
#define ERRNUM3 -3
#define CHIP_ID_ERR -4

enum BMA4XY_SENSOR_INT_MAP {
	BMA4XY_FFULL_INT = 8,
	BMA4XY_FWM_INT = 9,
	BMA4XY_DRDY_INT = 10,
};

enum BMA4XY_CONFIG_FUN {
	BMA4XY_SIG_MOTION_SENSOR = 0,
	BMA4XY_STEP_DETECTOR_SENSOR = 1,
	BMA4XY_STEP_COUNTER_SENSOR = 2,
	BMA4XY_TILT_SENSOR = 3,
	BMA4XY_PICKUP_SENSOR = 4,
	BMA4XY_GLANCE_DETECTOR_SENSOR = 5,
	BMA4XY_WAKEUP_SENSOR = 6,
	BMA4XY_ANY_MOTION_SENSOR = 7,
	BMA4XY_ORIENTATION_SENSOR = 8,
	BMA4XY_FLAT_SENSOR = 9,
	BMA4XY_TAP_SENSOR = 10,
	BMA4XY_HIGH_G_SENSOR = 11,
	BMA4XY_LOW_G_SENSOR = 12,
	BMA4XY_ACTIVITY_SENSOR = 13,
	BMA4XY_NO_MOTION_SENSOR = 14,
};

enum BMA4XY_INT_STATUS0 {
	SIG_MOTION_OUT = 0x01,
	STEP_DET_OUT = 0x02,
	TILT_OUT = 0x04,
	PICKUP_OUT = 0x08,
	GLANCE_OUT = 0x10,
	WAKEUP_OUT = 0x20,
	ANY_NO_MOTION_OUT = 0x40,
	ERROR_INT_OUT = 0x80,
};

enum BMA4XY_INT_STATUS1 {
	FIFOFULL_OUT = 0x01,
	FIFOWATERMARK_OUT = 0x02,
	MAG_DRDY_OUT = 0x20,
	ACC_DRDY_OUT = 0x80,
};

/*bma4 fifo analyse return err status*/
enum BMA4_FIFO_ANALYSE_RETURN_T {
	FIFO_OVER_READ_RETURN = -10,
	FIFO_SENSORTIME_RETURN = -9,
	FIFO_SKIP_OVER_LEN = -8,
	FIFO_M_A_OVER_LEN = -5,
	FIFO_M_OVER_LEN = -3,
	FIFO_A_OVER_LEN = -1
};

//+add by hzb for ontim debug
#include <ontim/ontim_dev_dgb.h>
static char version[]="bma455-1.0";
static char vendor_name[20]="bma455-Bosch";
    DEV_ATTR_DECLARE(gsensor)
    DEV_ATTR_DEFINE("version",version)
    DEV_ATTR_DEFINE("vendor",vendor_name)
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(gsensor,gsensor,8);
//-add by hzb for ontim debug

static void bma4xy_i2c_delay(u32 msec)
{
	if (msec <= 20)
		usleep_range(msec * 1000, msec * 1000 + 1000);
	else
		msleep(msec);
}
//#define MTK_NOT_SUPPORT_I2C_RW_MORETHAN_8_BYTES 1
#if defined(MTK_NOT_SUPPORT_I2C_RW_MORETHAN_8_BYTES)
/* For DMA */
#include <linux/dma-mapping.h>
static u8 *I2CDMABuf_va;
static dma_addr_t I2CDMABuf_pa;
#define I2C_MASTER_CLOCK 400

static int bma4xy_i2c_dma_read(struct i2c_client *client,
	uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int ret;
	s32 retry = 0;
	u8 buffer[2];
	struct i2c_msg msg[2] = {
	{
		.addr = (client->addr & I2C_MASK_FLAG),
		/*(client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),*/
		.flags = 0,
		.buf = buffer,
		.len = 1,
		.timing = I2C_MASTER_CLOCK
	},
	{
		.addr = (client->addr & I2C_MASK_FLAG),
		.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
		.flags = I2C_M_RD,
		.buf = (u8 *)I2CDMABuf_pa,
		.len = len,
		.timing = I2C_MASTER_CLOCK
	},
	};
	buffer[0] = reg_addr;
	if (data == NULL)
		return -EIO;
	for (retry = 0; retry < 10; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0) {
			GSE_ERR("I2C DMA read error retry=%d", retry);
			continue;
		}
		memcpy(data, I2CDMABuf_va, len);
		return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d",
	buffer[0], len, ret);
	return ret;
}

static int bma4xy_i2c_dma_write(struct i2c_client *client,
	uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	int ret;
	s32 retry = 0;
	u8 *wr_buf = I2CDMABuf_va;

	struct i2c_msg msg = {
		.addr = (client->addr & I2C_MASK_FLAG),
		.ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
		.flags = 0,
		.buf = (u8 *)I2CDMABuf_pa,
		.len = 1+len,
		.timing = I2C_MASTER_CLOCK
	};
	wr_buf[0] = reg_addr;
	if (data == NULL)
		return -EIO;
	memcpy(wr_buf+1, data, len+1);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0) {
			GSE_ERR("I2C DMA write error retry=%d", retry);
			continue;
		}
		return 0;
	}
	GSE_ERR("Dma I2C Write Error: 0x%04X, %d byte(s), err-code: %d",
	reg_addr, len, ret);
	return ret;
}

#endif
static int bma4xy_i2c_read(struct i2c_client *client,
	uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int32_t retry;
	struct i2c_msg msg[] = {
		{
		.addr = client->addr,
		.flags = 0,
		.len = 1,
		.buf = &reg_addr,
		},

		{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = len,
		.buf = data,
		},
	};
	for (retry = 0; retry < BMA4XY_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) > 0)
			break;
		else
			usleep_range(BMA4XY_I2C_WRITE_DELAY_TIME * 1000,
				BMA4XY_I2C_WRITE_DELAY_TIME * 1000);
	}

	if (BMA4XY_MAX_RETRY_I2C_XFER <= retry) {
		GSE_ERR("I2C xfer error");
		return -EIO;
	}

	return 0;
}

static int bma4xy_i2c_write(struct i2c_client *client,
	uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	int32_t retry;

	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = len + 1,
		.buf = NULL,
	};
	msg.buf = kmalloc(len + 1, GFP_KERNEL);
	if (!msg.buf) {
		GSE_ERR("Allocate mem failed\n");
		return -ENOMEM;
	}
	msg.buf[0] = reg_addr;
	memcpy(&msg.buf[1], data, len);
	for (retry = 0; retry < BMA4XY_MAX_RETRY_I2C_XFER; retry++) {
		if (i2c_transfer(client->adapter, &msg, 1) > 0)
			break;
		else
			usleep_range(BMA4XY_I2C_WRITE_DELAY_TIME * 1000,
				BMA4XY_I2C_WRITE_DELAY_TIME * 1000);
	}
	kfree(msg.buf);
	if (BMA4XY_MAX_RETRY_I2C_XFER <= retry) {
		GSE_ERR("I2C xfer error");
		return -EIO;
	}

	return 0;
}

static s8 bma4xy_i2c_read_wrapper(uint8_t dev_addr,
	uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int err;
	#if defined(MTK_NOT_SUPPORT_I2C_RW_MORETHAN_8_BYTES)
	if (len > 8) {
		err = bma4xy_i2c_dma_read(
			bma4xy_i2c_client, reg_addr, data, len);
		return err;
	}
	#endif
	err = bma4xy_i2c_read(bma4xy_i2c_client, reg_addr, data, len);
	return err;
}

static s8 bma4xy_i2c_write_wrapper(uint8_t dev_addr,
	uint8_t reg_addr, uint8_t *data, uint16_t len)
{
	int err;
	#if defined(MTK_NOT_SUPPORT_I2C_RW_MORETHAN_8_BYTES)
	if (len > 8) {
		err = bma4xy_i2c_dma_write(
			bma4xy_i2c_client, reg_addr, data, len);
		return err;
	}
	#endif
	err = bma4xy_i2c_write(bma4xy_i2c_client, reg_addr, data, len);
	return err;
}

static void BMA4XY_power(struct acc_hw *hw, unsigned int on)
{
}

static int BMA4XY_SetDataResolution(struct bma4xy_client_data *obj)
{
	obj->reso = &bma4xy_acc_data_resolution[0];
	return 0;
}
static int BMA4XY_ReadChipInfo(
	struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8) * 10);

	if ((NULL == buf) || (bufsize <= 30))
		return ERRNUM1;

	if (NULL == client) {
		*buf = 0;
		return ERRNUM2;
	}

	snprintf(buf, 96, "BMA4XY Chip");
	return 0;
}

static int BMA4XY_ReadData(
	struct i2c_client *client, s16 data[BMA4XY_ACC_AXIS_NUM])
{
	int err = 0;
	struct bma4_accel raw_data;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	if (NULL == client)
		return -EINVAL;
	err = bma455_read_accel_xyz(&raw_data, &client_data->device);
	if (err < 0)
		return err;
	data[BMA4XY_ACC_AXIS_X] = raw_data.x;
	data[BMA4XY_ACC_AXIS_Y] = raw_data.y;
	data[BMA4XY_ACC_AXIS_Z] = raw_data.z;
	return err;
}

static int BMA4XY_ReadSensorData(
	struct i2c_client *client, char *buf, int bufsize)
{
	struct bma4xy_client_data *obj =
		(struct bma4xy_client_data *)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[BMA4XY_ACC_AXIS_NUM];
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 10);

	if (NULL == buf)
		return -ERRNUM1;
	if (NULL == client) {
		*buf = 0;
		return -ERRNUM2;
	}

	if (sensor_suspend == 1)
		return 0;

	res = BMA4XY_ReadData(client, obj->data);
	if (res != 0) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -ERRNUM3;
	}

	obj->data[BMA4XY_ACC_AXIS_X] += obj->cali_sw[BMA4XY_ACC_AXIS_X];
	obj->data[BMA4XY_ACC_AXIS_Y] += obj->cali_sw[BMA4XY_ACC_AXIS_Y];
	obj->data[BMA4XY_ACC_AXIS_Z] += obj->cali_sw[BMA4XY_ACC_AXIS_Z];

	acc[obj->cvt.map[BMA4XY_ACC_AXIS_X]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_X] * obj->data[BMA4XY_ACC_AXIS_X];
	acc[obj->cvt.map[BMA4XY_ACC_AXIS_Y]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Y] * obj->data[BMA4XY_ACC_AXIS_Y];
	acc[obj->cvt.map[BMA4XY_ACC_AXIS_Z]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Z] * obj->data[BMA4XY_ACC_AXIS_Z];

	acc[BMA4XY_ACC_AXIS_X] =
	acc[BMA4XY_ACC_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	acc[BMA4XY_ACC_AXIS_Y] =
	acc[BMA4XY_ACC_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	acc[BMA4XY_ACC_AXIS_Z] =
	acc[BMA4XY_ACC_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;

	snprintf(buf, 96, "%04x %04x %04x",
		acc[BMA4XY_ACC_AXIS_X],
		acc[BMA4XY_ACC_AXIS_Y],
		acc[BMA4XY_ACC_AXIS_Z]);
	if (atomic_read(&obj->trace) & ADX_TRC_IOCTL)
		GSE_LOG("gsensor data: %s!\n", buf);
	return 0;
}

static int BMA4XY_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	struct bma4xy_client_data *obj =
		(struct bma4xy_client_data *)i2c_get_clientdata(client);

	if (!buf || !client)
		return -EINVAL;
	res = BMA4XY_ReadData(client, obj->data);
	if (0 != res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}
	snprintf(buf, PAGE_SIZE, "BMA4XY_ReadRawData %04x %04x %04x",
		obj->data[BMA4XY_ACC_AXIS_X],
		obj->data[BMA4XY_ACC_AXIS_Y],
		obj->data[BMA4XY_ACC_AXIS_Z]);
	return 0;
}

static int BMA4XY_ReadOffset(
	struct i2c_client *client, s8 ofs[BMA4XY_ACC_AXIS_NUM])
{
	int err = 0;
#ifdef SW_CALIBRATION
	ofs[0] = ofs[1] = ofs[2] = 0x0;
#else
	err = bma4xy_i2c_read(client,
	BMA4_OFFSET_0_ADDR, ofs, BMA4XY_ACC_AXIS_NUM);
	if (err)
		GSE_ERR("error: %d\n", err);
#endif
	GSE_LOG("offesx=%x, y=%x, z=%x", ofs[0], ofs[1], ofs[2]);

	return err;
}

static int BMA4XY_ResetCalibration(struct i2c_client *client)
{
	struct bma4xy_client_data *priv = i2c_get_clientdata(client);
	int err = 0;

	#ifdef SW_CALIBRATION
	#else
	u8 ofs[3] = {0, 0, 0};

	err = bma4xy_i2c_write(client, BMA4_OFFSET_0_ADDR, ofs, 3);
	if (err)
		GSE_ERR("error: %d\n", err);
	#endif

	memset(priv->cali_sw, 0x00, sizeof(priv->cali_sw));
	memset(priv->offset, 0x00, sizeof(priv->offset));
	return err;
}

static int BMA4XY_ReadCalibration(
	struct i2c_client *client, int dat[BMA4XY_ACC_AXIS_NUM])
{
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int mul;

	GSE_FUN();
	#ifdef SW_CALIBRATION
	mul = 0;/*only SW Calibration, disable HW Calibration*/
	#else
	err = BMA4XY_ReadOffset(client, obj->offset);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = priv->reso->sensitivity/bma4xy_acc_offset_resolution.sensitivity;
	#endif

	dat[obj->cvt.map[BMA4XY_ACC_AXIS_X]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_X]*(obj->offset[BMA4XY_ACC_AXIS_X]*mul +
	obj->cali_sw[BMA4XY_ACC_AXIS_X]);
	dat[obj->cvt.map[BMA4XY_ACC_AXIS_Y]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Y]*(obj->offset[BMA4XY_ACC_AXIS_Y]*mul +
	obj->cali_sw[BMA4XY_ACC_AXIS_Y]);
	dat[obj->cvt.map[BMA4XY_ACC_AXIS_Z]] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Z]*(obj->offset[BMA4XY_ACC_AXIS_Z]*mul +
	obj->cali_sw[BMA4XY_ACC_AXIS_Z]);

	return err;
}

static int BMA4XY_ReadCalibrationEx(
	struct i2c_client *client, int act[BMA4XY_ACC_AXIS_NUM],
	int raw[BMA4XY_ACC_AXIS_NUM])
{
	/*raw: the raw calibration data; act: the actual calibration data */
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int err;
	int mul;

	err = 0;
	#ifdef SW_CALIBRATION
	mul = 0;/* only SW Calibration, disable HW Calibration */
	#else
	err = BMA4XY_ReadOffset(client, obj->offset);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity / bma4xy_acc_offset_resolution.sensitivity;
	#endif

	raw[BMA4XY_ACC_AXIS_X] =
	obj->offset[BMA4XY_ACC_AXIS_X] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_X];
	raw[BMA4XY_ACC_AXIS_Y] =
	obj->offset[BMA4XY_ACC_AXIS_Y] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_Y];
	raw[BMA4XY_ACC_AXIS_Z] =
	obj->offset[BMA4XY_ACC_AXIS_Z] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_Z];
	act[obj->cvt.map[BMA4XY_ACC_AXIS_X]] =
		obj->cvt.sign[BMA4XY_ACC_AXIS_X] * raw[BMA4XY_ACC_AXIS_X];
	act[obj->cvt.map[BMA4XY_ACC_AXIS_Y]] =
		obj->cvt.sign[BMA4XY_ACC_AXIS_Y] * raw[BMA4XY_ACC_AXIS_Y];
	act[obj->cvt.map[BMA4XY_ACC_AXIS_Z]] =
		obj->cvt.sign[BMA4XY_ACC_AXIS_Z] * raw[BMA4XY_ACC_AXIS_Z];
	return err;
}

static int BMA4XY_WriteCalibration(
	struct i2c_client *client, int dat[BMA4XY_ACC_AXIS_NUM])
{
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[BMA4XY_ACC_AXIS_NUM], raw[BMA4XY_ACC_AXIS_NUM];

	err = BMA4XY_ReadCalibrationEx(client, cali, raw);
	if (0 != err) {/*offset will be updated in obj->offset */
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	raw[BMA4XY_ACC_AXIS_X], raw[BMA4XY_ACC_AXIS_Y], raw[BMA4XY_ACC_AXIS_Z],
	obj->offset[BMA4XY_ACC_AXIS_X],
	obj->offset[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z],
	obj->cali_sw[BMA4XY_ACC_AXIS_X],
	obj->cali_sw[BMA4XY_ACC_AXIS_Y],
	obj->cali_sw[BMA4XY_ACC_AXIS_Z]);

	/*calculate the real offset expected by caller */
	cali[BMA4XY_ACC_AXIS_X] += dat[BMA4XY_ACC_AXIS_X];
	cali[BMA4XY_ACC_AXIS_Y] += dat[BMA4XY_ACC_AXIS_Y];
	cali[BMA4XY_ACC_AXIS_Z] += dat[BMA4XY_ACC_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
		dat[BMA4XY_ACC_AXIS_X],
		dat[BMA4XY_ACC_AXIS_Y],
		dat[BMA4XY_ACC_AXIS_Z]);

	#ifdef SW_CALIBRATION
	obj->cali_sw[BMA4XY_ACC_AXIS_X] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_X] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_X]]);
	obj->cali_sw[BMA4XY_ACC_AXIS_Y] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Y] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Y]]);
	obj->cali_sw[BMA4XY_ACC_AXIS_Z] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Z] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Z]]);
	#else
	int divisor = obj->reso->sensitivity / lsb;/* modified */

	obj->offset[BMA4XY_ACC_AXIS_X] =
	(s8) (obj->cvt.sign[BMA4XY_ACC_AXIS_X] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_X]]) / (divisor));
	obj->offset[BMA4XY_ACC_AXIS_Y] =
	(s8) (obj->cvt.sign[BMA4XY_ACC_AXIS_Y] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Y]]) / (divisor));
	obj->offset[BMA4XY_ACC_AXIS_Z] =
	(s8) (obj->cvt.sign[BMA4XY_ACC_AXIS_Z] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Z]]) / (divisor));

	/*convert software calibration using standard calibration */
	obj->cali_sw[BMA4XY_ACC_AXIS_X] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_X] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_X]]) % (divisor);
	obj->cali_sw[BMA4XY_ACC_AXIS_Y] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Y] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Y]]) % (divisor);
	obj->cali_sw[BMA4XY_ACC_AXIS_Z] =
	obj->cvt.sign[BMA4XY_ACC_AXIS_Z] *
	(cali[obj->cvt.map[BMA4XY_ACC_AXIS_Z]]) % (divisor);

GSE_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	obj->offset[BMA4XY_ACC_AXIS_X] * divisor +
	obj->cali_sw[BMA4XY_ACC_AXIS_X],
	obj->offset[BMA4XY_ACC_AXIS_Y] * divisor +
	obj->cali_sw[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z] * divisor +
	obj->cali_sw[BMA4XY_ACC_AXIS_Z],
	obj->offset[BMA4XY_ACC_AXIS_X], obj->offset[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z],
	obj->cali_sw[BMA4XY_ACC_AXIS_X], obj->cali_sw[BMA4XY_ACC_AXIS_Y],
	obj->cali_sw[BMA4XY_ACC_AXIS_Z]);

	err = hwmsen_write_block(obj->client,
		BMA4_OFFSET_0_ADDR, obj->offset, BMA4XY_ACC_AXIS_NUM);
	if (err) {
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif
	bma4xy_i2c_delay(1);
	return err;
}

static int BMA4XY_CheckDeviceID(struct i2c_client *client)
{
	u8 databuf[2] = { 0 };
	int ret = 0;
	ret = bma4xy_i2c_read(client, BMA4XY_CHIP_ID, databuf, 1);
	if (ret < 0)
		goto exit_BMA4XY_CheckDeviceID;

	GSE_LOG("BMA4XY_CheckDeviceID %d done!\n ", databuf[0]);
	bma4xy_i2c_delay(1);
	return 0;
exit_BMA4XY_CheckDeviceID:
	if (ret < 0) {
		GSE_ERR("BMA4XY_CheckDeviceID %d failt!\n ", databuf[0]);
		return ret;
	}
	return ret;
}

static int BMA4XY_SetPowerMode(struct i2c_client *client, bool enable)
{
	int err = 0;
	struct bma4xy_client_data *client_data =
		(struct bma4xy_client_data *)i2c_get_clientdata(client);

	if (enable == 0 &&
		(client_data->sigmotion_enable == 0) &&
		(client_data->stepdet_enable == 0) &&
		(client_data->stepcounter_enable == 0) &&
		(client_data->tilt_enable == 0) &&
		(client_data->pickup_enable == 0) &&
		(client_data->glance_enable == 0) &&
		(client_data->wakeup_enable == 0)) {
			err = bma455_set_accel_enable(
				BMA4_DISABLE, &client_data->device);
			GSE_LOG("acc_op_mode %d", enable);
		}
	else if (enable == 1) {
		err = bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
		GSE_LOG("acc_op_mode %d", enable);
	}
	if (err) {
		GSE_ERR("failed");
		return err;
	}
	sensor_power = enable;
	client_data->acc_pm = enable;
	bma4xy_i2c_delay(5);
	GSE_LOG("leave Sensor power status is sensor_power = %d\n",
	sensor_power);
	return err;
}

static int BMA4XY_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	int err = 0;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	struct bma4_accel_config acc_config;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_accel_config(&acc_config, &client_data->device);
	acc_config.range = (uint8_t)(BMA4_ACCEL_RANGE_4G);
	err += bma455_set_accel_config(&acc_config, &client_data->device);
	if (err) {
		GSE_ERR("faliled");
		return -EIO;
	}
	bma4xy_i2c_delay(5);
	return BMA4XY_SetDataResolution(obj);
}

static int BMA4XY_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	uint8_t data = 0;
	int ret = 0;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);

	data = (uint8_t)bwrate;
	if (bwrate == 4)
		data = 0x74;
	else
		data |= 0xA0;
	ret = bma4xy_i2c_write_wrapper(obj->device.dev_addr, 0x40, &data, 1);
	if (ret) {
		GSE_ERR("faliled");
		return ret;
	}
	bma4xy_i2c_delay(5);
	GSE_LOG("acc_odr =%d", data);
	return ret;
}

static int bma4xy_check_chip_id(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t chip_id = 0;
	uint8_t check_chip_id = 0;
	err = client_data->device.bus_read(client_data->device.dev_addr,
			BMA4_CHIP_ID_ADDR, &chip_id, 1);
	if (err) {
		GSE_ERR("error");
		return err;
	}
	#if defined(BMA421)
	check_chip_id = BMA421_CHIP_ID;
	#elif defined(BMA421L)
	check_chip_id = BMA421L_CHIP_ID;
	#elif defined(BMA422)
	check_chip_id = BMA422_CHIP_ID;
	#elif defined(BMA424)
	check_chip_id = BMA424_CHIP_ID;
	#elif defined(BMA424SC)
	check_chip_id = BMA424SC_CHIP_ID;
	#elif defined(BMA455)
	check_chip_id = BMA455_CHIP_ID;
	#elif defined(BMA455N)
	check_chip_id = BMA455N_CHIP_ID;
	#endif
	if (chip_id == check_chip_id)
		return err;
	else {
		err = CHIP_ID_ERR;
		return err;
	}
}

static ssize_t bma4xy_show_chip_id(struct device_driver *ddri, char *buf)
{
	uint8_t chip_id = 0;
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma4xy_i2c_read_wrapper(client_data->device.dev_addr,
			BMA4_CHIP_ID_ADDR, &chip_id, 1);
	if (err) {
		GSE_ERR("falied");
		return err;
	}
	return snprintf(buf, 48, "chip_id=%x\n", chip_id);
}

static ssize_t bma4xy_show_acc_op_mode(struct device_driver *ddri, char *buf)
{
	int err;
	unsigned char acc_op_mode;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_accel_enable(&acc_op_mode, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 96, "1 mean enable now is %d\n", acc_op_mode);
}
static ssize_t bma4xy_store_acc_op_mode(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long op_mode;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &op_mode);
	if (err)
		return err;
	if (op_mode == 2 &&
		(client_data->sigmotion_enable == 0) &&
		(client_data->stepdet_enable == 0) &&
		(client_data->stepcounter_enable == 0) &&
		(client_data->tilt_enable == 0) &&
		(client_data->pickup_enable == 0) &&
		(client_data->glance_enable == 0) &&
		(client_data->wakeup_enable == 0) &&
		(client_data->anymotion_enable == 0) &&
		(client_data->nomotion_enable == 0) &&
		(client_data->orientation_enable== 0)) {
			err = bma455_set_accel_enable(
				BMA4_DISABLE, &client_data->device);
			GSE_LOG("acc_op_mode %ld", op_mode);
		}
	else if (op_mode == 0) {
		err = bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
		GSE_LOG("acc_op_mode %ld", op_mode);
	}
	if (err) {
		GSE_ERR("failed");
		return err;
	}
	client_data->acc_pm = op_mode;
	bma4xy_i2c_delay(5);
	return count;
}

static ssize_t bma4xy_show_acc_value(struct device_driver *ddri, char *buf)
{
	struct bma4_accel data;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	int err;

	err = bma455_read_accel_xyz(&data, &client_data->device);
	if (err < 0)
		return err;
	return snprintf(buf, 48, "%hd %hd %hd\n",
			data.x, data.y, data.z);
}

static ssize_t bma4xy_show_acc_range(struct device_driver *ddri, char *buf)
{
	int err;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma4_accel_config acc_config;

	err = bma455_get_accel_config(&acc_config, &client_data->device);
	if (err) {
		GSE_ERR("failed");
		return err;
	}
	return snprintf(buf, 16, "%d\n", acc_config.range);
}

static ssize_t bma4xy_store_acc_range(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long acc_range;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma4_accel_config acc_config;

	err = kstrtoul(buf, 10, &acc_range);
	if (err)
		return err;
	err = bma455_get_accel_config(&acc_config, &client_data->device);
	acc_config.range = (uint8_t)(acc_range);
	err += bma455_set_accel_config(&acc_config, &client_data->device);
	bma4xy_i2c_delay(5);
	if (err) {
		GSE_ERR("faliled");
		return -EIO;
	}
	return count;
}

static ssize_t bma4xy_show_acc_odr(struct device_driver *ddri, char *buf)
{
	int err;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma4_accel_config acc_config;

	err = bma455_get_accel_config(&acc_config, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	client_data->acc_odr = acc_config.odr;
	return snprintf(buf, 16, "%d\n", client_data->acc_odr);

}

static ssize_t bma4xy_store_acc_odr(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long acc_odr;
	uint8_t data = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &acc_odr);
	if (err)
		return err;
	data = (uint8_t)acc_odr;
	if (acc_odr == 4)
		data = 0x74;
	else
		data |= 0xA0;
	err = client_data->device.bus_write(client_data->device.dev_addr,
		0x40, &data, 1);
	if (err) {
		GSE_ERR("faliled");
		return -EIO;
	}
	GSE_ERR("acc_odr =%ld", acc_odr);
	bma4xy_i2c_delay(5);
	client_data->acc_odr = acc_odr;
	return count;
}

static ssize_t bma4xy_show_selftest(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	return snprintf(buf, 16, "%d\n", client_data->selftest);
}
static ssize_t bma4xy_store_selftest(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	uint8_t result = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_perform_accel_selftest(
		&result, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	if (result == 0) {
		GSE_LOG("Selftest successsful");
		client_data->selftest = 1;
	} else
		client_data->selftest = 0;
	return count;
}

static ssize_t bma4xy_show_foc(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	return snprintf(buf, 64,
		"Use echo g_sign aixs > foc to begin foc\n");
}
static ssize_t bma4xy_store_foc(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int g_value[3] = {0};
	int32_t data[3] = {0};
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	int err = 0;

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	err = sscanf(buf, "%11d %11d %11d",
		&g_value[0], &g_value[1], &g_value[2]);
	GSE_LOG("g_value0=%d, g_value1=%d, g_value2=%d",
		g_value[0], g_value[1], g_value[2]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	data[0] = g_value[0] * BMA4XY_MULTIPLIER;
	data[1] = g_value[1] * BMA4XY_MULTIPLIER;
	data[2] = g_value[2] * BMA4XY_MULTIPLIER;
	err = bma455_perform_accel_foc(data, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	GSE_FUN("FOC successsfully");
	return count;
}
static ssize_t bma4xy_show_config_function(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	return snprintf(buf, PAGE_SIZE,
		"sig_motion0=%d step_detector1=%d step_counter2=%d\n"
		"tilt3=%d pickup4=%d glance_detector5=%d wakeup6=%d\n"
		"any_motion7=%d nomotion8=%d\n"
		"orientation9=%d flat10=%d\n"
		"high_g11=%d low_g12=%d activity13=%d nomotion14=%d\n",
		client_data->sigmotion_enable, client_data->stepdet_enable,
		client_data->stepcounter_enable, client_data->tilt_enable,
		client_data->pickup_enable, client_data->glance_enable,
		client_data->wakeup_enable, client_data->anymotion_enable,
		client_data->nomotion_enable, client_data->orientation_enable,
		client_data->flat_enable, client_data->highg_enable,
		client_data->lowg_enable, client_data->activity_enable,
		client_data->nomotion_enable);
}

static ssize_t bma4xy_store_config_function(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;
	int config_func = 0;
	int enable = 0;
	uint8_t feature = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	ret = sscanf(buf, "%11d %11d", &config_func, &enable);
	GSE_LOG("config_func = %d, enable=%d", config_func, enable);
	if (ret != 2) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	if (config_func < 0 || config_func > 14)
		return -EINVAL;
	switch (config_func) {
	case BMA4XY_SIG_MOTION_SENSOR:
		#if defined(BMA422)
		feature = BMA422_SIG_MOTION;
		#endif
		#if defined(BMA422N)
		feature = BMA422N_SIG_MOTION;
		#endif
		#if defined(BMA455N)
		feature = BMA455N_SIG_MOTION;
		#endif
		#if defined(BMA424)
		feature = BMA424_SIG_MOTION;
		#endif
		#if defined(BMA455)
		feature = BMA455_SIG_MOTION;
		#endif
		client_data->sigmotion_enable = enable;
		break;
	case BMA4XY_STEP_DETECTOR_SENSOR:
		#if defined(BMA421)
		if (bma421_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA424SC)
		if (bma424sc_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA421L)
		if (bma421l_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA422)
		if (bma422_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA424)
		if (bma424_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA455)
		if (bma455_step_detector_enable(
			enable, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
		}
		#endif
		#if defined(BMA422N)
		feature = BMA422N_STEP_DETR;
		#endif
		#if defined(BMA455N)
		feature = BMA455N_STEP_DETR;
		#endif
		client_data->stepdet_enable = enable;
		break;
	case BMA4XY_STEP_COUNTER_SENSOR:
		#if defined(BMA421)
		feature = BMA421_STEP_CNTR;
		#endif
		#if defined(BMA424SC)
		feature = BMA424SC_STEP_CNTR;
		#endif
		#if defined(BMA421L)
		feature = BMA421L_STEP_CNTR;
		#endif
		#if defined(BMA422)
		feature = BMA422_STEP_CNTR;
		#endif
		#if defined(BMA422N)
		feature = BMA422N_STEP_CNTR;
		#endif
		#if defined(BMA455N)
		feature = BMA455N_STEP_CNTR;
		#endif
		#if defined(BMA424)
		feature = BMA424_STEP_CNTR;
		#endif
		#if defined(BMA455)
		feature = BMA455_STEP_CNTR;
		#endif
		client_data->stepcounter_enable = enable;
		break;
	case BMA4XY_TILT_SENSOR:
		#if defined(BMA422)
		feature = BMA422_TILT;
		#endif
		#if defined(BMA424)
		feature = BMA424_TILT;
		#endif
		#if defined(BMA455)
		feature = BMA455_TILT;
		#endif
		client_data->tilt_enable = enable;
		break;
	case BMA4XY_PICKUP_SENSOR:
		#if defined(BMA422)
		feature = BMA422_PICKUP;
		#endif
		#if defined(BMA424)
		feature = BMA424_PICKUP;
		#endif
		#if defined(BMA455)
		feature = BMA455_PICKUP;
		#endif
		client_data->pickup_enable = enable;
		break;
	case BMA4XY_GLANCE_DETECTOR_SENSOR:
		#if defined(BMA422)
		feature = BMA422_GLANCE;
		#endif
		#if defined(BMA424)
		feature = BMA424_GLANCE;
		#endif
		#if defined(BMA455)
		feature = BMA455_GLANCE;
		#endif
		client_data->glance_enable = enable;
		break;
	case BMA4XY_WAKEUP_SENSOR:
		#if defined(BMA422)
		feature = BMA422_WAKEUP;
		#endif
		#if defined(BMA424)
		feature = BMA424_WAKEUP;
		#endif
		#if defined(BMA455)
		feature = BMA455_WAKEUP;
		#endif
		client_data->wakeup_enable = enable;
		break;
	case BMA4XY_ANY_MOTION_SENSOR:
		#if defined(BMA420)
		feature = BMA420_ANY_MOTION;
		#endif
		#if defined(BMA421)
		feature = BMA421_ANY_MOTION;
		#endif
		#if defined(BMA424SC)
		feature = BMA424SC_ANY_MOTION;
		#endif
		#if defined(BMA421L)
		feature = BMA421L_ANY_MOTION;
		#endif
		#if defined(BMA422)
		feature = BMA422_ANY_MOTION;
		#endif
		#if defined(BMA422N)
		if (enable ==1) {
			if (bma422n_anymotion_enable_axis(
				client_data->any_motion_axis, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		else if (enable == 0) {
			if (bma422n_anymotion_enable_axis(
				0, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		#endif
		#if defined(BMA455N)
		if (enable ==1) {
			if (bma455n_anymotion_enable_axis(
				client_data->any_motion_axis, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		else if (enable == 0) {
			if (bma455n_anymotion_enable_axis(
				0, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		#endif
		#if defined(BMA424)
		feature = BMA424_ANY_MOTION;
		#endif
		#if defined(BMA455)
		feature = BMA455_ANY_MOTION;
		#endif
		client_data->anymotion_enable = enable;
		break;
	case BMA4XY_ORIENTATION_SENSOR:
		#if defined(BMA420)
		feature = BMA420_ORIENTATION;
		#endif
		#if defined(BMA422N)
		feature = BMA422N_ORIENTATION;
		#endif
		#if defined(BMA455N)
		feature = BMA455N_ORIENTATION;
		#endif
		#if defined(BMA456)
		feature = BMA420_ORIENTATION;
		#endif
		client_data->orientation_enable = enable;
		break;
	case BMA4XY_FLAT_SENSOR:
		#if defined(BMA420)
		feature = BMA420_FLAT;
		#endif
		#if defined(BMA421L)
		feature = BMA421L_FLAT;
		#endif
		#if defined(BMA456)
		feature = BMA456_FLAT;
		#endif
		client_data->flat_enable = enable;
		break;
	case BMA4XY_TAP_SENSOR:
		#if defined(BMA420)
		feature = BMA420_TAP;
		#endif
		client_data->tap_enable = enable;
		break;
	case BMA4XY_HIGH_G_SENSOR:
		#if defined(BMA420)
		feature = BMA420_HIGH_G;
		#endif
		client_data->highg_enable = enable;
		break;
	case BMA4XY_LOW_G_SENSOR:
		#if defined(BMA420)
		feature = BMA420_LOW_G;
		#endif
		client_data->lowg_enable = enable;
		break;
	case BMA4XY_ACTIVITY_SENSOR:
		#if defined(BMA421)
		feature = BMA421_ACTIVITY;
		#endif
		#if defined(BMA422N)
		feature = BMA422N_ACTIVITY;
		#endif
		#if defined(BMA455N)
		feature = BMA455N_ACTIVITY;
		#endif
		#if defined(BMA424SC)
		feature = BMA424SC_ACTIVITY;
		#endif
		client_data->activity_enable = enable;
		break;
	case BMA4XY_NO_MOTION_SENSOR:
		#if defined(BMA422N)
		if (enable ==1) {
			if (bma422n_no_motion_enable_axis(
				client_data->no_motion_axis, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		else if (enable == 0) {
			if (bma422n_no_motion_enable_axis(
				0, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		#endif
		#if defined(BMA455N)
		if (enable ==1) {
			if (bma455n_no_motion_enable_axis(
				client_data->no_motion_axis, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		else if (enable == 0) {
			if (bma455n_no_motion_enable_axis(
				0, &client_data->device) < 0) {
			GSE_ERR("set BMA4XY_STEP_DETECTOR_SENSOR error");
			return -EINVAL;
			}
		}
		#endif
		client_data->nomotion_enable = enable;
		break;
	default:
		GSE_ERR("Invalid sensor handle: %d", config_func);
		return -EINVAL;
	}
	#if defined(BMA420)
	if (bma420_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma420 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA421)
	if (bma421_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424SC)
	if (bma424sc_feature_enable(
		feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA421L)
	if (bma421l_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422)
	if (bma422_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422N)
	if (bma422n_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455N)
	if (bma455n_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424)
	if (bma424_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455)
	if (bma455_feature_enable(feature, enable, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	return count;
}

static ssize_t bma4xy_show_fifo_length(
	struct device_driver *ddri, char *buf)
{
	int err;
	uint16_t fifo_bytecount = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_fifo_length(&fifo_bytecount, &client_data->device);
	if (err) {
		GSE_ERR("read falied");
		return err;
	}
	return snprintf(buf, 96, "%d\n", fifo_bytecount);
}
static ssize_t bma4xy_store_fifo_flush(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long enable;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &enable);
	if (err)
		return err;
	if (enable)
		err = bma455_set_command_register(0xb0, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return -EIO;
	}
	bma4xy_i2c_delay(1);
	return count;
}
static ssize_t bma4xy_show_fifo_acc_enable(
	struct device_driver *ddri, char *buf)
{
	int err;
	uint8_t fifo_acc_enable;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_fifo_config(&fifo_acc_enable, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 16, "%x\n", fifo_acc_enable);
}
static ssize_t bma4xy_store_fifo_acc_enable(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long data;
	unsigned char fifo_acc_enable;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	fifo_acc_enable = (unsigned char)data;
	err = bma455_set_fifo_config(
		BMA4_FIFO_ACCEL, fifo_acc_enable, &client_data->device);
	if (err) {
		GSE_ERR("faliled");
		return -EIO;
	}
	client_data->fifo_acc_enable = fifo_acc_enable;
	bma4xy_i2c_delay(1);
	return count;
}


static ssize_t bma4xy_show_load_config_stream(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	return snprintf(buf, 48, "config stream %s\n",
		client_data->config_stream_name);
}

static int bma4xy_init_after_config_stream_load(
	struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t int_enable = 0x0a;
	uint8_t latch_enable = 0x01;
	uint8_t int1_map = 0xff;
	#if defined(BMA420)
	struct bma420_axes_remap axis_remap_data;
	#elif defined(BMA421)
	struct bma421_axes_remap axis_remap_data;
	#elif defined(BMA424SC)
	struct bma424sc_axes_remap axis_remap_data;
	#elif defined(BMA421L)
	struct bma421l_axes_remap axis_remap_data;
	#elif defined(BMA422)
	struct bma422_axes_remap axis_remap_data;
	#elif defined(BMA422N)
	struct bma422n_axes_remap axis_remap_data;
	#elif defined(BMA455N)
	struct bma455n_axes_remap axis_remap_data;
	#elif defined(BMA424)
	struct bma424_axes_remap axis_remap_data;
	#elif defined(BMA455)
	struct bma455_axes_remap axis_remap_data;
	#endif
	err = bma455_write_regs(
	BMA4_INT_MAP_1_ADDR, &int1_map, 1, &client_data->device);
	bma4xy_i2c_delay(10);
	err += bma455_write_regs(
	BMA4_INT1_IO_CTRL_ADDR, &int_enable, 1, &client_data->device);
	bma4xy_i2c_delay(1);
	err += bma455_write_regs(
	BMA4_INTR_LATCH_ADDR, &latch_enable, 1, &client_data->device);
	bma4xy_i2c_delay(1);
	if (err)
		GSE_ERR("map and enable interrupr1 failed err=%d", err);
	memset(&axis_remap_data, 0, sizeof(axis_remap_data));
	axis_remap_data.x_axis = 0;
	axis_remap_data.x_axis_sign = 0;
	axis_remap_data.y_axis = 1;
	axis_remap_data.y_axis_sign = 1;
	axis_remap_data.z_axis = 2;
	axis_remap_data.z_axis_sign = 1;
	#if defined(BMA420)
	err = bma420_set_remap_axes(&axis_remap_data, &client_data->device);
	#endif
	#if defined(BMA421)
	err = bma421_set_remap_axes(&axis_remap_data, &client_data->device);
	err = bma421_select_platform(BMA421_PHONE_CONFIG, &client_data->device);
	if (err)
		GSE_ERR("set bma421 step_count select platform error");
	#endif
	#if defined(BMA424SC)
	err = bma424sc_set_remap_axes(&axis_remap_data, &client_data->device);
	err = bma424sc_select_platform(BMA424SC_PHONE_CONFIG, &client_data->device);
	if (err)
		GSE_ERR("set bma421 step_count select platform error");
	#endif
	#if defined(BMA421L)
	err = bma421l_set_remap_axes(&axis_remap_data, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_set_remap_axes(&axis_remap_data, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_set_remap_axes(&axis_remap_data, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_set_remap_axes(&axis_remap_data, &client_data->device);
	bma4xy_i2c_delay(5);
	err += bma422n_select_platform(
		BMA422N_PHONE_CONFIG, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_set_remap_axes(&axis_remap_data, &client_data->device);
	bma4xy_i2c_delay(5);
	err += bma455n_select_platform(
		BMA455N_PHONE_CONFIG, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_set_remap_axes(&axis_remap_data, &client_data->device);
	#endif
	bma4xy_i2c_delay(1);
	if (err) {
		GSE_ERR("write axis_remap failed");
		return err;
	}
	return err;
}
#if defined(BMA4XY_ENABLE_INT1) || defined(BMA4XY_ENABLE_INT2)
static int bma4xy_init_fifo_config(
	struct bma4xy_client_data *client_data)
{
	int err = 0;
	err = bma455_set_fifo_config(
		BMA4_FIFO_HEADER, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("enable fifo header failed err=%d", err);
	bma4xy_i2c_delay(1);
	err = bma455_set_fifo_config(
		BMA4_FIFO_TIME, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("enable fifo timer failed err=%d", err);
	bma4xy_i2c_delay(1);
	return err;
}
#endif
static int bma4xy_update_config_stream(
	struct bma4xy_client_data *client_data, int choose)
{
	char *name;
	int err = 0;
	uint8_t crc_check = 0;

	switch (choose) {
	case 1:
	name = "android.tbin";
	break;
	case 2:
	name = "legacy.tbin";
	break;
	default:
	GSE_ERR("no choose fw = %d,use dafault ", choose);
	name = "bma4xy_config_stream";
	break;
	}
	GSE_LOG("choose the config_stream %s", name);
	client_data->config_stream_name = name;
	if ((choose == 1) || (choose == 2)) {
		GSE_ERR("no choose fw = %d,use dafault ", choose);
		GSE_LOG("choose the config_stream %s", name);
	} else if (choose == 3) {
		#if defined(BMA420)
		err = bma420_write_config_file(&client_data->device);
		#endif
		#if defined(BMA421)
		err = bma421_write_config_file(&client_data->device);
		#endif
		#if defined(BMA424SC)
		err = bma424sc_write_config_file(&client_data->device);
		#endif
		#if defined(BMA421L)
		err = bma421l_write_config_file(&client_data->device);
		#endif
		#if defined(BMA422)
		err = bma422_write_config_file(&client_data->device);
		#endif
		#if defined(BMA422N)
		err = bma422n_write_config_file(&client_data->device);
		#endif
		#if defined(BMA455N)
		err = bma455n_write_config_file(&client_data->device);
		#endif
		#if defined(BMA424)
		err = bma424_write_config_file(&client_data->device);
		#endif
		#if defined(BMA455)
		err = bma455_write_config_file(&client_data->device);
		#endif
		if (err)
			GSE_ERR("download config stream failer");
		bma4xy_i2c_delay(200);
		err = bma455_read_regs(BMA4_INTERNAL_STAT,
		&crc_check, 1, &client_data->device);
		if (err)
			GSE_ERR("reading CRC failer");
		if (crc_check != BMA4_ASIC_INITIALIZED)
			GSE_ERR("crc check error %x", crc_check);
	}
	return err;
}

static ssize_t bma4xy_store_load_config_stream(
	struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long choose = 0;
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &choose);
	if (err)
		return err;
	GSE_LOG("config_stream_choose %ld", choose);
	err = bma4xy_update_config_stream(client_data, choose);
	if (err) {
		GSE_ERR("config_stream load error");
		return count;
	}
	err = bma4xy_init_after_config_stream_load(client_data);
	if (err) {
		GSE_ERR("bma4xy_init_after_config_stream_load error");
		return count;
	}
	return count;
}

static int bma4xy_load_config_stream(
	struct bma4xy_client_data *client_data)
{
	int err = 0;

	err = bma4xy_update_config_stream(client_data, 3);
	if (err) {
		GSE_ERR("config_stream load error");
		return err;
	}
	err = bma4xy_init_after_config_stream_load(client_data);
	if (err) {
		GSE_ERR("bma4xy_init_after_config_stream_load error");
		return err;
	}
	return err;
}

static ssize_t bma4xy_show_fifo_watermark(
	struct device_driver *ddri, char *buf)
{
	int err;
	uint16_t data;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_fifo_wm(&data, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 48, "%d\n", data);
}

static ssize_t bma4xy_store_fifo_watermark(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long data;
	unsigned char fifo_watermark;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	fifo_watermark = (unsigned char)data;
	err = bma455_set_fifo_wm(fifo_watermark, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
static ssize_t bma4xy_show_fifo_data_out_frame(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	int err = 0;
	uint16_t fifo_bytecount = 0;

	if (!client_data->fifo_mag_enable && !client_data->fifo_acc_enable) {
		GSE_ERR("no selsect sensor fifo\n");
		return -EINVAL;
	}
	err = bma455_get_fifo_length(&fifo_bytecount, &client_data->device);
	if (err < 0) {
		GSE_ERR("read fifo_len err=%d", err);
		return -EINVAL;
	}
	if (fifo_bytecount == 0)
		return 0;
	err =  bma455_read_regs(BMA4_FIFO_DATA_ADDR,
		buf, fifo_bytecount, &client_data->device);
	if (err) {
		GSE_ERR("read fifo leght err");
		return -EINVAL;
	}
	return fifo_bytecount;
}

static ssize_t bma4xy_show_reg_sel(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	return snprintf(buf, 64, "reg=0X%02X, len=%d\n",
		client_data->reg_sel, client_data->reg_len);
}

static ssize_t bma4xy_store_reg_sel(
	struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	ssize_t ret;

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	ret = sscanf(buf, "%11X %11d",
		&client_data->reg_sel, &client_data->reg_len);
	if (ret != 2) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	return count;
}

static ssize_t bma4xy_show_reg_val(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	ssize_t ret;
	u8 reg_data[128], i;
	int pos;

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	ret = client_data->device.bus_read(client_data->device.dev_addr,
		client_data->reg_sel,
		reg_data, client_data->reg_len);
	if (ret < 0) {
		GSE_ERR("Reg op failed");
		return ret;
	}
	pos = 0;
	for (i = 0; i < client_data->reg_len; ++i) {
		pos += snprintf(buf + pos, 16, "%02X", reg_data[i]);
		buf[pos++] = (i + 1) % 16 == 0 ? '\n' : ' ';
	}
	if (buf[pos - 1] == ' ')
		buf[pos - 1] = '\n';
	return pos;
}

static ssize_t bma4xy_store_reg_val(
	struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	ssize_t ret;
	u8 reg_data[128];
	int i, j, status, digit;

	if (client_data == NULL) {
		GSE_ERR("Invalid client_data pointer");
		return -ENODEV;
	}
	status = 0;
	for (i = j = 0; i < count && j < client_data->reg_len; ++i) {
		if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t' ||
			buf[i] == '\r') {
			status = 0;
			++j;
			continue;
		}
		digit = buf[i] & 0x10 ? (buf[i] & 0xF) : ((buf[i] & 0xF) + 9);
		GSE_LOG("digit is %d", digit);
		switch (status) {
		case 2:
			++j; /* Fall thru */
		case 0:
			reg_data[j] = digit;
			status = 1;
			break;
		case 1:
			reg_data[j] = reg_data[j] * 16 + digit;
			status = 2;
			break;
		}
	}
	if (status > 0)
		++j;
	if (j > client_data->reg_len)
		j = client_data->reg_len;
	else if (j < client_data->reg_len) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	GSE_LOG("Reg data read as");
	for (i = 0; i < j; ++i)
		GSE_LOG("%d", reg_data[i]);
	ret = client_data->device.bus_write(client_data->device.dev_addr,
		client_data->reg_sel,
		reg_data, client_data->reg_len);
	if (ret < 0) {
		GSE_ERR("Reg op failed");
		return ret;
	}
	return count;
}

static ssize_t bma4xy_show_driver_version(
	struct device_driver *ddri, char *buf)
{
	return snprintf(buf, 128,
		"Driver version: %s\n", DRIVER_VERSION);
}

static ssize_t bma4xy_show_config_file_version(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	uint16_t version = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA420)
	err = bma420_get_config_id(&version, &client_data->device);
	#elif defined(BMA421)
	err = bma421_get_config_id(&version, &client_data->device);
	#elif defined(BMA424SC)
	err = bma424sc_get_config_id(&version, &client_data->device);
	#elif defined(BMA421L)
	err = bma421l_get_config_id(&version, &client_data->device);
	#elif defined(BMA422)
	err = bma422_get_config_id(&version, &client_data->device);
	#elif defined(BMA422N)
	err = bma422n_get_config_id(&version, &client_data->device);
	#elif defined(BMA455N)
	err = bma455n_get_config_id(&version, &client_data->device);
	#elif defined(BMA424)
	err = bma424_get_config_id(&version, &client_data->device);
	#elif defined(BMA455)
	err = bma455_get_config_id(&version, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 128,
		"Driver version: %s Config_stream version :0x%x\n",
		DRIVER_VERSION, version);
}

static ssize_t bma4xy_show_avail_sensor(
	struct device_driver *ddri, char *buf)
{
	uint16_t avail_sensor = 0;
	#if defined(BMA420)
		avail_sensor = 420;
	#elif defined(BMA421)
		avail_sensor = 421;
	#elif defined(BMA424SC)
		avail_sensor = 421;
	#elif defined(BMA421L)
		avail_sensor = 421;
	#elif defined(BMA422)
		avail_sensor = 422;
	#elif defined(BMA422N)
		avail_sensor = 4227;
	#elif defined(BMA455N)
		avail_sensor = 4557;
	#elif defined(BMA424)
		avail_sensor = 422;
	#elif defined(BMA455)
		avail_sensor = 455;
	#elif defined(BMA456)
		avail_sensor = 456;
	#endif
	return snprintf(buf, 32, "%d\n", avail_sensor);
}
#if defined(BMA422) || defined(BMA455) || defined(BMA424) || defined(BMA422N) || defined(BMA455N)
static ssize_t bma4xy_show_sig_motion_config(
	struct device_driver *ddri, char *buf)
{
	int err;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA422)
	struct bma422_sig_motion_config sig_m_config;
	#elif defined(BMA455)
	struct bma455_sig_motion_config sig_m_config;
	#elif defined(BMA424)
	struct bma424_sig_motion_config sig_m_config;
	#elif defined(BMA422N)
	struct bma422n_sig_motion_config sig_m_config;
	#elif defined(BMA455N)
	struct bma455n_sig_motion_config sig_m_config;
	#endif

	#if defined(BMA422)
	err = bma422_get_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_get_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_get_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_get_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_get_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	#if !defined(BMA422N) && !defined(BMA455N)
	return snprintf(buf, PAGE_SIZE,
	"threshold =0x%x skiptime= 0x%x prooftime = 0x%x\n",
	sig_m_config.threshold, sig_m_config.skiptime, sig_m_config.prooftime);
	#else
	return snprintf(buf, PAGE_SIZE,
	"block_size =0x%x p2p_min_intvl= 0x%x p2p_max_intvl = 0x%x mcr_min= 0x%x mcr_max = 0x%x\n",
	sig_m_config.block_size, sig_m_config.p2p_min_intvl, sig_m_config.p2p_max_intvl,
	sig_m_config.mcr_min,sig_m_config.mcr_max);
	#endif
}

static ssize_t bma4xy_store_sig_motion_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	#if !defined(BMA422N) && !defined(BMA455N)
	unsigned int data[3] = {0};
	#else
	unsigned int data[5] = {0};
	#endif
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA422)
	struct bma422_sig_motion_config sig_m_config;
	#elif defined(BMA455)
	struct bma455_sig_motion_config sig_m_config;
	#elif defined(BMA424)
	struct bma424_sig_motion_config sig_m_config;
	#elif defined(BMA422N)
	struct bma422n_sig_motion_config sig_m_config;
	#elif defined(BMA455N)
	struct bma455n_sig_motion_config sig_m_config;
	#endif

	#if !defined(BMA422N) && !defined(BMA455N)
	err = sscanf(buf, "%11x %11x %11x", &data[0], &data[1], &data[2]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&sig_m_config, 0, sizeof(sig_m_config));
	sig_m_config.threshold = (uint16_t)data[0];
	sig_m_config.skiptime = (uint16_t)data[1];
	sig_m_config.prooftime = (uint8_t)data[2];
	#else
	err = sscanf(buf, "%11x %11x %11x %11x %11x", &data[0], &data[1], &data[2], &data[3], &data[4]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&sig_m_config, 0, sizeof(sig_m_config));
	sig_m_config.block_size = (uint16_t)data[0];
	sig_m_config.p2p_min_intvl = (uint16_t)data[1];
	sig_m_config.p2p_max_intvl = (uint16_t)data[2];
	sig_m_config.mcr_min = (uint16_t)data[3];
	sig_m_config.mcr_max = (uint16_t)data[4];
	#endif
	#if defined(BMA422)
	err = bma422_set_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_set_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_set_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_set_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_set_sig_motion_config(&sig_m_config, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif
#if defined(BMA422) || defined(BMA455) || defined(BMA424)
static ssize_t bma4xy_show_tilt_threshold(
	struct device_driver *ddri, char *buf)
{
	int err;
	uint8_t tilt_threshold;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA422)
	err = bma422_tilt_get_threshold(&tilt_threshold, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_tilt_get_threshold(&tilt_threshold, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_tilt_get_threshold(&tilt_threshold, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 32, "%d\n", tilt_threshold);
}
static ssize_t bma4xy_store_tilt_threshold(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long tilt_threshold;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &tilt_threshold);
	if (err)
		return err;
	GSE_LOG("tilt_threshold %ld", tilt_threshold);
	#if defined(BMA422)
	err = bma422_tilt_set_threshold(tilt_threshold, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_tilt_set_threshold(tilt_threshold, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_tilt_set_threshold(tilt_threshold, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif
#if !defined(BMA420) && !defined(BMA456)
static ssize_t bma4xy_show_step_counter_val(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	uint32_t step_counter_val = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA421)
	err = bma421_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	GSE_LOG("val %u", step_counter_val);
	if (client_data->err_int_trigger_num == 0) {
		client_data->step_counter_val = step_counter_val;
		GSE_LOG("report val %u", client_data->step_counter_val);
		err = snprintf(buf, 96, "%u\n", client_data->step_counter_val);
		client_data->step_counter_temp = client_data->step_counter_val;
	} else {
		GSE_LOG("after err report val %u",
			client_data->step_counter_val + step_counter_val);
		err = snprintf(buf, 96, "%u\n",
			client_data->step_counter_val + step_counter_val);
		client_data->step_counter_temp =
			client_data->step_counter_val + step_counter_val;
	}
	return err;
}
static ssize_t bma4xy_show_step_counter_watermark(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	uint16_t watermark;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA421)
	err = bma421_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_step_counter_get_watermark(
	&watermark, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, 32, "%d\n", watermark);
}
static ssize_t bma4xy_store_step_counter_watermark(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long step_watermark;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &step_watermark);
	if (err)
		return err;
	GSE_LOG("watermark step_counter %ld", step_watermark);
	#if defined(BMA421)
	err = bma421_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_step_counter_set_watermark(
	step_watermark, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}

static ssize_t bma4xy_show_step_counter_parameter(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA421)
	struct bma421_stepcounter_settings setting;
	#elif defined(BMA424SC)
	struct bma424sc_stepcounter_settings setting;
	#elif defined(BMA421L)
	struct bma421l_stepcounter_settings setting;
	#elif defined(BMA422)
	struct bma422_stepcounter_settings setting;
	#elif defined(BMA422N)
	struct bma422n_stepcounter_settings setting;
	#elif defined(BMA455N)
	struct bma455n_stepcounter_settings setting;
	#elif defined(BMA424)
	struct bma424_stepcounter_settings setting;
	#elif defined(BMA455)
	struct bma455_stepcounter_settings setting;
	#endif
	#if defined(BMA421)
	err = bma421_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_stepcounter_get_parameter(
	&setting, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_stepcounter_get_parameter(&setting, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}

#if defined(BMA421L) || defined(BMA422) || defined(BMA455) || defined(BMA424)
	return snprintf(buf, PAGE_SIZE,
	"parameter1 =0x%x parameter2= 0x%x\n"
	"parameter3 = 0x%x parameter4 = 0x%x\n"
	"parameter5 = 0x%x parameter6 = 0x%x\n"
	"parameter7 = 0x%x\n",
	setting.param1, setting.param2, setting.param3, setting.param4,
	setting.param5, setting.param6, setting.param7);
#elif defined(BMA421) || defined(BMA424SC) || defined(BMA422N) || defined(BMA455N)
	return snprintf(buf, PAGE_SIZE,
	"parameter1 =0x%x parameter2= 0x%x\n"
	"parameter3 = 0x%x parameter4 = 0x%x\n"
	"parameter5 = 0x%x parameter6 = 0x%x\n"
	"parameter7 = 0x%x parameter8 = 0x%x\n"
	"parameter9 = 0x%x parameter10 = 0x%x\n"
	"parameter11 = 0x%x parameter12 = 0x%x\n"
	"parameter13 = 0x%x parameter14 = 0x%x\n"
	"parameter15 = 0x%x parameter16 = 0x%x\n"
	"parameter17 = 0x%x parameter18 = 0x%x\n"
	"parameter19 = 0x%x parameter20 = 0x%x\n"
	"parameter21 = 0x%x parameter22 = 0x%x\n"
	"parameter23 = 0x%x parameter24 = 0x%x\n"
	"parameter25 = 0x%x\n",
	setting.param1, setting.param2, setting.param3, setting.param4,
	setting.param5, setting.param6, setting.param7, setting.param8,
	setting.param9, setting.param10, setting.param11, setting.param12,
	setting.param13, setting.param14, setting.param15, setting.param16,
	setting.param17, setting.param18, setting.param19, setting.param20,
	setting.param21, setting.param22, setting.param23, setting.param24,
	setting.param25);
#endif
}
static ssize_t bma4xy_store_step_counter_parameter(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA421)
	unsigned int data[25] = {0};
	struct bma421_stepcounter_settings setting;
	#elif defined(BMA424SC)
	unsigned int data[25] = {0};
	struct bma424sc_stepcounter_settings setting;
	#elif defined(BMA422N)
	unsigned int data[25] = {0};
	struct bma422n_stepcounter_settings setting;
	#elif defined(BMA455N)
	unsigned int data[25] = {0};
	struct bma455n_stepcounter_settings setting;
	#elif defined(BMA421L)
	unsigned int data[7] = {0};
	struct bma421l_stepcounter_settings setting;
	#elif defined(BMA422)
	unsigned int data[7] = {0};
	struct bma422_stepcounter_settings setting;
	#elif defined(BMA424)
	unsigned int data[7] = {0};
	struct bma424_stepcounter_settings setting;
	#elif defined(BMA455)
	unsigned int data[7] = {0};
	struct bma455_stepcounter_settings setting;
	#endif

#if defined(BMA421L) || defined(BMA422) || defined(BMA455) || defined(BMA424)
	err = sscanf(buf, "%11x %11x %11x %11x %11x %11x %11x",
	&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6]);
	if (err != 7) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	setting.param1 = (uint16_t)data[0];
	setting.param2 = (uint16_t)data[1];
	setting.param3 = (uint16_t)data[2];
	setting.param4 = (uint16_t)data[3];
	setting.param5 = (uint16_t)data[4];
	setting.param6 = (uint16_t)data[5];
	setting.param7 = (uint16_t)data[6];
#elif defined(BMA421) || defined(BMA424SC) || defined(BMA422N) || defined(BMA455N)
	err = sscanf(buf,
	"%11x %11x %11x %11x %11x %11x %11x %11x\n"
	"%11x %11x %11x %11x %11x %11x %11x %11x\n"
	"%11x %11x %11x %11x %11x %11x %11x %11x\n"
	"%11x\n",
	&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6],
	&data[7], &data[8], &data[9], &data[10], &data[11], &data[12],
	&data[13],
	&data[14], &data[15], &data[16], &data[17], &data[18], &data[19],
	&data[20],
	&data[21], &data[22], &data[23], &data[24]);
	if (err != 25) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	setting.param1 = (uint16_t)data[0];
	setting.param2 = (uint16_t)data[1];
	setting.param3 = (uint16_t)data[2];
	setting.param4 = (uint16_t)data[3];
	setting.param5 = (uint16_t)data[4];
	setting.param6 = (uint16_t)data[5];
	setting.param7 = (uint16_t)data[6];
	setting.param8 = (uint16_t)data[7];
	setting.param9 = (uint16_t)data[8];
	setting.param10 = (uint16_t)data[9];
	setting.param11 = (uint16_t)data[10];
	setting.param12 = (uint16_t)data[11];
	setting.param13 = (uint16_t)data[12];
	setting.param14 = (uint16_t)data[13];
	setting.param15 = (uint16_t)data[14];
	setting.param16 = (uint16_t)data[15];
	setting.param17 = (uint16_t)data[16];
	setting.param18 = (uint16_t)data[17];
	setting.param19 = (uint16_t)data[18];
	setting.param20 = (uint16_t)data[19];
	setting.param21 = (uint16_t)data[20];
	setting.param22 = (uint16_t)data[21];
	setting.param23 = (uint16_t)data[22];
	setting.param24 = (uint16_t)data[23];
	setting.param25 = (uint16_t)data[24];
#endif
	#if defined(BMA421)
	err = bma421_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_stepcounter_set_parameter(
	&setting, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_stepcounter_set_parameter(&setting, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}

static ssize_t bma4xy_store_step_counter_reset(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long reset_counter;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &reset_counter);
	if (err)
		return err;
	GSE_LOG("reset_counter %ld", reset_counter);
	#if defined(BMA421)
	err = bma421_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_reset_step_counter(&client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_reset_step_counter(&client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	client_data->step_counter_val = 0;
	client_data->step_counter_temp = 0;
	return count;
}
#endif
static ssize_t bma4xy_show_anymotion_config(
	struct device_driver *ddri, char *buf)
{
	int err;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA420)
	struct bma420_anymotion_config any_motion;
	#elif defined(BMA421)
	struct bma421_anymotion_config any_motion;
	#elif defined(BMA424SC)
	struct bma424sc_anymotion_config any_motion;
	#elif defined(BMA421L)
	struct bma421l_anymotion_config any_motion;
	#elif defined(BMA422)
	struct bma422_anymotion_config any_motion;
	#elif defined(BMA422N)
	struct bma422n_anymotion_config any_motion;
	#elif defined(BMA455N)
	struct bma455n_anymotion_config any_motion;
	#elif defined(BMA424)
	struct bma424_anymotion_config any_motion;
	#elif defined(BMA455)
	struct bma455_anymotion_config any_motion;
	#endif

	#if defined(BMA420)
	err = bma420_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA421)
	err = bma421_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_get_any_motion_config(&any_motion, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	#if !defined(BMA422N) && !defined(BMA455N)
	return snprintf(buf, PAGE_SIZE,
	"duration =0x%x threshold= 0x%x nomotion_sel = 0x%x\n",
	any_motion.duration, any_motion.threshold,
	any_motion.nomotion_sel);
	#else
	return snprintf(buf, PAGE_SIZE,
	"duration =0x%x threshold= 0x%x\n",
	any_motion.duration, any_motion.threshold);
	#endif

}
static ssize_t bma4xy_store_anymotion_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	#if !defined(BMA422N) && !defined(BMA455N)
	unsigned int data[3] = {0};
	#else
	unsigned int data[2] = {0};
	#endif
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA420)
	struct bma420_anymotion_config any_motion;
	#elif defined(BMA421)
	struct bma421_anymotion_config any_motion;
	#elif defined(BMA424SC)
	struct bma424sc_anymotion_config any_motion;
	#elif defined(BMA421L)
	struct bma421l_anymotion_config any_motion;
	#elif defined(BMA422)
	struct bma422_anymotion_config any_motion;
	#elif defined(BMA422N)
	struct bma422n_anymotion_config any_motion;
	#elif defined(BMA455N)
	struct bma455n_anymotion_config any_motion;
	#elif defined(BMA424)
	struct bma424_anymotion_config any_motion;
	#elif defined(BMA455)
	struct bma455_anymotion_config any_motion;
	#endif

	#if !defined(BMA422N) && !defined(BMA455N)
	err = sscanf(buf, "%11x %11x %11x", &data[0], &data[1], &data[2]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&any_motion, 0, sizeof(any_motion));
	any_motion.duration = (uint16_t)data[0];
	any_motion.threshold = (uint16_t)data[1];
	any_motion.nomotion_sel = (uint8_t)data[2];
	#else
	err = sscanf(buf, "%11x %11x", &data[0], &data[1]);
	if (err != 2) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&any_motion, 0, sizeof(any_motion));
	any_motion.duration = (uint16_t)data[0];
	any_motion.threshold = (uint16_t)data[1];
	#endif
	#if defined(BMA420)
	err = bma420_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA421)
	err = bma421_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_set_any_motion_config(&any_motion, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
static ssize_t bma4xy_show_anymotion_enable_axis(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);


	return snprintf(buf, PAGE_SIZE,
	"anymotion_enable_axis = %d\n",
	client_data->any_motion_axis);
}

static ssize_t bma4xy_store_anymotion_enable_axis(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long data;
	uint8_t enable_axis;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	GSE_LOG("enable_axis %ld", data);
	enable_axis = (uint8_t)data;
	#if defined(BMA420)
	err = bma420_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA421)
	err = bma421_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA422N)
	client_data->any_motion_axis = enable_axis;
	#endif
	#if defined(BMA455N)
	client_data->any_motion_axis = enable_axis;
	#endif
	#if defined(BMA424)
	err = bma424_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_anymotion_enable_axis(enable_axis, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}

#if defined(BMA422N) ||defined(BMA455N)
static ssize_t bma4xy_show_no_motion_enable_axis(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE,
	"nomotion_enable_axis = %d\n",
	client_data->no_motion_axis);
}

static ssize_t bma4xy_store_no_motion_enable_axis(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned long data;
	uint8_t enable_axis;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	GSE_LOG("enable_axis %ld", data);
	enable_axis = (uint8_t)data;
	#if defined(BMA422N)
	client_data->no_motion_axis =enable_axis;
	#endif
	#if defined(BMA455N)
	client_data->no_motion_axis = enable_axis;
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}

static ssize_t bma4xy_show_nomotion_config(
	struct device_driver *ddri, char *buf)
{
	int err;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA422N)
	struct bma422n_nomotion_config no_motion;
	#elif defined(BMA455N)
	struct bma455n_nomotion_config no_motion;
	#endif
	#if defined(BMA422N)
	err = bma422n_get_no_motion_config(&no_motion, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_get_no_motion_config(&no_motion, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, PAGE_SIZE,
	"duration =0x%x threshold= 0x%x\n",
	no_motion.duration, no_motion.threshold);
}
static ssize_t bma4xy_store_nomotion_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned int data[2] = {0};
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA422N)
	struct bma422n_nomotion_config no_motion;
	#elif defined(BMA455N)
	struct bma455n_nomotion_config no_motion;
	#endif
	err = sscanf(buf, "%11x %11x", &data[0], &data[1]);
	if (err != 2) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&no_motion, 0, sizeof(no_motion));
	no_motion.duration = (uint16_t)data[0];
	no_motion.threshold = (uint16_t)data[1];
	#if defined(BMA422N)
	err = bma422n_set_no_motion_config(&no_motion, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_set_no_motion_config(&no_motion, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}

#endif
#if defined(BMA420) || defined(BMA456) || defined(BMA422N) || defined(BMA455N)
static ssize_t bma4xy_show_orientation_config(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA420)
	struct bma420_orientation_config orientation;
	#elif defined(BMA422N)
	struct bma422n_orientation_config orientation;
	#elif defined(BMA455N)
	struct bma455n_orientation_config orientation;
	#endif

	#if defined(BMA420)
	err = bma420_get_orientation_config(&orientation, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422n_get_orientation_config(&orientation, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_get_orientation_config(&orientation, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	#if defined(BMA420)
	return snprintf(buf, PAGE_SIZE,
	"upside_down =0x%x mode= 0x%x\n"
	"blocking = 0x%x hysteresis = 0x%x\n",
	orientation.upside_down, orientation.mode,
	orientation.blocking, orientation.hysteresis);
	#endif
	#if defined(BMA422N) || defined(BMA455N)
	return snprintf(buf, PAGE_SIZE,
	"upside_down =0x%x mode= 0x%x\n"
	"blocking = 0x%x theta = 0x%x hysteresis = 0x%x\n",
	orientation.upside_down, orientation.mode,
	orientation.blocking, orientation.theta, orientation.hysteresis);
	#endif
}
static ssize_t bma4xy_store_orientation_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	#if defined(BMA420)
	unsigned int data[4] = {0};
	#endif
	struct i2c_client *client = bma4xy_i2c_client;
	#if defined(BMA422N) || defined(BMA455N)
	unsigned int data[5] = {0};
	#endif
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	#if defined(BMA420)
	struct bma420_orientation_config orientation;
	#elif defined(BMA422N)
	struct bma422n_orientation_config orientation;
	#elif defined(BMA455N)
	struct bma455n_orientation_config orientation;
	#endif
	#if defined(BMA420)
	err = sscanf(buf, "%11x %11x %11x %11x",
	&data[0], &data[1], &data[2], &data[3]);
	if (err != 4) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&orientation, 0, sizeof(orientation));
	orientation.upside_down = (uint8_t)data[0];
	orientation.mode = (uint8_t)data[1];
	orientation.blocking = (uint8_t)data[2];
	orientation.hysteresis = (uint16_t)data[3];
	err = bma420_set_orientation_config(&orientation, &client_data->device);
	#endif
	#if defined(BMA422N) || defined(BMA455N)
	err = sscanf(buf, "%11x %11x %11x %11x %11x",
	&data[0], &data[1], &data[2], &data[3], &data[4]);
	if (err != 5) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&orientation, 0, sizeof(orientation));
	orientation.upside_down = (uint8_t)data[0];
	orientation.mode = (uint8_t)data[1];
	orientation.blocking = (uint8_t)data[2];
	orientation.theta = (uint8_t)data[3];
	orientation.hysteresis = (uint16_t)data[4];
	#if defined(BMA422N)
	err = bma422n_set_orientation_config(&orientation, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_set_orientation_config(&orientation, &client_data->device);
	#endif
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif
#if defined(BMA420) || defined(BMA420L) || defined(BMA456)
static ssize_t bma4xy_show_flat_config(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_flat_config flat;

	#if defined(BMA420)
	err = bma420_get_flat_config(&flat, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_get_flat_config(&flat, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed %d", client_data->flat_enable);
		return err;
	}
	return snprintf(buf, PAGE_SIZE,
	"theta =0x%x blocking= 0x%x\n"
	"hysteresis = 0x%x hold_time = 0x%x\n",
	flat.theta, flat.blocking,
	flat.hysteresis, flat.hold_time);
}
static ssize_t bma4xy_store_flat_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned int data[4] = {0};
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_flat_config flat;

	err = sscanf(buf, "%11x %11x %11x %11x",
	&data[0], &data[1], &data[2], &data[3]);
	if (err != 4) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&flat, 0, sizeof(flat));
	flat.theta = (uint8_t)data[0];
	flat.blocking = (uint8_t)data[1];
	flat.hysteresis = (uint8_t)data[2];
	flat.hold_time = (uint8_t)data[3];
	#if defined(BMA420)
	err = bma420_set_flat_config(&flat, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_set_flat_config(&flat, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed %d", client_data->flat_enable);
		return err;
	}
	return count;
}
#endif
#if defined(BMA420) || defined(BMA456)
static ssize_t bma4xy_show_highg_config(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_high_g_config highg_config;

	err = bma420_get_high_g_config(&highg_config, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, PAGE_SIZE,
	"threshold =0x%x hysteresis= 0x%x duration = 0x%x\n",
	highg_config.threshold, highg_config.hysteresis,
	highg_config.duration);
}
static ssize_t bma4xy_store_highg_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned int data[3] = {0};
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_high_g_config highg_config;

	err = sscanf(buf, "%11x %11x %11x", &data[0], &data[1], &data[2]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&highg_config, 0, sizeof(highg_config));
	highg_config.threshold = (uint16_t)data[0];
	highg_config.hysteresis = (uint16_t)data[1];
	highg_config.duration = (uint8_t)data[2];
	err = bma420_set_high_g_config(&highg_config, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif
#if defined(BMA420) || defined(BMA456)
static ssize_t bma4xy_show_lowg_config(
	struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_low_g_config lowg_config;

	err = bma420_get_low_g_config(&lowg_config, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	return snprintf(buf, PAGE_SIZE,
	"threshold = 0x%x hysteresis= 0x%x duration = 0x%x\n",
	lowg_config.threshold, lowg_config.hysteresis,
	lowg_config.duration);

}
static ssize_t bma4xy_store_lowg_config(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	unsigned int data[3] = {0};
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	struct bma420_low_g_config lowg_config;

	err = sscanf(buf, "%11x %11x %11x", &data[0], &data[1], &data[2]);
	if (err != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	memset(&lowg_config, 0, sizeof(lowg_config));
	lowg_config.threshold = (uint16_t)data[0];
	lowg_config.hysteresis = (uint16_t)data[1];
	lowg_config.duration = (uint8_t)data[2];
	err = bma420_set_low_g_config(&lowg_config, &client_data->device);
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif
#if defined(BMA420) || defined(BMA456)
static ssize_t bma4xy_show_tap_type(
	struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);
	return snprintf(buf, 8, "%d\n", client_data->tap_type);
}
static ssize_t bma4xy_store_tap_type(
	struct device_driver *ddri, const char *buf, size_t count)
{
	int32_t err = 0;
	unsigned long data;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = kstrtoul(buf, 16, &data);
	if (err)
		return err;
	client_data->tap_type = (uint8_t)data;
	#ifdef BMA420
	err = bma420_set_tap_selection(
	client_data->tap_type, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("write failed");
		return err;
	}
	return count;
}
#endif

#if defined(BMA420)
int bma420_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA420_ANY_MOTION;
	if (client_data->orientation_enable == BMA4_ENABLE)
		feature = feature | BMA420_ORIENTATION;
	if (client_data->flat_enable == BMA4_ENABLE)
		feature = feature | BMA420_FLAT;
	if (client_data->tap_enable == BMA4_ENABLE)
		feature = feature | BMA420_TAP;
	if (client_data->highg_enable == BMA4_ENABLE)
		feature = feature | BMA420_HIGH_G;
	if (client_data->lowg_enable == BMA4_ENABLE)
		feature = feature | BMA420_LOW_G;
	err = bma420_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;

}
#endif
#if defined(BMA421)
int bma421_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma421_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA421_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA421_ANY_MOTION;
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA421_STEP_CNTR;
	if (client_data->activity_enable == BMA4_ENABLE)
		feature = feature | BMA421_ACTIVITY;
	err = bma421_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif
#if defined(BMA424SC)
int bma424sc_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma424sc_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA424SC_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA424SC_ANY_MOTION;
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA424SC_STEP_CNTR;
	if (client_data->activity_enable == BMA4_ENABLE)
		feature = feature | BMA424SC_ACTIVITY;
	err = bma424sc_feature_enable(
		feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif

#if defined(BMA421L)
int bma421l_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma421l_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA421L_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA421L_ANY_MOTION;
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA421L_STEP_CNTR;
	if (client_data->flat_enable == BMA4_ENABLE)
		feature = feature | BMA421L_FLAT;
	err = bma421l_feature_enable(
		feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif

#if defined(BMA422)
int bma422_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->sigmotion_enable == BMA4_ENABLE)
		feature = feature | BMA422_SIG_MOTION;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma422_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA422_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA422_STEP_CNTR;
	if (client_data->tilt_enable == BMA4_ENABLE)
		feature = feature | BMA422_TILT;
	if (client_data->pickup_enable == BMA4_ENABLE)
		feature = feature | BMA422_PICKUP;
	if (client_data->glance_enable == BMA4_ENABLE)
		feature = feature | BMA422_GLANCE;
	if (client_data->wakeup_enable == BMA4_ENABLE)
		feature = feature | BMA422_WAKEUP;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA422_ANY_MOTION;
	err = bma422_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif
#if defined(BMA422N)
int bma422n_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->sigmotion_enable == BMA4_ENABLE)
		feature = feature | BMA422N_SIG_MOTION;
	if (client_data->stepdet_enable == BMA4_ENABLE) 
		feature = feature | BMA422N_STEP_DETR;
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA422N_STEP_CNTR;
	if (client_data->orientation_enable== BMA4_ENABLE)
		feature = feature | BMA422N_ORIENTATION;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		err = bma422n_anymotion_enable_axis(client_data->any_motion_axis, &client_data->device);
	err = bma422n_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif
#if defined(BMA455N)
int bma455n_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->sigmotion_enable == BMA4_ENABLE)
		feature = feature | BMA455N_SIG_MOTION;
	if (client_data->stepdet_enable == BMA4_ENABLE) 
		feature = feature | BMA455N_STEP_DETR;
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA455N_STEP_CNTR;
	if (client_data->orientation_enable== BMA4_ENABLE)
		feature = feature | BMA455N_ORIENTATION;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		err = bma455n_anymotion_enable_axis(client_data->any_motion_axis, &client_data->device);
	err = bma455n_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif

#if defined(BMA424)
int bma424_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->sigmotion_enable == BMA4_ENABLE)
		feature = feature | BMA424_SIG_MOTION;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma424_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA422_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA424_STEP_CNTR;
	if (client_data->tilt_enable == BMA4_ENABLE)
		feature = feature | BMA424_TILT;
	if (client_data->pickup_enable == BMA4_ENABLE)
		feature = feature | BMA424_PICKUP;
	if (client_data->glance_enable == BMA4_ENABLE)
		feature = feature | BMA424_GLANCE;
	if (client_data->wakeup_enable == BMA4_ENABLE)
		feature = feature | BMA424_WAKEUP;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA424_ANY_MOTION;
	err = bma424_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif

#if defined(BMA455)
int bma455_config_feature(struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t feature = 0;
	if (client_data->sigmotion_enable == BMA4_ENABLE)
		feature = feature | BMA455_SIG_MOTION;
	if (client_data->stepdet_enable == BMA4_ENABLE) {
		if (bma455_step_detector_enable(
			BMA4_ENABLE, &client_data->device) < 0)
			GSE_ERR("set BMA455_STEP_DECTOR error");
	}
	bma4xy_i2c_delay(2);
	if (client_data->stepcounter_enable == BMA4_ENABLE)
		feature = feature | BMA455_STEP_CNTR;
	if (client_data->tilt_enable == BMA4_ENABLE)
		feature = feature | BMA455_TILT;
	if (client_data->pickup_enable == BMA4_ENABLE)
		feature = feature | BMA455_PICKUP;
	if (client_data->glance_enable == BMA4_ENABLE)
		feature = feature | BMA455_GLANCE;
	if (client_data->wakeup_enable == BMA4_ENABLE)
		feature = feature | BMA455_WAKEUP;
	if (client_data->anymotion_enable == BMA4_ENABLE)
		feature = feature | BMA455_ANY_MOTION;
	err = bma455_feature_enable(feature, BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set feature err");
	return err;
}
#endif


#if defined(BMA4XY_ENABLE_INT1) || defined(BMA4XY_ENABLE_INT2)
static int bma4xy_reinit_after_error_interrupt(
	struct bma4xy_client_data *client_data)
{
	int err = 0;
	uint8_t data = 0;
	uint8_t crc_check = 0;

	client_data->err_int_trigger_num += 1;
	client_data->step_counter_val = client_data->step_counter_temp;
	/*reset the bma4xy*/
	err = bma455_set_command_register(0xB6, &client_data->device);
	if (!err)
		GSE_LOG("reset chip");
	/*reinit the fifo config*/
	err = bma4xy_init_fifo_config(client_data);
	if (err)
		GSE_ERR("fifo init failed");
	/*reload the config_stream*/
	err = bma455_write_config_file(&client_data->device);
	if (err)
		GSE_ERR("download config stream failer");
	bma4xy_i2c_delay(200);
	err = bma455_read_regs(BMA4_INTERNAL_STAT,
	&crc_check, 1, &client_data->device);
	if (err)
		GSE_ERR("reading CRC failer");
	if (crc_check != BMA4_ASIC_INITIALIZED)
		GSE_ERR("crc check error %x", crc_check);
	/*reconfig interrupt and remap*/
	err = bma4xy_init_after_config_stream_load(client_data);
	if (err)
		GSE_ERR("reconfig interrupt and remap error");
	/*reinit the feature*/
	#if defined(BMA420)
	err = bma420_config_feature(client_data);
	#endif
	#if defined(BMA421)
	err = bma421_config_feature(client_data);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_config_feature(client_data);
	#endif
	#if defined(BMA421L)
	err = bma421l_config_feature(client_data);
	#endif
	#if defined(BMA422)
	err = bma422_config_feature(client_data);
	#endif
	#if defined(BMA422N)
	err = bma422n_config_feature(client_data);
	#endif
	#if defined(BMA455N)
	err = bma455n_config_feature(client_data);
	#endif
	#if defined(BMA424)
	err = bma424_config_feature(client_data);
	#endif
	#if defined(BMA455)
	err = bma455_config_feature(client_data);
	#endif
	if (err)
		GSE_ERR("reinit the virtual sensor error");
	/*reinit acc*/
	if (client_data->acc_odr != 0) {
		data = client_data->acc_odr;
		if (data == 4)
			data = 0x74;
		else
			data |= 0xA0;
		err = client_data->device.bus_write(
			client_data->device.dev_addr,
			0x40, &data, 1);
		if (err)
			GSE_ERR("set acc_odr faliled");
		bma4xy_i2c_delay(2);
	}
	if (client_data->acc_pm == 0)
		err = bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	bma4xy_i2c_delay(2);
	err = bma455_set_fifo_config(BMA4_FIFO_ACCEL,
		client_data->fifo_acc_enable, &client_data->device);
	if (err)
		GSE_ERR("set acc_fifo_enable faliled");
	bma4xy_i2c_delay(5);
	return 0;
}

static void bma4xy_uc_function_handle(
	struct bma4xy_client_data *client_data, uint8_t status)
{
	int err = 0;
#if defined(BMA420) || defined(BMA456)
	unsigned char uc_gpio[2] = {0};
#endif
	if (status & ERROR_INT_OUT) {
		err = bma4xy_reinit_after_error_interrupt(client_data);
		if (err)
			GSE_ERR("reinit failed");
	}
#if defined(BMA422) || defined(BMA421) || \
defined(BMA421L) || defined(BMA455) || defined(BMA424) || defined(BMA424SC) || defined(BMA422N) || defined(BMA455N)
	#if defined(BMA4_STEP_COUNTER)
	if ((status & STEP_DET_OUT) == 0x02)
		step_notify(TYPE_STEP_DETECTOR);
	if ((status & SIG_MOTION_OUT) == 0x01)
		step_notify(TYPE_SIGNIFICANT);
	#endif
	#if defined(BMA4_WAKEUP)
	if ((status & WAKEUP_OUT) == 0x20)
		wag_notify();
	#endif
#endif
#if defined(BMA420) || defined(BMA456)
	if (client_data->orientation_enable)
		err = bma455_read_regs(BMA4_STEP_CNT_OUT_0_ADDR,
		&uc_gpio[0], 1, &client_data->device);
	if (client_data->highg_enable)
		err += bma455_read_regs(BMA4_HIGH_G_OUT_ADDR,
		&uc_gpio[1], 1, &client_data->device);
	if (err) {
		GSE_ERR("read uc_gpio failed");
		return;
	}
	GSE_LOG("%d %d %d", uc_gpio[0], uc_gpio[1], uc_gpio[2]);
#endif
}

static void bma4xy_irq_work_func(struct work_struct *work)
{
	struct bma4xy_client_data *client_data = container_of(work,
		struct bma4xy_client_data, irq_work);
	unsigned char int_status[2] = {0, 0};
	int err = 0;
	int in_suspend_copy;

	in_suspend_copy = atomic_read(&client_data->in_suspend);
	/*read the interrut status two register*/
	err = client_data->device.bus_read(client_data->device.dev_addr,
				BMA4_INT_STAT_0_ADDR, int_status, 2);
	if (err)
		return;
	GSE_LOG("int_status0 = 0x%x int_status1 =0x%x",
		int_status[0], int_status[1]);
	if (in_suspend_copy &&
		((int_status[0] & STEP_DET_OUT) == 0x02)) {
		return;
	}
	if (int_status[0])
		bma4xy_uc_function_handle(client_data, (uint8_t)int_status[0]);
}

static void bma4xy_delay_sigmo_work_func(struct work_struct *work)
{
	struct bma4xy_client_data *client_data =
	container_of(work, struct bma4xy_client_data,
	delay_work_sig.work);
	unsigned char int_status[2] = {0, 0};
	int err = 0;
	/*read the interrut status two register*/
	err = client_data->device.bus_read(client_data->device.dev_addr,
				BMA4_INT_STAT_0_ADDR, int_status, 2);
	if (err)
		return;
	GSE_LOG("int_status0 = %x int_status1 =%x",
		int_status[0], int_status[1]);
	#if defined(BMA4_STEP_COUNTER)
	if ((int_status[0] & SIG_MOTION_OUT) == 0x01)
		step_notify(TYPE_SIGNIFICANT);
	#endif
}

static irqreturn_t bma4xy_irq_handle(int irq, void *handle)
{
	struct bma4xy_client_data *client_data = handle;
	int in_suspend_copy;

	in_suspend_copy = atomic_read(&client_data->in_suspend);
	/*this only deal with SIG_motion CTS test*/
	if ((in_suspend_copy == 1) &&
		((client_data->sigmotion_enable == 1) &&
		(client_data->tilt_enable != 1) &&
		(client_data->pickup_enable != 1) &&
		(client_data->glance_enable != 1) &&
		(client_data->wakeup_enable != 1))) {
		wake_lock_timeout(&client_data->wakelock, HZ);
		schedule_delayed_work(&client_data->delay_work_sig,
			msecs_to_jiffies(50));
	} else if ((in_suspend_copy == 1) &&
		((client_data->sigmotion_enable == 1) ||
		(client_data->tilt_enable == 1) ||
		(client_data->pickup_enable == 1) ||
		(client_data->glance_enable == 1) ||
		(client_data->wakeup_enable == 1))) {
		wake_lock_timeout(&client_data->wakelock, HZ);
		schedule_work(&client_data->irq_work);
	} else
		schedule_work(&client_data->irq_work);
	return IRQ_HANDLED;
}

static int bma4xy_request_irq(struct bma4xy_client_data *client_data)
{
	int err = 0;
/*
	client_data->gpio_pin = 63;
	err = gpio_request_one(client_data->gpio_pin,
				GPIOF_IN, "bma4xy_interrupt");
	if (err < 0)
		return err;
	err = gpio_direction_input(client_data->gpio_pin);
	if (err < 0)
		return err;
	client_data->IRQ = gpio_to_irq(client_data->gpio_pin);
	err = request_irq(client_data->IRQ, bma4xy_irq_handle,
			IRQF_TRIGGER_RISING,
			SENSOR_NAME, client_data);
	if (err < 0)
		return err;
*/
	struct device_node *node = NULL;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");
	if (node) {
		/*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
		client_data->IRQ = irq_of_parse_and_map(node, 0);
		ret = request_irq(client_data->IRQ, bma4xy_irq_handle,
			IRQF_TRIGGER_RISING,
			SENSOR_NAME, client_data);
			if (ret > 0)
				printk(KERN_ERR "bma4xy request_irq failed");
	} else {
		printk(KERN_INFO "[%s] can not find!", __func__);
	}
	printk(KERN_INFO "irq_num= %d IRQ_num=%d",
		client_data->gpio_pin, client_data->IRQ);
	INIT_WORK(&client_data->irq_work, bma4xy_irq_work_func);
	INIT_DELAYED_WORK(&client_data->delay_work_sig,
		bma4xy_delay_sigmo_work_func);
	return err;
}
#endif

static int bma4xy_init_client(struct i2c_client *client, int reset_cali)
{
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int res = 0;

	GSE_FUN();
	res = BMA4XY_CheckDeviceID(client);
	if (res) {
		GSE_ERR("check device ID failed");
		return res;
	}
	res = BMA4XY_SetDataFormat(client, BMA4_ACCEL_RANGE_4G);
	if (res) {
		GSE_ERR("SetDataFormat failed");
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z =
		obj->reso->sensitivity;
	res = BMA4XY_SetPowerMode(client, 0);
	if (res) {
		GSE_ERR("SetPowerMode failed");
		return res;
	}
	if (0 != reset_cali) {
		/*reset calibration only in power on */
		res = BMA4XY_ResetCalibration(client);
		if (res) {
			GSE_ERR("ResetCalibration failed");
			return res;
		}
	}
	/*load config_stream*/
	res = bma4xy_load_config_stream(obj);
	if (res)
		GSE_ERR("init failed after load config_stream");
	GSE_LOG("BMA4XY_init_client OK!\n");
	return 0;
}

static int bma4xy_open(struct inode *inode, struct file *file)
{
	file->private_data = bma4xy_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int bma4xy_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long bma4xy_unlocked_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct bma4xy_client_data *obj =
		(struct bma4xy_client_data *)i2c_get_clientdata(client);
	char strbuf[BMA4XY_BUFSIZE];
	void __user *data;
	/*struct SENSOR_DATA sensor_data;*/
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	/* GSE_FUN(f); */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
		(void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
		(void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n",
			cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		bma4xy_init_client(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		BMA4XY_ReadChipInfo(client, strbuf, BMA4XY_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_SENSORDATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA4XY_SetPowerMode(client, 1);
		BMA4XY_ReadSensorData(client, strbuf, BMA4XY_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_GAIN:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_to_user(data, &gsensor_gain,
			sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_RAW_DATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA4XY_ReadRawData(client, strbuf);
		if (copy_to_user(data, &strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_SET_CALI:
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
			GSE_ERR("Perform calibration in suspend state!!\n");
			err = -EINVAL;
		} else {
			cali[BMA4XY_ACC_AXIS_X] =
		sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[BMA4XY_ACC_AXIS_Y] =
		sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[BMA4XY_ACC_AXIS_Z] =
		sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			err = BMA4XY_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = BMA4XY_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = BMA4XY_ReadCalibration(client, cali);
		if (0 != err)
			break;
		sensor_data.x =
		cali[BMA4XY_ACC_AXIS_X] *
		GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.y =
		cali[BMA4XY_ACC_AXIS_Y] *
		GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.z =
		cali[BMA4XY_ACC_AXIS_Z] *
		GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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
#ifdef CONFIG_COMPAT
static long bma4xy_compat_ioctl(struct file *file,
unsigned int cmd, unsigned long arg)
{
	long err = 0;

	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_GSENSOR_IOCTL_READ_SENSORDATA:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}
		err = file->f_op->unlocked_ioctl(file,
			GSENSOR_IOCTL_READ_SENSORDATA, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_READ_SENSORDATA failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_SET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}
		err = file->f_op->unlocked_ioctl(file,
			GSENSOR_IOCTL_SET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_SET_CALI failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_GET_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}
		err = file->f_op->unlocked_ioctl(file,
			GSENSOR_IOCTL_GET_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_GET_CALI failed.");
			return err;
		}
		break;
	case COMPAT_GSENSOR_IOCTL_CLR_CALI:
		if (arg32 == NULL) {
			err = -EINVAL;
			break;
		}
		err = file->f_op->unlocked_ioctl(file,
			GSENSOR_IOCTL_CLR_CALI, (unsigned long)arg32);
		if (err) {
			GSE_ERR("GSENSOR_IOCTL_CLR_CALI failed.");
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

static const struct file_operations bma4xy_fops = {
	.owner = THIS_MODULE,
	.open = bma4xy_open,
	.release = bma4xy_release,
	.unlocked_ioctl = bma4xy_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = bma4xy_compat_ioctl,
#endif
};

static int bma4xy_i2c_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("suspend function entrance");
	enable_irq_wake(client_data->IRQ);
	atomic_set(&client_data->in_suspend, 1);
	return err;
}

static int bma4xy_i2c_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("resume function entrance");
	disable_irq_wake(client_data->IRQ);
	atomic_set(&client_data->in_suspend, 0);
	return err;
}
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	char strbuf[BMA4XY_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	BMA4XY_ReadChipInfo(client, strbuf, BMA4XY_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	char strbuf[BMA4XY_BUFSIZE];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA4XY_ReadSensorData(client, strbuf, BMA4XY_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int err, len = 0, mul;
	int tmp[BMA4XY_ACC_AXIS_NUM];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	err = BMA4XY_ReadOffset(client, obj->offset);
	if (0 != err)
		return -EINVAL;

	err = BMA4XY_ReadCalibration(client, tmp);
	if (0 != err)
		return -EINVAL;

	mul = obj->reso->sensitivity / bma4xy_acc_offset_resolution.sensitivity;
	len +=
	snprintf(buf + len, PAGE_SIZE - len,
	"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
	obj->offset[BMA4XY_ACC_AXIS_X],
	obj->offset[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z],
	obj->offset[BMA4XY_ACC_AXIS_X],
	obj->offset[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z]);
	len +=
	snprintf(buf + len, PAGE_SIZE - len,
	"[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
	obj->cali_sw[BMA4XY_ACC_AXIS_X],
	obj->cali_sw[BMA4XY_ACC_AXIS_Y],
	obj->cali_sw[BMA4XY_ACC_AXIS_Z]);

	len +=
	snprintf(buf + len, PAGE_SIZE - len,
	"[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
	obj->offset[BMA4XY_ACC_AXIS_X] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_X],
	obj->offset[BMA4XY_ACC_AXIS_Y] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_Y],
	obj->offset[BMA4XY_ACC_AXIS_Z] * mul + obj->cali_sw[BMA4XY_ACC_AXIS_Z],
	tmp[BMA4XY_ACC_AXIS_X], tmp[BMA4XY_ACC_AXIS_Y], tmp[BMA4XY_ACC_AXIS_Z]);
	return len;
}

static ssize_t store_cali_value(struct device_driver *ddri,
	const char *buf, size_t count)
{
	struct i2c_client *client = bma4xy_i2c_client;
	int err, x, y, z;
	int dat[BMA4XY_ACC_AXIS_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = BMA4XY_ResetCalibration(client);
		if (0 != err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[BMA4XY_ACC_AXIS_X] = x;
		dat[BMA4XY_ACC_AXIS_Y] = y;
		dat[BMA4XY_ACC_AXIS_Z] = z;

		err = BMA4XY_WriteCalibration(client, dat);
		if (0 != err)
			GSE_ERR("write calibration err = %d\n", err);
	} else {
		GSE_ERR("invalid format\n");
	}
	return count;
}

static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "not support\n");
}

static ssize_t store_firlen_value(struct device_driver *ddri,
	const char *buf, size_t count)
{
	return count;
}

static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri,
	const char *buf, size_t count)
{
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);
	int trace;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (!kstrtoint(buf, 16, &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %d\n",
		buf, (int)count);

	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *obj = i2c_get_clientdata(client);

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len,
		"CUST: %d %d (%d %d)\n",
		obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id,
		obj->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}
	return len;
}

static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	unsigned char acc_op_mode;
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	err = bma455_get_accel_enable(&acc_op_mode, &client_data->device);
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	if (sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n",
		sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n",
		sensor_power);
	return snprintf(buf, PAGE_SIZE, "%x\n", acc_op_mode);
}

static ssize_t show_chip_orientation(struct device_driver *ddri, char *pbBuf)
{
	ssize_t _tLength = 0;

	GSE_LOG("[%s] default direction: %d\n", __func__, hw->direction);
	_tLength = snprintf(pbBuf, PAGE_SIZE,
		"default direction = %d\n", hw->direction);
	return _tLength;
}

static ssize_t store_chip_orientation(struct device_driver *ddri,
	const char *pbBuf, size_t tCount)
{
	int _nDirection = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *_pt_i2c_obj = i2c_get_clientdata(client);

	if (NULL == _pt_i2c_obj)
		return 0;

	if (!kstrtoint(pbBuf, 10, &_nDirection)) {
		if (hwmsen_get_convert(_nDirection, &_pt_i2c_obj->cvt))
			GSE_ERR("ERR: fail to set direction\n");
	}

	GSE_LOG("[%s] set direction: %d\n", __func__, _nDirection);

	return tCount;
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO,
	show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO,
	show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO,
	show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO,
	show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO,
	show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value, NULL);
static DRIVER_ATTR(orientation, S_IWUSR | S_IRUGO,
	show_chip_orientation, store_chip_orientation);
static DRIVER_ATTR(chip_id, S_IRUGO, bma4xy_show_chip_id, NULL);
static DRIVER_ATTR(acc_value, S_IRUGO, bma4xy_show_acc_value, NULL);
static DRIVER_ATTR(acc_range, S_IWUSR | S_IRUGO,
	bma4xy_show_acc_range, bma4xy_store_acc_range);
static DRIVER_ATTR(acc_odr, S_IWUSR | S_IRUGO,
	bma4xy_show_acc_odr, bma4xy_store_acc_odr);
static DRIVER_ATTR(selftest, S_IWUSR | S_IRUGO,
	bma4xy_show_selftest, bma4xy_store_selftest);
static DRIVER_ATTR(avail_sensor, S_IRUGO, bma4xy_show_avail_sensor, NULL);
static DRIVER_ATTR(fifo_length, S_IRUGO,
	bma4xy_show_fifo_length, NULL);
static DRIVER_ATTR(acc_fifo_enable, S_IWUSR | S_IRUGO,
	bma4xy_show_fifo_acc_enable, bma4xy_store_fifo_acc_enable);
static DRIVER_ATTR(load_fw, S_IWUSR | S_IRUGO,
	bma4xy_show_load_config_stream, bma4xy_store_load_config_stream);
static DRIVER_ATTR(reg_val, S_IWUSR | S_IRUGO,
	bma4xy_show_reg_val, bma4xy_store_reg_val);
static DRIVER_ATTR(fifo_data_frame, S_IRUGO,
	bma4xy_show_fifo_data_out_frame, NULL);
static DRIVER_ATTR(fifo_flush, S_IWUSR | S_IRUGO,
	NULL, bma4xy_store_fifo_flush);
static DRIVER_ATTR(foc, S_IWUSR | S_IRUGO,
	bma4xy_show_foc, bma4xy_store_foc);
static DRIVER_ATTR(acc_op_mode, S_IWUSR | S_IRUGO,
	bma4xy_show_acc_op_mode, bma4xy_store_acc_op_mode);
static DRIVER_ATTR(fifo_watermark, S_IWUSR | S_IRUGO,
	bma4xy_show_fifo_watermark, bma4xy_store_fifo_watermark);
static DRIVER_ATTR(reg_sel, S_IWUSR | S_IRUGO,
	bma4xy_show_reg_sel, bma4xy_store_reg_sel);
static DRIVER_ATTR(driver_version, S_IRUGO,
	bma4xy_show_driver_version, NULL);
static DRIVER_ATTR(config_file_version, S_IRUGO,
	bma4xy_show_config_file_version, NULL);
static DRIVER_ATTR(config_function, S_IWUSR | S_IRUGO,
	bma4xy_show_config_function, bma4xy_store_config_function);
#if defined(BMA422) || defined(BMA455) || defined(BMA424) || defined(BMA422N) || defined(BMA455N)
static DRIVER_ATTR(sig_config, S_IWUSR | S_IRUGO,
	bma4xy_show_sig_motion_config, bma4xy_store_sig_motion_config);
#endif
#if !defined(BMA420) && !defined(BMA456)
static DRIVER_ATTR(step_counter_val, S_IRUGO,
	bma4xy_show_step_counter_val, NULL);
static DRIVER_ATTR(step_counter_watermark, S_IWUSR | S_IRUGO,
	bma4xy_show_step_counter_watermark,
	bma4xy_store_step_counter_watermark);
static DRIVER_ATTR(step_counter_parameter, S_IWUSR | S_IRUGO,
	bma4xy_show_step_counter_parameter,
	bma4xy_store_step_counter_parameter);
static DRIVER_ATTR(step_counter_reset, S_IWUSR | S_IRUGO,
	NULL, bma4xy_store_step_counter_reset);
#endif
#if defined(BMA422) || defined(BMA455) || defined(BMA424)
static DRIVER_ATTR(tilt_threshold, S_IWUSR | S_IRUGO,
	bma4xy_show_tilt_threshold, bma4xy_store_tilt_threshold);
#endif
#if defined(BMA420) || defined(BMA456)
static DRIVER_ATTR(tap_type, S_IWUSR | S_IRUGO,
	bma4xy_show_tap_type, bma4xy_store_tap_type);
#endif
static DRIVER_ATTR(anymotion_config, S_IWUSR | S_IRUGO,
	bma4xy_show_anymotion_config, bma4xy_store_anymotion_config);
static DRIVER_ATTR(anymotion_config_enable_axis, S_IWUSR | S_IRUGO,
	bma4xy_show_anymotion_enable_axis,
	bma4xy_store_anymotion_enable_axis);
#if defined(BMA422N) || defined(BMA455N)
static DRIVER_ATTR(nomotion_config_enable_axis, S_IWUSR | S_IRUGO,
	bma4xy_show_no_motion_enable_axis, bma4xy_store_no_motion_enable_axis);
static DRIVER_ATTR(nomotion_config, S_IWUSR | S_IRUGO,
	bma4xy_show_nomotion_config,
	bma4xy_store_nomotion_config);
#endif
#if defined(BMA420) || defined(BMA456) || defined(BMA422N) || defined(BMA455N)
static DRIVER_ATTR(orientation_config, S_IWUSR | S_IRUGO,
	bma4xy_show_orientation_config, bma4xy_store_orientation_config);
#endif
#if defined(BMA420) || defined(BMA456)
static DRIVER_ATTR(flat_config, S_IWUSR | S_IRUGO,
	bma4xy_show_flat_config, bma4xy_store_flat_config);
#endif
#if defined(BMA420) || defined(BMA456)
static DRIVER_ATTR(highg_config, S_IWUSR | S_IRUGO,
	bma4xy_show_highg_config, bma4xy_store_highg_config);
#endif
#if defined(BMA420) || defined(BMA456)
static DRIVER_ATTR(lowg_config, S_IWUSR | S_IRUGO,
	bma4xy_show_lowg_config, bma4xy_store_lowg_config);
#endif


/*----------------------------------------------------------------------------*/
static struct driver_attribute *bma4xy_attr_list[] = {
	&driver_attr_chipinfo,
	&driver_attr_sensordata,
	&driver_attr_cali,
	&driver_attr_firlen,
	&driver_attr_trace,
	&driver_attr_status,
	&driver_attr_powerstatus,
	&driver_attr_orientation,
	&driver_attr_chip_id,
	&driver_attr_acc_op_mode,
	&driver_attr_acc_value,
	&driver_attr_acc_range,
	&driver_attr_acc_odr,
	&driver_attr_acc_fifo_enable,
	&driver_attr_fifo_length,
	&driver_attr_selftest,
	&driver_attr_avail_sensor,
	&driver_attr_foc,
	&driver_attr_driver_version,
	&driver_attr_load_fw,
	&driver_attr_fifo_data_frame,
	&driver_attr_fifo_flush,
	&driver_attr_fifo_watermark,
	&driver_attr_reg_sel,
	&driver_attr_reg_val,
	&driver_attr_config_function,
	&driver_attr_config_file_version,
#if defined(BMA422) || defined(BMA455) || defined(BMA424) || defined(BMA422N) || defined(BMA455N)
	&driver_attr_sig_config,
#endif
#if !defined(BMA420) && !defined(BMA456)
	&driver_attr_step_counter_val,
	&driver_attr_step_counter_watermark,
	&driver_attr_step_counter_parameter,
	&driver_attr_step_counter_reset,
#endif
#if defined(BMA422) || defined(BMA455) || defined(BMA424)
	&driver_attr_tilt_threshold,
#endif
	&driver_attr_anymotion_config,
	&driver_attr_anymotion_config_enable_axis,
#if defined(BMA420) || defined(BMA456)
	&driver_attr_tap_type,
#endif
#if defined(BMA422N) || defined(BMA455N)
	&driver_attr_nomotion_config_enable_axis,
	&driver_attr_nomotion_config,
#endif
#if defined(BMA420) || defined(BMA456) || defined(BMA422N) || defined(BMA455N)
	&driver_attr_orientation_config,
#endif
#if defined(BMA420) || defined(BMA456)
	&driver_attr_flat_config,
#endif
#if defined(BMA420) || defined(BMA456)
	&driver_attr_highg_config,
#endif
#if defined(BMA420) || defined(BMA456)
	&driver_attr_lowg_config,
#endif
};

static int bma4xy_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma4xy_attr_list) / sizeof(bma4xy_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;
	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bma4xy_attr_list[idx]);
		if (0 != err) {
			GSE_ERR("driver_create_file (%s) = %d\n",
			bma4xy_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int bma4xy_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma4xy_attr_list) / sizeof(bma4xy_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;
	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bma4xy_attr_list[idx]);
	return err;
}

/*----------------------------------------------------------------------------*/
/* if use  this typ of enable ,
Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int gsensor_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/*----------------------------------------------------------------------------*/
/*if use  this typ of enable , Gsensor only
enabled but not report inputEvent to HAL */
static int gsensor_enable_nodata(int en)
{
	int err = 0;

	if (((en == 0) && (sensor_power == false)) ||
		((en == 1) && (sensor_power == true))) {
		enable_status = sensor_power;
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		enable_status = !sensor_power;
		if (atomic_read(&bma4_obj_i2c_data->suspend) == 0) {
			err = BMA4XY_SetPowerMode(bma4_obj_i2c_data->client,
				enable_status);
			GSE_LOG("Gsensor not in suspend  enable_status = %d\n",
				enable_status);
		} else {
			GSE_LOG("G in suspend can not enable disable!s = %d\n",
				enable_status);
		}
	}
	if (err) {
		GSE_ERR("gsensor_enable_nodata fail!\n");
		return ERRNUM1;
	}
	GSE_LOG("gsensor_enable_nodata OK!\n");
	return 0;
}

static int gsensor_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	ACC_LOG("%s\n", __func__);
	return 0;
}

/*----------------------------------------------------------------------------*/
static int gsensor_flush(void)
{
	return acc_flush_report();
}

/*----------------------------------------------------------------------------*/
static int gsensor_set_delay(u64 ns)
{
	int err = 0;
	int value;
	int sample_delay;

	value = (int)ns / 1000 / 1000;
	if (value <= 5)
		sample_delay = BMA4_OUTPUT_DATA_RATE_200HZ;
	else if (value <= 10)
		sample_delay = BMA4_OUTPUT_DATA_RATE_100HZ;
	else
		sample_delay = BMA4_OUTPUT_DATA_RATE_100HZ;

	err = BMA4XY_SetBWRate(bma4_obj_i2c_data->client, sample_delay);
	if (err) {
		GSE_ERR("Set delay parameter error!\n");
		return ERRNUM1;
	}

	if (value >= 50)
		atomic_set(&bma4_obj_i2c_data->filter, 0);
	GSE_LOG("gsensor_set_delay (%d)\n", value);
	return 0;
}

static int gsensor_acc_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int value = 0;

	value = (int)samplingPeriodNs / 1000 / 1000;

	GSE_LOG("bma acc set delay = (%d) ok.\n", value);
	return gsensor_set_delay(samplingPeriodNs);
}

static int gsensor_get_data(int *x, int *y, int *z, int *status)
{
	char buff[BMA4XY_BUFSIZE];
	int ret;

	mutex_lock(&gsensor_mutex);
	BMA4XY_ReadSensorData(bma4_obj_i2c_data->client, buff, BMA4XY_BUFSIZE);
	mutex_unlock(&gsensor_mutex);
	ret = sscanf(buff, "%x %x %x", x, y, z);
	if (ret != 3) {
		GSE_ERR("Invalid argument");
		return -EINVAL;
	}
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	return 0;
}

static struct acc_init_info bma4xy_init_info = {
	.name = BMA4XY_DEV_NAME,
	.init = gsensor_local_init,
	.uninit = gsensor_remove,
};
static int bma4xy_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err;
	
	err = BMA4XY_SetPowerMode(bma4_obj_i2c_data->client, 1);
	if (err) {
		GSE_ERR("%s enable sensor failed!\n", __func__);
		return -1;
	}
	err = gsensor_acc_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		GSE_ERR("%s enable set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int bma4xy_factory_get_data(int32_t data[3], int *status)
{
	return gsensor_get_data(&data[0], &data[1], &data[2], status);
}

static int bma4xy_factory_get_raw_data(int32_t data[3])
{
	char strbuf[BMA4XY_BUFSIZE] = {0};

	BMA4XY_ReadRawData(bma4_obj_i2c_data->client, strbuf);
	data[0] = strbuf[0];
	data[1] = strbuf[1];
	data[2] = strbuf[2];

	return 0;
}

static int bma4xy_factory_enable_calibration(void)
{
	return 0;
}
static int bma4xy_factory_clear_cali(void)
{
	int err = 0;
	err = BMA4XY_ResetCalibration(bma4_obj_i2c_data->client);
	return 0;
}

static int bma4xy_factory_set_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };
	cali[BMA4XY_ACC_AXIS_X] = data[0]
	    * bma4_obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
	cali[BMA4XY_ACC_AXIS_Y] = data[1]
	    * bma4_obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
	cali[BMA4XY_ACC_AXIS_Z] = data[2]
	    * bma4_obj_i2c_data->reso->sensitivity / GRAVITY_EARTH_1000;
	err = BMA4XY_WriteCalibration(bma4_obj_i2c_data->client, cali);
	if (err) {
		GSE_ERR("bma_WriteCalibration failed!\n");
		return -1;
	}

	return 0;
}

static int bma4xy_factory_get_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };
	err = BMA4XY_ReadCalibration(bma4_obj_i2c_data->client, cali);
	if (err) {
		GSE_ERR("bmi160_ReadCalibration failed!\n");
		return -1;
	}
	data[0] = cali[BMA4XY_ACC_AXIS_X]
	    * GRAVITY_EARTH_1000 / bma4_obj_i2c_data->reso->sensitivity;
	data[1] = cali[BMA4XY_ACC_AXIS_X]
	    * GRAVITY_EARTH_1000 / bma4_obj_i2c_data->reso->sensitivity;
	data[2] = cali[BMA4XY_ACC_AXIS_X]
	    * GRAVITY_EARTH_1000 / bma4_obj_i2c_data->reso->sensitivity;
	return 0;
}

static int bma4xy_factory_do_self_test(void)
{
	return 0;
}

static struct accel_factory_fops bma4xy_factory_fops = {
	.enable_sensor = bma4xy_factory_enable_sensor,
	.get_data = bma4xy_factory_get_data,
	.get_raw_data = bma4xy_factory_get_raw_data,
	.enable_calibration = bma4xy_factory_enable_calibration,
	.clear_cali = bma4xy_factory_clear_cali,
	.set_cali = bma4xy_factory_set_cali,
	.get_cali = bma4xy_factory_get_cali,
	.do_self_test = bma4xy_factory_do_self_test,
};

static struct accel_factory_public bma4xy_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &bma4xy_factory_fops,
};

static int bma4xy_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int err = 0;
	struct i2c_client *new_client;
	struct bma4xy_client_data *client_data = NULL;
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};

//+add by hzb for ontim debug
        if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
           return -EIO;
        }
//-add by hzb for ontim debug
	GSE_FUN();

	err = get_accel_dts_func(client->dev.of_node, hw);
	if (err < 0) {
		GSE_ERR("get cust_baro dts info fail\n");
		goto exit_err_clean;
	}
	GSE_ERR("i2c_num=0x%x,I2C_ADDRS=0x%x;0x%x;\n",hw->i2c_num,hw->i2c_addr[0],client->addr);

	client_data = kzalloc(sizeof(struct bma4xy_client_data), GFP_KERNEL);
	if (NULL == client_data) {
		GSE_ERR("no memory available");
		err = -ENOMEM;
		goto exit_err_clean;
	}
	memset(client_data, 0, sizeof(struct bma4xy_client_data));
	/* h/w init */
	client_data->device.bus_read = bma4xy_i2c_read_wrapper;
	client_data->device.bus_write = bma4xy_i2c_write_wrapper;
	client_data->device.delay = bma4xy_i2c_delay;
	client_data->hw = hw;
	err = hwmsen_get_convert(client_data->hw->direction, &client_data->cvt);
	if (0 != err) {
		GSE_ERR("invalid direction: %d\n", client_data->hw->direction);
		goto exit_err_clean;
	}
	bma4_obj_i2c_data = client_data;
	client_data->client = client;
	new_client = client_data->client;
	i2c_set_clientdata(new_client, client_data);
	atomic_set(&client_data->trace, 0);
	atomic_set(&client_data->suspend, 0);
	bma4xy_i2c_client = new_client;
	#if defined(MTK_NOT_SUPPORT_I2C_RW_MORETHAN_8_BYTES)
	client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	client->dev.dma_mask = &client->dev.coherent_dma_mask;
	I2CDMABuf_va = (u8 *)dma_alloc_coherent(
	&client->dev, 256, &I2CDMABuf_pa, GFP_KERNEL | GFP_DMA);
	if (!I2CDMABuf_va) {
		GSE_ERR("Allocate Touch DMA I2C Buffer failed!\n");
		goto exit_err_clean;
	}
	#endif
	/* check chip id */
	err = bma4xy_check_chip_id(client_data);
	if (!err) {
		GSE_LOG("Bosch Sensortec Device %s detected", SENSOR_NAME);
	} else {
		GSE_ERR("Bosch Sensortec Device not found, chip id mismatch");
		goto exit_err_clean;
	}
#if defined(BMA420)
	err = bma420_init(&client_data->device);
#elif defined(BMA421)
	err = bma421_init(&client_data->device);
#elif defined(BMA424SC)
	err = bma424sc_init(&client_data->device);
#elif defined(BMA421L)
	err = bma421l_init(&client_data->device);
#elif defined(BMA422)
	err = bma422_init(&client_data->device);
#elif defined(BMA422N)
	err = bma422n_init(&client_data->device);
	client_data->any_motion_axis = 7;
	client_data->no_motion_axis = 7;
#elif defined(BMA455N)
	err = bma455n_init(&client_data->device);
	client_data->any_motion_axis = 7;
	client_data->no_motion_axis = 7;
#elif defined(BMA424)
	err = bma424_init(&client_data->device);
#elif defined(BMA455)
	err = bma455_init(&client_data->device);
#endif
	if (err)
		GSE_ERR("init failed\n");
	err = bma455_set_command_register(0xB6, &client_data->device);
	if (!err)
		GSE_ERR("reset chip");
	bma4xy_i2c_delay(10);

#if defined(BMA4XY_ENABLE_INT1) || defined(BMA4XY_ENABLE_INT2)
	err = bma4xy_request_irq(client_data);
	if (err < 0)
		GSE_ERR("Request irq failed");
#endif
	wake_lock_init(&client_data->wakelock, WAKE_LOCK_SUSPEND, "bma4xy");
	err = bma4xy_init_client(new_client, 1);
	if (err)
		GSE_ERR("bma4xy_device init cilent fail time\n");
	err = accel_factory_device_register(&bma4xy_factory_device);
	if (err) {
		GSE_ERR("bma4xy_device register failed\n");
		goto exit_err_clean;
	}
	err = bma4xy_create_attr(&bma4xy_init_info.platform_diver_addr->driver);
	if (err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_err_clean;
	}

	ctl.open_report_data= gsensor_open_report_data;
	ctl.enable_nodata = gsensor_enable_nodata;
	ctl.batch = gsensor_batch;
	ctl.flush = gsensor_flush;
	ctl.set_delay  = gsensor_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = false;
	
	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_err_clean;
	}
	data.get_data = gsensor_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err\n");
		goto exit_err_clean;
	}

	gsensor_init_flag = 0;
//+add by hzb for ontim debug
        REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//-add by hzb for ontim debug
	GSE_LOG("%s: OK\n", __func__);
	return 0;
exit_err_clean:
	if (err) {
		bma4xy_i2c_client = NULL;
		if (client_data != NULL)
			kfree(client_data);
		bma4_obj_i2c_data = NULL;
		return err;
	}
	return err;
}

static int bma4xy_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = bma4xy_delete_attr(&bma4xy_init_info.platform_diver_addr->driver);
	if (err != 0)
		GSE_ERR("bma150_delete_attr fail: %d\n", err);

	accel_factory_device_deregister(&bma4xy_factory_device);
	if (0 != err)
		GSE_ERR("acc_deregister fail: %d\n", err);
	bma4xy_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}
#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor_bma4xy",},
	{ }
};
MODULE_DEVICE_TABLE(i2c, accel_of_match);
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops bma4xy_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(bma4xy_i2c_suspend, bma4xy_i2c_resume)
};
#endif
static struct i2c_driver bma4xy_i2c_driver = {
	.driver = {
		.name = SENSOR_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &bma4xy_pm_ops,
#endif
		.of_match_table = accel_of_match,
	},
	.probe = bma4xy_i2c_probe,
	.remove = bma4xy_i2c_remove,
	.id_table = bma4xy_i2c_id,
};

static int gsensor_local_init(void)
{
	GSE_FUN();
	BMA4XY_power(hw, 1);
	if (i2c_add_driver(&bma4xy_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return ERRNUM1;
	}
	if (-1 == gsensor_init_flag)
		return ERRNUM1;
	return 0;
}

static int gsensor_remove(void)
{
	GSE_FUN();
	BMA4XY_power(hw, 0);
	i2c_del_driver(&bma4xy_i2c_driver);
	return 0;
}
#if defined(BMA4_STEP_COUNTER)
static int bma4xy_step_c_open_report_data(int open)
{
	return 0;
}

static int bma4xy_step_c_set_delay(u64 delay)
{
	return 0;
}

static int bma4xy_setp_d_set_selay(u64 delay)
{
	return 0;
}

static int bma4xy_step_c_enable_nodata(int en)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("bma4xy_step_c_enable_nodata en = %d", en);
	#if defined(BMA421)
	if (bma421_feature_enable(
		BMA421_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424SC)
	if (bma424sc_feature_enable(
		BMA424SC_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma424sc virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA421L)
	if (bma421l_feature_enable(
		BMA421L_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma421L virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422)
	if (bma422_feature_enable(
		BMA422_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424)
	if (bma424_feature_enable(
		BMA424_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455)
	if (bma455_feature_enable(
		BMA455_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422N)
	if (bma422n_feature_enable(
		BMA422N_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455N)
	if (bma455n_feature_enable(
		BMA455N_STEP_CNTR, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	if (en == 1)
		bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	if ((en == 0) && (bma4_obj_i2c_data->sigmotion_enable == 0) &&
		(bma4_obj_i2c_data->stepdet_enable == 0) &&
		(bma4_obj_i2c_data->acc_pm == 0))
		err = bma455_set_accel_enable(BMA4_DISABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	bma4_obj_i2c_data->stepcounter_enable = en;
	return err;
}

static int bma4xy_step_c_enable_significant(int en)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("bma4xy_step_c_enable_significant en = %d", en);
	#if defined(BMA422)
	if (bma422_feature_enable(
		BMA422_SIG_MOTION, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424)
	if (bma424_feature_enable(
		BMA424_SIG_MOTION, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455)
	if (bma455_feature_enable(
		BMA455_SIG_MOTION, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422N)
	if (bma422N_feature_enable(
		BMA422N_SIG_MOTION, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455N)
	if (bma455n_feature_enable(
		BMA455N_SIG_MOTION, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	if (en == 1)
		bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	if ((en == 0) && (bma4_obj_i2c_data->stepcounter_enable == 0) &&
		(bma4_obj_i2c_data->stepdet_enable == 0) &&
		(bma4_obj_i2c_data->acc_pm == 0))
		err = bma455_set_accel_enable(BMA4_DISABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	bma4_obj_i2c_data->sigmotion_enable = en;
	return err;

}

static int bma4xy_step_c_enable_step_detect(int enable)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("bma4xy_step_c_enable_step_detect en = %d", enable);
	#if defined(BMA421)
	if (bma421_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424SC)
	if (bma424sc_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma421 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA421L)
	if (bma421l_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma421L virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422)
	if (bma422_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424)
	if (bma424_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455)
	if (bma455_step_detector_enable(enable, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA422N)
	if (bma422n_feature_enable(BMA422N_STEP_DETR, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422N virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455N)
	if (bma455n_feature_enable(BMA455N_STEP_DETR, enable, &client_data->device) < 0) {
		GSE_ERR("set bma422N virtual error");
		return -EINVAL;
	}
	#endif
	if (enable == 1)
		bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	if ((enable == 0) && (bma4_obj_i2c_data->sigmotion_enable == 0) &&
		(bma4_obj_i2c_data->stepcounter_enable == 0) &&
		(bma4_obj_i2c_data->acc_pm == 0))
		err = bma455_set_accel_enable(BMA4_DISABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	bma4_obj_i2c_data->stepdet_enable = enable;
	return err;
}

static int bma4xy_step_c_get_data(uint32_t *value, int *status)
{
	int err = 0;
	uint32_t step_counter_val = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	#if defined(BMA421)
	err = bma421_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA424SC)
	err = bma424sc_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA421L)
	err = bma421l_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA422)
	err = bma422_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA424)
	err = bma424_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA455)
	err = bma455_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA422N)
	err = bma422_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	#if defined(BMA455N)
	err = bma455n_step_counter_output(
	&step_counter_val, &client_data->device);
	#endif
	if (err) {
		GSE_ERR("read failed");
		return err;
	}
	*value = step_counter_val;
	*status = 1;
	GSE_LOG("step_c_get_data = %d\n", (int)(*value));
	return err;
}

static int bma4xy_stc_get_data_significant(uint32_t *value, int *status)
{
	return 0;
}

static int bma4xy_stc_get_data_step_d(uint32_t *value, int *status)
{
	return 0;
}

static int bma455_step_c_flush(void)
{
	int err = 0;
	
	err = step_c_flush_report();
	pr_err("add for modify:flush complete \n");
	return err;
}

static int bma4xy_step_c_probe(void)
{
	struct step_c_control_path ctl = {0};
	struct step_c_data_path data = {0};
	int err = 0;

	ctl.open_report_data = bma4xy_step_c_open_report_data;
	ctl.enable_nodata = bma4xy_step_c_enable_nodata;
	ctl.enable_step_detect = bma4xy_step_c_enable_step_detect;
	ctl.enable_significant = bma4xy_step_c_enable_significant;
	ctl.step_c_set_delay = bma4xy_step_c_set_delay;
	ctl.step_d_set_delay = bma4xy_setp_d_set_selay;
	ctl.step_c_flush = bma455_step_c_flush;
	ctl.is_report_input_direct = false;
	ctl.is_counter_support_batch = false;
	err =  step_c_register_control_path(&ctl);
	if (err) {
		GSE_ERR("step_c_register_control_path fail = %d\n", err);
		goto exit;
	}
	data.get_data = bma4xy_step_c_get_data;
	data.vender_div = 1000;
	data.get_data_significant = bma4xy_stc_get_data_significant;
	data.get_data_step_d = bma4xy_stc_get_data_step_d;
	err = step_c_register_data_path(&data);
	if (err) {
		GSE_ERR("step_c_register_data_path fail = %d\n", err);
		goto exit;
	}
	GSE_LOG("%s: OK\n", __func__);
	return 0;
exit:
	GSE_ERR("err = %d\n", err);
	return err;
}

static int bma4xy_stc_remove(void)
{
	GSE_FUN();
	return 0;
}

static int bma4xy_stc_local_init(void)
{
	GSE_FUN();
	if (bma4_obj_i2c_data == NULL) {
		GSE_ERR("bma4xy_acc_init_error\n");
		return -ENODEV;
	}
	if (bma4xy_step_c_probe()) {
		GSE_ERR("failed to register bma4xy step_c driver\n");
		return -ENODEV;
	}
	return 0;
}

static struct step_c_init_info bma4xy_stc_init_info = {
	.name = "bma4xy_step_counter",
	.init = bma4xy_stc_local_init,
	.uninit = bma4xy_stc_remove,
};
#endif

#if defined(BMA4_WAKEUP)
static int bma4xy_wakeup_enable(int en)
{
	int err = 0;
	struct i2c_client *client = bma4xy_i2c_client;
	struct bma4xy_client_data *client_data = i2c_get_clientdata(client);

	GSE_LOG("bma4xy_wakeup_enable enable = %d\n", en);
	#if defined(BMA422)
	if (bma422_feature_enable(
		BMA422_WAKEUP, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA424)
	if (bma424_feature_enable(
		BMA424_WAKEUP, en, &client_data->device) < 0) {
		GSE_ERR("set bma422 virtual error");
		return -EINVAL;
	}
	#endif
	#if defined(BMA455)
	if (bma455_feature_enable(
		BMA455_WAKEUP, en, &client_data->device) < 0) {
		GSE_ERR("set bma455 virtual error");
		return -EINVAL;
	}
	#endif
	if (en == 1)
		bma455_set_accel_enable(BMA4_ENABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	if ((en == 0) && (bma4_obj_i2c_data->sigmotion_enable == 0) &&
		(bma4_obj_i2c_data->stepdet_enable == 0) &&
		(bma4_obj_i2c_data->stepcounter_enable == 0) &&
		(bma4_obj_i2c_data->acc_pm == 0))
		err = bma455_set_accel_enable(BMA4_DISABLE, &client_data->device);
	if (err)
		GSE_ERR("set acc_op_mode failed");
	bma4_obj_i2c_data->wakeup_enable = en;
	return err;
}

static int bma4xy_get_data_wakeup(u16 *value, int *status)
{
	return 0;
}
static int bma4xy_wakeup_probe(void)
{
	struct wag_control_path ctl = {0};
	struct wag_data_path data = {0};
	int err = 0;

	ctl.open_report_data = bma4xy_wakeup_enable;
	err = wag_register_control_path(&ctl);
	if (err) {
		GSE_ERR("wag_register_control_path fail = %d\n", err);
		goto exit;
	}
	data.get_data = bma4xy_get_data_wakeup;
	err = wag_register_data_path(&data);
	if (err) {
		GSE_ERR("wag_register_data_path fail = %d\n", err);
		goto exit;
	}
	GSE_LOG("%s: OK\n", __func__);
	return 0;
exit:
	GSE_LOG("err = %d\n", err);
	return err;
}

static int bma4xy_wakeup_remove(void)
{
	GSE_FUN();
	return 0;
}

static int bma4xy_wakeup_local_init(void)
{
	GSE_FUN();
	if (bma4_obj_i2c_data == NULL) {
		GSE_ERR("bma4xy_acc_init_error\n");
		return -ENODEV;
	}
	if (bma4xy_wakeup_probe()) {
		GSE_ERR("failed to register bma4xy step_c driver\n");
		return -ENODEV;
	}
	return 0;
}

static struct wag_init_info bma4xy_wakeup_init_info = {
	.name = "bma4xy_wakeup",
	.init = bma4xy_wakeup_local_init,
	.uninit = bma4xy_wakeup_remove,
};
#endif

static int __init BMA4xy_init(void)
{

	GSE_FUN();
	acc_driver_add(&bma4xy_init_info);
	#if defined(BMA4_STEP_COUNTER)
	step_c_driver_add(&bma4xy_stc_init_info);
	#endif
	#if defined(BMA4_WAKEUP)
	wag_driver_add(&bma4xy_wakeup_init_info);
	#endif
	return 0;
}

static void __exit BMA4xy_exit(void)
{
	GSE_FUN();
}
module_init(BMA4xy_init);
module_exit(BMA4xy_exit);
MODULE_AUTHOR("contact@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA4XY SENSOR DRIVER");
MODULE_LICENSE("GPL v2");


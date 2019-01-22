/* drivers/i2c/chips/lis3dh.c - LIS3DH motion sensor driver
 *
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


#include <cust_acc.h>
#include "lis3dh.h"
#include <accel.h>
#include <hwmsensor.h>

/* #define POWER_NONE_MACRO MT65XX_POWER_NONE */

#define COMPATIABLE_NAME "mediatek,lis3dh"

/* Maintain  cust info here */
struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;

/* For  driver get cust info */
struct acc_hw *get_cust_acc(void)
{
	return &accel_cust;
}



/*----------------------------------------------------------------------------*/
/* #define I2C_DRIVERID_LIS3DH 345 */
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_LIS3DH_LOWPASS   /*apply low pass filter on output*/
/*----------------------------------------------------------------------------*/
#define LIS3DH_AXIS_X          0
#define LIS3DH_AXIS_Y          1
#define LIS3DH_AXIS_Z          2
#define LIS3DH_AXES_NUM        3
#define LIS3DH_DATA_LEN        6
#define LIS3DH_DEV_NAME        "LIS3DH"
#define C_I2C_FIFO_SIZE	 8
/*----------------------------------------------------------------------------*/
static const struct i2c_device_id lis3dh_i2c_id[] = {{LIS3DH_DEV_NAME, 0}, {} };
/*the adapter id will be available in customization*/
/* static struct i2c_board_info __initdata i2c_LIS3DH={ I2C_BOARD_INFO("LIS3DH", (0x32>>1))}; */

/* static unsigned short lis3dh_force[] = {0x00, LIS3DH_I2C_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END}; */
/* static const unsigned short *const lis3dh_forces[] = { lis3dh_force, NULL }; */
/* static struct i2c_client_address_data lis3dh_addr_data = { .forces = lis3dh_forces,}; */

/*----------------------------------------------------------------------------*/
static int lis3dh_probe(void);
static int lis3dh_remove(void);
static int lis3dh_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lis3dh_i2c_remove(struct i2c_client *client);
/* static int lis3dh_i2c_detect(struct i2c_client *client, struct i2c_board_info *info); */
#ifndef USE_EARLY_SUSPEND
static int lis3dh_suspend(struct device *dev);
static int lis3dh_resume(struct device *dev);
#endif
/*----------------------------------------------------------------------------*/
enum {
	ADX_TRC_FILTER  = 0x01,
	ADX_TRC_RAWDATA = 0x02,
	ADX_TRC_IOCTL   = 0x04,
	ADX_TRC_CALI	= 0X08,
	ADX_TRC_INFO	= 0X10,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor {
	u8  whole;
	u8  fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
	struct scale_factor scalefactor;
	int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][LIS3DH_AXES_NUM];
	int sum[LIS3DH_AXES_NUM];
	int num;
	int idx;
};
/*----------------------------------------------------------------------------*/
struct lis3dh_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert   cvt;

	/*misc*/
	struct data_resolution *reso;
	atomic_t                trace;
	atomic_t                suspend;
	atomic_t                selftest;
	atomic_t				filter;
	s16                     cali_sw[LIS3DH_AXES_NUM + 1];

	/*data*/
	s8                      offset[LIS3DH_AXES_NUM + 1]; /*+1: for 4-byte alignment*/
	s16                     data[LIS3DH_AXES_NUM + 1];

#if defined(CONFIG_LIS3DH_LOWPASS)
	atomic_t                firlen;
	atomic_t                fir_en;
	struct data_filter      fir;
#endif
	/*early suspend*/
#ifdef USE_EARLY_SUSPEND
	struct early_suspend    early_drv;
#endif
};
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id accel_of_match[] = {
	{.compatible = "mediatek,gsensor"},
	{},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops lis3dh_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(lis3dh_suspend, lis3dh_resume)
};
#endif

static struct i2c_driver lis3dh_i2c_driver = {
	.driver = {
		/* .owner          = THIS_MODULE, */
		.name           = LIS3DH_DEV_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &lis3dh_pm_ops,
#endif

#ifdef CONFIG_OF
		.of_match_table = accel_of_match,
#endif
	},
	.probe		= lis3dh_i2c_probe,
	.remove			= lis3dh_i2c_remove,
	/* .detect				= lis3dh_i2c_detect, */
	.id_table = lis3dh_i2c_id,
	/* .address_data = &lis3dh_addr_data, */
};


static struct acc_init_info lis3dh_gsensor_driver = {
	.name = LIS3DH_DEV_NAME,
	.init = lis3dh_probe,
	.uninit = lis3dh_remove,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *lis3dh_i2c_client;
/* static struct platform_driver lis3dh_gsensor_driver; */
static struct lis3dh_i2c_data *obj_i2c_data;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain, gsensor_offset;
/* static char selftestRes[10] = {0}; */
static DEFINE_MUTEX(lis3dh_i2c_mutex);
static DEFINE_MUTEX(lis3dh_op_mutex);
static bool enable_status;
static int sensor_suspend;

/*----------------------------------------------------------------------------*/
#define GSE_TAG                  "[Gsensor] "
#define GSE_FUN(f)               pr_info(GSE_TAG"%s\n", __func__)
#define GSE_ERR(fmt, args...)    pr_notice(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)    pr_debug(GSE_TAG fmt, ##args)
/*----------------------------------------------------------------------------*/
/*dataformat +/-2g in 12-bit resolution; { 1, 0} = 1.0 = (2*2*1000)/(2^12); 1024 = (2^12)/(2*2)*/
/*dataformat +/-4g in 12-bit resolution; { 1, 9} = 1.9 = (2*4*1000)/(2^12); 512 = (2^12)/(2*4)*/
/*dataformat +/-8g in 12-bit resolution; { 1, 0} = 1.0 = (2*8*1000)/(2^12); 1024 = (2^12)/(2*8)*/
static struct data_resolution lis3dh_data_resolution[] = {
	/* combination by {FULL_RES,RANGE}*/
	{{ 1, 0}, 1024},
	{{ 1, 9}, 512},
	{{ 3, 9}, 256},
};
/*----------------------------------------------------------------------------*/
static struct data_resolution lis3dh_offset_resolution = {{15, 6}, 64};

/*--------------------read function----------------------------------*/
static int lis_i2c_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	u8 beg = addr;
	int err;
	struct i2c_msg msgs[2] = {{0}, {0} };

	mutex_lock(&lis3dh_i2c_mutex);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &beg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	if (!client) {
		mutex_unlock(&lis3dh_i2c_mutex);
		return -EINVAL;
	} else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&lis3dh_i2c_mutex);
		return -EINVAL;
	}
	err = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (err != 2) {
		GSE_ERR("i2c_transfer error: (%d %p %d) %d\n", addr, data, len, err);
		err = -EIO;
	} else
		err = 0;
	mutex_unlock(&lis3dh_i2c_mutex);
	return err;

}

static int lis_i2c_write_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
	/*because address also occupies one byte, the maximum length for write is 7 bytes*/
	int err, idx, num;
	char buf[C_I2C_FIFO_SIZE];

	err = 0;
	mutex_lock(&lis3dh_i2c_mutex);
	GSE_LOG(" ====brucelee=== C_I2C_FIFO_SIZE %d\n", C_I2C_FIFO_SIZE);
	if (!client) {
		mutex_unlock(&lis3dh_i2c_mutex);
		return -EINVAL;
	} else if (len >= C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
		mutex_unlock(&lis3dh_i2c_mutex);
		return -EINVAL;
	}

	num = 0;
	buf[num++] = addr;
	for (idx = 0; idx < len; idx++)
		buf[num++] = data[idx];

	err = i2c_master_send(client, buf, num);
	GSE_LOG("smith-acc return value is %d\n", err);
	if (err < 0) {
		GSE_ERR("smith-acc send command error\n");
		GSE_ERR("send command error!!\n");
		mutex_unlock(&lis3dh_i2c_mutex);
		return -EFAULT;
	}
	mutex_unlock(&lis3dh_i2c_mutex);
	return err;
}
/*----------------------------------------------------------------------------*/

static void dumpReg(struct i2c_client *client)
{
	int i = 0;
	u8 addr = 0x20;
	u8 regdata = 0;

	for (i = 0; i < 3 ; i++) {
		/* dump all */
		lis_i2c_read_block(client, addr, &regdata, 1);
		GSE_LOG("Reg addr=%x regdata=%x\n", addr, regdata);
		addr++;
	}
}
/*--------------------ADXL power control function----------------------------------*/
static void LIS3DH_power(struct acc_hw *hw, unsigned int on)
{
	/*
	*static unsigned int power_on = 0;
  *
	*if(hw->power_id != POWER_NONE_MACRO)		// have externel LDO
	*{
	*	GSE_LOG("power %s\n", on ? "on" : "off");
	*	if(power_on == on)	// power status not change
	*	{
	*		GSE_LOG("ignore power control: %d\n", on);
	*	}
	*	else if(on)	// power on
	*	{
	*		if(!hwPowerOn(hw->power_id, hw->power_vol, "LIS3DH"))
	*		{
	*			GSE_ERR("power on fails!!\n");
	*		}
	*	}
	*	else	// power off
	*	{
	*		if (!hwPowerDown(hw->power_id, "LIS3DH"))
	*		{
	*			GSE_ERR("power off fail!!\n");
	*		}
	*	}
	*}
	*power_on = on;
	*/
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_SetDataResolution(struct lis3dh_i2c_data *obj)
{
	int err;
	u8  dat, reso;

	err = lis_i2c_read_block(obj->client, LIS3DH_REG_CTL_REG4, &dat, 0x01);
	if (err) {
		GSE_ERR("write data format fail!!\n");
		return err;
	}

	/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
	reso  = (dat & 0x30) << 4;
	if (reso >= 0x3)
		reso = 0x2;


	if (reso < ARRAY_SIZE(lis3dh_data_resolution)) {
		obj->reso = &lis3dh_data_resolution[reso];
		return 0;
	} else
		return -EINVAL;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_ReadData(struct i2c_client *client, s16 data[LIS3DH_AXES_NUM])
{
	struct lis3dh_i2c_data *priv = i2c_get_clientdata(client);
	/* u8 addr = LIS3DH_REG_DATAX0; */
	u8 buf[LIS3DH_DATA_LEN] = {0};
	int err = 0;

	if (client == NULL)
		err = -EINVAL;

	else {
		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_X, buf, 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}
		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_X + 1, &buf[1], 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}

		data[LIS3DH_AXIS_X] = (s16)((buf[0] + (buf[1] << 8)) >> 4);
		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_Y, &buf[2], 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}
		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_Y + 1, &buf[3], 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}

		data[LIS3DH_AXIS_Y] =  (s16)((s16)(buf[2] + (buf[3] << 8)) >> 4);

		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_Z, &buf[4], 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}

		if (lis_i2c_read_block(client, LIS3DH_REG_OUT_Z + 1, &buf[5], 0x01)) {
			GSE_ERR("read  G sensor data register err!\n");
			return -1;
		}

		data[LIS3DH_AXIS_Z] = (s16)((buf[4] + (buf[5] << 8)) >> 4);

		/* GSE_LOG("[%08X %08X %08X %08x %08x %08x]\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]); */


		data[LIS3DH_AXIS_X] &= 0xfff;
		data[LIS3DH_AXIS_Y] &= 0xfff;
		data[LIS3DH_AXIS_Z] &= 0xfff;


		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n",
				data[LIS3DH_AXIS_X], data[LIS3DH_AXIS_Y], data[LIS3DH_AXIS_Z],
				data[LIS3DH_AXIS_X], data[LIS3DH_AXIS_Y], data[LIS3DH_AXIS_Z]);
		}

		if (data[LIS3DH_AXIS_X] & 0x800) {
			data[LIS3DH_AXIS_X] = ~data[LIS3DH_AXIS_X];
			data[LIS3DH_AXIS_X] &= 0xfff;
			data[LIS3DH_AXIS_X] += 1;
			data[LIS3DH_AXIS_X] = -data[LIS3DH_AXIS_X];
		}
		if (data[LIS3DH_AXIS_Y] & 0x800) {
			data[LIS3DH_AXIS_Y] = ~data[LIS3DH_AXIS_Y];
			data[LIS3DH_AXIS_Y] &= 0xfff;
			data[LIS3DH_AXIS_Y] += 1;
			data[LIS3DH_AXIS_Y] = -data[LIS3DH_AXIS_Y];
		}
		if (data[LIS3DH_AXIS_Z] & 0x800) {
			data[LIS3DH_AXIS_Z] = ~data[LIS3DH_AXIS_Z];
			data[LIS3DH_AXIS_Z] &= 0xfff;
			data[LIS3DH_AXIS_Z] += 1;
			data[LIS3DH_AXIS_Z] = -data[LIS3DH_AXIS_Z];
		}

		if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
			GSE_LOG("[%08X %08X %08X] => [%5d %5d %5d] after\n",
				data[LIS3DH_AXIS_X], data[LIS3DH_AXIS_Y], data[LIS3DH_AXIS_Z],
				data[LIS3DH_AXIS_X], data[LIS3DH_AXIS_Y], data[LIS3DH_AXIS_Z]);
		}

#ifdef CONFIG_LIS3DH_LOWPASS
		if (atomic_read(&priv->filter)) {
			if (atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend)) {
				int idx, firlen = atomic_read(&priv->firlen);

				if (priv->fir.num < firlen) {
					priv->fir.raw[priv->fir.num][LIS3DH_AXIS_X] = data[LIS3DH_AXIS_X];
					priv->fir.raw[priv->fir.num][LIS3DH_AXIS_Y] = data[LIS3DH_AXIS_Y];
					priv->fir.raw[priv->fir.num][LIS3DH_AXIS_Z] = data[LIS3DH_AXIS_Z];
					priv->fir.sum[LIS3DH_AXIS_X] += data[LIS3DH_AXIS_X];
					priv->fir.sum[LIS3DH_AXIS_Y] += data[LIS3DH_AXIS_Y];
					priv->fir.sum[LIS3DH_AXIS_Z] += data[LIS3DH_AXIS_Z];
					if (atomic_read(&priv->trace) & ADX_TRC_FILTER) {
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
							priv->fir.raw[priv->fir.num][LIS3DH_AXIS_X],
							priv->fir.raw[priv->fir.num][LIS3DH_AXIS_Y],
							priv->fir.raw[priv->fir.num][LIS3DH_AXIS_Z],
							priv->fir.sum[LIS3DH_AXIS_X],
							priv->fir.sum[LIS3DH_AXIS_Y],
							priv->fir.sum[LIS3DH_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				} else {
					idx = priv->fir.idx % firlen;
					priv->fir.sum[LIS3DH_AXIS_X] -= priv->fir.raw[idx][LIS3DH_AXIS_X];
					priv->fir.sum[LIS3DH_AXIS_Y] -= priv->fir.raw[idx][LIS3DH_AXIS_Y];
					priv->fir.sum[LIS3DH_AXIS_Z] -= priv->fir.raw[idx][LIS3DH_AXIS_Z];
					priv->fir.raw[idx][LIS3DH_AXIS_X] = data[LIS3DH_AXIS_X];
					priv->fir.raw[idx][LIS3DH_AXIS_Y] = data[LIS3DH_AXIS_Y];
					priv->fir.raw[idx][LIS3DH_AXIS_Z] = data[LIS3DH_AXIS_Z];
					priv->fir.sum[LIS3DH_AXIS_X] += data[LIS3DH_AXIS_X];
					priv->fir.sum[LIS3DH_AXIS_Y] += data[LIS3DH_AXIS_Y];
					priv->fir.sum[LIS3DH_AXIS_Z] += data[LIS3DH_AXIS_Z];
					priv->fir.idx++;
					data[LIS3DH_AXIS_X] = priv->fir.sum[LIS3DH_AXIS_X] / firlen;
					data[LIS3DH_AXIS_Y] = priv->fir.sum[LIS3DH_AXIS_Y] / firlen;
					data[LIS3DH_AXIS_Z] = priv->fir.sum[LIS3DH_AXIS_Z] / firlen;
					if (atomic_read(&priv->trace) & ADX_TRC_FILTER) {
						GSE_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n",
							idx,
							priv->fir.raw[idx][LIS3DH_AXIS_X],
							priv->fir.raw[idx][LIS3DH_AXIS_Y],
							priv->fir.raw[idx][LIS3DH_AXIS_Z],
							priv->fir.sum[LIS3DH_AXIS_X],
							priv->fir.sum[LIS3DH_AXIS_Y],
							priv->fir.sum[LIS3DH_AXIS_Z],
							data[LIS3DH_AXIS_X], data[LIS3DH_AXIS_Y], data[LIS3DH_AXIS_Z]);
					}
				}
			}
		}
#endif
	}
	return err;
}
/*----------------------------------------------------------------------------*/
/*
*static int LIS3DH_ReadOffset(struct i2c_client *client, s8 ofs[LIS3DH_AXES_NUM])
*{
*	int err;
*
*	return err;
*}
*/
/*----------------------------------------------------------------------------*/
static int LIS3DH_ResetCalibration(struct i2c_client *client)
{
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);

	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_ReadCalibration(struct i2c_client *client, int dat[LIS3DH_AXES_NUM])
{
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);

	dat[obj->cvt.map[LIS3DH_AXIS_X]] = obj->cvt.sign[LIS3DH_AXIS_X] * obj->cali_sw[LIS3DH_AXIS_X];
	dat[obj->cvt.map[LIS3DH_AXIS_Y]] = obj->cvt.sign[LIS3DH_AXIS_Y] * obj->cali_sw[LIS3DH_AXIS_Y];
	dat[obj->cvt.map[LIS3DH_AXIS_Z]] = obj->cvt.sign[LIS3DH_AXIS_Z] * obj->cali_sw[LIS3DH_AXIS_Z];

	return 0;
}
/*----------------------------------------------------------------------------*/
/*
*static int LIS3DH_ReadCalibrationEx(struct i2c_client *client, int act[LIS3DH_AXES_NUM], int raw[LIS3DH_AXES_NUM])
*{
*
*	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);
*	int err;
*	int mul;
*
*	if(err = LIS3DH_ReadOffset(client, obj->offset))
*	{
*		GSE_ERR("read offset fail, %d\n", err);
*		return err;
*	}
*
*	mul = obj->reso->sensitivity/lis3dh_offset_resolution.sensitivity;
*	raw[LIS3DH_AXIS_X] = obj->offset[LIS3DH_AXIS_X]*mul + obj->cali_sw[LIS3DH_AXIS_X];
*	raw[LIS3DH_AXIS_Y] = obj->offset[LIS3DH_AXIS_Y]*mul + obj->cali_sw[LIS3DH_AXIS_Y];
*	raw[LIS3DH_AXIS_Z] = obj->offset[LIS3DH_AXIS_Z]*mul + obj->cali_sw[LIS3DH_AXIS_Z];
*
*	act[obj->cvt.map[LIS3DH_AXIS_X]] = obj->cvt.sign[LIS3DH_AXIS_X]*raw[LIS3DH_AXIS_X];
*	act[obj->cvt.map[LIS3DH_AXIS_Y]] = obj->cvt.sign[LIS3DH_AXIS_Y]*raw[LIS3DH_AXIS_Y];
*	act[obj->cvt.map[LIS3DH_AXIS_Z]] = obj->cvt.sign[LIS3DH_AXIS_Z]*raw[LIS3DH_AXIS_Z];
*
*	return 0;
*}
*/
/*----------------------------------------------------------------------------*/
static int LIS3DH_WriteCalibration(struct i2c_client *client, int dat[LIS3DH_AXES_NUM])
{
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	s16 cali[LIS3DH_AXES_NUM];
	/* int cali[LIS3DH_AXES_NUM]; */


	GSE_FUN();
	if (!obj || !dat) {
		GSE_ERR("null ptr!!\n");
		return -EINVAL;
	}

	cali[obj->cvt.map[LIS3DH_AXIS_X]] = obj->cvt.sign[LIS3DH_AXIS_X] * obj->cali_sw[LIS3DH_AXIS_X];
	cali[obj->cvt.map[LIS3DH_AXIS_Y]] = obj->cvt.sign[LIS3DH_AXIS_Y] * obj->cali_sw[LIS3DH_AXIS_Y];
	cali[obj->cvt.map[LIS3DH_AXIS_Z]] = obj->cvt.sign[LIS3DH_AXIS_Z] * obj->cali_sw[LIS3DH_AXIS_Z];
	cali[LIS3DH_AXIS_X] += dat[LIS3DH_AXIS_X];
	cali[LIS3DH_AXIS_Y] += dat[LIS3DH_AXIS_Y];
	cali[LIS3DH_AXIS_Z] += dat[LIS3DH_AXIS_Z];

	obj->cali_sw[LIS3DH_AXIS_X] += obj->cvt.sign[LIS3DH_AXIS_X] * dat[obj->cvt.map[LIS3DH_AXIS_X]];
	obj->cali_sw[LIS3DH_AXIS_Y] += obj->cvt.sign[LIS3DH_AXIS_Y] * dat[obj->cvt.map[LIS3DH_AXIS_Y]];
	obj->cali_sw[LIS3DH_AXIS_Z] += obj->cvt.sign[LIS3DH_AXIS_Z] * dat[obj->cvt.map[LIS3DH_AXIS_Z]];

	return err;
}
/*----------------------------------------------------------------------------*/
/*
*static int LIS3DH_CheckDeviceID(struct i2c_client *client)
*{
*	u8 databuf[10];
*	int res = 0;
*
*	memset(databuf, 0, sizeof(u8)*10);
*	databuf[0] = LIS3DH_REG_DEVID;
*
*	res = i2c_master_send(client, databuf, 0x1);
*	if(res <= 0)
*	{
*		goto exit_LIS3DH_CheckDeviceID;
*	}
*
*	udelay(500);
*
*	databuf[0] = 0x0;
*	res = i2c_master_recv(client, databuf, 0x01);
*	if(res <= 0)
*	{
*		goto exit_LIS3DH_CheckDeviceID;
*	}
*
*
*	if(databuf[0]!=LIS3DH_FIXED_DEVID)
*	{
*		return LIS3DH_ERR_IDENTIFICATION;
*	}
*
*	exit_LIS3DH_CheckDeviceID:
*	if (res <= 0)
*	{
*		return LIS3DH_ERR_I2C;
*	}
*	return LIS3DH_SUCCESS;
*}
*/
/*----------------------------------------------------------------------------*/
static int LIS3DH_SetPowerMode(struct i2c_client *client, bool enable)
{
	u8 databuf[2];
	int res = 0;
	u8 addr = LIS3DH_REG_CTL_REG1;
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);

	/* GSE_LOG("enter Sensor power status is sensor_power = %d\n",sensor_power); */

	if (enable == sensor_power) {
		GSE_LOG("Sensor power status is newest!\n");
		return LIS3DH_SUCCESS;
	}

	if (lis_i2c_read_block(client, addr, databuf, 0x01)) {
		GSE_ERR("read power ctl register err!\n");
		return LIS3DH_ERR_I2C;
	}

	if (enable == true)
		databuf[0] &=  ~LIS3DH_MEASURE_MODE;
	else
		databuf[0] |= LIS3DH_MEASURE_MODE;

	res = lis_i2c_write_block(client, LIS3DH_REG_CTL_REG1, databuf, 0x1);

	if (res <= 0) {
		GSE_LOG("set power mode failed!\n");
		return LIS3DH_ERR_I2C;
	} else if (atomic_read(&obj->trace) & ADX_TRC_INFO)
		GSE_LOG("set power mode ok %d!\n", databuf[1]);

	sensor_power = enable;
	/* GSE_LOG("leave Sensor power status is sensor_power = %d\n",sensor_power); */
	return LIS3DH_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[10];
	u8 addr = LIS3DH_REG_CTL_REG4;
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 10);

	if (lis_i2c_read_block(client, addr, databuf, 0x01)) {
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS3DH_ERR_I2C;
	}

	databuf[0] &= ~0x30;
	databuf[0] |= dataformat;

	res = lis_i2c_write_block(client, LIS3DH_REG_CTL_REG4, databuf, 0x1);

	if (res <= 0)
		return LIS3DH_ERR_I2C;


	return LIS3DH_SetDataResolution(obj);
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[10];
	u8 addr = LIS3DH_REG_CTL_REG1;
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 10);

	if (lis_i2c_read_block(client, addr, databuf, 0x01)) {
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS3DH_ERR_I2C;
	}

	databuf[0] &= ~0xF0;
	databuf[0] |= bwrate;

	res = lis_i2c_write_block(client, LIS3DH_REG_CTL_REG1, databuf, 0x1);

	if (res < 0)
		return LIS3DH_ERR_I2C;

	return LIS3DH_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/* enalbe data ready interrupt */
static int LIS3DH_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	u8 databuf[2];
	u8 addr = LIS3DH_REG_CTL_REG3;
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 2);

	if (lis_i2c_read_block(client, addr, databuf, 0x01)) {
		GSE_ERR("read reg_ctl_reg1 register err!\n");
		return LIS3DH_ERR_I2C;
	}

	databuf[0] = 0x00;

	res = lis_i2c_write_block(client, LIS3DH_REG_CTL_REG3, databuf, 0x01);
	if (res < 0)
		return LIS3DH_ERR_I2C;

	return LIS3DH_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_Init(struct i2c_client *client, int reset_cali)
{

	struct lis3dh_i2c_data *obj = (struct lis3dh_i2c_data *)i2c_get_clientdata(client);
	int res = 0;
	u8 databuf[2] = {0, 0};
	/*
	*	res = LIS3DH_CheckDeviceID(client);
	*	if(res != LIS3DH_SUCCESS)
	*	{
	*		return res;
	*	}
	*/
	/* first clear reg1 */
	databuf[0] = 0x0f;
	res = lis_i2c_write_block(client, LIS3DH_REG_CTL_REG1, databuf, 0x01);
	/* if(res != LIS3DH_SUCCESS) */
	if (res < 0) {
		GSE_ERR("smith-acc 1\n");
		return res;
	}

	res = LIS3DH_SetBWRate(client, LIS3DH_BW_100HZ);/* 400 or 100 no other choice */
	/* if(res != LIS3DH_SUCCESS ) */
	if (res < 0) {
		GSE_ERR("smith-acc 2\n");
		return res;
	}

	res = LIS3DH_SetDataFormat(client, LIS3DH_RANGE_2G);/* 8g or 2G no oher choise */
	/* if(res != LIS3DH_SUCCESS) */
	if (res < 0) {
		GSE_ERR("smith-acc 3\n");
		return res;
	}
	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->reso->sensitivity;

	res = LIS3DH_SetIntEnable(client, false);
	/* if(res != LIS3DH_SUCCESS) */
	if (res < 0) {
		GSE_ERR("smith-acc 4\n");
		return res;
	}

	res = LIS3DH_SetPowerMode(client, enable_status);/* false); */
	/* if(res != LIS3DH_SUCCESS) */
	if (res < 0) {
		GSE_ERR("smith-acc 5\n");
		return res;
	}

	if (reset_cali != 0) {
		/* reset calibration only in power on */
		res = LIS3DH_ResetCalibration(client);
		/* if(res != LIS3DH_SUCCESS) */
		if (res < 0) {
			GSE_ERR("smith-acc 6\n");
			return res;
		}
	}

#ifdef CONFIG_LIS3DH_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	return LIS3DH_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8) * 10);

	if ((buf == NULL) || (bufsize <= 30))
		return -1;

	if (client == NULL) {
		*buf = 0;
		return -2;
	}

	sprintf(buf, "LIS3DH Chip");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
	struct lis3dh_i2c_data *obj = (struct lis3dh_i2c_data *)i2c_get_clientdata(client);
	u8 databuf[20];
	int acc[LIS3DH_AXES_NUM];
	int res = 0;

	memset(databuf, 0, sizeof(u8) * 10);

	if (buf == NULL)
		return -1;
	if (client == NULL) {
		*buf = 0;
		return -2;
	}

	if (sensor_suspend == 1) {
		/* GSE_LOG("sensor in suspend read not data!\n"); */
		return 0;
	}
#if 0
	if (sensor_power == FALSE) {
		res = LIS3DH_SetPowerMode(client, true);
		if (res)
			GSE_ERR("Power on lis3dh error %d!\n", res);
		msleep(20);
	}
#endif
	res = LIS3DH_ReadData(client, obj->data);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -3;
	}
	obj->data[LIS3DH_AXIS_X] += obj->cali_sw[LIS3DH_AXIS_X];
	obj->data[LIS3DH_AXIS_Y] += obj->cali_sw[LIS3DH_AXIS_Y];
	obj->data[LIS3DH_AXIS_Z] += obj->cali_sw[LIS3DH_AXIS_Z];

	/*remap coordinate*/
	acc[obj->cvt.map[LIS3DH_AXIS_X]] = obj->cvt.sign[LIS3DH_AXIS_X] * obj->data[LIS3DH_AXIS_X];
	acc[obj->cvt.map[LIS3DH_AXIS_Y]] = obj->cvt.sign[LIS3DH_AXIS_Y] * obj->data[LIS3DH_AXIS_Y];
	acc[obj->cvt.map[LIS3DH_AXIS_Z]] = obj->cvt.sign[LIS3DH_AXIS_Z] * obj->data[LIS3DH_AXIS_Z];

	/*
	*GSE_LOG("Mapped gsensor data: %d, %d, %d!\n",
	*acc[LIS3DH_AXIS_X], acc[LIS3DH_AXIS_Y], acc[LIS3DH_AXIS_Z]);
	*/

	/* Out put the mg */
	acc[LIS3DH_AXIS_X] = acc[LIS3DH_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	acc[LIS3DH_AXIS_Y] = acc[LIS3DH_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
	acc[LIS3DH_AXIS_Z] = acc[LIS3DH_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;


	sprintf(buf, "%04x %04x %04x", acc[LIS3DH_AXIS_X], acc[LIS3DH_AXIS_Y], acc[LIS3DH_AXIS_Z]);
	if (atomic_read(&obj->trace) & ADX_TRC_IOCTL) { /* atomic_read(&obj->trace) & ADX_TRC_IOCTL */
		GSE_LOG("gsensor data: %s!\n", buf);
		dumpReg(client);
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int LIS3DH_ReadRawData(struct i2c_client *client, char *buf)
{
	struct lis3dh_i2c_data *obj = (struct lis3dh_i2c_data *)i2c_get_clientdata(client);
	int res = 0;

	if (!buf || !client)
		return -EINVAL;

	res = LIS3DH_ReadData(client, obj->data);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	}
	sprintf(buf, "%04x %04x %04x", obj->data[LIS3DH_AXIS_X],
		obj->data[LIS3DH_AXIS_Y], obj->data[LIS3DH_AXIS_Z]);

	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis3dh_i2c_client;
	char strbuf[LIS3DH_BUFSIZE];

	if (client == NULL) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	LIS3DH_ReadChipInfo(client, strbuf, LIS3DH_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis3dh_i2c_client;
	char strbuf[LIS3DH_BUFSIZE];

	if (client == NULL) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	LIS3DH_ReadSensorData(client, strbuf, LIS3DH_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis3dh_i2c_client;
	struct lis3dh_i2c_data *obj;
	int err, len, mul;
	int tmp[LIS3DH_AXES_NUM];

	len = 0;

	if (client == NULL) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);



	err = LIS3DH_ReadCalibration(client, tmp);
	if (err)
		return -EINVAL;

	mul = obj->reso->sensitivity / lis3dh_offset_resolution.sensitivity;
	len += snprintf(buf + len, PAGE_SIZE - len,
		"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
		obj->offset[LIS3DH_AXIS_X], obj->offset[LIS3DH_AXIS_Y], obj->offset[LIS3DH_AXIS_Z],
		obj->offset[LIS3DH_AXIS_X], obj->offset[LIS3DH_AXIS_Y], obj->offset[LIS3DH_AXIS_Z]);
	len += snprintf(buf + len, PAGE_SIZE - len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		obj->cali_sw[LIS3DH_AXIS_X], obj->cali_sw[LIS3DH_AXIS_Y], obj->cali_sw[LIS3DH_AXIS_Z]);

	len += snprintf(buf + len, PAGE_SIZE - len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		obj->offset[LIS3DH_AXIS_X] * mul + obj->cali_sw[LIS3DH_AXIS_X],
		obj->offset[LIS3DH_AXIS_Y] * mul + obj->cali_sw[LIS3DH_AXIS_Y],
		obj->offset[LIS3DH_AXIS_Z] * mul + obj->cali_sw[LIS3DH_AXIS_Z],
		tmp[LIS3DH_AXIS_X], tmp[LIS3DH_AXIS_Y], tmp[LIS3DH_AXIS_Z]);

	return len;

}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct i2c_client *client = lis3dh_i2c_client;
	int err, x, y, z;
	int dat[LIS3DH_AXES_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = LIS3DH_ResetCalibration(client);
		if (err)
			GSE_ERR("reset offset err = %d\n", err);
	} else if (sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z) == 3) {
		dat[LIS3DH_AXIS_X] = x;
		dat[LIS3DH_AXIS_Y] = y;
		dat[LIS3DH_AXIS_Z] = z;
		err = LIS3DH_WriteCalibration(client, dat);
		if (err)
			GSE_ERR("write calibration err = %d\n", err);
	} else
		GSE_ERR("invalid format\n");

	return count;
}
/*----------------------------------------------------------------------------*/

static ssize_t show_power_status(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = lis3dh_i2c_client;
	struct lis3dh_i2c_data *obj;
	u8 data;

	if (client == NULL) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	lis_i2c_read_block(client, LIS3DH_REG_CTL_REG1, &data, 0x01);
	return snprintf(buf, PAGE_SIZE, "%x\n", data);
}
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_LIS3DH_LOWPASS
	struct i2c_client *client = lis3dh_i2c_client;
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);

	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);

		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++)
			GSE_LOG("[%5d %5d %5d]\n",
				obj->fir.raw[idx][LIS3DH_AXIS_X],
				obj->fir.raw[idx][LIS3DH_AXIS_Y], obj->fir.raw[idx][LIS3DH_AXIS_Z]);

		GSE_LOG("sum = [%5d %5d %5d]\n",
			obj->fir.sum[LIS3DH_AXIS_X], obj->fir.sum[LIS3DH_AXIS_Y], obj->fir.sum[LIS3DH_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n",
			obj->fir.sum[LIS3DH_AXIS_X] / len,
			obj->fir.sum[LIS3DH_AXIS_Y] / len, obj->fir.sum[LIS3DH_AXIS_Z] / len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_LIS3DH_LOWPASS
	struct i2c_client *client = lis3dh_i2c_client;
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;
	int ret = 0;

	ret = kstrtoint(buf, 10, &firlen);
	if (ret != 0)
		GSE_ERR("invallid format\n");
	else if (firlen > C_MAX_FIR_LENGTH)
		GSE_ERR("exceeds maximum filter length\n");
	else {
		atomic_set(&obj->firlen, firlen);
		if (firlen == 0)
			atomic_set(&obj->fir_en, 0);
		else {
			memset(&obj->fir, 0x00, sizeof(obj->fir));
			atomic_set(&obj->fir_en, 1);
		}
	}
#endif
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct lis3dh_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lis3dh_i2c_data *obj = obj_i2c_data;
	int trace;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s', length = %d\n", buf, (int)count);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lis3dh_i2c_data *obj = obj_i2c_data;

	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d %d (%d %d)\n",
				obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	} else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	return len;
}
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DRIVER_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DRIVER_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DRIVER_ATTR(power,                S_IRUGO, show_power_status,          NULL);
static DRIVER_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DRIVER_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DRIVER_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct driver_attribute *lis3dh_attr_list[] = {
	&driver_attr_chipinfo,     /*chip information*/
	&driver_attr_sensordata,   /*dump sensor data*/
	&driver_attr_cali,         /*show calibration data*/
	&driver_attr_power,         /*show power reg*/
	&driver_attr_firlen,       /*filter length: 0: disable, others: enable*/
	&driver_attr_trace,        /*trace log*/
	&driver_attr_status,
};
/*----------------------------------------------------------------------------*/
static int lis3dh_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(lis3dh_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, lis3dh_attr_list[idx]);
		if (err) {
			GSE_ERR("driver_create_file (%s) = %d\n", lis3dh_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int lis3dh_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(lis3dh_attr_list));

	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, lis3dh_attr_list[idx]);


	return err;
}

/*----------------------------------------------------------------------------*/
int gsensor_operate(void *self, uint32_t command, void *buff_in, int size_in,
		    void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	struct lis3dh_i2c_data *priv = (struct lis3dh_i2c_data *)self;
	struct hwm_sensor_data *gsensor_data;
	char buff[LIS3DH_BUFSIZE];

	/* GSE_FUN(f); */
	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			GSE_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value <= 5)
				sample_delay = LIS3DH_BW_200HZ;
			else if (value <= 10)
				sample_delay = LIS3DH_BW_100HZ;
			else
				sample_delay = LIS3DH_BW_50HZ;
			mutex_lock(&lis3dh_op_mutex);
			err = LIS3DH_SetBWRate(priv->client, sample_delay);
			if (err != LIS3DH_SUCCESS) /* 0x2C->BW=100Hz */
				GSE_ERR("Set delay parameter error!\n");
			mutex_unlock(&lis3dh_op_mutex);
			if (value >= 50)
				atomic_set(&priv->filter, 0);
			else {
				priv->fir.num = 0;
				priv->fir.idx = 0;
				priv->fir.sum[LIS3DH_AXIS_X] = 0;
				priv->fir.sum[LIS3DH_AXIS_Y] = 0;
				priv->fir.sum[LIS3DH_AXIS_Z] = 0;
				atomic_set(&priv->filter, 1);
			}
		}
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			GSE_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {

			value = *(int *)buff_in;
			mutex_lock(&lis3dh_op_mutex);
			GSE_LOG("Gsensor device enable function enable = %d, sensor_power = %d!\n",
				value, sensor_power);
			if (((value == 0) && (sensor_power == false)) || ((value == 1) && (sensor_power == true))) {
				enable_status = sensor_power;
				GSE_LOG("Gsensor device have updated!\n");
			} else {
				enable_status = !sensor_power;
				err = LIS3DH_SetPowerMode(priv->client, !sensor_power);
				GSE_LOG("Gsensor not in suspend lis3dh_SetPowerMode!, enable_status = %d\n",
					enable_status);
			}
			mutex_unlock(&lis3dh_op_mutex);
		}
		break;

	case SENSOR_GET_DATA:
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			GSE_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			mutex_lock(&lis3dh_op_mutex);
			gsensor_data = (struct hwm_sensor_data *)buff_out;
			LIS3DH_ReadSensorData(priv->client, buff, LIS3DH_BUFSIZE);
			err = sscanf(buff, "%x %x %x", &gsensor_data->values[0],
			       &gsensor_data->values[1], &gsensor_data->values[2]);
			gsensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			gsensor_data->value_divide = 1000;
			mutex_unlock(&lis3dh_op_mutex);
		}
		break;
	default:
		GSE_ERR("gsensor operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}

	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int lis3dh_open(struct inode *inode, struct file *file)
{
	file->private_data = lis3dh_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int lis3dh_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long lis3dh_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)

{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct lis3dh_i2c_data *obj = (struct lis3dh_i2c_data *)i2c_get_clientdata(client);
	char strbuf[LIS3DH_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	/* GSE_FUN(f); */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		LIS3DH_Init(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		LIS3DH_ReadChipInfo(client, strbuf, LIS3DH_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
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
		LIS3DH_SetPowerMode(client, true);
		LIS3DH_ReadSensorData(client, strbuf, LIS3DH_BUFSIZE);
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

		if (copy_to_user(data, &gsensor_gain, sizeof(struct GSENSOR_VECTOR3D))) {
			err = -EFAULT;
			break;
		}
		break;

	case GSENSOR_IOCTL_READ_OFFSET:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		if (copy_to_user(data, &gsensor_offset, sizeof(struct GSENSOR_VECTOR3D))) {
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
		LIS3DH_ReadRawData(client, strbuf);
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
			cali[LIS3DH_AXIS_X] = sensor_data.x * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[LIS3DH_AXIS_Y] = sensor_data.y * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			cali[LIS3DH_AXIS_Z] = sensor_data.z * obj->reso->sensitivity / GRAVITY_EARTH_1000;
			err = LIS3DH_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = LIS3DH_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = LIS3DH_ReadCalibration(client, cali);
		if (err)
			break;

		sensor_data.x = cali[LIS3DH_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.y = cali[LIS3DH_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
		sensor_data.z = cali[LIS3DH_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
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


/*----------------------------------------------------------------------------*/
static const struct file_operations lis3dh_fops = {
	.owner = THIS_MODULE,
	.open = lis3dh_open,
	.release = lis3dh_release,
	.unlocked_ioctl = lis3dh_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice lis3dh_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &lis3dh_fops,
};
/*----------------------------------------------------------------------------*/

#ifndef USE_EARLY_SUSPEND
/*----------------------------------------------------------------------------*/
static int lis3dh_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);

	int err = 0;
	/* u8 dat; */
	GSE_FUN();
	mutex_lock(&lis3dh_op_mutex);
	/* if(msg.event == PM_EVENT_SUSPEND) */
	/* { */
	if (obj == NULL) {
		mutex_unlock(&lis3dh_op_mutex);
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	/* read old data */
	err = LIS3DH_SetPowerMode(obj->client, false);
	if (err) {
		GSE_ERR("write power control fail!!\n");
		mutex_unlock(&lis3dh_op_mutex);
		return err;
	}

	atomic_set(&obj->suspend, 1);
	LIS3DH_power(obj->hw, 0);
	/* } */
	sensor_suspend = 1;
	mutex_unlock(&lis3dh_op_mutex);
	return err;
}
/*----------------------------------------------------------------------------*/
static int lis3dh_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lis3dh_i2c_data *obj = i2c_get_clientdata(client);
	int err;

	GSE_FUN();
	mutex_lock(&lis3dh_op_mutex);
	if (obj == NULL) {
		mutex_unlock(&lis3dh_op_mutex);
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	LIS3DH_power(obj->hw, 1);
	err = LIS3DH_Init(client, 0);
	if (err) {
		mutex_unlock(&lis3dh_op_mutex);
		GSE_ERR("initialize client fail!!\n");
		return err;
	}
	atomic_set(&obj->suspend, 0);
	sensor_suspend = 0;
	mutex_unlock(&lis3dh_op_mutex);
	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/

static void lis3dh_early_suspend(struct early_suspend *h)
{
	struct lis3dh_i2c_data *obj = container_of(h, struct lis3dh_i2c_data, early_drv);
	u8 databuf[2];
	int err = 0;
	u8 addr = LIS3DH_REG_CTL_REG1;

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->suspend, 1);
	mutex_lock(&lis3dh_op_mutex);
	GSE_FUN();
	err = LIS3DH_SetPowerMode(obj->client, false);
	if (err) {
		GSE_ERR("write power control fail!!\n");
		mutex_unlock(&lis3dh_op_mutex);
		return;
	}
	sensor_suspend = 1;
	mutex_unlock(&lis3dh_op_mutex);
	LIS3DH_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void lis3dh_late_resume(struct early_suspend *h)
{
	struct lis3dh_i2c_data *obj = container_of(h, struct lis3dh_i2c_data, early_drv);
	int err;

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}

	LIS3DH_power(obj->hw, 1);
	mutex_lock(&lis3dh_op_mutex);
	GSE_FUN();
	err = LIS3DH_Init(obj->client, 0);
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		mutex_unlock(&lis3dh_op_mutex);
		return;
	}
	sensor_suspend = 0;
	mutex_unlock(&lis3dh_op_mutex);
	atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
#if 0
static int lis3dh_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, LIS3DH_DEV_NAME);
	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
static int lis3dh_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

	struct i2c_client *new_client;
	struct lis3dh_i2c_data *obj;
	struct hwmsen_object sobj;
	int err = 0;
	int retry = 0;

	GSE_FUN();

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		GSE_ERR("====brucelee lis3dh_i2c_probe 111\n");
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct lis3dh_i2c_data));

	obj->hw = hw;

	GSE_ERR("====brucelee iic addr : %d\n", obj->hw->i2c_num);
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);

	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);

#ifdef CONFIG_LIS3DH_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);

#endif

	lis3dh_i2c_client = new_client;

	for (retry = 0; retry < 3; retry++) {
		err = LIS3DH_Init(new_client, 1);
		if (err) {
			GSE_ERR("lis3dh_device init cilent fail time: %d\n", retry);
			continue;
		}
	}
	if (err != 0)
		goto exit_init_failed;

	err = misc_register(&lis3dh_device);
	if (err) {
		GSE_ERR("lis3dh_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = lis3dh_create_attr(&lis3dh_gsensor_driver.platform_diver_addr->driver);
	if (err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	sobj.self = obj;
	sobj.polling = 1;
	sobj.sensor_operate = gsensor_operate;
	err = hwmsen_attach(ID_ACCELEROMETER, &sobj);
	if (err) {
		GSE_ERR("attach fail = %d\n", err);
		goto exit_kfree;
	}

#ifdef USE_EARLY_SUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 2,
		       obj->early_drv.suspend  = lis3dh_early_suspend,
				      obj->early_drv.resume   = lis3dh_late_resume,
						     register_early_suspend(&obj->early_drv);
#endif

	GSE_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&lis3dh_device);
exit_misc_device_register_failed:
exit_init_failed:
	/* i2c_detach_client(new_client); */
exit_kfree:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	return err;
}

/*----------------------------------------------------------------------------*/
static int lis3dh_i2c_remove(struct i2c_client *client)
{
	int err = 0;

	err = lis3dh_delete_attr(&lis3dh_gsensor_driver.platform_diver_addr->driver);
	if (err)
		GSE_ERR("lis3dh_delete_attr fail: %d\n", err);
	misc_deregister(&lis3dh_device);
	/* if((err = )) */
	/* { */
	/* GSE_ERR("misc_deregister fail: %d\n", err); */
	/* } */

	err = hwmsen_detach(ID_ACCELEROMETER);
	if (err)
		lis3dh_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lis3dh_probe(void)
{
	GSE_FUN();
	LIS3DH_power(hw, 1);
	/* lis3dh_force[0] = hw->i2c_num; */
	if (i2c_add_driver(&lis3dh_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return -1;
	}
	GSE_LOG("====brucelee =====lis3dh_probe end\n");
	return 0;
}
/*----------------------------------------------------------------------------*/
static int lis3dh_remove(void)
{

	GSE_FUN();
	LIS3DH_power(hw, 0);
	i2c_del_driver(&lis3dh_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/
static int __init lis3dh_init(void)
{
	GSE_FUN();
	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);
	if (!hw)
		GSE_ERR("get dts info fail\n");

	acc_driver_add(&lis3dh_gsensor_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit lis3dh_exit(void)
{
	GSE_FUN();
	/* platform_driver_unregister(&lis3dh_gsensor_driver); */
}
/*----------------------------------------------------------------------------*/
module_init(lis3dh_init);
module_exit(lis3dh_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LIS3DH I2C driver");
MODULE_AUTHOR("Chunlei.Wang@mediatek.com");

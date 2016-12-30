//tuwenzan@wind-mobi.com modify at 20161108 begin
/* BMA255 motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * VERSION: V1.7
 * HISTORY:
 * V1.0 --- Driver creation
 * V1.1 --- Add share I2C address function
 * V1.2 --- Fix the bug that sometimes sensor is stuck after system resume.
 * V1.3 --- Add FIFO interfaces.
 * V1.4 --- Use basic i2c function to read fifo data instead of i2c DMA mode.
 * V1.5 --- Add compensated value performed by MTK acceleration calibration.
 * V1.6 --- Add walkaround for powermode change.
 * V1.7 --- Add walkaround for fifo.
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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>


#include <cust_acc.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <accel.h>


#include "bma2x2.h"

/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_BMA2x2 255
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
/*apply low pass filter on output*/
/*#define CONFIG_BMA2x2_LOWPASS*/
#define SW_CALIBRATION
#define CONFIG_I2C_BASIC_FUNCTION

static struct mutex sensor_data_mutex;
static DECLARE_WAIT_QUEUE_HEAD(uplink_event_flag_wq);
static int CHIP_TYPE;

/*tad3sgh add --*/
#define MAX_FIFO_F_LEVEL 32
#define MAX_FIFO_F_BYTES 6
#define READFIFOCOUNT
#define DMAREAD
/*----------------------------------------------------------------------------*/
#define BMA2x2_AXIS_X          0
#define BMA2x2_AXIS_Y          1
#define BMA2x2_AXIS_Z          2
#define BMA2x2_AXES_NUM        3
#define BMA2x2_DATA_LEN        6
#define BMA2x2_DEV_NAME        "BMA255"

#define BMA2x2_MODE_NORMAL      0
#define BMA2x2_MODE_LOWPOWER    1
#define BMA2x2_MODE_SUSPEND     2

/*for bma222e chip*/
#define BMA222E_ACC_X_LSB__POS          0x00
#define BMA222E_ACC_X_LSB__LEN          0x00
#define BMA222E_ACC_X_LSB__MSK          0x00

#define BMA222E_ACC_X_MSB__POS           0
#define BMA222E_ACC_X_MSB__LEN           8
#define BMA222E_ACC_X_MSB__MSK           0xFF

#define BMA222E_ACC_Y_LSB__POS          0x00
#define BMA222E_ACC_Y_LSB__LEN          0x00
#define BMA222E_ACC_Y_LSB__MSK          0x00

#define BMA222E_ACC_Y_MSB__POS           0
#define BMA222E_ACC_Y_MSB__LEN           8
#define BMA222E_ACC_Y_MSB__MSK           0xFF

#define BMA222E_ACC_Z_LSB__POS          0x00
#define BMA222E_ACC_Z_LSB__LEN          0x00
#define BMA222E_ACC_Z_LSB__MSK          0x00

#define BMA222E_ACC_Z_MSB__POS           0
#define BMA222E_ACC_Z_MSB__LEN           8
#define BMA222E_ACC_Z_MSB__MSK           0xFF

/*for bma250e chip*/
#define BMA250E_ACC_X_LSB__POS          6
#define BMA250E_ACC_X_LSB__LEN          2
#define BMA250E_ACC_X_LSB__MSK          0xC0

#define BMA250E_ACC_X_MSB__POS           0
#define BMA250E_ACC_X_MSB__LEN           8
#define BMA250E_ACC_X_MSB__MSK           0xFF

#define BMA250E_ACC_Y_LSB__POS          6
#define BMA250E_ACC_Y_LSB__LEN          2
#define BMA250E_ACC_Y_LSB__MSK          0xC0

#define BMA250E_ACC_Y_MSB__POS           0
#define BMA250E_ACC_Y_MSB__LEN           8
#define BMA250E_ACC_Y_MSB__MSK           0xFF

#define BMA250E_ACC_Z_LSB__POS          6
#define BMA250E_ACC_Z_LSB__LEN          2
#define BMA250E_ACC_Z_LSB__MSK          0xC0

#define BMA250E_ACC_Z_MSB__POS           0
#define BMA250E_ACC_Z_MSB__LEN           8
#define BMA250E_ACC_Z_MSB__MSK           0xFF

/*for bma255 chip*/
#define BMA255_ACC_X_LSB__POS           4
#define BMA255_ACC_X_LSB__LEN           4
#define BMA255_ACC_X_LSB__MSK           0xF0

#define BMA255_ACC_X_MSB__POS           0
#define BMA255_ACC_X_MSB__LEN           8
#define BMA255_ACC_X_MSB__MSK           0xFF


#define BMA255_ACC_Y_LSB__POS           4
#define BMA255_ACC_Y_LSB__LEN           4
#define BMA255_ACC_Y_LSB__MSK           0xF0


#define BMA255_ACC_Y_MSB__POS           0
#define BMA255_ACC_Y_MSB__LEN           8
#define BMA255_ACC_Y_MSB__MSK           0xFF


#define BMA255_ACC_Z_LSB__POS           4
#define BMA255_ACC_Z_LSB__LEN           4
#define BMA255_ACC_Z_LSB__MSK           0xF0


#define BMA255_ACC_Z_MSB__POS           0
#define BMA255_ACC_Z_MSB__LEN           8
#define BMA255_ACC_Z_MSB__MSK           0xFF

/*for bma280*/
#define BMA280_ACC_X_LSB__POS           2
#define BMA280_ACC_X_LSB__LEN           6
#define BMA280_ACC_X_LSB__MSK           0xFC

#define BMA280_ACC_X_MSB__POS           0
#define BMA280_ACC_X_MSB__LEN           8
#define BMA280_ACC_X_MSB__MSK           0xFF

#define BMA280_ACC_Y_LSB__POS           2
#define BMA280_ACC_Y_LSB__LEN           6
#define BMA280_ACC_Y_LSB__MSK           0xFC

#define BMA280_ACC_Y_MSB__POS           0
#define BMA280_ACC_Y_MSB__LEN           8
#define BMA280_ACC_Y_MSB__MSK           0xFF

#define BMA280_ACC_Z_LSB__POS           2
#define BMA280_ACC_Z_LSB__LEN           6
#define BMA280_ACC_Z_LSB__MSK           0xFC

#define BMA280_ACC_Z_MSB__POS           0
#define BMA280_ACC_Z_MSB__LEN           8
#define BMA280_ACC_Z_MSB__MSK           0xFF

#define BMA2x2_EN_LOW_POWER__POS          6
#define BMA2x2_EN_LOW_POWER__LEN          1
#define BMA2x2_EN_LOW_POWER__MSK          0x40
#define BMA2x2_EN_LOW_POWER__REG          BMA2x2_REG_POWER_CTL

#define BMA2x2_EN_SUSPEND__POS            7
#define BMA2x2_EN_SUSPEND__LEN            1
#define BMA2x2_EN_SUSPEND__MSK            0x80
#define BMA2x2_EN_SUSPEND__REG            BMA2x2_REG_POWER_CTL

/* fifo mode*/
#define BMA2x2_FIFO_MODE__POS                 6
#define BMA2x2_FIFO_MODE__LEN                 2
#define BMA2x2_FIFO_MODE__MSK                 0xC0
#define BMA2x2_FIFO_MODE__REG                 BMA2x2_FIFO_MODE_REG

#define BMA2x2_FIFO_FRAME_COUNTER_S__POS             0
#define BMA2x2_FIFO_FRAME_COUNTER_S__LEN             7
#define BMA2x2_FIFO_FRAME_COUNTER_S__MSK             0x7F
#define BMA2x2_FIFO_FRAME_COUNTER_S__REG             BMA2x2_STATUS_FIFO_REG

#define BMA2x2_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA2x2_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

//tuwenzan@wind-mobi.com add at 20161128 begin
int cali_flag = 0;
struct i2c_client *bma2x2_client;
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id bma2x2_i2c_id[] = {{BMA2x2_DEV_NAME, 0}, {} };
//twz modify
#ifndef CONFIG_OF
static struct i2c_board_info __initdata bma2x2_i2c_info = {
	I2C_BOARD_INFO(BMA2x2_DEV_NAME, BMA2x2_I2C_ADDR)
};
#endif
/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
static int bma2x2_i2c_remove(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
enum {
	BMA_TRC_FILTER  = 0x01,
	BMA_TRC_RAWDATA = 0x02,
	BMA_TRC_IOCTL   = 0x04,
	BMA_TRC_CALI	= 0X08,
	BMA_TRC_INFO	= 0X10,
} BMA_TRC;
/*----------------------------------------------------------------------------*/
struct scale_factor {
	u8  whole;
	u16  fraction; //twz  modify u8->u16
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
	s16 raw[C_MAX_FIR_LENGTH][BMA2x2_AXES_NUM];
	int sum[BMA2x2_AXES_NUM];
	int num;
	int idx;
};
/*----------------------------------------------------------------------------*/
struct bma2x2_i2c_data {
	struct i2c_client *client;
	struct acc_hw *hw;
	struct hwmsen_convert   cvt;

/*misc*/
	struct data_resolution *reso;
	atomic_t                trace;
	atomic_t                suspend;
	atomic_t                selftest;
	atomic_t				filter;
	s16                     cali_sw[BMA2x2_AXES_NUM+1];
	struct mutex lock;

/*data*/
/*+1: for 4-byte alignment*/
	s8                      offset[BMA2x2_AXES_NUM+1];
	s16                     data[BMA2x2_AXES_NUM+1];
	u8                      fifo_count;

#if defined(CONFIG_BMA2x2_LOWPASS)
	atomic_t                firlen;
	atomic_t                fir_en;
	struct data_filter      fir;
#endif

/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend    early_drv;
#endif
};
#if !defined(CONFIG_HAS_EARLYSUSPEND)
static int bma2x2_suspend(struct i2c_client *client, pm_message_t msg);
static int bma2x2_resume(struct i2c_client *client);
#endif
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id gsensor_of_match[] = {
	{ .compatible = "mediatek,gsensor", },
	{},
};
#endif

static struct i2c_driver bma2x2_i2c_driver = {
	.driver = {
		.name           = BMA2x2_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = gsensor_of_match,
#endif
	},
	.probe				= bma2x2_i2c_probe,
	.remove				= bma2x2_i2c_remove,
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	.suspend            = bma2x2_suspend,
	.resume             = bma2x2_resume,
#endif
	.id_table = bma2x2_i2c_id,
};

/*----------------------------------------------------------------------------*/
static struct i2c_client *bma2x2_i2c_client;
static struct acc_init_info bma2x2_init_info;
static struct bma2x2_i2c_data *obj_i2c_data;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;
//static char selftestRes[8] = {0};
static int bma2x2_init_flag = -1; /* 0<==>OK -1 <==> fail */
#define DMA_BUFFER_SIZE (32*6)
static unsigned char *I2CDMABuf_va;
static dma_addr_t I2CDMABuf_pa;

/*----------------------------------------------------------------------------*/
#define GSE_TAG                 "[Gsensor] "
#define GSE_FUN(f)              printk(GSE_TAG"%s\n", __func__)
#define GSE_ERR(fmt, args...)   \
printk(GSE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)   printk(GSE_TAG fmt, ##args)


#define COMPATIABLE_NAME "mediatek,bma255"

struct acc_hw accel_cust;
static struct acc_hw *hw = &accel_cust;

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_2g_data_resolution[4] = {
/*combination by {FULL_RES,RANGE}*/
/*BMA222E +/-2g  in 8-bit;  { 15, 63} = 15.63;  64 = (2^8)/(2*2)*/
	 { { 15, 63 }, 64 },
/*BMA250E +/-2g  in 10-bit;  { 3, 91} = 3.91;  256 = (2^10)/(2*2)*/
	 { { 3, 91 }, 256 },
/*BMA255 +/-2g  in 12-bit;  { 0, 98} = 0.98;  1024 = (2^12)/(2*2)*/
	 {{ 0, 98}, 1024},
/*BMA280 +/-2g  in 14-bit;  { 0, 244} = 0.244;  4096 = (2^14)/(2*2)*/
	 { { 0, 488 }, 4096 },
	 };
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_4g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-4g  in 8-bit;  { 31, 25} = 31.25;  32 = (2^8)/(2*4)*/
	{ { 31, 25 }, 32 },
	/*BMA250E +/-4g  in 10-bit;  { 7, 81} = 7.81;  128 = (2^10)/(2*4)*/
	{ { 7, 81 }, 128 },
	/*BMA255 +/-4g  in 12-bit;  { 1, 95} = 1.95;  512 = (2^12)/(2*4)*/
	{ { 1, 95 }, 512 },
	/*BMA280 +/-4g  in 14-bit;  { 0, 488} = 0.488;  2048 = (2^14)/(2*4)*/
	{ { 0, 488 }, 2048 },
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_8g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-8g  in 8-bit;  { 62, 50} = 62.5;  16 = (2^8)/(2*8)*/
	{ { 62, 50 }, 16 },
	/*BMA250E +/-8g  in 10-bit;  { 15, 63} = 15.63;  64 = (2^10)/(2*8)*/
	{ { 15, 63 }, 64 },
	/*BMA255 +/-8g  in 12-bit;  { 3, 91} = 3.91;  256 = (2^12)/(2*8)*/
	{ { 3, 91 }, 256 },
	/*BMA280 +/-8g  in 14-bit;  { 0, 977} = 0.977;  1024 = (2^14)/(2*8)*/
	{ { 0, 977 }, 1024 },
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct data_resolution bma2x2_16g_data_resolution[4] = {
	/*combination by {FULL_RES,RANGE}*/
	/*BMA222E +/-16g  in 8-bit;  { 125, 0} = 125.0;  8 = (2^8)/(2*16)*/
	{ { 125, 0 }, 8 },
	/*BMA250E +/-16g  in 10-bit;  { 31, 25} = 31.25;  32 = (2^10)/(2*16)*/
	{ { 31, 25 }, 32 },
	/*BMA255 +/-16g  in 12-bit;  { 7, 81} = 7.81;  128 = (2^12)/(2*16)*/
	{ { 7, 81 }, 128 },
	/*BMA280 +/-16g  in 14-bit;  { 1, 953} = 1.953;  512 = (2^14)/(2*16)*/
	{ { 1, 953 }, 512 },
};
/*----------------------------------------------------------------------------*/

static struct data_resolution bma2x2_offset_resolution;

/* I2C operation functions */
#ifdef DMA_FEATURE
static int bma_i2c_dma_read(struct i2c_client *client,
	 unsigned char regaddr, unsigned char *readbuf, int readlen)
{
#ifdef DMA_READ
	int ret = 0;
	if (readlen > DMA_BUFFER_SIZE) {
		GSE_ERR("Read length cann't exceed dma buffer size!\n");
		return -EINVAL;
	}
	mutex_lock(&i2c_lock);
	/*write the register address*/
	ret = i2c_master_send(client, &regaddr, 1);
	if (ret < 0) {
		GSE_ERR("send command error!!\n");
		return -EFAULT;
	}

	/*dma read*/
	client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG;
	ret = i2c_master_recv(client, I2CDMABuf_pa, readlen);
	/*clear DMA flag once transfer done*/
	client->addr = client->addr & I2C_MASK_FLAG & (~I2C_DMA_FLAG);
	if (ret < 0) {
		GSE_ERR("dma receive data error!!\n");
		return -EFAULT;
	}
	memcpy(readbuf, I2CDMABuf_va, readlen);
	mutex_unlock(&i2c_lock);
	return ret;
#else

	int ret;
	s32 retry = 0;
	u8 buffer[2];

	struct i2c_msg msg[2] = {
	 {
	 .addr = (client->addr & I2C_MASK_FLAG),
	 .flags = 0,
	 .buf = buffer,
	 .len = 1,
	 },
	 {
	 .addr = (client->addr & I2C_MASK_FLAG),
	 .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
	 .flags = I2C_M_RD,
	 .buf = I2CDMABuf_pa,
	 .len = readlen,
	 },
	};

	buffer[0] = regaddr;

	if (readbuf == NULL)
		return BMA2x2_ERR_I2C;

	for (retry = 0; retry < 10; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		 if (ret < 0) {
			GSE_ERR("I2C DMA read error retry=%d", retry);
		 continue;
	 }
	 memcpy(readbuf, I2CDMABuf_va, readlen);
	 return 0;
	}
	GSE_ERR("Dma I2C Read Error: 0x%04X, %d byte(s), err-code: %d",
	 regaddr, readlen, ret);
	return ret;
#endif

}
#endif

int bma_i2c_read_block(struct i2c_client *client,u8 addr, u8 *data, u8 len)  //tuwenzan@wind-mobi.com modify at 20161128
{
	u8 beg = addr;
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
			.len = len,
			.buf = data,
		}
	};
	int err;

	if (!client)
		return -EINVAL;

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
#define I2C_BUFFER_SIZE 256
int bma_i2c_write_block(struct i2c_client *client, u8 addr,u8 *data, u8 len)  //tuwenzan@wind-mobi.com modify at 20161128
{
	/*
	*because address also occupies one byte,
	*the maximum length for write is 7 bytes
	*/
	int err, idx = 0, num = 0;
	char buf[32];

	if (!client)
		return -EINVAL;
	else if (len > C_I2C_FIFO_SIZE) {
		GSE_ERR(" length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
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


/*-----BMA255 power control function------------------------*/
static void BMA2x2_power(struct acc_hw *hw, unsigned int on)
{
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int BMA2x2_SetDataResolution(struct bma2x2_i2c_data *obj, u8 dataformat)
{

/*set g sensor dataresolution here*/

/*BMA255 only can set to 10-bit dataresolution*/
/*so do nothing in bma255 driver here*/

/*end of set dataresolution*/

/*we set measure range from -4g to +4g in
*BMA2x2_SetDataFormat(client, BMA2x2_RANGE_2G),
*and set 10-bit dataresolution BMA2x2_SetDataResolution()*/

	if (CHIP_TYPE >= 0) {
		switch (dataformat) {
		case 0x03:
		/*2g range*/
			obj->reso = &bma2x2_2g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_2g_data_resolution[CHIP_TYPE];
			break;
		case 0x05:
		/*4g range*/
			obj->reso = &bma2x2_4g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_4g_data_resolution[CHIP_TYPE];
			break;
		case 0x08:
		/*8g range*/
			obj->reso = &bma2x2_8g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_8g_data_resolution[CHIP_TYPE];
			break;
		case 0x0C:
		/*16g range*/
			obj->reso = &bma2x2_16g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_16g_data_resolution[CHIP_TYPE];
			break;
		default:
			obj->reso = &bma2x2_2g_data_resolution[CHIP_TYPE];
			bma2x2_offset_resolution = bma2x2_2g_data_resolution[CHIP_TYPE];
		}

	}
	return 0;

/*if you changed the measure range, for example call:
*BMA2x2_SetDataFormat(client, BMA2x2_RANGE_4G),
*you must set the right value to bma2x2_data_resolution*/

}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadData(struct i2c_client *client, s16 data[BMA2x2_AXES_NUM])
{
	u8 addr = BMA2x2_REG_DATAXLOW;
	u8 buf[BMA2x2_DATA_LEN] = {0};
	int err = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#ifdef CONFIG_BMA2x2_LOWPASS
	int num = 0;
	int X = BMA2x2_AXIS_X;
	int Y = BMA2x2_AXIS_Y;
	int Z = BMA2x2_AXIS_Z;
#endif
	if (NULL == client)
		err = -EINVAL;
	mutex_lock(&obj->lock);
	err = bma_i2c_read_block(client, addr, buf, BMA2x2_DATA_LEN);
	mutex_unlock(&obj->lock);
	if (err) {
		GSE_ERR("error: %d\n", err);
	} else {
		if (CHIP_TYPE == BMA255_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA255_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA255_ACC_X_MSB) << BMA255_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA255_ACC_X_LSB__LEN
			 + BMA255_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA255_ACC_X_LSB__LEN
			 + BMA255_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA255_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA255_ACC_Y_MSB) << BMA255_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA255_ACC_Y_LSB__LEN
			 + BMA255_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA255_ACC_Y_LSB__LEN
			 + BMA255_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA255_ACC_Z_LSB)
			| (BMA2x2_GET_BITSLICE(buf[5],
			 BMA255_ACC_Z_MSB) << BMA255_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA255_ACC_Z_LSB__LEN
			 + BMA255_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA255_ACC_Z_LSB__LEN
			 + BMA255_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA222E_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA222E_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA222E_ACC_X_MSB) << BMA222E_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_X_LSB__LEN
			 + BMA222E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_X_LSB__LEN
			 + BMA222E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA222E_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA222E_ACC_Y_MSB) << BMA222E_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_Y_LSB__LEN
			 + BMA222E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_Y_LSB__LEN
			 + BMA222E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA222E_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA222E_ACC_Z_MSB) << BMA222E_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA222E_ACC_Z_LSB__LEN
			 + BMA222E_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA222E_ACC_Z_LSB__LEN
			 + BMA222E_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA250E_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA250E_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA250E_ACC_X_MSB) << BMA250E_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_X_LSB__LEN
			 + BMA250E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_X_LSB__LEN
			 + BMA250E_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA250E_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA250E_ACC_Y_MSB) << BMA250E_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_Y_LSB__LEN
			 + BMA250E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_Y_LSB__LEN
			 + BMA250E_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA250E_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA250E_ACC_Z_MSB) << BMA250E_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA250E_ACC_Z_LSB__LEN
			 + BMA250E_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA250E_ACC_Z_LSB__LEN
			 + BMA250E_ACC_Z_MSB__LEN));
		} else if (CHIP_TYPE == BMA280_TYPE) {
			/* Convert sensor raw data to 16-bit integer */
			data[BMA2x2_AXIS_X] =
			 BMA2x2_GET_BITSLICE(buf[0], BMA280_ACC_X_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[1],
			 BMA280_ACC_X_MSB) << BMA280_ACC_X_LSB__LEN);
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] <<
			 (sizeof(short) * 8 - (BMA280_ACC_X_LSB__LEN
			 + BMA280_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_X] = data[BMA2x2_AXIS_X] >>
			 (sizeof(short) * 8 - (BMA280_ACC_X_LSB__LEN
			 + BMA280_ACC_X_MSB__LEN));
			data[BMA2x2_AXIS_Y] =
			 BMA2x2_GET_BITSLICE(buf[2], BMA280_ACC_Y_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[3],
			 BMA280_ACC_Y_MSB) << BMA280_ACC_Y_LSB__LEN);
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] <<
			 (sizeof(short) * 8 - (BMA280_ACC_Y_LSB__LEN
			 + BMA280_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Y] = data[BMA2x2_AXIS_Y] >>
			 (sizeof(short) * 8 - (BMA280_ACC_Y_LSB__LEN
			 + BMA280_ACC_Y_MSB__LEN));
			data[BMA2x2_AXIS_Z] =
			 BMA2x2_GET_BITSLICE(buf[4], BMA280_ACC_Z_LSB)
			 | (BMA2x2_GET_BITSLICE(buf[5],
			 BMA280_ACC_Z_MSB) << BMA280_ACC_Z_LSB__LEN);
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] <<
			 (sizeof(short) * 8 - (BMA280_ACC_Z_LSB__LEN
			 + BMA280_ACC_Z_MSB__LEN));
			data[BMA2x2_AXIS_Z] = data[BMA2x2_AXIS_Z] >>
			 (sizeof(short) * 8 - (BMA280_ACC_Z_LSB__LEN
			 + BMA280_ACC_Z_MSB__LEN));
		}

#ifdef CONFIG_BMA2x2_LOWPASS
		if (atomic_read(&priv->filter)) {
			if (atomic_read(&priv->fir_en) &&
			 !atomic_read(&priv->suspend)) {
				int idx, firlen = atomic_read(&priv->firlen);
				num = priv->fir.num;
				if (priv->fir.num < firlen) {
					priv->fir.raw[num][BMA2x2_AXIS_X]
					 = data[BMA2x2_AXIS_X];
					priv->fir.raw[num][BMA2x2_AXIS_Y]
					 = data[BMA2x2_AXIS_Y];
					priv->fir.raw[num][BMA2x2_AXIS_Z]
					 = data[BMA2x2_AXIS_Z];
					priv->fir.sum[BMA2x2_AXIS_X] +=
					 data[BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] +=
					 data[BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] +=
					 data[BMA2x2_AXIS_Z];
					if (atomic_read(&priv->trace) &
					 BMA_TRC_FILTER) {
						GSE_LOG(
						"add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n",
						 priv->fir.num,
						 priv->fir.raw[num][X],
						 priv->fir.raw[num][Y],
						 priv->fir.raw[num][Z],
						 priv->fir.sum[BMA2x2_AXIS_X],
						 priv->fir.sum[BMA2x2_AXIS_Y],
						 priv->fir.sum[BMA2x2_AXIS_Z]);
					}
					priv->fir.num++;
					priv->fir.idx++;
				} else {
					idx = priv->fir.idx % firlen;
					priv->fir.sum[BMA2x2_AXIS_X] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] -=
					 priv->fir.raw[idx][BMA2x2_AXIS_Z];
					priv->fir.raw[idx][BMA2x2_AXIS_X]
					 = data[BMA2x2_AXIS_X];
					priv->fir.raw[idx][BMA2x2_AXIS_Y]
					 = data[BMA2x2_AXIS_Y];
					priv->fir.raw[idx][BMA2x2_AXIS_Z]
					 = data[BMA2x2_AXIS_Z];
					priv->fir.sum[BMA2x2_AXIS_X] +=
					 data[BMA2x2_AXIS_X];
					priv->fir.sum[BMA2x2_AXIS_Y] +=
					 data[BMA2x2_AXIS_Y];
					priv->fir.sum[BMA2x2_AXIS_Z] +=
					 data[BMA2x2_AXIS_Z];
					priv->fir.idx++;
					data[BMA2x2_AXIS_X] =
					 priv->fir.sum[BMA2x2_AXIS_X]/firlen;
					data[BMA2x2_AXIS_Y] =
					 priv->fir.sum[BMA2x2_AXIS_Y]/firlen;
					data[BMA2x2_AXIS_Z] =
					 priv->fir.sum[BMA2x2_AXIS_Z]/firlen;
					if (atomic_read(&priv->trace) &
					 BMA_TRC_FILTER) {
						GSE_LOG(
						"[%2d][%5d %5d %5d][%5d %5d %5d]:[%5d %5d %5d]\n",
						 idx,
						 priv->fir.raw[idx][X],
						 priv->fir.raw[idx][Y],
						 priv->fir.raw[idx][Z],
						 priv->fir.sum[BMA2x2_AXIS_X],
						 priv->fir.sum[BMA2x2_AXIS_Y],
						 priv->fir.sum[BMA2x2_AXIS_Z],
						 data[BMA2x2_AXIS_X],
						 data[BMA2x2_AXIS_Y],
						 data[BMA2x2_AXIS_Z]);
					}
				}
			}
		}
#endif
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadOffset(struct i2c_client *client, s8 ofs[BMA2x2_AXES_NUM])
{
	int err = 0;
#ifndef SW_CALIBRATION
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#endif
#ifdef SW_CALIBRATION
	ofs[0] = ofs[1] = ofs[2] = 0x0;
#else
	mutex_lock(&obj->lock);
	err = bma_i2c_read_block(client, BMA2x2_REG_OFSX, ofs, BMA2x2_AXES_NUM);
	mutex_unlock(&obj->lock);
	if (err)
		GSE_ERR("error: %d\n", err);

#endif
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ResetCalibration(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;

	#ifdef SW_CALIBRATION
	#else
	err = bma_i2c_write_block(client, BMA2x2_REG_OFSX, ofs, 4);
	if (err)
		GSE_ERR("error: %d\n", err);

	#endif
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadCalibration(struct i2c_client *client,
	 int dat[BMA2x2_AXES_NUM])
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int mul;

	#ifdef SW_CALIBRATION
	/*only SW Calibration, disable HW Calibration*/
	mul = 0;
	#else
	err = BMA2x2_ReadOffset(client, obj->offset);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}
	mul = obj->reso->sensitivity/bma2x2_offset_resolution.sensitivity;
	#endif

	dat[obj->cvt.map[BMA2x2_AXIS_X]] =
	 obj->cvt.sign[BMA2x2_AXIS_X] *
	 (obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X]);
	dat[obj->cvt.map[BMA2x2_AXIS_Y]] =
	 obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y]);
	dat[obj->cvt.map[BMA2x2_AXIS_Z]] =
	 obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z]);
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadCalibrationEx(struct i2c_client *client,
	 int act[BMA2x2_AXES_NUM], int raw[BMA2x2_AXES_NUM])
{
	/*raw: the raw calibration data; act: the actual calibration data*/
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
#ifndef SW_CALIBRATION
	int err = 0;
#endif
	int mul;

	#ifdef SW_CALIBRATION
		/*only SW Calibration, disable HW Calibration*/
		mul = 0;
	#else
		err = BMA2x2_ReadOffset(client, obj->offset);
		if (err) {
			GSE_ERR("read offset fail, %d\n", err);
			return err;
		}
		mul = obj->reso->sensitivity /
		 bma2x2_offset_resolution.sensitivity;
	#endif

	raw[BMA2x2_AXIS_X] =
	 obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X];
	raw[BMA2x2_AXIS_Y] =
	 obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y];
	raw[BMA2x2_AXIS_Z] =
	 obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z];

	act[obj->cvt.map[BMA2x2_AXIS_X]] =
	 obj->cvt.sign[BMA2x2_AXIS_X]*raw[BMA2x2_AXIS_X];
	act[obj->cvt.map[BMA2x2_AXIS_Y]] =
	 obj->cvt.sign[BMA2x2_AXIS_Y]*raw[BMA2x2_AXIS_Y];
	act[obj->cvt.map[BMA2x2_AXIS_Z]] =
	 obj->cvt.sign[BMA2x2_AXIS_Z]*raw[BMA2x2_AXIS_Z];
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_WriteCalibration(struct i2c_client *client,
	 int dat[BMA2x2_AXES_NUM])
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	int cali[BMA2x2_AXES_NUM], raw[BMA2x2_AXES_NUM];
	err = BMA2x2_ReadCalibrationEx(client, cali, raw);
	if (err) {
		GSE_ERR("read offset fail, %d\n", err);
		return err;
	}

	GSE_LOG(
	 "OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	 raw[BMA2x2_AXIS_X], raw[BMA2x2_AXIS_Y], raw[BMA2x2_AXIS_Z],
	 obj->offset[BMA2x2_AXIS_X],
	 obj->offset[BMA2x2_AXIS_Y],
	 obj->offset[BMA2x2_AXIS_Z],
	 obj->cali_sw[BMA2x2_AXIS_X],
	 obj->cali_sw[BMA2x2_AXIS_Y],
	 obj->cali_sw[BMA2x2_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	cali[BMA2x2_AXIS_X] += dat[BMA2x2_AXIS_X];
	cali[BMA2x2_AXIS_Y] += dat[BMA2x2_AXIS_Y];
	cali[BMA2x2_AXIS_Z] += dat[BMA2x2_AXIS_Z];

	GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n",
	dat[BMA2x2_AXIS_X], dat[BMA2x2_AXIS_Y], dat[BMA2x2_AXIS_Z]);

#ifdef SW_CALIBRATION
	obj->cali_sw[BMA2x2_AXIS_X] = obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]]);
	obj->cali_sw[BMA2x2_AXIS_Y] = obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]]);
	obj->cali_sw[BMA2x2_AXIS_Z] = obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]]);
#else
	obj->offset[BMA2x2_AXIS_X] = (s8)(obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]])/(divisor));
	obj->offset[BMA2x2_AXIS_Y] = (s8)(obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]])/(divisor));
	obj->offset[BMA2x2_AXIS_Z] = (s8)(obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]])/(divisor));

	/*convert software calibration using standard calibration*/
	obj->cali_sw[BMA2x2_AXIS_X] = obj->cvt.sign[BMA2x2_AXIS_X] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_X]])%(divisor);
	obj->cali_sw[BMA2x2_AXIS_Y] = obj->cvt.sign[BMA2x2_AXIS_Y] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Y]])%(divisor);
	obj->cali_sw[BMA2x2_AXIS_Z] = obj->cvt.sign[BMA2x2_AXIS_Z] *
	 (cali[obj->cvt.map[BMA2x2_AXIS_Z]])%(divisor);

	GSE_LOG(
	"NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
	obj->offset[BMA2x2_AXIS_X]*divisor + obj->cali_sw[BMA2x2_AXIS_X],
	obj->offset[BMA2x2_AXIS_Y]*divisor + obj->cali_sw[BMA2x2_AXIS_Y],
	obj->offset[BMA2x2_AXIS_Z]*divisor + obj->cali_sw[BMA2x2_AXIS_Z],
	obj->offset[BMA2x2_AXIS_X],
	 obj->offset[BMA2x2_AXIS_Y],
	 obj->offset[BMA2x2_AXIS_Z],
	obj->cali_sw[BMA2x2_AXIS_X],
	 obj->cali_sw[BMA2x2_AXIS_Y],
	 obj->cali_sw[BMA2x2_AXIS_Z]);
	err = bma_i2c_write_block(obj->client,
	 BMA2x2_REG_OFSX, obj->offset, BMA2x2_AXES_NUM);
	if (err) {
		GSE_ERR("write offset fail: %d\n", err);
		return err;
	}
#endif
	return err;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_CheckDeviceID(struct i2c_client *client)
{
	u8 data;
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client, BMA2x2_REG_DEVID, &data, 0x01);
	res = bma_i2c_read_block(client, BMA2x2_REG_DEVID, &data, 0x01);
	mutex_unlock(&obj->lock);
	if (res < 0)
		goto exit_BMA2x2_CheckDeviceID;

	switch (data) {
	case BMA222E_CHIPID:
		CHIP_TYPE = BMA222E_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA250E_CHIPID:
		CHIP_TYPE = BMA250E_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA255_CHIPID:
		CHIP_TYPE = BMA255_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	case BMA280_CHIPID:
		CHIP_TYPE = BMA280_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d pass!\n ", data);
		break;
	default:
		CHIP_TYPE = UNKNOWN_TYPE;
		GSE_LOG("BMA2x2_CheckDeviceID %d failt!\n ", data);
		break;
	}

exit_BMA2x2_CheckDeviceID:
	if (res < 0)
		return BMA2x2_ERR_I2C;

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
int BMA2x2_SetPowerMode(struct i2c_client *client, bool enable) //tuwenzan@wind-mobi.com modify at 20161128
{
	u8 databuf[2] = {0};
	int res = 0;
	u8 temp, temp0, temp1;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	u8 actual_power_mode = 0;
	int count = 0;
	if (enable == sensor_power) {
		GSE_LOG("Sensor power status is newest!\n");
		return BMA2x2_SUCCESS;
	}

	mutex_lock(&obj->lock);
	if (enable == true)
		actual_power_mode = BMA2x2_MODE_NORMAL;
	else
		actual_power_mode = BMA2x2_MODE_SUSPEND;

	res = bma_i2c_read_block(client, 0x3E, &temp, 0x1);
	udelay(1000);
	if (res < 0)
		GSE_LOG("read  config failed!\n");
	switch (actual_power_mode) {
	case BMA2x2_MODE_NORMAL:
		databuf[0] = 0x00;
		databuf[1] = 0x00;
		while (count < 10) {
			res = bma_i2c_write_block(client,
				BMA2x2_MODE_CTRL_REG, &databuf[0], 1);
			udelay(1000);
			if (res < 0)
				GSE_LOG("write MODE_CTRL_REG failed!\n");
			res = bma_i2c_write_block(client,
				BMA2x2_LOW_POWER_CTRL_REG, &databuf[1], 1);
			udelay(2000);
			if (res < 0)
				GSE_LOG("write LOW_POWER_CTRL_REG failed!\n");
			res =
			 bma_i2c_read_block(client,
			 BMA2x2_MODE_CTRL_REG, &temp0, 0x1);
			if (res < 0)
				GSE_LOG("read MODE_CTRL_REG failed!\n");
			res =
			 bma_i2c_read_block(client,
			 BMA2x2_LOW_POWER_CTRL_REG, &temp1, 0x1);
			if (res < 0)
				GSE_LOG("read LOW_POWER_CTRL_REG failed!\n");
			if (temp0 != databuf[0]) {
				GSE_LOG("readback MODE_CTRL failed!\n");
				count++;
				continue;
			} else if (temp1 != databuf[1]) {
				GSE_LOG("readback LOW_POWER_CTRL failed!\n");
				count++;
				continue;
			} else {
				GSE_LOG("configure powermode success\n");
				break;
			}
		}
		udelay(1000);
		break;
	case BMA2x2_MODE_SUSPEND:
		databuf[0] = 0x80;
		databuf[1] = 0x00;
		while (count < 10) {
			res = bma_i2c_write_block(client,
				BMA2x2_LOW_POWER_CTRL_REG, &databuf[1], 1);
			udelay(1000);
			if (res < 0)
				GSE_LOG("write LOW_POWER_CTRL_REG failed!\n");
			res = bma_i2c_write_block(client,
				BMA2x2_MODE_CTRL_REG, &databuf[0], 1);
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(1000);
			res = bma_i2c_write_block(client, 0x3E, &temp, 0x1);
			if (res < 0)
				GSE_LOG("write  config failed!\n");
			udelay(2000);
			if (res < 0)
				GSE_LOG("write BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_MODE_CTRL_REG, &temp0, 0x1);
			if (res < 0)
				GSE_LOG("read BMA2x2_MODE_CTRL_REG failed!\n");
			res =
			bma_i2c_read_block(client,
			 BMA2x2_LOW_POWER_CTRL_REG, &temp1, 0x1);
			if (res < 0)
				GSE_LOG("read BLOW_POWER_CTRL_REG failed!\n");
			if (temp0 != databuf[0]) {
				GSE_LOG("readback MODE_CTRL failed!\n");
				count++;
				continue;
			} else if (temp1 != databuf[1]) {
				GSE_LOG("readback LOW_POWER_CTRL failed!\n");
				count++;
				continue;
			} else {
				GSE_LOG("configure powermode success\n");
				break;
			}
		}
		udelay(1000);
	break;
	}

	if (res < 0) {
		GSE_ERR("set power mode failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	sensor_power = enable;
	mutex_unlock(&obj->lock);
	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	u8 databuf[2] = {0};
	int res = 0;

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
	BMA2x2_RANGE_SEL_REG, &databuf[0], 1);
	databuf[0] = BMA2x2_SET_BITSLICE(databuf[0],
	 BMA2x2_RANGE_SEL, dataformat);
	res += bma_i2c_write_block(client,
	 BMA2x2_RANGE_SEL_REG, &databuf[0], 1);
	udelay(500);

	if (res < 0) {
		GSE_ERR("set data format failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	mutex_unlock(&obj->lock);
	return BMA2x2_SetDataResolution(obj, dataformat);
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_SetBWRate(struct i2c_client *client, u8 bwrate)
{
	u8 databuf[2] = {0};
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	mutex_lock(&obj->lock);
	res = bma_i2c_read_block(client,
	 BMA2x2_BANDWIDTH__REG, &databuf[0], 1);
	databuf[0] = BMA2x2_SET_BITSLICE(databuf[0],
	 BMA2x2_BANDWIDTH, bwrate);
	res += bma_i2c_write_block(client,
	 BMA2x2_BANDWIDTH__REG, &databuf[0], 1);
	udelay(500);

	if (res < 0) {
		GSE_ERR("set bandwidth failed, res = %d\n", res);
		mutex_unlock(&obj->lock);
		return BMA2x2_ERR_I2C;
	}
	mutex_unlock(&obj->lock);

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_SetIntEnable(struct i2c_client *client, u8 intenable)
{
	int res = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	mutex_lock(&obj->lock);
	res = bma_i2c_write_block(client, BMA2x2_INT_REG_1, &intenable, 0x01);
	udelay(500);
	if (res != BMA2x2_SUCCESS) {
		mutex_unlock(&obj->lock);
		return res;
	}

	res = bma_i2c_write_block(client, BMA2x2_INT_REG_2, &intenable, 0x01);
	udelay(500);
	if (res != BMA2x2_SUCCESS) {
		mutex_unlock(&obj->lock);
		return res;
	}
	mutex_unlock(&obj->lock);
	GSE_LOG("BMA255 disable interrupt ...\n");
	return BMA2x2_SUCCESS;
}

static int BMA2x2_DisableTempSensor(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 temp = 0;
	mutex_lock(&obj->lock);
	temp = 0xCA;
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	temp = 0x00;
	res += bma_i2c_write_block(client,
		0x4F, &temp, 1);
	udelay(1000);
	temp = 0x0A;
	res += bma_i2c_write_block(client,
		0x35, &temp, 1);
	udelay(1000);
	mutex_unlock(&obj->lock);
	return res;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_init_client(struct i2c_client *client, int reset_cali)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int res = 0;

	GSE_LOG("bma2x2_init_client\n");
	/*should add normal mode setting*/
	res = BMA2x2_SetPowerMode(client, true);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetPowerMode to normal OK!\n");

	res = BMA2x2_DisableTempSensor(client);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("temperature sensor disabled!\n");

	res = BMA2x2_CheckDeviceID(client);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_CheckDeviceID ok\n");

	res = BMA2x2_SetBWRate(client, BMA2x2_BW_100HZ);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetBWRate OK!\n");

	res = BMA2x2_SetDataFormat(client, BMA2x2_RANGE_4G);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetDataFormat OK!\n");

	gsensor_gain.x = gsensor_gain.y =
	 gsensor_gain.z = obj->reso->sensitivity;

	res = BMA2x2_SetIntEnable(client, 0x00);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA255 disable interrupt function!\n");

	res = BMA2x2_SetPowerMode(client, false);
	if (res != BMA2x2_SUCCESS)
		return res;

	GSE_LOG("BMA2x2_SetPowerMode to suspend OK!\n");

	if (0 != reset_cali) {
		/*reset calibration only in power on*/
		res = BMA2x2_ResetCalibration(client);
		if (res != BMA2x2_SUCCESS)
			return res;
	}

	GSE_LOG("bma2x2_init_client OK!\n");
#ifdef CONFIG_BMA2x2_LOWPASS
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif

	msleep(20);

	return BMA2x2_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadChipInfo(struct i2c_client *client,
	 char *buf, int bufsize)
{
	u8 databuf[10];

	memset(databuf, 0, sizeof(u8)*10);

	if ((NULL == buf) || (bufsize <= 30))
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	switch (CHIP_TYPE) {
	case BMA222E_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA222E Chip");
		break;
	case BMA250E_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA250E Chip");
		break;
	case BMA255_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA255 Chip");
		break;
	case BMA280_TYPE:
		snprintf(buf, BMA2x2_BUFSIZE, "BMA280 Chip");
		break;
	default:
		snprintf(buf, BMA2x2_BUFSIZE, "UNKOWN Chip");
		break;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_CompassReadData(struct i2c_client *client,
	 char *buf, int bufsize)
{
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	int acc[BMA2x2_AXES_NUM];
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (NULL == buf)
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	if (sensor_power == false) {
		res = BMA2x2_SetPowerMode(client, true);
		if (res)
			GSE_ERR("Power on bma255 error %d!\n", res);

	}
	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return BMA2x2_ERR_STATUS;
	} else {
		/* Add compensated value performed by MTK calibration process*/
		databuf[BMA2x2_AXIS_X] += obj->cali_sw[BMA2x2_AXIS_X];
		databuf[BMA2x2_AXIS_Y] += obj->cali_sw[BMA2x2_AXIS_Y];
		databuf[BMA2x2_AXIS_Z] += obj->cali_sw[BMA2x2_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[BMA2x2_AXIS_X]] =
		 obj->cvt.sign[BMA2x2_AXIS_X]*databuf[BMA2x2_AXIS_X];
		acc[obj->cvt.map[BMA2x2_AXIS_Y]] =
		 obj->cvt.sign[BMA2x2_AXIS_Y]*databuf[BMA2x2_AXIS_Y];
		acc[obj->cvt.map[BMA2x2_AXIS_Z]] =
		 obj->cvt.sign[BMA2x2_AXIS_Z]*databuf[BMA2x2_AXIS_Z];

		snprintf(buf, BMA2x2_BUFSIZE, "%d %d %d",
		 (s16)acc[BMA2x2_AXIS_X],
		 (s16)acc[BMA2x2_AXIS_Y],
		 (s16)acc[BMA2x2_AXIS_Z]);
		if (atomic_read(&obj->trace) & BMA_TRC_IOCTL)
			GSE_LOG("gsensor data for compass: %s!\n", buf);

	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadSensorData(struct i2c_client *client,
	 char *buf, int bufsize)
{
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	int acc[BMA2x2_AXES_NUM];
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (NULL == buf)
		return BMA2x2_ERR_I2C;

	if (NULL == client) {
		*buf = 0;
		return BMA2x2_ERR_CLIENT;
	}

	if (sensor_power == false) {
		res = BMA2x2_SetPowerMode(client, true);
		if (res)
			GSE_ERR("Power on bma255 error %d!\n", res);
	}
	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return BMA2x2_ERR_STATUS;
	} else {
		databuf[BMA2x2_AXIS_X] += obj->cali_sw[BMA2x2_AXIS_X];
		databuf[BMA2x2_AXIS_Y] += obj->cali_sw[BMA2x2_AXIS_Y];
		databuf[BMA2x2_AXIS_Z] += obj->cali_sw[BMA2x2_AXIS_Z];

		/*remap coordinate*/
		acc[obj->cvt.map[BMA2x2_AXIS_X]] =
		 obj->cvt.sign[BMA2x2_AXIS_X]*databuf[BMA2x2_AXIS_X];
		acc[obj->cvt.map[BMA2x2_AXIS_Y]] =
		 obj->cvt.sign[BMA2x2_AXIS_Y]*databuf[BMA2x2_AXIS_Y];
		acc[obj->cvt.map[BMA2x2_AXIS_Z]] =
		 obj->cvt.sign[BMA2x2_AXIS_Z]*databuf[BMA2x2_AXIS_Z];

		acc[BMA2x2_AXIS_X] =
		 acc[BMA2x2_AXIS_X] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;
		acc[BMA2x2_AXIS_Y] =
		 acc[BMA2x2_AXIS_Y] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;
		acc[BMA2x2_AXIS_Z] =
		 acc[BMA2x2_AXIS_Z] * GRAVITY_EARTH_1000
		 / obj->reso->sensitivity;

	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int BMA2x2_ReadRawData(struct i2c_client *client, char *buf)
{
	int res = 0;
	s16 databuf[BMA2x2_AXES_NUM];

	if (!buf || !client)
		return -EINVAL;

	res = BMA2x2_ReadData(client, databuf);
	if (res) {
		GSE_ERR("I2C error: ret value=%d", res);
		return -EIO;
	} else {
		snprintf(buf, BMA2x2_BUFSIZE,
		 "BMA2x2_ReadRawData %04x %04x %04x", databuf[BMA2x2_AXIS_X],
		 databuf[BMA2x2_AXIS_Y], databuf[BMA2x2_AXIS_Z]);
	}
	return 0;
}
int bma2x2_softreset(void)
{
	unsigned char temp = BMA2x2_SOFTRESET;
	int ret = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	mutex_lock(&obj->lock);
	ret = bma_i2c_write_block(bma2x2_i2c_client,
		BMA2x2_SOFTRESET_REG, &temp, 1);
	mutex_unlock(&obj->lock);
	msleep(30);
	if (ret < 0)
		GSE_ERR("bma2x2_softreset failed");
	return ret;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_mode(struct i2c_client *client, unsigned char *mode)
{
	int comres = 0;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;
	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_EN_LOW_POWER__REG, mode, 1);
	mutex_unlock(&obj->lock);
	*mode  = (*mode) >> 6;

	return comres;
}

/*----------------------------------------------------------------------------*/
int bma2x2_set_range(struct i2c_client *client, unsigned char range) //tuwenzan@wind-mobi.com modify at 20161128
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_RANGE_SEL__REG};
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
			BMA2x2_RANGE_SEL__REG, data+1, 1);

	data[1]  = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_RANGE_SEL, range);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else {
		BMA2x2_SetDataResolution(obj, range);
		return comres;
	}

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_range(struct i2c_client *client, unsigned char *range)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_RANGE_SEL__REG, &data, 1);
	mutex_unlock(&obj->lock);
	*range = BMA2x2_GET_BITSLICE(data, BMA2x2_RANGE_SEL);

	return comres;
}
/*----------------------------------------------------------------------------*/
int bma2x2_set_bandwidth(struct i2c_client *client,unsigned char bandwidth)  //tuwenzan@wind-mobi.com modify at 20161128
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_BANDWIDTH__REG};
	struct bma2x2_i2c_data *obj =
	 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL)
		return BMA2x2_ERR_I2C;


	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_BANDWIDTH__REG, data+1, 1);

	data[1] = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_BANDWIDTH, bandwidth);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else
		return comres;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_bandwidth(struct i2c_client *client,
	 unsigned char *bandwidth)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_BANDWIDTH__REG, &data, 1);
	mutex_unlock(&obj->lock);
	data = BMA2x2_GET_BITSLICE(data, BMA2x2_BANDWIDTH);

	if (data < 0x08) {
		/*7.81Hz*/
		*bandwidth = 0x08;
	} else if (data > 0x0f) {
		/* 1000Hz*/
		*bandwidth = 0x0f;
	} else {
		*bandwidth = data;
	}
	return comres;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_set_fifo_mode(struct i2c_client *client,
	 unsigned char fifo_mode)
{
	int comres = 0;
	unsigned char data[2] = {BMA2x2_FIFO_MODE__REG};
	struct bma2x2_i2c_data *obj =
	(struct bma2x2_i2c_data *)i2c_get_clientdata(client);

	if (client == NULL || fifo_mode >= 4)
		return BMA2x2_ERR_I2C;


	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client,
	 BMA2x2_FIFO_MODE__REG, data+1, 1);

	data[1]  = BMA2x2_SET_BITSLICE(data[1],
	 BMA2x2_FIFO_MODE, fifo_mode);

	comres = i2c_master_send(client, data, 2);
	mutex_unlock(&obj->lock);
	if (comres <= 0)
		return BMA2x2_ERR_I2C;
	else
		return comres;

}
/*----------------------------------------------------------------------------*/
static int bma2x2_get_fifo_mode(struct i2c_client *client,
	 unsigned char *fifo_mode)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_FIFO_MODE__REG, &data, 1);
	mutex_unlock(&obj->lock);
	*fifo_mode = BMA2x2_GET_BITSLICE(data, BMA2x2_FIFO_MODE);

	return comres;
}

static int bma2x2_get_fifo_framecount(struct i2c_client *client,
	 unsigned char *framecount)
{
	int comres = 0;
	unsigned char data;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (client == NULL)
		return BMA2x2_ERR_I2C;

	mutex_lock(&obj->lock);
	comres = bma_i2c_read_block(client, BMA2x2_FIFO_FRAME_COUNTER_S__REG,
	 &data, 1);
	mutex_unlock(&obj->lock);
	*framecount = BMA2x2_GET_BITSLICE(data, BMA2x2_FIFO_FRAME_COUNTER_S);
	return comres;
}

/*----------------------------------------------------------------------------*/

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	BMA2x2_ReadChipInfo(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}


/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t show_cpsopmode_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_mode(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor opmode for compass tilt compensation
*/
static ssize_t store_cpsopmode_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (data == BMA2x2_MODE_NORMAL)
		BMA2x2_SetPowerMode(bma2x2_i2c_client, true);
	else if (data == BMA2x2_MODE_SUSPEND)
		BMA2x2_SetPowerMode(bma2x2_i2c_client, false);
	else
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t show_cpsrange_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_range(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor range for compass tilt compensation
*/
static ssize_t store_cpsrange_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_range(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}
/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t show_cpsbandwidth_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_bandwidth(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
/*
g sensor bandwidth for compass tilt compensation
*/
static ssize_t store_cpsbandwidth_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_bandwidth(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);


	return count;
}

/*----------------------------------------------------------------------------*/
/*
g sensor data for compass tilt compensation
*/
static ssize_t show_cpsdata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA2x2_CompassReadData(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	char strbuf[BMA2x2_BUFSIZE];
	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}
	BMA2x2_ReadSensorData(client, strbuf, BMA2x2_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj;
	int err, len = 0, mul;
	int tmp[BMA2x2_AXES_NUM];

	if (NULL == client) {
		GSE_ERR("i2c client is null!!\n");
		return 0;
	}

	obj = i2c_get_clientdata(client);
	err = BMA2x2_ReadOffset(client, obj->offset);
	if (err)
		return -EINVAL;

	err = BMA2x2_ReadCalibration(client, tmp);
	if (err)
		return -EINVAL;
	else {
		mul = obj->reso->sensitivity
		 / bma2x2_offset_resolution.sensitivity;
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
		 mul,
		 obj->offset[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z],
		 obj->offset[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z]);
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		 obj->cali_sw[BMA2x2_AXIS_X],
		 obj->cali_sw[BMA2x2_AXIS_Y],
		 obj->cali_sw[BMA2x2_AXIS_Z]);

		len += snprintf(buf+len, PAGE_SIZE-len,
		 "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		 obj->offset[BMA2x2_AXIS_X]*mul + obj->cali_sw[BMA2x2_AXIS_X],
		 obj->offset[BMA2x2_AXIS_Y]*mul + obj->cali_sw[BMA2x2_AXIS_Y],
		 obj->offset[BMA2x2_AXIS_Z]*mul + obj->cali_sw[BMA2x2_AXIS_Z],
		 tmp[BMA2x2_AXIS_X], tmp[BMA2x2_AXIS_Y], tmp[BMA2x2_AXIS_Z]);
		return len;
	}
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	struct i2c_client *client = bma2x2_i2c_client;
	int err, x, y, z;
	int dat[BMA2x2_AXES_NUM];

	if (!strncmp(buf, "rst", 3)) {
		err = BMA2x2_ResetCalibration(client);
		if (err)
			GSE_ERR("reset offset err = %d\n", err);

	} else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
		dat[BMA2x2_AXIS_X] = x;
		dat[BMA2x2_AXIS_Y] = y;
		dat[BMA2x2_AXIS_Z] = z;
		err = BMA2x2_WriteCalibration(client, dat);
		if (err)
			GSE_ERR("write calibration err = %d\n", err);

	} else
		GSE_ERR("invalid format\n");

	return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMA2x2_LOWPASS
	int X = BMA2x2_AXIS_X;
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);
		GSE_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GSE_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][X],
			 obj->fir.raw[idx][BMA2x2_AXIS_Y],
			 obj->fir.raw[idx][BMA2x2_AXIS_Z]);
		}

		GSE_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[BMA2x2_AXIS_X],
		 obj->fir.sum[BMA2x2_AXIS_Y],
		 obj->fir.sum[BMA2x2_AXIS_Z]);
		GSE_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[X]/len,
		 obj->fir.sum[BMA2x2_AXIS_Y]/len,
		 obj->fir.sum[BMA2x2_AXIS_Z]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
#ifdef CONFIG_BMA2x2_LOWPASS
	struct i2c_client *client = bma2x2_i2c_client;
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int firlen;

	if (1 != sscanf(buf, "%11d", &firlen)) {
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
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device_driver *ddri,
	 char *buf)
{
	ssize_t res;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	int trace;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}
	if (1 == sscanf(buf, "0x%11x", &trace))
		atomic_set(&obj->trace, trace);
	else
		GSE_ERR("invalid content: '%s'\n", buf);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (obj == NULL) {
		GSE_ERR("i2c_data obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf+len, PAGE_SIZE-len,
		 "CUST: %d %d (%d %d)\n",
		 obj->hw->i2c_num,
		 obj->hw->direction,
		 obj->hw->power_id,
		 obj->hw->power_vol);
	} else
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_power_status_value(struct device_driver *ddri, char *buf)
{
	if (sensor_power)
		GSE_LOG("G sensor is in work mode, sensor_power = %d\n",
		 sensor_power);
	else
		GSE_LOG("G sensor is in standby mode, sensor_power = %d\n",
		 sensor_power);

	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_mode_value(struct device_driver *ddri, char *buf)
{
	unsigned char data;

	if (bma2x2_get_fifo_mode(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
static ssize_t store_fifo_mode_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	if (bma2x2_set_fifo_mode(bma2x2_i2c_client, (unsigned char) data) < 0)
		GSE_ERR("invalid content: '%s'\n", buf);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_framecount_value(struct device_driver *ddri,
	 char *buf)
{
	unsigned char data;

	if (bma2x2_get_fifo_framecount(bma2x2_i2c_client, &data) < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");
	else
		return snprintf(buf, PAGE_SIZE, "%d\n", data);

}

/*----------------------------------------------------------------------------*/
static ssize_t store_fifo_framecount_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;

	mutex_lock(&obj->lock);
	obj->fifo_count = (unsigned char)data;
	mutex_unlock(&obj->lock);
	return count;
}


/*----------------------------------------------------------------------------*/
static ssize_t show_fifo_data_out_frame_value(struct device_driver *ddri,
	 char *buf)
{
	unsigned char fifo_frame_cnt = 0;
	int err = 0;
	int read_cnt = 0;
	int num = 0;
	static char tmp_buf[32*6] = {0};
	char *pBuf = NULL;
	unsigned char f_len = 6;/* FIXME: ONLY USE 3-AXIS */
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(bma2x2_i2c_client);
	if (buf == NULL)
		return -EINVAL;


	err = bma2x2_get_fifo_framecount(bma2x2_i2c_client, &fifo_frame_cnt);
	if (err < 0)
		return snprintf(buf, PAGE_SIZE, "Read error\n");


	if (fifo_frame_cnt == 0) {
		GSE_ERR("fifo frame count is 0!!!");
		return 0;
	}
#ifdef READFIFOCOUNT
	read_cnt = fifo_frame_cnt;
	pBuf = &tmp_buf[0];
	mutex_lock(&obj->lock);
	while (read_cnt > 0) {
		if (bma_i2c_read_block(bma2x2_i2c_client,
		 BMA2x2_FIFO_DATA_OUTPUT_REG,
		 pBuf,
		 f_len) < 0) {
			GSE_ERR("[a]fatal error\n");
			num =
			 snprintf(buf, PAGE_SIZE, "Read byte block error\n");
			 return num;
		}
		pBuf += f_len;
		read_cnt--;
		GSE_ERR("fifo_frame_cnt %d, f_len %d", fifo_frame_cnt, f_len);
	}
	mutex_unlock(&obj->lock);
	memcpy(buf, tmp_buf, fifo_frame_cnt * f_len);
#else
	if (bma_i2c_dma_read(bma2x2_i2c_client,
	 BMA2x2_FIFO_DATA_OUTPUT_REG, buf,
	 fifo_frame_cnt * f_len) < 0) {
		GSE_ERR("[a]fatal error\n");
		return snprintf(buf, PAGE_SIZE, "Read byte block error\n");
	}
#endif
	return fifo_frame_cnt * f_len;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_registers(struct device_driver *ddri, char *buf)
{
	unsigned char temp = 0;
	int count = 0;
	int num = 0;
	struct bma2x2_i2c_data *obj = obj_i2c_data;
	if (buf == NULL)
		return -EINVAL;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	mutex_lock(&obj->lock);
	while (count <= MAX_REGISTER_MAP) {
		temp = 0;
		if (bma_i2c_read_block(bma2x2_i2c_client,
			count,
			&temp,
			1) < 0) {
			num =
			 snprintf(buf, PAGE_SIZE, "Read byte block error\n");
			return num;
		} else {
			num +=
			 snprintf(buf + num, PAGE_SIZE, "0x%x\n", temp);
		}
		count++;
	}
	mutex_unlock(&obj->lock);
	return num;
}

/*----------------------------------------------------------------------------*/
static ssize_t show_softreset(struct device_driver *ddri, char *buf)
{
	int ret = 0;
	int num = 0;
	if (buf == NULL)
		return -EINVAL;
	if (bma2x2_i2c_client == NULL)
		return -EINVAL;
	ret = bma2x2_softreset();
	if (ret < 0)
		num =
		snprintf(buf, PAGE_SIZE, "Write byte block error\n");
	else
		num =
		snprintf(buf, PAGE_SIZE, "softreset success\n");
	ret = bma2x2_init_client(bma2x2_i2c_client, 0);
	if (ret < 0)
		GSE_ERR("bma2x2_init_client failed\n");
	else
		GSE_ERR("bma2x2_init_client succeed\n");
	ret = BMA2x2_SetPowerMode(bma2x2_i2c_client, true);
	if (ret < 0)
		GSE_ERR("BMA2x2_SetPowerMode failed\n");
	else
		GSE_ERR("BMA2x2_SetPowerMode succeed\n");
	return num;

}

//tuwenzan@wind-mobi.com add at 20161128 begin
#ifdef BMA2XX_OFFSET_CALI
static ssize_t show_calibration_state(struct device_driver *ddri, char *buf)
{
	int len = 0;
	len = sprintf(buf,"%d",cali_flag);
	printk("tuwenzan cali_flag = %d\n",cali_flag);
	return len;
}

static ssize_t store_calibration_value(struct device_driver *ddri,
	 const char *buf, size_t count)
{
	int ret = 0;
	ret = simple_strtoul(buf,0,10);
	if(1 == ret){
		ret = bma2xx_offset_fast_cali(0,0,1);
		printk("tuwenzan ret = %d\n",ret);
		if(0 == ret){
			cali_flag = 1;
			printk("tuwenzan cali_flag = %d,ret = %d",cali_flag,ret);
		}else
			printk("calibration fail\n");
	}
	return count;
}
#endif
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(cpsdata, S_IWUSR | S_IRUGO, show_cpsdata_value, NULL);
static DRIVER_ATTR(cpsopmode, S_IWUSR | S_IRUGO, show_cpsopmode_value,
	 store_cpsopmode_value);
static DRIVER_ATTR(cpsrange, S_IWUSR | S_IRUGO, show_cpsrange_value,
	 store_cpsrange_value);
static DRIVER_ATTR(cpsbandwidth, S_IWUSR | S_IRUGO, show_cpsbandwidth_value,
	 store_cpsbandwidth_value);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO, show_firlen_value,
	 store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value,
	 store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(powerstatus, S_IRUGO, show_power_status_value, NULL);
static DRIVER_ATTR(fifo_mode, S_IWUSR | S_IRUGO, show_fifo_mode_value,
	 store_fifo_mode_value);
static DRIVER_ATTR(fifo_framecount, S_IWUSR |
	 S_IRUGO, show_fifo_framecount_value,
	 store_fifo_framecount_value);
static DRIVER_ATTR(fifo_data_frame, S_IRUGO, show_fifo_data_out_frame_value,
	 NULL);
static DRIVER_ATTR(dump_registers, S_IRUGO, show_registers, NULL);
static DRIVER_ATTR(softreset, S_IRUGO, show_softreset, NULL);
//tuwenzan@wind-mobi.com add at 20161128 begin
#ifdef BMA2XX_OFFSET_CALI
static DRIVER_ATTR(start_fast_calibration, 0644, show_calibration_state,store_calibration_value);
#endif
/*----------------------------------------------------------------------------*/
static struct driver_attribute *bma2x2_attr_list[] = {
	/*chip information*/
	 &driver_attr_chipinfo,
	/*dump sensor data*/
	 &driver_attr_sensordata,
	/*show calibration data*/
	 &driver_attr_cali,
	/*filter length: 0: disable, others: enable*/
	 &driver_attr_firlen,
	/*trace log*/
	 &driver_attr_trace,
	 &driver_attr_status,
	 &driver_attr_powerstatus,
	/*g sensor data for compass tilt compensation*/
	 &driver_attr_cpsdata,
	/*g sensor opmode for compass tilt compensation*/
	 &driver_attr_cpsopmode,
	/*g sensor range for compass tilt compensation*/
	 &driver_attr_cpsrange,
	/*g sensor bandwidth for compass tilt compensation*/
	 &driver_attr_cpsbandwidth,
	 &driver_attr_fifo_mode,
	 &driver_attr_fifo_framecount,
	 &driver_attr_fifo_data_frame,
	 &driver_attr_dump_registers,
	 &driver_attr_softreset,
#ifdef BMA2XX_OFFSET_CALI
	 &driver_attr_start_fast_calibration,
#endif
};
//tuwenzan@wind-mobi.com add at 20161128 end
/*----------------------------------------------------------------------------*/
static int bma2x2_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma2x2_attr_list)/sizeof(bma2x2_attr_list[0]));
	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bma2x2_attr_list[idx]);
		if (err) {
			GSE_ERR("driver_create_file (%s) = %d\n",
			 bma2x2_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(bma2x2_attr_list)/sizeof(bma2x2_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;


	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bma2x2_attr_list[idx]);


	return err;
}

/******************************************************************************
* Function Configuration
******************************************************************************/
static int bma2x2_open(struct inode *inode, struct file *file)
{
	file->private_data = bma2x2_i2c_client;

	if (file->private_data == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int bma2x2_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long bma2x2_unlocked_ioctl(struct file *file,
	 unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct bma2x2_i2c_data *obj =
		 (struct bma2x2_i2c_data *)i2c_get_clientdata(client);
	char strbuf[BMA2x2_BUFSIZE];
	void __user *data;
	struct SENSOR_DATA sensor_data;
	long err = 0;
	int cali[3];

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg,
		 _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg,
		 _IOC_SIZE(cmd));


	if (err) {
		GSE_ERR("access error: %08X, (%2d, %2d)\n",
		 cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch (cmd) {
	case GSENSOR_IOCTL_INIT:
		bma2x2_init_client(client, 0);
		break;

	case GSENSOR_IOCTL_READ_CHIPINFO:
		data = (void __user *) arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		BMA2x2_ReadChipInfo(client, strbuf, BMA2x2_BUFSIZE);
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
		BMA2x2_ReadSensorData(client, strbuf, BMA2x2_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
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

		if (copy_to_user(data, &gsensor_gain,
		 sizeof(struct GSENSOR_VECTOR3D))) {
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
		BMA2x2_ReadRawData(client, strbuf);
		if (copy_to_user(data, &strbuf, strlen(strbuf)+1)) {
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
			cali[BMA2x2_AXIS_X] =
			 sensor_data.x * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			cali[BMA2x2_AXIS_Y] =
			 sensor_data.y * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			cali[BMA2x2_AXIS_Z] =
			 sensor_data.z * obj->reso->sensitivity /
			 GRAVITY_EARTH_1000;
			err = BMA2x2_WriteCalibration(client, cali);
		}
		break;

	case GSENSOR_IOCTL_CLR_CALI:
		err = BMA2x2_ResetCalibration(client);
		break;

	case GSENSOR_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = BMA2x2_ReadCalibration(client, cali);
		if (err)
			break;


		sensor_data.x =
		 cali[BMA2x2_AXIS_X] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
		sensor_data.y =
		 cali[BMA2x2_AXIS_Y] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
		sensor_data.z =
		 cali[BMA2x2_AXIS_Z] * GRAVITY_EARTH_1000 /
		 obj->reso->sensitivity;
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
static const struct file_operations bma2x2_fops = {
	.open = bma2x2_open,
	.release = bma2x2_release,
	.unlocked_ioctl = bma2x2_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice bma2x2_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gsensor",
	.fops = &bma2x2_fops,
};
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_HAS_EARLYSUSPEND
/*----------------------------------------------------------------------------*/
static int bma2x2_suspend(struct i2c_client *client, pm_message_t msg)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	GSE_FUN();

	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GSE_ERR("null pointer!!\n");
			return -EINVAL;
		}
		atomic_set(&obj->suspend, 1);
		err = BMA2x2_SetPowerMode(obj->client, false);
		if (err) {
			GSE_ERR("write power control fail!!\n");
			return err;
		}
		BMA2x2_power(obj->hw, 0);
	}
	return err;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_resume(struct i2c_client *client)
{
	struct bma2x2_i2c_data *obj = i2c_get_clientdata(client);
	int err;
	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return -EINVAL;
	}

	BMA2x2_power(obj->hw, 1);
	err = bma2x2_init_client(client, 0);
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		return err;
	}

	atomic_set(&obj->suspend, 0);

	return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void bma2x2_early_suspend(struct early_suspend *h)
{
	struct bma2x2_i2c_data *obj =
	 container_of(h, struct bma2x2_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}
	atomic_set(&obj->suspend, 1);
	err = BMA2x2_SetPowerMode(obj->client, false);

	if (err) {
		GSE_ERR("write power control fail!!\n");
		return;
	}

	BMA2x2_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void bma2x2_late_resume(struct early_suspend *h)
{
	struct bma2x2_i2c_data *obj =
	 container_of(h, struct bma2x2_i2c_data, early_drv);
	int err;

	GSE_FUN();

	if (obj == NULL) {
		GSE_ERR("null pointer!!\n");
		return;
	}

	BMA2x2_power(obj->hw, 1);
	err = bma2x2_init_client(obj->client, 0);
	if (err) {
		GSE_ERR("initialize client fail!!\n");
		return;
	}
	atomic_set(&obj->suspend, 0);
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

/* if use  this typ of enable ,*/
/*Gsensor should report inputEvent(x, y, z ,stats, div) to HAL*/
static int bma2x2_open_report_data(int open)
{
/*should queuq work to report event if  is_report_input_direct=true*/
	return 0;
}

/* if use  this typ of enable ,*/
/*Gsensor only enabled but not report inputEvent to HAL*/
static int bma2x2_enable_nodata(int en)
{
	int err = 0;

	if (((en == 0) && (sensor_power == false))
	 || ((en == 1) && (sensor_power == true))) {
		GSE_LOG("Gsensor device have updated!\n");
	} else {
		err = BMA2x2_SetPowerMode(obj_i2c_data->client, !sensor_power);
	}

	return err;
}

static int bma2x2_set_delay(u64 ns)
{
	int err = 0;

	int value = (int)ns / 1000 / 1000;


	if (err != BMA2x2_SUCCESS)
		GSE_ERR("Set delay parameter error!\n");


	if (value >= 50)
		atomic_set(&obj_i2c_data->filter, 0);
	else {
#if defined(CONFIG_BMA2x2_LOWPASS)
	obj_i2c_data->fir.num = 0;
	obj_i2c_data->fir.idx = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_X] = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_Y] = 0;
	obj_i2c_data->fir.sum[BMA2x2_AXIS_Z] = 0;
	atomic_set(&obj_i2c_data->filter, 1);
#endif
	}

	return 0;
}

static int bma2x2_get_data(int *x , int *y, int *z, int *status)
{
	char buff[BMA2x2_BUFSIZE];
	/* use acc raw data for gsensor */
	BMA2x2_ReadSensorData(obj_i2c_data->client, buff, BMA2x2_BUFSIZE);

	sscanf(buff, "%11x %11x %11x", x, y, z);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}


/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_probe(struct i2c_client *client,
	 const struct i2c_device_id *id)
{
	struct i2c_client *new_client;
	struct bma2x2_i2c_data *obj;
	struct acc_control_path ctl = {0};
	struct acc_data_path data = {0};
	int err = 0;
	GSE_FUN();
	//printk("twz enter bma2x2 probe client addr = %x\n",client->addr);
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0, sizeof(struct bma2x2_i2c_data));

	obj->hw = hw;
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GSE_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	obj_i2c_data = obj;
	obj->client = client;
	bma2x2_client = client; //tuwenzan@wind-mobi.com add at 20161128
	new_client = obj->client;
	i2c_set_clientdata(new_client, obj);
#ifdef DMAREAD
	/*allocate DMA buffer*/
	//twz modify I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, DMA_BUFFER_SIZE,&I2CDMABuf_pa, GFP_KERNEL);
    client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	I2CDMABuf_va = (u8 *)dma_zalloc_coherent(&client->dev, DMA_BUFFER_SIZE,
	 &I2CDMABuf_pa, GFP_KERNEL);
	if (I2CDMABuf_va == NULL) {
		err = -ENOMEM;
		GSE_ERR("Allocate DMA I2C Buffer failed! error = %d\n", err);
	}
#endif
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	mutex_init(&obj->lock);

	mutex_init(&sensor_data_mutex);

#ifdef CONFIG_BMA2x2_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);


	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);


#endif

	bma2x2_i2c_client = new_client;
	err = bma2x2_init_client(new_client, 1);
	if (err)
		goto exit_init_failed;


	err = misc_register(&bma2x2_device);
	if (err) {
		GSE_ERR("bma2x2_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err =
	 bma2x2_create_attr(&(bma2x2_init_info.platform_diver_addr->driver));
	if (err) {
		GSE_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	ctl.open_report_data = bma2x2_open_report_data;
	ctl.enable_nodata = bma2x2_enable_nodata;
	ctl.set_delay  = bma2x2_set_delay;
	ctl.is_report_input_direct = false;

	err = acc_register_control_path(&ctl);
	if (err) {
		GSE_ERR("register acc control path err\n");
		goto exit_kfree;
	}

	data.get_data = bma2x2_get_data;
	data.vender_div = 1000;
	err = acc_register_data_path(&data);
	if (err) {
		GSE_ERR("register acc data path err\n");
		goto exit_kfree;
	}



#ifdef CONFIG_HAS_EARLYSUSPEND
	obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend  = bma2x2_early_suspend,
	obj->early_drv.resume   = bma2x2_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

	bma2x2_init_flag = 0;
	GSE_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&bma2x2_device);
exit_misc_device_register_failed:
exit_init_failed:
exit_kfree:
	kfree(obj);
exit:
	GSE_ERR("%s: err = %d\n", __func__, err);
	bma2x2_init_flag = -1;
	return err;
}

/*----------------------------------------------------------------------------*/
static int bma2x2_i2c_remove(struct i2c_client *client)
{
	int err = 0;
	err =
	 bma2x2_delete_attr(&(bma2x2_init_info.platform_diver_addr->driver));
	if (err)
		GSE_ERR("bma150_delete_attr fail: %d\n", err);

	err = misc_deregister(&bma2x2_device);
	if (err)
		GSE_ERR("misc_deregister fail: %d\n", err);

#ifdef DMAREAD
	if (I2CDMABuf_va != NULL) {
		dma_free_coherent(NULL,
		 DMA_BUFFER_SIZE, I2CDMABuf_va, I2CDMABuf_pa);
		I2CDMABuf_va = NULL;
	}
	I2CDMABuf_pa = 0;
#endif
	bma2x2_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_local_init(void)
{
	GSE_FUN();

	BMA2x2_power(hw, 1);
	if (i2c_add_driver(&bma2x2_i2c_driver)) {
		GSE_ERR("add driver error\n");
		return BMA2x2_ERR_I2C;
	}
	if (-1 == bma2x2_init_flag)
		return BMA2x2_ERR_I2C;

	return 0;
}
/*----------------------------------------------------------------------------*/
static int bma2x2_remove(void)
{
	GSE_FUN();
	BMA2x2_power(hw, 0);
	i2c_del_driver(&bma2x2_i2c_driver);
	return 0;
}

static struct acc_init_info bma2x2_init_info = {
	.name = "bma255",
	.init = bma2x2_local_init,
	.uninit = bma2x2_remove,
};

/*----------------------------------------------------------------------------*/
static int __init bma2x2_init(void)
{
	hw = get_accel_dts_func(COMPATIABLE_NAME, hw);

	GSE_FUN();
	#ifndef CONFIG_OF
	i2c_register_board_info(hw->i2c_num, &bma2x2_i2c_info, 1);
	#endif
	GSE_ERR("!!!!bma2x2_init i2c_register_board_info finishes\n");
	acc_driver_add(&bma2x2_init_info);
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit bma2x2_exit(void)
{
	GSE_FUN();
}
/*----------------------------------------------------------------------------*/
module_init(bma2x2_init);
module_exit(bma2x2_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BMA2x2 ACCELEROMETER SENSOR DRIVER");
MODULE_AUTHOR("contact@bosch-sensortec.com");
//tuwenzan@wind-mobi.com modify at 20161108 end

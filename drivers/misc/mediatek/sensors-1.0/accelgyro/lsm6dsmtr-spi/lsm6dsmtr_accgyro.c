/*
* Copyright (C) 2017 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

/* LSM6DSMSTR_ACC motion sensor driver
 *
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * VERSION: V1.
 * HISTORY: V1.0 --- Driver creation
 *          V1.1 --- Add share I2C address function
 *          V1.2 --- Fix the bug that sometimes sensor is stuck after system resume.
 *          V1.3 --- Add FIFO interfaces.
 *          V1.4 --- Use basic i2c function to read fifo data instead of i2c DMA mode.
 *          V1.5 --- Add compensated value performed by MTK acceleration calibration process.
 *          V1.6 --- Non SensorHub A+G uniform
 */
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/suspend.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include "accelgyro.h"
#include "cust_accelgyro.h"
#include "lsm6dsmtr_accgyro.h"

#include "linux/platform_data/spi-mt65xx.h"

#define SW_CALIBRATION
#define BYTES_PER_LINE		(16)
#define SRX_DEBUG

static int lsm6dsmtr_accelgyro_spi_probe(struct spi_device *spi);
static int lsm6dsmtr_accelgyro_spi_remove(struct spi_device *spi);
static int lsm6dsmtr_accelgyro_local_init(void);
static int lsm6dsmtr_accelgyro_remove(void);

struct scale_factor {
	u8 whole;
	u8 fraction;
};

struct data_resolution {
	struct scale_factor scalefactor;
	int sensitivity;
};

/* gyro sensor type */
enum SENSOR_TYPE_ENUM {
	LSM6DSMSTR_GYRO_TYPE = 0x0,
	INVALID_TYPE = 0xff
};

/* range */
enum BMG_RANGE_ENUM {
	BMG_RANGE_2000 = 0x0,	/* +/- 2000 degree/s */
	BMG_RANGE_1000,		/* +/- 1000 degree/s */
	BMG_RANGE_500,		/* +/- 500 degree/s */
	BMG_RANGE_250,		/* +/- 250 degree/s */
	BMG_RANGE_125,		/* +/- 125 degree/s */
	BMG_UNDEFINED_RANGE = 0xff
};

/* power mode */
enum BMG_POWERMODE_ENUM {
	BMG_SUSPEND_MODE = 0x0,
	BMG_NORMAL_MODE,
	BMG_UNDEFINED_POWERMODE = 0xff
};

/* debug information flags */
enum GYRO_TRC {
	GYRO_TRC_FILTER = 0x01,
	GYRO_TRC_RAWDATA = 0x02,
	GYRO_TRC_IOCTL = 0x04,
	GYRO_TRC_CALI = 0x08,
	GYRO_TRC_INFO = 0x10,
};

/* s/w data filter */
struct accel_data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
	int sum[BMG_AXES_NUM];
	int num;
	int idx;
};

struct gyro_data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
	int sum[BMG_AXES_NUM];
	int num;
	int idx;
};


struct lsm6dsmtr_accelgyro_data {
	struct accelgyro_hw *hw;
	struct hwmsen_convert cvt;
	struct lsm6dsmtr_t device;
	/*misc */
	struct data_resolution *accel_reso;
	atomic_t accel_trace;
	atomic_t accel_suspend;
	atomic_t accel_selftest;
	atomic_t accel_filter;
	s16 accel_cali_sw[LSM6DSMSTR_ACC_AXES_NUM + 1];
	struct accel_data_filter accel_fir;
	struct mutex accel_lock;
	struct mutex spi_lock;
	/* +1: for 4-byte alignment */
	s8 accel_offset[LSM6DSMSTR_ACC_AXES_NUM + 1];
	s16 accel_data[LSM6DSMSTR_ACC_AXES_NUM + 1];
	atomic_t accel_firlen;
	u8 fifo_count;

	u8 fifo_head_en;
	u8 fifo_data_sel;
	u16 fifo_bytecount;
	struct odr_t accel_odr;
	u64 fifo_time;
	atomic_t layout;

	bool is_input_enable;
	struct delayed_work work;
	struct input_dev *input;
	struct hrtimer timer;
	int is_timer_running;
	struct work_struct report_data_work;
	ktime_t work_delay_kt;
	uint64_t time_odr;
	struct mutex mutex_ring_buf;
	atomic_t wkqueue_en;
	atomic_t accel_delay;
	int accel_IRQ;
	uint16_t accel_gpio_pin;
	struct work_struct accel_irq_work;

	/* step detector */
	u8 std;
	/* spi */
	struct spi_device *spi;
	u8 *spi_buffer;		/* only used for SPI transfer internal */

	/* gyro sensor info */
	u8 gyro_sensor_name[MAX_SENSOR_NAME];
	enum SENSOR_TYPE_ENUM gyro_sensor_type;
	/* enum BMG_POWERMODE_ENUM gyro_power_mode; */
	enum BMG_RANGE_ENUM gyro_range;
	bool lsm6dsm_gyro_power;
	int gyro_datarate;
	/* sensitivity = 2^bitnum/range
	 * [+/-2000 = 4000; +/-1000 = 2000;
	 * +/-500 = 1000; +/-250 = 500;
	 * +/-125 = 250 ]
	 */
	struct data_resolution *gyro_reso;
	u16 gyro_sensitivity;
	struct odr_t gyro_odr;
	/*misc */
	struct mutex gyro_lock;
	atomic_t gyro_trace;
	atomic_t gyro_suspend;
	atomic_t gyro_filter;
	/* unmapped axis value */
	s16 gyro_cali_sw[BMG_AXES_NUM + 1];
	/* hw offset */
	s8 gyro_offset[BMG_AXES_NUM + 1];	/* +1:for 4-byte alignment */

#if defined(CONFIG_BMG_LOWPASS)
	atomic_t gyro_firlen;
	atomic_t gyro_fir_en;
	struct gyro_data_filter gyro_fir;
#endif

};

struct lsm6dsmtr_axis_data_t {
	s16 x;
	s16 y;
	s16 z;
};



static struct accelgyro_init_info lsm6dsmtr_accelgyro_init_info;
static struct lsm6dsmtr_accelgyro_data *accelgyro_obj_data;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;

/* signification motion flag*/
int sig_flag;

static int lsm6dsmtr_acc_init_flag = -1;
struct lsm6dsmtr_t *p_lsm6dsmtr;

struct accelgyro_hw accelgyro_cust;
static struct accelgyro_hw *hw = &accelgyro_cust;

#ifdef MTK_NEW_ARCH_ACCEL
struct platform_device *accelPltFmDev;
#endif

/* 0=OK, -1=fail */
static int lsm6dsmtr_gyro_init_flag = -1;
static struct lsm6dsmtr_accelgyro_data *gyro_obj_data;
static int bmg_set_powermode(struct lsm6dsmtr_accelgyro_data *obj, bool power_mode);

static struct data_resolution lsm6dsm_acc_data_resolution[] = {
	/* combination by {FULL_RES,RANGE} */
	{{0, 0}, 61},		/* 2g   1LSB=61 ug */
	{{0, 0}, 488},		/* 16g */
	{{0, 0}, 122},		/* 4g */
	{{0, 0}, 244},		/* 8g */
};

static struct data_resolution lsm6dsm_gyro_data_resolution[] = {
	/* combination by {FULL_RES,RANGE} */
	{{0, 0}, 875},		/* 245dps */
	{{0, 0}, 1750},		/* 500dps */
	{{0, 0}, 3500},		/* 1000dps */
	{{0, 0}, 7000},		/* 2000dps  1LSB=70mdps  here  mdps*100 */
};

static struct data_resolution lsm6dsmtr_acc_offset_resolution = { {0, 12}, 8192 };

static unsigned bufsiz = (15 * 1024);

int BMP160_spi_read_bytes(struct spi_device *spi, u16 addr, u8 *rx_buf, u32 data_len)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;
	u8 *tmp_buf = NULL;
	u32 package, reminder, retry;

	package = (data_len + 2) / 1024;
	reminder = (data_len + 2) % 1024;

	if ((package > 0) && (reminder != 0)) {
		xfer = kzalloc(sizeof(*xfer) * 4, GFP_KERNEL);
		retry = 1;
	} else {
		xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
		retry = 0;
	}
	if (xfer == NULL) {
		ACC_PR_ERR("%s, no memory for SPI transfer\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&accelgyro_obj_data->spi_lock);
	tmp_buf = accelgyro_obj_data->spi_buffer;

	spi_message_init(&msg);
	memset(tmp_buf, 0, 1 + data_len);
	*tmp_buf = (u8) (addr & 0xFF) | 0x80;
	xfer[0].tx_buf = tmp_buf;
	xfer[0].rx_buf = tmp_buf;
	xfer[0].len = 1 + data_len;
	xfer[0].delay_usecs = 5;

	xfer[0].speed_hz = 4 * 1000 * 1000;	/*4M */

	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(accelgyro_obj_data->spi, &msg);
	memcpy(rx_buf, tmp_buf + 1, data_len);

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;
	mutex_unlock(&accelgyro_obj_data->spi_lock);

	return 0;
}

int BMP160_spi_write_bytes(struct spi_device *spi, u16 addr, u8 *tx_buf, u32 data_len)
{
	struct spi_message msg;
	struct spi_transfer *xfer = NULL;
	u8 *tmp_buf = NULL;
	u32 package, reminder, retry;

	package = (data_len + 1) / 1024;
	reminder = (data_len + 1) % 1024;

	if ((package > 0) && (reminder != 0)) {
		xfer = kzalloc(sizeof(*xfer) * 2, GFP_KERNEL);
		retry = 1;
	} else {
		xfer = kzalloc(sizeof(*xfer), GFP_KERNEL);
		retry = 0;
	}
	if (xfer == NULL) {
		ACC_PR_ERR("%s, no memory for SPI transfer\n", __func__);
		return -ENOMEM;
	}
	tmp_buf = accelgyro_obj_data->spi_buffer;

	mutex_lock(&accelgyro_obj_data->spi_lock);
	spi_message_init(&msg);
	*tmp_buf = (u8) (addr & 0xFF);
	if (retry) {
		memcpy(tmp_buf + 1, tx_buf, (package * 1024 - 1));
		xfer[0].len = package * 1024;
	} else {
		memcpy(tmp_buf + 1, tx_buf, data_len);
		xfer[0].len = data_len + 1;
	}
	xfer[0].tx_buf = tmp_buf;
	xfer[0].delay_usecs = 5;

	xfer[0].speed_hz = 4 * 1000 * 1000;

	spi_message_add_tail(&xfer[0], &msg);
	spi_sync(accelgyro_obj_data->spi, &msg);

	if (retry) {
		addr = addr + package * 1024 - 1;
		spi_message_init(&msg);
		*tmp_buf = (u8) (addr & 0xFF);
		memcpy(tmp_buf + 1, (tx_buf + package * 1024 - 1), reminder);
		xfer[1].tx_buf = tmp_buf;
		xfer[1].len = reminder + 1;
		xfer[1].delay_usecs = 5;

		xfer[0].speed_hz = 4 * 1000 * 1000;

		spi_message_add_tail(&xfer[1], &msg);
		spi_sync(accelgyro_obj_data->spi, &msg);
	}

	kfree(xfer);
	if (xfer != NULL)
		xfer = NULL;
	mutex_unlock(&accelgyro_obj_data->spi_lock);

	return 0;
}

s8 bmi_read_wrapper(u8 dev_addr, u8 reg_addr, u8 *data, u8 len)
{
	int err = 0;

	err = BMP160_spi_read_bytes(accelgyro_obj_data->spi, reg_addr, data, len);
	if (err < 0)
		ACC_PR_ERR("read lsm6dsmtr i2c failed.\n");
	return err;
}

s8 bmi_write_wrapper(u8 dev_addr, u8 reg_addr, u8 *data, u8 len)
{
	int err = 0;

	err = BMP160_spi_write_bytes(accelgyro_obj_data->spi, reg_addr, data, len);
	if (err < 0)
		ACC_PR_ERR("read lsm6dsmtr i2c failed.\n");
	return err;
}

static void bmi_delay(u32 msec)
{
	mdelay(msec);
}

/*!
 * @brief Set data resolution
 *
 * @param[in] client the pointer of lsm6dsmtr_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_SetDataResolution(struct lsm6dsmtr_accelgyro_data *obj)
{
	int res;
	u8 dat, reso;

	res = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &dat, 1);
	if (res < 0) {
		ACC_PR_ERR("write data resolution fail!!\n");
		return res;
	}

	/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE} */
	reso = (dat & LSM6DSM_REG_CTRL1_XL_MASK_FS_XL) >> 2;
	if (reso >= 0x3)
		reso = 0x3;

	if (reso < ARRAY_SIZE(lsm6dsm_acc_data_resolution)) {
		obj->accel_reso = &lsm6dsm_acc_data_resolution[reso];
		return LSM6DSM_SUCCESS;
	}
	return 0;
}

static int LSM6DSMSTR_ACC_ReadData(struct lsm6dsmtr_accelgyro_data *obj,
				   s16 data[LSM6DSMSTR_ACC_AXES_NUM])
{
	int err = 0;
	u8 addr = LSM6DSM_REG_OUTX_L_XL;
	u8 buf[LSM6DSMSTR_ACC_DATA_LEN] = { 0 };

	err = BMP160_spi_read_bytes(obj->spi, addr, buf, LSM6DSMSTR_ACC_DATA_LEN);
	if (err) {
		ACC_PR_ERR("read data failed.\n");
	} else {
		/* Convert sensor raw data to 16-bit integer */

		data[LSM6DSMSTR_ACC_AXIS_X] = (s16) ((buf[1] << 8) | (buf[0]));
		data[LSM6DSMSTR_ACC_AXIS_Y] = (s16) ((buf[3] << 8) | (buf[2]));
		data[LSM6DSMSTR_ACC_AXIS_Z] = (s16) ((buf[5] << 8) | (buf[4]));
	}
	return err;
}

static int LSM6DSMSTR_ACC_ReadOffset(struct lsm6dsmtr_accelgyro_data *obj,
				     s8 ofs[LSM6DSMSTR_ACC_AXES_NUM])
{
	int err = 0;
#ifdef SW_CALIBRATION
	ofs[0] = ofs[1] = ofs[2] = 0x0;
#else
	/* not use HW cali, register not modify to st */
	err =
	    BMP160_spi_read_bytes(obj->spi, LSM6DSMSTR_ACC_REG_OFSX, ofs, LSM6DSMSTR_ACC_AXES_NUM);
	if (err)
		ACC_PR_ERR("error: %d\n", err);
#endif
	return err;
}


/*!
 * @brief Reset calibration for acc
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_ResetCalibration(struct lsm6dsmtr_accelgyro_data *obj)
{
	int err = 0;

	memset(obj->accel_cali_sw, 0x00, sizeof(obj->accel_cali_sw));
	memset(obj->accel_offset, 0x00, sizeof(obj->accel_offset));
	return err;
}

static int LSM6DSMSTR_ACC_ReadCalibration(struct lsm6dsmtr_accelgyro_data *obj,
					  int dat[LSM6DSMSTR_ACC_AXES_NUM])
{
	int err = 0;
	int mul;

#ifdef SW_CALIBRATION
	mul = 0;		/* only SW Calibration, disable HW Calibration */
#else
	err = LSM6DSMSTR_ACC_ReadOffset(obj, obj->accel_offset);
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	mul = obj->accel_reso->sensitivity / lsm6dsmtr_acc_offset_resolution.sensitivity;
#endif

	dat[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]] = obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X]
	    * (obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X] * mul +
	       obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X]);
	dat[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]] = obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y]
	    * (obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y] * mul +
	       obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y]);
	dat[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]] = obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z]
	    * (obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z] * mul +
	       obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z]);

	return err;
}

static int LSM6DSMSTR_ACC_ReadCalibrationEx(struct lsm6dsmtr_accelgyro_data *obj,
					    int act[LSM6DSMSTR_ACC_AXES_NUM],
					    int raw[LSM6DSMSTR_ACC_AXES_NUM])
{
	/* raw: the raw calibration data; act: the actual calibration data */
	int mul;
#ifdef SW_CALIBRATION
	/* only SW Calibration, disable  Calibration */
	mul = 0;
#else
	int err;

	err = LSM6DSMSTR_ACC_ReadOffset(obj, obj->accel_offset);
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	mul = obj->accel_reso->sensitivity / lsm6dsmtr_acc_offset_resolution.sensitivity;
#endif

	raw[LSM6DSMSTR_ACC_AXIS_X] =
	    obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X] * mul +
	    obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X];
	raw[LSM6DSMSTR_ACC_AXIS_Y] =
	    obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y] * mul +
	    obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y];
	raw[LSM6DSMSTR_ACC_AXIS_Z] =
	    obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z] * mul +
	    obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z];

	act[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] * raw[LSM6DSMSTR_ACC_AXIS_X];
	act[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] * raw[LSM6DSMSTR_ACC_AXIS_Y];
	act[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] * raw[LSM6DSMSTR_ACC_AXIS_Z];

	return 0;
}

static int LSM6DSMSTR_ACC_WriteCalibration(struct lsm6dsmtr_accelgyro_data *obj,
					   int dat[LSM6DSMSTR_ACC_AXES_NUM])
{
	int err = 0;
	int cali[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
	int raw[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
#ifndef SW_CALIBRATION
	int lsb = lsm6dsmtr_acc_offset_resolution.sensitivity;
	int divisor = obj->accel_reso->sensitivity / lsb;
#endif
	err = LSM6DSMSTR_ACC_ReadCalibrationEx(obj, cali, raw);
	/* accel_offset will be updated in obj->accel_offset */
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	ACC_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		raw[LSM6DSMSTR_ACC_AXIS_X], raw[LSM6DSMSTR_ACC_AXIS_Y], raw[LSM6DSMSTR_ACC_AXIS_Z],
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X], obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y],
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z], obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X],
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y],
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z]);
	/* calculate the real accel_offset expected by caller */
	cali[LSM6DSMSTR_ACC_AXIS_X] += dat[LSM6DSMSTR_ACC_AXIS_X];
	cali[LSM6DSMSTR_ACC_AXIS_Y] += dat[LSM6DSMSTR_ACC_AXIS_Y];
	cali[LSM6DSMSTR_ACC_AXIS_Z] += dat[LSM6DSMSTR_ACC_AXIS_Z];
	ACC_LOG("UPDATE: (%+3d %+3d %+3d)\n",
		dat[LSM6DSMSTR_ACC_AXIS_X], dat[LSM6DSMSTR_ACC_AXIS_Y], dat[LSM6DSMSTR_ACC_AXIS_Z]);
#ifdef SW_CALIBRATION
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]]);
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]]);
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]]);
#else
	obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X] =
	    (s8) (obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] *
		  (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]]) / (divisor));
	obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y] =
	    (s8) (obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] *
		  (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]]) / (divisor));
	obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z] =
	    (s8) (obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] *
		  (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]]) / (divisor));

	/*convert software calibration using standard calibration */
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]]) %
	    (divisor);
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]]) %
	    (divisor);
	obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] * (cali[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]]) %
	    (divisor);

	ACC_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X] * divisor +
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X],
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y] * divisor +
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y],
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z] * divisor +
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z], obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X],
		obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y], obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z],
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X],
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y],
		obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z]);
	/* not use HW cali, register not modify to st */
	err =
	    BMP160_spi_write_bytes(obj->spi, LSM6DSMSTR_ACC_REG_OFSX, obj->accel_offset,
				   LSM6DSMSTR_ACC_AXES_NUM);
	if (err) {
		ACC_PR_ERR("write accel_offset fail: %d\n", err);
		return err;
	}
#endif

	return err;
}

/*!
 * @brief Input event initialization for device
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_CheckDeviceID(struct lsm6dsmtr_accelgyro_data *obj)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	/* err = BMP160_spi_read_bytes(obj->spi, 0x7F, databuf, 1); */
	err = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_WHO_AM_I, databuf, 1);
	if (err < 0) {
		ACC_PR_ERR("read chip id failed.\n");
		return LSM6DSMSTR_ACC_ERR_SPI;
	}
	switch (databuf[0]) {
	case LSM6DS3_FIXED_DEVID:
	case LSM6DSL_FIXED_DEVID:
		ACC_LOG("check chip id %d successfully.\n", databuf[0]);
		break;
	default:
		ACC_LOG("check chip id %d %d failed.\n", databuf[0], databuf[1]);
		return LSM6DSM_ERR_IDENTIFICATION;
	}
	return err;

}

/*!
 * @brief Set power mode for acc
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 * @param[in] enable
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_SetPowerMode(struct lsm6dsmtr_accelgyro_data *obj, bool enable)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	if (enable == sensor_power) {
		ACC_LOG("power status is newest!\n");
		return 0;
	}
	mutex_lock(&obj->accel_lock);
	if (enable == true) {
		if (obj->accel_odr.acc_odr == 0)
			obj->accel_odr.acc_odr = LSM6DSM_REG_CTRL1_XL_ODR_104HZ;
		databuf[1] = obj->accel_odr.acc_odr;
	} else
		databuf[1] = LSM6DSM_REG_CTRL1_XL_ODR_0HZ;
	ACC_LOG("power status is %d!\n", databuf[1]);
	err = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);
	databuf[0] =
	    LSM6DSMSTR_SET_BITSLICE(databuf[0], LSM6DSM_REG_CTRL1_XL_MASK_ODR_XL, databuf[1]);
	err = BMP160_spi_write_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);

	if (err < 0) {
		ACC_PR_ERR("write power mode value to register failed.\n");
		mutex_unlock(&obj->accel_lock);
		return LSM6DSMSTR_ACC_ERR_SPI;
	}
	sensor_power = enable;
	mdelay(1);
	mutex_unlock(&obj->accel_lock);
	ACC_LOG("set power mode enable = %d ok!\n", enable);
	return 0;
}

/*!
 * @brief Set range value and resolution for acc
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 * @param[in] range value
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_SetDataFormat(struct lsm6dsmtr_accelgyro_data *obj, u8 dataformat)
{
	int res = 0;
	u8 databuf[2] = { 0 };

	mutex_lock(&obj->accel_lock);
	res = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);
	databuf[0] =
	    LSM6DSMSTR_SET_BITSLICE(databuf[0], LSM6DSM_REG_CTRL1_XL_MASK_FS_XL, dataformat);

	res += BMP160_spi_write_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);
	mdelay(1);

	if (res < 0) {
		ACC_PR_ERR("set range failed.\n");
		mutex_unlock(&obj->accel_lock);
		return LSM6DSMSTR_ACC_ERR_SPI;
	}
	mutex_unlock(&obj->accel_lock);
	return LSM6DSMSTR_ACC_SetDataResolution(obj);
}

/*!
 * @brief Set bandwidth, set ODR for acc
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 * @param[in] bandwidth value
 *
 * @return zero success, non-zero failed
 */
static int LSM6DSMSTR_ACC_SetBWRate(struct lsm6dsmtr_accelgyro_data *obj, u8 bwrate)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	if (bwrate == 0)
		return 0;

	mutex_lock(&obj->accel_lock);
	err = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);

	databuf[0] = LSM6DSMSTR_SET_BITSLICE(databuf[0], LSM6DSM_REG_CTRL1_XL_MASK_ODR_XL, bwrate);

	err += BMP160_spi_write_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &databuf[0], 1);

	mdelay(20);
	if (err < 0) {
		ACC_PR_ERR("set bandwidth failed, res = %d\n", err);
		mutex_unlock(&obj->accel_lock);
		return LSM6DSMSTR_ACC_ERR_SPI;
	}
	ACC_LOG("set bandwidth = %d ok.\n", bwrate);
	mutex_unlock(&obj->accel_lock);
	return err;
}


/*!
 * @brief lsm6dsmtr initialization
 *
 * @param[in] obj the pointer of struct lsm6dsmtr_accelgyro_data
 * @param[in] int reset calibration value
 *
 * @return zero success, non-zero failed
 */
static int lsm6dsmtr_acc_init_client(struct lsm6dsmtr_accelgyro_data *obj, int reset_cali)
{
	int err = 0;

	err = LSM6DSMSTR_ACC_CheckDeviceID(obj);
	if (err < 0)
		return err;

	/* set power mode, 0x00 init */
	err = LSM6DSMSTR_ACC_SetPowerMode(obj, false);
	if (err < 0)
		return err;

	/* set ODR HZ */
	err = LSM6DSMSTR_ACC_SetBWRate(obj, LSM6DSM_REG_CTRL1_XL_ODR_0HZ);
	if (err < 0)
		return err;

	/* set  resolution +-4G */
	err = LSM6DSMSTR_ACC_SetDataFormat(obj, LSM6DSM_REG_CTRL1_XL_FS_4G);

	if (err < 0)
		return err;

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->accel_reso->sensitivity;

	if (reset_cali != 0) {
		/* reset calibration only in power on */
		err = LSM6DSMSTR_ACC_ResetCalibration(obj);
		if (err < 0)
			return err;
	}
	ACC_LOG("lsm6dsmtr acc init OK.\n");
	return 0;
}

static int LSM6DSMSTR_ACC_ReadChipInfo(struct lsm6dsmtr_accelgyro_data *obj, char *buf, int bufsize)
{
	sprintf(buf, "lsm6dsmtr_acc");
	return 0;
}


static int LSM6DSMSTR_ACC_CompassReadData(struct lsm6dsmtr_accelgyro_data *obj, char *buf,
					  int bufsize)
{
	int res = 0;
	int acc[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
	s16 databuf[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };

	if (sensor_power == false) {
		res = LSM6DSMSTR_ACC_SetPowerMode(obj, true);
		if (res)
			ACC_PR_ERR("Power on lsm6dsmtr_acc error %d!\n", res);
	}
	res = LSM6DSMSTR_ACC_ReadData(obj, databuf);
	if (res) {
		ACC_PR_ERR("read acc data failed.\n");
		return -3;
	}
	/* Add compensated value performed by MTK calibration process */
	databuf[LSM6DSMSTR_ACC_AXIS_X] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X];
	databuf[LSM6DSMSTR_ACC_AXIS_Y] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y];
	databuf[LSM6DSMSTR_ACC_AXIS_Z] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z];
	/*remap coordinate */
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] * databuf[LSM6DSMSTR_ACC_AXIS_X];
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] * databuf[LSM6DSMSTR_ACC_AXIS_Y];
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] * databuf[LSM6DSMSTR_ACC_AXIS_Z];

	sprintf(buf, "%d %d %d",
		(s16) acc[LSM6DSMSTR_ACC_AXIS_X],
		(s16) acc[LSM6DSMSTR_ACC_AXIS_Y], (s16) acc[LSM6DSMSTR_ACC_AXIS_Z]);

	return 0;
}

static int LSM6DSMSTR_ACC_ReadSensorData(struct lsm6dsmtr_accelgyro_data *obj, char *buf,
					 int bufsize)
{
	int err = 0;
	int acc[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
	s16 databuf[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };

	if (sensor_power == false) {
		err = LSM6DSMSTR_ACC_SetPowerMode(obj, true);
		if (err) {
			ACC_PR_ERR("set power on acc failed.\n");
			return err;
		}
	}
	err = LSM6DSMSTR_ACC_ReadData(obj, databuf);
	if (err) {
		ACC_PR_ERR("read acc data failed.\n");
		return err;
	}

	databuf[LSM6DSMSTR_ACC_AXIS_X] =
	    (databuf[LSM6DSMSTR_ACC_AXIS_X] * obj->accel_reso->sensitivity / 1000) *
	    GRAVITY_EARTH_1000 / 1000;
	databuf[LSM6DSMSTR_ACC_AXIS_Y] =
	    (databuf[LSM6DSMSTR_ACC_AXIS_Y] * obj->accel_reso->sensitivity / 1000) *
	    GRAVITY_EARTH_1000 / 1000;
	databuf[LSM6DSMSTR_ACC_AXIS_Z] =
	    (databuf[LSM6DSMSTR_ACC_AXIS_Z] * obj->accel_reso->sensitivity / 1000) *
	    GRAVITY_EARTH_1000 / 1000;

	databuf[LSM6DSMSTR_ACC_AXIS_X] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X];
	databuf[LSM6DSMSTR_ACC_AXIS_Y] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y];
	databuf[LSM6DSMSTR_ACC_AXIS_Z] += obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z];

	/*remap coordinate */
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_X]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_X] * databuf[LSM6DSMSTR_ACC_AXIS_X];
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Y]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Y] * databuf[LSM6DSMSTR_ACC_AXIS_Y];
	acc[obj->cvt.map[LSM6DSMSTR_ACC_AXIS_Z]] =
	    obj->cvt.sign[LSM6DSMSTR_ACC_AXIS_Z] * databuf[LSM6DSMSTR_ACC_AXIS_Z];

	sprintf(buf, "%04x %04x %04x",
		acc[LSM6DSMSTR_ACC_AXIS_X], acc[LSM6DSMSTR_ACC_AXIS_Y], acc[LSM6DSMSTR_ACC_AXIS_Z]);
	return 0;
}

static int LSM6DSMSTR_ACC_ReadRawData(struct lsm6dsmtr_accelgyro_data *obj, char *buf)
{
	int err = 0;
	s16 databuf[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };

	err = LSM6DSMSTR_ACC_ReadData(obj, databuf);
	if (err) {
		ACC_PR_ERR("read acc raw data failed.\n");
		return -EIO;
	}
	sprintf(buf, "LSM6DSMSTR_ACC_ReadRawData %04x %04x %04x",
		databuf[LSM6DSMSTR_ACC_AXIS_X],
		databuf[LSM6DSMSTR_ACC_AXIS_Y], databuf[LSM6DSMSTR_ACC_AXIS_Z]);

	return 0;
}

static int lsm6dsmtr_acc_set_range(struct lsm6dsmtr_accelgyro_data *obj, unsigned char range)
{
	int comres = 0;
	unsigned char data[2] = { LSM6DSM_REG_CTRL1_XL };

	if (obj == NULL)
		return -1;
	mutex_lock(&obj->accel_lock);
	comres = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, data + 1, 1);

	data[1] = LSM6DSMSTR_SET_BITSLICE(data[1], LSM6DSM_REG_CTRL1_XL_MASK_FS_XL, range);

	/* comres = i2c_master_send(client, data, 2); */
	comres = BMP160_spi_write_bytes(obj->spi, data[0], &data[1], 1);
	mutex_unlock(&obj->accel_lock);
	if (comres <= 0)
		return LSM6DSMSTR_ACC_ERR_SPI;
	else
		return comres;
}

static int lsm6dsmtr_acc_get_range(struct lsm6dsmtr_accelgyro_data *obj, u8 *range)
{
	int comres = 0;
	u8 data;

	comres = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, &data, 1);
	*range = LSM6DSMSTR_GET_BITSLICE(data, LSM6DSM_REG_CTRL1_XL_MASK_FS_XL);
	return comres;
}

static int lsm6dsmtr_acc_get_bandwidth(struct lsm6dsmtr_accelgyro_data *obj,
				       unsigned char *bandwidth)
{
	int comres = 0;

	comres = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL1_XL, bandwidth, 1);
	*bandwidth = LSM6DSMSTR_GET_BITSLICE(*bandwidth, LSM6DSM_REG_CTRL1_XL_MASK_ODR_XL);
	return comres;
}

static ssize_t accel_show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[LSM6DSMSTR_BUFSIZE];
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	LSM6DSMSTR_ACC_ReadChipInfo(obj, strbuf, LSM6DSMSTR_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_acc_range_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	u8 data;

	err = lsm6dsmtr_acc_get_range(accelgyro_obj_data, &data);
	if (err < 0) {
		ACC_LOG("get acc range failed.\n");
		return err;
	}
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_store_acc_range_value(struct device_driver *ddri, const char *buf,
					   size_t count)
{
	unsigned long data;
	int err;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	err = lsm6dsmtr_acc_set_range(accelgyro_obj_data, (u8) data);
	if (err < 0) {
		ACC_PR_ERR("set acc range = %d failed.\n", (int)data);
		return err;
	}
	return count;
}

static ssize_t accel_show_acc_odr_value(struct device_driver *ddri, char *buf)
{
	int err;
	u8 data;

	err = lsm6dsmtr_acc_get_bandwidth(accelgyro_obj_data, &data);
	if (err < 0) {
		ACC_LOG("get acc odr failed.\n");
		return err;
	}
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_store_acc_odr_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int err;
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	/* srx mark, should add after */
	/* err = lsm6dsmtr_acc_set_odr_value(obj, (u8) data); */
	if (err < 0) {
		ACC_PR_ERR("set acc bandwidth failed.\n");
		return err;
	}
	obj->accel_odr.acc_odr = data;
	return count;
}

static ssize_t accel_show_cpsdata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[LSM6DSMSTR_BUFSIZE] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	LSM6DSMSTR_ACC_CompassReadData(obj, strbuf, LSM6DSMSTR_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[LSM6DSMSTR_BUFSIZE] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	LSM6DSMSTR_ACC_ReadSensorData(obj, strbuf, LSM6DSMSTR_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_cali_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	int len = 0;
	int mul;
	int tmp[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	obj = accelgyro_obj_data;
	err = LSM6DSMSTR_ACC_ReadOffset(obj, obj->accel_offset);
	if (err)
		return -EINVAL;
	err = LSM6DSMSTR_ACC_ReadCalibration(obj, tmp);
	if (err)
		return -EINVAL;

	mul = obj->accel_reso->sensitivity / lsm6dsmtr_acc_offset_resolution.sensitivity;
	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z]);
	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X],
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y],
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z]);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len,
		     "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_X] * mul +
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_X],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Y] * mul +
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Y],
		     obj->accel_offset[LSM6DSMSTR_ACC_AXIS_Z] * mul +
		     obj->accel_cali_sw[LSM6DSMSTR_ACC_AXIS_Z], tmp[LSM6DSMSTR_ACC_AXIS_X],
		     tmp[LSM6DSMSTR_ACC_AXIS_Y], tmp[LSM6DSMSTR_ACC_AXIS_Z]);

	return len;

}

static ssize_t accel_store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err, x, y, z;
	int dat[LSM6DSMSTR_ACC_AXES_NUM] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	if (!strncmp(buf, "rst", 3)) {
		err = LSM6DSMSTR_ACC_ResetCalibration(obj);
		if (err)
			ACC_PR_ERR("reset accel_offset err = %d\n", err);
	} else if (sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z) == 3) {
		dat[LSM6DSMSTR_ACC_AXIS_X] = x;
		dat[LSM6DSMSTR_ACC_AXIS_Y] = y;
		dat[LSM6DSMSTR_ACC_AXIS_Z] = z;
		err = LSM6DSMSTR_ACC_WriteCalibration(obj, dat);
		if (err)
			ACC_PR_ERR("write calibration err = %d\n", err);
	} else {
		ACC_PR_ERR("set calibration value by invalid format.\n");
	}
	return count;
}

static ssize_t accel_show_firlen_value(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "not support\n");
}

static ssize_t accel_store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
	return count;
}

static ssize_t accel_show_trace_value(struct device_driver *ddri, char *buf)
{
	int err;
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	err = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->accel_trace));
	return err;
}

static ssize_t accel_store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->accel_trace, trace);
	else
		ACC_PR_ERR("invalid content: '%s'\n", buf);

	return count;
}

static ssize_t accel_show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	if (obj->hw)
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d\n", obj->hw->direction);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	return len;
}

static ssize_t accel_show_power_status_value(struct device_driver *ddri, char *buf)
{
	if (sensor_power)
		ACC_LOG("G sensor is in work mode, sensor_power = %d\n", sensor_power);
	else
		ACC_LOG("G sensor is in standby mode, sensor_power = %d\n", sensor_power);

	return 0;
}


LSM6DSMSTR_RETURN_FUNCTION_TYPE lsm6dsmtr_write_reg(u8 v_addr_u8, u8 *v_data_u8, u8 v_len_u8)
{
	/* variable used for return the status of communication result */
	LSM6DSMSTR_RETURN_FUNCTION_TYPE com_rslt = E_LSM6DSMSTR_COMM_RES;
	/* check the p_lsm6dsmtr structure as NULL */
	if (p_lsm6dsmtr == LSM6DSMSTR_NULL)
		return E_LSM6DSMSTR_NULL_PTR;

	/* write data from register */
	com_rslt =
	    p_lsm6dsmtr->LSM6DSMSTR_BUS_WRITE_FUNC(p_lsm6dsmtr->dev_addr, v_addr_u8, v_data_u8,
						   v_len_u8);

	return com_rslt;
}

LSM6DSMSTR_RETURN_FUNCTION_TYPE lsm6dsmtr_init(struct lsm6dsmtr_t *lsm6dsmtr)
{
	/* variable used for return the status of communication result */
	LSM6DSMSTR_RETURN_FUNCTION_TYPE com_rslt = E_LSM6DSMSTR_COMM_RES;
	u8 v_data_u8 = LSM6DSMSTR_INIT_VALUE;
	/* u8 v_pmu_data_u8 = LSM6DSMSTR_INIT_VALUE; */
	/* assign lsm6dsmtr ptr */
	p_lsm6dsmtr = lsm6dsmtr;
	com_rslt =
	    p_lsm6dsmtr->LSM6DSMSTR_BUS_READ_FUNC(p_lsm6dsmtr->dev_addr,
						  LSM6DSM_REG_WHO_AM_I,
						  &v_data_u8,
						  LSM6DSMSTR_GEN_READ_WRITE_DATA_LENGTH);
	/* read Chip Id */
	p_lsm6dsmtr->chip_id = v_data_u8;
	/*no need for lsm because it had no PMU */
	/* To avoid gyro wakeup it is required to write 0x00 to 0x6C */
	/* com_rslt += lsm6dsmtr_write_reg(LSM6DSMSTR_USER_PMU_TRIGGER_ADDR,*/
	/* &v_pmu_data_u8, LSM6DSMSTR_GEN_READ_WRITE_DATA_LENGTH); */
	return com_rslt;
}

static ssize_t accel_show_layout_value(struct device_driver *ddri, char *buf)
{
	struct lsm6dsmtr_accelgyro_data *data = accelgyro_obj_data;

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		       data->hw->direction, atomic_read(&data->layout), data->cvt.sign[0],
		       data->cvt.sign[1], data->cvt.sign[2], data->cvt.map[0], data->cvt.map[1],
		       data->cvt.map[2]);
}

static ssize_t accel_store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct lsm6dsmtr_accelgyro_data *data = accelgyro_obj_data;
	int layout = 0;

	if (kstrtos32(buf, 10, &layout) == 0) {
		atomic_set(&data->layout, layout);
		if (!hwmsen_get_convert(layout, &data->cvt)) {
			ACC_PR_ERR("HWMSEN_GET_CONVERT function error!\r\n");
		} else if (!hwmsen_get_convert(data->hw->direction, &data->cvt)) {
			ACC_PR_ERR("invalid layout: %d, restore to %d\n", layout,
				   data->hw->direction);
		} else {
			ACC_PR_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	} else {
		ACC_PR_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}


static ssize_t accel_lsm6dsmtr_delay_show(struct device_driver *ddri, char *buf)
{
	struct lsm6dsmtr_accelgyro_data *client_data = accelgyro_obj_data;

	return sprintf(buf, "%d\n", atomic_read(&client_data->accel_delay));
}

static ssize_t accel_lsm6dsmtr_delay_store(struct device_driver *ddri, const char *buf,
					   size_t count)
{
	int err;
	unsigned long data;
	struct lsm6dsmtr_accelgyro_data *client_data = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	if (data < BMI_DELAY_MIN)
		data = BMI_DELAY_MIN;

	atomic_set(&client_data->accel_delay, (unsigned int)data);
	return count;
}

static int bmg_read_raw_data(struct lsm6dsmtr_accelgyro_data *obj, s16 data[BMG_AXES_NUM])
{
	int err = 0;
	struct lsm6dsmtr_accelgyro_data *priv = gyro_obj_data;

	if (priv->gyro_sensor_type == LSM6DSMSTR_GYRO_TYPE) {
		u8 buf_tmp[BMG_DATA_LEN] = { 0 };

		err = BMP160_spi_read_bytes(NULL, LSM6DSM_REG_OUTX_L_G, buf_tmp, 6);
		if (err) {
			GYRO_PR_ERR("read gyro raw data failed.\n");
			return err;
		}

		data[BMG_AXIS_X] = (s16) ((buf_tmp[1] << 8) | (buf_tmp[0]));
		data[BMG_AXIS_Y] = (s16) ((buf_tmp[3] << 8) | (buf_tmp[2]));
		data[BMG_AXIS_Z] = (s16) ((buf_tmp[5] << 8) | (buf_tmp[4]));
		if (atomic_read(&priv->gyro_trace) & GYRO_TRC_RAWDATA) {
			GYRO_LOG("[%s][16bit raw][%08X %08X %08X] => [%5d %5d %5d]\n",
				 priv->gyro_sensor_name,
				 data[BMG_AXIS_X],
				 data[BMG_AXIS_Y],
				 data[BMG_AXIS_Z],
				 data[BMG_AXIS_X], data[BMG_AXIS_Y], data[BMG_AXIS_Z]);
		}
	}
#ifdef CONFIG_BMG_LOWPASS
/*
*Example: firlen = 16, filter buffer = [0] ... [15],
*when 17th data come, replace [0] with this new data.
*Then, average this filter buffer and report average value to upper layer.
*/
	if (atomic_read(&priv->gyro_filter)) {
		if (atomic_read(&priv->gyro_fir_en) && !atomic_read(&priv->gyro_suspend)) {
			int idx, firlen = atomic_read(&priv->gyro_firlen);

			if (priv->gyro_fir.num < firlen) {
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_X] =
				    data[BMG_AXIS_X];
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_Y] =
				    data[BMG_AXIS_Y];
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_Z] =
				    data[BMG_AXIS_Z];
				priv->gyro_fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
				priv->gyro_fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
				priv->gyro_fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
				if (atomic_read(&priv->gyro_trace) & GYRO_TRC_FILTER) {
					GYRO_LOG("add [%2d][%5d %5d %5d] => [%5d %5d %5d]\n",
						 priv->gyro_fir.num,
						 priv->gyro_fir.raw
						 [priv->gyro_fir.num][BMG_AXIS_X],
						 priv->gyro_fir.raw
						 [priv->gyro_fir.num][BMG_AXIS_Y],
						 priv->gyro_fir.raw
						 [priv->gyro_fir.num][BMG_AXIS_Z],
						 priv->gyro_fir.sum[BMG_AXIS_X],
						 priv->gyro_fir.sum[BMG_AXIS_Y],
						 priv->gyro_fir.sum[BMG_AXIS_Z]);
				}
				priv->gyro_fir.num++;
				priv->gyro_fir.idx++;
			} else {
				idx = priv->gyro_fir.idx % firlen;
				priv->gyro_fir.sum[BMG_AXIS_X] -=
				    priv->gyro_fir.raw[idx][BMG_AXIS_X];
				priv->gyro_fir.sum[BMG_AXIS_Y] -=
				    priv->gyro_fir.raw[idx][BMG_AXIS_Y];
				priv->gyro_fir.sum[BMG_AXIS_Z] -=
				    priv->gyro_fir.raw[idx][BMG_AXIS_Z];
				priv->gyro_fir.raw[idx][BMG_AXIS_X] = data[BMG_AXIS_X];
				priv->gyro_fir.raw[idx][BMG_AXIS_Y] = data[BMG_AXIS_Y];
				priv->gyro_fir.raw[idx][BMG_AXIS_Z] = data[BMG_AXIS_Z];
				priv->gyro_fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
				priv->gyro_fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
				priv->gyro_fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
				priv->gyro_fir.idx++;
				data[BMG_AXIS_X] = priv->gyro_fir.sum[BMG_AXIS_X] / firlen;
				data[BMG_AXIS_Y] = priv->gyro_fir.sum[BMG_AXIS_Y] / firlen;
				data[BMG_AXIS_Z] = priv->gyro_fir.sum[BMG_AXIS_Z] / firlen;
				if (atomic_read(&priv->gyro_trace) & GYRO_TRC_FILTER) {
					GYRO_LOG
					    ("add [%2d][%5d %5d %5d] =>[%5d %5d %5d] : [%5d %5d %5d]\n",
					     idx, priv->gyro_fir.raw[idx][BMG_AXIS_X],
					     priv->gyro_fir.raw[idx][BMG_AXIS_Y],
					     priv->gyro_fir.raw[idx][BMG_AXIS_Z],
					     priv->gyro_fir.sum[BMG_AXIS_X],
					     priv->gyro_fir.sum[BMG_AXIS_Y],
					     priv->gyro_fir.sum[BMG_AXIS_Z], data[BMG_AXIS_X],
					     data[BMG_AXIS_Y], data[BMG_AXIS_Z]);
				}
			}
		}
	}
#endif
	return err;
}

#ifndef SW_CALIBRATION
/* get hardware offset value from chip register */
static int bmg_get_hw_offset(struct lsm6dsmtr_accelgyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;
	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
		 offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);
	return err;
}
#endif

#ifndef SW_CALIBRATION
/* set hardware offset value to chip register*/
static int bmg_set_hw_offset(struct lsm6dsmtr_accelgyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;
	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
		 offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);
	return err;
}
#endif

static int bmg_reset_calibration(struct lsm6dsmtr_accelgyro_data *obj)
{
	int err = 0;
#ifdef SW_CALIBRATION

#else
	err = bmg_set_hw_offset(NULL, obj->gyro_offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif
	memset(obj->gyro_cali_sw, 0x00, sizeof(obj->gyro_cali_sw));
	memset(obj->gyro_offset, 0x00, sizeof(obj->gyro_offset));
	return err;
}

static int bmg_read_calibration(struct lsm6dsmtr_accelgyro_data *obj,
				int act[BMG_AXES_NUM], int raw[BMG_AXES_NUM])
{
	/*
	 *raw: the raw calibration data, unmapped;
	 *act: the actual calibration data, mapped
	 */
	int err = 0;
	int mul;

	obj = gyro_obj_data;

#ifdef SW_CALIBRATION
	/* only sw calibration, disable hw calibration */
	mul = 0;
#else
	err = bmg_get_hw_offset(NULL, obj->gyro_offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed, %d\n", err);
		return err;
	}
	mul = 1;		/* mul = sensor sensitivity / offset sensitivity */
#endif

	raw[BMG_AXIS_X] = obj->gyro_offset[BMG_AXIS_X] * mul + obj->gyro_cali_sw[BMG_AXIS_X];
	raw[BMG_AXIS_Y] = obj->gyro_offset[BMG_AXIS_Y] * mul + obj->gyro_cali_sw[BMG_AXIS_Y];
	raw[BMG_AXIS_Z] = obj->gyro_offset[BMG_AXIS_Z] * mul + obj->gyro_cali_sw[BMG_AXIS_Z];

	act[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * raw[BMG_AXIS_X];
	act[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * raw[BMG_AXIS_Y];
	act[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * raw[BMG_AXIS_Z];
	return err;
}

static int bmg_write_calibration(struct lsm6dsmtr_accelgyro_data *obj, int dat[BMG_AXES_NUM])
{
	/* dat array : Android coordinate system, mapped, unit:LSB */
	int err = 0;
	int cali[BMG_AXES_NUM] = { 0 };
	int raw[BMG_AXES_NUM] = { 0 };

	obj = gyro_obj_data;
	/*offset will be updated in obj->offset */
	err = bmg_read_calibration(NULL, cali, raw);
	if (err) {
		GYRO_PR_ERR("read offset fail, %d\n", err);
		return err;
	}
	/* calculate the real offset expected by caller */
	cali[BMG_AXIS_X] += dat[BMG_AXIS_X];
	cali[BMG_AXIS_Y] += dat[BMG_AXIS_Y];
	cali[BMG_AXIS_Z] += dat[BMG_AXIS_Z];
	GYRO_LOG("UPDATE: add mapped data(%+3d %+3d %+3d)\n",
		 dat[BMG_AXIS_X], dat[BMG_AXIS_Y], dat[BMG_AXIS_Z]);

#ifdef SW_CALIBRATION
	/* obj->cali_sw array : chip coordinate system, unmapped,unit:LSB */
	obj->gyro_cali_sw[BMG_AXIS_X] =
	    obj->cvt.sign[BMG_AXIS_X] * (cali[obj->cvt.map[BMG_AXIS_X]]);
	obj->gyro_cali_sw[BMG_AXIS_Y] =
	    obj->cvt.sign[BMG_AXIS_Y] * (cali[obj->cvt.map[BMG_AXIS_Y]]);
	obj->gyro_cali_sw[BMG_AXIS_Z] =
	    obj->cvt.sign[BMG_AXIS_Z] * (cali[obj->cvt.map[BMG_AXIS_Z]]);
#else
	/* divisor = sensor sensitivity / offset sensitivity */
	int divisor = 1;

	obj->gyro_offset[BMG_AXIS_X] = (s8) (obj->cvt.sign[BMG_AXIS_X] *
					     (cali[obj->cvt.map[BMG_AXIS_X]]) / (divisor));
	obj->gyro_offset[BMG_AXIS_Y] = (s8) (obj->cvt.sign[BMG_AXIS_Y] *
					     (cali[obj->cvt.map[BMG_AXIS_Y]]) / (divisor));
	obj->gyro_offset[BMG_AXIS_Z] = (s8) (obj->cvt.sign[BMG_AXIS_Z] *
					     (cali[obj->cvt.map[BMG_AXIS_Z]]) / (divisor));

	/*convert software calibration using standard calibration */
	obj->gyro_cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X] *
	    (cali[obj->cvt.map[BMG_AXIS_X]]) % (divisor);
	obj->gyro_cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y] *
	    (cali[obj->cvt.map[BMG_AXIS_Y]]) % (divisor);
	obj->gyro_cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z] *
	    (cali[obj->cvt.map[BMG_AXIS_Z]]) % (divisor);
	/* HW calibration is under construction */
	err = bmg_set_hw_offset(NULL, obj->gyro_offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed.\n");
		return err;
	}
#endif
	return err;
}

/* get chip type */
static int bmg_get_chip_type(struct lsm6dsmtr_accelgyro_data *obj)
{
	int err = 0;
	u8 chip_id = 0;

	obj = gyro_obj_data;
	/* twice */
	/* err = BMP160_spi_read_bytes(NULL, LSM6DSM_REG_WHO_AM_I, &chip_id, 1); */
	err = BMP160_spi_read_bytes(NULL, LSM6DSM_REG_WHO_AM_I, &chip_id, 1);
	if (err != 0) {
		GYRO_PR_ERR("read chip id failed.\n");
		return err;
	}

	switch (chip_id) {
	case LSM6DS3_FIXED_DEVID:
	case LSM6DSL_FIXED_DEVID:
		obj->gyro_sensor_type = LSM6DSMSTR_GYRO_TYPE;
		strcpy(obj->gyro_sensor_name, BMG_DEV_NAME);
		break;
	default:
		obj->gyro_sensor_type = INVALID_TYPE;
		strcpy(obj->gyro_sensor_name, UNKNOWN_DEV);
		break;
	}
	if (obj->gyro_sensor_type == INVALID_TYPE) {
		GYRO_PR_ERR("unknown gyroscope.\n");
		return -1;
	}
	return err;
}

static int bmg_set_powermode(struct lsm6dsmtr_accelgyro_data *obj, bool power_mode)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	obj = gyro_obj_data;

	if (power_mode == obj->lsm6dsm_gyro_power)
		return 0;

	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == LSM6DSMSTR_GYRO_TYPE) {
		if (power_mode == true) {
			if (obj->gyro_odr.gyro_odr == 0)
				obj->gyro_odr.gyro_odr = LSM6DSM_REG_CTRL2_G_ODR_104HZ;
			power_mode = obj->gyro_odr.gyro_odr;
		} else
			power_mode = LSM6DSM_REG_CTRL2_G_ODR_0HZ;
		GYRO_LOG("power status is %d!\n", power_mode);

		err = BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL2_G, &databuf[0], 1);
		databuf[0] =
		    LSM6DSMSTR_SET_BITSLICE(databuf[0], LSM6DSM_REG_CTRL2_G_MASK_ODR_G, power_mode);

		err += BMP160_spi_write_bytes(NULL, LSM6DSM_REG_CTRL2_G, &databuf[0], 1);
		mdelay(55);
	}
	if (err < 0) {
		GYRO_PR_ERR("set power mode failed, err = %d, sensor name = %s\n",
			    err, obj->gyro_sensor_name);
	} else {
		obj->lsm6dsm_gyro_power = power_mode;
	}
	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set power mode = %d ok.\n", (int)power_mode);
	return err;
}

static int bmg_set_range(struct lsm6dsmtr_accelgyro_data *obj, u8 range)
{
	u8 err = 0;
	u8 data = 0;
	u8 reso;
	/* u8 actual_range = 0; */

	obj = gyro_obj_data;

	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == LSM6DSMSTR_GYRO_TYPE) {
		err = BMP160_spi_read_bytes(NULL, LSM6DSM_REG_CTRL2_G, &data, 1);
		data = BMG_SET_BITSLICE(data, LSM6DSM_REG_CTRL2_G_MASK_FS_G, range);
		err += BMP160_spi_write_bytes(NULL, LSM6DSM_REG_CTRL2_G, &data, 1);
		mdelay(1);
		if (err < 0) {
			GYRO_PR_ERR("set range failed.\n");
		} else {
			obj->gyro_range = range;
			/* bitnum: 16bit */
			BMP160_spi_read_bytes(obj->spi, LSM6DSM_REG_CTRL2_G, &data, 1);

			/*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE} */
			reso = (data & LSM6DSM_REG_CTRL2_G_MASK_FS_G) >> 2;
			if (reso >= 0x3)
				reso = 0x3;
			if (reso <
			    sizeof(lsm6dsm_gyro_data_resolution) /
			    sizeof(lsm6dsm_gyro_data_resolution[0]))
				obj->gyro_reso = &lsm6dsm_gyro_data_resolution[reso];
			/* return LSM6DSM_SUCCESS; */
			else
				return -EINVAL;
		}
	}
	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set range ok.\n");
	return err;
}

static int bmg_set_datarate(struct lsm6dsmtr_accelgyro_data *obj, int datarate)
{
	int err = 0;
	u8 data = 0;

	obj = gyro_obj_data;
	if (datarate == obj->gyro_datarate) {
		GYRO_LOG("set new data rate = %d, old data rate = %d\n", datarate,
			 obj->gyro_datarate);
		return 0;
	}
	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == LSM6DSMSTR_GYRO_TYPE) {
		err = BMP160_spi_read_bytes(NULL, LSM6DSM_REG_CTRL2_G, &data, 1);
		data = BMG_SET_BITSLICE(data, LSM6DSM_REG_CTRL2_G_MASK_ODR_G, datarate);
		err += BMP160_spi_write_bytes(NULL, LSM6DSM_REG_CTRL2_G, &data, 1);
	}
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
	else
		obj->gyro_datarate = datarate;

	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set data rate = %d ok.\n", datarate);
	return err;
}

static int bmg_init_client(struct lsm6dsmtr_accelgyro_data *obj, int reset_cali)
{
#ifdef CONFIG_BMG_LOWPASS
	struct lsm6dsmtr_accelgyro_data *obj = (struct lsm6dsmtr_accelgyro_data *)gyro_obj_data;
#endif
	int err = 0;

	/* set power mode */
	err = bmg_set_powermode(NULL, false);
	if (err < 0)
		return err;

	err = bmg_get_chip_type(NULL);
	if (err < 0)
		return err;
	/* set ODR 0HZ */
	err = bmg_set_datarate(NULL, LSM6DSM_REG_CTRL2_G_ODR_0HZ);
	if (err < 0)
		return err;
	/* set range and resolution */
	err = bmg_set_range(NULL, LSM6DSM_REG_CTRL2_G_FS_2000DPS);
	if (err < 0)
		return err;


	if (reset_cali != 0) {
		/*reset calibration only in power on */
		err = bmg_reset_calibration(obj);
		if (err < 0) {
			GYRO_PR_ERR("reset calibration failed.\n");
			return err;
		}
	}
#ifdef CONFIG_BMG_LOWPASS
	memset(&obj->gyro_fir, 0x00, sizeof(obj->gyro_fir));
#endif
	return 0;
}

/*
*Returns compensated and mapped value. unit is :degree/second
*/
static int bmg_read_sensor_data(struct lsm6dsmtr_accelgyro_data *obj, char *buf, int bufsize)
{
	s16 databuf[BMG_AXES_NUM] = { 0 };
	int gyro[BMG_AXES_NUM] = { 0 };
	int err = 0;

	obj = gyro_obj_data;

	err = bmg_read_raw_data(NULL, databuf);
	if (err) {
		GYRO_PR_ERR("bmg read raw data failed.\n");
		return err;
	}

	databuf[BMG_AXIS_X] = databuf[BMG_AXIS_X] * obj->gyro_reso->sensitivity / 100 * 131 / 1000;
	databuf[BMG_AXIS_Y] = databuf[BMG_AXIS_Y] * obj->gyro_reso->sensitivity / 100 * 131 / 1000;
	databuf[BMG_AXIS_Z] = databuf[BMG_AXIS_Z] * obj->gyro_reso->sensitivity / 100 * 131 / 1000;

	databuf[BMG_AXIS_X] += obj->gyro_cali_sw[BMG_AXIS_X];
	databuf[BMG_AXIS_Y] += obj->gyro_cali_sw[BMG_AXIS_Y];
	databuf[BMG_AXIS_Z] += obj->gyro_cali_sw[BMG_AXIS_Z];

	/*remap coordinate */
	gyro[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * databuf[BMG_AXIS_X];
	gyro[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * databuf[BMG_AXIS_Y];
	gyro[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * databuf[BMG_AXIS_Z];

	sprintf(buf, "%04x %04x %04x", gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y], gyro[BMG_AXIS_Z]);
	if (atomic_read(&obj->gyro_trace) & GYRO_TRC_IOCTL)
		GYRO_LOG("gyroscope data: %s\n", buf);
	return 0;
}

static ssize_t gyro_show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	return snprintf(buf, PAGE_SIZE, "%s\n", obj->gyro_sensor_name);
}

/*
* sensor data format is hex, unit:degree/second
*/
static ssize_t gyro_show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[BMG_BUFSIZE] = { 0 };

	bmg_read_sensor_data(NULL, strbuf, BMG_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*
* raw data format is s16, unit:LSB, axis mapped
*/
static ssize_t gyro_show_rawdata_value(struct device_driver *ddri, char *buf)
{
	s16 databuf[BMG_AXES_NUM] = { 0 };
	s16 dataraw[BMG_AXES_NUM] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	bmg_read_raw_data(NULL, dataraw);
	/*remap coordinate */
	databuf[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * dataraw[BMG_AXIS_X];
	databuf[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * dataraw[BMG_AXIS_Y];
	databuf[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * dataraw[BMG_AXIS_Z];
	return snprintf(buf, PAGE_SIZE, "%hd %hd %hd\n",
			databuf[BMG_AXIS_X], databuf[BMG_AXIS_Y], databuf[BMG_AXIS_Z]);
}

static ssize_t gyro_show_cali_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	int len = 0;
	int mul;
	int act[BMG_AXES_NUM] = { 0 };
	int raw[BMG_AXES_NUM] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	err = bmg_read_calibration(NULL, act, raw);
	if (err)
		return -EINVAL;

	mul = 1;		/* mul = sensor sensitivity / offset sensitivity */
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
			mul,
			obj->gyro_offset[BMG_AXIS_X],
			obj->gyro_offset[BMG_AXIS_Y],
			obj->gyro_offset[BMG_AXIS_Z],
			obj->gyro_offset[BMG_AXIS_X],
			obj->gyro_offset[BMG_AXIS_Y], obj->gyro_offset[BMG_AXIS_Z]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
			obj->gyro_cali_sw[BMG_AXIS_X],
			obj->gyro_cali_sw[BMG_AXIS_Y], obj->gyro_cali_sw[BMG_AXIS_Z]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[ALL]unmapped(%+3d, %+3d, %+3d), mapped(%+3d, %+3d, %+3d)\n",
			raw[BMG_AXIS_X], raw[BMG_AXIS_Y], raw[BMG_AXIS_Z],
			act[BMG_AXIS_X], act[BMG_AXIS_Y], act[BMG_AXIS_Z]);
	return len;
}

static ssize_t gyro_store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	int dat[BMG_AXES_NUM] = { 0 };
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	if (!strncmp(buf, "rst", 3)) {
		err = bmg_reset_calibration(obj);
		if (err)
			GYRO_PR_ERR("reset offset err = %d\n", err);
	} else if (sscanf(buf, "0x%02X 0x%02X 0x%02X",
			  &dat[BMG_AXIS_X], &dat[BMG_AXIS_Y], &dat[BMG_AXIS_Z]) == BMG_AXES_NUM) {
		err = bmg_write_calibration(NULL, dat);
		if (err)
			GYRO_PR_ERR("bmg write calibration failed, err = %d\n", err);
	} else {
		GYRO_INFO("invalid format\n");
	}
	return count;
}

static ssize_t gyro_show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMG_LOWPASS
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	if (atomic_read(&obj->gyro_firlen)) {
		int idx, len = atomic_read(&obj->gyro_firlen);

		GYRO_LOG("len = %2d, idx = %2d\n", obj->gyro_fir.num, obj->gyro_fir.idx);

		for (idx = 0; idx < len; idx++) {
			GYRO_LOG("[%5d %5d %5d]\n",
				 obj->gyro_fir.raw[idx][BMG_AXIS_X],
				 obj->gyro_fir.raw[idx][BMG_AXIS_Y],
				 obj->gyro_fir.raw[idx][BMG_AXIS_Z]);
		}

		GYRO_LOG("sum = [%5d %5d %5d]\n",
			 obj->gyro_fir.sum[BMG_AXIS_X],
			 obj->gyro_fir.sum[BMG_AXIS_Y], obj->gyro_fir.sum[BMG_AXIS_Z]);
		GYRO_LOG("avg = [%5d %5d %5d]\n",
			 obj->gyro_fir.sum[BMG_AXIS_X] / len,
			 obj->gyro_fir.sum[BMG_AXIS_Y] / len, obj->gyro_fir.sum[BMG_AXIS_Z] / len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->gyro_firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

static ssize_t gyro_store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_BMG_LOWPASS
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;
	int firlen;

	if (kstrtos32(buf, 10, &firlen) != 0) {
		GYRO_INFO("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GYRO_INFO("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->gyro_firlen, firlen);
		if (firlen == NULL) {
			atomic_set(&obj->gyro_fir_en, 0);
		} else {
			memset(&obj->gyro_fir, 0x00, sizeof(obj->gyro_fir));
			atomic_set(&obj->gyro_fir_en, 1);
		}
	}
#endif

	return count;
}

static ssize_t gyro_show_trace_value(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->gyro_trace));
	return res;
}

static ssize_t gyro_store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	if (kstrtos32(buf, 10, &trace) == 0)
		atomic_set(&obj->gyro_trace, trace);
	else
		GYRO_INFO("invalid content: '%s'\n", buf);
	return count;
}

static ssize_t gyro_show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	if (obj->hw)
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d\n", obj->hw->direction);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");

	len += snprintf(buf + len, PAGE_SIZE - len, "ver:%s\n", BMG_DRIVER_VERSION);
	return len;
}

static ssize_t gyro_show_power_mode_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%s mode\n",
			obj->lsm6dsm_gyro_power == BMG_NORMAL_MODE ? "normal" : "suspend");
	return len;
}

static ssize_t gyro_store_power_mode_value(struct device_driver *ddri, const char *buf,
					   size_t count)
{
	int err;
	unsigned long power_mode;

	err = kstrtoul(buf, 10, &power_mode);
	if (err < 0)
		return err;

	if (power_mode == BMI_GYRO_PM_NORMAL)
		err = bmg_set_powermode(NULL, true);
	else
		err = bmg_set_powermode(NULL, false);

	if (err < 0)
		GYRO_PR_ERR("set power mode failed.\n");

	return err;
}

static ssize_t gyro_show_range_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n", obj->gyro_range);
	return len;
}

static ssize_t gyro_store_range_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long range;
	int err;

	err = kstrtoul(buf, 10, &range);
	if (err == 0) {
		if ((range == BMG_RANGE_2000)
		    || (range == BMG_RANGE_1000)
		    || (range == BMG_RANGE_500)
		    || (range == BMG_RANGE_250)
		    || (range == BMG_RANGE_125)) {
			err = bmg_set_range(NULL, range);
			if (err) {
				GYRO_PR_ERR("set range value failed.\n");
				return err;
			}
		}
	}
	return count;
}

static ssize_t gyro_show_datarate_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct lsm6dsmtr_accelgyro_data *obj = gyro_obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n", obj->gyro_datarate);
	return len;
}

static ssize_t gyro_store_datarate_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long datarate;

	err = kstrtoul(buf, 10, &datarate);
	if (err < 0)
		return err;

	err = bmg_set_datarate(NULL, datarate);
	if (err < 0) {
		GYRO_PR_ERR("set data rate failed.\n");
		return err;
	}
	return count;
}

static DRIVER_ATTR(acc_chipinfo, S_IWUSR | S_IRUGO, accel_show_chipinfo_value, NULL);
static DRIVER_ATTR(acc_cpsdata, S_IWUSR | S_IRUGO, accel_show_cpsdata_value, NULL);
static DRIVER_ATTR(acc_range, S_IWUSR | S_IRUGO, accel_show_acc_range_value,
		   accel_store_acc_range_value);
static DRIVER_ATTR(acc_odr, S_IWUSR | S_IRUGO, accel_show_acc_odr_value, accel_store_acc_odr_value);
static DRIVER_ATTR(acc_sensordata, S_IWUSR | S_IRUGO, accel_show_sensordata_value, NULL);
static DRIVER_ATTR(acc_cali, S_IWUSR | S_IRUGO, accel_show_cali_value, accel_store_cali_value);
static DRIVER_ATTR(acc_firlen, S_IWUSR | S_IRUGO, accel_show_firlen_value,
		   accel_store_firlen_value);
static DRIVER_ATTR(acc_trace, S_IWUSR | S_IRUGO, accel_show_trace_value, accel_store_trace_value);
static DRIVER_ATTR(acc_status, S_IRUGO, accel_show_status_value, NULL);
static DRIVER_ATTR(acc_powerstatus, S_IRUGO, accel_show_power_status_value, NULL);

static DRIVER_ATTR(acc_layout, S_IRUGO | S_IWUSR, accel_show_layout_value,
		   accel_store_layout_value);

static DRIVER_ATTR(acc_delay, S_IRUGO | S_IWUSR, accel_lsm6dsmtr_delay_show,
		   accel_lsm6dsmtr_delay_store);

static DRIVER_ATTR(gyro_chipinfo, S_IWUSR | S_IRUGO, gyro_show_chipinfo_value, NULL);
static DRIVER_ATTR(gyro_sensordata, S_IWUSR | S_IRUGO, gyro_show_sensordata_value, NULL);
static DRIVER_ATTR(gyro_rawdata, S_IWUSR | S_IRUGO, gyro_show_rawdata_value, NULL);
static DRIVER_ATTR(gyro_cali, S_IWUSR | S_IRUGO, gyro_show_cali_value, gyro_store_cali_value);
static DRIVER_ATTR(gyro_firlen, S_IWUSR | S_IRUGO, gyro_show_firlen_value, gyro_store_firlen_value);
static DRIVER_ATTR(gyro_trace, S_IWUSR | S_IRUGO, gyro_show_trace_value, gyro_store_trace_value);
static DRIVER_ATTR(gyro_status, S_IRUGO, gyro_show_status_value, NULL);
static DRIVER_ATTR(gyro_op_mode, S_IWUSR | S_IRUGO, gyro_show_power_mode_value,
		   gyro_store_power_mode_value);
static DRIVER_ATTR(gyro_range, S_IWUSR | S_IRUGO, gyro_show_range_value, gyro_store_range_value);
static DRIVER_ATTR(gyro_odr, S_IWUSR | S_IRUGO, gyro_show_datarate_value,
		   gyro_store_datarate_value);

static struct driver_attribute *lsm6dsmtr_accelgyro_attr_list[] = {
	&driver_attr_acc_chipinfo,	/*chip information */
	&driver_attr_acc_sensordata,	/*dump sensor data */
	&driver_attr_acc_cali,	/*show calibration data */
	&driver_attr_acc_firlen,	/*filter length: 0: disable, others: enable */
	&driver_attr_acc_trace,	/*trace log */
	&driver_attr_acc_status,
	&driver_attr_acc_powerstatus,
	&driver_attr_acc_cpsdata,	/*g sensor data for compass tilt compensation */
	&driver_attr_acc_range,	/*g sensor range for compass tilt compensation */
	&driver_attr_acc_odr,	/*g sensor bandwidth for compass tilt compensation */

	&driver_attr_acc_layout,
	&driver_attr_acc_delay,

	/* chip information */
	&driver_attr_gyro_chipinfo,
	/* dump sensor data */
	&driver_attr_gyro_sensordata,
	/* dump raw data */
	&driver_attr_gyro_rawdata,
	/* show calibration data */
	&driver_attr_gyro_cali,
	/* filter length: 0: disable, others: enable */
	&driver_attr_gyro_firlen,
	/* trace flag */
	&driver_attr_gyro_trace,
	/* get hw configuration */
	&driver_attr_gyro_status,
	/* get power mode */
	&driver_attr_gyro_op_mode,
	/* get range */
	&driver_attr_gyro_range,
	/* get data rate */
	&driver_attr_gyro_odr,
};

static int lsm6dsmtr_accelgyro_create_attr(struct device_driver *driver)
{
	int err = 0;
	int idx = 0;
	int num = ARRAY_SIZE(lsm6dsmtr_accelgyro_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, lsm6dsmtr_accelgyro_attr_list[idx]);
		if (err) {
			ACC_PR_ERR("create driver file (%s) failed.\n",
				   lsm6dsmtr_accelgyro_attr_list[idx]->attr.name);
			break;
		}
	}
	return err;
}

static int lsm6dsmtr_accelgyro_delete_attr(struct device_driver *driver)
{
	int idx = 0;
	int err = 0;
	int num = ARRAY_SIZE(lsm6dsmtr_accelgyro_attr_list);

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, lsm6dsmtr_accelgyro_attr_list[idx]);
	return err;
}

#ifdef CONFIG_PM
static int lsm6dsmtr_acc_suspend(void)
{
	return 0;
}

static int lsm6dsmtr_acc_resume(void)
{
	return 0;
}


#ifdef CONFIG_PM
static int bmg_suspend(void)
{
	return 0;
}

static int bmg_resume(void)
{

	return 0;
}

static int pm_event_handler(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		lsm6dsmtr_acc_suspend();
		bmg_suspend();
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:
		lsm6dsmtr_acc_resume();
		bmg_resume();
		return NOTIFY_DONE;
	}
	return NOTIFY_DONE;
}

static struct notifier_block pm_notifier_func = {
	.notifier_call = pm_event_handler,
	.priority = 0,
};
#endif				/* CONFIG_PM */


#ifdef CONFIG_OF
static const struct of_device_id accelgyrosensor_of_match[] = {
	{.compatible = "mediatek,accelgyrosensor",},
	{},
};
#endif


static struct spi_driver lsm6dsmtr_accelgyro_spi_driver = {
	.driver = {
		   .name = LSM6DSMSTR_ACCELGYRO_DEV_NAME,
		   .bus = &spi_bus_type,
#ifdef CONFIG_OF
		   .of_match_table = accelgyrosensor_of_match,
#endif
		   },
	.probe = lsm6dsmtr_accelgyro_spi_probe,
	.remove = lsm6dsmtr_accelgyro_spi_remove,
};


/*!
 * @brief If use this type of enable, Gsensor only enabled but not report inputEvent to HAL
 *
 * @param[in] int enable true or false
 *
 * @return zero success, non-zero failed
 */
static int lsm6dsmtr_acc_enable_nodata(int en)
{
	int err = 0;

	err = LSM6DSMSTR_ACC_SetPowerMode(accelgyro_obj_data, en == 1 ? true : false);
	if (err < 0) {
		ACC_PR_ERR("LSM6DSMSTR_ACC_SetPowerMode failed.\n");
		return -1;
	}
	ACC_LOG("lsm6dsmtr_acc_enable_nodata ok!\n");
	return err;
}

/*!
 * @brief set ODR rate value for acc
 *
 * @param[in] u64 ns for sensor delay
 *
 * @return zero success, non-zero failed
 */

static int lsm6dsmtr_acc_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int value = 0;
	int sample_delay = 0;
	int err = 0;

	value = (int)samplingPeriodNs / 1000 / 1000;

	if (value <= 5)
		sample_delay = LSM6DSM_REG_CTRL1_XL_ODR_208HZ;
	else if (value <= 10)
		sample_delay = LSM6DSM_REG_CTRL1_XL_ODR_104HZ;
	else if (value <= 20)
		sample_delay = LSM6DSM_REG_CTRL1_XL_ODR_52HZ;
	else
		sample_delay = LSM6DSM_REG_CTRL1_XL_ODR_26HZ;
	err = LSM6DSMSTR_ACC_SetBWRate(accelgyro_obj_data, sample_delay);
	if (err < 0) {
		ACC_PR_ERR("set delay parameter error!\n");
		return -1;
	}
	ACC_LOG("lsm6dsmtr acc set delay = (%d) ok.\n", value);
	return 0;
}

static int lsm6dsmtr_acc_flush(void)
{
	return acc_flush_report();
}

/*!
 * @brief get the raw data for gsensor
 *
 * @param[in] int x axis value
 * @param[in] int y axis value
 * @param[in] int z axis value
 * @param[in] int status
 *
 * @return zero success, non-zero failed
 */
static int lsm6dsmtr_acc_get_data(int *x, int *y, int *z, int *status)
{
	int err = 0;
	char buff[LSM6DSMSTR_BUFSIZE];
	struct lsm6dsmtr_accelgyro_data *obj = accelgyro_obj_data;

	err = LSM6DSMSTR_ACC_ReadSensorData(obj, buff, LSM6DSMSTR_BUFSIZE);
	if (err < 0) {
		ACC_PR_ERR("lsm6dsmtr_acc_get_data failed.\n");
		return err;
	}
	err = sscanf(buff, "%x %x %x", x, y, z);

	if (err < 0) {
		ACC_PR_ERR("lsm6dsmtr_acc_get_data failed.\n");
		return err;
	}
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	return 0;
}

/*srx should be confirmed*/
int gyroscope_operate(void *self, uint32_t command, void *buff_in, int size_in,
		      void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	char buff[BMG_BUFSIZE] = { 0 };
	struct lsm6dsmtr_accelgyro_data *priv = (struct lsm6dsmtr_accelgyro_data *)self;
	struct hwm_sensor_data *gyroscope_data;

	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			GYRO_PR_ERR("set delay parameter error\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;

			/*
			 *Currently, fix data rate to 100Hz.
			 */
			sample_delay = LSM6DSM_REG_CTRL2_G_ODR_104HZ;

			GYRO_LOG("sensor delay command: %d, sample_delay = %d\n",
				 value, sample_delay);

			err = bmg_set_datarate(NULL, sample_delay);
			if (err < 0)
				GYRO_INFO("set delay parameter error\n");

			if (value >= 40)
				atomic_set(&priv->gyro_filter, 0);
			else {
#if defined(CONFIG_BMG_LOWPASS)
				priv->gyro_fir.num = 0;
				priv->gyro_fir.idx = 0;
				priv->gyro_fir.sum[BMG_AXIS_X] = 0;
				priv->gyro_fir.sum[BMG_AXIS_Y] = 0;
				priv->gyro_fir.sum[BMG_AXIS_Z] = 0;
				atomic_set(&priv->gyro_filter, 1);
#endif
			}
		}
		break;
	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			GYRO_PR_ERR("enable sensor parameter error\n");
			err = -EINVAL;
		} else {
			/* value:[0--->suspend, 1--->normal] */
			value = *(int *)buff_in;
			GYRO_LOG("sensor enable/disable command: %s\n",
				 value ? "enable" : "disable");

			err = bmg_set_powermode(NULL, (enum BMG_POWERMODE_ENUM)(!!value));
			if (err)
				GYRO_PR_ERR("set power mode failed, err = %d\n", err);
		}
		break;
	case SENSOR_GET_DATA:
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			GYRO_PR_ERR("get sensor data parameter error\n");
			err = -EINVAL;
		} else {
			gyroscope_data = (struct hwm_sensor_data *)buff_out;
			bmg_read_sensor_data(NULL, buff, BMG_BUFSIZE);
			err = sscanf(buff, "%x %x %x", &gyroscope_data->values[0],
				     &gyroscope_data->values[1], &gyroscope_data->values[2]);
			if (err)
				GYRO_INFO("get data failed, err = %d\n", err);
			gyroscope_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
			gyroscope_data->value_divide = DEGREE_TO_RAD;
		}
		break;
	default:
		GYRO_INFO("gyroscope operate function no this parameter %d\n", command);
		err = -1;
		break;
	}

	return err;
}

#endif				/* CONFIG_PM */

static int lsm6dsmtr_gyro_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

static int lsm6dsmtr_gyro_enable_nodata(int en)
{
	int err = 0;
	int retry = 0;

	for (retry = 0; retry < 3; retry++) {
		err = bmg_set_powermode(NULL, en == 1 ? true : false);
		if (err == 0) {
			GYRO_LOG("lsm6dsmtr_gyro_SetPowerMode ok.\n");
			break;
		}
	}
	if (err < 0)
		GYRO_LOG("lsm6dsmtr_gyro_SetPowerMode fail!\n");

	return err;
}

static int lsm6dsmtr_gyro_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int err;
	int value = (int)samplingPeriodNs / 1000 / 1000;

	int sample_delay = 0;
	struct lsm6dsmtr_accelgyro_data *priv = gyro_obj_data;

	if (value <= 5)
		sample_delay = LSM6DSM_REG_CTRL2_G_ODR_208HZ;
	else if (value <= 10)
		sample_delay = LSM6DSM_REG_CTRL2_G_ODR_104HZ;
	else if (value <= 20)
		sample_delay = LSM6DSM_REG_CTRL2_G_ODR_52HZ;
	else
		sample_delay = LSM6DSM_REG_CTRL2_G_ODR_26HZ;

	err = bmg_set_datarate(NULL, sample_delay);
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
	if (value >= 50)
		atomic_set(&priv->gyro_filter, 0);
	else {
#if defined(CONFIG_BMG_LOWPASS)
		priv->gyro_fir.num = 0;
		priv->gyro_fir.idx = 0;
		priv->gyro_fir.sum[BMG_AXIS_X] = 0;
		priv->gyro_fir.sum[BMG_AXIS_Y] = 0;
		priv->gyro_fir.sum[BMG_AXIS_Z] = 0;
		atomic_set(&priv->gyro_filter, 1);
#endif
	}
	GYRO_LOG("set gyro delay = %d\n", sample_delay);
	return 0;
}

static int lsm6dsmtr_gyro_flush(void)
{
	return gyro_flush_report();
}

static int lsm6dsmtr_gyro_get_data(int *x, int *y, int *z, int *status)
{
	char buff[BMG_BUFSIZE] = { 0 };

	bmg_read_sensor_data(NULL, buff, BMG_BUFSIZE);
	if (sscanf(buff, "%x %x %x", x, y, z) != 3)
		GYRO_PR_ERR("lsm6dsmtr_gyro_get_data failed");
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	return 0;
}

static int lsm6dsmtr_accel_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err;

	err = lsm6dsmtr_acc_enable_nodata(enabledisable == true ? 1 : 0);
	if (err) {
		ACC_PR_ERR("%s enable sensor failed!\n", __func__);
		return -1;
	}
	err = lsm6dsmtr_acc_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		ACC_PR_ERR("%s enable set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int lsm6dsmtr_accel_factory_get_data(int32_t data[3], int *status)
{
	return lsm6dsmtr_acc_get_data(&data[0], &data[1], &data[2], status);
}

static int lsm6dsmtr_accel_factory_get_raw_data(int32_t data[3])
{
	char strbuf[LSM6DSMSTR_BUFSIZE] = { 0 };

	LSM6DSMSTR_ACC_ReadRawData(accelgyro_obj_data, strbuf);
	ACC_PR_ERR("don't support lsm6dsmtr_factory_get_raw_data!\n");
	return 0;
}

static int lsm6dsmtr_accel_factory_enable_calibration(void)
{
	return 0;
}

static int lsm6dsmtr_accel_factory_clear_cali(void)
{
	int err = 0;

	err = LSM6DSMSTR_ACC_ResetCalibration(accelgyro_obj_data);
	if (err) {
		ACC_PR_ERR("lsm6dsmtr_ResetCalibration failed!\n");
		return -1;
	}
	return 0;
}

static int lsm6dsmtr_accel_factory_set_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	cali[LSM6DSMSTR_ACC_AXIS_X] = data[0]
	    * accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	cali[LSM6DSMSTR_ACC_AXIS_Y] = data[1]
	    * accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	cali[LSM6DSMSTR_ACC_AXIS_Z] = data[2]
	    * accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	err = LSM6DSMSTR_ACC_WriteCalibration(accelgyro_obj_data, cali);
	if (err) {
		ACC_PR_ERR("lsm6dsmtr_WriteCalibration failed!\n");
		return -1;
	}
	return 0;
}

static int lsm6dsmtr_accel_factory_get_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	err = LSM6DSMSTR_ACC_ReadCalibration(accelgyro_obj_data, cali);
	if (err) {
		ACC_PR_ERR("lsm6dsmtr_ReadCalibration failed!\n");
		return -1;
	}
	data[0] = cali[LSM6DSMSTR_ACC_AXIS_X]
	    * GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	data[1] = cali[LSM6DSMSTR_ACC_AXIS_Y]
	    * GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	data[2] = cali[LSM6DSMSTR_ACC_AXIS_Z]
	    * GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	return 0;
}

static int lsm6dsmtr_accel_factory_do_self_test(void)
{
	return 0;
}

static struct accel_factory_fops lsm6dsmtr_accel_factory_fops = {
	.enable_sensor = lsm6dsmtr_accel_factory_enable_sensor,
	.get_data = lsm6dsmtr_accel_factory_get_data,
	.get_raw_data = lsm6dsmtr_accel_factory_get_raw_data,
	.enable_calibration = lsm6dsmtr_accel_factory_enable_calibration,
	.clear_cali = lsm6dsmtr_accel_factory_clear_cali,
	.set_cali = lsm6dsmtr_accel_factory_set_cali,
	.get_cali = lsm6dsmtr_accel_factory_get_cali,
	.do_self_test = lsm6dsmtr_accel_factory_do_self_test,
};

static struct accel_factory_public lsm6dsmtr_accel_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &lsm6dsmtr_accel_factory_fops,
};


/*!
 * @brief
 *	This API reads the data from
 *	the given register
 *
 *	@param v_addr_u8 -> Address of the register
 *	@param v_data_u8 -> The data from the register
 *	@param v_len_u8 -> no of bytes to read
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 */
LSM6DSMSTR_RETURN_FUNCTION_TYPE lsm6dsmtr_read_reg(u8 v_addr_u8, u8 *v_data_u8, u8 v_len_u8)
{
	/* variable used for return the status of communication result */
	LSM6DSMSTR_RETURN_FUNCTION_TYPE com_rslt = E_LSM6DSMSTR_COMM_RES;
	/* check the p_lsm6dsmtr structure as NULL */
	if (p_lsm6dsmtr == LSM6DSMSTR_NULL)
		return E_LSM6DSMSTR_NULL_PTR;

	/* Read data from register */
	com_rslt =
	    p_lsm6dsmtr->LSM6DSMSTR_BUS_READ_FUNC(p_lsm6dsmtr->dev_addr,
						  v_addr_u8, v_data_u8, v_len_u8);

	return com_rslt;
}

#ifdef MTK_NEW_ARCH_ACCEL
static int lsm6dsmtracc_probe(struct platform_device *pdev)
{
	accelPltFmDev = pdev;
	return 0;
}

static int lsm6dsmtracc_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id lsm6dsmtr_of_match[] = {
	{.compatible = "mediatek,st_step_counter",},
	{},
};
#endif

static struct platform_driver lsm6dsmtracc_driver = {
	.probe = lsm6dsmtracc_probe,
	.remove = lsm6dsmtracc_remove,
	.driver = {
		   .name = "stepcounter",
		   /* .owner  = THIS_MODULE, */
#ifdef CONFIG_OF
		   .of_match_table = lsm6dsmtr_of_match,
#endif
		   }
};

#endif

static int bmg_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err = 0;

	err = lsm6dsmtr_gyro_enable_nodata(enabledisable == true ? 1 : 0);
	if (err) {
		GYRO_PR_ERR("%s enable failed!\n", __func__);
		return -1;
	}
	err = lsm6dsmtr_gyro_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		GYRO_PR_ERR("%s set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}

static int bmg_factory_get_data(int32_t data[3], int *status)
{
	return lsm6dsmtr_gyro_get_data(&data[0], &data[1], &data[2], status);
}

static int bmg_factory_get_raw_data(int32_t data[3])
{
	GYRO_LOG("don't support bmg_factory_get_raw_data!\n");
	return 0;
}

static int bmg_factory_enable_calibration(void)
{
	return 0;
}

static int bmg_factory_clear_cali(void)
{
	int err = 0;

	err = bmg_reset_calibration(gyro_obj_data);
	if (err) {
		GYRO_PR_ERR("bmg_ResetCalibration failed!\n");
		return -1;
	}
	return 0;
}

static int bmg_factory_set_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	cali[BMG_AXIS_X] = data[0] * gyro_obj_data->gyro_sensitivity / LSM6DSMSTR_FS_250_LSB;
	cali[BMG_AXIS_Y] = data[1] * gyro_obj_data->gyro_sensitivity / LSM6DSMSTR_FS_250_LSB;
	cali[BMG_AXIS_Z] = data[2] * gyro_obj_data->gyro_sensitivity / LSM6DSMSTR_FS_250_LSB;
	err = bmg_write_calibration(gyro_obj_data, cali);
	if (err) {
		GYRO_PR_ERR("bmg_WriteCalibration failed!\n");
		return -1;
	}
	return 0;
}

static int bmg_factory_get_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };
	int raw_offset[BMG_BUFSIZE] = { 0 };

	err = bmg_read_calibration(NULL, cali, raw_offset);
	if (err) {
		GYRO_PR_ERR("bmg_ReadCalibration failed!\n");
		return -1;
	}
	data[0] = cali[BMG_AXIS_X] * LSM6DSMSTR_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
	data[1] = cali[BMG_AXIS_Y] * LSM6DSMSTR_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
	data[2] = cali[BMG_AXIS_Z] * LSM6DSMSTR_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
	return 0;
}

static int bmg_factory_do_self_test(void)
{
	return 0;
}

static struct gyro_factory_fops bmg_factory_fops = {
	.enable_sensor = bmg_factory_enable_sensor,
	.get_data = bmg_factory_get_data,
	.get_raw_data = bmg_factory_get_raw_data,
	.enable_calibration = bmg_factory_enable_calibration,
	.clear_cali = bmg_factory_clear_cali,
	.set_cali = bmg_factory_set_cali,
	.get_cali = bmg_factory_get_cali,
	.do_self_test = bmg_factory_do_self_test,
};

static struct gyro_factory_public bmg_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &bmg_factory_fops,
};

static int lsm6dsmtr_accelgyro_spi_probe(struct spi_device *spi)
{

	int err = 0;
	/* accel sensor */
	struct lsm6dsmtr_accelgyro_data *obj;
	struct acc_control_path accel_ctl = { 0 };
	struct acc_data_path accel_data = { 0 };

	struct gyro_control_path gyro_ctl = { 0 };
	struct gyro_data_path gyro_data = { 0 };

	hw = get_accelgyro_dts_func(spi->dev.of_node, hw);
	if (!hw) {
		ACC_PR_ERR("device tree configuration error!\n");
		return 0;
	}

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(struct lsm6dsmtr_accelgyro_data));
	obj->hw = hw;
	/* map direction */
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		ACC_PR_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit;
	}

	/* Spi start,Initialize the driver data */
	obj->spi = spi;

	/* setup SPI parameters */
	/* CPOL=CPHA=0, speed 1MHz */
	obj->spi->mode = SPI_MODE_0;
	obj->spi->bits_per_word = 8;
	obj->spi->max_speed_hz = 1 * 1000 * 1000;

	spi_set_drvdata(spi, obj);
	/* allocate buffer for SPI transfer */
	obj->spi_buffer = kzalloc(bufsiz, GFP_KERNEL);
	if (obj->spi_buffer == NULL) {
		ACC_PR_ERR("kzalloc spi_buffer fail.\n");
		goto err_buf;
	}

	/*Spi end */

	/* acc */
	accelgyro_obj_data = obj;
	atomic_set(&obj->accel_trace, 0);
	atomic_set(&obj->accel_suspend, 0);
	mutex_init(&obj->accel_lock);

	/* gyro */
	gyro_obj_data = obj;
	atomic_set(&obj->gyro_trace, 0);
	atomic_set(&obj->gyro_suspend, 0);
	/* obj->gyro_power_mode = BMG_UNDEFINED_POWERMODE; */
	obj->gyro_range = BMG_UNDEFINED_RANGE;
	obj->gyro_datarate = LSM6DSMSTR_GYRO_ODR_RESERVED;
	mutex_init(&obj->gyro_lock);
#ifdef CONFIG_BMG_LOWPASS
	if (obj->hw->gyro_firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->gyro_firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->gyro_firlen, obj->hw->gyro_firlen);

	if (atomic_read(&obj->gyro_firlen) > 0)
		atomic_set(&obj->gyro_fir_en, 1);
#endif
	/*gyro end */

	mutex_init(&obj->spi_lock);

	err = lsm6dsmtr_acc_init_client(obj, 1);
	if (err)
		goto exit_init_failed;

	err = bmg_init_client(obj, 1);
	if (err)
		goto exit_init_client_failed;


#ifdef MTK_NEW_ARCH_ACCEL
	platform_driver_register(&lsm6dsmtracc_driver);
#endif

	/* acc factory */
	err = accel_factory_device_register(&lsm6dsmtr_accel_factory_device);
	if (err) {
		ACC_PR_ERR("acc_factory register failed.\n");
		goto exit_misc_device_register_failed;
	}
	/* gyro factory */
	/* err = misc_register(&bmg_device); */
	err = gyro_factory_device_register(&bmg_factory_device);
	if (err) {
		GYRO_PR_ERR("misc device register failed, err = %d\n", err);
		goto exit_misc_device_register_failed;
	}

	err = lsm6dsmtr_accelgyro_create_attr(&(lsm6dsmtr_accelgyro_spi_driver.driver));
	if (err) {
		ACC_PR_ERR("create attribute failed.\n");
		goto exit_create_attr_failed;
	}

	/* Init control path,acc */
	accel_ctl.enable_nodata = lsm6dsmtr_acc_enable_nodata;
	accel_ctl.batch = lsm6dsmtr_acc_batch;
	accel_ctl.flush = lsm6dsmtr_acc_flush;
	accel_ctl.is_support_batch = false;	/* using customization info */
	accel_ctl.is_report_input_direct = false;

	err = acc_register_control_path(&accel_ctl);
	if (err) {
		ACC_PR_ERR("register acc control path error.\n");
		goto exit_kfree;
	}
	/* gyro */
	gyro_ctl.open_report_data = lsm6dsmtr_gyro_open_report_data;
	gyro_ctl.enable_nodata = lsm6dsmtr_gyro_enable_nodata;
	gyro_ctl.batch = lsm6dsmtr_gyro_batch;
	gyro_ctl.flush = lsm6dsmtr_gyro_flush;
	gyro_ctl.is_report_input_direct = false;
	gyro_ctl.is_support_batch = obj->hw->gyro_is_batch_supported;
	ACC_LOG(">>gyro_register_control_path\n");
	err = gyro_register_control_path(&gyro_ctl);
	if (err) {
		GYRO_PR_ERR("register gyro control path err\n");
		goto exit_create_attr_failed;
	}

	/* Init data path,acc */
	accel_data.get_data = lsm6dsmtr_acc_get_data;
	accel_data.vender_div = 1000;
	err = acc_register_data_path(&accel_data);
	if (err) {
		ACC_PR_ERR("register acc data path error.\n");
		goto exit_kfree;
	}
	/* gyro */
	gyro_data.get_data = lsm6dsmtr_gyro_get_data;
	gyro_data.vender_div = DEGREE_TO_RAD;
	err = gyro_register_data_path(&gyro_data);
	if (err) {
		GYRO_PR_ERR("gyro_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}
	/* acc and gyro */
#ifdef CONFIG_PM
	err = register_pm_notifier(&pm_notifier_func);
	if (err) {
		ACC_PR_ERR("Failed to register PM notifier.\n");
		goto exit_kfree;
	}
#endif				/* CONFIG_PM */

	/* h/w init */
	obj->device.bus_read = bmi_read_wrapper;
	obj->device.bus_write = bmi_write_wrapper;
	obj->device.delay_msec = bmi_delay;
	lsm6dsmtr_init(&obj->device);

	lsm6dsmtr_acc_init_flag = 0;
	lsm6dsmtr_gyro_init_flag = 0;

	ACC_LOG("%s: is ok.\n", __func__);
	return 0;

exit_create_attr_failed:
	/* misc_deregister(&lsm6dsmtr_acc_device); */
	/* misc_deregister(&bmg_device); */

exit_misc_device_register_failed:
exit_init_failed:
err_buf:
	kfree(obj->spi_buffer);
	spi_set_drvdata(spi, NULL);
	obj->spi = NULL;
exit_kfree:
exit_init_client_failed:
	kfree(obj);

exit:
	ACC_PR_ERR("%s: err = %d\n", __func__, err);
	lsm6dsmtr_acc_init_flag = -1;
	lsm6dsmtr_gyro_init_flag = -1;
	return err;
}

static int lsm6dsmtr_accelgyro_spi_remove(struct spi_device *spi)
{
	struct lsm6dsmtr_accelgyro_data *obj = spi_get_drvdata(spi);
	int err = 0;

	err = lsm6dsmtr_accelgyro_delete_attr(&(lsm6dsmtr_accelgyro_spi_driver.driver));
	if (err)
		ACC_PR_ERR("delete device attribute failed.\n");

	if (obj->spi_buffer != NULL) {
		kfree(obj->spi_buffer);
		obj->spi_buffer = NULL;
	}

	spi_set_drvdata(spi, NULL);
	obj->spi = NULL;

	/* misc_deregister(&lsm6dsmtr_acc_device); */
	accel_factory_device_deregister(&lsm6dsmtr_accel_factory_device);
	gyro_factory_device_deregister(&bmg_factory_device);

	kfree(accelgyro_obj_data);
	return 0;
}

static int lsm6dsmtr_accelgyro_local_init(void)
{
	ACC_LOG(">>>>-lsm6dsmtr_accelgyro_local_init.\n");
	if (spi_register_driver(&lsm6dsmtr_accelgyro_spi_driver))
		return -1;
	if ((-1 == lsm6dsmtr_acc_init_flag) || (-1 == lsm6dsmtr_gyro_init_flag))
		return -1;
	ACC_LOG("<<<<<-lsm6dsmtr_accelgyro_local_init.\n");
	return 0;
}

static int lsm6dsmtr_accelgyro_remove(void)
{
	ACC_FUN();
	spi_unregister_driver(&lsm6dsmtr_accelgyro_spi_driver);
	return 0;
}

static struct accelgyro_init_info lsm6dsmtr_accelgyro_init_info = {
	.name = LSM6DSMSTR_ACCELGYRO_DEV_NAME,
	.init = lsm6dsmtr_accelgyro_local_init,
	.uninit = lsm6dsmtr_accelgyro_remove,
};

static int __init lsm6dsmtr_accelgyro_init(void)
{
	ACC_FUN();
	accelgyro_driver_add(&lsm6dsmtr_accelgyro_init_info);
	return 0;
}

static void __exit lsm6dsmtr_accelgyro_exit(void)
{
	ACC_FUN();
}

module_init(lsm6dsmtr_accelgyro_init);
module_exit(lsm6dsmtr_accelgyro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LSM6DSMSTR_ACC_GYRO SPI driver");
MODULE_AUTHOR("ruixue.su@mediatek.com");

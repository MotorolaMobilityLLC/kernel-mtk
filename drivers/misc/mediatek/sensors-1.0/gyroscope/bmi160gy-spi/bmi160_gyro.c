/* BOSCH Gyroscope Sensor Driver
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
 * History: V1.0 --- [2013.01.29]Driver creation
 *          V1.1 --- [2013.03.28]
 *                   1.Instead late_resume, use resume to make sure
 *                     driver resume is ealier than processes resume.
 *                   2.Add power mode setting in read data.
 *          V1.2 --- [2013.06.28]Add self test function.
 *          V1.3 --- [2013.07.26]Fix the bug of wrong axis remapping
 *                   in rawdata inode.
 */

#include <linux/interrupt.h>
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
#include <linux/suspend.h>

#include <gyroscope.h>
#include <cust_gyro.h>
#include <hwmsensor.h>
#include "bmi160_gyro.h"

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
/* MTK header */
#include "mtk_spi.h"
#include "mtk_spi_hal.h"

/* sensor type */
enum SENSOR_TYPE_ENUM {
	BMI160_GYRO_TYPE = 0x0,
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
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
	int sum[BMG_AXES_NUM];
	int num;
	int idx;
};

/* bmg data */
struct bmg_gyro_data {
	struct gyro_hw *hw;
	struct hwmsen_convert cvt;
	/* sensor info */
	u8 sensor_name[MAX_SENSOR_NAME];
	enum SENSOR_TYPE_ENUM sensor_type;
	enum BMG_POWERMODE_ENUM power_mode;
	enum BMG_RANGE_ENUM range;
	int datarate;
	/* sensitivity = 2^bitnum/range
	 * [+/-2000 = 4000; +/-1000 = 2000;
	 * +/-500 = 1000; +/-250 = 500;
	 * +/-125 = 250 ]
	 */
	u16 sensitivity;
	/*misc */
	struct mutex lock;
	atomic_t trace;
	atomic_t suspend;
	atomic_t filter;
	/* unmapped axis value */
	s16 cali_sw[BMG_AXES_NUM + 1];
	/* hw offset */
	s8 offset[BMG_AXES_NUM + 1];	/* +1:for 4-byte alignment */

#if defined(CONFIG_BMG_LOWPASS)
	atomic_t firlen;
	atomic_t fir_en;
	struct data_filter fir;
#endif
};

struct gyro_hw gyro_cust;
static struct gyro_hw *hw = &gyro_cust;

static struct gyro_init_info bmi160_gyro_init_info;
/* 0=OK, -1=fail */
static int bmi160_gyro_init_flag = -1;
static struct bmg_gyro_data *obj_data;
static int bmg_set_powermode(struct bmg_gyro_data *obj, enum BMG_POWERMODE_ENUM power_mode);

static int bmg_read_raw_data(struct bmg_gyro_data *obj, s16 data[BMG_AXES_NUM])
{
	int err = 0;
	struct bmg_gyro_data *priv = obj_data;

	if (priv->power_mode == BMG_SUSPEND_MODE) {
		err = bmg_set_powermode(NULL, (enum BMG_POWERMODE_ENUM)BMG_NORMAL_MODE);
		if (err < 0) {
			GYRO_PR_ERR("set power mode failed, err = %d\n", err);
			return err;
		}
	}
	if (priv->sensor_type == BMI160_GYRO_TYPE) {
		u8 buf_tmp[BMG_DATA_LEN] = { 0 };

		err = BMP160_spi_read_bytes(NULL, BMI160_USER_DATA_8_GYR_X_LSB__REG, buf_tmp, 6);
		if (err) {
			GYRO_PR_ERR("read gyro raw data failed.\n");
			return err;
		}
		/* Data X */
		data[BMG_AXIS_X] = (s16)
		    ((((s32) ((s8) buf_tmp[1]))
		      << BMI160_SHIFT_8_POSITION) | (buf_tmp[0]));
		/* Data Y */
		data[BMG_AXIS_Y] = (s16)
		    ((((s32) ((s8) buf_tmp[3]))
		      << BMI160_SHIFT_8_POSITION) | (buf_tmp[2]));
		/* Data Z */
		data[BMG_AXIS_Z] = (s16)
		    ((((s32) ((s8) buf_tmp[5]))
		      << BMI160_SHIFT_8_POSITION) | (buf_tmp[4]));
		if (atomic_read(&priv->trace) & GYRO_TRC_RAWDATA) {
			GYRO_LOG("[%s][16bit raw][%08X %08X %08X] => [%5d %5d %5d]\n",
				 priv->sensor_name,
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
	if (atomic_read(&priv->filter)) {
		if (atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend)) {
			int idx, firlen = atomic_read(&priv->firlen);

			if (priv->fir.num < firlen) {
				priv->fir.raw[priv->fir.num][BMG_AXIS_X] = data[BMG_AXIS_X];
				priv->fir.raw[priv->fir.num][BMG_AXIS_Y] = data[BMG_AXIS_Y];
				priv->fir.raw[priv->fir.num][BMG_AXIS_Z] = data[BMG_AXIS_Z];
				priv->fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
				if (atomic_read(&priv->trace) & GYRO_TRC_FILTER) {
					GYRO_LOG("add [%2d][%5d %5d %5d] => [%5d %5d %5d]\n",
						 priv->fir.num,
						 priv->fir.raw
						 [priv->fir.num][BMG_AXIS_X],
						 priv->fir.raw
						 [priv->fir.num][BMG_AXIS_Y],
						 priv->fir.raw
						 [priv->fir.num][BMG_AXIS_Z],
						 priv->fir.sum[BMG_AXIS_X],
						 priv->fir.sum[BMG_AXIS_Y],
						 priv->fir.sum[BMG_AXIS_Z]);
				}
				priv->fir.num++;
				priv->fir.idx++;
			} else {
				idx = priv->fir.idx % firlen;
				priv->fir.sum[BMG_AXIS_X] -= priv->fir.raw[idx][BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] -= priv->fir.raw[idx][BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] -= priv->fir.raw[idx][BMG_AXIS_Z];
				priv->fir.raw[idx][BMG_AXIS_X] = data[BMG_AXIS_X];
				priv->fir.raw[idx][BMG_AXIS_Y] = data[BMG_AXIS_Y];
				priv->fir.raw[idx][BMG_AXIS_Z] = data[BMG_AXIS_Z];
				priv->fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
				priv->fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
				priv->fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
				priv->fir.idx++;
				data[BMG_AXIS_X] = priv->fir.sum[BMG_AXIS_X] / firlen;
				data[BMG_AXIS_Y] = priv->fir.sum[BMG_AXIS_Y] / firlen;
				data[BMG_AXIS_Z] = priv->fir.sum[BMG_AXIS_Z] / firlen;
				if (atomic_read(&priv->trace) & GYRO_TRC_FILTER) {
					GYRO_LOG("add [%2d][%5d %5d %5d] =>[%5d %5d %5d] : [%5d %5d %5d]\n",
						 idx,
						 priv->fir.raw[idx][BMG_AXIS_X],
						 priv->fir.raw[idx][BMG_AXIS_Y],
						 priv->fir.raw[idx][BMG_AXIS_Z],
						 priv->fir.sum[BMG_AXIS_X],
						 priv->fir.sum[BMG_AXIS_Y],
						 priv->fir.sum[BMG_AXIS_Z],
						 data[BMG_AXIS_X],
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
static int bmg_get_hw_offset(struct bmg_gyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
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
static int bmg_set_hw_offset(struct bmg_gyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;
	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
		 offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);
	return err;
}
#endif

static int bmg_reset_calibration(struct bmg_gyro_data *obj)
{
	int err = 0;
#ifdef SW_CALIBRATION

#else
	err = bmg_set_hw_offset(NULL, obj->offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed, %d\n", err);
		return err;
	}
#endif
	memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
	memset(obj->offset, 0x00, sizeof(obj->offset));
	return err;
}

static int bmg_read_calibration(struct bmg_gyro_data *obj,
				int act[BMG_AXES_NUM], int raw[BMG_AXES_NUM])
{
	/*
	 *raw: the raw calibration data, unmapped;
	 *act: the actual calibration data, mapped
	 */
	int err = 0;
	int mul;

	obj = obj_data;

#ifdef SW_CALIBRATION
	/* only sw calibration, disable hw calibration */
	mul = 0;
#else
	err = bmg_get_hw_offset(NULL, obj->offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed, %d\n", err);
		return err;
	}
	mul = 1;		/* mul = sensor sensitivity / offset sensitivity */
#endif

	raw[BMG_AXIS_X] = obj->offset[BMG_AXIS_X] * mul + obj->cali_sw[BMG_AXIS_X];
	raw[BMG_AXIS_Y] = obj->offset[BMG_AXIS_Y] * mul + obj->cali_sw[BMG_AXIS_Y];
	raw[BMG_AXIS_Z] = obj->offset[BMG_AXIS_Z] * mul + obj->cali_sw[BMG_AXIS_Z];

	act[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * raw[BMG_AXIS_X];
	act[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * raw[BMG_AXIS_Y];
	act[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * raw[BMG_AXIS_Z];
	return err;
}

static int bmg_write_calibration(struct bmg_gyro_data *obj, int dat[BMG_AXES_NUM])
{
	/* dat array : Android coordinate system, mapped, unit:LSB */
	int err = 0;
	int cali[BMG_AXES_NUM] = { 0 };
	int raw[BMG_AXES_NUM] = { 0 };

	obj = obj_data;
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
	obj->cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X] * (cali[obj->cvt.map[BMG_AXIS_X]]);
	obj->cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y] * (cali[obj->cvt.map[BMG_AXIS_Y]]);
	obj->cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z] * (cali[obj->cvt.map[BMG_AXIS_Z]]);
#else
	/* divisor = sensor sensitivity / offset sensitivity */
	int divisor = 1;

	obj->offset[BMG_AXIS_X] = (s8) (obj->cvt.sign[BMG_AXIS_X] *
					(cali[obj->cvt.map[BMG_AXIS_X]]) / (divisor));
	obj->offset[BMG_AXIS_Y] = (s8) (obj->cvt.sign[BMG_AXIS_Y] *
					(cali[obj->cvt.map[BMG_AXIS_Y]]) / (divisor));
	obj->offset[BMG_AXIS_Z] = (s8) (obj->cvt.sign[BMG_AXIS_Z] *
					(cali[obj->cvt.map[BMG_AXIS_Z]]) / (divisor));

	/*convert software calibration using standard calibration */
	obj->cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X] *
	    (cali[obj->cvt.map[BMG_AXIS_X]]) % (divisor);
	obj->cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y] *
	    (cali[obj->cvt.map[BMG_AXIS_Y]]) % (divisor);
	obj->cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z] *
	    (cali[obj->cvt.map[BMG_AXIS_Z]]) % (divisor);
	/* HW calibration is under construction */
	err = bmg_set_hw_offset(NULL, obj->offset);
	if (err) {
		GYRO_PR_ERR("read hw offset failed.\n");
		return err;
	}
#endif
	return err;
}

/* get chip type */
static int bmg_get_chip_type(struct bmg_gyro_data *obj)
{
	int err = 0;
	u8 chip_id = 0;

	obj = obj_data;
	/* twice */
	err = BMP160_spi_read_bytes(NULL, BMI160_USER_CHIP_ID__REG, &chip_id, 1);
	err = BMP160_spi_read_bytes(NULL, BMI160_USER_CHIP_ID__REG, &chip_id, 1);
	if (err != 0) {
		GYRO_PR_ERR("read chip id failed.\n");
		return err;
	}
	switch (chip_id) {
	case SENSOR_CHIP_ID_BMI:
	case SENSOR_CHIP_ID_BMI_C2:
	case SENSOR_CHIP_ID_BMI_C3:
		obj->sensor_type = BMI160_GYRO_TYPE;
		strlcpy(obj->sensor_name, BMG_DEV_NAME, sizeof(obj->sensor_name));
		break;
	default:
		obj->sensor_type = INVALID_TYPE;
		strlcpy(obj->sensor_name, UNKNOWN_DEV, sizeof(obj->sensor_name));
		break;
	}
	if (obj->sensor_type == INVALID_TYPE) {
		GYRO_PR_ERR("unknown gyroscope.\n");
		return -1;
	}
	return err;
}

static int bmg_set_powermode(struct bmg_gyro_data *obj, enum BMG_POWERMODE_ENUM power_mode)
{
	int err = 0;
	u8 data = 0;
	u8 actual_power_mode = 0;

	obj = obj_data;
	if (power_mode == obj->power_mode)
		return 0;

	mutex_lock(&obj->lock);
	if (obj->sensor_type == BMI160_GYRO_TYPE) {
		if (power_mode == BMG_SUSPEND_MODE) {
			actual_power_mode = CMD_PMU_GYRO_SUSPEND;
		} else if (power_mode == BMG_NORMAL_MODE) {
			actual_power_mode = CMD_PMU_GYRO_NORMAL;
		} else {
			err = -EINVAL;
			GYRO_PR_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->lock);
			return err;
		}
		data = actual_power_mode;
		err += BMP160_spi_write_bytes(NULL, BMI160_CMD_COMMANDS__REG, &data, 1);
		mdelay(55);
	}
	if (err < 0) {
		GYRO_PR_ERR("set power mode failed, err = %d, sensor name = %s\n",
			 err, obj->sensor_name);
	} else {
		obj->power_mode = power_mode;
	}
	mutex_unlock(&obj->lock);
	GYRO_LOG("set power mode = %d ok.\n", (int)data);
	return err;
}

static int bmg_set_range(struct bmg_gyro_data *obj, enum BMG_RANGE_ENUM range)
{
	u8 err = 0;
	u8 data = 0;
	u8 actual_range = 0;

	obj = obj_data;
	if (range == obj->range)
		return 0;

	mutex_lock(&obj->lock);
	if (obj->sensor_type == BMI160_GYRO_TYPE) {
		if (range == BMG_RANGE_2000)
			actual_range = BMI160_RANGE_2000;
		else if (range == BMG_RANGE_1000)
			actual_range = BMI160_RANGE_1000;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_500;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_250;
		else if (range == BMG_RANGE_500)
			actual_range = BMI160_RANGE_125;
		else {
			err = -EINVAL;
			GYRO_PR_ERR("invalid range = %d\n", range);
			mutex_unlock(&obj->lock);
			return err;
		}
		err = BMP160_spi_read_bytes(NULL, BMI160_USER_GYR_RANGE__REG, &data, 1);
		data = BMG_SET_BITSLICE(data, BMI160_USER_GYR_RANGE, actual_range);
		err += BMP160_spi_write_bytes(NULL, BMI160_USER_GYR_RANGE__REG, &data, 1);
		mdelay(1);
		if (err < 0) {
			GYRO_PR_ERR("set range failed.\n");
		} else {
			obj->range = range;
			/* bitnum: 16bit */
			switch (range) {
			case BMG_RANGE_2000:
				obj->sensitivity = BMI160_FS_2000_LSB;	/* 16; */
				break;
			case BMG_RANGE_1000:
				obj->sensitivity = BMI160_FS_1000_LSB;	/* 33; */
				break;
			case BMG_RANGE_500:
				obj->sensitivity = BMI160_FS_500_LSB;	/* 66; */
				break;
			case BMG_RANGE_250:
				obj->sensitivity = BMI160_FS_250_LSB;	/* 131; */
				break;
			case BMG_RANGE_125:
				obj->sensitivity = BMI160_FS_125_LSB;	/* 262; */
				break;
			default:
				obj->sensitivity = BMI160_FS_2000_LSB;	/* 16; */
				break;
			}
		}
	}
	mutex_unlock(&obj->lock);
	GYRO_LOG("set range ok.\n");
	return err;
}

static int bmg_set_datarate(struct bmg_gyro_data *obj, int datarate)
{
	int err = 0;
	u8 data = 0;

	obj = obj_data;
	if (datarate == obj->datarate) {
		GYRO_LOG("set new data rate = %d, old data rate = %d\n", datarate, obj->datarate);
		return 0;
	}
	mutex_lock(&obj->lock);
	if (obj->sensor_type == BMI160_GYRO_TYPE) {
		err = BMP160_spi_read_bytes(NULL, BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
		data = BMG_SET_BITSLICE(data, BMI160_USER_GYR_CONF_ODR, datarate);
		err += BMP160_spi_write_bytes(NULL, BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
	}
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
	else
		obj->datarate = datarate;

	mutex_unlock(&obj->lock);
	GYRO_LOG("set data rate = %d ok.\n", datarate);
	return err;
}

static int bmg_init_client(struct bmg_gyro_data *obj, int reset_cali)
{
#ifdef CONFIG_BMG_LOWPASS
	struct bmg_gyro_data *obj = (struct bmg_gyro_data *)obj_data;
#endif
	int err = 0;

	err = bmg_get_chip_type(NULL);
	if (err < 0)
		return err;

	err = bmg_set_datarate(NULL, BMI160_GYRO_ODR_100HZ);
	if (err < 0)
		return err;

	err = bmg_set_range(NULL, (enum BMG_RANGE_ENUM)BMG_RANGE_2000);
	if (err < 0)
		return err;

	err = bmg_set_powermode(NULL, (enum BMG_POWERMODE_ENUM)BMG_SUSPEND_MODE);
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
	memset(&obj->fir, 0x00, sizeof(obj->fir));
#endif
	return 0;
}

/*
*Returns compensated and mapped value. unit is :degree/second
*/
static int bmg_read_sensor_data(struct bmg_gyro_data *obj, char *buf, int bufsize)
{
	s16 databuf[BMG_AXES_NUM] = { 0 };
	int gyro[BMG_AXES_NUM] = { 0 };
	int err = 0;

	obj = obj_data;
	err = bmg_read_raw_data(NULL, databuf);
	if (err) {
		GYRO_PR_ERR("bmg read raw data failed.\n");
		return err;
	}
	/* compensate data */
	databuf[BMG_AXIS_X] += obj->cali_sw[BMG_AXIS_X];
	databuf[BMG_AXIS_Y] += obj->cali_sw[BMG_AXIS_Y];
	databuf[BMG_AXIS_Z] += obj->cali_sw[BMG_AXIS_Z];

	/* remap coordinate */
	gyro[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * databuf[BMG_AXIS_X];
	gyro[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * databuf[BMG_AXIS_Y];
	gyro[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * databuf[BMG_AXIS_Z];

	/* convert: LSB -> degree/second(o/s) */
	gyro[BMG_AXIS_X] = gyro[BMG_AXIS_X] * BMI160_FS_250_LSB / obj->sensitivity;
	gyro[BMG_AXIS_Y] = gyro[BMG_AXIS_Y] * BMI160_FS_250_LSB / obj->sensitivity;
	gyro[BMG_AXIS_Z] = gyro[BMG_AXIS_Z] * BMI160_FS_250_LSB / obj->sensitivity;

	sprintf(buf, "%04x %04x %04x",
		gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y], gyro[BMG_AXIS_Z]);
	if (atomic_read(&obj->trace) & GYRO_TRC_IOCTL)
		GYRO_LOG("gyroscope data: %s\n", buf);
	return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct bmg_gyro_data *obj = obj_data;

	return snprintf(buf, PAGE_SIZE, "%s\n", obj->sensor_name);
}

/*
* sensor data format is hex, unit:degree/second
*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[BMG_BUFSIZE] = { 0 };

	bmg_read_sensor_data(NULL, strbuf, BMG_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*
* raw data format is s16, unit:LSB, axis mapped
*/
static ssize_t show_rawdata_value(struct device_driver *ddri, char *buf)
{
	s16 databuf[BMG_AXES_NUM] = { 0 };
	s16 dataraw[BMG_AXES_NUM] = { 0 };
	struct bmg_gyro_data *obj = obj_data;

	bmg_read_raw_data(NULL, dataraw);
	/*remap coordinate */
	databuf[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * dataraw[BMG_AXIS_X];
	databuf[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * dataraw[BMG_AXIS_Y];
	databuf[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * dataraw[BMG_AXIS_Z];
	return snprintf(buf, PAGE_SIZE, "%hd %hd %hd\n",
			databuf[BMG_AXIS_X], databuf[BMG_AXIS_Y], databuf[BMG_AXIS_Z]);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	int len = 0;
	int mul;
	int act[BMG_AXES_NUM] = { 0 };
	int raw[BMG_AXES_NUM] = { 0 };
	struct bmg_gyro_data *obj = obj_data;

	err = bmg_read_calibration(NULL, act, raw);
	if (err)
		return -EINVAL;

	mul = 1;	/* mul = sensor sensitivity / offset sensitivity */
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
			mul,
			obj->offset[BMG_AXIS_X],
			obj->offset[BMG_AXIS_Y],
			obj->offset[BMG_AXIS_Z],
			obj->offset[BMG_AXIS_X],
			obj->offset[BMG_AXIS_Y], obj->offset[BMG_AXIS_Z]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
			obj->cali_sw[BMG_AXIS_X],
			obj->cali_sw[BMG_AXIS_Y], obj->cali_sw[BMG_AXIS_Z]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"[ALL]unmapped(%+3d, %+3d, %+3d), mapped(%+3d, %+3d, %+3d)\n",
			raw[BMG_AXIS_X], raw[BMG_AXIS_Y], raw[BMG_AXIS_Z],
			act[BMG_AXIS_X], act[BMG_AXIS_Y], act[BMG_AXIS_Z]);
	return len;
}

static ssize_t store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err = 0;
	int dat[BMG_AXES_NUM] = { 0 };
	struct bmg_gyro_data *obj = obj_data;

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

static ssize_t show_firlen_value(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_BMG_LOWPASS
	struct bmg_gyro_data *obj = obj_data;

	if (atomic_read(&obj->firlen)) {
		int idx, len = atomic_read(&obj->firlen);

		GYRO_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);

		for (idx = 0; idx < len; idx++) {
			GYRO_LOG("[%5d %5d %5d]\n",
				 obj->fir.raw[idx][BMG_AXIS_X],
				 obj->fir.raw[idx][BMG_AXIS_Y], obj->fir.raw[idx][BMG_AXIS_Z]);
		}

		GYRO_LOG("sum = [%5d %5d %5d]\n",
			 obj->fir.sum[BMG_AXIS_X],
			 obj->fir.sum[BMG_AXIS_Y], obj->fir.sum[BMG_AXIS_Z]);
		GYRO_LOG("avg = [%5d %5d %5d]\n",
			 obj->fir.sum[BMG_AXIS_X] / len,
			 obj->fir.sum[BMG_AXIS_Y] / len, obj->fir.sum[BMG_AXIS_Z] / len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}

static ssize_t store_firlen_value(struct device_driver *ddri, const char *buf, size_t count)
{
#ifdef CONFIG_BMG_LOWPASS
	struct bmg_gyro_data *obj = obj_data;
	int firlen;

	if (kstrtos32(buf, 10, &firlen) != 0) {
		GYRO_INFO("invallid format\n");
	} else if (firlen > C_MAX_FIR_LENGTH) {
		GYRO_INFO("exceeds maximum filter length\n");
	} else {
		atomic_set(&obj->firlen, firlen);
		if (firlen == NULL) {
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
	struct bmg_gyro_data *obj = obj_data;

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct bmg_gyro_data *obj = obj_data;

	if (kstrtos32(buf, 10, &trace) == 0)
		atomic_set(&obj->trace, trace);
	else
		GYRO_INFO("invalid content: '%s'\n", buf);
	return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_gyro_data *obj = obj_data;

	if (obj->hw)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"CUST: %d (%d %d)\n",
				obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");

	len += snprintf(buf + len, PAGE_SIZE - len, "ver:%s\n", BMG_DRIVER_VERSION);
	return len;
}

static ssize_t show_power_mode_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_gyro_data *obj = obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%s mode\n",
			obj->power_mode == BMG_NORMAL_MODE ? "normal" : "suspend");
	return len;
}

static ssize_t store_power_mode_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long power_mode;

	err = kstrtoul(buf, 10, &power_mode);
	if (err < 0)
		return err;

	if (power_mode == BMI_GYRO_PM_NORMAL)
		err = bmg_set_powermode(NULL, BMG_NORMAL_MODE);
	else
		err = bmg_set_powermode(NULL, BMG_SUSPEND_MODE);

	if (err < 0)
		GYRO_PR_ERR("set power mode failed.\n");

	return err;
}

static ssize_t show_range_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_gyro_data *obj = obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n", obj->range);
	return len;
}

static ssize_t store_range_value(struct device_driver *ddri, const char *buf, size_t count)
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

static ssize_t show_datarate_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmg_gyro_data *obj = obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%d\n", obj->datarate);
	return len;
}

static ssize_t store_datarate_value(struct device_driver *ddri, const char *buf, size_t count)
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

static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(rawdata, S_IWUSR | S_IRUGO, show_rawdata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO, show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(gyro_op_mode, S_IWUSR | S_IRUGO, show_power_mode_value, store_power_mode_value);
static DRIVER_ATTR(gyro_range, S_IWUSR | S_IRUGO, show_range_value, store_range_value);
static DRIVER_ATTR(gyro_odr, S_IWUSR | S_IRUGO, show_datarate_value, store_datarate_value);

static struct driver_attribute *bmg_attr_list[] = {
	/* chip information */
	&driver_attr_chipinfo,
	/* dump sensor data */
	&driver_attr_sensordata,
	/* dump raw data */
	&driver_attr_rawdata,
	/* show calibration data */
	&driver_attr_cali,
	/* filter length: 0: disable, others: enable */
	&driver_attr_firlen,
	/* trace flag */
	&driver_attr_trace,
	/* get hw configuration */
	&driver_attr_status,
	/* get power mode */
	&driver_attr_gyro_op_mode,
	/* get range */
	&driver_attr_gyro_range,
	/* get data rate */
	&driver_attr_gyro_odr,
};

static int bmg_create_attr(struct device_driver *driver)
{
	int idx = 0;
	int err = 0;
	int num = ARRAY_SIZE(bmg_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bmg_attr_list[idx]);
		if (err) {
			GYRO_PR_ERR("driver_create_file (%s) = %d\n",
				 bmg_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int bmg_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = ARRAY_SIZE(bmg_attr_list);

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bmg_attr_list[idx]);

	return err;
}

int gyroscope_operate(void *self, uint32_t command, void *buff_in, int size_in,
		      void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	char buff[BMG_BUFSIZE] = { 0 };
	struct bmg_gyro_data *priv = (struct bmg_gyro_data *)self;
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
			sample_delay = BMI160_GYRO_ODR_100HZ;

			GYRO_LOG("sensor delay command: %d, sample_delay = %d\n",
				 value, sample_delay);

			err = bmg_set_datarate(NULL, sample_delay);
			if (err < 0)
				GYRO_INFO("set delay parameter error\n");

			if (value >= 40)
				atomic_set(&priv->filter, 0);
			else {
#if defined(CONFIG_BMG_LOWPASS)
				priv->fir.num = 0;
				priv->fir.idx = 0;
				priv->fir.sum[BMG_AXIS_X] = 0;
				priv->fir.sum[BMG_AXIS_Y] = 0;
				priv->fir.sum[BMG_AXIS_Z] = 0;
				atomic_set(&priv->filter, 1);
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

#if 0
static int bmg_open(struct inode *inode, struct file *file)
{
	file->private_data = obj_data;
	if (file->private_data == NULL) {
		GYRO_INFO("file->private_data is null pointer.\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int bmg_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static long bmg_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	char strbuf[BMG_BUFSIZE] = { 0 };
	int raw_offset[BMG_BUFSIZE] = { 0 };
	void __user *data;
	struct SENSOR_DATA sensor_data;
	int cali[BMG_AXES_NUM] = { 0 };
	struct bmg_gyro_data *obj = (struct bmg_gyro_data *)file->private_data;

	if (obj == NULL)
		return -EFAULT;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) {
		GYRO_PR_ERR("access error: %08x, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}
	switch (cmd) {
	case GYROSCOPE_IOCTL_INIT:
		bmg_init_client(obj, 0);
		err = bmg_set_powermode(NULL, BMG_NORMAL_MODE);
		if (err) {
			err = -EFAULT;
			break;
		}
		break;
	case GYROSCOPE_IOCTL_READ_SENSORDATA:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}

		bmg_read_sensor_data(NULL, strbuf, BMG_BUFSIZE);
		if (copy_to_user(data, strbuf, strlen(strbuf) + 1)) {
			err = -EFAULT;
			break;
		}
		break;
	case GYROSCOPE_IOCTL_SET_CALI:
		/* data unit is degree/second */
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
			GYRO_INFO("perform calibration in suspend mode\n");
			err = -EINVAL;
		} else {
			/* convert: degree/second -> LSB */
			cali[BMG_AXIS_X] = sensor_data.x * obj->sensitivity / BMI160_FS_250_LSB;
			cali[BMG_AXIS_Y] = sensor_data.y * obj->sensitivity / BMI160_FS_250_LSB;
			cali[BMG_AXIS_Z] = sensor_data.z * obj->sensitivity / BMI160_FS_250_LSB;
			err = bmg_write_calibration(NULL, cali);
		}
		break;
	case GYROSCOPE_IOCTL_CLR_CALI:
		err = bmg_reset_calibration(obj);
		break;
	case GYROSCOPE_IOCTL_GET_CALI:
		data = (void __user *)arg;
		if (data == NULL) {
			err = -EINVAL;
			break;
		}
		err = bmg_read_calibration(NULL, cali, raw_offset);
		if (err)
			break;

		sensor_data.x = cali[BMG_AXIS_X] * BMI160_FS_250_LSB / obj->sensitivity;
		sensor_data.y = cali[BMG_AXIS_Y] * BMI160_FS_250_LSB / obj->sensitivity;
		sensor_data.z = cali[BMG_AXIS_Z] * BMI160_FS_250_LSB / obj->sensitivity;
		if (copy_to_user(data, &sensor_data, sizeof(sensor_data))) {
			err = -EFAULT;
			break;
		}
		break;
	default:
		GYRO_PR_ERR("unknown IOCTL: 0x%08x\n", cmd);
		err = -ENOIOCTLCMD;
		break;
	}
	return err;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_bmg_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	GYRO_FUN();

	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		GYRO_PR_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}

	switch (cmd) {
	case COMPAT_GYROSCOPE_IOCTL_INIT:
	case COMPAT_GYROSCOPE_IOCTL_READ_SENSORDATA:
	case COMPAT_GYROSCOPE_IOCTL_SET_CALI:
	case COMPAT_GYROSCOPE_IOCTL_CLR_CALI:
	case COMPAT_GYROSCOPE_IOCTL_GET_CALI:{
			GYRO_LOG("compat_ion_ioctl : GYROSCOPE_IOCTL_XXX command is 0x%x\n", cmd);
			return filp->f_op->unlocked_ioctl(filp, cmd,
							  (unsigned long)compat_ptr(arg));
		}
	default:{
			GYRO_PR_ERR("compat_ion_ioctl : No such command!! 0x%x\n", cmd);
			return -ENOIOCTLCMD;
		}
	}
}
#endif

static const struct file_operations bmg_fops = {
	.owner = THIS_MODULE,
	.open = bmg_open,
	.release = bmg_release,
	.unlocked_ioctl = bmg_unlocked_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = compat_bmg_unlocked_ioctl,
#endif
};

static struct miscdevice bmg_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyroscope",
	.fops = &bmg_fops,
};
#endif

#ifdef CONFIG_PM
static int bmg_suspend(void)
{
	struct bmg_gyro_data *obj = obj_data;
	int err = 0;

	if (obj == NULL) {
		GYRO_INFO("null pointer\n");
		return -EINVAL;
	}
	atomic_set(&obj->suspend, 1);
	err = bmg_set_powermode(NULL, BMG_SUSPEND_MODE);
	if (err)
		GYRO_PR_ERR("bmg set suspend mode failed.\n");

	return err;
}

static int bmg_resume(void)
{
	int err;
	struct bmg_gyro_data *obj = obj_data;

	err = bmg_init_client(obj, 0);
	if (err) {
		GYRO_PR_ERR("initialize client failed, err = %d\n", err);
		return err;
	}
	atomic_set(&obj->suspend, 0);
	return 0;
}

static int pm_event_handler(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		bmg_suspend();
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:
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

static int bmi160_gyro_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

static int bmi160_gyro_enable_nodata(int en)
{
	int err = 0;
	int retry = 0;
	bool power = false;

	if (en == 1)
		power = true;
	else
		power = false;

	for (retry = 0; retry < 3; retry++) {
		err = bmg_set_powermode(NULL, power);
		if (err == 0) {
			GYRO_LOG("bmi160_gyro_SetPowerMode ok.\n");
			break;
		}
	}
	if (err < 0)
		GYRO_LOG("bmi160_gyro_SetPowerMode fail!\n");

	return err;
}

static int bmi160_gyro_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int err;
	int value = (int)samplingPeriodNs/1000/1000;
	/* Currently, fix data rate to 100Hz. */
	int sample_delay = BMI160_GYRO_ODR_100HZ;
	struct bmg_gyro_data *priv = obj_data;

	err = bmg_set_datarate(NULL, sample_delay);
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
	if (value >= 40)
		atomic_set(&priv->filter, 0);
	else {
#if defined(CONFIG_BMG_LOWPASS)
		priv->fir.num = 0;
		priv->fir.idx = 0;
		priv->fir.sum[BMG_AXIS_X] = 0;
		priv->fir.sum[BMG_AXIS_Y] = 0;
		priv->fir.sum[BMG_AXIS_Z] = 0;
		atomic_set(&priv->filter, 1);
#endif
	}
	GYRO_LOG("set gyro delay = %d\n", sample_delay);
	return 0;
}

static int bmi160_gyro_flush(void)
{
	return gyro_flush_report();
}
#if 0
static int bmi160_gyro_enable_nodata(int en)
{
	int err = 0;
	int retry = 0;
	bool power = false;

	if (en == 1)
		power = true;
	else
		power = false;
	for (retry = 0; retry < 3; retry++) {
		err = bmg_set_powermode(NULL, power);
		if (err == 0) {
			GYRO_LOG("bmi160_gyro_SetPowerMode ok.\n");
			break;
		}
	}
	if (err < 0)
		GYRO_LOG("bmi160_gyro_SetPowerMode fail!\n");
	return err;
}

static int bmi160_gyro_set_delay(u64 ns)
{
	int err;
	int value = (int)ns / 1000 / 1000;
	/* Currently, fix data rate to 100Hz. */
	int sample_delay = BMI160_GYRO_ODR_100HZ;
	struct bmg_gyro_data *priv = obj_data;

	err = bmg_set_datarate(NULL, sample_delay);
	if (err < 0)
		GYRO_INFO("set data rate failed.\n");
	if (value >= 40)
		atomic_set(&priv->filter, 0);
	else {
#if defined(CONFIG_BMG_LOWPASS)
		priv->fir.num = 0;
		priv->fir.idx = 0;
		priv->fir.sum[BMG_AXIS_X] = 0;
		priv->fir.sum[BMG_AXIS_Y] = 0;
		priv->fir.sum[BMG_AXIS_Z] = 0;
		atomic_set(&priv->filter, 1);
#endif
	}
	GYRO_LOG("set gyro delay = %d\n", sample_delay);
	return 0;
}
#endif
static int bmi160_gyro_get_data(int *x, int *y, int *z, int *status)
{
	char buff[BMG_BUFSIZE] = { 0 };

	bmg_read_sensor_data(NULL, buff, BMG_BUFSIZE);
	if (sscanf(buff, "%x %x %x", x, y, z) != 3)
		GYRO_PR_ERR("bmi160_gyro_get_data failed");
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	return 0;
}


static int bmg_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err = 0;

	err = bmi160_gyro_enable_nodata(enabledisable == true ? 1 : 0);
	if (err) {
		GYRO_PR_ERR("%s enable failed!\n", __func__);
		return -1;
	}
	err = bmi160_gyro_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		GYRO_PR_ERR("%s set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}
static int bmg_factory_get_data(int32_t data[3], int *status)
{
	return bmi160_gyro_get_data(&data[0], &data[1], &data[2], status);
}
static int bmg_factory_get_raw_data(int32_t data[3])
{
	GYRO_INFO("don't support bmg_factory_get_raw_data!\n");
	return 0;
}
static int bmg_factory_enable_calibration(void)
{
	return 0;
}
static int bmg_factory_clear_cali(void)
{
	int err = 0;

	err = bmg_reset_calibration(obj_data);
	if (err) {
		GYRO_INFO("bmg_ResetCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int bmg_factory_set_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	cali[BMG_AXIS_X] = data[0] * obj_data->sensitivity / BMI160_FS_250_LSB;
	cali[BMG_AXIS_Y] = data[1] * obj_data->sensitivity / BMI160_FS_250_LSB;
	cali[BMG_AXIS_Z] = data[2] * obj_data->sensitivity / BMI160_FS_250_LSB;
	err = bmg_write_calibration(obj_data, cali);
	if (err) {
		GYRO_INFO("bmg_WriteCalibration failed!\n");
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
		GYRO_INFO("bmg_ReadCalibration failed!\n");
		return -1;
	}
	data[0] = cali[BMG_AXIS_X] * BMI160_FS_250_LSB / obj_data->sensitivity;
	data[1] = cali[BMG_AXIS_Y] * BMI160_FS_250_LSB / obj_data->sensitivity;
	data[2] = cali[BMG_AXIS_Z] * BMI160_FS_250_LSB / obj_data->sensitivity;
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

static int bmg_spi_probe(struct spi_device *spi)
{
	int err = 0;
	struct bmg_gyro_data *obj;
	struct gyro_control_path ctl = { 0 };
	struct gyro_data_path data = { 0 };

	pr_debug("%s\n", __func__);

	hw = get_gyro_dts_func(spi->dev.of_node, hw);
	if (!hw)
		GYRO_PR_ERR("Gyro device tree configuration error.\n");

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	obj->hw = hw;
	err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
	if (err) {
		GYRO_PR_ERR("invalid direction: %d\n", obj->hw->direction);
		goto exit_hwmsen_get_convert_failed;
	}
	obj_data = obj;
	atomic_set(&obj->trace, 0);
	atomic_set(&obj->suspend, 0);
	obj->power_mode = BMG_UNDEFINED_POWERMODE;
	obj->range = BMG_UNDEFINED_RANGE;
	obj->datarate = BMI160_GYRO_ODR_RESERVED;
	mutex_init(&obj->lock);
#ifdef CONFIG_BMG_LOWPASS
	if (obj->hw->firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->firlen, obj->hw->firlen);

	if (atomic_read(&obj->firlen) > 0)
		atomic_set(&obj->fir_en, 1);
#endif
	err = bmg_init_client(obj, 1);
	if (err)
		goto exit_init_client_failed;

	/* err = misc_register(&bmg_device); */
	err = gyro_factory_device_register(&bmg_factory_device);
	if (err) {
		GYRO_PR_ERR("misc device register failed, err = %d\n", err);
		goto exit_misc_device_register_failed;
	}
	err = bmg_create_attr(&bmi160_gyro_init_info.platform_diver_addr->driver);
	if (err) {
		GYRO_PR_ERR("create attribute failed, err = %d\n", err);
		goto exit_create_attr_failed;
	}
	ctl.open_report_data = bmi160_gyro_open_report_data;
	ctl.enable_nodata = bmi160_gyro_enable_nodata;
	ctl.batch = bmi160_gyro_batch;
	ctl.flush = bmi160_gyro_flush;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = obj->hw->is_batch_supported;
	err = gyro_register_control_path(&ctl);
	if (err) {
		GYRO_PR_ERR("register gyro control path err\n");
		goto exit_create_attr_failed;
	}

	data.get_data = bmi160_gyro_get_data;
	data.vender_div = DEGREE_TO_RAD;
	err = gyro_register_data_path(&data);
	if (err) {
		GYRO_PR_ERR("gyro_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}
#ifdef CONFIG_PM
	err = register_pm_notifier(&pm_notifier_func);
	if (err) {
		GYRO_PR_ERR("Failed to register PM notifier.\n");
		goto exit_create_attr_failed;
	}
#endif				/* CONFIG_PM */

	bmi160_gyro_init_flag = 0;
	GYRO_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	/* misc_deregister(&bmg_device); */
exit_misc_device_register_failed:
exit_init_client_failed:
exit_hwmsen_get_convert_failed:
	kfree(obj);
exit:
	bmi160_gyro_init_flag = -1;
	GYRO_PR_ERR("err = %d\n", err);
	return err;
}

static int bmg_spi_remove(struct spi_device *spi)
{
	int err = 0;

	err = bmg_delete_attr(&bmi160_gyro_init_info.platform_diver_addr->driver);
	if (err)
		GYRO_PR_ERR("bmg_delete_attr failed, err = %d\n", err);

	/* misc_deregister(&bmg_device); */
	gyro_factory_device_deregister(&bmg_factory_device);

	obj_data = NULL;
	kfree(obj_data);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id gyro_of_match[] = {
	{.compatible = "mediatek,gyro"},
	{},
};
#endif

static struct spi_driver bmg_spi_driver = {
	.driver = {
		   .name = BMG_DEV_NAME,
#ifdef CONFIG_OF
		   .of_match_table = gyro_of_match,
#endif
		   },
	.probe = bmg_spi_probe,
	.remove = bmg_spi_remove,
};

static int bmi160_gyro_remove(void)
{
	spi_unregister_driver(&bmg_spi_driver);
	return 0;
}

static int bmi160_gyro_local_init(struct platform_device *pdev)
{
	if (spi_register_driver(&bmg_spi_driver)) {
		GYRO_PR_ERR("add gyro driver error.\n");
		return -1;
	}
	if (-1 == bmi160_gyro_init_flag)
		return -1;

	GYRO_LOG("bmi160 gyro init ok.\n");
	return 0;
}

static struct gyro_init_info bmi160_gyro_init_info = {
	.name = BMG_DEV_NAME,
	.init = bmi160_gyro_local_init,
	.uninit = bmi160_gyro_remove,
};

static int __init bmg_init(void)
{
	GYRO_LOG("%s: bosch gyroscope driver version: %s\n", __func__, BMG_DRIVER_VERSION);
	gyro_driver_add(&bmi160_gyro_init_info);
	return 0;
}

static void __exit bmg_exit(void)
{
	GYRO_FUN();
}
module_init(bmg_init);
module_exit(bmg_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMG I2C Driver");
MODULE_AUTHOR("xiaogang.fan@cn.bosch.com");
MODULE_VERSION(BMG_DRIVER_VERSION);

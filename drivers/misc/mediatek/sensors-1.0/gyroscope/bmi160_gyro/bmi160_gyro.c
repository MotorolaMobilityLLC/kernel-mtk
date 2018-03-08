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

#include <gyroscope.h>
#include <cust_gyro.h>
#include <hwmsensor.h>
//#include <hwmsen_dev.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include "bmi160_gyro.h"
//#include <mt_boot_common.h>

/* sensor type */
enum SENSOR_TYPE_ENUM {
    BMI160_GYRO_TYPE = 0x0,
    INVALID_TYPE = 0xff
};

/* range */
enum BMG_RANGE_ENUM {
    BMG_RANGE_2000 = 0x0,   /* +/- 2000 degree/s */
    BMG_RANGE_1000,         /* +/- 1000 degree/s */
    BMG_RANGE_500,          /* +/- 500 degree/s */
    BMG_RANGE_250,          /* +/- 250 degree/s */
    BMG_RANGE_125,          /* +/- 125 degree/s */
    BMG_UNDEFINED_RANGE = 0xff
};

/* power mode */
enum BMG_POWERMODE_ENUM {
    BMG_SUSPEND_MODE = 0x0,
    BMG_NORMAL_MODE,
    BMG_UNDEFINED_POWERMODE = 0xff
};

/* debug infomation flags */
enum GYRO_TRC {
    GYRO_TRC_FILTER  = 0x01,
    GYRO_TRC_RAWDATA = 0x02,
    GYRO_TRC_IOCTL   = 0x04,
    GYRO_TRC_CALI   = 0x08,
    GYRO_TRC_INFO   = 0x10,
};

/* s/w data filter */
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
    int sum[BMG_AXES_NUM];
    int num;
    int idx;
};

/* bmg i2c client data */
struct bmg_i2c_data {
    struct i2c_client *client;
    struct gyro_hw *hw;
    struct hwmsen_convert   cvt;
    /* sensor info */
    u8 sensor_name[MAX_SENSOR_NAME];
    enum SENSOR_TYPE_ENUM sensor_type;
    enum BMG_POWERMODE_ENUM power_mode;
    enum BMG_RANGE_ENUM range;
    int datarate;
    /* sensitivity = 2^bitnum/range
    [+/-2000 = 4000; +/-1000 = 2000;
     +/-500 = 1000; +/-250 = 500;
     +/-125 = 250 ] */
    u16 sensitivity;
    /*misc*/
    struct mutex lock;
    atomic_t    trace;
    atomic_t    suspend;
    atomic_t    filter;
    /* unmapped axis value */
    s16 cali_sw[BMG_AXES_NUM+1];
    /* hw offset */
    s8  offset[BMG_AXES_NUM+1];/* +1:for 4-byte alignment */

#if defined(CONFIG_BMG_LOWPASS)
    atomic_t    firlen;
    atomic_t    fir_en;
    struct data_filter  fir;
#endif
};
static bool isOpenLog = false;
struct gyro_hw gyro_cust;
static int bmi160_gyro_local_init(struct platform_device *pdev);
static struct gyro_hw *hw = &gyro_cust;
static struct gyro_init_info bmi160_gyro_init_info;
/* 0=OK, -1=fail */
static int bmi160_gyro_init_flag = -1;
static struct i2c_driver bmg_i2c_driver;
static struct bmg_i2c_data *obj_i2c_data;
static int bmg_set_powermode(struct i2c_client *client,
        enum BMG_POWERMODE_ENUM power_mode);
static const struct i2c_device_id bmi160_gyro_i2c_id[] = {
    {BMG_DEV_NAME, 0},
    {}
};

static void bmi160_gyro_delay(u32 msec)
{
    if (msec <= 20)
        usleep_range(msec * 1000, msec * 1000);
    else
        msleep(msec);
}

/* I2C operation functions */
static int bmg_i2c_read_block(struct i2c_client *client, u8 addr,
                u8 *data, u8 len)
{
    int err;
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
    if (!client)
        return -EINVAL;
    else if (len > C_I2C_FIFO_SIZE) {
        GYRO_PR_ERR("length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
        return -EINVAL;
    }
    err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
    if (err != 2) {
        GYRO_PR_ERR("i2c_transfer error: (%d %p %d) %d\n",
            addr, data, len, err);
        err = -EIO;
    } else {
        err = 0;
    }
    return err;
}

static int bmg_i2c_write_block(struct i2c_client *client, u8 addr,
                u8 *data, u8 len)
{
    /*
    *because address also occupies one byte,
    *the maximum length for write is 7 bytes
    */
    int err = 0;
    int idx = 0;
    int num = 0;
    char buf[C_I2C_FIFO_SIZE];
    if (!client)
        return -EINVAL;
    else if (len >= C_I2C_FIFO_SIZE) {
        GYRO_PR_ERR("length %d exceeds %d\n", len, C_I2C_FIFO_SIZE);
        return -EINVAL;
    }
    buf[num++] = addr;
    for (idx = 0; idx < len; idx++)
        buf[num++] = data[idx];
    err = i2c_master_send(client, buf, num);
    if (err < 0) {
        GYRO_PR_ERR("send command error.\n");
        return -EFAULT;
    } else {
        err = 0;
    }
    msleep(1);
    return err;
}

static int bmg_read_raw_data(struct i2c_client *client, s16 data[BMG_AXES_NUM])
{
    int err = 0;
    struct bmg_i2c_data *priv = obj_i2c_data;
    if(isOpenLog){
		printk("check in [%s] .\n",__func__);
	}
    if (priv->power_mode == BMG_SUSPEND_MODE) {
        err = bmg_set_powermode(client,
            (enum BMG_POWERMODE_ENUM)BMG_NORMAL_MODE);
        if (err < 0) {
            GYRO_PR_ERR("set power mode failed, err = %d\n", err);
            return err;
        }
    }
    if (priv->sensor_type == BMI160_GYRO_TYPE) {
        u8 buf_tmp[BMG_DATA_LEN] = {0};
        err = bmg_i2c_read_block(client,
            BMI160_USER_DATA_8_GYR_X_LSB__REG,
                buf_tmp, 6);
        if (err) {
            GYRO_PR_ERR("read gyro raw data failed.\n");
            return err;
        }
        /* Data X */
        data[BMG_AXIS_X] = (s16)
            ((((s32)((s8)buf_tmp[1]))
              << BMI160_SHIFT_8_POSITION) | (buf_tmp[0]));
        /* Data Y */
        data[BMG_AXIS_Y] = (s16)
            ((((s32)((s8)buf_tmp[3]))
              << BMI160_SHIFT_8_POSITION) | (buf_tmp[2]));
        /* Data Z */
        data[BMG_AXIS_Z] = (s16)
            ((((s32)((s8)buf_tmp[5]))
              << BMI160_SHIFT_8_POSITION) | (buf_tmp[4]));
		if(isOpenLog){
			printk("bmi160_gyro 0 [%s][16bit raw]\n\n"
            "[%08X %08X %08X] => [%5d %5d %5d]\n",
            priv->sensor_name,
            data[BMG_AXIS_X],
            data[BMG_AXIS_Y],
            data[BMG_AXIS_Z],
            data[BMG_AXIS_X],
            data[BMG_AXIS_Y],
            data[BMG_AXIS_Z]);
		}
        if (atomic_read(&priv->trace) & GYRO_TRC_RAWDATA) {
            GYRO_LOG("[%s][16bit raw]\n\n"
            "[%08X %08X %08X] => [%5d %5d %5d]\n",
            priv->sensor_name,
            data[BMG_AXIS_X],
            data[BMG_AXIS_Y],
            data[BMG_AXIS_Z],
            data[BMG_AXIS_X],
            data[BMG_AXIS_Y],
            data[BMG_AXIS_Z]);
        }
    }

#ifdef CONFIG_BMG_LOWPASS
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
                priv->fir.raw[priv->fir.num][BMG_AXIS_X] =
                        data[BMG_AXIS_X];
                priv->fir.raw[priv->fir.num][BMG_AXIS_Y] =
                        data[BMG_AXIS_Y];
                priv->fir.raw[priv->fir.num][BMG_AXIS_Z] =
                        data[BMG_AXIS_Z];
                priv->fir.sum[BMG_AXIS_X] += data[BMG_AXIS_X];
                priv->fir.sum[BMG_AXIS_Y] += data[BMG_AXIS_Y];
                priv->fir.sum[BMG_AXIS_Z] += data[BMG_AXIS_Z];
                if(isOpenLog){
			        printk("bmi160-gyro 1 [%s] add [%2d]\n\n"
                    "[%5d %5d %5d] => [%5d %5d %5d]\n",__func__,
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
                if (atomic_read(&priv->trace)&GYRO_TRC_FILTER) {
                    GYRO_LOG("add [%2d]\n\n"
                    "[%5d %5d %5d] => [%5d %5d %5d]\n",
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
                priv->fir.sum[BMG_AXIS_X] -=
                    priv->fir.raw[idx][BMG_AXIS_X];
                priv->fir.sum[BMG_AXIS_Y] -=
                    priv->fir.raw[idx][BMG_AXIS_Y];
                priv->fir.sum[BMG_AXIS_Z] -=
                    priv->fir.raw[idx][BMG_AXIS_Z];
                priv->fir.raw[idx][BMG_AXIS_X] =
                    data[BMG_AXIS_X];
                priv->fir.raw[idx][BMG_AXIS_Y] =
                    data[BMG_AXIS_Y];
                priv->fir.raw[idx][BMG_AXIS_Z] =
                    data[BMG_AXIS_Z];
                priv->fir.sum[BMG_AXIS_X] +=
                    data[BMG_AXIS_X];
                priv->fir.sum[BMG_AXIS_Y] +=
                    data[BMG_AXIS_Y];
                priv->fir.sum[BMG_AXIS_Z] +=
                    data[BMG_AXIS_Z];
                priv->fir.idx++;
                data[BMG_AXIS_X] =
                    priv->fir.sum[BMG_AXIS_X]/firlen;
                data[BMG_AXIS_Y] =
                    priv->fir.sum[BMG_AXIS_Y]/firlen;
                data[BMG_AXIS_Z] =
                    priv->fir.sum[BMG_AXIS_Z]/firlen;
                if(isOpenLog){
				    printk("bmi160-gyro 2 [%s]add [%2d]\n\n"
                    "[%5d %5d %5d] =>\n\n"
                    "[%5d %5d %5d] : [%5d %5d %5d]\n",__func__, idx,
                    priv->fir.raw[idx][BMG_AXIS_X],
                    priv->fir.raw[idx][BMG_AXIS_Y],
                    priv->fir.raw[idx][BMG_AXIS_Z],
                    priv->fir.sum[BMG_AXIS_X],
                    priv->fir.sum[BMG_AXIS_Y],
                    priv->fir.sum[BMG_AXIS_Z],
                    data[BMG_AXIS_X],
                    data[BMG_AXIS_Y],
                    data[BMG_AXIS_Z]);
				}
                if (atomic_read(&priv->trace)&GYRO_TRC_FILTER) {
                    GYRO_LOG("add [%2d]\n\n"
                    "[%5d %5d %5d] =>\n\n"
                    "[%5d %5d %5d] : [%5d %5d %5d]\n", idx,
                    priv->fir.raw[idx][BMG_AXIS_X],
                    priv->fir.raw[idx][BMG_AXIS_Y],
                    priv->fir.raw[idx][BMG_AXIS_Z],
                    priv->fir.sum[BMG_AXIS_X],
                    priv->fir.sum[BMG_AXIS_Y],
                    priv->fir.sum[BMG_AXIS_Z],
                    data[BMG_AXIS_X],
                    data[BMG_AXIS_Y],
                    data[BMG_AXIS_Z]);
                }
            }
        }
    }
#endif
    return err;
}

#ifndef SW_CALIBRATION
/* get hardware offset value from chip register */
static int bmg_get_hw_offset(struct i2c_client *client,
        s8 offset[BMG_AXES_NUM + 1])
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
static int bmg_set_hw_offset(struct i2c_client *client,
        s8 offset[BMG_AXES_NUM + 1])
{
    int err = 0;
    /* HW calibration is under construction */
    GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
    offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);
    return err;
}
#endif

static int bmg_reset_calibration(struct i2c_client *client)
{
    int err = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
#ifdef SW_CALIBRATION

#else
    err = bmg_set_hw_offset(client, obj->offset);
    if (err) {
        GYRO_PR_ERR("read hw offset failed, %d\n", err);
        return err;
    }
#endif
    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    memset(obj->offset, 0x00, sizeof(obj->offset));
    return err;
}

static int bmg_read_calibration(struct i2c_client *client,
    int act[BMG_AXES_NUM], int raw[BMG_AXES_NUM])
{
    /*
    *raw: the raw calibration data, unmapped;
    *act: the actual calibration data, mapped
    */
    int err = 0;
    int mul;
    struct bmg_i2c_data *obj = obj_i2c_data;

    #ifdef SW_CALIBRATION
    /* only sw calibration, disable hw calibration */
    mul = 0;
    #else
    err = bmg_get_hw_offset(client, obj->offset);
    if (err) {
        GYRO_PR_ERR("read hw offset failed, %d\n", err);
        return err;
    }
    mul = 1; /* mul = sensor sensitivity / offset sensitivity */
    #endif

    raw[BMG_AXIS_X] =
    obj->offset[BMG_AXIS_X]*mul + obj->cali_sw[BMG_AXIS_X];
    raw[BMG_AXIS_Y] =
    obj->offset[BMG_AXIS_Y]*mul + obj->cali_sw[BMG_AXIS_Y];
    raw[BMG_AXIS_Z] =
    obj->offset[BMG_AXIS_Z]*mul + obj->cali_sw[BMG_AXIS_Z];

    act[obj->cvt.map[BMG_AXIS_X]] =
    obj->cvt.sign[BMG_AXIS_X]*raw[BMG_AXIS_X];
    act[obj->cvt.map[BMG_AXIS_Y]] =
    obj->cvt.sign[BMG_AXIS_Y]*raw[BMG_AXIS_Y];
    act[obj->cvt.map[BMG_AXIS_Z]] =
    obj->cvt.sign[BMG_AXIS_Z]*raw[BMG_AXIS_Z];
    return err;
}

static int bmg_write_calibration(struct i2c_client *client,
    int dat[BMG_AXES_NUM])
{
    /* dat array : Android coordinate system, mapped, unit:LSB */
    int err = 0;
    int cali[BMG_AXES_NUM] = {0};
    int raw[BMG_AXES_NUM] = {0};
    struct bmg_i2c_data *obj = obj_i2c_data;
    /*offset will be updated in obj->offset */
    err = bmg_read_calibration(client, cali, raw);
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
    obj->cali_sw[BMG_AXIS_X] =
        obj->cvt.sign[BMG_AXIS_X]*(cali[obj->cvt.map[BMG_AXIS_X]]);
    obj->cali_sw[BMG_AXIS_Y] =
        obj->cvt.sign[BMG_AXIS_Y]*(cali[obj->cvt.map[BMG_AXIS_Y]]);
    obj->cali_sw[BMG_AXIS_Z] =
        obj->cvt.sign[BMG_AXIS_Z]*(cali[obj->cvt.map[BMG_AXIS_Z]]);
#else
    /* divisor = sensor sensitivity / offset sensitivity */
    int divisor = 1;
    obj->offset[BMG_AXIS_X] = (s8)(obj->cvt.sign[BMG_AXIS_X]*
        (cali[obj->cvt.map[BMG_AXIS_X]])/(divisor));
    obj->offset[BMG_AXIS_Y] = (s8)(obj->cvt.sign[BMG_AXIS_Y]*
        (cali[obj->cvt.map[BMG_AXIS_Y]])/(divisor));
    obj->offset[BMG_AXIS_Z] = (s8)(obj->cvt.sign[BMG_AXIS_Z]*
        (cali[obj->cvt.map[BMG_AXIS_Z]])/(divisor));

    /*convert software calibration using standard calibration*/
    obj->cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X]*
        (cali[obj->cvt.map[BMG_AXIS_X]])%(divisor);
    obj->cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y]*
        (cali[obj->cvt.map[BMG_AXIS_Y]])%(divisor);
    obj->cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z]*
        (cali[obj->cvt.map[BMG_AXIS_Z]])%(divisor);
    /* HW calibration is under construction */
    err = bmg_set_hw_offset(client, obj->offset);
    if (err) {
        GYRO_PR_ERR("read hw offset failed.\n");
        return err;
    }
#endif
    return err;
}

/* get chip type */
static int bmg_get_chip_type(struct i2c_client *client)
{
    int err = 0;
    u8 chip_id = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    /* twice */
    err = bmg_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, &chip_id, 1);
    err = bmg_i2c_read_block(client, BMI160_USER_CHIP_ID__REG, &chip_id, 1);
    if (err != 0) {
        GYRO_PR_ERR("read chip id failed.\n");
        return err;
    }
    switch (chip_id) {
    case SENSOR_CHIP_ID_BMI:
    case SENSOR_CHIP_ID_BMI_C2:
    case SENSOR_CHIP_ID_BMI_C3:
        obj->sensor_type = BMI160_GYRO_TYPE;
        strlcpy(obj->sensor_name, BMG_DEV_NAME, 48);
        break;
    default:
        obj->sensor_type = INVALID_TYPE;
        strlcpy(obj->sensor_name, UNKNOWN_DEV, 48);
        break;
    }
    if (obj->sensor_type == INVALID_TYPE) {
        GYRO_PR_ERR("unknown gyroscope.\n");
        err = -1;
        return err;
    }
    return err;
}

static int bmg_set_powermode(struct i2c_client *client,
        enum BMG_POWERMODE_ENUM power_mode)
{
    int err = 0;
    u8 data = 0;
    u8 actual_power_mode = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;

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
        /*data = actual_power_mode;*/
        err += bmg_i2c_write_block(client,
            BMI160_CMD_COMMANDS__REG, &actual_power_mode, 1);
        bmi160_gyro_delay(200);
        err += bmg_i2c_read_block(client,
            BMI160_USER_PMU_STATUS_ADDR, &data, 1);
            GYRO_PR_ERR("++++ [bmi160 gyro] get power mode = 0x%x\n", data);
    }
    if (err < 0)
        GYRO_PR_ERR("set power mode failed, err = %d, sensor name = %s\n",
        err, obj->sensor_name);
    else
        obj->power_mode = power_mode;
    mutex_unlock(&obj->lock);
    GYRO_LOG("set power mode = %d ok.\n", (int)data);
    return err;
}

static int bmg_set_range(struct i2c_client *client, enum BMG_RANGE_ENUM range)
{
    u8 err = 0;
    u8 data = 0;
    u8 actual_range = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;

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
        else if (range == BMG_RANGE_250)
            actual_range = BMI160_RANGE_250;
        else if (range == BMG_RANGE_125)
            actual_range = BMI160_RANGE_125;
        else {
            err = -EINVAL;
            GYRO_PR_ERR("invalid range = %d\n", range);
            mutex_unlock(&obj->lock);
            return err;
        }
        err = bmg_i2c_read_block(
        client, BMI160_USER_GYR_RANGE__REG, &data, 1);
        data = BMG_SET_BITSLICE(
        data, BMI160_USER_GYR_RANGE, actual_range);
        err += bmg_i2c_write_block(
        client, BMI160_USER_GYR_RANGE__REG, &data, 1);
        bmi160_gyro_delay(5);
        if (err < 0)
            GYRO_PR_ERR("set range failed.\n");
        else {
            obj->range = range;
            /* bitnum: 16bit */
            switch (range) {
            case BMG_RANGE_2000:
                obj->sensitivity = 165;
            break;
            case BMG_RANGE_1000:
                obj->sensitivity = 330;
            break;
            case BMG_RANGE_500:
                obj->sensitivity = 660;
            break;
            case BMG_RANGE_250:
                obj->sensitivity = 1310;
            break;
            case BMG_RANGE_125:
                obj->sensitivity = 2620;
            break;
            default:
                obj->sensitivity = 160;
            break;
            }
        }
    }
    mutex_unlock(&obj->lock);
    GYRO_LOG("set range ok.range=%d sens=%d\n",
        actual_range, obj->sensitivity);
    return err;
}

static int bmg_set_datarate(struct i2c_client *client,
        int datarate)
{
    int err = 0;
    u8 data = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    if (datarate == obj->datarate) {
        GYRO_LOG("set new data rate = %d, old data rate = %d\n",
        datarate, obj->datarate);
        return 0;
    }
    mutex_lock(&obj->lock);
    if (obj->sensor_type == BMI160_GYRO_TYPE) {
        err = bmg_i2c_read_block(client,
            BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
        data = BMG_SET_BITSLICE(data,
            BMI160_USER_GYR_CONF_ODR, datarate);
        err += bmg_i2c_write_block(client,
            BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
        bmi160_gyro_delay(5);
    }
    if (err < 0)
        GYRO_PR_ERR("set data rate failed.\n");
    else
        obj->datarate = datarate;
    mutex_unlock(&obj->lock);
    GYRO_LOG("set data rate = %d ok.\n", datarate);
    return err;
}

static int bmg_init_client(struct i2c_client *client, int reset_cali)
{
#ifdef CONFIG_BMG_LOWPASS
    struct bmg_i2c_data *obj =
        (struct bmg_i2c_data *)obj_i2c_data;
#endif
    int err = 0;
    err = bmg_get_chip_type(client);
    if (err < 0)
        return err;
    err = bmg_set_datarate(client, BMI160_GYRO_ODR_100HZ);
    if (err < 0)
        return err;
    err = bmg_set_range(client, (enum BMG_RANGE_ENUM)BMG_RANGE_2000);
    if (err < 0)
        return err;
    err = bmg_set_powermode(client,
        (enum BMG_POWERMODE_ENUM)BMG_SUSPEND_MODE);
    if (err < 0)
        return err;
    if (0 != reset_cali) {
        /*reset calibration only in power on*/
        err = bmg_reset_calibration(client);
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
static int bmg_read_sensor_data(struct i2c_client *client,
        char *buf, int bufsize)
{
    s16 databuf[BMG_AXES_NUM] = {0};
    int gyro[BMG_AXES_NUM] = {0};
    int err = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    err = bmg_read_raw_data(client, databuf);
    if (err) {
        GYRO_PR_ERR("bmg read raw data failed.\n");
        return err;
    } else {
        /* compensate data */
        databuf[BMG_AXIS_X] += obj->cali_sw[BMG_AXIS_X];
        databuf[BMG_AXIS_Y] += obj->cali_sw[BMG_AXIS_Y];
        databuf[BMG_AXIS_Z] += obj->cali_sw[BMG_AXIS_Z];

        /* remap coordinate */
        gyro[obj->cvt.map[BMG_AXIS_X]] =
            obj->cvt.sign[BMG_AXIS_X]*databuf[BMG_AXIS_X];
        gyro[obj->cvt.map[BMG_AXIS_Y]] =
            obj->cvt.sign[BMG_AXIS_Y]*databuf[BMG_AXIS_Y];
        gyro[obj->cvt.map[BMG_AXIS_Z]] =
            obj->cvt.sign[BMG_AXIS_Z]*databuf[BMG_AXIS_Z];

        /* convert: LSB -> degree/second(o/s) */
        gyro[BMG_AXIS_X] = (gyro[BMG_AXIS_X] * 1310) / obj->sensitivity;
        gyro[BMG_AXIS_Y] = (gyro[BMG_AXIS_Y] * 1310) / obj->sensitivity;
        gyro[BMG_AXIS_Z] = (gyro[BMG_AXIS_Z] * 1310) / obj->sensitivity;
		if(isOpenLog){
			printk("[%s] wcz %04x %04x %04x .\n",__func__,gyro[BMG_AXIS_X],gyro[BMG_AXIS_Y],gyro[BMG_AXIS_Z]);
			printk("[%s] gyro final xyz data: %d,%d,%d, sens:%d\n",__func__,
					gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y],
					gyro[BMG_AXIS_Z], obj->sensitivity);
		}
        snprintf(buf, 96, "%04x %04x %04x",
        gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y], gyro[BMG_AXIS_Z]);
        if (atomic_read(&obj->trace) & GYRO_TRC_IOCTL)
            GYRO_LOG("gyroscope data: %s\n", buf);
    }
    return 0;
}

static ssize_t show_chipinfo_value(struct device_driver *ddri, char *buf)
{
    struct bmg_i2c_data *obj = obj_i2c_data;
    return snprintf(buf, PAGE_SIZE, "%s\n", obj->sensor_name);
}

/*
* sensor data format is hex, unit:degree/second
*/
static ssize_t show_sensordata_value(struct device_driver *ddri, char *buf)
{
    struct bmg_i2c_data *obj = obj_i2c_data;
    char strbuf[BMG_BUFSIZE] = {0};
    bmg_read_sensor_data(obj->client, strbuf, BMG_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

/*
* raw data format is s16, unit:LSB, axis mapped
*/
static ssize_t show_rawdata_value(struct device_driver *ddri, char *buf)
{
    s16 databuf[BMG_AXES_NUM] = {0};
    s16 dataraw[BMG_AXES_NUM] = {0};
    struct bmg_i2c_data *obj = obj_i2c_data;
    bmg_read_raw_data(obj->client, dataraw);
    /*remap coordinate*/
    databuf[obj->cvt.map[BMG_AXIS_X]] =
            obj->cvt.sign[BMG_AXIS_X]*dataraw[BMG_AXIS_X];
    databuf[obj->cvt.map[BMG_AXIS_Y]] =
            obj->cvt.sign[BMG_AXIS_Y]*dataraw[BMG_AXIS_Y];
    databuf[obj->cvt.map[BMG_AXIS_Z]] =
            obj->cvt.sign[BMG_AXIS_Z]*dataraw[BMG_AXIS_Z];
    return snprintf(buf, PAGE_SIZE, "%hd %hd %hd\n",
            databuf[BMG_AXIS_X],
            databuf[BMG_AXIS_Y],
            databuf[BMG_AXIS_Z]);
}

static ssize_t show_cali_value(struct device_driver *ddri, char *buf)
{
    int err = 0;
    int len = 0;
    int mul = 0;
    int act[BMG_AXES_NUM] = {0};
    int raw[BMG_AXES_NUM] = {0};
    struct bmg_i2c_data *obj = obj_i2c_data;

    mul = len;
    err = bmg_read_calibration(obj->client, act, raw);
    if (err)
        return -EINVAL;
    else {
        mul = 1; /* mul = sensor sensitivity / offset sensitivity */
        len += snprintf(buf+len, PAGE_SIZE-len,
        "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n",
        mul,
        obj->offset[BMG_AXIS_X],
        obj->offset[BMG_AXIS_Y],
        obj->offset[BMG_AXIS_Z],
        obj->offset[BMG_AXIS_X],
        obj->offset[BMG_AXIS_Y],
        obj->offset[BMG_AXIS_Z]);
        len += snprintf(buf+len, PAGE_SIZE-len,
        "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
        obj->cali_sw[BMG_AXIS_X],
        obj->cali_sw[BMG_AXIS_Y],
        obj->cali_sw[BMG_AXIS_Z]);
        len += snprintf(buf+len, PAGE_SIZE-len,
        "[ALL]unmapped(%+3d, %+3d, %+3d), mapped(%+3d, %+3d, %+3d)\n",
        raw[BMG_AXIS_X], raw[BMG_AXIS_Y], raw[BMG_AXIS_Z],
        act[BMG_AXIS_X], act[BMG_AXIS_Y], act[BMG_AXIS_Z]);
        return len;
    }
}

static ssize_t store_cali_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    int err = 0;
    int dat[BMG_AXES_NUM] = {0};
    struct bmg_i2c_data *obj = obj_i2c_data;
    if (!strncmp(buf, "rst", 3)) {
        err = bmg_reset_calibration(obj->client);
        if (err)
            GYRO_PR_ERR("reset offset err = %d\n", err);
    } else if (BMG_AXES_NUM == sscanf(buf, "0x%02X 0x%02X 0x%02X",
        &dat[BMG_AXIS_X], &dat[BMG_AXIS_Y], &dat[BMG_AXIS_Z])) {
        err = bmg_write_calibration(obj->client, dat);
        if (err) {
            GYRO_PR_ERR("bmg write calibration failed, err = %d\n",
                err);
        }
    } else {
        GYRO_PR_ERR("invalid format\n");
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
    struct bmg_i2c_data *obj = obj_i2c_data;
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
    return res;
}

static ssize_t store_trace_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    int trace;
    struct bmg_i2c_data *obj = obj_i2c_data;
    if (1 == sscanf(buf, "0x%2x", &trace))
        atomic_set(&obj->trace, trace);
    else
        GYRO_PR_ERR("invalid content: '%s'\n", buf);
    return count;
}

static ssize_t show_status_value(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    if (obj->hw)
        len += snprintf(buf+len, PAGE_SIZE-len,
        "CUST: %d %d (%d %d)\n",
        obj->hw->i2c_num, obj->hw->direction,
        obj->hw->power_id, obj->hw->power_vol);
    else
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");

    len += snprintf(buf+len, PAGE_SIZE-len, "i2c addr:%#x,ver:%s\n",
        obj->client->addr, BMG_DRIVER_VERSION);
    return len;
}

static ssize_t show_power_mode_value(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    len += snprintf(buf+len, PAGE_SIZE-len, "%s mode\n",
        obj->power_mode == BMG_NORMAL_MODE ? "normal" : "suspend");
    return len;
}

static ssize_t store_power_mode_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    int err;
    unsigned long power_mode;
    struct bmg_i2c_data *obj = obj_i2c_data;
    err = kstrtoul(buf, 10, &power_mode);
    if (err < 0)
        return err;
    if (BMI_GYRO_PM_NORMAL == power_mode)
        err = bmg_set_powermode(obj->client, BMG_NORMAL_MODE);
    else
        err = bmg_set_powermode(obj->client, BMG_SUSPEND_MODE);
    if (err < 0)
        GYRO_PR_ERR("set power mode failed.\n");
    return err;
}

static ssize_t show_range_value(struct device_driver *ddri, char *buf)
{
    ssize_t len = 0;
    struct bmg_i2c_data *obj = obj_i2c_data;
    len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->range);
    return len;
}

static ssize_t store_range_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    struct bmg_i2c_data *obj = obj_i2c_data;
    unsigned long range;
    int err;
    err = kstrtoul(buf, 10, &range);
    if (err == 0) {
        if ((range == BMG_RANGE_2000)
        || (range == BMG_RANGE_1000)
        || (range == BMG_RANGE_500)
        || (range == BMG_RANGE_250)
        || (range == BMG_RANGE_125)) {
            err = bmg_set_range(obj->client, range);
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
    struct bmg_i2c_data *obj = obj_i2c_data;
    len += snprintf(buf+len, PAGE_SIZE-len, "%d\n", obj->datarate);
    return len;
}

static ssize_t store_datarate_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    int err;
    unsigned long datarate;
    struct bmg_i2c_data *obj = obj_i2c_data;
    err = kstrtoul(buf, 10, &datarate);
    if (err < 0)
        return err;
    err = bmg_set_datarate(obj->client, datarate);
    if (err < 0) {
        GYRO_PR_ERR("set data rate failed.\n");
        return err;
    }
    return count;
}

//add devinfo of gyro by yanghongjiang 20171125
#ifdef CONFIG_LCT_DEVINFO_SUPPORT
#define SLT_DEVINFO_ACC_DEBUG
#include  "dev_info.h"
//static char* temp_comments;
struct devinfo_struct *s_DEVINFO_gyro;   
//The followd code is for GTP style
static void devinfo_gyro_regchar(char *module,char * vendor,char *version,char *used)
{

	s_DEVINFO_gyro =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);	
	s_DEVINFO_gyro->device_type="Gyro";
	s_DEVINFO_gyro->device_module=module;
	s_DEVINFO_gyro->device_vendor=vendor;
	s_DEVINFO_gyro->device_ic="BMI160";
	s_DEVINFO_gyro->device_info=DEVINFO_NULL;
	s_DEVINFO_gyro->device_version=version;
	s_DEVINFO_gyro->device_used=used;
#ifdef SLT_DEVINFO_ACC_DEBUG
		printk("[DEVINFO magnetometer sensor]registe msensor device! type:<%s> module:<%s> vendor<%s> ic<%s> version<%s> info<%s> used<%s>\n",
				s_DEVINFO_gyro->device_type,s_DEVINFO_gyro->device_module,s_DEVINFO_gyro->device_vendor,
				s_DEVINFO_gyro->device_ic,s_DEVINFO_gyro->device_version,s_DEVINFO_gyro->device_info,s_DEVINFO_gyro->device_used);
#endif
       DEVINFO_CHECK_DECLARE(s_DEVINFO_gyro->device_type,s_DEVINFO_gyro->device_module,s_DEVINFO_gyro->device_vendor,s_DEVINFO_gyro->device_ic,s_DEVINFO_gyro->device_version,s_DEVINFO_gyro->device_info,s_DEVINFO_gyro->device_used);
}
      //devinfo_check_add_device(s_DEVINFO_ctp);


#endif

static DRIVER_ATTR(chipinfo, S_IWUSR | S_IRUGO, show_chipinfo_value, NULL);
static DRIVER_ATTR(sensordata, S_IWUSR | S_IRUGO, show_sensordata_value, NULL);
static DRIVER_ATTR(rawdata, S_IWUSR | S_IRUGO, show_rawdata_value, NULL);
static DRIVER_ATTR(cali, S_IWUSR | S_IRUGO, show_cali_value, store_cali_value);
static DRIVER_ATTR(firlen, S_IWUSR | S_IRUGO,
show_firlen_value, store_firlen_value);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO,
show_trace_value, store_trace_value);
static DRIVER_ATTR(status, S_IRUGO, show_status_value, NULL);
static DRIVER_ATTR(gyro_op_mode, S_IWUSR | S_IRUGO,
show_power_mode_value, store_power_mode_value);
static DRIVER_ATTR(gyro_range, S_IWUSR | S_IRUGO,
show_range_value, store_range_value);
static DRIVER_ATTR(gyro_odr, S_IWUSR | S_IRUGO,
show_datarate_value, store_datarate_value);

//add node for open log by yanghongjiang 20171215
static ssize_t store_openLog_value(struct device_driver *ddri,
        const char *buf, size_t count)
{
    int range;
    sscanf(buf,"%d",&range);
    printk("bmi160-gyro [%s] ,range = %d .\n",__func__,range);
    if(range > 0){
		isOpenLog = true;
	}else{
		isOpenLog = false;
	}
    return count;
}

static ssize_t show_openLog_value(struct device_driver *ddri, char *buf)
{
	int ret;
	printk("bmi160-gyro [%s] isShowLog = %d .\n",__func__,isOpenLog);
    ret = sprintf(buf,"[OpenLog] = %d \n",isOpenLog);
    return ret;
}

static DRIVER_ATTR(openLog, S_IWUSR | S_IRUGO, show_openLog_value, store_openLog_value);
//add by yanghongjiang end

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
    /* open log */
    &driver_attr_openLog,
};

static int bmg_create_attr(struct device_driver *driver)
{
    int idx = 0;
    int err = 0;
    int num = (int)(sizeof(bmg_attr_list)/sizeof(bmg_attr_list[0]));
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
    int num = (int)(sizeof(bmg_attr_list)/sizeof(bmg_attr_list[0]));
    for (idx = 0; idx < num; idx++)
        driver_remove_file(driver, bmg_attr_list[idx]);
    return err;
}

static int bmg_open(struct inode *inode, struct file *file)
{
    file->private_data = obj_i2c_data;
    if (file->private_data == NULL) {
        GYRO_PR_ERR("file->private_data is null pointer.\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}

static int bmg_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static long bmg_unlocked_ioctl(struct file *file, unsigned int cmd,
        unsigned long arg)
{
    int err = 0;
    char strbuf[BMG_BUFSIZE] = {0};
    int raw_offset[BMG_BUFSIZE] = {0};
    void __user *data;
    struct SENSOR_DATA sensor_data;
    int cali[BMG_AXES_NUM] = {0};
    struct bmg_i2c_data *obj = (struct bmg_i2c_data *)file->private_data;
    struct i2c_client *client = obj->client;

    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE,
            (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,
            (void __user *)arg, _IOC_SIZE(cmd));
    if (err) {
        GYRO_PR_ERR("access error: %08x, (%2d, %2d)\n",
            cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }
    switch (cmd) {
    case GYROSCOPE_IOCTL_INIT:
        bmg_init_client(client, 0);
        err = bmg_set_powermode(client, BMG_NORMAL_MODE);
        if (err) {
            err = -EFAULT;
            break;
        }
        break;
    case GYROSCOPE_IOCTL_READ_SENSORDATA:
        data = (void __user *) arg;
        if (data == NULL) {
            err = -EINVAL;
            break;
        }

        bmg_read_sensor_data(client, strbuf, BMG_BUFSIZE);
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
            GYRO_PR_ERR("perform calibration in suspend mode\n");
            err = -EINVAL;
        } else {
            /* convert: degree/second -> LSB */
            cali[BMG_AXIS_X] = (sensor_data.x * obj->sensitivity)/1310;
            cali[BMG_AXIS_Y] = (sensor_data.y * obj->sensitivity)/1310;
            cali[BMG_AXIS_Z] = (sensor_data.z * obj->sensitivity)/1310;
            err = bmg_write_calibration(client, cali);
        }
        break;
    case GYROSCOPE_IOCTL_CLR_CALI:
        err = bmg_reset_calibration(client);
        break;
    case GYROSCOPE_IOCTL_GET_CALI:
        data = (void __user *)arg;
        if (data == NULL) {
            err = -EINVAL;
            break;
        }
        err = bmg_read_calibration(client, cali, raw_offset);
        if (err)
            break;

        sensor_data.x = (cali[BMG_AXIS_X]* 1310) / obj->sensitivity;
        sensor_data.y = (cali[BMG_AXIS_Y]* 1310) / obj->sensitivity;
        sensor_data.z = (cali[BMG_AXIS_Z]* 1310) / obj->sensitivity;
        if (copy_to_user(
            data, &sensor_data, sizeof(sensor_data))) {
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

static const struct file_operations bmg_fops = {
    .owner = THIS_MODULE,
    .open = bmg_open,
    .release = bmg_release,
    .unlocked_ioctl = bmg_unlocked_ioctl,
};

static struct miscdevice bmg_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "gyroscope",
    .fops = &bmg_fops,
};

static int bmi160_gyro_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
	struct bmg_i2c_data *obj = i2c_get_clientdata(client);
	int err = 0;
	GYRO_FUN();

//	if (msg.event == PM_EVENT_SUSPEND) {
		if (obj == NULL) {
			GYRO_PR_ERR("null pointer\n");
			return -EINVAL;
		}

		atomic_set(&obj->suspend, 1);
		/*
		err = bmg_set_powermode(obj->client, BMG_SUSPEND_MODE);
		if (err) {
			GYRO_PR_ERR("bmg set suspend mode failed, err = %d\n",
				err);
			return err;
		}
		bmg_power(obj->hw, 0);*/
//	}   
	return err;
}

static int bmi160_gyro_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
	struct bmg_i2c_data *obj = i2c_get_clientdata(client);
//	int err;

	GYRO_FUN();

	if (obj == NULL) {
		GYRO_PR_ERR("null pointer\n");
		return -EINVAL;
	}

	//bmg_power(obj->hw, 1);
/*	err = bmg_init_client(obj -> client, 0);
	if (err) {
		GYRO_PR_ERR("initialize client failed, err = %d\n", err);
		return err;
	}
*/
//	bmg_set_powermode(client,power);
//	mdelay(10);

	atomic_set(&obj->suspend, 0);
	return 0;
}

static int bmi160_gyro_i2c_detect(struct i2c_client *client,
        struct i2c_board_info *info)
{
    strlcpy(info->type, BMG_DEV_NAME, 48);
    return 0;
}

static int bmi160_gyro_open_report_data(int open)
{
    return 0;
}

static int bmi160_gyro_enable_nodata(int en)
{
    int err = 0;
    int retry = 0;
    bool power = false;
    if (1 == en)
        power = true;
    else
        power = false;
    for (retry = 0; retry < 3; retry++) {
        err = bmg_set_powermode(obj_i2c_data->client, power);
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
    struct bmg_i2c_data *priv = obj_i2c_data;
    err = bmg_set_datarate(priv->client, sample_delay);
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

static int bmi160_gyro_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
#ifdef CUSTOM_KERNEL_SENSORHUB
    int err = 0;
    uint32_t period_ms;
    uint32_t timeout_ms;
    SCP_SENSOR_HUB_DATA req;
    int len;

    period_ms = samplingPeriodNs/1000000;
    timeout_ms = maxBatchReportLatencyNs/1000000;

    req.batch_req.sensorType = ID_ACCELEROMETER;
    req.batch_req.action = SENSOR_HUB_BATCH;
    req.batch_req.flag = flags;
    req.batch_req.period_ms = period_ms;
    req.batch_req.timeout_ms = timeout_ms;

    len = sizeof(req.batch_req);

    err = SCP_sensorHub_req_send(&req, &len, 1);
    if (err)
    {
        GYRO_PR_ERR("SCP_sensorHub_req_send fail : %d!\n", err);
    }
	 return err;
#endif
    return bmi160_gyro_set_delay((u64)samplingPeriodNs);
}

static int bmi160_gyro_flush(void)
{
    return gyro_flush_report();
}

static int bmi160_gyro_get_data(int *x, int *y, int *z, int *status)
{
    char buff[BMG_BUFSIZE] = {0};
    bmg_read_sensor_data(obj_i2c_data->client, buff, BMG_BUFSIZE);
    sscanf(buff, "%x %x %x", x, y, z);
    *status = SENSOR_STATUS_ACCURACY_MEDIUM;
    return 0;
}

static int bmi160_gyro_i2c_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    struct bmg_i2c_data *obj;
    struct gyro_control_path ctl = {0};
    struct gyro_data_path data = {0};
	isOpenLog = false;
    err = get_gyro_dts_func(client->dev.of_node, hw);
    if (!hw) {
        GYRO_PR_ERR("get dts info fail\n");
    }

    obj = kzalloc(sizeof(*obj), GFP_KERNEL);
    if (!obj) {
        err = -ENOMEM;
        goto exit;
    }
    obj->hw = hw;
#ifdef  CONFIG_LCT_3500
    obj->hw->direction = 1;
#endif
    err = hwmsen_get_convert(obj->hw->direction, &obj->cvt);
    if (err) {
        GYRO_PR_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit_hwmsen_get_convert_failed;
    }
    
    client->addr = 0x68;
	printk("+++yhj check gyro_default_i2c_addr: %x\n", client->addr);
	
    obj_i2c_data = obj;
#ifdef CONFIG_L3510_MAINBOARD
    obj->client = bmi160_acc_i2c_client;
#else
    obj->client = client; 
#endif
    i2c_set_clientdata(obj->client, obj);
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
    err = bmg_init_client(obj->client, 1);
    if (err)
        goto exit_init_client_failed;

    err = misc_register(&bmg_device);
    if (err) {
        GYRO_PR_ERR("misc device register failed, err = %d\n", err);
        goto exit_misc_device_register_failed;
    }

    err = bmg_create_attr(
        &bmi160_gyro_init_info.platform_diver_addr->driver);
    if (err) {
        GYRO_PR_ERR("create attribute failed, err = %d\n", err);
        goto exit_create_attr_failed;
    }
    ctl.open_report_data = bmi160_gyro_open_report_data;
    ctl.enable_nodata = bmi160_gyro_enable_nodata;
    ctl.batch = bmi160_gyro_batch;
    ctl.flush = bmi160_gyro_flush;
    ctl.set_delay  = bmi160_gyro_set_delay;
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

//add devinfo of gyro by yanghongjiang 20171125
   #ifdef CONFIG_LCT_DEVINFO_SUPPORT
	devinfo_gyro_regchar("BMI160","BOSCH","1.5",DEVINFO_USED);
   #endif
//end of add

    bmi160_gyro_init_flag = 0;
    GYRO_LOG("%s: OK\n", __func__);
    return 0;

exit_create_attr_failed:
    misc_deregister(&bmg_device);
exit_misc_device_register_failed:
exit_init_client_failed:
exit_hwmsen_get_convert_failed:
    kfree(obj);
exit:
    bmi160_gyro_init_flag = -1;
    GYRO_PR_ERR("err = %d\n", err);
    return err;
}

static int bmi160_gyro_i2c_remove(struct i2c_client *client)
{
    int err = 0;

    err = bmg_delete_attr(
        &bmi160_gyro_init_info.platform_diver_addr->driver);
    if (err)
        GYRO_PR_ERR("bmg_delete_attr failed, err = %d\n", err);
#if 0
    err = misc_deregister(&bmg_device);
    if (err)
        GYRO_PR_ERR("misc_deregister failed, err = %d\n", err);
#endif

    obj_i2c_data = NULL;
    i2c_unregister_device(client);
    kfree(i2c_get_clientdata(client));

    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id gyro_of_match[] = {
    {.compatible = "mediatek,gyro"},
    {},
};
#endif

#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops bmi160_gyro_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(bmi160_gyro_suspend, bmi160_gyro_resume)
};
#endif

static struct i2c_driver bmg_i2c_driver = {
    .driver = {
        .name = BMG_DEV_NAME,
#ifdef CONFIG_OF
        .of_match_table = gyro_of_match,
#endif
//#ifdef CONFIG_PM_SLEEP
//	.pm = &bmi160_gyro_pm_ops,
//#endif
    },
    .probe = bmi160_gyro_i2c_probe,
    .remove = bmi160_gyro_i2c_remove,
    .detect = bmi160_gyro_i2c_detect,
    .id_table = bmi160_gyro_i2c_id,
};

static int bmi160_gyro_remove(void)
{
    i2c_del_driver(&bmg_i2c_driver);
    return 0;
}

static int bmi160_gyro_local_init(struct platform_device *pdev)
{
    int err = 0;
    /*need define in the DTS side for bmi160_gyro*/
    /*
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    struct i2c_board_info info;
    adapter = i2c_get_adapter(3);
    if (adapter == NULL)
        printk(KERN_ERR"gsensor  error");
    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = 0x61;
    strlcpy(info.type, "bmi160_gyro", I2C_NAME_SIZE);
    client = i2c_new_device(adapter, &info);
    strlcpy(client->name, "bmi160_gyro", I2C_NAME_SIZE);
    */
    if (i2c_add_driver(&bmg_i2c_driver)) {
        GYRO_PR_ERR("add gyro driver error.\n");
        err = -1;
        return err;
    }
    if (-1 == bmi160_gyro_init_flag) {
        err = -1;
        return err;
    }
    GYRO_LOG("bmi160 gyro init ok.\n");
    return 0;
}

static struct gyro_init_info bmi160_gyro_init_info = {
        .name = BMG_DEV_NAME,
        .init = bmi160_gyro_local_init,
        .uninit = bmi160_gyro_remove,
};

static int __init bmi160_gyro_init(void)
{
#if 0
    const char *name = "mediatek,bmi160_gyro";
    hw = get_gyro_dts_func(name, hw);
    if (!hw) {
        GYRO_PR_ERR("Gyro device tree configuration error.\n");
        return 0;
    }
    GYRO_LOG("%s: bosch gyroscope driver version: %s\n",
    __func__, BMG_DRIVER_VERSION);
#endif
    gyro_driver_add(&bmi160_gyro_init_info);
    return 0;
}

static void __exit bmi160_gyro_exit(void)
{
    GYRO_FUN();
}

module_init(bmi160_gyro_init);
module_exit(bmi160_gyro_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMI160 GYRO Driver");
MODULE_AUTHOR("contact@bosch-sensortec.com>");

/* BMI160_ACC motion sensor driver
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
#include "bmi160_accgyro.h"

#include "linux/platform_data/spi-mt65xx.h"

#define SW_CALIBRATION
#define BYTES_PER_LINE		(16)

static int bmi160_accelgyro_spi_probe(struct spi_device *spi);
static int bmi160_accelgyro_spi_remove(struct spi_device *spi);
static int bmi160_accelgyro_local_init(void);
static int bmi160_accelgyro_remove(void);

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
struct gyro_data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMG_AXES_NUM];
	int sum[BMG_AXES_NUM];
	int num;
	int idx;
};


struct bmi160_accelgyro_data {
	struct accelgyro_hw *hw;
	struct hwmsen_convert cvt;
	struct bmi160_t device;
	/*misc */
	struct data_resolution *accel_reso;
	atomic_t accel_trace;
	atomic_t accel_suspend;
	atomic_t accel_selftest;
	atomic_t accel_filter;
	s16 accel_cali_sw[BMI160_ACC_AXES_NUM + 1];
	struct mutex accel_lock;
	struct mutex spi_lock;
	/* +1: for 4-byte alignment */
	s8 accel_offset[BMI160_ACC_AXES_NUM + 1];
	s16 accel_data[BMI160_ACC_AXES_NUM + 1];
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
	enum BMG_POWERMODE_ENUM gyro_power_mode;
	enum BMG_RANGE_ENUM gyro_range;
	int gyro_datarate;
	/* sensitivity = 2^bitnum/range
	 * [+/-2000 = 4000; +/-1000 = 2000;
	 * +/-500 = 1000; +/-250 = 500;
	 * +/-125 = 250 ]
	 */
	u16 gyro_sensitivity;
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

struct bmi160_axis_data_t {
	s16 x;
	s16 y;
	s16 z;
};



static struct accelgyro_init_info bmi160_accelgyro_init_info;
static struct bmi160_accelgyro_data *accelgyro_obj_data;
static bool sensor_power = true;
static struct GSENSOR_VECTOR3D gsensor_gain;

/* signification motion flag*/
int sig_flag;

static int bmi160_acc_init_flag = -1;
struct bmi160_t *p_bmi160;

struct accelgyro_hw accelgyro_cust;
static struct accelgyro_hw *hw = &accelgyro_cust;

#ifdef MTK_NEW_ARCH_ACCEL
struct platform_device *accelPltFmDev;
#endif

/* 0=OK, -1=fail */
static int bmi160_gyro_init_flag = -1;
static struct bmi160_accelgyro_data *gyro_obj_data;
static int bmg_set_powermode(struct bmi160_accelgyro_data *obj, enum BMG_POWERMODE_ENUM power_mode);

static struct data_resolution bmi160_acc_data_resolution[1] = {
	{{0, 12}, 8192}		/* +/-4G range */
};

static struct data_resolution bmi160_acc_offset_resolution = { {0, 12}, 8192 };

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

	xfer[0].speed_hz = 4*1000*1000;	/*4M*/

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

	xfer[0].speed_hz = 4*1000*1000;

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

		xfer[0].speed_hz = 4*1000*1000;

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
		ACC_PR_ERR("read bmi160 i2c failed.\n");
	return err;
}

s8 bmi_write_wrapper(u8 dev_addr, u8 reg_addr, u8 *data, u8 len)
{
	int err = 0;

	err = BMP160_spi_write_bytes(accelgyro_obj_data->spi, reg_addr, data, len);
	if (err < 0)
		ACC_PR_ERR("read bmi160 i2c failed.\n");
	return err;
}

static void bmi_delay(u32 msec)
{
	mdelay(msec);
}

/*!
 * @brief Set data resolution
 *
 * @param[in] client the pointer of bmi160_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetDataResolution(struct bmi160_accelgyro_data *obj)
{
	obj->accel_reso = &bmi160_acc_data_resolution[0];
	return 0;
}

static int BMI160_ACC_ReadData(struct bmi160_accelgyro_data *obj, s16 data[BMI160_ACC_AXES_NUM])
{
	int err = 0;
	u8 addr = BMI160_USER_DATA_14_ACC_X_LSB__REG;
	u8 buf[BMI160_ACC_DATA_LEN] = { 0 };

	err = BMP160_spi_read_bytes(obj->spi, addr, buf, BMI160_ACC_DATA_LEN);
	if (err) {
		ACC_PR_ERR("read data failed.\n");
	} else {
		/* Convert sensor raw data to 16-bit integer */
		/*Data X */
		data[BMI160_ACC_AXIS_X] = (s16) ((((s32) ((s8) buf[1]))
						  << BMI160_SHIFT_8_POSITION) | (buf[0]));
		 /*Data Y */
		data[BMI160_ACC_AXIS_Y] = (s16) ((((s32) ((s8) buf[3]))
						  << BMI160_SHIFT_8_POSITION) | (buf[2]));
		/* Data Z */
		data[BMI160_ACC_AXIS_Z] = (s16) ((((s32) ((s8) buf[5]))
						  << BMI160_SHIFT_8_POSITION) | (buf[4]));
	}
	return err;
}

static int BMI160_ACC_ReadOffset(struct bmi160_accelgyro_data *obj, s8 ofs[BMI160_ACC_AXES_NUM])
{
	int err = 0;
#ifdef SW_CALIBRATION
	ofs[0] = ofs[1] = ofs[2] = 0x0;
#else
	err = BMP160_spi_read_bytes(obj->spi, BMI160_ACC_REG_OFSX, ofs, BMI160_ACC_AXES_NUM);
	if (err)
		ACC_PR_ERR("error: %d\n", err);
#endif
	return err;
}


/*!
 * @brief Reset calibration for acc
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_ResetCalibration(struct bmi160_accelgyro_data *obj)
{
	int err = 0;
#ifdef SW_CALIBRATION

#else
	u8 ofs[4] = { 0, 0, 0, 0 };

	err = BMP160_spi_write_bytes(obj->spi, BMI160_ACC_REG_OFSX, ofs, 4);
	if (err)
		ACC_PR_ERR("write accel_offset failed.\n");
#endif
	memset(obj->accel_cali_sw, 0x00, sizeof(obj->accel_cali_sw));
	memset(obj->accel_offset, 0x00, sizeof(obj->accel_offset));
	return err;
}

static int BMI160_ACC_ReadCalibration(struct bmi160_accelgyro_data *obj, int dat[BMI160_ACC_AXES_NUM])
{
	int err = 0;
	int mul;

#ifdef SW_CALIBRATION
	mul = 0;		/* only SW Calibration, disable HW Calibration */
#else
	err = BMI160_ACC_ReadOffset(obj, obj->accel_offset);
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	mul = obj->accel_reso->sensitivity / bmi160_acc_offset_resolution.sensitivity;
#endif

	dat[obj->cvt.map[BMI160_ACC_AXIS_X]] = obj->cvt.sign[BMI160_ACC_AXIS_X]
	    * (obj->accel_offset[BMI160_ACC_AXIS_X] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_X]);
	dat[obj->cvt.map[BMI160_ACC_AXIS_Y]] = obj->cvt.sign[BMI160_ACC_AXIS_Y]
	    * (obj->accel_offset[BMI160_ACC_AXIS_Y] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Y]);
	dat[obj->cvt.map[BMI160_ACC_AXIS_Z]] = obj->cvt.sign[BMI160_ACC_AXIS_Z]
	    * (obj->accel_offset[BMI160_ACC_AXIS_Z] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Z]);

	return err;
}

static int BMI160_ACC_ReadCalibrationEx(struct bmi160_accelgyro_data *obj,
					int act[BMI160_ACC_AXES_NUM], int raw[BMI160_ACC_AXES_NUM])
{
	/* raw: the raw calibration data; act: the actual calibration data */
	int mul;
#ifdef SW_CALIBRATION
	/* only SW Calibration, disable  Calibration */
	mul = 0;
#else
	int err;

	err = BMI160_ACC_ReadOffset(obj, obj->accel_offset);
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	mul = obj->accel_reso->sensitivity / bmi160_acc_offset_resolution.sensitivity;
#endif

	raw[BMI160_ACC_AXIS_X] =
	    obj->accel_offset[BMI160_ACC_AXIS_X] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_X];
	raw[BMI160_ACC_AXIS_Y] =
	    obj->accel_offset[BMI160_ACC_AXIS_Y] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Y];
	raw[BMI160_ACC_AXIS_Z] =
	    obj->accel_offset[BMI160_ACC_AXIS_Z] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Z];

	act[obj->cvt.map[BMI160_ACC_AXIS_X]] =
	    obj->cvt.sign[BMI160_ACC_AXIS_X] * raw[BMI160_ACC_AXIS_X];
	act[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Y] * raw[BMI160_ACC_AXIS_Y];
	act[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Z] * raw[BMI160_ACC_AXIS_Z];

	return 0;
}

static int BMI160_ACC_WriteCalibration(struct bmi160_accelgyro_data *obj, int dat[BMI160_ACC_AXES_NUM])
{
	int err = 0;
	int cali[BMI160_ACC_AXES_NUM] = { 0 };
	int raw[BMI160_ACC_AXES_NUM] = { 0 };
#ifndef SW_CALIBRATION
	int lsb = bmi160_acc_offset_resolution.sensitivity;
	int divisor = obj->accel_reso->sensitivity / lsb;
#endif
	err = BMI160_ACC_ReadCalibrationEx(obj, cali, raw);
	/* accel_offset will be updated in obj->accel_offset */
	if (err) {
		ACC_PR_ERR("read accel_offset fail, %d\n", err);
		return err;
	}
	ACC_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		raw[BMI160_ACC_AXIS_X], raw[BMI160_ACC_AXIS_Y], raw[BMI160_ACC_AXIS_Z],
		obj->accel_offset[BMI160_ACC_AXIS_X], obj->accel_offset[BMI160_ACC_AXIS_Y],
		obj->accel_offset[BMI160_ACC_AXIS_Z], obj->accel_cali_sw[BMI160_ACC_AXIS_X],
		obj->accel_cali_sw[BMI160_ACC_AXIS_Y], obj->accel_cali_sw[BMI160_ACC_AXIS_Z]);
	/* calculate the real accel_offset expected by caller */
	cali[BMI160_ACC_AXIS_X] += dat[BMI160_ACC_AXIS_X];
	cali[BMI160_ACC_AXIS_Y] += dat[BMI160_ACC_AXIS_Y];
	cali[BMI160_ACC_AXIS_Z] += dat[BMI160_ACC_AXIS_Z];
	ACC_LOG("UPDATE: (%+3d %+3d %+3d)\n",
		dat[BMI160_ACC_AXIS_X], dat[BMI160_ACC_AXIS_Y], dat[BMI160_ACC_AXIS_Z]);
#ifdef SW_CALIBRATION
	obj->accel_cali_sw[BMI160_ACC_AXIS_X] =
	    obj->cvt.sign[BMI160_ACC_AXIS_X] * (cali[obj->cvt.map[BMI160_ACC_AXIS_X]]);
	obj->accel_cali_sw[BMI160_ACC_AXIS_Y] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Y] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Y]]);
	obj->accel_cali_sw[BMI160_ACC_AXIS_Z] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Z] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Z]]);
#else
	obj->accel_offset[BMI160_ACC_AXIS_X] =
	    (s8) (obj->cvt.sign[BMI160_ACC_AXIS_X] * (cali[obj->cvt.map[BMI160_ACC_AXIS_X]]) / (divisor));
	obj->accel_offset[BMI160_ACC_AXIS_Y] =
	    (s8) (obj->cvt.sign[BMI160_ACC_AXIS_Y] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Y]]) / (divisor));
	obj->accel_offset[BMI160_ACC_AXIS_Z] =
	    (s8) (obj->cvt.sign[BMI160_ACC_AXIS_Z] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Z]]) / (divisor));

	/*convert software calibration using standard calibration */
	obj->accel_cali_sw[BMI160_ACC_AXIS_X] =
	    obj->cvt.sign[BMI160_ACC_AXIS_X] * (cali[obj->cvt.map[BMI160_ACC_AXIS_X]]) % (divisor);
	obj->accel_cali_sw[BMI160_ACC_AXIS_Y] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Y] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Y]]) % (divisor);
	obj->accel_cali_sw[BMI160_ACC_AXIS_Z] =
	    obj->cvt.sign[BMI160_ACC_AXIS_Z] * (cali[obj->cvt.map[BMI160_ACC_AXIS_Z]]) % (divisor);

	ACC_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n",
		obj->accel_offset[BMI160_ACC_AXIS_X] * divisor + obj->accel_cali_sw[BMI160_ACC_AXIS_X],
		obj->accel_offset[BMI160_ACC_AXIS_Y] * divisor + obj->accel_cali_sw[BMI160_ACC_AXIS_Y],
		obj->accel_offset[BMI160_ACC_AXIS_Z] * divisor + obj->accel_cali_sw[BMI160_ACC_AXIS_Z],
		obj->accel_offset[BMI160_ACC_AXIS_X], obj->accel_offset[BMI160_ACC_AXIS_Y],
		obj->accel_offset[BMI160_ACC_AXIS_Z], obj->accel_cali_sw[BMI160_ACC_AXIS_X],
		obj->accel_cali_sw[BMI160_ACC_AXIS_Y], obj->accel_cali_sw[BMI160_ACC_AXIS_Z]);

	err =
	    BMP160_spi_write_bytes(obj->spi, BMI160_ACC_REG_OFSX, obj->accel_offset, BMI160_ACC_AXES_NUM);
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
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_CheckDeviceID(struct bmi160_accelgyro_data *obj)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	err = BMP160_spi_read_bytes(obj->spi, 0x7F, databuf, 1);
	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_CHIP_ID__REG, databuf, 1);
	if (err < 0) {
		ACC_PR_ERR("read chip id failed.\n");
		return BMI160_ACC_ERR_SPI;
	}
	switch (databuf[0]) {
	case SENSOR_CHIP_ID_BMI:
	case SENSOR_CHIP_ID_BMI_C2:
	case SENSOR_CHIP_ID_BMI_C3:
		ACC_LOG("check chip id %d successfully.\n", databuf[0]);
		break;
	default:
		ACC_LOG("check chip id %d %d failed.\n", databuf[0], databuf[1]);
		break;
	}
	return err;
}

/*!
 * @brief Set power mode for acc
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 * @param[in] enable
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetPowerMode(struct bmi160_accelgyro_data *obj, bool enable)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	if (enable == sensor_power) {
		ACC_LOG("power status is newest!\n");
		return 0;
	}
	mutex_lock(&obj->accel_lock);
	if (enable == true)
		databuf[0] = CMD_PMU_ACC_NORMAL;
	else
		databuf[0] = CMD_PMU_ACC_SUSPEND;

	err = BMP160_spi_write_bytes(obj->spi, BMI160_CMD_COMMANDS__REG, &databuf[0], 1);
	if (err < 0) {
		ACC_PR_ERR("write power mode value to register failed.\n");
		mutex_unlock(&obj->accel_lock);
		return BMI160_ACC_ERR_SPI;
	}
	sensor_power = enable;
	mdelay(1);
	mutex_unlock(&obj->accel_lock);
	ACC_LOG("set power mode enable = %d ok!\n", enable);
	return 0;
}

/*!
 * @brief Set range value for acc
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 * @param[in] range value
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetDataFormat(struct bmi160_accelgyro_data *obj, u8 dataformat)
{
	int res = 0;
	u8 databuf[2] = { 0 };

	mutex_lock(&obj->accel_lock);
	res = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0], BMI160_USER_ACC_RANGE, dataformat);
	res += BMP160_spi_write_bytes(obj->spi, BMI160_USER_ACC_RANGE__REG, &databuf[0], 1);
	mdelay(1);
	if (res < 0) {
		ACC_PR_ERR("set range failed.\n");
		mutex_unlock(&obj->accel_lock);
		return BMI160_ACC_ERR_SPI;
	}
	mutex_unlock(&obj->accel_lock);
	return BMI160_ACC_SetDataResolution(obj);
}

/*!
 * @brief Set bandwidth for acc
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 * @param[in] bandwidth value
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetBWRate(struct bmi160_accelgyro_data *obj, u8 bwrate)
{
	int err = 0;
	u8 databuf[2] = { 0 };

	if (bwrate == 0)
		return 0;
	mutex_lock(&obj->accel_lock);
	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0], BMI160_USER_ACC_CONF_ODR, bwrate);
	err += BMP160_spi_write_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	mdelay(20);
	if (err < 0) {
		ACC_PR_ERR("set bandwidth failed, res = %d\n", err);
		mutex_unlock(&obj->accel_lock);
		return BMI160_ACC_ERR_SPI;
	}
	ACC_LOG("set bandwidth = %d ok.\n", bwrate);
	mutex_unlock(&obj->accel_lock);
	return err;
}

/*!
 * @brief Set OSR for acc
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetOSR4(struct bmi160_accelgyro_data *obj)
{
	int err = 0;
	uint8_t databuf[2] = { 0 };
	uint8_t bandwidth = BMI160_ACCEL_OSR4_AVG1;
	uint8_t accel_undersampling_parameter = 0;

	mutex_lock(&obj->accel_lock);
	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0], BMI160_USER_ACC_CONF_ACC_BWP, bandwidth);
	databuf[0] = BMI160_SET_BITSLICE(databuf[0],
					 BMI160_USER_ACC_CONF_ACC_UNDER_SAMPLING,
					 accel_undersampling_parameter);
	err += BMP160_spi_write_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, &databuf[0], 1);
	mdelay(10);
	if (err < 0) {
		ACC_PR_ERR("set OSR failed.\n");
		mutex_unlock(&obj->accel_lock);
		return BMI160_ACC_ERR_SPI;
	}
	ACC_LOG("[%s] acc_bmp = %d, acc_us = %d ok.\n", __func__,
		bandwidth, accel_undersampling_parameter);
	mutex_unlock(&obj->accel_lock);
	return err;
}

/*!
 * @brief Set interrupt enable
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 * @param[in] enable for interrupt
 *
 * @return zero success, non-zero failed
 */
static int BMI160_ACC_SetIntEnable(struct bmi160_accelgyro_data *obj, u8 enable)
{
	int err = 0;

	mutex_lock(&obj->accel_lock);
	err = BMP160_spi_write_bytes(obj->spi, BMI160_USER_INT_EN_0_ADDR, &enable, 0x01);
	mdelay(1);
	if (err < 0) {
		mutex_unlock(&obj->accel_lock);
		return err;
	}
	err = BMP160_spi_write_bytes(obj->spi, BMI160_USER_INT_EN_1_ADDR, &enable, 0x01);
	mdelay(1);
	if (err < 0) {
		mutex_unlock(&obj->accel_lock);
		return err;
	}
	err = BMP160_spi_write_bytes(obj->spi, BMI160_USER_INT_EN_2_ADDR, &enable, 0x01);
	mdelay(1);
	if (err < 0) {
		mutex_unlock(&obj->accel_lock);
		return err;
	}
	mutex_unlock(&obj->accel_lock);
	ACC_LOG("bmi160 set interrupt enable = %d ok.\n", enable);
	return 0;
}

/*!
 * @brief bmi160 initialization
 *
 * @param[in] obj the pointer of struct bmi160_accelgyro_data
 * @param[in] int reset calibration value
 *
 * @return zero success, non-zero failed
 */
static int bmi160_acc_init_client(struct bmi160_accelgyro_data *obj, int reset_cali)
{
	int err = 0;

	err = BMI160_ACC_CheckDeviceID(obj);
	if (err < 0)
		return err;

	err = BMI160_ACC_SetBWRate(obj, BMI160_ACCEL_ODR_200HZ);
	if (err < 0)
		return err;

	err = BMI160_ACC_SetOSR4(obj);
	if (err < 0)
		return err;

	err = BMI160_ACC_SetDataFormat(obj, BMI160_ACCEL_RANGE_4G);
	if (err < 0)
		return err;

	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = obj->accel_reso->sensitivity;

	err = BMI160_ACC_SetIntEnable(obj, 0);
	if (err < 0)
		return err;

	err = BMI160_ACC_SetPowerMode(obj, false);
	if (err < 0)
		return err;

	if (reset_cali != 0) {
		/* reset calibration only in power on */
		err = BMI160_ACC_ResetCalibration(obj);
		if (err < 0)
			return err;
	}
	ACC_LOG("bmi160 acc init OK.\n");
	return 0;
}

static int BMI160_ACC_ReadChipInfo(struct bmi160_accelgyro_data *obj, char *buf, int bufsize)
{
	sprintf(buf, "bmi160_acc");
	return 0;
}


static int BMI160_ACC_CompassReadData(struct bmi160_accelgyro_data *obj, char *buf, int bufsize)
{
	int res = 0;
	int acc[BMI160_ACC_AXES_NUM] = { 0 };
	s16 databuf[BMI160_ACC_AXES_NUM] = { 0 };

	if (sensor_power == false) {
		res = BMI160_ACC_SetPowerMode(obj, true);
		if (res)
			ACC_PR_ERR("Power on bmi160_acc error %d!\n", res);
	}
	res = BMI160_ACC_ReadData(obj, databuf);
	if (res) {
		ACC_PR_ERR("read acc data failed.\n");
		return -3;
	}
	/* Add compensated value performed by MTK calibration process */
	databuf[BMI160_ACC_AXIS_X] += obj->accel_cali_sw[BMI160_ACC_AXIS_X];
	databuf[BMI160_ACC_AXIS_Y] += obj->accel_cali_sw[BMI160_ACC_AXIS_Y];
	databuf[BMI160_ACC_AXIS_Z] += obj->accel_cali_sw[BMI160_ACC_AXIS_Z];
	/*remap coordinate */
	acc[obj->cvt.map[BMI160_ACC_AXIS_X]] =
		obj->cvt.sign[BMI160_ACC_AXIS_X] * databuf[BMI160_ACC_AXIS_X];
	acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
		obj->cvt.sign[BMI160_ACC_AXIS_Y] * databuf[BMI160_ACC_AXIS_Y];
	acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
		obj->cvt.sign[BMI160_ACC_AXIS_Z] * databuf[BMI160_ACC_AXIS_Z];

	sprintf(buf, "%d %d %d",
		(s16) acc[BMI160_ACC_AXIS_X],
		(s16) acc[BMI160_ACC_AXIS_Y], (s16) acc[BMI160_ACC_AXIS_Z]);

	return 0;
}

static int BMI160_ACC_ReadSensorData(struct bmi160_accelgyro_data *obj, char *buf, int bufsize)
{
	int err = 0;
	int acc[BMI160_ACC_AXES_NUM] = { 0 };
	s16 databuf[BMI160_ACC_AXES_NUM] = { 0 };

	if (sensor_power == false) {
		err = BMI160_ACC_SetPowerMode(obj, true);
		if (err) {
			ACC_PR_ERR("set power on acc failed.\n");
			return err;
		}
	}
	err = BMI160_ACC_ReadData(obj, databuf);
	if (err) {
		ACC_PR_ERR("read acc data failed.\n");
		return err;
	}
	databuf[BMI160_ACC_AXIS_X] += obj->accel_cali_sw[BMI160_ACC_AXIS_X];
	databuf[BMI160_ACC_AXIS_Y] += obj->accel_cali_sw[BMI160_ACC_AXIS_Y];
	databuf[BMI160_ACC_AXIS_Z] += obj->accel_cali_sw[BMI160_ACC_AXIS_Z];
	/* remap coordinate */
	acc[obj->cvt.map[BMI160_ACC_AXIS_X]] =
		obj->cvt.sign[BMI160_ACC_AXIS_X] * databuf[BMI160_ACC_AXIS_X];
	acc[obj->cvt.map[BMI160_ACC_AXIS_Y]] =
		obj->cvt.sign[BMI160_ACC_AXIS_Y] * databuf[BMI160_ACC_AXIS_Y];
	acc[obj->cvt.map[BMI160_ACC_AXIS_Z]] =
		obj->cvt.sign[BMI160_ACC_AXIS_Z] * databuf[BMI160_ACC_AXIS_Z];
	/* Output the mg */
	acc[BMI160_ACC_AXIS_X] = acc[BMI160_ACC_AXIS_X]
		* GRAVITY_EARTH_1000 / obj->accel_reso->sensitivity;
	acc[BMI160_ACC_AXIS_Y] = acc[BMI160_ACC_AXIS_Y]
		* GRAVITY_EARTH_1000 / obj->accel_reso->sensitivity;
	acc[BMI160_ACC_AXIS_Z] = acc[BMI160_ACC_AXIS_Z]
		* GRAVITY_EARTH_1000 / obj->accel_reso->sensitivity;
	sprintf(buf, "%04x %04x %04x",
		acc[BMI160_ACC_AXIS_X], acc[BMI160_ACC_AXIS_Y], acc[BMI160_ACC_AXIS_Z]);

	return 0;
}

static int BMI160_ACC_ReadRawData(struct bmi160_accelgyro_data *obj, char *buf)
{
	int err = 0;
	s16 databuf[BMI160_ACC_AXES_NUM] = { 0 };

	err = BMI160_ACC_ReadData(obj, databuf);
	if (err) {
		ACC_PR_ERR("read acc raw data failed.\n");
		return -EIO;
	}
	sprintf(buf, "BMI160_ACC_ReadRawData %04x %04x %04x",
		databuf[BMI160_ACC_AXIS_X],
		databuf[BMI160_ACC_AXIS_Y], databuf[BMI160_ACC_AXIS_Z]);

	return 0;
}


int bmi160_acc_get_mode(struct bmi160_accelgyro_data *obj, unsigned char *mode)
{
	int comres = 0;
	u8 v_data_u8r = C_BMI160_ZERO_U8X;

	comres = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_PMU_STATUS__REG, &v_data_u8r, 1);
	*mode = BMI160_GET_BITSLICE(v_data_u8r, BMI160_USER_ACC_PMU_STATUS);
	return comres;
}

static int bmi160_acc_set_range(struct bmi160_accelgyro_data *obj, unsigned char range)
{
	int comres = 0;
	unsigned char data[2] = { BMI160_USER_ACC_RANGE__REG };

	if (obj == NULL)
		return -1;
	mutex_lock(&obj->accel_lock);
	comres = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_RANGE__REG, data + 1, 1);

	data[1] = BMI160_SET_BITSLICE(data[1], BMI160_USER_ACC_RANGE, range);

	/* comres = i2c_master_send(client, data, 2); */
	comres = BMP160_spi_write_bytes(obj->spi, data[0], &data[1], 1);
	mutex_unlock(&obj->accel_lock);
	if (comres <= 0)
		return BMI160_ACC_ERR_SPI;
	else
		return comres;
}

static int bmi160_acc_get_range(struct bmi160_accelgyro_data *obj, u8 *range)
{
	int comres = 0;
	u8 data;

	comres = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_RANGE__REG, &data, 1);
	*range = BMI160_GET_BITSLICE(data, BMI160_USER_ACC_RANGE);
	return comres;
}

static int bmi160_acc_set_bandwidth(struct bmi160_accelgyro_data *obj, unsigned char bandwidth)
{
	int comres = 0;
	unsigned char data[2] = { BMI160_USER_ACC_CONF_ODR__REG };

	ACC_LOG("[%s] bandwidth = %d\n", __func__, bandwidth);
	mutex_lock(&obj->accel_lock);
	comres = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, data + 1, 1);
	data[1] = BMI160_SET_BITSLICE(data[1], BMI160_USER_ACC_CONF_ODR, bandwidth);
	/* comres = i2c_master_send(client, data, 2); */
	comres = BMP160_spi_write_bytes(obj->spi, data[0], &data[1], 1);
	mdelay(1);
	mutex_unlock(&obj->accel_lock);
	if (comres <= 0)
		return BMI160_ACC_ERR_SPI;
	else
		return comres;
}

static int bmi160_acc_get_bandwidth(struct bmi160_accelgyro_data *obj, unsigned char *bandwidth)
{
	int comres = 0;

	comres = BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACC_CONF_ODR__REG, bandwidth, 1);
	*bandwidth = BMI160_GET_BITSLICE(*bandwidth, BMI160_USER_ACC_CONF_ODR);
	return comres;
}

static ssize_t accel_show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	char strbuf[BMI160_BUFSIZE];
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	BMI160_ACC_ReadChipInfo(obj, strbuf, BMI160_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_acc_op_mode_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	u8 data = 0;

	err = bmi160_acc_get_mode(accelgyro_obj_data, &data);
	if (err < 0) {
		ACC_PR_ERR("get acc op mode failed.\n");
		return err;
	}
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_store_acc_op_mode_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	ACC_LOG("store_acc_op_mode_value = %d .\n", (int)data);
	if (data == BMI160_ACC_MODE_NORMAL)
		err = BMI160_ACC_SetPowerMode(accelgyro_obj_data, true);
	else
		err = BMI160_ACC_SetPowerMode(accelgyro_obj_data, false);

	if (err < 0) {
		ACC_PR_ERR("set acc op mode = %d failed.\n", (int)data);
		return err;
	}
	return count;
}

static ssize_t accel_show_acc_range_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	u8 data;

	err = bmi160_acc_get_range(accelgyro_obj_data, &data);
	if (err < 0) {
		ACC_LOG("get acc range failed.\n");
		return err;
	}
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_store_acc_range_value(struct device_driver *ddri, const char *buf, size_t count)
{
	unsigned long data;
	int err;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	err = bmi160_acc_set_range(accelgyro_obj_data, (u8) data);
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

	err = bmi160_acc_get_bandwidth(accelgyro_obj_data, &data);
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
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	err = bmi160_acc_set_bandwidth(obj, (u8) data);
	if (err < 0) {
		ACC_PR_ERR("set acc bandwidth failed.\n");
		return err;
	}
	obj->accel_odr.acc_odr = data;
	return count;
}

static ssize_t accel_show_cpsdata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[BMI160_BUFSIZE] = { 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	BMI160_ACC_CompassReadData(obj, strbuf, BMI160_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_sensordata_value(struct device_driver *ddri, char *buf)
{
	char strbuf[BMI160_BUFSIZE] = { 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	BMI160_ACC_ReadSensorData(obj, strbuf, BMI160_BUFSIZE);
	return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);
}

static ssize_t accel_show_cali_value(struct device_driver *ddri, char *buf)
{
	int err = 0;
	int len = 0;
	int mul;
	int tmp[BMI160_ACC_AXES_NUM] = { 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	obj = accelgyro_obj_data;
	err = BMI160_ACC_ReadOffset(obj, obj->accel_offset);
	if (err)
		return -EINVAL;
	err = BMI160_ACC_ReadCalibration(obj, tmp);
	if (err)
		return -EINVAL;

	mul = obj->accel_reso->sensitivity / bmi160_acc_offset_resolution.sensitivity;
	len +=
		snprintf(buf + len, PAGE_SIZE - len,
			 "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,
			 obj->accel_offset[BMI160_ACC_AXIS_X], obj->accel_offset[BMI160_ACC_AXIS_Y],
			 obj->accel_offset[BMI160_ACC_AXIS_Z], obj->accel_offset[BMI160_ACC_AXIS_X],
			 obj->accel_offset[BMI160_ACC_AXIS_Y], obj->accel_offset[BMI160_ACC_AXIS_Z]);
	len +=
		snprintf(buf + len, PAGE_SIZE - len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1,
			 obj->accel_cali_sw[BMI160_ACC_AXIS_X], obj->accel_cali_sw[BMI160_ACC_AXIS_Y],
			 obj->accel_cali_sw[BMI160_ACC_AXIS_Z]);

	len +=
		snprintf(buf + len, PAGE_SIZE - len,
			 "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n",
			 obj->accel_offset[BMI160_ACC_AXIS_X] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_X],
			 obj->accel_offset[BMI160_ACC_AXIS_Y] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Y],
			 obj->accel_offset[BMI160_ACC_AXIS_Z] * mul + obj->accel_cali_sw[BMI160_ACC_AXIS_Z],
			 tmp[BMI160_ACC_AXIS_X], tmp[BMI160_ACC_AXIS_Y],
			 tmp[BMI160_ACC_AXIS_Z]);

	return len;

}

static ssize_t accel_store_cali_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int err, x, y, z;
	int dat[BMI160_ACC_AXES_NUM] = { 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (!strncmp(buf, "rst", 3)) {
		err = BMI160_ACC_ResetCalibration(obj);
		if (err)
			ACC_PR_ERR("reset accel_offset err = %d\n", err);
	} else if (sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z) == 3) {
		dat[BMI160_ACC_AXIS_X] = x;
		dat[BMI160_ACC_AXIS_Y] = y;
		dat[BMI160_ACC_AXIS_Z] = z;
		err = BMI160_ACC_WriteCalibration(obj, dat);
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
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	err = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->accel_trace));
	return err;
}

static ssize_t accel_store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->accel_trace, trace);
	else
		ACC_PR_ERR("invalid content: '%s'\n", buf);

	return count;
}

static ssize_t accel_show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d\n",
				obj->hw->direction);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}
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


static int bmi160_fifo_length(uint32_t *fifo_length)
{
	int comres = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	uint8_t a_data_u8r[2] = { 0, 0 };

	comres += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
	a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
	*fifo_length =
	    (uint32_t) (((uint32_t) ((uint8_t) (a_data_u8r[1]) << BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);

	return comres;
}

int bmi160_set_fifo_time_enable(u8 v_fifo_time_enable_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (v_fifo_time_enable_u8 <= 1) {
		/* write the fifo sensor time */
		com_rslt = BMP160_spi_read_bytes(obj->spi,
						 BMI160_USER_FIFO_TIME_ENABLE__REG, &v_data_u8, 1);
		if (com_rslt == 0) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_FIFO_TIME_ENABLE,
							v_fifo_time_enable_u8);
			com_rslt += BMP160_spi_write_bytes(obj->spi,
							   BMI160_USER_FIFO_TIME_ENABLE__REG,
							   &v_data_u8, 1);
		}
	} else {
		com_rslt = -2;
	}
	return com_rslt;
}

int bmi160_set_fifo_header_enable(u8 v_fifo_header_enable_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (v_fifo_header_enable_u8 <= 1) {
		/* read the fifo sensor header enable */
		com_rslt = BMP160_spi_read_bytes(obj->spi,
						 BMI160_USER_FIFO_HEADER_EN__REG, &v_data_u8, 1);
		if (com_rslt == 0) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_FIFO_HEADER_EN,
							v_fifo_header_enable_u8);
			com_rslt += BMP160_spi_write_bytes(obj->spi,
							   BMI160_USER_FIFO_HEADER_EN__REG,
							   &v_data_u8, 1);
		}
	} else {
		com_rslt = -2;
	}
	return com_rslt;
}

int bmi160_get_fifo_time_enable(u8 *v_fifo_time_enable_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	com_rslt = BMP160_spi_read_bytes(obj->spi,
					 BMI160_USER_FIFO_TIME_ENABLE__REG, &v_data_u8, 1);
	*v_fifo_time_enable_u8 = BMI160_GET_BITSLICE(v_data_u8, BMI160_USER_FIFO_TIME_ENABLE);
	return com_rslt;
}

static int bmi160_get_fifo_header_enable(u8 *v_fifo_header_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	com_rslt = BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_HEADER_EN__REG, &v_data_u8, 1);
	*v_fifo_header_u8 = BMI160_GET_BITSLICE(v_data_u8, BMI160_USER_FIFO_HEADER_EN);
	return com_rslt;
}


/*!
 *	@brief This API is used to read
 *	interrupt enable step detector interrupt from
 *	the register bit 0x52 bit 3
 *
 *	@param v_step_intr_u8 : The value of step detector interrupt enable
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 */
static BMI160_RETURN_FUNCTION_TYPE bmi160_get_std_enable(u8 *v_step_intr_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	/* read the step detector interrupt */
	com_rslt = p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						  BMI160_USER_INTR_ENABLE_2_STEP_DETECTOR_ENABLE__REG,
						  &v_data_u8,
						  BMI160_GEN_READ_WRITE_DATA_LENGTH);
	*v_step_intr_u8 =
		BMI160_GET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_2_STEP_DETECTOR_ENABLE);

	return com_rslt;
}

BMI160_RETURN_FUNCTION_TYPE bmi160_write_reg(u8 v_addr_u8, u8 *v_data_u8, u8 v_len_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	/* write data from register */
	com_rslt = p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, v_addr_u8, v_data_u8, v_len_u8);

	return com_rslt;
}

BMI160_RETURN_FUNCTION_TYPE bmi160_init(struct bmi160_t *bmi160)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	u8 v_pmu_data_u8 = BMI160_INIT_VALUE;
	/* assign bmi160 ptr */
	p_bmi160 = bmi160;
	com_rslt =
	    p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
					   BMI160_USER_CHIP_ID__REG,
					   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	/* read Chip Id */
	p_bmi160->chip_id = v_data_u8;
	/* To avoid gyro wakeup it is required to write 0x00 to 0x6C */
	com_rslt += bmi160_write_reg(BMI160_USER_PMU_TRIGGER_ADDR, &v_pmu_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	return com_rslt;
}


/*!
 *	@brief  Configure trigger condition of interrupt1
 *	and interrupt2 pin from the register 0x53
 *	@brief interrupt1 - bit 0
 *	@brief interrupt2 - bit 4
 *
 *  @param v_channel_u8: The value of edge trigger selection
 *   v_channel_u8  |   Edge trigger
 *  ---------------|---------------
 *       0         | BMI160_INTR1_EDGE_CTRL
 *       1         | BMI160_INTR2_EDGE_CTRL
 *
 *	@param v_intr_edge_ctrl_u8 : The value of edge trigger enable
 *	value    | interrupt enable
 * ----------|-------------------
 *  0x01     |  BMI160_EDGE
 *  0x00     |  BMI160_LEVEL
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_edge_ctrl(u8 v_channel_u8, u8 v_intr_edge_ctrl_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	switch (v_channel_u8) {
	case BMI160_INTR1_EDGE_CTRL:
		/* write the edge trigger interrupt1 */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR1_EDGE_CTRL__REG,
						   &v_data_u8,
						   BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR1_EDGE_CTRL, v_intr_edge_ctrl_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR1_EDGE_CTRL__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_INTR2_EDGE_CTRL:
		/* write the edge trigger interrupt2 */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_EDGE_CTRL__REG,
				&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR2_EDGE_CTRL, v_intr_edge_ctrl_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_EDGE_CTRL__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;
	}

	return com_rslt;
}

static int bmi160_set_command_register(u8 cmd_reg)
{
	int comres = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	comres += BMP160_spi_write_bytes(obj->spi, BMI160_CMD_COMMANDS__REG, &cmd_reg, 1);
	return comres;
}

static ssize_t accel_bmi160_fifo_bytecount_show(struct device_driver *ddri, char *buf)
{
	int comres = 0;
	uint32_t fifo_bytecount = 0;
	uint8_t a_data_u8r[2] = { 0, 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	comres +=
	    BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_BYTE_COUNTER_LSB__REG, a_data_u8r, 2);
	a_data_u8r[1] = BMI160_GET_BITSLICE(a_data_u8r[1], BMI160_USER_FIFO_BYTE_COUNTER_MSB);
	fifo_bytecount =
	    (uint32_t) (((uint32_t) ((uint8_t) (a_data_u8r[1]) << BMI160_SHIFT_8_POSITION)) | a_data_u8r[0]);
	comres = sprintf(buf, "%u\n", fifo_bytecount);
	return comres;
}

static ssize_t accel_bmi160_fifo_bytecount_store(struct device_driver *ddri, const char *buf,
					   size_t count)
{
	int err;
	unsigned long data;
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	client_data->fifo_bytecount = (u16) data;
	return count;
}

static int bmi160_fifo_data_sel_get(struct bmi160_accelgyro_data *client_data)
{
	int err = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	unsigned char data;
	unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;
	unsigned char fifo_datasel;


	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_ACC_EN__REG, &data, 1);
	fifo_acc_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_ACC_EN);

	err += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_GYRO_EN__REG, &data, 1);
	fifo_gyro_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_GYRO_EN);

	err += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_MAG_EN__REG, &data, 1);
	fifo_mag_en = BMI160_GET_BITSLICE(data, BMI160_USER_FIFO_MAG_EN);

	if (err)
		return err;

	fifo_datasel = (fifo_acc_en << BMI_ACC_SENSOR) |
	    (fifo_gyro_en << BMI_GYRO_SENSOR) | (fifo_mag_en << BMI_MAG_SENSOR);

	client_data->fifo_data_sel = fifo_datasel;

	return err;
}

static ssize_t accel_bmi160_fifo_data_sel_show(struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	err = bmi160_fifo_data_sel_get(client_data);
	if (err)
		return -EINVAL;
	return sprintf(buf, "%d\n", client_data->fifo_data_sel);
}

static ssize_t accel_bmi160_fifo_data_sel_store(struct device_driver *ddri, const char *buf, size_t count)
{
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	int err;
	unsigned long data;
	unsigned char fifo_datasel;
	unsigned char fifo_acc_en, fifo_gyro_en, fifo_mag_en;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	/* data format: aimed 0b0000 0x(m)x(g)x(a), x:1 enable, 0:disable */
	if (data > 7)
		return -EINVAL;

	fifo_datasel = (unsigned char)data;
	fifo_acc_en = fifo_datasel & (1 << BMI_ACC_SENSOR) ? 1 : 0;
	fifo_gyro_en = fifo_datasel & (1 << BMI_GYRO_SENSOR) ? 1 : 0;
	fifo_mag_en = fifo_datasel & (1 << BMI_MAG_SENSOR) ? 1 : 0;

	err += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_ACC_EN, fifo_acc_en);
	err += BMP160_spi_write_bytes(obj->spi, BMI160_USER_FIFO_ACC_EN__REG, &fifo_datasel, 1);
	udelay(500);
	err += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_GYRO_EN, fifo_gyro_en);
	err += BMP160_spi_write_bytes(obj->spi, BMI160_USER_FIFO_GYRO_EN__REG, &fifo_datasel, 1);
	udelay(500);
	err += BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);
	fifo_datasel = BMI160_SET_BITSLICE(fifo_datasel, BMI160_USER_FIFO_MAG_EN, fifo_mag_en);
	err += BMP160_spi_write_bytes(obj->spi, BMI160_USER_FIFO_MAG_EN__REG, &fifo_datasel, 1);
	udelay(500);
	if (err)
		return -EIO;

	client_data->fifo_data_sel = (u8) data;
	ACC_LOG("FIFO fifo_data_sel %d, A_en:%d, G_en:%d, M_en:%d\n",
		client_data->fifo_data_sel, fifo_acc_en, fifo_gyro_en, fifo_mag_en);
	return count;
}

static ssize_t accel_bmi160_fifo_data_out_frame_show(struct device_driver *ddri, char *buf)
{
	int err = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;
	uint32_t fifo_bytecount = 0;
	static char tmp_buf[1003] = { 0 };
	char *pBuf = NULL;

	pBuf = &tmp_buf[0];
	err = bmi160_fifo_length(&fifo_bytecount);
	if (err < 0) {
		ACC_PR_ERR("read fifo length error.\n");
		return -EINVAL;
	}
	if (fifo_bytecount == 0)
		return 0;
	client_data->fifo_bytecount = fifo_bytecount;
#ifdef FIFO_READ_USE_DMA_MODE_I2C
	err = i2c_dma_read(client, BMI160_USER_FIFO_DATA__REG, buf, client_data->fifo_bytecount);
#else
	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_DATA__REG, buf, fifo_bytecount);
#endif
	if (err < 0) {
		ACC_PR_ERR("read fifo data error.\n");
		return sprintf(buf, "Read byte block error.");
	}

	return client_data->fifo_bytecount;
}

static ssize_t accel_show_layout_value(struct device_driver *ddri, char *buf)
{
	struct bmi160_accelgyro_data *data = accelgyro_obj_data;

	return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n",
		       data->hw->direction, atomic_read(&data->layout), data->cvt.sign[0],
		       data->cvt.sign[1], data->cvt.sign[2], data->cvt.map[0], data->cvt.map[1],
		       data->cvt.map[2]);
}

static ssize_t accel_store_layout_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct bmi160_accelgyro_data *data = accelgyro_obj_data;
	int layout = 0;

	if (kstrtos32(buf, 10, &layout) == 0) {
		atomic_set(&data->layout, layout);
		if (!hwmsen_get_convert(layout, &data->cvt)) {
			ACC_PR_ERR("HWMSEN_GET_CONVERT function error!\r\n");
		} else if (!hwmsen_get_convert(data->hw->direction, &data->cvt)) {
			ACC_PR_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
		} else {
			ACC_PR_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
			hwmsen_get_convert(0, &data->cvt);
		}
	} else {
		ACC_PR_ERR("invalid format = '%s'\n", buf);
	}

	return count;
}


static void bmi_dump_reg(struct bmi160_accelgyro_data *client_data)
{
	int i;
	u8 dbg_buf0[REG_MAX0];
	u8 dbg_buf1[REG_MAX1];
	u8 dbg_buf_str0[REG_MAX0 * 3 + 1] = "";
	u8 dbg_buf_str1[REG_MAX1 * 3 + 1] = "";
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	ACC_LOG("\nFrom 0x00:\n");
	BMP160_spi_read_bytes(obj->spi, BMI160_USER_CHIP_ID__REG, dbg_buf0, REG_MAX0);
	for (i = 0; i < REG_MAX0; i++) {
		sprintf(dbg_buf_str0 + i * 3, "%02x%c", dbg_buf0[i],
			(((i + 1) % BYTES_PER_LINE == 0) ? '\n' : ' '));
	}
	ACC_LOG("%s\n", dbg_buf_str0);

	BMP160_spi_read_bytes(obj->spi, BMI160_USER_ACCEL_CONFIG_ADDR, dbg_buf1, REG_MAX1);
	ACC_LOG("\nFrom 0x40:\n");
	for (i = 0; i < REG_MAX1; i++) {
		sprintf(dbg_buf_str1 + i * 3, "%02x%c", dbg_buf1[i],
			(((i + 1) % BYTES_PER_LINE == 0) ? '\n' : ' '));
	}
	ACC_LOG("\n%s\n", dbg_buf_str1);
}

static ssize_t accel_bmi_register_show(struct device_driver *ddri, char *buf)
{
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	bmi_dump_reg(client_data);
	return sprintf(buf, "Dump OK\n");
}

static ssize_t accel_bmi_register_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	int reg_addr = 0;
	int data;
	u8 write_reg_add = 0;
	u8 write_data = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	err = sscanf(buf, "%3d %3d", &reg_addr, &data);
	if (err < 2)
		return err;

	if (data > 0xff)
		return -EINVAL;

	write_reg_add = (u8) reg_addr;
	write_data = (u8) data;
	err += BMP160_spi_write_bytes(obj->spi, write_reg_add, &write_data, 1);

	if (err) {
		ACC_PR_ERR("write reg fail\n");
		return err;
	}
	return count;
}

static ssize_t accel_bmi160_bmi_value_show(struct device_driver *ddri, char *buf)
{
	int err;
	u8 raw_data[12] = { 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	memset(raw_data, 0, sizeof(raw_data));
	err = BMP160_spi_read_bytes(obj->spi, BMI160_USER_DATA_8_GYRO_X_LSB__REG, raw_data, 12);
	if (err)
		return err;
	/* output:gyro x y z acc x y z */
	return sprintf(buf, "%hd %d %hd %hd %hd %hd\n",
		       (s16) (raw_data[1] << 8 | raw_data[0]),
		       (s16) (raw_data[3] << 8 | raw_data[2]),
		       (s16) (raw_data[5] << 8 | raw_data[4]),
		       (s16) (raw_data[7] << 8 | raw_data[6]),
		       (s16) (raw_data[9] << 8 | raw_data[8]),
		       (s16) (raw_data[11] << 8 | raw_data[10]));
}

static int bmi160_get_fifo_wm(u8 *v_fifo_wm_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	/* check the p_bmi160 structure as NULL */
	/* read the fifo water mark level */
	com_rslt = BMP160_spi_read_bytes(obj->spi, BMI160_USER_FIFO_WM__REG, &v_data_u8, 1);
	*v_fifo_wm_u8 = BMI160_GET_BITSLICE(v_data_u8, BMI160_USER_FIFO_WM);
	return com_rslt;
}

static int bmi160_set_fifo_wm(u8 v_fifo_wm_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	/* write the fifo water mark level */
	com_rslt = BMP160_spi_write_bytes(obj->spi, BMI160_USER_FIFO_WM__REG, &v_fifo_wm_u8, 1);
	return com_rslt;
}

static ssize_t accel_bmi160_fifo_watermark_show(struct device_driver *ddri, char *buf)
{
	int err;
	u8 data = 0xff;

	err = bmi160_get_fifo_wm(&data);
	if (err)
		return err;
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_bmi160_fifo_watermark_store(struct device_driver *ddri,
					   const char *buf, size_t count)
{
	int err;
	unsigned long data;
	u8 fifo_watermark;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;

	fifo_watermark = (u8) data;
	err = bmi160_set_fifo_wm(fifo_watermark);
	if (err)
		return -EIO;

	ACC_LOG("set fifo watermark = %d ok.", (int)fifo_watermark);
	return count;
}

static ssize_t accel_bmi160_delay_show(struct device_driver *ddri, char *buf)
{
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	return sprintf(buf, "%d\n", atomic_read(&client_data->accel_delay));
}

static ssize_t accel_bmi160_delay_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long data;
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	if (data < BMI_DELAY_MIN)
		data = BMI_DELAY_MIN;

	atomic_set(&client_data->accel_delay, (unsigned int)data);
	return count;
}

static ssize_t accel_bmi160_fifo_flush_store(struct device_driver *ddri, const char *buf, size_t count)
{
	int err;
	unsigned long enable;

	err = kstrtoul(buf, 10, &enable);
	if (err)
		return err;
	if (enable)
		err = bmi160_set_command_register(CMD_CLR_FIFO_DATA);
	if (err)
		ACC_PR_ERR("fifo flush failed!\n");
	return count;
}

static ssize_t accel_bmi160_fifo_header_en_show(struct device_driver *ddri, char *buf)
{
	int err;
	u8 data = 0xff;

	err = bmi160_get_fifo_header_enable(&data);
	if (err)
		return err;
	return sprintf(buf, "%d\n", data);
}

static ssize_t accel_bmi160_fifo_header_en_store(struct device_driver *ddri,
					   const char *buf, size_t count)
{
	int err;
	unsigned long data;
	u8 fifo_header_en;
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	err = kstrtoul(buf, 10, &data);
	if (err)
		return err;
	if (data > 1)
		return -ENOENT;
	fifo_header_en = (u8) data;
	err = bmi160_set_fifo_header_enable(fifo_header_en);
	if (err)
		return -EIO;
	client_data->fifo_head_en = fifo_header_en;
	return count;
}

static int bmg_read_raw_data(struct bmi160_accelgyro_data *obj, s16 data[BMG_AXES_NUM])
{
	int err = 0;
	struct bmi160_accelgyro_data *priv = gyro_obj_data;

	if (priv->gyro_power_mode == BMG_SUSPEND_MODE) {
		err = bmg_set_powermode(NULL, (enum BMG_POWERMODE_ENUM)BMG_NORMAL_MODE);
		if (err < 0) {
			GYRO_PR_ERR("set power mode failed, err = %d\n", err);
			return err;
		}
	}
	if (priv->gyro_sensor_type == BMI160_GYRO_TYPE) {
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
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_X] = data[BMG_AXIS_X];
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_Y] = data[BMG_AXIS_Y];
				priv->gyro_fir.raw[priv->gyro_fir.num][BMG_AXIS_Z] = data[BMG_AXIS_Z];
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
				priv->gyro_fir.sum[BMG_AXIS_X] -= priv->gyro_fir.raw[idx][BMG_AXIS_X];
				priv->gyro_fir.sum[BMG_AXIS_Y] -= priv->gyro_fir.raw[idx][BMG_AXIS_Y];
				priv->gyro_fir.sum[BMG_AXIS_Z] -= priv->gyro_fir.raw[idx][BMG_AXIS_Z];
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
					GYRO_LOG("add [%2d][%5d %5d %5d] =>[%5d %5d %5d] : [%5d %5d %5d]\n",
						 idx,
						 priv->gyro_fir.raw[idx][BMG_AXIS_X],
						 priv->gyro_fir.raw[idx][BMG_AXIS_Y],
						 priv->gyro_fir.raw[idx][BMG_AXIS_Z],
						 priv->gyro_fir.sum[BMG_AXIS_X],
						 priv->gyro_fir.sum[BMG_AXIS_Y],
						 priv->gyro_fir.sum[BMG_AXIS_Z],
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
static int bmg_get_hw_offset(struct bmi160_accelgyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
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
static int bmg_set_hw_offset(struct bmi160_accelgyro_data *obj, s8 offset[BMG_AXES_NUM + 1])
{
	int err = 0;
	/* HW calibration is under construction */
	GYRO_LOG("hw offset x=%x, y=%x, z=%x\n",
		 offset[BMG_AXIS_X], offset[BMG_AXIS_Y], offset[BMG_AXIS_Z]);
	return err;
}
#endif

static int bmg_reset_calibration(struct bmi160_accelgyro_data *obj)
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

static int bmg_read_calibration(struct bmi160_accelgyro_data *obj,
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

static int bmg_write_calibration(struct bmi160_accelgyro_data *obj, int dat[BMG_AXES_NUM])
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
	obj->gyro_cali_sw[BMG_AXIS_X] = obj->cvt.sign[BMG_AXIS_X] * (cali[obj->cvt.map[BMG_AXIS_X]]);
	obj->gyro_cali_sw[BMG_AXIS_Y] = obj->cvt.sign[BMG_AXIS_Y] * (cali[obj->cvt.map[BMG_AXIS_Y]]);
	obj->gyro_cali_sw[BMG_AXIS_Z] = obj->cvt.sign[BMG_AXIS_Z] * (cali[obj->cvt.map[BMG_AXIS_Z]]);
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
static int bmg_get_chip_type(struct bmi160_accelgyro_data *obj)
{
	int err = 0;
	u8 chip_id = 0;

	obj = gyro_obj_data;
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
		obj->gyro_sensor_type = BMI160_GYRO_TYPE;
		strlcpy(obj->gyro_sensor_name, BMG_DEV_NAME, sizeof(obj->gyro_sensor_name));
		break;
	default:
		obj->gyro_sensor_type = INVALID_TYPE;
		strlcpy(obj->gyro_sensor_name, UNKNOWN_DEV, sizeof(obj->gyro_sensor_name));
		break;
	}
	if (obj->gyro_sensor_type == INVALID_TYPE) {
		GYRO_PR_ERR("unknown gyroscope.\n");
		return -1;
	}
	return err;
}

static int bmg_set_powermode(struct bmi160_accelgyro_data *obj, enum BMG_POWERMODE_ENUM power_mode)
{
	int err = 0;
	u8 data = 0;
	u8 actual_power_mode = 0;

	obj = gyro_obj_data;
	if (power_mode == obj->gyro_power_mode)
		return 0;

	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == BMI160_GYRO_TYPE) {
		if (power_mode == BMG_SUSPEND_MODE) {
			actual_power_mode = CMD_PMU_GYRO_SUSPEND;
		} else if (power_mode == BMG_NORMAL_MODE) {
			actual_power_mode = CMD_PMU_GYRO_NORMAL;
		} else {
			err = -EINVAL;
			GYRO_PR_ERR("invalid power mode = %d\n", power_mode);
			mutex_unlock(&obj->gyro_lock);
			return err;
		}
		data = actual_power_mode;
		err += BMP160_spi_write_bytes(NULL, BMI160_CMD_COMMANDS__REG, &data, 1);
		mdelay(55);
	}
	if (err < 0) {
		GYRO_PR_ERR("set power mode failed, err = %d, sensor name = %s\n",
			 err, obj->gyro_sensor_name);
	} else {
		obj->gyro_power_mode = power_mode;
	}
	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set power mode = %d ok.\n", (int)data);
	return err;
}

static int bmg_set_range(struct bmi160_accelgyro_data *obj, enum BMG_RANGE_ENUM range)
{
	u8 err = 0;
	u8 data = 0;
	u8 actual_range = 0;

	obj = gyro_obj_data;
	if (range == obj->gyro_range)
		return 0;

	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == BMI160_GYRO_TYPE) {
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
			mutex_unlock(&obj->gyro_lock);
			return err;
		}
		err = BMP160_spi_read_bytes(NULL, BMI160_USER_GYR_RANGE__REG, &data, 1);
		data = BMG_SET_BITSLICE(data, BMI160_USER_GYR_RANGE, actual_range);
		err += BMP160_spi_write_bytes(NULL, BMI160_USER_GYR_RANGE__REG, &data, 1);
		mdelay(1);
		if (err < 0) {
			GYRO_PR_ERR("set range failed.\n");
		} else {
			obj->gyro_range = range;
			/* bitnum: 16bit */
			switch (range) {
			case BMG_RANGE_2000:
				obj->gyro_sensitivity = BMI160_FS_2000_LSB;	/* 16; */
				break;
			case BMG_RANGE_1000:
				obj->gyro_sensitivity = BMI160_FS_1000_LSB;	/* 33; */
				break;
			case BMG_RANGE_500:
				obj->gyro_sensitivity = BMI160_FS_500_LSB;	/* 66; */
				break;
			case BMG_RANGE_250:
				obj->gyro_sensitivity = BMI160_FS_250_LSB;	/* 131; */
				break;
			case BMG_RANGE_125:
				obj->gyro_sensitivity = BMI160_FS_125_LSB;	/* 262; */
				break;
			default:
				obj->gyro_sensitivity = BMI160_FS_2000_LSB;	/* 16; */
				break;
			}
		}
	}
	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set range ok.\n");
	return err;
}

static int bmg_set_datarate(struct bmi160_accelgyro_data *obj, int datarate)
{
	int err = 0;
	u8 data = 0;

	obj = gyro_obj_data;
	if (datarate == obj->gyro_datarate) {
		GYRO_LOG("set new data rate = %d, old data rate = %d\n", datarate, obj->gyro_datarate);
		return 0;
	}
	mutex_lock(&obj->gyro_lock);
	if (obj->gyro_sensor_type == BMI160_GYRO_TYPE) {
		err = BMP160_spi_read_bytes(NULL, BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
		data = BMG_SET_BITSLICE(data, BMI160_USER_GYR_CONF_ODR, datarate);
		err += BMP160_spi_write_bytes(NULL, BMI160_USER_GYR_CONF_ODR__REG, &data, 1);
	}
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
	else
		obj->gyro_datarate = datarate;

	mutex_unlock(&obj->gyro_lock);
	GYRO_LOG("set data rate = %d ok.\n", datarate);
	return err;
}

static int bmg_init_client(struct bmi160_accelgyro_data *obj, int reset_cali)
{
#ifdef CONFIG_BMG_LOWPASS
	struct bmi160_accelgyro_data *obj = (struct bmi160_accelgyro_data *)gyro_obj_data;
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
	memset(&obj->gyro_fir, 0x00, sizeof(obj->gyro_fir));
#endif
	return 0;
}

/*
*Returns compensated and mapped value. unit is :degree/second
*/
static int bmg_read_sensor_data(struct bmi160_accelgyro_data *obj, char *buf, int bufsize)
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
	/* compensate data */
	databuf[BMG_AXIS_X] += obj->gyro_cali_sw[BMG_AXIS_X];
	databuf[BMG_AXIS_Y] += obj->gyro_cali_sw[BMG_AXIS_Y];
	databuf[BMG_AXIS_Z] += obj->gyro_cali_sw[BMG_AXIS_Z];

	/* remap coordinate */
	gyro[obj->cvt.map[BMG_AXIS_X]] = obj->cvt.sign[BMG_AXIS_X] * databuf[BMG_AXIS_X];
	gyro[obj->cvt.map[BMG_AXIS_Y]] = obj->cvt.sign[BMG_AXIS_Y] * databuf[BMG_AXIS_Y];
	gyro[obj->cvt.map[BMG_AXIS_Z]] = obj->cvt.sign[BMG_AXIS_Z] * databuf[BMG_AXIS_Z];

	/* convert: LSB -> degree/second(o/s) */
	gyro[BMG_AXIS_X] = gyro[BMG_AXIS_X] * BMI160_FS_250_LSB / obj->gyro_sensitivity;
	gyro[BMG_AXIS_Y] = gyro[BMG_AXIS_Y] * BMI160_FS_250_LSB / obj->gyro_sensitivity;
	gyro[BMG_AXIS_Z] = gyro[BMG_AXIS_Z] * BMI160_FS_250_LSB / obj->gyro_sensitivity;

	sprintf(buf, "%04x %04x %04x",
		gyro[BMG_AXIS_X], gyro[BMG_AXIS_Y], gyro[BMG_AXIS_Z]);
	if (atomic_read(&obj->gyro_trace) & GYRO_TRC_IOCTL)
		GYRO_LOG("gyroscope data: %s\n", buf);
	return 0;
}

static ssize_t gyro_show_chipinfo_value(struct device_driver *ddri, char *buf)
{
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	err = bmg_read_calibration(NULL, act, raw);
	if (err)
		return -EINVAL;

	mul = 1;	/* mul = sensor sensitivity / offset sensitivity */
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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	if (atomic_read(&obj->gyro_firlen)) {
		int idx, len = atomic_read(&obj->gyro_firlen);

		GYRO_LOG("len = %2d, idx = %2d\n", obj->gyro_fir.num, obj->gyro_fir.idx);

		for (idx = 0; idx < len; idx++) {
			GYRO_LOG("[%5d %5d %5d]\n",
				 obj->gyro_fir.raw[idx][BMG_AXIS_X],
				 obj->gyro_fir.raw[idx][BMG_AXIS_Y], obj->gyro_fir.raw[idx][BMG_AXIS_Z]);
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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;
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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->gyro_trace));
	return res;
}

static ssize_t gyro_store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	if (kstrtos32(buf, 10, &trace) == 0)
		atomic_set(&obj->gyro_trace, trace);
	else
		GYRO_INFO("invalid content: '%s'\n", buf);
	return count;
}

static ssize_t gyro_show_status_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	if (obj->hw)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"CUST: %d\n",
				obj->hw->direction);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");

	len += snprintf(buf + len, PAGE_SIZE - len, "ver:%s\n", BMG_DRIVER_VERSION);
	return len;
}

static ssize_t gyro_show_power_mode_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

	len += snprintf(buf + len, PAGE_SIZE - len, "%s mode\n",
			obj->gyro_power_mode == BMG_NORMAL_MODE ? "normal" : "suspend");
	return len;
}

static ssize_t gyro_store_power_mode_value(struct device_driver *ddri, const char *buf, size_t count)
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

static ssize_t gyro_show_range_value(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

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
	struct bmi160_accelgyro_data *obj = gyro_obj_data;

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
static DRIVER_ATTR(acc_op_mode, S_IWUSR | S_IRUGO, accel_show_acc_op_mode_value, accel_store_acc_op_mode_value);
static DRIVER_ATTR(acc_range, S_IWUSR | S_IRUGO, accel_show_acc_range_value, accel_store_acc_range_value);
static DRIVER_ATTR(acc_odr, S_IWUSR | S_IRUGO, accel_show_acc_odr_value, accel_store_acc_odr_value);
static DRIVER_ATTR(acc_sensordata, S_IWUSR | S_IRUGO, accel_show_sensordata_value, NULL);
static DRIVER_ATTR(acc_cali, S_IWUSR | S_IRUGO, accel_show_cali_value, accel_store_cali_value);
static DRIVER_ATTR(acc_firlen, S_IWUSR | S_IRUGO, accel_show_firlen_value, accel_store_firlen_value);
static DRIVER_ATTR(acc_trace, S_IWUSR | S_IRUGO, accel_show_trace_value, accel_store_trace_value);
static DRIVER_ATTR(acc_status, S_IRUGO, accel_show_status_value, NULL);
static DRIVER_ATTR(acc_powerstatus, S_IRUGO, accel_show_power_status_value, NULL);

static DRIVER_ATTR(acc_fifo_bytecount, S_IRUGO | S_IWUSR, accel_bmi160_fifo_bytecount_show,
		   accel_bmi160_fifo_bytecount_store);
static DRIVER_ATTR(acc_fifo_data_sel, S_IRUGO | S_IWUSR, accel_bmi160_fifo_data_sel_show,
		   accel_bmi160_fifo_data_sel_store);
static DRIVER_ATTR(acc_fifo_data_frame, S_IRUGO, accel_bmi160_fifo_data_out_frame_show, NULL);
static DRIVER_ATTR(acc_layout, S_IRUGO | S_IWUSR, accel_show_layout_value, accel_store_layout_value);
static DRIVER_ATTR(acc_register, S_IRUGO | S_IWUSR, accel_bmi_register_show, accel_bmi_register_store);
static DRIVER_ATTR(acc_bmi_value, S_IRUGO, accel_bmi160_bmi_value_show, NULL);
static DRIVER_ATTR(acc_fifo_watermark, S_IRUGO | S_IWUSR, accel_bmi160_fifo_watermark_show,
		   accel_bmi160_fifo_watermark_store);
static DRIVER_ATTR(acc_delay, S_IRUGO | S_IWUSR, accel_bmi160_delay_show, accel_bmi160_delay_store);
static DRIVER_ATTR(acc_fifo_flush, S_IWUSR | S_IRUGO, NULL, accel_bmi160_fifo_flush_store);
static DRIVER_ATTR(acc_fifo_header_en, S_IWUSR | S_IRUGO, accel_bmi160_fifo_header_en_show,
		   accel_bmi160_fifo_header_en_store);
static DRIVER_ATTR(gyro_chipinfo, S_IWUSR | S_IRUGO, gyro_show_chipinfo_value, NULL);
static DRIVER_ATTR(gyro_sensordata, S_IWUSR | S_IRUGO, gyro_show_sensordata_value, NULL);
static DRIVER_ATTR(gyro_rawdata, S_IWUSR | S_IRUGO, gyro_show_rawdata_value, NULL);
static DRIVER_ATTR(gyro_cali, S_IWUSR | S_IRUGO, gyro_show_cali_value, gyro_store_cali_value);
static DRIVER_ATTR(gyro_firlen, S_IWUSR | S_IRUGO, gyro_show_firlen_value, gyro_store_firlen_value);
static DRIVER_ATTR(gyro_trace, S_IWUSR | S_IRUGO, gyro_show_trace_value, gyro_store_trace_value);
static DRIVER_ATTR(gyro_status, S_IRUGO, gyro_show_status_value, NULL);
static DRIVER_ATTR(gyro_op_mode, S_IWUSR | S_IRUGO, gyro_show_power_mode_value, gyro_store_power_mode_value);
static DRIVER_ATTR(gyro_range, S_IWUSR | S_IRUGO, gyro_show_range_value, gyro_store_range_value);
static DRIVER_ATTR(gyro_odr, S_IWUSR | S_IRUGO, gyro_show_datarate_value, gyro_store_datarate_value);

static struct driver_attribute *bmi160_accelgyro_attr_list[] = {
	&driver_attr_acc_chipinfo,	/*chip information */
	&driver_attr_acc_sensordata,	/*dump sensor data */
	&driver_attr_acc_cali,	/*show calibration data */
	&driver_attr_acc_firlen,	/*filter length: 0: disable, others: enable */
	&driver_attr_acc_trace,	/*trace log */
	&driver_attr_acc_status,
	&driver_attr_acc_powerstatus,
	&driver_attr_acc_cpsdata,	/*g sensor data for compass tilt compensation */
	&driver_attr_acc_op_mode,	/*g sensor opmode for compass tilt compensation */
	&driver_attr_acc_range,	/*g sensor range for compass tilt compensation */
	&driver_attr_acc_odr,	/*g sensor bandwidth for compass tilt compensation */

	&driver_attr_acc_fifo_bytecount,
	&driver_attr_acc_fifo_data_sel,
	&driver_attr_acc_fifo_data_frame,
	&driver_attr_acc_layout,
	&driver_attr_acc_register,
	&driver_attr_acc_bmi_value,
	&driver_attr_acc_fifo_watermark,
	&driver_attr_acc_delay,
	&driver_attr_acc_fifo_flush,
	&driver_attr_acc_fifo_header_en,

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

static int bmi160_accelgyro_create_attr(struct device_driver *driver)
{
	int err = 0;
	int idx = 0;
	int num = ARRAY_SIZE(bmi160_accelgyro_attr_list);

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, bmi160_accelgyro_attr_list[idx]);
		if (err) {
			ACC_PR_ERR("create driver file (%s) failed.\n",
				bmi160_accelgyro_attr_list[idx]->attr.name);
			break;
		}
	}
	return err;
}

static int bmi160_accelgyro_delete_attr(struct device_driver *driver)
{
	int idx = 0;
	int err = 0;
	int num = ARRAY_SIZE(bmi160_accelgyro_attr_list);

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, bmi160_accelgyro_attr_list[idx]);
	return err;
}

#ifdef CONFIG_PM
static int bmi160_acc_suspend(void)
{
#if 0
	int err = 0;
	u8 op_mode = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	if (atomic_read(&obj->wkqueue_en) == 1) {
		BMI160_ACC_SetPowerMode(obj, false);
		cancel_delayed_work_sync(&obj->work);
	}
	err = bmi160_acc_get_mode(obj, &op_mode);

	atomic_set(&obj->accel_suspend, 1);
	if (op_mode != BMI160_ACC_MODE_SUSPEND && sig_flag != 1)
		err = BMI160_ACC_SetPowerMode(obj, false);
	if (err) {
		ACC_PR_ERR("write power control failed.\n");
		return err;
	}
	BMI160_ACC_power(obj->hw, 0);

	return err;
#else
	return 0;
#endif
}

static int bmi160_acc_resume(void)
{
#if 0
	int err;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	BMI160_ACC_power(obj->hw, 1);
	err = bmi160_acc_init_client(obj, 0);
	if (err) {
		ACC_PR_ERR("initialize client fail!!\n");
		return err;
	}
	atomic_set(&obj->accel_suspend, 0);
	return 0;
#else
	return 0;
#endif
}


#ifdef CONFIG_PM
static int bmg_suspend(void)
{
#if 0
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
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
#else
	return 0;
#endif
}

static int bmg_resume(void)
{
#if 0
	int err;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	err = bmg_init_client(obj, 0);
	if (err) {
		GYRO_PR_ERR("initialize client failed, err = %d\n", err);
		return err;
	}
	atomic_set(&obj->suspend, 0);
	return 0;
#else
	return 0;
#endif
}

static int pm_event_handler(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		bmi160_acc_suspend();
		bmg_suspend();
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:
		bmi160_acc_resume();
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


static struct spi_driver bmi160_accelgyro_spi_driver = {
	.driver = {
		   .name = BMI160_ACCELGYRO_DEV_NAME,
		   .bus = &spi_bus_type,
#ifdef CONFIG_OF
		   .of_match_table = accelgyrosensor_of_match,
#endif
		   },
	.probe = bmi160_accelgyro_spi_probe,
	.remove = bmi160_accelgyro_spi_remove,
};


/*!
 * @brief If use this type of enable, Gsensor only enabled but not report inputEvent to HAL
 *
 * @param[in] int enable true or false
 *
 * @return zero success, non-zero failed
 */
static int bmi160_acc_enable_nodata(int en)
{
	int err = 0;
	bool power = false;

	if (en == 1)
		power = true;
	else
		power = false;

	err = BMI160_ACC_SetPowerMode(accelgyro_obj_data, power);
	if (err < 0) {
		ACC_PR_ERR("BMI160_ACC_SetPowerMode failed.\n");
		return -1;
	}
	ACC_LOG("bmi160_acc_enable_nodata ok!\n");
	return err;
}

/*!
 * @brief set ODR rate value for acc
 *
 * @param[in] u64 ns for sensor delay
 *
 * @return zero success, non-zero failed
 */

static int bmi160_acc_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	int value = 0;
	int sample_delay = 0;
	int err = 0;

	value = (int)samplingPeriodNs/1000/1000;
	if (value <= 5)
		sample_delay = BMI160_ACCEL_ODR_400HZ;
	else if (value <= 10)
		sample_delay = BMI160_ACCEL_ODR_200HZ;
	else
		sample_delay = BMI160_ACCEL_ODR_100HZ;
	err = BMI160_ACC_SetBWRate(accelgyro_obj_data, sample_delay);
	if (err < 0) {
		ACC_PR_ERR("set delay parameter error!\n");
		return -1;
	}
	ACC_LOG("bmi160 acc set delay = (%d) ok.\n", value);
	return 0;
}

static int bmi160_acc_flush(void)
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
static int bmi160_acc_get_data(int *x, int *y, int *z, int *status)
{
	int err = 0;
	char buff[BMI160_BUFSIZE];
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	err = BMI160_ACC_ReadSensorData(obj, buff, BMI160_BUFSIZE);
	if (err < 0) {
		ACC_PR_ERR("bmi160_acc_get_data failed.\n");
		return err;
	}
	err = sscanf(buff, "%x %x %x", x, y, z);
	if (err < 0) {
		ACC_PR_ERR("bmi160_acc_get_data failed.\n");
		return err;
	}
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	return 0;
}

int gyroscope_operate(void *self, uint32_t command, void *buff_in, int size_in,
		      void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value, sample_delay;
	char buff[BMG_BUFSIZE] = { 0 };
	struct bmi160_accelgyro_data *priv = (struct bmi160_accelgyro_data *)self;
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

#if 0
static int bmg_open(struct inode *inode, struct file *file)
{
	file->private_data = accelgyro_obj_data;
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
	struct bmi160_accelgyro_data *obj = (struct bmi160_accelgyro_data *)file->private_data;

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
	struct bmi160_accelgyro_data *priv = gyro_obj_data;

	err = bmg_set_datarate(NULL, sample_delay);
	if (err < 0)
		GYRO_PR_ERR("set data rate failed.\n");
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
	struct bmi160_accelgyro_data *priv = accelgyro_obj_data;

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

static int bmi160_accel_factory_enable_sensor(bool enabledisable, int64_t sample_periods_ms)
{
	int err;

	err = bmi160_acc_enable_nodata(enabledisable == true ? 1 : 0);
	if (err) {
		ACC_PR_ERR("%s enable sensor failed!\n", __func__);
		return -1;
	}
	err = bmi160_acc_batch(0, sample_periods_ms * 1000000, 0);
	if (err) {
		ACC_PR_ERR("%s enable set batch failed!\n", __func__);
		return -1;
	}
	return 0;
}
static int bmi160_accel_factory_get_data(int32_t data[3], int *status)
{
	return bmi160_acc_get_data(&data[0], &data[1], &data[2], status);
}
static int bmi160_accel_factory_get_raw_data(int32_t data[3])
{
	char strbuf[BMI160_BUFSIZE] = { 0 };

	BMI160_ACC_ReadRawData(accelgyro_obj_data, strbuf);
	ACC_PR_ERR("don't support bmi160_factory_get_raw_data!\n");
	return 0;
}
static int bmi160_accel_factory_enable_calibration(void)
{
	return 0;
}
static int bmi160_accel_factory_clear_cali(void)
{
	int err = 0;

	err = BMI160_ACC_ResetCalibration(accelgyro_obj_data);
	if (err) {
		ACC_PR_ERR("bmi160_ResetCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int bmi160_accel_factory_set_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	cali[BMI160_ACC_AXIS_X] = data[0]
		* accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	cali[BMI160_ACC_AXIS_Y] = data[1]
		* accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	cali[BMI160_ACC_AXIS_Z] = data[2]
		* accelgyro_obj_data->accel_reso->sensitivity / GRAVITY_EARTH_1000;
	err = BMI160_ACC_WriteCalibration(accelgyro_obj_data, cali);
	if (err) {
		ACC_PR_ERR("bmi160_WriteCalibration failed!\n");
		return -1;
	}
	return 0;
}
static int bmi160_accel_factory_get_cali(int32_t data[3])
{
	int err = 0;
	int cali[3] = { 0 };

	err = BMI160_ACC_ReadCalibration(accelgyro_obj_data, cali);
	if (err) {
		ACC_PR_ERR("bmi160_ReadCalibration failed!\n");
		return -1;
	}
	data[0] = cali[BMI160_ACC_AXIS_X]
		* GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	data[1] = cali[BMI160_ACC_AXIS_Y]
		* GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	data[2] = cali[BMI160_ACC_AXIS_Z]
		* GRAVITY_EARTH_1000 / accelgyro_obj_data->accel_reso->sensitivity;
	return 0;
}
static int bmi160_accel_factory_do_self_test(void)
{
	return 0;
}

static struct accel_factory_fops bmi160_accel_factory_fops = {
	.enable_sensor = bmi160_accel_factory_enable_sensor,
	.get_data = bmi160_accel_factory_get_data,
	.get_raw_data = bmi160_accel_factory_get_raw_data,
	.enable_calibration = bmi160_accel_factory_enable_calibration,
	.clear_cali = bmi160_accel_factory_clear_cali,
	.set_cali = bmi160_accel_factory_set_cali,
	.get_cali = bmi160_accel_factory_get_cali,
	.do_self_test = bmi160_accel_factory_do_self_test,
};

static struct accel_factory_public bmi160_accel_factory_device = {
	.gain = 1,
	.sensitivity = 1,
	.fops = &bmi160_accel_factory_fops,
};


int bmi160_set_intr_fifo_wm(u8 v_channel_u8, u8 v_intr_fifo_wm_u8)
{
	/* variable used for return the status of communication result */
	int com_rslt = -1;
	u8 v_data_u8 = 0;
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;

	switch (v_channel_u8) {
		/* write the fifo water mark interrupt */
	case BMI160_INTR1_MAP_FIFO_WM:
		com_rslt = BMP160_spi_read_bytes(obj->spi,
						 BMI160_USER_INTR_MAP_1_INTR1_FIFO_WM__REG,
						 &v_data_u8, 1);
		if (com_rslt == 0) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR_MAP_1_INTR1_FIFO_WM,
							v_intr_fifo_wm_u8);
			com_rslt += BMP160_spi_write_bytes(obj->spi,
							   BMI160_USER_INTR_MAP_1_INTR1_FIFO_WM__REG,
							   &v_data_u8, 1);
		}
		break;
	case BMI160_INTR2_MAP_FIFO_WM:
		com_rslt = BMP160_spi_read_bytes(obj->spi,
						 BMI160_USER_INTR_MAP_1_INTR2_FIFO_WM__REG,
						 &v_data_u8, 1);
		if (com_rslt == 0) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR_MAP_1_INTR2_FIFO_WM,
							v_intr_fifo_wm_u8);
			com_rslt += BMP160_spi_write_bytes(obj->spi,
							   BMI160_USER_INTR_MAP_1_INTR2_FIFO_WM__REG,
							   &v_data_u8, 1);
		}
		break;
	default:
		com_rslt = -2;
		break;
	}
	return com_rslt;
}

/*!
 *	@brief  API used for set the Configure level condition of interrupt1
 *	and interrupt2 pin form the register 0x53
 *	@brief interrupt1 - bit 1
 *	@brief interrupt2 - bit 5
 *
 *  @param v_channel_u8: The value of level condition selection
 *   v_channel_u8  |   level selection
 *  ---------------|---------------
 *       0         | BMI160_INTR1_LEVEL
 *       1         | BMI160_INTR2_LEVEL
 *
 *	@param v_intr_level_u8 : The value of level of interrupt enable
 *	value    | Behaviour
 * ----------|-------------------
 *  0x01     |  BMI160_LEVEL_HIGH
 *  0x00     |  BMI160_LEVEL_LOW
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_level(u8 v_channel_u8, u8 v_intr_level_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	switch (v_channel_u8) {
	case BMI160_INTR1_LEVEL:
		/* write the interrupt1 level */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR1_LEVEL__REG,
				&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR1_LEVEL, v_intr_level_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR1_LEVEL__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_INTR2_LEVEL:
		/* write the interrupt2 level */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_LEVEL__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR2_LEVEL, v_intr_level_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_LEVEL__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;
	}

	return com_rslt;
}

/*!
*	@brief API used to set the Output enable for interrupt1
*	and interrupt1 pin from the register 0x53
*	@brief interrupt1 - bit 3
*	@brief interrupt2 - bit 7
*
*  @param v_channel_u8: The value of output enable selection
*   v_channel_u8  |   level selection
*  ---------------|---------------
*       0         | BMI160_INTR1_OUTPUT_TYPE
*       1         | BMI160_INTR2_OUTPUT_TYPE
*
*	@param v_output_enable_u8 :
*	The value of output enable of interrupt enable
*	value    | Behaviour
* ----------|-------------------
*  0x01     |  BMI160_INPUT
*  0x00     |  BMI160_OUTPUT
*
*	@return results of bus communication function
*	@retval 0 -> Success
*	@retval -1 -> Error
*
*/
BMI160_RETURN_FUNCTION_TYPE bmi160_set_output_enable(u8 v_channel_u8, u8 v_output_enable_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	switch (v_channel_u8) {
	case BMI160_INTR1_OUTPUT_ENABLE:
		/* write the output enable of interrupt1 */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR1_OUTPUT_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR1_OUTPUT_ENABLE, v_output_enable_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR1_OUTPUT_ENABLE__REG,
						&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_INTR2_OUTPUT_ENABLE:
		/* write the output enable of interrupt2 */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_OUTPUT_EN__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR2_OUTPUT_EN, v_output_enable_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr, BMI160_USER_INTR2_OUTPUT_EN__REG,
					&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;
	}

	return com_rslt;
}

 /*!
  *     @brief This API is used to set
  *     interrupt enable step detector interrupt from
  *     the register bit 0x52 bit 3
  *
  *     @param v_step_intr_u8 : The value of step detector interrupt enable
  *
  *     @return results of bus communication function
  *     @retval 0 -> Success
  *     @retval -1 -> Error
  *
  */
static BMI160_RETURN_FUNCTION_TYPE bmi160_set_std_enable(u8 v_step_intr_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	com_rslt = p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						  BMI160_USER_INTR_ENABLE_2_STEP_DETECTOR_ENABLE__REG,
						  &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	if (com_rslt == SUCCESS) {
		v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
						BMI160_USER_INTR_ENABLE_2_STEP_DETECTOR_ENABLE,
						v_step_intr_u8);
		com_rslt +=
			p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
							BMI160_USER_INTR_ENABLE_2_STEP_DETECTOR_ENABLE__REG,
							&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
	}

	return com_rslt;
}

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
BMI160_RETURN_FUNCTION_TYPE bmi160_read_reg(u8 v_addr_u8, u8 *v_data_u8, u8 v_len_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	/* Read data from register */
	com_rslt =
		p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
					   v_addr_u8, v_data_u8, v_len_u8);

	return com_rslt;
}

/*!
 *	@brief This API used to trigger the step detector
 *	interrupt
 *
 *  @param  v_step_detector_u8 : The value of interrupt selection
 *  value    |  interrupt
 * ----------|-----------
 *   0       |  BMI160_MAP_INTR1
 *   1       |  BMI160_MAP_INTR2
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
static BMI160_RETURN_FUNCTION_TYPE bmi160_map_std_intr(u8 v_step_detector_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_step_det_u8 = BMI160_INIT_VALUE;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	u8 v_low_g_intr_u81_stat_u8 = BMI160_LOW_G_INTR_STAT;
	u8 v_low_g_intr_u82_stat_u8 = BMI160_LOW_G_INTR_STAT;
	u8 v_low_g_enable_u8 = BMI160_ENABLE_LOW_G;
	/* read the v_status_s8 of step detector interrupt */
	com_rslt = bmi160_get_std_enable(&v_step_det_u8);
	if (v_step_det_u8 != BMI160_STEP_DET_STAT_HIGH)
		com_rslt += bmi160_set_std_enable(BMI160_STEP_DETECT_INTR_ENABLE);
	switch (v_step_detector_u8) {
	case BMI160_MAP_INTR1:
		com_rslt += bmi160_read_reg(BMI160_USER_INTR_MAP_0_INTR1_LOW_G__REG,
					    &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_low_g_intr_u81_stat_u8;
		/* map the step detector interrupt to Low-g interrupt 1 */
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_MAP_0_INTR1_LOW_G__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		/* Enable the Low-g interrupt */
		com_rslt = bmi160_read_reg(BMI160_USER_INTR_ENABLE_1_LOW_G_ENABLE__REG,
					   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_low_g_enable_u8;
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_ENABLE_1_LOW_G_ENABLE__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);

		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		break;
	case BMI160_MAP_INTR2:
		/* map the step detector interrupt to Low-g interrupt 1 */
		com_rslt += bmi160_read_reg(BMI160_USER_INTR_MAP_2_INTR2_LOW_G__REG,
					    &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_low_g_intr_u82_stat_u8;

		com_rslt += bmi160_write_reg(BMI160_USER_INTR_MAP_2_INTR2_LOW_G__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		/* Enable the Low-g interrupt */
		com_rslt = bmi160_read_reg(BMI160_USER_INTR_ENABLE_1_LOW_G_ENABLE__REG,
					   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_low_g_enable_u8;
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_ENABLE_1_LOW_G_ENABLE__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		break;
	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;
	}
	return com_rslt;
}

/*!
*	@brief This API is used to select
*	the significant or any motion interrupt from the register 0x62 bit 1
*
*  @param  v_intr_significant_motion_select_u8 :
*	the value of significant or any motion interrupt selection
*	value    | Behaviour
* ----------|-------------------
*  0x00     |  ANY_MOTION
*  0x01     |  SIGNIFICANT_MOTION
*
*	@return results of bus communication function
*	@retval 0 -> Success
*	@retval -1 -> Error
*
*/
BMI160_RETURN_FUNCTION_TYPE bmi160_get_intr_sm_select(u8 *v_intr_significant_motion_select_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	/* read the significant or any motion interrupt */
	com_rslt = p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						  BMI160_USER_INTR_SIGNIFICATION_MOTION_SELECT__REG,
						  &v_data_u8,
						  BMI160_GEN_READ_WRITE_DATA_LENGTH);
	*v_intr_significant_motion_select_u8 =
		BMI160_GET_BITSLICE(v_data_u8, BMI160_USER_INTR_SIGNIFICATION_MOTION_SELECT);

	return com_rslt;
}

/*!
 *	@brief This API is used to write threshold
 *	definition for the any-motion interrupt
 *	from the register 0x60 bit 0 to 7
 *
 *  @param  v_any_motion_thres_u8 : The value of any motion threshold
 *
 *	@note any motion threshold changes according to accel g range
 *	accel g range can be set by the function ""
 *   accel_range    | any motion threshold
 *  ----------------|---------------------
 *      2g          |  v_any_motion_thres_u8*3.91 mg
 *      4g          |  v_any_motion_thres_u8*7.81 mg
 *      8g          |  v_any_motion_thres_u8*15.63 mg
 *      16g         |  v_any_motion_thres_u8*31.25 mg
 *	@note when v_any_motion_thres_u8 = 0
 *   accel_range    | any motion threshold
 *  ----------------|---------------------
 *      2g          |  1.95 mg
 *      4g          |  3.91 mg
 *      8g          |  7.81 mg
 *      16g         |  15.63 mg
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 *
*/
BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_any_motion_thres(u8 v_any_motion_thres_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	/* write any motion threshold */
	com_rslt = p_bmi160->BMI160_BUS_WRITE_FUNC
		(p_bmi160->dev_addr, BMI160_USER_INTR_MOTION_1_INTR_ANY_MOTION_THRES__REG,
		 &v_any_motion_thres_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);

	return com_rslt;
}

/*!
 *	@brief This API is used to write
 *	the significant skip time from the register 0x62 bit  2 and 3
 *
 *  @param  v_int_sig_mot_skip_u8 : the value of significant skip time
 *	value    | Behaviour
 * ----------|-------------------
 *  0x00     |  skip time 1.5 seconds
 *  0x01     |  skip time 3 seconds
 *  0x02     |  skip time 6 seconds
 *  0x03     |  skip time 12 seconds
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
static BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_sm_skip(u8 v_int_sig_mot_skip_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	if (v_int_sig_mot_skip_u8 <= BMI160_MAX_UNDER_SIG_MOTION) {
		/* write significant skip time */
		com_rslt = p_bmi160->BMI160_BUS_READ_FUNC
			(p_bmi160->dev_addr, BMI160_USER_INTR_SIGNIFICANT_MOTION_SKIP__REG,
			 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_SIGNIFICANT_MOTION_SKIP,
							v_int_sig_mot_skip_u8);
			com_rslt += p_bmi160->BMI160_BUS_WRITE_FUNC
				(p_bmi160->dev_addr, BMI160_USER_INTR_SIGNIFICANT_MOTION_SKIP__REG,
				 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
	} else {
		com_rslt = E_BMI160_OUT_OF_RANGE;
	}

	return com_rslt;
}

/*!
 *	@brief This API is used to write
 *	the significant proof time from the register 0x62 bit  4 and 5
 *
 *  @param  v_significant_motion_proof_u8 :
 *	the value of significant proof time
 *	value    | Behaviour
 * ----------|-------------------
 *  0x00     |  proof time 0.25 seconds
 *  0x01     |  proof time 0.5 seconds
 *  0x02     |  proof time 1 seconds
 *  0x03     |  proof time 2 seconds
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
static BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_sm_proof(u8 v_significant_motion_proof_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	if (v_significant_motion_proof_u8 <= BMI160_MAX_UNDER_SIG_MOTION) {
		/* write significant proof time */
		com_rslt = p_bmi160->BMI160_BUS_READ_FUNC
			(p_bmi160->dev_addr,
			 BMI160_USER_INTR_SIGNIFICANT_MOTION_PROOF__REG,
			 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR_SIGNIFICANT_MOTION_PROOF,
							v_significant_motion_proof_u8);
			com_rslt += p_bmi160->BMI160_BUS_WRITE_FUNC
				(p_bmi160->dev_addr,
				 BMI160_USER_INTR_SIGNIFICANT_MOTION_PROOF__REG,
				 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
	} else {
		com_rslt = E_BMI160_OUT_OF_RANGE;
	}

	return com_rslt;
}

/*!
*	@brief This API is used to write, select
*	the significant or any motion interrupt from the register 0x62 bit 1
*
*  @param  v_intr_significant_motion_select_u8 :
*	the value of significant or any motion interrupt selection
*	value    | Behaviour
* ----------|-------------------
*  0x00     |  ANY_MOTION
*  0x01     |  SIGNIFICANT_MOTION
*
*	@return results of bus communication function
*	@retval 0 -> Success
*	@retval -1 -> Error
*
*/
static BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_sm_select(u8 v_intr_significant_motion_select_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	if (v_intr_significant_motion_select_u8 <= BMI160_MAX_VALUE_SIGNIFICANT_MOTION) {
		/* write the significant or any motion interrupt */
		com_rslt = p_bmi160->BMI160_BUS_READ_FUNC
			(p_bmi160->dev_addr, BMI160_USER_INTR_SIGNIFICATION_MOTION_SELECT__REG,
			 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_SIGNIFICATION_MOTION_SELECT,
							v_intr_significant_motion_select_u8);
			com_rslt += p_bmi160->BMI160_BUS_WRITE_FUNC
				(p_bmi160->dev_addr, BMI160_USER_INTR_SIGNIFICATION_MOTION_SELECT__REG,
				 &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
	} else {
		com_rslt = E_BMI160_OUT_OF_RANGE;
	}

	return com_rslt;
}

/*!
 *	@brief This API used to trigger the  signification motion
 *	interrupt
 *
 *  @param  v_significant_u8 : The value of interrupt selection
 *  value    |  interrupt
 * ----------|-----------
 *   0       |  BMI160_MAP_INTR1
 *   1       |  BMI160_MAP_INTR2
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
*/
static BMI160_RETURN_FUNCTION_TYPE bmi160_map_sm_intr(u8 v_significant_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_sig_motion_u8 = BMI160_INIT_VALUE;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	u8 v_any_motion_intr1_stat_u8 = BMI160_ENABLE_ANY_MOTION_INTR1;
	u8 v_any_motion_intr2_stat_u8 = BMI160_ENABLE_ANY_MOTION_INTR2;
	u8 v_any_motion_axis_stat_u8 = BMI160_ENABLE_ANY_MOTION_AXIS;
	/* enable the significant motion interrupt */
	com_rslt = bmi160_get_intr_sm_select(&v_sig_motion_u8);
	if (v_sig_motion_u8 != BMI160_SIG_MOTION_STAT_HIGH)
		com_rslt += bmi160_set_intr_sm_select(BMI160_SIG_MOTION_INTR_ENABLE);
	switch (v_significant_u8) {
	case BMI160_MAP_INTR1:
		/* interrupt */
		com_rslt += bmi160_read_reg(BMI160_USER_INTR_MAP_0_INTR1_ANY_MOTION__REG,
					    &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_any_motion_intr1_stat_u8;
		/* map the signification interrupt to any-motion interrupt1 */
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_MAP_0_INTR1_ANY_MOTION__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		/* axis */
		com_rslt = bmi160_read_reg(BMI160_USER_INTR_ENABLE_0_ADDR,
					   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_any_motion_axis_stat_u8;
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_ENABLE_0_ADDR,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		break;

	case BMI160_MAP_INTR2:
		/* map the signification interrupt to any-motion interrupt2 */
		com_rslt += bmi160_read_reg(BMI160_USER_INTR_MAP_2_INTR2_ANY_MOTION__REG,
					    &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_any_motion_intr2_stat_u8;
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_MAP_2_INTR2_ANY_MOTION__REG,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		/* axis */
		com_rslt = bmi160_read_reg(BMI160_USER_INTR_ENABLE_0_ADDR,
					   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		v_data_u8 |= v_any_motion_axis_stat_u8;
		com_rslt += bmi160_write_reg(BMI160_USER_INTR_ENABLE_0_ADDR,
					     &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		p_bmi160->delay_msec(BMI160_GEN_READ_WRITE_DELAY);
		break;

	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;

	}
	return com_rslt;
}


/*!
 *	@brief  This API is used to set
 *	interrupt enable from the register 0x50 bit 0 to 7
 *
 *	@param v_enable_u8 : Value to decided to select interrupt
 *   v_enable_u8   |   interrupt
 *  ---------------|---------------
 *       0         | BMI160_ANY_MOTION_X_ENABLE
 *       1         | BMI160_ANY_MOTION_Y_ENABLE
 *       2         | BMI160_ANY_MOTION_Z_ENABLE
 *       3         | BMI160_DOUBLE_TAP_ENABLE
 *       4         | BMI160_SINGLE_TAP_ENABLE
 *       5         | BMI160_ORIENT_ENABLE
 *       6         | BMI160_FLAT_ENABLE
 *
 *	@param v_intr_enable_zero_u8 : The interrupt enable value
 *	value    | interrupt enable
 * ----------|-------------------
 *  0x01     |  BMI160_ENABLE
 *  0x00     |  BMI160_DISABLE
 *
 *	@return results of bus communication function
 *	@retval 0 -> Success
 *	@retval -1 -> Error
 *
 */
static BMI160_RETURN_FUNCTION_TYPE bmi160_set_intr_enable_0(u8 v_enable_u8,
							    u8 v_intr_enable_zero_u8)
{
	/* variable used for return the status of communication result */
	BMI160_RETURN_FUNCTION_TYPE com_rslt = E_BMI160_COMM_RES;
	u8 v_data_u8 = BMI160_INIT_VALUE;
	/* check the p_bmi160 structure as NULL */
	if (p_bmi160 == BMI160_NULL)
		return E_BMI160_NULL_PTR;

	switch (v_enable_u8) {
	case BMI160_ANY_MOTION_X_ENABLE:
		/* write any motion x */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_ANY_MOTION_X_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_0_ANY_MOTION_X_ENABLE,
							v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_ANY_MOTION_X_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_ANY_MOTION_Y_ENABLE:
		/* write any motion y */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Y_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Y_ENABLE,
							v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Y_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_ANY_MOTION_Z_ENABLE:
		/* write any motion z */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Z_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Z_ENABLE,
							v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_ANY_MOTION_Z_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_DOUBLE_TAP_ENABLE:
		/* write double tap */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_DOUBLE_TAP_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_0_DOUBLE_TAP_ENABLE,
							v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_DOUBLE_TAP_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_SINGLE_TAP_ENABLE:
		/* write single tap */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_SINGLE_TAP_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8, BMI160_USER_INTR_ENABLE_0_SINGLE_TAP_ENABLE,
							v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_SINGLE_TAP_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_ORIENT_ENABLE:
		/* write orient interrupt */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_ORIENT_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR_ENABLE_0_ORIENT_ENABLE, v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_ORIENT_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	case BMI160_FLAT_ENABLE:
		/* write flat interrupt */
		com_rslt =
			p_bmi160->BMI160_BUS_READ_FUNC(p_bmi160->dev_addr,
						   BMI160_USER_INTR_ENABLE_0_FLAT_ENABLE__REG,
						   &v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		if (com_rslt == SUCCESS) {
			v_data_u8 = BMI160_SET_BITSLICE(v_data_u8,
							BMI160_USER_INTR_ENABLE_0_FLAT_ENABLE, v_intr_enable_zero_u8);
			com_rslt +=
				p_bmi160->BMI160_BUS_WRITE_FUNC(p_bmi160->dev_addr,
								BMI160_USER_INTR_ENABLE_0_FLAT_ENABLE__REG,
								&v_data_u8, BMI160_GEN_READ_WRITE_DATA_LENGTH);
		}
		break;
	default:
		com_rslt = E_BMI160_OUT_OF_RANGE;
		break;
	}

	return com_rslt;
}

static int sm_init_interrupts(u8 sig_map_int_pin)
{
	int ret = 0;
	/*0x60  */
	ret += bmi160_set_intr_any_motion_thres(0x1e);
	/* 0x62(bit 3~2)        0=1.5s */
	ret += bmi160_set_intr_sm_skip(0);
	/* 0x62(bit 5~4) 1=0.5s */
	ret += bmi160_set_intr_sm_proof(1);
	/* 0x50 (bit 0, 1, 2)  INT_EN_0 anymo x y z */
	ret += bmi160_map_sm_intr(sig_map_int_pin);
	/* 0x62 (bit 1) INT_MOTION_3     int_sig_mot_sel close the signification_motion */
	ret += bmi160_set_intr_sm_select(0);
	/*close the anymotion interrupt */
	ret += bmi160_set_intr_enable_0(BMI160_ANY_MOTION_X_ENABLE, 0);
	ret += bmi160_set_intr_enable_0(BMI160_ANY_MOTION_Y_ENABLE, 0);
	ret += bmi160_set_intr_enable_0(BMI160_ANY_MOTION_Z_ENABLE, 0);
	if (ret)
		ACC_PR_ERR("bmi160 sig motion setting failed.\n");
	return ret;
}

#ifdef CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR
static void bmi_std_interrupt_handle(struct bmi160_accelgyro_data *client_data)
{
	u8 step_detect_enable = 0;
	int err = 0;

	err = bmi160_get_std_enable(&step_detect_enable);
	if (err < 0) {
		ACC_PR_ERR("get step detect enable failed.\n");
		return;
	}
	if (step_detect_enable == ENABLE)
		err = bmi160_step_notify(TYPE_STEP_DETECTOR);
	if (err < 0)
		ACC_PR_ERR("notify step detect failed.\n");
	ACC_LOG("step detect enable = %d.\n", step_detect_enable);
}

static void bmi_sm_interrupt_handle(struct bmi160_accelgyro_data *client_data)
{
	u8 sig_sel = 0;
	int err = 0;

	err = bmi160_get_intr_sm_select(&sig_sel);
	if (err < 0) {
		ACC_PR_ERR("get significant motion failed.\n");
		return;
	}
	if (sig_sel == ENABLE) {
		sig_flag = 1;
		err = bmi160_step_notify(TYPE_SIGNIFICANT);
	}
	if (err < 0)
		ACC_PR_ERR("notify significant motion failed.\n");
	ACC_LOG("signification motion = %d.\n", (int)sig_sel);
}

#endif

static void bmi_irq_work_func(struct work_struct *work)
{
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	u8 int_status[4] = { 0, 0, 0, 0 };

	BMP160_spi_read_bytes(obj->spi, BMI160_USER_INTR_STAT_0_ADDR, int_status, 4);
#if 0
	if (BMI160_GET_BITSLICE(int_status[0], BMI160_USER_INTR_STAT_0_ANY_MOTION))
		bmi_slope_interrupt_handle(client_data);
#endif

#ifdef CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR
	if (BMI160_GET_BITSLICE(int_status[0], BMI160_USER_INTR_STAT_0_STEP_INTR))
		bmi_std_interrupt_handle(client_data);
#endif

#if 0
	if (BMI160_GET_BITSLICE(int_status[1], BMI160_USER_INTR_STAT_1_FIFO_WM_INTR))
		bmi_fifo_watermark_interrupt_handle(client_data);
#endif

#ifdef CUSTOM_KERNEL_SIGNIFICANT_MOTION_SENSOR
	if (BMI160_GET_BITSLICE(int_status[0], BMI160_USER_INTR_STAT_0_SIGNIFICANT_INTR))
		bmi_sm_interrupt_handle(client_data);
#endif
}

static irqreturn_t bmi_irq_handler(int irq, void *handle)
{
	struct bmi160_accelgyro_data *client_data = accelgyro_obj_data;

	if (client_data == NULL)
		return IRQ_HANDLED;
	schedule_work(&client_data->accel_irq_work);
	return IRQ_HANDLED;
}

#ifdef MTK_NEW_ARCH_ACCEL
static int bmi160acc_probe(struct platform_device *pdev)
{
	accelPltFmDev = pdev;
	return 0;
}

static int bmi160acc_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id bmi160_of_match[] = {
	{.compatible = "mediatek,st_step_counter",},
	{},
};
#endif

static struct platform_driver bmi160acc_driver = {
	.probe = bmi160acc_probe,
	.remove = bmi160acc_remove,
	.driver = {
		   .name = "stepcounter",
		   /* .owner  = THIS_MODULE, */
#ifdef CONFIG_OF
		   .of_match_table = bmi160_of_match,
#endif
		   }
};

static int bmi160acc_setup_eint(struct spi_device *spi)
{
	int ret;
	struct device_node *node = NULL;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = { 0, 0 };
	struct bmi160_accelgyro_data *obj = accelgyro_obj_data;
	/* gpio setting */
	pinctrl = devm_pinctrl_get(&spi->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		ACC_PR_ERR("Cannot find step pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		ACC_PR_ERR("Cannot find step pinctrl default!\n");
		return ret;
	}
	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		ACC_PR_ERR("Cannot find step pinctrl pin_cfg!\n");
		return ret;
	}
	pinctrl_select_state(pinctrl, pins_cfg);
	node = of_find_compatible_node(NULL, NULL, "mediatek,ACC_1-eint");
	/* eint request */
	if (node) {
		ACC_LOG("irq node is ok!");
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);
		ACC_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		obj->IRQ = irq_of_parse_and_map(node, 0);
		ACC_LOG("obj->IRQ = %d\n", obj->IRQ);
		if (!obj->IRQ) {
			ACC_PR_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(obj->IRQ, bmi_irq_handler, IRQF_TRIGGER_RISING, "mediatek,ACC_1-eint", NULL)) {
			ACC_PR_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq(obj->IRQ);
	} else {
		ACC_PR_ERR("null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}
#endif

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

	cali[BMG_AXIS_X] = data[0] * gyro_obj_data->gyro_sensitivity / BMI160_FS_250_LSB;
	cali[BMG_AXIS_Y] = data[1] * gyro_obj_data->gyro_sensitivity / BMI160_FS_250_LSB;
	cali[BMG_AXIS_Z] = data[2] * gyro_obj_data->gyro_sensitivity / BMI160_FS_250_LSB;
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
	data[0] = cali[BMG_AXIS_X] * BMI160_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
	data[1] = cali[BMG_AXIS_Y] * BMI160_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
	data[2] = cali[BMG_AXIS_Z] * BMI160_FS_250_LSB / gyro_obj_data->gyro_sensitivity;
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

static int bmi160_accelgyro_spi_probe(struct spi_device *spi)
{

	int err = 0;
	/* accel sensor */
	struct bmi160_accelgyro_data *obj;
	struct acc_control_path accel_ctl = { 0 };
	struct acc_data_path accel_data = { 0 };

	struct gyro_control_path gyro_ctl = { 0 };
	struct gyro_data_path gyro_data = { 0 };

	ACC_LOG("enter bmi160_accelgyro_spi_probe");

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
	memset(obj, 0, sizeof(struct bmi160_accelgyro_data));
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

    /*Spi end*/

	/* acc */
	accelgyro_obj_data = obj;
	atomic_set(&obj->accel_trace, 0);
	atomic_set(&obj->accel_suspend, 0);
	mutex_init(&obj->accel_lock);

   /* gyro */
	gyro_obj_data = obj;
	atomic_set(&obj->gyro_trace, 0);
	atomic_set(&obj->gyro_suspend, 0);
	obj->gyro_power_mode = BMG_UNDEFINED_POWERMODE;
	obj->gyro_range = BMG_UNDEFINED_RANGE;
	obj->gyro_datarate = BMI160_GYRO_ODR_RESERVED;
	mutex_init(&obj->gyro_lock);
#ifdef CONFIG_BMG_LOWPASS
	if (obj->hw->gyro_firlen > C_MAX_FIR_LENGTH)
		atomic_set(&obj->gyro_firlen, C_MAX_FIR_LENGTH);
	else
		atomic_set(&obj->gyro_firlen, obj->hw->gyro_firlen);

	if (atomic_read(&obj->gyro_firlen) > 0)
		atomic_set(&obj->gyro_fir_en, 1);
#endif
    /*gyro end*/

	mutex_init(&obj->spi_lock);

	err = bmi160_acc_init_client(obj, 1);
	if (err)
		goto exit_init_failed;

	err = bmg_init_client(obj, 1);
	if (err)
		goto exit_init_client_failed;


#ifdef MTK_NEW_ARCH_ACCEL
	platform_driver_register(&bmi160acc_driver);
#endif

#ifdef FIFO_READ_USE_DMA_MODE_I2C
	gpDMABuf_va = (u8 *) dma_alloc_coherent(NULL,
						GTP_DMA_MAX_TRANSACTION_LENGTH, &gpDMABuf_pa,
						GFP_KERNEL);
	memset(gpDMABuf_va, 0, GTP_DMA_MAX_TRANSACTION_LENGTH);
#endif

#ifndef MTK_NEW_ARCH_ACCEL
	obj->accel_gpio_pin = 94;
	obj->accel_IRQ = gpio_to_irq(obj->accel_gpio_pin);
	err = request_irq(obj->accel_IRQ, bmi_irq_handler, IRQF_TRIGGER_RISING, "bmi160", obj);
	if (err)
		ACC_PR_ERR("could not request accel_IRQ\n");
#else

	err = bmi160acc_setup_eint(spi);
	if (err)
		ACC_PR_ERR("could not request accel_IRQ\n");

#endif
   /* acc factory */
	err = accel_factory_device_register(&bmi160_accel_factory_device);
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

	err = bmi160_accelgyro_create_attr(&(bmi160_accelgyro_spi_driver.driver));
	if (err) {
		ACC_PR_ERR("create attribute failed.\n");
		goto exit_create_attr_failed;
	}

	/* Init control path,acc */
	accel_ctl.enable_nodata = bmi160_acc_enable_nodata;
	accel_ctl.batch = bmi160_acc_batch;
	accel_ctl.flush = bmi160_acc_flush;
	accel_ctl.is_support_batch = false;	/* using customization info */
	accel_ctl.is_report_input_direct = false;

	err = acc_register_control_path(&accel_ctl);
	if (err) {
		ACC_PR_ERR("register acc control path error.\n");
		goto exit_kfree;
	}
	/* gyro */
	gyro_ctl.open_report_data = bmi160_gyro_open_report_data;
	gyro_ctl.enable_nodata = bmi160_gyro_enable_nodata;
	gyro_ctl.batch = bmi160_gyro_batch;
	gyro_ctl.flush = bmi160_gyro_flush;
	gyro_ctl.is_report_input_direct = false;
	gyro_ctl.is_support_batch = obj->hw->gyro_is_batch_supported;
	ACC_LOG(">>gyro_register_control_path\n");
	err = gyro_register_control_path(&gyro_ctl);
	if (err) {
		GYRO_PR_ERR("register gyro control path err\n");
		goto exit_create_attr_failed;
	}

	/* Init data path,acc */
	accel_data.get_data = bmi160_acc_get_data;
	accel_data.vender_div = 1000;
	err = acc_register_data_path(&accel_data);
	if (err) {
		ACC_PR_ERR("register acc data path error.\n");
		goto exit_kfree;
	}
	/* gyro */
	gyro_data.get_data = bmi160_gyro_get_data;
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
	bmi160_init(&obj->device);
	/* soft reset */
	bmi160_set_command_register(0xB6);
	mdelay(3);
	/* maps interrupt to INT1 pin, set interrupt trigger level way */
	bmi160_set_intr_edge_ctrl(BMI_INT0, BMI_INT_LEVEL);
	udelay(500);
	bmi160_set_intr_fifo_wm(BMI_INT0, ENABLE);
	udelay(500);
	bmi160_set_intr_level(BMI_INT0, ENABLE);
	udelay(500);
	bmi160_set_output_enable(BMI160_INTR1_OUTPUT_ENABLE, ENABLE);
	udelay(500);
	sm_init_interrupts(BMI160_MAP_INTR1);
	mdelay(3);
	bmi160_map_std_intr(BMI160_MAP_INTR1);
	mdelay(3);
	bmi160_set_std_enable(DISABLE);
	mdelay(3);
	/* fifo setting */
	bmi160_set_fifo_header_enable(ENABLE);
	udelay(500);
	bmi160_set_fifo_time_enable(DISABLE);
	udelay(500);
	bmi160_acc_init_client(obj, 1);
	INIT_WORK(&obj->accel_irq_work, bmi_irq_work_func);

	bmi160_acc_init_flag = 0;
	bmi160_gyro_init_flag = 0;

	ACC_LOG("%s: is ok.\n", __func__);
	return 0;

exit_create_attr_failed:
	/* misc_deregister(&bmi160_acc_device); */
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
	bmi160_acc_init_flag = -1;
	bmi160_gyro_init_flag = -1;
	return err;
}

static int bmi160_accelgyro_spi_remove(struct spi_device *spi)
{
	struct bmi160_accelgyro_data *obj = spi_get_drvdata(spi);
	int err = 0;

	err = bmi160_accelgyro_delete_attr(&(bmi160_accelgyro_spi_driver.driver));
	if (err)
		ACC_PR_ERR("delete device attribute failed.\n");

	if (obj->spi_buffer != NULL) {
		kfree(obj->spi_buffer);
		obj->spi_buffer = NULL;
	}

	spi_set_drvdata(spi, NULL);
	obj->spi = NULL;

	/* misc_deregister(&bmi160_acc_device); */
	accel_factory_device_deregister(&bmi160_accel_factory_device);
	gyro_factory_device_deregister(&bmg_factory_device);

	kfree(accelgyro_obj_data);
	return 0;
}

static int bmi160_accelgyro_local_init(void)
{
	ACC_LOG(">>>>-bmi160_accelgyro_local_init.\n");
	if (spi_register_driver(&bmi160_accelgyro_spi_driver))
		return -1;
	if ((-1 == bmi160_acc_init_flag) || (-1 == bmi160_gyro_init_flag))
		return -1;
	ACC_LOG("<<<<<-bmi160_accelgyro_local_init.\n");
	return 0;
}

static int bmi160_accelgyro_remove(void)
{
	ACC_FUN();
	spi_unregister_driver(&bmi160_accelgyro_spi_driver);
	return 0;
}

static struct accelgyro_init_info bmi160_accelgyro_init_info = {
	.name = BMI160_ACCELGYRO_DEV_NAME,
	.init = bmi160_accelgyro_local_init,
	.uninit = bmi160_accelgyro_remove,
};

static int __init bmi160_accelgyro_init(void)
{
	ACC_FUN();
	accelgyro_driver_add(&bmi160_accelgyro_init_info);
	return 0;
}

static void __exit bmi160_accelgyro_exit(void)
{
	ACC_FUN();
}
module_init(bmi160_accelgyro_init);
module_exit(bmi160_accelgyro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BMI160_ACC_GYRO SPI driver");
MODULE_AUTHOR("ruixue.su@mediatek.com");

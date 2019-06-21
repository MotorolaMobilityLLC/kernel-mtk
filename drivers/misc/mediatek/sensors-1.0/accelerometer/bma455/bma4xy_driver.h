/* BOSCH BMA4XY ACC Sensor Driver
 * (C) Copyright 2011~2017 Bosch Sensortec GmbH All Rights Reserved
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 * VERSION: v0.0.2.5
 * Date: 2017/05/08
 */
#ifndef _BMA4XY_DRIVER_H
#define _BMA4XY_DRIVER_H
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
#include <linux/firmware.h>
//#include <linux/hwmsensor.h>
//#include <linux/hwmsen_dev.h>
//#include <linux/sensors_io.h>
//#include <linux/hwmsen_helper.h>
#include "bma4_defs.h"
#include "bma4.h"
#if defined(BMA420)
#include "bma420.h"
#endif
#if defined(BMA421)
#include "bma421.h"
#endif
#if defined(BMA424SC)
#include "bma424sc.h"
#endif
#if defined(BMA421L)
#include "bma421l.h"
#endif
#if defined(BMA422)
#include "bma422.h"
#endif
#if defined(BMA424)
#include "bma424.h"
#endif
#if defined(BMA455)
#include "bma455.h"
#endif
#if defined(BMA422N)
#include "bma422n.h"
#endif
#if defined(BMA455N)
#include "bma455n.h"
#endif
#include <cust_acc.h>
#include <accel.h>


/* sensor name definition*/
#define SENSOR_NAME "bma455_acc"
#define SENSOR_NAME_UC "bma4xy_uc"
/*struct i2c_client *bma4xy_i2c_client;*/

/* generic */
#define BMA4XY_CHIP_ID (0x00)
#define CHECK_CHIP_ID_TIME_MAX   5
#define FIFO_DATA_BUFSIZE_BMA4XY    1024
#define BMA4XY_ENABLE_INT1  1
#define BMA4XY_I2C_WRITE_DELAY_TIME 1
#define BMA4XY_MAX_RETRY_I2C_XFER (10)
#define BMA4XY_MAX_RETRY_WAIT_DRDY (100)
#define BMA4XY_I2C_WRITE_DELAY_TIME 1
#define BMA4XY_MAX_RETRY_WAKEUP (5)
#define BMA4XY_DELAY_MIN (1)
#define BMA4XY_DELAY_DEFAULT (200)
#define BMA4XY_VALUE_MAX (32767)
#define BMA4XY_VALUE_MIN (-32768)
#define BYTES_PER_LINE (16)
#define REL_UC_STATUS    1
#define BMA4XY_BUFSIZE				256

/*fifo definition*/
#define A_BYTES_FRM      6
#define M_BYTES_FRM      8
#define MA_BYTES_FRM     14
#define I2C_DRIVERID_BMA4XY 422
#define DEBUG 1
#define SW_CALIBRATION
#define BMA4XY_ACC_AXIS_X          0
#define BMA4XY_ACC_AXIS_Y          1
#define BMA4XY_ACC_AXIS_Z          2
#define BMA4XY_ACC_AXIS_NUM     3
#define BMA4XY_DATA_LEN        6

enum ADX_TRC {
	ADX_TRC_FILTER = 0x01,
	ADX_TRC_RAWDATA = 0x02,
	ADX_TRC_IOCTL = 0x04,
	ADX_TRC_CALI = 0X08,
	ADX_TRC_INFO = 0X10,
};
struct scale_factor {
	u8 whole;
	u8 fraction;
};
struct data_resolution {
	struct scale_factor scalefactor;
	int sensitivity;
};
#define C_MAX_FIR_LENGTH (32)
struct data_filter {
	s16 raw[C_MAX_FIR_LENGTH][BMA4XY_ACC_AXIS_NUM];
	int sum[BMA4XY_ACC_AXIS_NUM];
	int num;
	int idx;
};
struct bma4xy_client_data {
	struct bma4_dev device;
	struct i2c_client *client;
	struct acc_hw *hw;
	/*misc */
	struct data_resolution *reso;
	atomic_t trace;
	atomic_t suspend;
	atomic_t filter;
	struct hwmsen_convert cvt;
	s16 cali_sw[BMA4XY_ACC_AXIS_NUM + 1];
	/*data */
	s8 offset[BMA4XY_ACC_AXIS_NUM + 1];	/*+1: for 4-byte alignment */
	s16 data[BMA4XY_ACC_AXIS_NUM + 1];
	struct input_dev *acc_input;
	struct input_dev *uc_input;
	uint8_t fifo_mag_enable;
	uint8_t fifo_acc_enable;
	uint32_t fifo_bytecount;
	uint8_t acc_pm;
	uint8_t acc_odr;
	uint8_t debug_level;
	int IRQ;
	uint8_t gpio_pin;
	struct work_struct irq_work;
	uint16_t fw_version;
	uint8_t config_stream_choose;
	char *config_stream_name;
	unsigned long config_stream_size;
	int reg_sel;
	int reg_len;
	struct wake_lock wakelock;
	struct delayed_work delay_work_sig;
#if defined(BMA422N) || defined(BMA455N)
	struct delayed_work delay_work_any_motion;
	struct delayed_work delay_work_no_motion;
#endif
	atomic_t in_suspend;
	uint8_t tap_type;
	uint8_t selftest;
	uint8_t sigmotion_enable;
	uint8_t stepdet_enable;
	uint8_t stepcounter_enable;
	uint8_t tilt_enable;
	uint8_t pickup_enable;
	uint8_t glance_enable;
	uint8_t wakeup_enable;
	uint8_t anymotion_enable;
	uint8_t nomotion_enable;
	uint8_t orientation_enable;
	uint8_t flat_enable;
	uint8_t tap_enable;
	uint8_t highg_enable;
	uint8_t lowg_enable;
	uint8_t activity_enable;
	uint8_t err_int_trigger_num;
	uint32_t step_counter_val;
	uint32_t step_counter_temp;
	uint8_t any_motion_axis;
	uint8_t no_motion_axis;
	uint8_t foc;
	uint8_t change_step_par;
};
#endif

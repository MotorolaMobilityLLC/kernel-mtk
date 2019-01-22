/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __BIOMETRIC_H__
#define __BIOMETRIC_H__

#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include "sensor_attr.h"
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/atomic.h>
#include <linux/ioctl.h>

#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <hwmsensor.h>
#include <linux/poll.h>
#include "sensor_event.h"


#define BIO_TAG						"<BIOMETRIC> "
#define BIO_PR_ERR(fmt, args...)		pr_err(BIO_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define BIO_LOG(fmt, args...)		pr_debug(BIO_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define BIO_VER(fmt, args...)		pr_debug(BIO_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define BIO_INFO(fmt, args...)		pr_info(BIO_TAG"%s %d : "fmt, __func__, __LINE__, ##args)

#define BIO_INVALID_VALUE -1

#define MAX_CHOOSE_BIO_NUM 5

struct bio_control_path {
	int (*enable_ekg)(int en);	/* only enable not report event to HAL */
	int (*enable_ppg1)(int en);	/* only enable not report event to HAL */
	int (*enable_ppg2)(int en);	/* only enable not report event to HAL */
	int (*set_delay_ekg)(u64 delay);
	int (*set_delay_ppg1)(u64 delay);
	int (*set_delay_ppg2)(u64 delay);
	bool is_report_input_direct;
	bool is_support_batch;	/* version2.used for batch mode support flag */
};

struct bio_data_path {
	int (*get_data_ekg)(int *raw_data, u32 *length);
	int (*get_data_ppg1)(int *raw_data, int *amb_data, int *agc_data, u32 *length);
	int (*get_data_ppg2)(int *raw_data, int *amb_data, int *agc_data, u32 *length);
	int vender_div;
};

struct bio_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver *platform_diver_addr;
};

struct bio_data {
	struct hwm_sensor_data bio_data;
	int data_updata;
	/* struct mutex lock; */
};

struct bio_context {
	struct input_dev *idev;
	struct sensor_attr_t mdev;
	struct work_struct report;
	struct mutex bio_op_mutex;
	int64_t ndelay;		/*polling period for reporting input event */
	struct hrtimer hrTimer;
	ktime_t target_ktime;
	struct workqueue_struct *bio_workqueue;
	unsigned long long active;
	struct bio_control_path bio_ctl;
	struct bio_data_path bio_data;
	struct bio_data drv_data;
	/* Active, but HAL don't need data sensor. such as orientation need */
	bool is_polling_run;
	int ekg_sn;
	int ppg1_sn;
	int ppg2_sn;
	int64_t ekg_read_time;
	int64_t ppg1_read_time;
	int64_t ppg2_read_time;
};


/* for auto detect */
extern int bio_driver_add(struct bio_init_info *obj);
extern int ekg_data_report(struct bio_data *data);
extern int ppg1_data_report(struct bio_data *data);
extern int ppg2_data_report(struct bio_data *data);
extern int bio_register_control_path(struct bio_control_path *ctl);
extern int bio_register_data_path(struct bio_data_path *data);

#endif

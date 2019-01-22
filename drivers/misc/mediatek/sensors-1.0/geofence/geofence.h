/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#ifndef __GEOFENCE_H__
#define __GEOFENCE_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include "sensor_attr.h"
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/atomic.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/errno.h>

#include "scp_helper.h"
#include <linux/notifier.h>
#include <SCP_sensorHub.h>
#include <sensors_io.h>
#include <hwmsen_helper.h>
#include <hwmsensor.h>
#include "sensor_event.h"

#define GEOFENCE_TAG          "<GEOFENCE> "
#define GEOFENCE_FUN(f)       pr_debug(GEOFENCE_TAG"%s\n", __func__)
#define GEOFENCE_PR_ERR(fmt, args...)  pr_err(GEOFENCE_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define GEOFENCE_LOG(fmt, args...)  pr_debug(GEOFENCE_TAG fmt, ##args)
#define GEOFENCE_VER(fmt, args...)  pr_debug(GEOFENCE_TAG"%s: "fmt, __func__, ##args) /* ((void)0) */

struct geofence_database {
	int source;
	int result;
	int mode;
};

#define IOCTL_GEOFENCE_ID      'G'
#define IOCTL_GEOFENCE_ENABLE _IO(IOCTL_GEOFENCE_ID, 1)
#define IOCTL_GEOFENCE_GET_DATA _IOR(IOCTL_GEOFENCE_ID, 2, struct geofence_database)
#define IOCTL_GEOFENCE_INJECT_CMD _IOW(IOCTL_GEOFENCE_ID, 3, int)
#define IOCTL_GEOFENCE_DISABLE _IO(IOCTL_GEOFENCE_ID, 4)

#define GEOFENCE_INVALID_VALUE -1

struct geofence_control_path {
	int (*open_report_data)(int open);  /* open data rerport to HAL */
	int (*enable_nodata)(int en); /* only enable not report event to HAL */
	int (*set_delay)(u64 delay);
	int (*batch)(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs);
	int (*flush)(void);/* open data rerport to HAL */
	bool is_report_input_direct;
	bool is_support_batch;/* version2.used for batch mode support flag */
};

struct geofence_data_path {
	int (*get_data)(int *result, int *source, int *mode);
	int vender_div;
};

struct geofence_data {
	struct hwm_sensor_data geofence_data;
	int data_updata;
  /* struct mutex lock; */
};

struct geofence_drv_obj {
	void *self;
	int polling;
	int (*geofence_operate)(void *self, uint32_t command, void *buff_in, int size_in,
	void *buff_out, int size_out, int *actualout);
};

struct geofence_control_context {
	struct geofence_data drv_data;
	struct geofence_control_path geofence_ctl;
	struct geofence_data_path geofence_data;
	bool is_active_nodata;
	bool is_active_data;
	bool is_first_data_after_enable;
	bool is_polling_run;
	bool is_batch_enable;
};

#endif

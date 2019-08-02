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

#ifndef __SITUATION_H__
#define __SITUATION_H__


#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <hwmsensor.h>
#include <linux/poll.h>
#include "sensor_attr.h"
#include "sensor_event.h"

#define SITUATION_TAG		            "<SITUATION> "
#define SITUATION_FUN(f)		        pr_debug(SITUATION_TAG"%s\n", __func__)
#define SITUATION_PR_ERR(fmt, args...)		pr_err(SITUATION_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define SITUATION_LOG(fmt, args...)		pr_debug(SITUATION_TAG fmt, ##args)
#define SITUATION_VER(fmt, args...)		pr_debug(SITUATION_TAG"%s: "fmt, __func__, ##args)

#define SITUATION_INVALID_VALUE -1

typedef enum {
	inpocket = 0,
	stationary,
	wake_gesture,
	pickup_gesture,
	glance_gesture,
	answer_call,
	motion_detect,
	device_orientation,
	tilt_detector,
	flat,
	sar, //add for SAR by Ontim
	max_situation_support,
} situation_index_table;

struct situation_control_path {
	int (*open_report_data)(int open);
	int (*batch)(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs);
	int (*flush)(void);
	bool is_support_wake_lock;
	bool is_support_batch;

	bool is_polling_mode;
};

struct situation_data_path {
	int (*get_data)(int *value, int *status);
};

struct situation_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
};

struct situation_data_control_context {
	struct situation_control_path situation_ctl;
	struct situation_data_path situation_data;
	bool is_active_data;
	bool is_active_nodata;
	bool is_batch_enable;
	int power;
	int enable;
	int64_t delay_ns;
	int64_t latency_ns;
};

struct sar_data {
	struct hwm_sensor_data sar_data;
	int data_updata;
};

struct situation_context {
	struct sensor_attr_t mdev;
	struct mutex situation_op_mutex;
	struct situation_data_control_context ctl_context[max_situation_support];
	struct wake_lock wake_lock[max_situation_support];
	char *wake_lock_name[max_situation_support];

	//add for SAR by Ontim
	struct sar_data	drv_data;
	struct work_struct	report_sar;
	struct timer_list		timer_sar;  /* sar polling timer */
	atomic_t delay_sar;/*sar polling period for reporting input event*/
	bool is_sar_first_data_after_enable;
	bool is_sar_polling_run;
	bool is_sar_batch_enable;	/* version2.this is used for judging whether sensor is in batch mode */
	bool is_get_valid_sar_data_after_enable;
	//end, Ontim
};

extern int sar_report_interrupt_data(int value);
extern int situation_data_report(int handle, uint32_t one_sample_data);
extern int situation_notify(int handle);
extern int situation_flush_report(int handle);
extern int situation_driver_add(struct situation_init_info *obj, int handle);
extern int situation_register_control_path(struct situation_control_path *ctl, int handle);
extern int situation_register_data_path(struct situation_data_path *data, int handle);

#endif

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

#ifndef __RGBW_H__
#define __RGBW_H__

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <batch.h>
#include <sensors_io.h>
#include <hwmsensor.h>
#include <hwmsen_dev.h>
#include "rgbw_factory.h"

#define RGBW_TAG		 "<RGBW> "
#define RGBW_FUN(f)		 pr_debug(RGBW_TAG"%s\n", __func__)
#define RGBW_ERR(fmt, args...)	 pr_debug(RGBW_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define RGBW_LOG(fmt, args...)	 pr_debug(RGBW_TAG fmt, ##args)
#define RGBW_VER(fmt, args...)   pr_debug(RGBW_TAG"%s: "fmt, __func__, ##args) /* ((void)0) */

#define   OP_rgbw_DELAY	0X01
#define	OP_rgbw_ENABLE	0X02
#define	OP_rgbw_GET_DATA	0X04

#define RGBW_INVALID_VALUE -1

#define EVENT_TYPE_RGBW_VALUE			ABS_X
#define EVENT_TYPE_RGBW_VALUE_G			ABS_Y
#define EVENT_TYPE_RGBW_VALUE_B			ABS_Z
#define EVENT_TYPE_RGBW_VALUE_W			ABS_RX
#define EVENT_TYPE_RGBW_STATUS			ABS_WHEEL


#define RGBW_VALUE_MAX (32767)
#define RGBW_VALUE_MIN (-32768)
#define RGBW_STATUS_MIN (0)
#define RGBW_STATUS_MAX (64)
#define RGBW_DIV_MAX (32767)
#define RGBW_DIV_MIN (1)


#define MAX_CHOOSE_RGBW_NUM 5

struct rgbw_control_path {
	int (*open_report_data)(int open);/* open data rerport to HAL */
	int (*enable_nodata)(int en);/* only enable not report event to HAL */
	int (*set_delay)(u64 delay);
	int (*access_data_fifo)(void);/* version2.used for flush operate */
	bool is_report_input_direct;
	bool is_support_batch;/* version2.used for batch mode support flag */
	bool is_polling_mode;
	bool is_use_common_factory;
};

struct rgbw_data_path {
	int (*get_data)(int *rgbw_value, int *value_g, int *value_b, int *value_w, int *status);
	int (*rgbw_get_raw_data)(int *rgbw_value);
	int vender_div;
};

struct rgbw_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver *platform_diver_addr;
};

struct rgbw_data {
	struct hwm_sensor_data rgbw_data;
	int data_updata;
};

struct rgbw_drv_obj {
	void *self;
	int polling;
	int (*rgbw_operate)(void *self, uint32_t command, void *buff_in, int size_in,
		void *buff_out, int size_out, int *actualout);
};

struct rgbw_context {
	struct input_dev		*idev;
	struct miscdevice	mdev;
	struct work_struct	report_rgbw;
	struct mutex			rgbw_op_mutex;
	struct timer_list		timer_rgbw;  /*rgbw polling timer */

	atomic_t			trace;
	atomic_t			delay_rgbw; /*rgbw polling period for reporting input event*/
	atomic_t			wake;  /*user-space request to wake-up, used with stop*/

	atomic_t			early_suspend;

	struct rgbw_data	drv_data;
	struct rgbw_control_path	rgbw_ctl;
	struct rgbw_data_path	rgbw_data;

	bool is_rgbw_active_nodata;/* Active, but HAL don't need data sensor. such as orientation need */
	bool is_rgbw_active_data;/* Active and HAL need data . */

	bool is_rgbw_first_data_after_enable;
	bool is_rgbw_polling_run;
	bool is_rgbw_batch_enable;/* version2.this is used for judging whether sensor is in batch mode */
	bool is_get_valid_rgbw_data_after_enable;
};

/* for auto detect */
extern int rgbw_driver_add(struct rgbw_init_info *obj);
extern int rgbw_data_report(struct input_dev *dev, int value, int value_g, int value_b, int value_w, int status);
extern int rgbw_register_control_path(struct rgbw_control_path *ctl);
extern int rgbw_register_data_path(struct rgbw_data_path *data);
extern struct platform_device *get_rgbw_platformdev(void);
#endif

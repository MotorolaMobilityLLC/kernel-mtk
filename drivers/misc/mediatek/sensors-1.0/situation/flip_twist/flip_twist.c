// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) "[fliptwist] " fmt

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

#include <hwmsensor.h>
#include <sensors_io.h>
#include "flip_twist.h"
#include "situation.h"

#include <hwmsen_helper.h>

#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "include/scp.h"

static int flip_twist_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_FLIP_TWIST, &data);
	if (err < 0) {
		pr_err("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp = data.time_stamp;
	*probability = data.gesture_data_t.probability;
	return 0;
}
static int flip_twist_open_report_data(int open)
{
	int ret = 0;

#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_FLIP_TWIST, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	ret = sensor_enable_to_hub(ID_FLIP_TWIST, open);
	return ret;
}
static int flip_twist_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_FLIP_TWIST,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}
static int flip_twist_recv_data(struct data_unit_t *event,
	void *reserved)
{
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
		pr_err("flip_twist do not support flush\n");
	else if (event->flush_action == DATA_ACTION)
		err = situation_notify_t(ID_FLIP_TWIST,
			(int64_t)event->time_stamp);
	return err;
}

static int flip_twist_hub_local_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = flip_twist_open_report_data;
	ctl.batch = flip_twist_batch;
	ctl.is_support_wake_lock = true;
	err = situation_register_control_path(&ctl, ID_FLIP_TWIST);
	if (err) {
		pr_err("register flip_twist control path err\n");
		goto exit;
	}

	data.get_data = flip_twist_get_data;
	err = situation_register_data_path(&data, ID_FLIP_TWIST);
	if (err) {
		pr_err("register flip_twist data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_FLIP_TWIST,
		flip_twist_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit_create_attr_failed;
	}
	return 0;
exit:
exit_create_attr_failed:
	return -1;
}
static int flip_twist_hub_local_uninit(void)
{
	return 0;
}

static struct situation_init_info flip_twist_hub_init_info = {
	.name = "flip_twist_hub",
	.init = flip_twist_hub_local_init,
	.uninit = flip_twist_hub_local_uninit,
};

static int __init flip_twist_hub_init(void)
{
	situation_driver_add(&flip_twist_hub_init_info, ID_FLIP_TWIST);
	return 0;
}

static void __exit flip_twist_hub_exit(void)
{
	pr_err("%s\n", __func__);
}

module_init(flip_twist_hub_init);
module_exit(flip_twist_hub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FLIP_TWIST_HUB driver");
MODULE_AUTHOR("jing.yu@yinno.com");


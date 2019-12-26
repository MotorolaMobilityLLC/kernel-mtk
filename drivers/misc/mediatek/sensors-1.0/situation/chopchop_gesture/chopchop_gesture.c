/* chopchop motion sensor driver
 *
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

#define pr_fmt(fmt) "[chopchop] " fmt

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
#include "chopchop_gesture.h"
#include <situation.h>
#include <hwmsen_helper.h>


#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "scp_helper.h"

static struct situation_init_info chopchop_init_info;

static int chopchop_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_CHOPCHOP_GESTURE, &data);
	if (err < 0) {
		pr_err("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp = data.time_stamp;
	*probability = data.gesture_data_t.probability;
	return 0;
}

static int chopchop_open_report_data(int open)
{
	int ret = 0;

#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_CHOPCHOP_GESTURE, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	pr_debug("%s : type=%d, open=%d\n", __func__, ID_CHOPCHOP_GESTURE, open);
	ret = sensor_enable_to_hub(ID_CHOPCHOP_GESTURE, open);
	return ret;
}
static int chopchop_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_CHOPCHOP_GESTURE,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}
static int chopchop_recv_data(struct data_unit_t *event,
	void *reserved)
{
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
		pr_debug("chopchop_gesture do not support flush\n");
	else if (event->flush_action == DATA_ACTION)
		err = situation_notify(ID_CHOPCHOP_GESTURE);
	return err;
}

static int chopchop_local_init(void)
{
	struct situation_control_path ctl = { 0 };
	struct situation_data_path data = { 0 };
	int err = 0;

	pr_debug("%s\n", __func__);

	ctl.open_report_data = chopchop_open_report_data;
	ctl.batch = chopchop_batch;
	ctl.is_support_wake_lock = true;
	err = situation_register_control_path(&ctl, ID_CHOPCHOP_GESTURE);
	if (err) {
		pr_err("register chopchop_gesture control path err\n");
		goto exit;
	}

	data.get_data = chopchop_get_data;
	err = situation_register_data_path(&data, ID_CHOPCHOP_GESTURE);
	if (err) {
		pr_err("register chopchop_gesture data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_CHOPCHOP_GESTURE,
		chopchop_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit_create_attr_failed;
	}
	return 0;
exit:
exit_create_attr_failed:
	return -1;
}

static int chopchop_local_uninit(void)
{
	return 0;
}

static struct situation_init_info chopchop_init_info = {
	.name = "chopchop_gesture",
	.init = chopchop_local_init,
	.uninit = chopchop_local_uninit,
};

static int __init chopchop_init(void)
{
	situation_driver_add(&chopchop_init_info, ID_CHOPCHOP_GESTURE);
	return 0;
}

static void __exit chopchop_exit(void)
{
	pr_debug("%s\n", __func__);
}

module_init(chopchop_init);
module_exit(chopchop_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CHOPCHOP_GESTURE driver");
MODULE_AUTHOR("zhigang.he@ontim.cn");

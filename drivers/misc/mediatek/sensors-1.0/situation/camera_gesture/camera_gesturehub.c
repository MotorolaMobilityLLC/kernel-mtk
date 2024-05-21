// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) "[camera_gesturehub] " fmt

#include <hwmsensor.h>
#include "camera_gesturehub.h"
#include <situation.h>
#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "include/scp.h"

static struct situation_init_info camera_gesture_hub_init_info;

static int camera_gesture_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_CAMERA_GESTURE, &data);
	if (err < 0) {
		pr_err("camera_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp		= data.time_stamp;
	*probability	= data.gesture_data_t.probability;
	return 0;
}
static int camera_gesture_open_report_data(int open)
{
	int ret = 0;
	ret = sensor_enable_to_hub(ID_CAMERA_GESTURE, open);
	return ret;
}
static int camera_gesture_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_CAMERA_GESTURE,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int camera_gesture_flush(void)
{
	return sensor_flush_to_hub(ID_CAMERA_GESTURE);
}

static int camera_gesture_recv_data(struct data_unit_t *event, void *reserved)
{
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
		err = situation_flush_report(ID_CAMERA_GESTURE);
    else if (event->flush_action == DATA_ACTION) {
		err = situation_data_report_t(ID_CAMERA_GESTURE,
			event->camera_event.state, (int64_t)event->time_stamp);
    }
	return err;
}

static int camera_gesture_hub_local_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = camera_gesture_open_report_data;
	ctl.batch = camera_gesture_batch;
	ctl.flush = camera_gesture_flush;
	ctl.is_support_wake_lock = true;
	ctl.is_support_batch = false;
	err = situation_register_control_path(&ctl, ID_CAMERA_GESTURE);
	if (err) {
		pr_err("register camera_gesture control path err\n");
		goto exit;
	}

	data.get_data = camera_gesture_get_data;
	err = situation_register_data_path(&data, ID_CAMERA_GESTURE);
	if (err) {
		pr_err("register camera_gesture data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_CAMERA_GESTURE,
		camera_gesture_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit;
	}
	return 0;
exit:
	return -1;
}
static int camera_gesture_hub_local_uninit(void)
{
	return 0;
}

static struct situation_init_info camera_gesture_hub_init_info = {
	.name = "camera_gesture_hub",
	.init = camera_gesture_hub_local_init,
	.uninit = camera_gesture_hub_local_uninit,
};

static int __init camera_gesture_hub_init(void)
{
    situation_driver_add(&camera_gesture_hub_init_info, ID_CAMERA_GESTURE);
	return 0;
}

static void __exit camera_gesture_hub_exit(void)
{
	pr_debug("%s\n", __func__);
}

module_init(camera_gesture_hub_init);
module_exit(camera_gesture_hub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CAMERA_GESTURE_HUB driver");
MODULE_AUTHOR("dingjiawang@huaqin.com");

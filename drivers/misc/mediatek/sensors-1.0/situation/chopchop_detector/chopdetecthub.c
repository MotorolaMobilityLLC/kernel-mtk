// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) "[chopdetecthub] " fmt

#include <hwmsensor.h>
#include "chopdetecthub.h"
#include <situation.h>
#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "include/scp.h"

static struct situation_init_info chopdetecthub_init_info;

static int chop_detect_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;

	err = sensor_get_data_from_hub(ID_CHOP_GESTURE, &data);
	if (err < 0) {
		pr_err("chop_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp		= data.time_stamp;
	*probability	= data.gesture_data_t.probability;
	return 0;
}
static int chop_detect_open_report_data(int open)
{
	int ret = 0;
#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_CHOP_GESTURE, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	ret = sensor_enable_to_hub(ID_CHOP_GESTURE, open);
	return ret;
}
static int chop_detect_batch(int flag,
	int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_CHOP_GESTURE,
		flag, samplingPeriodNs, maxBatchReportLatencyNs);
}

static int chop_detect_flush(void)
{
	return sensor_flush_to_hub(ID_CHOP_GESTURE);
}

static int chop_detect_recv_data(struct data_unit_t *event, void *reserved)
{
	int err = 0;

	if (event->flush_action == FLUSH_ACTION)
		err = situation_flush_report(ID_CHOP_GESTURE);
    else if (event->flush_action == DATA_ACTION) {
		err = situation_data_report_t(ID_CHOP_GESTURE,
			event->chop_event.state, (int64_t)event->time_stamp);
    }
	return err;
}

static int chopdetecthub_local_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = chop_detect_open_report_data;
	ctl.batch = chop_detect_batch;
	ctl.flush = chop_detect_flush;
	ctl.is_support_wake_lock = true;
	ctl.is_support_batch = false;
	err = situation_register_control_path(&ctl, ID_CHOP_GESTURE);
	if (err) {
		pr_err("register chop_detect control path err\n");
		goto exit;
	}

	data.get_data = chop_detect_get_data;
	err = situation_register_data_path(&data, ID_CHOP_GESTURE);
	if (err) {
		pr_err("register chop_detect data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_CHOP_GESTURE,
		chop_detect_recv_data);
	if (err) {
		pr_err("SCP_sensorHub_data_registration fail!!\n");
		goto exit;
	}
	return 0;
exit:
	return -1;
}
static int chopdetecthub_local_uninit(void)
{
	return 0;
}

static struct situation_init_info chopdetecthub_init_info = {
	.name = "chop_detect_hub",
	.init = chopdetecthub_local_init,
	.uninit = chopdetecthub_local_uninit,
};

static int __init chopdetecthub_init(void)
{
    situation_driver_add(&chopdetecthub_init_info, ID_CHOP_GESTURE);
	return 0;
}

static void __exit chopdetecthub_exit(void)
{
	pr_debug("%s\n", __func__);
}

module_init(chopdetecthub_init);
module_exit(chopdetecthub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CHOP_GESTURE_HUB driver");
MODULE_AUTHOR("dingjiawang@huaqin.com");

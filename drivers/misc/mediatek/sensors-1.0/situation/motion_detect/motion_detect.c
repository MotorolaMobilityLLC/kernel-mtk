/* stationary gesture sensor driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

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
#include "situation.h"
#include "motion_detect.h"
#include <hwmsen_helper.h>

#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "scp_helper.h"


#define MOTIONHUB_TAG                  "[motion_detect] "
#define MOTIONHUB_FUN(f)               pr_deubg(MOTIONHUB_TAG"%s\n", __func__)
#define MOTIONHUB_PR_ERR(fmt, args...)    pr_err(MOTIONHUB_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define MOTIONHUB_LOG(fmt, args...)    pr_debug(MOTIONHUB_TAG fmt, ##args)


static struct situation_init_info motion_detect_init_info;
static int motion_detect_get_data(int *probability, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;
	uint64_t time_stamp_gpt = 0;

	err = sensor_get_data_from_hub(ID_MOTION_DETECT, &data);
	if (err < 0) {
		MOTIONHUB_PR_ERR("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp		= data.time_stamp;
	time_stamp_gpt	= data.time_stamp_gpt;
	*probability	= data.gesture_data_t.probability;
	MOTIONHUB_LOG("recv ipi: timestamp: %lld, timestamp_gpt: %lld, probability: %d!\n", time_stamp, time_stamp_gpt,
		*probability);
	return 0;
}
static int motion_detect_open_report_data(int open)
{
	int ret = 0;

#if defined CONFIG_MTK_SCP_SENSORHUB_V1
	if (open == 1)
		ret = sensor_set_delay_to_hub(ID_MOTION_DETECT, 120);
#elif defined CONFIG_NANOHUB

#else

#endif
	MOTIONHUB_LOG("%s : type=%d, open=%d\n", __func__, ID_MOTION_DETECT, open);
	ret = sensor_enable_to_hub(ID_MOTION_DETECT, open);
	return ret;
}
static int motion_detect_batch(int flag, int64_t samplingPeriodNs, int64_t maxBatchReportLatencyNs)
{
	return sensor_batch_to_hub(ID_MOTION_DETECT, flag, samplingPeriodNs, maxBatchReportLatencyNs);
}
static int motion_detect_recv_data(struct data_unit_t *event, void *reserved)
{
	if (event->flush_action == FLUSH_ACTION)
		MOTIONHUB_LOG("stat do not support flush\n");
	else if (event->flush_action == DATA_ACTION)
		situation_notify(ID_MOTION_DETECT);
	return 0;
}

static int motion_detect_local_init(void)
{
	struct situation_control_path ctl = {0};
	struct situation_data_path data = {0};
	int err = 0;

	ctl.open_report_data = motion_detect_open_report_data;
	ctl.batch = motion_detect_batch;
	ctl.is_support_wake_lock = true;
	err = situation_register_control_path(&ctl, ID_MOTION_DETECT);
	if (err) {
		MOTIONHUB_PR_ERR("register stationary control path err\n");
		goto exit;
	}

	data.get_data = motion_detect_get_data;
	err = situation_register_data_path(&data, ID_MOTION_DETECT);
	if (err) {
		MOTIONHUB_PR_ERR("register stationary data path err\n");
		goto exit;
	}
	err = scp_sensorHub_data_registration(ID_MOTION_DETECT, motion_detect_recv_data);
	if (err) {
		MOTIONHUB_PR_ERR("SCP_sensorHub_data_registration fail!!\n");
		goto exit_create_attr_failed;
	}
	return 0;
exit:
exit_create_attr_failed:
	return -1;
}
static int motion_detect_local_uninit(void)
{
	return 0;
}

static struct situation_init_info motion_detect_init_info = {
	.name = "motion_detect_hub",
	.init = motion_detect_local_init,
	.uninit = motion_detect_local_uninit,
};

static int __init motion_detect_init(void)
{
	situation_driver_add(&motion_detect_init_info, ID_MOTION_DETECT);
	return 0;
}

static void __exit motion_detect_exit(void)
{
	MOTIONHUB_FUN();
}

module_init(motion_detect_init);
module_exit(motion_detect_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Stationary Gesture driver");
MODULE_AUTHOR("qiangming.xia@mediatek.com");

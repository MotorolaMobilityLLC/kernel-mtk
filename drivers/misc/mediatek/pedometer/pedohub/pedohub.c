/* pedohub motion sensor driver
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

#include <hwmsensor.h>
#include "pedohub.h"
#include <pedometer.h>
#include <SCP_sensorHub.h>
#include <linux/notifier.h>
#include "scp_helper.h"

#define PEDOHUB_TAG                  "[pedohub] "
#define PEDOHUB_FUN(f)               pr_err(PEDOHUB_TAG"%s\n", __func__)
#define PEDOHUB_ERR(fmt, args...)    pr_err(PEDOHUB_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define PEDOHUB_LOG(fmt, args...)    pr_err(PEDOHUB_TAG fmt, ##args)

typedef enum {
	PEDOHUB_TRC_INFO = 0X10,
} PEDOHHUB_TRC;

static struct pedo_init_info pedohub_init_info;

struct pedohub_ipi_data {
	atomic_t trace;
	atomic_t suspend;
};

static struct pedohub_ipi_data obj_ipi_data;

static ssize_t show_pedometer_value(struct device_driver *ddri, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", buf);
}

static ssize_t store_trace_value(struct device_driver *ddri, const char *buf, size_t count)
{
	struct pedohub_ipi_data *obj = &obj_ipi_data;
	int trace = 0;

	if (obj == NULL) {
		PEDOHUB_ERR("obj is null!!\n");
		return 0;
	}

	if (1 == sscanf(buf, "0x%x", &trace)) {
		atomic_set(&obj->trace, trace);
	} else {
		PEDOHUB_ERR("invalid content: '%s', length = %zu\n", buf, count);
		return 0;
	}
	return count;
}

static DRIVER_ATTR(pedometer, S_IRUGO, show_pedometer_value, NULL);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, NULL, store_trace_value);

static struct driver_attribute *pedohub_attr_list[] = {
	&driver_attr_pedometer,
	&driver_attr_trace,
};

static int pedohub_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(pedohub_attr_list) / sizeof(pedohub_attr_list[0]));


	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, pedohub_attr_list[idx]);
		if (0 != err) {
			PEDOHUB_ERR("driver_create_file (%s) = %d\n",
				    pedohub_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

static int pedohub_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(pedohub_attr_list) / sizeof(pedohub_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, pedohub_attr_list[idx]);

	return err;
}

static int pedometer_get_data(u32 *value, int *status)
{
	int err = 0;
	struct data_unit_t data;
	uint64_t time_stamp = 0;
	uint64_t time_stamp_gpt = 0;

	err = sensor_get_data_from_hub(ID_PEDOMETER, &data);
	if (err < 0) {
		PEDOHUB_ERR("sensor_get_data_from_hub fail!!\n");
		return -1;
	}
	time_stamp = data.time_stamp;
	time_stamp_gpt = data.time_stamp_gpt;
	value[0] = data.pedometer_t.accumulated_step_count;
	value[1] = data.pedometer_t.accumulated_step_length;
	value[2] = data.pedometer_t.step_frequency;
	value[3] = data.pedometer_t.step_length;
	PEDOHUB_LOG
	    ("recv ipi: timestamp: %lld, timestamp_gpt: %lld, count: %d, length: %d, freq: %d, length: %d!\n",
	     time_stamp, time_stamp_gpt, value[0], value[1], value[2], value[3]);
	return err;
}

static int pedometer_open_report_data(int open)
{
	return 0;
}

static int pedometer_enable_nodata(int en)
{
	return sensor_enable_to_hub(ID_PEDOMETER, en);
}

static int pedometer_set_delay(u64 delay)
{
	unsigned int delayms = 0;

	delayms = delay / 1000 / 1000;
	return sensor_set_delay_to_hub(ID_PEDOMETER, delayms);
}

static int pedohub_local_init(void)
{
	struct pedo_control_path ctl = { 0 };
	struct pedo_data_path data = { 0 };
	int err = 0;

	err = pedohub_create_attr(&pedohub_init_info.platform_diver_addr->driver);
	if (err) {
		PEDOHUB_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}
	ctl.open_report_data = pedometer_open_report_data;
	ctl.enable_nodata = pedometer_enable_nodata;
	ctl.set_delay = pedometer_set_delay;
	ctl.is_report_input_direct = false;
	ctl.is_support_batch = true;
	err = pedo_register_control_path(&ctl);
	if (err) {
		PEDOHUB_ERR("register pedometer control path err\n");
		goto exit;
	}

	data.get_data = pedometer_get_data;
	err = pedo_register_data_path(&data);
	if (err) {
		PEDOHUB_ERR("register pedometer data path err\n");
		goto exit;
	}
	return 0;
exit:
	pedohub_delete_attr(&(pedohub_init_info.platform_diver_addr->driver));
exit_create_attr_failed:
	return -1;
}

static int pedohub_local_uninit(void)
{
	return 0;
}

static struct pedo_init_info pedohub_init_info = {
	.name = "pedometer_hub",
	.init = pedohub_local_init,
	.uninit = pedohub_local_uninit,
};

static int __init pedohub_init(void)
{
	pedo_driver_add(&pedohub_init_info);
	return 0;
}

static void __exit pedohub_exit(void)
{
	PEDOHUB_FUN();
}

module_init(pedohub_init);
module_exit(pedohub_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ACTIVITYHUB driver");
MODULE_AUTHOR("hongxu.zhao@mediatek.com");

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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/cpumask.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "mcsi_b_tracer_interface.h"

static const struct of_device_id mcsi_b_tracer_of_ids[] = {
	{}
};

static int mcsi_b_tracer_probe(struct platform_device *pdev);
static int mcsi_b_tracer_remove(struct platform_device *pdev);
static int mcsi_b_tracer_suspend(struct platform_device *pdev, pm_message_t state);
static int mcsi_b_tracer_resume(struct platform_device *pdev);

static char *mcsi_b_tracer_dump_buf;

static struct mcsi_b_tracer mcsi_b_tracer_drv = {
	.plt_drv = {
		.driver = {
			.name = "mcsi_b_tracer",
			.bus = &platform_bus_type,
			.owner = THIS_MODULE,
			.of_match_table = mcsi_b_tracer_of_ids,
		},
		.probe = mcsi_b_tracer_probe,
		.remove = mcsi_b_tracer_remove,
		.suspend = mcsi_b_tracer_suspend,
		.resume = mcsi_b_tracer_resume,
	},
};


static int mcsi_b_tracer_probe(struct platform_device *pdev)
{
	struct mcsi_b_tracer_plt *plt = NULL;

	pr_debug("%s:%d: enter\n", __func__, __LINE__);

	plt = mcsi_b_tracer_drv.cur_plt;

	if (plt && plt->ops && plt->ops->probe)
		return plt->ops->probe(plt, pdev);

	return 0;
}

static int mcsi_b_tracer_remove(struct platform_device *pdev)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	pr_debug("%s:%d: enter\n", __func__, __LINE__);

	if (plt && plt->ops && plt->ops->remove)
		return plt->ops->remove(plt, pdev);

	return 0;
}

static int mcsi_b_tracer_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	pr_debug("%s:%d: enter\n", __func__, __LINE__);

	if (plt && plt->ops && plt->ops->suspend)
		return plt->ops->suspend(plt, pdev, state);

	return 0;
}

static int mcsi_b_tracer_resume(struct platform_device *pdev)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	pr_debug("%s:%d: enter\n", __func__, __LINE__);

	if (plt && plt->ops && plt->ops->resume)
		return plt->ops->resume(plt, pdev);

	return 0;
}

int mcsi_b_tracer_register(struct mcsi_b_tracer_plt *plt)
{
	if (!plt) {
		pr_warn("%s%d: plt is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	plt->common = &mcsi_b_tracer_drv;
	mcsi_b_tracer_drv.cur_plt = plt;

	return 0;
}

int mcsi_b_tracer_dump(char *buf, int len)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!buf) {
		pr_warn("%s:%d: buf is NULL\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (!plt) {
		pr_warn("%s:%d: plt is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (!plt->common) {
		pr_warn("%s:%d: plt->common is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (plt->ops && plt->ops->dump)
		return plt->ops->dump(plt, buf, len);

	pr_warn("no dump function implemented\n");

	return 0;
}

int mcsi_b_tracer_enable(unsigned char force, unsigned char enable)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!plt) {
		pr_warn("%s:%d: plt is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (plt->ops && plt->ops->setup)
		return plt->ops->setup(plt, force, enable);

	pr_warn("no enable function implemented\n");

	return 0;
}

int mcsi_b_tracer_disable(void)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!plt) {
		pr_warn("%s:%d: plt is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (plt->ops && plt->ops->disable)
		return plt->ops->disable(plt);

	pr_warn("no disable function implemented\n");

	return 0;
}

int mcsi_b_tracer_dump_setting(char *buf, int len)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!plt) {
		pr_warn("%s:%d: plt is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (plt->ops && plt->ops->dump_setting)
		return plt->ops->dump_setting(plt, buf, len);

	pr_warn("no dump_setting function implemented\n");

	return 0;
}

int mcsi_b_tracer_dump_min_len(void)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!plt) {
		pr_warn("%s:%d: plt is NULL\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (!plt->min_buf_len)
		pr_warn("%s:%d: min_buf_len is 0\n", __func__, __LINE__);

	return plt->min_buf_len;
}

int mt_mcsi_b_tracer_dump(char *buf)
{
	strncpy(buf, mcsi_b_tracer_dump_buf, strlen(mcsi_b_tracer_dump_buf)+1);
	return 0;
}

/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/dump_to_buf
 * dump traces to internal buffer
 */
static ssize_t mcsi_b_tracer_dump_to_buf_show(struct device_driver *driver, char *buf)
{
	if (!mcsi_b_tracer_dump_buf)
		mcsi_b_tracer_dump_buf = kzalloc(mcsi_b_tracer_drv.cur_plt->min_buf_len, GFP_KERNEL);

	if (!mcsi_b_tracer_dump_buf)
		return -ENOMEM;

	if (mcsi_b_tracer_drv.cur_plt)
		mcsi_b_tracer_dump(mcsi_b_tracer_dump_buf, mcsi_b_tracer_drv.cur_plt->min_buf_len);

	return snprintf(buf, PAGE_SIZE, "copy trace to internal buffer..\n"
			"using command \"cat /proc/mcsi_b_tracer/dump_db\" to see the trace\n");
}

static ssize_t mcsi_b_tracer_dump_to_buf_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}

static DRIVER_ATTR(dump_to_buf, 0664, mcsi_b_tracer_dump_to_buf_show, mcsi_b_tracer_dump_to_buf_store);

/*
 * interface: /proc/bus_trcaer/dump_db
 * AEE read this seq_file to get the content of internal buffer and generate db
 */
static int mcsi_b_tracer_dump_db_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", mcsi_b_tracer_dump_buf);
	return 0;
}

static int mcsi_b_tracer_dump_db_open(struct inode *inode, struct file *file)
{
	return single_open(file, mcsi_b_tracer_dump_db_show, PDE_DATA(inode));
}

static int mcsi_b_tracer_dump_db_release(struct inode *inode, struct file *file)
{
	/* release buffer after dumping to db */
	kfree(mcsi_b_tracer_dump_buf);
	mcsi_b_tracer_dump_buf = NULL;

	return single_release(inode, file);
}

static const struct file_operations mcsi_b_tracer_dump_db_proc_fops = {
	.open		= mcsi_b_tracer_dump_db_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= mcsi_b_tracer_dump_db_release,
};

/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/set_recording
 * resume/pause recording
 */
static ssize_t mcsi_b_tracer_set_recording_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Usage:\n"
			"echo $recording > /sys/bus/platform/drivers/mcsi_b_tracer/set_recording\n"
			"(0 for pause; 1 for resume)");
}

static ssize_t mcsi_b_tracer_set_recording_store(struct device_driver *driver, const char *buf, size_t count)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;
	unsigned long input = 0;
	unsigned char to_pause = 0;
	int ret = -1, i = 0;
	char *p = (char *)buf, *arg;

	while ((arg = strsep(&p, " ")) && (i <= 0)) {
		if (kstrtoul(arg, 16, &input) != 0) {
			pr_err("%s:%d: kstrtoul fail for %s\n", __func__, __LINE__, p);
			return 0;
		}
		switch (i) {
		case 0:
			to_pause = (input == 0) ? 1 : 0;
			break;
		default:
			break;
		}
		i++;
	}

	if (i < 1) {
		pr_err("%s:%d: too few arguments\n", __func__, __LINE__);
		return count;
	}

	if (plt && plt->ops) {
		ret = plt->ops->set_recording(plt, to_pause);

		if (ret)
			pr_err("%s:%d: recording failed\n", __func__, __LINE__);
	}

	return count;
}

static DRIVER_ATTR(set_recording, 0664, mcsi_b_tracer_set_recording_show, mcsi_b_tracer_set_recording_store);


/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/set_filter
 * setup filter
 */
static ssize_t mcsi_b_tracer_set_filter_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Usage:\n"
			"echo $low_content $high_content $low_content_enable $high_content_enable\n"
			"> /sys/bus/platform/drivers/mcsi_b_tracer/set_filter\n");
}

static ssize_t mcsi_b_tracer_set_filter_store(struct device_driver *driver, const char *buf, size_t count)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;
	struct mcsi_trace_filter f = {.low_content = 0, .high_content = 0,
		.low_content_enable = 0, .high_content_enable = 0};
	unsigned long input = 0;
	int ret = -1, i = 0;
	char *p = (char *)buf, *arg;

	while ((arg = strsep(&p, " ")) && (i <= 3)) {
		if (kstrtoul(arg, 16, &input) != 0) {
			pr_err("%s:%d: kstrtoul fail for %s\n", __func__, __LINE__, p);
			return 0;
		}
		switch (i) {
		case 0:
			f.low_content = input;
			break;
		case 1:
			f.high_content = input;
			break;
		case 2:
			f.low_content_enable = input;
			break;
		case 3:
			f.high_content_enable = input;
			break;
		default:
			break;
		}
		i++;
	}

	if (i <= 3) {
		pr_err("%s:%d: too few arguments\n", __func__, __LINE__);
		return count;
	}

	if (plt && plt->ops) {
		ret = plt->ops->set_filter(plt, f);

		if (ret)
			pr_err("%s:%d: recording failed\n", __func__, __LINE__);
	}

	return count;
}

static DRIVER_ATTR(set_filter, 0664, mcsi_b_tracer_set_filter_show, mcsi_b_tracer_set_filter_store);

/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/set_port
 * setup trace port
 */
static ssize_t mcsi_b_tracer_set_port_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE,
		"Usage:\necho $tp_sel $tp_num > /sys/bus/platform/drivers/mcsi_b_tracer/set_trigger\n");
}

static ssize_t mcsi_b_tracer_set_port_store(struct device_driver *driver, const char *buf, size_t count)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;
	struct mcsi_trace_port port;
	unsigned long input = 0;
	int ret = -1, i = 0;
	char *p = (char *)buf, *arg;

	while ((arg = strsep(&p, " ")) && (i <= 1)) {
		if (kstrtoul(arg, 16, &input) != 0) {
			pr_err("%s:%d: kstrtoul fail for %s\n", __func__, __LINE__, p);
			return 0;
		}
		switch (i) {
		case 0:
			port.tp_sel = input;
			break;
		case 1:
			port.tp_num = input;
			break;
		default:
			break;
		}
		i++;
	}

	if (i <= 1) {
		pr_err("%s:%d: too few arguments\n", __func__, __LINE__);
		return count;
	}

	if (plt && plt->ops) {
		ret = plt->ops->set_port(plt, port);

		if (ret)
			pr_err("%s:%d: recording failed\n", __func__, __LINE__);
	}

	return count;
}

static DRIVER_ATTR(set_port, 0664, mcsi_b_tracer_set_port_show, mcsi_b_tracer_set_port_store);


/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/set_trigger
 * setup trigger point
 */
static ssize_t mcsi_b_tracer_set_trigger_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "Usage:\n"
			"echo $cfg_trigger_ctrl0 $cfg_trigger_ctrl1 $lo_addr $hi_addr\n"
			"$lo_addr_mask $hi_addr_mask $other_match $other_match_mask\n"
			"> /sys/bus/platform/drivers/mcsi_b_tracer/set_trigger\n");
}

static ssize_t mcsi_b_tracer_set_trigger_store(struct device_driver *driver, const char *buf, size_t count)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;
	struct mcsi_trace_trigger_point trigger = {.cfg_trigger_ctrl0 = 0,
		.cfg_trigger_ctrl1 = 0, .tri_lo_addr = 0, .tri_hi_addr = 0,
		.tri_lo_addr_mask = 0, .tri_hi_addr_mask = 0, .tri_other_match = 0,
		.tri_other_match_mask = 0};
	unsigned long input = 0;
	int ret = -1, i = 0;
	char *p = (char *)buf, *arg;

	while ((arg = strsep(&p, " ")) && (i <= 7)) {
		if (kstrtoul(arg, 16, &input) != 0) {
			pr_err("%s:%d: kstrtoul fail for %s\n", __func__, __LINE__, p);
			return 0;
		}
		switch (i) {
		case 0:
			trigger.cfg_trigger_ctrl0 = input;
			break;
		case 1:
			trigger.cfg_trigger_ctrl1 = input;
			break;
		case 2:
			trigger.tri_lo_addr = input;
			break;
		case 3:
			trigger.tri_hi_addr = input;
			break;
		case 4:
			trigger.tri_lo_addr_mask = input;
			break;
		case 5:
			trigger.tri_hi_addr_mask = input;
			break;
		case 6:
			trigger.tri_other_match = input;
			break;
		case 7:
			trigger.tri_other_match_mask = input;
			break;
		default:
			break;
		}
		i++;
	}

	if (i <= 7) {
		pr_err("%s:%d: too few arguments\n", __func__, __LINE__);
		return count;
	}

	if (plt && plt->ops) {
		ret = plt->ops->set_trigger(plt, trigger);

		if (ret)
			pr_err("%s:%d: recording failed\n", __func__, __LINE__);
	}

	return count;
}

static DRIVER_ATTR(set_trigger, 0664, mcsi_b_tracer_set_trigger_show, mcsi_b_tracer_set_trigger_store);


/*
 * interface: /sys/bus/platform/drivers/mcsi_b_tracer/dump_setting
 * dump current setting of bus tracers
 */
static ssize_t dump_setting_show(struct device_driver *driver, char *buf)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;
	int ret = 0;

	ret = plt->ops->dump_setting(plt, buf, -1);
	if (ret)
		pr_err("%s:%d: dump failed\n", __func__, __LINE__);

	return strlen(buf);
}

static ssize_t dump_setting_store(struct device_driver *driver, const char *buf, size_t count)
{
	return count;
}

static DRIVER_ATTR(dump_setting, 0664, dump_setting_show, dump_setting_store);

static int mcsi_b_tracer_start(void)
{
	struct mcsi_b_tracer_plt *plt = mcsi_b_tracer_drv.cur_plt;

	if (!plt)
		return -ENODEV;

	if (!plt->ops) {
		pr_err("%s:%d: ops not installed\n", __func__, __LINE__);
		return -ENODEV;
	}

	if (plt->ops->start)
		return plt->ops->start(plt);

	return 0;
}

static int __init mcsi_b_tracer_init(void)
{
	int ret;
	static struct proc_dir_entry *root_dir;

	ret = mcsi_b_tracer_start();
	if (ret) {
		pr_err("%s:%d: mcsi_b_tracer_start failed\n", __func__, __LINE__);
		return -ENODEV;
	}

	/* since kernel already populates dts, our probe would be callback after this registration */
	ret = platform_driver_register(&mcsi_b_tracer_drv.plt_drv);
	if (ret) {
		pr_err("%s:%d: platform_driver_register failed\n", __func__, __LINE__);
		return -ENODEV;
	}

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_dump_setting);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_dump_to_buf);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_set_trigger);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_set_port);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_set_filter);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	ret = driver_create_file(&mcsi_b_tracer_drv.plt_drv.driver, &driver_attr_set_recording);
	if (ret)
		pr_err("%s:%d: driver_create_file failed.\n", __func__, __LINE__);

	/* create /proc/mcsi_b_tracer */
	root_dir = proc_mkdir("mcsi_b_tracer", NULL);
	if (!root_dir)
		return -EINVAL;

	/* create /proc/mcsi_b_tracer/dump_db */
	proc_create("dump_db", 0644, root_dir, &mcsi_b_tracer_dump_db_proc_fops);

	mcsi_b_tracer_dump_buf = kzalloc(mcsi_b_tracer_drv.cur_plt->min_buf_len, GFP_KERNEL);
	if (!mcsi_b_tracer_dump_buf)
		return -ENOMEM;

	mcsi_b_tracer_dump(mcsi_b_tracer_dump_buf, mcsi_b_tracer_drv.cur_plt->min_buf_len);

	/*
	 * mcsi_b_tracer_enable($force, $enable)
	 * $force=0: enable only if plt->enabled
	 * $force=1: enable if $enable, disable otherwise
	 */
	mcsi_b_tracer_enable(1, 1);

	return 0;
}

postcore_initcall(mcsi_b_tracer_init);

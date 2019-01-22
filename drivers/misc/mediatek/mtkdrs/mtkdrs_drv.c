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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <mt-plat/mtk_meminfo.h>
#include <mach/emi_mpu.h>

static ssize_t mtkdrs_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

/*
 * mtkdrs_debug_show
 * debug entry for debug system, do not wait for any lock here
 */
static ssize_t mtkdrs_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int n = 0;
	unsigned long long cnt;
	int ch;

	for (ch = 0; ch < 4; ch++) {
		cnt = get_drs_all_self_cnt(ch);
		n += snprintf(buf + n, PAGE_SIZE - n, "get_drs_all_self_cnt %d: %llu ps\n", ch, cnt);
	}
	for (ch = 0; ch < 4; ch++) {
		cnt = get_drs_rank1_self_cnt(ch);
		n += snprintf(buf + n, PAGE_SIZE - n, "get_drs_rank1_self_cnt %d: %llu ps\n", ch, cnt);
	}
	for (ch = 0; ch < 4; ch++) {
		cnt = get_drs_rank_prd(ch);
		n += snprintf(buf + n, PAGE_SIZE - n, "get_drs_rank_prd %d: %llu\n ps", ch, cnt);
	}

	return n;
}

static DEVICE_ATTR(status, S_IRUGO, mtkdrs_status_show, NULL);
static DEVICE_ATTR(debug, S_IRUGO, mtkdrs_debug_show, NULL);

static struct attribute *mtkdrs_attrs[] = {
	&dev_attr_status.attr,
	&dev_attr_debug.attr,
	NULL,
};

struct attribute_group mtkdrs_attr_group = {
	.attrs = mtkdrs_attrs,
	.name = "mtkdrs",
};

static int __init mtkdrs_init(void)
{
	int ret;

	ret = sysfs_create_group(power_kobj, &mtkdrs_attr_group);
	if (ret) {
		pr_warn("create sysfs mtkdrs group failed\n");
		return ret;
	}

	return ret;
}

static void __exit mtkdrs_exit(void) { }

module_init(mtkdrs_init);
module_exit(mtkdrs_exit);

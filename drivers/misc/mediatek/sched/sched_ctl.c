/*
 * Copyright (C) 2017 MediaTek Inc.
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

/*
 * Support asynchronus hint for external modules to get loading change
 * in time from scheduler's help.
 *
 * 1. call-back function for notification of status change
 *   - int register_sched_hint_notifier( void(*fp)(int status) )
 *
 * 2. control interface for user
 *   - /sys/devices/system/cpu/sched/...
 *
 */

#include <linux/irq_work.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/string.h>
#include <trace/events/sched.h>
#include <linux/cpu.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <mt-plat/met_drv.h>

struct sched_hint_data {
	struct attribute_group *attr_group;
	struct kobject *kobj;
};

/* global */
static struct sched_hint_data g_shd;
static struct kobj_attribute sched_iso_attr;
static struct kobj_attribute set_sched_iso_attr;
static struct kobj_attribute set_sched_deiso_attr;

static struct attribute *sched_attrs[] = {
	&sched_iso_attr.attr,
	&set_sched_iso_attr.attr,
	&set_sched_deiso_attr.attr,
	NULL,
};

static struct attribute_group sched_attr_group = {
	.attrs = sched_attrs,
};

/* turn on/off sched boost scheduling */
static ssize_t show_sched_iso(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf+len, max_len-len, "cpu_isolated_mask=0x%lx\n",
			cpu_isolated_mask->bits[0]);
	len += snprintf(buf+len, max_len-len, "iso_prio=%d\n", iso_prio);

	return len;
}

static ssize_t set_sched_iso(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) < nr_cpu_ids)
		sched_isolate_cpu(val);

	return count;
}

static ssize_t set_sched_deiso(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) < nr_cpu_ids)
		sched_deisolate_cpu(val);

	return count;
}

/* init function */
static int __init sched_hint_init(void)
{
	int ret;

	/*
	 * create a sched in cpu_subsys:
	 * /sys/devices/system/cpu/sched/...
	 */
	g_shd.attr_group = &sched_attr_group;
	g_shd.kobj =
	kobject_create_and_add("sched", &cpu_subsys.dev_root->kobj);

	if (g_shd.kobj) {
		ret = sysfs_create_group(g_shd.kobj, g_shd.attr_group);
		if (ret)
			kobject_put(g_shd.kobj);
		else
			kobject_uevent(g_shd.kobj, KOBJ_ADD);
	}

	return 0;
}
late_initcall(sched_hint_init);

static struct kobj_attribute sched_iso_attr =
__ATTR(sched_isolation, 0400, show_sched_iso, NULL);

static struct kobj_attribute set_sched_iso_attr =
__ATTR(set_sched_isolation, 0200, NULL, set_sched_iso);

static struct kobj_attribute set_sched_deiso_attr =
__ATTR(set_sched_deisolation, 0200, NULL, set_sched_deiso);

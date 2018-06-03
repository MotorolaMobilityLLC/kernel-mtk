/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/pm_qos.h>
#include <linux/sysfs.h>

#include <helio-dvfsrc.h>
#include <helio-dvfsrc-opp.h>

static struct pm_qos_request dvfsrc_emi_request;
static struct pm_qos_request dvfsrc_vcore_request;


static ssize_t dvfsrc_enable_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct helio_dvfsrc *dvfsrc;
	int err = 0;

	dvfsrc = dev_get_drvdata(dev);

	if (!dvfsrc)
		return sprintf(buf, "Failed to access dvfsrc\n");

	mutex_lock(&dvfsrc->devfreq->lock);
	err = sprintf(buf, "%d\n", dvfsrc->enabled);
	mutex_unlock(&dvfsrc->devfreq->lock);

	return err;
}

static ssize_t dvfsrc_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct helio_dvfsrc *dvfsrc;
	unsigned int enable = 0;

	dvfsrc = dev_get_drvdata(dev);

	if (!dvfsrc)
		return -ENODEV;

	mutex_lock(&dvfsrc->devfreq->lock);

	if (kstrtouint(buf, 10, &enable))
		return -EINVAL;

	if (enable > 1)
		return -EINVAL;

	helio_dvfsrc_enable(enable);

	mutex_unlock(&dvfsrc->devfreq->lock);

	return count;
}
static DEVICE_ATTR(dvfsrc_enable, 0644,
		dvfsrc_enable_show, dvfsrc_enable_store);

static ssize_t dvfsrc_dump_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct helio_dvfsrc *dvfsrc;
	char *p = buf;
	char *buff_end = p + PAGE_SIZE;
	int vcore_uv = get_cur_vcore_uv();
	int ddr_khz = get_cur_ddr_khz();

	dvfsrc = dev_get_drvdata(dev);

	if (!dvfsrc)
		return sprintf(buf, "Failed to access dvfsrc\n");

	p += snprintf(p, buff_end - p, "[%-12s]: %-8u (0x%x)\n",
			"vcore_uv", vcore_uv, vcore_uv_to_pmic(vcore_uv));
	p += snprintf(p, buff_end - p, "[%-12s]: %-8u\n",
			"ddr_khz", ddr_khz);

	p += snprintf(p, buff_end - p, "\n");

	return p - buf;
}

static DEVICE_ATTR(dvfsrc_dump, 0444, dvfsrc_dump_show, NULL);

/* ToDo: Add sysfs */
static struct attribute *helio_dvfsrc_attrs[] = {
	&dev_attr_dvfsrc_enable.attr,
	&dev_attr_dvfsrc_dump.attr,
	NULL,
};

static struct attribute_group helio_dvfsrc_attr_group = {
	.name = "helio-dvfsrc",
	.attrs = helio_dvfsrc_attrs,
};

int helio_dvfsrc_add_interface(struct device *dev)
{
	pm_qos_add_request(&dvfsrc_emi_request, PM_QOS_DDR_OPP,
			PM_QOS_DDR_OPP_DEFAULT_VALUE);
	pm_qos_add_request(&dvfsrc_vcore_request, PM_QOS_VCORE_OPP,
			PM_QOS_VCORE_OPP_DEFAULT_VALUE);

	return sysfs_create_group(&dev->kobj, &helio_dvfsrc_attr_group);
}

void helio_dvfsrc_remove_interface(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &helio_dvfsrc_attr_group);
}

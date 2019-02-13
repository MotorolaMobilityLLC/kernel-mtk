/*
 * Copyright (C) 2016 Motorola Mobility LLC
 * Copyright (C) 2018 Motorola Mobility LLC
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include "mmi_info.h"

struct mmi_storage_info *info;

SYSFS_SIMPLE_SHOW(fw, info->firmware_version, "%s", 32)
SYSFS_SIMPLE_SHOW(model, info->product_name, "%s", 32)
SYSFS_SIMPLE_SHOW(vendor, info->card_manufacturer, "%s", 32)
SYSFS_SIMPLE_SHOW(size, info->size, "%s", 16)
SYSFS_SIMPLE_SHOW(type, info->type, "%s", 16)

SYSFS_ATTRIBUTE(fw, 0444)
SYSFS_ATTRIBUTE(model, 0444)
SYSFS_ATTRIBUTE(vendor, 0444)
SYSFS_ATTRIBUTE(size, 0444)
SYSFS_ATTRIBUTE(type, 0444)

static struct attribute *st_info_properties_attrs[] = {
	&fw_attr.attr,
	&model_attr.attr,
	&vendor_attr.attr,
	&size_attr.attr,
	&type_attr.attr,
	NULL
};

static struct attribute_group st_info_properties_attr_group = {
	.attrs = st_info_properties_attrs,
};

static struct kobject *st_info_properties_kobj;

int mmi_storage_info_init(void)
{
	int ret = 0;
	int status = 0;
	struct property *p;
	struct device_node *n;

	n = of_find_node_by_path("/chosen/mmi,storage");
	if (n == NULL) {
		ret = 1;
		goto err;
	}

	info = kzalloc(sizeof(struct mmi_storage_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s: failed to allocate space for mmi_storage_info\n",
			__func__);
		ret = 1;
		goto err;
	}

	for_each_property_of_node(n, p) {
		if (!strcmp(p->name, "type") && p->value)
			strlcpy(info->type, (char *)p->value,
				sizeof(info->type));
		if (!strcmp(p->name, "size") && p->value)
			strlcpy(info->size, (char *)p->value,
				sizeof(info->size));
		if (!strcmp(p->name, "manufacturer") && p->value)
			strlcpy(info->card_manufacturer, (char *)p->value,
				sizeof(info->card_manufacturer));
		if (!strcmp(p->name, "product") && p->value)
			strlcpy(info->product_name, (char *)p->value,
				sizeof(info->product_name));
		if (!strcmp(p->name, "firmware") && p->value)
			strlcpy(info->firmware_version, (char *)p->value,
				sizeof(info->firmware_version));
	}

	of_node_put(n);

	pr_info("mmi_storage_info :%s: %s %s %s FV=%s\n",
		info->type,
		info->size,
		info->card_manufacturer,
		info->product_name,
		info->firmware_version);

	mmi_annotate_persist(
		"%s: %s %s %s FV=%s\n",
		info->type,
		info->size,
		info->card_manufacturer,
		info->product_name,
		info->firmware_version);

	/* Export to sysfs*/
	st_info_properties_kobj = kobject_create_and_add("storage", NULL);
	if (st_info_properties_kobj)
		status = sysfs_create_group(st_info_properties_kobj,
				&st_info_properties_attr_group);

	if (!st_info_properties_kobj || status) {
		pr_err("%s: failed to create /sys/storage\n", __func__);
		ret = 1;
		goto err;
	}

err:
	return ret;
}

void mmi_storage_info_exit(void)
{
	kfree(info);
	info = NULL;

	sysfs_remove_group(st_info_properties_kobj,
			&st_info_properties_attr_group);
	kobject_del(st_info_properties_kobj);
}

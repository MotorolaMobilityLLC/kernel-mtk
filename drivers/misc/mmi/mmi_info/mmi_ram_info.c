/*
 * Copyright (C) 2012 Motorola Mobility LLC
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
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include "mmi_info.h"

/** DDR types. */
enum MOT_DDR_TYPE
{
	MOT_DDR_TYPE_LPDDR1 = 0,	/**< Low power DDR1. */
	MOT_DDR_TYPE_LPDDR2 = 2,	/**< Low power DDR2, set to 2 for compatibility*/
	MOT_DDR_TYPE_PCDDR2 = 3,	/**< Personal computer DDR2. */
	MOT_DDR_TYPE_PCDDR3 = 4,	/**< Personal computer DDR3. */
	MOT_DDR_TYPE_LPDDR3 = 5,	/**< Low power DDR3. */
	MOT_DDR_TYPE_LPDDR4 = 6,	/**< Low power DDR4. */
	MOT_DDR_TYPE_RESERVED = 7,	/**< Reserved for future use. */
	MOT_DDR_TYPE_UNUSED = 0x7FFFFFFF
};


struct mmi_ddr_info *ddr_info;

static char sysfsram_type_name[20] = "unknown";
static char sysfsram_vendor_name[20] = "unknown";
static uint32_t sysfsram_ramsize;

static ssize_t mr_register_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	uint32_t val = 0;
	const char *name = attr->attr.name;

	if (ddr_info != NULL &&
		strnlen(name, 4) == 3 && name[0] == 'm' && name[1] == 'r') {
		switch (name[2]) {
		case '5':
			val = ddr_info->mr5;
			break;
		case '6':
			val = ddr_info->mr6;
			break;
		case '7':
			val = ddr_info->mr7;
			break;
		case '8':
			val = ddr_info->mr8;
			break;
		}
	}

	return snprintf(buf, 8, "0x%02x\n", val);
}

static ssize_t info_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 60, "%s:%s:%uMB\n",
			sysfsram_vendor_name,
			sysfsram_type_name,
			sysfsram_ramsize);
}

SYSFS_SIMPLE_SHOW(size, sysfsram_ramsize, "%u", 12)
SYSFS_SIMPLE_SHOW(type, sysfsram_type_name, "%s", 20)

SYSFS_ATTRIBUTE(size, 0444)
SYSFS_ATTRIBUTE(type, 0444)
SYSFS_ATTRIBUTE(info, 0444)

static struct kobj_attribute mr5_register_attr =
	__ATTR(mr5, 0444, mr_register_show, NULL);

static struct kobj_attribute mr6_register_attr =
	__ATTR(mr6, 0444, mr_register_show, NULL);

static struct kobj_attribute mr7_register_attr =
	__ATTR(mr7, 0444, mr_register_show, NULL);

static struct kobj_attribute mr8_register_attr =
	__ATTR(mr8, 0444, mr_register_show, NULL);

static struct attribute *ram_info_properties_attrs[] = {
	&mr5_register_attr.attr,
	&mr6_register_attr.attr,
	&mr7_register_attr.attr,
	&mr8_register_attr.attr,
	&size_attr.attr,
	&type_attr.attr,
	&info_attr.attr,
	NULL
};

static struct attribute_group ram_info_properties_attr_group = {
	.attrs = ram_info_properties_attrs,
};

static struct kobject *ram_info_properties_kobj;

int mmi_ram_info_init(void)
{
	int ret = 0;
	int status = 0;
	uint32_t vid, tid;
	struct device_node *n;
	const char *tname = "unknown";
	const char *vname = "unknown";
	const char *vendors[] = {
		"unknown",
		"Samsung",
		"Qimonda",
		"Elpida",
		"Etron",
		"Nanya",
		"Hynix",
		"Mosel",
		"Winbond",
		"ESMT",
		"unknown",
		"Spansion",
		"SST",
		"ZMOS",
		"Intel"
	};

	const char *types[] = {
		"LP2",
		"S2 SDRAM",
		"N NVM",
		"LP3"
	};
	const char *types_lpddr4[] = {
		"LP4",
		"LP4x", /* SkHynix LPDDR4X */
		"LP4x", /* Samsung LPDDR4X */
	};

	ddr_info = kzalloc(sizeof(struct mmi_ddr_info), GFP_KERNEL);
	if (!ddr_info) {
		pr_err("%s: failed to allocate space for mmi_ddr_info\n",
			__func__);
		ret = 1;
		goto err;
	}

	n = of_find_node_by_path("/chosen/mmi,ram");
	if (n != NULL) {

		of_property_read_u32(n, "mr5", &ddr_info->mr5);
		of_property_read_u32(n, "mr6", &ddr_info->mr6);
		of_property_read_u32(n, "mr7", &ddr_info->mr7);
		of_property_read_u32(n, "mr8", &ddr_info->mr8);
		of_property_read_u32(n, "type", &ddr_info->type);
		of_property_read_u32(n, "ramsize", &ddr_info->ramsize);

		of_node_put(n);

		ddr_info->mr5 &= 0xFF;
		ddr_info->mr6 &= 0xFF;
		ddr_info->mr7 &= 0xFF;
		ddr_info->mr8 &= 0xFF;

		/* identify vendor */
		vid = ddr_info->mr5 & 0xFF;
		if (vid < ARRAY_SIZE(vendors))
			vname = vendors[vid];
		else if (vid == 0x12)
			vname = "BAMC";
		else if (vid == 0x1B)
			vname = "ISSI";
		else if (vid == 0x1C)
			vname = "EMLSI";
		else if (vid == 0xC2)
			vname = "MACRONIX";
		else if (vid == 0xFE)
			vname = "Numonyx";
		else if (vid == 0xFF)
			vname = "Micron";

		snprintf(sysfsram_vendor_name, sizeof(sysfsram_vendor_name),
			"%s", vname);

		/* identify type */
		tid = ddr_info->mr8 & 0x03;
		if (ddr_info->type == MOT_DDR_TYPE_LPDDR4) {
			if (tid < ARRAY_SIZE(types_lpddr4))
				tname = types_lpddr4[tid];
		} else {
			if (tid < ARRAY_SIZE(types))
				tname = types[tid];
		}

		snprintf(sysfsram_type_name, sizeof(sysfsram_type_name),
			"%s", tname);

		/* extract size */
		sysfsram_ramsize = ddr_info->ramsize;

		mmi_annotate_persist(
			"RAM: %s, %s, %u MB, MR5:0x%02X, MR6:0x%02X, "
			"MR7:0x%02X, MR8:0x%02X\n",
			vname, tname, ddr_info->ramsize,
			ddr_info->mr5, ddr_info->mr6,
			ddr_info->mr7, ddr_info->mr8);

	} else {
		/* defaults will be reported */
		ddr_info->mr5 = 0xFF;
		ddr_info->mr6 = 0xFF;
		ddr_info->mr7 = 0xFF;
		ddr_info->mr8 = 0xFF;
		ddr_info->ramsize = 0xFF;
		pr_err("%s: failed to access RAM info in FDT\n", __func__);
	}

	/* create sysfs object */
	ram_info_properties_kobj = kobject_create_and_add("ram", NULL);

	if (ram_info_properties_kobj)
		status = sysfs_create_group(ram_info_properties_kobj,
			&ram_info_properties_attr_group);

	if (!ram_info_properties_kobj || status) {
		pr_err("%s: failed to create /sys/ram\n", __func__);
		return 1;
	} else
		return 0;
err:
	return ret;
}

void mmi_ram_info_exit(void)
{
	sysfs_remove_group(ram_info_properties_kobj,
			&ram_info_properties_attr_group);
	kobject_del(ram_info_properties_kobj);
}

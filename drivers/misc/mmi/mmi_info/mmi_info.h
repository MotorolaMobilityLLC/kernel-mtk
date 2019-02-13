/*
 * Copyright (C) 2013 Motorola Mobility LLC
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
#ifndef __ARCH_ARM_MACH_MSM_MMI_HW_INFO_H
#define __ARCH_ARM_MACH_MSM_MMI_HW_INFO_H

int mmi_storage_info_init(void);
void mmi_storage_info_exit(void);
int mmi_ram_info_init(void);
void mmi_ram_info_exit(void);
int mmi_unit_info_init(void);
void mmi_unit_info_exit(void);
int mmi_boot_info_init(void);
void mmi_boot_info_exit(void);
int mmi_get_bootarg(char *key, char **value);

struct mmi_storage_info {
	char type[16];	/* UFS or eMMC */
	char size[16];	/* size in GB */
	char card_manufacturer[32];
	char product_name[32];	/* model ID */
	char firmware_version[32];
};

struct mmi_ddr_info{
	unsigned int mr5;
	unsigned int mr6;
	unsigned int mr7;
	unsigned int mr8;
	unsigned int ramsize;
};

/* set of data provided to the modem over SMEM */
#define SERIALNO_MAX_LEN 64
#define ANDROIDBOOT_DEVICE_MAX_LEN 32
#define MMI_UNIT_INFO_VER 3
#define BARCODE_MAX_LEN 65
#define MACHINE_MAX_LEN 33
#define CARRIER_MAX_LEN 65
#define BASEBAND_MAX_LEN 97
#define SYSHW_MAX_LEN 32
#define DEVICE_MAX_LEN 33
#define RADIO_MAX_LEN 33
struct mmi_unit_info {
	uint32_t version;
	uint32_t system_rev;
	uint32_t system_serial_low;
	uint32_t system_serial_high;
	char machine[MACHINE_MAX_LEN];
	char barcode[BARCODE_MAX_LEN];
	char carrier[CARRIER_MAX_LEN];
	char baseband[BASEBAND_MAX_LEN];
	char device[DEVICE_MAX_LEN];
	uint32_t radio;
	uint32_t powerup_reason;
	char radio_str[RADIO_MAX_LEN];
};

/* select properties from /chosen node in of */
struct mmi_chosen_info {
	unsigned int system_rev;
	unsigned int system_serial_low;
	unsigned int system_serial_high;
	unsigned int powerup_reason;
	unsigned int product_id;
	unsigned int mbmversion;
	unsigned int boot_seq;
	char baseband[BASEBAND_MAX_LEN];
	char system_hw[SYSHW_MAX_LEN];
};

extern struct mmi_chosen_info mmi_chosen_data;

#define SYSFS_ATTRIBUTE(_name, _mode) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, _mode, _name##_show, NULL); \

#define SYSFS_SIMPLE_SHOW(_name, _param, _type, _size) \
static ssize_t _name##_show(struct kobject *kobj, \
	struct kobj_attribute *attr, char *buf) \
{ \
	return snprintf(buf, _size, _type"\n", _param); \
} \

#ifdef CONFIG_MMI_ANNOTATE
extern int mmi_annotate(const char *fmt, ...);
extern int mmi_annotate_persist(const char *fmt, ...);
#else
static inline int mmi_annotate(const char *fmt, ...)
{
	return 0;
}
static inline int mmi_annotate_persist(const char *fmt, ...)
{
	return 0;
}
#endif

#endif

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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <asm/setup.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include "mmi_info.h"

static struct proc_dir_entry *unitinfo_procfs_file;
static char serialno[SERIALNO_MAX_LEN];
static char carrier[CARRIER_MAX_LEN];
static char baseband[BASEBAND_MAX_LEN];
static char androidboot_device[ANDROIDBOOT_DEVICE_MAX_LEN];
static unsigned int androidboot_radio;
static char androidboot_radio_str[RADIO_MAX_LEN];

static void mmi_bootarg_setup(void)
{
	char *s;

	if (mmi_get_bootarg("androidboot.radio=", &s) == 0) {
		if (kstrtouint(s, 16, &androidboot_radio) < 0)
			androidboot_radio = 0;
		strlcpy(androidboot_radio_str, s, RADIO_MAX_LEN);
	}

	if (mmi_get_bootarg("androidboot.device=", &s) == 0)
		strlcpy(androidboot_device, s, ANDROIDBOOT_DEVICE_MAX_LEN);

	if (mmi_get_bootarg("androidboot.baseband=", &s) == 0)
		strlcpy(baseband, s, BASEBAND_MAX_LEN);

	if (mmi_get_bootarg("androidboot.carrier=", &s) == 0)
		strlcpy(carrier, s, CARRIER_MAX_LEN);

	if (mmi_get_bootarg("androidboot.serialno=", &s) == 0)
		strlcpy(serialno, s, SERIALNO_MAX_LEN);
}

static int unitinfo_seq_show(struct seq_file *f, void *ptr)
{
	seq_printf(f, "Hardware\t: %s\n", mmi_chosen_data.system_hw);
	seq_printf(f, "Revision\t: %04x\n", mmi_chosen_data.system_rev);
	seq_printf(f, "Serial\t\t: %08x%08x\n",
		mmi_chosen_data.system_serial_high,
		mmi_chosen_data.system_serial_low);

	seq_printf(f, "Device\t\t: %s\n", androidboot_device);
	/* Zero is not a valid "Radio" value.      */
	/* Lack of "Radio" entry in cpuinfo means: */
	/*	look for radio in "Revision"       */
	if (strnlen(androidboot_radio_str, RADIO_MAX_LEN))
		seq_printf(f, "Radio\t\t: %s\n", androidboot_radio_str);

	return 0;
}

static int unitinfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, unitinfo_seq_show, inode->i_private);
}

static const struct file_operations unitinfo_operations = {
	.open		= unitinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int  mmi_unit_info_init(void)
{
	mmi_bootarg_setup();

	/* /proc/unitinfo */
	unitinfo_procfs_file = proc_create("unitinfo",
		0444, NULL, &unitinfo_operations);

	return 0;
}

void mmi_unit_info_exit(void)
{
	if (unitinfo_procfs_file)
		remove_proc_entry("unitinfo", NULL);
}

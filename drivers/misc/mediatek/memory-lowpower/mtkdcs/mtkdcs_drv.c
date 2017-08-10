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

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt
#define CONFIG_MTKDCS_DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <mt-plat/mtk_meminfo.h>

/* Memory lowpower private header file */
#include "../internal.h"

static enum dcs_status sys_dcs_status;
static int dcs_initialized;
static int normal_channel_num;
static int lowpower_channel_num;
static DEFINE_SPINLOCK(dcs_lock);

/* dummy IPI APIs */
enum dcs_status dummy_ipi_read_dcs_status(void)
{
	return DCS_NORMAL;
}
int dummy_dram_channel_num(void)
{
	return 2;
}
int dummy_ipi_do_dcs(enum dcs_status status)
{
	return 0;
}

/*
 * dcs_dram_channel_switch
 *
 * Send a IPI call to SSPM to perform dynamic channel switch.
 * The dynamic channel switch only performed in stable status.
 * i.e., DCS_NORMAL or DCS_LOWPOWER.
 * @status: channel mode
 *
 * return 0 on success or error code
 */
int dcs_dram_channel_switch(enum dcs_status status)
{
	if (!spin_trylock(&dcs_lock))
		return -EBUSY;

	if ((sys_dcs_status < DCS_BUSY) &&
		(status < DCS_BUSY) &&
		(sys_dcs_status != DCS_BUSY)) {
		dummy_ipi_do_dcs(status);
		sys_dcs_status = DCS_BUSY;
	}
	spin_unlock(&dcs_lock);
	/*
	 * assume we success doing channel switch, the sys_dcs_status
	 * should be updated in the ISR
	 */
	spin_lock(&dcs_lock);
	sys_dcs_status = status;
	spin_unlock(&dcs_lock);
	return 0;
}

/*
 * dcs_get_channel_num_trylock
 * return the number of DRAM channels and get the dcs lock
 * @num: address storing the number of DRAM channels.
 *
 * return 0 on success or error code
 */
int dcs_get_channel_num_trylock(int *num)
{
	if (!spin_trylock(&dcs_lock))
		goto BUSY;

	switch (sys_dcs_status) {
	case DCS_NORMAL:
		*num = normal_channel_num;
		break;
	case DCS_LOWPOWER:
		*num = lowpower_channel_num;
		break;
	default:
		goto BUSY;
	}
	return 0;
BUSY:
	*num = -1;
	return -EBUSY;
}

/*
 * dcs_get_channel_num_unlock
 * unlock the dcs lock
 */
void dcs_get_channel_num_unlock(void)
{
	spin_unlock(&dcs_lock);
}

/*
 * dcs_initialied
 *
 * return true if dcs is initialized
 */
bool dcs_initialied(void)
{
	return dcs_initialized;
}

static int __init mtkdcs_init(void)
{
	/* read system dcs status */
	sys_dcs_status = dummy_ipi_read_dcs_status();
	/* read number of dram channels */
	normal_channel_num = dummy_dram_channel_num();

	/* the channel number must be multiple of 2 */
	if (normal_channel_num % 2) {
		pr_err("%s fail, incorrect normal channel num=%d\n",
				__func__, normal_channel_num);
		dcs_initialized = false;
		return -EINVAL;
	}

	lowpower_channel_num = (normal_channel_num / 2);

	dcs_initialized = true;

	return 0;
}

static void __exit mtkdcs_exit(void)
{
}

device_initcall(mtkdcs_init);
module_exit(mtkdcs_exit);

#ifdef CONFIG_MTKDCS_DEBUG
static int mtkdcs_show(struct seq_file *m, void *v)
{
	return 0;
}

static int mtkdcs_open(struct inode *inode, struct file *file)
{
	return single_open(file, &mtkdcs_show, NULL);
}

static ssize_t mtkdcs_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	static char state;

	if (count > 0) {
		if (get_user(state, buffer))
			return -EFAULT;
		state -= '0';

		if (state)
			/* low power to normal */
			pr_alert("state=%d\n", state);
		else
			/* normal to low power */
			pr_alert("state=%d\n", state);
	}

	return count;
}

static const struct file_operations mtkdcs_fops = {
	.open		= mtkdcs_open,
	.write		= mtkdcs_write,
	.read		= seq_read,
	.release	= single_release,
};

static int __init mtkdcs_debug_init(void)
{
	struct dentry *dentry;

	if (!dcs_initialied()) {
		pr_err("mtkdcs is not inited\n");
		return 1;
	}

	dentry = debugfs_create_file("mtkdcs", S_IRUGO, NULL, NULL,
			&mtkdcs_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs mtkdcs file\n");

	return 0;
}

late_initcall(mtkdcs_debug_init);
#endif /* CONFIG_MTKDCS_DEBUG */

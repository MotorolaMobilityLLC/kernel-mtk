/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/jiffies.h>
#include <linux/kallsyms.h>
#include <linux/kobject.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>

#include "smart.h"
#include <linux/platform_device.h>

#define SEQ_printf(m, x...)                                                    \
	do {                                                                   \
		if (m)                                                         \
			seq_printf(m, x);                                      \
		else                                                           \
			pr_debug(x);                                           \
	} while (0)
#undef TAG
#define TAG "[SMART]"

#define S_LOG(fmt, args...) pr_debug(TAG fmt, ##args)

static int turbo_support;
/* Operaton */

/******* FLIPER SETTING *********/

static ssize_t mt_smart_turbo_support_write(struct file *filp, const char *ubuf,
					    size_t cnt, loff_t *data)
{
	int ret;
	unsigned long arg;

	if (!kstrtoul_from_user(ubuf, cnt, 0, &arg)) {
		ret = 0;
		if (arg == 0)
			turbo_support = 0;
		else
			turbo_support = 1;
	} else
		ret = -EINVAL;

	return (ret < 0) ? ret : cnt;
}

static int mt_smart_turbo_support_show(struct seq_file *m, void *v)
{
	if (turbo_support)
		SEQ_printf(m, "1\n");
	else
		SEQ_printf(m, "0\n");
	return 0;
}

/*** Seq operation of mtprof ****/
static int mt_smart_turbo_support_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_smart_turbo_support_show, inode->i_private);
}

static const struct file_operations mt_smart_turbo_support_fops = {
    .open = mt_smart_turbo_support_open,
    .write = mt_smart_turbo_support_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static void check_l_plus_support(void)
{
	unsigned int segment_inner = (get_devinfo_with_index(30) & 0xE0) >> 5;
	unsigned int bining = get_devinfo_with_index(30) & 0x7;

	if (segment_inner == 7 || bining == 3)
		turbo_support = 1;
}

/*--------------------INIT------------------------*/
static int __init init_smart(void)
{
	struct proc_dir_entry *pe5;
	struct proc_dir_entry *smart_dir = NULL;

	/* poting */
	smart_dir = proc_mkdir("perfmgr/smart", NULL);

	pr_debug(TAG "init smart driver start\n");
	pe5 = proc_create("smart_turbo_support", 0644, smart_dir,
			  &mt_smart_turbo_support_fops);
	if (!pe5)
		return -ENOMEM;

	turbo_support = 0;
	check_l_plus_support();

	pr_debug(TAG "init smart driver done\n");

	return 0;
}
late_initcall(init_smart);

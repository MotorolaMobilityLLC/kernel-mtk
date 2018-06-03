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

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#undef TAG
#define TAG "[SOC DVFS FLIPER]"

/* #if defined(CONFIG_MTK_QOS_SUPPORT) */
#define MTK_QOS_SUPPORT
/* #endif */

#if defined(MTK_QOS_SUPPORT)
#include <linux/pm_qos.h>
#include <helio-dvfsrc-opp.h>
#endif

#if defined(CONFIG_MTK_DRAMC)
#include "mtk_dramc.h"
#endif


/* for debug */
static int fliper_debug;
/* KIR_PERF */

static int ddr_type;

#ifdef MTK_QOS_SUPPORT
/* QoS Method */
	static int ddr_now;
	static struct pm_qos_request emi_request;
	static int emi_opp;
#endif


static int mt_fliper_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "DEBUG: %d\n", fliper_debug);
	SEQ_printf(m, "DDR_TYPE: %d\n", ddr_type);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_fliper_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_fliper_show, inode->i_private);
}


static const struct file_operations mt_fliper_fops = {
	.open = mt_fliper_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#ifdef MTK_QOS_SUPPORT
static ssize_t mt_ddr_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int val;
	int ret;


	if (fliper_debug) {
		pr_debug("ddr_write fliper_debug\n");
		return cnt;
	}
	if (cnt >= sizeof(buf)) {
		pr_debug("ddr_write cnt >= sizeof\n");
		return -EINVAL;
	}
	if (copy_from_user(buf, ubuf, cnt)) {
		pr_debug("ddr_write copy_from_user\n");
		return -EFAULT;
	}
	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0) {
		pr_debug("ddr_write ret < 0\n");
		return ret;
	}
	if (val < -1 || val > emi_opp) {
		pr_debug("UNREQ\n");
		return -1;
	}

	pm_qos_update_request(&emi_request, val);

	ddr_now = val;

	return cnt;
}

static int mt_ddr_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", ddr_now);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_ddr_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_ddr_show, inode->i_private);
}
static const struct file_operations mt_ddr_fops = {
	.open = mt_ddr_open,
	.write = mt_ddr_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif
/*--------------------INIT------------------------*/

static int __init init_fliper(void)
{
	struct proc_dir_entry *fliperfs_dir;
	struct proc_dir_entry *fliper_dir;
#ifdef MTK_QOS_SUPPORT

	struct proc_dir_entry *ddr_dir;
#endif
	pr_debug("init fliper driver start\n");
	fliperfs_dir = proc_mkdir("fliperfs", NULL);
	if (!fliperfs_dir) {
		pr_debug("fliperfs_dir not create success\n");
		return -ENOMEM;
	}
	fliper_dir = proc_create("fliper", 0644, fliperfs_dir, &mt_fliper_fops);
	if (!fliper_dir) {
		pr_debug("fliper_dir not create success\n");
		return -ENOMEM;
	}
#ifdef MTK_QOS_SUPPORT
/* QoS Method */
	ddr_now = -1;
	if (!pm_qos_request_active(&emi_request)) {
		pr_debug("hh: emi pm_qos_add_request\n");
		pm_qos_add_request(&emi_request, PM_QOS_DDR_OPP,
					PM_QOS_DDR_OPP_DEFAULT_VALUE);
	} else {
		pr_debug("hh: emi pm_qos already request\n");
	}
	ddr_dir = proc_create("emi", 0644, fliperfs_dir, &mt_ddr_fops);
	if (!ddr_dir) {
		pr_debug("ddr_dir not create success\n");
		return -ENOMEM;
	}
	emi_opp = DDR_OPP_NUM - 1;
#endif

	fliper_debug = 0;

#if defined(CONFIG_MTK_DRAMC)
	ddr_type = get_ddr_type();
#else
	ddr_type = -1;
#endif

	pr_debug("init fliper driver done\n");

	return 0;
}
late_initcall(init_fliper);


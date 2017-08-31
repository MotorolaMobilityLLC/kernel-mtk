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
#include <linux/notifier.h>
#include <linux/suspend.h>

#include <linux/platform_device.h>

#include <mach/fliper.h>
#include <mtk_vcorefs_governor.h>
#include <mtk_vcorefs_manager.h>

#include <mtk_emi_bm.h>

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#undef TAG
#define TAG "[SOC DVFS FLIPER]"

#define BWM_SUPPORT 1
#define OPP_NUM 4

/* for debug */
static int fliper_debug;
static int channel;
/* KIR_PERF */
static int perf_now;
/* CG BW monitor */
static int cg_enable;
static int cg_port;
unsigned int cg_threshold[OPP_NUM];
static unsigned int cg_thr;
static int cg_period;
static int cg_intr;
/* TOTAL BW monitor */
#if 1
static int total_enable;
unsigned int total_threshold[OPP_NUM][OPP_NUM - 1];
unsigned int total_threshold_2[OPP_NUM][OPP_NUM - 1];
static int total_period;
static int total_intr;
#endif

/******* FLIPER SETTING *********/

int init_cg_monitor(void)
{
	if (channel == 4)
		cg_threshold[0] = cg_threshold[1] = cg_threshold[2] = cg_threshold[3] = cg_thr * 2;
	else if (channel == 2)
		cg_threshold[0] = cg_threshold[1] = cg_threshold[2] = cg_threshold[3] = cg_thr;

#if  defined(CONFIG_MTK_EMI_MBW)
	mt_set_emi_bw1_threshold(cg_threshold);
	mt_set_emi_bw1_axi_port(cg_port);
	mt_set_emi_bw1_intr_period(cg_period);
	mt_set_emi_bw1_intr_status(cg_intr);
	mt_set_emi_bw1_enable(cg_enable);
#endif

	return 0;
}
int init_total_monitor(void)
{
#if  defined(CONFIG_MTK_EMI_MBW)
	if (channel == 2)
		mt_set_emi_total_bw_threshold(total_threshold);
	else
		mt_set_emi_total_bw_threshold(total_threshold_2);

	mt_set_emi_total_bw_intr_period(total_period);
	mt_set_emi_total_bw_intr_status(total_intr);
	mt_set_emi_total_bw_enable(total_enable);
#endif
	return 0;
}

int notify_bwm_dcs(int chan_num)
{
	channel = chan_num;
	pr_debug(TAG"notify_bwm_dcs %d\n", channel);

	if (channel == 4)
		cg_threshold[0] = cg_threshold[1] = cg_threshold[2] = cg_threshold[3] = cg_thr * 2;
	else if (channel == 2)
		cg_threshold[0] = cg_threshold[1] = cg_threshold[2] = cg_threshold[3] = cg_thr;

	init_cg_monitor();
	init_total_monitor();

	return 0;
}

static int mt_fliper_show(struct seq_file *m, void *v)
{
#if BWM_SUPPORT
	SEQ_printf(m, "-----------------------------------------------------\n");
	SEQ_printf(m, "CG Fliper Enabled:%d, bw threshold:%dMB/s\n",
			cg_enable, cg_thr);
#if 0
	SEQ_printf(m, "TOTAL Fliper Enabled:%d, bw threshold:%u %u %uMB/s\n",
			total_enable, total_ulpm_bw_threshold, total_lpm_bw_threshold, total_hpm_bw_threshold);
#endif
#endif
	SEQ_printf(m, "KIR_PERF: %d\n", perf_now);
	SEQ_printf(m, "DEBUG: %d\n", fliper_debug);
#if BWM_SUPPORT
#if 0
	SEQ_printf(m, "CG History: 0x%08x\n", getCGHistory());
	SEQ_printf(m, "TOTAL History: 0x%08x\n", getTotalHistory());
	SEQ_printf(m, "CG Config: 0x%08x\n", getCGConfiguration());
	SEQ_printf(m, "TOTAL Config: 0x%08x\n", getTotalConfiguration());
	SEQ_printf(m, "-----------------------------------------------------\n");
#endif
#endif
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_fliper_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_fliper_show, inode->i_private);
}


static const struct file_operations mt_fliper_fops = {
	.open = mt_fliper_open,
	/*.write = mt_fliper_write,*/
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mt_dump_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int val;
	int ret;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

#if defined(CONFIG_MTK_EMI_MBW)
	if (val == 1) {
		mt_get_emi_bw1();
		mt_get_emi_total_bw();
	}
#endif

	return cnt;
}

static int mt_dump_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", 1);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_dump_show, inode->i_private);
}

static const struct file_operations mt_dump_fops = {
	.open = mt_dump_open,
	.write = mt_dump_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mt_perf_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int val;
	int ret;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < -1 || val > 4)
		return -1;

	vcorefs_request_dvfs_opp(KIR_PERF, val);
	perf_now = val;

	return cnt;
}

static int mt_perf_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", perf_now);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_perf_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_perf_show, inode->i_private);
}

static const struct file_operations mt_perf_fops = {
	.open = mt_perf_open,
	.write = mt_perf_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mt_cg_threshold_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;


	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

#if 0
	if (val < 400)
		return -1;
#endif

#if BWM_SUPPORT
	cg_thr = val;
	init_cg_monitor();
#endif

	return cnt;
}

static int mt_cg_threshold_show(struct seq_file *m, void *v)
{
#if BWM_SUPPORT
	SEQ_printf(m, "%d\n", cg_thr);
#endif
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_cg_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cg_threshold_show, inode->i_private);
}

static const struct file_operations mt_cg_threshold_fops = {
	.open = mt_cg_threshold_open,
	.write = mt_cg_threshold_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mt_cg_enable_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < 0)
		return -1;

#if BWM_SUPPORT
	cg_enable = val;
	init_cg_monitor();
#endif

	return cnt;
}

static int mt_cg_enable_show(struct seq_file *m, void *v)
{
#if BWM_SUPPORT
	SEQ_printf(m, "%d\n", cg_enable);
#endif
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_cg_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cg_enable_show, inode->i_private);
}

static const struct file_operations mt_cg_enable_fops = {
	.open = mt_cg_enable_open,
	.write = mt_cg_enable_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


	static int
fliper_pm_callback(struct notifier_block *nb,
		unsigned long action, void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
		break;
	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
#if BWM_SUPPORT
		pr_debug(TAG"Resume, restore CG configuration\n");
		init_cg_monitor();
		init_total_monitor();
#endif
		break;

	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}


/*--------------------INIT------------------------*/

static int __init init_fliper(void)
{
	struct proc_dir_entry *fliperfs_dir;
	struct proc_dir_entry *perf_dir, *cg_threshold_dir,
		*cg_enable_dir, *fliper_dir, *dump_dir;

	/*initialize*/
	cg_thr = 4000;
	cg_enable = 1;
	cg_port = 0xc3;
	cg_threshold[0] = cg_threshold[1] = cg_threshold[2] = cg_threshold[3] = cg_thr;
	cg_period = 17;
	cg_intr = 1;

	total_enable = 1;
	total_threshold[0][0] = 2600;
	total_threshold[0][1] = 10700;
	total_threshold[0][2] = 8500;
	total_threshold[1][0] = 2600;
	total_threshold[1][1] = 10700;
	total_threshold[1][2] = 8500;
	total_threshold[2][0] = 2600;
	total_threshold[2][1] = 10700;
	total_threshold[2][2] = 8500;
	total_threshold[3][0] = 2600;
	total_threshold[3][1] = 10700;
	total_threshold[3][2] = 8500;
	total_threshold_2[0][0] = 2600 * 2;
	total_threshold_2[0][1] = 10700 * 2;
	total_threshold_2[0][2] = 8500 * 2;
	total_threshold_2[1][0] = 2600 * 2;
	total_threshold_2[1][1] = 10700 * 2;
	total_threshold_2[1][2] = 8500 * 2;
	total_threshold_2[2][0] = 2600 * 2;
	total_threshold_2[2][1] = 10700 * 2;
	total_threshold_2[2][2] = 8500 * 2;
	total_threshold_2[3][0] = 2600 * 2;
	total_threshold_2[3][1] = 10700 * 2;
	total_threshold_2[3][2] = 8500 * 2;
	total_period = 17;
	total_intr = 1;

	channel = 2;

	pr_debug(TAG"init fliper driver start\n");
	fliperfs_dir = proc_mkdir("fliperfs", NULL);
	if (!fliperfs_dir)
		return -ENOMEM;

	perf_dir = proc_create("perf", 0644, fliperfs_dir, &mt_perf_fops);
	if (!perf_dir)
		return -ENOMEM;

	cg_threshold_dir = proc_create("cg_threshold", 0644, fliperfs_dir, &mt_cg_threshold_fops);
	if (!cg_threshold_dir)
		return -ENOMEM;


	cg_enable_dir = proc_create("cg_enable", 0644, fliperfs_dir, &mt_cg_enable_fops);
	if (!cg_enable_dir)
		return -ENOMEM;

	fliper_dir = proc_create("fliper", 0644, fliperfs_dir, &mt_fliper_fops);
	if (!fliper_dir)
		return -ENOMEM;

	dump_dir = proc_create("dump", 0644, fliperfs_dir, &mt_dump_fops);
	if (!fliper_dir)
		return -ENOMEM;

	perf_now = -1;
	fliper_debug = 0;

	init_cg_monitor();
	init_total_monitor();

	pm_notifier(fliper_pm_callback, 0);

	pr_debug(TAG"init fliper driver done\n");

	return 0;
}
late_initcall(init_fliper);


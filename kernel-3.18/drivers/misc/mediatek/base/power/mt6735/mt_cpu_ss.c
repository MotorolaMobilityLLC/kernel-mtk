/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>

#include "sync_write.h"
#include "mt_cpufreq.h"
#include "mach/mt_clkmgr.h"

static struct hrtimer mt_cpu_ss_timer;
struct task_struct *mt_cpu_ss_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(mt_cpu_ss_timer_waiter);

static int mt_cpu_ss_period_s;
static int mt_cpu_ss_period_ns = 100;

static int mt_cpu_ss_timer_flag;

static bool mt_cpu_ss_debug_mode;
static bool mt_cpu_ss_period_mode;

enum hrtimer_restart mt_cpu_ss_timer_func(struct hrtimer *timer)
{
	if (mt_cpu_ss_debug_mode)
		pr_debug("[%s]: enter timer function\n", __func__);

	mt_cpu_ss_timer_flag = 1;
	wake_up_interruptible(&mt_cpu_ss_timer_waiter);

	return HRTIMER_NORESTART;
}

int mt_cpu_ss_thread_handler(void *unused)
{
	unsigned int flag = 0;

	do {
		ktime_t ktime = ktime_set(mt_cpu_ss_period_s, mt_cpu_ss_period_ns);

		wait_event_interruptible(mt_cpu_ss_timer_waiter, mt_cpu_ss_timer_flag != 0);
		mt_cpu_ss_timer_flag = 0;

		if (!flag) {
			mt_cpufreq_set_cpu_clk_src(0, TOP_CKMUXSEL_CLKSQ);
			flag = 1;
		} else {
			mt_cpufreq_set_cpu_clk_src(0, TOP_CKMUXSEL_ARMPLL);
			flag = 0;
		}

		if (mt_cpu_ss_debug_mode)
			pr_debug("[%s]: TOP_CKMUXSEL = 0x%x\n", __func__, __raw_readl(TOP_CKMUXSEL));

		hrtimer_start(&mt_cpu_ss_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;
}

#ifdef CONFIG_PROC_FS
/*
 * PROC
 */

static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}

/* cpu_ss_debug_mode */
static int cpu_ss_debug_mode_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", (mt_cpu_ss_debug_mode) ? "enable" : "disable");

	return 0;
}

static ssize_t cpu_ss_debug_mode_proc_write(struct file *file, const char __user *buffer,
					    size_t count, loff_t *pos)
{
	char mode[20];

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (count >= sizeof(mode)) {
		pr_err("[%s]: bad argument!! input length is over buffer size\n", __func__);
		return -EINVAL;
	}

	if (sscanf(buf, "%19s", mode) == 1) {
		if (!strcmp(mode, "enable")) {
			pr_debug("[%s]: %s cpu speed switch debug mode\n", mode, __func__);
			mt_cpu_ss_debug_mode = true;
		} else if (!strcmp(mode, "disable")) {
			pr_debug("[%s]: %s cpu speed switch debug mode\n", mode, __func__);
			mt_cpu_ss_debug_mode = false;
		} else
			pr_err("[%s]: bad argument!! should be \"enable\" or \"disable\"\n",
			       __func__);
	} else
		pr_err("[%s]: bad argument!! should be \"enable\" or \"disable\"\n", __func__);

	free_page((unsigned long)buf);

	return count;
}

/* cpu_ss_period_mode */
static int cpu_ss_period_mode_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", (mt_cpu_ss_period_mode) ? "enable" : "disable");

	return 0;
}

static ssize_t cpu_ss_period_mode_proc_write(struct file *file, const char __user *buffer,
					     size_t count, loff_t *pos)
{
	char mode[20];
	ktime_t ktime = ktime_set(mt_cpu_ss_period_s, mt_cpu_ss_period_ns);

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (count >= sizeof(mode)) {
		pr_err("[%s]: bad argument!! input length is over buffer size\n", __func__);
		return -EINVAL;
	}

	if (sscanf(buf, "%19s", mode) == 1) {
		if (!strcmp(mode, "enable")) {
			pr_debug("[%s]: %s cpu speed switch period mode\n", mode, __func__);
			mt_cpu_ss_period_mode = true;

			mt_cpu_ss_thread =
			    kthread_run(mt_cpu_ss_thread_handler, 0, "cpu speed switch");
			if (IS_ERR(mt_cpu_ss_thread))
				pr_err("[%s]: failed to create cpu speed switch thread\n",
				       __func__);

			hrtimer_start(&mt_cpu_ss_timer, ktime, HRTIMER_MODE_REL);
		} else if (!strcmp(mode, "disable")) {
			pr_debug("[%s]: %s cpu speed switch period mode\n", mode, __func__);
			mt_cpu_ss_period_mode = false;

			kthread_stop(mt_cpu_ss_thread);

			mt_cpufreq_set_cpu_clk_src(0, TOP_CKMUXSEL_ARMPLL);

			hrtimer_cancel(&mt_cpu_ss_timer);
		} else
			pr_err("[%s]: bad argument!! should be \"enable\" or \"disable\"\n",
			       __func__);
	} else
		pr_err("[%s]: bad argument!! should be \"enable\" or \"disable\"\n", __func__);

	free_page((unsigned long)buf);

	return count;
}

/* cpu_ss_period */
static int cpu_ss_period_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d (s) %d (ns)\n", mt_cpu_ss_period_s, mt_cpu_ss_period_ns);

	return 0;
}

static ssize_t cpu_ss_period_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	int s = 0, ns = 0;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d", &s, &ns) == 2) {
		pr_debug("[%s]: set cpu speed switch period = %d (s), %d (ns)\n", __func__, s, ns);
		mt_cpu_ss_period_s = s;
		mt_cpu_ss_period_ns = ns;
	} else
		pr_err("[%s]: bad argument!! should be \"[s]\" or \"[ns]\"\n", __func__);

	return count;
}

/* cpu_ss_mode */
static int cpu_ss_mode_proc_show(struct seq_file *m, void *v)
{
	char *cpu_ss_mode;

	switch (mt_cpufreq_get_cpu_clk_src(0)) {
	case TOP_CKMUXSEL_CLKSQ:
		cpu_ss_mode = "CLKSQ";
		break;
	case TOP_CKMUXSEL_ARMPLL:
		cpu_ss_mode = "ARMPLL";
		break;
	case TOP_CKMUXSEL_MAINPLL:
		cpu_ss_mode = "MAINPLL";
		break;
	default:
		cpu_ss_mode = "UNKNOWN";
		break;
	}

	seq_printf(m, "CPU clock source is %s\n", cpu_ss_mode);

	return 0;
}

static ssize_t cpu_ss_mode_proc_write(struct file *file, const char __user *buffer, size_t count,
				      loff_t *pos)
{
	int mode = 0;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &mode)) {
		if (mode) {
			pr_debug("[%s]: config cpu speed switch mode = ARMPLL\n", __func__);
			mt_cpufreq_set_cpu_clk_src(0, TOP_CKMUXSEL_ARMPLL);
		} else {
			pr_debug("[%s]: config cpu speed switch mode = CLKSQ\n", __func__);
			mt_cpufreq_set_cpu_clk_src(0, TOP_CKMUXSEL_CLKSQ);
		}
	} else
		pr_err("[%s]: bad argument!! should be \"1\" or \"0\"\n", __func__);

	return count;
}

#define PROC_FOPS_RW(name)							\
	static int name ## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, NULL);		\
	}									\
	static const struct file_operations name ## _proc_fops = {		\
		.owner          = THIS_MODULE,					\
		.open           = name ## _proc_open,				\
		.read           = seq_read,					\
		.llseek         = seq_lseek,					\
		.release        = single_release,				\
		.write          = name ## _proc_write,				\
	}

#define PROC_FOPS_RO(name)							\
	static int name ## _proc_open(struct inode *inode, struct file *file)	\
	{									\
		return single_open(file, name ## _proc_show, NULL);		\
	}									\
	static const struct file_operations name ## _proc_fops = {		\
		.owner          = THIS_MODULE,					\
		.open           = name ## _proc_open,				\
		.read           = seq_read,					\
		.llseek         = seq_lseek,					\
		.release        = single_release,				\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(cpu_ss_debug_mode);
PROC_FOPS_RW(cpu_ss_period_mode);
PROC_FOPS_RW(cpu_ss_period);
PROC_FOPS_RW(cpu_ss_mode);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	const struct {
		const char *name;
		const struct file_operations *fops;
	} entries[] = {
	PROC_ENTRY(cpu_ss_debug_mode),
		    PROC_ENTRY(cpu_ss_period_mode),
		    PROC_ENTRY(cpu_ss_period), PROC_ENTRY(cpu_ss_mode),};

	dir = proc_mkdir("cpu_ss", NULL);

	if (!dir) {
		pr_err("fail to create /proc/cpu_ss @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			pr_err("%s(), create /proc/cpu_ss/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}
#endif	/* CONFIG_PROC_FS */




/*********************************
* cpu speed stress initialization
**********************************/
static int __init mt_cpu_ss_init(void)
{
	hrtimer_init(&mt_cpu_ss_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mt_cpu_ss_timer.function = mt_cpu_ss_timer_func;

	return _create_procfs();
}

static void __exit mt_cpu_ss_exit(void)
{
}
module_init(mt_cpu_ss_init);
module_exit(mt_cpu_ss_exit);

MODULE_DESCRIPTION("MediaTek CPU Speed Stress driver");
MODULE_LICENSE("GPL");

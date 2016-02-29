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
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/printk.h>

#include "internal.h"
/* #include <mt_hotplug_strategy.h> */
#ifdef CONFIG_MT_SCHED_MON_DEFAULT_ENABLE
#include "mt_sched_mon.h"
#endif
#include <mt_cpufreq.h>

#define BOOT_STR_SIZE 256 /* Before: 128 */
#define BOOT_LOG_NUM 192
/* Memory Usage
 * Before: (128+8)*64 = 8704.
 * Now: (64+8)*128 = 9216 */

struct boot_log_struct {
	u64 timestamp;
	char *event;
#ifdef TRACK_TASK_COMM
	pid_t pid;
	char comm[TASK_COMM_LEN];
#endif
} mt_bootprof[BOOT_LOG_NUM];

static int boot_log_count;
static DEFINE_MUTEX(mt_bootprof_lock);
static bool mt_bootprof_enabled;
static int bootprof_lk_t, bootprof_pl_t;
static u64 timestamp_on, timestamp_off;
int boot_finish = 0;

module_param_named(pl_t, bootprof_pl_t, int, S_IRUGO | S_IWUSR);
module_param_named(lk_t, bootprof_lk_t, int, S_IRUGO | S_IWUSR);

void log_boot(char *str)
{
	unsigned long long ts;
	struct boot_log_struct *p = &mt_bootprof[boot_log_count];
	size_t n = strlen(str);

	if (!mt_bootprof_enabled)
		return;
	ts = sched_clock();
	pr_err("BOOTPROF:%10Ld.%06ld:%s\n", nsec_high(ts), nsec_low(ts), str);
	if (boot_log_count >= BOOT_LOG_NUM) {
		pr_err("[BOOTPROF] not enuough bootprof buffer\n");
		return;
	}
	mutex_lock(&mt_bootprof_lock);
	p->timestamp = ts;
#ifdef TRACK_TASK_COMM
	p->pid = current->pid;
	memcpy(p->comm, current->comm, TASK_COMM_LEN);
#endif
	p->event = kzalloc(n, GFP_ATOMIC | __GFP_NORETRY |
			   __GFP_NOWARN);
	if (!p->event) {
		/* pr_err("log_boot alloc size %zu fail\n", n); */
		mt_bootprof_enabled = false;
		goto out;
	}
	memcpy(p->event, str, n);
	boot_log_count++;
out:
	mutex_unlock(&mt_bootprof_lock);
}

static void bootup_finish(void)
{
	initcall_debug = 0;
#ifdef CONFIG_MT_PRINTK_UART_CONSOLE
	mt_disable_uart();
#endif
#ifdef CONFIG_MT_SCHED_MON_DEFAULT_ENABLE
	mt_sched_monitor_switch(1);
#endif
	set_logtoomuch_enable(1);

	/* mt_cpufreq_set_min_freq(MT_CPU_DVFS_LITTLE, 0); */
	/* hps_set_cpu_num_base(BASE_PERF_SERV, 1, 0); */
}

/* extern void (*set_intact_mode)(void); */
static void mt_bootprof_switch(int on)
{
	mutex_lock(&mt_bootprof_lock);
	if (mt_bootprof_enabled ^ on) {
		unsigned long long ts = sched_clock();

		pr_err("BOOTPROF:%10Ld.%06ld: %s\n",
		       nsec_high(ts), nsec_low(ts), on ? "ON" : "OFF");

		if (on) {
			mt_bootprof_enabled = 1;
			timestamp_on = ts;
		} else {	/* boot up complete */
			mt_bootprof_enabled = 0;
			timestamp_off = ts;
			boot_finish = 1;
			/* log_store_bootup(); */
			bootup_finish();
		}
	}
	mutex_unlock(&mt_bootprof_lock);
}

static ssize_t
mt_bootprof_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	char buf[BOOT_STR_SIZE];
	size_t copy_size = cnt;

	if (cnt >= sizeof(buf))
		copy_size = BOOT_STR_SIZE - 1;

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	if (cnt == 1) {
		if (buf[0] == '0')
			mt_bootprof_switch(0);
		else if (buf[0] == '1')
			mt_bootprof_switch(1);
		return 1;
	}

	buf[copy_size] = 0;
	log_boot(buf);

	return cnt;

}

static int mt_bootprof_show(struct seq_file *m, void *v)
{
	int i;

	SEQ_printf(m, "----------------------------------------\n");
	SEQ_printf(m, "%d	    BOOT PROF (unit:msec)\n", mt_bootprof_enabled);
	SEQ_printf(m, "----------------------------------------\n");

	if (bootprof_pl_t > 0 && bootprof_lk_t > 0) {
		SEQ_printf(m, "%10d        : %s\n", bootprof_pl_t, "preloader");
		SEQ_printf(m, "%10d        : %s\n", bootprof_lk_t, "lk");
		SEQ_printf(m, "%10d        : %s\n",
			   gpt_boot_time() - bootprof_pl_t - bootprof_lk_t, "lk->Kernel");
		SEQ_printf(m, "----------------------------------------\n");
	}

	SEQ_printf(m, "%10Ld.%06ld : ON\n",
		   nsec_high(timestamp_on), nsec_low(timestamp_on));

	for (i = 0; i < boot_log_count; i++) {
		if (!mt_bootprof[i].event)
			continue;
		SEQ_printf(m, "%10Ld.%06ld : %s\n",
			   nsec_high(mt_bootprof[i].timestamp),
			   nsec_low(mt_bootprof[i].timestamp),
#ifdef TRACK_TASK_COMM
			   mt_bootprof[i].pid, mt_bootprof[i].comm,
#endif
			   mt_bootprof[i].event);
	}

	SEQ_printf(m, "%10Ld.%06ld : OFF\n",
		   nsec_high(timestamp_off), nsec_low(timestamp_off));
	SEQ_printf(m, "----------------------------------------\n");
	return 0;
}

/*** Seq operation of mtprof ****/
static int mt_bootprof_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_bootprof_show, inode->i_private);
}

static const struct file_operations mt_bootprof_fops = {
	.open = mt_bootprof_open,
	.write = mt_bootprof_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init init_boot_prof(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("bootprof", 0664, NULL, &mt_bootprof_fops);
	if (!pe)
		return -ENOMEM;
	/* set_intact_mode = NULL; */
	return 0;
}

static int __init init_bootprof_buf(void)
{
	mt_bootprof_switch(1);
	return 0;
}

early_initcall(init_bootprof_buf);
device_initcall(init_boot_prof);

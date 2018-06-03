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
#include <linux/ring_buffer.h>
#include <linux/trace_events.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>

#include <linux/platform_device.h>
#include <trace/events/sched.h>

#include <linux/ioctl.h>
#include "cpu_ctrl.h"
#include "eas_ctrl.h"
#include "fpsgo_common.h"
#include "perf_ioctl.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#ifdef CONFIG_MTK_FPSGO
#define TOUCH_TIMEOUT_NSEC 100000000
#define TOUCH_BOOST_EAS 80
#define TOUCH_BOOST_OPP 2
#define TOUCH_FSTB_ACTIVE_US 100000
#endif /* CONFIG_MTK_FPSGO */


extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);

void switch_usrtch(int enable);
long usrtch_ioctl(unsigned int cmd, unsigned long arg);
int init_usrtch(void);

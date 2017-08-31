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
#include <trace.h>
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
#include "legacy_controller.h"
#include "eas_controller.h"

#define DEV_MAJOR 121
#define DEV_NAME "debug"
#define DEV_IOCTL_AS 0xA0
#define IOCTL_WRITE_AS _IOW(DEV_IOCTL_AS, 10, int)
#define DEV_IOCTL_GM 0xB0
#define IOCTL_WRITE_GM _IOW(DEV_IOCTL_GM, 10, int)
#define DEV_IOCTL_TH 0xC0
#define IOCTL_WRITE_TH _IOW(DEV_IOCTL_TH, 10, int)
#define DEV_IOCTL_FC 0xD0
#define IOCTL_WRITE_FC _IOW(DEV_IOCTL_FC, 10, int)
#define DEV_IOCTL_IV 0xE0
#define IOCTL_WRITE_IV _IOW(DEV_IOCTL_IV, 10, int)
#define DEV_IOCTL_NR 0xF0
#define IOCTL_WRITE_NR _IOW(DEV_IOCTL_NR, 10, int)
#define DEV_IOCTL_QB 0xF1
#define IOCTL_WRITE_QB _IOW(DEV_IOCTL_QB, 10, int)


#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define HPS_LATENCY 20000000
#define TOUCH_TIMEOUT_SEC 5
#define RENDER_AWARE_TIMEOUT_MSEC 300
#define MAX_THREAD 5
#define TOUCH_BOOST_EAS 100
#define SUPER_BOOST 100

#define EAS 1
#define LEGACY 2

#define NR_PPM_CLUSTERS 3

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#define TAG "[SOC FBC]"

extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
extern int sched_scheduler_switch(SCHED_LB_TYPE new_sched);
extern int linear_real_boost(int);
#ifdef CONFIG_MTK_SCHED_VIP_TASKS
extern int vip_task_set(int pid, bool set_vip);
#endif

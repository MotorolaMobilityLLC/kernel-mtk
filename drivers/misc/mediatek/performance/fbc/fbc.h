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
#define TRACE 0
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
/*#define MET_USER_EVENT_SUPPORT*/
#if TRACE
#include <mt-plat/met_drv.h>
#endif
/*#include "core/trace.h"*/

#include <linux/platform_device.h>
#include <trace/events/sched.h>

#include <linux/ioctl.h>
#include "legacy_controller.h"
#include "eas_controller.h"
#include "mtk_unified_power.h"

#define DEV_MAJOR 121
#define DEV_NAME "debug"
#define DEV_IOCTL_FC 0xD0
#define IOCTL_WRITE_FC _IOW(DEV_IOCTL_FC, 10, int)
#define DEV_IOCTL_IV 0xE0
#define IOCTL_WRITE_IV _IOW(DEV_IOCTL_IV, 10, int)
#define DEV_IOCTL_NR 0xF0
#define IOCTL_WRITE_NR _IOW(DEV_IOCTL_NR, 10, int)
#define DEV_IOCTLID3 0xC0
#define IOCTL_WRITE3 _IOW(DEV_IOCTLID3, 10, int)

#define ID_EGL 1
#define ID_OMR 2

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#define TAG "[SOC FBC]"

extern int boost_value_for_GED_idx(int group_idx, int boost_value);
extern int linear_real_boost(int);
/*extern int linear_real_boost_pid(int, int);*/
extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
extern int update_userlimit_cpu_freq(int kicker, int num_cluster, struct ppm_limit_data *freq_limit);
extern int update_userlimit_cpu_core(int kicker, int num_cluster, struct ppm_limit_data *core_limit);



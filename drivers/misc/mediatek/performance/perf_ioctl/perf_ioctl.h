/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
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

#include <linux/ioctl.h>
#include <legacy_controller.h>

#define DEV_MAJOR 121
#define DEV_NAME "debug"

#define FPSGO_QUEUE          _IOW('g', 1, __u32)
#define FPSGO_DEQUEUE        _IOW('g', 3, __u32)
#define FPSGO_VSYNC          _IOW('g', 5, __u32)
#define FPSGO_QUEUE_LENGTH   _IOW('g', 6, __u32)
#define FPSGO_ACQUIRE_BUFFER _IOW('g', 7, __u32)

#define IOCTL_WRITE_AS _IOW('g', 8, int)
#define IOCTL_WRITE_GM _IOW('g', 9, int)
#define IOCTL_WRITE_TH _IOW('g', 10, int)
#define IOCTL_WRITE_FC _IOW('g', 11, int)
#define IOCTL_WRITE_IV _IOW('g', 12, int)
#define IOCTL_WRITE_NR _IOW('g', 13, int)
#define IOCTL_WRITE_SB _IOW('g', 14, int)

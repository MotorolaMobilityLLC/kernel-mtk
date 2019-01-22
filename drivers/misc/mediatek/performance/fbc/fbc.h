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
/*#include <mtk_ftrace.h>*/
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
/*#define MET_USER_EVENT_SUPPORT*/
#if TRACE
#include <mt-plat/met_drv.h>
#endif
/*#include "core/met_drv.h"*/
/*#include "core/trace.h"*/

#include <linux/platform_device.h>
#include <trace/events/sched.h>

#include <linux/ioctl.h>
#define	DEV_MAJOR	121
#define	DEV_NAME	"debug"
#define	DEV_IOCTLID	0xD0
#define	IOCTL_WRITE	_IOW(DEV_IOCTLID, 10, int)
#define	DEV_IOCTLID1 0xE0
#define	IOCTL_WRITE1 _IOW(DEV_IOCTLID1, 10, int)
#define	DEV_IOCTLID2 0xC0
#define	IOCTL_WRITE2 _IOW(DEV_IOCTLID2, 10, int)
#define	DEV_IOCTLID3 0xF0
#define	IOCTL_WRITE3 _IOW(DEV_IOCTLID3, 10, int)
/*#define SUPER_BOOST 30*/

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
/*extern int linear_real_boost(int);*/
/*extern int linear_real_boost_pid(int, int);*/


/*static void mt_power_pef_transfer(void);*/
/*static DEFINE_TIMER(mt_pp_transfer_timer, (void *)mt_power_pef_transfer, 0, 0);*/
static void mt_power_pef_transfer_work(void);
static DECLARE_WORK(mt_pp_work, (void *) mt_power_pef_transfer_work);
static DEFINE_SPINLOCK(tlock);

static int boost_value;
static int touch_boost_value;
static int fbc_debug;
static int fbc_touch; /* 0: no touch & no render, 1: touch 2: render only*/
static int fbc_touch_pre; /* 0: no touch & no render, 1: touch 2: render only*/
static int Twanted;
static int X_ms;
static int avgFT;
static int first_frame;
static int EMA;
static int boost_method;
static int frame_done;
static int SUPER_BOOST;
static int is_game;
static int is_30_fps;
/*static unsigned long long last_frame_ts;*/
/*static unsigned long long curr_frame_ts;*/

static struct hrtimer hrt;



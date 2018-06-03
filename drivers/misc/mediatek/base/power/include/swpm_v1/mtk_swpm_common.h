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

#ifndef __MTK_SWPM_COMMON_H__
#define __MTK_SWPM_COMMON_H__

#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <mt-plat/sync_write.h>


/* LOG */
#undef TAG
#define TAG     "[SWPM] "

#define swpm_err                swpm_info
#define swpm_warn               swpm_info
#define swpm_info(fmt, args...) pr_notice(TAG""fmt, ##args)
#define swpm_dbg(fmt, args...)                           \
	do {                                             \
		if (swpm_debug)                          \
			swpm_info(fmt, ##args);          \
		else                                     \
			pr_debug(TAG""fmt, ##args);      \
	} while (0)

#define swpm_readl(addr)	__raw_readl(addr)
#define swpm_writel(addr, val)	mt_reg_sync_writel(val, addr)

#define swpm_lock(lock)		mutex_lock(lock)
#define swpm_unlock(lock)	mutex_unlock(lock)

extern bool swpm_debug;
extern struct mutex swpm_mutex;
extern const struct of_device_id swpm_of_ids[];

extern int swpm_platform_init(void);
extern void swpm_send_init_ipi(unsigned int addr, unsigned int size,
	unsigned int ch_num);
#endif


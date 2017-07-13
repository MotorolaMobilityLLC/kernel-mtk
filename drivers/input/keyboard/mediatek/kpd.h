/*
 * (C) Copyright 2009
 * MediaTek <www.MediaTek.com>
 *
 * MT6516 Sensor IOCTL & data structure
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __KPD_H__
#define __KPD_H__

#include <linux/ioctl.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/atomic.h>
#include <hal_kpd.h>
#include <linux/kernel.h>
#include <linux/delay.h>

struct keypad_dts_data {
	u16 kpd_key_debounce;
	u16 kpd_use_extend_type;
	u16 kpd_hw_map_num;
	u16 kpd_hw_init_map[KPD_NUM_KEYS];
};
extern struct keypad_dts_data kpd_dts_data;

#define KPD_DEBUG	0
#define SET_KPD_KCOL		_IO('k', 29)

#define KPD_SAY		"kpd: "
#if KPD_DEBUG
#define kpd_print(fmt, arg...)	pr_debug(KPD_SAY fmt, ##arg)
#define kpd_info(fmt, arg...)	pr_debug(KPD_SAY fmt, ##arg)
#else
#define kpd_print(fmt, arg...)	do {} while (0)
#define kpd_info(fmt, arg...)	do {} while (0)
#endif

#endif				/* __KPD_H__ */

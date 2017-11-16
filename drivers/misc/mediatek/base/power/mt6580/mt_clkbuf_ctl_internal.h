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

/**
 * @file    mt_clk_buf_ctl.c
 * @brief   Driver for RF clock buffer control
 *
 */
#ifndef __MT_CLK_BUF_CTL_INTERNAL_H__
#define __MT_CLK_BUF_CTL_INTERNAL_H__

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/mutex.h>
#ifdef CONFIG_MTK_LEGACY
#include <mach/cust_clk_buf.h>
#endif /* CONFIG_MTK_LEGACY */
#else
#ifdef MTKDRV_CLKBUF_CTL /* for CTP */
#else
#include <gpio.h>
#endif
#endif

#ifdef __KERNEL__
#define RF_CK_BUF_REG            (spm_base + 0x620)
#define RF_CK_BUF_REG_SET        (spm_base + 0x624)
#define RF_CK_BUF_REG_CLR        (spm_base + 0x628)
#else
#define RF_CK_BUF_REG            0x10006620
#define RF_CK_BUF_REG_SET        0x10006624
#define RF_CK_BUF_REG_CLR        0x10006628
#endif

#include <mach/mt_clkbuf_ctl.h>

extern struct kobject *power_kobj;

#ifndef CONFIG_MTK_LEGACY
typedef enum {
	CLK_BUF_DISABLE		= 0,
	CLOCK_BUFFER_SW_CONTROL	= 1,
	CLOCK_BUFFER_HW_CONTROL = 2,
} CLK_BUF_STATUS;
#endif /* ! CONFIG_MTK_LEGACY */

#define CLKBUF_NUM      4

#define STA_CLK_ON      1
#define STA_CLK_OFF     0

#endif


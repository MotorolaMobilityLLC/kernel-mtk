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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <mach/mtk_clkmgr.h>
#include <mach/mtk_freqhopping.h>
#include "mt-plat/mtk_rtc.h"
#include "mtk_freqhopping_drv.h"

static int __init mt_power_management_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	pm_power_off = mt_power_off;

	/* mt_clkmgr_init(); */
	mt_freqhopping_init();
#endif

	return 0;
}

arch_initcall(mt_power_management_init);

MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");

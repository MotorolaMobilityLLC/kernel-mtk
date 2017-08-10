/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <mrdump.h>
#include <asm/memory.h>
#include <mach/wd_api.h>
#include "mrdump_private.h"

static void mrdump_hw_enable(bool enabled)
{
#if !defined(CONFIG_ARCH_MT6570) && !defined(CONFIG_ARCH_MT6757)
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res < 0)
		pr_alert("wd_ddr_reserved_mode, get wd api error %d\n", res);
	else
		wd_api->wd_dram_reserved_mode(enabled);
#endif
}

static void mrdump_reboot(void)
{
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res < 0) {
		pr_alert("arch_reset, get wd api error %d\n", res);
		while (1)
			cpu_relax();
	} else {
		wd_api->wd_sw_reset(1);
	}
}

const struct mrdump_platform mrdump_v1_platform = {
	.hw_enable = mrdump_hw_enable,
	.reboot = mrdump_reboot
};

int __init mrdump_init(void)
{
	mrdump_cblock_init();
	return mrdump_platform_init(&mrdump_v1_platform);
}

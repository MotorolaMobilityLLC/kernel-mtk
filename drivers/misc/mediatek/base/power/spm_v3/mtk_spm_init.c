/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <mtk_spm.h>
#include <mtk_sleep.h>
#include <mtk_cpuidle.h>
#include <mtk_spm_resource_req_internal.h>

static int __init mt_spm_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifndef CONFIG_MACH_MT6759
	mtk_cpuidle_init();
#endif
	spm_module_init();
#ifndef CONFIG_MACH_MT6759
	slp_module_init();
#endif
#endif

	spm_resource_req_init();

	return 0;
}

late_initcall(mt_spm_init);

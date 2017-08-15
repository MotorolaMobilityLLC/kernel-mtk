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

#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include <mach/mt_lbc.h>

/*--------------DEFAULT SETTING-------------------*/

#define TARGET_CORE (3)
#define TARGET_FREQ (1014000)
#define CLUSTER_NUM (2)

/*-----------------------------------------------*/

static int last_enable;

int perfmgr_get_target_core(void)
{
	return TARGET_CORE;
}

int perfmgr_get_target_freq(void)
{
	return TARGET_FREQ;
}

void perfmgr_boost(int enable, int core, int freq)
{
	struct ppm_limit_data core_to_set[CLUSTER_NUM];
	struct ppm_limit_data freq_to_set[CLUSTER_NUM];

	if (enable == last_enable)
		return;

	if (enable) {
		core_to_set[0].min = core;
		core_to_set[0].max = -1;
		freq_to_set[0].min = freq;
		freq_to_set[0].max = -1;
		core_to_set[1].min = -1;
		core_to_set[1].max = -1;
		freq_to_set[1].min = -1;
		freq_to_set[1].max = -1;
	} else {
		core_to_set[0].min = -1;
		core_to_set[0].max = -1;
		freq_to_set[0].min = -1;
		freq_to_set[0].max = -1;
		core_to_set[1].min = -1;
		core_to_set[1].max = -1;
		freq_to_set[1].min = -1;
		freq_to_set[1].max = -1;
	}

	last_enable = enable;

	update_userlimit_cpu_core(PPM_KIR_PERF_KERN, CLUSTER_NUM, core_to_set);
	update_userlimit_cpu_freq(PPM_KIR_PERF_KERN, CLUSTER_NUM, freq_to_set);
}


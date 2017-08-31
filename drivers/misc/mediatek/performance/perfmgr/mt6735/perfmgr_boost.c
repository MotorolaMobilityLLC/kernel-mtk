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

#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include "mt_hotplug_strategy.h"
#include "mt_cpufreq.h"

/*--------------DEFAULT SETTING-------------------*/

#define TARGET_CORE (3)
#define TARGET_FREQ (819000)

/*-----------------------------------------------*/

int perfmgr_get_target_core(void)
{
	return TARGET_CORE;
}

int perfmgr_get_target_freq(void)
{
	return TARGET_FREQ;
}

void init_perfmgr_boost(void)
{
	/* do nothing */
}

void perfmgr_boost(int enable, int core, int freq)
{
	if (enable) {
		/* hps */
		hps_set_cpu_num_base(BASE_PERF_SERV, core, 0);
		mt_cpufreq_set_min_freq(MT_CPU_DVFS_LITTLE, freq);
	} else {
		/* hps */
		hps_set_cpu_num_base(BASE_PERF_SERV, 1, 0);
		mt_cpufreq_set_min_freq(MT_CPU_DVFS_LITTLE, 0);
	}
}

/* redundant API */
void perfmgr_forcelimit_cpuset_cancel(void)
{

}

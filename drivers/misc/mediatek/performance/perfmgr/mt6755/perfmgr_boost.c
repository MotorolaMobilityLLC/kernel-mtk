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
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include "mt_ppm_api.h"

/*--------------DEFAULT SETTING-------------------*/

#define TARGET_CORE (3)
#define TARGET_FREQ (1014000)

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
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, core);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, freq);
	} else {
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, 0);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	}
}

/* redundant API */
void perfmgr_forcelimit_cpuset_cancel(void)
{

}

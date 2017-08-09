#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include "mt_hotplug_strategy.h"

/*--------------DEFAULT SETTING-------------------*/

#define TARGET_CORE (3)
#define TARGET_FREQ (858000)

/*-----------------------------------------------*/

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
	if (enable) {
		/* hps */
		hps_set_cpu_num_base(BASE_PERF_SERV, core, 0);
	} else {
		/* hps */
		hps_set_cpu_num_base(BASE_PERF_SERV, 1, 0);
	}
}


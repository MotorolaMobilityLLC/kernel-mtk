#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include "mt_ppm_api.h"

/*--------------DEFAULT SETTING-------------------*/

#define TARGET_CORE (3)
#define TARGET_FREQ (1066000)

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
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, core);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, freq);
	} else {
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, 0);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	}
}


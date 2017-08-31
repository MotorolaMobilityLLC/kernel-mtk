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

#include <linux/pm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
/* #include <linux/xlog.h> */

#include <linux/io.h>
#include <linux/uaccess.h>

/* #include "mach/irqs.h" */
#include <mt-plat/sync_write.h>
/* #include "mach/mt_reg_base.h" */
/* #include "mach/mt_typedefs.h" */
#include "mtk_spm.h"
#include "mtk_sleep.h"
#include "mtk_dcm.h"
#include "mtk_clkmgr.h"
#include "mtk_cpufreq.h"
#include "mtk_gpufreq.h"
/* #include "mach/mt_sleep.h" */
/* #include "mach/mt_dcm.h" */
/* #include <mach/mt_clkmgr.h> */
/* #include "mach/mt_cpufreq.h" */
/* #include "mach/mt_gpufreq.h" */
/* #include "mtk_cpuidle.h" */
#include "mtk_clkbuf_ctl.h"
/* #include "mach/mt_chip.h" */
#include <mach/mtk_freqhopping.h>
#include "mt-plat/mtk_rtc.h"
/* #include "mt_freqhopping_drv.h" */


/*int __attribute__ ((weak)) mt_cpu_dormant_init(void) { return MT_CPU_DORMANT_BYPASS; }*/

static int __init mt_power_management_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* unsigned int code = mt_get_chip_hw_code(); */

	pm_power_off = mt_power_off;

	/* cpu dormant driver init */
	/*mt_cpu_dormant_init();*/


	/*spm_module_init();*/ /* move to mtk_spm_init.c */
	/*slp_module_init();*/ /* move to mtk_spm_init.c */
	mt_freqhopping_init();

	/* mt_pm_log_init(); // power management log init */

	/* mt_dcm_init(); // dynamic clock management init */

#endif


	return 0;
}

arch_initcall(mt_power_management_init);


#if !defined(MT_DORMANT_UT)
static int __init mt_pm_late_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
/*	mt_idle_init(); */
	/* clk_buf_init(); *//* move to mtk_clkbuf_ctl.c */
#endif
	return 0;
}

late_initcall(mt_pm_late_init);
#endif				/* #if !defined (MT_DORMANT_UT) */


MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");

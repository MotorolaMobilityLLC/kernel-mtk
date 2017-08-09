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
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mt-plat/sync_write.h>
#include "mt_spm.h"
#include "mt_sleep.h"

static int __init mt_power_management_init(void)
{
	spm_module_init();
	/* slp_module_init(); */
	/* mt_clkmgr_init(); */

	return 0;
}
arch_initcall(mt_power_management_init);

MODULE_DESCRIPTION("MTK Power Management Init Driver");
MODULE_LICENSE("GPL");

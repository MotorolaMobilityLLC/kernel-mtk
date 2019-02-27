/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by vmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include <linux/delay.h>
#include "scp_feature_define.h"
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"
#include "scp_dvfs.h"

void scp_enable_sram(void)
{
	uint32_t reg_temp;

	/*enable sram, enable 1 block per time*/
	for (reg_temp = 0xffffffff; reg_temp != 0;) {
		reg_temp = reg_temp >> 1;
		writel(reg_temp, SCP_SRAM_PDN);
	}
	/*enable scp all TCM*/
	writel(0, SCP_CLK_CTRL_L1_SRAM_PD);
	writel(0, SCP_CLK_CTRL_TCM_TAIL_SRAM_PD);
}

/*
 * scp_sys_reset, reset scp
 */
int scp_sys_full_reset(void)
{
	pr_debug("[SCP]reset\n");

	/*copy loader to scp sram*/
	pr_debug("[SCP]copy to sram\n");
	memcpy_to_scp(SCP_TCM, (const void *)(size_t)scp_loader_base_virt
			, scp_loader_size);
	/*set info to sram*/
	pr_debug("[SCP]set firmware info to sram\n");
	writel(scp_fw_base_phys, SCP_TCM + 0x408);
	writel(scp_fw_size, SCP_TCM + 0x40C);

	return 0;
}


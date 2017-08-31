/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by kmalloc */
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
#include "sspm_define.h"
#include "sspm_helper.h"
#include "sspm_reservedmem.h"

#ifdef SSPM_EMI_PROTECTION_SUPPORT
#include <emi_mpu.h>

void sspm_set_emi_mpu(phys_addr_t base, phys_addr_t size)
{
	unsigned int shr_mem_phy_start, shr_mem_phy_end, shr_mem_mpu_id,
		     shr_mem_mpu_attr;
	shr_mem_mpu_id = SSPM_MPU_REGION_ID;
	shr_mem_phy_start = base;
	shr_mem_phy_end = base + size - 1;
	shr_mem_mpu_attr =
		SET_ACCESS_PERMISSON(FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
				NO_PROTECTION, FORBIDDEN, FORBIDDEN,
				NO_PROTECTION);
	pr_debug("[SSPM] MPU Start protect SSPM Share region<%d:%08x:%08x> %x\n",
			shr_mem_mpu_id, shr_mem_phy_start, shr_mem_phy_end,
			shr_mem_mpu_attr);
	emi_mpu_set_region_protection(shr_mem_phy_start,        /*START_ADDR */
			shr_mem_phy_end,  /*END_ADDR */
			shr_mem_mpu_id,   /*region */
			shr_mem_mpu_attr);

}
#endif

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
#include <linux/wakelock.h>
#include <linux/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include <linux/delay.h>
#include "scp_feature_define.h"
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"
#include "scp_dvfs.h"

#if ENABLE_SCP_EMI_PROTECTION
#include <mt_emi_api.h>

void set_scp_mpu(void)
{
	struct emi_region_info_t region_info;

	region_info.start = scp_mem_base_phys;
	region_info.end = scp_mem_base_phys + scp_mem_size - 0x1;
	region_info.region = MPU_REGION_ID_SCP_SMEM;
	SET_ACCESS_PERMISSION(region_info.apc, UNLOCK,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
		NO_PROTECTION, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
	emi_mpu_set_protection(&region_info);
}
#endif

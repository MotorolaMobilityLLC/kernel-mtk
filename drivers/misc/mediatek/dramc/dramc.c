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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <asm/cacheflush.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>

#include "mtk_dramc.h"

void __iomem *SLEEP_BASE_ADDR;

#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MTK_DRAMC_PASR)
static DEFINE_SPINLOCK(dramc_lock);
int acquire_dram_ctrl(void)
{
	unsigned int cnt;
	unsigned long save_flags;

	/* acquire SPM HW SEMAPHORE to avoid race condition */
	spin_lock_irqsave(&dramc_lock, save_flags);

	cnt = 2;
	do {
		if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) != 0x1) {
			writel(0x1, PDEF_SPM_AP_SEMAPHORE);
			if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1)
				break;
		}

		cnt--;
		/* pr_err("[DRAMC] wait for SPM HW SEMAPHORE\n"); */
		udelay(1);
	} while (cnt > 0);

	if (cnt == 0) {
		spin_unlock_irqrestore(&dramc_lock, save_flags);
		pr_warn("[DRAMC] can NOT get SPM HW SEMAPHORE!\n");
		return -1;
	}

	/* pr_err("[DRAMC] get SPM HW SEMAPHORE success!\n"); */

	spin_unlock_irqrestore(&dramc_lock, save_flags);
	return 0;
}

int release_dram_ctrl(void)
{
	/* release SPM HW SEMAPHORE to avoid race condition */
	if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x0) {
		pr_err("[DRAMC] has NOT acquired SPM HW SEMAPHORE\n");
		/* BUG(); */
	}

	writel(0x1, PDEF_SPM_AP_SEMAPHORE);
	if ((readl(PDEF_SPM_AP_SEMAPHORE) & 0x1) == 0x1) {
		pr_err("[DRAMC] release SPM HW SEMAPHORE fail!\n");
		/* BUG(); */
	}
	/* pr_err("[DRAMC] release SPM HW SEMAPHORE success!\n"); */
	return 0;
}
#endif

#ifdef PLAT_DBG_INFO_MANAGE
static void __iomem *plat_dbg_info_base[INFO_TYPE_MAX];
static unsigned int plat_dbg_info_size[INFO_TYPE_MAX];

static int __init plat_dbg_info_init(void)
{
	unsigned int temp_base[INFO_TYPE_MAX];
	unsigned int i;
	int ret;

	if (of_chosen) {
		ret = of_property_read_u32_array(of_chosen, "plat_dbg_info,base", temp_base, INFO_TYPE_MAX);
		ret |= of_property_read_u32_array(of_chosen, "plat_dbg_info,size", plat_dbg_info_size, INFO_TYPE_MAX);

		if (ret != 0) {
			pr_err("[PLAT DBG INFO] cannot find property\n");
			return -ENODEV;
		}

		for (i = 0; i < INFO_TYPE_MAX; i++) {
			if (temp_base[i] != 0)
				plat_dbg_info_base[i] = ioremap(temp_base[i], plat_dbg_info_size[i]);
			pr_warn("[PLAT DBG INFO] %d: 0x%x(%p), %d\n",
				i, temp_base[i], plat_dbg_info_base[i], plat_dbg_info_size[i]);
		}
	} else {
		pr_err("[PLAT DBG INFO] cannot find node \"of_chosen\"\n");
		return -ENODEV;
	}

	return 0;
}

void __iomem *get_dbg_info_base(DBG_INFO_TYPE info_type)
{
	if (info_type >= TYPE_END)
		return NULL;

	return plat_dbg_info_base[info_type];
}

unsigned int get_dbg_info_size(DBG_INFO_TYPE info_type)
{
	if (info_type >= TYPE_END)
		return 0;

	return plat_dbg_info_size[info_type];
}

core_initcall(plat_dbg_info_init);
#endif /* PLAT_DBG_INFO_MANAGE */


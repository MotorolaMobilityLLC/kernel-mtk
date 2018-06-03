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
#include <linux/slab.h>
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

#if defined(CONFIG_MACH_MT6757) || defined(CONFIG_MTK_DRAMC_PASR) || \
	defined(SW_ZQCS) || defined(SW_TX_TRACKING) || defined(DRAMC_MEMTEST_DEBUG_SUPPORT)
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
static void __iomem **plat_dbg_info_base;
static unsigned int *plat_dbg_info_size;
static unsigned int *plat_dbg_info_key;
static unsigned int plat_dbg_info_max;

static int __init plat_dbg_info_init(void)
{
	unsigned int *temp_base;
	unsigned int i;
	int ret;

	if (of_chosen) {
		ret = of_property_read_u32(of_chosen, "plat_dbg_info,max", &plat_dbg_info_max);

		temp_base = kmalloc_array(plat_dbg_info_max, sizeof(unsigned int), GFP_KERNEL);
		plat_dbg_info_key = kmalloc_array(plat_dbg_info_max, sizeof(unsigned int), GFP_KERNEL);
		plat_dbg_info_size = kmalloc_array(plat_dbg_info_max, sizeof(unsigned int), GFP_KERNEL);
		plat_dbg_info_base = kmalloc_array(plat_dbg_info_max, sizeof(void __iomem *), GFP_KERNEL);

		if ((temp_base == NULL) || (plat_dbg_info_key == NULL) ||
		    (plat_dbg_info_size == NULL) || (plat_dbg_info_base == NULL)) {
			pr_err("[PLAT DBG INFO] cannot allocate memory\n");
			return -ENODEV;
		}

		ret |= of_property_read_u32_array(of_chosen, "plat_dbg_info,key",
			plat_dbg_info_key, plat_dbg_info_max);
		ret |= of_property_read_u32_array(of_chosen, "plat_dbg_info,base",
			temp_base, plat_dbg_info_max);
		ret |= of_property_read_u32_array(of_chosen, "plat_dbg_info,size",
			plat_dbg_info_size, plat_dbg_info_max);
		if (ret != 0) {
			pr_err("[PLAT DBG INFO] cannot find property\n");
			return -ENODEV;
		}

		for (i = 0; i < plat_dbg_info_max; i++) {
			if (temp_base[i] != 0)
				plat_dbg_info_base[i] = ioremap(temp_base[i], plat_dbg_info_size[i]);
			else
				plat_dbg_info_base[i] = NULL;

			pr_warn("[PLAT DBG INFO] 0x%x: 0x%x(%p), %d\n",
				plat_dbg_info_key[i], temp_base[i], plat_dbg_info_base[i], plat_dbg_info_size[i]);
		}

		kfree(temp_base);
	} else {
		pr_err("[PLAT DBG INFO] cannot find node \"of_chosen\"\n");
		return -ENODEV;
	}

	return 0;
}

void __iomem *get_dbg_info_base(unsigned int key)
{
	unsigned int i;

	for (i = 0; i < plat_dbg_info_max; i++) {
		if (plat_dbg_info_key[i] == key)
			return plat_dbg_info_base[i];
	}

	return NULL;
}

unsigned int get_dbg_info_size(unsigned int key)
{
	unsigned int i;

	for (i = 0; i < plat_dbg_info_max; i++) {
		if (plat_dbg_info_key[i] == key)
			return plat_dbg_info_size[i];
	}

	return 0;
}

core_initcall(plat_dbg_info_init);
#endif /* PLAT_DBG_INFO_MANAGE */


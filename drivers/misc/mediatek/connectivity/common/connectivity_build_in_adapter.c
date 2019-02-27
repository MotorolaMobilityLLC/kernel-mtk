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

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG "[WMT-CONSYS-HW]"

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/export.h>

/*device tree mode*/
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#endif

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of_reserved_mem.h>

#include "connectivity_build_in_adapter.h"

#if 0

phys_addr_t gConEmiPhyBase;
EXPORT_SYMBOL(gConEmiPhyBase);

/*Reserved memory by device tree!*/

int reserve_memory_consys_fn(struct reserved_mem *rmem)
{
	WMT_PLAT_WARN_FUNC(DFT_TAG "[W]%s: name: %s,base: 0x%llx,size: 0x%llx\n",
		__func__, rmem->name, (unsigned long long)rmem->base,
		(unsigned long long)rmem->size);
	gConEmiPhyBase = rmem->base;
	return 0;
}

RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,consys-reserve-memory",
			reserve_memory_consys_fn);

#endif

void connectivity_export_show_stack(struct task_struct *tsk, unsigned long *sp)
{
	show_stack(tsk, sp);
}
EXPORT_SYMBOL(connectivity_export_show_stack);

void connectivity_export_tracing_record_cmdline(struct task_struct *tsk)
{
	tracing_record_cmdline(tsk);
}
EXPORT_SYMBOL(connectivity_export_tracing_record_cmdline);

#ifdef CPU_BOOST
void connectivity_export_mt_ppm_sysboost_freq(enum ppm_sysboost_user user,
					      unsigned int freq)
{
	mt_ppm_sysboost_freq(user, freq);
}
EXPORT_SYMBOL(connectivity_export_mt_ppm_sysboost_freq);

void connectivity_export_mt_ppm_sysboost_core(enum ppm_sysboost_user user,
					      unsigned int core_num)
{
	mt_ppm_sysboost_core(user, core_num);
}
EXPORT_SYMBOL(connectivity_export_mt_ppm_sysboost_core);

void connectivity_export_mt_ppm_sysboost_set_core_limit(
				enum ppm_sysboost_user user,
				unsigned int cluster,
				int min_core, int max_core)
{
	mt_ppm_sysboost_set_core_limit(user, cluster, min_core, max_core);
}
EXPORT_SYMBOL(connectivity_export_mt_ppm_sysboost_set_core_limit);

void connectivity_export_mt_ppm_sysboost_set_freq_limit(
				enum ppm_sysboost_user user,
				unsigned int cluster,
				int min_freq, int max_freq)
{
	mt_ppm_sysboost_set_freq_limit(user, cluster, min_freq, max_freq);
}
EXPORT_SYMBOL(connectivity_export_mt_ppm_sysboost_set_freq_limit);
#endif

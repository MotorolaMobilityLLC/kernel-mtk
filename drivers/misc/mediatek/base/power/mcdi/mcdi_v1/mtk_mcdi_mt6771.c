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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>

#include <mtk_idle_mcdi.h>

#include <mcdi_v1/mtk_mcdi_mt6771.h>
#include <mtk_mcdi_state.h>
#include <mtk_mcdi_governor.h>
#include <mt-plat/mtk_secure_api.h>
#include <mtk_mcdi.h>
#include <mtk_mcdi_mbox.h>
#include <sspm_mbox.h>

static DEFINE_SPINLOCK(async_wakeup_en_spin_lock);
static int mcdi_idle_state_mapping[NR_TYPES] = {
	MCDI_STATE_DPIDLE,	/* IDLE_TYPE_DP */
	MCDI_STATE_SODI3,	/* IDLE_TYPE_SO3 */
	MCDI_STATE_SODI,	/* IDLE_TYPE_SO */
	MCDI_STATE_CLUSTER_OFF	/* IDLE_TYPE_RG */
};

static const char mcdi_node_name[] = "mediatek,mt6771-mcdi";

int mcdi_get_mcdi_idle_state(int idx)
{
	return mcdi_idle_state_mapping[idx];
}

void mcdi_status_init(void)
{
	set_mcdi_enable_status(true);
}

/*
 * update avail cpu mask to ACAO_WAKEUP_EN register when online cpu changed or
 * isolation cpu changed.
 */

void mcdi_update_async_wakeup_enable(void)
{
	unsigned long flags;
	unsigned int online_cpu_mask, iso_cpu_mask, avail_cpu_mask;
	int cpu;

	spin_lock_irqsave(&async_wakeup_en_spin_lock, flags);

	online_cpu_mask = mcdi_mbox_read(MCDI_MBOX_AVAIL_CPU_MASK);
	iso_cpu_mask = mcdi_mbox_read(MCDI_MBOX_CPU_ISOLATION_MASK);

	/*
	 * Find the smallest available cpu
	 * 1. online cpu info(sw view)
	 * 2. CPUs that not be isolated
	 */
	avail_cpu_mask = online_cpu_mask & ~iso_cpu_mask;

	for (cpu = 0; cpu < NF_CPU; cpu++) {
		if (avail_cpu_mask & (1 << cpu))
			break;
	}

	mt_secure_call(MTK_SIP_KERNEL_MCDI_ARGS,
		MCDI_SMC_EVENT_ASYNC_WAKEUP_EN, (cpu % NF_CPU), 0);

	spin_unlock_irqrestore(&async_wakeup_en_spin_lock, flags);
}
void mcdi_of_init(void)
{
	struct device_node *node = NULL;

	/* MCDI sysram base */
	node = of_find_compatible_node(NULL, NULL, mcdi_node_name);

	if (!node)
		pr_info("node '%s' not found!\n", mcdi_node_name);

	mcdi_sysram_base = of_iomap(node, 0);

	if (!mcdi_sysram_base)
		pr_info("node '%s' can not iomap!\n", mcdi_node_name);

	pr_info("mcdi_sysram_base = %p\n", mcdi_sysram_base);
}

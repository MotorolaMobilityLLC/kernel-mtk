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

#include <mtk_idle_mcdi.h>

#include <mtk_mcdi_mt6758.h>
#include <mtk_mcdi_state.h>
#include <mtk_mcdi_governor.h>

static int mcdi_idle_state_mapping[NR_TYPES] = {
	MCDI_STATE_DPIDLE,		/* IDLE_TYPE_DP */
	MCDI_STATE_SODI3,		/* IDLE_TYPE_SO3 */
	MCDI_STATE_SODI,		/* IDLE_TYPE_SO */
	MCDI_STATE_CLUSTER_OFF,	/* IDLE_TYPE_MC */
	MCDI_STATE_CLUSTER_OFF,	/* IDLE_TYPE_SL */
	MCDI_STATE_CLUSTER_OFF	/* IDLE_TYPE_RG */
};

static const char mcdi_node_name[] = "mediatek,mt6763-mcdi";

int mcdi_get_mcdi_idle_state(int idx)
{
	return mcdi_idle_state_mapping[idx];
}

void mcdi_status_init(void)
{
	set_mcdi_enable_status(true);
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

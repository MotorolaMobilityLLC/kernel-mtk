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

#include <linux/of_address.h>
#include <linux/of.h>

#include "ddp_misc.h"
#include "ddp_reg.h"
#include "ddp_log.h"

unsigned long get_smi_larb_va(unsigned int larb)
{
	struct device_node *node = NULL;
	char smi_larb_dt_name[25];
	unsigned long va = 0;

	snprintf(smi_larb_dt_name, sizeof(smi_larb_dt_name),
		"mediatek,smi_larb%u", larb);
	node = of_find_compatible_node(NULL, NULL, smi_larb_dt_name);

	if (node != NULL)
		va = (unsigned long)of_iomap(node, 0);

	if (va == 0)
		DDP_PR_ERR("Cannot get smi larb va, smi DT name: %s\n",
			   smi_larb_dt_name);

	return va;
}

void enable_smi_ultra(unsigned int larb, unsigned int value)
{
	unsigned long va = get_smi_larb_va(larb);
	unsigned long offset = SMI_LARB_FORCE_ULTRA;
	unsigned int prev_value, cur_value;

	if (va == 0)
		return;

	prev_value = DISP_REG_GET(va + offset);

	DISP_REG_SET(NULL, va + offset, value);

	cur_value = DISP_REG_GET(va + offset);

	DDPMSG("enable smi ultra, larb:%u, prev:0x%x, cur:0x%x\n",
		larb, prev_value, cur_value);

}

void enable_smi_preultra(unsigned int larb, unsigned int value)
{
	unsigned long va = get_smi_larb_va(larb);
	unsigned long offset = SMI_LARB_FORCE_PREULTRA;
	unsigned int prev_value, cur_value;

	if (va == 0)
		return;

	prev_value = DISP_REG_GET(va + offset);

	DISP_REG_SET(NULL, va + offset, value);

	cur_value = DISP_REG_GET(va + offset);

	DDPMSG("enable smi pre-ultra, larb:%u, prev:0x%x, cur:0x%x\n",
		larb, prev_value, cur_value);
}


void disable_smi_ultra(unsigned int larb, unsigned int value)
{
	unsigned long va = get_smi_larb_va(larb);
	unsigned long offset = SMI_LARB_ULTRA_DIS;
	unsigned int prev_value, cur_value;

	if (va == 0)
		return;

	prev_value = DISP_REG_GET(va + offset);

	DISP_REG_SET(NULL, va + offset, value);

	cur_value = DISP_REG_GET(va + offset);

	DDPMSG("disable smi ultra, larb:%u, prev:0x%x, cur:0x%x\n",
		larb, prev_value, cur_value);
}

void disable_smi_preultra(unsigned int larb, unsigned int value)
{
	unsigned long va = get_smi_larb_va(larb);
	unsigned long offset = SMI_LARB_PREULTRA_DIS;
	unsigned int prev_value, cur_value;

	if (va == 0)
		return;

	prev_value = DISP_REG_GET(va + offset);

	DISP_REG_SET(NULL, va + offset, value);

	cur_value = DISP_REG_GET(va + offset);

	DDPMSG("disable smi pre-ultra, larb:%u, prev:0x%x, cur:0x%x\n",
		larb, prev_value, cur_value);
}

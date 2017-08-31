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

#include <linux/spinlock.h>

#include "mtk_spm_resource_req.h"

DEFINE_SPINLOCK(spm_resource_desc_update_lock);

struct spm_resource_desc {
	int id;
	unsigned int user_usage[2];		/* 64 bits */
};

static struct spm_resource_desc resc_desc[NF_SPM_RESOURCE];

static const char * const spm_resource_name[] = {
	"mainpll",
	"dram",
	"26m",
	"axi_bus"
};

bool spm_resource_req(unsigned int user, unsigned int req_mask)
{
	int i;
	int value = 0;
	unsigned int field = 0;
	unsigned int offset = 0;
	unsigned long flags;

	if (user >= NF_SPM_RESOURCE_USER)
		return false;

	spin_lock_irqsave(&spm_resource_desc_update_lock, flags);

	for (i = 0; i < NF_SPM_RESOURCE; i++) {

		value = !!(req_mask & (1 << i));
		field = user / 32;
		offset = user % 32;

		if (value)
			resc_desc[i].user_usage[field] |= (1 << offset);
		else
			resc_desc[i].user_usage[field] &= ~(1 << offset);
	}

	spin_unlock_irqrestore(&spm_resource_desc_update_lock, flags);

	return true;
}

unsigned int spm_get_resource_usage(void)
{
	int i;
	unsigned int resource_usage = 0;
	int resource_in_use = 0;
	unsigned long flags;

	spin_lock_irqsave(&spm_resource_desc_update_lock, flags);

	for (i = 0; i < NF_SPM_RESOURCE; i++) {

		resource_in_use = !!(resc_desc[i].user_usage[0] | resc_desc[i].user_usage[1]);

		if (resource_in_use)
			resource_usage |= (1 << i);
	}

	spin_unlock_irqrestore(&spm_resource_desc_update_lock, flags);

	return resource_usage;
}


/* TODO */
bool resc_desc_init(void)
{
	int i;

	for (i = 0; i < NF_SPM_RESOURCE_USER; i++)
		resc_desc[i].id = i;

	return true;
}

/* Debug only */
void spm_resource_req_dump(void)
{
	int i;
	unsigned long flags;

	pr_err("resource_req:\n");

	spin_lock_irqsave(&spm_resource_desc_update_lock, flags);

	for (i = 0; i < NF_SPM_RESOURCE; i++)
		pr_err("[%s]: %x, %x\n", spm_resource_name[i], resc_desc[i].user_usage[0], resc_desc[i].user_usage[1]);

	spin_unlock_irqrestore(&spm_resource_desc_update_lock, flags);
}


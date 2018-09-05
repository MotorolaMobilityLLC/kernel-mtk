/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/bitmap.h>
#include <asm/page.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "gsm.h"
#include "mdla.h"

#define GSM_SIZE (SZ_1M/PAGE_SIZE)
DECLARE_BITMAP(gsm_bitmap, GSM_SIZE);
DEFINE_SPINLOCK(gsm_lock);

void *gsm_alloc(size_t size)
{
	unsigned long flags;
	int pageno;
	int order = get_order(size);

	spin_lock_irqsave(&gsm_lock, flags);

	pageno = bitmap_find_free_region(gsm_bitmap, GSM_SIZE, order);
	spin_unlock_irqrestore(&gsm_lock, flags);
	if (unlikely(pageno < 0))
		return 0;
	return apu_mdla_gsm_top + (pageno << PAGE_SHIFT);
}

void *gsm_mva_to_virt(u32 mva)
{
	return (void *)(mva - GSM_MVA_BASE + apu_mdla_gsm_top);
}
u32 gsm_virt_to_mva(void *vaddr)
{
	if (vaddr)
		return (u32)(vaddr - apu_mdla_gsm_top + GSM_MVA_BASE);
	else
		return 0xFFFFFFFF;
}

int gsm_release(void *vaddr, size_t size)
{
	unsigned long flags;
	int order = get_order(size);
	int page = (vaddr - apu_mdla_gsm_top) >> PAGE_SHIFT;

	spin_lock_irqsave(&gsm_lock, flags);
	bitmap_release_region(gsm_bitmap, page, order);
	spin_unlock_irqrestore(&gsm_lock, flags);
	return 0;
}


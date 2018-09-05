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

#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/bitmap.h>

#include "mdla_hw_reg.h"
#include "mdla_pmu.h"
#include "mdla.h"

#define PMU_SIZE (16)

DECLARE_BITMAP(pmu_bitmap, PMU_SIZE);
DEFINE_SPINLOCK(pmu_lock);

static unsigned int pmu_reg_read(u32 offset)
{
	return ioread32(apu_mdla_biu_top + offset);
}

static void pmu_reg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_biu_top + offset);
}

int pmu_set_perf_event(u32 interface, u32 event)
{
	u32 reg;
	unsigned long flags;
	int handle;

	spin_lock_irqsave(&pmu_lock, flags);
	handle = bitmap_find_free_region(pmu_bitmap, PMU_SIZE, 0);
	spin_unlock_irqrestore(&pmu_lock, flags);
	if (unlikely(handle < 0))
		return -EFAULT;

	reg = pmu_reg_read(PMU_CFG_PMCR);

	/* Enable pmu_cnt_enX */
	reg |= 1 << (handle+17);
	pmu_reg_write(reg, PMU_CFG_PMCR);
	reg = (interface << 16) | event;
	pmu_reg_write(reg, PMU_EVENT_OFFSET +
		(handle) * PMU_CNT_SHIFT);
	return handle;
}

int pmu_unset_perf_event(int handle)
{
	u32 reg;

	bitmap_release_region(pmu_bitmap, handle, 0);
	reg = pmu_reg_read(PMU_CFG_PMCR);
	/* Enable pmu_cnt_enX */
	reg &= ~(1 << (handle+17));
	reg &= ~(0x6);
	pmu_reg_write(reg, PMU_CFG_PMCR);
	return 0;
}

int pmu_get_perf_event(int handle)
{
	return pmu_reg_read(PMU_EVENT_OFFSET +
		(handle) * PMU_CNT_SHIFT) >> 16;
}

int pmu_get_perf_counter(int handle)
{
	u32 reg = pmu_reg_read(PMU_CFG_PMCR);

	if ((1<<PMU_CLR_CMDE_SHIFT) & reg)
		return pmu_reg_read(PMU_CNT_OFFSET + 4 +
			(handle) * PMU_CNT_SHIFT);
	else
		return pmu_reg_read(PMU_CNT_OFFSET +
			(handle) * PMU_CNT_SHIFT);
}

u32 pmu_get_perf_start(void)
{
	return pmu_reg_read(PMU_START_TSTAMP);
}

u32 pmu_get_perf_end(void)
{
	return pmu_reg_read(PMU_END_TSTAMP);
}

u32 pmu_get_perf_cycle(void)
{
	return pmu_reg_read(PMU_CYCLE);
}

void pmu_reset_counter(void)
{
	pmu_reg_write(pmu_reg_read(PMU_CFG_PMCR)|0x2, PMU_CFG_PMCR);
}

void pmu_reset_cycle(void)
{
	pmu_reg_write(pmu_reg_read(PMU_CFG_PMCR)|0x4, PMU_CFG_PMCR);
}

void pmu_perf_set_mode(u32 mode)
{
	u32 reg = pmu_reg_read(PMU_CFG_PMCR);

	reg &= ~(1<<PMU_CLR_CMDE_SHIFT);
	pmu_reg_write(reg|(mode<<PMU_CLR_CMDE_SHIFT), PMU_CFG_PMCR);
}

void pmu_init(void)
{
	pmu_reg_write(CFG_PMCR_DEFAULT|0x4|0x2, PMU_CFG_PMCR);
}


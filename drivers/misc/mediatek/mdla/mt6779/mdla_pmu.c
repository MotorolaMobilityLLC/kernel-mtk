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
#include "mdla_debug.h"

#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/bitmap.h>

#include "mdla_hw_reg.h"
#include "mdla_pmu.h"
#include "mdla.h"

DECLARE_BITMAP(pmu_bitmap, MDLA_PMU_COUNTERS);
DEFINE_SPINLOCK(pmu_lock);

unsigned int pmu_reg_read(u32 offset)
{
	return ioread32(apu_mdla_biu_top + offset);
}

static void pmu_reg_write(u32 value, u32 offset)
{
	iowrite32(value, apu_mdla_biu_top + offset);
}

#define pmu_reg_set(mask, offset) \
	pmu_reg_write(pmu_reg_read(offset) | (mask), (offset))

#define pmu_reg_clear(mask, offset) \
	pmu_reg_write(pmu_reg_read(offset) & ~(mask), (offset))

int pmu_event_set(u32 handle, u32 val)
{
	u32 mask;

	if (handle >= MDLA_PMU_COUNTERS)
		return -EINVAL;

	mask = 1 << (handle+17);

	if (val == 0xFFFFFFFF) {
		mdla_debug("%s: clear pmu counter[%d]\n",
			__func__, handle);
		pmu_reg_write(0, PMU_EVENT_OFFSET +
			(handle) * PMU_CNT_SHIFT);
		pmu_reg_clear(mask, PMU_CFG_PMCR);
	} else {
		mdla_debug("%s: set pmu counter[%d] = 0x%x\n",
			__func__, handle, val);
		pmu_reg_write(val, PMU_EVENT_OFFSET +
			(handle) * PMU_CNT_SHIFT);
		pmu_reg_set(mask, PMU_CFG_PMCR);
	}

	return 0;
}

int pmu_set_perf_event(u32 interface, u32 event)
{
	unsigned long flags;
	int handle;

	spin_lock_irqsave(&pmu_lock, flags);
	handle = bitmap_find_free_region(pmu_bitmap, MDLA_PMU_COUNTERS, 0);
	spin_unlock_irqrestore(&pmu_lock, flags);
	if (unlikely(handle < 0))
		return -EFAULT;

	pmu_event_set(handle, ((interface << 16) | event));

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

void pmu_get_perf_counters(u32 out[MDLA_PMU_COUNTERS])
{
	int i;
	u32 offset = PMU_CNT_OFFSET;
	u32 reg = pmu_reg_read(PMU_CFG_PMCR);

	if ((1<<PMU_CLR_CMDE_SHIFT) & reg)
		offset = offset + 4;

	for (i = 0; i < MDLA_PMU_COUNTERS; i++)
		out[i] = pmu_reg_read(offset + (i * PMU_CNT_SHIFT));
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
	mdla_debug("mdla: %s\n", __func__);
	pmu_reg_set(PMU_PMCR_CNT_RST, PMU_CFG_PMCR);
	while (pmu_reg_read(PMU_CFG_PMCR) &
		PMU_PMCR_CNT_RST) {
	}
}

void pmu_reset_cycle(void)
{
	mdla_debug("mdla: %s\n", __func__);
	pmu_reg_set((PMU_PMCR_CCNT_EN | PMU_PMCR_CCNT_RST), PMU_CFG_PMCR);
	while (pmu_reg_read(PMU_CFG_PMCR) &
		PMU_PMCR_CCNT_RST) {
	}
}

void pmu_perf_set_mode(u32 mode)
{
	u32 reg = pmu_reg_read(PMU_CFG_PMCR);

	reg &= ~(1<<PMU_CLR_CMDE_SHIFT);
	pmu_reg_write(reg|(mode<<PMU_CLR_CMDE_SHIFT), PMU_CFG_PMCR);
}

void pmu_init(void)
{
	pmu_reg_write((CFG_PMCR_DEFAULT|
		PMU_PMCR_CCNT_RST|PMU_PMCR_CNT_RST), PMU_CFG_PMCR);
	while (pmu_reg_read(PMU_CFG_PMCR) &
		(PMU_PMCR_CCNT_RST|PMU_PMCR_CNT_RST)) {
	}
}


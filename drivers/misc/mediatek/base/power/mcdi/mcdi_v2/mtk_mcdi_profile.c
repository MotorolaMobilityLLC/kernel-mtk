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

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/tick.h>

#include "mtk_mcdi.h"
#include "mtk_mcdi_profile.h"

static bool mcdi_prof_en;
static int mcdi_prof_target_cpu;
static unsigned int mcdi_profile_sum_us[NF_MCDI_PROFILE - 1];
static unsigned long long mcdi_profile_ns[NF_MCDI_PROFILE];
static unsigned int mcdi_profile_count;

static DEFINE_SPINLOCK(mcdi_prof_spin_lock);

void set_mcdi_profile_sampling(int en)
{
	int i = 0;
	unsigned long flags;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	mcdi_prof_en = en;

	if (en) {

		for (i = 0; i < NF_MCDI_PROFILE; i++)
			mcdi_profile_ns[i] = 0;

		for (i = 0; i < (NF_MCDI_PROFILE - 1); i++)
			mcdi_profile_sum_us[i] = 0;

		mcdi_profile_count = 0;
	}

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	if (!en) {
		pr_info("cpu = %d, sample cnt = %u\n", mcdi_prof_target_cpu, mcdi_profile_count);

		for (i = 0; i < (NF_MCDI_PROFILE - 1); i++)
			pr_info("%d: %u\n", i, mcdi_profile_sum_us[i]);
	}
}

void set_mcdi_profile_target_cpu(int cpu)
{
	unsigned long flags;

	if (!(cpu >= 0 && cpu < NF_CPU))
		return;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	mcdi_prof_target_cpu = cpu;

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);
}

void mcdi_profile_ts(int idx)
{
	unsigned long flags;
	bool en = false;

	if (!(smp_processor_id() == mcdi_prof_target_cpu))
		return;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	en = mcdi_prof_en;

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	if (!en)
		return;

	mcdi_profile_ns[idx] = sched_clock();
}

void mcdi_profile_calc(void)
{
	unsigned long flags;
	bool en = false;
	int i = 0;

	if (!(smp_processor_id() == mcdi_prof_target_cpu))
		return;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	en = mcdi_prof_en;

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	if (!en)
		return;

	for (i = 0; i < (NF_MCDI_PROFILE - 1); i++)
		mcdi_profile_sum_us[i] += (unsigned int)div64_u64((mcdi_profile_ns[i + 1] -
								mcdi_profile_ns[i]), 1000);

	mcdi_profile_count++;
}

int get_mcdi_profile_cpu(void)
{
	unsigned long flags;
	int cpu = 0;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	cpu = mcdi_prof_target_cpu;

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	return cpu;
}

unsigned int get_mcdi_profile_cnt(void)
{
	unsigned long flags;
	unsigned int cnt;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	cnt = mcdi_profile_count;

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	return cnt;
}

unsigned int get_mcdi_profile_sum_us(int idx)
{
	unsigned long flags;
	unsigned int sum_us;

	if (!(idx >= 0 && idx < NF_MCDI_PROFILE - 1))
		return 0;

	spin_lock_irqsave(&mcdi_prof_spin_lock, flags);

	sum_us = mcdi_profile_sum_us[idx];

	spin_unlock_irqrestore(&mcdi_prof_spin_lock, flags);

	return sum_us;
}


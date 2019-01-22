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

#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/sched/prio.h>

#include "iccs.h"

void policy_update(struct iccs_governor *gov, enum transition new_policy)
{
	if (unlikely(!gov))
		return;

	if (gov->policy == new_policy)
		return;

	switch (new_policy) {
	case BENCHMARK:
		if (gov->policy != ONDEMAND)
			break;
	case PERFORMANCE:
		spin_lock(&gov->spinlock);
		gov->iccs_cache_shared_useful_bitmask = 0x7;
		gov->policy = new_policy;
		spin_unlock(&gov->spinlock);
		break;
	case ONDEMAND:
		hrtimer_cancel(&gov->hr_timer);
		spin_lock(&gov->spinlock);
		gov->policy = new_policy;
		spin_unlock(&gov->spinlock);
		hrtimer_start(&gov->hr_timer, gov->sampling, HRTIMER_MODE_REL);
		break;
	case POWERSAVE:
		spin_lock(&gov->spinlock);
		gov->iccs_cache_shared_useful_bitmask = 0x0;
		gov->policy = new_policy;
		spin_unlock(&gov->spinlock);
		break;
	default:
		break;
	}
}

void sampling_update(struct iccs_governor *gov, unsigned long long new_rate)
{
	unsigned long long rate = max(new_rate, ICCS_MIN_SAMPLING_RATE);

	if (unlikely(!gov))
		return;

	gov->sampling = ktime_set(0, MS_TO_NS(rate));
}

/* To get system status to determine whether to enable ICCS */
static int iccs_governor_task(void *data)
{
	struct iccs_governor *gov = data;
	unsigned char cache_shared_useful_bitmask = 0x0;

	if (unlikely(!gov))
		return -EINVAL;

	while (1) {

		if (gov->policy == ONDEMAND) {
			/* TODO:
			 * 1. Collect EMI BW/ Cache miss rate..
			 * 2. Update the iccs_cache_shared_useful_bitmask
			 */

			hrtimer_cancel(&gov->hr_timer);
			spin_lock(&gov->spinlock);
			gov->iccs_cache_shared_useful_bitmask = cache_shared_useful_bitmask;
			spin_unlock(&gov->spinlock);
			hrtimer_start(&gov->hr_timer, gov->sampling, HRTIMER_MODE_REL);
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static enum hrtimer_restart iccs_governor_timer(struct hrtimer *hrtimer)
{
	struct iccs_governor *gov = container_of(hrtimer, struct iccs_governor, hr_timer);

	if (unlikely(!gov)) {
		pr_err("Governor is Null\n");
		return HRTIMER_NORESTART;
	}

	if (unlikely(!gov->task))
		pr_err("Governor Thread is Null\n");
	else
		wake_up_process(gov->task);

	return HRTIMER_NORESTART;
}

int governor_activation(struct iccs_governor *gov)
{
	if (unlikely(!gov))
		return -EINVAL;

	/* Create kthread */
	gov->task = kthread_create(iccs_governor_task, (void *)gov, "iccs_governor");
	if (IS_ERR(gov->task))
		return PTR_ERR(gov->task);

	set_user_nice(gov->task, MIN_NICE);
	get_task_struct(gov->task);

	/* Initialize a hr_timer */
	hrtimer_init(&gov->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gov->hr_timer.function = (void *)&iccs_governor_timer;
	hrtimer_start(&gov->hr_timer, gov->sampling, HRTIMER_MODE_REL);

	return 0;
}

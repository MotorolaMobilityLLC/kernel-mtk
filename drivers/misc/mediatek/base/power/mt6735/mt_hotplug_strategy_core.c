/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <asm-generic/bug.h>

#include "mt_hotplug_strategy_internal.h"
#define MS_TO_NS(x)     (x * 1E6L)
/*============================================================================*/
/* Global variable definition */
/*============================================================================*/
static unsigned long long hps_cancel_time;
static ktime_t ktime;
/*============================================================================*/
/* Local function definition */
/*============================================================================*/
/*
 * hps timer callback
 */
static int _hps_timer_callback(unsigned long data)
{
	/*hps_warn("_hps_timer_callback\n"); */
	if (hps_ctxt.tsk_struct_ptr)
		wake_up_process(hps_ctxt.tsk_struct_ptr);
	return HRTIMER_NORESTART;
}

static long int hps_get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

/*
 * hps task main loop
 */
static int _hps_task_main(void *data)
{
	int cnt = 0;
	void (*algo_func_ptr)(void);

	hps_ctxt_print_basic(1);

	if (hps_ctxt.is_hmp)
		algo_func_ptr = hps_algo_hmp;
	else
		algo_func_ptr = hps_algo_smp;

	while (1) {
		/* TODO: showld we do dvfs?
		 * struct cpufreq_policy *policy;
		 * policy = cpufreq_cpu_get(0);
		 * dbs_freq_increase(policy, policy->max);
		 * cpufreq_cpu_put(policy);
		 */

		(*algo_func_ptr) ();

		if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_WAIT_QUEUE) {
			wait_event_timeout(hps_ctxt.wait_queue,
					   atomic_read(&hps_ctxt.is_ondemand) != 0,
					   msecs_to_jiffies(HPS_TIMER_INTERVAL_MS));
		} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER) {
			if (atomic_read(&hps_ctxt.is_ondemand) == 0) {
				mod_timer(&hps_ctxt.tmr_list,
					  (jiffies + msecs_to_jiffies(HPS_TIMER_INTERVAL_MS)));
				set_current_state(TASK_INTERRUPTIBLE);
				schedule();
			}
		} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER) {
			hrtimer_cancel(&hps_ctxt.hr_timer);
			hrtimer_start(&hps_ctxt.hr_timer, ktime, HRTIMER_MODE_REL);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}

		if (kthread_should_stop())
			break;
	}

	hps_warn("leave _hps_task_main, cnt:%08d\n", cnt++);
	return 0;
}

/*
 * hps task control interface
 */
int hps_task_start(void)
{
	struct sched_param param = {.sched_priority = HPS_TASK_PRIORITY };

	if (hps_ctxt.tsk_struct_ptr == NULL) {
		hps_ctxt.tsk_struct_ptr = kthread_create(_hps_task_main, NULL, "hps_main");
		if (IS_ERR(hps_ctxt.tsk_struct_ptr))
			return PTR_ERR(hps_ctxt.tsk_struct_ptr);

		sched_setscheduler_nocheck(hps_ctxt.tsk_struct_ptr, SCHED_FIFO, &param);
		get_task_struct(hps_ctxt.tsk_struct_ptr);
		wake_up_process(hps_ctxt.tsk_struct_ptr);
		hps_warn("hps_task_start success, ptr: %p, pid: %d\n",
			 hps_ctxt.tsk_struct_ptr, hps_ctxt.tsk_struct_ptr->pid);
	} else {
		hps_warn("hps task already exist, ptr: %p, pid: %d\n",
			 hps_ctxt.tsk_struct_ptr, hps_ctxt.tsk_struct_ptr->pid);
	}

	return 0;
}

void hps_task_stop(void)
{
	if (hps_ctxt.tsk_struct_ptr) {
		kthread_stop(hps_ctxt.tsk_struct_ptr);
		put_task_struct(hps_ctxt.tsk_struct_ptr);
		hps_ctxt.tsk_struct_ptr = NULL;
	}
}

void hps_task_wakeup_nolock(void)
{
	if (hps_ctxt.tsk_struct_ptr) {
		atomic_set(&hps_ctxt.is_ondemand, 1);
		if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_WAIT_QUEUE)
			wake_up(&hps_ctxt.wait_queue);
		else if ((hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER)
			 || (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER))
			wake_up_process(hps_ctxt.tsk_struct_ptr);
	}
}

void hps_task_wakeup(void)
{
	mutex_lock(&hps_ctxt.lock);

	hps_task_wakeup_nolock();

	mutex_unlock(&hps_ctxt.lock);
}

/*
 * init
 */
int hps_core_init(void)
{
	int r = 0;

	hps_warn("hps_core_init\n");
	if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER) {
		/*init timer */
		init_timer(&hps_ctxt.tmr_list);
		/*init_timer_deferrable(&hps_ctxt.tmr_list); */
		hps_ctxt.tmr_list.function = (void *)&_hps_timer_callback;
		hps_ctxt.tmr_list.data = (unsigned long)&hps_ctxt;
		hps_ctxt.tmr_list.expires = jiffies + msecs_to_jiffies(HPS_TIMER_INTERVAL_MS);
		add_timer(&hps_ctxt.tmr_list);
	} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER) {
		ktime = ktime_set(0, MS_TO_NS(HPS_TIMER_INTERVAL_MS));
		/*init Hrtimer */
		hrtimer_init(&hps_ctxt.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		hps_ctxt.hr_timer.function = (void *)&_hps_timer_callback;
		hrtimer_start(&hps_ctxt.hr_timer, ktime, HRTIMER_MODE_REL);

	}
	/* init and start task */
	r = hps_task_start();
	if (r)
		hps_error("hps_task_start fail(%d)\n", r);

	return r;
}

/*
 * deinit
 */
int hps_core_deinit(void)
{
	int r = 0;

	hps_warn("hps_core_deinit\n");
	if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER) {
		/*deinit timer */
		del_timer_sync(&hps_ctxt.tmr_list);
	} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER) {
		/*deinit timer */
		r = hrtimer_cancel(&hps_ctxt.hr_timer);
		if (r)
			hps_error("hps hr timer delete error!\n");
	}

	hps_task_stop();

	return r;
}

int hps_del_timer(void)
{
#if 1
	if (!hps_cancel_time)
		hps_cancel_time = hps_get_current_time_ms();
	if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER) {
		/*deinit timer */
		del_timer_sync(&hps_ctxt.tmr_list);
	} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER) {
		hrtimer_cancel(&hps_ctxt.hr_timer);
	}
#endif
	return 0;
}

int hps_restart_timer(void)
{
#if 1
	unsigned long long time_differ = 0;

	time_differ = hps_get_current_time_ms() - hps_cancel_time;
	if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_TIMER) {
		/*init timer */
		init_timer(&hps_ctxt.tmr_list);
		/*init_timer_deferrable(&hps_ctxt.tmr_list); */
		hps_ctxt.tmr_list.function = (void *)&_hps_timer_callback;
		hps_ctxt.tmr_list.data = (unsigned long)&hps_ctxt;

		if (time_differ >= HPS_TIMER_INTERVAL_MS) {
			hps_ctxt.tmr_list.expires =
			    jiffies + msecs_to_jiffies(HPS_TIMER_INTERVAL_MS);
			add_timer(&hps_ctxt.tmr_list);
			hps_task_wakeup_nolock();
			hps_cancel_time = 0;
		} else {
			hps_ctxt.tmr_list.expires =
			    jiffies + msecs_to_jiffies(HPS_TIMER_INTERVAL_MS - time_differ);
			add_timer(&hps_ctxt.tmr_list);
		}
	} else if (hps_ctxt.periodical_by == HPS_PERIODICAL_BY_HR_TIMER) {
#if 1
		hrtimer_start(&hps_ctxt.hr_timer, ktime, HRTIMER_MODE_REL);
		if (time_differ >= HPS_TIMER_INTERVAL_MS) {
			hps_task_wakeup_nolock();
			hps_cancel_time = 0;
		}
#else
		if (time_differ >= HPS_TIMER_INTERVAL_MS) {
			/*init Hrtimer */
			hrtimer_start(&hps_ctxt.hr_timer, ktime, HRTIMER_MODE_REL);
			hps_task_wakeup_nolock();
			hps_cancel_time = 0;
		}
#endif
	}
#endif
	return 0;
}

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

#include <linux/sched.h>
#include <mt-plat/met_drv.h>

#include "mdla.h"
#include "mdla_pmu.h"
#include "mdla_trace.h"
#include "met_mdlasys_events.h"
#include "mdla_proc.h"
#include <linux/kallsyms.h>
//#include <linux/trace_events.h>
#include <linux/preempt.h>

#define DEFAULT_POLLING_PERIOD_NS (1000000)  // 1ms
#define TRACE_LEN 128

static struct hrtimer hr_timer;

static noinline int tracing_mark_write(const char *buf)
{
	TRACE_PUTS(buf);
	return 0;
}

void mdla_trace_begin(const int cmd_num)
{
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf), "S|%d|MDLA|mdla_cmd_num: %d",
		task_pid_nr(current), cmd_num);

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
}

void mdla_trace_end(const int cmd_id)
{
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf), "F|%d|MDLA|mdla_cmd_id: %d",
		task_pid_nr(current), cmd_id);

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
}

/* MET: define to enable MET */
#if defined(MDLA_MET_READY)
#define CREATE_TRACE_POINTS
#include "met_mdlasys_events.h"
#endif

/*
 * MDLA PMU counter reader
 */
static void mdla_profile_pmu_counter(int core)
{
	trace_mdla_polling(core,
		mdla_max_cmd_id(),
		pmu_get_perf_counter(0),
		pmu_get_perf_cycle());
}

static void mdla_profile_register_read(void)
{
	int i = 0;

	for (i = 0 ; i < MTK_MDLA_CORE; i++)
		mdla_profile_pmu_counter(i);
}

/*
 * MDLA Polling Function
 */
static enum hrtimer_restart mdla_profile_polling(struct hrtimer *timer)
{
	/*call functions need to be called periodically*/
	mdla_profile_register_read();
	hrtimer_forward_now(&hr_timer, ns_to_ktime(DEFAULT_POLLING_PERIOD_NS));
	return HRTIMER_RESTART;
}

static int mdla_profile_timer_start(void)
{
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = mdla_profile_polling;
	hrtimer_start(&hr_timer, ns_to_ktime(DEFAULT_POLLING_PERIOD_NS),
		HRTIMER_MODE_REL);
	mdla_debug("%s: hrtimer_start()\n", __func__);

	return 0;
}

static int mdla_profile_timer_stop(int wait)
{
	int ret = 0;

	if (wait) {
		hrtimer_cancel(&hr_timer);
		mdla_debug("%s: hrtimer_cancel()\n", __func__);
	} else {
		ret = hrtimer_try_to_cancel(&hr_timer);
		mdla_debug("%s: hrtimer_try_to_cancel(): %d\n", __func__, ret);
	}
	return ret;
}

int mdla_profile_start(void)
{
	mdla_profile_timer_start();
	return 0;
}

int mdla_profile_stop(int wait)
{
	mdla_profile_timer_stop(wait);
	return 0;
}

void mdla_trace_tag_begin(const char *format, ...)
{
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf),
		"B|%d|%s", format, task_pid_nr(current));

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
}

void mdla_trace_tag_end(void)
{
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf), "E\n");


	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
}




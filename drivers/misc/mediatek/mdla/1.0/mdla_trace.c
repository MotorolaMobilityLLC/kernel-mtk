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
#include <linux/string.h>
#include <linux/types.h>
#include <mt-plat/met_drv.h>

#include "mdla.h"
#include "mdla_hw_reg.h"
#include "mdla_pmu.h"
#include "mdla_trace.h"
#include "mdla_decoder.h"
#include "met_mdlasys_events.h"
#include "mdla_proc.h"

#include <linux/kallsyms.h>
//#include <linux/trace_events.h>
#include <linux/preempt.h>

#define TRACE_LEN 128
#define PERIOD_DEFAULT 1000 // in micro-seconds (us)

static struct hrtimer hr_timer;
static u64 cfg_period;
static u32 cfg_op_trace;
static u32 cfg_cmd_trace;
static u32 cfg_pmu_int;
static u32 cfg_pmu_event[MDLA_PMU_COUNTERS];

static noinline int tracing_mark_write(const char *buf)
{
	TRACE_PUTS(buf);
	return 0;
}

void mdla_trace_begin(const int cmd_num, void *cmd)
{
	char buf[TRACE_LEN];
	char *p = (char *)cmd;
	int i;
	int len;

	if (!cfg_cmd_trace)
		return;

#define _VAL32(offset) (*((const u32 *)(cmd+offset)))

	len = snprintf(buf, sizeof(buf),
		"I|%d|MDLA|B|mdla_cmd_id:%d,mdla_cmd_num:%d",
		task_pid_nr(current),
		_VAL32(MREG_CMD_SWCMD_ID),
		cmd_num);

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);

	if (cfg_op_trace) {
		for (i = 0; i < cmd_num; i++, p += MREG_CMD_SIZE) {
			len = snprintf(buf, sizeof(buf), "I|%d|MDLA|C|",
				task_pid_nr(current));
			mdla_decode(p, buf+len, TRACE_LEN-len);
			tracing_mark_write(buf);
		}
	}
	pmu_reset_cycle();
}

void mdla_trace_end(u32 cmd_id, u64 end, int mode)
{
	char buf[TRACE_LEN];
	int len;

	if ((!cfg_cmd_trace) || (!end))
		return;

	if (cfg_pmu_int != mode)
		return;

	len = snprintf(buf, sizeof(buf),
		"I|%d|MDLA|E|mdla_cmd_id:%d,cycle:%u,sched_clock_result:%llu",
		task_pid_nr(current),
		cmd_id,
		pmu_get_perf_cycle(),
		end);

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
}

/* MET: define to enable MET */
#if defined(MDLA_MET_READY)
#define CREATE_TRACE_POINTS
#include "met_mdlasys_events.h"
#endif

void mdla_dump_prof(struct seq_file *s)
{
	int i;

#define _SHOW_VAL(t) \
	mdla_print_seq(s, "%s=%lu\n", #t, (unsigned long)cfg_##t)

	_SHOW_VAL(period);
	_SHOW_VAL(cmd_trace);
	_SHOW_VAL(op_trace);
	_SHOW_VAL(pmu_int);

	mdla_print_seq(s, "PMU_INT=%d\n",
		(mdla_reg_read(MREG_TOP_G_INTP2) & MDLA_IRQ_PMU_INTE) ? 1 : 0);
	mdla_print_seq(s, "PMU_CFG_PMCR=0x%x\n", pmu_reg_read(PMU_CFG_PMCR));
	for (i = 0; i < MDLA_PMU_COUNTERS; i++)
		mdla_print_seq(s, "c%d=0x%x\n", (i+1),
			pmu_reg_read(PMU_EVENT_OFFSET +	(i * PMU_CNT_SHIFT)));
}

/*
 * MDLA PMU counter reader
 */
static void mdla_profile_pmu_counter(int core)
{
	u32 c[MDLA_PMU_COUNTERS];

	pmu_get_perf_counters(c);
	mdla_debug("_id=c%d, c1=%u, c2=%u, c3=%u, c4=%u, c5=%u, c6=%u, c7=%u, c8=%u, c9=%u, c10=%u, c11=%u, c12=%u, c13=%u, c14=%u, c15=%u\n",
		core,
		c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
		c[8], c[9], c[10], c[11], c[12], c[13], c[14]);
	trace_mdla_polling(core, c);
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

	if (!cfg_period)
		return HRTIMER_NORESTART;

	hrtimer_forward_now(&hr_timer, ns_to_ktime(cfg_period * 1000));
	return HRTIMER_RESTART;
}

static int mdla_profile_timer_start(void)
{
	if (!cfg_period)
		return 0;

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = mdla_profile_polling;
	hrtimer_start(&hr_timer, ns_to_ktime(cfg_period * 1000),
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

static int mdla_profile_set_handle(const char *tag, u32 val)
{
	int counter;

	mdla_debug("%s: %s=0x%x\n", __func__, tag, val);

	if (sscanf(tag, "c%d", &counter) == 1) {
		if (counter < 1 || counter > MDLA_PMU_COUNTERS)
			return -EINVAL;
		counter--;
		cfg_pmu_event[counter] = val;
		return pmu_event_set(counter, val);
	}

#define _SET_VAL(t) \
	{ if (!strcmp(#t, tag)) { \
		cfg_##t = val; \
		return 0; } }

	_SET_VAL(cmd_trace);
	_SET_VAL(op_trace);
	_SET_VAL(period);

	if (!strcmp("PMU_INT", tag)) {
		if (val) {
			cfg_pmu_int = MDLA_TRACE_MODE_INT;
			mdla_debug("%s: PMU_INT enable.\n", __func__);
			mdla_reg_set(MDLA_IRQ_PMU_INTE, MREG_TOP_G_INTP2);
		} else {
			cfg_pmu_int = MDLA_TRACE_MODE_CMD;
			mdla_debug("%s: PMU_INT disable.\n", __func__);
			mdla_reg_clear(MDLA_IRQ_PMU_INTE, MREG_TOP_G_INTP2);
		}
		return 0;
	}

	mdla_debug("%s: invalid argument.\n", __func__);

	return -EINVAL;
}

static int mdla_profile_set(const char *str)
{
	char *p;
	char *token;
	char *tag_pos = NULL;
	char buf[64];
	u32 val = 0;
	int ret = -EINVAL;

	strncpy(buf, str, 64);
	p = &buf[0];

	for (token = strsep(&p, "=");
		token != NULL; token = strsep(&p, "=")) {

		if (!tag_pos) {
			tag_pos = token;
		} else {
			ret = kstrtouint(token, 0, &val);
			break;
		}
	}

	mdla_debug("%s: ret = %d, %s=%d\n", __func__, ret, tag_pos, val);

	if (!ret)
		mdla_profile_set_handle(tag_pos, val);

	return ret;
}

int mdla_profile(const char *str)
{
	int ret = 0;

	if (!strcmp(str, "start"))
		ret = mdla_profile_start();
	else if (!strcmp(str, "stop"))
		ret = mdla_profile_stop(1);
	else if (!strcmp(str, "try_stop"))
		ret = mdla_profile_stop(0);
	else if (!strcmp(str, "reset_counter"))
		pmu_reset_counter();
	else
		ret = mdla_profile_set(str);

	return ret;
}

/* restore trace settings after reset */
int mdla_profile_reset(void)
{
	int i;

	if (cfg_pmu_int)
		mdla_reg_set(MDLA_IRQ_PMU_INTE, MREG_TOP_G_INTP2);
	else
		mdla_reg_clear(MDLA_IRQ_PMU_INTE, MREG_TOP_G_INTP2);

	for (i = 0; i < MDLA_PMU_COUNTERS; i++)
		pmu_event_set(i, cfg_pmu_event[i]);

	return 0;
}

int mdla_profile_init(void)
{
	cfg_period = PERIOD_DEFAULT;
	cfg_op_trace = 0;
	cfg_cmd_trace = 0;
	cfg_pmu_int = MDLA_TRACE_MODE_CMD;
	memset(cfg_pmu_event, 0xFF, sizeof(cfg_pmu_event));

	return 0;
}

int mdla_profile_exit(void)
{
	cfg_period = 0;
	mdla_profile_stop(1);

	return 0;
}

void mdla_trace_tag_begin(const char *format, ...)
{
#if 0
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf),
		"B|%d|%s", format, task_pid_nr(current));

	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
#endif
}

void mdla_trace_tag_end(void)
{
#if 0
	char buf[TRACE_LEN];

	int len = snprintf(buf, sizeof(buf), "E\n");


	if (len >= TRACE_LEN)
		len = TRACE_LEN - 1;

	tracing_mark_write(buf);
#endif
}




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
#include <linux/kernel.h>
#include <mach/mtk_gpt.h>
#include "mtk_cpuidle.h"
#include "mtk_idle_internal.h"
#include "mtk_idle_profile.h"
#include "mtk_spm_resource_req_internal.h"

#if SPM_MET_TAGGING
#include <core/met_drv.h>
#endif

#define IDLE_PROF_TAG                   "Power/swap "
#define idle_prof_emerg(fmt, args...)   pr_emerg(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_alert(fmt, args...)   pr_alert(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_crit(fmt, args...)    pr_crit(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_err(fmt, args...)     pr_err(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_warn(fmt, args...)    pr_warn(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_notice(fmt, args...)  pr_notice(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_info(fmt, args...)    pr_debug(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_ver(fmt, args...)     pr_debug(IDLE_PROF_TAG fmt, ##args)
#define idle_prof_dbg(fmt, args...)     pr_debug(IDLE_PROF_TAG fmt, ##args)

/* idle ratio */
static bool idle_ratio_en;
static unsigned long long idle_ratio_profile_start_time;
static unsigned long long idle_ratio_profile_duration;

/* idle block information */
static unsigned long long idle_block_log_prev_time;
static unsigned int idle_block_log_time_criteria = 5000;    /* 5 sec */
static unsigned long long idle_cnt_dump_prev_time;
static unsigned int idle_cnt_dump_criteria = 5000;          /* 5 sec */

static struct mtk_idle_buf idle_log;

#define reset_log()              reset_idle_buf(idle_log)
#define get_log()                get_idle_buf(idle_log)
#define append_log(fmt, args...) idle_buf_append(idle_log, fmt, ##args)

struct mtk_idle_block {
	u64 prev_time;
	u32 time_critera;
	unsigned long last_cnt[NR_CPUS];
	unsigned long *cnt;
	unsigned long *block_cnt;
	unsigned int *block_mask;
	char *name;
	bool init;
};

#define IDLE_RATIO_WINDOW_MS 1000        /* 1000 ms */
/* xxidle recent idle ratio */
static struct mtk_idle_recent_ratio recent_ratio = {
	.value = 0,
	.value_dp = 0,
	.value_so3 = 0,
	.value_so = 0,
	.last_end_ts = 0,
	.start_ts = 0,
	.end_ts = 0,
};

struct mtk_idle_ratio {
	char *name;
	unsigned long long start;
	unsigned long long value;
};

struct mtk_idle_prof {
	struct mtk_idle_ratio ratio;
	struct mtk_idle_block block;
};

#define DEFINE_ATTR(abbr_name, block_name, block_ms)   \
	{                                                  \
		.ratio = {                                     \
			.name = (abbr_name),                       \
		},                                             \
		.block = {                                     \
			.name = (block_name),                      \
			.time_critera = (block_ms),                \
			.init = false,                             \
		}                                              \
	}                                                  \

static struct mtk_idle_prof idle_prof[NR_TYPES] = {
	[IDLE_TYPE_DP]  = DEFINE_ATTR("DP", "dpidle", 30000),
	[IDLE_TYPE_SO3] = DEFINE_ATTR("SODI3", "soidle3", 30000),
	[IDLE_TYPE_SO]  = DEFINE_ATTR("SODI", "soidle", 30000),
	[IDLE_TYPE_MC]  = DEFINE_ATTR("MC", "mcidle", ~0),
	[IDLE_TYPE_SL]  = DEFINE_ATTR("SL", "slidle", ~0),
	[IDLE_TYPE_RG]  = DEFINE_ATTR("RG", "rgidle", ~0)
};

/* SPM TAWM */
#define TRIGGER_TYPE                (2) /* b'10: high */
#define TWAM_PERIOD_MS              (1000)
#define WINDOW_LEN_SPEED            (TWAM_PERIOD_MS * 0x65B8)
#define WINDOW_LEN_NORMAL           (TWAM_PERIOD_MS * 0xD)
#define GET_EVENT_RATIO_SPEED(x)    ((x)/(WINDOW_LEN_SPEED/1000))
#define GET_EVENT_RATIO_NORMAL(x)   ((x)/(WINDOW_LEN_NORMAL/1000))

idle_twam_t idle_twam;

#if SPM_MET_TAGGING
#define idle_get_current_time_us(x) do {\
		struct timeval t;\
		do_gettimeofday(&t);\
		(x) = ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);\
	} while (0)

static unsigned long long idle_met_timestamp[NR_TYPES];
static unsigned long long idle_met_prev_tag[NR_TYPES];
static const char *idle_met_label[NR_TYPES] = {
	[IDLE_TYPE_DP] = "deep idle residency",
	[IDLE_TYPE_SO3] = "SODI3 residency",
	[IDLE_TYPE_SO] = "SODI residency",
};
#endif

p_idle_twam_t mtk_idle_get_twam(void)
{
	return &idle_twam;
}

void mtk_idle_twam_callback(struct twam_sig *ts)
{
	idle_prof_warn("spm twam (sel%d: %d) ratio: %5u/1000\n",
			idle_twam.sel, idle_twam.event,
			(idle_twam.speed_mode)?GET_EVENT_RATIO_SPEED(ts->sig0):GET_EVENT_RATIO_NORMAL(ts->sig0));
}

void mtk_idle_twam_disable(void)
{
	if (idle_twam.running == false)
		return;
	spm_twam_register_handler(NULL);
	spm_twam_disable_monitor();
	idle_twam.running = false;
}

void mtk_idle_twam_enable(u32 event)
{
	struct twam_sig montype = {0};
	struct twam_sig twamsig = {0};

	if (idle_twam.event != event)
		mtk_idle_twam_disable();

	if (idle_twam.running == true)
		return;

	idle_twam.event = (event < 32)?event:29;
	twamsig.sig0 = idle_twam.event;
	montype.sig0 = TRIGGER_TYPE;

	spm_twam_set_mon_type(&montype);
	spm_twam_set_window_length((idle_twam.speed_mode)?WINDOW_LEN_SPEED:WINDOW_LEN_NORMAL);
	spm_twam_register_handler(mtk_idle_twam_callback);
	spm_twam_set_idle_select(idle_twam.sel);
	spm_twam_enable_monitor(&twamsig, idle_twam.speed_mode);
	idle_twam.running = true;
}

static DEFINE_SPINLOCK(recent_idle_ratio_spin_lock);

void mtk_idle_ratio_calc_start(int type, int cpu)
{
	unsigned long flags;

	if (idle_ratio_en && type >= 0 && type < NR_TYPES && (cpu == 0 || cpu == 4))
		idle_prof[type].ratio.start = idle_get_current_time_ms();

	if (type < IDLE_TYPE_MC) {
		spin_lock_irqsave(&recent_idle_ratio_spin_lock, flags);

		recent_ratio.start_ts = idle_get_current_time_ms();

		spin_unlock_irqrestore(&recent_idle_ratio_spin_lock, flags);
	}

#if SPM_MET_TAGGING
	if (type < IDLE_TYPE_MC)
		idle_get_current_time_us(idle_met_timestamp[type]);
#endif
}

void mtk_idle_ratio_calc_stop(int type, int cpu)
{
	if (idle_ratio_en && type >= 0 && type < NR_TYPES && (cpu == 0 || cpu == 4))
		idle_prof[type].ratio.value += idle_get_current_time_ms() - idle_prof[type].ratio.start;

	if (type < IDLE_TYPE_MC) {
		struct mtk_idle_recent_ratio *ratio = NULL;
		unsigned long flags;
		unsigned long long interval = 0;
		unsigned long long last_idle_time = 0;
		unsigned long long last_ratio = 0;
		unsigned long long last_ratio_dp = 0;
		unsigned long long last_ratio_so3 = 0;
		unsigned long long last_ratio_so = 0;

		spin_lock_irqsave(&recent_idle_ratio_spin_lock, flags);

		ratio = &recent_ratio;

		ratio->end_ts = idle_get_current_time_ms();
		interval = ratio->end_ts - ratio->last_end_ts;
		last_idle_time = ratio->end_ts - ratio->start_ts;
		last_ratio = ratio->value;
		last_ratio_dp = ratio->value_dp;
		last_ratio_so3 = ratio->value_so3;
		last_ratio_so = ratio->value_so;

		if (interval >= IDLE_RATIO_WINDOW_MS) {
			ratio->value = (last_idle_time >= IDLE_RATIO_WINDOW_MS) ?
							IDLE_RATIO_WINDOW_MS : last_idle_time;
			ratio->value_dp = (type == IDLE_TYPE_DP) ? ratio->value : 0;
			ratio->value_so3 = (type == IDLE_TYPE_SO3) ? ratio->value : 0;
			ratio->value_so = (type == IDLE_TYPE_SO) ? ratio->value : 0;
		} else {
			ratio->value = ((IDLE_RATIO_WINDOW_MS - interval) *
							last_ratio / IDLE_RATIO_WINDOW_MS)
							+ last_idle_time;
			ratio->value_dp = ((IDLE_RATIO_WINDOW_MS - interval) *
							last_ratio_dp / IDLE_RATIO_WINDOW_MS)
							+ ((type == IDLE_TYPE_DP) ? last_idle_time : 0);
			ratio->value_so3 = ((IDLE_RATIO_WINDOW_MS - interval) *
							last_ratio_so3 / IDLE_RATIO_WINDOW_MS)
							+ ((type == IDLE_TYPE_SO3) ? last_idle_time : 0);
			ratio->value_so = ((IDLE_RATIO_WINDOW_MS - interval) *
							last_ratio_so / IDLE_RATIO_WINDOW_MS)
							+ ((type == IDLE_TYPE_SO) ? last_idle_time : 0);
		}
#if 0
		idle_prof_err("XXIDLE %llu, %llu, %llu, %llu, %llu, %llu, %llu, %llu, %d\n",
			ratio->last_end_ts, ratio->start_ts, ratio->end_ts,
			ratio->value, ratio->value_dp, ratio->value_so3, ratio->value_so, last_ratio, type);
#endif
		ratio->last_end_ts = ratio->end_ts;

		spin_unlock_irqrestore(&recent_idle_ratio_spin_lock, flags);
	}

#if SPM_MET_TAGGING
	if (type < IDLE_TYPE_MC) {
		unsigned long long idle_met_curr;

		idle_get_current_time_us(idle_met_curr);

		met_tag_oneshot(0, idle_met_label[type],
			((idle_met_curr - idle_met_timestamp[type])*100)/(idle_met_curr - idle_met_prev_tag[type]));
		idle_met_prev_tag[type] = idle_met_curr;
	}
#endif
}

bool mtk_idle_get_ratio_status(void)
{
	return idle_ratio_en;
}

void mtk_idle_disable_ratio_calc(void)
{
	idle_ratio_en = false;
}

void mtk_idle_enable_ratio_calc(void)
{
	int idx;

	if (idle_ratio_en == false) {
		for (idx = 0; idx < NR_TYPES; idx++)
			idle_prof[idx].ratio.value = 0;
		idle_ratio_profile_start_time = idle_get_current_time_ms();
		idle_ratio_en = true;
	}
}

static void mtk_idle_dump_cnt(int type)
{
	static struct mtk_idle_buf buf;
	struct mtk_idle_block *p_idle;
	bool enter_idle = false;
	unsigned long idle_cnt;
	int i;

	p_idle = &idle_prof[type].block;

	if (unlikely(p_idle->init == false)) {
		append_log("Not initialized --- ");
		return;
	}

	reset_idle_buf(buf);

	append_log("%s: ", idle_prof[type].ratio.name);

	for (i = 0; i < nr_cpu_ids; i++) {
		idle_cnt = p_idle->cnt[i] - p_idle->last_cnt[i];
		if (idle_cnt != 0) {
			idle_buf_append(buf, "[%d] = %lu, ", i, idle_cnt);
			enter_idle = true;
		}
		p_idle->last_cnt[i] = p_idle->cnt[i];
	}

	if (enter_idle)
		append_log("%s --- ", get_idle_buf(buf));
	else
		append_log("No enter --- ");
}

static DEFINE_SPINLOCK(idle_dump_cnt_spin_lock);

void mtk_idle_dump_cnt_in_interval(void)
{
	int i = 0;
	unsigned long long idle_cnt_dump_curr_time = 0;
	unsigned long flags;
	bool dump_log = false;

	idle_cnt_dump_curr_time = idle_get_current_time_ms();

	if (idle_cnt_dump_prev_time == 0)
		idle_cnt_dump_prev_time = idle_cnt_dump_curr_time;

	spin_lock_irqsave(&idle_dump_cnt_spin_lock, flags);

	if (((idle_cnt_dump_curr_time - idle_cnt_dump_prev_time) > idle_cnt_dump_criteria)) {
		dump_log = true;
		idle_cnt_dump_prev_time = idle_cnt_dump_curr_time;
	}

	spin_unlock_irqrestore(&idle_dump_cnt_spin_lock, flags);

	if (!dump_log)
		return;

	/* dump idle count */
	reset_log();

	mtk_idle_dump_cnt(IDLE_TYPE_DP);
	mtk_idle_dump_cnt(IDLE_TYPE_SO3);
	mtk_idle_dump_cnt(IDLE_TYPE_SO);

	/* dump log */
	idle_prof_warn("%s\n", get_log());

	/* dump idle ratio */
	if (idle_ratio_en) {
		idle_ratio_profile_duration = idle_get_current_time_ms() - idle_ratio_profile_start_time;
		reset_log();
		append_log("--- CPU idle: %llu, ", idle_ratio_profile_duration);
		for (i = 0; i < NR_TYPES; i++) {
			if (idle_prof[i].ratio.start == 0)
				continue;
			append_log("%s = %llu, ", idle_prof[i].ratio.name, idle_prof[i].ratio.value);
			idle_prof[i].ratio.value = 0;
		}
		append_log("--- (ms)\n");
		idle_prof_warn("%s\n", get_log());
		idle_ratio_profile_start_time = idle_get_current_time_ms();
	}

	/* if (spm_get_resource_usage())  */
	/*    spm_resource_req_dump();    */
}

static DEFINE_SPINLOCK(idle_blocking_spin_lock);

bool mtk_idle_select_state(int type, int reason)
{
	struct mtk_idle_block *p_idle;
	u64 curr_time;
	int i;
	unsigned long flags;
	bool dump_block_info;

	if (unlikely(type < 0 || type >= NR_TYPES))
		return false;

	p_idle = &idle_prof[type].block;

	if (unlikely(p_idle->init == false))
		return reason == NR_REASONS;

	curr_time = idle_get_current_time_ms();

	if (reason >= NR_REASONS) {
		p_idle->prev_time = curr_time;
		return true;
	}

	if (p_idle->prev_time == 0)
		p_idle->prev_time = curr_time;

	spin_lock_irqsave(&idle_blocking_spin_lock, flags);

	dump_block_info	= ((curr_time - p_idle->prev_time) > p_idle->time_critera)
			    && ((curr_time - idle_block_log_prev_time) > idle_block_log_time_criteria);

	if (dump_block_info) {
		p_idle->prev_time = curr_time;
		idle_block_log_prev_time = curr_time;
	}

	spin_unlock_irqrestore(&idle_blocking_spin_lock, flags);

	if (dump_block_info) {
		/* xxidle, rgidle count */
		reset_log();

		append_log("CNT(%s,rgidle): ", p_idle->name);
		for (i = 0; i < nr_cpu_ids; i++)
			append_log("[%d] = (%lu,%lu), ", i, p_idle->cnt[i], idle_prof[IDLE_TYPE_RG].block.cnt[i]);
		idle_prof_warn("%s\n", get_log());

		/* block category */
		reset_log();

		append_log("%s_block_cnt: ", p_idle->name);
		for (i = 0; i < NR_REASONS; i++)
			append_log("[%s] = %lu, ", mtk_get_reason_name(i), p_idle->block_cnt[i]);
		idle_prof_warn("%s\n", get_log());

		reset_log();

		append_log("%s_block_mask: ", p_idle->name);
		for (i = 0; i < NR_GRPS; i++)
			append_log("0x%08x, ", p_idle->block_mask[i]);
		idle_prof_warn("%s\n", get_log());

		memset(p_idle->block_cnt, 0, NR_REASONS * sizeof(p_idle->block_cnt[0]));

		spm_resource_req_block_dump();
	}
	p_idle->block_cnt[reason]++;
	return false;
}

void mtk_idle_block_setting(int type, unsigned long *cnt, unsigned long *block_cnt, unsigned int *block_mask)
{
	struct mtk_idle_block *p_idle;

	if (unlikely(type < 0 || type >= NR_TYPES))
		return;

	p_idle = &idle_prof[type].block;

	p_idle->cnt        = cnt;
	p_idle->block_cnt  = block_cnt;
	p_idle->block_mask = block_mask;

	if (cnt && block_cnt && block_mask)
		p_idle->init = true;
	else
		idle_prof_err("IDLE BLOCKING INFO SETTING FAIL (type:%d)\n", type);
}

void mtk_idle_recent_ratio_get(int *window_length_ms, struct mtk_idle_recent_ratio *ratio)
{
	unsigned long flags;

	spin_lock_irqsave(&recent_idle_ratio_spin_lock, flags);

	if (window_length_ms != NULL)
		*window_length_ms = IDLE_RATIO_WINDOW_MS;

	if (ratio != NULL) {
		ratio->value       = recent_ratio.value;
		ratio->value_dp    = recent_ratio.value_dp;
		ratio->value_so3   = recent_ratio.value_so3;
		ratio->value_so    = recent_ratio.value_so;
		ratio->last_end_ts = recent_ratio.last_end_ts;
	}

	spin_unlock_irqrestore(&recent_idle_ratio_spin_lock, flags);
}

void mtk_idle_twam_init(void)
{
#if SPM_MET_TAGGING
	met_tag_init();
#endif
	idle_twam.running = false;
	idle_twam.speed_mode = true;
	idle_twam.event = 29;
}


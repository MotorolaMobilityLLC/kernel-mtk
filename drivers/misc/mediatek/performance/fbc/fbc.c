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
#include "fbc.h"

static void notify_twanted_timeout_legacy(void);
static void notify_twanted_timeout_eas(void);
static void notify_touch_up_timeout(void);
static void notify_render_aware_timeout(void);
static DECLARE_WORK(mt_tt_legacy_work, (void *) notify_twanted_timeout_legacy);
static DECLARE_WORK(mt_tt_eas_work, (void *) notify_twanted_timeout_eas);
static DECLARE_WORK(mt_touch_timeout_work, (void *) notify_touch_up_timeout);
static DECLARE_WORK(mt_render_aware_timeout_work, (void *) notify_render_aware_timeout);
static DEFINE_SPINLOCK(tlock);
static DEFINE_SPINLOCK(tlock1);
static DEFINE_SPINLOCK(tlock2);
static struct workqueue_struct *wq;
static struct mutex notify_lock;
static struct hrtimer hrt, hrt1, hrt2;
static struct ppm_limit_data freq_limit[NR_PPM_CLUSTERS], core_limit[NR_PPM_CLUSTERS];
static unsigned long __read_mostly mark_addr;

static int fbc_debug, fbc_ux_state, fbc_ux_state_pre, fbc_render_aware, fbc_render_aware_pre;
static int fbc_game, fbc_trace;
static long long frame_budget, twanted, twanted_ms, avg_frame_time;
static int ema, boost_method, super_boost, boost_flag, big_enable;
static int boost_value, touch_boost_value, current_max_bv, avg_boost, chase_boost1, chase_boost2;
static int first_frame, first_vsync, frame_done, chase, last_frame_done_in_budget, act_switched;
static long long capacity, touch_capacity, current_max_capacity, avg_capacity, chase_capacity;
static long long his_cap[2];
static long long frame_info[MAX_THREAD][2]; /*0:current id, 1: frametime, if no render -1*/
static int his_bv[2];
static int power_ll[16][2], power_l[16][2], power_b[16][2];

struct fbc_operation_locked {
	void (*ux_enable)(int);
	void (*frame_cmplt)(unsigned long);
	void (*intended_vsync)(void);
	void (*no_render)(void);
	void (*game)(int);
	void (*act_switch)(int);
};

static struct fbc_operation_locked *fbc_op;

static inline bool is_ux_fbc_active(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	return !(fbc_debug || !fbc_ux_state);
}

inline void fbc_tracer(int pid, char *name, int count)
{
	if (!fbc_trace || !name)
		return;

	preempt_disable();
	event_trace_printk(mark_addr, "C|%d|%s|%d\n",
			   pid, name, count);
	preempt_enable();
}

void update_pwd_tbl(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		power_ll[i][0] = mt_cpufreq_get_freq_by_idx(0, i);
		power_ll[i][1] = upower_get_power(UPOWER_BANK_LL, i, UPOWER_CPU_STATES);
	}

	for (i = 0; i < 16; i++) {
		power_l[i][0] = mt_cpufreq_get_freq_by_idx(1, i);
		power_l[i][1] = upower_get_power(UPOWER_BANK_L, i, UPOWER_CPU_STATES);
	}

	for (i = 0; i < 16; i++) {
		power_b[i][0] = mt_cpufreq_get_freq_by_idx(2, i);
		power_b[i][1] = upower_get_power(UPOWER_BANK_B, i, UPOWER_CPU_STATES);
	}

#if 0
	for (i = 0; i < 16; i++) {
		pr_crit(TAG"ll freq:%d, cap:%d", power_ll[i][0], power_ll[i][1]);
		pr_crit(TAG"b freq:%d, cap:%d", power_b[i][0], power_b[i][1]);
	}
#endif
}

static void boost_freq_and_eas(int capacity)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	if (capacity > 1024)
		capacity = 1024;
	if (capacity <= 0)
		capacity = 1;

	/*LL freq*/
	for (i = 15; i >= 0; i--) {
		if (capacity <= power_ll[i][1]) {
			freq_limit[0].min = freq_limit[0].max = power_ll[i][0];
			break;
		}
	}

	if (i <= 0)
		freq_limit[0].min = freq_limit[0].max = power_ll[0][0];

	/*L freq*/
	for (i = 15; i >= 0; i--) {
		if (capacity <= power_l[i][1]) {
			freq_limit[1].min = freq_limit[1].max = power_l[i][0];
			break;
		}
	}

	if (i <= 0)
		freq_limit[1].min = freq_limit[1].max = power_l[0][0];

	/*B freq*/
	for (i = 15; i >= 0; i--) {
		if (capacity <= power_b[i][1]) {
			freq_limit[2].min = freq_limit[2].max = power_b[i][0];
			break;
		}
	}
	if (i <= 0)
		freq_limit[2].min = freq_limit[2].max = power_b[0][0];

	update_userlimit_cpu_freq(KIR_FBC, NR_PPM_CLUSTERS, freq_limit);

#if 0
	pr_crit(TAG"[boost_freq] capacity=%d (i,j)=(%d,%d), ll_freq=%d, b_freq=%d\n",
			capacity, i, j, freq_limit[0].max, freq_limit[2].max);
#endif
	fbc_tracer(-2, "fbc_freq_ll", freq_limit[0].max);
	fbc_tracer(-2, "fbc_freq_l", freq_limit[1].max);
	fbc_tracer(-2, "fbc_freq_b", freq_limit[2].max);
	fbc_tracer(-2, "capacity", capacity);
}

void boost_touch_core(int b_enable)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	if (b_enable) {
		core_limit[0].min = 3;
		core_limit[0].max = -1;
		core_limit[1].min = -1;
		core_limit[1].max = -1;
		core_limit[2].min = 1;
		core_limit[2].max = -1;
	} else {
		core_limit[0].min = 3;
		core_limit[0].max = -1;
		core_limit[1].min = -1;
		core_limit[1].max = -1;
		core_limit[2].min = -1;
		core_limit[2].max = -1;
	}
	update_userlimit_cpu_core(KIR_FBC, NR_PPM_CLUSTERS, core_limit);

	fbc_tracer(-4, "b_enable", b_enable);
}

void release_core(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	core_limit[0].min = -1;
	core_limit[0].max = -1;
	core_limit[1].min = -1;
	core_limit[1].max = -1;
	core_limit[2].min = -1;
	core_limit[2].max = -1;
	update_userlimit_cpu_core(KIR_FBC, NR_PPM_CLUSTERS, core_limit);

	fbc_tracer(-4, "b_enable", 0);
}

void release_freq_and_eas(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	freq_limit[0].min = -1;
	freq_limit[0].max = -1;
	freq_limit[1].min = -1;
	freq_limit[1].max = -1;
	freq_limit[2].min = -1;
	freq_limit[2].max = -1;
	update_userlimit_cpu_freq(KIR_FBC, NR_PPM_CLUSTERS, freq_limit);

	fbc_tracer(-2, "fbc_freq_ll", 0);
	fbc_tracer(-2, "fbc_freq_l", 0);
	fbc_tracer(-2, "fbc_freq_b", 0);
	fbc_tracer(-2, "capacity", 0);
}

void boost_touch_core_eas(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	core_limit[0].min = 3;
	core_limit[0].max = -1;
	core_limit[1].min = -1;
	core_limit[1].max = -1;
	core_limit[2].min = -1;
	core_limit[2].max = -1;

	update_userlimit_cpu_core(KIR_FBC, NR_PPM_CLUSTERS, core_limit);
}

void release_eas(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	perfmgr_kick_fg_boost(KIR_FBC, -1);
	fbc_tracer(-3, "boost_value", 0);
}
/*--------------------TIMER------------------------*/
static void enable_touch_up_timer(void)
{
	unsigned long flags;
	ktime_t ktime;

	spin_lock_irqsave(&tlock1, flags);
	ktime = ktime_set(0, NSEC_PER_SEC * TOUCH_TIMEOUT_SEC);
	hrtimer_start(&hrt1, ktime, HRTIMER_MODE_REL);
	spin_unlock_irqrestore(&tlock1, flags);
}

static void disable_touch_up_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tlock1, flags);
	hrtimer_cancel(&hrt1);
	spin_unlock_irqrestore(&tlock1, flags);
}

static enum hrtimer_restart mt_touch_timeout(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &mt_touch_timeout_work);

	return HRTIMER_NORESTART;
}

static void enable_render_aware_timer(void)
{
	unsigned long flags;
	ktime_t ktime;

	spin_lock_irqsave(&tlock2, flags);
	ktime = ktime_set(0, NSEC_PER_MSEC * RENDER_AWARE_TIMEOUT_MSEC);
	hrtimer_start(&hrt2, ktime, HRTIMER_MODE_REL);
	spin_unlock_irqrestore(&tlock2, flags);
}

static void disable_render_aware_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tlock2, flags);
	hrtimer_cancel(&hrt2);
	spin_unlock_irqrestore(&tlock2, flags);

}

static enum hrtimer_restart mt_render_aware_timeout(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &mt_render_aware_timeout_work);

	return HRTIMER_NORESTART;
}

static void enable_frame_twanted_timer(void)
{
	unsigned long flags;
	ktime_t ktime;

	fbc_tracer(-6, "twanted_timer", 1);
	spin_lock_irqsave(&tlock, flags);
	ktime = ktime_set(0, NSEC_PER_MSEC * twanted_ms);
	hrtimer_start(&hrt, ktime, HRTIMER_MODE_REL);
	spin_unlock_irqrestore(&tlock, flags);
}

static void disable_frame_twanted_timer(void)
{
	unsigned long flags;

	fbc_tracer(-6, "twanted_timer", 0);
	spin_lock_irqsave(&tlock, flags);
	hrtimer_cancel(&hrt);
	spin_unlock_irqrestore(&tlock, flags);
}

static enum hrtimer_restart mt_twanted_timeout(struct hrtimer *timer)
{
	if (!wq)
		return HRTIMER_NORESTART;

	switch (boost_method) {
	case EAS:
		queue_work(wq, &mt_tt_eas_work);
		break;
	case LEGACY:
		queue_work(wq, &mt_tt_legacy_work);
		break;
	default:
		queue_work(wq, &mt_tt_legacy_work);
		break;
	}

	return HRTIMER_NORESTART;
}

/*--------------------FRAME HINT OP------------------------*/
static void notify_touch(int action)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	fbc_ux_state_pre = fbc_ux_state;

	/*action 1: touch down 2: touch up*/
	if (action == 1) {
		fbc_ux_state = 1;
		disable_touch_up_timer();
	} else if (action == 0) {
		fbc_ux_state = 2;
		enable_touch_up_timer();
	}

	if (fbc_ux_state == 1)
		fbc_op->ux_enable(1);

	fbc_tracer(-3, "ux_state", fbc_ux_state);
}

static void notify_touch_up_timeout(void)
{
	mutex_lock(&notify_lock);

	fbc_ux_state_pre = fbc_ux_state;
	fbc_ux_state = 0;
	fbc_op->ux_enable(0);

	fbc_tracer(-3, "ux_state", fbc_ux_state);
	mutex_unlock(&notify_lock);

}

static void notify_render_aware_timeout(void)
{
	mutex_lock(&notify_lock);
	if (!is_ux_fbc_active()) {
		mutex_unlock(&notify_lock);
		return;
	}

	fbc_render_aware_pre = fbc_render_aware;
	fbc_render_aware = 0;
	release_core();
	release_eas();
	fbc_tracer(-3, "render_aware", fbc_render_aware);

	mutex_unlock(&notify_lock);
}

static void notify_act_switch(int begin)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	if (!begin)
		act_switched = 1;
}

static void notify_game(int game)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	fbc_game = game;
}

static void notify_no_render_legacy(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	last_frame_done_in_budget = 1;
	release_freq_and_eas();
	disable_frame_twanted_timer();

	fbc_tracer(-3, "no_render", 1);
	fbc_tracer(-3, "no_render", 0);
}

static void notify_twanted_timeout_legacy(void)
{
	mutex_lock(&notify_lock);

	if (!is_ux_fbc_active() || frame_done) {
		mutex_unlock(&notify_lock);
		return;
	}

	chase_capacity = capacity * (100 + super_boost) / 100;
	if (chase_capacity > 1024)
		chase_capacity = 1024;
	boost_freq_and_eas(chase_capacity);
	boost_flag = 1;
	chase = 1;
	fbc_tracer(-3, "chase", chase);

	mutex_unlock(&notify_lock);
}

void notify_fbc_enable_legacy(int enable)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	fbc_render_aware_pre = fbc_render_aware = 1;

	if (enable) {
		chase = 0;
		first_frame = 1;
		last_frame_done_in_budget = 1;
		his_cap[0] = his_cap[1] = -1;

		if (act_switched) {
			capacity = touch_capacity = 347;
			act_switched = 0;
		} else
			capacity = touch_capacity = current_max_capacity;

		if (capacity > power_ll[0][1]) {
			big_enable = 1;
			boost_touch_core(big_enable);
		} else {
			big_enable = 0;
			boost_touch_core(big_enable);
		}

		sched_scheduler_switch(SCHED_HMP_LB);
		boost_flag = 0;
	} else {
		for (i = 0; i < MAX_THREAD; i++) {
			frame_info[i][0] = -1;
			frame_info[i][1] = -2;
		}

		release_core();
		sched_scheduler_switch(SCHED_HYBRID_LB);
		if (boost_flag) {
			release_freq_and_eas();
			boost_flag = 0;
		}
	}
}

void notify_frame_complete_legacy(unsigned long frame_time)
{
	long long boost_linear = 0;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	disable_render_aware_timer();
	enable_render_aware_timer();
	fbc_render_aware_pre = fbc_render_aware;
	fbc_render_aware = 1;
	fbc_tracer(-3, "render_aware", fbc_render_aware);

	if (!fbc_render_aware_pre && fbc_render_aware)
		boost_touch_core(big_enable);

	if (chase == 1) {
		chase = 0;
		fbc_tracer(-3, "chase", chase);
		avg_capacity = (twanted * capacity
				+ ((long long)frame_time - twanted) * (chase_capacity))
				/ (long long)frame_time;
	} else
		avg_capacity = capacity;

	/* upon rendering ends, update*/
	frame_done = 1;
	disable_frame_twanted_timer();
	if (first_frame) {
		avg_frame_time = frame_time;
	} else {
		avg_frame_time = (ema * frame_time + (10 - ema) * avg_frame_time) / 10;
	}
	boost_linear = (long long)((avg_frame_time - twanted) * 100 / twanted);

	his_cap[1] = his_cap[0];
	his_cap[0] = capacity;
	capacity = avg_capacity * (100 + boost_linear) / 100;

	if (capacity > 1024)
		capacity = 1024;
	if (capacity <= 0)
		capacity = 1;

	if (first_frame) {
		current_max_capacity = 0;
		first_frame = 0;
	} else
		current_max_capacity = MAX(current_max_capacity, capacity);

	if (frame_time < frame_budget) {
		release_freq_and_eas();
		boost_flag = 0;
		last_frame_done_in_budget = 1;
	} else {
		last_frame_done_in_budget = 0;
	}

	if (!last_frame_done_in_budget) {
		boost_freq_and_eas(capacity);
		boost_flag = 1;
	}

	if (big_enable) {
		if (capacity < power_ll[0][1]
				&& his_cap[0] != -1 && his_cap[1] != -1
				&& his_cap[0] > capacity && his_cap[1] > his_cap[0]) {
			big_enable = 0;
			boost_touch_core(big_enable);
		}
	} else {
		if (capacity > power_ll[0][1] ||
				(his_cap[0] != -1 &&
				 capacity + (capacity - his_cap[0]) * (HPS_LATENCY / frame_budget + 1)
				 > power_ll[0][1])) {
			big_enable = 1;
			boost_touch_core(big_enable);
		}
	}

	fbc_tracer(-3, "avg_frame_time", avg_frame_time);
	fbc_tracer(-3, "frame_time", frame_time);
	fbc_tracer(-3, "avg_capacity", avg_capacity);
	fbc_tracer(-3, "current_max_capacity", current_max_capacity);
	fbc_tracer(-4, "capacity", capacity);
	fbc_tracer(-4, "his_cap[0]", his_cap[0]);
	fbc_tracer(-4, "his_cap[1]", his_cap[1]);

#if 0
	pr_crit(TAG" frame complete FT=%lu, avg_frame_time=%lld, boost_linear=%lld, capacity=%d",
			frame_time, avg_frame_time, boost_linear, capacity);
	if (frame_time > twanted)
		pr_crit(TAG"chase_frame\n");
#endif

}

void notify_intended_vsync_legacy(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	frame_done = 0;
	enable_frame_twanted_timer();
	if (last_frame_done_in_budget) {
		boost_freq_and_eas(capacity);
		boost_flag = 1;
	}
	fbc_tracer(-3, "last_frame_done_in_budget", last_frame_done_in_budget);

}

static void notify_twanted_timeout_eas(void)
{
	int boost_real;

	mutex_lock(&notify_lock);

	if (!is_ux_fbc_active() || frame_done) {
		mutex_unlock(&notify_lock);
		return;
	}

	boost_real = linear_real_boost(super_boost);
	if (boost_real != 0)
		chase_boost1 = (100 + boost_real) * (100 + boost_value) / 100 - 100;

	if (chase_boost1 > 100)
		chase_boost1 = 100;
	perfmgr_kick_fg_boost(KIR_FBC, chase_boost1);
	fbc_tracer(-3, "boost_value", chase_boost1);

	boost_flag = 1;
	chase = 1;
	fbc_tracer(-3, "chase", chase);
	fbc_tracer(-3, "boost_linear", super_boost);

	mutex_unlock(&notify_lock);
}

void notify_fbc_enable_eas(int enable)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	fbc_render_aware_pre = fbc_render_aware = 1;

	if (enable) {
		for (i = 0; i < MAX_THREAD; i++) {
			frame_info[i][0] = -1;
			frame_info[i][1] = -2;
		}

		chase = 0;
		first_frame = 1;
		first_vsync = 0;
		fbc_tracer(-4, "first_frame", first_frame);
		his_bv[0] = his_bv[1] = -1;

		if (act_switched) {
			touch_boost_value = TOUCH_BOOST_EAS;
			act_switched = 0;
		} else {
			touch_boost_value = current_max_bv;
		}

		current_max_bv = 0;
		boost_touch_core_eas();

		boost_flag = 0;
	} else {

		release_core();
		if (boost_flag) {
			release_eas();
			boost_flag = 0;
		}
	}
}

static void notify_no_render_eas(void)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	/* check if current pid is in frame_info */
	for (i = 0; i < MAX_THREAD &&
			frame_info[i][0] != -1 &&
			current->tgid != frame_info[i][0]; i++)
		;

	/* if not in frame_info, add it*/
	if (i < MAX_THREAD && current->tgid == frame_info[i][0])
		frame_info[i][1] = 0;

	fbc_tracer(-5, "frame_info[0][0]", frame_info[0][0]);
	fbc_tracer(-5, "frame_info[0][1]", frame_info[0][1]);
	fbc_tracer(-5, "frame_info[1][0]", frame_info[1][0]);
	fbc_tracer(-5, "frame_info[1][1]", frame_info[1][1]);
	fbc_tracer(-5, "frame_info[2][0]", frame_info[2][0]);
	fbc_tracer(-5, "frame_info[2][1]", frame_info[2][1]);
	fbc_tracer(-5, "frame_info[3][0]", frame_info[3][0]);
	fbc_tracer(-5, "frame_info[3][1]", frame_info[3][1]);
	fbc_tracer(-5, "frame_info[4][0]", frame_info[4][0]);
	fbc_tracer(-5, "frame_info[4][1]", frame_info[4][1]);

	/* if all other threads are done */
	for (i = 0; i < MAX_THREAD &&
			frame_info[i][0] != -1 &&
			frame_info[i][1] == 0; i++)
		;

	if (i < MAX_THREAD && frame_info[i][0] == -1) {
		chase = 0;
		perfmgr_kick_fg_boost(KIR_FBC, -1);
		fbc_tracer(-3, "boost_value", 0);
		disable_frame_twanted_timer();

		fbc_tracer(-3, "no_render", 1);
		fbc_tracer(-3, "no_render", 0);
	}
}

void notify_frame_complete_eas(unsigned long frame_time)
{

	long long boost_linear = 0;
	int boost_real = 0, i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	/* check if current pid is in frame_info */
	for (i = 0; i < MAX_THREAD &&
			frame_info[i][0] != -1 &&
			current->tgid != frame_info[i][0]; i++)
		;

	if (i < MAX_THREAD && current->tgid == frame_info[i][0])
		frame_info[i][1] = frame_time;

	fbc_tracer(-5, "frame_info[0][0]", frame_info[0][0]);
	fbc_tracer(-5, "frame_info[0][1]", frame_info[0][1]);
	fbc_tracer(-5, "frame_info[1][0]", frame_info[1][0]);
	fbc_tracer(-5, "frame_info[1][1]", frame_info[1][1]);
	fbc_tracer(-5, "frame_info[2][0]", frame_info[2][0]);
	fbc_tracer(-5, "frame_info[2][1]", frame_info[2][1]);
	fbc_tracer(-5, "frame_info[3][0]", frame_info[3][0]);
	fbc_tracer(-5, "frame_info[3][1]", frame_info[3][1]);
	fbc_tracer(-5, "frame_info[4][0]", frame_info[4][0]);
	fbc_tracer(-5, "frame_info[4][1]", frame_info[4][1]);

	/* check if every thread done */
	for (i = 0; i < MAX_THREAD &&
			frame_info[i][0] != -1 &&
			frame_info[i][1] >= 0; i++)
		;
	/* if someone not done */
	if (i < MAX_THREAD && frame_info[i][1] == -1)
		return;

	/* evaluate overall frame_time */
	frame_time = 0;
	for (i = 0; i < MAX_THREAD && frame_info[i][0] != -1; i++) {
		frame_time = MAX(frame_time, frame_info[i][1]);
		frame_info[i][1] = 0;
	}

	fbc_tracer(-5, "frame_info[0][0]", frame_info[0][0]);
	fbc_tracer(-5, "frame_info[0][1]", frame_info[0][1]);
	fbc_tracer(-5, "frame_info[1][0]", frame_info[1][0]);
	fbc_tracer(-5, "frame_info[1][1]", frame_info[1][1]);
	fbc_tracer(-5, "frame_info[2][0]", frame_info[2][0]);
	fbc_tracer(-5, "frame_info[2][1]", frame_info[2][1]);
	fbc_tracer(-5, "frame_info[3][0]", frame_info[3][0]);
	fbc_tracer(-5, "frame_info[3][1]", frame_info[3][1]);
	fbc_tracer(-5, "frame_info[4][0]", frame_info[4][0]);
	fbc_tracer(-5, "frame_info[4][1]", frame_info[4][1]);


	if (chase == 1) {
		avg_boost = (twanted * boost_value
				+ ((long long)frame_time - twanted) * (chase_boost1))
				/ (long long)frame_time;
	} else if (chase == 2) {
		avg_boost = (twanted * boost_value
				+ ((long long)frame_budget - twanted) * chase_boost1
				+ ((long long)frame_time - frame_budget) * (chase_boost2))
				/ (long long)frame_time;
	} else
		avg_boost = boost_value;

	frame_done = 1;
	disable_frame_twanted_timer();

	if (first_frame)
		avg_frame_time = frame_time;
	else
		avg_frame_time = (ema * frame_time + (10 - ema) * avg_frame_time) / 10;

	if (chase == 2) {
		boost_linear = (long long)((avg_frame_time - (twanted-((long long)frame_time-frame_budget))) * 100
					/ (twanted-(frame_time-frame_budget)));
	} else {
		boost_linear = (long long)((avg_frame_time - twanted) * 100 / twanted);
	}

	his_bv[1] = his_bv[0];
	his_bv[0] = boost_value;

	if (boost_linear < 0)
		boost_real = (-1) * linear_real_boost((-1) * boost_linear);
	else
		boost_real = linear_real_boost(boost_linear);

	if (boost_real != 0)
		boost_value = (100 + boost_real) * (100 + avg_boost) / 100 - 100;

	if (boost_value > 100)
		boost_value = 100;
	else if (boost_value <= 0)
		boost_value = 0;

	if (first_frame) {
		if (first_vsync)
			first_frame = 0;
		fbc_tracer(-4, "first_frame", first_frame);
	} else {
		current_max_bv = MAX(current_max_bv, boost_value);
	}

	if (chase != 2) {
		perfmgr_kick_fg_boost(KIR_FBC, -1);
		fbc_tracer(-3, "boost_value", 0);
		boost_flag = 0;
	} else {
		perfmgr_kick_fg_boost(KIR_FBC, boost_value);
		fbc_tracer(-3, "boost_value", boost_value);
		boost_flag = 1;
	}

	chase = 0;
	fbc_tracer(-3, "chase", chase);

#if 0
	pr_crit(TAG" pid:%d, frame complete FT=%lu, boost_linear=%lld, boost_real=%d, boost_value=%d\n",
			current->tgid, frame_time, boost_linear, boost_real, boost_value);
	pr_crit(TAG" frame complete FT=%lu", frame_time);
#endif

	fbc_tracer(-3, "avg_frame_time", avg_frame_time);
	fbc_tracer(-3, "frame_time", frame_time);
	fbc_tracer(-3, "avg_boost", avg_boost);
	fbc_tracer(-3, "boost_linear", boost_linear);
	fbc_tracer(-3, "boost_real", boost_real);
	fbc_tracer(-4, "current_max_bv", current_max_bv);
	fbc_tracer(-4, "his_bv[0]", his_bv[0]);
	fbc_tracer(-4, "his_bv[1]", his_bv[1]);
}

void notify_intended_vsync_eas(void)
{
	int i, boost_real;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	disable_render_aware_timer();
	enable_render_aware_timer();
	fbc_render_aware_pre = fbc_render_aware;
	fbc_render_aware = 1;
	fbc_tracer(-3, "render_aware", fbc_render_aware);

	if (!fbc_render_aware_pre && fbc_render_aware)
		fbc_op->ux_enable(1);

	frame_done = 0;

	/* check if current pid is in frame_info */
	for (i = 0; i < MAX_THREAD &&
			frame_info[i][0] != -1 &&
			current->tgid != frame_info[i][0]; i++)
		;

	/* if not in frame_info, add it*/
	if (i < MAX_THREAD && frame_info[i][0] == -1) {
		frame_info[i][0] = current->tgid;
		frame_info[i][1] = -1;
	}
	if (i < MAX_THREAD && frame_info[i][0] == current->tgid)
		frame_info[i][1] = -1;

	enable_frame_twanted_timer();

	if (first_frame && !first_vsync) {
		boost_value = touch_boost_value;
		first_vsync = 1;
	}

	if (chase == 0) {
		perfmgr_kick_fg_boost(KIR_FBC, boost_value);
		fbc_tracer(-3, "boost_value", boost_value);
		boost_flag = 1;
	} else if (chase == 1) {
		chase = 2;
		fbc_tracer(-3, "chase", chase);
		boost_real = linear_real_boost(super_boost);
		fbc_tracer(-3, "boost_linear", super_boost);
		if (boost_real != 0)
			chase_boost2 = (100 + boost_real) * (100 + chase_boost1) / 100 - 100;

		if (chase_boost2 > 100)
			chase_boost2 = 100;
		perfmgr_kick_fg_boost(KIR_FBC, chase_boost2);
		fbc_tracer(-3, "boost_value", chase_boost2);
	}

	fbc_tracer(-5, "frame_info[0][0]", frame_info[0][0]);
	fbc_tracer(-5, "frame_info[0][1]", frame_info[0][1]);
	fbc_tracer(-5, "frame_info[1][0]", frame_info[1][0]);
	fbc_tracer(-5, "frame_info[1][1]", frame_info[1][1]);
	fbc_tracer(-5, "frame_info[2][0]", frame_info[2][0]);
	fbc_tracer(-5, "frame_info[2][1]", frame_info[2][1]);
	fbc_tracer(-5, "frame_info[3][0]", frame_info[3][0]);
	fbc_tracer(-5, "frame_info[3][1]", frame_info[3][1]);
	fbc_tracer(-5, "frame_info[4][0]", frame_info[4][0]);
	fbc_tracer(-5, "frame_info[4][1]", frame_info[4][1]);
}

struct fbc_operation_locked fbc_legacy = {
	.ux_enable		= notify_fbc_enable_legacy,
	.frame_cmplt	= notify_frame_complete_legacy,
	.intended_vsync	= notify_intended_vsync_legacy,
	.no_render	= notify_no_render_legacy,
	.game	= notify_game,
	.act_switch	= notify_act_switch
};

struct fbc_operation_locked fbc_eas = {
	.ux_enable		= notify_fbc_enable_eas,
	.frame_cmplt	= notify_frame_complete_eas,
	.intended_vsync	= notify_intended_vsync_eas,
	.no_render	= notify_no_render_eas,
	.game	= notify_game,
	.act_switch	= notify_act_switch
};


/*--------------------DEV OP------------------------*/
static ssize_t device_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[32], cmd[32];
	int i, arg;

	arg = 0;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%31s %d", cmd, &arg) != 2)
		return -EFAULT;

	if (strncmp(cmd, "debug", 5) == 0) {
		mutex_lock(&notify_lock);
		fbc_debug = arg;
		release_core();
		release_freq_and_eas();
		mutex_unlock(&notify_lock);
	} else if (strncmp(cmd, "init", 4) == 0) {
		if (boost_method == EAS)
			touch_boost_value = arg;
		else
			touch_capacity = arg;
	} else if (strncmp(cmd, "twanted", 7) == 0) {
		if (arg < 60) {
			twanted_ms = arg;
			twanted = twanted_ms * NSEC_PER_MSEC;
		}
	} else if (strncmp(cmd, "ema", 3) == 0) {
		ema = arg;
	} else if (strncmp(cmd, "method", 6) == 0) {
		mutex_lock(&notify_lock);
		boost_method = arg;
		switch (boost_method) {
		case EAS:
			fbc_op = &fbc_eas;
			break;
		case LEGACY:
			fbc_op = &fbc_legacy;
			break;
		default:
			fbc_op = &fbc_legacy;
			break;
		}
		mutex_unlock(&notify_lock);
	} else if (strncmp(cmd, "super_boost", 11) == 0) {
		super_boost = arg;
	} else if (strncmp(cmd, "dump_tbl", 8) == 0) {
		for (i = 0; i < 16; i++) {
			pr_crit(TAG"ll freq:%d, cap:%d", power_ll[i][0], power_ll[i][1]);
			pr_crit(TAG"l freq:%d, cap:%d", power_l[i][0], power_l[i][1]);
			pr_crit(TAG"b freq:%d, cap:%d", power_b[i][0], power_b[i][1]);
		}
	} else if (strncmp(cmd, "trace", 5) == 0) {
		fbc_trace = arg;
	}

	return cnt;
}

static int device_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "-----------------------------------------------------\n");
	SEQ_printf(m, "trace:\t%d\n", fbc_trace);
	SEQ_printf(m, "debug:\t%d\n", fbc_debug);
	SEQ_printf(m, "method:\t%d\n", boost_method);
	SEQ_printf(m, "ux state:\t%d\n", fbc_ux_state);
	SEQ_printf(m, "ema:\t%d\n", ema);
	if (boost_method == LEGACY)
		SEQ_printf(m, "init:\t%lld\n", touch_capacity);
	else
		SEQ_printf(m, "init:\t%d\n", touch_boost_value);
	SEQ_printf(m, "twanted:\t%lld\n", twanted);
	SEQ_printf(m, "first frame:\t%d\n", first_frame);
	SEQ_printf(m, "super_boost:\t%d\n", super_boost);
	SEQ_printf(m, "game mode:\t%d\n", fbc_game);
	SEQ_printf(m, "-----------------------------------------------------\n");
	return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}


static ssize_t ioctl_gaming(unsigned int cmd, unsigned long arg)
{

	/* TODO: need to check touch? */

	switch (cmd) {
	/*receive game info*/
	case IOCTL_WRITE_GM:
		fbc_op->game(arg);
		break;

	default:
		return 0;
	}

	return 0;
}

ssize_t device_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;

	mutex_lock(&notify_lock);
	if (fbc_debug)
		goto ret_ioctl;

	if (fbc_game) {
		ret = ioctl_gaming(cmd, arg);
		goto ret_ioctl;
	}


	/* start of ux fbc */
	switch (cmd) {
	/*receive game info*/
	case IOCTL_WRITE_GM:
		fbc_op->game(arg);
		break;

	/*receive act switch info*/
	case IOCTL_WRITE_AS:
		fbc_op->act_switch(arg);
		break;

	/*receive touch info*/
	case IOCTL_WRITE_TH:
		notify_touch(arg);
		break;

	/*receive frame_time info*/
	case IOCTL_WRITE_FC:
		if (!is_ux_fbc_active())
			goto ret_ioctl;
		fbc_op->frame_cmplt(arg);
		break;

	/*receive Intended-Vsync signal*/
	case IOCTL_WRITE_IV:
		if (!is_ux_fbc_active())
			goto ret_ioctl;
		fbc_op->intended_vsync();
		break;

	/*receive no-render signal*/
	case IOCTL_WRITE_NR:
	if (!is_ux_fbc_active())
		goto ret_ioctl;
		fbc_op->no_render();
		break;

	default:
		pr_crit(TAG "non-game unknown cmd %u\n", cmd);
		ret = -1;
		goto ret_ioctl;
	}

ret_ioctl:
	mutex_unlock(&notify_lock);
	return ret;
}

static const struct file_operations Fops = {
	.unlocked_ioctl = device_ioctl,
	.compat_ioctl = device_ioctl,
	.open = device_open,
	.write = device_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*--------------------INIT------------------------*/
static int __init init_fbc(void)
{
	struct proc_dir_entry *pe;

	int ret_val = 0;


	pr_crit(TAG"Start to init FBC driver\n");

	mark_addr = kallsyms_lookup_name("tracing_mark_write");

	frame_budget = 16 * NSEC_PER_MSEC;
	twanted = 12 * NSEC_PER_MSEC;
	twanted_ms = twanted / NSEC_PER_MSEC;
	boost_method = EAS;
	ema = 5;
	super_boost = 30;
	touch_boost_value = TOUCH_BOOST_EAS;
	capacity = current_max_capacity = touch_capacity = 347;

	update_pwd_tbl();
	mutex_init(&notify_lock);
	fbc_op = &fbc_eas;

	wq = create_singlethread_workqueue("mt_fbc_work");
	if (!wq) {
		pr_crit(TAG"work create fail\n");
		return -ENOMEM;
	}

	hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt.function = &mt_twanted_timeout;
	hrtimer_init(&hrt1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt1.function = &mt_touch_timeout;
	hrtimer_init(&hrt2, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt2.function = &mt_render_aware_timeout;

	ret_val = register_chrdev(DEV_MAJOR, DEV_NAME, &Fops);
	if (ret_val < 0) {
		pr_crit(TAG"%s failed with %d\n",
				"Registering the character device ",
				ret_val);
		goto out_wq;
	}

	pe = proc_create("perfmgr/fbc", 0664, NULL, &Fops);
	if (!pe) {
		ret_val = -ENOMEM;
		goto out_chrdev;
	}

	pr_crit(TAG"init FBC driver done\n");

	return 0;

out_chrdev:
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
out_wq:
	destroy_workqueue(wq);
	return ret_val;
}
late_initcall(init_fbc);


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

static void notify_twanted_timeout_eas(void);
static void notify_touch_up_timeout(void);
static void notify_render_aware_timeout(void);
static DECLARE_WORK(mt_tt_eas_work, (void *) notify_twanted_timeout_eas);
static DECLARE_WORK(mt_touch_timeout_work, (void *) notify_touch_up_timeout);
static DECLARE_WORK(mt_render_aware_timeout_work, (void *) notify_render_aware_timeout);
static struct workqueue_struct *wq;
static struct hrtimer hrt, hrt1, hrt2;
static unsigned long __read_mostly mark_addr;
struct mutex notify_lock;
EXPORT_SYMBOL(notify_lock);

static struct ppm_limit_data core_limit[NR_PPM_CLUSTERS];
static int fbc_debug, fbc_ux_state, fbc_ux_state_pre, fbc_render_aware, fbc_render_aware_pre;
static int fbc_trace, has_frame;
static long frame_budget, twanted, twanted_ms, avg_frame_time, queue_time;
static int ema, super_boost, boost_flag;
static int boost_value, touch_boost_value, current_max_bv, avg_boost, chase_boost1, chase_boost2;
static int first_frame, swap_buffers_begin, first_vsync, frame_done, chase, act_switched;
static long frame_info[MAX_THREAD][2]; /*0:current id, 1: frametime, if no render -1*/
static int his_bv[2];
static int vip_group[10];
int fbc_game;
EXPORT_SYMBOL(fbc_game);

struct fbc_operation_locked {
	void (*ux_enable)(int);
	void (*frame_cmplt)(long);
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

void release_core(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	core_limit[0].min = -1;
	core_limit[0].max = -1;
	core_limit[1].min = -1;
	core_limit[1].max = -1;
#if NR_PPM_CLUSTERS == 3
	core_limit[2].min = -1;
	core_limit[2].max = -1;
#endif
	update_userlimit_cpu_core(PPM_KIR_FBC, NR_PPM_CLUSTERS, core_limit);

	fbc_tracer(-4, "b_enable", 0);
}

void boost_touch_core_eas(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	core_limit[0].min = 3;
	core_limit[0].max = -1;
	core_limit[1].min = -1;
	core_limit[1].max = -1;
#if NR_PPM_CLUSTERS == 3
	core_limit[2].min = -1;
	core_limit[2].max = -1;
#endif
	update_userlimit_cpu_core(PPM_KIR_FBC, NR_PPM_CLUSTERS, core_limit);
	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 2100);
	fbc_tracer(-3, "boost_value", 100);
}

void release_eas(void)
{
	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
	fbc_tracer(-3, "boost_value", 0);
}
/*--------------------TIMER------------------------*/
static void enable_touch_up_timer(void)
{
	ktime_t ktime;

	ktime = ktime_set(TOUCH_TIMEOUT_SEC, 0);
	hrtimer_start(&hrt1, ktime, HRTIMER_MODE_REL);
}

static void disable_touch_up_timer(void)
{
	hrtimer_cancel(&hrt1);
}

static enum hrtimer_restart mt_touch_timeout(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &mt_touch_timeout_work);

	return HRTIMER_NORESTART;
}

static void enable_render_aware_timer(void)
{
	ktime_t ktime;

	ktime = ktime_set(0, NSEC_PER_MSEC * RENDER_AWARE_TIMEOUT_MSEC);
	hrtimer_start(&hrt2, ktime, HRTIMER_MODE_REL);
}

static enum hrtimer_restart mt_render_aware_timeout(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &mt_render_aware_timeout_work);

	return HRTIMER_NORESTART;
}

static void enable_frame_twanted_timer(void)
{
	ktime_t ktime;

	fbc_tracer(-6, "twanted_timer", 1);
	ktime = ktime_set(0, NSEC_PER_MSEC * twanted_ms);
	hrtimer_start(&hrt, ktime, HRTIMER_MODE_REL);
}

static void disable_frame_twanted_timer(void)
{

	fbc_tracer(-6, "twanted_timer", 0);
	hrtimer_cancel(&hrt);
}

static enum hrtimer_restart mt_twanted_timeout(struct hrtimer *timer)
{
	if (!wq)
		return HRTIMER_NORESTART;

	queue_work(wq, &mt_tt_eas_work);

	return HRTIMER_NORESTART;
}

/*--------------------FRAME HINT OP------------------------*/
void notify_touch(int action)
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
EXPORT_SYMBOL(notify_touch);

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
	if (fbc_game && fbc_ux_state == 1)
		notify_touch(0);
}

static void notify_twanted_timeout_eas(void)
{
	mutex_lock(&notify_lock);

	if (!is_ux_fbc_active() || frame_done || swap_buffers_begin) {
		mutex_unlock(&notify_lock);
		return;
	}

	chase_boost1 = super_boost;
	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, chase_boost1 + 2000);
	fbc_tracer(-3, "boost_value", chase_boost1);

	boost_flag = 1;
	chase = 1;
	fbc_tracer(-3, "chase", chase);

	mutex_unlock(&notify_lock);
}

void notify_fbc_enable_eas(int enable)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	fbc_render_aware_pre = fbc_render_aware = 1;

	if (fbc_ux_state_pre == 0 && fbc_ux_state == 1) {
		has_frame = 0;
		fbc_tracer(-3, "has_frame", has_frame);
	}

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

#ifdef CONFIG_MTK_SCHED_VIP_TASKS
		for (i = 0; i < 10 && vip_group[i] != -1; i++)
			vip_task_set(vip_group[i], 0);
#endif

		for (i = 0; i < 10; i++)
			vip_group[i] = -1;

		fbc_tracer(-6, "vip0", vip_group[0]);
		fbc_tracer(-6, "vip1", vip_group[1]);
		fbc_tracer(-6, "vip2", vip_group[2]);
		fbc_tracer(-6, "vip3", vip_group[3]);
		fbc_tracer(-6, "vip4", vip_group[4]);
		fbc_tracer(-6, "vip5", vip_group[5]);
		fbc_tracer(-6, "vip6", vip_group[6]);
		fbc_tracer(-6, "vip7", vip_group[7]);
		fbc_tracer(-6, "vip8", vip_group[8]);
		fbc_tracer(-6, "vip9", vip_group[9]);

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

	has_frame--;
	if (has_frame < 0)
		has_frame = 0;
	fbc_tracer(-3, "has_frame", has_frame);

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
		if (!first_frame) {
			update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
			fbc_tracer(-3, "boost_value", 0);
		}
		disable_frame_twanted_timer();

		fbc_tracer(-3, "no_render", 1);
		fbc_tracer(-3, "no_render", 0);
	}
}

void notify_frame_complete_eas(long frame_time)
{

	long boost_linear = 0;
	int boost_real = 0, i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	has_frame--;
	if (has_frame < 0)
		has_frame = 0;
	fbc_tracer(-3, "has_frame", has_frame);

	for (i = 0; i < 10 && vip_group[i] != -1 && vip_group[i] != current->pid; i++)
		;
	if (i < 10 && vip_group[i] == -1) {
		vip_group[i] = current->pid;
#ifdef CONFIG_MTK_SCHED_VIP_TASKS
		vip_task_set(vip_group[i], 1);
#endif
	}
	fbc_tracer(-6, "vip0", vip_group[0]);
	fbc_tracer(-6, "vip1", vip_group[1]);
	fbc_tracer(-6, "vip2", vip_group[2]);
	fbc_tracer(-6, "vip3", vip_group[3]);
	fbc_tracer(-6, "vip4", vip_group[4]);
	fbc_tracer(-6, "vip5", vip_group[5]);
	fbc_tracer(-6, "vip6", vip_group[6]);
	fbc_tracer(-6, "vip7", vip_group[7]);
	fbc_tracer(-6, "vip8", vip_group[8]);
	fbc_tracer(-6, "vip9", vip_group[9]);

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
		frame_time = MAX(frame_time, frame_info[i][1]) - queue_time;
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


	if (chase == 1 && frame_time) {
		avg_boost = (twanted * boost_value
				+ (frame_time - twanted) * (chase_boost1))
				/ frame_time;
	} else if (chase == 2 && frame_time) {
		avg_boost = (twanted * boost_value
				+ (frame_budget - twanted) * chase_boost1
				+ (frame_time - frame_budget) * (chase_boost2))
				/ frame_time;
	} else
		avg_boost = boost_value;

	if (first_frame)
		avg_frame_time = frame_time;
	else
		avg_frame_time = (ema * frame_time + (10 - ema) * avg_frame_time) / 10;

	boost_linear = ((avg_frame_time - twanted) * 100 / twanted);

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
	}

	current_max_bv = MAX(current_max_bv, boost_value);


	if (has_frame == 0) {
		disable_frame_twanted_timer();
		frame_done = 1;
		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
		fbc_tracer(-3, "boost_value", 0);
		boost_flag = 0;
		chase = 0;
		fbc_tracer(-3, "chase", chase);
	} else {
		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, boost_value + 2000);
		fbc_tracer(-3, "boost_value", boost_value);
		boost_flag = 1;
	}


#if 0
	pr_crit(TAG" pid:%d, frame complete FT=%lu, boost_linear=%lld, boost_real=%d, boost_value=%d\n",
			current->tgid, frame_time, boost_linear, boost_real, boost_value);
	pr_crit(TAG" frame complete FT=%lu", frame_time);
#endif

	fbc_tracer(-3, "avg_frame_time", avg_frame_time);
	fbc_tracer(-3, "frame_time", frame_time);
	fbc_tracer(-3, "queue_time", queue_time);
	fbc_tracer(-3, "avg_boost", avg_boost);
	fbc_tracer(-3, "boost_linear", boost_linear);
	fbc_tracer(-3, "boost_real", boost_real);
	fbc_tracer(-4, "current_max_bv", current_max_bv);
	fbc_tracer(-4, "his_bv[0]", his_bv[0]);
	fbc_tracer(-4, "his_bv[1]", his_bv[1]);
}

void notify_intended_vsync_eas(void)
{
	int i;

	/* lock is mandatory*/
	WARN_ON(!mutex_is_locked(&notify_lock));

	has_frame++;
	swap_buffers_begin = 0;
	fbc_tracer(-3, "has_frame", has_frame);
	fbc_tracer(-3, "swap_buffers_begin", swap_buffers_begin);

	for (i = 0; i < 10 && vip_group[i] != -1 && vip_group[i] != current->pid; i++)
		;
	if (i < 10 && vip_group[i] == -1) {
		vip_group[i] = current->pid;
#ifdef CONFIG_MTK_SCHED_VIP_TASKS
		vip_task_set(vip_group[i], 1);
#endif
	}
	fbc_tracer(-6, "vip0", vip_group[0]);
	fbc_tracer(-6, "vip1", vip_group[1]);
	fbc_tracer(-6, "vip2", vip_group[2]);
	fbc_tracer(-6, "vip3", vip_group[3]);
	fbc_tracer(-6, "vip4", vip_group[4]);
	fbc_tracer(-6, "vip5", vip_group[5]);
	fbc_tracer(-6, "vip6", vip_group[6]);
	fbc_tracer(-6, "vip7", vip_group[7]);
	fbc_tracer(-6, "vip8", vip_group[8]);
	fbc_tracer(-6, "vip9", vip_group[9]);

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

	if (chase == 0)
		enable_frame_twanted_timer();

	if (first_frame && !first_vsync) {
		boost_value = touch_boost_value;
		first_vsync = 1;
	}

	if (chase == 0) {
		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, boost_value + 2000);
		fbc_tracer(-3, "boost_value", boost_value);
		boost_flag = 1;
	} else {
		chase = 2;
		fbc_tracer(-3, "chase", chase);

		chase_boost2 = chase_boost1;
		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, chase_boost2 + 2000);
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
	int arg;

	arg = 0;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%31s %d", cmd, &arg) != 2)
		return -EFAULT;

	if (strncmp(cmd, "debug", 5) == 0) {
		mutex_lock(&notify_lock);
		fbc_debug = arg;
		release_core();
		release_eas();
		mutex_unlock(&notify_lock);
	} else if (strncmp(cmd, "init", 4) == 0) {
		touch_boost_value = arg;
	} else if (strncmp(cmd, "twanted", 7) == 0) {
		if (arg < 60) {
			twanted_ms = arg;
			twanted = twanted_ms * NSEC_PER_MSEC;
		}
	} else if (strncmp(cmd, "ema", 3) == 0) {
		ema = arg;
	} else if (strncmp(cmd, "super_boost", 11) == 0) {
		super_boost = arg;
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
	SEQ_printf(m, "ux state:\t%d\n", fbc_ux_state);
	SEQ_printf(m, "ema:\t%d\n", ema);
	SEQ_printf(m, "init:\t%d\n", touch_boost_value);
	SEQ_printf(m, "twanted:\t%ld\n", twanted);
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

long device_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	ssize_t ret = 0;

	mutex_lock(&notify_lock);
	if (fbc_debug) {
		if (cmd == IOCTL_WRITE_FC)
			fbc_tracer(-3, "frame_time", arg);
		goto ret_ioctl;
	}

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

	/*receive frame_time info*/
	case IOCTL_WRITE_FC:
		if (!is_ux_fbc_active())
			goto ret_ioctl;
		fbc_op->frame_cmplt((long)arg);
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

	/*receive queue_time signal*/
	case IOCTL_WRITE_SB:
	if (!is_ux_fbc_active())
		goto ret_ioctl;
		swap_buffers_begin = 1;
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
	int i;
	int ret_val = 0;

	for (i = 0; i < 10; i++)
		vip_group[i] = -1;


	pr_crit(TAG"Start to init FBC driver\n");

	mark_addr = kallsyms_lookup_name("tracing_mark_write");

	frame_budget = 16 * NSEC_PER_MSEC;
	twanted = 12 * NSEC_PER_MSEC;
	twanted_ms = twanted / NSEC_PER_MSEC;
	ema = 5;
	super_boost = SUPER_BOOST;
	touch_boost_value = TOUCH_BOOST_EAS;

	mutex_init(&notify_lock);
	fbc_op = &fbc_eas;

	wq = create_singlethread_workqueue("mt_fbc_work");
	if (!wq) {
		pr_crit(TAG"work create fail\n");
		return -ENOMEM;
	}
	init_fbc_touch();

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


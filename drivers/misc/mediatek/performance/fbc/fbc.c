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

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define EAS 1
#define LEGACY 2
#define NR_PPM_CLUSTERS 3
static void notify_twanted_timeout_legacy(void);
static DECLARE_WORK(mt_tt_legacy_work, (void *) notify_twanted_timeout_legacy);
static DEFINE_SPINLOCK(tlock);
static struct workqueue_struct *wq;
static struct mutex notify_lock;


static int boost_value;
static int touch_boost_value;
static int fbc_debug;
static int fbc_touch; /* 0: no touch & no render, 1: touch 2: render only*/
static int fbc_touch_pre; /* 0: no touch & no render, 1: touch 2: render only*/
static long long Twanted;
static long long Twanted_ms;
static long long avg_frame_time;
static int first_frame;
static int ema;
static int boost_method;
static int frame_done;
static int super_boost;
static int is_game;
static int is_30_fps;
static int fbc_trace;
static int chase;
static unsigned long __read_mostly mark_addr;

static struct ppm_limit_data freq_limit[NR_PPM_CLUSTERS];
static struct ppm_limit_data core_limit[NR_PPM_CLUSTERS];

static struct hrtimer hrt;
static long long capacity;
static long long touch_capacity;
static long long current_max_capacity;
static long long chase_capacity;
static long long avg_capacity;
static int act_switched;

static int power_ll[16][2];
static int power_l[16][2];
static int power_b[16][2];



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

static void boost_freq(int capacity)
{
	int i;

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
	fbc_tracer(-1, "fbc_freq_ll", freq_limit[0].max);
	fbc_tracer(-1, "fbc_freq_l", freq_limit[1].max);
	fbc_tracer(-1, "fbc_freq_b", freq_limit[2].max);
	fbc_tracer(-1, "capacity", capacity);

}

static void enable_frame_twanted_timer(void)
{
	unsigned long flags;
	ktime_t ktime;

	spin_lock_irqsave(&tlock, flags);
	ktime = ktime_set(0, NSEC_PER_MSEC * Twanted_ms);
	hrtimer_start(&hrt, ktime, HRTIMER_MODE_REL);
	spin_unlock_irqrestore(&tlock, flags);
}

static void disable_frame_twanted_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tlock, flags);
	hrtimer_cancel(&hrt);
	spin_unlock_irqrestore(&tlock, flags);
}

/*static void mt_power_pef_transfer(void)*/
static enum hrtimer_restart mt_twanted_timeout(struct hrtimer *timer)
{
	if (boost_method == LEGACY)
		if (wq)
			queue_work(wq, &mt_tt_legacy_work);

	return HRTIMER_NORESTART;
}

static void notify_act_switch(int begin)
{
	mutex_lock(&notify_lock);
	if (fbc_debug || is_game || !fbc_touch) {
		mutex_unlock(&notify_lock);
		return;
	}

	if (!begin)
		act_switched = 1;

	mutex_unlock(&notify_lock);
}

static void notify_no_render_legacy(void)
{
	mutex_lock(&notify_lock);
	if (fbc_debug || is_game || !fbc_touch) {
		mutex_unlock(&notify_lock);
		return;
	}
	fbc_tracer(-1, "no_render", 1);
	fbc_tracer(-1, "no_render", 0);
	mutex_unlock(&notify_lock);
}
static void notify_twanted_timeout_legacy(void)
{
	mutex_lock(&notify_lock);
	if (fbc_debug || is_game || !fbc_touch) {
		mutex_unlock(&notify_lock);
		return;
	}

	if (frame_done == 0) {
		chase_capacity = capacity * (100 + super_boost) / 100;
		if (chase_capacity > 1024)
			chase_capacity = 1024;

		boost_freq(chase_capacity);
		chase = 1;
		fbc_tracer(-1, "chase", chase);
	}

	mutex_unlock(&notify_lock);
}

void notify_touch_legacy(int touch)
{
	mutex_lock(&notify_lock);
	fbc_touch_pre = fbc_touch;
	fbc_touch = touch;
	fbc_tracer(-1, "touch", fbc_touch);

	if (fbc_touch && fbc_touch_pre == 0) {
		chase = 0;

		if (act_switched) {
			capacity = touch_capacity = 347;
			act_switched = 0;
		} else
			capacity = touch_capacity = current_max_capacity;

		core_limit[0].min = 3;
		core_limit[0].max = -1;
		core_limit[1].min = -1;
		core_limit[1].max = -1;
		core_limit[2].min = 1;
		core_limit[2].max = -1;
		update_userlimit_cpu_core(KIR_FBC, NR_PPM_CLUSTERS, core_limit);

		boost_freq(capacity);
		perfmgr_kick_fg_boost(KIR_FBC, 100);
	} else if (fbc_touch == 0 && fbc_touch_pre) {
		core_limit[0].min = -1;
		core_limit[0].max = -1;
		core_limit[1].min = -1;
		core_limit[1].max = -1;
		core_limit[2].min = -1;
		core_limit[2].max = -1;
		update_userlimit_cpu_core(KIR_FBC, NR_PPM_CLUSTERS, core_limit);

		freq_limit[0].min = -1;
		freq_limit[0].max = -1;
		freq_limit[1].min = -1;
		freq_limit[1].max = -1;
		freq_limit[2].min = -1;
		freq_limit[2].max = -1;
		update_userlimit_cpu_freq(KIR_FBC, NR_PPM_CLUSTERS, freq_limit);

		perfmgr_kick_fg_boost(KIR_FBC, -1);
	}
	mutex_unlock(&notify_lock);
}

void notify_frame_complete_legacy(unsigned long frame_time)
{
	long long boost_linear = 0;

	mutex_lock(&notify_lock);

	if (fbc_debug || is_game || !fbc_touch) {
		mutex_unlock(&notify_lock);
		return;
	}

	if (chase == 1) {
		avg_capacity = (Twanted * capacity
				+ ((long long)frame_time - Twanted) * (chase_capacity))
			/ (long long)frame_time;
	} else
		avg_capacity = capacity;

	chase = 0;
	fbc_tracer(-1, "chase", chase);
	frame_done = 1;
	disable_frame_twanted_timer();
	if (first_frame) {
		avg_frame_time = frame_time;
		first_frame = 0;
	} else {
		avg_frame_time = (ema * frame_time + (10 - ema) * avg_frame_time) / 10;
	}
	boost_linear = (long long)((avg_frame_time - Twanted) * 100 / Twanted);

	capacity = avg_capacity * (100 + boost_linear) / 100;

	if (capacity > 1024)
		capacity = 1024;
	if (capacity <= 0)
		capacity = 1;

	if (!first_frame)
		current_max_capacity = MAX(current_max_capacity, capacity);

	boost_freq(capacity);

	fbc_tracer(-1, "avg_frame_time", avg_frame_time);
	fbc_tracer(-1, "frame_time", frame_time);
	fbc_tracer(-1, "avg_capacity", avg_capacity);

#if 0
	pr_crit(TAG" frame complete FT=%lu, avg_frame_time=%lld, boost_linear=%lld, capacity=%d",
			frame_time, avg_frame_time, boost_linear, capacity);
	if (frame_time > Twanted)
		pr_crit(TAG"chase_frame\n");
#endif

	mutex_unlock(&notify_lock);
}

void notify_intended_vsync_legacy(void)
{
	mutex_lock(&notify_lock);
	if (fbc_debug || !fbc_touch || is_game) {
		mutex_unlock(&notify_lock);
		return;
	}

	frame_done = 0;
	enable_frame_twanted_timer();

	mutex_unlock(&notify_lock);
}

void notify_touch_eas(void)
{
#if 0
	fbc_touch_pre = fbc_touch;
	fbc_touch = arg1;
	if (!fbc_debug) {
		if (fbc_touch == 1) {
			boost_value  = (touch_boost_value + boost_value) / 2;
			boost_value_for_GED_idx(1, boost_value);
			pr_crit(TAG"TOUCH, boost_value=%d\n", boost_value);
			mt_kernel_trace_counter("boost_value", boost_value);
			avg_frame_time = 0;
			first_frame = 0;
		}
	}
	mt_kernel_trace_counter("Touch", fbc_touch);
	if (fbc_touch == 1 && fbc_touch_pre == 0) {
		boost_value  = touch_boost_value;
		boost_value_for_GED_idx(1, boost_value);
		pr_crit(TAG"TOUCH, boost_value=%d\n", boost_value);
		mt_kernel_trace_counter("boost_value", boost_value);
		avg_frame_time = 0;
		first_frame = 0;
	} else if (fbc_touch == 1) {

		if (boost_value <= 0)
			boost_value_super = super_boost;
		else if (boost_value + super_boost < 100)
			boost_value_super = super_boost + boost_value;
		else
			boost_value_super = 100;

		boost_value_for_GED_idx(1, boost_value_super);
		mt_kernel_trace_counter("boost_value", boost_value_super);
	}
}
mt_kernel_trace_counter("Touch", fbc_touch);
#endif
}

static void notify_no_render_eas(void)
{
}

void notify_frame_complete_eas(unsigned long frame_time)
{
	long long boost_linear = 0;
	int boost_real = 0;

	frame_done = 1;
	disable_frame_twanted_timer();
	if (first_frame) {
		boost_linear = super_boost;
		avg_frame_time = frame_time;
		first_frame = 0;
	} else {
		avg_frame_time = (ema * frame_time + (10 - ema) * avg_frame_time) / 10;
	}
	boost_linear = (long long)((avg_frame_time - Twanted) * 100 / Twanted);

	if (boost_linear < 0)
		boost_real = (-1)*linear_real_boost((-1)*boost_linear);
	else
		boost_real = linear_real_boost(boost_linear);

	if (boost_real != 0) {
		if (boost_value <= 0) {
			boost_value += boost_linear;
			/*
			 *if (boost_real > 0 && boost_value > boost_real)
			 *	boost_value = boost_real;
			 */
		} else {
			boost_value
				= (100 + boost_real) * (100 + boost_value) / 100 - 100;
		}
	}

	if (boost_value > 100)
		boost_value = 100;
	else if (boost_value <= 0)
		boost_value = 0;

	perfmgr_kick_fg_boost(KIR_FBC, boost_value);
	pr_crit(TAG" pid:%d, frame complete FT=%lu, boost_linear=%lld, boost_real=%d, boost_value=%d\n",
			current->pid, frame_time, boost_linear, boost_real, boost_value);

	pr_crit(TAG" frame complete FT=%lu", frame_time);
	/*linear_real_boost_pid(boost_linear, current->pid), 1);*/
	/* linear_real_boost(boost_linear));*/
	if (frame_time > Twanted)
		pr_crit(TAG"chase_frame\n");

}

void notify_intended_vsync_eas(void)
{
}


static ssize_t device_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[32];
	int arg1;
	char option[32];
	int i;
#if 0
	int boost_value_super;
#endif

	arg1 = 0;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%31s %d", option, &arg1) != 2)
		return -EFAULT;

	if (strncmp(option, "debug", 5) == 0) {
		fbc_debug = arg1;
		boost_value = 0;
		perfmgr_kick_fg_boost(KIR_FBC, boost_value);
	} else if (strncmp(option, "touch", 5) == 0) {
		if (fbc_debug || is_game)
			return cnt;
		if (boost_method == EAS)
			notify_touch_eas();
		else if (boost_method == LEGACY)
			notify_touch_legacy(arg1);
	} else if (strncmp(option, "act_switch", 5) == 0) {
		if (fbc_debug || is_game)
			return cnt;
		notify_act_switch(arg1);
	} else if (strncmp(option, "init", 4) == 0) {
		touch_boost_value = arg1;
		boost_value  = touch_boost_value;
		touch_capacity = arg1;
	} else if (strncmp(option, "twanted", 7) == 0) {
		Twanted = arg1 * 1000000;
		Twanted_ms = Twanted / 1000000;
	} else if (strncmp(option, "ema", 3) == 0)
		ema = arg1;
	 else if (strncmp(option, "method", 6) == 0)
		boost_method = arg1;
	 else if (strncmp(option, "super_boost", 11) == 0)
		super_boost = arg1;
	 else if (strncmp(option, "game", 4) == 0)
		is_game = arg1;
	 else if (strncmp(option, "30fps", 5) == 0)
		is_30_fps = arg1;
	 else if (strncmp(option, "dump_tbl", 8) == 0) {
		for (i = 0; i < 16; i++) {
			pr_crit(TAG"ll freq:%d, cap:%d", power_ll[i][0], power_ll[i][1]);
			pr_crit(TAG"b freq:%d, cap:%d", power_b[i][0], power_b[i][1]);
		 }
	 } else if (strncmp(option, "trace", 5) == 0)
		fbc_trace = arg1;

	return cnt;
}

static int device_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "-----------------------------------------------------\n");
	SEQ_printf(m, "touch: %d\n", fbc_touch);
	SEQ_printf(m, "init: %lld\n", touch_capacity);
	SEQ_printf(m, "debug: %d\n", fbc_debug);
	SEQ_printf(m, "twanted: %lld\n", Twanted);
	SEQ_printf(m, "first frame: %d\n", first_frame);
	SEQ_printf(m, "ez: %d\n", boost_method);
	SEQ_printf(m, "super_boost: %d\n", super_boost);
	SEQ_printf(m, "ema: %d\n", ema);
	SEQ_printf(m, "game mode: %d\n", is_game);
	SEQ_printf(m, "30 fps: %d\n", is_30_fps);
	SEQ_printf(m, "trace: %d\n", fbc_trace);
	SEQ_printf(m, "-----------------------------------------------------\n");
	return 0;
}
/*** Seq operation of mtprof ****/
static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}


ssize_t device_ioctl(struct file *filp,
	  unsigned int cmd, unsigned long arg)
{
	unsigned long frame_time;

	switch (cmd) {

	/*receive frame_time info*/
	case IOCTL_WRITE_FC:

		frame_time = arg;

		if (boost_method == EAS)
			notify_frame_complete_eas(frame_time);
		else if (boost_method == LEGACY)
			notify_frame_complete_legacy(frame_time);
		break;

	/*receive Intended-Vsync signal*/
	case IOCTL_WRITE_IV:

		if (boost_method == EAS)
			notify_intended_vsync_eas();
		else if (boost_method == LEGACY)
			notify_intended_vsync_legacy();
		break;

	case IOCTL_WRITE_NR:

		if (boost_method == EAS)
			notify_no_render_eas();
		else if (boost_method == LEGACY)
			notify_no_render_legacy();
		break;

	case IOCTL_WRITE3:
		if (!fbc_debug) {
			if (is_game) {
				if (arg == ID_EGL) {
					disable_frame_twanted_timer();
				} else if (arg == ID_OMR) {
					if (is_30_fps) {
						disable_frame_twanted_timer();
					}
				}
			}
		}
		break;

	default:
		return -1;
	}
	return 0;
}


#if 1
static const struct file_operations Fops = {
	.unlocked_ioctl = device_ioctl,
	.compat_ioctl = device_ioctl,
	.open = device_open,
	.write = device_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


#endif

/*--------------------INIT------------------------*/
static int __init init_fbc(void)
{
	struct proc_dir_entry *pe;

	int ret_val;


	pr_crit(TAG"init FBC driver start\n");

	boost_value = 50;
	chase = 0;
	fbc_debug = 0;
	fbc_touch = 0;
	fbc_touch_pre = 0;
	Twanted = 12000000;
	Twanted_ms = Twanted / 1000000;
	touch_boost_value = 50;
	boost_method = 2;
	avg_frame_time = 0;
	first_frame = 1;
	ema = 5;
	frame_done = 0;
	super_boost = 30;
	is_game = 0;
	is_30_fps = 0;
	touch_capacity = 347;
	act_switched = 0;
	capacity = current_max_capacity = touch_capacity;
	fbc_trace = 0;
	mark_addr = kallsyms_lookup_name("tracing_mark_write");

	update_pwd_tbl();
	mutex_init(&notify_lock);

	wq = create_singlethread_workqueue("mt_tt_legacy_work");
	if (!wq)
		pr_crit(TAG"mt_tt_legacy_work create fail\n");

	ret_val = register_chrdev(DEV_MAJOR, DEV_NAME, &Fops);
	if (ret_val < 0) {
		pr_crit(TAG"%s failed with %d\n",
				"Sorry, registering the character device ",
				ret_val);
		return ret_val;
	}

	hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt.function = &mt_twanted_timeout;

	pe = proc_create("perfmgr/fbc", 0664, NULL, &Fops);
	if (!pe)
		return -ENOMEM;

	pr_crit(TAG"init FBC driver end\n");

	return 0;
}
late_initcall(init_fbc);


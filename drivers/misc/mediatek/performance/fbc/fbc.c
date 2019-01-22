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

static void enable_frame_twanted_timer(void)
{
	unsigned long flags;
	ktime_t ktime;

	/*mt_kernel_trace_counter("Timeout", 0);*/
	spin_lock_irqsave(&tlock, flags);
/*        if (!timer_pending(&mt_pp_transfer_timer)) {*/
/*		mt_pp_transfer_timer.expires = jiffies + msecs_to_jiffies(X_ms);*/
/*		add_timer(&mt_pp_transfer_timer);*/
/*        }*/
	ktime = ktime_set(0, NSEC_PER_MSEC*X_ms);
	hrtimer_start(&hrt, ktime, HRTIMER_MODE_REL);
	spin_unlock_irqrestore(&tlock, flags);
}

static void disable_frame_twanted_timer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&tlock, flags);
/*	del_timer(&mt_pp_transfer_timer);*/
	hrtimer_cancel(&hrt);
	spin_unlock_irqrestore(&tlock, flags);
}

/*static void mt_power_pef_transfer(void)*/
static enum hrtimer_restart mt_power_pef_transfer(struct hrtimer *timer)
{
	mt_power_pef_transfer_work();
	pr_crit(TAG"chase_frame\n");
	return HRTIMER_NORESTART;
}

static void mt_power_pef_transfer_work(void)
{
	int boost_value_super;

	if (frame_done == 0)
		if (boost_value < 0)
			boost_value_super = SUPER_BOOST;
		else if (boost_value + SUPER_BOOST < 100)
			boost_value_super = SUPER_BOOST + boost_value;
		else
			boost_value_super = 100;
		/*boost_value_for_GED_idx(1, boost_value_super);*/
			/*mt_kernel_trace_counter("Timeout", 1);*/
			/*mt_kernel_trace_counter("boost_value", boost_value_super);*/
	}
}

static ssize_t device_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	int ret;
	unsigned long arg1, arg2;
	char option[64], arg[10];
	int i, j;
#if 0
	int boost_value_super;
#endif

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	/* get option */
	for (i = 0; i < cnt && buf[i] != ' '; i++)
		option[i] = buf[i];
	option[i] = '\0';

	/* get arg1 */
	for (; i < cnt && buf[i] == ' '; i++)
		;
	for (j = 0; i < cnt && buf[i] != ' '; i++, j++)
		arg[j] = buf[i];
	arg[j] = '\0';
	if (j > 0) {
		ret = kstrtoul(arg, 0, (unsigned long *)&arg1);
		if (ret < 0) {
			pr_debug(TAG"1 ret of kstrtoul is broke\n");
			return ret;
		}
	}

	/* get arg2 */
	for (; i < cnt && buf[i] == ' '; i++)
		;
	for (j = 0; i < cnt && buf[i] != ' '; i++, j++)
		arg[j] = buf[i];
	arg[j] = '\0';
	if (j > 0) {
		ret = kstrtoul(arg, 0, (unsigned long *)&arg2);
		if (ret < 0) {
			pr_debug(TAG"2 ret of kstrtoul is broke\n");
			return ret;
		}
	}

	if (strncmp(option, "DEBUG", 5) == 0) {
		fbc_debug = arg1;
		boost_value = 0;
		/*boost_value_for_GED_idx(1, boost_value);*/
	} else if (strncmp(option, "TOUCH", 5) == 0) {
#if 0
		fbc_touch_pre = fbc_touch;
		fbc_touch = arg1;
		if (!fbc_debug) {
			if (fbc_touch == 1) {
				boost_value  = (touch_boost_value + boost_value) / 2;
				boost_value_for_GED_idx(1, boost_value);
				pr_crit(TAG"TOUCH, boost_value=%d\n", boost_value);
				mt_kernel_trace_counter("boost_value", boost_value);
				avgFT = 0;
				first_frame = 0;
			}
		}
		mt_kernel_trace_counter("Touch", fbc_touch);
#endif
#if 0
			if (fbc_touch == 1 && fbc_touch_pre == 0) {
				boost_value  = touch_boost_value;
				boost_value_for_GED_idx(1, boost_value);
				pr_crit(TAG"TOUCH, boost_value=%d\n", boost_value);
				mt_kernel_trace_counter("boost_value", boost_value);
				avgFT = 0;
				first_frame = 0;
			} else if (fbc_touch == 1) {

				if (boost_value <= 0)
					boost_value_super = SUPER_BOOST;
				else if (boost_value + SUPER_BOOST < 100)
					boost_value_super = SUPER_BOOST + boost_value;
				else
					boost_value_super = 100;

				boost_value_for_GED_idx(1, boost_value_super);
				mt_kernel_trace_counter("boost_value", boost_value_super);
			}
		}
		mt_kernel_trace_counter("Touch", fbc_touch);
#endif
	} else if (strncmp(option, "INIT", 4) == 0) {
		touch_boost_value = arg1;
		boost_value  = touch_boost_value;
	} else if (strncmp(option, "TWANTED", 7) == 0) {
		Twanted = arg1 * 1000000;
		X_ms = Twanted / 1000000;
	} else if (strncmp(option, "EMA", 3) == 0)
		EMA = arg1;
	 else if (strncmp(option, "METHOD", 6) == 0)
		boost_method = arg1;
	 else if (strncmp(option, "SUPER_BOOST", 11) == 0)
		SUPER_BOOST = arg1;
	 else if (strncmp(option, "GAME", 4) == 0)
		is_game = arg1;
	 else if (strncmp(option, "30FPS", 5) == 0)
		is_30_fps = arg1;

	return cnt;
}

static int device_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "-----------------------------------------------------\n");
	SEQ_printf(m, "TOUCH: %d\n", fbc_touch);
	SEQ_printf(m, "INIT: %d\n", touch_boost_value);
	SEQ_printf(m, "DEBUG: %d\n", fbc_debug);
	SEQ_printf(m, "TWANTED: %d\n", Twanted);
	SEQ_printf(m, "first frame: %d\n", first_frame);
	SEQ_printf(m, "EZ: %d\n", boost_method);
	SEQ_printf(m, "SUPER_BOOST: %d\n", SUPER_BOOST);
	SEQ_printf(m, "EMA: %d\n", EMA);
	SEQ_printf(m, "game mode: %d\n", is_game);
	SEQ_printf(m, "30 fps: %d\n", is_30_fps);
	SEQ_printf(m, "-----------------------------------------------------\n");
	return 0;
}
/*** Seq operation of mtprof ****/
static int device_open(struct inode *inode, struct file *file)
{
	return single_open(file, device_show, inode->i_private);
}


/*--------------------INIT------------------------*/
#define TIME_5SEC_IN_MS 5000


ssize_t device_ioctl(struct file *filp,
	  unsigned int cmd, unsigned long arg)
{
	int frame_time = arg;
	int boost_linear = 0;
	int boost_real = 0;
	/*int boost_real = 0;*/

	switch (cmd) {
		/*receive frame_time info*/
	case IOCTL_WRITE:
		frame_time = arg;
		if (!fbc_debug && !is_game) {
			if (boost_method == 0) {
				if (fbc_touch) {
					if (arg < Twanted && boost_value >= 10)
						boost_value -= 10;
					if (arg >= Twanted && boost_value <= 90)
						boost_value += 10;
					/*boost_value_for_GED_idx(1, boost_value);*/
					pr_crit(TAG" FT=%lu, boost_value=%d\n", arg, boost_value);
				}
			} else if (boost_method == 1) {
				frame_done = 1;
				disable_frame_twanted_timer();
				if (first_frame) {
					boost_linear = SUPER_BOOST;
					avgFT = frame_time;
					first_frame = 0;
				} else {
					avgFT = (EMA * frame_time + (10 - EMA) * avgFT) / 10;
				}
#if 0
				boost_linear
					= (boost_linear + 100)
					* (100 + (int)((avgFT - Twanted) * 100 / Twanted)) / 100 - 100;
				if (boost_linear <= 0)
					boost_linear = 0;

				boost_value = (EMA * boost_value + (10 - EMA) * linear_real_boost(boost_linear)) / 10;

#else
				boost_linear = (int)((avgFT - Twanted) * 100 / Twanted);

				if (boost_linear < 0)
					/*boost_real = (-1)*linear_real_boost((-1)*boost_linear);*/
				else
					/*boost_real = linear_real_boost(boost_linear);*/

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
			}

			if (boost_value > 100)
				boost_value = 100;
			else if (boost_value <= 0)
				boost_value = 0;
#endif

			/*boost_value_for_GED_idx(1, boost_value);*/
			/*pr_crit(TAG" pid:%d, frame complete FT=%lu, boost_linear=%d,
			 * boost_value=%d boost_value_sys=%d\n", current->pid, arg, boost_linear, 1, 1);
			 */
			pr_crit(TAG" frame complete FT=%lu", arg);
			/*linear_real_boost_pid(boost_linear, current->pid), 1);*/
			/* linear_real_boost(boost_linear));*/
			if (frame_time > Twanted)
				pr_crit(TAG"chase_frame\n");
			/*mt_kernel_trace_counter("boost_real", boost_real);*/
			/*mt_kernel_trace_counter("boost_linear", boost_linear);*/
			/*mt_kernel_trace_counter("boost_value", boost_value);*/
			/*mt_kernel_trace_counter("frame_time", frame_time);*/
			/*mt_kernel_trace_counter("averaged_frame_time", avgFT);*/
		}

	break;
	/*receive Intended-Vsync signal*/
	case IOCTL_WRITE1:
		if (!fbc_debug) {
			/*if (!is_game) {*/
			/*	if (boost_method == 1) {*/
			/*		frame_done = 0;*/
			/*		enable_frame_twanted_timer();*/
			pr_crit(TAG" frame: Intended Vsync\n");
			/*		mt_kernel_trace_counter("averaged_frame_time", avgFT);*/
					/*trace_counter("Intended Vsync:enable_twanted_timer", 1);*/
					/*trace_sched_heavy_task("Intended Vsync: enable_twanted_timer");*/
					/*	}*/
			/*}*/
		}
		break;
	case IOCTL_WRITE2:
		if (!fbc_debug) {
			if (is_game) {
				/*pr_crit(TAG" frame: Vsync-sf\n");*/
				enable_frame_twanted_timer();
				/*trace_counter("Vsync-sf: enable_twanted_timer", 1);*/
				/*trace_sched_heavy_task("Vsync-f: ensable_twanted_timer");*/
			}
		}
		break;
	case IOCTL_WRITE3:
		if (!fbc_debug) {
			if (is_game) {
				if (arg == ID_EGL) {
					/*pr_crit(TAG" frame: EGL\n");*/
					disable_frame_twanted_timer();
					/*trace_counter("EGL: disable_twanted_timer", 1);*/
					/*trace_sched_heavy_task("EGL: disable_twanted_timer");*/
				} else if (arg == ID_OMR) {
					/* com.tencent.tmgp.sgame */
					if (is_30_fps) {
						/*pr_crit(TAG" frame: OMR\n");*/
						disable_frame_twanted_timer();
						/*trace_counter("OMR: disable_twanted_timer", 1);*/
						/*trace_sched_heavy_task("OMR: disable_twanted_timer");*/
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
	/*.owner = THIS_MODULE,*/
	/*.poll = binder_poll,*/
	.unlocked_ioctl = device_ioctl,
	.compat_ioctl = device_ioctl,
	/*.mmap = binder_mmap,*/
	.open = device_open,
	.write = device_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	/*.flush = binder_flush,*/
	/*.release = binder_release,*/
};


#endif

static int __init init_fbc(void)
{
	struct proc_dir_entry *pe;

	int ret_val;

	boost_value = 50;
	fbc_debug = 0;
	fbc_touch = 0;
	fbc_touch_pre = 0;
	Twanted = 12000000;
	X_ms = Twanted / 1000000;
	touch_boost_value = 50;
	boost_method = 1;
	avgFT = 0;
	first_frame = 1;
	EMA = 5;
	frame_done = 0;
	SUPER_BOOST = 30;
	is_game = 0;
	is_30_fps = 0;


	pr_crit(TAG"init FBC driver start\n");
	ret_val = register_chrdev(DEV_MAJOR, DEV_NAME, &Fops);
	if (ret_val < 0) {
		pr_crit(TAG"%s failed with %d\n",
				"Sorry, registering the character device ",
				ret_val);
		return ret_val;
	}

	hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt.function = &mt_power_pef_transfer;

	pr_crit(TAG"init FBC driver end\n");


	pr_crit(TAG"init fbc driver start\n");
	pe = proc_create("fbc", 0664, NULL, &Fops);
	if (!pe)
		return -ENOMEM;


	return 0;
}
late_initcall(init_fbc);


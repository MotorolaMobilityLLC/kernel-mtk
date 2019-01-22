/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define TAG "[Boost Controller]"

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <mt-plat/mtk_sched.h>
#include <mt-plat/fpsgo_v2_common.h>

#ifdef CONFIG_TRACING
#include <linux/kallsyms.h>
#include <linux/trace_events.h>
#endif

static struct hrtimer ext_launch_timer;
static struct notifier_block ext_launch_lcmoff_fb_notifier;

static void walt_mode(int enable)
{
#ifdef CONFIG_SCHED_WALT
	sched_walt_enable(LT_WALT_POWERHAL, enable);
#else
	pr_debug("walt not be configured\n");
#endif
}

#ifdef CONFIG_TRACING
static unsigned long __read_mostly tracing_mark_write_addr;
static inline void __mt_update_tracing_mark_write_addr(void)
{
	if (unlikely(tracing_mark_write_addr == 0))
		tracing_mark_write_addr = kallsyms_lookup_name("tracing_mark_write");
}

static inline void eas_ctrl_extend_kernel_trace_begin(char *name, int id, int walt, int fpsgo)
{
	__mt_update_tracing_mark_write_addr();
	preempt_disable();
	event_trace_printk(tracing_mark_write_addr, "B|%d|%s|%d|%d|%d\n", current->tgid, name, id, walt, fpsgo);
	preempt_enable();
}

static inline void eas_ctrl_extend_kernel_trace_end(void)
{
	__mt_update_tracing_mark_write_addr();
	preempt_disable();
	event_trace_printk(tracing_mark_write_addr, "E\n");
	preempt_enable();
}
#endif

void ext_launch_cond(int cond_check)
{
	switch (cond_check) {
	case 0:
	case 2:
		hrtimer_cancel(&ext_launch_timer);
		pr_debug("ext_launch_%s\n", ((cond_check == 2) ? "pre" : "lcm_off"));
#ifdef CONFIG_TRACING
		eas_ctrl_extend_kernel_trace_begin("ext_launch_cond", cond_check, 0, 0);
#endif
		fpsgo_switch_enable_keep(0);
		walt_mode(0);
#ifdef CONFIG_TRACING
		eas_ctrl_extend_kernel_trace_end();
#endif
		break;
	case 1:
		pr_debug("ext_launch_lcm_on\n");
#ifdef CONFIG_TRACING
		eas_ctrl_extend_kernel_trace_begin("ext_launch_cond", cond_check, 0, 1);
#endif
		walt_mode(0);
		fpsgo_switch_enable_keep(1);
#ifdef CONFIG_TRACING
		eas_ctrl_extend_kernel_trace_end();
#endif
		break;
	default:
		break;
	}
}

void ext_launch_start(void)
{
	ktime_t ktime;

	ktime = ktime_set(30, 0);
	pr_debug("ext_launch_start\n");
	/*--feature start from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_start", 0, 1, 0);
#endif

	fpsgo_switch_enable_keep(0);
	walt_mode(1);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
	hrtimer_start(&ext_launch_timer, ktime, HRTIMER_MODE_REL);
}

void ext_launch_end(void)
{
	pr_debug("ext_launch_end\n");
	/*--feature end from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_end", 0, 0, 1);
#endif
	fpsgo_switch_enable_keep(1);
	walt_mode(0);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
}

enum hrtimer_restart ext_launch_notify_method(struct hrtimer *timer)
{
	pr_debug("ext_launch_notify_method is called\n");

	ext_launch_end();

	return HRTIMER_NORESTART;
}

static int ext_launch_lcmoff_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	if (!evdata) {
		pr_info("evdata is NULL!!\n");
		return 0;
	}

	blank = *(int *)evdata->data;
	pr_info("@%s: blank = %d, event = %lu\n", __func__, blank, event);
	/* skip if it's not a blank event */
	if (event != FB_EVENT_BLANK)
		return 0;
	switch (blank) {
	/* LCM ON */
	case FB_BLANK_UNBLANK:
		ext_launch_cond(1);
		break;
	/* LCM OFF */
	case FB_BLANK_POWERDOWN:
		ext_launch_cond(0);
		break;
	default:
		break;
	}
	return 0;
}

void ext_launch_notify_init(void)
{
	int ret;
	ktime_t ktime;

	ktime = ktime_set(30, 0);
	hrtimer_init(&ext_launch_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ext_launch_timer.function = ext_launch_notify_method;

	pr_debug("ext_launch_notify_init_timer\n");

	ext_launch_lcmoff_fb_notifier = (struct notifier_block){
		.notifier_call = ext_launch_lcmoff_fb_notifier_callback,
	};

	ret = fb_register_client(&ext_launch_lcmoff_fb_notifier);
	if (ret)
		pr_info("@%s: lcmoff policy register FB client failed!\n", __func__);

	pr_debug("ext_launch_notify_init_fb\n");
}

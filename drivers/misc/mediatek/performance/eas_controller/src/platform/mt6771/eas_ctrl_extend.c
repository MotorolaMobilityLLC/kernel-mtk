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

void ext_launch_start(void)
{
	pr_debug("ext_launch_start\n");
	/*--feature start from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_start", 0, 1, 0);
#endif

	walt_mode(1);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
}

void ext_launch_end(void)
{
	pr_debug("ext_launch_end\n");
	/*--feature end from here--*/
#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_begin("ext_launch_end", 0, 0, 1);
#endif
	walt_mode(0);

#ifdef CONFIG_TRACING
	eas_ctrl_extend_kernel_trace_end();
#endif
}

void eas_ctrl_extend_notify_init(void)
{
	pr_debug("eas_ctl_extend_notify_init\n");
}

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

#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/trace_events.h>
#include <linux/debugfs.h>
#include <fpsgo_common.h>

#include "fstb/fstb.h"
#include "fbt/include/fbt_cpu.h"
#include "fbt/include/fbt_notifier.h"
#include "../fbc/fbc.h"

uint32_t fpsgo_systrace_mask;
int fpsgo_benchmark_hint;
struct dentry *fpsgo_debugfs_dir;

static unsigned long __read_mostly mark_addr;
static struct dentry *debugfs_common_dir;

#define GENERATE_STRING(name, unused) #name
static const char * const mask_string[] = {
	FPSGO_SYSTRACE_LIST(GENERATE_STRING)
};

#define FPSGO_DEBUGFS_ENTRY(name) \
static int fpsgo_##name##_open(struct inode *i, struct file *file) \
{ \
	return single_open(file, fpsgo_##name##_show, i->i_private); \
} \
\
static const struct file_operations fpsgo_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = fpsgo_##name##_open, \
	.read = seq_read, \
	.write = fpsgo_##name##_write, \
	.llseek = seq_lseek, \
	.release = single_release, \
}

void __fpsgo_systrace_c(pid_t pid, int val, const char *fmt, ...)
{
	char log[256];
	va_list args;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	preempt_disable();
	event_trace_printk(mark_addr, "C|%d|%s|%d\n", pid, log, val);
	preempt_enable();
}

void __fpsgo_systrace_b(pid_t tgid, const char *fmt, ...)
{
	char log[256];
	va_list args;

	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);

	preempt_disable();
	event_trace_printk(mark_addr, "B|%d|%s\n", tgid, log);
	preempt_enable();
}

void __fpsgo_systrace_e(void)
{
	preempt_disable();
	event_trace_printk(mark_addr, "E\n");
	preempt_enable();
}

int fpsgo_is_fstb_enable(void)
{
	return is_fstb_enable();
}

int fpsgo_switch_fstb(int enable)
{
	return switch_fstb(enable);
}

int fpsgo_fstb_sample_window(long long time_usec)
{
	return switch_sample_window(time_usec);
}

int fpsgo_fstb_fps_range(int nr_level, struct fps_level *level)
{
	return switch_fps_range(nr_level, level);
}

int fpsgo_fstb_fps_error_threhosld(int threshold)
{
	return switch_fps_error_threhosld(threshold);
}

int fpsgo_fstb_percentile_frametime(int ratio)
{
	return switch_percentile_frametime(ratio);
}

int fpsgo_fstb_force_vag(int arg)
{
	return switch_force_vag(arg);
}

int fpsgo_fstb_vag_fps(int arg)
{
	return switch_vag_fps(arg);
}

int fpsgo_switch_fbt_game(int enable)
{
	return switch_fbt_game(enable);
}

void fpsgo_switch_fbt_ux(int arg)
{
	return switch_fbc(arg);
}

void fpsgo_game_enable(int enable)
{
	int off = !enable;

	if (fpsgo_benchmark_hint == off || off > 1 || off < 0)
		return;

	fpsgo_benchmark_hint = off;
	fbt_notifier_push_benchmark_hint(fpsgo_benchmark_hint);
}

void fpsgo_switch_twanted(int arg)
{
	return switch_twanted(arg);
}

void fpsgo_switch_init_boost(int arg)
{
	return switch_init_boost(arg);
}

void fpsgo_switch_ema(int arg)
{
	return switch_ema(arg);
}

void fpsgo_switch_super_boost(int arg)
{
	return switch_super_boost(arg);
}

static int fpsgo_systrace_mask_show(struct seq_file *m, void *unused)
{
	int i;

	seq_puts(m, " Current enabled systrace:\n");
	for (i = 0; (1U << i) < FPSGO_DEBUG_MAX; i++)
		seq_printf(m, "  %-*s ... %s\n", 12, mask_string[i],
			   fpsgo_systrace_mask & (1U << i) ?
			     "On" : "Off");
	return 0;
}

static ssize_t fpsgo_systrace_mask_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	uint32_t val;
	int ret;

	ret = kstrtou32_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fpsgo_systrace_mask = val & (FPSGO_DEBUG_MAX - 1U);
	return cnt;
}

FPSGO_DEBUGFS_ENTRY(systrace_mask);

static int fpsgo_benchmark_hint_show(struct seq_file *m, void *unused)
{
	seq_printf(m, "%d\n", !fpsgo_benchmark_hint);
	return 0;
}

static ssize_t fpsgo_benchmark_hint_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val, off;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	off = !val;
	if (fpsgo_benchmark_hint == off || off > 1 || off < 0)
		return cnt;

	fpsgo_benchmark_hint = off;
	fbt_notifier_push_benchmark_hint(fpsgo_benchmark_hint);
	return cnt;
}

FPSGO_DEBUGFS_ENTRY(benchmark_hint);

static int __init init_fpsgo_common(void)
{
	fpsgo_debugfs_dir = debugfs_create_dir("fpsgo", NULL);
	if (!fpsgo_debugfs_dir)
		return -ENODEV;

	debugfs_common_dir = debugfs_create_dir("common",
						fpsgo_debugfs_dir);
	if (!debugfs_common_dir)
		return -ENODEV;

	debugfs_create_file("systrace_mask",
			    S_IRUGO | S_IWUSR,
			    debugfs_common_dir,
			    NULL,
			    &fpsgo_systrace_mask_fops);

	debugfs_create_file("fpsgo_game_enable",
			    S_IRUGO | S_IWUSR,
			    debugfs_common_dir,
			    NULL,
			    &fpsgo_benchmark_hint_fops);

	mark_addr = kallsyms_lookup_name("tracing_mark_write");
	fpsgo_systrace_mask = FPSGO_DEBUG_MANDATORY;

	return 0;
}

device_initcall(init_fpsgo_common);

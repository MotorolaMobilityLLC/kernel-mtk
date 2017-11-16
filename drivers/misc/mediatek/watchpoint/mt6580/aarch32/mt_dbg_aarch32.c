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

#include <linux/device.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#ifdef CONFIG_SMP
#include <linux/cpu.h>
#endif

#include "hw_watchpoint_internal.h"

#define MAX_NR_WATCH_POINT 4
#define MAX_NR_BREAK_POINT 6

unsigned int *mt_save_dbg_regs(unsigned int *data, unsigned int cpu_id)
{
	unsigned int dbg_nr = hw_watchpoint_nr();
	int i;

	cpu_id &= 0xf;
	data[0] = readl(DBGDSCR(cpu_id));
	for (i = 0; i < dbg_nr; ++i) {
		data[i * 2 + 1] = readl(DBGWVR_BASE(cpu_id) + i * 4);
		data[i * 2 + 2] = readl(DBGWCR_BASE(cpu_id) + i * 4);
	}

	for (i = 0; i < MAX_NR_BREAK_POINT; ++i) {
		data[i * 2 + 9] = readl(DBGBVR_BASE(cpu_id) + i * 4);
		data[i * 2 + 10] = readl(DBGBCR_BASE(cpu_id) + i * 4);
	}

	return data;
}

void mt_restore_dbg_regs(unsigned int *data, unsigned int cpu_id)
{
	unsigned int dbg_nr = hw_watchpoint_nr();
	int i;

	cpu_id &= 0xf;
	writel(UNLOCK_KEY, DBGLAR(cpu_id));
	writel(~UNLOCK_KEY, DBGOSLAR(cpu_id));
	writel(data[0], DBGDSCR(cpu_id));

	for (i = 0; i < dbg_nr; ++i) {
		writel(data[i * 2 + 1], DBGWVR_BASE(cpu_id) + i * 4);
		writel(data[i * 2 + 2], DBGWCR_BASE(cpu_id) + i * 4);
	}

	for (i = 0; i < MAX_NR_BREAK_POINT; ++i) {
		writel(data[i * 2 + 9], DBGBVR_BASE(cpu_id) + i * 4);
		writel(data[i * 2 + 10], DBGBCR_BASE(cpu_id) + i * 4);
	}
}

void mt_copy_dbg_regs(int to, int from)
{
	unsigned int dbg_nr = hw_watchpoint_nr();
	int i;
	unsigned int cpu_id = to;

	cpu_id &= 0xf;
	writel(UNLOCK_KEY, DBGLAR(cpu_id));
	writel(~UNLOCK_KEY, DBGOSLAR(cpu_id));
	writel(DBGDSCR(0), DBGDSCR(cpu_id));

	for (i = 0; i < dbg_nr; ++i) {
		writel(readl(DBGWVR_BASE(0) + i * 4),
		       DBGWVR_BASE(cpu_id) + i * 4);
		writel(readl(DBGWCR_BASE(0) + i * 4),
		       DBGWCR_BASE(cpu_id) + i * 4);
	}

	for (i = 0; i < MAX_NR_BREAK_POINT; ++i) {
		writel(readl(DBGBVR_BASE(0) + i * 4),
		       DBGBVR_BASE(cpu_id) + i * 4);
		writel(readl(DBGBCR_BASE(0) + i * 4),
		       DBGBCR_BASE(cpu_id) + i * 4);
	}
}

#ifdef CONFIG_SMP
static int __cpuinit
regs_hotplug_callback(struct notifier_block *nfb, unsigned long action,
		      void *hcpu)
{
	int i = 0;
	unsigned int cpu = (unsigned int)hcpu;

	pr_info("[WP]CB cpu=%d\n", cpu);

	switch (action) {
	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
		writel(UNLOCK_KEY, DBGLAR(cpu));
		writel(~UNLOCK_KEY, DBGOSLAR(cpu));
		writel(readl(DBGDSCR(0)) | readl(DBGDSCR(cpu)), DBGDSCR(cpu));

		for (i = 0; i < MAX_NR_WATCH_POINT; ++i) {
			writel(readl(DBGWVR_BASE(0) + i * 4),
			       DBGWVR_BASE(cpu) + i * 4);
			writel(readl(DBGWCR_BASE(0) + i * 4),
			       DBGWCR_BASE(cpu) + i * 4);
		}

		for (i = 0; i < MAX_NR_BREAK_POINT; ++i) {
			writel(readl(DBGBVR_BASE(0) + i * 4),
			       DBGBVR_BASE(cpu) + i * 4);
			writel(readl(DBGBCR_BASE(0) + i * 4),
			       DBGBCR_BASE(cpu) + i * 4);
		}
		break;

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpu_nfb __cpuinitdata = {
	.notifier_call = regs_hotplug_callback
};

static int __init regs_backup(void)
{

	register_cpu_notifier(&cpu_nfb);

	return 0;
}

module_init(regs_backup);
#endif

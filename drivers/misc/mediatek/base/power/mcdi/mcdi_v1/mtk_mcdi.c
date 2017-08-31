/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/cpu.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#include "mtk_mcdi.h"
#include <mtk_mcdi_governor.h>
#include <mtk_mcdi_state.h>

#define MCDI_CPU_OFF        0
#define MCDI_CLUSTER_OFF    0

#define NF_CMD_BUF          128

static DEFINE_SPINLOCK(mcdi_enabled_spin_lock);
static bool mcdi_enabled;
static bool mcdi_paused;

static unsigned long mcdi_cnt_cpu[NF_CPU];
static unsigned long mcdi_cnt_cluster[NF_CLUSTER];

#define log2buf(p, s, fmt, args...) \
	(p += snprintf(p, sizeof(s) - strlen(s), fmt, ##args))

#undef mcdi_log
#define mcdi_log(fmt, args...)	log2buf(p, dbg_buf, fmt, ##args)

/* debugfs */
static char dbg_buf[4096] = { 0 };
static char cmd_buf[512] = { 0 };

/* mcdi_state */
static int _mcdi_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int mcdi_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _mcdi_state_open, inode->i_private);
}

static ssize_t mcdi_state_read(struct file *filp,
			       char __user *userbuf, size_t count, loff_t *f_pos)
{
	unsigned long flags;
	int len = 0;
	int i;
	char *p = dbg_buf;
	bool _mcdi_enabled = false;
	bool _mcdi_paused = false;

	spin_lock_irqsave(&mcdi_enabled_spin_lock, flags);

	_mcdi_enabled = mcdi_enabled;
	_mcdi_paused = mcdi_paused;

	spin_unlock_irqrestore(&mcdi_enabled_spin_lock, flags);

	mcdi_log("mcdi_enabled = %d\n", _mcdi_enabled);
	mcdi_log("mcdi_paused = %d\n", _mcdi_paused);

	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cpu: ");
	for (i = 0; i < NF_CPU; i++)
		mcdi_log("%lu ", mcdi_cnt_cpu[i]);
	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cluster: ");
	for (i = 0; i < NF_CLUSTER; i++)
		mcdi_log("%lu ", mcdi_cnt_cluster[i]);
	mcdi_log("\n");

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcdi_state_write(struct file *filp,
				const char __user *userbuf, size_t count, loff_t *f_pos)
{
	unsigned long flags;
	char cmd[NF_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %x", cmd, &param) == 2) {
		if (!strcmp(cmd, "enable")) {
			spin_lock_irqsave(&mcdi_enabled_spin_lock, flags);
			mcdi_enabled = param;
			spin_unlock_irqrestore(&mcdi_enabled_spin_lock, flags);
		}
		return count;
	}

	return -EINVAL;
}

static const struct file_operations mcdi_state_fops = {
	.open = mcdi_state_open,
	.read = mcdi_state_read,
	.write = mcdi_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* debugfs entry */
static const char mcdi_debugfs_dir_name[] = "mcdi";
static struct dentry *root_entry;

static int mcdi_debugfs_init(void)
{
	/* TODO: check if debugfs_create_file() failed */
	/* Initialize debugfs */
	root_entry = debugfs_create_dir(mcdi_debugfs_dir_name, NULL);
	if (!root_entry) {
		pr_err("Can not create debugfs dir `%s`\n", mcdi_debugfs_dir_name);
		return 1;
	}

	debugfs_create_file("mcdi_state", 0644, root_entry, NULL, &mcdi_state_fops);

	return 0;
}

static void __go_to_wfi(void)
{
	isb();
	mb();
	__asm__ __volatile__("wfi" : : : "memory");
}

void mcdi_cpu_off(int cpu)
{
#if MCDI_CPU_OFF
	state = get_residency_vote_result(cpu);

	mtk_enter_idle_state(MTK_DPIDLE_MODE);
#else
	__go_to_wfi();
#endif
}

void mcdi_cluster_off(int cpu)
{
#if MCDI_CLUSTER_OFF
	int cluster_idx = cpu / 4;

	/* Notify SSPM: cluster can be OFF */
	/* TODO */

	state = get_residency_vote_result(cpu);

	mtk_enter_idle_state(MTK_DPIDLE_MODE);
#else
	__go_to_wfi();
#endif
}

int mcdi_enter(int cpu)
{
	unsigned long flags;
	bool mcdi_working = false;
	int cluster_idx = cpu / 4;
	int state = -1;

	spin_lock_irqsave(&mcdi_enabled_spin_lock, flags);
	mcdi_working = mcdi_enabled && !mcdi_paused;
	spin_unlock_irqrestore(&mcdi_enabled_spin_lock, flags);

	if (!mcdi_working) {
		__go_to_wfi();
		return 0;
	}

	state = mcdi_governor_select(cpu);

	/* TODO */
	if (state == MCDI_STATE_WFI) {
		__go_to_wfi();
	} else if (state == MCDI_STATE_CPU_OFF) {

		mcdi_cpu_off(cpu);

		mcdi_cnt_cpu[cpu]++;
	} else if (state == MCDI_STATE_CLUSTER_OFF) {

		mcdi_cluster_off(cpu);

		mcdi_cnt_cluster[cluster_idx]++;

		mcdi_cnt_cpu[cpu]++;
	} else {
		/* TODO */
	}

	mcdi_governor_reflect(cpu, state);

	return 0;
}

bool mcdi_pause(bool en)
{
	unsigned long flags;
	bool ret = false;

	spin_lock_irqsave(&mcdi_enabled_spin_lock, flags);

	if (mcdi_paused == en) {
		ret = false;
		goto release_lock;
	}

	/* TODO */
	/* Notify SSPM to enable MCDI */
	/* Notify SSPM to disable MCDI */

	mcdi_paused = en;
	ret = true;

release_lock:
	spin_unlock_irqrestore(&mcdi_enabled_spin_lock, flags);

	return ret;
}

/* Disable MCDI during cpu_up/cpu_down period */
static int mcdi_cpu_callback(struct notifier_block *nfb,
				   unsigned long action, void *hcpu)
{
	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
		mcdi_pause(true);
		break;

	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
	case CPU_DOWN_FAILED:
	case CPU_DOWN_FAILED_FROZEN:
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		mcdi_avail_cpu_cluster_update();

		mcdi_pause(false);

		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mcdi_cpu_notifier = {
	.notifier_call = mcdi_cpu_callback,
	.priority   = INT_MAX,
};

static int mcdi_hotplug_cb_init(void)
{
	register_cpu_notifier(&mcdi_cpu_notifier);

	return 0;
}

static int __init mcdi_init(void)
{
	unsigned long flags;

	/* Activate MCDI after SMP */
	pr_warn("mcdi_init\n");

	spin_lock_irqsave(&mcdi_enabled_spin_lock, flags);
	mcdi_paused = false;
	spin_unlock_irqrestore(&mcdi_enabled_spin_lock, flags);

	/* Register CPU up/down callbacks */
	mcdi_hotplug_cb_init();

	/* debugfs init */
	mcdi_debugfs_init();

	/* MCDI governor init */
	mcdi_governor_init();

	return 0;
}

late_initcall(mcdi_init);

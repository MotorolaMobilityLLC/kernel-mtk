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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/tick.h>
#include <linux/uaccess.h>

#include <mtk_cpuidle.h>
#include <mtk_idle.h>
#include <mtk_idle_profile.h>

#include <mtk_mcdi.h>
#include <mtk_mcdi_governor.h>
#include <mtk_mcdi_mbox.h>
#include <mtk_mcdi_reg.h>
#include <mtk_mcdi_state.h>

#include <sspm_mbox.h>

#include <trace/events/mtk_events.h>
/* #define USING_TICK_BROADCAST */

#define MCDI_CPU_OFF        1
#define MCDI_CLUSTER_OFF    1

#define NF_CMD_BUF          128
#define LOG_BUF_LEN         1024

#define MCDI_SYSRAM_SIZE    0x800
#define MCDI_SYSRAM_NF_WORD (MCDI_SYSRAM_SIZE / 4)
#define SYSRAM_DUMP_RANGE   50

#define MCDI_DEBUG_INFO_MAGIC_NUM           0x1eef9487
#define MCDI_DEBUG_INFO_NON_REPLACE_OFFSET  0x0008

static unsigned long mcdi_cnt_cpu[NF_CPU];
static unsigned long mcdi_cnt_cluster[NF_CLUSTER];

void __iomem *mcdi_sysram_base;

static unsigned long mcdi_cnt_cpu_last[NF_CPU];
static unsigned long mcdi_cnt_cluster_last[NF_CLUSTER];

static unsigned long any_core_cpu_cond_info_last[NF_ANY_CORE_CPU_COND_INFO];

static const char *any_core_cpu_cond_name[NF_ANY_CORE_CPU_COND_INFO] = {
	"pause",
	"multi core",
	"residency",
	"last core"
};

static unsigned long long mcdi_heart_beat_log_prev;
static DEFINE_SPINLOCK(mcdi_heart_beat_spin_lock);

static unsigned int mcdi_heart_beat_log_dump_thd = 5000;          /* 5 sec */

#define log2buf(p, s, fmt, args...) \
	(p += snprintf(p, sizeof(s) - strlen(s), fmt, ##args))

#undef mcdi_log
#define mcdi_log(fmt, args...)	log2buf(p, dbg_buf, fmt, ##args)

struct mtk_mcdi_buf {
	char buf[LOG_BUF_LEN];
	char *p_idx;
};

#define reset_mcdi_buf(mcdi) ((mcdi).p_idx = (mcdi).buf)
#define get_mcdi_buf(mcdi)   ((mcdi).buf)
#define mcdi_buf_append(mcdi, fmt, args...) \
	((mcdi).p_idx += snprintf((mcdi).p_idx, LOG_BUF_LEN - strlen((mcdi).buf), fmt, ##args))

static inline long int get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

static int cluster_idx_map[NF_CPU] = {
	0,
	0,
	0,
	0,
	1,
	1,
	1,
	1
};

int cluster_idx_get(int cpu)
{
	if (!(cpu >= 0 && cpu < NF_CPU)) {
		WARN_ON(!(cpu >= 0 && cpu < NF_CPU));

		return 0;
	}

	return cluster_idx_map[cpu];
}

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
	int len = 0;
	int i;
	char *p = dbg_buf;
	unsigned long any_core_cpu_cond_info[NF_ANY_CORE_CPU_COND_INFO];

	struct mcdi_feature_status feature_stat;

	get_mcdi_feature_status(&feature_stat);

	mcdi_log("Feature:\n");
	mcdi_log("\tenable = %d\n", feature_stat.enable);
	mcdi_log("\tpause = %d\n", feature_stat.pause);
	mcdi_log("\tmax s_state = %d\n", feature_stat.s_state);
	mcdi_log("\tcluster_off = %d\n", feature_stat.cluster_off);
	mcdi_log("\tany_core = %d\n", feature_stat.any_core);

	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cpu: ");
	for (i = 0; i < NF_CPU; i++)
		mcdi_log("%lu ", mcdi_cnt_cpu[i]);
	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cluster: ");
	for (i = 0; i < NF_CLUSTER; i++) {
		mcdi_cnt_cluster[i] = mcdi_mbox_read(MCDI_MBOX_CLUSTER_0_CNT + i);
		mcdi_log("%lu ", mcdi_cnt_cluster[i]);
	}
	mcdi_log("\n");

	any_core_cpu_cond_get(any_core_cpu_cond_info);

	for (i = 0; i < NF_ANY_CORE_CPU_COND_INFO; i++) {
		mcdi_log("%s = %lu\n",
			any_core_cpu_cond_name[i],
			any_core_cpu_cond_info[i]
		);
	}

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcdi_state_write(struct file *filp,
				const char __user *userbuf, size_t count, loff_t *f_pos)
{
	char cmd[NF_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %x", cmd, &param) == 2) {
		if (!strcmp(cmd, "enable"))
			set_mcdi_enable_status(param);
		else if (!strcmp(cmd, "s_state"))
			set_mcdi_s_state(param);
		return count;
	}

	return -EINVAL;
}

/* mcdi_debug */
static int sysram_dump_start_idx;

static int _mcdi_debug_open(struct seq_file *s, void *data)
{
	return 0;
}

static int mcdi_debug_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _mcdi_debug_open, inode->i_private);
}

static ssize_t mcdi_debug_read(struct file *filp,
			       char __user *userbuf, size_t count, loff_t *f_pos)
{
	int len = 0;
	int i;
	char *p = dbg_buf;
	int end_idx = 0;

	end_idx = (sysram_dump_start_idx + (SYSRAM_DUMP_RANGE >= MCDI_SYSRAM_NF_WORD)) ?
				MCDI_SYSRAM_NF_WORD :
				(sysram_dump_start_idx + SYSRAM_DUMP_RANGE);

	mcdi_log("\nsysram: 0x%x\n", sysram_dump_start_idx);
	for (i = sysram_dump_start_idx; i < end_idx; i++)
		mcdi_log("[0x%04x] = %08x\n", i * 4, mcdi_read(mcdi_sysram_base + 4 * i));

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcdi_debug_write(struct file *filp,
				const char __user *userbuf, size_t count, loff_t *f_pos)
{
	char cmd[NF_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %x", cmd, &param) == 2) {
		if (!strcmp(cmd, "sysram")) {
			if (param >= 0 && param < MCDI_SYSRAM_NF_WORD)
				sysram_dump_start_idx = param;
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

static const struct file_operations mcdi_debug_fops = {
	.open = mcdi_debug_open,
	.read = mcdi_debug_read,
	.write = mcdi_debug_write,
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
	debugfs_create_file("mcdi_debug", 0644, root_entry, NULL, &mcdi_debug_fops);

	return 0;
}

static void __go_to_wfi(void)
{
	isb();
	mb();
	__asm__ __volatile__("wfi" : : : "memory");
}

unsigned int mcdi_mbox_read(int id)
{
	unsigned int val = 0;

	sspm_mbox_read(MCDI_MBOX, id, &val, 1);

	return val;
}

void mcdi_mbox_write(int id, unsigned int val)
{
	sspm_mbox_write(MCDI_MBOX, id, (void *)&val, 1);
}

void mcdi_sysram_init(void)
{
	if (!mcdi_sysram_base)
		return;

	memset_io((void __iomem *)(mcdi_sysram_base + MCDI_DEBUG_INFO_NON_REPLACE_OFFSET),
				0,
				MCDI_SYSRAM_SIZE - MCDI_DEBUG_INFO_NON_REPLACE_OFFSET);
}

void mcdi_cpu_off(int cpu)
{
#if MCDI_CPU_OFF
	int state = 0;

	state = get_residency_latency_result(cpu);

	switch (state) {
	case MCDI_STATE_CPU_OFF:

#ifdef USING_TICK_BROADCAST
		tick_broadcast_enter();
#endif

		mtk_enter_idle_state(MTK_MCDI_CPU_MODE);

#ifdef USING_TICK_BROADCAST
		tick_broadcast_exit();
#endif

		break;
	case MCDI_STATE_CLUSTER_OFF:
	case MCDI_STATE_SODI:
	case MCDI_STATE_DPIDLE:
	case MCDI_STATE_SODI3:

#ifdef USING_TICK_BROADCAST
		tick_broadcast_enter();
#endif

		mtk_enter_idle_state(MTK_MCDI_CLUSTER_MODE);

#ifdef USING_TICK_BROADCAST
		tick_broadcast_exit();
#endif

		break;
	default:
		/* should NOT happened */
		__go_to_wfi();

		break;
	}
#else
	__go_to_wfi();
#endif
}

void mcdi_cluster_off(int cpu)
{
#if MCDI_CLUSTER_OFF
	int cluster_idx = cluster_idx_get(cpu);

	/* Notify SSPM: cluster can be OFF */
	mcdi_mbox_write(MCDI_MBOX_CLUSTER_0_CAN_POWER_OFF + cluster_idx, 1);

	mtk_enter_idle_state(MTK_MCDI_CLUSTER_MODE);

#elif MCDI_CPU_OFF
	mcdi_cpu_off(cpu);
#else
	__go_to_wfi();
#endif
}

void mcdi_heart_beat_log_dump(void)
{
	static struct mtk_mcdi_buf buf;
	int i;
	unsigned long long mcdi_heart_beat_log_curr = 0;
	unsigned long flags;
	bool dump_log = false;
	unsigned long mcdi_cnt;
	unsigned long any_core_info = 0;
	unsigned long any_core_cpu_cond_info[NF_ANY_CORE_CPU_COND_INFO];
	unsigned int cpu_mask = 0;
	unsigned int cluster_mask = 0;
	struct mcdi_feature_status feature_stat;

	spin_lock_irqsave(&mcdi_heart_beat_spin_lock, flags);

	mcdi_heart_beat_log_curr = get_current_time_ms();

	if (mcdi_heart_beat_log_prev == 0)
		mcdi_heart_beat_log_prev = mcdi_heart_beat_log_curr;

	if ((mcdi_heart_beat_log_curr - mcdi_heart_beat_log_prev) > mcdi_heart_beat_log_dump_thd) {
		dump_log = true;
		mcdi_heart_beat_log_prev = mcdi_heart_beat_log_curr;
	}

	spin_unlock_irqrestore(&mcdi_heart_beat_spin_lock, flags);

	if (!dump_log)
		return;

	reset_mcdi_buf(buf);

	mcdi_buf_append(buf, "mcdi cpu: ");

	for (i = 0; i < NF_CPU; i++) {
		mcdi_cnt = mcdi_cnt_cpu[i] - mcdi_cnt_cpu_last[i];
		mcdi_buf_append(buf, "%lu, ", mcdi_cnt);
		mcdi_cnt_cpu_last[i] = mcdi_cnt_cpu[i];
	}

	mcdi_buf_append(buf, "cluster : ");

	for (i = 0; i < NF_CLUSTER; i++) {
		mcdi_cnt_cluster[i] = mcdi_mbox_read(MCDI_MBOX_CLUSTER_0_CNT + i);

		mcdi_cnt = mcdi_cnt_cluster[i] - mcdi_cnt_cluster_last[i];
		mcdi_buf_append(buf, "%lu, ", mcdi_cnt);
		mcdi_cnt_cluster_last[i] = mcdi_cnt_cluster[i];
	}

	any_core_cpu_cond_get(any_core_cpu_cond_info);

	for (i = 0; i < NF_ANY_CORE_CPU_COND_INFO; i++) {
		any_core_info = any_core_cpu_cond_info[i] - any_core_cpu_cond_info_last[i];
		mcdi_buf_append(buf, "%s = %lu, ", any_core_cpu_cond_name[i], any_core_info);
		any_core_cpu_cond_info_last[i] = any_core_cpu_cond_info[i];
	}

	get_mcdi_avail_mask(&cpu_mask, &cluster_mask);

	mcdi_buf_append(buf, "avail cpu = %04x, cluster = %04x", cpu_mask, cluster_mask);

	get_mcdi_feature_status(&feature_stat);

	mcdi_buf_append(buf, ", enabled = %d, max_s_state = %d",
						feature_stat.enable,
						feature_stat.s_state);

	pr_warn("%s\n", get_mcdi_buf(buf));
}

int mcdi_enter(int cpu)
{
	int cluster_idx = cluster_idx_get(cpu);
	int state = -1;

	mtk_idle_dump_cnt_in_interval();
	mcdi_heart_beat_log_dump();

	state = mcdi_governor_select(cpu, cluster_idx);

	switch (state) {
	case MCDI_STATE_WFI:
		__go_to_wfi();

		break;
	case MCDI_STATE_CPU_OFF:

		trace_mcdi(cpu, 1);

		mcdi_cpu_off(cpu);

		trace_mcdi(cpu, 0);

		mcdi_cnt_cpu[cpu]++;

		break;
	case MCDI_STATE_CLUSTER_OFF:

		trace_mcdi(cpu, 1);

		mcdi_cluster_off(cpu);

		trace_mcdi(cpu, 0);

		mcdi_cnt_cpu[cpu]++;

		break;
	case MCDI_STATE_SODI:
		soidle_enter(cpu);

		break;
	case MCDI_STATE_DPIDLE:
		dpidle_enter(cpu);

		break;
	case MCDI_STATE_SODI3:
		soidle3_enter(cpu);

		break;
	}

	mcdi_governor_reflect(cpu, state);

	return 0;
}

void wakeup_all_cpu(void)
{
	int cpu = 0;

	for (cpu = 0; cpu < NF_CPU; cpu++) {
		if (cpu_online(cpu))
			smp_send_reschedule(cpu);
	}
}

void wait_until_all_cpu_powered_on(void)
{
	while (!(mcdi_mbox_read(MCDI_MBOX_CPU_CLUSTER_PWR_STAT) == 0x0))
		;
}

bool mcdi_pause(bool paused)
{
	struct mcdi_feature_status feature_stat;

	get_mcdi_feature_status(&feature_stat);

	if (!feature_stat.enable)
		return true;

	if (paused) {
		mcdi_state_pause(true);

		wakeup_all_cpu();

		wait_until_all_cpu_powered_on();
	} else {
		mcdi_state_pause(false);
	}

	return true;
}

bool mcdi_task_pause(bool paused)
{
	bool ret = false;

	/* TODO */
#if 1
	if (paused) {
		/* Notify SSPM to disable MCDI */
		mcdi_mbox_write(MCDI_MBOX_PAUSE_ACTION, 1);

		/* Polling until MCDI Task stopped */
		while (!(mcdi_mbox_read(MCDI_MBOX_PAUSE_ACK) == 1))
			;
	} else {
		/* Notify SSPM to enable MCDI */
		mcdi_mbox_write(MCDI_MBOX_PAUSE_ACTION, 0);

		/* Polling until MCDI Task resume */
		while (!(mcdi_mbox_read(MCDI_MBOX_PAUSE_ACK) == 0))
			;
	}

	ret = true;

#endif
	return ret;
}

void update_avail_cpu_mask_to_mcdi_controller(unsigned int cpu_mask)
{
	mcdi_mbox_write(MCDI_MBOX_AVAIL_CPU_MASK, cpu_mask);
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

static const char mcdi_node_name[] = "mediatek,mt6763-mcdi";

static void mcdi_of_init(void)
{
	struct device_node *node = NULL;

	/* MCDI sysram base */
	node = of_find_compatible_node(NULL, NULL, mcdi_node_name);

	if (!node)
		pr_err("node '%s' not found!\n", mcdi_node_name);

	mcdi_sysram_base = of_iomap(node, 0);

	if (!mcdi_sysram_base)
		pr_err("node '%s' can not iomap!\n", mcdi_node_name);

	pr_info("mcdi_sysram_base = %p\n", mcdi_sysram_base);
}

static int __init mcdi_init(void)
{
	/* Activate MCDI after SMP */
	pr_warn("mcdi_init\n");

	/* Register CPU up/down callbacks */
	mcdi_hotplug_cb_init();

	/* debugfs init */
	mcdi_debugfs_init();

	/* MCDI governor init */
	mcdi_governor_init();

	/* of init */
	mcdi_of_init();

	/* MCDI sysram space init */
	mcdi_sysram_init();

	return 0;
}

late_initcall(mcdi_init);

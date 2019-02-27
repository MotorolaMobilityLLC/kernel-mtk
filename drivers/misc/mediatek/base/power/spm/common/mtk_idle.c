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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h> /* copy_from_user / copy_to_user */

#include <trace/events/mtk_idle_event.h> /* trace header */

#include <mtk_mcdi_governor.h> /* idle_refcnt_inc/dec */

/* add/remove_cpu_to/from_perfer_schedule_domain */
#include <linux/irqchip/mtk-gic-extend.h>

#include <mtk_idle.h>
#include <mtk_idle_internal.h>
#include <mtk_spm_internal.h>


/* [ByChip] Internal weak functions: implemented in mtk_idle_cond_check.c */
void __attribute__((weak)) mtk_idle_cg_monitor(int sel) {}

/* External weak functions: implemented in mcdi driver */
void __attribute__((weak)) idle_refcnt_inc(void) {}
void __attribute__((weak)) idle_refcnt_dec(void) {}

/* Local variables */
static unsigned long rg_cnt[NR_CPUS] = {0};
/* External variables */
//extern unsigned long slp_dp_cnt[NR_CPUS];	/* FIXME: sleep dpidle count */
static unsigned long slp_dp_cnt[NR_CPUS];	/* FIXM: To be removed */

static void go_to_wfi(void)
{
	isb();
	mb();	/* memory barrier */
	__asm__ __volatile__("wfi" : : : "memory");
}

int rgidle_enter(int cpu)
{
#if 0 //FIXME
	remove_cpu_from_prefer_schedule_domain(cpu);
#endif
	mtk_idle_ratio_calc_start(IDLE_TYPE_RG, cpu);

	idle_refcnt_inc();

	#if MTK_IDLE_TRACE_TAG_ENABLE
	trace_rgidle_rcuidle(cpu, 1);
	#endif

	go_to_wfi();

	#if MTK_IDLE_TRACE_TAG_ENABLE
	trace_rgidle_rcuidle(cpu, 0);
	#endif

	rg_cnt[cpu]++;

	idle_refcnt_dec();

	mtk_idle_ratio_calc_stop(IDLE_TYPE_RG, cpu);
#if 0 //FIXME
	add_cpu_to_prefer_schedule_domain(cpu);
#endif
	return CPUIDLE_STATE_DP;
}
EXPORT_SYMBOL(rgidle_enter);


static int _idle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int idle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _idle_state_open, inode->i_private);
}

#define logbufsz	4096
static char logbuf[logbufsz] = { 0 };

static ssize_t idle_state_read(
	struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i;
	char *p = logbuf;
	size_t sz = logbufsz;

	#undef log
	#define log(fmt, args...) \
	do { \
		int l = scnprintf(p, sz, fmt, ##args); \
		p += l; \
		sz -= l; \
	} while (0)

	log("*************** idle state ***********************\n");
	for_each_possible_cpu(i) {
		log("cpu%d: slp_dp=%lu, dp=%lu, so3=%lu, so=%lu, rg=%lu\n"
			, i, slp_dp_cnt[i], dp_cnt[i], so3_cnt[i]
			, so_cnt[i], rg_cnt[i]);
	}
	log("\n");

	log("*************** variable dump ********************\n");
	log("feature enable: dp=%d, so3=%d, so=%d\n",
		mtk_dpidle_enabled() ? 1 : 0,
		mtk_sodi3_enabled() ? 1 : 0,
		mtk_sodi_enabled() ? 1 : 0);

	log("idle_ratio_profile=%d\n", mtk_idle_get_ratio_status() ? 1 : 0);
	log("idle_latency_profile=%d\n"
			, mtk_idle_latency_profile_is_on() ? 1 : 0);
	log("twam_handler:%s (clk:%s)\n",
		(mtk_idle_get_twam()->running) ? "on" : "off",
		(mtk_idle_get_twam()->speed_mode) ? "speed" : "normal");
	log("\n");

	#define MTK_DEBUGFS_IDLE	"/d/cpuidle/idle_state"
	#define MTK_DEBUGFS_DPIDLE	"/d/cpuidle/dpidle_state"
	#define MTK_DEBUGFS_SODI3	"/d/cpuidle/soidle3_state"
	#define MTK_DEBUGFS_SODI	"/d/cpuidle/soidle_state"

	log("*************** idle command help ****************\n");
	log("status help:          cat %s\n", MTK_DEBUGFS_IDLE);
	log("dpidle help:          cat %s\n", MTK_DEBUGFS_DPIDLE);
	log("sodi help:            cat %s\n", MTK_DEBUGFS_SODI);
	log("sodi3 help:           cat %sn", MTK_DEBUGFS_SODI3);
	log("idle ratio profile:   echo ratio 1/0 > %s\n", MTK_DEBUGFS_IDLE);
	log("idle latency profile: echo latency 1/0 > %s\n", MTK_DEBUGFS_IDLE);
	log("cgmon off/dp/so3/so:  echo cgmon 0/1/2/3 > %s\n"
		, MTK_DEBUGFS_IDLE);
	log("\n");

	return simple_read_from_buffer(
		userbuf, count, f_pos, logbuf, p - logbuf);
}

#define cmdbufsz	256
static char cmdbuf[cmdbufsz] = { 0 };

static ssize_t idle_state_write(struct file *filp, const char __user *userbuf
		, size_t count, loff_t *f_pos)
{
	char cmd[128];
	int parm;

	count = min(count, sizeof(cmdbuf) - 1);

	if (copy_from_user(cmdbuf, userbuf, count))
		return -EFAULT;

	cmdbuf[count] = '\0';

	if (sscanf(cmdbuf, "%127s %x", cmd, &parm) == 2) {
		if (!strcmp(cmd, "ratio")) {
			if (parm == 1)
				mtk_idle_enable_ratio_calc();
			else
				mtk_idle_disable_ratio_calc();
		} else if (!strcmp(cmd, "latency")) {
			mtk_idle_latency_profile_enable(parm ? true : false);
		} else if (!strcmp(cmd, "spmtwam_clk")) {
			mtk_idle_get_twam()->speed_mode = parm;
		} else if (!strcmp(cmd, "spmtwam_sel")) {
			mtk_idle_get_twam()->sel = parm;
		} else if (!strcmp(cmd, "spmtwam")) {
			idle_info("spmtwam_event = %d\n", parm);
			if (parm >= 0)
				mtk_idle_twam_enable(parm);
			else
				mtk_idle_twam_disable();
		} else if (!strcmp(cmd, "cgmon")) {
			mtk_idle_cg_monitor(parm == 1 ? IDLE_TYPE_DP :
				parm == 2 ? IDLE_TYPE_SO3 :
				parm == 3 ? IDLE_TYPE_SO : -1);
		}
		return count;
	} else if ((!kstrtoint(cmdbuf, 10, &parm)) == 1) {
		return count;
	}

	return -EINVAL;
}

static const struct file_operations idle_state_fops = {
	.open = idle_state_open,
	.read = idle_state_read,
	.write = idle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void mtk_idle_init(struct dentry *root_entry)
{
	debugfs_create_file(
		"idle_state", 0644, root_entry, NULL, &idle_state_fops);

	mtk_idle_block_setting(IDLE_TYPE_RG, rg_cnt, NULL);
}

void __init mtk_cpuidle_framework_init(void)
{
	struct dentry *root_entry;

	root_entry = debugfs_create_dir("cpuidle", NULL);
	if (!root_entry)
		return;

	mtk_idle_init(root_entry);
	mtk_dpidle_init(root_entry);
	mtk_sodi_init(root_entry);
	mtk_sodi3_init(root_entry);

	spm_resource_req_init();
}
EXPORT_SYMBOL(mtk_cpuidle_framework_init);

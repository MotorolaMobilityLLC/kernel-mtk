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

#include <mtk_idle.h>
#include <mtk_idle_internal.h>
#include <mtk_spm_internal.h> /* mtk_idle_cond_check */


static bool sodi_feature_enable = MTK_IDLE_FEATURE_ENABLE_SODI;
static bool sodi_bypass_idle_cond;
static bool sodi_bypass_dis_check;
static bool sodi_disp_is_ready;
static bool sodi_mempll_pwr_mode;
static unsigned int sodi_flag = MTK_IDLE_LOG_REDUCE;

unsigned long so_cnt[NR_CPUS] = {0};
static unsigned long so_block_cnt[NR_REASONS] = {0};

/* [ByChip] Internal weak functions: implemented in mtk_idle_cond_check.c */
void __attribute__((weak)) mtk_idle_cg_monitor(int sel) {}
bool __attribute__((weak)) mtk_idle_cond_check(int idle_type) {return false; }
void __attribute__((weak)) mtk_idle_cond_update_mask(
	int idle_type, unsigned int reg, unsigned int mask) {}
int __attribute__((weak)) mtk_idle_cond_append_info(
	bool short_log, int idle_type, char *logptr, unsigned int logsize);


bool mtk_sodi_enabled(void)
{
	return sodi_feature_enable;
}

static unsigned long get_uptime(void)
{
	struct timespec uptime;

	get_monotonic_boottime(&uptime);
	return (unsigned long)uptime.tv_sec;
}

/* for display use, abandoned 'spm_enable_sodi' */
void mtk_sodi_disp_ready(bool enable)
{
	sodi_disp_is_ready = enable;
}

bool mtk_sodi_disp_is_ready(void)
{
	return sodi_disp_is_ready;
}

/* for display use, abandoned 'spm_sodi_mempll_pwr_mode' */
void mtk_sodi_disp_mempll_pwr_mode(bool pwr_mode)
{
	sodi_mempll_pwr_mode = pwr_mode;
}
static int sodi_uptime_block_count;
bool sodi_can_enter(int reason)
{
	/* sodi is disabled */
	if (!sodi_feature_enable)
		return false;

	if (reason < NR_REASONS)
		goto out;

	/* check cg condition for sodi */
	if (!sodi_bypass_idle_cond && !mtk_idle_cond_check(IDLE_TYPE_SO)) {
		reason = BY_CLK;
		goto out;
	}

	/* display driver is ready ? */
	if (!sodi_bypass_dis_check && !sodi_disp_is_ready) {
		reason = BY_DIS;
		goto out;
	}

	/* check current uptime > 20 ? */
	if (sodi_uptime_block_count != -1) {
		if (get_uptime() <= 20) {
			sodi_uptime_block_count++;
			reason = BY_BOOT;
			goto out;
		} else {
			idle_warn("SODI: blocked by uptime, count = %d\n",
				sodi_uptime_block_count);
			sodi_uptime_block_count = -1;
		}
	}

	reason = NR_REASONS;

out:
	return mtk_idle_select_state(IDLE_TYPE_SO, reason);
}

int soidle_enter(int cpu)
{
	unsigned int op_cond = 0;

	/* Display driver indicates sodi dram cg/pdn mode */
	if (sodi_mempll_pwr_mode)
		op_cond |= MTK_IDLE_OPT_SODI_CG_MODE;   /* CG mode */
	else
		op_cond &= ~MTK_IDLE_OPT_SODI_CG_MODE;  /* PDN mode */

	lockdep_off();
	mtk_idle_ratio_calc_start(IDLE_TYPE_SO, cpu);
	mtk_idle_enter(IDLE_TYPE_SO, cpu, op_cond, sodi_flag);
	mtk_idle_ratio_calc_stop(IDLE_TYPE_SO, cpu);
	lockdep_on();

	so_cnt[cpu]++;

	return CPUIDLE_STATE_SO;
}
EXPORT_SYMBOL(soidle_enter);


static int _soidle_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int soidle_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _soidle_state_open, inode->i_private);
}


#define logbufsz	4096
static char logbuf[logbufsz] = { 0 };

static ssize_t soidle_state_read(
	struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i;
	char *p = logbuf;

	#undef log
	#define log(fmt, args...) ({\
		p += scnprintf(p, logbufsz - strlen(logbuf), fmt, ##args); p; })

	log("*************** sodi state ***********************\n");
	for (i = 0; i < NR_REASONS; i++) {
		if (so_block_cnt[i] > 0)
			log("[%d] block_cnt[%s]=%lu\n"
				, i, mtk_idle_block_reason_name(i),
				so_block_cnt[i]);
	}
	log("\n");

	p += mtk_idle_cond_append_info(
		false, IDLE_TYPE_SO, p, logbufsz - strlen(logbuf));
	log("\n");

	log("*************** variable dump ********************\n");
	log("enable=%d bypass=%d bypass_dis=%d\n",
		sodi_feature_enable ? 1 : 0,
		sodi_bypass_idle_cond ? 1 : 0,
		sodi_bypass_dis_check ? 1 : 0);
	log("log_flag=0x%x\n", sodi_flag);
	log("\n");

	log("*************** sodi command *********************\n");
	log("sodi state:    cat /d/cpuidle/soidle_state\n");
	log("sodi on/off:   echo 1/0 > /d/cpuidle/soidle_state\n");
	log("bypass cg/pll: echo bypass 1/0 > /d/cpuidle/soidle_state\n");
	log("bypass dis:    echo bypass_dis 1/0 > /d/cpuidle/soidle_state\n");
	log("cg monitor:    echo cgmon 1/0 > /d/cpuidle/soidle_state\n");
	log("set log_flag:  echo log [hex] > /d/cpuidle/soidle_state\n");
	log("               [0] reduced [1] spm res usage [2] disable\n");
	log("\n");

	return simple_read_from_buffer(
		userbuf, count, f_pos, logbuf, p - logbuf);
}

#define cmdbufsz	256
static char cmdbuf[cmdbufsz] = { 0 };

static ssize_t soidle_state_write(
		struct file *filp, const char __user *userbuf
		, size_t count, loff_t *f_pos)
{
	char cmd[128];
	int parm, parm1;

	count = min(count, sizeof(cmdbuf) - 1);

	if (copy_from_user(cmdbuf, userbuf, count))
		return -EFAULT;

	cmdbuf[count] = '\0';

	if (sscanf(cmdbuf, "%127s %x %x", cmd, &parm, &parm1) == 3) {
		if (!strcmp(cmd, "mask"))
			mtk_idle_cond_update_mask(IDLE_TYPE_SO, parm, parm1);
	} else if (sscanf(cmdbuf, "%127s %x", cmd, &parm) == 2) {
		if (!strcmp(cmd, "soidle"))
			sodi_feature_enable = !!parm;
		else if (!strcmp(cmd, "bypass"))
			sodi_bypass_idle_cond = !!parm;
		else if (!strcmp(cmd, "bypass_dis"))
			sodi_bypass_dis_check = !!parm;
		else if (!strcmp(cmd, "cgmon"))
			mtk_idle_cg_monitor(parm ? IDLE_TYPE_SO : -1);
		else if (!strcmp(cmd, "log"))
			sodi_flag = parm;
		return count;
	} else if ((!kstrtoint(cmdbuf, 10, &parm)) == 1) {
		sodi_feature_enable = !!parm;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations soidle_state_fops = {
	.open = soidle_state_open,
	.read = soidle_state_read,
	.write = soidle_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

void mtk_sodi_init(struct dentry *root_entry)
{
	sodi_uptime_block_count = 0;
	sodi_bypass_idle_cond = false;
	sodi_bypass_dis_check = false;
	sodi_disp_is_ready = false;

	debugfs_create_file(
		"soidle_state", 0644, root_entry, NULL, &soidle_state_fops);

	mtk_idle_block_setting(IDLE_TYPE_SO, so_cnt, so_block_cnt);
}

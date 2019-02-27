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
#include <ddp_pwm.h> /* bool disp_pwm_is_osc(void); */

#include <mtk_idle.h>
#include <mtk_idle_internal.h>
#include <mtk_spm_internal.h> /* mtk_idle_cond_check */


static bool sodi3_feature_enable = MTK_IDLE_FEATURE_ENABLE_SODI3;
static bool sodi3_bypass_idle_cond;
static bool sodi3_bypass_dis_check;
static bool sodi3_bypass_pwm_check;
static bool sodi3_disp_is_ready;
static bool sodi3_force_vcore_lp_mode;
static unsigned int sodi3_flag = MTK_IDLE_LOG_REDUCE;

unsigned long so3_cnt[NR_CPUS] = {0};
static unsigned long so3_block_cnt[NR_REASONS] = {0};


/* [ByChip] Internal weak functions: implemented in mtk_idle_cond_check.c */
void __attribute__((weak)) mtk_idle_cg_monitor(int sel) {}
bool __attribute__((weak)) mtk_idle_cond_check(int idle_type) {return false; }
void __attribute__((weak)) mtk_idle_cond_update_mask(
	int idle_type, unsigned int reg, unsigned int mask) {}
int __attribute__((weak)) mtk_idle_cond_append_info(
	bool short_log, int idle_type, char *logptr, unsigned int logsize);

/* External weak function: implemented in disp driver */
bool __attribute__((weak)) disp_pwm_is_osc(void) {return false; }


bool mtk_sodi3_enabled(void)
{
	return sodi3_feature_enable;
}

/* for display use, abandoned 'spm_enable_sodi3' */
void mtk_sodi3_disp_ready(bool enable)
{
	sodi3_disp_is_ready = enable;
}

static unsigned long get_uptime(void)
{
	struct timespec uptime;

	get_monotonic_boottime(&uptime);
	return (unsigned long)uptime.tv_sec;
}

static int sodi3_uptime_block_count;
bool sodi3_can_enter(int reason)
{
	/* sodi3 is disabled */
	if (!sodi3_feature_enable)
		return false;

	if (reason < NR_REASONS)
		goto out;

	/* check cg condition for sodi3 */
	if (!sodi3_bypass_idle_cond && !mtk_idle_cond_check(IDLE_TYPE_SO3)) {
		reason = BY_CLK;
		goto out;
	}

	/* display driver is ready ? */
	if (!sodi3_bypass_dis_check &&
		!(sodi3_disp_is_ready && mtk_sodi_disp_is_ready())) {
		reason = BY_DIS;
		goto out;
	}

	/* pwm clock uses ulposc ? */
	if (!disp_pwm_is_osc() && !sodi3_bypass_pwm_check) {
		reason = BY_PWM;
		goto out;
	}

	/* check current uptime > 30 ? */
	if (sodi3_uptime_block_count != -1) {
		if (get_uptime() <= 30) {
			sodi3_uptime_block_count++;
			reason = BY_BOOT;
			goto out;
		} else {
			pr_notice("Power/swap SODI3: blocked by uptime, count = %d\n",
				sodi3_uptime_block_count);
			sodi3_uptime_block_count = -1;
		}
	}

	reason = NR_REASONS;

out:
	return mtk_idle_select_state(IDLE_TYPE_SO3, reason);
}




int soidle3_enter(int cpu)
{
	unsigned int op_cond = 0;

	if (sodi3_force_vcore_lp_mode)
		op_cond |= MTK_IDLE_OPT_VCORE_LP_MODE;

	lockdep_off();
	mtk_idle_ratio_calc_start(IDLE_TYPE_SO3, cpu);
	mtk_idle_enter(IDLE_TYPE_SO3, cpu, op_cond, sodi3_flag);
	mtk_idle_ratio_calc_stop(IDLE_TYPE_SO3, cpu);
	lockdep_on();

	so3_cnt[cpu]++;

	return CPUIDLE_STATE_SO3;
}
EXPORT_SYMBOL(soidle3_enter);



static int _soidle3_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int soidle3_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _soidle3_state_open, inode->i_private);
}

	#define log2buf(p, s, fmt, args...) \
		(p += scnprintf(p, sizeof(s) - strlen(s), fmt, ##args))
	#define log(fmt, args...)	log2buf(p, logbuf, fmt, ##args)



#define logbufsz	4096
static char logbuf[logbufsz] = { 0 };

static ssize_t soidle3_state_read(
	struct file *filp, char __user *userbuf, size_t count, loff_t *f_pos)
{
	int i;
	char *p = logbuf;

	#undef log
	#define log(fmt, args...) ({\
		p += scnprintf(p, logbufsz - strlen(logbuf), fmt, ##args); p; })

	log("*************** sodi3 state **********************\n");
	for (i = 0; i < NR_REASONS; i++) {
		if (so3_block_cnt[i] > 0)
			log("[%d] block_cnt[%s]=%lu\n", i
				, mtk_idle_block_reason_name(i)
				, so3_block_cnt[i]);
	}
	log("\n");

	p += mtk_idle_cond_append_info(
		false, IDLE_TYPE_SO3, p, logbufsz - strlen(logbuf));
	log("\n");


	log("*************** variable dump ********************\n");
	log("enable=%d bypass=%d bypass_dis=%d\n"
		, sodi3_feature_enable ? 1 : 0
		, sodi3_bypass_idle_cond ? 1 : 0
		, sodi3_bypass_dis_check ? 1 : 0);

	log("bypass_pwm=%d force_vcore_lp_mode=%d\n"
			, sodi3_bypass_pwm_check ? 1 : 0
			, sodi3_force_vcore_lp_mode ? 1 : 0);

	log("log_flag=0x%x\n", sodi3_flag);
	log("\n");

	log("*************** sodi3 command ********************\n");
	log("sodi3 state:   cat /d/cpuidle/soidle3_state\n");
	log("sodi3 on/off:  echo 1/0 > /d/cpuidle/soidle3_state\n");
	log("bypass cg/pll: echo bypass 1/0 > /d/cpuidle/soidle3_state\n");
	log("bypass dis:    echo bypass_dis 1/0 > /d/cpuidle/soidle3_state\n");
	log("bypass pwm:    echo bypass_dis 1/0 > /d/cpuidle/soidle3_state\n");
	log("cg monitor:    echo cgmon 1/0 > /d/cpuidle/soidle3_state\n");
	log("set log_flag:  echo log [hex_value] > /d/cpuidle/soidle3_state\n");
	log("               [0] reduced [1] spm res usage [2] disable\n");
	log("\n");

	return simple_read_from_buffer(
		userbuf, count, f_pos, logbuf, p - logbuf);
}

#define cmdbufsz	256
static char cmdbuf[cmdbufsz] = { 0 };

static ssize_t soidle3_state_write(
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
			mtk_idle_cond_update_mask(IDLE_TYPE_SO3, parm, parm1);
	} else if (sscanf(cmdbuf, "%127s %x", cmd, &parm) == 2) {
		if (!strcmp(cmd, "soidle3"))
			sodi3_feature_enable = !!parm;
		else if (!strcmp(cmd, "bypass"))
			sodi3_bypass_idle_cond = !!parm;
		else if (!strcmp(cmd, "bypass_dis"))
			sodi3_bypass_dis_check = !!parm;
		else if (!strcmp(cmd, "bypass_pwm"))
			sodi3_bypass_pwm_check = !!parm;
		else if (!strcmp(cmd, "force_vcore_lp_mode"))
			sodi3_force_vcore_lp_mode = !!parm;
		else if (!strcmp(cmd, "cgmon"))
			mtk_idle_cg_monitor(parm ? IDLE_TYPE_SO3 : -1);
		else if (!strcmp(cmd, "log"))
			sodi3_flag = parm;
		return count;
	} else if ((!kstrtoint(cmdbuf, 10, &parm)) == 1) {
		sodi3_feature_enable = !!parm;
		return count;
	}

	return -EINVAL;
}

static const struct file_operations soidle3_state_fops = {
	.open = soidle3_state_open,
	.read = soidle3_state_read,
	.write = soidle3_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

void mtk_sodi3_init(struct dentry *root_entry)
{
	sodi3_uptime_block_count = 0;

	sodi3_bypass_idle_cond = false;
	sodi3_bypass_dis_check = false;
	sodi3_bypass_pwm_check = false;
	sodi3_disp_is_ready = false;
	sodi3_force_vcore_lp_mode = false;

	debugfs_create_file(
		"soidle3_state", 0644, root_entry, NULL, &soidle3_state_fops);

	mtk_idle_block_setting(IDLE_TYPE_SO3, so3_cnt, so3_block_cnt);
}

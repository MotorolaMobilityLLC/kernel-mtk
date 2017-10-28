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

#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/suspend.h>

#include <linux/platform_device.h>

#include <mach/fliper.h>
#include <mt_vcorefs_governor.h>
#include <mt_vcorefs_manager.h>

#include <mach/mt_emi_bm.h>
#include <mach/mt_ppm_api.h>

#define SEQ_printf(m, x...)\
	do {\
		if (m)\
			seq_printf(m, x);\
		else\
			pr_debug(x);\
	} while (0)
#undef TAG
#define TAG "[SOC DVFS FLIPER]"
#define X_ms 700
#define Y_steps (4200/X_ms)

#define MIN_DEFAULT_LPM 500
int l_boost_flag;
/*
#if 0
extern void BM_Enable(const unsigned int);
extern void BM_Pause(void);
extern int BM_GetLatencyCycle(const unsigned int);
#endif
*/
static unsigned long long emi_bw;
static int step;
static void mt_power_pef_transfer(void);
static DEFINE_TIMER(mt_pp_transfer_timer, (void *)mt_power_pef_transfer, 0, 0);
static void mt_power_pef_transfer_work(void);
static DECLARE_WORK(mt_pp_work, (void *) mt_power_pef_transfer_work);

static int cg_lpm_bw_threshold, cg_hpm_bw_threshold;
static int cg_fliper_enabled;
static int fliper_debug;
static int LPM_MAX_BW, HPM_MAX_BW;
static int pre_bw1, pre_bw2;
#if 0
static unsigned int emi_polling(unsigned int *__restrict__ emi_value)
{
	int		j = 0;		/* skip 4 WSCTs */

	BM_Pause();


	/* Get Latency */
	emi_value[j++] = BM_GetLatencyCycle(1);
	emi_value[j++] = BM_GetLatencyCycle(2);
	emi_value[j++] = BM_GetLatencyCycle(3);
	emi_value[j++] = BM_GetLatencyCycle(4);
	emi_value[j++] = BM_GetLatencyCycle(5);
	emi_value[j++] = BM_GetLatencyCycle(6);
	emi_value[j++] = BM_GetLatencyCycle(7);
	emi_value[j++] = BM_GetLatencyCycle(8);

	BM_Enable(0);
	BM_Enable(1);

	return j;
}
#endif
/******* FLIPER SETTING *********/
static void enable_fliper_polling(void)
{
	l_boost_flag = 0;
	mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	pr_debug(TAG"fliper polling start\n");
	mod_timer(&mt_pp_transfer_timer, jiffies + msecs_to_jiffies(X_ms));
}

static void disable_fliper_polling(void)
{
	mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	pr_crit("fliper polling disable ---\n");
	del_timer(&mt_pp_transfer_timer);
}

static void mt_power_pef_transfer(void)
{
	mod_timer(&mt_pp_transfer_timer, jiffies + msecs_to_jiffies(X_ms));
	schedule_work(&mt_pp_work);
}

int getCGHistory(void)
{
	return BM_GetBWST1();
}

int getCGConfiguration(void)
{
	return BM_GetBW1();
}

int setCG(const unsigned int value)
{
	return BM_SetBW1(value);
}

void enable_cg_fliper(int enable)
{
	if (enable) {
		vcorefs_enable_perform_bw(true);
		pr_debug(TAG"CG fliper enabled\n");

		cg_fliper_enabled = 1;
	} else {
		vcorefs_enable_perform_bw(false);
		pr_debug(TAG"CG fliper disabled\n");

		cg_fliper_enabled = 0;
	}
}

int cg_set_threshold(int bw1, int bw2)
{
	int lpm_threshold, hpm_threshold;
	int ddr_perf = vcorefs_get_ddr_by_steps(OPPI_PERF);
	int ddr_curr = vcorefs_get_curr_ddr();

	if (bw1 <= BW_THRESHOLD_MAX && bw1 >= BW_THRESHOLD_MIN &&
			bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN &&
			bw1 != pre_bw1 && bw2 != pre_bw2) {
		lpm_threshold = bw1 * THRESHOLD_SCALE / LPM_MAX_BW + 1;
		hpm_threshold = bw2 * THRESHOLD_SCALE / HPM_MAX_BW;
		if (lpm_threshold > 127 || lpm_threshold < 1 || hpm_threshold > 127 || hpm_threshold < 1) {
			pr_debug(TAG"error set threshold out of range\n");
			return 0;
		}
		if (ddr_perf == ddr_curr) {
			setCG(0x8300b0 | (hpm_threshold << 8));
			pr_debug(TAG"ddr high, Configure CG: 0x%08x\n", getCGConfiguration());
		} else {
			setCG(0x8300b0 | (lpm_threshold << 8));
			pr_debug(TAG"ddr low, Configure CG: 0x%08x\n", getCGConfiguration());
		}
		vcorefs_set_perform_bw_threshold(lpm_threshold, hpm_threshold);
		pr_debug(TAG"Set CG bdw threshold %d %d -> %d %d, (%d,%d)\n",
				cg_lpm_bw_threshold, cg_hpm_bw_threshold, bw1, bw2, lpm_threshold, hpm_threshold);

		cg_lpm_bw_threshold = bw1;
		cg_hpm_bw_threshold = bw2;

	}
#if 0
	else {
		pr_debug(TAG"Set CG bdw threshold Error: (MAX:%d, MIN:%d)\n",
			BW_THRESHOLD_MAX, BW_THRESHOLD_MIN);
	}
#endif

	pre_bw1 = bw1;
	pre_bw2 = bw2;

	return 0;
}

int cg_restore_threshold(void)
{
#if 0
	pr_debug(TAG"Restore CG bdw threshold %d %d -> %d %d\n",
		cg_lpm_bw_threshold, cg_hpm_bw_threshold, CG_DEFAULT_LPM, CG_DEFAULT_HPM);
#endif
	cg_set_threshold(CG_DEFAULT_LPM, CG_DEFAULT_HPM);
	return 0;
}

static ssize_t mt_fliper_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long arg1, arg2;
	char option[64];

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%63s %lu %lu", option, &arg1, &arg2) == 3 ||
			sscanf(buf, "%63s %lu", option, &arg1) == 2) {

		if (strncmp(option, "ENABLE_CG", 9) == 0) {
			enable_cg_fliper(arg1);
		} else if (strncmp(option, "SET_CG_THRES", 12) == 0) {
			cg_set_threshold(arg1, arg2);
		} else if (strncmp(option, "RESTORE_CG", 10) == 0) {
			cg_restore_threshold();
		}	else if (strncmp(option, "SET_CG", 6) == 0) {
			setCG(arg1);
		} else if (strncmp(option, "POWER_MODE", 10) == 0) {
			if (!fliper_debug) {
				if (arg1 == Default) {
					pr_debug(TAG"POWER_MODE: default\n");
					enable_cg_fliper(1);
					enable_fliper_polling();
					cg_set_threshold(CG_DEFAULT_LPM, CG_DEFAULT_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
				} else if (arg1 == Low_Power_Mode) {
					pr_debug(TAG"POWER_MODE: LOW_POWER\n");
#if 0
					enable_cg_fliper(1);
					cg_set_threshold(CG_LOW_POWER_LPM, CG_LOW_POWER_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
#else
					enable_cg_fliper(0);
					disable_fliper_polling();
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
#endif
				} else if (arg1 == Just_Make_Mode) {
					pr_debug(TAG"POWER_MODE: JUST_MAKE\n");
					enable_cg_fliper(1);
					enable_fliper_polling();
					cg_set_threshold(CG_JUST_MAKE_LPM, CG_JUST_MAKE_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
				} else if (arg1 == Performance_Mode) {
					pr_debug(TAG"POWER_MODE: PERFORMANCE\n");
					enable_cg_fliper(1);
					enable_fliper_polling();
#if 0
					cg_set_threshold(CG_PERFORMANCE_LPM, CG_PERFORMANCE_HPM);
#else
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_PERF);
#endif
				}
			}
		} else if (strncmp(option, "DEBUG", 5) == 0) {
			fliper_debug = arg1;
			if (fliper_debug >= 2)
				enable_fliper_polling();
			else
				disable_fliper_polling();
		} else {
			pr_debug(TAG"unknown option\n");
		}

	}

	return cnt;
}

static int mt_fliper_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "-----------------------------------------------------\n");
	SEQ_printf(m, "CG Fliper Enabled:%d, bw threshold:%d %dMB/s\n",
			cg_fliper_enabled, cg_lpm_bw_threshold, cg_hpm_bw_threshold);
	SEQ_printf(m, "CG History: 0x%08x\n", getCGHistory());
	SEQ_printf(m, "CG Configuration: 0x%08x\n", getCGConfiguration());
	if (fliper_debug == 2)
		SEQ_printf(m, "BW: %6llu\n", emi_bw);
	SEQ_printf(m, "-----------------------------------------------------\n");
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_fliper_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_fliper_show, inode->i_private);
}

static const struct file_operations mt_fliper_fops = {
	.open = mt_fliper_open,
	.write = mt_fliper_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

	static int
fliper_pm_callback(struct notifier_block *nb,
		unsigned long action, void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
		disable_fliper_polling();
		break;
	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
		pr_debug(TAG"Resume, restore CG configuration\n");
		pre_bw1 = BW_THRESHOLD_MAX;
		pre_bw2 = BW_THRESHOLD_MAX;
		cg_set_threshold(CG_DEFAULT_LPM, CG_DEFAULT_HPM);
		enable_fliper_polling();
		break;

	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}


static void mt_power_pef_transfer_work(void)
{
	int count;
	int threshold_counter;

	threshold_counter = getCGHistory();
	for (count = 0; threshold_counter != 0;
			count++, threshold_counter &= threshold_counter - 1)
		;


	/*Get EMI*/
	if (fliper_debug == 2) {
		emi_bw = get_mem_bw();
		pr_crit(TAG"EMI:Rate %6llu\n", emi_bw);
	}
/*
	if (fliper_debug == 3){
		emi_polling(emi_value);
		pr_crit(TAG"latency %d:%d:%d:%d:%d:%d:%d:%d\n",
		emi_value[0], emi_value[1],emi_value[2], emi_value[3],
		emi_value[4], emi_value[5], emi_value[6], emi_value[7]);
	}
	*/
	if (count > 26 && cg_lpm_bw_threshold >= 1000) {
		cg_lpm_bw_threshold -= 500;
		cg_hpm_bw_threshold -= 500;
		cg_set_threshold(cg_lpm_bw_threshold, cg_hpm_bw_threshold);
		step = Y_steps;
		l_boost_flag = 1;
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 1066000);
	}
	if (count > 26 && cg_lpm_bw_threshold >= MIN_DEFAULT_LPM)
		step = Y_steps;

	if (count <= 26)
		step--;
	if (step <= 0 && l_boost_flag == 1) {
		cg_restore_threshold();
		l_boost_flag = 0;
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	}
}
/*--------------------INIT------------------------*/
#define TIME_5SEC_IN_MS 5000
static int __init init_fliper(void)
{
	struct proc_dir_entry *pe;

	pr_debug(TAG"init fliper driver start\n");
	pe = proc_create("fliper", 0644, NULL, &mt_fliper_fops);
	if (!pe)
		return -ENOMEM;

	LPM_MAX_BW = dram_steps_freq(1) * 8;
	HPM_MAX_BW = dram_steps_freq(0) * 8;
	pr_debug(TAG"(LPM_MAX_BW,HPM_MAX_BW):%d,%d\n", LPM_MAX_BW, HPM_MAX_BW);
	cg_set_threshold(CG_DEFAULT_LPM, CG_DEFAULT_HPM);
	enable_cg_fliper(1);
	pre_bw1 = BW_THRESHOLD_MAX;
	pre_bw2 = BW_THRESHOLD_MAX;

	enable_fliper_polling();
	fliper_debug = 0;

	pm_notifier(fliper_pm_callback, 0);


#if 0
	pr_debug("prepare mt pp transfer: jiffies:%lu-->%lu\n", jiffies, jiffies + msecs_to_jiffies(TIME_5SEC_IN_MS));
	pr_debug("-	next jiffies:%lu >>> %lu\n", jiffies, jiffies + msecs_to_jiffies(X_ms));
	mod_timer(&mt_pp_transfer_timer, jiffies + msecs_to_jiffies(TIME_5SEC_IN_MS));
#endif

	pr_debug(TAG"init fliper driver done\n");

	return 0;
}
late_initcall(init_fliper);


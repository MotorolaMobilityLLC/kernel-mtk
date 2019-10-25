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

#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>

#include <linux/platform_device.h>

#include <mach/fliper.h>
#include <mtk_vcorefs_governor.h>
#include <mtk_vcorefs_manager.h>

#include <mtk_emi_bm.h>

#define SEQ_printf(m, x...)                                                    \
	do {                                                                   \
		if (m)                                                         \
			seq_printf(m, x);                                      \
		else                                                           \
			pr_debug(x);                                           \
	} while (0)
#undef TAG
#define TAG "[SOC DVFS FLIPER]"

#define THRESHOLD_SCALE 64
#define BW_THRESHOLD_MAX 12800
#define BW_THRESHOLD_MIN 100
#define BW_MARGIN 300

#define BIT_BW1_INT_EN 0       /*1*/
#define BIT_BW1_INT_MASK 1     /*no use*/
#define BIT_SM_BW_INT_MODE 3   /*0*/
#define BIT_BW1_INT_PER 4      /*period*/
#define BIT_SM_BW_INT_BW_THR 8 /*threshold*/
#define BIT_BW1_INT_BW_SEL 16  /*axi sel*/
#define BIT_BW1_INT_CLR 31     /*1*/

#define DEFAULT_INT_EN 1
#define DEFAULT_INT_MASK 0
#define DEFAULT_INT_MODE 0
#define DEFAULT_PER 0       /*0:2^20, 1:2^21, 2: 2^22, 3:2^24*/
#define DEFAULT_PER_TOTAL 2 /*0:2^20, 1:2^21, 2: 2^22, 3:2^24*/
#if 0
#define DEFAULT_SEL_LP4 0x43
#else
#define DEFAULT_SEL_LP4 0xc3
#endif
#define DEFAULT_SEL_LP3 0x43
#define DEFAULT_INT_CLR 1

#define LP4_CG_ULPM_BW_THRESHOLD 6400
#define LP4_CG_LPM_BW_THRESHOLD 6400
#define LP4_CG_HPM_BW_THRESHOLD 6100
#define LP4_CG_LOW_POWER_ULPM 6400
#define LP4_CG_LOW_POWER_LPM 6400
#define LP4_CG_LOW_POWER_HPM 6100

#define LP3_CG_ULPM_BW_THRESHOLD 2000
#define LP3_CG_LPM_BW_THRESHOLD 2000
#define LP3_CG_HPM_BW_THRESHOLD 1700
#define LP3_CG_LOW_POWER_ULPM 3000
#define LP3_CG_LOW_POWER_LPM 3000
#define LP3_CG_LOW_POWER_HPM 2700

#define CG_SPORT_ULPM 500
#define CG_SPORT_LPM 500
#define CG_SPORT_HPM 500

enum DDRTYPE { TYPE_LPDDR3 = 1, TYPE_LPDDR4, TYPE_LPDDR4X };

static int cg_ulpm_bw_threshold, cg_lpm_bw_threshold, cg_hpm_bw_threshold;
static u32 total_ulpm_bw_threshold, total_lpm_bw_threshold,
    total_hpm_bw_threshold;
static int cg_fliper_enabled, total_fliper_enabled;
static int fliper_debug;
static int POWER_MODE;
static int ULPM_MAX_BW, LPM_MAX_BW, HPM_MAX_BW;
static int TOTAL_ULPM_BW_THRESHOLD, TOTAL_LPM_BW_THRESHOLD,
    TOTAL_HPM_BW_THRESHOLD;
static int CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD;
static unsigned int cg_reg, total_reg;
static int perf_now;
/******* FLIPER SETTING *********/

int getTotalHistory(void) { return BM_GetBWST(); }

int getCGHistory(void) { return BM_GetBWST1(); }

int getTotalConfiguration(void) { return BM_GetBW(); }

int getCGConfiguration(void) { return BM_GetBW1(); }

int setTotal(const unsigned int value)
{
	int ret;

	ret = BM_SetBW(value);

	return ret;
}

int setCG(const unsigned int value)
{
	int ret;

	ret = BM_SetBW1(value);

	return ret;
}

void enable_cg_fliper(int enable)
{

	if (enable) {
		vcorefs_enable_perform_bw(true);
		pr_debug(TAG "CG fliper enabled\n");

		cg_fliper_enabled = 1;
	} else {
		vcorefs_enable_perform_bw(false);
		pr_debug(TAG "CG fliper disabled\n");

		cg_fliper_enabled = 0;
	}
}

void enable_total_fliper(int enable)
{

	if (enable) {
		vcorefs_enable_total_bw(true);
		pr_debug(TAG "TOTAL fliper enabled\n");

		total_fliper_enabled = 1;
	} else {
		vcorefs_enable_total_bw(false);
		pr_debug(TAG "TOTAL fliper disabled\n");

		total_fliper_enabled = 0;
	}
}

int disable_cg_fliper(void)
{
	fliper_debug = 1;
	enable_cg_fliper(0);

	return 0;
}

int cg_set_bw(int threshold)
{

	if (threshold == -1) {
		fliper_debug = 0;
		cg_restore_threshold();
	} else if (threshold > 0) {
		fliper_debug = 1;
		vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
		cg_set_threshold(threshold, threshold, threshold - BW_MARGIN);
	}

	return 0;
}

int cg_set_threshold(int bw1, int bw2, int bw3)
{
	int ulpm_threshold, lpm_threshold, hpm_threshold;
	int ddr_perf = vcorefs_get_ddr_by_steps(OPPI_PERF);
	int ddr_lpm = vcorefs_get_ddr_by_steps(OPPI_LOW_PWR);
	int ddr_ulpm = vcorefs_get_ddr_by_steps(OPPI_ULTRA_LOW_PWR);
	int ddr_curr = vcorefs_get_curr_ddr();

	if (bw1 <= BW_THRESHOLD_MAX && bw1 >= BW_THRESHOLD_MIN &&
	    bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN &&
	    bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN) {
		if (get_ddr_type() == TYPE_LPDDR3) {
			ulpm_threshold = bw1 * THRESHOLD_SCALE / ULPM_MAX_BW;
			lpm_threshold = bw2 * THRESHOLD_SCALE / LPM_MAX_BW;
			hpm_threshold = bw3 * THRESHOLD_SCALE / HPM_MAX_BW;
			if (ulpm_threshold >= 128)
				ulpm_threshold = 127;
			if (lpm_threshold >= 128)
				lpm_threshold = 127;
			if (hpm_threshold >= 128)
				hpm_threshold = 127;
		} else {
			ulpm_threshold =
			    bw1 * THRESHOLD_SCALE * 2 / ULPM_MAX_BW;
			lpm_threshold = bw2 * THRESHOLD_SCALE * 2 / LPM_MAX_BW;
			hpm_threshold = bw3 * THRESHOLD_SCALE * 2 / HPM_MAX_BW;
			if (ulpm_threshold >= 128)
				ulpm_threshold = 127;
			if (lpm_threshold >= 128)
				lpm_threshold = 127;
			if (hpm_threshold >= 128)
				hpm_threshold = 127;
		}
		if (ulpm_threshold > 127 || ulpm_threshold < 1 ||
		    lpm_threshold > 127 || lpm_threshold < 1 ||
		    hpm_threshold > 127 || hpm_threshold < 1) {
			pr_debug(TAG "error set threshold out of range\n");
			return 0;
		}
		if (ddr_perf == ddr_curr) {
			setCG(cg_reg | (hpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr high, Configure CG: 0x%08x\n",
				 getCGConfiguration());
		} else if (ddr_lpm == ddr_curr) {
			setCG(cg_reg | (lpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr low, Configure CG: 0x%08x\n",
				 getCGConfiguration());
		} else if (ddr_ulpm == ddr_curr) {
			setCG(cg_reg |
			      (ulpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr ultra low, Configure CG: 0x%08x\n",
				 getCGConfiguration());
		}
		vcorefs_set_perform_bw_threshold(ulpm_threshold, lpm_threshold,
						 hpm_threshold);
		pr_debug(
		    TAG
		    "Set CG bdw threshold %d %d %d -> %d %d %d, (%d %d,%d)\n",
		    cg_ulpm_bw_threshold, cg_lpm_bw_threshold,
		    cg_hpm_bw_threshold, bw1, bw2, bw3, ulpm_threshold,
		    lpm_threshold, hpm_threshold);

		cg_ulpm_bw_threshold = bw1;
		cg_lpm_bw_threshold = bw2;
		cg_hpm_bw_threshold = bw3;

	} else {
		pr_debug(TAG "Set CG bdw threshold Error: (MAX:%d, MIN:%d)\n",
			 BW_THRESHOLD_MAX, BW_THRESHOLD_MIN);
	}

	return 0;
}

int total_set_threshold(int bw1, int bw2, int bw3)
{
	u32 ulpm_threshold, lpm_threshold, hpm_threshold;
	int ddr_perf = vcorefs_get_ddr_by_steps(OPPI_PERF);
	int ddr_lpm = vcorefs_get_ddr_by_steps(OPPI_LOW_PWR);
	int ddr_ulpm = vcorefs_get_ddr_by_steps(OPPI_ULTRA_LOW_PWR);
	int ddr_curr = vcorefs_get_curr_ddr();

	if (bw1 <= BW_THRESHOLD_MAX && bw1 >= BW_THRESHOLD_MIN &&
	    bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN &&
	    bw2 <= BW_THRESHOLD_MAX && bw2 >= BW_THRESHOLD_MIN) {
		if (get_ddr_type() == TYPE_LPDDR3) {
			ulpm_threshold = bw1 * THRESHOLD_SCALE / ULPM_MAX_BW;
			lpm_threshold = bw2 * THRESHOLD_SCALE / LPM_MAX_BW;
			hpm_threshold = bw3 * THRESHOLD_SCALE / HPM_MAX_BW;
			if (ulpm_threshold >= 128)
				ulpm_threshold = 127;
			if (lpm_threshold >= 128)
				lpm_threshold = 127;
			if (hpm_threshold >= 128)
				hpm_threshold = 127;
		} else {
			ulpm_threshold =
			    bw1 * THRESHOLD_SCALE * 2 / ULPM_MAX_BW;
			lpm_threshold = bw2 * THRESHOLD_SCALE * 2 / LPM_MAX_BW;
			hpm_threshold = bw3 * THRESHOLD_SCALE * 2 / HPM_MAX_BW;
			if (ulpm_threshold >= 128)
				ulpm_threshold = 127;
			if (lpm_threshold >= 128)
				lpm_threshold = 127;
			if (hpm_threshold >= 128)
				hpm_threshold = 127;
		}
		if (ulpm_threshold > 127 || ulpm_threshold < 1 ||
		    lpm_threshold > 127 || lpm_threshold < 1 ||
		    hpm_threshold > 127 || hpm_threshold < 1) {
			pr_debug(TAG "error set threshold out of range\n");
			return 0;
		}
		if (ddr_perf == ddr_curr) {
			setTotal(total_reg |
				 (hpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr high, Configure TOTAL: 0x%08x\n",
				 getTotalConfiguration());
		} else if (ddr_lpm == ddr_curr) {
			setTotal(total_reg |
				 (lpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr low, Configure TOTAL: 0x%08x\n",
				 getTotalConfiguration());
		} else if (ddr_ulpm == ddr_curr) {
			setTotal(total_reg |
				 (ulpm_threshold << BIT_SM_BW_INT_BW_THR));
			pr_debug(TAG "ddr ultra low, Configure TOTAL: 0x%08x\n",
				 getTotalConfiguration());
		}
		vcorefs_set_total_bw_threshold(ulpm_threshold, lpm_threshold,
					       hpm_threshold);
		pr_debug(
		    TAG
		    "Set TOTAL bdw threshold %u %u %u-> %u %u %u, (%u %u,%u)\n",
		    total_ulpm_bw_threshold, total_lpm_bw_threshold,
		    total_hpm_bw_threshold, bw1, bw2, bw3, ulpm_threshold,
		    lpm_threshold, hpm_threshold);

		total_ulpm_bw_threshold = bw1;
		total_lpm_bw_threshold = bw2;
		total_hpm_bw_threshold = bw3;

	} else {
		pr_debug(TAG
			 "Set TOTAL bdw threshold Error: (MAX:%d, MIN:%d)\n",
			 BW_THRESHOLD_MAX, BW_THRESHOLD_MIN);
	}

	return 0;
}

int cg_restore_threshold(void)
{

	if (Default == POWER_MODE) {
		pr_debug(TAG "restrore POWER_MODE: default\n");
		enable_cg_fliper(1);
		cg_set_threshold(CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD,
				 CG_HPM_BW_THRESHOLD);
		vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
	} else if (Low_Power_Mode == POWER_MODE) {
		pr_debug(TAG "restrore POWER_MODE: LOW_POWER\n");
		if (get_ddr_type() == TYPE_LPDDR3) {
			enable_cg_fliper(1);
			cg_set_threshold(LP3_CG_LOW_POWER_ULPM,
					 LP3_CG_LOW_POWER_LPM,
					 LP3_CG_LOW_POWER_HPM);
			vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
		} else {
			enable_cg_fliper(1);
			cg_set_threshold(LP4_CG_LOW_POWER_ULPM,
					 LP4_CG_LOW_POWER_LPM,
					 LP4_CG_LOW_POWER_HPM);
			vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
		}
	} else if (Sport_Mode == POWER_MODE) {
		pr_debug(TAG "restrore POWER_MODE: SPORT\n");
		enable_cg_fliper(1);
		cg_set_threshold(CG_SPORT_ULPM, CG_SPORT_LPM, CG_SPORT_HPM);
		vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
	} else if (Performance_Mode == POWER_MODE) {
		pr_debug(TAG "restrore POWER_MODE: PERFORMANCE\n");
		enable_cg_fliper(1);
#if 0
		cg_set_threshold(CG_PERFORMANCE_ULPM, CG_PERFORMANCE_LPM, CG_PERFORMANCE_HPM);
#else
		vcorefs_request_dvfs_opp(KIR_PERF, OPPI_PERF);
#endif
	}

	return 0;
#if 0
	pr_debug(TAG"Restore CG bdw threshold %d %d %d -> %d %d %d\n",
			cg_ulpm_bw_threshold, cg_lpm_bw_threshold, cg_hpm_bw_threshold,
			CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	cg_set_threshold(CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
	return 0;
	if (arg1 == Default) {
		pr_debug(TAG"POWER_MODE: default\n");
				POWER_MODE = Default;
				enable_cg_fliper(1);
				cg_set_threshold(CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD, CG_HPM_BW_THRESHOLD);
				vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
			} else if (arg1 == Low_Power_Mode) {
				pr_debug(TAG"POWER_MODE: LOW_POWER\n");
				POWER_MODE = Low_Power_Mode;
				if (get_ddr_type() == TYPE_LPDDR3) {
					enable_cg_fliper(1);
					cg_set_threshold(CG_LOW_POWER_ULPM, CG_LOW_POWER_LPM, CG_LOW_POWER_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
				} else {
					enable_cg_fliper(0);
					cg_set_threshold(CG_LOW_POWER_ULPM, CG_LOW_POWER_LPM, CG_LOW_POWER_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
				}
			} else if (arg1 == Sport_Mode) {
				pr_debug(TAG"POWER_MODE: SPORT\n");
				POWER_MODE = Sport_Mode;
				enable_cg_fliper(1);
				cg_set_threshold(CG_SPORT_ULPM, CG_SPORT_LPM, CG_SPORT_HPM);
				vcorefs_request_dvfs_opp(KIR_PERF, OPPI_UNREQ);
			} else if (arg1 == Performance_Mode) {
				pr_debug(TAG"POWER_MODE: PERFORMANCE\n");
				POWER_MODE = Performance_Mode;
				enable_cg_fliper(1);
#if 0
				cg_set_threshold(CG_PERFORMANCE_ULPM, CG_PERFORMANCE_LPM, CG_PERFORMANCE_HPM);
#else
				vcorefs_request_dvfs_opp(KIR_PERF, OPPI_PERF);
#endif
			}
#endif
}

int total_restore_threshold(void)
{
	pr_debug(TAG "Restore TOTAL bdw threshold %u %u %u -> %d %d %d\n",
		 total_ulpm_bw_threshold, total_lpm_bw_threshold,
		 total_hpm_bw_threshold, TOTAL_ULPM_BW_THRESHOLD,
		 TOTAL_LPM_BW_THRESHOLD, TOTAL_HPM_BW_THRESHOLD);
	total_set_threshold(TOTAL_ULPM_BW_THRESHOLD, TOTAL_LPM_BW_THRESHOLD,
			    TOTAL_HPM_BW_THRESHOLD);
	return 0;
}

static ssize_t mt_fliper_write(struct file *filp, const char *ubuf, size_t cnt,
			       loff_t *data)
{
	char buf[64];
	long arg1, arg2, arg3;
	char option[64];

	arg1 = 0;
	arg2 = 0;
	arg3 = 0;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	if (sscanf(buf, "%61s %lu %lu %lu", option, &arg1, &arg2, &arg3) == 4 ||
	    sscanf(buf, "%61s %lu", option, &arg1) == 2) {
		if (strncmp(option, "ENABLE_CG", 9) == 0) {
			enable_cg_fliper(arg1);
		} else if (strncmp(option, "ENABLE_TOTAL", 12) == 0) {
			enable_total_fliper(arg1);
		} else if (strncmp(option, "SET_CG_THRESH", 13) == 0) {
			cg_set_threshold(arg1, arg1, arg1 - 300);
		} else if (strncmp(option, "SET_CG_THRES", 12) == 0) {
			cg_set_threshold(arg1, arg2, arg3);
		} else if (strncmp(option, "SET_TOTAL_THRES", 15) == 0) {
			total_set_threshold(arg1, arg2, arg3);
		} else if (strncmp(option, "RESTORE_CG", 10) == 0) {
			cg_restore_threshold();
		} else if (strncmp(option, "RESTORE_TOTAL", 13) == 0) {
			total_restore_threshold();
		} else if (strncmp(option, "SET_CG", 6) == 0) {
			setCG(arg1);
		} else if (strncmp(option, "SET_TOTAL", 9) == 0) {
			setTotal(arg1);
		} else if (strncmp(option, "SET_VCORE_MIN", 13) == 0) {
			vcorefs_request_dvfs_opp(KIR_PERF, arg1);
		} else if (strncmp(option, "POWER_MODE", 10) == 0) {
			if (!fliper_debug) {
				if (arg1 == Default) {
					pr_debug(TAG "POWER_MODE: default\n");
					POWER_MODE = Default;
					enable_cg_fliper(1);
					cg_set_threshold(CG_ULPM_BW_THRESHOLD,
							 CG_LPM_BW_THRESHOLD,
							 CG_HPM_BW_THRESHOLD);
					vcorefs_request_dvfs_opp(KIR_PERF,
								 OPPI_UNREQ);
				} else if (arg1 == Low_Power_Mode) {
					pr_debug(TAG "POWER_MODE: LOW_POWER\n");
					POWER_MODE = Low_Power_Mode;
					if (get_ddr_type() == TYPE_LPDDR3) {
						enable_cg_fliper(1);
						cg_set_threshold(
						    LP3_CG_LOW_POWER_ULPM,
						    LP3_CG_LOW_POWER_LPM,
						    LP3_CG_LOW_POWER_HPM);
						vcorefs_request_dvfs_opp(
						    KIR_PERF, OPPI_UNREQ);
					} else {
						enable_cg_fliper(1);
						cg_set_threshold(
						    LP4_CG_LOW_POWER_ULPM,
						    LP4_CG_LOW_POWER_LPM,
						    LP4_CG_LOW_POWER_HPM);
						vcorefs_request_dvfs_opp(
						    KIR_PERF, OPPI_UNREQ);
					}
				} else if (arg1 == Sport_Mode) {
					pr_debug(TAG "POWER_MODE: SPORT\n");
					POWER_MODE = Sport_Mode;
					enable_cg_fliper(1);
					cg_set_threshold(CG_SPORT_ULPM,
							 CG_SPORT_LPM,
							 CG_SPORT_HPM);
					vcorefs_request_dvfs_opp(KIR_PERF,
								 OPPI_UNREQ);
				} else if (arg1 == Performance_Mode) {
					pr_debug(TAG
						 "POWER_MODE: PERFORMANCE\n");
					POWER_MODE = Performance_Mode;
					enable_cg_fliper(1);
#if 0
					cg_set_threshold(CG_PERFORMANCE_ULPM, CG_PERFORMANCE_LPM, CG_PERFORMANCE_HPM);
#else
					vcorefs_request_dvfs_opp(KIR_PERF,
								 OPPI_PERF);
#endif
				}
			}
		} else if (strncmp(option, "DEBUG", 5) == 0) {
			fliper_debug = arg1;
		} else {
			pr_debug(TAG "unknown option\n");
		}
	}
	return cnt;
}

static int mt_fliper_show(struct seq_file *m, void *v)
{
	SEQ_printf(m,
		   "-----------------------------------------------------\n");
	SEQ_printf(m, "CG Fliper Enabled:%d, bw threshold:%d %d %dMB/s\n",
		   cg_fliper_enabled, cg_ulpm_bw_threshold, cg_lpm_bw_threshold,
		   cg_hpm_bw_threshold);
	SEQ_printf(m, "TOTAL Fliper Enabled:%d, bw threshold:%u %u %uMB/s\n",
		   total_fliper_enabled, total_ulpm_bw_threshold,
		   total_lpm_bw_threshold, total_hpm_bw_threshold);
	SEQ_printf(m, "KIR_PERF: %d\n", perf_now);
	SEQ_printf(m, "DEBUG: %d\n", fliper_debug);
	SEQ_printf(m, "CG History: 0x%08x\n", getCGHistory());
	SEQ_printf(m, "TOTAL History: 0x%08x\n", getTotalHistory());
	SEQ_printf(m, "CG Config: 0x%08x\n", getCGConfiguration());
	SEQ_printf(m, "TOTAL Config: 0x%08x\n", getTotalConfiguration());
	SEQ_printf(m,
		   "-----------------------------------------------------\n");
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

static ssize_t mt_perf_write(struct file *filp, const char *ubuf, size_t cnt,
			     loff_t *data)
{
	char buf[64];
	int val;
	int ret;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoint(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < -1 || val > 2)
		return -1;

	if (vcorefs_request_dvfs_opp(KIR_PERF, val) < 0)
		return cnt;

	perf_now = val;

	return cnt;
}

static int mt_perf_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", perf_now);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_perf_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_perf_show, inode->i_private);
}

static const struct file_operations mt_perf_fops = {
    .open = mt_perf_open,
    .write = mt_perf_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static ssize_t mt_cg_threshold_write(struct file *filp, const char *ubuf,
				     size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (get_ddr_type() == TYPE_LPDDR3)
		return -1;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < 400)
		return -1;

	cg_set_threshold(val, val, val - BW_MARGIN);

	return cnt;
}

static int mt_cg_threshold_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", cg_ulpm_bw_threshold);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_cg_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cg_threshold_show, inode->i_private);
}

static const struct file_operations mt_cg_threshold_fops = {
    .open = mt_cg_threshold_open,
    .write = mt_cg_threshold_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static ssize_t mt_cg_threshold_ddr3_write(struct file *filp, const char *ubuf,
					  size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < 400)
		return -1;

	if (get_ddr_type() == TYPE_LPDDR3)
		cg_set_threshold(val, val, val - BW_MARGIN);

	return cnt;
}

static int mt_cg_threshold_ddr3_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", cg_ulpm_bw_threshold);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_cg_threshold_ddr3_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cg_threshold_ddr3_show, inode->i_private);
}

static const struct file_operations mt_cg_threshold_ddr3_fops = {
    .open = mt_cg_threshold_ddr3_open,
    .write = mt_cg_threshold_ddr3_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
static ssize_t mt_cg_enable_write(struct file *filp, const char *ubuf,
				  size_t cnt, loff_t *data)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (fliper_debug)
		return -1;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val < 0 || val > 1)
		return -1;

	enable_cg_fliper(val);

	return cnt;
}

static int mt_cg_enable_show(struct seq_file *m, void *v)
{
	SEQ_printf(m, "%d\n", cg_fliper_enabled);
	return 0;
}
/*** Seq operation of mtprof ****/
static int mt_cg_enable_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_cg_enable_show, inode->i_private);
}

static const struct file_operations mt_cg_enable_fops = {
    .open = mt_cg_enable_open,
    .write = mt_cg_enable_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int fliper_pm_callback(struct notifier_block *nb, unsigned long action,
			      void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
		break;
	case PM_HIBERNATION_PREPARE:
		break;

	case PM_POST_SUSPEND:
		pr_debug(TAG "Resume, restore CG configuration\n");
		cg_set_threshold(cg_ulpm_bw_threshold, cg_lpm_bw_threshold,
				 cg_hpm_bw_threshold);
		break;

	case PM_POST_HIBERNATION:
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

/*--------------------INIT------------------------*/

static int __init init_fliper(void)
{
	struct proc_dir_entry *fliperfs_dir;
	struct proc_dir_entry *perf_dir, *cg_threshold_dir,
	    *cg_threshold_ddr3_dir, *cg_enable_dir, *fliper_dir;

	pr_debug(TAG "init fliper driver start\n");
	fliperfs_dir = proc_mkdir("fliperfs", NULL);
	if (!fliperfs_dir)
		return -ENOMEM;

	perf_dir = proc_create("perf", 0644, fliperfs_dir, &mt_perf_fops);
	if (!perf_dir)
		return -ENOMEM;

	cg_threshold_dir = proc_create("cg_threshold", 0644, fliperfs_dir,
				       &mt_cg_threshold_fops);
	if (!cg_threshold_dir)
		return -ENOMEM;

	cg_threshold_ddr3_dir =
	    proc_create("cg_threshold_ddr3", 0644, fliperfs_dir,
			&mt_cg_threshold_ddr3_fops);
	if (!cg_threshold_ddr3_dir)
		return -ENOMEM;

	cg_enable_dir =
	    proc_create("cg_enable", 0644, fliperfs_dir, &mt_cg_enable_fops);
	if (!cg_enable_dir)
		return -ENOMEM;

	fliper_dir = proc_create("fliper", 0644, fliperfs_dir, &mt_fliper_fops);
	if (!fliper_dir)
		return -ENOMEM;

	if (get_ddr_type() == TYPE_LPDDR3)
		cg_reg = (DEFAULT_INT_EN << BIT_BW1_INT_EN) |
			 (DEFAULT_INT_MASK << BIT_BW1_INT_MASK) |
			 (DEFAULT_INT_MODE << BIT_SM_BW_INT_MODE) |
			 (DEFAULT_PER << BIT_BW1_INT_PER) |
			 (DEFAULT_SEL_LP3 << BIT_BW1_INT_BW_SEL) |
			 (DEFAULT_INT_CLR << BIT_BW1_INT_CLR);
	else
		cg_reg = (DEFAULT_INT_EN << BIT_BW1_INT_EN) |
			 (DEFAULT_INT_MASK << BIT_BW1_INT_MASK) |
			 (DEFAULT_INT_MODE << BIT_SM_BW_INT_MODE) |
			 (DEFAULT_PER << BIT_BW1_INT_PER) |
			 (DEFAULT_SEL_LP4 << BIT_BW1_INT_BW_SEL) |
			 (DEFAULT_INT_CLR << BIT_BW1_INT_CLR);

	total_reg = (DEFAULT_INT_EN << BIT_BW1_INT_EN) |
		    (DEFAULT_INT_MASK << BIT_BW1_INT_MASK) |
		    (DEFAULT_INT_MODE << BIT_SM_BW_INT_MODE) |
		    (DEFAULT_PER_TOTAL << BIT_BW1_INT_PER) |
		    (DEFAULT_INT_CLR << BIT_BW1_INT_CLR);

	ULPM_MAX_BW = dram_steps_freq(OPPI_ULTRA_LOW_PWR) * 4;
	LPM_MAX_BW = dram_steps_freq(OPPI_LOW_PWR) * 4;
	HPM_MAX_BW = dram_steps_freq(OPPI_PERF) * 4;

	TOTAL_ULPM_BW_THRESHOLD = ULPM_MAX_BW * 9 / 10;
	TOTAL_LPM_BW_THRESHOLD = LPM_MAX_BW * 9 / 10;
	TOTAL_HPM_BW_THRESHOLD = HPM_MAX_BW * 1 / 2;

	perf_now = -1;
	POWER_MODE = Default;

	if (get_ddr_type() == TYPE_LPDDR3) {
		CG_ULPM_BW_THRESHOLD = LP3_CG_ULPM_BW_THRESHOLD;
		CG_LPM_BW_THRESHOLD = LP3_CG_LPM_BW_THRESHOLD;
		CG_HPM_BW_THRESHOLD = LP3_CG_HPM_BW_THRESHOLD;
	} else {
		CG_ULPM_BW_THRESHOLD = LP4_CG_ULPM_BW_THRESHOLD;
		CG_LPM_BW_THRESHOLD = LP4_CG_LPM_BW_THRESHOLD;
		CG_HPM_BW_THRESHOLD = LP4_CG_HPM_BW_THRESHOLD;
	}
	pr_debug(TAG "(ULPM_MAX_BW,LPM_MAX_BW,HPM_MAX_BW):%d,%d,%d\n",
		 ULPM_MAX_BW, LPM_MAX_BW, HPM_MAX_BW);

	cg_set_threshold(CG_ULPM_BW_THRESHOLD, CG_LPM_BW_THRESHOLD,
			 CG_HPM_BW_THRESHOLD);
	total_set_threshold(TOTAL_ULPM_BW_THRESHOLD, TOTAL_LPM_BW_THRESHOLD,
			    TOTAL_HPM_BW_THRESHOLD);
#if 1
	enable_cg_fliper(1);
	enable_total_fliper(1);
	fliper_debug = 0;
#else
	fliper_debug = 1;
#endif

	pm_notifier(fliper_pm_callback, 0);

	pr_debug(TAG "init fliper driver done\n");

	return 0;
}
late_initcall(init_fliper);

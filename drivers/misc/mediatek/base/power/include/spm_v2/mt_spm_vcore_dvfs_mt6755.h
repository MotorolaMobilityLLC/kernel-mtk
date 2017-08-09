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

#ifndef _MT_SPM_VCORE_DVFS_H
#define _MT_SPM_VCORE_DVFS_H

#include <linux/kernel.h>
#include "mt_spm.h"
#include <mt-plat/aee.h>

#undef VCOREFS_SPM_TAG
#define VCOREFS_SPM_TAG "[VcoreFS_SPM]"

#define spm_vcorefs_crit(fmt, args...)	\
	pr_err(VCOREFS_SPM_TAG"[CRTT]"fmt, ##args)
#define spm_vcorefs_err(fmt, args...)	\
	pr_err(VCOREFS_SPM_TAG"[ERR]"fmt, ##args)
#define spm_vcorefs_warn(fmt, args...)	\
	pr_warn(VCOREFS_SPM_TAG"[WARN]"fmt, ##args)
#define spm_vcorefs_info(fmt, args...)	\
	pr_warn(VCOREFS_SPM_TAG""fmt, ##args)	/* pr_info(TAG""fmt, ##args) */
#define spm_vcorefs_debug(fmt, args...)	\
	pr_debug(VCOREFS_SPM_TAG""fmt, ##args)

#define spm_vcorefs_aee_warn(string, args...) do {\
	pr_err("[ERR]"string, ##args); \
	aee_kernel_warning(VCOREFS_SPM_TAG, "[ERR]"string, ##args);  \
} while (0)

/* load fw for boot up */
extern void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data);

/* vcore dvfs request */
extern int spm_vcorefs_set_dvfs_hpm(int opp, int vcore, int ddr);
extern int spm_vcorefs_set_dvfs_hpm_force(int opp, int vcore, int ddr);
extern int spm_vcorefs_set_total_bw(int opp, int vcore, int ddr);	/* OVL >3 & CA Data-Link */

/* debug only */
extern int spm_vcorefs_set_dvfs_lpm_force(int opp, int vcore, int ddr);
extern void spm_vcorefs_set_pcm_flag(u32 flag, bool set);

/* bw monitor threshold setting to spm */
extern int spm_vcorefs_set_total_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);
extern int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);

/* bw monitor enable/disable in spm dvfs logic */
extern void spm_vcorefs_enable_total_bw(bool enable);
extern void spm_vcorefs_enable_perform_bw(bool enable);
extern int spm_vcorefs_get_clk_mem_pll(void);

/* misc vcore dvfs support func */
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern int spm_vcorefs_set_cpu_dvfs_req(u32 screen_on, u32 mask);
extern u32 spm_vcorefs_get_MD_status(void);
extern bool spm_vcorefs_is_dvfs_in_porgress(void);

/* SRAM debug */
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);

#endif				/* _MT_SPM_VCORE_DVFS_H */

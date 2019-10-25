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

#include "mtk_spm.h"
#include <linux/kernel.h>
#include <mt-plat/aee.h>

#undef TAG
#define TAG "[VcoreFS_SPM]"

#define spm_vcorefs_crit spm_vcorefs_info
#define spm_vcorefs_err spm_vcorefs_info
#define spm_vcorefs_warn spm_vcorefs_info
#define spm_vcorefs_debug spm_vcorefs_info

#define spm_vcorefs_info(fmt, args...) pr_notice(TAG "" fmt, ##args)

#define spm_vcorefs_aee_warn(string, args...)                                  \
	do {                                                                   \
		pr_notice("[ERR]" string, ##args);                             \
		aee_kernel_warning(TAG, "[ERR]" string, ##args);               \
	} while (0)

/* load fw for boot up */
extern void spm_vcorefs_init_state(int opp);
extern void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data);

/* vcore dvfs request */
extern int spm_vcorefs_set_opp(int opp, int vcore, int ddr, bool non_force);
extern int spm_vcorefs_set_opp_fix(int opp, int vcore, int ddr);

/* debug only */
extern void spm_vcorefs_set_pcm_flag(u32 flag, bool set);

/* bw monitor threshold setting to spm */
extern int spm_vcorefs_set_perform_bw_threshold(u32 ulpm_threshold,
						u32 lpm_threshold,
						u32 hpm_threshold);
extern int spm_vcorefs_set_total_bw_threshold(u32 ulpm_threshold,
					      u32 lpm_threshold,
					      u32 hpm_threshold);

/* bw monitor enable/disable in spm dvfs logic */
extern void spm_vcorefs_enable_total_bw(bool enable, bool lock);
extern void spm_vcorefs_enable_perform_bw(bool enable, bool lock);

/* 2nd ulpm bw monitor */
void spm_vcorefs_enable_ulpm_perform_bw(bool enable, bool lock);
void spm_vcorefs_enable_ulpm_total_bw(bool enable, bool lock);

/* msdc dvfs setting api */
#define MSDC0_DVFS 0
#define MSDC1_DVFS 1
#define MSDC2_DVFS 2
#define MSDC3_DVFS 3

extern int spm_msdc_dvfs_setting(int msdc, bool enable);

/* misc vcore dvfs support func */
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern int spm_vcorefs_set_cpu_dvfs_req(u32 screen_on, u32 mask);
extern u32 spm_vcorefs_get_MD_status(void);
extern int spm_vcorefs_get_opp(void);
extern int spm_vcorefs_is_offload_lpm_enable(void);

/* SRAM debug */
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);

extern void ISP_Halt_Mask(int);

extern void spm_vcoefs_MD_LPM_req(bool enable);
extern void spm_vcorefs_emi_grouping_req(bool enable);
extern void spm_vcorefs_off_load_lpm_req(bool enable);
#endif /* _MT_SPM_VCORE_DVFS_H */

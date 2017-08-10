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

#include "mt_spm.h"

#undef TAG
#define TAG "[VcoreFS_SPM]"

#define spm_vcorefs_crit(fmt, args...)	\
	pr_err(TAG""fmt, ##args)
#define spm_vcorefs_err(fmt, args...)	\
	pr_err(TAG""fmt, ##args)
#define spm_vcorefs_warn(fmt, args...)	\
	pr_warn(TAG""fmt, ##args)
#define spm_vcorefs_info(fmt, args...)	\
	pr_warn(TAG""fmt, ##args)
#define spm_vcorefs_debug(fmt, args...)	\
	pr_debug(TAG""fmt, ##args)

/* SPM_SW_RSV_1[3:0], trigger by cpu_wake_up_event */
#define SPM_DVFS_NORMAL			0x1
#define SPM_DVFS_ULPM			0x2
#define SPM_CLEAN_WAKE_EVENT_DONE	0xA
#define SPM_SCREEN_SETTING_DONE		0xB
#define SPM_OFFLOAD			0xF

/* PCM_REG6_DATA[24:23]: F/W do DVFS target state */
#define SPM_VCORE_STA_REG	(0x3 << 23)
#define SPM_SCREEN_ON_HPM	(0x3 << 23)
#define SPM_SCREEN_ON_LPM	(0x2 << 23)
#define SPM_SCREEN_ON_ULPM	(0x0 << 23)

/* read shuffle value for mapping ddr freq */
#define SPM_SHUFFLE_ADDR	0x63c

#define SPM_FLAG_DVFS_ACTIVE	(1 << 22)
#define MASK_MD_DVFS_REQ	0x0

#define PMIC_ACK_DONE		(1 << 31)

/* load fw for boot up */
extern void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data);

/* vcore dvfs request */
extern int spm_set_vcore_dvfs(int opp, u32 md_dvfs_req, int kicker, int user_opp);

/* bw monitor threshold setting to spm */
extern int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);

/* bw monitor enable/disable in spm dvfs logic */
extern void spm_vcorefs_enable_perform_bw(bool enable);

/* misc vcore dvfs support func */
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern int spm_vcorefs_set_cpu_dvfs_req(u32 screen_on, u32 mask);
extern u32 spm_vcorefs_get_MD_status(void);

extern void spm_vcorefs_init_dvfs_con(void);

/* AEE debug */
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);

extern void ISP_Halt_Mask(int);

extern void spm_mask_wakeup_event(int val);

#endif	/* _MT_SPM_VCORE_DVFS_H */

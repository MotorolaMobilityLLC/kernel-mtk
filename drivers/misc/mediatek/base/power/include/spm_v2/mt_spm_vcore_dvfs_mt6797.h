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
#define SPM_SCREEN_ON			0x1
#define SPM_SCREEN_OFF			0x2
#define SPM_CLEAN_WAKE_EVENT_DONE	0xA
#define SPM_SCREEN_SETTING_DONE		0xB
#define SPM_OFFLOAD			0xF

/*
 * SPM_SW_RSV_5[1:0]: F/W do DVFS target state
 * 0x0:1600, 0x1:1270, 0x2:1066
 */
#define SPM_SCREEN_ON_HPM	0x3	/* 1.0/1600 */
#define SPM_SCREEN_ON_LPM	0x2	/* 0.9/1270 */
#define SPM_SCREEN_OFF_HPM	0x1	/* 0.9/1270 */
#define SPM_SCREEN_OFF_LPM	0x0	/* 0.9/1066 */

/* read shuffle value for mapping ddr freq */
#define SPM_SHUFFLE_ADDR	0x63c

/* load fw for boot up */
extern void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data, bool screen_on, u32 cpu_dvfs_req);

/* vcore dvfs request */
extern int spm_set_vcore_dvfs(int opp, bool screen_on);

/* bw monitor threshold setting to spm */
extern int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);

/* bw monitor enable/disable in spm dvfs logic */
extern void spm_vcorefs_enable_perform_bw(bool enable);

/* misc vcore dvfs support func */
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern int spm_vcorefs_screen_on_setting(void);
extern int spm_vcorefs_screen_off_setting(u32);
extern int spm_vcorefs_set_cpu_dvfs_req(u32 screen_on, u32 mask);
extern u32 spm_vcorefs_get_MD_status(void);

/* AEE debug */
extern void aee_rr_rec_vcore_dvfs_status(u32 val);
extern u32 aee_rr_curr_vcore_dvfs_status(void);

#endif	/* _MT_SPM_VCORE_DVFS_H */

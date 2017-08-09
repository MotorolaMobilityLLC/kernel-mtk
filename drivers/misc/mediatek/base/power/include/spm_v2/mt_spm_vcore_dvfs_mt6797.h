#ifndef _MT_SPM_VCORE_DVFS_H
#define _MT_SPM_VCORE_DVFS_H

#include <linux/kernel.h>
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

/* load fw for boot up */
extern void spm_go_to_vcore_dvfs(u32 spm_flags, u32 spm_data, bool screen_on, u32 cpu_dvfs_req);

/* vcore dvfs request */
extern int spm_set_vcore_dvfs(int opp, bool screen_on);

/* bw monitor threshold setting to spm */
extern int spm_vcorefs_set_perform_bw_threshold(u32 lpm_threshold, u32 hpm_threshold);

/* bw monitor enable/disable in spm dvfs logic */
extern void spm_vcorefs_enable_perform_bw(bool enable);
extern int spm_vcorefs_get_clk_mem_pll(void);

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

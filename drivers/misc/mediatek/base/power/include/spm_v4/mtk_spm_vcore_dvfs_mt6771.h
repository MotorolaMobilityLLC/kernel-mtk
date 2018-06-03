/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __MTK_SPM_VCORE_DVFS_MT6771_H__
#define __MTK_SPM_VCORE_DVFS_MT6771_H__

#include "mtk_spm.h"
#include <mtk_vcorefs_manager.h>

/* Feature will disable both of DVS/DFS are 0 */
/* TODO: enable after function verify */
#define SPM_VCORE_DVS_EN       0 /* SB disabled */
#define SPM_DDR_DFS_EN         0 /* SB disabled */
#define SPM_MM_CLK_EN          0 /* for intra-frame dvfs */
#define VMODEM_VCORE_COBUCK    1 /* SB disabled */

#define SPM_DVFS_TIMEOUT       1000	/* 1ms */

enum vcorefs_smc_cmd {
	VCOREFS_SMC_CMD_0,
	VCOREFS_SMC_CMD_1,
	VCOREFS_SMC_CMD_2,
	VCOREFS_SMC_CMD_3,
	NUM_VCOREFS_SMC_CMD,
};

extern void spm_go_to_vcorefs(int spm_flags);
extern int spm_set_vcore_dvfs(struct kicker_config *krconf);
extern void spm_vcorefs_init(void);
extern int spm_dvfs_flag_init(void);
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern u32 spm_vcorefs_get_MD_status(void);
extern int spm_vcorefs_pwarp_cmd(void);
extern int spm_vcorefs_get_opp(void);
extern void spm_request_dvfs_opp(int id, enum dvfs_opp opp);
extern u32 spm_vcorefs_get_md_srcclkena(void);
extern void dvfsrc_md_scenario_update(bool);
extern void dvfsrc_set_scp_vcore_request(unsigned int val);
extern void dvfsrc_set_power_model_ddr_request(unsigned int level);
extern void helio_dvfsrc_sspm_ipi_init(void);
extern void dvfsrc_hw_policy_mask(bool mask);
extern int spm_get_vcore_opp(unsigned int opp);
extern int spm_vcorefs_get_dvfs_opp(void);
extern void dvfsrc_update_sspm_vcore_opp_table(int opp, unsigned int vcore_uv);
extern void dvfsrc_update_sspm_ddr_opp_table(int opp, unsigned int ddr_khz);

#endif /* __MTK_SPM_VCORE_DVFS_MT6771_H__ */

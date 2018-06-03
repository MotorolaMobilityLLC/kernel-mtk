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

#ifndef __MTK_SPM_VCORE_DVFS_H__
#define __MTK_SPM_VCORE_DVFS_H__

#include "mtk_spm.h"
#include <mtk_vcorefs_manager.h>

/* Feature will disable both of DVS/DFS are 0 */
#define SPM_VCORE_DVS_EN       1
#define SPM_DDR_DFS_EN         1
#define SPM_MM_CLK_EN          0

#define SPM_DVFS_TIMEOUT       1000	/* 1ms */

enum dvfsrc_channel {
	DVFSRC_CHANNEL_1 = 1,
	DVFSRC_CHANNEL_2,
	DVFSRC_CHANNEL_3,
	DVFSRC_CHANNEL_4,
	NUM_DVFSRC_CHANNEL,
};

enum dvfsrc_hrt_kir {
	DVFSRC_HRT_MM,
	DVFSRC_HRT_CAM,
	DVFSRC_HRT_MD,
	NUM_DVFSRC_HRT_KIR,
};

enum vcorefs_smc_cmd {
	VCOREFS_SMC_CMD_0,
	VCOREFS_SMC_CMD_1,
	VCOREFS_SMC_CMD_2,
	NUM_VCOREFS_SMC_CMD,
};

extern void spm_go_to_vcorefs(int spm_flags);
extern int spm_set_vcore_dvfs(struct kicker_config *krconf);
extern void spm_vcorefs_init(void);
extern int spm_dvfs_flag_init(void);
extern char *spm_vcorefs_dump_dvfs_regs(char *p);
extern u32 spm_vcorefs_get_MD_status(void);
extern int spm_vcorefs_pwarp_cmd(void);
extern void spm_dvfsrc_set_channel_bw(enum dvfsrc_channel);
extern void spm_dvfsrc_hrt_bw_config(enum dvfsrc_hrt_kir hrt_kir, bool config);
extern int spm_vcorefs_get_opp(void);
extern void dvfsrc_md_scenario_update(bool);
extern void spm_msdc_wqhd_workaround(bool);

#endif /* __MTK_SPM_VCORE_DVFS_H__ */

/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <mtk_spm_internal.h>
#include <mtk_spm_reg.h>
#include <mtk_sleep_reg_md_reg.h>
#include <sleep_def.h>

int spm_dvfs_flag_init(void)
{
	return SPM_FLAG_RUN_COMMON_SCENARIO;
}

void get_spm_reg(char *p)
{
	p += sprintf(p, "SPM_SW_FLAG            : 0x%x\n",
			spm_read(SPM_SW_FLAG));
	p += sprintf(p, "SPM_SW_RSV_5           : 0x%x\n",
			spm_read(SPM_SW_RSV_5));
	p += sprintf(p, "MD2SPM_DVFS_CON        : 0x%x\n",
			spm_read(MD2SPM_DVFS_CON));
	p += sprintf(p, "SPM_DVFS_EVENT_STA     : 0x%x\n",
			spm_read(SPM_DVFS_EVENT_STA));
	p += sprintf(p, "SPM_DVFS_LEVEL         : 0x%x\n",
			spm_read(SPM_DVFS_LEVEL));
	p += sprintf(p, "SPM_DFS_LEVEL          : 0x%x\n",
			spm_read(SPM_DFS_LEVEL));
	p += sprintf(p, "SPM_DVS_LEVEL          : 0x%x\n",
			spm_read(SPM_DVS_LEVEL));
	p += sprintf(p, "SPM_ACK_CHK_TIMER2     : 0x%x\n",
			spm_read(SPM_ACK_CHK_TIMER2));

	p += sprintf(p, "PCM_REG_DATA_0~3       : 0x%x, 0x%x, 0x%x, 0x%x\n",
			spm_read(PCM_REG0_DATA), spm_read(PCM_REG1_DATA),
			spm_read(PCM_REG2_DATA), spm_read(PCM_REG3_DATA));
	p += sprintf(p, "PCM_REG_DATA_4~7       : 0x%x, 0x%x, 0x%x, 0x%x\n",
			spm_read(PCM_REG4_DATA), spm_read(PCM_REG5_DATA),
			spm_read(PCM_REG6_DATA), spm_read(PCM_REG7_DATA));
	p += sprintf(p, "PCM_REG_DATA_8~11      : 0x%x, 0x%x, 0x%x, 0x%x\n",
			spm_read(PCM_REG8_DATA), spm_read(PCM_REG9_DATA),
			spm_read(PCM_REG10_DATA), spm_read(PCM_REG11_DATA));
	p += sprintf(p, "PCM_REG_DATA_12~15     : 0x%x, 0x%x, 0x%x, 0x%x\n",
			spm_read(PCM_REG12_DATA), spm_read(PCM_REG13_DATA),
			spm_read(PCM_REG14_DATA), spm_read(PCM_REG15_DATA));
	p += sprintf(p, "MD_SPM_DVFS_CMD16~19   : 0x%x, 0x%x, 0x%x, 0x%x\n",
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD16),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD17),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD18),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD19));
	p += sprintf(p, "SPM_DVFS_CMD0~1        : 0x%x, 0x%x\n",
			spm_read(SPM_DVFS_CMD0), spm_read(SPM_DVFS_CMD1));
	p += sprintf(p, "PCM_IM_PTR             : 0x%x (%u)\n",
			spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));
	p += sprintf(p, "\n");
}


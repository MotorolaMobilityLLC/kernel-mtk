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

int spm_dvfs_flag_init(int dvfsrc_en)
{
#if 0 /*CW disable for now*/
	if (dvfsrc_en)
		return SPM_FLAG_RUN_COMMON_SCENARIO;
#endif
	return SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DISABLE_VCORE_DVS |
		SPM_FLAG_DISABLE_VCORE_DFS;
}

u32 spm_get_dvfs_level(void)
{
	return spm_read(SPM_DVFS_LEVEL);
}
#if 1
u32 spm_get_pcm_reg9_data(void)
{
	return spm_read(SPM_DVFS_STA);
}
#endif
u32 spm_vcorefs_get_MD_status(void)
{
	return spm_read(MD2SPM_DVFS_CON);
}

u32 spm_vcorefs_get_md_srcclkena(void)
{
	return spm_read(PCM_REG13_DATA) & (1U << 8);
}

int is_spm_enabled(void)
{
	return 1;
}

void get_spm_reg(char *p)
{
	p += sprintf(p, "%-24s: 0x%08x\n",
			"POWERON_CONFIG_EN",
			spm_read(POWERON_CONFIG_EN));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_SW_FLAG_0",
			spm_read(SPM_SW_FLAG_0));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"SPM_SW_RSV_9",
			spm_read(SPM_SW_RSV_9));
	p += sprintf(p, "%-24s: 0x%08x\n",
			"MD2SPM_DVFS_CON",
			spm_read(MD2SPM_DVFS_CON));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x\n",
			"SPM_DVFS_LEVEL, SPM_DVFS_STA",
			spm_read(SPM_DVFS_LEVEL), spm_read(SPM_DVFS_STA));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"SPM_DVFS_CMD0~4",
			spm_read(SPM_DVFS_CMD0), spm_read(SPM_DVFS_CMD1),
			spm_read(SPM_DVFS_CMD2), spm_read(SPM_DVFS_CMD3),
			spm_read(SPM_DVFS_CMD4));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			"MD_SPM_DVFS_CMD16~19",
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD16),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD17),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD18),
			spm_read(SLEEP_REG_MD_SPM_DVFS_CMD19));
	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n ",
			"SPM_VCORE_DVFS_SHORTCUT",
			spm_read(SPM_VCORE_DVFS_SHORTCUT00),
			spm_read(SPM_VCORE_DVFS_SHORTCUT01),
			spm_read(SPM_VCORE_DVFS_SHORTCUT02),
			spm_read(SPM_VCORE_DVFS_SHORTCUT03));

	p += sprintf(p, "0x%08x, 0x%08x, 0x%08x, 0x%08x\n ",
			spm_read(SPM_VCORE_DVFS_SHORTCUT04),
			spm_read(SPM_VCORE_DVFS_SHORTCUT05),
			spm_read(SPM_VCORE_DVFS_SHORTCUT06),
			spm_read(SPM_VCORE_DVFS_SHORTCUT07));

	p += sprintf(p, "0x%08x, 0x%08x, 0x%08x, 0x%08x\n ",
			spm_read(SPM_VCORE_DVFS_SHORTCUT08),
			spm_read(SPM_VCORE_DVFS_SHORTCUT09),
			spm_read(SPM_VCORE_DVFS_SHORTCUT10),
			spm_read(SPM_VCORE_DVFS_SHORTCUT11));

	p += sprintf(p, "0x%08x, 0x%08x, 0x%08x, 0x%08x\n ",
			spm_read(SPM_VCORE_DVFS_SHORTCUT12),
			spm_read(SPM_VCORE_DVFS_SHORTCUT13),
			spm_read(SPM_VCORE_DVFS_SHORTCUT14),
			spm_read(SPM_VCORE_DVFS_SHORTCUT15));

	p += sprintf(p, "%-24s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n ",
			"SPM_VCORE_DVFS_SHORTCUT_CON0",
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON00),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON01),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON02),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON03));

	p += sprintf(p, "0x%08x, 0x%08x, 0x%08x, 0x%08x\n 0x%08x\n",
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON04),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON05),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON06),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON07),
			spm_read(SPM_VCORE_DVFS_SHORTCUT_CON08));
}


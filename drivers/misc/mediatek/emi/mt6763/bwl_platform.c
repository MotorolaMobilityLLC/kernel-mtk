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

#include <linux/kernel.h>
#include <linux/delay.h>
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>

#include "mt_emi.h"
#include "bwl_platform.h"

#define LAST_EMI_DECS_CTRL	(LAST_EMI_BASE + 0x4)

static unsigned int decs_status;

unsigned int decode_bwl_env(
	unsigned int dram_type, unsigned int ch_num, unsigned int rk_num)
{
	return BWL_ENV_LPDDR4_2CH;
}

unsigned int acquire_bwl_ctrl(void __iomem *LAST_EMI_BASE)
{
	unsigned int timeout;

	decs_status = readl(IOMEM(LAST_EMI_DECS_CTRL));

	mt_reg_sync_writel(0x1, LAST_EMI_DECS_CTRL);
	for (timeout = 1; readl(IOMEM(LAST_EMI_DECS_CTRL)) != 2; timeout++) {
		mdelay(1);
		if ((timeout % 100) == 0) {
			mt_reg_sync_writel(decs_status, LAST_EMI_DECS_CTRL);
			pr_debug("[BWL] switch scenario timeout\n");
			return -1;
		}
	}

	return 0;
}

void release_bwl_ctrl(void __iomem *LAST_EMI_BASE)
{
	mt_reg_sync_writel(decs_status, LAST_EMI_DECS_CTRL);
}

void resume_decs(void __iomem *CEN_EMI_BASE)
{
	writel(0x00FF0000, EMI_BMEN);
	writel(0x00C00003, EMI_MSEL);
	mt_reg_sync_writel(0x00000024, EMI_MSEL2);
}

/* move ELM API here, since ELM is disalbed for QoS */
void dump_last_bm(char *buf, unsigned int leng)
{
}

unsigned int BM_GetBW(void)
{
	void __iomem *CEN_EMI_BASE;

	CEN_EMI_BASE = mt_cen_emi_base_get();

	return readl(IOMEM(EMI_BWCT0));
}

int BM_SetBW(const unsigned int BW_config)
{
	void __iomem *CEN_EMI_BASE;

	CEN_EMI_BASE = mt_cen_emi_base_get();
	mt_reg_sync_writel(BW_config, EMI_BWCT0);
	return 0;
}


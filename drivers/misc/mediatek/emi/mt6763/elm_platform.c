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

#include <linux/platform_device.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>

#include <mtk_dramc.h>
#include "mt_emi.h"

static void __iomem *CEN_EMI_BASE;
static unsigned int elm_valid;
static unsigned int dram_data_rate;
static unsigned int emi_cgma_st0;
static unsigned int emi_cgma_st1;
static unsigned int emi_cgma_st2;
static unsigned int emi_ebmint_st;
static unsigned int emi_ltct0;
static unsigned int emi_ltct1;
static unsigned int emi_ltct2;
static unsigned int emi_ltct3;
static unsigned int emi_ltst0;
static unsigned int emi_ltst1;
static unsigned int emi_ltst2;
static unsigned int emi_bmen;
static unsigned int emi_bmen2;
static unsigned int emi_msel;
static unsigned int emi_msel2;
static unsigned int emi_bmrw0;
static unsigned int emi_bcnt;
static unsigned int emi_wsct;
static unsigned int emi_wsct2;
static unsigned int emi_wsct3;
static unsigned int emi_wsct4;
static unsigned int emi_ttype[16];

void mt_elm_init(void __iomem *elm_base)
{
	elm_valid = 0;
	CEN_EMI_BASE = elm_base;
}

unsigned int disable_emi_dcm(void)
{
	unsigned int emi_dcm_status;
	unsigned int value;

	value = readl(IOMEM(EMI_CONM));
	emi_dcm_status = value & 0xFF000000;
	value &= ~0xFF000000;
	mt_reg_sync_writel(value | 0x40000000, IOMEM(EMI_CONM));

	return emi_dcm_status;
}

void restore_emi_dcm(unsigned int emi_dcm_status)
{
	unsigned int value;

	value = readl(IOMEM(EMI_CONM)) & 0x00FFFFFF;
	mt_reg_sync_writel(value | emi_dcm_status, IOMEM(EMI_CONM));
}

void turn_on_elm(void)
{
	/* BWM */
	mt_reg_sync_writel(
		readl(IOMEM(EMI_BMEN)) | 0x00000001,
		IOMEM(EMI_BMEN));

	/* CGM */
	mt_reg_sync_writel(0x0ffffff0, IOMEM(EMI_CGMA));

	/* ELM */
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) | 0x00000008,
		IOMEM(EMI_LTCT0_2ND));
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) | 0x80000000,
		IOMEM(EMI_LTCT0_2ND));
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) | 0x00000001,
		IOMEM(EMI_LTCT0_2ND));
}

void turn_off_elm(void)
{
	/* BWM */
	mt_reg_sync_writel(
		readl(IOMEM(EMI_BMEN)) & 0xfffffffc,
		IOMEM(EMI_BMEN));

	/* CGM */
	mt_reg_sync_writel(0x0, IOMEM(EMI_CGMA));

	/* ELM */
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) & ~0x00000001,
		IOMEM(EMI_LTCT0_2ND));
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) & ~0x00000008,
		IOMEM(EMI_LTCT0_2ND));
}

void reset_elm(void)
{
	mt_reg_sync_writel(
		readl(IOMEM(EMI_BMEN)) & 0xfffffffc,
		IOMEM(EMI_BMEN));
	mt_reg_sync_writel(
		readl(IOMEM(EMI_LTCT0_2ND)) | 0x80000000,
		IOMEM(EMI_LTCT0_2ND));
	mt_reg_sync_writel(
		readl(IOMEM(EMI_BMEN)) | 0x00000001,
		IOMEM(EMI_BMEN));
}

void dump_elm(char *buf, unsigned int leng)
{
	snprintf(buf, leng,
		"[EMI ELM] ddr data rate: %d, valid: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n%s: 0x%08x\n"
		"%s: 0x%08x\n",
		dram_data_rate, elm_valid,
		"EMI_CGMA_ST0", emi_cgma_st0,
		"EMI_CGMA_ST1", emi_cgma_st1,
		"EMI_CGMA_ST2", emi_cgma_st2,
		"EMI_EBMINT_ST", emi_ebmint_st,
		"EMI_LTCT0", emi_ltct0,
		"EMI_LTCT1", emi_ltct1,
		"EMI_LTCT2", emi_ltct2,
		"EMI_LTCT3", emi_ltct3,
		"EMI_LTST0", emi_ltst0,
		"EMI_LTST1", emi_ltst1,
		"EMI_LTST2", emi_ltst2,
		"EMI_BMEN", emi_bmen,
		"EMI_BMEN2", emi_bmen2,
		"EMI_MSEL", emi_msel,
		"EMI_MSEL2", emi_msel2,
		"EMI_BMRW0", emi_bmrw0,
		"EMI_BCNT", emi_bcnt,
		"EMI_WSCT", emi_wsct,
		"EMI_WSCT2", emi_wsct2,
		"EMI_WSCT3", emi_wsct3,
		"EMI_WSCT4", emi_wsct4,
		"EMI_TTYPE1", emi_ttype[0],
		"EMI_TTYPE2", emi_ttype[1],
		"EMI_TTYPE3", emi_ttype[2],
		"EMI_TTYPE4", emi_ttype[3],
		"EMI_TTYPE5", emi_ttype[4],
		"EMI_TTYPE6", emi_ttype[5],
		"EMI_TTYPE7", emi_ttype[6],
		"EMI_TTYPE8", emi_ttype[7],
		"EMI_TTYPE9", emi_ttype[8],
		"EMI_TTYPE10", emi_ttype[9],
		"EMI_TTYPE11", emi_ttype[10],
		"EMI_TTYPE12", emi_ttype[11],
		"EMI_TTYPE13", emi_ttype[12],
		"EMI_TTYPE14", emi_ttype[13],
		"EMI_TTYPE15", emi_ttype[14],
		"EMI_TTYPE16", emi_ttype[15]
	);
}

void save_debug_reg(void)
{
	int i;

	dram_data_rate = get_dram_data_rate();

	emi_cgma_st0 = readl(IOMEM(EMI_CGMA_ST0));
	emi_cgma_st1 = readl(IOMEM(EMI_CGMA_ST1));
	emi_cgma_st2 = readl(IOMEM(EMI_CGMA_ST2));
	emi_ebmint_st = readl(IOMEM(EMI_EBMINT_ST));

	emi_ltct0 = readl(IOMEM(EMI_LTCT0_2ND));
	emi_ltct1 = readl(IOMEM(EMI_LTCT1_2ND));
	emi_ltct2 = readl(IOMEM(EMI_LTCT2_2ND));
	emi_ltct3 = readl(IOMEM(EMI_LTCT3_2ND));
	emi_ltst0 = readl(IOMEM(EMI_LTST0_2ND));
	emi_ltst1 = readl(IOMEM(EMI_LTST1_2ND));
	emi_ltst2 = readl(IOMEM(EMI_LTST2_2ND));

	emi_bmen = readl(IOMEM(EMI_BMEN));
	emi_bmen2 = readl(IOMEM(EMI_BMEN2));
	emi_msel = readl(IOMEM(EMI_MSEL));
	emi_msel2 = readl(IOMEM(EMI_MSEL2));
	emi_bmrw0 = readl(IOMEM(EMI_BMRW0));
	emi_bcnt = readl(IOMEM(EMI_BCNT));
	emi_wsct = readl(IOMEM(EMI_WSCT));
	emi_wsct2 = readl(IOMEM(EMI_WSCT2));
	emi_wsct3 = readl(IOMEM(EMI_WSCT3));
	emi_wsct4 = readl(IOMEM(EMI_WSCT4));

	for (i = 0; i < 16; i++)
		emi_ttype[i] = readl(IOMEM(EMI_TTYPE(i)));

	elm_valid = 1;
}

unsigned int BM_GetBW(void)
{
	return readl(IOMEM(EMI_BWCT0));
}

int BM_SetBW(const unsigned int BW_config)
{
	mt_reg_sync_writel(BW_config, EMI_BWCT0);
	return 0;
}


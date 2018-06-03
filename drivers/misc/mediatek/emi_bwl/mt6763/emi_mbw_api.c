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
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
#include <linux/of_address.h>
#include "emi_mbw.h"
#include "emi_bwl.h"

static void __iomem *CEN_EMI_BASE; /* not initialise statics to 0 or NULL */

void BM_Init(void)
{
	CEN_EMI_BASE = mt_cen_emi_base_get();
}

void BM_DeInit(void)
{
}

void BM_Enable(const unsigned int enable)
{
	const unsigned int value = readl(IOMEM(EMI_BMEN));

	mt_reg_sync_writel((value & ~(BUS_MON_PAUSE | BUS_MON_EN)) |
		(enable ? BUS_MON_EN : 0), EMI_BMEN);
}

void BM_Pause(void)
{
	const unsigned int value = readl(IOMEM(EMI_BMEN));

	mt_reg_sync_writel(value | BUS_MON_PAUSE, EMI_BMEN);
}

void BM_Continue(void)
{
	const unsigned int value = readl(IOMEM(EMI_BMEN));

	mt_reg_sync_writel(value & (~BUS_MON_PAUSE), EMI_BMEN);
}

unsigned int BM_IsOverrun(void)
{
	/*
	 * return 0 if EMI_BCNT(bus cycle counts)
	 * or EMI_WACT(total word counts) is overrun,
	 * otherwise return an !0 value
	 */
	const unsigned int value = readl(IOMEM(EMI_BMEN));

	return value & BC_OVERRUN;
}

void BM_SetReadWriteType(const unsigned int ReadWriteType)
{
	const unsigned int value = readl(IOMEM(EMI_BMEN));

	/*
	 * ReadWriteType: 00/11 --> both R/W
	 *                   01 --> only R
	 *                   10 --> only W
	 */
	mt_reg_sync_writel((value & 0xFFFFFFCF) |
		(ReadWriteType << 4), EMI_BMEN);
}

int BM_GetBusCycCount(void)
{
	return BM_IsOverrun() ? BM_ERR_OVERRUN : readl(IOMEM(EMI_BCNT));
}

unsigned int BM_GetTransAllCount(void)
{
	return readl(IOMEM(EMI_TACT));
}

int BM_GetTransCount(const unsigned int counter_num)
{
	unsigned int iCount;

	switch (counter_num) {
	case 1:
		iCount = readl(IOMEM(EMI_TSCT));
		break;

	case 2:
		iCount = readl(IOMEM(EMI_TSCT2));
		break;

	case 3:
		iCount = readl(IOMEM(EMI_TSCT3));
		break;

	default:
		return BM_ERR_WRONG_REQ;
	}

	return iCount;
}

long long BM_GetWordAllCount(void)
{
	unsigned int word_all_count;

	word_all_count = readl(IOMEM(EMI_WACT));

	if (BM_IsOverrun() && (word_all_count == 0xFFFFFFFF))
		return BM_ERR_OVERRUN;
	else
		return word_all_count;
}

int BM_GetWordCount(const unsigned int counter_num)
{
	unsigned int iCount;

	switch (counter_num) {
	case 1:
		iCount = readl(IOMEM(EMI_WSCT));
		break;

	case 2:
		iCount = readl(IOMEM(EMI_WSCT2));
		break;

	case 3:
		iCount = readl(IOMEM(EMI_WSCT3));
		break;

	case 4:
		iCount = readl(IOMEM(EMI_WSCT4));
		break;

	default:
		return BM_ERR_WRONG_REQ;
	}

	return iCount;
}

unsigned int BM_GetBandwidthWordCount(void)
{
	return readl(IOMEM(EMI_BACT));
}

unsigned int BM_GetOverheadWordCount(void)
{
	return readl(IOMEM(EMI_BSCT));
}

int BM_GetTransTypeCount(const unsigned int counter_num)
{
	return (counter_num < 1	|| counter_num > BM_COUNTER_MAX) ?
	BM_ERR_WRONG_REQ : readl(IOMEM(EMI_TTYPE1 + (counter_num - 1) * 8));
}

int BM_SetMonitorCounter(const unsigned int counter_num,
	const unsigned int master, const unsigned int trans_type)
{
	unsigned int value;
	unsigned long addr;
	const unsigned int iMask = 0xFFFF;

	if (counter_num < 1 || counter_num > BM_COUNTER_MAX)
		return BM_ERR_WRONG_REQ;

	if (counter_num == 1) {
		addr = (unsigned long) EMI_BMEN;
		value = (readl(IOMEM(addr)) & ~(iMask << 16)) |
		((trans_type & 0xFF) << 24) | ((master & 0xFF) << 16);
	} else {
		addr = (counter_num <= 3) ? (unsigned long) EMI_MSEL :
			((unsigned long) EMI_MSEL2 + (counter_num / 2 - 2) * 8);

		/* clear master and transaction type fields */
		value =
		readl(IOMEM(addr)) & ~(iMask << ((counter_num % 2) * 16));

		/* set master and transaction type fields */
		value |= (((trans_type & 0xFF) << 8) |
		(master & 0xFF)) << ((counter_num % 2) * 16);
	}

	mt_reg_sync_writel(value, addr);

	return BM_REQ_OK;
}

int BM_SetMaster(const unsigned int counter_num, const unsigned int master)
{
	unsigned int value;
	unsigned long addr;
	const unsigned int iMask = 0xFF;

	if (counter_num < 1 || counter_num > BM_COUNTER_MAX)
		return BM_ERR_WRONG_REQ;

	if (counter_num == 1) {
		addr = (unsigned long) EMI_BMEN;
		value = (readl(IOMEM(addr)) & ~(iMask << 16)) |
		((master & iMask) << 16);
	} else {
		addr = (counter_num <= 3) ? (unsigned long) EMI_MSEL :
			((unsigned long) EMI_MSEL2 + (counter_num / 2 - 2) * 8);

		/* clear master and transaction type fields */
		value =
		readl(IOMEM(addr)) & ~(iMask << ((counter_num % 2) * 16));

		/* set master and transaction type fields */
		value |= ((master & iMask) << ((counter_num % 2) * 16));
	}

	mt_reg_sync_writel(value, addr);

	return BM_REQ_OK;
}

int BM_SetIDSelect(const unsigned int counter_num,
	const unsigned int id, const unsigned int enable)
{
	unsigned int value, shift_num;
	unsigned long addr;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX)
	    || (id > 0x1FFF) || (enable > 1))
		return BM_ERR_WRONG_REQ;

	addr = (unsigned long) EMI_BMID0 + ((counter_num - 1) / 2) * 4;

	/* field's offset in the target EMI_BMIDx register */
	shift_num = ((counter_num - 1) % 2) * 16;

	/* clear SELx_ID field */
	value = readl(IOMEM(addr)) & ~(0x1FFF << shift_num);

	/* set SELx_ID field */
	value |= id << shift_num;

	mt_reg_sync_writel(value, addr);

	value = (readl(IOMEM(EMI_BMEN2)) & ~(1 << (counter_num - 1))) |
					(enable << (counter_num - 1));

	mt_reg_sync_writel(value, EMI_BMEN2);

	return BM_REQ_OK;
}

int BM_SetUltraHighFilter(const unsigned int counter_num,
		const unsigned int enable)
{
	unsigned int value;

	if ((counter_num < 1 || counter_num > BM_COUNTER_MAX)
	    || (enable > 1)) {
		return BM_ERR_WRONG_REQ;
	}

	value = (readl(IOMEM(EMI_BMEN1)) & ~(1 << (counter_num - 1))) |
					(enable << (counter_num - 1));

	mt_reg_sync_writel(value, EMI_BMEN1);

	return BM_REQ_OK;
}

int BM_SetLatencyCounter(void)
{
	unsigned int value;

	value = readl(IOMEM(EMI_BMEN2)) & ~(0x3 << 24);
	/* emi_ttype1 -- emi_ttype8 change as total latencies for m0 -- m7,
	 * and emi_ttype9 -- emi_ttype16 change
	 * as total transaction counts for m0 -- m7
	 */
	value |= (0x2 << 24);
	mt_reg_sync_writel(value, EMI_BMEN2);
	return BM_REQ_OK;
}

int BM_GetLatencyCycle(const unsigned int counter_num)
{
	unsigned int cycle_count;

	switch (counter_num) {
	case 1:
		cycle_count = readl(IOMEM(EMI_TTYPE1));
		break;
	case 2:
		cycle_count = readl(IOMEM(EMI_TTYPE2));
		break;
	case 3:
		cycle_count = readl(IOMEM(EMI_TTYPE3));
		break;
	case 4:
		cycle_count = readl(IOMEM(EMI_TTYPE4));
		break;
	case 5:
		cycle_count = readl(IOMEM(EMI_TTYPE5));
		break;
	case 6:
		cycle_count = readl(IOMEM(EMI_TTYPE6));
		break;
	case 7:
		cycle_count = readl(IOMEM(EMI_TTYPE7));
		break;
	case 8:
		cycle_count = readl(IOMEM(EMI_TTYPE7));
		break;
	case 9:
		cycle_count = readl(IOMEM(EMI_TTYPE9));
		break;
	case 10:
		cycle_count = readl(IOMEM(EMI_TTYPE10));
		break;
	case 11:
		cycle_count = readl(IOMEM(EMI_TTYPE11));
		break;
	case 12:
		cycle_count = readl(IOMEM(EMI_TTYPE12));
		break;
	case 13:
		cycle_count = readl(IOMEM(EMI_TTYPE13));
		break;
	case 14:
		cycle_count = readl(IOMEM(EMI_TTYPE14));
		break;
	case 15:
		cycle_count = readl(IOMEM(EMI_TTYPE15));
		break;
	case 16:
		cycle_count = readl(IOMEM(EMI_TTYPE16));
		break;
	default:
		return BM_ERR_WRONG_REQ;
	}
	return cycle_count;
}

int BM_GetEmiDcm(void)
{
	return readl(IOMEM(EMI_CONM)) >> 24;
}

int BM_SetEmiDcm(const unsigned int setting)
{
	unsigned int value;

	value = readl(IOMEM(EMI_CONM));
	mt_reg_sync_writel((value & 0x00FFFFFF) | (setting << 24), EMI_CONM);

	return BM_REQ_OK;
}

unsigned int BM_GetBWST(void)
{
	return readl(IOMEM(EMI_BWST0));
}

unsigned int BM_GetBWST2(void)
{
	return readl(IOMEM(EMI_BWST_2ND));
}

unsigned int BM_GetBWST3(void)
{
	return readl(IOMEM(EMI_BWST_3RD));
}

unsigned int BM_GetBWST4(void)
{
	return readl(IOMEM(EMI_BWST_4TH));
}

int BM_SetBW(const unsigned int BW_config)
{
	mt_reg_sync_writel(BW_config, EMI_BWCT0);
	return BM_REQ_OK;
}

int BM_SetBW2(const unsigned int BW_config)
{
	mt_reg_sync_writel(BW_config, EMI_BWCT0_2ND);
	return BM_REQ_OK;
}

int BM_SetBW3(const unsigned int BW_config)
{
	mt_reg_sync_writel(BW_config, EMI_BWCT0_3RD);
	return BM_REQ_OK;
}

int BM_SetBW4(const unsigned int BW_config)
{
	mt_reg_sync_writel(BW_config, EMI_BWCT0_4TH);
	return BM_REQ_OK;
}

unsigned int BM_GetBW(void)
{
	return readl(IOMEM(EMI_BWCT0));
}

unsigned int BM_GetBW2(void)
{
	return readl(IOMEM(EMI_BWCT0_2ND));
}

unsigned int BM_GetBW3(void)
{
	return readl(IOMEM(EMI_BWCT0_3RD));
}

unsigned int BM_GetBW4(void)
{
	return readl(IOMEM(EMI_BWCT0_4TH));
}

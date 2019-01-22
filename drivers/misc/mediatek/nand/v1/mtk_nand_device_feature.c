/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include "mtk_nand.h"
#include <linux/io.h>
#include "mtk_nand_util.h"
#include "mtk_nand_device_feature.h"

#if (defined(CONFIG_MTK_MLC_NAND_SUPPORT) || defined(CONFIG_MTK_TLC_NAND_SUPPORT))
static bool MLC_DEVICE = TRUE;
#endif

u16 randomizer_seed[128] = {
	0x576A, 0x05E8, 0x629D, 0x45A3,
	0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70,
	0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5,
	0x7809, 0x2521, 0x48F4, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969,
	0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8,
	0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6,
	0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602,
	0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C,
	0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3,
	0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1,
	0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8,
	0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2,
	0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF,
	0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951,
	0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74,
	0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49,
	0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

rrtryFunctionType rtyFuncArray[] = {
#ifdef SUPPORT_MICRON_DEVICE
	mtk_nand_micron_rrtry,
#endif
#ifdef SUPPORT_SANDISK_DEVICE
	mtk_nand_sandisk_rrtry,
	mtk_nand_sandisk_19nm_rrtry,
	mtk_nand_sandisk_tlc_1ynm_rrtry,
#endif
#ifdef SUPPORT_HYNIX_DEVICE
	mtk_nand_hynix_rrtry,
	mtk_nand_hynix_16nm_rrtry,
#endif
#ifdef SUPPORT_TOSHIBA_DEVICE
	mtk_nand_toshiba_rrtry,
	mtk_nand_toshiba_tlc_rrtry
#endif
};

TransferPageNumber fsFuncArray[] = {
#ifdef SUPPORT_MICRON_DEVICE
	micron_pairpage_mapping,
#endif
#ifdef SUPPORT_HYNIX_DEVICE
	hynix_pairpage_mapping,
#endif
#ifdef SUPPORT_SANDISK_DEVICE
	sandisk_pairpage_mapping,
#endif
#ifdef SUPPORT_TOSHIBA_DEVICE
	toshiba_pairpage_mapping,
#endif
};

GetLowPageNumber functArray[] = {
#ifdef SUPPORT_MICRON_DEVICE
	MICRON_TRANSFER,
#endif
#ifdef SUPPORT_HYNIX_DEVICE
	HYNIX_TRANSFER,
#endif
#ifdef SUPPORT_SANDISK_DEVICE
	SANDISK_TRANSFER,
#endif
#ifdef SUPPORT_TOSHIBA_DEVICE
	TOSHIBA_TRANSFER,
#endif

};


/* read retry related function*/
u32 mtk_nand_rrtry_setting(flashdev_info deviceinfo, enum readRetryType type, u32 retryStart,
				  u32 loopNo)
{
	return retryStart + loopNo;
}

void mtk_nand_rrtry_func(struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
				bool defValue)
{
	if (MLC_DEVICE)
		rtyFuncArray[deviceinfo.feature_set.FeatureSet.rtype] (mtd, deviceinfo, feature,
									defValue);
}


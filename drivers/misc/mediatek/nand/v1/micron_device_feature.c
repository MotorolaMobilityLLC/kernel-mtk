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

#ifdef SUPPORT_MICRON_DEVICE

u32 micron_pairpage_mapping(u32 page, bool high_to_low)
{
	u32 offset;

	if (high_to_low == TRUE) {
		/*Micron 256pages */
		if ((page < 4) || (page > 251))
			return page;

		offset = page % 4;
		if (offset == 0 || offset == 1)
			return page;
		else
			return page - 6;

	} else {
		if ((page == 2) || (page == 3) || (page > 247))
			return page;

		offset = page % 4;
		if (offset == 0 || offset == 1)
			return page + 6;
		else
			return page;
	}
}

u32 MICRON_TRANSFER(u32 pageNo)
{
	u32 temp;

	if (pageNo < 4)
		return pageNo;
	temp = (pageNo - 4) & 0xFFFFFFFE;
	if (pageNo <= 130)
		return pageNo + temp;
	return pageNo + temp - 2;
}

void mtk_nand_micron_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
				  bool defValue)
{
	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,
				deviceinfo.feature_set.FeatureSet.readRetryAddress,
								(u8 *)&feature, 4);
}
EXPORT_SYMBOL(mtk_nand_micron_rrtry);

#endif


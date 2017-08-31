/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_IOMMU_H_
#define _MTK_IOMMU_H_

#ifdef CONFIG_MTK_IOMMU
unsigned long mtk_get_pgt_base(void);

#else
static unsigned long mtk_get_pgt_base(void)
{
	return 0;
}

#endif

#endif

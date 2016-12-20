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

#ifndef __MTK_BIF_INTF_H
#define __MTK_BIF_INTF_H

#ifdef CONFIG_MTK_BIF_SUPPORT
#include <linux/errno.h>

extern int mtk_bif_init(void);
extern int mtk_bif_get_vbat(unsigned int *vbat);
extern int mtk_bif_get_tbat(int *tbat);
extern bool mtk_bif_is_hw_exist(void);

#else /* Not CONFIG_MTK_BIF_SUPPORT */

static inline int mtk_bif_init(void)
{
	return -ENOTSUPP;
}

static inline int mtk_bif_get_vbat(unsigned int *vbat)
{
	*vbat = 0;
	return -ENOTSUPP;
}

static inline int mtk_bif_get_tbat(int *tbat)
{
	*tbat = 0;
	return -ENOTSUPP;
}

static inline bool mtk_bif_is_hw_exist(void)
{
	return false;
}

#endif /* CONFIG_MTK_BIF_SUPPORT */
#endif /* __MTK_BIF_INTF_H */

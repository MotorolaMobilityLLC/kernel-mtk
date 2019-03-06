/*
 * Copyright (C) 2019 MediaTek Inc.
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

#ifndef __MTK_PE_50_INTF_H
#define __MTK_PE_50_INTF_H

#include <mt-plat/prop_chgalgo_class.h>

struct mtk_pe50 {
	struct prop_chgalgo_device *pca_algo;
	bool online;
};

#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_50_SUPPORT
extern int mtk_pe50_init(struct charger_manager *chgmgr);
extern bool mtk_pe50_is_ready(struct charger_manager *chgmgr);
extern int mtk_pe50_start(struct charger_manager *chgmgr);
extern bool mtk_pe50_is_running(struct charger_manager *chgmgr);
extern int mtk_pe50_plugout_reset(struct charger_manager *chgmgr);
#else
static inline int mtk_pe50_init(struct charger_manager *chgmgr)
{
	return -ENOTSUPP;
}

static inline int mtk_pe50_is_ready(struct charger_manager *chgmgr)
{
	return -ENOTSUPP;
}

static inline int mtk_pe50_start(struct charger_manager *chgmgr)
{
	return -ENOTSUPP;
}

static inline bool mtk_pe50_is_running(struct charger_manager *chgmgr)
{
	return false;
}

static inline int mtk_pe50_plugout_reset(struct charger_manager *chgmgr)
{
	return -ENOTSUPP;
}
#endif /* CONFIG_MTK_PUMP_EXPRESS_PLUS_50_SUPPORT */
#endif /* __MTK_PE_50_INTF_H */

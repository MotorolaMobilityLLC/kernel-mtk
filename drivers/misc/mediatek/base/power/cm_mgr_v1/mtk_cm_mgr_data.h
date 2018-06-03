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

#ifndef __MTK_CM_MGR_DATA_H__
#define __MTK_CM_MGR_DATA_H__

#if defined(CONFIG_MACH_MT6758)
#include <mtk_cm_mgr_mt6758_data.h>
#elif defined(CONFIG_MACH_MT6771)
#include <mtk_cm_mgr_mt6771_data.h>
#elif defined(CONFIG_MACH_MT6775)
#include <mtk_cm_mgr_mt6771_data.h>
#endif

#endif	/* __MTK_CM_MGR_DATA_H__ */

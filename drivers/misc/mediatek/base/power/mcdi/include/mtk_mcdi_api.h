/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_MCDI_COMMON_H__
#define __MTK_MCDI_COMMON_H__

#if defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6758)

#include "mcdi_v1/mtk_mcdi_api.h"

#elif defined(CONFIG_MACH_MT6739)

#include "mcdi_v2/mtk_mcdi_api.h"

#endif

#endif /* __MTK_MCDI_COMMON_H__ */

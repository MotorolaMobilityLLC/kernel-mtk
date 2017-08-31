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

#ifndef __MTK_SPM_RESOURCE_REQ_H__
#define __MTK_SPM_RESOURCE_REQ_H__

/* SPM resource request APIs: public */

#if defined(CONFIG_MACH_MT6757)

#elif defined(CONFIG_MACH_KIBOPLUS)

#include "spm_v2/mtk_spm_resource_req.h"

#elif defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)

#include "spm_v3/mtk_spm_resource_req.h"

#endif

#endif /* __MTK_SPM_RESOURCE_REQ_H__ */

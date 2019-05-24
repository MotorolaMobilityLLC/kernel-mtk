/*
 * Copyright (C) 2019 MediaTek Inc.
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


#ifndef _MTK_SECURE_API_H_
#define _MTK_SECURE_API_H_

#if defined(CONFIG_MACH_MT6739) \
	|| defined(CONFIG_MACH_MT6771) \
	|| defined(CONFIG_MACH_MT6763)
#include "mtk_secure_api_v1.3.h"
#elif defined(CONFIG_MACH_MT6761) \
	|| defined(CONFIG_MACH_MT6765) \
	|| defined(CONFIG_MACH_MT6779) \
	|| defined(CONFIG_MACH_MT8173)
#include "mtk_secure_api_v1.4.h"
#elif defined(CONFIG_MACH_MT6768)
#include "mtk_secure_api_v1.6.h"
#else
#error "no matching ATF version!"
#endif

#endif /* _MTK_SECURE_API_H_ */

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

#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H

#ifdef CONFIG_MTK_CLKMGR
#ifdef CONFIG_ARCH_MT6757M
#include "mach/mt_clkmgr2.h"
#elif defined(CONFIG_ARCH_MT6753)
#include "mach/mt_clkmgr3.h"
#else /* CONFIG_ARCH_MT6757 */
#include "mach/mt_clkmgr1_legacy.h"
#endif
#else /* !CONFIG_MTK_CLKMGR */
#ifdef CONFIG_ARCH_MT6757M
#error "Does not support common clock framework"
#elif defined(CONFIG_ARCH_MT6753)
#error "Does not support common clock framework"
#else /* CONFIG_ARCH_MT6757 */
#include "mach/mt_clkmgr1.h"
#endif
#endif

#endif

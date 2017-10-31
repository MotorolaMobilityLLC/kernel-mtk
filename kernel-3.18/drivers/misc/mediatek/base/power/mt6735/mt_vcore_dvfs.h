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

#ifndef _MT_VCORE_DVFS_
#define _MT_VCORE_DVFS_

#if defined(CONFIG_ARCH_MT6735)
#include "mt_vcore_dvfs_1.h"
#endif

#if defined(CONFIG_ARCH_MT6735M)
#include "mt_vcore_dvfs_2.h"
#endif

#if defined(CONFIG_ARCH_MT6753)
#include "mt_vcore_dvfs_3.h"
#endif

#endif


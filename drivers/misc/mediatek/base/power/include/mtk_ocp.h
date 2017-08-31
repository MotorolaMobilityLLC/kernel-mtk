/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef _MTK_OCP_H_
#define _MTK_OCP_H_

#if defined(CONFIG_MACH_MT6759)

#include "ocp_v1/mtk_ocp.h"

#elif defined(CONFIG_MACH_MT6799)

#include "../mt6799/mtk_ocp.h"

#else

#error NO corresponding project of mtk_ocp.h header file can be found!!!

#endif

#endif /* _MTK_OCP_H_ */


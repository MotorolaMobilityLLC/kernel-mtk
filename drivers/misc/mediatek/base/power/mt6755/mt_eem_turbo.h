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

#ifndef _MT_EEM_TURBO_
#define _MT_EEM_TURBO_

#ifdef __KERNEL__
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353) && defined(CONFIG_MTK_MT6750TT)
	extern unsigned int *get_turbo(unsigned int binLevel, unsigned int binLevelEng);
#endif
#endif
#endif

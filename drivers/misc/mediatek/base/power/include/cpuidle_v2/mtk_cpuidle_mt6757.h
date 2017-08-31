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

#ifndef __MTK_CPUIDLE_MT6799_H__
#define __MTK_CPUIDLE_MT6799_H__

enum {
	KP_IRQ_NR = 0,
	CONN_WDT_IRQ_NR,
	LOWBATTERY_IRQ_NR,
	MD1_WDT_IRQ_NR,
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	C2K_WDT_IRQ_NR,
#endif
#endif
	IRQ_NR_MAX
};

#endif

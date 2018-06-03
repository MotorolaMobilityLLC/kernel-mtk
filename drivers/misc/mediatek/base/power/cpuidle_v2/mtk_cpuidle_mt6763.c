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

#include "mtk_cpuidle.h"
#include "mtk_spm.h"

char *irq_match[] = {
	"mediatek,kp",
	"mediatek,consys",
	"mediatek,mdcldma",
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	"mediatek,ap2c2k_ccif",
#endif
#endif
	""
};

unsigned int irq_nr[IRQ_NR_MAX];

int wake_src_irq[] = {
	WAKE_SRC_R12_KP_IRQ_B,
	WAKE_SRC_R12_CONN2AP_WDT_IRQ_B,
	WAKE_SRC_R12_MD1_WDT_B,
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	WAKE_SRC_R12_C2K_WDT_IRQ_B,
#endif
#endif
	0
};

int irq_offset[] = {
	0,
	0,
	2,
#ifdef CONFIG_MTK_MD3_SUPPORT
#if CONFIG_MTK_MD3_SUPPORT /* Using this to check >0 */
	1,
#endif
#endif
	0
};


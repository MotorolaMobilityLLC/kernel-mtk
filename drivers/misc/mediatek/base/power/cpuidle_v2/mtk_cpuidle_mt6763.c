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
	"mediatek,mt6763-consys",
	"mediatek,mdcldma",
	""
};

unsigned int irq_nr[IRQ_NR_MAX];

int wake_src_irq[] = {
	WAKE_SRC_R12_KP_IRQ_B,
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B,
	WAKE_SRC_R12_MD1_WDT_B,
	0
};

int irq_offset[] = {
	0,
	1,
	3,
	0
};


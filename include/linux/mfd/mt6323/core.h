/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MFD_MT6323_CORE_H__
#define __MFD_MT6323_CORE_H__

enum mt6323_irq_numbers {
	MT6323_IRQ_SPKL_AB = 0,
	MT6323_IRQ_SPKR_AB,
	MT6323_IRQ_SPKL,
	MT6323_IRQ_SPKR,
	MT6323_IRQ_BAT_L,
	MT6323_IRQ_BAT_H,
	MT6323_IRQ_FG_BAT_L,
	MT6323_IRQ_FG_BAT_H,
	MT6323_IRQ_WATCHDOG,
	MT6323_IRQ_PWRKEY,
	MT6323_IRQ_THR_L,
	MT6323_IRQ_THR_H,
	MT6323_IRQ_VBATON_UNDET,
	MT6323_IRQ_BVALID_DET,
	MT6323_IRQ_CHRDET,
	MT6323_IRQ_OV,
	MT6323_IRQ_LDO,
	MT6323_IRQ_HOMEKEY,
	MT6323_IRQ_ACCDET,
	MT6323_IRQ_AUDIO,
	MT6323_IRQ_RTC,
	MT6323_IRQ_PWRKEY_RSTB,
	MT6323_IRQ_HDMI_SIFM,
	MT6323_IRQ_HDMI_CEC,
	MT6323_IRQ_VCA15,
	MT6323_IRQ_VSRMCA15,
	MT6323_IRQ_VCORE,
	MT6323_IRQ_VGPU,
	MT6323_IRQ_VIO18,
	MT6323_IRQ_VPCA7,
	MT6323_IRQ_VSRMCA7,
	MT6323_IRQ_VDRM,
	MT6323_IRQ_NR,
};

struct mt6323_chip {
	struct device *dev;
	struct regmap *regmap;
	int irq;
	struct irq_domain *irq_domain;
	struct mutex irqlock;
	u16 irq_masks_cur[2];
	u16 irq_masks_cache[2];
};

#endif /* __MFD_MT6323_CORE_H__ */

/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu <flora.fu@mediatek.com>
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

#ifndef __MFD_MT6397_CORE_H__
#define __MFD_MT6397_CORE_H__

enum PMIC_INT_CON_GRP {
	GRP_INT_CON0 = 0,
	GRP_INT_CON1,
	MT6397_IRQ_GROUP_NR,
};

enum PMIC_INT_STATUS_GRP {
	GRP_INT_STATUS0 = 0,
	GRP_INT_STATUS1,
	MT6397_IRQ_STATUS_GROUP_NR,
};

enum PMIC_INT_STATUS {
	RG_INT_STATUS_SPKL_AB = 0,
	RG_INT_STATUS_SPKR_AB,
	RG_INT_STATUS_SPKL,
	RG_INT_STATUS_SPKR,
	RG_INT_STATUS_BAT_L,
	RG_INT_STATUS_BAT_H,
	RG_INT_STATUS_FG_BAT_L,
	RG_INT_STATUS_FG_BAT_H,
	RG_INT_STATUS_WATCHDOG,
	RG_INT_STATUS_PWRKEY,
	RG_INT_STATUS_THR_L,
	RG_INT_STATUS_THR_H,
	RG_INT_STATUS_VBATON_UNDET,
	RG_INT_STATUS_BVALID_DET,
	RG_INT_STATUS_CHRDET,
	RG_INT_STATUS_OV,
	RG_INT_STATUS_LDO,
	RG_INT_STATUS_HOMEKEY,
	RG_INT_STATUS_ACCDET,
	RG_INT_STATUS_AUDIO,
	RG_INT_STATUS_RTC,
	RG_INT_STATUS_PWRKEY_RSTB,
	RG_INT_STATUS_HDMI_SIFM,
	RG_INT_STATUS_HDMI_CEC,
	RG_INT_STATUS_VCA15,
	RG_INT_STATUS_VSRMCA15,
	RG_INT_STATUS_VCORE,
	RG_INT_STATUS_VGPU,
	RG_INT_STATUS_VIO18,
	RG_INT_STATUS_VPCA7,
	RG_INT_STATUS_VSRMCA7,
	RG_INT_STATUS_VDRM,
	MT6397_IRQ_NR,
};

struct mt6397_chip {
	struct device *dev;
	struct regmap *regmap;
	struct irq_domain *irq_domain;
	struct mutex irqlock;
	int irq;
	u16 irq_masks_cur[MT6397_IRQ_GROUP_NR];
	u16 irq_masks_cache[MT6397_IRQ_GROUP_NR];
};

#endif /* __MFD_MT6397_CORE_H__ */

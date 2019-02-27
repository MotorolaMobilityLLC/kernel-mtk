/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __HELIO_DVFSRC_MT6765_H
#define __HELIO_DVFSRC_MT6765_H

#define PMIC_VCORE_ADDR         PMIC_RG_BUCK_VCORE_VOSEL
#define VCORE_BASE_UV           500000
#define VCORE_STEP_UV           6250
#define VCORE_INVALID           0x80

/* DVFSRC_BASIC_CONTROL 0x0 */
#define DVFSRC_EN_SHIFT		0
#define DVFSRC_EN_MASK		0x1
#define DVFSRC_OUT_EN_SHIFT	8
#define DVFSRC_OUT_EN_MASK	0x1
#define FORCE_EN_CUR_SHIFT	14
#define FORCE_EN_CUR_MASK	0x1
#define FORCE_EN_TAR_SHIFT	15
#define FORCE_EN_TAR_MASK	0x1

/* DVFSRC_SW_REQ 0x4 */
#define EMI_SW_AP_SHIFT		0
#define EMI_SW_AP_MASK		0x3

/* DVFSRC_VCORE_REQUEST2 0x4C */
#define VCORE_QOS_GEAR0_SHIFT	24
#define VCORE_QOS_GEAR0_MASK	0x3

/* DVFSRC_LEVEL 0xDC */
#define CURRENT_LEVEL_SHIFT	16
#define CURRENT_LEVEL_MASK	0xFFFF

#endif /* __HELIO_DVFSRC_MT6765_H */

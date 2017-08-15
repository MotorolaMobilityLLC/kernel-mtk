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

#ifndef __MTK_MFG_REG_H__
#define __MTK_MFG_REG_H__

/* MFG reg */
#define MFG_DEBUG_SEL   0x180
#define MFG_DEBUG_A 0x184
#define MFG_DEBUG_IDEL	(1 << 2)

<<<<<<< HEAD   (c29b7d [ALPS02724800] GPU: GPU requests VCore DVFS OPP)
=======
/* MFG Power Monitor */
#define MFG_PERF_EN_00 0x3C0
#define MFG_PERF_EN_01 0x3C4
#define MFG_PERF_EN_02 0x3C8
#define MFG_PERF_EN_03 0x3CC
#define MFG_PERF_EN_04 0x3D0
#define MFG_1to2_CFG_CON_00 0x8F0
#define MFG_1to2_CFG_CON_01 0x8F4

>>>>>>> BRANCH (19e0ff [ALPS02724800] GPU: Add AXI1to2 setting)
extern volatile void *g_MFG_base;

#define base_write32(addr, value)	writel(value, addr)
#define base_read32(addr)			readl(addr)
#define MFG_write32(addr, value)	base_write32(g_MFG_base+addr, value)
#define MFG_read32(addr)			base_read32(g_MFG_base+addr)

#endif /* __MTK_MFG_REG_H__ */

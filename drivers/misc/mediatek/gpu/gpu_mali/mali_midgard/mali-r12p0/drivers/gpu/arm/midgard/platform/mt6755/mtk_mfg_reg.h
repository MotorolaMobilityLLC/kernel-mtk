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

#ifndef __MTK_MFG_REG_H__
#define __MTK_MFG_REG_H__

/* MT6755 MFG reg */
#define MFG_DEBUG_SEL   0x180
#define MFG_DEBUG_A 0x184
#define MFG_BUS_IDLE_BIT	(1 << 2)

#define MFG_WRITE32(value, addr)	writel(value, addr)
#define MFG_READ32(addr)	readl(addr)
#define MFG_DEBUG_CTRL_REG(value)	((value) + MFG_DEBUG_SEL)
#define MFG_DEBUG_STAT_REG(value)	((value) + MFG_DEBUG_A)

#endif /* __MTK_MFG_REG_H__ */
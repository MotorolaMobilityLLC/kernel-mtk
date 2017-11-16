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

#ifndef __HW_WATCHPOINT_INTERNAL_H__
#define __HW_WATCHPOINT_INTERNAL_H__

extern void __iomem **MT_HW_DBG_BASE;

#define DBGWVR_BASE(i)		(MT_HW_DBG_BASE[i]+0x0180)
#define DBGWCR_BASE(i)		(MT_HW_DBG_BASE[i]+0x01C0)
#define DBGLAR(i)		(MT_HW_DBG_BASE[i]+0x0FB0)
#define DBGDSCR(i)		(MT_HW_DBG_BASE[i]+0x0088)
#define DBGWFAR(i)		(MT_HW_DBG_BASE[i]+0x0018)
#define DBGOSLAR(i)		(MT_HW_DBG_BASE[i]+0x0300)
#define DBGOSSAR(i)		(MT_HW_DBG_BASE[i]+0x0304)
#define DBGBVR_BASE(i)		(MT_HW_DBG_BASE[i]+0x0100)
#define DBGBCR_BASE(i)		(MT_HW_DBG_BASE[i]+0x0140)

#define UNLOCK_KEY		(0xC5ACCE55)
#define HDBGEN			(1 << 14)
#define MDBGEN			(1 << 15)
#define DBGWCR_VAL		(0x000001E7)
#define WP_EN			(1 << 0)
#define LSC_LDR			(1 << 3)
#define LSC_STR			(2 << 3)
#define LSC_ALL			(3 << 3)

extern unsigned int hw_watchpoint_nr(void);
#endif

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

#ifndef __MT_FDE_AES_H__
#define __MT_FDE_AES_H__

#include <linux/spinlock.h>
#include <linux/types.h>

#define FDE_AES_DBG		0

#define FDE_TAG					"[FDE] "
#if FDE_AES_DBG
#define FDEERR(fmt, arg...)		pr_err(FDE_TAG "[error @ %s:%d] " fmt, __func__, __LINE__, ##arg)
#define FDELOG(fmt, arg...)		pr_err(FDE_TAG fmt, ##arg)
#else
#define FDEERR(fmt, arg...)		pr_err(FDE_TAG fmt, ##arg)
#define FDELOG(fmt, arg...)		{}
#endif

#define FDE_OK                              (0)
#define FDE_EAGAIN                          (EAGAIN) /* Try again : 11 */
#define FDE_ENODEV							(ENODEV) /* No such device : 19 */
#define FDE_EINVAL                          (EINVAL) /* Invalid argument : 22 */

#define FDE_MSDC0	0 /* eMMC */
#define FDE_MSDC1	1 /* SD */

/******************************************register operation***********************************/
/* REGISTER */
#define	CONTEXT_SEL			(FDE_AES_CORE_BASE + 0x000)
#define	CONTEXT_WORD0		(FDE_AES_CORE_BASE + 0x004)
#define	CONTEXT_WORD1		(FDE_AES_CORE_BASE + 0x008)
#define	CONTEXT_WORD3		(FDE_AES_CORE_BASE + 0x00C)
#define	REG_COM_A			(FDE_AES_CORE_BASE + 0x020)
#define	REG_COM_B			(FDE_AES_CORE_BASE + 0x024)
#define	REG_COM_C			(FDE_AES_CORE_BASE + 0x028)
#define	REG_COM_D			(FDE_AES_CORE_BASE + 0x100)
#define	REG_COM_E			(FDE_AES_CORE_BASE + 0x104)
#define	REG_COM_F			(FDE_AES_CORE_BASE + 0x108)
#define	REG_COM_G			(FDE_AES_CORE_BASE + 0x10C)
#define	REG_COM_H			(FDE_AES_CORE_BASE + 0x110)
#define	REG_COM_I			(FDE_AES_CORE_BASE + 0x114)
#define	REG_COM_J			(FDE_AES_CORE_BASE + 0x118)
#define	REG_COM_K			(FDE_AES_CORE_BASE + 0x11C)
#define	REG_COM_L			(FDE_AES_CORE_BASE + 0x200)
#define	REG_COM_M			(FDE_AES_CORE_BASE + 0x204)
#define	REG_COM_N			(FDE_AES_CORE_BASE + 0x208)
#define	REG_COM_O			(FDE_AES_CORE_BASE + 0x20C)
#define	REG_COM_P			(FDE_AES_CORE_BASE + 0x210)
#define	REG_COM_Q			(FDE_AES_CORE_BASE + 0x214)
#define	REG_COM_R			(FDE_AES_CORE_BASE + 0x218)
#define	REG_COM_S			(FDE_AES_CORE_BASE + 0x21C)
#define	REG_COM_T			(FDE_AES_CORE_BASE + 0x300)
#define	REG_COM_U			(FDE_AES_CORE_BASE + 0x400)

#define FDE_READ32(addr)       (*(volatile u32*)(addr))
#define FDE_WRITE32(addr, val) (*(volatile u32*)(addr) = (val))

#define FDE_READ16(addr)       (*(volatile u16*)(addr))
#define FDE_WRITE16(addr, val) (*(volatile u16*)(addr) = (val))

#define FDE_READ8(addr)        (*(volatile u8*)(addr))
#define FDE_WRITE8(addr, val)  (*(volatile u8*)(addr) = (val))

/***********************************end of register operation****************************************/

/***********************************FDE Param********************************************************/
typedef struct mt_fde_aes_t {
	/* CONTEXT_SEL 0x0 */
	u8						context_id;
	u8						context_bp;
	u8						sector_size; /* 1:512B 2:1024B 3:2048B 4:4096B */
	u8						reserve;
	/* CONTEXT_WORD1 0x8 */
	u32						sector_offset_L;
	u32						sector_offset_H;
	void __iomem            *base;
	spinlock_t              lock;
	u32						status;		/* Enable/Disable */
	u32						test_case;
	u32						hw_crypto;
} mt_fde_aes_context;

/* ============================================================================== */
/* FDE_AES Exported Function */
/* ============================================================================== */
void __iomem *fde_aes_get_base(void);
u32 fde_aes_get_hw(void);
void fde_aes_set_case(u32 test_case);
u32 fde_aes_get_case(void);

s32 fde_aes_check_enable(s32 dev_num, u8 bEnable);
s32 fde_aes_exec(s32 dev_num, u32 blkcnt, u32 opcode);
s32 fde_aes_done(s32 dev_num, u32 blkcnt, u32 opcode);

#endif /* __MT_FDE_AES_H__ */

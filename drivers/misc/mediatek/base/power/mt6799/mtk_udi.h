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
/*
 * @file    mtk_udi.h
 * @brief   Driver for UDI interface define
 *
 */
#ifndef _MT_UDI_H
#define _MT_UDI_H

#ifdef __KERNEL__
#ifdef __MT_UDI_C__
#include <mt-plat/mtk_secure_api.h>
#endif
#endif

#ifdef __KERNEL__
#ifdef __MT_UDI_C__
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define mt_secure_call_udi	mt_secure_call
#else
/* This is workaround for idvfs use */
static noinline int mt_secure_call_udi(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	register u64 reg0 __asm__("x0") = function_id;
	register u64 reg1 __asm__("x1") = arg0;
	register u64 reg2 __asm__("x2") = arg1;
	register u64 reg3 __asm__("x3") = arg2;
	int ret = 0;

	asm volatile ("smc    #0\n" : "+r" (reg0) :
		"r"(reg1), "r"(reg2), "r"(reg3));

	ret = (int)reg0;
	return ret;
}
#endif
#endif
#endif

/* define for UDI service */
#ifdef CONFIG_ARM64
#define MTK_SIP_KERNEL_UDI_WRITE		0xC20003D0 /* MTK_SIP_KERNEL_OCP_WRITE */
#define MTK_SIP_KERNEL_UDI_READ			0xC20003D1 /* MTK_SIP_KERNEL_OCP_READ */
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK	0xC20003A0
#define MTK_SIP_KERNEL_UDI_BIT_CTRL		0xC20003A1
#else
#define MTK_SIP_KERNEL_UDI_WRITE		0x820003D0 /* MTK_SIP_KERNEL_OCP_WRITE */
#define MTK_SIP_KERNEL_UDI_READ			0x820003D1 /* MTK_SIP_KERNEL_OCP_READ */
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK	0x820003A0
#define MTK_SIP_KERNEL_UDI_BIT_CTRL		0x820003A1
#endif

/*
 * bit operation
 */
#define BIT_UDI(bit)		(1U << (bit))

#define MSB_UDI(range)	(1 ? range)
#define LSB_UDI(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r: Range in the form of MSB:LSB
 */
#define BITMASK_UDI(r) \
	(((unsigned) -1 >> (31 - MSB_UDI(r))) & ~((1U << LSB_UDI(r)) - 1))

#define GET_BITS_VAL_UDI(_bits_, _val_) \
	(((_val_) & (BITMASK_UDI(_bits_))) >> ((0) ? _bits_))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS_UDI(r, val)	((val << LSB_UDI(r)) & BITMASK_UDI(r))

/* dfine for UDI register service */
#ifdef __KERNEL__
#define udi_reg_read(addr)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_READ, addr, 0, 0)
#define udi_reg_write(addr, val)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_WRITE, addr, val, 0)
#define udi_reg_field(addr, range, val)	\
				udi_reg_write(addr, (udi_reg_read(addr) & ~(BITMASK_UDI(range))) | BITS_UDI(range, val))

#define udi_jtag_clock(sw_tck, i_trst, i_tms, i_tdi, count)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_JTAG_CLOCK,	\
									(((1 << (sw_tck & 0x03)) << 3) |	\
									((i_trst & 0x01) << 2) |	\
									((i_tms & 0x01) << 1) |	\
									(i_tdi & 0x01)),	\
									count, 0)

#define udi_bit_ctrl(sw_tck, i_tdi, i_tms, i_trst)	\
				mt_secure_call_udi(MTK_SIP_KERNEL_UDI_BIT_CTRL,	\
									((sw_tck & 0x0f) << 3) |	\
									((i_trst & 0x01) << 2) |	\
									((i_tms & 0x01) << 1) |	\
									(i_tdi & 0x01), 0, 0)

#else
#define udi_reg_read(addr)	\
				ptp3_reg_read(addr)
#define udi_reg_write(addr, val)	\
				ptp3_reg_write(addr, val)
#define udi_jtag_clock(sw_tck, i_trst, i_tms, i_tdi, count)	\
				UDIRead((((1 << (sw_tck & 0x03)) << 3) |	\
						((i_trst & 0x01) << 2) |	\
						((i_tms & 0x01) << 1) |	\
						(i_tdi & 0x01)),	\
						count)
#endif
/* UDI Extern Function */
/* UDI_EXTERN unsigned int mt_udi_get_level(void); */
/* UDI_EXTERN void mt_udi_lock(unsigned long *flags); */
/* UDI_EXTERN void mt_udi_unlock(unsigned long *flags); */
/* UDI_EXTERN int mt_udi_idle_can_enter(void); */
/* UDI_EXTERN int mt_udi_status(ptp_det_id id); */

#ifdef CONFIG_ARM64
/* UDI_EXTERN int get_udi_status(void); */
/* UDI_EXTERN unsigned int get_vcore_ptp_volt(int uv); */
#endif /* CONFIG_ARM64 */

/* #define UDI_FIFOSIZE 16384  */
/* recv string for temp  */
#define UDI_FIFOSIZE 256

/* udi extern function for ATF test */
#ifndef __KERNEL__
extern unsigned int IR_bit_count, DR_bit_count;
extern unsigned char IR_byte[UDI_FIFOSIZE], DR_byte[UDI_FIFOSIZE];
extern unsigned int jtag_sw_tck;	/* default debug channel = 1 */

extern int udi_jtag_clock_read(void);
#endif /* __KERNEL__ */

#endif /* _MT_UDI_H */

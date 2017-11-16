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


#ifndef _MT_TRUSTY_API_H_
#define _MT_TRUSTY_API_H_

#define SMC_ARG0		"r0"
#define SMC_ARG1		"r1"
#define SMC_ARG2		"r2"
#define SMC_ARG3		"r3"
#define SMC_ARCH_EXTENSION	".arch_extension sec\n"
#define SMC_REGISTERS_TRASHED	"ip"

#define SMC_NR(entity, fn, fastcall, smc64) ((((fastcall)&0x1) << 31) | \
					     (((smc64) & 0x1) << 30) | \
					     (((entity) & 0x3F) << 24) | \
					     ((fn) & 0xFFFF) \
					    )

#define SMC_FASTCALL_NR(entity, fn)	SMC_NR((entity), (fn), 1, 0)

#define	SMC_ENTITY_SIP			2	/* SIP Service calls */

#define SMC_FC_CPU_ON			SMC_FASTCALL_NR(SMC_ENTITY_SIP, 0)
#define SMC_FC_CPU_DORMANT		SMC_FASTCALL_NR(SMC_ENTITY_SIP, 1)
#define SMC_FC_CPU_DORMANT_CANCEL	SMC_FASTCALL_NR(SMC_ENTITY_SIP, 2)
#define SMC_FC_CPU_OFF			SMC_FASTCALL_NR(SMC_ENTITY_SIP, 3)
#define SMC_FC_CPU_ERRATA_802022	SMC_FASTCALL_NR(SMC_ENTITY_SIP, 4)

static inline ulong mt_trusty_call(ulong r0, ulong r1, ulong r2, ulong r3)
{
	register ulong _r0 asm(SMC_ARG0) = r0;
	register ulong _r1 asm(SMC_ARG1) = r1;
	register ulong _r2 asm(SMC_ARG2) = r2;
	register ulong _r3 asm(SMC_ARG3) = r3;

	asm volatile(
		__asmeq("%0", SMC_ARG0)
		__asmeq("%1", SMC_ARG1)
		__asmeq("%2", SMC_ARG2)
		__asmeq("%3", SMC_ARG3)
		__asmeq("%4", SMC_ARG0)
		__asmeq("%5", SMC_ARG1)
		__asmeq("%6", SMC_ARG2)
		__asmeq("%7", SMC_ARG3)
		SMC_ARCH_EXTENSION
		"smc	#0"	/* switch to secure world */
		: "=r" (_r0), "=r" (_r1), "=r" (_r2), "=r" (_r3)
		: "r" (_r0), "r" (_r1), "r" (_r2), "r" (_r3)
		: SMC_REGISTERS_TRASHED);
	return _r0;
}


#endif /* _MT_TRUSTY_API_H_ */


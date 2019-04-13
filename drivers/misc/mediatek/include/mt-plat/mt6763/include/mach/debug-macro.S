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

@#include <asm/arch/hardware.h>

	.macro  addruart, rp, rv, tmp
	mov	\rp, #0x2000
	movt	\rp, #0x1100
	mov	\rv, #0x2000
	movt	\rv, #0xF100
	.endm

	.macro  senduart,rd,rx
	strb	\rd, [\rx, #0x0]
	.endm

	.macro  waituart,rd,rx
1001:	ldr	\rd, [\rx, #0x14]	@ UART0 status
	tst	\rd, #0x40		@ write allow
	beq	1001b
	.endm

	.macro  busyuart,rd,rx
1002:   ldr	\rd, [\rx, #0x14]	@ UART0 status
	tst	\rd, #0x40		@ uart full .eq. busy
	beq	1002b
	.endm

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

	.global write_cntpctl
write_cntpctl:
	.func
	mcr	p15, 0, r0, c14, c2, 1
	bx	lr
	.endfunc

	.global read_cntpctl
read_cntpctl:
	.func
	mrc	p15, 0, r0, c14, c2, 1
	bx	lr
	.endfunc

	.global read_cpu_id
read_cpu_id:
	.func
	mrc	p15, 0, r0, c0, c0, 5
	ands	r0, r0, #0xff
	bx	lr
	.endfunc

	.global read_cluster_id
read_cluster_id:
	.func
	mrc	p15, 0, r0, c0, c0, 5
	lsr	r0, r0, #0x8
	ands	r0, r0, #0xff
	bx	lr
	.endfunc

	.global mt_save_generic_timer
mt_save_generic_timer:
	.func
	mrc	p15, 0, r2, c14, c2, 1	@ read CNTP_CTL
	mrc	p15, 0, r3, c14, c2, 0	@ read CNTP_TVAL
	mrc	p15, 0, r12, c14, c1, 0	@ read CNTKCTL
	stm	r0!, {r2, r3, r12}
	bx	lr
	.endfunc

	.global mt_restore_generic_timer
mt_restore_generic_timer:
	.func
	ldm	r0!, {r2, r3, r12}
	mcr	p15, 0, r3, c14, c2, 0	@ write CNTP_TVAL
	mcr	p15, 0, r12, c14, c1, 0	@ write CNTKCTL
	mcr	p15, 0, r2, c14, c2, 1	@ write CNTP_CTL
	bx	lr
	.endfunc

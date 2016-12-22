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


	.text

	.global __switch_to_smp
	.global __switch_to_amp

.equ ACTLR_SMP_BIT ,0x00000040

__switch_to_smp:
	MRRC	p15,1,r0,r1,c15
	ORR	r0,r0,#ACTLR_SMP_BIT
	MCRR	p15,1,r0,r1,c15
	BX	lr

__switch_to_amp:
	MRRC	p15,1,r0,r1,c15
	BIC	r0,r0,#ACTLR_SMP_BIT
	MCRR	p15,1,r0,r1,c15
	BX	lr

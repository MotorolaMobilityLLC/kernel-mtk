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

#if !defined(__MRDUMP_PRIVATE_H__)
#define __MRDUMP_PRIVATE_H__

struct mrdump_platform {
	void (*hw_enable)(bool enabled);
	void (*reboot)(void);
};

struct pt_regs;

extern struct mrdump_control_block mrdump_cblock;

void mrdump_cblock_init(void);

int mrdump_platform_init(const struct mrdump_platform *plat);

void mrdump_save_current_backtrace(struct pt_regs *regs);

extern void __disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2(void);
extern void __inner_flush_dcache_all(void);
extern void __disable_dcache(void);

static inline void mrdump_mini_save_regs(struct pt_regs *regs)
{
#ifdef __aarch64__
	__asm__ volatile ("stp	x0, x1, [sp,#-16]!\n\t"
			  "1: mov	x1, %0\n\t"
			  "add	x0, x1, #16\n\t"
			  "stp	x2, x3, [x0],#16\n\t"
			  "stp	x4, x5, [x0],#16\n\t"
			  "stp	x6, x7, [x0],#16\n\t"
			  "stp	x8, x9, [x0],#16\n\t"
			  "stp	x10, x11, [x0],#16\n\t"
			  "stp	x12, x13, [x0],#16\n\t"
			  "stp	x14, x15, [x0],#16\n\t"
			  "stp	x16, x17, [x0],#16\n\t"
			  "stp	x18, x19, [x0],#16\n\t"
			  "stp	x20, x21, [x0],#16\n\t"
			  "stp	x22, x23, [x0],#16\n\t"
			  "stp	x24, x25, [x0],#16\n\t"
			  "stp	x26, x27, [x0],#16\n\t"
			  "ldr	x1, [x29]\n\t"
			  "stp	x28, x1, [x0],#16\n\t"
			  "mov	x1, sp\n\t"
			  "stp	x30, x1, [x0],#16\n\t"
			  "mrs	x1, daif\n\t"
			  "adr	x30, 1b\n\t"
			  "stp	x30, x1, [x0],#16\n\t"
			  "sub	x1, x0, #272\n\t"
			  "ldr	x0, [sp]\n\t"
			  "str	x0, [x1]\n\t"
			  "ldr	x0, [sp, #8]\n\t"
			  "str	x0, [x1, #8]\n\t" "ldp	x0, x1, [sp],#16\n\t" :  : "r" (regs) : "cc");
#else
	asm volatile ("stmia %1, {r0 - r15}\n\t" "mrs %0, cpsr\n":"=r" (regs->uregs[16]) : "r"(regs) : "memory");
#endif
}
#endif /* __MRDUMP_PRIVATE_H__ */

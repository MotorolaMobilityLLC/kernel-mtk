/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT_PWM_PRV_H__
#define __MT_PWM_PRV_H__

#define INREG32(reg)		__raw_readl((void *)reg)
#define OUTREG32(reg, val)	mt_reg_sync_writel(val, reg)
#define SETREG32(reg, val)	OUTREG32(reg, INREG32(reg) | (val))
#define CLRREG32(reg, val)	OUTREG32(reg, INREG32(reg) & ~(val))
#define MASKREG32(x, y, z)	OUTREG32(x, (INREG32(x) & ~(y)) | (z))

#ifdef CONFIG_OF
extern void __iomem *pwm_base;
/* unsigned int pwm_irqnr; */

#define PWM_BASE pwm_base
#endif

/***********************************
* PWM register address             *
************************************/
#define PWM_ENABLE	(PWM_BASE + 0x0000)

#define PWM_3DLCM	(PWM_BASE + 0x1D0)
#define PWM_3DLCM_ENABLE_OFFSET	0

#define PWM_INT_ENABLE	(PWM_BASE + 0x0200)
#define PWM_INT_STATUS	(PWM_BASE + 0x0204)
#define PWM_INT_ACK	(PWM_BASE + 0x0208)
#define PWM_EN_STATUS	(PWM_BASE + 0x020c)

#define PWM_CK_26M_SEL	(PWM_BASE + 0x0210)
#define PWM_CK_26M_SEL_OFFSET	0

/* PWM control registers */
#define PWM_CON_CLKDIV_MASK		0x00000007
#define PWM_CON_CLKDIV_OFFSET		0
#define PWM_CON_CLKSEL_MASK		0x00000008
#define PWM_CON_CLKSEL_OFFSET		3
#define PWM_CON_CLKSEL_OLD_MASK		0x00000010
#define PWM_CON_CLKSEL_OLD_OFFSET	4

#define PWM_CON_SRCSEL_MASK		0x00000020
#define PWM_CON_SRCSEL_OFFSET		5

#define PWM_CON_MODE_MASK		0x00000040
#define PWM_CON_MODE_OFFSET		6

#define PWM_CON_IDLE_VALUE_MASK		0x00000080
#define PWM_CON_IDLE_VALUE_OFFSET	7

#define PWM_CON_GUARD_VALUE_MASK	0x00000100
#define PWM_CON_GUARD_VALUE_OFFSET	8

#define PWM_CON_STOP_BITS_MASK		0x00007E00
#define PWM_CON_STOP_BITS_OFFSET	9
#define PWM_CON_OLD_MODE_MASK		0x00008000
#define PWM_CON_OLD_MODE_OFFSET		15

#define BLOCK_CLK	(66UL * 1000 * 1000)
#define PWM_26M_CLK	(26UL * 1000 * 1000)

#endif

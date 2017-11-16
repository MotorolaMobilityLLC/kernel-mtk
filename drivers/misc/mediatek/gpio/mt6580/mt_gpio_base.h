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

#ifndef _MT_GPIO_BASE_H_
#define _MT_GPIO_BASE_H_

#include "mt-plat/sync_write.h"
#include <mach/gpio_const.h>

#define GPIO_WR32(addr, data)   mt_reg_sync_writel(data, addr)
#define GPIO_RD32(addr)         __raw_readl(addr)
/* #define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT)) */
/* #define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT))) */
#define GPIO_SW_SET_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT, REG)   GPIO_WR32(REG, (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) & ~((unsigned long)(BIT)))


/*----------------------------------------------------------------------------*/
typedef struct {
	unsigned int val;
} VAL_REGS_4;
typedef struct {
	unsigned int val;
	unsigned int set;
	unsigned int rst;
} VAL_REGS_12;
typedef struct {         /*FIXME: check GPIO spec*/
	unsigned int val;
	unsigned int set;
	unsigned int rst;
	unsigned int _align1;
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS_12    dir[3];             /*0x0000 ~ 0x0023: 36  bytes*/
	VAL_REGS_12    dout[3];            /*0x0024 ~ 0x0047: 36  bytes*/
	VAL_REGS_4     din[3];             /*0x0048 ~ 0x0053: 12  bytes*/
	VAL_REGS       mode[12];           /*0x0054 ~ 0x0113: 192 bytes*/
} GPIO_REGS;

#endif /* _MT_GPIO_BASE_H_ */

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
#ifndef _MT_GPIO_BASE_H_
#define _MT_GPIO_BASE_H_

#include "mt-plat/sync_write.h"
#include <mach/gpio_const.h>

/* #define SELF_TEST
#define GPIO_BRINGUP */


#define GPIO_WR32(addr, data)   mt_reg_sync_writel(data, addr)
#define GPIO_RD32(addr)         __raw_readl(addr)

/* #define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT)) */
/* #define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT))) */
#define GPIO_SW_SET_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT, REG)   GPIO_WR32(REG, (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) & ~((unsigned long)(BIT)))


/*----------------------------------------------------------------------------*/
typedef struct {		/*FIXME: check GPIO spec */
	unsigned int val;
	unsigned int set;
	unsigned int clr;
	unsigned int rst;
} VAL_REGS;


/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS dir[7];	/*0x0000 ~ 0x006F: 112 bytes */
	unsigned char rsv00[144];	/*0x00E0 ~ 0x00FF: 32 bytes */
	VAL_REGS dout[7];	/*0x0400 ~ 0x04DF: 224 bytes */
	unsigned char rsv04[144];	/*0x04B0 ~ 0x04FF: 32 bytes */
	VAL_REGS din[7];	/*0x0500 ~ 0x05DF: 224 bytes */
	unsigned char rsv05[144];	/*0x05E0 ~ 0x05FF:      32 bytes */
	VAL_REGS mode[21];	/*0x0600 ~ 0x08AF: 688 bytes */

} GPIO_REGS;

#ifdef SELF_TEST
void mt_gpio_self_test_base(void);
#endif

#endif				/* _MT_GPIO_BASE_H_ */

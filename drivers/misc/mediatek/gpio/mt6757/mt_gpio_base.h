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

#define GPIO_WR32(addr, data)   mt_reg_sync_writel(data, (unsigned long)addr)
#define GPIO_RD32(addr)         __raw_readl(addr)
/* #define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT)) */
/* #define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT))) */
#define GPIO_SW_SET_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) | ((BIT)))
#define GPIO_SET_BITS(BIT, REG)   GPIO_WR32(REG, (BIT))
#define GPIO_CLR_BITS(BIT, REG)   GPIO_WR32(REG, GPIO_RD32(REG) & ~((BIT)))


#define IOCFG_WR32(addr, data)   mt_reg_sync_writel(data, (IOCFG_BASE + (unsigned long)addr))
#define IOCFG_RD32(addr)         __raw_readl((IOCFG_BASE + (unsigned long)addr))
#define IOCFG_SW_SET_BITS(BIT, REG)   IOCFG_WR32(REG, IOCFG_RD32(REG) | ((unsigned long)(BIT)))
#define IOCFG_SET_BITS(BIT, REG)   IOCFG_WR32(REG, (unsigned long)(BIT))
#define IOCFG_CLR_BITS(BIT, REG)   IOCFG_WR32(REG, IOCFG_RD32(REG) & ~((unsigned long)(BIT)))

/*----------------------------------------------------------------------------*/
typedef struct {		/*FIXME: check GPIO spec */
	unsigned int val;
	unsigned int set;
	unsigned int rst;
	unsigned int _align1;
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS dir[7];	/*0x0000 ~ 0x006F: 112 bytes */
	u8 rsv00[144];		/*0x0070 ~ 0x00FF: 144 bytes */
	VAL_REGS dout[7];	/*0x0100 ~ 0x016F: 112 bytes */
	u8 rsv01[144];		/*0x0170 ~ 0x01FF: 144 bytes */
	VAL_REGS din[7];	/*0x0200 ~ 0x026F: 112 bytes */
	u8 rsv02[144];		/*0x0270 ~ 0x02FF: 144 bytes */
	VAL_REGS mode[25];	/*0x0300 ~ 0x048F: 400 bytes */
} GPIO_REGS;

#ifdef SELF_TEST
void mt_gpio_self_test_base(void);
#endif

#define MAX_GPIO_PIN MT_GPIO_BASE_MAX

extern struct device_node *get_gpio_np(void);
extern struct device_node *get_iocfg_np(void);
#endif				/* _MT_GPIO_BASE_H_ */

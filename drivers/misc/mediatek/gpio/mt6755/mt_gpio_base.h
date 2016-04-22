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

/*Marvin's USB request to direct access GPIO base*/
#define USB_WR32(addr, data)   mt_reg_sync_writel(data, (GPIO_BASE + (unsigned long)addr))
#define USB_RD32(addr)         __raw_readl((GPIO_BASE + (unsigned long)addr))

/*----------------------------------------------------------------------------*/
typedef struct {		/*FIXME: check GPIO spec */
	unsigned int val;
	unsigned int set;
	unsigned int rst;
	unsigned int _align1;
} VAL_REGS;
/*----------------------------------------------------------------------------*/
typedef struct {
	VAL_REGS dir[6];	/*0x0000 ~ 0x005F: 96  bytes */
	u8 rsv00[160];		/*0x0060 ~ 0x00FF: 160 bytes */
	VAL_REGS dout[6];	/*0x0100 ~ 0x015F: 96  bytes */
	u8 rsv01[160];		/*0x0160 ~ 0x01FF: 160 bytes */
	VAL_REGS din[6];	/*0x0200 ~ 0x025F: 96  bytes */
	u8 rsv02[160];		/*0x0260 ~ 0x02FF: 160 bytes */
	VAL_REGS mode[19];	/*0x0300 ~ 0x042F: 384 bytes */
} GPIO_REGS;
#define MAX_GPIO_PIN MT_GPIO_BASE_MAX

extern struct device_node *get_gpio_np(void);
extern struct device_node *get_iocfg_np(void);
#endif				/* _MT_GPIO_BASE_H_ */

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

#include <linux/io.h>
#include <mach/gpio_const.h>
/*----------------------------------------------------------------------------*/
struct VAL_REGS {
	unsigned int val;
	unsigned int set;
	unsigned int rst;
	unsigned int _align1;
};
/*----------------------------------------------------------------------------*/
struct GPIO_REGS {
	struct VAL_REGS dir[8];         /*0x0000 ~ 0x007F: 96  bytes */
	u8 rsv00[128];                  /*0x0080 ~ 0x00FF: 128 bytes */
	struct VAL_REGS dout[8];        /*0x0100 ~ 0x017F: 96  bytes */
	u8 rsv01[128];                  /*0x0180 ~ 0x01FF: 128 bytes */
	struct VAL_REGS din[8];          /*0x0200 ~ 0x027F: 96  bytes */
	u8 rsv02[128];                  /*0x0280 ~ 0x02FF: 128 bytes */
	struct VAL_REGS mode[29];       /*0x0300 ~ 0x04CF: 464 bytes */
};

#ifdef SELF_TEST
void mt_gpio_self_test_base(void);
#endif

#define MAX_GPIO_PIN MT_GPIO_BASE_MAX

extern struct device_node *get_gpio_np(void);
extern struct device_node *get_iocfg_np(void);
#endif				/* _MT_GPIO_BASE_H_ */

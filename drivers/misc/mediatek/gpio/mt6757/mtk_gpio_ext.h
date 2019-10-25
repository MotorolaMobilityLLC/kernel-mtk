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

#ifndef _MT_GPIO_EXT_H_
#define _MT_GPIO_EXT_H_

#include <mach/mtk_pmic_wrap.h>

#define GPIOEXT_WR(addr, data) pwrap_write((unsigned long)addr, data)
#define GPIOEXT_RD(addr)                                                       \
	({                                                                     \
		unsigned long ext_data;                                        \
		(pwrap_read((unsigned long)addr, &ext_data) != 0) ? -1         \
								  : ext_data;  \
	})
#define GPIOEXT_SET_BITS(BIT, REG) (GPIOEXT_WR(REG, (unsigned long)(BIT)))
#define GPIOEXT_CLR_BITS(BIT, REG)                                             \
	({                                                                     \
		unsigned long ext_data;                                        \
		int ret;                                                       \
		ret = GPIOEXT_RD(REG);                                         \
		ext_data = ret;                                                \
		(ret < 0)                                                      \
		    ? -1                                                       \
		    : (GPIOEXT_WR(REG, ext_data & ~((unsigned long)(BIT))))    \
	})

/*----------------------------------------------------------------------------*/
struct struct_EXT_VAL_REGS {
	unsigned short val;
	unsigned short set;
	unsigned short rst;
	unsigned short _align;
};

#define EXT_VAL_REGS struct struct_EXT_VAL_REGS
/*----------------------------------------------------------------------------*/
struct struct_GPIOEXT_REGS {
	EXT_VAL_REGS dir[4];     /*0x0000 ~ 0x001F: 32 bytes */
	EXT_VAL_REGS pullen[4];  /*0x0020 ~ 0x003F: 32 bytes */
	EXT_VAL_REGS pullsel[4]; /*0x0040 ~ 0x005F: 32 bytes */
	EXT_VAL_REGS dinv[4];    /*0x0060 ~ 0x007F: 32 bytes */
	EXT_VAL_REGS dout[4];    /*0x0080 ~ 0x009F: 32 bytes */
	EXT_VAL_REGS din[4];     /*0x00A0 ~ 0x00BF: 32 bytes */
	EXT_VAL_REGS mode[10];   /*0x00C0 ~ 0x010F: 80 bytes */
};

#define GPIOEXT_REGS struct struct_GPIOEXT_REGS
#endif /*_MT_GPIO_EXT_H_*/

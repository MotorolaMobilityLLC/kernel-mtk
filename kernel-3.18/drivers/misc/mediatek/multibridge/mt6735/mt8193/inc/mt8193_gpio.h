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

#ifndef MT8193_GPIO_H
#define MT8193_GPIO_H

#include <generated/autoconf.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/irqs.h>

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <asm/tlbflush.h>
#include <asm/page.h>
#include <linux/slab.h>

#include <linux/module.h>
#include "mt8193_pinmux.h"


#define MT8193_GPIO_OUTPUT      1
#define MT8193_GPIO_INPUT       0
#define MT8193_GPIO_HIGH        1
#define MT8193_GPIO_LOW         0

enum MT8193_GPIO_PIN {
	GPIO_PIN_UNSUPPORTED = -1,

	GPIO_PIN_GPIO0,
	GPIO_PIN_GPIO1,
	GPIO_PIN_GPIO2,
	GPIO_PIN_GPIO3,
	GPIO_PIN_GPIO4,
	GPIO_PIN_GPIO5,
	GPIO_PIN_GPIO6,
	GPIO_PIN_GPIO7,
	GPIO_PIN_GPIO8,
	GPIO_PIN_GPIO9,
	GPIO_PIN_GPIO10,
	GPIO_PIN_GPIO11,
	GPIO_PIN_GPIO12,
	GPIO_PIN_GPIO13,
	GPIO_PIN_GPIO14,
	GPIO_PIN_GPIO15,
	GPIO_PIN_GPIO16,
	GPIO_PIN_GPIO17,
	GPIO_PIN_GPIO18,
	GPIO_PIN_GPIO19,
	GPIO_PIN_GPIO20,
	GPIO_PIN_GPIO21,
	GPIO_PIN_GPIO22,
	GPIO_PIN_GPIO23,
	GPIO_PIN_GPIO24,
	GPIO_PIN_GPIO25,
	GPIO_PIN_GPIO26,
	GPIO_PIN_GPIO27,
	GPIO_PIN_GPIO28,
	GPIO_PIN_GPIO29,
	GPIO_PIN_GPIO30,
	GPIO_PIN_GPIO31,
	GPIO_PIN_GPIO32,
	GPIO_PIN_GPIO33,
	GPIO_PIN_GPIO34,
	GPIO_PIN_GPIO35,
	GPIO_PIN_GPIO36,
	GPIO_PIN_GPIO37,
	GPIO_PIN_GPIO38,
	GPIO_PIN_GPIO39,
	GPIO_PIN_GPIO40,
	GPIO_PIN_GPIO41,
	GPIO_PIN_GPIO42,
	GPIO_PIN_GPIO43,
	GPIO_PIN_GPIO44,
	GPIO_PIN_GPIO45,
	GPIO_PIN_GPIO46,
	GPIO_PIN_GPIO47,
	GPIO_PIN_GPIO48,
	GPIO_PIN_GPIO49,
	GPIO_PIN_GPIO50,
	GPIO_PIN_GPIO51,
	GPIO_PIN_GPIO52,
	GPIO_PIN_GPIO53,
	GPIO_PIN_GPIO54,
	GPIO_PIN_GPIO55,
	GPIO_PIN_GPIO56,
	GPIO_PIN_GPIO57,
	GPIO_PIN_GPIO58,
	GPIO_PIN_GPIO59,
	GPIO_PIN_GPIO60,
	GPIO_PIN_GPIO61,
	GPIO_PIN_GPIO62,
	GPIO_PIN_GPIO63,
	GPIO_PIN_GPIO64,
	GPIO_PIN_GPIO65,
	GPIO_PIN_GPIO66,
	GPIO_PIN_GPIO67,
	GPIO_PIN_GPIO68,
	GPIO_PIN_GPIO69,
	GPIO_PIN_GPIO70,
	GPIO_PIN_GPIO71,

	GPIO_PIN_MAX
};


/* _au4GpioTbl[gpio num] is pad num */
static const u32 _au4GpioTbl[GPIO_PIN_MAX] = {
	PIN_NFD6,    /* gpio0 */
	PIN_NFD5,
	PIN_NFD4,
	PIN_NFD3,
	PIN_NFD2,
	PIN_NFD1,
	PIN_NFD0,    /* gpio6 */
	PIN_G0,       /* gpio 7 */
	PIN_B5,
	PIN_B4,
	PIN_B3,
	PIN_B2,
	PIN_B1,
	PIN_B0,
	PIN_DE,
	PIN_VCLK,
	PIN_HSYNC,
	PIN_VSYNC,
	PIN_CEC,
	PIN_HDMISCK,
	PIN_HDMISD,
	PIN_HTPLG,
	PIN_I2S_DATA,
	PIN_I2S_LRCK,
	PIN_I2S_BCK,
	PIN_DPI1CK,
	PIN_DPI1D7,
	PIN_DPI1D6,
	PIN_DPI1D5,
	PIN_DPI1D4,
	PIN_DPI1D3,
	PIN_DPI1D2,
	PIN_DPI1D1,
	PIN_DPI1D0,
	PIN_DPI0CK,
	PIN_DPI0HSYNC,
	PIN_DPI0VSYNC,
	PIN_DPI0D11,
	PIN_DPI0D10,
	PIN_DPI0D9,
	PIN_DPI0D8,
	PIN_DPI0D7,
	PIN_DPI0D6,
	PIN_DPI0D5,
	PIN_DPI0D4,
	PIN_DPI0D3,
	PIN_DPI0D2,
	PIN_DPI0D1,
	PIN_DPI0D0,
	PIN_SDA,
	PIN_SCL,
	PIN_NRNB,
	PIN_NCLE,
	PIN_NALE,
	PIN_NWEB,
	PIN_NREB,
	PIN_NLD7,
	PIN_NLD6,
	PIN_NLD5,
	PIN_NLD4,
	PIN_NLD3,
	PIN_NLD2,
	PIN_NLD1,
	PIN_NLD0,             /* gpio 63 */
	PIN_INT,              /* gpio 64*/
	PIN_NFRBN,            /* gpio 65*/
	PIN_NFCLE,
	PIN_NFALE,
	PIN_NFWEN,
	PIN_NFREN,
	PIN_NFCEN,
	PIN_NFD7,            /* gpio 71 */
};

extern int GPIO_Config(u32 u4GpioNum, u8 u1Mode, u8 u1Value);
extern u8 GPIO_Input(u32 i4GpioNum);
extern int GPIO_Output(u32 u4GpioNum, u32 u4High);

#endif /* MT8193_H */

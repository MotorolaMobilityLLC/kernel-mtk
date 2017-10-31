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

#ifndef MT8193_H
#define MT8193_H


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


#include "mt8193_iic.h"

#define CKGEN_BASE                       0x1000


#define REG_RW_BUS_CKCFG                 0x000
#define CLK_BUS_SEL_XTAL                 0
#define CLK_BUS_SEL_NFIPLL_D2                 1
#define CLK_BUS_SEL_NFIPLL_D3                 2
#define CLK_BUS_SEL_XTAL_D2                 3
#define CLK_BUS_SEL_32K                4
#define CLK_BUS_SEL_PAD_DPI0                 5
#define CLK_BUS_SEL_PAD_DPI1                 6
#define CLK_BUS_SEL_ROSC                 7


#define REG_RW_NFI_CKCFG                 0x008
#define CLK_NFI_SEL_NFIPLL               0
#define CLK_NFI_SEL_NFIPLL_D2            1
#define CLK_NFI_SEL_NFIPLL_D3            2
#define CLK_NFI_SEL_XTAL_D1              3
#define CLK_PDN_NFI                      (1U<<7)


#define REG_RW_HDMI_PLL_CKCFG             0x00c
#define CLK_HDMI_PLL_SEL_HDMIPLL               0
#define CLK_HDMI_PLL_SEL_32K                  1
#define CLK_HDMI_PLL_SEL_XTAL_D1            2
#define CLK_HDMI_PLL_SEL_NFIPLL             3
#define CLK_PDN_HDMI_PLL                    (1U<<7)



#define REG_RW_HDMI_DISP_CKCFG           0x010
#define CLK_HDMI_DISP_SEL_DISP               0
#define CLK_HDMI_DISP_SEL_32K                  1
#define CLK_HDMI_DISP_SEL_XTAL_D1            2
#define CLK_HDMI_DISP_SEL_NFIPLL             3
#define CLK_PDN_HDMI_DISP                      (1U<<7)


#define REG_RW_LVDS_DISP_CKCFG           0x014
#define CLK_LVDS_DISP_SEL_AD_VPLL_DPIX         0
#define CLK_LVDS_DISP_SEL_32K                  1
#define CLK_LVDS_DISP_SEL_XTAL_D1            2
#define CLK_LVDS_DISP_SEL_NFIPLL             3
#define CLK_PDN_LVDS_DISP                      (1U<<7)


#define REG_RW_LVDS_CTS_CKCFG            0x018
#define CLK_LVDS_CTS_SEL_AD_VPLL_DPIX         0
#define CLK_LVDS_CTS_SEL_32K                  1
#define CLK_LVDS_CTS_SEL_XTAL_D1            2
#define CLK_LVDS_CTS_SEL_NFIPLL             3
#define CLK_PDN_LVDS_CTS                      (1U<<7)


#define REG_RW_PMUX0                 0x200
#define REG_RW_PMUX1                 0x204
#define REG_RW_PMUX2                 0x208
#define REG_RW_PMUX3                 0x20c
#define REG_RW_PMUX4                 0x210
#define REG_RW_PMUX5                 0x214
#define REG_RW_PMUX6                 0x218
#define REG_RW_PMUX7                 0x21c
#define REG_RW_PMUX8                 0x220
#define REG_RW_PMUX9                 0x224



#define REG_RW_GPIO_EN_1             0x128
#define REG_RW_GPIO_OUT_1             0x11c
#define REG_RW_GPIO_IN_1             0x138


#define REG_RW_GPIO_EN_0             0x124        /* need get register addr */
/* #define REG_RW_GPIO_OUT_0             0x11c       // need get register addr */
#define REG_RW_GPIO_IN_0             0x134       /* need get register addr */

/* #define REG_RW_GPIO_EN_2             0x128        // need get register addr */
/* #define REG_RW_GPIO_OUT_2             0x11c       // need get register addr */
/* #define REG_RW_GPIO_IN_2            0x138       // need get register addr */


#define REG_RW_PLL_GPANACFG0          0x34c
#define PLL_GPANACFG0_NFIPLL_EN       (1U<<1)


#define REG_RW_PAD_PD0            0x258
#define REG_RW_PAD_PD1            0x25c
#define REG_RW_PAD_PD2            0x260

#define REG_RW_PAD_PU0            0x264
#define REG_RW_PAD_PU1            0x268
#define REG_RW_PAD_PU2            0x26c


extern int mt8193_ckgen_i2c_write(u16 addr, u32 data);
extern u32 mt8193_ckgen_i2c_read(u16 addr);


#define IO_READ8(base, offset)                          mt8193_ckgen_i2c_read((base) + (offset))
#define IO_READ16(base, offset)                         mt8193_ckgen_i2c_read((base) + (offset))
#define IO_READ32(base, offset)                         mt8193_ckgen_i2c_read((base) + (offset))

/*===========================================================================*/
/* Macros for register write                                                 */
/*===========================================================================*/
#define IO_WRITE8(base, offset, value)                  mt8193_ckgen_i2c_write((base) + (offset), (value))
#define IO_WRITE16(base, offset, value)                 mt8193_ckgen_i2c_write((base) + (offset), (value))
#define IO_WRITE32(base, offset, value)                 mt8193_ckgen_i2c_write((base) + (offset), (value))


#define CKGEN_READ8(offset)            IO_READ8(CKGEN_BASE, (offset))
#define CKGEN_READ16(offset)           IO_READ16(CKGEN_BASE, (offset))
#define CKGEN_READ32(offset)           IO_READ32(CKGEN_BASE, (offset))

#define CKGEN_WRITE8(offset, value)    IO_WRITE8(CKGEN_BASE, (offset), (value))
#define CKGEN_WRITE16(offset, value)   IO_WRITE16(CKGEN_BASE, (offset), (value))
#define CKGEN_WRITE32(offset, value)   IO_WRITE32(CKGEN_BASE, (offset), (value))


/*=======================================================================*/
/* Constant Definitions                                                  */
/*=======================================================================*/

enum SRC_CK_T {
	SRC_CK_APLL,
	SRC_CK_ARMPLL,
	SRC_CK_VDPLL,
	SRC_CK_DMPLL,
	SRC_CK_SYSPLL1,
	SRC_CK_SYSPLL2,
	SRC_CK_USBCK,
	SRC_CK_MEMPLL,
	SRC_CK_MCK
};

enum e_CLK_T {
	e_CLK_NFI,                /*0   0x70.3 */
	e_CLK_HDMIPLL,            /* 1   0x70.7 */
	e_CLK_HDMIDISP,
	e_CLK_LVDSDISP,
	e_CLK_LVDSCTS,
	e_CLK_MAX                 /* 2 */
};

enum e_CKGEN_T {
	e_CKEN_NFI,               /* 0   0x300.31 */
	e_CKEN_HDMI,              /* 1   0x300.29 */
	e_CKEN_MAX                /* 2 */
};

#define MT8193_I2C_ID       1
#define USING_MT8193_DPI1   1

#endif /* MT8193_H */

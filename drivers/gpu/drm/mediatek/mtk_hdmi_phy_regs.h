/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
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
#ifndef _MTK_HDMI_PHY_REGS_H
#define _MTK_HDMI_PHY_REGS_H

#define HDMI_CON0		0x00
#define RG_HDMITX_PLL_EN		BIT(31)
#define RG_HDMITX_PLL_FBKDIV		(0x7f << 24)
#define PLL_FBKDIV_SHIFT		24
#define RG_HDMITX_PLL_FBKSEL		(0x3 << 22)
#define PLL_FBKSEL_SHIFT		22
#define RG_HDMITX_PLL_PREDIV		(0x3 << 20)
#define PREDIV_SHIFT			20
#define RG_HDMITX_PLL_POSDIV		(0x3 << 18)
#define POSDIV_SHIFT			18
#define RG_HDMITX_PLL_RST_DLY		(0x3 << 16)
#define RG_HDMITX_PLL_IR		(0xf << 12)
#define PLL_IR_SHIFT			12
#define RG_HDMITX_PLL_IC		(0xf << 8)
#define PLL_IC_SHIFT			8
#define RG_HDMITX_PLL_BP		(0xf << 4)
#define PLL_BP_SHIFT			4
#define RG_HDMITX_PLL_BR		(0x3 << 2)
#define PLL_BR_SHIFT			2
#define RG_HDMITX_PLL_BC		(0x3 << 0)
#define PLL_BC_SHIFT			0
#define HDMI_CON1		0x04
#define RG_HDMITX_PLL_DIVEN		(0x7 << 29)
#define PLL_DIVEN_SHIFT			29
#define RG_HDMITX_PLL_AUTOK_EN		BIT(28)
#define RG_HDMITX_PLL_AUTOK_KF		(0x3 << 26)
#define RG_HDMITX_PLL_AUTOK_KS		(0x3 << 24)
#define RG_HDMITX_PLL_AUTOK_LOAD	BIT(23)
#define RG_HDMITX_PLL_BAND		(0x3f << 16)
#define RG_HDMITX_PLL_REF_SEL		BIT(15)
#define RG_HDMITX_PLL_BIAS_EN		BIT(14)
#define RG_HDMITX_PLL_BIAS_LPF_EN	BIT(13)
#define RG_HDMITX_PLL_TXDIV_EN		BIT(12)
#define RG_HDMITX_PLL_TXDIV		(0x3 << 10)
#define PLL_TXDIV_SHIFT			10
#define RG_HDMITX_PLL_LVROD_EN		BIT(9)
#define RG_HDMITX_PLL_MONVC_EN		BIT(8)
#define RG_HDMITX_PLL_MONCK_EN		BIT(7)
#define RG_HDMITX_PLL_MONREF_EN		BIT(6)
#define RG_HDMITX_PLL_TST_EN		BIT(5)
#define RG_HDMITX_PLL_TST_CK_EN		BIT(4)
#define RG_HDMITX_PLL_TST_SEL		(0xf << 0)
#define HDMI_CON2		0x08
#define RGS_HDMITX_PLL_AUTOK_BAND	(0x7f << 8)
#define RGS_HDMITX_PLL_AUTOK_FAIL	BIT(1)
#define RG_HDMITX_EN_TX_CKLDO		BIT(0)
#define HDMI_CON3		0x0c
#define RG_HDMITX_SER_EN		(0xf << 28)
#define RG_HDMITX_PRD_EN		(0xf << 24)
#define RG_HDMITX_PRD_IMP_EN		(0xf << 20)
#define RG_HDMITX_DRV_EN		(0xf << 16)
#define RG_HDMITX_DRV_IMP_EN		(0xf << 12)
#define DRV_IMP_EN_SHIFT		12
#define RG_HDMITX_MHLCK_FORCE		BIT(10)
#define RG_HDMITX_MHLCK_PPIX_EN		BIT(9)
#define RG_HDMITX_MHLCK_EN		BIT(8)
#define RG_HDMITX_SER_DIN_SEL		(0xf << 4)
#define RG_HDMITX_SER_5T1_BIST_EN	BIT(3)
#define RG_HDMITX_SER_BIST_TOG		BIT(2)
#define RG_HDMITX_SER_DIN_TOG		BIT(1)
#define RG_HDMITX_SER_CLKDIG_INV	BIT(0)
#define HDMI_CON4		0x10
#define RG_HDMITX_PRD_IBIAS_CLK		(0xf << 24)
#define RG_HDMITX_PRD_IBIAS_D2		(0xf << 16)
#define RG_HDMITX_PRD_IBIAS_D1		(0xf << 8)
#define RG_HDMITX_PRD_IBIAS_D0		(0xf << 0)
#define PRD_IBIAS_CLK_SHIFT		24
#define PRD_IBIAS_D2_SHIFT		16
#define PRD_IBIAS_D1_SHIFT		8
#define PRD_IBIAS_D0_SHIFT		0
#define HDMI_CON5		0x14
#define RG_HDMITX_DRV_IBIAS_CLK		(0x3f << 24)
#define RG_HDMITX_DRV_IBIAS_D2		(0x3f << 16)
#define RG_HDMITX_DRV_IBIAS_D1		(0x3f << 8)
#define RG_HDMITX_DRV_IBIAS_D0		(0x3f << 0)
#define DRV_IBIAS_CLK_SHIFT		24
#define DRV_IBIAS_D2_SHIFT		16
#define DRV_IBIAS_D1_SHIFT		8
#define DRV_IBIAS_D0_SHIFT		0
#define HDMI_CON6		0x18
#define RG_HDMITX_DRV_IMP_CLK		(0x3f << 24)
#define RG_HDMITX_DRV_IMP_D2		(0x3f << 16)
#define RG_HDMITX_DRV_IMP_D1		(0x3f << 8)
#define RG_HDMITX_DRV_IMP_D0		(0x3f << 0)
#define DRV_IMP_CLK_SHIFT		24
#define DRV_IMP_D2_SHIFT		16
#define DRV_IMP_D1_SHIFT		8
#define DRV_IMP_D0_SHIFT		0
#define HDMI_CON7		0x1c
#define RG_HDMITX_MHLCK_DRV_IBIAS	(0x1f << 27)
#define RG_HDMITX_SER_DIN		(0x3ff << 16)
#define RG_HDMITX_CHLDC_TST		(0xf << 12)
#define RG_HDMITX_CHLCK_TST		(0xf << 8)
#define RG_HDMITX_RESERVE		(0xff << 0)
#define HDMI_CON8		0x20
#define RGS_HDMITX_2T1_LEV		(0xf << 16)
#define RGS_HDMITX_2T1_EDG		(0xf << 12)
#define RGS_HDMITX_5T1_LEV		(0xf << 8)
#define RGS_HDMITX_5T1_EDG		(0xf << 4)
#define RGS_HDMITX_PLUG_TST		BIT(0)

#endif

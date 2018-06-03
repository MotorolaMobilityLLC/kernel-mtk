/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
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

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>

#include "clk-mtk.h"
#include "clk-gate.h"
#include "clk-mux.h"

#include <dt-bindings/clock/mt6763-clk.h>

#define MT_CCF_BRINGUP	1
#ifdef CONFIG_ARM64
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif

#define mt_reg_sync_writel(v, a) \
	do { \
		__raw_writel((v), IOMEM(a)); \
		mb(); } \
while (0)

#define clk_readl(addr)			__raw_readl(IOMEM(addr))

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_setl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

#define PLL_EN  (0x1 << 0)
#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)

static DEFINE_SPINLOCK(mt6763_clk_lock);

/* Total 10 subsys */
void __iomem *cksys_base;
void __iomem *infracfg_base;
void __iomem *apmixed_base;
void __iomem *audio_base;
void __iomem *cam_base;
void __iomem *img_base;
void __iomem *mfgcfg_base;
void __iomem *mmsys_config_base;
void __iomem *pericfg_base;
void __iomem *venc_gcon_base;

/* CKSYS */
#define CLK_MISC_CFG_0		(cksys_base + 0x104)
#define CLK_DBG_CFG		(cksys_base + 0x10C)
#define CLK26CALI_0		(cksys_base + 0x220)
#define CLK26CALI_1		(cksys_base + 0x224)
#define CLK_SCP_CFG_0		(cksys_base + 0x0200)
#define CLK_SCP_CFG_1		(cksys_base + 0x0204)
/* CG */
#define INFRA_PDN_SET0		(infracfg_base + 0x0080)
#define INFRA_PDN_CLR0		(infracfg_base + 0x0084)
#define INFRA_PDN_STA0		(infracfg_base + 0x0090)
#define INFRA_PDN_SET1		(infracfg_base + 0x0088)
#define INFRA_PDN_CLR1		(infracfg_base + 0x008C)
#define INFRA_PDN_STA1		(infracfg_base + 0x0094)
#define INFRA_PDN_SET2		(infracfg_base + 0x00A4)
#define INFRA_PDN_CLR2		(infracfg_base + 0x00A8)
#define INFRA_PDN_STA2		(infracfg_base + 0x00AC)
#define INFRA_PDN_SET3		(infracfg_base + 0x00C0)
#define INFRA_PDN_CLR3		(infracfg_base + 0x00C4)
#define INFRA_PDN_STA3		(infracfg_base + 0x00C8)

#define AP_PLL_CON3		(apmixed_base + 0x000C)
#define AP_PLL_CON4		(apmixed_base + 0x0010)
#define ARMPLL_LL_CON0		(apmixed_base + 0x0200)
#define ARMPLL_LL_CON1		(apmixed_base + 0x0204)
#define ARMPLL_LL_PWR_CON0	(apmixed_base + 0x020C)
#define ARMPLL_L_CON0		(apmixed_base + 0x0210)
#define ARMPLL_L_CON1		(apmixed_base + 0x0214)
#define ARMPLL_L_PWR_CON0	(apmixed_base + 0x021C)
#define MAINPLL_CON0		(apmixed_base + 0x0220)
#define MAINPLL_PWR_CON0	(apmixed_base + 0x022C)
#define UNIVPLL_CON0		(apmixed_base + 0x0230)
#define UNIVPLL_PWR_CON0	(apmixed_base + 0x023C)
#define MFGPLL_CON0		(apmixed_base + 0x0240)
#define MFGPLL_PWR_CON0		(apmixed_base + 0x024C)
#define MSDCPLL_CON0		(apmixed_base + 0x0250)
#define MSDCPLL_PWR_CON0	(apmixed_base + 0x025C)
#define TVDPLL_CON0		(apmixed_base + 0x0260)
#define TVDPLL_PWR_CON0		(apmixed_base + 0x026C)
#define MMPLL_CON0		(apmixed_base + 0x0270)
#define MMPLL_PWR_CON0		(apmixed_base + 0x027C)
#define CCIPLL_CON0		(apmixed_base + 0x0290)
#define CCIPLL_PWR_CON0		(apmixed_base + 0x029C)
#define APLL1_CON0		(apmixed_base + 0x02A0)
#define APLL1_CON1		(apmixed_base + 0x02A4)
#define APLL1_CON2		(apmixed_base + 0x02A8)
#define APLL1_PWR_CON0		(apmixed_base + 0x02B0)
#define APLL2_CON0		(apmixed_base + 0x02B4)
#define APLL2_CON1		(apmixed_base + 0x02B8)
#define APLL2_CON2		(apmixed_base + 0x02BC)
#define APLL2_PWR_CON0		(apmixed_base + 0x02C4)

#define AUDIO_TOP_CON0		(audio_base + 0x0000)
#define AUDIO_TOP_CON1		(audio_base + 0x0004)

#define CAMSYS_CG_CON		(cam_base + 0x0000)
#define CAMSYS_CG_SET		(cam_base + 0x0004)
#define CAMSYS_CG_CLR		(cam_base + 0x0008)

#define IMG_CG_CON		(img_base + 0x0000)
#define IMG_CG_SET		(img_base + 0x0004)
#define IMG_CG_CLR		(img_base + 0x0008)

#define MFG_CG_CON              (mfgcfg_base + 0x0000)
#define MFG_CG_SET              (mfgcfg_base + 0x0004)
#define MFG_CG_CLR              (mfgcfg_base + 0x0008)

#define MM_CG_CON0            (mmsys_config_base + 0x100)
#define MM_CG_SET0            (mmsys_config_base + 0x104)
#define MM_CG_CLR0            (mmsys_config_base + 0x108)
#define MM_CG_CON1            (mmsys_config_base + 0x110)
#define MM_CG_SET1            (mmsys_config_base + 0x114)
#define MM_CG_CLR1            (mmsys_config_base + 0x118)

#define VENC_CG_CON		(venc_gcon_base + 0x0000)
#define VENC_CG_SET		(venc_gcon_base + 0x0004)
#define VENC_CG_CLR		(venc_gcon_base + 0x0008)

#if MT_CCF_BRINGUP
#define INFRA_CG0 0x032f8100/*[25:24][21][19:15][8]*/
#define INFRA_CG1 0x08000a00/*[27][11][9]*/
#define INFRA_CG2 0x00000005/*[2][0]*/
#define INFRA_CG3 0xFFFFFFFF
#define CAMSYS_CG	0x1FFF
#define IMG_CG	0x3FFF
#define MFG_CG	0x1
#define MM_CG0	0xFFFFFFFF /* un-gating in preloader */
#define MM_CG1  0x000003FF /* un-gating in preloader */
#define VENC_CG 0x001111 /* inverse */
#else
/*add normal cg init setting*/
#endif



#define CK_CFG_0 0x40
#define CK_CFG_0_SET 0x44
#define CK_CFG_0_CLR 0x48
#define CK_CFG_1 0x50
#define CK_CFG_1_SET 0x54
#define CK_CFG_1_CLR 0x58
#define CK_CFG_2 0x60
#define CK_CFG_2_SET 0x64
#define CK_CFG_2_CLR 0x68
#define CK_CFG_3 0x70
#define CK_CFG_3_SET 0x74
#define CK_CFG_3_CLR 0x78
#define CK_CFG_4 0x80
#define CK_CFG_4_SET 0x84
#define CK_CFG_4_CLR 0x88
#define CK_CFG_5 0x90
#define CK_CFG_5_SET 0x94
#define CK_CFG_5_CLR 0x98
#define CK_CFG_6 0xa0
#define CK_CFG_6_SET 0xa4
#define CK_CFG_6_CLR 0xa8
#define CK_CFG_7 0xb0
#define CK_CFG_7_SET 0xb4
#define CK_CFG_7_CLR 0xb8
#define CK_CFG_8 0xc0
#define CK_CFG_8_SET 0xc4
#define CK_CFG_8_CLR 0xc8
#define CK_CFG_9 0xd0
#define CK_CFG_9_SET 0xd4
#define CK_CFG_9_CLR 0xd8
#define CK_CFG_10 0xe0
#define CK_CFG_10_SET 0xe4
#define CK_CFG_10_CLR 0xe8
#define CLK_CFG_UPDATE 0x004
#define CLK_CFG_UPDATE1 0x008

static const struct mtk_fixed_clk fixed_clks[] __initconst = {
	FIXED_CLK(TOP_CLK26M, "f_f26m_ck", "clk26m", 26000000),
};

static const struct mtk_fixed_factor top_divs[] __initconst = {
	FACTOR(TOP_CLK13M, "clk13m", "clk26m", 1,
		2),
	FACTOR(TOP_SYSPLL_CK, "syspll_ck", "mainpll", 1,
		1),
	FACTOR(TOP_SYSPLL_D2, "syspll_d2", "syspll_ck", 1,
		2),
	FACTOR(TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1,
		2),
	FACTOR(TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1,
		4),
	FACTOR(TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1,
		8),
	FACTOR(TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1,
		16),
	FACTOR(TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1,
		3),
	FACTOR(TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1,
		2),
	FACTOR(TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1,
		4),
	FACTOR(TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1,
		8),
	FACTOR(TOP_SYSPLL_D3_D3, "syspll_d3_d3", "syspll_d3", 1,
		8),
	FACTOR(TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1,
		5),
	FACTOR(TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1,
		2),
	FACTOR(TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1,
		4),
	FACTOR(TOP_SYSPLL_D7, "syspll_d7", "mainpll", 1,
		7),
	FACTOR(TOP_SYSPLL4_D2, "syspll4_d2", "syspll_d7", 1,
		2),
	FACTOR(TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1,
		4),
	FACTOR(TOP_UNIVPLL_CK, "univpll_ck", "univpll", 1,
		1),
	FACTOR(TOP_UNIVPLL_D2, "univpll_d2", "univpll_ck", 1,
		2),
	FACTOR(TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1,
		2),
	FACTOR(TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1,
		7),
	FACTOR(TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1,
		26),
	FACTOR(TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1,
		3),
	FACTOR(TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1,
		2),
	FACTOR(TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1,
		4),
	FACTOR(TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1,
		8),
	FACTOR(TOP_UNIVPLL2_D16, "univpll2_d16", "univpll_d3", 1,
		16),
	FACTOR(TOP_UNIVPLL2_D32, "univpll2_d32", "univpll_d3", 1,
		32),
	FACTOR(TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1,
		5),
	FACTOR(TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1,
		2),
	FACTOR(TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1,
		4),
	FACTOR(TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d5", 1,
		8),
	FACTOR(TOP_UNIVPLL_D7, "univpll_d7", "univpll", 1,
		7),
	FACTOR(TOP_UNIVPLL_D26, "univpll_d26", "univpll_ck", 1,
		26),
	FACTOR(TOP_UNIVPLL_D52, "univpll_d52", "univpll_ck", 1,
		52),
	FACTOR(TOP_UNIVPLL_D104, "univpll_d104", "univpll_ck", 1,
		104),
	FACTOR(TOP_UNIVPLL_D208, "univpll_d208", "univpll_ck", 1,
		208),
	FACTOR(TOP_APLL1_CK, "apll1_ck", "apll1", 1,
		1),
	FACTOR(TOP_APLL1_D2, "apll1_d2", "apll1", 1,
		2),
	FACTOR(TOP_APLL1_D4, "apll1_d4", "apll1", 1,
		4),
	FACTOR(TOP_APLL1_D8, "apll1_d8", "apll1", 1,
		8),
	FACTOR(TOP_APLL2_CK, "apll2_ck", "apll2", 1,
		1),
	FACTOR(TOP_APLL2_D2, "apll2_d2", "apll2", 1,
		2),
	FACTOR(TOP_APLL2_D4, "apll2_d4", "apll2", 1,
		4),
	FACTOR(TOP_APLL2_D8, "apll2_d8", "apll2", 1,
		8),
	FACTOR(TOP_TVDPLL_CK, "tvdpll_ck", "tvdpll", 1,
		1),
	FACTOR(TOP_TVDPLL_D2, "tvdpll_d2", "tvdpll_ck", 1,
		2),
	FACTOR(TOP_TVDPLL_D4, "tvdpll_d4", "tvdpll", 1,
		4),
	FACTOR(TOP_TVDPLL_D8, "tvdpll_d8", "tvdpll", 1,
		8),
	FACTOR(TOP_TVDPLL_D16, "tvdpll_d16", "tvdpll", 1,
		16),
	FACTOR(TOP_MMPLL_CK, "mmpll_ck", "mmpll", 1,
		1),
	FACTOR(TOP_MFGPLL_CK, "mfgpll_ck", "mfgpll", 1,
		1),
	FACTOR(TOP_MSDCPLL_CK, "msdcpll_ck", "msdcpll", 1,
		1),
	FACTOR(TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll", 1,
		2),
	FACTOR(TOP_MSDCPLL_D4, "msdcpll_d4", "msdcpll", 1,
		4),
	FACTOR(TOP_MSDCPLL_D8, "msdcpll_d8", "msdcpll", 1,
		8),
	FACTOR(TOP_MSDCPLL_D16, "msdcpll_d16", "msdcpll", 1,
		16),
	FACTOR(TOP_AD_OSC_CK, "ad_osc_ck", "osc", 1,
		1),
	FACTOR(TOP_OSC_D2, "osc_d2", "osc", 1,
		2),
	FACTOR(TOP_OSC_D4, "osc_d4", "osc", 1,
		4),
	FACTOR(TOP_OSC_D8, "osc_d8", "osc", 1,
		8),
	FACTOR(TOP_OSC_D16, "osc_d16", "osc", 1,
		16),
	FACTOR(TOP_OSC_D32, "osc_d32", "osc", 1,
		32),
};

static const char * const axi_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll_d7",
	"osc_d8"
};

static const char * const mm_parents[] __initconst = {
	"clk26m",
	"mmpll_ck",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"syspll2_d2"
};

static const char * const pwm_parents[] __initconst = {
	"clk26m",
	"univpll2_d22",
	"univpll2_d4"
};

static const char * const cam_parents[] __initconst = {
	"clk26m",
	"mmpll_ck",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"syspll2_d2",
	"tvdpll_ck",
	"apll2_ck"
};

static const char * const mfg_parents[] __initconst = {
	"clk26m",
	"mfgpll_ck",
	"univpll_d3",
	"syspll_d2"
};

static const char * const camtg_parents[] __initconst = {
	"clk26m",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"univpll2_d16",
	"univpll2_d32",
	"univpll_d104",
	"univpll_d208"
};

static const char * const uart_parents[] __initconst = {
	"clk26m",
	"univpll2_d8"
};

static const char * const spi_parents[] __initconst = {
	"clk26m",
	"syspll3_d2",
	"syspll2_d4",
	"msdcpll_d4"
};

static const char * const msdc50_hclk_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll2_d2"
};

static const char * const msdc50_0_parents[] __initconst = {
	"clk26m",
	"msdcpll_ck",
	"msdcpll_d2",
	"univpll1_d4",
	"syspll2_d2",
	"univpll1_d2"
};

static const char * const msdc30_1_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"syspll2_d2",
	"syspll_d7",
	"msdcpll_d2"
};

static const char * const msdc30_2_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"syspll2_d2",
	"syspll_d7",
	"msdcpll_d2"
};

static const char * const msdc30_3_parents[] __initconst = {
	"clk26m",
	"msdcpll_d8",
	"msdcpll_d4",
	"univpll1_d4",
	"univpll_d26",
	"syspll_d7",
	"univpll_d7",
	"syspll3_d4",
	"msdcpll_d16"
};

static const char * const audio_parents[] __initconst = {
	"clk26m",
	"syspll3_d4",
	"syspll4_d4",
	"syspll1_d16"
};

static const char * const aud_intbus_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll4_d2"
};

static const char * const fpwrap_ulposc_parents[] __initconst = {
	"clk26m",
	"osc_d16",
	"osc_d4",
	"osc_d8"
};

static const char * const scp_parents[] __initconst = {
	"clk26m",
	"syspll1_d22",
	"syspll1_d4"
};

static const char * const atb_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll_d5"
};

static const char * const sspm_parents[] __initconst = {
	"clk26m",
	"syspll_d52",
	"univpll_d5"
};

static const char * const dpi0_parents[] __initconst = {
	"clk26m",
	"tvdpll_d2",
	"tvdpll_d4",
	"tvdpll_d8",
	"tvdpll_d16"
};

static const char * const scam_parents[] __initconst = {
	"clk26m",
	"syspll3_d2"
};

static const char * const aud_1_parents[] __initconst = {
	"clk26m",
	"apll1_ck"
};

static const char * const aud_2_parents[] __initconst = {
	"clk26m",
	"apll2_ck"
};

static const char * const disppwm_parents[] __initconst = {
	"clk26m",
	"univpll2_d4",
	"osc_d4",
	"osc_d16"
};

static const char * const ssusb_top_sys_parents[] __initconst = {
	"clk26m",
	"univpll3_d2"
};

static const char * const ssusb_top_xhci_parents[] __initconst = {
	"clk26m",
	"univpll3_d2"
};

static const char * const usb_top_parents[] __initconst = {
	"clk26m",
	"univpll3_d4"
};

static const char * const spm_parents[] __initconst = {
	"clk26m",
	"syspll1_d8"
};

static const char * const i2c_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"univpll3_d4"
};

static const char * const f52m_mfg_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"univpll2_d4",
	"univpll2_d8"
};

static const char * const seninf_parents[] __initconst = {
	"clk26m",
	"univpll_d72",
	"univpll2_d23",
	"univpll1_d4"
};

static const char * const dxcc_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll1_d4",
	"syspll1_d8"
};

static const char * const camtg2_parents[] __initconst = {
	"clk26m",
	"univpll_d52",
	"univpll2_d8",
	"univpll_d26",
	"univpll2_d16",
	"univpll2_d32",
	"univpll_d104",
	"univpll_d208"
};

static const char * const aud_engen1_parents[] __initconst = {
	"clk26m",
	"apll1_d2",
	"apll1_d4",
	"apll1_d8"
};

static const char * const aud_engen2_parents[] __initconst = {
	"clk26m",
	"apll2_d2",
	"apll2_d4",
	"apll2_d8"
};

static const char * const faes_ufsfde_parents[] __initconst = {
	"clk26m",
	"syspll_d2",
	"syspll1_d2",
	"syspll_d3",
	"syspll1_d4",
	"univpll_d3"
};

static const char * const fufs_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll1_d8",
	"syspll1_d16"
};


#define INVALID_UPDATE_REG 0xFFFFFFFF
#define INVALID_UPDATE_SHIFT -1
#define INVALID_MUX_GATE -1

static const struct mtk_mux_clr_set_upd top_muxes[] __initconst = {
#if MT_CCF_BRINGUP
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(TOP_MUX_AXI, "axi_sel", axi_parents, CK_CFG_0,
		CK_CFG_0_SET, CK_CFG_0_CLR, 0, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_MM, "mm_sel", mm_parents, CK_CFG_0,
		CK_CFG_0_SET, CK_CFG_0_CLR, 24, 3, 31, CLK_CFG_UPDATE, 3),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD(TOP_MUX_PWM, "pwm_sel", pwm_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 0, 2, 7, CLK_CFG_UPDATE, 4),
	MUX_CLR_SET_UPD(TOP_MUX_CAM, "cam_sel", cam_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 8, 3, 15, CLK_CFG_UPDATE, 5),
	/* PDN: No HS */
	MUX_CLR_SET_UPD(TOP_MUX_MFG, "mfg_sel", mfg_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 24, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE, 7),
	/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG, "camtg_sel", camtg_parents, CK_CFG_2,
		CK_CFG_2_SET, CK_CFG_2_CLR, 0, 3, 7, CLK_CFG_UPDATE, 8),
	MUX_CLR_SET_UPD(TOP_MUX_UART, "uart_sel", uart_parents, CK_CFG_2,
		CK_CFG_2_SET, CK_CFG_2_CLR, 8, 1, 15, CLK_CFG_UPDATE, 9),
	MUX_CLR_SET_UPD(TOP_MUX_SPI, "spi_sel", spi_parents, CK_CFG_2,
		CK_CFG_2_SET, CK_CFG_2_CLR, 16, 2, 23, CLK_CFG_UPDATE, 10),
	/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0_HCLK, "msdc50_hclk_sel", msdc50_hclk_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 8, 2, 15, CLK_CFG_UPDATE, 12),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0, "msdc50_0_sel", msdc50_0_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 16, 3, 23, CLK_CFG_UPDATE, 13),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_1, "msdc30_1_sel", msdc30_1_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 24, 3, 31, CLK_CFG_UPDATE, 14),
	/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_2, "msdc30_2_sel", msdc30_2_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 0, 3, 7, CLK_CFG_UPDATE, 15),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_3, "msdc30_3_sel", msdc30_3_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 8, 4, 15, CLK_CFG_UPDATE, 16),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO, "audio_sel", audio_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 16, 2, 23, CLK_CFG_UPDATE, 17),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_INTBUS, "aud_intbus_sel", aud_intbus_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 24, 2, 31, CLK_CFG_UPDATE, 18),
	/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(TOP_MUX_FPWRAP_ULPOSC, "fpwrap_ulposc_sel", fpwrap_ulposc_parents, CK_CFG_5,
		CK_CFG_5_SET, CK_CFG_5_CLR, 0, 2, 7, CLK_CFG_UPDATE, 19),
	MUX_CLR_SET_UPD(TOP_MUX_SCP, "scp_sel", scp_parents, CK_CFG_5,
		CK_CFG_5_SET, CK_CFG_5_CLR, 8, 2, 15, CLK_CFG_UPDATE, 20),
	MUX_CLR_SET_UPD(TOP_MUX_ATB, "atb_sel", atb_parents, CK_CFG_5,
		CK_CFG_5_SET, CK_CFG_5_CLR, 16, 2, 23, CLK_CFG_UPDATE, 21),
	MUX_CLR_SET_UPD(TOP_MUX_SSPM, "sspm_sel", sspm_parents, CK_CFG_5,
		CK_CFG_5_SET, CK_CFG_5_CLR, 24, 2, 31, CLK_CFG_UPDATE, 22),
	/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(TOP_MUX_DPI0, "dpi0_sel", dpi0_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 0, 3, 7, CLK_CFG_UPDATE, 23),
	MUX_CLR_SET_UPD(TOP_MUX_SCAM, "scam_sel", scam_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 8, 1, 15, CLK_CFG_UPDATE, 24),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_1, "aud_1_sel", aud_1_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 16, 1, 23, CLK_CFG_UPDATE, 25),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_2, "aud_2_sel", aud_2_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 24, 1, 31, CLK_CFG_UPDATE, 26),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(TOP_MUX_DISP_PWM, "disppwm_sel", disppwm_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 0, 2, 7, CLK_CFG_UPDATE, 27),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_SYS, "ssusb_top_sys_sel", ssusb_top_sys_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 8, 1, 15, CLK_CFG_UPDATE, 28),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_XHCI, "ssusb_top_xhci_sel", ssusb_top_xhci_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 16, 1, 23, CLK_CFG_UPDATE, 29),
	MUX_CLR_SET_UPD(TOP_MUX_USB_TOP, "usb_top_sel", usb_top_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 24, 1, 31, CLK_CFG_UPDATE, 30),
	/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(TOP_MUX_SPM, "spm_sel", spm_parents, CK_CFG_8,
		CK_CFG_8_SET, CK_CFG_8_CLR, 0, 1, 7, CLK_CFG_UPDATE, 31),
	/* i2c hang@8s */
	MUX_CLR_SET_UPD(TOP_MUX_I2C, "i2c_sel", i2c_parents, CK_CFG_8,
		CK_CFG_8_SET, CK_CFG_8_CLR, 16, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE1, 1),
	/* CLK_CFG_9 */
	MUX_CLR_SET_UPD(TOP_MUX_F52M_MFG, "f52m_mfg_sel", f52m_mfg_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 0, 2, 7, CLK_CFG_UPDATE, 11),
	MUX_CLR_SET_UPD(TOP_MUX_SENINF, "seninf_sel", seninf_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 8, 2, 15, CLK_CFG_UPDATE1, 3),
	MUX_CLR_SET_UPD(TOP_MUX_DXCC, "dxcc_sel", dxcc_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 16, 2, 23, CLK_CFG_UPDATE1, 4),
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG2, "camtg2_sel", camtg2_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 24, 3, 31, CLK_CFG_UPDATE1, 9),
	/* CLK_CFG_10 */
	MUX_CLR_SET_UPD(TOP_MUX_AUD_ENG1, "aud_eng1_sel", aud_engen1_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 0, 2, 7, CLK_CFG_UPDATE1, 5),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_ENG2, "aud_eng2_sel", aud_engen2_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 8, 2, 15, CLK_CFG_UPDATE1, 6),
	MUX_CLR_SET_UPD(TOP_MUX_FAES_UFSFDE, "faes_ufsfde_sel", faes_ufsfde_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 16, 3, 23, CLK_CFG_UPDATE1, 7),
	MUX_CLR_SET_UPD(TOP_MUX_FUFS, "fufs_sel", fufs_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 24, 2, 31, CLK_CFG_UPDATE1, 8),
#else
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(TOP_MUX_AXI, "axi_sel", axi_parents, CK_CFG_0,
		CK_CFG_0_SET, CK_CFG_0_CLR, 0, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_MM, "mm_sel", mm_parents, CK_CFG_0,
		CK_CFG_0_SET, CK_CFG_0_CLR, 24, 3, 31, CLK_CFG_UPDATE, 3),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD(TOP_MUX_PWM, "pwm_sel", pwm_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 0, 2, 7, CLK_CFG_UPDATE, 4),
	MUX_CLR_SET_UPD(TOP_MUX_CAM, "cam_sel", cam_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 8, 3, 15, CLK_CFG_UPDATE, 5),
	MUX_CLR_SET_UPD(TOP_MUX_MFG, "mfg_sel", mfg_parents, CK_CFG_1,
		CK_CFG_1_SET, CK_CFG_1_CLR, 24, 2, 31, CLK_CFG_UPDATE, 7),
	/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG, "camtg_sel", camtg_parents, CK_CFG_2,
		CK_CFG_2_SET, CK_CFG_2_CLR, 0, 3, 7, CLK_CFG_UPDATE, 8),
	MUX_CLR_SET_UPD(TOP_MUX_UART, "uart_sel", uart_parents, CK_CFG_2,
		CK_CFG_2_SET, CK_CFG_2_CLR, 8, 1, 15, CLK_CFG_UPDATE, 9),
	MUX_CLR_SET_UPD(TOP_MUX_SPI, "spi_sel", spi_parents, CK_CFG_2,/*spi*/
		CK_CFG_2_SET, CK_CFG_2_CLR, 16, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0_HCLK, "msdc50_hclk_sel", msdc50_hclk_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 8, 2, 15, CLK_CFG_UPDATE, 12),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0, "msdc50_0_sel", msdc50_0_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 16, 3, 23, CLK_CFG_UPDATE, 13),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_1, "msdc30_1_sel", msdc30_1_parents, CK_CFG_3,
		CK_CFG_3_SET, CK_CFG_3_CLR, 24, 3, 31, CLK_CFG_UPDATE, 14),
	/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_2, "msdc30_2_sel", msdc30_2_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 0, 3, 7, CLK_CFG_UPDATE, 15),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_3, "msdc30_3_sel", msdc30_3_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 8, 4, 15, CLK_CFG_UPDATE, 16),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO, "audio_sel", audio_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 16, 2, 23, CLK_CFG_UPDATE, 17),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_INTBUS, "aud_intbus_sel", aud_intbus_parents, CK_CFG_4,
		CK_CFG_4_SET, CK_CFG_4_CLR, 24, 2, 31, CLK_CFG_UPDATE, 18),
	/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(TOP_MUX_FPWRAP_ULPOSC, "fpwrap_ulposc_sel", fpwrap_ulposc_parents, CK_CFG_5,/*ulposc*/
		CK_CFG_5_SET, CK_CFG_5_CLR, 0, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_SCP, "scp_sel", scp_parents, CK_CFG_5,/*scp*/
		CK_CFG_5_SET, CK_CFG_5_CLR, 8, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_ATB, "atb_sel", atb_parents, CK_CFG_5,
		CK_CFG_5_SET, CK_CFG_5_CLR, 16, 2, 23, CLK_CFG_UPDATE, 21),
	MUX_CLR_SET_UPD(TOP_MUX_SSPM, "sspm_sel", sspm_parents, CK_CFG_5,/*sspm no use*/
		CK_CFG_5_SET, CK_CFG_5_CLR, 24, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(TOP_MUX_DPI0, "dpi0_sel", dpi0_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 0, 3, 7, CLK_CFG_UPDATE, 23),
	MUX_CLR_SET_UPD(TOP_MUX_SCAM, "scam_sel", scam_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 8, 1, 15, CLK_CFG_UPDATE, 24),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_1, "aud_1_sel", aud_1_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 16, 1, 23, CLK_CFG_UPDATE, 25),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_2, "aud_2_sel", aud_2_parents, CK_CFG_6,
		CK_CFG_6_SET, CK_CFG_6_CLR, 24, 1, 31, CLK_CFG_UPDATE, 26),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(TOP_MUX_DISP_PWM, "disppwm_sel", disppwm_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 0, 2, 7, CLK_CFG_UPDATE, 27),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_SYS, "ssusb_top_sys_sel", ssusb_top_sys_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 8, 1, 15, CLK_CFG_UPDATE, 28),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_XHCI, "ssusb_top_xhci_sel", ssusb_top_xhci_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 16, 1, 23, CLK_CFG_UPDATE, 29),
	MUX_CLR_SET_UPD(TOP_MUX_USB_TOP, "usb_top_sel", usb_top_parents, CK_CFG_7,
		CK_CFG_7_SET, CK_CFG_7_CLR, 24, 1, 31, CLK_CFG_UPDATE, 30),
	/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(TOP_MUX_SPM, "spm_sel", spm_parents, CK_CFG_8,
		CK_CFG_8_SET, CK_CFG_8_CLR, 0, 1, 7, CLK_CFG_UPDATE, 31),
	MUX_CLR_SET_UPD(TOP_MUX_I2C, "i2c_sel", i2c_parents, CK_CFG_8,
		CK_CFG_8_SET, CK_CFG_8_CLR, 16, 2, 23, CLK_CFG_UPDATE1, 1),
	/* CLK_CFG_9 */
	MUX_CLR_SET_UPD(TOP_MUX_F52M_MFG, "f52m_mfg_sel", f52m_mfg_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 0, 2, 7, CLK_CFG_UPDATE, 11),
	MUX_CLR_SET_UPD(TOP_MUX_SENINF, "seninf_sel", seninf_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 8, 2, 15, CLK_CFG_UPDATE1, 3),
	MUX_CLR_SET_UPD(TOP_MUX_DXCC, "dxcc_sel", dxcc_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 16, 2, 23, CLK_CFG_UPDATE1, 4),
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG2, "camtg2_sel", camtg2_parents, CK_CFG_9,
		CK_CFG_9_SET, CK_CFG_9_CLR, 24, 3, 31, CLK_CFG_UPDATE1, 9),
	/* CLK_CFG_10 */
	MUX_CLR_SET_UPD(TOP_MUX_AUD_ENG1, "aud_eng1_sel", aud_engen1_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 0, 2, 7, CLK_CFG_UPDATE1, 5),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_ENG2, "aud_eng2_sel", aud_engen2_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 8, 2, 15, CLK_CFG_UPDATE1, 6),
	MUX_CLR_SET_UPD(TOP_MUX_FAES_UFSFDE, "faes_ufsfde_sel", faes_ufsfde_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 16, 3, 23, CLK_CFG_UPDATE1, 7),
	MUX_CLR_SET_UPD(TOP_MUX_FUFS, "fufs_sel", fufs_parents, CK_CFG_10,
		CK_CFG_10_SET, CK_CFG_10_CLR, 24, 2, 31, CLK_CFG_UPDATE1, 8),
#endif
};

/* TODO: remove audio clocks after audio driver ready */

static int mtk_cg_bit_is_cleared(struct clk_hw *hw)
{
#if 1
	struct mtk_clk_gate *cg = to_clk_gate(hw);
	u32 val;

	regmap_read(cg->regmap, cg->sta_ofs, &val);

	val &= BIT(cg->bit);

	return val == 0;
#endif
	return 0;
}

static int mtk_cg_bit_is_set(struct clk_hw *hw)
{
#if 1
	struct mtk_clk_gate *cg = to_clk_gate(hw);
	u32 val;

	regmap_read(cg->regmap, cg->sta_ofs, &val);

	val &= BIT(cg->bit);

	return val != 0;
#endif
	return 0;
}
#if 1
static void mtk_cg_set_bit(struct clk_hw *hw)
{
	struct mtk_clk_gate *cg = to_clk_gate(hw);

	regmap_update_bits(cg->regmap, cg->sta_ofs, BIT(cg->bit), BIT(cg->bit));
}

static void mtk_cg_clr_bit(struct clk_hw *hw)
{
#if 1
	struct mtk_clk_gate *cg = to_clk_gate(hw);

	regmap_update_bits(cg->regmap, cg->sta_ofs, BIT(cg->bit), 0);
#endif
}
#endif
static int mtk_cg_enable(struct clk_hw *hw)
{
	mtk_cg_clr_bit(hw);

	return 0;
}

static void mtk_cg_disable(struct clk_hw *hw)
{
	mtk_cg_set_bit(hw);
}

static int mtk_cg_enable_inv(struct clk_hw *hw)
{
	mtk_cg_set_bit(hw);

	return 0;
}

static void mtk_cg_disable_inv(struct clk_hw *hw)
{
	mtk_cg_clr_bit(hw);
}

const struct clk_ops mtk_clk_gate_ops = {
	.is_enabled	= mtk_cg_bit_is_cleared,
	.enable		= mtk_cg_enable,
	.disable	= mtk_cg_disable,
};

const struct clk_ops mtk_clk_gate_ops_inv = {
	.is_enabled	= mtk_cg_bit_is_set,
	.enable		= mtk_cg_enable_inv,
	.disable	= mtk_cg_disable_inv,
};

static const struct mtk_gate_regs infra0_cg_regs = {
	.set_ofs = 0x80,
	.clr_ofs = 0x84,
	.sta_ofs = 0x90,
};

static const struct mtk_gate_regs infra1_cg_regs = {
	.set_ofs = 0x88,
	.clr_ofs = 0x8c,
	.sta_ofs = 0x94,
};

static const struct mtk_gate_regs infra2_cg_regs = {
	.set_ofs = 0xa4,
	.clr_ofs = 0xa8,
	.sta_ofs = 0xac,
};

static const struct mtk_gate_regs infra3_cg_regs = {
	.set_ofs = 0xc0,
	.clr_ofs = 0xc4,
	.sta_ofs = 0xc8,
};

#define GATE_INFRA0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra0_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_INFRA1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra1_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_INFRA2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra2_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_INFRA3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &infra3_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate infra_clks[] __initconst = {
	/* INFRA0 */
	GATE_INFRA0(INFRACFG_AO_PMIC_CG_TMR, "infra_pmic_tmr",
		"f_f26m_ck", 0),
	GATE_INFRA0(INFRACFG_AO_PMIC_CG_AP, "infra_pmic_ap",
		"f_f26m_ck", 1),
	GATE_INFRA0(INFRACFG_AO_PMIC_CG_MD, "infra_pmic_md",
		"f_f26m_ck", 2),
	GATE_INFRA0(INFRACFG_AO_PMIC_CG_CONN, "infra_pmic_conn",
		"f_f26m_ck", 3),
	GATE_INFRA0(INFRACFG_AO_SCPSYS_CG, "infra_scp",
		"axi_sel", 4),
	GATE_INFRA0(INFRACFG_AO_SEJ_CG, "infra_sej",
		"f_f26m_ck", 5),
	GATE_INFRA0(INFRACFG_AO_APXGPT_CG, "infra_apxgpt",
		"axi_sel", 6),
	GATE_INFRA0(INFRACFG_AO_ICUSB_CG, "infra_icusb",
		"usb_top_sel", 8),
	GATE_INFRA0(INFRACFG_AO_GCE_CG, "infra_gce",
		"axi_sel", 9),
	GATE_INFRA0(INFRACFG_AO_THERM_CG, "infra_therm",
		"axi_sel", 10),
	GATE_INFRA0(INFRACFG_AO_I2C0_CG, "infra_i2c0",
		"i2c_sel", 11),
	GATE_INFRA0(INFRACFG_AO_I2C1_CG, "infra_i2c1",
		"i2c_sel", 12),
	GATE_INFRA0(INFRACFG_AO_I2C2_CG, "infra_i2c2",
		"i2c_sel", 13),
	GATE_INFRA0(INFRACFG_AO_I2C3_CG, "infra_i2c3",
		"i2c_sel", 14),
	GATE_INFRA0(INFRACFG_AO_PWM_HCLK_CG, "infra_pwm_hclk",
		"i2c_sel", 15),
	GATE_INFRA0(INFRACFG_AO_PWM1_CG, "infra_pwm1",
		"i2c_sel", 16),
	GATE_INFRA0(INFRACFG_AO_PWM2_CG, "infra_pwm2",
		"i2c_sel", 17),
	GATE_INFRA0(INFRACFG_AO_PWM3_CG, "infra_pwm3",
		"i2c_sel", 18),
	GATE_INFRA0(INFRACFG_AO_PWM4_CG, "infra_pwm4",
		"i2c_sel", 19),
	GATE_INFRA0(INFRACFG_AO_PWM_CG, "infra_pwm",
		"i2c_sel", 21),
	GATE_INFRA0(INFRACFG_AO_UART0_CG, "infra_uart0",
		"uart_sel", 22),
	GATE_INFRA0(INFRACFG_AO_UART1_CG, "infra_uart1",
		"uart_sel", 23),
	GATE_INFRA0(INFRACFG_AO_UART2_CG, "infra_uart2",
		"uart_sel", 24),
	GATE_INFRA0(INFRACFG_AO_UART3_CG, "infra_uart3",
		"uart_sel", 25),
	GATE_INFRA0(INFRACFG_AO_GCE_26M_CG, "infra_gce_26m",
		"axi_sel", 27),
	GATE_INFRA0(INFRACFG_AO_CQ_DMA_FPC_CG, "infra_cqdma_fpc",
		"axi_sel", 28),
	GATE_INFRA0(INFRACFG_AO_BTIF_CG, "infra_btif",
		"axi_sel", 31),
	/* INFRA1 */
	GATE_INFRA1(INFRACFG_AO_SPI0_CG, "infra_spi0",
		"spi_sel", 1),
	GATE_INFRA1(INFRACFG_AO_MSDC0_CG, "infra_msdc0",
		"msdc50_hclk_sel", 2),
	GATE_INFRA1(INFRACFG_AO_MSDC1_CG, "infra_msdc1",
		"axi_sel", 4),
	GATE_INFRA1(INFRACFG_AO_MSDC2_CG, "infra_msdc2",
		"axi_sel", 5),
	GATE_INFRA1(INFRACFG_AO_MSDC0_SCK_CG, "infra_msdc0_sck",
		"msdc50_0_sel", 6),
	GATE_INFRA1(INFRACFG_AO_DVFSRC_CG, "infra_dvfsrc",
		"f_f26m_ck", 7),
	GATE_INFRA1(INFRACFG_AO_GCPU_CG, "infra_gcpu",
		"axi_sel", 8),
	GATE_INFRA1(INFRACFG_AO_TRNG_CG, "infra_trng",
		"axi_sel", 9),
	GATE_INFRA1(INFRACFG_AO_AUXADC_CG, "infra_auxadc",
		"f_f26m_ck", 10),
	GATE_INFRA1(INFRACFG_AO_CPUM_CG, "infra_cpum",
		"axi_sel", 11),
	GATE_INFRA1(INFRACFG_AO_CCIF1_AP_CG, "infra_ccif1_ap",
		"axi_sel", 12),
	GATE_INFRA1(INFRACFG_AO_CCIF1_MD_CG, "infra_ccif1_md",
		"axi_sel", 13),
	GATE_INFRA1(INFRACFG_AO_AUXADC_MD_CG, "infra_auxadc_md",
		"f_f26m_ck", 14),
	GATE_INFRA1(INFRACFG_AO_MSDC1_SCK_CG, "infra_msdc1_sck",
		"msdc30_1_sel", 16),
	GATE_INFRA1(INFRACFG_AO_MSDC2_SCK_CG, "infra_msdc2_sck",
		"msdc30_2_sel", 17),
	GATE_INFRA1(INFRACFG_AO_AP_DMA_CG, "infra_apdma",
		"axi_sel", 18),
	GATE_INFRA1(INFRACFG_AO_XIU_CG, "infra_xiu",
		"axi_sel", 19),
	GATE_INFRA1(INFRACFG_AO_DEVICE_APC_CG, "infra_device_apc",
		"axi_sel", 20),
	GATE_INFRA1(INFRACFG_AO_CCIF_AP_CG, "infra_ccif_ap",
		"axi_sel", 23),
	GATE_INFRA1(INFRACFG_AO_DEBUGSYS_CG, "infra_debugsys",
		"axi_sel", 24),
	GATE_INFRA1(INFRACFG_AO_AUDIO_CG, "infra_audio",
		"axi_sel", 25),
	GATE_INFRA1(INFRACFG_AO_CCIF_MD_CG, "infra_ccif_md",
		"axi_sel", 26),
	GATE_INFRA1(INFRACFG_AO_DXCC_SEC_CORE_CG, "infra_dxcc_sec_core",
		"dxcc_sel", 27),
	GATE_INFRA1(INFRACFG_AO_DXCC_AO_CG, "infra_dxcc_ao",
		"dxcc_sel", 28),
	GATE_INFRA1(INFRACFG_AO_DRAMC_F26M_CG, "infra_dramc_f26m",
		"f_f26m_ck", 31),
	/* INFRA2 */
	GATE_INFRA2(INFRACFG_AO_IRTX_CG, "infra_irtx",
		"f_f26m_ck", 0),
	GATE_INFRA2(INFRACFG_AO_DISP_PWM_CG, "infra_disppwm",
		"axi_sel", 2),
	GATE_INFRA2(INFRACFG_AO_CLDMA_BCLK_CK, "infra_cldma_bclk",
		"axi_sel", 3),
	GATE_INFRA2(INFRACFG_AO_AUDIO_26M_BCLK_CK, "infracfg_ao_audio_26m_bclk_ck",
		"f_f26m_ck", 4),
	GATE_INFRA2(INFRACFG_AO_SPI1_CG, "infra_spi1",
		"spi_sel", 6),
	GATE_INFRA2(INFRACFG_AO_I2C4_CG, "infra_i2c4",
		"i2c_sel", 7),
	GATE_INFRA2(INFRACFG_AO_MODEM_TEMP_SHARE_CG, "infra_md_tmp_share",
		"f_f26m_ck", 8),
	GATE_INFRA2(INFRACFG_AO_SPI2_CG, "infra_spi2",
		"spi_sel", 9),
	GATE_INFRA2(INFRACFG_AO_SPI3_CG, "infra_spi3",
		"spi_sel", 10),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_SCK_CG, "infra_unipro_sck",
		"spi_sel", 11),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_TICK_CG, "infra_unipro_tick",
		"spi_sel", 12),
	GATE_INFRA2(INFRACFG_AO_UFS_MP_SAP_BCLK_CG, "infra_ufs_mp_sap_bck",
		"spi_sel", 13),
	GATE_INFRA2(INFRACFG_AO_MD32_BCLK_CG, "infra_md32_bclk",
		"spi_sel", 14),
	GATE_INFRA2(INFRACFG_AO_SSPM_CG, "infra_sspm",
		"spi_sel", 15),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_MBIST_CG, "infra_unipro_mbist",
		"spi_sel", 16),
	GATE_INFRA2(INFRACFG_AO_SSPM_BUS_HCLK_CG, "infra_sspm_bus_hclk",
		"spi_sel", 17),
	GATE_INFRA2(INFRACFG_AO_I2C5_CG, "infra_i2c5",
		"i2c_sel", 18),
	GATE_INFRA2(INFRACFG_AO_I2C5_ARBITER_CG, "infra_i2c5_arbiter",
		"i2c_sel", 19),
	GATE_INFRA2(INFRACFG_AO_I2C5_IMM_CG, "infra_i2c5_imm",
		"i2c_sel", 20),
	GATE_INFRA2(INFRACFG_AO_I2C1_ARBITER_CG, "infra_i2c1_arbiter",
		"i2c_sel", 21),
	GATE_INFRA2(INFRACFG_AO_I2C1_IMM_CG, "infra_i2c1_imm",
		"i2c_sel", 22),
	GATE_INFRA2(INFRACFG_AO_I2C2_ARBITER_CG, "infra_i2c2_arbiter",
		"i2c_sel", 23),
	GATE_INFRA2(INFRACFG_AO_I2C2_IMM_CG, "infra_i2c2_imm",
		"i2c_sel", 24),
	GATE_INFRA2(INFRACFG_AO_SPI4_CG, "infra_spi4",
		"spi_sel", 25),
	GATE_INFRA2(INFRACFG_AO_SPI5_CG, "infra_spi5",
		"spi_sel", 26),
	GATE_INFRA2(INFRACFG_AO_CQ_DMA_CG, "infra_cqdma",
		"axi_sel", 27),
	GATE_INFRA2(INFRACFG_AO_UFS_CG, "infra_ufs",
		"fufs_sel", 28),
	GATE_INFRA2(INFRACFG_AO_AES_CG, "infra_aes",
		"spi_sel", 29),
	GATE_INFRA2(INFRACFG_AO_UFS_TICK_CG, "infra_ufs_tick",
		"fufs_sel", 30),
	/* INFRA3 */
	GATE_INFRA3(INFRACFG_AO_MSDC0_SELF_CG, "infra_msdc0_self",
		"msdc50_0_sel", 0),
	GATE_INFRA3(INFRACFG_AO_MSDC1_SELF_CG, "infra_msdc1_self",
		"msdc50_0_sel", 1),
	GATE_INFRA3(INFRACFG_AO_MSDC2_SELF_CG, "infra_msdc2_self",
		"msdc50_0_sel", 2),
	GATE_INFRA3(INFRACFG_AO_SSPM_26M_SELF_CG, "infra_sspm_26m_self",
		"f_f26m_ck", 3),
	GATE_INFRA3(INFRACFG_AO_SSPM_32K_SELF_CG, "infra_sspm_32k_self",
		"f_f26m_ck", 4),
	GATE_INFRA3(INFRACFG_AO_UFS_AXI_CG, "infra_ufs_axi",
		"axi_sel", 5),
	GATE_INFRA3(INFRACFG_AO_I2C6_CG, "infra_i2c6",
		"i2c_sel", 6),
	GATE_INFRA3(INFRACFG_AO_AP_MSDC0_CG, "infra_ap_msdc0",
		"msdc50_hclk_sel", 7),
	GATE_INFRA3(INFRACFG_AO_MD_MSDC0_CG, "infra_md_msdc0",
		"msdc50_hclk_sel", 8),
};

static const struct mtk_gate_regs mfg_cfg_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_MFG_CFG(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mfg_cfg_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mfg_cfg_clks[] __initconst = {
	GATE_MFG_CFG(MFGCFG_BG3D, "mfg_cfg_bg3d", "mfg_sel", 0)
};

static const struct mtk_gate_regs audio0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x0,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs audio1_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x4,
	.sta_ofs = 0x4,
};

#define GATE_AUDIO0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio0_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops,		\
	}

#define GATE_AUDIO1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &audio1_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops,		\
	}

static const struct mtk_gate audio_clks[] __initconst = {
	/* AUDIO0 */
	GATE_AUDIO0(AUDIO_AFE, "aud_afe", "audio_sel",
		2),
	GATE_AUDIO0(AUDIO_I2S, "aud_i2s", "clk_null",
		6),
	GATE_AUDIO0(AUDIO_22M, "aud_22m", "aud_engen1_sel",
		8),
	GATE_AUDIO0(AUDIO_24M, "aud_24m", "aud_engen2_sel",
		9),
	GATE_AUDIO0(AUDIO_APLL2_TUNER, "aud_apll2_tuner", "aud_engen2_sel",
		18),
	GATE_AUDIO0(AUDIO_APLL_TUNER, "aud_apll_tuner", "aud_engen1_sel",
		19),
	GATE_AUDIO0(AUDIO_ADC, "aud_adc", "audio_sel",
		24),
	GATE_AUDIO0(AUDIO_DAC, "aud_dac", "audio_sel",
		25),
	GATE_AUDIO0(AUDIO_DAC_PREDIS, "aud_dac_predis", "audio_sel",
		26),
	GATE_AUDIO0(AUDIO_TML, "aud_tml", "audio_sel",
		27),
	/* AUDIO1: hf_faudio_ck/hf_faud_engen1_ck/hf_faud_engen2_ck */
	GATE_AUDIO1(AUDIO_I2S1_BCLK_SW, "aud_i2s1_bclk", "audio_sel",
		4),
	GATE_AUDIO1(AUDIO_I2S2_BCLK_SW, "aud_i2s2_bclk", "audio_sel",
		5),
	GATE_AUDIO1(AUDIO_I2S3_BCLK_SW, "aud_i2s3_bclk", "audio_sel",
		6),
	GATE_AUDIO1(AUDIO_I2S4_BCLK_SW, "aud_i2s4_bclk", "audio_sel",
		7),
};

static const struct mtk_gate_regs cam_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_CAM(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &cam_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate cam_clks[] __initconst = {
	GATE_CAM(CAMSYS_LARB2_CGPDN, "camsys_larb2", "cam_sel", 0),
	GATE_CAM(CAMSYS_DFP_VAD_CGPDN, "camsys_dfp_vad", "cam_sel", 1),
	GATE_CAM(CAMSYS_CAM_CGPDN, "camsys_cam", "cam_sel", 6),
	GATE_CAM(CAMSYS_CAMTG_CGPDN, "camsys_camtg", "cam_sel", 7),
	GATE_CAM(CAMSYS_SENINF_CGPDN, "camsys_seninf", "cam_sel", 8),
	GATE_CAM(CAMSYS_CAMSV0_CGPDN, "camsys_camsv0", "cam_sel", 9),
	GATE_CAM(CAMSYS_CAMSV1_CGPDN, "camsys_camsv1", "cam_sel", 10),
	GATE_CAM(CAMSYS_CAMSV2_CGPDN, "camsys_camsv2", "cam_sel", 11),
	GATE_CAM(CAMSYS_CCU_CGPDN, "camsys_ccu", "cam_sel", 12),
};

static const struct mtk_gate_regs img_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_IMG(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &img_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate img_clks[] __initconst = {
	GATE_IMG(IMG_LARB5, "imgsys_larb5", "mm_sel", 0),
	GATE_IMG(IMG_DFP_VAD, "imgsys_dfp_vad", "mm_sel", 1),
	GATE_IMG(IMG_DIP, "imgsys_dip", "mm_sel", 6),
	GATE_IMG(IMG_DPE, "imgsys_dpe", "mm_sel", 10),
	GATE_IMG(IMG_FDVT, "imgsys_fdvt", "mm_sel", 11),
	GATE_IMG(IMG_GEPF, "imgsys_gepf", "mm_sel", 12),
	GATE_IMG(IMG_RSC, "imgsys_rsc", "mm_sel", 13),
};

static const struct mtk_gate_regs mm0_cg_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs mm1_cg_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

#define GATE_MM0(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &mm0_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_MM1(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &mm1_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mm_clks[] __initconst = {
	/* MM0 */
	GATE_MM0(MMSYS_SMI_COMMON, "mm_smi_common", "mm_sel", 0),
	GATE_MM0(MMSYS_SMI_LARB0, "mm_smi_larb0", "mm_sel", 1),
	GATE_MM0(MMSYS_SMI_LARB1, "mm_smi_larb1", "mm_sel", 2),
	GATE_MM0(MMSYS_GALS_COMM0, "mm_gals_comm0", "mm_sel", 3),
	GATE_MM0(MMSYS_GALS_COMM1, "mm_gals_comm1", "mm_sel", 4),
	GATE_MM0(MMSYS_GALS_VENC2MM, "mm_gals_venc2mm", "mm_sel", 5),
	GATE_MM0(MMSYS_GALS_VDEC2MM, "mm_gals_vdec2mm", "mm_sel", 6),
	GATE_MM0(MMSYS_GALS_IMG2MM, "mm_gals_img2mm", "mm_sel", 7),
	GATE_MM0(MMSYS_GALS_CAM2MM, "mm_gals_cam2mm", "mm_sel", 8),
	GATE_MM0(MMSYS_GALS_IPU2MM, "mm_gals_ipu2mm", "mm_sel", 9),
	GATE_MM0(MMSYS_MDP_DL_TXCK, "mm_mdp_dl_txck", "mm_sel", 10),
	GATE_MM0(MMSYS_IPU_DL_TXCK, "mm_ipu_dl_txck", "mm_sel", 11),
	GATE_MM0(MMSYS_MDP_RDMA0, "mm_mdp_rdma0", "mm_sel", 12),
	GATE_MM0(MMSYS_MDP_RDMA1, "mm_mdp_rdma1", "mm_sel", 13),
	GATE_MM0(MMSYS_MDP_RSZ0, "mm_mdp_rsz0", "mm_sel", 14),
	GATE_MM0(MMSYS_MDP_RSZ1, "mm_mdp_rsz1", "mm_sel", 15),
	GATE_MM0(MMSYS_MDP_TDSHP, "mm_mdp_tdshp", "mm_sel", 16),
	GATE_MM0(MMSYS_MDP_WROT0, "mm_mdp_wrot0", "mm_sel", 17),
	GATE_MM0(MMSYS_MDP_WDMA, "mm_mdp_wdma", "mm_sel", 18),
	GATE_MM0(MMSYS_FAKE_ENG, "mm_fake_eng", "mm_sel", 19),
	GATE_MM0(MMSYS_DISP_OVL0, "mm_disp_ovl0", "mm_sel", 20),
	GATE_MM0(MMSYS_DISP_OVL0_2L, "mm_disp_ovl0_2l", "mm_sel", 21),
	GATE_MM0(MMSYS_DISP_OVL1_2L, "mm_disp_ovl1_2l", "mm_sel", 22),
	GATE_MM0(MMSYS_DISP_RDMA0, "mm_disp_rdma0", "mm_sel", 23),
	GATE_MM0(MMSYS_DISP_RDMA1, "mm_disp_rdma1", "mm_sel", 24),
	GATE_MM0(MMSYS_DISP_WDMA0, "mm_disp_wdma0", "mm_sel", 25),
	GATE_MM0(MMSYS_DISP_COLOR0, "mm_disp_color0", "mm_sel", 26),
	GATE_MM0(MMSYS_DISP_CCORR0, "mm_disp_ccorr0", "mm_sel", 27),
	GATE_MM0(MMSYS_DISP_AAL0, "mm_disp_aal0", "mm_sel", 28),
	GATE_MM0(MMSYS_DISP_GAMMA0, "mm_disp_gamma0", "mm_sel", 29),
	GATE_MM0(MMSYS_DISP_DITHER0, "mm_disp_dither0", "mm_sel", 30),
	GATE_MM0(MMSYS_DISP_SPLIT, "mm_disp_split", "mm_sel", 31),
	/* MM1 */
	GATE_MM1(MMSYS_DSI0_MM_CK, "mm_dsi0_mmck", "mm_sel", 0),
	GATE_MM1(MMSYS_DSI0_IF_CK, "mm_dsi0_ifck", "mm_sel", 1),/* should mipipll1 */
	GATE_MM1(MMSYS_DPI_MM_CK, "mm_dpi_mmck", "mm_sel", 2),
	GATE_MM1(MMSYS_DPI_IF_CK, "mm_dpi_ifck", "mm_sel", 3),/* should mipipll2 */
	GATE_MM1(MMSYS_FAKE_ENG2, "mm_fake_eng2", "mm_sel", 4),
	GATE_MM1(MMSYS_MDP_DL_RX_CK, "mm_mdp_dl_rxck", "mm_sel", 5),
	GATE_MM1(MMSYS_IPU_DL_RX_CK, "mm_ipu_dl_rxck", "mm_sel", 6),
	GATE_MM1(MMSYS_26M, "mm_26m", "f_f26m_ck", 7),
	GATE_MM1(MMSYS_MMSYS_R2Y, "mm_mmsys_r2y", "mm_sel", 8),
	GATE_MM1(MMSYS_DISP_RSZ, "mm_disp_rsz", "mm_sel", 9),
};

static const struct mtk_gate_regs venc_global_con_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_VENC_GLOBAL_CON(_id, _name, _parent, _shift) {	\
		.id = _id,					\
		.name = _name,					\
		.parent_name = _parent,				\
		.regs = &venc_global_con_cg_regs,		\
		.shift = _shift,				\
		.ops = &mtk_clk_gate_ops_setclr_inv,			\
	}

static const struct mtk_gate venc_global_con_clks[] __initconst = {
	GATE_VENC_GLOBAL_CON(VENC_GCON_LARB, "venc_larb",
		"mm_sel", 0),
	GATE_VENC_GLOBAL_CON(VENC_GCON_VENC, "venc_venc",
		"mm_sel", 4),
	GATE_VENC_GLOBAL_CON(VENC_GCON_JPGENC, "venc_jpgenc",
		"mm_sel", 8),
	GATE_VENC_GLOBAL_CON(VENC_GCON_VDEC, "venc_vdec",
		"mm_sel", 12),
};

#if 0
static bool timer_ready;
static struct clk_onecell_data *top_data;
static struct clk_onecell_data *pll_data;

static void mtk_clk_enable_critical(void)
{
	if (!timer_ready || !top_data || !pll_data)
		return;
#if 0
	clk_prepare_enable(top_data->clks[CLK_TOP_AXI_SEL]);
	clk_prepare_enable(top_data->clks[CLK_TOP_MEM_SEL]);
	clk_prepare_enable(top_data->clks[CLK_TOP_DDRPHYCFG_SEL]);
	clk_prepare_enable(top_data->clks[CLK_TOP_RTC_SEL]);
#endif
}
#endif
static void __init mtk_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(TOP_NR_CLK);

	mtk_clk_register_fixed_clks(fixed_clks, ARRAY_SIZE(fixed_clks), clk_data);

	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
	mtk_clk_register_mux_clr_set_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
		&mt6763_clk_lock, clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	cksys_base = base;

	clk_writel(CLK_SCP_CFG_0, clk_readl(CLK_SCP_CFG_0) | 0x3FF);
	clk_writel(CLK_SCP_CFG_1, clk_readl(CLK_SCP_CFG_1) | 0x11);

}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,topckgen", mtk_topckgen_init);

static void __init mtk_infracfg_ao_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(INFRACFG_AO_NR_CLK);

	mtk_clk_register_gates(node, infra_clks, ARRAY_SIZE(infra_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	infracfg_base = base;
	/*mtk_clk_enable_critical();*/
#if MT_CCF_BRINGUP
	clk_writel(INFRA_PDN_SET0, INFRA_CG0);
	clk_writel(INFRA_PDN_SET1, INFRA_CG1);
	clk_writel(INFRA_PDN_SET2, INFRA_CG2);
#else
#endif
}
CLK_OF_DECLARE(mtk_infracfg_ao, "mediatek,infracfg_ao",
		mtk_infracfg_ao_init);

/* FIXME: modify FMAX */
#define MT6763_PLL_FMAX		(3200UL * MHZ)

#define CON0_MT6763_RST_BAR	BIT(24)

#define PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg,	\
			_pcw_shift, _div_table) {			\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT6763_RST_BAR,			\
		.fmax = MT6763_PLL_FMAX,				\
		.pcwbits = _pcwbits,					\
		.pd_reg = _pd_reg,					\
		.pd_shift = _pd_shift,					\
		.tuner_reg = _tuner_reg,				\
		.pcw_reg = _pcw_reg,					\
		.pcw_shift = _pcw_shift,				\
		.div_table = _div_table,				\
	}

#define PLL(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg,	\
			_pcw_shift)					\
		PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits, \
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg, _pcw_shift, \
			NULL)

static const struct mtk_pll_data plls[] = {
	/* FIXME: need to fix flags/div_table/tuner_reg/table */
	PLL(APMIXED_MAINPLL, "mainpll", 0x0220, 0x022C, 0x00000001, 0,
		22, 0x0224, 24, 0x0, 0x0224, 0),

	PLL(APMIXED_UNIVPLL, "univpll", 0x0230, 0x023C, 0x00000001, 0,
		22, 0x0234, 24, 0x0, 0x0234, 0),

	PLL(APMIXED_MFGPLL, "mfgpll", 0x0240, 0x024C, 0x00000001, 0,
		22, 0x0244, 24, 0x0, 0x0244, 0),

	PLL(APMIXED_MSDCPLL, "msdcpll", 0x0250, 0x025C, 0x00000001, 0,
		22, 0x0254, 24, 0x0, 0x0254, 0),

	PLL(APMIXED_TVDPLL, "tvdpll", 0x0260, 0x026C, 0x00000001, 0,
		22, 0x0264, 24, 0x0, 0x0264, 0),

	PLL(APMIXED_MMPLL, "mmpll", 0x0270, 0x027C, 0x00000001, 0,
		22, 0x0274, 24, 0x0, 0x0274, 0),

	PLL(APMIXED_APLL1, "apll1", 0x02A0, 0x02B0, 0x00000001, 0,
		32, 0x02A0, 1, 0x0, 0x02A4, 0),

	PLL(APMIXED_APLL2, "apll2", 0x02b4, 0x02c4, 0x00000001, 0,
		32, 0x02b4, 1, 0x0, 0x02b8, 0),
};

static void __init mtk_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(APMIXED_NR_CLK);

	/* FIXME: add code for APMIXEDSYS */
	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls), clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	apmixed_base = base;
	/*clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xee2b8ae2);*//* ARMPLL4, MPLL, CCIPLL, EMIPLL, MAINPLL */
	/*clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xee2b8ae2);*/
	clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0x007d5550);/* MPLL, CCIPLL, MAINPLL, TDCLKSQ, CLKSQ1 */
	clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0x7f5);
#if 0
/*MFGPLL*/
	clk_clrl(MFGPLL_CON0, PLL_EN);
	clk_setl(MFGPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MFGPLL_PWR_CON0, PLL_PWR_ON);
#endif
/*UNIVPLL*/
	clk_clrl(UNIVPLL_CON0, PLL_EN);
	clk_setl(UNIVPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(UNIVPLL_PWR_CON0, PLL_PWR_ON);
/*MSDCPLL*/
	clk_clrl(MSDCPLL_CON0, PLL_EN);
	clk_setl(MSDCPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MSDCPLL_PWR_CON0, PLL_PWR_ON);
/*MMPLL*/
	clk_clrl(MMPLL_CON0, PLL_EN);
	clk_setl(MMPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MMPLL_PWR_CON0, PLL_PWR_ON);
/*TVDPLL*/
	clk_clrl(TVDPLL_CON0, PLL_EN);
	clk_setl(TVDPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(TVDPLL_PWR_CON0, PLL_PWR_ON);
/*APLL1*/
	clk_clrl(APLL1_CON0, PLL_EN);
	clk_setl(APLL1_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL1_PWR_CON0, PLL_PWR_ON);
/*APLL2*/
	clk_clrl(APLL2_CON0, PLL_EN);
	clk_setl(APLL2_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL2_PWR_CON0, PLL_PWR_ON);

}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,apmixed",
		mtk_apmixedsys_init);


static void __init mtk_audio_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(AUDIO_NR_CLK);

	mtk_clk_register_gates(node, audio_clks, ARRAY_SIZE(audio_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	audio_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(AUDIO_TOP_CON0, AUDIO_DISABLE_CG0);*/
	/*clk_writel(AUDIO_TOP_CON1, AUDIO_DISABLE_CG1);*/
#endif

}
CLK_OF_DECLARE(mtk_audio, "mediatek,audio", mtk_audio_init);

static void __init mtk_camsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(CAMSYS_NR_CLK);

	mtk_clk_register_gates(node, cam_clks, ARRAY_SIZE(cam_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	cam_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(CAMSYS_CG_CLR, CAMSYS_CG);*/
#else
	clk_writel(CAMSYS_CG_SET, CAMSYS_CG);
#endif
}
CLK_OF_DECLARE(mtk_camsys, "mediatek,camsys", mtk_camsys_init);

static void __init mtk_imgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(IMG_NR_CLK);

	mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	img_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(IMG_CG_CLR, IMG_CG);*/
#else
	clk_writel(IMG_CG_SET, IMG_CG);
#endif
}
CLK_OF_DECLARE(mtk_imgsys, "mediatek,imgsys", mtk_imgsys_init);

static void __init mtk_mfg_cfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(MFGCFG_NR_CLK);

	mtk_clk_register_gates(node, mfg_cfg_clks, ARRAY_SIZE(mfg_cfg_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	mfgcfg_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(MFG_CG_CLR, MFG_CG);*/
#else
	clk_writel(MFG_CG_SET, MFG_CG);
#endif

}
CLK_OF_DECLARE(mtk_mfg_cfg, "mediatek,mfgcfg", mtk_mfg_cfg_init);

static void __init mtk_mmsys_config_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(MMSYS_CONFIG_NR_CLK);

	mtk_clk_register_gates(node, mm_clks, ARRAY_SIZE(mm_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	mmsys_config_base = base;
#if MT_CCF_BRINGUP
	/*clk_writel(MM_CG_CLR0, MM_CG0);*/
	/*clk_writel(MM_CG_CLR1, MM_CG1);*/
#else
	clk_writel(MM_CG_SET0, MM_CG0);
	clk_writel(MM_CG_SET1, MM_CG1);
#endif
}
CLK_OF_DECLARE(mtk_mmsys_config, "mediatek,mmsys_config",
		mtk_mmsys_config_init);

static void __init mtk_venc_global_con_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(VENC_GCON_NR_CLK);

	mtk_clk_register_gates(node, venc_global_con_clks, ARRAY_SIZE(venc_global_con_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	venc_gcon_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(VENC_CG_SET, VENC_CG);*/
#else
	clk_writel(VENC_CG_CLR, VENC_CG);
#endif
}
CLK_OF_DECLARE(mtk_venc_global_con, "mediatek,venc_gcon",
		mtk_venc_global_con_init);


unsigned int mt_get_ckgen_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1 = 0;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFFC0FC)|(ID << 8)|(0x1));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));

	clk26cali_1 = clk_readl(CLK26CALI_1);
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
	mdelay(10);
	i++;
	if (i > 10000)
		break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024;

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	/*clk_writel(CLK26CALI_0, clk26cali_0);*/
	/*clk_writel(CLK26CALI_1, clk26cali_1);*/

	/*print("ckgen meter[%d] = %d Khz\n", ID, output);*/
	if (i > 10000)
		return 0;
	else
		return output;

}

unsigned int mt_get_abist_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1 = 0;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC)|(ID << 16));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF) | (1 << 24));

	clk26cali_1 = clk_readl(CLK26CALI_1);

	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10000)
		break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024;

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	/*clk_writel(CLK26CALI_0, clk26cali_0);*/
	/*clk_writel(CLK26CALI_1, clk26cali_1);*/

	/*pr_debug("%s = %d Khz\n", abist_array[ID-1], output);*/
	if (i > 10000)
		return 0;
	else
		return (output * 2);
}

#if 1
void mp_enter_suspend(int id, int suspend)
{
	/* mp0*/
	if (id == 0) {
		if (suspend) {
			/* mp0 enter suspend */
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xff87ffff);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xffffdfff);
		} else {
			/* mp0 leave suspend */
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x780000);/* bit[22:19]*/
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x2000);/* bit[13] */
		}
	} else if (id == 1) { /* mp1 */
		if (suspend) {
			/* mp1 enter suspend */
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xfffeeeef);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xfffffffe);
		} else {
			/* mp1 leave suspend */
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x00011110);/* bit[16][12][8][4] */
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x00000001);/* bit[0]*/
		}
	}
}
#endif

void pll_if_on(void)
{
	if (clk_readl(UNIVPLL_CON0) & 0x1)
		pr_err("suspend warning: UNIVPLL is on!!!\n");
	if (clk_readl(MFGPLL_CON0) & 0x1)
		pr_err("suspend warning: MFGPLL is on!!!\n");
	if (clk_readl(MMPLL_CON0) & 0x1)
		pr_err("suspend warning: MMPLL is on!!!\n");
	if (clk_readl(MSDCPLL_CON0) & 0x1)
		pr_err("suspend warning: MSDCPLL is on!!!\n");
	if (clk_readl(TVDPLL_CON0) & 0x1)
		pr_err("suspend warning: TVDPLL is on!!!\n");
	if (clk_readl(APLL1_CON0) & 0x1)
		pr_err("suspend warning: APLL1 is on!!!\n");
	if (clk_readl(APLL2_CON0) & 0x1)
		pr_err("suspend warning: APLL2 is on!!!\n");
}

void clock_force_off(void)
{
	/*DISP CG*/
	clk_writel(MM_CG_SET0, MM_CG0);
	clk_writel(MM_CG_SET1, MM_CG1);
	/*AUDIO*/
	/*clk_writel(AUDIO_TOP_CON0, AUDIO_CG0);*/
	/*clk_writel(AUDIO_TOP_CON1, AUDIO_CG1);*/
	/*MFG*/
	clk_writel(MFG_CG_SET, MFG_CG);
	/*ISP*/
	clk_writel(IMG_CG_SET, IMG_CG);
	/*VENC inverse*/
	clk_writel(VENC_CG_CLR, VENC_CG);
	/*CAM*/
	clk_writel(CAMSYS_CG_SET, CAMSYS_CG);
}

void pll_force_off(void)
{
/*MFGPLL*/
	clk_clrl(MFGPLL_CON0, PLL_EN);
	clk_setl(MFGPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MFGPLL_PWR_CON0, PLL_PWR_ON);
/*UNIVPLL*/
	clk_clrl(UNIVPLL_CON0, PLL_EN);
	clk_setl(UNIVPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(UNIVPLL_PWR_CON0, PLL_PWR_ON);
/*MSDCPLL*/
	clk_clrl(MSDCPLL_CON0, PLL_EN);
	clk_setl(MSDCPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MSDCPLL_PWR_CON0, PLL_PWR_ON);
/*MMPLL*/
	clk_clrl(MMPLL_CON0, PLL_EN);
	clk_setl(MMPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MMPLL_PWR_CON0, PLL_PWR_ON);
/*TVDPLL*/
	clk_clrl(TVDPLL_CON0, PLL_EN);
	clk_setl(TVDPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(TVDPLL_PWR_CON0, PLL_PWR_ON);
/*APLL1*/
	clk_clrl(APLL1_CON0, PLL_EN);
	clk_setl(APLL1_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL1_PWR_CON0, PLL_PWR_ON);
/*APLL2*/
	clk_clrl(APLL2_CON0, PLL_EN);
	clk_setl(APLL2_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL2_PWR_CON0, PLL_PWR_ON);
}

void armpll_control(int id, int on)
{
	if (id == 1) {
		if (on) {
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_PWR_CON0) | 0x01), ARMPLL_LL_PWR_CON0);
			udelay(100);
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_PWR_CON0) & 0xfffffffd), ARMPLL_LL_PWR_CON0);
			udelay(10);
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_CON1) | 0x80000000), ARMPLL_LL_CON1);
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_CON0) | 0x01), ARMPLL_LL_CON0);
			udelay(100);
		} else {
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_CON0) & 0xfffffffe), ARMPLL_LL_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_PWR_CON0) | 0x00000002), ARMPLL_LL_PWR_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL_LL_PWR_CON0) & 0xfffffffe), ARMPLL_LL_PWR_CON0);
		}
	} else if (id == 2) {
		if (on) {
			mt_reg_sync_writel((clk_readl(ARMPLL_L_PWR_CON0) | 0x01), ARMPLL_L_PWR_CON0);
			udelay(100);
			mt_reg_sync_writel((clk_readl(ARMPLL_L_PWR_CON0) & 0xfffffffd), ARMPLL_L_PWR_CON0);
			udelay(10);
			mt_reg_sync_writel((clk_readl(ARMPLL_L_CON1) | 0x80000000), ARMPLL_L_CON1);
			mt_reg_sync_writel((clk_readl(ARMPLL_L_CON0) | 0x01), ARMPLL_L_CON0);
			udelay(100);
		} else {
			mt_reg_sync_writel((clk_readl(ARMPLL_L_CON0) & 0xfffffffe), ARMPLL_L_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL_L_PWR_CON0) | 0x00000002), ARMPLL_L_PWR_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL_L_PWR_CON0) & 0xfffffffe), ARMPLL_L_PWR_CON0);
		}
	}
}

void cam_mtcmos_patch(int on)
{
	if (on) {
		/* do something */
		/* do something */
	} else {
		clk_writel(CAMSYS_CG_CLR, 0x0001);
	}
}

static int __init clk_mt6763_init(void)
{
	/*timer_ready = true;*/
	/*mtk_clk_enable_critical();*/

	return 0;
}
arch_initcall(clk_mt6763_init);


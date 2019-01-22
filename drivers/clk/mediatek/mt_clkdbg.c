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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/module.h>
#include <linux/version.h>

#include "mt_clkdbg.h"

#define DUMMY_REG_TEST		0
#define DUMP_INIT_STATE		0
#define CLKDBG_PM_DOMAIN	0
#define CLKDBG_6799		1
#define CLKDBG_6755		0
#define CLKDBG_6755_TK		0
#define CLKDBG_8173		0

#define TAG	"[clkdbg] "

#define clk_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

/************************************************
 **********      register access       **********
 ************************************************/

#ifndef BIT
#define BIT(_bit_)		(u32)(1U << (_bit_))
#endif

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */
#define clk_setl(addr, val)	clk_writel(addr, clk_readl(addr) | (val))
#define clk_clrl(addr, val)	clk_writel(addr, clk_readl(addr) & ~(val))
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

/************************************************
 **********      struct definition     **********
 ************************************************/

#if CLKDBG_6799

static void __iomem *topckgen_base;	/* 0x10210000 */
static void __iomem *infrasys_base;	/* 0x10000000 */
static void __iomem *perisys_base;	/* 0x11010000 */
static void __iomem *mcucfg_base;	/* 0x10200000 */
static void __iomem *apmixedsys_base;	/* 0x10212000 */
static void __iomem *audiosys_base;	/* 0x10c00000 */
static void __iomem *mfgsys_base;	/* 0x13ffe000 */
static void __iomem *mmsys_base;	/* 0x14000000 */
static void __iomem *imgsys_base;	/* 0x15020000 */
static void __iomem *vdecsys_base;	/* 0x16000000 */
static void __iomem *vencsys_base;	/* 0x17000000 */
static void __iomem *scpsys_base;	/* 0x10b00000 */
static void __iomem *camsys_base;	/* 0x18000000 */
static void __iomem *ipusys_base;	/* 0x18800000 */
static void __iomem *mjcsys_base;	/* 0x12000000 */

#define TOPCKGEN_REG(ofs)	(topckgen_base + ofs)
#define INFRA_REG(ofs)		(infrasys_base + ofs)
#define PERI_REG(ofs)		(perisys_base + ofs)
#define MCUCFG_REG(ofs)		(mcucfg_base + ofs)
#define APMIXED_REG(ofs)	(apmixedsys_base + ofs)
#define AUDIO_REG(ofs)		(audiosys_base + ofs)
#define MFG_REG(ofs)		(mfgsys_base + ofs)
#define MM_REG(ofs)		(mmsys_base + ofs)
#define IMG_REG(ofs)		(imgsys_base + ofs)
#define VDEC_REG(ofs)		(vdecsys_base + ofs)
#define VENC_REG(ofs)		(vencsys_base + ofs)
#define SCP_REG(ofs)		(scpsys_base + ofs)
#define CAM_REG(ofs)		(camsys_base + ofs)
#define IPU_REG(ofs)		(ipusys_base + ofs)
#define MJC_REG(ofs)		(mjcsys_base + ofs)

/* TOPCKGEN */
#define CLK_CFG_20		TOPCKGEN_REG(0x210)
#define CLK_CFG_21		TOPCKGEN_REG(0x214)
#define CLK_MISC_CFG_1          TOPCKGEN_REG(0x414)
#define CLK26CALI_0             TOPCKGEN_REG(0x520)
#define CLK26CALI_1             TOPCKGEN_REG(0x524)
#define CLK26CALI_2             TOPCKGEN_REG(0x528)

#define CLK_CFG_0		TOPCKGEN_REG(0x100)
#define CLK_CFG_1		TOPCKGEN_REG(0x110)
#define CLK_CFG_2		TOPCKGEN_REG(0x120)
#define CLK_CFG_3		TOPCKGEN_REG(0x130)
#define CLK_CFG_4		TOPCKGEN_REG(0x140)
#define CLK_CFG_5		TOPCKGEN_REG(0x150)
#define CLK_CFG_6		TOPCKGEN_REG(0x160)
#define CLK_CFG_7		TOPCKGEN_REG(0x170)
#define CLK_CFG_8		TOPCKGEN_REG(0x180)
#define CLK_CFG_9		TOPCKGEN_REG(0x190)
#define CLK_CFG_10              TOPCKGEN_REG(0x1A0)
#define CLK_CFG_11              TOPCKGEN_REG(0x1B0)
#define CLK_CFG_12              TOPCKGEN_REG(0x1C0)
#define CLK_CFG_13              TOPCKGEN_REG(0x1D0)
#define CLK_CFG_14              TOPCKGEN_REG(0x1E0)

/* APMIXEDSYS Register */

#define ARMPLL1_CON0		APMIXED_REG(0x310)
#define ARMPLL1_CON1		APMIXED_REG(0x314)
#define ARMPLL1_PWR_CON0	APMIXED_REG(0x31C)
#define ARMPLL2_CON0		APMIXED_REG(0x320)
#define ARMPLL2_CON1		APMIXED_REG(0x324)
#define ARMPLL2_PWR_CON0	APMIXED_REG(0x32C)
#define ARMPLL3_CON0		APMIXED_REG(0x330)
#define ARMPLL3_CON1		APMIXED_REG(0x334)
#define ARMPLL3_PWR_CON0	APMIXED_REG(0x33C)
#define CCIPLL_CON0		APMIXED_REG(0x350)
#define CCIPLL_CON1		APMIXED_REG(0x354)
#define CCIPLL_PWR_CON0		APMIXED_REG(0x35C)

#define FSMIPLL_CON0		APMIXED_REG(0x200)
#define FSMIPLL_CON1		APMIXED_REG(0x204)
#define FSMIPLL_PWR_CON0	APMIXED_REG(0x20C)

#define GPUPLL_CON0		APMIXED_REG(0x210)
#define GPUPLL_CON1		APMIXED_REG(0x214)
#define GPUPLL_PWR_CON0		APMIXED_REG(0x21C)

#define MAINPLL_CON0		APMIXED_REG(0x230)
#define MAINPLL_CON1		APMIXED_REG(0x234)
#define MAINPLL_PWR_CON0	APMIXED_REG(0x23C)

#define UNIVPLL_CON0		APMIXED_REG(0x240)
#define UNIVPLL_CON1		APMIXED_REG(0x244)
#define UNIVPLL_PWR_CON0	APMIXED_REG(0x24C)

#define MSDCPLL_CON0		APMIXED_REG(0x250)
#define MSDCPLL_CON1		APMIXED_REG(0x254)
#define MSDCPLL_PWR_CON0	APMIXED_REG(0x25C)

#define MMPLL_CON0		APMIXED_REG(0x260)
#define MMPLL_CON1		APMIXED_REG(0x264)
#define MMPLL_PWR_CON0		APMIXED_REG(0x26C)

#define VCODECPLL_CON0		APMIXED_REG(0x270)
#define VCODECPLL_CON1		APMIXED_REG(0x274)
#define VCODECPLL_PWR_CON0	APMIXED_REG(0x27C)

#define TVDPLL_CON0		APMIXED_REG(0x280)
#define TVDPLL_CON1		APMIXED_REG(0x284)
#define TVDPLL_PWR_CON0		APMIXED_REG(0x28C)

#define APLL1_CON0		APMIXED_REG(0x2A0)
#define APLL1_CON1		APMIXED_REG(0x2A4)
#define APLL1_CON2		APMIXED_REG(0x2A8)
#define APLL1_PWR_CON0		APMIXED_REG(0x2B8)

#define APLL2_CON0		APMIXED_REG(0x2C0)
#define APLL2_CON1		APMIXED_REG(0x2C4)
#define APLL2_CON2		APMIXED_REG(0x2C8)
#define APLL2_PWR_CON0		APMIXED_REG(0x2D8)

/* INFRASYS Register */
#define INFRA_PDN0_SET		INFRA_REG(0x0080)
#define INFRA_PDN0_CLR		INFRA_REG(0x0084)
#define INFRA_PDN0_STA		INFRA_REG(0x0090)

#define INFRA_PDN1_SET		INFRA_REG(0x0088)
#define INFRA_PDN1_CLR		INFRA_REG(0x008C)
#define INFRA_PDN1_STA		INFRA_REG(0x0094)

#define INFRA_PDN2_SET		INFRA_REG(0x00A4)
#define INFRA_PDN2_CLR		INFRA_REG(0x00A8)
#define INFRA_PDN2_STA		INFRA_REG(0x00AC)

#define INFRA_PDN3_SET		INFRA_REG(0x00B0)
#define INFRA_PDN3_CLR		INFRA_REG(0x00B4)
#define INFRA_PDN3_STA		INFRA_REG(0x00B8)

#define INFRA_PDN4_SET		INFRA_REG(0x00BC)
#define INFRA_PDN4_CLR		INFRA_REG(0x00C0)
#define INFRA_PDN4_STA		INFRA_REG(0x00C4)

/* PERISYS Register */

#define PERI_PDN0_SET		PERI_REG(0x0270)
#define PERI_PDN0_CLR		PERI_REG(0x0274)
#define PERI_PDN0_STA		PERI_REG(0x0278)

#define PERI_PDN1_SET		PERI_REG(0x0280)
#define PERI_PDN1_CLR		PERI_REG(0x0284)
#define PERI_PDN1_STA		PERI_REG(0x0288)

#define PERI_PDN2_SET		PERI_REG(0x0290)
#define PERI_PDN2_CLR		PERI_REG(0x0294)
#define PERI_PDN2_STA		PERI_REG(0x0298)

#define PERI_PDN3_SET		PERI_REG(0x02A0)
#define PERI_PDN3_CLR		PERI_REG(0x02A4)
#define PERI_PDN3_STA		PERI_REG(0x02A8)

#define PERI_PDN4_SET		PERI_REG(0x02B0)
#define PERI_PDN4_CLR		PERI_REG(0x02B4)
#define PERI_PDN4_STA		PERI_REG(0x02B8)

#define PERI_PDN5_SET		PERI_REG(0x02C0)
#define PERI_PDN5_CLR		PERI_REG(0x02C4)
#define PERI_PDN5_STA		PERI_REG(0x02C8)

/* Audio Register */

#define AUDIO_TOP_CON0		AUDIO_REG(0x0000)
#define AUDIO_TOP_CON1		AUDIO_REG(0x0004)

/* MFGCFG Register */

#define MFG_CG_CON		MFG_REG(0)
#define MFG_CG_SET		MFG_REG(4)
#define MFG_CG_CLR		MFG_REG(8)

/* CAMSYS Register */

#define CAM_CG_CON		CAM_REG(0)
#define CAM_CG_SET		CAM_REG(4)
#define CAM_CG_CLR		CAM_REG(8)

/* MMSYS Register */

#define DISP_CG_CON0		MM_REG(0x100)
#define DISP_CG_SET0		MM_REG(0x104)
#define DISP_CG_CLR0		MM_REG(0x108)
#define DISP_CG_CON1		MM_REG(0x110)
#define DISP_CG_SET1		MM_REG(0x114)
#define DISP_CG_CLR1		MM_REG(0x118)
#define DISP_CG_CON2		MM_REG(0x140)
#define DISP_CG_SET2		MM_REG(0x144)
#define DISP_CG_CLR2		MM_REG(0x148)

/* IMGSYS Register */
#define IMG_CG_CON		IMG_REG(0x0000)
#define IMG_CG_SET		IMG_REG(0x0004)
#define IMG_CG_CLR		IMG_REG(0x0008)

/* MJCSYS Register */
#define MJC_CG_CON		MJC_REG(0x0000)
#define MJC_CG_SET		MJC_REG(0x0004)
#define MJC_CG_CLR		MJC_REG(0x0008)

/* IPUSYS Register */
#define IPU_CG_CON		IPU_REG(0x0000)
#define IPU_CG_SET		IPU_REG(0x0004)
#define IPU_CG_CLR		IPU_REG(0x0008)

/* VDEC Register */
#define VDEC_CKEN_SET		VDEC_REG(0x0000)
#define VDEC_CKEN_CLR		VDEC_REG(0x0004)
#define LARB_CKEN_SET		VDEC_REG(0x0008)/*LARB1*/
#define LARB_CKEN_CLR		VDEC_REG(0x000C)/*LARB1*/

/* VENC Register */
#define VENC_CG_CON		VENC_REG(0x0)
#define VENC_CG_SET		VENC_REG(0x4)
#define VENC_CG_CLR		VENC_REG(0x8)

/* SCPSYS Register */

#define SPM_PWR_STATUS		SCP_REG(0x0190)
#define SPM_PWR_STATUS_2ND	SCP_REG(0x0194)
#define SPM_MFG0_PWR_CON	SCP_REG(0x0300)
#define SPM_MFG1_PWR_CON	SCP_REG(0x0304)
#define SPM_MFG2_PWR_CON	SCP_REG(0x0308)
#define SPM_MFG3_PWR_CON	SCP_REG(0x030C)
#define SPM_C2K_PWR_CON		SCP_REG(0x0310)
#define SPM_MD1_PWR_CON		SCP_REG(0x0314)
#define SPM_DPY_CH0_PWR_CON	SCP_REG(0x0318)
#define SPM_DPY_CH1_PWR_CON	SCP_REG(0x031C)
#define SPM_DPY_CH2_PWR_CON	SCP_REG(0x0320)
#define SPM_DPY_CH3_PWR_CON	SCP_REG(0x0324)
#define SPM_INFRA_PWR_CON	SCP_REG(0x0328)
#define SPM_AUD_PWR_CON		SCP_REG(0x0330)
#define SPM_MJC_PWR_CON		SCP_REG(0x0334)
#define SPM_MM0_PWR_CON		SCP_REG(0x0338)
#define SPM_CAM_PWR_CON		SCP_REG(0x0340)
#define SPM_IPU_PWR_CON		SCP_REG(0x0344)
#define SPM_ISP_PWR_CON		SCP_REG(0x0348)
#define SPM_VEN_PWR_CON		SCP_REG(0x034C)
#define SPM_VDE_PWR_CON		SCP_REG(0x0350)
#define SPM_EXT_BUCK_ISO	SCP_REG(0x0394)
#define SPM_IPU_SRAM_CON	SCP_REG(0x03A0)

#endif /* CLKDBG_SOC */

static bool is_valid_reg(void __iomem *addr)
{
	#ifndef CONFIG_ARM64
		return 1;
	#else
		return ((u64)addr & 0xf0000000) || (((u64)addr >> 32) & 0xf0000000);
	#endif
}

enum FMETER_TYPE {
	ABIST,
	CKGEN
};

enum ABIST_CLK {
	ABIST_CLK_NULL,

#if CLKDBG_6799
	AD_MDPLL_FS26M_CK	= 1,
	AD_MDPLL_FS208M_CK	= 2,
	AD_OSC_CK	= 3,
	AD_CSI0A_CDPHY_DELAYCAL_CK	= 4,
	AD_CSI0B_CDPHY_DELAYCAL_CK	= 5,
	AD_CSI1A_CDPHY_DELAYCAL_CK	= 6,
	AD_CSI1B_CDPHY_DELAYCAL_CK	= 7,
	AD_CSI2A_CDPHY_DELAYCAL_CK	= 8,
	AD_CSI2B_CDPHY_DELAYCAL_CK	= 9,
	AD_DSI0_CKG_DSICLK	= 10,
	AD_DSI0_TEST_CK	= 11,
	AD_DSI1_CKG_DSICLK	= 12,
	AD_DSI1_TEST_CK	= 13,
	AD_CCIPLL_CK_VCORE	= 14,
	AD_MAINPLL_CK	= 15,
	AD_UNIVPLL_CK	= 16,
	AD_MSDCPLL_CK	= 17,
	AD_EMIPLL_CK	= 18,
	AD_GPUPLL_CK	= 19,
	AD_TVDPLL_CK	= 20,
	AD_MMPLL_CK	= 21,
	AD_VCODECPLL_CK	= 22,
	AD_APLL1_CK	= 23,
	AD_APLL2_CK	= 24,
	AD_MDMCUPLL_CK	= 25,
	AD_MDINFRAPLL_CK	= 26,
	AD_BRPPLL_CK	= 27,
	AD_EQPLL_CK	= 28,
	AD_IMCPLL_CK	= 29,
	AD_ICCPLL_CK	= 30,
	AD_MPCPLL_CK	= 31,
	AD_DFEPLL_CK	= 32,
	AD_MD2GPLL_CK	= 33,
	AD_INTFPLL_CK	= 34,
	AD_C2KCPPLL_CK	= 35,
	AD_FSMIPLL_CK	= 36,
	AD_LTXBPLL_CK	= 37,
	AD_USB20_192M_CK	= 38,
	AD_MPLL_CK	= 39,
	AD_ARMPLL2_CK	= 40,
	AD_ARMPLL3_CK	= 41,
	AD_CCIPLL_CK	= 42,
	AD_RAKEPLL_CK	= 43,
	AD_CSPLL_CK	= 44,
	AD_TVDPLL_DIV4_CK	= 45,
	AD_PLLGP_TSTDIV2_CK	= 46,
	AD_MP_PLL_CK_ABIST_OUT	= 47,
	AD_MP_RX0_TSTCK_DIV2	= 48,
	mp_tx_mon_div2_ck	= 49,
	mp_rx20_mon_div2_ck	= 50,
	AD_ARMPLL_L_CK_VCORE	= 51,
	AD_ARMPLL_M_CK_VCORE	= 52,
	AD_ARMPLL_B_CK_VCORE	= 53,
	AD_ARMPLL_ML_CK_VCORE	= 54,
	AD_OSC_SYNC_CK	= 55,
	AD_OSC_SYNC_CK_2	= 56,
	msdc_01_in_ck	= 57,
	msdc_02_in_ck	= 58,
	msdc_11_in_ck	= 59,
	msdc_12_in_ck	= 60,
	msdc_31_in_ck	= 61,
	msdc_31_in_ck2	= 62,
	AD_OSC_CK_2	= 63,
#endif /* CLKDBG_6799 */

	ABIST_CLK_END,
};

static const char * const ABIST_CLK_NAME[] = {
#if CLKDBG_6799
	[AD_MDPLL_FS26M_CK] = "AD_MDPLL_FS26M_CK",
	[AD_MDPLL_FS208M_CK] = "AD_MDPLL_FS208M_CK",
	[AD_OSC_CK] = "AD_OSC_CK",
	[AD_CSI0A_CDPHY_DELAYCAL_CK] = "AD_CSI0A_CDPHY_DELAYCAL_CK",
	[AD_CSI0B_CDPHY_DELAYCAL_CK] = "AD_CSI0B_CDPHY_DELAYCAL_CK",
	[AD_CSI1A_CDPHY_DELAYCAL_CK] = "AD_CSI1A_CDPHY_DELAYCAL_CK",
	[AD_CSI1B_CDPHY_DELAYCAL_CK] = "AD_CSI1B_CDPHY_DELAYCAL_CK",
	[AD_CSI2A_CDPHY_DELAYCAL_CK] = "AD_CSI2A_CDPHY_DELAYCAL_CK",
	[AD_CSI2B_CDPHY_DELAYCAL_CK] = "AD_CSI2B_CDPHY_DELAYCAL_CK",
	[AD_DSI0_CKG_DSICLK] = "AD_DSI0_CKG_DSICLK",
	[AD_DSI0_TEST_CK] = "AD_DSI0_TEST_CK",
	[AD_DSI1_CKG_DSICLK] = "AD_DSI1_CKG_DSICLK",
	[AD_DSI1_TEST_CK] = "AD_DSI1_TEST_CK",
	[AD_CCIPLL_CK_VCORE] = "AD_CCIPLL_CK_VCORE",
	[AD_MAINPLL_CK] = "AD_MAINPLL_CK",
	[AD_UNIVPLL_CK] = "AD_UNIVPLL_CK",
	[AD_MSDCPLL_CK] = "AD_MSDCPLL_CK",
	[AD_EMIPLL_CK] = "AD_EMIPLL_CK",
	[AD_GPUPLL_CK] = "AD_GPUPLL_CK",
	[AD_TVDPLL_CK] = "AD_TVDPLL_CK",
	[AD_MMPLL_CK] = "AD_MMPLL_CK",
	[AD_VCODECPLL_CK] = "AD_VCODECPLL_CK",
	[AD_APLL1_CK] = "AD_APLL1_CK",
	[AD_APLL2_CK] = "AD_APLL2_CK",
	[AD_MDMCUPLL_CK] = "AD_MDMCUPLL_CK",
	[AD_MDINFRAPLL_CK] = "AD_MDINFRAPLL_CK",
	[AD_BRPPLL_CK] = "AD_BRPPLL_CK",
	[AD_EQPLL_CK] = "AD_EQPLL_CK",
	[AD_IMCPLL_CK] = "AD_IMCPLL_CK",
	[AD_ICCPLL_CK] = "AD_ICCPLL_CK",
	[AD_MPCPLL_CK] = "AD_MPCPLL_CK",
	[AD_DFEPLL_CK] = "AD_DFEPLL_CK",
	[AD_MD2GPLL_CK] = "AD_MD2GPLL_CK",
	[AD_INTFPLL_CK] = "AD_INTFPLL_CK",
	[AD_C2KCPPLL_CK] = "AD_C2KCPPLL_CK",
	[AD_FSMIPLL_CK] = "AD_FSMIPLL_CK",
	[AD_LTXBPLL_CK] = "AD_LTXBPLL_CK",
	[AD_USB20_192M_CK] = "AD_USB20_192M_CK",
	[AD_MPLL_CK] = "AD_MPLL_CK",
	[AD_ARMPLL2_CK] = "AD_ARMPLL2_CK",
	[AD_ARMPLL3_CK] = "AD_ARMPLL3_CK",
	[AD_CCIPLL_CK] = "AD_CCIPLL_CK",
	[AD_RAKEPLL_CK] = "AD_RAKEPLL_CK",
	[AD_CSPLL_CK] = "AD_CSPLL_CK",
	[AD_TVDPLL_DIV4_CK] = "AD_TVDPLL_DIV4_CK",
	[AD_PLLGP_TSTDIV2_CK] = "AD_PLLGP_TSTDIV2_CK",
	[AD_MP_PLL_CK_ABIST_OUT] = "AD_MP_PLL_CK_ABIST_OUT",
	[AD_MP_RX0_TSTCK_DIV2] = "AD_MP_RX0_TSTCK_DIV2",
	[mp_tx_mon_div2_ck] = "mp_tx_mon_div2_ck",
	[mp_rx20_mon_div2_ck] = "mp_rx20_mon_div2_ck",
	[AD_ARMPLL_L_CK_VCORE] = "AD_ARMPLL_L_CK_VCORE",
	[AD_ARMPLL_M_CK_VCORE] = "AD_ARMPLL_M_CK_VCORE",
	[AD_ARMPLL_B_CK_VCORE] = "AD_ARMPLL_B_CK_VCORE",
	[AD_ARMPLL_ML_CK_VCORE] = "AD_ARMPLL_ML_CK_VCORE",
	[AD_OSC_SYNC_CK] = "AD_OSC_SYNC_CK",
	[AD_OSC_SYNC_CK_2] = "AD_OSC_SYNC_CK_2",
	[msdc_01_in_ck] = "msdc_01_in_ck",
	[msdc_02_in_ck] = "msdc_02_in_ck",
	[msdc_11_in_ck] = "msdc_11_in_ck",
	[msdc_12_in_ck] = "msdc_12_in_ck",
	[msdc_31_in_ck] = "msdc_31_in_ck",
	[msdc_31_in_ck2] = "msdc_31_in_ck2",
	[AD_OSC_CK_2] = "AD_OSC_CK_2",
#endif /* CLKDBG_6799 */
};

enum CKGEN_CLK {
	CKGEN_CLK_NULL,

#if CLKDBG_6799
	hf_faxi_ck	= 1,
	hf_fmem_ck	= 2,
	hf_fddrphycfg_ck	= 3,
	hf_fmm_ck	= 4,
	hf_fsflash_ck	= 5,
	f_fpwm_ck	= 6,
	f_fdisppwm_ck	= 7,
	hf_fvdec_ck	= 8,
	hf_fvenc_ck	= 9,
	hf_fmfg_ck	= 10,
	f_fcamtg_ck	= 11,
	f_fi2c_ck	= 12,
	f_fuart_ck	= 13,
	hf_fspi_ck	= 14,
	hf_faxi_peri_ck	= 15,
	f_fusb20_ck	= 16,
	f_fusb30_p0_ck	= 17,
	hf_fmsdc50_0_hclk_ck	= 18,
	hf_fmsdc50_0_ck	= 19,
	hf_fmsdc30_1_ck	= 20,
	f_fi3c_ck	= 21,
	hf_fmsdc30_3_ck	= 22,
	hf_fmsdc50_3_hclk_ck	= 23,
	hf_fsmi0_2x_ck	= 24,
	hf_faudio_ck	= 25,
	hf_faud_intbus_ck	= 26,
	hf_fpmicspi_ck	= 27,
	hf_fscp_ck	= 28,
	hf_fatb_ck	= 29,
	hf_fmjc_ck	= 30,
	hf_fdpi0_ck	= 31,
	hf_fdsp_ck	= 32,
	hf_faud_1_ck	= 33,
	hf_faud_2_ck	= 34,
	hf_faud_engen1_ck	= 35,
	hf_faud_engen2_ck	= 36,
	hf_fdfp_mfg_ck	= 37,
	hf_fcam_ck	= 38,
	hf_fipu_if_ck	= 39,
	hf_fsmi1_2x_ck	= 40,
	hf_faxi_mfg_in_as_ck	= 41,
	hf_fimg_ck	= 42,
	hf_fufo_enc_ck	= 43,
	hf_fufo_dec_ck	= 44,
	hf_fpcie_mac_ck	= 45,
	hf_femi_ck	= 46,
	hf_faes_ufsfde_ck	= 47,
	hf_faes_fde_ck	= 48,
	hf_faudio_h_ck	= 49,
	hf_fpwrmcu_ck	= 50,
	hf_fancmd32_ck	= 51,
	hf_fslow_mfg_ck	= 52,
	hf_fufs_card_ck	= 53,
	hf_fbsi_spi_ck	= 54,
	hf_fdxcc_ck	= 55,
	f_fseninf_ck	= 56,
	hf_fdfp_ck	= 57,
	hf_fsmi1_2x_ck2	= 58,
	hf_faes_fde_ck2	= 59,
	hf_faudio_h_ck2	= 60,
	hf_fpwrmcu_ck2	= 61,
	f_frtc_ck	= 62,
	f_f26m_ck	= 63,
#endif /* CLKDBG_6799*/

	CKGEN_CLK_END,
};

static const char * const CKGEN_CLK_NAME[] = {
#if CLKDBG_6799
	[hf_faxi_ck]	= "hf_faxi_ck",
	[hf_fmem_ck]	= "hf_fmem_ck",
	[hf_fddrphycfg_ck]	= "hf_fddrphycfg_ck",
	[hf_fmm_ck]	= "hf_fmm_ck",
	[hf_fsflash_ck]	= "hf_fsflash_ck",
	[f_fpwm_ck]	= "f_fpwm_ck",
	[f_fdisppwm_ck]	= "f_fdisppwm_ck",
	[hf_fvdec_ck]	= "hf_fvdec_ck",
	[hf_fvenc_ck]	= "hf_fvenc_ck",
	[hf_fmfg_ck]	= "hf_fmfg_ck",
	[f_fcamtg_ck]	= "f_fcamtg_ck",
	[f_fi2c_ck]	= "f_fi2c_ck",
	[f_fuart_ck]	= "f_fuart_ck",
	[hf_fspi_ck]	= "hf_fspi_ck",
	[hf_faxi_peri_ck]	= "hf_faxi_peri_ck",
	[f_fusb20_ck]	= "f_fusb20_ck",
	[f_fusb30_p0_ck]	= "f_fusb30_p0_ck",
	[hf_fmsdc50_0_hclk_ck]	= "hf_fmsdc50_0_hclk_ck",
	[hf_fmsdc50_0_ck]	= "hf_fmsdc50_0_ck",
	[hf_fmsdc30_1_ck]	= "hf_fmsdc30_1_ck",
	[f_fi3c_ck]	= "f_fi3c_ck",
	[hf_fmsdc30_3_ck]	= "hf_fmsdc30_3_ck",
	[hf_fmsdc50_3_hclk_ck]	= "hf_fmsdc50_3_hclk_ck",
	[hf_fsmi0_2x_ck]	= "hf_fsmi0_2x_ck",
	[hf_faudio_ck]	= "hf_faudio_ck",
	[hf_faud_intbus_ck]	= "hf_faud_intbus_ck",
	[hf_fpmicspi_ck]	= "hf_fpmicspi_ck",
	[hf_fscp_ck]	= "hf_fscp_ck",
	[hf_fatb_ck]	= "hf_fatb_ck",
	[hf_fmjc_ck]	= "hf_fmjc_ck",
	[hf_fdpi0_ck]	= "hf_fdpi0_ck",
	[hf_fdsp_ck]	= "hf_fdsp_ck",
	[hf_faud_1_ck]	= "hf_faud_1_ck",
	[hf_faud_2_ck]	= "hf_faud_2_ck",
	[hf_faud_engen1_ck]	= "hf_faud_engen1_ck",
	[hf_faud_engen2_ck]	= "hf_faud_engen2_ck",
	[hf_fdfp_mfg_ck]	= "hf_fdfp_mfg_ck",
	[hf_fcam_ck]	= "hf_fcam_ck",
	[hf_fipu_if_ck]	= "hf_fipu_if_ck",
	[hf_fsmi1_2x_ck]	= "hf_fsmi1_2x_ck",
	[hf_faxi_mfg_in_as_ck]	= "hf_faxi_mfg_in_as_ck",
	[hf_fimg_ck]	= "hf_fimg_ck",
	[hf_fufo_enc_ck]	= "hf_fufo_enc_ck",
	[hf_fufo_dec_ck]	= "hf_fufo_dec_ck",
	[hf_fpcie_mac_ck]	= "hf_fpcie_mac_ck",
	[hf_femi_ck]	= "hf_femi_ck",
	[hf_faes_ufsfde_ck]	= "hf_faes_ufsfde_ck",
	[hf_faes_fde_ck]	= "hf_faes_fde_ck",
	[hf_faudio_h_ck]	= "hf_faudio_h_ck",
	[hf_fpwrmcu_ck]	= "hf_fpwrmcu_ck",
	[hf_fancmd32_ck]	= "hf_fancmd32_ck",
	[hf_fslow_mfg_ck]	= "hf_fslow_mfg_ck",
	[hf_fufs_card_ck]	= "hf_fufs_card_ck",
	[hf_fbsi_spi_ck]	= "hf_fbsi_spi_ck",
	[hf_fdxcc_ck]	= "hf_fdxcc_ck",
	[f_fseninf_ck]	= "f_fseninf_ck",
	[hf_fdfp_ck]	= "hf_fdfp_ck",
	[hf_fsmi1_2x_ck2]	= "hf_fsmi1_2x_ck2",
	[hf_faes_fde_ck2]	= "hf_faes_fde_ck2",
	[hf_faudio_h_ck2]	= "hf_faudio_h_ck2",
	[hf_fpwrmcu_ck2]	= "hf_fpwrmcu_ck2",
	[f_frtc_ck]	= "f_frtc_ck",
	[f_f26m_ck]	= "f_f26m_ck",
#endif /* CLKDBG_6799 */
};

#if CLKDBG_8173

static void set_fmeter_divider_ca7(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_1);

	v = ALT_BITS(v, 15, 8, k1);
	clk_writel(CLK_MISC_CFG_1, v);
}

static void set_fmeter_divider_ca15(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_2);

	v = ALT_BITS(v, 7, 0, k1);
	clk_writel(CLK_MISC_CFG_2, v);
}
#endif /* CLKDBG_6755 */


#if CLKDBG_6755
static void set_fmeter_divider(u32 k1)
{
	u32 v = clk_readl(CLK_MISC_CFG_0);

	v = ALT_BITS(v, 31, 24, k1);
	clk_writel(CLK_MISC_CFG_0, v);
}

static bool wait_fmeter_done(u32 tri_bit)
{
	static int max_wait_count;
	int wait_count = (max_wait_count > 0) ? (max_wait_count * 2 + 2) : 100;
	int i;

	/* wait fmeter */
	for (i = 0; i < wait_count && (clk_readl(CLK26CALI_0) & tri_bit); i++)
		udelay(20);

	if (!(clk_readl(CLK26CALI_0) & tri_bit)) {
		max_wait_count = max(max_wait_count, i);
		return true;
	}

	return false;
}

#endif /* CLKDBG_6755 */
#if 0
static u32 fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
#if 0
	void __iomem *clk_cfg_reg = CLK_DBG_CFG;
	void __iomem *cnt_reg =	CLK26CALI_1;
	u32 cksw_ckgen[] = {13, 8, clk};
	u32 cksw_abist[] = {21, 16, clk};
	u32 *cksw_hlv =		(type == CKGEN) ? cksw_ckgen	: cksw_abist;
	u32 en_bit =	BIT(12);
	u32 tri_bit =	BIT(4);
	u32 clk_misc_cfg_0;
	u32 clk_misc_cfg_1;
	u32 clk_cfg_val;
	u32 freq = 0;

	if (!is_valid_reg(topckgen_base))
		return 0;

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);	/* backup reg value */
	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);
	clk_cfg_val = clk_readl(clk_cfg_reg);
	/* set meter div (0 = /1) */
	set_fmeter_divider(k1);
	/* select fmeter */
	if (type == CKGEN)
		clk_setl(CLK_DBG_CFG, BIT(0));
	else
		clk_clrl(CLK_DBG_CFG, BIT(1));
	/* select meter clock input */
	clk_writel_mask(CLK_DBG_CFG, cksw_hlv[0], cksw_hlv[1], cksw_hlv[2]);
	/* trigger fmeter */
	clk_setl(CLK26CALI_0, en_bit);			/* enable fmeter_en */
	clk_clrl(CLK26CALI_0, tri_bit);			/* start fmeter */
	clk_setl(CLK26CALI_0, en_bit|tri_bit);

	if (wait_fmeter_done(tri_bit)) {
		u32 cnt = clk_readl(cnt_reg) & 0xFFFF;

		freq = (cnt * 26000) / 1024;
	}
	/* restore register settings */
	clk_writel(clk_cfg_reg, clk_cfg_val);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);
	return freq;
#endif
	return 0;
}
#endif
/* k1 = 0 */
static u32 measure_stable_fmeter_freq(enum FMETER_TYPE type, int k1, int clk)
{
	u32 freq;

	if (type == CKGEN)
		freq = mt_get_ckgen_freq(clk);
	else
		freq = mt_get_abist_freq(clk);
	/*freq = fmeter_freq(type, k1, clk);*/
	#if 0
	maxfreq = max(freq, last_freq);

	while (maxfreq > 0 && ABS_DIFF(freq, last_freq) * 100 / maxfreq > 10) {
		last_freq = freq;
		freq = fmeter_freq(type, k1, clk);
		maxfreq = max(freq, last_freq);
	}
	#endif

	return freq;
}

static u32 measure_abist_freq(enum ABIST_CLK clk)
{
	return measure_stable_fmeter_freq(ABIST, 0, clk);
}

static u32 measure_ckgen_freq(enum CKGEN_CLK clk)
{
	return measure_stable_fmeter_freq(CKGEN, 0, clk);
}



static enum ABIST_CLK abist_clk[] = {
	AD_MDPLL_FS26M_CK,
	AD_MDPLL_FS208M_CK,
	AD_OSC_CK,
	AD_CSI0A_CDPHY_DELAYCAL_CK,
	AD_CSI0B_CDPHY_DELAYCAL_CK,
	AD_CSI1A_CDPHY_DELAYCAL_CK,
	AD_CSI1B_CDPHY_DELAYCAL_CK,
	AD_CSI2A_CDPHY_DELAYCAL_CK,
	AD_CSI2B_CDPHY_DELAYCAL_CK,
	AD_DSI0_CKG_DSICLK,
	AD_DSI0_TEST_CK,
	AD_DSI1_CKG_DSICLK,
	AD_DSI1_TEST_CK,
	AD_CCIPLL_CK_VCORE,
	AD_MAINPLL_CK,
	AD_UNIVPLL_CK,
	AD_MSDCPLL_CK,
	AD_EMIPLL_CK,
	AD_GPUPLL_CK,
	AD_TVDPLL_CK,
	AD_MMPLL_CK,
	AD_VCODECPLL_CK,
	AD_APLL1_CK,
	AD_APLL2_CK,
	AD_MDMCUPLL_CK,
	AD_MDINFRAPLL_CK,
	AD_BRPPLL_CK,
	AD_EQPLL_CK,
	AD_IMCPLL_CK,
	AD_ICCPLL_CK,
	AD_MPCPLL_CK,
	AD_DFEPLL_CK,
	AD_MD2GPLL_CK,
	AD_INTFPLL_CK,
	AD_C2KCPPLL_CK,
	AD_FSMIPLL_CK,
	AD_LTXBPLL_CK,
	AD_USB20_192M_CK,
	AD_MPLL_CK,
	AD_ARMPLL2_CK,
	AD_ARMPLL3_CK,
	AD_CCIPLL_CK,
	AD_RAKEPLL_CK,
	AD_CSPLL_CK,
	AD_TVDPLL_DIV4_CK,
	AD_PLLGP_TSTDIV2_CK,
	AD_MP_PLL_CK_ABIST_OUT,
	AD_MP_RX0_TSTCK_DIV2,
	mp_tx_mon_div2_ck,
	mp_rx20_mon_div2_ck,
	AD_ARMPLL_L_CK_VCORE,
	AD_ARMPLL_M_CK_VCORE,
	AD_ARMPLL_B_CK_VCORE,
	AD_ARMPLL_ML_CK_VCORE,
	AD_OSC_SYNC_CK,
	AD_OSC_SYNC_CK_2,
	msdc_01_in_ck,
	msdc_02_in_ck,
	msdc_11_in_ck,
	msdc_12_in_ck,
	msdc_31_in_ck,
	msdc_31_in_ck2,
	AD_OSC_CK_2,
};

static enum CKGEN_CLK ckgen_clk[] = {
	hf_faxi_ck,
	hf_fmem_ck,
	hf_fddrphycfg_ck,
	hf_fmm_ck,
	hf_fsflash_ck,
	f_fpwm_ck,
	f_fdisppwm_ck,
	hf_fvdec_ck,
	hf_fvenc_ck,
	hf_fmfg_ck,
	f_fcamtg_ck,
	f_fi2c_ck,
	f_fuart_ck,
	hf_fspi_ck,
	hf_faxi_peri_ck,
	f_fusb20_ck,
	f_fusb30_p0_ck,
	hf_fmsdc50_0_hclk_ck,
	hf_fmsdc50_0_ck,
	hf_fmsdc30_1_ck,
	f_fi3c_ck,
	hf_fmsdc30_3_ck,
	hf_fmsdc50_3_hclk_ck,
	hf_fsmi0_2x_ck,
	hf_faudio_ck,
	hf_faud_intbus_ck,
	hf_fpmicspi_ck,
	hf_fscp_ck,
	hf_fatb_ck,
	hf_fmjc_ck,
	hf_fdpi0_ck,
	hf_fdsp_ck,
	hf_faud_1_ck,
	hf_faud_2_ck,
	hf_faud_engen1_ck,
	hf_faud_engen2_ck,
	hf_fdfp_mfg_ck,
	hf_fcam_ck,
	hf_fipu_if_ck,
	hf_fsmi1_2x_ck,
	hf_faxi_mfg_in_as_ck,
	hf_fimg_ck,
	hf_fufo_enc_ck,
	hf_fufo_dec_ck,
	hf_fpcie_mac_ck,
	hf_femi_ck,
	hf_faes_ufsfde_ck,
	hf_faes_fde_ck,
	hf_faudio_h_ck,
	hf_fpwrmcu_ck,
	hf_fancmd32_ck,
	hf_fslow_mfg_ck,
	hf_fufs_card_ck,
	hf_fbsi_spi_ck,
	hf_fdxcc_ck,
	f_fseninf_ck,
	hf_fdfp_ck,
	hf_fsmi1_2x_ck2,
	hf_faes_fde_ck2,
	hf_faudio_h_ck2,
	hf_fpwrmcu_ck2,
	f_frtc_ck,
	f_f26m_ck,
};
#if CLKDBG_8173
static u32 measure_armpll_freq(u32 jit_ctrl)
{
#if CLKDBG_8173
	u32 freq;
	u32 mcu26c;
	u32 armpll_jit_ctrl;
	u32 top_dcmctl;

	if (!is_valid_reg(mcucfg_base) || !is_valid_reg(infrasys_base))
		return 0;

	mcu26c = clk_readl(MCU_26C);
	armpll_jit_ctrl = clk_readl(ARMPLL_JIT_CTRL);
	top_dcmctl = clk_readl(INFRA_TOPCKGEN_DCMCTL);

	clk_setl(MCU_26C, 0x8);
	clk_setl(ARMPLL_JIT_CTRL, jit_ctrl);
	clk_clrl(INFRA_TOPCKGEN_DCMCTL, 0x700);

	freq = measure_stable_fmeter_freq(ABIST, 1, ARMPLL_OCC_MON);

	clk_writel(INFRA_TOPCKGEN_DCMCTL, top_dcmctl);
	clk_writel(ARMPLL_JIT_CTRL, armpll_jit_ctrl);
	clk_writel(MCU_26C, mcu26c);

	return freq;
#else
	return 0;
#endif
}

static u32 measure_ca53_freq(void)
{
	return measure_armpll_freq(0x01);
}

static u32 measure_ca57_freq(void)
{
	return measure_armpll_freq(0x11);
}

#endif /* CLKDBG_8173 */

#if DUMP_INIT_STATE

static void print_abist_clock(enum ABIST_CLK clk)
{
	u32 freq = measure_abist_freq(clk);

	clk_info("%2d: %-29s: %u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void print_ckgen_clock(enum CKGEN_CLK clk)
{
	u32 freq = measure_ckgen_freq(clk);

	clk_info("%2d: %-29s: %u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

static void print_fmeter_all(void)
{
	size_t i;
	u32 freq;

	if (!is_valid_reg(apmixedsys_base))
		return;

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		print_abist_clock(abist_clk[i]);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		print_ckgen_clock(ckgen_clk[i]);
}

#endif /* DUMP_INIT_STATE */

static void seq_print_abist_clock(enum ABIST_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_abist_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, ABIST_CLK_NAME[clk], freq);
}

static void seq_print_ckgen_clock(enum CKGEN_CLK clk,
		struct seq_file *s, void *v)
{
	u32 freq = measure_ckgen_freq(clk);

	seq_printf(s, "%2d: %-29s: %u\n", clk, CKGEN_CLK_NAME[clk], freq);
}

static int seq_print_fmeter_all(struct seq_file *s, void *v)
{
	size_t i;

	if (!is_valid_reg(apmixedsys_base))
		return 0;

	for (i = 0; i < ARRAY_SIZE(abist_clk); i++)
		seq_print_abist_clock(abist_clk[i], s, v);

	for (i = 0; i < ARRAY_SIZE(ckgen_clk); i++)
		seq_print_ckgen_clock(ckgen_clk[i], s, v);

	return 0;
}

struct regname {
	void __iomem *reg;
	const char *name;
};

static size_t get_regnames(struct regname *regnames, size_t size)
{
	struct regname rn[] = {
#if CLKDBG_6799
#if 1
		{SPM_MFG0_PWR_CON, "SPM_MFG0_PWR_CON"},
		{SPM_MFG1_PWR_CON, "SPM_MFG1_PWR_CON"},
		{SPM_MFG2_PWR_CON, "SPM_MFG2_PWR_CON"},
		/*{SPM_MFG3_PWR_CON, "SPM_MFG3_PWR_CON"},*/
		{SPM_C2K_PWR_CON, "SPM_C2K_PWR_CON"},
		{SPM_MD1_PWR_CON, "SPM_MD1_PWR_CON"},
		#if 0
		{SPM_DPY_CH0_PWR_CON, "SPM_DPY_CH0_PWR_CON"},
		{SPM_DPY_CH1_PWR_CON, "SPM_DPY_CH1_PWR_CON"},
		{SPM_DPY_CH2_PWR_CON, "SPM_DPY_CH2_PWR_CON"},
		{SPM_DPY_CH3_PWR_CON, "SPM_DPY_CH3_PWR_CON"},
		{SPM_INFRA_PWR_CON, "SPM_INFRA_PWR_CON"},
		#endif
		{SPM_AUD_PWR_CON, "SPM_AUD_PWR_CON"},
		{SPM_MJC_PWR_CON, "SPM_MJC_PWR_CON"},
		{SPM_MM0_PWR_CON, "SPM_MM0_PWR_CON"},
		{SPM_CAM_PWR_CON, "SPM_CAM_PWR_CON"},
		{SPM_IPU_PWR_CON, "SPM_IPU_PWR_CON"},
		{SPM_ISP_PWR_CON, "SPM_ISP_PWR_CON"},
		{SPM_VEN_PWR_CON, "SPM_VEN_PWR_CON"},
		{SPM_VDE_PWR_CON, "SPM_VDE_PWR_CON"},
#endif
		{SPM_PWR_STATUS, "SPM_PWR_STATUS"},
		{SPM_PWR_STATUS_2ND, "SPM_PWR_STATUS_2ND"},
#if 1
		{CLK_CFG_0, "CLK_CFG_0"},
		{CLK_CFG_1, "CLK_CFG_1"},
		{CLK_CFG_2, "CLK_CFG_2"},
		{CLK_CFG_3, "CLK_CFG_3"},
		{CLK_CFG_4, "CLK_CFG_4"},
		{CLK_CFG_5, "CLK_CFG_5"},
		{CLK_CFG_6, "CLK_CFG_6"},
		{CLK_CFG_7, "CLK_CFG_7"},
		{CLK_CFG_8, "CLK_CFG_8"},
		{CLK_CFG_9, "CLK_CFG_9"},
		{CLK_CFG_10, "CLK_CFG_10"},
		{CLK_CFG_11, "CLK_CFG_11"},
		{CLK_CFG_12, "CLK_CFG_12"},
		{CLK_CFG_13, "CLK_CFG_13"},
		{CLK_CFG_14, "CLK_CFG_14"},
#endif
#if 0
		{CLK_MISC_CFG_1, "CLK_MISC_CFG_1"},
		{CLK26CALI_0, "CLK26CALI_0"},
		{CLK26CALI_1, "CLK26CALI_1"},
		{CLK26CALI_2, "CLK26CALI_2"},
#endif
#if 1
		{ARMPLL1_CON0, "ARMPLL1_CON0"},
		#if 0
		{ARMPLL1_CON1, "ARMPLL1_CON1"},
		{ARMPLL1_PWR_CON0, "ARMPLL1_PWR_CON0"},
		#endif
		{ARMPLL2_CON0, "ARMPLL2_CON0"},
		#if 0
		{ARMPLL2_CON1, "ARMPLL2_CON1"},
		{ARMPLL2_PWR_CON0, "ARMPLL2_PWR_CON0"},
		#endif
		{ARMPLL3_CON0, "ARMPLL3_CON0"},
		#if 0
		{ARMPLL3_CON1, "ARMPLL3_CON1"},
		{ARMPLL3_PWR_CON0, "ARMPLL3_PWR_CON0"},
		#endif
		{CCIPLL_CON0, "CCIPLL_CON0"},
		#if 0
		{CCIPLL_CON1, "CCIPLL_CON1"},
		{CCIPLL_PWR_CON0, "CCIPLL_PWR_CON0"},
		#endif
		{MAINPLL_CON0, "MAINPLL_CON0"},
		#if 0
		{MAINPLL_CON1, "MAINPLL_CON1"},
		{MAINPLL_PWR_CON0, "MAINPLL_PWR_CON0"},
		#endif
		{FSMIPLL_CON0, "FSMIPLL_CON0"},
		#if 0
		{FSMIPLL_CON1, "FSMIPLL_CON1"},
		{FSMIPLL_PWR_CON0, "FSMIPLL_PWR_CON0"},
		#endif
#endif
		{GPUPLL_CON0, "GPUPLL_CON0"},
		#if 0
		{GPUPLL_CON1, "GPUPLL_CON1"},
		{GPUPLL_PWR_CON0, "GPUPLL_PWR_CON0"},
		#endif
		{UNIVPLL_CON0, "UNIVPLL_CON0"},
		#if 0
		{UNIVPLL_CON1, "UNIVPLL_CON1"},
		{UNIVPLL_PWR_CON0, "UNIVPLL_PWR_CON0"},
		#endif
		{MMPLL_CON0, "MMPLL_CON0"},
		#if 0
		{MMPLL_CON1, "MMPLL_CON1"},
		{MMPLL_PWR_CON0, "MMPLL_PWR_CON0"},
		#endif
		{MSDCPLL_CON0, "MSDCPLL_CON0"},
		#if 0
		{MSDCPLL_CON1, "MSDCPLL_CON1"},
		{MSDCPLL_PWR_CON0, "MSDCPLL_PWR_CON0"},
		#endif
#if 1
		{TVDPLL_CON0, "TVDPLL_CON0"},
		#if 0
		{TVDPLL_CON1, "TVDPLL_CON1"},
		{TVDPLL_PWR_CON0, "TVDPLL_PWR_CON0"},
		#endif
		{VCODECPLL_CON0, "VCODECPLL_CON0"},
		#if 0
		{VCODECPLL_CON1, "VCODECPLL_CON1"},
		{VCODECPLL_PWR_CON0, "VCODECPLL_PWR_CON0"},
		#endif
		{APLL1_CON0, "APLL1_CON0"},
		#if 0
		{APLL1_CON1, "APLL1_CON1"},
		{APLL1_CON2, "APLL1_CON2"},
		{APLL1_PWR_CON0, "APLL1_PWR_CON0"},
		#endif
		{APLL2_CON0, "APLL2_CON0"},
		#if 0
		{APLL2_CON1, "APLL2_CON1"},
		{APLL2_CON2, "APLL2_CON2"},
		{APLL2_PWR_CON0, "APLL2_PWR_CON0"},
		#endif
#endif
#if 1
		{INFRA_PDN0_STA, "INFRA_PDN0_STA"},
		{INFRA_PDN1_STA, "INFRA_PDN1_STA"},
		{INFRA_PDN2_STA, "INFRA_PDN2_STA"},
		{INFRA_PDN3_STA, "INFRA_PDN3_STA"},
		{INFRA_PDN4_STA, "INFRA_PDN4_STA"},
		{PERI_PDN0_STA, "PERI_PDN0_STA"},
		{PERI_PDN1_STA, "PERI_PDN1_STA"},
		{PERI_PDN2_STA, "PERI_PDN2_STA"},
		{PERI_PDN3_STA, "PERI_PDN3_STA"},
		{PERI_PDN4_STA, "PERI_PDN4_STA"},
		{PERI_PDN5_STA, "PERI_PDN5_STA"},
		{DISP_CG_CON0, "DISP_CG_CON0"},
		{DISP_CG_CON1, "DISP_CG_CON1"},
		{DISP_CG_CON2, "DISP_CG_CON2"},
		{IMG_CG_CON, "IMG_CG_CON"},
		{CAM_CG_CON, "CAM_CG_CON"},
		{IPU_CG_CON, "IPU_CG_CON"},
		{VDEC_CKEN_SET, "VDEC_CKEN_SET"},
		{LARB_CKEN_SET, "LARB_CKEN_SET"},
		{VENC_CG_CON, "VENC_CG_CON"},

		{AUDIO_TOP_CON0, "AUDIO_TOP_CON0"},
		{AUDIO_TOP_CON1, "AUDIO_TOP_CON1"},
		{MFG_CG_CON, "MFG_CG_CON"},
		{MJC_CG_CON, "MJC_CG_CON"},
#endif
#endif /* CLKDBG_SOC */
	};

	size_t n = min(ARRAY_SIZE(rn), size);

	memcpy(regnames, rn, sizeof(rn[0]) * n);
	return n;
}

#if 1

void print_reg(void __iomem *reg, const char *name)
{
	if (!is_valid_reg(reg))
		return;

	clk_info("%-21s: 0x%08x\n", name, clk_readl(reg));
}

void print_registers(void)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		print_reg(rn[i].reg, rn[i].name);
}

#endif /* DUMP_INIT_STATE */

static void seq_print_reg(void __iomem *reg, const char *name,
		struct seq_file *s, void *v)
{
	u32 val;

	if (!is_valid_reg(reg))
		return;

	val = clk_readl(reg);

	clk_info("%-21s: [0x%p] = 0x%08x\n", name, reg, val);
	seq_printf(s, "%-21s: [0x%p] = 0x%08x\n", name, reg, val);
}

static int seq_print_regs(struct seq_file *s, void *v)
{
	static struct regname rn[128];
	int i, n;

	n = get_regnames(rn, ARRAY_SIZE(rn));

	for (i = 0; i < n; i++)
		seq_print_reg(rn[i].reg, rn[i].name, s, v);

	return 0;
}

#if CLKDBG_6799
/* ROOT */
#define clk_null	"clk_null"
#define clk26m	"clk26m"
#define clk32k	"clk32k"

/* PLL */
#define armpll1	"armpll1"
#define armpll2	"armpll2"
#define armpll3	"armpll3"
#define ccipll	"ccipll"
#define mainpll	"mainpll"
#define msdcpll	"msdcpll"
#define univpll	"univpll"
#define gpupll	"gpupll"
#define fsmipll	"fsmipll"
#define mmpll	"mmpll"
#define vcodecpll	"vcodecpll"
#define tvdpll	"tvdpll"
#define apll1	"apll1"
#define apll2	"apll2"
#define ulposc	"ulposc"

/* DIV */
#define clk13m	"clk13m"
#define dmpll_ck	"dmpll_ck"
#define dmpll_d16	"dmpll_d16"
#define dmpllch2_ck	"dmpllch2_ck"
#define dmpllch0_ck	"dmpllch0_ck"
#define syspll_ck	"syspll_ck"
#define syspll_d2	"syspll_d2"
#define syspll1_d2	"syspll1_d2"
#define syspll1_d4	"syspll1_d4"
#define syspll1_d8	"syspll1_d8"
#define syspll1_d16	"syspll1_d16"
#define syspll_d3	"syspll_d3"
#define syspll2_d2	"syspll2_d2"
#define syspll2_d4	"syspll2_d4"
#define syspll2_d8	"syspll2_d8"
#define syspll2_d3	"syspll2_d3"
#define syspll_d5	"syspll_d5"
#define syspll3_d2	"syspll3_d2"
#define syspll3_d4	"syspll3_d4"
#define syspll_d7	"syspll_d7"
#define syspll4_d2	"syspll4_d2"
#define syspll4_d4	"syspll4_d4"
#define univpll_ck	"univpll_ck"
#define univpll_d7	"univpll_d7"
#define univpll_d26	"univpll_d26"
#define univpll_d2	"univpll_d2"
#define univpll1_d2	"univpll1_d2"
#define univpll1_d4	"univpll1_d4"
#define univpll1_d8	"univpll1_d8"
#define univpll_d3	"univpll_d3"
#define univpll2_d2	"univpll2_d2"
#define univpll2_d4	"univpll2_d4"
#define univpll2_d8	"univpll2_d8"
#define univpll_d5	"univpll_d5"
#define univpll3_d2	"univpll3_d2"
#define univpll3_d4	"univpll3_d4"
#define apll1_ck	"apll1_ck"
#define apll1_d2	"apll1_d2"
#define apll1_d4	"apll1_d4"
#define apll1_d8	"apll1_d8"
#define apll2_ck	"apll2_ck"
#define apll2_d2	"apll2_d2"
#define apll2_d4	"apll2_d4"
#define apll2_d8	"apll2_d8"
#define gpupll_ck	"gpupll_ck"
#define mmpll_ck	"mmpll_ck"
#define mmpll_d3	"mmpll_d3"
#define mmpll_d4	"mmpll_d4"
#define mmpll_d5	"mmpll_d5"
#define mmpll_d6	"mmpll_d6"
#define mmpll_d8	"mmpll_d8"
#define vcodecpll_ck	"vcodecpll_ck"
#define vcodecpll_d4	"vcodecpll_d4"
#define vcodecpll_d6	"vcodecpll_d6"
#define vcodecpll_d7	"vcodecpll_d7"
#define fsmipll_ck	"fsmipll_ck"
#define fsmipll_d3	"fsmipll_d3"
#define fsmipll_d4	"fsmipll_d4"
#define fsmipll_d5	"fsmipll_d5"
#define vcodecplld6_ck	"vcodecplld6_ck"
#define vcodecplld6_d2	"vcodecplld6_d2"
#define tvdpll_ck	"tvdpll_ck"
#define tvdpll_d2	"tvdpll_d2"
#define tvdpll_d4	"tvdpll_d4"
#define tvdpll_d8	"tvdpll_d8"
#define tvdpll_d16	"tvdpll_d16"
#define emipll_ck	"emipll_ck"
#define emipll_d2	"emipll_d2"
#define emipll_d4	"emipll_d4"
#define msdcpll_ck	"msdcpll_ck"
#define msdcpll_d2	"msdcpll_d2"
#define msdcpll_d4	"msdcpll_d4"
#define ulposcpll_ck	"ulposcpll_ck"
#define ulposcpll_d2	"ulposcpll_d2"
#define ulposcpll_d8	"ulposcpll_d8"
#define f_frtc_ck	"f_frtc_ck"
#define axi_ck	"axi_ck"
#define mem_ck	"mem_ck"
#define mm_ck	"mm_ck"
#define sflash_ck	"sflash_ck"
#define f_fpwm_ck	"f_fpwm_ck"
#define f_fdisppwm_ck	"f_fdisppwm_ck"
#define vdec_ck	"vdec_ck"
#define venc_ck	"venc_ck"
#define mfg_ck	"mfg_ck"
#define f_fcamtg_ck	"f_fcamtg_ck"
#define f_fi2c_ck	"f_fi2c_ck"
#define f_fuart_ck	"f_fuart_ck"
#define spi_ck	"spi_ck"
#define axi_peri_ck	"axi_peri_ck"
#define smi0_2x_ck	"smi0_2x_ck"
#define audio_ck	"audio_ck"
#define dpi0_ck	"dpi0_ck"
#define dsp_ck	"dsp_ck"
#define aud_1_ck	"aud_1_ck"
#define aud_2_ck	"aud_2_ck"
#define aud_engen1_ck	"aud_engen1_ck"
#define aud_engen2_ck	"aud_engen2_ck"
#define cam_ck	"cam_ck"
#define ipu_if_ck	"ipu_if_ck"
#define axi_mfg_in_ck	"axi_mfg_in_ck"
#define img_ck	"img_ck"
#define ufo_enc_ck	"ufo_enc_ck"
#define aes_ufsfde_ck	"aes_ufsfde_ck"
#define aes_fde_ck	"aes_fde_ck"
#define audio_h_ck	"audio_h_ck"
#define ancmd32_ck	"ancmd32_ck"
#define dxcc_ck	"dxcc_ck"
#define dfp_ck	"dfp_ck"

/* TOP */
#define axi_sel	"axi_sel"
#define mem_sel	"mem_sel"
#define ddrphycfg_sel	"ddrphycfg_sel"
#define mm_sel	"mm_sel"
#define sflash_sel	"sflash_sel"
#define pwm_sel	"pwm_sel"
#define disppwm_sel	"disppwm_sel"
#define vdec_sel	"vdec_sel"
#define venc_sel	"venc_sel"
#define mfg_sel	"mfg_sel"
#define camtg_sel	"camtg_sel"
#define i2c_sel	"i2c_sel"
#define uart_sel	"uart_sel"
#define spi_sel	"spi_sel"
#define axi_peri_sel	"axi_peri_sel"
#define usb20_sel	"usb20_sel"
#define usb30_p0_sel	"usb30_p0_sel"
#define msdc50_0_hclk_sel	"msdc50_0_hclk_sel"
#define msdc50_0_sel	"msdc50_0_sel"
#define msdc30_1_sel	"msdc30_1_sel"
#define i3c_sel	"i3c_sel"
#define msdc30_3_sel	"msdc30_3_sel"
#define msdc50_3_hclk_sel	"msdc50_3_hclk_sel"
#define smi0_2x_sel	"smi0_2x_sel"
#define audio_sel	"audio_sel"
#define aud_intbus_sel	"aud_intbus_sel"
#define pmicspi_sel	"pmicspi_sel"
#define scp_sel	"pmicspi_sel"
#define atb_sel	"atb_sel"
#define mjc_sel	"mjc_sel"
#define dpi0_sel	"dpi0_sel"
#define dsp_sel	"dsp_sel"
#define aud_1_sel	"aud_1_sel"
#define aud_2_sel	"aud_2_sel"
#define aud_engen1_sel	"aud_engen1_sel"
#define aud_engen2_sel	"aud_engen2_sel"
#define dfp_mfg_sel	"dfp_mfg_sel"
#define cam_sel	"cam_sel"
#define ipu_if_sel	"ipu_if_sel"
#define seninf_sel	"seninf_sel"
#define axi_mfg_in_as_sel	"axi_mfg_in_as_sel"
#define img_sel	"img_sel"
#define ufo_enc_sel	"ufo_enc_sel"
#define ufo_dec_sel	"ufo_dec_sel"
#define pcie_mac_sel	"pcie_mac_sel"
#define emi_sel	"emi_sel"
#define aes_ufsfde_sel	"aes_ufsfde_sel"
#define aes_fde_sel	"aes_fde_sel"
#define audio_h_sel	"audio_h_sel"
#define pwrmcu_sel	"pwrmcu_sel"
#define ancmd32_sel	"ancmd32_sel"
#define slow_mfg_sel	"slow_mfg_sel"
#define ufs_card_sel	"ufs_card_sel"
#define bsi_spi_sel	"bsi_spi_sel"
#define dxcc_sel	"dxcc_sel"
#define smi1_2x_sel	"smi1_2x_sel"
#define dfp_sel	"dfp_sel"

/* INFRA */
#define infra_pmic_tmr	"infra_pmic_tmr"
#define infra_pmic_ap	"infra_pmic_ap"
#define infra_pmic_md	"infra_pmic_md"
#define infra_pmic_conn	"infra_pmic_conn"
#define infra_scp	"infra_scp"
#define infra_sej	"infra_sej"
#define infra_apxgpt	"infra_apxgpt"
#define infra_dvfsrc	"infra_dvfsrc"
#define infra_gce	"infra_gce"
#define infra_dbg_bclk	"infra_dbg_bclk"
#define infra_sshub_asy	"infra_sshub_asy"
#define infra_spm_asy	"infra_spm_asy"
#define infra_cldma_ap	"infra_cldma_ap"
#define infra_ccif3_ap	"infra_ccif3_ap"
#define infra_aes_top0	"infra_aes_top0"
#define infra_aes_top1	"infra_aes_top1"
#define infra_devapcmpu	"infra_devapcmpu"
#define infra_ccif4_ap	"infra_ccif4_ap"
#define infra_ccif4_md	"infra_ccif4_md"
#define infra_ccif3_md	"infra_ccif3_md"
#define infra_ccif5_ap	"infra_ccif5_ap"
#define infra_ccif5_md	"infra_ccif5_md"
#define infra_md2md_0	"infra_md2md_0"
#define infra_md2md_1	"infra_md2md_1"
#define infra_md2md_2	"infra_md2md_2"
#define infra_fhctl	"infra_fhctl"
#define infra_md_tmp_sr	"infra_md_tmp_sr"
/* INFRA1 */
#define infra_md2md_3	"infra_md2md_3"
#define infra_md2md_4	"infra_md2md_4"
#define infra_aud_dcm	"infra_aud_dcm"
#define infra_cldma_hck	"infra_cldma_hck"
#define infra_md2md_5	"infra_md2md_5"
#define infra_trng	"infra_trng"
#define infra_auxadc	"infra_auxadc"
#define infra_cpum	"infra_cpum"
#define infra_ccif1_ap	"infra_ccif1_ap"
#define infra_ccif1_md	"infra_ccif1_md"
#define infra_ccif2_ap	"infra_ccif2_ap"
#define infra_ccif2_md	"infra_ccif2_md"
#define infra_xiu_ckcg	"infra_xiu_ckcg"
#define infra_devapc	"infra_devapc"
#define infra_smi_l2c	"infra_smi_l2c"
#define infra_ccif0_ap	"infra_ccif0_ap"
#define infra_audio	"infra_audio"
#define infra_ccif0_md	"infra_ccif0_md"
#define infra_emil2cft	"infra_emil2cft"
#define infra_spm_pcm	"infra_spm_pcm"
#define infra_emi_md32	"infra_emi_md32"
#define infra_anc_md32	"infra_anc_md32"
#define infra_dramc_26m	"infra_dramc_26m"
/* INFRA2 */
#define infra_therm_bck	"infra_therm_bck"
#define infra_ptp_bclk	"infra_ptp_bclk"
#define infra_auxadc_md	"infra_auxadc_md"
#define infra_dvfsctlrx	"infra_dvfsctlrx"

/* PERI */
/* PERICFG0 */
#define peri_pwm_bclk	"peri_pwm_bclk"
#define peri_pwm_fbclk1	"peri_pwm_fbclk1"
#define peri_pwm_fbclk2	"peri_pwm_fbclk2"
#define peri_pwm_fbclk3	"peri_pwm_fbclk3"
#define peri_pwm_fbclk4	"peri_pwm_fbclk4"
#define peri_pwm_fbclk5	"peri_pwm_fbclk5"
#define peri_pwm_fbclk6	"peri_pwm_fbclk6"
#define peri_pwm_fbclk7	"peri_pwm_fbclk7"
#define peri_i2c0_bclk	"peri_i2c0_bclk"
#define peri_i2c1_bclk	"peri_i2c1_bclk"
#define peri_i2c2_bclk	"peri_i2c2_bclk"
#define peri_i2c3_bclk	"peri_i2c3_bclk"
#define peri_i2c4_bclk	"peri_i2c4_bclk"
#define peri_i2c5_bclk	"peri_i2c5_bclk"
#define peri_i2c6_bclk	"peri_i2c6_bclk"
#define peri_i2c7_bclk	"peri_i2c7_bclk"
#define peri_i2c8_bclk	"peri_i2c8_bclk"
#define peri_i2c9_bclk	"peri_i2c9_bclk"
#define peri_idvfs	"peri_idvfs"
/* PERICFG1 */
#define peri_uart0	"peri_uart0"
#define peri_uart1	"peri_uart1"
#define peri_uart2	"peri_uart2"
#define peri_uart3	"peri_uart3"
#define peri_uart4	"peri_uart4"
#define peri_uart5	"peri_uart5"
#define peri_uart6	"peri_uart6"
#define peri_uart7	"peri_uart7"
#define pericfg_rg_spi0	"pericfg_rg_spi0"
#define pericfg_rg_spi1	"pericfg_rg_spi1"
#define pericfg_rg_spi2	"pericfg_rg_spi2"
#define pericfg_rg_spi3	"pericfg_rg_spi3"
#define pericfg_rg_spi4	"pericfg_rg_spi4"
#define pericfg_rg_spi5	"pericfg_rg_spi5"
#define pericfg_rg_spi6	"pericfg_rg_spi6"
#define pericfg_rg_spi7	"pericfg_rg_spi7"
#define pericfg_rg_spi8	"pericfg_rg_spi8"
#define pericfg_rg_spi9	"pericfg_rg_spi9"
#define peri_spi10	"pericfg_spi10"
/* PERICFG2 */
#define peri_msdc0	"peri_msdc0"
#define peri_msdc1	"peri_msdc1"
#define peri_msdc3	"peri_msdc3"
#define peri_ufscard	"peri_ufscard"
#define peri_ufscard_mp	"peri_ufscard_mp"
#define peri_ufs_aes	"peri_ufs_aes"
#define peri_flashif	"peri_flashif"
#define peri_msdc0_ap	"peri_msdc0_ap"
#define peri_msdc0_md	"peri_msdc0_md"
/* PERICFG3 */
#define peri_usb_p0	"peri_usb_p0"
#define peri_usb_p1	"peri_usb_p1"
#define peri_pcie_p0	"peri_pcie_p0"
#define peri_pcie_p0obf	"peri_pcie_p0obf"
#define peri_pcie_p0aux	"peri_pcie_p0aux"
#define peri_pcie_p1	"peri_pcie_p1"
/* PERICFG4 */
#define peri_ap_dm	"peri_ap_dm"
#define pericfg_rg_irtx	"pericfg_rg_irtx"
#define peri_disp_pwm0	"peri_disp_pwm0"
#define peri_disp_pwm1	"peri_disp_pwm1"
#define peri_cq_dma	"peri_cq_dma"
#define peri_mbist_off	"peri_mbist_off"
#define peri_devapc	"peri_devapc"
#define peri_gcpu_biu	"peri_gcpu_biu"
#define peri_dxcc_ao	"peri_dxcc_ao"
#define peri_dxcc_pub	"peri_dxcc_pub"
#define peri_dxcc_sec	"peri_dxcc_sec"
#define peri_debugtop	"peri_debugtop"
#define peri_dxfde_core	"peri_dxfde_core"
#define peri_ufozip	"peri_ufozip"
/* PERICFG5 */
#define peri_apb_peri	"peri_apb_peri"
#define peri_apb_smi	"peri_apb_smi"
#define peri_apb_pmeter	"peri_apb_pmeter"
#define peri_apb_vad	"peri_apb_vad"

/* MFG */
#define mfg_cfg_baxi	"mfg_cfg_baxi"
#define mfg_cfg_bmem	"mfg_cfg_bmem"
#define mfg_cfg_bg3d	"mfg_cfg_bg3d"
#define mfg_cfg_b26m	"mfg_cfg_b26m"

/* IMG */
#define imgsys_larb5	"imgsys_larb5"
#define imgsys_larb2	"imgsys_larb2"
#define imgsys_dip	"imgsys_dip"
#define imgsys_fdvt	"imgsys_fdvt"
#define imgsys_dpe	"imgsys_dpe"
#define imgsys_rsc	"imgsys_rsc"
#define imgsys_wpe	"imgsys_wpe"
#define imgsys_gepf	"imgsys_gepf"
#define imgsys_eaf	"imgsys_eaf"
#define imgsys_dfp	"imgsys_dfp"
#define imgsys_vad	"imgsys_vad"


/* MM */
#define mm_cg0_b0	"mm_cg0_b0"
#define mm_cg0_b1	"mm_cg0_b1"
#define mm_cg0_b2	"mm_cg0_b2"
#define mm_cg0_b3	"mm_cg0_b3"
#define mm_cg0_b4	"mm_cg0_b4"
#define mm_cg0_b5	"mm_cg0_b5"
#define mm_cg0_b6	"mm_cg0_b6"
#define mm_cg0_b7	"mm_cg0_b7"
#define mm_cg0_b8	"mm_cg0_b8"
#define mm_cg0_b9	"mm_cg0_b9"
#define mm_cg0_b10	"mm_cg0_b10"
#define mm_cg0_b11	"mm_cg0_b11"
#define mm_cg0_b12	"mm_cg0_b12"
#define mm_cg0_b13	"mm_cg0_b13"
#define mm_cg0_b14	"mm_cg0_b14"
#define mm_cg0_b15	"mm_cg0_b15"
#define mm_cg0_b16	"mm_cg0_b16"
#define mm_cg0_b17	"mm_cg0_b17"
#define mm_cg0_b18	"mm_cg0_b18"
#define mm_cg0_b19	"mm_cg0_b19"
#define mm_cg0_b20	"mm_cg0_b20"
#define mm_cg0_b21	"mm_cg0_b21"
#define mm_cg0_b22	"mm_cg0_b22"
#define mm_cg0_b23	"mm_cg0_b23"
#define mm_cg0_b24	"mm_cg0_b24"
#define mm_cg0_b25	"mm_cg0_b25"
#define mm_cg0_b26	"mm_cg0_b26"
#define mm_cg0_b27	"mm_cg0_b27"
#define mm_cg0_b28	"mm_cg0_b28"
#define mm_cg0_b29	"mm_cg0_b29"
#define mm_cg0_b30	"mm_cg0_b30"
#define mm_cg0_b31	"mm_cg0_b31"
/* MM1 */
#define mm_cg1_b0	"mm_cg1_b0"
#define mm_cg1_b1	"mm_cg1_b1"
#define mm_cg1_b2	"mm_cg1_b2"
#define mm_cg1_b3	"mm_cg1_b3"
#define mm_cg1_b4	"mm_cg1_b4"
#define mm_cg1_b5	"mm_cg1_b5"
#define mm_cg1_b6	"mm_cg1_b6"
#define mm_cg1_b7	"mm_cg1_b7"
#define mm_cg1_b8	"mm_cg1_b8"
#define mm_cg1_b9	"mm_cg1_b9"
#define mm_cg1_b10	"mm_cg1_b10"
#define mm_cg1_b11	"mm_cg1_b11"
#define mm_cg1_b12	"mm_cg1_b12"
#define mm_cg1_b13	"mm_cg1_b13"
#define mm_cg1_b14	"mm_cg1_b14"
#define mm_cg1_b15	"mm_cg1_b15"
#define mm_cg1_b16	"mm_cg1_b16"
#define mm_cg1_b17	"mm_cg1_b17"
/* MM2 */
#define mm_cg2_b0	"mm_cg2_b0"
#define mm_cg2_b1	"mm_cg2_b1"
#define mm_cg2_b2	"mm_cg2_b2"
#define mm_cg2_b3	"mm_cg2_b3"
#define mm_cg2_b4	"mm_cg2_b4"
#define mm_cg2_b5	"mm_cg2_b5"
#define mm_cg2_b6	"mm_cg2_b6"
#define mm_cg2_b7	"mm_cg2_b7"
#define mm_cg2_b8	"mm_cg2_b8"
#define mm_cg2_b9	"mm_cg2_b9"
#define mm_cg2_b10	"mm_cg2_b10"
#define mm_cg2_b11	"mm_cg2_b11"
#define mm_cg2_b12	"mm_cg2_b12"
#define mm_cg2_b13	"mm_cg2_b13"
#define mm_cg2_b14	"mm_cg2_b14"

/* VDEC */
#define vdec_cken	"vdec_cken"
/* VDEC1 */
#define vdec_larb1_cken	"vdec_larb1_cken"

/* VENC */
#define venc_cke0	"venc_cke0"
#define venc_cke1	"venc_cke1"
#define venc_cke2	"venc_cke2"
#define venc_cke3	"venc_cke3"
#define venc_cke4	"venc_cke4"
#define venc_cke5	"venc_cke5"

/* AUDIO0 */
#define aud_afe	"aud_afe"
#define aud_22m	"aud_22m"
#define aud_24m	"aud_24m"
#define aud_apll2_tuner	"aud_apll2_tuner"
#define aud_apll_tuner	"aud_apll_tuner"
#define aud_adc	"aud_adc"
#define aud_dac	"aud_dac"
#define aud_dac_predis	"aud_dac_predis"
#define aud_tml	"aud_tml"
/* AUDIO1 */
#define aud_adc_hires	"aud_adc_hires"
#define aud_hires_tml	"aud_hires_tml"
#define aud_adda6_adc	"aud_adda6_adc"
#define aud_adda6_hires	"aud_adda6_hires"
#define aud_adda4_adc	"aud_adda4_adc"
#define aud_adda4_hires	"aud_adda4_hires"
#define aud_adda5_adc	"aud_adda5_adc"
#define aud_adda5_hires	"aud_adda5_hires"

/* CAM */
#define camsys_larb6	"camsys_larb6"
#define camsys_dfp_vad	"camsys_dfp_vad"
#define camsys_larb3	"camsys_larb3"
#define camsys_cam	"camsys_cam"
#define camsys_camtg	"camsys_camtg"
#define camsys_seninf	"camsys_seninf"
#define camsys_camsv0	"camsys_camsv0"
#define camsys_camsv1	"camsys_camsv1"
#define camsys_camsv2	"camsys_camsv2"
#define camsys_ccu	"camsys_ccu"

/* IMG */
#define imgsys_larb5	"imgsys_larb5"
#define imgsys_larb2	"imgsys_larb2"
#define imgsys_dip	"imgsys_dip"
#define imgsys_fdvt	"imgsys_fdvt"
#define imgsys_dpe	"imgsys_dpe"
#define imgsys_rsc	"imgsys_rsc"
#define imgsys_wpe	"imgsys_wpe"
#define imgsys_gepf	"imgsys_gepf"
#define imgsys_eaf	"imgsys_eaf"
#define imgsys_dfp	"imgsys_dfp"
#define imgsys_vad	"imgsys_vad"

/* IPU */
#define ipusys_ipu	"ipusys_ipu"
#define ipusys_isp	"ipusys_isp"
#define ipusys_ipu_dfp	"ipusys_ipu_dfp"
#define ipusys_ipu_vad	"ipusys_ipu_vad"
#define ipusys_jtag	"ipusys_jtag"
#define ipusys_axi	"ipusys_axi"
#define ipusys_ahb	"ipusys_ahb"
#define ipusys_mm_axi	"ipusys_mm_axi"
#define ipusys_cam_axi	"ipusys_cam_axi"
#define ipusys_img_axi	"ipusys_img_axi"

/* MJC */
#define mjc_smi_larb	"mjc_smi_larb"
#define mjc_top0	"mjc_top0"
#define mjc_top1	"mjc_top1"
#define mjc_top2	"mjc_top2"
#define mjc_fake_engine	"mjc_fake_engine"
#define mjc_meter	"mjc_meter"

#define pg_md1	"pg_md1"
#define pg_mm0	"pg_mm0"
#define pg_mfg0	"pg_mfg0"
#define pg_mfg1	"pg_mfg1"
#define pg_mfg2	"pg_mfg2"
#define pg_isp	"pg_isp"
#define pg_vde	"pg_vde"
#define pg_ven	"pg_ven"
#define pg_ipu_shutdown	"pg_ipu_shutdown"
#define pg_ipu_sleep	"pg_ipu_sleep"
#define pg_audio	"pg_audio"
#define pg_cam	"pg_cam"
#define pg_c2k	"pg_c2k"
#define pg_mjc	"pg_mjc"







#endif /* CLKDBG_6755 */

static const char * const *get_all_clk_names(size_t *num)
{
	static const char * const clks[] = {
#if CLKDBG_6799
	/* ROOT */
	clk_null,
	clk26m,
	clk32k,

	/* PLL */
	armpll1,
	armpll2,
	armpll3,
	ccipll,
	mainpll,
	msdcpll,
	univpll,
	gpupll,
	fsmipll,
	mmpll,
	vcodecpll,
	tvdpll,
	apll1,
	apll2,
	ulposc,

	/* DIV */
	clk13m,
	ulposc,
	dmpll_ck,
	dmpll_d16,
	dmpllch2_ck,
	dmpllch0_ck,
	syspll_ck,
	syspll_d2,
	syspll1_d2,
	syspll1_d4,
	syspll1_d8,
	syspll1_d16,
	syspll_d3,
	syspll2_d2,
	syspll2_d4,
	syspll2_d8,
	syspll2_d3,
	syspll_d5,
	syspll3_d2,
	syspll3_d4,
	syspll_d7,
	syspll4_d2,
	syspll4_d4,
	univpll_ck,
	univpll_d7,
	univpll_d26,
	univpll_d2,
	univpll1_d2,
	univpll1_d4,
	univpll1_d8,
	univpll_d3,
	univpll2_d2,
	univpll2_d4,
	univpll2_d8,
	univpll_d5,
	univpll3_d2,
	univpll3_d4,
	apll1_ck,
	apll1_d2,
	apll1_d4,
	apll1_d8,
	apll2_ck,
	apll2_d2,
	apll2_d4,
	apll2_d8,
	gpupll_ck,
	mmpll_ck,
	mmpll_d3,
	mmpll_d4,
	mmpll_d5,
	mmpll_d6,
	mmpll_d8,
	vcodecpll_ck,
	vcodecpll_d4,
	vcodecpll_d6,
	vcodecpll_d7,
	fsmipll_ck,
	fsmipll_d3,
	fsmipll_d4,
	fsmipll_d5,
	vcodecplld6_ck,
	vcodecplld6_d2,
	tvdpll_ck,
	tvdpll_d2,
	tvdpll_d4,
	tvdpll_d8,
	tvdpll_d16,
	emipll_ck,
	emipll_d2,
	emipll_d4,
	msdcpll_ck,
	msdcpll_d2,
	msdcpll_d4,
	ulposcpll_ck,
	ulposcpll_d2,
	ulposcpll_d8,
	f_frtc_ck,
	axi_ck,
	mem_ck,
	mm_ck,
	sflash_ck,
	f_fpwm_ck,
	f_fdisppwm_ck,
	vdec_ck,
	venc_ck,
	mfg_ck,
	f_fcamtg_ck,
	f_fi2c_ck,
	f_fuart_ck,
	spi_ck,
	axi_peri_ck,
	smi0_2x_ck,
	audio_ck,
	dpi0_ck,
	dsp_ck,
	aud_1_ck,
	aud_2_ck,
	aud_engen1_ck,
	aud_engen2_ck,
	cam_ck,
	ipu_if_ck,
	axi_mfg_in_ck,
	img_ck,
	ufo_enc_ck,
	aes_ufsfde_ck,
	aes_fde_ck,
	audio_h_ck,
	ancmd32_ck,
	dxcc_ck,
	dfp_ck,

	/* TOP */
	axi_sel,
	mem_sel,
	ddrphycfg_sel,
	mm_sel,
	sflash_sel,
	pwm_sel,
	disppwm_sel,
	vdec_sel,
	venc_sel,
	mfg_sel,
	camtg_sel,
	i2c_sel,
	uart_sel,
	spi_sel,
	axi_peri_sel,
	usb20_sel,
	usb30_p0_sel,
	msdc50_0_hclk_sel,
	msdc50_0_sel,
	msdc30_1_sel,
	i3c_sel,
	msdc30_3_sel,
	msdc50_3_hclk_sel,
	smi0_2x_sel,
	audio_sel,
	aud_intbus_sel,
	pmicspi_sel,
	scp_sel,
	atb_sel,
	mjc_sel,
	dpi0_sel,
	dsp_sel,
	aud_1_sel,
	aud_2_sel,
	aud_engen1_sel,
	aud_engen2_sel,
	dfp_mfg_sel,
	cam_sel,
	ipu_if_sel,
	seninf_sel,
	axi_mfg_in_as_sel,
	img_sel,
	ufo_enc_sel,
	ufo_dec_sel,
	pcie_mac_sel,
	emi_sel,
	aes_ufsfde_sel,
	aes_fde_sel,
	audio_h_sel,
	pwrmcu_sel,
	ancmd32_sel,
	slow_mfg_sel,
	ufs_card_sel,
	bsi_spi_sel,
	dxcc_sel,
	smi1_2x_sel,
	dfp_sel,

	/* INFRA */
	infra_pmic_tmr,
	infra_pmic_ap,
	infra_pmic_md,
	infra_pmic_conn,
	infra_scp,
	infra_sej,
	infra_apxgpt,
	infra_dvfsrc,
	infra_gce,
	infra_dbg_bclk,
	infra_sshub_asy,
	infra_spm_asy,
	infra_cldma_ap,
	infra_ccif3_ap,
	infra_aes_top0,
	infra_aes_top1,
	infra_devapcmpu,
	infra_ccif4_ap,
	infra_ccif4_md,
	infra_ccif3_md,
	infra_ccif5_ap,
	infra_ccif5_md,
	infra_md2md_0,
	infra_md2md_1,
	infra_md2md_2,
	infra_fhctl,
	infra_md_tmp_sr,
	/* INFRA1 */
	infra_md2md_3,
	infra_md2md_4,
	infra_aud_dcm,
	infra_cldma_hck,
	infra_md2md_5,
	infra_trng,
	infra_auxadc,
	infra_cpum,
	infra_ccif1_ap,
	infra_ccif1_md,
	infra_ccif2_ap,
	infra_ccif2_md,
	infra_xiu_ckcg,
	infra_devapc,
	infra_smi_l2c,
	infra_ccif0_ap,
	infra_audio,
	infra_ccif0_md,
	infra_emil2cft,
	infra_spm_pcm,
	infra_emi_md32,
	infra_anc_md32,
	infra_dramc_26m,
	/* INFRA2 */
	infra_therm_bck,
	infra_ptp_bclk,
	infra_auxadc_md,
	infra_dvfsctlrx,

	/* PERI */
	/* PERICFG0 */
	peri_pwm_bclk,
	peri_pwm_fbclk1,
	peri_pwm_fbclk2,
	peri_pwm_fbclk3,
	peri_pwm_fbclk4,
	peri_pwm_fbclk5,
	peri_pwm_fbclk6,
	peri_pwm_fbclk7,
	peri_i2c0_bclk,
	peri_i2c1_bclk,
	peri_i2c2_bclk,
	peri_i2c3_bclk,
	peri_i2c4_bclk,
	peri_i2c5_bclk,
	peri_i2c6_bclk,
	peri_i2c7_bclk,
	peri_i2c8_bclk,
	peri_i2c9_bclk,
	peri_idvfs,
	/* PERICFG1 */
	peri_uart0,
	peri_uart1,
	peri_uart2,
	peri_uart3,
	peri_uart4,
	peri_uart5,
	peri_uart6,
	peri_uart7,
	pericfg_rg_spi0,
	pericfg_rg_spi1,
	pericfg_rg_spi2,
	pericfg_rg_spi3,
	pericfg_rg_spi4,
	pericfg_rg_spi5,
	pericfg_rg_spi6,
	pericfg_rg_spi7,
	pericfg_rg_spi8,
	pericfg_rg_spi9,
	peri_spi10,
	/* PERICFG2 */
	peri_msdc0,
	peri_msdc1,
	peri_msdc3,
	peri_ufscard,
	peri_ufscard_mp,
	peri_ufs_aes,
	peri_flashif,
	peri_msdc0_ap,
	peri_msdc0_md,
	/* PERICFG3 */
	peri_usb_p0,
	peri_usb_p1,
	peri_pcie_p0,
	peri_pcie_p0obf,
	peri_pcie_p0aux,
	peri_pcie_p1,
	/* PERICFG4 */
	peri_ap_dm,
	pericfg_rg_irtx,
	peri_disp_pwm0,
	peri_disp_pwm1,
	peri_cq_dma,
	peri_mbist_off,
	peri_devapc,
	peri_gcpu_biu,
	peri_dxcc_ao,
	peri_dxcc_pub,
	peri_dxcc_sec,
	peri_debugtop,
	peri_dxfde_core,
	peri_ufozip,
	/* PERICFG5 */
	peri_apb_peri,
	peri_apb_smi,
	peri_apb_pmeter,
	peri_apb_vad,

		/* MFG */
	mfg_cfg_baxi,
	mfg_cfg_bmem,
	mfg_cfg_bg3d,
	mfg_cfg_b26m,

		/* IMG */
	imgsys_larb5,
	imgsys_larb2,
	imgsys_dip,
	imgsys_fdvt,
	imgsys_dpe,
	imgsys_rsc,
	imgsys_wpe,
	imgsys_gepf,
	imgsys_eaf,
	imgsys_dfp,
	imgsys_vad,


		/* MM */
	mm_cg0_b0,
	mm_cg0_b1,
	mm_cg0_b2,
	mm_cg0_b3,
	mm_cg0_b4,
	mm_cg0_b5,
	mm_cg0_b6,
	mm_cg0_b7,
	mm_cg0_b8,
	mm_cg0_b9,
	mm_cg0_b10,
	mm_cg0_b11,
	mm_cg0_b12,
	mm_cg0_b13,
	mm_cg0_b14,
	mm_cg0_b15,
	mm_cg0_b16,
	mm_cg0_b17,
	mm_cg0_b18,
	mm_cg0_b19,
	mm_cg0_b20,
	mm_cg0_b21,
	mm_cg0_b22,
	mm_cg0_b23,
	mm_cg0_b24,
	mm_cg0_b25,
	mm_cg0_b26,
	mm_cg0_b27,
	mm_cg0_b28,
	mm_cg0_b29,
	mm_cg0_b30,
	mm_cg0_b31,
	/* MM1 */
	mm_cg1_b0,
	mm_cg1_b1,
	mm_cg1_b2,
	mm_cg1_b3,
	mm_cg1_b4,
	mm_cg1_b5,
	mm_cg1_b6,
	mm_cg1_b7,
	mm_cg1_b8,
	mm_cg1_b9,
	mm_cg1_b10,
	mm_cg1_b11,
	mm_cg1_b12,
	mm_cg1_b13,
	mm_cg1_b14,
	mm_cg1_b15,
	mm_cg1_b16,
	mm_cg1_b17,
	/* MM2 */
	mm_cg2_b0,
	mm_cg2_b1,
	mm_cg2_b2,
	mm_cg2_b3,
	mm_cg2_b4,
	mm_cg2_b5,
	mm_cg2_b6,
	mm_cg2_b7,
	mm_cg2_b8,
	mm_cg2_b9,
	mm_cg2_b10,
	mm_cg2_b11,
	mm_cg2_b12,
	mm_cg2_b13,
	mm_cg2_b14,

	/* VDEC */
	vdec_cken,
	/* VDEC1 */
	vdec_larb1_cken,

	/* VENC */
	venc_cke0,
	venc_cke1,
	venc_cke2,
	venc_cke3,
	venc_cke4,
	venc_cke5,

	/* AUDIO */
	/* AUDIO0 */
	aud_afe,
	aud_22m,
	aud_24m,
	aud_apll2_tuner,
	aud_apll_tuner,
	aud_adc,
	aud_dac,
	aud_dac_predis,
	aud_tml,
	/* AUDIO1 */
	aud_adc_hires,
	aud_hires_tml,
	aud_adda6_adc,
	aud_adda6_hires,
	aud_adda4_adc,
	aud_adda4_hires,
	aud_adda5_adc,
	aud_adda5_hires,

	/* CAM */
	camsys_larb6,
	camsys_dfp_vad,
	camsys_larb3,
	camsys_cam,
	camsys_camtg,
	camsys_seninf,
	camsys_camsv0,
	camsys_camsv1,
	camsys_camsv2,
	camsys_ccu,

	/* IMG */
	imgsys_larb5,
	imgsys_larb2,
	imgsys_dip,
	imgsys_fdvt,
	imgsys_dpe,
	imgsys_rsc,
	imgsys_wpe,
	imgsys_gepf,
	imgsys_eaf,
	imgsys_dfp,
	imgsys_vad,

	/* IPU */
	ipusys_ipu,
	ipusys_isp,
	ipusys_ipu_dfp,
	ipusys_ipu_vad,
	ipusys_jtag,
	ipusys_axi,
	ipusys_ahb,
	ipusys_mm_axi,
	ipusys_cam_axi,
	ipusys_img_axi,

	/* MJC */
	mjc_smi_larb,
	mjc_top0,
	mjc_top1,
	mjc_top2,
	mjc_fake_engine,
	mjc_meter,

	pg_md1,
	pg_mm0,
	pg_mfg0,
	pg_mfg1,
	pg_mfg2,
	pg_isp,
	pg_vde,
	pg_ven,
	pg_ipu_shutdown,
	pg_ipu_sleep,
	pg_audio,
	pg_cam,
	pg_c2k,
	pg_mjc,
#endif /* CLKDBG_SOC */
	};

	*num = ARRAY_SIZE(clks);

	return clks;
}

static void dump_clk_state(const char *clkname, struct seq_file *s)
{
#if 1
	struct clk *c = __clk_lookup(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : clk_get_parent(c);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		__clk_get_name(c),
		(__clk_is_enabled(c) || __clk_is_prepared(c)) ? "ON" : "off",
		__clk_is_prepared(c),
		__clk_get_enable_count(c),
		clk_get_rate(c),
		p ? __clk_get_name(p) : "");
#endif
}

int univpll_is_used(void)
{
	/*
	* 0: univpll is not used, sspm can disable
	* 1: univpll is used, sspm cannot disable
	*/
	struct clk *c = __clk_lookup("univpll");

	return __clk_get_enable_count(c);
}

int ipu_is_used(void)
{
	struct clk *c = __clk_lookup("pg_ipu_shutdown");

	return __clk_get_enable_count(c);
}

static int clkdbg_dump_state_all(struct seq_file *s, void *v)
{
	int i;
	size_t num;

	const char * const *clks = get_all_clk_names(&num);

	pr_debug("\n");
	for (i = 0; i < num; i++)
		dump_clk_state(clks[i], s);

	return 0;
}

static char last_cmd[128] = "null";

static int clkdbg_prepare_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_prepare_enable(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_prepare_enable(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_disable_unprepare(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_disable_unprepare(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	clk_disable_unprepare(clk);
	seq_puts(s, "0\n");

	return 0;
}

static int clkdbg_set_parent(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *parent_name;
	struct clk *clk;
	struct clk *parent;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	parent_name = strsep(&c, " ");

	seq_printf(s, "clk_set_parent(%s, %s): ", clk_name, parent_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	parent = __clk_lookup(parent_name);
	if (IS_ERR_OR_NULL(parent)) {
		seq_printf(s, "clk_get(): 0x%p\n", parent);
		return PTR_ERR(parent);
	}

	r = clk_prepare_enable(clk);
	if (r) {
		seq_printf(s, "clk_prepare_enable() failed: %d\n", r);
		return r;
	}
	r = clk_set_parent(clk, parent);
	clk_disable_unprepare(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_set_rate(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *rate_str;
	struct clk *clk;
	unsigned long rate;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	rate_str = strsep(&c, " ");
	r = kstrtoul(rate_str, 0, &rate);

	seq_printf(s, "clk_set_rate(%s, %lu): %d: ", clk_name, rate, r);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_set_rate(clk, rate);
	seq_printf(s, "%d\n", r);

	return r;
}

void *register_from_str(const char *str)
{
	if (sizeof(void *) == sizeof(unsigned long)) {
		unsigned long v;

		if (kstrtoul(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else if (sizeof(void *) == sizeof(unsigned long long)) {
		unsigned long long v;

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else {
		unsigned long long v;

		clk_warn("unexpected pointer size: sizeof(void *): %zu\n",
			sizeof(void *));

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	}

	clk_warn("%s(): parsing error: %s\n", __func__, str);

	return NULL;
}

static int parse_reg_val_from_cmd(void __iomem **preg, unsigned long *pval)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *reg_str;
	char *val_str;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	reg_str = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (preg)
		*preg = register_from_str(reg_str);

	if (pval)
		r = kstrtoul(val_str, 0, pval);

	return r;
}

static int clkdbg_reg_read(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, NULL);
	seq_printf(s, "readl(0x%p): ", reg);

	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_write(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_writel(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_set(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_setl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_clr(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_clrl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

#if CLKDBG_PM_DOMAIN

/*
 * pm_domain support
 */

static struct generic_pm_domain **get_all_pm_domain(int *numpd)
{
	static struct generic_pm_domain *pds[10];
	const int maxpd = ARRAY_SIZE(pds);
#if CLKDBG_6799
	const char *cmp = "mediatek,mt6799-scpsys";
#endif

	struct device_node *node;
	int i;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_err("node '%s' not found!\n", cmp);
		return NULL;
	}

	for (i = 0; i < maxpd; i++) {
		struct of_phandle_args pa;

		pa.np = node;
		pa.args[0] = i;
		pa.args_count = 1;
		pds[i] = of_genpd_get_from_provider(&pa);

		if (IS_ERR(pds[i]))
			break;
	}

	if (numpd)
		*numpd = i;

	return pds;
}

static struct generic_pm_domain *genpd_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		if (strcmp(name, pd->name) == 0)
			return pd;
	}

	return NULL;
}

static struct platform_device *pdev_from_name(const char *name)
{
	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd))
			continue;

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			if (strcmp(name, pdev->name) == 0)
				return pdev;
		}
	}

	return NULL;
}

static void seq_print_all_genpd(struct seq_file *s)
{
	static const char * const gpd_status_name[] = {
		"ACTIVE",
		"WAIT_MASTER",
		"BUSY",
		"REPEAT",
		"POWER_OFF",
	};

	static const char * const prm_status_name[] = {
		"active",
		"resuming",
		"suspended",
		"suspending",
	};

	int i;
	int numpd;
	struct generic_pm_domain **pds = get_all_pm_domain(&numpd);

	seq_puts(s, "domain_on [pmd_name  status]\n");
	seq_puts(s, "\tdev_on (dev_name usage_count, disable, status)\n");
	seq_puts(s, "------------------------------------------------------\n");

	for (i = 0; i < numpd; i++) {
		struct pm_domain_data *pdd;
		struct generic_pm_domain *pd = pds[i];

		if (IS_ERR_OR_NULL(pd)) {
			seq_printf(s, "pd[%d]: 0x%p\n", i, pd);
			continue;
		}

		seq_printf(s, "%c [%-9s %11s]\n",
			(pd->status == GPD_STATE_ACTIVE) ? '+' : '-',
			pd->name, gpd_status_name[pd->status]);

		list_for_each_entry(pdd, &pd->dev_list, list_node) {
			struct device *dev = pdd->dev;
			struct platform_device *pdev = to_platform_device(dev);

			seq_printf(s, "\t%c (%-16s %3d, %d, %10s)\n",
				pm_runtime_active(dev) ? '+' : '-',
				pdev->name,
				atomic_read(&dev->power.usage_count),
				dev->power.disable_depth,
				prm_status_name[dev->power.runtime_status]);
		}
	}
}

static int clkdbg_dump_genpd(struct seq_file *s, void *v)
{
	seq_print_all_genpd(s);

	return 0;
}

static int clkdbg_pm_genpd_poweron(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *genpd_name;
	struct generic_pm_domain *pd;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	genpd_name = strsep(&c, " ");

	seq_printf(s, "pm_genpd_poweron(%s): ", genpd_name);

	pd = genpd_from_name(genpd_name);
	if (pd) {
		int r = pm_genpd_poweron(pd);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_genpd_poweroff_unused(struct seq_file *s, void *v)
{
	seq_puts(s, "pm_genpd_poweroff_unused()\n");
	pm_genpd_poweroff_unused();

	return 0;
}

static int clkdbg_pm_runtime_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_enable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_enable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_disable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_disable(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		pm_runtime_disable(&pdev->dev);
		seq_puts(s, "\n");
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_get_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_get_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_get_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

static int clkdbg_pm_runtime_put_sync(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *dev_name;
	struct platform_device *pdev;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	dev_name = strsep(&c, " ");

	seq_printf(s, "pm_runtime_put_sync(%s): ", dev_name);

	pdev = pdev_from_name(dev_name);
	if (pdev) {
		int r = pm_runtime_put_sync(&pdev->dev);

		seq_printf(s, "%d\n", r);
	} else {
		seq_puts(s, "NULL\n");
	}

	return 0;
}

#endif /* CLKDBG_PM_DOMAIN */

#define CMDFN(_cmd, _fn) {	\
	.cmd = _cmd,		\
	.fn = _fn,		\
}

static int clkdbg_show(struct seq_file *s, void *v)
{
	static const struct {
		const char	*cmd;
		int (*fn)(struct seq_file *, void *);
	} cmds[] = {
		CMDFN("dump_regs", seq_print_regs),
		CMDFN("dump_state", clkdbg_dump_state_all),
		CMDFN("fmeter", seq_print_fmeter_all),
		CMDFN("prepare_enable", clkdbg_prepare_enable),
		CMDFN("disable_unprepare", clkdbg_disable_unprepare),
		CMDFN("set_parent", clkdbg_set_parent),
		CMDFN("set_rate", clkdbg_set_rate),
		CMDFN("reg_read", clkdbg_reg_read),
		CMDFN("reg_write", clkdbg_reg_write),
		CMDFN("reg_set", clkdbg_reg_set),
		CMDFN("reg_clr", clkdbg_reg_clr),
#if CLKDBG_PM_DOMAIN
		CMDFN("dump_genpd", clkdbg_dump_genpd),
		CMDFN("pm_genpd_poweron", clkdbg_pm_genpd_poweron),
		CMDFN("pm_genpd_poweroff_unused",
			clkdbg_pm_genpd_poweroff_unused),
		CMDFN("pm_runtime_enable", clkdbg_pm_runtime_enable),
		CMDFN("pm_runtime_disable", clkdbg_pm_runtime_disable),
		CMDFN("pm_runtime_get_sync", clkdbg_pm_runtime_get_sync),
		CMDFN("pm_runtime_put_sync", clkdbg_pm_runtime_put_sync),
#endif /* CLKDBG_PM_DOMAIN */
	};

	int i;
	char cmd[sizeof(last_cmd) + 1];

	pr_debug("last_cmd: %s\n", last_cmd);

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cmds[i].cmd, token) == 0)
			return cmds[i].fn(s, v);
	}

	return 0;
}

static int clkdbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, clkdbg_show, NULL);
}

static ssize_t clkdbg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
{
	char desc[sizeof(last_cmd) - 1];
	int len = 0;

	pr_debug("count: %zu\n", count);
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	strncpy(last_cmd, desc, sizeof(last_cmd));
	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations clkdbg_fops = {
	.owner		= THIS_MODULE,
	.open		= clkdbg_open,
	.read		= seq_read,
	.write		= clkdbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

static int __init get_base_from_node(
			const char *cmp, int idx, void __iomem **pbase)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		clk_warn("node '%s' not found!\n", cmp);
		return -1;
	}

	*pbase = get_reg(node, idx);
	clk_info("%s base: 0x%p\n", cmp, *pbase);

	return 0;
}

static void __init init_iomap(void)
{
#if CLKDBG_6799
	get_base_from_node("mediatek,mt6799-topckgen", 0, &topckgen_base);
	get_base_from_node("mediatek,mt6799-infracfg_ao", 0, &infrasys_base);
	get_base_from_node("mediatek,mt6799-pericfg", 0, &perisys_base);
	get_base_from_node("mediatek,mcucfg", 0, &mcucfg_base);
	get_base_from_node("mediatek,mt6799-apmixedsys", 0, &apmixedsys_base);
	get_base_from_node("mediatek,mt6799-mfg_cfg", 0, &mfgsys_base);
	get_base_from_node("mediatek,mt6799-mmsys_config", 0, &mmsys_base);
	get_base_from_node("mediatek,mt6799-imgsys", 0, &imgsys_base);
	get_base_from_node("mediatek,mt6799-vdec_gcon", 0, &vdecsys_base);
	get_base_from_node("mediatek,mt6799-venc_global_con", 0, &vencsys_base);
	get_base_from_node("mediatek,audio", 0, &audiosys_base);
	get_base_from_node("mediatek,mt6799-scpsys", 1, &scpsys_base);
	get_base_from_node("mediatek,mjc_config", 0, &mjcsys_base);
	get_base_from_node("mediatek,mt6799-camsys", 0, &camsys_base);
	get_base_from_node("mediatek,mt6799-ipusys", 0, &ipusys_base);
#endif /* CLKDBG_SOC */
}

/*
 * clkdbg pm_domain support
 */

static const struct of_device_id clkdbg_id_table[] = {
	{ .compatible = "mediatek,clkdbg-pd",},
	{ },
};
MODULE_DEVICE_TABLE(of, clkdbg_id_table);

static int clkdbg_probe(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_remove(struct platform_device *pdev)
{
	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static int clkdbg_pd_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	clk_info("%s():%d: pdev: %s\n", __func__, __LINE__, pdev->name);

	return 0;
}

static const struct dev_pm_ops clkdbg_pd_pm_ops = {
	.runtime_suspend = clkdbg_pd_runtime_suspend,
	.runtime_resume = clkdbg_pd_runtime_resume,
};

static struct platform_driver clkdbg_driver = {
	.probe		= clkdbg_probe,
	.remove		= clkdbg_remove,
	.driver		= {
		.name	= "clkdbg",
		.owner	= THIS_MODULE,
		.of_match_table = clkdbg_id_table,
		.pm = &clkdbg_pd_pm_ops,
	},
};

module_platform_driver(clkdbg_driver);

/*
 * pm_domain sample code
 */

static struct platform_device *my_pdev;

int power_on_before_work(void)
{
	return pm_runtime_get_sync(&my_pdev->dev);
}

int power_off_after_work(void)
{
	return pm_runtime_put_sync(&my_pdev->dev);
}

static const struct of_device_id my_id_table[] = {
	{ .compatible = "mediatek,my-device",},
	{ },
};
MODULE_DEVICE_TABLE(of, my_id_table);

static int my_probe(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);

	my_pdev = pdev;

	return 0;
}

static int my_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver my_driver = {
	.probe		= my_probe,
	.remove		= my_remove,
	.driver		= {
		.name	= "my_driver",
		.owner	= THIS_MODULE,
		.of_match_table = my_id_table,
	},
};

module_platform_driver(my_driver);

/*
 * init functions
 */

int __init mt_clkdbg_init(void)
{
	init_iomap();

#if DUMP_INIT_STATE
	print_registers();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
arch_initcall(mt_clkdbg_init);

int __init mt_clkdbg_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("clkdbg", 0, 0, &clkdbg_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
module_init(mt_clkdbg_debug_init);

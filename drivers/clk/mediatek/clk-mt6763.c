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

const char *ckgen_array[] = {
"hf_faxi_ck",
"hf_fmem_ck",
"hf_fddrphycfg_ck",
"hf_fmm_ck",
"hf_fsflash_ck",
"f_fpwm_ck",
"f_fdisppwm_ck",
"hf_fvdec_ck",
"hf_fvenc_ck",
"hf_fmfg_ck",
"f_fcamtg_ck",
"f_fi2c_ck",
"f_fuart_ck",
"hf_fspi_ck",
"hf_faxi_peri_ck",
"f_fusb20_ck",
"f_fusb30_p0_ck",
"hf_fmsdc50_0_hclk_ck",
"hf_fmsdc50_0_ck",
"hf_fmsdc30_1_ck",
"f_fi3c_ck",
"hf_fmsdc30_3_ck",
"hf_fmsdc50_3_hclk_ck",
"hf_fsmi0_2x_ck",
"hf_faudio_ck",
"hf_faud_intbus_ck",
"hf_fpmicspi_ck",
"hf_fscp_ck",
"hf_fatb_ck",
"hf_fmjc_ck",
"hf_fdpi0_ck",
"hf_fdsp_ck",
"hf_faud_1_ck",
"hf_faud_2_ck",
"hf_faud_engen1_ck",
"hf_faud_engen2_ck",
"hf_fdfp_mfg_ck",
"hf_fcam_ck",
"hf_fipu_if_ck",
"hf_fsmi1_2x_ck",
"hf_faxi_mfg_in_as_ck",
"hf_fimg_ck",
"hf_fufo_enc_ck",
"hf_fufo_dec_ck",
"hf_fpcie_mac_ck",
"hf_femi_ck",
"hf_faes_ufsfde_ck",
"hf_faes_fde_ck",
"hf_faudio_h_ck",
"hf_fsspm_ck",
"hf_fancmd32_ck",
"hf_fslow_mfg_ck",
"hf_fufs_card_ck",
"hf_fbsi_spi_ck",
"hf_fdxcc_ck",
"f_fseninf_ck",
"hf_fdfp_ck",
"hf_fsmi1_2x_ck",
"hf_faes_fde_ck",
"hf_faudio_h_ck",
"hf_fsspm_ck",
"f_frtc_ck",
"f_f26m_ck"
};

const char *abist_array[] = {
"AD_MDPLL_FS26M_CK",
"AD_MDPLL_FS208M_CK",
"AD_OSC_CK",
"AD_CSI0A_CDPHY_DELAYCAL_CK",
"AD_CSI0B_CDPHY_DELAYCAL_CK",
"AD_CSI1A_CDPHY_DELAYCAL_CK",
"AD_CSI1B_CDPHY_DELAYCAL_CK",
"AD_CSI2A_CDPHY_DELAYCAL_CK",
"AD_CSI2B_CDPHY_DELAYCAL_CK",
"AD_DSI0_CKG_DSICLK",
"AD_DSI0_TEST_CK",
"AD_DSI1_CKG_DSICLK",
"AD_DSI1_TEST_CK",
"AD_CCIPLL_CK_VCORE",
"AD_MAINPLL_CK",
"AD_UNIVPLL_CK",
"AD_MSDCPLL_CK",
"AD_EMIPLL_CK",
"AD_GPUPLL_CK",
"AD_TVDPLL_CK",
"AD_MMPLL_CK",
"AD_VCODECPLL_CK",
"AD_APLL1_CK",
"AD_APLL2_CK",
"AD_MDMCUPLL_CK",
"AD_MDINFRAPLL_CK",
"AD_BRPPLL_CK",
"AD_EQPLL_CK",
"AD_IMCPLL_CK",
"AD_ICCPLL_CK",
"AD_MPCPLL_CK",
"AD_DFEPLL_CK",
"AD_MD2GPLL_CK",
"AD_INTFPLL_CK",
"AD_C2KCPPLL_CK",
"AD_FSMIPLL_CK",
"AD_LTXBPLL_CK",
"AD_USB20_192M_CK",
"AD_MPLL_CK",
"AD_ARMPLL2_CK",
"AD_ARMPLL3_CK",
"AD_CCIPLL_CK",
"AD_RAKEPLL_CK",
"AD_CSPLL_CK",
"AD_TVDPLL_DIV4_CK",
"AD_PLLGP_TSTDIV2_CK",
"AD_MP_PLL_CK_ABIST_OUT",
"AD_MP_RX0_TSTCK_DIV2",
"mp_tx_mon_div2_ck",
"mp_rx20_mon_div2_ck",
"AD_ARMPLL_L_CK_VCORE",
"AD_ARMPLL_M_CK_VCORE",
"AD_ARMPLL_B_CK_VCORE",
"AD_ARMPLL_ML_CK_VCORE",
"AD_OSC_SYNC_CK",
"AD_OSC_SYNC_CK_2",
"msdc_01_in_ck",
"msdc_02_in_ck",
"msdc_11_in_ck",
"msdc_12_in_ck",
"msdc_31_in_ck",
"msdc_31_in_ck",
"AD_OSC_CK_2",
};

static DEFINE_SPINLOCK(mt6763_clk_lock);

/* Total 13 subsys */
void __iomem *cksys_base;
void __iomem *infracfg_base;
void __iomem *apmixed_base;
void __iomem *audio_base;
void __iomem *cam_base;
void __iomem *img_base;
void __iomem *ipu_base;
void __iomem *mfgcfg_base;
void __iomem *mmsys_config_base;
void __iomem *pericfg_base;
void __iomem *mjc_base;
void __iomem *vdec_gcon_base;
void __iomem *venc_gcon_base;

/* CKSYS */
#define TST_SEL_1		(cksys_base + 0x024)
#define CLK_CFG_0		(cksys_base + 0x100)
#define CLK_CFG_1		(cksys_base + 0x110)
#define CLK_CFG_2		(cksys_base + 0x120)
#define CLK_CFG_5		(cksys_base + 0x150)
#define CLK_CFG_7		(cksys_base + 0x170)
#define CLK_CFG_9		(cksys_base + 0x190)
#define CLK_CFG_10		(cksys_base + 0x1a0)
#define CLK_CFG_20		(cksys_base + 0x210)
#define CLK_CFG_21		(cksys_base + 0x214)
#define CLK_MISC_CFG_1		(cksys_base + 0x414)
#define CLK26CALI_0		(cksys_base + 0x520)
#define CLK26CALI_1		(cksys_base + 0x524)
#define CLK26CALI_2		(cksys_base + 0x528)
#define TOP_CLK2		(cksys_base + 0x0120)
#define CLK_SCP_CFG_0		(cksys_base + 0x0400)
#define CLK_SCP_CFG_1		(cksys_base + 0x0404)

/* CG */
#define INFRA_PDN_SET0		(infracfg_base + 0x0080)
#define INFRA_PDN_SET1		(infracfg_base + 0x0088)
#define INFRA_PDN_SET2		(infracfg_base + 0x00A4)
#define INFRA_PDN_SET3		(infracfg_base + 0x00B0)
#define INFRA_PDN_SET4		(infracfg_base + 0x00BC)

#define AP_PLL_CON3		(apmixed_base + 0x000C)
#define AP_PLL_CON4		(apmixed_base + 0x0010)
#define FSMIPLL_CON0		(apmixed_base + 0x0200)
#define FSMIPLL_PWR_CON0	(apmixed_base + 0x020C)
#define GPUPLL_CON0		(apmixed_base + 0x0210)
#define GPUPLL_PWR_CON0		(apmixed_base + 0x021C)
#define UNIVPLL_CON0		(apmixed_base + 0x0240)
#define UNIVPLL_PWR_CON0	(apmixed_base + 0x024C)
#define MSDCPLL_CON0		(apmixed_base + 0x0250)
#define MSDCPLL_PWR_CON0	(apmixed_base + 0x025C)
#define MMPLL_CON0		(apmixed_base + 0x0260)
#define MMPLL_PWR_CON0		(apmixed_base + 0x026C)
#define VCODECPLL_CON0		(apmixed_base + 0x0270)
#define VCODECPLL_CON1		(apmixed_base + 0x0274)
#define VCODECPLL_CON2		(apmixed_base + 0x0278)
#define VCODECPLL_PWR_CON0	(apmixed_base + 0x027C)
#define TVDPLL_CON0		(apmixed_base + 0x0280)
#define TVDPLL_PWR_CON0		(apmixed_base + 0x028C)
#define EMIPLL_CON0		(apmixed_base + 0x0290)
#define APLL1_CON0		(apmixed_base + 0x02A0)
#define APLL1_PWR_CON0		(apmixed_base + 0x02B8)
#define APLL2_CON0		(apmixed_base + 0x02C0)
#define APLL2_PWR_CON0		(apmixed_base + 0x02D8)

#define ARMPLL1_CON0		(apmixed_base + 0x0310)
#define ARMPLL1_CON1		(apmixed_base + 0x0314)
#define ARMPLL1_PWR_CON0	(apmixed_base + 0x031C)
#define ARMPLL2_CON0		(apmixed_base + 0x0320)
#define ARMPLL2_CON1		(apmixed_base + 0x0324)
#define ARMPLL2_PWR_CON0	(apmixed_base + 0x032C)
#define ARMPLL3_CON0		(apmixed_base + 0x0330)
#define ARMPLL3_CON1		(apmixed_base + 0x0334)
#define ARMPLL3_PWR_CON0	(apmixed_base + 0x033C)

#define AUDIO_TOP_CON0		(audio_base + 0x0000)
#define AUDIO_TOP_CON1		(audio_base + 0x0004)

#define CAMSYS_CG_CON		(cam_base + 0x0000)
#define CAMSYS_CG_SET		(cam_base + 0x0004)
#define CAMSYS_CG_CLR		(cam_base + 0x0008)

#define IMG_CG_CON		(img_base + 0x0000)
#define IMG_CG_SET		(img_base + 0x0004)
#define IMG_CG_CLR		(img_base + 0x0008)

#define IPU_CG_CON              (ipu_base + 0x0000)
#define IPU_CG_SET              (ipu_base + 0x0004)
#define IPU_CG_CLR              (ipu_base + 0x0008)

#define MFG_CG_CON              (mfgcfg_base + 0x0000)
#define MFG_CG_SET              (mfgcfg_base + 0x0004)
#define MFG_CG_CLR              (mfgcfg_base + 0x0008)

#define MM_CG_CON0            (mmsys_config_base + 0x100)
#define MM_CG_SET0            (mmsys_config_base + 0x104)
#define MM_CG_CLR0            (mmsys_config_base + 0x108)
#define MM_CG_CON1            (mmsys_config_base + 0x110)
#define MM_CG_SET1            (mmsys_config_base + 0x114)
#define MM_CG_CLR1            (mmsys_config_base + 0x118)
#define MM_CG_CON2            (mmsys_config_base + 0x140)
#define MM_CG_SET2            (mmsys_config_base + 0x144)
#define MM_CG_CLR2            (mmsys_config_base + 0x148)

#define PERI_CG_SET0             (pericfg_base + 0x0270)
#define PERI_CG_CLR0             (pericfg_base + 0x0274)
#define PERI_CG_STA0             (pericfg_base + 0x0278)
#define PERI_CG_SET1             (pericfg_base + 0x0280)
#define PERI_CG_CLR1             (pericfg_base + 0x0284)
#define PERI_CG_STA1             (pericfg_base + 0x0288)
#define PERI_CG_SET2             (pericfg_base + 0x0290)
#define PERI_CG_CLR2             (pericfg_base + 0x0294)
#define PERI_CG_STA2             (pericfg_base + 0x0298)
#define PERI_CG_SET3             (pericfg_base + 0x02A0)
#define PERI_CG_CLR3             (pericfg_base + 0x02A4)
#define PERI_CG_STA3             (pericfg_base + 0x02A8)
#define PERI_CG_SET4             (pericfg_base + 0x02B0)
#define PERI_CG_CLR4             (pericfg_base + 0x02B4)
#define PERI_CG_STA4             (pericfg_base + 0x02B8)
#define PERI_CG_SET5             (pericfg_base + 0x02C0)
#define PERI_CG_CLR5             (pericfg_base + 0x02C4)
#define PERI_CG_STA5             (pericfg_base + 0x02C8)

#define MJC_CG_CON              (mjc_base + 0x0000)
#define MJC_CG_SET              (mjc_base + 0x0004)
#define MJC_CG_CLR              (mjc_base + 0x0008)

#define VDEC_CKEN_SET           (vdec_gcon_base + 0x0000)
#define VDEC_CKEN_CLR           (vdec_gcon_base + 0x0004)
#define LARB1_CKEN_SET          (vdec_gcon_base + 0x0008)
#define LARB1_CKEN_CLR          (vdec_gcon_base + 0x000C)


#define VENC_CG_CON		(venc_gcon_base + 0x0000)
#define VENC_CG_SET		(venc_gcon_base + 0x0004)
#define VENC_CG_CLR		(venc_gcon_base + 0x0008)


#define AUDIO_DISABLE_CG0 0x00004000 /* [14]APB3_SEL = 1 */
#define AUDIO_DISABLE_CG1 0x00000000 /* default: all power on */
#define CAMSYS_DISABLE_CG	0x1FFF
#define IMG_DISABLE_CG	0xFFF
#define IPU_DISABLE_CG	0x3FF
#define MFG_DISABLE_CG	0xF
#define MM_DISABLE_CG0	0xFFFFFFFF /* un-gating in preloader */
#define MM_DISABLE_CG1  0xFFFFFFFF /* un-gating in preloader */
#define MM_DISABLE_CG2  0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG0 0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG1 0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG2 0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG3 0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG4 0xFFFFFFFF /* un-gating in preloader */
#define PERI_DISABLE_CG5 0xFFFFFFFF /* un-gating in preloader */
#define MJC_DISABLE_CG 0x7F
#define VDEC_DISABLE_CG	0x111      /* inverse */
#define LARB_DISABLE_CG	0x1	  /* inverse */
#define VENC_DISABLE_CG 0x111111 /* inverse */

#define INFRA_CG0 0x00080000/*aes_top1[19], fhctl[30] cannot gate, suspend fail*/
#define INFRA_CG1 0x40400e00/*trng[9], cpum[11], smi_l2c[22], auxadc[10], anc_md32[30]*/

#define PERI_CG0 0x03ff00ff/*pwm0-7[7:0], i2c0-9[25:16]*/
/*uart0-7[7:0], spi0-10[26:16]*/
/*#define PERI_CG1 0x07ff00ff*/
#define PERI_CG1 0x07ff00fc
#define PERI_CG2 0x00010000/*flashif[16]*/
#define PERI_CG3 0x00000172/*usb_p1[1], mpcie[8][6:4]*/
#define PERI_CG4 0x11070053/*mbist_mem_off_dly[8] cannot gate, suspend fail*/

#define CAM_CG 0x00001fc7/*[12:6][2:0]*/
#define IMG_CG	0xFFF/**/
#define MFG_CG	0xF/*[3:0]*/
#define VDE_CG	0x1
#define LARB1_CG 0x1
#define VEN_CG 0x111111/*[20][16][12][8][4][0]*/
#define MJC_CG 0x7f/*[6:0]*/
#define IPU_CG 0x3ff/*[9:0]*/

#define MM_CG0	0x000007fe/*mdp series[10][8:5][3:2]*/
#define MM_CG1	0x00040040/*ipu[6], mdp2[18]*/
#define MM_CG2	0x0000c180/*pmeter[8:7], 32k,img2[15:14]*/

#define CK_CFG_0 0x100
#define CK_CFG_0_SET 0x104
#define CK_CFG_0_CLR 0x108
#define CK_CFG_1 0x110
#define CK_CFG_1_SET 0x114
#define CK_CFG_1_CLR 0x118
#define CK_CFG_2 0x120
#define CK_CFG_2_SET 0x124
#define CK_CFG_2_CLR 0x128
#define CK_CFG_3 0x130
#define CK_CFG_3_SET 0x134
#define CK_CFG_3_CLR 0x138
#define CK_CFG_4 0x140
#define CK_CFG_4_SET 0x144
#define CK_CFG_4_CLR 0x148
#define CK_CFG_5 0x150
#define CK_CFG_5_SET 0x154
#define CK_CFG_5_CLR 0x158
#define CK_CFG_6 0x160
#define CK_CFG_6_SET 0x164
#define CK_CFG_6_CLR 0x168
#define CK_CFG_7 0x170
#define CK_CFG_7_SET 0x174
#define CK_CFG_7_CLR 0x178
#define CK_CFG_8 0x180
#define CK_CFG_8_SET 0x184
#define CK_CFG_8_CLR 0x188
#define CK_CFG_9 0x190
#define CK_CFG_9_SET 0x194
#define CK_CFG_9_CLR 0x198
#define CK_CFG_10 0x1a0
#define CK_CFG_10_SET 0x1a4
#define CK_CFG_10_CLR 0x1a8
#define CK_CFG_11 0x1b0
#define CK_CFG_11_SET 0x1b4
#define CK_CFG_11_CLR 0x1b8
#define CK_CFG_12 0x1c0
#define CK_CFG_12_SET 0x1c4
#define CK_CFG_12_CLR 0x1c8
#define CK_CFG_13 0x1d0
#define CK_CFG_13_SET 0x1d4
#define CK_CFG_13_CLR 0x1d8
#define CK_CFG_14 0x1e0
#define CK_CFG_14_SET 0x1e4
#define CK_CFG_14_CLR 0x1e8
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
	FACTOR(TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_ck", 1,
		2),
	FACTOR(TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_ck", 1,
		7),
	FACTOR(TOP_UNIVPLL1_D8, "univpll1_d8", "univpll", 1,
		26),
	FACTOR(TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1,
		3),
	FACTOR(TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d2", 1,
		2),
	FACTOR(TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d2", 1,
		4),
	FACTOR(TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d2", 1,
		8),
	FACTOR(TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1,
		5),
	FACTOR(TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d3", 1,
		2),
	FACTOR(TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d3", 1,
		4),
	FACTOR(TOP_UNIVPLL3_D8, "univpll3_d8", "univpll_d3", 1,
		8),
	FACTOR(TOP_UNIVPLL_D7, "univpll_d7", "univpll", 1,
		7),
	FACTOR(TOP_UNIVPLL_D26, "univpll_d26", "univpll_ck", 1,
		26),
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
	"ulposcpll_d8"
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
	"mmpll_ck",
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
	"ulposcpll_d16",
	"ulposcpll_d4",
	"ulposcpll_d8"
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
	"ulposcpll_d4",
	"ulposcpll_d8"
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
};

/* TODO: remove audio clocks after audio driver ready */

static int mtk_cg_bit_is_cleared(struct clk_hw *hw)
{
	struct mtk_clk_gate *cg = to_clk_gate(hw);
	u32 val;

	regmap_read(cg->regmap, cg->sta_ofs, &val);

	val &= BIT(cg->bit);

	return val == 0;
}

static int mtk_cg_bit_is_set(struct clk_hw *hw)
{
	struct mtk_clk_gate *cg = to_clk_gate(hw);
	u32 val;

	regmap_read(cg->regmap, cg->sta_ofs, &val);

	val &= BIT(cg->bit);

	return val != 0;
}

static void mtk_cg_set_bit(struct clk_hw *hw)
{
	struct mtk_clk_gate *cg = to_clk_gate(hw);

	regmap_update_bits(cg->regmap, cg->sta_ofs, BIT(cg->bit), BIT(cg->bit));
}

static void mtk_cg_clr_bit(struct clk_hw *hw)
{
	struct mtk_clk_gate *cg = to_clk_gate(hw);

	regmap_update_bits(cg->regmap, cg->sta_ofs, BIT(cg->bit), 0);
}

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
		"axi_ck", 4),
	GATE_INFRA0(INFRACFG_AO_SEJ_CG, "infra_sej",
		"axi_sel", 5),
	GATE_INFRA0(INFRACFG_AO_APXGPT_CG, "infra_apxgpt",
		"f_f26m_ck", 6),
	GATE_INFRA0(INFRACFG_AO_ICUSB_CG, "infra_icusb",
		"f_f26m_ck", 8),
	GATE_INFRA0(INFRACFG_AO_GCE_CG, "infra_gce",
		"axi_sel", 9),
	GATE_INFRA0(INFRACFG_AO_THERM_CG, "infra_therm",
		"axi_sel", 10),
	GATE_INFRA0(INFRACFG_AO_I2C0_CG, "infra_i2c0",
		"f_f26m_ck", 11),
	GATE_INFRA0(INFRACFG_AO_I2C1_CG, "infra_i2c1",
		"f_f26m_ck", 12),
	GATE_INFRA0(INFRACFG_AO_I2C2_CG, "infra_i2c2",
		"axi_sel", 13),
	GATE_INFRA0(INFRACFG_AO_I2C3_CG, "infra_i2c3",
		"axi_sel", 14),
	GATE_INFRA0(INFRACFG_AO_PWM_HCLK_CG, "infra_pwm_hclk",
		"axi_sel", 15),
	GATE_INFRA0(INFRACFG_AO_PWM1_CG, "infra_pwm1",
		"axi_sel", 16),
	GATE_INFRA0(INFRACFG_AO_PWM2_CG, "infra_pwm2",
		"axi_sel", 17),
	GATE_INFRA0(INFRACFG_AO_PWM3_CG, "infra_pwm3",
		"axi_sel", 18),
	GATE_INFRA0(INFRACFG_AO_PWM4_CG, "infra_pwm4",
		"axi_sel", 19),
	GATE_INFRA0(INFRACFG_AO_PWM_CG, "infra_pwm",
		"axi_sel", 21),
	GATE_INFRA0(INFRACFG_AO_UART0_CG, "infra_uart0",
		"axi_sel", 22),
	GATE_INFRA0(INFRACFG_AO_UART1_CG, "infra_uart1",
		"axi_sel", 22),
	GATE_INFRA0(INFRACFG_AO_UART2_CG, "infra_uart2",
		"axi_sel", 24),
	GATE_INFRA0(INFRACFG_AO_UART3_CG, "infra_uart3",
		"f_f26m_ck", 25),
	GATE_INFRA0(INFRACFG_AO_GCE_26M_CG, "infra_gce_26m",
		"axi_ck", 27),
	GATE_INFRA0(INFRACFG_AO_CQ_DMA_FPC_CG, "infra_cqdma_fpc",
		"f_f26m_ck", 28),
	GATE_INFRA0(INFRACFG_AO_BTIF_CG, "infra_btif",
		"f_f26m_ck", 31),
	/* INFRA1 */
	GATE_INFRA1(INFRACFG_AO_SPI0_CG, "infra_spi0",
		"axi_sel", 1),
	GATE_INFRA1(INFRACFG_AO_MSDC0_CG, "infra_msdc0",
		"axi_sel", 2),
	GATE_INFRA1(INFRACFG_AO_MSDC1_CG, "infra_msdc1",
		"axi_sel", 4),
	GATE_INFRA1(INFRACFG_AO_MSDC2_CG, "infra_msdc2",
		"axi_sel", 5),
	GATE_INFRA1(INFRACFG_AO_MSDC0_SCK_CG, "infra_msdc0_sck",
		"axi_sel", 6),
	GATE_INFRA1(INFRACFG_AO_DVFSRC_CG, "infra_dvfsrc",
		"axi_sel", 7),
	GATE_INFRA1(INFRACFG_AO_GCPU_CG, "infra_gcpu",
		"f_f26m_ck", 8),
	GATE_INFRA1(INFRACFG_AO_TRNG_CG, "infra_trng",
		"axi_sel", 9),
	GATE_INFRA1(INFRACFG_AO_AUXADC_CG, "infra_auxadc",
		"axi_sel", 10),
	GATE_INFRA1(INFRACFG_AO_CPUM_CG, "infra_cpum",
		"axi_sel", 11),
	GATE_INFRA1(INFRACFG_AO_CCIF1_AP_CG, "infra_ccif1_ap",
		"axi_sel", 12),
	GATE_INFRA1(INFRACFG_AO_CCIF1_MD_CG, "infra_ccif1_md",
		"axi_sel", 13),
	GATE_INFRA1(INFRACFG_AO_AUXADC_MD_CG, "infra_auxadc_md",
		"f_f26m_ck", 14),
	GATE_INFRA1(INFRACFG_AO_MSDC1_SCK_CG, "infra_msdc1_sck",
		"axi_sel", 16),
	GATE_INFRA1(INFRACFG_AO_MSDC2_SCK_CG, "infra_msdc2_sck",
		"mm_ck", 17),
	GATE_INFRA1(INFRACFG_AO_AP_DMA_CG, "infra_apdma",
		"axi_sel", 18),
	GATE_INFRA1(INFRACFG_AO_XIU_CG, "infra_xiu",
		"axi_sel", 19),
	GATE_INFRA1(INFRACFG_AO_DEVICE_APC_CG, "infra_device_apc",
		"axi_sel", 20),
	GATE_INFRA1(INFRACFG_AO_CCIF_AP_CG, "infra_ccif_ap",
		"mem_ck", 23),
	GATE_INFRA1(INFRACFG_AO_DEBUGSYS_CG, "infra_debugsys",
		"f_f26m_ck", 24),
	GATE_INFRA1(INFRACFG_AO_AUDIO_CG, "infra_audio",
		"f_f26m_ck", 25),
	GATE_INFRA1(INFRACFG_AO_CCIF_MD_CG, "infra_ccif_md",
		"ancmd32_ck", 26),
	GATE_INFRA1(INFRACFG_AO_DXCC_SEC_CORE_CG, "infra_dxcc_sec_core",
		"f_f26m_ck", 27),
	GATE_INFRA1(INFRACFG_AO_DXCC_AO_CG, "infra_dxcc_ao",
		"axi_sel", 28),
	GATE_INFRA1(INFRACFG_AO_DRAMC_F26M_CG, "infra_dramc_f26m",
		"axi_sel", 31),
	/* INFRA2 */
	GATE_INFRA2(INFRACFG_AO_IRTX_CG, "infra_irtx",
		"f_f26m_ck", 0),
	GATE_INFRA2(INFRACFG_AO_DISP_PWM_CG, "infra_disppwm",
		"f_f26m_ck", 2),
	GATE_INFRA2(INFRACFG_AO_CLDMA_BCLK_CK, "infra_cldma_bclk",
		"f_f26m_ck", 3),
	GATE_INFRA2(INFRACFG_AO_AUDIO_26M_BCLK_CK, "infracfg_ao_audio_26m_bclk_ck",
		"f_f26m_ck", 4),
	GATE_INFRA2(INFRACFG_AO_SPI1_CG, "infra_spi1",
		"f_f26m_ck", 6),
	GATE_INFRA2(INFRACFG_AO_I2C4_CG, "infra_i2c4",
		"f_f26m_ck", 7),
	GATE_INFRA2(INFRACFG_AO_MODEM_TEMP_SHARE_CG, "infra_md_tmp_share",
		"f_f26m_ck", 8),
	GATE_INFRA2(INFRACFG_AO_SPI2_CG, "infra_spi2",
		"f_f26m_ck", 9),
	GATE_INFRA2(INFRACFG_AO_SPI3_CG, "infra_spi3",
		"f_f26m_ck", 10),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_SCK_CG, "infra_unipro_sck",
		"f_f26m_ck", 11),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_TICK_CG, "infra_unipro_tick",
		"f_f26m_ck", 12),
	GATE_INFRA2(INFRACFG_AO_UFS_MP_SAP_BCLK_CG, "infra_ufs_mp_sap_bck",
		"f_f26m_ck", 13),
	GATE_INFRA2(INFRACFG_AO_MD32_BCLK_CG, "infra_md32_bclk",
		"f_f26m_ck", 14),
	GATE_INFRA2(INFRACFG_AO_SSPM_CG, "infra_sspm",
		"f_f26m_ck", 15),
	GATE_INFRA2(INFRACFG_AO_UNIPRO_MBIST_CG, "infra_unipro_mbist",
		"f_f26m_ck", 16),
	GATE_INFRA2(INFRACFG_AO_SSPM_BUS_HCLK_CG, "infra_sspm_bus_hclk",
		"f_f26m_ck", 17),
	GATE_INFRA2(INFRACFG_AO_I2C5_CG, "infra_i2c5",
		"f_f26m_ck", 18),
	GATE_INFRA2(INFRACFG_AO_I2C5_ARBITER_CG, "infra_i2c5_arbiter",
		"f_f26m_ck", 19),
	GATE_INFRA2(INFRACFG_AO_I2C5_IMM_CG, "infra_i2c5_imm",
		"f_f26m_ck", 20),
	GATE_INFRA2(INFRACFG_AO_I2C1_ARBITER_CG, "infra_i2c1_arbiter",
		"f_f26m_ck", 21),
	GATE_INFRA2(INFRACFG_AO_I2C1_IMM_CG, "infra_i2c1_imm",
		"f_f26m_ck", 22),
	GATE_INFRA2(INFRACFG_AO_I2C2_ARBITER_CG, "infra_i2c2_arbiter",
		"f_f26m_ck", 23),
	GATE_INFRA2(INFRACFG_AO_I2C2_IMM_CG, "infra_i2c2_imm",
		"f_f26m_ck", 24),
	GATE_INFRA2(INFRACFG_AO_SPI4_CG, "infra_spi4",
		"f_f26m_ck", 25),
	GATE_INFRA2(INFRACFG_AO_SPI5_CG, "infra_spi5",
		"f_f26m_ck", 26),
	GATE_INFRA2(INFRACFG_AO_CQ_DMA_CG, "infra_cqdma",
		"f_f26m_ck", 27),
	GATE_INFRA2(INFRACFG_AO_UFS_CG, "infra_ufs",
		"f_f26m_ck", 28),
	GATE_INFRA2(INFRACFG_AO_AES_CG, "infra_aes",
		"f_f26m_ck", 29),
	GATE_INFRA2(INFRACFG_AO_UFS_TICK_CG, "infra_ufs_tick",
		"f_f26m_ck", 30),
	/* INFRA3 */
	GATE_INFRA3(INFRACFG_AO_MSDC0_SELF_CG, "infra_msdc0_self",
		"f_f26m_ck", 0),
	GATE_INFRA3(INFRACFG_AO_MSDC1_SELF_CG, "infra_msdc1_self",
		"f_f26m_ck", 1),
	GATE_INFRA3(INFRACFG_AO_MSDC2_SELF_CG, "infra_msdc2_self",
		"f_f26m_ck", 2),
	GATE_INFRA3(INFRACFG_AO_SSPM_26M_SELF_CG, "infra_sspm_26m_self",
		"f_f26m_ck", 3),
	GATE_INFRA3(INFRACFG_AO_SSPM_32K_SELF_CG, "infra_sspm_32k_self",
		"f_f26m_ck", 4),
	GATE_INFRA3(INFRACFG_AO_UFS_AXI_CG, "infra_ufs_axi",
		"f_f26m_ck", 5),
	GATE_INFRA3(INFRACFG_AO_I2C6_CG, "infra_i2c6",
		"f_f26m_ck", 6),
	GATE_INFRA3(INFRACFG_AO_AP_MSDC0_CG, "infra_ap_msdc0",
		"f_f26m_ck", 7),
	GATE_INFRA3(INFRACFG_AO_MD_MSDC0_CG, "infra_md_msdc0",
		"f_f26m_ck", 8),
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
	GATE_IMG(IMG_LARB5, "imgsys_larb5", "img_sel", 0),
	GATE_IMG(IMG_DFP_VAD, "imgsys_dfp_vad", "img_sel", 1),
	GATE_IMG(IMG_DIP, "imgsys_dip", "img_sel", 6),
	GATE_IMG(IMG_DPE, "imgsys_dpe", "img_sel", 10),
	GATE_IMG(IMG_FDVT, "imgsys_fdvt", "img_sel", 11),
	GATE_IMG(IMG_GEPF, "imgsys_gepf", "img_sel", 12),
	GATE_IMG(IMG_RSC, "imgsys_rsc", "img_sel", 13),
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

static const struct mtk_gate_regs mm2_cg_regs = {
	.set_ofs = 0x144,
	.clr_ofs = 0x148,
	.sta_ofs = 0x140,
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
	GATE_MM1(MMSYS_DSI0_IF_CK, "mm_dsi0_ifck", "mm_sel", 1),
	GATE_MM1(MMSYS_DSI1_MM_CK, "mm_dsi1_mmck", "mm_sel", 2),
	GATE_MM1(MMSYS_DSI1_IF_CK, "mm_dsi1_ifck", "mm_sel", 3),
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
		"venc_sel", 0),
	GATE_VENC_GLOBAL_CON(VENC_GCON_VENC, "venc_venc",
		"venc_sel", 4),
	GATE_VENC_GLOBAL_CON(VENC_GCON_JPGENC, "venc_jpgenc",
		"venc_sel", 8),
	GATE_VENC_GLOBAL_CON(VENC_GCON_VDEC, "venc_vdec",
		"venc_sel", 12),
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
	/*clk_writel(CLK_SCP_CFG_0, clk_readl(CLK_SCP_CFG_0) | 0x3EF);*/
	/*clk_writel(CLK_SCP_CFG_1, clk_readl(CLK_SCP_CFG_1) | 0x15);*/
	/*mtk_clk_enable_critical();*/
#if 0
	/* USB20 MUX PDN */
	clk_writel(cksys_base + CK_CFG_3_CLR, 0x80000000);
	clk_writel(cksys_base + CK_CFG_3_SET, 0x80000000);
	/* I2C MUX PDN */
	clk_writel(cksys_base + CK_CFG_2_CLR, 0x80000000);
	clk_writel(cksys_base + CK_CFG_2_SET, 0x80000000);
	/* MSDC50_HCLK MUX PDN */
	clk_writel(cksys_base + CK_CFG_4_CLR, 0x00008000);
	clk_writel(cksys_base + CK_CFG_4_SET, 0x00008000);
	/* MSDC53_HCLK MUX PDN */
	clk_writel(cksys_base + CK_CFG_5_CLR, 0x00800000);
	clk_writel(cksys_base + CK_CFG_5_SET, 0x00800000);
	/* ufo_dec mux pdn - OK */
	clk_writel(cksys_base + CK_CFG_10_CLR, 0x80000000);
	clk_writel(cksys_base + CK_CFG_10_SET, 0x80000000);
	/* seninf/ipu_if mux pdn */
	clk_writel(cksys_base + CK_CFG_9_CLR, 0x80800000);
	clk_writel(cksys_base + CK_CFG_9_SET, 0x80800000);
	/* pcie_mac mux pdn */
	clk_writel(cksys_base + CK_CFG_11_CLR, 0x00000080);
	clk_writel(cksys_base + CK_CFG_11_SET, 0x00000080);
	/* ufs_card mux pdn */
	clk_writel(cksys_base + CK_CFG_13_CLR, 0x00000080);
	clk_writel(cksys_base + CK_CFG_13_SET, 0x00000080);
	/* andmd32/audio_h mux pdn */
	clk_writel(cksys_base + CK_CFG_12_CLR, 0x00800080);
	clk_writel(cksys_base + CK_CFG_12_SET, 0x00800080);
#endif
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
	/*clk_writel(INFRA_PDN_SET0, INFRA_CG0);*/
	/*clk_writel(INFRA_PDN_SET1, INFRA_CG1);*/
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
#if 0
	PLL(CLK_APMIXED_ARMPLL1, "armpll1", 0x0310, 0x031C, 0xfc0001c1, 0,
		22, 0x0314, 28, 0x0, 0x0314, 0),
	PLL(CLK_APMIXED_ARMPLL2, "armpll2", 0x0320, 0x032C, 0xfc0001c1, 0,
		22, 0x0324, 28, 0x0, 0x0324, 0),
	PLL(CLK_APMIXED_ARMPLL3, "armpll3", 0x0330, 0x033C, 0xfc0001c1, 0,
		22, 0x0334, 28, 0x0, 0x0334, 0),
	PLL(CLK_APMIXED_ARMPLL4, "armpll4", 0x0340, 0x034C, 0xfc0001c1, 0,
		22, 0x0344, 28, 0x0, 0x0344, 0),
	PLL(CLK_APMIXED_CCIPLL, "ccipll", 0x0350, 0x035C, 0xfc0001c1, 0,
		22, 0x0354, 28, 0x0, 0x0354, 0),
	PLL(CLK_APMIXED_MAINPLL, "mainpll", 0x0230, 0x023C, 0xfc0001c1, 0,
		22, 0x0234, 28, 0x0, 0x0234, 0),
#endif
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
#if 0
/*GPUPLL*/
	clk_clrl(GPUPLL_CON0, PLL_EN);
	clk_setl(GPUPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(GPUPLL_PWR_CON0, PLL_PWR_ON);
/*FSMIPLL*/
	clk_clrl(FSMIPLL_CON0, PLL_EN);
	clk_setl(FSMIPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(FSMIPLL_PWR_CON0, PLL_PWR_ON);
/*UNIVPLL*/
#if 0
	clk_clrl(UNIVPLL_CON0, PLL_EN);
	clk_setl(UNIVPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(UNIVPLL_PWR_CON0, PLL_PWR_ON);
#endif
/*MSDCPLL*/
#if 1
	clk_clrl(MSDCPLL_CON0, PLL_EN);
	clk_setl(MSDCPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MSDCPLL_PWR_CON0, PLL_PWR_ON);
#endif
/*MMPLL*/
	clk_clrl(MMPLL_CON0, PLL_EN);
	clk_setl(MMPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MMPLL_PWR_CON0, PLL_PWR_ON);
/*VCODECPLL*/
	clk_clrl(VCODECPLL_CON0, PLL_EN);
	clk_setl(VCODECPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(VCODECPLL_PWR_CON0, PLL_PWR_ON);
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
#endif
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

#if 0/*MT_CCF_BRINGUP*/
	clk_writel(AUDIO_TOP_CON0, AUDIO_DISABLE_CG0);
	clk_writel(AUDIO_TOP_CON1, AUDIO_DISABLE_CG1);
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

#if 0/*MT_CCF_BRINGUP*/
	/*clk_writel(CAMSYS_CG_CLR, CAMSYS_DISABLE_CG);*/
	clk_writel(CAMSYS_CG_SET, CAM_CG);
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

#if 0/*MT_CCF_BRINGUP*/
	/*clk_writel(IMG_CG_CLR, IMG_DISABLE_CG);*/
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

#if 0/*MT_CCF_BRINGUP*/
	/*clk_writel(MFG_CG_CLR, MFG_DISABLE_CG);*/
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
#if 0/*MT_CCF_BRINGUP*/
	clk_writel(MM_CG_SET0, MM_CG0);
	clk_writel(MM_CG_SET1, MM_CG1);
	clk_writel(MM_CG_SET2, MM_CG2);
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

#if 0/*MT_CCF_BRINGUP*/
	/*clk_writel(VENC_CG_SET, VENC_DISABLE_CG);*/
	clk_writel(VENC_CG_CLR, VEN_CG);
#endif
}
CLK_OF_DECLARE(mtk_venc_global_con, "mediatek,venc_gcon",
		mtk_venc_global_con_init);

#if 0
unsigned int mt_get_ckgen_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_1, clk26cali_2;

	clk_dbg_cfg = clk_readl(CLK_CFG_21);
	clk_writel(CLK_CFG_21, (clk_dbg_cfg & 0xFF80FFFF)|(ID << 16));

	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);
	clk_writel(CLK_MISC_CFG_1, 0x00000000);

	clk26cali_2 = clk_readl(CLK26CALI_2);
	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk_writel(CLK26CALI_0, 0x80);
	clk_writel(CLK26CALI_0, 0x90);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x10) {
	mdelay(10);
	i++;
	if (i > 10)
		break;
	}

	temp = clk_readl(CLK26CALI_2) & 0xFFFF;

	output = (temp * 26000) / 1024;

	clk_writel(CLK_CFG_21, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_2, clk26cali_2);

	/*print("ckgen meter[%d] = %d Khz\n", ID, output);*/
	return output;

}

unsigned int mt_get_abist_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_1, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_CFG_20);
	clk_writel(CLK_CFG_20, (clk_dbg_cfg & 0xFFFF80FF)|(ID << 8)|(0x01 << 31));

	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);
	clk_writel(CLK_MISC_CFG_1, 0x00000000);

	clk26cali_1 = clk_readl(CLK26CALI_1);
	clk26cali_0 = clk_readl(CLK26CALI_0);

	clk_writel(CLK26CALI_0, 0x80);
	clk_writel(CLK26CALI_0, 0x81);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x01) {
		mdelay(10);
		i++;
		if (i > 10)
		break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024;

	clk_writel(CLK_CFG_20, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	/*pr_debug("%s = %d Khz\n", abist_array[ID-1], output);*/
	return output;
}

unsigned int mt_get_abist2_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_1, clk26cali_1;

	clk_dbg_cfg = clk_readl(CLK_CFG_20);
	clk_writel(CLK_CFG_20, (clk_dbg_cfg & 0xFFFF80FF)|(ID << 8)|(0x01 << 31)|(0x01 << 14));

	clk_misc_cfg_1 = clk_readl(CLK_MISC_CFG_1);
	clk_writel(CLK_MISC_CFG_1, 0x00000000);

	clk26cali_1 = clk_readl(CLK26CALI_1);
	clk26cali_0 = clk_readl(CLK26CALI_0);

	clk_writel(CLK26CALI_0, 0x80);
	clk_writel(CLK26CALI_0, 0x81);

	/* wait frequency meter finish */
	while (clk_readl(CLK26CALI_0) & 0x01) {
		mdelay(10);
		i++;
		if (i > 10)
		break;
	}

	temp = clk_readl(CLK26CALI_1) & 0xFFFF;

	output = (temp * 26000) / 1024;

	clk_writel(CLK_CFG_20, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_1, clk_misc_cfg_1);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	/*pr_debug("%s = %d Khz\n", abist_array[ID-1], output);*/
	return output;
}

void switch_mfg_clk(int src)
{
	if (src == 0)
		clk_writel(TOP_CLK2, clk_readl(TOP_CLK2)&0xfffffcff);
	else
		clk_writel(TOP_CLK2, (clk_readl(TOP_CLK2)&0xfffffcff)|(0x01<<8));
}

void mp_enter_suspend(int id, int suspend)
{
	/* mp0*/
	if (id == 0) {
		if (suspend) {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xfdff7fdf);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xfdff7fdf);
		} else {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x02008020);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x02008020);
		}
	} else if (id == 1) { /* mp1 */
		if (suspend) {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xfbfeffbf);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xfbfeffbf);
		} else {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x04010040);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x04010040);
		}
	}
}

void pll_if_on(void)
{
	if (clk_readl(UNIVPLL_CON0) & 0x1)
		pr_err("suspend warning: UNIVPLL is on!!!\n");
	if (clk_readl(GPUPLL_CON0) & 0x1)
		pr_err("suspend warning: GPUPLL is on!!!\n");
	if (clk_readl(MMPLL_CON0) & 0x1)
		pr_err("suspend warning: MMPLL is on!!!\n");
	if (clk_readl(MSDCPLL_CON0) & 0x1)
		pr_err("suspend warning: MSDCPLL is on!!!\n");
	if (clk_readl(FSMIPLL_CON0) & 0x1)
		pr_err("suspend warning: FSMIPLL is on!!!\n");
	if (clk_readl(VCODECPLL_CON0) & 0x1)
		pr_err("suspend warning: VCODECPLL is on!!!\n");
	if (clk_readl(TVDPLL_CON0) & 0x1)
		pr_err("suspend warning: TVDPLL is on!!!\n");
	if (clk_readl(APLL1_CON0) & 0x1)
		pr_err("suspend warning: APLL1 is on!!!\n");
	if (clk_readl(APLL2_CON0) & 0x1)
		pr_err("suspend warning: APLL2 is on!!!\n");
}

#define AUDIO_ENABLE_CG0 0x0F0C0304 /* [14]APB3_SEL = 1 */
#define AUDIO_ENABLE_CG1 0x0F330000 /* default: all power on */

#define INFRA_ENABLE_CG0 0xFFFFFFFF
#define INFRA_ENABLE_CG1 0xFFFFFFFF
#define INFRA_ENABLE_CG2 0xFFFFFFFF
#define INFRA_ENABLE_CG3 0xFFFFFFFF
#define INFRA_ENABLE_CG4 0xFFFFFFFF

#define CAMSYS_ENABLE_CG	0x1FFF
#define IMG_ENABLE_CG	0xFFF
#define IPU_ENABLE_CG	0x3FF
#define MFG_ENABLE_CG	0xF
#define MM_ENABLE_CG0	0xFFFFFFFF /* un-gating in preloader */
#define MM_ENABLE_CG1  0xFFFFFFFF /* un-gating in preloader */
#define MM_ENABLE_CG2  0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG0 0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG1 0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG2 0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG3 0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG4 0xFFFFFFFF /* un-gating in preloader */
#define PERI_ENABLE_CG5 0xFFFFFFFF /* un-gating in preloader */
#define MJC_ENABLE_CG 0x7F
#define VDEC_ENABLE_CG	0x111      /* inverse */
#define LARB_ENABLE_CG	0x1	  /* inverse */
#define VENC_ENABLE_CG 0x111111 /* inverse */

void clock_force_off(void)
{
	#if 0
	/*INFRA_AO CG*/
	clk_writel(INFRA_PDN_SET0, INFRA_ENABLE_CG0);
	clk_writel(INFRA_PDN_SET1, INFRA_ENABLE_CG1);
	clk_writel(INFRA_PDN_SET2, INFRA_ENABLE_CG2);
	clk_writel(INFRA_PDN_SET3, INFRA_ENABLE_CG3);
	clk_writel(INFRA_PDN_SET4, INFRA_ENABLE_CG4);
	/*PERI CG*/
	clk_writel(PERI_CG_SET0, PERI_ENABLE_CG0);
	clk_writel(PERI_CG_SET1, PERI_ENABLE_CG1);
	clk_writel(PERI_CG_SET2, PERI_ENABLE_CG2);
	clk_writel(PERI_CG_SET3, PERI_ENABLE_CG3);
	clk_writel(PERI_CG_SET4, PERI_ENABLE_CG4);
	clk_writel(PERI_CG_SET5, PERI_ENABLE_CG5);
	#endif
	/*DISP CG*/
	clk_writel(MM_CG_SET0, MM_ENABLE_CG0);
	clk_writel(MM_CG_SET1, MM_ENABLE_CG1);
	clk_writel(MM_CG_SET2, MM_ENABLE_CG2);
	/*AUDIO*/
	clk_writel(AUDIO_TOP_CON0, AUDIO_ENABLE_CG0);
	clk_writel(AUDIO_TOP_CON1, AUDIO_ENABLE_CG1);
	/*MFG*/
	clk_writel(MFG_CG_SET, MFG_ENABLE_CG);
	/*ISP*/
	clk_writel(IMG_CG_SET, IMG_ENABLE_CG);
	/*VDE not inverse*/
	clk_writel(VDEC_CKEN_CLR, VDEC_ENABLE_CG);
	clk_writel(LARB1_CKEN_CLR, LARB_ENABLE_CG);
	/*VENC not inverse*/
	clk_writel(VENC_CG_CLR, VENC_ENABLE_CG);
	/*MJC*/
	clk_writel(MJC_CG_SET, MJC_ENABLE_CG);
	/*CAM*/
	clk_writel(CAMSYS_CG_SET, CAMSYS_ENABLE_CG);
	/*IPU*/
	clk_writel(IPU_CG_SET, IPU_ENABLE_CG);
}

void mmsys_cg_check(void)
{
	pr_err("[MM_CG_CON0]=0x%08x\n", clk_readl(MM_CG_CON0));
	pr_err("[MM_CG_CON1]=0x%08x\n", clk_readl(MM_CG_CON1));
	pr_err("[MM_CG_CON2]=0x%08x\n", clk_readl(MM_CG_CON2));
}

void mfgsys_cg_sts(void)
{
	pr_err("[CLK_CFG_2]=0x%08x\n", clk_readl(CLK_CFG_2));
}

unsigned int mfgsys_cg_check(void)
{
	return clk_readl(MFG_CG_CON);
}

void pll_force_off(void)
{
/*GPUPLL*/
	clk_clrl(GPUPLL_CON0, PLL_EN);
	clk_setl(GPUPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(GPUPLL_PWR_CON0, PLL_PWR_ON);
/*FSMIPLL*/
	clk_clrl(FSMIPLL_CON0, PLL_EN);
	clk_setl(FSMIPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(FSMIPLL_PWR_CON0, PLL_PWR_ON);
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
/*VCODECPLL*/
	clk_clrl(VCODECPLL_CON0, PLL_EN);
	clk_setl(VCODECPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(VCODECPLL_PWR_CON0, PLL_PWR_ON);
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
			mt_reg_sync_writel((clk_readl(ARMPLL1_PWR_CON0) | 0x01), ARMPLL1_PWR_CON0);
			udelay(100);
			mt_reg_sync_writel((clk_readl(ARMPLL1_PWR_CON0) & 0xfffffffd), ARMPLL1_PWR_CON0);
			udelay(10);
			mt_reg_sync_writel((clk_readl(ARMPLL1_CON1) | 0x80000000), ARMPLL1_CON1);
			mt_reg_sync_writel((clk_readl(ARMPLL1_CON0) | 0x01), ARMPLL1_CON0);
			udelay(100);
		} else {
			mt_reg_sync_writel((clk_readl(ARMPLL1_CON0) & 0xfffffffe), ARMPLL1_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL1_PWR_CON0) | 0x00000002), ARMPLL1_PWR_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL1_PWR_CON0) & 0xfffffffe), ARMPLL1_PWR_CON0);
		}
	} else if (id == 2) {
		if (on) {
			mt_reg_sync_writel((clk_readl(ARMPLL2_PWR_CON0) | 0x01), ARMPLL2_PWR_CON0);
			udelay(100);
			mt_reg_sync_writel((clk_readl(ARMPLL2_PWR_CON0) & 0xfffffffd), ARMPLL2_PWR_CON0);
			udelay(10);
			mt_reg_sync_writel((clk_readl(ARMPLL2_CON1) | 0x80000000), ARMPLL2_CON1);
			mt_reg_sync_writel((clk_readl(ARMPLL2_CON0) | 0x01), ARMPLL2_CON0);
			udelay(100);
		} else {
			mt_reg_sync_writel((clk_readl(ARMPLL2_CON0) & 0xfffffffe), ARMPLL2_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL2_PWR_CON0) | 0x00000002), ARMPLL2_PWR_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL2_PWR_CON0) & 0xfffffffe), ARMPLL2_PWR_CON0);
		}
	} else if (id == 3) {
		if (on) {
			mt_reg_sync_writel((clk_readl(ARMPLL3_PWR_CON0) | 0x01), ARMPLL3_PWR_CON0);
			udelay(100);
			mt_reg_sync_writel((clk_readl(ARMPLL3_PWR_CON0) & 0xfffffffd), ARMPLL3_PWR_CON0);
			udelay(10);
			mt_reg_sync_writel((clk_readl(ARMPLL3_CON1) | 0x80000000), ARMPLL3_CON1);
			mt_reg_sync_writel((clk_readl(ARMPLL3_CON0) | 0x01), ARMPLL3_CON0);
			udelay(100);
		} else {
			mt_reg_sync_writel((clk_readl(ARMPLL3_CON0) & 0xfffffffe), ARMPLL3_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL3_PWR_CON0) | 0x00000002), ARMPLL3_PWR_CON0);
			mt_reg_sync_writel((clk_readl(ARMPLL3_PWR_CON0) & 0xfffffffe), ARMPLL3_PWR_CON0);
		}
	}
}
#endif
static int __init clk_mt6763_init(void)
{
	/*timer_ready = true;*/
	/*mtk_clk_enable_critical();*/

	return 0;
}
arch_initcall(clk_mt6763_init);


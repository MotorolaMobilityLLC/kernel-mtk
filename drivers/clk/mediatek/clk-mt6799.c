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

#include <dt-bindings/clock/mt6799-clk.h>

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

static DEFINE_SPINLOCK(mt6799_clk_lock);

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
#define PERI_CG1 0x07ff00fe
#define PERI_CG2 0x00010000/*flashif[16]*/
#define PERI_CG3 0x00000172/*usb_p1[1], mpcie[8][6:4]*/
#define PERI_CG4 0x1107005b/*mbist_mem_off_dly[8] cannot gate, suspend fail*/

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

static const struct mtk_fixed_clk fixed_clks[] __initconst = {
	FIXED_CLK(CLK_TOP_CLK26M, "f_f26m_ck", "clk26m", 26000000),
};

static const struct mtk_fixed_factor top_divs[] __initconst = {
	FACTOR(CLK_TOP_CLK13M, "clk13m", "clk26m", 1,
		2),
	FACTOR(CLK_TOP_ULPOSC, "ulposc", "clk_null", 1,
		1),
#if 0
	FACTOR(CLK_TOP_DMPLL, "dmpll_ck", "None", None,
		None),
	FACTOR(CLK_TOP_DMPLL_D16, "dmpll_d16", "dmpll_ck", 1,
		16),
	FACTOR(CLK_TOP_DMPLLCH2, "dmpllch2_ck", "None", None,
		None),
	FACTOR(CLK_TOP_DMPLLCH0, "dmpllch0_ck", "None", None,
		None),
#endif
	FACTOR(CLK_TOP_SYSPLL, "syspll_ck", "mainpll", 1,
		1),
	FACTOR(CLK_TOP_SYSPLL_D2, "syspll_d2", "syspll_ck", 1,
		2),
	FACTOR(CLK_TOP_SYSPLL1_D2, "syspll1_d2", "syspll_d2", 1,
		2),
	FACTOR(CLK_TOP_SYSPLL1_D4, "syspll1_d4", "syspll_d2", 1,
		4),
	FACTOR(CLK_TOP_SYSPLL1_D8, "syspll1_d8", "syspll_d2", 1,
		8),
	FACTOR(CLK_TOP_SYSPLL1_D16, "syspll1_d16", "syspll_d2", 1,
		16),
	FACTOR(CLK_TOP_SYSPLL_D3, "syspll_d3", "mainpll", 1,
		3),
	FACTOR(CLK_TOP_SYSPLL2_D2, "syspll2_d2", "syspll_d3", 1,
		2),
	FACTOR(CLK_TOP_SYSPLL2_D4, "syspll2_d4", "syspll_d3", 1,
		4),
	FACTOR(CLK_TOP_SYSPLL2_D8, "syspll2_d8", "syspll_d3", 1,
		8),
	FACTOR(CLK_TOP_SYSPLL2_D3, "syspll2_d3", "syspll_d3", 1,
		3),
	FACTOR(CLK_TOP_SYSPLL_D5, "syspll_d5", "mainpll", 1,
		5),
	FACTOR(CLK_TOP_SYSPLL3_D2, "syspll3_d2", "syspll_d5", 1,
		2),
	FACTOR(CLK_TOP_SYSPLL3_D4, "syspll3_d4", "syspll_d5", 1,
		4),
	FACTOR(CLK_TOP_SYSPLL_D7, "syspll_d7", "mainpll", 1,
		7),
	FACTOR(CLK_TOP_SYSPLL4_D2, "syspll4_d2", "syspll_d7", 1,
		2),
	FACTOR(CLK_TOP_SYSPLL4_D4, "syspll4_d4", "syspll_d7", 1,
		4),
	FACTOR(CLK_TOP_UNIVPLL, "univpll_ck", "univpll", 1,
		1),
	FACTOR(CLK_TOP_UNIVPLL_D7, "univpll_d7", "univpll_ck", 1,
		7),
	FACTOR(CLK_TOP_UNIVPLL_D26, "univpll_d26", "univpll", 1,
		26),
	FACTOR(CLK_TOP_UNIVPLL_D2, "univpll_d2", "univpll", 1,
		2),
	FACTOR(CLK_TOP_UNIVPLL1_D2, "univpll1_d2", "univpll_d2", 1,
		2),
	FACTOR(CLK_TOP_UNIVPLL1_D4, "univpll1_d4", "univpll_d2", 1,
		4),
	FACTOR(CLK_TOP_UNIVPLL1_D8, "univpll1_d8", "univpll_d2", 1,
		8),
	FACTOR(CLK_TOP_UNIVPLL_D3, "univpll_d3", "univpll", 1,
		3),
	FACTOR(CLK_TOP_UNIVPLL2_D2, "univpll2_d2", "univpll_d3", 1,
		2),
	FACTOR(CLK_TOP_UNIVPLL2_D4, "univpll2_d4", "univpll_d3", 1,
		4),
	FACTOR(CLK_TOP_UNIVPLL2_D8, "univpll2_d8", "univpll_d3", 1,
		8),
	FACTOR(CLK_TOP_UNIVPLL_D5, "univpll_d5", "univpll", 1,
		5),
	FACTOR(CLK_TOP_UNIVPLL3_D2, "univpll3_d2", "univpll_d5", 1,
		2),
	FACTOR(CLK_TOP_UNIVPLL3_D4, "univpll3_d4", "univpll_d5", 1,
		4),
	FACTOR(CLK_TOP_APLL1, "apll1_ck", "apll1", 1,
		1),
	FACTOR(CLK_TOP_APLL1_D2, "apll1_d2", "apll1", 1,
		2),
	FACTOR(CLK_TOP_APLL1_D4, "apll1_d4", "apll1", 1,
		4),
	FACTOR(CLK_TOP_APLL1_D8, "apll1_d8", "apll1", 1,
		8),
	FACTOR(CLK_TOP_APLL2, "apll2_ck", "apll2", 1,
		1),
	FACTOR(CLK_TOP_APLL2_D2, "apll2_d2", "apll2", 1,
		2),
	FACTOR(CLK_TOP_APLL2_D4, "apll2_d4", "apll2", 1,
		4),
	FACTOR(CLK_TOP_APLL2_D8, "apll2_d8", "apll2", 1,
		8),
	FACTOR(CLK_TOP_GPUPLL, "gpupll_ck", "gpupll", 1,
		1),
	FACTOR(CLK_TOP_MMPLL, "mmpll_ck", "mmpll", 1,
		1),
	FACTOR(CLK_TOP_MMPLL_D3, "mmpll_d3", "mmpll_ck", 1,
		3),
	FACTOR(CLK_TOP_MMPLL_D4, "mmpll_d4", "mmpll", 1,
		4),
	FACTOR(CLK_TOP_MMPLL_D5, "mmpll_d5", "mmpll", 1,
		5),
	FACTOR(CLK_TOP_MMPLL_D6, "mmpll_d6", "mmpll", 1,
		6),
	FACTOR(CLK_TOP_MMPLL_D8, "mmpll_d8", "mmpll", 1,
		8),
	FACTOR(CLK_TOP_VCODECPLL, "vcodecpll_ck", "vcodecpll", 1,
		1),
	FACTOR(CLK_TOP_VCODECPLL_D4, "vcodecpll_d4", "vcodecpll_ck", 1,
		4),
	FACTOR(CLK_TOP_VCODECPLL_D6, "vcodecpll_d6", "vcodecpll", 1,
		6),
	FACTOR(CLK_TOP_VCODECPLL_D7, "vcodecpll_d7", "vcodecpll", 1,
		7),
	FACTOR(CLK_TOP_FSMIPLL, "fsmipll_ck", "fsmipll", 1,
		1),
	FACTOR(CLK_TOP_FSMIPLL_D3, "fsmipll_d3", "fsmipll_ck", 1,
		3),
	FACTOR(CLK_TOP_FSMIPLL_D4, "fsmipll_d4", "fsmipll", 1,
		4),
	FACTOR(CLK_TOP_FSMIPLL_D5, "fsmipll_d5", "fsmipll", 1,
		5),
	FACTOR(CLK_TOP_VCODECPLLD6, "vcodecplld6_ck", "vcodecpll", 1,
		1),
	FACTOR(CLK_TOP_VCODECPLLD6_D2, "vcodecplld6_d2", "vcodecplld6_ck", 1,
		2),
	FACTOR(CLK_TOP_TVDPLL, "tvdpll_ck", "tvdpll", 1,
		1),
	FACTOR(CLK_TOP_TVDPLL_D2, "tvdpll_d2", "tvdpll_ck", 1,
		2),
	FACTOR(CLK_TOP_TVDPLL_D4, "tvdpll_d4", "tvdpll", 1,
		4),
	FACTOR(CLK_TOP_TVDPLL_D8, "tvdpll_d8", "tvdpll", 1,
		8),
	FACTOR(CLK_TOP_TVDPLL_D16, "tvdpll_d16", "tvdpll", 1,
		16),
	FACTOR(CLK_TOP_EMIPLL, "emipll_ck", "emipll", 1,
		1),
	FACTOR(CLK_TOP_EMIPLL_D2, "emipll_d2", "emipll", 1,
		2),
	FACTOR(CLK_TOP_EMIPLL_D4, "emipll_d4", "emipll", 1,
		4),
	FACTOR(CLK_TOP_MSDCPLL, "msdcpll_ck", "msdcpll", 1,
		1),
	FACTOR(CLK_TOP_MSDCPLL_D2, "msdcpll_d2", "msdcpll", 1,
		2),
	FACTOR(CLK_TOP_MSDCPLL_D4, "msdcpll_d4", "msdcpll", 1,
		4),
	FACTOR(CLK_TOP_ULPOSCPLL, "ulposcpll_ck", "ulposc", 1,
		1),
	FACTOR(CLK_TOP_ULPOSCPLL_D2, "ulposcpll_d2", "ulposc", 1,
		2),
	FACTOR(CLK_TOP_ULPOSCPLL_D8, "ulposcpll_d8", "ulposc", 1,
		8),
	FACTOR(CLK_TOP_F_FRTC, "f_frtc_ck", "clk32k", 1,
		1),
};

static const char * const axi_parents[] __initconst = {
	"clk26m",
	"syspll_d5",
	"syspll1_d4",
	"syspll2_d2",
	"syspll2_d3",
	"univpll2_d2",
	"syspll3_d2",
	"syspll_d7"
};

#if 0
static const char * const mem_parents[] __initconst = {
	"clk26m",
	"emipll_ck",
	"dmpllch0_ck",
	"dmpllch2_ck"
};
#endif
static const char * const ddrphycfg_parents[] __initconst = {
	"clk26m",
	"syspll1_d8"
};

static const char * const mm_parents[] __initconst = {
	"clk26m",
	"vcodecpll_d6",
	"mmpll_d5",
	"vcodecpll_d7",
	"mmpll_d6",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"syspll_d5",
	"syspll1_d4"
};

static const char * const sflash_parents[] __initconst = {
	"clk13m",
	"clk26m",
	"syspll2_d8",
	"syspll3_d4",
	"univpll3_d4",
	"syspll4_d2",
	"syspll2_d4",
	"univpll2_d4"
};

static const char * const pwm_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"univpll3_d4"
};

static const char * const disppwm_parents[] __initconst = {
	"clk26m",
	"ulposcpll_d2",
	"ulposcpll_d8",
	"univpll1_d4"
};

static const char * const vdec_parents[] __initconst = {
	"clk26m",
	"syspll_d5",
	"vcodecpll_d6",
	"vcodecpll_d7",
	"mmpll_d5",
	"mmpll_d6",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"univpll3_d2",
	"univpll_d5",
	"univpll2_d2",
	"syspll2_d2",
	"univpll1_d4",
	"univpll2_d4",
	"syspll4_d2"
};

static const char * const venc_parents[] __initconst = {
	"clk26m",
	"vcodecpll_d6",
	"univpll_d5",
	"univpll_d3",
	"vcodecpll_d7",
	"syspll_d3",
	"univpll1_d2",
	"tvdpll_d2",
	"mmpll_d8",
	"syspll1_d2",
	"mmpll_d5",
	"mmpll_d6",
	"univpll2_d2",
	"syspll1_d4"
};

static const char * const mfg_parents[] __initconst = {
	"clk26m",
	"gpupll_ck",
	"syspll_d2"
};

static const char * const camtg_parents[] __initconst = {
	"clk26m",
	"univpll_d26",
	"univpll2_d2",
	"syspll3_d2",
	"syspll3_d4",
	"univpll1_d4",
	"syspll1_d8"
};

static const char * const i2c_parents[] __initconst = {
	"clk26m",
	"univpll3_d4"
};

static const char * const uart_parents[] __initconst = {
	"clk26m",
	"univpll2_d8"
};

static const char * const spi_parents[] __initconst = {
	"clk26m",
	"syspll3_d2",
	"syspll1_d4",
	"syspll4_d2",
	"univpll3_d2",
	"univpll2_d4",
	"univpll1_d8"
};

static const char * const axi_peri_parents[] __initconst = {
	"clk26m",
	"syspll_d5",
	"syspll2_d2",
	"syspll1_d4",
	"univpll_d5",
	"univpll2_d2",
	"syspll1_d2",
	"syspll_d7"
};

static const char * const usb20_parents[] __initconst = {
	"clk26m",
	"univpll1_d8",
	"univpll3_d4"
};

static const char * const usb30_p0_parents[] __initconst = {
	"clk26m",
	"univpll3_d2",
	"univpll2_d4"
};

static const char * const msdc50_hsel_parents[] __initconst = {
	"clk26m",
	"univpll_d5",
	"syspll2_d2",
	"syspll4_d2",
	"syspll1_d2",
	"univpll1_d4"
};

static const char * const msdc50_0_parents[] __initconst = {
	"clk26m",
	"msdcpll_d2",
	"msdcpll_ck",
	"univpll1_d4",
	"syspll2_d2",
	"syspll_d3",
	"msdcpll_d4",
	"univpll1_d2"
};

static const char * const msdc30_1_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"msdcpll_d4",
	"univpll1_d4",
	"syspll2_d2",
	"syspll_d7",
	"univpll_d7",
	"mmpll_d5"
};

static const char * const i3c_parents[] __initconst = {
	"clk26m",
	"univpll3_d2",
	"univpll3_d4"
};

static const char * const msdc30_3_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"msdcpll_d2",
	"univpll1_d4",
	"syspll2_d2",
	"univpll1_d2",
	"syspll_d3",
	"msdcpll_d4"
};

static const char * const msdc53_hsel_parents[] __initconst = {
	"clk26m",
	"syspll2_d2",
	"syspll4_d2",
	"univpll1_d4"
};

static const char * const smi0_2x_parents[] __initconst = {
	"clk26m",
	"fsmipll_d3",
	"fsmipll_d4",
	"fsmipll_d5",
	"vcodecpll_d4",
	"mmpll_d3",
	"emipll_ck",
	"syspll_d2"
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
	"syspll4_d2",
	"univpll3_d2",
	"univpll2_d8",
	"syspll1_d4"
};

static const char * const pmicspi_parents[] __initconst = {
	"clk26m",
	"syspll1_d8",
	"syspll3_d4",
	"syspll1_d16",
	"univpll3_d4",
	"univpll_d26",
	"syspll1_d8",
	"syspll1_d16"
};

static const char * const scp_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"univpll1_d2",
	"syspll_d3",
	"univpll_d3",
	"vcodecpll_d7",
	"univpll2_d2",
	"univpll2_d4"
};

static const char * const atb_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll1_d2"
};

static const char * const mjc_parents[] __initconst = {
	"clk26m",
	"univpll_d3",
	"syspll_d5",
	"univpll1_d2",
	"vcodecpll_d7",
	"ulposcpll_ck"
};

static const char * const dpi0_parents[] __initconst = {
	"clk26m",
	"tvdpll_d2",
	"tvdpll_d4",
	"tvdpll_d8",
	"tvdpll_d16"
};

static const char * const dsp_parents[] __initconst = {
	"clk26m",
	"syspll_d2",
	"vcodecpll_d4",
	"vcodecpll_d6",
	"vcodecpll_d7",
	"mmpll_d4",
	"mmpll_d5",
	"mmpll_d6",
	"syspll_d3",
	"univpll_d2",
	"univpll1_d2",
	"syspll1_d2",
	"syspll1_d4",
	"vcodecplld6_d2",
	"fsmipll_d4"
};

static const char * const aud_1_parents[] __initconst = {
	"clk26m",
	"apll1_ck"
};

static const char * const aud_2_parents[] __initconst = {
	"clk26m",
	"apll2_ck"
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

static const char * const dfp_mfg_parents[] __initconst = {
	"clk26m",
	"univpll2_d2",
	"univpll2_d4",
	"univpll2_d8"
};

static const char * const cam_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"vcodecpll_d6",
	"vcodecpll_d7",
	"mmpll_d5",
	"mmpll_d6",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"vcodecplld6_d2"
};

static const char * const smi1_2x_parents[] __initconst = {
	"clk26m",
	"fsmipll_d3",
	"fsmipll_d4",
	"fsmipll_d5",
	"vcodecpll_d4",
	"mmpll_d3",
	"emipll_ck",
	"syspll_d2"
};

static const char * const axi_mfg_in_parents[] __initconst = {
	"clk26m",
	"axi_ck",
	"univpll2_d8"
};

static const char * const img_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"vcodecpll_d6",
	"vcodecpll_d7",
	"mmpll_d5",
	"mmpll_d6",
	"syspll_d3",
	"univpll1_d2",
	"syspll1_d2",
	"vcodecplld6_d2"
};

static const char * const ufo_enc_parents[] __initconst = {
	"clk26m",
	"univpll_d2",
	"mmpll_d4",
	"syspll_d2",
	"mmpll_d5",
	"univpll1_d2",
	"syspll1_d2",
	"syspll_d5"
};

static const char * const pcie_mac_parents[] __initconst = {
	"clk26m",
	"syspll3_d2",
	"univpll2_d4",
	"syspll2_d4",
	"syspll4_d2"
};
#if 0
static const char * const emi_parents[] __initconst = {
	"clk26m",
	"emipll_ck",
	"dmpllch0_ck",
	"dmpllch2_ck",
	"dmpll_ck",
	"dmpll_d16",
	"emipll_d2",
	"emipll_d4"
};
#endif
static const char * const aes_ufsfde_parents[] __initconst = {
	"clk26m",
	"syspll_d2",
	"syspll1_d2",
	"syspll_d3",
	"syspll1_d4",
	"emipll_d2",
	"univpll_d3"
};

static const char * const aes_fde_parents[] __initconst = {
	"clk26m",
	"syspll_d2",
	"syspll_d3",
	"emipll_d2",
	"univpll_d3"
};

static const char * const audio_h_parents[] __initconst = {
	"clk26m",
	"univpll_d7",
	"apll1_ck",
	"apll2_ck"
};

static const char * const sspm_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"univpll_d5",
	"syspll_d5",
	"univpll1_d2",
	"syspll_d3",
	"univpll_d3",
	"syspll2_d2"
};

static const char * const ancmd32_parents[] __initconst = {
	"clk26m",
	"msdcpll_d2",
	"vcodecpll_d7",
	"syspll1_d2",
	"emipll_d4",
	"syspll_d5",
	"msdcpll_d4",
	"univpll1_d4"
};

static const char * const slow_mfg_parents[] __initconst = {
	"clk26m",
	"univpll2_d4",
	"univpll2_d8"
};

static const char * const ufs_card_parents[] __initconst = {
	"clk26m",
	"syspll1_d4",
	"syspll1_d8",
	"syspll1_d16"
};

static const char * const bsi_spi_parents[] __initconst = {
	"clk26m",
	"syspll2_d3",
	"syspll1_d4",
	"syspll_d7"
};

static const char * const dxcc_parents[] __initconst = {
	"clk26m",
	"syspll1_d2",
	"syspll1_d4",
	"syspll1_d8"
};

static const char * const seninf_parents[] __initconst = {
	"clk26m",
	"univpll1_d2",
	"univpll2_d2",
	"univpll1_d4"
};

static const char * const dfp_parents[] __initconst = {
	"clk26m",
	"univpll2_d8"
};

static const struct mtk_composite top_muxes[] __initconst = {
	/* CLK_CFG_0 */
	MUX(CLK_TOP_AXI_SEL, "axi_sel", axi_parents,
		0x100, 0, 3),
#if 0
	MUX(CLK_TOP_MEM_SEL, "mem_sel", mem_parents,
		0x100, 8, 2),
#endif
	MUX(CLK_TOP_DDRPHYCFG_SEL, "ddrphycfg_sel", ddrphycfg_parents,
		0x100, 16, 1),
	MUX(CLK_TOP_MM_SEL, "mm_sel", mm_parents,
		0x100, 24, 4),
	/* CLK_CFG_1 */
	MUX(CLK_TOP_SFLASH_SEL, "sflash_sel", sflash_parents,
		0x110, 0, 3),
	MUX(CLK_TOP_PWM_SEL, "pwm_sel", pwm_parents,
		0x110, 8, 2),
	MUX(CLK_TOP_DISPPWM_SEL, "disppwm_sel", disppwm_parents,
		0x110, 16, 2),
	MUX(CLK_TOP_VDEC_SEL, "vdec_sel", vdec_parents,
		0x110, 24, 4),
	/* CLK_CFG_2 */
	MUX(CLK_TOP_VENC_SEL, "venc_sel", venc_parents,
		0x120, 0, 4),
	MUX(CLK_TOP_MFG_SEL, "mfg_sel", mfg_parents,
		0x120, 8, 2),
	MUX(CLK_TOP_CAMTG_SEL, "camtg_sel", camtg_parents,
		0x120, 16, 3),
	MUX(CLK_TOP_I2C_SEL, "i2c_sel", i2c_parents,
		0x120, 24, 1),
	/* CLK_CFG_3 */
	MUX(CLK_TOP_UART_SEL, "uart_sel", uart_parents,
		0x130, 0, 1),
	MUX(CLK_TOP_SPI_SEL, "spi_sel", spi_parents,
		0x130, 8, 3),
	MUX(CLK_TOP_AXI_PERI_SEL, "axi_peri_sel", axi_peri_parents,
		0x130, 16, 3),
	MUX(CLK_TOP_USB20_SEL, "usb20_sel", usb20_parents,
		0x130, 24, 2),
	/* CLK_CFG_4 */
	MUX(CLK_TOP_USB30_P0_SEL, "usb30_p0_sel", usb30_p0_parents,
		0x140, 0, 2),
	MUX(CLK_TOP_MSDC50_HCLK_SEL, "msdc50_hsel_sel", msdc50_hsel_parents,
		0x140, 8, 3),
	MUX(CLK_TOP_MSDC50_0_SEL, "msdc50_0_sel", msdc50_0_parents,
		0x140, 16, 3),
	MUX(CLK_TOP_MSDC30_1_SEL, "msdc30_1_sel", msdc30_1_parents,
		0x140, 24, 3),
	/* CLK_CFG_5 */
	MUX(CLK_TOP_I3C_SEL, "i3c_sel", i3c_parents,
		0x150, 0, 2),
	MUX(CLK_TOP_MSDC30_3_SEL, "msdc30_3_sel", msdc30_3_parents,
		0x150, 8, 3),
	MUX(CLK_TOP_MSDC53_HCLK_SEL, "msdc53_hsel_sel", msdc53_hsel_parents,
		0x150, 16, 2),
	MUX(CLK_TOP_SMI0_2X_SEL, "smi0_2x_sel", smi0_2x_parents,
		0x150, 24, 3),
	/* CLK_CFG_6 */
	MUX(CLK_TOP_AUDIO_SEL, "audio_sel", audio_parents,
		0x160, 0, 2),
	MUX(CLK_TOP_AUD_INTBUS_SEL, "aud_intbus_sel", aud_intbus_parents,
		0x160, 8, 3),
	MUX(CLK_TOP_PMICSPI_SEL, "pmicspi_sel", pmicspi_parents,
		0x160, 16, 3),
	MUX(CLK_TOP_SCP_SEL, "scp_sel", scp_parents,
		0x160, 24, 3),
	/* CLK_CFG_7 */
	MUX(CLK_TOP_ATB_SEL, "atb_sel", atb_parents,
		0x170, 0, 2),
	MUX(CLK_TOP_MJC_SEL, "mjc_sel", mjc_parents,
		0x170, 8, 3),
	MUX(CLK_TOP_DPI0_SEL, "dpi0_sel", dpi0_parents,
		0x170, 16, 3),
	MUX(CLK_TOP_DSP_SEL, "dsp_sel", dsp_parents,
		0x170, 24, 4),
	/* CLK_CFG_8 */
	MUX(CLK_TOP_AUD_1_SEL, "aud_1_sel", aud_1_parents,
		0x180, 0, 1),
	MUX(CLK_TOP_AUD_2_SEL, "aud_2_sel", aud_2_parents,
		0x180, 8, 1),
	MUX(CLK_TOP_AUD_ENGEN1_SEL, "aud_engen1_sel", aud_engen1_parents,
		0x180, 16, 2),
	MUX(CLK_TOP_AUD_ENGEN2_SEL, "aud_engen2_sel", aud_engen2_parents,
		0x180, 24, 2),
	/* CLK_CFG_9 */
	MUX(CLK_TOP_DFP_MFG_SEL, "dfp_mfg_sel", dfp_mfg_parents,
		0x190, 0, 2),
	MUX(CLK_TOP_CAM_SEL, "cam_sel", cam_parents,
		0x190, 8, 4),
	MUX(CLK_TOP_IPU_IF_SEL, "ipu_if_sel", cam_parents,
		0x190, 16, 4),
	MUX(CLK_TOP_SMI1_2X_SEL, "smi1_2x_sel", smi1_2x_parents,
		0x190, 24, 3),
	/* CLK_CFG_10 */
	MUX(CLK_TOP_AXI_MFG_IN_SEL, "axi_mfg_in_sel", axi_mfg_in_parents,
		0x1a0, 0, 2),
	MUX(CLK_TOP_IMG_SEL, "img_sel", img_parents,
		0x1a0, 8, 4),
	MUX(CLK_TOP_UFO_ENC_SEL, "ufo_enc_sel", ufo_enc_parents,
		0x1a0, 16, 3),
	MUX(CLK_TOP_UFO_DEC_SEL, "ufo_dec_sel", ufo_enc_parents,
		0x1a0, 24, 3),
	/* CLK_CFG_11 */
	MUX(CLK_TOP_PCIE_MAC_SEL, "pcie_mac_sel", pcie_mac_parents,
		0x1b0, 0, 3),
#if 0
	MUX(CLK_TOP_EMI_SEL, "emi_sel", emi_parents,
		0x1b0, 8, 3),
#endif
	MUX(CLK_TOP_AES_UFSFDE_SEL, "aes_ufsfde_sel", aes_ufsfde_parents,
		0x1b0, 16, 3),
	MUX(CLK_TOP_AES_FDE_SEL, "aes_fde_sel", aes_fde_parents,
		0x1b0, 24, 3),
	/* CLK_CFG_12 */
	MUX(CLK_TOP_AUDIO_H_SEL, "audio_h_sel", audio_h_parents,
		0x1c0, 0, 2),
	MUX(CLK_TOP_SSPM_SEL, "sspm_sel", sspm_parents,
		0x1c0, 8, 3),
	MUX(CLK_TOP_ANCMD32_SEL, "ancmd32_sel", ancmd32_parents,
		0x1c0, 16, 3),
	MUX(CLK_TOP_SLOW_MFG_SEL, "slow_mfg_sel", slow_mfg_parents,
		0x1c0, 24, 2),
	/* CLK_CFG_13 */
	MUX(CLK_TOP_UFS_CARD_SEL, "ufs_card_sel", ufs_card_parents,
		0x1d0, 0, 2),
	MUX(CLK_TOP_BSI_SPI_SEL, "bsi_spi_sel", bsi_spi_parents,
		0x1d0, 8, 2),
	MUX(CLK_TOP_DXCC_SEL, "dxcc_sel", dxcc_parents,
		0x1d0, 16, 2),
	MUX(CLK_TOP_SENINF_SEL, "seninf_sel", seninf_parents,
		0x1d0, 24, 2),
	/* CLK_CFG_14 */
	MUX(CLK_TOP_DFP_SEL, "dfp_sel", dfp_parents,
		0x1e0, 0, 1),
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
	.set_ofs = 0xb0,
	.clr_ofs = 0xb4,
	.sta_ofs = 0xb8,
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

static const struct mtk_gate infra_clks[] __initconst = {
	/* INFRA0 */
	GATE_INFRA0(CLK_INFRA_PMIC_TMR, "infra_pmic_tmr",
		"f_f26m_ck", 0),
	GATE_INFRA0(CLK_INFRA_PMIC_AP, "infra_pmic_ap",
		"f_f26m_ck", 1),
	GATE_INFRA0(CLK_INFRA_PMIC_MD, "infra_pmic_md",
		"f_f26m_ck", 2),
	GATE_INFRA0(CLK_INFRA_PMIC_CONN, "infra_pmic_conn",
		"f_f26m_ck", 3),
	GATE_INFRA0(CLK_INFRA_SCP, "infra_scp",
		"axi_ck", 4),
	GATE_INFRA0(CLK_INFRA_SEJ, "infra_sej",
		"axi_sel", 5),
	GATE_INFRA0(CLK_INFRA_APXGPT, "infra_apxgpt",
		"f_f26m_ck", 6),
	GATE_INFRA0(CLK_INFRA_DVFSRC, "infra_dvfsrc",
		"f_f26m_ck", 7),
	GATE_INFRA0(CLK_INFRA_GCE, "infra_gce",
		"axi_sel", 8),
	GATE_INFRA0(CLK_INFRA_DBG_BCLK, "infra_dbg_bclk",
		"axi_sel", 9),
	GATE_INFRA0(CLK_INFRA_SSHUB_APB_ASYNC, "infra_sshub_asy",
		"f_f26m_ck", 10),
	GATE_INFRA0(CLK_INFRA_SPM_APB_ASYNC, "infra_spm_asy",
		"f_f26m_ck", 12),
	GATE_INFRA0(CLK_INFRA_CLDMA_AP_TOP, "infra_cldma_ap",
		"axi_sel", 13),
	GATE_INFRA0(CLK_INFRA_CCIF3_AP, "infra_ccif3_ap",
		"axi_sel", 17),
	GATE_INFRA0(CLK_INFRA_AES_TOP0, "infra_aes_top0",
		"axi_sel", 18),
	GATE_INFRA0(CLK_INFRA_AES_TOP1, "infra_aes_top1",
		"axi_sel", 19),
	GATE_INFRA0(CLK_INFRA_DEVAPC_MPU, "infra_devapcmpu",
		"axi_sel", 20),
	GATE_INFRA0(CLK_INFRA_RESERVED_FOR_CCIF4_AP, "infra_ccif4_ap",
		"axi_sel", 21),
	GATE_INFRA0(CLK_INFRA_RESERVED_FOR_CCIF4_MD, "infra_ccif4_md",
		"axi_sel", 22),
	GATE_INFRA0(CLK_INFRA_CCIF3_MD, "infra_ccif3_md",
		"axi_sel", 24),
	GATE_INFRA0(CLK_INFRA_RESERVED_FOR_CCIF5_AP, "infra_ccif5_ap",
		"axi_sel", 25),
	GATE_INFRA0(CLK_INFRA_RESERVED_FOR_CCIF5_MD, "infra_ccif5_md",
		"axi_sel", 26),
	GATE_INFRA0(CLK_INFRA_MD2MD_CCIF_SET_0, "infra_md2md_0",
		"axi_sel", 27),
	GATE_INFRA0(CLK_INFRA_MD2MD_CCIF_SET_1_AND_MODEM_TEMP_SHARE, "infra_md2md_1",
		"f_f26m_ck", 28),
	GATE_INFRA0(CLK_INFRA_MD2MD_CCIF_SET_2, "infra_md2md_2",
		"axi_ck", 29),
	GATE_INFRA0(CLK_INFRA_FHCTL, "infra_fhctl",
		"f_f26m_ck", 30),
	GATE_INFRA0(CLK_INFRA_RESERVED_FOR_MODEM_TEMP_SHARE, "infra_md_tmp_sr",
		"f_f26m_ck", 31),
	/* INFRA1 */
	GATE_INFRA1(CLK_INFRA_MD2MD_CCIF_MD_SET_3, "infra_md2md_3",
		"axi_sel", 0),
	GATE_INFRA1(CLK_INFRA_MD2MD_CCIF_MD_SET_4, "infra_md2md_4",
		"axi_sel", 3),
	GATE_INFRA1(CLK_INFRA_AUDIO_DCM_EN, "infra_aud_dcm",
		"axi_sel", 5),
	GATE_INFRA1(CLK_INFRA_CLDMA_AO_TOP_HCLK, "infra_cldma_hck",
		"axi_sel", 6),
	GATE_INFRA1(CLK_INFRA_MD2MD_CCIF_MD_SET_5, "infra_md2md_5",
		"axi_sel", 7),
	GATE_INFRA1(CLK_INFRA_TRNG, "infra_trng",
		"axi_sel", 9),
	GATE_INFRA1(CLK_INFRA_AUXADC, "infra_auxadc",
		"f_f26m_ck", 10),
	GATE_INFRA1(CLK_INFRA_CPUM, "infra_cpum",
		"axi_sel", 11),
	GATE_INFRA1(CLK_INFRA_CCIF1_AP, "infra_ccif1_ap",
		"axi_sel", 12),
	GATE_INFRA1(CLK_INFRA_CCIF1_MD, "infra_ccif1_md",
		"axi_sel", 13),
	GATE_INFRA1(CLK_INFRA_CCIF2_AP, "infra_ccif2_ap",
		"axi_sel", 16),
	GATE_INFRA1(CLK_INFRA_CCIF2_MD, "infra_ccif2_md",
		"axi_sel", 17),
	GATE_INFRA1(CLK_INFRA_XIU_CKCG_SET_FOR_DBG_CTRLER, "infra_xiu_ckcg",
		"f_f26m_ck", 19),
	GATE_INFRA1(CLK_INFRA_DEVICE_APC, "infra_devapc",
		"axi_sel", 20),
	GATE_INFRA1(CLK_INFRA_SMI_L2C, "infra_smi_l2c",
		"mm_ck", 22),
	GATE_INFRA1(CLK_INFRA_CCIF0_AP, "infra_ccif0_ap",
		"axi_sel", 23),
	GATE_INFRA1(CLK_INFRA_AUDIO, "infra_audio",
		"axi_sel", 25),
	GATE_INFRA1(CLK_INFRA_CCIF0_MD, "infra_ccif0_md",
		"axi_sel", 26),
	GATE_INFRA1(CLK_INFRA_RESERVED_FOR_EMI_L2C_FT, "infra_emil2cft",
		"mem_ck", 27),
	GATE_INFRA1(CLK_INFRA_RESERVED_FOR_SPM_PCM, "infra_spm_pcm",
		"f_f26m_ck", 28),
	GATE_INFRA1(CLK_INFRA_EMI_L2C_FT_SET_AND_ANC_MD32_AND_SPM_PCM, "infra_emi_md32",
		"f_f26m_ck", 29),
	GATE_INFRA1(CLK_INFRA_ANC_MD32, "infra_anc_md32",
		"ancmd32_ck", 30),
	GATE_INFRA1(CLK_INFRA_DRAMC_F26M, "infra_dramc_26m",
		"f_f26m_ck", 31),
	/* INFRA2 */
	GATE_INFRA2(CLK_INFRA_THERM_BCLK, "infra_therm_bck",
		"axi_sel", 6),
	GATE_INFRA2(CLK_INFRA_PTP_BCLK, "infra_ptp_bclk",
		"axi_sel", 7),
	GATE_INFRA2(CLK_INFRA_AUXADC_MD, "infra_auxadc_md",
		"f_f26m_ck", 24),
	GATE_INFRA2(CLK_INFRA_DVFS_CTRL_APB_RX, "infra_dvfsctlrx",
		"f_f26m_ck", 30),
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
	GATE_MFG_CFG(CLK_MFG_CFG_BAXI, "mfg_cfg_baxi", "axi_mfg_in_sel", 0),
	GATE_MFG_CFG(CLK_MFG_CFG_BMEM, "mfg_cfg_bmem", "mem_sel", 1),
	GATE_MFG_CFG(CLK_MFG_CFG_BG3D, "mfg_cfg_bg3d", "mfg_sel", 2),
	GATE_MFG_CFG(CLK_MFG_CFG_B26M, "mfg_cfg_b26m", "f_f26m_ck", 3),
};

static const struct mtk_gate_regs pericfg0_cg_regs = {
	.set_ofs = 0x270,
	.clr_ofs = 0x274,
	.sta_ofs = 0x278,
};

static const struct mtk_gate_regs pericfg1_cg_regs = {
	.set_ofs = 0x280,
	.clr_ofs = 0x284,
	.sta_ofs = 0x288,
};

static const struct mtk_gate_regs pericfg2_cg_regs = {
	.set_ofs = 0x290,
	.clr_ofs = 0x294,
	.sta_ofs = 0x298,
};

static const struct mtk_gate_regs pericfg3_cg_regs = {
	.set_ofs = 0x2a0,
	.clr_ofs = 0x2a4,
	.sta_ofs = 0x2a8,
};

static const struct mtk_gate_regs pericfg4_cg_regs = {
	.set_ofs = 0x2b0,
	.clr_ofs = 0x2b4,
	.sta_ofs = 0x2b8,
};

static const struct mtk_gate_regs pericfg5_cg_regs = {
	.set_ofs = 0x2c0,
	.clr_ofs = 0x2c4,
	.sta_ofs = 0x2c8,
};

#define GATE_PERICFG0(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg0_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERICFG1(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg1_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERICFG2(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg2_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERICFG3(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg3_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}


#define GATE_PERICFG4(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg4_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

#define GATE_PERICFG5(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &pericfg5_cg_regs,		\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate pericfg_clks[] __initconst = {
	/* PERICFG0 */
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_BCLK, "peri_pwm_bclk",
		"pwm_sel", 0),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK1, "peri_pwm_fbclk1",
		"pwm_sel", 1),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK2, "peri_pwm_fbclk2",
		"pwm_sel", 2),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK3, "peri_pwm_fbclk3",
		"pwm_sel", 3),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK4, "peri_pwm_fbclk4",
		"pwm_sel", 4),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK5, "peri_pwm_fbclk5",
		"pwm_sel", 5),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK6, "peri_pwm_fbclk6",
		"pwm_sel", 6),
	GATE_PERICFG0(CLK_PERICFG_RG_PWM_FBCLK7, "peri_pwm_fbclk7",
		"pwm_sel", 7),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C0_BCLK, "peri_i2c0_bclk",
		"i2c_sel", 16),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C1_BCLK, "peri_i2c1_bclk",
		"i2c_sel", 17),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C2_BCLK, "peri_i2c2_bclk",
		"i2c_sel", 18),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C3_BCLK, "peri_i2c3_bclk",
		"i2c_sel", 19),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C4_BCLK, "peri_i2c4_bclk",
		"i2c_sel", 20),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C5_BCLK, "peri_i2c5_bclk",
		"i2c_sel", 21),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C6_BCLK, "peri_i2c6_bclk",
		"i2c_sel", 22),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C7_BCLK, "peri_i2c7_bclk",
		"i2c_sel", 23),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C8_BCLK, "peri_i2c8_bclk",
		"i2c_sel", 24),
	GATE_PERICFG0(CLK_PERICFG_RG_I2C9_BCLK, "peri_i2c9_bclk",
		"i2c_sel", 25),
	GATE_PERICFG0(CLK_PERICFG_RG_IDVFS, "peri_idvfs",
		"i2c_sel", 31),
	/* PERICFG1 */
	GATE_PERICFG1(CLK_PERICFG_RG_UART0, "peri_uart0",
		"uart_sel", 0),
	GATE_PERICFG1(CLK_PERICFG_RG_UART1, "peri_uart1",
		"uart_sel", 1),
	GATE_PERICFG1(CLK_PERICFG_RG_UART2, "peri_uart2",
		"uart_sel", 2),
	GATE_PERICFG1(CLK_PERICFG_RG_UART3, "peri_uart3",
		"uart_sel", 3),
	GATE_PERICFG1(CLK_PERICFG_RG_UART4, "peri_uart4",
		"uart_sel", 4),
	GATE_PERICFG1(CLK_PERICFG_RG_UART5, "peri_uart5",
		"uart_sel", 5),
	GATE_PERICFG1(CLK_PERICFG_RG_UART6, "peri_uart6",
		"uart_sel", 6),
	GATE_PERICFG1(CLK_PERICFG_RG_UART7, "peri_uart7",
		"uart_sel", 7),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI0, "pericfg_rg_spi0",
		"spi_sel", 16),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI1, "pericfg_rg_spi1",
		"spi_sel", 17),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI2, "pericfg_rg_spi2",
		"spi_sel", 18),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI3, "pericfg_rg_spi3",
		"spi_sel", 19),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI4, "pericfg_rg_spi4",
		"spi_sel", 20),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI5, "pericfg_rg_spi5",
		"spi_sel", 21),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI6, "pericfg_rg_spi6",
		"spi_sel", 22),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI7, "pericfg_rg_spi7",
		"spi_sel", 23),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI8, "pericfg_rg_spi8",
		"spi_sel", 24),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI9, "pericfg_rg_spi9",
		"spi_sel", 25),
	GATE_PERICFG1(CLK_PERICFG_RG_SPI10, "peri_spi10",
		"spi_sel", 26),
	/* PERICFG2 */
	GATE_PERICFG2(CLK_PERICFG_RG_MSDC0_AP_NORM, "peri_msdc0",
		"msdc50_0_sel", 0),
	GATE_PERICFG2(CLK_PERICFG_RG_MSDC1, "peri_msdc1",
		"msdc30_1_sel", 1),
	GATE_PERICFG2(CLK_PERICFG_RG_MSDC3, "peri_msdc3",
		"msdc30_3_sel", 3),
	GATE_PERICFG2(CLK_PERICFG_RG_UFSCARD, "peri_ufscard",
		"ufs_card_sel", 10),
	GATE_PERICFG2(CLK_PERICFG_RG_UFSCARD_MP_SAP_CFG, "peri_ufscard_mp",
		"clk_null", 11),
	GATE_PERICFG2(CLK_PERICFG_RG_UFS_AES_CORE, "peri_ufs_aes",
		"aes_ufsfde_sel", 12),
	GATE_PERICFG2(CLK_PERICFG_RG_FLASHIF, "peri_flashif",
		"sflash_sel", 16),
	GATE_PERICFG2(CLK_PERICFG_RG_MSDC0_AP_SECURE, "peri_msdc0_ap",
		"msdc50_hsel_sel", 24),
	GATE_PERICFG2(CLK_PERICFG_RG_MSDC0_MD_SECURE, "peri_msdc0_md",
		"msdc50_hsel_sel", 25),
	/* PERICFG3 */
	GATE_PERICFG3(CLK_PERICFG_RG_USB_P0, "peri_usb_p0",
		"usb30_p0_sel", 0),
	GATE_PERICFG3(CLK_PERICFG_RG_USB_P1, "peri_usb_p1",
		"usb20_sel", 1),
	GATE_PERICFG3(CLK_PERICFG_RG_MPCIE_P0, "peri_pcie_p0",
		"pcie_mac_sel", 4),
	GATE_PERICFG3(CLK_PERICFG_RG_MPCIE_P0_OBFF_CLK, "peri_pcie_p0obf",
		"f_f26m_ck", 5),
	GATE_PERICFG3(CLK_PERICFG_RG_MPCIE_P0_AUX_CLK, "peri_pcie_p0aux",
		"f_f26m_ck", 6),
	GATE_PERICFG3(CLK_PERICFG_RG_MPCIE_P1, "peri_pcie_p1",
		"axi_peri_sel", 8),
	/* PERICFG4 */
	GATE_PERICFG4(CLK_PERICFG_RG_AP_DM, "peri_ap_dm",
		"axi_peri_sel", 0),
	GATE_PERICFG4(CLK_PERICFG_RG_IRTX, "pericfg_rg_irtx",
		"f_f26m_ck", 1),
	GATE_PERICFG4(CLK_PERICFG_RG_DISP_PWM0, "peri_disp_pwm0",
		"disppwm_sel", 3),
	GATE_PERICFG4(CLK_PERICFG_RG_DISP_PWM1, "peri_disp_pwm1",
		"disppwm_sel", 4),
	GATE_PERICFG4(CLK_PERICFG_RG_CQ_DMA, "peri_cq_dma",
		"axi_peri_sel", 6),
	GATE_PERICFG4(CLK_PERICFG_RG_MBIST_MEM_OFF_DLY, "peri_mbist_off",
		"f_f26m_ck", 8),
	GATE_PERICFG4(CLK_PERICFG_RG_DEVICE_APC_PERI, "peri_devapc",
		"axi_peri_sel", 9),
	GATE_PERICFG4(CLK_PERICFG_RG_GCPU_BIU, "peri_gcpu_biu",
		"axi_peri_sel", 10),
	GATE_PERICFG4(CLK_PERICFG_RG_DXCC_AO, "peri_dxcc_ao",
		"f_frtc_ck", 16),
	GATE_PERICFG4(CLK_PERICFG_RG_DXCC_PUB, "peri_dxcc_pub",
		"dxcc_sel", 17),
	GATE_PERICFG4(CLK_PERICFG_RG_DXCC_SEC, "peri_dxcc_sec",
		"dxcc_sel", 18),
	GATE_PERICFG4(CLK_PERICFG_RG_DEBUGTOP, "peri_debugtop",
		"axi_peri_sel", 20),
	GATE_PERICFG4(CLK_PERICFG_RG_DXFDE_CORE, "peri_dxfde_core",
		"aes_fde_sel", 24),
	GATE_PERICFG4(CLK_PERICFG_RG_UFOZIP, "peri_ufozip",
		"ufo_enc_sel", 28),
	/* PERICFG5 */
	GATE_PERICFG5(CLK_PERICFG_RG_PERIAPB_PERICFG_REG, "peri_apb_peri",
		"axi_peri_sel", 0),
	GATE_PERICFG5(CLK_PERICFG_RG_PERIAPB_SMI_4X1_INST, "peri_apb_smi",
		"axi_peri_sel", 1),
	GATE_PERICFG5(CLK_PERICFG_RG_PERIAPB_POWER_METER, "peri_apb_pmeter",
		"clk_null", 2),
	GATE_PERICFG5(CLK_PERICFG_RG_PERIAPB_VAD, "peri_apb_vad",
		"clk_null", 3),
};

static const struct mtk_gate_regs vdec0_cg_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x4,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs vdec1_cg_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0xc,
	.sta_ofs = 0x8,
};

#define GATE_VDEC0_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec0_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

#define GATE_VDEC1_I(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &vdec1_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr_inv,	\
	}

static const struct mtk_gate vdec_clks[] __initconst = {
	/* VDEC0 */
	GATE_VDEC0_I(CLK_VDEC_CKEN, "vdec_cken", "vdec_sel", 0),
	/* VDEC1 */
	GATE_VDEC1_I(CLK_VDEC_LARB1_CKEN, "vdec_larb1_cken", "vdec_sel", 0),
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
	GATE_AUDIO0(CLK_AUDIO_AFE, "aud_afe", "audio_sel",
		2),
	GATE_AUDIO0(CLK_AUDIO_22M, "aud_22m", "aud_engen1_sel",
		8),
	GATE_AUDIO0(CLK_AUDIO_24M, "aud_24m", "aud_engen2_sel",
		9),
	GATE_AUDIO0(CLK_AUDIO_APLL2_TUNER, "aud_apll2_tuner", "aud_2_sel",
		18),
	GATE_AUDIO0(CLK_AUDIO_APLL_TUNER, "aud_apll_tuner", "aud_1_sel",
		19),
	GATE_AUDIO0(CLK_AUDIO_ADC, "aud_adc", "audio_sel",
		24),
	GATE_AUDIO0(CLK_AUDIO_DAC, "aud_dac", "audio_sel",
		25),
	GATE_AUDIO0(CLK_AUDIO_DAC_PREDIS, "aud_dac_predis", "audio_sel",
		26),
	GATE_AUDIO0(CLK_AUDIO_TML, "aud_tml", "audio_sel",
		27),
	/* AUDIO1 */
	GATE_AUDIO1(CLK_AUDIO_ADC_HIRES, "aud_adc_hires", "audio_h_sel",
		16),
	GATE_AUDIO1(CLK_AUDIO_ADC_HIRES_TML, "aud_hires_tml", "audio_h_sel",
		17),
	GATE_AUDIO1(CLK_AUDIO_ADDA6_ADC, "aud_adda6_adc", "audio_sel",
		20),
	GATE_AUDIO1(CLK_AUDIO_ADDA6_ADC_HIRES, "aud_adda6_hires", "audio_h_sel",
		21),
	GATE_AUDIO1(CLK_AUDIO_ADDA4_ADC, "aud_adda4_adc", "audio_sel",
		24),
	GATE_AUDIO1(CLK_AUDIO_ADDA4_ADC_HIRES, "aud_adda4_hires", "audio_h_sel",
		25),
	GATE_AUDIO1(CLK_AUDIO_ADDA5_ADC, "aud_adda5_adc", "audio_sel",
		26),
	GATE_AUDIO1(CLK_AUDIO_ADDA5_ADC_HIRES, "aud_adda5_hires", "audio_h_sel",
		27),
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
	GATE_CAM(CLK_CAM_LARB6, "camsys_larb6", "cam_sel", 0),
	GATE_CAM(CLK_CAM_DFP_VAD, "camsys_dfp_vad", "cam_sel", 1),
	GATE_CAM(CLK_CAM_LARB3, "camsys_larb3", "cam_sel", 2),
	GATE_CAM(CLK_CAM, "camsys_cam", "cam_sel", 6),
	GATE_CAM(CLK_CAMTG, "camsys_camtg", "camtg_sel", 7),
	GATE_CAM(CLK_CAM_SENINF, "camsys_seninf", "cam_sel", 8),
	GATE_CAM(CLK_CAMSV0, "camsys_camsv0", "cam_sel", 9),
	GATE_CAM(CLK_CAMSV1, "camsys_camsv1", "cam_sel", 10),
	GATE_CAM(CLK_CAMSV2, "camsys_camsv2", "cam_sel", 11),
	GATE_CAM(CLK_CAM_CCU, "camsys_ccu", "cam_sel", 12),
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
	GATE_IMG(CLK_IMG_LARB5, "imgsys_larb5", "img_sel", 0),
	GATE_IMG(CLK_IMG_LARB2, "imgsys_larb2", "img_sel", 1),
	GATE_IMG(CLK_IMG_DIP, "imgsys_dip", "img_sel", 2),
	GATE_IMG(CLK_IMG_FDVT, "imgsys_fdvt", "img_sel", 3),
	GATE_IMG(CLK_IMG_DPE, "imgsys_dpe", "img_sel", 4),
	GATE_IMG(CLK_IMG_RSC, "imgsys_rsc", "img_sel", 5),
	GATE_IMG(CLK_IMG_WPE, "imgsys_wpe", "img_sel", 6),
	GATE_IMG(CLK_IMG_GEPF, "imgsys_gepf", "img_sel", 7),
	GATE_IMG(CLK_IMG_EAF, "imgsys_eaf", "img_sel", 8),
	GATE_IMG(CLK_IMG_DFP, "imgsys_dfp", "img_sel", 10),
	GATE_IMG(CLK_IMG_VAD, "imgsys_vad", "img_sel", 11),
};

static const struct mtk_gate_regs ipu_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_IPU(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &ipu_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate ipu_clks[] __initconst = {
	GATE_IPU(CLK_IPU, "ipusys_ipu", "dsp_sel", 0),
	GATE_IPU(CLK_IPU_ISP, "ipusys_isp", "ipu_if_sel", 1),
	GATE_IPU(CLK_IPU_DFP, "ipusys_ipu_dfp", "dfp_sel", 2),
	GATE_IPU(CLK_IPU_VAD, "ipusys_ipu_vad", "dfp_sel", 3),
	GATE_IPU(CLK_IPU_JTAG, "ipusys_jtag", "clk_null", 4),
	GATE_IPU(CLK_IPU_AXI, "ipusys_axi", "ipu_if_sel", 5),
	GATE_IPU(CLK_IPU_AHB, "ipusys_ahb", "ipu_if_sel", 6),
	GATE_IPU(CLK_IPU_MM_AXI, "ipusys_mm_axi", "ipu_if_sel", 7),
	GATE_IPU(CLK_IPU_CAM_AXI, "ipusys_cam_axi", "ipu_if_sel", 8),
	GATE_IPU(CLK_IPU_IMG_AXI, "ipusys_img_axi", "ipu_if_sel", 9),
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

#define GATE_MM2(_id, _name, _parent, _shift) {	\
		.id = _id,			\
		.name = _name,			\
		.parent_name = _parent,		\
		.regs = &mm2_cg_regs,		\
		.shift = _shift,		\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate mm_clks[] __initconst = {
	/* MM0 */
	GATE_MM0(CLK_MM_CG0_B0, "mm_cg0_b0", "mm_sel", 0),
	GATE_MM0(CLK_MM_CG0_B1, "mm_cg0_b1", "mm_sel", 1),
	GATE_MM0(CLK_MM_CG0_B2, "mm_cg0_b2", "mm_sel", 2),
	GATE_MM0(CLK_MM_CG0_B3, "mm_cg0_b3", "mm_sel", 3),
	GATE_MM0(CLK_MM_CG0_B4, "mm_cg0_b4", "mm_sel", 4),
	GATE_MM0(CLK_MM_CG0_B5, "mm_cg0_b5", "mm_sel", 5),
	GATE_MM0(CLK_MM_CG0_B6, "mm_cg0_b6", "mm_sel", 6),
	GATE_MM0(CLK_MM_CG0_B7, "mm_cg0_b7", "mm_sel", 7),
	GATE_MM0(CLK_MM_CG0_B8, "mm_cg0_b8", "mm_sel", 8),
	GATE_MM0(CLK_MM_CG0_B9, "mm_cg0_b9", "mm_sel", 9),
	GATE_MM0(CLK_MM_CG0_B10, "mm_cg0_b10", "mm_sel", 10),
	GATE_MM0(CLK_MM_CG0_B11, "mm_cg0_b11", "mm_sel", 11),
	GATE_MM0(CLK_MM_CG0_B12, "mm_cg0_b12", "mm_sel", 12),
	GATE_MM0(CLK_MM_CG0_B13, "mm_cg0_b13", "mm_sel", 13),
	GATE_MM0(CLK_MM_CG0_B14, "mm_cg0_b14", "mm_sel", 14),
	GATE_MM0(CLK_MM_CG0_B15, "mm_cg0_b15", "mm_sel", 15),
	GATE_MM0(CLK_MM_CG0_B16, "mm_cg0_b16", "mm_sel", 16),
	GATE_MM0(CLK_MM_CG0_B17, "mm_cg0_b17", "mm_sel", 17),
	GATE_MM0(CLK_MM_CG0_B18, "mm_cg0_b18", "mm_sel", 18),
	GATE_MM0(CLK_MM_CG0_B19, "mm_cg0_b19", "mm_sel", 19),
	GATE_MM0(CLK_MM_CG0_B20, "mm_cg0_b20", "mm_sel", 20),
	GATE_MM0(CLK_MM_CG0_B21, "mm_cg0_b21", "mm_sel", 21),
	GATE_MM0(CLK_MM_CG0_B22, "mm_cg0_b22", "mm_sel", 22),
	GATE_MM0(CLK_MM_CG0_B23, "mm_cg0_b23", "mm_sel", 23),
	GATE_MM0(CLK_MM_CG0_B24, "mm_cg0_b24", "mm_sel", 24),
	GATE_MM0(CLK_MM_CG0_B25, "mm_cg0_b25", "mm_sel", 25),
	GATE_MM0(CLK_MM_CG0_B26, "mm_cg0_b26", "mm_sel", 26),
	GATE_MM0(CLK_MM_CG0_B27, "mm_cg0_b27", "mm_sel", 27),
	GATE_MM0(CLK_MM_CG0_B28, "mm_cg0_b28", "mm_sel", 28),
	GATE_MM0(CLK_MM_CG0_B29, "mm_cg0_b29", "mm_sel", 29),
	GATE_MM0(CLK_MM_CG0_B30, "mm_cg0_b30", "mm_sel", 30),
	GATE_MM0(CLK_MM_CG0_B31, "mm_cg0_b31", "mm_sel", 31),
	/* MM1 */
	GATE_MM1(CLK_MM_CG1_B0, "mm_cg1_b0", "dpi0_sel", 0),
	GATE_MM1(CLK_MM_CG1_B1, "mm_cg1_b1", "mm_sel", 1),
	GATE_MM1(CLK_MM_CG1_B2, "mm_cg1_b2", "mm_sel", 2),
	GATE_MM1(CLK_MM_CG1_B3, "mm_cg1_b3", "mm_sel", 3),
	GATE_MM1(CLK_MM_CG1_B4, "mm_cg1_b4", "mm_sel", 4),
	GATE_MM1(CLK_MM_CG1_B5, "mm_cg1_b5", "mm_sel", 5),
	GATE_MM1(CLK_MM_CG1_B6, "mm_cg1_b6", "smi0_2x_sel", 6),
	GATE_MM1(CLK_MM_CG1_B7, "mm_cg1_b7", "smi0_2x_sel", 7),
	GATE_MM1(CLK_MM_CG1_B8, "mm_cg1_b8", "smi0_2x_sel", 8),
	GATE_MM1(CLK_MM_CG1_B9, "mm_cg1_b9", "smi0_2x_sel", 9),
	GATE_MM1(CLK_MM_CG1_B10, "mm_cg1_b10", "smi0_2x_sel", 10),
	GATE_MM1(CLK_MM_CG1_B11, "mm_cg1_b11", "smi0_2x_sel", 11),
	GATE_MM1(CLK_MM_CG1_B12, "mm_cg1_b12", "smi0_2x_sel", 12),
	GATE_MM1(CLK_MM_CG1_B13, "mm_cg1_b13", "smi0_2x_sel", 13),
	GATE_MM1(CLK_MM_CG1_B14, "mm_cg1_b14", "smi0_2x_sel", 14),
	GATE_MM1(CLK_MM_CG1_B15, "mm_cg1_b15", "smi0_2x_sel", 15),
	GATE_MM1(CLK_MM_CG1_B16, "mm_cg1_b16", "smi0_2x_sel", 16),
	GATE_MM1(CLK_MM_CG1_B17, "mm_cg1_b17", "smi0_2x_sel", 17),
	/* MM2 */
	GATE_MM2(CLK_MM_CG2_B0, "mm_cg2_b0", "mm_sel", 0),
	GATE_MM2(CLK_MM_CG2_B1, "mm_cg2_b1", "clk_null", 1),
	GATE_MM2(CLK_MM_CG2_B2, "mm_cg2_b2", "mm_sel", 2),
	GATE_MM2(CLK_MM_CG2_B3, "mm_cg2_b3", "clk_null", 3),
	GATE_MM2(CLK_MM_CG2_B4, "mm_cg2_b4", "mm_sel", 4),
	GATE_MM2(CLK_MM_CG2_B5, "mm_cg2_b5", "mm_sel", 5),
	GATE_MM2(CLK_MM_CG2_B6, "mm_cg2_b6", "mm_sel", 6),
	GATE_MM2(CLK_MM_CG2_B7, "mm_cg2_b7", "clk_null", 7),
	GATE_MM2(CLK_MM_CG2_B8, "mm_cg2_b8", "clk_null", 8),
	GATE_MM2(CLK_MM_CG2_B9, "mm_cg2_b9", "smi0_2x_sel", 9),
	GATE_MM2(CLK_MM_CG2_B10, "mm_cg2_b10", "smi0_2x_sel", 10),
	GATE_MM2(CLK_MM_CG2_B11, "mm_cg2_b11", "smi0_2x_sel", 11),
	GATE_MM2(CLK_MM_CG2_B12, "mm_cg2_b12", "smi0_2x_sel", 12),
	GATE_MM2(CLK_MM_CG2_B13, "mm_cg2_b13", "img_sel", 13),
	GATE_MM2(CLK_MM_CG2_B14, "mm_cg2_b14", "clk_null", 14),
};

static const struct mtk_gate_regs venc_global_con_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_VENC_GLOBAL_CON_I(_id, _name, _parent, _shift) {	\
		.id = _id,					\
		.name = _name,					\
		.parent_name = _parent,				\
		.regs = &venc_global_con_cg_regs,		\
		.shift = _shift,				\
		.ops = &mtk_clk_gate_ops_setclr_inv,			\
	}

static const struct mtk_gate venc_global_con_clks[] __initconst = {
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE0, "venc_cke0",
		"venc_sel", 0),
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE1, "venc_cke1",
		"venc_sel", 4),
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE2, "venc_cke2",
		"venc_sel", 8),
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE3, "venc_cke3",
		"venc_sel", 12),
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE4, "venc_cke4",
		"clk_null", 16),
	GATE_VENC_GLOBAL_CON_I(CLK_VENC_GLOBAL_CON_CKE5, "venc_cke5",
		"clk_null", 20),
};

static const struct mtk_gate_regs mjc_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_MJC(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &mjc_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,		\
	}

static const struct mtk_gate mjc_clks[] __initconst = {
	GATE_MJC(CLK_MJC_SMI_LARB, "mjc_smi_larb", "mjc_sel", 0),
	GATE_MJC(CLK_MJC_TOP0, "mjc_top0", "mjc_sel", 1),
	GATE_MJC(CLK_MJC_TOP1, "mjc_top1", "mjc_sel", 2),
	GATE_MJC(CLK_MJC_TOP2, "mjc_top2", "mjc_sel", 3),
	GATE_MJC(CLK_MJC_GALS_AXI, "mjc_gals_axi", "mjc_sel", 4),
	GATE_MJC(CLK_MJC_FAKE_ENGINE, "mjc_fake_engine", "mjc_sel", 5),
	GATE_MJC(CLK_MJC_METER, "mjc_meter", "dfp_sel", 6),
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

	clk_data = mtk_alloc_clk_data(CLK_TOP_NR_CLK);

	mtk_clk_register_fixed_clks(fixed_clks, ARRAY_SIZE(fixed_clks), clk_data);

	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
	mtk_clk_register_composites(top_muxes, ARRAY_SIZE(top_muxes), base,
		&mt6799_clk_lock, clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	cksys_base = base;
	clk_writel(CLK_SCP_CFG_0, clk_readl(CLK_SCP_CFG_0) | 0x3EF);
	clk_writel(CLK_SCP_CFG_1, clk_readl(CLK_SCP_CFG_1) | 0x15);
	/*mtk_clk_enable_critical();*/
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt6799-topckgen", mtk_topckgen_init);

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

	clk_data = mtk_alloc_clk_data(CLK_INFRA_NR_CLK);

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
#endif
}
CLK_OF_DECLARE(mtk_infracfg_ao, "mediatek,mt6799-infracfg_ao",
		mtk_infracfg_ao_init);
#if 0
struct mtk_clk_usb {
	int id;
	const char *name;
	const char *parent;
	u32 reg_ofs;
};

#define APMIXED_USB(_id, _name, _parent, _reg_ofs) {			\
		.id = _id,						\
		.name = _name,						\
		.parent = _parent,					\
		.reg_ofs = _reg_ofs,					\
	}

static const struct mtk_clk_usb apmixed_usb[] __initconst = {
	APMIXED_USB(CLK_APMIXED_REF2USB_TX, "ref2usb_tx", "clk26m", 0x8),
};
#endif
/* FIXME: modify FMAX */
#define MT6799_PLL_FMAX		(3200UL * MHZ)

#define CON0_MT6799_RST_BAR	BIT(24)

#define PLL_B(_id, _name, _reg, _pwr_reg, _en_mask, _flags, _pcwbits,	\
			_pd_reg, _pd_shift, _tuner_reg, _pcw_reg,	\
			_pcw_shift, _div_table) {			\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.rst_bar_mask = CON0_MT6799_RST_BAR,			\
		.fmax = MT6799_PLL_FMAX,				\
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
	PLL(CLK_APMIXED_UNIVPLL, "univpll", 0x0240, 0x024C, 0x00000001, 0,
		22, 0x0244, 28, 0x0, 0x0244, 0),
	PLL(CLK_APMIXED_MSDCPLL, "msdcpll", 0x0250, 0x025C, 0x00000001, 0,
		22, 0x0254, 28, 0x0, 0x0254, 0),
#if 0
	PLL(CLK_APMIXED_EMIPLL, "emipll", 0x0290, 0x029C, 0xfc0001c1, 0,
		22, 0x0294, 28, 0x0, 0x0294, 0),
#endif
	PLL(CLK_APMIXED_GPUPLL, "gpupll", 0x0210, 0x021C, 0x00000001, 0,
		22, 0x0214, 28, 0x0, 0x0214, 0),
	PLL(CLK_APMIXED_TVDPLL, "tvdpll", 0x0280, 0x028C, 0x00000001, 0,
		22, 0x0284, 28, 0x0, 0x0284, 0),
	PLL(CLK_APMIXED_MMPLL, "mmpll", 0x0260, 0x026C, 0x00000001, 0,
		22, 0x0264, 28, 0x0, 0x0264, 0),
	PLL(CLK_APMIXED_VCODECPLL, "vcodecpll", 0x0270, 0x027C, 0x00000001, 0,
		22, 0x0274, 28, 0x0, 0x0274, 0),
	PLL(CLK_APMIXED_FSMIPLL, "fsmipll", 0x0200, 0x020C, 0x00000001, 0,
		22, 0x0204, 28, 0x0, 0x0204, 0),
#if 0
	PLL(CLK_APMIXED_MPLL, "mpll", 0x0220, 0x022C, 0xfc0001c1, 0,
		22, 0x0224, 28, 0x0, 0x0224, 0),
#endif
	PLL(CLK_APMIXED_APLL1, "apll1", 0x02A0, 0x02B8, 0x00000001, 0,
		32, 0x02A4, 28, 0x0, 0x02A8, 0),
	PLL(CLK_APMIXED_APLL2, "apll2", 0x02C0, 0x02D8, 0x00000001, 0,
		32, 0x02C4, 28, 0x0, 0x02C8, 0),
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

	clk_data = mtk_alloc_clk_data(CLK_APMIXED_NR_CLK);

	/* FIXME: add code for APMIXEDSYS */
	mtk_clk_register_plls(node, plls, ARRAY_SIZE(plls), clk_data);
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	apmixed_base = base;
	clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xee2b8ae2);/* ARMPLL4, MPLL, CCIPLL, EMIPLL, MAINPLL */
	clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xee2ffee2);

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
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt6799-apmixedsys",
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

	clk_data = mtk_alloc_clk_data(CLK_AUDIO_NR_CLK);

	mtk_clk_register_gates(node, audio_clks, ARRAY_SIZE(audio_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	audio_base = base;

#if MT_CCF_BRINGUP
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
	clk_data = mtk_alloc_clk_data(CLK_CAM_NR_CLK);

	mtk_clk_register_gates(node, cam_clks, ARRAY_SIZE(cam_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	cam_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(CAMSYS_CG_CLR, CAMSYS_DISABLE_CG);*/
	clk_writel(CAMSYS_CG_SET, CAM_CG);
#endif
}
CLK_OF_DECLARE(mtk_camsys, "mediatek,mt6799-camsys", mtk_camsys_init);

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
	clk_data = mtk_alloc_clk_data(CLK_IMG_NR_CLK);

	mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	img_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(IMG_CG_CLR, IMG_DISABLE_CG);*/
	clk_writel(IMG_CG_SET, IMG_CG);
#endif
}
CLK_OF_DECLARE(mtk_imgsys, "mediatek,mt6799-imgsys", mtk_imgsys_init);

static void __init mtk_ipusys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(CLK_IPU_NR_CLK);

	mtk_clk_register_gates(node, ipu_clks, ARRAY_SIZE(ipu_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	ipu_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(IPU_CG_CLR, IPU_DISABLE_CG);*/
	clk_writel(IPU_CG_SET, IPU_CG);
#endif

}
CLK_OF_DECLARE(mtk_ipusys, "mediatek,mt6799-ipusys", mtk_ipusys_init);

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

	clk_data = mtk_alloc_clk_data(CLK_MFG_CFG_NR_CLK);

	mtk_clk_register_gates(node, mfg_cfg_clks, ARRAY_SIZE(mfg_cfg_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	mfgcfg_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(MFG_CG_CLR, MFG_DISABLE_CG);*/
	clk_writel(MFG_CG_SET, MFG_CG);
#endif

}
CLK_OF_DECLARE(mtk_mfg_cfg, "mediatek,mt6799-mfg_cfg", mtk_mfg_cfg_init);

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
	clk_data = mtk_alloc_clk_data(CLK_MM_NR_CLK);

	mtk_clk_register_gates(node, mm_clks, ARRAY_SIZE(mm_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	mmsys_config_base = base;
#if MT_CCF_BRINGUP
	clk_writel(MM_CG_SET0, MM_CG0);
	clk_writel(MM_CG_SET1, MM_CG1);
	clk_writel(MM_CG_SET2, MM_CG2);
#endif
}
CLK_OF_DECLARE(mtk_mmsys_config, "mediatek,mt6799-mmsys_config",
		mtk_mmsys_config_init);
#if 0
static void __init mtk_pcie_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_PCIE_NR_CLK);

	mtk_clk_register_gates(node, pcie_clks, ARRAY_SIZE(pcie_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

}
CLK_OF_DECLARE(mtk_pcie, "mediatek,mt6799-pcie", mtk_pcie_init);
#endif
static void __init mtk_pericfg_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_PERICFG_NR_CLK);

	mtk_clk_register_gates(node, pericfg_clks, ARRAY_SIZE(pericfg_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	pericfg_base = base;
#if MT_CCF_BRINGUP
	clk_writel(PERI_CG_SET0, PERI_CG0);
	clk_writel(PERI_CG_SET1, PERI_CG1);
	clk_writel(PERI_CG_SET2, PERI_CG2);
	clk_writel(PERI_CG_SET3, PERI_CG3);
	clk_writel(PERI_CG_SET4, PERI_CG4);
#endif
}
CLK_OF_DECLARE(mtk_pericfg, "mediatek,mt6799-pericfg", mtk_pericfg_init);
#if 0
static void __init mtk_pwrap_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_PWRAP_NR_CLK);

	mtk_clk_register_gates(node, pwrap_clks, ARRAY_SIZE(pwrap_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

}
CLK_OF_DECLARE(mtk_pwrap, "mediatek,mt6799-pwrap", mtk_pwrap_init);

static void __init mtk_spi_nor_ext_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_SPI_NOR_EXT_NR_CLK);

	mtk_clk_register_gates(node, spi_nor_ext_clks, ARRAY_SIZE(spi_nor_ext_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

}
CLK_OF_DECLARE(mtk_spi_nor_ext, "mediatek,mt6799-spi_nor_ext",
		mtk_spi_nor_ext_init);
#endif

static void __init mtk_mjc_config_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}

	clk_data = mtk_alloc_clk_data(CLK_MJC_NR_CLK);

	mtk_clk_register_gates(node, mjc_clks, ARRAY_SIZE(mjc_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	mjc_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(MJC_CG_CLR, MJC_DISABLE_CG);*/
	clk_writel(MJC_CG_SET, MJC_CG);
#endif

}
CLK_OF_DECLARE(mtk_mjc_config, "mediatek,mjc_config",
		mtk_mjc_config_init);

#if 0
static void __init mtk_usb0_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_USB0_NR_CLK);

	mtk_clk_register_gates(node, usb0_clks, ARRAY_SIZE(usb0_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

}
CLK_OF_DECLARE(mtk_usb0, "mediatek,mt6799-usb0", mtk_usb0_init);
#endif
static void __init mtk_vdec_gcon_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	base = of_iomap(node, 0);
	if (!base) {
		pr_err("%s(): ioremap failed\n", __func__);
		return;
	}
	clk_data = mtk_alloc_clk_data(CLK_VDEC_NR_CLK);

	mtk_clk_register_gates(node, vdec_clks, ARRAY_SIZE(vdec_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	vdec_gcon_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(VDEC_CKEN_SET, VDEC_DISABLE_CG);*/
	/*clk_writel(LARB1_CKEN_SET, LARB_DISABLE_CG);*/
	clk_writel(VDEC_CKEN_CLR, VDE_CG);
	clk_writel(LARB1_CKEN_CLR, LARB1_CG);
#endif
}
CLK_OF_DECLARE(mtk_vdec_gcon, "mediatek,mt6799-vdec_gcon", mtk_vdec_gcon_init);
#if 0
static void __init mtk_venc_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	int r;

	clk_data = mtk_alloc_clk_data(CLK_VENC_NR_CLK);

	mtk_clk_register_gates(node, venc_clks, ARRAY_SIZE(venc_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

}
CLK_OF_DECLARE(mtk_venc, "mediatek,mt6799-venc", mtk_venc_init);
#endif
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
	clk_data = mtk_alloc_clk_data(CLK_VENC_GLOBAL_CON_NR_CLK);

	mtk_clk_register_gates(node, venc_global_con_clks, ARRAY_SIZE(venc_global_con_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);
	venc_gcon_base = base;

#if MT_CCF_BRINGUP
	/*clk_writel(VENC_CG_SET, VENC_DISABLE_CG);*/
	clk_writel(VENC_CG_CLR, VEN_CG);
#endif
}
CLK_OF_DECLARE(mtk_venc_global_con, "mediatek,mt6799-venc_global_con",
		mtk_venc_global_con_init);

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
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xfdffffdf);
		} else {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x02008020);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x02000020);
		}
	} else if (id == 1) { /* mp1 */
		if (suspend) {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) & 0xfbfeffbf);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) & 0xfbffffbf);
		} else {
			clk_writel(AP_PLL_CON3, clk_readl(AP_PLL_CON3) | 0x04010040);
			clk_writel(AP_PLL_CON4, clk_readl(AP_PLL_CON4) | 0x04000040);
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

static int __init clk_mt6799_init(void)
{
	/*timer_ready = true;*/
	/*mtk_clk_enable_critical();*/

	return 0;
}
arch_initcall(clk_mt6799_init);


/*
* Copyright (c) 2014 MediaTek Inc.
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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>

#include "clk-mtk-v1.h"
#include "clk-pll-v1.h"
#include "clk-gate-v1.h"
#include "clk-mt6735-pll.h"

#include <dt-bindings/clock/mt6735-clk.h>

#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP	0
#endif

/*
 * platform clocks
 */

/* ROOT */
#define clk_null		"clk_null"
#define clk26m			"clk26m"
#define clk32k			"clk32k"

#define clkph_mck_o		"clkph_mck_o"
#define dpi_ck			"dpi_ck"

/* PLL */
#define armpll		"armpll"
#define mainpll		"mainpll"
#define msdcpll		"msdcpll"
#define univpll		"univpll"
#define mmpll		"mmpll"
#define vencpll		"vencpll"
#define tvdpll		"tvdpll"
#define apll1		"apll1"
#define apll2		"apll2"

/* DIV */
#define ad_apll1_ck		"ad_apll1_ck"
#define ad_sys_26m_ck		"ad_sys_26m_ck"
#define ad_sys_26m_d2		"ad_sys_26m_d2"
#define dmpll_ck		"dmpll_ck"
#define dmpll_d2		"dmpll_d2"
#define dmpll_d4		"dmpll_d4"
#define dmpll_d8		"dmpll_d8"
#define dpi_ck		"dpi_ck"
#define mmpll_ck		"mmpll_ck"
#define msdcpll_ck		"msdcpll_ck"
#define msdcpll_d16		"msdcpll_d16"
#define msdcpll_d2		"msdcpll_d2"
#define msdcpll_d4		"msdcpll_d4"
#define msdcpll_d8		"msdcpll_d8"
#define syspll_d2		"syspll_d2"
#define syspll_d3		"syspll_d3"
#define syspll_d5		"syspll_d5"
#define syspll1_d16		"syspll1_d16"
#define syspll1_d2		"syspll1_d2"
#define syspll1_d4		"syspll1_d4"
#define syspll1_d8		"syspll1_d8"
#define syspll2_d2		"syspll2_d2"
#define syspll2_d4		"syspll2_d4"
#define syspll3_d2		"syspll3_d2"
#define syspll3_d4		"syspll3_d4"
#define syspll4_d2		"syspll4_d2"
#define syspll4_d2_d8		"syspll4_d2_d8"
#define syspll4_d4		"syspll4_d4"
#define tvdpll_ck		"tvdpll_ck"
#define tvdpll_d2		"tvdpll_d2"
#define tvdpll_d4		"tvdpll_d4"
#define univpll_d2		"univpll_d2"
#define univpll_d26		"univpll_d26"
#define univpll_d3		"univpll_d3"
#define univpll_d5		"univpll_d5"
#define univpll1_d2		"univpll1_d2"
#define univpll1_d4		"univpll1_d4"
#define univpll1_d8		"univpll1_d8"
#define univpll2_d2		"univpll2_d2"
#define univpll2_d4		"univpll2_d4"
#define univpll2_d8		"univpll2_d8"
#define univpll3_d2		"univpll3_d2"
#define univpll3_d4		"univpll3_d4"
#define vencpll_ck		"vencpll_ck"
#define vencpll_d3		"vencpll_d3"
#define whpll_audio_ck		"whpll_audio_ck"

/* TOP */
#define axi_sel			"axi_sel"
#define mem_sel			"mem_sel"
#define ddrphycfg_sel			"ddrphycfg_sel"
#define mm_sel			"mm_sel"
#define pwm_sel			"pwm_sel"
#define vdec_sel			"vdec_sel"
#define mfg_sel			"mfg_sel"
#define camtg_sel			"camtg_sel"
#define uart_sel			"uart_sel"
#define spi_sel			"spi_sel"
#define usb20_sel			"usb20_sel"
#define msdc50_0_sel			"msdc50_0_sel"
#define msdc30_0_sel			"msdc30_0_sel"
#define msdc30_1_sel			"msdc30_1_sel"
#define msdc30_2_sel			"msdc30_2_sel"
#define msdc30_3_sel			"msdc30_3_sel"
#define audio_sel			"audio_sel"
#define aud_intbus_sel			"aud_intbus_sel"
#define pmicspi_sel			"pmicspi_sel"
#define scp_sel			"scp_sel"
#define atb_sel			"atb_sel"
#define dpi0_sel			"dpi0_sel"
#define scam_sel			"scam_sel"
#define mfg13m_sel			"mfg13m_sel"
#define aud_1_sel			"aud_1_sel"
#define aud_2_sel			"aud_2_sel"
#define irda_sel			"irda_sel"
#define irtx_sel			"irtx_sel"
#define disppwm_sel			"disppwm_sel"

/* INFRA */
#define infra_dbgclk		"infra_dbgclk"
#define infra_gce		"infra_gce"
#define infra_trbg		"infra_trbg"
#define infra_cpum		"infra_cpum"
#define infra_devapc		"infra_devapc"
#define infra_audio		"infra_audio"
#define infra_gcpu		"infra_gcpu"
#define infra_l2csram		"infra_l2csram"
#define infra_m4u		"infra_m4u"
#define infra_cldma		"infra_cldma"
#define infra_connmcubus	"infra_connmcubus"
#define infra_kp		"infra_kp"
#define infra_apxgpt		"infra_apxgpt"
#define infra_sej		"infra_sej"
#define infra_ccif0ap		"infra_ccif0ap"
#define infra_ccif1ap		"infra_ccif1ap"
#define infra_pmicspi		"infra_pmicspi"
#define infra_pmicwrap		"infra_pmicwrap"

/* PERI */
#define peri_disp_pwm		"peri_disp_pwm"
#define peri_therm		"peri_therm"
#define peri_pwm1		"peri_pwm1"
#define peri_pwm2		"peri_pwm2"
#define peri_pwm3		"peri_pwm3"
#define peri_pwm4		"peri_pwm4"
#define peri_pwm5		"peri_pwm5"
#define peri_pwm6		"peri_pwm6"
#define peri_pwm7		"peri_pwm7"
#define peri_pwm		"peri_pwm"
#define peri_usb0		"peri_usb0"
#define peri_irda		"peri_irda"
#define peri_apdma		"peri_apdma"
#define peri_msdc30_0		"peri_msdc30_0"
#define peri_msdc30_1		"peri_msdc30_1"
#define peri_msdc30_2		"peri_msdc30_2"
#define peri_msdc30_3		"peri_msdc30_3"
#define peri_uart0		"peri_uart0"
#define peri_uart1		"peri_uart1"
#define peri_uart2		"peri_uart2"
#define peri_uart3		"peri_uart3"
#define peri_uart4		"peri_uart4"
#define peri_btif		"peri_btif"
#define peri_i2c0		"peri_i2c0"
#define peri_i2c1		"peri_i2c1"
#define peri_i2c2		"peri_i2c2"
#define peri_i2c3		"peri_i2c3"
#define peri_auxadc		"peri_auxadc"
#define peri_spi0		"peri_spi0"
#define peri_irtx		"peri_irtx"

/* MFG */
#define mfg_bg3d			"mfg_bg3d"

/* IMG */
#define img_image_larb2_smi		"img_image_larb2_smi"
#define img_image_cam_smi		"img_image_cam_smi"
#define img_image_cam_cam		"img_image_cam_cam"
#define img_image_sen_tg		"img_image_sen_tg"
#define img_image_sen_cam		"img_image_sen_cam"
#define img_image_cam_sv		"img_image_cam_sv"
#define img_image_sufod		"img_image_sufod"
#define img_image_fd		"img_image_fd"

/* MM_SYS */
#define mm_disp0_smi_common		"mm_disp0_smi_common"
#define mm_disp0_smi_larb0		"mm_disp0_smi_larb0"
#define mm_disp0_cam_mdp		"mm_disp0_cam_mdp"
#define mm_disp0_mdp_rdma		"mm_disp0_mdp_rdma"
#define mm_disp0_mdp_rsz0		"mm_disp0_mdp_rsz0"
#define mm_disp0_mdp_rsz1		"mm_disp0_mdp_rsz1"
#define mm_disp0_mdp_tdshp		"mm_disp0_mdp_tdshp"
#define mm_disp0_mdp_wdma		"mm_disp0_mdp_wdma"
#define mm_disp0_mdp_wrot		"mm_disp0_mdp_wrot"
#define mm_disp0_fake_eng		"mm_disp0_fake_eng"
#define mm_disp0_disp_ovl0		"mm_disp0_disp_ovl0"
#define mm_disp0_disp_rdma0		"mm_disp0_disp_rdma0"
#define mm_disp0_disp_rdma1		"mm_disp0_disp_rdma1"
#define mm_disp0_disp_wdma0		"mm_disp0_disp_wdma0"
#define mm_disp0_disp_color		"mm_disp0_disp_color"
#define mm_disp0_disp_ccorr		"mm_disp0_disp_ccorr"
#define mm_disp0_disp_aal		"mm_disp0_disp_aal"
#define mm_disp0_disp_gamma		"mm_disp0_disp_gamma"
#define mm_disp0_disp_dither		"mm_disp0_disp_dither"
#define mm_disp1_dsi_engine		"mm_disp1_dsi_engine"
#define mm_disp1_dsi_digital		"mm_disp1_dsi_digital"
#define mm_disp1_dpi_engine		"mm_disp1_dpi_engine"
#define mm_disp1_dpi_pixel		"mm_disp1_dpi_pixel"

/* VDEC */
#define vdec0_vdec		"vdec0_vdec"
#define vdec1_larb		"vdec1_larb"

/* VENC */
#define venc_larb		"venc_larb"
#define venc_venc		"venc_venc"
#define venc_jpgenc		"venc_jpgenc"
#define venc_jpgdec		"venc_jpgdec"

/* AUDIO */
#define audio_afe		"audio_afe"
#define audio_i2s		"audio_i2s"
#define audio_22m		"audio_22m"
#define audio_24m		"audio_24m"
#define audio_apll2_tuner		"audio_apll2_tuner"
#define audio_apll_tuner		"audio_apll_tuner"
#define audio_adc		"audio_adc"
#define audio_dac		"audio_dac"
#define audio_dac_predis		"audio_dac_predis"
#define audio_tml		"audio_tml"

struct mtk_fixed_factor {
	int id;
	const char *name;
	const char *parent_name;
	int mult;
	int div;
};

#define FACTOR(_id, _name, _parent, _mult, _div) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.mult = _mult,				\
		.div = _div,				\
	}

static void __init init_factors(struct mtk_fixed_factor *clks, int num,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < num; i++) {
		struct mtk_fixed_factor *ff = &clks[i];

		clk = clk_register_fixed_factor(NULL, ff->name, ff->parent_name,
				0, ff->mult, ff->div);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
					ff->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[ff->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] factor %3d: %s\n", i, ff->name);
#endif /* MT_CCF_DEBUG */
	}
}

static struct mtk_fixed_factor root_clk_alias[] __initdata = {
	FACTOR(TOP_CLKPH_MCK_O, clkph_mck_o, clk_null, 1, 1),
	FACTOR(TOP_DPI_CK, dpi_ck, clk_null, 1, 1),
};

static void __init init_clk_root_alias(struct clk_onecell_data *clk_data)
{
	init_factors(root_clk_alias, ARRAY_SIZE(root_clk_alias), clk_data);
}

static struct mtk_fixed_factor top_divs[] __initdata = {
	FACTOR(TOP_UNIVPLL_D2, univpll_d2, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL_D3, univpll_d3, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL1_D2, univpll1_d2, univpll, 1, 1),
	FACTOR(TOP_AD_APLL1_CK, ad_apll1_ck, apll1, 1, 1),
	FACTOR(TOP_AD_SYS_26M_CK, ad_sys_26m_ck, clk26m, 1, 1),
	FACTOR(TOP_AD_SYS_26M_D2, ad_sys_26m_d2, clk26m, 1, 1),
	FACTOR(TOP_DMPLL_CK, dmpll_ck, clkph_mck_o, 1, 1),
	FACTOR(TOP_DMPLL_D2, dmpll_d2, clkph_mck_o, 1, 1),
	FACTOR(TOP_DMPLL_D4, dmpll_d4, clkph_mck_o, 1, 1),
	FACTOR(TOP_DMPLL_D8, dmpll_d8, clkph_mck_o, 1, 1),
	FACTOR(TOP_MMPLL_CK, mmpll_ck, mmpll, 1, 1),
	FACTOR(TOP_MSDCPLL_CK, msdcpll_ck, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D16, msdcpll_d16, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D2, msdcpll_d2, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D4, msdcpll_d4, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D8, msdcpll_d8, msdcpll, 1, 1),
	FACTOR(TOP_SYSPLL_D2, syspll_d2, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL_D3, syspll_d3, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL_D5, syspll_d5, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL1_D16, syspll1_d16, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL1_D2, syspll1_d2, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL1_D4, syspll1_d4, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL1_D8, syspll1_d8, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL2_D2, syspll2_d2, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL2_D4, syspll2_d4, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL3_D2, syspll3_d2, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL3_D4, syspll3_d4, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL4_D2, syspll4_d2, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL4_D2_D8, syspll4_d2_d8, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL4_D4, syspll4_d4, mainpll, 1, 1),
	FACTOR(TOP_TVDPLL_CK, tvdpll_ck, tvdpll, 1, 1),
	FACTOR(TOP_TVDPLL_D2, tvdpll_d2, tvdpll, 1, 1),
	FACTOR(TOP_TVDPLL_D4, tvdpll_d4, tvdpll, 1, 1),
	FACTOR(TOP_UNIVPLL_D26, univpll_d26, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL_D5, univpll_d5, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL1_D4, univpll1_d4, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL1_D8, univpll1_d8, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL2_D2, univpll2_d2, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL2_D4, univpll2_d4, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL2_D8, univpll2_d8, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL3_D2, univpll3_d2, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL3_D4, univpll3_d4, univpll, 1, 1),
	FACTOR(TOP_VENCPLL_CK, vencpll_ck, vencpll, 1, 1),
	FACTOR(TOP_VENCPLL_D3, vencpll_d3, vencpll, 1, 1),
	FACTOR(TOP_WHPLL_AUDIO_CK, whpll_audio_ck, clk_null, 1, 1),
};

static void __init init_clk_top_div(struct clk_onecell_data *clk_data)
{
	init_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
}

static const char *axi_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll_d5,
		syspll1_d4,
		univpll_d5,
		univpll2_d2,
		dmpll_ck,
		dmpll_d2};

static const char *mem_parents[] __initconst = {
		clk26m,
		dmpll_ck};

static const char *ddrphycfg_parents[] __initconst = {
		clk26m,
		syspll1_d8};

static const char *mm_parents[] __initconst = {
		clk26m,
		vencpll_ck,
		syspll1_d2,
		syspll_d5,
		syspll1_d4,
		univpll_d5,
		univpll2_d2,
		dmpll_ck};

static const char *pwm_parents[] __initconst = {
		clk26m,
		univpll2_d4,
		univpll3_d2,
		univpll1_d4};

static const char *vdec_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll_d5,
		syspll1_d4,
		univpll_d5,
		syspll_d2,
		syspll2_d2,
		msdcpll_d2};

static const char *mfg_parents[] __initconst = {
		clk26m,
		mmpll_ck,
		clk26m,
		clk26m,
		clk26m,
		clk26m,
		clk26m,
		clk26m,
		clk26m,
		syspll_d3,
		syspll1_d2,
		syspll_d5,
		univpll_d3,
		univpll1_d2
};

static const char *camtg_parents[] __initconst = {
		clk26m,
		univpll_d26,
		univpll2_d2,
		syspll3_d2,
		syspll3_d4,
		msdcpll_d4
};

static const char *uart_parents[] __initconst = {
		clk26m,
		univpll2_d8
};

static const char *spi_parents[] __initconst = {
		clk26m,
		syspll3_d2,
		msdcpll_d8,
		syspll2_d4,
		syspll4_d2,
		univpll2_d4,
		univpll1_d8
};

static const char *usb20_parents[] __initconst = {
		clk26m,
		univpll1_d8,
		univpll3_d4
};

static const char *msdc50_0_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll2_d2,
		syspll4_d2,
		univpll_d5,
		univpll1_d4
};

static const char *msdc30_0_parents[] __initconst = {
		clk26m,
		msdcpll_ck,
		msdcpll_d2,
		msdcpll_d4,
		syspll2_d2,
		syspll1_d4,
		univpll1_d4,
		univpll_d3,
		univpll_d26,
		syspll2_d4,
		univpll_d2
};

static const char *msdc30_1_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		msdcpll_d4,
		syspll2_d2,
		syspll1_d4,
		univpll1_d4,
		univpll_d26,
		syspll2_d4
};

static const char *msdc30_2_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		msdcpll_d4,
		syspll2_d2,
		syspll1_d4,
		univpll1_d4,
		univpll_d26,
		syspll2_d4
};

static const char *msdc30_3_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		msdcpll_d4,
		syspll2_d2,
		syspll1_d4,
		univpll1_d4,
		univpll_d26,
		msdcpll_d16,
		syspll2_d4
};

static const char *audio_parents[] __initconst = {
		clk26m,
		syspll3_d4,
		syspll4_d4,
		syspll1_d16
};

static const char *aud_intbus_parents[] __initconst = {
		clk26m,
		syspll1_d4,
		syspll4_d2,
		dmpll_d4
};

static const char *pmicspi_parents[] __initconst = {
		clk26m,
		syspll1_d8,
		syspll3_d4,
		syspll1_d16,
		univpll3_d4,
		univpll_d26,
		dmpll_d4,
		dmpll_d8
};

static const char *scp_parents[] __initconst = {
		clk26m,
		syspll1_d8,
		dmpll_d2,
		dmpll_d4
};

static const char *atb_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll_d5,
		dmpll_ck
};

static const char *dpi0_parents[] __initconst = {
		clk26m,
		tvdpll_ck,
		tvdpll_d2,
		tvdpll_d4,
		dpi_ck
};

static const char *scam_parents[] __initconst = {
		clk26m,
		syspll3_d2,
		univpll2_d4,
		vencpll_d3
};

static const char *mfg13m_parents[] __initconst = {
		clk26m,
		ad_sys_26m_d2
};

static const char *aud_1_parents[] __initconst = {
		clk26m,
		ad_apll1_ck
};

static const char *aud_2_parents[] __initconst = {
		clk26m,
		whpll_audio_ck
};

static const char *irda_parents[] __initconst = {
		clk26m,
		univpll2_d4
};

static const char *irtx_parents[] __initconst = {
		clk26m,
		ad_sys_26m_ck
};

static const char *disppwm_parents[] __initconst = {
		clk26m,
		univpll2_d4,
		syspll4_d2_d8,
		ad_sys_26m_ck
};

struct mtk_mux {
	int id;
	const char *name;
	uint32_t reg;
	int shift;
	int width;
	int gate;
	const char **parent_names;
	int num_parents;
};

#define MUX(_id, _name, _parents, _reg, _shift, _width, _gate) {	\
		.id = _id,						\
		.name = _name,						\
		.reg = _reg,						\
		.shift = _shift,					\
		.width = _width,					\
		.gate = _gate,						\
		.parent_names = (const char **)_parents,		\
		.num_parents = ARRAY_SIZE(_parents),			\
	}

static struct mtk_mux top_muxes[] __initdata = {
	MUX(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 8, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_DDRPHY, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 16, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 24, 3, 31),
	MUX(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0, 2, 7),
	MUX(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 8, 3, 15),
	MUX(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 16, 4, 23),
	MUX(TOP_MUX_CAMTG, camtg_sel, camtg_parents, 0x0050, 24, 3, 31),
	MUX(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 0, 1, 7),
	MUX(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 8, 3, 15),
	MUX(TOP_MUX_USB20, usb20_sel, usb20_parents, 0x0060, 16, 2, 23),
	MUX(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0060, 24, 3, 31),
	MUX(TOP_MUX_MSDC30_0, msdc30_0_sel, msdc30_0_parents, 0x0070, 0, 4, 7),
	MUX(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 8, 3, 15),
	MUX(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0070, 16, 3, 23),
	MUX(TOP_MUX_MSDC30_3, msdc30_3_sel, msdc30_3_parents, 0x0070, 24, 4, 31),
	MUX(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 0, 2, 7),
	MUX(TOP_MUX_AUDINTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 8, 2, 15),
	MUX(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0080, 16, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_SCP, scp_sel, scp_parents, 0x0080, 24, 2, 31),
	MUX(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 0, 2, 7),
	MUX(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x0090, 8, 3, 15),
	MUX(TOP_MUX_SCAM, scam_sel, scam_parents, 0x0090, 16, 2, 23),
	MUX(TOP_MUX_MFG13M, mfg13m_sel, mfg13m_parents, 0x0090, 24, 1, 31),
	MUX(TOP_MUX_AUD1, aud_1_sel, aud_1_parents, 0x00a0, 0, 1, 7),
	MUX(TOP_MUX_AUD2, aud_2_sel, aud_2_parents, 0x00a0, 8, 1, 15),
	MUX(TOP_MUX_IRDA, irda_sel, irda_parents, 0x00a0, 16, 1, 23),
	MUX(TOP_MUX_IRTX, irtx_sel, irtx_parents, 0x00a0, 24, 1, 31),
	MUX(TOP_MUX_DISPPWM, disppwm_sel, disppwm_parents, 0x00b0, 0, 2, 7),
};

static void __init init_clk_topckgen(void __iomem *top_base,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(top_muxes); i++) {
		struct mtk_mux *mux = &top_muxes[i];

		clk = mtk_clk_register_mux(mux->name,
			mux->parent_names, mux->num_parents,
			top_base + mux->reg, mux->shift, mux->width, mux->gate);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
					mux->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[mux->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] mux %3d: %s\n", i, mux->name);
#endif /* MT_CCF_DEBUG */
	}
}

struct mtk_pll {
	int id;
	const char *name;
	const char *parent_name;
	uint32_t reg;
	uint32_t pwr_reg;
	uint32_t en_mask;
	unsigned int flags;
	const struct clk_ops *ops;
};

#define PLL(_id, _name, _parent, _reg, _pwr_reg, _en_mask, _flags, _ops) { \
		.id = _id,						\
		.name = _name,						\
		.parent_name = _parent,					\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.ops = _ops,						\
	}

static struct mtk_pll plls[] __initdata = {
	PLL(APMIXED_ARMPLL, armpll, clk26m, 0x0200, 0x020C, 0x00000001, HAVE_PLL_HP, &mt_clk_arm_pll_ops),
	PLL(APMIXED_MAINPLL, mainpll, clk26m, 0x0210, 0x021C, 0xF0000101, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_MSDCPLL, msdcpll, clk26m, 0x0240, 0x024C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_UNIVPLL, univpll, clk26m, 0x0220, 0x022C, 0xFC000001, HAVE_PLL_HP, &mt_clk_univ_pll_ops),
	PLL(APMIXED_MMPLL, mmpll, clk26m, 0x0230, 0x023C, 0x00000001, HAVE_PLL_HP, &mt_clk_mm_pll_ops),
	PLL(APMIXED_VENCPLL, vencpll, clk26m, 0x0250, 0x025C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_TVDPLL, tvdpll, clk26m, 0x0260, 0x026C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_APLL1, apll1, clk26m, 0x0270, 0x0280, 0x00000001, HAVE_PLL_HP, &mt_clk_aud_pll_ops),
	PLL(APMIXED_APLL2, apll2, clk26m, 0x0284, 0x0294, 0x00000001, HAVE_PLL_HP, &mt_clk_aud_pll_ops),
};

static void __init init_clk_apmixedsys(void __iomem *apmixed_base,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(plls); i++) {
		struct mtk_pll *pll = &plls[i];

		clk = mtk_clk_register_pll(pll->name, pll->parent_name,
				apmixed_base + pll->reg,
				apmixed_base + pll->pwr_reg,
				pll->en_mask, pll->flags, pll->ops);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
					pll->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[pll->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] pll %3d: %s\n", i, pll->name);
#endif /* MT_CCF_DEBUG */
	}
}

struct mtk_gate_regs {
	u32 sta_ofs;
	u32 clr_ofs;
	u32 set_ofs;
};

struct mtk_gate {
	int id;
	const char *name;
	const char *parent_name;
	struct mtk_gate_regs *regs;
	int shift;
	uint32_t flags;
};

#define GATE(_id, _name, _parent, _regs, _shift, _flags) {	\
		.id = _id,					\
		.name = _name,					\
		.parent_name = _parent,				\
		.regs = &_regs,					\
		.shift = _shift,				\
		.flags = _flags,				\
	}

static void __init init_clk_gates(
		void __iomem *reg_base,
		struct mtk_gate *clks, int num,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < num; i++) {
		struct mtk_gate *gate = &clks[i];

		clk = mtk_clk_register_gate(gate->name, gate->parent_name,
				reg_base + gate->regs->set_ofs,
				reg_base + gate->regs->clr_ofs,
				reg_base + gate->regs->sta_ofs,
				gate->shift, gate->flags);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n",
					gate->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[gate->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] gate %3d: %s\n", i, gate->name);
#endif /* MT_CCF_DEBUG */
	}
}

static struct mtk_gate_regs infra_cg_regs = {
	.set_ofs = 0x0040,
	.clr_ofs = 0x0044,
	.sta_ofs = 0x0048,
};

static struct mtk_gate infra_clks[] __initdata = {
	GATE(INFRA_DBGCLK, infra_dbgclk, axi_sel, infra_cg_regs, 0, 0),
	GATE(INFRA_GCE, infra_gce, axi_sel, infra_cg_regs, 1, 0),
	GATE(INFRA_TRBG, infra_trbg, axi_sel, infra_cg_regs, 2, 0),
	GATE(INFRA_CPUM, infra_cpum, axi_sel, infra_cg_regs, 3, 0),
	GATE(INFRA_DEVAPC, infra_devapc, axi_sel, infra_cg_regs, 4, 0),
	GATE(INFRA_AUDIO, infra_audio, aud_intbus_sel, infra_cg_regs, 5, 0),
	GATE(INFRA_GCPU, infra_gcpu, axi_sel, infra_cg_regs, 6, 0),
	GATE(INFRA_L2C_SRAM, infra_l2csram, axi_sel, infra_cg_regs, 7, 0),
	GATE(INFRA_M4U, infra_m4u, axi_sel, infra_cg_regs, 8, 0),
	GATE(INFRA_CLDMA, infra_cldma, axi_sel, infra_cg_regs, 12, 0),
	GATE(INFRA_CONNMCU_BUS, infra_connmcubus, axi_sel, infra_cg_regs, 15, 0),
	GATE(INFRA_KP, infra_kp, axi_sel, infra_cg_regs, 16, 0),
	GATE(INFRA_APXGPT, infra_apxgpt, axi_sel, infra_cg_regs, 18, 0),
	GATE(INFRA_SEJ, infra_sej, axi_sel, infra_cg_regs, 19, 0),
	GATE(INFRA_CCIF0_AP, infra_ccif0ap, axi_sel, infra_cg_regs, 20, 0),
	GATE(INFRA_CCIF1_AP, infra_ccif1ap, axi_sel, infra_cg_regs, 21, 0),
	GATE(INFRA_PMIC_SPI, infra_pmicspi, pmicspi_sel, infra_cg_regs, 22, 0),
	GATE(INFRA_PMIC_WRAP, infra_pmicwrap, axi_sel, infra_cg_regs, 23, 0),
};

static void __init init_clk_infrasys(void __iomem *infrasys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init infrasys gates:\n");
	init_clk_gates(infrasys_base, infra_clks, ARRAY_SIZE(infra_clks),
		clk_data);
}

static struct mtk_gate_regs peri0_cg_regs = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x0010,
	.sta_ofs = 0x0018,
};

static struct mtk_gate peri_clks[] __initdata = {
	GATE(PERI_DISP_PWM, peri_disp_pwm, disppwm_sel, peri0_cg_regs, 0, 0),
	GATE(PERI_THERM, peri_therm, axi_sel, peri0_cg_regs, 1, 0),
	GATE(PERI_PWM1, peri_pwm1, axi_sel, peri0_cg_regs, 2, 0),
	GATE(PERI_PWM2, peri_pwm2, axi_sel, peri0_cg_regs, 3, 0),
	GATE(PERI_PWM3, peri_pwm3, axi_sel, peri0_cg_regs, 4, 0),
	GATE(PERI_PWM4, peri_pwm4, axi_sel, peri0_cg_regs, 5, 0),
	GATE(PERI_PWM5, peri_pwm5, axi_sel, peri0_cg_regs, 6, 0),
	GATE(PERI_PWM6, peri_pwm6, axi_sel, peri0_cg_regs, 7, 0),
	GATE(PERI_PWM7, peri_pwm7, axi_sel, peri0_cg_regs, 8, 0),
	GATE(PERI_PWM, peri_pwm, axi_sel, peri0_cg_regs, 9, 0),
	GATE(PERI_USB0, peri_usb0, usb20_sel, peri0_cg_regs, 10, 0),
	GATE(PERI_IRDA, peri_irda, irda_sel, peri0_cg_regs, 11, 0),
	GATE(PERI_APDMA, peri_apdma, axi_sel, peri0_cg_regs, 12, 0),
	GATE(PERI_MSDC30_0, peri_msdc30_0, msdc30_0_sel, peri0_cg_regs, 13, 0),
	GATE(PERI_MSDC30_1, peri_msdc30_1, msdc30_1_sel, peri0_cg_regs, 14, 0),
	GATE(PERI_MSDC30_2, peri_msdc30_2, msdc30_2_sel, peri0_cg_regs, 15, 0),
	GATE(PERI_MSDC30_3, peri_msdc30_3, msdc30_3_sel, peri0_cg_regs, 16, 0),
	GATE(PERI_UART0, peri_uart0, uart_sel, peri0_cg_regs, 17, 0),
	GATE(PERI_UART1, peri_uart1, uart_sel, peri0_cg_regs, 18, 0),
	GATE(PERI_UART2, peri_uart2, uart_sel, peri0_cg_regs, 19, 0),
	GATE(PERI_UART3, peri_uart3, uart_sel, peri0_cg_regs, 20, 0),
	GATE(PERI_UART4, peri_uart4, uart_sel, peri0_cg_regs, 21, 0),
	GATE(PERI_BTIF, peri_btif, axi_sel, peri0_cg_regs, 22, 0),
	GATE(PERI_I2C0, peri_i2c0, axi_sel, peri0_cg_regs, 23, 0),
	GATE(PERI_I2C1, peri_i2c1, axi_sel, peri0_cg_regs, 24, 0),
	GATE(PERI_I2C2, peri_i2c2, axi_sel, peri0_cg_regs, 25, 0),
	GATE(PERI_I2C3, peri_i2c3, axi_sel, peri0_cg_regs, 26, 0),
	GATE(PERI_AUXADC, peri_auxadc, axi_sel, peri0_cg_regs, 27, 0),
	GATE(PERI_SPI0, peri_spi0, spi_sel, peri0_cg_regs, 28, 0),
	GATE(PERI_IRTX, peri_irtx, irtx_sel, peri0_cg_regs, 29, 0),
};

static void __init init_clk_perisys(void __iomem *perisys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init perisys gates:\n");
	init_clk_gates(perisys_base, peri_clks, ARRAY_SIZE(peri_clks),
		clk_data);
}

static struct mtk_gate_regs mfg_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate mfg_clks[] __initdata = {
	GATE(MFG_BG3D, mfg_bg3d, mfg_sel, mfg_cg_regs, 0, 0),
};

static void __init init_clk_mfgsys(void __iomem *mfgsys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mfgsys gates:\n");
	init_clk_gates(mfgsys_base, mfg_clks, ARRAY_SIZE(mfg_clks), clk_data);
}

static struct mtk_gate_regs img_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate img_clks[] __initdata = {
	GATE(IMG_IMAGE_LARB2_SMI, img_image_larb2_smi, mm_sel, img_cg_regs, 0, 0),
	GATE(IMG_IMAGE_CAM_SMI, img_image_cam_smi, mm_sel, img_cg_regs, 5, 0),
	GATE(IMG_IMAGE_CAM_CAM, img_image_cam_cam, mm_sel, img_cg_regs, 6, 0),
	GATE(IMG_IMAGE_SEN_TG, img_image_sen_tg, mm_sel, img_cg_regs, 7, 0),
	GATE(IMG_IMAGE_SEN_CAM, img_image_sen_cam, camtg_sel, img_cg_regs, 8, 0),
	GATE(IMG_IMAGE_CAM_SV, img_image_cam_sv, mm_sel, img_cg_regs, 9, 0),
	GATE(IMG_IMAGE_SUFOD, img_image_sufod, mm_sel, img_cg_regs, 10, 0),
	GATE(IMG_IMAGE_FD, img_image_fd, mm_sel, img_cg_regs, 11, 0),
};

static void __init init_clk_imgsys(void __iomem *imgsys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init imgsys gates:\n");
	init_clk_gates(imgsys_base, img_clks, ARRAY_SIZE(img_clks), clk_data);
}

static struct mtk_gate_regs mm0_cg_regs = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static struct mtk_gate_regs mm1_cg_regs = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

static struct mtk_gate mm_clks[] __initdata = {
	/* MM0 */
	GATE(MM_DISP0_SMI_COMMON, mm_disp0_smi_common, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_LARB0, mm_disp0_smi_larb0, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_DISP0_CAM_MDP, mm_disp0_cam_mdp, mm_sel, mm0_cg_regs, 2, 0),
	GATE(MM_DISP0_MDP_RDMA, mm_disp0_mdp_rdma, mm_sel, mm0_cg_regs, 3, 0),
	GATE(MM_DISP0_MDP_RSZ0, mm_disp0_mdp_rsz0, mm_sel, mm0_cg_regs, 4, 0),
	GATE(MM_DISP0_MDP_RSZ1, mm_disp0_mdp_rsz1, mm_sel, mm0_cg_regs, 5, 0),
	GATE(MM_DISP0_MDP_TDSHP, mm_disp0_mdp_tdshp, mm_sel, mm0_cg_regs, 6, 0),
	GATE(MM_DISP0_MDP_WDMA, mm_disp0_mdp_wdma, mm_sel, mm0_cg_regs, 7, 0),
	GATE(MM_DISP0_MDP_WROT, mm_disp0_mdp_wrot, mm_sel, mm0_cg_regs, 8, 0),
	GATE(MM_DISP0_FAKE_ENG, mm_disp0_fake_eng, mm_sel, mm0_cg_regs, 9, 0),
	GATE(MM_DISP0_DISP_OVL0, mm_disp0_disp_ovl0, mm_sel, mm0_cg_regs, 10, 0),
	GATE(MM_DISP0_DISP_RDMA0, mm_disp0_disp_rdma0, mm_sel, mm0_cg_regs, 11, 0),
	GATE(MM_DISP0_DISP_RDMA1, mm_disp0_disp_rdma1, mm_sel, mm0_cg_regs, 12, 0),
	GATE(MM_DISP0_DISP_WDMA0, mm_disp0_disp_wdma0, mm_sel, mm0_cg_regs, 13, 0),
	GATE(MM_DISP0_DISP_COLOR, mm_disp0_disp_color, mm_sel, mm0_cg_regs, 14, 0),
	GATE(MM_DISP0_DISP_CCORR, mm_disp0_disp_ccorr, mm_sel, mm0_cg_regs, 15, 0),
	GATE(MM_DISP0_DISP_AAL, mm_disp0_disp_aal, mm_sel, mm0_cg_regs, 16, 0),
	GATE(MM_DISP0_DISP_GAMMA, mm_disp0_disp_gamma, mm_sel, mm0_cg_regs, 17, 0),
	GATE(MM_DISP0_DISP_DITHER, mm_disp0_disp_dither, mm_sel, mm0_cg_regs, 18, 0),
	/* MM1 */
	GATE(MM_DISP1_DSI_ENGINE, mm_disp1_dsi_engine, mm_sel, mm1_cg_regs, 2, 0),
	GATE(MM_DISP1_DSI_DIGITAL, mm_disp1_dsi_digital, mm_sel, mm1_cg_regs, 3, 0),
	GATE(MM_DISP1_DPI_ENGINE, mm_disp1_dpi_engine, mm_sel, mm1_cg_regs, 4, 0),
	GATE(MM_DISP1_DPI_PIXEL, mm_disp1_dpi_pixel, dpi0_sel, mm1_cg_regs, 5, 0),
};

static void __init init_clk_mmsys(void __iomem *mmsys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mmsys gates:\n");
	init_clk_gates(mmsys_base, mm_clks, ARRAY_SIZE(mm_clks),
		clk_data);
}

static struct mtk_gate_regs vdec0_cg_regs = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0000,
};

static struct mtk_gate_regs vdec1_cg_regs = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x000c,
	.sta_ofs = 0x0008,
};

static struct mtk_gate vdec_clks[] __initdata = {
	GATE(VDEC0_VDEC, vdec0_vdec, vdec_sel, vdec0_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC1_LARB, vdec1_larb, vdec_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
};

static void __init init_clk_vdecsys(void __iomem *vdecsys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vdecsys gates:\n");
	init_clk_gates(vdecsys_base, vdec_clks, ARRAY_SIZE(vdec_clks),
		clk_data);
}

static struct mtk_gate_regs venc_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate venc_clks[] __initdata = {
	GATE(VENC_LARB, venc_larb, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_VENC, venc_venc, mm_sel, venc_cg_regs, 4, CLK_GATE_INVERSE),
	GATE(VENC_JPGENC, venc_jpgenc, mm_sel, venc_cg_regs, 8, CLK_GATE_INVERSE),
	GATE(VENC_JPGDEC, venc_jpgdec, mm_sel, venc_cg_regs, 12, CLK_GATE_INVERSE),
};

static void __init init_clk_vencsys(void __iomem *vencsys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vencsys gates:\n");
	init_clk_gates(vencsys_base, venc_clks, ARRAY_SIZE(venc_clks),
		clk_data);
}

static struct mtk_gate_regs aud_cg_regs = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0000,
	.sta_ofs = 0x0000,
};

static struct mtk_gate audio_clks[] __initdata = {
	GATE(AUDIO_AFE, audio_afe, aud_intbus_sel, aud_cg_regs, 2, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_I2S, audio_i2s, aud_intbus_sel, aud_cg_regs, 6, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_22M, audio_22m, aud_intbus_sel, aud_cg_regs, 8, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_24M, audio_24m, aud_intbus_sel, aud_cg_regs, 9, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL2_TUNER, audio_apll2_tuner, aud_intbus_sel, aud_cg_regs, 18, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL_TUNER, audio_apll_tuner, aud_intbus_sel, aud_cg_regs, 19, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_ADC, audio_adc, aud_intbus_sel, aud_cg_regs, 24, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC, audio_dac, aud_intbus_sel, aud_cg_regs, 25, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC_PREDIS, audio_dac_predis, aud_intbus_sel, aud_cg_regs, 26, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_TML, audio_tml, aud_intbus_sel, aud_cg_regs, 27, CLK_GATE_NO_SETCLR_REG),
};

static void __init init_clk_audiosys(void __iomem *audiosys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init audiosys gates:\n");
	init_clk_gates(audiosys_base, audio_clks, ARRAY_SIZE(audio_clks),
		clk_data);
}

/*
 * device tree support
 */

static struct clk_onecell_data *alloc_clk_data(unsigned int clk_num)
{
	int i;
	struct clk_onecell_data *clk_data;

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return NULL;

	clk_data->clks = kcalloc(clk_num, sizeof(struct clk *), GFP_KERNEL);
	if (!clk_data->clks) {
		kfree(clk_data);
		return NULL;
	}

	clk_data->clk_num = clk_num;

	for (i = 0; i < clk_num; ++i)
		clk_data->clks[i] = ERR_PTR(-ENOENT);

	return clk_data;
}

static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

static void __init mt_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap topckgen failed\n");
		return;
	}

	clk_data = alloc_clk_data(TOP_NR_CLK);

	init_clk_root_alias(clk_data);
	init_clk_top_div(clk_data);
	init_clk_topckgen(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt6735-topckgen", mt_topckgen_init);

static void __init mt_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap apmixedsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(APMIXED_NR_CLK);

	init_clk_apmixedsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt6735-apmixedsys",
		mt_apmixedsys_init);

static void __init mt_infrasys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap infrasys failed\n");
		return;
	}

	clk_data = alloc_clk_data(INFRA_NR_CLK);

	init_clk_infrasys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_infrasys, "mediatek,mt6735-infrasys", mt_infrasys_init);

static void __init mt_perisys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap perisys failed\n");
		return;
	}

	clk_data = alloc_clk_data(PERI_NR_CLK);

	init_clk_perisys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_perisys, "mediatek,mt6735-perisys", mt_perisys_init);

static void __init mt_mfgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap mfgsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(MFG_NR_CLK);

	init_clk_mfgsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_mfgsys, "mediatek,mt6735-mfgsys", mt_mfgsys_init);

static void __init mt_imgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap imgsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(IMG_NR_CLK);

	init_clk_imgsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_imgsys, "mediatek,mt6735-imgsys", mt_imgsys_init);

static void __init mt_mmsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap mmsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(MM_NR_CLK);

	init_clk_mmsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_mmsys, "mediatek,mt6735-mmsys", mt_mmsys_init);

static void __init mt_vdecsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap vdecsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(VDEC_NR_CLK);

	init_clk_vdecsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_vdecsys, "mediatek,mt6735-vdecsys", mt_vdecsys_init);

static void __init mt_vencsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap vencsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(VENC_NR_CLK);

	init_clk_vencsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_vencsys, "mediatek,mt6735-vencsys", mt_vencsys_init);

static void __init mt_audiosys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap audiosys failed\n");
		return;
	}

	clk_data = alloc_clk_data(AUDIO_NR_CLK);

	init_clk_audiosys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_audiosys, "mediatek,mt6735-audiosys", mt_audiosys_init);

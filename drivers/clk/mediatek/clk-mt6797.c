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
#include "clk-mt6797-pll.h"
#define _MUX_CLR_SET_UPDS_
#define _MUX_UPDS_
#ifdef _MUX_UPDS_
#include "clk-mux.h"
#endif

#include <dt-bindings/clock/mt6797-clk.h>

#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP	0
#endif


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

static DEFINE_SPINLOCK(mt6797_clk_lock);

/*
 * platform clocks
 */

/* ROOT */
#define clk_null		"clk_null"
#define clk26m			"clk26m"
/* #define clkph_mck_o           "clkph_mck_o" */
#define ulposc          "ulposc"

/* PLL */
#define mainpll		"mainpll"
#define univpll		"univpll"
#define mfgpll		"mfgpll"
#define msdcpll		"msdcpll"
#define imgpll		"imgpll"
#define tvdpll		"tvdpll"
#define mpll		"mpll"
#define codecpll	"codecpll"
#define vdecpll		"vdecpll"
#define apll1		"apll1"
#define apll2		"apll2"


/* DIV */
#define dmpll_ck	"dmpll_ck"
#define dmpll_d2	"dmpll_d2"
#define dmpll_d4	"dmpll_d4"
#define dmpll_d8	"dmpll_d8"
#define dmpll_d16	"dmpll_d16"
#define syspll_ck	"syspll_ck"
#define syspll_d2	"syspll_d2"
#define syspll1_d2	"syspll1_d2"
#define syspll1_d4	"syspll1_d4"
#define syspll1_d8	"syspll1_d8"
#define syspll1_d16	"syspll1_d16"
#define syspll_d3	"syspll_d3"
#define syspll_d3_d3	"syspll_d3_d3"
#define syspll2_d2	"syspll2_d2"
#define syspll2_d4	"syspll2_d4"
#define syspll2_d8	"syspll2_d8"
#define syspll_d5	"syspll_d5"
#define syspll3_d2	"syspll3_d2"
#define syspll3_d4	"syspll3_d4"
#define syspll_d7	"syspll_d7"
#define syspll4_d2	"syspll4_d2"
#define syspll4_d4	"syspll4_d4"
#define univpll_ck	"univpll_ck"
#define univpll_d7	"univpll_d7"
#define univpll_d26	"univpll_d26"
#define ssusb_phy_48m_ck	"ssusb_phy_48m_ck"
#define usb_phy48m_ck	"usb_phy48m_ck"
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
#define univpll3_d8	"univpll3_d8"
#define ulposc_ck_org	"ulposc_ck_org"
#define ulposc_ck	"ulposc_ck"
#define ulposc_d2	"ulposc_d2"
#define ulposc_d3	"ulposc_d3"
#define ulposc_d4	"ulposc_d4"
#define ulposc_d8	"ulposc_d8"
#define ulposc_d10	"ulposc_d10"
#define apll1_ck	"apll1_ck"
#define apll2_ck	"apll2_ck"
#define mfgpll_ck	"mfgpll_ck"
#define mfgpll_d2	"mfgpll_d2"
#define imgpll_ck	"imgpll_ck"
#define imgpll_d2	"imgpll_d2"
#define imgpll_d4	"imgpll_d4"
#define codecpll_ck	"codecpll_ck"
#define codecpll_d2	"codecpll_d2"
#define vdecpll_ck	"vdecpll_ck"
#define tvdpll_ck	"tvdpll_ck"
#define tvdpll_d2	"tvdpll_d2"
#define tvdpll_d4	"tvdpll_d4"
#define tvdpll_d8	"tvdpll_d8"
#define tvdpll_d16	"tvdpll_d16"
#define msdcpll_ck	"msdcpll_ck"
#define msdcpll_d2	"msdcpll_d2"
#define msdcpll_d4	"msdcpll_d4"
#define msdcpll_d8	"msdcpll_d8"

/* TOP */
#define axi_sel            "axi_sel"
#define ulposc_axi_ck_mux        "ulposc_axi_ck_mux"
#define ulposc_axi_ck_mux_pre    "ulposc_axi_ck_mux_pre"
#define mem_sel            "mem_sel"
#define ddrphycfg_sel      "ddrphycfg_sel"
#define mm_sel             "mm_sel"
#define pwm_sel            "pwm_sel"
#define vdec_sel           "vdec_sel"
#define venc_sel           "venc_sel"
#define mfg_sel            "mfg_sel"
#define camtg_sel          "camtg_sel"
#define uart_sel           "uart_sel"
#define spi_sel            "spi_sel"
#define ulposc_spi_ck_mux  "ulposc_spi_ck_mux"
#define usb20_sel          "usb20_sel"
#define msdc50_0_hclk_sel  "msdc50_0_hclk_sel"
#define msdc50_0_sel       "msdc50_0_sel"
#define msdc30_1_sel       "msdc30_1_sel"
#define msdc30_2_sel       "msdc30_2_sel"
#define audio_sel          "audio_sel"
#define aud_intbus_sel     "aud_intbus_sel"
#define pmicspi_sel        "pmicspi_sel"
#define scp_sel            "scp_sel"
#define atb_sel            "atb_sel"
#define mjc_sel            "mjc_sel"
#define dpi0_sel           "dpi0_sel"
#define aud_1_sel          "aud_1_sel"
#define aud_2_sel          "aud_2_sel"
#define ssusb_top_sys_sel  "ssusb_top_sys_sel"
#define spm_sel            "spm_sel"
#define bsi_spi_sel        "bsi_spi_sel"
#define audio_h_sel        "audio_h_sel"
#define anc_md32_sel       "anc_md32_sel"
#define mfg_52m_sel        "mfg_52m_sel"
/*--------------------------------------------------------------*/
#define f26m_sel		"f26m_sel"


/* INFRA */
#define	infra_pmic_tmr           "infra_pmic_tmr"
#define	infra_pmic_ap            "infra_pmic_ap"
#define	infra_pmic_md            "infra_pmic_md"
#define	infra_pmic_conn          "infra_pmic_conn"
#define	infra_scp                "infra_scp"
#define	infra_sej                "infra_sej"
#define	infra_apxgpt             "infra_apxgpt"
#define	infra_sej_13m            "infra_sej_13m"
#define	infra_icusb              "infra_icusb"
#define	infra_gce                "infra_gce"
#define	infra_therm              "infra_therm"
#define	infra_i2c0               "infra_i2c0"
#define	infra_i2c1               "infra_i2c1"
#define	infra_i2c2               "infra_i2c2"
#define	infra_i2c3               "infra_i2c3"
#define	infra_pwm_hclk           "infra_pwm_hclk"
#define	infra_pwm1               "infra_pwm1"
#define	infra_pwm2               "infra_pwm2"
#define	infra_pwm3               "infra_pwm3"
#define	infra_pwm4               "infra_pwm4"
#define	infra_pwm                "infra_pwm"
#define	infra_uart0              "infra_uart0"
#define	infra_uart1              "infra_uart1"
#define	infra_uart2              "infra_uart2"
#define	infra_uart3              "infra_uart3"
#define	infra_md2md_ccif_0       "infra_md2md_ccif_0"
#define	infra_md2md_ccif_1       "infra_md2md_ccif_1"
#define	infra_md2md_ccif_2       "infra_md2md_ccif_2"
#define	infra_fhctl              "infra_fhctl"
#define	infra_btif               "infra_btif"
#define	infra_md2md_ccif_3       "infra_md2md_ccif_3"
#define	infra_spi                "infra_spi"
#define	infra_msdc0              "infra_msdc0"
#define	infra_md2md_ccif_4       "infra_md2md_ccif_4"
#define	infra_msdc1              "infra_msdc1"
#define	infra_msdc2              "infra_msdc2"
#define	infra_md2md_ccif_5       "infra_md2md_ccif_5"
#define	infra_gcpu               "infra_gcpu"
#define	infra_trng               "infra_trng"
#define	infra_auxadc             "infra_auxadc"
#define	infra_cpum               "infra_cpum"
#define	infra_ap_c2k_ccif_0      "infra_ap_c2k_ccif_0"
#define	infra_ap_c2k_ccif_1      "infra_ap_c2k_ccif_1"
#define	infra_cldma              "infra_cldma"
#define	infra_disp_pwm           "infra_disp_pwm"
#define	infra_ap_dma             "infra_ap_dma"
#define	infra_device_apc         "infra_device_apc"
#define	infra_l2c_sram           "infra_l2c_sram"
#define	infra_ccif_ap            "infra_ccif_ap"
#define	infra_debugsys           "infra_debugsys"
#define	infra_audio              "infra_audio"
#define	infra_ccif_md            "infra_ccif_md"
#define	infra_dramc_f26m         "infra_dramc_f26m"
#define	infra_i2c4               "infra_i2c4"
#define	infra_i2c_appm           "infra_i2c_appm"
#define	infra_i2c_gpupm          "infra_i2c_gpupm"
#define	infra_i2c2_imm           "infra_i2c2_imm"
#define	infra_i2c2_arb           "infra_i2c2_arb"
#define	infra_i2c3_imm           "infra_i2c3_imm"
#define	infra_i2c3_arb           "infra_i2c3_arb"
#define	infra_i2c5               "infra_i2c5"
#define	infra_sys_cirq           "infra_sys_cirq"
#define	infra_spi1               "infra_spi1"
#define	infra_dramc_b_f26m       "infra_dramc_b_f26m"
#define	infra_anc_md32           "infra_anc_md32"
#define	infra_anc_md32_32k       "infra_anc_md32_32k"
#define	infra_dvfs_spm1          "infra_dvfs_spm1"
#define	infra_aes_top0           "infra_aes_top0"
#define	infra_aes_top1           "infra_aes_top1"
#define	infra_ssusb_bus          "infra_ssusb_bus"
#define	infra_spi2               "infra_spi2"
#define	infra_spi3               "infra_spi3"
#define	infra_spi4               "infra_spi4"
#define	infra_spi5               "infra_spi5"
#define	infra_irtx               "infra_irtx"
#define	infra_ssusb_sys          "infra_ssusb_sys"
#define	infra_ssusb_ref          "infra_ssusb_ref"
#define	infra_audio_26m          "infra_audio_26m"
#define	infra_audio_26m_pad_top  "infra_audio_26m_pad_top"
#define	infra_modem_temp_share   "infra_modem_temp_share"
#define	infra_vad_wrap_soc       "infra_vad_wrap_soc"
#define	infra_dramc_conf         "infra_dramc_conf"
#define	infra_dramc_b_conf       "infra_dramc_b_conf"


/* MFG */
#define mfg_bg3d			"mfg_bg3d"
#define infra_mfg_vcg			"infra_mfg_vcg"

/* IMG */
#define	img_fdvt  "img_fdvt"
#define	img_dpe   "img_dpe"
#define	img_dip   "img_dip"
#define	img_larb6 "img_larb6"

/* CAM */
#define	cam_camsv2 "cam_camsv2"
#define	cam_camsv1 "cam_camsv1"
#define	cam_camsv0 "cam_camsv0"
#define	cam_seninf "cam_seninf"
#define	cam_camtg  "cam_camtg"
#define	cam_camsys "cam_camsys"
#define	cam_larb2  "cam_larb2"


/* MM_SYS */
#define	mm_smi_common               "mm_smi_common"
#define	mm_smi_larb0                "mm_smi_larb0"
#define	mm_smi_larb5                "mm_smi_larb5"
#define	mm_cam_mdp                  "mm_cam_mdp"
#define	mm_mdp_rdma0                "mm_mdp_rdma0"
#define	mm_mdp_rdma1                "mm_mdp_rdma1"
#define	mm_mdp_rsz0                 "mm_mdp_rsz0"
#define	mm_mdp_rsz1                 "mm_mdp_rsz1"
#define	mm_mdp_rsz2                 "mm_mdp_rsz2"
#define	mm_mdp_tdshp                "mm_mdp_tdshp"
#define	mm_mdp_color                "mm_mdp_color"
#define	mm_mdp_wdma                 "mm_mdp_wdma"
#define	mm_mdp_wrot0                "mm_mdp_wrot0"
#define	mm_mdp_wrot1                "mm_mdp_wrot1"
#define	mm_fake_eng                 "mm_fake_eng"
#define	mm_disp_ovl0                "mm_disp_ovl0"
#define	mm_disp_ovl1                "mm_disp_ovl1"
#define	mm_disp_ovl0_2l             "mm_disp_ovl0_2l"
#define	mm_disp_ovl1_2l             "mm_disp_ovl1_2l"
#define	mm_disp_rdma0               "mm_disp_rdma0"
#define	mm_disp_rdma1               "mm_disp_rdma1"
#define	mm_disp_wdma0               "mm_disp_wdma0"
#define	mm_disp_wdma1               "mm_disp_wdma1"
#define	mm_disp_color               "mm_disp_color"
#define	mm_disp_ccorr               "mm_disp_ccorr"
#define	mm_disp_aal                 "mm_disp_aal"
#define	mm_disp_gamma               "mm_disp_gamma"
#define	mm_disp_od                  "mm_disp_od"
#define	mm_disp_dither              "mm_disp_dither"
#define	mm_disp_ufoe                "mm_disp_ufoe"
#define	mm_disp_dsc                 "mm_disp_dsc"
#define	mm_disp_split               "mm_disp_split"
#define	mm_dsi0_mm_clock            "mm_dsi0_mm clock"
#define	mm_dsi1_mm_clock            "mm_dsi1_mm_clock"
#define	mm_dpi_mm_clock             "mm_dpi_mm_clock"
#define	mm_dpi_interface_clock      "mm_dpi_interface_clock"
#define	mm_larb4_axi_asif_mm_clock  "mm_larb4_axi_asif_mm_clock"
#define	mm_larb4_axi_asif_mjc_clock "mm_larb4_axi_asif_mjc_clock"
#define	mm_disp_ovl0_mout_clock     "mm_disp_ovl0_mout_clock"
#define	mm_fake_eng2                "mm_fake_eng2"
#define	mm_dsi0_interface_clock	"mm_dsi0_interface_clock"
#define	mm_dsi1_interface_clock	"mm_dsi1_interface_clock"

/* MJC */
#define	mjc_smi_larb    "mjc_smi_larb"
#define	mjc_top_clk_0   "mjc_top_clk_0"
#define	mjc_top_clk_1   "mjc_top_clk_1"
#define	mjc_top_clk_2   "mjc_top_clk_2"
#define	mjc_fake_engine "mjc_fake_engine"
#define	mjc_larb4_asif  "mjc_larb4_asif"

/* VDEC */
#define	vdec_cken_eng   "vdec_cken_eng"
#define	vdec_active     "vdec_active"
#define	vdec_cken       "vdec_cken"
#define	vdec_larb1_cken "vdec_larb1_cken"

/* VENC */
#define	venc_0  "venc_0"
#define	venc_1  "venc_1"
#define	venc_2  "venc_2"
#define	venc_3  "venc_3"

/* AUDIO */
#define	audio_pdn_tml         "audio_pdn_tml"
#define	audio_pdn_dac_predis  "audio_pdn_dac_predis"
#define	audio_pdn_dac         "audio_pdn_dac"
#define	audio_pdn_adc         "audio_pdn_adc"
#define	audio_pdn_apll_tuner  "audio_pdn_apll_tuner"
#define	audio_pdn_apll2_tuner "audio_pdn_apll2_tuner"
#define	audio_pdn_24m         "audio_pdn_24m"
#define	audio_pdn_22m         "audio_pdn_22m"
#define	audio_pdn_afe         "audio_pdn_afe"
#define	audio_pdn_adc_hires_tml "audio_pdn_adc_hires_tml"
#define	audio_pdn_adc_hires "audio_pdn_adc_hires"

#ifdef CONFIG_OF
void __iomem  *cksys_base;
void __iomem  *infracfg_base;
void __iomem  *audio_base;
void __iomem  *mfgcfg_base;
void __iomem  *mmsys_config_base;
void __iomem  *img_base;
void __iomem  *vdec_gcon_base;
void __iomem  *venc_gcon_base;
void __iomem  *mjcsys_base;
void __iomem  *camsys_base;

#define INFRA_PDN_SET0          (infracfg_base + 0x0080)
#define INFRA_PDN_SET1          (infracfg_base + 0x0088)
#define INFRA_PDN_SET2          (infracfg_base + 0x00A8)
#define MFG_CG_SET              (mfgcfg_base + 4)
#define IMG_CG_SET              (img_base + 0x0004)
#define MM_CG_SET0            (mmsys_config_base + 0x104)
#define MM_CG_SET1            (mmsys_config_base + 0x114)
#define MM_DUMMY_CG_SET0            (mmsys_config_base + 0x894)
#define MM_DUMMY_CG_SET1            (mmsys_config_base + 0x898)
#define VDEC_CKEN_CLR           (vdec_gcon_base + 0x0004)
#define LARB_CKEN_CLR           (vdec_gcon_base + 0x000C)
#define VENC_CG_CON             (venc_gcon_base + 0x0)
#define AUDIO_TOP_CON0          (audio_base + 0x0000)
#define AUDIO_TOP_CON1          (audio_base + 0x0004)
#define MJC_CG_SET				(mjcsys_base + 0x0008)
#define CAMSYS_CG_SET			(camsys_base + 0x0004)
#endif

#define INFRA0_CG  0x87BFFD00/*0: Disable  ( with clock), 1: Enable ( without clock )*/
#define INFRA1_CG  0x0244CA76/*0: Disable  ( with clock), 1: Enable ( without clock )*/
#define INFRA2_CG  0x2DFE36FF/*0: Disable  ( with clock), 1: Enable ( without clock ), 9:dummy for usb ref*/
#define AUD_0_CG   0x0F0C0304
#define AUD_1_CG   0x00030000
#define MFG_CG     0x00000001/*set*/
#define MM_0_CG   0xFFFFFFF8
#define MM_1_CG   0x000003F0
#define MM_DUMMY_0_CG   0xFFFFBFF8/*function on off*/
#define MM_DUMMY_1_CG   0x00000120/*function on off*/
#define IMG_CG     0x00000C41
#define VDEC_CG    0x00000111/*set*/
#define LARB_CG    0x00000001
#define VENC_CG    0x00001111/*set*/
#define MJC_CG     0x0000001F
#define CAM_CG	   0x00000FC1

#define CG_BOOTUP_PDN			1


#define INFRA_BUS_DCM_CTRL_OFS (0x70)
#define MSDCPLL_CON0_OFS (0x250)
#define MSDCPLL_PWR_CON0_OFS (0x25c)
#define TVDPLL_CON0_OFS (0x270)
#define TVDPLL_PWR_CON0_OFS (0x27c)
#define APLL1_CON0_OFS (0x2A0)
#define APLL1_PWR_CON0_OFS (0x2B0)
#define APLL2_CON0_OFS (0x2B4)
#define APLL2_PWR_CON0_OFS (0x2C4)


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
			pr_err("Failed to register clk %s: %ld\n", ff->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[ff->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] factor %3d: %s\n", i, ff->name);
#endif				/* MT_CCF_DEBUG */
	}
}

static struct mtk_fixed_factor root_clk_alias[] __initdata = {
	/* FACTOR(TOP_CLKPH_MCK_O, clkph_mck_o, clk_null, 1, 1), */
	FACTOR(TOP_ULPOSC_CK_ORG, ulposc, clk_null, 1, 1),
};

static void __init init_clk_root_alias(struct clk_onecell_data *clk_data)
{
	init_factors(root_clk_alias, ARRAY_SIZE(root_clk_alias), clk_data);
}

static struct mtk_fixed_factor top_divs[] __initdata = {
	FACTOR(TOP_SYSPLL_CK, syspll_ck, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL_D2, syspll_d2, mainpll, 1, 2),
	FACTOR(TOP_SYSPLL1_D2, syspll1_d2, syspll_d2, 1, 2),
	FACTOR(TOP_SYSPLL1_D4, syspll1_d4, syspll_d2, 1, 4),
	FACTOR(TOP_SYSPLL1_D8, syspll1_d8, syspll_d2, 1, 8),
	FACTOR(TOP_SYSPLL1_D16, syspll1_d16, syspll_d2, 1, 16),
	FACTOR(TOP_SYSPLL_D3, syspll_d3, mainpll, 1, 3),
	FACTOR(TOP_SYSPLL_D3_D3, syspll_d3_d3, syspll_d3, 1, 3),
	FACTOR(TOP_SYSPLL2_D2, syspll2_d2, syspll_d3, 1, 2),
	FACTOR(TOP_SYSPLL2_D4, syspll2_d4, syspll_d3, 1, 4),
	FACTOR(TOP_SYSPLL2_D8, syspll2_d8, syspll_d3, 1, 8),
	FACTOR(TOP_SYSPLL_D5, syspll_d5, mainpll, 1, 5),
	FACTOR(TOP_SYSPLL3_D2, syspll3_d2, syspll_d5, 1, 2),
	FACTOR(TOP_SYSPLL3_D4, syspll3_d4, syspll_d5, 1, 4),
	FACTOR(TOP_SYSPLL_D7, syspll_d7, mainpll, 1, 7),
	FACTOR(TOP_SYSPLL4_D2, syspll4_d2, syspll_d7, 1, 2),
	FACTOR(TOP_SYSPLL4_D4, syspll4_d4, syspll_d7, 1, 4),
	FACTOR(TOP_UNIVPLL_CK, univpll_ck, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL_D7, univpll_d7, univpll, 1, 7),
	FACTOR(TOP_UNIVPLL_D26, univpll_d26, univpll, 1, 26),
	FACTOR(TOP_SSUSB_PHY_48M_CK, ssusb_phy_48m_ck, univpll, 1, 1),	/*  todo: check later */
	FACTOR(TOP_USB_PHY48M_CK, usb_phy48m_ck, univpll, 1, 1),	/*  todo: check later */
	FACTOR(TOP_UNIVPLL_D2, univpll_d2, univpll, 1, 2),
	FACTOR(TOP_UNIVPLL1_D2, univpll1_d2, univpll_d2, 1, 2),
	FACTOR(TOP_UNIVPLL1_D4, univpll1_d4, univpll_d2, 1, 4),
	FACTOR(TOP_UNIVPLL1_D8, univpll1_d8, univpll_d2, 1, 8),
	FACTOR(TOP_UNIVPLL_D3, univpll_d3, univpll, 1, 3),
	FACTOR(TOP_UNIVPLL2_D2, univpll2_d2, univpll, 1, 2),
	FACTOR(TOP_UNIVPLL2_D4, univpll2_d4, univpll, 1, 4),
	FACTOR(TOP_UNIVPLL2_D8, univpll2_d8, univpll, 1, 8),
	FACTOR(TOP_UNIVPLL_D5, univpll_d5, univpll, 1, 5),
	FACTOR(TOP_UNIVPLL3_D2, univpll3_d2, univpll_d5, 1, 2),
	FACTOR(TOP_UNIVPLL3_D4, univpll3_d4, univpll_d5, 1, 4),
	FACTOR(TOP_UNIVPLL3_D8, univpll3_d8, univpll_d5, 1, 8),
	FACTOR(TOP_ULPOSC_CK_ORG, ulposc_ck_org, ulposc, 1, 1),	/*  todo: check later */
	FACTOR(TOP_ULPOSC_CK, ulposc_ck, ulposc_ck_org, 1, 3),
	FACTOR(TOP_ULPOSC_D2, ulposc_d2, ulposc_ck, 1, 2),
	FACTOR(TOP_ULPOSC_D3, ulposc_d3, ulposc_ck, 1, 4),
	FACTOR(TOP_ULPOSC_D4, ulposc_d4, ulposc_ck, 1, 8),
	FACTOR(TOP_ULPOSC_D8, ulposc_d8, ulposc_ck, 1, 10),
	FACTOR(TOP_ULPOSC_D10, ulposc_d10, ulposc_ck_org, 1, 1),
	FACTOR(TOP_APLL1_CK, apll1_ck, apll1, 1, 1),
	FACTOR(TOP_APLL2_CK, apll2_ck, apll2, 1, 1),
	FACTOR(TOP_MFGPLL_CK, mfgpll_ck, mfgpll, 1, 1),
	FACTOR(TOP_MFGPLL_D2, mfgpll_d2, mfgpll_ck, 1, 2),
	FACTOR(TOP_IMGPLL_CK, imgpll_ck, imgpll, 1, 1),
	FACTOR(TOP_IMGPLL_D2, imgpll_d2, imgpll_ck, 1, 2),
	FACTOR(TOP_IMGPLL_D4, imgpll_d4, imgpll_ck, 1, 4),
	FACTOR(TOP_CODECPLL_CK, codecpll_ck, codecpll, 1, 1),
	FACTOR(TOP_CODECPLL_D2, codecpll_d2, codecpll_ck, 1, 2),
	FACTOR(TOP_VDECPLL_CK, vdecpll_ck, vdecpll, 1, 1),
	FACTOR(TOP_TVDPLL_CK, tvdpll_ck, tvdpll, 1, 1),
	FACTOR(TOP_TVDPLL_D2, tvdpll_d2, tvdpll_ck, 1, 2),
	FACTOR(TOP_TVDPLL_D4, tvdpll_d4, tvdpll_ck, 1, 4),
	FACTOR(TOP_TVDPLL_D8, tvdpll_d8, tvdpll_ck, 1, 8),
	FACTOR(TOP_TVDPLL_D16, tvdpll_d16, tvdpll_ck, 1, 16),
	FACTOR(TOP_MSDCPLL_CK, msdcpll_ck, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D2, msdcpll_d2, msdcpll_ck, 1, 2),
	FACTOR(TOP_MSDCPLL_D4, msdcpll_d4, msdcpll_ck, 1, 4),
	FACTOR(TOP_MSDCPLL_D8, msdcpll_d8, msdcpll_ck, 1, 8),
};

static void __init init_clk_top_div(struct clk_onecell_data *clk_data)
{
	init_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
}

static const char *axi_parents[] __initconst = {
	clk26m,
	syspll_d7,
	ulposc_axi_ck_mux,
};

static const char *ulposc_axi_ck_mux_parents[] __initconst = {
	syspll1_d4,
	ulposc_axi_ck_mux_pre,
};

static const char *ulposc_axi_ck_mux_pre_parents[] __initconst = {
	ulposc_d2,
	ulposc_d3,
};

static const char *mem_parents[] __initconst = {
	clk26m,
	dmpll_ck,
};

static const char *ddrphycfg_parents[] __initconst = {
	clk26m,
	syspll3_d2,
	syspll2_d4,
	syspll1_d8,
};

static const char *mm_parents[] __initconst = {
	clk26m,
	imgpll_ck,
	univpll1_d2,
	syspll1_d2,
};

static const char *pwm_parents[] __initconst = {
	clk26m,
	univpll2_d4,
	ulposc_d2,
	ulposc_d3,
	ulposc_d8,
	ulposc_d10,
	ulposc_d4,
};

static const char *vdec_parents[] __initconst = {
	clk26m,
	vdecpll_ck,
	imgpll_ck,
	syspll_d3,
	univpll_d5,
	clk26m,
	clk26m,
};

static const char *venc_parents[] __initconst = {
	clk26m,
	codecpll_ck,
	syspll_d3,
};

static const char *mfg_parents[] __initconst = {
	clk26m,
	mfgpll_ck,
	syspll_d3,
	univpll_d3,
};

static const char *camtg[] __initconst = {
	clk26m,
	univpll_d26,
	univpll2_d2,
};

static const char *uart_parents[] __initconst = {
	clk26m,
	univpll2_d8,
};

static const char *spi_parents[] __initconst = {
	clk26m,
	syspll3_d2,
	syspll2_d4,
	ulposc_spi_ck_mux,
};

static const char *ulposc_spi_ck_mux_parents[] __initconst = {
	ulposc_d2,
	ulposc_d3,
};

static const char *usb20_parents[] __initconst = {
	clk26m,
	univpll1_d8,
	syspll4_d2,
};

static const char *msdc50_0_hclk_parents[] __initconst = {
	clk26m,
	syspll1_d2,
	syspll2_d2,
	syspll4_d2,
};

static const char *msdc50_0_parents[] __initconst = {
	clk26m,
	msdcpll,
	syspll_d3,
	univpll1_d4,
	syspll2_d2,
	syspll_d7,
	msdcpll_d2,
	univpll1_d2,
	univpll_d3,
};

static const char *msdc30_1_parents[] __initconst = {
	clk26m,
	univpll2_d2,
	msdcpll_d2,
	univpll1_d4,
	syspll2_d2,
	syspll_d7,
	univpll_d7,
};

static const char *msdc30_2_parents[] __initconst = {
	clk26m,
	univpll2_d8,
	syspll2_d8,
	syspll1_d8,
	msdcpll_d8,
	syspll3_d4,
	univpll_d26,
};

static const char *audio_parents[] __initconst = {
	clk26m,
	syspll3_d4,
	syspll4_d4,
	syspll1_d16,
};

static const char *aud_intbus_parents[] __initconst = {
	clk26m,
	syspll1_d4,
	syspll4_d2,
};

static const char *pmicspi_parents[] __initconst = {
	clk26m,
	univpll_d26,
	syspll3_d4,
	syspll1_d8,
	ulposc_d4,
	ulposc_d8,
	syspll2_d8,
};

static const char *scp_parents[] __initconst = {
	clk26m,
	syspll_d3,
	ulposc_ck,
	univpll_d5,
};

static const char *atb_parents[] __initconst = {
	clk26m,
	syspll1_d2,
	syspll_d5,
};

static const char *mjc_parents[] __initconst = {
	clk26m,
	imgpll_ck,
	univpll_d5,
	syspll1_d2,
};

static const char *dpi0_parents[] __initconst = {
	clk26m,
	tvdpll_d2,
	tvdpll_d4,
	tvdpll_d8,
	tvdpll_d16,
	clk26m,
	clk26m,
};

static const char *aud_1_parents[] __initconst = {
	clk26m,
	apll1_ck,
};

static const char *aud_2_parents[] __initconst = {
	clk26m,
	apll2_ck,
};

static const char *ssusb_top_sys_parents[] __initconst = {
	clk26m,
	univpll3_d2,
};

static const char *spm_parents[] __initconst = {
	clk26m,
	syspll1_d8,
};

static const char *bsi_spi_parents[] __initconst = {
	clk26m,
	syspll_d3_d3,
	syspll1_d4,
	syspll_d7,
};

static const char *audio_h_parents[] __initconst = {
	clk26m,
	apll2_ck,
	apll1_ck,
	univpll_d7,
};

static const char *anc_md32_parents[] __initconst = {
	clk26m,
	syspll1_d2,
	univpll_d5,
};

static const char *mfg_52m_parents[] __initconst = {
	clk26m,
	univpll2_d8,
	univpll2_d4,
	univpll2_d4,
};

#ifndef _MUX_UPDS_

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
	MUX(TOP_MUX_ULPOSC_AXI_CK_MUX_PRE, ulposc_axi_ck_mux_pre, ulposc_axi_ck_mux_pre_parents,
	    0x0040, 3, 1,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_ULPOSC_AXI_CK_MUX, ulposc_axi_ck_mux, ulposc_axi_ck_mux_parents, 0x0040, 2, 1,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 8, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_DDRPHYCFG, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 16, 2,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 24, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 8, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_VENC, venc_sel, venc_parents, 0x0050, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 24, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_CAMTG, camtg_sel, camtg, 0x0060, 0, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 8, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_ULPOSC_SPI_CK_MUX, ulposc_spi_ck_mux, ulposc_spi_ck_mux_parents, 0x0060, 18, 1,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_USB20, usb20_sel, usb20_parents, 0x0060, 24, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, 0x0070, 8, 2,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 16, 4, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 24, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AUD_INTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 24, 2,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_SCP, scp_sel, scp_parents, 0x0090, 8, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MJC, mjc_sel, mjc_parents, 0x0090, 24, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00A0, 0, 3, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AUD_1, aud_1_sel, aud_1_parents, 0x00A0, 16, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AUD_2, aud_2_sel, aud_2_parents, 0x00A0, 24, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_SSUSB_TOP_SYS, ssusb_top_sys_sel, ssusb_top_sys_parents, 0x00B0, 8, 1,
	    INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_SPM, spm_sel, spm_parents, 0x00C0, 0, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_BSI_SPI, bsi_spi_sel, bsi_spi_parents, 0x00C0, 8, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_AUDIO_H, audio_h_sel, audio_h_parents, 0x00C0, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_ANC_MD32, anc_md32_sel, anc_md32_parents, 0x00C0, 24, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MFG_52M, mfg_52m_sel, mfg_52m_parents, 0x0104, 1, 2,
		0),/* non glitch operation */
};


#else

#define _UPDATE_REG 0x04
#define INVALID_UPDATE_REG 0xFFFFFFFF
#define INVALID_UPDATE_SHIFT -1
#define INVALID_MUX_GATE -1
#ifdef _MUX_CLR_SET_UPDS_
static struct mtk_mux_clr_set_upd top_muxes[] __initdata = {
	MUX_CLR_SET_UPD(TOP_MUX_ULPOSC_AXI_CK_MUX_PRE, ulposc_axi_ck_mux_pre, ulposc_axi_ck_mux_pre_parents,
		0x0040, 0x0044, 0x0048, 3, 1,
		INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_ULPOSC_AXI_CK_MUX, ulposc_axi_ck_mux, ulposc_axi_ck_mux_parents, 0x0040,
	0x0044, 0x0048, 2, 1, INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0x0044, 0x0048, 0, 2, INVALID_MUX_GATE,
		_UPDATE_REG, 0),	/*AXI no gating */
	MUX_CLR_SET_UPD(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 0x0044, 0x0048, 8, 1, INVALID_MUX_GATE,
		_UPDATE_REG, 1),	/*15->INVALID_MUX_GATE */
	MUX_CLR_SET_UPD(TOP_MUX_DDRPHYCFG, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 0x0044, 0x0048, 16, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 2),/*23 */
	MUX_CLR_SET_UPD(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 0x0044, 0x0048, 24, 2, INVALID_MUX_GATE,
	_UPDATE_REG, 3),/*31*/
	MUX_CLR_SET_UPD(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0x0054, 0x0058, 0, 3, 7, _UPDATE_REG, 4),
	MUX_CLR_SET_UPD(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 0x0054, 0x0058, 8, 3, 15, _UPDATE_REG, 5),
	MUX_CLR_SET_UPD(TOP_MUX_VENC, venc_sel, venc_parents, 0x0050, 0x0054, 0x0058, 16, 2, 23, _UPDATE_REG, 6),
	MUX_CLR_SET_UPD(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 0x0054, 0x0058, 24, 2, 31, _UPDATE_REG, 7),
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG, camtg_sel, camtg, 0x0060, 0x0064, 0x0068, 0, 2, 7, _UPDATE_REG, 8),
	MUX_CLR_SET_UPD(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 0x0064, 0x0068, 8, 1, 15, _UPDATE_REG, 9),
	MUX_CLR_SET_UPD(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 0x0064, 0x0068, 16, 2, 23, _UPDATE_REG, 10),
	/*      TODO : need special non glitch */
	MUX_CLR_SET_UPD(TOP_MUX_ULPOSC_SPI_CK_MUX, ulposc_spi_ck_mux, ulposc_spi_ck_mux_parents,
	0x0060, 0x0064, 0x0068, 18, 1,	INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_USB20, usb20_sel, usb20_parents, 0x0060, 0x0064, 0x0068, 24, 2, 31,
	_UPDATE_REG, 11),	/*R1 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, 0x0070, 0x0074, 0x0078, 8, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 12),	/*R1 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 0x0074, 0x0078, 16, 4, 23,
	_UPDATE_REG, 13),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 0x0074, 0x0078, 24, 3, 31,
	_UPDATE_REG, 14),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0x0084, 0x0088, 0, 3, 7,
	_UPDATE_REG, 15),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 0x0084, 0x0088, 16, 2, 23, _UPDATE_REG, 17),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_INTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 0x0084, 0x0088, 24, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 18),
	MUX_CLR_SET_UPD(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0x0094, 0x0098, 0, 3, INVALID_MUX_GATE,
		_UPDATE_REG, 19),	/*7->INVALID_MUX_GATE */
	MUX_CLR_SET_UPD(TOP_MUX_SCP, scp_sel, scp_parents, 0x0090, 0x0094, 0x0098, 8, 2, INVALID_MUX_GATE,
	_UPDATE_REG, 20),/*15*/
	MUX_CLR_SET_UPD(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 0x0094, 0x0098, 16, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 21),/*23*/
	MUX_CLR_SET_UPD(TOP_MUX_MJC, mjc_sel, mjc_parents, 0x0090, 0x0094, 0x0098, 24, 2, 31, _UPDATE_REG, 22),
	MUX_CLR_SET_UPD(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00A0, 0x00A4, 0x00A8, 0, 3, 7, _UPDATE_REG, 23),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_1, aud_1_sel, aud_1_parents, 0x00A0, 0x00A4, 0x00A8, 16, 1, 23, _UPDATE_REG, 25),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_2, aud_2_sel, aud_2_parents, 0x00A0, 0x00A4, 0x00A8, 24, 1, 31, _UPDATE_REG, 26),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_SYS, ssusb_top_sys_sel, ssusb_top_sys_parents, 0x00B0, 0x00B4, 0x00B8, 8, 1,
		INVALID_MUX_GATE, _UPDATE_REG, 28),
	MUX_CLR_SET_UPD(TOP_MUX_SPM, spm_sel, spm_parents, 0x00C0, 0x00C4, 0x00C8, 0, 1, INVALID_MUX_GATE,
	_UPDATE_REG, 30),/*7*/
	MUX_CLR_SET_UPD(TOP_MUX_BSI_SPI, bsi_spi_sel, bsi_spi_parents, 0x00C0, 0x00C4, 0x00C8, 8, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 31),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO_H, audio_h_sel, audio_h_parents, 0x00C0, 0x00C4, 0x00C8, 16, 2, 23,
	_UPDATE_REG, 27),
	MUX_CLR_SET_UPD(TOP_MUX_ANC_MD32, anc_md32_sel, anc_md32_parents, 0x00C0, 0x00C4, 0x00C8, 24, 2, 31,
	_UPDATE_REG, 29),
	MUX_CLR_SET_UPD(TOP_MUX_MFG_52M, mfg_52m_sel, mfg_52m_parents, 0x0104, 0x0104,  0x0104, 1, 2,
	INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),/* non glitch operation */
};

#else
static struct mtk_mux_upd top_muxes[] __initdata = {
	MUX_UPD(TOP_MUX_ULPOSC_AXI_CK_MUX_PRE, ulposc_axi_ck_mux_pre, ulposc_axi_ck_mux_pre_parents,
		0x0040, 3, 1,
		INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_UPD(TOP_MUX_ULPOSC_AXI_CK_MUX, ulposc_axi_ck_mux, ulposc_axi_ck_mux_parents, 0x0040, 2,
		1,
		INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_UPD(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0, 2, INVALID_MUX_GATE,
		_UPDATE_REG, 0),	/*AXI no gating */
	MUX_UPD(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 8, 1, INVALID_MUX_GATE,
		_UPDATE_REG, 1),	/*15->INVALID_MUX_GATE */
	MUX_UPD(TOP_MUX_DDRPHYCFG, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 16, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 2),/*23 */
	MUX_UPD(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 24, 2, INVALID_MUX_GATE, _UPDATE_REG, 3),/*31*/
	MUX_UPD(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0, 3, 7, _UPDATE_REG, 4),
	MUX_UPD(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 8, 3, 15, _UPDATE_REG, 5),
	MUX_UPD(TOP_MUX_VENC, venc_sel, venc_parents, 0x0050, 16, 2, 23, _UPDATE_REG, 6),
	MUX_UPD(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 24, 2, 31, _UPDATE_REG, 7),
	MUX_UPD(TOP_MUX_CAMTG, camtg_sel, camtg, 0x0060, 0, 2, 7, _UPDATE_REG, 8),
	MUX_UPD(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 8, 1, 15, _UPDATE_REG, 9),
	MUX_UPD(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 16, 2, 23, _UPDATE_REG, 10),
	/*      TODO : need special non glitch */
	MUX_UPD(TOP_MUX_ULPOSC_SPI_CK_MUX, ulposc_spi_ck_mux, ulposc_spi_ck_mux_parents, 0x0060, 18,
		1,
		INVALID_MUX_GATE, INVALID_UPDATE_REG, INVALID_UPDATE_SHIFT),
	MUX_UPD(TOP_MUX_USB20, usb20_sel, usb20_parents, 0x0060, 24, 2, 31, _UPDATE_REG, 11),	/*R1 */
	MUX_UPD(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, 0x0070, 8, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 12),	/*R1 */
	MUX_UPD(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 16, 4, 23, _UPDATE_REG,
		13),
	MUX_UPD(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 24, 3, 31, _UPDATE_REG,
		14),
	MUX_UPD(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0, 3, 7, _UPDATE_REG, 15),
	MUX_UPD(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 16, 2, 23, _UPDATE_REG, 17),
	MUX_UPD(TOP_MUX_AUD_INTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 24, 2,
		INVALID_MUX_GATE, _UPDATE_REG, 18),
	MUX_UPD(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0, 3, INVALID_MUX_GATE,
		_UPDATE_REG, 19),	/*7->INVALID_MUX_GATE */
	MUX_UPD(TOP_MUX_SCP, scp_sel, scp_parents, 0x0090, 8, 2, INVALID_MUX_GATE, _UPDATE_REG, 20),/*15*/
	MUX_UPD(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 16, 2, INVALID_MUX_GATE, _UPDATE_REG, 21),/*23*/
	MUX_UPD(TOP_MUX_MJC, mjc_sel, mjc_parents, 0x0090, 24, 2, 31, _UPDATE_REG, 22),
	MUX_UPD(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00A0, 0, 3, 7, _UPDATE_REG, 23),
	MUX_UPD(TOP_MUX_AUD_1, aud_1_sel, aud_1_parents, 0x00A0, 16, 1, 23, _UPDATE_REG, 25),
	MUX_UPD(TOP_MUX_AUD_2, aud_2_sel, aud_2_parents, 0x00A0, 24, 1, 31, _UPDATE_REG, 26),
	MUX_UPD(TOP_MUX_SSUSB_TOP_SYS, ssusb_top_sys_sel, ssusb_top_sys_parents, 0x00B0, 8, 1,
		INVALID_MUX_GATE, _UPDATE_REG, 28),
	MUX_UPD(TOP_MUX_SPM, spm_sel, spm_parents, 0x00C0, 0, 1, INVALID_MUX_GATE, _UPDATE_REG, 30),/*7*/
	MUX_UPD(TOP_MUX_BSI_SPI, bsi_spi_sel, bsi_spi_parents, 0x00C0, 8, 2, INVALID_MUX_GATE, _UPDATE_REG, 31),
	MUX_UPD(TOP_MUX_AUDIO_H, audio_h_sel, audio_h_parents, 0x00C0, 16, 2, 23, _UPDATE_REG, 27),
	MUX_UPD(TOP_MUX_ANC_MD32, anc_md32_sel, anc_md32_parents, 0x00C0, 24, 2, 31, _UPDATE_REG,
		29),
	MUX_UPD(TOP_MUX_MFG_52M, mfg_52m_sel, mfg_52m_parents, 0x0104, 1, 2, INVALID_MUX_GATE, INVALID_UPDATE_REG,
		INVALID_UPDATE_SHIFT),	/*R1 -- 0 *//* non glitch operation */
};
#endif
#endif

#ifndef _MUX_UPDS_
static void __init init_clk_topckgen(void __iomem *top_base, struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(top_muxes); i++) {
		struct mtk_mux *mux = &top_muxes[i];

		clk = mtk_clk_register_mux(mux->name,
					   mux->parent_names, mux->num_parents,
					   top_base + mux->reg, mux->shift, mux->width, mux->gate);

		if (IS_ERR(clk)) {
			pr_err("Failed to register clk %s: %ld\n", mux->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[mux->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] mux %3d: %s\n", i, mux->name);
#endif				/* MT_CCF_DEBUG */
	}
}
#endif
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
	PLL(APMIXED_MAINPLL, mainpll, clk26m, 0x0220, 0x022C, 0xF0000101, HAVE_RST_BAR,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_UNIVPLL, univpll, clk26m, 0x0230, 0x023C, 0xFE000011, HAVE_RST_BAR,
	    &mt_clk_univ_pll_ops),
	PLL(APMIXED_MFGPLL, mfgpll, clk26m, 0x0240, 0x024C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_mm_pll_ops),
	PLL(APMIXED_MSDCPLL, msdcpll, clk26m, 0x0250, 0x025C, 0x00000121, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_IMGPLL, imgpll, clk26m, 0x0260, 0x026C, 0x00000121, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_TVDPLL, tvdpll, clk26m, 0x0270, 0x027C, 0xC0000121, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_MPLL, mpll, clk26m, 0x0280, 0x028c, 0x00000141, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_CODECPLL, codecpll, clk26m, 0x0290, 0x029C, 0x00000121, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_VDECPLL, vdecpll, clk26m, 0x02E4, 0x02F0, 0x00000121, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_APLL1, apll1, clk26m, 0x02A0, 0x02B0, 0x00000131, HAVE_PLL_HP,
	    &mt_clk_aud_pll_ops),
	PLL(APMIXED_APLL2, apll2, clk26m, 0x02B4, 0x02C4, 0x00000131, HAVE_PLL_HP,
	    &mt_clk_aud_pll_ops),
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
			pr_err("Failed to register clk %s: %ld\n", pll->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[pll->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] pll %3d: %s\n", i, pll->name);
#endif				/* MT_CCF_DEBUG */
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

static void __init init_clk_gates(void __iomem *reg_base,
				  struct mtk_gate *clks, int num, struct clk_onecell_data *clk_data)
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
			pr_err("Failed to register clk %s: %ld\n", gate->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[gate->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] gate %3d: %s\n", i, gate->name);
#endif				/* MT_CCF_DEBUG */
	}
}

static struct mtk_gate_regs infra0_cg_regs = {
	.set_ofs = 0x0080,
	.clr_ofs = 0x0084,
	.sta_ofs = 0x0090,
};

static struct mtk_gate_regs infra1_cg_regs = {
	.set_ofs = 0x0088,
	.clr_ofs = 0x008c,
	.sta_ofs = 0x0094,
};

static struct mtk_gate_regs infra2_cg_regs = {
	.set_ofs = 0x00a8,
	.clr_ofs = 0x00ac,
	.sta_ofs = 0x00b0,
};

static struct mtk_gate infra_clks[] __initdata = {
	GATE(INFRA_PMIC_TMR, infra_pmic_tmr, ulposc, infra0_cg_regs, 0, 0),
	GATE(INFRA_PMIC_AP, infra_pmic_ap, pmicspi_sel, infra0_cg_regs, 1, 0),
	GATE(INFRA_PMIC_MD, infra_pmic_md, pmicspi_sel, infra0_cg_regs, 2, 0),
	GATE(INFRA_PMIC_CONN, infra_pmic_conn, pmicspi_sel, infra0_cg_regs, 3, 0),
	GATE(INFRA_SCP, infra_scp, scp_sel, infra0_cg_regs, 4, 0),
	GATE(INFRA_SEJ, infra_sej, axi_sel, infra0_cg_regs, 5, 0),
	GATE(INFRA_APXGPT, infra_apxgpt, axi_sel, infra0_cg_regs, 6, 0),
	GATE(INFRA_SEJ_13M, infra_sej_13m, f26m_sel, infra0_cg_regs, 7, 0),
	GATE(INFRA_ICUSB, infra_icusb, usb20_sel, infra0_cg_regs, 8, 0),
	GATE(INFRA_GCE, infra_gce, axi_sel, infra0_cg_regs, 9, 0),
	GATE(INFRA_THERM, infra_therm, axi_sel, infra0_cg_regs, 10, 0),
	GATE(INFRA_I2C0, infra_i2c0, axi_sel, infra0_cg_regs, 11, 0),
	GATE(INFRA_I2C1, infra_i2c1, axi_sel, infra0_cg_regs, 12, 0),
	GATE(INFRA_I2C2, infra_i2c2, axi_sel, infra0_cg_regs, 13, 0),
	GATE(INFRA_I2C3, infra_i2c3, axi_sel, infra0_cg_regs, 14, 0),
	GATE(INFRA_PWM_HCLK, infra_pwm_hclk, axi_sel, infra0_cg_regs, 15, 0),
	GATE(INFRA_PWM1, infra_pwm1, axi_sel, infra0_cg_regs, 16, 0),
	GATE(INFRA_PWM2, infra_pwm2, axi_sel, infra0_cg_regs, 17, 0),
	GATE(INFRA_PWM3, infra_pwm3, axi_sel, infra0_cg_regs, 18, 0),
	GATE(INFRA_PWM4, infra_pwm4, axi_sel, infra0_cg_regs, 19, 0),
	GATE(INFRA_PWM, infra_pwm, axi_sel, infra0_cg_regs, 21, 0),
	GATE(INFRA_UART0, infra_uart0, uart_sel, infra0_cg_regs, 22, 0),
	GATE(INFRA_UART1, infra_uart1, uart_sel, infra0_cg_regs, 23, 0),
	GATE(INFRA_UART2, infra_uart2, uart_sel, infra0_cg_regs, 24, 0),
	GATE(INFRA_UART3, infra_uart3, uart_sel, infra0_cg_regs, 25, 0),
	GATE(INFRA_MD2MD_CCIF_0, infra_md2md_ccif_0, axi_sel, infra0_cg_regs, 27, 0),
	GATE(INFRA_MD2MD_CCIF_1, infra_md2md_ccif_1, axi_sel, infra0_cg_regs, 28, 0),
	GATE(INFRA_MD2MD_CCIF_2, infra_md2md_ccif_2, axi_sel, infra0_cg_regs, 29, 0),
	GATE(INFRA_FHCTL, infra_fhctl, f26m_sel, infra0_cg_regs, 30, 0),
	GATE(INFRA_BTIF, infra_btif, axi_sel, infra0_cg_regs, 31, 0),
	GATE(INFRA_MD2MD_CCIF_3, infra_md2md_ccif_3, axi_sel, infra1_cg_regs, 0, 0),
	GATE(INFRA_SPI, infra_spi, spi_sel, infra1_cg_regs, 1, 0),
	GATE(INFRA_MSDC0, infra_msdc0, msdc50_0_sel, infra1_cg_regs, 2, 0),
	GATE(INFRA_MD2MD_CCIF_4, infra_md2md_ccif_4, axi_sel, infra1_cg_regs, 3, 0),
	GATE(INFRA_MSDC1, infra_msdc1, msdc30_1_sel, infra1_cg_regs, 4, 0),
	GATE(INFRA_MSDC2, infra_msdc2, msdc30_2_sel, infra1_cg_regs, 5, 0),
	GATE(INFRA_MD2MD_CCIF_5, infra_md2md_ccif_5, axi_sel, infra1_cg_regs, 7, 0),
	GATE(INFRA_GCPU, infra_gcpu, axi_sel, infra1_cg_regs, 8, 0),
	GATE(INFRA_TRNG, infra_trng, axi_sel, infra1_cg_regs, 9, 0),
	GATE(INFRA_AUXADC, infra_auxadc, f26m_sel, infra1_cg_regs, 10, 0),
	GATE(INFRA_CPUM, infra_cpum, axi_sel, infra1_cg_regs, 11, 0),
	GATE(INFRA_AP_C2K_CCIF_0, infra_ap_c2k_ccif_0, axi_sel, infra1_cg_regs, 12, 0),
	GATE(INFRA_AP_C2K_CCIF_1, infra_ap_c2k_ccif_1, axi_sel, infra1_cg_regs, 13, 0),
	GATE(INFRA_CLDMA, infra_cldma, axi_sel, infra1_cg_regs, 16, 0),
	GATE(INFRA_DISP_PWM, infra_disp_pwm, pwm_sel, infra1_cg_regs, 17, 0),
	GATE(INFRA_AP_DMA, infra_ap_dma, axi_sel, infra1_cg_regs, 18, 0),
	GATE(INFRA_DEVICE_APC, infra_device_apc, axi_sel, infra1_cg_regs, 20, 0),
	GATE(INFRA_L2C_SRAM, infra_l2c_sram, mm_sel, infra1_cg_regs, 22, 0),
	GATE(INFRA_CCIF_AP, infra_ccif_ap, axi_sel, infra1_cg_regs, 23, 0),
	/*GATE(INFRA_DEBUGSYS, infra_debugsys, axi_sel, infra1_cg_regs, 24, 0),*/
	GATE(INFRA_AUDIO, infra_audio, axi_sel, infra1_cg_regs, 25, 0),
	GATE(INFRA_CCIF_MD, infra_ccif_md, axi_sel, infra1_cg_regs, 26, 0),
	GATE(INFRA_DRAMC_F26M, infra_dramc_f26m, f26m_sel, infra1_cg_regs, 31, 0),
	GATE(INFRA_I2C4, infra_i2c4, axi_sel, infra2_cg_regs, 0, 0),
	GATE(INFRA_I2C_APPM, infra_i2c_appm, axi_sel, infra2_cg_regs, 1, 0),
	GATE(INFRA_I2C_GPUPM, infra_i2c_gpupm, axi_sel, infra2_cg_regs, 2, 0),
	GATE(INFRA_I2C2_IMM, infra_i2c2_imm, axi_sel, infra2_cg_regs, 3, 0),
	GATE(INFRA_I2C2_ARB, infra_i2c2_arb, axi_sel, infra2_cg_regs, 4, 0),
	GATE(INFRA_I2C3_IMM, infra_i2c3_imm, axi_sel, infra2_cg_regs, 5, 0),
	GATE(INFRA_I2C3_ARB, infra_i2c3_arb, axi_sel, infra2_cg_regs, 6, 0),
	GATE(INFRA_I2C5, infra_i2c5, axi_sel, infra2_cg_regs, 7, 0),
	GATE(INFRA_SYS_CIRQ, infra_sys_cirq, axi_sel, infra2_cg_regs, 8, 0),
	GATE(INFRA_SPI1, infra_spi1, spi_sel, infra2_cg_regs, 10, 0),
	GATE(INFRA_DRAMC_B_F26M, infra_dramc_b_f26m, f26m_sel, infra2_cg_regs, 11, 0),
	GATE(INFRA_ANC_MD32, infra_anc_md32, anc_md32_sel, infra2_cg_regs, 12, 0),
	GATE(INFRA_ANC_MD32_32K, infra_anc_md32_32k, clk_null, infra2_cg_regs, 13, 0),
	GATE(INFRA_DVFS_SPM1, infra_dvfs_spm1, axi_sel, infra2_cg_regs, 15, 0),
	GATE(INFRA_AES_TOP0, infra_aes_top0, axi_sel, infra2_cg_regs, 16, 0),
	GATE(INFRA_AES_TOP1, infra_aes_top1, axi_sel, infra2_cg_regs, 17, 0),
	GATE(INFRA_SSUSB_BUS, infra_ssusb_bus, axi_sel, infra2_cg_regs, 18, 0),
	GATE(INFRA_SPI2, infra_spi2, spi_sel, infra2_cg_regs, 19, 0),
	GATE(INFRA_SPI3, infra_spi3, spi_sel, infra2_cg_regs, 20, 0),
	GATE(INFRA_SPI4, infra_spi4, spi_sel, infra2_cg_regs, 21, 0),
	GATE(INFRA_SPI5, infra_spi5, spi_sel, infra2_cg_regs, 22, 0),
	GATE(INFRA_IRTX, infra_irtx, spi_sel, infra2_cg_regs, 23, 0),
	GATE(INFRA_SSUSB_SYS, infra_ssusb_sys, ssusb_top_sys_sel, infra2_cg_regs, 24, 0),
	GATE(INFRA_SSUSB_REF, infra_ssusb_ref, f26m_sel, infra2_cg_regs, 9, 0),/*9:dummy*/
	GATE(INFRA_AUDIO_26M, infra_audio_26m, f26m_sel, infra2_cg_regs, 26, 0),
	GATE(INFRA_AUDIO_26M_PAD_TOP, infra_audio_26m_pad_top, f26m_sel, infra2_cg_regs, 27, 0),
	GATE(INFRA_MODEM_TEMP_SHARE, infra_modem_temp_share, axi_sel, infra2_cg_regs, 28, 0),
	GATE(INFRA_VAD_WRAP_SOC, infra_vad_wrap_soc, axi_sel, infra2_cg_regs, 29, 0),
	GATE(INFRA_DRAMC_CONF, infra_dramc_conf, axi_sel, infra2_cg_regs, 30, 0),
	GATE(INFRA_DRAMC_B_CONF, infra_dramc_b_conf, axi_sel, infra2_cg_regs, 31, 0),
	GATE(INFRA_MFG_VCG, infra_mfg_vcg, mfg_52m_sel, infra1_cg_regs, 14, 0),/*Virturl CG*/
};

static void __init init_clk_infrasys(void __iomem *infrasys_base,
				     struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init infrasys gates:\n");
	init_clk_gates(infrasys_base, infra_clks, ARRAY_SIZE(infra_clks), clk_data);
}

static struct mtk_gate_regs mfg_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate mfg_clks[] __initdata = {
	GATE(MFG_BG3D, mfg_bg3d, mfg_sel, mfg_cg_regs, 0, 0),
};

static void __init init_clk_mfgsys(void __iomem *mfgsys_base, struct clk_onecell_data *clk_data)
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
	GATE(IMG_FDVT, img_fdvt, mm_sel, img_cg_regs, 11, 0),
	GATE(IMG_DPE, img_dpe, mm_sel, img_cg_regs, 10, 0),
	GATE(IMG_DIP, img_dip, mm_sel, img_cg_regs, 6, 0),
	GATE(IMG_LARB6, img_larb6, mm_sel, img_cg_regs, 0, 0),
};

static void __init init_clk_imgsys(void __iomem *imgsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init imgsys gates:\n");
	init_clk_gates(imgsys_base, img_clks, ARRAY_SIZE(img_clks), clk_data);
}

static struct mtk_gate_regs cam_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate cam_clks[] __initdata = {
	GATE(CAM_CAMSV2, cam_camsv2, mm_sel, cam_cg_regs, 11, 0),
	GATE(CAM_CAMSV1, cam_camsv1, mm_sel, cam_cg_regs, 10, 0),
	GATE(CAM_CAMSV0, cam_camsv0, mm_sel, cam_cg_regs, 9, 0),
	GATE(CAM_SENINF, cam_seninf, mm_sel, cam_cg_regs, 8, 0),
	GATE(CAM_CAMTG, cam_camtg, camtg_sel, cam_cg_regs, 7, 0),
	GATE(CAM_CAMSYS, cam_camsys, mm_sel, cam_cg_regs, 6, 0),
	GATE(CAM_LARB2, cam_larb2, mm_sel, cam_cg_regs, 0, 0),
};

static void __init init_clk_camsys(void __iomem *camsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init camsys gates:\n");
	init_clk_gates(camsys_base, cam_clks, ARRAY_SIZE(cam_clks), clk_data);
}

static struct mtk_gate_regs mm0_cg_regs = {
	.set_ofs = 0x0104,
	.clr_ofs = 0x0108,
	.sta_ofs = 0x0100,
};

static struct mtk_gate_regs mm0_dummy_cg_regs = {
	.set_ofs = 0x0894,
	.clr_ofs = 0x0894,
	.sta_ofs = 0x0894,
};

static struct mtk_gate_regs mm1_cg_regs = {
	.set_ofs = 0x0114,
	.clr_ofs = 0x0118,
	.sta_ofs = 0x0110,
};

static struct mtk_gate_regs mm1_dummy_cg_regs = {
	.set_ofs = 0x0898,
	.clr_ofs = 0x0898,
	.sta_ofs = 0x0898,
};

static struct mtk_gate mm_clks[] __initdata = {
	GATE(MM_SMI_COMMON, mm_smi_common, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_SMI_LARB0, mm_smi_larb0, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_SMI_LARB5, mm_smi_larb5, mm_sel, mm0_cg_regs, 2, 0),
	GATE(MM_CAM_MDP, mm_cam_mdp, mm_sel, mm0_dummy_cg_regs, 3, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_RDMA0, mm_mdp_rdma0, mm_sel, mm0_dummy_cg_regs, 4, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_RDMA1, mm_mdp_rdma1, mm_sel, mm0_dummy_cg_regs, 5, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_RSZ0, mm_mdp_rsz0, mm_sel, mm0_dummy_cg_regs, 6, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_RSZ1, mm_mdp_rsz1, mm_sel, mm0_dummy_cg_regs, 7, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_RSZ2, mm_mdp_rsz2, mm_sel, mm0_dummy_cg_regs, 8, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_TDSHP, mm_mdp_tdshp, mm_sel, mm0_dummy_cg_regs, 9, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_COLOR, mm_mdp_color, mm_sel, mm0_dummy_cg_regs, 10, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_WDMA, mm_mdp_wdma, mm_sel, mm0_dummy_cg_regs, 11, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_WROT0, mm_mdp_wrot0, mm_sel, mm0_dummy_cg_regs, 12, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_MDP_WROT1, mm_mdp_wrot1, mm_sel, mm0_dummy_cg_regs, 13, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_FAKE_ENG, mm_fake_eng, mm_sel, mm0_cg_regs, 14, 0),
	GATE(MM_DISP_OVL0, mm_disp_ovl0, mm_sel, mm0_dummy_cg_regs, 15, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_OVL1, mm_disp_ovl1, mm_sel, mm0_dummy_cg_regs, 16, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_OVL0_2L, mm_disp_ovl0_2l, mm_sel, mm0_dummy_cg_regs, 17,
	CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_OVL1_2L, mm_disp_ovl1_2l, mm_sel, mm0_dummy_cg_regs, 18,
	CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_RDMA0, mm_disp_rdma0, mm_sel, mm0_dummy_cg_regs, 19, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_RDMA1, mm_disp_rdma1, mm_sel, mm0_dummy_cg_regs, 20, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_WDMA0, mm_disp_wdma0, mm_sel, mm0_dummy_cg_regs, 21, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_WDMA1, mm_disp_wdma1, mm_sel, mm0_dummy_cg_regs, 22, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_COLOR, mm_disp_color, mm_sel, mm0_dummy_cg_regs, 23, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_CCORR, mm_disp_ccorr, mm_sel, mm0_dummy_cg_regs, 24, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_AAL, mm_disp_aal, mm_sel, mm0_dummy_cg_regs, 25, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_GAMMA, mm_disp_gamma, mm_sel, mm0_dummy_cg_regs, 26, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_OD, mm_disp_od, mm_sel, mm0_dummy_cg_regs, 27, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_DITHER, mm_disp_dither, mm_sel, mm0_dummy_cg_regs, 28, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_UFOE, mm_disp_ufoe, mm_sel, mm0_dummy_cg_regs, 29, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_DSC, mm_disp_dsc, mm_sel, mm0_dummy_cg_regs, 30, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DISP_SPLIT, mm_disp_split, mm_sel, mm0_dummy_cg_regs, 31, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_DSI0_MM_CLOCK, mm_dsi0_mm_clock, mm_sel, mm1_cg_regs, 0, 0),
	GATE(MM_DSI1_MM_CLOCK, mm_dsi1_mm_clock, mm_sel, mm1_cg_regs, 2, 0),
	GATE(MM_DPI_MM_CLOCK, mm_dpi_mm_clock, mm_sel, mm1_cg_regs, 4, 0),
	GATE(MM_DPI_INTERFACE_CLOCK, mm_dpi_interface_clock, dpi0_sel, mm1_dummy_cg_regs, 5,
	CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_LARB4_AXI_ASIF_MM_CLOCK, mm_larb4_axi_asif_mm_clock, mm_sel, mm1_cg_regs, 6, 0),
	GATE(MM_LARB4_AXI_ASIF_MJC_CLOCK, mm_larb4_axi_asif_mjc_clock, mjc_sel, mm1_cg_regs, 7, 0),
	GATE(MM_DISP_OVL0_MOUT_CLOCK, mm_disp_ovl0_mout_clock, mm_sel, mm1_dummy_cg_regs, 8,
	CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MM_FAKE_ENG2, mm_fake_eng2, mm_sel, mm1_cg_regs, 9, 0),
	GATE(MM_DSI0_INTERFACE_CLOCK, mm_dsi0_interface_clock, clk_null, mm1_cg_regs, 1, 0),
	GATE(MM_DSI1_INTERFACE_CLOCK, mm_dsi1_interface_clock, clk_null, mm1_cg_regs, 3, 0),
};

static void __init init_clk_mmsys(void __iomem *mmsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mmsys gates:\n");
	init_clk_gates(mmsys_base, mm_clks, ARRAY_SIZE(mm_clks), clk_data);
}

static struct mtk_gate_regs mjc_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate mjc_clks[] __initdata = {
	GATE(MJC_SMI_LARB, mjc_smi_larb, mjc_sel, mjc_cg_regs, 0, 0),
	GATE(MJC_TOP_CLK_0, mjc_top_clk_0, mjc_sel, mjc_cg_regs, 1, 0),
	GATE(MJC_TOP_CLK_1, mjc_top_clk_1, mjc_sel, mjc_cg_regs, 2, 0),
	GATE(MJC_TOP_CLK_2, mjc_top_clk_2, mjc_sel, mjc_cg_regs, 3, 0),
	GATE(MJC_FAKE_ENGINE, mjc_fake_engine, mjc_sel, mjc_cg_regs, 4, 0),
	GATE(MJC_LARB4_ASIF, mjc_larb4_asif, mjc_sel, mjc_cg_regs, 5, 0),
};

static void __init init_clk_mjcsys(void __iomem *mjcsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mjcsys gates:\n");
	init_clk_gates(mjcsys_base, mjc_clks, ARRAY_SIZE(mjc_clks), clk_data);
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
	GATE(VDEC_CKEN_ENG, vdec_cken_eng, vdec_sel, vdec0_cg_regs, 8, CLK_GATE_INVERSE),
	GATE(VDEC_ACTIVE, vdec_active, vdec_sel, vdec0_cg_regs, 4, CLK_GATE_INVERSE),
	GATE(VDEC_CKEN, vdec_cken, vdec_sel, vdec0_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC_LARB1_CKEN, vdec_larb1_cken, mm_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
};

static void __init init_clk_vdecsys(void __iomem *vdecsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vdecsys gates:\n");
	init_clk_gates(vdecsys_base, vdec_clks, ARRAY_SIZE(vdec_clks), clk_data);
}

static struct mtk_gate_regs venc_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0008,
	.sta_ofs = 0x0000,
};

static struct mtk_gate venc_clks[] __initdata = {
	GATE(VENC_0, venc_0, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_1, venc_1, venc_sel, venc_cg_regs, 4, CLK_GATE_INVERSE),
	GATE(VENC_2, venc_2, venc_sel, venc_cg_regs, 8, CLK_GATE_INVERSE),
	GATE(VENC_3, venc_3, venc_sel, venc_cg_regs, 12, CLK_GATE_INVERSE),
};

static void __init init_clk_vencsys(void __iomem *vencsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vencsys gates:\n");
	init_clk_gates(vencsys_base, venc_clks, ARRAY_SIZE(venc_clks), clk_data);
}

static struct mtk_gate_regs aud0_cg_regs = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0000,
	.sta_ofs = 0x0000,
};

static struct mtk_gate_regs aud1_cg_regs = {
	.set_ofs = 0x0004,
	.clr_ofs = 0x0004,
	.sta_ofs = 0x0004,
};


static struct mtk_gate audio_clks[] __initdata = {
	GATE(AUDIO_PDN_TML, audio_pdn_tml, audio_sel, aud0_cg_regs, 27, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_DAC_PREDIS, audio_pdn_dac_predis, audio_sel, aud0_cg_regs, 26,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_DAC, audio_pdn_dac, audio_sel, aud0_cg_regs, 25, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_ADC, audio_pdn_adc, audio_sel, aud0_cg_regs, 24, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_APLL_TUNER, audio_pdn_apll_tuner, aud_1_sel, aud0_cg_regs, 19,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_APLL2_TUNER, audio_pdn_apll2_tuner, aud_2_sel, aud0_cg_regs, 18,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_24M, audio_pdn_24m, aud_2_sel, aud0_cg_regs, 9, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_22M, audio_pdn_22m, aud_1_sel, aud0_cg_regs, 8, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_AFE, audio_pdn_afe, audio_sel, aud0_cg_regs, 2, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_ADC_HIRES_TML, audio_pdn_adc_hires_tml, audio_h_sel, aud1_cg_regs, 17,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_PDN_ADC_HIRES, audio_pdn_adc_hires, audio_h_sel, aud1_cg_regs, 16,
	     CLK_GATE_NO_SETCLR_REG),
};

static void __init init_clk_audiosys(void __iomem *audiosys_base,
				     struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init audiosys gates:\n");
	init_clk_gates(audiosys_base, audio_clks, ARRAY_SIZE(audio_clks), clk_data);
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
#ifndef _MUX_UPDS_
	init_clk_topckgen(base, clk_data);
#else
	#ifdef _MUX_CLR_SET_UPDS_
	mtk_clk_register_mux_clr_set_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
				  &mt6797_clk_lock, clk_data);
	#else
	mtk_clk_register_mux_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
				  &mt6797_clk_lock, clk_data);
	#endif
#endif
	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");

	mt_reg_sync_writel(0x00000FFF, (base + 0x200));	/* CLK_SCP_CFG_0 = 0x00000FFF */
	mt_reg_sync_writel(0x00000007, (base + 0x204));	/* CLK_SCP_CFG_1 = 0x00000007 */

#if CG_BOOTUP_PDN
	cksys_base = base;
#endif
}

CLK_OF_DECLARE(mtk_topckgen, "mediatek,topckgen", mt_topckgen_init);

#define ISO_EN_BIT 0x2
#define ON_BIT 0x1
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

	mt_reg_sync_writel(0x00044440, (base + 0x0C));	/* AP_PLL_CON3, 0x00044440 */
	mt_reg_sync_writel(0xC, (base + 0x10));	/* AP_PLL_CON4, temp & 0xC */

	/*disable*/
/* MSDCPLL */
	mt_reg_sync_writel(__raw_readl(base + MSDCPLL_CON0_OFS) & ~ON_BIT,
			   (base + MSDCPLL_CON0_OFS)); /*CON0[0]=0*/
	mt_reg_sync_writel(__raw_readl(base + MSDCPLL_PWR_CON0_OFS) | ISO_EN_BIT,
			   (base + MSDCPLL_PWR_CON0_OFS)); /*PWR_CON0[1]=1*/
	mt_reg_sync_writel(__raw_readl(base + MSDCPLL_PWR_CON0_OFS) & ~ON_BIT,
			   (base + MSDCPLL_PWR_CON0_OFS)); /*PWR_CON0[0]=0*/
/*TVDPLL*/
	mt_reg_sync_writel(__raw_readl(base + TVDPLL_CON0_OFS) & ~ON_BIT,
			   (base + TVDPLL_CON0_OFS)); /*CON0[0]=0*/
	mt_reg_sync_writel(__raw_readl(base + TVDPLL_PWR_CON0_OFS) | ISO_EN_BIT,
			   (base + TVDPLL_PWR_CON0_OFS)); /*PWR_CON0[1]=1*/
	mt_reg_sync_writel(__raw_readl(base + TVDPLL_PWR_CON0_OFS) & ~ON_BIT,
			   (base + TVDPLL_PWR_CON0_OFS)); /*PWR_CON0[0]=0*/
/*APLL 1,2 */
	mt_reg_sync_writel(__raw_readl(base + APLL1_CON0_OFS) & ~ON_BIT,
			   (base + APLL1_CON0_OFS)); /*CON0[0]=0*/
	mt_reg_sync_writel(__raw_readl(base + APLL1_PWR_CON0_OFS) | ISO_EN_BIT,
			   (base + APLL1_PWR_CON0_OFS)); /*PWR_CON0[1]=1*/
	mt_reg_sync_writel(__raw_readl(base + APLL1_PWR_CON0_OFS) & ~ON_BIT,
			   (base + APLL1_PWR_CON0_OFS)); /*PWR_CON0[0]=0*/

	mt_reg_sync_writel(__raw_readl(base + APLL2_CON0_OFS) & ~ON_BIT,
			   (base + APLL2_CON0_OFS)); /*CON0[0]=0*/
	mt_reg_sync_writel(__raw_readl(base + APLL2_PWR_CON0_OFS) | ISO_EN_BIT,
			   (base + APLL2_PWR_CON0_OFS)); /*PWR_CON0[1]=1*/
	mt_reg_sync_writel(__raw_readl(base + APLL2_PWR_CON0_OFS) & ~ON_BIT,
			   (base + APLL2_PWR_CON0_OFS)); /*PWR_CON0[0]=0*/
}

CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,apmixed", mt_apmixedsys_init);

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

	mt_reg_sync_writel(__raw_readl(base + INFRA_BUS_DCM_CTRL_OFS) | (1 << 21),
			   (base + INFRA_BUS_DCM_CTRL_OFS));

#if CG_BOOTUP_PDN
	infracfg_base = base;
	clk_writel(INFRA_PDN_SET0, INFRA0_CG);
	clk_writel(INFRA_PDN_SET1, INFRA1_CG);
	clk_writel(INFRA_PDN_SET2, INFRA2_CG);
#endif
}

CLK_OF_DECLARE(mtk_infrasys, "mediatek,infracfg_ao", mt_infrasys_init);

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

#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x1, (base + mfg_cg_regs.clr_ofs));
#endif
#if CG_BOOTUP_PDN
	mfgcfg_base = base;
	clk_writel(MFG_CG_SET, MFG_CG);
#endif

}

CLK_OF_DECLARE(mtk_mfgsys, "mediatek,g3d_config", mt_mfgsys_init);

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

#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x00000C41, base + img_cg_regs.clr_ofs);
#endif
#if CG_BOOTUP_PDN
	img_base = base;
	clk_writel(IMG_CG_SET, IMG_CG);
#endif


}

CLK_OF_DECLARE(mtk_imgsys, "mediatek,imgsys_config", mt_imgsys_init);

static void __init mt_camsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap camsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(CAM_NR_CLK);

	init_clk_camsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");


#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x00000FC1, base + cam_cg_regs.clr_ofs);
#endif
#if CG_BOOTUP_PDN
	camsys_base = base;
	clk_writel(CAMSYS_CG_SET, CAM_CG);
#endif

}

CLK_OF_DECLARE(mtk_camsys, "mediatek,camsys_config", mt_camsys_init);

#define MMSYS_CG_SET0_OFS 0x104
#define MMSYS_CG_SET1_OFS 0x114
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

#if CG_BOOTUP_PDN
	mmsys_config_base = base;
	clk_writel(MM_CG_SET0, MM_0_CG);
	clk_writel(MM_CG_SET1, MM_1_CG);
	clk_writel(MM_DUMMY_CG_SET0, ~MM_DUMMY_0_CG);
	clk_writel(MM_DUMMY_CG_SET1, ~MM_DUMMY_1_CG);
#endif

}

CLK_OF_DECLARE(mtk_mmsys, "mediatek,mmsys_config", mt_mmsys_init);

static void __init mt_mjcsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_err("ioremap mjcsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(MJC_NR_CLK);

	init_clk_mjcsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");

#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x0000003F, base + mjc_cg_regs.clr_ofs);
#endif
#if CG_BOOTUP_PDN
	mjcsys_base = base;
	clk_writel(MJC_CG_SET, MJC_CG);
#endif

}

CLK_OF_DECLARE(mtk_mjcsys, "mediatek,mjc_config-v1", mt_mjcsys_init);

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

#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x00000111, base + vdec0_cg_regs.set_ofs);
	mt_reg_sync_writel(0x00000001, base + vdec1_cg_regs.set_ofs);
#endif
#if CG_BOOTUP_PDN
	vdec_gcon_base = base;
	clk_writel(VDEC_CKEN_CLR, VDEC_CG);
	clk_writel(LARB_CKEN_CLR, LARB_CG);
#endif

}

CLK_OF_DECLARE(mtk_vdecsys, "mediatek,mt6797-vdec_gcon", mt_vdecsys_init);

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
#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x00001111, base + venc_cg_regs.clr_ofs);
#endif
#if CG_BOOTUP_PDN
	venc_gcon_base = base;
	clk_clrl(VENC_CG_CON, VENC_CG);
#endif

}

CLK_OF_DECLARE(mtk_vencsys, "mediatek,mt6797-venc_gcon", mt_vencsys_init);

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

#if MT_CCF_BRINGUP
	mt_reg_sync_writel(0x801c4000, base + aud0_cg_regs.set_ofs);
	mt_reg_sync_writel(0x00000000, base + aud1_cg_regs.set_ofs);
#endif
#if CG_BOOTUP_PDN
	audio_base = base;
	clk_writel(AUDIO_TOP_CON0, clk_readl(AUDIO_TOP_CON0)|AUD_0_CG);
	clk_writel(AUDIO_TOP_CON1, AUD_1_CG);
#endif

}

CLK_OF_DECLARE(mtk_audiosys, "mediatek,audio", mt_audiosys_init);

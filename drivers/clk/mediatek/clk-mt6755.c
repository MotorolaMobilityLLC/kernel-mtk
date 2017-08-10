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
#include <linux/delay.h>
#include <linux/slab.h>

#include "clk-mtk-v1.h"
#include "clk-pll-v1.h"
#include "clk-gate-v1.h"
#include "clk-mt6755-pll.h"
#define _MUX_CLR_SET_UPDS_
#define _MUX_UPDS_
#ifdef _MUX_UPDS_
#include "clk-mux.h"
static DEFINE_SPINLOCK(mt6755_clk_lock);
#endif
#include <dt-bindings/clock/mt6755-clk.h>
#include "../../misc/mediatek/base/power/mt6755/mt_spm_mtcmos.h"
#include "../../misc/mediatek/include/mt-plat/mt_devinfo.h"

#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP	0
#endif
#ifdef CONFIG_ARM64
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif
#define mt_reg_sync_writel(v, a) \
	do {	\
		__raw_writel((v), IOMEM((a)));   \
		mb();  /* for mt_reg_sync_writel() */ \
	} while (0)
#define clk_readl(addr)			__raw_readl(IOMEM(addr))

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_setl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

/*
 * platform clocks
 */

/* ROOT */
#define clk_null		"clk_null"
#define clk26m			"clk26m"
#define clk32k			"clk32k"
#define clk13m			"clk13m"

#define clkph_mck_o		"clkph_mck_o"
#define dpi_ck			"dpi_ck"

/* PLL */
#define armbpll		"armbpll"
#define armspll		"armspll"
/*#define armpll		"armpll"*/
#define mainpll		"mainpll"
#define msdcpll		"msdcpll"
#define univpll		"univpll"
#define mmpll		"mmpll"
#define vencpll		"vencpll"
#define tvdpll		"tvdpll"
#define apll1		"apll1"
#define apll2		"apll2"
#define oscpll		"oscpll"

/* DIV */
#define syspll_ck		"syspll_ck"
#define syspll1_ck		"syspll1_ck"
#define	syspll2_ck		"syspll2_ck"
#define syspll3_ck		"syspll3_ck"
#define	syspll4_ck		"syspll4_ck"
#define univpll_ck		"univpll_ck"
#define univpll1_ck		"univpll1_ck"
#define univpll2_ck		"univpll2_ck"
#define univpll3_ck		"univpll3_ck"
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
/*---------------------------------------------*/
#define osc_ck	"osc_ck"
#define osc_d8	"osc_d8"
#define osc_d2	"osc_d2"
#define syspll_d7	"syspll_d7"
#define univpll_d7	"univpll_d7"
#define osc_d4	"osc_d4"
#define	tvdpll_d8	"tvdpll_d8"
#define	tvdpll_d16	"tvdpll_d16"
#define	apll1_ck	"apll1_ck"
#define	apll2_ck	"apll2_ck"
#define	syspll_d3_d3	"syspll_d3_d3"



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
/*--------------------------------------------------------------*/
#define ssusb_top_sys_sel		"ssusb_top_sys_sel"
#define usb_top_sel	"usb_top_sel"
#define i2c_sel			"i2c_sel"
#define f26m_sel		"f26m_sel"
#define msdc50_0_hclk_sel		"msdc50_0_hclk_sel"
#define	bsi_spi_sel	"bsi_spi_sel"
#define spm_sel				"spm_sel"
#define	dvfsp_sel	"dvfsp_sel"
#define i2s_sel				"i2s_sel"



/* INFRA */
#define	infra_pmictmr	"infra_pmictmr"
#define	infra_pmicap	"infra_pmicap"
#define	infra_pmicmd	"infra_pmicmd"
#define	infra_pmicconn	"infra_pmicconn"
#define	infra_scpsys	"infra_scpsys"
#define	infra_sej	"infra_sej"
#define	infra_apxgpt	"infra_apxgpt"
#define	infra_icusb	"infra_icusb"
#define	infra_gce	"infra_gce"
#define	infra_therm	"infra_therm"
#define	infra_i2c0	"infra_i2c0"
#define	infra_i2c1	"infra_i2c1"
#define	infra_i2c2	"infra_i2c2"
#define	infra_i2c3	"infra_i2c3"
#define	infra_pwmhclk	"infra_pwmhclk"
#define	infra_pwm1	"infra_pwm1"
#define	infra_pwm2	"infra_pwm2"
#define	infra_pwm3	"infra_pwm3"
#define	infra_pwm4	"infra_pwm4"
#define	infra_pwm	"infra_pwm"
#define	infra_uart0	"infra_uart0"
#define	infra_uart1	"infra_uart1"
#define	infra_uart2	"infra_uart2"
#define	infra_uart3	"infra_uart3"
#define	infra_md2mdccif0	"infra_md2mdccif0"
#define	infra_md2mdccif1	"infra_md2mdccif1"
#define	infra_md2mdccif2	"infra_md2mdccif2"
#define	infra_btif	"infra_btif"
#define	infra_md2mdccif3	"infra_md2mdccif3"
#define	infra_spi0	"infra_spi0"
#define	infra_msdc0	"infra_msdc0"
#define	infra_md2mdccif4	"infra_md2mdccif4"
#define	infra_msdc1	"infra_msdc1"
#define	infra_msdc2	"infra_msdc2"
#define	infra_msdc3	"infra_msdc3"
#define	infra_md2mdccif5	"infra_md2mdccif5"
#define	infra_gcpu	"infra_gcpu"
#define	infra_trng	"infra_trng"
#define	infra_auxadc	"infra_auxadc"
#define	infra_cpum	"infra_cpum"
#define	infra_ccif1ap	"infra_ccif1ap"
#define	infra_ccif1md	"infra_ccif1md"
#define	infra_apdma	"infra_apdma"
#define	infra_xiu	"infra_xiu"
#define	infra_deviceapc	"infra_deviceapc"
#define	infra_xiu2ahb	"infra_xiu2ahb"
#define	infra_ccifap	"infra_ccifap"
#define	infra_debugsys	"infra_debugsys"
#define	infra_audio	"infra_audio" /* confirm if for audio??*/
#define	infra_ccifmd	"infra_ccifmd"
#define	infra_dramcf26m	"infra_dramcf26m"
#define	infra_irtx	"infra_irtx"
#define	infra_ssusbtop	"infra_ssusbtop"
#define	infra_disppwm	"infra_disppwm"
#define	infra_cldmabclk	"infra_cldmabclk"
#define	infra_audio26mbclk	"infra_audio26mbclk"
#define	infra_mdtmp26mbclk	"infra_mdtmp26mbclk"
#define	infra_spi1	"infra_spi1"
#define	infra_i2c4	"infra_i2c4"
#define	infra_mdtmpshare	"infra_mdtmpshare"


#if 0
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
#if 0
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
#endif
#endif
/* MFG */
#define mfg_bg3d			"mfg_bg3d"

/* IMG */
#define img_image_larb2_smi		"img_image_larb2_smi"
#define img_image_larb2_smi_m4u		"img_image_larb2_smi_m4u"
#define img_image_larb2_smi_smi_common		"img_image_larb2_smi_smi_common"
#define img_image_larb2_smi_met_smi_common		"img_image_larb2_smi_met_smi_common"
#define img_image_larb2_smi_ispsys		"img_image_larb2_smi_ispsys"
#define img_image_cam_smi		"img_image_cam_smi"
#define img_image_cam_cam		"img_image_cam_cam"
#define img_image_sen_tg		"img_image_sen_tg"
#define img_image_sen_cam		"img_image_sen_cam"
#define img_image_cam_sv		"img_image_cam_sv"
#define img_image_sufod		"img_image_sufod"
#define img_image_fd		"img_image_fd"

/* MM_SYS */
#define	mm_disp0_smi_common	"mm_disp0_smi_common"
#define	mm_disp0_smi_common_m4u	"mm_disp0_smi_common_m4u"
#define	mm_disp0_smi_common_mali	"mm_disp0_smi_common_mali"
#define	mm_disp0_smi_common_dispsys	"mm_disp0_smi_common_dispsys"
#define	mm_disp0_smi_common_smi_common	"mm_disp0_smi_common_smi_common"
#define	mm_disp0_smi_common_met_smi_common	"mm_disp0_smi_common_met_smi_common"
#define	mm_disp0_smi_common_ispsys	"mm_disp0_smi_common_ispsys"
#define	mm_disp0_smi_common_fdvt	"mm_disp0_smi_common_fdvt"
#define	mm_disp0_smi_common_vdec_gcon	"mm_disp0_smi_common_vdec_gcon"
#define	mm_disp0_smi_common_jpgenc	"mm_disp0_smi_common_jpgenc"
#define	mm_disp0_smi_common_jpgdec	"mm_disp0_smi_common_jpgdec"

#define	mm_disp0_smi_larb0	"mm_disp0_smi_larb0"
#define	mm_disp0_smi_larb0_m4u	"mm_disp0_smi_larb0_m4u"
#define	mm_disp0_smi_larb0_dispsys	"mm_disp0_smi_larb0_dispsys"
#define	mm_disp0_smi_larb0_smi_common	"mm_disp0_smi_larb0_smi_common"
#define	mm_disp0_smi_larb0_met_smi_common	"mm_disp0_smi_larb0_met_smi_common"


#define	mm_disp0_cam_mdp	"mm_disp0_cam_mdp"
#define	mm_disp0_mdp_rdma	"mm_disp0_mdp_rdma"
#define	mm_disp0_mdp_rsz0	"mm_disp0_mdp_rsz0"
#define	mm_disp0_mdp_rsz1	"mm_disp0_mdp_rsz1"
#define	mm_disp0_mdp_tdshp	"mm_disp0_mdp_tdshp"
#define	mm_disp0_mdp_wdma	"mm_disp0_mdp_wdma"
#define	mm_disp0_mdp_wrot	"mm_disp0_mdp_wrot"
#define	mm_disp0_fake_eng	"mm_disp0_fake_eng"
#define	mm_disp0_disp_ovl0	"mm_disp0_disp_ovl0"
#define	mm_disp0_disp_ovl1	"mm_disp0_disp_ovl1"
#define	mm_disp0_disp_rdma0	"mm_disp0_disp_rdma0"
#define	mm_disp0_disp_rdma1	"mm_disp0_disp_rdma1"
#define	mm_disp0_disp_wdma0	"mm_disp0_disp_wdma0"
#define	mm_disp0_disp_color	"mm_disp0_disp_color"
#define	mm_disp0_disp_ccorr	"mm_disp0_disp_ccorr"
#define	mm_disp0_disp_aal	"mm_disp0_disp_aal"
#define	mm_disp0_disp_gamma	"mm_disp0_disp_gamma"
#define	mm_disp0_disp_dither	"mm_disp0_disp_dither"
#define	mm_disp0_mdp_color	"mm_disp0_mdp_color"
#define	mm_disp0_ufoe_mout	"mm_disp0_ufoe_mout"
#define	mm_disp0_disp_wdma1	"mm_disp0_disp_wdma1"
#define	mm_disp0_disp_2lovl0	"mm_disp0_disp_2lovl0"
#define	mm_disp0_disp_2lovl1	"mm_disp0_disp_2lovl1"
#define	mm_disp0_disp_ovl0mout	"mm_disp0_disp_ovl0mout"
#define	mm_disp1_dsi_engine	"mm_disp1_dsi_engine"
#define	mm_disp1_dsi_digital	"mm_disp1_dsi_digital"
#define	mm_disp1_dpi_pixel	"mm_disp1_dpi_pixel"
#define	mm_disp1_dpi_engine	"mm_disp1_dpi_engine"
#if 0
#define		""
#define		""
#define		""
#define		""
#define		""
#define		""
#define		""
#define		""
#define		""
#define		""
#endif
#if 0
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
#endif
/* VDEC */
#define vdec0_vdec		"vdec0_vdec"
#define vdec1_larb		"vdec1_larb"
#define vdec1_larb_m4u		"vdec1_larb_m4u"
#define vdec1_larb_smi_common		"vdec1_larb_smi_common"
#define vdec1_larb_met_smi_common		"vdec1_larb_met_smi_common"
#define vdec1_larb_vdec_gcon		"vdec1_larb_vdec_gcon"

/* VENC */
#define venc_larb		"venc_larb"
#define venc_larb_m4u		"venc_larb_m4u"
#define venc_larb_smi_common		"venc_larb_smi_common"
#define venc_larb_met_smi_common		"venc_larb_met_smi_common"
#define venc_larb_jpgenc		"venc_larb_jpgenc"
#define venc_larb_jpgdec		"venc_larb_jpgdec"

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
#define	audio_apll1div0	"audio_apll1div0"
#define	audio_apll2div0	"audio_apll2div0"


#ifdef CONFIG_OF
void __iomem  *apmixed_base;
void __iomem  *cksys_base;
void __iomem  *infracfg_base;
void __iomem  *audio_base;
void __iomem  *mfgcfg_base;
void __iomem  *mmsys_config_base;
void __iomem  *img_base;
void __iomem  *vdec_gcon_base;
void __iomem  *venc_gcon_base;
void __iomem  *scp_base;
/*PLL INIT*/
#define ARMCA15PLL_CON0					(apmixed_base + 0x200)
#define ARMCA7PLL_CON0					(apmixed_base + 0x210)
#define MAINPLL_CON0						(apmixed_base + 0x220)
#define UNIVPLL_CON0						(apmixed_base + 0x230)
#define MMPLL_CON0							(apmixed_base + 0x240)
#define MSDCPLL_CON0            (apmixed_base + 0x250)
#define MSDCPLL_PWR_CON0        (apmixed_base + 0x25C)
#define VENCPLL_CON0			(apmixed_base + 0x260)
#define TVDPLL_CON0             (apmixed_base + 0x270)
#define TVDPLL_PWR_CON0         (apmixed_base + 0x27C)
#define APLL1_CON0              (apmixed_base + 0x2A0)
#define APLL1_PWR_CON0          (apmixed_base + 0x2B0)
#define APLL2_CON0              (apmixed_base + 0x2B4)
#define APLL2_PWR_CON0          (apmixed_base + 0x2C4)
#define INFRA_PDN_SET0          (infracfg_base + 0x0080)
#define INFRA_PDN_SET1          (infracfg_base + 0x0088)
#define INFRA_PDN_SET2          (infracfg_base + 0x00A4)
#define MFG_CG_SET              (mfgcfg_base + 4)
#define IMG_CG_SET              (img_base + 0x0004)
#define DISP_CG_SET0            (mmsys_config_base + 0x104)
#define VDEC_CKEN_CLR           (vdec_gcon_base + 0x0004)
#define LARB_CKEN_CLR           (vdec_gcon_base + 0x000C)
#define VENC_CG_CON             (venc_gcon_base + 0x0)
#define AUDIO_TOP_CON0          (audio_base + 0x0000)
#define ULPOSC_CON							(scp_base + 0x0458)
#endif

#define INFRA0_CG  0x832ff910/*0: Disable  ( with clock), 1: Enable ( without clock )*/
#define INFRA1_CG  0x6f0802/*0: Disable  ( with clock), 1: Enable ( without clock )*/
#define INFRA2_CG  0xc1/*0: Disable  ( with clock), 1: Enable ( without clock )*/
#define AUD_CG    0x0F0C0344/*same as denali*/
#define MFG_CG    0x00000001/*set*/
#define DISP0_CG  0xfffffffc
#define DISP1_CG  0x0000003F
#define IMG_CG    0x00000FE1/*no bit10	SUFOD_CKPDN*/
#define VDEC_CG   0x00000001/*set*/
#define LARB_CG   0x00000001
#define VENC_CG   0x00001111/*set*/

#define CG_BOOTUP_PDN			1
#define ULPOSC_EN BIT(0)
#define ULPOSC_RST BIT(1)
#define ULPOSC_CG_EN BIT(2)

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
	FACTOR(TOP_AD_APLL1_CK, apll1_ck, apll1, 1, 1),
	FACTOR(TOP_AD_APLL2_CK, apll2_ck, apll2, 1, 1),
	/*FACTOR(TOP_AD_SYS_26M_CK, ad_sys_26m_ck, clk26m, 1, 1),
	FACTOR(TOP_AD_SYS_26M_D2, ad_sys_26m_d2, clk26m, 1, 1),*/
	FACTOR(TOP_DMPLL_CK, dmpll_ck, clkph_mck_o, 1, 1),
	/*FACTOR(TOP_DMPLL_D2, dmpll_d2, clkph_mck_o, 1, 1),
	FACTOR(TOP_DMPLL_D4, dmpll_d4, clkph_mck_o, 1, 1),
	FACTOR(TOP_DMPLL_D8, dmpll_d8, clkph_mck_o, 1, 1),*/

	FACTOR(TOP_MMPLL_CK, mmpll_ck, mmpll, 1, 1),
	/*FACTOR(TOP_MPLL_208M_CK, mmpll_208m_ck, mmpll, 1, 1),used for scpsys, no use*/
#if 1
	FACTOR(TOP_OSC_CK, osc_ck, oscpll, 1, 1),
	FACTOR(TOP_OSC_D2, osc_d2, oscpll, 1, 2),
	FACTOR(TOP_OSC_D4, osc_d4, oscpll, 1, 4),
	FACTOR(TOP_OSC_D8, osc_d8, oscpll, 1, 8),
#else
	FACTOR(TOP_OSC_D2, osc_d2, clk_null, 1, 1),
	FACTOR(TOP_OSC_D4, osc_d4, clk_null, 1, 1),
	FACTOR(TOP_OSC_D8, osc_d8, clk_null, 1, 1),
#endif
	FACTOR(TOP_MSDCPLL_CK, msdcpll_ck, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D16, msdcpll_d16, msdcpll, 1, 16),
	FACTOR(TOP_MSDCPLL_D2, msdcpll_d2, msdcpll, 1, 2),
	FACTOR(TOP_MSDCPLL_D4, msdcpll_d4, msdcpll, 1, 4),
	FACTOR(TOP_MSDCPLL_D8, msdcpll_d8, msdcpll, 1, 8),


	FACTOR(TOP_SYSPLL_CK, syspll_ck, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL_D3, syspll_d3, mainpll, 1, 3),
	FACTOR(TOP_SYSPLL_D3_D3, syspll_d3_d3, mainpll, 1, 9),
	FACTOR(TOP_SYSPLL_D5, syspll_d5, mainpll, 1, 5),
	FACTOR(TOP_SYSPLL_D7, syspll_d7, mainpll, 1, 7),

	FACTOR(TOP_SYSPLL1_CK, syspll1_ck, mainpll, 1, 2),
	FACTOR(TOP_SYSPLL1_D16, syspll1_d16, syspll1_ck, 1, 16),
	FACTOR(TOP_SYSPLL1_D2, syspll1_d2, syspll1_ck, 1, 2),
	FACTOR(TOP_SYSPLL1_D4, syspll1_d4, syspll1_ck, 1, 4),
	FACTOR(TOP_SYSPLL1_D8, syspll1_d8, syspll1_ck, 1, 8),

	FACTOR(TOP_SYSPLL2_CK, syspll2_ck, mainpll, 1, 3),
	FACTOR(TOP_SYSPLL2_D2, syspll2_d2, syspll2_ck, 1, 2),
	FACTOR(TOP_SYSPLL2_D4, syspll2_d4, syspll2_ck, 1, 4),

	FACTOR(TOP_SYSPLL3_CK, syspll3_ck, mainpll, 1, 5),
	FACTOR(TOP_SYSPLL3_D2, syspll3_d2, syspll3_ck, 1, 2),
	FACTOR(TOP_SYSPLL3_D4, syspll3_d4, syspll3_ck, 1, 4),

	FACTOR(TOP_SYSPLL4_CK, syspll4_ck, mainpll, 1, 7),
	FACTOR(TOP_SYSPLL4_D2, syspll4_d2, syspll4_ck, 1, 2),
	FACTOR(TOP_SYSPLL4_D4, syspll4_d4, syspll4_ck, 1, 4),

	FACTOR(TOP_TVDPLL_CK, tvdpll_ck, tvdpll, 1, 1),
	FACTOR(TOP_TVDPLL_D2, tvdpll_d2, tvdpll, 1, 2),
	FACTOR(TOP_TVDPLL_D4, tvdpll_d4, tvdpll, 1, 4),
	FACTOR(TOP_TVDPLL_D8, tvdpll_d8, tvdpll, 1, 8),
	FACTOR(TOP_TVDPLL_D16, tvdpll_d16, tvdpll, 1, 16),


	FACTOR(TOP_UNIVPLL_CK, univpll_ck, univpll, 1, 1),
	FACTOR(TOP_UNIVPLL_D26, univpll_d26, univpll, 1, 26),
	FACTOR(TOP_UNIVPLL_D7, univpll_d7, univpll, 1, 7),
	FACTOR(TOP_UNIVPLL_D5, univpll_d5, univpll, 1, 5),
	FACTOR(TOP_UNIVPLL_D3, univpll_d3, univpll, 1, 3),
	FACTOR(TOP_UNIVPLL_D2, univpll_d2, univpll, 1, 2),

	FACTOR(TOP_UNIVPLL1_CK, univpll1_ck, univpll, 1, 2),
	FACTOR(TOP_UNIVPLL1_D2, univpll1_d2, univpll1_ck, 1, 2),
	FACTOR(TOP_UNIVPLL1_D4, univpll1_d4, univpll1_ck, 1, 4),
	FACTOR(TOP_UNIVPLL1_D8, univpll1_d8, univpll1_ck, 1, 8),

	FACTOR(TOP_UNIVPLL2_CK, univpll2_ck, univpll, 1, 3),
	FACTOR(TOP_UNIVPLL2_D2, univpll2_d2, univpll2_ck, 1, 2),
	FACTOR(TOP_UNIVPLL2_D4, univpll2_d4, univpll2_ck, 1, 4),
	FACTOR(TOP_UNIVPLL2_D8, univpll2_d8, univpll2_ck, 1, 8),

	FACTOR(TOP_UNIVPLL3_CK, univpll3_ck, univpll, 1, 5),
	FACTOR(TOP_UNIVPLL3_D2, univpll3_d2, univpll3_ck, 1, 2),
	FACTOR(TOP_UNIVPLL3_D4, univpll3_d4, univpll3_ck, 1, 4),

	FACTOR(TOP_VENCPLL_CK, vencpll_ck, vencpll, 1, 1),
	FACTOR(INFRA_CLK_13M, clk13m, clk26m, 1, 2),
	/*FACTOR(TOP_VENCPLL_D3, vencpll_d3, vencpll, 1, 1),
	FACTOR(TOP_WHPLL_AUDIO_CK, whpll_audio_ck, clk_null, 1, 1),*/
};

static void __init init_clk_top_div(struct clk_onecell_data *clk_data)
{
	init_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);
}

static const char *axi_parents[] __initconst = {
		clk26m,
		syspll1_d4,
		syspll2_d2,
		osc_d8};

static const char *mem_parents[] __initconst = {
		dmpll_ck};

static const char *ddrphycfg_parents[] __initconst = {
		clk26m,
		syspll1_d8};

static const char *mm_parents[] __initconst = {
		clk26m,
		syspll_d3,
		vencpll_ck,
		syspll1_d2,
		syspll2_d2,
		syspll_d7,
		syspll2_d4,
		syspll4_d2};

static const char *pwm_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		univpll2_d4};

static const char *vdec_parents[] __initconst = {
		clk26m,
		univpll1_d2,
		vencpll_ck,
		univpll_d5,
		syspll1_d4};

static const char *mfg_parents[] __initconst = {
		clk26m,
		mmpll_ck,
		univpll_d3,
		syspll_d3
};

static const char *camtg_parents[] __initconst = {
		clk26m,
		univpll_d26,
		univpll2_d2,
};

static const char *uart_parents[] __initconst = {
		clk26m,
		univpll2_d8
};

static const char *spi_parents[] __initconst = {
		clk26m,
		syspll3_d2,
		syspll2_d4,
		msdcpll_d4
};
/*
static const char *usb20_parents[] __initconst = {
		clk26m,
		univpll1_d8,
		univpll3_d4
};
*/

static const char *msdc50_0_hclk_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll2_d2,
		syspll4_d2
};

static const char *msdc50_0_parents[] __initconst = {
		clk26m,
		msdcpll_ck,
		msdcpll_d2,
		univpll1_d4,
		syspll2_d2,
		syspll_d7,
		msdcpll_d4,
		univpll_d2,
		univpll1_d2
};

/*
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
*/
static const char *msdc30_1_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		msdcpll_d4,
		univpll1_d4,
		syspll2_d2,
		syspll_d7,
		univpll_d7,
		msdcpll_d2
};

static const char *msdc30_2_parents[] __initconst = {
		clk26m,
		univpll2_d2,
		msdcpll_d4,
		univpll1_d4,
		syspll2_d2,
		syspll_d7,
		univpll_d7,
		msdcpll_d2
};

static const char *msdc30_3_parents[] __initconst = {
		clk26m,
		msdcpll_d8,
		msdcpll_d4,
		univpll1_d4,
		univpll_d26,
		syspll_d7,
		univpll_d7,
		syspll3_d4,
		msdcpll_d16
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
		syspll4_d2
};

static const char *pmicspi_parents[] __initconst = {
		clk26m,
		syspll1_d8,
		osc_d4
};
/*
static const char *scp_parents[] __initconst = {
		clk26m,
		syspll1_d8,
		dmpll_d2,
		dmpll_d4
};
*/
static const char *atb_parents[] __initconst = {
		clk26m,
		syspll1_d2,
		syspll_d5
};

static const char *dpi0_parents[] __initconst = {
		clk26m,
		tvdpll_d2,
		tvdpll_d4,
		tvdpll_d8,
		tvdpll_d16
};

static const char *scam_parents[] __initconst = {
		clk26m,
		syspll3_d2
};
/*
static const char *mfg13m_parents[] __initconst = {
		clk26m,
		ad_sys_
};
*/
static const char *aud_1_parents[] __initconst = {
		clk26m,
		apll1_ck
};

static const char *aud_2_parents[] __initconst = {
		clk26m,
		apll2_ck
};
/*
static const char *irda_parents[] __initconst = {
		clk26m,
		univpll2_d4
};

static const char *irtx_parents[] __initconst = {
		clk26m,
		ad_sys_26m_ck
};
*/
static const char *disppwm_parents[] __initconst = {
		clk26m,
		univpll2_d4,
		osc_d2,
		osc_d8
};
/* extra 6 */

static const char *ssusb_top_sys_parents[] __initconst = {
		clk26m,
		univpll3_d2
};

static const char *usb_top_parents[] __initconst = {
		clk26m,
		univpll3_d4
};

static const char *spm_parents[] __initconst = {
		clk26m,
		syspll1_d8
};

static const char *bsi_spi_parents[] __initconst = {
		clk26m,
		syspll_d3_d3,
		syspll1_d4,
		syspll_d7
};

static const char *i2c_parents[] __initconst = {
		clk26m,
		syspll1_d8,
		univpll3_d4
};
static const char *dvfsp_parents[] __initconst = {
		clk26m,
		syspll1_d8
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
	/* CLK_CFG_0 */
	MUX(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 8, 2, INVALID_MUX_GATE_BIT),
	/*why not 23 pdn bit*/
	MUX(TOP_MUX_DDRPHY, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 16, 1, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 24, 3, 31),
	/* CLK_CFG_1 */
	MUX(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0, 2, 7),
	MUX(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 8, 3, 15),
	MUX(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 24, 2, 31),
	/* CLK_CFG_2 */
	MUX(TOP_MUX_CAMTG, camtg_sel, camtg_parents, 0x0060, 0, 2, 7),
	MUX(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 8, 1, 15),
	MUX(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 16, 2, 23),
	/* CLK_CFG_3 */
	MUX(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, 0x0070, 8, 2, 15),
	MUX(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 16, 4, 23),
	MUX(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 24, 4, 31),
	/* CLK_CFG_4 */
	MUX(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0, 4, 7),
	MUX(TOP_MUX_MSDC30_3, msdc30_3_sel, msdc30_3_parents, 0x0080, 8, 4, 15),
	MUX(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 16, 2, 23),
	MUX(TOP_MUX_AUDINTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 24, 2, 31),
	/* CLK_CFG_5 */
	MUX(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0, 2, INVALID_MUX_GATE_BIT),
	/*MUX(TOP_MUX_SCP, scp_sel, scp_parents, 0x0090, 8, 2, 15),*/
	MUX(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 16, 2, 23),
	/*MUX(TOP_MUX_MJC, mjc_sel, mjc_parents, 0x0090, 24, 2, 31),*/
	/* CLK_CFG_6 */
	MUX(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00a0, 0, 3, 7),
	MUX(TOP_MUX_SCAM, scam_sel, scam_parents, 0x00a0, 8, 1, 15),
	MUX(TOP_MUX_AUD1, aud_1_sel, aud_1_parents, 0x00a0, 16, 1, 23),
	MUX(TOP_MUX_AUD2, aud_2_sel, aud_2_parents, 0x00a0, 24, 1, 31),
/* CLK_CFG_7 */
	MUX(TOP_MUX_DISPPWM, disppwm_sel, disppwm_parents, 0x00b0, 0, 2, 7),
	MUX(TOP_MUX_SSUSBTOPSYS, ssusb_top_sys_sel, ssusb_top_sys_parents, 0x00b0, 8, 1, 15),
	MUX(TOP_MUX_USBTOP, usb_top_sel, usb_top_parents, 0x00b0, 24, 1, 31),
	/* CLK_CFG_8 */
	MUX(TOP_MUX_SPM, spm_sel, spm_parents, 0x00c0, 0, 1, 7),
	MUX(TOP_MUX_BSISPI, bsi_spi_sel, bsi_spi_parents, 0x00c0, 8, 2, 15),
	MUX(TOP_MUX_I2C, i2c_sel, i2c_parents, 0x00c0, 16, 2, INVALID_MUX_GATE_BIT),
	MUX(TOP_MUX_DVFSP, dvfsp_sel, dvfsp_parents, 0x00c0, 24, 1, 31),
};
#else

#define _UPDATE_REG 0x04
#define _UPDATE1_REG 0x08
#define INVALID_UPDATE_REG 0xFFFFFFFF
#define INVALID_UPDATE_SHIFT -1
#define INVALID_MUX_GATE -1
#ifdef _MUX_CLR_SET_UPDS_
static struct mtk_mux_clr_set_upd top_muxes[] __initdata = {
	/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0x0044, 0x0048, 0, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 0),
	MUX_CLR_SET_UPD(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 0x0044, 0x0048, 8, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 1),
	/*why not 23 pdn bit*/
	MUX_CLR_SET_UPD(TOP_MUX_DDRPHY, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 0x0044, 0x0048, 16, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 2),
	MUX_CLR_SET_UPD(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 0x0044, 0x0048, 24, 3,
	INVALID_MUX_GATE, _UPDATE_REG, 3),
	/* CLK_CFG_1 */
	MUX_CLR_SET_UPD(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0x0054, 0x0058, 0, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 4),
	MUX_CLR_SET_UPD(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 0x0054, 0x0058, 8, 3,
	INVALID_MUX_GATE, _UPDATE_REG, 5),
	MUX_CLR_SET_UPD(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 0x0054, 0x0058, 24, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 7),
	/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG, camtg_sel, camtg_parents, 0x0060, 0x0064, 0x0068, 0, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 8),
	MUX_CLR_SET_UPD(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 0x0064, 0x0068, 8, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 9),
	MUX_CLR_SET_UPD(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 0x0064, 0x0068, 16, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 10),
	/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, 0x0070, 0x0074, 0x0078, 8, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 12),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 0x0074, 0x0078, 16, 4,
	INVALID_MUX_GATE, _UPDATE_REG, 13),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 0x0074, 0x0078, 24, 4,
	INVALID_MUX_GATE, _UPDATE_REG, 14),
	/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0x0084, 0x0088, 0, 4,
	INVALID_MUX_GATE, _UPDATE_REG, 15),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_3, msdc30_3_sel, msdc30_3_parents, 0x0080, 0x0084, 0x0088, 8, 4,
	INVALID_MUX_GATE, _UPDATE_REG, 16),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 0x0084, 0x0088, 16, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 17),
	MUX_CLR_SET_UPD(TOP_MUX_AUDINTBUS, aud_intbus_sel, aud_intbus_parents, 0x0080, 0x0084, 0x0088, 24, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 18),
	/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0x0094, 0x0098, 0, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 19),
	MUX_CLR_SET_UPD(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 0x0094, 0x0098, 16, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 21),
	/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00a0, 0x00a4, 0x00a8, 0, 3,
	INVALID_MUX_GATE, _UPDATE_REG, 23),
	MUX_CLR_SET_UPD(TOP_MUX_SCAM, scam_sel, scam_parents, 0x00a0, 0x00a4, 0x00a8, 8, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 24),
	MUX_CLR_SET_UPD(TOP_MUX_AUD1, aud_1_sel, aud_1_parents, 0x00a0, 0x00a4, 0x00a8, 16, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 25),
	MUX_CLR_SET_UPD(TOP_MUX_AUD2, aud_2_sel, aud_2_parents, 0x00a0, 0x00a4, 0x00a8, 24, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 26),
	/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(TOP_MUX_DISPPWM, disppwm_sel, disppwm_parents, 0x00b0, 0x00b4, 0x00b8, 0, 2,
	INVALID_MUX_GATE, _UPDATE_REG, 27),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSBTOPSYS, ssusb_top_sys_sel, ssusb_top_sys_parents, 0x00b0, 0x00b4, 0x00b8, 8, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 28),
	MUX_CLR_SET_UPD(TOP_MUX_USBTOP, usb_top_sel, usb_top_parents, 0x00b0, 0x00b4, 0x00b8, 24, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 30),
	/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(TOP_MUX_SPM, spm_sel, spm_parents, 0x00c0, 0x00c4, 0x00c8, 0, 1,
	INVALID_MUX_GATE, _UPDATE_REG, 31),
	MUX_CLR_SET_UPD(TOP_MUX_BSISPI, bsi_spi_sel, bsi_spi_parents, 0x00c0, 0x00c4, 0x00c8, 8, 2,
	INVALID_MUX_GATE, _UPDATE1_REG, 0),
	MUX_CLR_SET_UPD(TOP_MUX_I2C, i2c_sel, i2c_parents, 0x00c0, 0x00c4, 0x00c8, 16, 2,
	INVALID_MUX_GATE, _UPDATE1_REG, 1),
	MUX_CLR_SET_UPD(TOP_MUX_DVFSP, dvfsp_sel, dvfsp_parents, 0x00c0, 0x00c4, 0x00c8, 24, 1,
	INVALID_MUX_GATE, _UPDATE1_REG, 2),
};
#else
static struct mtk_mux_upd top_muxes[] __initdata = {
	/* CLK_CFG_0 */
	MUX_UPD(TOP_MUX_AXI, axi_sel, axi_parents, 0x0040, 0, 2, INVALID_MUX_GATE, _UPDATE_REG, 0),
	MUX_UPD(TOP_MUX_MEM, mem_sel, mem_parents, 0x0040, 8, 2, INVALID_MUX_GATE, _UPDATE_REG, 1),
	/*why not 23 pdn bit*/
	MUX_UPD(TOP_MUX_DDRPHY, ddrphycfg_sel, ddrphycfg_parents, 0x0040, 16, 1, INVALID_MUX_GATE, _UPDATE_REG, 2),
	MUX_UPD(TOP_MUX_MM, mm_sel, mm_parents, 0x0040, 24, 3, INVALID_MUX_GATE, _UPDATE_REG, 3),
	/* CLK_CFG_1 */
	MUX_UPD(TOP_MUX_PWM, pwm_sel, pwm_parents, 0x0050, 0, 2, INVALID_MUX_GATE, _UPDATE_REG, 4),
	MUX_UPD(TOP_MUX_VDEC, vdec_sel, vdec_parents, 0x0050, 8, 3, INVALID_MUX_GATE, _UPDATE_REG, 5),
	MUX_UPD(TOP_MUX_MFG, mfg_sel, mfg_parents, 0x0050, 24, 2, INVALID_MUX_GATE, _UPDATE_REG, 7),
	/* CLK_CFG_2 */
	MUX_UPD(TOP_MUX_CAMTG, camtg_sel, camtg_parents, 0x0060, 0, 2, INVALID_MUX_GATE, _UPDATE_REG, 8),
	MUX_UPD(TOP_MUX_UART, uart_sel, uart_parents, 0x0060, 8, 1, INVALID_MUX_GATE, _UPDATE_REG, 9),
	MUX_UPD(TOP_MUX_SPI, spi_sel, spi_parents, 0x0060, 16, 2, INVALID_MUX_GATE, _UPDATE_REG, 10),
	/* CLK_CFG_3 */
	MUX_UPD(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents,
		0x0070, 8, 2, INVALID_MUX_GATE, _UPDATE_REG, 12),
	MUX_UPD(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, 0x0070, 16, 4, INVALID_MUX_GATE, _UPDATE_REG, 13),
	MUX_UPD(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, 0x0070, 24, 4, INVALID_MUX_GATE, _UPDATE_REG, 14),
	/* CLK_CFG_4 */
	MUX_UPD(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, 0x0080, 0, 4, INVALID_MUX_GATE, _UPDATE_REG, 15),
	MUX_UPD(TOP_MUX_MSDC30_3, msdc30_3_sel, msdc30_3_parents, 0x0080, 8, 4, INVALID_MUX_GATE, _UPDATE_REG, 16),
	MUX_UPD(TOP_MUX_AUDIO, audio_sel, audio_parents, 0x0080, 16, 2, INVALID_MUX_GATE, _UPDATE_REG, 17),
	MUX_UPD(TOP_MUX_AUDINTBUS, aud_intbus_sel, aud_intbus_parents,
		0x0080, 24, 2, INVALID_MUX_GATE, _UPDATE_REG, 18),
	/* CLK_CFG_5 */
	MUX_UPD(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, 0x0090, 0, 2, INVALID_MUX_GATE, _UPDATE_REG, 19),
	/*MUX(TOP_MUX_SCP, scp_sel, scp_parents, 0x0090, 8, 2, 15),*/
	MUX_UPD(TOP_MUX_ATB, atb_sel, atb_parents, 0x0090, 16, 2, INVALID_MUX_GATE, _UPDATE_REG, 21),
	/*MUX(TOP_MUX_MJC, mjc_sel, mjc_parents, 0x0090, 24, 2, 31),*/
	/* CLK_CFG_6 */
	MUX_UPD(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, 0x00a0, 0, 3, INVALID_MUX_GATE, _UPDATE_REG, 23),
	MUX_UPD(TOP_MUX_SCAM, scam_sel, scam_parents, 0x00a0, 8, 1, INVALID_MUX_GATE, _UPDATE_REG, 24),
	MUX_UPD(TOP_MUX_AUD1, aud_1_sel, aud_1_parents, 0x00a0, 16, 1, INVALID_MUX_GATE, _UPDATE_REG, 25),
	MUX_UPD(TOP_MUX_AUD2, aud_2_sel, aud_2_parents, 0x00a0, 24, 1, INVALID_MUX_GATE, _UPDATE_REG, 26),
	/* CLK_CFG_7 */
	MUX_UPD(TOP_MUX_DISPPWM, disppwm_sel, disppwm_parents, 0x00b0, 0, 2, INVALID_MUX_GATE, _UPDATE_REG, 27),
	MUX_UPD(TOP_MUX_SSUSBTOPSYS, ssusb_top_sys_sel, ssusb_top_sys_parents,
		0x00b0, 8, 1, INVALID_MUX_GATE, _UPDATE_REG, 28),
	MUX_UPD(TOP_MUX_USBTOP, usb_top_sel, usb_top_parents, 0x00b0, 24, 1, INVALID_MUX_GATE, _UPDATE_REG, 30),
	/* CLK_CFG_8 */
	MUX_UPD(TOP_MUX_SPM, spm_sel, spm_parents, 0x00c0, 0, 1, INVALID_MUX_GATE, _UPDATE_REG, 31),
	MUX_UPD(TOP_MUX_BSISPI, bsi_spi_sel, bsi_spi_parents, 0x00c0, 8, 2, INVALID_MUX_GATE, _UPDATE1_REG, 0),
	MUX_UPD(TOP_MUX_I2C, i2c_sel, i2c_parents, 0x00c0, 16, 2, INVALID_MUX_GATE, _UPDATE1_REG, 1),
	MUX_UPD(TOP_MUX_DVFSP, dvfsp_sel, dvfsp_parents, 0x00c0, 24, 1, INVALID_MUX_GATE, _UPDATE1_REG, 2),
};
#endif
#endif

#ifndef _MUX_UPDS_
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
/*ten pll*/
static struct mtk_pll plls[] __initdata = {
	PLL(APMIXED_ARMBPLL, armbpll, clk26m, 0x0200, 0x020C, 0x00000001, HAVE_PLL_HP, &mt_clk_arm_pll_ops),
	PLL(APMIXED_ARMSPLL, armspll, clk26m, 0x0210, 0x021C, 0x00000001, HAVE_PLL_HP, &mt_clk_arm_pll_ops),
	PLL(APMIXED_MAINPLL, mainpll, clk26m, 0x0220, 0x022C, 0xF0000101, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_UNIVPLL, univpll, clk26m, 0x0230, 0x023C, 0xFC000001, HAVE_FIX_FRQ, &mt_clk_univ_pll_ops),
	PLL(APMIXED_MMPLL, mmpll, clk26m, 0x0240, 0x024C, 0x00000001, HAVE_PLL_HP, &mt_clk_mm_pll_ops),
	PLL(APMIXED_MSDCPLL, msdcpll, clk26m, 0x0250, 0x025C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_VENCPLL, vencpll, clk26m, 0x0260, 0x026C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	PLL(APMIXED_TVDPLL, tvdpll, clk26m, 0x0270, 0x027C, 0x00000001, HAVE_PLL_HP, &mt_clk_sdm_pll_ops),
	/*No.28 JADE NO MPLL*/
	/*No.29 JADE NO VCODEPLL? not list in PLL SPEC*/
	PLL(APMIXED_APLL1, apll1, clk26m, 0x02A0, 0x02B0, 0x00000001, HAVE_FIX_FRQ, &mt_clk_aud_pll_ops),
	PLL(APMIXED_APLL2, apll2, clk26m, 0x02B4, 0x02C4, 0x00000001, HAVE_FIX_FRQ, &mt_clk_aud_pll_ops),
	PLL(SCP_OSCPLL, oscpll, clk26m, 0x0458, 0x0458, 0x00000001, HAVE_FIX_FRQ, &mt_clk_spm_pll_ops),
	/*PLL(APMIXED_ARMPLL, armpll, clk26m, 0x0200, 0x020C, 0x00000001, HAVE_PLL_HP, &mt_clk_arm_pll_ops),*/
};

static void __init init_clk_apmixedsys(void __iomem *apmixed_base, void __iomem *spm_base,
		struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(plls); i++) {
		struct mtk_pll *pll = &plls[i];
			if (!strcmp("oscpll", pll->name)) {
				clk = mtk_clk_register_pll(pll->name, pll->parent_name,
				spm_base + pll->reg,
				spm_base + pll->pwr_reg,
				pll->en_mask, pll->flags, pll->ops);
			} else {
				clk = mtk_clk_register_pll(pll->name, pll->parent_name,
				apmixed_base + pll->reg,
				apmixed_base + pll->pwr_reg,
				pll->en_mask, pll->flags, pll->ops);
			}

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
	.set_ofs = 0x00a4,
	.clr_ofs = 0x00a8,
	.sta_ofs = 0x00ac,
};

static struct mtk_gate infra_clks[] __initdata = {
	/* INFRA0 */
	GATE(INFRA_PMIC_TMR, infra_pmictmr, pmicspi_sel, infra0_cg_regs, 0, 0),
	GATE(INFRA_PMIC_AP, infra_pmicap, pmicspi_sel, infra0_cg_regs, 1, 0),
	GATE(INFRA_PMIC_MD, infra_pmicmd, pmicspi_sel, infra0_cg_regs, 2, 0),
	GATE(INFRA_PMIC_CONN, infra_pmicconn, pmicspi_sel, infra0_cg_regs, 3, 0),
	/*GATE(INFRA_SCPSYS, infra_scpsys, null, infra0_cg_regs, 4, 0),*/
	GATE(INFRA_SEJ, infra_sej, f26m_sel, infra0_cg_regs, 5, 0),/*?*/
	GATE(INFRA_APXGPT, infra_apxgpt, axi_sel, infra0_cg_regs, 6, 0),
	GATE(INFRA_ICUSB, infra_icusb, axi_sel, infra0_cg_regs, 8, 0),
	GATE(INFRA_GCE, infra_gce, axi_sel, infra0_cg_regs, 9, 0),
	GATE(INFRA_THERM, infra_therm, axi_sel, infra0_cg_regs, 10, 0),
	GATE(INFRA_I2C0, infra_i2c0, i2c_sel, infra0_cg_regs, 11, 0),
	GATE(INFRA_I2C1, infra_i2c1, i2c_sel, infra0_cg_regs, 12, 0),
	GATE(INFRA_I2C2, infra_i2c2, i2c_sel, infra0_cg_regs, 13, 0),
	GATE(INFRA_I2C3, infra_i2c3, i2c_sel, infra0_cg_regs, 14, 0),
	GATE(INFRA_PWM_HCLK, infra_pwmhclk, axi_sel, infra0_cg_regs, 15, 0),
	GATE(INFRA_PWM1, infra_pwm1, i2c_sel, infra0_cg_regs, 16, 0),
	GATE(INFRA_PWM2, infra_pwm2, i2c_sel, infra0_cg_regs, 17, 0),
	GATE(INFRA_PWM3, infra_pwm3, i2c_sel, infra0_cg_regs, 18, 0),
	GATE(INFRA_PWM4, infra_pwm4, i2c_sel, infra0_cg_regs, 19, 0),
	GATE(INFRA_PWM, infra_pwm, i2c_sel, infra0_cg_regs, 21, 0),
	GATE(INFRA_UART0, infra_uart0, uart_sel, infra0_cg_regs, 22, 0),
	GATE(INFRA_UART1, infra_uart1, uart_sel, infra0_cg_regs, 23, 0),
	GATE(INFRA_UART2, infra_uart2, uart_sel, infra0_cg_regs, 24, 0),
	GATE(INFRA_UART3, infra_uart3, uart_sel, infra0_cg_regs, 25, 0),
	GATE(INFRA_MD2MD_CCIF0, infra_md2mdccif0, axi_sel, infra0_cg_regs, 27, 0),
	GATE(INFRA_MD2MD_CCIF1, infra_md2mdccif1, axi_sel, infra0_cg_regs, 28, 0),
	GATE(INFRA_MD2MD_CCIF2, infra_md2mdccif2, axi_sel, infra0_cg_regs, 29, 0),
	GATE(INFRA_BTIF, infra_btif, axi_sel, infra0_cg_regs, 31, 0),
	/* INFRA1 */
	GATE(INFRA_MD2MD_CCIF3, infra_md2mdccif3, axi_sel, infra1_cg_regs, 0, 0),
	GATE(INFRA_SPI0, infra_spi0, spi_sel, infra1_cg_regs, 1, 0),
	GATE(INFRA_MSDC0, infra_msdc0, msdc50_0_sel, infra1_cg_regs, 2, 0),
	GATE(INFRA_MD2MD_CCIF4, infra_md2mdccif4, axi_sel, infra1_cg_regs, 3, 0),
	GATE(INFRA_MSDC1, infra_msdc1, msdc30_1_sel, infra1_cg_regs, 4, 0),
	GATE(INFRA_MSDC2, infra_msdc2, msdc30_2_sel, infra1_cg_regs, 5, 0),
	GATE(INFRA_MSDC3, infra_msdc3, msdc30_3_sel, infra1_cg_regs, 6, 0),
	GATE(INFRA_MD2MD_CCIF5, infra_md2mdccif5, axi_sel, infra1_cg_regs, 7, 0),
	GATE(INFRA_GCPU, infra_gcpu, axi_sel, infra1_cg_regs, 8, 0),
	GATE(INFRA_TRNG, infra_trng, axi_sel, infra1_cg_regs, 9, 0),
	GATE(INFRA_AUXADC, infra_auxadc, f26m_sel, infra1_cg_regs, 10, 0),
	GATE(INFRA_CPUM, infra_cpum, axi_sel, infra1_cg_regs, 11, 0),
	GATE(INFRA_CCIF1_AP, infra_ccif1ap, axi_sel, infra1_cg_regs, 12, 0),
	GATE(INFRA_CCIF1_MD, infra_ccif1md, axi_sel, infra1_cg_regs, 13, 0),
	/*GATE(INFRA_NFI, infra_nfi, axi_sel, infra1_cg_regs, 16, 0),
	GATE(INFRA_NFI_ECC, infra_nfiecc, axi_sel, infra1_cg_regs, 17, 0),*/
	GATE(INFRA_AP_DMA, infra_apdma, axi_sel, infra1_cg_regs, 18, 0),
	GATE(INFRA_XIU, infra_xiu, axi_sel, infra1_cg_regs, 19, 0),
	GATE(INFRA_DEVICE_APC, infra_deviceapc, axi_sel, infra1_cg_regs, 20, 0),
	GATE(INFRA_XIU2AHB, infra_xiu2ahb, axi_sel, infra1_cg_regs, 21, 0),
	/*GATE(INFRA_L2C_SRAM, infra_l2csram, axi_sel, infra1_cg_regs, 22, 0),*/
	GATE(INFRA_CCIF_AP, infra_ccifap, axi_sel, infra1_cg_regs, 23, 0),
	GATE(INFRA_DEBUGSYS, infra_debugsys, axi_sel, infra1_cg_regs, 24, 0),
	GATE(INFRA_AUDIO, infra_audio, axi_sel, infra1_cg_regs, 25, 0),
	GATE(INFRA_CCIF_MD, infra_ccifmd, axi_sel, infra1_cg_regs, 26, 0),
	GATE(INFRA_DRAMC_F26M, infra_dramcf26m, f26m_sel, infra1_cg_regs, 31, 0),


	/* INFRA2 */
	GATE(INFRA_IRTX, infra_irtx, f26m_sel, infra2_cg_regs, 0, 0),
	GATE(INFRA_SSUSB_TOP, infra_ssusbtop, ssusb_top_sys_sel, infra2_cg_regs, 1, 0),
	GATE(INFRA_DISP_PWM, infra_disppwm, axi_sel, infra2_cg_regs, 2, 0),
	GATE(INFRA_CLDMA_BCLK, infra_cldmabclk, clk_null, infra2_cg_regs, 3, 0),
	GATE(INFRA_AUDIO_26M_BCLK, infra_audio26mbclk, clk_null, infra2_cg_regs, 4, 0),
	GATE(INFRA_MD_TEMP_26M_BCLK, infra_mdtmp26mbclk, clk_null, infra2_cg_regs, 5, 0),
	GATE(INFRA_SPI1, infra_spi1, spi_sel, infra2_cg_regs, 6, 0),
	GATE(INFRA_I2C4, infra_i2c4, i2c_sel, infra2_cg_regs, 7, 0),
	GATE(INFRA_MD_TEMP_SHARE, infra_mdtmpshare, clk_null, infra2_cg_regs, 8, 0),

#if 0
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
#endif

};

static void __init init_clk_infrasys(void __iomem *infrasys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init infrasys gates:\n");
	init_clk_gates(infrasys_base, infra_clks, ARRAY_SIZE(infra_clks),
		clk_data);
}

#if 0
static struct mtk_gate_regs peri0_cg_regs = {
	.set_ofs = 0x0008,
	.clr_ofs = 0x0010,
	.sta_ofs = 0x0018,
};

static struct mtk_gate peri_clks[] __initdata = {

	/*GATE(PERI_AXI, peri_axi, periaxi_sel, peri0_cg_regs, 31, 0),//not perisys, periconfig*/
#if 0
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
#endif
};

static void __init init_clk_perisys(void __iomem *perisys_base,
		struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init perisys gates:\n");
	init_clk_gates(perisys_base, peri_clks, ARRAY_SIZE(peri_clks),
		clk_data);
}
#endif
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
	GATE(IMG_IMAGE_LARB2_SMI_M4U, img_image_larb2_smi_m4u, mm_sel, img_cg_regs, 0, 0),

	GATE(IMG_IMAGE_LARB2_SMI_SMI_COMMON, img_image_larb2_smi_smi_common, mm_sel, img_cg_regs, 0, 0),
	GATE(IMG_IMAGE_LARB2_SMI_MET_SMI_COMMON, img_image_larb2_smi_met_smi_common, mm_sel, img_cg_regs, 0, 0),
	GATE(IMG_IMAGE_LARB2_SMI_ISPSYS, img_image_larb2_smi_ispsys, mm_sel, img_cg_regs, 0, 0),

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
	GATE(MM_DISP0_SMI_COMMON_M4U, mm_disp0_smi_common_m4u, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_MALI, mm_disp0_smi_common_mali, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_DISPSYS, mm_disp0_smi_common_dispsys, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_SMI_COMMON, mm_disp0_smi_common_smi_common, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_MET_SMI_COMMON, mm_disp0_smi_common_met_smi_common, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_ISPSYS, mm_disp0_smi_common_ispsys, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_FDVT, mm_disp0_smi_common_fdvt, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_VDEC_GCON, mm_disp0_smi_common_vdec_gcon, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_JPGENC, mm_disp0_smi_common_jpgenc, mm_sel, mm0_cg_regs, 0, 0),
	GATE(MM_DISP0_SMI_COMMON_JPGDEC, mm_disp0_smi_common_jpgdec, mm_sel, mm0_cg_regs, 0, 0),

	GATE(MM_DISP0_SMI_LARB0, mm_disp0_smi_larb0, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_DISP0_SMI_LARB0_M4U, mm_disp0_smi_larb0_m4u, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_DISP0_SMI_LARB0_DISPSYS, mm_disp0_smi_larb0_dispsys, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_DISP0_SMI_LARB0_SMI_COMMON, mm_disp0_smi_larb0_smi_common, mm_sel, mm0_cg_regs, 1, 0),
	GATE(MM_DISP0_SMI_LARB0_MET_SMI_COMMON, mm_disp0_smi_larb0_met_smi_common, mm_sel, mm0_cg_regs, 1, 0),

	GATE(MM_DISP0_CAM_MDP, mm_disp0_cam_mdp, mm_sel, mm0_cg_regs, 2, 0),
	GATE(MM_DISP0_MDP_RDMA, mm_disp0_mdp_rdma, mm_sel, mm0_cg_regs, 3, 0),
	GATE(MM_DISP0_MDP_RSZ0, mm_disp0_mdp_rsz0, mm_sel, mm0_cg_regs, 4, 0),
	GATE(MM_DISP0_MDP_RSZ1, mm_disp0_mdp_rsz1, mm_sel, mm0_cg_regs, 5, 0),
	GATE(MM_DISP0_MDP_TDSHP, mm_disp0_mdp_tdshp, mm_sel, mm0_cg_regs, 6, 0),
	GATE(MM_DISP0_MDP_WDMA, mm_disp0_mdp_wdma, mm_sel, mm0_cg_regs, 7, 0),
	GATE(MM_DISP0_MDP_WROT, mm_disp0_mdp_wrot, mm_sel, mm0_cg_regs, 8, 0),
	GATE(MM_DISP0_FAKE_ENG, mm_disp0_fake_eng, mm_sel, mm0_cg_regs, 9, 0),
	GATE(MM_DISP0_DISP_OVL0, mm_disp0_disp_ovl0, mm_sel, mm0_cg_regs, 10, 0),
	GATE(MM_DISP0_DISP_OVL1, mm_disp0_disp_ovl1, mm_sel, mm0_cg_regs, 11, 0),
	GATE(MM_DISP0_DISP_RDMA0, mm_disp0_disp_rdma0, mm_sel, mm0_cg_regs, 12, 0),
	GATE(MM_DISP0_DISP_RDMA1, mm_disp0_disp_rdma1, mm_sel, mm0_cg_regs, 13, 0),
	GATE(MM_DISP0_DISP_WDMA0, mm_disp0_disp_wdma0, mm_sel, mm0_cg_regs, 14, 0),
	GATE(MM_DISP0_DISP_COLOR, mm_disp0_disp_color, mm_sel, mm0_cg_regs, 15, 0),
	GATE(MM_DISP0_DISP_CCORR, mm_disp0_disp_ccorr, mm_sel, mm0_cg_regs, 16, 0),
	GATE(MM_DISP0_DISP_AAL, mm_disp0_disp_aal, mm_sel, mm0_cg_regs, 17, 0),
	GATE(MM_DISP0_DISP_GAMMA, mm_disp0_disp_gamma, mm_sel, mm0_cg_regs, 18, 0),
	GATE(MM_DISP0_DISP_DITHER, mm_disp0_disp_dither, mm_sel, mm0_cg_regs, 19, 0),
	GATE(MM_DISP0_MDP_COLOR, mm_disp0_mdp_color, mm_sel, mm0_cg_regs, 20, 0),
	GATE(MM_DISP0_DISP_UFOE_MOUT, mm_disp0_ufoe_mout, mm_sel, mm0_cg_regs, 21, 0),
	GATE(MM_DISP0_DISP_WDMA1, mm_disp0_disp_wdma1, mm_sel, mm0_cg_regs, 22, 0),
	GATE(MM_DISP0_DISP_2L_OVL0, mm_disp0_disp_2lovl0, mm_sel, mm0_cg_regs, 23, 0),
	GATE(MM_DISP0_DISP_2L_OVL1, mm_disp0_disp_2lovl1, mm_sel, mm0_cg_regs, 24, 0),
	GATE(MM_DISP0_DISP_OVL0_MOUT, mm_disp0_disp_ovl0mout, mm_sel, mm0_cg_regs, 25, 0),
	/* MM1 */
	GATE(MM_DISP1_DSI_ENGINE, mm_disp1_dsi_engine, mm_sel, mm1_cg_regs, 0, 0),
	GATE(MM_DISP1_DSI_DIGITAL, mm_disp1_dsi_digital, mm_sel, mm1_cg_regs, 1, 0),
	GATE(MM_DISP1_DPI_PIXEL, mm_disp1_dpi_pixel, dpi0_sel, mm1_cg_regs, 2, 0),
	GATE(MM_DISP1_DPI_ENGINE, mm_disp1_dpi_engine, mm_sel, mm1_cg_regs, 3, 0),

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
	GATE(VDEC1_LARB_M4U, vdec1_larb_m4u, vdec_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC1_LARB_SMI_COMMON, vdec1_larb_smi_common, vdec_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC1_LARB_MET_SMI_COMMON, vdec1_larb_met_smi_common, vdec_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC1_LARB_VDEC_GCON, vdec1_larb_vdec_gcon, vdec_sel, vdec1_cg_regs, 0, CLK_GATE_INVERSE),
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
	GATE(VENC_LARB_M4U, venc_larb_m4u, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_LARB_SMI_COMMON, venc_larb_smi_common, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_LARB_MET_SMI_COMMON, venc_larb_met_smi_common, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_LARB_JPGENC, venc_larb_jpgenc, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_LARB_JPGDEC, venc_larb_jpgdec, mm_sel, venc_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_VENC, venc_venc, mm_sel, venc_cg_regs, 4, CLK_GATE_INVERSE),
	GATE(VENC_JPGENC, venc_jpgenc, mm_sel, venc_cg_regs, 8, CLK_GATE_INVERSE),
	GATE(VENC_JPGDEC, venc_jpgdec, mm_sel, venc_cg_regs, 12, CLK_GATE_INVERSE),
	/*note: clk inverse: 1: Clock enabled 0: Clock disabled*/
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

static struct mtk_gate_regs aud_div0_regs = {
/*Audio Clock Selection Register 0, 0	apll1_div0_pdn 1	apll2_div0_pdn*/
	.set_ofs = 0x05a0,
	.clr_ofs = 0x05a0,
	.sta_ofs = 0x05a0,
};

static struct mtk_gate audio_clks[] __initdata = {

	GATE(AUDIO_AFE, audio_afe, audio_sel, aud_cg_regs, 2, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_I2S, audio_i2s, i2s_sel, aud_cg_regs, 6, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_22M, audio_22m, aud_1_sel, aud_cg_regs, 8, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_24M, audio_24m, aud_2_sel, aud_cg_regs, 9, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL2_TUNER, audio_apll2_tuner, aud_2_sel, aud_cg_regs, 18, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL_TUNER, audio_apll_tuner, aud_1_sel, aud_cg_regs, 19, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_ADC, audio_adc, audio_sel, aud_cg_regs, 24, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC, audio_dac, audio_sel, aud_cg_regs, 25, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC_PREDIS, audio_dac_predis, audio_sel, aud_cg_regs, 26, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_TML, audio_tml, audio_sel, aud_cg_regs, 27, CLK_GATE_NO_SETCLR_REG),

	GATE(AUDIO_APLL1_DIV0, audio_apll1div0, clk_null, aud_div0_regs, 0, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL2_DIV0, audio_apll2div0, clk_null, aud_div0_regs, 1, CLK_GATE_NO_SETCLR_REG),

	#if 0
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
  #endif
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
#ifndef _MUX_UPDS_
	init_clk_topckgen(base, clk_data);
#else
	#ifdef _MUX_CLR_SET_UPDS_
	mtk_clk_register_mux_clr_set_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
				  &mt6755_clk_lock, clk_data);
	#else
	mtk_clk_register_mux_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
				  &mt6755_clk_lock, clk_data);
	#endif
#endif

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
}
CLK_OF_DECLARE(mtk_topckgen, "mediatek,mt6755-topckgen", mt_topckgen_init);

#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)
static void __init mt_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	void __iomem *spm_base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	spm_base = get_reg(node, 1);
	if ((!base) || (!spm_base)) {
		pr_err("ioremap apmixedsys/spm failed\n");
		return;
	}

	clk_data = alloc_clk_data(APMIXED_NR_CLK);

	init_clk_apmixedsys(base, spm_base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_err("could not register clock provide\n");
	scp_base = spm_base;
#if CG_BOOTUP_PDN
	apmixed_base = base;
	/*msdcpll*/
		clk_clrl(MSDCPLL_CON0, 0x1);
		clk_setl(MSDCPLL_PWR_CON0, PLL_ISO_EN);
		clk_clrl(MSDCPLL_PWR_CON0, PLL_PWR_ON);

/*tvdpll*/
		clk_clrl(TVDPLL_CON0, 0x1);
		clk_setl(TVDPLL_PWR_CON0, PLL_ISO_EN);
		clk_clrl(TVDPLL_PWR_CON0, PLL_PWR_ON);

/*apll1*/
		clk_clrl(APLL1_CON0, 0x1);
		clk_setl(APLL1_PWR_CON0, PLL_ISO_EN);
		clk_clrl(APLL1_PWR_CON0, PLL_PWR_ON);
/*apll2*/
		clk_clrl(APLL2_CON0, 0x1);
		clk_setl(APLL2_PWR_CON0, PLL_ISO_EN);
		clk_clrl(APLL2_PWR_CON0, PLL_PWR_ON);
/*oscpll default enable*/
	/* OSC EN = 1 */
		clk_setl(ULPOSC_CON, ULPOSC_EN);
		udelay(11);
	/* OSC RST  */
		clk_setl(ULPOSC_CON, ULPOSC_RST);
		udelay(40);
		clk_clrl(ULPOSC_CON, ULPOSC_RST);
		udelay(130);
	/* OSC CG_EN = 1 */
		clk_setl(ULPOSC_CON, ULPOSC_CG_EN);
#endif
}
CLK_OF_DECLARE(mtk_apmixedsys, "mediatek,mt6755-apmixedsys",
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
#if CG_BOOTUP_PDN
	/*INFRA CG*/
	infracfg_base = base;
	clk_writel(INFRA_PDN_SET0, INFRA0_CG);
	clk_writel(INFRA_PDN_SET1, INFRA1_CG);
	clk_writel(INFRA_PDN_SET2, INFRA2_CG);
#endif
}
CLK_OF_DECLARE(mtk_infrasys, "mediatek,mt6755-infrasys", mt_infrasys_init);
#if 0
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
CLK_OF_DECLARE(mtk_perisys, "mediatek,mt6755-perisys", mt_perisys_init);
#endif
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
#if 0/*CG_BOOTUP_PDN*/
	/*MFG*/
	mfgcfg_base = base;
	clk_writel(MFG_CG_SET, MFG_CG);
#endif
}
CLK_OF_DECLARE(mtk_mfgsys, "mediatek,mt6755-mfgsys", mt_mfgsys_init);

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
#if CG_BOOTUP_PDN
	/*ISP*/
	img_base = base;
	clk_writel(IMG_CG_SET, IMG_CG);
#endif
}
CLK_OF_DECLARE(mtk_imgsys, "mediatek,mt6755-imgsys", mt_imgsys_init);

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
	/*DISP*/
	mmsys_config_base = base;
	clk_writel(DISP_CG_SET0, DISP0_CG); /*DCM enable*/
#endif
}
CLK_OF_DECLARE(mtk_mmsys, "mediatek,mt6755-mmsys", mt_mmsys_init);

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
#if CG_BOOTUP_PDN
	/*VDE*/
	vdec_gcon_base = base;
	clk_writel(VDEC_CKEN_CLR, VDEC_CG);
	clk_writel(LARB_CKEN_CLR, LARB_CG);
#endif
}
CLK_OF_DECLARE(mtk_vdecsys, "mediatek,mt6755-vdecsys", mt_vdecsys_init);

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
#if CG_BOOTUP_PDN
	/*VENC*/
	venc_gcon_base = base;
	clk_clrl(VENC_CG_CON, VENC_CG);
#endif
}
CLK_OF_DECLARE(mtk_vencsys, "mediatek,mt6755-vencsys", mt_vencsys_init);

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
#if CG_BOOTUP_PDN
	/*AUDIO*/
	audio_base = base;
	clk_writel(AUDIO_TOP_CON0, AUD_CG);
#endif
}
CLK_OF_DECLARE(mtk_audiosys, "mediatek,mt6755-audiosys", mt_audiosys_init);

void pll_if_on(void)
{
		u32 segment = get_devinfo_with_index(21) & 0xFF;

		if ((segment == 0x43) || (segment == 0x4B)) {
			pr_err("segment = 6738 chip\n");
			switch_armpll_l_hwmode(1);
			switch_armpll_ll_hwmode(1);
		} else
			pr_err("segment != 6738 chip\n");

		if (clk_readl(UNIVPLL_CON0) & 0x1)
			pr_err("suspend warning: UNIVPLL is on!!!\n");

		if (clk_readl(MMPLL_CON0) & 0x1)
			pr_err("suspend warning: MMPLL is on!!!\n");

		if (clk_readl(MSDCPLL_CON0) & 0x1)
			pr_err("suspend warning: MSDCPLL is on!!!\n");

		if (clk_readl(VENCPLL_CON0) & 0x1)
			pr_err("suspend warning: VENCPLL is on!!!\n");

		if (clk_readl(TVDPLL_CON0) & 0x1)
			pr_err("suspend warning: TVDPLL is on!!!\n");

		if (clk_readl(APLL1_CON0) & 0x1)
			pr_err("suspend warning: APLL1 is on!!!\n");

		if (clk_readl(APLL2_CON0) & 0x1)
			pr_err("suspend warning: APLL2 is on!!!\n");
}

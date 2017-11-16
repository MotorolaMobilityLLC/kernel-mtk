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

#ifndef __MT_CLKMGR_H__
#define __MT_CLKMGR_H__

#include <linux/list.h>
#include <linux/types.h>
#include <linux/io.h>
#include <mach/mt_spm_mtcmos.h>

#define CONFIG_CLKMGR_STAT

#ifdef CONFIG_OF
extern void __iomem		*clk_apmixed_base;
extern void __iomem		*clk_topcksys_base;
extern void __iomem		*clk_infracfg_ao_base;
extern void __iomem		*clk_audio_base;
extern void __iomem		*clk_mfgcfg_base;
extern void __iomem		*clk_mmsys_config_base;
extern void __iomem		*clk_imgsys_base;

#undef CLK_APMIXED_BASE
#undef CLK_TOPCKSYS_BASE
#undef CLK_INFRACFG_AO_BASE
#undef CLK_AUDIO_BASE
#undef CLK_MFGCFG_BASE
#undef CLK_MMSYS_CONFIG_BASE
#undef CLK_IMGSYS_BASE

#define CLK_APMIXED_BASE	clk_apmixed_base
#define CLK_TOPCKSYS_BASE	clk_topcksys_base
#define CLK_INFRACFG_AO_BASE	clk_infracfg_ao_base
#define CLK_AUDIO_BASE		clk_audio_base
#define CLK_MFGCFG_BASE		clk_mfgcfg_base
#define CLK_MMSYS_CONFIG_BASE	clk_mmsys_config_base
#define CLK_IMGSYS_BASE		clk_imgsys_base

#else

#include <mach/mt_reg_base.h>
#define CLK_APMIXED_BASE	APMIXED_BASE
#define CLK_TOPCKSYS_BASE	CKSYS_BASE
#define CLK_INFRACFG_AO_BASE	INFRACFG_AO_BASE
#define CLK_AUDIO_BASE		AUDIO_BASE
#define CLK_MFGCFG_BASE		MFGCFG_BASE
#define CLK_MMSYS_CONFIG_BASE	MMSYS_CONFIG_BASE
#define CLK_IMGSYS_BASE		IMGSYS_BASE

#endif

/***********************/
/* APMIXEDSYS Register */
/***********************/
#define AP_PLL_CON0		(CLK_APMIXED_BASE + 0x0000)
#define AP_PLL_CON1		(CLK_APMIXED_BASE + 0x0004)
#define AP_PLL_CON2		(CLK_APMIXED_BASE + 0x0008)
#define AP_PLL_CON3		(CLK_APMIXED_BASE + 0x000C)
#define AP_PLL_CON4		(CLK_APMIXED_BASE + 0x0010)
#define PLL_HP_CON0		(CLK_APMIXED_BASE + 0x0014)
#define PLL_HP_CON1		(CLK_APMIXED_BASE + 0x0018)
#define CLKSQ_STB_CON0		(CLK_APMIXED_BASE + 0x001C)
#define PLL_PWR_CON0		(CLK_APMIXED_BASE + 0x0020)
#define PLL_PWR_CON1		(CLK_APMIXED_BASE + 0x0024)
#define PLL_PWR_CON2		(CLK_APMIXED_BASE + 0x0028)
#define PLL_PWR_CON3		(CLK_APMIXED_BASE + 0x002C)
#define PLL_ISO_CON0		(CLK_APMIXED_BASE + 0x0030)
#define PLL_ISO_CON1		(CLK_APMIXED_BASE + 0x0034)
#define PLL_ISO_CON2		(CLK_APMIXED_BASE + 0x0038)
#define PLL_ISO_CON3		(CLK_APMIXED_BASE + 0x003C)
#define PLL_EN_CON0		(CLK_APMIXED_BASE + 0x0040)
#define PLL_EN_CON1		(CLK_APMIXED_BASE + 0x0044)
#define PLL_STB_CON0		(CLK_APMIXED_BASE + 0x0048)
#define PLL_STB_CON1		(CLK_APMIXED_BASE + 0x004C)
#define PLL_STB_CON2		(CLK_APMIXED_BASE + 0x0050)
#define DIV_STB_CON0		(CLK_APMIXED_BASE + 0x0054)
#define PLL_CHG_CON0		(CLK_APMIXED_BASE + 0x0058)
#define PLL_CHG_CON1		(CLK_APMIXED_BASE + 0x005C)
#define PLL_TEST_CON0		(CLK_APMIXED_BASE + 0x0060)
#define PLL_TEST_CON1		(CLK_APMIXED_BASE + 0x0064)
#define PLL_INT_CON0		(CLK_APMIXED_BASE + 0x0068)
#define PLL_INT_CON1		(CLK_APMIXED_BASE + 0x006C)
#define ARMPLL_CON0		(CLK_APMIXED_BASE + 0x0100)
#define ARMPLL_CON1		(CLK_APMIXED_BASE + 0x0104)
#define ARMPLL_CON2		(CLK_APMIXED_BASE + 0x0108)
#define ARMPLL_CON3		(CLK_APMIXED_BASE + 0x010C)
#define ARMPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0110)
#define MAINPLL_CON0		(CLK_APMIXED_BASE + 0x0120)
#define MAINPLL_CON1		(CLK_APMIXED_BASE + 0x0124)
#define MAINPLL_CON2		(CLK_APMIXED_BASE + 0x0128)
#define MAINPLL_CON3		(CLK_APMIXED_BASE + 0x012C)
#define MAINPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0130)
#define UNIVPLL_CON0		(CLK_APMIXED_BASE + 0x0140)
#define UNIVPLL_CON1		(CLK_APMIXED_BASE + 0x0144)
#define UNIVPLL_CON2		(CLK_APMIXED_BASE + 0x0148)
#define UNIVPLL_CON3		(CLK_APMIXED_BASE + 0x014C)
#define UNIVPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0150)
#define MCUPLL_CON0		(CLK_APMIXED_BASE + 0x0220)
#define MCUPLL_CON1		(CLK_APMIXED_BASE + 0x0224)
#define MCUPLL_CON2		(CLK_APMIXED_BASE + 0x0228)
#define MCUPLL_CON3		(CLK_APMIXED_BASE + 0x022C)
#define MCUPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0230)
#define MCUPLL_PATHSEL_CON	(CLK_APMIXED_BASE + 0x0234)
#define WHPLL_CON0		(CLK_APMIXED_BASE + 0x0240)
#define WHPLL_CON1		(CLK_APMIXED_BASE + 0x0244)
#define WHPLL_CON2		(CLK_APMIXED_BASE + 0x0248)
#define WHPLL_CON3		(CLK_APMIXED_BASE + 0x024C)
#define WHPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0250)
#define WHPLL_PATHSEL_CON	(CLK_APMIXED_BASE + 0x0254)
#define WPLL_CON0		(CLK_APMIXED_BASE + 0x0260)
#define WPLL_CON1		(CLK_APMIXED_BASE + 0x0264)
#define WPLL_CON2		(CLK_APMIXED_BASE + 0x0268)
#define WPLL_CON3		(CLK_APMIXED_BASE + 0x026C)
#define WPLL_PWR_CON0	(CLK_APMIXED_BASE + 0x0270)
#define WPLL_PATHSEL_CON	(CLK_APMIXED_BASE + 0x0274)
#define AP_AUXADC_CON0	(CLK_APMIXED_BASE + 0x0400)
#define AP_AUXADC_CON1		(CLK_APMIXED_BASE + 0x0404)
#define AP_AUXADC_CON2		(CLK_APMIXED_BASE + 0x0408)
#define TS_CON0			(CLK_APMIXED_BASE + 0x0600)
#define TS_CON1			(CLK_APMIXED_BASE + 0x0604)
#define AP_ABIST_MON_CON0	(CLK_APMIXED_BASE + 0x0E00)
#define AP_ABIST_MON_CON1	(CLK_APMIXED_BASE + 0x0E04)
#define AP_ABIST_MON_CON2	(CLK_APMIXED_BASE + 0x0E08)
#define AP_ABIST_MON_CON3	(CLK_APMIXED_BASE + 0x0E0C)
#define OCCSCAN_CON0		(CLK_APMIXED_BASE + 0x0E1C)
#define CLKDIV_CON0		(CLK_APMIXED_BASE + 0x0E20)
#define RSV_RW0_CON0		(CLK_APMIXED_BASE + 0x0F00)
#define RSV_RW0_CON1		(CLK_APMIXED_BASE + 0x0F04)
#define RSV_RW1_CON0		(CLK_APMIXED_BASE + 0x0F08)
#define RSV_RW1_CON1		(CLK_APMIXED_BASE + 0x0F0C)
#define RSV_RO_CON0		(CLK_APMIXED_BASE + 0x0F10)
#define RSV_RO_CON1		(CLK_APMIXED_BASE + 0x0F14)
#define RSV_ATPG_CON0		(CLK_APMIXED_BASE + 0x0F18)
/***********************/
/* TOPCKGEN Register   */
/***********************/
#define CLK_MUX_SEL0		(CLK_TOPCKSYS_BASE + 0x000)
#define CLK_MUX_SEL1		(CLK_TOPCKSYS_BASE + 0x004)
#define TOPBUS_DCMCTL	(CLK_TOPCKSYS_BASE + 0x008)
#define TOPEMI_DCMCTL	(CLK_TOPCKSYS_BASE + 0x00C)
#define FREQ_MTR_CTRL	(CLK_TOPCKSYS_BASE + 0x010)
#define CLK_GATING_CTRL0	(CLK_TOPCKSYS_BASE + 0x020)
#define CLK_GATING_CTRL1	(CLK_TOPCKSYS_BASE + 0x024)
#define INFRABUS_DCMCTL0	(CLK_TOPCKSYS_BASE + 0x028)
#define INFRABUS_DCMCTL1	(CLK_TOPCKSYS_BASE + 0x02C)
#define MPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x030)
#define UPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x034)
#define TEST_DBG_CTRL		(CLK_TOPCKSYS_BASE + 0x038)
#define CLK_GATING_CTRL2	(CLK_TOPCKSYS_BASE + 0x03C)
#define SET_CLK_GATING_CTRL0	(CLK_TOPCKSYS_BASE + 0x050)
#define SET_CLK_GATING_CTRL1	(CLK_TOPCKSYS_BASE + 0x054)
#define SET_INFRABUS_DCMCTL0	(CLK_TOPCKSYS_BASE + 0x058)
#define SET_INFRABUS_DCMCTL1	(CLK_TOPCKSYS_BASE + 0x05C)
#define SET_MPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x060)
#define SET_UPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x064)
#define SET_TEST_DBG_CTRL	(CLK_TOPCKSYS_BASE + 0x068)
#define SET_CLK_GATING_CTRL2	(CLK_TOPCKSYS_BASE + 0x06C)
#define CLR_CLK_GATING_CTRL0	(CLK_TOPCKSYS_BASE + 0x080)
#define CLR_CLK_GATING_CTRL1	(CLK_TOPCKSYS_BASE + 0x084)
#define CLR_INFRABUS_DCMCTL0	(CLK_TOPCKSYS_BASE + 0x088)
#define CLR_INFRABUS_DCMCTL1	(CLK_TOPCKSYS_BASE + 0x08C)
#define CLR_MPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x090)
#define CLR_UPLL_FREDIV_EN	(CLK_TOPCKSYS_BASE + 0x094)
#define CLR_TEST_DBG_CTRL	(CLK_TOPCKSYS_BASE + 0x098)
#define CLR_CLK_GATING_CTRL2	(CLK_TOPCKSYS_BASE + 0x09C)
#define LPM_CTRL		(CLK_TOPCKSYS_BASE + 0x100)
#define LPM_TOTAL_TIME	(CLK_TOPCKSYS_BASE + 0x104)
#define LPM_LOW2HIGH_COUNT	(CLK_TOPCKSYS_BASE + 0x108)
#define LPM_HIGH_DUR_TIME	(CLK_TOPCKSYS_BASE + 0x10C)
#define LPM_LONGEST_HIGHTIME	(CLK_TOPCKSYS_BASE + 0x110)
#define LPM_GOODDUR_COUNT	(CLK_TOPCKSYS_BASE + 0x114)
#define CLK_GATING_CTRL0_SEN	(CLK_TOPCKSYS_BASE + 0x220)
#define CLK_GATING_CTRL1_SEN	(CLK_TOPCKSYS_BASE + 0x224)
#define CLK_GATING_CTRL2_SEN	(CLK_TOPCKSYS_BASE + 0x23C)
/***********************/
/* INFRASYS Register   */
/***********************/
#define TOP_CKMUXSEL		(CLK_INFRACFG_AO_BASE + 0x000)
#define TOP_CKDIV1		(CLK_INFRACFG_AO_BASE + 0x008)
#define INFRA_PDN_SET		(CLK_INFRACFG_AO_BASE + 0x030)
#define INFRA_PDN_CLR		(CLK_INFRACFG_AO_BASE + 0x034)
#define INFRA_PDN_STA		(CLK_INFRACFG_AO_BASE + 0x038)
#define TOPAXI_PROT_EN		(CLK_INFRACFG_AO_BASE + 0x220)
#define TOPAXI_PROT_STA0	(CLK_INFRACFG_AO_BASE + 0x224)
#define TOPAXI_PROT_STA1	(CLK_INFRACFG_AO_BASE + 0x228)
#define INFRA_RSVD1		(CLK_INFRACFG_AO_BASE + 0xA00)
/***********************/
/* Audio Register      */
/***********************/
#define AUDIO_TOP_CON0		(CLK_AUDIO_BASE + 0x0000)
/***********************/
/* MFGCFG Register     */
/***********************/
#define MFG_CG_STA		(CLK_MFGCFG_BASE + 0x0)
#define MFG_CG_SET		(CLK_MFGCFG_BASE + 0x4)
#define MFG_CG_CLR		(CLK_MFGCFG_BASE + 0x8)
/***********************/
/* MMSYS Register      */
/***********************/
#define MMSYS_CG_CON0		(CLK_MMSYS_CONFIG_BASE + 0x100)
#define MMSYS_CG_SET0		(CLK_MMSYS_CONFIG_BASE + 0x104)
#define MMSYS_CG_CLR0		(CLK_MMSYS_CONFIG_BASE + 0x108)
#define MMSYS_CG_CON1		(CLK_MMSYS_CONFIG_BASE + 0x110)
#define MMSYS_CG_SET1		(CLK_MMSYS_CONFIG_BASE + 0x114)
#define MMSYS_CG_CLR1		(CLK_MMSYS_CONFIG_BASE + 0x118)
/***********************/
/* IMGSYS Register     */
/***********************/
#define IMG_CG_CON		(CLK_IMGSYS_BASE + 0x0000)
#define IMG_CG_SET		(CLK_IMGSYS_BASE + 0x0004)
#define IMG_CG_CLR		(CLK_IMGSYS_BASE + 0x0008)
/*  */
/* CG_CLK Control BIT */
/*  */
/***********************/
/* CG_MIXED            */
/***********************/
#define USB48M_EN_BIT		BIT(28)
#define UNIV48M_EN_BIT		BIT(29)

#define CG_MIXED_MASK           (USB48M_EN_BIT \
			      | UNIV48M_EN_BIT)
/***********************/
/* CG_MPLL             */
/***********************/
#define MPLL_D2_EN_BIT		BIT(0)
#define MPLL_D3_EN_BIT		BIT(1)
#define MPLL_D5_EN_BIT		BIT(2)
#define MPLL_D7_EN_BIT		BIT(3)
#define MPLL_D11_EN_BIT		BIT(4)
#define MPLL_D4_EN_BIT		BIT(8)
#define MPLL_D6_EN_BIT		BIT(9)
#define MPLL_D10_EN_BIT		BIT(10)
#define MPLL_D14_EN_BIT		BIT(11)
#define MPLL_D8_EN_BIT		BIT(16)
#define MPLL_D12_EN_BIT		BIT(17)
#define MPLL_D20_EN_BIT		BIT(18)
#define MPLL_D28_EN_BIT		BIT(19)
#define MPLL_D16_EN_BIT		BIT(23)
#define MPLL_D24_EN_BIT		BIT(24)
#define MPLL_D40_EN_BIT		BIT(25)
#define MEMPLL_D2_EN_BIT	BIT(31)
#define CG_MPLL_MASK           (MPLL_D2_EN_BIT  \
			      | MPLL_D3_EN_BIT  \
			      | MPLL_D5_EN_BIT  \
			      | MPLL_D7_EN_BIT  \
			      | MPLL_D11_EN_BIT	\
			      | MPLL_D4_EN_BIT  \
			      | MPLL_D6_EN_BIT  \
			      | MPLL_D10_EN_BIT \
			      | MPLL_D14_EN_BIT	\
			      | MPLL_D8_EN_BIT  \
			      | MPLL_D12_EN_BIT \
			      | MPLL_D20_EN_BIT \
			      | MPLL_D28_EN_BIT	\
			      | MPLL_D16_EN_BIT	\
			      | MPLL_D24_EN_BIT	\
			      | MPLL_D40_EN_BIT	\
			      | MEMPLL_D2_EN_BIT)
/***********************/
/* CG_UPLL             */
/***********************/
#define UPLL_D2_EN_BIT		BIT(0)
#define UPLL_D3_EN_BIT		BIT(1)
#define UPLL_D5_EN_BIT		BIT(2)
#define UPLL_D7_EN_BIT		BIT(3)
#define UPLL_D4_EN_BIT		BIT(8)
#define UPLL_D6_EN_BIT		BIT(9)
#define UPLL_D10_EN_BIT		BIT(10)
#define UPLL_D8_EN_BIT		BIT(16)
#define UPLL_D12_EN_BIT		BIT(17)
#define UPLL_D20_EN_BIT		BIT(18)
#define UPLL_D16_EN_BIT		BIT(24)
#define UPLL_D24_EN_BIT		BIT(25)
#define CG_UPLL_MASK           (UPLL_D2_EN_BIT  \
			      | UPLL_D3_EN_BIT  \
			      | UPLL_D5_EN_BIT  \
			      | UPLL_D7_EN_BIT  \
			      | UPLL_D4_EN_BIT  \
			      | UPLL_D6_EN_BIT  \
			      | UPLL_D10_EN_BIT \
			      | UPLL_D8_EN_BIT  \
			      | UPLL_D12_EN_BIT \
			      | UPLL_D20_EN_BIT \
			      | UPLL_D16_EN_BIT \
			      | UPLL_D24_EN_BIT)
/***********************/
/* CG_CTRL0            */
/***********************/
#define PWM_MM_SW_CG_BIT		BIT(0)
#define CAM_MM_SW_CG_BIT		BIT(1)
#define MFG_MM_SW_CG_BIT		BIT(2)
#define SPM_52M_SW_CG_BIT		BIT(3)
#define MIPI_26M_DBG_EN_BIT		BIT(4)
#define SCAM_MM_SW_CG_BIT		BIT(5)
#define SC_26M_CK_SEL_EN_BIT		BIT(6)
#define SC_MEM_CK_OFF_EN_BIT		BIT(7)
#define SC_AXI_CK_OFF_EN_BIT		BIT(8)
#define SMI_MM_SW_BIT			BIT(9)
#define ARM_EMICLK_WFI_GATING_DIS_BIT	BIT(30)
#define ARMDCM_CLKOFF_EN_BIT		BIT(31)

#define CG_CTRL0_MASK          (PWM_MM_SW_CG_BIT        \
			      | CAM_MM_SW_CG_BIT        \
			      | MFG_MM_SW_CG_BIT        \
			      | SPM_52M_SW_CG_BIT       \
			      | SCAM_MM_SW_CG_BIT	\
			      | SMI_MM_SW_BIT	        \
			      | ARM_EMICLK_WFI_GATING_DIS_BIT)
/* enable bit @ CG_CTRL0 */
#define CG_CTRL0_EN_MASK       (MIPI_26M_DBG_EN_BIT	\
			      | SC_26M_CK_SEL_EN_BIT    \
			      | SC_MEM_CK_OFF_EN_BIT    \
			      | SC_AXI_CK_OFF_EN_BIT	\
			      | ARMDCM_CLKOFF_EN_BIT)
/***********************/
/* CG_CTRL1            */
/***********************/
#define THEM_SW_CG_BIT			BIT(1)
#define APDMA_SW_CG_BIT			BIT(2)
#define I2C0_SW_CG_BIT			BIT(3)
#define I2C1_SW_CG_BIT			BIT(4)
#define AUXADC1_SW_CG_BIT		BIT(5)
#define NFI_SW_CG_BIT			BIT(6)
#define NFIECC_SW_CG_BIT		BIT(7)
#define DEBUGSYS_SW_CG_BIT		BIT(8)
#define PWM_SW_CG_BIT			BIT(9)
#define UART0_SW_CG_BIT			BIT(10)
#define UART1_SW_CG_BIT			BIT(11)
#define BTIF_SW_CG_BIT			BIT(12)
#define USB_SW_CG_BIT			BIT(13)
#define FHCTL_SW_CG_BIT			BIT(14)
#define AUXADC2_SW_CG_BIT		BIT(15)
#define I2C2_SW_CG_BIT			BIT(16)
#define MSDC0_SW_CG_BIT			BIT(17)
#define MSDC1_SW_CG_BIT			BIT(18)
#define NFI2X_SW_CG_BIT			BIT(19)
#define PMIC_SW_CG_AP_BIT		BIT(20)
#define SEJ_SW_CG_BIT			BIT(21)
#define MEMSLP_DLYER_SW_CG_BIT		BIT(22)
#define SPI_SW_CG_BIT			BIT(23)
#define APXGPT_SW_CG_BIT		BIT(24)
#define AUDIO_SW_CG_BIT			BIT(25)
#define SPM_SW_CG_BIT			BIT(26)
#define PMIC_SW_CG_MD_BIT		BIT(27)
#define PMIC_SW_CG_CONN_BIT		BIT(28)
#define PMIC_26M_SW_CG_BIT		BIT(29)
#define AUX_SW_CG_ADC_BIT		BIT(30)
#define AUX_SW_CG_TP_BIT		BIT(31)

#define CG_CTRL1_MASK         (THEM_SW_CG_BIT			\
			      | APDMA_SW_CG_BIT         \
			      | I2C0_SW_CG_BIT          \
			      | I2C1_SW_CG_BIT          \
			      |	AUXADC1_SW_CG_BIT		\
			      | NFI_SW_CG_BIT           \
			      | NFIECC_SW_CG_BIT        \
			      | DEBUGSYS_SW_CG_BIT      \
			      | PWM_SW_CG_BIT           \
			      | UART0_SW_CG_BIT         \
			      | UART1_SW_CG_BIT         \
			      | BTIF_SW_CG_BIT          \
			      | USB_SW_CG_BIT           \
			      | FHCTL_SW_CG_BIT         \
			      | AUXADC2_SW_CG_BIT		\
			      | I2C2_SW_CG_BIT			\
			      | MSDC0_SW_CG_BIT         \
			      | MSDC1_SW_CG_BIT         \
			      | NFI2X_SW_CG_BIT			\
			      | PMIC_SW_CG_AP_BIT       \
			      | SEJ_SW_CG_BIT           \
			      | MEMSLP_DLYER_SW_CG_BIT  \
			      | SPI_SW_CG_BIT			\
			      | APXGPT_SW_CG_BIT        \
			      | AUDIO_SW_CG_BIT         \
			      | SPM_SW_CG_BIT           \
			      | PMIC_SW_CG_MD_BIT		\
			      | PMIC_SW_CG_CONN_BIT		\
			      | PMIC_26M_SW_CG_BIT      \
			      | AUX_SW_CG_ADC_BIT		\
			      | AUX_SW_CG_TP_BIT)



/***********************/
/* CG_CTRL2            */
/***********************/
#define RBIST_SW_CG_BIT		BIT(1)
#define NFI_BUS_SW_CG_BIT	BIT(2)
#define GCE_SW_CG_BIT		BIT(4)
#define TRNG_SW_CG_BIT		BIT(5)
#define SEJ_13M_SW_CG_BIT	BIT(6)
#define AES_SW_CG_BIT		BIT(7)
#define PWM_BCLK_SW_CG_BIT	BIT(8)
#define PWM1_FBCLK_SW_CG_BIT	BIT(9)
#define PWM2_FBCLK_SW_CG_BIT	BIT(10)
#define PWM3_FBCLK_SW_CG_BIT	BIT(11)
#define PWM4_FBCLK_SW_CG_BIT	BIT(12)
#define PWM5_FBCLK_SW_CG_BIT	BIT(13)

#define CG_CTRL2_MASK		(RBIST_SW_CG_BIT		\
				| NFI_BUS_SW_CG_BIT     \
				| GCE_SW_CG_BIT         \
				| TRNG_SW_CG_BIT        \
				| SEJ_13M_SW_CG_BIT     \
				| AES_SW_CG_BIT         \
				| PWM_BCLK_SW_CG_BIT    \
				| PWM1_FBCLK_SW_CG_BIT  \
				| PWM2_FBCLK_SW_CG_BIT  \
				| PWM3_FBCLK_SW_CG_BIT  \
				| PWM4_FBCLK_SW_CG_BIT  \
				| PWM5_FBCLK_SW_CG_BIT)

/***********************/
/* CG_MMSYS_CON0       */
/***********************/
#define SMI_COMMON_BIT	BIT(0)
#define SMI_LARB0_BIT	BIT(1)
#define CAM_MDP_BIT	BIT(2)
#define MDP_RDMA_BIT	BIT(3)
#define MDP_RSZ0_BIT	BIT(4)
#define MDP_RSZ1_BIT	BIT(5)
#define MDP_TDSHP_BIT	BIT(6)
#define MDP_WDMA_BIT	BIT(7)
#define MDP_WROT_BIT	BIT(8)
#define FAKE_ENG_BIT	BIT(9)
#define DISP_OVL0_BIT	BIT(10)
#define DISP_RDMA0_BIT	BIT(11)
#define DISP_WDMA0_BIT	BIT(13)
#define DISP_COLOR_BIT	BIT(14)
#define DISP_AAL_BIT	BIT(16)
#define DISP_GAMMA_BIT	BIT(17)
#define DISP_DITHER_BIT BIT(18)
#define CG_MMSYS0_MASK		(SMI_COMMON_BIT		\
				| SMI_LARB0_BIT		\
				| CAM_MDP_BIT		\
				| MDP_RDMA_BIT		\
				| MDP_RSZ0_BIT		\
				| MDP_RSZ1_BIT		\
				| MDP_TDSHP_BIT		\
				| MDP_WDMA_BIT		\
				| MDP_WROT_BIT		\
				| FAKE_ENG_BIT		\
				| DISP_OVL0_BIT		\
				| DISP_RDMA0_BIT	\
				| DISP_WDMA0_BIT	\
				| DISP_COLOR_BIT	\
				| DISP_AAL_BIT		\
				| DISP_GAMMA_BIT	\
				| DISP_DITHER_BIT)

/***********************/
/* CG_MMSYS_CON1       */
/***********************/
#define DSI_ENGINE_BIT		BIT(2)
#define DSI_DIGITAL_BIT		BIT(3)

#define CG_MMSYS1_MASK         (DSI_ENGINE_BIT		\
			      | DSI_DIGITAL_BIT)

/***********************/
/* CG_IMGSYS_CON       */
/***********************/
#define LARB1_SMI_CKPDN_BIT	BIT(0)
#define CAM_SMI_CKPDN_BIT	BIT(5)
#define CAM_CAM_CKPDN_BIT	BIT(6)
#define SEN_TG_CKPDN_BIT	BIT(7)
#define SEN_CAM_CKPDN_BIT	BIT(8)
#define VCODEC_CKPDN_BIT	BIT(9)

#define CG_IMGSYS_MASK         (LARB1_SMI_CKPDN_BIT	\
			      | CAM_SMI_CKPDN_BIT	\
			      | CAM_CAM_CKPDN_BIT	\
			      | SEN_TG_CKPDN_BIT	\
			      | SEN_CAM_CKPDN_BIT	\
			      | VCODEC_CKPDN_BIT)

/***********************/
/* CG_MFGSYS_CON       */
/***********************/
#define BG3D_BIT		BIT(0)
#define CG_MFGSYS_MASK		BG3D_BIT


/***********************/
/* CG_AUDIO_TOP_CON */
/***********************/
#define AUD_PDN_AFE_EN_BIT	BIT(2)
#define AUD_PDN_I2S_EN_BIT	BIT(6)
#define AUD_PDN_ADC_EN_BIT	BIT(24)
#define AUD_PDN_DAC_EN_BIT	BIT(25)

#define CG_AUDIO_MASK          (AUD_PDN_AFE_EN_BIT          \
			      | AUD_PDN_I2S_EN_BIT          \
			      | AUD_PDN_ADC_EN_BIT          \
			      | AUD_PDN_DAC_EN_BIT)

/***********************/
/* CG_INFRA_AO */
/***********************/
#define PLL1_CK_BIT	BIT(0)
#define PLL2_CK_BIT	BIT(1)

#define ISP_PWR_STA_MASK    (0x1 << 5)
#define MFG_PWR_STA_MASK    (0x1 << 4)
#define DIS_PWR_STA_MASK    (0x1 << 3)
#define CONN_PWR_STA_MASK   (0x1 << 1)
#define MD1_PWR_STA_MASK    (0x1 << 0)

/* CG_GPR ID */
enum cg_grp_id {
	CG_MIXED	= 0,
	CG_MPLL		= 1,
	CG_UPLL		= 2,
	CG_CTRL0	= 3,
	CG_CTRL1	= 4,
	CG_CTRL2	= 5,
	CG_MMSYS0	= 6,
	CG_MMSYS1	= 7,
	CG_IMGSYS	= 8,
	CG_MFGSYS	= 9,
	CG_AUDIO	= 10,
	CG_INFRA_AO	= 11,
	NR_GRPS,
};

/* CG_CLK ID */
enum cg_clk_id {
/* CG_MIXED */
	MT_CG_SYS_26M,
	MT_CG_SYS_TEMP,
	MT_CG_MEMPLL,
	MT_CG_UNIV_48M,
	MT_CG_USB_48M,

/* CG_MPLL */
	MT_CG_MPLL_D2,
	MT_CG_MPLL_D3,
	MT_CG_MPLL_D5,
	MT_CG_MPLL_D7,
	MT_CG_MPLL_D11,
	MT_CG_MPLL_D4,
	MT_CG_MPLL_D6,
	MT_CG_MPLL_D10,
	MT_CG_MPLL_D14,
	MT_CG_MPLL_D8,
	MT_CG_MPLL_D12,
	MT_CG_MPLL_D20,
	MT_CG_MPLL_D28,
	MT_CG_MPLL_D16,
	MT_CG_MPLL_D24,
	MT_CG_MPLL_D40,
/* CG_UPLL */
	MT_CG_UPLL_D2,
	MT_CG_UPLL_D3,
	MT_CG_UPLL_D5,
	MT_CG_UPLL_D7,
	MT_CG_UPLL_D4,
	MT_CG_UPLL_D6,
	MT_CG_UPLL_D10,
	MT_CG_UPLL_D8,
	MT_CG_UPLL_D12,
	MT_CG_UPLL_D20,
	MT_CG_UPLL_D16,
	MT_CG_UPLL_D24,
/* CG_INFRA_AO */
	MT_CG_PLL2_CK,

/* CG_CTRL0 */
	MT_CG_PWM_MM_SW_CG,
	MT_CG_CAM_MM_SW_CG,
	MT_CG_MFG_MM_SW_CG,
	MT_CG_SPM_52M_SW_CG,
	MT_CG_MIPI_26M_DBG_EN,
	MT_CG_SCAM_MM_SW_CG,
	MT_CG_SC_26M_CK_SEL_EN,
	MT_CG_SC_MEM_CK_OFF_EN,
	MT_CG_SC_AXI_CK_OFF_EN,
	MT_CG_SMI_MM_SW_CG,
	MT_CG_ARM_EMICLK_WFI_GATING_DIS,
	MT_CG_ARMDCM_CLKOFF_EN,

/* CG_CTRL1 */
	MT_CG_THEM_SW_CG,
	MT_CG_APDMA_SW_CG,
	MT_CG_I2C0_SW_CG,
	MT_CG_I2C1_SW_CG,
	MT_CG_AUXADC1_SW_CG,
	MT_CG_NFI_SW_CG,
	MT_CG_NFIECC_SW_CG,
	MT_CG_DEBUGSYS_SW_CG,
	MT_CG_PWM_SW_CG,
	MT_CG_UART0_SW_CG,
	MT_CG_UART1_SW_CG,
	MT_CG_BTIF_SW_CG,
	MT_CG_USB_SW_CG,
	MT_CG_FHCTL_SW_CG,
	MT_CG_AUXADC2_SW_CG,
	MT_CG_I2C2_SW_CG,
	MT_CG_MSDC0_SW_CG,
	MT_CG_MSDC1_SW_CG,
	MT_CG_NFI2X_SW_CG,
	MT_CG_PMIC_SW_CG_AP,
	MT_CG_SEJ_SW_CG,
	MT_CG_MEMSLP_DLYER_SW_CG,
	MT_CG_SPI_SW_CG,
	MT_CG_APXGPT_SW_CG,
	MT_CG_AUDIO_SW_CG,
	MT_CG_SPM_SW_CG,
	MT_CG_PMIC_SW_CG_MD,
	MT_CG_PMIC_SW_CG_CONN,
	MT_CG_PMIC_26M_SW_CG,
	MT_CG_AUX_SW_CG_ADC,
	MT_CG_AUX_SW_CG_TP,

/* CG_CTRL2 */
	MT_CG_RBIST_SW_CG,
	MT_CG_NFI_BUS_SW_CG,
	MT_CG_GCE_SW_CG,
	MT_CG_TRNG_SW_CG,
	MT_CG_SEJ_13M_SW_CG,
	MT_CG_AES_SW_CG,
	MT_CG_PWM_BCLK_SW_CG,
	MT_CG_PWM1_FBCLK_SW_CG,
	MT_CG_PWM2_FBCLK_SW_CG,
	MT_CG_PWM3_FBCLK_SW_CG,
	MT_CG_PWM4_FBCLK_SW_CG,
	MT_CG_PWM5_FBCLK_SW_CG,

/* CG_MMSYS0 */
	MT_CG_DISP0_SMI_COMMON,
	MT_CG_DISP0_SMI_LARB0,
	MT_CG_DISP0_CAM_MDP,		/* DCM */
	MT_CG_DISP0_MDP_RDMA,		/* DCM */
	MT_CG_DISP0_MDP_RSZ0,		/* DCM */
	MT_CG_DISP0_MDP_RSZ1,		/* DCM */
	MT_CG_DISP0_MDP_TDSHP,		/* DCM */
	MT_CG_DISP0_MDP_WDMA,		/* DCM */
	MT_CG_DISP0_MDP_WROT,		/* DCM */
	MT_CG_DISP0_FAKE_ENG,
	MT_CG_DISP0_DISP_OVL0,		/* DCM */
	MT_CG_DISP0_DISP_RDMA0,		/* DCM */
	MT_CG_DISP0_DISP_WDMA0,		/* DCM */
	MT_CG_DISP0_DISP_COLOR,		/* DCM */
	MT_CG_DISP0_DISP_AAL,		/* DCM */
	MT_CG_DISP0_DISP_GAMMA,		/* DCM */
	MT_CG_DISP0_DISP_DITHER,	/* DCM */

/* CG_MMSYS1 */
	MT_CG_DISP1_DSI_ENGINE,
	MT_CG_DISP1_DSI_DIGITAL,

/* CG_IMGSYS */
	MT_CG_LARB1_SMI_CKPDN,
	MT_CG_CAM_SMI_CKPDN,
	MT_CG_CAM_CAM_CKPDN,
	MT_CG_SEN_TG_CKPDN,
	MT_CG_SEN_CAM_CKPDN,
	MT_CG_VCODEC_CKPDN,

/* CG_MFGSYS */
	MT_CG_BG3D,

/* CG_AUDIO */

	MT_CG_AUD_PDN_AFE_EN,
	MT_CG_AUD_PDN_I2S_EN,
	MT_CG_AUD_PDN_ADC_EN,
	MT_CG_AUD_PDN_DAC_EN,
	NR_CLKS,
	MT_CG_INVALID,

	CG_MIXED_FROM	= MT_CG_SYS_26M,
	CG_MIXED_TO	= MT_CG_USB_48M,
	CG_MPLL_FROM	= MT_CG_MPLL_D2,
	CG_MPLL_TO	= MT_CG_MPLL_D40,
	CG_UPLL_FROM	= MT_CG_UPLL_D2,
	CG_UPLL_TO	= MT_CG_UPLL_D24,
	CG_CTRL0_FROM	= MT_CG_PWM_MM_SW_CG,
	CG_CTRL0_TO	= MT_CG_ARMDCM_CLKOFF_EN,
	CG_CTRL1_FROM	= MT_CG_THEM_SW_CG,
	CG_CTRL1_TO	= MT_CG_AUX_SW_CG_TP,
	CG_CTRL2_FROM	= MT_CG_RBIST_SW_CG,
	CG_CTRL2_TO	= MT_CG_PWM5_FBCLK_SW_CG,
	CG_MMSYS0_FROM	= MT_CG_DISP0_SMI_COMMON,
	CG_MMSYS0_TO	= MT_CG_DISP0_FAKE_ENG,
	CG_MMSYS1_FROM	= MT_CG_DISP1_DSI_ENGINE,
	CG_MMSYS1_TO	= MT_CG_DISP1_DSI_DIGITAL,
	CG_IMGSYS_FROM	= MT_CG_LARB1_SMI_CKPDN,
	CG_IMGSYS_TO	= MT_CG_VCODEC_CKPDN,
	CG_MFGSYS_FROM	= MT_CG_BG3D,
	CG_MFGSYS_TO	= MT_CG_BG3D,
	CG_AUDIO_FROM	= MT_CG_AUD_PDN_AFE_EN,
	CG_AUDIO_TO	= MT_CG_AUD_PDN_DAC_EN,
/* add for build error */
	MT_CG_INFRA_UART0		= MT_CG_UART0_SW_CG,
	MT_CG_INFRA_UART1		= MT_CG_UART1_SW_CG,
	MT_CG_INFRA_MSDC_0		= MT_CG_MSDC0_SW_CG,
	MT_CG_INFRA_MSDC_1		= MT_CG_MSDC1_SW_CG,
	MT_CG_INFRA_UART2		= MT_CG_INVALID,
	MT_CG_INFRA_UART3		= MT_CG_INVALID,
	MT_CG_INFRA_GCE			= MT_CG_GCE_SW_CG,
};


/* CLKMUX ID */
enum clkmux_id {
	MT_CLKMUX_UART0_GFMUX_SEL,
	MT_CLKMUX_EMI1X_GFMUX_SEL,
	MT_CLKMUX_AXIBUS_GFMUX_SEL,
	MT_CLKMUX_MFG_MUX_SEL,
	MT_CLKMUX_MSDC0_MUX_SEL,
	MT_CLKMUX_CAM_MUX_SEL,
	MT_CLKMUX_PWM_MM_MUX_SEL,
	MT_CLKMUX_UART1_GFMUX_SEL,
	MT_CLKMUX_MSDC1_MUX_SEL,
	MT_CLKMUX_PMICSPI_MUX_SEL,
	MT_CLKMUX_AUD_HF_26M_SEL,
	MT_CLKMUX_AUD_INTBUS_SEL,
	MT_CLKMUX_NFI2X_GFMUX_SEL,
	MT_CLKMUX_NFI1X_INFRA_SEL,
	MT_CLKMUX_MFG_GFMUX_SEL,
	MT_CLKMUX_SPI_GFMUX_SEL,
	MT_CLKMUX_DDRPHYCFG_GFMUX_SEL,
	MT_CLKMUX_SMI_GFMUX_SEL,
	MT_CLKMUX_USB_GFMUX_SEL,
	MT_CLKMUX_SCAM_MUX_SEL,
	NR_CLKMUXS,
	MT_CLKMUX_INVALID,
};


/* PLL ID */
enum pll_id {
	ARMPLL,
	MAINPLL,
	UNIVPLL,
	NR_PLLS,
};

/* SUBSYS ID */
enum subsys_id {
	SYS_MD1,	/* MDSYS */
	SYS_CON,	/* CONSYS */
	SYS_DIS,	/* MMSYS */
	SYS_MFG,	/* MFGSYS */
	SYS_IMG,	/* IMGSYS */
	NR_SYSS,
};

/* LARB ID */
enum larb_id {
	MT_LARB0,
	MT_LARB1,
	NR_LARBS,
};

/* larb monitor mechanism definition*/
enum {
	LARB_MONITOR_LEVEL_HIGH		= 10,
	LARB_MONITOR_LEVEL_MEDIUM	= 20,
	LARB_MONITOR_LEVEL_LOW		= 30,
};

struct larb_monitor {
	struct list_head link;
	int level;
	void (*backup)(struct larb_monitor *h, int larb_idx);       /* called before disable larb clock */
	void (*restore)(struct larb_monitor *h, int larb_idx);      /* called after enable larb clock */
};


/* PLL data structure */
struct pll;

struct pll_ops {
	int (*get_state)(struct pll *pll);
	void (*enable)(struct pll *pll);
	void (*disable)(struct pll *pll);
	void (*fsel)(struct pll *pll, unsigned int value);
	int (*dump_regs)(struct pll *pll, unsigned int *ptr);
	unsigned int (*vco_calc)(struct pll *pll);
	int (*hp_enable)(struct pll *pll);
	int (*hp_disable)(struct pll *pll);
};

struct pll {
	const char *name;
	const int type;
	const int mode;
	const int feat;
	int state;
	unsigned int cnt;
	const unsigned int en_mask;
	void __iomem *base_addr;
	void __iomem *pwr_addr;
	const struct pll_ops *ops;
	const unsigned int hp_id;
	const int hp_switch;
};

/* SUBSYS data structure */
struct subsys;

struct subsys_ops {
	int (*enable)(struct subsys *sys);
	int (*disable)(struct subsys *sys);
	int (*get_state)(struct subsys *sys);
	int (*dump_regs)(struct subsys *sys, unsigned int *ptr);
};

struct subsys {
	const char *name;
	const int type;
	const int force_on;
	unsigned int state;
	unsigned int default_sta;
	const unsigned int sta_mask;	/* mask in PWR_STATUS */
	void __iomem *ctl_addr;		/* SPM_XXX_PWR_CON */
	const struct subsys_ops *ops;
	const struct cg_grp *start;
	const unsigned int nr_grps;
};


/* CLKMUX data structure */
struct clkmux;

struct clkmux_ops {
	int (*sel)(struct clkmux *mux, enum cg_clk_id clksrc);
	enum cg_clk_id (*get)(struct clkmux *mux);
};

struct clkmux_map {
	const unsigned int val;
	const enum cg_clk_id id;
	const unsigned int mask;
};

struct clkmux {
	const char *name;
	void __iomem *base_addr;
	unsigned int offset;
	const struct clkmux_ops *ops;
	const struct clkmux_map *map;
	const unsigned int nr_map;
	const enum cg_clk_id drain;
};


/* CG_GRP data structure */
struct cg_grp;

struct cg_grp_ops {
	unsigned int (*get_state)(struct cg_grp *grp);
	int (*dump_regs)(struct cg_grp *grp, unsigned int *ptr);
};

struct cg_grp {
	const char *name;
	void __iomem *set_addr;
	void __iomem *clr_addr;
	void __iomem *sta_addr;
	unsigned int mask;
	unsigned int state;
	const struct cg_grp_ops *ops;
	struct subsys *sys;
};

/* CG_CLK data structure */
struct cg_clk;

struct cg_clk_ops {
	int (*get_state)(struct cg_clk *clk);
	int (*check_validity)(struct cg_clk *clk);	/* 1: valid, 0: invalid */
	int (*enable)(struct cg_clk *clk);
	int (*disable)(struct cg_clk *clk);
};

struct cg_clk {
	const char *name;
	int cnt;
	unsigned int state;
	const unsigned int mask;
	int force_on;
	const struct cg_clk_ops *ops;
	struct cg_grp *grp;
	struct pll *parent;

	enum cg_clk_id src;

};
/*=============================================================*/
/* Global function definition */
/*=============================================================*/

extern int enable_clock(enum cg_clk_id id, char *name);
extern int disable_clock(enum cg_clk_id id, char *name);
extern int clkmux_sel(enum clkmux_id id, enum cg_clk_id clksrc, const char *name);
extern int clkmux_get(enum clkmux_id id, const char *name);
extern void enable_mux(int id, char *name);
extern void disable_mux(int id, char *name);
extern int mt_enable_clock(enum cg_clk_id id, const char *name);
extern int mt_disable_clock(enum cg_clk_id id, const char *name);
extern int clock_is_on(enum cg_clk_id id);
extern void dump_clk_info_by_id(enum cg_clk_id id);
extern int enable_pll(enum pll_id id, const char *name);
extern int disable_pll(enum pll_id id, const char *name);
extern int pll_fsel(enum pll_id id, unsigned int value);
extern int pll_is_on(enum pll_id id);
extern int enable_subsys(enum subsys_id id, const char *name);
extern int disable_subsys(enum subsys_id id, const char *name);
extern int disable_subsys_force(enum subsys_id id, const char *name);
extern void register_larb_monitor(struct larb_monitor *handler);
extern void unregister_larb_monitor(struct larb_monitor *handler);
extern int md_power_on(enum subsys_id id);
extern int md_power_off(enum subsys_id id, unsigned int timeout);
extern int conn_power_on(void);
extern int conn_power_off(void);
extern int subsys_is_on(enum subsys_id id);
extern const char *grp_get_name(enum cg_grp_id id);
extern int grp_dump_regs(enum cg_grp_id id, unsigned int *ptr);
extern const char *pll_get_name(enum pll_id id);
extern int pll_dump_regs(enum pll_id id, unsigned int *ptr);
extern const char *subsys_get_name(enum subsys_id id);
extern int subsys_dump_regs(enum subsys_id id, unsigned int *ptr);
extern int snapshot_golden_setting(const char *func, const unsigned int line);
extern void print_mtcmos_trace_info_for_met(void);
extern bool clkmgr_idle_can_enter(unsigned int *condition_mask, unsigned int *block_mask);
extern enum cg_grp_id clk_id_to_grp_id(enum cg_clk_id id);
extern unsigned int clk_id_to_mask(enum cg_clk_id id);
extern int clkmgr_is_locked(void);
extern int mt_clkmgr_init(void);
extern void set_mipi26m(int en);
extern void clk_set_force_on(int id);
extern void clk_clr_force_on(int id);
extern int clk_is_force_on(int id);
extern void slp_check_pm_mtcmos_pll(void);
extern void clk_stat_bug(void);
extern void clkmgr_faudintbus_sq2pll(void);
extern void clkmgr_faudintbus_pll2sq(void);
#ifdef CONFIG_MMC_MTK
extern void msdc_clk_status(int *status);
#endif

#endif /* __MT_CLKMGR_H__ */

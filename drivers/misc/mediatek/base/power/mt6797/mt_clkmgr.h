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

#ifndef _MT_CLKMGR_H
#define _MT_CLKMGR_H

#include <linux/list.h>
/*#include "mach/mt_reg_base.h"*/
/*#include "mach/mt_typedefs.h"*/

#define CONFIG_CLKMGR_STAT

#ifdef CONFIG_OF
extern void __iomem  *clk_apmixed_base;
extern void __iomem  *clk_cksys_base;
extern void __iomem  *clk_infracfg_ao_base;
extern void __iomem  *clk_audio_base;
extern void __iomem  *clk_mfgcfg_base;
extern void __iomem  *clk_mmsys_config_base;
extern void __iomem  *clk_imgsys_base;
extern void __iomem  *clk_vdec_gcon_base;
extern void __iomem  *clk_mjc_config_base;
extern void __iomem  *clk_venc_gcon_base;
/*extern void __iomem  *clk_mcumixed_base;*/
extern void __iomem	*clk_camsys_base;
extern void __iomem	*clk_topmics_base;
#endif


/* APMIXEDSYS Register */
#define AP_PLL_CON0             (clk_apmixed_base + 0x00)
#define AP_PLL_CON0             (clk_apmixed_base + 0x00)
#define AP_PLL_CON1             (clk_apmixed_base + 0x04)
#define AP_PLL_CON2             (clk_apmixed_base + 0x08)
#define AP_PLL_CON3             (clk_apmixed_base + 0x0C)
#define AP_PLL_CON4             (clk_apmixed_base + 0x10)
#define AP_PLL_CON5             (clk_apmixed_base + 0x14)
#define AP_PLL_CON6             (clk_apmixed_base + 0x18)
#define AP_PLL_CON7             (clk_apmixed_base + 0x1C)
#define CLKSQ_STB_CON0          (clk_apmixed_base + 0x20)
#define PLL_PWR_CON0            (clk_apmixed_base + 0x24)
#define PLL_PWR_CON1            (clk_apmixed_base + 0x28)
#define PLL_ISO_CON0            (clk_apmixed_base + 0x2C)
#define PLL_ISO_CON1            (clk_apmixed_base + 0x30)
#define PLL_STB_CON0            (clk_apmixed_base + 0x34)
#define DIV_STB_CON0            (clk_apmixed_base + 0x38)
#define PLL_CHG_CON0            (clk_apmixed_base + 0x3C)
#define PLL_TEST_CON0           (clk_apmixed_base + 0x40)
#define MAINPLL_CON0            (clk_apmixed_base + 0x220)
#define MAINPLL_CON1            (clk_apmixed_base + 0x224)
#define MAINPLL_PWR_CON0        (clk_apmixed_base + 0x22C)
#define UNIVPLL_CON0            (clk_apmixed_base + 0x230)
#define UNIVPLL_CON1            (clk_apmixed_base + 0x234)
#define UNIVPLL_PWR_CON0        (clk_apmixed_base + 0x23C)
#define MFGPLL_CON0              (clk_apmixed_base + 0x240)
#define MFGPLL_CON1              (clk_apmixed_base + 0x244)
#define MMPLL_CON1              (clk_apmixed_base + 0x244) /*keep for gpufreq, remove later*/
#define MFGPLL_PWR_CON0          (clk_apmixed_base + 0x24C)
#define MSDCPLL_CON0            (clk_apmixed_base + 0x250)
#define MSDCPLL_CON1            (clk_apmixed_base + 0x254)
#define MSDCPLL_PWR_CON0        (clk_apmixed_base + 0x25C)
#define IMGPLL_CON0            (clk_apmixed_base + 0x260)
#define IMGPLL_CON1            (clk_apmixed_base + 0x264)
#define IMGPLL_PWR_CON0        (clk_apmixed_base + 0x26C)
#define TVDPLL_CON0             (clk_apmixed_base + 0x270)
#define TVDPLL_CON1             (clk_apmixed_base + 0x274)
#define TVDPLL_PWR_CON0         (clk_apmixed_base + 0x27C)
#define MPLL_CON0               (clk_apmixed_base + 0x280)
#define MPLL_CON1               (clk_apmixed_base + 0x284)
#define MPLL_PWR_CON0           (clk_apmixed_base + 0x28C)
#define CODECPLL_CON0          (clk_apmixed_base + 0x290)
#define CODECPLL_CON1          (clk_apmixed_base + 0x294)
#define CODECPLL_PWR_CON0      (clk_apmixed_base + 0x29C)
#define APLL1_CON0              (clk_apmixed_base + 0x2A0)
#define APLL1_CON1              (clk_apmixed_base + 0x2A4)
#define APLL1_CON2              (clk_apmixed_base + 0x2A8)
#define APLL1_PWR_CON0          (clk_apmixed_base + 0x2B0)
#define APLL2_CON0              (clk_apmixed_base + 0x2B4)
#define APLL2_CON1              (clk_apmixed_base + 0x2B8)
#define APLL2_CON2              (clk_apmixed_base + 0x2BC)
#define APLL2_PWR_CON0          (clk_apmixed_base + 0x2C4)
#define VDECPLL_CON0              (clk_apmixed_base + 0x2E4)
#define VDECPLL_CON1              (clk_apmixed_base + 0x2E8)
#define VDECPLL_CON2              (clk_apmixed_base + 0x2EC)
#define VDECPLL_PWR_CON0          (clk_apmixed_base + 0x2F0)

/* MCUMIXEDSYS Register */
/* base addr will be add in freq hopping */
#define MCU_PLL_CON0            (0x000)
#define MCU_PLL_CON1            (0x004)
#define MCU_PLL_CON2            (0x008)

#define ARMCAXPLL0_CON0         (0x200)
#define ARMCAXPLL0_CON1         (0x204)
#define ARMCAXPLL0_CON2         (0x208)
#define ARMCAXPLL0_PWR_CON0      (0x20c)

#define ARMCAXPLL1_CON0         (0x210)
#define ARMCAXPLL1_CON1         (0x214)
#define ARMCAXPLL1_CON2         (0x218)
#define ARMCAXPLL1_PWR_CON0      (0x21c)

#define ARMCAXPLL2_CON0         (0x220)
#define ARMCAXPLL2_CON1         (0x224)
#define ARMCAXPLL2_CON2         (0x228)
#define ARMCAXPLL2_PWR_CON0      (0x22c)

#define ARMCAXPLL3_CON0         (0x230)
#define ARMCAXPLL3_CON1         (0x234)
#define ARMCAXPLL3_CON2         (0x238)
#define ARMCAXPLL3_PWR_CON0     (0x23c)

#define ARMPLLDIV_MUXSEL        (0x270)
#define ARMPLLDIV_CKDIV         (0x274)
#define ARMPLLDIV_ARM_K1		(0x27C)
#define ARMPLLDIV_MON_EN        (0x284)

/*TOPMICS Register*/
#define TOPMISC_TEST_MODE_CFG	(clk_topmics_base + 0)

/* TOPCKGEN Register */
#define CLK_MODE                (clk_cksys_base + 0x000)
#define CLK_CFG_UPDATE          (clk_cksys_base + 0x004)
#if 0
#define TST_SEL_0               (clk_cksys_base + 0x020)
#define TST_SEL_1               (clk_cksys_base + 0x024)
#define TST_SEL_2               (clk_cksys_base + 0x028)
#endif
#define CLK_CFG_0               (clk_cksys_base + 0x040)
#define CLK_CFG_1               (clk_cksys_base + 0x050)
#define CLK_CFG_2               (clk_cksys_base + 0x060)
#define CLK_CFG_3               (clk_cksys_base + 0x070)
#define CLK_CFG_4               (clk_cksys_base + 0x080)
#define CLK_CFG_5               (clk_cksys_base + 0x090)
#define CLK_CFG_6               (clk_cksys_base + 0x0A0)
#define CLK_CFG_7               (clk_cksys_base + 0x0B0)
#define CLK_CFG_8               (clk_cksys_base + 0x0C0)
#if 0
#define CLK_CFG_10               (clk_cksys_base + 0xDE)
#define CLK_CFG_11               (clk_cksys_base + 0xAD)
#endif

#define CLK_MISC_CFG_0          (clk_cksys_base + 0x104)
#define CLK_MISC_CFG_1          (clk_cksys_base + 0x108)
#define CLK_DBG_CFG             (clk_cksys_base + 0x10C)
#define CLK_SCP_CFG_0           (clk_cksys_base + 0x200)
#define CLK_SCP_CFG_1           (clk_cksys_base + 0x204)
#define CLK26CALI_0             (clk_cksys_base + 0x220)
#define CLK26CALI_1             (clk_cksys_base + 0x224)
#define CKSTA_REG               (clk_cksys_base + 0x22C)


/* INFRASYS Register */
#if 0
#define TOP_CKMUXSEL            (clk_infracfg_ao_base + 0x00)
#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)
#define INFRA_TOPCKGEN_CKDIV1_BIG (clk_infracfg_ao_base + 0x0024)
#define INFRA_TOPCKGEN_CKDIV1_SML (clk_infracfg_ao_base + 0x0028)
#define INFRA_TOPCKGEN_CKDIV1_BUS (clk_infracfg_ao_base + 0x002C)
#endif

#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)
#define TOP_DCMCTL              (clk_infracfg_ao_base + 0x10)
#define INFRA_BUS_DCM_CTRL      (clk_infracfg_ao_base + 0x70)
#define PERI_BUS_DCM_CTRL       (clk_infracfg_ao_base + 0x74)
#define INFRA_SW_CG0_SET          (clk_infracfg_ao_base + 0x80)
#define INFRA_SW_CG0_CLR          (clk_infracfg_ao_base + 0x84)
#define INFRA_SW_CG1_SET          (clk_infracfg_ao_base + 0x88)
#define INFRA_SW_CG1_CLR          (clk_infracfg_ao_base + 0x8C)
#define INFRA_SW_CG0_STA          (clk_infracfg_ao_base + 0x90)
#define INFRA_SW_CG1_STA          (clk_infracfg_ao_base + 0x94)
#define INFRA_MODULE_CLK_SEL      (clk_infracfg_ao_base + 0x98)
#define INFRA_SW_CG2_SET          (clk_infracfg_ao_base + 0xA8)
#define INFRA_SW_CG2_CLR          (clk_infracfg_ao_base + 0xAC)
#define INFRA_SW_CG2_STA          (clk_infracfg_ao_base + 0xB0)
#define INFRA_I2C_DBTOOL_MISC     (clk_infracfg_ao_base + 0x100)
#define TOPAXI_PROT_EN           (clk_infracfg_ao_base + 0x0220)
#define TOPAXI_PROT_STA1         (clk_infracfg_ao_base + 0x0228)
#define INFRA_TOPAXI_PROTECTEN   (clk_infracfg_ao_base + 0x0220)
#define INFRA_TOPAXI_PROTECTSTA0 (clk_infracfg_ao_base + 0x0224)
#define INFRA_TOPAXI_PROTECTSTA1 (clk_infracfg_ao_base + 0x0228)
#define INFRA_PLL_ULPOSC_CON0          (clk_infracfg_ao_base + 0xB00)
#define INFRA_AO_DBG_CON0	(clk_infracfg_ao_base + 0x500)
#define INFRA_AO_DBG_CON1	(clk_infracfg_ao_base + 0x504)
#define INFRA_PDN_STA0          (clk_infracfg_ao_base + 0x0090)

/* Audio Register*/
#define AUDIO_TOP_CON0          (clk_audio_base + 0x0000)
#define AUDIO_TOP_CON1			(clk_audio_base + 0x0004)

/* MFGCFG Register*/
#define MFG_CG_CON              (clk_mfgcfg_base + 0)
#define MFG_CG_SET              (clk_mfgcfg_base + 4)
#define MFG_CG_CLR              (clk_mfgcfg_base + 8)

/* MMSYS Register*/

#define DISP_CG_CON0            (clk_mmsys_config_base + 0x100)
#define DISP_CG_SET0            (clk_mmsys_config_base + 0x104)
#define DISP_CG_CLR0            (clk_mmsys_config_base + 0x108)
#define DISP_CG_CON1            (clk_mmsys_config_base + 0x110)
#define DISP_CG_SET1            (clk_mmsys_config_base + 0x114)
#define DISP_CG_CLR1            (clk_mmsys_config_base + 0x118)
#if 0
#define MMSYS_DUMMY             (clk_mmsys_config_base + 0x890)
#define	SMI_LARB_BWL_EN_REG     (clk_mmsys_config_base + 0x21050)
#endif
/* IMGSYS Register */
#define IMG_CG_CON              (clk_imgsys_base + 0x0000)
#define IMG_CG_SET              (clk_imgsys_base + 0x0004)
#define IMG_CG_CLR              (clk_imgsys_base + 0x0008)

/* VDEC Register */
#define VDEC_CKEN_SET           (clk_vdec_gcon_base + 0x0000)
#define VDEC_CKEN_CLR           (clk_vdec_gcon_base + 0x0004)
#define LARB_CKEN_SET           (clk_vdec_gcon_base + 0x0008)
#define LARB_CKEN_CLR           (clk_vdec_gcon_base + 0x000C)

/* MJC Register*/
#define MJC_CG_CON              (clk_mjc_config_base + 0x0000)
#define MJC_CG_SET              (clk_mjc_config_base + 0x0004)
#define MJC_CG_CLR              (clk_mjc_config_base + 0x0008)

/* VENC Register*/
#define VENC_CG_CON             (clk_venc_gcon_base + 0x0)
#define VENC_CG_SET             (clk_venc_gcon_base + 0x4)
#define VENC_CG_CLR             (clk_venc_gcon_base + 0x8)

/* CAM Register*/
#define CAM_CG_CON				(clk_camsys_base + 0x0)
#define CAM_CG_SET				(clk_camsys_base + 0x4)
#define CAM_CG_CLR				(clk_camsys_base + 0x8)

enum {
	MAINPLL    = 0,
	UNIVPLL    = 1,
	MFGPLL      = 2,
	MSDCPLL      = 3,
	IMGPLL      = 4,
	TVDPLL     = 5,
	MPLL       = 6,
	CODECPLL      = 7,
	VDECPLL      = 8,
	APLL1      = 9,
	APLL2      = 10,
	NR_PLLS    = 11,
};

enum {
	SYS_MD1 = 0,
	SYS_CONN = 1,
	SYS_DIS = 2,
	SYS_ISP = 3,
	SYS_VDE = 4,
	SYS_MFG_CORE3 = 5,
	SYS_MFG_CORE2 = 6,
	SYS_MFG_CORE1 = 7,
	SYS_MFG_CORE0 = 8,
	SYS_MFG = 9,
	SYS_MFG_ASYNC = 10,
	SYS_MJC = 11,
	SYS_VEN = 12,
	SYS_AUDIO = 13,
	SYS_C2K = 14,
	NR_SYSS = 15,
};

#define _ARMPLL_B_DIV_MASK_    0xFFFFFFE0
#define _ARMPLL_LL_DIV_MASK_   0xFFFFFC1F
#define _ARMPLL_L_DIV_MASK_    0xFFFF83FF
#define _ARMPLL_CCI_DIV_MASK_  0xFFF07FFF

#define _ARMPLL_B_DIV_BIT_  (0)
#define _ARMPLL_LL_DIV_BIT_ (5)
#define _ARMPLL_L_DIV_BIT_	(10)
#define _ARMPLL_CCI_DIV_BIT_ (15)

#define _ARMPLL_DIV_4_ 11
#define _ARMPLL_DIV_2_ 10
#define _ARMPLL_DIV_1_ 8


extern int clkmgr_is_locked(void);

/* init */
extern void mt_clkmgr_init(void);

extern void slp_check_pm_mtcmos_pll(void);
extern void clk_misc_cfg_ops(bool flag);

/* pll API */
extern void enable_armpll_ll(void);
extern void disable_armpll_ll(void);
extern void enable_armpll_l(void);
extern void disable_armpll_l(void);
extern void switch_armpll_l_hwmode(int enable);
extern void switch_armpll_ll_hwmode(int enable);
extern int subsys_is_on(int id);
#endif

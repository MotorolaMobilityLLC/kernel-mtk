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
#endif


/* APMIXEDSYS Register */
#if 0
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
#endif

/*ARMPLL1*/
#define ARMPLL1_CON0           (clk_apmixed_base + 0x310)
#define ARMPLL1_CON1           (clk_apmixed_base + 0x314)
#define ARMPLL1_PWR_CON0           (clk_apmixed_base + 0x31C)
/*ARMPLL2*/
#define ARMPLL2_CON0           (clk_apmixed_base + 0x320)
#define ARMPLL2_CON1           (clk_apmixed_base + 0x324)
#define ARMPLL2_PWR_CON0           (clk_apmixed_base + 0x32C)
/*ARMPLL3*/
#define ARMPLL3_CON0           (clk_apmixed_base + 0x330)
#define ARMPLL3_CON1           (clk_apmixed_base + 0x334)
#define ARMPLL3_PWR_CON0           (clk_apmixed_base + 0x344)
/*CCIPLL*/
#define CCIPLL_CON0           (clk_apmixed_base + 0x350)
#define CCIPLL_CON1           (clk_apmixed_base + 0x354)
#define CCIPLL_PWR_CON0           (clk_apmixed_base + 0x35C)
/*MMPLL*/
#define MMPLL_CON0           (clk_apmixed_base + 0x260)
#define MMPLL_CON1           (clk_apmixed_base + 0x264)
#define MMPLL_PWR_CON0           (clk_apmixed_base + 0x26C)
/*GPUPLL*/
#define GPUPLL_CON0           (clk_apmixed_base + 0x210)
#define GPUPLL_CON1           (clk_apmixed_base + 0x214)
#define GPUPLL_PWR_CON0           (clk_apmixed_base + 0x21C)

/*ARMPLL DIV*/
#if 0
#define INFRA_TOPCKGEN_CKDIV1_BUS	(clk_infracfg_ao_base + 0x08)
#define INFRA_TOPCKGEN_CKDIV1_SML	(clk_infracfg_ao_base + 0x24)
#define INFRA_TOPCKGEN_CKDIV1_BIG	(clk_infracfg_ao_base + 0x28)
#define INFRA_TOPCKGEN_CKDIV1			(clk_infracfg_ao_base + 0x2C)
#endif
/* MCUCFG Register */
#define MP0_PLL_DIV_CFG           (clk_mcucfg_base + 0x7A0) /*ARMPLL_M*/
#define MP1_PLL_DIV_CFG           (clk_mcucfg_base + 0x7A4) /*ARMPLL_L*/
#define MP2_PLL_DIV_CFG           (clk_mcucfg_base + 0x7A8) /*ARMPLL_B*/
#define BUS_PLL_DIV_CFG           (clk_mcucfg_base + 0x7C0) /*CCIPLL*/

#if 0
#define MAINPLL_CON0            (clk_apmixed_base + 0x220)
#define MAINPLL_CON1            (clk_apmixed_base + 0x224)
#define MAINPLL_PWR_CON0        (clk_apmixed_base + 0x22C)
#define UNIVPLL_CON0            (clk_apmixed_base + 0x230)
#define UNIVPLL_CON1            (clk_apmixed_base + 0x234)
#define UNIVPLL_PWR_CON0        (clk_apmixed_base + 0x23C)
#define MMPLL_CON0              (clk_apmixed_base + 0x240)
#define MMPLL_CON1              (clk_apmixed_base + 0x244)
#define MMPLL_PWR_CON0          (clk_apmixed_base + 0x24C)
#define MSDCPLL_CON0            (clk_apmixed_base + 0x250)
#define MSDCPLL_CON1            (clk_apmixed_base + 0x254)
#define MSDCPLL_PWR_CON0        (clk_apmixed_base + 0x25C)
#define VENCPLL_CON0            (clk_apmixed_base + 0x260)
#define VENCPLL_CON1            (clk_apmixed_base + 0x264)
#define VENCPLL_PWR_CON0        (clk_apmixed_base + 0x26C)
#define TVDPLL_CON0             (clk_apmixed_base + 0x270)
#define TVDPLL_CON1             (clk_apmixed_base + 0x274)
#define TVDPLL_PWR_CON0         (clk_apmixed_base + 0x27C)
#define MPLL_CON0               (clk_apmixed_base + 0x280)
#define MPLL_CON1               (clk_apmixed_base + 0x284)
#define MPLL_PWR_CON0           (clk_apmixed_base + 0x28C)

#define APLL1_CON0              (clk_apmixed_base + 0x2A0)
#define APLL1_CON1              (clk_apmixed_base + 0x2A4)
#define APLL1_PWR_CON0          (clk_apmixed_base + 0x2B0)
#define APLL2_CON0              (clk_apmixed_base + 0x2B4)
#define APLL2_CON1              (clk_apmixed_base + 0x2B8)
#define APLL2_PWR_CON0          (clk_apmixed_base + 0x2C4)
#endif
/*TOPMICS Register*/
/*define TOPMISC_TEST_MODE_CFG	(clk_topmics_base + 0)*/

/* TOPCKGEN Register */
#if 0
#define CLK_MODE                (clk_cksys_base + 0x000)
#define CLK_CFG_UPDATE          (clk_cksys_base + 0x004)
#define CLK_CFG_UPDATE1          (clk_cksys_base + 0x008)

#define CLK_CFG_0               (clk_cksys_base + 0x040)
#define CLK_CFG_1               (clk_cksys_base + 0x050)
#define CLK_CFG_2               (clk_cksys_base + 0x060)
#define CLK_CFG_3               (clk_cksys_base + 0x070)
#define CLK_CFG_4               (clk_cksys_base + 0x080)
#define CLK_CFG_5               (clk_cksys_base + 0x090)
#define CLK_CFG_6               (clk_cksys_base + 0x0A0)
#define CLK_CFG_7               (clk_cksys_base + 0x0B0)
#define CLK_CFG_8               (clk_cksys_base + 0x0C0)
#define CLK_CFG_9               (clk_cksys_base + 0x0D0)

#define CLK_MISC_CFG_0          (clk_cksys_base + 0x104)
#define CLK_MISC_CFG_1          (clk_cksys_base + 0x108)
#define CLK_DBG_CFG             (clk_cksys_base + 0x10C)
#define CLK_SCP_CFG_0           (clk_cksys_base + 0x200)
#define CLK_SCP_CFG_1           (clk_cksys_base + 0x204)
#define CLK26CALI_0             (clk_cksys_base + 0x220)
#define CLK26CALI_1             (clk_cksys_base + 0x224)
#endif

#if 0
/* INFRASYS Register */
#define TOP_CKDIV1              (clk_infracfg_ao_base + 0x08)
#define TOP_DCMCTL              (clk_infracfg_ao_base + 0x10)
#define TOP_METER              (clk_infracfg_ao_base + 0x1c)
#endif
#define INFRA_BUS_DCM_CTRL      (clk_infracfg_ao_base + 0x70)
#define PERI_BUS_DCM_CTRL       (clk_infracfg_ao_base + 0x74)
#define INFRA_SW_CG0_SET          (clk_infracfg_ao_base + 0x80)
#define INFRA_SW_CG0_CLR          (clk_infracfg_ao_base + 0x84)
#define INFRA_SW_CG1_SET          (clk_infracfg_ao_base + 0x88)
#define INFRA_SW_CG1_CLR          (clk_infracfg_ao_base + 0x8C)
#define INFRA_SW_CG0_STA          (clk_infracfg_ao_base + 0x90)
#define INFRA_SW_CG1_STA          (clk_infracfg_ao_base + 0x94)
#define INFRA_MODULE_CLK_SEL      (clk_infracfg_ao_base + 0x98)
#define INFRA_SW_CG2_SET          (clk_infracfg_ao_base + 0xA4)
#define INFRA_SW_CG2_CLR          (clk_infracfg_ao_base + 0xA8)
#define INFRA_SW_CG2_STA          (clk_infracfg_ao_base + 0xAC)
#define INFRA_I2C_DBTOOL_MISC     (clk_infracfg_ao_base + 0x100)
#define INFRA_TOPAXI_PROTECTEN   (clk_infracfg_ao_base + 0x0220)
#define INFRA_TOPAXI_PROTECTSTA0 (clk_infracfg_ao_base + 0x0224)
#define INFRA_TOPAXI_PROTECTSTA1 (clk_infracfg_ao_base + 0x0228)
#define INFRA_PLL_ULPOSC_CON0          (clk_infracfg_ao_base + 0xB00)
#define INFRA_AO_DBG_CON0	(clk_infracfg_ao_base + 0x500)
#define INFRA_AO_DBG_CON1	(clk_infracfg_ao_base + 0x504)

/* Audio Register*/
#define AUDIO_TOP_CON0          (clk_audio_base + 0x0000)

/* MFGCFG Register*/
#define MFG_CG_CON              (clk_mfgcfg_base + 0)

/* MMSYS Register*/

#define DISP_CG_CON0            (clk_mmsys_config_base + 0x100)
#define DISP_CG_CON1            (clk_mmsys_config_base + 0x110)
#define DISP_CG_CON0_DUMMY      (clk_mmsys_config_base + 0x8A8)
#define DISP_CG_CON1_DUMMY      (clk_mmsys_config_base + 0x8AC)
#if 0
#define MMSYS_DUMMY             (clk_mmsys_config_base + 0x890)
#define	SMI_LARB_BWL_EN_REG     (clk_mmsys_config_base + 0x21050)

/* IMGSYS Register */
#define IMG_CG_CON              (clk_imgsys_base + 0x0000)

/* VDEC Register */
#define VDEC_CKEN_SET           (clk_vdec_gcon_base + 0x0000)
#define LARB_CKEN_SET           (clk_vdec_gcon_base + 0x0008)

/* VENC Register*/
#define VENC_CG_CON             (clk_venc_gcon_base + 0x0)

/* CAM Register*/
#define CAM_CG_CON				(clk_camsys_base + 0x0)

#endif



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

extern unsigned int mt_get_abist_freq(unsigned int ID);
extern unsigned int mt_get_ckgen_freq(unsigned int ID);
#endif

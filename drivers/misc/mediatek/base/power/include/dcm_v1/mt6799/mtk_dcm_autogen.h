/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __MTK_DCM_AUTOGEN_H__
#define __MTK_DCM_AUTOGEN_H__

#include <mtk_dcm.h>

#if defined(__KERNEL__) && defined(CONFIG_OF)
extern unsigned long dcm_topckgen_base;
extern unsigned long dcm_infracfg_ao_base;
extern unsigned long dcm_pericfg_base;
extern unsigned long dcm_mcucfg_base;
extern unsigned long dcm_mcucfg_phys_base;
extern unsigned long dcm_cci_base;
extern unsigned long dcm_cci_phys_base;
extern unsigned long dcm_lpdma_base;
extern unsigned long dcm_lpdma_phys_base;
#ifndef USE_DRAM_API_INSTEAD
extern unsigned long dcm_dramc0_ao_base;
extern unsigned long dcm_dramc1_ao_base;
extern unsigned long dcm_dramc1_ao_base;
extern unsigned long dcm_dramc2_ao_base;
extern unsigned long dcm_dramc3_ao_base;
extern unsigned long dcm_ddrphy0_ao_base;
extern unsigned long dcm_ddrphy1_ao_base;
extern unsigned long dcm_ddrphy2_ao_base;
extern unsigned long dcm_ddrphy3_ao_base;
#endif
extern unsigned long dcm_emi_base;
extern unsigned long dcm_chn0_emi_base;
extern unsigned long dcm_chn1_emi_base;
extern unsigned long dcm_chn2_emi_base;
extern unsigned long dcm_chn3_emi_base;

#define TOPCKGEN_BASE		(dcm_topckgen_base)
#define INFRACFG_AO_BASE	(dcm_infracfg_ao_base)
#define PERICFG_BASE		(dcm_pericfg_base)
#define MCUCFG_BASE		(dcm_mcucfg_base)
#define MCSI_REG_BASE		(dcm_cci_base)
#define MCSI_PHYS_BASE		(dcm_cci_phys_base)
#define LPDMA_BASE		(dcm_lpdma_base)
#define LPDMA_PHYS_BASE		(dcm_lpdma_phys_base)
#ifndef USE_DRAM_API_INSTEAD
#define DRAMC0_AO_BASE		(dcm_dramc0_ao_base)
#define DRAMC1_AO_BASE		(dcm_dramc1_ao_base)
#define DRAMC2_AO_BASE		(dcm_dramc2_ao_base)
#define DRAMC3_AO_BASE		(dcm_dramc3_ao_base)
#define DDRPHY0AO_BASE		(dcm_ddrphy0_ao_base)
#define DDRPHY1AO_BASE		(dcm_ddrphy1_ao_base)
#define DDRPHY2AO_BASE		(dcm_ddrphy2_ao_base)
#define DDRPHY3AO_BASE		(dcm_ddrphy3_ao_base)
#endif
#define EMI_BASE		(dcm_emi_base)
#define CHN0_EMI_BASE		(dcm_chn0_emi_base)
#define CHN1_EMI_BASE		(dcm_chn1_emi_base)
#define CHN2_EMI_BASE		(dcm_chn2_emi_base)
#define CHN3_EMI_BASE		(dcm_chn3_emi_base)
#endif /* #if defined(__KERNEL__) && defined(CONFIG_OF) */

/* Register Definition */
#define CCI_DCM (MCSI_REG_BASE + 0x18)
#define CCI_DCM_PHYS (MCSI_PHYS_BASE + 0x18)
#define CCI_CACTIVE (MCSI_REG_BASE + 0x1c)
#define CCI_CACTIVE_PHYS (MCSI_PHYS_BASE + 0x1c)
/* #define PMIC_WRAP_DCM_EN (PWRAP_BASE + 0x1cc) */
#define MP0_SCAL_SYNC_DCM_CONFIG (MCUCFG_BASE + 0x108)
#define MP0_SCAL_CCI_ADB400_DCM_CONFIG (MCUCFG_BASE + 0x114)
#define MP0_SCAL_BUS_FABRIC_DCM_CTRL (MCUCFG_BASE + 0x118)
#define MP0_SCAL_SYNC_DCM_CLUSTER_CONFIG (MCUCFG_BASE + 0x11c)
#define L2C_SRAM_CTRL (MCUCFG_BASE + 0x648)
#define CCI_CLK_CTRL (MCUCFG_BASE + 0x660)
#define BUS_FABRIC_DCM_CTRL (MCUCFG_BASE + 0x668)
#define MCU_MISC_DCM_CTRL (MCUCFG_BASE + 0x66c)
#define CCI_ADB400_DCM_CONFIG (MCUCFG_BASE + 0x740)
#define SYNC_DCM_CONFIG (MCUCFG_BASE + 0x744)
#define SYNC_DCM_CLUSTER_CONFIG (MCUCFG_BASE + 0x74c)
#define MCUSYS_GIC_SYNC_DCM (MCUCFG_BASE + 0x758)
#define MP0_PLL_DIVIDER_CFG (MCUCFG_BASE + 0x7a0)
#define MP1_PLL_DIVIDER_CFG (MCUCFG_BASE + 0x7a4)
#define MP2_PLL_DIVIDER_CFG (MCUCFG_BASE + 0x7a8)
#define BUS_PLL_DIVIDER_CFG (MCUCFG_BASE + 0x7c0)
#define MCUCFG_MP0_RGU_DCM (MCUCFG_BASE + 0x1c70)
#define MCUCFG_SYNC_DCM (MCUCFG_BASE + 0x2274)
#define MCUCFG_DBG_CONTROL (MCUCFG_BASE + 0x2728)
#define MCUCFG_MP1_RGU_DCM (MCUCFG_BASE + 0x3c70)
#define EMI_CONM (EMI_BASE + 0x60)
#define EMI_CONN (EMI_BASE + 0x68)
#define CHN0_EMI_CHN_EMI_CONB (CHN0_EMI_BASE + 0x8)
#define CHN1_EMI_CHN_EMI_CONB (CHN1_EMI_BASE + 0x8)
#define CHN2_EMI_CHN_EMI_CONB (CHN2_EMI_BASE + 0x8)
#define CHN3_EMI_CHN_EMI_CONB (CHN3_EMI_BASE + 0x8)
#define LPDMA_CONB (LPDMA_BASE + 0x8)
#ifndef USE_DRAM_API_INSTEAD
#define DDRPHY0AO_MISC_CG_CTRL0 (DDRPHY0AO_BASE + 0x284)
#define DDRPHY0AO_MISC_CG_CTRL2 (DDRPHY0AO_BASE + 0x28c)
#define DDRPHY0AO_MISC_CG_CTRL5 (DDRPHY0AO_BASE + 0x298)
#define DDRPHY0AO_MISC_CTRL3 (DDRPHY0AO_BASE + 0x2a8)
#define DDRPHY0AO_SHU1_B0_DQ0 (DDRPHY0AO_BASE + 0xc00)
#define DDRPHY0AO_RFU_0XC20 (DDRPHY0AO_BASE + 0xc20)
#define DDRPHY0AO_SHU1_B1_DQ0 (DDRPHY0AO_BASE + 0xc80)
#define DDRPHY0AO_RFU_0XCA0 (DDRPHY0AO_BASE + 0xca0)
#define DDRPHY0AO_SHU2_B0_DQ0 (DDRPHY0AO_BASE + 0x1100)
#define DDRPHY0AO_RFU_0X1120 (DDRPHY0AO_BASE + 0x1120)
#define DDRPHY0AO_SHU2_B1_DQ0 (DDRPHY0AO_BASE + 0x1180)
#define DDRPHY0AO_RFU_0X11A0 (DDRPHY0AO_BASE + 0x11a0)
#define DDRPHY0AO_SHU3_B0_DQ0 (DDRPHY0AO_BASE + 0x1600)
#define DDRPHY0AO_RFU_0X1620 (DDRPHY0AO_BASE + 0x1620)
#define DDRPHY0AO_SHU3_B1_DQ0 (DDRPHY0AO_BASE + 0x1680)
#define DDRPHY0AO_RFU_0X16A0 (DDRPHY0AO_BASE + 0x16a0)
#define DDRPHY0AO_SHU4_B0_DQ0 (DDRPHY0AO_BASE + 0x1b00)
#define DDRPHY0AO_RFU_0X1B20 (DDRPHY0AO_BASE + 0x1b20)
#define DDRPHY0AO_SHU4_B1_DQ0 (DDRPHY0AO_BASE + 0x1b80)
#define DDRPHY0AO_RFU_0X1BA0 (DDRPHY0AO_BASE + 0x1ba0)
#define DRAMC0_AO_DRAMC_PD_CTRL (DRAMC0_AO_BASE + 0x38)
#define DRAMC0_AO_CLKAR (DRAMC0_AO_BASE + 0x3c)
#define DDRPHY1AO_MISC_CG_CTRL0 (DDRPHY1AO_BASE + 0x284)
#define DDRPHY1AO_MISC_CG_CTRL2 (DDRPHY1AO_BASE + 0x28c)
#define DDRPHY1AO_MISC_CG_CTRL5 (DDRPHY1AO_BASE + 0x298)
#define DDRPHY1AO_MISC_CTRL3 (DDRPHY1AO_BASE + 0x2a8)
#define DDRPHY1AO_SHU1_B0_DQ0 (DDRPHY1AO_BASE + 0xc00)
#define DDRPHY1AO_RFU_0XC20 (DDRPHY1AO_BASE + 0xc20)
#define DDRPHY1AO_SHU1_B1_DQ0 (DDRPHY1AO_BASE + 0xc80)
#define DDRPHY1AO_RFU_0XCA0 (DDRPHY1AO_BASE + 0xca0)
#define DDRPHY1AO_SHU2_B0_DQ0 (DDRPHY1AO_BASE + 0x1100)
#define DDRPHY1AO_RFU_0X1120 (DDRPHY1AO_BASE + 0x1120)
#define DDRPHY1AO_SHU2_B1_DQ0 (DDRPHY1AO_BASE + 0x1180)
#define DDRPHY1AO_RFU_0X11A0 (DDRPHY1AO_BASE + 0x11a0)
#define DDRPHY1AO_SHU3_B0_DQ0 (DDRPHY1AO_BASE + 0x1600)
#define DDRPHY1AO_RFU_0X1620 (DDRPHY1AO_BASE + 0x1620)
#define DDRPHY1AO_SHU3_B1_DQ0 (DDRPHY1AO_BASE + 0x1680)
#define DDRPHY1AO_RFU_0X16A0 (DDRPHY1AO_BASE + 0x16a0)
#define DDRPHY1AO_SHU4_B0_DQ0 (DDRPHY1AO_BASE + 0x1b00)
#define DDRPHY1AO_RFU_0X1B20 (DDRPHY1AO_BASE + 0x1b20)
#define DDRPHY1AO_SHU4_B1_DQ0 (DDRPHY1AO_BASE + 0x1b80)
#define DDRPHY1AO_RFU_0X1BA0 (DDRPHY1AO_BASE + 0x1ba0)
#define DRAMC1_AO_DRAMC_PD_CTRL (DRAMC1_AO_BASE + 0x38)
#define DRAMC1_AO_CLKAR (DRAMC1_AO_BASE + 0x3c)
#define DDRPHY2AO_MISC_CG_CTRL0 (DDRPHY2AO_BASE + 0x284)
#define DDRPHY2AO_MISC_CG_CTRL2 (DDRPHY2AO_BASE + 0x28c)
#define DDRPHY2AO_MISC_CG_CTRL5 (DDRPHY2AO_BASE + 0x298)
#define DDRPHY2AO_MISC_CTRL3 (DDRPHY2AO_BASE + 0x2a8)
#define DDRPHY2AO_SHU1_B0_DQ0 (DDRPHY2AO_BASE + 0xc00)
#define DDRPHY2AO_RFU_0XC20 (DDRPHY2AO_BASE + 0xc20)
#define DDRPHY2AO_SHU1_B1_DQ0 (DDRPHY2AO_BASE + 0xc80)
#define DDRPHY2AO_RFU_0XCA0 (DDRPHY2AO_BASE + 0xca0)
#define DDRPHY2AO_SHU2_B0_DQ0 (DDRPHY2AO_BASE + 0x1100)
#define DDRPHY2AO_RFU_0X1120 (DDRPHY2AO_BASE + 0x1120)
#define DDRPHY2AO_SHU2_B1_DQ0 (DDRPHY2AO_BASE + 0x1180)
#define DDRPHY2AO_RFU_0X11A0 (DDRPHY2AO_BASE + 0x11a0)
#define DDRPHY2AO_SHU3_B0_DQ0 (DDRPHY2AO_BASE + 0x1600)
#define DDRPHY2AO_RFU_0X1620 (DDRPHY2AO_BASE + 0x1620)
#define DDRPHY2AO_SHU3_B1_DQ0 (DDRPHY2AO_BASE + 0x1680)
#define DDRPHY2AO_RFU_0X16A0 (DDRPHY2AO_BASE + 0x16a0)
#define DDRPHY2AO_SHU4_B0_DQ0 (DDRPHY2AO_BASE + 0x1b00)
#define DDRPHY2AO_RFU_0X1B20 (DDRPHY2AO_BASE + 0x1b20)
#define DDRPHY2AO_SHU4_B1_DQ0 (DDRPHY2AO_BASE + 0x1b80)
#define DDRPHY2AO_RFU_0X1BA0 (DDRPHY2AO_BASE + 0x1ba0)
#define DRAMC2_AO_DRAMC_PD_CTRL (DRAMC2_AO_BASE + 0x38)
#define DRAMC2_AO_CLKAR (DRAMC2_AO_BASE + 0x3c)
#define DDRPHY3AO_MISC_CG_CTRL0 (DDRPHY3AO_BASE + 0x284)
#define DDRPHY3AO_MISC_CG_CTRL2 (DDRPHY3AO_BASE + 0x28c)
#define DDRPHY3AO_MISC_CG_CTRL5 (DDRPHY3AO_BASE + 0x298)
#define DDRPHY3AO_MISC_CTRL3 (DDRPHY3AO_BASE + 0x2a8)
#define DDRPHY3AO_SHU1_B0_DQ0 (DDRPHY3AO_BASE + 0xc00)
#define DDRPHY3AO_RFU_0XC20 (DDRPHY3AO_BASE + 0xc20)
#define DDRPHY3AO_SHU1_B1_DQ0 (DDRPHY3AO_BASE + 0xc80)
#define DDRPHY3AO_RFU_0XCA0 (DDRPHY3AO_BASE + 0xca0)
#define DDRPHY3AO_SHU2_B0_DQ0 (DDRPHY3AO_BASE + 0x1100)
#define DDRPHY3AO_RFU_0X1120 (DDRPHY3AO_BASE + 0x1120)
#define DDRPHY3AO_SHU2_B1_DQ0 (DDRPHY3AO_BASE + 0x1180)
#define DDRPHY3AO_RFU_0X11A0 (DDRPHY3AO_BASE + 0x11a0)
#define DDRPHY3AO_SHU3_B0_DQ0 (DDRPHY3AO_BASE + 0x1600)
#define DDRPHY3AO_RFU_0X1620 (DDRPHY3AO_BASE + 0x1620)
#define DDRPHY3AO_SHU3_B1_DQ0 (DDRPHY3AO_BASE + 0x1680)
#define DDRPHY3AO_RFU_0X16A0 (DDRPHY3AO_BASE + 0x16a0)
#define DDRPHY3AO_SHU4_B0_DQ0 (DDRPHY3AO_BASE + 0x1b00)
#define DDRPHY3AO_RFU_0X1B20 (DDRPHY3AO_BASE + 0x1b20)
#define DDRPHY3AO_SHU4_B1_DQ0 (DDRPHY3AO_BASE + 0x1b80)
#define DDRPHY3AO_RFU_0X1BA0 (DDRPHY3AO_BASE + 0x1ba0)
#define DRAMC3_AO_DRAMC_PD_CTRL (DRAMC3_AO_BASE + 0x38)
#define DRAMC3_AO_CLKAR (DRAMC3_AO_BASE + 0x3c)
#endif
#define PERICFG_PERI_BIU_REG_DCM_CTRL (PERICFG_BASE + 0x210)
#define PERICFG_PERI_BIU_EMI_DCM_CTRL (PERICFG_BASE + 0x214)
#define PERICFG_PERI_BIU_DBC_CTRL (PERICFG_BASE + 0x218)
#define PERICFG_PERI_DCM_EMI_EARLY_CTRL (PERICFG_BASE + 0x220)
#define PERICFG_PERI_DCM_REG_EARLY_CTRL (PERICFG_BASE + 0x224)
#define INFRA_BUS_DCM_CTRL (INFRACFG_AO_BASE + 0x70)
#define INFRA_BUS_DCM_CTRL_1 (INFRACFG_AO_BASE + 0x74)
#define INFRA_MDBUS_DCM_CTRL (INFRACFG_AO_BASE + 0xd0)
#define INFRA_QAXIBUS_DCM_CTRL (INFRACFG_AO_BASE + 0xd4)
#define INFRA_EMI_DCM_CTRL (INFRACFG_AO_BASE + 0xdc)
#define INFRA_EMI_BUS_CTRL_1_STA (INFRACFG_AO_BASE + 0xF70) /* read for INFRA_EMI_DCM_CTRL */
#define TOPCKGEN_DCM_CFG (TOPCKGEN_BASE + 0x4)

/* INFRACFG_AO */
bool dcm_infracfg_ao_axi_is_on(void);
void dcm_infracfg_ao_axi(int on);
bool dcm_infracfg_ao_emi_is_on(void);
void dcm_infracfg_ao_emi(int on);
bool dcm_infracfg_ao_md_qaxi_is_on(void);
void dcm_infracfg_ao_md_qaxi(int on);
bool dcm_infracfg_ao_qaxi_is_on(void);
void dcm_infracfg_ao_qaxi(int on);
#if 0
/* PWRAP */
bool dcm_pwrap_pmic_wrap_is_on(void);
void dcm_pwrap_pmic_wrap(int on);
#endif
/* MCUCFG */
bool dcm_mcucfg_adb400_dcm_is_on(void);
void dcm_mcucfg_adb400_dcm(int on);
bool dcm_mcucfg_bus_arm_pll_divider_dcm_is_on(void);
void dcm_mcucfg_bus_arm_pll_divider_dcm(int on);
bool dcm_mcucfg_bus_sync_dcm_is_on(void);
void dcm_mcucfg_bus_sync_dcm(int on);
bool dcm_mcucfg_bus_clock_dcm_is_on(void);
void dcm_mcucfg_bus_clock_dcm(int on);
bool dcm_mcucfg_bus_fabric_dcm_is_on(void);
void dcm_mcucfg_bus_fabric_dcm(int on);
bool dcm_mcucfg_cntvalue_dcm_is_on(void);
void dcm_mcucfg_cntvalue_dcm(int on);
bool dcm_mcucfg_gic_sync_dcm_is_on(void);
void dcm_mcucfg_gic_sync_dcm(int on);
bool dcm_mcucfg_l2_shared_dcm_is_on(void);
void dcm_mcucfg_l2_shared_dcm(int on);
bool dcm_mcucfg_mp0_adb_dcm_is_on(void);
void dcm_mcucfg_mp0_adb_dcm(int on);
bool dcm_mcucfg_mp0_arm_pll_divider_dcm_is_on(void);
void dcm_mcucfg_mp0_arm_pll_divider_dcm(int on);
bool dcm_mcucfg_mp0_bus_dcm_is_on(void);
void dcm_mcucfg_mp0_bus_dcm(int on);
bool dcm_mcucfg_mp0_rgu_dcm_is_on(void);
void dcm_mcucfg_mp0_rgu_dcm(int on);
bool dcm_mcucfg_mp0_stall_dcm_is_on(void);
void dcm_mcucfg_mp0_stall_dcm(int on);
bool dcm_mcucfg_mp0_sync_dcm_cfg_is_on(void);
void dcm_mcucfg_mp0_sync_dcm_cfg(int on);
bool dcm_mcucfg_mp1_arm_pll_divider_dcm_is_on(void);
void dcm_mcucfg_mp1_arm_pll_divider_dcm(int on);
bool dcm_mcucfg_mp1_rgu_dcm_is_on(void);
void dcm_mcucfg_mp1_rgu_dcm(int on);
bool dcm_mcucfg_mp1_stall_dcm_is_on(void);
void dcm_mcucfg_mp1_stall_dcm(int on);
bool dcm_mcucfg_mp1_sync_dcm_enable_is_on(void);
void dcm_mcucfg_mp1_sync_dcm_enable(int on);
bool dcm_mcucfg_mp2_arm_pll_divider_dcm_is_on(void);
void dcm_mcucfg_mp2_arm_pll_divider_dcm(int on);
bool dcm_mcucfg_mp_stall_dcm_is_on(void);
void dcm_mcucfg_mp_stall_dcm(int on);
bool dcm_mcucfg_sync_dcm_cfg_is_on(void);
void dcm_mcucfg_sync_dcm_cfg(int on);
bool dcm_mcucfg_mcu_misc_dcm_is_on(void);
void dcm_mcucfg_mcu_misc_dcm(int on);
/* TOPCKGEN */
unsigned int topckgen_emi_dcm_full_fsel_get(void);
void topckgen_emi_dcm_full_fsel_set(unsigned int val);
void topckgen_emi_dcm_dbc_cnt_set(unsigned int val);
bool dcm_topckgen_cksys_dcm_emi_is_on(void);
void dcm_topckgen_cksys_dcm_emi(int on);
/* EMI */
bool dcm_emi_emi_dcm_reg_is_on(void);
void dcm_emi_emi_dcm_reg(int on);
/* CHN0_EMI */
bool dcm_chn0_emi_emi_dcm_reg_is_on(void);
void dcm_chn0_emi_emi_dcm_reg(int on);
/* CHN1_EMI */
bool dcm_chn1_emi_emi_dcm_reg_is_on(void);
void dcm_chn1_emi_emi_dcm_reg(int on);
/* CHN2_EMI */
bool dcm_chn2_emi_emi_dcm_reg_is_on(void);
void dcm_chn2_emi_emi_dcm_reg(int on);
/* CHN3_EMI */
bool dcm_chn3_emi_emi_dcm_reg_is_on(void);
void dcm_chn3_emi_emi_dcm_reg(int on);
/* LPDMA */
bool dcm_lpdma_lpdma_is_on(void);
void dcm_lpdma_lpdma(int on);
#ifndef USE_DRAM_API_INSTEAD
/* DDRPHY0AO */
bool dcm_ddrphy0ao_ddrphy_is_on(void);
void dcm_ddrphy0ao_ddrphy(int on);
bool dcm_ddrphy0ao_ddrphy_e2_is_on(void);
void dcm_ddrphy0ao_ddrphy_e2(int on);
/* DRAMC0_AO */
bool dcm_dramc0_ao_dramc_dcm_is_on(void);
void dcm_dramc0_ao_dramc_dcm(int on);
/* DDRPHY1AO */
bool dcm_ddrphy1ao_ddrphy_is_on(void);
void dcm_ddrphy1ao_ddrphy(int on);
bool dcm_ddrphy1ao_ddrphy_e2_is_on(void);
void dcm_ddrphy1ao_ddrphy_e2(int on);
/* DRAMC1_AO */
bool dcm_dramc1_ao_dramc_dcm_is_on(void);
void dcm_dramc1_ao_dramc_dcm(int on);
/* DDRPHY2AO */
bool dcm_ddrphy2ao_ddrphy_is_on(void);
void dcm_ddrphy2ao_ddrphy(int on);
bool dcm_ddrphy2ao_ddrphy_e2_is_on(void);
void dcm_ddrphy2ao_ddrphy_e2(int on);
/* DRAMC2_AO */
bool dcm_dramc2_ao_dramc_dcm_is_on(void);
void dcm_dramc2_ao_dramc_dcm(int on);
/* DDRPHY3AO */
bool dcm_ddrphy3ao_ddrphy_is_on(void);
void dcm_ddrphy3ao_ddrphy(int on);
bool dcm_ddrphy3ao_ddrphy_e2_is_on(void);
void dcm_ddrphy3ao_ddrphy_e2(int on);
/* DRAMC3_AO */
bool dcm_dramc3_ao_dramc_dcm_is_on(void);
void dcm_dramc3_ao_dramc_dcm(int on);
#endif
/* PERICFG */
bool dcm_pericfg_peri_dcm_is_on(void);
void dcm_pericfg_peri_dcm(int on);
bool dcm_pericfg_peri_dcm_emi_group_biu_is_on(void);
void dcm_pericfg_peri_dcm_emi_group_biu(int on);
bool dcm_pericfg_peri_dcm_emi_group_bus_is_on(void);
void dcm_pericfg_peri_dcm_emi_group_bus(int on);
bool dcm_pericfg_peri_dcm_reg_group_biu_is_on(void);
void dcm_pericfg_peri_dcm_reg_group_biu(int on);
bool dcm_pericfg_peri_dcm_reg_group_bus_is_on(void);
void dcm_pericfg_peri_dcm_reg_group_bus(int on);
/* MCSI_REG */
bool dcm_mcsi_reg_cci_cactive_is_on(void);
void dcm_mcsi_reg_cci_cactive(int on);
bool dcm_mcsi_reg_cci_dcm_is_on(void);
void dcm_mcsi_reg_cci_dcm(int on);

#endif

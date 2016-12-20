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

#ifndef __MTK_DCM_AUTOGEN_H__
#define __MTK_DCM_AUTOGEN_H__

#ifndef __KERNEL__
/* Base */
#define INFRACFG_AO_BASE 0x10001000
#define MP0_CPUCFG_BASE 0x10200000
#define MP1_CPUCFG_BASE 0x10200200
#define MCU_MISCCFG_BASE 0x10200400
#define EMI_BASE 0x10219000
#define DRAMC_CH0_TOP0_BASE 0x10228000
#define DRAMC_CH0_TOP1_BASE 0x1022a000
#define CHN0_EMI_BASE 0x1022d000
#define DRAMC_CH1_TOP0_BASE 0x10230000
#define DRAMC_CH1_TOP1_BASE 0x10232000
#define CHN1_EMI_BASE 0x10235000
#define CCI_BASE 0x10390000
#define CCI_PHYS_BASE CCI_BASE
#include <sys/types.h>
#else
#include "mtk_dcm.h"
#endif

/* Register Definition */
#define INFRA_TOPCKGEN_DCMCTL (INFRACFG_AO_BASE + 0x10)
#define INFRA_BUS_DCM_CTRL (INFRACFG_AO_BASE + 0x70)
#define PERI_BUS_DCM_CTRL (INFRACFG_AO_BASE + 0x74)
#define MEM_DCM_CTRL (INFRACFG_AO_BASE + 0x78)
#define DFS_MEM_DCM_CTRL (INFRACFG_AO_BASE + 0x7c)
#define P2P_RX_CLK_ON (INFRACFG_AO_BASE + 0xa0)
#define INFRA_MISC (INFRACFG_AO_BASE + 0xf00)
#define MP0_DBG_PWR_CTRL (MP0_CPUCFG_BASE + 0x68)
#define MP1_DBG_PWR_CTRL (MP1_CPUCFG_BASE + 0x68)
#define L2C_SRAM_CTRL (MCU_MISCCFG_BASE + 0x248)
#define CCI_CLK_CTRL (MCU_MISCCFG_BASE + 0x260)
#define BUS_FABRIC_DCM_CTRL (MCU_MISCCFG_BASE + 0x268)
#define CCI_ADB400_DCM_CONFIG (MCU_MISCCFG_BASE + 0x340)
#define SYNC_DCM_CONFIG (MCU_MISCCFG_BASE + 0x344)
#define SYNC_DCM_CLUSTER_CONFIG (MCU_MISCCFG_BASE + 0x34c)
#define GIC_SYNC_DCM_CFG (MCU_MISCCFG_BASE + 0x358)
#define BIG_DBG_PWR_CTRL (MCU_MISCCFG_BASE + 0x35c)
#define MP0_LAST_CORE_DCM (MCU_MISCCFG_BASE + 0x3A0)
#define MP1_LAST_CORE_DCM (MCU_MISCCFG_BASE + 0x3A4)
#define MP0_RGU_DCM_CONFIG (MCU_MISCCFG_BASE + 0x1870)
#define MP1_RGU_DCM_CONFIG (MCU_MISCCFG_BASE + 0x3870)
#define EMI_CONM (EMI_BASE + 0x60)
#define EMI_CONN (EMI_BASE + 0x68)
#define DRAMC_CH0_TOP0_MISC_CG_CTRL0 (DRAMC_CH0_TOP0_BASE + 0x284)
#define DRAMC_CH0_TOP0_MISC_CG_CTRL2 (DRAMC_CH0_TOP0_BASE + 0x28c)
#define DRAMC_CH0_TOP1_DRAMC_PD_CTRL (DRAMC_CH0_TOP1_BASE + 0x38)
#define DRAMC_CH0_TOP1_CLKAR (DRAMC_CH0_TOP1_BASE + 0x3c)
#define CHN0_EMI_CHN_EMI_CONB (CHN0_EMI_BASE + 0x8)
#define DRAMC_CH1_TOP0_MISC_CG_CTRL0 (DRAMC_CH1_TOP0_BASE + 0x284)
#define DRAMC_CH1_TOP0_MISC_CG_CTRL2 (DRAMC_CH1_TOP0_BASE + 0x28c)
#define DRAMC_CH1_TOP1_DRAMC_PD_CTRL (DRAMC_CH1_TOP1_BASE + 0x38)
#define DRAMC_CH1_TOP1_CLKAR (DRAMC_CH1_TOP1_BASE + 0x3c)
#define CHN1_EMI_CHN_EMI_CONB (CHN1_EMI_BASE + 0x8)
/* #define MCSI_A_DCM (MP0_CPUCFG_BASE + 0x190000) */
#define MCSI_A_DCM (CCI_BASE + 0x0)
#define MCSI_A_DCM_PHYS (CCI_PHYS_BASE + 0x0)

/* INFRACFG_AO */
bool dcm_infracfg_ao_dcm_infra_bus_is_on(int on);
void dcm_infracfg_ao_dcm_infra_bus(int on);
bool dcm_infracfg_ao_dcm_peri_bus_is_on(int on);
void dcm_infracfg_ao_dcm_peri_bus(int on);
bool dcm_infracfg_ao_infra_md_fmem_is_on(int on);
void dcm_infracfg_ao_infra_md_fmem(int on);
bool dcm_infracfg_ao_p2p_rxclk_ctrl_is_on(int on);
void dcm_infracfg_ao_p2p_rxclk_ctrl(int on);
bool dcm_infracfg_ao_mcu_armpll_bus_is_on(int on);
void dcm_infracfg_ao_mcu_armpll_bus_mode1(int on);
bool dcm_infracfg_ao_mcu_armpll_ca7l_is_on(int on);
void dcm_infracfg_ao_mcu_armpll_ca7l_mode1(int on);
bool dcm_infracfg_ao_mcu_armpll_ca7ll_is_on(int on);
void dcm_infracfg_ao_mcu_armpll_ca7ll_mode1(int on);
void dcm_infracfg_ao_mcu_armpll_bus_mode2(int on);
void dcm_infracfg_ao_mcu_armpll_ca7l_mode2(int on);
void dcm_infracfg_ao_mcu_armpll_ca7ll_mode2(int on);
/* MCSI_A */
bool dcm_mcsi_a_is_on(int on);
void dcm_mcsi_a(int on);
/* MP0_CPUCFG */
bool dcm_mp0_cpucfg_dcm_mcu_bus_is_on(int on);
void dcm_mp0_cpucfg_dcm_mcu_bus(int on);
/* MP1_CPUCFG */
bool dcm_mp1_cpucfg_dcm_mcu_bus_is_on(int on);
void dcm_mp1_cpucfg_dcm_mcu_bus(int on);
/* MCU_MISCCFG */
bool dcm_mcu_misccfg_cci_sync_dcm_is_on(int on);
void dcm_mcu_misccfg_cci_sync_dcm(int on);
bool dcm_mcu_misccfg_mp0_last_core_dcm_is_on(int on);
void dcm_mcu_misccfg_mp0_last_core_dcm(int on);
bool dcm_mcu_misccfg_mp0_sync_dcm_is_on(int on);
void dcm_mcu_misccfg_mp0_sync_dcm(int on);
bool dcm_mcu_misccfg_mp0_stall_dcm_is_on(int on);
void dcm_mcu_misccfg_mp0_stall_dcm(int on);
bool dcm_mcu_misccfg_mp1_last_core_dcm_is_on(int on);
void dcm_mcu_misccfg_mp1_last_core_dcm(int on);
bool dcm_mcu_misccfg_mp1_sync_dcm_is_on(int on);
void dcm_mcu_misccfg_mp1_sync_dcm(int on);
bool dcm_mcu_misccfg_mp1_stall_dcm_is_on(int on);
void dcm_mcu_misccfg_mp1_stall_dcm(int on);
bool dcm_mcu_misccfg_stall_dcm_enhance_is_on(int on);
void dcm_mcu_misccfg_stall_dcm_enhance(int on);
bool dcm_mcu_misccfg_dcm_mcu_bus_is_on(int on);
void dcm_mcu_misccfg_dcm_mcu_bus(int on);
bool dcm_mcu_misccfg_gic_sync_dcm_is_on(int on);
void dcm_mcu_misccfg_gic_sync_dcm(int on);
bool dcm_mcu_misccfg_mp0_rgu_dcm_is_on(int on);
void dcm_mcu_misccfg_mp0_rgu_dcm(int on);
bool dcm_mcu_misccfg_mp1_rgu_dcm_is_on(int on);
void dcm_mcu_misccfg_mp1_rgu_dcm(int on);
/* EMI */
bool dcm_emi_dcm_emi_group_is_on(int on);
void dcm_emi_dcm_emi_group(int on);
/* DRAMC_CH0_TOP0 */
bool dcm_dramc_ch0_top0_ddrphy_is_on(int on);
void dcm_dramc_ch0_top0_ddrphy(int on);
/* DRAMC_CH0_TOP1 */
bool dcm_dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl_is_on(int on);
void dcm_dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl(int on);
/* CHN0_EMI */
bool dcm_chn0_emi_dcm_emi_group_is_on(int on);
void dcm_chn0_emi_dcm_emi_group(int on);
/* DRAMC_CH1_TOP0 */
bool dcm_dramc_ch1_top0_ddrphy_is_on(int on);
void dcm_dramc_ch1_top0_ddrphy(int on);
/* DRAMC_CH1_TOP1 */
bool dcm_dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl_is_on(int on);
void dcm_dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl(int on);
/* CHN1_EMI */
bool dcm_chn1_emi_dcm_emi_group_is_on(int on);
void dcm_chn1_emi_dcm_emi_group(int on);

#endif

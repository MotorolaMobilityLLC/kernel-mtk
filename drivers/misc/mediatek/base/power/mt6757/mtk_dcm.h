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

#ifndef __MTK_DCM_H__
#define __MTK_DCM_H__

extern unsigned long dcm_topckgen_base;
extern unsigned long dcm_mcucfg_base;
extern unsigned long dcm_mcucfg_phys_base;
extern unsigned long dcm_dramc_ch0_top0_base;
extern unsigned long dcm_dramc_ch0_top1_base;
extern unsigned long dcm_dramc_ch0_top3_base;
extern unsigned long dcm_dramc_ch1_top0_base;
extern unsigned long dcm_dramc_ch1_top1_base;
extern unsigned long dcm_dramc_ch1_top3_base;
extern unsigned long dcm_emi_base;
extern unsigned long dcm_infracfg_ao_base;
extern unsigned long dcm_cci_base;
extern unsigned long dcm_cci_phys_base;

#define INFRACFG_AO_BASE (dcm_infracfg_ao_base)
#define MCUCFG_BASE (dcm_mcucfg_base)
#define MP0_CPUCFG_BASE (MCUCFG_BASE + 0x0)
#define MP1_CPUCFG_BASE (MCUCFG_BASE + 0x200)
#define MCU_MISCCFG_BASE (MCUCFG_BASE + 0x400)

#define TOPCKGEN_BASE (dcm_topckgen_base)
#define DRAMC_CH0_TOP0_BASE (dcm_dramc_ch0_top0_base)
#define DRAMC_CH0_TOP1_BASE (dcm_dramc_ch0_top1_base)
#define CHN0_EMI_BASE (dcm_dramc_ch0_top3_base)
#define DRAMC_CH1_TOP0_BASE (dcm_dramc_ch1_top0_base)
#define DRAMC_CH1_TOP1_BASE (dcm_dramc_ch1_top1_base)
#define CHN1_EMI_BASE (dcm_dramc_ch1_top3_base)
#define EMI_BASE (dcm_emi_base)
#define CCI_BASE (dcm_cci_base)
#define CCI_PHYS_BASE (dcm_cci_phys_base)

void mtk_dcm_disable(void);
void mtk_dcm_restore(void);

int sync_dcm_set_cci_freq(unsigned int cci);
int sync_dcm_set_mp0_freq(unsigned int mp0);
int sync_dcm_set_mp1_freq(unsigned int mp1);

#define DCM_OFF (0)
#define DCM_ON (1)

#define ENABLE_DCM_IN_LK

/*****************************************************/
int dcm_armcore(int);
int dcm_infra(int);
int dcm_peri(int);
int dcm_mcusys(int);
int dcm_dramc_ao(int);
int dcm_emi(int);
int dcm_ddrphy(int);
int dcm_stall_preset(void);
int dcm_stall(int);
int dcm_gic_sync(int);
int dcm_last_core(int);
int dcm_rgu(int);

/*****************************************************/
enum { ARMCORE_DCM = 0,
	   MCUSYS_DCM,
	   INFRA_DCM,
	   PERI_DCM,
	   EMI_DCM,
	   DRAMC_DCM,
	   DDRPHY_DCM,
	   STALL_DCM,
	   GIC_SYNC_DCM,
	   LAST_CORE_DCM,
	   RGU_DCM,
	   NR_DCM = 11,
};

enum { ARMCORE_DCM_TYPE = (1U << 0),
	   MCUSYS_DCM_TYPE = (1U << 1),
	   INFRA_DCM_TYPE = (1U << 2),
	   PERI_DCM_TYPE = (1U << 3),
	   EMI_DCM_TYPE = (1U << 4),
	   DRAMC_DCM_TYPE = (1U << 5),
	   DDRPHY_DCM_TYPE = (1U << 6),
	   STALL_DCM_TYPE = (1U << 7),
	   GIC_SYNC_DCM_TYPE = (1U << 8),
	   LAST_CORE_DCM_TYPE = (1U << 9),
	   RGU_DCM_TYPE = (1U << 10),
	   NR_DCM_TYPE = 11,
};

/* #define ALL_DCM_TYPE	 (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE |
 * \
 */
/*	PERI_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE |
 * DDRPHY_DCM_TYPE | \
 */
/*	STALL_DCM_TYPE | GIC_SYNC_DCM_TYPE | LAST_CORE_DCM_TYPE |
 * RGU_DCM_TYPE)
 */

/* #define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE |
 * \
 */
/*			   PERI_DCM_TYPE | STALL_DCM_TYPE) */

#ifdef ENABLE_DCM_IN_LK
#define INIT_DCM_TYPE_BY_K 0
#endif

#endif /* #ifndef __MTK_DCM_H__ */

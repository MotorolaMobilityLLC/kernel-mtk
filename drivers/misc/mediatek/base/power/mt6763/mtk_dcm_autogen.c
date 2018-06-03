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

#ifndef __KERNEL__
#include "cmessage.h"
#include "API.h"
#else
#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_secure_api.h>
#endif

#include "mtk_dcm_autogen.h"

#ifndef __KERNEL__

/*
*unsigned int reg_read(UINT32P reg_addr)
*{
*	return *((UINT32P)(reg_addr));
*}
*void reg_write(UINT32P reg_addr, unsigned int wdata)
*{
*	*((UINT32P)(reg_addr)) = wdata;
*}
*/

#define IOMEM(a) (a)
#define __raw_readl(a)  (*(volatile unsigned long*)(a))
/* #define dsb() do { __asm__ __volatile__ ("dsb\n\t"); } while(0) */
#define mt_reg_sync_writel(val, addr)  ({ *(unsigned long *)(addr) = val; dsb(); })

#define reg_read(addr)	__raw_readl(IOMEM(addr))
#define reg_write(addr, val)	mt_reg_sync_writel((val), ((void *)addr))

#else
#define reg_read(addr)	__raw_readl(IOMEM(addr))
#define reg_write(addr, val)	mt_reg_sync_writel((val), ((void *)addr))
#endif

#if defined(__KERNEL__) /* for KERNEL */
#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#define MCSI_A_SMC_WRITE(addr, val)  mcsi_a_smc_write_phy(addr##_PHYS, val)
#define MCSI_A_SMC_READ(addr)  mcsi_a_smc_read_phy(addr##_PHYS)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#define MCSI_A_SMC_WRITE(addr, val)  reg_write(addr, val)
#define MCSI_A_SMC_READ(addr)  reg_read(addr)
#endif
#else /* !defined(__KERNEL__), for CTP */
#define MCUSYS_SMC_WRITE(addr, val)  reg_write(addr, val)
#define MCSI_A_SMC_WRITE(addr, val)  reg_write(addr, val)
#define MCSI_A_SMC_READ(addr)  reg_read(addr)
#endif /* defined(__KERNEL__) */

#define INFRACFG_AO_DCM_INFRA_BUS_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22))
#define INFRACFG_AO_DCM_INFRA_BUS_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22))
#define INFRACFG_AO_DCM_INFRA_BUS_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22))
#if 0
static unsigned int infracfg_ao_infra_dcm_rg_fsel_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 5) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_infra_dcm_rg_sfsel_get(void)
{
	return (reg_read(INFRA_BUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
static void infracfg_ao_infra_dcm_rg_fsel_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 5)) |
		(val & 0x1f) << 5);
}
static void infracfg_ao_infra_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(INFRA_BUS_DCM_CTRL,
		(reg_read(INFRA_BUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}

bool dcm_infracfg_ao_dcm_infra_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(INFRA_BUS_DCM_CTRL) &
		~INFRACFG_AO_DCM_INFRA_BUS_REG0_MASK &
		INFRACFG_AO_DCM_INFRA_BUS_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_dcm_infra_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_dcm_infra_bus'" */
		infracfg_ao_infra_dcm_rg_fsel_set(0x10);
		infracfg_ao_infra_dcm_rg_sfsel_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL,
			(reg_read(INFRA_BUS_DCM_CTRL) &
			~INFRACFG_AO_DCM_INFRA_BUS_REG0_MASK) |
			INFRACFG_AO_DCM_INFRA_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_dcm_infra_bus'" */
		infracfg_ao_infra_dcm_rg_fsel_set(0x10);
		infracfg_ao_infra_dcm_rg_sfsel_set(0x1);
		reg_write(INFRA_BUS_DCM_CTRL,
			(reg_read(INFRA_BUS_DCM_CTRL) &
			~INFRACFG_AO_DCM_INFRA_BUS_REG0_MASK) |
			INFRACFG_AO_DCM_INFRA_BUS_REG0_OFF);
	}
}

#define INFRACFG_AO_DCM_PERI_BUS_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1f << 15) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			/* (0x1 << 22) | remove PMIC HWDCM setting due to PMIC Wrapper issue */ \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 31))
#define INFRACFG_AO_DCM_PERI_BUS_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x1f << 15) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			/* (0x1 << 22) | remove PMIC HWDCM setting due to PMIC Wrapper issue */ \
			(0x1 << 28) | \
			(0x1 << 29) | \
			(0x1 << 31))
#define INFRACFG_AO_DCM_PERI_BUS_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 15) | \
			(0x0 << 20) | \
			(0x0 << 21) | \
			/* (0x0 << 22) | remove PMIC HWDCM setting due to PMIC Wrapper issue */ \
			(0x0 << 28) | \
			(0x0 << 29) | \
			(0x0 << 31))
#if 0
static unsigned int infracfg_ao_peri_dcm_rg_fsel_get(void)
{
	return (reg_read(PERI_BUS_DCM_CTRL) >> 5) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_peri_dcm_rg_sfsel_get(void)
{
	return (reg_read(PERI_BUS_DCM_CTRL) >> 10) & 0x1f;
}
#endif
#if 0
static unsigned int infracfg_ao_pmic_cnt_mst_rg_sfsel_get(void)
{
	return (reg_read(PERI_BUS_DCM_CTRL) >> 23) & 0x1f;
}
#endif
static void infracfg_ao_peri_dcm_rg_fsel_set(unsigned int val)
{
	reg_write(PERI_BUS_DCM_CTRL,
		(reg_read(PERI_BUS_DCM_CTRL) &
		~(0x1f << 5)) |
		(val & 0x1f) << 5);
}
static void infracfg_ao_peri_dcm_rg_sfsel_set(unsigned int val)
{
	reg_write(PERI_BUS_DCM_CTRL,
		(reg_read(PERI_BUS_DCM_CTRL) &
		~(0x1f << 10)) |
		(val & 0x1f) << 10);
}
static void infracfg_ao_pmic_cnt_mst_rg_sfsel_set(unsigned int val)
{
	reg_write(PERI_BUS_DCM_CTRL,
		(reg_read(PERI_BUS_DCM_CTRL) &
		~(0x1f << 23)) |
		(val & 0x1f) << 23);
}

bool dcm_infracfg_ao_dcm_peri_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(PERI_BUS_DCM_CTRL) &
		~INFRACFG_AO_DCM_PERI_BUS_REG0_MASK &
		INFRACFG_AO_DCM_PERI_BUS_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_dcm_peri_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_dcm_peri_bus'" */
		infracfg_ao_peri_dcm_rg_fsel_set(0x1f);
		infracfg_ao_peri_dcm_rg_sfsel_set(0x0);
		infracfg_ao_pmic_cnt_mst_rg_sfsel_set(0x0);
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_DCM_PERI_BUS_REG0_MASK) |
			INFRACFG_AO_DCM_PERI_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_dcm_peri_bus'" */
		infracfg_ao_peri_dcm_rg_fsel_set(0x1f);
		infracfg_ao_peri_dcm_rg_sfsel_set(0x1f);
		infracfg_ao_pmic_cnt_mst_rg_sfsel_set(0x0);
		reg_write(PERI_BUS_DCM_CTRL,
			(reg_read(PERI_BUS_DCM_CTRL) &
			~INFRACFG_AO_DCM_PERI_BUS_REG0_MASK) |
			INFRACFG_AO_DCM_PERI_BUS_REG0_OFF);
	}
}

#define INFRACFG_AO_INFRA_MD_FMEM_REG0_MASK ((0x1 << 28))
#define INFRACFG_AO_INFRA_MD_FMEM_REG0_ON ((0x1 << 28))
#define INFRACFG_AO_INFRA_MD_FMEM_REG0_OFF ((0x0 << 28))

bool dcm_infracfg_ao_infra_md_fmem_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(INFRA_MISC) &
		~INFRACFG_AO_INFRA_MD_FMEM_REG0_MASK &
		INFRACFG_AO_INFRA_MD_FMEM_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_infra_md_fmem(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_infra_md_fmem'" */
		reg_write(INFRA_MISC,
			(reg_read(INFRA_MISC) &
			~INFRACFG_AO_INFRA_MD_FMEM_REG0_MASK) |
			INFRACFG_AO_INFRA_MD_FMEM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_infra_md_fmem'" */
		reg_write(INFRA_MISC,
			(reg_read(INFRA_MISC) &
			~INFRACFG_AO_INFRA_MD_FMEM_REG0_MASK) |
			INFRACFG_AO_INFRA_MD_FMEM_REG0_OFF);
	}
}

#define INFRACFG_AO_P2P_RXCLK_CTRL_REG0_MASK ((0xf << 0))
#define INFRACFG_AO_P2P_RXCLK_CTRL_REG0_ON ((0x0 << 0))
#define INFRACFG_AO_P2P_RXCLK_CTRL_REG0_OFF ((0x0 << 0))

bool dcm_infracfg_ao_p2p_rxclk_ctrl_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(P2P_RX_CLK_ON) &
		~INFRACFG_AO_P2P_RXCLK_CTRL_REG0_MASK &
		INFRACFG_AO_P2P_RXCLK_CTRL_REG0_ON);

	return ret;
}

void dcm_infracfg_ao_p2p_rxclk_ctrl(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_p2p_rxclk_ctrl'" */
		reg_write(P2P_RX_CLK_ON,
			(reg_read(P2P_RX_CLK_ON) &
			~INFRACFG_AO_P2P_RXCLK_CTRL_REG0_MASK) |
			INFRACFG_AO_P2P_RXCLK_CTRL_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_p2p_rxclk_ctrl'" */
		reg_write(P2P_RX_CLK_ON,
			(reg_read(P2P_RX_CLK_ON) &
			~INFRACFG_AO_P2P_RXCLK_CTRL_REG0_MASK) |
			INFRACFG_AO_P2P_RXCLK_CTRL_REG0_OFF);
	}
}

/*******************************************************************************************
 * Software workaround for INFRA_TOPCKGEN_DCMCTL register bit 18 17 16 shifting
 * Root cause : HW RTL register read value 18 17 16 bit wrong match to 17 16 15
 * This workaround is for left shift 1 for bit (15, 16, 17) after read register(0x10001010)
 *******************************************************************************************/
 #if 0
#define INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND_MASK ((0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17))

#define INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg) ((reg & ~INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND_MASK) | \
			(reg & INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND_MASK) << 1)
#endif
/******************************************************************************/

#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_MASK ((0x1 << 3))
#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_ON ((0x1 << 3))
#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_OFF ((0x0 << 3))

bool dcm_infracfg_ao_mcu_armpll_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(INFRA_TOPCKGEN_DCMCTL) &
		~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_MASK &
		INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_ON);

	return ret;
}

void dcm_infracfg_ao_mcu_armpll_bus_mode1(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_bus'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_bus'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE1_OFF);
	}
}

#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_MASK ((0x1 << 3) | \
			(0x1 << 14) | \
			(0x1 << 18))
#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_ON ((0x1 << 3) | \
			(0x1 << 14) | \
			(0x1 << 18))
#define INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_OFF ((0x0 << 3) | \
			(0x0 << 14) | \
			(0x0 << 18))

void dcm_infracfg_ao_mcu_armpll_bus_mode2(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_bus'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_ON);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_ON);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_bus'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_OFF);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_OFF);
#endif
	}
}

#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_MASK ((0x1 << 1))
#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_ON ((0x1 << 1))
#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_OFF ((0x0 << 1))

bool dcm_infracfg_ao_mcu_armpll_ca7l_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(INFRA_TOPCKGEN_DCMCTL) &
		~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_MASK &
		INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_ON);

	return ret;
}

void dcm_infracfg_ao_mcu_armpll_ca7l_mode1(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_ca7l'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_ca7l'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE1_OFF);
	}
}

#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_MASK ((0x1 << 1) | \
			(0x1 << 12) | \
			(0x1 << 16))
#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_ON ((0x1 << 1) | \
			(0x1 << 12) | \
			(0x1 << 16))
#define INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_OFF ((0x0 << 1) | \
			(0x0 << 12) | \
			(0x0 << 16))

void dcm_infracfg_ao_mcu_armpll_ca7l_mode2(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_ca7l'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_BUS_REG0_MODE2_ON);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_ON);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_ca7l'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_OFF);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7L_REG0_MODE2_OFF);
#endif
	}
}

#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_MASK ((0x1 << 2))
#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_ON ((0x1 << 2))
#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_OFF ((0x0 << 2))

bool dcm_infracfg_ao_mcu_armpll_ca7ll_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(INFRA_TOPCKGEN_DCMCTL) &
		~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_MASK &
		INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_ON);

	return ret;
}

void dcm_infracfg_ao_mcu_armpll_ca7ll_mode1(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_ca7ll'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_ca7ll'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE1_OFF);
	}
}

#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_MASK ((0x1 << 2) | \
			(0x1 << 13) | \
			(0x1 << 17))
#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_ON ((0x1 << 2) | \
			(0x1 << 13) | \
			(0x1 << 17))
#define INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_OFF ((0x0 << 2) | \
			(0x0 << 13) | \
			(0x0 << 17))

void dcm_infracfg_ao_mcu_armpll_ca7ll_mode2(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'infracfg_ao_mcu_armpll_ca7ll'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_ON);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_ON);
#endif
	} else {
		/* TINFO = "Turn OFF DCM 'infracfg_ao_mcu_armpll_ca7ll'" */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(reg_read(INFRA_TOPCKGEN_DCMCTL) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_OFF);
#if 0
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			(INFRA_TOPCKGEN_DCMCTL_SW_WORKAROUND(reg_read(INFRA_TOPCKGEN_DCMCTL)) &
			~INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_MASK) |
			INFRACFG_AO_MCU_ARMPLL_CA7LL_REG0_MODE2_OFF);
#endif
	}
}

#define MCSI_A_REG0_MASK ((0xffff << 16))
#define MCSI_A_REG0_ON ((0xffff << 16))
#define MCSI_A_REG0_OFF ((0x0 << 16))

bool dcm_mcsi_a_is_on(int on)
{
	bool ret = false;

	ret &= !!(MCSI_A_SMC_READ(MCSI_A_DCM) &
		~MCSI_A_REG0_MASK &
		MCSI_A_REG0_ON);

	return ret;
}

void dcm_mcsi_a(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dcm_mcsi_a'" */
		MCSI_A_SMC_WRITE(MCSI_A_DCM,
			(MCSI_A_SMC_READ(MCSI_A_DCM) &
			~MCSI_A_REG0_MASK) |
			MCSI_A_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dcm_mcsi_a'" */
		MCSI_A_SMC_WRITE(MCSI_A_DCM,
			(MCSI_A_SMC_READ(MCSI_A_DCM) &
			~MCSI_A_REG0_MASK) |
			MCSI_A_REG0_OFF);
	}
}

#define MP0_CPUCFG_DCM_MCU_BUS_REG0_MASK ((0xf << 0))
#define MP0_CPUCFG_DCM_MCU_BUS_REG0_ON ((0x0 << 0))
#define MP0_CPUCFG_DCM_MCU_BUS_REG0_OFF ((0xf << 0))

bool dcm_mp0_cpucfg_dcm_mcu_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP0_DBG_PWR_CTRL) &
		~MP0_CPUCFG_DCM_MCU_BUS_REG0_MASK &
		MP0_CPUCFG_DCM_MCU_BUS_REG0_ON);

	return ret;
}

void dcm_mp0_cpucfg_dcm_mcu_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mp0_cpucfg_dcm_mcu_bus'" */
		reg_write(MP0_DBG_PWR_CTRL,
			(reg_read(MP0_DBG_PWR_CTRL) &
			~MP0_CPUCFG_DCM_MCU_BUS_REG0_MASK) |
			MP0_CPUCFG_DCM_MCU_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mp0_cpucfg_dcm_mcu_bus'" */
		reg_write(MP0_DBG_PWR_CTRL,
			(reg_read(MP0_DBG_PWR_CTRL) &
			~MP0_CPUCFG_DCM_MCU_BUS_REG0_MASK) |
			MP0_CPUCFG_DCM_MCU_BUS_REG0_OFF);
	}
}

#define MP1_CPUCFG_DCM_MCU_BUS_REG0_MASK ((0xf << 0))
#define MP1_CPUCFG_DCM_MCU_BUS_REG0_ON ((0x0 << 0))
#define MP1_CPUCFG_DCM_MCU_BUS_REG0_OFF ((0xf << 0))

bool dcm_mp1_cpucfg_dcm_mcu_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP1_DBG_PWR_CTRL) &
		~MP1_CPUCFG_DCM_MCU_BUS_REG0_MASK &
		MP1_CPUCFG_DCM_MCU_BUS_REG0_ON);

	return ret;
}

void dcm_mp1_cpucfg_dcm_mcu_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mp1_cpucfg_dcm_mcu_bus'" */
		reg_write(MP1_DBG_PWR_CTRL,
			(reg_read(MP1_DBG_PWR_CTRL) &
			~MP1_CPUCFG_DCM_MCU_BUS_REG0_MASK) |
			MP1_CPUCFG_DCM_MCU_BUS_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mp1_cpucfg_dcm_mcu_bus'" */
		reg_write(MP1_DBG_PWR_CTRL,
			(reg_read(MP1_DBG_PWR_CTRL) &
			~MP1_CPUCFG_DCM_MCU_BUS_REG0_MASK) |
			MP1_CPUCFG_DCM_MCU_BUS_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_MASK ((0x1 << 31))
#define MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_ON ((0x1 << 31))
#define MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_OFF ((0x0 << 31))

bool dcm_mcu_misccfg_mp0_last_core_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP0_LAST_CORE_DCM) &
		~MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_MASK &
		MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_last_core_dcm(int on)
{
	if (on) {
		/* TINFO = "Enable last core idle mp0 armpll divider" */
		reg_write(MP0_LAST_CORE_DCM,
			(reg_read(MP0_LAST_CORE_DCM) &
			~MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_ON);
	} else {
		/* TINFO = "Disable last core idle mp0 armpll divider" */
		reg_write(MP0_LAST_CORE_DCM,
			(reg_read(MP0_LAST_CORE_DCM) &
			~MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_LAST_CORE_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_MASK ((0x1 << 31))
#define MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_ON ((0x1 << 31))
#define MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_OFF ((0x0 << 31))

bool dcm_mcu_misccfg_mp1_last_core_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP1_LAST_CORE_DCM) &
		~MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_MASK &
		MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_last_core_dcm(int on)
{
	if (on) {
		/* TINFO = "Enable last core idle mp1 armpll divider" */
		reg_write(MP1_LAST_CORE_DCM,
			(reg_read(MP1_LAST_CORE_DCM) &
			~MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_ON);
	} else {
		/* TINFO = "Disable last core idle mp1 armpll divider" */
		reg_write(MP1_LAST_CORE_DCM,
			(reg_read(MP1_LAST_CORE_DCM) &
			~MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_LAST_CORE_DCM_REG0_OFF);
	}
}
#define MCU_MISCCFG_CCI_SYNC_DCM_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_CCI_SYNC_DCM_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_CCI_SYNC_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcu_misccfg_cci_sync_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CONFIG) &
		~MCU_MISCCFG_CCI_SYNC_DCM_REG0_MASK &
		MCU_MISCCFG_CCI_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_cci_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_cci_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_CCI_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_CCI_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_cci_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_CCI_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_CCI_SYNC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_SYNC_DCM_REG0_MASK ((0x1 << 8))
#define MCU_MISCCFG_MP0_SYNC_DCM_REG0_ON ((0x1 << 8))
#define MCU_MISCCFG_MP0_SYNC_DCM_REG0_OFF ((0x0 << 8))

bool dcm_mcu_misccfg_mp0_sync_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CONFIG) &
		~MCU_MISCCFG_MP0_SYNC_DCM_REG0_MASK &
		MCU_MISCCFG_MP0_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp0_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp0_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_SYNC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK ((0x1 << 7))
#define MCU_MISCCFG_MP0_STALL_DCM_REG0_ON ((0x1 << 7))
#define MCU_MISCCFG_MP0_STALL_DCM_REG0_OFF ((0x0 << 7))

bool dcm_mcu_misccfg_mp0_stall_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		~MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK &
		MCU_MISCCFG_MP0_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp0_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp0_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP0_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP0_STALL_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_SYNC_DCM_REG0_MASK ((0x1 << 16))
#define MCU_MISCCFG_MP1_SYNC_DCM_REG0_ON ((0x1 << 16))
#define MCU_MISCCFG_MP1_SYNC_DCM_REG0_OFF ((0x0 << 16))

bool dcm_mcu_misccfg_mp1_sync_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CONFIG) &
		~MCU_MISCCFG_MP1_SYNC_DCM_REG0_MASK &
		MCU_MISCCFG_MP1_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp1_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp1_sync_dcm'" */
		reg_write(SYNC_DCM_CONFIG,
			(reg_read(SYNC_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_SYNC_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_SYNC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP0_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_MP0_RGU_DCM_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_MP0_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcu_misccfg_mp0_rgu_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP0_RGU_DCM_CONFIG) &
			~MCU_MISCCFG_MP0_RGU_DCM_REG0_MASK &
			MCU_MISCCFG_MP0_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp0_rgu_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON MP0 DCM RGU 'rg_cpusys_rgu_dcm_config'" */
		reg_write(MP0_RGU_DCM_CONFIG,
				(reg_read(MP0_RGU_DCM_CONFIG) &
				~MCU_MISCCFG_MP0_RGU_DCM_REG0_MASK) |
				MCU_MISCCFG_MP0_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF MP0 DCM RGU 'rg_cpusys_rgu_dcm_config'" */
		reg_write(MP0_RGU_DCM_CONFIG,
				(reg_read(MP0_RGU_DCM_CONFIG) &
				~MCU_MISCCFG_MP0_RGU_DCM_REG0_MASK) |
				MCU_MISCCFG_MP0_RGU_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_RGU_DCM_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_MP1_RGU_DCM_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_MP1_RGU_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcu_misccfg_mp1_rgu_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(MP1_RGU_DCM_CONFIG) &
			~MCU_MISCCFG_MP1_RGU_DCM_REG0_MASK &
			MCU_MISCCFG_MP1_RGU_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_rgu_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON MP1 DCM RGU 'rg_cpusys_rgu_dcm_config'" */
		reg_write(MP1_RGU_DCM_CONFIG,
				(reg_read(MP1_RGU_DCM_CONFIG) &
				~MCU_MISCCFG_MP1_RGU_DCM_REG0_MASK) |
				MCU_MISCCFG_MP1_RGU_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF MP1 DCM RGU 'rg_cpusys_rgu_dcm_config'" */
		reg_write(MP1_RGU_DCM_CONFIG,
				(reg_read(MP1_RGU_DCM_CONFIG) &
				~MCU_MISCCFG_MP1_RGU_DCM_REG0_MASK) |
				MCU_MISCCFG_MP1_RGU_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK ((0x1 << 15))
#define MCU_MISCCFG_MP1_STALL_DCM_REG0_ON ((0x1 << 15))
#define MCU_MISCCFG_MP1_STALL_DCM_REG0_OFF ((0x0 << 15))

bool dcm_mcu_misccfg_mp1_stall_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		~MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK &
		MCU_MISCCFG_MP1_STALL_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_mp1_stall_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_mp1_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_STALL_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_mp1_stall_dcm'" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_MP1_STALL_DCM_REG0_MASK) |
			MCU_MISCCFG_MP1_STALL_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_MASK ((0x79F << 17))
#define MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_ON ((0x31F << 17))
#define MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_OFF ((0x0 << 17))

bool dcm_mcu_misccfg_stall_dcm_enhance_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
		~MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_MASK &
		MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_stall_dcm_enhance(int on)
{
	if (on) {
		/* TINFO = "Turn ON STALL DCM ENHANCE" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_MASK) |
			MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_ON);
	} else {
		/* TINFO = "Turn OFF STALL DCM ENHANCE" */
		reg_write(SYNC_DCM_CLUSTER_CONFIG,
			(reg_read(SYNC_DCM_CLUSTER_CONFIG) &
			~MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_MASK) |
			MCU_MISCCFG_STALL_DCM_ENHANCE_REG0_OFF);
	}
}

#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK ((0x3 << 0))
#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON ((0x3 << 0))
#define MCU_MISCCFG_GIC_SYNC_DCM_REG0_OFF ((0x0 << 0))

bool dcm_mcu_misccfg_gic_sync_dcm_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(GIC_SYNC_DCM_CFG) &
			~MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK &
			MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON);

	return ret;
}

void dcm_mcu_misccfg_gic_sync_dcm(int on)
{
	if (on) {
		/* TINFO = "Turn ON GIC SYNC DCM" */
		reg_write(GIC_SYNC_DCM_CFG,
				(reg_read(GIC_SYNC_DCM_CFG) &
				~MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK) |
				MCU_MISCCFG_GIC_SYNC_DCM_REG0_ON);
	} else {
		/* TINFO = "Turn OFF GIC SYNC DCM" */
		reg_write(GIC_SYNC_DCM_CFG,
				(reg_read(GIC_SYNC_DCM_CFG) &
				~MCU_MISCCFG_GIC_SYNC_DCM_REG0_MASK) |
				MCU_MISCCFG_GIC_SYNC_DCM_REG0_OFF);
	}
}

#define MCU_MISCCFG_DCM_MCU_BUS_REG0_MASK ((0x1 << 0))
#define MCU_MISCCFG_DCM_MCU_BUS_REG1_MASK ((0x1 << 8))
#define MCU_MISCCFG_DCM_MCU_BUS_REG2_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22) | \
			(0x1 << 23))
#define MCU_MISCCFG_DCM_MCU_BUS_REG3_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6))
#define MCU_MISCCFG_DCM_MCU_BUS_REG4_MASK ((0xf << 0))
#define MCU_MISCCFG_DCM_MCU_BUS_REG0_ON ((0x1 << 0))
#define MCU_MISCCFG_DCM_MCU_BUS_REG1_ON ((0x1 << 8))
#define MCU_MISCCFG_DCM_MCU_BUS_REG2_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 18) | \
			(0x1 << 19) | \
			(0x1 << 20) | \
			(0x1 << 21) | \
			(0x1 << 22) | \
			(0x1 << 23))
#define MCU_MISCCFG_DCM_MCU_BUS_REG3_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 3) | \
			(0x1 << 4) | \
			(0x1 << 5) | \
			(0x1 << 6))
#define MCU_MISCCFG_DCM_MCU_BUS_REG4_ON ((0x0 << 0))
#define MCU_MISCCFG_DCM_MCU_BUS_REG0_OFF ((0x0 << 0))
#define MCU_MISCCFG_DCM_MCU_BUS_REG1_OFF ((0x0 << 8))
#define MCU_MISCCFG_DCM_MCU_BUS_REG2_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 18) | \
			(0x0 << 19) | \
			(0x0 << 20) | \
			(0x0 << 21) | \
			(0x0 << 22) | \
			(0x0 << 23))
#define MCU_MISCCFG_DCM_MCU_BUS_REG3_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x0 << 3) | \
			(0x0 << 4) | \
			(0x0 << 5) | \
			(0x0 << 6))
#define MCU_MISCCFG_DCM_MCU_BUS_REG4_OFF ((0xf << 0))

bool dcm_mcu_misccfg_dcm_mcu_bus_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(L2C_SRAM_CTRL) &
		~MCU_MISCCFG_DCM_MCU_BUS_REG0_MASK &
		MCU_MISCCFG_DCM_MCU_BUS_REG0_ON);
	ret &= !!(reg_read(CCI_CLK_CTRL) &
		~MCU_MISCCFG_DCM_MCU_BUS_REG1_MASK &
		MCU_MISCCFG_DCM_MCU_BUS_REG1_ON);
	ret &= !!(reg_read(BUS_FABRIC_DCM_CTRL) &
		~MCU_MISCCFG_DCM_MCU_BUS_REG2_MASK &
		MCU_MISCCFG_DCM_MCU_BUS_REG2_ON);
	ret &= !!(reg_read(CCI_ADB400_DCM_CONFIG) &
		~MCU_MISCCFG_DCM_MCU_BUS_REG3_MASK &
		MCU_MISCCFG_DCM_MCU_BUS_REG3_ON);
	ret &= !!(reg_read(BIG_DBG_PWR_CTRL) &
		~MCU_MISCCFG_DCM_MCU_BUS_REG4_MASK &
		MCU_MISCCFG_DCM_MCU_BUS_REG4_ON);

	return ret;
}

void dcm_mcu_misccfg_dcm_mcu_bus(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'mcu_misccfg_dcm_mcu_bus'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG0_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG0_ON);
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG1_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG1_ON);
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG2_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG2_ON);
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG3_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG3_ON);
		reg_write(BIG_DBG_PWR_CTRL,
			(reg_read(BIG_DBG_PWR_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG4_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG4_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'mcu_misccfg_dcm_mcu_bus'" */
		reg_write(L2C_SRAM_CTRL,
			(reg_read(L2C_SRAM_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG0_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG0_OFF);
		reg_write(CCI_CLK_CTRL,
			(reg_read(CCI_CLK_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG1_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG1_OFF);
		reg_write(BUS_FABRIC_DCM_CTRL,
			(reg_read(BUS_FABRIC_DCM_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG2_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG2_OFF);
		reg_write(CCI_ADB400_DCM_CONFIG,
			(reg_read(CCI_ADB400_DCM_CONFIG) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG3_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG3_OFF);
		reg_write(BIG_DBG_PWR_CTRL,
			(reg_read(BIG_DBG_PWR_CTRL) &
			~MCU_MISCCFG_DCM_MCU_BUS_REG4_MASK) |
			MCU_MISCCFG_DCM_MCU_BUS_REG4_OFF);
	}
}

#define EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define EMI_DCM_EMI_GROUP_REG1_MASK ((0xff << 24))
#define EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define EMI_DCM_EMI_GROUP_REG1_ON ((0x0 << 24))
#define EMI_DCM_EMI_GROUP_REG0_OFF ((0xf << 24))
#define EMI_DCM_EMI_GROUP_REG1_OFF ((0xe << 24))

bool dcm_emi_dcm_emi_group_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(EMI_CONM) &
		~EMI_DCM_EMI_GROUP_REG0_MASK &
		EMI_DCM_EMI_GROUP_REG0_ON);
	ret &= !!(reg_read(EMI_CONN) &
		~EMI_DCM_EMI_GROUP_REG1_MASK &
		EMI_DCM_EMI_GROUP_REG1_ON);

	return ret;
}

void dcm_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'emi_dcm_emi_group'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_DCM_EMI_GROUP_REG0_MASK) |
			EMI_DCM_EMI_GROUP_REG0_ON);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_DCM_EMI_GROUP_REG1_MASK) |
			EMI_DCM_EMI_GROUP_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'emi_dcm_emi_group'" */
		reg_write(EMI_CONM,
			(reg_read(EMI_CONM) &
			~EMI_DCM_EMI_GROUP_REG0_MASK) |
			EMI_DCM_EMI_GROUP_REG0_OFF);
		reg_write(EMI_CONN,
			(reg_read(EMI_CONN) &
			~EMI_DCM_EMI_GROUP_REG1_MASK) |
			EMI_DCM_EMI_GROUP_REG1_OFF);
	}
}

#define DRAMC_CH0_TOP0_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DRAMC_CH0_TOP0_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 26))
#define DRAMC_CH0_TOP0_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DRAMC_CH0_TOP0_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 26))
#define DRAMC_CH0_TOP0_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DRAMC_CH0_TOP0_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x0 << 26))
#if 0
static unsigned int dramc_ch0_top0_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
		(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_dramc_ch0_top0_ddrphy_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
		~DRAMC_CH0_TOP0_DDRPHY_REG0_MASK &
		DRAMC_CH0_TOP0_DDRPHY_REG0_ON);
	ret &= !!(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
		~DRAMC_CH0_TOP0_DDRPHY_REG1_MASK &
		DRAMC_CH0_TOP0_DDRPHY_REG1_ON);

	return ret;
}

void dcm_dramc_ch0_top0_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch0_top0_ddrphy'" */
		dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(0x8);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH0_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG0_ON);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH0_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch0_top0_ddrphy'" */
		dramc_ch0_top0_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH0_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG0_OFF);
		reg_write(DRAMC_CH0_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH0_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH0_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH0_TOP0_DDRPHY_REG1_OFF);
	}
}

#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_MASK ((0x1 << 31))
#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_ON ((0x1 << 31))
#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_OFF ((0x0 << 31))

bool dcm_dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
		~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_MASK &
		DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_ON);
	ret &= !!(reg_read(DRAMC_CH0_TOP1_CLKAR) &
		~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_MASK &
		DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_ON);

	return ret;
}

void dcm_dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl'" */
		reg_write(DRAMC_CH0_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_MASK) |
			DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_ON);
		reg_write(DRAMC_CH0_TOP1_CLKAR,
			(reg_read(DRAMC_CH0_TOP1_CLKAR) &
			~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_MASK) |
			DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch0_top1_dcm_dramc_ch0_pd_ctrl'" */
		reg_write(DRAMC_CH0_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH0_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_MASK) |
			DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG0_OFF);
		reg_write(DRAMC_CH0_TOP1_CLKAR,
			(reg_read(DRAMC_CH0_TOP1_CLKAR) &
			~DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_MASK) |
			DRAMC_CH0_TOP1_DCM_DRAMC_CH0_PD_CTRL_REG1_OFF);
	}
}

#define CHN0_EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define CHN0_EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define CHN0_EMI_DCM_EMI_GROUP_REG0_OFF ((0xf << 24))

bool dcm_chn0_emi_dcm_emi_group_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(CHN0_EMI_CHN_EMI_CONB) &
		~CHN0_EMI_DCM_EMI_GROUP_REG0_MASK &
		CHN0_EMI_DCM_EMI_GROUP_REG0_ON);

	return ret;
}

void dcm_chn0_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'chn0_emi_dcm_emi_group'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN0_EMI_DCM_EMI_GROUP_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn0_emi_dcm_emi_group'" */
		reg_write(CHN0_EMI_CHN_EMI_CONB,
			(reg_read(CHN0_EMI_CHN_EMI_CONB) &
			~CHN0_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN0_EMI_DCM_EMI_GROUP_REG0_OFF);
	}
}

#define DRAMC_CH1_TOP0_DDRPHY_REG0_MASK ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DRAMC_CH1_TOP0_DDRPHY_REG1_MASK ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x1 << 26))
#define DRAMC_CH1_TOP0_DDRPHY_REG0_ON ((0x0 << 8) | \
			(0x0 << 9) | \
			(0x0 << 10) | \
			(0x0 << 11) | \
			(0x0 << 12) | \
			(0x0 << 13) | \
			(0x0 << 14) | \
			(0x0 << 15) | \
			(0x0 << 16) | \
			(0x0 << 17) | \
			(0x0 << 19))
#define DRAMC_CH1_TOP0_DDRPHY_REG1_ON ((0x0 << 6) | \
			(0x0 << 7) | \
			(0x0 << 26))
#define DRAMC_CH1_TOP0_DDRPHY_REG0_OFF ((0x1 << 8) | \
			(0x1 << 9) | \
			(0x1 << 10) | \
			(0x1 << 11) | \
			(0x1 << 12) | \
			(0x1 << 13) | \
			(0x1 << 14) | \
			(0x1 << 15) | \
			(0x1 << 16) | \
			(0x1 << 17) | \
			(0x1 << 19))
#define DRAMC_CH1_TOP0_DDRPHY_REG1_OFF ((0x1 << 6) | \
			(0x1 << 7) | \
			(0x0 << 26))
#if 0
static unsigned int dramc_ch1_top0_rg_mem_dcm_idle_fsel_get(void)
{
	return (reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) >> 21) & 0x1f;
}
#endif
static void dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(unsigned int val)
{
	reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
		(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
		~(0x1f << 21)) |
		(val & 0x1f) << 21);
}

bool dcm_dramc_ch1_top0_ddrphy_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
		~DRAMC_CH1_TOP0_DDRPHY_REG0_MASK &
		DRAMC_CH1_TOP0_DDRPHY_REG0_ON);
	ret &= !!(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
		~DRAMC_CH1_TOP0_DDRPHY_REG1_MASK &
		DRAMC_CH1_TOP0_DDRPHY_REG1_ON);

	return ret;
}

void dcm_dramc_ch1_top0_ddrphy(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch1_top0_ddrphy'" */
		dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(0x8);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH1_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG0_ON);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH1_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch1_top0_ddrphy'" */
		dramc_ch1_top0_rg_mem_dcm_idle_fsel_set(0x0);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL0,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL0) &
			~DRAMC_CH1_TOP0_DDRPHY_REG0_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG0_OFF);
		reg_write(DRAMC_CH1_TOP0_MISC_CG_CTRL2,
			(reg_read(DRAMC_CH1_TOP0_MISC_CG_CTRL2) &
			~DRAMC_CH1_TOP0_DDRPHY_REG1_MASK) |
			DRAMC_CH1_TOP0_DDRPHY_REG1_OFF);
	}
}

#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_MASK ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x1 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_MASK ((0x1 << 31))
#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_ON ((0x1 << 0) | \
			(0x1 << 1) | \
			(0x1 << 2) | \
			(0x0 << 26) | \
			(0x1 << 30) | \
			(0x1 << 31))
#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_ON ((0x1 << 31))
#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_OFF ((0x0 << 0) | \
			(0x0 << 1) | \
			(0x0 << 2) | \
			(0x1 << 26) | \
			(0x0 << 30) | \
			(0x0 << 31))
#define DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_OFF ((0x0 << 31))

bool dcm_dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
		~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_MASK &
		DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_ON);
	ret &= !!(reg_read(DRAMC_CH1_TOP1_CLKAR) &
		~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_MASK &
		DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_ON);

	return ret;
}

void dcm_dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl'" */
		reg_write(DRAMC_CH1_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_MASK) |
			DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_ON);
		reg_write(DRAMC_CH1_TOP1_CLKAR,
			(reg_read(DRAMC_CH1_TOP1_CLKAR) &
			~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_MASK) |
			DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'dramc_ch1_top1_dcm_dramc_ch1_pd_ctrl'" */
		reg_write(DRAMC_CH1_TOP1_DRAMC_PD_CTRL,
			(reg_read(DRAMC_CH1_TOP1_DRAMC_PD_CTRL) &
			~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_MASK) |
			DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG0_OFF);
		reg_write(DRAMC_CH1_TOP1_CLKAR,
			(reg_read(DRAMC_CH1_TOP1_CLKAR) &
			~DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_MASK) |
			DRAMC_CH1_TOP1_DCM_DRAMC_CH1_PD_CTRL_REG1_OFF);
	}
}

#define CHN1_EMI_DCM_EMI_GROUP_REG0_MASK ((0xff << 24))
#define CHN1_EMI_DCM_EMI_GROUP_REG0_ON ((0x0 << 24))
#define CHN1_EMI_DCM_EMI_GROUP_REG0_OFF ((0xf << 24))

bool dcm_chn1_emi_dcm_emi_group_is_on(int on)
{
	bool ret = false;

	ret &= !!(reg_read(CHN1_EMI_CHN_EMI_CONB) &
		~CHN1_EMI_DCM_EMI_GROUP_REG0_MASK &
		CHN1_EMI_DCM_EMI_GROUP_REG0_ON);

	return ret;
}

void dcm_chn1_emi_dcm_emi_group(int on)
{
	if (on) {
		/* TINFO = "Turn ON DCM 'chn1_emi_dcm_emi_group'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN1_EMI_DCM_EMI_GROUP_REG0_ON);
	} else {
		/* TINFO = "Turn OFF DCM 'chn1_emi_dcm_emi_group'" */
		reg_write(CHN1_EMI_CHN_EMI_CONB,
			(reg_read(CHN1_EMI_CHN_EMI_CONB) &
			~CHN1_EMI_DCM_EMI_GROUP_REG0_MASK) |
			CHN1_EMI_DCM_EMI_GROUP_REG0_OFF);
	}
}


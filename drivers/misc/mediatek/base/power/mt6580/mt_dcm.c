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

#if defined(__KERNEL__)
#include <linux/init.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <asm/bug.h>
/*#include <mach/mt_typedefs.h>*/
#include <mt-plat/sync_write.h>
#include "mt_dcm.h"
#include <mach/mt_secure_api.h>
#include <linux/of.h>
#include <linux/of_address.h>
#else
#include <sync_write.h>
#include <common.h>
#endif

#define _DCM_ACLK_STRESS_
/*#define DCM_DEFAULT_ALL_OFF */

#if defined(__KERNEL__)
#if defined(CONFIG_OF)
static unsigned long topckctrl_base;
static unsigned long mcucfg_base;
static unsigned long mcucfg_phys_base;
static unsigned long dramc_ao_base;
static unsigned long emi_reg_base;
static unsigned long infracfg_ao_base;
/*static unsigned long pericfg_base; */
/*static unsigned long apmixed_base; */
static unsigned long biu_base;

#define MCUCFG_NODE "mediatek,MCUCFG"
#define INFRACFG_AO_NODE "mediatek,INFRACFG_AO"
#define TOPCKGEN_NODE "mediatek,TOPCKGEN"
#define DRAMC_AO_NODE "mediatek,DRAMC0"
#define EMI_REG_NODE "mediatek,EMI"
#define BIU_NODE "mediatek,MCUCFG_BIU"

#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKCTRL_BASE
#define INFRACFG_AO_BASE        (infracfg_ao_base)	/*0xF0001000 */
#define MCUCFG_BASE             (mcucfg_base)	/*0xF0200000 */
#define TOPCKCTRL_BASE           (topckctrl_base)	/*0xF0000000 */
#define DRAMC_AO_BASE              (dramc_ao_base)	/*0xF0207000 */
#define EMI_REG_BASE              (emi_reg_base)	/*0xF0205000 */
#define BIU_BASE				(biu_base)	/*0xF020D000 */

#else				/*#if defined (CONFIG_OF) */
#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKCTRL_BASE
#define INFRACFG_AO_BASE        0xF0001000
#define MCUCFG_BASE             0xF0200000
#define TOPCKCTRL_BASE           0xF0000000
#define DRAMC_AO_BASE           0xF0207000
#define EMI_REG_BASE            0xF0205000	/*rainier is in TOPCKGEN_BASE */
#endif				/*#if defined (CONFIG_OF) */
#else
#define INFRACFG_AO_BASE        0x10001000
#define MCUCFG_BASE             0x10200000
#define TOPCKCTRL_BASE           0x10000000
#define DRAMC_AO_BASE           0x10207000
#define EMI_REG_BASE            0x10205000	/*rainier is in TOPCKGEN_BASE */
#endif


/*APMIXED  //use for special source clock remove in rainier */
/*#define APMIXED_PLL_CON2 (APMIXED_BASE + 0x08) */      /*0x10209008 */

/* MCUCFG registers */
#define MCUCFG_ACLKEN_DIV       (MCUCFG_BASE + 0x60)	/*0x10200060 */
#define MCUCFG_L2C_SRAM_CTRL (MCUCFG_BASE + 0x0)	/*0x10200060 */
#define MCUCFG_MISC_CONFIG      (MCUCFG_BASE + 0x5c)	/*0x10200000 */

#define MCUCFG_ACLKEN_DIV_PHYS       (mcucfg_phys_base + 0x60)	/*0x10200060 */
#define MCUCFG_L2C_SRAM_CTRL_PHYS    (mcucfg_phys_base + 0x0)	/*0x10200000 */
#define MCUCFG_MISC_CONFIG_PHYS      (mcucfg_phys_base + 0x5c)	/*0x10200000 */


/* BIU */
#define MCU_BIU_CON	(BIU_BASE + 0x0)	/*0x1020000  */


/* INFRASYS_AO */
#define TOP_CKMUXSEL (INFRACFG_AO_BASE + 0x0000)	/* 0x10001000 */
#define TOP_CKDIV1 (INFRACFG_AO_BASE + 0x0008)	/* 0x100001008 */
#define INFRA_TOPCKGEN_DCMCTL  (INFRACFG_AO_BASE + 0x010)	/*0x10001010 */
#define INFRA_TOPCKGEN_DCMDBC  (INFRACFG_AO_BASE + 0x014)	/*0x10001014 */
#define INFRA_RSVD1            (INFRACFG_AO_BASE + 0xA00)	/*0x10001A00 */

/* TOP CLOCK CTRL */
#define TOPEMI_DCMCTL    (TOPCKCTRL_BASE + 0x00C)	/*0x1000000C */
#define CLK_GATING_CTRL0 (TOPCKCTRL_BASE + 0x020)	/*0x10000020 */
#define	INFRABUS_DCMCTL0 (TOPCKCTRL_BASE + 0x028)	/*0x10000028 */
#define	INFRABUS_DCMCTL1 (TOPCKCTRL_BASE + 0x02C)	/*0x1000002c */
#ifdef _DCM_ACLK_STRESS_
#define MPLL_FREDIV_EN   (TOPCKCTRL_BASE + 0x030)	/*0x10000030 */
#endif

/*#define INFRA_GLOBALCON_DCMDBC (INFRACFG_AO_BASE + 0x054)*/ /*0x10000054 */


/* TOPCKGEN */
/*#define TOPCKG_DCM_CFG (TOPCKGEN_BASE + 0x4) */  /*0x10210004 */


/* DRAMC_AO */
#define DRAMC_PD_CTRL   (DRAMC_AO_BASE + 0x1dc)	/*0x102141dc */

#define EMI_CONH        (EMI_REG_BASE + 0x38)	/*10205038 */



#if !defined(__KERNEL__)
#define KERN_NOTICE
#define KERN_CONT
#define printk(fmt, args...) dbg_print(fmt, ##args)
#define late_initcall(a)

#define DEFINE_MUTEX(a)  a
#define mutex_lock(a)
#define mutex_unlock(a)

#define IOMEM(a) (a)
#define __raw_readl(a)  (*(volatile unsigned long*)(a))
#define dsb() { __asm__ __volatile__ ("dsb"); }
#define mt_reg_sync_writel(val, addr)  ({ *(unsigned long *)(addr) = val; dsb(); })
#define unlikely(a)  (a)
#define BUG() do { __asm__ __volatile__ ("dsb"); } while (1)
#define BUG_ON(exp) do { if (exp) BUG(); } while (0)
#else
#define USING_KernelLOG
#endif



#define TAG     "[Power/dcm] "

/*#define DCM_ENABLE_DCM_CFG */

#define dcm_err(fmt, args...)                                   \
	pr_err(TAG fmt, ##args)
#define dcm_warn(fmt, args...)                          \
	pr_warn(TAG fmt, ##args)
#define dcm_info(fmt, args...)                          \
	pr_notice(TAG fmt, ##args)
#define dcm_dbg(fmt, args...)                                   \
	pre_debug(TAG fmt, ##args)
#define dcm_ver(fmt, args...)                                   \
	pre_debug(TAG fmt, ##args)




/** macro **/
#define and(v, a) ((v) & (a))
#define or(v, o) ((v) | (o))
#define aor(v, a, o) (((v) & (a)) | (o))

#define reg_read(addr)         __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)

#define DCM_OFF (0)
#define DCM_ON (1)

/** global **/
static DEFINE_MUTEX(dcm_lock);
static unsigned int dcm_initiated;


/*****************************************
 * following is implementation per DCM module.
 * 1. per-DCM function is 1-argu with ON/OFF/MODE option.
 *****************************************/
typedef int (*DCM_FUNC) (int);

/** 0x10000020 CLK_GATING_CTRL0
* 31 31 armdcm_clkoff_en (1: enable , 0: disable )
* 30 30 arm_emiclk_wfi_gating_dis (1: disable, 0: enable)
* ......
**/

#define ARM_EMICLK_WFI_GAT_DIS_MASK (1<<30)
#define ARM_EMICLK_WFI_GAT_DIS_ON (0<<30)
#define ARM_EMICLK_WFI_GAT_DIS_OFF (1<<30)


/** 0x10001014	INFRA_TOPCKGEN_DCMDBC
 * 6	0	topckgen_dcm_dbc_cnt.
 * BUT, this field does not behave as its name.
 * only topckgen_dcm_dbc_cnt[0] is using as ARMPLL DCM mode 1 switch.
 **/

/** 0x10001010	INFRA_TOPCKGEN_DCMCTL
 * 1	1	arm_dcm_wfi_enable
 * 2	2	arm_dcm_wfe_enable
 **/

typedef enum {
	ARMCORE_DCM_OFF = DCM_OFF,
	ARMCORE_DCM_MODE1 = DCM_ON,
	ARMCORE_DCM_MODE2 = DCM_ON + 1,
#ifdef _DCM_ACLK_STRESS_
	ARMCORE_DCM_MODE1_ARMPLL = DCM_ON + 2,
	ARMCORE_DCM_MODE1_UNIPLL = DCM_ON + 3,
	ARMCORE_DCM_MODE1_MAINPLL = DCM_ON + 4,
	ARMCORE_DCM_MODE2_ARMPLL = DCM_ON + 5,
	ARMCORE_DCM_MODE2_UNIPLL = DCM_ON + 6,
	ARMCORE_DCM_MODE2_MAINPLL = DCM_ON + 7,
#endif
} ENUM_ARMCORE_DCM;

/** 0x0000	TOP_CKMUXSEL
 * 3	2	mux1_sel         * "00: CLKSQ, 01: ARMPLL, 10: UNIVPLL, 11: MAINPLL/2
 **/

/** 0x0008	TOP_CKDIV1
 * 4	0	clkdiv1_sel  0x01xxx (4-xxx)/4, 0x10xxx (5-xxx)/5, 0x11xxx (6-xxx)/6
 **/
#ifdef _DCM_ACLK_STRESS_
int G_DCM_TAKE_CTRL = 0;
#endif

int dcm_armcore_pll_clkdiv(int pll, int div)
{

	BUG_ON(pll < 0 || pll > 3);

#if 0				/*fixme, should we have to switch 26M first? */
	reg_write(TOP_CKMUXSEL, aor(reg_read(TOP_CKMUXSEL), ~(3 << 2), 0 << 0));	/*26Mhz*/
#endif				/*#if 0  //fixme, should we have to switch 26M first? */

#ifdef _DCM_ACLK_STRESS_
	/*disable mainpll cg  */
	if (3 == pll) {
		reg_write(INFRA_RSVD1, aor(reg_read(INFRA_RSVD1), ~(1 << 1), (0 << 1)));
		reg_write(MPLL_FREDIV_EN, aor(reg_read(MPLL_FREDIV_EN), ~(1 << 0), (1 << 0)));
	}

	if ((2 == pll) || (3 == pll))
		G_DCM_TAKE_CTRL = 1;
	else
		G_DCM_TAKE_CTRL = 0;
#endif
	reg_write(TOP_CKMUXSEL, aor(reg_read(TOP_CKMUXSEL), ~(3 << 2), pll << 2));
	reg_write(TOP_CKDIV1, aor(reg_read(TOP_CKDIV1), ~(0x1f << 0), div << 0));

	return 0;
}


typedef enum {
	ARMCORE_PLL_GATING_OFF = DCM_OFF,
	ARMCORE_PLL_GATING_ON = DCM_ON,
} ENUM_ARMCORE_PLL_GATING;

int dcm_armcore_pll_gating(ENUM_ARMCORE_PLL_GATING mode)
{
	if (mode == ARMCORE_PLL_GATING_OFF)
		reg_write(INFRA_RSVD1, aor(reg_read(INFRA_RSVD1), ~(1 << 2), (0 << 2)));/*disable armpll ck gating */
	else if (mode == ARMCORE_PLL_GATING_ON)
		reg_write(INFRA_RSVD1, aor(reg_read(INFRA_RSVD1), ~(1 << 2), (1 << 2)));/*enable armpll ck gating */

	return 0;
}

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{

	if (mode == ARMCORE_DCM_OFF) {
		/*swithc to mode 2, and turn wfi/wfe-enable off */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));/*disable mode 1 */
		/*disable wfi/wfe-en */
		reg_write(INFRA_TOPCKGEN_DCMCTL, aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(0x3 << 1), (0 << 1)));

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_OFF));

		return 0;
	}

	if (mode == ARMCORE_DCM_MODE2) {
		/*switch to mode 2 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));/*disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (3 << 1)));

		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(1, (3 << 3) | (0));	/* armpll, 6/6 */

		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	} else if (mode == ARMCORE_DCM_MODE1) {
		/*switch to mode 1, and mode 2 off */
		/*enable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, aor(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0), (1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (0 << 1)));
		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	}
#ifdef _DCM_ACLK_STRESS_
	else if (mode == ARMCORE_DCM_MODE1_ARMPLL) {
		/*switch to mode 1, and mode 2 off */
		/*enable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, aor(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0), (1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (0 << 1)));
		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(1, (3 << 3) | (0));	/* armpll, 6/6 */
		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);
		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	} else if (mode == ARMCORE_DCM_MODE1_UNIPLL) {
		/*switch to mode 1, and mode 2 off */
		/*enable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, aor(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0), (1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (0 << 1)));
		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(2, (3 << 3) | (0));	/* armpll, 6/6 */
		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);
		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	} else if (mode == ARMCORE_DCM_MODE1_MAINPLL) {
		/*switch to mode 1, and mode 2 off */
		/*enable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, aor(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0), (1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (0 << 1)));
		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(3, (3 << 3) | (0));	/* armpll, 6/6 */
		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);
		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	}
	/*mode 2 */
	else if (mode == ARMCORE_DCM_MODE2_ARMPLL) {
		/*switch to mode 2 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));/*disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (3 << 1)));

		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(1, (3 << 3) | (0));	/* armpll, 6/6 */

		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	} else if (mode == ARMCORE_DCM_MODE2_UNIPLL) {
		/*switch to mode 2 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));/*disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (3 << 1)));

		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(2, (3 << 3) | (0));	/* armpll, 6/6 */

		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	} else if (mode == ARMCORE_DCM_MODE2_MAINPLL) {
		/*switch to mode 2 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));/*disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (3 << 1)));

		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(3, (3 << 3) | (0));	/* armpll, 6/6 */

		/* ARMPLL Gating Enable */
		dcm_armcore_pll_gating(ARMCORE_PLL_GATING_ON);

		reg_write(CLK_GATING_CTRL0,
			  aor(reg_read(CLK_GATING_CTRL0), ~ARM_EMICLK_WFI_GAT_DIS_MASK,
			      ARM_EMICLK_WFI_GAT_DIS_ON));
	}
#endif
	return 0;
}


/**************************
 * MCUSYS DCM
 **************************/
/** mcucfg+0x5c CA7_MISC_CONFIG Whole BUS "CG1"
* 9 9   l2_bus_dcm_en "0:disable 1:enable"
**/
#define MCUCFG_L2_BUS_DCM_MASK (0x1<<9)
#define MCUCFG_L2_BUS_DCM_ON   (0x1<<9)
#define MCUCFG_L2_BUS_DCM_OFF  (0x0<<9)


/** mcucfg+0x060	ACLKEN_DIV "CG0"
 * 4	0	axi_div_sel, "L2C BUS clock division::
       5'b10001: 1/1 x cpu clock ,
       5'b10010: 1/2 x cpu clock ,
       5'b10011: 1/3 x cpu clock ,
       5'b10100: 1/4 x cpu clock ,
       5'b10101: 1/5 x cpu clock
 **/
#define MCUCFG_ACLKEN_DIV_MASK (0x01f << 0)
#define MCUCFG_ACLKEN_DIV_ON (0x12 << 0)	/* 1/2 as default */
#define MCUCFG_ACLKEN_DIV_OFF (0x12 << 0)

/** mcucfg+0x0	MCUCFG_L2C_SRAM_CTRL "CG2" (L2C_SRAM)
 * 8	8	l2c_sram_dcm_en, L2C SRAM DCM enable, "0: Disable, 1: Enable"
 **/
#define MCUCFG_L2C_SRAM_CTRL_MASK (0x1 << 8)
#define MCUCFG_L2C_SRAM_CTRL_ON (0x1 << 8)
#define MCUCFG_L2C_SRAM_CTRL_OFF (0x0 << 8)

/** biucfg + 0x0 MCU_BIU_CON (CG3) l2 bus
* 12 12 dcm_en "0: disable, 1: enable"
**/
#define MCU_BIU_CON_DCM_EN_MASK (0x1 << 12)
#define MCU_BIU_CON_DCM_EN_ON (0x1 << 12)
#define MCU_BIU_CON_DCM_EN_OFF (0x0 << 12)


typedef enum {
	MCUSYS_DCM_OFF = DCM_OFF,
	MCUSYS_DCM_ON = DCM_ON,
#ifdef _DCM_ACLK_STRESS_
	MCUSYS_DCM_ACLKDIV_1_00 = DCM_ON + 1,
	MCUSYS_DCM_ACLKDIV_1_11 = DCM_ON + 2,
	MCUSYS_DCM_ACLKDIV_1_12 = DCM_ON + 3,
	MCUSYS_DCM_ACLKDIV_1_13 = DCM_ON + 4,
	MCUSYS_DCM_ACLKDIV_1_14 = DCM_ON + 5,
	MCUSYS_DCM_ACLKDIV_1_15 = DCM_ON + 6,
	MCUSYS_DCM_ACLKDIV_1_16 = DCM_ON + 7,
#endif
} ENUM_MCUSYS_DCM;

typedef enum {
	L2BUS_CLK_DIV_1 = 0x11,
	L2BUS_CLK_DIV_2 = 0x12,
	L2BUS_CLK_DIV_3 = 0x13,
	L2BUS_CLK_DIV_4 = 0x14,
	L2BUS_CLK_DIV_5 = 0x15,
	L2BUS_CLK_DIV_6 = 0x16,
} ENUM_MCUSYS_L2BUS_CLK_DIV;

/*DVFS should set this register */
int dcm_mcusys_l2bus_clk_div(ENUM_MCUSYS_L2BUS_CLK_DIV div)
{
	int value;

	div &= 0x1f;
	value = (div << 0);

	reg_write(MCUCFG_ACLKEN_DIV, aor(reg_read(MCUCFG_ACLKEN_DIV),
					 ~MCUCFG_ACLKEN_DIV_MASK, value));

	return 0;

}

int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_OFF) {

		/*l2bus div2 */
		/*dcm_mcusys_l2bus_clk_div(MCUCFG_ACLKEN_DIV_ON); */

		/*Whole  BUS DCM */
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_OFF));

		/*L2C SRAM DCM */
		/*MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,  */
		/*aor(reg_read(MCUCFG_L2C_SRAM_CTRL), ~MCUCFG_L2C_SRAM_CTRL_MASK, MCUCFG_L2C_SRAM_CTRL_OFF)); */

		/*L2 BUS DCM */
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_OFF));


	}
#ifdef _DCM_ACLK_STRESS_
	else if (on == MCUSYS_DCM_ACLKDIV_1_11) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_1);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	} else if (on == MCUSYS_DCM_ACLKDIV_1_12) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_2);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	} else if (on == MCUSYS_DCM_ACLKDIV_1_13) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_3);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	} else if (on == MCUSYS_DCM_ACLKDIV_1_14) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_4);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	} else if (on == MCUSYS_DCM_ACLKDIV_1_15) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_5);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	} else if (on == MCUSYS_DCM_ACLKDIV_1_16) {
		/*l2bus div1 */
		dcm_mcusys_l2bus_clk_div(L2BUS_CLK_DIV_6);
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));
	}
#else
	else {
		/*l2bus div1 */
		/*dcm_mcusys_l2bus_clk_div(MCUCFG_ACLKEN_DIV_OFF); */

		/*Whole  BUS DCM */
		MCUSYS_SMC_WRITE(MCUCFG_MISC_CONFIG,
				 aor(reg_read(MCUCFG_MISC_CONFIG), ~MCUCFG_L2_BUS_DCM_MASK,
				     MCUCFG_L2_BUS_DCM_ON));
		/*L2C SRAM DCM */
		/*MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,  */
		/*aor(reg_read(MCUCFG_L2C_SRAM_CTRL), ~MCUCFG_L2C_SRAM_CTRL_MASK, MCUCFG_L2C_SRAM_CTRL_ON)); */

		/*L2 BUS DCM */
		MCUSYS_SMC_WRITE(MCU_BIU_CON,
				 aor(reg_read(MCU_BIU_CON), ~MCU_BIU_CON_DCM_EN_MASK,
				     MCU_BIU_CON_DCM_EN_ON));

	}
#endif
	return 0;
}


/** 0x10000010	INFRA_TOPCKGEN_DCMCTL
 * 0	0	infra_dcm_enable
 * this field actually is to activate clock ratio between infra/fast_peri/slow_peri.
 * and need to set when bus clock switch from CLKSQ to PLL.
 * do ASSERT, for each time infra/peri bus dcm setting.
 **/
#define ASSERT_INFRA_DCMCTL() \
	do {      \
		volatile unsigned int dcmctl;                           \
		dcmctl = reg_read(INFRA_TOPCKGEN_DCMCTL);               \
		BUG_ON(!(dcmctl & 1));                                  \
	} while (0)


/** 0x10000028 INFRABUS_DCMCTL0
* 31 31 peridcm_force_on
* 29 29 peridcm_force_clk_off
* 28 28 peridcm_force_clk_slow
* 27 27 peridcm_clkoff_en
* 26 26 peridcm_clkslw_en
* 25 21 peridcm_sfsel
	(0x10000 /1
	 0x01000 /2
	 0x00100 /4
	 0x00010 /8
	 0x00001 /16)
* 20 16 peridcm_fsel
	(0x10000 /1
	 0x01000 /2
	 0x00100 /4
	 0x00010 /8
	 0x00001 /16)
* 15 15 infradcm_force_on
* 13 13 infradcm_force_clk_off
* 12 12 infradcm_force_clk_slow
* 11 11 infradcm_clkoff_en
* 10 10 infradcm_clkslw_en
* 9   5 infradcm_sfsel
	(0x10000 /1
	 0x01000 /2
	 0x00100 /4
	 0x00010 /8
	 0x00001 /16)
* 4  0  infradcm_fsel
	(0x10000 /1
	 0x01000 /2
	 0x00100 /4
	 0x00010 /8
	 0x00001 /16)
**/
#define PERI_DCM_EN_MASK ((1<<27)|(1<<27))
#define PERI_DCM_EN_ON	  ((1<<27)|(1<<26))
#define PERI_DCM_EN_OFF  ((0<<27)|(0<<26))
#define PERI_DCM_SFSEL_MASK (0x1f<<21)
#define PERI_DCM_FSEL_MASK	 (0x1f<<16)

#define INFRA_DCM_EN_MASK ((1<<11)|(1<<10))
#define INFRA_DCM_EN_ON	  ((1<<11)|(1<<10))
#define INFRA_DCM_EN_OFF  ((0<<11)|(0<<10))
#define INFRA_DCM_SFSEL_MASK (0x1f<<5)
#define INFRA_DCM_FSEL_MASK	 (0x1f<<0)


/** 0x1000002c INFRABUS_DCMCTL1
* 31 31 busdcm_pllck_sel
19 15 periDCM_debouce_cnt
14 14 periDCM_debounce_en
13 9  infraDCM_debouce_cnt
8   8    inraDCM_debounce_en
7   3    pmic_sfel
2   2  pmic_spiclkdcm_en
1   1  pmic_bclkdcm_en
0   0  usbdcm_en
**/
#define BUSFCM_PLLCK_SEL_MASK (1<<31)
#define PERI_DCM_DBC_CNT_MASK     (0x1f<<15)
#define PERI_DCM_DBC_EN_MASK      (1<<14)
#define INFRA_DCM_DBC_CNT_MASK	  (0x1f<<9)
#define INFRA_DCM_DBC_EN_MASK     (1<<8)
#define PMIC_SFSEL_MASK    (0x1f<<3)
#define PMIC_SPICLKDCM_EN_MASK (1<<2)
#define PMIC_BCLKDCM_EN_MASK (1<<1)
#define USBDCM_EN_MASK (1<<0)

#define PMIC_DCM_EN_ON   ((1<<2)|(1<<1))
#define PMIC_DCM_EN_OFF  ((0<<2)|(0<<1))

#define USB_DCM_EN_ON  (1<<0)
#define USB_DCM_EN_OFF (0<<0)


typedef enum {
	INFRA_DCM_OFF = DCM_OFF,
	INFRA_DCM_ON = DCM_ON,
} ENUM_INFRA_DCM;


typedef enum {
	INFRA_DCM_DBC_OFF = DCM_OFF,
	INFRA_DCM_DBC_ON = DCM_ON,
} ENUM_INFRA_DCM_DBC;


/* cnt : 0~0x1f */
int dcm_infra_dbc(ENUM_INFRA_DCM_DBC on, int cnt)
{
	int value;

	cnt &= 0x1f;
	on = (on != 0) ? 1 : 0;
	value = (cnt << 9) | (on << 8);

	reg_write(INFRABUS_DCMCTL1, aor(reg_read(INFRABUS_DCMCTL1),
					~(INFRA_DCM_DBC_CNT_MASK | INFRA_DCM_DBC_EN_MASK), value));

	return 0;
}

/*
5'b10000: /1
5'b01000: /2
5'b00100: /4
5'b00010: /8
5'b00001: /16
5'b00000: /32
*/
typedef enum {
	INFRA_DCM_SFSEL_DIV_1 = 0x10,
	INFRA_DCM_SFSEL_DIV_2 = 0x08,
	INFRA_DCM_SFSEL_DIV_4 = 0x04,
	INFRA_DCM_SFSEL_DIV_8 = 0x02,
	INFRA_DCM_SFSEL_DIV_16 = 0x01,
	INFRA_DCM_SFSEL_DIV_32 = 0x00,
} ENUM_INFRA_SFSEL;

int dcm_infra_sfsel(int cnt)
{
	int value;

	cnt &= 0x1f;
	value = (cnt << 5);

	reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0), ~INFRA_DCM_SFSEL_MASK, value));

	return 0;
}


/** input argument
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 **/
int dcm_infra_rate(unsigned full, unsigned int sfel)
{


	full = 0x10 >> full;
	sfel = 0x10 >> sfel;

	reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0),
					~(PERI_DCM_SFSEL_MASK | INFRA_DCM_FSEL_MASK),
					(full << 0) | (sfel << 5)));

	return 0;
}

int dcm_infra(ENUM_INFRA_DCM on)
{

	/*ASSERT_INFRA_DCMCTL(); */

	if (on) {
#if 1				/*override the dbc and fsel setting !! */
		dcm_infra_dbc(INFRA_DCM_DBC_ON, 0x7);	/* dbc 8 */
		dcm_infra_sfsel(INFRA_DCM_SFSEL_DIV_32);	/*sfsel /32 */
#endif
		reg_write(INFRABUS_DCMCTL0,
			  aor(reg_read(INFRABUS_DCMCTL0), ~INFRA_DCM_EN_MASK, INFRA_DCM_EN_ON));
	} else {
#if 1				/*override the dbc and fsel setting !! */
		dcm_infra_dbc(INFRA_DCM_DBC_OFF, 0x7);	/* dbc 8 */
		dcm_infra_sfsel(INFRA_DCM_SFSEL_DIV_32);	/*sfsel /32 */
#endif
		reg_write(INFRABUS_DCMCTL0,
			  aor(reg_read(INFRABUS_DCMCTL0), ~INFRA_DCM_EN_MASK, INFRA_DCM_EN_OFF));
	}

	return 0;
}

typedef enum {
	PERI_DCM_OFF = DCM_OFF,
	PERI_DCM_ON = DCM_ON,
} ENUM_PERI_DCM;


/* cnt: 0~0x1f */
int dcm_peri_dbc(int on, int cnt)
{
	int value;

	BUG_ON(cnt > 0x1f);

	on = (on != 0) ? 1 : 0;
	value = (cnt << 15) | (on << 14);

	reg_write(INFRABUS_DCMCTL1,
		  aor(reg_read(INFRABUS_DCMCTL1),
		      ~(PERI_DCM_DBC_CNT_MASK | PERI_DCM_DBC_EN_MASK), value));

	return 0;
}

/*
5'b10000: /1
5'b01000: /2
5'b00100: /4
5'b00010: /8
5'b00001: /16
5'b00000: /32
*/
typedef enum {
	PERI_DCM_SFSEL_DIV_1 = 0x10,
	PERI_DCM_SFSEL_DIV_2 = 0x08,
	PERI_DCM_SFSEL_DIV_4 = 0x04,
	PERI_DCM_SFSEL_DIV_8 = 0x02,
	PERI_DCM_SFSEL_DIV_16 = 0x01,
	PERI_DCM_SFSEL_DIV_32 = 0x00,
} ENUM_PERI_SFSEL;

int dcm_peri_sfsel(int cnt)
{
	int value;

	cnt &= 0x1f;
	value = (cnt << 21);

	reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0), ~PERI_DCM_SFSEL_MASK, value));

	return 0;
}


/** input argument
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 * default: 5, 5, 5
 **/
int dcm_peri_rate(unsigned int full, unsigned int sfel)
{

	full = 0x10 >> full;
	sfel = 0x10 >> sfel;

	reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0),
					~(PERI_DCM_SFSEL_MASK | PERI_DCM_FSEL_MASK),
					(full << 16) | (sfel << 21)));

	return 0;
}


int dcm_peri(ENUM_PERI_DCM on)
{

	if (on) {
#if 1				/*override the dbc and fsel setting !! */
		dcm_peri_dbc(PERI_DCM_ON, 0x7);
		dcm_peri_sfsel(PERI_DCM_SFSEL_DIV_32);
#endif
		reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0),
						~PERI_DCM_EN_MASK, PERI_DCM_EN_ON));
	} else {
#if 1				/*override the dbc and fsel setting !! */
		dcm_peri_dbc(PERI_DCM_OFF, 0x7);
		dcm_peri_sfsel(PERI_DCM_SFSEL_DIV_32);
#endif
		reg_write(INFRABUS_DCMCTL0, aor(reg_read(INFRABUS_DCMCTL0),
						~PERI_DCM_EN_MASK, PERI_DCM_EN_OFF));
	}

	return 0;
}



typedef enum {
	PMIC_DCM_OFF = DCM_OFF,
	PMIC_DCM_ON = DCM_ON,
} ENUM_PMIC_DCM;

/** input argument
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 **/
int dcm_pmic_rate(unsigned int sfel)
{

	sfel = 0x10 >> sfel;

	reg_write(INFRABUS_DCMCTL1, aor(reg_read(INFRABUS_DCMCTL1), ~PMIC_SFSEL_MASK, (sfel << 3)));

	return 0;
}

int dcm_pmic(ENUM_PMIC_DCM on)
{
	if (on) {
		reg_write(INFRABUS_DCMCTL1,
			  aor(reg_read(INFRABUS_DCMCTL1),
			      ~(PMIC_SPICLKDCM_EN_MASK | PMIC_BCLKDCM_EN_MASK), PMIC_DCM_EN_ON));
	} else {
		reg_write(INFRABUS_DCMCTL1,
			  aor(reg_read(INFRABUS_DCMCTL1),
			      ~(PMIC_SPICLKDCM_EN_MASK | PMIC_BCLKDCM_EN_MASK), PMIC_DCM_EN_OFF));
	}

	return 0;
}

typedef enum {
	USB_DCM_OFF = DCM_OFF,
	USB_DCM_ON = DCM_ON,
} ENUM_USB_DCM;

int dcm_usb(ENUM_USB_DCM on)
{
	if (on) {
		reg_write(INFRABUS_DCMCTL1,
			  aor(reg_read(INFRABUS_DCMCTL1), ~USBDCM_EN_MASK, USB_DCM_EN_ON));
	} else {
		reg_write(INFRABUS_DCMCTL1,
			  aor(reg_read(INFRABUS_DCMCTL1), ~USBDCM_EN_MASK, USB_DCM_EN_OFF));
	}

	return 0;

}


/* will set in preloader */
#if 0
/** 0x102141dc	DRAMC_PD_CTRL
 * 31	31	COMBCLKCTRL ("DQPHY clock dynamic gating control (gating during All-bank Refresh), 1 : controlled by dramc , 0 : always no gating" )
 * 30	30	PHYCLKDYNGEN ("CMDPHY clock dynamic gating control, 1 : controlled by dramc, 0 : always no gating")
 * 25	25	DCMEN ("DRAMC non-freerun clock gating function, 0: disable, 1: enable")
 **/


typedef enum {
	DRAMC_AO_DCM_OFF = DCM_OFF,
	DRAMC_AO_DCM_ON = DCM_ON,
} ENUM_DRAMC_AO_DCM;

int dcm_dramc_ao(ENUM_DRAMC_AO_DCM on)
{
	if (on) {
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~((1 << 25) | (1 << 30) | (1 << 31)),
					     ((1 << 25) | (1 << 30) | (1 << 31))));
	} else {
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~((1 << 25) | (1 << 30) | (1 << 31)),
					     ((0 << 25) | (0 << 30) | (0 << 31))));
	}

	return 0;
}
#endif

/** 0x1000000c	TOPEMI_DCMCTL
 * 27 27 emidcm_clk_off
 * 26 26 emidcm_force_off
 * 25 21 emidcm_idle_fsel( / emidcm_idle_fsel+1)
 * 20 16 emidcm_full_sel
 * 15 9  emidcm_dbc_cnt
 * 8  8  emidcm_dbc_enable
 * 7  7  emidcm_enable
 * 5  1  emidcm_apb_sel
 * 0  0  emidcm_apb_tog
 **/
#define EMIDCM_EN_MASK ((1<<27)|(1<<7))
#define EMIDCM_EN_ON ((1<<27)|(1<<7))
#define EMIDCM_EN_OFF ((0<<27)|(0<<7))

#define EMIDCM_DBC_CNT_MASK (0x7f<<9)
#define EMIDCM_DBC_EN_MASK (1<<8)

#define EMIDCM_TOG_0 (0<<0)
#define EMIDCM_TOG_1 (1<<0)

#define EMIDCM_APB_SEL_FORCE_ON  (1<<1)
#define EMIDCM_APB_SEL_DCM       (1<<2)
#define EMIDCM_APB_SEL_DBC		 (1<<3)
#define EMIDCM_APB_SEL_FSEL		 (1<<4)
#define EMIDCM_APB_SEL_IDLE_FSEL (1<<5)

typedef enum {
	EMI_DCM_OFF = DCM_OFF,
	EMI_DCM_ON = DCM_ON,
} ENUM_EMI_DCM;


/** 0x10205038	EMI_CONH
* 1 0 DCM RATION (00: non dcm, 01: 1/8 slow down, 10: 1/16 slow down, 11: 1/32 slow down)
**/

#define EMIDCM_RATIO_MASK (0x3<<0)

typedef enum {
	EMI_DCM_SLOW_DOWN_RATIO_NON = 0x0,
	EMI_DCM_SLOW_DOWN_RATIO_DIV_8 = 0x1,
	EMI_DCM_SLOW_DOWN_RATIO_DIV_16 = 0x2,
	EMI_DCM_SLOW_DOWN_RATIO_DIV_32 = 0x3,
} ENUM_EMI_DCM_SLOW_DOWN_RATIO;

int dcm_emi_slow_down_ratio(ENUM_EMI_DCM_SLOW_DOWN_RATIO ratio)
{

	int value;

	ratio &= 0x3;
	value = (ratio << 0);
	reg_write(EMI_CONH, aor(reg_read(EMI_CONH), ~EMIDCM_RATIO_MASK, value));
	return 0;
}


/* cnt: 0~0x7f */
int dcm_emi_dbc(int on, int cnt)
{
	int value;

	BUG_ON(cnt > 0x7f);

	on = (on != 0) ? 1 : 0;
	value = (cnt << 9) | (on << 8) | EMIDCM_APB_SEL_DBC;

	reg_write(TOPEMI_DCMCTL,
		  aor(reg_read(TOPEMI_DCMCTL), ~(EMIDCM_DBC_CNT_MASK | EMIDCM_DBC_EN_MASK), value));
	reg_write(TOPEMI_DCMCTL, and(reg_read(TOPEMI_DCMCTL), ~EMIDCM_TOG_1));
	reg_write(TOPEMI_DCMCTL, or(reg_read(TOPEMI_DCMCTL), EMIDCM_TOG_1));

	return 0;
}

int dcm_emi(ENUM_EMI_DCM on)
{
	if (on) {
		dcm_emi_dbc(1, 0xf);
		dcm_emi_slow_down_ratio(EMI_DCM_SLOW_DOWN_RATIO_DIV_32);/*EMI emidcm_idle_fsel default is 0x1f (/32) */
		reg_write(TOPEMI_DCMCTL, aor(reg_read(TOPEMI_DCMCTL),
					     ~EMIDCM_EN_MASK, EMIDCM_EN_ON | EMIDCM_APB_SEL_DCM));
	} else {
		dcm_emi_dbc(0, 0);
		dcm_emi_slow_down_ratio(EMI_DCM_SLOW_DOWN_RATIO_NON);
		reg_write(TOPEMI_DCMCTL, aor(reg_read(TOPEMI_DCMCTL),
					     ~EMIDCM_EN_MASK,
					     EMIDCM_EN_OFF | EMIDCM_APB_SEL_DCM));
	}

	reg_write(TOPEMI_DCMCTL, and(reg_read(TOPEMI_DCMCTL), ~EMIDCM_TOG_1));
	reg_write(TOPEMI_DCMCTL, or(reg_read(TOPEMI_DCMCTL), EMIDCM_TOG_1));

	return 0;
}


/*****************************************************/

enum {
	ARMCORE_DCM_TYPE = (1U << 0),
	MCUSYS_DCM_TYPE = (1U << 1),
	INFRA_DCM_TYPE = (1U << 2),
	PERI_DCM_TYPE = (1U << 3),
	EMI_DCM_TYPE = (1U << 4),
	PMIC_DCM_TYPE = (1U << 5),
	USB_DCM_TYPE = (1U << 6),
	/*DRAMC_DCM_TYPE  = (1U << 5), *//*set in preloader */
	NR_DCM_TYPE = 7,
};

#define ALL_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | PERI_DCM_TYPE |  \
						EMI_DCM_TYPE | PMIC_DCM_TYPE | USB_DCM_TYPE /*| DRAMC_DCM_TYPE*/)

#define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | PERI_DCM_TYPE |  \
						EMI_DCM_TYPE | PMIC_DCM_TYPE | USB_DCM_TYPE)


typedef struct _dcm {
	int current_state;
	int saved_state;
	int disable_refcnt;
	int default_state;
	DCM_FUNC func;
	int typeid;
	char *name;
} DCM;

static DCM dcm_array[NR_DCM_TYPE] = {
	{
	 .typeid = ARMCORE_DCM_TYPE,
	 .name = "ARMCORE_DCM",
	 .func = (DCM_FUNC) dcm_armcore,
	 .current_state = ARMCORE_DCM_MODE1,
	 .default_state = ARMCORE_DCM_MODE1,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = MCUSYS_DCM_TYPE,
	 .name = "MCUSYS_DCM",
	 .func = (DCM_FUNC) dcm_mcusys,
	 .current_state = MCUSYS_DCM_ON,
	 .default_state = MCUSYS_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = INFRA_DCM_TYPE,
	 .name = "INFRA_DCM",
	 .func = (DCM_FUNC) dcm_infra,
	 .current_state = INFRA_DCM_ON,
	 .default_state = INFRA_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = PERI_DCM_TYPE,
	 .name = "PERI_DCM",
	 .func = (DCM_FUNC) dcm_peri,
	 .current_state = PERI_DCM_ON,
	 .default_state = PERI_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = EMI_DCM_TYPE,
	 .name = "EMI_DCM",
	 .func = (DCM_FUNC) dcm_emi,
	 .current_state = EMI_DCM_ON,
	 .default_state = EMI_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = PMIC_DCM_TYPE,
	 .name = "PMIC_DCM",
	 .func = (DCM_FUNC) dcm_pmic,
	 .current_state = PMIC_DCM_ON,
	 .default_state = PMIC_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = USB_DCM_TYPE,
	 .name = "USB_DCM",
	 .func = (DCM_FUNC) dcm_usb,
	 .current_state = USB_DCM_ON,
	 .default_state = USB_DCM_ON,
	 .disable_refcnt = 0,
	 },
	/*  {
	   .typeid = DRAMC_DCM_TYPE,
	   .name = "DRAMC_DCM",
	   .func = (DCM_FUNC)dcm_dramc_ao,
	   .current_state = DRAMC_AO_DCM_ON,
	   .default_state = DRAMC_AO_DCM_ON,
	   .disable_refcnt = 0,
	   }, */
};



/*****************************************
 * DCM driver will provide regular APIs :
 * 1. dcm_restore(type) to recovery CURRENT_STATE before any power-off reset.
 * 2. dcm_set_default(type) to reset as cold-power-on init state.
 * 3. dcm_disable(type) to disable all dcm.
 * 4. dcm_set_state(type) to set dcm state.
 * 5. dcm_dump_state(type) to show CURRENT_STATE.
 * 6. /sys/power/dcm_state interface:  'restore', 'disable', 'dump', 'set'. 4 commands.
 *
 * spsecified APIs for workaround:
 * 1. (definitely no workaround now)
 *****************************************/

void dcm_set_default(unsigned int type)
{
	int i;
	DCM *dcm;

	dcm_info("[%s]type:0x%08x\n", __func__, type);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++) {
		if (type & dcm->typeid) {
			dcm->saved_state = dcm->current_state = dcm->default_state;
			dcm->disable_refcnt = 0;
			dcm->func(dcm->current_state);

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);

		}
	}

	mutex_unlock(&dcm_lock);
}

void dcm_set_state(unsigned int type, int state)
{
	int i;
	DCM *dcm;

	dcm_info("[%s]type:0x%08x, set:%d\n", __func__, type, state);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			dcm->saved_state = state;
			if (dcm->disable_refcnt == 0) {
				dcm->current_state = state;
				dcm->func(dcm->current_state);
			}

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);

		}
	}

	mutex_unlock(&dcm_lock);
}


void dcm_disable(unsigned int type)
{
	int i;
	DCM *dcm;

	dcm_info("[%s]type:0x%08x\n", __func__, type);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			dcm->current_state = DCM_OFF;
			dcm->disable_refcnt++;
			dcm->func(dcm->current_state);

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);

		}
	}

	mutex_unlock(&dcm_lock);

}

void dcm_restore(unsigned int type)
{
	int i;
	DCM *dcm;

	dcm_info("[%s]type:0x%08x\n", __func__, type);

	mutex_lock(&dcm_lock);

	for (i = 0, dcm = &dcm_array[0]; type && (i < NR_DCM_TYPE); i++, dcm++) {
		if (type & dcm->typeid) {
			type &= ~(dcm->typeid);

			if (dcm->disable_refcnt > 0)
				dcm->disable_refcnt--;
			if (dcm->disable_refcnt == 0) {
				dcm->current_state = dcm->saved_state;
				dcm->func(dcm->current_state);
			}

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);

		}
	}

	mutex_unlock(&dcm_lock);
}


void dcm_dump_state(int type)
{
	int i;
	DCM *dcm;

	dcm_info("\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++) {
		if (type & dcm->typeid) {
			dcm_info("[%-16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);
		}
	}

}

#define REG_DUMP(addr) { dcm_info("%-30s(0x%08lx): 0x%08x\n", #addr, addr, reg_read(addr)); }


void dcm_dump_regs(void)
{
	dcm_info("\n******** dcm dump register *********\n");
	/*ARMCORE */
	REG_DUMP(INFRA_RSVD1);
	REG_DUMP(INFRA_TOPCKGEN_DCMCTL);
	REG_DUMP(INFRA_TOPCKGEN_DCMDBC);
	REG_DUMP(CLK_GATING_CTRL0);
#ifdef _DCM_ACLK_STRESS_
	REG_DUMP(MPLL_FREDIV_EN);
#endif
	/*ARMCORE PLL DIV */
	REG_DUMP(TOP_CKMUXSEL);
	REG_DUMP(TOP_CKDIV1);
	/*MCUSYS */
	REG_DUMP(MCUCFG_ACLKEN_DIV);
	REG_DUMP(MCUCFG_MISC_CONFIG);
	REG_DUMP(MCU_BIU_CON);
	/*INFRA,PERI,PMIC,USB */
	REG_DUMP(INFRABUS_DCMCTL1);
	REG_DUMP(INFRABUS_DCMCTL0);
	/*EMI */
	REG_DUMP(TOPEMI_DCMCTL);
	/*DRAMC */
	REG_DUMP(DRAMC_PD_CTRL);
	/*EMI CONH */
	REG_DUMP(EMI_CONH);
}

#if defined(CONFIG_PM)
static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int len = 0;
	char *p = buf;
	int i;
	DCM *dcm;

/*        dcm_dump_state(ALL_DCM_TYPE); */
	p += sprintf(p, "\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++) {
		p += sprintf(p, "[%-16s 0x%08x] current state:%d (%d)\n",
			     dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);
	}

	p += sprintf(p, "\n********** dcm_state help *********\n");
	p += sprintf(p, "set:           echo set [mask] [mode] > /sys/power/dcm_state\n");
	p += sprintf(p, "disable:       echo disable [mask] > /sys/power/dcm_state\n");
	p += sprintf(p, "restore:       echo restore [mask] > /sys/power/dcm_state\n");
	p += sprintf(p, "dump:          echo dump [mask] > /sys/power/dcm_state\n");
	p += sprintf(p, "***** [mask] is hexl bit mask of dcm;\n");
	p += sprintf(p, "***** [mode] is type of DCM to set and retained\n");


	len = p - buf;
	return len;
}

static ssize_t dcm_state_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
			       size_t n)
{
	char cmd[16];
	unsigned int mask;

	if (sscanf(buf, "%15s %x", cmd, &mask) == 2) {
		mask &= ALL_DCM_TYPE;

		if (!strcmp(cmd, "restore")) {
			/*dcm_dump_regs(); */
			dcm_restore(mask);
			/*dcm_dump_regs(); */
		} else if (!strcmp(cmd, "disable")) {
			/*dcm_dump_regs(); */
			dcm_disable(mask);
			/*dcm_dump_regs(); */
		} else if (!strcmp(cmd, "dump")) {
			dcm_dump_state(mask);
			dcm_dump_regs();
		} else if (!strcmp(cmd, "set")) {
			int mode;

			if (sscanf(buf, "%15s %x %d", cmd, &mask, &mode) == 3) {
				mask &= ALL_DCM_TYPE;

				dcm_set_state(mask, mode);
			}
		} else {
			dcm_info("SORRY, do not support your command: %s\n", cmd);
			return -EINVAL;
		}
		return n;
	}

	dcm_info("SORRY, do not support your command.\n");

	return -EINVAL;
}

static struct kobj_attribute dcm_state_attr = {
	.attr = {
		 .name = "dcm_state",
		 .mode = 0644,
		 },
	.show = dcm_state_show,
	.store = dcm_state_store,
};


#endif				/*#if defined (CONFIG_PM) */


#if defined(CONFIG_OF)
static int mt_dcm_dts_map(void)
{
	struct device_node *node;
	struct resource r;

	/* topckgen */
	node = of_find_compatible_node(NULL, NULL, TOPCKGEN_NODE);
	if (!node) {
		dcm_info("error: cannot find node " TOPCKGEN_NODE);
		BUG();
	}
	topckctrl_base = (unsigned long)of_iomap(node, 0);
	if (!topckctrl_base) {
		dcm_info("error: cannot iomap " TOPCKGEN_NODE);
		BUG();
	}


	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " MCUCFG_NODE);
		BUG();
	}
	if (of_address_to_resource(node, 0, &r)) {
		dcm_info("error: cannot get phys addr" MCUCFG_NODE);
		BUG();
	}
	mcucfg_phys_base = r.start;

	mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!mcucfg_base) {
		dcm_info("error: cannot iomap " MCUCFG_NODE);
		BUG();
	}

	/* dramc */
	node = of_find_compatible_node(NULL, NULL, DRAMC_AO_NODE);
	if (!node) {
		dcm_info("error: cannot find node " DRAMC_AO_NODE);
		BUG();
	}
	dramc_ao_base = (unsigned long)of_iomap(node, 0);
	if (!dramc_ao_base) {
		dcm_info("error: cannot iomap " DRAMC_AO_NODE);
		BUG();
	}

	/* emi_reg */
	node = of_find_compatible_node(NULL, NULL, EMI_REG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " EMI_REG_NODE);
		BUG();
	}
	emi_reg_base = (unsigned long)of_iomap(node, 0);
	if (!emi_reg_base) {
		dcm_info("error: cannot iomap " EMI_REG_NODE);
		BUG();
	}

	/* infracfg_ao */
	node = of_find_compatible_node(NULL, NULL, INFRACFG_AO_NODE);
	if (!node) {
		dcm_info("error: cannot find node " INFRACFG_AO_NODE);
		BUG();
	}
	infracfg_ao_base = (unsigned long)of_iomap(node, 0);
	if (!infracfg_ao_base) {
		dcm_info("error: cannot iomap " INFRACFG_AO_NODE);
		BUG();
	}

	/* BIU */
	node = of_find_compatible_node(NULL, NULL, BIU_NODE);
	if (!node) {
		dcm_info("error: cannot find node " BIU_NODE);
		BUG();
	}
	biu_base = (unsigned long)of_iomap(node, 0);
	if (!biu_base) {
		dcm_info("error: cannot iomap " BIU_NODE);
		BUG();
	}


	return 0;
}
#else
static int mt_dcm_dts_map(void)
{
	return 0;
}
#endif


int mt_dcm_init(void)
{
	if (dcm_initiated)
		return 0;

	/** workaround **/

	dcm_initiated = 1;
	mt_dcm_dts_map();




#if !defined(DCM_DEFAULT_ALL_OFF)
	/** enable all dcm **/
	dcm_set_default(INIT_DCM_TYPE);
	/*mt_dcm_topckg_disable(); *//* default TOPCKG DCM OFF for I2C performance but keep others setting as DCM ON */
#else				/*#if !defined (DCM_DEFAULT_ALL_OFF) */
	dcm_set_state(ALL_DCM_TYPE, DCM_OFF);
#endif				/*#if !defined (DCM_DEFAULT_ALL_OFF) */

	dcm_dump_regs();

#if defined(CONFIG_PM)
	{
		int err = 0;

		err = sysfs_create_file(power_kobj, &dcm_state_attr.attr);

		if (err)
			dcm_err("[%s]: fail to create sysfs\n", __func__);
	}

#if defined(DCM_DEBUG_MON)
	{
		int err = 0;

		err = sysfs_create_file(power_kobj, &dcm_debug_mon_attr.attr);

		if (err)
			dcm_err("[%s]: fail to create sysfs\n", __func__);
	}
#endif				/*#if defined (DCM_DEBUG_MON) */
#endif				/*#if defined (CONFIG_PM) */

	return 0;
}

late_initcall(mt_dcm_init);


/**** public APIs *****/


void mt_dcm_disable(void)
{
	mt_dcm_init();
	dcm_disable(ALL_DCM_TYPE);
}
EXPORT_SYMBOL(mt_dcm_disable);

void mt_dcm_restore(void)
{
	mt_dcm_init();
	dcm_restore(ALL_DCM_TYPE);
}
EXPORT_SYMBOL(mt_dcm_restore);

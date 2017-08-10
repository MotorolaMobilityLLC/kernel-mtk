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

#include <linux/init.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/cpu.h>
#include <asm/bug.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/sync_write.h>
#include "mt_dcm.h"
#include <mach/mt_secure_api.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt_dramc.h>

/* #define DCM_DEFAULT_ALL_OFF */

#if defined(CONFIG_OF)
static unsigned long topckgen_base;
static unsigned long mcucfg_base;
static unsigned long mcucfg_phys_base;
static unsigned long dramc_ao_base;
static unsigned long emi_reg_base;
static unsigned long infracfg_ao_base;
/* static unsigned long pericfg_base; */
/* static unsigned long apmixed_base; */
static unsigned long ddrphy_base;

#define MCUCFG_NODE "mediatek,mcucfg"
#define INFRACFG_AO_NODE "mediatek,infracfg_ao"
#define TOPCKGEN_NODE "mediatek,topckgen"
#define EMI_REG_NODE "mediatek,emi"

#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKGEN_BASE
#define INFRACFG_AO_BASE	(infracfg_ao_base)      /* 0xF0000000 */
#define MCUCFG_BASE	     (mcucfg_base)	   /* 0xF0200000 */
#define TOPCKGEN_BASE	   (topckgen_base)	 /* 0xF0210000 */
#if 0
#define PERICFG_BASE	      (pericfg_base)	    /* 0xF0002000 */
#endif
#define DRAMC_AO_BASE	      (dramc_ao_base)	    /* 0xF0214000 */
#define EMI_REG_BASE	      (emi_reg_base)	    /* 0xF0203000 */
/* #define APMIXED_BASE	      (apmixed_base)	    //0x10209000 */
#define DDRPHY_BASE	     (ddrphy_base)	   /* 0xF0213000 */

#else				/* !defined(CONFIG_OF) */
#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKGEN_BASE
#define INFRACFG_AO_BASE	0xF0000000
#define MCUCFG_BASE	     0xF0200000
#define TOPCKGEN_BASE	   0xF0210000
/* #define PERICFG_BASE	    0xF0002000 */
#define DRAMC_AO_BASE	   0xF0214000
#define EMI_REG_BASE	    0xF0203000
/* #define APMIXED_BASE	    0xF0209000 */
#define DDRPHY_BASE	     0xF0213000
#endif				/* #if defined(CONFIG_OF) */

/* MCUCFG registers */
#define MCUCFG_L2C_SRAM_CTRL			(MCUCFG_BASE + 0x648) /* 0x10200648 */
#define MCUCFG_CCI_CLK_CTRL			(MCUCFG_BASE + 0x660) /* 0x10200660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL		(MCUCFG_BASE + 0x668) /* 0x10200668 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG		(MCUCFG_BASE + 0x740) /* 0x10200740 */
#define MCUCFG_SYNC_DCM_CONFIG			(MCUCFG_BASE + 0x744) /* 0x10200744 */
#define MCUCFG_SYNC_DCM_CLUSTER_CONFIG		(MCUCFG_BASE + 0x74C) /* 0x1020074C */

#define MCUCFG_L2C_SRAM_CTRL_PHYS		(mcucfg_phys_base + 0x648) /* 0x10200648 */
#define MCUCFG_CCI_CLK_CTRL_PHYS		(mcucfg_phys_base + 0x660) /* 0x10200660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL_PHYS		(mcucfg_phys_base + 0x668) /* 0x10200668 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG_PHYS	(mcucfg_phys_base + 0x740) /* 0x10200740 */
#define MCUCFG_SYNC_DCM_CONFIG_PHYS		(mcucfg_phys_base + 0x744) /* 0x10200744 */
#define MCUCFG_SYNC_DCM_CLUSTER_CONFIG_PHYS	(mcucfg_phys_base + 0x74C) /* 0x1020074C */

/* INFRASYS_AO */
#define TOP_CKMUXSEL			(INFRACFG_AO_BASE + 0x000) /* 0x10001000 */
#define INFRA_TOPCKGEN_CKDIV1_BIG	(INFRACFG_AO_BASE + 0x024) /* 0x10001024 */
#define INFRA_TOPCKGEN_CKDIV1_SML	(INFRACFG_AO_BASE + 0x028) /* 0x10001028 */
#define INFRA_TOPCKGEN_CKDIV1_BUS	(INFRACFG_AO_BASE + 0x02C) /* 0x1000102C */
#define TOP_DCMCTL			(INFRACFG_AO_BASE + 0x010) /* 0x10001010 */
#define	INFRA_BUS_DCM_CTRL		(INFRACFG_AO_BASE + 0x070) /* 0x10001070 */
#define	PERI_BUS_DCM_CTRL		(INFRACFG_AO_BASE + 0x074) /* 0x10001074 */
#define	P2P_RX_CLK_ON			(INFRACFG_AO_BASE + 0x0a0) /* 0x100010a0 */
#define	INFRA_MISC			(INFRACFG_AO_BASE + 0xf00) /* 0x10001f00 */
#define	INFRA_MISC_1			(INFRACFG_AO_BASE + 0xf0c) /* 0x10001f0c */

/* TOPCKGEN */
#define TOPCKG_CLK_MISC_CFG_0		(TOPCKGEN_BASE + 0x104)	/* 0x10000104 */

#if 0
/* perisys */
#define PERI_GLOBALCON_DCMCTL (PERICFG_BASE + 0x050)	/* 0x10002050 */
#define PERI_GLOBALCON_DCMDBC (PERICFG_BASE + 0x054)	/* 0x10002054 */
#define PERI_GLOBALCON_DCMFSEL (PERICFG_BASE + 0x058)	/* 0x10002058 */
#endif /* 0 */

/* DRAMC_AO */
#define DRAMC_GDDR3CTL1 (DRAMC_AO_BASE + 0x0f4)	/* 0x100040f4 */
#define DRAMC_CLKCTRL   (DRAMC_AO_BASE + 0x130)	/* 0x10004130 */
#define DRAMC_PD_CTRL   (DRAMC_AO_BASE + 0x1dc)	/* 0x100041dc */
#define DRAMC_PERFCTL0  (DRAMC_AO_BASE + 0x1ec)	/* 0x100041ec */

/* ddrphy */
#define DDRPHY_CG_CTRL  (DDRPHY_BASE + 0x63c)	/* 0x1000f63c */

/* EMI */
#define EMI_CONM	(EMI_REG_BASE + 0x060)	/* 0x10203060 */

#define TAG	"[Power/dcm] "
/* #define DCM_ENABLE_DCM_CFG */
#define dcm_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define dcm_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define dcm_info(fmt, args...)	pr_warn(TAG fmt, ##args)
#define dcm_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)
#define dcm_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

/** macro **/
#define and(v, a) ((v) & (a))
#define or(v, o) ((v) | (o))
#define aor(v, a, o) (((v) & (a)) | (o))

#define reg_read(addr)	 __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#endif

#define REG_DUMP(addr) dcm_info("%-30s(0x%08lx): 0x%08x\n", #addr, addr, reg_read(addr))

#define DCM_OFF (0)
#define DCM_ON (1)

/* #define DCM_DEBUG */

/** global **/
static DEFINE_MUTEX(dcm_lock);
static unsigned int dcm_initiated;

/*****************************************
 * following is implementation per DCM module.
 * 1. per-DCM function is 1-argu with ON/OFF/MODE option.
 *****************************************/
typedef int (*DCM_FUNC)(int);
typedef void (*DCM_PRESET_FUNC)(void);

/*
 * INFRACFG_AO DCM golden setting ---------------------------------------------
 */
/*
 * 0x10001010	TOP_DCMCTL	"0: Disable,1: Enable"
 * 0	0	infra_dcm_enable
 * 1	1	armpll_dcm_enable_big
 * 2	2	armpll_dcm_enable_sml
 * 3	3	armpll_dcm_enable_bus
 * 12	12	armpll_dcm_wfi_enable_big
 * 13	13	armpll_dcm_wfi_enable_sml
 * 14	14	armpll_dcm_wfi_enable_bus
 * 16	16	armpll_dcm_wfe_enable_big
 * 17	17	armpll_dcm_wfe_enable_sml
 * 18	18	armpll_dcm_wfe_enable_bus
 */
#define INFRA_TOP_DCMCTL_MODE1_MASK	((0x1 << 1) | (0x1 << 2) | (0x1 << 3))
#define INFRA_TOP_DCMCTL_MODE1_ON	((0x1 << 1) | (0x1 << 2) | (0x1 << 3))
#define INFRA_TOP_DCMCTL_MODE1_OFF	((0x0 << 1) | (0x0 << 2) | (0x0 << 3))
#define INFRA_TOP_DCMCTL_MODE2_MASK	((0x1 << 12) | (0x1 << 13) | \
					 (0x1 << 14) | (0x1 << 16) | \
					 (0x1 << 17) | (0x1 << 18))
#define INFRA_TOP_DCMCTL_MODE2_ON	((0x1 << 12) | (0x1 << 13) | \
					 (0x1 << 14) | (0x1 << 16) | \
					 (0x1 << 17) | (0x1 << 18))
#define INFRA_TOP_DCMCTL_MODE2_OFF	((0x0 << 12) | (0x0 << 13) | \
					 (0x0 << 14) | (0x0 << 16) | \
					 (0x0 << 17) | (0x0 << 18))

typedef enum {
	ARMCORE_DCM_OFF = DCM_OFF,
	ARMCORE_DCM_MODE1 = DCM_ON,
	ARMCORE_DCM_MODE2 = DCM_ON+1,
} ENUM_ARMCORE_DCM;

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{
	if (mode == ARMCORE_DCM_OFF) {
		reg_write(TOP_DCMCTL,
			  aor(reg_read(TOP_DCMCTL),
			      ~(INFRA_TOP_DCMCTL_MODE1_MASK |
				INFRA_TOP_DCMCTL_MODE2_MASK),
			      (INFRA_TOP_DCMCTL_MODE1_OFF |
			       INFRA_TOP_DCMCTL_MODE2_OFF)));

		return 0;
	}

	if (mode == ARMCORE_DCM_MODE2) {
		reg_write(TOP_DCMCTL,
			  aor(reg_read(TOP_DCMCTL),
			      ~(INFRA_TOP_DCMCTL_MODE1_MASK |
				INFRA_TOP_DCMCTL_MODE2_MASK),
			      (INFRA_TOP_DCMCTL_MODE1_OFF |
			       INFRA_TOP_DCMCTL_MODE2_ON)));
	} else if (mode == ARMCORE_DCM_MODE1) {
		reg_write(TOP_DCMCTL,
			  aor(reg_read(TOP_DCMCTL),
			      ~(INFRA_TOP_DCMCTL_MODE1_MASK |
				INFRA_TOP_DCMCTL_MODE2_MASK),
			      (INFRA_TOP_DCMCTL_MODE1_ON |
			       INFRA_TOP_DCMCTL_MODE2_OFF)));
	}

	return 0;
}

/*
 * 0x0000	TOP_CKMUXSEL: Switch clock source for ARMPLL
 * 5	4	armpll_mux1_sel_big	4'b0	Sel Big cluster ARMBPLL clk MUX 1
 *	"00: CLKSQ(26M),01: ARMBPLL_1700M,10: MAINPLL_1092M,11: UNIVPLL_624M"
 * 9	8	armpll_mux1_sel_sml	4'b0	Sel Small cluster ARMSPLL clk MUX 1
 *	"00: CLKSQ(26M),01: ARMSPLL_1000M,10: MAINPLL_1092M,11: UNIVPLL_624M"
 * 13	12	armpll_mux1_sel_bus	4'b0	Sel CCI clock MUX 1
 *	"00: CLKSQ(26M),01: ARMBPLL_1700M,10: ARMSPLL_1000M,11: CLKSQ (26M)"
 */
#define INFRA_TOP_CKMUXSEL_MASK	((0x2 << 4) | (0x2 << 8) | (0x2 << 12))

/*
 * 0x0024	INFRA_TOPCKGEN_CKDIV1_BIG: Big cluster ARMBPLL clock divider 1
 * 4	0	ckdiv1_divsel_big	5'b0 (5'b00000~5'b11111)
 */
/*
 * 0x0028	INFRA_TOPCKGEN_CKDIV1_SML: Small cluster ARMSPLL clock divider 1
 * 4	0	ckdiv1_divsel_sml	5'b0 (5'b00000~5'b11111)
 */
/*
 * 0x002C	INFRA_TOPCKGEN_CKDIV1_BUS: CCI clock divider 1
 * 4	0	ckdiv1_divsel_bus	5'b0 (5'b00000~5'b11111)
 */
#define INFRA_TOPCKGEN_CKDIV1_MASK	(0x1F << 0)
/*
 * 0x10000104	CLK_MISC_CFG_0	32		miscellanenous control (TOPCKGEN)
 * 4	4	armpll_divider_pll1_ck_en	RW	PUBLIC	0	1: Enable clock (MAINPLL)
 * 5	5	armpll_divider_pll2_ck_en	RW	PUBLIC	0	1: Enable clock (UNIVPLL)
 */
#define TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK	(0x1 << 4)
#define TOPCKG_CLK_MISC_CFG_0_MAINPLL_EN	TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK
#define TOPCKG_CLK_MISC_CFG_0_MAINPLL_DIS	(0x0 << 4)
#define TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK	(0x1 << 5)
#define TOPCKG_CLK_MISC_CFG_0_UNIVPLL_EN	TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK
#define TOPCKG_CLK_MISC_CFG_0_UNIVPLL_DIS	(0x0 << 5)

int dcm_armcore_pll_big_clkdiv(int pll, int div)
{
	if (pll < 0 || pll > 3 || div < 0 || div > 31)
		BUG_ON(1);

	if (pll == 2)
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK,
			      TOPCKG_CLK_MISC_CFG_0_MAINPLL_EN));
	else if (pll == 3)
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK,
			      TOPCKG_CLK_MISC_CFG_0_UNIVPLL_EN));
	else
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~(TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK |
				TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK),
			      (TOPCKG_CLK_MISC_CFG_0_MAINPLL_DIS |
			       TOPCKG_CLK_MISC_CFG_0_UNIVPLL_DIS)));
	reg_write(TOP_CKMUXSEL,
		  aor(reg_read(TOP_CKMUXSEL),
		      ~INFRA_TOP_CKMUXSEL_MASK,
		      (pll << 4)));
	reg_write(INFRA_TOPCKGEN_CKDIV1_BIG,
		  aor(reg_read(INFRA_TOPCKGEN_CKDIV1_BIG),
		      ~INFRA_TOPCKGEN_CKDIV1_MASK, div << 0));

	return 0;
}

int dcm_armcore_pll_sml_clkdiv(int pll, int div)
{
	if (pll < 0 || pll > 3 || div < 0 || div > 31)
		BUG_ON(1);

	if (pll == 2)
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK,
			      TOPCKG_CLK_MISC_CFG_0_MAINPLL_EN));
	else if (pll == 3)
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK,
			      TOPCKG_CLK_MISC_CFG_0_UNIVPLL_EN));
	else
		reg_write(TOPCKG_CLK_MISC_CFG_0,
			  aor(reg_read(TOPCKG_CLK_MISC_CFG_0),
			      ~(TOPCKG_CLK_MISC_CFG_0_MAINPLL_MASK |
				TOPCKG_CLK_MISC_CFG_0_UNIVPLL_MASK),
			      (TOPCKG_CLK_MISC_CFG_0_MAINPLL_DIS |
			       TOPCKG_CLK_MISC_CFG_0_UNIVPLL_DIS)));
	reg_write(TOP_CKMUXSEL,
		  aor(reg_read(TOP_CKMUXSEL),
		      ~INFRA_TOP_CKMUXSEL_MASK,
		      (pll << 8)));
	reg_write(INFRA_TOPCKGEN_CKDIV1_SML,
		  aor(reg_read(INFRA_TOPCKGEN_CKDIV1_SML),
		      ~INFRA_TOPCKGEN_CKDIV1_MASK, div << 0));

	return 0;
}

int dcm_armcore_pll_bus_clkdiv(int pll, int div)
{
	if (pll < 0 || pll > 3 || div < 0 || div > 31)
		BUG_ON(1);

	reg_write(TOP_CKMUXSEL,
		  aor(reg_read(TOP_CKMUXSEL),
		      ~INFRA_TOP_CKMUXSEL_MASK,
		      (pll << 12)));
	reg_write(INFRA_TOPCKGEN_CKDIV1_BUS,
		  aor(reg_read(INFRA_TOPCKGEN_CKDIV1_BUS),
		      ~INFRA_TOPCKGEN_CKDIV1_MASK, div << 0));

	return 0;
}

/*
 * 0x10001010	TOP_DCMCTL
 * 0	0	infra_dcm_enable
 * this field actually is to activate clock ratio between infra/fast_peri/slow_peri.
 * and need to set when bus clock switch from CLKSQ to PLL.
 * do ASSERT, for each time infra/peri bus dcm setting.
 */
#define ASSERT_INFRA_DCMCTL() \
	do {      \
		volatile unsigned int dcmctl;			   \
		dcmctl = reg_read(TOP_DCMCTL);	       \
		BUG_ON(!(dcmctl & 1));				  \
	} while (0)

/*
 * 0x10001070	INFRA_BUS_DCM_CTRL
 * 0	0	infra_dcm_rg_clkoff_en
 * 1	1	infra_dcm_rg_clkslow_en
 * 2	2	infra_dcm_rg_force_clkoff
 * 3	3	infra_dcm_rg_force_clkslow
 * 4	4	infra_dcm_rg_force_on
 * 9	5	infra_dcm_rg_fsel		5'h1f off:0x10,on:0x10	Selects Infra DCM active divide
 * 14	10	infra_dcm_rg_sfsel		5'h1f off:0x10,on:0	Selecs Infra DCM idle divide
 * 19	15	infra_dcm_dbc_rg_dbc_num	5'h10	Infra DCM de-bounce number
 * 20	20	infra_dcm_dbc_rg_dbc_en
 * 21	21	rg_axi_dcm_dis_en		1'b0	keep default
 * 22	22	rg_pllck_sel_no_spm		1'b0	keep default
 */
#define INFRA_BUS_DCM_CTRL_MASK	((0x3 << 0) | (0x7 << 2) | (0x1 << 20))
#define INFRA_BUS_DCM_CTRL_EN	((0x3 << 0) | (0x0 << 2) | (0x1 << 20))
#define INFRA_BUS_DCM_CTRL_DIS	((0x0 << 0) | (0x0 << 2) | (0x0 << 20))
#define INFRA_BUS_DCM_CTRL_SEL_MASK	((0x1F << 5) | (0x1F << 10))
#define INFRA_BUS_DCM_CTRL_SEL_EN	((0x10 << 5) | (0x0 << 10))

/*
 * 0x100010a0	P2P_RX_CLK_ON
 * 3	0	p2p_rx_clk_force_on	4'b1111	"0: Dies not force, 1: Force"
 */
#define P2P_RX_CLK_ON_MASK	(0xF<<0)
#define P2P_RX_CLK_ON_EN	(0<<0)
#define P2P_RX_CLK_ON_DIS	(0xF<<0)

/*
 * 0x10001f00	INFRA_MISC	32
 * 28	28	infra_md_fmem_dcm_en	1'b0	"0: disable INFRA_MD DCM, 1: Enable it"
 */
#define INFRA_MISC_MASK		(1<<28)
#define INFRA_MISC_EN		(1<<28)
#define INFRA_MISC_DIS		(0<<28)

/*
 * 0x10001F0C	INFRA_MISC_1	32	Infrasys Miscellaneous Control Register
 * 24	24	topaxi_second_level_dcm_mask_reg	1'b1	"0: enable fine-grained DCM, 1: mask it"
 */
#define INFRA_MISC_1_MASK	(1<<24)
#define INFRA_MISC_1_EN		(0<<24)
#define INFRA_MISC_1_DIS	(1<<24)

typedef enum {
	INFRA_DCM_OFF = DCM_OFF,
	INFRA_DCM_ON = DCM_ON,
} ENUM_INFRA_DCM;

int dcm_infra(ENUM_INFRA_DCM on)
{
	/* ASSERT_INFRA_DCMCTL(); */

	if (on) {
		reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
						 ~INFRA_BUS_DCM_CTRL_MASK,
						 INFRA_BUS_DCM_CTRL_EN));
		reg_write(P2P_RX_CLK_ON, aor(reg_read(P2P_RX_CLK_ON),
						 ~P2P_RX_CLK_ON_MASK,
						 P2P_RX_CLK_ON_EN));
		reg_write(INFRA_MISC, aor(reg_read(INFRA_MISC),
						 ~INFRA_MISC_MASK,
						 INFRA_MISC_EN));
		reg_write(INFRA_MISC_1, aor(reg_read(INFRA_MISC_1),
						 ~INFRA_MISC_1_MASK,
						 INFRA_MISC_1_EN));
	} else {
		reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
						 ~INFRA_BUS_DCM_CTRL_MASK,
						 INFRA_BUS_DCM_CTRL_DIS));
		reg_write(P2P_RX_CLK_ON, aor(reg_read(P2P_RX_CLK_ON),
						 ~P2P_RX_CLK_ON_MASK,
						 P2P_RX_CLK_ON_DIS));
		reg_write(INFRA_MISC, aor(reg_read(INFRA_MISC),
						 ~INFRA_MISC_MASK,
						 INFRA_MISC_DIS));
		reg_write(INFRA_MISC_1, aor(reg_read(INFRA_MISC_1),
						 ~INFRA_MISC_1_MASK,
						 INFRA_MISC_1_DIS));
	}

	return 0;
}

int dcm_infra_dbc(int cnt)
{
	reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
					  ~(0x1f << 15), (cnt << 15)));

	return 0;
}

/*
 * input argument
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 * default: 0, 0
 */
int dcm_infra_rate(unsigned int fsel, unsigned int sfsel)
{
	BUG_ON(fsel > 5 || sfsel > 5);

	fsel = 0x10 >> fsel;
	sfsel = 0x10 >> sfsel;

	reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
					 ~INFRA_BUS_DCM_CTRL_SEL_MASK,
					 ((fsel << 5) | (sfsel << 10))));

	return 0;
}

void dcm_infra_preset(void)
{
	reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
					 ~INFRA_BUS_DCM_CTRL_SEL_MASK,
					 INFRA_BUS_DCM_CTRL_SEL_EN));
}

/*
 * 0x10001074	PERI_BUS_DCM_CTRL
 * 0	0	peri_dcm_rg_clkoff_en		1'b1	off: 0		on: 1
 * 1	1	peri_dcm_rg_clkslow_en		1'b0	off: 0		on: 1
 * 3	3	peri_dcm_rg_force_clkslow	1'b0	off: 0		on: 0
 * 4	4	peri_dcm_rg_force_on		1'b0	off: 0		on: 0
 * 9	5	peri_dcm_rg_fsel		5'h1f	off: 0x1F	on: 0x1F
 * 14	10	peri_dcm_rg_sfsel		5'h1f	off: 0x1F	on: 0
 * 19	15	peri_dcm_dbc_rg_dbc_num		5'h10	off: 0		on: 0x1F
 * 20	20	peri_dcm_dbc_rg_dbc_en		1'b1	off: 0		on: 1
 * 21	21	re_usb_dcm_en			1'b1	off: 0		on: 1
 * 22	22	re_pmic_dcm_en			1'b1	off: 0		on: 1
 * 27	23	pmic_cnt_mst_rg_sfsel		5'h08	off: 0		on: 0
 * 28	28	re_icusb_dcm_en			1'b0	off: 0		on: 1
 * 29	29	rg_audio_dcm_en			1'b0	off: 0		on: 1
 * 31	31	rg_ssusb_top_dcm_en		1'b0	off: 0		on: 1
 */
#define PERI_DCM_MASK		((1<<0) | (1<<1) | (0x1F<<15) | (1<<20))
#define PERI_DCM_EN		((1<<0) | (1<<1) | (0x1F<<15) | (1<<20))
#define PERI_DCM_DIS		((0<<0) | (0<<1) | (0<<15) | (0<<20))
#define PERI_DCM_SEL_MASK	((0x1F<<5) | (0x1F<<10))
#define PERI_DCM_SEL_EN		((0x1F<<5) | (0<<10))

typedef enum {
	PERI_DCM_OFF = DCM_OFF,
	PERI_DCM_ON = DCM_ON,
} ENUM_PERI_DCM;

int dcm_peri(ENUM_PERI_DCM on)
{
	if (on == PERI_DCM_ON)
		reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						 ~PERI_DCM_MASK,
						 PERI_DCM_EN));
	else
		reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						 ~PERI_DCM_MASK,
						 PERI_DCM_DIS));

	return 0;
}


int dcm_peri_dbc(int cnt)
{
	reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						  ~(0x1f << 15),
						  (cnt << 15)));

	return 0;
}

/*
 * input argument
 * 0: 1/1
 * 1: 1/2
 * 2: 1/4
 * 3: 1/8
 * 4: 1/16
 * 5: 1/32
 * default: 0, 0
 */
int dcm_peri_rate(unsigned int fsel, unsigned int sfsel)
{
	BUG_ON(fsel > 5 || sfsel > 5);

	fsel = 0x10 >> fsel;
	sfsel = 0x10 >> sfsel;

	reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
					 ~PERI_DCM_SEL_MASK,
					 ((fsel << 5) | (sfsel << 10))));

	return 0;
}

void dcm_peri_preset(void)
{
	reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
					 ~PERI_DCM_SEL_MASK,
					 PERI_DCM_SEL_EN));
}

/*
 * 0x10001074	PERI_BUS_DCM_CTRL
 * ...
 * 21	21	re_usb_dcm_en			1'b1	off: 0		on: 1
 * 22	22	re_pmic_dcm_en			1'b1	off: 0		on: 1
 * 27	23	pmic_cnt_mst_rg_sfsel		5'h08	off: 0		on: 0
 * 28	28	re_icusb_dcm_en			1'b0	off: 0		on: 1
 * 29	29	rg_audio_dcm_en			1'b0	off: 0		on: 1
 * 31	31	rg_ssusb_top_dcm_en		1'b0	off: 0		on: 1
 */
#define MISC_DCM_DEFAULT_MASK	((1<<21) | (1<<22) | (1<<28) | (1<<29) | (1<<31))
#define MISC_USB_DCM_EN		(1<<21)
#define MISC_PMIC_DCM_MASK	((1<<22) | (0x1F<<23))
#define MISC_PMIC_DCM_EN	((1<<22) | (0<<23))
#define MISC_PMIC_DCM_DIS	((0<<22) | (0<<23))
#define MISC_ICUSB_DCM_EN	(1<<28)
#define MISC_AUDIO_DCM_EN	(1<<29)
#define MISC_SSUSB_DCM_EN	(1<<31)

typedef enum {
	MISC_DCM_OFF = DCM_OFF,
	PMIC_DCM_OFF = DCM_OFF,
	USB_DCM_OFF = DCM_OFF,
	ICUSB_DCM_OFF = DCM_OFF,
	AUDIO_DCM_OFF = DCM_OFF,
	SSUSB_DCM_OFF = DCM_OFF,

	MISC_DCM_ON = DCM_ON,
	PMIC_DCM_ON = DCM_ON,
	USB_DCM_ON = DCM_ON,
	ICUSB_DCM_ON = DCM_ON,
	AUDIO_DCM_ON = DCM_ON,
	SSUSB_DCM_ON = DCM_ON,
} ENUM_MISC_DCM;

/** argu REG, is 1-bit hot value **/
int _dcm_peri_misc(unsigned int reg, int on)
{
	reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						  ~reg, (on) ? reg : 0));

	return 0;
}

int dcm_pmic(ENUM_MISC_DCM on)
{
	if (on == PMIC_DCM_ON)
		reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						 ~MISC_PMIC_DCM_MASK,
						 MISC_PMIC_DCM_EN));
	else
		reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						 ~MISC_PMIC_DCM_MASK,
						 MISC_PMIC_DCM_DIS));

	return 0;
}

int dcm_usb(ENUM_MISC_DCM on)
{
	_dcm_peri_misc(MISC_USB_DCM_EN, on);

	return 0;
}

int dcm_icusb(ENUM_MISC_DCM on)
{
	_dcm_peri_misc(MISC_ICUSB_DCM_EN, on);

	return 0;
}

int dcm_audio(ENUM_MISC_DCM on)
{
	_dcm_peri_misc(MISC_AUDIO_DCM_EN, on);

	return 0;
}

int dcm_ssusb(ENUM_MISC_DCM on)
{
	_dcm_peri_misc(MISC_SSUSB_DCM_EN, on);

	return 0;
}

/*
 * MCUSYS DCM golden setting --------------------------------------------------
 */
/*
 * 0x0648	L2C_SRAM_CTRL
 * 0	0	l2c_sram_dcm_en, L2C SRAM DCM enable, "0: Disable, 1: Enable"
 */
#define MCUCFG_L2C_SRAM_CTRL_MASK	(0x1 << 0)
#define MCUCFG_L2C_SRAM_CTRL_ON		(0x1 << 0)
#define MCUCFG_L2C_SRAM_CTRL_OFF	(0x0 << 0)

/*
 * 0x0660	CCI_CLK_CTRL
 * 8	8	MCU_BUS_DCM_EN	"0: Disable, 1: Enable"
 */
#define MCUCFG_CCI_CLK_CTRL_MASK	(0x1 << 8)
#define MCUCFG_CCI_CLK_CTRL_ON		(0x1 << 8)
#define MCUCFG_CCI_CLK_CTRL_OFF		(0x0 << 8)

/*
 * 0x0668	BUS_FABRIC_DCM_CTRL
 * 0	0	ACLK_INFRA_dynamic_CG_en	"0: disable, 1: enable"
 * 1	1	EMI2_ADB400_S_DCM_CTRL		"0: disable, 1: enable"
 * 4	4	MP0_ADB400_S_DCM_CTRL		"0: disable, 1: enable"
 * 5	5	MP0_ADB400_M_DCM_CTRL		"0: disable, 1: enable"
 * 6	6	MP1_ADB400_S_DCM_CTRL		"0: disable, 1: enable"
 * 7	7	MP0_ADB400_M_DCM_CTRL		"0: disable, 1: enable"
 * 8	8	InfraCLK_INFRA_dynamic_CG_en	"0: disable, 1: enable"
 * 9	9	EMICLK_EMI_dynamic_CG_en	"0: disable, 1: enable"
 * 21	21	L2_Share_ADB400_DCM_CTRL	"0: disable, 1: enable"
 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL_MASK	((0x3 << 0) | (0x3F << 4) | (0x1 << 21))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_ON	((0x3 << 0) | (0x3F << 4) | (0x1 << 21))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_OFF	((0x0 << 0) | (0x0 << 4) | (0x0 << 21))

/*
 * 0x0740	CCI_ADB400_DCM_CONFIG
 * 0	0	cci_m0_adb400_dcm_en	"0: disable, 1: enable"
 * 2	2	cci_m2_adb400_dcm_en	"0: disable, 1: enable"
 * 5	5	cci_s3_adb400_dcm_en	"0: disable, 1: enable"
 * 6	6	cci_s4_adb400_dcm_en	"0: disable, 1: enable"
 * 7	7	l2c_adb400_dcm_en	"0: disable, 1: enable"
 * 16	16	cci_mst_dcm_en		"0: disable, 1: enable"
 * 17	17	cci_slv_dcm_en		"0: disable, 1: enable"
 * 18	18	cci400_mst_rw_dcm_en	"0: disable, 1: enable"
 * 19	19	cci400_slv_rw_dcm_en	"0: disable, 1: enable"
 * 24	24	cci400_top_stall_dcm_en	"0: disable, 1: enable"
 * 28	25	cci400_top_dbnc_cycle	"default:4'hF, Debounce Value must >= 8"
 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG_MASK	((0x1 << 0) | (0x1 << 2) | \
						 (0x7 << 5) | (0xF << 16) | \
						 (0x1 << 24) | (0xF << 25))
#define MCUCFG_CCI_ADB400_DCM_CONFIG_ON		((0x1 << 0) | (0x1 << 2) | \
						 (0x7 << 5) | (0xF << 16) | \
						 (0x1 << 24) | (0xF << 25))
#define MCUCFG_CCI_ADB400_DCM_CONFIG_OFF	((0x0 << 0) | (0x0 << 2) | \
						 (0x0 << 5) | (0x0 << 16) | \
						 (0x0 << 24) | (0xF << 25))

/*
 * 0x0744	SYNC_DCM_CONFIG
 * 0	0	cci_sync_dcm_div_en	"0: disable, 1: enable"
 * 5	1	cci_sync_dcm_div_sel	"Floor(CPU_Freq/(4*system_timer_Freq))"
 * 6	6	cci_sync_dcm_update_tog	"change value will update the divider value"
 * 8	8	mp0_sync_dcm_div_en	"0: disable, 1: enable"
 * 13	9	mp0_sync_dcm_div_sel	"Floor(CPU_Freq/(4*system_timer_Freq))"
 * 14	14	mp0_sync_dcm_update_tog	"change value will update the divider value"
 * 16	16	mp1_sync_dcm_div_en	"0: disable, 1: enable"
 * 21	17	mp1_sync_dcm_div_sel	"Floor(CPU_Freq/(4*system_timer_Freq))"
 * 22	22	mp1_sync_dcm_update_tog	"change value will update the divider value"
 */
#define MCUCFG_SYNC_DCM_MASK		((0x1 << 0) | (0x1F << 1) | (0x1 << 6) | \
					 (0x1 << 8) | (0x1F << 9) | (0x1 << 14) | \
					 (0x1 << 16) | (0x1F << 17) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_CCI_MASK	((0x1 << 0) | (0x1F << 1) | (0x1 << 6))
#define MCUCFG_SYNC_DCM_MP0_MASK	((0x1 << 8) | (0x1F << 9) | (0x1 << 14))
#define MCUCFG_SYNC_DCM_MP1_MASK	((0x1 << 16) | (0x1F << 17) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_ON		((0x1 << 0) | (0x1 << 8) | (0x1 << 16))
#define MCUCFG_SYNC_DCM_OFF		((0x0 << 0) | (0x0 << 8) | (0x0 << 16))
#define MCUCFG_SYNC_DCM_SEL0		((0x0 << 1) | (0x0 << 9) | (0x0 << 17))
#define MCUCFG_SYNC_DCM_SEL2		((0x2 << 1) | (0x2 << 9) | (0x2 << 17))
#define MCUCFG_SYNC_DCM_TOGMASK		((0x1 << 6) | (0x1 << 14) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_CCI_TOGMASK	(0x1 << 6)
#define MCUCFG_SYNC_DCM_MP0_TOGMASK	(0x1 << 14)
#define MCUCFG_SYNC_DCM_MP1_TOGMASK	(0x1 << 22)
#define MCUCFG_SYNC_DCM_TOG1		((0x1 << 6) | (0x1 << 14) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_CCI_TOG1	MCUCFG_SYNC_DCM_CCI_TOGMASK
#define MCUCFG_SYNC_DCM_MP0_TOG1	MCUCFG_SYNC_DCM_MP0_TOGMASK
#define MCUCFG_SYNC_DCM_MP1_TOG1	MCUCFG_SYNC_DCM_MP1_TOGMASK
#define MCUCFG_SYNC_DCM_TOG0		((0x0 << 6) | (0x0 << 14) | (0x0 << 22))
#define MCUCFG_SYNC_DCM_CCI_TOG0	(0x0 << 6)
#define MCUCFG_SYNC_DCM_MP0_TOG0	(0x0 << 14)
#define MCUCFG_SYNC_DCM_MP1_TOG0	(0x0 << 22)
#define MCUCFG_SYNC_DCM_SELTOG_MASK	((0x1F << 1) | (0x1 << 6) | \
					(0x1F << 9) | (0x1 << 14) | \
					(0x1F << 17) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_SELTOG_CCI_MASK	((0x1F << 1) | (0x1 << 6))
#define MCUCFG_SYNC_DCM_SELTOG_MP0_MASK	((0x1F << 9) | (0x1 << 14))
#define MCUCFG_SYNC_DCM_SELTOG_MP1_MASK	((0x1F << 17) | (0x1 << 22))

/*
 * 0x074C	SYNC_DCM_CLUSTER_CONFIG
 * 4	0	mp0_sync_dcm_stall_wr_del_sel	Debounce Value must >= 16
 * 7	7	mp0_sync_dcm_stall_wr_en
 */
#define MCUCFG_SYNC_DCM_CLUSTER_MASK	((0x1F << 0) | (0x1 << 7))
#define MCUCFG_SYNC_DCM_CLUSTER_EN	((0x1F << 0) | (0x1 << 7))
#define MCUCFG_SYNC_DCM_CLUSTER_DIS	((0x0F << 0) | (0x0 << 7))

/* Do not enable it since it will impact the performance */
/* #define ENABLE_SYNC_DCM_CLUSTER */

typedef enum {
	MCUSYS_DCM_OFF = DCM_OFF,
	MCUSYS_DCM_ON = DCM_ON,
} ENUM_MCUSYS_DCM;
int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_OFF) {
		MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,
				aor(reg_read(MCUCFG_L2C_SRAM_CTRL),
					~MCUCFG_L2C_SRAM_CTRL_MASK,
					MCUCFG_L2C_SRAM_CTRL_OFF));
		MCUSYS_SMC_WRITE(MCUCFG_CCI_CLK_CTRL,
				aor(reg_read(MCUCFG_CCI_CLK_CTRL),
					~MCUCFG_CCI_CLK_CTRL_MASK,
					MCUCFG_CCI_CLK_CTRL_OFF));
		MCUSYS_SMC_WRITE(MCUCFG_BUS_FABRIC_DCM_CTRL,
				aor(reg_read(MCUCFG_BUS_FABRIC_DCM_CTRL),
				    ~MCUCFG_BUS_FABRIC_DCM_CTRL_MASK,
				    MCUCFG_BUS_FABRIC_DCM_CTRL_OFF));
		MCUSYS_SMC_WRITE(MCUCFG_CCI_ADB400_DCM_CONFIG,
				aor(reg_read(MCUCFG_CCI_ADB400_DCM_CONFIG),
				    ~MCUCFG_CCI_ADB400_DCM_CONFIG_MASK,
				    MCUCFG_CCI_ADB400_DCM_CONFIG_OFF));
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_OFF |
				     MCUCFG_SYNC_DCM_SEL0 |
				     MCUCFG_SYNC_DCM_TOG0)));
#ifdef ENABLE_SYNC_DCM_CLUSTER
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CLUSTER_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CLUSTER_CONFIG),
				    ~MCUCFG_SYNC_DCM_CLUSTER_MASK,
				    MCUCFG_SYNC_DCM_CLUSTER_DIS));
#endif
	} else {
		MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,
				aor(reg_read(MCUCFG_L2C_SRAM_CTRL),
					~MCUCFG_L2C_SRAM_CTRL_MASK,
					MCUCFG_L2C_SRAM_CTRL_ON));
		MCUSYS_SMC_WRITE(MCUCFG_CCI_CLK_CTRL,
				aor(reg_read(MCUCFG_CCI_CLK_CTRL),
					~MCUCFG_CCI_CLK_CTRL_MASK,
					MCUCFG_CCI_CLK_CTRL_ON));
		MCUSYS_SMC_WRITE(MCUCFG_BUS_FABRIC_DCM_CTRL,
				aor(reg_read(MCUCFG_BUS_FABRIC_DCM_CTRL),
				    ~MCUCFG_BUS_FABRIC_DCM_CTRL_MASK,
				    MCUCFG_BUS_FABRIC_DCM_CTRL_ON));
		MCUSYS_SMC_WRITE(MCUCFG_CCI_ADB400_DCM_CONFIG,
				aor(reg_read(MCUCFG_CCI_ADB400_DCM_CONFIG),
				    ~MCUCFG_CCI_ADB400_DCM_CONFIG_MASK,
				    MCUCFG_CCI_ADB400_DCM_CONFIG_ON));
		/* set xxx_sync_dcm_tog as 0 first */
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_TOGMASK,
				    MCUCFG_SYNC_DCM_TOG0));
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_ON |
				     MCUCFG_SYNC_DCM_SEL0 |
				     MCUCFG_SYNC_DCM_TOG1)));
#ifdef ENABLE_SYNC_DCM_CLUSTER
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CLUSTER_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CLUSTER_CONFIG),
				    ~MCUCFG_SYNC_DCM_CLUSTER_MASK,
				    MCUCFG_SYNC_DCM_CLUSTER_EN));
#endif
	}

	return 0;
}

int dcm_mcusys_cci_stall(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_OFF) {
		MCUSYS_SMC_WRITE(MCUCFG_CCI_ADB400_DCM_CONFIG,
				aor(reg_read(MCUCFG_CCI_ADB400_DCM_CONFIG),
				    ~MCUCFG_CCI_ADB400_DCM_CONFIG_MASK,
				    MCUCFG_CCI_ADB400_DCM_CONFIG_OFF));
	} else {
		MCUSYS_SMC_WRITE(MCUCFG_CCI_ADB400_DCM_CONFIG,
				aor(reg_read(MCUCFG_CCI_ADB400_DCM_CONFIG),
				    ~MCUCFG_CCI_ADB400_DCM_CONFIG_MASK,
				    MCUCFG_CCI_ADB400_DCM_CONFIG_ON));
	}

	return 0;
}

int dcm_mcusys_sync_dcm(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_OFF) {
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_OFF |
				     MCUCFG_SYNC_DCM_SEL2 |
				     MCUCFG_SYNC_DCM_TOG0)));
#ifdef ENABLE_SYNC_DCM_CLUSTER
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CLUSTER_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CLUSTER_CONFIG),
				    ~MCUCFG_SYNC_DCM_CLUSTER_MASK,
				    MCUCFG_SYNC_DCM_CLUSTER_DIS));
#endif
	} else {
		/* set xxx_sync_dcm_tog as 0 first */
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_TOGMASK,
				    MCUCFG_SYNC_DCM_TOG0));
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_ON |
				     MCUCFG_SYNC_DCM_SEL2 |
				     MCUCFG_SYNC_DCM_TOG1)));
#ifdef ENABLE_SYNC_DCM_CLUSTER
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CLUSTER_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CLUSTER_CONFIG),
				    ~MCUCFG_SYNC_DCM_CLUSTER_MASK,
				    MCUCFG_SYNC_DCM_CLUSTER_EN));
#endif
	}

	return 0;
}

#if 0
/*
 * TOPCKGEN DCM golden setting ------------------------------------------------
 */
/*
 * 0x10000004	DCM_CFG
 * 4	0	dcm_full_fsel (axi bus dcm full fsel)
 * 7	7	dcm_enable
 * 14	8	dcm_dbc_cnt
 * 15	15	dcm_dbc_enable
 * 20	16	mem_dcm_full_fsel ("1xxxx:1/1, 01xxx:1/2, 001xx: 1/4, 0001x: 1/8, 00001: 1/16, 00000: 1/32")
 * 21	21	mem_dcm_cfg_latch
 * 22	22	mem_dcm_idle_align
 * 23	23	mem_dcm_enable
 * 30	24	mem_dcm_dbc_cnt
 * 31	31	mem_dcm_dbc_enable
 */
#define TOPCKG_DCM_CFG_MASK     ((0x1f<<0) | (1<<7) | (0x7f<<8) | (1<<15))
#define TOPCKG_DCM_CFG_ON       ((0<<0) | (1<<7) | (0<<8) | (0<<15))
#define TOPCKG_DCM_CFG_OFF      (0<<7)
/* Used for slow idle to enable or disable TOPCK DCM */
#define TOPCKG_DCM_CFG_QMASK     (1<<7)
#define TOPCKG_DCM_CFG_QON       (1<<7)
#define TOPCKG_DCM_CFG_QOFF      (0<<7)

#define TOPCKG_DCM_CFG_FMEM_MASK	    ((0x1f<<16) | (1<<21) | (1<<22) \
					     | (1<<23) | (0x7f<<24) | (1<<31))
#define TOPCKG_DCM_CFG_FMEM_ON	      ((0<<16) | (1<<21) | (0x1<<22) \
					     | (1<<23) | (0<<24) | (0<<31))
#define TOPCKG_DCM_CFG_FMEM_OFF	     ((1<<21) | (0<<23))
/* toggle mem_dcm_cfg_latch since it's triggered by rising edge */
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK     (1<<21)
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_CLEAR    (0<<21)
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_ON       (1<<21)

/*
 * TOPCKG_CLK_MISC_CFG_2
 * 7   0   mem_dcm_force_idle (0: does not force idle, 1: force idle to high)
 */
#define TOPCKG_CLK_MISC_CFG_2_MASK     (0xf<<0)
#define TOPCKG_CLK_MISC_CFG_2_ON       (0xf<<0)
#define TOPCKG_CLK_MISC_CFG_2_OFF      (0x0<<0)

typedef enum {
	TOPCKG_DCM_OFF = DCM_OFF,
	TOPCKG_DCM_ON = DCM_ON,
} ENUM_TOPCKG_DCM;

int dcm_topckg_dbc(int on, int cnt)
{
	int value;

	cnt &= 0x7f;
	on = (on != 0) ? 1 : 0;
	value = (cnt << 8) | (cnt << 24) | (on << 15) | (on << 31);

	reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
				      ~((0xff << 8) | (0xff << 24)), value));

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
int dcm_topckg_rate(unsigned int fmem, unsigned int faxi)
{

	fmem = 0x10 >> fmem;
	faxi = 0x10 >> faxi;

	reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
				      ~((0x1f << 0) | (0x1f << 16)), (fmem << 16) | (faxi << 0)));

	return 0;
}

/** FMEM DCM enable or disable (separate fmem DCM setting from TOPCK)
 *  For writing reg successfully, we need to toggle mem_dcm_cfg_latch first.
 **/
int dcm_fmem(ENUM_TOPCKG_DCM on)
{
	if (on) {
		/* write reverse value of 21th bit */
		reg_write(TOPCKG_DCM_CFG,
			  aor(reg_read(TOPCKG_DCM_CFG),
			      ~TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK,
			      and(~reg_read(TOPCKG_DCM_CFG), TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK)));
		reg_write(TOPCKG_DCM_CFG,
			  aor(reg_read(TOPCKG_DCM_CFG),
			      ~TOPCKG_DCM_CFG_FMEM_MASK, TOPCKG_DCM_CFG_FMEM_ON));
		/* Debug only: force fmem enter idle */
/*		reg_write(TOPCKG_CLK_MISC_CFG_2, TOPCKG_CLK_MISC_CFG_2_ON); */
	} else {
		/* write reverse value of 21th bit */
		reg_write(TOPCKG_DCM_CFG,
			  aor(reg_read(TOPCKG_DCM_CFG),
			      ~TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK,
			      and(~reg_read(TOPCKG_DCM_CFG), TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK)));
		reg_write(TOPCKG_DCM_CFG,
			  aor(reg_read(TOPCKG_DCM_CFG),
			      ~TOPCKG_DCM_CFG_FMEM_MASK, TOPCKG_DCM_CFG_FMEM_OFF));
		/* Debug only: force fmem enter idle */
/*		reg_write(TOPCKG_CLK_MISC_CFG_2, TOPCKG_CLK_MISC_CFG_2_OFF); */
	}

	return 0;
}

int dcm_topckg(ENUM_TOPCKG_DCM on)
{
	if (on) {
		dcm_fmem(on);
		/* please be noticed, here TOPCKG_DCM_CFG_ON will overrid dbc/fsel setting !! */
		reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
					      ~TOPCKG_DCM_CFG_MASK, TOPCKG_DCM_CFG_ON));
	} else {
		dcm_fmem(on);
		reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
					      ~TOPCKG_DCM_CFG_MASK, TOPCKG_DCM_CFG_OFF));
	}

	return 0;
}
#endif /* 0 */

/*
 * DRAMC DCM golden setting ---------------------------------------------------
 */
/*
 * 0x100040f4	DRAMC_GDDR3CTL1
 * 21	21	CKE1FIXOFF	RW	PUBLIC	1'b0	off: 0, on: 0
 *	"CKE1 always off, 0: CKE1 hardware control,1: CKE1 always off"
 * 20	20	CKE1FIXON	RW	PUBLIC	1'b0	off: 0, on: 0
 *	"CKE1 always on, 0: CKE1 hardware control, 1: CKE1 always on"
 */
#define DRAMC_GDDR3CTL1_MASK	((1<<21) | (1<<20))
#define DRAMC_GDDR3CTL1_EN	((0<<21) | (0<<20))
#define DRAMC_GDDR3CTL1_DIS	((0<<21) | (0<<20))

/*
 * 0x10004130	DRAMC_CLKCTRL
 * 29	29	CLK_EN_1	RW	PUBLIC	1'b0	off: 0, on: 1
 *	"Clock pad 1 enable, 0: disable, 1: enable"
 * 28	28	CLK_EN_0	RW	PUBLIC	1'b1	off: 0, on: 1
 *	"Clock pad 0 enable, 0: disable, 1: enable"
 */
#define DRAMC_CLKCTRL_MASK	((1<<29) | (1<<28))
#define DRAMC_CLKCTRL_EN	((1<<29) | (1<<28))
#define DRAMC_CLKCTRL_DIS	((0<<29) | (0<<28))

/*
 * 0x100041dc	DRAMC_PD_CTRL
 * 31	31	COMBCLKCTRL	RW	PUBLIC	1'b1	off: 0, on: 1
 *	"MACRO_COMB clock dynamic gating control
 *	1'b0: disable, MACRO_COMB clock is not gated.
 *	1'b1: enable"
 * 30	30	PHYCLKDYNGEN	RW	PUBLIC	1'b0	off: 0, on: 1
 *	"DDRPHY clock dynamic gating control
 *	1 : controlled by dramc
 *	0 : always no gating"
 * 26	26	MIOCKCTRLOFF	RW	PUBLIC	1'b0	off: 1, on: 0
 *	"dram clk gating parameter
 *	1 : always no gating
 *	0 : controlled by dramc"
 * 25	25	DCMEN	RW	PUBLIC	1'b0	off: 0, on: 1
 *	"DRAMC non-freerun clock gating function
 *	0: disable
 *	1: enable"
 * 24	24	REFFRERUN	RW	PUBLIC	1'b0	off: 0, on: 1
 *	Using FREE-RUN CLK to count refresh period
 * 3	3	COMBPHY_CLKENSAME	RW	PUBLIC	1'b0	off: 0, on: 1
 *	"MACRO_COMB clock dynamic gating control is the same as DDRPHY
 *	1'b0: disable, MACRO_COMB clock is gated by COMBCLKCTRL option.
 *	1'b1: enable"
 */
#define DRAMC_PD_CTRL_MASK	((1<<31) | (1<<30) | (1<<26) | (1<<25) | \
				 (1<<24) | (1<<3))
#define DRAMC_PD_CTRL_EN	((1<<31) | (1<<30) | (0<<26) | (1<<25) | \
				 (1<<24) | (1<<3))
#define DRAMC_PD_CTRL_DIS	((0<<31) | (0<<30) | (1<<26) | (0<<25) | \
				 (0<<24) | (0<<3))

/*
 * 0x100041ec	DRAMC_PERFCTL0
 * 16	16	DISDMOEDIS	RW	PUBLIC	1'b0	off: 1, on: 0
 *	"For power saving, the self-refresh may disable the IO output enable
 *	0: enable the power saving function
 *	1: disable the power saving function"
 */
#define DRAMC_PERFCTL0_MASK	(1<<16)
#define DRAMC_PERFCTL0_EN	(0<<16)
#define DRAMC_PERFCTL0_DIS	(1<<16)

typedef enum {
	DRAMC_AO_DCM_OFF = DCM_OFF,
	DRAMC_AO_DCM_ON = DCM_ON,
} ENUM_DRAMC_AO_DCM;

int dcm_dramc_ao(ENUM_DRAMC_AO_DCM on)
{
	if (on) {
#if 0
		reg_write(DRAMC_GDDR3CTL1, aor(reg_read(DRAMC_GDDR3CTL1),
					     ~DRAMC_GDDR3CTL1_MASK,
					     DRAMC_GDDR3CTL1_EN));
		reg_write(DRAMC_CLKCTRL, aor(reg_read(DRAMC_CLKCTRL),
					     ~DRAMC_CLKCTRL_MASK,
					     DRAMC_CLKCTRL_EN));
#endif
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_EN));
		reg_write(DRAMC_PERFCTL0, aor(reg_read(DRAMC_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_EN));
	} else {
#if 0
		reg_write(DRAMC_GDDR3CTL1, aor(reg_read(DRAMC_GDDR3CTL1),
					     ~DRAMC_GDDR3CTL1_MASK,
					     DRAMC_GDDR3CTL1_DIS));
		reg_write(DRAMC_CLKCTRL, aor(reg_read(DRAMC_CLKCTRL),
					     ~DRAMC_CLKCTRL_MASK,
					     DRAMC_CLKCTRL_DIS));
#endif
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_DIS));
		reg_write(DRAMC_PERFCTL0, aor(reg_read(DRAMC_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_DIS));
	}

	return 0;
}

/*
 * DDRPHY DCM golden setting --------------------------------------------------
 */
/*
 * 0x1000f63c	DDRPHY_CG_CTRL
 * 2	2	RG_M_CK_DYN_CG_EN	RW	PUBLIC	"1'b0	off: 0, on: 1
 *	"DDRPHY C/A M_CK clock gating function enable
 *	1'b0: disable
 *	1'b1: enable"
 * 1	1	RG_M_CK_DYN_CG_EN_COMB	RW	PUBLIC	1'b0	off: 0, on: 1
 *	"DDRPHY DQ M_CK clock gating function enable
 *	1'b0: disable
 *	1'b1: enable"
 */
#define DDRPHY_CG_CTRL_MASK	((1<<2) | (1<<1))
#define DDRPHY_CG_CTRL_EN	((1<<2) | (1<<1))
#define DDRPHY_CG_CTRL_DIS	((0<<2) | (0<<1))

typedef enum {
	DDRPHY_DCM_OFF = DCM_OFF,
	DDRPHY_DCM_ON = DCM_ON,
} ENUM_DDRPHY_DCM;

int dcm_ddrphy(ENUM_DDRPHY_DCM on)
{
	if (on) {
		reg_write(DDRPHY_CG_CTRL,
			  aor(reg_read(DDRPHY_CG_CTRL),
			      ~DDRPHY_CG_CTRL_MASK, DDRPHY_CG_CTRL_EN));
	} else {
		reg_write(DDRPHY_CG_CTRL,
			  aor(reg_read(DDRPHY_CG_CTRL),
			      ~DDRPHY_CG_CTRL_MASK, DDRPHY_CG_CTRL_DIS));
	}

	return 0;
}

/*
 * EMI DCM golden setting -----------------------------------------------------
 */
/** 0x10203060	EMI_CONM
 * 31	24	EMI_DCM_DIS	8'h0	off: FF, on: 0
 **/
#define EMI_CONM_MASK	(0xFF<<24)
#define EMI_CONM_EN	(0<<24)
#define EMI_CONM_DIS	(0xFF<<24)

typedef enum {
	EMI_DCM_OFF = DCM_OFF,
	EMI_DCM_ON = DCM_ON,
} ENUM_EMI_DCM;

int dcm_emi(ENUM_EMI_DCM on)
{
	if (on)
		reg_write(EMI_CONM, aor(reg_read(EMI_CONM), ~EMI_CONM_MASK,
					EMI_CONM_EN));
	else
		reg_write(EMI_CONM, aor(reg_read(EMI_CONM), ~EMI_CONM_MASK,
					EMI_CONM_DIS));

	return 0;
}

/*****************************************************/
enum {
	ARMCORE_DCM = 0,
	MCUSYS_DCM,
	INFRA_DCM,
	PERI_DCM,
	EMI_DCM,
	DRAMC_DCM,
	DDRPHY_DCM,
	PMIC_DCM,
	USB_DCM,
	ICUSB_DCM,
	AUDIO_DCM,
	SSUSB_DCM,

	NR_DCM = 12,
};

enum {
	ARMCORE_DCM_TYPE	= (1U << 0),
	MCUSYS_DCM_TYPE		= (1U << 1),
	INFRA_DCM_TYPE		= (1U << 2),
	PERI_DCM_TYPE		= (1U << 3),
	EMI_DCM_TYPE		= (1U << 4),
	DRAMC_DCM_TYPE		= (1U << 5),
	DDRPHY_DCM_TYPE		= (1U << 6),
	PMIC_DCM_TYPE		= (1U << 7),
	USB_DCM_TYPE		= (1U << 8),
	ICUSB_DCM_TYPE		= (1U << 9),
	AUDIO_DCM_TYPE		= (1U << 10),
	SSUSB_DCM_TYPE		= (1U << 11),

	NR_DCM_TYPE = 12,
};

#define ALL_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE | \
		       DDRPHY_DCM_TYPE | /* PMIC_DCM_TYPE | */ USB_DCM_TYPE | \
		       ICUSB_DCM_TYPE | AUDIO_DCM_TYPE | SSUSB_DCM_TYPE)

#define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | /* EMI_DCM_TYPE | DRAMC_DCM_TYPE |*/ \
		       /*| DDRPHY_DCM_TYPE | PMIC_DCM_TYPE | */ USB_DCM_TYPE | \
		       ICUSB_DCM_TYPE | AUDIO_DCM_TYPE | SSUSB_DCM_TYPE)

typedef struct _dcm {
	int current_state;
	int saved_state;
	int disable_refcnt;
	int default_state;
	DCM_FUNC func;
	DCM_PRESET_FUNC preset_func;
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
	 .preset_func = (DCM_PRESET_FUNC) dcm_infra_preset,
	 .current_state = INFRA_DCM_ON,
	 .default_state = INFRA_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = PERI_DCM_TYPE,
	 .name = "PERI_DCM",
	 .func = (DCM_FUNC) dcm_peri,
	 .preset_func = (DCM_PRESET_FUNC) dcm_peri_preset,
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
	 .typeid = DRAMC_DCM_TYPE,
	 .name = "DRAMC_DCM",
	 .func = (DCM_FUNC) dcm_dramc_ao,
	 .current_state = DRAMC_AO_DCM_ON,
	 .default_state = DRAMC_AO_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = DDRPHY_DCM_TYPE,
	 .name = "DDRPHY_DCM",
	 .func = (DCM_FUNC) dcm_ddrphy,
	 .current_state = DDRPHY_DCM_ON,
	 .default_state = DDRPHY_DCM_ON,
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
	{
	 .typeid = ICUSB_DCM_TYPE,
	 .name = "ICUSB_DCM",
	 .func = (DCM_FUNC) dcm_icusb,
	 .current_state = ICUSB_DCM_ON,
	 .default_state = ICUSB_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = AUDIO_DCM_TYPE,
	 .name = "AUDIO_DCM",
	 .func = (DCM_FUNC) dcm_audio,
	 .current_state = AUDIO_DCM_ON,
	 .default_state = AUDIO_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = SSUSB_DCM_TYPE,
	 .name = "SSUSB_DCM",
	 .func = (DCM_FUNC) dcm_ssusb,
	 .current_state = SSUSB_DCM_ON,
	 .default_state = SSUSB_DCM_ON,
	 .disable_refcnt = 0,
	 },
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
			dcm->saved_state = dcm->default_state;
			dcm->current_state = dcm->default_state;
			dcm->disable_refcnt = 0;
			if (dcm->preset_func)
				dcm->preset_func();
			dcm->func(dcm->current_state);

			dcm_info("[%16s 0x%08x] current state:%d (%d)\n",
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
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
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
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
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
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
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
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
				 dcm->name, dcm->typeid, dcm->current_state,
				 dcm->disable_refcnt);
		}
	}
}

void dcm_dump_regs(void)
{
	dcm_info("\n******** dcm dump register *********\n");
	REG_DUMP(MCUCFG_L2C_SRAM_CTRL);
	REG_DUMP(MCUCFG_CCI_CLK_CTRL);
	REG_DUMP(MCUCFG_BUS_FABRIC_DCM_CTRL);
	REG_DUMP(MCUCFG_CCI_ADB400_DCM_CONFIG);
	REG_DUMP(MCUCFG_SYNC_DCM_CONFIG);
#ifdef ENABLE_SYNC_DCM_CLUSTER
	REG_DUMP(MCUCFG_SYNC_DCM_CLUSTER_CONFIG);
#endif
	REG_DUMP(TOP_CKMUXSEL);
	REG_DUMP(INFRA_TOPCKGEN_CKDIV1_BIG);
	REG_DUMP(INFRA_TOPCKGEN_CKDIV1_SML);
	REG_DUMP(INFRA_TOPCKGEN_CKDIV1_BUS);
	REG_DUMP(TOPCKG_CLK_MISC_CFG_0);
	REG_DUMP(TOP_DCMCTL);
	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(PERI_BUS_DCM_CTRL);
	REG_DUMP(P2P_RX_CLK_ON);
	REG_DUMP(INFRA_MISC);
	REG_DUMP(INFRA_MISC_1);
#if 0
	REG_DUMP(DRAMC_GDDR3CTL1);
	REG_DUMP(DRAMC_CLKCTRL);
#endif
	REG_DUMP(DRAMC_PD_CTRL);
	REG_DUMP(DRAMC_PERFCTL0);
	REG_DUMP(DDRPHY_CG_CTRL);
	REG_DUMP(EMI_CONM);
}


#if defined(CONFIG_PM)
static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	int len = 0;
	int i;
	DCM *dcm;

	/* dcm_dump_state(ALL_DCM_TYPE); */
	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++)
		len += snprintf(buf+len, PAGE_SIZE-len,
				"[%-16s 0x%08x] current state:%d (%d)\n",
				dcm->name, dcm->typeid, dcm->current_state,
				dcm->disable_refcnt);

	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n********** dcm_state help *********\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"set:		echo set [mask] [mode] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"disable:	echo disable [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"restore:	echo restore [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"dump:		echo dump [mask] > /sys/power/dcm_state\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"***** [mask] is hexl bit mask of dcm;\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"***** [mode] is type of DCM to set and retained\n");

	return len;
}

static ssize_t dcm_state_store(struct kobject *kobj,
			       struct kobj_attribute *attr, const char *buf,
			       size_t n)
{
	char cmd[16];
	unsigned int mask;
	int ret, mode;

	if (sscanf(buf, "%15s %x", cmd, &mask) == 2) {
		mask &= ALL_DCM_TYPE;

		if (!strcmp(cmd, "restore")) {
			/* dcm_dump_regs(); */
			dcm_restore(mask);
			/* dcm_dump_regs(); */
		} else if (!strcmp(cmd, "disable")) {
			/* dcm_dump_regs(); */
			dcm_disable(mask);
			/* dcm_dump_regs(); */
		} else if (!strcmp(cmd, "dump")) {
			dcm_dump_state(mask);
			dcm_dump_regs();
		} else if (!strcmp(cmd, "set")) {
			if (sscanf(buf, "%15s %x %d", cmd, &mask, &mode) == 3) {
				mask &= ALL_DCM_TYPE;

				dcm_set_state(mask, mode);
			}
		} else {
			dcm_info("SORRY, do not support your command: %s\n", cmd);
		}
		ret = n;
	} else {
		dcm_info("SORRY, do not support your command.\n");
		ret = -EINVAL;
	}

	return ret;
}

static struct kobj_attribute dcm_state_attr = {
	.attr = {
		 .name = "dcm_state",
		 .mode = 0644,
		 },
	.show = dcm_state_show,
	.store = dcm_state_store,
};
#endif				/* #if defined (CONFIG_PM) */

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
	topckgen_base = (unsigned long)of_iomap(node, 0);
	if (!topckgen_base) {
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
	dramc_ao_base = (unsigned long)mt_dramc_base_get();
	if (!dramc_ao_base) {
		dcm_info("error: cannot get dramc_ao_base from dram api");
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

	/* ddrphy */
	ddrphy_base = (unsigned long)mt_ddrphy_base_get();
	if (!ddrphy_base) {
		dcm_info("error: cannot get ddrphy_base from dram api");
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

	mt_dcm_dts_map();

#if !defined(DCM_DEFAULT_ALL_OFF)
	/** enable all dcm **/
	dcm_set_default(INIT_DCM_TYPE);
#else /* #if !defined (DCM_DEFAULT_ALL_OFF) */
	dcm_set_state(ALL_DCM_TYPE, DCM_OFF);
#endif /* #if !defined (DCM_DEFAULT_ALL_OFF) */

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
#endif /* #if defined (DCM_DEBUG_MON) */
#endif /* #if defined (CONFIG_PM) */

	dcm_initiated = 1;

	return 0;
}
late_initcall(mt_dcm_init);

/**** public APIs *****/
void mt_dcm_disable(void)
{
	mt_dcm_init();
	dcm_disable(ALL_DCM_TYPE);
}

void mt_dcm_restore(void)
{
	mt_dcm_init();
	dcm_restore(ALL_DCM_TYPE);
}

unsigned int sync_dcm_convert_freq2div(unsigned int freq)
{
	unsigned int div = 0;

	if (freq < 52)
		return 0;

	/* max divided ratio = Floor (CPU Frequency / 4* system timer Frequency) */
	div = freq / 52;
	if (div > 31)
		return 31;

	return div;
}

/* unit of frequency is MHz */
int sync_dcm_set_cpu_freq(unsigned int cci, unsigned int mp0, unsigned int mp1)
{
	mt_dcm_init();
	/* set xxx_sync_dcm_tog as 0 first */
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_TOGMASK,
			    MCUCFG_SYNC_DCM_TOG0));
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_SELTOG_MASK,
			    (MCUCFG_SYNC_DCM_TOG1 |
			     (sync_dcm_convert_freq2div(cci) << 1) |
			     (sync_dcm_convert_freq2div(mp0) << 9) |
			     (sync_dcm_convert_freq2div(mp1) << 17))));
#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, cci=%u/%u,%u, mp0=%u/%u,%u, mp1=%u/%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CONFIG), cci,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 1)) >> 1),
		 sync_dcm_convert_freq2div(cci), mp0,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 9)) >> 9),
		 sync_dcm_convert_freq2div(mp0), mp1,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 17)) >> 17),
		 sync_dcm_convert_freq2div(mp1));
#endif

	return 0;
}

int sync_dcm_set_cpu_div(unsigned int cci, unsigned int mp0, unsigned int mp1)
{
	if (cci > 31 || mp0 > 31 || mp1 > 31)
		return -1;

	mt_dcm_init();
	/* set xxx_sync_dcm_tog as 0 first */
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_TOGMASK,
			    MCUCFG_SYNC_DCM_TOG0));
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_SELTOG_MASK,
			    (MCUCFG_SYNC_DCM_TOG1 |
			     (cci << 1) |
			     (mp0 << 9) |
			     (mp1 << 17))));
#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, cci=%u/%u, mp0=%u/%u, mp1=%u/%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CONFIG), cci,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 1)) >> 1), mp0,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 9)) >> 9), mp1,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 17)) >> 17));
#endif

	return 0;
}

int sync_dcm_set_cci_freq(unsigned int cci)
{
	mt_dcm_init();
	/* set xxx_sync_dcm_tog as 0 first */
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_CCI_TOGMASK,
			    MCUCFG_SYNC_DCM_CCI_TOG0));
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_SELTOG_CCI_MASK,
			    (MCUCFG_SYNC_DCM_CCI_TOG1 |
			     (sync_dcm_convert_freq2div(cci) << 1))));
#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, cci=%u, cci_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CONFIG), cci,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 1)) >> 1),
		 sync_dcm_convert_freq2div(cci));
#endif

	return 0;
}

int sync_dcm_set_mp0_freq(unsigned int mp0)
{
	mt_dcm_init();
	/* set xxx_sync_dcm_tog as 0 first */
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_MP0_TOGMASK,
			    MCUCFG_SYNC_DCM_MP0_TOG0));
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_SELTOG_MP0_MASK,
			    (MCUCFG_SYNC_DCM_MP0_TOG1 |
			     (sync_dcm_convert_freq2div(mp0) << 9))));
#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, mp0=%u, mp0_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CONFIG), mp0,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 9)) >> 9),
		 sync_dcm_convert_freq2div(mp0));
#endif

	return 0;
}

int sync_dcm_set_mp1_freq(unsigned int mp1)
{
	mt_dcm_init();
	/* set xxx_sync_dcm_tog as 0 first */
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_MP1_TOGMASK,
			    MCUCFG_SYNC_DCM_MP1_TOG0));
	MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
			    ~MCUCFG_SYNC_DCM_SELTOG_MP1_MASK,
			    (MCUCFG_SYNC_DCM_MP1_TOG1 |
			     (sync_dcm_convert_freq2div(mp1) << 17))));
#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, mp1=%u, mp1_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_CONFIG), mp1,
		 (and(reg_read(MCUCFG_SYNC_DCM_CONFIG), (0x1F << 17)) >> 17),
		 sync_dcm_convert_freq2div(mp1));
#endif

	return 0;
}

/* mt_dcm_topckg_disable/enable is used for slow idle */
void mt_dcm_topckg_disable(void)
{
#if 0
#if !defined(DCM_DEFAULT_ALL_OFF)
	reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
			~TOPCKG_DCM_CFG_QMASK, TOPCKG_DCM_CFG_QOFF));
#endif /* #if !defined (DCM_DEFAULT_ALL_OFF) */
#endif /* 0 */
}

/* mt_dcm_topckg_disable/enable is used for slow idle */
void mt_dcm_topckg_enable(void)
{
#if 0
#if !defined(DCM_DEFAULT_ALL_OFF)
	if (dcm_array[TOPCKG_DCM].current_state != DCM_OFF) {
		reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
				~TOPCKG_DCM_CFG_QMASK, TOPCKG_DCM_CFG_QON));
	}
#endif /* #if !defined (DCM_DEFAULT_ALL_OFF) */
#endif /* 0 */
}

void mt_dcm_topck_off(void)
{
#if 0
	mt_dcm_init();
	dcm_set_state(TOPCKG_DCM_TYPE, DCM_OFF);
#endif /* 0 */
}

void mt_dcm_topck_on(void)
{
#if 0
	mt_dcm_init();
	dcm_set_state(TOPCKG_DCM_TYPE, DCM_ON);
#endif /* 0 */
}

void mt_dcm_peri_off(void)
{
}

void mt_dcm_peri_on(void)
{
}


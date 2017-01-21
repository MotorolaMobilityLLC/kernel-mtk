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
static unsigned long pericfg_base;
static unsigned long efuse_base;
static unsigned long apmixed_base;
static unsigned long ddrphy_base;

#define MCUCFG_NODE "mediatek,MCUCFG"
#define INFRACFG_AO_NODE "mediatek,INFRACFG_AO"
#define TOPCKGEN_NODE "mediatek,CKSYS"
#define PERICFG_NODE "mediatek,PERICFG"
#define EMI_REG_NODE "mediatek,EMI"
#define EFUSE_NODE "mediatek,EFUSEC"
#define APMIXED_NODE "mediatek,APMIXED"

#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKGEN_BASE
#define INFRACFG_AO_BASE        (infracfg_ao_base)	/* 0xF0000000 */
#define MCUCFG_BASE             (mcucfg_base)	/* 0xF0200000 */
#define TOPCKGEN_BASE           (topckgen_base)	/* 0xF0210000 */
#define PERICFG_BASE              (pericfg_base)	/* 0xF0002000 */
#define DRAMC_AO_BASE              (dramc_ao_base)	/* 0xF0214000 */
#define EMI_REG_BASE              (emi_reg_base)	/* 0xF0203000 */
#define EFUSE_BASE              (efuse_base)	/* 0xF0206000 */
#define APMIXED_BASE              (apmixed_base)	/* 0x10209000 */
#define DDRPHY_BASE             (ddrphy_base)	/* 0xF0213000 */

#else				/* #if defined (CONFIG_OF) */
#undef INFRACFG_AO_BASE
#undef MCUCFG_BASE
#undef TOPCKGEN_BASE
#define INFRACFG_AO_BASE        0xF0000000
#define MCUCFG_BASE             0xF0200000
#define TOPCKGEN_BASE           0xF0210000
#define PERICFG_BASE            0xF0002000
#define DRAMC_AO_BASE           0xF0214000
#define EMI_REG_BASE            0xF0203000
#define EFUSE_BASE              0xF0206000
#define APMIXED_BASE            0xF0209000
#define DDRPHY_BASE             0xF0213000
#endif				/* #if defined (CONFIG_OF) */

/* APMIXED */
#define APMIXED_PLL_CON2 (APMIXED_BASE + 0x08)	/* 0x10209008 */

/* MCUCFG registers */
#define MCUCFG_ACLKEN_DIV       (MCUCFG_BASE + 0x640)	/* 0x10200640 */
#define MCUCFG_L2C_SRAM_CTRL    (MCUCFG_BASE + 0x648)	/* 0x10200648 */
#define MCUCFG_CCI_CLK_CTRL     (MCUCFG_BASE + 0x660)	/* 0x10200660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL (MCUCFG_BASE + 0x668)	/* 0x10200668 */

#define MCUCFG_ACLKEN_DIV_PHYS       (mcucfg_phys_base + 0x640)	/* 0x10200640 */
#define MCUCFG_L2C_SRAM_CTRL_PHYS    (mcucfg_phys_base + 0x648)	/* 0x10200648 */
#define MCUCFG_CCI_CLK_CTRL_PHYS     (mcucfg_phys_base + 0x660)	/* 0x10200660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL_PHYS (mcucfg_phys_base + 0x668)	/* 0x10200668 */

/* INFRASYS_AO */
#define TOP_CKMUXSEL (INFRACFG_AO_BASE + 0x0000)	/* 0x100000000 */
#define TOP_CKDIV1 (INFRACFG_AO_BASE + 0x0008)	/* 0x100000008 */
#define INFRA_TOPCKGEN_DCMCTL  (INFRACFG_AO_BASE + 0x010)	/* 0x10000010 */
#define INFRA_TOPCKGEN_DCMDBC  (INFRACFG_AO_BASE + 0x014)	/* 0x10000014 */
#define	INFRA_GLOBALCON_DCMCTL (INFRACFG_AO_BASE + 0x050)	/* 0x10000050 */
#define INFRA_GLOBALCON_DCMDBC (INFRACFG_AO_BASE + 0x054)	/* 0x10000054 */
#define INFRA_GLOBALCON_DCMFSEL (INFRACFG_AO_BASE + 0x058)	/* 0x10000058 */

/* TOPCKGEN */
#define TOPCKG_DCM_CFG (TOPCKGEN_BASE + 0x4)	/* 0x10210004 */
#define TOPCKG_CLK_MISC_CFG_2 (TOPCKGEN_BASE + 0x218)	/* 0x10210218 */

/* perisys */
#define PERI_GLOBALCON_DCMCTL (PERICFG_BASE + 0x050)	/* 0x10002050 */
#define PERI_GLOBALCON_DCMDBC (PERICFG_BASE + 0x054)	/* 0x10002054 */
#define PERI_GLOBALCON_DCMFSEL (PERICFG_BASE + 0x058)	/* 0x10002058 */

/* DRAMC_AO */
#define DRAMC_PD_CTRL   (DRAMC_AO_BASE + 0x1dc)	/* 0x102141dc */
#define DRAMC_CLKCTRL   (DRAMC_AO_BASE + 0x130)	/* 0x10214130 */
#define DRAMC_PERFCTL0  (DRAMC_AO_BASE + 0x1ec)	/* 0x102141ec */
#define DRAMC_GDDR3CTL1 (DRAMC_AO_BASE + 0x0f4)	/* 0x102140f4 */

/* ddrphy */
#define DDRPHY_MEMPLL_DIVIDER  (DDRPHY_BASE + 0x640)	/* 0x10213640 */

/* EMI */
#define EMI_CONM        (EMI_REG_BASE + 0x060)	/* 0x10203060 */

/* efuse */
#define EFUSE_REG_DCM_ON (EFUSE_BASE + 0x55c)	/* 0x1020655c */

#define TAG     "[Power/dcm] "
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

#define reg_read(addr)         __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#endif

#define REG_DUMP(addr) dcm_info("%-30s(0x%08lx): 0x%08x\n", #addr, addr, reg_read(addr))

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

/** 0x10000014	INFRA_TOPCKGEN_DCMDBC
 * 6	0	topckgen_dcm_dbc_cnt.
 * BUT, this field does not behave as its name.
 * only topckgen_dcm_dbc_cnt[0] is using as ARMPLL DCM mode 1 switch.
 **/

/** 0x10000010	INFRA_TOPCKGEN_DCMCTL
 * 1	1	arm_dcm_wfi_enable
 * 2	2	arm_dcm_wfe_enable
 **/

typedef enum {
	ARMCORE_DCM_OFF = DCM_OFF,
	ARMCORE_DCM_MODE1 = DCM_ON,
	ARMCORE_DCM_MODE2 = DCM_ON + 1,
} ENUM_ARMCORE_DCM;

/** 0x0000	TOP_CKMUXSEL
 * 3	2	mux1_sel         * "00: CLKSQ, 01: ARMPLL, 10: MAINPLL, 11: 1'b0"
 **/

/** 0x0008	TOP_CKDIV1
 * 4	0	clkdiv1_sel
 **/

int dcm_armcore_pll_clkdiv(int pll, int div)
{

	BUG_ON(pll < 0 || pll > 2);

#if 0 /* FIXME, should we have to switch 26M first? */
	reg_write(TOP_CKMUXSEL, aor(reg_read(TOP_CKMUXSEL), ~(3 << 2), 0 << 0));	/* 26Mhz */
#endif

	{
		/* the cg of pll_src_2 */
		if (pll == 2)
			reg_write(APMIXED_PLL_CON2,
				  aor(reg_read(APMIXED_PLL_CON2), ~(0x1 << 5), 1 << 5));
	}


	reg_write(TOP_CKMUXSEL, aor(reg_read(TOP_CKMUXSEL), ~(3 << 2), pll << 2));
	{
		/* cg clear after mux */
		if (pll != 2)
			reg_write(APMIXED_PLL_CON2,
				  aor(reg_read(APMIXED_PLL_CON2), ~(0x1 << 5), 0 << 5));
	}

	reg_write(TOP_CKDIV1, aor(reg_read(TOP_CKDIV1), ~(0x1f << 0), div << 0));

	return 0;
}

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{

	if (mode == ARMCORE_DCM_OFF) {
		/* swithc to mode 2, and turn wfi/wfe-enable off */
		/* disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));
		/* disable wfi/wfe-en */
		reg_write(INFRA_TOPCKGEN_DCMCTL, aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(0x3 << 1), (0 << 1)));

		return 0;
	}

	if (mode == ARMCORE_DCM_MODE2) {
		/* switch to mode 2 */
		/* disable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, and(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (3 << 1)));

		/* OVERRIDE pll mux and clkdiv !! */
		dcm_armcore_pll_clkdiv(1, (3 << 3) | (0));	/* armpll, 6/6 */
	}

	else if (mode == ARMCORE_DCM_MODE1) {
		/* switch to mode 1, and mode 2 off */
		/* enable mode 1 */
		reg_write(INFRA_TOPCKGEN_DCMDBC, aor(reg_read(INFRA_TOPCKGEN_DCMDBC), ~(1 << 0), (1 << 0)));
		reg_write(INFRA_TOPCKGEN_DCMCTL,
			  aor(reg_read(INFRA_TOPCKGEN_DCMCTL), ~(3 << 1), (0 << 1)));
	}

	return 0;
}


/**************************
 * MCUSYS DCM
 **************************/

/** mcucfg+0x0640	ACLKEN_DIV
 * 4	0	axi_div_sel, "L2C SRAM interface and MCU_BIU clock divider setting ::
       5'h01: 1/1 x cpu clock ,
       5'h11: 1/1 x cpu clock ,
       5'b12: 1/2 x cpu clock ,
       5'b14: 1/4 x cpu clock
 * Please set 0x1020-0640 bit[4:0],  5’h12 for 1/2 CPU clock.
 * mcu_biu can only run up to 1/2 of 1.3GHz CPU clock.
 **/
#define MCUCFG_ACLKEN_DIV_MASK (0x01f << 0)
#define MCUCFG_ACLKEN_DIV_ON (0x12 << 0)
#define MCUCFG_ACLKEN_DIV_OFF (0x12 << 0)

/** 0x0648	L2C_SRAM_CTRL
 * 0	0	l2c_sram_dcm_en, L2C SRAM DCM enable, "0: Disable, 1: Enable"
 **/
#define MCUCFG_L2C_SRAM_CTRL_MASK (0x1 << 0)
#define MCUCFG_L2C_SRAM_CTRL_ON (0x1 << 0)
#define MCUCFG_L2C_SRAM_CTRL_OFF (0x0 << 0)

/** 0x0660	CCI_CLK_CTRL
 * 8	8	MCU_BUS_DCM_EN	"0: Disable, 1: Enable"
 **/
#define MCUCFG_CCI_CLK_CTRL_MASK (0x1 << 8)
#define MCUCFG_CCI_CLK_CTRL_ON  (0x1 << 8)
#define MCUCFG_CCI_CLK_CTRL_OFF  (0x0 << 8)

/** 0x0668	BUS_FABRIC_DCM_CTRL
 * 0	0	ACLK_INFRA_dynamic_CG_en	"0: disable, 1: enable"
 * 1	1	ACLK_EMI_dynamic_CG_en	        "0: disable, 1: enable"
 * 8	8	InfraCLK_INFRA_dynamic_CG_en	"0: disable, 1: enable"
 * 9	9	EMICLK_EMI_dynamic_CG_en	"0: disable, 1: enable"
 **/
#define MCUCFG_BUS_FABRIC_DCM_CTRL_MASK ((3<<0) | (3<<8))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_ON   ((3<<0) | (3<<8))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_OFF  ((0<<0) | (0<<8))

typedef enum {
	MCUSYS_DCM_OFF = DCM_OFF,
	MCUSYS_DCM_ON = DCM_ON,
} ENUM_MCUSYS_DCM;
int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_OFF) {
		/* MCUSYS CCI CTRL, */
		MCUSYS_SMC_WRITE(MCUCFG_CCI_CLK_CTRL,
				 aor(reg_read(MCUCFG_CCI_CLK_CTRL), ~MCUCFG_CCI_CLK_CTRL_MASK,
				     MCUCFG_CCI_CLK_CTRL_OFF));
		/* L2C SRAM DCM */
		MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,
				 aor(reg_read(MCUCFG_L2C_SRAM_CTRL), ~MCUCFG_L2C_SRAM_CTRL_MASK,
				     MCUCFG_L2C_SRAM_CTRL_OFF));

		/* bus_fabric_dcm_ctrl */
		MCUSYS_SMC_WRITE(MCUCFG_BUS_FABRIC_DCM_CTRL,
				 aor(reg_read(MCUCFG_BUS_FABRIC_DCM_CTRL),
				     ~MCUCFG_BUS_FABRIC_DCM_CTRL_MASK,
				     MCUCFG_BUS_FABRIC_DCM_CTRL_OFF));
	} else {
		MCUSYS_SMC_WRITE(MCUCFG_CCI_CLK_CTRL,
				 aor(reg_read(MCUCFG_CCI_CLK_CTRL), ~MCUCFG_CCI_CLK_CTRL_MASK,
				     MCUCFG_CCI_CLK_CTRL_ON));
		/* L2C SRAM DCM */
		MCUSYS_SMC_WRITE(MCUCFG_L2C_SRAM_CTRL,
				 aor(reg_read(MCUCFG_L2C_SRAM_CTRL), ~MCUCFG_L2C_SRAM_CTRL_MASK,
				     MCUCFG_L2C_SRAM_CTRL_ON));

		/* bus_fabric_dcm_ctrl */
		MCUSYS_SMC_WRITE(MCUCFG_BUS_FABRIC_DCM_CTRL,
				 aor(reg_read(MCUCFG_BUS_FABRIC_DCM_CTRL),
				     ~MCUCFG_BUS_FABRIC_DCM_CTRL_MASK,
				     MCUCFG_BUS_FABRIC_DCM_CTRL_ON));
	}

	return 0;
}

/** 0x10210004	DCM_CFG
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
 **/
#define TOPCKG_DCM_CFG_MASK     ((0x1f<<0) | (1<<7) | (0x7f<<8) | (1<<15))
#define TOPCKG_DCM_CFG_ON       ((0<<0) | (1<<7) | (0<<8) | (0<<15))
#define TOPCKG_DCM_CFG_OFF      (0<<7)
/* Used for slow idle to enable or disable TOPCK DCM */
#define TOPCKG_DCM_CFG_QMASK     (1<<7)
#define TOPCKG_DCM_CFG_QON       (1<<7)
#define TOPCKG_DCM_CFG_QOFF      (0<<7)

#define TOPCKG_DCM_CFG_FMEM_MASK            ((0x1f<<16) | (1<<21) | (1<<22) \
					     | (1<<23) | (0x7f<<24) | (1<<31))
#define TOPCKG_DCM_CFG_FMEM_ON              ((0<<16) | (1<<21) | (0x1<<22) \
					     | (1<<23) | (0<<24) | (0<<31))
#define TOPCKG_DCM_CFG_FMEM_OFF             ((1<<21) | (0<<23))
/* toggle mem_dcm_cfg_latch since it's triggered by rising edge */
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_MASK     (1<<21)
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_CLEAR    (0<<21)
#define TOPCKG_DCM_CFG_FMEM_TOGGLE_ON       (1<<21)

/** TOPCKG_CLK_MISC_CFG_2
 * 7   0   mem_dcm_force_idle (0: does not force idle, 1: force idle to high)
 **/
#define TOPCKG_CLK_MISC_CFG_2_MASK     (0xf<<0)
#define TOPCKG_CLK_MISC_CFG_2_ON       (0xf<<0)
#define TOPCKG_CLK_MISC_CFG_2_OFF      (0x0<<0)

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

/** 0x10000050	INFRA_GLOBALCON_DCMCTL
 * 0	0	faxi_dcm_enable
 * 1	1	fmem_dcm_enable
 * 8	8	axi_clock_gated_en
 * 9	9	l2c_sram_infra_dcm_en
 **/
#define INFRA_GLOBALCON_DCMCTL_MASK     (0x00000303)
#define INFRA_GLOBALCON_DCMCTL_ON       (0x00000303)
#define INFRA_GLOBALCON_DCMCTL_OFF      (0x00000000)


/** 0x10000054	INFRA_GLOBALCON_DCMDBC
 * 6	0	dcm_dbc_cnt (default 7'h7F)
 * 8	8	faxi_dcm_dbc_enable
 * 22	16	dcm_dbc_cnt_fmem (default 7'h7F)
 * 24	24	dcm_dbc_enable_fmem
 **/
#define INFRA_GLOBALCON_DCMDBC_MASK  ((0x7f<<0) | (1<<8) | (0x7f<<16) | (1<<24))
#define INFRA_GLOBALCON_DCMDBC_ON      ((0<<0) | (1<<8) | (0<<16) | (1<<24))
#define INFRA_GLOBALCON_DCMDBC_OFF     INFRA_GLOBALCON_DCMDBC_ON	/* dont-care */


/** 0x10000058	INFRA_GLOBALCON_DCMFSEL
 * 2	0	dcm_qtr_fsel ("1xx: 1/4, 01x: 1/8, 001: 1/16, 000: 1/32")
 * 11	8	dcm_half_fsel ("1xxx:1/2, 01xx: 1/4, 001x: 1/8, 0001: 1/16, 0000: 1/32")
 * 20	16	dcm_full_fsel ("1xxxx:1/1, 01xxx:1/2, 001xx: 1/4, 0001x: 1/8, 00001: 1/16, 00000: 1/32")
 * 28	24	dcm_full_fsel_fmem ("1xxxx:1/1, 01xxx:1/2, 001xx: 1/4, 0001x: 1/8, 00001: 1/16, 00000: 1/32")
 **/
#define INFRA_GLOBALCON_DCMFSEL_MASK ((0x7<<0) | (0x7<<8) | (0x1f<<16) | (0x1f<<24))
#define INFRA_GLOBALCON_DCMFSEL_ON ((0<<0) | (0<<8) | (0x10<<16) | (0x10<<24))
#define INFRA_GLOBALCON_DCMFSEL_OFF (INFRA_GLOBALCON_DCMFSEL_ON)	/* dont-care */


typedef enum {
	TOPCKG_DCM_OFF = DCM_OFF,
	TOPCKG_DCM_ON = DCM_ON,
} ENUM_TOPCKG_DCM;

typedef enum {
	INFRA_DCM_OFF = DCM_OFF,
	INFRA_DCM_ON = DCM_ON,
} ENUM_INFRA_DCM;


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

/* cnt : 0~0x7f */
int dcm_infra_dbc(int on, int cnt)
{
	int value;

	cnt &= 0x7f;
	on = (on != 0) ? 1 : 0;
	value = (cnt << 0) | (cnt << 16) | (on << 8) | (on << 24);

	reg_write(INFRA_GLOBALCON_DCMDBC, aor(reg_read(INFRA_GLOBALCON_DCMDBC),
					      ~INFRA_GLOBALCON_DCMDBC_MASK, value));

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
int dcm_infra_rate(unsigned fmem, unsigned int full, unsigned int half, unsigned int quarter)
{

	BUG_ON(half < 1);
	BUG_ON(quarter < 2);

	fmem = 0x10 >> fmem;
	full = 0x10 >> full;
	half = 0x10 >> half;
	quarter = 0x10 >> quarter;

	reg_write(INFRA_GLOBALCON_DCMFSEL, aor(reg_read(INFRA_GLOBALCON_DCMFSEL),
					       ~INFRA_GLOBALCON_DCMFSEL_MASK,
					       (fmem << 24) | (full << 16) | (half << 8) | (quarter
											    << 0)));

	return 0;
}

int dcm_infra(ENUM_INFRA_DCM on)
{

	ASSERT_INFRA_DCMCTL();

	if (on) {
#if 1				/* override the dbc and fsel setting !! */
		reg_write(INFRA_GLOBALCON_DCMDBC,
			  aor(reg_read(INFRA_GLOBALCON_DCMDBC), ~INFRA_GLOBALCON_DCMDBC_MASK,
			      INFRA_GLOBALCON_DCMDBC_ON));
		reg_write(INFRA_GLOBALCON_DCMFSEL,
			  aor(reg_read(INFRA_GLOBALCON_DCMFSEL), ~INFRA_GLOBALCON_DCMFSEL_MASK,
			      INFRA_GLOBALCON_DCMFSEL_ON));
#endif
		reg_write(INFRA_GLOBALCON_DCMCTL,
			  aor(reg_read(INFRA_GLOBALCON_DCMCTL), ~INFRA_GLOBALCON_DCMCTL_MASK,
			      INFRA_GLOBALCON_DCMCTL_ON));
	} else {
		reg_write(INFRA_GLOBALCON_DCMCTL,
			  aor(reg_read(INFRA_GLOBALCON_DCMCTL), ~INFRA_GLOBALCON_DCMCTL_MASK,
			      INFRA_GLOBALCON_DCMCTL_OFF));
	}

	return 0;
}


/** 0x10002050	PERI_GLOBALCON_DCMCTL
 * 0	0	DCM_ENABLE
 * 1	1	AXI_CLOCK_GATED_EN
 * 7	4	AHB_BUS_SLP_REQ
 * 12	8	DCM_IDLE_BYPASS_EN
 **/

/** 0x10002054	PERI_GLOBALCON_DCMDBC
 * 7	7	DCM_DBC_ENABLE
 * 6	0	DCM_DBC_CNT
 **/

/** 0x10002058	PERI_GLOBALCON_DCMFSEL
 * 20	16	DCM_FULL_FSEL
 * 11	8	DCM_HALF_FSEL
 * 2	0	DCM_QTR_FSEL
 **/

#define PERI_GLOBALCON_DCMCTL_MASK  ((1<<0) | (1<<1) | (0xf<<4) | (0xf<<8))
#define PERI_GLOBALCON_DCMCTL_ON  ((1<<0) | (1<<1) | (0xf<<4) | (0x0<<8))
#define PERI_GLOBALCON_DCMCTL_OFF  ((0<<0) | (0<<1) | (0x0<<4) | (0x0<<8))

#define PERI_GLOBALCON_DCMDBC_MASK  ((0x7f<<0) | (1<<7))
#define PERI_GLOBALCON_DCMDBC_ON  ((0x70<<0) | (1<<7))
#define PERI_GLOBALCON_DCMDBC_OFF  ((0x70<<0) | (0<<7))

#define PERI_GLOBALCON_DCMFSEL_MASK  ((0x7<<0) | (0xf<<8) | (0x1f<<16))
#define PERI_GLOBALCON_DCMFSEL_ON  ((0<<0) | (0<<8) | (0x0<<16))
#define PERI_GLOBALCON_DCMFSEL_OFF  ((0<<0) | (0<<8) | (0x0<<16))


typedef enum {
	PERI_DCM_OFF = DCM_OFF,
	PERI_DCM_ON = DCM_ON,
} ENUM_PERI_DCM;


/* cnt: 0~0x7f */
int dcm_peri_dbc(int on, int cnt)
{

	BUG_ON(cnt > 0x7f);

	on = (on != 0) ? 1 : 0;

	reg_write(PERI_GLOBALCON_DCMDBC,
		  aor(reg_read(PERI_GLOBALCON_DCMDBC),
		      ~PERI_GLOBALCON_DCMDBC_MASK, ((cnt << 0) | (on << 7))));

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
int dcm_peri_rate(unsigned int full, unsigned int half, unsigned int quarter)
{

	BUG_ON(half < 1);
	BUG_ON(quarter < 2);

	full = 0x10 >> full;
	half = 0x10 >> half;
	quarter = 0x10 >> quarter;

	reg_write(PERI_GLOBALCON_DCMFSEL, aor(reg_read(PERI_GLOBALCON_DCMFSEL),
					      ~PERI_GLOBALCON_DCMFSEL_MASK,
					      (full << 16) | (half << 8) | (quarter << 0)));

	return 0;
}


int dcm_peri(ENUM_PERI_DCM on)
{

	if (on) {
#if 1				/* override the dbc and fsel setting !! */
		reg_write(PERI_GLOBALCON_DCMDBC,
			  aor(reg_read(PERI_GLOBALCON_DCMDBC), ~PERI_GLOBALCON_DCMDBC_MASK,
			      PERI_GLOBALCON_DCMDBC_ON));
		reg_write(PERI_GLOBALCON_DCMFSEL,
			  aor(reg_read(PERI_GLOBALCON_DCMFSEL), ~PERI_GLOBALCON_DCMFSEL_MASK,
			      PERI_GLOBALCON_DCMFSEL_ON));
#endif
		reg_write(PERI_GLOBALCON_DCMCTL, aor(reg_read(PERI_GLOBALCON_DCMCTL),
						     ~PERI_GLOBALCON_DCMCTL_MASK,
						     PERI_GLOBALCON_DCMCTL_ON));
	} else {
#if 1				/* override the dbc and fsel setting !! */
		reg_write(PERI_GLOBALCON_DCMDBC,
			  aor(reg_read(PERI_GLOBALCON_DCMDBC), ~PERI_GLOBALCON_DCMDBC_MASK,
			      PERI_GLOBALCON_DCMDBC_OFF));
		reg_write(PERI_GLOBALCON_DCMFSEL,
			  aor(reg_read(PERI_GLOBALCON_DCMFSEL), ~PERI_GLOBALCON_DCMFSEL_MASK,
			      PERI_GLOBALCON_DCMFSEL_OFF));
#endif
		reg_write(PERI_GLOBALCON_DCMCTL, aor(reg_read(PERI_GLOBALCON_DCMCTL),
						     ~PERI_GLOBALCON_DCMCTL_MASK,
						     PERI_GLOBALCON_DCMCTL_OFF));
	}

	return 0;
}


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

/** 0x10203060	EMI_CONM
 * 31	24	EMI_DCM_DIS
 **/
typedef enum {
	EMI_DCM_OFF = DCM_OFF,
	EMI_DCM_ON = DCM_ON,
} ENUM_EMI_DCM;

int dcm_emi(ENUM_EMI_DCM on)
{
	if (on)
		reg_write(EMI_CONM, aor(reg_read(EMI_CONM), ~(0xff << 24), (0 << 24)));
	else
		reg_write(EMI_CONM, aor(reg_read(EMI_CONM), ~(0xff << 24), (0xff << 24)));

	return 0;
}

#if defined(CONFIG_ARCH_MT6753)
/** 0x10213640	DDRPHY_MEMPLL_DIVIDER
 * 15	15	R_COMB_M_CK_CG_EN -> 0:Disable, 1: Enable
 * 17	17	R_MEMPLL3_CKOUT_CG_EN
 *		...
 * 31	31	R_D_MIO_CK_CG_EN
 **/

#define DDRPHY_MEMPLL_DIVIDER_MASK  ((1<<15) | (0x7ff<<17))
#define DDRPHY_MEMPLL_DIVIDER_ON  ((1<<15) | (0x7ff<<17))
#define DDRPHY_MEMPLL_DIVIDER_OFF  ((0<<15) | (0x0<<17))

typedef enum {
	DDRPHY_DCM_OFF = DCM_OFF,
	DDRPHY_DCM_ON = DCM_ON,
} ENUM_DDRPHY_DCM;

int dcm_ddrphy(ENUM_DDRPHY_DCM on)
{
	if (on) {
		reg_write(DDRPHY_MEMPLL_DIVIDER,
			  aor(reg_read(DDRPHY_MEMPLL_DIVIDER),
			      ~DDRPHY_MEMPLL_DIVIDER_MASK, DDRPHY_MEMPLL_DIVIDER_ON));
	} else {
		reg_write(DDRPHY_MEMPLL_DIVIDER,
			  aor(reg_read(DDRPHY_MEMPLL_DIVIDER),
			      ~DDRPHY_MEMPLL_DIVIDER_MASK, DDRPHY_MEMPLL_DIVIDER_OFF));
	}

	return 0;
}
#endif				/* CONFIG_ARCH_MT6753 */

/** 0x1020655c	EFUSE_REG_DCM_ON
 * 0	0	DCM_ON
 **/

typedef enum {
	EFUSE_DCM_OFF = DCM_OFF,
	EFUSE_DCM_ON = DCM_ON,
} ENUM_EFUSE_DCM;

int dcm_efuse(ENUM_EFUSE_DCM on)
{
	if (on)
		reg_write(EFUSE_REG_DCM_ON, aor(reg_read(EFUSE_REG_DCM_ON), ~(1 << 0), (1 << 0)));
	else
		reg_write(EFUSE_REG_DCM_ON, aor(reg_read(EFUSE_REG_DCM_ON), ~(1 << 0), (0 << 0)));

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
	TOPCKG_DCM,
	EFUSE_DCM,
#if defined(CONFIG_ARCH_MT6753)
	DDRPHY_DCM,

	NR_DCM = 9,
#else				/* !CONFIG_ARCH_MT6753 */
	NR_DCM = 8,
#endif				/* CONFIG_ARCH_MT6753 */
};

enum {
	ARMCORE_DCM_TYPE = (1U << 0),
	MCUSYS_DCM_TYPE = (1U << 1),
	INFRA_DCM_TYPE = (1U << 2),
	PERI_DCM_TYPE = (1U << 3),
	EMI_DCM_TYPE = (1U << 4),
	DRAMC_DCM_TYPE = (1U << 5),
	TOPCKG_DCM_TYPE = (1U << 6),
	EFUSE_DCM_TYPE = (1U << 7),
#if defined(CONFIG_ARCH_MT6753)
	DDRPHY_DCM_TYPE = (1U << 8),

	NR_DCM_TYPE = 9,
#else				/* !CONFIG_ARCH_MT6753 */
	NR_DCM_TYPE = 8,
#endif				/* CONFIG_ARCH_MT6753 */
};

/* Do not do infra DCM ON/OFF here due to H/W limitation. Do it on preloader
 * instead */
#if defined(CONFIG_ARCH_MT6753)
#define ALL_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE | \
		       TOPCKG_DCM_TYPE | EFUSE_DCM_TYPE | DDRPHY_DCM_TYPE)

#define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | /*INFRA_DCM_TYPE |*/ \
		       PERI_DCM_TYPE | /* EMI_DCM_TYPE | DRAMC_DCM_TYPE |*/ \
		       TOPCKG_DCM_TYPE | EFUSE_DCM_TYPE /*| DDRPHY_DCM_TYPE */)
#else				/* !CONFIG_ARCH_MT6753 */
#define ALL_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE | \
		       TOPCKG_DCM_TYPE | EFUSE_DCM_TYPE)

#define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | /*INFRA_DCM_TYPE |*/ \
		       PERI_DCM_TYPE | /* EMI_DCM_TYPE | DRAMC_DCM_TYPE |*/ \
		       TOPCKG_DCM_TYPE | EFUSE_DCM_TYPE)
#endif				/* CONFIG_ARCH_MT6753 */


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
	 .typeid = DRAMC_DCM_TYPE,
	 .name = "DRAMC_DCM",
	 .func = (DCM_FUNC) dcm_dramc_ao,
	 .current_state = DRAMC_AO_DCM_ON,
	 .default_state = DRAMC_AO_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = TOPCKG_DCM_TYPE,
	 .name = "TOPCKG_DCM",
	 .func = (DCM_FUNC) dcm_topckg,
	 .current_state = TOPCKG_DCM_ON,
	 .default_state = TOPCKG_DCM_ON,
	 .disable_refcnt = 0,
	 },
	{
	 .typeid = EFUSE_DCM_TYPE,
	 .name = "EFUSE_DCM",
	 .func = (DCM_FUNC) dcm_efuse,
	 .current_state = EFUSE_DCM_ON,
	 .default_state = EFUSE_DCM_ON,
	 .disable_refcnt = 0,
	 },
#if defined(CONFIG_ARCH_MT6753)
	{
	 .typeid = DDRPHY_DCM_TYPE,
	 .name = "DDRPHY_DCM",
	 .func = (DCM_FUNC) dcm_ddrphy,
	 .current_state = DDRPHY_DCM_ON,
	 .default_state = DDRPHY_DCM_ON,
	 .disable_refcnt = 0,
	 },
#endif				/* CONFIG_ARCH_MT6753 */
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

void dcm_dump_regs(void)
{
	dcm_info("\n******** dcm dump register *********\n");
	REG_DUMP(APMIXED_PLL_CON2);
	REG_DUMP(MCUCFG_ACLKEN_DIV);
	REG_DUMP(MCUCFG_L2C_SRAM_CTRL);
	REG_DUMP(MCUCFG_CCI_CLK_CTRL);
	REG_DUMP(MCUCFG_BUS_FABRIC_DCM_CTRL);
	REG_DUMP(TOP_CKMUXSEL);
	REG_DUMP(TOP_CKDIV1);
	REG_DUMP(INFRA_TOPCKGEN_DCMCTL);
	REG_DUMP(INFRA_TOPCKGEN_DCMDBC);
	REG_DUMP(INFRA_GLOBALCON_DCMCTL);
	REG_DUMP(INFRA_GLOBALCON_DCMDBC);
	REG_DUMP(INFRA_GLOBALCON_DCMFSEL);
	REG_DUMP(TOPCKG_DCM_CFG);
	REG_DUMP(PERI_GLOBALCON_DCMCTL);
	REG_DUMP(PERI_GLOBALCON_DCMDBC);
	REG_DUMP(PERI_GLOBALCON_DCMFSEL);
	REG_DUMP(DRAMC_PD_CTRL);
	REG_DUMP(DRAMC_CLKCTRL);
	REG_DUMP(DRAMC_PERFCTL0);
	REG_DUMP(DRAMC_GDDR3CTL1);
	REG_DUMP(DDRPHY_MEMPLL_DIVIDER);
	REG_DUMP(EMI_CONM);
	REG_DUMP(EFUSE_REG_DCM_ON);
}


#if defined(CONFIG_PM)
static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int len = 0;
	char *p = buf;
	int i;
	DCM *dcm;

	/* dcm_dump_state(ALL_DCM_TYPE); */
	p += sprintf(p, "\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++)
		p += sprintf(p, "[%-16s 0x%08x] current state:%d (%d)\n",
			     dcm->name, dcm->typeid, dcm->current_state, dcm->disable_refcnt);

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

	/* pericfg */
	node = of_find_compatible_node(NULL, NULL, PERICFG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " PERICFG_NODE);
		BUG();
	}
	pericfg_base = (unsigned long)of_iomap(node, 0);
	if (!pericfg_base) {
		dcm_info("error: cannot iomap " PERICFG_NODE);
		BUG();
	}

	/* efuse */
	node = of_find_compatible_node(NULL, NULL, EFUSE_NODE);
	if (!node) {
		dcm_info("error: cannot find node " EFUSE_NODE);
		BUG();
	}
	efuse_base = (unsigned long)of_iomap(node, 0);
	if (!efuse_base) {
		dcm_info("error: cannot iomap " EFUSE_NODE);
		BUG();
	}

	/* apmixed */
	node = of_find_compatible_node(NULL, NULL, APMIXED_NODE);
	if (!node) {
		dcm_info("error: cannot find node " APMIXED_NODE);
		BUG();
	}
	apmixed_base = (unsigned long)of_iomap(node, 0);
	if (!apmixed_base) {
		dcm_info("error: cannot iomap " APMIXED_NODE);
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
	/* default TOPCKG DCM OFF for I2C performance but keep others setting as DCM ON */
	mt_dcm_topckg_disable();
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

/* mt_dcm_topckg_disable/enable is used for slow idle */
void mt_dcm_topckg_disable(void)
{
#if !defined(DCM_DEFAULT_ALL_OFF)
	reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
			~TOPCKG_DCM_CFG_QMASK, TOPCKG_DCM_CFG_QOFF));
#endif /* #if !defined (DCM_DEFAULT_ALL_OFF) */
}
EXPORT_SYMBOL(mt_dcm_topckg_disable);

/* mt_dcm_topckg_disable/enable is used for slow idle */
void mt_dcm_topckg_enable(void)
{
#if !defined(DCM_DEFAULT_ALL_OFF)
	if (dcm_array[TOPCKG_DCM].current_state != DCM_OFF) {
		reg_write(TOPCKG_DCM_CFG, aor(reg_read(TOPCKG_DCM_CFG),
				~TOPCKG_DCM_CFG_QMASK, TOPCKG_DCM_CFG_QON));
	}
#endif /* #if !defined (DCM_DEFAULT_ALL_OFF) */
}
EXPORT_SYMBOL(mt_dcm_topckg_enable);

void mt_dcm_topck_off(void)
{
	mt_dcm_init();
	dcm_set_state(TOPCKG_DCM_TYPE, DCM_OFF);
}
EXPORT_SYMBOL(mt_dcm_topck_off);

void mt_dcm_topck_on(void)
{
	mt_dcm_init();
	dcm_set_state(TOPCKG_DCM_TYPE, DCM_ON);
}
EXPORT_SYMBOL(mt_dcm_topck_on);

void mt_dcm_peri_off(void)
{
	mt_dcm_init();
	dcm_set_state(PERI_DCM_TYPE, DCM_OFF);
}
EXPORT_SYMBOL(mt_dcm_peri_off);

void mt_dcm_peri_on(void)
{
	mt_dcm_init();
	dcm_set_state(PERI_DCM_TYPE, DCM_ON);
}
EXPORT_SYMBOL(mt_dcm_peri_on);

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
#include <linux/delay.h>
#include <asm/bug.h>
#include <mt-plat/mt_io.h>
#include <mt-plat/sync_write.h>
#include "mt_dcm.h"
#if 0
#include <mach/mt_secure_api.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt_dramc.h>
#include "mt_freqhopping_drv.h"


/* #define DCM_DEFAULT_ALL_OFF */
/* #define NON_AO_MP2_DCM_CONFIG */

#if defined(CONFIG_OF)
static unsigned long mcucfg_base;
static unsigned long mcucfg_phys_base;
static unsigned long mcucfg2_base;
static unsigned long mcucfg2_phys_base;
static unsigned long emi_reg_base;
static unsigned long dramc_conf_base;
static unsigned long dramc_conf_b_base;
static unsigned long infracfg_ao_base;
/*static unsigned long mcumixed_base;*/

#define MCUCFG_NODE			"mediatek,mcucfg"
#define MCUCFG2_NODE		"mediatek,mcucfg2"
#define INFRACFG_AO_NODE	"mediatek,infracfg_ao"
#define EMI_REG_NODE		"mediatek,emi"
#define DRAMC_CONFIG_NODE	"mediatek,dramc"
#define DRAMC_CONFIG_B_NODE "mediatek,dramc_conf_b"
#define MCUMIXED_REG_NODE	"mediatek,mcumixed"

#undef MCUCFG_BASE
#undef INFRACFG_AO_BASE
#undef EMI_REG_BASE
#undef DRAMC_CONFIG_BASE
#undef DRAMC_CONFIG_B_BASE
#undef MCUMIXED_BASE
#define MCUCFG_BASE			(mcucfg_base)
#define MCUCFG2_BASE		(mcucfg2_base)
#define INFRACFG_AO_BASE	(infracfg_ao_base)
#define EMI_REG_BASE	    (emi_reg_base)
#define DRAMC_CONFIG_BASE	(dramc_conf_base)
#define DRAMC_CONFIG_B_BASE	(dramc_conf_b_base)
/*#define MCUMIXED_BASE		(mcumixed_base)*/
#else	/* !defined(CONFIG_OF) */
#error "undefined CONFIG_OF"
#endif	/* #if defined(CONFIG_OF) */

/* MCUCFG registers */
#define MCUCFG_MP0_AXI_CONFIG			(MCUCFG_BASE + 0x02C) /* 0x1022002C */
#define MCUCFG_MP1_AXI_CONFIG			(MCUCFG_BASE + 0x22C) /* 0x1022022C */
#define MCUCFG_L2C_SRAM_CTRL			(MCUCFG_BASE + 0x648) /* 0x10220648 */
#define MCUCFG_CCI_CLK_CTRL				(MCUCFG_BASE + 0x660) /* 0x10220660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL		(MCUCFG_BASE + 0x668) /* 0x10220668 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG	(MCUCFG_BASE + 0x740) /* 0x10220740 */
#define MCUCFG_SYNC_DCM_CONFIG			(MCUCFG_BASE + 0x744) /* 0x10220744 */
#define MCUCFG_SYNC_DCM_CLUSTER_CONFIG	(MCUCFG_BASE + 0x74C) /* 0x1022074C */

#define MCUCFG_MP0_AXI_CONFIG_PHYS			(mcucfg_phys_base + 0x02C) /* 0x1022002C */
#define MCUCFG_L2C_SRAM_CTRL_PHYS			(mcucfg_phys_base + 0x648) /* 0x10220648 */
#define MCUCFG_CCI_CLK_CTRL_PHYS			(mcucfg_phys_base + 0x660) /* 0x10220660 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL_PHYS		(mcucfg_phys_base + 0x668) /* 0x10220668 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG_PHYS	(mcucfg_phys_base + 0x740) /* 0x10220740 */
#define MCUCFG_SYNC_DCM_CONFIG_PHYS			(mcucfg_phys_base + 0x744) /* 0x10220744 */
#define MCUCFG_SYNC_DCM_CLUSTER_CONFIG_PHYS	(mcucfg_phys_base + 0x74C) /* 0x1022074C */

/* MCUCFG2 registers */
#define MCUCFG_SYNC_DCM_MP2_CONFIG		(MCUCFG2_BASE + 0x274)		/* 0x10222274 */
#define MCUCFG_SYNC_DCM_MP2_CONFIG_PHYS	(mcucfg2_phys_base + 0x274) /* 0x10222274 */

/* INFRASYS_AO */
#define TOP_CKMUXSEL				(INFRACFG_AO_BASE + 0x000) /* 0x10001000 */
#define INFRA_TOPCKGEN_CKDIV1_BIG	(INFRACFG_AO_BASE + 0x024) /* 0x10001024 */
#define INFRA_TOPCKGEN_CKDIV1_SML	(INFRACFG_AO_BASE + 0x028) /* 0x10001028 */
#define INFRA_TOPCKGEN_CKDIV1_BUS	(INFRACFG_AO_BASE + 0x02C) /* 0x1000102C */
#define TOP_DCMCTL					(INFRACFG_AO_BASE + 0x010) /* 0x10001010 */
#define	INFRA_BUS_DCM_CTRL			(INFRACFG_AO_BASE + 0x070) /* 0x10001070 */
#define	PERI_BUS_DCM_CTRL			(INFRACFG_AO_BASE + 0x074) /* 0x10001074 */
#define MEM_DCM_CTRL				(INFRACFG_AO_BASE + 0x078) /* 0x10001078 */
#define DFS_MEM_DCM_CTRL			(INFRACFG_AO_BASE + 0x07C) /* 0x1000107C */
#define	P2P_RX_CLK_ON				(INFRACFG_AO_BASE + 0x0a0) /* 0x100010a0 */
#define	INFRA_MISC					(INFRACFG_AO_BASE + 0xf00) /* 0x10001f00 */
#define	INFRA_MISC_1				(INFRACFG_AO_BASE + 0xf0c) /* 0x10001f0c */
#define INFRA_MISC_2				(INFRACFG_AO_BASE + 0xf18) /* 0x10001f18 */

/* DRAMC_AO */
#define DRAMC_PD_CTRL   (DRAMC_CONFIG_BASE + 0x1dc)	/* 0x102141dc */
#define DRAMC_PERFCTL0  (DRAMC_CONFIG_BASE + 0x1ec)	/* 0x102141ec */

/* EMI */
#define EMI_CONM	(EMI_REG_BASE + 0x060)	/* 0x10203060 */

/* DRAMC_B_AO */
#define DRAMC_B_PD_CTRL   (DRAMC_CONFIG_B_BASE + 0x1dc)	/* 0x102141dc */
#define DRAMC_B_PERFCTL0  (DRAMC_CONFIG_B_BASE + 0x1ec)	/* 0x102141ec */

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

#define REG_DUMP(addr) dcm_info("%-30s(0x%08lX): 0x%08X\n", #addr, addr, reg_read(addr))

/*#define DCM_DEBUG*/

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
 *  dcm_armcore
 */
#define ARMPLLDIV_DCMCTL	(/*MCUMIXED_BASE +*/ 0x278)

#define ARMPLL_DCM_ENABLE_0_MASK		(0x1<<0)
#define ARMPLL_DCM_WFI_ENABLE_0_MASK	(0x1<<1)
#define ARMPLL_DCM_WFE_ENABLE_0_MASK	(0x1<<2)
#define ARMPLL_DCM_ENABLE_1_MASK		(0x1<<4)
#define ARMPLL_DCM_WFI_ENABLE_1_MASK	(0x1<<5)
#define ARMPLL_DCM_WFE_ENABLE_1_MASK	(0x1<<6)
#define ARMPLL_DCM_ENABLE_2_MASK		(0x1<<8)
#define ARMPLL_DCM_WFI_ENABLE_2_MASK	(0x1<<9)
#define ARMPLL_DCM_WFE_ENABLE_2_MASK	(0x1<<10)
#define ARMPLL_DCM_ENABLE_3_MASK		(0x1<<12)
#define ARMPLL_DCM_WFI_ENABLE_3_MASK	(0x1<<13)
#define ARMPLL_DCM_WFE_ENABLE_3_MASK	(0x1<<14)

#define ARMPLL_DCM_ENABLE_0_ON			(0x1<<0)
#define ARMPLL_DCM_WFI_ENABLE_0_ON		(0x1<<1)
#define ARMPLL_DCM_WFE_ENABLE_0_ON		(0x1<<2)
#define ARMPLL_DCM_ENABLE_1_ON			(0x1<<4)
#define ARMPLL_DCM_WFI_ENABLE_1_ON		(0x1<<5)
#define ARMPLL_DCM_WFE_ENABLE_1_ON		(0x1<<6)
#define ARMPLL_DCM_ENABLE_2_ON			(0x1<<8)
#define ARMPLL_DCM_WFI_ENABLE_2_ON		(0x1<<9)
#define ARMPLL_DCM_WFE_ENABLE_2_ON		(0x1<<10)
#define ARMPLL_DCM_ENABLE_3_ON			(0x1<<12)
#define ARMPLL_DCM_WFI_ENABLE_3_ON		(0x1<<13)
#define ARMPLL_DCM_WFE_ENABLE_3_ON		(0x1<<14)

#define ARMPLL_DCM_ENABLE_0_OFF			(0x0<<0)
#define ARMPLL_DCM_WFI_ENABLE_0_OFF		(0x0<<1)
#define ARMPLL_DCM_WFE_ENABLE_0_OFF		(0x0<<2)
#define ARMPLL_DCM_ENABLE_1_OFF			(0x0<<4)
#define ARMPLL_DCM_WFI_ENABLE_1_OFF		(0x0<<5)
#define ARMPLL_DCM_WFE_ENABLE_1_OFF		(0x0<<6)
#define ARMPLL_DCM_ENABLE_2_OFF			(0x0<<8)
#define ARMPLL_DCM_WFI_ENABLE_2_OFF		(0x0<<9)
#define ARMPLL_DCM_WFE_ENABLE_2_OFF		(0x0<<10)
#define ARMPLL_DCM_ENABLE_3_OFF			(0x0<<12)
#define ARMPLL_DCM_WFI_ENABLE_3_OFF		(0x0<<13)
#define ARMPLL_DCM_WFE_ENABLE_3_OFF		(0x0<<14)


typedef enum {
	ARMCORE_DCM_OFF = DCM_OFF,
	ARMCORE_DCM_MODE1 = DCM_ON,
	ARMCORE_DCM_MODE2 = DCM_ON+1,
} ENUM_ARMCORE_DCM;

int dcm_armcore(ENUM_ARMCORE_DCM mode)
{
	dcm_info("%s(%d)\n", __func__, mode);

	if (mode == ARMCORE_DCM_OFF) {
		mt6757_0x1001AXXX_reg_write(ARMPLLDIV_DCMCTL,
			  aor(mt6757_0x1001AXXX_reg_read(ARMPLLDIV_DCMCTL),
					~(ARMPLL_DCM_ENABLE_0_MASK |
					ARMPLL_DCM_WFI_ENABLE_0_MASK |
					ARMPLL_DCM_WFE_ENABLE_0_MASK |
					ARMPLL_DCM_ENABLE_1_MASK |
					ARMPLL_DCM_WFI_ENABLE_1_MASK |
					ARMPLL_DCM_WFE_ENABLE_1_MASK |
					ARMPLL_DCM_ENABLE_2_MASK |
					ARMPLL_DCM_WFI_ENABLE_2_MASK |
					ARMPLL_DCM_WFE_ENABLE_2_MASK |
					ARMPLL_DCM_ENABLE_3_MASK |
					ARMPLL_DCM_WFI_ENABLE_3_MASK |
					ARMPLL_DCM_WFE_ENABLE_3_MASK),
					(ARMPLL_DCM_ENABLE_0_OFF |
					ARMPLL_DCM_WFI_ENABLE_0_OFF |
					ARMPLL_DCM_WFE_ENABLE_0_OFF |
					ARMPLL_DCM_ENABLE_1_OFF |
					ARMPLL_DCM_WFI_ENABLE_1_OFF |
					ARMPLL_DCM_WFE_ENABLE_1_OFF |
					ARMPLL_DCM_ENABLE_2_OFF |
					ARMPLL_DCM_WFI_ENABLE_2_OFF |
					ARMPLL_DCM_WFE_ENABLE_2_OFF |
					ARMPLL_DCM_ENABLE_3_OFF |
					ARMPLL_DCM_WFI_ENABLE_3_OFF |
					ARMPLL_DCM_WFE_ENABLE_3_OFF)));
	} else if (mode == ARMCORE_DCM_MODE1) {
		mt6757_0x1001AXXX_reg_write(ARMPLLDIV_DCMCTL,
			  aor(mt6757_0x1001AXXX_reg_read(ARMPLLDIV_DCMCTL),
			      ~(ARMPLL_DCM_ENABLE_0_MASK |
					ARMPLL_DCM_WFI_ENABLE_0_MASK |
					ARMPLL_DCM_WFE_ENABLE_0_MASK |
					ARMPLL_DCM_ENABLE_1_MASK |
					ARMPLL_DCM_WFI_ENABLE_1_MASK |
					ARMPLL_DCM_WFE_ENABLE_1_MASK |
					ARMPLL_DCM_ENABLE_2_MASK |
					ARMPLL_DCM_WFI_ENABLE_2_MASK |
					ARMPLL_DCM_WFE_ENABLE_2_MASK |
					ARMPLL_DCM_ENABLE_3_MASK |
					ARMPLL_DCM_WFI_ENABLE_3_MASK |
					ARMPLL_DCM_WFE_ENABLE_3_MASK),
			      (ARMPLL_DCM_ENABLE_0_ON |
					ARMPLL_DCM_WFI_ENABLE_0_OFF |
					ARMPLL_DCM_WFE_ENABLE_0_OFF |
					ARMPLL_DCM_ENABLE_1_ON |
					ARMPLL_DCM_WFI_ENABLE_1_OFF |
					ARMPLL_DCM_WFE_ENABLE_1_OFF |
					ARMPLL_DCM_ENABLE_2_ON |
					ARMPLL_DCM_WFI_ENABLE_2_OFF |
					ARMPLL_DCM_WFE_ENABLE_2_OFF |
					ARMPLL_DCM_ENABLE_3_ON |
					ARMPLL_DCM_WFI_ENABLE_3_OFF |
					ARMPLL_DCM_WFE_ENABLE_3_OFF)));
	} else if (mode == ARMCORE_DCM_MODE2) {
		mt6757_0x1001AXXX_reg_write(ARMPLLDIV_DCMCTL,
			  aor(mt6757_0x1001AXXX_reg_read(ARMPLLDIV_DCMCTL),
			      ~(ARMPLL_DCM_ENABLE_0_MASK |
					ARMPLL_DCM_WFI_ENABLE_0_MASK |
					ARMPLL_DCM_WFE_ENABLE_0_MASK |
					ARMPLL_DCM_ENABLE_1_MASK |
					ARMPLL_DCM_WFI_ENABLE_1_MASK |
					ARMPLL_DCM_WFE_ENABLE_1_MASK |
					ARMPLL_DCM_ENABLE_2_MASK |
					ARMPLL_DCM_WFI_ENABLE_2_MASK |
					ARMPLL_DCM_WFE_ENABLE_2_MASK |
					ARMPLL_DCM_ENABLE_3_MASK |
					ARMPLL_DCM_WFI_ENABLE_3_MASK |
					ARMPLL_DCM_WFE_ENABLE_3_MASK),
			      (ARMPLL_DCM_ENABLE_0_OFF |
					ARMPLL_DCM_WFI_ENABLE_0_ON |
					ARMPLL_DCM_WFE_ENABLE_0_ON |
					ARMPLL_DCM_ENABLE_1_OFF |
					ARMPLL_DCM_WFI_ENABLE_1_ON |
					ARMPLL_DCM_WFE_ENABLE_1_ON |
					ARMPLL_DCM_ENABLE_2_OFF |
					ARMPLL_DCM_WFI_ENABLE_2_ON |
					ARMPLL_DCM_WFE_ENABLE_2_ON |
					ARMPLL_DCM_ENABLE_3_OFF |
					ARMPLL_DCM_WFI_ENABLE_3_ON |
					ARMPLL_DCM_WFE_ENABLE_3_ON)));
	}

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
#define INFRA_BUS_DCM_CTRL_MASK	((0x3 << 0) | (0x7 << 2) | (0x1F << 5) | (0x1F << 10) | \
								 (0x1F << 15) | (0x1 << 20) | (0x1 << 21) | (0x1 << 30))
#define INFRA_BUS_DCM_CTRL_EN	((0x3 << 0) | (0x0 << 2) | (0x10 << 5) | (0x01 << 10) | \
								 (0x10 << 15) | (0x1 << 20) | (0x1 << 21) | (0x1 << 30))
#define INFRA_BUS_DCM_CTRL_DIS	((0x0 << 0) | (0x0 << 2) | (0x10 << 5) | (0x10 << 10) | \
								 (0x10 << 15) | (0x0 << 20) | (0x1 << 21) | (0x0 << 30))
#define INFRA_BUS_DCM_CTRL_SEL_MASK	((0x1F << 5) | (0x1F << 10) | (0x1F << 15))
#define INFRA_BUS_DCM_CTRL_SEL_EN	((0x10 << 5) | (0x01 << 10) | (0x10 << 15))
#define INFRA_BUS_DCM_CTRL_DBC_MASK	(0x1F << 15)

/*
 * 0x100010a0	P2P_RX_CLK_ON
 * 3	0	p2p_rx_clk_force_on	4'b1111	"0: Dies not force, 1: Force"
 */
#define P2P_RX_CLK_ON_MASK	(0xF<<0)
#define P2P_RX_CLK_ON_EN	(0x0<<0)
#define P2P_RX_CLK_ON_DIS	(0xF<<0)

/*
 * 0x10001F18	INFRA_MISC_2	32	Infrasys Miscellaneous Control Register
 * 22	22	md32_apb_rx_cg_mask
 * 23	23	dvfs_proc0_apb_rx_cg_mask
 * 24	24	dvfs_proc1_apb_rx_cg_mask
 * 25	25	pmic_apb_rx_cg_mask
 * 26	26	spm_apb_rx_cg_mask
 * 27	27	emi_mpu_apb_rx_cg_mask
 * 28	28	emi_apb_rx_cg_mask
 * 30	30	mmsys_apb_idle_mask
 * 31	31	mmsys_cg_gated_mask
 */
#define INFRA_MISC_2_MASK	(0x7F<<22 | 0x3<<30)
#define INFRA_MISC_2_EN		(0x0<<22 | 0x0<<30)
#define INFRA_MISC_2_DIS	(0x7F<<22 | 0x3<<30)

typedef enum {
	INFRA_DCM_OFF = DCM_OFF,
	INFRA_DCM_ON = DCM_ON,
} ENUM_INFRA_DCM;

int dcm_infra(ENUM_INFRA_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == INFRA_DCM_ON) {
		reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
						 ~INFRA_BUS_DCM_CTRL_MASK,
						 INFRA_BUS_DCM_CTRL_EN));

#if 0 /* workaround to avoid access dram reg fail */
		reg_write(P2P_RX_CLK_ON, aor(reg_read(P2P_RX_CLK_ON),
						 ~P2P_RX_CLK_ON_MASK,
						 P2P_RX_CLK_ON_EN));
#endif

		reg_write(INFRA_MISC_2, aor(reg_read(INFRA_MISC_2),
						 ~INFRA_MISC_2_MASK,
						 INFRA_MISC_2_EN));
	} else {
		reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
						 ~INFRA_BUS_DCM_CTRL_MASK,
						 INFRA_BUS_DCM_CTRL_DIS));

#if 0 /* workaround to avoid access dram reg fail*/
		reg_write(P2P_RX_CLK_ON, aor(reg_read(P2P_RX_CLK_ON),
						 ~P2P_RX_CLK_ON_MASK,
						 P2P_RX_CLK_ON_DIS));
#endif

		reg_write(INFRA_MISC_2, aor(reg_read(INFRA_MISC_2),
						 ~INFRA_MISC_2_MASK,
						 INFRA_MISC_2_DIS));
	}

	return 0;
}

int dcm_infra_dbc(int cnt)
{
	dcm_info("%s(%d)\n", __func__, cnt);

	reg_write(INFRA_BUS_DCM_CTRL, aor(reg_read(INFRA_BUS_DCM_CTRL),
					  ~INFRA_BUS_DCM_CTRL_DBC_MASK, (cnt << 15)));

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
	dcm_info("%s(%d,%d)\n", __func__, fsel, sfsel);

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
 * 0	0	peri_dcm_rg_clkoff_en		1'b0	1'b0	1'b1
 * 1	1	peri_dcm_rg_clkslow_en		1'b0	1'b0	1'b1
 * 2	2	peri_dcm_rg_force_clkoff	1'b0	1'b0	1'b0
 * 3	3	peri_dcm_rg_force_clkslow	1'b0	1'b0	1'b0
 * 4	4	peri_dcm_rg_force_on		1'b0	1'b0	1'b0
 * 9	5	peri_dcm_rg_fsel			5'h1f	5'b10000	5'b10000
 * 14	10	peri_dcm_rg_sfsel			5'h1f	5'b00001	5'b00001
 * 19	15	peri_dcm_dbc_rg_dbc_num		5'h10	5'h10	5'h10
 * 20	20	peri_dcm_dbc_rg_dbc_en		1'b1	1'b1	1'b1
 * 21	21	rg_usb_dcm_en				1'b1	1'b0	1'b1
 * 22	22	rg_pmic_dcm_en				1'b0	1'b0	1'b0
 * 27	23	pmic_cnt_mst_rg_sfsel		5'h8	5'b0	5'b0
 * 28	28	rg_icusb_dcm_en				1'b0	1'b0	1'b1
 * 29	29	rg_audio_dcm_en				1'b0	1'b0	1'b1
 * 31	31	rg_ssusb_top_dcm_en	1'b0	1'b0	1'b1
 */
#define PERI_DCM_MASK		((1<<0) | (1<<1) | (0x7<<2) | (0x1F<<5) | (0x1F<<10) | (0x1F<<15) | (1<<20) | \
							 (1<<21) | (1<<22) | (0x1F<<23) | (1<<28) | (1<<29) | (1<<31))
#define PERI_DCM_EN			((1<<0) | (1<<1) | (0x0<<2) | (0x10<<5) | (0x01<<10) | (0x10<<15) | (1<<20) | \
							 (1<<21) | (0<<22) | (0x0<<23) | (1<<28) | (1<<29) | (1<<31))
#define PERI_DCM_DIS		((0<<0) | (0<<1) | (0x0<<2) | (0x10<<5) | (0x10<<10) | (0x10<<15) | (0<<20) | \
							 (0<<21) | (0<<22) | (0x0<<23) | (0<<28) | (0<<29) | (0<<31))
#define PERI_DCM_SEL_MASK	((0x1F<<5) | (0x1F<<10))
#define PERI_DCM_SEL_EN		((0x10<<5) | (0x01<<10))
#define PERI_DCM_DB_MASK	(0x1F<<15)

typedef enum {
	PERI_DCM_OFF = DCM_OFF,
	PERI_DCM_ON = DCM_ON,
} ENUM_PERI_DCM;

int dcm_peri(ENUM_PERI_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

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
	dcm_info("%s(%d)\n", __func__, cnt);

	reg_write(PERI_BUS_DCM_CTRL, aor(reg_read(PERI_BUS_DCM_CTRL),
						  ~PERI_DCM_DB_MASK,
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

/**
 * 0x10001078	MEM_DCM_CTRL
 *    0	        0	mem_dcm_apb_toggle
 *    5	        1	mem_dcm_apb_sel
 *    6	        6	mem_dcm_force_on
 *    7	        7	mem_dcm_dcm_en
 *    8	        8	mem_dcm_dbc_en
 *    15	9	mem_dcm_dbc_cnt
 *    20	16	mem_dcm_fsel
 *    25	21	mem_dcm_idle_fsel
 *    26	26	mem_dcm_force_off
 *    29	29	mem_mdsys_gating_sel
 *    31	31	mem_dcm_hwcg_off_disable
 **/

/**
 * 0x1000107C	DFS_MEM_DCM_CTRL
 * 1	0	dramc_free_clk_slow_ratio
 * 2	2	dramc_free_clk_switch_rate_enable
 * 3	3	dramc0_hclk_ck_cg
 * 4	4	dramc1_hclk_ck_cg
 * 8	8	dramc_pipe_ck_cg
 * 9	9	dramc_pipe_slowdown_enable
 **/
#define MEM_BUS_DCM_CTRL_MASK	((0x1 << 6) | (0x1 << 7) | (0x1 << 8) | (0x7f << 9) | \
								 (0x1f << 16) | (0x1f << 21) | (0x1 << 26) | \
								 (0x1 << 29) | (0x1 << 31))
#define MEM_BUS_DCM_CTRL_EN		((0x0 << 6) | (0x1 << 7) | (0x1 << 8) | (0x10 << 9) | \
								 (0x0 << 16) | (0x1f << 21) | (0x0 << 26) | \
								 (0x0 << 29) | (0x0 << 31))
#define MEM_BUS_DCM_CTRL_DIS	((0x0 << 6) | (0x0 << 7) | (0x0 << 8) | (0x2f << 9) | \
								 (0x0 << 16) | (0x1f << 21) | (0x0 << 26) | \
								 (0x0 << 29) | (0x1 << 31))

#define DFS_MEM_BUS_DCM_CTRL_MASK	((0x1 << 3) | (0x1 << 4) | (0x1 << 8) | (0x1 << 9))
#define DFS_MEM_BUS_DCM_CTRL_EN		((0x1 << 3) | (0x1 << 4) | (0x1 << 8) | (0x1 << 9))
#define DFS_MEM_BUS_DCM_CTRL_DIS	((0x0 << 3) | (0x0 << 4) | (0x0 << 8) | (0x0 << 9))

#define MEM_BUS_DCM_CTRL_SEL_MASK	((0x1F << 16) | (0x1F << 21))
#define MEM_BUS_DCM_CTRL_SEL_EN		((0x0 << 16) | (0x1f  << 21))
#define MEM_BUS_DCM_CTRL_DBC_MASK	(0x7F << 9)
#define MEM_BUS_DCM_CTRL_DBC_SHFT	9
#define MEM_BUS_DCM_TOGGLE			(0x1)
#define MEM_BUS_DCM_SEL         (0x1f << 1)
#define MEM_BUS_DCM_FSEL_MASK	((0x1f << 16) | (0x1f << 21))
#define MEM_BUS_DCM_FSEL_SHFT		16
#define MEM_BUS_DCM_IDLE_FSEL_SHFT  21
#define MEM_DCM_FSEL_EN		    ((0x0 << 16) | (0x1f << 21))

#define DFS_MEM_BUS_RATE_TOGGLE (0x1 << 2)
#define DFS_MEM_BUS_RATE_MASK	(0x3 << 0)
#define DFS_MEM_BUS_RATE_SHFT	0
#define DRAMC_PIPE_SLOWDOWN_EN	(0x1 << 9)

typedef enum {
	MEM_DCM_OFF = DCM_OFF,
	MEM_DCM_ON = DCM_ON,
} ENUM_MEM_DCM;

void dcm_mem_toggle(void)
{
	reg_write(MEM_DCM_CTRL, or(reg_read(MEM_DCM_CTRL), MEM_BUS_DCM_SEL));
	reg_write(MEM_DCM_CTRL, or(reg_read(MEM_DCM_CTRL), MEM_BUS_DCM_TOGGLE));
	reg_write(MEM_DCM_CTRL, and(reg_read(MEM_DCM_CTRL), ~(MEM_BUS_DCM_TOGGLE)));
}
/*
 * input argument
 * 0: 1/1(0x0)
 * 1: 1/2(0x01)
 * 2: 1/4(0x03)
 * 3: 1/8(0x07)
 * 4: 1/16(0x0f)
 * 5: 1/32(0x1f)
 * default: 0, 0
 */
int dcm_mem_rate(unsigned int fsel, unsigned int sfsel)
{
	BUG_ON(fsel > 5 || sfsel > 5);

	/* fsel = 0x1f - (0x1f >> fsel); */
	/* sfsel = 0x1f - (0x1f >> sfsel); */
	fsel = (0x1 << fsel) - 1;
	sfsel = (0x1 << sfsel) - 1;
	reg_write(MEM_DCM_CTRL, aor(reg_read(MEM_DCM_CTRL),
					 ~MEM_BUS_DCM_FSEL_MASK,
					 ((fsel << MEM_BUS_DCM_FSEL_SHFT) | (sfsel << MEM_BUS_DCM_IDLE_FSEL_SHFT))));
	dcm_mem_toggle();
	return 0;
}

/*
 * input argument
 * 0: 1/1
 * 1: 1/16
 * 2: 1/32
 * 3: 1/64
 * default: 0, 0
 */
int dcm_dfs_mem_rate(unsigned int sfsel)
{
	BUG_ON(sfsel > 3);

	reg_write(DFS_MEM_DCM_CTRL,
					aor(reg_read(DFS_MEM_DCM_CTRL),
						~DFS_MEM_BUS_RATE_MASK,
						(sfsel << DFS_MEM_BUS_RATE_SHFT)));
	reg_write(DFS_MEM_DCM_CTRL, and(reg_read(DFS_MEM_DCM_CTRL), ~DRAMC_PIPE_SLOWDOWN_EN));
	reg_write(DFS_MEM_DCM_CTRL, or(reg_read(DFS_MEM_DCM_CTRL), DFS_MEM_BUS_RATE_TOGGLE));
	reg_write(DFS_MEM_DCM_CTRL, and(reg_read(DFS_MEM_DCM_CTRL), ~DFS_MEM_BUS_RATE_TOGGLE));
	reg_write(DFS_MEM_DCM_CTRL, or(reg_read(DFS_MEM_DCM_CTRL), DRAMC_PIPE_SLOWDOWN_EN));

	return 0;
}

void dcm_mem_preset(void)
{
	reg_write(MEM_DCM_CTRL, aor(reg_read(MEM_DCM_CTRL),
					 ~MEM_BUS_DCM_FSEL_MASK,
					 MEM_DCM_FSEL_EN));
}

int dcm_mem(ENUM_MEM_DCM on)
{
	if (on == MEM_DCM_ON) {

		reg_write(MEM_DCM_CTRL,
					aor(reg_read(MEM_DCM_CTRL),
						~MEM_BUS_DCM_CTRL_MASK,
						MEM_BUS_DCM_CTRL_EN));

		dcm_mem_toggle();

		dcm_dfs_mem_rate(0x0);

		reg_write(DFS_MEM_DCM_CTRL,
					aor(reg_read(DFS_MEM_DCM_CTRL),
						~DFS_MEM_BUS_DCM_CTRL_MASK,
						DFS_MEM_BUS_DCM_CTRL_EN));

	} else if (on == MEM_DCM_OFF) {

		reg_write(MEM_DCM_CTRL,
					aor(reg_read(MEM_DCM_CTRL),
						~MEM_BUS_DCM_CTRL_MASK,
						MEM_BUS_DCM_CTRL_DIS));

		dcm_mem_toggle();

		reg_write(DFS_MEM_DCM_CTRL,
					aor(reg_read(DFS_MEM_DCM_CTRL),
						~DFS_MEM_BUS_DCM_CTRL_MASK,
						DFS_MEM_BUS_DCM_CTRL_DIS));
	}

	return 0;
}


int dcm_mem_dbc(int cnt)
{
	reg_write(MEM_DCM_CTRL, aor(reg_read(MEM_DCM_CTRL),
						  ~MEM_BUS_DCM_CTRL_DBC_MASK,
						  (cnt << MEM_BUS_DCM_CTRL_DBC_SHFT)));

	return 0;
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
 * 0	11	xxx		"0: disable, 1: enable"
 * 16	21	yyy		"0: disable, 1: enable"
 */
#define MCUCFG_BUS_FABRIC_DCM_CTRL_MASK	((0xFFF << 0) | (0x3F << 16))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_ON	((0xFFF << 0) | (0x3F << 16))
#define MCUCFG_BUS_FABRIC_DCM_CTRL_OFF	((0x0   << 0) | (0x0  << 16))

/*
 * 0x0740	CCI_ADB400_DCM_CONFIG
 * 0	0	cci_m0_adb400_dcm_en	"0: disable, 1: enable"
 * 1	1	cci_m1_adb400_dcm_en	"0: disable, 1: enable"
 * 2	2	cci_m2_adb400_dcm_en	"0: disable, 1: enable"
 * 3	3	cci_s2_adb400_dcm_en	"0: disable, 1: enable"
 * 4	4	cci_s3_adb400_dcm_en	"0: disable, 1: enable"
 * 5	5	cci_s4_adb400_dcm_en	"0: disable, 1: enable"
 * 6	6	cci_s5_adb400_dcm_en	"0: disable, 1: enable"
 * 7	7	l2c_adb400_dcm_en	"0: disable, 1: enable"
 */
#define MCUCFG_CCI_ADB400_DCM_CONFIG_MASK	(0xFF << 0)
#define MCUCFG_CCI_ADB400_DCM_CONFIG_ON		(0xFF << 0)
#define MCUCFG_CCI_ADB400_DCM_CONFIG_OFF	(0x0  << 0)

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
#define MCUCFG_SYNC_DCM_DIV_SEL1	((0x0 << 1) | (0x0 << 9) | (0x0 << 17))
#define MCUCFG_SYNC_DCM_DIV_SEL2	((0x1 << 1) | (0x1 << 9) | (0x1 << 17))
#define MCUCFG_SYNC_DCM_DIV_SEL4	((0x3 << 1) | (0x3 << 9) | (0x3 << 17))
#define MCUCFG_SYNC_DCM_DIV_SEL32	((0x1F << 1) | (0x1F << 9) | (0x1F << 17))
#define MCUCFG_SYNC_DCM_TOGMASK		((0x1 << 6) | (0x1 << 14) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_CCI_TOGMASK	(0x1 << 6)
#define MCUCFG_SYNC_DCM_MP0_TOGMASK	(0x1 << 14)
#define MCUCFG_SYNC_DCM_MP1_TOGMASK	(0x1 << 22)
#define MCUCFG_SYNC_DCM_TOG1		((0x1 << 6) | (0x1 << 14) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_TOG0		((0x0 << 6) | (0x0 << 14) | (0x0 << 22))
#define MCUCFG_SYNC_DCM_CCI_TOG1	(0x1 << 6)
#define MCUCFG_SYNC_DCM_CCI_TOG0	(0x0 << 6)
#define MCUCFG_SYNC_DCM_MP0_TOG1	(0x1 << 14)
#define MCUCFG_SYNC_DCM_MP0_TOG0	(0x0 << 14)
#define MCUCFG_SYNC_DCM_MP1_TOG1	(0x1 << 22)
#define MCUCFG_SYNC_DCM_MP1_TOG0	(0x0 << 22)
#define MCUCFG_SYNC_DCM_SELTOG_MASK	((0x1F << 1) | (0x1 << 6) | \
					(0x1F << 9) | (0x1 << 14) | \
					(0x1F << 17) | (0x1 << 22))
#define MCUCFG_SYNC_DCM_SELTOG_CCI_MASK	((0x1F << 1) | (0x1 << 6))
#define MCUCFG_SYNC_DCM_SELTOG_MP0_MASK	((0x1F << 9) | (0x1 << 14))
#define MCUCFG_SYNC_DCM_SELTOG_MP1_MASK	((0x1F << 17) | (0x1 << 22))

/*
 *	0x10222274 MCUCFG_SYNC_DCM_MP2_CONFIG
 *	6:2	div_sel	DCM Divider Select
 *		00000 - Div1
 *		00001 - Div2
 *		...
 *		11111 - Div32
 *	1	update_tog	DCM update toggle
 *	0	dcm_en	DCM mode enable
 */
#define MCUCFG_SYNC_DCM_MP2_MASK	((0x1 << 0) | (0x1 << 1) | (0x1F << 2))
#define MCUCFG_SYNC_DCM_MP2_ON		(0x1 << 0)
#define MCUCFG_SYNC_DCM_MP2_OFF		(0x0 << 0)
#define MCUCFG_SYNC_DCM_MP2_TOGMASK	(0x1 << 1)
#define MCUCFG_SYNC_DCM_MP2_TOG0	(0x0 << 1)
#define MCUCFG_SYNC_DCM_MP2_TOG1	(0x1 << 1)
#define MCUCFG_SYNC_DCM_MP2_DIV_SEL0	(0x0 << 2)
#define MCUCFG_SYNC_DCM_MP2_DIV_SEL2	(0x1 << 2)
#define MCUCFG_SYNC_DCM_MP2_DIV_SEL4	(0x3 << 2)
#define MCUCFG_SYNC_DCM_MP2_DIV_SEL32	(0x1F << 2)
#define MCUCFG_SYNC_DCM_MP2_TOGDIV_MASK	((0x1 << 1) | (0x1F << 2))

/*
 * 0x074C	SYNC_DCM_CLUSTER_CONFIG
 * 4	0	mp0_sync_dcm_stall_wr_del_sel	Debounce Value must >= 16
 * 7	7	mp0_sync_dcm_stall_wr_en
 */
#define MCUCFG_SYNC_DCM_CLUSTER_MASK	((0x1F << 0) | (0x1 << 7))
#define MCUCFG_SYNC_DCM_CLUSTER_EN	((0x1F << 0) | (0x1 << 7))
#define MCUCFG_SYNC_DCM_CLUSTER_DIS	((0x0F << 0) | (0x0 << 7))

/* Fine-grained DCM use MCSI-A internal slice valid to gate clock
 * 0x20390000	MCSI-A CG
 * 16   Dynamic clock gating enable for Slave interface 0
 * 17   Dynamic clock gating enable for Slave interface 1
 * 18   Dynamic clock gating enable for Slave interface 2
 * 19   Dynamic clock gating enable for Slave interface 3
 * 20   Dynamic clock gating enable for Slave interface 4
 * 21   Dynamic clock gating enable for Slave interface 5
 * 22   Dynamic clock gating select for all Slave interface
     0x0: coarse-grained DCM
     0x1: fine-grained DCM
 * 23   Dynamic clock gating select for all Master interface
     0x0: coarse-grained DCM
     0x1: fine-grained DCM
 * 24   Dynamic clock gating enable for Master interface 0
 * 25   Dynamic clock gating enable for Master interface 1
 * 26   Dynamic clock gating enable for Master interface 2
 * 27   Dynamic clock gating enable for DVM manage
 * 28   Dynamic clock gating enable for Snoop Request Block
 * 29   Dynamic clock gating enable for Master interface 0 Read, Write channel
 * 30   Dynamic clock gating enable for Master interface 1 Read, Write channel
 * 31   Dynamic clock gating enable for Master interface 2 Read, Write channel
 */
#define MSCI_A_DCM_MASK	(0xFFFF << 16)
#define MSCI_A_DCM_ON	(0xFFFF << 16)
#define MSCI_A_DCM_OFF	(0x0    << 16)

#ifdef NON_AO_MCUSYS_DCM
void dcm_non_ao_mcusys(ENUM_MCUSYS_DCM on)
{
	if (on == MCUSYS_DCM_ON) {
		MCUSYS_SMC_WRITE(MSCI_A_DCM,
				aor(reg_read(MSCI_A_DCM),
				    ~MSCI_A_DCM_MASK,
				    MSCI_A_DCM_ON));
	} else {
		MCUSYS_SMC_WRITE(MSCI_A_DCM,
				aor(reg_read(MSCI_A_DCM),
				    ~MSCI_A_DCM_MASK,
				    MSCI_A_DCM_OFF));
	}
}
#endif

int dcm_mcusys_sync_dcm(ENUM_MCUSYS_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == MCUSYS_DCM_ON) {
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_ON |
				     MCUCFG_SYNC_DCM_DIV_SEL4 |
				     MCUCFG_SYNC_DCM_TOG1)));

		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_ON |
				     MCUCFG_SYNC_DCM_DIV_SEL4 |
				     MCUCFG_SYNC_DCM_TOG0)));
	} else {
		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_OFF |
				     MCUCFG_SYNC_DCM_DIV_SEL1 |
				     MCUCFG_SYNC_DCM_TOG1)));

		MCUSYS_SMC_WRITE(MCUCFG_SYNC_DCM_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_CONFIG),
				    ~MCUCFG_SYNC_DCM_MASK,
				    (MCUCFG_SYNC_DCM_OFF |
				     MCUCFG_SYNC_DCM_DIV_SEL1 |
				     MCUCFG_SYNC_DCM_TOG0)));
	}

	return 0;
}

int dcm_mcusys_mp2_sync_dcm(ENUM_MCUSYS_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == MCUSYS_DCM_ON) {

		reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
				    ~MCUCFG_SYNC_DCM_MP2_MASK,
				    (MCUCFG_SYNC_DCM_MP2_ON |
				     MCUCFG_SYNC_DCM_MP2_DIV_SEL4 |
				     MCUCFG_SYNC_DCM_MP2_TOG1)));
		/* dcm_info("[0x%x] = 0x%x\n", MCUCFG_SYNC_DCM_MP2_CONFIG, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG)); */

		reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
				    ~MCUCFG_SYNC_DCM_MP2_MASK,
				    (MCUCFG_SYNC_DCM_MP2_ON |
				     MCUCFG_SYNC_DCM_MP2_DIV_SEL4 |
				     MCUCFG_SYNC_DCM_MP2_TOG0)));
		/* dcm_info("[0x%x] = 0x%x\n", MCUCFG_SYNC_DCM_MP2_CONFIG, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG)); */
	} else {
		/* dcm_info("MP2 sync DCM OFF\n"); */

		reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
				    ~MCUCFG_SYNC_DCM_MP2_MASK,
				    (MCUCFG_SYNC_DCM_MP2_OFF |
				     MCUCFG_SYNC_DCM_MP2_DIV_SEL0 |
				     MCUCFG_SYNC_DCM_MP2_TOG1)));
		/* dcm_info("[0x%x] = 0x%x\n", MCUCFG_SYNC_DCM_MP2_CONFIG, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG)); */

		reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
				aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
				    ~MCUCFG_SYNC_DCM_MP2_MASK,
				    (MCUCFG_SYNC_DCM_MP2_OFF |
				     MCUCFG_SYNC_DCM_MP2_DIV_SEL0 |
				     MCUCFG_SYNC_DCM_MP2_TOG0)));
		/* dcm_info("[0x%x] = 0x%x\n", MCUCFG_SYNC_DCM_MP2_CONFIG, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG)); */
	}

	return 0;
}

int dcm_mcusys_little(ENUM_MCUSYS_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == MCUSYS_DCM_ON) {

		/* set l2_sram_dcm_en = 1 */
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
				    ~0xff,
				    0xff));

		dcm_mcusys_sync_dcm(MCUSYS_DCM_ON);

#ifdef NON_AO_MCUSYS_DCM
		dcm_non_ao_mcusys(MCUSYS_DCM_ON);
#endif
	} else {

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
				    ~0xff,
				    0x0));

		dcm_mcusys_sync_dcm(MCUSYS_DCM_OFF);

#ifdef NON_AO_MCUSYS_DCM
		dcm_non_ao_mcusys(MCUSYS_DCM_OFF);
#endif
	}

	return 0;
}

int dcm_mcusys(ENUM_MCUSYS_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == MCUSYS_DCM_ON) {

		dcm_mcusys_little(MCUSYS_DCM_ON);
#ifdef NON_AO_MP2_DCM_CONFIG
		dcm_mcusys_mp2_sync_dcm(MCUSYS_DCM_ON);
#endif

	} else {

		dcm_mcusys_little(MCUSYS_DCM_OFF);
#ifdef NON_AO_MP2_DCM_CONFIG
		dcm_mcusys_mp2_sync_dcm(MCUSYS_DCM_OFF);
#endif
	}

	return 0;
}

/*
 * DRAMC DCM golden setting ---------------------------------------------------
 */

/*
 * 0x100041dc	DRAMC_PD_CTRL
 * 0x100131dc	DRAMC_B_PD_CTRL
 * 25	25	DCMEN
 * 1	1	DCMEN2
 */
#define DRAMC_PD_CTRL_MASK	((1<<25) | (1<<1))
#define DRAMC_PD_CTRL_EN	((1<<25) | (1<<1))
#define DRAMC_PD_CTRL_DIS	((0<<25) | (0<<1))

/*
 * 0x100041ec	DRAMC_PERFCTL0
 * 0x100131ec	DRAMC_B_PERFCTL0
 * 16	16	DISDMOEDIS
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
	dcm_info("%s(%d)\n", __func__, on);

	if (on) {

		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_EN));

		reg_write(DRAMC_B_PD_CTRL, aor(reg_read(DRAMC_B_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_EN));

		reg_write(DRAMC_PERFCTL0, aor(reg_read(DRAMC_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_EN));

		reg_write(DRAMC_B_PERFCTL0, aor(reg_read(DRAMC_B_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_EN));
	} else {

		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_DIS));

		reg_write(DRAMC_B_PD_CTRL, aor(reg_read(DRAMC_B_PD_CTRL),
					     ~DRAMC_PD_CTRL_MASK,
					     DRAMC_PD_CTRL_DIS));

		reg_write(DRAMC_PERFCTL0, aor(reg_read(DRAMC_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_DIS));

		reg_write(DRAMC_B_PERFCTL0, aor(reg_read(DRAMC_B_PERFCTL0),
					     ~DRAMC_PERFCTL0_MASK,
					     DRAMC_PERFCTL0_DIS));
	}

	return 0;
}

/*
 * DDRPHY DCM golden setting --------------------------------------------------
 */
/*
 * 0x100041dc	DRAMC_PD_CTRL
 * 0x100131dc	DRAMC_B_PD_CTRL
 * 31	31	COMBCLKCTRL
 * 30	30	PHYCLKDYNGEN
 * 26	26	MIOCKCTRLOFF
 * 3	3	COMBPHY_CLKENSAME
 */
#define DDRPHY_CTRL_MASK	((1<<31) | (1<<30) | (1<<26) | (1<<3))
#define DDRPHY_CTRL_EN		((1<<31) | (1<<30) | (0<<26) | (0<<3))
#define DDRPHY_CTRL_DIS		((0<<31) | (0<<30) | (1<<26) | (1<<3))

typedef enum {
	DDRPHY_DCM_OFF = DCM_OFF,
	DDRPHY_DCM_ON = DCM_ON,
} ENUM_DDRPHY_DCM;

int dcm_ddrphy(ENUM_DDRPHY_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on) {
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DDRPHY_CTRL_MASK,
					     DDRPHY_CTRL_EN));

		reg_write(DRAMC_B_PD_CTRL, aor(reg_read(DRAMC_B_PD_CTRL),
					     ~DDRPHY_CTRL_MASK,
					     DDRPHY_CTRL_EN));
	} else {
		reg_write(DRAMC_PD_CTRL, aor(reg_read(DRAMC_PD_CTRL),
					     ~DDRPHY_CTRL_MASK,
					     DDRPHY_CTRL_DIS));

		reg_write(DRAMC_B_PD_CTRL, aor(reg_read(DRAMC_B_PD_CTRL),
					     ~DDRPHY_CTRL_MASK,
					     DDRPHY_CTRL_DIS));
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
#define EMI_CONM_EN		(0x0<<24)
#define EMI_CONM_DIS	(0xFF<<24)

typedef enum {
	EMI_DCM_OFF = DCM_OFF,
	EMI_DCM_ON = DCM_ON,
} ENUM_EMI_DCM;

int dcm_emi(ENUM_EMI_DCM on)
{
	dcm_info("%s(%d)\n", __func__, on);

	if (on == EMI_DCM_ON)
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
#if 0
	PMIC_DCM_TYPE		= (1U << 7),
#else
	MEM_DCM_TYPE		= (1U << 7),
#endif
	USB_DCM_TYPE		= (1U << 8),
	ICUSB_DCM_TYPE		= (1U << 9),
	AUDIO_DCM_TYPE		= (1U << 10),
	SSUSB_DCM_TYPE		= (1U << 11),

	NR_DCM_TYPE = 12,
};

#define ALL_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | EMI_DCM_TYPE | DRAMC_DCM_TYPE | \
		       DDRPHY_DCM_TYPE | MEM_DCM_TYPE | USB_DCM_TYPE | \
		       ICUSB_DCM_TYPE | AUDIO_DCM_TYPE | SSUSB_DCM_TYPE)

#define INIT_DCM_TYPE  (ARMCORE_DCM_TYPE | MCUSYS_DCM_TYPE | INFRA_DCM_TYPE | \
		       PERI_DCM_TYPE | /* EMI_DCM_TYPE | DRAMC_DCM_TYPE |*/ \
		       /*| DDRPHY_DCM_TYPE */ MEM_DCM_TYPE | USB_DCM_TYPE | \
		       ICUSB_DCM_TYPE | AUDIO_DCM_TYPE | SSUSB_DCM_TYPE | MEM_DCM_TYPE)

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
#if 0
	/* keep pmic DCM OFF */
	{
	 .typeid = PMIC_DCM_TYPE,
	 .name = "PMIC_DCM",
	 .func = (DCM_FUNC) dcm_pmic,
	 .current_state = PMIC_DCM_OFF,
	 .default_state = PMIC_DCM_OFF,
	 .disable_refcnt = 0,
	 },
#else
	/* new added */
	{
	 .typeid = MEM_DCM_TYPE,
	 .name = "MEM_DCM",
	 .func = (DCM_FUNC) dcm_mem,
	 .preset_func = (DCM_PRESET_FUNC) dcm_mem_preset,
	 .current_state = MEM_DCM_ON,
	 .default_state = MEM_DCM_ON,
	 .disable_refcnt = 0,
	 },
#endif
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
	 }
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
			#if 1
			if (dcm->disable_refcnt == 0) {
			#endif
				dcm->current_state = state;
				dcm->func(dcm->current_state);
			#if 1
			}
			#endif

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

	dcm_info("\n=== armcore DCM ===\n");
	dcm_info("%-30s(0x%08X): 0x%08X\n", "ARMPLLDIV_DCMCTL",
		(unsigned int)(0x1001A000+ARMPLLDIV_DCMCTL), mt6757_0x1001AXXX_reg_read(ARMPLLDIV_DCMCTL));

	dcm_info("\nmcusys DCM:\n");
	REG_DUMP(MCUCFG_L2C_SRAM_CTRL);
	REG_DUMP(MCUCFG_CCI_CLK_CTRL);
	REG_DUMP(MCUCFG_BUS_FABRIC_DCM_CTRL);
	REG_DUMP(MCUCFG_CCI_ADB400_DCM_CONFIG);
	REG_DUMP(MCUCFG_SYNC_DCM_CONFIG);
#ifdef NON_AO_MP2_DCM_CONFIG
	REG_DUMP(MCUCFG_SYNC_DCM_MP2_CONFIG);
#endif
#ifdef NON_AO_MCUSYS_DCM
	REG_DUMP(MSCI_A_DCM);
#endif

	dcm_info("\n=== infra DCM ===\n");
	REG_DUMP(INFRA_BUS_DCM_CTRL);
	REG_DUMP(P2P_RX_CLK_ON);
	REG_DUMP(INFRA_MISC_2);

	dcm_info("\n=== peri DCM ===\n");
	REG_DUMP(PERI_BUS_DCM_CTRL);

	dcm_info("\n=== EMI DCM ===\n");
	REG_DUMP(EMI_CONM);

	dcm_info("\n=== DRAMC/DDRPHY DCM ===\n");
	REG_DUMP(DRAMC_PD_CTRL);
	REG_DUMP(DRAMC_B_PD_CTRL);
	REG_DUMP(DRAMC_PERFCTL0);
	REG_DUMP(DRAMC_B_PERFCTL0);

	dcm_info("\n=== MEM DCM ===\n");
	REG_DUMP(MEM_DCM_CTRL);
	REG_DUMP(DFS_MEM_DCM_CTRL);
}


#if defined(CONFIG_PM)
static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	int len = 0;
	char *p = buf;
	int i;
	DCM *dcm;

	/* dcm_dump_state(ALL_DCM_TYPE); */
	p += sprintf(p, "\n******** dcm dump state *********\n");
	for (i = 0, dcm = &dcm_array[0]; i < NR_DCM_TYPE; i++, dcm++)
		p += sprintf(p, "[%-16s 0x%08x] current state: %s\n",
			     dcm->name, dcm->typeid, dcm->current_state ? "ON" : "OFF");

	p += sprintf(p, "\n******** dcm dump register *********\n");
	p += sprintf(p, "\n=== armcore DCM ===\n");
	p += sprintf(p, "%-30s(0x%08X): 0x%08X\n", "ARMPLLDIV_DCMCTL",
				(unsigned int)(0x1001A000+ARMPLLDIV_DCMCTL),
				mt6757_0x1001AXXX_reg_read(ARMPLLDIV_DCMCTL));

	p += sprintf(p, "\nmcusys DCM:\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_L2C_SRAM_CTRL",
				MCUCFG_L2C_SRAM_CTRL, reg_read(MCUCFG_L2C_SRAM_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_CCI_CLK_CTRL",
				MCUCFG_CCI_CLK_CTRL, reg_read(MCUCFG_CCI_CLK_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_BUS_FABRIC_DCM_CTRL",
				MCUCFG_BUS_FABRIC_DCM_CTRL, reg_read(MCUCFG_BUS_FABRIC_DCM_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_CCI_ADB400_DCM_CONFIG",
				MCUCFG_CCI_ADB400_DCM_CONFIG, reg_read(MCUCFG_CCI_ADB400_DCM_CONFIG));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_SYNC_DCM_CONFIG",
				MCUCFG_SYNC_DCM_CONFIG, reg_read(MCUCFG_SYNC_DCM_CONFIG));
#ifdef NON_AO_MP2_DCM_CONFIG
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MCUCFG_SYNC_DCM_MP2_CONFIG",
				MCUCFG_SYNC_DCM_MP2_CONFIG, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG));
#endif

	p += sprintf(p, "\n=== infra DCM ===\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "INFRA_BUS_DCM_CTRL",
				INFRA_BUS_DCM_CTRL, reg_read(INFRA_BUS_DCM_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "P2P_RX_CLK_ON",
				P2P_RX_CLK_ON, reg_read(P2P_RX_CLK_ON));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "INFRA_MISC_2",
				INFRA_MISC_2, reg_read(INFRA_MISC_2));

	p += sprintf(p, "\n=== peri DCM ===\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "PERI_BUS_DCM_CTRL",
				PERI_BUS_DCM_CTRL, reg_read(PERI_BUS_DCM_CTRL));

	p += sprintf(p, "\n=== EMI DCM ===\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "EMI_CONM", EMI_CONM, reg_read(EMI_CONM));

	p += sprintf(p, "\n=== DRAMC/DDRPHY DCM ===\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "DRAMC_PD_CTRL",
				DRAMC_PD_CTRL, reg_read(DRAMC_PD_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "DRAMC_B_PD_CTRL",
				DRAMC_B_PD_CTRL, reg_read(DRAMC_B_PD_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "DRAMC_PERFCTL0",
				DRAMC_PERFCTL0, reg_read(DRAMC_PERFCTL0));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "DRAMC_B_PERFCTL0",
				DRAMC_B_PERFCTL0, reg_read(DRAMC_B_PERFCTL0));

	p += sprintf(p, "\n=== MEM DCM ===\n");
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "MEM_DCM_CTRL",
				MEM_DCM_CTRL, reg_read(MEM_DCM_CTRL));
	p += sprintf(p, "%-30s(0x%08lX): 0x%08X\n", "DFS_MEM_DCM_CTRL",
				DFS_MEM_DCM_CTRL, reg_read(DFS_MEM_DCM_CTRL));

	p += sprintf(p, "\n********** dcm_state help *********\n");
	p += sprintf(p, "set:		echo set [mask] [mode] > /sys/power/dcm_state\n");
	p += sprintf(p, "set:		echo default [mask] > /sys/power/dcm_state\n");
#if 1
	p += sprintf(p, "disable:	echo disable [mask] > /sys/power/dcm_state\n");
	p += sprintf(p, "restore:	echo restore [mask] > /sys/power/dcm_state\n");
	p += sprintf(p, "dump:		echo dump [mask] > /sys/power/dcm_state\n");
#endif
	p += sprintf(p, "***** [mask] is hexl bit mask of dcm;\n");
	p += sprintf(p, "***** [mode] is type of DCM to set and retained\n");

	len = p - buf;
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
		} else if (!strcmp(cmd, "default")) {
			dcm_set_default(mask);
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

	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " MCUCFG_NODE);
		BUG();
	}
	if (of_address_to_resource(node, 0, &r)) {
		dcm_info("error: cannot get phys addr index 1 of " MCUCFG_NODE);
		BUG();
	}
	mcucfg_phys_base = r.start;

	mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!mcucfg_base) {
		dcm_info("error: cannot iomap index 0 of " MCUCFG_NODE);
		BUG();
	}

	/* mcucfg2 */
	if (of_address_to_resource(node, 1, &r)) {
		dcm_info("error: cannot get phys addr index 1 of " MCUCFG2_NODE);
		BUG();
	}
	mcucfg2_phys_base = r.start;

	mcucfg2_base = (unsigned long)of_iomap(node, 1);
	if (!mcucfg2_base) {
		dcm_info("error: cannot iomap index 1 of " MCUCFG2_NODE);
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

	/* dramc_conf_base */
	node = of_find_compatible_node(NULL, NULL, DRAMC_CONFIG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " DRAMC_CONFIG_NODE);
		BUG();
	}
	dramc_conf_base = (unsigned long)of_iomap(node, 0);
	if (!dramc_conf_base) {
		dcm_info("error: cannot iomap " DRAMC_CONFIG_NODE);
		BUG();
	}

	/* dramc_conf_b_base */
	node = of_find_compatible_node(NULL, NULL, DRAMC_CONFIG_B_NODE);
	if (!node) {
		dcm_info("error: cannot find node " DRAMC_CONFIG_B_NODE);
		BUG();
	}
	dramc_conf_b_base = (unsigned long)of_iomap(node, 0);
	if (!dramc_conf_b_base) {
		dcm_info("error: cannot iomap " DRAMC_CONFIG_B_NODE);
		BUG();
	}

	/* mcumixed_base */
	/*node = of_find_compatible_node(NULL, NULL, MCUMIXED_REG_NODE);
	if (!node) {
		dcm_info("error: cannot find node " MCUMIXED_REG_NODE);
		BUG();
	}
	mcumixed_base = (unsigned long)of_iomap(node, 0);
	if (!mcumixed_base) {
		dcm_info("error: cannot iomap " MCUMIXED_REG_NODE);
		BUG();
	}*/

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

	dcm_info("=== mt_dcm_init ===\n");

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

	if (freq < 65)
		return 0;

	/* max divided ratio = Floor (CPU Frequency / 5  * system timer Frequency) */
	div = freq / 65;
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

int sync_dcm_set_mp2_freq(unsigned int mp2)
{
	mt_dcm_init();

	/* set xxx_sync_dcm_tog as 0 first */
	reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
			    ~MCUCFG_SYNC_DCM_MP2_TOGMASK,
			    MCUCFG_SYNC_DCM_MP2_TOG0));

	reg_write(MCUCFG_SYNC_DCM_MP2_CONFIG,
			aor(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG),
			    ~MCUCFG_SYNC_DCM_MP2_TOGDIV_MASK,
			    (MCUCFG_SYNC_DCM_MP2_TOG1 |
			     (sync_dcm_convert_freq2div(mp2) << 2))));

#ifdef DCM_DEBUG
	dcm_info("%s: SYNC_DCM_CONFIG=0x%08x, mp2=%u, mp2_div_sel=%u,%u\n",
		 __func__, reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG), mp2,
		 (and(reg_read(MCUCFG_SYNC_DCM_MP2_CONFIG), (0x1F << 2)) >> 2),
		 sync_dcm_convert_freq2div(mp2));
#endif

	return 0;
}

#else /* Bring Up dummy code */

void mt_dcm_disable(void)
{
}

void mt_dcm_restore(void)
{
}

int sync_dcm_set_cci_freq(unsigned int cci)
{
	return -1;
}

int sync_dcm_set_mp0_freq(unsigned int mp0)
{
	return -1;
}

int sync_dcm_set_mp1_freq(unsigned int mp1)
{
	return -1;
}

int sync_dcm_set_mcsi_a_freq(unsigned int mcsi_a)
{
	return -1;
}

#endif

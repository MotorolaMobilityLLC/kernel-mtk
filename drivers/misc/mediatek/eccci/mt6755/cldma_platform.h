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

#ifndef __CLDMA_PLATFORM_H__
#define __CLDMA_PLATFORM_H__

#include <linux/skbuff.h>
/* this is the platform header file for CLDMA MODEM, not just CLDMA! */

/* Modem WDT */
/* BASE_ADDR_MDRSTCTL+ */
#define REG_MDRSTCTL_WDTCR            (0x0000) /*WDT_MODE*/
#define REG_MDRSTCTL_WDTRR            (0x0010) /*WDT_RESTART*/
#define REG_MDRSTCTL_WDTIR            (0x023C) /*LENGTH*/
#define REG_MDRSTCTL_WDTSR            (0x0034) /*WDT_STATUS*/
#define WDT_MD_MODE		REG_MDRSTCTL_WDTCR
#define WDT_MD_LENGTH           REG_MDRSTCTL_WDTIR
#define WDT_MD_RESTART          REG_MDRSTCTL_WDTRR
#define WDT_MD_STA              REG_MDRSTCTL_WDTSR
#define WDT_MD_MODE_KEY	        (0x55000008)
/* L1_BASE_ADDR_L1RGU+ */
#define REG_L1RSTCTL_WDT_MODE         (0x0000)
#define REG_L1RSTCTL_WDT_LENGTH       (0x0004)
#define REG_L1RSTCTL_WDT_RESTART      (0x0008)
#define REG_L1RSTCTL_WDT_STA          (0x000C)
#define REG_L1RSTCTL_WDT_SWRST        (0x001C)
#define L1_WDT_MD_MODE_KEY	(0x00002200)

/* MD1 PLL */
/* MD_CLKSW_BASE+ */
#define R_CLKSEL_CTL			(0x0024)
	/*Bit 17: RF1_CKSEL,    Bit 16: INTF_CKSEL,  Bit 15: MD2G_CKSEL, Bit 14: DFE_CKSEL
	Bit 13: CMP_CKSEL,    Bit 12: ICC_CKSEL,   Bit 11: IMC_CKSEL,  Bit 10: EQ_CKSEL
	Bit  9: BRP_CKSEL,    Bit 8: L1MCU_CKSEL,  Bit 6-5: ATB_SRC_CKSEL,
	Bit  4: ATB_CKSEL,    Bit 3: DBG_CKSEL,    Bit   2: ARM7_CKSEL
	Bit  1: PSMCU_CKSEL,  Bit 0: BUS_CKSEL*/
#define R_FLEXCKGEN_SEL0		(0x0028) /* Bit  29-28: EQ_CLK src = EQPLL
	Bit  26-24: EQ+DIVSEL, divided-by bit[2:0]+1
	Bit  21-20: BRP_CLK src = IMCPLL
	Bit  13-12: ARM7_CLK src = DFEPLL
	Bit  5-4: BUS_CLK src = EQPLL
	Bit  2-0: BUS_DIVSEL, divided-by bit[2:0]+1 */
#define R_FLEXCKGEN_SEL1		(0x002C)
#define R_FLEXCKGEN_SEL2		(0x0044)
/* Bit  0: PSMCUPLL_RDY, Bit  1: L1MCUPLL_RDY */
#define R_PLL_STS			(0x0040)
#define R_FLEXCKGEN_STS0		(0x0030)
	/* Bit  31: EQ_CK_RDY */
	/* Bit  23: BRP_CK_RDY */
	/* Bit  7: BUS_CK_RDY */
#define R_FLEXCKGEN_STS1		(0x0034)
	/* Bit  31: DFE_CK_RDY
	Bit  23: CMP_CK_RDY
	Bit  15: ICC_CK_RDY
	Bit  7:  IMC_CK_RDY */
#define R_FLEXCKGEN_STS2		(0x0048)
	/* Bit  15: INTF_CK_RDY, Bit  23: MD2G_CK_RDY */
/* PSMCU DCM: MD_GLOBAL_CON_DCM_BASE+ */
/* Bit 26-20: DBC_CNT, Bit 16-12: IDLE_FSEL, Bit 2: DBC_EN, Bit 1: DCM_EN */
#define R_PSMCU_DCM_CTL0		(0x0010)
/* Bit 5: APB_LOAD_TOG, Bit 4-0: APB_SEL */
#define R_PSMCU_DCM_CTL1		(0x0014)
/* MD_GLOBAL_CON_DCM_BASE+ */
/* Bit 26-20: DBC_CNT, Bit 16-12: IDLE_FSEL, Bit 2: DBC_EN, Bit 1: DCM_EN */
#define R_ARM7_DCM_CTL0			(0x0020)
#define R_ARM7_DCM_CTL1			(0x0024)/* Bit	5: APB_LOAD_TOG, Bit 4-0: APB_SEL */
#define MD_GLOBAL_CON_DUMMY		(0x1000)
#define MD_PLL_MAGIC_NUM		(0x67550000)
/* MDSYS_CLKCTL_BASE+ */
#define R_DCM_SHR_SET_CTL		(0x0004)
	/* Bit  16: BUS_PLL_SWITCH
	Bit  15: BUS_QUARTER_FREQ_SEL
	Bit  14: BUS_SLOW_FREQ
	Bit 12-8: HFBUS_SFSEL
	Bit   4-0: HFBUS_FSEL */
#define R_LTEL2_BUS_DCM_CTL		(0x0010)
#define R_MDDMA_BUS_DCM_CTL		(0x0014)
#define R_MDREG_BUS_DCM_CTL		(0x0018)
#define R_MODULE_BUS2X_DCM_CTL		(0x001C)
#define R_MODULE_BUS1X_DCM_CTL		(0x0020)
#define R_MDINFRA_CKEN			(0x0044) /* Bit 31: PSPERI_MAS_DCM_EN
	Bit  30: PSPERI_SLV_DCM_EN
	Bit  4: SOE_CKEN
	Bit  3: BUSREC_CKEN
	Bit  2: BUSMON_CKEN
	Bit  1: MDUART2_CKEN
	Bit  0: MDUART1_CKEN */
#define R_MDPERI_CKEN			(0x0048)/* Bit 31: MDDBGSYS_DCM_EN
	Bit  21: USB0_LINK_CK_SEL
	Bit  20: USB1_LINK_CK_SEL
	Bit  17: TRACE_CKEN
	Bit  16: MDPERI_MISCREG_CKEN
	Bit  15: PCCIF_CKEN
	Bit  14: MDEINT_CKEN
	Bit  13: MDCFGCTL_CKEN
	Bit  12: MDRGU_CKEN
	Bit  11: A7OST_CKEN
	Bit  10: MDOST_CKEN
	Bit  9: MDTOPSM_CKEN
	Bit  8: MDCIRQ_CKEN
	Bit  7: MDECT_CKEN
	Bit  6: USIM2_CKEN
	Bit  5: USIM1_CKEN
	Bit  4: GPTMLITE_CKEN
	Bit  3: MDGPTM_CKEN
	Bit  2: I2C_CKEN
	Bit  1: MDGDMA_CKEN
	Bit  0: MDUART0_CKEN */
#define R_MDPERI_DCM_MASK		(0x0064)/* Bit  12: MDRGU_DCM_MASK
	Bit  11: A7OST_DCM_MASK
	Bit  10: MDOST_DCM_MASK
	Bit  9: MDTOPSM_DCM_MASK */
#define R_PSMCU_AO_CLK_CTL		(0x00C0)
/* MD_PERI_MISC_BASE+ */
#define R_L1_PMS			(0x00C4)
/* PMDL1A0_BASE+ */
#define REG_DCM_PLLCK_SEL		(0x0188) /* Bit 7: 0: clock do not from PLL, 1: clock from PLL */
#define R_L1MCU_PWR_AWARE		(0x0190)
#define R_L1AO_PWR_AWARE		(0x0194)
#define R_BUSL2DCM_CON3			(0x0198)
#define R_L1MCU_DCM_CON2		(0x0184)
#define R_L1MCU_DCM_CON			(0x0180)
/* ap_mixed_base+ */
#define AP_PLL_CON0				 0x0	/*	((UINT32P)(APMIXED_BASE+0x0))	*/
#define MDPLL1_CON0              0x2C8	/*	((UINT32P)(APMIXED_BASE+0x02C8))	*/

/* CCIF */
#define APCCIF_CON    (0x00)
#define APCCIF_BUSY   (0x04)
#define APCCIF_START  (0x08)
#define APCCIF_TCHNUM (0x0C)
#define APCCIF_RCHNUM (0x10)
#define APCCIF_ACK    (0x14)
#define APCCIF_CHDATA (0x100)
#define APCCIF_SRAM_SIZE 512
/* channel usage */
#define EXCEPTION_NONE (0)
/* AP to MD */
#define H2D_EXCEPTION_ACK (1)
#define H2D_EXCEPTION_CLEARQ_ACK (2)
#define H2D_FORCE_MD_ASSERT (3)
#define H2D_MPU_FORCE_ASSERT (4)
/* MD to AP */
#define D2H_EXCEPTION_INIT (1)
#define D2H_EXCEPTION_INIT_DONE (2)
#define D2H_EXCEPTION_CLEARQ_DONE (3)
#define D2H_EXCEPTION_ALLQ_RESET (4)
/* peer */
#define AP_MD_PEER_WAKEUP (5)
#define AP_MD_SEQ_ERROR (6)
#define AP_MD_CCB_WAKEUP (8)

struct md_pll_reg {
	void __iomem *md_clkSW;
	void __iomem *md_dcm;
	void __iomem *psmcu_misc;
	void __iomem *md_peri_misc;
	void __iomem *md_L1_a0;
	void __iomem *md_top_Pll;
	void __iomem *md_sys_clk;
	/*void __iomem *md_l1_conf;*/
	/* the follow is used for dump */
	void __iomem *md_pc_mon1;
	void __iomem *md_pc_mon2;
	void __iomem *md_busreg1;
	void __iomem *md_busreg2;
	void __iomem *md_busrec;
	void __iomem *md_ect_0;
	void __iomem *md_ect_1;
	void __iomem *md_ect_2;
	void __iomem *md_ect_3;
	void __iomem *md_bootup_0;
	void __iomem *md_bootup_1;
	void __iomem *md_bootup_2;
	void __iomem *md_bootup_3;
	void __iomem *md_clk_ctl01;
	void __iomem *md_clk_ctl02;
	void __iomem *md_clk_ctl03;
	void __iomem *md_clk_ctl04;
	void __iomem *md_clk_ctl05;
	void __iomem *md_clk_ctl06;
	void __iomem *md_clk_ctl07;
	void __iomem *md_clk_ctl08;
	void __iomem *md_clk_ctl09;
	void __iomem *md_clk_ctl10;
	void __iomem *md_clk_ctl11;
	void __iomem *md_clk_ctl12;
	void __iomem *md_clk_ctl13;
	void __iomem *md_clk_ctl14;
	void __iomem *md_clk_ctl15;
	void __iomem *md_boot_stats0;
	void __iomem *md_boot_stats1;
};
#define MD_BUSREG_DUMP_ADDR1   0x20060000
#define MD_BUSREG_DUMP_LEN1    (0xF8)
#define MD_BUSREG_DUMP_ADDR2   0x20200000
#define MD_BUSREG_DUMP_LEN2    (0xE4)
#define MD_BUSREC_DUMP_ADDR   0x203C0000
#define MD_BUSREC_DUMP_LEN    (0x41C)
#define MD_ECT_DUMP_ADDR0   0x200D0A00
#define MD_ECT_DUMP_LEN0    (0x30)
#define MD_ECT_DUMP_ADDR1   0x10478130
#define MD_ECT_DUMP_LEN1    (8)
#define MD_ECT_DUMP_ADDR2   0x1046B130
#define MD_ECT_DUMP_LEN2    (8)
#define MD_ECT_DUMP_ADDR3   0x1046C130
#define MD_ECT_DUMP_LEN3    (8)
#define MD_Bootup_DUMP_ADDR0   0x20060100
#define MD_Bootup_DUMP_LEN0    (4)
#define MD_Bootup_DUMP_ADDR1   0x20060200
#define MD_Bootup_DUMP_LEN1    (4)
#define MD_Bootup_DUMP_ADDR2   0x20141000
#define MD_Bootup_DUMP_LEN2    (4)
#define MD_Bootup_DUMP_ADDR3   0x20070498
#define MD_Bootup_DUMP_LEN3    (4)
#define MD_Clkctrl_DUMP_ADDR01    0x20150100
#define MD_Clkctrl_DUMP_LEN01  0x18
#define MD_Clkctrl_DUMP_ADDR02    0x20151000
#define MD_Clkctrl_DUMP_LEN02  0x4
#define MD_Clkctrl_DUMP_ADDR03    0x20140000
#define MD_Clkctrl_DUMP_LEN03  0x9C
#define MD_Clkctrl_DUMP_ADDR04    0x20140200
#define MD_Clkctrl_DUMP_LEN04  0x8
#define MD_Clkctrl_DUMP_ADDR05    0x20140300
#define MD_Clkctrl_DUMP_LEN05  0x10
#define MD_Clkctrl_DUMP_ADDR06    0x20140400
#define MD_Clkctrl_DUMP_LEN06  0x138
#define MD_Clkctrl_DUMP_ADDR07    0x20140600
#define MD_Clkctrl_DUMP_LEN07  0x34
#define MD_Clkctrl_DUMP_ADDR08    0x20140800
#define MD_Clkctrl_DUMP_LEN08  0x8
#define MD_Clkctrl_DUMP_ADDR09    0x20130000
#define MD_Clkctrl_DUMP_LEN09  0x50
#define MD_Clkctrl_DUMP_ADDR10    0x20130080
#define MD_Clkctrl_DUMP_LEN10  0x90
#define MD_Clkctrl_DUMP_ADDR11    0x20130400
#define MD_Clkctrl_DUMP_LEN11  0x4
#define MD_Clkctrl_DUMP_ADDR12    0x20130800
#define MD_Clkctrl_DUMP_LEN12  0x8
#define MD_Clkctrl_DUMP_ADDR13    0x20130C00
#define MD_Clkctrl_DUMP_LEN13  0x10
#define MD_Clkctrl_DUMP_ADDR14    0x20131000
#define MD_Clkctrl_DUMP_LEN14  0x4
#define MD_Clkctrl_DUMP_ADDR15    0x20120000
#define MD_Clkctrl_DUMP_LEN15  0xC4

struct md_hw_info {
	/* HW info - Register Address */
	unsigned long cldma_ap_ao_base;
	unsigned long cldma_md_ao_base;
	unsigned long cldma_ap_pdn_base;
	unsigned long cldma_md_pdn_base;
	unsigned long md_rgu_base;
	unsigned long l1_rgu_base;
	unsigned long ap_mixed_base;
	unsigned long md_boot_slave_Vector;
	unsigned long md_boot_slave_Key;
	unsigned long md_boot_slave_En;
	unsigned long ap_ccif_base;
	unsigned long md_ccif_base;
	unsigned int sram_size;

	/* HW info - Interrutpt ID */
	unsigned int cldma_irq_id;
	unsigned int ap_ccif_irq_id;
	unsigned int md_wdt_irq_id;
	unsigned int ap2md_bus_timeout_irq_id;

	/* HW info - Interrupt flags */
	unsigned long cldma_irq_flags;
	unsigned long ap_ccif_irq_flags;
	unsigned long md_wdt_irq_flags;
	unsigned long ap2md_bus_timeout_irq_flags;
};

int ccci_modem_remove(struct platform_device *dev);
void ccci_modem_shutdown(struct platform_device *dev);
int ccci_modem_suspend(struct platform_device *dev, pm_message_t state);
int ccci_modem_resume(struct platform_device *dev);
int ccci_modem_pm_suspend(struct device *device);
int ccci_modem_pm_resume(struct device *device);
int ccci_modem_pm_restore_noirq(struct device *device);
int md_cd_power_on(struct ccci_modem *md);
int md_cd_power_off(struct ccci_modem *md, unsigned int stop_type);
int md_cd_soft_power_off(struct ccci_modem *md, unsigned int mode);
int md_cd_soft_power_on(struct ccci_modem *md, unsigned int mode);
int md_cd_let_md_go(struct ccci_modem *md);
void md_cd_lock_cldma_clock_src(int locked);
void md_cd_lock_modem_clock_src(int locked);
int md_cd_bootup_cleanup(struct ccci_modem *md, int success);
int md_cd_low_power_notify(struct ccci_modem *md, LOW_POEWR_NOTIFY_TYPE type, int level);
int md_cd_get_modem_hw_info(struct platform_device *dev_ptr, struct ccci_dev_cfg *dev_cfg, struct md_hw_info *hw_info);
int md_cd_io_remap_md_side_register(struct ccci_modem *md);
void md_cd_dump_debug_register(struct ccci_modem *md);
void md_cd_dump_md_bootup_status(struct ccci_modem *md);
void md_cd_check_emi_state(struct ccci_modem *md, int polling);
void cldma_dump_register(struct ccci_modem *md);
void md_cldma_hw_reset(struct ccci_modem *md);
/* ADD_SYS_CORE */
int ccci_modem_syssuspend(void);
void ccci_modem_sysresume(void);
void md_cd_check_md_DCM(struct ccci_modem *md);

extern unsigned long infra_ao_base;
extern void ccci_mem_dump(int md_id, void *start_addr, int len);
extern void subsys_if_on(void);
#endif				/* __CLDMA_PLATFORM_H__ */

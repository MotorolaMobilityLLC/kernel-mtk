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
#include <linux/clk.h>
/* this is the platform header file for CLDMA MODEM, not just CLDMA! */

/* ap_mixed_base+ */
#define AP_PLL_CON0		0x0	/*	((UINT32P)(APMIXED_BASE+0x0))	*/
#define MDPLL1_CON0		0x2C8	/*	((UINT32P)(APMIXED_BASE+0x2C8))	*/
#define MDPLL1_CON2		0x2D0	/*	((UINT32P)(APMIXED_BASE+0x2D0))	*/

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

struct ccci_clk_node {
	struct clk *clk_ref;
	unsigned char *clk_name;
};

struct md_pll_reg {
	void __iomem *md_clkSW;
	void __iomem *md_dcm;
	void __iomem *md_peri_misc;
	void __iomem *md_L1_a0;
	void __iomem *md_top_Pll;
	void __iomem *md_sys_clk;
	/*void __iomem *md_l1_conf;*/
	/* the follow is used for dump */
	void __iomem *md_pc_mon1;
	void __iomem *md_pc_mon2;
	void __iomem *md_dbgsys_clk;
	void __iomem *md_busreg1;
	void __iomem *md_busreg2;
	void __iomem *md_busrec;
	void __iomem *md_busrec_L1;
	void __iomem *md_ect_1;
	void __iomem *md_ect_2;
	void __iomem *md_ect_3;
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

	/* HW info - Interrupt flags */
	unsigned long cldma_irq_flags;
	unsigned long ap_ccif_irq_flags;
	unsigned long md_wdt_irq_flags;
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
#endif				/* __CLDMA_PLATFORM_H__ */

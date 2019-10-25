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

#ifndef __MD_SYS1_PLATFORM_H__
#define __MD_SYS1_PLATFORM_H__

#include "ccif_hif_platform.h"
#include <linux/clk.h>
#include <linux/skbuff.h>
/* this is the platform header file for CLDMA MODEM, not just CLDMA! */

/* ap_mixed_base+ */
#define AP_PLL_CON0 0x0   /*	((UINT32P)(APMIXED_BASE+0x0))	*/
#define MDPLL1_CON0 0x2C8 /*	((UINT32P)(APMIXED_BASE+0x2C8))	*/
#define MDPLL1_CON2 0x2D0 /*	((UINT32P)(APMIXED_BASE+0x2D0))	*/

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
	unsigned int ap_ccif_irq_id;
	unsigned int md_wdt_irq_id;

	/* HW info - Interrupt flags */
	unsigned long ap_ccif_irq_flags;
	unsigned long md_wdt_irq_flags;
	void *hif_hw_info;
};

struct cldma_hw_info {
	unsigned long cldma_ap_ao_base;
	unsigned long cldma_md_ao_base;
	unsigned long cldma_ap_pdn_base;
	unsigned long cldma_md_pdn_base;
	unsigned int cldma_irq_id;
	unsigned long cldma_irq_flags;
};

static inline int md_cd_pccif_send(struct ccci_modem *md, int channel_id)
{
	return 0;
}

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
int md_cd_low_power_notify(struct ccci_modem *md, LOW_POEWR_NOTIFY_TYPE type,
			   int level);
int md_cd_get_modem_hw_info(struct platform_device *dev_ptr,
			    struct ccci_dev_cfg *dev_cfg,
			    struct md_hw_info *hw_info);
int md_cd_io_remap_md_side_register(struct ccci_modem *md);
void md_cd_dump_debug_register(struct ccci_modem *md);
void md_cd_dump_md_bootup_status(struct ccci_modem *md);
void md_cd_check_emi_state(struct ccci_modem *md, int polling);
void cldma_dump_register(struct md_cd_ctrl *md_ctrl);
void md_cldma_hw_reset(unsigned char md_id);
/* ADD_SYS_CORE */
int ccci_modem_syssuspend(void);
void ccci_modem_sysresume(void);
void md_cd_check_md_DCM(struct md_cd_ctrl *md_ctrl);
void md1_sleep_timeout_proc(void);

extern unsigned long infra_ao_base;
#ifdef FEATURE_BSI_BPI_SRAM_CFG
extern void __iomem *ap_sleep_base;
extern void __iomem *ap_topckgen_base;
extern atomic_t md_power_on_off_cnt;
#endif
extern void ccci_mem_dump(int md_id, void *start_addr, int len);
#endif /* __CLDMA_PLATFORM_H__ */

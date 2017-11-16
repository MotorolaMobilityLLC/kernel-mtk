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

#include <linux/delay.h>
#include <linux/kthread.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mt_boot_common.h>
#include <mach/mt_clkmgr.h>
#include "ccci_off.h"
#if !defined(CONFIG_MTK_CCCI_EXT) || defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)

static void internal_md_power_down(void)
{
	/**
	 * MDMCU Control Power Registers
	 */
	#define MD_TOPSM_BASE					(0x20030000)
	#define MD_TOPSM_RM_TMR_PWR0(base)			((volatile unsigned int *)((base) + 0x0018))
	#define MD_TOPSM_SM_REQ_MASK(base)			((volatile unsigned int *)((base) + 0x08B0))
	#define MD_TOPSM_RM_PWR_CON0(base)			((volatile unsigned int *)((base) + 0x0800))
	#define MD_TOPSM_RM_PWR_CON1(base)			((volatile unsigned int *)((base) + 0x0804))
	#define MD_TOPSM_RM_PLL_MASK0(base)			((volatile unsigned int *)((base) + 0x0830))
	#define MD_TOPSM_RM_PLL_MASK1(base)			((volatile unsigned int *)((base) + 0x0834))

	#define MODEM2G_TOPSM_BASE				(0x23010000)
	#define MODEM2G_TOPSM_RM_CLK_SETTLE(base)		((volatile unsigned int *)((base) + 0x0000))
	#define MODEM2G_TOPSM_RM_TMRPWR_SETTLE(base)		((volatile unsigned int *)((base) + 0x0004))
	#define MODEM2G_TOPSM_RM_TMR_TRG0(base)			((volatile unsigned int *)((base) + 0x0010))
	#define MODEM2G_TOPSM_RM_TMR_TRG1(base)			((volatile unsigned int *)((base) + 0x0014))
	#define MODEM2G_TOPSM_RM_TMR_PWR0(base)			((volatile unsigned int *)((base) + 0x0018))
	#define MODEM2G_TOPSM_RM_TMR_PWR1(base)			((volatile unsigned int *)((base) + 0x001C))
	#define MODEM2G_TOPSM_RM_PWR_CON0(base)			((volatile unsigned int *)((base) + 0x0800))
	#define MODEM2G_TOPSM_RM_PWR_CON1(base)			((volatile unsigned int *)((base) + 0x0804))
	#define MODEM2G_TOPSM_RM_PWR_CON2(base)			((volatile unsigned int *)((base) + 0x0808))
	#define MODEM2G_TOPSM_RM_PWR_CON3(base)			((volatile unsigned int *)((base) + 0x080C))
	#define MODEM2G_TOPSM_RM_PWR_CON4(base)			((volatile unsigned int *)((base) + 0x0810))
	#define MODEM2G_TOPSM_RM_PWR_CON5(base)			((volatile unsigned int *)((base) + 0x0814))
	#define MODEM2G_TOPSM_RM_PWR_CON6(base)			((volatile unsigned int *)((base) + 0x0818))
	#define MODEM2G_TOPSM_RM_PWR_CON7(base)			((volatile unsigned int *)((base) + 0x081C))
	#define MODEM2G_TOPSM_RM_PLL_MASK0(base)		((volatile unsigned int *)((base) + 0x0830))
	#define MODEM2G_TOPSM_RM_PLL_MASK1(base)		((volatile unsigned int *)((base) + 0x0834))
	#define MODEM2G_TOPSM_RM_PLL_MASK2(base)		((volatile unsigned int *)((base) + 0x0838))
	#define MODEM2G_TOPSM_RM_PLL_MASK3(base)		((volatile unsigned int *)((base) + 0x083C))
	#define MODEM2G_TOPSM_SM_REQ_MASK(base)			((volatile unsigned int *)((base) + 0x08B0))
	/**
	 *[TDD] MDMCU Control Power Registers
	 */
	#define TDD_BASE					(0x24000000)
	#define TDD_HALT_CFG_ADDR(base)				((volatile unsigned int *)((base) + 0x0000))
	#define TDD_HALT_STATUS_ADDR(base)			((volatile unsigned int *)((base) + 0x0002))

	/*unsigned short status;*/
	/*unsigned short i;*/
	unsigned int md_topsm_base, modem2g_topsm_base, tdd_base;

	pr_notice("[ccci-off]internal md disabled, so power down!\n");

	md_power_on(0);

	md_topsm_base		= (unsigned int)ioremap_nocache(MD_TOPSM_BASE, 0x840);
	modem2g_topsm_base	= (unsigned int)ioremap_nocache(MODEM2G_TOPSM_BASE, 0x8C0);
	tdd_base		= (unsigned int)ioremap_nocache(TDD_BASE, 0x010);

	pr_notice("[ccci-off]MD2G/HSPA power down...\n");

	/*[MD2G/HSPA] MDMCU Control Power Down Sequence*/
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base)) | 0x44,
		MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base)) | 0x44,
		MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base)) | 0x44,
		MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base)) | 0x44,
		MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base)) | 0x44,
		MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base));

	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base)) & 0xFFFFFF7F,
		MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000200, MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base)) & 0xFFFFFF7F,
		MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000200, MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base)) & 0xFFFFFF7F,
		MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000200, MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base)) & 0xFFFFFF7F,
		MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000200, MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base)) & 0xFFFFFF7F,
		MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000200, MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base));

	mt_reg_sync_writel(0xFFFFFFFF, MD_TOPSM_SM_REQ_MASK(md_topsm_base));
	mt_reg_sync_writel(0x00000000, MODEM2G_TOPSM_RM_TMR_PWR0(modem2g_topsm_base));
	mt_reg_sync_writel(0x00000000, MODEM2G_TOPSM_RM_TMR_PWR1(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base)) & ~(0x1<<2) & ~(0x1<<6),
		MODEM2G_TOPSM_RM_PWR_CON0(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base)) & ~(0x1<<2) & ~(0x1<<6),
		MODEM2G_TOPSM_RM_PWR_CON1(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base)) & ~(0x1<<2) & ~(0x1<<6),
		MODEM2G_TOPSM_RM_PWR_CON2(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base)) & ~(0x1<<2) & ~(0x1<<6),
		MODEM2G_TOPSM_RM_PWR_CON3(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base)) & ~(0x1<<2) & ~(0x1<<6),
		MODEM2G_TOPSM_RM_PWR_CON4(modem2g_topsm_base));

#if 0
	pr_debug("[ccci-off]TDD power down...\n");

	/*[TDD] MDMCU Control Power Down Sequence*/
	mt_reg_sync_writew(0x1, TDD_HALT_CFG_ADDR(tdd_base));
	status = *((volatile unsigned short *)TDD_HALT_STATUS_ADDR(tdd_base));
	while ((status & 0x1) == 0) {
		/*
		* status & 0x1 TDD is in *HALT* STATE
		* status & 0x2 TDD is in *NORMAL* STATE
		* status & 0x4 TDD is in *SLEEP* STATE
		*/
		i = 100;
		while (i--)
			;
		status = *((volatile unsigned short *)TDD_HALT_STATUS_ADDR(tdd_base));
	}
#endif
	pr_debug("[ccci-off]ABB power down...\n");

	/*[ABB] MDMCU Control Power Down Sequence*/
	mt_reg_sync_writel((*MD_TOPSM_RM_PWR_CON0(md_topsm_base))  | 0x00000090,
		MD_TOPSM_RM_PWR_CON0(md_topsm_base));
	mt_reg_sync_writel((*MD_TOPSM_RM_PLL_MASK0(md_topsm_base)) | 0xFFFF0000,
		MD_TOPSM_RM_PLL_MASK0(md_topsm_base));
	mt_reg_sync_writel((*MD_TOPSM_RM_PLL_MASK1(md_topsm_base)) | 0x000000FF,
		MD_TOPSM_RM_PLL_MASK1(md_topsm_base));
	/*mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PLL_MASK0(modem2g_topsm_base)) | 0xFFFFFFFF,*/
	mt_reg_sync_writel(0xFFFFFFFF,
		MODEM2G_TOPSM_RM_PLL_MASK0(modem2g_topsm_base));
	mt_reg_sync_writel((*MODEM2G_TOPSM_RM_PLL_MASK1(modem2g_topsm_base)) | 0x0000000F,
		MODEM2G_TOPSM_RM_PLL_MASK1(modem2g_topsm_base));

	pr_debug("[ccci-off]MDMCU power down...\n");

	/*[MDMCU] APMCU Control Power Down Sequence*/
	mt_reg_sync_writel(0xFFFFFFFF, MD_TOPSM_SM_REQ_MASK(md_topsm_base));
	mt_reg_sync_writel(0x00000000, MD_TOPSM_RM_TMR_PWR0(md_topsm_base));
	mt_reg_sync_writel(0x0005229A, MD_TOPSM_RM_PWR_CON0(md_topsm_base));
	mt_reg_sync_writel(0xFFFFFFFF, MD_TOPSM_RM_PLL_MASK0(md_topsm_base));
	mt_reg_sync_writel(0xFFFFFFFF, MD_TOPSM_RM_PLL_MASK1(md_topsm_base));

	mt_reg_sync_writel(0xFFFFFFFF, MODEM2G_TOPSM_SM_REQ_MASK(modem2g_topsm_base));
	mt_reg_sync_writel(0xFFFFFFFF, MODEM2G_TOPSM_RM_PLL_MASK0(modem2g_topsm_base));
	mt_reg_sync_writel(0xFFFFFFFF, MODEM2G_TOPSM_RM_PLL_MASK1(modem2g_topsm_base));

	pr_debug("[ccci-off]call md_power_off...\n");

	md_power_off(0, 100);

	iounmap((void *)md_topsm_base);
	iounmap((void *)modem2g_topsm_base);
	iounmap((void *)tdd_base);

	pr_debug("[ccci-off]internal md power down done...\n");
}

static int modem_power_down_worker(void *data)
{
	unsigned int val;

	val = get_devinfo_with_index(3);
	if ((val & (0x1 << 16)) == 0)
		internal_md_power_down();
	else
		pr_debug("[ccci-off]md1 effused,no need power off\n");
	return 0;
}
static void modem_power_down(void)
{
	/* start kthread to power down modem */
	struct task_struct *md_power_kthread;

	md_power_kthread = kthread_run(modem_power_down_worker, NULL, "md_power_off_kthread");
	if (IS_ERR(md_power_kthread)) {
		pr_debug("[ccci-off] create kthread for power off md fail, only direct call API\n");
		modem_power_down_worker(NULL);
	} else {
		pr_debug("[ccci-off] create kthread for power off md ok\n");
	}
}
#endif

static int __init modem_off_init(void)
{
#ifndef CONFIG_MTK_CCCI_EXT
	modem_power_down();
#else
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if ((get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT) || (get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT)) {
		pr_debug("[ccci-off]power off MD in charging mode %d\n", get_boot_mode());
		modem_power_down();
	}
#endif
#endif
	return 0;
}
module_init(modem_off_init);

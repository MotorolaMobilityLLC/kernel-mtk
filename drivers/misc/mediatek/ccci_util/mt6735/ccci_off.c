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
#if defined(CONFIG_MTK_CLKMGR)
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif	/*CONFIG_MTK_CLKMGR */

#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include "ccci_off.h"

#if !defined(CONFIG_MTK_CLKMGR)
static struct clk *clk_scp_sys_md1_main;
#endif

#if !defined(CONFIG_MTK_ECCCI_DRIVER) || defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#define sync_write32(v, a)			mt_reg_sync_writel(v, a)
#define sync_write16(v, a)			mt_reg_sync_writew(v, a)
#define sync_write8(v, a)			mt_reg_sync_writeb(v, a)

#define MD_TOPSM_BASE				(0x20030000)
#define MD_TOPSM_RM_TMR_PWR0(base)		((base)+0x0018)
#define MD_TOPSM_RM_PWR_CON0(base)		((base)+0x0800)
#define MD_TOPSM_RM_PWR_CON1(base)		((base)+0x0804)
#define MD_TOPSM_RM_PWR_CON2(base)		((base)+0x0808)
#define MD_TOPSM_RM_PWR_CON3(base)		((base)+0x080C)
#define MD_TOPSM_RM_PLL_MASK0(base)		((base)+0x0830)
#define MD_TOPSM_RM_PLL_MASK1(base)		((base)+0x0834)
#define MD_TOPSM_SM_REQ_MASK(base)		((base)+0x08B0)

#define MODEM_LITE_TOPSM_BASE			(0x23010000)

#define MODEM_LITE_TOPSM_RM_TMR_PWR0(base)	((base)+0x0018)
#define MODEM_LITE_TOPSM_RM_TMR_PWR1(base)	((base)+0x001C)
#define MODEM_LITE_TOPSM_RM_PWR_CON0(base)	((base)+0x0800)
#define MODEM_LITE_TOPSM_RM_PLL_MASK0(base)	((base)+0x0830)
#define MODEM_LITE_TOPSM_RM_PLL_MASK1(base)	((base)+0x0834)
#define MODEM_LITE_TOPSM_SM_REQ_MASK(base)	((base)+0x08B0)

#define MODEM_TOPSM_BASE			(0x27010000)

#define MODEM_TOPSM_RM_TMR_PWR0(base)		((base)+0x0018)
#define MODEM_TOPSM_RM_TMR_PWR1(base)		((base)+0x001C)
#define MODEM_TOPSM_RM_PWR_CON1(base)		((base)+0x0804)
#define MODEM_TOPSM_RM_PWR_CON2(base)		((base)+0x0808)
#define MODEM_TOPSM_RM_PWR_CON3(base)		((base)+0x080C)
#define MODEM_TOPSM_RM_PWR_CON4(base)		((base)+0x0810)
#define MODEM_TOPSM_RM_PLL_MASK0(base)		((base)+0x0830)
#define MODEM_TOPSM_RM_PLL_MASK1(base)		((base)+0x0834)
#define MODEM_TOPSM_SM_REQ_MASK(base)		((base)+0x08B0)

#define TDD_BASE				(0x24000000)
#define TDD_HALT_CFG_ADDR(base)			((base)+0x00000000)
#define TDD_HALT_STATUS_ADDR(base)		((base)+0x00000002)

#define LTEL1_BASE				(0x26600000)

#define MD_PLL_MIXEDSYS_BASE			(0x20120000)
#define PLL_PLL_CON2_1(base)			((base)+0x0024)
#define PLL_PLL_CON4(base)			((base)+0x0050)
#define PLL_CLKSW_CKSEL4(base)			((base)+0x0094)
#define PLL_CLKSW_CKSEL6(base)			((base)+0x009C)
#define PLL_DFS_CON7(base)			((base)+0x00AC)
#define PLL_MDPLL_CON0(base)			((base)+0x0100)
#define PLL_ARM7PLL_CON0(base)			((base)+0x0150)
#define PLL_ARM7PLL_CON1(base)			((base)+0x0154)

static void internal_md_power_down(void)
{
	int ret = 0;
	unsigned short status, i;
	void __iomem *md_topsm_base, *modem_lite_topsm_base, *modem_topsm_base,
	    *tdd_base, *ltelt1_base, *ltelt1_base_1, *ltelt1_base_2,
	    *md_pll_mixedsys_base;

	pr_debug("[ccci-off]shutdown MDSYS1 !!!\n");
#if defined(CONFIG_MTK_CLKMGR)
	pr_debug("[ccci-off]Call start md_power_on()\n");
	ret = md_power_on(SYS_MD1);
	pr_debug("[ccci-off]Call end md_power_on() ret=%d\n", ret);
#else
	pr_debug("[ccci-off]Call start clk_prepare_enable()\n");
	ret = clk_prepare_enable(clk_scp_sys_md1_main);
	pr_debug("[ccci-off]Call end clk_prepare_enable()ret=%d\n", ret);
#endif
	pr_debug("[ccci-off]0.power on MD_INFRA/MODEM_TOP ret=%d\n", ret);
	if (ret)
		return;

	md_topsm_base = ioremap_nocache(MD_TOPSM_BASE, 0x8C0);
	modem_lite_topsm_base = ioremap_nocache(MODEM_LITE_TOPSM_BASE, 0x08C0);
	modem_topsm_base = ioremap_nocache(MODEM_TOPSM_BASE, 0x8C0);
	tdd_base = ioremap_nocache(TDD_BASE, 0x010);
	ltelt1_base = ioremap_nocache(LTEL1_BASE, 0x60000);
	ltelt1_base_1 = ioremap_nocache(0x2012045C, 0x4);
	ltelt1_base_2 = ioremap_nocache(0x200308B0, 0x4);
	md_pll_mixedsys_base = ioremap_nocache(MD_PLL_MIXEDSYS_BASE, 0x160);

	pr_debug("[ccci-off]1.power on MD2G/HSPA\n");
	/* power on MD2G */
	sync_write32(ioread32(MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base)) | 0x44,
		     MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base));
	/* power on MD2G SRAM */
#if 0
	sync_write32(ioread32(MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base)) | 0x88,
		     MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base));
#endif
	/* power on HSPA */
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base)) | 0x44,
		     MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base)) | 0x44,
		     MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base)) | 0x44,
		     MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base)) | 0x44,
		     MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base));
	/* power on HSPA SRAM */
#if 0
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base)) | 0x88,
		     MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base)) | 0x88,
		     MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base)) | 0x88,
		     MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base)) | 0x88,
		     MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base));
#endif

	pr_debug("[ccci-off]2.power off MD2G/HSPA\n");
	/* power off MD2G */
	sync_write32(0xFFFFFFFF, MD_TOPSM_SM_REQ_MASK(md_topsm_base));
	sync_write32(0, MODEM_LITE_TOPSM_RM_TMR_PWR0(modem_lite_topsm_base));
	sync_write32(0, MODEM_LITE_TOPSM_RM_TMR_PWR1(modem_lite_topsm_base));
	sync_write32(ioread32(MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MODEM_LITE_TOPSM_RM_PWR_CON0(modem_lite_topsm_base));
	/* power off HSPA */
	sync_write32(0xFFFFFFFF, MD_TOPSM_SM_REQ_MASK(md_topsm_base));
	sync_write32(0, MODEM_TOPSM_RM_TMR_PWR0(modem_topsm_base));
	sync_write32(0, MODEM_TOPSM_RM_TMR_PWR1(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MODEM_TOPSM_RM_PWR_CON1(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MODEM_TOPSM_RM_PWR_CON2(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MODEM_TOPSM_RM_PWR_CON3(modem_topsm_base));
	sync_write32(ioread32(MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MODEM_TOPSM_RM_PWR_CON4(modem_topsm_base));

	pr_debug("[ccci-off]3.power off TDD\n");
	sync_write16(0x1, TDD_HALT_CFG_ADDR(tdd_base));
	status = ioread16(TDD_HALT_STATUS_ADDR(tdd_base));
	while ((status & 0x1) == 0) {
		/*
		* status & 0x1 TDD is in *HALT* STATE
		* status & 0x2 TDD is in *NORMAL* STATE
		* status & 0x4 TDD is in *SLEEP* STATE
		*/
		i = 100;
		while (i--)
			;
		status = ioread16(TDD_HALT_STATUS_ADDR(tdd_base));
	}

	pr_debug("[ccci-off]4.power off LTEL1\n");
	sync_write32(0x01FF, PLL_DFS_CON7(md_pll_mixedsys_base));
	sync_write32(0x0010, PLL_PLL_CON4(md_pll_mixedsys_base));
	sync_write32(0x6000, PLL_ARM7PLL_CON1(md_pll_mixedsys_base));
	sync_write32(0x2000, PLL_ARM7PLL_CON1(md_pll_mixedsys_base));
	sync_write32(ioread32(PLL_ARM7PLL_CON0(md_pll_mixedsys_base)) | 0x8000, PLL_ARM7PLL_CON0(md_pll_mixedsys_base));
	sync_write32(ioread32(PLL_MDPLL_CON0(md_pll_mixedsys_base)) | 0x8000, PLL_MDPLL_CON0(md_pll_mixedsys_base));
	sync_write32(0x4500, PLL_CLKSW_CKSEL4(md_pll_mixedsys_base));
	sync_write32(0x0003, PLL_CLKSW_CKSEL6(md_pll_mixedsys_base));

	sync_write32(0x21008510, ltelt1_base_1);
	sync_write32(ioread32(ltelt1_base_2) | 0xC, ltelt1_base_2);

	sync_write32(0x01010101, ltelt1_base + 0x030c8);
	sync_write32(0x10041000, ltelt1_base + 0x0306c);
	sync_write32(0x10041000, ltelt1_base + 0x03070);
	sync_write32(0x10041000, ltelt1_base + 0x03074);

	sync_write32(0x10040000, ltelt1_base + 0x0306c);
	sync_write32(0x10040000, ltelt1_base + 0x03070);
	sync_write32(0x10040000, ltelt1_base + 0x03074);
	sync_write32(0x10040000, ltelt1_base + 0x03078);

	sync_write32(0x88888888, ltelt1_base + 0x02000);
	sync_write32(0x88888888, ltelt1_base + 0x02004);
	sync_write32(0x88888888, ltelt1_base + 0x02008);
	sync_write32(0x88888888, ltelt1_base + 0x0200c);

	sync_write32(0x88888888, ltelt1_base + 0x32000);
	sync_write32(0x88888888, ltelt1_base + 0x32004);

	sync_write32(0x88888888, ltelt1_base + 0x22000);
	sync_write32(0x88888888, ltelt1_base + 0x22004);
	sync_write32(0x88888888, ltelt1_base + 0x22008);
	sync_write32(0x88888888, ltelt1_base + 0x2200c);
	sync_write32(0x88888888, ltelt1_base + 0x22010);
	sync_write32(0x88888888, ltelt1_base + 0x22014);
	sync_write32(0x88888888, ltelt1_base + 0x22018);
	sync_write32(0x88888888, ltelt1_base + 0x2201c);

	sync_write32(0x88888888, ltelt1_base + 0x42000);
	sync_write32(0x88888888, ltelt1_base + 0x42004);
	sync_write32(0x88888888, ltelt1_base + 0x42008);

	sync_write32(0x88888888, ltelt1_base + 0x52000);
	sync_write32(0x88888888, ltelt1_base + 0x52004);
	sync_write32(0x88888888, ltelt1_base + 0x52008);
	sync_write32(0x88888888, ltelt1_base + 0x5200c);

	sync_write32(0x88888888, ltelt1_base + 0x12000);
	sync_write32(0x88888888, ltelt1_base + 0x12004);
	sync_write32(0x88888888, ltelt1_base + 0x12008);
	sync_write32(0x88888888, ltelt1_base + 0x1200c);

	sync_write32(0x0000000C, ltelt1_base + 0x031b4);
	sync_write32(0x00520C41, ltelt1_base + 0x031c4);

	sync_write32(0x00000004, ltelt1_base + 0x02030);
	sync_write32(0x00000008, ltelt1_base + 0x02034);
	sync_write32(0x0000000C, ltelt1_base + 0x02038);
	sync_write32(0x00000010, ltelt1_base + 0x0203c);
	sync_write32(0x00000018, ltelt1_base + 0x02040);

	sync_write32(0x00000004, ltelt1_base + 0x32028);
	sync_write32(0x00000008, ltelt1_base + 0x3202c);
	sync_write32(0x0000000C, ltelt1_base + 0x32030);
	sync_write32(0x00000010, ltelt1_base + 0x32034);
	sync_write32(0x00000018, ltelt1_base + 0x32038);

	sync_write32(0x00080004, ltelt1_base + 0x22044);
	sync_write32(0x00100008, ltelt1_base + 0x22048);
	sync_write32(0x0000000C, ltelt1_base + 0x2204c);
	sync_write32(0x00000010, ltelt1_base + 0x22050);
	sync_write32(0x00000018, ltelt1_base + 0x22054);

	sync_write32(0x00000004, ltelt1_base + 0x4202c);
	sync_write32(0x00000008, ltelt1_base + 0x42030);
	sync_write32(0x0000000C, ltelt1_base + 0x42034);
	sync_write32(0x00000010, ltelt1_base + 0x42038);
	sync_write32(0x00000018, ltelt1_base + 0x4203c);

	sync_write32(0x00000004, ltelt1_base + 0x5202c);
	sync_write32(0x00000008, ltelt1_base + 0x52030);
	sync_write32(0x0000000C, ltelt1_base + 0x52034);
	sync_write32(0x00000010, ltelt1_base + 0x52038);
	sync_write32(0x00000018, ltelt1_base + 0x5203c);

	sync_write32(0x00000004, ltelt1_base + 0x1202c);
	sync_write32(0x00000008, ltelt1_base + 0x12030);
	sync_write32(0x0000000C, ltelt1_base + 0x12034);
	sync_write32(0x00000010, ltelt1_base + 0x12038);
	sync_write32(0x00000018, ltelt1_base + 0x1203c);

	sync_write32(0x05004321, ltelt1_base + 0x030a0);
	sync_write32(0x00432064, ltelt1_base + 0x030a4);
	sync_write32(0x0000000F, ltelt1_base + 0x03118);
	sync_write32(0x00000000, ltelt1_base + 0x03104);

	sync_write32(0x00000000, ltelt1_base + 0x03100);
	sync_write32(0x02020006, ltelt1_base + 0x03004);
	sync_write32(0x00000002, ltelt1_base + 0x03110);
	sync_write32(0x00000001, ltelt1_base + 0x030f0);

	sync_write32(ioread32(ltelt1_base + 0x030d4) | 0x1, ltelt1_base + 0x030d4);
	sync_write32(0x01010101, ltelt1_base + 0x030b8);
	sync_write32(0x01010101, ltelt1_base + 0x030bc);

	sync_write32(0x00000000, ltelt1_base + 0x04014);
	sync_write32(0x00000190, ltelt1_base + 0x04018);
	sync_write32(0x000000C8, ltelt1_base + 0x0401c);
	sync_write32(0x0000001E, ltelt1_base + 0x04028);

	sync_write32(0x00000001, ltelt1_base + 0x030d4);
	udelay(1000);
	sync_write32(0x00000030, ltelt1_base + 0x04058);
	udelay(1000);

	sync_write32(0x00000001, ltelt1_base + 0x03120);
	udelay(1000);
	sync_write32(0x00000001, ltelt1_base + 0x04000);
	udelay(1000);

	sync_write32(ioread32(PLL_ARM7PLL_CON0(md_pll_mixedsys_base)) & ~(0x8000),
		     PLL_ARM7PLL_CON0(md_pll_mixedsys_base));
	sync_write32(ioread32(PLL_MDPLL_CON0(md_pll_mixedsys_base)) & ~(0x8000), PLL_MDPLL_CON0(md_pll_mixedsys_base));
	sync_write32(0x6000, PLL_ARM7PLL_CON1(md_pll_mixedsys_base));
	sync_write32(0x4000, PLL_ARM7PLL_CON1(md_pll_mixedsys_base));

	pr_debug("[ccci-off]5.power off LTEL2/ARM7\n");
	/* power off LTEL2 */
	sync_write32(ioread32(MD_TOPSM_RM_PWR_CON2(md_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MD_TOPSM_RM_PWR_CON2(md_topsm_base));
	/* power off ARM7 */
	sync_write32(ioread32(MD_TOPSM_RM_PWR_CON3(md_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MD_TOPSM_RM_PWR_CON3(md_topsm_base));

	pr_debug("[ccci-off]6.power off ABB\n");
	sync_write32(ioread32(MD_TOPSM_RM_PWR_CON1(md_topsm_base)) & ~(0x1 << 2) & ~(0x1 << 6),
		     MD_TOPSM_RM_PWR_CON1(md_topsm_base));
	sync_write32(ioread32(MD_TOPSM_RM_PWR_CON1(md_topsm_base)) | 0x00000090, MD_TOPSM_RM_PWR_CON1(md_topsm_base));
	sync_write32(ioread32(MD_TOPSM_RM_PLL_MASK0(md_topsm_base)) | 0xFFFF0000, MD_TOPSM_RM_PLL_MASK0(md_topsm_base));

/*	sync_write32(ioread32(MD_TOPSM_RM_PLL_MASK1(md_topsm_base)) | 0xFFFFFFFF,
			MD_TOPSM_RM_PLL_MASK1(md_topsm_base));*/
	sync_write32(0xFFFFFFFF, MD_TOPSM_RM_PLL_MASK1(md_topsm_base));
/*	sync_write32(ioread32(MODEM_TOPSM_RM_PLL_MASK0(modem_topsm_base)) | 0xFFFFFFFF,
			MODEM_TOPSM_RM_PLL_MASK0(modem_topsm_base));*/
	sync_write32(0xFFFFFFFF, MODEM_TOPSM_RM_PLL_MASK0(modem_topsm_base));

	sync_write32(ioread32(MODEM_TOPSM_RM_PLL_MASK1(modem_topsm_base)) | 0x0000000F,
			MODEM_TOPSM_RM_PLL_MASK1(modem_topsm_base));
/*	sync_write32(ioread32(MODEM_LITE_TOPSM_RM_PLL_MASK0(modem_lite_topsm_base)) | 0xFFFFFFFF,
			MODEM_LITE_TOPSM_RM_PLL_MASK0(modem_lite_topsm_base));*/
	sync_write32(0xFFFFFFFF, MODEM_LITE_TOPSM_RM_PLL_MASK0(modem_lite_topsm_base));

	sync_write32(ioread32(MODEM_LITE_TOPSM_RM_PLL_MASK1(modem_lite_topsm_base)) | 0x0000000F,
			MODEM_LITE_TOPSM_RM_PLL_MASK1(modem_lite_topsm_base));

	pr_debug("[ccci-off]7.power off CR4\n");
	sync_write32(0xFFFFFFFF, MD_TOPSM_SM_REQ_MASK(md_topsm_base));
	sync_write32(0x00000000, MD_TOPSM_RM_TMR_PWR0(md_topsm_base));
	sync_write32(0x0005229A, MD_TOPSM_RM_PWR_CON0(md_topsm_base));
	sync_write32(0xFFFFFFFF, MD_TOPSM_RM_PLL_MASK0(md_topsm_base));
	sync_write32(0xFFFFFFFF, MD_TOPSM_RM_PLL_MASK1(md_topsm_base));

	sync_write32(0xFFFFFFFF, MODEM_LITE_TOPSM_SM_REQ_MASK(modem_lite_topsm_base));
	sync_write32(0xFFFFFFFF, MODEM_LITE_TOPSM_RM_PLL_MASK0(modem_lite_topsm_base));
	sync_write32(0xFFFFFFFF, MODEM_LITE_TOPSM_RM_PLL_MASK1(modem_lite_topsm_base));

	sync_write32(0xFFFFFFFF, MODEM_TOPSM_SM_REQ_MASK(modem_topsm_base));
	sync_write32(0xFFFFFFFF, MODEM_TOPSM_RM_PLL_MASK0(modem_topsm_base));
	sync_write32(0xFFFFFFFF, MODEM_TOPSM_RM_PLL_MASK1(modem_topsm_base));

	pr_debug("[ccci-off]8.power off MD_INFRA/MODEM_TOP\n");
#if defined(CONFIG_MTK_CLKMGR)
	ret = md_power_off(SYS_MD1, 0);
#else
	clk_disable_unprepare(clk_scp_sys_md1_main);
#endif

	iounmap(md_topsm_base);
	iounmap(modem_lite_topsm_base);
	iounmap(modem_topsm_base);
	iounmap(tdd_base);
	iounmap(ltelt1_base);
	iounmap(ltelt1_base_1);
	iounmap(ltelt1_base_2);
	iounmap(md_pll_mixedsys_base);
}

static int modem_power_down_worker(void *data)
{
	unsigned int val;

	val = get_devinfo_with_index(4);
	if ((val & (0x1 << 1)) == 0)
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

int ccci_md_off(void)
{
#ifndef CONFIG_MTK_ECCCI_DRIVER
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

#if !defined(CONFIG_MTK_CLKMGR)
static int ccci_off_probe(struct platform_device *pdev)
{
	clk_scp_sys_md1_main = devm_clk_get(&pdev->dev, "scp-sys-md1-main");
	if (IS_ERR(clk_scp_sys_md1_main)) {
		pr_debug("[ccci-off]modem %d get scp-sys-md1-main failed\n", 1);
		return -1;
	}
	pr_debug("[ccci-off][CCF]clk_scp_sys_md1_main=%p\n", clk_scp_sys_md1_main);

	ccci_md_off();

	return 0;
}

static int ccci_off_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ccci_off_of_ids[] = {
	{.compatible = "mediatek,ccci_off",},
	{}
};
#endif
static struct platform_driver ccci_off_dev_drv = {
	.probe = ccci_off_probe,
	.remove = ccci_off_remove,
	.driver = {
		   .name = "ccci_off",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = ccci_off_of_ids,
#endif
		   },
};
#endif /* !defined(CONFIG_MTK_CLKMGR) */

static int __init ccci_off_init(void)
{
	int ret;
#if defined(CONFIG_MTK_CLKMGR)
	pr_debug("ccci_off_init 1\n");
	ret = ccci_md_off();
#else
	ret = platform_driver_register(&ccci_off_dev_drv);
	if (ret)
		pr_debug("[ccci-off] platform driver registered failed(%d)\n", ret);
	else
		pr_debug("[ccci-off]platform driver registered OK\n");
#endif
	return 0;
}
module_init(ccci_off_init);

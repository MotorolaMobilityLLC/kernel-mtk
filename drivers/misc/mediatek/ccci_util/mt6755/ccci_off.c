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
#include <mt-plat/upmu_common.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include "ccci_off.h"
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
#include "include/pmic_api_buck.h"
#endif

#if !defined(CONFIG_MTK_CLKMGR)
static struct clk *clk_scp_sys_md1_main;
#endif

#if !defined(CONFIG_MTK_ECCCI_DRIVER) || defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)

#define sync_write32(v, a)			mt_reg_sync_writel(v, a)
#define sync_write16(v, a)			mt_reg_sync_writew(v, a)
#define sync_write8(v, a)			mt_reg_sync_writeb(v, a)

/*  */
#define MD_P_TOPSM_BASE			(0x200D0000)
#define REG_MD_P_TOPSM_RM_PWR0_CON(base)	((base)+0x0800)
#define REG_MD_P_TOPSM_RM_PWR1_CON(base)	((base)+0x0804)
#define REG_MD_P_TOPSM_RM_PWR2_CON(base)	((base)+0x0808)
#define REG_MD_P_TOPSM_RM_PWR3_CON(base)	((base)+0x080C)
#define REG_MD_P_TOPSM_RM_PWR4_CON(base)	((base)+0x0810)
#define REG_MD_P_TOPSM_RM_TMR_PWR0(base)	((base)+0x0018)
#define REG_MD_P_TOPSM_RM_TMR_PWR1(base)	((base)+0x001C)

#define MD_L1_TOPSM_BASE			(0x26070000)
#define REG_MD_L1_TOPSM_SM_TMR_PWR0(base)	((base)+0x0140)
#define REG_MD_L1_TOPSM_SM_TMR_PWR1(base)	((base)+0x0144)
#define REG_MD_L1_TOPSM_SM_TMR_PWR2(base)	((base)+0x0148)
#define REG_MD_L1_TOPSM_SM_TMR_PWR3(base)	((base)+0x014C)
#define REG_MD_L1_TOPSM_SM_TMR_PWR4(base)	((base)+0x0150)

static void internal_md_power_down(void)
{
	int ret = 0;
	void __iomem *md_p_topsm_base, *md_l1_topsm_base;

	pr_debug("[ccci-off]shutdown MDSYS1 !!!\n");
#if defined(CONFIG_MTK_CLKMGR)
	pr_debug("[ccci-off]Call start md_power_on()\n");
	ret = md_power_on(SYS_MD1);
	pr_debug("[ccci-off]Call end md_power_on() ret=%d\n", ret);
#else
	pr_debug("[ccci-off]Call start clk_prepare_enable()\n");
	clk_prepare_enable(clk_scp_sys_md1_main);
	pr_debug("[ccci-off]Call end clk_prepare_enable()\n");
#endif
	pr_debug("[ccci-off]0.power on MD_INFRA/MODEM_TOP ret=%d\n", ret);
	if (ret)
		return;

	md_p_topsm_base = ioremap_nocache(MD_P_TOPSM_BASE, 0x830);
	md_l1_topsm_base = ioremap_nocache(MD_L1_TOPSM_BASE, 0x200);
	/* 1. Shutting off ARM7, HSPAL2, LTEL2 power domains */
	/* Shutting off ARM7 through software */
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR1_CON(md_p_topsm_base))&(~0xE6045),
		REG_MD_P_TOPSM_RM_PWR1_CON(md_p_topsm_base));
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR1_CON(md_p_topsm_base))|(0xB8),
		REG_MD_P_TOPSM_RM_PWR1_CON(md_p_topsm_base));
	/* Masking control of ostimer on ARM7,HSPAL2,LTEL2 */
	sync_write32(0x01, REG_MD_P_TOPSM_RM_TMR_PWR0(md_p_topsm_base));
	/* De-asserting software power req */
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR0_CON(md_p_topsm_base))&(~0x44),
		REG_MD_P_TOPSM_RM_PWR0_CON(md_p_topsm_base));/* PSMCU */
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR2_CON(md_p_topsm_base))&(~0x44),
		REG_MD_P_TOPSM_RM_PWR2_CON(md_p_topsm_base));/* HSPAL2 */
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR3_CON(md_p_topsm_base))&(~0x44),
		REG_MD_P_TOPSM_RM_PWR3_CON(md_p_topsm_base));/* LTEL2 */
	sync_write32(ioread32(REG_MD_P_TOPSM_RM_PWR4_CON(md_p_topsm_base))&(~0x44),
		REG_MD_P_TOPSM_RM_PWR4_CON(md_p_topsm_base));/* INFRA */

	/* 2. PSMCU and INFRA power domains should be shut off at the end,
	after complete register sequence has been executed: */
	sync_write32(0x00, REG_MD_P_TOPSM_RM_TMR_PWR0(md_p_topsm_base));/* PSMCU into sleep */
	sync_write32(0x00, REG_MD_P_TOPSM_RM_TMR_PWR1(md_p_topsm_base));/* INFRA into sleep */

	/* 3. Shutting off power domains except L1MCU by masking all ostimers control
	on mtcmos power domain: */
	sync_write32(ioread32(REG_MD_L1_TOPSM_SM_TMR_PWR0(md_l1_topsm_base))|~(0x1),
		REG_MD_L1_TOPSM_SM_TMR_PWR0(md_l1_topsm_base));
	sync_write32(0xFFFFFFFF, REG_MD_L1_TOPSM_SM_TMR_PWR1(md_l1_topsm_base));
	sync_write32(0xFFFFFFFF, REG_MD_L1_TOPSM_SM_TMR_PWR2(md_l1_topsm_base));
	sync_write32(0xFFFFFFFF, REG_MD_L1_TOPSM_SM_TMR_PWR3(md_l1_topsm_base));
	sync_write32(0xFFFFFFFF, REG_MD_L1_TOPSM_SM_TMR_PWR4(md_l1_topsm_base));

	/* 4. L1MCU power domain is shut off in the end
	after all register sequence has been executed: */
	sync_write32(0xFFFFFFFF, REG_MD_L1_TOPSM_SM_TMR_PWR0(md_l1_topsm_base));

	pr_notice("[ccci-off]8.power off ARM7, HSPAL2, LTEL2\n");
	/* no need to poll, as MD SW didn't run and enter sleep mode, polling will not get result */
#if defined(CONFIG_MTK_CLKMGR)
	ret = md_power_off(SYS_MD1, 0);
#else
	clk_disable_unprepare(clk_scp_sys_md1_main);
#endif
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	vmd1_pmic_setting_off();
#else
	/* VMODEM off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN, 0); /* 0x063A[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_EN, 0); /* 0x062C[0]=0, 0:Disable, 1:Enable */
	/* VMD1 off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VSLEEP_EN, 0); /* 0x064E[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_EN, 0); /* 0x0640[0]=0, 0:Disable, 1:Enable */
	/* VSRAM_MD off */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN, 0); /* 0x0662[8]=0, 0:SW control, 1:HW control */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_EN, 0); /* 0x0654[0]=0, 0:Disable, 1:Enable */
#endif

	iounmap(md_p_topsm_base);
	iounmap(md_l1_topsm_base);

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

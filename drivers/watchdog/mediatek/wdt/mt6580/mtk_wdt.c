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

#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>

#include <asm/uaccess.h>
#include <linux/types.h>
#include "mt_wdt.h"
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif

#include <linux/trusty/trusty.h>
#include <linux/trusty/smcall.h>
#include <asm/fiq_glue.h>

#include <mt-plat/aee.h>
#include <mt-plat/sync_write.h>
#include <ext_wd_drv.h>

#include <mach/wd_api.h>
#ifdef CONFIG_OF
void __iomem *toprgu_base = 0;
int wdt_irq_id = 0;

static const struct of_device_id rgu_of_match[] = {
	{.compatible = "mediatek,TOPRGU",},
	{},
};
#endif

/**---------------------------------------------------------------------
 * Sub feature switch region
 *----------------------------------------------------------------------
 */
#define NO_DEBUG 1

/*----------------------------------------------------------------------
 *   IRQ ID
 *--------------------------------------------------------------------*/
#ifdef CONFIG_OF
#define AP_RGU_WDT_IRQ_ID    wdt_irq_id
#else
#define AP_RGU_WDT_IRQ_ID    WDT_IRQ_BIT_ID
#endif

/*
 * internal variables
 */
/* static char expect_close; // Not use */
/* static spinlock_t rgu_reg_operation_spinlock = SPIN_LOCK_UNLOCKED; */
static DEFINE_SPINLOCK(rgu_reg_operation_spinlock);
static unsigned int timeout;

static int g_last_time_time_out_value;
static int g_wdt_enable = 1;

#ifndef __USING_DUMMY_WDT_DRV__	/* FPGA will set this flag */

#ifndef CONFIG_FIQ_GLUE
static bool rgu_wdt_intr_has_trigger;	/* For test use */
#endif

/*
    this function set the timeout value.
    value: second
*/
void mtk_wdt_set_time_out_value(unsigned int value)
{
	/*
	 * TimeOut = BitField 15:5
	 * Key     = BitField  4:0 = 0x08
	 */
	spin_lock(&rgu_reg_operation_spinlock);

	/* 1 tick means 512 * T32K -> 1s = T32/512 tick = 64 */
	/* --> value * (1<<6) */
	timeout = (unsigned int)(value * (1 << 6));
	timeout = timeout << 5;
	mt_reg_sync_writel((timeout | MTK_WDT_LENGTH_KEY), MTK_WDT_LENGTH);

	spin_unlock(&rgu_reg_operation_spinlock);
}

/*
    watchdog mode:
    debug_en:   debug module reset enable.
    irq:        generate interrupt instead of reset
    ext_en:     output reset signal to outside
    ext_pol:    polarity of external reset signal
    wdt_en:     enable watch dog timer
*/
void mtk_wdt_mode_config(bool dual_mode_en, bool irq, bool ext_en, bool ext_pol, bool wdt_en)
{
	unsigned int tmp;

	spin_lock(&rgu_reg_operation_spinlock);

	/* pr_debug(" mtk_wdt_mode_config  mode value=%x,pid=%d\n",DRV_Reg32(MTK_WDT_MODE),current->pid); */
	tmp = __raw_readl(MTK_WDT_MODE);
	tmp |= MTK_WDT_MODE_KEY;

	/* Bit 0 : Whether enable watchdog or not */
	if (wdt_en == TRUE)
		tmp |= MTK_WDT_MODE_ENABLE;
	else
		tmp &= ~MTK_WDT_MODE_ENABLE;

	/* Bit 1 : Configure extern reset signal polarity. */
	if (ext_pol == TRUE)
		tmp |= MTK_WDT_MODE_EXT_POL;
	else
		tmp &= ~MTK_WDT_MODE_EXT_POL;

	/* Bit 2 : Whether enable external reset signal */
	if (ext_en == TRUE)
		tmp |= MTK_WDT_MODE_EXTEN;
	else
		tmp &= ~MTK_WDT_MODE_EXTEN;

	/* Bit 3 : Whether generating interrupt instead of reset signal */
	if (irq == TRUE)
		tmp |= MTK_WDT_MODE_IRQ;
	else
		tmp &= ~MTK_WDT_MODE_IRQ;

	/* Bit 6 : Whether enable debug module reset */
	if (dual_mode_en == TRUE)
		tmp |= MTK_WDT_MODE_DUAL_MODE;
	else
		tmp &= ~MTK_WDT_MODE_DUAL_MODE;

	/* Bit 4: WDT_Auto_restart, this is a reserved bit, we use it as bypass powerkey flag. */
	/* Because HW reboot always need reboot to kernel, we set it always. */
	tmp |= MTK_WDT_MODE_AUTO_RESTART;

	mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	/* dual_mode(1); //always dual mode */
	/* mdelay(100); */

	spin_unlock(&rgu_reg_operation_spinlock);
}

/* EXPORT_SYMBOL(mtk_wdt_mode_config); */

int mtk_wdt_enable(enum wk_wdt_en en)
{
	unsigned int tmp = 0;

	spin_lock(&rgu_reg_operation_spinlock);

	tmp = __raw_readl(MTK_WDT_MODE);

	tmp |= MTK_WDT_MODE_KEY;
	if (WK_WDT_EN == en) {
		tmp |= MTK_WDT_MODE_ENABLE;
		g_wdt_enable = 1;
	}
	if (WK_WDT_DIS == en) {
		tmp &= ~MTK_WDT_MODE_ENABLE;
		g_wdt_enable = 0;
	}
	pr_debug("mtk_wdt_enable value=%x,pid=%d\n", tmp, current->pid);
	mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	spin_unlock(&rgu_reg_operation_spinlock);
	return 0;
}

int mtk_wdt_confirm_hwreboot(void)
{
	/* aee need confirm wd can hw reboot */
	/* pr_debug("mtk_wdt_probe : Initialize to dual mode\n"); */
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
	return 0;
}


void mtk_wdt_restart(enum wd_restart_type type)
{
	/* pr_debug("WDT:[mtk_wdt_restart] type  =%d, pid=%d\n",type,current->pid); */

	if (type == WD_TYPE_NORMAL) {
		spin_lock(&rgu_reg_operation_spinlock);
		mt_reg_sync_writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);
		spin_unlock(&rgu_reg_operation_spinlock);
	} else if (type == WD_TYPE_NOLOCK) {
		mt_reg_sync_writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);
	} else
		pr_debug("WDT:[mtk_wdt_restart] type=%d error pid =%d\n", type, current->pid);
}

void wdt_dump_reg(void)
{
	pr_alert("****************dump wdt reg start*************\n");
	pr_alert("MTK_WDT_MODE:0x%x\n", __raw_readl(MTK_WDT_MODE));
	pr_alert("MTK_WDT_LENGTH:0x%x\n", __raw_readl(MTK_WDT_LENGTH));
	pr_alert("MTK_WDT_RESTART:0x%x\n", __raw_readl(MTK_WDT_RESTART));
	pr_alert("MTK_WDT_STATUS:0x%x\n", __raw_readl(MTK_WDT_STATUS));
	pr_alert("MTK_WDT_INTERVAL:0x%x\n", __raw_readl(MTK_WDT_INTERVAL));
	pr_alert("MTK_WDT_SWRST:0x%x\n", __raw_readl(MTK_WDT_SWRST));
	pr_alert("MTK_WDT_NONRST_REG:0x%x\n", __raw_readl(MTK_WDT_NONRST_REG));
	pr_alert("MTK_WDT_NONRST_REG2:0x%x\n", __raw_readl(MTK_WDT_NONRST_REG2));
	pr_alert("MTK_WDT_REQ_MODE:0x%x\n", __raw_readl(MTK_WDT_REQ_MODE));
	pr_alert("MTK_WDT_REQ_IRQ_EN:0x%x\n", __raw_readl(MTK_WDT_REQ_IRQ_EN));
	pr_alert("MTK_WDT_DRAMC_CTL:0x%x\n", __raw_readl(MTK_WDT_DRAMC_CTL));
	pr_alert("****************dump wdt reg end*************\n");

}

void wdt_arch_reset(char mode)
{
	unsigned int wdt_mode_val;
#ifdef CONFIG_OF
	struct device_node *np_rgu;
#endif
	pr_debug("wdt_arch_reset called@Kernel mode =%c\n", mode);
#ifdef CONFIG_OF
	np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);

	if (!toprgu_base) {
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_err("RGU iomap failed\n");
		pr_debug("RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id);
	}
#endif
	spin_lock(&rgu_reg_operation_spinlock);
	/* Watchdog Rest */
	mt_reg_sync_writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);
	wdt_mode_val = __raw_readl(MTK_WDT_MODE);
	pr_debug("wdt_arch_reset called MTK_WDT_MODE =%x\n", wdt_mode_val);
	/* clear autorestart bit: autoretart: 1, bypass power key, 0: not bypass power key */
	wdt_mode_val &= (~MTK_WDT_MODE_AUTO_RESTART);
	/* make sure WDT mode is hw reboot mode, can not config isr mode  */
	wdt_mode_val &= (~(MTK_WDT_MODE_IRQ | MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
	if (mode)
		/* mode != 0 means by pass power key reboot, We using auto_restart bit as by pass power key flag */
		wdt_mode_val =
		    wdt_mode_val | (MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN |
				    MTK_WDT_MODE_AUTO_RESTART);
	else
		wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN);

	mt_reg_sync_writel(wdt_mode_val, MTK_WDT_MODE);
	pr_debug("wdt_arch_reset called end MTK_WDT_MODE =%x\n", wdt_mode_val);
	udelay(100);
	mt_reg_sync_writel(MTK_WDT_SWRST_KEY, MTK_WDT_SWRST);
	pr_debug("wdt_arch_reset: SW_reset happen\n");
	spin_unlock(&rgu_reg_operation_spinlock);

	while (1) {
		wdt_dump_reg();
		pr_err("wdt_arch_reset error\n");
	}

}

int mtk_rgu_dram_reserved(int enable)
{
	unsigned int tmp;

	if (1 == enable) {
		/* enable ddr reserved mode */
		tmp = __raw_readl(MTK_WDT_MODE);
		tmp |= (MTK_WDT_MODE_DDR_RESERVE | MTK_WDT_MODE_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	} else if (0 == enable) {
		/* disable ddr reserved mode, set reset mode,
		   disable watchdog output reset signal */
		tmp = __raw_readl(MTK_WDT_MODE);
		tmp &= (~MTK_WDT_MODE_DDR_RESERVE);
		tmp |= MTK_WDT_MODE_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	}

	pr_debug("mtk_rgu_dram_reserved:MTK_WDT_MODE(0x%x)\n", __raw_readl(MTK_WDT_MODE));
	return 0;
}

int mtk_wdt_swsysret_config(int bit, int set_value)
{
	unsigned int wdt_sys_val;

	spin_lock(&rgu_reg_operation_spinlock);
	wdt_sys_val = __raw_readl(MTK_WDT_SWSYSRST);
	pr_debug("fwq2 before set wdt_sys_val =%x\n", wdt_sys_val);
	wdt_sys_val |= MTK_WDT_SWSYS_RST_KEY;
	switch (bit) {
	case MTK_WDT_SWSYS_RST_MD_RST:
		if (1 == set_value)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_MD_RST;
		if (0 == set_value)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_MD_RST;
		break;
	case MTK_WDT_SWSYS_RST_MD_LITE_RST:
		if (1 == set_value)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_MD_LITE_RST;
		if (0 == set_value)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_MD_LITE_RST;
		break;
	}
	mt_reg_sync_writel(wdt_sys_val, MTK_WDT_SWSYSRST);
	spin_unlock(&rgu_reg_operation_spinlock);

	mdelay(10);
	pr_debug("after set wdt_sys_val =%x,wdt_sys_val=%x\n", __raw_readl(MTK_WDT_SWSYSRST),
		 wdt_sys_val);
	return 0;
}

int mtk_wdt_request_en_set(int mark_bit, WD_REQ_CTL en)
{
	int res = 0;
	unsigned int tmp;

	spin_lock(&rgu_reg_operation_spinlock);
	tmp = __raw_readl(MTK_WDT_REQ_MODE);
	tmp |= MTK_WDT_REQ_MODE_KEY;

	if (MTK_WDT_REQ_MODE_SPM_SCPSYS == mark_bit) {
		if (WD_REQ_EN == en)
			tmp |= (MTK_WDT_REQ_MODE_SPM_SCPSYS);
		if (WD_REQ_DIS == en)
			tmp &= ~(MTK_WDT_REQ_MODE_SPM_SCPSYS);
	} else if (MTK_WDT_REQ_MODE_SPM_THERMAL == mark_bit) {
		if (WD_REQ_EN == en)
			tmp |= (MTK_WDT_REQ_MODE_SPM_THERMAL);
		if (WD_REQ_DIS == en)
			tmp &= ~(MTK_WDT_REQ_MODE_SPM_THERMAL);
	} else if (MTK_WDT_REQ_MODE_THERMAL == mark_bit) {
		if (WD_REQ_EN == en)
			tmp |= (MTK_WDT_REQ_MODE_THERMAL);
		if (WD_REQ_DIS == en)
			tmp &= ~(MTK_WDT_REQ_MODE_THERMAL);
	} else
		res = -1;

	mt_reg_sync_writel(tmp, MTK_WDT_REQ_MODE);
	spin_unlock(&rgu_reg_operation_spinlock);
	return res;
}

int mtk_wdt_request_mode_set(int mark_bit, WD_REQ_MODE mode)
{
	int res = 0;
	unsigned int tmp;

	spin_lock(&rgu_reg_operation_spinlock);
	tmp = __raw_readl(MTK_WDT_REQ_IRQ_EN);
	tmp |= MTK_WDT_REQ_IRQ_KEY;

	if (MTK_WDT_REQ_MODE_SPM_SCPSYS == mark_bit) {
		if (WD_REQ_IRQ_MODE == mode)
			tmp |= (MTK_WDT_REQ_IRQ_SPM_SCPSYS_EN);
		if (WD_REQ_RST_MODE == mode)
			tmp &= ~(MTK_WDT_REQ_IRQ_SPM_SCPSYS_EN);
	} else if (MTK_WDT_REQ_MODE_SPM_THERMAL == mark_bit) {
		if (WD_REQ_IRQ_MODE == mode)
			tmp |= (MTK_WDT_REQ_IRQ_SPM_THERMAL_EN);
		if (WD_REQ_RST_MODE == mode)
			tmp &= ~(MTK_WDT_REQ_IRQ_SPM_THERMAL_EN);
	} else if (MTK_WDT_REQ_MODE_THERMAL == mark_bit) {
		if (WD_REQ_IRQ_MODE == mode)
			tmp |= (MTK_WDT_REQ_IRQ_THERMAL_EN);
		if (WD_REQ_RST_MODE == mode)
			tmp &= ~(MTK_WDT_REQ_IRQ_THERMAL_EN);
	} else
		res = -1;
	mt_reg_sync_writel(tmp, MTK_WDT_REQ_IRQ_EN);
	spin_unlock(&rgu_reg_operation_spinlock);
	return res;
}

void mtk_wdt_cpu_callback(struct task_struct *wk_tsk, unsigned long action, int hotcpu, int kicker_init)
{
}
EXPORT_SYMBOL(mtk_wdt_cpu_callback);

#else
/* ------------------------------------------------------------------------------------------------- */
/* Dummy functions */
/* ------------------------------------------------------------------------------------------------- */
void mtk_wdt_set_time_out_value(unsigned int value)
{
}

static void mtk_wdt_set_reset_length(unsigned int value)
{
}

void mtk_wdt_mode_config(bool dual_mode_en, bool irq, bool ext_en, bool ext_pol, bool wdt_en)
{
}

int mtk_wdt_enable(enum wk_wdt_en en)
{
	return 0;
}

void mtk_wdt_restart(enum wd_restart_type type)
{
}

static void mtk_wdt_sw_trigger(void)
{
}

static unsigned char mtk_wdt_check_status(void)
{
	return 0;
}

void wdt_arch_reset(char mode)
{
}

int mtk_wdt_confirm_hwreboot(void)
{
	return 0;
}

void mtk_wd_suspend(void)
{
}

void mtk_wd_resume(void)
{
}

void wdt_dump_reg(void)
{
}

int mtk_wdt_swsysret_config(int bit, int set_value)
{
	return 0;
}

int mtk_wdt_request_mode_set(int mark_bit, WD_REQ_MODE mode)
{
	return 0;
}

int mtk_wdt_request_en_set(int mark_bit, WD_REQ_CTL en)
{
	return 0;
}

int mtk_rgu_dram_reserved(int enable)
{
	return 0;
}

#endif				/* #ifndef __USING_DUMMY_WDT_DRV__ */

#ifndef CONFIG_FIQ_GLUE
static void wdt_report_info(void)
{
	/* extern struct task_struct *wk_tsk; */
	struct task_struct *task;

	task = &init_task;
	pr_debug("Qwdt: -- watchdog time out\n");

	for_each_process(task) {
		if (task->state == 0) {
			pr_debug("PID: %d, name: %s\n backtrace:\n", task->pid, task->comm);
			show_stack(task, NULL);
			pr_debug("\n");
		}
	}

	pr_debug("backtrace of current task:\n");
	show_stack(NULL, NULL);
	pr_debug("Qwdt: -- watchdog time out\n");
}
#endif


#ifdef CONFIG_FIQ_GLUE
static void wdt_fiq(void *arg, void *regs, void *svc_sp)
{
	unsigned int wdt_mode_val;
	struct wd_api *wd_api = NULL;

	if (get_wd_api(&wd_api) != 0)
		aee_wdt_printf("\n get_wd_api failed!");

	wdt_mode_val = __raw_readl(MTK_WDT_STATUS);
	mt_reg_sync_writel(wdt_mode_val, MTK_WDT_NONRST_REG);
#ifdef	CONFIG_MTK_WD_KICKER
	aee_wdt_printf("\n kick=0x%08x,check=0x%08x,STA=%x\n", wd_api->wd_get_kick_bit(),
		       wd_api->wd_get_check_bit(), wdt_mode_val);
#endif

#ifdef	CONFIG_MTK_AEE_FEATURE
	aee_wdt_fiq_info(arg, regs, svc_sp);
#endif

#if 0
	asm volatile("mov %0, %1\n\t"
		  "mov fp, %2\n\t"
		 : "=r" (sp)
		 : "r" (svc_sp), "r" (preg[11])
		 );
	*((unsigned int *)(0x00000000)); /* trigger exception */
#endif
}
#else				/* CONFIG_FIQ_GLUE */
static irqreturn_t mtk_wdt_isr(int irq, void *dev_id)
{
	pr_err("fwq mtk_wdt_isr\n");
#ifndef __USING_DUMMY_WDT_DRV__	/* FPGA will set this flag */
	/* mt65xx_irq_mask(AP_RGU_WDT_IRQ_ID); */
	rgu_wdt_intr_has_trigger = 1;
	wdt_report_info();
	BUG();

#endif
	return IRQ_HANDLED;
}
#endif				/* CONFIG_FIQ_GLUE */

/*
 * Device interface
 */
static int mtk_wdt_probe(struct platform_device *dev)
{
	int ret = 0;
	unsigned int interval_val;

	pr_err("******** MTK WDT driver probe!! ********\n");
#ifdef CONFIG_OF
	if (!toprgu_base) {
		toprgu_base = of_iomap(dev->dev.of_node, 0);
		if (!toprgu_base) {
			pr_err("RGU iomap failed\n");
			return -ENODEV;
		}
	}
	if (!wdt_irq_id) {
		wdt_irq_id = irq_of_parse_and_map(dev->dev.of_node, 0);
		if (!wdt_irq_id) {
			pr_err("RGU get IRQ ID failed\n");
			return -ENODEV;
		}
	}
	pr_debug("RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id);

#endif
#ifndef __USING_DUMMY_WDT_DRV__	/* FPGA will set this flag */

#ifndef CONFIG_FIQ_GLUE
	pr_debug("******** MTK WDT register irq ********\n");
	ret =
	    request_irq(AP_RGU_WDT_IRQ_ID, (irq_handler_t) mtk_wdt_isr, IRQF_TRIGGER_FALLING,
			"mt_wdt", NULL);
#else
#ifdef CONFIG_TRUSTY_WDT_FIQ_ARMV7_SUPPORT
	{

	pr_debug("******** MTK WDT register trusty fiq ********\n");
	ret = request_trusty_fiq(NULL, AP_RGU_WDT_IRQ_ID, wdt_fiq, IRQF_TRIGGER_FALLING, NULL);
	if (ret) {
		pr_err("mtk_wdt_probe: smc for request trusty fiq failed\n");
		BUG_ON(1);
	}
	}
#else
	pr_debug("******** MTK WDT register fiq ********\n");
	ret = request_fiq(AP_RGU_WDT_IRQ_ID, wdt_fiq, IRQF_TRIGGER_FALLING, NULL);
#endif
#endif

	if (ret != 0) {
		pr_err("mtk_wdt_probe : failed to request irq (%d)\n", ret);
		return ret;
	}
	pr_debug("mtk_wdt_probe : Success to request irq\n");


	/* Set timeout vale and restart counter */
	g_last_time_time_out_value = 30;
	mtk_wdt_set_time_out_value(g_last_time_time_out_value);

	mtk_wdt_restart(WD_TYPE_NORMAL);

	/**
	 * Set the reset length: we will set a special magic key.
	 * For Power off and power on reset, the INTERVAL default value is 0x7FF.
	 * We set Interval[1:0] to different value to distinguish different stage.
	 * Enter pre-loader, we will set it to 0x0
	 * Enter u-boot, we will set it to 0x1
	 * Enter kernel, we will set it to 0x2
	 * And the default value is 0x3 which means reset from a power off and power on reset
	 */
#define POWER_OFF_ON_MAGIC	(0x3)
#define PRE_LOADER_MAGIC	(0x0)
#define U_BOOT_MAGIC		(0x1)
#define KERNEL_MAGIC		(0x2)
#define MAGIC_NUM_MASK		(0x3)


#ifdef CONFIG_MTK_WD_KICKER	/* Initialize to dual mode */
	pr_debug("mtk_wdt_probe : Initialize to dual mode\n");
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
#else				/* Initialize to disable wdt */
	pr_debug("mtk_wdt_probe : Initialize to disable wdt\n");
	mtk_wdt_mode_config(FALSE, FALSE, TRUE, FALSE, FALSE);
	g_wdt_enable = 0;
#endif


	/* Update interval register value and check reboot flag */
	interval_val = __raw_readl(MTK_WDT_INTERVAL);
	interval_val &= ~(MAGIC_NUM_MASK);
	interval_val |= (KERNEL_MAGIC);
	/* Write back INTERVAL REG */
	mt_reg_sync_writel(interval_val, MTK_WDT_INTERVAL);
#endif
	udelay(100);
	pr_debug("mtk_wdt_probe : done WDT_MODE(%x),MTK_WDT_NONRST_REG(%x)\n",
		 __raw_readl(MTK_WDT_MODE), __raw_readl(MTK_WDT_NONRST_REG));
	pr_debug("mtk_wdt_probe : done MTK_WDT_REQ_MODE(%x)\n", __raw_readl(MTK_WDT_REQ_MODE));
	pr_debug("mtk_wdt_probe : done MTK_WDT_REQ_IRQ_EN(%x)\n", __raw_readl(MTK_WDT_REQ_IRQ_EN));

	return ret;
}

static int mtk_wdt_remove(struct platform_device *dev)
{
	pr_debug("******** MTK wdt driver remove!! ********\n");

#ifndef __USING_DUMMY_WDT_DRV__	/* FPGA will set this flag */
	free_irq(AP_RGU_WDT_IRQ_ID, NULL);
#endif
	return 0;
}

static void mtk_wdt_shutdown(struct platform_device *dev)
{
	pr_debug("******** MTK WDT driver shutdown!! ********\n");

	/* mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE); */
	/* kick external wdt */
	/* mtk_wdt_mode_config(TRUE, FALSE, FALSE, FALSE, FALSE); */

	mtk_wdt_restart(WD_TYPE_NORMAL);
	pr_debug("******** MTK WDT driver shutdown done ********\n");
}

void mtk_wd_suspend(void)
{
	/* mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE); */
	/* en debug, dis irq, dis ext, low pol, dis wdt */
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, FALSE);

	mtk_wdt_restart(WD_TYPE_NORMAL);

	/*aee_sram_printk("[WDT] suspend\n"); */
	/*pr_debug("[WDT] suspend\n"); */
}

void mtk_wd_resume(void)
{

	if (g_wdt_enable == 1) {
		mtk_wdt_set_time_out_value(g_last_time_time_out_value);
		mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
		mtk_wdt_restart(WD_TYPE_NORMAL);
	}

	/*aee_sram_printk("[WDT] resume(%d)\n", g_wdt_enable); */
	/*pr_debug("[WDT] resume(%d)\n", g_wdt_enable); */
}



static struct platform_driver mtk_wdt_driver = {
	.driver = {
		   .name = "mtk-wdt",
#ifdef CONFIG_OF
		   .of_match_table = rgu_of_match,
#endif
		   },
	.probe = mtk_wdt_probe,
	.remove = mtk_wdt_remove,
	.shutdown = mtk_wdt_shutdown,
/* .suspend        = mtk_wdt_suspend, */
/* .resume = mtk_wdt_resume, */
};

#ifndef CONFIG_OF
struct platform_device mtk_device_wdt = {
	.name = "mtk-wdt",
	.id = 0,
	.dev = {
		}
};
#endif

/*
 * init and exit function
 */
static int __init mtk_wdt_init(void)
{

	int ret;

#ifndef CONFIG_OF
	ret = platform_device_register(&mtk_device_wdt);
	if (ret) {
		pr_err("****[mtk_wdt_driver] Unable to device register(%d)\n", ret);
		return ret;
	}
#endif
	ret = platform_driver_register(&mtk_wdt_driver);
	if (ret) {
		pr_err("****[mtk_wdt_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}
	pr_alert("mtk_wdt_init ok\n");
	return 0;
}

static void __exit mtk_wdt_exit(void)
{
}
arch_initcall(mtk_wdt_init);
/* module_exit(mtk_wdt_exit); */

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MT6582 Watchdog Device Driver");
MODULE_LICENSE("GPL");

/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/init.h>        /* For init/exit macros */
#include <linux/module.h>      /* For MODULE_ marcros  */
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>

#include <linux/uaccess.h>
#include <linux/types.h>
#include <mtk_wdt.h>
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
#include <mt-plat/sync_write.h>
#include <ext_wd_drv.h>

#include <mach/wd_api.h>
#include <mt-plat/upmu_common.h>
#include <linux/irqchip/mtk-eic.h>

#ifdef CONFIG_OF
void __iomem *toprgu_base;
int	wdt_irq_id;
int ext_debugkey_io = -1;
int ext_debugkey_io_eint = -1;

static const struct of_device_id rgu_of_match[] = {
	{ .compatible = "mediatek,mt6763-toprgu", },
	{},
};
#endif

/**---------------------------------------------------------------------
 * Sub feature switch region
 *----------------------------------------------------------------------
 */
#define NO_DEBUG 1

/*
 *----------------------------------------------------------------------
 *   IRQ ID
 *--------------------------------------------------------------------
 */
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
#ifndef CONFIG_KICK_SPM_WDT
static unsigned int timeout;
#endif
static volatile bool  rgu_wdt_intr_has_trigger; /* For test use */
static int g_last_time_time_out_value;
static int g_wdt_enable = 1;
#ifdef CONFIG_KICK_SPM_WDT
#include <mach/mt_spm.h>
static void spm_wdt_init(void);

#endif

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
/*
 *   this function set the timeout value.
 *   value: second
*/
void mtk_wdt_set_time_out_value(unsigned int value)
{
	/*
	 * TimeOut = BitField 15:5
	 * Key	   = BitField  4:0 = 0x08
	 */
	spin_lock(&rgu_reg_operation_spinlock);

    #ifdef CONFIG_KICK_SPM_WDT
	spm_wdt_set_timeout(value);
    #else

	/* 1 tick means 512 * T32K -> 1s = T32/512 tick = 64 */
	/* --> value * (1<<6) */
	timeout = (unsigned int)(value * (1 << 6));
	timeout = timeout << 5;
	mt_reg_sync_writel((timeout | MTK_WDT_LENGTH_KEY), MTK_WDT_LENGTH);
    #endif
	spin_unlock(&rgu_reg_operation_spinlock);
}
/*
 *   watchdog mode:
 *   debug_en:   debug module reset enable.
 *   irq:        generate interrupt instead of reset
 *   ext_en:     output reset signal to outside
 *   ext_pol:    polarity of external reset signal
 *   wdt_en:     enable watch dog timer
*/
void mtk_wdt_mode_config(bool dual_mode_en,
					bool irq,
					bool ext_en,
					bool ext_pol,
					bool wdt_en)
{
	#ifndef CONFIG_KICK_SPM_WDT
	unsigned int tmp;
	#endif
	spin_lock(&rgu_reg_operation_spinlock);
	#ifdef CONFIG_KICK_SPM_WDT
	if (wdt_en == TRUE) {
		pr_debug("wdt enable spm timer.....\n");
		spm_wdt_enable_timer();
	} else {
		pr_debug("wdt disable spm timer.....\n");
		spm_wdt_disable_timer();
	}
    #else
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
	pr_debug(" mtk_wdt_mode_config  mode value=%x, tmp:%x,pid=%d\n", __raw_readl(MTK_WDT_MODE), tmp, current->pid);
    #endif
	spin_unlock(&rgu_reg_operation_spinlock);
}
/* EXPORT_SYMBOL(mtk_wdt_mode_config); */

int mtk_wdt_enable(enum wk_wdt_en en)
{
	unsigned int tmp = 0;

	spin_lock(&rgu_reg_operation_spinlock);
    #ifdef CONFIG_KICK_SPM_WDT
	if (en == WK_WDT_EN) {
		spm_wdt_enable_timer();
		pr_debug("wdt enable spm timer\n");

		tmp = __raw_readl(MTK_WDT_REQ_MODE);
		tmp |=  MTK_WDT_REQ_MODE_KEY;
		tmp |= (MTK_WDT_REQ_MODE_SPM_SCPSYS);
		mt_reg_sync_writel(tmp, MTK_WDT_REQ_MODE);
		g_wdt_enable = 1;
	}
	if (en == WK_WDT_DIS) {
		spm_wdt_disable_timer();
		pr_debug("wdt disable spm timer\n ");
		tmp = __raw_readl(MTK_WDT_REQ_MODE);
		tmp |=  MTK_WDT_REQ_MODE_KEY;
		tmp &= ~(MTK_WDT_REQ_MODE_SPM_SCPSYS);
		mt_reg_sync_writel(tmp, MTK_WDT_REQ_MODE);
		g_wdt_enable = 0;
	}
    #else

	tmp = __raw_readl(MTK_WDT_MODE);

	tmp |= MTK_WDT_MODE_KEY;
	if (en == WK_WDT_EN) {
		tmp |= MTK_WDT_MODE_ENABLE;
		g_wdt_enable = 1;
	}
	if (en == WK_WDT_DIS) {
		tmp &= ~MTK_WDT_MODE_ENABLE;
		g_wdt_enable = 0;
	}
	pr_debug("mtk_wdt_enable value=%x,pid=%d\n", tmp, current->pid);
	mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	#endif
	spin_unlock(&rgu_reg_operation_spinlock);
	return 0;
}
int  mtk_wdt_confirm_hwreboot(void)
{
    /* aee need confirm wd can hw reboot */
    /* pr_debug("mtk_wdt_probe : Initialize to dual mode\n"); */
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
	return 0;
}


void mtk_wdt_restart(enum wd_restart_type type)
{

#ifdef CONFIG_OF
	struct device_node *np_rgu;

	np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);

	if (!toprgu_base) {
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_debug("RGU iomap failed\n");
		/* pr_debug("RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id); */
	}
#endif

    /* pr_debug("WDT:[mtk_wdt_restart] type  =%d, pid=%d\n",type,current->pid); */

	if (type == WD_TYPE_NORMAL) {
		/* printk("WDT:ext restart\n" ); */
		spin_lock(&rgu_reg_operation_spinlock);
	#ifdef CONFIG_KICK_SPM_WDT
		spm_wdt_restart_timer();
	#else
		mt_reg_sync_writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);
	#endif
		spin_unlock(&rgu_reg_operation_spinlock);
	} else if (type == WD_TYPE_NOLOCK) {
	#ifdef CONFIG_KICK_SPM_WDT
		spm_wdt_restart_timer_nolock();
	#else
		*(volatile u32 *)(MTK_WDT_RESTART) = MTK_WDT_RESTART_KEY;
	#endif
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
	pr_alert("MTK_WDT_EXT_REQ_CON:0x%x\n", __raw_readl(MTK_WDT_EXT_REQ_CON));
	pr_alert("MTK_WDT_DEBUG_CTL:0x%x\n", __raw_readl(MTK_WDT_DEBUG_CTL));
	pr_alert("MTK_WDT_LATCH_CTL:0x%x\n", __raw_readl(MTK_WDT_LATCH_CTL));
	pr_alert("MTK_WDT_DEBUG_CTL2:0x%x\n", __raw_readl(MTK_WDT_DEBUG_CTL2));
	pr_alert("****************dump wdt reg end*************\n");

}

void aee_wdt_dump_reg(void)
{
/*
 *	aee_wdt_printf("***dump wdt reg start***\n");
 *	aee_wdt_printf("MODE:0x%x\n", __raw_readl(MTK_WDT_MODE));
 *	aee_wdt_printf("LENGTH:0x%x\n", __raw_readl(MTK_WDT_LENGTH));
 *	aee_wdt_printf("RESTART:0x%x\n", __raw_readl(MTK_WDT_RESTART));
 *	aee_wdt_printf("STATUS:0x%x\n", __raw_readl(MTK_WDT_STATUS));
 *	aee_wdt_printf("INTERVAL:0x%x\n", __raw_readl(MTK_WDT_INTERVAL));
 *	aee_wdt_printf("SWRST:0x%x\n", __raw_readl(MTK_WDT_SWRST));
 *	aee_wdt_printf("NONRST_REG:0x%x\n", __raw_readl(MTK_WDT_NONRST_REG));
 *	aee_wdt_printf("NONRST_REG2:0x%x\n", __raw_readl(MTK_WDT_NONRST_REG2));
 *	aee_wdt_printf("REQ_MODE:0x%x\n", __raw_readl(MTK_WDT_REQ_MODE));
 *	aee_wdt_printf("REQ_IRQ_EN:0x%x\n", __raw_readl(MTK_WDT_REQ_IRQ_EN));
 *	aee_wdt_printf("DRAMC_CTL:0x%x\n", __raw_readl(MTK_WDT_DEBUG_2_REG));
 *	aee_wdt_printf("LATCH_CTL:0x%x\n", __raw_readl(MTK_WDT_LATCH_CTL));
 *	aee_wdt_printf("***dump wdt reg end***\n");
*/
}

void wdt_arch_reset(char mode)
{
	unsigned int wdt_mode_val;
#ifdef CONFIG_OF
	struct device_node *np_rgu;
#endif
	pr_debug("wdt_arch_reset called@Kernel mode =%d\n", mode);
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

#ifndef CONFIG_FPGA_EARLY_PORTING
#ifdef CONFIG_MTK_PMIC
	/*
	 * Dump all PMIC registers value AND clear all PMIC
	 * exception registers before SW trigger WDT for easier
	 * issue debugging.
	 *
	 * This is added since X20 but shall be common in all future platforms.
	 *
	 * This shall be executed when WDT mechanism is enabled since
	 * it may hang inside, for example, IPI hang due to SSPM issue.
	 */
	pmic_pre_wdt_reset();
#endif
#endif

	wdt_mode_val = __raw_readl(MTK_WDT_MODE);

	pr_debug("wdt_arch_reset called MTK_WDT_MODE =%x\n", wdt_mode_val);

	/* clear autorestart bit: autoretart: 1, bypass power key, 0: not bypass power key */
	wdt_mode_val &= (~MTK_WDT_MODE_AUTO_RESTART);

	/* make sure WDT mode is hw reboot mode, can not config isr mode  */
	wdt_mode_val &= (~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));

	if (mode)
		/* mode != 0 means by pass power key reboot, We using auto_restart bit as by pass power key flag */
		wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
	else
		wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN);

	/*set latch register to 0 for SW reset*/
	/* mt_reg_sync_writel((MTK_WDT_LENGTH_CTL_KEY | 0x0), MTK_WDT_LATCH_CTL); */

	mt_reg_sync_writel(wdt_mode_val, MTK_WDT_MODE);

	pr_debug("wdt_arch_reset called end MTK_WDT_MODE =%x\n", wdt_mode_val);

	udelay(100);

	pr_debug("wdt_arch_reset: SW_reset happen\n");

	__inner_flush_dcache_all();

	mt_reg_sync_writel(MTK_WDT_SWRST_KEY, MTK_WDT_SWRST);

	spin_unlock(&rgu_reg_operation_spinlock);

	while (1) {
		/* wdt_dump_reg(); */
		/* pr_err("wdt_arch_reset dump\n"); */
		cpu_relax();
	}

}

int mtk_rgu_dram_reserved(int enable)
{
	volatile unsigned int tmp;

	if (enable == 1) {
		/* enable ddr reserved mode */
		tmp = __raw_readl(MTK_WDT_MODE);
		tmp |= (MTK_WDT_MODE_DDR_RESERVE | MTK_WDT_MODE_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	} else if (enable == 0) {
		/* disable ddr reserved mode, set reset mode, */
		/* disable watchdog output reset signal */
		tmp = __raw_readl(MTK_WDT_MODE);
		tmp &= (~MTK_WDT_MODE_DDR_RESERVE);
		tmp |= MTK_WDT_MODE_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_MODE);
	}
	pr_debug("mtk_rgu_dram_reserved:MTK_WDT_MODE(0x%x)\n", __raw_readl(MTK_WDT_MODE));

	return 0;
}

int mtk_rgu_cfg_emi_dcs(int enable)
{
	volatile unsigned int tmp;

	tmp = __raw_readl(MTK_WDT_DEBUG_CTL);

	if (enable == 1) {
		/* enable emi dcs */
		tmp |= MTK_WDT_DEBUG_CTL_EMI_DCS_EN;
	} else if (enable == 0) {
		/* disable emi dcs */
		tmp &= (~MTK_WDT_DEBUG_CTL_EMI_DCS_EN);
	} else
		return -1;

	tmp |= MTK_WDT_DEBUG_CTL_KEY;
	mt_reg_sync_writel(tmp, MTK_WDT_DEBUG_CTL);

	pr_debug("%s: MTK_WDT_DEBUG_CTL(0x%x)\n", __func__, __raw_readl(MTK_WDT_DEBUG_CTL));

	return 0;
}

int mtk_rgu_cfg_dvfsrc(int enable)
{
	volatile unsigned int tmp;

	tmp = __raw_readl(MTK_WDT_DEBUG_CTL);

	if (enable == 1) {
		/* enable dvfsrc_en */
		tmp |= MTK_WDT_DEBUG_CTL_DVFSRC_EN;
	} else if (enable == 0) {
		/* disable dvfsrc_en */
		tmp &= (~MTK_WDT_DEBUG_CTL_DVFSRC_EN);
	} else
		return -1;

	tmp |= MTK_WDT_DEBUG_CTL_KEY;
	mt_reg_sync_writel(tmp, MTK_WDT_DEBUG_CTL);

	pr_debug("%s: MTK_WDT_DEBUG_CTL(0x%x)\n", __func__, __raw_readl(MTK_WDT_DEBUG_CTL));

	return 0;
}


int mtk_rgu_mcu_cache_preserve(int enable)
{
	volatile unsigned int tmp;

	if (enable == 1) {
		/* enable cache retention */
		tmp = __raw_readl(MTK_WDT_DEBUG_CTL);
		tmp |= (MTK_RG_MCU_CACHE_PRESERVE | MTK_WDT_DEBUG_CTL_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_DEBUG_CTL);
	} else if (enable == 0) {
		/* disable cache retention */
		tmp = __raw_readl(MTK_WDT_DEBUG_CTL);
		tmp &= (~MTK_RG_MCU_CACHE_PRESERVE);
		tmp |= MTK_WDT_DEBUG_CTL_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_DEBUG_CTL);
	}
	pr_debug("mtk_rgu_mcu_cache_preserve:MTK_WDT_DEBUG_CTL(0x%x)\n", __raw_readl(MTK_WDT_DEBUG_CTL));

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
		if (set_value == 1)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_MD_RST;
		if (set_value == 0)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_MD_RST;
		break;
	case MTK_WDT_SWSYS_RST_MFG_RST: /* MFG reset */
		if (set_value == 1)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_MFG_RST;
		if (set_value == 0)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_MFG_RST;
		break;
	case MTK_WDT_SWSYS_RST_C2K_RST: /* c2k reset */
		if (set_value == 1)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_C2K_RST;
		if (set_value == 0)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_C2K_RST;
		break;
	case MTK_WDT_SWSYS_RST_CONMCU_RST:
		if (set_value == 1)
			wdt_sys_val |= MTK_WDT_SWSYS_RST_CONMCU_RST;
		if (set_value == 0)
			wdt_sys_val &= ~MTK_WDT_SWSYS_RST_CONMCU_RST;
		break;
	}
	mt_reg_sync_writel(wdt_sys_val, MTK_WDT_SWSYSRST);
	spin_unlock(&rgu_reg_operation_spinlock);

	/* mdelay(10); */
	pr_debug("after set wdt_sys_val =%x,wdt_sys_val=%x\n", __raw_readl(MTK_WDT_SWSYSRST), wdt_sys_val);
	return 0;
}

int mtk_wdt_request_en_set(int mark_bit, WD_REQ_CTL en)
{
	int res = 0;
	unsigned int tmp, ext_req_con;
	struct device_node *np_rgu;

	if (!toprgu_base) {
		np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_err("RGU iomap failed\n");
		pr_debug("RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id);
	}

	spin_lock(&rgu_reg_operation_spinlock);
	tmp = __raw_readl(MTK_WDT_REQ_MODE);
	tmp |=  MTK_WDT_REQ_MODE_KEY;

	if (mark_bit == MTK_WDT_REQ_MODE_SPM_SCPSYS) {
		if (en == WD_REQ_EN)
			tmp |= (MTK_WDT_REQ_MODE_SPM_SCPSYS);
		if (en == WD_REQ_DIS)
			tmp &=  ~(MTK_WDT_REQ_MODE_SPM_SCPSYS);
	} else if (mark_bit == MTK_WDT_REQ_MODE_SPM_THERMAL) {
		if (en == WD_REQ_EN)
			tmp |= (MTK_WDT_REQ_MODE_SPM_THERMAL);
		if (en == WD_REQ_DIS)
			tmp &=  ~(MTK_WDT_REQ_MODE_SPM_THERMAL);
	} else if (mark_bit == MTK_WDT_REQ_MODE_EINT) {
		if (en == WD_REQ_EN) {
			if (ext_debugkey_io != -1) {
				ext_debugkey_io_eint = mt_gpio_to_eint(ext_debugkey_io);
				pr_err("RGU ext_debugkey_io_eint is %d\n", ext_debugkey_io_eint);
				ext_req_con = (ext_debugkey_io_eint << 4) | 0x01;
				mt_reg_sync_writel(ext_req_con, MTK_WDT_EXT_REQ_CON);
				tmp |= (MTK_WDT_REQ_MODE_EINT);
			} else {
				tmp &= ~(MTK_WDT_REQ_MODE_EINT);
				res = -1;
			}
		}
		if (en == WD_REQ_DIS)
			tmp &= ~(MTK_WDT_REQ_MODE_EINT);
	} else if (mark_bit == MTK_WDT_REQ_MODE_SYSRST) {
		if (en == WD_REQ_EN) {
			mt_reg_sync_writel(MTK_WDT_SYSDBG_DEG_EN1_KEY, MTK_WDT_SYSDBG_DEG_EN1);
			mt_reg_sync_writel(MTK_WDT_SYSDBG_DEG_EN2_KEY, MTK_WDT_SYSDBG_DEG_EN2);
			tmp |= (MTK_WDT_REQ_MODE_SYSRST);
		}
		if (en == WD_REQ_DIS)
			tmp &= ~(MTK_WDT_REQ_MODE_SYSRST);
	} else if (mark_bit == MTK_WDT_REQ_MODE_THERMAL) {
		if (en == WD_REQ_EN)
			tmp |= (MTK_WDT_REQ_MODE_THERMAL);
		if (en == WD_REQ_DIS)
			tmp &=  ~(MTK_WDT_REQ_MODE_THERMAL);
	} else
		res =  -1;

	mt_reg_sync_writel(tmp, MTK_WDT_REQ_MODE);
	spin_unlock(&rgu_reg_operation_spinlock);
	return res;
}

int mtk_wdt_request_mode_set(int mark_bit, WD_REQ_MODE mode)
{
	int res = 0;
	unsigned int tmp;
	struct device_node *np_rgu;

	if (!toprgu_base) {
		np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_err("RGU iomap failed\n");
		pr_debug("RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id);
	}

	spin_lock(&rgu_reg_operation_spinlock);
	tmp = __raw_readl(MTK_WDT_REQ_IRQ_EN);
	tmp |= MTK_WDT_REQ_IRQ_KEY;

	if (mark_bit == MTK_WDT_REQ_MODE_SPM_SCPSYS) {
		if (mode == WD_REQ_IRQ_MODE)
			tmp |= (MTK_WDT_REQ_IRQ_SPM_SCPSYS_EN);
		if (mode == WD_REQ_RST_MODE)
			tmp &=  ~(MTK_WDT_REQ_IRQ_SPM_SCPSYS_EN);
	} else if (mark_bit == MTK_WDT_REQ_MODE_SPM_THERMAL) {
		if (mode == WD_REQ_IRQ_MODE)
			tmp |= (MTK_WDT_REQ_IRQ_SPM_THERMAL_EN);
		if (mode == WD_REQ_RST_MODE)
			tmp &=  ~(MTK_WDT_REQ_IRQ_SPM_THERMAL_EN);
	} else if (mark_bit == MTK_WDT_REQ_MODE_EINT) {
		if (mode == WD_REQ_IRQ_MODE)
			tmp |= (MTK_WDT_REQ_IRQ_EINT_EN);
		if (mode == WD_REQ_RST_MODE)
			tmp &= ~(MTK_WDT_REQ_IRQ_EINT_EN);
	} else if (mark_bit == MTK_WDT_REQ_MODE_SYSRST) {
		if (mode == WD_REQ_IRQ_MODE)
			tmp |= (MTK_WDT_REQ_IRQ_SYSRST_EN);
		if (mode == WD_REQ_RST_MODE)
			tmp &= ~(MTK_WDT_REQ_IRQ_SYSRST_EN);
	} else if (mark_bit == MTK_WDT_REQ_MODE_THERMAL) {
		if (mode == WD_REQ_IRQ_MODE)
			tmp |= (MTK_WDT_REQ_IRQ_THERMAL_EN);
		if (mode == WD_REQ_RST_MODE)
			tmp &=  ~(MTK_WDT_REQ_IRQ_THERMAL_EN);
	} else
		res =  -1;
	mt_reg_sync_writel(tmp, MTK_WDT_REQ_IRQ_EN);
	spin_unlock(&rgu_reg_operation_spinlock);
	return res;
}

/*this API is for C2K only
* flag: 1 is to clear;0 is to set
* shift: which bit need to do set or clear
*/
void mtk_wdt_set_c2k_sysrst(unsigned int flag, unsigned int shift)
{
#ifdef CONFIG_OF
	struct device_node *np_rgu;
#endif
	unsigned int ret;
#ifdef CONFIG_OF
	np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);

	if (!toprgu_base) {
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_err("mtk_wdt_set_c2k_sysrst RGU iomap failed\n");
		pr_debug("mtk_wdt_set_c2k_sysrst RGU base: 0x%p  RGU irq: %d\n", toprgu_base, wdt_irq_id);
	}
#endif
	if (flag == 1) {
		ret = __raw_readl(MTK_WDT_SWSYSRST);
		ret &= (~(1 << shift));
		mt_reg_sync_writel((ret|MTK_WDT_SWSYS_RST_KEY), MTK_WDT_SWSYSRST);
	} else { /* means set x bit */
		ret = __raw_readl(MTK_WDT_SWSYSRST);
		ret |= ((1 << shift));
		mt_reg_sync_writel((ret|MTK_WDT_SWSYS_RST_KEY), MTK_WDT_SWSYSRST);
	}
}

int mtk_wdt_dfd_count_en(int value)
{
	volatile unsigned int tmp;

	if (value == 1) {
		/* enable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp |= (MTK_WDT_DFD_COUNT_EN|MTK_WDT_LATCH_CTL_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	} else if (value == 0) {
		/* disable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp &= (~MTK_WDT_DFD_COUNT_EN);
		tmp |= MTK_WDT_LATCH_CTL_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	}
	pr_debug("mtk_wdt_dfd_count_en:MTK_WDT_LATCH_CTL(0x%x)\n", __raw_readl(MTK_WDT_LATCH_CTL));

	return 0;
}

int mtk_wdt_dfd_thermal1_dis(int value)
{
	volatile unsigned int tmp;

	if (value == 1) {
		/* enable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp |= (MTK_WDT_DFD_THERMAL1_DIS|MTK_WDT_LATCH_CTL_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	} else if (value == 0) {
		/* disable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp &= (~MTK_WDT_DFD_THERMAL1_DIS);
		tmp |= MTK_WDT_LATCH_CTL_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	}
	pr_debug("mtk_wdt_dfd_thermal1_dis:MTK_WDT_LATCH_CTL(0x%x)\n", __raw_readl(MTK_WDT_LATCH_CTL));

	return 0;
}

int mtk_wdt_dfd_thermal2_dis(int value)
{
	volatile unsigned int tmp;

	if (value == 1) {
		/* enable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp |= (MTK_WDT_DFD_THERMAL2_DIS|MTK_WDT_LATCH_CTL_KEY);
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	} else if (value == 0) {
		/* disable dfd count */
		tmp = __raw_readl(MTK_WDT_LATCH_CTL);
		tmp &= (~MTK_WDT_DFD_THERMAL2_DIS);
		tmp |= MTK_WDT_LATCH_CTL_KEY;
		mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);
	}
	pr_debug("mtk_wdt_dfd_thermal2_dis:MTK_WDT_LATCH_CTL(0x%x)\n", __raw_readl(MTK_WDT_LATCH_CTL));

	return 0;
}

int mtk_wdt_dfd_timeout(int value)
{
	volatile unsigned int tmp;

	value = value << MTK_WDT_DFD_TIMEOUT_SHIFT;
	value = value & MTK_WDT_DFD_TIMEOUT_MASK;

	/* enable dfd count */
	tmp = __raw_readl(MTK_WDT_LATCH_CTL);
	tmp &= (~MTK_WDT_DFD_TIMEOUT_MASK);
	tmp |= (value|MTK_WDT_LATCH_CTL_KEY);
	mt_reg_sync_writel(tmp, MTK_WDT_LATCH_CTL);

	pr_debug("mtk_wdt_dfd_timeout:MTK_WDT_LATCH_CTL(0x%x)\n", __raw_readl(MTK_WDT_LATCH_CTL));

	return 0;
}

#else
/* ------------------------------------------------------------------------------------------------- */
/* Dummy functions */
/* ------------------------------------------------------------------------------------------------- */
void mtk_wdt_set_time_out_value(unsigned int value) {}
static void mtk_wdt_set_reset_length(unsigned int value) {}
void mtk_wdt_mode_config(bool dual_mode_en, bool irq,	bool ext_en, bool ext_pol, bool wdt_en) {}
int mtk_wdt_enable(enum wk_wdt_en en) { return 0; }
void mtk_wdt_restart(enum wd_restart_type type) {}
static void mtk_wdt_sw_trigger(void){}
static unsigned char mtk_wdt_check_status(void){ return 0; }
void wdt_arch_reset(char mode) {}
int  mtk_wdt_confirm_hwreboot(void){return 0; }
void mtk_wd_suspend(void){}
void mtk_wd_resume(void){}
void wdt_dump_reg(void){}
int mtk_wdt_swsysret_config(int bit, int set_value) { return 0; }
int mtk_wdt_request_mode_set(int mark_bit, WD_REQ_MODE mode) {return 0; }
int mtk_wdt_request_en_set(int mark_bit, WD_REQ_CTL en) {return 0; }
void mtk_wdt_set_c2k_sysrst(unsigned int flag) {}
int mtk_rgu_dram_reserved(int enable) {return 0; }
int mtk_rgu_cfg_emi_dcs(int enable) {return 0; }
int mtk_rgu_mcu_cache_preserve(int enable) {return 0; }
int mtk_wdt_dfd_count_en(int value) {return 0; }
int mtk_wdt_dfd_thermal1_dis(int value) {return 0; }
int mtk_wdt_dfd_thermal2_dis(int value) {return 0; }
int mtk_wdt_dfd_timeout(int value) {return 0; }

#endif /* #ifndef __USING_DUMMY_WDT_DRV__ */

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
get_wd_api(&wd_api);
	wdt_mode_val = __raw_readl(MTK_WDT_STATUS);
	mt_reg_sync_writel(wdt_mode_val, MTK_WDT_NONRST_REG);
    #ifdef	CONFIG_MTK_WD_KICKER
	aee_wdt_printf("\n kick=0x%08x,check=0x%08x,STA=%x\n", wd_api->wd_get_kick_bit(),
	wd_api->wd_get_check_bit(), wdt_mode_val);
	aee_wdt_dump_reg();
    #endif

	aee_wdt_fiq_info(arg, regs, svc_sp);
#if 0
	asm volatile("mov %0, %1\n\t"
		  "mov fp, %2\n\t"
		 : "=r" (sp)
		 : "r" (svc_sp), "r" (preg[11])
		 );
	*((volatile unsigned int *)(0x00000000)); /* trigger exception */
#endif
}
#else /* CONFIG_FIQ_GLUE */
static irqreturn_t mtk_wdt_isr(int irq, void *dev_id)
{
	pr_err("fwq mtk_wdt_isr\n");
#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
	/* mt65xx_irq_mask(AP_RGU_WDT_IRQ_ID); */
	rgu_wdt_intr_has_trigger = 1;
	wdt_report_info();
	WARN_ON(1);

#endif
	return IRQ_HANDLED;
}
#endif /* CONFIG_FIQ_GLUE */

/*
 * Device interface
 */
static int mtk_wdt_probe(struct platform_device *dev)
{
	int ret = 0;
	unsigned int interval_val;
	struct device_node *node;
	u32 ints[2] = { 0, 0 };

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

	node = of_find_compatible_node(NULL, NULL, "mediatek, mrdump_ext_rst-eint");

	if (node && of_device_is_available(node)) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		ext_debugkey_io = ints[0];
	}

	pr_err("mtk_wdt_probe: ext_debugkey_io=%d\n", ext_debugkey_io);

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */

#ifndef CONFIG_FIQ_GLUE
	pr_debug("******** MTK WDT register irq ********\n");
    #ifdef CONFIG_KICK_SPM_WDT
	ret = spm_wdt_register_irq((irq_handler_t)mtk_wdt_isr);
    #else
	ret = request_irq(AP_RGU_WDT_IRQ_ID, (irq_handler_t)mtk_wdt_isr, IRQF_TRIGGER_NONE, "mt_wdt", NULL);
    #endif		/* CONFIG_KICK_SPM_WDT */
#else
	pr_debug("******** MTK WDT register fiq ********\n");
    #ifdef CONFIG_KICK_SPM_WDT
	ret = spm_wdt_register_fiq(wdt_fiq);
    #else
	ret = request_fiq(AP_RGU_WDT_IRQ_ID, wdt_fiq, IRQF_TRIGGER_FALLING, NULL);
    #endif		/* CONFIG_KICK_SPM_WDT */
#endif

	if (ret != 0) {
		pr_err("mtk_wdt_probe : failed to request irq (%d)\n", ret);
		return ret;
	}
	pr_debug("mtk_wdt_probe : Success to request irq\n");

    #ifdef CONFIG_KICK_SPM_WDT
	spm_wdt_init();
    #endif

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

	/* Reset External debug key */
	mtk_wdt_request_en_set(MTK_WDT_REQ_MODE_SYSRST, WD_REQ_DIS);
	mtk_wdt_request_en_set(MTK_WDT_REQ_MODE_EINT, WD_REQ_DIS);
	mtk_wdt_request_mode_set(MTK_WDT_REQ_MODE_SYSRST, WD_REQ_IRQ_MODE);
	mtk_wdt_request_mode_set(MTK_WDT_REQ_MODE_EINT, WD_REQ_IRQ_MODE);
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

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
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

	/*aee_sram_printk("[WDT] suspend\n");*/
	pr_debug("[WDT] suspend\n");
}

void mtk_wd_resume(void)
{

	if (g_wdt_enable == 1) {
		mtk_wdt_set_time_out_value(g_last_time_time_out_value);
		mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
		mtk_wdt_restart(WD_TYPE_NORMAL);
	}

	/*aee_sram_printk("[WDT] resume(%d)\n", g_wdt_enable);*/
	pr_debug("[WDT] resume(%d)\n", g_wdt_enable);
}



static struct platform_driver mtk_wdt_driver = {

	.driver     = {
		.name	= "mtk-wdt",
#ifdef CONFIG_OF
		.of_match_table = rgu_of_match,
#endif
	},
	.probe	= mtk_wdt_probe,
	.remove	= mtk_wdt_remove,
	.shutdown	= mtk_wdt_shutdown,
/* .suspend	= mtk_wdt_suspend, */
/* .resume	= mtk_wdt_resume, */
};
#ifndef CONFIG_OF
struct platform_device mtk_device_wdt = {
		.name		= "mtk-wdt",
		.id		= 0,
		.dev		= {
		}
};
#endif

#ifdef CONFIG_KICK_SPM_WDT
static void spm_wdt_init(void)
{
	unsigned int tmp;
	/* set scpsys reset mode , not trigger irq */
	/* #ifndef CONFIG_ARM64 */
	/*6795 Macro*/
	tmp = __raw_readl(MTK_WDT_REQ_MODE);
	tmp |=  MTK_WDT_REQ_MODE_KEY;
	tmp |= (MTK_WDT_REQ_MODE_SPM_SCPSYS);
	mt_reg_sync_writel(tmp, MTK_WDT_REQ_MODE);

	tmp = __raw_readl(MTK_WDT_REQ_IRQ_EN);
	tmp |= MTK_WDT_REQ_IRQ_KEY;
	tmp &= ~(MTK_WDT_REQ_IRQ_SPM_SCPSYS_EN);
	mt_reg_sync_writel(tmp, MTK_WDT_REQ_IRQ_EN);
	/* #endif */

	pr_debug("mtk_wdt_init [MTK_WDT] not use RGU WDT use_SPM_WDT!! ********\n");
	/* pr_alert("WDT REQ_MODE=0x%x,  WDT REQ_EN=0x%x\n",*/
	/* __raw_readl(MTK_WDT_REQ_MODE), __raw_readl(MTK_WDT_REQ_IRQ_EN)); */

	tmp = __raw_readl(MTK_WDT_MODE);
	tmp |= MTK_WDT_MODE_KEY;
	/* disable wdt */
	tmp &= (~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE|MTK_WDT_MODE_DUAL_MODE));

	/* Bit 4: WDT_Auto_restart, this is a reserved bit, we use it as bypass powerkey flag. */
	/* Because HW reboot always need reboot to kernel, we set it always. */
	tmp |= MTK_WDT_MODE_AUTO_RESTART;
	/* BIt2  ext signal */
	tmp |= MTK_WDT_MODE_EXTEN;
	mt_reg_sync_writel(tmp, MTK_WDT_MODE);

}
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
/*this function is for those user who need WDT APIs before WDT driver's probe*/
static int __init mtk_wdt_get_base_addr(void)
{
#ifdef CONFIG_OF
	struct device_node *np_rgu;

	np_rgu = of_find_compatible_node(NULL, NULL, rgu_of_match[0].compatible);

	if (!toprgu_base) {
		toprgu_base = of_iomap(np_rgu, 0);
		if (!toprgu_base)
			pr_err("RGU iomap failed\n");

		pr_debug("RGU base: 0x%p\n", toprgu_base);
	}

#endif
	return 0;
}
core_initcall(mtk_wdt_get_base_addr);
postcore_initcall(mtk_wdt_init);
module_exit(mtk_wdt_exit);

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("Watchdog Device Driver");
MODULE_LICENSE("GPL");

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

#include "mtk-phy.h"

#ifdef CONFIG_PROJECT_PHY
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif
#include <linux/io.h>
#include "mtk-phy-asic.h"
#include "mu3d_hal_osal.h"
#ifdef CONFIG_MTK_UART_USB_SWITCH
#include "mu3d_hal_usb_drv.h"
#endif

#include <mt-plat/upmu_common.h>
#include "mtk_spm_resource_req.h"
#include "mtk_idle.h"
#include "mtk_clk_id.h"
#include "musb_core.h"

#ifdef CONFIG_OF
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mt-plat/mtk_chip.h>
#include "mtk_devinfo.h"

static int usb20_phy_rev6;
#ifndef CONFIG_MTK_CLKMGR
static struct clk *ssusb_clk;
static struct clk *ssusb_clk_sck;
#endif
static DEFINE_SPINLOCK(mu3phy_clock_lock);

bool sib_mode;

#ifdef VCORE_OPS_DEV
#include <mtk_vcorefs_manager.h>
DEFINE_MUTEX(vcore_op_lock);
void vcore_op(int on)
{
	int ret;
	static int stauts_on;

#ifdef U3_COMPLIANCE
	os_printk(K_INFO, "%s, U3_COMPLIANCE, directly return\n", __func__);
	return;
#endif

	if (mt_get_chip_sw_ver() < CHIP_SW_VER_02) {
		os_printk(K_INFO, "%s, directly return\n", __func__);
		return;
	}

	mutex_lock(&vcore_op_lock);

	if (stauts_on != on) {
		if (on)
			ret = vcorefs_request_dvfs_opp(KIR_USB, OPP_1);
		else
			ret = vcorefs_request_dvfs_opp(KIR_USB, OPP_UNREQ);

		if (ret == 0)
			stauts_on = on;
		os_printk(K_INFO, "%s, on<%d>, ret<%d>\n", __func__, on, ret);
	}
	mutex_unlock(&vcore_op_lock);
}
#endif

typedef enum {
	VA10_OP_OFF = 0,
	VA10_OP_ON,
} VA10_OP;

void VA10_operation(int op)
{
	int ret;
	unsigned short sw_en, volsel;

	if (op == VA10_OP_ON) {
		/* VA10 is shared by U3/UFS */
		/* default on and set voltage by PMIC */
		/* off/on in SPM suspend/resume */

		ret = pmic_set_register_value(PMIC_RG_VA10_SW_EN, 0x01);
		if (ret)
			os_printk(K_INFO, "%s VA10 enable FAIL!!!\n", __func__);

	} else
		os_printk(K_INFO, "%s skip operation %d for VA10\n", __func__, op);

	sw_en = pmic_get_register_value(PMIC_RG_VA10_SW_EN);
	volsel = pmic_get_register_value(PMIC_RG_VA10_VOSEL);
	os_printk(K_INFO, "%s, VA10, sw_en:%x, volsel:%x\n", __func__, sw_en, volsel);
}

static int dpidle_status = USB_DPIDLE_ALLOWED;
static DEFINE_SPINLOCK(usb_hal_dpidle_lock);
#define DPIDLE_TIMER_INTERVAL_MS 30
static void issue_dpidle_timer(void);
static void dpidle_timer_wakeup_func(unsigned long data)
{
	struct timer_list *timer = (struct timer_list *)data;

	{
		static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 1);
		static int skip_cnt;

		if (__ratelimit(&ratelimit)) {
			os_printk(K_INFO, "dpidle_timer<%p> alive, skip_cnt<%d>\n", timer, skip_cnt);
			skip_cnt = 0;
		} else
			skip_cnt++;
	}
	os_printk(K_DEBUG, "dpidle_timer<%p> alive...\n", timer);
	if (dpidle_status == USB_DPIDLE_TIMER)
		issue_dpidle_timer();
	kfree(timer);
}
static void issue_dpidle_timer(void)
{
	struct timer_list *timer;

	timer = kzalloc(sizeof(struct timer_list), GFP_ATOMIC);
	if (!timer)
		return;

	os_printk(K_DEBUG, "add dpidle_timer<%p>\n", timer);
	init_timer(timer);
	timer->function = dpidle_timer_wakeup_func;
	timer->data = (unsigned long)timer;
	timer->expires = jiffies + msecs_to_jiffies(DPIDLE_TIMER_INTERVAL_MS);
	add_timer(timer);
}

void usb_hal_dpidle_request(int mode)
{
	unsigned long flags;

#ifdef U3_COMPLIANCE
	os_printk(K_INFO, "%s, U3_COMPLIANCE, directly return\n", __func__);
	return;
#endif

	spin_lock_irqsave(&usb_hal_dpidle_lock, flags);

	/* update dpidle_status */
	dpidle_status = mode;

	switch (mode) {
	case USB_DPIDLE_ALLOWED:
		spm_resource_req(SPM_RESOURCE_USER_SSUSB, 0);
		enable_dpidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		enable_soidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		os_printk(K_INFO, "USB_DPIDLE_ALLOWED\n");
		break;
	case USB_DPIDLE_FORBIDDEN:
		disable_dpidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		disable_soidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		{
			static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 3);

			if (__ratelimit(&ratelimit))
				os_printk(K_INFO, "USB_DPIDLE_FORBIDDEN\n");
		}
		break;
	case USB_DPIDLE_SRAM:
		spm_resource_req(SPM_RESOURCE_USER_SSUSB,
				SPM_RESOURCE_CK_26M | SPM_RESOURCE_MAINPLL);
		enable_dpidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		enable_soidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		{
			static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 3);
			static int skip_cnt;

			if (__ratelimit(&ratelimit)) {
				os_printk(K_INFO, "USB_DPIDLE_SRAM, skip_cnt<%d>\n", skip_cnt);
				skip_cnt = 0;
			} else
				skip_cnt++;
		}
		break;
	case USB_DPIDLE_TIMER:
		spm_resource_req(SPM_RESOURCE_USER_SSUSB, SPM_RESOURCE_CK_26M);
		enable_dpidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		enable_soidle_by_bit(MTK_CG_PERI3_RG_USB_P0_CK_PDN_STA);
		os_printk(K_INFO, "USB_DPIDLE_TIMER\n");
		issue_dpidle_timer();
		break;
	default:
		os_printk(K_WARNIN, "[ERROR] Are you kidding!?!?\n");
		break;
	}

	spin_unlock_irqrestore(&usb_hal_dpidle_lock, flags);
}

static bool usb_enable_clock(bool enable)
{
	static int count;
	unsigned long flags;

	if (!ssusb_clk || IS_ERR(ssusb_clk)) {
		pr_err("clock not ready, ssusb_clk:%p", ssusb_clk);
		return -1;
	}

	if ((mt_get_chip_sw_ver() >= CHIP_SW_VER_02) && (!ssusb_clk_sck || IS_ERR(ssusb_clk_sck))) {
		pr_err("clock not ready, ssusb_clk_sck:%p", ssusb_clk_sck);
		return -1;
	}

	spin_lock_irqsave(&mu3phy_clock_lock, flags);
	os_printk(K_INFO, "CG, enable<%d>, count<%d>\n", enable, count);

	if (enable && count == 0) {
		usb_hal_dpidle_request(USB_DPIDLE_FORBIDDEN);
		if (clk_enable(ssusb_clk) != 0)
			pr_err("ssusb_ref_clk enable fail\n");
		if ((mt_get_chip_sw_ver() >= CHIP_SW_VER_02) && clk_enable(ssusb_clk_sck) != 0)
			pr_err("ssusb_ref_clk_sck enable fail\n");


	} else if (!enable && count == 1) {
		if (mt_get_chip_sw_ver() >= CHIP_SW_VER_02)
			clk_disable(ssusb_clk_sck);
		clk_disable(ssusb_clk);
		usb_hal_dpidle_request(USB_DPIDLE_ALLOWED);
	}

	if (enable)
		count++;
	else
		count = (count == 0) ? 0 : (count - 1);

	spin_unlock_irqrestore(&mu3phy_clock_lock, flags);

	return 0;
}


#ifdef NEVER
/*Turn on/off ADA_SSUSB_XTAL_CK 26MHz*/
void enable_ssusb_xtal_clock(bool enable)
{
	if (enable) {
		/*
		 * 1 *AP_PLL_CON0 =| 0x1 [0]=1: RG_LTECLKSQ_EN
		 * 2 Wait PLL stable (100us)
		 * 3 *AP_PLL_CON0 =| 0x2 [1]=1: RG_LTECLKSQ_LPF_EN
		 * 4 *AP_PLL_CON2 =| 0x1 [0]=1: DA_REF2USB_TX_EN
		 * 5 Wait PLL stable (100us)
		 * 6 *AP_PLL_CON2 =| 0x2 [1]=1: DA_REF2USB_TX_LPF_EN
		 * 7 *AP_PLL_CON2 =| 0x4 [2]=1: DA_REF2USB_TX_OUT_EN
		 */
		writel(readl((void __iomem *)AP_PLL_CON0) | (0x00000001),
		       (void __iomem *)AP_PLL_CON0);
		/*Wait 100 usec */
		udelay(100);

		writel(readl((void __iomem *)AP_PLL_CON0) | (0x00000002),
		       (void __iomem *)AP_PLL_CON0);

		writel(readl((void __iomem *)AP_PLL_CON2) | (0x00000001),
		       (void __iomem *)AP_PLL_CON2);

		/*Wait 100 usec */
		udelay(100);

		writel(readl((void __iomem *)AP_PLL_CON2) | (0x00000002),
		       (void __iomem *)AP_PLL_CON2);

		writel(readl((void __iomem *)AP_PLL_CON2) | (0x00000004),
		       (void __iomem *)AP_PLL_CON2);
	} else {
		/*
		 * AP_PLL_CON2 &= 0xFFFFFFF8        [2]=0: DA_REF2USB_TX_OUT_EN
		 *                                  [1]=0: DA_REF2USB_TX_LPF_EN
		 *                                  [0]=0: DA_REF2USB_TX_EN
		 */
		/* writel(readl((void __iomem *)AP_PLL_CON2)&~(0x00000007), */
		/* (void __iomem *)AP_PLL_CON2); */
	}
}

/*Turn on/off AD_LTEPLL_SSUSB26M_CK 26MHz*/
void enable_ssusb26m_ck(bool enable)
{
	if (enable) {
		/*
		 * 1 *AP_PLL_CON0 =| 0x1 [0]=1: RG_LTECLKSQ_EN
		 * 2 Wait PLL stable (100us)
		 * 3 *AP_PLL_CON0 =| 0x2 [1]=1: RG_LTECLKSQ_LPF_EN
		 */
		writel(readl((void __iomem *)AP_PLL_CON0) | (0x00000001),
		       (void __iomem *)AP_PLL_CON0);
		/*Wait 100 usec */
		udelay(100);

		writel(readl((void __iomem *)AP_PLL_CON0) | (0x00000002),
		       (void __iomem *)AP_PLL_CON0);

	} else {
		/*
		 * AP_PLL_CON2 &= 0xFFFFFFF8        [2]=0: DA_REF2USB_TX_OUT_EN
		 *                                  [1]=0: DA_REF2USB_TX_LPF_EN
		 *                                  [0]=0: DA_REF2USB_TX_EN
		 */
		/* writel(readl((void __iomem *)AP_PLL_CON2)&~(0x00000007), */
		/* (void __iomem *)AP_PLL_CON2); */
	}
}
#endif				/* NEVER */

void usb20_pll_settings(bool host, bool forceOn)
{
	/* FIXME */
	return;

	if (host) {
		if (forceOn) {
			os_printk(K_DEBUG, "%s-%d - Set USBPLL_FORCE_ON.\n", __func__, __LINE__);
			/* Set RG_SUSPENDM to 1 */
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_SUSPENDM_OFST,
				RG_SUSPENDM, 1);
			/* force suspendm = 1 */
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST,
				FORCE_SUSPENDM, 1);
#ifndef CONFIG_MTK_TYPEC_SWITCH
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_PHYA_REG6, RG_SSUSB_RESERVE6_OFST,
				RG_SSUSB_RESERVE6, 0x1);
#endif

		} else {
			os_printk(K_DEBUG, "%s-%d - Clear USBPLL_FORCE_ON.\n", __func__, __LINE__);
			/* Set RG_SUSPENDM to 1 */
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_SUSPENDM_OFST,
				RG_SUSPENDM, 0);
			/* force suspendm = 1 */
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST,
				FORCE_SUSPENDM, 0);
#ifndef CONFIG_MTK_TYPEC_SWITCH
			U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_PHYA_REG6, RG_SSUSB_RESERVE6_OFST,
				RG_SSUSB_RESERVE6, 0x0);
#endif
			return;
		}
	}

	os_printk(K_DEBUG, "%s-%d - Set PLL_FORCE_MODE and SIFSLV PLL_FORCE_ON.\n", __func__,
		  __LINE__);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR2_0, RG_SIFSLV_USB20_PLL_FORCE_MODE_OFST,
			  RG_SIFSLV_USB20_PLL_FORCE_MODE, 0x1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDCR0, RG_SIFSLV_USB20_PLL_FORCE_ON_OFST,
			  RG_SIFSLV_USB20_PLL_FORCE_ON, 0x0);
}

void usb20_rev6_setting(int value, bool is_update)
{
	if (is_update)
		usb20_phy_rev6 = value;

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST,
		RG_USB20_PHY_REV_6, value);
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool in_uart_mode;
void uart_usb_switch_dump_register(void)
{
	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(1); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(true);
	udelay(250);

# ifdef CONFIG_MTK_FPGA
	pr_debug("[MUSB]addr: 0x6B, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYDTM0 + 0x3));
	pr_debug("[MUSB]addr: 0x6E, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYDTM1 + 0x2));
	pr_debug("[MUSB]addr: 0x22, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYACR4 + 0x2));
	pr_debug("[MUSB]addr: 0x68, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYDTM0));
	pr_debug("[MUSB]addr: 0x6A, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYDTM0 + 0x2));
	pr_debug("[MUSB]addr: 0x1A, value: %x\n",
		USB_PHY_Read_Register8((phys_addr_t) (uintptr_t)U3D_U2PHYACR6 + 0x2));
#else
#if 0
	os_printk(K_INFO, "[MUSB]addr: 0x6B, value: %x\n", U3PhyReadReg8(U3D_U2PHYDTM0 + 0x3));
	os_printk(K_INFO, "[MUSB]addr: 0x6E, value: %x\n", U3PhyReadReg8(U3D_U2PHYDTM1 + 0x2));
	os_printk(K_INFO, "[MUSB]addr: 0x22, value: %x\n", U3PhyReadReg8(U3D_U2PHYACR4 + 0x2));
	os_printk(K_INFO, "[MUSB]addr: 0x68, value: %x\n", U3PhyReadReg8(U3D_U2PHYDTM0));
	os_printk(K_INFO, "[MUSB]addr: 0x6A, value: %x\n", U3PhyReadReg8(U3D_U2PHYDTM0 + 0x2));
	os_printk(K_INFO, "[MUSB]addr: 0x1A, value: %x\n", U3PhyReadReg8(U3D_USBPHYACR6 + 0x2));
#else
	os_printk(K_INFO, "[MUSB]addr: 0x18, value: 0x%x\n",
		U3PhyReadReg32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6));
	os_printk(K_INFO, "[MUSB]addr: 0x20, value: 0x%x\n",
		U3PhyReadReg32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4));
	os_printk(K_INFO, "[MUSB]addr: 0x68, value: 0x%x\n",
		U3PhyReadReg32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0));
	os_printk(K_INFO, "[MUSB]addr: 0x6C, value: 0x%x\n",
		U3PhyReadReg32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1));
#endif
#endif

	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(0); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(false);

	/*os_printk(K_INFO, "[MUSB]addr: 0x110020B0 (UART0), value: %x\n\n", */
	/*	  DRV_Reg8(ap_uart0_base + 0xB0)); */
}

bool usb_phy_check_in_uart_mode(void)
{
	PHY_INT32 usb_port_mode;

	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(1); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(true);

	udelay(250);
	usb_port_mode = U3PhyReadReg32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0) >> RG_UART_MODE_OFST;

	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(0);*/

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(false);
	os_printk(K_INFO, "%s+ usb_port_mode = %d\n", __func__, usb_port_mode);

	if (usb_port_mode == 0x1)
		return true;
	else
		return false;
}

void usb_phy_switch_to_uart(void)
{
	PHY_INT32 ret;

	if (usb_phy_check_in_uart_mode()) {
		os_printk(K_INFO, "%s+ UART_MODE\n", __func__);
		return;
	}

	os_printk(K_INFO, "%s+ USB_MODE\n", __func__);

	/*---POWER-----*/
	/*AVDD18_USB_P0 is always turned on. The driver does _NOT_ need to control it. */
	/*hwPowerOn(MT6332_POWER_LDO_VUSB33, VOL_3300, "VDD33_USB_P0"); */
	ret = pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
	if (ret)
		pr_debug("VUSB33 enable FAIL!!!\n");

	VA10_operation(VA10_OP_ON);

	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(1); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(true);
	udelay(250);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
			  RG_USB20_BC11_SW_EN, 0);

	/* Set RG_SUSPENDM to 1 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);

	/* force suspendm = 1 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);

	/* Set ru_uart_mode to 2'b01 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_UART_MODE_OFST, RG_UART_MODE, 1);

	/* Set RG_UART_EN to 1 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_TX_OE_OFST, FORCE_UART_TX_OE, 1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_BIAS_EN_OFST, FORCE_UART_BIAS_EN,
			  1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_TX_OE_OFST, RG_UART_TX_OE, 1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_BIAS_EN_OFST, RG_UART_BIAS_EN, 1);

	/* Set RG_USB20_DM_100K_EN to 1 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_DM_100K_EN_OFST,
			  RG_USB20_DM_100K_EN, 1);

	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(0); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(false);

	/* SET UART_USB_SEL to UART0 */
	DRV_WriteReg32(ap_uart0_base + 0x6E0, (DRV_Reg32(ap_uart0_base + 0x6E0) | (1<<25)));

	in_uart_mode = true;
}


void usb_phy_switch_to_usb(void)
{
	in_uart_mode = false;

	/* CLEAR UART_USB_SEL to UART0 */
	DRV_WriteReg32(ap_uart0_base + 0x6E0, (DRV_Reg32(ap_uart0_base + 0x6E0) & ~(1<<25)));

	/* clear force_uart_en */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);

	/* clear ru_uart_mode to 2'b00 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_UART_MODE_OFST, RG_UART_MODE, 0);

	phy_init_soc(u3phy);

	/* disable the USB clock turned on in phy_init_soc() */
	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(0); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(false);
}
#endif

#ifdef CONFIG_U3_PHY_SMT_LOOP_BACK_SUPPORT

#define USB30_PHYD_PIPE0 (SSUSB_SIFSLV_U3PHYD_BASE+0x40)
#define USB30_PHYD_RX0 (SSUSB_SIFSLV_U3PHYD_BASE+0x2c)
#define USB30_PHYD_MIX0 (SSUSB_SIFSLV_U3PHYD_BASE+0x0)
#define USB30_PHYD_T2RLB (SSUSB_SIFSLV_U3PHYD_BASE+0x30)

bool u3_loop_back_test(void)
{
	int reg;

	bool loop_back_ret = false;

	pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
	/* VA10 is shared by U3/UFS */
	/* default on and set voltage by PMIC */
	/* off/on in SPM suspend/resume */
	pmic_set_register_value(PMIC_RG_VA10_SW_EN, 0x01);
	usb_enable_clock(true);

	/*SSUSB_IP_SW_RST = 0*/
	writel(0x00031000, U3D_SSUSB_IP_PW_CTRL0);
	/*SSUSB_IP_HOST_PDN = 0*/
	writel(0x00000000, U3D_SSUSB_IP_PW_CTRL1);
	/*SSUSB_IP_DEV_PDN = 0*/
	writel(0x00000000, U3D_SSUSB_IP_PW_CTRL2);
	/*SSUSB_IP_PCIE_PDN = 0*/
	writel(0x00000000, U3D_SSUSB_IP_PW_CTRL3);
	/*SSUSB_U3_PORT_DIS/SSUSB_U3_PORT_PDN = 0*/
	writel(0x0000000C, U3D_SSUSB_U3_CTRL_0P);
	mdelay(10);

	writel((readl(USB30_PHYD_PIPE0)&~(0x01<<30))|0x01<<30,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x01<<28))|0x00<<28,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x03<<26))|0x01<<26,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x03<<24))|0x00<<24,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x01<<22))|0x00<<22,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x01<<21))|0x00<<21,
							USB30_PHYD_PIPE0);
	writel((readl(USB30_PHYD_PIPE0)&~(0x01<<20))|0x01<<20,
							USB30_PHYD_PIPE0);
	mdelay(10);

	/*T2R loop back disable*/
	writel((readl(USB30_PHYD_RX0)&~(0x01<<15))|0x00<<15,
							USB30_PHYD_RX0);
	mdelay(10);

	/* TSEQ lock detect threshold */
	writel((readl(USB30_PHYD_MIX0)&~(0x07<<24))|0x07<<24,
							USB30_PHYD_MIX0);
	/* set default TSEQ polarity check value = 1 */
	writel((readl(USB30_PHYD_MIX0)&~(0x01<<28))|0x01<<28,
							USB30_PHYD_MIX0);
	/* TSEQ polarity check enable */
	writel((readl(USB30_PHYD_MIX0)&~(0x01<<29))|0x01<<29,
							USB30_PHYD_MIX0);
	/* TSEQ decoder enable */
	writel((readl(USB30_PHYD_MIX0)&~(0x01<<30))|0x01<<30,
							USB30_PHYD_MIX0);
	mdelay(10);

	/* set T2R loop back TSEQ length (x 16us) */
	writel((readl(USB30_PHYD_T2RLB)&~(0xff<<0))|0xF0<<0,
							USB30_PHYD_T2RLB);
	/* set T2R loop back BDAT reset period (x 16us) */
	writel((readl(USB30_PHYD_T2RLB)&~(0x0f<<12))|0x0F<<12,
							USB30_PHYD_T2RLB);
	/* T2R loop back pattern select */
	writel((readl(USB30_PHYD_T2RLB)&~(0x03<<8))|0x00<<8,
							USB30_PHYD_T2RLB);
	mdelay(10);

	/* T2R loop back serial mode */
	writel((readl(USB30_PHYD_RX0)&~(0x01<<13))|0x01<<13,
							USB30_PHYD_RX0);
	/* T2R loop back parallel mode = 0 */
	writel((readl(USB30_PHYD_RX0)&~(0x01<<12))|0x00<<12,
							USB30_PHYD_RX0);
	/* T2R loop back mode enable */
	writel((readl(USB30_PHYD_RX0)&~(0x01<<11))|0x01<<11,
							USB30_PHYD_RX0);
	/* T2R loop back enable */
	writel((readl(USB30_PHYD_RX0)&~(0x01<<15))|0x01<<15,
							USB30_PHYD_RX0);
	mdelay(100);

	reg = U3PhyReadReg32((phys_addr_t)SSUSB_SIFSLV_U3PHYD_BASE+0xb4);
	os_printk(K_INFO, "read back             : 0x%x\n", reg);
	os_printk(K_INFO, "read back t2rlb_lock  : %d\n", (reg>>2)&0x01);
	os_printk(K_INFO, "read back t2rlb_pass  : %d\n", (reg>>3)&0x01);
	os_printk(K_INFO, "read back t2rlb_passth: %d\n", (reg>>4)&0x01);

	if ((reg&0x0E) == 0x0E)
		loop_back_ret = true;
	else
		loop_back_ret = false;

	return loop_back_ret;
}
#endif

#define RG_SSUSB_VUSB10_ON (1<<29)
#define RG_SSUSB_VUSB10_ON_OFST (29)

#ifdef CONFIG_MTK_SIB_USB_SWITCH
#include <linux/wakelock.h>
static struct wake_lock sib_wakelock;
void usb_phy_sib_enable_switch(bool enable)
{
	/*
	 * It's MD debug usage. No need to care low power.
	 * Thus, no power off BULK and Clock at the end of function.
	 * MD SIB still needs these power and clock source.
	 */
	pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
	/* VA10 is shared by U3/UFS */
	/* default on and set voltage by PMIC */
	/* off/on in SPM suspend/resume */
	pmic_set_register_value(PMIC_RG_VA10_SW_EN, 0x01);

	usb_enable_clock(true);
	udelay(250);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
			  RG_SSUSB_VUSB10_ON, 1);
	/* SSUSB_IP_SW_RST = 0 */
	U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif_base + 0x700), 0x00031000);
	/* SSUSB_IP_HOST_PDN = 0 */
	U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif_base + 0x704), 0x00000000);
	/* SSUSB_IP_DEV_PDN = 0 */
	U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif_base + 0x708), 0x00000000);
	/* SSUSB_IP_PCIE_PDN = 0 */
	U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif_base + 0x70C), 0x00000000);
	/* SSUSB_U3_PORT_DIS/SSUSB_U3_PORT_PDN = 0*/
	U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif_base + 0x730), 0x0000000C);

	/*
	 * USBMAC mode is 0x62910002 (bit 1)
	 * MDSIB  mode is 0x62910008 (bit 3)
	 * 0x0629 just likes a signature. Can't be removed.
	 */
	if (enable) {
		U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif2_base+0x300), 0x62910008);
		sib_mode = true;
		if (!wake_lock_active(&sib_wakelock))
			wake_lock(&sib_wakelock);
	} else {
		U3PhyWriteReg32((phys_addr_t) (uintptr_t) (u3_sif2_base+0x300), 0x62910002);
		sib_mode = false;
		if (wake_lock_active(&sib_wakelock))
			wake_unlock(&sib_wakelock);
	}
}

bool usb_phy_sib_enable_switch_status(void)
{
	int reg;
	bool ret;

	pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
	/* VA10 is shared by U3/UFS */
	/* default on and set voltage by PMIC */
	/* off/on in SPM suspend/resume */
	pmic_set_register_value(PMIC_RG_VA10_SW_EN, 0x01);

	usb_enable_clock(true);
	udelay(250);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
			  RG_SSUSB_VUSB10_ON, 1);

	reg = U3PhyReadReg32((phys_addr_t) (uintptr_t) (u3_sif2_base+0x300));
	if (reg == 0x62910008)
		ret = true;
	else
		ret = false;

	return ret;
}
#endif


#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
int usb2jtag_usb_init(void)
{
	struct device_node *node = NULL;
	void __iomem *usb3_sif2_base;
	u32 temp;

	node = of_find_compatible_node(NULL, NULL, "mediatek,usb3");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mediatek,usb3 failed\n");
		return -1;
	}

	usb3_sif2_base = of_iomap(node, 2);
	if (!usb3_sif2_base) {
		pr_err("[USB2JTAG] iomap usb3_sif2_base failed\n");
		return -1;
	}

	/* rg_usb20_gpio_ctl: bit[9] = 1 */
	temp = readl(usb3_sif2_base + 0x820);
	writel(temp | (1 << 9), usb3_sif2_base + 0x820);

	/* RG_USB20_BC11_SW_EN: bit[23] = 0 */
	temp = readl(usb3_sif2_base + 0x818);
	writel(temp & ~(1 << 23), usb3_sif2_base + 0x818);

	/* RG_USB20_BGR_EN: bit[0] = 1 */
	temp = readl(usb3_sif2_base + 0x800);
	writel(temp | (1 << 0), usb3_sif2_base + 0x800);

	/* rg_sifslv_mac_bandgap_en: bit[17] = 0 */
	temp = readl(usb3_sif2_base + 0x808);
	writel(temp & ~(1 << 17), usb3_sif2_base + 0x808);

	/* wait stable */
	mdelay(1);

	iounmap(usb3_sif2_base);

	return 0;
}
#endif


/*This "power on/initial" sequence refer to "6593_USB_PORT0_PWR Sequence 20130729.xls"*/
PHY_INT32 phy_init_soc(struct u3phy_info *info)
{
	PHY_INT32 ret;

	os_printk(K_INFO, "%s+\n", __func__);

#ifdef CONFIG_MTK_SIB_USB_SWITCH
	wake_lock_init(&sib_wakelock, WAKE_LOCK_SUSPEND, "SIB.lock");
#endif

	/*This power on sequence refers to Sheet .1 of "6593_USB_PORT0_PWR Sequence 20130729.xls" */

	/*---POWER-----*/
	/*AVDD18_USB_P0 is always turned on. The driver does _NOT_ need to control it. */
	/*hwPowerOn(MT6332_POWER_LDO_VUSB33, VOL_3300, "VDD33_USB_P0"); */
	ret = pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
	if (ret)
		pr_debug("VUSB33 enable FAIL!!!\n");

	VA10_operation(VA10_OP_ON);

	/*---CLOCK-----*/
	/* ADA_SSUSB_XTAL_CK:26MHz */
	/*set_ada_ssusb_xtal_ck(1); */

	/* AD_LTEPLL_SSUSB26M_CK:26MHz always on */
	/* It seems that when turning on ADA_SSUSB_XTAL_CK, AD_LTEPLL_SSUSB26M_CK will also turn on. */
	/* enable_ssusb26m_ck(true); */

	/* f_fusb30_ck:125MHz */
	usb_enable_clock(true);

	/* AD_SSUSB_48M_CK:48MHz */
	/* It seems that when turning on f_fusb30_ck, AD_SSUSB_48M_CK will also turn on. */

	/*Wait 50 usec */
	udelay(250);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
			  RG_SSUSB_VUSB10_ON, 1);

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		goto reg_done;
#endif

	/*switch to USB function. (system register, force ip into usb mode) */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL,
			  0);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);
	/*DP/DM BC1.1 path Disable */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
			  RG_USB20_BC11_SW_EN, 0);
	/*dp_100k disable */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_DP_100K_MODE_OFST,
			  RG_USB20_DP_100K_MODE, 1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, USB20_DP_100K_EN_OFST, USB20_DP_100K_EN, 0);
	/*dm_100k disable */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_DM_100K_EN_OFST,
			  RG_USB20_DM_100K_EN, 0);
	/*OTG Enable */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST,
			  RG_USB20_OTG_VBUSCMP_EN, 1);
	/*Release force suspendm.  (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll) */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0);

	/*Wait 800 usec */
	udelay(800);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, FORCE_VBUSVALID_OFST, FORCE_VBUSVALID, 1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, FORCE_AVALID_OFST, FORCE_AVALID, 1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, FORCE_SESSEND_OFST, FORCE_SESSEND, 1);

	/* USB PLL Force settings */
	usb20_pll_settings(false, false);

	/* RG_SSUSB_RESERVE[10] to 1, adjust leakge of TX common mode voltage spec margin via FT */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (SSUSB_USB30_PHYA_SIV_B_BASE + 0x2C), 10, (0x1<<10), 1);

#ifdef CONFIG_MTK_UART_USB_SWITCH
reg_done:
#endif
	os_printk(K_DEBUG, "%s-\n", __func__);

	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration(struct u3phy_info *info)
{
	PHY_INT32 i = 0;
	PHY_INT32 fgRet = 0;
	PHY_INT32 u4FmOut = 0;
	PHY_INT32 u4Tmp = 0;

	os_printk(K_DEBUG, "%s\n", __func__);

	/* => RG_USB20_HSTX_SRCAL_EN = 1 */
	/* enable USB ring oscillator */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR5, RG_USB20_HSTX_SRCAL_EN_OFST,
			  RG_USB20_HSTX_SRCAL_EN, 1);

	/* wait 1us */
	udelay(1);

	/* => USBPHY base address + 0x110 = 1 */
	/* Enable free run clock */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x110)
			  , RG_FRCK_EN_OFST, RG_FRCK_EN, 0x1);

	/* => USBPHY base address + 0x100 = 0x04 */
	/* Setting cyclecnt */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x100)
			  , RG_CYCLECNT_OFST, RG_CYCLECNT, 0x400);

	/* => USBPHY base address + 0x100 = 0x01 */
	/* Enable frequency meter */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x100)
			  , RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0x1);

	/* USB_FM_VLD, should be 1'b1, Read frequency valid */
	os_printk(K_DEBUG, "Freq_Valid=(0x%08X)\n",
		  U3PhyReadReg32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x110)));

	mdelay(1);

	/* wait for FM detection done, set 10ms timeout */
	for (i = 0; i < 10; i++) {
		/* => USBPHY base address + 0x10C = FM_OUT */
		/* Read result */
		u4FmOut = U3PhyReadReg32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x10C));
		os_printk(K_DEBUG, "FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

		/* check if FM detection done */
		if (u4FmOut != 0) {
			fgRet = 0;
			os_printk(K_DEBUG, "FM detection done! loop = %d\n", i);
			break;
		}
		fgRet = 1;
		mdelay(1);
	}
	/* => USBPHY base address + 0x100 = 0x00 */
	/* Disable Frequency meter */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x100)
			  , RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0);

	/* => USBPHY base address + 0x110 = 0x00 */
	/* Disable free run clock */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) (u3_sif2_base + 0x110)
			  , RG_FRCK_EN_OFST, RG_FRCK_EN, 0);

	/* RG_USB20_HSTX_SRCTRL[2:0] = (1024/FM_OUT) * reference clock frequency * 0.028 */
	if (u4FmOut == 0) {
		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR5, RG_USB20_HSTX_SRCTRL_OFST,
				  RG_USB20_HSTX_SRCTRL, 0x4);
		fgRet = 1;
	} else {
		/* set reg = (1024/FM_OUT) * REF_CK * U2_SR_COEF_E60802 / 1000 (round to the nearest digits) */
		/* u4Tmp = (((1024 * REF_CK * U2_SR_COEF_E60802) / u4FmOut) + 500) / 1000; */
		u4Tmp = (1024 * REF_CK * U2_SR_COEF_E60802) / (u4FmOut * 1000);
		os_printk(K_DEBUG, "SR calibration value u1SrCalVal = %d\n", (PHY_UINT8) u4Tmp);
		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR5, RG_USB20_HSTX_SRCTRL_OFST,
				  RG_USB20_HSTX_SRCTRL, u4Tmp);
	}

	/* => RG_USB20_HSTX_SRCAL_EN = 0 */
	/* disable USB ring oscillator */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR5, RG_USB20_HSTX_SRCAL_EN_OFST,
			  RG_USB20_HSTX_SRCAL_EN, 0);

	return fgRet;
}

/*This "save current" sequence refers to "6593_USB_PORT0_PWR Sequence 20130729.xls"*/
void usb_phy_savecurrent(unsigned int clk_on)
{
	os_printk(K_INFO, "%s clk_on=%d+\n", __func__, clk_on);

	if (sib_mode) {
		pr_err("%s sib_mode can't savecurrent\n", __func__);
		return;
	}

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		goto reg_done;
#endif

	/*switch to USB function. (system register, force ip into usb mode) */
	/* force_uart_en      1'b0 */
	/* U3D_U2PHYDTM0 FORCE_UART_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	/* RG_UART_EN         1'b0 */
	/* U3D_U2PHYDTM1 RG_UART_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	/* rg_usb20_gpio_ctl  1'b0 */
	/* U3D_U2PHYACR4 RG_USB20_GPIO_CTL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL,
			  0);
	/* usb20_gpio_mode       1'b0 */
	/* U3D_U2PHYACR4 USB20_GPIO_MODE */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);

	/*DP/DM BC1.1 path Disable */
	/* RG_USB20_BC11_SW_EN 1'b0 */
	/* U3D_USBPHYACR6 RG_USB20_BC11_SW_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
			  RG_USB20_BC11_SW_EN, 0);

	/*OTG Disable */
	/* RG_USB20_OTG_VBUSCMP_EN 1b0 */
	/* U3D_USBPHYACR6 RG_USB20_OTG_VBUSCMP_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST,
			  RG_USB20_OTG_VBUSCMP_EN, 0);

	/*let suspendm=1, enable usb 480MHz pll */
	/* RG_SUSPENDM 1'b1 */
	/* U3D_U2PHYDTM0 RG_SUSPENDM */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);

	/*force_suspendm=1 */
	/* force_suspendm        1'b1 */
	/* U3D_U2PHYDTM0 FORCE_SUSPENDM */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);

	/*Wait USBPLL stable. */
	/* Wait 2 ms. */
	udelay(2000);

	/* RG_DPPULLDOWN 1'b1 */
	/* U3D_U2PHYDTM0 RG_DPPULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 1);

	/* RG_DMPULLDOWN 1'b1 */
	/* U3D_U2PHYDTM0 RG_DMPULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 1);

	/* RG_XCVRSEL[1:0] 2'b01 */
	/* U3D_U2PHYDTM0 RG_XCVRSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_XCVRSEL_OFST, RG_XCVRSEL, 0x1);

	/* RG_TERMSEL    1'b1 */
	/* U3D_U2PHYDTM0 RG_TERMSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_TERMSEL_OFST, RG_TERMSEL, 1);

	/* RG_DATAIN[3:0]        4'b0000 */
	/* U3D_U2PHYDTM0 RG_DATAIN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DATAIN_OFST, RG_DATAIN, 0);

	/* force_dp_pulldown     1'b1 */
	/* U3D_U2PHYDTM0 FORCE_DP_PULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN,
			  1);

	/* force_dm_pulldown     1'b1 */
	/* U3D_U2PHYDTM0 FORCE_DM_PULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN,
			  1);

	/* force_xcversel        1'b1 */
	/* U3D_U2PHYDTM0 FORCE_XCVRSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 1);

	/* force_termsel 1'b1 */
	/* U3D_U2PHYDTM0 FORCE_TERMSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 1);

	/* force_datain  1'b1 */
	/* U3D_U2PHYDTM0 FORCE_DATAIN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DATAIN_OFST, FORCE_DATAIN, 1);

	/* wait 800us */
	udelay(800);

	/*let suspendm=0, set utmi into analog power down */
	/* RG_SUSPENDM 1'b0 */
	/* U3D_U2PHYDTM0 RG_SUSPENDM */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 0);

	/* wait 1us */
	udelay(1);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_VBUSVALID_OFST, RG_VBUSVALID, 0);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_AVALID_OFST, RG_AVALID, 0);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_SESSEND_OFST, RG_SESSEND, 1);

	/* USB PLL Force settings */
	usb20_pll_settings(false, false);

#ifdef CONFIG_MTK_UART_USB_SWITCH
reg_done:
#endif
	/* TODO:
	 * Turn off internal 48Mhz PLL if there is no other hardware module is
	 * using the 48Mhz clock -the control register is in clock document
	 * Turn off SSUSB reference clock (26MHz)
	 */
	if (clk_on) {
		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
				RG_SSUSB_VUSB10_ON, 0);

		/* Wait 10 usec. */
		udelay(10);

		/* f_fusb30_ck:125MHz */
		usb_enable_clock(false);

		/* ADA_SSUSB_XTAL_CK:26MHz */
		/*set_ada_ssusb_xtal_ck(0); */

	}

	os_printk(K_INFO, "%s-\n", __func__);
}

/*This "recovery" sequence refers to "6593_USB_PORT0_PWR Sequence 20130729.xls"*/
void usb_phy_recover(unsigned int clk_on)
{
	PHY_INT32 ret;

	os_printk(K_INFO, "%s clk_on=%d+\n", __func__, clk_on);

	if (!clk_on) {
		/*---POWER-----*/
		/*AVDD18_USB_P0 is always turned on. The driver does _NOT_ need to control it. */
		/*hwPowerOn(MT6332_POWER_LDO_VUSB33, VOL_3300, "VDD33_USB_P0"); */
		ret = pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, 0x01);
		if (ret)
			pr_debug("VUSB33 enable FAIL!!!\n");

		VA10_operation(VA10_OP_ON);

		/*---CLOCK-----*/
		/* ADA_SSUSB_XTAL_CK:26MHz */
		/*set_ada_ssusb_xtal_ck(1); */

		/* AD_LTEPLL_SSUSB26M_CK:26MHz always on */
		/* It seems that when turning on ADA_SSUSB_XTAL_CK, AD_LTEPLL_SSUSB26M_CK will also turn on. */
		/* enable_ssusb26m_ck(true); */

		/* f_fusb30_ck:125MHz */
		usb_enable_clock(true);

		/* AD_SSUSB_48M_CK:48MHz */
		/* It seems that when turning on f_fusb30_ck, AD_SSUSB_48M_CK will also turn on. */

		/* Wait 50 usec. (PHY 3.3v & 1.8v power stable time) */
		udelay(250);

		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
				RG_SSUSB_VUSB10_ON, 1);
	}
#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		goto reg_done;
#endif

	/*switch to USB function. (system register, force ip into usb mode) */
	/* force_uart_en      1'b0 */
	/* U3D_U2PHYDTM0 FORCE_UART_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	/* RG_UART_EN         1'b0 */
	/* U3D_U2PHYDTM1 RG_UART_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	/* rg_usb20_gpio_ctl  1'b0 */
	/* U3D_U2PHYACR4 RG_USB20_GPIO_CTL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL,
			  0);
	/* usb20_gpio_mode       1'b0 */
	/* U3D_U2PHYACR4 USB20_GPIO_MODE */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);

	/*Release force suspendm. (force_suspendm=0) (let suspendm=1, enable usb 480MHz pll) */
	/*force_suspendm        1'b0 */
	/* U3D_U2PHYDTM0 FORCE_SUSPENDM */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0);

	/* RG_DPPULLDOWN 1'b0 */
	/* U3D_U2PHYDTM0 RG_DPPULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 0);

	/* RG_DMPULLDOWN 1'b0 */
	/* U3D_U2PHYDTM0 RG_DMPULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 0);

	/* RG_XCVRSEL[1:0]       2'b00 */
	/* U3D_U2PHYDTM0 RG_XCVRSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_XCVRSEL_OFST, RG_XCVRSEL, 0);

	/* RG_TERMSEL    1'b0 */
	/* U3D_U2PHYDTM0 RG_TERMSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_TERMSEL_OFST, RG_TERMSEL, 0);

	/* RG_DATAIN[3:0]        4'b0000 */
	/* U3D_U2PHYDTM0 RG_DATAIN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, RG_DATAIN_OFST, RG_DATAIN, 0);

	/* force_dp_pulldown     1'b0 */
	/* U3D_U2PHYDTM0 FORCE_DP_PULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN,
			  0);

	/* force_dm_pulldown     1'b0 */
	/* U3D_U2PHYDTM0 FORCE_DM_PULLDOWN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN,
			  0);

	/* force_xcversel        1'b0 */
	/* U3D_U2PHYDTM0 FORCE_XCVRSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 0);

	/* force_termsel 1'b0 */
	/* U3D_U2PHYDTM0 FORCE_TERMSEL */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 0);

	/* force_datain  1'b0 */
	/* U3D_U2PHYDTM0 FORCE_DATAIN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM0, FORCE_DATAIN_OFST, FORCE_DATAIN, 0);

	/*DP/DM BC1.1 path Disable */
	/* RG_USB20_BC11_SW_EN   1'b0 */
	/* U3D_USBPHYACR6 RG_USB20_BC11_SW_EN */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
			  RG_USB20_BC11_SW_EN, 0);

	/*OTG Enable */
	/* RG_USB20_OTG_VBUSCMP_EN       1b1 */
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST,
			  RG_USB20_OTG_VBUSCMP_EN, 1);

#if 1 /* FIXME */
	/* fill in U3 related sequence here */
#endif
	/* Wait 800 usec */
	udelay(800);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_VBUSVALID_OFST, RG_VBUSVALID, 1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_AVALID_OFST, RG_AVALID, 1);
	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_U2PHYDTM1, RG_SESSEND_OFST, RG_SESSEND, 0);

	/* EFUSE related sequence */
	{
		u32 evalue;

		/* [4:0] => RG_USB20_INTR_CAL[4:0] */
		evalue = (get_devinfo_with_index(108) & (0x1f<<0)) >> 0;
		if (evalue) {
			os_printk(K_INFO, "apply efuse setting, RG_USB20_INTR_CAL=0x%x\n", evalue);
			U3PhyWriteField32((phys_addr_t) U3D_USBPHYACR1, RG_USB20_INTR_CAL_OFST,
					RG_USB20_INTR_CAL, evalue);
		} else
			os_printk(K_DEBUG, "!evalue\n");

		/* [21:16] (BGR_code) => RG_SSUSB_IEXT_INTR_CTRL[5:0] */
		evalue = (get_devinfo_with_index(107) & (0x3f << 16)) >> 16;
		if (evalue) {
			os_printk(K_INFO, "apply efuse setting, RG_SSUSB_IEXT_INTR_CTRL=0x%x\n", evalue);
			U3PhyWriteField32((phys_addr_t) U3D_USB30_PHYA_REG0, 10,
					(0x3f<<10), evalue);
		} else
			os_printk(K_DEBUG, "!evalue\n");

		/* [12:8] (RX_50_code) => RG_SSUSB_IEXT_RX_IMPSEL[4:0] */
		evalue = (get_devinfo_with_index(107) & (0x1f << 8)) >> 8;
		if (evalue) {
			os_printk(K_INFO, "apply efuse setting, rg_ssusb_rx_impsel=0x%x\n", evalue);
			U3PhyWriteField32((phys_addr_t) U3D_PHYD_IMPCAL1, RG_SSUSB_RX_IMPSEL_OFST,
					RG_SSUSB_RX_IMPSEL, evalue);
		} else
			os_printk(K_DEBUG, "!evalue\n");

		/* [4:0] (TX_50_code) => RG_SSUSB_IEXT_TX_IMPSEL[4:0], don't care : 0-bit */
		evalue = (get_devinfo_with_index(107) & (0x1f << 0)) >> 0;
		if (evalue) {
			os_printk(K_INFO, "apply efuse setting, rg_ssusb_tx_impsel=0x%x\n", evalue);
			U3PhyWriteField32((phys_addr_t) U3D_PHYD_IMPCAL0, RG_SSUSB_TX_IMPSEL_OFST,
					RG_SSUSB_TX_IMPSEL, evalue);

		} else
			os_printk(K_DEBUG, "!evalue\n");
	}

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_DISCTH_OFST, RG_USB20_DISCTH, 0xF);

	U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST,
		RG_USB20_PHY_REV_6, usb20_phy_rev6);

#ifdef MTK_USB_PHY_TUNING
	mtk_usb_phy_tuning();
#endif

	/* USB PLL Force settings */
	usb20_pll_settings(false, false);

	/* special for 10nm */
	{
		u32 val, offset;

		/* 0x11C30B2C[10] to 1, to adjust U3 TX weak-eidle performance */
		val = 0x1;
		offset = 0x2c;
		U3PhyWriteField32((phys_addr_t) (uintptr_t) ((u3_sif2_base + 0xb00) + offset), 10,
				(0x1<<10), val);

		/* 0x11C30B20[3] to 0, U3 RX seting for BAND calibration margin, align calibration setting @FT stage */
		val = 0x0;
		offset = 0x20;
		U3PhyWriteField32((phys_addr_t) (uintptr_t) ((u3_sif2_base + 0xb00) + offset), 3,
				(0x1<<3), val);

	}

#ifdef CONFIG_MTK_UART_USB_SWITCH
reg_done:
#endif
	os_printk(K_INFO, "%s-\n", __func__);
}

/*
 * This function is to solve the condition described below.
 * The system boot has 3 situations.
 * 1. Booting without cable, so connection work called by musb_gadget_start()
 *    would turn off pwr/clk by musb_stop(). [REF CNT = 0]
 * 2. Booting with normal cable, the pwr/clk has already turned on at initial stage.
 *    and also set the flag (musb->is_clk_on=1).
 *    So musb_start() would not turn on again. [REF CNT = 1]
 * 3. Booting with OTG cable, the pwr/clk would be turned on by host one more time.[REF CNT=2]
 *    So device should turn off pwr/clk which are turned on during the initial stage.
 *    However, does _NOT_ touch the PHY registers. So we need this fake function to keep the REF CNT correct.
 *    NOT FOR TURN OFF PWR/CLK.
 */
void usb_fake_powerdown(unsigned int clk_on)
{
	os_printk(K_INFO, "%s clk_on=%d+\n", __func__, clk_on);

	if (clk_on) {
		/*---CLOCK-----*/
		/* f_fusb30_ck:125MHz */
		usb_enable_clock(false);
	}

	os_printk(K_INFO, "%s-\n", __func__);
}

#ifdef CONFIG_USBIF_COMPLIANCE
static bool charger_det_en = true;

void Charger_Detect_En(bool enable)
{
	charger_det_en = enable;
}
#endif


/* BC1.2 */
void Charger_Detect_Init(void)
{
	os_printk(K_INFO, "%s+\n", __func__);

	if (mu3d_force_on) {
		os_printk(K_INFO, "%s-, SKIP\n", __func__);
		return;
	}

#ifdef CONFIG_USBIF_COMPLIANCE
	if (charger_det_en == true) {
#endif
		/* turn on USB reference clock. */
		usb_enable_clock(true);

		/* wait 50 usec. */
		udelay(250);

		/* RG_USB20_BC11_SW_EN = 1'b1 */
		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
				  RG_USB20_BC11_SW_EN, 1);

		udelay(1);

		/* 4 14. turn off internal 48Mhz PLL. */
		usb_enable_clock(false);

#ifdef CONFIG_USBIF_COMPLIANCE
	} else {
		os_printk(K_INFO, "%s do not init detection as charger_det_en is false\n",
			  __func__);
	}
#endif

	os_printk(K_INFO, "%s-\n", __func__);
}

void Charger_Detect_Release(void)
{
	os_printk(K_INFO, "%s+\n", __func__);

	if (mu3d_force_on) {
		os_printk(K_INFO, "%s-, SKIP\n", __func__);
		return;
	}

#ifdef CONFIG_USBIF_COMPLIANCE
	if (charger_det_en == true) {
#endif
		/* turn on USB reference clock. */
		usb_enable_clock(true);

		/* wait 50 usec. */
		udelay(250);

		/* RG_USB20_BC11_SW_EN = 1'b0 */
		U3PhyWriteField32((phys_addr_t) (uintptr_t) U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST,
				  RG_USB20_BC11_SW_EN, 0);

		udelay(1);

		/* 4 14. turn off internal 48Mhz PLL. */
		usb_enable_clock(false);

#ifdef CONFIG_USBIF_COMPLIANCE
	} else {
		os_printk(K_DEBUG, "%s do not release detection as charger_det_en is false\n",
			  __func__);
	}
#endif

	os_printk(K_INFO, "%s-\n", __func__);
}



#ifdef CONFIG_OF
static int mt_usb_dts_probe(struct platform_device *pdev)
{
	int retval = 0;
#ifndef CONFIG_MTK_CLKMGR
	struct clk *clk_tmp;

	clk_tmp = devm_clk_get(&pdev->dev, "ssusb_clk");
	if (IS_ERR(clk_tmp)) {
		pr_err("clk_tmp get ssusb_clk fail\n");
		return PTR_ERR(clk_tmp);
	}

	ssusb_clk = clk_tmp;

	retval = clk_prepare(ssusb_clk);
	if (retval == 0)
		pr_debug("ssusb_clk<%p> prepare done\n", ssusb_clk);
	else
		pr_err("ssusb_clk prepare fail\n");

	if (mt_get_chip_sw_ver() >= CHIP_SW_VER_02) {
		clk_tmp = devm_clk_get(&pdev->dev, "ssusb_clk_sck");
		if (IS_ERR(clk_tmp)) {
			pr_err("clk_tmp get ssusb_clk_sck fail\n");
			return PTR_ERR(clk_tmp);
		}

		ssusb_clk_sck = clk_tmp;

		retval = clk_prepare(ssusb_clk_sck);
		if (retval == 0)
			pr_debug("ssusb_clk_sck<%p> prepare done\n", ssusb_clk_sck);
		else
			pr_err("ssusb_clk_sck prepare fail\n");
	}
#endif

	if (mt_get_chip_sw_ver() >= CHIP_SW_VER_02)
		usb20_phy_rev6 = 1;
	else
		usb20_phy_rev6 = 0;
	pr_warn("%s, usb20_phy_rev6 to %d\n", __func__, usb20_phy_rev6);

	return retval;
}

static int mt_usb_dts_remove(struct platform_device *pdev)
{
#ifndef CONFIG_MTK_CLKMGR
	if ((mt_get_chip_sw_ver() >= CHIP_SW_VER_02) && !IS_ERR(ssusb_clk_sck))
		clk_unprepare(ssusb_clk_sck);
	if (!IS_ERR(ssusb_clk))
		clk_unprepare(ssusb_clk);
#endif
	return 0;
}


static const struct of_device_id apusb_of_ids[] = {
	{.compatible = "mediatek,usb3_phy",},
	{},
};

MODULE_DEVICE_TABLE(of, apusb_of_ids);

static struct platform_driver mt_usb_dts_driver = {
	.remove = mt_usb_dts_remove,
	.probe = mt_usb_dts_probe,
	.driver = {
		   .name = "mt_dts_mu3phy",
		   .of_match_table = apusb_of_ids,
		   },
};
MODULE_DESCRIPTION("mtu3phy MUSB PHY Layer");
MODULE_AUTHOR("MediaTek");
MODULE_LICENSE("GPL v2");
module_platform_driver(mt_usb_dts_driver);

void enable_ipsleep_wakeup(void)
{
	/* TODO */
}
void disable_ipsleep_wakeup(void)
{
	/* TODO */
}
#endif

#endif

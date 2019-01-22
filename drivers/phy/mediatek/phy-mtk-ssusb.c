/*
 * Copyright (C) 2017 MediaTek Inc.
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


#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/phy/phy.h>
#include <linux/delay.h>

#include "phy-mtk.h"
#include "phy-mtk-ssusb-reg.h"
#include <mtk-ssusb-hal.h>

static DEFINE_SPINLOCK(mu3phy_clock_lock);

#define MTK_USB_PHY_BASE	(phy_drv->phy_base)
#define MTK_USB_PHY_PORT_BASE	(instance->port_base)

/*
 *#define MTK_USB_IPPC_BASE	(phy_drv->ippc_base)
 *#define U3D_SSUSB_IP_PW_CTRL0		(MTK_USB_IPPC_BASE + 0x0000)
 *#define U3D_SSUSB_IP_PW_CTRL1		(MTK_USB_IPPC_BASE + 0x0004)
 *#define U3D_SSUSB_IP_PW_CTRL2		(MTK_USB_IPPC_BASE + 0x0008)
 *#define U3D_SSUSB_IP_PW_CTRL3		(MTK_USB_IPPC_BASE + 0x000C)
 *#define U3D_SSUSB_U3_CTRL_0P		(MTK_USB_IPPC_BASE + 0x0030)
 */

#ifdef CONFIG_MTK_UART_USB_SWITCH
#define AP_UART0_COMPATIBLE_NAME "mediatek,gpio"
#endif

static bool usb_enable_clock(struct mtk_phy_drv *u3phy, bool enable)
{
	static int count;
	unsigned long flags;

	spin_lock_irqsave(&mu3phy_clock_lock, flags);
	phy_printk(K_INFO, "CG, enable<%d>, count<%d>\n", enable, count);

	if (enable && count == 0) {
		usb_hal_dpidle_request(USB_DPIDLE_FORBIDDEN);
		if (clk_enable(u3phy->clk) != 0)
			phy_printk(K_ERR, "ssusb_clk enable fail\n");


	} else if (!enable && count == 1) {
		clk_disable(u3phy->clk);
		usb_hal_dpidle_request(USB_DPIDLE_ALLOWED);
	}

	if (enable)
		count++;
	else
		count = (count == 0) ? 0 : (count - 1);

	spin_unlock_irqrestore(&mu3phy_clock_lock, flags);

	return 0;
}

void u3phywrite32(void __iomem *addr, int offset, int mask, int value)
{
	int cur_value;
	int new_value;

	cur_value = readl(addr);
	new_value = (cur_value & (~mask)) | ((value << offset) & mask);
	writel(new_value, addr);
}

int u3phyread32(void __iomem *addr)
{
	return readl(addr);
}

#ifdef CONFIG_U3_PHY_SMT_LOOP_BACK_SUPPORT
bool phy_u3_loop_back_test(struct mtk_phy_instance *instance)
{
	int reg;
	bool loop_back_ret = false;
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	/* VA10 is shared by U3/UFS */
	/* default on and set voltage by PMIC */
	/* off/on in SPM suspend/resume */

	usb_enable_clock(phy_drv, true);

#if 0
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
#endif
	writel((readl(U3D_PHYD_PIPE0) & ~(0x01<<30)) | 0x01<<30,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x01<<28)) | 0x00<<28,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x03<<26)) | 0x01<<26,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x03<<24)) | 0x00<<24,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x01<<22)) | 0x00<<22,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x01<<21)) | 0x00<<21,
							U3D_PHYD_PIPE0);
	writel((readl(U3D_PHYD_PIPE0) & ~(0x01<<20)) | 0x01<<20,
							U3D_PHYD_PIPE0);
	mdelay(10);

	/*T2R loop back disable*/
	writel((readl(U3D_PHYD_RX0)&~(0x01<<15)) | 0x00<<15,
							U3D_PHYD_RX0);
	mdelay(10);

	/* TSEQ lock detect threshold */
	writel((readl(U3D_PHYD_MIX0) & ~(0x07<<24)) | 0x07<<24,
							U3D_PHYD_MIX0);
	/* set default TSEQ polarity check value = 1 */
	writel((readl(U3D_PHYD_MIX0) & ~(0x01<<28)) | 0x01<<28,
							U3D_PHYD_MIX0);
	/* TSEQ polarity check enable */
	writel((readl(U3D_PHYD_MIX0) & ~(0x01<<29)) | 0x01<<29,
							U3D_PHYD_MIX0);
	/* TSEQ decoder enable */
	writel((readl(U3D_PHYD_MIX0) & ~(0x01<<30)) | 0x01<<30,
							U3D_PHYD_MIX0);
	mdelay(10);

	/* set T2R loop back TSEQ length (x 16us) */
	writel((readl(U3D_PHYD_T2RLB) & ~(0xff<<0)) | 0xF0<<0,
							U3D_PHYD_T2RLB);
	/* set T2R loop back BDAT reset period (x 16us) */
	writel((readl(U3D_PHYD_T2RLB) & ~(0x0f<<12)) | 0x0F<<12,
							U3D_PHYD_T2RLB);
	/* T2R loop back pattern select */
	writel((readl(U3D_PHYD_T2RLB) & ~(0x03<<8)) | 0x00<<8,
							U3D_PHYD_T2RLB);
	mdelay(10);

	/* T2R loop back serial mode */
	writel((readl(U3D_PHYD_RX0) & ~(0x01<<13)) | 0x01<<13,
							U3D_PHYD_RX0);
	/* T2R loop back parallel mode = 0 */
	writel((readl(U3D_PHYD_RX0) & ~(0x01<<12)) | 0x00<<12,
							U3D_PHYD_RX0);
	/* T2R loop back mode enable */
	writel((readl(U3D_PHYD_RX0) & ~(0x01<<11)) | 0x01<<11,
							U3D_PHYD_RX0);
	/* T2R loop back enable */
	writel((readl(U3D_PHYD_RX0) & ~(0x01<<15)) | 0x01<<15,
							U3D_PHYD_RX0);
	mdelay(100);

	reg = u3phyread32(SSUSB_SIFSLV_U3PHYD_BASE + 0xb4);
	phy_printk(K_INFO, "read back             : 0x%x\n", reg);
	phy_printk(K_INFO, "read back t2rlb_lock  : %d\n", (reg >> 2) & 0x01);
	phy_printk(K_INFO, "read back t2rlb_pass  : %d\n", (reg >> 3) & 0x01);
	phy_printk(K_INFO, "read back t2rlb_passth: %d\n", (reg >> 4) & 0x01);

	if ((reg & 0x0E) == 0x0E)
		loop_back_ret = true;
	else
		loop_back_ret = false;

	usb_enable_clock(phy_drv, false);
	return loop_back_ret;
}
#endif

#ifdef CONFIG_MTK_SIB_USB_SWITCH
static void phy_sib_enable(struct mtk_phy_instance *instance, bool enable)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	phy_printk(K_DEBUG, "usb_phy_sib_enable_switch enable =%d\n", enable);

	/*
	 * It's MD debug usage. No need to care low power.
	 * Thus, no power off BULK and Clock at the end of function.
	 * MD SIB still needs these power and clock source.
	 */
	/* VA10 is shared by U3/UFS */
	/* default on and set voltage by PMIC */
	/* off/on in SPM suspend/resume */

#if 0
	usb_enable_clock(phy_drv, true);
	udelay(250);

	u3phywrite32(U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST,
			  RG_SSUSB_VUSB10_ON, 1);


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
#endif

	/*
	 * USBMAC mode is 0x62910002 (bit 1)
	 * MDSIB  mode is 0x62910008 (bit 3)
	 * 0x0629 just likes a signature. Can't be removed.
	 */
	if (enable) {
		writel(0x62910008, (phy_drv->phy_base + 0x300));
		instance->sib_mode = true;
	} else {
		writel(0x62910002, (phy_drv->phy_base + 0x300));
		instance->sib_mode = false;
	}
}

#endif

static int phy_slew_rate_calibration(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;
	int i = 0;
	int fgRet = 0;
	int u4FmOut = 0;
	int u4Tmp = 0;

	phy_printk(K_DEBUG, "%s\n", __func__);

	/* enable USB ring oscillator */
	u3phywrite32(U3D_USBPHYACR5, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 1);

	/* wait 1us */
	udelay(1);

	/* Enable free run clock */
	u3phywrite32(RG_SSUSB_SIFSLV_FMMONR1, RG_FRCK_EN_OFST, RG_FRCK_EN, 0x1);
	/* Setting cyclecnt */
	u3phywrite32(RG_SSUSB_SIFSLV_FMCR0, RG_CYCLECNT_OFST, RG_CYCLECNT, 0x400);
	/* Enable frequency meter */
	u3phywrite32(RG_SSUSB_SIFSLV_FMCR0, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0x1);
	phy_printk(K_DEBUG, "Freq_Valid=(0x%08X)\n", u3phyread32(RG_SSUSB_SIFSLV_FMMONR1));

	mdelay(1);

	/* wait for FM detection done, set 10ms timeout */
	for (i = 0; i < 10; i++) {
		u4FmOut = u3phyread32(RG_SSUSB_SIFSLV_FMMONR0);
		phy_printk(K_DEBUG, "FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);
		if (u4FmOut != 0) {
			fgRet = 0;
			phy_printk(K_DEBUG, "FM detection done! loop = %d\n", i);
			break;
		}
		fgRet = 1;
		mdelay(1);
	}
	u3phywrite32(RG_SSUSB_SIFSLV_FMCR0, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0);
	u3phywrite32(RG_SSUSB_SIFSLV_FMMONR1, RG_FRCK_EN_OFST, RG_FRCK_EN, 0);

	if (u4FmOut == 0) {
		u3phywrite32(U3D_USBPHYACR5, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, 0x4);
		fgRet = 1;
	} else {
		u4Tmp = (1024 * U3D_PHY_REF_CK * U2_SR_COEF) / (u4FmOut * 1000);
		phy_printk(K_DEBUG, "SR calibration value u1SrCalVal = %d\n", u4Tmp);
		u3phywrite32(U3D_USBPHYACR5, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, u4Tmp);
	}

	u3phywrite32(U3D_USBPHYACR5, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0);

	return fgRet;
}

static int phy_init_soc(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;
	struct device_node *of_node = instance->phy->dev.of_node;

	phy_printk(K_DEBUG, "%s\n", __func__);
	usb_enable_clock(phy_drv, true);
	udelay(250);
	u3phywrite32(U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST, RG_SSUSB_VUSB10_ON, 1);

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (instance->uart_mode)
		goto reg_done;
#endif

	/*switch to USB function. (system register, force ip into usb mode) */
	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL, 0);
	u3phywrite32(U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_UART_MODE_OFST, RG_UART_MODE, 0);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_DP_100K_MODE_OFST, RG_USB20_DP_100K_MODE, 1);
	u3phywrite32(U3D_U2PHYACR4, USB20_DP_100K_EN_OFST, USB20_DP_100K_EN, 0);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_DM_100K_EN_OFST, RG_USB20_DM_100K_EN, 0);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0);
	udelay(800);
	u3phywrite32(U3D_U2PHYDTM1, FORCE_VBUSVALID_OFST, FORCE_VBUSVALID, 1);
	u3phywrite32(U3D_U2PHYDTM1, FORCE_AVALID_OFST, FORCE_AVALID, 1);
	u3phywrite32(U3D_U2PHYDTM1, FORCE_SESSEND_OFST, FORCE_SESSEND, 1);

	/*u3phywrite32((SSUSB_USB30_PHYA_SIV_B_BASE + 0x2C), 10, (0x1<<10), 1);*/
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST, RG_USB20_PHY_REV_6, 0x1);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_DISCTH_OFST, RG_USB20_DISCTH, 0xF);

	if (of_device_is_compatible(of_node, "mediatek,mt6758-phy")) {
		u3phywrite32(U3D_SPLLC_XTALCTL3, RG_SSUSB_XTAL_RX_PWD_OFST,
				RG_SSUSB_XTAL_RX_PWD, 1);
		u3phywrite32(U3D_SPLLC_XTALCTL3, RG_SSUSB_FRC_XTAL_RX_PWD_OFST,
				RG_SSUSB_FRC_XTAL_RX_PWD, 1);
		u3phywrite32(U3D_U3PHYA_DA_REG36, RG_SSUSB_DA_SSUSB_PLL_BAND_OFST,
				RG_SSUSB_DA_SSUSB_PLL_BAND, 0x2D);
		u3phywrite32(U3D_B2_PHYD_RXDET2, RG_SSUSB_RXDET_STB2_SET_P3_OFST,
				RG_SSUSB_RXDET_STB2_SET_P3, 0x10);
		u3phywrite32(U3D_U3PHYA_DA_REG32, RG_SSUSB_LFPS_DEGLITCH_U3_OFST,
			RG_SSUSB_LFPS_DEGLITCH_U3, 1);
	} else if (of_device_is_compatible(of_node, "mediatek,mt6799-phy")) {
		/* 0x11C30B2C[10] to 1, to adjust U3 TX weak-eidle performance */
		u3phywrite32(U3D_USB30_PHYA_REGB, RG_SSUSB_RESERVE10_OFST,
				RG_SSUSB_RESERVE10, 0x1);
		/* 0x11C30B20[3] to 0, U3 RX seting for BAND calibration margin, align calibration setting @FT stage */
		u3phywrite32(U3D_USB30_PHYA_REG8, RG_SSUSB_CDR_BYPASS_OFST,
				RG_SSUSB_CDR_BYPASS, 0x2);
		/* 0x11C30958[31:28], rg_ssusb_cdr_bic_ltr[3:0] from 4'b1111 to 4'b0100, analog circuit performance */
		u3phywrite32(U3D_PHYD_CDR0, RG_SSUSB_CDR_BIC_LTR_OFST,
				RG_SSUSB_CDR_BIC_LTR, 0x4);
	}

/* EFUSE related sequence */
	if (of_device_is_compatible(of_node, "mediatek,mt6758-phy") ||
		of_device_is_compatible(of_node, "mediatek,mt6799-phy")) {
		u32 evalue;

		evalue = (get_devinfo_with_index(108) & (0x1f<<0)) >> 0;
		if (evalue) {
			phy_printk(K_INFO, "apply efuse setting, RG_USB20_INTR_CAL=0x%x\n", evalue);
			u3phywrite32(U3D_USBPHYACR1, RG_USB20_INTR_CAL_OFST,
					RG_USB20_INTR_CAL, evalue);
		}
		evalue = (get_devinfo_with_index(107) & (0x3f << 16)) >> 16;
		if (evalue) {
			phy_printk(K_INFO, "apply efuse setting, RG_SSUSB_IEXT_INTR_CTRL=0x%x\n", evalue);
			u3phywrite32(U3D_USB30_PHYA_REG0, RG_SSUSB_IEXT_INTR_CTRL_OFST,
					RG_SSUSB_IEXT_INTR_CTRL, evalue);
		}
		evalue = (get_devinfo_with_index(107) & (0x1f << 8)) >> 8;
		if (evalue) {
			phy_printk(K_INFO, "apply efuse setting, rg_ssusb_rx_impsel=0x%x\n", evalue);
			u3phywrite32(U3D_PHYD_IMPCAL1, RG_SSUSB_RX_IMPSEL_OFST,
					RG_SSUSB_RX_IMPSEL, evalue);
		}
		evalue = (get_devinfo_with_index(107) & (0x1f << 0)) >> 0;
		if (evalue) {
			phy_printk(K_INFO, "apply efuse setting, rg_ssusb_tx_impsel=0x%x\n", evalue);
			u3phywrite32(U3D_PHYD_IMPCAL0, RG_SSUSB_TX_IMPSEL_OFST,
					RG_SSUSB_TX_IMPSEL, evalue);
		}
	}

#ifdef CONFIG_MTK_UART_USB_SWITCH
reg_done:
#endif
	usb_enable_clock(phy_drv, false);

	return 0;
}


static void phy_savecurrent(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	phy_printk(K_DEBUG, "%s\n", __func__);
	if (instance->sib_mode) {
		phy_printk(K_INFO, "%s sib_mode can't savecurrent\n", __func__);
		return;
	}

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (instance->uart_mode)
		goto reg_done;
#endif

	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL, 0);
	u3phywrite32(U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);
	/*u3phywrite32(U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 0);*/
	u3phywrite32(U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);
	udelay(2000);
	u3phywrite32(U3D_U2PHYDTM0, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 1);
	u3phywrite32(U3D_U2PHYDTM0, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 1);
	u3phywrite32(U3D_U2PHYDTM0, RG_XCVRSEL_OFST, RG_XCVRSEL, 0x1);
	u3phywrite32(U3D_U2PHYDTM0, RG_TERMSEL_OFST, RG_TERMSEL, 1);
	u3phywrite32(U3D_U2PHYDTM0, RG_DATAIN_OFST, RG_DATAIN, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DATAIN_OFST, FORCE_DATAIN, 1);

	udelay(800);

	u3phywrite32(U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 0);

	udelay(1);

	u3phywrite32(U3D_U2PHYDTM1, RG_VBUSVALID_OFST, RG_VBUSVALID, 0);
	u3phywrite32(U3D_U2PHYDTM1, RG_AVALID_OFST, RG_AVALID, 0);
	u3phywrite32(U3D_U2PHYDTM1, RG_SESSEND_OFST, RG_SESSEND, 1);

#ifdef CONFIG_MTK_UART_USB_SWITCH
reg_done:
#endif
	u3phywrite32(U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST, RG_SSUSB_VUSB10_ON, 1);
	udelay(10);
	usb_enable_clock(phy_drv, false);
}

#define VAL_MAX_WIDTH_2	0x3
#define VAL_MAX_WIDTH_3	0x7
void usb_phy_tuning(struct mtk_phy_instance *instance)
{
	s32 u2_vrt_ref, u2_term_ref, u2_enhance;
	struct device_node *of_node;

	if (!instance->phy_tuning.inited) {
		instance->phy_tuning.u2_vrt_ref = -1;
		instance->phy_tuning.u2_term_ref = -1;
		instance->phy_tuning.u2_enhance = -1;
		of_node = of_find_compatible_node(NULL, NULL, instance->phycfg->tuning_node_name);
		if (of_node) {
			/* value won't be updated if property not being found */
			of_property_read_u32(of_node, "u2_vrt_ref", (u32 *) &instance->phy_tuning.u2_vrt_ref);
			of_property_read_u32(of_node, "u2_term_ref", (u32 *) &instance->phy_tuning.u2_term_ref);
			of_property_read_u32(of_node, "u2_enhance", (u32 *) &instance->phy_tuning.u2_enhance);
		}
		instance->phy_tuning.inited = true;
	}
	u2_vrt_ref = instance->phy_tuning.u2_vrt_ref;
	u2_term_ref = instance->phy_tuning.u2_term_ref;
	u2_enhance = instance->phy_tuning.u2_enhance;

	if (u2_vrt_ref != -1) {
		if (u2_vrt_ref <= VAL_MAX_WIDTH_3) {
			u3phywrite32(U3D_USBPHYACR1, RG_USB20_VRT_VREF_SEL_OFST,
				RG_USB20_VRT_VREF_SEL, u2_vrt_ref);
		}
	}
	if (u2_term_ref != -1) {
		if (u2_term_ref <= VAL_MAX_WIDTH_3) {
			u3phywrite32(U3D_USBPHYACR1, RG_USB20_TERM_VREF_SEL_OFST,
				RG_USB20_TERM_VREF_SEL, u2_term_ref);
		}
	}
	if (u2_enhance != -1) {
		if (u2_enhance <= VAL_MAX_WIDTH_2) {
			u3phywrite32(U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST,
				RG_USB20_PHY_REV_6, u2_enhance);
		}
	}
}


static void phy_recover(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	phy_printk(K_DEBUG, "%s\n", __func__);
	usb_enable_clock(phy_drv, true);
	udelay(250);
	u3phywrite32(U3D_USB30_PHYA_REG1, RG_SSUSB_VUSB10_ON_OFST, RG_SSUSB_VUSB10_ON, 1);

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (instance->uart_mode)
		return;
#endif

	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 0);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_GPIO_CTL_OFST, RG_USB20_GPIO_CTL, 0);
	u3phywrite32(U3D_U2PHYACR4, USB20_GPIO_MODE_OFST, USB20_GPIO_MODE, 0);

	u3phywrite32(U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_DPPULLDOWN_OFST, RG_DPPULLDOWN, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_DMPULLDOWN_OFST, RG_DMPULLDOWN, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_XCVRSEL_OFST, RG_XCVRSEL, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_TERMSEL_OFST, RG_TERMSEL, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_DATAIN_OFST, RG_DATAIN, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DP_PULLDOWN_OFST, FORCE_DP_PULLDOWN, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DM_PULLDOWN_OFST, FORCE_DM_PULLDOWN, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_XCVRSEL_OFST, FORCE_XCVRSEL, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_TERMSEL_OFST, FORCE_TERMSEL, 0);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_DATAIN_OFST, FORCE_DATAIN, 0);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);
	/*u3phywrite32(U3D_USBPHYACR6, RG_USB20_OTG_VBUSCMP_EN_OFST, RG_USB20_OTG_VBUSCMP_EN, 1);*/

	/* Wait 800 usec */
	udelay(800);

	u3phywrite32(U3D_U2PHYDTM1, RG_VBUSVALID_OFST, RG_VBUSVALID, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_AVALID_OFST, RG_AVALID, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_SESSEND_OFST, RG_SESSEND, 0);

	phy_slew_rate_calibration(instance);
	usb_phy_tuning(instance);
}


static int charger_detect_init(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;


	phy_printk(K_DEBUG, "%s+\n", __func__);

	usb_enable_clock(phy_drv, true);
	udelay(250);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 1);
	udelay(1);
	usb_enable_clock(phy_drv, false);

	return 0;
}

static int charger_detect_release(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	phy_printk(K_DEBUG, "%s+\n", __func__);

	usb_enable_clock(phy_drv, true);
	udelay(250);
	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);
	udelay(1);
	usb_enable_clock(phy_drv, false);

	return 0;
}

static void phy_charger_switch_bc11(struct mtk_phy_instance *instance, bool on)
{
	if (on)
		charger_detect_init(instance);
	else
		charger_detect_release(instance);
}

static int phy_lpm_enable(struct mtk_phy_instance  *instance, bool on)
{
	phy_printk(K_DEBUG, "%s+ = %d\n", __func__, on);

	if (on)
		u3phywrite32(U3D_U2PHYDCR1, RG_USB20_SW_PLLMODE_OFST, RG_USB20_SW_PLLMODE, 0x1);
	else
		u3phywrite32(U3D_U2PHYDCR1, RG_USB20_SW_PLLMODE_OFST, RG_USB20_SW_PLLMODE, 0x0);

	return 0;
}

static int phy_host_mode(struct mtk_phy_instance  *instance, bool on)
{
	phy_printk(K_DEBUG, "%s+ = %d\n", __func__, on);

	if (on) {
		u3phywrite32(U3D_U2PHYACR4, RG_USB20_TX_BIAS_EN_OFST, RG_USB20_TX_BIAS_EN, 0x1);
		u3phywrite32(U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST, RG_USB20_PHY_REV_6, 0x1);
	} else {
		u3phywrite32(U3D_U2PHYACR4, RG_USB20_TX_BIAS_EN_OFST, RG_USB20_TX_BIAS_EN, 0x0);
		u3phywrite32(U3D_USBPHYACR6, RG_USB20_PHY_REV_6_OFST, RG_USB20_PHY_REV_6, 0x0);
	}

	return 0;
}

static int phy_ioread(struct mtk_phy_instance  *instance, int mode, u32 reg)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	if ((mode == 0) && reg < 0x800)
		return readl(phy_drv->phy_base + reg);
	else if ((mode == 1) && reg < 0x800)
		return readl(instance->port_base + reg);
	else
		return 0;
}

static int phy_iowrite(struct mtk_phy_instance  *instance, int mode, u32 val, u32 reg)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	if ((mode == 0) && reg < 0x800)
		writel(val, phy_drv->phy_base + reg);
	else if ((mode == 1) && reg < 0x800)
		writel(val, instance->port_base + reg);

	return 0;
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
static bool phy_check_in_uart_mode(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;
	int usb_port_mode;

	usb_enable_clock(phy_drv, true);
	udelay(250);

	usb_port_mode = u3phyread32(U3D_U2PHYDTM0) >> RG_UART_MODE_OFST;

	usb_enable_clock(phy_drv, false);
	phy_printk(K_DEBUG, "usb_port_mode = %d\n", usb_port_mode);

	if (usb_port_mode == 0x1)
		return true;
	else
		return false;
}

static void phy_switch_to_uart(struct mtk_phy_instance *instance)
{
	struct mtk_phy_drv *phy_drv = instance->phy_drv;

	if (phy_check_in_uart_mode(instance)) {
		phy_printk(K_DEBUG, "%s+ UART_MODE\n", __func__);
		return;
	}

	phy_printk(K_DEBUG, "%s+ USB_MODE\n", __func__);

	usb_enable_clock(phy_drv, true);
	udelay(250);

	u3phywrite32(U3D_USBPHYACR6, RG_USB20_BC11_SW_EN_OFST, RG_USB20_BC11_SW_EN, 0);
	u3phywrite32(U3D_U2PHYDTM0, RG_SUSPENDM_OFST, RG_SUSPENDM, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_SUSPENDM_OFST, FORCE_SUSPENDM, 1);
	u3phywrite32(U3D_U2PHYDTM0, RG_UART_MODE_OFST, RG_UART_MODE, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_TX_OE_OFST, FORCE_UART_TX_OE, 1);
	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_BIAS_EN_OFST, FORCE_UART_BIAS_EN, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_EN_OFST, RG_UART_EN, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_TX_OE_OFST, RG_UART_TX_OE, 1);
	u3phywrite32(U3D_U2PHYDTM1, RG_UART_BIAS_EN_OFST, RG_UART_BIAS_EN, 1);
	u3phywrite32(U3D_U2PHYACR4, RG_USB20_DM_100K_EN_OFST, RG_USB20_DM_100K_EN, 1);

	usb_enable_clock(phy_drv, false);

	instance->uart_mode = true;
}


static void phy_switch_to_usb(struct mtk_phy_instance *instance)
{
	instance->uart_mode = false;

	u3phywrite32(U3D_U2PHYDTM0, FORCE_UART_EN_OFST, FORCE_UART_EN, 0);

	phy_init_soc(instance);
}
#endif

int mtk_phy_drv_init(struct platform_device *pdev, struct mtk_phy_drv *mtkphy)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	mtkphy->phy_base = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(mtkphy->phy_base)) {
		dev_err(dev, "failed to remap phy regs\n");
		return PTR_ERR(mtkphy->phy_base);
	}

#if 0
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res)
		return -EINVAL;

	mtkphy->ippc_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!mtkphy->ippc_base) {
		dev_err(dev, "failed to remap ippc_base regs\n");
		return PTR_ERR(mtkphy->ippc_base);
	}
#endif

	mtkphy->clk = devm_clk_get(dev, "ssusb_clk");
	if (IS_ERR(mtkphy->clk)) {
		dev_err(dev, "failed to get usb_hs_clk\n");
		return PTR_ERR(mtkphy->clk);
	}

	clk_prepare(mtkphy->clk);

	mtkphy->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(mtkphy->vusb33)) {
		dev_err(dev, "cannot get vusb33\n");
		ret = PTR_ERR(mtkphy->vusb33);
		goto disable_clks;
	}

	mtkphy->vusb10 = devm_regulator_get(dev, "va10");
	if (IS_ERR(mtkphy->vusb10)) {
		dev_err(dev, "cannot get vusb10\n");
		ret = PTR_ERR(mtkphy->vusb10);
		goto disable_clks;
	}

	ret = regulator_enable(mtkphy->vusb33);
	if (ret) {
		dev_err(dev, "Failed to enable VUSB33\n");
		goto disable_clks;
	}

	ret = regulator_enable(mtkphy->vusb10);
	if (ret) {
		dev_err(dev, "Failed to enable VUSB10\n");
		goto disable_clks;
	}

	return ret;

disable_clks:
	clk_unprepare(mtkphy->clk);
	return ret;
}

int mtk_phy_drv_exit(struct platform_device *pdev, struct mtk_phy_drv *mtkphy)
{
	if (!IS_ERR_OR_NULL(mtkphy->clk))
		clk_unprepare(mtkphy->clk);

	return 0;
}


static const struct mtk_phy_interface ssusb_phys[] = {
{
	.name		= "port0",
	.tuning_node_name = "mediatek,phy_tuning",
	.port_num	= 0,
	.reg_offset = 0x800,
	.usb_phy_init = phy_init_soc,
	.usb_phy_savecurrent = phy_savecurrent,
	.usb_phy_recover  = phy_recover,
	.usb_phy_switch_to_bc11 = phy_charger_switch_bc11,
	.usb_phy_lpm_enable = phy_lpm_enable,
	.usb_phy_host_mode = phy_host_mode,
	.usb_phy_io_read = phy_ioread,
	.usb_phy_io_write = phy_iowrite,
#ifdef CONFIG_MTK_UART_USB_SWITCH
	.usb_phy_switch_to_usb = phy_switch_to_usb,
	.usb_phy_switch_to_uart = phy_switch_to_uart,
	.usb_phy_check_in_uart_mode = phy_check_in_uart_mode,
#endif
#ifdef CONFIG_MTK_SIB_USB_SWITCH
	.usb_phy_sib_enable_switch = phy_sib_enable,
#endif
#ifdef CONFIG_U3_PHY_SMT_LOOP_BACK_SUPPORT
	.usb_phy_u3_loop_back_test = phy_u3_loop_back_test,
#endif
	},
};

const struct mtk_usbphy_config ssusb_phy_config = {
	.phys			= ssusb_phys,
	.num_phys		= 1,
	.usb_drv_init = mtk_phy_drv_init,
	.usb_drv_exit = mtk_phy_drv_exit,
};


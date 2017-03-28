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

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include "mtk_musb.h"
#include "musb_core.h"
#include "usb20.h"

#define FRA (48)
#define PARA (28)

static void HQA_special(void)
{
	u8 val;

	val = USBPHY_READ8(0x18);
	DBG(0, "HQA, 0x18, before:%x\n", val);
	USBPHY_CLR8(0x18, 0x08);
	USBPHY_SET8(0x18, 0x06);
	val = USBPHY_READ8(0x18);
	DBG(0, "HQA, 0x18, after:%x\n", val);
}

#ifdef FPGA_PLATFORM
bool usb_enable_clock(bool enable)
{
	return true;
}

void usb_phy_poweron(void)
{
}

void usb_phy_savecurrent(void)
{
}

void usb_phy_recover(void)
{
}

/* BC1.2 */
void Charger_Detect_Init(void)
{
}

void Charger_Detect_Release(void)
{
}

void usb_phy_context_save(void)
{
}

void usb_phy_context_restore(void)
{
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);

	usb_port_mode = USB_PHY_Read_Register8(0x6B);
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) || (usb_port_mode == 0x5E))
		return true;
	else
		return false;
}

void usb_phy_switch_to_uart(void)
{
	int var;
#if 0
	/* SW disconnect */
	var = USB_PHY_Read_Register8(0x68);
	DBG(0, "[MUSB]addr: 0x68, value: %x\n", var);
	USB_PHY_Write_Register8(0x15, 0x68);
	DBG(0, "[MUSB]addr: 0x68, value after: %x\n", USB_PHY_Read_Register8(0x68));

	var = USB_PHY_Read_Register8(0x6A);
	DBG(0, "[MUSB]addr: 0x6A, value: %x\n", var);
	USB_PHY_Write_Register8(0x0, 0x6A);
	DBG(0, "[MUSB]addr: 0x6A, value after: %x\n", USB_PHY_Read_Register8(0x6A));
	/* SW disconnect */
#endif
	/* Set ru_uart_mode to 2'b01 */
	var = USB_PHY_Read_Register8(0x6B);
	DBG(0, "[MUSB]addr: 0x6B, value: %x\n", var);
	USB_PHY_Write_Register8(var | 0x7C, 0x6B);
	DBG(0, "[MUSB]addr: 0x6B, value after: %x\n", USB_PHY_Read_Register8(0x6B));

	/* Set RG_UART_EN to 1 */
	var = USB_PHY_Read_Register8(0x6E);
	DBG(0, "[MUSB]addr: 0x6E, value: %x\n", var);
	USB_PHY_Write_Register8(var | 0x07, 0x6E);
	DBG(0, "[MUSB]addr: 0x6E, value after: %x\n", USB_PHY_Read_Register8(0x6E));

	/* Set RG_USB20_DM_100K_EN to 1 */
	var = USB_PHY_Read_Register8(0x22);
	DBG(0, "[MUSB]addr: 0x22, value: %x\n", var);
	USB_PHY_Write_Register8(var | 0x02, 0x22);
	DBG(0, "[MUSB]addr: 0x22, value after: %x\n", USB_PHY_Read_Register8(0x22));

	var = DRV_Reg8(UART1_BASE + 0x90);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value: %x\n", var);
	DRV_WriteReg8(UART1_BASE + 0x90, var | 0x01);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value after: %x\n\n", DRV_Reg8(UART1_BASE + 0x90));

	/* SW disconnect */
	mt_usb_disconnect();
}

void usb_phy_switch_to_usb(void)
{
	int var;
	/* Set RG_UART_EN to 0 */
	var = USB_PHY_Read_Register8(0x6E);
	DBG(0, "[MUSB]addr: 0x6E, value: %x\n", var);
	USB_PHY_Write_Register8(var & ~0x01, 0x6E);
	DBG(0, "[MUSB]addr: 0x6E, value after: %x\n", USB_PHY_Read_Register8(0x6E));

	/* Set RG_USB20_DM_100K_EN to 0 */
	var = USB_PHY_Read_Register8(0x22);
	DBG(0, "[MUSB]addr: 0x22, value: %x\n", var);
	USB_PHY_Write_Register8(var & ~0x02, 0x22);
	DBG(0, "[MUSB]addr: 0x22, value after: %x\n", USB_PHY_Read_Register8(0x22));

	var = DRV_Reg8(UART1_BASE + 0x90);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value: %x\n", var);
	DRV_WriteReg8(UART1_BASE + 0x90, var & ~0x01);
	DBG(0, "[MUSB]addr: 0x11002090 (UART1), value after: %x\n\n", DRV_Reg8(UART1_BASE + 0x90));
#if 0
	/* SW connect */
	var = USB_PHY_Read_Register8(0x68);
	DBG(0, "[MUSB]addr: 0x68, value: %x\n", var);
	USB_PHY_Write_Register8(0x0, 0x68);
	DBG(0, "[MUSB]addr: 0x68, value after: %x\n", USB_PHY_Read_Register8(0x68));

	var = USB_PHY_Read_Register8(0x6A);
	DBG(0, "[MUSB]addr: 0x6A, value: %x\n", var);
	USB_PHY_Write_Register8(0x0, 0x6A);
	DBG(0, "[MUSB]addr: 0x6A, value after: %x\n", USB_PHY_Read_Register8(0x6A));
	/* SW connect */
#endif
	/* SW connect */
	mt_usb_connect();
}
#endif

#else

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool in_uart_mode = false;
#endif

static DEFINE_SPINLOCK(musb_reg_clock_lock);

static void enable_phy_clock(bool enable)
{
	/* USB phy 48M clock , UNIVPLL_CON0[26] */
	/* if (enable) { */
	/* writel(readl((void __iomem *)UNIVPLL_CON0)|(0x04000000), */
	/* (void __iomem *)UNIVPLL_CON0); */
	/* } else { */
	/* writel(readl((void __iomem *)UNIVPLL_CON0)&~(0x04000000), */
	/* (void __iomem *)UNIVPLL_CON0); */
	/* } */
}

bool usb_enable_clock(bool enable)
{
	static int count;
	static int real_enable = 0, real_disable;
	static int virt_enable = 0, virt_disable;
	bool res = TRUE;
	unsigned long flags;

	DBG(1, "enable(%d),count(%d),<%d,%d,%d,%d>\n",
	    enable, count, virt_enable, virt_disable, real_enable, real_disable);

	spin_lock_irqsave(&musb_reg_clock_lock, flags);

	if (enable && count == 0) {
		real_enable++;
		enable_phy_clock(true);
#ifdef CONFIG_MTK_CLKMGR
		res = enable_clock(MT_CG_PERI_USB0, "PERI_USB");
#else
		res = clk_enable(musb_clk);
#endif
	} else if (!enable && count == 1) {
		real_disable++;
#ifdef CONFIG_MTK_CLKMGR
		res = disable_clock(MT_CG_PERI_USB0, "PERI_USB");
#else
		res = 0;
		clk_disable(musb_clk);
#endif
		enable_phy_clock(false);
	}

	if (enable) {
		virt_enable++;
		count++;
	} else {
		virt_disable++;
		count = (count == 0) ? 0 : (count - 1);
	}

	spin_unlock_irqrestore(&musb_reg_clock_lock, flags);

	DBG(1, "enable(%d),count(%d),res(%d),<%d,%d,%d,%d>\n",
	    enable, count, res, virt_enable, virt_disable, real_enable, real_disable);
	return 1;
}

static void hs_slew_rate_cal(void)
{
	unsigned long data;
	unsigned long x;
	unsigned char value;
	unsigned long start_time, timeout;
	unsigned int timeout_flag = 0;
	/* 4 s1:enable usb ring oscillator. */
	USBPHY_SET8(0x15, 0x80);

	/* 4 s2:wait 1us. */
	udelay(1);

	/* 4 s3:enable free run clock */
	USBPHY_SET8(0xf00 - 0x800 + 0x11, 0x01);
	/* 4 s4:setting cyclecnt. */
	USBPHY_SET8(0xf00 - 0x800 + 0x01, 0x04);
	/* 4 s5:enable frequency meter */
	USBPHY_SET8(0xf00 - 0x800 + 0x03, 0x01);

	/* 4 s6:wait for frequency valid. */
	start_time = jiffies;
	timeout = jiffies + 3 * HZ;

	while (!(USBPHY_READ8(0xf00 - 0x800 + 0x10) & 0x1)) {
		if (time_after(jiffies, timeout)) {
			timeout_flag = 1;
			break;
		}
	}

	/* 4 s7: read result. */
	if (timeout_flag) {
		DBG(0, "[USBPHY] Slew Rate Calibration: Timeout\n");
		value = 0x4;
	} else {
		data = USBPHY_READ32(0xf00 - 0x800 + 0x0c);
		x = ((1024 * FRA * PARA) / data);
		value = (unsigned char)(x / 1000);
		if ((x - value * 1000) / 100 >= 5)
			value += 1;
		DBG(0, "[USBPHY]slew calibration:FM_OUT =%lu,x=%lu,value=%d\n", data, x, value);
	}

	/* 4 s8: disable Frequency and run clock. */
	USBPHY_CLR8(0xf00 - 0x800 + 0x03, 0x01);
	USBPHY_CLR8(0xf00 - 0x800 + 0x11, 0x01);

	/* 4 s9: */
#define MSK_RG_USB20_HSTX_SRCTRL 0x7
	/* all clr first then set */
	USBPHY_CLR8(0x15, MSK_RG_USB20_HSTX_SRCTRL << 4);
	USBPHY_SET8(0x15, (value & MSK_RG_USB20_HSTX_SRCTRL) << 4);

	/* 4 s10:disable usb ring oscillator. */
	USBPHY_CLR8(0x15, 0x80);
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);
	usb_port_mode = USBPHY_READ8(0x6B);
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) || (usb_port_mode == 0x5E)) {
		DBG(0, "%s:%d - IN UART MODE : 0x%x\n", __func__, __LINE__, usb_port_mode);
		in_uart_mode = true;
	} else {
		DBG(0, "%s:%d - NOT IN UART MODE : 0x%x\n", __func__, __LINE__, usb_port_mode);
		in_uart_mode = false;
	}
	return in_uart_mode;
}

void usb_phy_switch_to_uart(void)
{
	if (usb_phy_check_in_uart_mode())
		return;

	usb_enable_clock(true);
	udelay(50);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* Set RG_SUSPENDM to 1 */
	USBPHY_SET8(0x68, 0x08);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	/* Set ru_uart_mode to 2'b01 */
	USBPHY_SET8(0x6B, 0x5C);

	/* Set RG_UART_EN to 1 */
	USBPHY_SET8(0x6E, 0x07);

	/* Set RG_USB20_DM_100K_EN to 1 */
	USBPHY_SET8(0x22, 0x02);
	usb_enable_clock(false);

	/* GPIO Selection */
	DRV_WriteReg32(ap_uart0_base + 0xB0, 0x1);
}


void usb_phy_switch_to_usb(void)
{
	/* GPIO Selection */
	DRV_WriteReg32(ap_uart0_base + 0xB0, 0x0);

	usb_enable_clock(true);
	udelay(50);
	/* clear force_uart_en */
	USBPHY_WRITE8(0x6B, 0x00);
	usb_enable_clock(false);
	usb_phy_poweron();
	/* disable the USB clock turned on in usb_phy_poweron() */
	usb_enable_clock(false);
}
#endif

/* Denali_USB_PWR Sequence 20141030.xls */
void usb_phy_poweron(void)
{

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (usb_phy_check_in_uart_mode())
		return;
#endif

	/* enable USB MAC clock. */
	usb_enable_clock(true);

	/* wait 50 usec for PHY3.3v/1.8v stable. */
	udelay(50);

	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	USBPHY_CLR8(0x21, 0x03);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* rg_usb20_dp_100k_mode, 1'b1 */
	USBPHY_SET8(0x22, 0x04);
	/* USB20_DP_100K_EN 1'b0, RG_USB20_DM_100K_EN, 1'b0 */
	USBPHY_CLR8(0x22, 0x03);

	/* RG_USB20_OTG_VBUSCMP_EN, 1'b1 */
	USBPHY_SET8(0x1a, 0x10);

	/* force_suspendm, 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);

	/* 7 s7: wait for 800 usec. */
	udelay(800);

	/* force enter device mode, from K2, FIXME */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2F);
	USBPHY_SET8(0x6d, 0x3F);

	DBG(0, "usb power on success\n");
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
static bool skipDisableUartMode = true;
#endif

/* Denali_USB_PWR Sequence 20141030.xls */
static void usb_phy_savecurrent_internal(void)
{

	/* 4 1. swtich to USB function. (system register, force ip into usb mode. */

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* enable USB MAC clock. */
		usb_enable_clock(true);

		/* wait 50 usec for PHY3.3v/1.8v stable. */
		udelay(50);

		/* force_uart_en, 1'b0 */
		USBPHY_CLR8(0x6b, 0x04);
		/* RG_UART_EN, 1'b0 */
		USBPHY_CLR8(0x6e, 0x01);
		/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
		USBPHY_CLR8(0x21, 0x03);

		/* RG_USB20_BC11_SW_EN, 1'b0 */
		USBPHY_CLR8(0x1a, 0x80);
		/* RG_USB20_OTG_VBUSCMP_EN, 1'b0 */
		USBPHY_CLR8(0x1a, 0x10);

		/* RG_SUSPENDM, 1'b1 */
		USBPHY_SET8(0x68, 0x08);
		/* force_suspendm, 1'b1 */
		USBPHY_SET8(0x6a, 0x04);

		usb_enable_clock(false);
	} else {
		if (skipDisableUartMode)
			skipDisableUartMode = false;
		else
			return;
	}
#else
	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	USBPHY_CLR8(0x21, 0x03);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSCMP_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x10);

	/* RG_SUSPENDM, 1'b1 */
	USBPHY_SET8(0x68, 0x08);
	/* force_suspendm, 1'b1 */
	USBPHY_SET8(0x6a, 0x04);
#endif

	/* RG_DPPULLDOWN, 1'b1, RG_DMPULLDOWN, 1'b1 */
	USBPHY_SET8(0x68, 0xc0);
	/* RG_XCVRSEL[1:0], 2'b01. */
	USBPHY_CLR8(0x68, 0x20);
	USBPHY_SET8(0x68, 0x10);
	/* RG_TERMSEL, 1'b1 */
	USBPHY_SET8(0x68, 0x04);
	/* RG_DATAIN[3:0], 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/* force_dp_pulldown, 1'b1, force_dm_pulldown, 1'b1,
	   force_xcversel, 1'b1, force_termsel, 1'b1, force_datain, 1'b1 */
	USBPHY_SET8(0x6a, 0xba);

	udelay(800);

	/* RG_SUSPENDM, 1'b0 */
	USBPHY_CLR8(0x68, 0x08);

	/* ALPS00427972, implement the analog register formula */
	/*
	   DBG(0, "%s: USBPHY_READ8(0x05) = 0x%x\n", __func__, USBPHY_READ8(0x05));
	   DBG(0, "%s: USBPHY_READ8(0x07) = 0x%x\n", __func__, USBPHY_READ8(0x07));
	 */
	/* ALPS00427972, implement the analog register formula */

	udelay(1);

	/* force enter device mode, from K2, FIXME */
	/* force enter device mode */
	/* USBPHY_CLR8(0x6c, 0x10); */
	/* USBPHY_SET8(0x6c, 0x2E); */
	/* USBPHY_SET8(0x6d, 0x3E); */

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode) {
		USBPHY_SET8(0x68, 0x08);
		DBG(0, "%s:%d - SWITCH to UART MODE after savecurrent!\n", __func__, __LINE__);
	}
#endif
}

void usb_phy_savecurrent(void)
{


	usb_phy_savecurrent_internal();


	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);


	DBG(0, "usb save current success\n");
}

/* Denali_USB_PWR Sequence 20141030.xls */
void usb_phy_recover(void)
{


	/* turn on USB reference clock. */
	usb_enable_clock(true);


	/* wait 50 usec. */
	udelay(50);

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* clean PUPD_BIST_EN */
		/* PUPD_BIST_EN = 1'b0 */
		/* PMIC will use it to detect charger type */
		USBPHY_CLR8(0x1d, 0x10);

		/* force_uart_en, 1'b0 */
		USBPHY_CLR8(0x6b, 0x04);
		/* RG_UART_EN, 1'b0 */
		USBPHY_CLR8(0x6e, 0x01);
		/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
		USBPHY_CLR8(0x21, 0x03);

		/* force_suspendm, 1'b0 */
		USBPHY_CLR8(0x6a, 0x04);

		skipDisableUartMode = false;
	} else {
		if (!skipDisableUartMode)
			return;
	}
#else

	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	USBPHY_CLR8(0x1d, 0x10);

	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	USBPHY_CLR8(0x21, 0x03);

	/* force_suspendm, 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);
#endif

	/* RG_DPPULLDOWN, 1'b0, RG_DMPULLDOWN, 1'b0 */
	USBPHY_CLR8(0x68, 0xc0);
	/* RG_XCVRSEL[1:0], 2'b00. */
	USBPHY_CLR8(0x68, 0x30);
	/* RG_TERMSEL, 1'b0 */
	USBPHY_CLR8(0x68, 0x04);
	/* RG_DATAIN[3:0], 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/* force_dp_pulldown, 1'b0, force_dm_pulldown, 1'b0,
	   force_xcversel, 1'b0, force_termsel, 1'b0, force_datain, 1'b0 */
	USBPHY_CLR8(0x6a, 0xba);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSCMP_EN, 1'b1 */
	USBPHY_SET8(0x1a, 0x10);


	/* wait 800 usec. */
	udelay(800);

	/* force enter device mode, from K2, FIXME */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2F);
	USBPHY_SET8(0x6d, 0x3F);

	/* from K2, FIXME */
//#if defined(MTK_HDMI_SUPPORT)  //modify longcheer_liml_2017_03_08
	USBPHY_SET8(0x05, 0x06);
	USBPHY_SET8(0x05, 0x60);
//#endif

	/* adjustment after HQA */
	HQA_special();

	hs_slew_rate_cal();
/*huyunge@wind-mobi.com 20170327 modify for */
#ifdef CONFIG_WIND_OTG_HDD_SUPPORT
	USBPHY_SET8(0x18, 0xf0);
#endif
	DBG(0, "usb recovery success\n");
}

/* BC1.2 */
void Charger_Detect_Init(void)
{
	unsigned long flags;
	int do_lock;

	do_lock = 0;
	if (mtk_musb) {
		spin_lock_irqsave(&mtk_musb->lock, flags);
		do_lock = 1;
	} else {
		DBG(0, "mtk_musb is NULL\n");

	}
	/* turn on USB reference clock. */
	usb_enable_clock(true);
	/* wait 50 usec. */
	udelay(50);
	/* RG_USB20_BC11_SW_EN = 1'b1 */
	USBPHY_SET8(0x1a, 0x80);

	if (do_lock)
		spin_unlock_irqrestore(&mtk_musb->lock, flags);
	DBG(0, "Charger_Detect_Init\n");
}

void Charger_Detect_Release(void)
{
	unsigned long flags;
	int do_lock;

	do_lock = 0;
	if (mtk_musb) {
		spin_lock_irqsave(&mtk_musb->lock, flags);
		do_lock = 1;
	} else {
		DBG(0, "mtk_musb is NULL\n");
	}

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	udelay(1);
	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);

	if (do_lock)
		spin_unlock_irqrestore(&mtk_musb->lock, flags);
	DBG(0, "Charger_Detect_Release\n");
}

void usb_phy_context_save(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	in_uart_mode = usb_phy_check_in_uart_mode();
#endif
}

void usb_phy_context_restore(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		usb_phy_switch_to_uart();
#endif
	usb_phy_savecurrent_internal();
}

#endif

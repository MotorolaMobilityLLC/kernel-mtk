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

#ifndef __USB20_H__
#define __USB20_H__

#ifdef CONFIG_FPGA_EARLY_PORTING
#define FPGA_PLATFORM
#endif

struct mt_usb_glue {
	struct device *dev;
	struct platform_device *musb;
};

#if CONFIG_MTK_GAUGE_VERSION == 30
#define KAL_TRUE	true
#define KAL_FALSE	false
typedef bool	kal_bool;

extern unsigned int upmu_get_rgs_chrdet(void);
#endif

extern CHARGER_TYPE mt_charger_type_detection(void);
extern kal_bool upmu_is_chr_det(void);
extern void BATTERY_SetUSBState(int usb_state);
extern void upmu_interrupt_chrdet_int_en(unsigned int val);

/* specific USB fuctnion */
typedef enum {
	CABLE_MODE_CHRG_ONLY = 0,
	CABLE_MODE_NORMAL,
	CABLE_MODE_HOST_ONLY,
	CABLE_MODE_MAX
} CABLE_MODE;

#ifdef CONFIG_MTK_UART_USB_SWITCH
typedef enum {
	PORT_MODE_USB = 0,
	PORT_MODE_UART,
	PORT_MODE_MAX
} PORT_MODE;

extern bool usb_phy_check_in_uart_mode(void);
extern void usb_phy_switch_to_usb(void);
extern void usb_phy_switch_to_uart(void);
#endif

#ifdef FPGA_PLATFORM
extern void USB_PHY_Write_Register8(UINT8 var, UINT8 addr);
extern UINT8 USB_PHY_Read_Register8(UINT8 addr);
#endif

extern struct clk *musb_clk;
extern void __iomem *ap_uart0_base;
#ifdef CONFIG_MTK_UART_USB_SWITCH
extern bool in_uart_mode;
#endif
extern int usb20_phy_init_debugfs(void);
extern CHARGER_TYPE mt_get_charger_type(void);
#include <upmu_common.h>
bool hwPowerOn(MT65XX_POWER powerId, int powerVolt, char *mode_name);

#if defined(CONFIG_USB_MTK_OTG) && defined(CONFIG_TCPC_CLASS)
extern bool usb20_host_tcpc_boost_on;
#endif
#endif

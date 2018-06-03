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


#ifndef __PHY_MTK_SSUSB_H
#define __PHY_MTK_SSUSB_H
#include <linux/types.h>
#include <linux/clk.h>

enum mtk_usb_phy_role {
	ROLE_DEVICE,
	ROLE_HOST,
	ROLE_SIB,
	ROLE_UART,
};

#if 0
enum mtk_usb_idle_state {
	USB_PHY_DPIDLE_ALLOWED = 0,
	USB_PHY_DPIDLE_FORBIDDEN,
	USB_PHY_DPIDLE_SRAM,
	USB_PHY_DPIDLE_TIMER
};
#endif

struct mtk_phy_tuning {
	s32 u2_vrt_ref;
	s32 u2_term_ref;
	s32 u2_enhance;
	bool inited;
};

struct mtk_phy_instance {
	const struct mtk_phy_interface *phycfg;
	struct mtk_phy_drv *phy_drv;
	struct phy *phy;
	void __iomem *port_base;
	bool sib_mode;
	bool uart_mode;
	int phy_number;
	struct mtk_phy_tuning phy_tuning;
};


struct mtk_phy_interface {
	int  (*usb_phy_init)(struct mtk_phy_instance *x);
	void (*usb_phy_savecurrent)(struct mtk_phy_instance *x);
	void (*usb_phy_recover)(struct mtk_phy_instance *x);
	void (*usb_phy_switch_to_bc11)(struct mtk_phy_instance *x, bool on);
	int  (*usb_phy_lpm_enable)(struct mtk_phy_instance *x, bool on);
	int  (*usb_phy_host_mode)(struct mtk_phy_instance *x, bool on);
	int  (*usb_phy_io_read)(struct mtk_phy_instance *x, int mode, u32 reg);
	int  (*usb_phy_io_write)(struct mtk_phy_instance *x, int mode, u32 val, u32 reg);
	void (*usb_phy_switch_to_usb)(struct mtk_phy_instance *x);
	void (*usb_phy_switch_to_uart)(struct mtk_phy_instance *x);
	bool (*usb_phy_check_in_uart_mode)(struct mtk_phy_instance *x);
	void (*usb_phy_sib_enable_switch)(struct mtk_phy_instance *x, bool enable);
	bool (*usb_phy_u3_loop_back_test)(struct mtk_phy_instance *x);
	unsigned int port_num;
	unsigned int reg_offset;
	char *name;
	char *tuning_node_name;
};

struct mtk_usbphy_config {
	const struct mtk_phy_interface *phys;
	unsigned int num_phys;
	int (*usb_drv_init)(struct platform_device *pdev, struct mtk_phy_drv *drv);
	int (*usb_drv_exit)(struct platform_device *pdev, struct mtk_phy_drv *drv);
};

struct mtk_phy_drv {
	struct device *dev;
	const struct mtk_usbphy_config *phycfg;
	void __iomem *phy_base;
	void __iomem *ippc_base;
	struct mtk_phy_instance **phys;
	int nphys;
	struct clk *clk;
	struct clk *ssusb_clk_refpll;
	struct clk *ssusb_clk_mux;
	struct clk *ssusb_clk_mux_default;
	struct clk *ssusb_clk_mux_host;
	struct regulator *vusb33;
	struct regulator *vusb10;
};

extern const struct mtk_usbphy_config ssusb_phy_config;

/*Set the debug level for phy driver*/
#define K_ALET	(1<<6)
#define K_CRIT	(1<<5)
#define K_ERR	(1<<4)
#define K_WARNIN	(1<<3)
#define K_NOTICE	(1<<2)
#define K_INFO		(1<<1)
#define K_DEBUG	(1<<0)

extern u32 phy_debug_level;

#define phy_printk(level, fmt, args...) do { \
		if (phy_debug_level & level) { \
			pr_info("[MTKPHY]" fmt, ## args); \
		} \
	} while (0)

#endif


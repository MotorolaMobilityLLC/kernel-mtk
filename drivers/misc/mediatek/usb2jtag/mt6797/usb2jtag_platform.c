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

#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "../usb2jtag_v1.h"
#include <mach/upmu_hw.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/sync_write.h>
void __iomem *INFRA_AO_BASE;
void __iomem *USB_SIF_BASE;
static int mt_usb2jtag_hw_init(void)
{
	struct device_node *node = NULL;
	int ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,usb2jtag_v1");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mediatek,usb2jtag_v1 failed\n");
		return -1;
	}
	INFRA_AO_BASE = of_iomap(node, 0);
	if (!INFRA_AO_BASE) {
		pr_err("[USB2JTAG] iomap INFRA_AO_BASE failed\n");
		return -1;
	}
	USB_SIF_BASE = of_iomap(node, 1);
	if (!USB_SIF_BASE) {
		pr_err("[USB2JTAG] iomap USB_SIF_BASE failed\n");
		return -1;
	}

	/* set ap_usb2jtag_en: 0x1000_0700 bit[13] = 1 */
	writel(readl(INFRA_AO_BASE + 0xF00) | (0x1 << 14), INFRA_AO_BASE + 0xF00);

	ret = pmic_set_register_value(MT6351_PMIC_RG_VUSB33_EN, 0x01);
	if (ret)
		pr_debug("VUSB33 enable FAIL!!!\n");

	/* Set RG_VUSB10_ON as 1 after VDD10 Ready */
	/*hwPowerOn(MT6331_POWER_LDO_VUSB10, VOL_1000, "VDD10_USB_P0"); */
	ret = pmic_set_register_value(MT6351_PMIC_RG_VA10_EN, 0x01);
	if (ret)
		pr_debug("VA10 enable FAIL!!!\n");

	ret = pmic_set_register_value(MT6351_PMIC_RG_VA10_VOSEL, 0x01);
	if (ret)
		pr_debug("VA10 output selection to 0.95 FAIL!!!\n");

	/* delay for enabling usb clock */
	udelay(50);
	writel(readl(USB_SIF_BASE + 0x0808) & ~(0x1 << 17), USB_SIF_BASE + 0x0808);
	/* Set rg_usb_gpio_ctrl: 0x11210821 bit[1] = 1 */
	writel(readl(USB_SIF_BASE + 0x0820) | (0x1 << 9), USB_SIF_BASE + 0x0820);
	/* clear RG_USB20_BC11_SW_EN: 0x1121081A bit[7] = 0 */
	writel(readl(USB_SIF_BASE + 0x0818) & ~(0x1 << 23), USB_SIF_BASE + 0x0818);
	/* Set BGR_EN 0x11210800 bit[0] = 1 */
	writel(readl(USB_SIF_BASE + 0x0800) | (0x1 << 0), USB_SIF_BASE + 0x0800);
	pr_err("[USB2JTAG] 0x11210820 = 0x%x\n", readl(USB_SIF_BASE + 0x0820));
	pr_err("[USB2JTAG] 0x11210818 = 0x%x\n", readl(USB_SIF_BASE + 0x0818));
	pr_err("[USB2JTAG] setting done\n");
	return 0;
}


static int __init mt_usb2jtag_platform_init(void)
{
	struct mt_usb2jtag_driver *mt_usb2jtag_drv;

	mt_usb2jtag_drv = get_mt_usb2jtag_drv();
	mt_usb2jtag_drv->usb2jtag_init = mt_usb2jtag_hw_init;
	mt_usb2jtag_drv->usb2jtag_suspend = NULL;
	mt_usb2jtag_drv->usb2jtag_resume = NULL;

	return 0;
}

arch_initcall(mt_usb2jtag_platform_init);

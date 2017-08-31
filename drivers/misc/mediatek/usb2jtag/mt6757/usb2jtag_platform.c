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

#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "../usb2jtag_v1.h"
#include <mach/upmu_hw.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/sync_write.h>
void __iomem *INFRA_AO_BASE;
void __iomem *USB0_PSIF2_BASE;
void __iomem *USB1_PSIF_BASE;
static int mt_usb2jtag_hw_init(void)
{
	struct device_node *node = NULL;
#ifndef CONFIG_FPGA_EARLY_PORTING
	int ret;
#endif

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
	USB0_PSIF2_BASE = of_iomap(node, 1);
	if (!USB0_PSIF2_BASE) {
		pr_err("[USB2JTAG] iomap USB0_PSIF2_BASE failed\n");
		return -1;
	}
	USB1_PSIF_BASE = of_iomap(node, 2);
	if (!USB1_PSIF_BASE) {
		pr_err("[USB2JTAG] iomap USB1_PSIF_BASE failed\n");
		return -1;
	}


	/* set ap_usb2jtag_en: 0x1000_1F00 bit[14] = 1 */
	writel(readl(INFRA_AO_BASE + 0xF00) | (0x1 << 14), INFRA_AO_BASE + 0xF00);

#ifndef CONFIG_FPGA_EARLY_PORTING
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
#endif

	writel(0x00100488, USB0_PSIF2_BASE + 0x0818);
	writel(0x0000006f, USB0_PSIF2_BASE + 0x0800);
	writel(0x00000000, USB0_PSIF2_BASE + 0x0808);

	/* Set BGR_EN 0x11210800 bit[0] = 1 */
	writel(readl(USB1_PSIF_BASE + 0x0800) | 0x21, USB1_PSIF_BASE + 0x0800);
	/* clear RG_USB20_BC11_SW_EN: 0x1121081A bit[7] = 0 */
	writel(readl(USB1_PSIF_BASE + 0x0818) & ~(1 << 23), USB1_PSIF_BASE + 0x0818);
	/* Set rg_usb_gpio_ctrl: 0x11210821 bit[1] = 1 */
	writel(readl(USB1_PSIF_BASE + 0x0820) | (1 << 9), USB1_PSIF_BASE + 0x0820);


	writel(0x00050000, USB0_PSIF2_BASE + 0x086c);
	/* delay for enabling usb clock */
	udelay(100);
	writel(0x0000ff1a, USB0_PSIF2_BASE + 0x0820);
#if 0
	pr_err("[USB2JTAG] 0x11210820 = 0x%x\n", readl(USB_SIF_BASE + 0x0820));
	pr_err("[USB2JTAG] 0x11210818 = 0x%x\n", readl(USB_SIF_BASE + 0x0818));
	pr_err("[USB2JTAG] setting done\n");
#endif
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

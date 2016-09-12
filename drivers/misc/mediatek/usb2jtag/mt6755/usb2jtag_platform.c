/*
* Copyright (C) 2016 MediaTek Inc.
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "../usb2jtag_v1.h"
#include <mt-plat/sync_write.h>
#include <mt-plat/upmu_common.h>

static int mt_usb2jtag_hw_init(void)
{
	unsigned int ret = 0;
	struct device_node *node = NULL;

	void __iomem *INFRA_AO_BASE;
	void __iomem *USB0PSIF2_BASE;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-infrasys");
	if (!node) {
		pr_err("[USB2JTAG] map node @ infrasys failed\n");
		return -1;
	}
	INFRA_AO_BASE = of_iomap(node, 0);
	if (!INFRA_AO_BASE) {
		pr_err("[USB2JTAG] iomap infrasys base failed\n");
		return -1;
	}
	node = of_find_compatible_node(NULL, NULL, "mediatek,usb0p_sif2");
	if (!node) {
		pr_err("[USB2JTAG] map node @ usb0p_sif2 failed\n");
		return -1;
	}

	USB0PSIF2_BASE = of_iomap(node, 0);
	if (!node) {
		pr_err("[USB2JTAG] iomap usb0p_sif2 base failed\n");
		return -1;
	}
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	ret = pmic_set_register_value(PMIC_LDO_VUSB33_EN, 0x01);
#else /* for pmic mt6351 setting */
	ret |= pmic_set_register_value(MT6351_PMIC_RG_VUSB33_EN, 0x01);
	ret |= pmic_set_register_value(MT6351_PMIC_RG_VA10_EN, 0x01);
	ret |= pmic_set_register_value(MT6351_PMIC_RG_VA10_VOSEL, 0x02);
#endif
	if (ret) {
		pr_err("[USB2JTAG] USB2JTAGE Voltage config failed\n");
		BUG_ON(1);
	}

	writew(readl(INFRA_AO_BASE + 0xF00) | 0x1 << 14, INFRA_AO_BASE + 0xF00);
	writel(0x00100488 , USB0PSIF2_BASE + 0x818);
	writel(0x6F, USB0PSIF2_BASE + 0x800);
	writel(0x0, USB0PSIF2_BASE + 0x808);
	writel(0xFF1A, USB0PSIF2_BASE + 0x820);
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

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

void __iomem *PERICFG_BASE;

static int mt_usb2jtag_hw_init(void)
{
	struct device_node *node = NULL;
	unsigned int temp;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-pericfg");
	if (!node) {
		pr_err("[USB2JTAG] map node @ mediatek,mt6799-pericfg failed\n");
		return -1;
	}

	PERICFG_BASE = of_iomap(node, 0);
	if (!PERICFG_BASE) {
		pr_err("[USB2JTAG] iomap PERICFG_BASE failed\n");
		return -1;
	}

	/* Init USB config */
	if (usb2jtag_usb_init() != 0) {
		pr_err("[USB2JTAG] USB init failed\n");
		return -1;
	}
	temp = readl(PERICFG_BASE + 0x6c);
	writel(temp | 0x00010000, PERICFG_BASE + 0x6c);

#if 0
	pr_err("[USB2JTAG] 0x1101006c = 0x%x\n", readl(PERICFG_BASE + 0x6c));
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

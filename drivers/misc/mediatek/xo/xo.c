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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/switch.h>
#include <linux/i2c.h>
#ifndef CONFIG_OF
#include <mach/irqs.h>
#endif
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#ifndef FPGA_PLATFORM
#include <linux/clk.h>
struct clk *bsi_clk;
struct clk *bsirg_clk;
#endif
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

static const struct of_device_id apusb_of_ids[] = {
	{ .compatible = "mediatek,mt8167-xo", },
	{},
};

MODULE_DEVICE_TABLE(of, apusb_of_ids);

static int mt_xo_dts_probe(struct platform_device *pdev)
{
	int retval = 0;

#ifndef FPGA_PLATFORM
	bsi_clk = devm_clk_get(&pdev->dev, "bsi");
	if (IS_ERR(bsi_clk))
		return PTR_ERR(bsi_clk);
	retval = clk_prepare(bsi_clk);
	if (!retval)
		return retval;

	bsirg_clk = devm_clk_get(&pdev->dev, "rgbsi");
	if (IS_ERR(bsirg_clk))
		return PTR_ERR(bsirg_clk);
	retval = clk_prepare(bsirg_clk);
	if (!retval)
		return retval;

	clk_enable(bsirg_clk);
	clk_enable(bsi_clk);
#endif
	return retval;
}

static int mt_xo_dts_remove(struct platform_device *pdev)
{
#ifndef FPGA_PLATFORM
	clk_disable(bsirg_clk);
	clk_disable(bsi_clk);
	clk_unprepare(bsirg_clk);
	clk_unprepare(bsi_clk);
#endif

	return 0;
}

static struct platform_driver mt_xo_dts_driver = {
	.remove		= mt_xo_dts_remove,
	.probe		= mt_xo_dts_probe,
	.driver		= {
		.name	= "mt_dts_xo",
#ifdef CONFIG_OF
		.of_match_table = apusb_of_ids,
#endif
	},
};

static int __init mt_xo_init(void)
{
	int ret;

	ret = platform_driver_register(&mt_xo_dts_driver);

	return ret;
}

module_init(mt_xo_init);

static void __exit mt_xo_exit(void)
{
	platform_driver_unregister(&mt_xo_dts_driver);
}
module_exit(mt_xo_exit);


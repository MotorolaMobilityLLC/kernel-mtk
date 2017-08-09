/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Flora Fu, MediaTek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/mfd/mt6397/registers.h>

static const struct mfd_cell mt6397_devs[] = {
	{
		.name = "mt6397-pmic",
		.of_compatible = "mediatek,mt6397-pmic",
	}, {
		.name = "mt6397-regulator",
		.of_compatible = "mediatek,mt6397-regulator",
	}, {
		.name = "mt6397-pinctrl",
		.of_compatible = "mediatek,mt6397-pinctrl",
	},  {
		.name = "mt6397-codec",
		.of_compatible = "mediatek,mt6397-codec",
	},
};

static int mt6397_probe(struct platform_device *pdev)
{
	int ret;
	struct mt6397_chip *mt6397;

	mt6397 = devm_kzalloc(&pdev->dev, sizeof(*mt6397), GFP_KERNEL);
	if (!mt6397)
		return -ENOMEM;

	mt6397->dev = &pdev->dev;
	/*
	 * mt6397 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	mt6397->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!mt6397->regmap)
		return -ENODEV;

	platform_set_drvdata(pdev, mt6397);

	mt6397->irq = platform_get_irq(pdev, 0);

	if (mt6397->irq <= 0)
		return -EINVAL;

	ret = mfd_add_devices(&pdev->dev, -1, mt6397_devs,
			ARRAY_SIZE(mt6397_devs), NULL, 0, NULL);
	if (ret)
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);

	return ret;
}

static int mt6397_remove(struct platform_device *pdev)
{
	mfd_remove_devices(&pdev->dev);

	return 0;
}

static const struct of_device_id mt6397_of_match[] = {
	{ .compatible = "mediatek,mt6397" },
	{ }
};
MODULE_DEVICE_TABLE(of, mt6397_of_match);

static struct platform_driver mt6397_driver = {
	.probe = mt6397_probe,
	.remove = mt6397_remove,
	.driver = {
		.name = "mt6397",
		.of_match_table = of_match_ptr(mt6397_of_match),
	},
};

module_platform_driver(mt6397_driver);

MODULE_AUTHOR("Flora Fu, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6397 PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt6397");

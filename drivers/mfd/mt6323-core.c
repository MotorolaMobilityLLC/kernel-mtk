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
#include <linux/mfd/mt6323/core.h>
#include <linux/mfd/mt6323/registers.h>

static const struct mfd_cell mt6323_devs[] = {
	{
		.name = "mt6323-regulator",
		.of_compatible = "mediatek,mt6323-regulator"
	}, {
		.name = "mt6323-pmic",
		.of_compatible = "mediatek,mt6323-pmic"
	},
};

static int mt6323_probe(struct platform_device *pdev)
{
	int ret;
	struct mt6323_chip *mt6323;

	mt6323 = devm_kzalloc(&pdev->dev, sizeof(*mt6323), GFP_KERNEL);
	if (!mt6323)
		return -ENOMEM;

	mt6323->dev = &pdev->dev;
	/*
	 * mt6323 MFD is child device of soc pmic wrapper.
	 * Regmap is set from its parent.
	 */
	mt6323->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!mt6323->regmap)
		return -ENODEV;

	platform_set_drvdata(pdev, mt6323);

	ret = mfd_add_devices(&pdev->dev, -1, mt6323_devs,
			ARRAY_SIZE(mt6323_devs), NULL, 0, NULL);
	if (ret)
		dev_err(&pdev->dev, "failed to add child devices: %d\n", ret);

	return ret;
}

static int mt6323_remove(struct platform_device *pdev)
{
	mfd_remove_devices(&pdev->dev);

	return 0;
}

static const struct of_device_id mt6323_of_match[] = {
	{ .compatible = "mediatek,mt6323" },
	{ }
};
MODULE_DEVICE_TABLE(of, mt6323_of_match);

static struct platform_driver mt6323_driver = {
	.probe = mt6323_probe,
	.remove = mt6323_remove,
	.driver = {
		.name = "mt6323",
		.of_match_table = of_match_ptr(mt6323_of_match),
	},
};

module_platform_driver(mt6323_driver);

MODULE_AUTHOR("Flora Fu, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6323 PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt6323");

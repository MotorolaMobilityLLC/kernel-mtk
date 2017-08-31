/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <linux/mfd/rt5081_pmu.h>

struct rt5081_pmu_ldo_data {
	struct rt5081_pmu_chip *chip;
	struct device *dev;
};

static irqreturn_t rt5081_pmu_ldo_oc_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static struct rt5081_pmu_irq_desc rt5081_ldo_irq_desc[] = {
	RT5081_PMU_IRQDESC(ldo_oc),
};

static void rt5081_pmu_ldo_irq_register(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(rt5081_ldo_irq_desc); i++) {
		if (!rt5081_ldo_irq_desc[i].name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
						   rt5081_ldo_irq_desc[i].name);
		if (!res)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, res->start, NULL,
					rt5081_ldo_irq_desc[i].irq_handler,
					IRQF_TRIGGER_FALLING,
					rt5081_ldo_irq_desc[i].name,
					platform_get_drvdata(pdev));
		if (ret < 0) {
			dev_err(&pdev->dev, "request %s irq fail\n", res->name);
			continue;
		}
		rt5081_ldo_irq_desc[i].irq = res->start;
	}
}

static inline int rt_parse_dt(struct device *dev)
{
	return 0;
}

static int rt5081_pmu_ldo_probe(struct platform_device *pdev)
{
	struct rt5081_pmu_ldo_data *ldo_data;
	bool use_dt = pdev->dev.of_node;

	ldo_data = devm_kzalloc(&pdev->dev, sizeof(*ldo_data), GFP_KERNEL);
	if (!ldo_data)
		return -ENOMEM;
	if (use_dt)
		rt_parse_dt(&pdev->dev);
	ldo_data->chip = dev_get_drvdata(pdev->dev.parent);
	ldo_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, ldo_data);

	rt5081_pmu_ldo_irq_register(pdev);
	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return 0;
}

static int rt5081_pmu_ldo_remove(struct platform_device *pdev)
{
	struct rt5081_pmu_ldo_data *ldo_data = platform_get_drvdata(pdev);

	dev_info(ldo_data->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct of_device_id rt_ofid_table[] = {
	{ .compatible = "richtek,rt5081_pmu_ldo", },
	{ },
};
MODULE_DEVICE_TABLE(of, rt_ofid_table);

static const struct platform_device_id rt_id_table[] = {
	{ "rt5081_pmu_ldo", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, rt_id_table);

static struct platform_driver rt5081_pmu_ldo = {
	.driver = {
		.name = "rt5081_pmu_ldo",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt_ofid_table),
	},
	.probe = rt5081_pmu_ldo_probe,
	.remove = rt5081_pmu_ldo_remove,
	.id_table = rt_id_table,
};
module_platform_driver(rt5081_pmu_ldo);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("cy_huang <cy_huang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU Vib LDO");
MODULE_VERSION("1.0.0_G");

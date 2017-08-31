/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include "inc/rt5081_pmu.h"

struct rt5081_pmu_core_data {
	struct rt5081_pmu_chip *chip;
	struct device *dev;
};

static irqreturn_t rt5081_pmu_otp_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_core_data *core_data = data;

	dev_err(core_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_vdda_ovp_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_core_data *core_data = data;

	dev_err(core_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_vdda_uv_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_core_data *core_data = data;

	dev_err(core_data->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static struct rt5081_pmu_irq_desc rt5081_core_irq_desc[] = {
	RT5081_PMU_IRQDESC(otp),
	RT5081_PMU_IRQDESC(vdda_ovp),
	RT5081_PMU_IRQDESC(vdda_uv),
};

static void rt5081_pmu_core_irq_register(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(rt5081_core_irq_desc); i++) {
		if (!rt5081_core_irq_desc[i].name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
						rt5081_core_irq_desc[i].name);
		if (!res)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, res->start, NULL,
					rt5081_core_irq_desc[i].irq_handler,
					IRQF_TRIGGER_FALLING,
					rt5081_core_irq_desc[i].name,
					platform_get_drvdata(pdev));
		if (ret < 0) {
			dev_err(&pdev->dev, "request %s irq fail\n", res->name);
			continue;
		}
		rt5081_core_irq_desc[i].irq = res->start;
	}
}

static inline int rt_parse_dt(struct device *dev)
{
	return 0;
}

static int rt5081_pmu_core_probe(struct platform_device *pdev)
{
	struct rt5081_pmu_core_data *core_data;
	bool use_dt = pdev->dev.of_node;

	core_data = devm_kzalloc(&pdev->dev, sizeof(*core_data), GFP_KERNEL);
	if (!core_data)
		return -ENOMEM;
	if (use_dt)
		rt_parse_dt(&pdev->dev);
	core_data->chip = dev_get_drvdata(pdev->dev.parent);
	core_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, core_data);

	rt5081_pmu_core_irq_register(pdev);
	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return 0;
}

static int rt5081_pmu_core_remove(struct platform_device *pdev)
{
	struct rt5081_pmu_core_data *core_data = platform_get_drvdata(pdev);

	dev_info(core_data->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct of_device_id rt_ofid_table[] = {
	{ .compatible = "richtek,rt5081_pmu_core", },
	{ },
};
MODULE_DEVICE_TABLE(of, rt_ofid_table);

static const struct platform_device_id rt_id_table[] = {
	{ "rt5081_pmu_core", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, rt_id_table);

static struct platform_driver rt5081_pmu_core = {
	.driver = {
		.name = "rt5081_pmu_core",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt_ofid_table),
	},
	.probe = rt5081_pmu_core_probe,
	.remove = rt5081_pmu_core_remove,
	.id_table = rt_id_table,
};
module_platform_driver(rt5081_pmu_core);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("cy_huang <cy_huang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU Core");
MODULE_VERSION("1.0.0_G");

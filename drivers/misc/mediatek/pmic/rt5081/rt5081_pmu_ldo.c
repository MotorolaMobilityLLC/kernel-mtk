/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  Patrick Chang <patrick_chang@richtek.com>
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
#include <linux/regulator/mediatek/mtk_regulator_core.h>
#include <linux/regulator/mediatek/mtk_regulator.h>
#include "inc/rt5081_pmu.h"

#define rt5081_ldo_vol_reg RT5081_PMU_REG_LDOVOUT
#define rt5081_ldo_vol_mask (0x0F)
#define rt5081_ldo_vol_shift (0)
#define rt5081_ldo_enable_reg RT5081_PMU_REG_LDOVOUT
#define rt5081_ldo_enable_bit (1 << 7)
#define rt5081_ldo_min_uV (1600000)
#define rt5081_ldo_max_uV (4000000)
#define rt5081_ldo_step_uV (200000)
#define rt5081_ldo_id 0
#define rt5081_ldo_type REGULATOR_VOLTAGE

struct rt5081_pmu_ldo_data {
	struct rt5081_pmu_chip *chip;
	struct device *dev;
	struct mtk_simple_regulator_desc mreg_desc;
};

struct rt5081_pmu_ldo_platform_data {
	uint8_t cfg;
};

static irqreturn_t rt5081_pmu_ldo_oc_irq_handler(int irq, void *data)
{
	pr_info("%s: IRQ triggered\n", __func__);
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

static int rt5081_ldo_reg_read(void *chip, uint32_t reg, uint32_t *data)
{
	int ret = rt5081_pmu_reg_read(chip, (uint8_t)reg);

	if (ret < 0)
		return ret;
	*data = ret;
	return 0;
}

static int rt5081_ldo_reg_write(void *chip, uint32_t reg, uint32_t data)
{
	return rt5081_pmu_reg_write(chip, (uint8_t)reg, (uint8_t)data);
}

static int rt5081_ldo_reg_update_bits(void *chip, uint32_t reg, uint32_t mask,
		uint32_t data)
{
	return rt5081_pmu_reg_update_bits(chip, (uint8_t)reg,
			(uint8_t)mask, (uint8_t)data);
}

static int rt5081_ldo_list_voltage(struct mtk_simple_regulator_desc *mreg_desc,
	unsigned selector)
{
	int vout = 0;

	vout = rt5081_ldo_min_uV + selector * rt5081_ldo_step_uV;
	if (vout > rt5081_ldo_max_uV)
		return -EINVAL;
	return vout;
}

static struct mtk_simple_regulator_control_ops mreg_ctrl_ops = {
	.register_read = rt5081_ldo_reg_read,
	.register_write = rt5081_ldo_reg_write,
	.register_update_bits = rt5081_ldo_reg_update_bits,
};

static const struct mtk_simple_regulator_desc mreg_desc_table[] = {
	mreg_decl(rt5081_ldo, rt5081_ldo_list_voltage, 13, &mreg_ctrl_ops),
};

static inline int rt_parse_dt(struct device *dev,
	struct rt5081_pmu_ldo_platform_data *pdata,
	struct rt5081_pmu_ldo_platform_data *mask)
{
	struct device_node *np = dev->of_node;
	uint32_t val;

	if (of_property_read_u32(np, "ldo_oms", &val) == 0) {
		mask->cfg |= (0x1  <<  6);
		pdata->cfg |= (val  <<  6);
	}

	mask->cfg |= (0x01 << 5);
	if (of_property_read_u32(np, "ldo_vrc", &val) == 0) {
		mask->cfg |= (0x3  <<  1);
		pdata->cfg |= (val  <<  1);
		pdata->cfg |= (1  <<  5);
	}

	if (of_property_read_u32(np, "ldo_vrc_lt", &val) == 0) {
		mask->cfg = 0x3  <<  3;
		pdata->cfg = val  <<  3;
	}

	return 0;
}

static int ldo_apply_dts(struct rt5081_pmu_chip *chip,
	struct rt5081_pmu_ldo_platform_data *pdata,
	struct rt5081_pmu_ldo_platform_data *mask)
{
	return rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_LDOCFG,
		mask->cfg, pdata->cfg);
}

static int rt5081_ldo_enable(struct mtk_simple_regulator_desc *mreg_desc)
{
	int retval;

	pr_debug("%s: (%s) Enable regulator\n", __func__, mreg_desc->rdesc.name);
	retval = rt5081_ldo_reg_update_bits(mreg_desc->client,
		RT5081_PMU_REG_OSCCTRL, 0x01, 0x01);
	if (retval < 0)
		return retval;
	return rt5081_ldo_reg_update_bits(mreg_desc->client,
		mreg_desc->enable_reg, mreg_desc->enable_bit,
		mreg_desc->enable_bit);
}

static int rt5081_ldo_disable(struct mtk_simple_regulator_desc *mreg_desc)
{
	int retval;

	pr_debug("%s: (%s) disable regulator\n", __func__, mreg_desc->rdesc.name);
	retval =  rt5081_ldo_reg_update_bits(mreg_desc->client,
		RT5081_PMU_REG_OSCCTRL, 0x01, 0x00);
	return rt5081_ldo_reg_update_bits(mreg_desc->client,
		mreg_desc->enable_reg, mreg_desc->enable_bit, 0);
}

const  struct mtk_simple_regulator_ext_ops mreg_ext_ops = {
	.enable = rt5081_ldo_enable,
	.disable = rt5081_ldo_disable,
};

static int rt5081_pmu_ldo_probe(struct platform_device *pdev)
{
	int ret;
	struct rt5081_pmu_ldo_data *ldo_data;
	bool use_dt = pdev->dev.of_node;
	struct rt5081_pmu_ldo_platform_data pdata, mask;

	ldo_data = devm_kzalloc(&pdev->dev, sizeof(*ldo_data), GFP_KERNEL);
	if (!ldo_data)
		return -ENOMEM;

	memset(&pdata, 0, sizeof(pdata));
	memset(&mask, 0, sizeof(mask));
	if (use_dt)
		rt_parse_dt(&pdev->dev, &pdata, &mask);
	ldo_data->chip = dev_get_drvdata(pdev->dev.parent);
	ldo_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, ldo_data);
	ldo_data->mreg_desc = *mreg_desc_table;
	ldo_data->mreg_desc.client = ldo_data->chip;

	ret = ldo_apply_dts(ldo_data->chip, &pdata, &mask);
	if (ret < 0)
		goto probe_err;
	/*Check chip revision here */
	ret = mtk_simple_regulator_register(&ldo_data->mreg_desc,
		ldo_data->dev, &mreg_ext_ops, NULL);
	if (ret < 0)
		goto probe_err;

	rt5081_pmu_ldo_irq_register(pdev);

	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return ret;
probe_err:
	dev_info(&pdev->dev, "%s: register mtk regulator failed\n", __func__);
	return ret;
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
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU Vib LDO");
MODULE_VERSION("1.0.0_G");

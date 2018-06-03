/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  patrick_chang <patrick_chang@richtek.com>
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

#define rt5081_dsvp_vol_reg RT5081_PMU_REG_DBVPOS
#define rt5081_dsvp_vol_mask (0x3F)
#define rt5081_dsvp_vol_shift (0)
#define rt5081_dsvp_enable_reg RT5081_PMU_REG_DBCTRL2
#define rt5081_dsvp_enable_bit (1 << 6)
#define rt5081_dsvp_id 0
#define rt5081_dsvp_type REGULATOR_VOLTAGE

#define rt5081_dsvn_vol_reg RT5081_PMU_REG_DBVNEG
#define rt5081_dsvn_vol_mask (0x3F)
#define rt5081_dsvn_vol_shift (0)
#define rt5081_dsvn_enable_reg RT5081_PMU_REG_DBCTRL2
#define rt5081_dsvn_enable_bit (1 << 3)
#define rt5081_dsvn_id 1
#define rt5081_dsvn_type REGULATOR_VOLTAGE

#define rt5081_dsv_min_uV (4000000)
#define rt5081_dsv_max_uV (6000000)
#define rt5081_dsv_step_uV (50000)

#define rt5081_dsvp_min_uV rt5081_dsv_min_uV
#define rt5081_dsvp_max_uV rt5081_dsv_max_uV
#define rt5081_dsvp_step_uV rt5081_dsv_step_uV

#define rt5081_dsvn_min_uV rt5081_dsv_min_uV
#define rt5081_dsvn_max_uV rt5081_dsv_max_uV
#define rt5081_dsvn_step_uV rt5081_dsv_step_uV

enum {
	RT5081_DSV_POS,
	RT5081_DSV_NEG,
};

struct rt5081_pmu_dsv_data {
	struct rt5081_pmu_chip *chip;
	struct device *dev;
	struct mtk_simple_regulator_desc mreg_desc[2];
};

struct rt5081_pmu_dsv_platform_data {
	union {
		uint8_t raw;
		struct {
			uint8_t db_ext_en : 1;
			uint8_t reserved : 3;
			uint8_t db_periodic_fix : 1;
			uint8_t db_single_pin : 1;
			uint8_t db_freq_pm : 1;
			uint8_t db_periodic_mode : 1;
		};
	} db_ctrl1;

	union {
		uint8_t raw;
		struct {
			uint8_t db_startup : 1;
			uint8_t db_vneg_20ms : 1;
			uint8_t db_vneg_disc : 1;
			uint8_t reserved : 1;
			uint8_t db_vpos_20ms : 1;
			uint8_t db_vpos_disc : 1;
		};
	} db_ctrl2;

	union {
		uint8_t raw;
		struct {
			uint8_t vbst : 6;
			uint8_t delay : 2;
		} bitfield;
	} db_vbst;

	uint8_t db_vpos_slew;
	uint8_t db_vneg_slew;
};

static irqreturn_t rt5081_pmu_dsv_vneg_ocp_irq_handler(int irq, void *data)
{
	/* Use pr_info()  instead of dev_info */
	pr_info("%s: IRQ triggered\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dsv_vpos_ocp_irq_handler(int irq, void *data)
{
	pr_info("%s: IRQ triggered\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dsv_bst_ocp_irq_handler(int irq, void *data)
{
	pr_info("%s: IRQ triggered\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dsv_vneg_scp_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_dsv_data *dsv_data = data;
	int ret;

	pr_info("%s: IRQ triggered\n", __func__);
	ret = rt5081_pmu_reg_read(dsv_data->chip, RT5081_PMU_REG_DBSTAT);
	if (ret&0x40)
		regulator_notifier_call_chain(
			dsv_data->mreg_desc[RT5081_DSV_NEG].rdev,
			REGULATOR_EVENT_FAIL, NULL);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_dsv_vpos_scp_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_dsv_data *dsv_data = data;
	int ret;

	pr_info("%s: IRQ triggered\n", __func__);
	ret = rt5081_pmu_reg_read(dsv_data->chip, RT5081_PMU_REG_DBSTAT);
	if (ret&0x80)
		regulator_notifier_call_chain(
			dsv_data->mreg_desc[RT5081_DSV_POS].rdev,
			REGULATOR_EVENT_FAIL, NULL);
	return IRQ_HANDLED;
}

static struct rt5081_pmu_irq_desc rt5081_dsv_irq_desc[] = {
	RT5081_PMU_IRQDESC(dsv_vneg_ocp),
	RT5081_PMU_IRQDESC(dsv_vpos_ocp),
	RT5081_PMU_IRQDESC(dsv_bst_ocp),
	RT5081_PMU_IRQDESC(dsv_vneg_scp),
	RT5081_PMU_IRQDESC(dsv_vpos_scp),
};

static void rt5081_pmu_dsv_irq_register(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(rt5081_dsv_irq_desc); i++) {
		if (!rt5081_dsv_irq_desc[i].name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
						   rt5081_dsv_irq_desc[i].name);
		if (!res)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, res->start, NULL,
					rt5081_dsv_irq_desc[i].irq_handler,
					IRQF_TRIGGER_FALLING,
					rt5081_dsv_irq_desc[i].name,
					platform_get_drvdata(pdev));
		if (ret < 0) {
			dev_err(&pdev->dev, "request %s irq fail\n", res->name);
			continue;
		}
		rt5081_dsv_irq_desc[i].irq = res->start;
	}
}

static int rt5081_dsv_reg_read(void *chip, uint32_t reg, uint32_t *data)
{
	int ret = rt5081_pmu_reg_read(chip, (uint8_t)reg);

	if (ret < 0)
		return ret;
	*data = ret;
	return 0;
}

static int rt5081_dsv_reg_write(void *chip, uint32_t reg, uint32_t data)
{
	return rt5081_pmu_reg_write(chip, (uint8_t)reg, (uint8_t)data);
}

static int rt5081_dsv_reg_update_bits(void *chip, uint32_t reg, uint32_t mask,
		uint32_t data)
{
	return rt5081_pmu_reg_update_bits(chip, (uint8_t)reg,
			(uint8_t)mask, (uint8_t)data);
}

static int rt5081_dsv_list_voltage(struct mtk_simple_regulator_desc *mreg_desc,
	unsigned selector)
{
	int vout = 0;

	vout = rt5081_dsv_min_uV + selector * rt5081_dsv_step_uV;
	if (vout > rt5081_dsv_max_uV)
		return -EINVAL;
	return vout;
}

static struct mtk_simple_regulator_control_ops mreg_ctrl_ops = {
	.register_read = rt5081_dsv_reg_read,
	.register_write = rt5081_dsv_reg_write,
	.register_update_bits = rt5081_dsv_reg_update_bits,
};

static struct mtk_simple_regulator_desc mreg_desc_table[] = {
	mreg_decl(rt5081_dsvp, rt5081_dsv_list_voltage, 41, &mreg_ctrl_ops),
	mreg_decl(rt5081_dsvn, rt5081_dsv_list_voltage, 41, &mreg_ctrl_ops),
};

struct dbctrl_bitfield_desc {
	const char *name;
	uint8_t shift;
};

static const struct dbctrl_bitfield_desc dbctrl1_desc[] = {
	{ "db_ext_en", 0 },
	{ "db_periodic_fix", 4 },
	{ "db_single_pin", 5 },
	{ "db_freq_pm", 6 },
	{ "db_periodic_mode", 7 },
};

static const struct dbctrl_bitfield_desc dbctrl2_desc[] = {
	{ "db_startup", 0 },
	{ "db_vneg_20ms", 1 },
	{ "db_vneg_disc", 2 },
	{ "db_vpos_20ms", 4 },
	{ "db_vpos_disc", 5 },
};


static inline int rt_parse_dt(struct device *dev,
	struct rt5081_pmu_dsv_platform_data *pdata,
	struct rt5081_pmu_dsv_platform_data *mask)
{
	struct device_node *np = dev->of_node;
	int i;
	uint32_t val;

	for (i = 0; i < ARRAY_SIZE(dbctrl1_desc); i++) {
		if (of_property_read_u32(np, dbctrl1_desc[i].name, &val) == 0) {
			mask->db_ctrl1.raw |= (1 << dbctrl1_desc[i].shift);
			pdata->db_ctrl1.raw |= (val << dbctrl1_desc[i].shift);
		}
	}

	for (i = 0; i < ARRAY_SIZE(dbctrl2_desc); i++) {
		if (of_property_read_u32(np, dbctrl2_desc[i].name, &val) == 0) {
			mask->db_ctrl2.raw |= (1 << dbctrl2_desc[i].shift);
			pdata->db_ctrl2.raw |= (val << dbctrl2_desc[i].shift);
		}
	}

	if (of_property_read_u32(np, "db_delay", &val) == 0) {
		mask->db_vbst.bitfield.delay = 0x3;
		pdata->db_vbst.bitfield.delay = val;
	}

	if (of_property_read_u32(np, "db_vbst", &val) == 0) {
		if (val >= 4000 && val <= 6150) {
			mask->db_vbst.bitfield.vbst = 0x3f;
			pdata->db_vbst.bitfield.vbst = (val - 4000) / 50;
		}
	}

	if (of_property_read_u32(np, "db_vpos_slew", &val) == 0) {
		mask->db_vpos_slew = 0x3  <<  6;
		pdata->db_vpos_slew = val  <<  6;
	}

	if (of_property_read_u32(np, "db_vneg_slew", &val) == 0) {
		mask->db_vneg_slew = 0x3  <<  6;
		pdata->db_vneg_slew = val  <<  6;
	}

	return 0;
}

static int register_mtk_regulators(struct rt5081_pmu_dsv_data *dsv_data)
{
	int i, ret = 0;

	BUILD_BUG_ON(sizeof(dsv_data->mreg_desc)  != sizeof(mreg_desc_table));

	memcpy(dsv_data->mreg_desc, mreg_desc_table, sizeof(mreg_desc_table));
	for (i = 0; i < ARRAY_SIZE(mreg_desc_table); i++) {
		dsv_data->mreg_desc[i].client = dsv_data->chip;
		ret = mtk_simple_regulator_register(dsv_data->mreg_desc + i,
			dsv_data->dev, NULL, NULL);
		if (ret < 0) {
			dev_info(dsv_data->dev,
				"%s: failed to register mtk reg (%d)\n",
				__func__, ret);
			break;
		}
	}

	return ret;
}

static int dsv_apply_dts(struct rt5081_pmu_chip *chip,
	struct rt5081_pmu_dsv_platform_data *pdata,
	struct rt5081_pmu_dsv_platform_data *mask)
{
	int ret = 0;

	if (mask->db_ctrl1.raw)
		ret = rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_DBCTRL1,
			mask->db_ctrl1.raw, pdata->db_ctrl1.raw);
	if (ret < 0)
		return ret;
	if (mask->db_ctrl2.raw)
		ret = rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_DBCTRL2,
			mask->db_ctrl2.raw, pdata->db_ctrl2.raw);
	if (ret < 0)
		return ret;
	if (mask->db_vbst.raw)
		ret = rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_DBVBST,
			mask->db_vbst.raw, pdata->db_vbst.raw);
	if (ret < 0)
		return ret;
	if (mask->db_vpos_slew)
		ret = rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_DBVPOS,
			mask->db_vpos_slew, pdata->db_vpos_slew);
	if (ret < 0)
		return ret;
	if (mask->db_vneg_slew)
		ret = rt5081_pmu_reg_update_bits(chip, RT5081_PMU_REG_DBVNEG,
			mask->db_vneg_slew, pdata->db_vneg_slew);
	if (ret < 0)
		return ret;

	return ret;
}

static int rt5081_pmu_dsv_probe(struct platform_device *pdev)
{
	struct rt5081_pmu_dsv_data *dsv_data;
	bool use_dt = pdev->dev.of_node;
	struct rt5081_pmu_dsv_platform_data pdata, mask;
	int ret;

	dsv_data = devm_kzalloc(&pdev->dev, sizeof(*dsv_data), GFP_KERNEL);
	if (!dsv_data)
		return -ENOMEM;

	memset(&pdata, 0, sizeof(pdata));
	memset(&mask, 0, sizeof(mask));
	if (use_dt)
		rt_parse_dt(&pdev->dev, &pdata, &mask);
	dsv_data->chip = dev_get_drvdata(pdev->dev.parent);
	dsv_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, dsv_data);

	ret = dsv_apply_dts(dsv_data->chip, &pdata, &mask);
	if (ret < 0)
		goto mtk_reg_apply_dts_fail;
	ret = register_mtk_regulators(dsv_data);
	if (ret < 0)
		goto mtk_reg_register_fail;

	rt5081_pmu_dsv_irq_register(pdev);
	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return ret;
mtk_reg_apply_dts_fail:
mtk_reg_register_fail:
	dev_info(&pdev->dev, "%s failed\n", __func__);
	return ret;
}

static int rt5081_pmu_dsv_remove(struct platform_device *pdev)
{
	struct rt5081_pmu_dsv_data *dsv_data = platform_get_drvdata(pdev);

	dev_info(dsv_data->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct of_device_id rt_ofid_table[] = {
	{ .compatible = "richtek,rt5081_pmu_dsv", },
	{ },
};
MODULE_DEVICE_TABLE(of, rt_ofid_table);

static const struct platform_device_id rt_id_table[] = {
	{ "rt5081_pmu_dsv", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, rt_id_table);

static struct platform_driver rt5081_pmu_dsv = {
	.driver = {
		.name = "rt5081_pmu_dsv",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt_ofid_table),
	},
	.probe = rt5081_pmu_dsv_probe,
	.remove = rt5081_pmu_dsv_remove,
	.id_table = rt_id_table,
};
module_platform_driver(rt5081_pmu_dsv);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU DSV");
MODULE_VERSION("1.0.0_G");

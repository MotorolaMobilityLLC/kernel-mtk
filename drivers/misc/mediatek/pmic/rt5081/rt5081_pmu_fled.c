/*
 *  Copyright (C) 2016 Richtek Technology Corp.
 *  jeff_chang <jeff_chang@richtek.com>
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
#include "../../rt-flashlight/rtfled.h"

#include "inc/rt5081_pmu.h"
#include "inc/rt5081_pmu_fled.h"

#define RT5081_PMU_FLED_DRV_VERSION	"1.0.0_MTK"

enum {
	RT5081_FLED_CTRL_LED1,
	RT5081_FLED_CTRL_LED2,
	RT5081_FLED_CTRL_BOTH,
};

struct rt5081_pmu_fled_data {
	rt_fled_dev_t base;
	struct rt5081_pmu_chip *chip;
	struct device *dev;
	unsigned char suspend:1;
	unsigned char ext_ctrl:1;
	unsigned char fled_ctrl:2; /* fled1, fled2, both */
};

static irqreturn_t rt5081_pmu_fled_strbpin_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled_torpin_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled_tx_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled_lvf_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_fled_data *info = data;

	dev_err(info->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled2_short_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_fled_data *info = data;

	dev_err(info->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled1_short_irq_handler(int irq, void *data)
{
	struct rt5081_pmu_fled_data *info = data;

	dev_err(info->dev, "%s\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled2_strb_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled1_strb_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled2_strb_to_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled1_strb_to_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled2_tor_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static irqreturn_t rt5081_pmu_fled1_tor_irq_handler(int irq, void *data)
{
	return IRQ_HANDLED;
}

static struct rt5081_pmu_irq_desc rt5081_fled_irq_desc[] = {
	RT5081_PMU_IRQDESC(fled_strbpin),
	RT5081_PMU_IRQDESC(fled_torpin),
	RT5081_PMU_IRQDESC(fled_tx),
	RT5081_PMU_IRQDESC(fled_lvf),
	RT5081_PMU_IRQDESC(fled2_short),
	RT5081_PMU_IRQDESC(fled1_short),
	RT5081_PMU_IRQDESC(fled2_strb),
	RT5081_PMU_IRQDESC(fled1_strb),
	RT5081_PMU_IRQDESC(fled2_strb_to),
	RT5081_PMU_IRQDESC(fled1_strb_to),
	RT5081_PMU_IRQDESC(fled2_tor),
	RT5081_PMU_IRQDESC(fled1_tor),
};

static void rt5081_pmu_fled_irq_register(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(rt5081_fled_irq_desc); i++) {
		if (!rt5081_fled_irq_desc[i].name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				rt5081_fled_irq_desc[i].name);
		if (!res)
			continue;
		ret = devm_request_threaded_irq(&pdev->dev, res->start, NULL,
				rt5081_fled_irq_desc[i].irq_handler,
				IRQF_TRIGGER_FALLING,
				rt5081_fled_irq_desc[i].name,
				platform_get_drvdata(pdev));
		if (ret < 0) {
			dev_err(&pdev->dev, "request %s irq fail\n", res->name);
			continue;
		}
		rt5081_fled_irq_desc[i].irq = res->start;
	}
}

static inline int rt_parse_dt(struct device *dev,
				struct rt5081_pmu_fled_data *fi)
{
	struct device_node *np = dev->of_node;
	int ret = 0;
	u32 val = 0;
	unsigned char regval;

	ret = of_property_read_u32(np, "rt5081,fled-ctrl-sel", &val);
	if (ret < 0) {
		pr_err("%s use default RT5081_FLED_CTRL_BOTH\n", __func__);
		fi->fled_ctrl = RT5081_FLED_CTRL_BOTH;
	} else
		fi->fled_ctrl = val;

	fi->fled_ctrl = (fi->fled_ctrl > RT5081_FLED_CTRL_BOTH) ?
		RT5081_FLED_CTRL_BOTH : fi->fled_ctrl;

	pr_info("RT5081 control Fled (%d)\n", fi->fled_ctrl);

	ret = of_property_read_u32(np, "rt5081,fled-ext_ctrl", &val);
	if (ret < 0) {
		pr_err("%s use default i2c control\n", __func__);
		fi->ext_ctrl = 0;
	} else
		fi->ext_ctrl = val ? 1 : 0;
	pr_info("RT5081 Fled use %s control\n", fi->ext_ctrl ? "gpio" : "i2c");

	ret = of_property_read_u32(np, "rt5081,fled-default-torch-cur", &val);
	if (ret < 0)
		pr_err("%s use default torch cur\n", __func__);
	else {
		pr_err("%s use torch cur %d\n", __func__, val);
		regval = (val > 400000) ? 30 : (val - 25000)/12500;
		rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLED1TORCTRL,
				RT5081_FLED_TORCHCUR_MASK,
				regval << RT5081_FLED_TORCHCUR_SHIFT);
		rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLED2TORCTRL,
				RT5081_FLED_TORCHCUR_MASK,
				regval << RT5081_FLED_TORCHCUR_SHIFT);
	}

	ret = of_property_read_u32(np, "rt5091,fled-default-strobe-cur", &val);
	if (ret < 0)
		pr_err("%s use default strobe cur\n", __func__);
	else {
		pr_err("%s use strobe cur %d\n", __func__, val);
		regval = (val > 1500000) ? 112 : (val - 100000)/12500;
		rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLED1STRBCTRL,
				RT5081_FLED_STROBECUR_MASK,
				regval << RT5081_FLED_STROBECUR_SHIFT);
		rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLED2STRBCTRL2,
				RT5081_FLED_STROBECUR_MASK,
				regval << RT5081_FLED_STROBECUR_SHIFT);
	}

	ret = of_property_read_u32(np,
		"rt5081,fled-default-strb-timeout", &val);
	if (ret < 0)
		pr_err("%s use default strobe timeout\n", __func__);
	else {
		regval = (val > 2423) ? 74 : (val - 64)/32;
		rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLEDSTRBCTRL,
				RT5081_FLED_STROBE_TIMEOUT_MASK,
				regval << RT5081_FLED_STROBE_TIMEOUT_SHIFT);
	}
	return 0;
}

static struct platform_device rt_fled_pdev = {
	.name = "rt-flash-led",
	.id = -1,
};

static struct flashlight_properties rt5081_fled_props = {
	.type = FLASHLIGHT_TYPE_LED,
	.torch_brightness = 0,
	.torch_max_brightness = 31, /* 0000 ~ 1110 */
	.strobe_brightness = 0,
	.strobe_max_brightness = 113, /* 0000000 ~ 1110000 */
	.strobe_delay = 0,
	.strobe_timeout = 0,
	.alias_name = "rt5081-fled",
};

static int rt5081_fled_reset(struct rt5081_pmu_fled_data *info)
{
#if 0
	return rt5081_pmu_reg_update_bits(info->chip, info->,
			u8 mask, u8 data);
#endif
	return 0;
}

static int rt5081_fled_reg_init(struct rt5081_pmu_fled_data *info)
{
	/* TBD */
	return 0;
}

static int rt5081_fled_init(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_fled_reset(fi);
	if (ret < 0) {
		dev_err(fi->dev, "reset rt5081 fled fail\n");
		goto err_init;
	}
	ret = rt5081_fled_reg_init(fi);
	if (ret < 0)
		dev_err(fi->dev, "init rt5081 fled register fail\n");
err_init:
	return ret;
}

static int rt5081_fled_suspend(struct rt_fled_dev *info, pm_message_t state)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;

	fi->suspend = 1;
	return 0;
}

static int rt5081_fled_resume(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;

	fi->suspend = 0;
	return 0;
}

static int rt5081_fled_set_mode(struct rt_fled_dev *info,
					flashlight_mode_t mode)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	unsigned char strobe_mask;
	int ret = 0;

	switch (mode) {
	case FLASHLIGHT_MODE_TORCH:
		ret = rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_FLEDCS_EN_MASK);
		ret |= rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_STROBE_EN_MASK);
		ret |= rt5081_pmu_reg_set_bit(fi->chip,
				RT5081_PMU_REG_FLEDEN, RT5081_TORCH_EN_MASK);
		dev_info(fi->dev, "set to torch mode\n");
		break;
	case FLASHLIGHT_MODE_FLASH:
		if (fi->fled_ctrl == RT5081_FLED_CTRL_LED1)
			strobe_mask = RT5081_FLEDCS1_MASK;
		else if (fi->fled_ctrl == RT5081_FLED_CTRL_LED2)
			strobe_mask = RT5081_FLEDCS2_MASK;
		else
			strobe_mask = RT5081_FLEDCS1_MASK|RT5081_FLEDCS2_MASK;

		ret = rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_TORCH_EN_MASK);
		ret |= rt5081_pmu_reg_set_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_STROBE_EN_MASK);

		ret |= rt5081_pmu_reg_update_bits(fi->chip,
			RT5081_PMU_REG_FLEDEN,
			RT5081_FLEDCS_EN_MASK, strobe_mask);

		dev_info(fi->dev, "set to flash mode\n");
		break;
	case FLASHLIGHT_MODE_OFF:

		ret = rt5081_pmu_reg_update_bits(fi->chip,
				RT5081_PMU_REG_FLEDEN, 0x0f, 0);
#if 0
		ret = rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_FLEDCS_EN_MASK);
		ret |= rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_TORCH_EN_MASK);
		ret |= rt5081_pmu_reg_clr_bit(fi->chip,
			RT5081_PMU_REG_FLEDEN, RT5081_STROBE_EN_MASK);
#endif
		dev_info(fi->dev, "set to off mode\n");
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int rt5081_fled_get_mode(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret;


	ret = rt5081_pmu_reg_read(fi->chip, RT5081_PMU_REG_FLEDEN);
	if (ret < 0)
		return -EINVAL;

	if (ret & RT5081_STROBE_EN_MASK)
		return FLASHLIGHT_MODE_FLASH;
	else if (ret & RT5081_TORCH_EN_MASK)
		return FLASHLIGHT_MODE_TORCH;
	else
		return FLASHLIGHT_MODE_OFF;
}

static int rt5081_fled_strobe(struct rt_fled_dev *info)
{
	return 0;
}

static int rt5081_fled_torch_current_list(
			struct rt_fled_dev *info, int selector)
{

	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;

	return (selector > fi->base.init_props->torch_max_brightness) ?
		-EINVAL : 25000 + selector * 12500;
}

static int rt5081_fled_strobe_current_list(struct rt_fled_dev *info,
							int selector)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;

	return (selector > fi->base.init_props->strobe_max_brightness) ?
		-EINVAL : 100000 + selector * 12500;
}

static unsigned int rt5081_timeout_level_list[] = {
	50000, 75000, 100000, 125000, 150000, 175000, 200000, 200000,
};

static int rt5081_fled_timeout_level_list(struct rt_fled_dev *info,
							int selector)
{
	return (selector > ARRAY_SIZE(rt5081_timeout_level_list)) ?
		-EINVAL : rt5081_timeout_level_list[selector];
}

static int rt5081_fled_strobe_timeout_list(struct rt_fled_dev *info,
							int selector)
{
	if (selector > 74)
		return 2432;
	return 64 + selector * 32;
}

static int rt5081_fled_set_torch_current_sel(struct rt_fled_dev *info,
								int selector)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret;

	ret = rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLED1TORCTRL,
			RT5081_FLED_TORCHCUR_MASK,
			selector << RT5081_FLED_TORCHCUR_SHIFT);
	ret |= rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLED2TORCTRL,
			RT5081_FLED_TORCHCUR_MASK,
			selector << RT5081_FLED_TORCHCUR_SHIFT);

	return ret;
}

static int rt5081_fled_set_strobe_current_sel(struct rt_fled_dev *info,
								int selector)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret;

	ret = rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLED1STRBCTRL,
			RT5081_FLED_STROBECUR_MASK,
			selector << RT5081_FLED_STROBECUR_SHIFT);
	ret |= rt5081_pmu_reg_update_bits(fi->chip,
			RT5081_PMU_REG_FLED2STRBCTRL2,
			RT5081_FLED_STROBECUR_MASK,
			selector << RT5081_FLED_STROBECUR_SHIFT);

	return ret;
}

static int rt5081_fled_set_timeout_level_sel(struct rt_fled_dev *info,
								int selector)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret;

	ret = rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLED1CTRL,
		RT5081_FLED_TIMEOUT_LEVEL_MASK,
		selector << RT5081_TIMEOUT_LEVEL_SHIFT);

	ret |= rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLED2CTRL,
		RT5081_FLED_TIMEOUT_LEVEL_MASK,
		selector << RT5081_TIMEOUT_LEVEL_SHIFT);

	return ret;
}

static int rt5081_fled_set_strobe_timeout_sel(struct rt_fled_dev *info,
							int selector)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_pmu_reg_update_bits(fi->chip, RT5081_PMU_REG_FLEDSTRBCTRL,
			RT5081_FLED_STROBE_TIMEOUT_MASK,
			selector << RT5081_FLED_STROBE_TIMEOUT_SHIFT);
	return ret;
}

static int rt5081_fled_get_torch_current_sel(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_pmu_reg_read(fi->chip, RT5081_PMU_REG_FLED1TORCTRL);
	if (ret < 0) {
		pr_err("%s get fled tor current sel fail\n", __func__);
		return ret;
	}

	ret &= RT5081_FLED_TORCHCUR_MASK;
	ret >>= RT5081_FLED_TORCHCUR_SHIFT;
	return ret;
}

static int rt5081_fled_get_strobe_current_sel(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_pmu_reg_read(fi->chip, RT5081_PMU_REG_FLED1STRBCTRL);
	if (ret < 0) {
		pr_err("%s get fled strobe curr sel fail\n", __func__);
		return ret;
	}

	ret &= RT5081_FLED_STROBECUR_MASK;
	ret >>= RT5081_FLED_STROBECUR_SHIFT;
	return ret;
}

static int rt5081_fled_get_timeout_level_sel(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_pmu_reg_read(fi->chip, RT5081_PMU_REG_FLED1CTRL);
	if (ret < 0) {
		pr_err("%s get fled timeout level fail\n", __func__);
		return ret;
	}

	ret &= RT5081_FLED_TIMEOUT_LEVEL_MASK;
	ret >>= RT5081_TIMEOUT_LEVEL_SHIFT;
	return ret;
}

static int rt5081_fled_get_strobe_timeout_sel(struct rt_fled_dev *info)
{
	struct rt5081_pmu_fled_data *fi = (struct rt5081_pmu_fled_data *)info;
	int ret = 0;

	ret = rt5081_pmu_reg_read(fi->chip, RT5081_PMU_REG_FLEDSTRBCTRL);
	if (ret < 0) {
		pr_err("%s get fled timeout level fail\n", __func__);
		return ret;
	}

	ret &= RT5081_FLED_STROBE_TIMEOUT_MASK;
	ret >>= RT5081_FLED_STROBE_TIMEOUT_SHIFT;
	return ret;
}

static void rt5081_fled_shutdown(struct rt_fled_dev *info)
{
	rt5081_fled_set_mode(info, FLASHLIGHT_MODE_OFF);
}

static struct rt_fled_hal rt5081_fled_hal = {
	.fled_init = rt5081_fled_init,
	.fled_suspend = rt5081_fled_suspend,
	.fled_resume = rt5081_fled_resume,
	.fled_set_mode = rt5081_fled_set_mode,
	.fled_get_mode = rt5081_fled_get_mode,
	.fled_strobe = rt5081_fled_strobe,
	.fled_troch_current_list = rt5081_fled_torch_current_list,
	.fled_strobe_current_list = rt5081_fled_strobe_current_list,
	.fled_timeout_level_list = rt5081_fled_timeout_level_list,
	/* .fled_lv_protection_list = rt5081_fled_lv_protection_list, */
	.fled_strobe_timeout_list = rt5081_fled_strobe_timeout_list,
	/* method to set */
	.fled_set_torch_current_sel = rt5081_fled_set_torch_current_sel,
	.fled_set_strobe_current_sel = rt5081_fled_set_strobe_current_sel,
	.fled_set_timeout_level_sel = rt5081_fled_set_timeout_level_sel,

	.fled_set_strobe_timeout_sel = rt5081_fled_set_strobe_timeout_sel,

	/* method to get */
	.fled_get_torch_current_sel = rt5081_fled_get_torch_current_sel,
	.fled_get_strobe_current_sel = rt5081_fled_get_strobe_current_sel,
	.fled_get_timeout_level_sel = rt5081_fled_get_timeout_level_sel,
	.fled_get_strobe_timeout_sel = rt5081_fled_get_strobe_timeout_sel,
	/* PM shutdown, optional */
	.fled_shutdown = rt5081_fled_shutdown,
};

static int rt5081_pmu_fled_probe(struct platform_device *pdev)
{
	struct rt5081_pmu_fled_data *fled_data;
	bool use_dt = pdev->dev.of_node;
	int ret;

	pr_info("%s (%s)\n", __func__, RT5081_PMU_FLED_DRV_VERSION);
	fled_data = devm_kzalloc(&pdev->dev, sizeof(*fled_data), GFP_KERNEL);
	if (!fled_data)
		return -ENOMEM;
	if (use_dt)
		rt_parse_dt(&pdev->dev, fled_data);
	fled_data->chip = dev_get_drvdata(pdev->dev.parent);
	fled_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, fled_data);
	rt_fled_pdev.dev.parent = &(pdev->dev);

	fled_data->base.init_props = &rt5081_fled_props;
	fled_data->base.hal = &rt5081_fled_hal;

	ret = platform_device_register(&rt_fled_pdev);
	if (ret < 0)
		goto err_register_pdev;

	rt5081_pmu_fled_irq_register(pdev);
	dev_info(&pdev->dev, "%s successfully\n", __func__);
	return 0;
err_register_pdev:
	return ret;
}

static int rt5081_pmu_fled_remove(struct platform_device *pdev)
{
	struct rt5081_pmu_fled_data *fled_data = platform_get_drvdata(pdev);

	dev_info(fled_data->dev, "%s successfully\n", __func__);
	return 0;
}

static const struct of_device_id rt_ofid_table[] = {
	{ .compatible = "richtek,rt5081_pmu_fled", },
	{ },
};
MODULE_DEVICE_TABLE(of, rt_ofid_table);

static const struct platform_device_id rt_id_table[] = {
	{ "rt5081_pmu_fled", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, rt_id_table);

static struct platform_driver rt5081_pmu_fled = {
	.driver = {
		.name = "rt5081_pmu_fled",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt_ofid_table),
	},
	.probe = rt5081_pmu_fled_probe,
	.remove = rt5081_pmu_fled_remove,
	.id_table = rt_id_table,
};
module_platform_driver(rt5081_pmu_fled);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jeff_chang <jeff_chang@richtek.com>");
MODULE_DESCRIPTION("Richtek RT5081 PMU Fled");
MODULE_VERSION(RT5081_PMU_FLED_DRV_VERSION);

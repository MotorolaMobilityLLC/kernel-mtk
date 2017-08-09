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

static void mt6323_irq_lock(struct irq_data *data)
{
	struct mt6323_chip *mt6323 = irq_data_get_irq_chip_data(data);

	mutex_lock(&mt6323->irqlock);
}

static void mt6323_irq_sync_unlock(struct irq_data *data)
{
	struct mt6323_chip *mt6323 = irq_data_get_irq_chip_data(data);

	regmap_write(mt6323->regmap, MT6323_INT_CON0, mt6323->irq_masks_cur[0]);
	regmap_write(mt6323->regmap, MT6323_INT_CON1, mt6323->irq_masks_cur[1]);

	mutex_unlock(&mt6323->irqlock);
}

static void mt6323_irq_disable(struct irq_data *data)
{
	struct mt6323_chip *mt6323 = irq_data_get_irq_chip_data(data);
	int shift = data->hwirq & 0xf;
	int reg = data->hwirq >> 4;

	mt6323->irq_masks_cur[reg] &= ~BIT(shift);
}

static void mt6323_irq_enable(struct irq_data *data)
{
	struct mt6323_chip *mt6323 = irq_data_get_irq_chip_data(data);
	int shift = data->hwirq & 0xf;
	int reg = data->hwirq >> 4;

	mt6323->irq_masks_cur[reg] |= BIT(shift);
}

#ifdef CONFIG_PM_SLEEP
static int mt6323_irq_set_wake(struct irq_data *irq_data, unsigned int on)
{
	struct mt6323_chip *mt6323 = irq_data_get_irq_chip_data(irq_data);
	int shift = irq_data->hwirq & 0xf;
	int reg = irq_data->hwirq >> 4;

	if (on)
		mt6323->wake_mask[reg] |= BIT(shift);
	else
		mt6323->wake_mask[reg] &= ~BIT(shift);

	return 0;
}
#else
#define mt6323_irq_set_wake NULL
#endif

static struct irq_chip mt6323_irq_chip = {
	.name = "mt6323-irq",
	.irq_bus_lock = mt6323_irq_lock,
	.irq_bus_sync_unlock = mt6323_irq_sync_unlock,
	.irq_enable = mt6323_irq_enable,
	.irq_disable = mt6323_irq_disable,
	.irq_set_wake = mt6323_irq_set_wake,
};

static void mt6323_irq_handle_reg(struct mt6323_chip *mt6323, int reg,
		int irqbase)
{
	unsigned int status;
	int i, irq, ret;

	ret = regmap_read(mt6323->regmap, reg, &status);
	if (ret) {
		dev_err(mt6323->dev, "Failed to read irq status: %d\n", ret);
		return;
	}

	for (i = 0; i < 16; i++) {
		if (status & BIT(i)) {
			irq = irq_find_mapping(mt6323->irq_domain, irqbase + i);
			if (irq)
				handle_nested_irq(irq);
		}
	}

	regmap_write(mt6323->regmap, reg, status);
}

static irqreturn_t mt6323_irq_thread(int irq, void *data)
{
	struct mt6323_chip *mt6323 = data;

	mt6323_irq_handle_reg(mt6323, MT6323_INT_STATUS0, 0);
	mt6323_irq_handle_reg(mt6323, MT6323_INT_STATUS1, 16);

	return IRQ_HANDLED;
}

static int mt6323_irq_domain_map(struct irq_domain *d, unsigned int irq,
					irq_hw_number_t hw)
{
	struct mt6323_chip *mt6323 = d->host_data;

	irq_set_chip_data(irq, mt6323);
	irq_set_chip_and_handler(irq, &mt6323_irq_chip, handle_level_irq);
	irq_set_nested_thread(irq, 1);
	irq_set_noprobe(irq);

	return 0;
}

static const struct irq_domain_ops mt6323_irq_domain_ops = {
	.map = mt6323_irq_domain_map,
};

static int mt6323_irq_init(struct mt6323_chip *mt6323)
{
	int ret;

	mutex_init(&mt6323->irqlock);

	/* Mask all interrupt sources */
	regmap_write(mt6323->regmap, MT6323_INT_CON0, 0x0);
	regmap_write(mt6323->regmap, MT6323_INT_CON1, 0x0);

	mt6323->irq_domain = irq_domain_add_linear(mt6323->dev->of_node,
		MT6323_IRQ_NR, &mt6323_irq_domain_ops, mt6323);
	if (!mt6323->irq_domain) {
		dev_err(mt6323->dev, "could not create irq domain\n");
		return -ENOMEM;
	}

	ret = devm_request_threaded_irq(mt6323->dev, mt6323->irq, NULL,
		mt6323_irq_thread, IRQF_ONESHOT, "mt6323-pmic", mt6323);
	if (ret) {
		dev_err(mt6323->dev, "failed to register irq=%d; err: %d\n",
			mt6323->irq, ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mt6323_irq_suspend(struct device *dev)
{
	struct mt6323_chip *chip = dev_get_drvdata(dev);

	regmap_write(chip->regmap, MT6323_INT_CON0, chip->wake_mask[0]);
	regmap_write(chip->regmap, MT6323_INT_CON1, chip->wake_mask[1]);

	enable_irq_wake(chip->irq);

	return 0;
}

static int mt6323_irq_resume(struct device *dev)
{
	struct mt6323_chip *chip = dev_get_drvdata(dev);

	regmap_write(chip->regmap, MT6323_INT_CON0, chip->irq_masks_cur[0]);
	regmap_write(chip->regmap, MT6323_INT_CON1, chip->irq_masks_cur[1]);

	disable_irq_wake(chip->irq);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mt6323_pm_ops, mt6323_irq_suspend,
			mt6323_irq_resume);

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

	mt6323->irq = platform_get_irq(pdev, 0);
	if (mt6323->irq > 0) {
		ret = mt6323_irq_init(mt6323);
		if (ret)
			return ret;
	}

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
		.pm = &mt6323_pm_ops,
	},
};

module_platform_driver(mt6323_driver);

MODULE_AUTHOR("Flora Fu, MediaTek");
MODULE_DESCRIPTION("Driver for MediaTek MT6323 PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:mt6323");

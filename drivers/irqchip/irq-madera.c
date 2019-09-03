/*
 * Interrupt support for Cirrus Logic Madera codecs
 *
 * Copyright 2016-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/irqchip/irq-madera.h>
#include <linux/irqchip/irq-madera-pdata.h>
#include <linux/mfd/madera/core.h>
#include <linux/mfd/madera/pdata.h>
#include <linux/mfd/madera/registers.h>

struct madera_irq_priv {
	struct device			*dev;
	int				irq;
	struct regmap_irq_chip_data	*irq_data;
	struct madera			*madera;
};

#define MADERA_IRQ(_irq, _reg)						     \
	[MADERA_IRQ_ ## _irq] = { .reg_offset = (_reg) - MADERA_IRQ1_STATUS_1, \
				.mask = MADERA_ ## _irq ## _EINT1 }

static const struct regmap_irq madera_irqs[MADERA_NUM_IRQ] = {
	MADERA_IRQ(FLL1_LOCK,		MADERA_IRQ1_STATUS_2),
	MADERA_IRQ(FLL2_LOCK,		MADERA_IRQ1_STATUS_2),
	MADERA_IRQ(FLL3_LOCK,		MADERA_IRQ1_STATUS_2),
	MADERA_IRQ(FLLAO_LOCK,		MADERA_IRQ1_STATUS_2),

	MADERA_IRQ(MICDET1,		MADERA_IRQ1_STATUS_6),
	MADERA_IRQ(MICDET2,		MADERA_IRQ1_STATUS_6),
	MADERA_IRQ(HPDET,		MADERA_IRQ1_STATUS_6),

	MADERA_IRQ(MICD_CLAMP_RISE,	MADERA_IRQ1_STATUS_7),
	MADERA_IRQ(MICD_CLAMP_FALL,	MADERA_IRQ1_STATUS_7),
	MADERA_IRQ(JD1_RISE,		MADERA_IRQ1_STATUS_7),
	MADERA_IRQ(JD1_FALL,		MADERA_IRQ1_STATUS_7),

	MADERA_IRQ(ASRC2_IN1_LOCK,	MADERA_IRQ1_STATUS_9),
	MADERA_IRQ(ASRC2_IN2_LOCK,	MADERA_IRQ1_STATUS_9),
	MADERA_IRQ(ASRC1_IN1_LOCK,	MADERA_IRQ1_STATUS_9),
	MADERA_IRQ(ASRC1_IN2_LOCK,	MADERA_IRQ1_STATUS_9),
	MADERA_IRQ(DRC2_SIG_DET,	MADERA_IRQ1_STATUS_9),
	MADERA_IRQ(DRC1_SIG_DET,	MADERA_IRQ1_STATUS_9),

	MADERA_IRQ(DSP_IRQ1,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ2,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ3,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ4,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ5,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ6,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ7,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ8,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ9,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ10,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ11,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ12,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ13,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ14,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ15,		MADERA_IRQ1_STATUS_11),
	MADERA_IRQ(DSP_IRQ16,		MADERA_IRQ1_STATUS_11),

	MADERA_IRQ(HP3R_SC,		MADERA_IRQ1_STATUS_12),
	MADERA_IRQ(HP3L_SC,		MADERA_IRQ1_STATUS_12),
	MADERA_IRQ(HP2R_SC,		MADERA_IRQ1_STATUS_12),
	MADERA_IRQ(HP2L_SC,		MADERA_IRQ1_STATUS_12),
	MADERA_IRQ(HP1R_SC,		MADERA_IRQ1_STATUS_12),
	MADERA_IRQ(HP1L_SC,		MADERA_IRQ1_STATUS_12),

	MADERA_IRQ(SPK_OVERHEAT_WARN,	MADERA_IRQ1_STATUS_15),
	MADERA_IRQ(SPK_OVERHEAT,	MADERA_IRQ1_STATUS_15),

	MADERA_IRQ(DSP1_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP2_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP3_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP4_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP5_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP6_BUS_ERR,	MADERA_IRQ1_STATUS_33),
	MADERA_IRQ(DSP7_BUS_ERR,	MADERA_IRQ1_STATUS_33),

	MADERA_IRQ(BOOT_DONE,		MADERA_IRQ1_STATUS_1),
};

static const struct regmap_irq_chip madera_irq = {
	.name		= "madera IRQ",
	.status_base	= MADERA_IRQ1_STATUS_1,
	.mask_base	= MADERA_IRQ1_MASK_1,
	.ack_base	= MADERA_IRQ1_STATUS_1,
	.runtime_pm	= true,
	.num_regs	= 33,
	.irqs		= madera_irqs,
	.num_irqs	= ARRAY_SIZE(madera_irqs),
};

static int madera_map_irq(struct madera *madera, int irq)
{
	struct madera_irq_priv *priv;

	if (!madera->irq_dev)
		return -ENOENT;

	priv = dev_get_drvdata(madera->irq_dev);

	return regmap_irq_get_virq(priv->irq_data, irq);
}

int madera_request_irq(struct madera *madera, int irq, const char *name,
			irq_handler_t handler, void *data)
{
	irq = madera_map_irq(madera, irq);

	if (irq < 0)
		return irq;

	return request_threaded_irq(irq, NULL, handler, IRQF_ONESHOT, name,
				    data);

}
EXPORT_SYMBOL_GPL(madera_request_irq);

void madera_free_irq(struct madera *madera, int irq, void *data)
{
	irq = madera_map_irq(madera, irq);

	if (irq < 0)
		return;

	free_irq(irq, data);
}
EXPORT_SYMBOL_GPL(madera_free_irq);

int madera_set_irq_wake(struct madera *madera, int irq, int on)
{
	irq = madera_map_irq(madera, irq);

	if (irq < 0)
		return irq;

	return irq_set_irq_wake(irq, on);
}
EXPORT_SYMBOL_GPL(madera_set_irq_wake);

#ifdef CONFIG_PM_SLEEP
static int madera_suspend_noirq(struct device *dev)
{
	struct madera_irq_priv *priv = dev_get_drvdata(dev);

	dev_dbg(priv->dev, "No IRQ suspend, reenabling IRQ\n");

	enable_irq(priv->irq);

	return 0;
}

static int madera_suspend(struct device *dev)
{
	struct madera_irq_priv *priv = dev_get_drvdata(dev);

	dev_dbg(priv->dev, "Suspend, disabling IRQ\n");

	disable_irq(priv->irq);

	return 0;
}

static int madera_resume_noirq(struct device *dev)
{
	struct madera_irq_priv *priv = dev_get_drvdata(dev);

	dev_dbg(priv->dev, "No IRQ resume, disabling IRQ\n");

	disable_irq(priv->irq);

	return 0;
}

static int madera_resume(struct device *dev)
{
	struct madera_irq_priv *priv = dev_get_drvdata(dev);

	dev_dbg(priv->dev, "Resume, reenabling IRQ\n");

	enable_irq(priv->irq);

	return 0;
}
#endif

static irqreturn_t madera_boot_done(int irq, void *data)
{
	struct madera *madera = data;

	dev_warn(madera->dev, "BOOT_DONE\n");

	return IRQ_HANDLED;
}

static const struct dev_pm_ops madera_irq_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(madera_suspend, madera_resume)
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(madera_suspend_noirq,
				      madera_resume_noirq)
};

static int madera_irq_probe(struct platform_device *pdev)
{
	struct madera *madera = dev_get_drvdata(pdev->dev.parent);
	struct madera_irq_priv *priv;
	struct irq_data *irq_data;
	unsigned int irq_flags = madera->pdata.irqchip.irq_flags;
	int ret;

	dev_dbg(&pdev->dev, "probe\n");

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	priv->madera = madera;
	priv->irq = madera->irq;

	/* Read the flags from the interrupt controller if not specified */
	if (!irq_flags) {
		irq_data = irq_get_irq_data(priv->irq);
		if (!irq_data) {
			dev_err(priv->dev, "Invalid IRQ: %d\n", priv->irq);
			return -EINVAL;
		}

		irq_flags = irqd_get_trigger_type(irq_data);

		/* Codec defaults to trigger low, use this if no flags given */
		if (irq_flags == IRQ_TYPE_NONE)
			irq_flags = IRQF_TRIGGER_LOW;
	}

	if (irq_flags & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
		dev_err(priv->dev, "Host interrupt not level-triggered\n");
		return -EINVAL;
	}

	if (irq_flags & IRQF_TRIGGER_HIGH) {
		ret = regmap_update_bits(madera->regmap, MADERA_IRQ1_CTRL,
					 MADERA_IRQ_POL_MASK, 0);
		if (ret) {
			dev_err(priv->dev,
				"Failed to set IRQ polarity: %d\n", ret);
			return ret;
		}
	}

	/*
	 * NOTE: regmap registers this against the OF node of the parent of
	 * the regmap - that is, against the mfd driver
	 */
	ret = regmap_add_irq_chip(madera->regmap, priv->irq, IRQF_ONESHOT, 0,
				  &madera_irq, &priv->irq_data);
	if (ret) {
		dev_err(priv->dev, "add_irq_chip failed: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, priv);
	madera->irq_dev = priv->dev;

	madera_request_irq(madera, MADERA_IRQ_BOOT_DONE, "BOOT_DONE",
			  madera_boot_done, madera);

	return 0;
}

static int madera_irq_remove(struct platform_device *pdev)
{
	struct madera_irq_priv *priv = platform_get_drvdata(pdev);

	/*
	 * The IRQ is disabled by the parent MFD driver before
	 * it starts cleaning up all child drivers
	 */

	madera_free_irq(priv->madera, MADERA_IRQ_BOOT_DONE, priv->madera);

	priv->madera->irq_dev = NULL;

	regmap_del_irq_chip(priv->irq, priv->irq_data);

	return 0;
}

static struct platform_driver madera_irq_driver = {
	.probe	= madera_irq_probe,
	.remove = madera_irq_remove,
	.driver = {
		.name	= "madera-irq",
		.pm	= &madera_irq_pm_ops,
	}
};
module_platform_driver(madera_irq_driver);

MODULE_DESCRIPTION("Madera IRQ driver");
MODULE_AUTHOR("Richard Fitzgerald <rf@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL v2");

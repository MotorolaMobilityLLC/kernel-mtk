/*
 * GPIO support for Cirrus Logic Madera codecs
 *
 * Copyright 2015-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/mfd/madera/core.h>
#include <linux/mfd/madera/pdata.h>
#include <linux/mfd/madera/registers.h>

struct madera_gpio {
	struct madera *madera;
	struct gpio_chip gpio_chip;
};

static int madera_gpio_get_direction(struct gpio_chip *chip,
				     unsigned int offset)
{
	struct madera_gpio *madera_gpio = gpiochip_get_data(chip);
	struct madera *madera = madera_gpio->madera;
	unsigned int val;
	int ret;

	ret = regmap_read(madera->regmap,
			  MADERA_GPIO1_CTRL_2 + (2 * offset), &val);
	if (ret < 0)
		return ret;

	return (val & MADERA_GP1_DIR_MASK) >> MADERA_GP1_DIR_SHIFT;
}

static int madera_gpio_direction_in(struct gpio_chip *chip, unsigned int offset)
{
	struct madera_gpio *madera_gpio = gpiochip_get_data(chip);
	struct madera *madera = madera_gpio->madera;

	return regmap_update_bits(madera->regmap,
				  MADERA_GPIO1_CTRL_2 + (2 * offset),
				  MADERA_GP1_DIR_MASK, MADERA_GP1_DIR);
}

static int madera_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	struct madera_gpio *madera_gpio = gpiochip_get_data(chip);
	struct madera *madera = madera_gpio->madera;
	unsigned int val;
	int ret;

	ret = regmap_read(madera->regmap,
			  MADERA_GPIO1_CTRL_1 + (2 * offset), &val);
	if (ret < 0)
		return ret;

	return !!(val & MADERA_GP1_LVL_MASK);
}

static int madera_gpio_direction_out(struct gpio_chip *chip,
				     unsigned int offset, int value)
{
	struct madera_gpio *madera_gpio = gpiochip_get_data(chip);
	struct madera *madera = madera_gpio->madera;
	unsigned int regval;
	int ret;

	if (value)
		regval = MADERA_GP1_LVL;
	else
		regval = 0;

	ret = regmap_update_bits(madera->regmap,
				 MADERA_GPIO1_CTRL_2 + (2 * offset),
				 MADERA_GP1_DIR_MASK, 0);
	if (ret < 0)
		return ret;

	return regmap_update_bits(madera->regmap,
				  MADERA_GPIO1_CTRL_1 + (2 * offset),
				  MADERA_GP1_LVL_MASK, regval);
}

static void madera_gpio_set(struct gpio_chip *chip, unsigned int offset,
			    int value)
{
	struct madera_gpio *madera_gpio = gpiochip_get_data(chip);
	struct madera *madera = madera_gpio->madera;
	unsigned int regval;
	int ret;

	if (value)
		regval = MADERA_GP1_LVL;
	else
		regval = 0;

	ret = regmap_update_bits(madera->regmap,
				 MADERA_GPIO1_CTRL_1 + (2 * offset),
				 MADERA_GP1_LVL_MASK, regval);
	if (ret)
		dev_warn(madera->dev, "Failed to write to 0x%x (%d)\n",
			 MADERA_GPIO1_CTRL_1 + (2 * offset), ret);
}

static struct gpio_chip template_chip = {
	.label			= "madera",
	.owner			= THIS_MODULE,
	.get_direction		= madera_gpio_get_direction,
	.direction_input	= madera_gpio_direction_in,
	.get			= madera_gpio_get,
	.direction_output	= madera_gpio_direction_out,
	.set			= madera_gpio_set,
	.can_sleep		= true,
};

static int madera_gpio_probe(struct platform_device *pdev)
{
	struct madera *madera = dev_get_drvdata(pdev->dev.parent);
	struct madera_pdata *pdata = dev_get_platdata(madera->dev);
	struct madera_gpio *madera_gpio;
	int ret;

	madera_gpio = devm_kzalloc(&pdev->dev, sizeof(*madera_gpio),
				   GFP_KERNEL);
	if (!madera_gpio)
		return -ENOMEM;

	madera_gpio->madera = madera;
	madera_gpio->gpio_chip = template_chip;
	madera_gpio->gpio_chip.parent = &pdev->dev;

	if (IS_ENABLED(CONFIG_OF_GPIO))
		madera_gpio->gpio_chip.of_node = madera->dev->of_node;

	switch (madera->type) {
	case CS47L15:
		madera_gpio->gpio_chip.ngpio = CS47L15_NUM_GPIOS;
		break;
	case CS47L35:
		madera_gpio->gpio_chip.ngpio = CS47L35_NUM_GPIOS;
		break;
	case CS47L85:
	case WM1840:
		madera_gpio->gpio_chip.ngpio = CS47L85_NUM_GPIOS;
		break;
	case CS47L90:
	case CS47L91:
		madera_gpio->gpio_chip.ngpio = CS47L90_NUM_GPIOS;
		break;
	case CS47L92:
	case CS47L93:
		madera_gpio->gpio_chip.ngpio = CS47L92_NUM_GPIOS;
		break;
	default:
		dev_err(&pdev->dev, "Unknown chip variant %d\n", madera->type);
		return -EINVAL;
	}

	if (pdata && pdata->gpio_base)
		madera_gpio->gpio_chip.base = pdata->gpio_base;
	else
		madera_gpio->gpio_chip.base = -1;

	ret = devm_gpiochip_add_data(&pdev->dev, &madera_gpio->gpio_chip,
				     madera_gpio);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register gpiochip, %d\n", ret);
		return ret;
	}

	return 0;
}

static struct platform_driver madera_gpio_driver = {
	.driver.name	= "madera-gpio",
	.driver.owner	= THIS_MODULE,
	.probe		= madera_gpio_probe,
};

module_platform_driver(madera_gpio_driver);

MODULE_DESCRIPTION("GPIO interface for Madera codecs");
MODULE_AUTHOR("Nariman Poushin <nariman@opensource.wolfsonmicro.com>");
MODULE_AUTHOR("Richard Fitzgerald <rf@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:madera-gpio");

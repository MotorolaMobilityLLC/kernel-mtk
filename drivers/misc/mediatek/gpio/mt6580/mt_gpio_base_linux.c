/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mfd/core.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>

#include "mt_gpio_base.h"
#include <mt-plat/mt_gpio_core.h>
/*#include <mach/eint.h>*/
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/of_gpio.h>
#include <linux/idr.h>
#include <linux/irqchip/mt-eic.h>

static const signed int pin_eint_map[MT_GPIO_BASE_MAX] = {

};

static int mtk_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return mt_set_gpio_mode_base(offset, 0);
}

static int mtk_get_gpio_direction(struct gpio_chip *chip, unsigned offset)
{
	return 1 - mt_get_gpio_dir_base(offset);
}

static int mtk_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	return mt_set_gpio_dir_base(offset, 0);
}

static int mtk_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	int status = mt_get_gpio_dir_base(offset);

	if (status == 0)
		return mt_get_gpio_in_base(offset);
	else if (status == 1)
		return mt_get_gpio_out_base(offset);
	return 1;
}

static int mtk_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	return mt_set_gpio_dir_base(offset, 1);
}

static void mtk_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	mt_set_gpio_out_base(offset, value);
}

static int mtk_gpio_to_irq(struct gpio_chip *chip, unsigned pin)
{
	return mt_gpio_to_irq(pin);
	return 0;
}

static int mtk_gpio_set_debounce(struct gpio_chip *chip, unsigned offset, unsigned debounce)
{
	/* mt_eint_set_hw_debounce(offset, debounce); */
	return mt_gpio_set_debounce(offset, debounce);
	/* return 0; */
}

static struct gpio_chip mtk_gpio_chip = {
	.label = "mtk-gpio",
	.request = mtk_gpio_request,
	.get_direction = mtk_get_gpio_direction,
	.direction_input = mtk_gpio_direction_input,
	.get = mtk_gpio_get,
	.direction_output = mtk_gpio_direction_output,
	.set = mtk_gpio_set,
	.base = MT_GPIO_BASE_START,
	.ngpio = MT_GPIO_BASE_MAX,
	.to_irq = mtk_gpio_to_irq,
	.set_debounce = mtk_gpio_set_debounce,
};

static int __init mtk_gpio_init(void)
{
	return gpiochip_add(&mtk_gpio_chip);
}
postcore_initcall(mtk_gpio_init);

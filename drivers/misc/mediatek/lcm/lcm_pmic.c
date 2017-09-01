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

#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#ifdef CONFIG_RT5081A_PMU_DSV
static int regulator_inited;

#ifdef CONFIG_RT5081A_PMU_DSV_EXTPIN
static int ext_gpio_pin;
#else
static struct regulator *disp_bias_pos;
static struct regulator *disp_bias_neg;
#endif /* CONFIG_RT5081A_PMU_DSV_EXTPIN */

int display_bias_regulator_init(void)
{
#ifdef CONFIG_RT5081A_PMU_DSV_EXTPIN
	struct device_node *np = NULL;
	struct device_node *dsv_np = NULL;
	int ret = 0;

	if (regulator_inited)
		return ret;

	np = of_find_node_by_name(NULL, "rt5081a_pmu_dts");
	if (np)
		dsv_np = of_get_child_by_name(np, "dsv");
	if (dsv_np) {
#if (!defined(CONFIG_MTK_GPIO) || defined(CONFIG_MTK_GPIOLIB_STAND))
		ret = of_get_named_gpio(dsv_np, "extpin_gpio", 0);
		if (ret < 0)
			goto lcm_regu_init_err;
		ext_gpio_pin = ret;
#else
		ret = of_property_read_u32(dsv_np, "extpin_gpio_num", &ext_gpio_pin);
		if (ret < 0)
			goto lcm_regu_init_err;
#endif /* CONFIG_MTK_GPIO, CONFIG_MTK_GPIOLIB_STAND */
		ret = gpio_request_one(ext_gpio_pin, GPIOF_OUT_INIT_HIGH, "mtk_dsv_extpin");
		if (ret < 0) {
			pr_err("%s gpio request fail\n", __func__);
			goto lcm_regu_init_err;
		}
	} else {
		pr_err("%s no dsv of node\n", __func__);
		goto lcm_regu_init_err;
	}

	regulator_inited = 1;
	pr_info("%s: success, ext_gpio_pin = %d\n", __func__, ext_gpio_pin);

	return 0;

lcm_regu_init_err:
	regulator_inited = 1;
	return -EINVAL;
#else
	int ret = 0;

	if (regulator_inited)
		return ret;

	/* please only get regulator once in a driver */
	disp_bias_pos = regulator_get(NULL, "dsv_pos");
	if (IS_ERR(disp_bias_pos)) { /* handle return value */
		ret = PTR_ERR(disp_bias_pos);
		pr_err("get dsv_pos fail, error: %d\n", ret);
		return ret;
	}

	disp_bias_neg = regulator_get(NULL, "dsv_neg");
	if (IS_ERR(disp_bias_neg)) { /* handle return value */
		ret = PTR_ERR(disp_bias_pos);
		pr_err("get dsv_neg fail, error: %d\n", ret);
		return ret;
	}

	pr_info("%s: success\n", __func__);
	regulator_inited = 1;

	return ret; /* must be 0 */
#endif /* CONFIG_RT5081A_PMU_DSV_EXTPIN */
}
EXPORT_SYMBOL(display_bias_regulator_init);

int display_bias_enable(void)
{
	int ret = 0;
	int retval = 0;

	if (!regulator_inited)
		return ret;

#ifdef CONFIG_RT5081A_PMU_DSV_EXTPIN
	gpio_set_value(ext_gpio_pin, 1);
	ret = gpio_get_value(ext_gpio_pin);
	retval = ret ? 0 : -EINVAL;
	if (retval < 0)
		pr_err("%s fail\n", __func__);
	return retval;
#else
	/* set voltage with min & max*/
	ret = regulator_set_voltage(disp_bias_pos, 5400000, 5400000);
	if (ret < 0)
		pr_err("set voltage disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_set_voltage(disp_bias_neg, 5400000, 5400000);
	if (ret < 0)
		pr_err("set voltage disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	/* enable regulator */
	ret = regulator_enable(disp_bias_pos);
	if (ret < 0)
		pr_err("enable regulator disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_enable(disp_bias_neg);
	if (ret < 0)
		pr_err("enable regulator disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	return retval;
#endif /* CONFIG_RT5081A_PMU_DSV_EXTPIN */
}
EXPORT_SYMBOL(display_bias_enable);

int display_bias_disable(void)
{
	int ret = 0;
	int retval = 0;

	if (!regulator_inited)
		return ret;

#ifdef CONFIG_RT5081A_PMU_DSV_EXTPIN
	gpio_set_value(ext_gpio_pin, 0);
	ret = gpio_get_value(ext_gpio_pin);
	retval = ret ? -EINVAL : 0;
	if (retval < 0)
		pr_err("%s fail\n", __func__);
	return retval;
#else
	ret = regulator_disable(disp_bias_neg);
	if (ret < 0)
		pr_err("disable regulator disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_disable(disp_bias_pos);
	if (ret < 0)
		pr_err("disable regulator disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	return retval;
#endif /* CONFIG_RT5081A_PMU_DSV_EXTPIN */
}
EXPORT_SYMBOL(display_bias_disable);

#else
int display_bias_regulator_init(void)
{
	return 0;
}
EXPORT_SYMBOL(display_bias_regulator_init);

int display_bias_enable(void)
{
	return 0;
}
EXPORT_SYMBOL(display_bias_enable);

int display_bias_disable(void)
{
	return 0;
}
EXPORT_SYMBOL(display_bias_disable);
#endif


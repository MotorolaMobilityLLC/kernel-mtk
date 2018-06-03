/*
 * MTK Regulator Driver
 * Copyright (C) 2016
 * Author: Patrick Chang <patrick_chang@richtek.com>
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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/regulator/mediatek/mtk_regulator.h>

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
int mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	return 0;
}

int mtk_regulator_get_exclusive(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	return 0;
}

int devm_mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	return 0;
}

void mtk_regulator_put(struct mtk_regulator *mreg)
{
}

void devm_mtk_regulator_put(struct mtk_regulator *mreg)
{
}

int mtk_regulator_enable(struct mtk_regulator *mreg, bool enable)
{
	return 0;
}

int mtk_regulator_force_disable(struct mtk_regulator *mreg)
{
	return 0;
}

int mtk_regulator_is_enabled(struct mtk_regulator *mreg)
{
	return 0;
}

int mtk_regulator_set_mode(struct mtk_regulator *mreg,
	unsigned int mode)
{
	return 0;
}

unsigned int mtk_regulator_get_mode(struct mtk_regulator *mreg)
{
	return 0;
}

int mtk_regulator_set_voltage(struct mtk_regulator *mreg, int min_uV,
	int max_uV)
{
	return 0;
}

int mtk_regulator_get_voltage(struct mtk_regulator *mreg)
{
	return 0;
}

int mtk_regulator_set_current_limit(struct mtk_regulator *mreg,
					     int min_uA, int max_uA)
{
	return 0;
}

int mtk_regulator_get_current_limit(struct mtk_regulator *mreg)
{
	return 0;
}

#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

int mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	struct mtk_simple_regulator_device *mreg_dev = NULL;

	mreg->consumer = regulator_get(dev, id);
	if (IS_ERR(mreg->consumer))
		return PTR_ERR(mreg->consumer);

	mreg_dev = mtk_simple_regulator_get_dev_by_name(id);
	if (!mreg_dev) {
		pr_info("%s: no mreg device\n", __func__);
		return -ENODEV;
	}

	mreg->mreg_adv_ops = (struct mtk_simple_regulator_adv_ops *)
		dev_get_drvdata(&mreg_dev->dev);
	if (!mreg->mreg_adv_ops)
		pr_info("%s: no adv ops\n", __func__);

	return 0;
}

int mtk_regulator_get_exclusive(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	struct mtk_simple_regulator_device *mreg_dev = NULL;

	mreg->consumer = regulator_get_exclusive(dev, id);
	if (IS_ERR(mreg->consumer))
		return PTR_ERR(mreg->consumer);

	mreg_dev = mtk_simple_regulator_get_dev_by_name(id);
	if (!mreg_dev) {
		pr_info("%s: no mreg device\n", __func__);
		return -ENODEV;
	}

	mreg->mreg_adv_ops = (struct mtk_simple_regulator_adv_ops *)
		dev_get_drvdata(&mreg_dev->dev);
	if (!mreg->mreg_adv_ops)
		pr_info("%s: no adv ops\n", __func__);

	return 0;
}

int devm_mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg)
{
	struct mtk_simple_regulator_device *mreg_dev = NULL;

	mreg->consumer = devm_regulator_get(dev, id);
	if (IS_ERR(mreg->consumer))
		return PTR_ERR(mreg->consumer);

	mreg_dev = mtk_simple_regulator_get_dev_by_name(id);
	if (!mreg_dev) {
		pr_info("%s: no mreg device\n", __func__);
		return -ENODEV;
	}

	mreg->mreg_adv_ops = (struct mtk_simple_regulator_adv_ops *)
		dev_get_drvdata(&mreg_dev->dev);
	if (!mreg->mreg_adv_ops)
		pr_info("%s: no adv ops\n", __func__);

	return 0;
}

void mtk_regulator_put(struct mtk_regulator *mreg)
{
	regulator_put(mreg->consumer);
}

void devm_mtk_regulator_put(struct mtk_regulator *mreg)
{
	devm_regulator_put(mreg->consumer);
}

int mtk_regulator_enable(struct mtk_regulator *mreg, bool enable)
{
	return (enable ? regulator_enable :  regulator_disable)(mreg->consumer);
}

int mtk_regulator_force_disable(struct mtk_regulator *mreg)
{
	return regulator_force_disable(mreg->consumer);
}

int mtk_regulator_is_enabled(struct mtk_regulator *mreg)
{
	return regulator_is_enabled(mreg->consumer);
}

int mtk_regulator_set_mode(struct mtk_regulator *mreg,
	unsigned int mode)
{
	return regulator_set_mode(mreg->consumer, mode);
}

unsigned int mtk_regulator_get_mode(struct mtk_regulator *mreg)
{
	return regulator_get_mode(mreg->consumer);
}

int mtk_regulator_set_voltage(struct mtk_regulator *mreg, int min_uV,
	int max_uV)
{
	return regulator_set_voltage(mreg->consumer, min_uV, max_uV);
}

int mtk_regulator_get_voltage(struct mtk_regulator *mreg)
{
	return regulator_get_voltage(mreg->consumer);
}

int mtk_regulator_set_current_limit(struct mtk_regulator *mreg,
					     int min_uA, int max_uA)
{
	return regulator_set_current_limit(mreg->consumer,
		min_uA, max_uA);
}

int mtk_regulator_get_current_limit(struct mtk_regulator *mreg)
{
	return regulator_get_current_limit(mreg->consumer);
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

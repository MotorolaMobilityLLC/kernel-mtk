/*
 *  drivers/misc/mediatek/power/mt6797/mtk_gpuregulator_intf.c
 *  Driver for Richtek RT5735 Regulator
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include "mtk_gpuregulator_intf.h"

struct mtk_gpuregulator_info {
	struct device *dev;
	struct mtk_gpuregulator_platform_data *pdata;
	void *parent_info;
	u8 regval;
};

static struct mtk_gpuregulator_info *g_intf_info;

int rt_is_hw_exist(void)
{
	return g_intf_info ? 1 : 0;
}
EXPORT_SYMBOL(rt_is_hw_exist);

int rt_is_sw_ready(void)
{
	return g_intf_info ? 1 : 0;
}
EXPORT_SYMBOL(rt_is_sw_ready);

const char *rt_get_chip_name(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_chip_name)
		return "undefined chip name";
	return user_intf->get_chip_name(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_get_chip_name);

const char *rt_get_compatible_name(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_compatible_name)
		return "undefined compatible name";
	return user_intf->get_compatible_name(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_get_compatible_name);

int rt_get_chip_i2c_channel(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_chip_i2c_channel)
		return -ENOTSUPP;
	return user_intf->get_chip_i2c_channel(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_get_chip_i2c_channel);

int rt_get_chip_capability(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_chip_capability)
		return -ENOTSUPP;
	return user_intf->get_chip_capability(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_get_chip_capability);

int rt_hw_init_once(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->hw_init_once)
		return -ENOTSUPP;
	return user_intf->hw_init_once(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_hw_init_once);

int rt_hw_init_before_enable(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->hw_init_before_enable)
		return -ENOTSUPP;
	return user_intf->hw_init_before_enable(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_hw_init_before_enable);

int rt_set_vsel_enable(u8 vsel, u8 enable)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->set_vsel_enable)
		return -ENOTSUPP;
	return user_intf->set_vsel_enable(
		g_intf_info->parent_info, vsel, enable);
}
EXPORT_SYMBOL(rt_set_vsel_enable);

int rt_is_vsel_enabled(u8 vsel)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->is_vsel_enabled)
		return -ENOTSUPP;
	return user_intf->is_vsel_enabled(g_intf_info->parent_info, vsel);
}
EXPORT_SYMBOL(rt_is_vsel_enabled);

int rt_set_voltage(u8 vsel, int volt_uV)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->set_voltage)
		return -ENOTSUPP;
	return user_intf->set_voltage(g_intf_info->parent_info, vsel, volt_uV);
}
EXPORT_SYMBOL(rt_set_voltage);

int rt_get_voltage(u8 vsel)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_voltage)
		return -ENOTSUPP;
	return user_intf->get_voltage(g_intf_info->parent_info, vsel);
}
EXPORT_SYMBOL(rt_get_voltage);

int rt_get_slew_rate(u8 up_down)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->get_slew_rate)
		return -ENOTSUPP;
	return user_intf->get_slew_rate(g_intf_info->parent_info, up_down);
}
EXPORT_SYMBOL(rt_get_slew_rate);

int rt_set_slew_rate(u8 up_down, u8 val)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->set_slew_rate)
		return -ENOTSUPP;
	return user_intf->set_slew_rate(g_intf_info->parent_info, up_down, val);
}
EXPORT_SYMBOL(rt_set_slew_rate);

int rt_enable_force_pwm(u8 vsel, u8 enable)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->enable_force_pwm)
		return -ENOTSUPP;
	return user_intf->enable_force_pwm(
		g_intf_info->parent_info, vsel, enable);
}
EXPORT_SYMBOL(rt_enable_force_pwm);

int rt_is_force_pwm_enabled(u8 vsel)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->is_force_pwm_enabled)
		return -ENOTSUPP;
	return user_intf->is_force_pwm_enabled(g_intf_info->parent_info, vsel);
}
EXPORT_SYMBOL(rt_is_force_pwm_enabled);

int rt_enable_discharge(u8 enable)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->enable_discharge)
		return -ENOTSUPP;
	return user_intf->enable_discharge(g_intf_info->parent_info, enable);
}
EXPORT_SYMBOL(rt_enable_discharge);

int rt_is_discharge_enabled(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->is_discharge_enabled)
		return -ENOTSUPP;
	return user_intf->is_discharge_enabled(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_is_discharge_enabled);

int rt_read_interface(u8 cmd, u8 *return_data, u8 mask, u8 shift)
{
	const struct chip_io_intf *io_intf = g_intf_info->pdata->io_intf;
	int ret = 0;

	if (!return_data)
		return -EINVAL;
	io_intf->io_lock(g_intf_info->parent_info);
	ret = io_intf->io_read(g_intf_info->parent_info, cmd);
	if (ret < 0) {
		dev_err(g_intf_info->dev, "read interface 0x%02x fail\n", cmd);
		goto OUT_READ_INTF;
	}
	ret &= (mask << shift);
	*return_data = ret >> shift;
OUT_READ_INTF:
	io_intf->io_unlock(g_intf_info->parent_info);
	return ret;
}
EXPORT_SYMBOL(rt_read_interface);

int rt_read_byte(u8 cmd, u8 *return_data)
{
	const struct chip_io_intf *io_intf = g_intf_info->pdata->io_intf;
	int ret = 0;

	if (!return_data)
		return -EINVAL;
	io_intf->io_lock(g_intf_info->parent_info);
	ret = io_intf->io_read(g_intf_info->parent_info, cmd);
	if (ret < 0) {
		dev_err(g_intf_info->dev, "read byte 0x%02x, fail\n", cmd);
		goto OUT_READ_BYTE;
	}
	*return_data = ret & 0xff;
OUT_READ_BYTE:
	io_intf->io_unlock(g_intf_info->parent_info);
	return ret;
}
EXPORT_SYMBOL(rt_read_byte);

int rt_config_interface(u8 cmd, u8 val, u8 mask, u8 shift)
{
	const struct chip_io_intf *io_intf = g_intf_info->pdata->io_intf;
	u8 tmp = 0;
	int ret = 0;

	io_intf->io_lock(g_intf_info->parent_info);
	ret = io_intf->io_read(g_intf_info->parent_info, cmd);
	if (ret < 0) {
		dev_err(g_intf_info->dev, "config intf rd 0x%02x fail\n", cmd);
		goto OUT_CONFIG_INTF;
	}
	tmp = ret & 0xff;
	tmp &= ~(mask << shift);
	tmp |= (val << shift);
	ret = io_intf->io_write(g_intf_info->parent_info, cmd, tmp);
OUT_CONFIG_INTF:
	io_intf->io_unlock(g_intf_info->parent_info);
	return ret;
}
EXPORT_SYMBOL(rt_config_interface);

void rt_dump_registers(void)
{
	const struct mtk_user_intf *user_intf = g_intf_info->pdata->user_intf;

	if (!user_intf->dump_registers)
		return;
	user_intf->dump_registers(g_intf_info->parent_info);
}
EXPORT_SYMBOL(rt_dump_registers);

static ssize_t show_rt_access(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct mtk_gpuregulator_info *mtk_rtinfo = dev_get_drvdata(dev);

	return sprintf(buf, "0x%02x\n", mtk_rtinfo->regval);
}

static ssize_t store_rt_access(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct mtk_gpuregulator_info *mtk_rtinfo = dev_get_drvdata(dev);
	void *parent_info = mtk_rtinfo->parent_info;
	const struct chip_io_intf *io_intf = mtk_rtinfo->pdata->io_intf;
	u32 regaddr = 0, regval = 0;
	int ret = 0;

	dev_info(dev, "[%s]\n", __func__);
	ret = sscanf(buf, "0x%x 0x%x\n", &regaddr, &regval);
	if (ret <= 0)
		return -EINVAL;
	switch (ret) {
	case 1:
		dev_err(dev, "rd reg %02x\n", regaddr);
		io_intf->io_lock(parent_info);
		ret = io_intf->io_read(parent_info, regaddr);
		io_intf->io_unlock(parent_info);
		if (ret < 0)
			dev_err(dev, "rd reg 0x%02x fail\n", regaddr);
		else
			mtk_rtinfo->regval = ret & 0xff;
		break;
	case 2:
	default:
		dev_err(dev, "wr reg %02x value %02x\n", regaddr, regval);
		io_intf->io_lock(parent_info);
		ret = io_intf->io_write(parent_info, regaddr, regval);
		io_intf->io_unlock(parent_info);
		if (ret < 0)
			dev_err(dev, "wr reg %02x fail\n", regaddr);
		break;
	}
	return count;
}

static const DEVICE_ATTR(rt_access, S_IRUGO | S_IWUSR | S_IWGRP,
		   show_rt_access, store_rt_access);

static int mtk_gpuregulator_intf_probe(struct platform_device *pdev)
{
	struct mtk_gpuregulator_info *mtk_rtinfo;
	struct mtk_gpuregulator_platform_data *pdata = pdev->dev.platform_data;
	void *parent_info = dev_get_drvdata(pdev->dev.parent);
	int ret = 0;

	if (!pdata || !parent_info)
		return -EINVAL;
	if (!pdata->user_intf || !pdata->io_intf)
		return -EINVAL;
	mtk_rtinfo = devm_kzalloc(&pdev->dev, sizeof(*mtk_rtinfo), GFP_KERNEL);
	if (!mtk_rtinfo)
		return -ENOMEM;

	mtk_rtinfo->dev = &pdev->dev;
	mtk_rtinfo->parent_info = parent_info;
	mtk_rtinfo->pdata = pdata;
	platform_set_drvdata(pdev, mtk_rtinfo);

	ret = device_create_file(&pdev->dev, &dev_attr_rt_access);
	if (ret < 0)
		goto ERR_CREATE_FILE;
	g_intf_info = mtk_rtinfo;
	dev_info(&pdev->dev, "mtk gpuregulator driver successfully probed\n");
	return 0;
ERR_CREATE_FILE:
	devm_kfree(&pdev->dev, mtk_rtinfo);
	return ret;
}

static int mtk_gpuregulator_intf_remove(struct platform_device *pdev)
{
	struct mtk_gpuregulator_info *mtk_rtinfo = platform_get_drvdata(pdev);

	g_intf_info = NULL;
	device_remove_file(&pdev->dev, &dev_attr_rt_access);
	dev_info(mtk_rtinfo->dev, "mtk gpuregulator driver successfully removed\n");
	return 0;
}

static struct platform_driver mtk_gpuregulator_intf_driver = {
	.driver = {
		.name = "mtk_gpuregulator_intf",
	},
	.probe = mtk_gpuregulator_intf_probe,
	.remove = mtk_gpuregulator_intf_remove,
};
module_platform_driver(mtk_gpuregulator_intf_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTK GPU Interface Driver for Richtek Regulator");
MODULE_AUTHOR("CY Huang <cy_huang@richtek.com>");
MODULE_VERSION("1.0.0_M");

/*
 *  drivers/switch/switch_usb_rt8973.c
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 *  USB switch driver
 *
 * Author: Patrick Chang <patrick_chang@richtek.com>
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/i2c.h>

#define RT8973_DEVICE_NAME		"rt8973"
#define RT8973_DRV_VER			"1.0.0_M"

#define RT8973_REG_CTRL	0x02

#define RT8973_REGVAL_MANUAL_CTRL	0x09
#define RT8973_REG_DEV_TYPE	0x0A

#define RT8973_REG_MANUAL_SW	0X13

#define RT8973_REGVAL_SW_CHG	0x6c
#define RT8973_REGVAL_SW_USB	0x24

#define RT8973_REGVAL_DEV_TYPE_SDP	(1 << 2)
#define RT8973_REGVAL_DEV_TYPE_CDP	(1 << 5)

struct rt8973_usb_switch_data {
	struct switch_dev sdev;
	struct i2c_client *iic;

};

static const char *const usb_states[] = {
	"charger",
	"usb",
};

static const uint8_t usb_state_regvals[] = {
	RT8973_REGVAL_SW_CHG,
	RT8973_REGVAL_SW_USB
};

static ssize_t rt8973_usb_switch_print_state(struct switch_dev *sdev, char *buf)
{
	int state;

	state = switch_get_state(sdev);
	if (state < 0)
		return state;

	WARN_ON(state >= ARRAY_SIZE(usb_states));

	return snprintf(buf,  PAGE_SIZE, "%s\n", usb_states[state]);
}

static int rt8973_usb_switch_set_state(struct switch_dev *sdev, int state)
{
	struct rt8973_usb_switch_data	*switch_data =
		container_of(sdev, struct rt8973_usb_switch_data, sdev);

	WARN_ON(state >= ARRAY_SIZE(usb_state_regvals));

	return i2c_smbus_write_byte_data(switch_data->iic,
		RT8973_REG_MANUAL_SW, usb_state_regvals[state]);
}

static int rt8973_usb_switch_parse_dt(struct rt8973_usb_switch_data *switch_data)
{
	struct device_node *np = switch_data->iic->dev.of_node;
	int retval;

	if (!np)
		return -EINVAL;
	retval = of_property_read_string(np, "usb_switch_name",
		(char const **)&switch_data->sdev.name);
	if (retval < 0)
		switch_data->sdev.name = "usb_switch";

	return 0;
}

static int rt8973_usb_switch_init(struct rt8973_usb_switch_data *switch_data)
{
	int ret;
	bool is_std_or_cdp = false;

	ret = i2c_smbus_read_byte_data(switch_data->iic, RT8973_REG_DEV_TYPE);
	if (ret < 0)
		return ret;

	if (ret & (RT8973_REGVAL_DEV_TYPE_CDP | RT8973_REGVAL_DEV_TYPE_SDP)) {
		is_std_or_cdp = true;
		switch_data->sdev.state = 1;
	} else
		is_std_or_cdp = false;

	ret = i2c_smbus_write_byte_data(switch_data->iic, RT8973_REG_MANUAL_SW,
		usb_state_regvals[is_std_or_cdp ? 1 : 0]);
	if (ret < 0)
		return ret;
	ret = i2c_smbus_write_byte_data(switch_data->iic, RT8973_REG_CTRL,
		RT8973_REGVAL_MANUAL_CTRL);
	return ret;
}

static int rt8973_usbsw_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct rt8973_usb_switch_data *switch_data;
	int ret = 0;

	switch_data = devm_kzalloc(&client->dev, sizeof(*switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;
	switch_data->sdev.name = "usb_switch";
	switch_data->sdev.set_state = rt8973_usb_switch_set_state;
	switch_data->sdev.print_state = rt8973_usb_switch_print_state;
	switch_data->iic = client;
	i2c_set_clientdata(client, switch_data);
	rt8973_usb_switch_parse_dt(switch_data);
	ret = rt8973_usb_switch_init(switch_data);

	if (ret < 0)
		goto err_init;
	ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	return 0;
err_switch_dev_register:
	i2c_smbus_write_byte_data(client, 0x1b, 0x01);
err_init:
	devm_kfree(&client->dev, switch_data);
	return ret;
}

static int rt8973_usbsw_remove(struct i2c_client *client)
{
	return 0;
}

static void rt8973_usbsw_shutdown(struct i2c_client *client)
{
	/* Reset RT8973 */
	i2c_smbus_write_byte_data(client, 0x1b, 0x01);
}

#ifdef CONFIG_PM
static int rt8973_usbsw_suspend(struct device *dev)
{
	return 0;
}

static int rt8973_usbsw_resume(struct device *dev)
{
	return 0;
}

#ifdef CONFIG_PM_RUNTIME
static int rt8973_usbsw_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	return 0;
}

static int rt8973_usbsw_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	return 0;
}

static int rt8973_usbsw_runtime_idle(struct device *dev)
{
	return 0;
}
#endif /* #ifdef CONFIG_PM_RUNTIME */

static const struct dev_pm_ops rt8973_usbsw_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rt8973_usbsw_suspend, rt8973_usbsw_resume)
#ifdef CONFIG_PM_RUNTIME
	SET_RUNTIME_PM_OPS(rt8973_usbsw_runtime_suspend,
			   rt8973_usbsw_runtime_resume,
			   rt8973_usbsw_runtime_idle)
#endif /* #ifdef CONFIG_PM_RUNTIME */
};
#define ptr8973_usbsw_pm_ops (&rt8973_usbsw_pm_ops)
#else
#define ptr8973_usbsw_pm_ops (NULL)
#endif /* #ifdef CONFIG_PM */

static const struct i2c_device_id rt8973_usbsw_id[] = {
	{RT8973_DEVICE_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt8973_usbsw_id);

#ifdef CONFIG_OF
static const struct of_device_id rt8973_usbsw_match_table[] = {
	{.compatible = "richtek,rt8973-usbsw",},
	{},
};
MODULE_DEVICE_TABLE(of, rt8973_usbsw_match_table);
#endif /* #ifdef CONFIG_OF */

static struct i2c_driver rt8973_i2c_driver = {
	.driver = {
		.name = RT8973_DEVICE_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(rt8973_usbsw_match_table),
#endif /* #ifdef CONFIG_OF */
		.pm = ptr8973_usbsw_pm_ops,
	},
	.shutdown	= rt8973_usbsw_shutdown,
	.probe = rt8973_usbsw_probe,
	.remove = rt8973_usbsw_remove,
	.id_table = rt8973_usbsw_id,
};

static int __init rt8973_usbsw_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	ret = i2c_add_driver(&rt8973_i2c_driver);

	return ret;
}

static void __exit rt8973_usbsw_exit(void)
{
	pr_info("%s\n", __func__);
	i2c_del_driver(&rt8973_i2c_driver);
}

subsys_initcall_sync(rt8973_usbsw_init);
module_exit(rt8973_usbsw_exit);

MODULE_VERSION(RT8973_DRV_VER);
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_DESCRIPTION("USB switch driver");
MODULE_LICENSE("GPL");

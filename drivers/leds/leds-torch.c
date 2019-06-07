/*
 * LED driver : leds-torch.c
 *
 * Copyright (C) 2019 Ontim Electronics
 * Gaowei Pu <gaowei.pu@ontim.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/led-class-flash.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
struct pinctrl *pinctrtorch;
struct pinctrl_state *torch_off, *torch_on;
struct mutex torch_lock;
static int get_torch_pinctl_info(struct platform_device *pdev)
{
	int ret = 0;
	pinctrtorch = devm_pinctrl_get(&pdev->dev);
 	if (IS_ERR(pinctrtorch)) {
 		ret = PTR_ERR(pinctrtorch);
 		dev_info(&pdev->dev, "fwq Cannot find pinctrtorch!\n");
 		return ret;
 	}
 	torch_off = pinctrl_lookup_state(pinctrtorch, "torch_off");
 	if (IS_ERR(torch_off)) {
 		ret = PTR_ERR(torch_off);
 		dev_info(&pdev->dev, "Cannot find pinctrl torch_off %d!\n", ret);
 	}
 	torch_on = pinctrl_lookup_state(pinctrtorch, "torch_on");
 	if (IS_ERR(torch_on)) {
 		ret = PTR_ERR(torch_on);
 		dev_info(&pdev->dev, "Cannot find pinctrl torch_on!\n");
 		return ret;
 	}
	pinctrl_select_state(pinctrtorch, torch_off);
	return ret;
}
static int torch_led_brightness_set(struct led_classdev *led_cdev,
				       enum led_brightness brightness)
{
	mutex_lock(&torch_lock);
	if (brightness == LED_OFF)
		pinctrl_select_state(pinctrtorch, torch_off);
	else
		pinctrl_select_state(pinctrtorch, torch_on);
	mutex_unlock(&torch_lock);
	return 0;
}

static int torch_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct led_classdev *led_cdev;
	struct led_classdev_flash *fled_cdev;
	ret = get_torch_pinctl_info(pdev);
	if (ret)
	{
		dev_err(&pdev->dev, "can't get torch pinctrl.\n");
		return -1;
	}
	fled_cdev = devm_kzalloc(&pdev->dev, sizeof(*fled_cdev), GFP_KERNEL);
	if (!fled_cdev)
		return -ENOMEM;
	led_cdev = &fled_cdev->led_cdev;
	led_cdev->name = "torch_led";//fixme
	//led_cdev->brightness_set = torch_led_brightness_set;
	led_cdev->brightness_set_sync = torch_led_brightness_set;
	mutex_init(&torch_lock);
	ret = led_classdev_flash_register(&pdev->dev, fled_cdev);
	if (ret) {
		dev_err(&pdev->dev, "can't register LED %s\n", led_cdev->name);
		mutex_destroy(&torch_lock);
		return ret;
	}
	return ret;
}

static const struct of_device_id torch_match[] = {
	{ .compatible = "ontim,torch", },
	{ /* sentinel */ },
};

static struct platform_driver torch_driver = {
	.driver = {
		.name  = "torch",
		.of_match_table = torch_match,
	},
	.probe  = torch_probe,
};

module_platform_driver(torch_driver);

MODULE_AUTHOR("gaowei <gaowei.pu@ontim.cn>");
MODULE_DESCRIPTION("torch LED driver");
MODULE_LICENSE("GPL v2");

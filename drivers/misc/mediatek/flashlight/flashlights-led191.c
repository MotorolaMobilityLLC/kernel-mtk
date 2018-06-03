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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include "flashlight.h"
#include "flashlight-dt.h"

/* define device tree */
/* TODO: modify temp device tree name */
#ifndef LED191_DTNAME
#define LED191_DTNAME "mediatek,flashlights_led191"
#endif

/* TODO: define driver name */
#define LED191_NAME "flashlights_led191"

#ifdef CONFIG_OF
/* #waning "defined CONFIG_OF" */
#else
/* #waning "not defined CONFIG_OF" */
#endif

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(led191_mutex);
static struct work_struct led191_work;

/* define pinctrl */
/* TODO: define pinctrl */
#define LED191_PINCTRL_PIN_XXX 0
#define LED191_PINCTRL_PINSTATE_LOW 0
#define LED191_PINCTRL_PINSTATE_HIGH 1
#define LED191_PINCTRL_STATE_HWEN_HIGH "hwen_high"
#define LED191_PINCTRL_STATE_HWEN_LOW  "hwen_low"
static struct pinctrl *led191_pinctrl;
static struct pinctrl_state *led191_hwen_high;
static struct pinctrl_state *led191_hwen_low;

/* define usage count */
static int use_count;

static int g_flash_duty = -1;

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int led191_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	led191_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(led191_pinctrl)) {
		fl_pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(led191_pinctrl);
	}

	/* TODO: Flashlight XXX pin initialization */
	led191_hwen_high = pinctrl_lookup_state(led191_pinctrl, LED191_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(led191_hwen_high)) {
		fl_pr_err("Failed to init (%s)\n", LED191_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(led191_hwen_high);
	}
	led191_hwen_low = pinctrl_lookup_state(led191_pinctrl, LED191_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(led191_hwen_low)) {
		fl_pr_err("Failed to init (%s)\n", LED191_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(led191_hwen_low);
	}

	return ret;
}

static int led191_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(led191_pinctrl)) {
		fl_pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case LED191_PINCTRL_PIN_XXX:
		if (state == LED191_PINCTRL_PINSTATE_LOW && !IS_ERR(led191_hwen_low))
			ret = pinctrl_select_state(led191_pinctrl, led191_hwen_low);
		else if (state == LED191_PINCTRL_PINSTATE_HIGH && !IS_ERR(led191_hwen_high))
			ret = pinctrl_select_state(led191_pinctrl, led191_hwen_high);
		else
			fl_pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		fl_pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	fl_pr_debug("pin(%d) state(%d), ret:%d\n", pin, state, ret);

	return ret;
}


/******************************************************************************
 * led191 operations
 *****************************************************************************/
/* flashlight enable function */
static int led191_enable(void)
{
	int pin = 0;

	/* TODO: wrap enable function */
	if (g_flash_duty == 1) {
		led191_pinctrl_set(pin, 1);
	} else {
		led191_pinctrl_set(pin, 1);
		led191_pinctrl_set(pin, 0);
	}

	led191_pinctrl_set(pin, 1);

	return 0;
}

/* flashlight disable function */
static int led191_disable(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap disable function */

	return led191_pinctrl_set(pin, state);
}

/* set flashlight level */
static int led191_set_level(int level)
{
	/* TODO: wrap set level function */
	g_flash_duty = level;

	return 0;
}

/* flashlight init */
static int led191_init(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap init function */

	return led191_pinctrl_set(pin, state);
}

/* flashlight uninit */
static int led191_uninit(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap uninit function */

	return led191_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer led191_timer;
static unsigned int led191_timeout_ms;

static void led191_work_disable(struct work_struct *data)
{
	fl_pr_debug("work queue callback\n");
	led191_disable();
}

static enum hrtimer_restart led191_timer_func(struct hrtimer *timer)
{
	schedule_work(&led191_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int led191_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		fl_pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		led191_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		led191_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (led191_timeout_ms) {
				ktime = ktime_set(led191_timeout_ms / 1000,
						(led191_timeout_ms % 1000) * 1000000);
				hrtimer_start(&led191_timer, ktime, HRTIMER_MODE_REL);
			}
			led191_enable();
		} else {
			led191_disable();
			hrtimer_cancel(&led191_timer);
		}
		break;
	default:
		fl_pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int led191_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int led191_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&led191_mutex);
	use_count--;
	if (!use_count)
		led191_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&led191_mutex);

	fl_pr_debug("Release: %d\n", use_count);

	return 0;
}

static int led191_set_driver(void)
{
	/* init chip and set usage count */
	mutex_lock(&led191_mutex);
	if (!use_count)
		led191_init();
	use_count++;
	mutex_unlock(&led191_mutex);

	fl_pr_debug("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t led191_strobe_store(struct flashlight_arg arg)
{
	led191_set_driver();
	led191_set_level(arg.level);
	led191_enable();
	msleep(arg.dur);
	led191_disable();
	led191_release(NULL);

	return 0;
}

static struct flashlight_operations led191_ops = {
	led191_open,
	led191_release,
	led191_ioctl,
	led191_strobe_store,
	led191_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int led191_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * led191_init();
	 */

	return 0;
}

static int led191_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;

	fl_pr_debug("Probe start.\n");

	/* init work queue */
	INIT_WORK(&led191_work, led191_work_disable);

	/* init timer */
	hrtimer_init(&led191_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	led191_timer.function = led191_timer_func;
	led191_timeout_ms = 100;

	/* init chip hw */
	led191_chip_init();

	/* register flashlight operations */
	if (flashlight_dev_register(LED191_NAME, &led191_ops)) {
		err = -EFAULT;
		goto err;
	}
#if 0
	if (i2c_add_driver(&led191_i2c_driver)) {
		fl_pr_debug("Failed to add i2c driver.\n");
		return -1;
	}
#endif

	/* clear usage count */
	use_count = 0;

	fl_pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int led191_i2c_remove(struct i2c_client *client)
{
	fl_pr_debug("Remove start.\n");

	/* flush work queue */
	flush_work(&led191_work);

	/* unregister flashlight operations */
	flashlight_dev_unregister(LED191_NAME);

	fl_pr_debug("Remove done.\n");

	return 0;
}

static const struct i2c_device_id led191_i2c_id[] = {
	{LED191_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id led191_i2c_of_match[] = {
	{.compatible = LED191_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver led191_i2c_driver = {
	.driver = {
		   .name = LED191_NAME,
#ifdef CONFIG_OF
		   .of_match_table = led191_i2c_of_match,
#endif
		   },
	.probe = led191_i2c_probe,
	.remove = led191_i2c_remove,
	.id_table = led191_i2c_id,
};

static int led191_probe(struct platform_device *dev)
{
	int err;

	fl_pr_debug("Probe start.\n");

		/* init pinctrl */
	if (led191_pinctrl_init(dev)) {
		fl_pr_debug("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto error;
	}

	if (i2c_add_driver(&led191_i2c_driver)) {
		fl_pr_debug("Failed to add i2c driver.\n");
		return -1;
	}

	fl_pr_debug("Probe done.\n");
	return 0;

error:
	return err;
}

static int led191_remove(struct platform_device *dev)
{
	fl_pr_debug("Remove start.\n");

	i2c_del_driver(&led191_i2c_driver);

	fl_pr_debug("Remove done.\n");

	return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id led191_gpio_of_match[] = {
	{.compatible = LED191_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, led191_gpio_of_match);
#else
static struct platform_device led191_gpio_platform_device[] = {
	{
		.name = LED191_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, led191_gpio_platform_device);
#endif

static struct platform_driver led191_platform_driver = {
	.probe = led191_probe,
	.remove = led191_remove,
	.driver = {
		.name = LED191_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = led191_gpio_of_match,
#endif
	},
};

static int __init flashlight_led191_init(void)
{
	int ret;

	fl_pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&led191_gpio_platform_device);
	if (ret) {
		fl_pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&led191_platform_driver);
	if (ret) {
		fl_pr_err("Failed to register platform driver\n");
		return ret;
	}

	fl_pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_led191_exit(void)
{
	fl_pr_debug("Exit start.\n");

	platform_driver_unregister(&led191_platform_driver);

	fl_pr_debug("Exit done.\n");
}

module_init(flashlight_led191_init);
module_exit(flashlight_led191_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight LED191 GPIO Driver");


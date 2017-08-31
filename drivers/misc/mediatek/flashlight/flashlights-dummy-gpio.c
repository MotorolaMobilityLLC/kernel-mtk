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
#ifndef DUMMY_DTNAME
#define DUMMY_DTNAME "mediatek,flashlights_dummy_gpio"
#endif

/* TODO: define driver name */
#define DUMMY_NAME "flashlights-dummy-gpio"

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(dummy_mutex);
static struct work_struct dummy_work;

/* define pinctrl */
/* TODO: define pinctrl */
#define DUMMY_PINCTRL_PIN_XXX 0
#define DUMMY_PINCTRL_PINSTATE_LOW 0
#define DUMMY_PINCTRL_PINSTATE_HIGH 1
#define DUMMY_PINCTRL_STATE_XXX_HIGH "xxx_high"
#define DUMMY_PINCTRL_STATE_XXX_LOW  "xxx_low"
static struct pinctrl *dummy_pinctrl;
static struct pinctrl_state *dummy_xxx_high;
static struct pinctrl_state *dummy_xxx_low;

/* define usage count */
static int use_count;


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int dummy_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	dummy_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(dummy_pinctrl)) {
		fl_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(dummy_pinctrl);
	}

	/* Flashlight XXX pin initialization */
	dummy_xxx_high = pinctrl_lookup_state(dummy_pinctrl, DUMMY_PINCTRL_STATE_XXX_HIGH);
	if (IS_ERR(dummy_xxx_high)) {
		fl_err("Failed to init (%s)\n", DUMMY_PINCTRL_STATE_XXX_HIGH);
		ret = PTR_ERR(dummy_xxx_high);
	}
	dummy_xxx_low = pinctrl_lookup_state(dummy_pinctrl, DUMMY_PINCTRL_STATE_XXX_LOW);
	if (IS_ERR(dummy_xxx_low)) {
		fl_err("Failed to init (%s)\n", DUMMY_PINCTRL_STATE_XXX_LOW);
		ret = PTR_ERR(dummy_xxx_low);
	}

	return ret;
}

static int dummy_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(dummy_pinctrl)) {
		fl_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case DUMMY_PINCTRL_PIN_XXX:
		if (state == DUMMY_PINCTRL_PINSTATE_LOW && !IS_ERR(dummy_xxx_low))
			pinctrl_select_state(dummy_pinctrl, dummy_xxx_low);
		else if (state == DUMMY_PINCTRL_PINSTATE_HIGH && !IS_ERR(dummy_xxx_high))
			pinctrl_select_state(dummy_pinctrl, dummy_xxx_high);
		else
			fl_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		fl_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	fl_dbg("pin(%d) state(%d)\n", pin, state);

	return ret;
}


/******************************************************************************
 * dummy operations
 *****************************************************************************/
/* flashlight enable function */
static int dummy_enable(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap enable function */

	return dummy_pinctrl_set(pin, state);
}

/* flashlight disable function */
static int dummy_disable(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap disable function */

	return dummy_pinctrl_set(pin, state);
}

/* set flashlight level */
static int dummy_set_level(int level)
{
	int pin = 0, state = 0;

	/* TODO: wrap set level function */

	return dummy_pinctrl_set(pin, state);
}

/* flashlight init */
int dummy_init(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap init function */

	return dummy_pinctrl_set(pin, state);
}

/* flashlight uninit */
int dummy_uninit(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap uninit function */

	return dummy_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer fl_timer;
static unsigned int fl_timeout_ms;

static void dummy_work_disable(struct work_struct *data)
{
	fl_dbg("work queue callback\n");
	dummy_disable();
}

static enum hrtimer_restart fl_timer_func(struct hrtimer *timer)
{
	schedule_work(&dummy_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int dummy_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_user_arg *fl_arg;
	int ct_index;
	ktime_t ktime;

	fl_arg = (struct flashlight_user_arg *)arg;
	ct_index = fl_get_ct_index(fl_arg->ct_id);
	if (flashlight_ct_index_verify(ct_index)) {
		fl_err("Failed with error index\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		fl_dbg("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		fl_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		dummy_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_STEP:
		fl_dbg("FLASH_IOC_SET_STEP(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (fl_timeout_ms) {
				ktime = ktime_set(fl_timeout_ms / 1000,
						(fl_timeout_ms % 1000) * 1000000);
				hrtimer_start(&fl_timer, ktime, HRTIMER_MODE_REL);
			}
			dummy_enable();
		} else {
			dummy_disable();
			hrtimer_cancel(&fl_timer);
		}
		break;
	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				ct_index, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int dummy_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int dummy_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&dummy_mutex);
	use_count--;
	if (!use_count)
		dummy_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&dummy_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int dummy_set_driver(int scenario)
{
	/* init chip and set usage count */
	mutex_lock(&dummy_mutex);
	if (!use_count)
		dummy_init();
	use_count++;
	mutex_unlock(&dummy_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t dummy_strobe_store(struct flashlight_arg arg)
{
	dummy_set_driver(FLASHLIGHT_SCENARIO_CAMERA);
	dummy_set_level(arg.level);
	dummy_enable();
	msleep(arg.dur);
	dummy_disable();
	dummy_release(NULL);

	return 0;
}

static struct flashlight_operations dummy_ops = {
	dummy_open,
	dummy_release,
	dummy_ioctl,
	dummy_strobe_store,
	dummy_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int dummy_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * dummy_init();
	 */

	return 0;
}

static int dummy_probe(struct platform_device *dev)
{
	int err;

	fl_dbg("Probe start.\n");

	/* init pinctrl */
	if (dummy_pinctrl_init(dev)) {
		fl_dbg("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init work queue */
	INIT_WORK(&dummy_work, dummy_work_disable);

	/* init timer */
	hrtimer_init(&fl_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fl_timer.function = fl_timer_func;
	fl_timeout_ms = 100;

	/* init chip hw */
	dummy_chip_init();

	/* register flashlight operations */
	if (flashlight_dev_register(DUMMY_NAME, &dummy_ops)) {
		err = -EFAULT;
		goto err;
	}

	/* clear usage count */
	use_count = 0;

	fl_dbg("Probe done.\n");

	return 0;
err:
	return err;
}

static int dummy_remove(struct platform_device *dev)
{
	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&dummy_work);

	/* unregister flashlight operations */
	flashlight_dev_unregister(DUMMY_NAME);

	fl_dbg("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dummy_gpio_of_match[] = {
	{.compatible = DUMMY_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, dummy_gpio_of_match);
#else
static struct platform_device dummy_gpio_platform_device[] = {
	{
		.name = DUMMY_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, dummy_gpio_platform_device);
#endif

static struct platform_driver dummy_platform_driver = {
	.probe = dummy_probe,
	.remove = dummy_remove,
	.driver = {
		.name = DUMMY_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = dummy_gpio_of_match,
#endif
	},
};

static int __init flashlight_dummy_init(void)
{
	int ret;

	fl_dbg("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&dummy_gpio_platform_device);
	if (ret) {
		fl_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&dummy_platform_driver);
	if (ret) {
		fl_err("Failed to register platform driver\n");
		return ret;
	}

	fl_dbg("Init done.\n");

	return 0;
}

static void __exit flashlight_dummy_exit(void)
{
	fl_dbg("Exit start.\n");

	platform_driver_unregister(&dummy_platform_driver);

	fl_dbg("Exit done.\n");
}

module_init(flashlight_dummy_init);
module_exit(flashlight_dummy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight DUMMY GPIO Driver");


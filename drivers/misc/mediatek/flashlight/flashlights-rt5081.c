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

#include "richtek/rt-flashlight.h"
#include "mtk_charger.h"

#include "flashlight.h"
#include "flashlight-dt.h"

/* device tree should be defined in flashlight-dt.h */
#ifndef RT5081_DTNAME
#define RT5081_DTNAME "mediatek,flashlights_rt5081"
#endif

#define RT5081_NAME "flashlights-rt5081"

/* define ct, level */
#define RT5081_CT_NUM 2
#define RT5081_CT_HT 0
#define RT5081_CT_LT 1

#define RT5081_NONE (-1)
#define RT5081_DISABLE 0
#define RT5081_ENABLE 1
#define RT5081_ENABLE_TORCH 1
#define RT5081_ENABLE_FLASH 2

#define RT5081_LEVEL_NUM 32
#define RT5081_LEVEL_TORCH 16
#define RT5081_LEVEL_FLASH RT5081_LEVEL_NUM
#define RT5081_WDT_TIMEOUT 1248 /* ms */

/* define mutex, work queue and timer */
static DEFINE_MUTEX(rt5081_mutex);
static DEFINE_MUTEX(rt5081_enable_mutex);
static DEFINE_MUTEX(rt5081_disable_mutex);
static struct work_struct rt5081_work_ht;
static struct work_struct rt5081_work_lt;
static struct hrtimer fl_timer_ht;
static struct hrtimer fl_timer_lt;
static unsigned int rt5081_timeout_ms[RT5081_CT_NUM];

/* define usage count */
static int use_count;

/* define RTK flashlight device */
struct flashlight_device *flashlight_dev_ht;
struct flashlight_device *flashlight_dev_lt;
#define RT_FLED_DEVICE_HT  "rt-flash-led1"
#define RT_FLED_DEVICE_LT  "rt-flash-led2"

/* define charger consumer */
static struct charger_consumer *flashlight_charger_consumer;
#define CHARGER_SUPPLY_NAME "charger_port1"

/* is decrease voltage */
static int has_decrease_voltage;

/******************************************************************************
 * rt5081 operations
 *****************************************************************************/
static const unsigned char rt5081_torch_level[RT5081_LEVEL_TORCH] = {
	0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12,
	0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E
};

static const unsigned char rt5081_strobe_level[RT5081_LEVEL_FLASH] = {
	0x80, 0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10,
	0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x20, 0x24, 0x28, 0x2C,
	0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54,
	0x58, 0x5C
};

static int rt5081_en_ht;
static int rt5081_en_lt;
static int rt5081_level_ht;
static int rt5081_level_lt;

static int rt5081_is_charger_ready(void)
{
	if (flashlight_is_ready(flashlight_dev_ht) &&
			flashlight_is_ready(flashlight_dev_ht))
		return FLASHLIGHT_CHARGER_READY;
	else
		return FLASHLIGHT_CHARGER_NOT_READY;
}

static int rt5081_is_torch(int level)
{
	if (level >= RT5081_LEVEL_TORCH)
		return -1;

	return 0;
}

#if 0
static int rt5081_is_torch_by_timeout(int timeout)
{
	if (!timeout)
		return 0;

	if (timeout >= RT5081_WDT_TIMEOUT)
		return 0;

	return -1;
}
#endif

static int rt5081_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= RT5081_LEVEL_NUM)
		level = RT5081_LEVEL_NUM - 1;

	return level;
}

/* flashlight enable function */
static int rt5081_enable(void)
{
	int ret = 0;

	if (!flashlight_dev_ht) {
		fl_err("Failed to enable since no flashlight device.\n");
		return -1;
	}

	if ((rt5081_en_ht == RT5081_ENABLE_FLASH)
			|| (rt5081_en_lt == RT5081_ENABLE_FLASH))
		ret = flashlight_set_mode(
				flashlight_dev_ht, FLASHLIGHT_MODE_FLASH);
	else
		ret = flashlight_set_mode(
				flashlight_dev_ht, FLASHLIGHT_MODE_TORCH);

	return ret;
}

/* flashlight disable function */
static int rt5081_disable(void)
{
	if (!flashlight_dev_ht) {
		fl_err("Failed to disable since no flashlight device.\n");
		return -1;
	}

	return flashlight_set_mode(flashlight_dev_ht, FLASHLIGHT_MODE_OFF);
}

/* set flashlight level */
static int rt5081_set_level_ht(int level)
{
	level = rt5081_verify_level(level);
	rt5081_level_ht = level;

	if (!flashlight_dev_ht) {
		fl_err("Failed to set ht level since no flashlight device.\n");
		return -1;
	}

	/* set brightness level */
	if (!rt5081_is_torch(level))
		flashlight_set_torch_brightness(
				flashlight_dev_ht, rt5081_torch_level[level]);
	flashlight_set_strobe_brightness(
			flashlight_dev_ht, rt5081_strobe_level[level]);

	return 0;
}

int rt5081_set_level_lt(int level)
{
	level = rt5081_verify_level(level);
	rt5081_level_lt = level;

	if (!flashlight_dev_lt) {
		fl_err("Failed to set lt level since no flashlight device.\n");
		return -1;
	}

	/* set brightness level */
	if (!rt5081_is_torch(level))
		flashlight_set_torch_brightness(
				flashlight_dev_lt, rt5081_torch_level[level]);
	flashlight_set_strobe_brightness(
			flashlight_dev_lt, rt5081_strobe_level[level]);

	return 0;
}

static int rt5081_set_level(int ct_index, int level)
{
	if (ct_index == RT5081_CT_HT)
		rt5081_set_level_ht(level);
	else if (ct_index == RT5081_CT_LT)
		rt5081_set_level_lt(level);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

/* flashlight init */
static int rt5081_init(int scenario)
{
	/* clear flashlight state */
	rt5081_en_ht = RT5081_NONE;
	rt5081_en_lt = RT5081_NONE;

	/* notify charger to decrease voltage */
	if (scenario != FLASHLIGHT_SCENARIO_FLASHLIGHT) {
		fl_info("Decrease voltage level.\n");
		if (!flashlight_charger_consumer) {
			fl_err("Failed with no charger consumer handler.\n");
			return -1;
		}
		charger_manager_enable_high_voltage_charging(flashlight_charger_consumer, false);
		has_decrease_voltage = 1;
	}

	return 0;
}

/* flashlight uninit */
static int rt5081_uninit(void)
{
	/* clear flashlight state */
	rt5081_en_ht = RT5081_NONE;
	rt5081_en_lt = RT5081_NONE;

	/* notify charger to increase voltage */
	if (has_decrease_voltage) {
		fl_info("Increase voltage level.\n");
		if (!flashlight_charger_consumer) {
			fl_err("Failed with no charger consumer handler.\n");
			return -1;
		}
		charger_manager_enable_high_voltage_charging(flashlight_charger_consumer, true);
		has_decrease_voltage = 0;
	}

	return rt5081_disable();
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static void rt5081_work_disable_ht(struct work_struct *data)
{
	fl_dbg("ht work queue callback\n");
	rt5081_disable();
}

static void rt5081_work_disable_lt(struct work_struct *data)
{
	fl_dbg("lt work queue callback\n");
	rt5081_disable();
}

static enum hrtimer_restart fl_timer_func_ht(struct hrtimer *timer)
{
	schedule_work(&rt5081_work_ht);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart fl_timer_func_lt(struct hrtimer *timer)
{
	schedule_work(&rt5081_work_lt);
	return HRTIMER_NORESTART;
}

int rt5081_timer_start(int ct_index, ktime_t ktime)
{
	if (ct_index == RT5081_CT_HT)
		hrtimer_start(&fl_timer_ht, ktime, HRTIMER_MODE_REL);
	else if (ct_index == RT5081_CT_LT)
		hrtimer_start(&fl_timer_lt, ktime, HRTIMER_MODE_REL);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

int rt5081_timer_cancel(int ct_index)
{
	if (ct_index == RT5081_CT_HT)
		hrtimer_cancel(&fl_timer_ht);
	else if (ct_index == RT5081_CT_LT)
		hrtimer_cancel(&fl_timer_lt);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

/******************************************************************************
 * Flashlight operation wrapper function
 *****************************************************************************/
static int rt5081_operate(int ct_index, int enable)
{
	ktime_t ktime;

	/* setup enable/disable */
	if (ct_index == RT5081_CT_HT) {
		rt5081_en_ht = enable;
		if (rt5081_en_ht)
			if (rt5081_is_torch(rt5081_level_ht))
				rt5081_en_ht = RT5081_ENABLE_FLASH;
	} else if (ct_index == RT5081_CT_LT) {
		rt5081_en_lt = enable;
		if (rt5081_en_lt)
			if (rt5081_is_torch(rt5081_level_lt))
				rt5081_en_lt = RT5081_ENABLE_FLASH;
	} else {
		fl_err("Error ct index\n");
		return -1;
	}

	/* operate flashlight and setup timer */
	if ((rt5081_en_ht != RT5081_NONE) && (rt5081_en_lt != RT5081_NONE)) {
		if ((rt5081_en_ht == RT5081_DISABLE) &&
				(rt5081_en_lt == RT5081_DISABLE)) {
			rt5081_disable();
			rt5081_timer_cancel(RT5081_CT_HT);
			rt5081_timer_cancel(RT5081_CT_LT);
		} else {
			if (rt5081_timeout_ms[RT5081_CT_HT]) {
				ktime = ktime_set(
						rt5081_timeout_ms[RT5081_CT_HT] / 1000,
						(rt5081_timeout_ms[RT5081_CT_HT] % 1000) * 1000000);
				rt5081_timer_start(RT5081_CT_HT, ktime);
			}
			if (rt5081_timeout_ms[RT5081_CT_LT]) {
				ktime = ktime_set(
						rt5081_timeout_ms[RT5081_CT_LT] / 1000,
						(rt5081_timeout_ms[RT5081_CT_LT] % 1000) * 1000000);
				rt5081_timer_start(RT5081_CT_LT, ktime);
			}
			rt5081_enable();
		}

		/* clear flashlight state */
		rt5081_en_ht = RT5081_NONE;
		rt5081_en_lt = RT5081_NONE;
	}

	return 0;
}

/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int rt5081_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_user_arg *fl_arg;
	int ct_index;

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
		rt5081_timeout_ms[ct_index] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		rt5081_set_level(ct_index, fl_arg->arg);
		break;

	case FLASH_IOC_SET_STEP:
		fl_dbg("FLASH_IOC_SET_STEP(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		rt5081_operate(ct_index, fl_arg->arg);
		break;

	case FLASH_IOC_IS_CHARGER_READY:
		fl_dbg("FLASH_IOC_IS_CHARGER_READY(%d)\n", ct_index);
		fl_arg->arg = rt5081_is_charger_ready();

		break;

	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				ct_index, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int rt5081_open(void *pArg)
{
	/* Actual behavior move to set driver function since power saving issue */
	return 0;
}

static int rt5081_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&rt5081_mutex);
	use_count--;
	if (!use_count)
		rt5081_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&rt5081_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int rt5081_set_driver(int scenario)
{
	int ret = 0;

	/* init chip and set usage count */
	mutex_lock(&rt5081_mutex);
	if (!use_count)
		ret = rt5081_init(scenario);
	use_count++;
	mutex_unlock(&rt5081_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return ret;
}

static ssize_t rt5081_strobe_store(struct flashlight_arg arg)
{
#if 1
	rt5081_set_driver(FLASHLIGHT_SCENARIO_CAMERA);
	rt5081_set_level(arg.ct, arg.level);

	if (arg.level < 0)
		rt5081_operate(arg.ct, RT5081_DISABLE);
	else
		rt5081_operate(arg.ct, RT5081_ENABLE);

	msleep(arg.dur);
	rt5081_operate(arg.ct, RT5081_DISABLE);
	rt5081_release(NULL);
#endif

#if 0
	int i;

	rt5081_set_driver(FLASHLIGHT_SCENARIO_CAMERA);
	for (i = 0; i < RT5081_LEVEL_NUM; i++) {
		rt5081_set_level(RT5081_CT_HT, i);
		rt5081_set_level(RT5081_CT_LT, i);
		rt5081_operate(RT5081_CT_HT, RT5081_ENABLE);
		rt5081_operate(RT5081_CT_LT, RT5081_ENABLE);
		msleep(50);
		rt5081_operate(RT5081_CT_HT, RT5081_DISABLE);
		rt5081_operate(RT5081_CT_LT, RT5081_DISABLE);
		msleep(200);
	}
	rt5081_release(NULL);
#endif

	return 0;
}

static struct flashlight_operations rt5081_ops = {
	rt5081_open,
	rt5081_release,
	rt5081_ioctl,
	rt5081_strobe_store,
	rt5081_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int rt5081_probe(struct platform_device *pdev)
{
	fl_dbg("Probe start.\n");

	/* init work queue */
	INIT_WORK(&rt5081_work_ht, rt5081_work_disable_ht);
	INIT_WORK(&rt5081_work_lt, rt5081_work_disable_lt);

	/* init timer */
	hrtimer_init(&fl_timer_ht, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fl_timer_ht.function = fl_timer_func_ht;
	hrtimer_init(&fl_timer_lt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fl_timer_lt.function = fl_timer_func_lt;
	rt5081_timeout_ms[RT5081_CT_HT] = 200;
	rt5081_timeout_ms[RT5081_CT_LT] = 200;

	/* register flashlight operations */
	if (flashlight_dev_register(RT5081_NAME, &rt5081_ops))
		return -EFAULT;

	/* clear attributes */
	use_count = 0;
	has_decrease_voltage = 0;

	/* get RTK flashlight handler */
	flashlight_dev_ht = find_flashlight_by_name(RT_FLED_DEVICE_HT);
	if (!flashlight_dev_ht) {
		fl_err("Failed to get ht flashlight device.\n");
		return -EFAULT;
	}
	flashlight_dev_lt = find_flashlight_by_name(RT_FLED_DEVICE_LT);
	if (!flashlight_dev_lt) {
		fl_err("Failed to get lt flashlight device.\n");
		return -EFAULT;
	}

	/* setup strobe mode timeout */
	if (flashlight_set_strobe_timeout(flashlight_dev_ht, 400, 600))
		fl_err("Failed to set strobe timeout.\n");

	/* get charger consumer manager */
	flashlight_charger_consumer = charger_manager_get_by_name(&flashlight_dev_ht->dev, CHARGER_SUPPLY_NAME);
	if (!flashlight_charger_consumer) {
		fl_err("Failed to get charger manager.\n");
		return -EFAULT;
	}

	fl_dbg("Probe done.\n");

	return 0;
}

static int rt5081_remove(struct platform_device *pdev)
{
	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&rt5081_work_ht);
	flush_work(&rt5081_work_lt);

	/* unregister flashlight operations */
	flashlight_dev_unregister(RT5081_NAME);

	/* clear RTK flashlight device */
	flashlight_dev_ht = NULL;
	flashlight_dev_lt = NULL;

	fl_dbg("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rt5081_of_match[] = {
	{.compatible = RT5081_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, rt5081_of_match);
#else
static struct platform_device rt5081_platform_device[] = {
	{
		.name = RT5081_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, rt5081_platform_device);
#endif

static struct platform_driver rt5081_platform_driver = {
	.probe = rt5081_probe,
	.remove = rt5081_remove,
	.driver = {
		.name = RT5081_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt5081_of_match,
#endif
	},
};

static int __init flashlight_rt5081_init(void)
{
	int ret;

	fl_dbg("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&rt5081_platform_device);
	if (ret) {
		fl_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&rt5081_platform_driver);
	if (ret) {
		fl_err("Failed to register platform driver\n");
		return ret;
	}

	fl_dbg("Init done.\n");

	return 0;
}

static void __exit flashlight_rt5081_exit(void)
{
	fl_dbg("Exit start.\n");

	platform_driver_unregister(&rt5081_platform_driver);

	fl_dbg("Exit done.\n");
}

/* replace module_init() since conflict in kernel init process */
late_initcall(flashlight_rt5081_init);
module_exit(flashlight_rt5081_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight RT5081 Driver");


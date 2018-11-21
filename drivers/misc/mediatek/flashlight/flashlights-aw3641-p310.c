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

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"



/* define device tree */
#ifndef AW3641_DTNAME
#define AW3641_DTNAME "mediatek,flashlights_aw3641"
#endif

#define AW3641_NAME "flashlights-aw3641"

/* define registers */

/* define mutex and work queue */
static DEFINE_MUTEX(aw3641_mutex);
static struct work_struct aw3641_work;

/* define pinctrl */
#define AW3641_PINCTRL_PIN_HWEN 0
#define AW3641_PINCTRL_PIN_HWMODE 1
#define AW3641_PINCTRL_PINSTATE_LOW 0
#define AW3641_PINCTRL_PINSTATE_HIGH 1
#define AW3641_PINCTRL_STATE_HWEN_HIGH "hwen_high"
#define AW3641_PINCTRL_STATE_HWEN_LOW  "hwen_low"
#define AW3641_PINCTRL_STATE_HWMODE_HIGH "hwmode_high"
#define AW3641_PINCTRL_STATE_HWMODE_LOW  "hwmode_low"
#define AW3641_LEVEL_NUM 11
#define AW3641_LEVEL_TORCH 3

static struct pinctrl *aw3641_pinctrl=NULL;
static struct pinctrl_state *aw3641_hwen_high=NULL;
static struct pinctrl_state *aw3641_hwen_low=NULL;
static struct pinctrl_state *aw3641_hwmode_high=NULL;
static struct pinctrl_state *aw3641_hwmode_low=NULL;


/* define usage count */
static int use_count;

static int g_flash_duty = -1;

/* platform data */
struct aw3641_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int aw3641_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;
	pr_debug("Flashlight_use_gpio_probe enter \n");
	/* get pinctrl */
	aw3641_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(aw3641_pinctrl)) {
		 pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(aw3641_pinctrl);
	}

	/*  Flashlight pin initialization */
	aw3641_hwen_high = pinctrl_lookup_state(aw3641_pinctrl, "flashlightpin_en1");
	if (IS_ERR(aw3641_hwen_high)) {
		pr_err("Failed to init en (%s)\n", AW3641_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(aw3641_hwen_high);
	}
	aw3641_hwen_low = pinctrl_lookup_state(aw3641_pinctrl, "flashlightpin_en0");
	if (IS_ERR(aw3641_hwen_low)) {
		pr_err("Failed to init en(%s)\n", AW3641_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(aw3641_hwen_low);
	}

	aw3641_hwmode_high = pinctrl_lookup_state(aw3641_pinctrl, "flashlightpin_cfg1");
	if (IS_ERR(aw3641_hwen_high)) {
		pr_err("Failed to init mode high(%s)\n", AW3641_PINCTRL_STATE_HWMODE_HIGH);
		ret = PTR_ERR(aw3641_hwen_high);
	}
	aw3641_hwmode_low = pinctrl_lookup_state(aw3641_pinctrl, "flashlightpin_cfg0");
	if (IS_ERR(aw3641_hwen_low)) {
		pr_err("Failed to init mode low(%s)\n", AW3641_PINCTRL_STATE_HWMODE_LOW);
		ret = PTR_ERR(aw3641_hwen_low);
	}
    	pr_debug("Flashlight_use_gpio_probe exit\n");
	return ret;
}

static int aw3641_pinctrl_set(int pin,int state)
{
	int ret = 0;

	if (IS_ERR(aw3641_pinctrl)) {
		pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case AW3641_PINCTRL_PIN_HWEN:
		if (state == AW3641_PINCTRL_PINSTATE_LOW && !IS_ERR(aw3641_hwen_low)){
			ret = pinctrl_select_state(aw3641_pinctrl, aw3641_hwen_low);
			}
		else if (state == AW3641_PINCTRL_PINSTATE_HIGH && !IS_ERR(aw3641_hwen_high)){
			ret = pinctrl_select_state(aw3641_pinctrl, aw3641_hwen_high);
			}
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case AW3641_PINCTRL_PIN_HWMODE:
		if (state == AW3641_PINCTRL_PINSTATE_LOW && !IS_ERR(aw3641_hwmode_low)){
			ret = pinctrl_select_state(aw3641_pinctrl, aw3641_hwmode_low);
			pr_debug("flashmode is low");
			}
		else if (state == AW3641_PINCTRL_PINSTATE_HIGH && !IS_ERR(aw3641_hwmode_high)){
			ret = pinctrl_select_state(aw3641_pinctrl, aw3641_hwmode_high);
			pr_debug("flashmode is high");
			}
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d), ret:%d\n", pin, state, ret);

	return ret;
}

static int aw3641_is_torch(int level)
{
	if(level >= AW3641_LEVEL_TORCH)
		return -1;
	return 0;
}

static int aw3641_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= AW3641_LEVEL_NUM)
		level = AW3641_LEVEL_NUM;

	return level;
}
#if 0
static int aw3641_flash_cur_adjust(int num)
{
	int number,i;
	if (num>9)
		num=9;

	number=AW3641_LEVEL_NUM-num;
	for(i=0;i<number;i++)
	{
		aw3641_pinctrl_set(0,0);
		//pr_debug("en_low");
		udelay(1);
		aw3641_pinctrl_set(0,1);		
	}
	return 	number;	
}
#endif
/******************************************************************************
 * aw3641 operations
 *****************************************************************************/
/* flashlight enable function */
static int aw3641_enable(void)
{
	int pin = 0 ,state=1,i;

	if (aw3641_is_torch(g_flash_duty)== 0) {
		aw3641_pinctrl_set(pin, state);
		aw3641_pinctrl_set(pin+1, state-1);
	} else if(aw3641_is_torch(g_flash_duty)==-1){
		pinctrl_select_state(aw3641_pinctrl, aw3641_hwmode_high);
		pinctrl_select_state(aw3641_pinctrl, aw3641_hwen_high);
		for(i=0;i<AW3641_LEVEL_NUM-g_flash_duty;i++) {
			pinctrl_select_state(aw3641_pinctrl, aw3641_hwen_low);
			pinctrl_select_state(aw3641_pinctrl, aw3641_hwen_high);
		}
	}
	return 0;
}

/* flashlight disable function */
static int aw3641_disable(void)
{
	int pin = 0, state = 0;
	aw3641_pinctrl_set(pin+1, state);
	return aw3641_pinctrl_set(pin, state);
}

/* set flashlight level */
static int aw3641_set_level(int level)
{
 	level= aw3641_verify_level(level);
	g_flash_duty = level;
	return 0;
}

/* flashlight init */
static int aw3641_init(void)
{
	int pin = 0, state = 0;
	aw3641_pinctrl_set(pin+1, state);
	return aw3641_pinctrl_set(pin, state);
}

/* flashlight uninit */
static int aw3641_uninit(void)
{
	int pin = 0, state = 0;
	aw3641_pinctrl_set(pin+1, state);
	return aw3641_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer aw3641_timer;
static unsigned int aw3641_timeout_ms;

static void aw3641_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	aw3641_disable();
}

static enum hrtimer_restart aw3641_timer_func(struct hrtimer *timer)
{
	schedule_work(&aw3641_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int aw3641_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;
	printk("%s:  cmd =  %u", __func__, cmd);
	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("aw3641 FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		aw3641_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("aw3641_FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		aw3641_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("aw3641 FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (aw3641_timeout_ms) {
				ktime = ktime_set(aw3641_timeout_ms / 1000,
						(aw3641_timeout_ms % 1000) * 1000000);
				hrtimer_start(&aw3641_timer, ktime, HRTIMER_MODE_REL);
			}
			aw3641_enable();
		} else {
			aw3641_disable();
			hrtimer_cancel(&aw3641_timer);
		}
		break;
	default:
		pr_info("aw3641 No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int aw3641_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int aw3641_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int aw3641_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&aw3641_mutex);
	if (set) {
		if (!use_count)
			ret = aw3641_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = aw3641_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&aw3641_mutex);

	return ret;
}

static ssize_t aw3641_strobe_store(struct flashlight_arg arg)
{
	aw3641_set_driver(1);
	aw3641_set_level(arg.level);
	aw3641_timeout_ms = 0;
	aw3641_enable();
	msleep(arg.dur);
	aw3641_disable();
	aw3641_set_driver(0);

	return 0;
}

static struct flashlight_operations aw3641_ops = {
	aw3641_open,
	aw3641_release,
	aw3641_ioctl,
	aw3641_strobe_store,
	aw3641_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int aw3641_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * aw3641_init();
	 */

	return 0;
}

static int aw3641_parse_dt(struct device *dev,
		struct aw3641_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		pr_info("Parse no dt, node.\n");
		return 0;
	}
	pr_info("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		pr_info("Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num * sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE, AW3641_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel, pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int aw3641_probe(struct platform_device *pdev)
{
	struct aw3641_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;

	pr_debug("%s: Probe start.\n", __func__);

	/* init pinctrl */
	if (aw3641_pinctrl_init(pdev)) {
		pr_err("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		err = aw3641_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&aw3641_work, aw3641_work_disable);

	/* init timer */
	hrtimer_init(&aw3641_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw3641_timer.function = aw3641_timer_func;
	aw3641_timeout_ms = 100;

	/* init chip hw */
	aw3641_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(&pdata->dev_id[i], &aw3641_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(AW3641_NAME, &aw3641_ops)) {
			err = -EFAULT;
			goto err;
		}
	}

	pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int aw3641_remove(struct platform_device *pdev)
{
	struct aw3641_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(AW3641_NAME);

	/* flush work queue */
	flush_work(&aw3641_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw3641_gpio_of_match[] = {
	{.compatible = AW3641_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, aw3641_gpio_of_match);
#else
static struct platform_device aw3641_gpio_platform_device[] = {
	{
		.name = AW3641_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, aw3641_gpio_platform_device);
#endif

static struct platform_driver aw3641_platform_driver = {
	.probe = aw3641_probe,
	.remove = aw3641_remove,
	.driver = {
		.name = AW3641_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw3641_gpio_of_match,
#endif
	},
};

static int __init flashlight_aw3641_init(void)
{
	int ret;

	pr_debug("%s: Init start.\n", __func__);

#ifndef CONFIG_OF
	ret = platform_device_register(&aw3641_gpio_platform_device);
	if (ret) {
		pr_debug("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&aw3641_platform_driver);
	if (ret) {
		pr_debug("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_aw3641_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&aw3641_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_aw3641_init);
module_exit(flashlight_aw3641_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight AW3641 GPIO Driver");


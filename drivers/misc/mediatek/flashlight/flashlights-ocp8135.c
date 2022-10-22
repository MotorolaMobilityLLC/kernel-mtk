// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
#ifndef OCP8135_DTNAME
#define OCP8135_DTNAME "mediatek,flashlights_ocp8135"
#endif

#define OCP8135_NAME "flashlights_ocp8135"

/* define registers */

/* define mutex and work queue */
static DEFINE_MUTEX(ocp8135_mutex);
static struct work_struct ocp8135_work;

/* define pinctrl */
#define OCP8135_PINCTRL_PIN_HWEN 0
#define OCP8135_PINCTRL_PINSTATE_LOW 0
#define OCP8135_PINCTRL_PINSTATE_HIGH 1
#define OCP8135_PINCTRL_STATE_HW_CH0_HIGH "torch_en_pin1"
#define OCP8135_PINCTRL_STATE_HW_CH0_LOW  "torch_en_pin0"
#define OCP8135_PINCTRL_STATE_HW_CH1_HIGH "flash_en_pin1"
#define OCP8135_PINCTRL_STATE_HW_CH1_LOW  "flash_en_pin0"
static struct pinctrl *ocp8135_pinctrl;
static struct pinctrl_state *ocp8135_hw_ch0_high;
static struct pinctrl_state *ocp8135_hw_ch0_low;
static struct pinctrl_state *ocp8135_hw_ch1_high;
static struct pinctrl_state *ocp8135_hw_ch1_low;

/* define usage count */
static int use_count;

static int g_flash_duty = -1;
static int g_flash_channel_idx = 0;
/* platform data */
struct ocp8135_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int ocp8135_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	ocp8135_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(ocp8135_pinctrl)) {
		pr_info("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(ocp8135_pinctrl);
		return ret;
	}

	/*  Flashlight pin initialization */
	ocp8135_hw_ch0_high = pinctrl_lookup_state(ocp8135_pinctrl, OCP8135_PINCTRL_STATE_HW_CH0_HIGH);
	if (IS_ERR(ocp8135_hw_ch0_high)) {
		pr_info("Failed to init (%s)\n", OCP8135_PINCTRL_STATE_HW_CH0_HIGH);
		ret = PTR_ERR(ocp8135_hw_ch0_high);
	}
	ocp8135_hw_ch0_low = pinctrl_lookup_state(ocp8135_pinctrl, OCP8135_PINCTRL_STATE_HW_CH0_LOW);
	if (IS_ERR(ocp8135_hw_ch0_low)) {
		pr_info("Failed to init (%s)\n", OCP8135_PINCTRL_STATE_HW_CH0_LOW);
		ret = PTR_ERR(ocp8135_hw_ch0_low);
	}

	ocp8135_hw_ch1_high = pinctrl_lookup_state(ocp8135_pinctrl, OCP8135_PINCTRL_STATE_HW_CH1_HIGH);
	if (IS_ERR(ocp8135_hw_ch1_high)) {
		pr_info("Failed to init (%s)\n", OCP8135_PINCTRL_STATE_HW_CH1_HIGH);
		ret = PTR_ERR(ocp8135_hw_ch1_high);
	}
	ocp8135_hw_ch1_low = pinctrl_lookup_state(ocp8135_pinctrl, OCP8135_PINCTRL_STATE_HW_CH1_LOW);
	if (IS_ERR(ocp8135_hw_ch1_low)) {
		pr_info("Failed to init (%s)\n", OCP8135_PINCTRL_STATE_HW_CH1_LOW);
		ret = PTR_ERR(ocp8135_hw_ch1_low);
	}

	return ret;
}

static int ocp8135_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(ocp8135_pinctrl)) {
		pr_info("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case OCP8135_PINCTRL_PIN_HWEN:
		if (state == OCP8135_PINCTRL_PINSTATE_LOW
			&& !IS_ERR(ocp8135_hw_ch0_low) && !IS_ERR(ocp8135_hw_ch1_low))
		{
			ret = pinctrl_select_state(ocp8135_pinctrl, ocp8135_hw_ch0_low);
			ret = pinctrl_select_state(ocp8135_pinctrl, ocp8135_hw_ch1_low);
		}
		else if (state == OCP8135_PINCTRL_PINSTATE_HIGH
			&& !IS_ERR(ocp8135_hw_ch0_high) && !IS_ERR(ocp8135_hw_ch1_high) && !IS_ERR(ocp8135_hw_ch1_low))
		{
			ret = pinctrl_select_state(ocp8135_pinctrl, ocp8135_hw_ch0_high);
			if(g_flash_channel_idx == 1)
				ret = pinctrl_select_state(ocp8135_pinctrl, ocp8135_hw_ch1_high);
			else if(g_flash_channel_idx == 0)
				ret = pinctrl_select_state(ocp8135_pinctrl, ocp8135_hw_ch1_low);
		}
		break;
	default:
		pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d), ret:%d\n", pin, state, ret);

	return ret;
}


/******************************************************************************
 * ocp8135 operations
 *****************************************************************************/
/* flashlight enable function */
static int ocp8135_enable(void)
{
	int pin = OCP8135_PINCTRL_PIN_HWEN;

	if (g_flash_duty == 1) {
		ocp8135_pinctrl_set(pin, 1);
	} else {
		ocp8135_pinctrl_set(pin, 1);
		ocp8135_pinctrl_set(pin, 0);
	}
	ocp8135_pinctrl_set(pin, 1);

	return 0;
}

/* flashlight disable function */
static int ocp8135_disable(void)
{
	int pin = 0;
	int state = 0;

	return ocp8135_pinctrl_set(pin, state);
}

/* set flashlight level */
static int ocp8135_set_level(int level)
{
	g_flash_duty = level;
	return 0;
}

/* flashlight init */
static int ocp8135_init(void)
{
	int pin = 0;
	int state = 0;

	return ocp8135_pinctrl_set(pin, state);
}

/* flashlight uninit */
static int ocp8135_uninit(void)
{
	int pin = 0;
	int state = 0;

	return ocp8135_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer ocp8135_timer;
static unsigned int ocp8135_timeout_ms;

static void ocp8135_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	ocp8135_disable();
}

static enum hrtimer_restart ocp8135_timer_func(struct hrtimer *timer)
{
	schedule_work(&ocp8135_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int ocp8135_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp8135_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp8135_set_level(fl_arg->arg);
		if((int)fl_arg->arg >= 15)
			g_flash_channel_idx = 1;
		else
			g_flash_channel_idx = 0;
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (ocp8135_timeout_ms) {
				s = ocp8135_timeout_ms / 1000;
				ns = ocp8135_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&ocp8135_timer, ktime,
						HRTIMER_MODE_REL);
			}
			ocp8135_enable();
		} else {
			ocp8135_disable();
			hrtimer_cancel(&ocp8135_timer);
		}
		break;
	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int ocp8135_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp8135_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp8135_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&ocp8135_mutex);
	if (set) {
		if (!use_count)
			ret = ocp8135_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = ocp8135_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&ocp8135_mutex);

	return ret;
}

static ssize_t ocp8135_strobe_store(struct flashlight_arg arg)
{
	ocp8135_set_driver(1);
	ocp8135_set_level(arg.level);
	ocp8135_timeout_ms = 0;
	ocp8135_enable();
	msleep(arg.dur);
	ocp8135_disable();
	ocp8135_set_driver(0);

	return 0;
}

static struct flashlight_operations ocp8135_ops = {
	ocp8135_open,
	ocp8135_release,
	ocp8135_ioctl,
	ocp8135_strobe_store,
	ocp8135_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int ocp8135_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * ocp8135_init();
	 */

	return 0;
}

static int ocp8135_parse_dt(struct device *dev,
		struct ocp8135_platform_data *pdata)
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
			pdata->channel_num *
			sizeof(struct flashlight_device_id),
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
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
				OCP8135_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

#ifdef FLASH_NODE
static int led_flash_state = 0;
static ssize_t led_flash_show(struct device *dev, struct device_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", led_flash_state);
}
static ssize_t led_flash_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size){
	unsigned long value;
	int err;
	err = kstrtoul(buf, 10, &value);
	if(err != 0){
		return err;
	}
	switch(value){
	case 0:
		g_flash_channel_idx = 1;
		err = ocp8135_pinctrl_set(0, 0);
		if(err < 0)
			pr_info("AAA - error1 - AAA\n");
		led_flash_state = 0;
		break;
	case 1:
		g_flash_channel_idx = 1;
		err = ocp8135_pinctrl_set(0, 1);
		if(err < 0)
			pr_info("AAA - error1 - AAA\n");
		led_flash_state = 1;
		break;
	default :
		break;
	}
	return 1;
}
static DEVICE_ATTR(led_flash, 0664, led_flash_show, led_flash_store);
#endif


static int ocp8135_probe(struct platform_device *pdev)
{
	struct ocp8135_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;
	int ret;
	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (ocp8135_pinctrl_init(pdev)) {
		pr_debug("Failed to init pinctrl.\n");
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
		err = ocp8135_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&ocp8135_work, ocp8135_work_disable);

	/* init timer */
	hrtimer_init(&ocp8135_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ocp8135_timer.function = ocp8135_timer_func;
	ocp8135_timeout_ms = 100;

	/* init chip hw */
	ocp8135_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&ocp8135_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(OCP8135_NAME, &ocp8135_ops)) {
			err = -EFAULT;
			goto err;
		}
	}
        g_flash_channel_idx = 1;
	ret = ocp8135_pinctrl_set(0, 0);
	if(ret < 0)
		pr_info("AAA - error1 - AAA\n");
	g_flash_channel_idx = 0;
	ret = ocp8135_pinctrl_set(0, 0);
	if(ret < 0)
		pr_info("AAA - error2 - AAA\n");

#ifdef FLASH_NODE
	ret = device_create_file(&pdev->dev, &dev_attr_led_flash);
	if(ret < 0){
		pr_err("=== create led_flash_node file failed ===\n");
	}
#endif
	pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int ocp8135_remove(struct platform_device *pdev)
{
	struct ocp8135_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(OCP8135_NAME);

	/* flush work queue */
	flush_work(&ocp8135_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ocp8135_gpio_of_match[] = {
	{.compatible = OCP8135_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, ocp8135_gpio_of_match);
#else
static struct platform_device ocp8135_gpio_platform_device[] = {
	{
		.name = OCP8135_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, ocp8135_gpio_platform_device);
#endif

static struct platform_driver ocp8135_platform_driver = {
	.probe = ocp8135_probe,
	.remove = ocp8135_remove,
	.driver = {
		.name = OCP8135_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = ocp8135_gpio_of_match,
#endif
	},
};

static int __init flashlight_ocp8135_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&ocp8135_gpio_platform_device);
	if (ret) {
		pr_info("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&ocp8135_platform_driver);
	if (ret) {
		pr_info("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_ocp8135_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&ocp8135_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_ocp8135_init);
module_exit(flashlight_ocp8135_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight OCP8135 GPIO Driver");


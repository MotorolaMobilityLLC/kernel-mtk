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
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"

/* define device tree */
#ifndef RT4509_DTNAME
#define RT4509_DTNAME "mediatek,flashlights_rt4509"
#endif
#ifndef RT4509_DTNAME_I2C
#define RT4509_DTNAME_I2C "mediatek,flashlights_rt4509_i2c"
#endif

#define RT4509_I2C_NAME "strobe_rt4509"

#define RT4509_NAME "flashlights-rt4509"

#ifndef RT4509_2_DTNAME_I2C
#define RT4509_2_DTNAME_I2C "mediatek,flashlights_rt4509_2_i2c"
#endif

/* define registers */
#define RT4509_REG_DEV_INFO (0x00)

#define RT4509_REG_LD_CTRL (0x06)

#define RT4509_REG_PULSE_WIDTH1 (0x03)
#define RT4509_REG_PULSE_WIDTH2 (0x04)

#define RT4509_REG_PULSE_DELAY1 (0x01)
#define RT4509_REG_PULSE_DELAY2 (0x02)

#define RT4509_REG_EN_CTRL (0x17)
#define RT4509_LD_ENABLE (0x01)
#define RT4509_LD_DISABLE (0x00)

#define RT4509_REG_LD_STAT (0x30)

/* define level */
#define RT4509_LEVEL_NUM 15
#define RT4509_LEVEL_TORCH 0
#define RT4509_HW_TIMEOUT 100 /* ms */

/* define mutex and work queue */
static DEFINE_MUTEX(rt4509_mutex);
static struct work_struct rt4509_work;

/* define pinctrl */
#define RT4509_PINCTRL_PIN_HWEN 0
#define RT4509_PINCTRL_PIN_STRBHWEN 1
#define RT4509_PINCTRL_PINSTATE_LOW 0
#define RT4509_PINCTRL_PINSTATE_HIGH 1
#define RT4509_PINCTRL_STATE_HWEN_HIGH "rt4509_hwen_high"
#define RT4509_PINCTRL_STATE_HWEN_LOW  "rt4509_hwen_low"
#define RT4509_PINCTRL_STATE_STRBHWEN_HIGH "rt4509_strbhwen_high"
#define RT4509_PINCTRL_STATE_STRBHWEN_LOW  "rt4509_strbhwen_low"
static struct pinctrl *rt4509_pinctrl;
static struct pinctrl_state *rt4509_hwen_high;
static struct pinctrl_state *rt4509_hwen_low;
static struct pinctrl_state *rt4509_strbhwen_high;
static struct pinctrl_state *rt4509_strbhwen_low;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *rt4509_i2c_client;
static struct i2c_client *rt4509_i2c_client_2;

/* platform data */
struct rt4509_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};

static struct rt4509_platform_data *rt4509_pdata;

/* rt4509 chip data */
struct rt4509_chip_data {
	struct i2c_client *client;
	struct rt4509_platform_data *pdata;
	struct mutex lock;
};

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int rt4509_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	rt4509_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(rt4509_pinctrl)) {
		pr_info("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(rt4509_pinctrl);
		return ret;
	}

	/* Flashlight HWEN pin initialization */
	rt4509_hwen_high = pinctrl_lookup_state(rt4509_pinctrl, RT4509_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(rt4509_hwen_high)) {
		pr_info("Failed to init (%s)\n", RT4509_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(rt4509_hwen_high);
	}
	rt4509_hwen_low = pinctrl_lookup_state(rt4509_pinctrl, RT4509_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(rt4509_hwen_low)) {
		pr_info("Failed to init (%s)\n", RT4509_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(rt4509_hwen_low);
	}
	rt4509_strbhwen_high = pinctrl_lookup_state(
		rt4509_pinctrl, RT4509_PINCTRL_STATE_STRBHWEN_HIGH);
	if (IS_ERR(rt4509_strbhwen_high)) {
		pr_info("Failed to init (%s)\n", RT4509_PINCTRL_STATE_STRBHWEN_HIGH);
		ret = PTR_ERR(rt4509_strbhwen_high);
	}
	rt4509_strbhwen_low = pinctrl_lookup_state(
		rt4509_pinctrl, RT4509_PINCTRL_STATE_STRBHWEN_LOW);
	if (IS_ERR(rt4509_strbhwen_low)) {
		pr_info("Failed to init (%s)\n", RT4509_PINCTRL_STATE_STRBHWEN_LOW);
		ret = PTR_ERR(rt4509_strbhwen_low);
	}


	return ret;
}

static int rt4509_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(rt4509_pinctrl)) {
		pr_info("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case RT4509_PINCTRL_PIN_HWEN:
		if (state == RT4509_PINCTRL_PINSTATE_LOW && !IS_ERR(rt4509_hwen_low))
			pinctrl_select_state(rt4509_pinctrl, rt4509_hwen_low);
		else if (state == RT4509_PINCTRL_PINSTATE_HIGH && !IS_ERR(rt4509_hwen_high))
			pinctrl_select_state(rt4509_pinctrl, rt4509_hwen_high);
		else
			pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case RT4509_PINCTRL_PIN_STRBHWEN:
		if (state == RT4509_PINCTRL_PINSTATE_LOW && !IS_ERR(rt4509_strbhwen_low))
			pinctrl_select_state(rt4509_pinctrl, rt4509_strbhwen_low);
		else if (state == RT4509_PINCTRL_PINSTATE_HIGH && !IS_ERR(rt4509_strbhwen_high))
			pinctrl_select_state(rt4509_pinctrl, rt4509_strbhwen_high);
		else
			pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d)\n", pin, state);

	return ret;
}

/******************************************************************************
 * rt4509 operations
 *****************************************************************************/
static const int rt4509_current[RT4509_LEVEL_NUM] = {
	 40, 100, 140,  200,  240,  300, 340, 400, 500, 600,
	700, 800, 900, 1000, 1100
};

static const unsigned char rt4509_flash_level[RT4509_LEVEL_NUM] = {
	0x02, 0x05, 0x07, 0x0A, 0x0C, 0x0F, 0x11, 0x14, 0x19, 0x1E,
	0x23, 0x28, 0x2D, 0x32, 0x37
};

static int rt4509_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= RT4509_LEVEL_NUM)
		level = RT4509_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int rt4509_read_reg(struct i2c_client *client, u8 reg)
{
	int val = 0;
	struct rt4509_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);

	return val;
}

static int rt4509_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct rt4509_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	pr_info("client:%s writing at 0x%02x val(0x%02x)\n",
		client->name, reg, val);
	if (ret < 0)
		pr_info("failed writing at 0x%02x\n", reg);

	ret = rt4509_read_reg(client, RT4509_REG_LD_STAT);
	pr_info("status 0x%02x\n", ret);

	ret = rt4509_read_reg(client, reg);
	pr_info("value 0x%02x\n", ret);

	return ret;
}

/* flashlight enable function */
static int rt4509_enable(struct i2c_client *client)
{
	unsigned char reg = 0, val = 0;

	reg = RT4509_REG_EN_CTRL;
	val = RT4509_LD_ENABLE;

	return rt4509_write_reg(client, reg, val);
}

/* flashlight disable function */
static int rt4509_disable(struct i2c_client *client)
{
	unsigned char reg = 0, val = 0;

	reg = RT4509_REG_EN_CTRL;
	val = RT4509_LD_DISABLE;

	return rt4509_write_reg(client, reg, val);
}

/* set flashlight level */
static int rt4509_set_level(struct i2c_client *client, int level)
{
	unsigned char reg = 0, val = 0;

	level = rt4509_verify_level(level);

	reg = RT4509_REG_LD_CTRL;
	val = rt4509_flash_level[level];

	return rt4509_write_reg(client, reg, val);
}

/* enable rt4509 i2c control*/
static int rt4509_hidden_mode(struct i2c_client *client)
{
	rt4509_write_reg(client, 0xC0, 0x5A);
	rt4509_write_reg(client, 0xC1, 0xA5);
	rt4509_write_reg(client, 0xC2, 0x62);
	rt4509_write_reg(client, 0xC3, 0x86);
	rt4509_write_reg(client, 0xC4, 0x68);
	rt4509_write_reg(client, 0xC5, 0x26);
	rt4509_write_reg(client, 0xC6, 0x5A);
	rt4509_write_reg(client, 0xC7, 0xE1);
	rt4509_write_reg(client, 0xB0, 0xC0);

	mdelay(2);

	return 0;
}

static int rt4509_disable_i2c_control(void)
{
	rt4509_write_reg(rt4509_i2c_client, 0xB0, 0x00);

	return 0;
}


/* flashlight init */
static int rt4509_init(void)
{
	int ret = 0;

	rt4509_pinctrl_set(RT4509_PINCTRL_PIN_HWEN, RT4509_PINCTRL_PINSTATE_HIGH);
	rt4509_pinctrl_set(RT4509_PINCTRL_PIN_STRBHWEN, RT4509_PINCTRL_PINSTATE_HIGH);

	mdelay(5);

	ret = rt4509_read_reg(rt4509_i2c_client, RT4509_REG_DEV_INFO);
	pr_info("dev_info %d 0x%02x\n", ret, ret);
	if (ret == -EREMOTEIO) {
		int i;
		pr_info("rt4509 is not availible %p\n", rt4509_pdata);
		for (i = 0; i < rt4509_pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(&rt4509_pdata->dev_id[i]);
		return -1;
	}

	/* set pulse width to 10ms */
	rt4509_write_reg(rt4509_i2c_client, RT4509_REG_PULSE_WIDTH1, 0x27);
	rt4509_write_reg(rt4509_i2c_client, RT4509_REG_PULSE_WIDTH2, 0x10);

	/* set pulse width to 300us */
	rt4509_write_reg(rt4509_i2c_client_2, RT4509_REG_PULSE_WIDTH1, 0x01);
	rt4509_write_reg(rt4509_i2c_client_2, RT4509_REG_PULSE_WIDTH2, 0x2C);

	rt4509_write_reg(rt4509_i2c_client_2, RT4509_REG_PULSE_DELAY1, 0x00);
	rt4509_write_reg(rt4509_i2c_client_2, RT4509_REG_PULSE_DELAY2, 0x0A);

	return 0;
}

/* flashlight uninit */
static int rt4509_uninit(void)
{

	rt4509_pinctrl_set(RT4509_PINCTRL_PIN_HWEN, RT4509_PINCTRL_PINSTATE_LOW);
	rt4509_pinctrl_set(RT4509_PINCTRL_PIN_STRBHWEN, RT4509_PINCTRL_PINSTATE_LOW);

	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer rt4509_timer;
static unsigned int rt4509_timeout_ms;

static void rt4509_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	rt4509_disable(rt4509_i2c_client);
}

static enum hrtimer_restart rt4509_timer_func(struct hrtimer *timer)
{
	schedule_work(&rt4509_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int rt4509_ioctl(struct i2c_client *client,
		unsigned int cmd, unsigned long arg)
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
		rt4509_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		rt4509_set_level(client, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (rt4509_timeout_ms) {
				s = rt4509_timeout_ms / 1000;
				ns = rt4509_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&rt4509_timer, ktime,
						HRTIMER_MODE_REL);
			}
			rt4509_hidden_mode(client);
			rt4509_enable(client);
		} else if (fl_arg->arg == 2) {
			rt4509_enable(client);
		} else {
			rt4509_disable(client);
			rt4509_disable_i2c_control();
			hrtimer_cancel(&rt4509_timer);
		}
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
		pr_debug("FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = RT4509_LEVEL_NUM;
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_debug("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = RT4509_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = rt4509_verify_level(fl_arg->arg);
		pr_debug("FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = rt4509_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		pr_debug("FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = RT4509_HW_TIMEOUT;
		break;

	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int rt4509_ioctl_1(unsigned int cmd, unsigned long arg)
{
	return rt4509_ioctl(rt4509_i2c_client, cmd, arg);
}

static int rt4509_ioctl_2(unsigned int cmd, unsigned long arg)
{
	return rt4509_ioctl(rt4509_i2c_client_2, cmd, arg);
}

static int rt4509_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int rt4509_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int rt4509_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&rt4509_mutex);
	if (set) {
		if (!use_count)
			ret = rt4509_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = rt4509_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&rt4509_mutex);

	return ret;
}

static ssize_t rt4509_strobe_store(
		struct i2c_client *client, struct flashlight_arg arg)
{
	rt4509_set_driver(1);
	rt4509_set_level(client, arg.level);
	rt4509_timeout_ms = 0;
	rt4509_hidden_mode(client);
	rt4509_enable(client);
	msleep(arg.dur);
	rt4509_disable(client);
	rt4509_set_driver(0);

	return 0;
}

static ssize_t rt4509_strobe_store_1(struct flashlight_arg arg)
{
	return rt4509_strobe_store(rt4509_i2c_client, arg);
}

static ssize_t rt4509_strobe_store_2(struct flashlight_arg arg)
{
	return rt4509_strobe_store(rt4509_i2c_client_2, arg);
}

static struct flashlight_operations rt4509_ops = {
	rt4509_open,
	rt4509_release,
	rt4509_ioctl_1,
	rt4509_strobe_store_1,
	rt4509_set_driver
};

static struct flashlight_operations rt4509_ops_2 = {
	rt4509_open,
	rt4509_release,
	rt4509_ioctl_2,
	rt4509_strobe_store_2,
	rt4509_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int rt4509_chip_init(struct rt4509_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * rt4509_init();
	 */

	return 0;
}

static int rt4509_parse_dt(struct device *dev,
		struct rt4509_platform_data *pdata)
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
				RT4509_NAME);
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

static int rt4509_i2c_probe(
		struct i2c_client *client, const struct i2c_device_id *id)
{
	struct rt4509_chip_data *chip;
	int err;

	pr_info("Probe start.\n");
	if (client == NULL)
		pr_info("NULL\n");
	pr_info("i2c name %s addr %d\n", client->name, client->addr);

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_info("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct rt4509_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	i2c_set_clientdata(client, chip);
	if (!strncmp(client->name, RT4509_I2C_NAME, I2C_NAME_SIZE))
		rt4509_i2c_client = client;
	else
		rt4509_i2c_client_2 = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init chip hw */
	rt4509_chip_init(chip);

	pr_info("Probe done.\n");

	return 0;

err_out:
	return err;
}

static int rt4509_i2c_remove(struct i2c_client *client)
{
	struct rt4509_chip_data *chip = i2c_get_clientdata(client);

	pr_debug("Remove start.\n");

	client->dev.platform_data = NULL;

	/* free resource */
	kfree(chip);

	pr_debug("Remove done.\n");

	return 0;
}

static const struct i2c_device_id rt4509_i2c_id[] = {
	{RT4509_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt4509_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id rt4509_i2c_of_match[] = {
	{.compatible = RT4509_DTNAME_I2C},
	{.compatible = RT4509_2_DTNAME_I2C},
	{},
};
MODULE_DEVICE_TABLE(of, rt4509_i2c_of_match);
#endif

static struct i2c_driver rt4509_i2c_driver = {
	.driver = {
		.name = RT4509_NAME,
#ifdef CONFIG_OF
		.of_match_table = rt4509_i2c_of_match,
#endif
	},
	.probe = rt4509_i2c_probe,
	.remove = rt4509_i2c_remove,
	.id_table = rt4509_i2c_id,
};

/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int rt4509_probe(struct platform_device *pdev)
{
	struct rt4509_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct rt4509_chip_data *chip = NULL;
	int err;
	int i;

	pr_info("Probe start.\n");

	/* init pinctrl */
	if (rt4509_pinctrl_init(pdev)) {
		pr_info("Failed to init pinctrl.\n");
		return -1;
	}

	if (i2c_add_driver(&rt4509_i2c_driver)) {
		pr_debug("Failed to add i2c driver.\n");
		return -1;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err_free;
		}
		pdev->dev.platform_data = pdata;
		err = rt4509_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err_free;
	}

	rt4509_pdata = pdata;

	/* init work queue */
	INIT_WORK(&rt4509_work, rt4509_work_disable);

	/* init timer */
	hrtimer_init(&rt4509_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	rt4509_timer.function = rt4509_timer_func;
	rt4509_timeout_ms = 100;

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (pdata->dev_id[i].ct == 0) {
				if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&rt4509_ops)) {
					err = -EFAULT;
					goto err_free;
				}
			} else {
				if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&rt4509_ops_2)) {
					err = -EFAULT;
					goto err_free;
				}
			}
	} else {
		if (flashlight_dev_register(RT4509_NAME, &rt4509_ops)) {
			err = -EFAULT;
			goto err_free;
		}
	}

	pr_info("Probe done.\n");

	return 0;
err_free:
	pr_info("Probe error.\n");
	chip = i2c_get_clientdata(rt4509_i2c_client);
	i2c_set_clientdata(rt4509_i2c_client, NULL);
	kfree(chip);
	return err;
}

static int rt4509_remove(struct platform_device *pdev)
{
	struct rt4509_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	i2c_del_driver(&rt4509_i2c_driver);

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(RT4509_NAME);

	/* flush work queue */
	flush_work(&rt4509_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rt4509_of_match[] = {
	{.compatible = RT4509_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, rt4509_of_match);
#else
static struct platform_device rt4509_platform_device[] = {
	{
		.name = RT4509_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, rt4509_platform_device);
#endif

static struct platform_driver rt4509_platform_driver = {
	.probe = rt4509_probe,
	.remove = rt4509_remove,
	.driver = {
		.name = RT4509_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = rt4509_of_match,
#endif
	},
};

static int __init flashlight_rt4509_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&rt4509_platform_device);
	if (ret) {
		pr_info("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&rt4509_platform_driver);
	if (ret) {
		pr_info("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_rt4509_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&rt4509_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_rt4509_init);
module_exit(flashlight_rt4509_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roger Wang <Roger-HY.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight RT4509 Driver");


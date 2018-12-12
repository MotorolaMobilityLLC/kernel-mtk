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

/* device tree should be defined in flashlight-dt.h */
#ifndef HA7109_DTNAME
#define HA7109_DTNAME "mediatek,flashlights_ha7109"
#endif
#ifndef HA7109_DTNAME_I2C
#define HA7109_DTNAME_I2C "mediatek,flashlights_ha7109_i2c"
#endif

#define HA7109_NAME "flashlights-ha7109"

/* define registers */
#define HA7109_REG_WRITE_PROTECT (0x00)

#define HA7109_INDUCTOR_CURRENT_LIMIT (0x40)
#define HA7109_FLASH_RAMP_TIME        (0x00)
#define HA7109_FLASH_TIMEOUT          (0x07)

#define HA7109_REG_CURRENT_CONTROL (0x02)

#define HA7109_REG_ILDK_RANGE (0x01)
#define HA7109_REG_TORCH (0x01)
#define HA7109_REG_FLASH (0x00)
#define HA7109_REG_EN_GBP (0x02)

#define HA7109_REG_ENABLE (0x07)
#define HA7109_ENABLE_STANDBY (0x00)
#define HA7109_ENABLE_LASER (0x01)

#define HA7109_REG_WDT (0x09)
#define HA7109_REG_WDT_ENABLE (0x01)
#define HA7109_REG_WDT_DISABLE (0x00)

#define HA7109_REG_STATUS (0x0D)

/* define level */
#define HA7109_LEVEL_NUM 25
#define HA7109_LEVEL_TORCH 6
#define HA7109_HW_TIMEOUT 50 /* ms */

/* define mutex and work queue */
static DEFINE_MUTEX(ha7109_mutex);
static struct work_struct ha7109_work;

/* define pinctrl */
#define HA7109_PINCTRL_PIN_HWEN 0
#define HA7109_PINCTRL_PINSTATE_LOW 0
#define HA7109_PINCTRL_PINSTATE_HIGH 1
#define HA7109_PINCTRL_STATE_HWEN_HIGH "ha7109_hwen_high"
#define HA7109_PINCTRL_STATE_HWEN_LOW  "ha7109_hwen_low"
static struct pinctrl *ha7109_pinctrl;
static struct pinctrl_state *ha7109_hwen_high;
static struct pinctrl_state *ha7109_hwen_low;

/* define usage count */
static int use_count;

/* define i2c */
static struct i2c_client *ha7109_i2c_client;

/* platform data */
struct ha7109_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};

static struct ha7109_platform_data *ha7109_pdata;

/* ha7109 chip data */
struct ha7109_chip_data {
	struct i2c_client *client;
	struct ha7109_platform_data *pdata;
	struct mutex lock;
};

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int ha7109_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	ha7109_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(ha7109_pinctrl)) {
		pr_info("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(ha7109_pinctrl);
		return ret;
	}

	/* Flashlight HWEN pin initialization */
	ha7109_hwen_high = pinctrl_lookup_state(ha7109_pinctrl, HA7109_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(ha7109_hwen_high)) {
		pr_info("Failed to init (%s)\n", HA7109_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(ha7109_hwen_high);
	}
	ha7109_hwen_low = pinctrl_lookup_state(ha7109_pinctrl, HA7109_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(ha7109_hwen_low)) {
		pr_info("Failed to init (%s)\n", HA7109_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(ha7109_hwen_low);
	}

	return ret;
}

static int ha7109_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(ha7109_pinctrl)) {
		pr_info("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case HA7109_PINCTRL_PIN_HWEN:
		if (state == HA7109_PINCTRL_PINSTATE_LOW && !IS_ERR(ha7109_hwen_low))
			pinctrl_select_state(ha7109_pinctrl, ha7109_hwen_low);
		else if (state == HA7109_PINCTRL_PINSTATE_HIGH && !IS_ERR(ha7109_hwen_high))
			pinctrl_select_state(ha7109_pinctrl, ha7109_hwen_high);
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
 * ha7109 operations
 *****************************************************************************/
static const int ha7109_current[HA7109_LEVEL_NUM] = {
	  90,  120,  150,  180,  210,  240,  270,  300,  330,  360,
	 390,  420,  450,  480,  510,  540,  570,  600,  630,  660,
	 690,  720,  750,  780,  810
};

static const unsigned char ha7109_flash_level[HA7109_LEVEL_NUM] = {
	0x00, 0x2A, 0x55, 0x7F, 0xAA, 0xD5, 0x00, 0x0E, 0x1C, 0x2A,
	0x39, 0x47, 0x55, 0x63, 0x71, 0x7F, 0x8D, 0x9B, 0xAA, 0xB8,
	0xC6, 0xD5, 0xE3, 0xF1, 0xFF
};

static int ha7109_is_torch(int level)
{
	if (level >= HA7109_LEVEL_TORCH)
		return -1;

	return 0;
}

static int ha7109_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= HA7109_LEVEL_NUM)
		level = HA7109_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int ha7109_read_reg(struct i2c_client *client, u8 reg)
{
	int val = 0;
	struct ha7109_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);

	return val;
}

static int ha7109_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct ha7109_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	pr_info("writing at 0x%02x val(0x%02x)\n", reg, val);
	if (ret < 0)
		pr_info("failed writing at 0x%02x val(0x%02x)\n", reg, val);

	ret = ha7109_read_reg(ha7109_i2c_client, HA7109_REG_STATUS);
	pr_info("status 0x%02x\n", ret);

	ret = ha7109_read_reg(ha7109_i2c_client, reg);
	pr_info("value 0x%02x\n", ret);
	return ret;
}

/* flashlight enable function */
static int ha7109_enable(void)
{
	unsigned char reg, val;

	reg = HA7109_REG_ENABLE;
	val = HA7109_ENABLE_LASER;

	return ha7109_write_reg(ha7109_i2c_client, reg, val);
}

/* flashlight disable function */
static int ha7109_disable(void)
{
	unsigned char reg, val;

	reg = HA7109_REG_ENABLE;
	val = HA7109_ENABLE_STANDBY;

	return ha7109_write_reg(ha7109_i2c_client, reg, val);
}

/* set flashlight current */
static int ha7109_set_current(int level)
{
	unsigned char reg, val;

	level = ha7109_verify_level(level);

	reg = HA7109_REG_CURRENT_CONTROL;
	val = ha7109_flash_level[level];

	return ha7109_write_reg(ha7109_i2c_client, reg, val);
}

/* set flashlight ILDK range */
static int ha7109_set_range(int level)
{
	unsigned char reg, val;

	reg = HA7109_REG_ILDK_RANGE;

	if (!ha7109_is_torch(level)) {
		/* torch mode */
		val = HA7109_REG_TORCH;
	} else {
		/* flash mode */
		val = HA7109_REG_FLASH;
	}

	return ha7109_write_reg(ha7109_i2c_client, reg, val);
}

/* set flashlight level */
static int ha7109_set_level(int level)
{
	ha7109_set_current(level);
	ha7109_set_range(level);
	return 0;
}

extern int ha7109_strobe_trigger(int en)
{
	unsigned char val;

	if (en == 1)
		val = HA7109_ENABLE_LASER;
	else
		val = HA7109_ENABLE_STANDBY;

	return ha7109_write_reg(ha7109_i2c_client, HA7109_REG_ENABLE, val);
}

/* flashlight init */
int ha7109_init(void)
{
	int ret;

	ha7109_pinctrl_set(HA7109_PINCTRL_PIN_HWEN, HA7109_PINCTRL_PINSTATE_HIGH);

	msleep(20);
	/* Reduce soft-start time  */
	ha7109_i2c_client->addr = 0x33;
	ret = ha7109_write_reg(ha7109_i2c_client, HA7109_REG_WRITE_PROTECT, 0x5A);
	pr_info("addr:%d ret(%d).\n", ha7109_i2c_client->addr, ret);

	ret = ha7109_write_reg(ha7109_i2c_client, 0x2F, 0x20);
	pr_info("addr:%d ret(%d).\n", ha7109_i2c_client->addr, ret);

	ha7109_i2c_client->addr = 0x4F;
	ret = ha7109_write_reg(ha7109_i2c_client, HA7109_REG_WRITE_PROTECT, 0x5A);
	pr_info("addr:%d ret(%d).\n", ha7109_i2c_client->addr, ret);
	if (ret == -121) {
		pr_info("ha7109 is not availible %p\n", ha7109_pdata);
		flashlight_dev_unregister_by_device_id(ha7109_pdata->dev_id);
		return -1;
	}

	/* disable */
	ret = ha7109_write_reg(ha7109_i2c_client, HA7109_REG_ENABLE, HA7109_ENABLE_STANDBY);
	pr_info("disable (%d).\n", ret);

	/* WDT */
	ret = ha7109_write_reg(ha7109_i2c_client, HA7109_REG_WDT, HA7109_REG_WDT_DISABLE);
	pr_info("WDT return (%d).\n", ret);

	ret = ha7109_read_reg(ha7109_i2c_client, HA7109_REG_STATUS);
	pr_info("HA7109 status(%d).\n", ret);

	ha7109_set_level(0);
	return ret;
}

/* flashlight uninit */
int ha7109_uninit(void)
{
	ha7109_disable();
	ha7109_pinctrl_set(HA7109_PINCTRL_PIN_HWEN, HA7109_PINCTRL_PINSTATE_LOW);

	return 0;
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer ha7109_timer;
static unsigned int ha7109_timeout_ms;

static void ha7109_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	ha7109_disable();
}

static enum hrtimer_restart ha7109_timer_func(struct hrtimer *timer)
{
	schedule_work(&ha7109_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int ha7109_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		ha7109_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		ha7109_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (ha7109_timeout_ms) {
				ktime = ktime_set(ha7109_timeout_ms / 1000,
						(ha7109_timeout_ms % 1000) * 1000000);
				hrtimer_start(&ha7109_timer, ktime, HRTIMER_MODE_REL);
			}
			ha7109_enable();
		} else {
			ha7109_disable();
			hrtimer_cancel(&ha7109_timer);
		}
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
		pr_debug("FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = HA7109_LEVEL_NUM;
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_debug("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = HA7109_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = ha7109_verify_level(fl_arg->arg);
		pr_debug("FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = ha7109_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		pr_debug("FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = HA7109_HW_TIMEOUT;
		break;

	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int ha7109_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ha7109_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ha7109_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&ha7109_mutex);
	if (set) {
		if (!use_count)
			ret = ha7109_init();
		if (ret != -1)
			use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = ha7109_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&ha7109_mutex);

	return ret;
}

static ssize_t ha7109_strobe_store(struct flashlight_arg arg)
{
	ha7109_set_driver(1);
	ha7109_set_level(arg.level);
	ha7109_timeout_ms = 0;
	ha7109_enable();
	msleep(arg.dur);
	ha7109_disable();
	ha7109_set_driver(0);

	return 0;
}

static struct flashlight_operations ha7109_ops = {
	ha7109_open,
	ha7109_release,
	ha7109_ioctl,
	ha7109_strobe_store,
	ha7109_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int ha7109_chip_init(struct ha7109_chip_data *chip)
{
	/* NOTE: Chip initialication move to "set driver" operation for power saving issue.
	 * ha7109_init();
	 */

	return 0;
}

static int ha7109_parse_dt(struct device *dev,
		struct ha7109_platform_data *pdata)
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
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE, HA7109_NAME);
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

static int ha7109_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ha7109_chip_data *chip;
	int err;

	pr_debug("i2c probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_info("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}

	/* init chip private data */
	chip = kzalloc(sizeof(struct ha7109_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	i2c_set_clientdata(client, chip);
	ha7109_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init chip hw */
	ha7109_chip_init(chip);

	pr_debug("i2c probe done.\n");

	return 0;

err_out:
	return err;
}

static int ha7109_i2c_remove(struct i2c_client *client)
{
	struct ha7109_chip_data *chip = i2c_get_clientdata(client);

	pr_debug("Remove start.\n");

	client->dev.platform_data = NULL;

	/* free resource */
	kfree(chip);

	pr_debug("Remove done.\n");

	return 0;
}

static const struct i2c_device_id ha7109_i2c_id[] = {
	{HA7109_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ha7109_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id ha7109_i2c_of_match[] = {
	{.compatible = HA7109_DTNAME_I2C},
	{},
};
MODULE_DEVICE_TABLE(of, ha7109_i2c_of_match);
#endif

static struct i2c_driver ha7109_i2c_driver = {
	.driver = {
		.name = HA7109_NAME,
#ifdef CONFIG_OF
		.of_match_table = ha7109_i2c_of_match,
#endif
	},
	.probe = ha7109_i2c_probe,
	.remove = ha7109_i2c_remove,
	.id_table = ha7109_i2c_id,
};

/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int ha7109_probe(struct platform_device *pdev)
{
	struct ha7109_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct ha7109_chip_data *chip = NULL;
	int err;
	int i;

	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (ha7109_pinctrl_init(pdev)) {
		pr_info("Failed to init pinctrl.\n");
		return -1;
	}

	if (i2c_add_driver(&ha7109_i2c_driver)) {
		pr_info("Failed to add i2c driver.\n");
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
		err = ha7109_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err_free;
	}

	ha7109_pdata = pdata;

	/* init work queue */
	INIT_WORK(&ha7109_work, ha7109_work_disable);

	/* init timer */
	hrtimer_init(&ha7109_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ha7109_timer.function = ha7109_timer_func;
	ha7109_timeout_ms = 0;

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(&pdata->dev_id[i], &ha7109_ops)) {
				err = -EFAULT;
				goto err_free;
			}
	} else {
		if (flashlight_dev_register(HA7109_NAME, &ha7109_ops)) {
			err = -EFAULT;
			goto err_free;
		}
	}

	pr_debug("Probe done.\n");

	return 0;
err_free:
	i2c_set_clientdata(ha7109_i2c_client, NULL);
	kfree(chip);
	return err;
}

static int ha7109_remove(struct platform_device *pdev)
{
	struct ha7109_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	i2c_del_driver(&ha7109_i2c_driver);

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(HA7109_NAME);

	/* flush work queue */
	flush_work(&ha7109_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ha7109_of_match[] = {
	{.compatible = HA7109_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, ha7109_of_match);
#else
static struct platform_device ha7109_platform_device[] = {
	{
		.name = HA7109_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, ha7109_platform_device);
#endif

static struct platform_driver ha7109_platform_driver = {
	.probe = ha7109_probe,
	.remove = ha7109_remove,
	.driver = {
		.name = HA7109_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = ha7109_of_match,
#endif
	},
};

static int __init flashlight_ha7109_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&ha7109_platform_device);
	if (ret) {
		pr_info("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&ha7109_platform_driver);
	if (ret) {
		pr_info("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_ha7109_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&ha7109_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_ha7109_init);
module_exit(flashlight_ha7109_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roger Wang <Roger-HY.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight HA7109 Driver");


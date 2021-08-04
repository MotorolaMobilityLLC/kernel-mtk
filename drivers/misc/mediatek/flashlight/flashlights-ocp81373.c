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
#ifndef OCP81373_DTNAME
#define OCP81373_DTNAME "mediatek,flashlights_ocp81373"
#endif
#ifndef OCP81373_DTNAME_I2C
#define OCP81373_DTNAME_I2C "mediatek,flashlights_ocp81373_i2c"
#endif

#define OCP81373_NAME "flashlights-ocp81373"

/* define registers */
#define OCP81373_REG_ENABLE (0x01)
#define OCP81373_MASK_ENABLE_LED1 (0x01)
#define OCP81373_MASK_ENABLE_LED2 (0x02)
#define OCP81373_DISABLE (0x00)
#define OCP81373_ENABLE_LED1 (0x01)
#define OCP81373_ENABLE_LED1_TORCH (0x09)
#define OCP81373_ENABLE_LED1_FLASH (0x0D)
#define OCP81373_ENABLE_LED2 (0x02)
#define OCP81373_ENABLE_LED2_TORCH (0x0A)
#define OCP81373_ENABLE_LED2_FLASH (0x0E)

#define OCP81373_REG_TORCH_LEVEL_LED1 (0x05)
#define OCP81373_REG_FLASH_LEVEL_LED1 (0x03)
#define OCP81373_REG_TORCH_LEVEL_LED2 (0x06)
#define OCP81373_REG_FLASH_LEVEL_LED2 (0x04)

#define OCP81373_REG_TIMING_CONF (0x08)
#define OCP81373_TORCH_RAMP_TIME (0x10)
#define OCP81373_FLASH_TIMEOUT   (0x0F)

#define OCP81373_REG_FLAG1 (0x0A)
#define OCP81373_REG_FLAG2 (0x0B)
#define OCP81373_REG_DEVICE_ID (0x0C)
#define OCP81373_VER_DEVICE_ID (0x3A)
/* define channel, level */
#define OCP81373_CHANNEL_NUM 2
#define OCP81373_CHANNEL_CH1 0
#define OCP81373_CHANNEL_CH2 1

#define OCP81373_LEVEL_NUM 26
#define OCP81373_LEVEL_TORCH 7

#define OCP81373_HW_TIMEOUT 600 /* ms */

/* define mutex and work queue */
static DEFINE_MUTEX(ocp81373_mutex);
static struct work_struct ocp81373_work_ch1;
static struct work_struct ocp81373_work_ch2;

/* define pinctrl */
#define OCP81373_PINCTRL_PIN_HWEN 0
#define OCP81373_PINCTRL_PINSTATE_LOW 0
#define OCP81373_PINCTRL_PINSTATE_HIGH 1
#define OCP81373_PINCTRL_STATE_HWEN_HIGH "flashlight_hwen_high"
#define OCP81373_PINCTRL_STATE_HWEN_LOW  "flashlight_hwen_low"
static struct pinctrl *ocp81373_pinctrl;
static struct pinctrl_state *ocp81373_hwen_high;
static struct pinctrl_state *ocp81373_hwen_low;

/* define usage count */
static int use_count;

static int ocp81373_chip_id;
/* define i2c */
static struct i2c_client *ocp81373_i2c_client;

/* platform data */
struct ocp81373_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};

/* ocp81373 chip data */
struct ocp81373_chip_data {
	struct i2c_client *client;
	struct ocp81373_platform_data *pdata;
	struct mutex lock;
};

struct ocp81373_platform_data *g_ocp81373_pdata;

/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int ocp81373_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	ocp81373_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(ocp81373_pinctrl)) {
		pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(ocp81373_pinctrl);
	}

	/* Flashlight HWEN pin initialization */
	ocp81373_hwen_high = pinctrl_lookup_state(
			ocp81373_pinctrl, OCP81373_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(ocp81373_hwen_high)) {
		pr_err("Failed to init (%s)\n", OCP81373_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(ocp81373_hwen_high);
	}
	ocp81373_hwen_low = pinctrl_lookup_state(
			ocp81373_pinctrl, OCP81373_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(ocp81373_hwen_low)) {
		pr_err("Failed to init (%s)\n", OCP81373_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(ocp81373_hwen_low);
	}

	return ret;
}

static int ocp81373_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(ocp81373_pinctrl)) {
		pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case OCP81373_PINCTRL_PIN_HWEN:
		if (state == OCP81373_PINCTRL_PINSTATE_LOW &&
				!IS_ERR(ocp81373_hwen_low))
			pinctrl_select_state(ocp81373_pinctrl, ocp81373_hwen_low);
		else if (state == OCP81373_PINCTRL_PINSTATE_HIGH &&
				!IS_ERR(ocp81373_hwen_high))
			pinctrl_select_state(ocp81373_pinctrl, ocp81373_hwen_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_info("pin(%d) state(%d)\n", pin, state);

	return ret;
}

/******************************************************************************
 * ocp81373 operations
 *****************************************************************************/
static const int ocp81373_current[OCP81373_LEVEL_NUM] = {
	 22,  46,  70,  93,  116, 140, 163, 198, 245, 304,
	351, 398, 445, 503,  550, 597, 656, 703, 750, 796,
	855, 902, 949, 996, 1054, 1101
};

static const unsigned char ocp81373_torch_level[OCP81373_LEVEL_NUM] = {
	0x13, 0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char ocp81373_flash_level[OCP81373_LEVEL_NUM] = {
	0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x10, 0x14, 0x19,
	0x1D, 0x21, 0x25, 0x2A, 0x2E, 0x32, 0x37, 0x3B, 0x3F, 0x43,
	0x48, 0x4C, 0x50, 0x54, 0x59, 0x5D
};

static unsigned char ocp81373_reg_enable;
static int ocp81373_level_ch1 = -1;
static int ocp81373_level_ch2 = -1;

static int ocp81373_is_torch(int level)
{
	if (level >= OCP81373_LEVEL_TORCH)
		return -1;

	return 0;
}

static int ocp81373_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= OCP81373_LEVEL_NUM)
		level = OCP81373_LEVEL_NUM - 1;

	return level;
}

/* i2c wrapper function */
static int ocp81373_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret;
	struct ocp81373_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);

	if (ret < 0)
		pr_err("failed writing at 0x%02x\n", reg);

	return ret;
}

static int ocp81373_read_reg(struct i2c_client *client, u8 reg)
{
	int val;
	struct ocp81373_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	val = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);

	return val;
}

/* flashlight enable function */
static int ocp81373_enable_ch1(void)
{
	unsigned char reg, val;

	reg = OCP81373_REG_ENABLE;
	if (!ocp81373_is_torch(ocp81373_level_ch1)) {
		/* torch mode */
		ocp81373_reg_enable |= OCP81373_ENABLE_LED1_TORCH;
	} else {
		/* flash mode */
		ocp81373_reg_enable |= OCP81373_ENABLE_LED1_FLASH;
	}
	val = ocp81373_reg_enable;

	return ocp81373_write_reg(ocp81373_i2c_client, reg, val);
}

static int ocp81373_enable_ch2(void)
{
	unsigned char reg, val;

	reg = OCP81373_REG_ENABLE;
	if (!ocp81373_is_torch(ocp81373_level_ch2)) {
		/* torch mode */
		ocp81373_reg_enable |= OCP81373_ENABLE_LED2_TORCH;
	} else {
		/* flash mode */
		ocp81373_reg_enable |= OCP81373_ENABLE_LED2_FLASH;
	}
	val = ocp81373_reg_enable;

	return ocp81373_write_reg(ocp81373_i2c_client, reg, val);
}

static int ocp81373_enable(int channel)
{
	if (channel == OCP81373_CHANNEL_CH1)
		ocp81373_enable_ch1();
	else if (channel == OCP81373_CHANNEL_CH2)
		ocp81373_enable_ch2();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* flashlight disable function */
static int ocp81373_disable_ch1(void)
{
	unsigned char reg, val;

	reg = OCP81373_REG_ENABLE;
	if (ocp81373_reg_enable & OCP81373_MASK_ENABLE_LED2) {
		/* if LED 2 is enable, disable LED 1 */
		ocp81373_reg_enable &= (~OCP81373_ENABLE_LED1);
	} else {
		/* if LED 2 is disable, disable LED 1 and clear mode */
		ocp81373_reg_enable &= (~OCP81373_ENABLE_LED1_FLASH);
	}
	val = ocp81373_reg_enable;

	return ocp81373_write_reg(ocp81373_i2c_client, reg, val);
}

static int ocp81373_disable_ch2(void)
{
	unsigned char reg, val;

	reg = OCP81373_REG_ENABLE;
	if (ocp81373_reg_enable & OCP81373_MASK_ENABLE_LED1) {
		/* if LED 1 is enable, disable LED 2 */
		ocp81373_reg_enable &= (~OCP81373_ENABLE_LED2);
	} else {
		/* if LED 1 is disable, disable LED 2 and clear mode */
		ocp81373_reg_enable &= (~OCP81373_ENABLE_LED2_FLASH);
	}
	val = ocp81373_reg_enable;

	return ocp81373_write_reg(ocp81373_i2c_client, reg, val);
}

static int ocp81373_disable(int channel)
{
	if (channel == OCP81373_CHANNEL_CH1)
		ocp81373_disable_ch1();
	else if (channel == OCP81373_CHANNEL_CH2)
		ocp81373_disable_ch2();
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

/* set flashlight level */
static int ocp81373_set_level_ch1(int level)
{
	int ret;
	unsigned char reg, val;

	level = ocp81373_verify_level(level);

	/* set torch brightness level */
	reg = OCP81373_REG_TORCH_LEVEL_LED1;
	val = ocp81373_torch_level[level];
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);

	ocp81373_level_ch1 = level;

	/* set flash brightness level */
	reg = OCP81373_REG_FLASH_LEVEL_LED1;
	val = ocp81373_flash_level[level];
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);

	return ret;
}

static int ocp81373_set_level_ch2(int level)
{
	int ret;
	unsigned char reg, val;

	level = ocp81373_verify_level(level);

	/* set torch brightness level */
	reg = OCP81373_REG_TORCH_LEVEL_LED2;
	val = ocp81373_torch_level[level];
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);

	ocp81373_level_ch2 = level;

	/* set flash brightness level */
	reg = OCP81373_REG_FLASH_LEVEL_LED2;
	val = ocp81373_flash_level[level];
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);

	return ret;
}

static int ocp81373_set_level(int channel, int level)
{
	if (channel == OCP81373_CHANNEL_CH1)
		ocp81373_set_level_ch1(level);
	else if (channel == OCP81373_CHANNEL_CH2)
		ocp81373_set_level_ch2(level);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

static int ocp81373_get_flag(int num)
{
	if (num == 1)
		return ocp81373_read_reg(ocp81373_i2c_client, OCP81373_REG_FLAG1);
	else if (num == 2)
		return ocp81373_read_reg(ocp81373_i2c_client, OCP81373_REG_FLAG2);

	pr_info("Error num\n");
	return 0;
}

/* flashlight init */
int ocp81373_init(void)
{
	int ret;
	unsigned char reg, val;

	ocp81373_pinctrl_set(
			OCP81373_PINCTRL_PIN_HWEN, OCP81373_PINCTRL_PINSTATE_HIGH);
	msleep(20);

	ocp81373_chip_id = ocp81373_read_reg(ocp81373_i2c_client, OCP81373_REG_DEVICE_ID);
        pr_err("gt++ banben = 0x%x", ocp81373_chip_id);

        if (OCP81373_VER_DEVICE_ID != ocp81373_chip_id)
	{
	    return -1;
	}
	/* clear enable register */
	reg = OCP81373_REG_ENABLE;
	val = OCP81373_DISABLE;
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);

	ocp81373_reg_enable = val;

	/* set torch current ramp time and flash timeout */
	reg = OCP81373_REG_TIMING_CONF;
	val = OCP81373_TORCH_RAMP_TIME | OCP81373_FLASH_TIMEOUT;
	ret = ocp81373_write_reg(ocp81373_i2c_client, reg, val);
	pr_err("gt++ ret = %d", ret);
	return ret;
}

/* flashlight uninit */
int ocp81373_uninit(void)
{
	ocp81373_disable(OCP81373_CHANNEL_CH1);
	ocp81373_disable(OCP81373_CHANNEL_CH2);

	ocp81373_pinctrl_set(
			OCP81373_PINCTRL_PIN_HWEN, OCP81373_PINCTRL_PINSTATE_LOW);

	return 0;
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer ocp81373_timer_ch1;
static struct hrtimer ocp81373_timer_ch2;
static unsigned int ocp81373_timeout_ms[OCP81373_CHANNEL_NUM];

static void ocp81373_work_disable_ch1(struct work_struct *data)
{
	pr_debug("ht work queue callback\n");
	ocp81373_disable_ch1();
}

static void ocp81373_work_disable_ch2(struct work_struct *data)
{
	pr_info("lt work queue callback\n");
	ocp81373_disable_ch2();
}

static enum hrtimer_restart ocp81373_timer_func_ch1(struct hrtimer *timer)
{
	schedule_work(&ocp81373_work_ch1);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart ocp81373_timer_func_ch2(struct hrtimer *timer)
{
	schedule_work(&ocp81373_work_ch2);
	return HRTIMER_NORESTART;
}

static int ocp81373_timer_start(int channel, ktime_t ktime)
{
	if (channel == OCP81373_CHANNEL_CH1)
		hrtimer_start(&ocp81373_timer_ch1, ktime, HRTIMER_MODE_REL);
	else if (channel == OCP81373_CHANNEL_CH2)
		hrtimer_start(&ocp81373_timer_ch2, ktime, HRTIMER_MODE_REL);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}

static int ocp81373_timer_cancel(int channel)
{
	if (channel == OCP81373_CHANNEL_CH1)
		hrtimer_cancel(&ocp81373_timer_ch1);
	else if (channel == OCP81373_CHANNEL_CH2)
		hrtimer_cancel(&ocp81373_timer_ch2);
	else {
		pr_err("Error channel\n");
		return -1;
	}

	return 0;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int ocp81373_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	/* verify channel */
	if (channel < 0 || channel >= OCP81373_CHANNEL_NUM) {
		pr_err("Failed with error channel\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_info("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp81373_timeout_ms[channel] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_info("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		ocp81373_set_level(channel, fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_info("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (ocp81373_timeout_ms[channel]) {
				s = ocp81373_timeout_ms[channel] / 1000;
				ns = ocp81373_timeout_ms[channel] % 1000
					* 1000000;
				ktime = ktime_set(s, ns);
				ocp81373_timer_start(channel, ktime);
			}
			ocp81373_enable(channel);
		} else {
			ocp81373_disable(channel);
			ocp81373_timer_cancel(channel);
		}
		break;

	case FLASH_IOC_GET_DUTY_NUMBER:
		pr_info("FLASH_IOC_GET_DUTY_NUMBER(%d)\n", channel);
		fl_arg->arg = OCP81373_LEVEL_NUM;
		break;

	case FLASH_IOC_GET_MAX_TORCH_DUTY:
		pr_info("FLASH_IOC_GET_MAX_TORCH_DUTY(%d)\n", channel);
		fl_arg->arg = OCP81373_LEVEL_TORCH - 1;
		break;

	case FLASH_IOC_GET_DUTY_CURRENT:
		fl_arg->arg = ocp81373_verify_level(fl_arg->arg);
		pr_info("FLASH_IOC_GET_DUTY_CURRENT(%d): %d\n",
				channel, (int)fl_arg->arg);
		fl_arg->arg = ocp81373_current[fl_arg->arg];
		break;

	case FLASH_IOC_GET_HW_TIMEOUT:
		pr_info("FLASH_IOC_GET_HW_TIMEOUT(%d)\n", channel);
		fl_arg->arg = OCP81373_HW_TIMEOUT;
		break;

	case FLASH_IOC_GET_HW_FAULT:
		pr_info("FLASH_IOC_GET_HW_FAULT(%d)\n", channel);
		fl_arg->arg = ocp81373_get_flag(1);
		break;

	case FLASH_IOC_GET_HW_FAULT2:
		pr_info("FLASH_IOC_GET_HW_FAULT2(%d)\n", channel);
		fl_arg->arg = ocp81373_get_flag(2);
		break;

	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int ocp81373_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp81373_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int ocp81373_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&ocp81373_mutex);
	if (set) {
		if (!use_count)
			ret = ocp81373_init();
		use_count++;
		pr_info("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = ocp81373_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_info("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&ocp81373_mutex);

	return ret;
}

static ssize_t ocp81373_strobe_store(struct flashlight_arg arg)
{
	ocp81373_set_driver(1);
	ocp81373_set_level(arg.channel, arg.level);
	ocp81373_timeout_ms[arg.channel] = 0;
	ocp81373_enable(arg.channel);
	msleep(arg.dur);
	ocp81373_disable(arg.channel);
	ocp81373_set_driver(0);

	return 0;
}

static struct flashlight_operations ocp81373_ops = {
	ocp81373_open,
	ocp81373_release,
	ocp81373_ioctl,
	ocp81373_strobe_store,
	ocp81373_set_driver
};


/******************************************************************************
 * I2C device and driver
 *****************************************************************************/
static int ocp81373_chip_init(struct ocp81373_chip_data *chip)
{
	int ver = 0;
	ver = ocp81373_init();

	return ver;
}

static int ocp81373_parse_dt(struct device *dev,
		struct ocp81373_platform_data *pdata)
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
	pr_err("Channel number(%d).\n", pdata->channel_num);

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
				OCP81373_NAME);
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

static int ocp81373_i2c_probe(
		struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ocp81373_chip_data *chip;
	int err;
	int i;

	pr_debug("i2c probe start.\n");

	/* check i2c */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("Failed to check i2c functionality.\n");
		err = -ENODEV;
		goto err_out;
	}
	/* init chip private data */
	chip = kzalloc(sizeof(struct ocp81373_chip_data), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err_out;
	}
	chip->client = client;

	i2c_set_clientdata(client, chip);
	ocp81373_i2c_client = client;

	/* init mutex and spinlock */
	mutex_init(&chip->lock);

	/* init chip hw */
	if (ocp81373_chip_init(chip))
	{
                err = -ENODEV;
 		goto err_out;
	}

	/* register flashlight device */
	pr_err("gt++ channel_num = %d", g_ocp81373_pdata->channel_num);
	if (g_ocp81373_pdata->channel_num) {
		for (i = 0; i < g_ocp81373_pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
					&g_ocp81373_pdata->dev_id[i],
					&ocp81373_ops)) {
				err = -EFAULT;
				goto err_out;
			}
	} else {
		if (flashlight_dev_register(OCP81373_NAME, &ocp81373_ops)) {
			err = -EFAULT;
			goto err_out;
		}
	}

	pr_debug("gt++ i2c probe done.\n");

	return 0;

err_out:
	pr_err("i2c probe failed");
	return err;
}

static int ocp81373_i2c_remove(struct i2c_client *client)
{
	struct ocp81373_chip_data *chip = i2c_get_clientdata(client);

	pr_info("Remove start.\n");

	client->dev.platform_data = NULL;

	/* free resource */
	kfree(chip);

	pr_info("Remove done.\n");

	return 0;
}

static const struct i2c_device_id ocp81373_i2c_id[] = {
	{OCP81373_NAME, 0},
	{}
};

#ifdef CONFIG_OF
static const struct of_device_id ocp81373_i2c_of_match[] = {
	{.compatible = OCP81373_DTNAME_I2C},
	{},
};
#endif

static struct i2c_driver ocp81373_i2c_driver = {
	.driver = {
		.name = OCP81373_NAME,
#ifdef CONFIG_OF
		.of_match_table = ocp81373_i2c_of_match,
#endif
	},
	.probe = ocp81373_i2c_probe,
	.remove = ocp81373_i2c_remove,
	.id_table = ocp81373_i2c_id,
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int ocp81373_probe(struct platform_device *pdev)
{
	struct ocp81373_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct ocp81373_chip_data *chip = NULL;
	int err;
	//int i;

	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (ocp81373_pinctrl_init(pdev)) {
		pr_info("Failed to init pinctrl.\n");
		return -1;
	}

	if (i2c_add_driver(&ocp81373_i2c_driver)) {
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
		err = ocp81373_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err_free;
	}

	/* init work queue */
	INIT_WORK(&ocp81373_work_ch1, ocp81373_work_disable_ch1);
	INIT_WORK(&ocp81373_work_ch2, ocp81373_work_disable_ch2);

	/* init timer */
	hrtimer_init(&ocp81373_timer_ch1, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ocp81373_timer_ch1.function = ocp81373_timer_func_ch1;
	hrtimer_init(&ocp81373_timer_ch2, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ocp81373_timer_ch2.function = ocp81373_timer_func_ch2;
	ocp81373_timeout_ms[OCP81373_CHANNEL_CH1] = 100;
	ocp81373_timeout_ms[OCP81373_CHANNEL_CH2] = 100;

	/* clear usage count */
	use_count = 0;
	g_ocp81373_pdata = pdata;

	pr_debug("Probe done.\n");

	return 0;
err_free:
	chip = i2c_get_clientdata(ocp81373_i2c_client);
	i2c_set_clientdata(ocp81373_i2c_client, NULL);
	kfree(chip);
	return err;
}

static int ocp81373_remove(struct platform_device *pdev)
{
	struct ocp81373_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_err("Remove start.\n");

	i2c_del_driver(&ocp81373_i2c_driver);

	/* unregister flashlight device */
        if (OCP81373_VER_DEVICE_ID == ocp81373_chip_id) {
		if (pdata && pdata->channel_num)
			for (i = 0; i < pdata->channel_num; i++)
				flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
		else
			flashlight_dev_unregister(OCP81373_NAME);
        }

	/* flush work queue */
	flush_work(&ocp81373_work_ch1);
	flush_work(&ocp81373_work_ch2);

	pr_err("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ocp81373_of_match[] = {
	{.compatible = OCP81373_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, ocp81373_of_match);
#else
static struct platform_device ocp81373_platform_device[] = {
	{
		.name = OCP81373_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, ocp81373_platform_device);
#endif

static struct platform_driver ocp81373_platform_driver = {
	.probe = ocp81373_probe,
	.remove = ocp81373_remove,
	.driver = {
		.name = OCP81373_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = ocp81373_of_match,
#endif
	},
};

static int __init flashlight_ocp81373_init(void)
{
	int ret;

	pr_info("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&ocp81373_platform_device);
	if (ret) {
		pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&ocp81373_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	pr_info("Init done.\n");

	return 0;
}

static void __exit flashlight_ocp81373_exit(void)
{
	pr_info("Exit start.\n");

	platform_driver_unregister(&ocp81373_platform_driver);

	pr_info("Exit done.\n");
}

module_init(flashlight_ocp81373_init);
module_exit(flashlight_ocp81373_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight OCP81373 Driver");


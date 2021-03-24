// SPDX-License-Identifier: GPL-2.0
// TI LM3697 LED chip family driver
// Copyright (C) 2018 Texas Instruments Incorporated - https://www.ti.com/

#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <uapi/linux/uleds.h>

#define LM3697_REV			0x0
#define LM3697_RESET			0x1
#define LM3697_OUTPUT_CONFIG		0x10
#define LM3697_CTRL_A_B_BRT_CFG		0x16
#define LM3697_FEEDBACK_ENABLE		0x19
#define LM3697_BOOST_CTRL		0x1a

#define LM3697_CTRL_A_BRT_LSB		0x20
#define LM3697_CTRL_A_BRT_MSB		0x21
#define LM3697_CTRL_ENABLE		0x24

#define LM3697_SW_RESET		BIT(0)

#define LM3697_CTRL_A_EN	BIT(0)
#define LM3697_CTRL_B_EN	BIT(1)
#define LM3697_CTRL_A_B_EN	(LM3697_CTRL_A_EN | LM3697_CTRL_B_EN)

#define LM3697_MAX_LED_STRINGS	3

#define LM3697_CONTROL_A	0
#define LM3697_MAX_CONTROL_BANKS 2
#define LMU_11BIT_LSB_MASK	(BIT(0) | BIT(1) | BIT(2))
#define LMU_11BIT_MSB_SHIFT	3

#define MAX_BRIGHTNESS_8BIT	255
#define MAX_BRIGHTNESS_11BIT	2047

struct lm3697 {
	int enable_gpio;
	struct i2c_client *client;
	struct led_classdev led_dev;
	struct device dev;
	struct mutex lock;

	unsigned int brightness;
	unsigned int max_brightness;
	int  enabled;
};

struct lm3697 *ext_lm3697_data;

static int platform_read_i2c_block(struct i2c_client *client, char *writebuf,
	int writelen, char *readbuf, int readlen)
{
	int ret;
	unsigned char cnt = 0;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = 0,
				 .len = writelen,
				 .buf = writebuf,
			 },
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};

		while (cnt < 5) {
			ret = i2c_transfer(client->adapter, msgs, 2);
			if (ret < 0)
				dev_err(&client->dev, "%s: i2c read error\n",
								__func__);
			else
				break;

			cnt++;
			mdelay(2);
		}
	} else {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		while (cnt < 5) {
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0)
				dev_err(&client->dev, "%s:i2c read error.\n",
								__func__);
			else
				break;

			cnt++;
			mdelay(2);
		}
	}
	return ret;
}

static int lm3697_i2c_read(struct i2c_client *client, u8 addr, u8 *val)
{
	return platform_read_i2c_block(client, &addr, 1, val, 1);
}

static int platform_write_i2c_block(struct i2c_client *client,
		char *writebuf, int writelen)
{
	int ret;
	unsigned char cnt = 0;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	while (cnt < 5) {
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c write error.\n",
								__func__);
		else
			break;

		cnt++;
		mdelay(2);
	}

	return ret;
}

static int lm3697_i2c_write(struct i2c_client *client, u8 addr, const u8 val)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = val;

	return platform_write_i2c_block(client, buf, sizeof(buf));
}

static int lm3697_i2c_write_bit(struct i2c_client *client,
	unsigned int reg_addr, unsigned int  mask, unsigned char reg_data)
{
	unsigned char reg_val = 0;

	lm3697_i2c_read(client, reg_addr, &reg_val);
	reg_val &= mask;
	reg_val |= reg_data;
	lm3697_i2c_write(client, reg_addr, reg_val);

	return 0;
}

static void lm3697_brightness_set(struct led_classdev *led_cdev,
				enum led_brightness brt_val)
{
	struct lm3697 *led = container_of(led_cdev, struct lm3697,
					      led_dev);
	struct device *dev = &led->dev;

	mutex_lock(&led->lock);

	if (brt_val == LED_OFF) {
		lm3697_i2c_write(led->client, LM3697_CTRL_ENABLE, 0x0);
		led->enabled = LED_OFF;
	} else {
		lm3697_i2c_write(led->client,  LM3697_CTRL_A_BRT_MSB, brt_val);
		if (!led->enabled) {
			lm3697_i2c_write(led->client, LM3697_CTRL_ENABLE, 0x1);
			led->enabled = brt_val;
		}
	}

brightness_out:
	mutex_unlock(&led->lock);
}

int lm3697_set_brightness_level(unsigned int level)
{
	if (!ext_lm3697_data)
		return 0;
	pr_info("%s LM3697_level is %d\n", __func__, level);

	lm3697_i2c_write(ext_lm3697_data->client,
				LM3697_CTRL_A_BRT_LSB,
				0X00);
	lm3697_i2c_write(ext_lm3697_data->client,
				LM3697_CTRL_A_BRT_MSB,
				level  & 0xff);
}

EXPORT_SYMBOL_GPL(lm3697_set_brightness_level);

static unsigned char chipid;
static int lm3697_read_chipid(struct lm3697 *priv)
{
	int ret = -1;
	u8 value = 0;
	unsigned char cnt = 0;

	while (cnt < 5) {
		ret = lm3697_i2c_read(priv->client, LM3697_REV, &value);
		if (ret < 0) {
			pr_err("%s: failed to read reg LM3697_REG_ID: %d\n",
				__func__, ret);
		}
		switch (value) {
		case 0x01:
			pr_info("%s lm3697 detected\n", __func__);
			chipid = value;
			return 0;
		default:
			pr_info("%s unsupported device revision (0x%x)\n",
				__func__, value);
			break;
		}
		cnt++;

		msleep(2);
	}

	return -EINVAL;
}

int is_lm3697_chip_exist(void)
{
	if (chipid == 0x1)
		return 1;
	else
		return 0;
}

static int lm3697_gpio_init(struct lm3697 *priv)
{
	int ret = -1;

	if (gpio_is_valid(priv->enable_gpio)) {
		ret = gpio_request(priv->enable_gpio, "lm3697_gpio");
		if (ret < 0) {
			pr_err("failed to request gpio\n");
			return -1;
		}
		ret = gpio_direction_output(priv->enable_gpio, 0);
		pr_info(" request gpio init\n");
		if (ret < 0) {
			pr_err("failed to set output");
			gpio_free(priv->enable_gpio);
			return ret;
		}
		gpio_set_value(priv->enable_gpio, true);
	} else
		return -EINVAL;
}

static int lm3697_init(struct lm3697 *priv)
{
	int ret = -1;

	/* Change BLK to Linear mode. 0x00 = exponential, 0x01 = Linear */
	ret = lm3697_i2c_write(priv->client, LM3697_CTRL_A_B_BRT_CFG, 0x01);
	if (ret) {
		pr_err("Cannot write LM3697_CTRL_A_B_BRT_CFG\n");
		goto out;
	}

out:
	return ret;
}

static void lm3697_probe_dt(struct device *dev, struct lm3697 *priv)
{
	priv->enable_gpio = 22;
}

static int lm3697_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct lm3697 *led;
	int ret;
	pr_err("lm3697_probe!\n");

	led = kzalloc(sizeof(struct lm3697), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	led->client = client;
	led->brightness = LED_OFF;
	led->led_dev.name = "lm3697";
	led->led_dev.max_brightness = MAX_BRIGHTNESS_8BIT;
	led->led_dev.brightness_set = lm3697_brightness_set;
	mutex_init(&led->lock);
	lm3697_probe_dt(&client->dev, led);
	i2c_set_clientdata(client, led);
	lm3697_init(led);
	lm3697_gpio_init(led);

	ret = lm3697_read_chipid(led);
	if(ret < 0)
	{
		pr_err("Failed to read lm3697 chipid: %d\n", ret);
		goto err_init;
	}
	ret = led_classdev_register(&client->dev, &led->led_dev);
	if (ret < 0) {
		pr_err("%s : Register led class failed\n", __func__);
		goto err_init;
	}
	ext_lm3697_data = led;
	return 0;

err_init:
	kfree(led);
	return ret;
}

static int lm3697_remove(struct i2c_client *client)
{
	struct lm3697 *led = i2c_get_clientdata(client);

	int ret;

	ret = lm3697_i2c_write(client, LM3697_CTRL_ENABLE, 0);
	if (ret) {
		pr_err( "Failed to disable the device\n");
		return ret;
	}
	led_classdev_unregister(&led->led_dev);
	mutex_destroy(&led->lock);
	kfree(led);
	ext_lm3697_data = NULL;
	return 0;
}

static const struct i2c_device_id lm3697_id[] = {
	{ "lm3697", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lm3697_id);

static const struct of_device_id of_lm3697_leds_match[] = {
	{ .compatible = "awinic,aw99703-bl", },
	{},
};

static struct i2c_driver lm3697_driver = {
    .probe		= lm3697_probe,
	.remove		= lm3697_remove,
	.id_table	= lm3697_id,
	.driver = {
		.name = "lm3697",
        .owner = THIS_MODULE,
		.of_match_table = of_lm3697_leds_match,
	},
};
module_i2c_driver(lm3697_driver);

MODULE_DESCRIPTION("LM3697 LED driver");
MODULE_LICENSE("GPL v2");

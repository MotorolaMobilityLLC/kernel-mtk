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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/types.h>

#include <linux/platform_device.h>

#include <linux/leds.h>

#define SGM37604A_DEV_NAME "sgm37604a"

#define BACKLIGHT_TAG                  "[SGM37604A] "
#define BACKLIGHT_FUN(f)               pr_debug("[SGM37604A] %s\n", __func__)
#define BACKLIGHT_ERR(fmt, args...)    pr_debug("[SGM37604A] %s %d : "fmt, __func__, __LINE__, ##args)
#define BACKLIGHT_LOG(fmt, args...)    pr_debug(fmt, ##args)

#define SGM37604A_MIN_VALUE_SETTINGS 16		/* value min leds_brightness_set */
#define SGM37604A_MAX_VALUE_SETTINGS 4095	/* value max leds_brightness_set */
#define MIN_MAX_SCALE(x) (((x) < SGM37604A_MIN_VALUE_SETTINGS) ? SGM37604A_MIN_VALUE_SETTINGS :\
(((x) > SGM37604A_MAX_VALUE_SETTINGS) ? SGM37604A_MAX_VALUE_SETTINGS:(x)))

/* I2C variable */
static struct i2c_client *new_client;

/* static unsigned char current_brightness; */
static unsigned char is_suspend;

struct semaphore SGM37604A_lock;

/* SGM37604A Register Map */
#define MAX_BRIGHTNESS			(4095)
#define MID_BRIGHTNESS          (16)

#define SGM37604A_CTL_BRIGHTNESS_LSB_REG			0x1A
#define SGM37604A_CTL_BRIGHTNESS_MSB_REG			0x19
#define SGM37604A_CTL_BACKLIGHT_MODE_REG                       	0x11
#define SGM37604A_CTL_BACKLIGHT_LED_REG                        	0x10
#define SGM37604A_CTL_BACKLIGHT_CURRENT_REG                     0x1B

unsigned char backlight_mode_reg = 0x65;
unsigned char backlight_led_reg = 0x1F;
unsigned char backlight_current_reg = 0x00;

static char SGM37604A_i2c_write(struct i2c_client *client, u8 reg_addr, u8 *data, u8 len)
{
	s32 dummy;

	if (client == NULL)
		return -1;

	while (len-- != 0) {
		dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);

		reg_addr++;
		data++;
		if (dummy < 0)
			return -1;

	}

	return 0;
}

static int SGM37604A_smbus_write_byte(struct i2c_client *client,
				   unsigned char reg_addr, unsigned char *data)
{
	int ret_val = 0;
	int i = 0;

	ret_val = SGM37604A_i2c_write(client, reg_addr, data, 1);

	for (i = 0; i < 5; i++) {
		if (ret_val != 0)
			SGM37604A_i2c_write(client, reg_addr, data, 1);
		else
			return ret_val;
	}

	return ret_val;
}

unsigned int SGM37604A_set_brightness_level(unsigned int level_a)
{
	int ret_code = 0;
	unsigned char data0,data1;

	data0 = (level_a & 0x0F);
	data1 = level_a;

	/* Control A Brightness LSB & MSB */
	ret_code = SGM37604A_smbus_write_byte(new_client, SGM37604A_CTL_BRIGHTNESS_LSB_REG, &data0);
	if (ret_code != 0) {
		BACKLIGHT_ERR("[Cust_SetBacklight] CTL_LSB fail: %d\n", ret_code);
		return ret_code;
	}

	ret_code = SGM37604A_smbus_write_byte(new_client, SGM37604A_CTL_BRIGHTNESS_MSB_REG, &data1);
	if (ret_code != 0) {
		BACKLIGHT_ERR("[Cust_SetBacklight] CTL_MSB fail: %d\n", ret_code);
		return ret_code;
	}

	return 0;
}

static void SGM37604A_set_backlight_reg_init(void)
{
	int ret_code = 0;
	unsigned char data0, data1, data2;

	data0 =	backlight_mode_reg;
	data1 = backlight_led_reg;
	data2 = backlight_current_reg;

	ret_code = SGM37604A_smbus_write_byte(new_client, SGM37604A_CTL_BACKLIGHT_MODE_REG, &data0);
	if (ret_code != 0) {
		BACKLIGHT_ERR("[Cust_SetBacklight] CTL_MODE fail: %d\n", ret_code);
	}
	BACKLIGHT_LOG("[SGM37604A] set backlight mode reg = 0x%x \n", data0);

	ret_code = SGM37604A_smbus_write_byte(new_client, SGM37604A_CTL_BACKLIGHT_LED_REG, &data1);
	if (ret_code != 0) {
		BACKLIGHT_ERR("[Cust_SetBacklight] CTL_LED fail: %d\n", ret_code);
	}
	BACKLIGHT_LOG("[SGM37604A] set backlight led reg = 0x%x \n", data1);

	ret_code = SGM37604A_smbus_write_byte(new_client, SGM37604A_CTL_BACKLIGHT_CURRENT_REG, &data2);
	if (ret_code != 0) {
		BACKLIGHT_ERR("[Cust_SetBacklight] CTL_CURRENT fail: %d\n", ret_code);
	}
	BACKLIGHT_LOG("[SGM37604A] set backlight current reg = 0x%x \n", data2);
}

int sgm37604a_set_backlight_level(unsigned int level)
{
	unsigned int level_a = 0;
	unsigned int ret_code = 0;

	BACKLIGHT_LOG("[SGM37604A] sgm37604a_set_backlight_level  [%d]\n", level);

	if (level == 0) {
		BACKLIGHT_LOG("------backlight_level suspend-----\n");
		ret_code = down_interruptible(&SGM37604A_lock);

		level_a = 0;
		SGM37604A_set_brightness_level(level_a);
		up(&SGM37604A_lock);

		is_suspend = 1;
	} else {

		level_a = MIN_MAX_SCALE(level);

		if (is_suspend == 1) {
			BACKLIGHT_LOG("------backlight_level resume-----\n");
			is_suspend = 0;

			mdelay(10);
			ret_code = down_interruptible(&SGM37604A_lock);

			BACKLIGHT_LOG("[Brightness] level_a=%d\n", level_a);

			up(&SGM37604A_lock);
		}

		ret_code = down_interruptible(&SGM37604A_lock);

		ret_code = SGM37604A_set_brightness_level(level_a);
		if (ret_code != 0)
			BACKLIGHT_LOG("[Brightness] setting fail ----------\n");

		up(&SGM37604A_lock);

	}

	return 0;
}

static int SGM37604A_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	new_client = client;

	BACKLIGHT_FUN();

	sema_init(&SGM37604A_lock, 1);
	BACKLIGHT_LOG("SGM37604A_probe init. \n");
	/*SGM37604A register config init*/
	SGM37604A_set_backlight_reg_init();

	if (client == NULL)
		BACKLIGHT_LOG("%s client is NULL\n", __func__);
	else
		BACKLIGHT_LOG("%s %p %x %x\n", __func__, client->adapter, client->addr, client->flags);
	return 0;
}


static int SGM37604A_remove(struct i2c_client *client)
{
	new_client = NULL;
	return 0;
}


static int __attribute__ ((unused)) SGM37604A_detect(struct i2c_client *client, int kind,
						  struct i2c_board_info *info)
{
	return 0;
}

static struct of_device_id sgm37604a_of_match[] = {
	{.compatible = "mediatek,sgm37604a"},
	{},
};
MODULE_DEVICE_TABLE(of,sgm37604a_of_match);

static const struct i2c_device_id sgm37604a_i2c_id[] = {
	{ "sgm37604a", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm37604a_i2c_id);

static struct i2c_driver SGM37604A_i2c_driver = {
	.driver = {
 			.owner  = THIS_MODULE,
		   	.name = SGM37604A_DEV_NAME,
#ifdef CONFIG_OF
		   	.of_match_table = sgm37604a_of_match,
#endif
		   },

	.id_table = sgm37604a_i2c_id,

	.probe = SGM37604A_probe,
	.remove = SGM37604A_remove,
};

module_i2c_driver(SGM37604A_i2c_driver);

MODULE_AUTHOR("YeQiang Li <liyeqiang@longcheer.com>");
MODULE_DESCRIPTION("SGM37604A Driver");
MODULE_LICENSE("GPL");

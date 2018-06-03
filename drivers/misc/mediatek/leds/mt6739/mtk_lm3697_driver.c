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

#define LM3697_DEV_NAME "charge-pump"

#define CPD_TAG                  "[ChargePump] "
#define CPD_FUN(f)               pr_debug("[ChargePump] %s\n", __func__)
#define CPD_ERR(fmt, args...)    pr_debug("[ChargePump] %s %d : "fmt, __func__, __LINE__, ##args)
#define CPD_LOG(fmt, args...)    pr_debug(fmt, ##args)

#define LM3697_MIN_VALUE_SETTINGS 10	/* value leds_brightness_set */
#define LM3697_MAX_VALUE_SETTINGS 255	/* value leds_brightness_set */
#define MIN_MAX_SCALE(x) (((x) < LM3697_MIN_VALUE_SETTINGS) ? LM3697_MIN_VALUE_SETTINGS :\
(((x) > LM3697_MAX_VALUE_SETTINGS) ? LM3697_MAX_VALUE_SETTINGS:(x)))

/* I2C variable */
static struct i2c_client *new_client;
static const struct i2c_device_id lm3697_i2c_id[] = { {LM3697_DEV_NAME, 0}, {} };

/* Flash control */
unsigned char strobe_ctrl;
unsigned char flash_ctrl;
unsigned char flash_status;

/* static unsigned char current_brightness; */
static unsigned char is_suspend;

struct semaphore lm3697_lock;

/* generic */
#define LM3697_MAX_RETRY_I2C_XFER (100)
#define LM3697_I2C_WRITE_DELAY_TIME 1

/* LM3697 Register Map */
#define MAX_BRIGHTNESS			(255)
#define MID_BRIGHTNESS          (125)

#define BANK_NONE			0x00
#define BANK_A				0x01
#define BANK_B				0x02

#define LM3697_REVISION_REG				0x00
#define LM3697_SW_RESET_REG				0x01
#define LM3697_HVLED_CURR_SINK_OUT_CFG_REG		0x10
#define LM3697_CTL_A_RAMP_TIME_REG			0x11
#define LM3697_CTL_B_RAMP_TIME_REG			0x12
#define LM3697_CTL_RUNTIME_RAMP_TIME_REG		0x13
#define LM3697_CTL_RUNTIME_RAMP_CFG_REG			0x14
#define LM3697_BRIGHTNESS_CFG_REG			0x16
#define LM3697_CTL_A_FULL_SCALE_CURR_REG		0x17
#define LM3697_CTL_B_FULL_SCALE_CURR_REG		0x18
#define LM3697_HVLED_CURR_SINK_FEEDBACK_REG		0x19
#define LM3697_BOOST_CTL_REG				0x1A
#define LM3697_AUTO_FREQ_THRESHOLD_REG			0x1B
#define LM3697_PWM_CFG_REG				0x1C
#define LM3697_CTL_A_BRIGHTNESS_LSB_REG			0x20
#define LM3697_CTL_A_BRIGHTNESS_MSB_REG			0x21
#define LM3697_CTL_B_BRIGHTNESS_LSB_REG			0x22
#define LM3697_CTL_B_BRIGHTNESS_MSB_REG			0x23
#define LM3697_CTL_B_BANK_EN_REG			0x24

#if 0
/* i2c read routine for API*/
static char lm3697_i2c_read(struct i2c_client *client, u8 reg_addr, u8 *data, u8 len)
{

	s32 dummy;

	if (client == NULL)
		return -1;

	while (len-- != 0) {
		dummy = i2c_master_send(client, (char *)&reg_addr, 1);
		if (dummy < 0) {
			pr_debug("send dummy is %d", dummy);
			return -1;
		}

		dummy = i2c_master_recv(client, (char *)data, 1);
		if (dummy < 0) {
			pr_debug("recv dummy is %d", dummy);
			return -1;
		}
		reg_addr++;
		data++;
	}
	return 0;
}
#endif				/* 0 */

/* i2c write routine for */
static char lm3697_i2c_write(struct i2c_client *client, u8 reg_addr, u8 *data, u8 len)
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
	/* CPD_LOG("\n [LM3697] lm3697_i2c_write\n"); */
	return 0;
}

#if 0
static int lm3697_smbus_read_byte(struct i2c_client *client,
				  unsigned char reg_addr, unsigned char *data)
{
	return lm3697_i2c_read(client, reg_addr, data, 1);
}
#endif				/* 0 */

static int lm3697_smbus_write_byte(struct i2c_client *client,
				   unsigned char reg_addr, unsigned char *data)
{
	int ret_val = 0;
	int i = 0;

	ret_val = lm3697_i2c_write(client, reg_addr, data, 1);

	for (i = 0; i < 5; i++) {
		if (ret_val != 0)
			lm3697_i2c_write(client, reg_addr, data, 1);
		else
			return ret_val;
	}
	return ret_val;
}

#if 0
static int lm3697_smbus_read_byte_block(struct i2c_client *client,
					unsigned char reg_addr, unsigned char *data,
					unsigned char len)
{
	return lm3697_i2c_read(client, reg_addr, data, len);
}
#endif				/* 0 */

unsigned int lm3697_set_brightness_level(unsigned int level_a)
{
	int ret_code = 0;
	unsigned char data0 = 0;
	unsigned char data1 = 0;

	/* Control A Brightness LSB & MSB */
	data0 = 0;
	ret_code = lm3697_smbus_write_byte(new_client, LM3697_CTL_A_BRIGHTNESS_LSB_REG, &data0);
	if (ret_code != 0) {
		CPD_ERR("[Cust_SetBacklight] CTL_A_LSB fail: %d\n", ret_code);
		return ret_code;
	}

	data1 = level_a;
	ret_code = lm3697_smbus_write_byte(new_client, LM3697_CTL_A_BRIGHTNESS_MSB_REG, &data1);
	if (ret_code != 0) {
		CPD_ERR("[Cust_SetBacklight] CTL_A_MSB fail: %d\n", ret_code);
		return ret_code;
	}

	/* Control B Brightness LSB & MSB */
	data0 = 0;
	ret_code = lm3697_smbus_write_byte(new_client, LM3697_CTL_B_BRIGHTNESS_LSB_REG, &data0);
	if (ret_code != 0) {
		CPD_ERR("[Cust_SetBacklight] CTL_B_LSB fail: %d\n", ret_code);
		return ret_code;
	}

	data1 = level_a;
	ret_code = lm3697_smbus_write_byte(new_client, LM3697_CTL_B_BRIGHTNESS_MSB_REG, &data1);
	if (ret_code != 0) {
		CPD_ERR("[Cust_SetBacklight] CTL_B_MSB fail: %d\n", ret_code);
		return ret_code;
	}

	CPD_LOG("[Brightness] level_a=%d\n", level_a);

	return 0;
}

int chargepump_set_backlight_level(unsigned int level)
{
	unsigned int level_a = 0;
	unsigned int ret_code = 0;
	unsigned char data = 0;

	/* CPD_LOG("\n[LM3697] chargepump_set_backlight_level  [%d]\n", level); */

	if (level == 0) {
		CPD_LOG("------	 backlight_level suspend-----\n");
		ret_code = down_interruptible(&lm3697_lock);

		level_a = 0;
		lm3697_set_brightness_level(level_a);

		data = 0x0;
		lm3697_smbus_write_byte(new_client, LM3697_CTL_B_BANK_EN_REG, &data);

		up(&lm3697_lock);
		CPD_LOG("[LM3697] flash_status = %d\n", flash_status);

		is_suspend = 1;
	} else {

		level_a = MIN_MAX_SCALE(level);

		if (is_suspend == 1) {
			CPD_LOG("------        backlight_level resume-----\n");
			is_suspend = 0;

			mdelay(10);

			ret_code = down_interruptible(&lm3697_lock);

			data = 0x3;
			lm3697_smbus_write_byte(new_client, LM3697_CTL_B_BANK_EN_REG, &data);

			CPD_LOG("[Brightness] level_a=%d\n", level_a);

			up(&lm3697_lock);
		}

		ret_code = down_interruptible(&lm3697_lock);

		ret_code = lm3697_set_brightness_level(level_a);
		if (ret_code != 0)
			CPD_LOG("[Brightness] setting fail ----------\n");

		up(&lm3697_lock);

	}

	return 0;
}

static int lm3697_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	new_client = client;

	CPD_FUN();

	sema_init(&lm3697_lock, 1);

	if (client == NULL)
		CPD_LOG("%s client is NULL\n", __func__);
	else
		CPD_LOG("%s %p %x %x\n", __func__, client->adapter, client->addr, client->flags);

	return 0;
}


static int lm3697_remove(struct i2c_client *client)
{
	new_client = NULL;
	return 0;
}


static int __attribute__ ((unused)) lm3697_detect(struct i2c_client *client, int kind,
						  struct i2c_board_info *info)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id lm3697_of_match[] = {
	{.compatible = "mediatek,chargepump"},
	{},
};
#endif				/* CONFIG_OF */

static struct i2c_driver lm3697_i2c_driver = {
	.driver = {
/* .owner  = THIS_MODULE, */
		   .name = LM3697_DEV_NAME,
#ifdef CONFIG_OF
		   .of_match_table = lm3697_of_match,
#endif				/* CONFIG_OF */
		   },
	.probe = lm3697_probe,
	.remove = lm3697_remove,
	.id_table = lm3697_i2c_id,
};

static int __init lm3697_init(void)
{
	CPD_FUN();

#ifndef	CONFIG_MTK_LEDS
	register_early_suspend(&lm3697_early_suspend_desc);
#endif

	i2c_add_driver(&lm3697_i2c_driver);

	return 0;
}

static void __exit lm3697_exit(void)
{
	i2c_del_driver(&lm3697_i2c_driver);
}
EXPORT_SYMBOL(lm3697_flash_strobe_en);

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("lm3697 driver");
MODULE_LICENSE("GPL");

module_init(lm3697_init);
module_exit(lm3697_exit);

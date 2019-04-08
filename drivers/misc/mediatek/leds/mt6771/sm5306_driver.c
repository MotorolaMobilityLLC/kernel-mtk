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
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#define SM5306_DEV_NAME "SM5306"

#define BACKLIGHT_TAG                  "[SM5306] "
#define BACKLIGHT_FUN(f)               pr_debug("[SM5306] %s\n", __func__)
#define BACKLIGHT_ERR(fmt, args...)    pr_err("[SM5306] %s %d : "fmt, __func__, __LINE__, ##args)
#define BACKLIGHT_LOG(fmt, args...)    pr_debug(fmt, ##args)

struct bled_config_t {
	u8 addr;
	u8 val;
};

static struct bled_config_t sm5306_bled_settings[] = {
	{.addr = 0x01, .val = 0x19},
	{.addr = 0x02, .val = 0x78},
	{.addr = 0x05, .val = 0x1b},
	{.addr = 0x00, .val = 0x16},
	{.addr = 0x04, .val = 0x07},
	{.addr = 0x03, .val = 0xff},
};

struct sm5306_data {
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	unsigned short addr;
#ifdef CONFIG_FB
	struct notifier_block fb_notif;
#endif
};

static char SM5306_i2c_write(struct i2c_client *client, u8 reg_addr, u8 *data, u8 len)
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

static int SM5306_smbus_write_byte(struct i2c_client *client,
				   unsigned char reg_addr, unsigned char *data)
{
	int ret_val = 0;
	int i = 0;

	ret_val = SM5306_i2c_write(client, reg_addr, data, 1);

	for (i = 0; i < 5; i++) {
		if (ret_val != 0)
			SM5306_i2c_write(client, reg_addr, data, 1);
		else
			return ret_val;
	}

	return ret_val;
}

static int SM5306_set_backlight_reg_init(struct sm5306_data *drvdata)
{
	int ret, i;

	for (i = 0; i < sizeof(sm5306_bled_settings) / sizeof(struct bled_config_t); i++) {
		BACKLIGHT_LOG("set addr 0x%02X = 0x%02x\n", sm5306_bled_settings[i].addr, sm5306_bled_settings[i].val);
		ret = SM5306_smbus_write_byte(drvdata->client, sm5306_bled_settings[i].addr, &sm5306_bled_settings[i].val);
		if (ret)
			return ret;
	}
	return ret;
}

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				       unsigned long event, void *data)
{
	int *blank;
	struct fb_event *evdata = data;
	struct sm5306_data *drvdata =
		container_of(self, struct sm5306_data, fb_notif);

	/*
	 *  FB_EVENT_BLANK(0x09): A hardware display blank change occurred.
	 *  FB_EARLY_EVENT_BLANK(0x10): A hardware display blank early change
	 * occurred.
	 */
	if (evdata && evdata->data && (event == FB_EVENT_BLANK)) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK || *blank == FB_BLANK_NORMAL)
			SM5306_set_backlight_reg_init(drvdata);
	}

	return NOTIFY_OK;
}
#endif

static int SM5306_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sm5306_data *drvdata;
	int err = 0;

	drvdata = kzalloc(sizeof(struct sm5306_data), GFP_KERNEL);
	if (drvdata == NULL) {
		pr_err("%s : kzalloc failed\n", __func__);
		err = -ENOMEM;
		goto err_out;
	}

	drvdata->client = client;
	drvdata->adapter = client->adapter;
	client->addr = 0x36;
	drvdata->addr = client->addr;
	i2c_set_clientdata(client, drvdata);
	/*SM5306 register config init*/
	err = SM5306_set_backlight_reg_init(drvdata);
	if (err) {
		pr_err("%s : Init this IC failed: %d\n", __func__, err);
		goto err_out;
	}

#if defined(CONFIG_FB)
	drvdata->fb_notif.notifier_call = fb_notifier_callback;
	err = fb_register_client(&drvdata->fb_notif);
	if (err)
		pr_err("%s : Unable to register fb_notifier: %d\n", __func__, err);
#endif
	return 0;
err_out:
	return err;
}

static int SM5306_remove(struct i2c_client *client)
{
	return 0;
}

static struct of_device_id SM5306_of_match[] = {
	{.compatible = "mediatek,sm5306"},
	{},
};
MODULE_DEVICE_TABLE(of,SM5306_of_match);

static const struct i2c_device_id SM5306_i2c_id[] = {
	{ "sm5306", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, SM5306_i2c_id);

static struct i2c_driver SM5306_i2c_driver = {
	.driver = {
 			.owner  = THIS_MODULE,
		   	.name = SM5306_DEV_NAME,
#ifdef CONFIG_OF
		   	.of_match_table = SM5306_of_match,
#endif
		   },

	.id_table = SM5306_i2c_id,

	.probe = SM5306_probe,
	.remove = SM5306_remove,
};

module_i2c_driver(SM5306_i2c_driver);

MODULE_AUTHOR("Motorola>");
MODULE_DESCRIPTION("SM5306 Driver");
MODULE_LICENSE("GPL");

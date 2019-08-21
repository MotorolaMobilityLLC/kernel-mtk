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

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/i2c/ocp2131.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#define LOG_TAG "LCM_BIAS"
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)


/*****************************************************************************
 * Define
 *****************************************************************************/

#define I2C_ID_NAME "OCP2131"
static unsigned int GPIO_LCD_BIAS_ENN;
static unsigned int GPIO_LCD_BIAS_ENP;

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/

static struct i2c_client *OCP2131_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int OCP2131_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int OCP2131_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct OCP2131_dev {
	struct i2c_client *client;

};

static const struct i2c_device_id OCP2131_id[] = {
	{I2C_ID_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, OCP2131_id);

static struct of_device_id OCP2131_dt_match[] = {
	{ .compatible = "mediatek,i2c_lcd_bias" },
	{ },
};

static struct i2c_driver OCP2131_iic_driver = {
	.id_table = OCP2131_id,
	.probe = OCP2131_probe,
	.remove = OCP2131_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		.owner = THIS_MODULE,
		.name = "OCP2131",
		.of_match_table = of_match_ptr(OCP2131_dt_match),
	},
};

/*****************************************************************************
 * Function
 *****************************************************************************/
static void bias_get_dts_info(void)
{
	static struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,i2c_lcd_bias");
	GPIO_LCD_BIAS_ENN = of_get_named_gpio(node, "bias_gpio_enn", 0);
	GPIO_LCD_BIAS_ENP = of_get_named_gpio(node, "bias_gpio_enp", 0);
	gpio_request(GPIO_LCD_BIAS_ENN, "bias_gpio_enn");
	gpio_request(GPIO_LCD_BIAS_ENP, "bias_gpio_enp");
}

static void set_gpio_output(unsigned int GPIO, unsigned int output)
{
	gpio_direction_output(GPIO, output);
	gpio_set_value(GPIO, output);
}

void OCP2131_GPIO_ENP_enable(void)
{
	set_gpio_output(GPIO_LCD_BIAS_ENP, 1);
}

void OCP2131_GPIO_ENN_enable(void)
{
	set_gpio_output(GPIO_LCD_BIAS_ENN, 1);
}

void OCP2131_GPIO_ENP_disable(void)
{
	set_gpio_output(GPIO_LCD_BIAS_ENP, 0);
}

void OCP2131_GPIO_ENN_disable(void)
{
	set_gpio_output(GPIO_LCD_BIAS_ENN, 0);
}

static int OCP2131_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	LCM_LOGI("OCP2131_iic_probe\n");
	LCM_LOGI("TPS: info==>name=%s addr=0x%x\n", client->name, client->addr);
	OCP2131_i2c_client = client;
	bias_get_dts_info();
	return 0;
}

static int OCP2131_remove(struct i2c_client *client)
{
	LCM_LOGI("OCP2131_remove\n");
	OCP2131_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}


 int OCP2131_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = OCP2131_i2c_client;
	char write_data[2] = { 0 };
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		LCM_LOGI("OCP2131 write data fail !!\n");
	return ret;
}

static int __init OCP2131_iic_init(void)
{
	int ret = 0;

	LCM_LOGI("OCP2131_iic_init\n");
	ret = i2c_add_driver(&OCP2131_iic_driver);
	if (ret < 0)
	{
		LCM_LOGI("OCP2131 i2c driver add fail !!\n");
		return ret ;
	}

	LCM_LOGI("OCP2131_iic_init success\n");
	return 0;
}

static void __exit OCP2131_iic_exit(void)
{
	LCM_LOGI("OCP2131_iic_exit\n");
	i2c_del_driver(&OCP2131_iic_driver);
}

module_init(OCP2131_iic_init);
module_exit(OCP2131_iic_exit);
MODULE_DESCRIPTION("MTK OCP2131 I2C Driver");
MODULE_AUTHOR("TS");
MODULE_LICENSE("GPL");

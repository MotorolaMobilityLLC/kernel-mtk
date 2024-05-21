/*
 * Copyright (C) 2021 Siliconmitus Inc.
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
#include "lcd_bias.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#define LCD_BIAS_POSCNTL_REG	0x00
#define LCD_BIAS_NEGCNTL_REG	0x01
#define LCD_BIAS_CONTROL_REG	0x03

enum {
	FIRST_VP_AFTER_VN = 0,
	FIRST_VN_AFTER_VP,
};

enum {
	LCD_BIAS_GPIO_STATE_ENP0 = 0,
	LCD_BIAS_GPIO_STATE_ENP1,
	LCD_BIAS_GPIO_STATE_ENN0,
	LCD_BIAS_GPIO_STATE_ENN1,
	LCD_BIAS_GPIO_STATE_MAX,
};

static struct pinctrl *pinctl_bias_enp;
static struct pinctrl *pinctl_bias_enn;
static struct pinctrl *pinctl_tp_reset;
struct pinctrl_state *enn_output0, *enn_output1, *enp_output0, *enp_output1, *tp_reset0, *tp_reset1;

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_client *lcd_bias_i2c_client;
unsigned int enn_gpio = 0;
unsigned int enp_gpio = 0;
unsigned int en_reset_gpio = 0;
/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int lcd_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lcd_bias_i2c_remove(struct i2c_client *client);
static void lcd_bias_gpio_select_state(int lcd_bias_gpio_state);

/*****************************************************************************
 * Extern Area
 *****************************************************************************/
static void lcd_bias_write_byte(unsigned char addr, unsigned char value)
{
	int ret = 0;
	unsigned char write_data[2] = {0};

	write_data[0] = addr;
	write_data[1] = value;

	if (NULL == lcd_bias_i2c_client) {
		printk("[LCD][BIAS] lcd_bias_i2c_client is null!!\n");
		return ;
	}
	ret = i2c_master_send(lcd_bias_i2c_client, write_data, 2);
	if (ret < 0)
		printk("[LCD][BIAS] i2c write data fail!! ret=%d\n", ret);
}

static void lcd_bias_set_vpn(unsigned int en, unsigned int seq, unsigned int value)
{
	unsigned char level;

	level = (value - 4000) / 100;  //eg.  5.0V= 4.0V + Hex 0x0A (Bin 0 1010) * 100mV
	if (seq == FIRST_VP_AFTER_VN) {
		if (en) {
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP1);
			mdelay(1);
			lcd_bias_write_byte(LCD_BIAS_POSCNTL_REG, level);
			mdelay(5);
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN1);
			lcd_bias_write_byte(LCD_BIAS_NEGCNTL_REG, level);
		} else {
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP0);
			mdelay(5);
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN0);
		}
	} else if (seq == FIRST_VN_AFTER_VP) {
		if (en) {
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN1);
			mdelay(1);
			lcd_bias_write_byte(LCD_BIAS_NEGCNTL_REG, level);
			mdelay(5);
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP1);
			lcd_bias_write_byte(LCD_BIAS_POSCNTL_REG, level);
		} else {
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN0);
			mdelay(5);
			lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP0);
		}
	}
}

void lcd_bias_set_vspn(unsigned int en, unsigned int seq, unsigned int value)
{
	if ((value <= 4000) || (value > 6500)) {
		printk("[LCD][BIAS] unreasonable voltage value:%d\n", value);
		return ;
	}

	lcd_bias_set_vpn(en, seq, value);
}

void lcd_bias_set_vsp(unsigned int en)
{
	if (en) {
		lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP1);
	} else {
		lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENP0);
	}
}

void lcd_bias_set_vsn(unsigned int en)
{
	if (en) {
		lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN1);
	} else {
		lcd_bias_gpio_select_state(LCD_BIAS_GPIO_STATE_ENN0);
	}
}

void tp_reset_en(unsigned int en)
{
	if (en) {
		pinctrl_select_state(pinctl_tp_reset, tp_reset1);
	} else {
		pinctrl_select_state(pinctl_tp_reset, tp_reset0);
	}
}

/*****************************************************************************
 * Data Structure
 *****************************************************************************/
static const struct of_device_id i2c_of_match[] = {
	{ .compatible = "mtk,i2c_lcd_bias", },
	{},
};

static const struct i2c_device_id lcd_bias_i2c_id[] = {
	{"lcd_Bias_I2C", 0},
	{},
};

static struct i2c_driver lcd_bias_i2c_driver = {
	.id_table = lcd_bias_i2c_id,
	.probe = lcd_bias_i2c_probe,
	.remove = lcd_bias_i2c_remove,
	.driver = {
		.name = "lcd_bias_i2c",
		.of_match_table = i2c_of_match,
	},
};

/*****************************************************************************
 * Function
 *****************************************************************************/
static long lcd_bias_set_state(unsigned int lcd_bias_gpio_state)
{
	int ret = 0;

	switch (lcd_bias_gpio_state) {
	case LCD_BIAS_GPIO_STATE_ENP0:
		ret = pinctrl_select_state(pinctl_bias_enp, enp_output0);
		if (ret < 0) {
			pr_info("%s pinctrl_select_state failed for enp low\n",	__func__);
			return ret;
		}
		break;
	case LCD_BIAS_GPIO_STATE_ENP1:
		ret = pinctrl_select_state(pinctl_bias_enp, enp_output1);
		if (ret < 0) {
			pr_info("%s pinctrl_select_state failed for enp high\n", __func__);
			return ret;
		}
		break;
	case LCD_BIAS_GPIO_STATE_ENN0:
		ret = pinctrl_select_state(pinctl_bias_enn, enn_output0);
		if (ret < 0) {
			pr_info("%s pinctrl_select_state failed for enn low\n", __func__);
			return ret;
		}
		break;
	case LCD_BIAS_GPIO_STATE_ENN1:
		ret = pinctrl_select_state(pinctl_bias_enn, enn_output1);
		if (ret < 0) {
			pr_info("%s pinctrl_select_state failed for enn high\n", __func__);
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

void lcd_bias_gpio_select_state(int lcd_bias_gpio_state)
{
	BUG_ON(!((unsigned int)(lcd_bias_gpio_state) < (unsigned int)(LCD_BIAS_GPIO_STATE_MAX)));
	lcd_bias_set_state(lcd_bias_gpio_state);
}

static void lcd_bias_pinctrl_init(struct device *dev)
{
	pinctl_bias_enp = devm_pinctrl_get(dev);
	if (IS_ERR(pinctl_bias_enp)) {
		pr_info("pinctl_bias_enp = %d\n",PTR_ERR(pinctl_bias_enp));
	} else {
		enp_output0 = pinctrl_lookup_state(pinctl_bias_enp, "lcd_enp_output_low");
		if (IS_ERR(enp_output0)) {
		pr_err("lookup state enp_output0 failed\n");
		}
		enp_output1 = pinctrl_lookup_state(pinctl_bias_enp, "lcd_enp_output_high");
		if (IS_ERR(enp_output1)) {
			pr_err("lookup state enp_output1 failed\n");
		}
	}

	pinctl_bias_enn = devm_pinctrl_get(dev);
	if (IS_ERR(pinctl_bias_enn)) {
		pr_info("pinctl_bias_enn = %d\n",PTR_ERR(pinctl_bias_enn));
	} else {
		enn_output0 = pinctrl_lookup_state(pinctl_bias_enn, "lcd_enn_output_low");
		if (IS_ERR(enn_output0)) {
			pr_err("lookup state enn_output0 failed\n");
		}
		enn_output1 = pinctrl_lookup_state(pinctl_bias_enn, "lcd_enn_output_high");
		if (IS_ERR(enn_output1)) {
			pr_err("lookup state enn_output1 failed\n");
		}
	}

	pinctl_tp_reset = devm_pinctrl_get(dev);
	if (IS_ERR(pinctl_tp_reset)) {
		pr_info("pinctl_tp_reset = %d\n",PTR_ERR(pinctl_tp_reset));
	} else {
		tp_reset0 = pinctrl_lookup_state(pinctl_tp_reset, "tp_reset_output_low");
		if (IS_ERR(tp_reset0)) {
			pr_err("lookup state tp_reset0 failed\n");
		}
		tp_reset1 = pinctrl_lookup_state(pinctl_tp_reset, "tp_reset_output_high");
		if (IS_ERR(tp_reset1)) {
			pr_err("lookup state tp_reset1 failed\n");
		}
	}
}

static int lcd_bias_parse_dt(struct device *dev)
{
	enp_gpio = of_get_named_gpio(dev->of_node, "mtk,enp", 0);
	if (gpio_is_valid(enp_gpio)) {
		dev_info(dev, "%s: enp gpio provided ok.\n", __func__);
	} else {
		dev_err(dev, "%s: no enp gpio provided.\n", __func__);
		return -1;
	}
	enn_gpio = of_get_named_gpio(dev->of_node, "mtk,enn", 0);
	if (gpio_is_valid(enn_gpio)) {
		dev_info(dev, "%s: enn gpio provided ok.\n", __func__);
	} else {
		dev_err(dev, "%s: no enn gpio provided.\n", __func__);
		return -1;
	}

	en_reset_gpio = of_get_named_gpio(dev->of_node, "mtk,tp_reset", 0);
	if (gpio_is_valid(en_reset_gpio)) {
		dev_info(dev, "%s: lcm_reset gpio provided ok.\n", __func__);
	} else {
		dev_err(dev, "%s: no lcm_reset gpio provided.\n", __func__);
		return -1;
	}

	printk("[LCD][BIAS] enp_gpio = %d,enn_gpio = %d\n", enp_gpio, enn_gpio);
	printk("[LCD][BIAS] en_reset_gpio = %d\n", en_reset_gpio);
	return 0;
}

static int lcd_bias_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *dev;

	if (NULL == client) {
		printk("[LCD][BIAS] i2c_client is NULL\n");
		return -1;
	}

	dev = &client->dev;
	lcd_bias_i2c_client = client;
	printk("[LCD][BIAS] lcd_bias_i2c_probe success addr = 0x%x\n", client->addr);

	lcd_bias_pinctrl_init(dev);
	ret = lcd_bias_parse_dt(dev);
	if (ret < 0) {
		printk("[LCD][BIAS] lcd bias parse error\n");
		return -1;
	}

	return 0;
}

static int lcd_bias_i2c_remove(struct i2c_client *client)
{
	lcd_bias_i2c_client = NULL;
	i2c_unregister_device(client);

	return 0;
}

static int __init lcd_bias_init(void)
{
	if (i2c_add_driver(&lcd_bias_i2c_driver)) {
		printk("[LCD][BIAS] Failed to register lcd_bias_i2c_driver!\n");
		return -1;
	}

	printk("[LCD][BIAS] lcm bias init\n");
	return 0;
}

static void __exit lcd_bias_exit(void)
{
	i2c_del_driver(&lcd_bias_i2c_driver);
}

module_init(lcd_bias_init);
module_exit(lcd_bias_exit);

MODULE_DESCRIPTION("MTK LCD BIAS I2C Driver");
MODULE_LICENSE("GPL");

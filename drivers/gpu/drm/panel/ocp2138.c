/*
 * Copyright (C) 2021 Wingtech.Inc.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "ocp2138.h"

#define LOG_TAG "LCM_BIAS_OCP2138"
#define LCM_LOGI(fmt, args...)  pr_info("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGE(fmt, args...)  pr_debug("[KERNEL ERROR/"LOG_TAG"]"fmt, ##args)

#define ocp2138_DEV_NAME "ocp2138"

#define ocp2138_VPOS_ADDRESS		0x00
#define ocp2138_VNEG_ADDRESS		0x01

static unsigned int GPIO_LCD_BIAS_ENN;
static unsigned int GPIO_LCD_BIAS_ENP;
int ocp2138_BiasPower_enable(LCM_LDO_VOLTAGE_E avdd, LCM_LDO_VOLTAGE_E avee,u32 pwrup_delay);
int ocp2138_BiasPower_disable(u32 pwrdown_delay);

static const struct i2c_device_id ocp2138_i2c_id[] = {{ocp2138_DEV_NAME,0},{}};
static struct i2c_client *i2c_client = NULL;
static const struct of_device_id panel_i2c_of_match[] = {
	 {
	  .compatible = "ocp,bias_ocp2138",
	  },
 };

static int ocp2138_read_chip_info(struct i2c_client *client)
{
	int ret;
	u8 id_data[4] = {0};

	ret = i2c_smbus_read_i2c_block_data(client, 0x03, 1, &id_data[0]);
	if(ret < 0)
	{
		LCM_LOGE("ocp2138_read_chip_info() I2C Error\n");
		return -1;
	}

	LCM_LOGD("ocp2138_read_chip_info() id_data = 0x%x.\n", id_data[0]);
	return 0;
}

int ocp2138_set_voltage(LCM_LDO_VOLTAGE_E avdd, LCM_LDO_VOLTAGE_E avee,u32 pwrup_delay)
{
	u8 reg_value_avdd =0x00;
	u8 reg_value_avee =0x00;

	reg_value_avdd = avdd;
	reg_value_avee = avee;

	if(i2c_client == NULL)
	{
		LCM_LOGE("ocp2138_set_voltage i2c error\n");
		return -1;
	}

	if( reg_value_avdd == LCM_LDO_VOL_DEFAULT &&  reg_value_avee == LCM_LDO_VOL_DEFAULT)
	{
		LCM_LOGD("LCD bias IC have not conmfig and use default voltage\n");
		if ( gpio_is_valid(GPIO_LCD_BIAS_ENP) )
			gpio_direction_output(GPIO_LCD_BIAS_ENP, 1);

		if(pwrup_delay)
			mdelay(pwrup_delay);
		else
			mdelay(8);
		if ( gpio_is_valid(GPIO_LCD_BIAS_ENN) )
			gpio_direction_output(GPIO_LCD_BIAS_ENN, 1);
		return 0;
	}

	if( reg_value_avdd >= LCM_LDO_VOL_MAX || reg_value_avee >= LCM_LDO_VOL_MAX)
	{
		LCM_LOGE("LCD bias IC unsupport voltage avdd = %d, avee = %d\n", reg_value_avdd, reg_value_avee);
		return -1;
	}

	if ( gpio_is_valid(GPIO_LCD_BIAS_ENP) )
		gpio_direction_output(GPIO_LCD_BIAS_ENP, 1);
	if(0 != i2c_smbus_write_i2c_block_data(i2c_client, ocp2138_VPOS_ADDRESS, 1, &reg_value_avdd))
	{
		LCM_LOGE("LCD bias IC avdd write fail\n");
		if ( gpio_is_valid(GPIO_LCD_BIAS_ENN) )
			gpio_direction_output(GPIO_LCD_BIAS_ENN, 1);
		return -1;
	}

	if(pwrup_delay)
		mdelay(pwrup_delay);
	else
		mdelay(8);
	if ( gpio_is_valid(GPIO_LCD_BIAS_ENN) )
		gpio_direction_output(GPIO_LCD_BIAS_ENN, 1);

	if(0 != i2c_smbus_write_i2c_block_data(i2c_client, ocp2138_VNEG_ADDRESS, 1, &reg_value_avee))
	{
		LCM_LOGE("LCD bias IC avee write fail\n");
		return -1;
	}
	if(pwrup_delay)
		mdelay(pwrup_delay);
	else
		mdelay(8);

	return 0;
}

int ocp2138_BiasPower_enable(LCM_LDO_VOLTAGE_E avdd, LCM_LDO_VOLTAGE_E avee,u32 pwrup_delay)
{
	int rc = 0;

	rc = ocp2138_set_voltage(avdd,avee,pwrup_delay);
	if( rc < 0 )
		LCM_LOGE("ocp2138 Bias Power enable fail");

        return rc;
}
EXPORT_SYMBOL(ocp2138_BiasPower_enable);

int ocp2138_BiasPower_disable(u32 pwrdown_delay)
{
	if ( gpio_is_valid(GPIO_LCD_BIAS_ENN) )
		gpio_direction_output(GPIO_LCD_BIAS_ENN, 0);
	if(pwrdown_delay)
		mdelay(pwrdown_delay);
	else
		mdelay(8);
	if ( gpio_is_valid(GPIO_LCD_BIAS_ENP) )
		gpio_direction_output(GPIO_LCD_BIAS_ENP, 0);
	if(pwrdown_delay)
		mdelay(pwrdown_delay);
	else
		mdelay(8);

        return 0;
}
EXPORT_SYMBOL(ocp2138_BiasPower_disable);

static int ocp2138_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;

	/* enn, enp gpio info */
	GPIO_LCD_BIAS_ENN = of_get_named_gpio(np, "ocp,lcd-bias-enn-gpio", 0);
	if (!gpio_is_valid(GPIO_LCD_BIAS_ENN))
	{
		LCM_LOGD("ocp2138_parse_dt() ennnnn gpio not specified\n");
	}

	GPIO_LCD_BIAS_ENP = of_get_named_gpio(np, "ocp,lcd-bias-enp-gpio", 0);
	if (!gpio_is_valid(GPIO_LCD_BIAS_ENP))
	{
		LCM_LOGD("ocp2138_parse_dt() enpppp gpio not specified\n");
	}

	return 0;
}

static int ocp2138_gpio_configure(void)
{
	int ret = 0;

	/* request enn gpio */
	if (gpio_is_valid(GPIO_LCD_BIAS_ENN))
	{
		ret = gpio_request(GPIO_LCD_BIAS_ENN, "ocp2138_enn_gpio");
		if (ret)
		{
			LCM_LOGE("[GPIO] GPIO_LCD_BIAS_ENN gpio request failed");
			goto err_enn_gpio_req;
		}
	}

	/* request enp gpio */
	if (gpio_is_valid(GPIO_LCD_BIAS_ENP))
	{
		ret = gpio_request(GPIO_LCD_BIAS_ENP, "ocp2138_enp_gpio");
		if (ret)
		{
			LCM_LOGE("[GPIO]GPIO_LCD_BIAS_ENP gpio request failed");
			goto err_enp_gpio_dir;
		}
	}

	return 0;

err_enp_gpio_dir:
	if (gpio_is_valid(GPIO_LCD_BIAS_ENP))
		gpio_free(GPIO_LCD_BIAS_ENP);

err_enn_gpio_req:
	return ret;
}

static int ocp2138_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int ocp2138_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc = 0;

	rc = ocp2138_parse_dt(&client->dev);
	if (rc)
	{
	  LCM_LOGE("ocp2138_i2c_probe parse dt error.\n");
	  return -ENODEV;
	}

	rc = ocp2138_gpio_configure();
	if (rc)
	{
	  LCM_LOGE("ocp2138_i2c_probe() configure gpio error.\n");
	}

	gpio_direction_output(GPIO_LCD_BIAS_ENP, 1);
	gpio_direction_output(GPIO_LCD_BIAS_ENN, 1);

	if(ocp2138_read_chip_info(client) != 0)
	{
		LCM_LOGE("ocp2138_i2c_probe() read chip info error.\n");

		return -ENODEV;
	}
	i2c_client = client;
	LCM_LOGD("ocp2138_i2c_probe() OKAY.\n");
	//printk("[%d  %s]hxl_check_ocp  enter  rc:%d !!  \n", __LINE__, __FUNCTION__,rc);
	return 0;
}

struct i2c_driver ocp2138_i2c_driver = {
	.driver = {
		.name = ocp2138_DEV_NAME,
		.owner = THIS_MODULE,
        	.of_match_table = panel_i2c_of_match,
	},
	.probe = ocp2138_i2c_probe,
	.remove = ocp2138_i2c_remove,
	.id_table = ocp2138_i2c_id,
};


/* called when loaded into kernel */
static int __init ocp2138_driver_init(void)
{
	if(i2c_add_driver(&ocp2138_i2c_driver) != 0)
	{
		printk("ocp2138_driver_init() Fail\n");
		return -1;
	}
	return 0;
}

/* should never be called */
static void __exit ocp2138_driver_exit(void)
{
	i2c_del_driver(&ocp2138_i2c_driver);
}

module_init(ocp2138_driver_init);
module_exit(ocp2138_driver_exit);

MODULE_AUTHOR("WingTech Driver Team");
MODULE_DESCRIPTION("Panel Power Supply Driver");
MODULE_LICENSE("GPL v2");


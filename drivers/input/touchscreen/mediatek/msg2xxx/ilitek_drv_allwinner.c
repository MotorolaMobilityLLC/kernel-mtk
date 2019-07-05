/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */
 
/**
 *
 * @file    ilitek_drv_allwinner.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */
 
/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/kobject.h>
#include <asm/irq.h>
#include <asm/io.h>
#include "ilitek_drv_common.h"

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/

#define MSG_TP_IC_NAME "msg2xxx" //"msg22xx" or "msg28xx" /* Please define the mstar touch ic name based on the mutual-capacitive ic or self capacitive ic that you are using */

/*=============================================================*/
// VARIABLE DEFINITION
/*=============================================================*/

struct i2c_client *g_I2cClient = NULL;

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
struct regulator *g_ReguVdd = NULL;
struct regulator *g_ReguVcc_i2c = NULL;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

static const unsigned short normal_i2c[2] = {0x26,I2C_CLIENT_END};

struct ctp_config_info config_info = {	
	.input_type = CTP_TYPE,	
	.name = NULL,	
	.int_number = 0,
};

static int twi_id = 0;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;

static int ctp_get_system_config(void)
{   

        twi_id = config_info.twi_id;
	    screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if( (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
       		 }
        return 1;
}

int ctp_ts_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if(twi_id == adapter->nr)
	{
		strlcpy(info->type, MSG_TP_IC_NAME, I2C_NAME_SIZE);
		return 0;
	}
	else
	{
		return -ENODEV;
	}
}

static struct i2c_board_info i2c_info_dev =  {
	I2C_BOARD_INFO("msg2xxx", 0x26),
	.platform_data	= NULL,
};

static int add_ctp_device(void) {
	
	struct i2c_adapter *adap;
	
	adap = i2c_get_adapter(twi_id);
	g_I2cClient = i2c_new_device(adap, &i2c_info_dev);

	return 0;
}

/*=============================================================*/
// FUNCTION DEFINITION
/*=============================================================*/

/* probe function is used for matching and initializing input device */
static int /*__devinit*/ touch_driver_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    int ret = 0;
    const char *vdd_name = "vdd";
    const char *vcc_i2c_name = "vcc_i2c";
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

    printk("*** %s ***\n", __func__);
    
    if (client == NULL)
    {
        printk("i2c client is NULL\n");
        return -1;
    }
    g_I2cClient = client;  

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
    g_ReguVdd = regulator_get(&g_I2cClient->dev, vdd_name);
	if (IS_ERR(g_ReguVdd)) {
		printk("regulator_get vdd fail\n");
		//return -EINVAL;
	}
	else {
	    ret = regulator_set_voltage(g_ReguVdd, 2600000, 3300000); 
	    if (ret)
	    {
	        printk("Could not set to 2800mv.\n");
	    }
	}
    g_ReguVcc_i2c = regulator_get(&g_I2cClient->dev, vcc_i2c_name);
	if (IS_ERR(g_ReguVcc_i2c)) {
		printk("regulator_get vcc fail\n");
		//return -EINVAL;
	}
	else {
	    ret = regulator_set_voltage(g_ReguVcc_i2c, 1800000, 1800000);  
	    if (ret)
	    {
	        printk("Could not set to 1800mv.\n");
	    }
	}
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
                printk("Ilitek: %s, I2C_FUNC_I2C not support\n", __func__);
                return -1;
        }

    return MsDrvInterfaceTouchDeviceProbe(g_I2cClient, id);
}

/* remove function is triggered when the input device is removed from input sub-system */
static int touch_driver_remove(struct i2c_client *client)
{
    printk("*** %s ***\n", __func__);

    return MsDrvInterfaceTouchDeviceRemove(client);
}

int touch_driver_suspend(struct i2c_client *client, pm_message_t mesg)
{
	printk("ilitek: *** %s ***\n", __func__);

	MsDrvSuspend();
	 
	return 0;
}

int touch_driver_resume(struct i2c_client *client)
{
	printk("ilitek: *** %s ***\n", __func__);

	MsDrvResume();
	
	return 0;
}
/*
static const struct dev_pm_ops ilitek_pm_ops =
{
	.suspend  = touch_driver_suspend,
	.resume   = touch_driver_resume,
};
*/


/* The I2C device list is used for matching I2C device and I2C device driver. */
static const struct i2c_device_id touch_device_id[] =
{
    {MSG_TP_IC_NAME, 0},
    {}, /* should not omitted */ 
};

MODULE_DEVICE_TABLE(i2c, touch_device_id);

static struct of_device_id touch_match_table[] = {
    { .compatible = "mstar,msg2xxx",},
	//{ .compatible = "tchip,ilitek",},
	//{ .compatible = "ilitek,2120",},
    {},
};

static struct i2c_driver touch_device_driver =
{
    .class  = I2C_CLASS_HWMON,
    .driver = {
        .name = MSG_TP_IC_NAME,
        .owner = THIS_MODULE,
        .of_match_table = touch_match_table,
	    //.pm = &ilitek_pm_ops,
    },
    .probe = touch_driver_probe,
    .remove = touch_driver_remove,
    .id_table = touch_device_id,
    
#if !(defined CONFIG_HAS_EARLYSUSPEND) && !(defined CONFIG_ENABLE_NOTIFIER_FB)
    .suspend = touch_driver_suspend,
    .resume = touch_driver_resume,
#endif
    .address_list = normal_i2c,
    
};

static int __init touch_driver_init(void)
{
    s32 ret = -1;
	
	if (input_fetch_sysconfig_para(&(config_info.input_type))) {
			printk("Ilitek: %s: ctp_fetch_sysconfig_para err.\n", __func__);			
			return 0;
		} else {
			ret = input_init_platform_resource(&(config_info.input_type));
			if (0 != ret) {
				printk("Ilitek: %s:ctp_ops.init_platform_resource err. \n", __func__);
			}
	}	

	if(config_info.ctp_used == 0) {       
		printk("Ilitek: *** ctp_used set to 0 !\n");       
		printk("Ilitek: *** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");        
		return 0;	
	}

	if(!ctp_get_system_config()) {            
		printk("Ilitek: %s:read config fail!\n",__func__);            
		return ret;    
	}

	add_ctp_device();

	touch_device_driver.detect = ctp_ts_detect; 	

    /* register driver */

    ret = i2c_add_driver(&touch_device_driver);
    if (ret < 0)
    {
        printk("add ILITEK/MStar touch device driver i2c driver failed.\n");
        return -ENODEV;
    }
    printk("add ILITEK/MStar touch device driver i2c driver.\n");

    return ret;

}

static void __exit touch_driver_exit(void)
{
    printk("remove ILITEK/MStar touch device driver i2c driver.\n");

    i2c_del_driver(&touch_device_driver);
}

module_init(touch_driver_init);
module_exit(touch_driver_exit);
MODULE_LICENSE("GPL");


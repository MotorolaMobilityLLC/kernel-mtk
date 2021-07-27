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
/*MMI_STOPSHIP  <display>: <Need to modify the registration method of I2C devices>*/
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"
#include "lcm_define.h"
#include "disp_dts_gpio.h"
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>

#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>

#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif

#endif
#endif
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
/*****************************************************************************
 * Define
 *****************************************************************************/
#ifndef CONFIG_FPGA_EARLY_PORTING
#define I2C_I2C_LCD_BIAS_CHANNEL 6
#define TPS_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL        /* for I2C channel 0 */
#define I2C_ID_NAME "BL_control"
#define TPS_ADDR 0x36

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info BL_control_board_info __initdata = {
	I2C_BOARD_INFO(I2C_ID_NAME,
			TPS_ADDR) };

static struct i2c_client *BL_control_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int BL_control_probe(struct i2c_client *client,
		const struct i2c_device_id *id);
static int BL_control_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/

struct BL_control_dev {
	struct i2c_client *client;

};

static const struct of_device_id _lcm_i2c_of_match[] = {
	{ .compatible = "mediatek,I2C_LCD_BACKLIGHT", },
	{},
};

static const struct i2c_device_id BL_control_id[] = {
	{I2C_ID_NAME, 0},
	{}
};
static struct i2c_driver BL_control_iic_driver = {
	.id_table = BL_control_id,
	.probe = BL_control_probe,
	.remove = BL_control_remove,
	/* .detect               = mt6605_detect, */
	.driver = {
		.owner = THIS_MODULE,
		.name = "BL_control",
		.of_match_table = _lcm_i2c_of_match,
	},
};

static int BL_control_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	pr_info("[LCM]%s\n",__func__);
	BL_control_i2c_client = client;
	return 0;
}

static int BL_control_remove(struct i2c_client *client)
{
	pr_info("[LCM]%s\n",__func__);
	//BL_control_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

int BL_control_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = BL_control_i2c_client;
	char write_data[2] = { 0 };
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_notice("[LCM]BL_control write data fail !!\n");
	return ret;
}

static int __init BL_control_iic_init(void)
{
	pr_info("[LCM/lsy]%s\n",__func__);
	i2c_register_board_info(TPS_I2C_BUSNUM, &BL_control_board_info, 1);
	pr_info("[LCM]%s\n",__func__);
	i2c_add_driver(&BL_control_iic_driver);
	pr_info("[LCM]%s\n",__func__);
	return 0;
}

static void __exit BL_control_iic_exit(void)
{
	pr_info("[LCM]%s\n",__func__);
	i2c_del_driver(&BL_control_iic_driver);
}


module_init(BL_control_iic_init);
module_exit(BL_control_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK BL_control I2C Driver");
MODULE_LICENSE("GPL");
#endif


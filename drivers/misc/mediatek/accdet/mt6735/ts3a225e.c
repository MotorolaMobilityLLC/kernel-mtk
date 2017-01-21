/* drivers/hwmon/mt6516/amit/IQS128.c - IQS128/PS driver
 *
 * Author: MingHsien Hsieh <minghsien.hsieh@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/module.h>

#include "upmu_common.h"
/*
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
*/
#include <linux/io.h>
#include "ts3a225e.h"

/******************************************************************************
 * configuration
*******************************************************************************/
#define TS3A225E_DEV_NAME     "TS3A225E"

struct i2c_client *ts3a225e_i2c_client;
static const struct i2c_device_id ts3a225e_i2c_id[] = { {"TS3A225E", 0}, {} };
/*static struct i2c_board_info i2c_TS3A225E __initdata = { I2C_BOARD_INFO("TS3A225E", (0X76 >> 1)) };*/

static int ts3a225e_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	unsigned char devicve_id[1] = {0};

	pr_warn("ts3a225e_i2c_probe\n");

	ts3a225e_i2c_client = client;

	ts3a225e_read_byte(0x01, &devicve_id[0]);
	pr_warn("ts3a225e_i2c_probe ID=%x\n", devicve_id[0]);

	return 0;
}

static int ts3a225e_i2c_remove(struct i2c_client *client)
{
	pr_warn("TS3A225E_i2c_remove\n");

	ts3a225e_i2c_client = NULL;

	return 0;
}

static int ts3a225e_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	pr_debug("TS3A225E_i2c_suspend\n");

	return 0;
}

static int ts3a225e_i2c_resume(struct i2c_client *client)
{
	pr_debug("TS3A225E_i2c_resume\n");

	return 0;
}

static const struct of_device_id ts3a225e_of_match[] = {
	{.compatible = "mediatek,ts3a225e"},
	{},
};

static struct i2c_driver ts3a225e_i2c_driver = {
	.probe = ts3a225e_i2c_probe,
	.remove = ts3a225e_i2c_remove,
	.suspend = ts3a225e_i2c_suspend,
	.resume = ts3a225e_i2c_resume,
	.id_table = ts3a225e_i2c_id,
	.driver = {
		   .name = TS3A225E_DEV_NAME,
		   .of_match_table = ts3a225e_of_match,
		   },
};

static DEFINE_MUTEX(ts3a225e_i2c_access);

int ts3a225e_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[1] = { 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&ts3a225e_i2c_access);

	/*new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;*/
	ts3a225e_i2c_client->ext_flag =
	    ((ts3a225e_i2c_client->ext_flag) & I2C_MASK_FLAG) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(ts3a225e_i2c_client, &cmd_buf[0], (1 << 8 | 1));
	if (ret < 0) {
		/*new_client->addr = new_client->addr & I2C_MASK_FLAG;*/
		ts3a225e_i2c_client->ext_flag = 0;

		mutex_unlock(&ts3a225e_i2c_access);
		return 0;
	}

	readData = cmd_buf[0];
	*returnData = readData;

	/*new_client->addr = new_client->addr & I2C_MASK_FLAG;*/
	ts3a225e_i2c_client->ext_flag = 0;

	mutex_unlock(&ts3a225e_i2c_access);
	return 1;
}

int ts3a225e_write_byte(unsigned char cmd, unsigned char writeData)
{
	char write_data[2] = { 0 };
	int ret = 0;

	mutex_lock(&ts3a225e_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	ts3a225e_i2c_client->ext_flag = ((ts3a225e_i2c_client->ext_flag) & I2C_MASK_FLAG) | I2C_DIRECTION_FLAG;

	ret = i2c_master_send(ts3a225e_i2c_client, write_data, 2);
	if (ret < 0) {
		ts3a225e_i2c_client->ext_flag = 0;
		mutex_unlock(&ts3a225e_i2c_access);
		return 0;
	}

	ts3a225e_i2c_client->ext_flag = 0;
	mutex_unlock(&ts3a225e_i2c_access);
	return 1;
}

/******************************************************************************
 * extern functions
*******************************************************************************/
/*----------------------------------------------------------------------------*/
static int ts3a225e_probe(struct platform_device *pdev)
{
	pr_warn("ts3a225e_probe\n");

	if (i2c_add_driver(&ts3a225e_i2c_driver)) {
		pr_err("ts3a225e add driver error\n");
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ts3a225e_remove(struct platform_device *pdev)
{
	pr_warn("ts3a225e remove\n");

	i2c_del_driver(&ts3a225e_i2c_driver);

	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct of_device_id audio_switch_of_match[] = {
	{.compatible = "mediatek,audio_switch",},
	{},
};

static struct platform_driver ts3a225e_audio_switch_driver = {
	.probe = ts3a225e_probe,
	.remove = ts3a225e_remove,
	.driver = {
		   .name = "audio_switch",
		   .of_match_table = audio_switch_of_match,
		   }
};

/*----------------------------------------------------------------------------*/
static int __init ts3a225e_init(void)
{
	pr_warn("ts3a225e_init\n");

	/*i2c_register_board_info(3, &i2c_TS3A225E, 1);*/

	if (platform_driver_register(&ts3a225e_audio_switch_driver)) {
		pr_err("ts3a225e failed to register driver");
		return -ENODEV;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit ts3a225e_exit(void)
{
	pr_warn("ts3a225e_exit\n");
}

/*----------------------------------------------------------------------------*/
module_init(ts3a225e_init);
module_exit(ts3a225e_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Dexiang Liu");
MODULE_DESCRIPTION("TS3A225E driver");
MODULE_LICENSE("GPL");

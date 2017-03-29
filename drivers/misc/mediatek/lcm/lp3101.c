#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
#include <linux/uaccess.h>
/* #include <linux/delay.h> */
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include "lp3101.h"
/*****************************************************************************
 * Define
 *****************************************************************************/
#define I2C_ID_NAME "lp3101"

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static const struct of_device_id lcm_of_match[] = {
	{.compatible = "mediatek,i2c_lcd_bias"},
	{},
};

static struct i2c_client *lp3101_i2c_client;

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
static int lp3101_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int lp3101_remove(struct i2c_client *client);
/*****************************************************************************
 * Data Structure
 *****************************************************************************/
/*
   static struct lp3101_dev	{
   struct i2c_client	*client;
   };
 */
static const struct i2c_device_id lp3101_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

static struct i2c_driver lp3101_i2c_driver = {
	.id_table		= lp3101_id,
	.probe		= lp3101_probe,
	.remove		= lp3101_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "lp3101",
#if !defined(CONFIG_MTK_LEGACY)
		.of_match_table = lcm_of_match,
#endif
	},
};


/*****************************************************************************
 * Function
 *****************************************************************************/
static int lp3101_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	LP_LOG("lp3101_probe: info==>name=%s addr=0x%x\n", client->name, client->addr);
	lp3101_i2c_client = client;

	return 0;
}

static int lp3101_remove(struct i2c_client *client)
{
	LP_LOG("lp3101_remove\n");
	lp3101_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}

int lp3101_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;

	struct i2c_client *client = lp3101_i2c_client;
	char write_data[2] = {0};

	if (client == NULL) {
		pr_err("lp3101 write data fail 0 !!\n");
		return -1;
	}

	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_err("lp3101 write data fail 1 !!\n");

	return ret;
}

/*
 * module load/unload record keeping
 */
static int __init lp3101_i2c_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&lp3101_i2c_driver);
	if (ret < 0) {
		pr_err("%s: failed to register lp3101 i2c driver", __func__);
		return -1;
	}

	LP_LOG("lp3101_i2c_init success\n");
	return 0;
}

static void __exit lp3101_i2c_exit(void)
{
	LP_LOG("lp3101_i2c_exit\n");
	i2c_del_driver(&lp3101_i2c_driver);
}

module_init(lp3101_i2c_init);
module_exit(lp3101_i2c_exit);

MODULE_AUTHOR("Erick.wang");
MODULE_DESCRIPTION("MTK lp3101 I2C Driver");
MODULE_LICENSE("GPL");

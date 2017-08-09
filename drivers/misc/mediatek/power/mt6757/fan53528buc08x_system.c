/*
 *  fan53528_system.c
 *  OS. dependent utility Header for FAN53528
 *
 *  copyright (c) 2015 richtek technology corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * this program is free software; you can redistribute it and/or modify
 * it under the terms of the gnu general public license version 2 as
 * published by the free software foundation; either version 2
 * of the license, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "fan53528buc08x_system.h"
#include "fan53528buc08x_buck_core.h"

static DEFINE_MUTEX(fan53528_io_lock);

static struct i2c_client *fan53528_i2c_client;

static int fan53528_read_device(void *client, u32 addr, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
}

static int fan53528_write_device(void *client, u32 addr,
		int len, const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
}

static int fan53528_read_byte(unsigned char addr, unsigned char *val)
{
	int ret = 0;

	ret = fan53528_read_device(fan53528_i2c_client, addr, 1, val);
	if (ret < 0) {
		FAN53528_ERR("%s read 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

static int fan53528_write_byte(unsigned char addr, unsigned char value)
{
	int ret = 0;

	ret = fan53528_write_device(fan53528_i2c_client, addr, 1, &value);
	if (ret < 0) {
		FAN53528_ERR("%s write 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

static void fan53528_lock(void)
{
	mutex_lock(&fan53528_io_lock);
}

static void fan53528_unlock(void)
{
	mutex_unlock(&fan53528_io_lock);
}

struct fan53528_system_intf fan53528_intf = {
	.lock = fan53528_lock,
	.unlock = fan53528_unlock,
	.io_read = fan53528_read_byte,
	.io_write = fan53528_write_byte,
};

static void fan53528_hw_init(void)
{
	/* initial setting */
}

static int fan53528_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	unsigned char regval = 0;

	pr_info("%s\n", __func__);
	fan53528_i2c_client = kmalloc(sizeof(*client) , GFP_KERNEL);
	fan53528_i2c_client = client;

	ret = fan53528_read_byte(FAN53528_REG_ID1, &regval);
	if (ret < 0) {
		pr_err("%s chip not match\n", __func__);
		return -ENODEV;
	}

	fan53528_hw_init();
	return 0;
}

static int fan53528_i2c_remove(struct i2c_client *client)
{
	mutex_destroy(&fan53528_io_lock);
	pr_info("%s remove Succefully\n", __func__);
	return 0;
}

static int fan53528_i2c_suspend(struct device *dev)
{
	return 0;
}

static int fan53528_i2c_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(fan53528_pm_ops,
		fan53528_i2c_suspend, fan53528_i2c_resume);

static const struct i2c_device_id fan53528_device_id[] = {
	{ "fan53528buc08x", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, fan53528_device_id);

static const struct of_device_id fan53528_of_match_id[] = {
	{ .compatible = "mediatek,ext_buck_lp4", },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, fan53528_of_device_id);


static struct i2c_driver fan53528buc08x_driver = {
	.driver = {
		.name = "fan53528buc08x",
		.of_match_table = fan53528_of_match_id,
		.pm = &fan53528_pm_ops,
	},
	.probe = fan53528_i2c_probe,
	.remove = fan53528_i2c_remove,
	.id_table = fan53528_device_id,
};

static int __init fan53528buc08x_init(void)
{
	pr_info("%s : initializing...\n", __func__);
	return i2c_add_driver(&fan53528buc08x_driver);
}
module_init(fan53528buc08x_init);

static void __exit fan53528buc08x_exit(void)
{
	i2c_del_driver(&fan53528buc08x_driver);
}
module_exit(fan53528buc08x_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BUCK Driver for FAN53528");
MODULE_AUTHOR("Sakya <jeff_chang@richtek.com>");
MODULE_VERSION("1.0.0_MTK");

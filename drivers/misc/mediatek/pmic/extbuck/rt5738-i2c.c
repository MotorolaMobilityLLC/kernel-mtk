/*
 *  Driver for Richtek RT5738 I2C
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include "rt5738.h"

#define RT5738_DRV_VERSION	"1.0.0_MTK"

static int rt5738_read_device(void *client, u32 addr, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
}

static int rt5738_write_device(void *client, u32 addr,
					int len, const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
}

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT5738_REG_VSEL0, 1, RT_WBITS_WR_ONCE, {});
RT_REG_DECL(RT5738_REG_VSEL1, 1, RT_WBITS_WR_ONCE, {});
RT_REG_DECL(RT5738_REG_CTRL1, 1, RT_WBITS_WR_ONCE, {});
RT_REG_DECL(RT5738_REG_ID1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT5738_REG_ID2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT5738_REG_MONITOR, 1, RT_VOLATILE, {});
RT_REG_DECL(RT5738_REG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT5738_REG_CTRL3, 1, RT_WBITS_WR_ONCE, {});
RT_REG_DECL(RT5738_REG_CTRL4, 1, RT_WBITS_WR_ONCE, {});

static const rt_register_map_t rt5738_regmap[] = {
	RT_REG(RT5738_REG_VSEL0),
	RT_REG(RT5738_REG_VSEL1),
	RT_REG(RT5738_REG_CTRL1),
	RT_REG(RT5738_REG_ID1),
	RT_REG(RT5738_REG_ID2),
	RT_REG(RT5738_REG_MONITOR),
	RT_REG(RT5738_REG_CTRL2),
	RT_REG(RT5738_REG_CTRL3),
	RT_REG(RT5738_REG_CTRL4),
};

static struct rt_regmap_properties rt5738_regmap_props = {
	.name = "rt5738",
	.aliases = "rt5738",
	.register_num = ARRAY_SIZE(rt5738_regmap),
	.rm = rt5738_regmap,
	.rt_regmap_mode = RT_CACHE_WR_THROUGH,
};

static struct rt_regmap_fops rt5738_regmap_fops = {
	.read_device = rt5738_read_device,
	.write_device = rt5738_write_device,
};

#endif /* CONFIG_RT_REGMAP */

int rt5738_read_byte(void *client,
			uint32_t addr, uint32_t *val)
{
	int ret = 0;
	struct i2c_client *i2c = (struct i2c_client *)client;

	ret = rt5738_read_device(i2c, addr, 1, val);
	if (ret < 0) {
		RT5738_ERR("%s read 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

int rt5738_write_byte(void *client,
			uint32_t addr, uint32_t value)
{
	int ret = 0;
	struct i2c_client *i2c = (struct i2c_client *)client;

	ret = rt5738_write_device(i2c, addr, 1, &value);
	if (ret < 0) {
		RT5738_ERR("%s write 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

int rt5738_assign_bit(void *client, uint32_t reg,
					uint32_t  mask, uint32_t data)
{
	struct i2c_client *i2c = (struct i2c_client *)client;
	struct rt5738_chip *ri = i2c_get_clientdata(i2c);
	unsigned char tmp = 0;
	uint32_t regval = 0;
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = rt5738_read_byte(i2c, reg, &regval);
	if (ret < 0) {
		RT5738_ERR("%s read fail reg0x%02x data0x%02x\n",
				__func__, reg, data);
		goto OUT_ASSIGN;
	}
	tmp = ((regval & 0xff) & ~mask);
	tmp |= (data & mask);
	ret = rt5738_write_byte(i2c, reg, tmp);
	if (ret < 0)
		RT5738_ERR("%s write fail reg0x%02x data0x%02x\n",
				__func__, reg, tmp);
OUT_ASSIGN:
	mutex_unlock(&ri->io_lock);
	return  ret;
}

static int rt5738_regmap_init(struct rt5738_chip *chip)
{
#ifdef CONFIG_RT_REGMAP
	chip->regmap_dev = rt_regmap_device_register(&rt5738_regmap_props,
		&rt5738_regmap_fops, chip->dev, chip->i2c, chip);
	if (!chip->regmap_dev)
		return -EINVAL;
#endif /* CONFIG_RT_REGMAP */
	return 0;
}

static int rt5738_i2c_probe(struct i2c_client *i2c,
					const struct i2c_device_id *id)
{
	struct rt5738_chip *chip;
	int ret;

	RT5738_INFO("%s ver(%s)\n", __func__, RT5738_DRV_VERSION);
	chip = devm_kzalloc(&i2c->dev, sizeof(struct rt5738_chip), GFP_KERNEL);

	chip->i2c = i2c;
	chip->dev = &i2c->dev;
	mutex_init(&chip->io_lock);

	i2c_set_clientdata(i2c, chip);

	ret = rt5738_regmap_init(chip);
	if (ret < 0) {
		RT5738_ERR("%s rt5738 regmap init fail\n", __func__);
		return -EINVAL;
	}

	ret = rt5738_regulator_init(chip);
	if (ret < 0) {
		RT5738_ERR("%s rt5738 regiater regulator fail\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int rt5738_i2c_remove(struct i2c_client *i2c)
{
	struct rt5738_chip *chip = i2c_get_clientdata(i2c);

	if (chip) {
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(chip->regmap_dev);
#endif /* CONFIG_RT_REGMAP */
		mutex_destroy(&chip->io_lock);
	}
	return 0;
}

static const struct of_device_id rt_match_table[] = {
	{ .compatible = "richtek,rt5738", },
	{ },
};

static const struct i2c_device_id rt_dev_id[] = {
	{"rt5738", 0},
	{ }
};

static struct i2c_driver rt5738_i2c_driver = {
	.driver = {
		.name	= "rt5738",
		.owner	= THIS_MODULE,
		.of_match_table	= rt_match_table,
	},
	.probe	= rt5738_i2c_probe,
	.remove	= rt5738_i2c_remove,
	.id_table = rt_dev_id,
};

static int __init rt5738_i2c_init(void)
{
	RT5738_INFO("%s\n", __func__);
	return i2c_add_driver(&rt5738_i2c_driver);
}
subsys_initcall(rt5738_i2c_init);

static void __exit rt5738_i2c_exit(void)
{
	i2c_del_driver(&rt5738_i2c_driver);
}
module_exit(rt5738_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Chang <jeff_chang@richtek.com>");
MODULE_VERSION(RT5738_DRV_VERSION);
MODULE_DESCRIPTION("Regulator driver for RT5738");

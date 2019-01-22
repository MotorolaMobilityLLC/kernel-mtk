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
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/mediatek/mtk_regulator_core.h>
#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

#define RT5738_DRV_VERSION	"1.0.0_MTK"

#define RT5738_REG_VSEL0	(0x00)
#define RT5738_REG_VSEL1	(0x01)
#define RT5738_REG_CTRL1	(0x02)
#define RT5738_REG_ID1		(0x03)
#define RT5738_REG_ID2		(0x04)
#define RT5738_REG_MONITOR	(0x05)
#define RT5738_REG_CTRL2	(0x06)
#define RT5738_REG_CTRL3	(0x07)
#define RT5738_REG_CTRL4	(0x08)

#define RT5738_INFO(format, args...) pr_info(format, ##args)
#define RT5738_ERR(format, args...)	pr_err(format, ##args)

enum {
	RT5738_A,
	RT5738_B,
	RT5738_C,
};

static const char * const rt5738_text[] = {
	"rt5738a",
	"rt5738b",
	"rt5738c",
};

struct rt5738_regulator_info {
	struct device *dev;
	struct i2c_client *i2c;
	struct mutex io_lock;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_props;
#endif /* #ifdef CONFIG_RT_REGMAP */
	struct  mtk_simple_regulator_desc *mreg_desc;
	int id;
	int pin_sel;
	int ramp_dly;
	unsigned char mode_reg;
	unsigned char mode_bit;
	unsigned char ramp_up_reg;
	unsigned char ramp_down_reg;
	unsigned char ramp_mask;
	unsigned char ramp_shift;
};

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
	struct rt5738_regulator_info *ri = i2c_get_clientdata(i2c);
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

#define rt5738_set_bit(i2c, reg, mask) \
	rt5738_assign_bit(i2c, reg, mask, mask)

#define rt5738_clr_bit(i2c, reg, mask) \
	rt5738_assign_bit(i2c, reg, mask, 0x00)

static int rt5738_regmap_init(struct rt5738_regulator_info *info)
{
#ifdef CONFIG_RT_REGMAP
	info->regmap_dev = rt_regmap_device_register(info->regmap_props,
		&rt5738_regmap_fops, info->dev, info->i2c, info);
	if (!info->regmap_dev)
		return -EINVAL;
#endif /* CONFIG_RT_REGMAP */
	return 0;
}

#define rt5738_buck0_vol_reg	RT5738_REG_VSEL0
#define rt5738_buck0_vol_mask	(0xff)
#define rt5738_buck0_vol_shift	(0)
#define rt5738_buck0_enable_reg	RT5738_REG_CTRL2
#define rt5738_buck0_enable_bit	(0x01)
#define rt5738_buck0_min_uV	(300000)
#define rt5738_buck0_max_uV	(1850000)
#define rt5738_buck0_id		(0)

#define rt5738_buck1_vol_reg	RT5738_REG_VSEL1
#define rt5738_buck1_vol_mask	(0xff)
#define rt5738_buck1_vol_shift	(0)
#define rt5738_buck1_enable_reg	RT5738_REG_CTRL2
#define rt5738_buck1_enable_bit	(0x02)
#define rt5738_buck1_min_uV	(300000)
#define rt5738_buck1_max_uV	(1850000)
#define rt5738_buck1_id		(1)

static struct mtk_simple_regulator_control_ops rt5738_mreg_ctrl_ops = {
	.register_read = rt5738_read_byte,
	.register_write = rt5738_write_byte,
	.register_update_bits = rt5738_assign_bit,
};

static int rt5738_list_voltage(
	struct mtk_simple_regulator_desc *mreg_desc, unsigned selector)
{
	unsigned int vout = 0;

	if (selector > 200)
		vout = 1300000 + selector * 10000;
	else
		vout = mreg_desc->min_uV + 5000 * selector;

	if (vout > mreg_desc->max_uV)
		return -EINVAL;
	return vout;
}

static struct mtk_simple_regulator_desc rt5738_desc_table[] = {
	mreg_decl(rt5738_buck0,
		rt5738_list_voltage, 256, &rt5738_mreg_ctrl_ops),
	mreg_decl(rt5738_buck1,
		rt5738_list_voltage, 256, &rt5738_mreg_ctrl_ops),
};

#define RT5738_REGULATOR_MODE_REG0	RT5738_REG_CTRL1
#define RT5738_REGULATOR_MODE_REG1	RT5738_REG_CTRL1
#define RT5738_REGULATOR_MODE_MASK0	(0x01)
#define RT5738_REGULATOR_MODE_MASK1	(0x02)

#define RT5738_REGULATOR_DECL(_id)	\
{	\
	.mode_reg = RT5738_REGULATOR_MODE_REG##_id,	\
	.mode_bit = RT5738_REGULATOR_MODE_MASK##_id,	\
	.ramp_up_reg = RT5738_REG_CTRL3,		\
	.ramp_down_reg = RT5738_REG_CTRL4,		\
	.ramp_mask = 0x3f,	\
	.ramp_shift = 0,					\
}

static struct rt5738_regulator_info rt5738_regulator_infos[] = {
	RT5738_REGULATOR_DECL(0),
	RT5738_REGULATOR_DECL(1),
};

static struct rt5738_regulator_info *rt5738_find_regulator_info(int id)
{
	struct rt5738_regulator_info *info;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(rt5738_regulator_infos); i++) {
		info = &rt5738_regulator_infos[i];
		if (info->mreg_desc->rdesc.id == id)
			return info;
	}
	return NULL;
}

static int rt5738_set_mode(
	struct mtk_simple_regulator_desc *mreg_desc, unsigned int mode)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret;

	switch (mode) {
	case REGULATOR_MODE_FAST: /* force pwm mode */
		ret = rt5738_set_bit(mreg_desc->client,
				info->mode_reg, info->mode_bit);
		break;
	case REGULATOR_MODE_NORMAL:
	default:
		ret = rt5738_clr_bit(mreg_desc->client,
				info->mode_reg, info->mode_bit);
		break;
	}
	return ret;
}

static unsigned int rt5738_get_mode(
		struct mtk_simple_regulator_desc *mreg_desc)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret;
	uint32_t regval = 0;

	ret = rt5738_read_byte(mreg_desc->client, info->mode_reg, &regval);
	if (ret < 0) {
		RT5738_ERR("%s read mode fail\n", __func__);
		return ret;
	}

	if (regval & info->mode_bit)
		return REGULATOR_MODE_FAST;
	return REGULATOR_MODE_NORMAL;
}

static int rt5738_set_ramp_dly(struct mtk_simple_regulator_desc *mreg_desc,
								int ramp_dly)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret = 0;
	unsigned char regval;

	if (ramp_dly > 63)
		regval = 63;
	else
		regval = ramp_dly;

	ret = rt5738_assign_bit(mreg_desc->client, info->ramp_up_reg,
			info->ramp_mask, regval << info->ramp_shift);
	ret = rt5738_assign_bit(mreg_desc->client, info->ramp_down_reg,
			info->ramp_mask, regval << info->ramp_shift);

	return ret;
}

static struct mtk_simple_regulator_ext_ops rt5738_regulator_ext_ops = {
	.set_mode = rt5738_set_mode,
	.get_mode = rt5738_get_mode,
};

static int rt5738_parse_dt(
	struct rt5738_regulator_info *info, struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 val;
	int ret;

	if (!np) {
		RT5738_ERR("%s cant find node (0x%02x)\n",
				__func__, info->i2c->addr);
		return 0;
	}
	ret = of_property_read_u32(np, "ramp_dly", &val);
	if (ret >= 0)
		info->pin_sel = val ? 1 : 0;
	else
		info->pin_sel = 1;

#ifdef CONFIG_RT_REGMAP
	info->regmap_props = devm_kzalloc(dev,
		sizeof(struct rt_regmap_properties), GFP_KERNEL);

	info->regmap_props->name = rt5738_text[info->id];
	info->regmap_props->aliases = rt5738_text[info->id];
	info->regmap_props->register_num = ARRAY_SIZE(rt5738_regmap);
	info->regmap_props->rm = rt5738_regmap;
	info->regmap_props->rt_regmap_mode = RT_CACHE_WR_THROUGH;
#endif /* CONFIG_RT_REGMAP */

	ret = of_property_read_u32(np, "ramp_dly", &val);
	if (ret >= 0) {
		RT5738_INFO("%s set ramp dly(%x)\n", __func__, info->ramp_dly);
		info->ramp_dly = val;
	} else {
		RT5738_ERR("%s use chip default ramp dly\n", __func__);
		info->ramp_dly = 0;
	}

	return 0;
}

static int rt5738_i2c_probe(struct i2c_client *i2c,
					const struct i2c_device_id *id)
{
	struct rt5738_regulator_info *info;
	int ret;

	RT5738_INFO("%s ver(%s) slv(0x%02x)\n",
		__func__, RT5738_DRV_VERSION, i2c->addr);

	info = devm_kzalloc(&i2c->dev,
		sizeof(struct rt5738_regulator_info), GFP_KERNEL);
	switch (i2c->addr) {
	case 0x50:
		info->id = RT5738_A;
		break;
	case 0x52:
		info->id = RT5738_C;
		break;
	case 0x57:
		info->id = RT5738_B;
		break;
	default:
		RT5738_ERR("%s invalid Slave Addr\n", __func__);
		return -ENODEV;
	}

	info->i2c = i2c;
	info->dev = &i2c->dev;
	mutex_init(&info->io_lock);

	i2c_set_clientdata(i2c, info);

	ret = rt5738_parse_dt(info, &i2c->dev);
	if (ret < 0) {
		RT5738_ERR("%s parse dt (%x) fail\n", __func__, i2c->addr);
		return -EINVAL;
	}

	ret = rt5738_regmap_init(info);
	if (ret < 0) {
		RT5738_ERR("%s rt5738 regmap init fail\n", __func__);
		return -EINVAL;
	}

	info->mreg_desc = devm_kzalloc(&i2c->dev,
		sizeof(struct mtk_simple_regulator_desc), GFP_KERNEL);
	memcpy(info->mreg_desc, &rt5738_desc_table[info->pin_sel],
			sizeof(struct mtk_simple_regulator_desc));
	info->mreg_desc->client = i2c;

	ret = mtk_simple_regulator_register(info->mreg_desc,
			info->dev, &rt5738_regulator_ext_ops, NULL);
	if (ret < 0) {
		RT5738_ERR("%s rt5738 register regulator fail\n", __func__);
		return -EINVAL;
	}
	RT5738_INFO("%s (%s) register simple regulator ok\n", __func__,
		info->mreg_desc->rdesc.name);

	if (info->ramp_dly) {
		ret = rt5738_set_ramp_dly(info->mreg_desc, info->ramp_dly);
		if (ret < 0) {
			RT5738_ERR("%s (%s)set ramp dly fail\n", __func__,
				info->mreg_desc->rdesc.name);
		}
	}
	pr_info("%s Successfully\n", __func__);
	return 0;
}

static int rt5738_i2c_remove(struct i2c_client *i2c)
{
	struct rt5738_regulator_info *info = i2c_get_clientdata(i2c);

	if (info) {
		mtk_simple_regulator_unregister(info->mreg_desc);
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(info->regmap_dev);
#endif /* CONFIG_RT_REGMAP */
		mutex_destroy(&info->io_lock);
	}
	return 0;
}

static const struct of_device_id rt_match_table[] = {
	{ .compatible = "mediatek,ext_buck_lp4x", },
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

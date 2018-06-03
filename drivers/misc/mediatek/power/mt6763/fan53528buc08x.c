/*
 * fan53528_regulator.c
 * FAN53528 Regulator / Buck_Driver
 * Copyright (C) 2014
 * Author: Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/mutex.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif /* CONFIG_DEBUG_FS */

static u8 fan53528_trace_flag;
static u8 fan53528_info_flag;
#define FAN53528_INFO(format, args...) do {\
	if (fan53528_info_flag)	\
		pr_info(format, ##args);\
} while (0)

#define FAN53528_CHIP_ID	(0x8108)

/* Voltage setting */
#define FAN53528_REG_VSEL0	(0x00)
#define FAN53528_REG_VSEL1	(0x01)
/* Control register */
#define FAN53528_REG_CONTROL	(0x02)
/* IC Type */
#define FAN53528_REG_ID1	(0x03)
/* IC mask version */
#define FAN53528_REG_ID2	(0x04)
/* Monitor register */
#define FAN53528_REG_MONITOR	(0x05)

/* Buck enable bit */
#define FAN53528_VSELEN_MASK	(0x80)
/* Voltage output bit */
#define FAN53528_VOUT_SHIFT	(0)
#define FAN53528_VOUT_MASK	(0x7f)

/* force pwm mode select bit */
#define FAN53528_PWMSEL0_MASK	(0x01)
#define FAN53528_PWMSEL1_MASK	(0x02)

/* slew rate select bit */
#define FAN53528_SLEWRATE_MASK	(0x70)
#define FAN53528_SLEWRATE_SHIFT	(4)

/* power good mask */
#define FAN53528_POWER_GOOD_MASK	(0x80)

struct fan53528_regulator_info {
	struct regulator_desc *desc;
	struct regulator_dev *regulator;
	struct i2c_client *i2c;
	struct device *dev;
	struct mutex io_lock;
	int min_uv;
	int max_uv;
	u8 reg_addr;
};

static int fan53528_read_device(void *client, u32 addr, int len, void *dst)
{
	int ret = 0;
	struct i2c_client *i2c = (struct i2c_client *)client;

	ret = i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);

	if (ret < 0)
		FAN53528_INFO("%s: I2CR[0x%02x] failed\n", __func__, addr);
	else
		FAN53528_INFO("%s: I2CR[0x%02x] = 0x%02x\n",
				__func__, addr, *((u8 *)dst));
	return ret;
}

static int fan53528_write_device(void *client, u32 addr,
		int len, const void *src)
{
	int ret = 0;
	struct i2c_client *i2c = (struct i2c_client *)client;

	ret = i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
	if (ret < 0)
		FAN53528_INFO("%s: I2CW[0x%02x] = 0x%02x failed\n",
				__func__, addr, *((u8 *)src));
	else
		FAN53528_INFO("%s: I2CW[0x%02x] = 0x%02x\n",
				__func__, addr, *((u8 *)src));
	return ret;
}

static int fan53528_read_byte(struct i2c_client *i2c,
			unsigned char addr, unsigned char *val)
{
	int ret = 0;

	ret = fan53528_read_device(i2c, addr, 1, val);
	if (ret < 0) {
		pr_err("%s read 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

static int fan53528_write_byte(struct i2c_client *i2c,
			unsigned char addr, unsigned char value)
{
	int ret = 0;

	ret = fan53528_write_device(i2c, addr, 1, &value);
	if (ret < 0) {
		pr_err("%s write 0x%02x fail\n", __func__, addr);
		return ret;
	}
	return ret;
}

static int fan53528_assign_bit(struct i2c_client *i2c, unsigned char reg,
		unsigned char mask, unsigned char data)
{
	struct fan53528_regulator_info *ri = i2c_get_clientdata(i2c);
	unsigned char tmp = 0, regval = 0;
	int ret = 0;

	mutex_lock(&ri->io_lock);
	ret = fan53528_read_byte(i2c, reg, &regval);
	if (ret < 0) {
		pr_err("%s read fail reg0x%02x data0x%02x\n",
				__func__, reg, data);
		goto OUT_ASSIGN;
	}
	tmp = ((regval & 0xff) & ~mask);
	tmp |= (data & mask);
	ret = fan53528_write_byte(i2c, reg, tmp);
	if (ret < 0)
		pr_err("%s write fail reg0x%02x data0x%02x\n",
				__func__, reg, tmp);
OUT_ASSIGN:
	mutex_unlock(&ri->io_lock);
	return  ret;
}

#define fan53528_set_bit(i2c, reg, mask) \
	fan53528_assign_bit(i2c, reg, mask, mask)

#define fan53528_clr_bit(i2c, reg, mask) \
	fan53528_assign_bit(i2c, reg, mask, 0x00)

#ifdef CONFIG_DEBUG_FS
struct rt_debug_st {
	void *data_ptr;
	int id;
};

struct dentry *debugfs_rt_dir;
struct dentry *debugfs_file[5];
struct rt_debug_st rtdbg_data[5];

enum {
	FAN53528_DBG_REG,
	FAN53528_DBG_DATA,
	FAN53528_DBG_REGS,
	FAN53528_DBG_INFO,
	FAN53528_DBG_TRACE,
};

static int de_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}
static ssize_t de_read(struct file *file,
	char __user *user_buffer, size_t count, loff_t *position)
{
	struct rt_debug_st *db = file->private_data;
	struct fan53528_regulator_info *info = db->data_ptr;
	char tmp[200] = {0};
	int ret, i;
	u8 regval;

	switch (db->id) {
	case FAN53528_DBG_REG:
		sprintf(tmp, "0x%02x\n", info->reg_addr);
		break;
	case FAN53528_DBG_DATA:
		ret = fan53528_read_byte(info->i2c,
						info->reg_addr, &regval);
		if (ret < 0)
			dev_err(&info->i2c->dev, "i2c read fail\n");
		else
			sprintf(tmp, "0x%02x\n", regval);
		break;
	case FAN53528_DBG_REGS:
		for (i = FAN53528_REG_VSEL0;
				i <= FAN53528_REG_MONITOR; i++) {
			ret = fan53528_read_byte(info->i2c, i, &regval);
			sprintf(tmp+strlen(tmp),
				"reg0x%02x = 0x%02x\n", i, regval);
		}
			break;
	case FAN53528_DBG_INFO:
		sprintf(tmp, "fan53528_info_flag = %d\n", fan53528_info_flag);
		break;
	case FAN53528_DBG_TRACE:
		sprintf(tmp, "fan53528_trace_flag = %d\n", fan53528_trace_flag);
		break;
	default:
		break;
	}
	return simple_read_from_buffer(user_buffer, count,
					position, tmp, strlen(tmp));
}

static ssize_t de_write(struct file *file,
	const char __user *user_buffer, size_t count, loff_t *position)
{
	struct rt_debug_st *db = file->private_data;
	struct fan53528_regulator_info *info = db->data_ptr;
	char buf[200] = {0};
	unsigned long yo;
	int ret;

	simple_write_to_buffer(buf, sizeof(buf), position, user_buffer, count);
	ret = kstrtoul(buf, 16, &yo);

	switch (db->id) {
	case FAN53528_DBG_REG:
		if (yo < FAN53528_REG_VSEL0 ||
				yo > FAN53528_REG_MONITOR) {
			pr_info("%s out of range\n", __func__);
			return -1;
		}
		info->reg_addr = yo;
		break;
	case FAN53528_DBG_DATA:
		fan53528_write_byte(info->i2c, info->reg_addr, yo);
		break;
	case FAN53528_DBG_INFO:
		fan53528_info_flag = yo > 0 ? 1 : 0;
		break;
	case FAN53528_DBG_TRACE:
		fan53528_trace_flag = yo > 0 ? 1 : 0;
		break;
	default:
		break;
	}
	return count;
}

static const struct file_operations debugfs_fops = {
	.open = de_open,
	.read = de_read,
	.write = de_write,
};
#endif /* CONFIG_DEBUG_FS */

static int fan53528_list_voltage(
			struct regulator_dev *rdev, unsigned index)
{
	return (index >= rdev->desc->n_voltages) ?
		-EINVAL : 350000 + index * 6250;
}

static inline int check_range(struct fan53528_regulator_info *info,
		int min_uv, int max_uv)
{
	if (min_uv < info->min_uv || min_uv > info->max_uv)
		return -EINVAL;
	return 0;
}

static int fan53528_set_voltage_sel(
		struct regulator_dev *rdev, unsigned selector)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	u8 data;
	const int count = rdev->desc->n_voltages;

	FAN53528_INFO("%s selector = %d\n", __func__, selector);
	if (fan53528_trace_flag)
		WARN_ON(1);
	if (selector > count)
		return -EINVAL;

	data = (u8)selector;
	return fan53528_assign_bit(info->i2c, FAN53528_REG_VSEL1,
				FAN53528_VOUT_MASK, data<<FAN53528_VOUT_SHIFT);
}

static int fan53528_get_voltage_sel(struct regulator_dev *rdev)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;
	unsigned char regval;

	ret = fan53528_read_byte(info->i2c,
				FAN53528_REG_VSEL1, &regval);
	if (ret < 0)
		return ret;
	return (regval&FAN53528_VOUT_MASK) >> FAN53528_VOUT_SHIFT;
}

static int fan53528_enable(struct regulator_dev *rdev)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);

	FAN53528_INFO("%s\n", __func__);
	if (fan53528_trace_flag)
		WARN_ON(1);
	return fan53528_set_bit(info->i2c,
			FAN53528_REG_VSEL1, FAN53528_VSELEN_MASK);
}

static int fan53528_disable(struct regulator_dev *rdev)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);

	FAN53528_INFO("%s\n", __func__);
	if (fan53528_trace_flag)
		WARN_ON(1);
	return fan53528_clr_bit(info->i2c,
			FAN53528_REG_VSEL1, FAN53528_VSELEN_MASK);
}

static int fan53528_is_enabled(struct regulator_dev *rdev)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;
	u8 regval;

	ret = fan53528_read_byte(info->i2c,
				FAN53528_REG_VSEL1, &regval);
	if (ret < 0)
		return ret;
	ret = (regval & FAN53528_VSELEN_MASK) ? 1 : 0;
	return ret;
}

static int fan53528_set_mode(
			struct regulator_dev *rdev, unsigned int mode)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;

	FAN53528_INFO("%s mode =%d\n", __func__, mode);
	if (fan53528_trace_flag)
		WARN_ON(1);
	switch (mode) {
	case REGULATOR_MODE_FAST: /* force pwm mode */
		ret = fan53528_set_bit(info->i2c,
				FAN53528_REG_CONTROL,
				FAN53528_PWMSEL0_MASK);
		break;
	case REGULATOR_MODE_NORMAL: /* auto mode */
	default:
		ret = fan53528_clr_bit(info->i2c,
				FAN53528_REG_CONTROL,
				FAN53528_PWMSEL0_MASK);
		break;
	}
	return ret;
}

static unsigned int fan53528_get_mode(struct regulator_dev *rdev)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;
	unsigned char regval;

	ret = fan53528_read_byte(info->i2c,
					FAN53528_REG_CONTROL, &regval);
	if (ret < 0) {
		pr_err("%s get mode fail\n", __func__);
		return -EIO;
	}

	if (regval & FAN53528_PWMSEL0_MASK)
		return REGULATOR_MODE_FAST; /* force pwm mode */
	return REGULATOR_MODE_NORMAL; /* auto mode */
}

static int fan53528_set_ramp_delay(
			struct regulator_dev *rdev, int ramp_delay)
{
	struct fan53528_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;
	unsigned char regval;

	FAN53528_INFO("%s ramp_delay = %d(uV/uS)\n", __func__, ramp_delay);
	switch (ramp_delay) {
	case 0 ... 999:
	default:
		regval = 7;
		break;
	case 1000 ... 1999:
		regval = 6;
		break;
	case 2000 ... 3999:
		regval = 5;
		break;
	case 4000 ... 7999:
		regval = 4;
		break;
	case 8000 ... 15999:
		regval = 3;
		break;
	case 16000 ... 31999:
		regval = 2;
		break;
	case 32000 ... 63999:
		regval = 1;
		break;
	case 64000:
		regval = 0;
		break;
	}

	ret = fan53528_assign_bit(info->i2c, FAN53528_REG_CONTROL,
		FAN53528_SLEWRATE_MASK, regval<<FAN53528_SLEWRATE_SHIFT);
	return ret;
}

static struct regulator_ops fan53528_regulator_ops = {
	.list_voltage		= fan53528_list_voltage,
	.get_voltage_sel	= fan53528_get_voltage_sel,
	.set_voltage_sel	= fan53528_set_voltage_sel,
	.enable			= fan53528_enable,
	.disable		= fan53528_disable,
	.is_enabled		= fan53528_is_enabled,
	.set_mode		= fan53528_set_mode,
	.get_mode		= fan53528_get_mode,
	.set_ramp_delay		= fan53528_set_ramp_delay,
};

static struct regulator_desc fan53528_regulator_desc = {
	.id = 0,
	.name = "ext_buck_lp4x",
	.n_voltages = 128,
	.ops = &fan53528_regulator_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
};

inline struct regulator_dev *fan53528_regulator_register(
		struct regulator_desc *desc, struct device *dev,
		struct regulator_init_data *init_data, void *driver_data)
{
	struct regulator_config config = {
		.dev = dev,
		.init_data = init_data,
		.driver_data = driver_data,
	};

	pr_info("%s\n", __func__);
	return regulator_register(desc, &config);
}

static struct regulator_init_data *rt_parse_dt(
		struct fan53528_regulator_info *info, struct device *dev)
{
	struct regulator_init_data *init_data = NULL;
#ifdef CONFIG_OF
	struct device_node *np =
		of_find_node_by_name(NULL, "fan53528_buck_info");

	if (!np) {
		pr_err("%s cannnot find node fan53528_buck_info\n", __func__);
		return NULL;
	}

	init_data = of_get_regulator_init_data(dev, np, NULL);

	if (!init_data) {
		pr_err("%s regulator init data is NULL\n", __func__);
		return NULL;
	}
	init_data->constraints.valid_modes_mask |=
		(REGULATOR_MODE_NORMAL|REGULATOR_MODE_FAST);
	init_data->constraints.valid_ops_mask |=
		REGULATOR_CHANGE_MODE;
	pr_info("regulator-name = %s, min_uA = %d, max_uA = %d\n",
			init_data->constraints.name,
			init_data->constraints.min_uV,
			init_data->constraints.max_uV);
#endif /* CONFIG_OF */
	return init_data;
}

static int fan53528_check_id(struct fan53528_regulator_info *ri)
{
	int ret;
	u8 regval[2];

	ret = fan53528_read_byte(ri->i2c,
				FAN53528_REG_ID1, &regval[0]);
	if (ret < 0)
		return ret;

	ret = fan53528_read_byte(ri->i2c,
				FAN53528_REG_ID2, &regval[1]);
	if (ret < 0)
		return ret;

	ret = (regval[0] << 8) | regval[1];
	pr_info("%s ID = 0x%04x\n", __func__, ret);

	if (ret != FAN53528_CHIP_ID)
		return -EINVAL;

	pr_info("%s OK!\n", __func__);
	return 0;
}

static void fan53528_reg_init(struct fan53528_regulator_info *ri)
{
#if 0 /* no need to init @ kernel */
	int ret;

	pr_info("%s\n", __func__);
	ret = fan53528_write_byte(ri->i2c,
			FAN53528_REG_VSEL0, fan53528_regval[0]);
	ret |= fan53528_write_byte(ri->i2c,
			FAN53528_REG_VSEL1, fan53528_regval[1]);
	ret |= fan53528_write_byte(ri->i2c,
			FAN53528_REG_CONTROL, fan53528_regval[2]);
	if (ret < 0)
		pr_err("%s Failed!!\n", __func__);
#endif /* #if 0 */
}

static int fan53528_check_power_good(
			struct fan53528_regulator_info *ri)
{
	int ret = 0;
	u8 regval = 0;

	ret = fan53528_read_byte(ri->i2c, FAN53528_REG_MONITOR, &regval);
	if (ret < 0)
		return ret;

	ret = (regval & FAN53528_POWER_GOOD_MASK) ? 0 : -EINVAL;
	if (!ret)
		pr_info("%s OK!\n", __func__);
	return ret;
}

static int fan53528_regulator_probe(struct i2c_client *i2c,
					const struct i2c_device_id *id)
{
	struct fan53528_regulator_info *ri;
	struct regulator_init_data *init_data = NULL;
	bool use_dt = i2c->dev.of_node;
	int ret;

	pr_info("%s ...\n", __func__);
	ri = devm_kzalloc(&i2c->dev, sizeof(*ri), GFP_KERNEL);
	ri->i2c = i2c;
	ri->dev = &i2c->dev;
	i2c_set_clientdata(i2c, ri);

	ret = fan53528_check_id(ri);
	if (ret < 0) {
		pr_err("%s ID not Match...\n", __func__);
		return -ENODEV;
	}

	ret = fan53528_check_power_good(ri);
	if (ret < 0) {
		pr_err("%s Power not Good\n", __func__);
		return -EINVAL;
	}

	fan53528_reg_init(ri);
	mutex_init(&ri->io_lock);
	ri->desc = &fan53528_regulator_desc;
	if (use_dt)
		init_data = rt_parse_dt(ri, &i2c->dev);
	else {
		pr_err("%s cannot find device node\n", __func__);
		return -ENODEV;
	}

	if (init_data == NULL) {
		pr_err("%s no init data\n", __func__);
		return -EINVAL;
	}

	ri->regulator = fan53528_regulator_register(
				ri->desc, &i2c->dev, init_data, ri);
	if (IS_ERR(ri->regulator)) {
		dev_err(ri->dev,
			"fail to register regulator %s\n", ri->desc->name);
		ret = PTR_ERR(ri->regulator);
		return ret;
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_rt_dir = debugfs_create_dir("fan53528_dbg", 0);
	if (!IS_ERR(debugfs_rt_dir)) {
		rtdbg_data[0].data_ptr = ri;
		rtdbg_data[0].id = FAN53528_DBG_REG;
		debugfs_file[0] = debugfs_create_file("reg", S_IFREG|S_IRUGO,
			debugfs_rt_dir, (void *)&rtdbg_data[0], &debugfs_fops);

		rtdbg_data[1].data_ptr = ri;
		rtdbg_data[1].id = FAN53528_DBG_DATA;
		debugfs_file[1] = debugfs_create_file("data", S_IFREG|S_IRUGO,
			debugfs_rt_dir, (void *)&rtdbg_data[1], &debugfs_fops);

		rtdbg_data[2].data_ptr = ri;
		rtdbg_data[2].id = FAN53528_DBG_REGS;
		debugfs_file[2] = debugfs_create_file("regs", S_IFREG|S_IRUGO,
			debugfs_rt_dir, (void *)&rtdbg_data[2], &debugfs_fops);

		rtdbg_data[3].data_ptr = ri;
		rtdbg_data[3].id = FAN53528_DBG_INFO;
		debugfs_file[3] = debugfs_create_file("info", S_IFREG|S_IRUGO,
			debugfs_rt_dir, (void *)&rtdbg_data[3], &debugfs_fops);

		rtdbg_data[4].data_ptr = ri;
		rtdbg_data[4].id = FAN53528_DBG_TRACE;
		debugfs_file[4] = debugfs_create_file("trace", S_IFREG|S_IRUGO,
			debugfs_rt_dir, (void *)&rtdbg_data[4], &debugfs_fops);
	}
#endif /* CONFIG_DEBUG_FS */
	pr_info("%s Successfully\n", __func__);

	return 0;
}

static int fan53528_regulator_remove(struct i2c_client *i2c)
{
	struct fan53528_regulator_info *ri = i2c_get_clientdata(i2c);

	if (ri) {
		mutex_destroy(&ri->io_lock);
		regulator_unregister(ri->regulator);
	}
#ifdef CONFIG_DEBUG_FS
	if (!IS_ERR(debugfs_rt_dir))
		debugfs_remove_recursive(debugfs_rt_dir);
#endif /* CONFIG_DEBUG_FS */
	return 0;
}

static const struct of_device_id rt_match_table[] = {
	{ .compatible = "mediatek,ext_buck_lp4x", },
	{ },
};

static const struct i2c_device_id rt_dev_id[] = {
	{"fan53528", 0},
	{ }
};

static struct i2c_driver fan53528_regulator_driver = {
	.driver = {
		.name	= "fan53528",
		.owner	= THIS_MODULE,
		.of_match_table	= rt_match_table,
	},
	.probe	= fan53528_regulator_probe,
	.remove	= fan53528_regulator_remove,
	.id_table = rt_dev_id,
};

static int __init fan53528_regulator_init(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&fan53528_regulator_driver);
}
subsys_initcall(fan53528_regulator_init);

static void __exit fan53528_regulator_exit(void)
{
	i2c_del_driver(&fan53528_regulator_driver);
}
module_exit(fan53528_regulator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jeff Chang <jeff_chang@richtek.com>");
MODULE_VERSION("1.0.0_MTK");
MODULE_DESCRIPTION("Regulator driver for FAN53528");

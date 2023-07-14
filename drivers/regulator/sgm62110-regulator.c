// SPDX-License-Identifier: GPL-2.0+
/*
 * SGM62110, Boost Buck
 * Copyright (C) 2022  Motorola Mobility LLC,
 *
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

/*reg 01, bit 3: 1 for pwm/ 0 form lpm*/
#define REG_CONTROL_BIT_FPWM	3

enum sgm62110_registers {
	SGM62110_REG_CONTROL = 1,
	SGM62110_REG_STATUS,
	SGM62110_REG_DEVID,
	SGM62110_REG_VOUT1,
	SGM62110_REG_VOUT2,
	SGM62110_REG_MAX = SGM62110_REG_VOUT2,
};

struct sgm62110_chip {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_desc *rdesc;
	struct regulator_dev *rdev;
	int chip_irq;
	int chip_cs_pin;

	int reg_vout2;
};

static const struct regmap_config sgm62110_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = SGM62110_REG_MAX,
};

static int sgm62110_regulator_set_fpwm_mode(struct sgm62110_chip *chip,
				     unsigned int en)
{
	int val, ret = 0;

	ret = regmap_read(chip->regmap, SGM62110_REG_CONTROL, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read control register(%d)\n",
			ret);
		return ret;
	}
	dev_dbg(chip->dev, "sgm62110 get ctrl %x\n", val);

	val = (val & ~BIT(REG_CONTROL_BIT_FPWM)) | ( (!!en) << REG_CONTROL_BIT_FPWM);

	ret = regmap_write(chip->regmap, SGM62110_REG_CONTROL, val);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read control register(%d)\n",
			ret);
		return ret;
	}

	dev_info(chip->dev, "sgm62110 set pwm %d->%x\n", val, en);

	return ret;
}

static int sgm62110_regulator_set_mode(struct regulator_dev *rdev,
				     unsigned int mode)
{
	struct sgm62110_chip *chip = rdev_get_drvdata(rdev);
	int ret = 0;

	ret = sgm62110_regulator_set_fpwm_mode(chip, mode);

	return ret;
}

static int sgm62110_get_status(struct regulator_dev *rdev)
{
	struct sgm62110_chip *chip = rdev_get_drvdata(rdev);
	int ret;
	unsigned int status = 0;

	ret = regmap_read(chip->regmap, SGM62110_REG_CONTROL, &status);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read control register(%d)\n",
			ret);
		return ret;
	}

	return status;
}


static int count=0;
static int sgm62110__is_enabled_regulator(struct regulator_dev *rdev)
{
	struct sgm62110_chip *chip = rdev_get_drvdata(rdev);
	dev_dbg(chip->dev, "is_enable count =%d \n",count);
	return count;
}

static int sgm62110_regulator_enable_supply(struct regulator_dev *rdev)
{
	struct sgm62110_chip *chip = rdev_get_drvdata(rdev);
	int ret = 0;
	count++;
	dev_dbg(chip->dev, "sgm62110_regulator_enable_supply  count =%d \n",count);
	ret = sgm62110_regulator_set_fpwm_mode(chip, 1);

	return ret;
}

static int sgm62110_regulator_disable_supply(struct regulator_dev *rdev)
{
	struct sgm62110_chip *chip = rdev_get_drvdata(rdev);
	int ret = 0;
	count--;
	dev_dbg(chip->dev, "sgm62110_regulator_disable_supply  count =%d \n",count);
	ret = sgm62110_regulator_set_fpwm_mode(chip, 0);

	return ret;
}

static const struct regulator_ops sgm62110_regulator_ops = {
	.set_mode = sgm62110_regulator_set_mode,
	.get_status = sgm62110_get_status,
	.is_enabled = sgm62110__is_enabled_regulator,
	.enable = sgm62110_regulator_enable_supply,
	.disable = sgm62110_regulator_disable_supply,
};

static const struct regulator_init_data sgm62110_init_default = {
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
};

static struct regulator_desc sgm62110_desc = {
	.name = "bob_buck",
	.of_match = of_match_ptr("buck"),
	.ops = &sgm62110_regulator_ops,
	.regulators_node = of_match_ptr("regulators"),                     \
	.owner = THIS_MODULE,
};

static int sgm62110_regulator_init(struct sgm62110_chip *chip)
{
	struct regulator_config config = {};
	int ret = 0;

	chip->rdesc = &sgm62110_desc;

	config.regmap = chip->regmap;
	config.dev = chip->dev;
	config.driver_data = chip;
	config.init_data = &sgm62110_init_default;

	chip->rdev = devm_regulator_register(chip->dev, chip->rdesc,
						 &config);
	if (IS_ERR(chip->rdev)) {
		dev_err(chip->dev,
			"Failed to register regulator(%s)\n",
			chip->rdesc->name);
		return ret;
	}

	return 0;
}
static int sgm62110_parse_dt(struct device_node *np, struct sgm62110_chip *chip)
{
	int ret = 0;

	ret = of_property_read_u32(np, "mmi,reg-vout2", &chip->reg_vout2);
	if (ret) {
		chip->reg_vout2 = 0;
		dev_err(chip->dev,"Failed to read node of mmi,reg-vout2\n");
	}

	return ret;
}

static int sgm62110_init_reg_vout2(struct sgm62110_chip *chip)
{
	int ret = 0, val = 0;
	bool range = 0;
	unsigned int reg_val = 0;

	if (!chip->reg_vout2)
		return ret;

	ret = regmap_read(chip->regmap, SGM62110_REG_CONTROL, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read control register(%d)\n",
			ret);
		return ret;
	}

	range = !!(val & (1<<6));

	if (range)
		reg_val = (chip->reg_vout2 - 2025) / 25;
	else
		reg_val = (chip->reg_vout2 - 1800) / 25;

	ret = regmap_write(chip->regmap, SGM62110_REG_VOUT2, reg_val);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to wirte reg vout2(%d)\n",
			ret);
		return ret;
	}

	return ret;
}

static int sgm62110_hw_init(struct sgm62110_chip *chip)
{
	sgm62110_init_reg_vout2(chip);

	return 0;
}

static int sgm62110_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sgm62110_chip *chip;
	struct device_node *node = client->dev.of_node;
	int error = 0;
	pr_info("sgm62110_i2c_probe start\n");

	chip = devm_kzalloc(dev, sizeof(struct sgm62110_chip), GFP_KERNEL);
	if (!chip) {
		dev_err(chip->dev, "sgm62110_i2c_probe Memory error...\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, chip);
	chip->chip_irq = client->irq;
	chip->dev = dev;
	chip->regmap = devm_regmap_init_i2c(client, &sgm62110_regmap_config);
	if (IS_ERR(chip->regmap)) {
		error = PTR_ERR(chip->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n",
			error);
		return error;
	}

	sgm62110_regulator_init(chip);

	sgm62110_parse_dt(node, chip);

	sgm62110_hw_init(chip);

	dev_info(chip->dev, "sgm62110_i2c_probe end\n");
	return error;
}

static int sgm62110_i2c_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

static int sgm62110_suspend(struct device *device)
{
	struct i2c_client *client = to_i2c_client(device);
	struct sgm62110_chip *chip = i2c_get_clientdata(client);

	sgm62110_regulator_set_fpwm_mode(chip, FALSE);

	return 0;
}

static int sgm62110_resume(struct device *device)
{
	return 0;
}

static const struct dev_pm_ops sgm62110_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sgm62110_suspend, sgm62110_resume)
};

static const struct i2c_device_id sgm62110_i2c_id[] = {
	{"sgm62110", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm62110_i2c_id);

static struct i2c_driver sgm62110_driver = {
	.driver = {
		.name = "sgm62110",
		.pm = &sgm62110_pm_ops,
	},
	.probe = sgm62110_i2c_probe,
	.remove = sgm62110_i2c_remove,
	.id_table = sgm62110_i2c_id,
};

module_i2c_driver(sgm62110_driver);

MODULE_AUTHOR("motorola");
MODULE_DESCRIPTION("SGM62110 regulator driver");
MODULE_LICENSE("GPL");

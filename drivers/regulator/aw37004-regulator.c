// SPDX-License-Identifier: GPL-2.0+
/*
 * aw37004, Multi-Output Regulators
 * Copyright (C) 2024  Motorola Mobility LLC,
 */

#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

/* Registers */
#define AW37004_REG_NUM (AW37004_CHIP_REV2-AW37004_CHIP_REV+1)

#define AW37004_CHIP_REV 0x00
#define AW37004_CURRENT_LIMITSEL 0x01
#define AW37004_DISCHARGE_RESISTORS 0x02
#define AW37004_LDO1_VOUT 0x03
#define AW37004_LDO2_VOUT 0x04
#define AW37004_LDO3_VOUT 0x05
#define AW37004_LDO4_VOUT 0x06
#define AW37004_LDO1_LDO2_SEQ 0x0a
#define AW37004_LDO3_LDO4_SEQ 0x0b
#define AW37004_LDO_EN 0x0e
#define AW37004_SEQ_STATUS 0x0f
#define AW37004_CHIP_REV2 0x19


/* AW37004_LDO1_VSEL ~ AW37004_LDO4_VSEL =
 * 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 */
#define  AW37004_LDO1_VSEL                      AW37004_LDO1_VOUT
#define  AW37004_LDO2_VSEL                      AW37004_LDO2_VOUT
#define  AW37004_LDO3_VSEL                      AW37004_LDO3_VOUT
#define  AW37004_LDO4_VSEL                      AW37004_LDO4_VOUT


#define  AW37004_VSEL_SHIFT                     0
#define  AW37004_VSEL_MASK                      (0xff << 0)

#define  AW37004_N_VOLTAGES                     256

#define  AW37004_ID                             0x00
#define  AW37004_ID2                            0x04

static int ldo_chipid = -1;

enum slg51000_regulators {
	AW37004_REGULATOR_LDO1 = 0,
	AW37004_REGULATOR_LDO2,
	AW37004_REGULATOR_LDO3,
	AW37004_REGULATOR_LDO4,
	AW37004_MAX_REGULATORS,
};

struct aw37004 {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_desc *rdesc[AW37004_MAX_REGULATORS];
	struct regulator_dev *rdev[AW37004_MAX_REGULATORS];
	int chip_cs_pin;
	int chip_vin1_pin;
	int init_value;
	bool shutdown_ldo;
};

struct aw37004_evt_sta {
	unsigned int sreg;
};

static const struct aw37004_evt_sta aw37004_status_reg = { AW37004_LDO_EN };

static const struct regmap_range aw37004_writeable_ranges[] = {
      /* Do not let useless register writeable */
	regmap_reg_range(AW37004_CURRENT_LIMITSEL, AW37004_SEQ_STATUS),
};

static const struct regmap_range aw37004_readable_ranges[] = {
	regmap_reg_range(AW37004_CHIP_REV, AW37004_CHIP_REV2),
};

static const struct regmap_range aw37004_volatile_ranges[] = {
	regmap_reg_range(AW37004_CURRENT_LIMITSEL, AW37004_SEQ_STATUS),
};

static const struct regmap_access_table aw37004_writeable_table = {
	.yes_ranges	= aw37004_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(aw37004_writeable_ranges),
};

static const struct regmap_access_table aw37004_readable_table = {
	.yes_ranges	= aw37004_readable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(aw37004_readable_ranges),
};

static const struct regmap_access_table aw37004_volatile_table = {
	.yes_ranges	= aw37004_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(aw37004_volatile_ranges),
};

static const struct regmap_config aw37004_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AW37004_CHIP_REV2,
	.wr_table = &aw37004_writeable_table,
	.rd_table = &aw37004_readable_table,
	.volatile_table = &aw37004_volatile_table,
};

static int aw37004_get_current_limit(struct regulator_dev *rdev)
{
	struct aw37004 *chip = rdev_get_drvdata(rdev);
	uint8_t reg_dump[AW37004_REG_NUM];
	uint8_t reg_idx;
	unsigned int val = 0;

	dev_err(chip->dev, "************ start dump aw37004 register ************\n");
	dev_err(chip->dev, "register name =%s \n",rdev->desc->name);
	dev_err(chip->dev, "register 0x00:      chip version\n");
	dev_err(chip->dev, "register 0x01:      LDO CL\n");
	dev_err(chip->dev, "register 0x03~0x06: LDO1~LDO4 OUT Voltage\n");
	dev_err(chip->dev, "register 0x0e:      Bit[3:0] LDO4~LDO1 EN\n");

	for (reg_idx = 0; reg_idx < AW37004_REG_NUM; reg_idx++) {
		regmap_read(chip->regmap, reg_idx, &val);
		reg_dump[reg_idx] = val;
		dev_err(chip->dev, "Reg[0x%02x] = 0x%x", reg_idx, reg_dump[reg_idx]);
	}
	dev_err(chip->dev, "************ end dump aw37004 register ************\n");

	return 0;
}

static int aw37004_get_status(struct regulator_dev * rdev)
{
	struct aw37004 *chip = rdev_get_drvdata(rdev);
	int ret, id = rdev_get_id(rdev);
	unsigned int status = 0;

	ret = regulator_is_enabled_regmap(rdev);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read enable register(%d)\n",
			ret);
		return ret;
	}

	if (!ret)
		return REGULATOR_STATUS_OFF;

	aw37004_get_current_limit(rdev);

	ret = regmap_read(chip->regmap, aw37004_status_reg.sreg, &status);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to read status register(%d)\n",
			ret);
		return ret;
	}

	if (status & (0x01ul << id)) {
		return REGULATOR_STATUS_ON;
	} else {
		return REGULATOR_STATUS_OFF;
	}
}

static const struct regulator_ops aw37004_regl_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear,
	.map_voltage = regulator_map_voltage_linear,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_status = aw37004_get_status,
	.get_current_limit = aw37004_get_current_limit,
};

static int aw37004_of_parse_cb(struct device_node *np,
				const struct regulator_desc *desc,
				struct regulator_config *config)
{
	int ena_gpio;

	ena_gpio = of_get_named_gpio(np, "enable-gpios", 0);
	if (gpio_is_valid(ena_gpio))
		config->ena_gpiod = gpio_to_desc(ena_gpio);

	return 0;
}

#define AW37004_REGL_DESC(_id, _name, _s_name, _min, _step)       \
	[AW37004_REGULATOR_##_id] = {                             \
		.name = #_name,                                    \
		.supply_name = _s_name,                            \
		.id = AW37004_REGULATOR_##_id,                    \
		.of_match = of_match_ptr(#_name),                  \
		.of_parse_cb = aw37004_of_parse_cb,               \
		.ops = &aw37004_regl_ops,                         \
		.regulators_node = of_match_ptr("regulators"),     \
		.n_voltages = AW37004_N_VOLTAGES,                  \
		.min_uV = _min,                                    \
		.uV_step = _step,                                  \
		.linear_min_sel = 0,                               \
		.vsel_mask = AW37004_VSEL_MASK,                   \
		.vsel_reg = AW37004_##_id##_VSEL,                 \
		.enable_reg = AW37004_LDO_EN,       \
		.enable_mask = BIT(AW37004_REGULATOR_##_id),      \
		.type = REGULATOR_VOLTAGE,                         \
		.owner = THIS_MODULE,                              \
	}

static struct regulator_desc aw37004_regls_desc[AW37004_MAX_REGULATORS] = {
	AW37004_REGL_DESC(LDO1, ldo1, "vin1", 600000, 6000),
	AW37004_REGL_DESC(LDO2, ldo2, "vin1", 600000, 6000),
	AW37004_REGL_DESC(LDO3, ldo3, "vin2", 1200000, 12500),
	AW37004_REGL_DESC(LDO4, ldo4, "vin2", 1200000, 12500),
};

static int aw37004_regulator_init(struct aw37004 *chip)
{
	struct regulator_config config = { };
	struct regulator_desc *rdesc;
	u8 vsel_range[1];
	int id, ret = 0;
	const unsigned int ldo_regs[AW37004_MAX_REGULATORS] = {
		AW37004_LDO1_VOUT,
		AW37004_LDO2_VOUT,
		AW37004_LDO3_VOUT,
		AW37004_LDO4_VOUT,
	};

	const unsigned int initial_voltage[AW37004_MAX_REGULATORS] = {
		0x64,//LDO1 DVDD 1.2V
		0x64,//LDO2 DVDD 1.2V
		0x80,//LDO3 AVDD 2.8V
		0x80,//LDO4 AVDD 2.8V
	};

	/*Disable all ldo output by default*/
	ret = regmap_write(chip->regmap, AW37004_LDO_EN, chip->init_value);
	if (ret < 0) {
		dev_err(chip->dev,
			"Disable all LDO output failed!!!\n");
		return ret;
	}
	/* Enable all ldo discharge by default */
	ret = regmap_write(chip->regmap, AW37004_DISCHARGE_RESISTORS, 0x8f);
	if (ret < 0) {
		dev_err(chip->dev,
			"Enable LDO discharge failed!!!\n");
		return ret;
	}
	for (id = 0; id < AW37004_MAX_REGULATORS; id++) {
		chip->rdesc[id] = &aw37004_regls_desc[id];
		rdesc = chip->rdesc[id];
		config.regmap = chip->regmap;
		config.dev = chip->dev;
		config.driver_data = chip;

		ret = regmap_bulk_read(chip->regmap, ldo_regs[id],
				       vsel_range, 1);
		pr_err("aw37004_regulator_init: LDO%d, default value:0x%x", (id+1), vsel_range[0]);
		if (ret < 0) {
			dev_err(chip->dev,
				"Failed to read the ldo register\n");
			return ret;
		}

		ret = regmap_write(chip->regmap, ldo_regs[id], initial_voltage[id]);
		if (ret < 0) {
			dev_err(chip->dev,
				"Failed to write inital voltage register\n");
			return ret;
		}
		pr_err("aw37004_regulator_init: LDO%d, initial value:0x%x", (id+1), initial_voltage[id]);

		chip->rdev[id] = devm_regulator_register(chip->dev, rdesc,
							 &config);
		if (IS_ERR(chip->rdev[id])) {
			ret = PTR_ERR(chip->rdev[id]);
			dev_err(chip->dev,
				"Failed to register regulator(%s):%d\n",
				chip->rdesc[id]->name, ret);
			return ret;
		}
	}

	return 0;
}

static int aw37004_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct aw37004 *chip;
	int error, cs_gpio, vin1_gpio, ret, i, value;

	/* Set all register to initial value when probe driver to avoid register value was modified.
	*/
	const unsigned int initial_register[5][2] = {
		{AW37004_CURRENT_LIMITSEL, 	0x00},
		{AW37004_DISCHARGE_RESISTORS, 	0x00},
		{AW37004_LDO1_LDO2_SEQ, 	0x00},
		{AW37004_LDO3_LDO4_SEQ, 	0x00},
		{AW37004_SEQ_STATUS, 		0x00},
	};
	chip = devm_kzalloc(dev, sizeof(struct aw37004), GFP_KERNEL);
	if (!chip) {
		dev_err(chip->dev, "aw37004_i2c_probe Memory error...\n");
		return -ENOMEM;
	}

	dev_info(chip->dev, "aw37004_i2c_probe Enter...\n");

	cs_gpio = of_get_named_gpio(dev->of_node, "cs-gpios", 0);
	if (cs_gpio > 0) {
		if (!gpio_is_valid(cs_gpio)) {
			dev_err(dev, "Invalid chip select pin\n");
			return -EPERM;
		}

		ret = devm_gpio_request_one(dev, cs_gpio, GPIOF_OUT_INIT_LOW,
					    "aw37004_cs_pin");
		if (ret) {
			dev_err(dev, "GPIO(%d) request failed(%d)\n",
				cs_gpio, ret);
			return ret;
		}

		chip->chip_cs_pin = cs_gpio;
	}

	dev_info(chip->dev, "aw37004_i2c_probe cs_gpio:%d...\n", cs_gpio);

	vin1_gpio = of_get_named_gpio(dev->of_node, "vin1-gpios", 0);
	if (vin1_gpio > 0) {
		if (!gpio_is_valid(vin1_gpio)) {
			dev_err(dev, "Invalid vin1 select pin\n");
			return -EPERM;
		}

		ret = devm_gpio_request_one(dev, vin1_gpio, GPIOF_OUT_INIT_HIGH,
					    "aw37004_vin1_pin");
		if (ret) {
			dev_err(dev, "GPIO(%d) request failed(%d)\n",
				vin1_gpio, ret);
			return ret;
		}

		chip->chip_vin1_pin = vin1_gpio;
	}

	dev_info(chip->dev, "aw37004_i2c_probe vin1_gpio:%d...\n", vin1_gpio);

	if (of_property_read_u32(dev->of_node, "init-value", &value) < 0) {
		dev_info(chip->dev, "aw37004_i2c_probe no init_value, use default 0x0\n");
		value = 0x0;
	}
	chip->init_value = value;
	dev_info(chip->dev, "aw37004_i2c_probe init_value:%d...\n", value);
	chip->shutdown_ldo = of_property_read_bool(dev->of_node, "shutdown-ldo");

	mdelay(10);

	i2c_set_clientdata(client, chip);
	chip->dev = dev;
	chip->regmap = devm_regmap_init_i2c(client, &aw37004_regmap_config);
	if (IS_ERR(chip->regmap)) {
		error = PTR_ERR(chip->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n",
			error);
		return error;
	}

	ret = regmap_read(chip->regmap, AW37004_CHIP_REV, &ldo_chipid);
	if (ret < 0 || ldo_chipid != AW37004_ID) {
		dev_err(dev, "Failed to read CHIP ID:0x%x, ret:%d\n", ldo_chipid,ret);
		ret = -ENODEV;
		return ret;
	} else {
		dev_info(chip->dev, "AW37004 CHIP ID matched!\n");
	}

	ret = regmap_read(chip->regmap, AW37004_CHIP_REV2, &ldo_chipid);
	if (ret < 0 || ldo_chipid != AW37004_ID2) {
		dev_err(dev, "Failed to read CHIP ID2:0x%x, ret:%d\n", ldo_chipid,ret);
		ret = -ENODEV;
		return ret;
	} else {
		dev_info(chip->dev, "AW37004 CHIP ID2 matched!\n");
	}

	for (i = 0; i < 5; i++) {
		ret = regmap_write(chip->regmap, initial_register[i][0], initial_register[i][1]);
		if (ret < 0) {
			dev_err(chip->dev,"Failed to write register: 0x%x, value: 0x%x \n",
				initial_register[i][0], initial_register[i][1]);
		}

		dev_info(chip->dev,"Success to write register: 0x%x, value: 0x%x \n",
			initial_register[i][0], initial_register[i][1]);
	}

	ret = aw37004_regulator_init(chip);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to init regulator(%d)\n", ret);
		return ret;
	}

	aw37004_get_current_limit(chip->rdev[0]);

	dev_info(chip->dev, "aw37004_i2c_probe Exit...\n");

	return ret;
}

static int aw37004_i2c_remove(struct i2c_client *client)
{
	struct aw37004 *chip = i2c_get_clientdata(client);
	struct gpio_desc *desc;
	int ret = 0;

	if (chip->chip_cs_pin > 0) {
		desc = gpio_to_desc(chip->chip_cs_pin);
		ret = gpiod_direction_output_raw(desc, GPIOF_INIT_LOW);
		if (ret) {
			dev_err(chip->dev, "aw37004_i2c_remove cs-pin output %d\n", ret);
			return ret;
		}
	}


	if (chip->chip_vin1_pin > 0) {
		desc = gpio_to_desc(chip->chip_vin1_pin);
		ret = gpiod_direction_output_raw(desc, GPIOF_INIT_LOW);
		if (ret) {
			dev_err(chip->dev, "aw37004_i2c_remove vin1-pin output %d\n", ret);
			return ret;
		}
	}

	return ret;
}

static void aw37004_i2c_shutdown(struct i2c_client *client)
{
	struct aw37004 *chip = i2c_get_clientdata(client);
	unsigned int val = 0;

	if (chip->shutdown_ldo) {
		regmap_read(chip->regmap, AW37004_LDO_EN, &val);
		/* Disable AVDD1 when shutdown to meet device SPEC and avoid current leak */
		regmap_write(chip->regmap, AW37004_LDO_EN, val & ~(1<<2));
		dev_info(chip->dev, "aw37004_i2c_shutdown");
	}
	if (chip) {
		regmap_write(chip->regmap, AW37004_LDO_EN, 0);
		dev_err(chip->dev, "aw37004_i2c_shutdown force disable all LDOs");
	}
}

static const struct i2c_device_id aw37004_i2c_id[] = {
	{"aw37004", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, aw37004_i2c_id);

static struct i2c_driver aw37004_regulator_driver = {
	.driver = {
		.name = "aw37004-regulator",
	},
	.probe = aw37004_i2c_probe,
	.remove = aw37004_i2c_remove,
	.shutdown = aw37004_i2c_shutdown,
	.id_table = aw37004_i2c_id,
};

module_i2c_driver(aw37004_regulator_driver);

MODULE_AUTHOR("LONGC168 <longc168@motorola.com>");
MODULE_DESCRIPTION("AW37004 regulator driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0+
/*
 * dio80151c, Multi-Output Regulators
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
#define DIO80151C_REG_NUM (DIO80151C_SEQ_STATUS-DIO80151C_CHIP_REV+1)

#define DIO80151C_CHIP_REV 0x00
#define DIO80151C_CURRENT_LIMITSEL 0x01
#define DIO80151C_DISCHARGE_RESISTORS 0x02
#define DIO80151C_LDO1_VOUT 0x03
#define DIO80151C_LDO2_VOUT 0x04
#define DIO80151C_LDO3_VOUT 0x05
#define DIO80151C_LDO4_VOUT 0x06
#define DIO80151C_LDO1_LDO2_SEQ 0x0a
#define DIO80151C_LDO3_LDO4_SEQ 0x0b
#define DIO80151C_LDO_EN 0x0e
#define DIO80151C_SEQ_STATUS 0x0f


/* DIO80151C_LDO1_VSEL ~ DIO80151C_LDO4_VSEL =
 * 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 */
#define  DIO80151C_LDO1_VSEL                      DIO80151C_LDO1_VOUT
#define  DIO80151C_LDO2_VSEL                      DIO80151C_LDO2_VOUT
#define  DIO80151C_LDO3_VSEL                      DIO80151C_LDO3_VOUT
#define  DIO80151C_LDO4_VSEL                      DIO80151C_LDO4_VOUT


#define  DIO80151C_VSEL_SHIFT                     0
#define  DIO80151C_VSEL_MASK                      (0xff << 0)

#define  DIO80151C_N_VOLTAGES                     256

#define  DIO80151C_ID                             0x04

static int ldo_chipid = -1;

enum slg51000_regulators {
	DIO80151C_REGULATOR_LDO1 = 0,
	DIO80151C_REGULATOR_LDO2,
	DIO80151C_REGULATOR_LDO3,
	DIO80151C_REGULATOR_LDO4,
	DIO80151C_MAX_REGULATORS,
};

struct dio80151c {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_desc *rdesc[DIO80151C_MAX_REGULATORS];
	struct regulator_dev *rdev[DIO80151C_MAX_REGULATORS];
	int chip_cs_pin;
	int chip_vin1_pin;
	int init_value;
	bool shutdown_ldo;
};

struct dio80151c_evt_sta {
	unsigned int sreg;
};

static const struct dio80151c_evt_sta dio80151c_status_reg = { DIO80151C_LDO_EN };

static const struct regmap_range dio80151c_writeable_ranges[] = {
      /* Do not let useless register writeable */
	regmap_reg_range(DIO80151C_CURRENT_LIMITSEL, DIO80151C_SEQ_STATUS),
};

static const struct regmap_range dio80151c_readable_ranges[] = {
	regmap_reg_range(DIO80151C_CHIP_REV, DIO80151C_SEQ_STATUS),
};

static const struct regmap_range dio80151c_volatile_ranges[] = {
	regmap_reg_range(DIO80151C_CURRENT_LIMITSEL, DIO80151C_SEQ_STATUS),
};

static const struct regmap_access_table dio80151c_writeable_table = {
	.yes_ranges	= dio80151c_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(dio80151c_writeable_ranges),
};

static const struct regmap_access_table dio80151c_readable_table = {
	.yes_ranges	= dio80151c_readable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(dio80151c_readable_ranges),
};

static const struct regmap_access_table dio80151c_volatile_table = {
	.yes_ranges	= dio80151c_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(dio80151c_volatile_ranges),
};

static const struct regmap_config dio80151c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = DIO80151C_SEQ_STATUS,
	.wr_table = &dio80151c_writeable_table,
	.rd_table = &dio80151c_readable_table,
	.volatile_table = &dio80151c_volatile_table,
};

static int dio80151c_get_current_limit(struct regulator_dev *rdev)
{
	struct dio80151c *chip = rdev_get_drvdata(rdev);
	uint8_t reg_dump[DIO80151C_REG_NUM];
	uint8_t reg_idx;
	unsigned int val = 0;

	dev_err(chip->dev, "************ start dump dio80151c register ************\n");
	dev_err(chip->dev, "register name =%s \n",rdev->desc->name);
	dev_err(chip->dev, "register 0x00:      chip version\n");
	dev_err(chip->dev, "register 0x01:      LDO CL\n");
	dev_err(chip->dev, "register 0x03~0x06: LDO1~LDO4 OUT Voltage\n");
	dev_err(chip->dev, "register 0x0e:      Bit[3:0] LDO4~LDO1 EN\n");

	for (reg_idx = 0; reg_idx < DIO80151C_REG_NUM; reg_idx++) {
		regmap_read(chip->regmap, reg_idx, &val);
		reg_dump[reg_idx] = val;
		dev_err(chip->dev, "Reg[0x%02x] = 0x%x", reg_idx, reg_dump[reg_idx]);
	}
	dev_err(chip->dev, "************ end dump dio80151c register ************\n");

	return 0;
}

static int dio80151c_get_status(struct regulator_dev * rdev)
{
	struct dio80151c *chip = rdev_get_drvdata(rdev);
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

	dio80151c_get_current_limit(rdev);

	ret = regmap_read(chip->regmap, dio80151c_status_reg.sreg, &status);
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

static const struct regulator_ops dio80151c_regl_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear,
	.map_voltage = regulator_map_voltage_linear,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_status = dio80151c_get_status,
	.get_current_limit = dio80151c_get_current_limit,
};

static int dio80151c_of_parse_cb(struct device_node *np,
				const struct regulator_desc *desc,
				struct regulator_config *config)
{
	int ena_gpio;

	ena_gpio = of_get_named_gpio(np, "enable-gpios", 0);
	if (gpio_is_valid(ena_gpio))
		config->ena_gpiod = gpio_to_desc(ena_gpio);

	return 0;
}

#define DIO80151C_REGL_DESC(_id, _name, _s_name, _min, _step)       \
	[DIO80151C_REGULATOR_##_id] = {                             \
		.name = #_name,                                    \
		.supply_name = _s_name,                            \
		.id = DIO80151C_REGULATOR_##_id,                    \
		.of_match = of_match_ptr(#_name),                  \
		.of_parse_cb = dio80151c_of_parse_cb,               \
		.ops = &dio80151c_regl_ops,                         \
		.regulators_node = of_match_ptr("regulators"),     \
		.n_voltages = DIO80151C_N_VOLTAGES,                  \
		.min_uV = _min,                                    \
		.uV_step = _step,                                  \
		.linear_min_sel = 0,                               \
		.vsel_mask = DIO80151C_VSEL_MASK,                   \
		.vsel_reg = DIO80151C_##_id##_VSEL,                 \
		.enable_reg = DIO80151C_LDO_EN,       \
		.enable_mask = BIT(DIO80151C_REGULATOR_##_id),      \
		.type = REGULATOR_VOLTAGE,                         \
		.owner = THIS_MODULE,                              \
	}

static struct regulator_desc dio80151c_regls_desc[DIO80151C_MAX_REGULATORS] = {
	DIO80151C_REGL_DESC(LDO1, ldo1, "vin1", 600000, 6000),
	DIO80151C_REGL_DESC(LDO2, ldo2, "vin1", 600000, 6000),
	DIO80151C_REGL_DESC(LDO3, ldo3, "vin2", 1200000, 12500),
	DIO80151C_REGL_DESC(LDO4, ldo4, "vin2", 1200000, 12500),
};

static int dio80151c_regulator_init(struct dio80151c *chip)
{
	struct regulator_config config = { };
	struct regulator_desc *rdesc;
	u8 vsel_range[1];
	int id, ret = 0;
	const unsigned int ldo_regs[DIO80151C_MAX_REGULATORS] = {
		DIO80151C_LDO1_VOUT,
		DIO80151C_LDO2_VOUT,
		DIO80151C_LDO3_VOUT,
		DIO80151C_LDO4_VOUT,
	};

	const unsigned int initial_voltage[DIO80151C_MAX_REGULATORS] = {
		0x64,//LDO1 DVDD 1.2V
		0x64,//LDO2 DVDD 1.2V
		0x80,//LDO3 AVDD 2.8V
		0x80,//LDO4 AVDD 2.8V
	};

	/*Disable all ldo output by default*/
	ret = regmap_write(chip->regmap, DIO80151C_LDO_EN, chip->init_value);
	if (ret < 0) {
		dev_err(chip->dev,
			"Disable all LDO output failed!!!\n");
		return ret;
	}
	/* Enable all ldo discharge by default */
	ret = regmap_write(chip->regmap, DIO80151C_DISCHARGE_RESISTORS, 0x8f);
	if (ret < 0) {
		dev_err(chip->dev,
			"Enable LDO discharge failed!!!\n");
		return ret;
	}
	for (id = 0; id < DIO80151C_MAX_REGULATORS; id++) {
		chip->rdesc[id] = &dio80151c_regls_desc[id];
		rdesc = chip->rdesc[id];
		config.regmap = chip->regmap;
		config.dev = chip->dev;
		config.driver_data = chip;

		ret = regmap_bulk_read(chip->regmap, ldo_regs[id],
				       vsel_range, 1);
		pr_err("dio80151c_regulator_init: LDO%d, default value:0x%x", (id+1), vsel_range[0]);
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
		pr_err("dio80151c_regulator_init: LDO%d, initial value:0x%x", (id+1), initial_voltage[id]);

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

static int dio80151c_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct dio80151c *chip;
	int error, cs_gpio, vin1_gpio, ret, i, value;

	/* Set all register to initial value when probe driver to avoid register value was modified.
	*/
	const unsigned int initial_register[5][2] = {
		{DIO80151C_CURRENT_LIMITSEL, 	0x00},
		{DIO80151C_DISCHARGE_RESISTORS, 	0x00},
		{DIO80151C_LDO1_LDO2_SEQ, 	0x00},
		{DIO80151C_LDO3_LDO4_SEQ, 	0x00},
		{DIO80151C_SEQ_STATUS, 		0x00},
	};
	chip = devm_kzalloc(dev, sizeof(struct dio80151c), GFP_KERNEL);
	if (!chip) {
		dev_err(chip->dev, "dio80151c_i2c_probe Memory error...\n");
		return -ENOMEM;
	}

	dev_info(chip->dev, "dio80151c_i2c_probe Enter...\n");

	cs_gpio = of_get_named_gpio(dev->of_node, "cs-gpios", 0);
	if (cs_gpio > 0) {
		if (!gpio_is_valid(cs_gpio)) {
			dev_err(dev, "Invalid chip select pin\n");
			return -EPERM;
		}

		ret = devm_gpio_request_one(dev, cs_gpio, GPIOF_OUT_INIT_LOW,
					    "dio80151c_cs_pin");
		if (ret) {
			dev_err(dev, "GPIO(%d) request failed(%d)\n",
				cs_gpio, ret);
			return ret;
		}

		chip->chip_cs_pin = cs_gpio;
	}

	dev_info(chip->dev, "dio80151c_i2c_probe cs_gpio:%d...\n", cs_gpio);

	vin1_gpio = of_get_named_gpio(dev->of_node, "vin1-gpios", 0);
	if (vin1_gpio > 0) {
		if (!gpio_is_valid(vin1_gpio)) {
			dev_err(dev, "Invalid vin1 select pin\n");
			return -EPERM;
		}

		ret = devm_gpio_request_one(dev, vin1_gpio, GPIOF_OUT_INIT_HIGH,
					    "dio80151c_vin1_pin");
		if (ret) {
			dev_err(dev, "GPIO(%d) request failed(%d)\n",
				vin1_gpio, ret);
			return ret;
		}

		chip->chip_vin1_pin = vin1_gpio;
	}

	dev_info(chip->dev, "dio80151c_i2c_probe vin1_gpio:%d...\n", vin1_gpio);

	if (of_property_read_u32(dev->of_node, "init-value", &value) < 0) {
		dev_info(chip->dev, "dio80151c_i2c_probe no init_value, use default 0x0\n");
		value = 0x0;
	}
	chip->init_value = value;
	dev_info(chip->dev, "dio80151c_i2c_probe init_value:%d...\n", value);
	chip->shutdown_ldo = of_property_read_bool(dev->of_node, "shutdown-ldo");

	mdelay(10);

	i2c_set_clientdata(client, chip);
	chip->dev = dev;
	chip->regmap = devm_regmap_init_i2c(client, &dio80151c_regmap_config);
	if (IS_ERR(chip->regmap)) {
		error = PTR_ERR(chip->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n",
			error);
		return error;
	}

	ret = regmap_read(chip->regmap, DIO80151C_CHIP_REV, &ldo_chipid);
	if (ret < 0 || ldo_chipid != DIO80151C_ID) {
		dev_err(dev, "Failed to read CHIP ID:0x%x, ret:%d\n", ldo_chipid,ret);
		ret = -ENODEV;
		return ret;
	} else {
		dev_info(chip->dev, "DIO80151C CHIP ID matched!\n");
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

	ret = dio80151c_regulator_init(chip);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to init regulator(%d)\n", ret);
		return ret;
	}

	dio80151c_get_current_limit(chip->rdev[0]);

	dev_info(chip->dev, "dio80151c_i2c_probe Exit...\n");

	return ret;
}

static int dio80151c_i2c_remove(struct i2c_client *client)
{
	struct dio80151c *chip = i2c_get_clientdata(client);
	struct gpio_desc *desc;
	int ret = 0;

	if (chip->chip_cs_pin > 0) {
		desc = gpio_to_desc(chip->chip_cs_pin);
		ret = gpiod_direction_output_raw(desc, GPIOF_INIT_LOW);
		if (ret) {
			dev_err(chip->dev, "dio80151c_i2c_remove cs-pin output %d\n", ret);
			return ret;
		}
	}


	if (chip->chip_vin1_pin > 0) {
		desc = gpio_to_desc(chip->chip_vin1_pin);
		ret = gpiod_direction_output_raw(desc, GPIOF_INIT_LOW);
		if (ret) {
			dev_err(chip->dev, "dio80151c_i2c_remove vin1-pin output %d\n", ret);
			return ret;
		}
	}

	return ret;
}

static void dio80151c_i2c_shutdown(struct i2c_client *client)
{
	struct dio80151c *chip = i2c_get_clientdata(client);
	unsigned int val = 0;

	if (chip->shutdown_ldo) {
		regmap_read(chip->regmap, DIO80151C_LDO_EN, &val);
		/* Disable AVDD1 when shutdown to meet device SPEC and avoid current leak */
		regmap_write(chip->regmap, DIO80151C_LDO_EN, val & ~(1<<2));
		dev_info(chip->dev, "dio80151c_i2c_shutdown");
	}
	if (chip) {
		regmap_write(chip->regmap, DIO80151C_LDO_EN, 0);
		dev_err(chip->dev, "dio80151c_i2c_shutdown force disable all LDOs");
	}
}

static const struct i2c_device_id dio80151c_i2c_id[] = {
	{"dio80151c", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, dio80151c_i2c_id);

static struct i2c_driver dio80151c_regulator_driver = {
	.driver = {
		.name = "dio80151c-regulator",
	},
	.probe = dio80151c_i2c_probe,
	.remove = dio80151c_i2c_remove,
	.shutdown = dio80151c_i2c_shutdown,
	.id_table = dio80151c_i2c_id,
};

module_i2c_driver(dio80151c_regulator_driver);

MODULE_AUTHOR("LONGC168 <longc168@motorola.com>");
MODULE_DESCRIPTION("DIO80151C regulator driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0+
/*
 * sgm38121, Multi-Output Regulators
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
#define SGM38121_REG_NUM (SGM38121_SEQ_STATUS-SGM38121_CHIP_REV+1)

#define SGM38121_CHIP_REV 0x00
//#define SGM38121_CURRENT_LIMITSEL 0x01
#define SGM38121_DISCHARGE_RESISTORS 0x02
#define SGM38121_LDO1_VOUT 0x03
#define SGM38121_LDO2_VOUT 0x04
#define SGM38121_LDO3_VOUT 0x05
#define SGM38121_LDO4_VOUT 0x06
#define SGM38121_LDO1_LDO2_SEQ 0x0a
#define SGM38121_LDO3_LDO4_SEQ 0x0b
#define SGM38121_LDO_EN 		0x0e
#define SGM38121_SEQ_STATUS 	0x0f

#define DVDD_MIN_MV				504
#define DVDD_MAX_MV				1504
#define DVDD_MIN_REG_VAL		0x03
#define DVDD_MAX_REG_VAL		0x7D
#define DVDD_STEP_MV			8

#define AVDD_MIN_MV				1384
#define AVDD_MAX_MV				3424
#define AVDD_MIN_REG_VAL		0x0F
#define AVDD_MAX_REG_VAL		0xFF
#define AVDD_STEP_MV			8
/* SGM38121_LDO1_VSEL ~ SGM38121_LDO4_VSEL =
 * 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
 */
#define  SGM38121_LDO1_VSEL                      SGM38121_LDO1_VOUT
#define  SGM38121_LDO2_VSEL                      SGM38121_LDO2_VOUT
#define  SGM38121_LDO3_VSEL                      SGM38121_LDO3_VOUT
#define  SGM38121_LDO4_VSEL                      SGM38121_LDO4_VOUT

#define  SGM38121_VSEL_SHIFT                     0
#define  SGM38121_VSEL_MASK                      (0xff << 0)

#define  SGM38121_N_VOLTAGES                     256

#define  SGM38121_ID                             0x80

static int ldo_chipid = -1;

enum sgm38121_regulators {
	SGM38121_REGULATOR_LDO1 = 0,
	SGM38121_REGULATOR_LDO2,
	SGM38121_REGULATOR_LDO3,
	SGM38121_REGULATOR_LDO4,
	SGM38121_MAX_REGULATORS,
};

struct sgm38121 {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_desc *rdesc[SGM38121_MAX_REGULATORS];
	struct regulator_dev *rdev[SGM38121_MAX_REGULATORS];
	int chip_cs_pin;
	int chip_vin1_pin;
	int init_value;
	bool shutdown_ldo;
	int ldo_output[SGM38121_MAX_REGULATORS];
};

struct sgm38121_evt_sta {
	unsigned int sreg;
};

static const struct sgm38121_evt_sta sgm38121_status_reg = { SGM38121_LDO_EN };

static const struct regmap_range sgm38121_writeable_ranges[] = {
      /* Do not let useless register writeable */
	regmap_reg_range(SGM38121_DISCHARGE_RESISTORS, SGM38121_SEQ_STATUS),
};

static const struct regmap_range sgm38121_readable_ranges[] = {
	regmap_reg_range(SGM38121_CHIP_REV, SGM38121_SEQ_STATUS),
};

static const struct regmap_range sgm38121_volatile_ranges[] = {
	regmap_reg_range(SGM38121_DISCHARGE_RESISTORS, SGM38121_SEQ_STATUS),
};

static const struct regmap_access_table sgm38121_writeable_table = {
	.yes_ranges	= sgm38121_writeable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(sgm38121_writeable_ranges),
};

static const struct regmap_access_table sgm38121_readable_table = {
	.yes_ranges	= sgm38121_readable_ranges,
	.n_yes_ranges	= ARRAY_SIZE(sgm38121_readable_ranges),
};

static const struct regmap_access_table sgm38121_volatile_table = {
	.yes_ranges	= sgm38121_volatile_ranges,
	.n_yes_ranges	= ARRAY_SIZE(sgm38121_volatile_ranges),
};

static const struct regmap_config sgm38121_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = SGM38121_SEQ_STATUS,
	.wr_table = &sgm38121_writeable_table,
	.rd_table = &sgm38121_readable_table,
	.volatile_table = &sgm38121_volatile_table,
};

static int sgm38121_get_current_limit(struct regulator_dev *rdev)
{
	struct sgm38121 *chip = rdev_get_drvdata(rdev);
	uint8_t reg_dump[SGM38121_REG_NUM];
	uint8_t reg_idx;
	unsigned int val = 0;
	for (reg_idx = 0; reg_idx < SGM38121_REG_NUM; reg_idx++) {
		regmap_read(chip->regmap, reg_idx, &val);
		reg_dump[reg_idx] = val;
		dev_err(chip->dev, "Reg[0x%02x] = 0x%x", reg_idx, reg_dump[reg_idx]);
	}
	dev_err(chip->dev, "************ end dump sgm38121 register ************\n");

	return 0;
}

static int sgm38121_get_status(struct regulator_dev * rdev)
{
	struct sgm38121 *chip = rdev_get_drvdata(rdev);
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

	sgm38121_get_current_limit(rdev);

	ret = regmap_read(chip->regmap, sgm38121_status_reg.sreg, &status);
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
static int sgm38121_output_voltage_set(struct sgm38121 *sgm38121, int channel, int mV)
{
	uint8_t reg = 0;
	uint8_t reg_val = 0;
	int ret = 0;
	switch(channel)
	{
	case SGM38121_REGULATOR_LDO1: //Vout =  (504 + 8 × d) × 1mV
		reg = SGM38121_LDO1_VOUT;
		if (mV < 528) {
			mV = 528;
			reg_val = 0x03;
		} else if (mV > DVDD_MAX_MV) {
			mV = DVDD_MAX_MV;
			reg_val = DVDD_MAX_REG_VAL;
		} else {
			reg_val = (mV - DVDD_MIN_MV) / DVDD_STEP_MV;
		}
		break;
	case SGM38121_REGULATOR_LDO2:
		reg = SGM38121_LDO2_VOUT;
		if (mV < 528) {
			mV = 528;
			reg_val = DVDD_MIN_REG_VAL;
		} else if (mV > DVDD_MAX_MV) {
			mV = DVDD_MAX_MV;
			reg_val = DVDD_MAX_REG_VAL;
		} else {
			reg_val = (mV - DVDD_MIN_MV) / DVDD_STEP_MV;
		}
		break;
	case SGM38121_REGULATOR_LDO3:	//VOUT = (1384 + 8 × d) × 1mV
		reg = SGM38121_LDO3_VOUT;
		if (mV < 1504) {
			mV = 1504;
			reg_val = AVDD_MIN_REG_VAL;
		} else if (mV > AVDD_MAX_MV) {
			mV = AVDD_MAX_MV;
			reg_val = AVDD_MAX_REG_VAL;
		} else {
			reg_val = (mV - AVDD_MIN_MV) / AVDD_STEP_MV;
		}
		break;
	case SGM38121_REGULATOR_LDO4:
		reg = SGM38121_LDO4_VOUT;
		if (mV < 1504) {
			mV = 1504;
			reg_val = AVDD_MIN_REG_VAL;
		} else if (mV > AVDD_MAX_MV) {
			mV = AVDD_MAX_MV;
			reg_val = AVDD_MAX_REG_VAL;
		} else {
			reg_val = (mV - AVDD_MIN_MV) / AVDD_STEP_MV;
		}
		break;
	default:
		break;
	}
	ret = regmap_write(sgm38121->regmap, reg, reg_val);
	dev_info(sgm38121->dev,"set ldo:%d, voltage = %d",channel+ 1, mV);
	return ret;
}

static int sgm38121_output_voltage_get(struct sgm38121 *sgm38121, int channel, int *value)
{
	uint8_t reg = 0;
	int reg_val = 0;
	int ret = 0;
	switch(channel)
	{
	case SGM38121_REGULATOR_LDO1: //Vout =  (504 + 8 × d) × 1mV
		reg = SGM38121_LDO1_VOUT;
		ret = regmap_read(sgm38121->regmap, reg, &reg_val);
		if (reg_val < DVDD_MIN_REG_VAL) reg_val =  DVDD_MIN_REG_VAL;
		else if (reg_val > DVDD_MAX_REG_VAL) reg_val =  DVDD_MAX_REG_VAL;
		*value = reg_val * DVDD_STEP_MV + DVDD_MIN_MV;
		break;
	case SGM38121_REGULATOR_LDO2:
		reg = SGM38121_LDO2_VOUT;
		ret = regmap_read(sgm38121->regmap, reg, &reg_val);
		if (reg_val < DVDD_MIN_REG_VAL) reg_val =  DVDD_MIN_REG_VAL;
		else if (reg_val > DVDD_MAX_REG_VAL) reg_val =  DVDD_MAX_REG_VAL;
		*value = reg_val * DVDD_STEP_MV + DVDD_MIN_MV;
		break;
	case SGM38121_REGULATOR_LDO3:	//VOUT = (1384 + 8 × d) × 1mV
		reg = SGM38121_LDO3_VOUT;
		ret = regmap_read(sgm38121->regmap, reg, &reg_val);
		if (reg_val < AVDD_MIN_REG_VAL) reg_val =  AVDD_MIN_REG_VAL;
		else if (reg_val > AVDD_MAX_REG_VAL) reg_val =  AVDD_MAX_REG_VAL;
		*value = reg_val * AVDD_STEP_MV + AVDD_MIN_MV;
		break;
	case SGM38121_REGULATOR_LDO4:
		reg = SGM38121_LDO4_VOUT;
		ret = regmap_read(sgm38121->regmap, reg, &reg_val);
		if (reg_val < AVDD_MIN_REG_VAL) reg_val =  AVDD_MIN_REG_VAL;
		else if (reg_val > AVDD_MAX_REG_VAL) reg_val =  AVDD_MAX_REG_VAL;
		*value = reg_val * AVDD_STEP_MV + AVDD_MIN_MV;
		break;
	default:
		break;
	}
	dev_info(sgm38121->dev,"get ldo:%d, voltage = %d",channel+ 1, *value);
	return ret;
}
static int sgm38121_regulator_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned int *selector)
{
	struct sgm38121 *chip = rdev_get_drvdata(rdev);
	int ret = 0;
	int index = 0;
	dev_info(chip->dev,"sgm38121_regulaton_set_voltage: name = %s\n", rdev->desc->name);

	if (0 == strcmp(rdev->desc->name,"ldo1")) {
		index = 0;
	} else if (0 == strcmp(rdev->desc->name,"ldo2")) {
		index = 1;
	}else if (0 == strcmp(rdev->desc->name,"ldo3")) {
		index = 2;
	}else if (0 == strcmp(rdev->desc->name,"ldo4")) {
		index = 3;
	} else {
		dev_err(chip->dev,"sgm38121_regulaton_set_voltage: name error.\n");
	}
	ret = sgm38121_output_voltage_set(chip, index, max_uV/1000);
	dev_info(chip->dev,"sgm38121_regulaton_set_voltage: Mv:%d,index:%d\n", max_uV/1000,index);

	return ret;
}

static int sgm38121_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct sgm38121 *chip = rdev_get_drvdata(rdev);
	int ret = 0;
	int index = 0;
	if (0 == strcmp(rdev->desc->name,"ldo1")) {
		index = 0;
	} else if (0 == strcmp(rdev->desc->name,"ldo2")) {
		index = 1;
	}else if (0 == strcmp(rdev->desc->name,"ldo3")) {
		index = 2;
	}else if (0 == strcmp(rdev->desc->name,"ldo4")) {
		index = 3;
	} else {
		dev_err(chip->dev,"sgm38121_regulaton_get_voltage: name error.\n");
	}
	ret = sgm38121_output_voltage_get(chip,index, &chip->ldo_output[index]);
	if (ret < 0)
		return -1;
	dev_info(chip->dev,"sgm38121_regulaton_get_voltage:voltage = %d, name = %s\n",
					chip->ldo_output[index], rdev->desc->name);
	return chip->ldo_output[index] * 1000;
}
static const struct regulator_ops sgm38121_regl_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.list_voltage = regulator_list_voltage_linear,
	.map_voltage = regulator_map_voltage_linear,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_status = sgm38121_get_status,
	.get_current_limit = sgm38121_get_current_limit,
	.set_voltage = sgm38121_regulator_set_voltage,
	.get_voltage = sgm38121_regulator_get_voltage,
};

static int sgm38121_of_parse_cb(struct device_node *np,
				const struct regulator_desc *desc,
				struct regulator_config *config)
{
	int ena_gpio;

	ena_gpio = of_get_named_gpio(np, "enable-gpios", 0);
	if (gpio_is_valid(ena_gpio))
		config->ena_gpiod = gpio_to_desc(ena_gpio);

	return 0;
}

#define SGM38121_REGL_DESC(_id, _name, _s_name, _min, _step)       \
	[SGM38121_REGULATOR_##_id] = {                             \
		.name = #_name,                                    \
		.supply_name = _s_name,                            \
		.id = SGM38121_REGULATOR_##_id,                    \
		.of_match = of_match_ptr(#_name),                  \
		.of_parse_cb = sgm38121_of_parse_cb,               \
		.ops = &sgm38121_regl_ops,                         \
		.regulators_node = of_match_ptr("regulators"),     \
		.n_voltages = SGM38121_N_VOLTAGES,                  \
		.min_uV = _min,                                    \
		.uV_step = _step,                                  \
		.linear_min_sel = 0,                               \
		.vsel_mask = SGM38121_VSEL_MASK,                   \
		.vsel_reg = SGM38121_##_id##_VSEL,                 \
		.enable_reg = SGM38121_LDO_EN,       \
		.enable_mask = BIT(SGM38121_REGULATOR_##_id),      \
		.type = REGULATOR_VOLTAGE,                         \
		.owner = THIS_MODULE,                              \
	}

static struct regulator_desc sgm38121_regls_desc[SGM38121_MAX_REGULATORS] = {
	SGM38121_REGL_DESC(LDO1, ldo1, "vin1", 600000, 6000),
	SGM38121_REGL_DESC(LDO2, ldo2, "vin1", 600000, 6000),
	SGM38121_REGL_DESC(LDO3, ldo3, "vin2", 1200000, 12500),
	SGM38121_REGL_DESC(LDO4, ldo4, "vin2", 1200000, 12500),
};

static int sgm38121_regulator_init(struct sgm38121 *chip)
{
	struct regulator_config config = { };
	struct regulator_desc *rdesc;
	u8 vsel_range[1];
	int id, ret = 0;
	const unsigned int ldo_regs[SGM38121_MAX_REGULATORS] = {
		SGM38121_LDO1_VOUT,
		SGM38121_LDO2_VOUT,
		SGM38121_LDO3_VOUT,
		SGM38121_LDO4_VOUT,
	};
	/*Disable all ldo output by default*/
	ret = regmap_write(chip->regmap, SGM38121_LDO_EN, 0);
	if (ret < 0) {
		dev_err(chip->dev,
			"Disable all LDO output failed!!!\n");
		return ret;
	}
	/* Enable all ldo discharge by default */
	ret = regmap_write(chip->regmap, SGM38121_DISCHARGE_RESISTORS, 0x8f);
	if (ret < 0) {
		dev_err(chip->dev,
			"Enable LDO discharge failed!!!\n");
		return ret;
	}
	for (id = 0; id < SGM38121_MAX_REGULATORS; id++) {
		chip->rdesc[id] = &sgm38121_regls_desc[id];
		rdesc = chip->rdesc[id];
		config.regmap = chip->regmap;
		config.dev = chip->dev;
		config.driver_data = chip;

		ret = regmap_bulk_read(chip->regmap, ldo_regs[id],
				       vsel_range, 1);
		if (ret < 0) {
			dev_err(chip->dev,
				"Failed to read the ldo register\n");
			return ret;
		}
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
static ssize_t registers_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sgm38121 *chip = dev_get_drvdata(dev);
	int reg_idx = 0;
	int val;
	int reg_dump[32];
    dev_err(chip->dev, "registers_show\n");
	for (reg_idx = 0; reg_idx <= SGM38121_SEQ_STATUS; reg_idx++) {
		regmap_read(chip->regmap, reg_idx, &val);
		reg_dump[reg_idx] = val;
		sprintf(buf, "%#.2x=%#.2x\n", reg_idx, val);
	}
	return 0;
}

static ssize_t registers_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sgm38121 *chip = dev_get_drvdata(dev);
	ssize_t ret = 0;
	unsigned int reg;
	unsigned int val;
		dev_err(chip->dev, "registers_store\n");
	if (sscanf(buf, "%x %x", &reg, &val) != 2)
		return -EINVAL;

	if (reg > 0x0F || val > 255)
		return -EINVAL;
	ret = regmap_write(chip->regmap, reg, val);
	if (ret < 0)
		return ret;
	return count;
}

static DEVICE_ATTR_RW(registers);

static struct attribute *sgm38121_sysfs_attributes[] = {
	&dev_attr_registers.attr,
	NULL,
};

static struct attribute_group sgm38121_sysfs_attr_group = {
	.attrs = sgm38121_sysfs_attributes,
};

static int sgm38121_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct sgm38121 *chip;
	//int error, cs_gpio, vin1_gpio, ret, i, value;
	int error, ret;
	int retry = 2;
	chip = devm_kzalloc(dev, sizeof(struct sgm38121), GFP_KERNEL);
	if (!chip) {
		dev_err(chip->dev, "sgm38121_i2c_probe Memory error...\n");
		return -ENOMEM;
	}

	dev_info(chip->dev, "sgm38121_i2c_probe Enter...\n");

	i2c_set_clientdata(client, chip);
	chip->dev = dev;
	chip->regmap = devm_regmap_init_i2c(client, &sgm38121_regmap_config);
	if (IS_ERR(chip->regmap)) {
		error = PTR_ERR(chip->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n",
			error);
		return error;
	}

	do {
		ret = regmap_read(chip->regmap, SGM38121_CHIP_REV, &ldo_chipid);
		if (ldo_chipid == SGM38121_ID) {
			dev_info(chip->dev, "read chip id success!\n");
			break;
		}
		retry--;
	} while (retry > 0);

	if (ret < 0 ) {
		dev_err(dev, "Failed to read CHIP ID:0x%x, ret:%d\n", ldo_chipid,ret);
		ret = -ENODEV;
		return ret;
	} else {
		if (ldo_chipid == SGM38121_ID) {
			dev_info(chip->dev, "SGM38121 CHIP ID matched!\n");
		} else {
			dev_err(dev, "Failed to read other CHIP ID:0x%x, ret:%d\n", ldo_chipid,ret);
			ret = -ENODEV;
			return ret;
		}
	}

	ret = sysfs_create_group(&dev->kobj, &sgm38121_sysfs_attr_group);
	if (ret < 0) {
		dev_err(dev,"sgm38121 error creating sysfs attr files\n");
		return ret;
	}

	ret = sgm38121_regulator_init(chip);
	if (ret < 0) {
		dev_err(chip->dev, "Failed to init regulator(%d)\n", ret);
		return ret;
	}
	regmap_write(chip->regmap, SGM38121_LDO1_VOUT, 0x4B);
	regmap_write(chip->regmap, SGM38121_LDO2_VOUT, 0x57);
	regmap_write(chip->regmap, SGM38121_LDO3_VOUT, 0xB1);
	regmap_write(chip->regmap, SGM38121_LDO4_VOUT, 0xB1);

	sgm38121_get_current_limit(chip->rdev[0]);
	dev_info(chip->dev, "sgm38121_i2c_probe Exit...\n");
	return ret;
}

static int sgm38121_i2c_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

static void sgm38121_i2c_shutdown(struct i2c_client *client)
{
	struct sgm38121 *chip = i2c_get_clientdata(client);
	//unsigned int val = 0;

	if (chip->shutdown_ldo) {
		dev_info(chip->dev, "sgm38121_i2c_shutdown");
	}
	if (chip) {
		regmap_write(chip->regmap, SGM38121_LDO_EN, 0);
		dev_err(chip->dev, "sgm38121_i2c_shutdown force disable all LDOs");
	}
}

static const struct i2c_device_id sgm38121_i2c_id[] = {
	{"sgm38121", 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, sgm38121_i2c_id);

static struct i2c_driver sgm38121_regulator_driver = {
	.driver = {
		.name = "sgm,sgm38121",
	},
	.probe = sgm38121_i2c_probe,
	.remove = sgm38121_i2c_remove,
	.shutdown = sgm38121_i2c_shutdown,
	.id_table = sgm38121_i2c_id,
};

module_i2c_driver(sgm38121_regulator_driver);

MODULE_AUTHOR("LONGC168 <longc168@motorola.com>");
MODULE_DESCRIPTION("SGM38121 regulator driver");
MODULE_LICENSE("GPL");

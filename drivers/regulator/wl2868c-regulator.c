// SPDX-License-Identifier: GPL-2.0
/*
 * regulator driver for will semicondutor WL2868C 7-ldo PMIC
 *
 * Copyright (c) 2023 Huaqin Inc.
 * Author: Lan Degao<landegao@huaqin.com>
 */

#include "wl2868c-regulator.h"

#define  REGULATOR_MAX  7

static int CI = -1;

enum CHIP_NUM {
	WL2868C,
	SGM38120,
	CHIP_NUM_MAX,
};

//static struct class *ldo_debug_class;
static struct wl2868c_regulator *ldo_wl2868c_regulator;

#define VOLTAGE_ROUND_UP(volt, min_uV, step) \
	(((((volt) - (min_uV)) + (step) - 1) / (step)) * (step) + (min_uV))

#define REG_PRINT(fmt, ...) \
	do { \
		switch (CI) { \
		case WL2868C: \
			pr_info("[PMIC-WL2868C]:line:%d " fmt, __LINE__, ##__VA_ARGS__); \
			break; \
		case SGM38120: \
			pr_info("[PMIC-SGM38120]:line:%d " fmt, __LINE__, ##__VA_ARGS__); \
			break; \
		default: \
			pr_info("[PMIC-NOTMATCH]:line:%d " fmt, __LINE__, ##__VA_ARGS__); \
		} \
	} while(0)

#define CHIP(NUM, i2c_address, cid, cid_addr, ldo_en_address, ldo_out, range) \
	[NUM] = { \
		.i2c_addr = i2c_address, \
		.chip_id = cid, \
		.chip_id_addr = cid_addr, \
		.ldo_en_addr = ldo_en_address, \
		.ldo_out_addr = ldo_out, \
		.ldo_range = range, \
	}

#define CHIP_CONFIG(NUM, register_bits, value_bits, is_read_reg, is_write_reg, max_reg) \
	[NUM] = { \
		.reg_bits			= register_bits, \
		.val_bits			= value_bits, \
		.readable_reg		= is_read_reg, \
		.writeable_reg		= is_write_reg, \
		.max_register		= max_reg, \
		.cache_type			= REGCACHE_RBTREE, \
	}

#define VOLT_RANGE(ldo_chan, min, max, step) \
	[ldo_chan] = { \
		.min_uV = min, \
		.max_uV = max, \
		.uV_step = step, \
	}

#define SGM38120_LDO1_2_MIN_UV 504000

static struct ldo_volt_range wl_range[REGULATOR_MAX] = {
	VOLT_RANGE(0, 496000,  1512000, 8000),
	VOLT_RANGE(1, 496000,  1512000, 8000),
	VOLT_RANGE(2, 1504000, 3544000, 8000),
	VOLT_RANGE(3, 1504000, 3544000, 8000),
	VOLT_RANGE(4, 1504000, 3544000, 8000),
	VOLT_RANGE(5, 1504000, 3544000, 8000),
	VOLT_RANGE(6, 1504000, 3544000, 8000),
};

static struct ldo_volt_range sgm_range[REGULATOR_MAX] = {
	VOLT_RANGE(0, 528000,  1504000, 8000),
	VOLT_RANGE(1, 528000,  1504000, 8000),
	VOLT_RANGE(2, 1504000, 3544000, 8000),
	VOLT_RANGE(3, 1504000, 3544000, 8000),
	VOLT_RANGE(4, 1504000, 3544000, 8000),
	VOLT_RANGE(5, 1504000, 3544000, 8000),
	VOLT_RANGE(6, 1504000, 3544000, 8000),
};

static struct chip_info pmic_chip[CHIP_NUM_MAX] = {
	/*   chip_name ,i2c_addr,chip_id,chip_id_addr,ldo_en_addr,ldo_out_addr,ldo_range */
	CHIP(WL2868C,  0x2F, 0x82, 0x00, 0x0E, 0x03, wl_range),
	CHIP(SGM38120, 0x35, 0xD9, 0x00, 0x03, 0x04, sgm_range),
};

static bool is_read_wl2868c_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0x0F:
		return false;
	default:
		return true;
	}
}

static bool is_write_wl2868c_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0x02 ... 0x0E:
		return true;
	default:
		return false;
	}
}

static bool is_read_sgm38120_reg(struct device *dev, unsigned int reg)
{
	return true;
}

static bool is_write_sgm38120_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0x02 ... 0x0A:
		return true;
	default:
		return false;
	}
}

static struct regmap_config  chip_regmap_config[CHIP_NUM_MAX] = {
	CHIP_CONFIG(WL2868C, 8, 8, is_read_wl2868c_reg, is_write_wl2868c_reg, 0x25),
	CHIP_CONFIG(SGM38120, 8, 8, is_read_sgm38120_reg, is_write_sgm38120_reg, 0x1E),
};

static int  wl2868c_regulator_enable(struct regulator_dev *rdev)
{
	int ret = 0;
	unsigned int current_bit, enable_bit;
	struct chip_info *chip;
	struct regmap *wl2868c_regmap;
	struct wl2868c_regulator *wl2868c_reg = rdev_get_drvdata(rdev);
	if (!wl2868c_reg) {
		REG_PRINT("get wl286c NULL pointer");
		return -EINVAL;
	}
	wl2868c_regmap = wl2868c_reg->regmap;
	chip = wl2868c_reg->chip_info;
	ret = regmap_read(wl2868c_regmap, chip->ldo_en_addr, &current_bit);
	if (ret < 0) {
		REG_PRINT("read ldo en reg failed");
		return -EIO;
	}

	enable_bit = 1 << (wl2868c_reg->channel_num);
	current_bit |= enable_bit;
	current_bit |= 0x80;
	REG_PRINT("enable ldo_en_addr 0x%x current_bit:%d", chip->ldo_en_addr, current_bit);
	ret = regmap_write(wl2868c_regmap, chip->ldo_en_addr, current_bit);
	if (ret < 0) {
		REG_PRINT("write ldo enable address failed");
		return -EIO;
	}
	REG_PRINT("Enable %s", wl2868c_reg->regulator_name);
	return ret;
}

static int  wl2868c_regulator_disable(struct regulator_dev *rdev)
{
	int ret = 0;
	unsigned int disable_bit, current_bit;
	struct chip_info *chip;
	struct regmap *wl2868c_regmap;
	struct wl2868c_regulator *wl2868c_reg = rdev_get_drvdata(rdev);
	if (!wl2868c_reg) {
		REG_PRINT("get NULL pointer");
		return -EINVAL;
	}
	wl2868c_regmap = wl2868c_reg->regmap;
	chip = wl2868c_reg->chip_info;
	ret = regmap_read(wl2868c_regmap, chip->ldo_en_addr, &current_bit);
	if (ret < 0) {
		REG_PRINT("read ldo en reg failed");
		return -EIO;
	}

	disable_bit = ~(1 << wl2868c_reg->channel_num);
	current_bit &= disable_bit;
	REG_PRINT("disable ldo_en_addr 0x%x current_bit:%d", chip->ldo_en_addr, current_bit);
	ret = regmap_write(wl2868c_regmap, chip->ldo_en_addr, current_bit);
	if (ret < 0) {
		REG_PRINT("write ldo enable address failed");
		return -EIO;
	}
	REG_PRINT("disable %s", wl2868c_reg->regulator_name);
	return ret;
}

static int  wl2868c_regulator_is_enabled(struct regulator_dev *rdev)
{
	int ret = 0;
	unsigned int reg_val;
	unsigned char enable_bit;
	struct chip_info *chip = NULL;
	struct regmap *wl2868c_regmap;
	struct wl2868c_regulator *wl2868c_reg = rdev_get_drvdata(rdev);
	if (!wl2868c_reg) {
		REG_PRINT("get wl286c NULL pointer");
		return -EINVAL;
	}
	wl2868c_regmap = wl2868c_reg->regmap;
	chip = wl2868c_reg->chip_info;
	enable_bit = 1 << (wl2868c_reg->channel_num);
	ret = regmap_read(wl2868c_regmap, chip->ldo_en_addr, &reg_val);
	if (ret < 0) {
		REG_PRINT("read ldo enable address failed");
		return -EIO;
	}
	return !!(reg_val & enable_bit);
}

static int  wl2868c_regulator_get_voltage(struct regulator_dev *rdev)
{
	int ret = 0;
	unsigned int volt, reg_addr, uV;
	struct chip_info *chip;
	struct ldo_volt_range *range;
	struct regmap *wl2868c_regmap = NULL;
	struct wl2868c_regulator *wl2868c_reg = rdev_get_drvdata(rdev);
	if (!wl2868c_reg) {
		REG_PRINT("get NULL pointer");
		return -EINVAL;
	}
	wl2868c_regmap = wl2868c_reg->regmap;
	chip = wl2868c_reg->chip_info;
	reg_addr = chip->ldo_out_addr + wl2868c_reg->channel_num;
	range = &chip->ldo_range[wl2868c_reg->channel_num];
	ret = regmap_read(wl2868c_regmap, reg_addr, &volt);
	if (ret < 0)
		REG_PRINT("read voltage failed");

	switch (wl2868c_reg->chip_index) {
	case WL2868C:
		switch (wl2868c_reg->channel_num){
		case 0 ... 1 :
			uV = (range->min_uV) + (volt & 0x7F) * (range->uV_step);
			break;
		case 2 ... 6 :
			uV = range->min_uV + (volt & 0xFF) * range->uV_step;
			break;
		default:
			return -EINVAL;
		}
	break;
	case SGM38120:
		switch (wl2868c_reg->channel_num) {
		case 0 ... 1 :
			uV = SGM38120_LDO1_2_MIN_UV + (volt & 0x7F) * (range->uV_step);
			break;
		case 2 ... 6 :
			uV = range->min_uV + (volt & 0xFF) * range->uV_step;
			break;
		default:
			return -EINVAL;
		}
	}
	REG_PRINT("%s get voltage: %d uV , bit %x", wl2868c_reg->regulator_name, uV, volt);
	return uV;
}

static int  wl2868c_regulator_set_voltage(struct regulator_dev *rdev,
		int min_uV, int max_uV,
		unsigned int *selector)
{
	int ret = 0;
	unsigned int reg_val = 0;
	unsigned int volt_reg;
	unsigned int round_uV;
	struct chip_info *chip;
	struct regmap *wl2868c_regmap;
	struct ldo_volt_range *range;
	struct wl2868c_regulator *wl2868c_reg = rdev_get_drvdata(rdev);
	if (!wl2868c_reg) {
		REG_PRINT("get NULL pointer");
		return -EINVAL;
	}
	wl2868c_regmap = wl2868c_reg->regmap;
	chip = wl2868c_reg->chip_info;
	range = &chip->ldo_range[wl2868c_reg->channel_num];
	volt_reg = chip->ldo_out_addr + wl2868c_reg->channel_num;
	round_uV = VOLTAGE_ROUND_UP(min_uV, range->min_uV, range->uV_step);

	if (round_uV < range->min_uV) {
		round_uV = range->min_uV;
	}
	if (round_uV > range->max_uV) {
		round_uV = range->max_uV;
	}

	switch (wl2868c_reg->chip_index) {
	case WL2868C:
		switch (wl2868c_reg->channel_num){
		case 0 ... 6 :
			reg_val = (round_uV - range->min_uV) / range->uV_step;
			break;
		default:
			return -EINVAL;
		}
	break;
	case SGM38120:
		switch (wl2868c_reg->channel_num){
		case 0 ... 1 :
			reg_val = (round_uV - 504000) / range->uV_step;
			break;
		case 2 ... 6 :
			reg_val = (round_uV - range->min_uV) / range->uV_step;
			break;
		default:
			return -EINVAL;
		}
	}

	ret = regmap_write(wl2868c_regmap, volt_reg, reg_val);
	if (ret < 0) {
		REG_PRINT("%s set voltage failed", wl2868c_reg->regulator_name);
		return -EIO;
	}
	REG_PRINT("%s set voltage: %d uV , bit %x", wl2868c_reg->regulator_name, round_uV, reg_val);
	return ret;
}

/* operatin function set of ldo in PMIC wl2868c */
static struct regulator_ops  wl2868c_regulator_ops = {
	.is_enabled		= wl2868c_regulator_is_enabled,
	.enable			= wl2868c_regulator_enable,
	.disable		= wl2868c_regulator_disable,
	.get_voltage	= wl2868c_regulator_get_voltage,
	.set_voltage	= wl2868c_regulator_set_voltage,
};

static int wl2868c_register_ldo_regulator(struct wl2868c_regulator *wl2868c_reg, struct regmap *wl2868c_regmap)
{
	int ret = 0;
	struct regulator_config reg_config = {};
	struct regulator_init_data *init_data = NULL;
	struct device_node *reg_node= wl2868c_reg->of_node;
	struct device *dev = wl2868c_reg->dev;
	struct chip_info *chip = wl2868c_reg->chip_info;
	struct ldo_volt_range *range = NULL;

	ret = of_property_read_string(reg_node, "regulator-name", &wl2868c_reg->regulator_name);
	ret = of_property_read_u32(reg_node, "ldo-channel", &wl2868c_reg->channel_num);
	if (0 != ret) {
		REG_PRINT("ldo esential infomation lost");
		return -EINVAL;
	}
	init_data = of_get_regulator_init_data(dev, reg_node, &wl2868c_reg->rdesc);
	if ( !init_data) {
		REG_PRINT("get ldo init data failed");
		return -ENODATA;
	}
	range = &chip->ldo_range[wl2868c_reg->channel_num];
	init_data->constraints.min_uV = range->min_uV;
	init_data->constraints.max_uV = range->max_uV;
	reg_config.dev = dev;
	reg_config.init_data = init_data;
	reg_config.driver_data = wl2868c_reg;
	reg_config.of_node = reg_node;
	reg_config.regmap = wl2868c_regmap;

	wl2868c_reg->rdesc.type = REGULATOR_VOLTAGE;
	wl2868c_reg->rdesc.owner = THIS_MODULE;
	wl2868c_reg->rdesc.name = "WL2868C";
	wl2868c_reg->rdesc.ops = &wl2868c_regulator_ops;
	wl2868c_reg->rdesc.n_voltages = (range->max_uV - range->min_uV) / range->uV_step + 2;

	wl2868c_reg->rdev = devm_regulator_register(dev, &wl2868c_reg->rdesc,
			&reg_config);
	if (IS_ERR(wl2868c_reg->rdev)) {
		REG_PRINT("%s: failed to register regulator\n",
				wl2868c_reg->rdesc.name);
		return -EINVAL;
	}
	REG_PRINT("%s: regulator registered\n", wl2868c_reg->rdesc.name);
	return ret;
}

static int wl2868c_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int i;
	int ret_ldo7 = 0;
	int ret = 0;
	struct pinctrl *pinctrl;
	struct device *dev = NULL;
	struct device_node *child = NULL;
	struct device_node *node = NULL;
	struct wl2868c_regulator *wl2868c_reg = NULL;
	struct regmap *wl2868c_regmap;
	const char *pinctrl_name;
	const unsigned int max_try_time = 2;
	int try_time = 0;
	int chip_id;

	if (!client) {
		pr_info("pmic i2c client is NULL");
		return -EINVAL;
	}

	dev = &client->dev;
	if(!dev->of_node) {
		REG_PRINT("dev->of_node is NULL");
		return -EINVAL;
	}

	node = dev->of_node;
	of_property_read_string(node, "pinctrl-names", &pinctrl_name);
	if (pinctrl_name) {
		pinctrl = devm_pinctrl_get_select(dev, pinctrl_name);
		if (IS_ERR(pinctrl)) {
			REG_PRINT("Couldn't select %s pinctrl rc=%ld",
				pinctrl_name, PTR_ERR(pinctrl));
		} else {
			REG_PRINT("pinctrl probe success");
		}
	} else {
		REG_PRINT("no pinctrl configuration");
	}

	for (;try_time < max_try_time; try_time++) {
		for (i = 0; i < CHIP_NUM_MAX; i++) {
			client->addr = pmic_chip[i].i2c_addr;
			chip_id = i2c_smbus_read_byte_data(client, (pmic_chip[i].chip_id_addr) & 0xFF);

			REG_PRINT("I2C result=%d", chip_id);
			if(chip_id < 0) {
				REG_PRINT("Failed to read chip ID,  chip_addr=0x%x, addr=0x%x",
						pmic_chip[i].i2c_addr, (pmic_chip[i].chip_id_addr) & 0xFF);
				continue;
			}

			if (pmic_chip[i].chip_id == chip_id) {
				CI = i;
				REG_PRINT("chip %d identified", i);
				break;
			}else{
				CI = -1;
			}
		}

		if (CI >= 0 && CI < ARRAY_SIZE(pmic_chip)) {
			break;
		}
		REG_PRINT("No LDO found, try (%d/%d)", try_time, max_try_time);
		msleep(1);
	}

	if(CI == -1) {
		REG_PRINT("LDO not found");
		return -EINVAL;
	}

	if(CI < 0 || CI >= ARRAY_SIZE(pmic_chip)) {
		REG_PRINT("Invalid chip index");
		return -EINVAL;
	}

	wl2868c_regmap = devm_regmap_init_i2c(client, &chip_regmap_config[CI]);
	if (!wl2868c_regmap) {
		REG_PRINT("chip regmap init failed");
		return -EIO;
	}

	if (CI == 0) {
		ret_ldo7 = regmap_write(wl2868c_regmap, 0x09, 0x25);//just for wl2868c
		if (ret_ldo7 < 0) {
			REG_PRINT("write 7ch-ldo enable volt failed");
			return -EIO;
		}
	}

	REG_PRINT("i2c probe success");

	for_each_available_child_of_node (dev->of_node, child) {
		if(child == NULL) {
			REG_PRINT("child node is NULL");
			return -EINVAL;
		}

		wl2868c_reg = devm_kzalloc(dev, sizeof(struct wl2868c_regulator), GFP_KERNEL);

		if(wl2868c_reg == NULL){
			REG_PRINT("failed to allocate memory for wl2868c_reg");
			return -ENOMEM;
		}
		wl2868c_reg->of_node = child;
		wl2868c_reg->dev = dev;
		wl2868c_reg->regmap = wl2868c_regmap;
		wl2868c_reg->chip_info = &pmic_chip[CI];
		wl2868c_reg->chip_index = CI;
		ret = wl2868c_register_ldo_regulator(wl2868c_reg, wl2868c_regmap);
		if (ret < 0) {
			REG_PRINT("register regulator failed");
			return ret;
		}
	}

	ldo_wl2868c_regulator = wl2868c_reg;

	REG_PRINT("all regulator register success");
	return 0;
}

static int wl2868c_i2c_remove(struct i2c_client *client)
{
	if (ldo_wl2868c_regulator)
	{
		ldo_wl2868c_regulator = NULL;
	}

	i2c_unregister_device(client);
	return 0;
}

static const struct of_device_id wl2868c_i2c_of_match[] = {
	{ .compatible = "will,wl2868c", },
	{},
};
MODULE_DEVICE_TABLE(of, wl2868c_i2c_of_match);

static struct i2c_driver wl2868c_i2c_driver = {
	.probe = wl2868c_i2c_probe,
	.remove = wl2868c_i2c_remove,
	.driver = {
		.name = "WL2868C_I2C",
		.of_match_table = wl2868c_i2c_of_match,
		.owner = THIS_MODULE
	},
};
module_i2c_driver(wl2868c_i2c_driver);

MODULE_AUTHOR("Zhang Pengfei <hq161@motorola.com>");
MODULE_DESCRIPTION("7-ldo PMIC power supply for camera and other usage");
MODULE_LICENSE("GPL v2");

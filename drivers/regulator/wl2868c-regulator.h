#ifndef __REGULATOR_WL2868C_H_
#define __REGULATOR_WL2868C_H_

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>
#include <linux/regmap.h>

#define LDO_ERROR			(-1)
#define LDO_SUCCESS			(0)
#define LDO_ENABLE			(0x01)
#define LDO_DISABLE			(0x00)
#define AW_ERROR_LOOP			(3)

struct ldo_volt_range{
	unsigned int min_uV;
	unsigned int max_uV;
	unsigned int uV_step;
};

struct chip_info {
	unsigned int i2c_addr;
	unsigned int chip_id;
	unsigned int chip_id_addr;
	unsigned int ldo_en_addr;
	unsigned int ldo_out_addr;
	struct ldo_volt_range *ldo_range;
};

struct wl2868c_regulator{
	struct regulator_desc rdesc;
	struct regulator_dev  *rdev;
	unsigned int channel_num;
	struct device *dev;
	struct device_node *of_node;
	const char *regulator_name;
	struct regmap *regmap;
	struct chip_info *chip_info;
	unsigned int chip_index;
};

#endif
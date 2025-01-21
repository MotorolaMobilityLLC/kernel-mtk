// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */
#include <linux/bits.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include "vtskin_temp.h"

struct __thermal_cooling_bind_param {
	struct device_node *cooling_device;
	unsigned long min;
	unsigned long max;
};

struct __thermal_bind_params {
	struct __thermal_cooling_bind_param *tcbp;
	unsigned int count;
	unsigned int trip_id;
	unsigned int usage;
};

struct __thermal_zone {
	int passive_delay;
	int polling_delay;
	int slope;
	int offset;

	/* trip data */
	int ntrips;
	struct thermal_trip *trips;

	/* cooling binding data */
	int num_tbps;
	struct __thermal_bind_params *tbps;

	/* sensor interface */
	void *sensor_data;
	const struct thermal_zone_of_device_ops *ops;
};

static int __vtskin_get_temp(void *data, int *temp)
{
	struct vtskin_temp_tz *skin_tz = (struct vtskin_temp_tz *)data;
	struct vtskin_data *skin_data = skin_tz->skin_data;
	struct vtskin_tz_param *skin_param = skin_data->params;
	struct thermal_zone_device *tzd;
	struct __thermal_zone *tz;
	long long vtskin = 0, coef;
	int tz_temp, i, ret;
	char *sensor_name;

	if (skin_param[skin_tz->id].ref_num == 0) {
		*temp = THERMAL_TEMP_INVALID;
		return 0;
	}

	for (i = 0; i < skin_param[skin_tz->id].ref_num; i++) {
		sensor_name = skin_param[skin_tz->id].vtskin_ref[i].sensor_name;
		if (!sensor_name) {
			dev_err(skin_data->dev, "get sensor name fail %d\n", i);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}

		tzd = thermal_zone_get_zone_by_name(sensor_name);
		if (IS_ERR_OR_NULL(tzd) || !tzd->ops->get_temp) {
			dev_err(skin_data->dev, "get %s temp fail\n", sensor_name);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}

		if (strncmp(tzd->type, "vtskin", strlen("vtskin")) == 0) {
			tz = tzd->devdata;
			if (tz == NULL) {
				dev_err(skin_data->dev, "vtskin tz is NULL\n");
				return -EINVAL;
			}
			ret = __vtskin_get_temp(tz->sensor_data, &tz_temp);
		} else {
			ret = tzd->ops->get_temp(tzd, &tz_temp);
		}

		if (ret < 0) {
			dev_err(skin_data->dev, "%s get_temp fail %d\n", sensor_name, ret);
			*temp = THERMAL_TEMP_INVALID;
			return -EINVAL;
		}

		if (skin_param[skin_tz->id].operation == OP_MAX) {
			if (i == 0)
				*temp = THERMAL_TEMP_INVALID;

			if (tz_temp > *temp)
				*temp = tz_temp;

		} else if (skin_param[skin_tz->id].operation == OP_COEF) {
			coef = skin_param[skin_tz->id].vtskin_ref[i].sensor_coef;
			vtskin += tz_temp * coef;
			if (i == skin_param[skin_tz->id].ref_num - 1)
				*temp = (int)(vtskin / 100000000);
		}
	}

	return 0;
}

static int vtskin_get_temp(void *data, int *temp)
{
	struct vtskin_temp_tz *skin_tz = (struct vtskin_temp_tz *)data;
	struct vtskin_data *skin_data = skin_tz->skin_data;
	int ret;

	mutex_lock(&skin_data->lock);
	ret = __vtskin_get_temp(data, temp);
	mutex_unlock(&skin_data->lock);

	return ret;
}

static const struct thermal_zone_of_device_ops vtskin_ops = {
	.get_temp = vtskin_get_temp,
};

static int vtskin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct vtskin_temp_tz *skin_tz;
	struct vtskin_data *skin_data;
	struct thermal_zone_device *tzdev;
	int i, ret;

	if (!pdev->dev.of_node) {
		dev_err(dev, "Only DT based supported\n");
		return -ENODEV;
	}

	skin_data = (struct vtskin_data *)of_device_get_match_data(dev);
	if (!skin_data)	{
		dev_err(dev, "Error: Failed to get lvts platform data\n");
		return -ENODATA;
	}

	skin_data->dev = dev;
	platform_set_drvdata(pdev, skin_data);

	mutex_init(&skin_data->lock);

	for (i = 0; i < skin_data->num_sensor; i++) {
		skin_tz = devm_kzalloc(dev, sizeof(*skin_tz), GFP_KERNEL);
		if (!skin_tz)
			return -ENOMEM;

		skin_tz->id = i;
		skin_tz->skin_data = skin_data;

		tzdev = devm_thermal_zone_of_sensor_register(dev, skin_tz->id,
				skin_tz, &vtskin_ops);

		if (IS_ERR(tzdev)) {
			ret = PTR_ERR(tzdev);
			dev_err(dev,
				"Error: Failed to register skin_tz.id %d, ret = %d\n",
				skin_tz->id, ret);
			return ret;
		}

		ret = snprintf(skin_data->params[i].tz_name, THERMAL_NAME_LENGTH, tzdev->type);
		if (ret < 0)
			dev_notice(dev, "copy tz_name fail %s\n", tzdev->type);
	}

	plat_vtskin_info = skin_data;

	return 0;
}

enum mt6855_vtskin_sensor_enum {
	mt6855_VTSKIN_MAX,
	mt6855_VTSKIN_1,
	mt6855_VTSKIN_2,
	mt6855_VTSKIN_3,
	mt6855_VTSKIN_4,
	mt6855_VTSKIN_5,
	mt6855_VTSKIN_6,
	mt6855_VTSKIN_NUM,
};

struct vtskin_tz_param mt6855_vtskin_params[] = {
	[mt6855_VTSKIN_MAX] = {
		.ref_num = 0,
		.operation = OP_MAX,
	},
	[mt6855_VTSKIN_1] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[mt6855_VTSKIN_2] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[mt6855_VTSKIN_3] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[mt6855_VTSKIN_4] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[mt6855_VTSKIN_5] = {
		.ref_num = 0,
		.operation = OP_COEF,
	},
	[mt6855_VTSKIN_6] = {
		.ref_num = 0,
		.operation = OP_COEF,
	}
};

static struct vtskin_data mt6855_vtskin_data = {
	.num_sensor = mt6855_VTSKIN_NUM,
	.params = mt6855_vtskin_params,
};


static const struct of_device_id vtskin_of_match[] = {
	{
		.compatible = "mediatek,mt6855-virtual-tskin",
		.data = (void *)&mt6855_vtskin_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, vtskin_of_match);

static struct platform_driver vtskin_driver = {
	.probe = vtskin_probe,
	.driver = {
		.name = "mtk-virtual-tskin",
		.of_match_table = vtskin_of_match,
	},
};

module_platform_driver(vtskin_driver);

MODULE_AUTHOR("Samuel Hsieh <samuel.hsieh@mediatek.com>");
MODULE_DESCRIPTION("Mediatek on virtual tskin driver");
MODULE_LICENSE("GPL v2");

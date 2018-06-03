/*
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * MTK Regulator Driver Interface.
 */

#ifndef __LINUX_REGULATOR_MTK_REGULATOR_CLASS_H_
#define __LINUX_REGULATOR_MTK_REGULATOR_CLASS_H_

#include <linux/kernel.h>
#include <linux/device.h>

struct mtk_simple_regulator_device {
	struct device dev;
};

extern struct mtk_simple_regulator_device *
mtk_simple_regulator_device_register(const char *name, struct device *parent,
	void *drvdata);

extern void mtk_simple_regulator_device_unregister(
	struct mtk_simple_regulator_device *mreg_dev);

extern struct mtk_simple_regulator_device *mtk_simple_regulator_get_dev_by_name(
	const char *name);

#define to_mreg_device(obj) \
	container_of(obj, struct mtk_simple_regulator_device, dev)

#endif /* __LINUX_REGULATOR_MTK_REGULATOR_CLASS_H_ */

/*
 * mtk_regulator.h -- MTK Regulator driver support.
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_REGULATOR_MTK_REGULATOR_H_
#define __LINUX_REGULATOR_MTK_REGULATOR_H_

#include <linux/regulator/consumer.h>
#include <linux/regulator/mediatek/mtk_regulator_class.h>

struct mtk_simple_regulator_adv_ops;
struct mtk_regulator {
	struct regulator *consumer;
	struct mtk_simple_regulator_adv_ops *mreg_adv_ops;
};

enum mtk_simple_regulator_property {
	MREG_PROP_SET_RAMP_DELAY = 0,
};

union mtk_simple_regulator_propval {
	int32_t intval;
	uint32_t uintval;
	const char *strval;
};

struct mtk_simple_regulator_adv_ops {
	int (*set_property)(struct mtk_regulator *mreg,
		enum mtk_simple_regulator_property prop,
		union mtk_simple_regulator_propval *val);
	int (*get_property)(struct mtk_regulator *mreg,
		enum mtk_simple_regulator_property prop,
		union mtk_simple_regulator_propval *val);
};

extern int mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg);
extern int mtk_regulator_get_exclusive(struct device *dev, const char *id,
	struct mtk_regulator *mreg);
extern int devm_mtk_regulator_get(struct device *dev, const char *id,
	struct mtk_regulator *mreg);
extern void mtk_regulator_put(struct mtk_regulator *mreg);
extern void devm_mtk_regulator_put(struct mtk_regulator *mreg);

extern int mtk_regulator_enable(struct mtk_regulator *mreg, bool enable);
extern int mtk_regulator_force_disable(struct mtk_regulator *mreg);
extern int mtk_regulator_is_enabled(struct mtk_regulator *mreg);

extern int mtk_regulator_set_voltage(struct mtk_regulator *mreg,
					int min_uv, int max_uv);
extern int mtk_regulator_get_voltage(struct mtk_regulator *mreg);
extern int mtk_regulator_set_current_limit(struct mtk_regulator *mreg,
					     int min_uA, int max_uA);
extern int mtk_regulator_get_current_limit(struct mtk_regulator *mreg);

#endif /* __LINUX_REGULATOR_MTK_REGULATOR_H_ */

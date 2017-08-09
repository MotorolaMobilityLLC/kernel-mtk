/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _REGULATOR_CODEGEN_H_
#define _REGULATOR_CODEGEN_H_

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <mach/upmu_hw.h>
#include <mach/upmu_sw.h>

/*****************************************************************************
 * PMIC extern function
 ******************************************************************************/
extern int mtk_regulator_enable(struct regulator_dev *rdev);
extern int mtk_regulator_disable(struct regulator_dev *rdev);
extern int mtk_regulator_is_enabled(struct regulator_dev *rdev);
extern int mtk_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector);
extern int mtk_regulator_get_voltage_sel(struct regulator_dev *rdev);
extern int mtk_regulator_list_voltage(struct regulator_dev *rdev, unsigned selector);

/* -------Code Gen Start-------*/
/* Non regular voltage regulator */
#define NON_REGULAR_VOLTAGE_REGULATOR_GEN(_name, _type, array, mode, use, en, vol)	\
{	\
	.desc = {	\
		.name = #_name,	\
		.n_voltages = (sizeof(array)/sizeof(array[0])),	\
		.ops = &pmic_##_type##_##_name##_ops,	\
		.type = REGULATOR_VOLTAGE,	\
	},	\
	.init_data = {	\
		.constraints = {	\
			.valid_ops_mask = (mode),	\
		},	\
	},	\
	.pvoltages = (void *)(array),	\
	.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
	.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
	.en_reg = (PMU_FLAGS_LIST_ENUM)(en),	\
	.vol_reg = (PMU_FLAGS_LIST_ENUM)(vol),	\
	.isUsedable = (use),	\
}

/* Regular voltage regulator */
#define REGULAR_VOLTAGE_REGULATOR_GEN(_name, _type, min, max, step, min_sel, mode, use, en, vol)	\
{	\
	.desc = {	\
		.name = #_name,	\
		.n_voltages = ((max) - (min)) / (step) + 1,	\
		.ops = &pmic_##_type##_##_name##_ops,	\
		.type = REGULATOR_VOLTAGE,	\
		.min_uV = (min),	\
		.uV_step = (step),	\
		.linear_min_sel = min_sel,	\
	},	\
	.init_data = {	\
		.constraints = {	\
			.valid_ops_mask = (mode),	\
		},	\
	},	\
	.en_att = __ATTR(LDO_##_name##_STATUS, 0664, show_LDO_STATUS, store_LDO_STATUS),	\
	.voltage_att = __ATTR(LDO_##_name##_VOLTAGE, 0664, show_LDO_VOLTAGE, store_LDO_VOLTAGE),	\
	.en_reg = (PMU_FLAGS_LIST_ENUM)(en),	\
	.vol_reg = (PMU_FLAGS_LIST_ENUM)(vol),	\
	.isUsedable = (use),	\
}

/* Fixed voltage regulator */
#define FIXED_REGULAR_VOLTAGE_REGULATOR_GEN(_name, _type, mode, use, en, vol)	\
{	\
	.desc = {	\
		.name = #_name,	\
		.n_voltages = 1,	\
		.ops = &pmic_##_type##_##_name##_ops,	\
		.type = REGULATOR_VOLTAGE,	\
	},	\
	.init_data = {	\
		.constraints = {	\
			.valid_ops_mask = (mode),	\
		},	\
	},	\
	.en_reg = (PMU_FLAGS_LIST_ENUM)(en),	\
	.vol_reg = (PMU_FLAGS_LIST_ENUM)(vol),	\
	.isUsedable = (use),	\
}



#endif				/* _REGULATOR_CODEGEN_H_ */

/*
 *  Driver for Richtek RT5738 Regulator
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/regulator/machine.h>
#include <linux/regulator/mtk_regulator_core.h>
#include "rt5738.h"

struct rt5738_regulator_info {
	struct  mtk_simple_regulator_desc *mreg_desc;
	unsigned char mode_reg;
	unsigned char mode_bit;
	unsigned char ramp_up_reg;
	unsigned char ramp_down_reg;
	unsigned char ramp_mask;
	unsigned char ramp_shift;
};


#define rt5738_buck0_vol_reg	RT5738_REG_VSEL0
#define rt5738_buck0_vol_mask	(0xff)
#define rt5738_buck0_vol_shift	(0)
#define rt5738_buck0_enable_reg	RT5738_REG_CTRL2
#define rt5738_buck0_enable_bit	(0x01)
#define rt5738_buck0_min_uV	(300000)
#define rt5738_buck0_max_uV	(1850000)
#define rt5738_buck0_id		(0)

#define rt5738_buck1_vol_reg	RT5738_REG_VSEL1
#define rt5738_buck1_vol_mask	(0xff)
#define rt5738_buck1_vol_shift	(0)
#define rt5738_buck1_enable_reg	RT5738_REG_CTRL2
#define rt5738_buck1_enable_bit	(0x02)
#define rt5738_buck1_min_uV	(300000)
#define rt5738_buck1_max_uV	(1850000)
#define rt5738_buck1_id		(1)

static struct mtk_simple_regulator_control_ops rt5738_mreg_ctrl_ops = {
	.register_read = rt5738_read_byte,
	.register_write = rt5738_write_byte,
	.register_update_bits = rt5738_assign_bit,
};

static int rt5738_list_voltage(
	struct mtk_simple_regulator_desc *mreg_desc, unsigned selector)
{
	unsigned int vout = 0;

	if (selector > 200)
		vout = 1300000 + selector * 10000;
	else
		vout = mreg_desc->min_uV + 500000 * selector;

	if (vout > mreg_desc->max_uV)
		return -EINVAL;
	return vout;
}

static struct mtk_simple_regulator_desc rt5738_desc_table[] = {
	mreg_decl(rt5738_buck0,
		rt5738_list_voltage, 256, &rt5738_mreg_ctrl_ops),
	mreg_decl(rt5738_buck1,
		rt5738_list_voltage, 256, &rt5738_mreg_ctrl_ops),
};

static struct regulator_init_data rt5738_init_data[2] = {
	{
		.constraints = {
			.name = "ext_buck_vdram",
			.min_uV = 300000,
			.max_uV = 1850000,
			.valid_modes_mask =
				(REGULATOR_MODE_NORMAL|REGULATOR_MODE_FAST),
			.valid_ops_mask =
				REGULATOR_CHANGE_MODE|REGULATOR_CHANGE_VOLTAGE,
			.always_on = 1,
		},
	},
	{
		.constraints = {
			.name = "ext_buck_rt5738-2",
			.min_uV = 300000,
			.max_uV = 1850000,
			.valid_modes_mask =
				(REGULATOR_MODE_NORMAL|REGULATOR_MODE_FAST),
			.valid_ops_mask =
				REGULATOR_CHANGE_MODE|REGULATOR_CHANGE_VOLTAGE,
			.always_on = 1,
		},
	},
};

#define RT5738_REGULATOR_MODE_REG0	RT5738_REG_CTRL1
#define RT5738_REGULATOR_MODE_REG1	RT5738_REG_CTRL1
#define RT5738_REGULATOR_MODE_MASK0	(0x01)
#define RT5738_REGULATOR_MODE_MASK1	(0x02)

#define RT5738_REGULATOR_DECL(_id)	\
{	\
	.mreg_desc = &rt5738_desc_table[_id],	\
	.mode_reg = RT5738_REGULATOR_MODE_REG##_id,	\
	.mode_bit = RT5738_REGULATOR_MODE_MASK##_id,	\
	.ramp_up_reg = RT5738_REG_CTRL3,		\
	.ramp_down_reg = RT5738_REG_CTRL4,		\
	.ramp_mask = 0x3f,	\
	.ramp_shift = 0,					\
}

static struct rt5738_regulator_info rt5738_regulator_infos[] = {
	RT5738_REGULATOR_DECL(0),
	RT5738_REGULATOR_DECL(1),
};

static struct rt5738_regulator_info *rt5738_find_regulator_info(int id)
{
	struct rt5738_regulator_info *info;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(rt5738_regulator_infos); i++) {
		info = &rt5738_regulator_infos[i];
		if (info->mreg_desc->rdesc.id == id)
			return info;
	}
	return NULL;
}

static int rt5738_set_mode(
	struct mtk_simple_regulator_desc *mreg_desc, unsigned int mode)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret;

	switch (mode) {
	case REGULATOR_MODE_FAST: /* force pwm mode */
		ret = rt5738_set_bit(mreg_desc->client,
				info->mode_reg, info->mode_bit);
		break;
	case REGULATOR_MODE_NORMAL:
	default:
		ret = rt5738_clr_bit(mreg_desc->client,
				info->mode_reg, info->mode_bit);
		break;
	}
	return ret;
}

static unsigned int rt5738_get_mode(
		struct mtk_simple_regulator_desc *mreg_desc)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret;
	uint32_t regval = 0;

	ret = rt5738_read_byte(mreg_desc->client, info->mode_reg, &regval);
	if (ret < 0) {
		RT5738_ERR("%s read mode fail\n", __func__);
		return ret;
	}

	if (regval & info->mode_bit)
		return REGULATOR_MODE_FAST;
	return REGULATOR_MODE_NORMAL;
}

static int rt5738_set_ramp_dly(struct mtk_simple_regulator_desc *mreg_desc,
								int ramp_dly)
{
	struct rt5738_regulator_info *info =
		rt5738_find_regulator_info(mreg_desc->rdesc.id);
	int ret = 0;
	unsigned char regval;

	if (ramp_dly > 63)
		regval = 63;
	else
		regval = ramp_dly;

	ret = rt5738_assign_bit(mreg_desc->client, info->ramp_up_reg,
			info->ramp_mask, regval << info->ramp_shift);
	ret = rt5738_assign_bit(mreg_desc->client, info->ramp_down_reg,
			info->ramp_mask, regval << info->ramp_shift);

	return ret;
}

static struct mtk_simple_regulator_ext_ops rt5738_regulator_ext_ops = {
	.set_mode = rt5738_set_mode,
	.get_mode = rt5738_get_mode,
};

int rt5738_regulator_deinit(struct rt5738_chip *chip)
{
	int ret = 0;
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(rt5738_desc_table); i++)
		ret |= mtk_simple_regulator_unregister(&rt5738_desc_table[i]);
	return ret;
}

int rt5738_regulator_init(struct rt5738_chip *chip)
{
	int ret = 0;
	unsigned int i = 0;

	if (chip == NULL) {
		RT5738_ERR("%s Null chip info\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(rt5738_desc_table); i++) {
		rt5738_desc_table[i].client = chip->i2c;
		rt5738_desc_table[i].def_init_data =
					&rt5738_init_data[i];
#if 1
		ret = mtk_simple_regulator_register(&rt5738_desc_table[i],
				chip->dev, &rt5738_regulator_ext_ops, NULL);
#else
		ret = mtk_simple_regulator_register(&rt5738_desc_table[i],
				&rt5738_regulator_ext_ops, NULL);
#endif
		if (ret < 0)
			RT5738_ERR("%s register mtk simple regulator fail\n",
				__func__);
		rt5738_set_ramp_dly(&rt5738_desc_table[i], 0);
	}
	return 0;
}

/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2024. All rights reserved.
 * 2024-05-06 File created.
 */

#ifndef __FS15V1_OPS_H__
#define __FS15V1_OPS_H__

#include "frsm-dev.h"
#include "internal.h"

#define FS15V1_00H_STATUS		0x00
#define FS15V1_03H_ANASTAT		0x03
#define FS15V1_0EH_CHIPINI		0x0E
#define FS15V1_10H_PWRCTRL		0x10
#define FS15V1_11H_SYSCTRL		0x11
#define FS15V1_1FH_ACCKEY		0x1F
#define FS15V1_3FH_LNMCTRL		0x3F
#define FS15V1_E3H_OTPPG0W3		0xE3

#define FS15V1_00H_BOPS_MASK		BIT(9)
#define FS15V1_00H_CPOLDS_MASK		BIT(6)
#define FS15V1_00H_OCDS_MASK		BIT(5)
#define FS15V1_00H_UVDS_MASK		BIT(4)
#define FS15V1_00H_OVDS_MASK		BIT(3)
#define FS15V1_00H_OTPDS_MASK		BIT(2)
#define FS15V1_03H_OCDS_MASK		BIT(13)
#define FS15V1_0EH_INIST_SHIFT		0
#define FS15V1_0EH_INIST_MASK		GENMASK(1, 0)
#define FS15V1_3FH_LNMODE_SHIFT		15
#define FS15V1_3FH_LNMODE_MASK		BIT(15)
#define FS15V1_E3H_AGL_POTR_SHIFT	8
#define FS15V1_E3H_AGL_POTR_MASK	GENMASK(10, 8)

#define FS15V1_00H_STATUS_OK		0x0000
#define FS15V1_03H_ANASTAT_OK		0x000D
#define FS15V1_0EH_INIST_OK		0x0003
#define FS15V1_10H_I2C_RESET		0x0003
#define FS15V1_10H_POWER_UP		0x0000
#define FS15V1_10H_POWER_DOWN		0x0001
#define FS15V1_11H_SYSTEM_UP		0x00C0
#define FS15V1_11H_SYSTEM_DOWN		0x0000
#define FS15V1_1FH_ACCKEY_ON		0xCA91
#define FS15V1_1FH_ACCKEY_OFF		0x5A5A
#define FS15V1_1FH_ACCKEY_DFT		0x0000

static int fs15v1_set_mute(struct frsm_dev *frsm_dev, int mute);
static int fs15v1_shut_down(struct frsm_dev *frsm_dev);

static int fs15v1_i2c_reset(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret  = frsm_reg_write(frsm_dev, FS15V1_10H_PWRCTRL,
			FS15V1_10H_I2C_RESET);
	FRSM_DELAY_MS(5);
	ret |= frsm_reg_wait_stable(frsm_dev, FS15V1_0EH_CHIPINI,
			FS15V1_0EH_INIST_MASK, FS15V1_0EH_INIST_OK);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to check CHIPINI\n");

	return ret;
}

static int fs15v1_agl_init(struct frsm_dev *frsm_dev)
{
	uint16_t reg_val, field_val;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;
	if (!((frsm_dev->pdata->fs15wt_series & GENMASK(1, 0))
			== GENMASK(1, 0)))
		return 0;
	ret = frsm_reg_write(frsm_dev, FS15V1_10H_PWRCTRL,
			FS15V1_10H_POWER_UP); //osc on
	ret |= frsm_reg_write(frsm_dev, FS15V1_1FH_ACCKEY,
			FS15V1_1FH_ACCKEY_ON);
	ret |= frsm_reg_read(frsm_dev, FS15V1_E3H_OTPPG0W3, &reg_val);

	field_val = (reg_val & FS15V1_E3H_AGL_POTR_MASK)
			>> FS15V1_E3H_AGL_POTR_SHIFT;
	if (field_val == 0)
		field_val = 7;
	else if (field_val != 4)
		field_val -= 1;

	reg_val = (reg_val & ~FS15V1_E3H_AGL_POTR_MASK) |
			(field_val << FS15V1_E3H_AGL_POTR_SHIFT);
	ret |= frsm_reg_write(frsm_dev, FS15V1_E3H_OTPPG0W3, reg_val);
	ret |= frsm_reg_write(frsm_dev, FS15V1_1FH_ACCKEY,
			FS15V1_1FH_ACCKEY_DFT);
	ret |= frsm_reg_write(frsm_dev, FS15V1_10H_PWRCTRL,
			FS15V1_10H_POWER_DOWN);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to init agl\n");

	return ret;
}

static int fs15v1_dev_init(struct frsm_dev *frsm_dev)
{
	struct scene_table *scene;
	uint16_t val;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret = frsm_reg_read(frsm_dev, FS15V1_1FH_ACCKEY, &val);
	if (ret || (!frsm_dev->force_init && val == FS15V1_1FH_ACCKEY_OFF))
		return ret;

	frsm_dev->force_init = false;
	frsm_dev->state &= BIT(EVENT_STAT_MNTR);

	ret = fs15v1_i2c_reset(frsm_dev);
	if (ret)
		return ret;

	if (frsm_dev->tbl_scene == NULL) {
		dev_err(frsm_dev->dev, "Scene table is null\n");
		return -EINVAL;
	}

	scene = (struct scene_table *)frsm_dev->tbl_scene->buf;
	/* write reg table */
	ret = frsm_write_reg_table(frsm_dev, scene->reg);
	ret |= fs15v1_agl_init(frsm_dev);
	if (ret)
		return ret;

	ret |= fs15v1_set_mute(frsm_dev, 1);
	ret |= fs15v1_shut_down(frsm_dev);
	if (!ret) {
		frsm_dev->cur_scene = scene;
		frsm_reg_write(frsm_dev, FS15V1_1FH_ACCKEY,
				FS15V1_1FH_ACCKEY_OFF);
	}

	return ret;
}

static int fs15v1_set_scene(struct frsm_dev *frsm_dev,
		struct scene_table *scene)
{
	const struct scene_table *cur_scene;
	uint16_t val;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL || scene == NULL)
		return -EINVAL;

	cur_scene = frsm_dev->cur_scene;
	if (cur_scene == scene)
		return 0;

	if (cur_scene && cur_scene->reg == scene->reg)
		return 0;

	ret = frsm_reg_read(frsm_dev, FS15V1_1FH_ACCKEY, &val);
	if (ret)
		return ret;

	ret  = frsm_write_reg_table(frsm_dev, scene->reg);
	ret |= frsm_reg_write(frsm_dev, FS15V1_1FH_ACCKEY, val);
	ret |= frsm_reg_read(frsm_dev, FS15V1_3FH_LNMCTRL, &val);
	frsm_dev->state_lnm = !!(val & FS15V1_3FH_LNMODE_MASK);

	return ret;
}

static int fs15v1_hw_params(struct frsm_dev *frsm_dev)
{
	int ret;

	ret = frsm_reg_update_bits(frsm_dev, FS15V1_3FH_LNMCTRL,
			FS15V1_3FH_LNMODE_MASK,
			frsm_dev->state_lnm << FS15V1_3FH_LNMODE_SHIFT);
	return ret;
}

static int fs15v1_start_up(struct frsm_dev *frsm_dev)
{
	return frsm_reg_write(frsm_dev, FS15V1_10H_PWRCTRL,
			FS15V1_10H_POWER_UP);
}

static int fs15v1_set_mute(struct frsm_dev *frsm_dev, int mute)
{
	uint16_t val;
	int ret;

	if (mute) {
		ret = frsm_reg_read(frsm_dev, FS15V1_3FH_LNMCTRL, &val);
		if (val & FS15V1_3FH_LNMODE_MASK) {
			ret |= frsm_reg_write(frsm_dev, FS15V1_3FH_LNMCTRL,
				val & ~FS15V1_3FH_LNMODE_MASK);
			FRSM_DELAY_MS(8);
		}
		ret |= frsm_reg_write(frsm_dev, FS15V1_11H_SYSCTRL,
				FS15V1_11H_SYSTEM_DOWN);
	} else {
		ret = frsm_reg_write(frsm_dev, FS15V1_11H_SYSCTRL,
				FS15V1_11H_SYSTEM_UP);
	}

	return ret;
}

static int fs15v1_shut_down(struct frsm_dev *frsm_dev)
{
	return frsm_reg_write(frsm_dev, FS15V1_10H_PWRCTRL,
			FS15V1_10H_POWER_DOWN);
}

static int fs15v1_stat_monitor(struct frsm_dev *frsm_dev)
{
	uint16_t stat, anast;
	int ret;

	ret = frsm_reg_read_status(frsm_dev, FS15V1_00H_STATUS, &stat);
	ret |= frsm_reg_read_status(frsm_dev, FS15V1_03H_ANASTAT, &anast);
	if (ret)
		return ret;

	if (stat == FS15V1_00H_STATUS_OK && anast == FS15V1_03H_ANASTAT_OK)
		return ret;

	if (stat & FS15V1_00H_BOPS_MASK)
		dev_info(frsm_dev->dev, "BOP detected\n");
	if (stat & FS15V1_00H_CPOLDS_MASK)
		dev_err(frsm_dev->dev, "CPOL detected\n");
	if (anast & FS15V1_03H_OCDS_MASK)
		dev_err(frsm_dev->dev, "OC detected\n");
	if (stat & FS15V1_00H_UVDS_MASK)
		dev_err(frsm_dev->dev, "UV detected\n");
	if (stat & FS15V1_00H_OVDS_MASK)
		dev_err(frsm_dev->dev, "OV detected\n");
	if (stat & FS15V1_00H_OTPDS_MASK)
		dev_err(frsm_dev->dev, "OT detected\n");

	if ((stat & 0x007C) != FS15V1_00H_STATUS_OK
			&& (anast & 0xFC0D) != FS15V1_03H_ANASTAT_OK)
		frsm_reg_dump(frsm_dev, 0xCF); // dump reg: 00~CF

	return ret;
}

static int fs15v1_dev_ops(struct frsm_dev *frsm_dev)
{
	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	frsm_dev->ops.dev_init  = fs15v1_dev_init;
	frsm_dev->ops.set_scene = fs15v1_set_scene;
	frsm_dev->ops.hw_params = fs15v1_hw_params;
	frsm_dev->ops.start_up = fs15v1_start_up;
	frsm_dev->ops.set_mute  = fs15v1_set_mute;
	frsm_dev->ops.shut_down = fs15v1_shut_down;
	frsm_dev->ops.stat_monitor = fs15v1_stat_monitor;

	return 0;
}

#endif // __FS15V1_OPS_H__

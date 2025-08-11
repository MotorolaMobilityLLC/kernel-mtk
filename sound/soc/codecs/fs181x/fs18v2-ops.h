/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2024. All rights reserved.
 * 2024-05-06 File created.
 */

#ifndef __FS18V2_OPS_H__
#define __FS18V2_OPS_H__

#include "frsm-dev.h"
#include "internal.h"

#define FS18V2_00H_STATUS		0x00
#define FS18V2_03H_ANASTAT		0x03
#define FS18V2_0EH_CHIPINI		0x0E
#define FS18V2_10H_PWRCTRL		0x10
#define FS18V2_11H_SYSCTRL		0x11
#define FS18V2_1FH_ACCKEY		0x1F
#define FS18V2_3FH_LNMCTRL		0x3F

#define FS18V2_00H_SPKT_MASK		BIT(11)
#define FS18V2_00H_SPKS_MASK		BIT(10)
#define FS18V2_00H_BOPS_MASK		BIT(9)
#define FS18V2_00H_OTWDS_MASK		BIT(8)
#define FS18V2_00H_OCDS_MASK		BIT(5)
#define FS18V2_00H_UVDS_MASK		BIT(4)
#define FS18V2_00H_OVDS_MASK		BIT(3)
#define FS18V2_00H_OTPDS_MASK		BIT(2)
#define FS18V2_00H_BOVDS_MASK		BIT(0)
#define FS18V2_03H_UVDL_MASK		BIT(12)
#define FS18V2_03H_OVDL_MASK		BIT(11)
#define FS18V2_0EH_INIST_MASK		GENMASK(1, 0)
#define FS18V2_3FH_LNMODE_SHIFT		15
#define FS18V2_3FH_LNMODE_MASK		BIT(15)

#define FS18V2_00H_STATUS_OK		0x0000
#define FS18V2_03H_ANASTAT_OK		0x000D
#define FS18V2_0EH_INIST_OK		0x0003
#define FS18V2_10H_I2C_RESET		0x0003
#define FS18V2_10H_POWER_UP		0x0000
#define FS18V2_10H_POWER_DOWN		0x0001
#define FS18V2_11H_SYSTEM_UP		0x008F
#define FS18V2_11H_SYSTEM_DOWN		0x0000
#define FS18V2_1FH_ACCKEY_ON		0xCA91
#define FS18V2_1FH_ACCKEY_OFF		0x5A5A
#define FS18V2_1FH_ACCKEY_DFT		0x0000

static int fs18v2_set_mute(struct frsm_dev *frsm_dev, int mute);
static int fs18v2_shut_down(struct frsm_dev *frsm_dev);

static int fs18v2_i2c_reset(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret  = frsm_reg_write(frsm_dev, FS18V2_10H_PWRCTRL,
			FS18V2_10H_I2C_RESET);
	FRSM_DELAY_MS(10);
	ret |= frsm_reg_wait_stable(frsm_dev, FS18V2_0EH_CHIPINI,
			FS18V2_0EH_INIST_MASK, FS18V2_0EH_INIST_OK);
	if (ret)
		dev_err(frsm_dev->dev, "Failed to check CHIPINI\n");

	return ret;
}

static int fs18v2_dev_init(struct frsm_dev *frsm_dev)
{
	struct scene_table *scene;
	uint16_t val;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret = frsm_reg_read(frsm_dev, FS18V2_1FH_ACCKEY, &val);
	if (ret || (!frsm_dev->force_init && val == FS18V2_1FH_ACCKEY_OFF))
		return ret;

	frsm_dev->force_init = false;
	frsm_dev->state &= BIT(EVENT_STAT_MNTR);

	ret = fs18v2_i2c_reset(frsm_dev);
	if (ret)
		return ret;

	if (frsm_dev->tbl_scene == NULL) {
		dev_err(frsm_dev->dev, "Scene table is null\n");
		return -EINVAL;
	}

	scene = (struct scene_table *)frsm_dev->tbl_scene->buf;
	/* write reg table */
	ret = frsm_write_reg_table(frsm_dev, scene->reg);
	if (ret)
		return ret;

	ret |= fs18v2_set_mute(frsm_dev, 1);
	ret |= fs18v2_shut_down(frsm_dev);
	if (!ret) {
		frsm_dev->cur_scene = scene;
		frsm_reg_write(frsm_dev, FS18V2_1FH_ACCKEY,
				FS18V2_1FH_ACCKEY_OFF);
	}

	return ret;
}

static int fs18v2_set_scene(struct frsm_dev *frsm_dev,
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

	ret = frsm_reg_read(frsm_dev, FS18V2_1FH_ACCKEY, &val);
	if (ret)
		return ret;

	ret  = frsm_write_reg_table(frsm_dev, scene->reg);
	ret |= frsm_reg_write(frsm_dev, FS18V2_1FH_ACCKEY, val);
	ret |= frsm_reg_read(frsm_dev, FS18V2_3FH_LNMCTRL, &val);
	frsm_dev->state_lnm = !!(val & FS18V2_3FH_LNMODE_MASK);

	return ret;
}

static int fs18v2_hw_params(struct frsm_dev *frsm_dev)
{
	int ret;

	ret = frsm_reg_update_bits(frsm_dev, FS18V2_3FH_LNMCTRL,
			FS18V2_3FH_LNMODE_MASK,
			frsm_dev->state_lnm << FS18V2_3FH_LNMODE_SHIFT);
	return ret;
}

static int fs18v2_start_up(struct frsm_dev *frsm_dev)
{
	return frsm_reg_write(frsm_dev, FS18V2_11H_SYSCTRL,
			FS18V2_11H_SYSTEM_UP);
}

static int fs18v2_set_mute(struct frsm_dev *frsm_dev, int mute)
{
	uint16_t val;
	int ret;

	if (mute) {
		ret = frsm_reg_read(frsm_dev, FS18V2_3FH_LNMCTRL, &val);
		if (val & FS18V2_3FH_LNMODE_MASK) {
			ret |= frsm_reg_write(frsm_dev, FS18V2_3FH_LNMCTRL,
				val & ~FS18V2_3FH_LNMODE_MASK);
			FRSM_DELAY_MS(10);
		}
		ret |= frsm_reg_write(frsm_dev, FS18V2_10H_PWRCTRL,
				FS18V2_10H_POWER_DOWN);
		FRSM_DELAY_MS(20);
	} else {
		ret = frsm_reg_write(frsm_dev, FS18V2_10H_PWRCTRL,
				FS18V2_10H_POWER_UP);
	}

	return ret;
}

static int fs18v2_shut_down(struct frsm_dev *frsm_dev)
{
	return frsm_reg_write(frsm_dev, FS18V2_11H_SYSCTRL,
			FS18V2_11H_SYSTEM_DOWN);
}

static int fs18v2_stat_monitor(struct frsm_dev *frsm_dev)
{
	uint16_t stat, anast;
	int ret;

	ret = frsm_reg_read_status(frsm_dev, FS18V2_00H_STATUS, &stat);
	ret |= frsm_reg_read_status(frsm_dev, FS18V2_03H_ANASTAT, &anast);
	if (ret)
		return ret;

	if (stat == FS18V2_00H_STATUS_OK && anast == FS18V2_03H_ANASTAT_OK)
		return ret;

	if (stat & FS18V2_00H_SPKT_MASK)
		dev_info(frsm_dev->dev, "SPKP detected\n");
	if (stat & FS18V2_00H_SPKS_MASK)
		dev_err(frsm_dev->dev, "SPKE detected\n");
	if (stat & FS18V2_00H_BOPS_MASK)
		dev_info(frsm_dev->dev, "BOP detected\n");
	if (stat & FS18V2_00H_OTWDS_MASK)
		dev_err(frsm_dev->dev, "OTW detected\n");
	if (stat & FS18V2_00H_OCDS_MASK)
		dev_err(frsm_dev->dev, "OC detected\n");
	if ((anast & FS18V2_03H_UVDL_MASK) || (anast & FS18V2_03H_OVDL_MASK))
		dev_err(frsm_dev->dev, "UV/OV detected\n");
	if (stat & FS18V2_00H_OTPDS_MASK)
		dev_err(frsm_dev->dev, "OT detected\n");
	if (stat & FS18V2_00H_BOVDS_MASK)
		dev_err(frsm_dev->dev, "BOV detected\n");

	if ((stat & 0x003F) != FS18V2_00H_STATUS_OK
			&& (anast & 0xFF0F) != FS18V2_03H_ANASTAT_OK)
		frsm_reg_dump(frsm_dev, 0xCF); // dump reg: 00~CF

	return ret;
}

static int fs18v2_dev_ops(struct frsm_dev *frsm_dev)
{
	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	frsm_dev->ops.dev_init  = fs18v2_dev_init;
	frsm_dev->ops.set_scene = fs18v2_set_scene;
	frsm_dev->ops.hw_params = fs18v2_hw_params;
	frsm_dev->ops.start_up = fs18v2_start_up;
	frsm_dev->ops.set_mute  = fs18v2_set_mute;
	frsm_dev->ops.shut_down = fs18v2_shut_down;
	frsm_dev->ops.stat_monitor = fs18v2_stat_monitor;

	return 0;
}

#endif // __FS18V2_OPS_H__

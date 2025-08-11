/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2025. All rights reserved.
 * 2025-02-28 File created.
 */

#ifndef __FS18V3_OPS_H__
#define __FS18V3_OPS_H__

#include "frsm-dev.h"
#include "internal.h"

#define FS18V3_00H_STATUS		0x00
#define FS18V3_03H_ANASTAT		0x03
#define FS18V3_05H_CHANASTAT		0x05
#define FS18V3_0EH_CHIPINI		0x0E
#define FS18V3_10H_PWRCTRL		0x10
#define FS18V3_11H_SYSCTRL		0x11
#define FS18V3_1FH_ACCKEY		0x1F

#define FS18V3_00H_BOPS_MASK		BIT(9)
#define FS18V3_00H_OTWDS_MASK		BIT(8)
#define FS18V3_00H_UVDS_MASK		BIT(4)
#define FS18V3_00H_OVDS_MASK		BIT(3)
#define FS18V3_00H_OTPDS_MASK		BIT(2)
#define FS18V3_05H_CH2OCDL_MASK		BIT(13)
#define FS18V3_05H_CH1OCDL_MASK		BIT(5)
#define FS18V3_05H_CHOCDL_MASK		(FS18V3_05H_CH2OCDL_MASK |\
		FS18V3_05H_CH1OCDL_MASK)
#define FS18V3_0EH_INIST_SHIFT		0
#define FS18V3_0EH_INIST_MASK		GENMASK(1, 0)
#define FS18V3_00H_STATUS_OK		0x0000
#define FS18V3_03H_ANASTAT_OK		0x000D
#define FS18V3_05H_CHANASTAT_OK		0x0C0C
#define FS18V3_0EH_INIST_OK		0x0003
#define FS18V3_10H_I2C_RESET		0x0003
#define FS18V3_10H_POWER_UP		0x0000
#define FS18V3_10H_POWER_DOWN		0x0001
#define FS18V3_11H_SYSTEM_MASK		GENMASK(1, 0)
#define FS18V3_11H_SYSTEM_UP		0x0003
#define FS18V3_11H_SYSTEM_DOWN		0x0000
#define FS18V3_1FH_ACCKEY_ON		0xCA91
#define FS18V3_1FH_ACCKEY_OFF		0x5A5A
#define FS18V3_1FH_ACCKEY_DFT		0x0000

static int fs18v3_set_mute(struct frsm_dev *frsm_dev, int mute);
static int fs18v3_shut_down(struct frsm_dev *frsm_dev);

static int fs18v3_i2c_reset(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret = frsm_reg_write(frsm_dev, FS18V3_10H_PWRCTRL,
			FS18V3_10H_I2C_RESET);
	FRSM_DELAY_MS(5);

	return ret;
}

static int fs18v3_dev_init(struct frsm_dev *frsm_dev)
{
	struct scene_table *scene;
	uint16_t val;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret = frsm_reg_read(frsm_dev, FS18V3_1FH_ACCKEY, &val);
	if (ret || (!frsm_dev->force_init && val == FS18V3_1FH_ACCKEY_OFF))
		return ret;

	frsm_dev->force_init = false;
	frsm_dev->state &= BIT(EVENT_STAT_MNTR);

	ret = fs18v3_i2c_reset(frsm_dev);
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

	ret |= fs18v3_set_mute(frsm_dev, 1);
	ret |= fs18v3_shut_down(frsm_dev);
	if (!ret) {
		frsm_dev->cur_scene = scene;
		frsm_reg_write(frsm_dev, FS18V3_1FH_ACCKEY,
				FS18V3_1FH_ACCKEY_OFF);
	}

	return ret;
}

static int fs18v3_set_scene(struct frsm_dev *frsm_dev,
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

	ret = frsm_reg_read(frsm_dev, FS18V3_1FH_ACCKEY, &val);
	if (ret)
		return ret;

	ret  = frsm_write_reg_table(frsm_dev, scene->reg);
	ret |= frsm_reg_write(frsm_dev, FS18V3_1FH_ACCKEY, val);

	return ret;
}

static int fs18v3_hw_params(struct frsm_dev *frsm_dev)
{
	return 0;
}

static int fs18v3_start_up(struct frsm_dev *frsm_dev)
{
	int ret;

	ret  = frsm_reg_write(frsm_dev, FS18V3_10H_PWRCTRL,
			FS18V3_10H_POWER_UP);
	ret |= frsm_reg_update_bits(frsm_dev, FS18V3_11H_SYSCTRL,
			FS18V3_11H_SYSTEM_MASK, FS18V3_11H_SYSTEM_UP);

	return ret;
}

static int fs18v3_set_mute(struct frsm_dev *frsm_dev, int mute)
{
	return 0;
}

static int fs18v3_shut_down(struct frsm_dev *frsm_dev)
{
	int ret;

	ret  = frsm_reg_write(frsm_dev, FS18V3_10H_PWRCTRL,
			FS18V3_10H_POWER_DOWN);
	ret |= frsm_reg_update_bits(frsm_dev, FS18V3_11H_SYSCTRL,
			FS18V3_11H_SYSTEM_MASK, FS18V3_11H_SYSTEM_DOWN);

	return ret;
}

static int fs18v3_stat_monitor(struct frsm_dev *frsm_dev)
{
	uint16_t stat, chanast;
	int ret;

	ret  = frsm_reg_read(frsm_dev, FS18V3_00H_STATUS, &stat);
	ret |= frsm_reg_read(frsm_dev, FS18V3_05H_CHANASTAT, &chanast);
	if (ret)
		return ret;

	if (stat == FS18V3_00H_STATUS_OK && !(chanast & FS18V3_05H_CHOCDL_MASK))
		return ret;

	if (chanast & FS18V3_05H_CH2OCDL_MASK)
		dev_err(frsm_dev->dev, "CH2 OC detected\n");
	if (chanast & FS18V3_05H_CH1OCDL_MASK)
		dev_err(frsm_dev->dev, "CH1 OC detected\n");
	if (stat & FS18V3_00H_BOPS_MASK)
		dev_err(frsm_dev->dev, "BOP detected\n");
	if (stat & FS18V3_00H_OTWDS_MASK)
		dev_err(frsm_dev->dev, "OTW detected\n");
	if (stat & FS18V3_00H_UVDS_MASK)
		dev_err(frsm_dev->dev, "UV detected\n");
	if (stat & FS18V3_00H_OVDS_MASK)
		dev_err(frsm_dev->dev, "OV detected\n");
	if (stat & FS18V3_00H_OTPDS_MASK)
		dev_err(frsm_dev->dev, "OTP detected\n");

	frsm_reg_dump(frsm_dev, 0xCF); // dump reg, and read clear: 00~CF

	return ret;
}

static int fs18v3_dev_ops(struct frsm_dev *frsm_dev)
{
	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	frsm_dev->ops.dev_init  = fs18v3_dev_init;
	frsm_dev->ops.set_scene = fs18v3_set_scene;
	frsm_dev->ops.hw_params = fs18v3_hw_params;
	frsm_dev->ops.start_up = fs18v3_start_up;
	frsm_dev->ops.set_mute  = fs18v3_set_mute;
	frsm_dev->ops.shut_down = fs18v3_shut_down;
	frsm_dev->ops.stat_monitor = fs18v3_stat_monitor;

	return 0;
}

#endif // __FS18V3_OPS_H__

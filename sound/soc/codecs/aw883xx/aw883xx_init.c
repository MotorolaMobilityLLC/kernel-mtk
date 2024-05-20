// SPDX-License-Identifier: GPL-2.0
/* aw883xx_init.c   aw883xx codec driver
 *
 * Copyright (c) 2020 AWINIC Technology CO., LTD
 *
 *  Author: Bruce zhao <zhaolei@awinic.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/*#define DEBUG*/
#include <linux/module.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>
#include <sound/control.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

#include "aw883xx.h"
#include "aw883xx_device.h"
#include "aw883xx_bin_parse.h"
#include "aw883xx_log.h"


static int aw883xx_dev_reg_read(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t *reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_read(aw883xx, reg_addr, reg_data);
}

static int aw883xx_dev_reg_write(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_write(aw883xx, reg_addr, reg_data);
}

static int aw883xx_dev_reg_write_bits(struct aw_device *aw_dev,
			uint8_t reg_addr, uint16_t mask, uint16_t reg_data)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_reg_write_bits(aw883xx, reg_addr, mask, reg_data);
}

static int aw883xx_dev_dsp_write_bits(struct aw_device *aw_dev,
	uint16_t dsp_addr, uint32_t dsp_mask, uint32_t dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_write_bits(aw883xx, dsp_addr, dsp_mask, dsp_data, data_type);
}

static int aw883xx_dev_dsp_write(struct aw_device *aw_dev,
			uint16_t dsp_addr, uint32_t dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_write(aw883xx, dsp_addr, dsp_data, data_type);
}

static int aw883xx_dev_dsp_writes(struct aw_device *aw_dev,
			uint8_t *data, uint32_t len, uint16_t base)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_writes(aw883xx, data, len, base);
}

static int aw883xx_dev_dsp_read(struct aw_device *aw_dev,
			uint16_t dsp_addr, uint32_t *dsp_data, uint8_t data_type)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_dev->private_data;

	return aw883xx_dsp_read(aw883xx, dsp_addr, dsp_data, data_type);
}

static int aw883xx_dev_common_init(struct aw_device *aw_pa)
{
	struct aw883xx *aw883xx = (struct aw883xx *)aw_pa->private_data;

	aw_pa->prof_info.prof_desc = NULL;
	aw_pa->prof_info.count = 0;
	aw_pa->prof_info.prof_type = AW_DEV_NONE_TYPE_ID;
	aw_pa->channel = 0;
	aw_pa->i2c_lock = &aw883xx->i2c_lock;
	aw_pa->i2c = aw883xx->i2c;
	aw_pa->fw_status = AW_DEV_FW_FAILED;

	aw_pa->chip_id = aw883xx->chip_id;
	aw_pa->dev = aw883xx->dev;
	aw_pa->ops.aw_get_version = aw883xx_get_version;
	aw_pa->ops.aw_reg_write = aw883xx_dev_reg_write;
	aw_pa->ops.aw_reg_write_bits = aw883xx_dev_reg_write_bits;
	aw_pa->ops.aw_reg_read = aw883xx_dev_reg_read;
	aw_pa->ops.aw_dsp_write_bits = aw883xx_dev_dsp_write_bits;
	aw_pa->ops.aw_dsp_read = aw883xx_dev_dsp_read;
	aw_pa->ops.aw_dsp_write = aw883xx_dev_dsp_write;
	aw_pa->ops.aw_dsp_writes = aw883xx_dev_dsp_writes;
	aw_pa->ops.aw_get_dev_num = aw883xx_get_dev_num;

	return 0;
}

int aw883xx_init_check_chipid(struct aw883xx *aw883xx, uint16_t chipid)
{
	switch (chipid) {
	case AW883XX_PID_2049:
	case AW883XX_PID_2066:
	case AW883XX_PID_2183:
		aw883xx->chip_id = chipid;
		break;
	default:
		aw_dev_err(aw883xx->dev, "unsupported chip_id 0x%04x", chipid);
		return -EINVAL;
	}
	return 0;
}

int aw883xx_init(struct aw883xx *aw883xx)
{
	int ret = -1;
	struct aw_device *aw_pa = NULL;

	aw_pa = devm_kzalloc(aw883xx->dev, sizeof(struct aw_device), GFP_KERNEL);
	if (aw_pa == NULL)
		return -ENOMEM;

	aw_pa->private_data = (void *)aw883xx;
	aw883xx->aw_pa = aw_pa;

	aw883xx_dev_common_init(aw_pa);

	switch (aw883xx->chip_id) {
	case AW883XX_PID_2049:
		aw883xx_pid_2049_dev_init(aw_pa);
		break;
	case AW883XX_PID_2066:
		aw883xx_pid_2066_dev_init(aw_pa);
		break;
	case AW883XX_PID_2183:
		aw883xx_pid_2183_dev_init(aw_pa);
		break;
	default:
		aw_dev_err(aw883xx->dev, "unsupported chip_id 0x%04x", aw883xx->chip_id);
		return -EINVAL;
	}

	ret = aw883xx_device_probe(aw_pa);
	if (ret < 0) {
		aw_dev_err(aw883xx->dev, "device_probe failed");
		return -EINVAL;
	}

	return 0;
}


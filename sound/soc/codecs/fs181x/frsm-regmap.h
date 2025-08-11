/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-06-14 File created.
 */

#ifndef __FRSM_REGMAP_H__
#define __FRSM_REGMAP_H__

#include <linux/regmap.h>
#include "internal.h"

#define FRSM_REG_BYTES     (sizeof(uint8_t))
#define FRSM_REG_VAL_BYTES (sizeof(uint16_t))

static int frsm_reg_read(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t *pval)
{
	unsigned int val;
	int retries = 0;
	int ret;

	if (frsm_dev == NULL || frsm_dev->regmap == NULL || pval == NULL)
		return -EINVAL;

	do {
		mutex_lock(&frsm_dev->io_lock);
		ret = regmap_read(frsm_dev->regmap, reg, &val);
		mutex_unlock(&frsm_dev->io_lock);
		if (!ret)
			break;
		FRSM_DELAY_MS(FRSM_I2C_MDELAY);
		retries++;
	} while (retries < FRSM_I2C_RETRY);

	if (ret) {
		dev_err(frsm_dev->dev, "Failed to read %02Xh: %d\n", reg, ret);
		return -EIO;
	}

	*pval = val;
	dev_dbg(frsm_dev->dev, "RR: %02x %04x\n", reg, *pval);

	return 0;
}

static int frsm_reg_write(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t val)
{
	int retries = 0;
	int ret;

	if (frsm_dev == NULL || frsm_dev->regmap == NULL)
		return -EINVAL;

	do {
		mutex_lock(&frsm_dev->io_lock);
		ret = regmap_write(frsm_dev->regmap, reg, val);
		mutex_unlock(&frsm_dev->io_lock);
		if (!ret)
			break;
		FRSM_DELAY_MS(FRSM_I2C_MDELAY);
		retries++;
	} while (retries < FRSM_I2C_RETRY);

	if (ret) {
		dev_err(frsm_dev->dev, "Failed to write %02Xh: %d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(frsm_dev->dev, "RW: %02x %04x\n", reg, val);

	return 0;
}

static int frsm_reg_update_bits(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t mask, uint16_t val)
{
	int retries = 0;
	int ret;

	if (frsm_dev == NULL || frsm_dev->regmap == NULL)
		return -EINVAL;

	do {
		mutex_lock(&frsm_dev->io_lock);
		ret = regmap_update_bits(frsm_dev->regmap, reg, mask, val);
		mutex_unlock(&frsm_dev->io_lock);
		if (!ret)
			break;
		FRSM_DELAY_MS(FRSM_I2C_MDELAY);
		retries++;
	} while (retries < FRSM_I2C_RETRY);

	if (ret) {
		dev_err(frsm_dev->dev,
				"Failed to update %02Xh: %d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(frsm_dev->dev, "RU: %02x %04x %04x\n", reg, mask, val);

	return 0;
}

static int frsm_reg_bulk_read(struct frsm_dev *frsm_dev,
		uint8_t reg, void *buf, int buf_size)
{
	int val_count;
	int ret;

	if (frsm_dev == NULL || frsm_dev->regmap == NULL || buf == NULL)
		return -EINVAL;

	mutex_lock(&frsm_dev->io_lock);
	val_count = buf_size / FRSM_REG_VAL_BYTES; // change to reg val count
	ret = regmap_bulk_read(frsm_dev->regmap, reg, buf, val_count);
	mutex_unlock(&frsm_dev->io_lock);
	if (ret) {
		dev_err(frsm_dev->dev,
				"Failed to bulk read %02Xh: %d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(frsm_dev->dev, "BR: %02x N:%d\n", reg, val_count);

	return 0;
}

static int frsm_reg_bulk_write(struct frsm_dev *frsm_dev,
		uint8_t reg, const void *buf, int buf_size)
{
	int val_count;
	int ret;

	if (frsm_dev == NULL || frsm_dev->regmap == NULL || buf == NULL)
		return -EINVAL;

	mutex_lock(&frsm_dev->io_lock);
	val_count = buf_size / FRSM_REG_VAL_BYTES; // change to reg val count
	ret = regmap_bulk_write(frsm_dev->regmap, reg, buf, val_count);
	mutex_unlock(&frsm_dev->io_lock);
	if (ret) {
		dev_err(frsm_dev->dev,
				"Failed to bulk write %02Xh: %d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(frsm_dev->dev, "BW: %02x N:%d\n", reg, val_count);

	return 0;
}

static int frsm_reg_read_status(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t *pval)
{
	uint16_t old, value;
	int count;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	for (count = 0; count < FRSM_I2C_RETRY; count++) {
		ret = frsm_reg_read(frsm_dev, reg, &value);
		if (ret)
			return ret;
		if (count > 0 && old == value)
			break;
		old = value;
	}

	if (pval)
		*pval = value;

	return ret;
}

static int frsm_reg_wait_stable(struct frsm_dev *frsm_dev,
		uint8_t reg, uint16_t mask, uint16_t val)
{
	uint16_t status;
	int i, ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	FRSM_DELAY_MS(5);
	for (i = 0; i < FRSM_WAIT_TIMES; i++) {
		ret = frsm_reg_read_status(frsm_dev, reg, &status);
		if (ret)
			return ret;
		dev_dbg(frsm_dev->dev, "WS: %02x %04x %04x\n",
				reg, mask, val);
		if ((status & mask) == val)
			return 0;
		FRSM_DELAY_MS(FRSM_I2C_MDELAY);
	}

	dev_err(frsm_dev->dev, "Wait %02Xh stable timeout!\n", reg);

	return -ETIMEDOUT;
}

static int frsm_reg_dump(struct frsm_dev *frsm_dev, uint8_t reg_max)
{
	uint16_t val[8] = { 0 };
	uint8_t reg;
	uint8_t idx;
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	dev_info(frsm_dev->dev, "Dump registers[0x00-0x%02X]:\n", reg_max);
	for (reg = 0x00; reg <= reg_max; reg++) {
		idx = (reg & 0x7);
		ret = frsm_reg_read(frsm_dev, reg, &val[idx]);
		if (ret)
			break;
		if (idx != 0x7 && reg != reg_max)
			continue;
		dev_info(frsm_dev->dev,
				"%02Xh: %04x %04x %04x %04x %04x %04x %04x %04x\n",
				(reg & 0xF8), val[0], val[1], val[2],
				val[3], val[4], val[5], val[6], val[7]);
		memset(val, 0, sizeof(val));
	}

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static int frsm_reg_write_table(struct frsm_dev *frsm_dev,
		const struct reg_table *reg)
{
	const struct reg_update *regu;
	const struct reg_burst *regb;
	const struct reg_val *regv;
	const struct cmd_pkg *pkg;
	int index = 0;
	int ret = 0;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	if (reg == NULL)
		return -EINVAL;

	while (index < reg->size) {
		pkg = (struct cmd_pkg *)(reg->buf + index);
		switch (pkg->cmd) {
		case 0x00 ... FRSM_REG_MAX:
			regv = (struct reg_val *)pkg;
			ret = frsm_reg_write(frsm_dev, regv->reg, regv->val);
			index += sizeof(*regv);
			break;
		case FRSM_REG_UPDATE:
			regu = (struct reg_update *)pkg;
			ret = frsm_reg_update_bits(frsm_dev,
					regu->reg, regu->mask, regu->val);
			index += sizeof(*regu);
			break;
		case FRSM_REG_BURST:
			regb = (struct reg_burst *)pkg;
			ret = frsm_reg_bulk_write(frsm_dev, regb->buf[0],
					regb->buf + 1, regb->size >> 1);
			index += sizeof(*regb) + regb->size;
			break;
		case FRSM_REG_DELAY:
			regv = (struct reg_val *)pkg;
			dev_dbg(frsm_dev->dev, "DT: %d(ms)\n", regv->val);
			FRSM_DELAY_MS(regv->val);
			index += sizeof(*regv);
			break;
		default:
			ret = -ENOTSUPP;
			break;
		}
		if (ret)
			break;
	}

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static const struct regmap_config frsm_i2c_regmap = {
	.reg_bits = FRSM_REG_BYTES * 8,
	.val_bits = FRSM_REG_VAL_BYTES * 8,
	.max_register = FRSM_REG_MAX,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.cache_type = REGCACHE_NONE,
};

static int frsm_regmap_init(struct frsm_dev *frsm_dev)
{
	struct regmap *regmap;
	int ret;

	if (frsm_dev == NULL || frsm_dev->i2c == NULL)
		return -EINVAL;

	regmap = devm_regmap_init_i2c(frsm_dev->i2c, &frsm_i2c_regmap);
	if (!IS_ERR_OR_NULL(regmap)) {
		frsm_dev->regmap = regmap;
		return 0;
	}

	ret = PTR_ERR(regmap);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static inline void frsm_regmap_unused_func(void)
{
	frsm_reg_bulk_read(NULL, 0, NULL, 0);
}

#endif // __FRSM_REGMAP_H__

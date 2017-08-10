/*
 * Register map access API - I2C support
 *
 * Copyright 2011 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/module.h>
#ifdef CONFIG_MTK_I2C_EXTENSION
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#endif


static int regmap_smbus_byte_reg_read(void *context, unsigned int reg,
				      unsigned int *val)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;

	if (reg > 0xff)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret < 0)
		return ret;

	*val = ret;

	return 0;
}

static int regmap_smbus_byte_reg_write(void *context, unsigned int reg,
				       unsigned int val)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);

	if (val > 0xff || reg > 0xff)
		return -EINVAL;

	return i2c_smbus_write_byte_data(i2c, reg, val);
}

static struct regmap_bus regmap_smbus_byte = {
	.reg_write = regmap_smbus_byte_reg_write,
	.reg_read = regmap_smbus_byte_reg_read,
};

static int regmap_smbus_word_reg_read(void *context, unsigned int reg,
				      unsigned int *val)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;

	if (reg > 0xff)
		return -EINVAL;

	ret = i2c_smbus_read_word_data(i2c, reg);
	if (ret < 0)
		return ret;

	*val = ret;

	return 0;
}

static int regmap_smbus_word_reg_write(void *context, unsigned int reg,
				       unsigned int val)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);

	if (val > 0xffff || reg > 0xff)
		return -EINVAL;

	return i2c_smbus_write_word_data(i2c, reg, val);
}

static struct regmap_bus regmap_smbus_word = {
	.reg_write = regmap_smbus_word_reg_write,
	.reg_read = regmap_smbus_word_reg_read,
};

#ifdef CONFIG_MTK_I2C_EXTENSION
static int regmap_i2c_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;
	struct i2c_msg msg;
	uint8_t *data_v = NULL;
	dma_addr_t dma_buf_p;

	memset(&msg, 0, sizeof(struct i2c_msg));

	if (count <= 8) {
		msg.addr = i2c->addr;
		msg.flags = 0;
		msg.len = count;
		msg.buf = (uint8_t *)data;
	} else {
		data_v = dma_alloc_coherent(&(i2c->adapter->dev), count, &dma_buf_p,
					    GFP_KERNEL|GFP_DMA32);
		if (!data_v)
			return -1;

		memcpy(data_v, (uint8_t *)data, count);
		msg.addr = i2c->addr;
		msg.flags = 0;
		msg.len = count;
		msg.buf = (void *)((uintptr_t)dma_buf_p);
		msg.ext_flag |= I2C_DMA_FLAG;
	}

	ret = i2c_transfer(i2c->adapter, &msg, 1);

	if (count > 8)
		dma_free_coherent(&(i2c->adapter->dev), count, data_v, dma_buf_p);

	if (ret == 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}
#else
static int regmap_i2c_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;

	ret = i2c_master_send(i2c, data, count);
	if (ret == count)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}
#endif

static int regmap_i2c_gather_write(void *context,
				   const void *reg, size_t reg_size,
				   const void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg xfer[2];
	int ret;

	/* If the I2C controller can't do a gather tell the core, it
	 * will substitute in a linear write for us.
	 */
	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_NOSTART))
		return -ENOTSUPP;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = reg_size;
	xfer[0].buf = (void *)reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_NOSTART;
	xfer[1].len = val_size;
	xfer[1].buf = (void *)val;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret == 2)
		return 0;
	if (ret < 0)
		return ret;
	else
		return -EIO;
}

#ifdef CONFIG_MTK_I2C_EXTENSION
static int regmap_i2c_read(void *context,
			   const void *reg, size_t reg_size,
			   void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg msg;
	int ret;
	size_t def_len = (val_size > reg_size) ? val_size : reg_size;
	uint8_t *data_v = NULL;
	dma_addr_t dma_buf_p;

	memset(&msg, 0, sizeof(struct i2c_msg));

	if (def_len <= 8) {
		data_v = kzalloc(def_len, GFP_KERNEL);
		if (data_v == NULL)
			return -1;

		memcpy(data_v, (uint8_t *)reg, reg_size);
		msg.addr = i2c->addr;
		msg.flags = 0;
		msg.len = ((val_size & 0xff)<<8) | reg_size;
		msg.buf = data_v;
		msg.ext_flag = I2C_WR_FLAG | I2C_RS_FLAG;

		ret = i2c_transfer(i2c->adapter, &msg, 1);

		memcpy((uint8_t *)val, data_v, val_size);

		kfree(data_v);
	} else {
		data_v = dma_alloc_coherent(&(i2c->adapter->dev), def_len, &dma_buf_p,
					    GFP_KERNEL|GFP_DMA32);
		if (!data_v)
			return -1;

		memcpy(data_v, (uint8_t *)reg, reg_size);
		msg.addr = i2c->addr;
		msg.flags = 0;
		msg.len = ((val_size & 0xff)<<8) | reg_size;
		msg.buf = (void *)((uintptr_t)dma_buf_p);
		msg.ext_flag = I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG;

		ret = i2c_transfer(i2c->adapter, &msg, 1);

		memcpy((uint8_t *)val, data_v, val_size);

		dma_free_coherent(&(i2c->adapter->dev), def_len, data_v, dma_buf_p);
	}

	if (ret == 1)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}
#else
static int regmap_i2c_read(void *context,
			   const void *reg, size_t reg_size,
			   void *val, size_t val_size)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	struct i2c_msg xfer[2];
	int ret;

	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = reg_size;
	xfer[0].buf = (void *)reg;

	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = val_size;
	xfer[1].buf = val;

	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret == 2)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}
#endif

static struct regmap_bus regmap_i2c = {
	.write = regmap_i2c_write,
	.gather_write = regmap_i2c_gather_write,
	.read = regmap_i2c_read,
	.reg_format_endian_default = REGMAP_ENDIAN_BIG,
	.val_format_endian_default = REGMAP_ENDIAN_BIG,
};

static const struct regmap_bus *regmap_get_i2c_bus(struct i2c_client *i2c,
					const struct regmap_config *config)
{
	if (i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C))
		return &regmap_i2c;
	else if (config->val_bits == 16 && config->reg_bits == 8 &&
		 i2c_check_functionality(i2c->adapter,
					 I2C_FUNC_SMBUS_WORD_DATA))
		return &regmap_smbus_word;
	else if (config->val_bits == 8 && config->reg_bits == 8 &&
		 i2c_check_functionality(i2c->adapter,
					 I2C_FUNC_SMBUS_BYTE_DATA))
		return &regmap_smbus_byte;

	return ERR_PTR(-ENOTSUPP);
}

/**
 * regmap_init_i2c(): Initialise register map
 *
 * @i2c: Device that will be interacted with
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer to
 * a struct regmap.
 */
struct regmap *regmap_init_i2c(struct i2c_client *i2c,
			       const struct regmap_config *config)
{
	const struct regmap_bus *bus = regmap_get_i2c_bus(i2c, config);

	if (IS_ERR(bus))
		return ERR_CAST(bus);

	return regmap_init(&i2c->dev, bus, &i2c->dev, config);
}
EXPORT_SYMBOL_GPL(regmap_init_i2c);

/**
 * devm_regmap_init_i2c(): Initialise managed register map
 *
 * @i2c: Device that will be interacted with
 * @config: Configuration for register map
 *
 * The return value will be an ERR_PTR() on error or a valid pointer
 * to a struct regmap.  The regmap will be automatically freed by the
 * device management code.
 */
struct regmap *devm_regmap_init_i2c(struct i2c_client *i2c,
				    const struct regmap_config *config)
{
	const struct regmap_bus *bus = regmap_get_i2c_bus(i2c, config);

	if (IS_ERR(bus))
		return ERR_CAST(bus);

	return devm_regmap_init(&i2c->dev, bus, &i2c->dev, config);
}
EXPORT_SYMBOL_GPL(devm_regmap_init_i2c);

MODULE_LICENSE("GPL");

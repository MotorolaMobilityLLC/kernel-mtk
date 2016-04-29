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
#include <linux/init.h>
#if defined(CONFIG_SND_SOC_FLORIDA) /* lenovo-sw zhouwl, for wm8281 */
#include <linux/dma-mapping.h>

static const int I2cBuf_len = 1024;
static void* I2cBuf_pa;
static void* I2cBuf;
#endif /*CONFIG_SND_SOC_FLORIDA*/


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

static int regmap_i2c_write(void *context, const void *data, size_t count)
{
	struct device *dev = context;
	struct i2c_client *i2c = to_i2c_client(dev);
	int ret;
#if defined(CONFIG_SND_SOC_FLORIDA) /* lenovo-sw zhouwl, for wm8281 */
	u8* buf = (u8*)I2cBuf;

	i2c->addr = (i2c->addr & I2C_MASK_FLAG);
	i2c->timing = 400;
	i2c->ext_flag = ((i2c->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG |I2C_RS_FLAG | I2C_DMA_FLAG;
	memcpy( buf, data, count);
	ret = i2c_master_send(i2c, (const char*)I2cBuf_pa, count);
#else
	ret = i2c_master_send(i2c, data, count);
#endif /*CONFIG_SND_SOC_FLORIDA*/
	if (ret == count)
		return 0;
	else if (ret < 0)
		return ret;
	else
		return -EIO;
}

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

static int regmap_i2c_read(void *context,
			   const void *reg, size_t reg_size,
			   void *val, size_t val_size)
{
#ifdef CONFIG_SND_SOC_FLORIDA  /* k5 wm8281,Florida I2C with MTK platform interface Read */
	struct device *dev = context;
	int ret = 0;
	int i=0;
	u8* buf = (u8*)I2cBuf;
	u8 lens;
	struct i2c_client *i2c = to_i2c_client(dev);
	if(reg_size + val_size > I2cBuf_len) {
		return -EFAULT;
	}
	i2c->addr = (i2c->addr & I2C_MASK_FLAG);
	i2c->timing = 400;
	i2c->ext_flag = I2C_WR_FLAG | I2C_RS_FLAG | I2C_DMA_FLAG;
	memcpy( buf, reg, reg_size);
	ret = i2c_master_send(i2c, (const char*)I2cBuf_pa, (val_size << 8) | reg_size);
	if (ret < 0) {
		return -EFAULT;
	}
	memcpy( val, buf, val_size);
	return 0;
#else
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
#endif /*CONFIG_SND_SOC_FLORIDA*/
}

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

#if defined(CONFIG_SND_SOC_FLORIDA) /* lenovo-sw zhouwl, for wm8281 */
	I2cBuf = (char *)dma_alloc_coherent(NULL, I2cBuf_len, &I2cBuf_pa, GFP_KERNEL);
	if(I2cBuf == NULL) {
		//printk("devm_regmap_init_i2c : failed to allocate DMA buffer");
		return NULL;
	}
#endif /*CONFIG_SND_SOC_FLORIDA*/
	return devm_regmap_init(&i2c->dev, bus, &i2c->dev, config);
}
EXPORT_SYMBOL_GPL(devm_regmap_init_i2c);

MODULE_LICENSE("GPL");

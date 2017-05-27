/*
 *  drivers/misc/mediatek/power/mt6797/rt5735.c
 *  Driver for Richtek RT5735 Regulator
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include "rt5735.h"
#include "mtk_gpuregulator_intf.h"
#include "mt_gpufreq.h"
#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif /* #ifdef CONFIG_RT_REGMAP */

#include <mt_gpio.h>
#include <linux/delay.h>
#include <mt-plat/charging.h>
#include "fan53555.h"

#define RT5735_I2C_CHANNEL 0

struct rt5735_info {
	struct i2c_client *i2c;
	struct device *dev;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
#endif /* #ifdef CONFIG_RT_REGMAP */
	struct platform_device *pdev;
	struct mutex vario_lock;
};

static u8 rt5735_cfg_default[4] = {
	0x00, /* RT5753_REG_PGOOD */
	0x19, /* RT5753_REG_TIME */
	0x01, /* RT5753_REG_COMMAND */
	0x63, /* RT5753_REG_LIMCONF */
};

static int rt5735_read_device(void *client, u32 addr, int len, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_read_i2c_block_data(i2c, addr, len, dst);
}

static int rt5735_write_device(void *client, u32 addr, int len, const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_write_i2c_block_data(i2c, addr, len, src);
}

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT5735_REG_PID, 1, RT_WBITS_WR_ONCE, {0x00});
RT_REG_DECL(RT5735_REG_RID, 1, RT_WBITS_WR_ONCE, {0x00});
RT_REG_DECL(RT5735_REG_FID, 1, RT_WBITS_WR_ONCE, {0x00});
RT_REG_DECL(RT5735_REG_VID, 1, RT_WBITS_WR_ONCE, {0x00});
RT_REG_DECL(RT5735_REG_PROGVSEL1, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(RT5735_REG_PROGVSEL0, 1, RT_NORMAL_WR_ONCE, {});
RT_REG_DECL(RT5735_REG_PGOOD, 1, RT_WBITS_WR_ONCE, {0x13});
RT_REG_DECL(RT5735_REG_TIME, 1, RT_WBITS_WR_ONCE, {0x1c});
RT_REG_DECL(RT5735_REG_COMMAND, 1, RT_WBITS_WR_ONCE, {0xe1});
RT_REG_DECL(RT5735_REG_LIMCONF, 1, RT_WBITS_WR_ONCE, {0xf7});

static const rt_register_map_t rt5735_regmap[] = {
	RT_REG(RT5735_REG_PID),
	RT_REG(RT5735_REG_RID),
	RT_REG(RT5735_REG_FID),
	RT_REG(RT5735_REG_VID),
	RT_REG(RT5735_REG_PROGVSEL1),
	RT_REG(RT5735_REG_PROGVSEL0),
	RT_REG(RT5735_REG_PGOOD),
	RT_REG(RT5735_REG_TIME),
	RT_REG(RT5735_REG_COMMAND),
	RT_REG(RT5735_REG_LIMCONF),
};

static struct rt_regmap_properties rt5735_regmap_props = {
	.name = "rt5735",
	.aliases = "rt5735",
	.register_num = ARRAY_SIZE(rt5735_regmap),
	.rm = rt5735_regmap,
	.rt_regmap_mode = RT_CACHE_DISABLE,
};

static struct rt_regmap_fops rt5735_regmap_fops = {
	.read_device = rt5735_read_device,
	.write_device = rt5735_write_device,
};
#endif /* #ifdef CONFIG_RT_REGMAP */

static int rt5735_assign_bit(struct rt5735_info *info, u8 reg, u8 data,
		u8 mask)
{
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};

	return rt_regmap_update_bits(info->regmap_dev, &rrd, reg, mask, data);
#else
	u8 tmp = 0, regval = 0;
	int ret = 0;

	mutex_lock(&info->vario_lock);
	ret = rt5735_read_device(info->i2c, reg, 1, &regval);
	if (ret < 0) {
		dev_err(info->dev, "ar fail reg0x%02x data0x%02x\n", reg, data);
		goto OUT_ASSIGN;
	}
	tmp = ((regval & 0xff) & ~mask);
	tmp |= (data & mask);
	ret = rt5735_write_device(info->i2c, reg, 1, &tmp);
	if (ret < 0)
		dev_err(info->dev, "aw fail reg0x%02x data0x%02x\n", reg, tmp);
OUT_ASSIGN:
	mutex_unlock(&info->vario_lock);
	return  ret;
#endif /* #ifdef CONFIG_RT_REGMAP */
}

#define rt5735_set_bit(info, reg, mask) \
	rt5735_assign_bit(info, reg, mask, mask)

#define rt5735_clr_bit(info, reg, mask) \
	rt5735_assign_bit(info, reg, 0x00, mask)

static void rt5735_io_lock(void *r_info)
{
	struct rt5735_info *info = (struct rt5735_info *)r_info;

	mutex_lock(&info->vario_lock);
}

static void rt5735_io_unlock(void *r_info)
{
	struct rt5735_info *info = (struct rt5735_info *)r_info;

	mutex_unlock(&info->vario_lock);
}

static int rt5735_io_read(void *r_info, u8 reg)
{
	struct rt5735_info *info = (struct rt5735_info *)r_info;
	int ret = 0;
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};

	ret = rt_regmap_reg_read(info->regmap_dev, &rrd, reg);
	return (ret < 0 ? ret : rrd.rt_data.data_u32);
#else
	u8 regval;

	ret = rt5735_read_device(info->i2c, reg, 1, &regval);
	return (ret < 0 ? ret : regval);
#endif
}

static int rt5735_io_write(void *r_info, u8 reg, u8 data)
{
	struct rt5735_info *info = (struct rt5735_info *)r_info;
#ifdef CONFIG_RT_REGMAP
	struct rt_reg_data rrd = {0};

	return rt_regmap_reg_write(info->regmap_dev, &rrd, reg, data);
#else
	return rt5735_write_device(info->i2c, reg, 1, &data);
#endif
}

static struct chip_io_intf rt5735_io_intf = {
	.io_lock = rt5735_io_lock,
	.io_unlock = rt5735_io_unlock,
	.io_read = rt5735_io_read,
	.io_write = rt5735_io_write,
};

static const char *rt5735_get_chip_name(void *info)
{
	return "rt5735";
}

static const char *rt5735_get_compatible_name(void *info)
{
	return "rt5735";
}

static int rt5735_get_chip_i2c_channel(void *info)
{
	return RT5735_I2C_CHANNEL;
}

static int rt5735_hw_init_once(void *info)
{
	return 0;
}

static int rt5735_hw_init_before_enable(void *info)
{
	return 0;
}

static int rt5735_get_chip_capability(void *info)
{
	u32 capa = 0;

	capa |= RT_CAPABILITY_ENABLE;
	capa |= RT_CAPABILITY_VOLTAGE;
	capa |= RT_CAPABILITY_MODE;
	capa |= RT_CAPABILITY_DISCHARGE;
	return capa;
}

static int rt5735_set_vsel_enable(void *info, u8 vsel, u8 enable)
{
	struct rt5735_info *r_info = (struct rt5735_info *)info;
	u8 regaddr = 0;

	regaddr = (vsel ? RT5735_REG_PROGVSEL1 : RT5735_REG_PROGVSEL0);
	return (enable ? rt5735_set_bit(r_info, regaddr, RT5735_VSEL_ENMASK) :
			rt5735_clr_bit(r_info, regaddr, RT5735_VSEL_ENMASK));
}

static int rt5735_is_vsel_enabled(void *info, u8 vsel)
{
	u8 regaddr = 0;
	int ret = 0;

	regaddr = (vsel ? RT5735_REG_PROGVSEL1 : RT5735_REG_PROGVSEL0);
	ret = rt5735_io_read(info, regaddr);
	if (ret < 0)
		goto OUT_IS_ENABLED;
	ret = ((ret & RT5735_VSEL_ENMASK) ? 1 : 0);
OUT_IS_ENABLED:
	return ret;
}

static int rt5735_set_voltage(void *info, u8 vsel, int volt_uV)
{
	struct rt5735_info *r_info = (struct rt5735_info *)info;
	u8 regaddr = 0;
	u8 regval = 0;

	if (volt_uV < 600000 || volt_uV > 1393750)
		return -EINVAL;
	regaddr = (vsel ? RT5735_REG_PROGVSEL1 : RT5735_REG_PROGVSEL0);
	regval = (volt_uV - 600000) / 6250;
	regval <<= RT5735_VSEL_PROGSHFT;
	return rt5735_assign_bit(r_info, regaddr, regval, RT5735_VSEL_PROGMASK);
}

static int rt5735_get_voltage(void *info, u8 vsel)
{
	u8 regaddr = 0;
	int ret = 0;

	regaddr = (vsel ? RT5735_REG_PROGVSEL1 : RT5735_REG_PROGVSEL0);
	ret = rt5735_io_read(info, regaddr);
	if (ret < 0)
		goto OUT_GET_VOLT;
	ret &= RT5735_VSEL_PROGMASK;
	ret >>= RT5735_VSEL_PROGSHFT;
	ret = 600000 + (ret * 6250);
OUT_GET_VOLT:
	return ret;
}

static int rt5735_enable_force_pwm(void *info, u8 vsel, u8 enable)
{
	struct rt5735_info *r_info = (struct rt5735_info *)info;
	u8 regmask = 0;
	int ret = 0;

	regmask = (vsel ? RT5735_PWMSEL1_MASK : RT5735_PWMSEL0_MASK);
	if (enable)
		ret = rt5735_set_bit(r_info, RT5735_REG_COMMAND, regmask);
	else
		ret = rt5735_clr_bit(r_info, RT5735_REG_COMMAND, regmask);
	return ret;
}

static int rt5735_is_force_pwm_enabled(void *info, u8 vsel)
{
	u8 regmask = 0;
	int ret = 0;

	ret = rt5735_io_read(info, RT5735_REG_COMMAND);
	if (ret < 0)
		goto OUT_GET_MODE;
	regmask = (vsel ? RT5735_PWMSEL1_MASK : RT5735_PWMSEL0_MASK);
	ret = (ret & regmask) ? 1 : 0;
OUT_GET_MODE:
	return ret;
}

static int rt5735_enable_discharge(void *info, u8 enable)
{
	struct rt5735_info *r_info = (struct rt5735_info *)info;
	u8 regmask = RT5735_DISCHG_MASK;
	int ret = 0;

	if (enable)
		ret = rt5735_set_bit(r_info, RT5735_REG_PGOOD, regmask);
	else
		ret = rt5735_clr_bit(r_info, RT5735_REG_PGOOD, regmask);
	return ret;
}

static int rt5735_is_discharge_enabled(void *info)
{
	int ret = 0;

	ret = rt5735_io_read(info, RT5735_REG_PGOOD);
	if (ret < 0)
		goto OUT_IS_DISCHG;
	ret = (ret & RT5735_DISCHG_MASK) ? 1 : 0;
OUT_IS_DISCHG:
	return ret;
}

static void rt5735_dump_registers(void *r_info)
{
	struct rt5735_info *info = (struct rt5735_info *)r_info;
	int ret = 0;
	int i = 0;

	dev_err(info->dev, "%s ++\n", __func__);
	for (i = RT5735_REG_RANGE1_START; i <= RT5735_REG_RANGE1_END; i++) {
		ret = rt5735_io_read(r_info, i);
		dev_err(info->dev, "reg 0x%02x, value 0x%02x\n", i, ret);
	}
	for (i = RT5735_REG_RANGE2_START; i <= RT5735_REG_RANGE2_END; i++) {
		ret = rt5735_io_read(r_info, i);
		dev_err(info->dev, "reg 0x%02x, value 0x%02x\n", i, ret);
	}
	for (i = RT5735_REG_RANGE3_START; i <= RT5735_REG_RANGE3_END; i++) {
		ret = rt5735_io_read(r_info, i);
		dev_err(info->dev, "reg 0x%02x, value 0x%02x\n", i, ret);
	}
	dev_err(info->dev, "%s --\n", __func__);
}

static int rt5735_get_slew_rate(void *r_info, u8 up_down)
{
	int ret = 0;
	int slew_rate;

	if (up_down) {
		ret = rt5735_io_read(r_info, RT5735_REG_TIME);
		if (ret < 0)
			return ret;
		ret = (ret & RT5735_DVSUP_MASK) >> RT5735_DVSUP_SHFT;
		switch (ret) {
		case 0:
		default:
			slew_rate = 64;
			break;
		case 1:
			slew_rate = 16;
			break;
		case 2:
		case 6:
			slew_rate = 32;
			break;
		case 3:
		case 7:
			slew_rate = 8;
			break;
		case 4:
		case 5:
			slew_rate = 4;
			break;
		}
	} else {
		ret = rt5735_io_read(r_info, RT5735_REG_LIMCONF);
		if (ret < 0)
			return ret;
		ret = (ret & RT5735_DVSDOWN_MASK) >> RT5735_DVSDOWN_SHFT;
		switch (ret) {
		case 0:
		default:
			slew_rate = 32;
			break;
		case 1:
			slew_rate = 4;
			break;
		case 2:
			slew_rate = 8;
			break;
		case 3:
			slew_rate = 16;
			break;
		}
	}
	return slew_rate;
}

static int rt5735_set_slew_rate(void *r_info, u8 up_down, u8 val)
{
	int ret = 0;
	u8 regval = 0;

	if (up_down) {
		switch (val) {
		case 4:
			regval = 4;
			break;
		case 8:
			regval = 3;
		break;
		case 16:
			regval = 1;
			break;
		case 32:
			regval = 2;
			break;
		case 64:
		default:
			regval = 4;
			break;
		};
		ret = rt5735_assign_bit(r_info, RT5735_REG_TIME,
				regval << RT5735_DVSUP_SHFT, RT5735_DVSUP_MASK);
	} else {
		switch (val) {
		case 4:
			regval = 1;
			break;
		case 8:
			regval = 2;
			break;
		case 16:
			regval = 3;
			break;
		case 32:
		default:
			regval = 0;
			break;
		}
		ret = rt5735_assign_bit(r_info, RT5735_REG_LIMCONF,
			regval << RT5735_DVSDOWN_SHFT, RT5735_DVSDOWN_MASK);
	}
	return ret;
}

/* Used for RT5735A SDA low workaround, called in RTC driver */
static int rt5735_i2c7_switch_gpio_shutdown(void)
{
	pr_alert("%s +\n", __func__);

	/* emulator read addr 0x1c offset 0x23 */
	/* switch gpio mode */
	mt_set_gpio_mode(SCL7_GPIO, 0);
	mt_set_gpio_mode(SDA7_GPIO, 0);

	/* config output */
	mt_set_gpio_dir(SCL7_GPIO, 1);
	mt_set_gpio_dir(SDA7_GPIO, 1);

	/* output high */
	mt_set_gpio_out(SCL7_GPIO, 1);
	mt_set_gpio_out(SDA7_GPIO, 1);

	/* trigger start */
	mt_set_gpio_out(SDA7_GPIO, 0);
	udelay(2);

	mt_set_gpio_out(SCL7_GPIO, 0);

	/* write 0x1c, bit data == 0x0011 1000 */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);	/* bit7 */
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit6 */
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	/* add a delay 2us for data change at middle of scl low */
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit5 */
	/* mt_set_gpio_out(SDA7_GPIO, 1); */
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	mt_set_gpio_out(SDA7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit4 */

	/* mt_set_gpio_out(SDA7_GPIO, 1); */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	mt_set_gpio_out(SDA7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit3 */

	/* mt_set_gpio_out(SDA7_GPIO, 1); */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit2 */

	/* mt_set_gpio_out(SDA7_GPIO, 0); */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit1 */
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	mt_set_gpio_out(SDA7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1); /* bit0 */
	/* mt_set_gpio_out(SDA7_GPIO, 1); */

	/* slave ACK, Switch SDA input mode */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(1);
	mt_set_gpio_dir(SDA7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	/* udelay(2); */
	/* mt_set_gpio_out(SDA7_GPIO, 1); */
	mt_set_gpio_dir(SDA7_GPIO, 1);

	/*****************end first ack***********************************/
	/* write 0x23, 0x0010, 0011 */
	udelay(5);
	mt_set_gpio_out(SDA7_GPIO, 0); /* bit7:0 */
	udelay(2);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SDA7_GPIO, 1); /* bit5:1 */
	udelay(2);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SDA7_GPIO, 0); /* bit4:0 */
	udelay(2);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	/* mt_set_gpio_dir(SDA7_GPIO, 0); */
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SDA7_GPIO, 1); /* bit1:1 */
	udelay(2);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);

	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	/* end */

	/* slave ACK, set SDA input mode */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_dir(SDA7_GPIO, 0);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 1);
	mt_set_gpio_dir(SDA7_GPIO, 1);

	/*****************end second ack***********************************/
	/* restart */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 0);
	udelay(3);
	mt_set_gpio_out(SCL7_GPIO, 0);
	/*****************read command**********************************/
	/* read data 0x1001,1100 */

	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);

	/* finish first 4 bits */
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(2);
	mt_set_gpio_out(SDA7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);

	/* ack */
	udelay(2);
	mt_set_gpio_dir(SDA7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 0);
	udelay(5);
	mt_set_gpio_out(SCL7_GPIO, 1);

	pr_alert("%s -\n", __func__);
	return 0;
}

static const struct mtk_user_intf mtk_user_intf = {
	.get_chip_name = rt5735_get_chip_name,
	.get_compatible_name = rt5735_get_compatible_name,
	.get_chip_i2c_channel = rt5735_get_chip_i2c_channel,
	.get_chip_capability = rt5735_get_chip_capability,
	.hw_init_once = rt5735_hw_init_once,
	.hw_init_before_enable = rt5735_hw_init_before_enable,
	.set_vsel_enable = rt5735_set_vsel_enable,
	.is_vsel_enabled = rt5735_is_vsel_enabled,
	.set_voltage = rt5735_set_voltage,
	.get_voltage = rt5735_get_voltage,
	.enable_force_pwm = rt5735_enable_force_pwm,
	.is_force_pwm_enabled = rt5735_is_force_pwm_enabled,
	.enable_discharge = rt5735_enable_discharge,
	.is_discharge_enabled = rt5735_is_discharge_enabled,
	.dump_registers = rt5735_dump_registers,
	.get_slew_rate = rt5735_get_slew_rate,
	.set_slew_rate = rt5735_set_slew_rate,
	.i2c7_switch_gpio_shutdown = rt5735_i2c7_switch_gpio_shutdown,
};

static const struct mtk_gpuregulator_platform_data rtintf_platdata = {
	.user_intf = &mtk_user_intf,
	.io_intf = &rt5735_io_intf,
};

static int rt5735_parse_dt(struct device *dev,
		struct rt5735_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	u32 tmp = 0;

	if (of_property_read_bool(np, "rt,pgdvs"))
		pdata->pgdvs = 1;
	if (of_property_read_bool(np, "rt,pgdcdc"))
		pdata->pgdcdc = 1;
	if (of_property_read_u32(np, "rt,dvs_up", &tmp) >= 0)
		pdata->dvs_up = tmp;
	if (of_property_read_u32(np, "rt,dvs_down", &tmp) >= 0)
		pdata->dvs_down = tmp;
	if (of_property_read_bool(np, "rt,dvs_mdoe"))
		pdata->dvs_mode = 1;
	if (of_property_read_u32(np, "rt,ioc", &tmp) >= 0)
		pdata->ioc = tmp;
	if (of_property_read_u32(np, "rt,tpwth", &tmp) >= 0)
		pdata->tpwth = tmp;
	if (of_property_read_bool(np, "rt,rearm"))
		pdata->rearm = 1;
	return 0;
}

static int rt5735_parse_platform_data(struct rt5735_platform_data *pdata)
{
	if (pdata->pgdvs)
		rt5735_cfg_default[0] |= RT5735_PGDVS_MASK;
	if (pdata->pgdcdc)
		rt5735_cfg_default[0] |= RT5735_PGDCDC_MASK;
	rt5735_cfg_default[1] &= ~(RT5735_DVSUP_MASK);
	rt5735_cfg_default[1] |= ((pdata->dvs_up) << RT5735_DVSUP_SHFT);
	if (pdata->dvs_mode)
		rt5735_cfg_default[2] |= RT5735_DVSMODE_MASK;
	rt5735_cfg_default[3] &= ~(RT5735_DVSDOWN_MASK);
	rt5735_cfg_default[3] |= ((pdata->dvs_down) << RT5735_DVSDOWN_SHFT);
	rt5735_cfg_default[3] &= ~(RT5735_IOC_MASK);
	rt5735_cfg_default[3] |= ((pdata->ioc) << RT5735_IOC_SHFT);
	rt5735_cfg_default[3] &= ~(RT5735_TPWTH_MASK);
	rt5735_cfg_default[3] |= ((pdata->tpwth) << RT5735_TPWTH_SHFT);
	if (pdata->rearm)
		rt5735_cfg_default[3] |= RT5735_REARM_MASK;
	return 0;
}

static int rt5735_chip_init(struct rt5735_info *info)
{
	int ret = 0;

	ret = rt5735_write_device(info->i2c, RT5735_REG_PGOOD, 3,
			rt5735_cfg_default);
	if (ret < 0)
		return ret;
	return rt5735_write_device(info->i2c, RT5735_REG_LIMCONF, 1,
			&rt5735_cfg_default[3]);
}

/* Used for RT5735A SDA low workaround */
static int rt5735_i2c7_switch_gpio_bootup(void)
{
	int i;

	pr_alert("%s +\n", __func__);

	/* switch gpio mode */
	mt_set_gpio_mode(SCL7_GPIO, 0);
	mt_set_gpio_mode(SDA7_GPIO, 0);

	/* dir, scl output, sda input */
	mt_set_gpio_dir(SCL7_GPIO, 1);
	mt_set_gpio_dir(SDA7_GPIO, 0);

	/* level */
	mt_set_gpio_out(SCL7_GPIO, 1);

	udelay(20);

	/* output 9 cycle */
	for (i = 0; i < 9; i++) {
		udelay(5);
		mt_set_gpio_out(SCL7_GPIO, 0);
		udelay(5);
		mt_set_gpio_out(SCL7_GPIO, 1);
	}

	/* switch i2c mode */
	udelay(5);
	mt_set_gpio_mode(SCL7_GPIO, 1);
	mt_set_gpio_mode(SDA7_GPIO, 1);

	pr_alert("%s -\n", __func__);
	return 0;
}

static int rt5735_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct rt5735_platform_data *pdata = client->dev.platform_data;
	struct rt5735_info *info;
	bool use_dt = client->dev.of_node;
	int ret = 0;
	u8 regval = 0;

	pr_alert("%s\n", __func__);

	if (is_fan53555_exist() == 1) {
		pr_err("%s: fan53555 is detected\n", __func__);
		return -ENODEV;
	}

	/* Fix SDA7 remains low issue */
	rt5735_i2c7_switch_gpio_bootup();

	/* check product id for exist */
	ret = rt5735_read_device(client, RT5735_REG_PID, 1, &regval);
	pr_info("regval = 0x%02x\n, ret = %d", regval, ret);

	if (ret < 0) {
		/*
		 * No FAN53555 is detected and can't successfully probe RT5735A after workaround.
		 * Assume this workaround can't work, try next fix if exist.
		 */
		pr_err("%s: can't probe I2C device\n", __func__);
		battery_disable_batfet();
	} else if (regval != RT5735_PRODUCT_ID) {
		pr_err("%s chip not match\n", __func__);
		return -ENODEV;
	}

	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	if (use_dt && !pdata) {
		pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			ret = -ENOMEM;
			goto OUT_ALLOC_PDATA;
		}
		rt5735_parse_dt(&client->dev, pdata);
		client->dev.platform_data = pdata;
	}

	info->i2c = client;
	info->dev = &client->dev;
	mutex_init(&info->vario_lock);
	i2c_set_clientdata(client, info);

	rt5735_parse_platform_data(pdata);
	ret = rt5735_chip_init(info);
	if (ret < 0)
		goto OUT_CHIP_INIT;

#ifdef CONFIG_RT_REGMAP
	info->regmap_dev = rt_regmap_device_register(&rt5735_regmap_props,
			&rt5735_regmap_fops,
			&client->dev,
			client,
			info);
	if (!info->regmap_dev) {
		ret = -EINVAL;
		goto OUT_REGMAP_DEV;
	}
#endif /* #ifdef CONFIG_RT_REGMAP */
	info->pdev = platform_device_register_data(&client->dev,
			"mtk_gpuregulator_intf",
			-1, &rtintf_platdata,
			sizeof(rtintf_platdata));
	if (!info->pdev)
		goto OUT_PDEV;

	mt_gpufreq_ext_ic_init();

	dev_info(&client->dev, "rt5735 successfully probed\n");

	return 0;
OUT_PDEV:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
OUT_REGMAP_DEV:
#endif /* #ifdef CONFIG_RT_REGMAP */
OUT_CHIP_INIT:
	mutex_destroy(&info->vario_lock);
	if (use_dt)
		devm_kfree(&client->dev, pdata);
OUT_ALLOC_PDATA:
	devm_kfree(&client->dev, info);
	return ret;
}

static int rt5735_i2c_remove(struct i2c_client *client)
{
	struct rt5735_info *info = i2c_get_clientdata(client);

	platform_device_unregister(info->pdev);
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
#endif /* #ifdef CONFIG_RT_REGMAP */
	mutex_destroy(&info->vario_lock);
	dev_info(&client->dev, "rt5735 successfully removed\n");
	return 0;
}

static int rt5735_i2c_suspend(struct device *dev)
{
	return 0;
}

static int rt5735_i2c_resume(struct device *dev)
{
	return 0;
}

static SIMPLE_DEV_PM_OPS(rt5735_pm_ops, rt5735_i2c_suspend, rt5735_i2c_resume);

static const struct i2c_device_id rt5735_device_id[] = {
	{ "rt5735-regulator", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, rt5735_device_id);

static const struct of_device_id rt5735_of_match_id[] = {
	{ .compatible = "rt,rt5735-regulator", },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, rt5735_of_device_id);

static struct i2c_driver rt5735_driver = {
	.driver = {
		.name = "rt5735-regulator",
		.of_match_table = rt5735_of_match_id,
		.pm = &rt5735_pm_ops,
	},
	.probe = rt5735_i2c_probe,
	.remove = rt5735_i2c_remove,
	.id_table = rt5735_device_id,
};

static int __init rt5735_init(void)
{
	return i2c_add_driver(&rt5735_driver);
}

static void __exit rt5735_exit(void)
{
	i2c_del_driver(&rt5735_driver);
}

device_initcall_sync(rt5735_init);
module_exit(rt5735_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Regulator Driver for Richtek RT5735");
MODULE_AUTHOR("CY Huang <cy_huang@richtek.com>");
MODULE_VERSION("1.0.0_M");

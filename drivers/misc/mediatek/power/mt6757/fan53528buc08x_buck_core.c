/*
 *  fan53528_buck_core.c
 *  Buck Core Driver for FAN53528
 *
 *  Copyright (C) 2015 Richtek Technology Corp.
 *  Sakya <jeff_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#include "fan53528buc08x_system.h"
#include "fan53528buc08x_buck_core.h"

#define FAN53528_I2C_CHANNEL           0

static int fan53528_assign_bit(unsigned char reg,
		unsigned char mask, unsigned char data)
{
	unsigned char tmp = 0, regval = 0;
	int ret = 0;

	fan53528_intf.lock();
	ret = fan53528_intf.io_read(reg, &regval);
	if (ret < 0) {
		FAN53528_ERR("%s read fail reg0x%02x data0x%02x\n",
				__func__, reg, data);
		goto OUT_ASSIGN;
	}
	tmp = ((regval & 0xff) & ~mask);
	tmp |= (data & mask);
	ret = fan53528_intf.io_write(reg, tmp);
	if (ret < 0)
		FAN53528_ERR("%s write fail reg0x%02x data0x%02x\n",
				__func__, reg, tmp);
OUT_ASSIGN:
	fan53528_intf.unlock();
	return  ret;
}

#define fan53528_set_bit(reg, mask) \
	fan53528_assign_bit(reg, mask, mask)

#define fan53528_clr_bit(reg, mask) \
	fan53528_assign_bit(reg, mask, 0x00)

int fan53528_get_chip_i2c_channel(void)
{
	return FAN53528_I2C_CHANNEL;
}

int fan53528_set_enable(unsigned char enable)
{
	unsigned char addr = 0;

	FAN53528_PRINT("%s\n", __func__);
	addr =  FAN53528_REG_VSEL0;
	return enable ? fan53528_set_bit(addr, FAN53528_VSELEN_MASK) :
		fan53528_clr_bit(addr, FAN53528_VSELEN_MASK);
}

int fan53528_is_enabled(void)
{
	unsigned char addr = 0, val = 0;
	int ret = 0;

	addr = FAN53528_REG_VSEL0;
	ret = fan53528_intf.io_read(addr, &val);
	if (ret < 0)
		return ret;
	ret = ((val & FAN53528_VSELEN_MASK) ? 1 : 0);
	return ret;
}

int fan53528_set_voltage(int volt_uv)
{
	unsigned char addr = 0;
	unsigned char regval = 0;

	FAN53528_PRINT("%s, volt_uv = %d\n", __func__, volt_uv);
	if (volt_uv < 350000 || volt_uv > 1143750)
		return -1;
	addr = FAN53528_REG_VSEL0;
	regval = (volt_uv - 350000) / 6250;
	regval <<= FAN53528_VOUT_SHIFT;
	return fan53528_assign_bit(addr, FAN53528_VOUT_MASK,
					regval << FAN53528_VOUT_SHIFT);
}

int fan53528_get_voltage(void)
{
	unsigned char addr = 0, regval = 0;
	int ret = 0;

	addr = FAN53528_REG_VSEL0;
	ret = fan53528_intf.io_read(addr, &regval);
	if (ret < 0)
		return ret;

	ret = (regval & FAN53528_VOUT_MASK) >> FAN53528_VOUT_SHIFT;
	ret = (ret * 6250) + 350000;
	return ret;
}

int fan53528_enable_force_pwm(unsigned char enable)
{
	unsigned char regmask = 0;
	int ret = 0;

	FAN53528_PRINT("%s\n", __func__);
	regmask = FAN53528_PWMSEL0_MASK;
	if (enable)
		ret = fan53528_set_bit(FAN53528_REG_CONTROL, regmask);
	else
		ret = fan53528_clr_bit(FAN53528_REG_CONTROL, regmask);
	return ret;
}

int fan53528_is_force_pwm_enabled(void)
{
	unsigned char regmask = 0, regval = 0;
	int ret = 0;

	regmask = FAN53528_PWMSEL0_MASK;
	ret = fan53528_intf.io_read(FAN53528_REG_CONTROL, &regval);
	if (ret < 0)
		return ret;
	ret = (regval & regmask) ? 1 : 0;
	return ret;
}

int fan53528_enable_discharge(unsigned char enable)
{
	unsigned char regmask = FAN53528_DISCHG_MASK;
	int ret = 0;

	if (enable)
		ret = fan53528_set_bit(FAN53528_REG_CONTROL, regmask);
	else
		ret = fan53528_clr_bit(FAN53528_REG_CONTROL, regmask);
	return ret;
}

int fan53528_is_discharge_enabled(void)
{
	unsigned char regval = 0;
	int ret = 0;

	ret = fan53528_intf.io_read(FAN53528_REG_CONTROL, &regval);
	if (ret < 0)
		return ret;
	ret = (regval & FAN53528_DISCHG_MASK) ? 1 : 0;
	return ret;
}

/* fan53528 only can set low-to-high slew rate */
int fan53528_set_slew_rate(unsigned char up_down, unsigned char mv_us)
{
	unsigned char regval = 0;
	int ret = 0;

	switch (mv_us) {
	case 0:
	default:
		regval = 7;
		break;
	case 1:
		regval = 6;
		break;
	case 2:
		regval = 5;
		break;
	case 4:
		regval = 4;
		break;
	case 8:
		regval = 3;
		break;
	case 16:
		regval = 2;
		break;
	case 32:
		regval = 1;
		break;
	case 64:
		regval = 0;
		break;
	}

	ret = fan53528_assign_bit(FAN53528_REG_CONTROL,
		FAN53528_SLEWRATE_MASK, regval << FAN53528_SLEWRATE_SHIFT);
	return ret;
}

int fan53528_get_slew_rate(unsigned char up_down)
{
	unsigned char regval = 0;
	int ret = 0, slew_rate = 0;

	ret = fan53528_intf.io_read(FAN53528_REG_CONTROL, &regval);
	if (ret < 0)
		return ret;
	ret = (regval & FAN53528_SLEWRATE_MASK) >> FAN53528_SLEWRATE_SHIFT;
	switch (ret) {
	case 0:
		slew_rate = 64;
		break;
	case 1:
		slew_rate = 32;
		break;
	case 2:
		slew_rate = 16;
		break;
	case 3:
		slew_rate = 8;
		break;
	case 4:
		slew_rate = 4;
		break;
	case 5:
		slew_rate = 2;
		break;
	case 6:
		slew_rate = 1;
		break;
	case 7:
		slew_rate = 0;
		break;
}
	return slew_rate;
}

void fan53528_dump_registers(void)
{
	int i;
	unsigned char regval = 0;

	FAN53528_PRINT("%s start\n", __func__);
	for (i = FAN53528_REG_VSEL0; i <= FAN53528_REG_MONITOR; i++) {
		fan53528_intf.io_read(i, &regval);
		FAN53528_PRINT("reg0x%02x = 0x%02x\n", i, regval);
	}
	FAN53528_PRINT("%s end\n", __func__);
}

#if 0
struct buck_user_intf fan53528_user_intf = {
	.get_chip_i2c_channel = fan53528_get_chip_i2c_channel,
	.set_enable = fan53528_set_enable,
	.is_enabled = fan53528_is_enabled,
	.set_voltage = fan53528_set_voltage,
	.get_voltage = fan53528_get_voltage,
	.enable_force_pwm = fan53528_enable_force_pwm,
	.is_force_pwm_enabled = fan53528_is_force_pwm_enabled,
	.enable_discharge = fan53528_enable_discharge,
	.is_discharge_enabled = fan53528_is_discharge_enabled,
	.get_slew_rate = fan53528_get_slew_rate,
	.set_slew_rate = fan53528_set_slew_rate,
	.dump_registers = fan53528_dump_registers,
};

struct mtk_buck fan53528_buck = {
	.name = "fan53528",
	.i2c_channel = 0,
	.intf = &fan53528_user_intf,
};

int mtk_buck_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	ret = mtk_buck_register(&fan53528_buck);
	if (ret < 0)
		pr_err("%s regitser fan53528 buck fail\n", __func__);
	return ret;
}
#endif

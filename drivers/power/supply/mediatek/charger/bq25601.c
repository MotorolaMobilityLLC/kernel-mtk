/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#define pr_fmt(fmt) "[bq25601]:%s: " fmt, __func__

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <mt-plat/charger_type.h>

#include "bq25601.h"
#include "bq25601_reg.h"
#include "mtk_charger_intf.h"

enum bq25601_part_no {
	PN_BQ25600,
	PN_BQ25600D,
	PN_BQ25601,
	PN_BQ25601D,
};

static int pn_data[] = {
//		[PN_BQ25600] = 0x00, [PN_BQ25600D] = 0x01, [PN_BQ25601] = 0x02,
		[PN_BQ25600] = 0x00, [PN_BQ25600D] = 0x01, [PN_BQ25601] = 0x09,
		[PN_BQ25601D] = 0x07,
};

static char *pn_str[] = {
		[PN_BQ25600] = "bq25600", [PN_BQ25600D] = "bq25600d",
		[PN_BQ25601] = "bq25601", [PN_BQ25601D] = "bq25601d",
};

#include <ontim/ontim_dev_dgb.h>
static  char charge_ic_vendor_name[50]="BQ25601";
DEV_ATTR_DECLARE(charge_ic)
DEV_ATTR_DEFINE("vendor",charge_ic_vendor_name)
DEV_ATTR_DECLARE_END;
ONTIM_DEBUG_DECLARE_AND_INIT(charge_ic,charge_ic,8);

struct bq25601 {
	struct device *dev;
	struct i2c_client *client;

	enum bq25601_part_no part_no;
	int revision;

	const char *chg_dev_name;
	const char *eint_name;

	bool chg_det_enable;

	enum charger_type chg_type;

	int status;
	int irq;

	struct mutex i2c_rw_lock;

	bool charge_enabled; /* Register bit status */
	bool power_good;

	struct bq25601_platform_data *platform_data;
	struct charger_device *chg_dev;

	struct power_supply *psy;
};

static const struct charger_properties bq25601_chg_props = {
	.alias_name = "bq25601",
};

#define BQ25601_SLAVE_ADDR 0x6b
static int __bq25601_read_reg(struct bq25601 *bq, u8 reg, u8 *data)
{
	int ret = 0;
	struct i2c_adapter *adap = bq->client->adapter;
	struct i2c_msg msg[2];
	u8 *w_buf = NULL;
	u8 *r_buf = NULL;

	memset(msg, 0, 2 * sizeof(struct i2c_msg));

	w_buf = kzalloc(1, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;
	r_buf = kzalloc(1, GFP_KERNEL);
	if (r_buf == NULL)
		return -1;

	*w_buf = reg;

	msg[0].addr = BQ25601_SLAVE_ADDR;//new_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = w_buf;

	msg[1].addr = BQ25601_SLAVE_ADDR;//new_client->addr;
	msg[1].flags = 1;
	msg[1].len = 1;
	msg[1].buf = r_buf;

	ret = i2c_transfer(adap, msg, 2);

	memcpy(data, r_buf, 1);

	kfree(w_buf);
	kfree(r_buf);
	return ret;

}

static int __bq25601_write_reg(struct bq25601 *bq, int reg, u8 val)
{
	int ret = 0;
	struct i2c_adapter *adap = bq->client->adapter;
	struct i2c_msg msg;
	u8 *w_buf = NULL;

	memset(&msg, 0, sizeof(struct i2c_msg));

	w_buf = kzalloc(2, GFP_KERNEL);
	if (w_buf == NULL)
		return -1;

	w_buf[0] = reg;
	memcpy(w_buf + 1, &val, 1);

	msg.addr = BQ25601_SLAVE_ADDR;//new_client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = w_buf;

	ret = i2c_transfer(adap, &msg, 1);

	kfree(w_buf);
	return ret;

}

static int bq25601_read_byte(struct bq25601 *bq, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq25601_read_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);
	if (ret<0)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	else
		ret =0;

	return ret;
}

static int bq25601_write_byte(struct bq25601 *bq, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq25601_write_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	if (ret<0)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	else
		ret =0;

	return ret;
}

static int bq25601_update_bits(struct bq25601 *bq, u8 reg, u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq25601_read_reg(bq, reg, &tmp);
	if (ret<0) {
		pr_info("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __bq25601_write_reg(bq, reg, tmp);
	if (ret<0)
		pr_info("Failed: reg=%02X, ret=%d\n", reg, ret);
	else
		ret =0;

out:
	mutex_unlock(&bq->i2c_rw_lock);
	return ret;
}

static int bq25601_enable_otg(struct bq25601 *bq)
{
	u8 val = REG01_OTG_ENABLE << REG01_OTG_CONFIG_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_01, REG01_OTG_CONFIG_MASK,
				   val);
}

static int bq25601_disable_otg(struct bq25601 *bq)
{
	u8 val = REG01_OTG_DISABLE << REG01_OTG_CONFIG_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_01, REG01_OTG_CONFIG_MASK,
				   val);
}

static int bq25601_enable_charger(struct bq25601 *bq)
{
	int ret;
	u8 val = REG01_CHG_ENABLE << REG01_CHG_CONFIG_SHIFT;

	ret = bq25601_update_bits(bq, BQ25601_REG_01, REG01_CHG_CONFIG_MASK,
				  val);

	return ret;
}

static int bq25601_disable_charger(struct bq25601 *bq)
{
	int ret;
	u8 val = REG01_CHG_DISABLE << REG01_CHG_CONFIG_SHIFT;

	ret = bq25601_update_bits(bq, BQ25601_REG_01, REG01_CHG_CONFIG_MASK,
				  val);
	return ret;
}

int bq25601_set_chargecurrent(struct bq25601 *bq, int curr)
{
	u8 ichg;

	if (curr < REG02_ICHG_BASE)
		curr = REG02_ICHG_BASE;

	ichg = (curr - REG02_ICHG_BASE) / REG02_ICHG_LSB;
	return bq25601_update_bits(bq, BQ25601_REG_02, REG02_ICHG_MASK,
				   ichg << REG02_ICHG_SHIFT);
}

int bq25601_set_term_current(struct bq25601 *bq, int curr)
{
	u8 iterm;

	if (curr < REG03_ITERM_BASE)
		curr = REG03_ITERM_BASE;

	iterm = (curr - REG03_ITERM_BASE) / REG03_ITERM_LSB;

	return bq25601_update_bits(bq, BQ25601_REG_03, REG03_ITERM_MASK,
				   iterm << REG03_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(bq25601_set_term_current);

int bq25601_set_prechg_current(struct bq25601 *bq, int curr)
{
	u8 iprechg;

	if (curr < REG03_IPRECHG_BASE)
		curr = REG03_IPRECHG_BASE;

	iprechg = (curr - REG03_IPRECHG_BASE) / REG03_IPRECHG_LSB;

	return bq25601_update_bits(bq, BQ25601_REG_03, REG03_IPRECHG_MASK,
				   iprechg << REG03_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(bq25601_set_prechg_current);

int bq25601_set_chargevolt(struct bq25601 *bq, int volt)
{
	u8 val;

	if (volt < REG04_VREG_BASE)
		volt = REG04_VREG_BASE;

	val = (volt - REG04_VREG_BASE) / REG04_VREG_LSB;
	return bq25601_update_bits(bq, BQ25601_REG_04, REG04_VREG_MASK,
				   val << REG04_VREG_SHIFT);
}

int bq25601_set_input_volt_limit(struct bq25601 *bq, int volt)
{
	u8 val;

	if (volt < REG06_VINDPM_BASE)
		volt = REG06_VINDPM_BASE;

	val = (volt - REG06_VINDPM_BASE) / REG06_VINDPM_LSB;
	return bq25601_update_bits(bq, BQ25601_REG_06, REG06_VINDPM_MASK,
				   val << REG06_VINDPM_SHIFT);
}

int bq25601_set_input_current_limit(struct bq25601 *bq, int curr)
{
	u8 val;

	if (curr < REG00_IINLIM_BASE)
		curr = REG00_IINLIM_BASE;

	val = (curr - REG00_IINLIM_BASE) / REG00_IINLIM_LSB;
	return bq25601_update_bits(bq, BQ25601_REG_00, REG00_IINLIM_MASK,
				   val << REG00_IINLIM_SHIFT);
}

int bq25601_set_watchdog_timer(struct bq25601 *bq, u8 timeout)
{
	u8 temp;

	temp = (u8)(((timeout - REG05_WDT_BASE) / REG05_WDT_LSB)
		    << REG05_WDT_SHIFT);

	return bq25601_update_bits(bq, BQ25601_REG_05, REG05_WDT_MASK, temp);
}
EXPORT_SYMBOL_GPL(bq25601_set_watchdog_timer);

int bq25601_disable_watchdog_timer(struct bq25601 *bq)
{
	u8 val = REG05_WDT_DISABLE << REG05_WDT_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_05, REG05_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(bq25601_disable_watchdog_timer);

int bq25601_reset_watchdog_timer(struct bq25601 *bq)
{
	u8 val = REG01_WDT_RESET << REG01_WDT_RESET_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_01, REG01_WDT_RESET_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_reset_watchdog_timer);

int bq25601_reset_chip(struct bq25601 *bq)
{
	int ret;
	u8 val = REG0B_REG_RESET << REG0B_REG_RESET_SHIFT;

	ret = bq25601_update_bits(bq, BQ25601_REG_0B, REG0B_REG_RESET_MASK,
				  val);
	return ret;
}
EXPORT_SYMBOL_GPL(bq25601_reset_chip);

int bq25601_enter_hiz_mode(struct bq25601 *bq)
{
	u8 val = REG00_HIZ_ENABLE << REG00_ENHIZ_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_00, REG00_ENHIZ_MASK, val);
}
EXPORT_SYMBOL_GPL(bq25601_enter_hiz_mode);

int bq25601_exit_hiz_mode(struct bq25601 *bq)
{

	u8 val = REG00_HIZ_DISABLE << REG00_ENHIZ_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_00, REG00_ENHIZ_MASK, val);
}
EXPORT_SYMBOL_GPL(bq25601_exit_hiz_mode);

int bq25601_get_hiz_mode(struct bq25601 *bq, u8 *state)
{
	u8 val;
	int ret;

	ret = bq25601_read_byte(bq, BQ25601_REG_00, &val);
	if (ret)
		return ret;
	*state = (val & REG00_ENHIZ_MASK) >> REG00_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(bq25601_get_hiz_mode);

static int bq25601_enable_term(struct bq25601 *bq, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = REG05_TERM_ENABLE << REG05_EN_TERM_SHIFT;
	else
		val = REG05_TERM_DISABLE << REG05_EN_TERM_SHIFT;

	ret = bq25601_update_bits(bq, BQ25601_REG_05, REG05_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(bq25601_enable_term);

int bq25601_set_boost_current(struct bq25601 *bq, int curr)
{
	u8 val;

	val = (curr <= BOOSTI_500 ? REG02_BOOST_LIM_0P5A : REG02_BOOST_LIM_1P2A);

	return bq25601_update_bits(bq, BQ25601_REG_02, REG02_BOOST_LIM_MASK,
				   val << REG02_BOOST_LIM_SHIFT);
}

int bq25601_set_boost_voltage(struct bq25601 *bq, int volt)
{
	u8 val;

	if (volt == BOOSTV_4850)
		val = REG06_BOOSTV_4P85V;
	else if (volt == BOOSTV_5150)
		val = REG06_BOOSTV_5P15V;
	else if (volt == BOOSTV_5300)
		val = REG06_BOOSTV_5P3V;
	else
		val = REG06_BOOSTV_5V;

	return bq25601_update_bits(bq, BQ25601_REG_06, REG06_BOOSTV_MASK,
				   val << REG06_BOOSTV_SHIFT);
}
EXPORT_SYMBOL_GPL(bq25601_set_boost_voltage);

static int bq25601_set_acovp_threshold(struct bq25601 *bq, int volt)
{
	u8 val;

	if (volt == VAC_OVP_14000)
		val = REG06_OVP_14P0V;
	else if (volt == VAC_OVP_10500)
		val = REG06_OVP_10P5V;
	else if (volt == VAC_OVP_6500)
		val = REG06_OVP_6P5V;
	else
		val = REG06_OVP_5P5V;

	return bq25601_update_bits(bq, BQ25601_REG_06, REG06_OVP_MASK,
				   val << REG06_OVP_SHIFT);
}
EXPORT_SYMBOL_GPL(bq25601_set_acovp_threshold);

static int bq25601_set_stat_ctrl(struct bq25601 *bq, int ctrl)
{
	u8 val;

	val = ctrl;

	return bq25601_update_bits(bq, BQ25601_REG_00, REG00_STAT_CTRL_MASK,
				   val << REG00_STAT_CTRL_SHIFT);
}

static int bq25601_set_int_mask(struct bq25601 *bq, int mask)
{
	u8 val;

	val = mask;

	return bq25601_update_bits(bq, BQ25601_REG_0A, REG0A_INT_MASK_MASK,
				   val << REG0A_INT_MASK_SHIFT);
}

static int bq25601_enable_batfet(struct bq25601 *bq)
{
	const u8 val = REG07_BATFET_ON << REG07_BATFET_DIS_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_enable_batfet);

static int bq25601_disable_batfet(struct bq25601 *bq)
{
	const u8 val = REG07_BATFET_OFF << REG07_BATFET_DIS_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_disable_batfet);

static int bq25601_set_batfet_delay(struct bq25601 *bq, uint8_t delay)
{
	u8 val;

	if (delay == 0)
		val = REG07_BATFET_DLY_0S;
	else
		val = REG07_BATFET_DLY_10S;

	val <<= REG07_BATFET_DLY_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_07, REG07_BATFET_DLY_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_set_batfet_delay);

static int bq25601_enable_safety_timer(struct bq25601 *bq)
{
	const u8 val = REG05_CHG_TIMER_ENABLE << REG05_EN_TIMER_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_enable_safety_timer);

static int bq25601_disable_safety_timer(struct bq25601 *bq)
{
	const u8 val = REG05_CHG_TIMER_DISABLE << REG05_EN_TIMER_SHIFT;

	return bq25601_update_bits(bq, BQ25601_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq25601_disable_safety_timer);

static struct bq25601_platform_data *bq25601_parse_dt(struct device_node *np,
						      struct bq25601 *bq)
{
	int ret;
	struct bq25601_platform_data *pdata;

	pdata = devm_kzalloc(bq->dev, sizeof(struct bq25601_platform_data),
			     GFP_KERNEL);
	if (!pdata)
		return NULL;

	if (of_property_read_string(np, "charger_name", &bq->chg_dev_name) <
	    0) {
		bq->chg_dev_name = "primary_chg";
		pr_info("no charger name\n");
	}

	if (of_property_read_string(np, "eint_name", &bq->eint_name) < 0) {
		bq->eint_name = "chr_stat";
		pr_info("no eint name\n");
	}

	bq->chg_det_enable =
		of_property_read_bool(np, "ti,bq25601,charge-detect-enable");

	ret = of_property_read_u32(np, "ti,bq25601,usb-vlim", &pdata->usb.vlim);
	if (ret) {
		pdata->usb.vlim = 4500;
		pr_info("Failed to read node of ti,bq25601,usb-vlim\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,usb-ilim", &pdata->usb.ilim);
	if (ret) {
		pdata->usb.ilim = 2000;
		pr_info("Failed to read node of ti,bq25601,usb-ilim\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,usb-vreg", &pdata->usb.vreg);
	if (ret) {
		pdata->usb.vreg = 4200;
		pr_info("Failed to read node of ti,bq25601,usb-vreg\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,usb-ichg", &pdata->usb.ichg);
	if (ret) {
		pdata->usb.ichg = 2000;
		pr_info("Failed to read node of ti,bq25601,usb-ichg\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,stat-pin-ctrl",
				   &pdata->statctrl);
	if (ret) {
		pdata->statctrl = 0;
		pr_info("Failed to read node of ti,bq25601,stat-pin-ctrl\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,precharge-current",
				   &pdata->iprechg);
	if (ret) {
		pdata->iprechg = 180;
		pr_info("Failed to read node of ti,bq25601,precharge-current\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,termination-current",
				   &pdata->iterm);
	if (ret) {
		pdata->iterm = 180;
		pr_info("Failed to read node of ti,bq25601,termination-current\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,boost-voltage",
				   &pdata->boostv);
	if (ret) {
		pdata->boostv = 5000;
		pr_info("Failed to read node of ti,bq25601,boost-voltage\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,boost-current",
				   &pdata->boosti);
	if (ret) {
		pdata->boosti = 1200;
		pr_info("Failed to read node of ti,bq25601,boost-current\n");
	}

	ret = of_property_read_u32(np, "ti,bq25601,vac-ovp-threshold",
				   &pdata->vac_ovp);
	if (ret) {
		pdata->vac_ovp = 6500;
		pr_info("Failed to read node of ti,bq25601,vac-ovp-threshold\n");
	}

	return pdata;
}

#if 0
static int bq25601_get_charger_type(struct bq25601 *bq, enum charger_type *type)
{
	int ret;

	u8 reg_val = 0;
	int vbus_stat = 0;
	enum charger_type chg_type = CHARGER_UNKNOWN;

	ret = bq25601_read_byte(bq, BQ25601_REG_08, &reg_val);

	if (ret)
		return ret;

	vbus_stat = (reg_val & REG08_VBUS_STAT_MASK);
	vbus_stat >>= REG08_VBUS_STAT_SHIFT;

	switch (vbus_stat) {

	case REG08_VBUS_TYPE_NONE:
		chg_type = CHARGER_UNKNOWN;
		break;
	case REG08_VBUS_TYPE_SDP:
		chg_type = STANDARD_HOST;
		break;
	case REG08_VBUS_TYPE_CDP:
		chg_type = CHARGING_HOST;
		break;
	case REG08_VBUS_TYPE_DCP:
		chg_type = STANDARD_CHARGER;
		break;
	case REG08_VBUS_TYPE_UNKNOWN:
		chg_type = NONSTANDARD_CHARGER;
		break;
	case REG08_VBUS_TYPE_NON_STD:
		chg_type = NONSTANDARD_CHARGER;
		break;
	default:
		chg_type = NONSTANDARD_CHARGER;
		break;
	}

	*type = chg_type;

	return 0;
}

static int bq25601_inform_charger_type(struct bq25601 *bq)
{
	int ret = 0;
	union power_supply_propval propval;

	if (!bq->psy) {
		bq->psy = power_supply_get_by_name("charger");
		if (!bq->psy)
			return -ENODEV;
	}

	if (bq->chg_type != CHARGER_UNKNOWN)
		propval.intval = 1;
	else
		propval.intval = 0;

	ret = power_supply_set_property(bq->psy, POWER_SUPPLY_PROP_ONLINE,
					&propval);

	if (ret < 0)
		pr_notice("inform power supply online failed:%d\n", ret);

	propval.intval = bq->chg_type;

	ret = power_supply_set_property(bq->psy,
					POWER_SUPPLY_PROP_CHARGE_TYPE,
					&propval);

	if (ret < 0)
		pr_notice("inform power supply charge type failed:%d\n", ret);

	return ret;
}

static irqreturn_t bq25601_irq_handler(int irq, void *data)
{
	int ret;
	u8 reg_val;
	bool prev_pg;
	enum charger_type prev_chg_type;

	ret = bq25601_read_byte(bq, BQ25601_REG_08, &reg_val);
	if (ret)
		return IRQ_HANDLED;

	prev_pg = bq->power_good;

	bq->power_good = !!(reg_val & REG08_PG_STAT_MASK);

	if (!prev_pg && bq->power_good)
		pr_notice("adapter/usb inserted\n");
	else if (prev_pg && !bq->power_good)
		pr_notice("adapter/usb removed\n");

	prev_chg_type = bq->chg_type;

	ret = bq25601_get_charger_type(bq, &bq->chg_type);
	if (!ret && prev_chg_type != bq->chg_type && bq->chg_det_enable)
		bq25601_inform_charger_type(bq);

	return IRQ_HANDLED;
}

static int bq25601_register_interrupt(struct bq25601 *bq)
{
	int ret = 0;
	struct device_node *np;

	np = of_find_node_by_name(NULL, bq->eint_name);
	if (np) {
		bq->irq = irq_of_parse_and_map(np, 0);
	} else {
		pr_info("couldn't get irq node\n");
		return -ENODEV;
	}

	pr_info("irq = %d\n", bq->irq);

	ret = devm_request_threaded_irq(bq->dev, bq->irq, NULL,
					bq25601_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					bq->eint_name, bq);
	if (ret < 0) {
		pr_info("request thread irq failed:%d\n", ret);
		return ret;
	}

	enable_irq_wake(bq->irq);

	return 0;
}
#endif

static int bq25601_init_device(struct bq25601 *bq)
{
	int ret;

	bq25601_disable_watchdog_timer(bq);

	ret = bq25601_set_stat_ctrl(bq, bq->platform_data->statctrl);
	if (ret)
		pr_info("Failed to set stat pin control mode, ret = %d\n", ret);

	ret = bq25601_set_prechg_current(bq, bq->platform_data->iprechg);
	if (ret)
		pr_info("Failed to set prechg current, ret = %d\n", ret);

	ret = bq25601_set_term_current(bq, bq->platform_data->iterm);
	if (ret)
		pr_info("Failed to set termination current, ret = %d\n", ret);

	ret = bq25601_set_boost_voltage(bq, bq->platform_data->boostv);
	if (ret)
		pr_info("Failed to set boost voltage, ret = %d\n", ret);

	ret = bq25601_set_boost_current(bq, bq->platform_data->boosti);
	if (ret)
		pr_info("Failed to set boost current, ret = %d\n", ret);

	ret = bq25601_set_acovp_threshold(bq, bq->platform_data->vac_ovp);
	if (ret)
		pr_info("Failed to set acovp threshold, ret = %d\n", ret);

	ret = bq25601_set_int_mask(bq, REG0A_IINDPM_INT_MASK |
					       REG0A_VINDPM_INT_MASK);
	if (ret)
		pr_info("Failed to set vindpm and iindpm int mask\n");

	return 0;
}

#if 0
static void determine_initial_status(struct bq25601 *bq)
{
	bq25601_irq_handler(bq->irq, (void *) bq);
}
#endif

static int bq25601_detect_device(struct bq25601 *bq)
{
	int ret;
	u8 data;

	ret = bq25601_read_byte(bq, BQ25601_REG_0B, &data);
	if (!ret) {
		bq->part_no = (data & REG0B_PN_MASK) >> REG0B_PN_SHIFT;
		bq->revision =
			(data & REG0B_DEV_REV_MASK) >> REG0B_DEV_REV_SHIFT;
		pr_info("%s;part_no=%x;revision=%x;\n", __func__,bq->part_no,bq->revision);
	}

	return ret;
}

static void bq25601_dump_regs(struct bq25601 *bq)
{
	int addr;
	u8 val[0x0c];
	int ret;

	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq25601_read_byte(bq, addr, &val[addr]);
	}
	pr_err("bq25601 [0x0]=0x%.2x [0x1]=0x%.2x [0x2]=0x%.2x  [0x3]=0x%.2x [0x4]=0x%.2x [0x5]=0x%.2x [0x6]=0x%.2x \n",
		                      val[0],val[1],val[2],val[3],val[4],val[5],val[6]);
	pr_err("bq25601 [0x7]=0x%.2x [0x8]=0.2x%x [0x9]=0x%.2x  [0xa]=0x%.2x [0xb]=0x%.2x  \n",
		                      val[7],val[8],val[9],val[0xa],val[0xb]);
}

static ssize_t bq25601_show_registers(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct bq25601 *bq = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "bq25601 Reg");
	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq25601_read_byte(bq, addr, &val);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,
				       "Reg[%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t bq25601_store_registers(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct bq25601 *bq = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg < 0x0B)
		bq25601_write_byte(bq, (unsigned char)reg, (unsigned char)val);

	return count;
}

static DEVICE_ATTR(registers, 0644, bq25601_show_registers,
		   bq25601_store_registers);

static struct attribute *bq25601_attributes[] = {
	&dev_attr_registers.attr, NULL,
};

static const struct attribute_group bq25601_attr_group = {
	.attrs = bq25601_attributes,
};

static int bq25601_charging(struct charger_device *chg_dev, bool enable)
{

	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 val;

	if (enable)
		ret = bq25601_enable_charger(bq);
	else
		ret = bq25601_disable_charger(bq);

	pr_info("%s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	ret = bq25601_read_byte(bq, BQ25601_REG_01, &val);

	if (!ret)
		bq->charge_enabled = !!(val & REG01_CHG_CONFIG_MASK);

	return ret;
}

static int bq25601_plug_in(struct charger_device *chg_dev)
{

	int ret;

	ret = bq25601_charging(chg_dev, true);

	if (ret)
		pr_info("Failed to enable charging:%d\n", ret);

	return ret;
}

static int bq25601_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = bq25601_charging(chg_dev, false);

	if (ret)
		pr_info("Failed to disable charging:%d\n", ret);

	return ret;
}

static int bq25601_dump_register(struct charger_device *chg_dev)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);

	bq25601_dump_regs(bq);

	return 0;
}

static int bq25601_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);

	*en = bq->charge_enabled;

	return 0;
}

static int bq25601_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 val;

	ret = bq25601_read_byte(bq, BQ25601_REG_08, &val);
	if (!ret) {
		val = val & REG08_CHRG_STAT_MASK;
		val = val >> REG08_CHRG_STAT_SHIFT;
		*done = (val == REG08_CHRG_STAT_CHGDONE);
	}

	return ret;
}

static int bq25601_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
       int ret;
	pr_info("charge curr = %d\n", curr);
       ret = bq25601_set_chargecurrent(bq, curr / 1000);

	return ret;
}

static int bq25601_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg;
	int ret;

	ret = bq25601_read_byte(bq, BQ25601_REG_02, &reg_val);
	if (!ret) {
		ichg = (reg_val & REG02_ICHG_MASK) >> REG02_ICHG_SHIFT;
		ichg = ichg * REG02_ICHG_LSB + REG02_ICHG_BASE;
		*curr = ichg * 1000;
	}

	return ret;
}

static int bq25601_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	*curr = 60 * 1000;

	return 0;
}

static int bq25601_set_vchg(struct charger_device *chg_dev, u32 volt)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
        int ret=0;
	pr_info("charge volt = %d\n", volt);
       ret = bq25601_set_chargevolt(bq, volt / 1000);
	return ret;
}

static int bq25601_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int vchg;
	int ret;

	ret = bq25601_read_byte(bq, BQ25601_REG_04, &reg_val);
	if (!ret) {
		vchg = (reg_val & REG04_VREG_MASK) >> REG04_VREG_SHIFT;
		vchg = vchg * REG04_VREG_LSB + REG04_VREG_BASE;
		*volt = vchg * 1000;
	}

	return ret;
}

static int bq25601_set_ivl(struct charger_device *chg_dev, u32 volt)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);

	pr_info("vindpm volt = %d\n", volt);

	return bq25601_set_input_volt_limit(bq, volt / 1000);
}

static int bq25601_set_icl(struct charger_device *chg_dev, u32 curr)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
       int ret=0;
	pr_info("indpm curr = %d\n", curr);

	ret =bq25601_set_input_current_limit(bq, curr / 1000);

	return ret;
}

static int bq25601_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int icl;
	int ret;

	ret = bq25601_read_byte(bq, BQ25601_REG_00, &reg_val);
	if (!ret) {
		icl = (reg_val & REG00_IINLIM_MASK) >> REG00_IINLIM_SHIFT;
		icl = icl * REG00_IINLIM_LSB + REG00_IINLIM_BASE;
		*curr = icl * 1000;
	}

	return ret;
}

static int bq25601_kick_wdt(struct charger_device *chg_dev)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);

	return bq25601_reset_watchdog_timer(bq);
}

static int bq25601_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);

	if (en)
		ret = bq25601_enable_otg(bq);
	else
		ret = bq25601_disable_otg(bq);

	pr_info("%s OTG %s\n", en ? "enable" : "disable",
	       !ret ? "successfully" : "failed");


	return ret;
}

static int bq25601_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	if (en)
		ret = bq25601_enable_safety_timer(bq);
	else
		ret = bq25601_disable_safety_timer(bq);

	return ret;
}

static int bq25601_is_safety_timer_enabled(struct charger_device *chg_dev,
					   bool *en)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = bq25601_read_byte(bq, BQ25601_REG_05, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG05_EN_TIMER_MASK);

	return ret;
}

static int bq25601_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	struct bq25601 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	pr_info("otg curr = %d\n", curr);

	ret = bq25601_set_boost_current(bq, curr / 1000);

	return ret;
}

static struct charger_ops bq25601_chg_ops = {
	/* Normal charging */
	.plug_in = bq25601_plug_in,
	.plug_out = bq25601_plug_out,
	.dump_registers = bq25601_dump_register,
	.enable = bq25601_charging,
	.is_enabled = bq25601_is_charging_enable,
	.get_charging_current = bq25601_get_ichg,
	.set_charging_current = bq25601_set_ichg,
	.get_input_current = bq25601_get_icl,
	.set_input_current = bq25601_set_icl,
	.get_constant_voltage = bq25601_get_vchg,
	.set_constant_voltage = bq25601_set_vchg,
	.kick_wdt = bq25601_kick_wdt,
	.set_mivr = bq25601_set_ivl,
	.is_charging_done = bq25601_is_charging_done,
	.get_min_charging_current = bq25601_get_min_ichg,

	/* Safety timer */
	.enable_safety_timer = bq25601_set_safety_timer,
	.is_safety_timer_enabled = bq25601_is_safety_timer_enabled,

	/* Power path */
	.enable_powerpath = NULL,
	.is_powerpath_enabled = NULL,

	/* OTG */
	.enable_otg = bq25601_set_otg,
	.set_boost_current_limit = bq25601_set_boost_ilmt,
	.enable_discharge = NULL,

	/* PE+/PE+20 */
	.send_ta_current_pattern = NULL,
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
	.enable_cable_drop_comp = NULL,

	/* ADC */
	.get_tchg_adc = NULL,
};

static const struct of_device_id bq25601_charger_match_table[] = {
	{
		.compatible = "ti,bq25600", .data = &pn_data[PN_BQ25600],
	},
	{
		.compatible = "ti,bq25600D", .data = &pn_data[PN_BQ25600D],
	},
	{
		.compatible = "ti,bq25601", .data = &pn_data[PN_BQ25601],
	},
	{
		.compatible = "ti,bq25601D", .data = &pn_data[PN_BQ25601D],
	},
	{},
};
// MODULE_DEVICE_TABLE(of, bq25601_charger_match_table);

static int bq25601_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct bq25601 *bq;
	const struct of_device_id *match;
	struct device_node *node = client->dev.of_node;

	int ret = 0;

	pr_info("bq25601 probe start\n");

//+add by hzb for ontim debug
        if(CHECK_THIS_DEV_DEBUG_AREADY_EXIT()==0)
        {
           return -EIO;
        }
//-add by hzb for ontim debug

	bq = devm_kzalloc(&client->dev, sizeof(struct bq25601), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->dev = &client->dev;
	bq->client = client;

	i2c_set_clientdata(client, bq);

	mutex_init(&bq->i2c_rw_lock);

	ret = bq25601_detect_device(bq);
	if (ret) {
		pr_info("No bq25601 device found!\n");
		return -ENODEV;
	}

	match = of_match_node(bq25601_charger_match_table, node);
	if (match == NULL) {
		pr_info("device tree match not found\n");
		return -EINVAL;
	}

	if (bq->part_no != *(int *)match->data)
		pr_info("part no mismatch, hw:%s, devicetree:%s\n",
			pn_str[bq->part_no], pn_str[*(int *)match->data]);

	if( bq->part_no == 0x09)
	       strncpy(charge_ic_vendor_name,"SY6974",20);
	else if ( bq->part_no == 0x0f)
       	strncpy(charge_ic_vendor_name,"BQ25601",20);


	bq->platform_data = bq25601_parse_dt(node, bq);

	if (!bq->platform_data) {
		pr_info("No platform data provided.\n");
		return -EINVAL;
	}

	ret = bq25601_init_device(bq);
	if (ret) {
		pr_info("Failed to init device\n");
		return ret;
	}

#if 0
	bq25601_register_interrupt(bq);
#endif

	bq->chg_dev =
		charger_device_register(bq->chg_dev_name, &client->dev, bq,
					&bq25601_chg_ops, &bq25601_chg_props);
	if (IS_ERR_OR_NULL(bq->chg_dev)) {
		ret = PTR_ERR(bq->chg_dev);
		return ret;
	}

	bq->psy = power_supply_get_by_name("charger");

	if (!bq->psy) {
		pr_err("%s: get power supply failed\n", __func__);
		return -EINVAL;
	}


	ret = sysfs_create_group(&bq->dev->kobj, &bq25601_attr_group);
	if (ret)
		dev_info(bq->dev, "failed to register sysfs. err: %d\n", ret);

#if 0
	determine_initial_status(bq);
#endif

	pr_info("bq25601 probe successfully, Part Num:%d, Revision:%d\n!",
	       bq->part_no, bq->revision);

//+add by hzb for ontim debug
	REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV();
//-add by hzb for ontim debug

	return 0;
}

static int bq25601_charger_remove(struct i2c_client *client)
{
	struct bq25601 *bq = i2c_get_clientdata(client);

	mutex_destroy(&bq->i2c_rw_lock);

	sysfs_remove_group(&bq->dev->kobj, &bq25601_attr_group);

	return 0;
}

static void bq25601_charger_shutdown(struct i2c_client *client)
{
}

static const struct i2c_device_id bq25601_charger_id[] = {
	{"bq25601", 0}, {},
};

MODULE_DEVICE_TABLE(i2c, bq25601_charger_id);

static struct i2c_driver bq25601_charger_driver = {
	.driver = {

			.name = "bq25601-charger",
			.owner = THIS_MODULE,
			.of_match_table = bq25601_charger_match_table,
		},

	.probe = bq25601_charger_probe,
	.remove = bq25601_charger_remove,
	.shutdown = bq25601_charger_shutdown,
	.id_table = bq25601_charger_id,

};

module_i2c_driver(bq25601_charger_driver);

MODULE_DESCRIPTION("TI BQ25601 Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Texas Instruments");

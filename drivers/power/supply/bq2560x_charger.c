/*
 * BQ2560x battery charging driver
 *
 * Copyright (C) 2013 Texas Instruments
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define pr_fmt(fmt)	"[bq2560x]:%s: " fmt, __func__

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/math64.h>
//#include <mt-plat/charger_type.h>

//#include "mtk_charger_intf.h"
#include <linux/regulator/driver.h>
#include "charger_class.h"
#include "bq2560x_reg.h"
#include "bq2560x.h"
#include "mtk_charger.h"
#include <tcpm.h>

//+bug 589756,yaocankun,20201014,add,add charger info
/*#include <linux/hardware_info.h>
*/
enum {
	CHARGER_IC_UNKNOWN,
	CHARGER_IC_SGM41511,	//PN:0010
	CHARGER_IC_BQ2560X,
	CHARGER_IC_ETA6953,	//PN:0010
	CHARGER_IC_SY6974,	//PN:1001
	CHARGER_IC_SGM41542,	//PN:1101
	CHARGER_IC_SGM41513,  //PN:0000

};
static int charger_ic_type = CHARGER_IC_UNKNOWN;
//-bug 589756,yaocankun,20201014,add,add charger info

static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	5, 10, 15, 20, 30, 40, 50, 60,
	80, 100, 120, 140, 160, 180, 200, 240
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	5, 10, 15, 20, 30, 40, 50, 60,
	80, 100, 120, 140, 160, 180, 200, 240
};

enum {
	PN_BQ25600,
	PN_BQ25600D,
	PN_BQ25601,
	PN_BQ25601D,
};

static int pn_data[] = {
	[PN_BQ25600] = 0x00,
	[PN_BQ25600D] = 0x01,
	[PN_BQ25601] = 0x02,
	[PN_BQ25601D] = 0x07,
};

static char *pn_str[] = {
	[PN_BQ25600] = "bq25600",
	[PN_BQ25600D] = "bq25600d",
	[PN_BQ25601] = "bq25601",
	[PN_BQ25601D] = "bq25601d",
};

struct bq2560x {
	struct device *dev;
	struct i2c_client *client;

	int part_no;
	int revision;

	const char *chg_dev_name;
	const char *eint_name;

	bool chg_det_enable;

//	enum charger_type chg_type;

	int status;
	int irq;

	struct mutex i2c_rw_lock;

	bool charge_enabled;	/* Register bit status */
	bool hiz_mode;
	bool power_good;

	struct bq2560x_platform_data *platform_data;
	struct charger_device *chg_dev;
	struct regulator_dev *otg_rdev;
//+BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod
	struct power_supply_desc psy_desc;
	struct power_supply_config psy_cfg;
	struct power_supply *psy;
//-BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod

	struct notifier_block pd_nb;
	struct mutex attach_lock;
	struct tcpc_device *tcpc_dev;
	bool attach;
	bool mmi_charging_full;
	struct power_supply *chg_psy;
};

static const struct charger_properties bq2560x_chg_props = {
	.alias_name = "bq2560x",
};

static int __bq2560x_read_reg(struct bq2560x *bq, u8 reg, u8 *data)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(bq->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u8) ret;

	return 0;
}

static int __bq2560x_write_reg(struct bq2560x *bq, int reg, u8 val)
{
	s32 ret;

	ret = i2c_smbus_write_byte_data(bq->client, reg, val);
	if (ret < 0) {
		pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
		       val, reg, ret);
		return ret;
	}
	return 0;
}

static int bq2560x_read_byte(struct bq2560x *bq, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq2560x_read_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	return ret;
}

static int bq2560x_write_byte(struct bq2560x *bq, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq2560x_write_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}

static int bq2560x_update_bits(struct bq2560x *bq, u8 reg, u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __bq2560x_read_reg(bq, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= data & mask;

	ret = __bq2560x_write_reg(bq, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&bq->i2c_rw_lock);
	return ret;
}

static int bq2560x_enable_otg(struct bq2560x *bq)
{
	u8 val = REG01_OTG_ENABLE << REG01_OTG_CONFIG_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_OTG_CONFIG_MASK,
				   val);

}

static int bq2560x_disable_otg(struct bq2560x *bq)
{
	u8 val = REG01_OTG_DISABLE << REG01_OTG_CONFIG_SHIFT;
	//pr_err("lsw_charger bq2560x_disable_otg\n");
	return bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_OTG_CONFIG_MASK,
				   val);

}

static int bq2560x_enable_charger(struct bq2560x *bq)
{
	int ret;
	u8 val = REG01_CHG_ENABLE << REG01_CHG_CONFIG_SHIFT;

	ret =
	    bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);

	return ret;
}

static int bq2560x_disable_charger(struct bq2560x *bq)
{
	int ret;
	u8 val = REG01_CHG_DISABLE << REG01_CHG_CONFIG_SHIFT;

	ret =
	    bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);
	return ret;
}

int bq2560x_set_chargecurrent(struct bq2560x *bq, int curr)
{
	u8 ichg;

	if (bq->part_no == 0x0) {
		if (curr < SGM4154x_ICHRG_I_MIN_uA)
			curr = SGM4154x_ICHRG_I_MIN_uA;
		else if ( curr > SGM4154x_ICHRG_I_MAX_uA)
			curr = SGM4154x_ICHRG_I_MAX_uA;
		if (curr <= 40)
			ichg = curr / 5;
		else if (curr <= 110)
			ichg = 0x08 + (curr -40) / 10;
		else if (curr <= 270)
			ichg = 0x0F + (curr -110) / 20;
		else if (curr <= 540)
			ichg = 0x17 + (curr -270) / 30;
		else if (curr <= 1500)
			ichg = 0x20 + (curr -540) / 60;
		else if (curr <= 2940)
			ichg = 0x30 + (curr -1500) / 120;
		else
			ichg = 0x3d;
	} else {
		if (curr < REG02_ICHG_BASE)
			curr = REG02_ICHG_BASE;

		ichg = (curr - REG02_ICHG_BASE) / REG02_ICHG_LSB;
	}

	pr_err("bq2560x_set_chargecurrent : the curr is %d, the val is 0x%.2x \n", curr, ichg);
	return bq2560x_update_bits(bq, BQ2560X_REG_02, REG02_ICHG_MASK,
				   ichg << REG02_ICHG_SHIFT);

}

int bq2560x_set_term_current(struct bq2560x *bq, int curr)
{
	u8 iterm;

	if (bq->part_no == 0x0) {
		//sgm41513 the iterm range is 5ma-240ma, 5/10/20/40ma steps
		for(iterm = 1; iterm < 16 && curr >= ITERM_CURRENT_STABLE[iterm]; iterm++)
			;
		iterm--;
	} else {
		if (curr < REG03_ITERM_BASE)
			curr = REG03_ITERM_BASE;
		iterm = (curr - REG03_ITERM_BASE) / REG03_ITERM_LSB;
	}

	pr_err(" bq2560x_set_term_current : the curr is %d, the val is 0x%.2x \n ", curr, iterm);
	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_ITERM_MASK,
				   iterm << REG03_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_term_current);

int bq2560x_set_prechg_current(struct bq2560x *bq, int curr)
{
	u8 iprechg;

	if (bq->part_no == 0x0) {
		//sgm41513 the iprechg range is 5ma-240ma, 5/10/20/40ma steps
		for(iprechg = 1; iprechg < 16 && curr >= IPRECHG_CURRENT_STABLE[iprechg]; iprechg++)
			;
		iprechg--;
	} else {
		if (curr < REG03_IPRECHG_BASE)
			curr = REG03_IPRECHG_BASE;
		iprechg = (curr - REG03_IPRECHG_BASE) / REG03_IPRECHG_LSB;
	}
	pr_err(" bq2560x_set_prechg_current : the curr is %d, the val is 0x%.2x \n ", curr, iprechg);
	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_IPRECHG_MASK,
				   iprechg << REG03_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_prechg_current);

int bq2560x_set_chargevolt(struct bq2560x *bq, int volt)
{
	u8 val;

	if (volt < REG04_VREG_BASE)
		volt = REG04_VREG_BASE;

//bug589756, yaocankun, 20201030, mod, fix chg cv difference
//	val = (volt - REG04_VREG_BASE) / REG04_VREG_LSB + 1;
	val = (volt - REG04_VREG_BASE) / REG04_VREG_LSB;
	return bq2560x_update_bits(bq, BQ2560X_REG_04, REG04_VREG_MASK,
				   val << REG04_VREG_SHIFT);
}

int bq2560x_set_input_volt_limit(struct bq2560x *bq, int volt)
{
	u8 val;

	if (volt < REG06_VINDPM_BASE)
		volt = REG06_VINDPM_BASE;

	val = (volt - REG06_VINDPM_BASE) / REG06_VINDPM_LSB;
	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_VINDPM_MASK,
				   val << REG06_VINDPM_SHIFT);
}

int bq2560x_set_input_current_limit(struct bq2560x *bq, int curr)
{
	u8 val;

	if (curr < REG00_IINLIM_BASE)
		curr = REG00_IINLIM_BASE;

	val = (curr - REG00_IINLIM_BASE) / REG00_IINLIM_LSB;
	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_IINLIM_MASK,
				   val << REG00_IINLIM_SHIFT);
}

int bq2560x_set_watchdog_timer(struct bq2560x *bq, u8 timeout)
{
	u8 temp;

	temp = (u8) (((timeout -
		       REG05_WDT_BASE) / REG05_WDT_LSB) << REG05_WDT_SHIFT);

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_WDT_MASK, temp);
}
EXPORT_SYMBOL_GPL(bq2560x_set_watchdog_timer);

int bq2560x_disable_watchdog_timer(struct bq2560x *bq)
{
	u8 val = REG05_WDT_DISABLE << REG05_WDT_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_watchdog_timer);

int bq2560x_reset_watchdog_timer(struct bq2560x *bq)
{
	u8 val = REG01_WDT_RESET << REG01_WDT_RESET_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_WDT_RESET_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_reset_watchdog_timer);

int bq2560x_reset_chip(struct bq2560x *bq)
{
	int ret;
	u8 val = REG0B_REG_RESET << REG0B_REG_RESET_SHIFT;

	ret =
	    bq2560x_update_bits(bq, BQ2560X_REG_0B, REG0B_REG_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_reset_chip);

int bq2560x_enter_hiz_mode(struct bq2560x *bq)
{
	u8 val = REG00_HIZ_ENABLE << REG00_ENHIZ_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(bq2560x_enter_hiz_mode);

int bq2560x_exit_hiz_mode(struct bq2560x *bq)
{

	u8 val = REG00_HIZ_DISABLE << REG00_ENHIZ_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(bq2560x_exit_hiz_mode);

int bq2560x_get_hiz_mode(struct bq2560x *bq, u8 *state)
{
	u8 val;
	int ret;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_00, &val);
	if (ret)
		return ret;
	*state = (val & REG00_ENHIZ_MASK) >> REG00_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(bq2560x_get_hiz_mode);

int bq2560x_enable_term(struct bq2560x *bq, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = REG05_TERM_ENABLE << REG05_EN_TERM_SHIFT;
	else
		val = REG05_TERM_DISABLE << REG05_EN_TERM_SHIFT;

	ret = bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_enable_term);

int bq2560x_set_boost_current(struct bq2560x *bq, int curr)
{
	u8 val;

	val = REG02_BOOST_LIM_0P5A;
	if (curr == BOOSTI_1200)
		val = REG02_BOOST_LIM_1P2A;

	if ( charger_ic_type == CHARGER_IC_SGM41513 || charger_ic_type == CHARGER_IC_ETA6953) {
		if (curr >= BOOSTI_1200) {
			val = REG02_BOOST_LIM_1P2A;
			pr_err("the SGM41513/ETA6935 set the boot lim is 1200\n");
		}
	}
//the sgm41542 boot lim is diff sgm41511
	if ( charger_ic_type == CHARGER_IC_SGM41542 ) {
		if (curr == BOOSTI_1200) {
			val = REG02_BOOST_LIM_0P5A;
			pr_err("the SGM41542 set the boot lim is 1200\n");
		} else if (curr == BOOSTI_2000) {
			val = REG02_BOOST_LIM_1P2A;
			pr_err("the SGM41542 set the boot lim is 2000\n");
		}
	}

	return bq2560x_update_bits(bq, BQ2560X_REG_02, REG02_BOOST_LIM_MASK,
				   val << REG02_BOOST_LIM_SHIFT);
}

int bq2560x_set_boost_voltage(struct bq2560x *bq, int volt)
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

	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_BOOSTV_MASK,
				   val << REG06_BOOSTV_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_boost_voltage);

int bq2560x_set_acovp_threshold(struct bq2560x *bq, int volt)
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

	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_OVP_MASK,
				   val << REG06_OVP_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_acovp_threshold);

static int bq2560x_set_stat_ctrl(struct bq2560x *bq, int ctrl)
{
	u8 val;

	val = ctrl;

	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_STAT_CTRL_MASK,
				   val << REG00_STAT_CTRL_SHIFT);
}
static struct bq2560x *g_bq2560x;
void bq2560x_enable_statpin(bool en)
{

	if(en)
		bq2560x_set_stat_ctrl(g_bq2560x, 0);
	else
		bq2560x_set_stat_ctrl(g_bq2560x, 3);
}
EXPORT_SYMBOL_GPL(bq2560x_enable_statpin);


static int bq2560x_set_int_mask(struct bq2560x *bq, int mask)
{
	u8 val;

	val = mask;

	return bq2560x_update_bits(bq, BQ2560X_REG_0A, REG0A_INT_MASK_MASK,
				   val << REG0A_INT_MASK_SHIFT);
}

int bq2560x_enable_batfet(struct bq2560x *bq)
{
	const u8 val = REG07_BATFET_ON << REG07_BATFET_DIS_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_enable_batfet);

int bq2560x_disable_batfet(struct bq2560x *bq)
{
	const u8 val = REG07_BATFET_OFF << REG07_BATFET_DIS_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_batfet);

int bq2560x_set_batfet_delay(struct bq2560x *bq, uint8_t delay)
{
	u8 val;

	if (delay == 0)
		val = REG07_BATFET_DLY_0S;
	else
		val = REG07_BATFET_DLY_10S;

	val <<= REG07_BATFET_DLY_SHIFT;

	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_BATFET_DLY_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_batfet_delay);

int bq2560x_enable_safety_timer(struct bq2560x *bq)
{
	const u8 val = REG05_CHG_TIMER_ENABLE << REG05_EN_TIMER_SHIFT;

	pr_err("chg enable safety timer\n");

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_enable_safety_timer);

int bq2560x_disable_safety_timer(struct bq2560x *bq)
{
	const u8 val = REG05_CHG_TIMER_DISABLE << REG05_EN_TIMER_SHIFT;

	pr_err("chg disable safety timer\n");

	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_safety_timer);

static struct bq2560x_platform_data *bq2560x_parse_dt(struct device_node *np,
						      struct bq2560x *bq)
{
	int ret;
	struct bq2560x_platform_data *pdata;

	pdata = devm_kzalloc(bq->dev, sizeof(struct bq2560x_platform_data),
			     GFP_KERNEL);
	if (!pdata)
		return NULL;

	if (of_property_read_string(np, "charger_name", &bq->chg_dev_name) < 0) {
		bq->chg_dev_name = "primary_chg";
		pr_warn("no charger name\n");
	}

	if (of_property_read_string(np, "eint_name", &bq->eint_name) < 0) {
		bq->eint_name = "chr_stat";
		pr_warn("no eint name\n");
	}

	bq->chg_det_enable =
	    of_property_read_bool(np, "ti,bq2560x,charge-detect-enable");

	ret = of_property_read_u32(np, "ti,bq2560x,usb-vlim", &pdata->usb.vlim);
	if (ret) {
		pdata->usb.vlim = 4500;
		pr_err("Failed to read node of ti,bq2560x,usb-vlim\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,usb-ilim", &pdata->usb.ilim);
	if (ret) {
		pdata->usb.ilim = 2000;
		pr_err("Failed to read node of ti,bq2560x,usb-ilim\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,usb-vreg", &pdata->usb.vreg);
	if (ret) {
		pdata->usb.vreg = 4200;
		pr_err("Failed to read node of ti,bq2560x,usb-vreg\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,usb-ichg", &pdata->usb.ichg);
	if (ret) {
		pdata->usb.ichg = 2000;
		pr_err("Failed to read node of ti,bq2560x,usb-ichg\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,stat-pin-ctrl",
				   &pdata->statctrl);
	if (ret) {
		pdata->statctrl = 0;
		pr_err("Failed to read node of ti,bq2560x,stat-pin-ctrl\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,precharge-current",
				   &pdata->iprechg);
	if (ret) {
		pdata->iprechg = 180;
		pr_err("Failed to read node of ti,bq2560x,precharge-current\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,termination-current",
				   &pdata->iterm);
	if (ret) {
		pdata->iterm = 180;
		pr_err
		    ("Failed to read node of ti,bq2560x,termination-current\n");
	}

	ret =
	    of_property_read_u32(np, "ti,bq2560x,boost-voltage",
				 &pdata->boostv);
	if (ret) {
		pdata->boostv = 5000;
		pr_err("Failed to read node of ti,bq2560x,boost-voltage\n");
	}

	ret =
	    of_property_read_u32(np, "ti,bq2560x,boost-current",
				 &pdata->boosti);
	if (ret) {
		pdata->boosti = 1200;
		pr_err("Failed to read node of ti,bq2560x,boost-current\n");
	}

	ret = of_property_read_u32(np, "ti,bq2560x,vac-ovp-threshold",
				   &pdata->vac_ovp);
	if (ret) {
		pdata->vac_ovp = 6500;
		pr_err("Failed to read node of ti,bq2560x,vac-ovp-threshold\n");
	}

	return pdata;
}
/*
static int bq2560x_get_charger_type(struct bq2560x *bq, enum charger_type *type)
{
	int ret;

	u8 reg_val = 0;
	int vbus_stat = 0;
	enum charger_type chg_type = CHARGER_UNKNOWN;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_08, &reg_val);

	if (ret)
		return ret;

	vbus_stat = (reg_val & REG08_VBUS_STAT_MASK);
	vbus_stat >>= REG08_VBUS_STAT_SHIFT;

	switch (vbus_stat) {

	case REG08_VBUS_TYPE_NONE:
		chg_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
		break;
	case REG08_VBUS_TYPE_SDP:
		chg_type = POWER_SUPPLY_USB_TYPE_SDP;
		break;
	case REG08_VBUS_TYPE_CDP:
		chg_type = POWER_SUPPLY_USB_TYPE_CDP;
		break;
	case REG08_VBUS_TYPE_DCP:
		chg_type = POWER_SUPPLY_USB_TYPE_DCP;
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
static int bq2560x_inform_charger_type(struct bq2560x *bq)
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
*/
static irqreturn_t bq2560x_irq_handler(int irq, void *data)
{


	int ret;
	u8 reg_val;
	bool prev_pg;
//	enum charger_type prev_chg_type;
	struct bq2560x *bq = data;

	pr_err("triggered\n");
	ret = bq2560x_read_byte(bq, BQ2560X_REG_08, &reg_val);
	if (ret)
		return IRQ_HANDLED;

	prev_pg = bq->power_good;

	bq->power_good = !!(reg_val & REG08_PG_STAT_MASK);

	if (!prev_pg && bq->power_good)
		pr_notice("adapter/usb inserted\n");
	else if (prev_pg && !bq->power_good)
		pr_notice("adapter/usb removed\n");
/*

	prev_chg_type = bq->chg_type;

	ret = bq2560x_get_charger_type(bq, &bq->chg_type);
	if (!ret && prev_chg_type != bq->chg_type && bq->chg_det_enable)
		bq2560x_inform_charger_type(bq);
*/
	return IRQ_HANDLED;
}

static int bq2560x_register_interrupt(struct bq2560x *bq)
{
	int ret = 0;
	struct device_node *np;

	np = of_find_node_by_name(NULL, bq->eint_name);
	if (np) {
		bq->irq = irq_of_parse_and_map(np, 0);
	} else {
		pr_err("couldn't get irq node\n");
		return -ENODEV;
	}

	pr_info("irq = %d\n", bq->irq);

	ret = devm_request_threaded_irq(bq->dev, bq->irq, NULL,
					bq2560x_irq_handler,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					bq->eint_name, bq);
	if (ret < 0) {
		pr_err("request thread irq failed:%d\n", ret);
		return ret;
	}

	enable_irq_wake(bq->irq);

	return 0;
}

static int bq2560x_init_device(struct bq2560x *bq)
{
	int ret;

	bq2560x_disable_watchdog_timer(bq);

	ret = bq2560x_set_stat_ctrl(bq, bq->platform_data->statctrl);
	if (ret)
		pr_err("Failed to set stat pin control mode, ret = %d\n", ret);

	ret = bq2560x_set_prechg_current(bq, bq->platform_data->iprechg);
	if (ret)
		pr_err("Failed to set prechg current, ret = %d\n", ret);

	ret = bq2560x_set_term_current(bq, bq->platform_data->iterm);
	if (ret)
		pr_err("Failed to set termination current, ret = %d\n", ret);

	ret = bq2560x_set_boost_voltage(bq, bq->platform_data->boostv);
	if (ret)
		pr_err("Failed to set boost voltage, ret = %d\n", ret);

	ret = bq2560x_set_boost_current(bq, bq->platform_data->boosti);
	if (ret)
		pr_err("Failed to set boost current, ret = %d\n", ret);

	ret = bq2560x_set_acovp_threshold(bq, bq->platform_data->vac_ovp);
	if (ret)
		pr_err("Failed to set acovp threshold, ret = %d\n", ret);

// disable charger safety timer
	ret = bq2560x_disable_safety_timer(bq);
	if (ret)
		pr_err("Failed to disable safety timer, ret = %d\n", ret);


	ret = bq2560x_set_int_mask(bq,
				   REG0A_IINDPM_INT_MASK |
				   REG0A_VINDPM_INT_MASK);
	if (ret)
		pr_err("Failed to set vindpm and iindpm int mask\n");

	return 0;
}

static void determine_initial_status(struct bq2560x *bq)
{
	bq2560x_irq_handler(bq->irq, (void *) bq);
}

static int bq2560x_detect_device(struct bq2560x *bq)
{
	int ret;
	u8 data;
	u8 data_2;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_0B, &data);
	if (!ret) {
		bq->part_no = (data & REG0B_PN_MASK) >> REG0B_PN_SHIFT;
		bq->revision =
		    (data & REG0B_DEV_REV_MASK) >> REG0B_DEV_REV_SHIFT;
	}

	switch(bq->part_no)
	{
		case 0x2:
			charger_ic_type = CHARGER_IC_ETA6953;
			pr_info("Found charger ic ETA6953 or SGM41511\n");

		break;
		case 0x9:
			charger_ic_type = CHARGER_IC_SY6974;
			pr_info("Found charger ic SY6974\n");

		break;

		case 0xD:
			charger_ic_type = CHARGER_IC_SGM41542;
			pr_err("Found: charger ic SGM41542\n");
		break;

		case 0x0:

			charger_ic_type = CHARGER_IC_SGM41513;
			pr_err("Found charger ic SGM41513\n");

		break;

		default:
			charger_ic_type = CHARGER_IC_ETA6953;
			pr_err("Unknow charger ic, set it to ETA6953\n");

	}

	if(charger_ic_type == CHARGER_IC_ETA6953) {
		ret = bq2560x_read_byte(bq, BQ2560X_REG_0C, &data_2);
		if (!ret) {
			if(data_2 == REG0C_DEVICE_SGM) {
				charger_ic_type = CHARGER_IC_SGM41511;
				pr_info("Found charger ic is SGM41511\n");
			} else {
				pr_info("Found charger ic is ETA6953\n");
			}
		} else {
			pr_err("get charger ic BQ2560X_REG_0C error,defaultly setting to CHARGER_IC_ETA6953\n");
		}
	}

	return ret;
}

static void bq2560x_dump_regs(struct bq2560x *bq)
{
	int addr;
	u8 val;
	int ret;

	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq2560x_read_byte(bq, addr, &val);
		if (ret == 0)
			pr_err("Reg[%.2x] = 0x%.2x\n", addr, val);
	}
}

static ssize_t
bq2560x_show_registers(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	struct bq2560x *bq = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "bq2560x Reg");
	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq2560x_read_byte(bq, addr, &val);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,
				       "Reg[%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t
bq2560x_store_registers(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct bq2560x *bq = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg < 0x0B) {
		bq2560x_write_byte(bq, (unsigned char) reg,
				   (unsigned char) val);
	}

	return count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, bq2560x_show_registers,
		   bq2560x_store_registers);

static struct attribute *bq2560x_attributes[] = {
	&dev_attr_registers.attr,
	NULL,
};

static const struct attribute_group bq2560x_attr_group = {
	.attrs = bq2560x_attributes,
};

static int bq2560x_charging(struct charger_device *chg_dev, bool enable)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 val;

//+Extb RNN-3427, liyiying.wt, add, 2021/01/15, reset hiz mode when plug in and plug out usb
#if defined(CONFIG_WT_PROJECT_T99653AA1) || defined(CONFIG_WT_PROJECT_T99652AA1)
	ret = bq2560x_exit_hiz_mode(bq);
	pr_err("lyy_hiz_mode reg_val:%d\n", ret);
#endif
//-Extb RNN-3427, liyiying.wt, add, 2021/01/15, reset hiz mode when plug in and plug out usb

	if (enable)
		ret = bq2560x_enable_charger(bq);
	else
		ret = bq2560x_disable_charger(bq);

	pr_err("%s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	ret = bq2560x_read_byte(bq, BQ2560X_REG_01, &val);

	if (!ret) {
		val = !!(val & REG01_CHG_CONFIG_MASK);
		if (bq->charge_enabled != val) {
			bq->charge_enabled = val;
			power_supply_changed(bq->psy);
		}
	}

	return ret;
}

static int bq2560x_enable_hz(struct charger_device *chg_dev, bool enable)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 val;

	if (enable)
		ret = bq2560x_enter_hiz_mode(bq);
	else
		ret = bq2560x_exit_hiz_mode(bq);

	pr_err("lsw_charger %s enable_hz %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	ret = bq2560x_read_byte(bq, BQ2560X_REG_00, &val);

	if (!ret) {
		val = !!(val & REG00_ENHIZ_MASK);
		if (bq->hiz_mode != val) {
			bq->hiz_mode = val;
			power_supply_changed(bq->psy);
		}
	}

	return ret;
}

//EKELLIS-68,yaocankun,add,20210413,enable usb suspend
static int bq2560x_enable_vbus(struct charger_device *chg_dev, bool enable)
{
	int ret = 0;

	ret = bq2560x_enable_hz(chg_dev, !enable);

	pr_err("lsw_charger %s enable_vbus %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}
//EKELLIS-68,yaocankun,add,20210413,enable usb suspend
//EKELLIS-99,yaocankun,add,20210413,enable otg when in hiz
/*
static int bq2560x_get_vbus_enable(struct charger_device *chg_dev, bool *enable)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	unsigned char en = 0;


	ret = bq2560x_get_hiz_mode(bq, &en);
	*enable = !en;

	pr_err("lsw_charger get enable_vbus %s %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}*/
//EKELLIS-99,yaocankun,add,20210413,enable otg when in hiz

static int bq2560x_plug_in(struct charger_device *chg_dev)
{

	int ret;

	ret = bq2560x_charging(chg_dev, true);

	if (ret)
		pr_err("Failed to enable charging:%d\n", ret);

	return ret;
}

static int bq2560x_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = bq2560x_charging(chg_dev, false);

	if (ret)
		pr_err("Failed to disable charging:%d\n", ret);

	ret = bq2560x_enable_hz(chg_dev, false);

	if (ret)
		pr_err("Failed to disable HIZ mode:%d\n", ret);

	return ret;
}

static int bq2560x_dump_register(struct charger_device *chg_dev)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	bq2560x_dump_regs(bq);

	return 0;
}

static int bq2560x_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	*en = bq->charge_enabled;

	return 0;
}

static int bq2560x_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 val;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_08, &val);
	if (!ret) {
		val = val & REG08_CHRG_STAT_MASK;
		val = val >> REG08_CHRG_STAT_SHIFT;
		*done = (val == REG08_CHRG_STAT_CHGDONE);
	}

	return ret;
}

static int bq2560x_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("charge curr = %d\n", curr);

	return bq2560x_set_chargecurrent(bq, curr / 1000);
}

static int bq2560x_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg;
	int ret;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_02, &reg_val);
	if (!ret) {
		if (bq->part_no == 0x0) {
			ichg = reg_val & REG02_ICHG_MASK;
			if (ichg <= 0x8)
				*curr = ichg * 5000;
			else if (ichg <= 0xF)
				*curr = 40000 + (ichg - 0x8) * 10000;
			else if (ichg <= 0x17)
				*curr = 110000 + (ichg - 0xF) * 20000;
			else if (ichg <= 0x20)
				*curr = 270000 + (ichg - 0x17) * 30000;
			else if (ichg <= 0x30)
				*curr = 540000 + (ichg - 0x20) * 60000;
			else if (ichg <= 0x3C)
				*curr = 1500000 + (ichg - 0x30) * 120000;
			else
				*curr = 3000000;
		} else {
			ichg = (reg_val & REG02_ICHG_MASK) >> REG02_ICHG_SHIFT;
			ichg = ichg * REG02_ICHG_LSB + REG02_ICHG_BASE;
			*curr = ichg * 1000;
		}
	}

	return ret;
}

static int bq2560x_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	*curr = 60 * 1000;

	return 0;
}

static int bq2560x_set_vchg(struct charger_device *chg_dev, u32 volt)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("charge volt = %d\n", volt);

	return bq2560x_set_chargevolt(bq, volt / 1000);
}

static int bq2560x_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int vchg;
	int ret;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_04, &reg_val);
	if (!ret) {
		vchg = (reg_val & REG04_VREG_MASK) >> REG04_VREG_SHIFT;
//bug589756, yaocankun, 20201030, mod, fix chg cv difference
//		vchg -= 1;
		vchg = vchg * REG04_VREG_LSB + REG04_VREG_BASE;
		*volt = vchg * 1000;
	}

	return ret;
}

static int bq2560x_set_ivl(struct charger_device *chg_dev, u32 volt)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("vindpm volt = %d\n", volt);

	return bq2560x_set_input_volt_limit(bq, volt / 1000);

}

static int bq2560x_set_icl(struct charger_device *chg_dev, u32 curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("indpm curr = %d\n", curr);

	return bq2560x_set_input_current_limit(bq, curr / 1000);
}

static int bq2560x_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int icl;
	int ret;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_00, &reg_val);
	if (!ret) {
		icl = (reg_val & REG00_IINLIM_MASK) >> REG00_IINLIM_SHIFT;
		icl = icl * REG00_IINLIM_LSB + REG00_IINLIM_BASE;
		*curr = icl * 1000;
	}

	return ret;

}

static int bq2560x_kick_wdt(struct charger_device *chg_dev)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	return bq2560x_reset_watchdog_timer(bq);
}

static int bq2560x_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	struct bq2560x *bq =
		dev_get_drvdata(&chg_dev->dev);

	if (!bq->psy) {
		dev_notice(bq->dev, "%s: cannot get psy\n", __func__);
		return -ENODEV;
	}

	pr_err("event:%d\n", event);
	switch (event) {
	case EVENT_FULL:
		bq->mmi_charging_full = true;
		break;
	case EVENT_RECHARGE:
	case EVENT_DISCHARGE:
		bq->mmi_charging_full = false;
		break;
	default:
		break;
	}

	power_supply_changed(bq->psy);

	return 0;
}

static int bq2560x_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;
//EKELLIS-99,yaocankun,add,20210413,enable otg when in hiz
	u8 state = 0;
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	ret = bq2560x_get_hiz_mode(bq,&state);
	if(!ret && state && en)
	{
		ret = bq2560x_exit_hiz_mode(bq);
		pr_err("exit hz mode before enable OTG %s\n",
				!ret ? "successfully" : "failed");
	}
	else if(!ret)
	{
		if(state)
		{
			pr_err("hz mode is on with disable otg\n");
		}
		else if(en)
		{
			pr_err("hz mode is off with enable otg\n");
		}
		else
			pr_err("hz mode is off with disable otg\n");
	}
	else
		pr_err("error in get hz mode\n");
//EKELLIS-99,yaocankun,add,20210413,enable otg when in hiz

	if (en)
		ret = bq2560x_enable_otg(bq);
	else
		ret = bq2560x_disable_otg(bq);

	pr_err("%s OTG %s\n", en ? "enable" : "disable",
			!ret ? "successfully" : "failed");

	return ret;
}

static int bq2560x_is_otg_enable(struct charger_device *chg_dev, bool *en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_01, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG01_OTG_CONFIG_MASK);

	return ret;
}

static int bq2560x_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	if (en)
		ret = bq2560x_enable_safety_timer(bq);
	else
		ret = bq2560x_disable_safety_timer(bq);

	return ret;
}

static int bq2560x_is_safety_timer_enabled(struct charger_device *chg_dev,
					   bool *en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_05, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG05_EN_TIMER_MASK);

	return ret;
}

static int bq2560x_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	pr_err("otg curr = %d\n", curr);

	ret = bq2560x_set_boost_current(bq, curr / 1000);

	return ret;
}
//+EXTR ROG-1485,lishuwen.wt,2020.11.10,modify,insert OTG then reboot phone that first boot up can not detect OTG

static int bq2560x_boost_enable_otg(struct regulator_dev *rdev)
{
	int ret;
	struct bq2560x *bq = rdev_get_drvdata(rdev);

	ret = bq2560x_enable_otg(bq);

	pr_err("%s OTG enable %s\n",
	       !ret ? "successfully" : "failed");

	return ret;
}
static int bq2560x_boost_disable_otg(struct regulator_dev *rdev)
{
	int ret;
	struct bq2560x *bq = rdev_get_drvdata(rdev);

	ret = bq2560x_disable_otg(bq);

	pr_err("%s OTG disable %s\n",
	       !ret ? "successfully" : "failed");

	return ret;
}
static int bq2560x_otg_set_boost_ilmt(struct regulator_dev *rdev,
					  int min_uA, int max_uA)
{
	struct bq2560x *bq = rdev_get_drvdata(rdev);
	int cur_otg = 0;
	int ret;
	if(max_uA <= 500000)
		cur_otg = 500;
	else
		cur_otg = 1200;

	pr_err("config otg current at %dmA\n", cur_otg);

	ret = bq2560x_set_boost_current(bq, cur_otg);

	return ret;
}
unsigned int bq2560x_get_otg_config(struct bq2560x *bq)
{
	u8 reg_val;
	int val_ret = 0;
	int ret;

	ret = bq2560x_read_byte(bq, BQ2560X_REG_01, &reg_val);
	if (!ret) {
		val_ret = (reg_val & REG01_OTG_CONFIG_MASK) >> REG01_OTG_CONFIG_SHIFT;
	}

	return val_ret;
}
static int bq2560x_is_enabled_vbus(struct regulator_dev *rdev)
{
	struct bq2560x *bq = rdev_get_drvdata(rdev);
	return bq2560x_get_otg_config(bq);
}
//static int bq2560x_otg_set_boost_vlmt(struct regulator_dev *rdev,
//					unsigned int sel)
//{
//	struct bq2560x *bq = rdev_get_drvdata(rdev);
//	int ret;
//
//	pr_err("force vboost at %dmV\n", bq->platform_data->boostv);
//
//	ret = bq2560x_set_boost_voltage(bq, bq->platform_data->boostv);
//	if (ret)
//		pr_err("Failed to set boost voltage, ret = %d\n", ret);
//
//	return ret;
//}
static const struct regulator_ops bq2560x_chg_otg_ops = {
//	.list_voltage = regulator_list_voltage_linear,
	.enable = bq2560x_boost_enable_otg,
	.disable = bq2560x_boost_disable_otg,
	.is_enabled = bq2560x_is_enabled_vbus,
//	.set_voltage_sel = bq2560x_otg_set_boost_vlmt,
//	.get_voltage_sel = mt6370_boost_get_voltage_sel,
	.set_current_limit = bq2560x_otg_set_boost_ilmt,
//	.get_current_limit = mt6370_boost_get_current_limit,
};

static const struct regulator_desc bq2560x_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &bq2560x_chg_otg_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.fixed_uV = 5000000,
	.n_voltages = 1,
//	.vsel_reg = MT6370_PMU_REG_CHGCTRL5,
//	.vsel_mask = MT6370_MASK_BOOST_VOREG,
//	.enable_reg = MT6370_PMU_REG_CHGCTRL1,
//	.enable_mask = MT6370_MASK_OPA_MODE,
//	.csel_reg = MT6370_PMU_REG_CHGCTRL10,
//	.csel_mask = MT6370_MASK_BOOST_OC,

};

static int bq2560x_psy_gauge_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	struct bq2560x *bq;
	int ret;
	u8 status;
	u8 hiz_mode;
	union power_supply_propval online = {0};

	bq = (struct bq2560x *)power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		mutex_lock(&bq->attach_lock);
		val->intval = bq->attach;
		mutex_unlock(&bq->attach_lock);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		ret = bq2560x_get_hiz_mode(bq, &hiz_mode);
		if (ret)
			return -EINVAL;
		hiz_mode = !!hiz_mode;

		ret = bq2560x_read_byte(bq, BQ2560X_REG_08, &status);
		pr_info("status:0x%x, ret:%d", status, ret);
		if (ret)
			return -EINVAL;

		if (bq->chg_psy == NULL) {
			bq->chg_psy = power_supply_get_by_name("primary_chg");
		}
		if (IS_ERR_OR_NULL(bq->chg_psy)) {
			pr_err("%s Couldn't get chg_psy\n", __func__);
			return -EINVAL;
		} else {
			ret = power_supply_get_property(bq->chg_psy,
						POWER_SUPPLY_PROP_ONLINE, &online);
			if (ret)
				return -EINVAL;
		}

		if (!online.intval)
			bq->mmi_charging_full = false;

		if (!online.intval || (online.intval && hiz_mode)) {
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		}

		status = status & REG08_CHRG_STAT_MASK;
		status = status >> REG08_CHRG_STAT_SHIFT;
		if (status == 0) /* Ready */
			if (bq->charge_enabled)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if ((status == 1) || (status == 2)) /* Charge in progress */
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (status == 3) /* Charge done */
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;

		if (bq->mmi_charging_full == true)
			val->intval = POWER_SUPPLY_STATUS_FULL;

		pr_info("bat_status:%d\n", val->intval);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = bq->psy_desc.type;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2560x_psy_gauge_set_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	int ret = 0;
	struct bq2560x *bq;

	bq = (struct bq2560x *)power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = -EINVAL;
		break;
	default:
		ret = -EINVAL;
		break;
		}

	pr_info("%s psp:%d ret:%d val:%d", __func__, psp, ret, val->intval);

	return ret;
}
//-BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod

static int bq2560x_chg_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	return 0;
}

static enum power_supply_usb_type bq2560x_chg_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
//	POWER_SUPPLY_USB_TYPE_SDP,
//	POWER_SUPPLY_USB_TYPE_CDP,
//	POWER_SUPPLY_USB_TYPE_DCP,
};

static enum power_supply_property bq2560x_chg_psy_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
};

static char *bq2560x_psy_supplied_to[] = {
	"battery",
};

static const struct power_supply_desc bq2560x_psy_desc = {
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = bq2560x_chg_psy_usb_types,
	.num_usb_types = ARRAY_SIZE(bq2560x_chg_psy_usb_types),
	.properties = bq2560x_chg_psy_properties,
	.num_properties = ARRAY_SIZE(bq2560x_chg_psy_properties),
	.property_is_writeable = bq2560x_chg_property_is_writeable,
	.get_property = bq2560x_psy_gauge_get_property,
	.set_property = bq2560x_psy_gauge_set_property,
};

static int bq2560x_chg_init_psy(struct bq2560x *bq)
{
	struct power_supply_config cfg = {
		.drv_data = bq,
		.of_node = bq->dev->of_node,
		.supplied_to = bq2560x_psy_supplied_to,
		.num_supplicants = ARRAY_SIZE(bq2560x_psy_supplied_to),
	};
	pr_info("bq2560x_chg_init_psy!\n");
	//pr_info(bq->dev, "%s\n", __func__);
	memcpy(&bq->psy_desc, &bq2560x_psy_desc, sizeof(bq->psy_desc));
	bq->psy_desc.name = "bq2560x-charger";
	bq->psy = devm_power_supply_register(bq->dev, &bq->psy_desc, &cfg);

	return IS_ERR(bq->psy) ? PTR_ERR(bq->psy) : 0;
}

//-EXTR ROG-1485,lishuwen.wt,2020.11.10,modify,insert OTG then reboot phone that first boot up can not detect OTG
static struct charger_ops bq2560x_chg_ops = {
	/* Normal charging */
	.plug_in = bq2560x_plug_in,
	.plug_out = bq2560x_plug_out,
	.dump_registers = bq2560x_dump_register,
	.enable = bq2560x_charging,
//+Bug 597174,lishuwen.wt,2020.11.06,modify,add charger current nod
#ifdef WT_COMPILE_FACTORY_VERSION
#if defined(CONFIG_WT_PROJECT_T99653AA1) || defined(CONFIG_WT_PROJECT_T99652AA1)
	.enable_vbus = bq2560x_enable_vbus,
#endif
#endif
//-Bug 597174,lishuwen.wt,2020.11.06,modify,add charger current nod
	.is_enabled = bq2560x_is_charging_enable,
	.get_charging_current = bq2560x_get_ichg,
	.set_charging_current = bq2560x_set_ichg,
	.get_input_current = bq2560x_get_icl,
	.set_input_current = bq2560x_set_icl,
	.get_constant_voltage = bq2560x_get_vchg,
	.set_constant_voltage = bq2560x_set_vchg,
	.kick_wdt = bq2560x_kick_wdt,
	.set_mivr = bq2560x_set_ivl,
	.is_charging_done = bq2560x_is_charging_done,
	.get_min_charging_current = bq2560x_get_min_ichg,

	/* Safety timer */
	.enable_safety_timer = bq2560x_set_safety_timer,
	.is_safety_timer_enabled = bq2560x_is_safety_timer_enabled,

	/* Power path */
	//.enable_powerpath = bq2560x_enable_vbus,
	//.is_powerpath_enabled = bq2560x_get_vbus_enable,

        /* Hz mode */
        .enable_hz = bq2560x_enable_hz,

	/* OTG */
	.enable_otg = bq2560x_set_otg,
	.is_otg_enable = bq2560x_is_otg_enable,
	.set_boost_current_limit = bq2560x_set_boost_ilmt,
	.enable_discharge = NULL,

	/* PE+/PE+20 */
	.send_ta_current_pattern = NULL,
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
	.enable_cable_drop_comp = NULL,

	/* ADC */
	.get_tchg_adc = NULL,

	/* Event */
	.event = bq2560x_do_event,
};

static struct of_device_id bq2560x_charger_match_table[] = {
	{
	 .compatible = "ti,bq25600",
	 .data = &pn_data[PN_BQ25600],
	 },
	{
	 .compatible = "ti,bq25600D",
	 .data = &pn_data[PN_BQ25600D],
	 },
	{
	 .compatible = "ti,bq25601",
	 .data = &pn_data[PN_BQ25601],
	 },
	{
	 .compatible = "ti,bq25601D",
	 .data = &pn_data[PN_BQ25601D],
	 },
	{},
};
MODULE_DEVICE_TABLE(of, bq2560x_charger_match_table);

static void handle_typec_attach(struct bq2560x *bq,
				bool en)
{
	mutex_lock(&bq->attach_lock);
	bq->attach = en;
	if (!bq->attach)
		bq->mmi_charging_full = false;
	mutex_unlock(&bq->attach_lock);
}

static int pd_tcp_notifier_call(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct tcp_notify *noti = data;
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	bool atm_enabled = false;
	struct bq2560x *bq =
		(struct bq2560x *)container_of(nb,
		struct bq2560x, pd_nb);

	switch (event) {
	case TCP_NOTIFY_TYPEC_STATE:
		if (noti->typec_state.old_state == TYPEC_UNATTACHED &&
		    (noti->typec_state.new_state == TYPEC_ATTACHED_SNK ||
		    noti->typec_state.new_state == TYPEC_ATTACHED_CUSTOM_SRC ||
		    noti->typec_state.new_state == TYPEC_ATTACHED_NORP_SRC)) {
			pr_info("%s USB Plug in, pol = %d\n", __func__,
					noti->typec_state.polarity);
			handle_typec_attach(bq, true);
		} else if ((noti->typec_state.old_state == TYPEC_ATTACHED_SNK ||
		    noti->typec_state.old_state == TYPEC_ATTACHED_CUSTOM_SRC ||
			noti->typec_state.old_state == TYPEC_ATTACHED_NORP_SRC)
			&& noti->typec_state.new_state == TYPEC_UNATTACHED) {
			pr_info("%s USB Plug out\n", __func__);
			handle_typec_attach(bq, false);
		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_SRC &&
			noti->typec_state.new_state == TYPEC_ATTACHED_SNK) {
			pr_info("%s Source_to_Sink\n", __func__);
			handle_typec_attach(bq, true);
		}  else if (noti->typec_state.old_state == TYPEC_ATTACHED_SNK &&
			noti->typec_state.new_state == TYPEC_ATTACHED_SRC) {
			pr_info("%s Sink_to_Source\n", __func__);
			handle_typec_attach(bq, false);
		}
		break;
	default:
		break;
	};

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		pr_err("%s Couldn't get chg_psy\n", __func__);
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		atm_enabled = info->atm_enabled;
	}

	if (atm_enabled)
		power_supply_changed(bq->psy);

	return NOTIFY_OK;
}

static int bq2560x_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct bq2560x *bq;
	const struct of_device_id *match;
	struct device_node *node = client->dev.of_node;
	struct regulator_config config = { };
	int ret = 0;

	bq = devm_kzalloc(&client->dev, sizeof(struct bq2560x), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->dev = &client->dev;
	bq->client = client;

	i2c_set_clientdata(client, bq);

	mutex_init(&bq->i2c_rw_lock);

	ret = bq2560x_detect_device(bq);
	if (ret) {
		pr_err("No bq2560x device found!\n");
		return -ENODEV;
	}

	match = of_match_node(bq2560x_charger_match_table, node);
	if (match == NULL) {
		pr_err("device tree match not found\n");
		return -EINVAL;
	}

	if (bq->part_no != *(int *)match->data)
		pr_info("part no mismatch, hw:%s, devicetree:%s\n",
			pn_str[bq->part_no], pn_str[*(int *) match->data]);

	bq->platform_data = bq2560x_parse_dt(node, bq);

	if (!bq->platform_data) {
		pr_err("No platform data provided.\n");
		return -EINVAL;
	}

	ret = bq2560x_init_device(bq);
	if (ret) {
		pr_err("Failed to init device\n");
		return ret;
	}

	bq2560x_register_interrupt(bq);

	bq->chg_dev = charger_device_register(bq->chg_dev_name,
					      &client->dev, bq,
					      &bq2560x_chg_ops,
					      &bq2560x_chg_props);
	if (IS_ERR_OR_NULL(bq->chg_dev)) {
		ret = PTR_ERR(bq->chg_dev);
		return ret;
	}

	ret = sysfs_create_group(&bq->dev->kobj, &bq2560x_attr_group);
	if (ret)
		dev_err(bq->dev, "failed to register sysfs. err: %d\n", ret);

	determine_initial_status(bq);

	g_bq2560x = bq;

	mutex_init(&bq->attach_lock);

	bq->tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
	if (!bq->tcpc_dev) {
		pr_notice("%s get tcpc device type_c_port0 fail\n", __func__);
		ret = -ENODEV;
		return ret;
	}

	bq->pd_nb.notifier_call = pd_tcp_notifier_call;
	ret = register_tcp_dev_notifier(bq->tcpc_dev, &bq->pd_nb,
					TCP_NOTIFY_TYPE_ALL);
	if (ret < 0) {
		pr_notice("%s: register tcpc notifer fail\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	config.dev = bq->dev;
	config.driver_data = bq;
	bq->otg_rdev = devm_regulator_register(bq->dev,
						&bq2560x_otg_rdesc, &config);
	if (IS_ERR(bq->otg_rdev)) {
		ret = PTR_ERR(bq->otg_rdev);
		pr_err("bq2560x otg probe fail, ret:%d\n", ret);
	}

	pr_err("Found charger ic %d\n", charger_ic_type);

	bq2560x_chg_init_psy(bq);

//+BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod
//	bq->psy_desc.name = "bq2560x-gauge";
//	bq->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
//	bq->psy_desc.properties = bq2560x_gauge_properties;
//	bq->psy_desc.num_properties = ARRAY_SIZE(bq2560x_gauge_properties);
//	bq->psy_desc.get_property = bq2560x_psy_gauge_get_property;
//	bq->psy_desc.set_property = bq2560x_psy_gauge_set_property;
//	bq->psy_cfg.drv_data = bq;
//	bq->psy_cfg.supplied_to = bq2560x_charger_supplied_to;
//	bq->psy_cfg.num_supplicants = ARRAY_SIZE(bq2560x_charger_supplied_to);
//	bq->psy = power_supply_register(bq->dev, &bq->psy_desc, &bq->psy_cfg);
//-BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod

//+EXTR ROG-1709,lishuwen.wt,2020.11.21,modify,plug out OTG then reboot phone that OTG REG error config case vbus always on
	bq2560x_disable_otg(bq);
//-EXTR ROG-1709,lishuwen.wt,2020.11.21,modify,plug out OTG then reboot phone that OTG REG error config case vbus always on
	bq2560x_enable_vbus(bq->chg_dev,true);
	pr_err("bq2560x probe successfully, Part Num:%d, Revision:%d\n!",
	       bq->part_no, bq->revision);

	return 0;
}

static int bq2560x_charger_remove(struct i2c_client *client)
{
	struct bq2560x *bq = i2c_get_clientdata(client);

	mutex_destroy(&bq->i2c_rw_lock);

//+BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod
	power_supply_unregister(bq->psy);
//-BUG 605310,lishuwen.wt,2020.12.05,modify,status for charger current nod

	sysfs_remove_group(&bq->dev->kobj, &bq2560x_attr_group);

	return 0;
}

static void bq2560x_charger_shutdown(struct i2c_client *client)
{
//+EXTR ROG-1709,lishuwen.wt,2020.11.21,modify,plug out OTG then reboot phone that OTG REG error config case vbus always on
	struct bq2560x *bq = i2c_get_clientdata(client);
	pr_err("bq2560x_charger_shutdown\n");
	bq2560x_disable_otg(bq);
//-EXTR ROG-1709,lishuwen.wt,2020.11.21,modify,plug out OTG then reboot phone that OTG REG error config case vbus always on
	bq2560x_enable_vbus(bq->chg_dev,true);

}

static struct i2c_driver bq2560x_charger_driver = {
	.driver = {
		   .name = "bq2560x-charger",
		   .owner = THIS_MODULE,
		   .of_match_table = bq2560x_charger_match_table,
		   },

	.probe = bq2560x_charger_probe,
	.remove = bq2560x_charger_remove,
	.shutdown = bq2560x_charger_shutdown,

};

module_i2c_driver(bq2560x_charger_driver);

MODULE_DESCRIPTION("TI BQ2560x Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Texas Instruments");

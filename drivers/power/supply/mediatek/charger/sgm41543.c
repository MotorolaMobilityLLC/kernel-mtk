/*
 * sgm41543 battery charging driver
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

#define pr_fmt(fmt)	"[sgm41543]:%s: " fmt, __func__

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
#include <linux/printk.h>
//#include "mtk_charger_intf.h"
#include "mtk_charger_intf.h"
#include <linux/regulator/driver.h>
#include <mt-plat/v1/mtk_charger.h>
#include <mt-plat/v1/charger_class.h>
//#include "sgm41543_reg.h"
#include "sgm41543.h"
#include <mt-plat/upmu_common.h>
/*#include <linux/hardware_info.h>*/
#include <mtk_battery_internal.h>

#define SGM41543_STATUS_PLUGIN			0x0001
#define SGM41543_STATUS_PG				0x0002
#define	SGM41543_STATUS_CHARGE_ENABLE	0x0004
#define SGM41543_STATUS_FAULT			0x0008
#define SGM41543_STATUS_VINDPM			0x00010
#define SGM41543_STATUS_IINDPM			0x00020
#define SGM41543_STATUS_EXIST			0x0100


enum sgm41543_vbus_type {
	SGM41543_VBUS_NONE = 0,
	SGM41543_VBUS_USB_SDP,
	SGM41543_VBUS_USB_CDP,
	SGM41543_VBUS_USB_DCP,
	SGM41543_VBUS_MAXC,
	SGM41543_VBUS_UNKNOWN,
	SGM41543_VBUS_NONSTAND,
	SGM41543_VBUS_OTG,
	SGM41543_VBUS_TYPE_NUM,
};

struct sgm41543_charge_param {
	int vlim;
	int ilim;
	int ichg;
	int vreg;
};

enum stat_ctrl {
	STAT_CTRL_STAT,
	STAT_CTRL_ICHG,
	STAT_CTRL_INDPM,
	STAT_CTRL_DISABLE,
};

enum vboost {
	BOOSTV_4850 = 4850,
	BOOSTV_5000 = 5000,
	BOOSTV_5150 = 5150,
	BOOSTV_5300 = 5300,
};

enum iboost {
	BOOSTI_1200 = 1200,
	BOOSTI_2000 = 2000,
};

enum vac_ovp {
	VAC_OVP_5500 = 5500,
	VAC_OVP_6500 = 6500,
	VAC_OVP_10500 = 10500,
	VAC_OVP_14000 = 14000,
};


struct sgm41543_platform_data {
	struct sgm41543_charge_param usb;
	int iprechg;
	int iterm;
	int charge_voltage;
	int charge_current;
	bool enable_term;

	enum stat_ctrl statctrl;
	enum vboost boostv;
	enum iboost boosti;
	enum vac_ovp vac_ovp;

};
void sgm41543_enable_statpin(bool en);

enum {
	CHARGER_IC_UNKNOWN,
	CHARGER_IC_SGM41543,  //PN:0000
};
//static int charger_ic_type = CHARGER_IC_UNKNOWN;


static const unsigned int IPRECHG_CURRENT_STABLE[] = {
	60, 120, 180, 240, 300, 360, 420, 480,
	540, 600, 660, 720
};

static const unsigned int ITERM_CURRENT_STABLE[] = {
	60, 120, 180, 240, 300, 360, 420, 480,
	540, 600, 660, 720, 780, 840, 900, 960
};


enum {
	PN_BQ25600,
	PN_BQ25600D,
	PN_BQ25601,
	PN_BQ25601D,
};
/*
static int pn_data[] = {
	[PN_BQ25600] = 0x00,
	[PN_BQ25600D] = 0x01,
	[PN_BQ25601] = 0x02,
	[PN_BQ25601D] = 0x07,
};*/

static char *pn_str[] = {
	[PN_BQ25600] = "bq25600",
	[PN_BQ25600D] = "bq25600d",
	[PN_BQ25601] = "bq25601",
	[PN_BQ25601D] = "bq25601d",
};

struct sgm41543 {
	struct device *dev;
	struct i2c_client *client;

	int part_no;
	int revision;
	int	vbus_type;

	const char *chg_dev_name;
	const char *eint_name;

	bool chg_det_enable;

//	enum charger_type chg_type;

	int status;
	int irq;
	int irq_gpio;
	int online;

	struct mutex i2c_rw_lock;
	struct mutex pe_lock;

	bool charge_enabled;	/* Register bit status */
	bool power_good;

	struct sgm41543_platform_data *platform_data;
	struct charger_device *chg_dev;
	struct work_struct irq_work;
	struct work_struct adapter_in_work;
	struct work_struct adapter_out_work;
	struct power_supply_desc psy_desc;
	struct power_supply_config psy_cfg;
	struct power_supply *psy;
};

static const struct charger_properties sgm41543_chg_props = {
	.alias_name = "sgm41543",
};

static int __sgm41543_read_reg(struct sgm41543 *bq, u8 reg, u8 *data)
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

static int __sgm41543_write_reg(struct sgm41543 *bq, int reg, u8 val)
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

static int sgm41543_read_byte(struct sgm41543 *bq, u8 reg, u8 *data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __sgm41543_read_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	return ret;
}

static int sgm41543_write_byte(struct sgm41543 *bq, u8 reg, u8 data)
{
	int ret;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __sgm41543_write_reg(bq, reg, data);
	mutex_unlock(&bq->i2c_rw_lock);

	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

	return ret;
}

static int sgm41543_update_bits(struct sgm41543 *bq, u8 reg, u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	mutex_lock(&bq->i2c_rw_lock);
	ret = __sgm41543_read_reg(bq, reg, &tmp);
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}
	tmp &= ~mask;
	tmp |= data & mask;

	ret = __sgm41543_write_reg(bq, reg, tmp);
	if (ret)
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
	mutex_unlock(&bq->i2c_rw_lock);
	return ret;
}

static enum sgm41543_vbus_type sgm41543_get_vbus_type(struct sgm41543 *bq)
{
	u8 val = 0;
	int ret;

	ret = sgm41543_read_byte(bq, SGM41543_REG_08, &val);
	if (ret < 0)
		return 0;
	val &= REG08_VBUS_STAT_MASK;
	val >>= REG08_VBUS_STAT_SHIFT;

	return val;
}

static int sgm41543_enable_otg(struct sgm41543 *bq)
{
	u8 val = REG01_OTG_ENABLE << REG01_OTG_CONFIG_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_01, REG01_OTG_CONFIG_MASK,
				   val);

}

static int sgm41543_disable_otg(struct sgm41543 *bq)
{
	u8 val = REG01_OTG_DISABLE << REG01_OTG_CONFIG_SHIFT;
	pr_err("sgm41543_disable_otg\n");
	return sgm41543_update_bits(bq, SGM41543_REG_01, REG01_OTG_CONFIG_MASK,
				   val);

}

static int sgm41543_enable_charger(struct sgm41543 *bq)
{
	int ret;
	u8 val = REG01_CHG_ENABLE << REG01_CHG_CONFIG_SHIFT;

	ret =
	    sgm41543_update_bits(bq, SGM41543_REG_01, REG01_CHG_CONFIG_MASK, val);
	if (ret == 0)
		bq->status |= SGM41543_STATUS_CHARGE_ENABLE;
	return ret;
}

static int sgm41543_disable_charger(struct sgm41543 *bq)
{
	int ret;
	u8 val = REG01_CHG_DISABLE << REG01_CHG_CONFIG_SHIFT;

	ret =
	    sgm41543_update_bits(bq, SGM41543_REG_01, REG01_CHG_CONFIG_MASK, val);
	if (ret == 0)
		bq->status &= ~SGM41543_STATUS_CHARGE_ENABLE;
	return ret;
}

int sgm41543_set_chargecurrent(struct sgm41543 *bq, int curr)
{
	u8 ichg;

		if (curr < SGM4153x_ICHRG_I_MIN_uA)
			curr = SGM4153x_ICHRG_I_MIN_uA;
		else if ( curr > SGM4153x_ICHRG_I_MAX_uA)
			curr = SGM4153x_ICHRG_I_MAX_uA;

		if (curr <= 3720)
			ichg = curr / 60;
		else
			ichg = 0x3F;

	pr_err("sgm41543_set_chargecurrent : the curr is %d, the val is 0x%.2x \n", curr, ichg);

	return sgm41543_update_bits(bq, SGM41543_REG_02, REG02_ICHG_MASK,
				   ichg << REG02_ICHG_SHIFT);

}

int sgm41543_set_term_current(struct sgm41543 *bq, int curr)
{
	u8 iterm;

	for(iterm = 1; iterm < 16 && curr >= ITERM_CURRENT_STABLE[iterm]; iterm++);
	iterm--;

	pr_err(" sgm41543_set_term_current : the curr is %d, the val is 0x%.2x \n ", curr, iterm);


	return sgm41543_update_bits(bq, SGM41543_REG_03, REG03_ITERM_MASK,
				   iterm << REG03_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(sgm41543_set_term_current);

int sgm41543_set_prechg_current(struct sgm41543 *bq, int curr)
{
	u8 iprechg;

	for(iprechg = 1; iprechg < 12 && curr >= IPRECHG_CURRENT_STABLE[iprechg]; iprechg++);
	iprechg--;

	pr_err(" sgm41543_set_prechg_current : the curr is %d, the val is 0x%.2x \n ", curr, iprechg);
	return sgm41543_update_bits(bq, SGM41543_REG_03, REG03_IPRECHG_MASK,
				   iprechg << REG03_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(sgm41543_set_prechg_current);

int sgm41543_set_chargevolt(struct sgm41543 *bq, int volt)
{
	u8 val;

	if (volt < REG04_VREG_BASE)
		volt = REG04_VREG_BASE;

	val = (volt - REG04_VREG_BASE) / REG04_VREG_LSB;
	pr_err(" sgm41543_set_chargevolt : the volt is %d, the val is 0x%.2x \n ", volt, val);
	return sgm41543_update_bits(bq, SGM41543_REG_04, REG04_VREG_MASK,
				   val << REG04_VREG_SHIFT);
}

int sgm41543_set_input_volt_limit(struct sgm41543 *bq, int volt)
{
	u8 val;

	if (volt < REG06_VINDPM_BASE)
		volt = REG06_VINDPM_BASE;

	val = (volt - REG06_VINDPM_BASE) / REG06_VINDPM_LSB;
	pr_err(" sgm41543_set_volt_limit : the volt is %d, the val is 0x%.2x \n ", volt, val);
	return sgm41543_update_bits(bq, SGM41543_REG_06, REG06_VINDPM_MASK,
				   val << REG06_VINDPM_SHIFT);
}

int sgm41543_set_input_current_limit(struct sgm41543 *bq, int curr)
{
	u8 val;

	if (curr < REG00_IINLIM_BASE)
		curr = REG00_IINLIM_BASE;

	val = (curr - REG00_IINLIM_BASE) / REG00_IINLIM_LSB;
	pr_err(" sgm41543_set_input_current_limit : the curr is %d, the val is 0x%.2x \n ", curr, val);
	return sgm41543_update_bits(bq, SGM41543_REG_00, REG00_IINLIM_MASK,
				   val << REG00_IINLIM_SHIFT);
}

int sgm41543_set_watchdog_timer(struct sgm41543 *bq, u8 timeout)
{
	u8 temp;

	temp = (u8) (((timeout -
		       REG05_WDT_BASE) / REG05_WDT_LSB) << REG05_WDT_SHIFT);

	return sgm41543_update_bits(bq, SGM41543_REG_05, REG05_WDT_MASK, temp);
}
EXPORT_SYMBOL_GPL(sgm41543_set_watchdog_timer);

int sgm41543_disable_watchdog_timer(struct sgm41543 *bq)
{
	u8 val = REG05_WDT_DISABLE << REG05_WDT_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_05, REG05_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(sgm41543_disable_watchdog_timer);

int sgm41543_reset_watchdog_timer(struct sgm41543 *bq)
{
	u8 val = REG01_WDT_RESET << REG01_WDT_RESET_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_01, REG01_WDT_RESET_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_reset_watchdog_timer);

int sgm41543_reset_chip(struct sgm41543 *bq)
{
	int ret;
	u8 val = REG0B_REG_RESET << REG0B_REG_RESET_SHIFT;

	ret =
	    sgm41543_update_bits(bq, SGM41543_REG_0B, REG0B_REG_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(sgm41543_reset_chip);

int sgm41543_enter_hiz_mode(struct sgm41543 *bq)
{
	u8 val = REG00_HIZ_ENABLE << REG00_ENHIZ_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(sgm41543_enter_hiz_mode);

int sgm41543_exit_hiz_mode(struct sgm41543 *bq)
{

	u8 val = REG00_HIZ_DISABLE << REG00_ENHIZ_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(sgm41543_exit_hiz_mode);

static int sgm41543_set_hz(struct sgm41543 *bq, bool en)
{
	if (en) {
		sgm41543_enter_hiz_mode(bq);
	} else {
		sgm41543_exit_hiz_mode(bq);
		sgm41543_set_input_current_limit(bq, 2050);
	}

	return 0;
}


int sgm41543_get_hiz_mode(struct sgm41543 *bq, u8 *state)
{
	u8 val;
	int ret;

	ret = sgm41543_read_byte(bq, SGM41543_REG_00, &val);
	if (ret)
		return ret;
	*state = (val & REG00_ENHIZ_MASK) >> REG00_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(sgm41543_get_hiz_mode);

static int sgm41543_enable_term(struct sgm41543 *bq, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = REG05_TERM_ENABLE << REG05_EN_TERM_SHIFT;
	else
		val = REG05_TERM_DISABLE << REG05_EN_TERM_SHIFT;

	ret = sgm41543_update_bits(bq, SGM41543_REG_05, REG05_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(sgm41543_enable_term);

int sgm41543_set_boost_current(struct sgm41543 *bq, int curr)
{
	u8 val;

	val = REG02_BOOST_LIM_1P2A;
	if (curr == BOOSTI_2000)
		val = REG02_BOOST_LIM_2P0A;

	return sgm41543_update_bits(bq, SGM41543_REG_02, REG02_BOOST_LIM_MASK,
				   val << REG02_BOOST_LIM_SHIFT);
}

int sgm41543_set_boost_voltage(struct sgm41543 *bq, int volt)
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

	return sgm41543_update_bits(bq, SGM41543_REG_06, REG06_BOOSTV_MASK,
				   val << REG06_BOOSTV_SHIFT);
}
EXPORT_SYMBOL_GPL(sgm41543_set_boost_voltage);

static int sgm41543_set_acovp_threshold(struct sgm41543 *bq, int volt)
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

	return sgm41543_update_bits(bq, SGM41543_REG_06, REG06_OVP_MASK,
				   val << REG06_OVP_SHIFT);
}
EXPORT_SYMBOL_GPL(sgm41543_set_acovp_threshold);

static int sgm41543_set_stat_ctrl(struct sgm41543 *bq, int ctrl)
{
	u8 val;
	int ret = 0;
	val = ctrl;

	if(STAT_CTRL_STAT == val) // ctrl:0 1 3
		ret = sgm41543_update_bits(bq, SGM41543_REG_00, REG00_STAT_CTRL_MASK, REG00_STAT_CTRL_STAT);
	else if(STAT_CTRL_ICHG == val) // STAT_SET[1:0]
		ret = sgm41543_update_bits(bq, SGM41543_REG_00, REG00_STAT_CTRL_MASK, (REG00_STAT_CTRL_ICHG << REG00_STAT_CTRL_SHIFT));
	else if(STAT_CTRL_DISABLE == val)//Disable(float pin)
		ret = sgm41543_update_bits(bq, SGM41543_REG_00, REG00_STAT_CTRL_MASK, (REG00_STAT_CTRL_DISABLE << REG00_STAT_CTRL_SHIFT));
	else {
		ret = -1;
		dev_err(bq->dev, "%s: ctrl:%d ret:%d\n", __func__, val,ret);
	}

	dev_info(bq->dev, "%s: ctrl:%d ret:%d\n", __func__, val,ret);
	return ret;
}
static struct sgm41543 *g_sgm41543;
void sgm41543_enable_statpin(bool en)
{
	if (!g_sgm41543) {
		pr_err("%s: g_sgm41543 NULL\n",__func__);
		return;
	}

	if(en)
		sgm41543_set_stat_ctrl(g_sgm41543, 0);
	else
		sgm41543_set_stat_ctrl(g_sgm41543, 3);
}

static int sgm41543_set_int_mask(struct sgm41543 *bq, int mask)
{
	u8 val;

	val = mask;

	return sgm41543_update_bits(bq, SGM41543_REG_0A, REG0A_INT_MASK_MASK,
				   val << REG0A_INT_MASK_SHIFT);
}

static int sgm41543_enable_batfet(struct sgm41543 *bq)
{
	const u8 val = REG07_BATFET_ON << REG07_BATFET_DIS_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_enable_batfet);

static int sgm41543_disable_batfet(struct sgm41543 *bq)
{
	const u8 val = REG07_BATFET_OFF << REG07_BATFET_DIS_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_07, REG07_BATFET_DIS_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_disable_batfet);

static int sgm41543_set_batfet_delay(struct sgm41543 *bq, uint8_t delay)
{
	u8 val;

	if (delay == 0)
		val = REG07_BATFET_DLY_0S;
	else
		val = REG07_BATFET_DLY_10S;

	val <<= REG07_BATFET_DLY_SHIFT;

	return sgm41543_update_bits(bq, SGM41543_REG_07, REG07_BATFET_DLY_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_set_batfet_delay);

static int sgm41543_enable_safety_timer(struct sgm41543 *bq)
{
	const u8 val = REG05_CHG_TIMER_ENABLE << REG05_EN_TIMER_SHIFT;

	pr_err("chg enable safety timer\n");

	return sgm41543_update_bits(bq, SGM41543_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_enable_safety_timer);

static int sgm41543_disable_safety_timer(struct sgm41543 *bq)
{
	const u8 val = REG05_CHG_TIMER_DISABLE << REG05_EN_TIMER_SHIFT;

	pr_err("chg disable safety timer\n");

	return sgm41543_update_bits(bq, SGM41543_REG_05, REG05_EN_TIMER_MASK,
				   val);
}
EXPORT_SYMBOL_GPL(sgm41543_disable_safety_timer);

int sgm41543_force_dpdm(struct sgm41543 *bq);
static DEFINE_MUTEX(sgm41543_type_det_lock);
static int sgm41543_update_chg_type(struct charger_device *chg_dev, bool en)
{
	int ret = 0;
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	union power_supply_propval propval;
	struct power_supply *chrdet_psy;
	enum charger_type chg_type = CHARGER_UNKNOWN;
	int wait_cdp_cnt = 200, wait_plugin_cnt = 5;
	static bool first_connect = true;

	chrdet_psy = power_supply_get_by_name("charger");
	if (!chrdet_psy) {
		pr_notice("[%s]: get power supply failed\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&sgm41543_type_det_lock);
	if (en) {
		if (first_connect) {
			while (wait_cdp_cnt >= 0) {
				if (is_usb_rdy()) {
					pr_info("CDP, PASS\n");
					break;
				}
				pr_info("CDP, block\n");
				msleep(100);
				wait_cdp_cnt--;
			}
			if (wait_cdp_cnt <= 0)
				pr_info("CDP, timeout\n");
			else
				pr_info("CDP, free\n");
		}
		sgm41543_set_hz(bq, 0);
		Charger_Detect_Init();
		while (wait_plugin_cnt >= 0) {
			if (bq->status&SGM41543_STATUS_PLUGIN)
				break;
			msleep(100);
			wait_plugin_cnt--;
		}
		sgm41543_force_dpdm(bq);
		bq->vbus_type = sgm41543_get_vbus_type(bq);
		switch ((int)bq->vbus_type) {
		case SGM41543_VBUS_USB_SDP:
			chg_type = STANDARD_HOST;
			break;
		case SGM41543_VBUS_USB_CDP:
			chg_type = CHARGING_HOST;
			break;
		case SGM41543_VBUS_USB_DCP:
		case SGM41543_VBUS_MAXC:
			chg_type = STANDARD_CHARGER;
			break;
		case SGM41543_VBUS_NONSTAND:
		case SGM41543_VBUS_UNKNOWN:
			chg_type = NONSTANDARD_CHARGER;
			break;
		default:
			chg_type = CHARGER_UNKNOWN;
			break;
		}
	}

	if (chg_type != STANDARD_CHARGER) {
		Charger_Detect_Release();

#ifdef CONFIG_CUSTOMER_SUPPORT
	} else if (propval.intval != CHARGER_PD_12V) {
		//schedule_delayed_work(&bq->ico_work, 0);
		if (pe.tune_done || pe.tune_fail) {
			sgm41543_reset_pe_param();
			schedule_delayed_work(&bq->check_pe_tuneup_work, 0);
		}
#endif
	}

	//if (chg_type!=CHARGER_UNKNOWN && bq->cfg.use_absolute_vindpm)
	//	sgm41543_adjust_absolute_vindpm(bq);

	pr_info("[%s] en:%d vbus_type:%d chg_type:%d\n", __func__, en, bq->vbus_type, chg_type);
	ret = power_supply_get_property(chrdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0) {
		pr_info("[%s]: get charge type failed, ret = %d\n", __func__, ret);
		mutex_unlock(&sgm41543_type_det_lock);
		return ret;
	}
#ifdef CONFIG_CUSTOMER_SUPPORT
	if (propval.intval != CHARGER_PD_12V && propval.intval != CHARGER_PD_9V
		&& propval.intval != CHARGER_PE_12V && propval.intval != CHARGER_PE_9V) {
		propval.intval = chg_type;
		ret = power_supply_set_property(chrdet_psy,
				POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
		if (ret < 0) {
			pr_info("[%s]: set charge type failed, ret = %d\n", __func__, ret);
			mutex_unlock(&sgm41543_type_det_lock);
			return ret;
		}
	}
#else
	propval.intval = chg_type;
	ret = power_supply_set_property(chrdet_psy,
			POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0) {
		pr_info("[%s]: set charge type failed, ret = %d\n", __func__, ret);
		mutex_unlock(&sgm41543_type_det_lock);
		return ret;
	}

#endif
	mutex_unlock(&sgm41543_type_det_lock);
	return 0;
}

static struct sgm41543_platform_data *sgm41543_parse_dt(struct device_node *np,
						      struct sgm41543 *bq)
{
	int ret;
	struct sgm41543_platform_data *pdata;

	pdata = devm_kzalloc(bq->dev, sizeof(struct sgm41543_platform_data),
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
	    of_property_read_bool(np, "ti,sgm41543,charge-detect-enable");

	ret = of_property_read_u32(np, "ti,sgm41543,usb-vlim", &pdata->usb.vlim);
	if (ret) {
		pdata->usb.vlim = 4500;
		pr_err("Failed to read node of ti,sgm41543,usb-vlim\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,usb-ilim", &pdata->usb.ilim);
	if (ret) {
		pdata->usb.ilim = 2000;
		pr_err("Failed to read node of ti,sgm41543,usb-ilim\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,usb-vreg", &pdata->usb.vreg);
	if (ret) {
		pdata->usb.vreg = 4200;
		pr_err("Failed to read node of ti,sgm41543,usb-vreg\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,usb-ichg", &pdata->usb.ichg);
	if (ret) {
		pdata->usb.ichg = 2000;
		pr_err("Failed to read node of ti,sgm41543,usb-ichg\n");
	}

	pdata->enable_term = of_property_read_bool(np, "ti,sgm41543,enable_term");
	if (pdata->enable_term != 1) {
		pdata->enable_term = 1;
	}
	dev_err(bq->dev, "%s ti,sgm41543,enable-termination: %d\n", __func__, pdata->enable_term);

	ret = of_property_read_u32(np, "ti,sgm41543,stat-pin-ctrl",
				   &pdata->statctrl);
	if (ret) {
		pdata->statctrl = 0;
		pr_err("Failed to read node of ti,sgm41543,stat-pin-ctrl\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,precharge-current",
				   &pdata->iprechg);
	if (ret) {
		pdata->iprechg = 180;
		pr_err("Failed to read node of ti,sgm41543,precharge-current\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,termination-current",
				   &pdata->iterm);
	if (ret) {
		pdata->iterm = 180;
		pr_err
		    ("Failed to read node of ti,sgm41543,termination-current\n");
	}

	ret =
	    of_property_read_u32(np, "ti,sgm41543,boost-voltage",
				 &pdata->boostv);
	if (ret) {
		pdata->boostv = 5000;
		pr_err("Failed to read node of ti,sgm41543,boost-voltage\n");
	}

	ret =
	    of_property_read_u32(np, "ti,sgm41543,boost-current",
				 &pdata->boosti);
	if (ret) {
		pdata->boosti = 1200;
		pr_err("Failed to read node of ti,sgm41543,boost-current\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,vac-ovp-threshold",
				   &pdata->vac_ovp);
	if (ret) {
		pdata->vac_ovp = 14000;
		pr_err("Failed to read node of ti,sgm41543,vac-ovp-threshold\n");
	}

	ret = of_property_read_u32(np, "ti,sgm41543,charge-voltage", &pdata->charge_voltage);

	if (ret < 0) {
		pdata->charge_voltage = 4400;
	}
	dev_err(bq->dev, "%s ti,sgm41543,charge-voltage: %d\n", __func__, pdata->charge_voltage);

	ret = of_property_read_u32(np, "ti,sgm41543,charge-current", &pdata->charge_current);

	if (ret < 0) {
		pdata->charge_current = 2000;
	}
	dev_err(bq->dev, "%s ti,sgm41543,charge-current: %d\n", __func__, pdata->charge_current);

	return pdata;
}

static void sgm41543_adapter_in_workfunc(struct work_struct *work)
{
	struct sgm41543 *bq = container_of(work, struct sgm41543, adapter_in_work);
	int ret;
	struct power_supply *chrdet_psy;
	union power_supply_propval propval;

	chrdet_psy = power_supply_get_by_name("charger");
	if (chrdet_psy) {
		propval.intval = 1;
		ret = power_supply_set_property(chrdet_psy,
				POWER_SUPPLY_PROP_ONLINE, &propval);
		if (ret) {
			dev_info(bq->dev, "[%s] set chr_online failed:%d\n", __func__, ret);
			return;
		}
		ret = sgm41543_update_chg_type(bq->chg_dev, true);
		if (ret)
			dev_info(bq->dev, "[%s] update chr_type failed:%d\n", __func__, ret);

		sgm41543_set_hz(bq, 0);
	} else {
		dev_info(bq->dev, "[%s] get chrdet_psy failed:%d\n", __func__, ret);
	}
/*	if (pe.enable)
		schedule_delayed_work(&bq->monitor_work, 0);*/
}

static void sgm41543_adapter_out_workfunc(struct work_struct *work)
{
	struct sgm41543 *bq = container_of(work, struct sgm41543, adapter_out_work);
	int ret;
	struct power_supply *chrdet_psy;
	union power_supply_propval propval;

	ret = sgm41543_set_input_volt_limit(bq, 4600);
	if (ret < 0)
		dev_info(bq->dev, "%s:reset vindpm threshold to 4600 failed:%d\n", __func__, ret);
	else
		dev_info(bq->dev, "%s:reset vindpm threshold to 4600 successfully\n", __func__);

/*	if (pe.enable)
		cancel_delayed_work_sync(&bq->monitor_work);

	cancel_delayed_work_sync(&bq->pe_volt_tune_work);
	sgm41543_reset_pe_param();
	__pm_relax(bq->pe_tune_wakelock);*/

	chrdet_psy = power_supply_get_by_name("charger");
	if (chrdet_psy) {
		propval.intval = CHARGER_UNKNOWN;
		ret = power_supply_set_property(chrdet_psy,
				POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
		if (ret)
			dev_info(bq->dev, "[%s] reset chr_type failed:%d\n", __func__, ret);
		propval.intval = 0;
		ret = power_supply_set_property(chrdet_psy,
				POWER_SUPPLY_PROP_ONLINE, &propval);
		if (ret)
			dev_info(bq->dev, "[%s] reset chr_online failed:%d\n", __func__, ret);
	} else {
		dev_info(bq->dev, "[%s] get chrdet_psy failed:%d\n", __func__, ret);
	}
}
static void sgm41543_dump_regs(struct sgm41543 *bq);
static void sgm41543_charger_irq_workfunc(struct work_struct *work)
{
	struct sgm41543 *bq = container_of(work, struct sgm41543, irq_work);
	u8 status = 0;
	u8 fault = 0;
	u8 charge_status = 0;
	u8 temp = 0;
	int ret;

	mdelay(100);

	/* Read STATUS and FAULT registers */
	ret = sgm41543_read_byte(bq, SGM41543_REG_08, &status);
	if (ret)
		return;

	ret = sgm41543_read_byte(bq, SGM41543_REG_09, &fault);
	if (ret)
		return;

	ret = sgm41543_read_byte(bq, SGM41543_REG_0A, &temp);
	if (ret)
		return;

	bq->vbus_type = (status & REG08_VBUS_STAT_MASK) >> REG08_VBUS_STAT_SHIFT;
	bq->online = (status & REG08_PG_STAT_MASK) >> REG08_PG_STAT_SHIFT;

	sgm41543_dump_regs(bq);
	dev_info(bq->dev, "%s:bq status = %.2x, bq->vbus_type = %.2x, online = %.2x\n",
			__func__, bq->status, bq->vbus_type, bq->online);

	if (!(temp & REG0A_VBUS_GD_MASK) && (bq->status & SGM41543_STATUS_PLUGIN)) {
		dev_err(bq->dev, "%s:adapter removed\n", __func__);
		bq->status &= ~SGM41543_STATUS_PLUGIN;
		schedule_work(&bq->adapter_out_work);
	} else if ((temp & REG0A_VBUS_GD_MASK) && !(bq->status & SGM41543_STATUS_PLUGIN)) {
		//sgm41543_use_absolute_vindpm(bq, true);  //Note: Register is reset to default value when input source is plugged-in
		dev_err(bq->dev, "%s:adapter plugged in\n", __func__);
		bq->status |= SGM41543_STATUS_PLUGIN;
		schedule_work(&bq->adapter_in_work);
	}

	if ((status & REG08_PG_STAT_MASK) && !(bq->status & SGM41543_STATUS_PG))
		bq->status |= SGM41543_STATUS_PG;
	else if (!(status & REG08_PG_STAT_MASK) && (bq->status & SGM41543_STATUS_PG))
		bq->status &= ~SGM41543_STATUS_PG;

	if (fault && !(bq->status & SGM41543_STATUS_FAULT))
		bq->status |= SGM41543_STATUS_FAULT;
	else if (!fault && (bq->status & SGM41543_STATUS_FAULT))
		bq->status &= ~SGM41543_STATUS_FAULT;

	charge_status = (status & REG08_CHRG_STAT_MASK) >> REG08_CHRG_STAT_SHIFT;
	if (charge_status == REG08_CHRG_STAT_IDLE)
		dev_info(bq->dev, "%s:not charging\n", __func__);
	else if (charge_status == REG08_CHRG_STAT_PRECHG)
		dev_info(bq->dev, "%s:precharging\n", __func__);
	else if (charge_status == REG08_CHRG_STAT_FASTCHG)
		dev_info(bq->dev, "%s:fast charging\n", __func__);
	else if (charge_status == REG08_CHRG_STAT_CHGDONE)
		dev_info(bq->dev, "%s:charge done!\n", __func__);

	if (fault)
		dev_info(bq->dev, "%s:charge fault:%02x\n", __func__, fault);
}

static irqreturn_t sgm41543_irq_handler(int irq, void *data)
{
	struct sgm41543 *bq = data;

	dev_info(bq->dev, "in %s\n", __func__);
	schedule_work(&bq->irq_work);
	return IRQ_HANDLED;
}

static int sgm41543_init_device(struct sgm41543 *bq)
{
	int ret;

	sgm41543_disable_watchdog_timer(bq);

	sgm41543_enable_term(bq, bq->platform_data->enable_term);

	ret = sgm41543_set_stat_ctrl(bq, bq->platform_data->statctrl);
	if (ret)
		pr_err("Failed to set stat pin control mode, ret = %d\n", ret);

	ret = sgm41543_set_prechg_current(bq, bq->platform_data->iprechg);

	if (ret)
		pr_err("Failed to set prechg current, ret = %d\n", ret);

	ret = sgm41543_set_term_current(bq, bq->platform_data->iterm);
	if (ret)
		pr_err("Failed to set termination current, ret = %d\n", ret);

	ret = sgm41543_set_chargevolt(bq, bq->platform_data->charge_voltage);
	if (ret < 0) {
		dev_info(bq->dev, "%s:Failed to set charge voltage:%d\n", __func__, ret);
		return ret;
	}

	ret = sgm41543_set_input_current_limit(bq, 1500);
	if (ret < 0) {
		dev_info(bq->dev, "%s:Failed to set charge current:%d\n", __func__, ret);
		return ret;
	}

	ret = sgm41543_set_chargecurrent(bq, bq->platform_data->charge_current);
	if (ret < 0) {
		dev_info(bq->dev, "%s:Failed to set charge current:%d\n", __func__, ret);
		return ret;
	}

	ret = sgm41543_enable_charger(bq);
	if (ret < 0) {
		dev_info(bq->dev, "%s:Failed to enable charger:%d\n", __func__, ret);
		return ret;
	}

	ret = sgm41543_set_boost_voltage(bq, bq->platform_data->boostv);
	if (ret)
		pr_err("Failed to set boost voltage, ret = %d\n", ret);

	ret = sgm41543_set_boost_current(bq, bq->platform_data->boosti);
	if (ret)
		pr_err("Failed to set boost current, ret = %d\n", ret);

	ret = sgm41543_set_acovp_threshold(bq, bq->platform_data->vac_ovp);
	if (ret)
		pr_err("Failed to set acovp threshold, ret = %d\n", ret);

// disable charger safety timer
	ret = sgm41543_disable_safety_timer(bq);
	if (ret)
		pr_err("Failed to disable safety timer, ret = %d\n", ret);


	ret = sgm41543_set_int_mask(bq,
				   REG0A_IINDPM_INT_MASK |
				   REG0A_VINDPM_INT_MASK);
	if (ret)
		pr_err("Failed to set vindpm and iindpm int mask\n");

	ret = sgm41543_set_input_volt_limit(bq, 4600);
	if (ret < 0)
		dev_info(bq->dev, "%s:reset vindpm threshold to 4600 failed:%d\n", __func__, ret);
	else
		dev_info(bq->dev, "%s:reset vindpm threshold to 4600 successfully\n", __func__);

	return 0;
}

static void determine_initial_status(struct sgm41543 *bq)
{
	sgm41543_irq_handler(bq->irq, (void *) bq);
}

static int sgm41543_detect_device(struct sgm41543 *bq)
{
	int ret;
	u8 data;

	ret = sgm41543_read_byte(bq, SGM41543_REG_0B, &data);
	if (!ret) {
		bq->part_no = (data & REG0B_PN_MASK) >> REG0B_PN_SHIFT;
		bq->revision =
		    (data & REG0B_DEV_REV_MASK) >> REG0B_DEV_REV_SHIFT;
	}
	return ret;
}

static void sgm41543_dump_regs(struct sgm41543 *bq)
{
	int addr;
	u8 val;
	int ret;

	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = sgm41543_read_byte(bq, addr, &val);
		if (ret == 0)
			pr_err("Reg[%.2x] = 0x%.2x\n", addr, val);
	}
}

static ssize_t
sgm41543_show_registers(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	struct sgm41543 *bq = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sgm41543 Reg");
	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = sgm41543_read_byte(bq, addr, &val);
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
sgm41543_store_registers(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct sgm41543 *bq = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg < 0x0B) {
		sgm41543_write_byte(bq, (unsigned char) reg,
				   (unsigned char) val);
	}

	return count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, sgm41543_show_registers,
		   sgm41543_store_registers);

static struct attribute *sgm41543_attributes[] = {
	&dev_attr_registers.attr,
	NULL,
};

static const struct attribute_group sgm41543_attr_group = {
	.attrs = sgm41543_attributes,
};
static char *sgm41543_charger_supplied_to[] = {
	"battery",
};

static int sgm41543_charging(struct charger_device *chg_dev, bool enable)
{

	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 val;

	if (enable)
		ret = sgm41543_enable_charger(bq);
	else
		ret = sgm41543_disable_charger(bq);

	pr_err("%s charger %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	ret = sgm41543_read_byte(bq, SGM41543_REG_01, &val);

	if (!ret)
		bq->charge_enabled = !!(val & REG01_CHG_CONFIG_MASK);

	power_supply_changed(bq->psy);
	return ret;
}


static int sgm41543_enable_vbus(struct charger_device *chg_dev, bool enable)
{

	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;

	if (enable)
		ret = sgm41543_exit_hiz_mode(bq);
	else
		ret = sgm41543_enter_hiz_mode(bq);

	pr_err("lsw_charger %s enable_vbus %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	power_supply_changed(bq->psy);
	return ret;
}

static int sgm41543_get_vbus_enable(struct charger_device *chg_dev, bool *enable)
{

	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	unsigned char en = 0;


	ret = sgm41543_get_hiz_mode(bq, &en);
	*enable = !en;

	pr_err("lsw_charger get enable_vbus %s %s\n", enable ? "enable" : "disable",
	       !ret ? "successfully" : "failed");

	return ret;
}


static int sgm41543_plug_in(struct charger_device *chg_dev)
{

	int ret;

	ret = sgm41543_charging(chg_dev, true);

	if (ret)
		pr_err("Failed to enable charging:%d\n", ret);

	return ret;
}

static int sgm41543_plug_out(struct charger_device *chg_dev)
{
	int ret;

	ret = sgm41543_charging(chg_dev, false);

	if (ret)
		pr_err("Failed to disable charging:%d\n", ret);

	return ret;
}

static int sgm41543_dump_register(struct charger_device *chg_dev)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	sgm41543_dump_regs(bq);

	return 0;
}

static int sgm41543_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	*en = bq->charge_enabled;

	return 0;
}

static int sgm41543_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 val;

	ret = sgm41543_read_byte(bq, SGM41543_REG_08, &val);
	if (!ret) {
		val = val & REG08_CHRG_STAT_MASK;
		val = val >> REG08_CHRG_STAT_SHIFT;
		*done = (val == REG08_CHRG_STAT_CHGDONE);
	}

	return ret;
}

static int sgm41543_set_ichg(struct charger_device *chg_dev, u32 curr)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("charge curr = %d\n", curr);

	return sgm41543_set_chargecurrent(bq, curr / 1000);
}

static int sgm41543_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg;
	int ret;

	ret = sgm41543_read_byte(bq, SGM41543_REG_02, &reg_val);
	if (!ret) {
			ichg = ((u32)(reg_val & REG02_ICHG_MASK ) >> REG02_ICHG_SHIFT);
			if (ichg <= 0x3F)
				*curr = ichg * 60000;
			else
				*curr = 3780000;
	}
	pr_err("sgm41543_get_ichg = %d\n", *curr);
	return ret;
}

static int sgm41543_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
	*curr = 60 * 1000;

	return 0;
}

static int sgm41543_get_vbus(struct charger_device *chg_dev, u32 *vbus)
{
	int val = 0;
	val = pmic_get_vbus();
	*vbus = (u32)(val * 1000);
	pr_info("%s: use pmic get vbus = %d , mv = %d \n", __func__,*vbus, val);
	return 0;
}

static int sgm41543_set_vchg(struct charger_device *chg_dev, u32 volt)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("charge volt = %d\n", volt);

	return sgm41543_set_chargevolt(bq, volt / 1000);
}

static int sgm41543_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int vchg;
	int ret;

	ret = sgm41543_read_byte(bq, SGM41543_REG_04, &reg_val);
	if (!ret) {
		vchg = (reg_val & REG04_VREG_MASK) >> REG04_VREG_SHIFT;
		vchg = vchg * REG04_VREG_LSB + REG04_VREG_BASE;
		*volt = vchg * 1000;
	}

	return ret;
}

#ifndef CONFIG_MOTO_SGM41543_MIVR_DISABLE
static int sgm41543_set_ivl(struct charger_device *chg_dev, u32 volt)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("vindpm volt = %d\n", volt);

	return sgm41543_set_input_volt_limit(bq, volt / 1000);

}
#endif

static int sgm41543_set_icl(struct charger_device *chg_dev, u32 curr)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("indpm curr = %d\n", curr);

	return sgm41543_set_input_current_limit(bq, curr / 1000);
}

static int sgm41543_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int icl;
	int ret;

	ret = sgm41543_read_byte(bq, SGM41543_REG_00, &reg_val);
	if (!ret) {
		icl = (reg_val & REG00_IINLIM_MASK) >> REG00_IINLIM_SHIFT;
		icl = icl * REG00_IINLIM_LSB + REG00_IINLIM_BASE;
		*curr = icl * 1000;
	}

	return ret;

}

static int sgm41543_kick_wdt(struct charger_device *chg_dev)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	return sgm41543_reset_watchdog_timer(bq);
}

static int sgm41543_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	struct sgm41543 *bq =
		dev_get_drvdata(&chg_dev->dev);

	if (!bq->psy) {
		dev_notice(bq->dev, "%s: cannot get psy\n", __func__);
		return -ENODEV;
	}


	switch (event) {
	case EVENT_EOC:
	case EVENT_RECHARGE:
/*	case EVENT_DISCHARGE:
		pr_err("event:%d\n", event);
		power_supply_changed(bq->psy);
		break;*/
	default:
		break;
	}
	return 0;
}

static int sgm41543_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;

	u8 state = 0;
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41543_get_hiz_mode(bq,&state);
	if(!ret && state && en)
	{
		ret = sgm41543_exit_hiz_mode(bq);
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


	if (en)
		ret = sgm41543_enable_otg(bq);
	else
		ret = sgm41543_disable_otg(bq);

	pr_err("%s OTG %s\n", en ? "enable" : "disable",
			!ret ? "successfully" : "failed");

	return ret;
}

static int sgm41543_set_safety_timer(struct charger_device *chg_dev, bool en)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	if (en)
		ret = sgm41543_enable_safety_timer(bq);
	else
		ret = sgm41543_disable_safety_timer(bq);

	return ret;
}

static int sgm41543_is_safety_timer_enabled(struct charger_device *chg_dev,
					   bool *en)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = sgm41543_read_byte(bq, SGM41543_REG_05, &reg_val);

	if (!ret)
		*en = !!(reg_val & REG05_EN_TIMER_MASK);

	return ret;
}

static int sgm41543_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	struct sgm41543 *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;

	pr_err("otg curr = %d\n", curr);

	ret = sgm41543_set_boost_current(bq, curr / 1000);

	return ret;
}

int sgm41543_force_dpdm(struct sgm41543 *bq)
{
	int ret, timeout = 50;
	u8 val = REG07_FORCE_DPDM << REG07_FORCE_DPDM_SHIFT;

	ret = sgm41543_update_bits(bq, SGM41543_REG_07, REG07_FORCE_DPDM_MASK, val);
	if (ret)
		return ret;

	while (timeout >= 0) {
		ret = sgm41543_read_byte(bq, SGM41543_REG_07, &val);
		if (ret)
			return ret;
		if (!(val&REG07_FORCE_DPDM_MASK))
			break;
		msleep(20);
		timeout--;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sgm41543_force_dpdm);

static int sgm41543_test_bit(struct sgm41543 *bq, u8 cmd, u8 shift,
	bool *is_one)
{
	int ret = 0;
	u8 ret_val = 0;
	u8 data = 0;

	ret = sgm41543_read_byte(bq, cmd, &data);
	if (ret_val < 0) {
		*is_one = false;
		return ret_val;
	}

	data = ret_val & (1 << shift);
	*is_one = (data == 0 ? false : true);

	return ret_val;
}

static int sgm41543_enable_pump_express(struct sgm41543 *bq, bool en)
{
	int ret = 0, i = 0;
	bool pumpx_en = false;
	const int max_wait_times = 3;

	pr_info("%s: en = %d\n", __func__, en);
	ret = sgm41543_set_icl(bq->chg_dev, 800000);
	ret = sgm41543_set_ichg(bq->chg_dev, 2000000);
	if (ret < 0)
		return ret;

	ret = sgm41543_charging(bq->chg_dev, true);
	if (ret < 0)
		return ret;

	for (i = 0; i < max_wait_times; i++) {
		msleep(2500);
		ret = sgm41543_test_bit(bq, SGM41543_REG_0D,
			REG0D_PUMPX_UP_SHIFT, &pumpx_en);
		if (ret >= 0 && !pumpx_en)
			break;
	}
	if (i == max_wait_times) {
		pr_info("%s: pumpx done fail(%d)\n", __func__, ret);
		ret = -EIO;
	} else
		ret = 0;

	return ret;
}

static int sgm41543_en_pe_current_partern(struct charger_device *chg_dev,
						bool is_up)
{
	int ret = 0;
	struct sgm41543 *bq = charger_get_data(chg_dev);
	mutex_lock(&bq->pe_lock);
	ret = sgm41543_update_bits(bq, SGM41543_REG_0D,
				REG0D_PUMPX_EN_MASK, SGM41543_EN_PUMPX << REG0D_PUMPX_EN_SHIFT);
	if (ret <0)
		pr_info("[%s] enable PUMPX fail\n", __func__);

	if (is_up){
		ret = sgm41543_update_bits(bq, SGM41543_REG_0D,
				REG0D_PUMPX_UP_MASK, SGM41543_PUMPX_UP << REG0D_PUMPX_UP_SHIFT);
		pr_info("[%s]  set pumpx up\n", __func__);}
	else{
		ret = sgm41543_update_bits(bq, SGM41543_REG_0D,
				REG0D_PUMPX_DN_MASK, SGM41543_PUMPX_DN << REG0D_PUMPX_DN_SHIFT);
		pr_info("[%s]  set pumpx down\n", __func__);}
	if (ret < 0)
		pr_info("%s: set pumpx up/down fail\n", __func__);
	pr_info("%s: set pump\n", __func__);
//	bq2589x_dump_regs(bq);
	ret = sgm41543_enable_pump_express(bq,true);
//	bq2589x_report_fchg_type(bq);
//	bq2589x_dump_regs(bq);
	mutex_unlock(&bq->pe_lock);
	return ret;

}

static struct charger_ops sgm41543_chg_ops = {
	/* Normal charging */
	.plug_in = sgm41543_plug_in,
	.plug_out = sgm41543_plug_out,
	.dump_registers = sgm41543_dump_register,
	.enable = sgm41543_charging,

#ifdef WT_COMPILE_FACTORY_VERSION
	.enable_vbus = sgm41543_enable_vbus,
#endif

	.is_enabled = sgm41543_is_charging_enable,
	.get_charging_current = sgm41543_get_ichg,
	.set_charging_current = sgm41543_set_ichg,
	.get_input_current = sgm41543_get_icl,
	.set_input_current = sgm41543_set_icl,
	.get_constant_voltage = sgm41543_get_vchg,
	.set_constant_voltage = sgm41543_set_vchg,
	.kick_wdt = sgm41543_kick_wdt,
#ifdef CONFIG_MOTO_SGM41543_MIVR_DISABLE
	.set_mivr = NULL,
#else
	.set_mivr = sgm41543_set_ivl,
#endif
	.is_charging_done = sgm41543_is_charging_done,
	.get_min_charging_current = sgm41543_get_min_ichg,
	.enable_chg_type_det = NULL,//sgm41543_update_chg_type,
	/* Safety timer */
	.enable_safety_timer = sgm41543_set_safety_timer,
	.is_safety_timer_enabled = sgm41543_is_safety_timer_enabled,

	/* Power path */
	.enable_powerpath = sgm41543_enable_vbus,
	.is_powerpath_enabled = sgm41543_get_vbus_enable,

	/* OTG */
	.enable_otg = sgm41543_set_otg,
	.set_boost_current_limit = sgm41543_set_boost_ilmt,
	.enable_discharge = NULL,

	/* PE+/PE+20 */
	.send_ta_current_pattern = sgm41543_en_pe_current_partern,
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
	.enable_cable_drop_comp = NULL,

	/* ADC */
	.get_tchg_adc = NULL,
	.get_vbus_adc = sgm41543_get_vbus,
	/* Event */
	.event = sgm41543_do_event,
};

static struct of_device_id sgm41543_charger_match_table[] = {
	{.compatible = "ti,sgm41543-2",},
	{},
};
MODULE_DEVICE_TABLE(of, sgm41543_charger_match_table);

static int sgm41543_charge_status(struct sgm41543 *bq)
{
	u8 status = 0;
	u8 charge_status = 0;

	sgm41543_read_byte(bq, SGM41543_REG_08, &status);
	charge_status = (status & REG08_CHRG_STAT_MASK) >> REG08_CHRG_STAT_SHIFT;

	switch (charge_status) {
	case REG08_CHRG_STAT_FASTCHG:
		return POWER_SUPPLY_CHARGE_TYPE_FAST;
	case REG08_CHRG_STAT_PRECHG:
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	case REG08_CHRG_STAT_CHGDONE:
	case REG08_CHRG_STAT_IDLE:
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	default:
		return POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	}
}

static enum power_supply_property sgm41543_gauge_properties[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
//	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static int sgm41543_psy_gauge_get_property(struct power_supply *psy,
	enum power_supply_property psp, union power_supply_propval *val)
{
	struct sgm41543 *bq;
	int ret;
	u8 status;
	u8 pg_status;
	//int voltage;

	bq = (struct sgm41543 *)power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = sgm41543_read_byte(bq, SGM41543_REG_08, &status);
		if (!ret) {
			pg_status = status & REG08_PG_STAT_MASK;
			pg_status = pg_status >> REG08_PG_STAT_SHIFT;
			if (pg_status == 0) /* No input power */
			{
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			}

			status = status & REG08_CHRG_STAT_MASK;
			status = status >> REG08_CHRG_STAT_SHIFT;
			if (status == 0) /* Ready */
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			else if ((status == 1) || (status == 2)) /* Charge in progress */
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else if (status == 3) /* Charge done */
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else
				val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		}else
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/*voltage = pmic_get_vbus();
		pr_err("%s val:%d", __func__,voltage);
		if (voltage > 4400)
			val->intval = 1;
		else
			val->intval = 0;*/
		val->intval = bq->online;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = sgm41543_charge_status(bq);
		break;
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = POWER_SUPPLY_TYPE_USB_DCP;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sgm41543_psy_gauge_set_property(struct power_supply *psy,
	enum power_supply_property psp, const union power_supply_propval *val)
{
	int ret = 0;
	struct sgm41543 *bq;

	bq = (struct sgm41543 *)power_supply_get_drvdata(psy);

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

static int sgm41543_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct sgm41543 *bq;
	const struct of_device_id *match;
	struct device_node *node = client->dev.of_node;
	int ret = 0;
	int irq_gpio = 0, irqn = 0;

	bq = devm_kzalloc(&client->dev, sizeof(struct sgm41543), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->dev = &client->dev;
	bq->client = client;

	i2c_set_clientdata(client, bq);

	pr_err("sgm41543 probe enter!\n");
	mutex_init(&bq->i2c_rw_lock);
	mutex_init(&bq->pe_lock);

	ret = sgm41543_detect_device(bq);
	if (!ret) {
		bq->status |= SGM41543_STATUS_EXIST;
		dev_info(bq->dev, "%s: charger device sgm41543 detected, revision:%d\n",
			__func__, bq->revision);
	} else {
		pr_err("No sgm41543 device found!\n");
		return -ENODEV;
	}

	if (match == NULL) {
		pr_err("device tree match not found\n");
		return -EINVAL;
	}

	if (bq->part_no != *(int *)match->data)
		pr_info("part no mismatch, hw:%s, devicetree:%s\n",
			pn_str[bq->part_no], pn_str[*(int *) match->data]);

	bq->platform_data = sgm41543_parse_dt(node, bq);

	if (!bq->platform_data) {
		pr_err("No platform data provided.\n");
		return -EINVAL;
	}

	ret = sgm41543_init_device(bq);
	if (ret) {
		pr_err("Failed to init device\n");
		return ret;
	}


	bq->chg_dev = charger_device_register(bq->chg_dev_name,
					      &client->dev, bq,
					      &sgm41543_chg_ops,
					      &sgm41543_chg_props);
	if (IS_ERR_OR_NULL(bq->chg_dev)) {
		ret = PTR_ERR(bq->chg_dev);
		return ret;
	}

	ret = sysfs_create_group(&bq->dev->kobj, &sgm41543_attr_group);
	if (ret)
		dev_err(bq->dev, "failed to register sysfs. err: %d\n", ret);

	determine_initial_status(bq);

	g_sgm41543 = bq;

	bq->psy_desc.name = "sgm41543-gauge";
	bq->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
	bq->psy_desc.properties = sgm41543_gauge_properties;
	bq->psy_desc.num_properties = ARRAY_SIZE(sgm41543_gauge_properties);
	bq->psy_desc.get_property = sgm41543_psy_gauge_get_property;
	bq->psy_desc.set_property = sgm41543_psy_gauge_set_property;
	bq->psy_cfg.drv_data = bq;
	bq->psy_cfg.supplied_to = sgm41543_charger_supplied_to;
	bq->psy_cfg.num_supplicants = ARRAY_SIZE(sgm41543_charger_supplied_to);
	bq->psy = power_supply_register(bq->dev, &bq->psy_desc, &bq->psy_cfg);

	sgm41543_disable_otg(bq);//plug out OTG then reboot phone that OTG REG error config case vbus always on
	sgm41543_enable_vbus(bq->chg_dev,true);


	irq_gpio = of_get_named_gpio(bq->dev->of_node, "sgm41543,intr_gpio", 0);
	if (!gpio_is_valid(irq_gpio)) {
		dev_info(bq->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
		return -EINVAL;
	}
	ret = gpio_request(irq_gpio, "sgm41543 irq pin");
	if (ret) {
		dev_info(bq->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
		goto err_0;
	}
	gpio_direction_input(irq_gpio);
	irqn = gpio_to_irq(irq_gpio);
	if (irqn < 0) {
		dev_info(bq->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		ret = irqn;
		goto err_1;
	}
	bq->client->irq = irqn;
	INIT_WORK(&bq->irq_work, sgm41543_charger_irq_workfunc);
	INIT_WORK(&bq->adapter_in_work, sgm41543_adapter_in_workfunc);
	INIT_WORK(&bq->adapter_out_work, sgm41543_adapter_out_workfunc);

	ret = request_irq(bq->client->irq, sgm41543_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "sgm41543_charger1_irq", bq);
	if (ret) {
		dev_info(bq->dev, "%s:Request IRQ %d failed: %d\n", __func__, bq->client->irq, ret);
		goto err_irq;
	} else {
		dev_info(bq->dev, "%s:irq = %d\n", __func__, bq->client->irq);
	}
	schedule_work(&bq->irq_work);/*in case of adapter has been in when power off*/

	pr_err("sgm41543 probe successfully, Part Num:%d, Revision:%d\n!",
	       bq->part_no, bq->revision);

	return 0;

err_irq:
	cancel_work_sync(&bq->irq_work);
	cancel_work_sync(&bq->adapter_in_work);
	cancel_work_sync(&bq->adapter_out_work);
err_1:
	gpio_free(irq_gpio);
err_0:
	g_sgm41543 = NULL;
return ret;
}

static int sgm41543_charger_remove(struct i2c_client *client)
{
	struct sgm41543 *bq = i2c_get_clientdata(client);

	mutex_destroy(&bq->i2c_rw_lock);
	power_supply_unregister(bq->psy);
	sysfs_remove_group(&bq->dev->kobj, &sgm41543_attr_group);
	return 0;
}

static void sgm41543_charger_shutdown(struct i2c_client *client)
{
	struct sgm41543 *bq = i2c_get_clientdata(client);
	sgm41543_disable_otg(bq);
	dev_info(bq->dev, "%s: shutdown\n", __func__);

#ifdef WT_COMPILE_FACTORY_VERSION
	sgm41543_enable_vbus(bq->chg_dev,true);
#endif

	cancel_work_sync(&bq->irq_work);
	cancel_work_sync(&bq->adapter_in_work);
	cancel_work_sync(&bq->adapter_out_work);

	free_irq(bq->client->irq, NULL);

}

static struct i2c_driver sgm41543_charger_driver = {
	.driver = {
		   .name = "sgm41543-2",
		   .owner = THIS_MODULE,
		   .of_match_table = sgm41543_charger_match_table,
		   },

	.probe = sgm41543_charger_probe,
	.remove = sgm41543_charger_remove,
	.shutdown = sgm41543_charger_shutdown,

};

module_i2c_driver(sgm41543_charger_driver);

MODULE_DESCRIPTION("WT Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("WT Charger");

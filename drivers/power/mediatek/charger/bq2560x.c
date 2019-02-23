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
#include <linux/err.h>
#include <linux/bitops.h>
#include <linux/math64.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "mtk_charger_intf.h"
#include "bq2560x_reg.h"
#include "bq2560x.h"
#include <mt-plat/charger_type.h>
#include <mt-plat/mtk_boot.h>
#include <mt-plat/upmu_common.h>

#if 1
#undef pr_debug
#define pr_debug pr_err
#undef pr_info
#define pr_info pr_err
#undef dev_dbg
#define dev_dbg dev_err
#else
#undef pr_info
#define pr_info pr_debug
#endif

enum bq2560x_part_no {
	BQ25600 = 0x00,
	BQ25601 = 0x02,
};

enum bq2560x_charge_state {
	CHARGE_STATE_IDLE = REG08_CHRG_STAT_IDLE,
	CHARGE_STATE_PRECHG = REG08_CHRG_STAT_PRECHG,
	CHARGE_STATE_FASTCHG = REG08_CHRG_STAT_FASTCHG,
	CHARGE_STATE_CHGDONE = REG08_CHRG_STAT_CHGDONE,
};


struct bq2560x {
	struct device *dev;
	struct i2c_client *client;

	int chg_type;

	enum bq2560x_part_no part_no;
	int revision;
	int status;
	int irq;
	struct mutex i2c_rw_lock;

	struct power_supply *usb_psy;

	bool charge_enabled;/* Register bit status */

	struct bq2560x_platform_data* platform_data;
	struct charger_device *chg_dev;

};

static const struct charger_properties bq2560x_chg_props = {
	.alias_name = "bq2560x",
};

static int __bq2560x_read_reg(struct bq2560x* bq, u8 reg, u8 *data)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(bq->client, reg);
	if (ret < 0) {
		pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
		return ret;
	}

	*data = (u8)ret;
	return 0;
}

static int __bq2560x_write_reg(struct bq2560x* bq, int reg, u8 val)
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

static int bq2560x_read_byte(struct bq2560x *bq, u8 *data, u8 reg)
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

	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	}

	return ret;
}


static int bq2560x_update_bits(struct bq2560x *bq, u8 reg,
				u8 mask, u8 data)
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
	if (ret) {
		pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
	}

out:
	mutex_unlock(&bq->i2c_rw_lock);
	return ret;
}

static int bq2560x_enable_otg(struct bq2560x *bq)
{
	u8 val = REG01_OTG_ENABLE << REG01_OTG_CONFIG_SHIFT;
	pr_err("ti bq25601 debug %s: \n", __func__);
	return bq2560x_update_bits(bq, BQ2560X_REG_01,
				REG01_OTG_CONFIG_MASK, val);

}

static int bq2560x_disable_otg(struct bq2560x *bq)
{
	u8 val = REG01_OTG_DISABLE << REG01_OTG_CONFIG_SHIFT;
	pr_err("ti bq25601 debug %s: \n", __func__);
	return bq2560x_update_bits(bq, BQ2560X_REG_01,
				   REG01_OTG_CONFIG_MASK, val);

}

static int bq2560x_enable_charger(struct bq2560x *bq)
{
	int ret;

	u8 val = REG01_CHG_ENABLE << REG01_CHG_CONFIG_SHIFT;
	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);

	return ret;
}

static int bq2560x_disable_charger(struct bq2560x *bq)
{
	int ret;
	u8 val = REG01_CHG_DISABLE << REG01_CHG_CONFIG_SHIFT;
	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_CHG_CONFIG_MASK, val);
	return ret;
}

int bq2560x_set_chargecurrent(struct bq2560x *bq, int curr)
{
	u8 ichg;
	pr_err("ti bq25601 debug %s: curr:%d \n", __func__,curr);
	if (curr < REG02_ICHG_BASE)
		curr = REG02_ICHG_BASE;

	ichg = (curr - REG02_ICHG_BASE)/REG02_ICHG_LSB;
	return bq2560x_update_bits(bq, BQ2560X_REG_02, REG02_ICHG_MASK,
				ichg << REG02_ICHG_SHIFT);

}

int bq2560x_set_term_current(struct bq2560x *bq, int curr)
{
	u8 iterm;
	pr_err("ti bq25601 debug %s: curr:%d \n", __func__,curr);
	if (curr < REG03_ITERM_BASE)
		curr = REG03_ITERM_BASE;

	iterm = (curr - REG03_ITERM_BASE) / REG03_ITERM_LSB;

	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_ITERM_MASK,
				iterm << REG03_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_term_current);


int bq2560x_set_prechg_current(struct bq2560x *bq, int curr)
{
	u8 iprechg;
	pr_err("ti bq25601 debug %s: \n", __func__);
	if (curr < REG03_IPRECHG_BASE)
		curr = REG03_IPRECHG_BASE;

	iprechg = (curr - REG03_IPRECHG_BASE) / REG03_IPRECHG_LSB;
	pr_err("ti bq25601 debug %s: iprechg:%02X \n", __func__,iprechg);
	return bq2560x_update_bits(bq, BQ2560X_REG_03, REG03_IPRECHG_MASK,
				iprechg << REG03_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_prechg_current);

int bq2560x_set_chargevolt(struct bq2560x *bq, int volt)
{
	u8 val;
	pr_err("ti bq25601 debug %s: \n", __func__);
	if (volt < REG04_VREG_BASE)
		volt = REG04_VREG_BASE;

	val = (volt - REG04_VREG_BASE)/REG04_VREG_LSB;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_04, REG04_VREG_MASK,
				val << REG04_VREG_SHIFT);
}


int bq2560x_set_input_volt_limit(struct bq2560x *bq, int volt)
{
	u8 val;

	if (volt < REG06_VINDPM_BASE)
		volt = REG06_VINDPM_BASE;
	pr_err("ti bq25601 debug %s: \n", __func__);
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
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_IINLIM_MASK,
				val << REG00_IINLIM_SHIFT);
}


int bq2560x_set_watchdog_timer(struct bq2560x *bq, u8 timeout)
{
	u8 temp;

	temp = (u8)(((timeout - REG05_WDT_BASE) / REG05_WDT_LSB) << REG05_WDT_SHIFT);

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

	return bq2560x_update_bits(bq, BQ2560X_REG_01, REG01_WDT_RESET_MASK, val);
}
EXPORT_SYMBOL_GPL(bq2560x_reset_watchdog_timer);

int bq2560x_reset_chip(struct bq2560x *bq)
{
	int ret;
	u8 val = REG0B_REG_RESET << REG0B_REG_RESET_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	ret = bq2560x_update_bits(bq, BQ2560X_REG_0B, REG0B_REG_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(bq2560x_reset_chip);

int bq2560x_enter_hiz_mode(struct bq2560x *bq)
{
	u8 val = REG00_HIZ_ENABLE << REG00_ENHIZ_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(bq2560x_enter_hiz_mode);

int bq2560x_exit_hiz_mode(struct bq2560x *bq)
{

	u8 val = REG00_HIZ_DISABLE << REG00_ENHIZ_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(bq2560x_exit_hiz_mode);

int bq2560x_get_hiz_mode(struct bq2560x *bq, u8 *state)
{
	u8 val;
	int ret;

	ret = bq2560x_read_byte(bq, &val, BQ2560X_REG_00);
	if (ret)
		return ret;
	*state = (val & REG00_ENHIZ_MASK) >> REG00_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(bq2560x_get_hiz_mode);


static int bq2560x_enable_term(struct bq2560x* bq, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = REG05_TERM_ENABLE << REG05_EN_TERM_SHIFT;
	else
		val = REG05_TERM_DISABLE << REG05_EN_TERM_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
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
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
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
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_06, REG06_BOOSTV_MASK,
				val << REG06_BOOSTV_SHIFT);
}
EXPORT_SYMBOL_GPL(bq2560x_set_boost_voltage);

static int bq2560x_set_acovp_threshold(struct bq2560x *bq, int volt)
{
	u8 val;

	if (volt == VAC_OVP_14300)
		val = REG06_OVP_14P3V;
	else if (volt == VAC_OVP_10500)
		val = REG06_OVP_10P5V;
	else if (volt == VAC_OVP_6200)
		val = REG06_OVP_6P2V;
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
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_00, REG00_STAT_CTRL_MASK,
				val << REG00_STAT_CTRL_SHIFT);
}


static int bq2560x_set_int_mask(struct bq2560x *bq, int mask)
{
	u8 val;

	val = mask;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_0A, REG0A_INT_MASK_MASK,
				val << REG0A_INT_MASK_SHIFT);
}


static int bq2560x_enable_batfet(struct bq2560x *bq)
{
	const u8 val = REG07_BATFET_ON << REG07_BATFET_DIS_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_BATFET_DIS_MASK,
				val);
}
EXPORT_SYMBOL_GPL(bq2560x_enable_batfet);


static int bq2560x_disable_batfet(struct bq2560x *bq)
{
	const u8 val = REG07_BATFET_OFF << REG07_BATFET_DIS_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_BATFET_DIS_MASK,
				val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_batfet);

static int bq2560x_set_batfet_delay(struct bq2560x *bq, uint8_t delay)
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

static int bq2560x_set_vdpm_bat_track(struct bq2560x *bq)
{
	const u8 val = REG07_VDPM_BAT_TRACK_200MV << REG07_VDPM_BAT_TRACK_SHIFT;
	pr_err("ti bq25601 debug %s: val:%02X \n", __func__,val);
	return bq2560x_update_bits(bq, BQ2560X_REG_07, REG07_VDPM_BAT_TRACK_MASK,
				val);
}
EXPORT_SYMBOL_GPL(bq2560x_set_vdpm_bat_track);

static int bq2560x_enable_safety_timer(struct bq2560x *bq)
{
	const u8 val = REG05_CHG_TIMER_ENABLE << REG05_EN_TIMER_SHIFT;
	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TIMER_MASK,
				val);
}
EXPORT_SYMBOL_GPL(bq2560x_enable_safety_timer);


static int bq2560x_disable_safety_timer(struct bq2560x *bq)
{
	const u8 val = REG05_CHG_TIMER_DISABLE << REG05_EN_TIMER_SHIFT;
	return bq2560x_update_bits(bq, BQ2560X_REG_05, REG05_EN_TIMER_MASK,
				val);
}
EXPORT_SYMBOL_GPL(bq2560x_disable_safety_timer);

static struct bq2560x_platform_data* bq2560x_parse_dt(struct device *dev,
							struct bq2560x * bq)
{
    int ret;
    struct device_node *np = dev->of_node;
	struct bq2560x_platform_data* pdata;
	pr_err("ti bq25601 debug %s: \n", __func__);
	pdata = devm_kzalloc(dev, sizeof(struct bq2560x_platform_data),
						GFP_KERNEL);
	if (!pdata) {
		pr_err("Out of memory\n");
		return NULL;
	}
#if 0
	ret = of_property_read_u32(np, "ti,bq2560x,chip-enable-gpio", &bq->gpio_ce);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,chip-enable-gpio\n");
	}
#endif

    ret = of_property_read_u32(np,"ti,bq2560x,usb-vlim",&pdata->usb.vlim);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,usb-vlim\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,usb-ilim",&pdata->usb.ilim);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,usb-ilim\n");
	}
    ret = of_property_read_u32(np,"ti,bq2560x,usb-vreg",&pdata->usb.vreg);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,usb-vreg\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,usb-ichg",&pdata->usb.ichg);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,usb-ichg\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,stat-pin-ctrl",&pdata->statctrl);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,stat-pin-ctrl\n");
	}
    ret = of_property_read_u32(np,"ti,bq2560x,precharge-current",&pdata->iprechg);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,precharge-current\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,termination-current",&pdata->iterm);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,termination-current\n");
	}
    ret = of_property_read_u32(np,"ti,bq2560x,boost-voltage",&pdata->boostv);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,boost-voltage\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,boost-current",&pdata->boosti);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,boost-current\n");
	}

    ret = of_property_read_u32(np,"ti,bq2560x,vac-ovp-threshold",&pdata->vac_ovp);
    if(ret) {
		pr_err("Failed to read node of ti,bq2560x,vac-ovp-threshold\n");
	}
    return pdata;
}

static int bq2560x_init_device(struct bq2560x *bq)
{
	int ret;
	bq2560x_disable_watchdog_timer(bq);

	bq2560x_set_vdpm_bat_track(bq);

	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_set_stat_ctrl(bq, bq->platform_data->statctrl);
	if (ret)
		pr_err("Failed to set stat pin control mode, ret = %d\n",ret);
	pr_err("ti bq25601 debug %s: statctrl:%d \n", __func__,bq->platform_data->statctrl);
	ret = bq2560x_set_prechg_current(bq, bq->platform_data->iprechg);
	if (ret)
		pr_err("Failed to set prechg current, ret = %d\n",ret);
	pr_err("ti bq25601 debug %s: iprechg:%d \n", __func__,bq->platform_data->iprechg);
	ret = bq2560x_set_term_current(bq, bq->platform_data->iterm);
	if (ret)
		pr_err("Failed to set termination current, ret = %d\n",ret);
	pr_err("ti bq25601 debug %s: iterm:%d \n", __func__,bq->platform_data->iterm);
	ret = bq2560x_set_boost_voltage(bq, bq->platform_data->boostv);
	if (ret)
		pr_err("Failed to set boost voltage, ret = %d\n",ret);
	ret = bq2560x_set_boost_current(bq, bq->platform_data->boosti);
	if (ret)
		pr_err("Failed to set boost current, ret = %d\n",ret);
	ret = bq2560x_set_acovp_threshold(bq, bq->platform_data->vac_ovp);
	if (ret)
		pr_err("Failed to set acovp threshold, ret = %d\n",ret);

	ret = bq2560x_set_int_mask(bq, REG0A_IINDPM_INT_MASK | REG0A_VINDPM_INT_MASK);
	if (ret)
		pr_err("Failed to set vindpm and iindpm int mask\n");

	return 0;
}

/*
static int bq2560x_detect_device(struct bq2560x* bq)
{
    int ret;
    u8 data;

    ret = bq2560x_read_byte(bq, &data, BQ2560X_REG_0B);
    if(ret == 0){
        bq->part_no = (data & REG0B_PN_MASK) >> REG0B_PN_SHIFT;
        bq->revision = (data & REG0B_DEV_REV_MASK) >> REG0B_DEV_REV_SHIFT;
    }

    return ret;
}
*/
static void bq2560x_dump_regs(struct bq2560x *bq)
{
	int addr;
	u8 val;
	int ret;
	pr_err("ti bq25601 debug %s: \n", __func__);
	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq2560x_read_byte(bq, &val, addr);
		if (ret == 0)
			pr_err("ti bq25601 debug Reg[%.2x] = 0x%.2x\n", addr, val);
	}


}

static ssize_t bq2560x_show_registers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bq2560x *bq = dev_get_drvdata(dev);
	u8 addr;
	u8 val;
	u8 tmpbuf[200];
	int len;
	int idx = 0;
	int ret ;

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "bq2560x Reg");
	for (addr = 0x0; addr <= 0x0B; addr++) {
		ret = bq2560x_read_byte(bq, &val, addr);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,"Reg[%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t bq2560x_store_registers(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct bq2560x *bq = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;
	unsigned int val;

	ret = sscanf(buf, "%x %x", &reg, &val);
	if (ret == 2 && reg < 0x0B) {
		bq2560x_write_byte(bq, (unsigned char)reg, (unsigned char)val);
	}

	return count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, bq2560x_show_registers, bq2560x_store_registers);

static struct attribute *bq2560x_attributes[] = {
	&dev_attr_registers.attr,
	NULL,
};

static const struct attribute_group bq2560x_attr_group = {
	.attrs = bq2560x_attributes,
};

static int bq2560x_get_pg_state(struct bq2560x *bq)
{
	u8 reg_val;
	int pg_stat;
	int ret;
	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_08);
	if (!ret) {
		pg_stat = (reg_val & REG08_PG_STAT_MASK) >> REG08_PG_STAT_SHIFT;
	}
	pr_err("ti bq25601 debug %s: pg_stat:%d \n", __func__,pg_stat);
	return pg_stat;
}

static int bq2560x_get_vbus_state(struct bq2560x *bq)
{
	u8 reg_val;
	int vbus_stat;
	int ret;
	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_08);
	if (!ret) {
		vbus_stat = (reg_val & REG08_VBUS_STAT_MASK) >> REG08_VBUS_STAT_SHIFT;
	}
	pr_err("ti bq25601 debug %s: vbus_stat:%d \n", __func__,vbus_stat);
	return vbus_stat;
}


static int bq2560x_get_charger_type(struct bq2560x *bq)
{
	unsigned int vbus_stat = 0;
	unsigned int pg_stat = 0;
	enum charger_type CHR_Type_num = CHARGER_UNKNOWN;

	pr_err("ti bq25601 debug %s: \n", __func__);

	pg_stat = bq2560x_get_pg_state(bq);
	vbus_stat = bq2560x_get_vbus_state(bq);

	switch (vbus_stat) {
	case 0: /* No input */
		CHR_Type_num = CHARGER_UNKNOWN;
		break;
	case 1: /* SDP */
		CHR_Type_num = STANDARD_HOST;
		break;
	case 2: /* CDP */
		CHR_Type_num = CHARGING_HOST;
		break;
	case 3: /* DCP */
		CHR_Type_num = STANDARD_CHARGER;
		break;
	case 5: /* Unknown adapter */
		CHR_Type_num = NONSTANDARD_CHARGER;
		break;
	case 6: /* Non-standard adapter */
		CHR_Type_num = NONSTANDARD_CHARGER;
		break;
	default:
		CHR_Type_num = NONSTANDARD_CHARGER;
		break;
	}
	pr_err("ti bq25601 debug vbus_stat: 0x%x, pg_stat:0x%x CHR_Type_num:%d\n", vbus_stat, pg_stat,CHR_Type_num);
	return CHR_Type_num;
}

static int bq2560x_set_charger_type(struct bq2560x *bq)
{
	int ret = 0;
	union power_supply_propval propval;

#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
	if (bq->chg_type == STANDARD_HOST ||
	    bq->chg_type == CHARGING_HOST)
	{
		pr_err("ti bq25601 debug Charger_Detect_Release \n");
		Charger_Detect_Release();
	}
#endif

	pr_err("ti bq25601 debug %s:bq->chg_type:%d \n", __func__,bq->chg_type);
	if (bq->chg_type != CHARGER_UNKNOWN)
		propval.intval = 1;
	else
		propval.intval = 0;

	ret = power_supply_set_property(bq->usb_psy,
		POWER_SUPPLY_PROP_ONLINE, &propval);
	if (ret < 0)
		pr_err("%s: inform power supply online failed, ret = %d\n",
			__func__, ret);

	propval.intval = bq->chg_type;
	ret = power_supply_set_property(bq->usb_psy,
		POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
	if (ret < 0)
		pr_err("%s: inform power supply type failed, ret = %d\n",
			__func__, ret);

	return ret;
}

static irqreturn_t bq2560x_irq_handler(int irq, void *data)
{
	u8 pg_stat = 0;
	enum charger_type org_chg_type;
	struct bq2560x *bq = (struct bq2560x *)data;


	pr_err("ti bq25601 debug %s: \n", __func__);
	pg_stat = bq2560x_get_pg_state(bq);

	org_chg_type = bq->chg_type;
	if (pg_stat) {
		pr_err("%s: plugin\n", __func__);
		bq->chg_type = bq2560x_get_charger_type(bq);
	} else {
#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
		pr_err("ti bq25601 debug Charger_Detect_Init \n");
		Charger_Detect_Init();
#endif
		bq->chg_type = CHARGER_UNKNOWN;
		pr_err("%s: plugout\n", __func__);
	}
	if (bq->chg_type != org_chg_type)
		bq2560x_set_charger_type(bq);

	return IRQ_HANDLED;
}

static int bq2560x_register_irq(struct bq2560x *bq)
{
	int ret = 0;
	struct device_node *np;
	pr_err("ti bq25601 debug %s: \n", __func__);
	/* Parse irq number from dts */
	np = of_find_compatible_node(NULL, NULL, "mediatek,sw-charger");
	if (np)
		bq->irq = irq_of_parse_and_map(np, 0);
	else {
		pr_err("%s: cannot get node\n", __func__);
		ret = -ENODEV;
		goto err_nodev;
	}
	pr_err("%s: irq = %d\n", __func__, bq->irq);

	/* Request threaded IRQ */
	ret = devm_request_threaded_irq(bq->dev, bq->irq, NULL,
		bq2560x_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"bq2560x_irq", bq);
	if (ret < 0) {
		pr_err("%s: request thread irq failed\n", __func__);
		goto err_request_irq;
	}

	enable_irq_wake(bq->irq);
	return 0;

err_nodev:
err_request_irq:
	return ret;
}

static int bq2560x_charging(struct charger_device *chg_dev, bool enable)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret = 0;
	u8 val;
	pr_err("ti bq25601 debug %s: enable:%d\n", __func__,enable);
	if (enable)
		ret = bq2560x_enable_charger(bq);
	else
		ret = bq2560x_disable_charger(bq);
	pr_err("%s charger %s\n", enable ? "enable" : "disable",
				  !ret ? "successfully" : "failed");
	ret = bq2560x_read_byte(bq, &val, BQ2560X_REG_01);

	if (!ret)
		bq->charge_enabled = !!(val & REG01_CHG_CONFIG_MASK);
	return ret;
}

static int bq2560x_plug_in(struct charger_device *chg_dev)
{
//	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
//	enum charger_type org_chg_type;
	int ret;
	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_charging(chg_dev, true);

	if (ret)
		pr_err("Failed to enable charging:%d\n", ret);
/*
	org_chg_type = bq->chg_type;
	bq->chg_type = bq2560x_get_charger_type(bq);

	if (org_chg_type != bq->chg_type)
		bq2560x_set_charger_type(bq);
*/
	return ret;
}

static int bq2560x_plug_out(struct charger_device *chg_dev)
{
//	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
//	enum charger_type org_chg_type;
	int ret;
	pr_err("ti bq25601 debug %s: \n", __func__);
	ret = bq2560x_charging(chg_dev, false);

	if (ret)
		pr_err("Failed to disable charging:%d\n", ret);
/*
	org_chg_type = bq->chg_type;
	bq->chg_type = bq2560x_get_charger_type(bq);

	if (org_chg_type != bq->chg_type)
		bq2560x_set_charger_type(bq);
*/
	return ret;
}

static int bq2560x_dump_register(struct charger_device *chg_dev)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	pr_err("ti bq25601 debug %s: \n", __func__);
	bq2560x_dump_regs(bq);

	return 0;
}

static int bq2560x_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	pr_err("ti bq25601 debug %s: \n", __func__);
	*en = bq->charge_enabled;
	return 0;
}

static int bq2560x_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 val;
	ret = bq2560x_read_byte(bq, &val, BQ2560X_REG_08);
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

	pr_err("ti bq25601 debug charge curr = %d\n", curr);

	return bq2560x_set_chargecurrent(bq, curr/1000);
}


static int bq2560x_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int ichg;
	int ret;

	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_02);
	if (!ret) {
		ichg = (reg_val & REG02_ICHG_MASK) >> REG02_ICHG_SHIFT;
		ichg = ichg * REG02_ICHG_LSB + REG02_ICHG_BASE;
		*curr = ichg * 1000;
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
	pr_err("ti bq25601 debug charge volt = %d\n", volt);
	return bq2560x_set_chargevolt(bq, volt/1000);
}

static int bq2560x_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int vchg;
	int ret;

	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_04);
	if (!ret) {
		vchg = (reg_val & REG04_VREG_MASK) >> REG04_VREG_SHIFT;
		vchg = vchg * REG04_VREG_LSB + REG04_VREG_BASE;
		*volt = vchg * 1000;
	}

	return ret;
}

static int bq2560x_set_ivl(struct charger_device *chg_dev, u32 volt)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("ti bq25601 debug %s: vindpm volt = %d\n", __func__, volt);

	return bq2560x_set_input_volt_limit(bq, volt/1000);

}

static int bq2560x_set_icl(struct charger_device *chg_dev, u32 curr)
{

	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);

	pr_err("ti bq25601 debug %s: curr = %d\n", __func__, curr);

	return bq2560x_set_input_current_limit(bq, curr/1000);
}

static int bq2560x_get_icl(struct charger_device *chg_dev, u32 *curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	u8 reg_val;
	int icl;
	int ret;

	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_00);
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

static int bq2560x_set_otg(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	if (en)
		ret = bq2560x_enable_otg(bq);
	else
		ret = bq2560x_disable_otg(bq);

	pr_err("ti bq25601 debug %s OTG %s\n", en ? "enable" : "disable",
				!ret ? "successfully" : "failed");

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

static int bq2560x_is_safety_timer_enabled(struct charger_device *chg_dev, bool *en)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	u8 reg_val;

	ret = bq2560x_read_byte(bq, &reg_val, BQ2560X_REG_05);

	if (!ret)
		*en = !!(reg_val & REG05_EN_TIMER_MASK);
	return ret;
}


static int bq2560x_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
	struct bq2560x *bq = dev_get_drvdata(&chg_dev->dev);
	int ret;
	pr_err("ti bq25601 debug otg curr = %d\n", curr);

	ret = bq2560x_set_boost_current(bq, curr/1000);

	return ret;
}

static int bq2560x_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	if (chg_dev == NULL)
		return -EINVAL;

	pr_info("%s: event = %d\n", __func__, event);
	switch (event) {
	case EVENT_EOC:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}

	return 0;
}

static struct charger_ops bq2560x_chg_ops = {
	/* Normal charging */
	.plug_in = bq2560x_plug_in,
	.plug_out = bq2560x_plug_out,
	.dump_registers = bq2560x_dump_register,
	.enable = bq2560x_charging,
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
	.enable_powerpath = NULL,
	.is_powerpath_enabled = NULL,

	/* OTG */
	.enable_otg = bq2560x_set_otg,
	.set_boost_current_limit = bq2560x_set_boost_ilmt,
	.enable_discharge = NULL,

	/* PE+/PE+20 */
	.send_ta_current_pattern = NULL,
	.set_pe20_efficiency_table = NULL,
	.send_ta20_current_pattern = NULL,
//	.set_ta20_reset = NULL,
	.enable_cable_drop_comp = NULL,

	/* ADC */
	.get_tchg_adc = NULL,

	.event = bq2560x_do_event,
};



static int bq2560x_charger_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct bq2560x *bq;

	int ret;
	unsigned int pg_stat = 0;
	u8 reg0b;
	pr_err("ti bq25601 debug %s: addr:%02x \n", __func__,client->addr);
	bq = devm_kzalloc(&client->dev, sizeof(struct bq2560x), GFP_KERNEL);
	if (!bq) {
		pr_err("Out of memory\n");
		return -ENOMEM;
	}

	bq->dev = &client->dev;
	bq->client = client;

	i2c_set_clientdata(client, bq);
	mutex_init(&bq->i2c_rw_lock);
	ret = bq2560x_read_byte(bq, &reg0b, BQ2560X_REG_0B);
	if(ret) {
		pr_err("No bq2560x device found!\n");
		return -ENODEV;
	}
		pr_err("ti bq25601 debug %s: REG0B:%d \n", __func__,reg0b);
	if (client->dev.of_node)
		bq->platform_data = bq2560x_parse_dt(&client->dev, bq);
	else
		bq->platform_data = client->dev.platform_data;
	if (!bq->platform_data) {
		pr_err("No platform data provided.\n");
		return -EINVAL;
	}

	bq->usb_psy = power_supply_get_by_name("charger");
	if (!bq->usb_psy) {
		pr_err("%s: get power supply failed\n", __func__);
		return -EINVAL;
	}

	bq->chg_dev = charger_device_register("primary_chg",
			&client->dev, bq,
			&bq2560x_chg_ops,
			&bq2560x_chg_props);
	if (IS_ERR_OR_NULL(bq->chg_dev)) {
		ret = PTR_ERR(bq->chg_dev);
		goto err_0;
	}
	bq2560x_dump_regs(bq);
	ret = bq2560x_init_device(bq);
	if (ret) {
		pr_err("Failed to init device\n");
		return ret;
	}
	bq2560x_dump_regs(bq);
	ret = sysfs_create_group(&bq->dev->kobj, &bq2560x_attr_group);
	if (ret) {
		dev_err(bq->dev, "failed to register sysfs. err: %d\n", ret);
	}

	/* Force charger type detection */

#if defined(CONFIG_PROJECT_PHY) || defined(CONFIG_PHY_MTK_SSUSB)
	pr_err("ti bq25601 debug Charger_Detect_Init \n");
	Charger_Detect_Init();
#endif
	msleep(50);

	pg_stat = bq2560x_get_pg_state(bq);
	if (pg_stat) {

		bq->chg_type = bq2560x_get_charger_type(bq);
		bq2560x_set_charger_type(bq);
	}

	bq2560x_register_irq(bq);

	pr_err("bq2560x probe successfully, Part Num:%d, Revision:%d\n!",
				bq->part_no, bq->revision);

	return 0;
err_0:
	return ret;
}

static int bq2560x_charger_remove(struct i2c_client *client)
{
	struct bq2560x *bq = i2c_get_clientdata(client);


	mutex_destroy(&bq->i2c_rw_lock);

	sysfs_remove_group(&bq->dev->kobj, &bq2560x_attr_group);


	return 0;
}


static void bq2560x_charger_shutdown(struct i2c_client *client)
{
}

static struct of_device_id bq2560x_charger_match_table[] = {
	{.compatible = "ti,bq25601",},
	{},
};
MODULE_DEVICE_TABLE(of,bq2560x_charger_match_table);

static const struct i2c_device_id bq2560x_charger_id[] = {
	{ "bq25601-charger", BQ25601 },
	{},
};
MODULE_DEVICE_TABLE(i2c, bq2560x_charger_id);

static struct i2c_driver bq2560x_charger_driver = {
	.driver 	= {
		.name 	= "bq2560x-charger",
		.owner 	= THIS_MODULE,
		.of_match_table = bq2560x_charger_match_table,
	},
	.id_table	= bq2560x_charger_id,
	.probe		= bq2560x_charger_probe,
	.remove		= bq2560x_charger_remove,
	.shutdown	= bq2560x_charger_shutdown,
};

module_i2c_driver(bq2560x_charger_driver);

MODULE_DESCRIPTION("TI BQ2560x Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Texas Instruments");

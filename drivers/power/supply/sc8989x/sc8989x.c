// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Southchip Semiconductor Technology(Shanghai) Co., Ltd.
 */
#define pr_fmt(fmt) "[sc8989x]:%s: " fmt, __func__

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
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <mt-plat/v1/charger_type.h>
#include <mt-plat/v1/mtk_charger.h>
#include <mt-plat/v1/charger_class.h>
#include "mtk_charger_intf.h"
#include <mt-plat/v1/mtk_battery.h>
#include "sc8989x.h"
#include <mt-plat/upmu_common.h>

#define CHARGER_EVENT_USE

extern int g_quick_charging_flag;
extern void hvdcp_chgstat_notify(void);
extern int g_charger_status;
int sc8989x_exit_hiz_mode(struct sc8989x *sc);
static int g_num = 0;
static bool up_vol_test = true;
static bool start_count_work = true;
static int error_times = 0;
static enum power_supply_usb_type sc8989x_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_CDP,  
    POWER_SUPPLY_USB_TYPE_ACA,
};

static int pn_data[] = {
    [PN_SC89890H] = 0x04,
};

static char *pn_str[] = {
    [PN_SC89890H] = "sc89890h",
};

static const struct charger_properties sc8989x_chg_props = {
    .alias_name = "sc8989x",
};

static struct charger_device *s_chg_dev_otg;
static struct power_supply_desc sc8989x_power_supply_desc;
int g_hvdcp_type;

static enum power_supply_property sc8989x_power_supply_props[] = {
    POWER_SUPPLY_PROP_MANUFACTURER,
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
    POWER_SUPPLY_PROP_USB_TYPE,
    POWER_SUPPLY_PROP_CHARGING_ENABLED,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CURRENT_MAX
};

static int __sc8989x_read_reg(struct sc8989x *sc, u8 reg, u8 *data)
{
    s32 ret;

    ret = i2c_smbus_read_byte_data(sc->client, reg);
    if (ret < 0) {
        pr_err("i2c read fail: can't read from reg 0x%02X\n", reg);
        return ret;
    }

    *data = (u8) ret;

    return 0;
}

static int __sc8989x_write_reg(struct sc8989x *sc, int reg, u8 val)
{
    s32 ret;

    ret = i2c_smbus_write_byte_data(sc->client, reg, val);
    if (ret < 0) {
        pr_err("i2c write fail: can't write 0x%02X to reg 0x%02X: %d\n",
               val, reg, ret);
        return ret;
    }
    return 0;
}

static int sc8989x_read_byte(struct sc8989x *sc, u8 reg, u8 *data)
{
    int ret;

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc8989x_read_reg(sc, reg, data);
    mutex_unlock(&sc->i2c_rw_lock);

    return ret;
}

static int sc8989x_write_byte(struct sc8989x *sc, u8 reg, u8 data)
{
    int ret;

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc8989x_write_reg(sc, reg, data);
    mutex_unlock(&sc->i2c_rw_lock);

    if (ret)
        pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

    return ret;
}

static int sc8989x_update_bits(struct sc8989x *sc, u8 reg, u8 mask, u8 data)
{
    int ret;
    u8 tmp;

    mutex_lock(&sc->i2c_rw_lock);
    ret = __sc8989x_read_reg(sc, reg, &tmp);
    if (ret) {
        pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);
        goto out;
    }

    tmp &= ~mask;
    tmp |= data & mask;

    ret = __sc8989x_write_reg(sc, reg, tmp);
    if (ret)
        pr_err("Failed: reg=%02X, ret=%d\n", reg, ret);

out:
    mutex_unlock(&sc->i2c_rw_lock);
    return ret;
}

static int sc8989x_set_key1(struct sc8989x *sc)
{
    sc8989x_write_byte(sc, SC8989X_REG_7E, SC8989X_KEY1);
    sc8989x_write_byte(sc, SC8989X_REG_7E, SC8989X_KEY2);
    sc8989x_write_byte(sc, SC8989X_REG_7E, SC8989X_KEY3);
    return sc8989x_write_byte(sc, SC8989X_REG_7E, SC8989X_KEY5);
}

static int sc8989x_enable_otg(struct regulator_dev *rdev)
{
    struct sc8989x *sc = charger_get_data(s_chg_dev_otg);
    
    Charger_Detect_Release();
    sc8989x_exit_hiz_mode(sc);
    u8 val = SC8989X_OTG_ENABLE << SC8989X_OTG_CONFIG_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_03,
                   SC8989X_OTG_CONFIG_MASK, val);
}

static int sc8989x_disable_otg(struct regulator_dev *rdev)
{
    struct sc8989x *sc = charger_get_data(s_chg_dev_otg);

    u8 val = SC8989X_OTG_DISABLE << SC8989X_OTG_CONFIG_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_03,
                   SC8989X_OTG_CONFIG_MASK, val);
}
static int sc8989x_set_vsys_min(struct sc8989x *sc, int val)
{
    u8 limit;

    limit = (val- SC8989X_SYS_MINV_BASE) / SC8989X_SYS_MINV_LSB;
    limit <<= SC8989X_SYS_MINV_SHIFT;
    return sc8989x_update_bits(sc, SC8989X_REG_03,
                SC8989X_SYS_MINV_MASK, limit);
}

static int sc8989x_is_enabled_vbus(struct regulator_dev *rdev)
{
    u8 temp = 0;
    int ret = 0;
    struct sc8989x *sc = charger_get_data(s_chg_dev_otg);

    ret = sc8989x_read_byte(sc, SC8989X_REG_03, &temp);
    return (temp & SC8989X_OTG_CONFIG_MASK)? 1 : 0;
}

static int sc8989x_enable_hvdcp(struct sc8989x *sc)
{
    int ret;
    u8 val = SC8989X_HVDCP_ENABLE << SC8989X_HVDCPEN_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_02,
                SC8989X_HVDCPEN_MASK, val);
    return ret;
}
EXPORT_SYMBOL_GPL(sc8989x_enable_hvdcp);

static int sc8989x_disable_hvdcp(struct sc8989x *sc)
{
    int ret;
    u8 val = SC8989X_HVDCP_DISABLE << SC8989X_HVDCPEN_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_02, 
                SC8989X_HVDCPEN_MASK, val);
    return ret;
}
EXPORT_SYMBOL_GPL(sc8989x_disable_hvdcp);
static int sc8989x_enable_charger(struct sc8989x *sc)
{
    int ret;
    if(sc8989x_is_enabled_vbus(NULL)){
        pr_info("sc8989x_enable_charger fail\n");
        return 0;
    } else {
        u8 val = SC8989X_CHG_ENABLE << SC8989X_CHG_CONFIG_SHIFT;

        ret = sc8989x_update_bits(sc, SC8989X_REG_03,
                SC8989X_CHG_CONFIG_MASK, val);
        pr_info("sc8989x_enable_charger success\n");
        return ret;
    }
    return 0;
}

static int sc8989x_disable_charger(struct sc8989x *sc)
{
    int ret;

    u8 val = SC8989X_CHG_DISABLE << SC8989X_CHG_CONFIG_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_03, 
                SC8989X_CHG_CONFIG_MASK, val);
    return ret;
}

int sc8989x_adc_start(struct sc8989x *sc, bool oneshot)
{
    u8 val;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_02, &val);
    if (ret < 0) {
        dev_err(sc->dev, "%s failed to read register 0x02:%d\n", __func__, ret);
        return ret;
    }

    if (((val & SC8989X_CONV_RATE_MASK) >> SC8989X_CONV_RATE_SHIFT) == SC8989X_ADC_CONTINUE_ENABLE)
        return 0; /*is doing continuous scan*/
    if (oneshot)
        ret = sc8989x_update_bits(sc, SC8989X_REG_02, SC8989X_CONV_START_MASK,
                    SC8989X_CONV_START << SC8989X_CONV_START_SHIFT);
    else
        ret = sc8989x_update_bits(sc, SC8989X_REG_02, SC8989X_CONV_RATE_MASK,  
                    SC8989X_ADC_CONTINUE_ENABLE << SC8989X_CONV_RATE_SHIFT);
    return ret;
}
EXPORT_SYMBOL_GPL(sc8989x_adc_start);

int sc8989x_adc_stop(struct sc8989x *sc)
{
    return sc8989x_update_bits(sc, SC8989X_REG_02, SC8989X_CONV_RATE_MASK, 
                SC8989X_ADC_CONTINUE_DISABLE << SC8989X_CONV_RATE_SHIFT);
}
EXPORT_SYMBOL_GPL(sc8989x_adc_stop);


int sc8989x_adc_read_battery_volt(struct sc8989x *sc)
{
    uint8_t val;
    int volt;
    int ret;
    ret = sc8989x_read_byte(sc, SC8989X_REG_0E, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read battery voltage failed :%d\n", ret);
        return ret;
    } else{
        volt = SC8989X_BATV_BASE + ((val & SC8989X_BATV_MASK) >> SC8989X_BATV_SHIFT) * SC8989X_BATV_LSB ;
        return volt;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_adc_read_battery_volt);


int sc8989x_adc_read_sys_volt(struct sc8989x *sc)
{
    uint8_t val;
    int volt;
    int ret;
    ret = sc8989x_read_byte(sc,  SC8989X_REG_0F, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read system voltage failed :%d\n", ret);
        return ret;
    } else{
        volt = SC8989X_SYSV_BASE + ((val & SC8989X_SYSV_MASK) >> SC8989X_SYSV_SHIFT) * SC8989X_SYSV_LSB ;
        return volt;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_adc_read_sys_volt);

int sc8989x_adc_read_vbus_volt(struct sc8989x *sc)
{
    uint8_t val;
    int volt;
    int ret;
    ret = sc8989x_read_byte(sc, SC8989X_REG_11, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read vbus voltage failed :%d\n", ret);
        return ret;
    } else{
        volt = SC8989X_VBUSV_BASE + ((val & SC8989X_VBUSV_MASK) >> SC8989X_VBUSV_SHIFT) * SC8989X_VBUSV_LSB ;
        return volt;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_adc_read_vbus_volt);

int sc8989x_adc_read_temperature(struct sc8989x *sc)
{
    uint8_t val;
    int temp;
    int ret;
    ret = sc8989x_read_byte(sc, SC8989X_REG_10, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read temperature failed :%d\n", ret);
        return ret;
    } else{
        temp = SC8989X_TSPCT_BASE + ((val & SC8989X_TSPCT_MASK) >> SC8989X_TSPCT_SHIFT) * SC8989X_TSPCT_LSB ;
        return temp;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_adc_read_temperature);

int sc8989x_adc_read_charge_current(struct sc8989x *sc)
{
    uint8_t val;
    int volt;
    int ret;
    ret = sc8989x_read_byte(sc, SC8989X_REG_12, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read charge current failed :%d\n", ret);
        return ret;
    } else{
        volt = (int)(SC8989X_ICHGR_BASE + ((val & SC8989X_ICHGR_MASK) >> SC8989X_ICHGR_SHIFT) * SC8989X_ICHGR_LSB) ;
        return volt;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_adc_read_charge_current);
int sc8989x_set_chargecurrent(struct sc8989x *sc, int curr)
{
    u8 ichg;

    if (curr < SC8989X_ICHG_BASE)
        curr = SC8989X_ICHG_BASE;

    ichg = (curr - SC8989X_ICHG_BASE)/SC8989X_ICHG_LSB;
    return sc8989x_update_bits(sc, SC8989X_REG_04, 
                        SC8989X_ICHG_MASK, ichg << SC8989X_ICHG_SHIFT);

}

int sc8989x_set_term_current(struct sc8989x *sc, int curr)
{
    u8 iterm;

    if (curr < SC8989X_ITERM_BASE)
        curr = SC8989X_ITERM_BASE;

    iterm = (curr + 29) / SC8989X_ITERM_LSB;// + 29 in order to seam protection

    return sc8989x_update_bits(sc, SC8989X_REG_05, 
                        SC8989X_ITERM_MASK, iterm << SC8989X_ITERM_SHIFT);

}
EXPORT_SYMBOL_GPL(sc8989x_set_term_current);

int sc8989x_set_prechg_current(struct sc8989x *sc, int curr)
{
    u8 iprechg;

    if (curr < SC8989X_IPRECHG_BASE)
        curr = SC8989X_IPRECHG_BASE;

    iprechg = (curr - SC8989X_IPRECHG_BASE) / SC8989X_IPRECHG_LSB;

    return sc8989x_update_bits(sc, SC8989X_REG_05, 
                        SC8989X_IPRECHG_MASK, iprechg << SC8989X_IPRECHG_SHIFT);

}
EXPORT_SYMBOL_GPL(sc8989x_set_prechg_current);

int sc8989x_set_chargevolt(struct sc8989x *sc, int volt)
{
    u8 val;

    if (volt < SC8989X_VREG_BASE)
        volt = SC8989X_VREG_BASE;

    val = (volt - SC8989X_VREG_BASE)/SC8989X_VREG_LSB;
    return sc8989x_update_bits(sc, SC8989X_REG_06, 
                        SC8989X_VREG_MASK, val << SC8989X_VREG_SHIFT);
}

int sc8989x_set_vindpm_offset_os(struct sc8989x *sc, u8 offset_os)
{
    int ret;
    ret = sc8989x_update_bits(sc, SC8989X_REG_0D,
                        SC8989X_FORCE_VINDPM_MASK, offset_os);
    if (ret) {
        pr_err("%s SC8989X_FORCE_VINDPM_MASK fail\n", __func__);
        return ret;
    }

    return ret;
}

int sc8989x_set_input_volt_limit(struct sc8989x *sc, int volt)
{
    u8 val;

    if (volt < SC8989X_VINDPM_BASE)
        volt = SC8989X_VINDPM_BASE;

    sc8989x_set_vindpm_offset_os(sc, SC8989X_FORCE_VINDPM_MASK);

    val = (volt - SC8989X_VINDPM_BASE) / SC8989X_VINDPM_LSB;
    return sc8989x_update_bits(sc, SC8989X_REG_0D, 
                        SC8989X_VINDPM_MASK, val << SC8989X_VINDPM_SHIFT);
}

int sc8989x_set_input_current_limit(struct sc8989x *sc, int curr)
{
    u8 val;

    if (curr < SC8989X_IINLIM_BASE)
        curr = SC8989X_IINLIM_BASE;

    val = (curr - SC8989X_IINLIM_BASE) / SC8989X_IINLIM_LSB;

    return sc8989x_update_bits(sc, SC8989X_REG_00, SC8989X_IINLIM_MASK, 
                        val << SC8989X_IINLIM_SHIFT);
}

int sc8989x_set_watchdog_timer(struct sc8989x *sc, u8 timeout)
{
    u8 val;

    val = (timeout - SC8989X_WDT_BASE) / SC8989X_WDT_LSB;
    val <<= SC8989X_WDT_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_07, 
                        SC8989X_WDT_MASK, val); 
}
EXPORT_SYMBOL_GPL(sc8989x_set_watchdog_timer);

int sc8989x_disable_watchdog_timer(struct sc8989x *sc)
{
    u8 val = SC8989X_WDT_DISABLE << SC8989X_WDT_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_07, 
                        SC8989X_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(sc8989x_disable_watchdog_timer);

int sc8989x_reset_watchdog_timer(struct sc8989x *sc)
{
    u8 val = SC8989X_WDT_RESET << SC8989X_WDT_RESET_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_03, 
                        SC8989X_WDT_RESET_MASK, val);
}
EXPORT_SYMBOL_GPL(sc8989x_reset_watchdog_timer);

int sc8989x_force_dpdm(struct sc8989x *sc)
{
    int ret;
    u8 val = SC8989X_FORCE_DPDM << SC8989X_FORCE_DPDM_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_02, 
                        SC8989X_FORCE_DPDM_MASK, val);

    pr_info("Force DPDM %s\n", !ret ? 
"successfully" : "failed");
    
    return ret;

}
EXPORT_SYMBOL_GPL(sc8989x_force_dpdm);
int sc8989x_reset_chip(struct sc8989x *sc)
{
    int ret;
    u8 val = SC8989X_RESET << SC8989X_RESET_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_14, 
                        SC8989X_RESET_MASK, val);
    return ret;
}
EXPORT_SYMBOL_GPL(sc8989x_reset_chip);

int sc8989x_enter_hiz_mode(struct sc8989x *sc)
{
    u8 val = SC8989X_HIZ_ENABLE << SC8989X_ENHIZ_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_00, 
                        SC8989X_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(sc8989x_enter_hiz_mode);

int sc8989x_exit_hiz_mode(struct sc8989x *sc)
{

    u8 val = SC8989X_HIZ_DISABLE << SC8989X_ENHIZ_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_00, 
                        SC8989X_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(sc8989x_exit_hiz_mode);

int sc8989x_set_hiz(struct charger_device *chgdev, bool en)
{
    struct sc8989x *sc = dev_get_drvdata(&chgdev->dev);
    pr_info("%s en = %d\n", __func__, en);

    if (en) {
        return sc8989x_enter_hiz_mode(sc);
    } else {
        return sc8989x_exit_hiz_mode(sc);
    }
}

int sc8989x_get_hiz_mode(struct sc8989x *sc, u8 *state)
{
    u8 val;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_00, &val);
    if (ret)
        return ret;
    *state = (val & SC8989X_ENHIZ_MASK) >> SC8989X_ENHIZ_SHIFT;

    return 0;
}
EXPORT_SYMBOL_GPL(sc8989x_get_hiz_mode);

static int sc8989x_enable_powerpath(struct charger_device *chg_dev, bool en)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_info("%s en = %d\n", __func__, en);

    return sc8989x_set_hiz(chg_dev, !en);
}

static int sc8989x_enable_ilmt(struct sc8989x *sc, bool enable)
{
    u8 val;
    int ret;

    if (enable)
        val = SC8989X_ENILIM_ENABLE << SC8989X_ENILIM_SHIFT;
    else
        val = SC8989X_ENILIM_DISABLE << SC8989X_ENILIM_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_00, 
                        SC8989X_ENILIM_MASK, val);

    return ret;
}

static int sc8989x_enable_term(struct sc8989x *sc, bool enable)
{
    u8 val;
    int ret;

    if (enable)
        val = SC8989X_TERM_ENABLE << SC8989X_EN_TERM_SHIFT;
    else
        val = SC8989X_TERM_DISABLE << SC8989X_EN_TERM_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_07, 
                        SC8989X_EN_TERM_MASK, val);

    return ret;
}
EXPORT_SYMBOL_GPL(sc8989x_enable_term);

int sc8989x_set_boost_current(struct sc8989x *sc, int curr)
{
    u8 temp;

    if (curr >= 2450)
        temp = SC8989X_BOOST_LIM_2450MA;
    else if (curr >= 2150)
        temp = SC8989X_BOOST_LIM_2150MA;
    else if (curr >= 1875)
        temp = SC8989X_BOOST_LIM_1875MA;
    else if (curr >= 1650)
        temp = SC8989X_BOOST_LIM_1650MA;
    else if (curr >= 1400)
        temp = SC8989X_BOOST_LIM_1400MA;
    else if (curr >= 1200)
        temp = SC8989X_BOOST_LIM_1200MA;
    else if (curr >= 750)
        temp = SC8989X_BOOST_LIM_750MA;
    else
        temp = SC8989X_BOOST_LIM_500MA;

    return sc8989x_update_bits(sc, SC8989X_REG_0A,
                SC8989X_BOOST_LIM_MASK,
                temp << SC8989X_BOOST_LIM_SHIFT);
}

static int sc8989x_enable_auto_dpdm(struct sc8989x *sc, bool enable)
{
    u8 val;
    int ret;

    if (enable)
        val = SC8989X_AUTO_DPDM_ENABLE << SC8989X_AUTO_DPDM_EN_SHIFT;
    else
        val = SC8989X_AUTO_DPDM_DISABLE << SC8989X_AUTO_DPDM_EN_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_02, 
                        SC8989X_AUTO_DPDM_EN_MASK, val);

    return ret;

}
EXPORT_SYMBOL_GPL(sc8989x_enable_auto_dpdm);

int sc8989x_set_boost_voltage(struct sc8989x *sc, int volt)
{
    u8 val = 0;

    if (volt < SC8989X_BOOSTV_BASE)
        volt = SC8989X_BOOSTV_BASE;
    if (volt > SC8989X_BOOSTV_BASE 
            + (SC8989X_BOOSTV_MASK >> SC8989X_BOOSTV_SHIFT) 
            * SC8989X_BOOSTV_LSB)
        volt = SC8989X_BOOSTV_BASE 
            + (SC8989X_BOOSTV_MASK >> SC8989X_BOOSTV_SHIFT) 
            * SC8989X_BOOSTV_LSB;

    val = ((volt - SC8989X_BOOSTV_BASE) / SC8989X_BOOSTV_LSB) 
            << SC8989X_BOOSTV_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_0A, 
                SC8989X_BOOSTV_MASK, val);


}
EXPORT_SYMBOL_GPL(sc8989x_set_boost_voltage);

static int sc8989x_enable_ico(struct sc8989x *sc, bool enable)
{
    u8 val;
    int ret;

    if (enable)
        val = SC8989X_ICO_ENABLE << SC8989X_ICOEN_SHIFT;
    else
        val = SC8989X_ICO_DISABLE << SC8989X_ICOEN_SHIFT;

    ret = sc8989x_update_bits(sc, SC8989X_REG_02, SC8989X_ICOEN_MASK, val);

    return ret;

}
EXPORT_SYMBOL_GPL(sc8989x_enable_ico);

static int sc8989x_read_idpm_limit(struct sc8989x *sc)
{
    uint8_t val;
    int curr;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_13, &val);
    if (ret < 0) {
        dev_err(sc->dev, "read vbus voltage failed :%d\n", ret);
        return ret;
    } else{
        curr = SC8989X_IDPM_LIM_BASE + ((val & SC8989X_IDPM_LIM_MASK) >> SC8989X_IDPM_LIM_SHIFT) * SC8989X_IDPM_LIM_LSB ;
        return curr;
    }
}
EXPORT_SYMBOL_GPL(sc8989x_read_idpm_limit);

static int sc8989x_enable_safety_timer(struct sc8989x *sc)
{
    const u8 val = SC8989X_CHG_TIMER_ENABLE << SC8989X_EN_TIMER_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_07, SC8989X_EN_TIMER_MASK,
                   val);
}
EXPORT_SYMBOL_GPL(sc8989x_enable_safety_timer);

static int sc8989x_disable_safety_timer(struct sc8989x *sc)
{
    const u8 val = SC8989X_CHG_TIMER_DISABLE << SC8989X_EN_TIMER_SHIFT;

    return sc8989x_update_bits(sc, SC8989X_REG_07, SC8989X_EN_TIMER_MASK,
                   val);

}
EXPORT_SYMBOL_GPL(sc8989x_disable_safety_timer);

static int sc8989x_enable_apsd_bc12(struct sc8989x *sc)
{
    u8 val;
    int ret;

    sc8989x_write_byte(sc, SC8989X_REG_01, SC8989X_BC12_ENABLE_KEY1);
    msleep(30);
    sc8989x_write_byte(sc, SC8989X_REG_01, SC8989X_BC12_ENABLE_KEY2);
    msleep(30);

    val = SC8989X_DPDM_ENABLE << SC8989X_CHG_DPDM_SHIFT;
    ret = sc8989x_update_bits(sc, SC8989X_REG_02, SC8989X_CHG_DPDM_MASK, val);

    return ret;
}

static int sc8989x_get_apsd_status(struct sc8989x *sc, u8 *state)
{
    u8 val;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_02, &val);
    if (ret)
        return ret;
    *state = (val & SC8989X_CHG_DPDM_MASK) >> SC8989X_CHG_DPDM_SHIFT;
    pr_info("%s state = %d\n",__func__, state);

    return ret;
}

static struct sc8989x_platform_data *sc8989x_parse_dt(struct device_node *np,
                              struct sc8989x *sc)
{
    int ret;
    int irq_gpio = 0, irqn = 0;
    int chg_en_gpio = 0;
    struct sc8989x_platform_data *pdata;

    pdata = devm_kzalloc(sc->dev, sizeof(struct sc8989x_platform_data),
                 GFP_KERNEL);
    if (!pdata)
        return NULL;

    if (of_property_read_string(np, "charger_name", &sc->chg_dev_name) < 0) {
        sc->chg_dev_name = "primary_chg";
        pr_warn("no charger name\n");
    }

    if (of_property_read_string(np, "eint_name", &sc->eint_name) < 0) {
        sc->eint_name = "chr_stat";
        pr_warn("no eint name\n");
    }

    sc->chg_det_enable =
        of_property_read_bool(np, "sc,sc8989x,charge-detect-enable");

    ret = of_property_read_u32(np, "sc,sc8989x,usb-vlim", &pdata->usb.vlim);
    if (ret) {
        pdata->usb.vlim = 4500;
        pr_err("Failed to read node of sc,sc8989x,usb-vlim\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,usb-ilim", &pdata->usb.ilim);
    if (ret) {
        pdata->usb.ilim = 2000;
        pr_err("Failed to read node of sc,sc8989x,usb-ilim\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,usb-vreg", &pdata->usb.vreg);
    if (ret) {
        pdata->usb.vreg = 4200;
        pr_err("Failed to read node of sc,sc8989x,usb-vreg\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,usb-ichg", &pdata->usb.ichg);
    if (ret) {
        pdata->usb.ichg = 2000;
        pr_err("Failed to read node of sc,sc8989x,usb-ichg\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,precharge-current",
                   &pdata->iprechg);
    if (ret) {
        pdata->iprechg = 180;
        pr_err("Failed to read node of sc,sc8989x,precharge-current\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,term-current",
                   &pdata->iterm);
    if (ret) {
        pdata->iterm = 180;
        pr_err
            ("Failed to read node of sc,sc8989x,term-current\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,boost-voltage",
                 &pdata->boostv);
    if (ret) {
        pdata->boostv = 5000;
        pr_err("Failed to read node of sc,sc8989x,boost-voltage\n");
    }

    ret = of_property_read_u32(np, "sc,sc8989x,boost-current",
                 &pdata->boosti);
    if (ret) {
        pdata->boosti = 1200;
        pr_err("Failed to read node of sc,sc8989x,boost-current\n");
    }

    irq_gpio = of_get_named_gpio(np, "sc,irq-gpio", 0);
    if (!gpio_is_valid(irq_gpio))
    {
        dev_err(sc->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
    }
//    ret = gpio_request(irq_gpio, "sc89890 irq pin");
//    if (ret) {
//        dev_err(sc->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
//    }
    gpio_direction_input(irq_gpio);
    irqn = gpio_to_irq(irq_gpio);
    if (irqn < 0) {
        dev_err(sc->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
    }
    sc->irq = irqn;
    
    chg_en_gpio = of_get_named_gpio(np, "sc,chg-en-gpio", 0);
    if (!gpio_is_valid(chg_en_gpio))
    {
        dev_err(sc->dev, "%s: %d gpio get failed\n", __func__, chg_en_gpio);
    }
  //  ret = gpio_request(chg_en_gpio, "sc chg en pin");
    //if (ret) {
      //  dev_err(sc->dev, "%s: %d gpio request failed\n", __func__, chg_en_gpio);
   // }
    gpio_direction_output(chg_en_gpio,0);//default enable charge

    return pdata;
}

static int sc8989x_get_charger_type(struct sc8989x *sc, enum charger_type *type)
{
    int ret;

    u8 reg_val = 0;
    int vbus_stat = 0;
    enum charger_type chg_type = CHARGER_UNKNOWN;

    ret = sc8989x_read_byte(sc, SC8989X_REG_0B, &reg_val);

    if (ret)
        return ret;

    vbus_stat = (reg_val & SC8989X_VBUS_STAT_MASK);
    vbus_stat >>= SC8989X_VBUS_STAT_SHIFT;

    switch (vbus_stat) {

    case SC8989X_VBUS_TYPE_NONE:
        chg_type = CHARGER_UNKNOWN;
        break;
    case SC8989X_VBUS_TYPE_SDP:
        chg_type = STANDARD_HOST;
        break;
    case SC8989X_VBUS_TYPE_CDP:
        chg_type = CHARGING_HOST;
        break;
    case SC8989X_VBUS_TYPE_DCP:
        chg_type = STANDARD_CHARGER;
        break;
    case SC8989X_VBUS_TYPE_HVDCP:

        chg_type = STANDARD_CHARGER;
    case SC8989X_VBUS_TYPE_UNKNOWN:
        chg_type = NONSTANDARD_CHARGER;
        break;
    case SC8989X_VBUS_TYPE_NON_STD:
        chg_type = NONSTANDARD_CHARGER;
        break;
    default:
        chg_type = NONSTANDARD_CHARGER;
        break;
    }

    *type = chg_type;

    return 0;
}

static int sc8989x_get_state(struct sc8989x *sc,
                 struct sc8989x_state *state)
{
    u8 chrg_stat;
    u8 fault;
    u8 chrg_param_0,chrg_param_1,chrg_param_2;
    int ret;

    msleep(1); /*for  BC12*/
    ret = sc8989x_read_byte(sc, SC8989X_REG_0B, &chrg_stat);
    if (ret){
        ret = sc8989x_read_byte(sc, SC8989X_REG_0B, &chrg_stat);
        if (ret){
            pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n", __func__, sc->state.chrg_type, sc->state.chrg_stat, sc->state.online);
            pr_err("%s read SC8989x_CHRG_STAT fail\n",__func__);
            return ret;
        }
    }
    state->chrg_type = chrg_stat & SC8989X_VBUS_STAT_MASK;
    state->chrg_stat = (chrg_stat & SC8989X_CHRG_STAT_MASK) >> SC8989X_CHRG_STAT_SHIFT;
    state->online = !!(chrg_stat & SC8989X_PG_STAT_MASK);
    state->therm_stat = !!(chrg_stat & SC8989X_SDP_STAT_MASK);
    state->vsys_stat = !!(chrg_stat & SC8989X_VSYS_STAT_MASK);
    
    pr_err("%s chrg_type =%d,chrg_stat =%d online = %d\n",__func__,state->chrg_type,state->chrg_stat,state->online);
    

    ret = sc8989x_read_byte(sc, SC8989X_REG_0C, &fault);
    if (ret){
        pr_err("%s read SC8989X_REG_0C fail\n",__func__);
        return ret;
    }
    state->chrg_fault = fault;  
    state->ntc_fault = fault & SC8989X_FAULT_NTC_MASK;
    state->health = state->ntc_fault;
    ret = sc8989x_read_byte(sc, SC8989X_REG_00, &chrg_param_0);
    if (ret){
        pr_err("%s read SC8989X_REG_00 fail\n",__func__);
        return ret;
    }
    state->hiz_en = !!(chrg_param_0 & SC8989X_ENHIZ_MASK);
    
    ret = sc8989x_read_byte(sc, SC8989X_REG_07, &chrg_param_1);
    if (ret){
        pr_err("%s read SC8989X_CHRG_CTRL_5 fail\n",__func__);
        return ret;
    }
    state->term_en = !!(chrg_param_1 & SC8989X_EN_TERM_MASK);
    
    ret = sc8989x_read_byte(sc, SC8989X_REG_11, &chrg_param_2);
    if (ret){
        pr_err("%s read SC8989x_CHRG_CTRL_a fail\n",__func__);
        return ret;
    }

    state->vbus_gd = !!(chrg_param_2 & SC8989X_VBUS_GD_MASK);

    if (state->chrg_stat == 0 || state->chrg_stat == SC8989X_CHRG_STAT_CHGDONE) {
        state->chrg_en = false;
    } else {
        state->chrg_en = true;
    }

    return 0;
}

static int sc8989x_inform_charger_type(struct sc8989x *sc)
{
    int ret = 0;
    union power_supply_propval propval;

    if (!sc->psy) {
        sc->psy = power_supply_get_by_name("charger");
        if (!sc->psy)
            return -ENODEV;
    }

    if (sc->chg_type != CHARGER_UNKNOWN)
        propval.intval = 1;
    else
        propval.intval = 0;

    ret = power_supply_set_property(sc->psy, POWER_SUPPLY_PROP_ONLINE,
                    &propval);

    if (ret < 0)
        pr_notice("inform power supply online failed:%d\n", ret);

    propval.intval = sc->chg_type;

    ret = power_supply_set_property(sc->psy,
                    POWER_SUPPLY_PROP_CHARGE_TYPE,
                    &propval);

    if (ret < 0)
        pr_notice("inform power supply charge type failed:%d\n", ret);

    return ret;
}


static irqreturn_t sc8989x_irq_handler(int irq, void *data)
{
    int ret;
    u8 reg_val;
    bool prev_vbus_gd;
    struct sc8989x *sc = data;

    ret = sc8989x_read_byte(sc, SC8989X_REG_11, &reg_val);
    if (ret)
        return IRQ_HANDLED;

    prev_vbus_gd = sc->vbus_good;
    sc->vbus_good = !!(reg_val & SC8989X_VBUS_GD_MASK);

    if (!prev_vbus_gd && sc->vbus_good){
        pr_err("adapter/usb inserted\n");
        sc->first_force = false;
    }
    //lock wakelock
    pr_err("%s entry\n",__func__);
    
    schedule_delayed_work(&sc->charge_detect_delayed_work, 100);

    return IRQ_HANDLED;
}

static int sc8989x_register_interrupt(struct sc8989x *sc)
{
    int ret = 0;

    pr_info("irq = %d\n", sc->irq);
    ret = devm_request_threaded_irq(sc->dev, sc->irq, NULL,
                    sc8989x_irq_handler,
                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                    dev_name(sc->dev), sc);
    if (ret < 0) {
        pr_err("request thread irq failed:%d\n", ret);
        return ret;
    }

    enable_irq_wake(sc->irq);

    return 0;
}

static int sc8989x_init_device(struct sc8989x *sc)
{
    int ret;

    sc8989x_disable_watchdog_timer(sc);

    ret = sc8989x_enable_ilmt(sc, false);
    if (ret)
        pr_err("Failed to disable iinlimit ret = %d\n", ret);

    ret = sc8989x_set_prechg_current(sc, sc->platform_data->iprechg);
    if (ret)
        pr_err("Failed to set prechg current, ret = %d\n", ret);

    ret = sc8989x_set_term_current(sc, sc->platform_data->iterm);
    if (ret)
        pr_err("Failed to set termination current, ret = %d\n", ret);

    ret = sc8989x_set_boost_voltage(sc, sc->platform_data->boostv);
    if (ret)
        pr_err("Failed to set boost voltage, ret = %d\n", ret);

    ret = sc8989x_set_boost_current(sc, sc->platform_data->boosti);
    if (ret)
        pr_err("Failed to set boost current, ret = %d\n", ret);

    return 0;
}

static void determine_initial_status(struct sc8989x *sc)
{
    sc8989x_irq_handler(sc->irq, (void *) sc);
}

static int sc8989x_detect_device(struct sc8989x *sc)
{
    int ret;
    u8 data;

    ret = sc8989x_read_byte(sc, SC8989X_REG_14, &data);
    if (!ret) {
        sc->part_no = (data & SC8989X_PN_MASK) >> SC8989X_PN_SHIFT;
        sc->revision =
            (data & SC8989X_DEV_REV_MASK) >> SC8989X_DEV_REV_SHIFT;
    }

    return ret;
}

static void sc8989x_dump_regs(struct sc8989x *sc)
{
    int addr;
    u8 val;
    int ret;

    for (addr = 0x0; addr <= 0x14; addr++) {
        ret = sc8989x_read_byte(sc, addr, &val);
        if (ret == 0)
            pr_err("Reg[%.2x] = 0x%.2x\n", addr, val);
    }
}

static ssize_t
sc8989x_show_registers(struct device *dev, struct device_attribute *attr,
               char *buf)
{
    struct sc8989x *sc = dev_get_drvdata(dev);
    u8 addr;
    u8 val;
    u8 tmpbuf[200];
    int len;
    int idx = 0;
    int ret;

    idx = snprintf(buf, PAGE_SIZE, "%s:\n", "sc8989x Reg");
    for (addr = 0x0; addr <= 0x14; addr++) {
        ret = sc8989x_read_byte(sc, addr, &val);
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
sc8989x_store_registers(struct device *dev,
            struct device_attribute *attr, const char *buf,
            size_t count)
{
    struct sc8989x *sc = dev_get_drvdata(dev);
    int ret;
    unsigned int reg;
    unsigned int val;

    ret = sscanf(buf, "%x %x", &reg, &val);
    if (ret == 2 && reg < 0x14) {
        sc8989x_write_byte(sc, (unsigned char) reg,
                   (unsigned char) val);
    }

    return count;
}

static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, sc8989x_show_registers,
           sc8989x_store_registers);

static struct attribute *sc8989x_attributes[] = {
    &dev_attr_registers.attr,
    NULL,
};

static const struct attribute_group sc8989x_attr_group = {
    .attrs = sc8989x_attributes,
};

static int sc8989x_charging(struct charger_device *chg_dev, bool enable)
{

    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    int ret = 0;
    u8 val;

    if (enable)
        ret = sc8989x_enable_charger(sc);
    else
        ret = sc8989x_disable_charger(sc);

    pr_err("%s charger %s\n", enable ? "enable" : "disable",
           !ret ? "successfully" : "failed");

    ret = sc8989x_read_byte(sc, SC8989X_REG_03, &val);

    if (!ret)
        sc->charge_enabled = !!(val & SC8989X_CHG_CONFIG_MASK);

    return ret;
}

static int sc8989x_plug_in(struct charger_device *chg_dev)
{
    int ret;

    ret = sc8989x_charging(chg_dev, true);

    if (ret)
        pr_err("Failed to enable charging:%d\n", ret);

    return ret;
}

static int sc8989x_plug_out(struct charger_device *chg_dev)
{
    int ret;

    ret = sc8989x_charging(chg_dev, false);

    if (ret)
        pr_err("Failed to disable charging:%d\n", ret);

    return ret;
}

static int sc8989x_dump_register(struct charger_device *chg_dev)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    sc8989x_dump_regs(sc);

    return 0;
}

static int sc8989x_is_charging_enable(struct charger_device *chg_dev, bool *en)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    *en = sc->charge_enabled;

    return 0;
}

static int sc8989x_is_charging_done(struct charger_device *chg_dev, bool *done)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    int ret;
    u8 val;

    ret = sc8989x_read_byte(sc, SC8989X_REG_0B, &val);
    if (!ret) {
        val = val & SC8989X_CHRG_STAT_MASK;
        val = val >> SC8989X_CHRG_STAT_SHIFT;
        *done = (val == SC8989X_CHRG_STAT_CHGDONE);
    }

    return ret;
}

static int sc8989x_set_ichg(struct charger_device *chg_dev, u32 curr)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_err("charge curr = %d\n", curr);

    return sc8989x_set_chargecurrent(sc, curr / 1000);
}

static int sc8989x_get_ichg(struct charger_device *chg_dev, u32 *curr)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    u8 reg_val;
    int ichg;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_04, &reg_val);
    if (!ret) {
        ichg = (reg_val & SC8989X_ICHG_MASK) >> SC8989X_ICHG_SHIFT;
        ichg = ichg * SC8989X_ICHG_LSB + SC8989X_ICHG_BASE;
        *curr = ichg * 1000;
    }

    return ret;
}

static int sc8989x_get_min_ichg(struct charger_device *chg_dev, u32 *curr)
{
    *curr = 60 * 1000;

    return 0;
}

static int sc8989x_set_vchg(struct charger_device *chg_dev, u32 volt)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_err("charge volt = %d\n", volt);

    return sc8989x_set_chargevolt(sc, volt / 1000);
}

static int sc8989x_get_vchg(struct charger_device *chg_dev, u32 *volt)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    u8 reg_val;
    int vchg;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_06, &reg_val);
    if (!ret) {
        vchg = (reg_val & SC8989X_VREG_MASK) >> SC8989X_VREG_SHIFT;
        vchg = vchg * SC8989X_VREG_LSB + SC8989X_VREG_BASE;
        *volt = vchg * 1000;
    }

    return ret;
}

static int sc8989x_set_ivl(struct charger_device *chg_dev, u32 volt)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_err("vindpm volt = %d\n", volt);

    return sc8989x_set_input_volt_limit(sc, volt / 1000);

}

static int sc8989x_set_icl(struct charger_device *chg_dev, u32 curr)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_err("indpm curr = %d\n", curr);

    return sc8989x_set_input_current_limit(sc, curr / 1000);
}

static int sc8989x_get_icl(struct charger_device *chg_dev, u32 *curr)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    u8 reg_val;
    int icl;
    int ret;

    ret = sc8989x_read_byte(sc, SC8989X_REG_00, &reg_val);
    if (!ret) {
        icl = (reg_val & SC8989X_IINLIM_MASK) >> SC8989X_IINLIM_SHIFT;
        icl = icl * SC8989X_IINLIM_LSB + SC8989X_IINLIM_BASE;
        *curr = icl * 1000;
    }

    return ret;
}

int sc8989x_set_ieoc(struct charger_device *chg_dev, u32 uA)
{
    int ret = 0;

    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    pr_err("%s sc8989x_set_ieoc %d\n", __func__, uA);

    ret = sc8989x_set_term_current(sc, uA/1000);

    return ret;
}

static int sc8989x_kick_wdt(struct charger_device *chg_dev)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    return sc8989x_reset_watchdog_timer(sc);
}

static int sc8989x_set_otg(struct charger_device *chg_dev, bool en)
{
    int ret;
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);

    if (en) {
        ret = sc8989x_disable_charger(sc);
        ret |= sc8989x_enable_otg(NULL);
    }
    else {
        ret = sc8989x_disable_otg(NULL);
        ret |= sc8989x_enable_charger(sc);
    }

    pr_err("%s OTG %s\n", en ? "enable" : "disable",
           !ret ? "successfully" : "failed");

    return ret;
}

static int sc8989x_set_safety_timer(struct charger_device *chg_dev, bool en)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    int ret;

    if (en)
        ret = sc8989x_enable_safety_timer(sc);
    else
        ret = sc8989x_disable_safety_timer(sc);

    return ret;
}

static int sc8989x_is_safety_timer_enabled(struct charger_device *chg_dev,
                       bool *en)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    int ret;
    u8 reg_val;

    ret = sc8989x_read_byte(sc, SC8989X_REG_07, &reg_val);

    if (!ret)
        *en = !!(reg_val & SC8989X_EN_TIMER_MASK);

    return ret;
}

static int sc8989x_set_boost_ilmt(struct charger_device *chg_dev, u32 curr)
{
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    int ret;

    pr_err("otg curr = %d\n", curr);

    ret = sc8989x_set_boost_current(sc, curr / 1000);

    return ret;
}


static int32_t sc8989x_set_dpdm( struct sc8989x *sc, uint8_t dp_val, uint8_t dm_val)
{
    uint8_t data_reg = 0;

    uint8_t mask = SC8989X_DMDAC_MASK|SC8989X_DPDAC_MASK ;

    data_reg  = (dp_val << SC8989X_DPDAC_SHIFT) & SC8989X_DPDAC_MASK;
    data_reg |= (dm_val << SC8989X_DMDAC_SHIFT) & SC8989X_DMDAC_MASK;

   return sc8989x_update_bits(sc, SC8989X_REG_01, mask, data_reg);

}

int32_t sc8989x_charging_enable_hvdcp30(struct  sc8989x *sc, bool enable)
{
    //enter continuous mode DP 0.6, DM 3.3
    return  sc8989x_set_dpdm(sc, SC8989X_DM_0P6V, SC8989X_DM_3P3V);
}

int32_t  sc8989x_charging_set_hvdcp30(struct  sc8989x *sc, bool increase)
{
    int32_t ret = 0;


    if (increase) {
        //DP 3.3, DM 3.3
        ret =  sc8989x_set_dpdm(sc, SC8989X_DM_3P3V, SC8989X_DM_3P3V);
        if (ret < 0)
            return ret;
        //need test 3ms
        mdelay(3);
        //DP 0.6, DM 3.3
        ret =  sc8989x_set_dpdm(sc, SC8989X_DM_0P6V, SC8989X_DM_3P3V);
        if (ret < 0)
            return ret;
        mdelay(3);
    } else {
        //DP 0.6, DM 3.3
        ret =  sc8989x_set_dpdm(sc, SC8989X_DM_0P6V, SC8989X_DM_0P6V);
        if (ret < 0)
            return ret;
        //need test
        mdelay(3);
        //DP 0.6, DM 3.3
        ret =  sc8989x_set_dpdm(sc, SC8989X_DM_0P6V, SC8989X_DM_3P3V);
        if (ret < 0)
            return ret;
        mdelay(3);
    }
    return 0;
}

static void chg_type_hvdcp30_work(struct work_struct *work)
{
    struct sc8989x * sc = container_of(work, struct sc8989x, hvdcp_work.work);
    int vbus;
    /*Set cur is 500ma before do BC1.2 & QC*/
    if (sc->chg_consumer != NULL) {
        charger_manager_set_qc_input_current_limit(sc->chg_consumer, 0, 500000);
        charger_manager_set_qc_charging_current_limit(sc->chg_consumer, 0, 500000);
        sc8989x_set_input_current_limit(sc, 500);
        sc8989x_set_chargecurrent(sc,500);
    }
    //sgm4154x_charging_set_hvdcp20(sgm,9);
    sc8989x_charging_enable_hvdcp30(sc,true);
    msleep(50);
    if (start_count_work) {
        pr_err("%s start count work\n",__func__);
        schedule_delayed_work(&sc->time_test_work, 1);
        start_count_work = false;
    }
    if (g_num <= 5 && up_vol_test) {
        pr_err("%s counting time<5s, error_times = %d, g_num = %d\n",__func__, error_times, g_num);
        if (error_times++ > 4) {
            error_times = 0;
            g_num = 0;
            pr_err("%s charging times>5, begin low vol charge\n", __func__);
            up_vol_test = false;
            start_count_work = true;
            msleep(100);
        }
    }
    /* HVDCP raises the voltage */
    if (up_vol_test) {
        for (int i = 0; i < 5; i++) {
            sc8989x_charging_set_hvdcp30(sc,true);
        }
        for (i = 0; i < 5; i++) {
            vbus = battery_get_vbus();
            if(vbus > 5500) {
                break;
            }
            msleep(200);
        }
    }
    if (vbus >= 5500) {
        sc8989x_set_input_current_limit(sc,3250);
        sc8989x_set_chargecurrent(sc,3600);
        hvdcp_chgstat_notify();
    }
    /* set curr default after BC1.2*/
    if (sc->chg_consumer != NULL) {
        charger_manager_set_qc_input_current_limit(sc->chg_consumer, 0, -1);
        charger_manager_set_qc_charging_current_limit(sc->chg_consumer, 0, -1);
    }
    pr_err("%s HVDCP %d\n",__func__,vbus);
    return ;
}

static void time_test_work_func(struct work_struct *work)
{
    struct sc8989x *sc = NULL;
    struct delayed_work *time_test_work = NULL;
    struct sc8989x_state state;
    int ret = 0;

    time_test_work = container_of(work, struct delayed_work, work);
    if(time_test_work == NULL) {
        pr_err("Cann't get time_test_work\n");
        return ;
    }
    sc = container_of(time_test_work, struct sc8989x, time_test_work);
    if(sc == NULL) {
        pr_err("Cann't get sc \n");
        return ;
    }

    ret = sc8989x_get_state(sc, &state);
    if (ret) {
        pr_err("%s: get reg state error\n", __func__);
        return ;
    }

    mutex_lock(&sc->lock);
    sc->state = state;
    mutex_unlock(&sc->lock);
    g_num++;
    if (g_num < 5) {
        schedule_delayed_work(&sc->time_test_work, 1000);
    } else {
        g_num = 0;
        error_times = 0;
        pr_err("%s counting times>5s, restart count num\n", __func__);
        up_vol_test = true;
        start_count_work = true;
    }
}

static int sc8989x_get_hvdcp_type(struct charger_device *chg_dev, bool *state)
{
    if (g_hvdcp_type == true) {
       *state = true;
    }

    return 0;
}

static int sc8989x_set_key(struct sc8989x *sc)
{
    sc8989x_write_byte(sc, SC8989X_REG_7D, SC8989X_KEY1);
    sc8989x_write_byte(sc, SC8989X_REG_7D, SC8989X_KEY2);
    sc8989x_write_byte(sc, SC8989X_REG_7D, SC8989X_KEY3);
    return sc8989x_write_byte(sc, SC8989X_REG_7D, SC8989X_KEY4);
}

static void set_iindpm_vindpm_intput_disable(struct sc8989x *sc)
{
    sc8989x_set_key(sc);
    sc8989x_update_bits(sc, SC8989X_REG_88, SC8989X_VINDPM_INT_MASK_MASK,
            SC8989X_VINDPM_INT_MASK << SC8989X_VINDPM_INT_MASK_SHIFT);
    sc8989x_update_bits(sc, SC8989X_REG_88, SC8989X_IINDPM_INT_MASK_MASK,
            SC8989X_IINDPM_INT_MASK << SC8989X_IINDPM_INT_MASK_SHIFT);
    sc8989x_set_key(sc);
}

static void chg_type_apsd_work(struct work_struct *work)
{
    int ret;
    int i = 0;

    u8 state;
    struct sc8989x *sc = NULL;
    struct delayed_work *apsd_work = NULL;

    apsd_work = container_of(work, struct delayed_work, work);
    if(apsd_work == NULL) {
        pr_err("Cann't get apsd_work\n");
        return ;
    }
    sc = container_of(apsd_work, struct sc8989x, apsd_work);
    if(sc == NULL) {
        pr_err("Cann't get sc \n");
        return ;
    }
    pr_err("%s enter",__func__);

    /*Set cur is 500ma before do BC1.2 & QC*/
    charger_manager_set_qc_input_current_limit(sc->chg_consumer, 0, 500000);
    charger_manager_set_qc_charging_current_limit(sc->chg_consumer, 0, 500000);
    sc8989x_set_input_current_limit(sc, 500);
    sc8989x_set_chargecurrent(sc,500);

    msleep(1000);
    Charger_Detect_Init();
    ret = sc8989x_enable_apsd_bc12(sc);
    /*BC12 input detection is completed at most 1s*/
    for (i = 0; i < 10; i++){
        msleep(100);
        ret = sc8989x_get_apsd_status(sc,&state);
        if (!ret){
            if (state){
                pr_err("input detection not completed error ret = %d\n", state);
            } else {
                schedule_delayed_work(&sc->charge_detect_delayed_work,100);
                pr_err("input detection completed ret = %d\n", state);
                break;
            }
        } else {
            pr_err("get apsd status fail ret = %d\n", ret);
        }
    }
    Charger_Detect_Release();
    /* set curr default after BC1.2*/
    charger_manager_set_qc_input_current_limit(sc->chg_consumer, 0, -1);
    charger_manager_set_qc_charging_current_limit(sc->chg_consumer, 0, -1);

    return ;
}

static void charger_monitor_work_func(struct work_struct *work)
{
    int ret = 0;
    struct sc8989x *sc = NULL;
    struct delayed_work *charge_monitor_work = NULL;
    //static u8 last_chg_method = 0;
    struct sc8989x_state state;

    pr_err("%s --Start---\n",__func__);
    charge_monitor_work = container_of(work, struct delayed_work, work);
    if(charge_monitor_work == NULL) {
        pr_err("Cann't get charge_monitor_work\n");
        return ;
    }
    sc = container_of(charge_monitor_work, struct sc8989x, charge_monitor_work);
    if(sc == NULL) {
        pr_err("Cann't get sc \n");
        return ;
    }

    ret = sc8989x_get_state(sc, &state);
    if (ret) {
        pr_err("%s: get reg state error\n", __func__);
        goto OUT;
    }

    mutex_lock(&sc->lock);
    sc->state = state;
    mutex_unlock(&sc->lock);

    if (!state.chrg_type || (state.chrg_type == SC8989x_OTG_MODE))
        g_charger_status = POWER_SUPPLY_STATUS_DISCHARGING;
    else if (!state.chrg_stat)
        g_charger_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
    else if (state.chrg_stat == SC8989X_CHRG_STAT_CHGDONE)
        g_charger_status = POWER_SUPPLY_STATUS_FULL;
    else
        g_charger_status = POWER_SUPPLY_STATUS_CHARGING;

    if(!sc->state.vbus_gd) {
        dev_err(sc->dev, "Vbus not present, disable charge\n");
        sc8989x_disable_charger(sc);
        goto OUT;
    }
    if(!state.online)
    {
        dev_err(sc->dev, "Vbus not online\n");
        goto OUT;
    }
    sc8989x_dump_register(sc->chg_dev);

    pr_err("%s--End--\n",__func__);
OUT:
    schedule_delayed_work(&sc->charge_monitor_work, 10*HZ);
}

static void charger_detect_work_func(struct work_struct *work)
{
    static bool type_dect_again = 1;
    static u8 hvdcp_num = 3;
    struct delayed_work *charge_detect_delayed_work = NULL;
    struct sc8989x * sc = NULL;
    //static int charge_type_old = 0;
    int curr_in_limit = 0;
    u32 non_stand_current_limit = 0;
    struct sc8989x_state state;
    struct power_supply *chrgdet_psy;
    enum charger_type chrg_type = CHARGER_UNKNOWN;
    union power_supply_propval propval = {0};
    static bool fullStatus = false;
    int ret;
    u8 reg_val;

    g_hvdcp_type = false;

    pr_err("%s --do charger Detect--\n",__func__);
    charge_detect_delayed_work = container_of(work, struct delayed_work, work);
    if(charge_detect_delayed_work == NULL) {
        pr_err("Cann't get charge_detect_delayed_work\n");
        return ;
    }

    chrgdet_psy = power_supply_get_by_name("charger");
    if (!chrgdet_psy) {
        pr_notice("[%s]: get power supply failed\n", __func__);
        return;
    }

    sc = container_of(charge_detect_delayed_work, struct sc8989x, charge_detect_delayed_work);
    if(sc == NULL) {
        pr_err("Cann't get sc8989x_device\n");
        return ;
    }

    if (!sc->charger_wakelock->active)
        __pm_stay_awake(sc->charger_wakelock);

    ret = sc8989x_get_state(sc, &state);
    mutex_lock(&sc->lock);
    sc->state = state;
    mutex_unlock(&sc->lock);

    if(!sc->state.vbus_gd) {
        dev_err(sc->dev, "Vbus not present, disable charge\n");
        sc8989x_disable_hvdcp(sc);
        Charger_Detect_Init();
        g_quick_charging_flag = 0;
        type_dect_again = 1;
        hvdcp_num = 3;
        fullStatus = false;
        sc8989x_disable_charger(sc);
        /* release wakelock */
        if (sc->charger_wakelock->active)
            __pm_relax(sc->charger_wakelock);
        goto err;
    }
    if(!state.online)
    {
        dev_err(sc->dev, "Vbus not online\n");      
        sc8989x_disable_hvdcp(sc);
        Charger_Detect_Init();
        g_quick_charging_flag = 0;
        type_dect_again = 1;
        fullStatus = false;
        sc8989x_disable_charger(sc);
        /* release wakelock */
        if (sc->charger_wakelock->active)
            __pm_relax(sc->charger_wakelock);
        goto err;
    }

    if(state.online && state.chrg_stat == SC8989X_CHRG_STAT_CHGDONE) {
        dev_err(sc->dev, "charging terminated\n");
    }
    /* set vindpm 4600mv when plug-in */
    sc8989x_set_input_volt_limit(sc, 4600);

    switch(sc->state.chrg_type) {
        case SC8989x_USB_SDP:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
            chrg_type = STANDARD_HOST;
            pr_err("SC8989x charger type: SDP\n");
            curr_in_limit = 500;
            break;

        case SC8989x_USB_CDP:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
            chrg_type = CHARGING_HOST;
            pr_err("SC8989x charger type: CDP\n");
            curr_in_limit = 1500;
            break;

        case SC8989x_USB_DCP:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
            chrg_type = STANDARD_CHARGER;
            if ((hvdcp_num--) > 0) {
                sc8989x_enable_hvdcp(sc);
                sc8989x_enable_apsd_bc12(sc);
            }
            pr_err("SC8989x charger type: DCP\n");
            curr_in_limit = 2400;
            break;

        case SC8989x_USB_HVDCP:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_HVDCP;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
            chrg_type = STANDARD_CHARGER;

            /* in terminated status forbiden QC charge */
            if(state.online && (state.chrg_stat == SC8989X_CHRG_STAT_CHGDONE) && !fullStatus) {
                fullStatus = true;
            } else if (fullStatus) {
                goto err;
            }

            sc8989x_set_dpdm(sc, SC8989X_DM_0P6V, SC8989X_DM_HIZ);
            schedule_delayed_work(&sc->hvdcp_work, msecs_to_jiffies(1400));//needs delay 1.4S
            g_hvdcp_type = true;
            pr_err("SC8989x charger type: HVDCP\n");
            curr_in_limit = 3250;
            break;

        case SC8989x_USB_NON_STANDARD_AP:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_USB_ACA;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_ACA;
            chrg_type = NONSTANDARD_CHARGER;
            curr_in_limit = 1000;
            ret = sc8989x_get_icl(sc->chg_dev,&non_stand_current_limit);
            if (!ret) {
                curr_in_limit = (non_stand_current_limit / 1000);
                switch(curr_in_limit) {
                case SC8989x_APPLE_1A:
                   chrg_type = APPLE_1_0A_CHARGER;
                   break;
                case SC8989x_APPLE_2A:
                case SC8989x_APPLE_2P1A:
                case SC8989x_APPLE_2P4A:
                   chrg_type = APPLE_2_1A_CHARGER;
                   break;
                default:
                   chrg_type = APPLE_1_0A_CHARGER;
                   break;
               }
            }
            pr_err("SC8989x charger type: NON_STANDARD_ADAPTER, curr_in_limit = %d \n", curr_in_limit);
            break;

        case SC8989x_UNKNOWN:
            sc8989x_power_supply_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
            sc->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
            chrg_type = NONSTANDARD_CHARGER;
            pr_err("SC8989x charger type: UNKNOWN\n");
            curr_in_limit = 500;
            /*SC8989 apple 10w bc1.2 entry*/
            if (!sc->first_force) {
                ret = sc8989x_read_byte(sc, SC8989X_REG_91, &reg_val);
                if (ret)
                    sc8989x_set_key1(sc);
                sc8989x_write_byte(sc, SC8989X_REG_91, 0xA8);
                msleep(60);//Waiting time cannot be modified;
                sc8989x_force_dpdm(sc);
                msleep(200);//BC1.2 recognition duration
                sc8989x_write_byte(sc, SC8989X_REG_91, 0x28);
                sc8989x_set_key1(sc);
                sc->first_force = true;
                pr_err("SC8989x charger type: UNKNOWN TWICE\n");
                goto err;
            }
            if (type_dect_again) {
                type_dect_again = 0;
                schedule_delayed_work(&sc->apsd_work, 1000);
                pr_err("[%s]: chrg_type = unknown, try again \n", __func__);
                goto err;
            }
            break;

        default:
            pr_err("SC8989x charger type: default\n");
            //curr_in_limit = 500000;
            //break;
            __pm_relax(sc->charger_wakelock);
            return;
    }

    /* release dpdm bus*/
    if (sc->state.chrg_type != SC8989x_USB_HVDCP) {
        Charger_Detect_Release();
    }

    ret = power_supply_get_property(chrgdet_psy,
                        POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
    if(ret < 0) {
        pr_info("[%s]: get charge type failed, ret = %d\n", __func__, ret);
        __pm_relax(sc->charger_wakelock);
        return;
    }

    
    //set charge parameters
    dev_err(sc->dev, "Update: curr_in_limit = %d\n", curr_in_limit);
    sc8989x_set_input_current_limit(sc, curr_in_limit);

    //enable charge
    sc8989x_enable_charger(sc);
    sc8989x_dump_register(sc->chg_dev);
    //set Vsys_min
    dev_err(sc->dev, "Update: vsys_min \n");
    sc8989x_set_vsys_min(sc, 3500);
    
err:
    /* Report the charger type */
    propval.intval = chrg_type;
    ret = power_supply_set_property(chrgdet_psy,
                POWER_SUPPLY_PROP_CHARGE_TYPE, &propval);
    if (ret < 0) {
        pr_err("[%s]: set charge type failed, ret = %d\n", __func__, ret);
            __pm_relax(sc->charger_wakelock);
        return;
    }
    pr_err("%s --Notify charged--\n",__func__);
    power_supply_changed(sc->charger);  
    return;
}

static int sc8989x_charging_switch(struct charger_device *chg_dev,bool enable)
{
    int ret;
    struct sc8989x *sc = charger_get_data(chg_dev);

    if (enable)
        ret = sc8989x_enable_charger(sc);
    else
        ret = sc8989x_disable_charger(sc);
    return ret;
}


static int sc8989x_property_is_writeable(struct power_supply *psy,
                     enum power_supply_property prop)
{
    switch (prop) {
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
    case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
        return true;
    default:
        return false;
    }
}
static int sc8989x_charger_set_property(struct power_supply *psy,
        enum power_supply_property prop,
        const union power_supply_propval *val)
{
    struct sc8989x *sc = power_supply_get_drvdata(psy);
    int ret = -EINVAL;

    switch (prop) {
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        ret = sc8989x_set_input_current_limit(sc, val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
        sc8989x_charging_switch(s_chg_dev_otg,val->intval);
        break;
   /*case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        ret = sgm4154x_set_input_volt_lim(s_chg_dev_otg, val->intval);
        break;*/
    default:
        return -EINVAL;
    }

    return ret;
}

static int sc8989x_charger_get_property(struct power_supply *psy,
                enum power_supply_property psp,
                union power_supply_propval *val)
{
    struct sc8989x *sc = power_supply_get_drvdata(psy);
    struct sc8989x_state state;
    int ret = 0;

    mutex_lock(&sc->lock);
    //ret = sgm4154x_get_state(sgm, &state);
    state = sc->state;
    mutex_unlock(&sc->lock);
    if (ret)
        return ret;

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        if (!state.chrg_type || (state.chrg_type == SC8989x_OTG_MODE))
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (!state.chrg_stat)
            val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if (state.chrg_stat == SC8989X_CHRG_STAT_CHGDONE)
            val->intval = POWER_SUPPLY_STATUS_FULL;
        else
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
        break;
    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        switch (state.chrg_stat) {      
        case SC8989X_CHRG_STAT_PRECHG:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
            break;
        case SC8989X_CHRG_STAT_FASTCHG:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
            break;      
        case SC8989X_CHRG_STAT_CHGDONE:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
            break;
        case SC8989X_CHRG_STAT_IDLE:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
            break;
        default:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
        }
        break;

    case POWER_SUPPLY_PROP_USB_TYPE:
        val->intval = sc->psy_usb_type;
        break;

    case POWER_SUPPLY_PROP_MANUFACTURER:
        val->strval = SC8989X_MANUFACTURER;
        break;

    case POWER_SUPPLY_PROP_MODEL_NAME:
        val->strval = SC8989X_NAME;
        break;

    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = state.online;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = state.vbus_gd;
        break;
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = sc8989x_power_supply_desc.type;
        break;  

    case POWER_SUPPLY_PROP_HEALTH:
        if (state.chrg_fault & 0xF8)
            val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
        else
            val->intval = POWER_SUPPLY_HEALTH_GOOD;

        switch (state.health) {
        case SC8989X_FAULT_NTC_HOT:
            val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
            break;
        case SC8989X_FAULT_NTC_WARM:
            val->intval = POWER_SUPPLY_HEALTH_GOOD;
            break;
        case SC8989X_FAULT_NTC_COOL:
            val->intval = POWER_SUPPLY_HEALTH_GOOD;
            break;
        case SC8989X_FAULT_NTC_COLD:
            val->intval = POWER_SUPPLY_HEALTH_COLD;
            break;
        }
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = battery_get_vbus();
        break;

    case POWER_SUPPLY_PROP_CURRENT_NOW:
        //val->intval = state.ibus_adc;
        break;

/*  case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        ret = sgm4154x_get_input_volt_lim(sgm);
        if (ret < 0)
            return ret;

        val->intval = ret;
        break;*/

    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        break;

    case POWER_SUPPLY_PROP_CHARGING_ENABLED:
        val->intval = state.chrg_en;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = battery_get_vbus() * 1000; /* uV */
        break;

    case POWER_SUPPLY_PROP_CURRENT_MAX:
        switch (sc->psy_usb_type) {
            case POWER_SUPPLY_USB_TYPE_SDP: /* 500mA */
                val->intval = 500000;
                break;
            case POWER_SUPPLY_USB_TYPE_CDP: /* 1500mA */
            case POWER_SUPPLY_USB_TYPE_ACA:
                val->intval = 1500000;
                break;
            case POWER_SUPPLY_USB_TYPE_DCP: /* 2000mA */
                val->intval = 2000000;
                break;
            default :
                break;
        }
        break;

    default:
        return -EINVAL;
    }

    return ret;
}

static char *sc8989x_charger_supplied_to[] = {
     "battery",
};

static struct power_supply_desc sc8989x_power_supply_desc = {
    .name = "sgm4154x-charger",
    .type = POWER_SUPPLY_TYPE_USB,
    .usb_types = sc8989x_usb_type,
    .num_usb_types = ARRAY_SIZE(sc8989x_usb_type),
    .properties = sc8989x_power_supply_props,
    .num_properties = ARRAY_SIZE(sc8989x_power_supply_props),
    .get_property = sc8989x_charger_get_property,
    .set_property = sc8989x_charger_set_property,
    .property_is_writeable = sc8989x_property_is_writeable,
};

static int sc8989x_power_supply_init(struct sc8989x *sc,
                            struct device *dev)
{
    struct power_supply_config psy_cfg = { .drv_data = sc,
                        .of_node = dev->of_node, };

    psy_cfg.supplied_to = sc8989x_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(sc8989x_charger_supplied_to);

    sc->charger = devm_power_supply_register(sc->dev,
                         &sc8989x_power_supply_desc,
                         &psy_cfg);
    if (IS_ERR(sc->charger))
        return -EINVAL;
    
    return 0;
}

static struct regulator_ops sc8989x_vbus_ops = {
    .enable = sc8989x_enable_otg,
    .disable = sc8989x_disable_otg,
    .is_enabled = sc8989x_is_enabled_vbus,
//  .list_voltage = regulator_list_voltage_linear,
//  .set_voltage_sel = sgm4154x_set_boost_voltage_limit,
//  .get_voltage_sel = sgm4154x_boost_get_voltage_sel,
//  .set_current_limit = sgm4154x_set_boost_current_limit,
//  .get_current_limit = sgm4154x_boost_get_current_limit,*/

};

static const struct regulator_desc sc8989x_otg_rdesc = {
    .of_match = "sc8989x,otg-vbus",
    .name = "usb-otg-vbus",
    .ops = &sc8989x_vbus_ops,
    .owner = THIS_MODULE,
    .type = REGULATOR_VOLTAGE,
    .fixed_uV = 5150000,
    .n_voltages = 1,
//  .min_uV = 4850000,
//  .uV_step = 150000, /* 150mV per step */
//  .n_voltages = 4, /* 4850mV to 5300mV */
//  .vsel_reg = SGM4154x_CHRG_CTRL_6,
//  .vsel_mask = SGM4154x_BOOSTV,
//  .enable_reg = SGM4154x_CHRG_CTRL_1,
//  .enable_mask = SGM4154x_OTG_EN,
//  .csel_reg = SGM4154x_CHRG_CTRL_2,
//  .csel_mask = SGM4154x_BOOST_LIM,
};

static int sc8989x_vbus_regulator_register(struct sc8989x *sc)
{
    struct regulator_config config = {};
    int ret = 0;
    /* otg regulator */
    config.dev = sc->dev;
    config.driver_data = sc;
    sc->otg_rdev = devm_regulator_register(sc->dev,
                        &sc8989x_otg_rdesc, &config);
    if (IS_ERR(sc->otg_rdev)) {
        ret = PTR_ERR(sc->otg_rdev);
        pr_info("%s: register otg regulator failed (%d)\n", __func__, ret);
    }
    sc->otg_rdev->constraints->valid_ops_mask |= REGULATOR_CHANGE_STATUS;
    return ret;
}

static int sc8989x_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
    if (chg_dev == NULL) {
       return -EINVAL;
    }
        pr_info ("%s: event = %d\n", __func__, event);
#ifdef CHARGER_EVENT_USE
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
#endif
    return 0;
}

static int sc8989x_enable_termination(struct charger_device *chg_dev,bool en)
{
    int ret;
    u8 val;
    struct sc8989x *sc = dev_get_drvdata(&chg_dev->dev);
    if (en) {
        val = SC8989X_TERM_ENABLE << SC8989X_EN_TERM_SHIFT;
    } else {
        val = SC8989X_TERM_DISABLE << SC8989X_EN_TERM_SHIFT;
    }
    ret = sc8989x_update_bits(sc, SC8989X_REG_07,
                    SC8989X_EN_TERM_MASK, val);
    return ret;
}

static struct charger_ops sc8989x_chg_ops = {
    /* Normal charging */
    .plug_in = sc8989x_plug_in,
    .plug_out = sc8989x_plug_out,
    .dump_registers = sc8989x_dump_register,
    .enable = sc8989x_charging,
    .is_enabled = sc8989x_is_charging_enable,
    .get_charging_current = sc8989x_get_ichg,
    .set_charging_current = sc8989x_set_ichg,
    .get_input_current = sc8989x_get_icl,
    .set_input_current = sc8989x_set_icl,
    .get_constant_voltage = sc8989x_get_vchg,
    .set_constant_voltage = sc8989x_set_vchg,
    .kick_wdt = sc8989x_kick_wdt,
    .set_mivr = sc8989x_set_ivl,
    .is_charging_done = sc8989x_is_charging_done,
    .get_min_charging_current = sc8989x_get_min_ichg,
    .enable_termination = sc8989x_enable_termination,
    .get_charger_type_hvdcp = sc8989x_get_hvdcp_type,
    .set_eoc_current = sc8989x_set_ieoc,
    /* Safety timer */
    .enable_safety_timer = sc8989x_set_safety_timer,
    .is_safety_timer_enabled = sc8989x_is_safety_timer_enabled,

    /* Power path */
    .enable_powerpath = sc8989x_enable_powerpath,
    .enable_hz = sc8989x_set_hiz,
    .is_powerpath_enabled = NULL,

    /* OTG */
    .enable_otg = sc8989x_set_otg,
    .set_boost_current_limit = sc8989x_set_boost_ilmt,
    .event = sc8989x_do_event,
    .enable_discharge = NULL,

    /* PE+/PE+20 */
    .send_ta_current_pattern = NULL,
    .set_pe20_efficiency_table = NULL,
    .send_ta20_current_pattern = NULL,
    .enable_cable_drop_comp = NULL,

    /* ADC */
    .get_tchg_adc = NULL,
};

static struct of_device_id sc8989x_charger_match_table[] = {
    {
     .compatible = "sc,sc89890h",
     .data = &pn_data[PN_SC89890H],
     },
    {},
};
MODULE_DEVICE_TABLE(of, sc8989x_charger_match_table);


static int sc8989x_charger_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    struct sc8989x *sc;
    const struct of_device_id *match;
    struct device *dev = &client->dev;
    struct device_node *node = client->dev.of_node;
    char *name = NULL;
    int ret = 0;

    sc = devm_kzalloc(&client->dev, sizeof(struct sc8989x), GFP_KERNEL);
    if (!sc)
        return -ENOMEM;

    sc->dev = &client->dev;
    sc->client = client;

    i2c_set_clientdata(client, sc);

    mutex_init(&sc->lock);
    mutex_init(&sc->i2c_rw_lock);

    ret = sc8989x_detect_device(sc);
    if (ret) {
        pr_err("No sc8989x device found!\n");
        return -ENODEV;
    }

    match = of_match_node(sc8989x_charger_match_table, node);
    if (match == NULL) {
        pr_err("device tree match not found\n");
        return -EINVAL;
    }

    if (sc->part_no != *(int *)match->data)
        pr_info("part no mismatch, hw:%s, devicetree:%d\n",
            pn_str[sc->part_no],*(int *) match->data);

    sc->platform_data = sc8989x_parse_dt(node, sc);

    if (!sc->platform_data) {
        pr_err("No platform data provided.\n");
        return -EINVAL;
    }

    ret = sc8989x_init_device(sc);
    if (ret) {
        pr_err("Failed to init device\n");
        return ret;
    }

    
    sc8989x_register_interrupt(sc);

    sc->chg_dev = charger_device_register(sc->chg_dev_name,
                          &client->dev, sc,
                          &sc8989x_chg_ops,
                          &sc8989x_chg_props);
    if (IS_ERR_OR_NULL(sc->chg_dev)) {
        ret = PTR_ERR(sc->chg_dev);
        return ret;
    }

    INIT_DELAYED_WORK(&sc->charge_detect_delayed_work, charger_detect_work_func);
    INIT_DELAYED_WORK(&sc->charge_monitor_work, charger_monitor_work_func);
    INIT_DELAYED_WORK(&sc->hvdcp_work, chg_type_hvdcp30_work);
    INIT_DELAYED_WORK(&sc->apsd_work, chg_type_apsd_work);
    INIT_DELAYED_WORK(&sc->time_test_work, time_test_work_func);

    set_iindpm_vindpm_intput_disable(sc);
    sc8989x_enable_ico(sc, false);
        /* otg regulator */
    s_chg_dev_otg=sc->chg_dev;

    ret = sc8989x_power_supply_init(sc, dev);
    if (ret) {
        pr_err("Failed to register power supply\n");
        return ret;
    }

    sc->chg_consumer = charger_manager_get_by_name(sc->dev, "primary_chg");
    if(!sc->chg_consumer) {
         pr_err("%s: get chg_consumer failed\n", __func__);
    }

    ret = sc8989x_vbus_regulator_register(sc);
    if (ret < 0) {
        dev_err(dev, "failed to init regulator\n");
        return ret;
    }
    name = devm_kasprintf(sc->dev, GFP_KERNEL, "%s","sc8989x suspend wakelock");
    sc->charger_wakelock = wakeup_source_register(sc->dev, name);

    schedule_delayed_work(&sc->apsd_work,100);
    schedule_delayed_work(&sc->charge_monitor_work,100);

    pr_err("sc8989x probe successfully, Part Num:%d, Revision:%d\n!",
           sc->part_no, sc->revision);

    return 0;
}

static int sc8989x_charger_remove(struct i2c_client *client)
{
    struct sc8989x *sc = i2c_get_clientdata(client);

    cancel_delayed_work_sync(&sc->charge_monitor_work);

    regulator_unregister(sc->otg_rdev);

    power_supply_unregister(sc->charger);

    mutex_destroy(&sc->lock);
    mutex_destroy(&sc->i2c_rw_lock);

    return 0;
}

static void sc8989x_charger_shutdown(struct i2c_client *client)
{
    int ret = 0;
    u8 val;
    struct sc8989x *sc = i2c_get_clientdata(client);

    pr_info("sc89890h_charger_shutdown\n");

    val = SC8989X_RESET << SC8989X_RESET_SHIFT;
    ret = sc8989x_update_bits(sc, SC8989X_REG_14,
                    SC8989X_RESET_MASK, val);
    if (ret) {
        pr_err("[%s] sc8989x register reset fail\n", __func__);
    }

    ret = sc8989x_disable_charger(sc);
    if (ret) {
        pr_err("Failed to disable charger, ret = %d\n", ret);
    }
}

static struct i2c_driver sc8989x_charger_driver = {
    .driver = {
           .name = "sc8989x-charger",
           .owner = THIS_MODULE,
           .of_match_table = sc8989x_charger_match_table,
           },

    .probe = sc8989x_charger_probe,
    .remove = sc8989x_charger_remove,
    .shutdown = sc8989x_charger_shutdown,
};

module_i2c_driver(sc8989x_charger_driver);

MODULE_DESCRIPTION("SC SC8989X Charger Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("South Chip");

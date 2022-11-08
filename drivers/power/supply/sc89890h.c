#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "sc89890h.h"
#include "charger_class.h"
#if defined (CONFIG_TINNO_SCC_SUPPORT)
#include <linux/scc_drv.h>
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

#undef dev_fmt
#define dev_fmt(fmt) "202203251630: " fmt

extern int current_max_sgm7220;
extern int current_max_wusb3801;

int sc89890h_vtemp = 250;
EXPORT_SYMBOL(sc89890h_vtemp);

extern int demomode_enable;
extern int demomode_max_soc;
extern int demomode_min_soc;

static char *sc89890h_charge_state[] = {
    "charge-disable",
    "pre-charge",
    "fast-charge",
    "charge-terminated",
};

static enum power_supply_usb_type sc89890h_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
};

static enum power_supply_usb_type sc89890h_charge_type[] = {
    POWER_SUPPLY_CHARGE_TYPE_NONE,
    POWER_SUPPLY_CHARGE_TYPE_TRICKLE,
    POWER_SUPPLY_CHARGE_TYPE_FAST,
    POWER_SUPPLY_CHARGE_TYPE_TAPER,
};

static enum power_supply_property sc89890h_charger_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_USB_TYPE,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static enum power_supply_property sc89890h_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static enum power_supply_property sc89890h_usb_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CHARGE_ENABLED,
};

static int sc89890h_read_reg(
    struct sc89890h_device *sc_device, u8 reg, u8 *data)
{
    int ret;

    ret = i2c_smbus_read_byte_data(sc_device->client, reg);
    if (ret < 0) {
        dev_err(sc_device->dev, "read reg: %#x failed", reg);
        return -EINVAL;
    } else
        *data = ret;

    return 0;
}

static int sc89890h_write_reg(
    struct sc89890h_device *sc_device, u8 reg, u8 value)
{
    int ret;

    ret = i2c_smbus_write_byte_data(sc_device->client, reg, value);
    if (ret < 0) {
        dev_err(sc_device->dev, "write reg: %#x failed", reg);
        return -EINVAL;
    }

    return 0;
}

static int sc89890h_update_bits(
    struct sc89890h_device *sc_device, u8 reg, u8 mask, u8 val)
{
    u8  tmp;
    int ret;

    ret = sc89890h_read_reg(sc_device, reg, &tmp);
    if (ret)
        return ret;

    tmp &= ~mask;
    tmp |= val & mask;

    ret = sc89890h_write_reg(sc_device, reg, tmp);
    if (ret)
        return ret;

    return 0;
}

static int sc89890h_disable_ilimit_pin(
    struct sc89890h_device *sc_device)
{
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_0,
        SC89890H_ILI_PIN_EN_MASK, SC89890H_ILI_PIN_DISABLE);
}

static int sc89890h_disable_hvdcp(
    struct sc89890h_device *sc_device)
{
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_2,
        SC89890H_HVDCP_EN_MASK, SC89890H_HVDCP_DISABLE);
}

static int sc89890h_disable_watchdog_timer(
    struct sc89890h_device *sc_device)
{
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_7,
        SC89890H_WDT_TIMER_MASK, SC89890H_WDT_TIMER_DISABLE);
}

static void sc89890h_charge_enable_ctrl(
    struct sc89890h_device *sc_device, int en)
{   
    sc_device->chg_en = !!en;
    sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_3,
        SC89890H_OTG_CHG_MASK, en ? SC89890H_CHRG_EN : 0);
}

static void sc89890h_set_usbin_current_limit(
    struct sc89890h_device *sc_device , unsigned int iindpm)
{
    u8 reg_val;

    reg_val = (iindpm - SC89890H_IINDPM_I_MIN_MA)
              / SC89890H_IINDPM_STEP_MA;
    sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_0,
        SC89890H_IINDPM_I_MASK, reg_val);
}

static int sc89890h_get_cc_ibat_value(
    struct sc89890h_device *sc_device)
{
    int ret;
    int ichg;
    u8 reg_val;

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_4, &reg_val);
    if (ret < 0)
        ichg = -EINVAL;
    else
        ichg = reg_val & SC89890H_ICHRG_CUR_MASK;

    return ichg;
}

static int sc89890h_set_fast_charge_current(
    struct sc89890h_device *sc_device, unsigned int chrg_curr)
{
    u8 reg_val;

    reg_val = chrg_curr / SC89890H_ICHRG_CURRENT_STEP_MA;
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_4,
        SC89890H_ICHRG_CUR_MASK, reg_val);
}

static int sc89890h_set_taper_current(
    struct sc89890h_device *sc_device, int term_current)
{
    u8 reg_val;

    reg_val = (term_current - SC89890H_TERMCHRG_I_MIN_MA)
                / SC89890H_TERMCHRG_CURRENT_STEP_MA;
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_5,
        SC89890H_TERMCHRG_CUR_MASK, reg_val);
}

static int sc89890h_set_prechg_current(
    struct sc89890h_device *sc_device, int prechg_current)
{
    u8 reg_val;

    reg_val = (prechg_current - SC89890H_PRECHG_I_MIN_MA)
                / SC89890H_PRECHG_CURRENT_STEP_MA;
    reg_val = reg_val << 4;
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_5,
        SC89890H_PRECHG_CUR_MASK, reg_val);
}

static int sc89890h_set_battery_voltage(
    struct sc89890h_device *sc_device, unsigned int chrg_volt)
{
    u8 reg_val;

    reg_val = (chrg_volt - SC89890H_VBAT_VREG_MIN_MV)
                / SC89890H_VBAT_VREG_STEP_MV;
    reg_val = reg_val << 2;
    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_6,
        SC89890H_VBAT_VREG_MASK, reg_val);
}

static int sc89890h_recharge_voltage(
    struct sc89890h_device *sc_device, unsigned int rechg_mv)
{
    u8 reg_val;

    if (rechg_mv == 100)
        reg_val = SC89890H_RECHARGE_VOLTAGE_100MV;
    else
        reg_val = SC89890H_RECHARGE_VOLTAGE_200MV;

    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_6,
        SC89890H_RECHARGE_VOLTAGE_MASK, reg_val);
}

static int sc89890h_set_boost_current(
    struct sc89890h_device *sc_device, int boost_current)
{
    u8 reg_val;

    switch (boost_current) {
    case 500:
        reg_val = SC89890H_BOOST_CUR_500MA;
        break;
    case 750:
        reg_val = SC89890H_BOOST_CUR_750MA;
        break;
    case 1200:
        reg_val = SC89890H_BOOST_CUR_1200MA;
        break;
    case 1400:
        reg_val = SC89890H_BOOST_CUR_1400MA;
        break;
    case 1650:
        reg_val = SC89890H_BOOST_CUR_1650MA;
        break;
    case 1875:
        reg_val = SC89890H_BOOST_CUR_1875MA;
        break;
    case 2150:
        reg_val = SC89890H_BOOST_CUR_2150MA;
        break;
    case 2450:
        reg_val = SC89890H_BOOST_CUR_2450MA;
        break;
    default:
        reg_val = SC89890H_BOOST_CUR_1400MA;
        break;
    }

    return sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_A,
        SC89890H_BOOST_CUR_MASK, reg_val);
}

static int sc89890h_chipid_detect(struct sc89890h_device *sc_device)
{
    int ret = 0;
    u8 val = 0;

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_14, &val);
    if (ret < 0) {
        dev_err(sc_device->dev, "read SC89890H_CHRG_CTRL_14 failed");
        return -EINVAL;
    } else {
        val = val & SC89890H_PN_MASK;
        return val;
    }
}

static int sc89890h_hardware_init(struct sc89890h_device *sc_device)
{
    int ret;
	
	sc89890h_charge_enable_ctrl(sc_device, 1);

    ret = sc89890h_set_taper_current(sc_device, 300);
    if (ret) {
        dev_err(sc_device->dev, "set sc89890h taper current failed");
        return -EPERM;
    }

    ret = sc89890h_set_prechg_current(sc_device, 360);
   if (ret) {
       dev_err(sc_device->dev, "set sc89890h precharge current failed");
       return -EPERM;
   }

    ret = sc89890h_set_battery_voltage(sc_device, 4448);
    if (ret) {
        dev_err(sc_device->dev, "set sc89890h charge voltage failed");
        return -EPERM;
    }

    ret = sc89890h_recharge_voltage(sc_device, 200);
    if (ret) {
        dev_err(sc_device->dev, "set sc89890h recharge voltage failed");
        return -EPERM;
    }

    ret = sc89890h_set_fast_charge_current(sc_device, 3000);
    if (ret) {
        dev_err(sc_device->dev, "set sc89890h fast charge current failed");
        return -EPERM;
    }

    ret = sc89890h_set_boost_current(sc_device, 1200);
    if (ret) {
            dev_err(sc_device->dev, "set sc89890h boost current failed");
            return -EPERM;
    }

    ret = sc89890h_disable_watchdog_timer(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "disable sc89890h watchdog timer failed");
        return -EPERM;
    }

    ret = sc89890h_disable_ilimit_pin(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "disable sc89890h ilimit pin failed");
        return -EPERM;
    }

    ret = sc89890h_disable_hvdcp(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "disable sc89890h hvdcp failed");
        return -EPERM;
    }

    return 0;
}

static int sc89890h_get_charge_state(
    struct sc89890h_device *sc_device)
{
    int ret;
    u8 status;

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_B, &status);
    if (ret) {
        dev_err(sc_device->dev, "get sc89890h status register failed");
        return -EINVAL;
    }

    return status;
}

static int sc89890h_get_status(
    struct sc89890h_device *sc_device, struct sc89890h_state *state)
{
    int ret;
    u8 status, fault, vbus_good, dpm_stat;

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_B, &status);
    if (ret) {
        dev_err(sc_device->dev, "read sc89890h 0x0b register failed");
        return -EINVAL;
    }
    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_C, &fault);
    if (ret) {
        dev_err(sc_device->dev, "read sc89890h 0x0c register failed");
        return -EINVAL;
    }
    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_11, &vbus_good);
    if (ret) {
        dev_err(sc_device->dev, "read sc89890h 0x11 register failed");
        return -EINVAL;
    }
    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_13, &dpm_stat);
    if (ret) {
        dev_err(sc_device->dev, "read sc89890h 0x13 register failed");
        return -EINVAL;
    }

    state->vbus_gd = (vbus_good & SC89890H_VBUS_GOOD_MASK) >> 7;
    state->chrg_stat = (status & SC89890H_CHG_STAT_MASK) >> 3;
    state->vbus_stat = (status & SC89890H_VBUS_STAT_MASK) >> 5;

    dev_info(sc_device->dev, "[0b]: %#x, [0c]: %#x, [11]: %#x, [13]: %#x",
        status, fault, vbus_good, dpm_stat);

    return 0;
}

static int sc89890h_get_initial_state(
    struct sc89890h_device *sc_device)
{
    int ret;

    ret = sc89890h_get_status(sc_device, &sc_device->state);
    if (ret < 0) {
        dev_info(sc_device->dev, "get charge status failed\n");
        return -EINVAL;
    }

    if (sc_device->state.vbus_gd) {
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1000000;
        sc_device->state.vbus_stat = 0;
        sc_device->state.chrg_stat = SC89890H_FAST_CHARGE;
        // release dp dm
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_2,
            SC89890H_DPM_FORCE_DET, SC89890H_DPM_FORCE_DET);
        schedule_delayed_work(&sc_device->charge_work, msecs_to_jiffies(30000));
        dev_info(sc_device->dev, "charger online during power up\n");
    }

    return 0;
}

static int sc89890h_get_battery_temperature(
    struct sc89890h_device *sc_device)
{
    int ret;
    int charge_state;
    union power_supply_propval psy_prop;

    ret = power_supply_get_property(sc_device->battery_psy,
            POWER_SUPPLY_PROP_TEMP, &psy_prop);
    if (ret) {
        dev_err(sc_device->dev, "get battery temp failed");
        return -ENODEV;
    }

    // set virtual temperature
    if (sc_device->vtemp != SC89890H_TEMP_INVALID) {
        psy_prop.intval = sc_device->vtemp;
        dev_info(sc_device->dev, "set vtemp: %d", sc_device->vtemp);
    }

    sc_device->temp = psy_prop.intval;
    dev_info(sc_device->dev, "battery temp: %d", sc_device->temp);

    if (sc_device->temp <= -170 || sc_device->temp > 570)
        charge_state = SC89890H_CHG_DISABLE;
    else if (sc_device->temp > -170 && sc_device->temp < 0)
        charge_state = SC89890H_STATE_COLD;
    else if (sc_device->temp >= 0 && sc_device->temp < 150)
        charge_state = SC89890H_STATE_COOL;
    else if (sc_device->temp > 150 && sc_device->temp <= 450)
        charge_state = SC89890H_STATE_NORMAL;
    else if (sc_device->temp > 450 && sc_device->temp <= 570)
        charge_state = SC89890H_STATE_WARM;

    return charge_state;
}

static void sc89890h_set_charge_voltage_current(
    struct sc89890h_device *sc_device)
{
    switch (sc_device->temp_state) {
    case SC89890H_STATE_COLD:
        dev_info(sc_device->dev, "cold state");
        sc_device->ibat = 960;
        sc89890h_set_battery_voltage(sc_device, 4192);
        sc89890h_set_fast_charge_current(sc_device, 960);
        break;
    case SC89890H_STATE_COOL:
        dev_info(sc_device->dev, "cool state");
        sc89890h_set_battery_voltage(sc_device, 4448);
        break;
    case SC89890H_STATE_NORMAL:
        dev_info(sc_device->dev, "normal state");
        sc_device->ibat = 3000;
        sc89890h_set_battery_voltage(sc_device, 4448);
        sc89890h_set_fast_charge_current(sc_device, 3000);
        break;
    case SC89890H_STATE_WARM:
        dev_info(sc_device->dev, "warm state");
        sc_device->ibat = 1980;
        sc89890h_set_battery_voltage(sc_device, 4192);
        sc89890h_set_fast_charge_current(sc_device, 1980);
        break;
    case SC89890H_CHG_DISABLE:
        dev_info(sc_device->dev, "charge disabled state");
        break;
    default:
        dev_err(sc_device->dev, "invalid temp state: %d", sc_device->temp_state);
        break;
    }
}

static inline int sc89890h_set_cvcharge_5v(
    struct sc89890h_device *sc_device)
{
    int ret;
    union power_supply_propval psy_prop;

    ret = power_supply_get_property(sc_device->battery_psy,
            POWER_SUPPLY_PROP_CURRENT_NOW, &psy_prop);
    if (ret) {
        dev_err(sc_device->dev, "get battery charge current failed");
        return -ENODEV;
    }

    // dpdm should only be controlled in dcp
    // dpdm should be HIZ state in sdp/cdp
    if (sc_device->state.vbus_stat == SC89890H_USB_DCP) {
        if (psy_prop.intval > 0 && psy_prop.intval < 1500000) {
            // enhable 5V
            dev_info(sc_device->dev, "enable dp dm for dcp 5v");
            sc_device->qc_type = QUICK_CHARGE_1P0;
            sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
                SC89890H_DP_VSEL_MASK | SC89890H_DM_VSEL_MASK,
                SC89890H_DP_0V6 | SC89890H_DM_HIZ);
        }
    }

    return 0;
}

static inline void sc89890h_get_thermal_temp(
    struct sc89890h_device *sc_device)
{
    int temp;

    sc_device->tz_ap = thermal_zone_get_zone_by_name("mtktsAP");
    if (!IS_ERR(sc_device->tz_ap) && sc_device->tz_ap->ops &&
             sc_device->tz_ap->ops->get_temp) {
        sc_device->tz_ap->ops->get_temp(sc_device->tz_ap, &temp);
        dev_info(sc_device->dev, "mtktsAP temp: %d", temp / 1000);
    }

    sc_device->tz_rf = thermal_zone_get_zone_by_name("mtktsbtsmdpa");
    if (!IS_ERR(sc_device->tz_rf) && sc_device->tz_rf->ops &&
             sc_device->tz_rf->ops->get_temp) {
        sc_device->tz_rf->ops->get_temp(sc_device->tz_rf, &temp);
        dev_info(sc_device->dev, "mtktsbtsmdpa temp: %d", temp / 1000);
    }

    sc_device->tz_chg = thermal_zone_get_zone_by_name("mtktscharger");
    if (!IS_ERR(sc_device->tz_chg) && sc_device->tz_chg->ops &&
             sc_device->tz_chg->ops->get_temp) {
        sc_device->tz_chg->ops->get_temp(sc_device->tz_chg, &temp);
        dev_info(sc_device->dev, "mtktscharger temp: %d", temp / 1000);
    }

    sc_device->tz_batt = thermal_zone_get_zone_by_name("mtktsbattery");
    if (!IS_ERR(sc_device->tz_batt) && sc_device->tz_batt->ops &&
        sc_device->tz_batt->ops->get_temp) {
        sc_device->tz_batt->ops->get_temp(sc_device->tz_batt, &temp);
        dev_info(sc_device->dev, "mtktsbattery temp: %d", temp / 1000);
    }
}

static void sc89890h_charge_online_work(
    struct work_struct *work)
{
    int ret, err = 0;
    int new_temp;
    int chg_state;
#if defined (CONFIG_TINNO_SCC_SUPPORT)
    int ichg_o, ichg_n, scc_value;
    union power_supply_propval psy_prop;
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    struct sc89890h_device *sc_device = container_of(
        work, struct sc89890h_device, charge_work.work);

    if (!sc_device->battery_psy) {
        sc_device->battery_psy = power_supply_get_by_name("battery");
        if (!sc_device->battery_psy) {
            err = -ENODEV;
            dev_err(sc_device->dev, "get battery power supply failed");
            goto out;
        }
    }

    sc89890h_get_thermal_temp(sc_device);

    /*if (sc_device->qc_type != QUICK_CHARGE_1P0) {
        ret = sc89890h_set_cvcharge_5v(sc_device);
        if (ret) {
            err = -EPERM;
            dev_err(sc_device->dev, "set constant charge voltage to 5v failed");
            goto out;
        }
    }*/

    new_temp = sc89890h_get_battery_temperature(sc_device);
    if (new_temp < 0) {
        err = -ENODEV;
        dev_err(sc_device->dev, "get charge state failed");
        goto out;
    }
    if (sc_device->temp_state != new_temp) {
        sc_device->temp_state = new_temp;
        dev_info(sc_device->dev, "temp status changed");
        if (sc_device->chg_en && sc_device->temp_state == SC89890H_CHG_DISABLE) {
            //disable_irq(sc_device->client->irq);
            sc89890h_charge_enable_ctrl(sc_device, 0);
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev, "disable charge, temperature: %d", sc_device->temp);
        } else if (!sc_device->chg_en) {
            sc89890h_charge_enable_ctrl(sc_device, 1);
            //enable_irq(sc_device->client->irq);
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev, "enable charge, temperature: %d", sc_device->temp);
        }
        sc89890h_set_charge_voltage_current(sc_device);
    }
    if (sc_device->temp_state == SC89890H_STATE_COOL) {
        ret = power_supply_get_property(sc_device->battery_psy,
                POWER_SUPPLY_PROP_VOLTAGE_NOW, &psy_prop);
        if (ret) {
            dev_err(sc_device->dev, "get battery voltage failed");
            return;
        }
        if (psy_prop.intval < 4200000) {
            sc_device->ibat = 1980;
            sc89890h_set_fast_charge_current(sc_device, sc_device->ibat);
            dev_info(sc_device->dev,
                "battery voltage: %d, set charge current to 1980",
                psy_prop.intval);
        } else {
            sc_device->ibat = 960;
            sc89890h_set_fast_charge_current(sc_device, sc_device->ibat);
            dev_info(sc_device->dev,
                "battery voltage: %d, set charge current to 960",
                psy_prop.intval);
        }
    }
    if (sc_device->temp <= -100 || sc_device->temp >= 550) {
        power_supply_changed(sc_device->ac_psy);
        power_supply_changed(sc_device->usb_psy);
        power_supply_changed(sc_device->charger_psy);
        dev_info(sc_device->dev, "device shutdown, temperature: %d", sc_device->temp);
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    if (sc_device->state.vbus_stat == SC89890H_USB_DCP) {
        scc_value = scc_get_current();
        scc_value /= 1000;
        if (scc_value > sc_device->ibat)
            scc_value = sc_device->ibat;
        dev_info(sc_device->dev, "scc current = %d, jeita current: %d",
            scc_value, sc_device->ibat);
        ichg_o = sc89890h_get_cc_ibat_value(sc_device);
        if (ichg_o < 0) {
            err = -EINVAL;
            dev_err(sc_device->dev, "get cc ibat value failed");
            goto out;
        }
        ichg_n = (scc_value / 60) & SC89890H_ICHRG_CUR_MASK;
        if (ichg_n != ichg_o) {
            sc89890h_set_fast_charge_current(sc_device, scc_value);
            dev_info(sc_device->dev, "scc set cc current = %d", scc_value);
        }
    }
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    if (demomode_enable) {
        ret = power_supply_get_property(sc_device->battery_psy,
                POWER_SUPPLY_PROP_CAPACITY, &psy_prop);
        if (ret) {
            err = -EINVAL;
            dev_err(sc_device->dev, "get battery capacity failed");
            goto out;
        }
        if (psy_prop.intval >= demomode_max_soc && sc_device->chg_en) {
            sc89890h_charge_enable_ctrl(sc_device, 0);
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev, "disable charge due to max_soc");
        } else if (psy_prop.intval <= demomode_min_soc && !sc_device->chg_en) {
            sc89890h_charge_enable_ctrl(sc_device, 1);
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev, "enable charge due to min_soc");
        }
    }

    ret = sc89890h_get_charge_state(sc_device);
    if (ret < 0) {
        err = -EINVAL;
        dev_err(sc_device->dev, "get charge state failed");
        goto out;
    }
    chg_state = (ret & SC89890H_CHG_STAT_MASK) >> 3;
    if (sc_device->state.chrg_stat != chg_state) {
        sc_device->state.chrg_stat = chg_state;
        dev_info(sc_device->dev, "charge state: %s",
            sc89890h_charge_state[chg_state]);
        power_supply_changed(sc_device->ac_psy);
        power_supply_changed(sc_device->usb_psy);
        power_supply_changed(sc_device->charger_psy);
    }

out:
    if (err || !sc_device->state.vbus_gd)
        cancel_delayed_work(&sc_device->charge_work);
    else
        schedule_delayed_work(&sc_device->charge_work, msecs_to_jiffies(1000));
}

static void sc89890h_request_qc3p0(struct sc89890h_device *sc_device)
{
    int i, ret, value, vbus_volt_old, vbus_volt_new, time_out = 0;

    ret = iio_read_channel_processed(sc_device->vbus, &value);
    if (ret < 0) {
        dev_err(sc_device->dev, "get vbus voltage failed before qc3.0 check");
        return;
    } else {
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        vbus_volt_old = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
    }

    // qc3.0 commubication
    dev_info(sc_device->dev, "hvdcp qc3.0 checking");
    // enter continuous mode
    sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
        SC89890H_DP_VSEL_MASK | SC89890H_DM_VSEL_MASK,
        SC89890H_DP_0V6 | SC89890H_DM_3V3);
    msleep(100);
    for (i = 0; i < 20; i++) {
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DM_VSEL_MASK, SC89890H_DM_0V6);
        usleep_range(100, 100);
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DM_VSEL_MASK, SC89890H_DM_3V3);
        // check vbus voltage until vbus stable
        while (time_out < 100) {
            msleep(10);
            ret = iio_read_channel_processed(sc_device->vbus, &value);
            if (ret < 0) {
                dev_err(sc_device->dev, "check qc3.0 vbus failed, try count: %d", i);
                return;
            }
            // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
            vbus_volt_new = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
            if (vbus_volt_new < (vbus_volt_old - 100)) {
                vbus_volt_old = vbus_volt_new;
                break;
            }
            time_out++;
        }
        if (time_out == 100) {
            dev_info(sc_device->dev, "qc3.0 not supported");
            return;
        }
        if (vbus_volt_new < sc_device->chg_volt) {
            sc_device->qc_type = QUICK_CHARGE_3P0;
            sc_device->voltage_max = 5600000;
            sc_device->current_max = 3000000;
            sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
                SC89890H_DP_VSEL_MASK, SC89890H_DP_3V3);
            usleep_range(100, 100);
            sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
                SC89890H_DP_VSEL_MASK, SC89890H_DP_0V6);
            sc89890h_set_usbin_current_limit(sc_device, sc_device->chg_curr);
            msleep(100);
            ret = iio_read_channel_processed(sc_device->vbus, &value);
            if (ret < 0) {
                dev_err(sc_device->dev, "get vbus voltage failed after increase vbus voltage");
                return;
            }
            vbus_volt_new = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev,
                "hvdcp qc3.0 detected, vbus voltage: %d", vbus_volt_new);
            break;
        }
    }
}

static void sc89890h_request_qc2p0(struct sc89890h_device *sc_device)
{
    int ret, value, vbus_volt, time_out = 0;

    // qc2.0 commubication
    dev_info(sc_device->dev, "hvdcp qc2.0 checking");
    sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
        SC89890H_DP_VSEL_MASK | SC89890H_DM_VSEL_MASK,
        SC89890H_DP_3V3 | SC89890H_DM_0V6);
    // set qc2.0 timeout to 3s, wait vbus stable
    while (time_out < 30) {
        time_out++;
        msleep(100);
        ret = iio_read_channel_processed(sc_device->vbus, &value);
        if (ret < 0) {
            dev_err(sc_device->dev, "get vbus voltage failed when check qc2.0");
            return;
        }
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        vbus_volt = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
        if (vbus_volt >= 8200 && vbus_volt <= 9200) {
            sc_device->qc_type = QUICK_CHARGE_2P0;
            sc_device->voltage_max = 9000000;
            sc_device->current_max = 2000000;
            //sc89890h_set_usbin_current_limit(sc_device, 2000);
            power_supply_changed(sc_device->ac_psy);
            power_supply_changed(sc_device->usb_psy);
            power_supply_changed(sc_device->charger_psy);
            dev_info(sc_device->dev,
                "hvdcp qc2.0 detected, vbus voltage: %d", vbus_volt);
            break;
        }
    }
}

static void sc89890h_hvdcp_detect_work(
    struct work_struct *work)
{
    struct sc89890h_state state;
    struct sc89890h_device *sc_device = container_of(
        work, struct sc89890h_device, hvdcp_work.work);

    dev_info(sc_device->dev, "hvdcp work started");
    if (sc_device->qc_force == QC_TYPE_FORCE_1P0) {
        dev_info(sc_device->dev, "force qc1.0");
        sc89890h_set_usbin_current_limit(sc_device, 3000);
    } else if (sc_device->qc_force == QC_TYPE_FORCE_2P0) {
        dev_info(sc_device->dev, "force qc2.0");
        sc89890h_request_qc2p0(sc_device);
    } else if (sc_device->qc_force == QC_TYPE_FORCE_3P0) {
        dev_info(sc_device->dev, "force qc3.0");
        sc89890h_request_qc3p0(sc_device);
    } else { // sc_device->qc_force == QC_TYPE_AUTO_CHECK
        dev_info(sc_device->dev, "auto check qc type");
        sc_device->qc_type = QUICK_CHARGE_1P0; // default mode qc1.0
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1000000;
        sc89890h_request_qc2p0(sc_device);
        if (sc_device->qc_type == QUICK_CHARGE_2P0)
            sc89890h_request_qc3p0(sc_device);
    }

    if (sc_device->qc_type == QUICK_CHARGE_1P0) {
        // qc1.0 set current to 1A
        sc_device->qc_type = QUICK_CHARGE_1P0;
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1000000;
        if (sc_device->state.vbus_stat != SC89890H_USB_UNKNOWN &&
            sc_device->state.vbus_stat != SC89890H_USB_NSTANDA)
            sc89890h_set_usbin_current_limit(sc_device, 3000);
        power_supply_changed(sc_device->ac_psy);
        power_supply_changed(sc_device->usb_psy);
        power_supply_changed(sc_device->charger_psy);
        dev_info(sc_device->dev, "set charge mode to qc1.0");
    }

    if (current_max_sgm7220 == 1500 ||
        current_max_wusb3801 == 1500) {
        sc89890h_set_usbin_current_limit(sc_device, 1500);
        dev_info(sc_device->dev, "change charge current to 1500ma");
    }

    sc89890h_get_status(sc_device, &state);
    if (!state.vbus_gd) {
        sc_device->voltage_max = 0;
        sc_device->current_max = 0;
        sc_device->state.vbus_stat = 0;
        sc_device->state.chrg_stat = 0;
        power_supply_changed(sc_device->ac_psy);
        power_supply_changed(sc_device->usb_psy);
        power_supply_changed(sc_device->charger_psy);
        dev_info(sc_device->dev, "charger offline in hvdcp work");
    }
}

static void sc89890h_psy_change_work(
    struct work_struct *work)
{
    struct sc89890h_device *sc_device = container_of(
        work, struct sc89890h_device, psy_work.work);

    power_supply_changed(sc_device->ac_psy);
    power_supply_changed(sc_device->usb_psy);
    power_supply_changed(sc_device->charger_psy);
}

static irqreturn_t sc89890h_irq_handler_thread(
    int irq, void *private)
{
    struct sc89890h_state state;
    struct sc89890h_device *sc_device = private;

    dev_info(sc_device->dev, "charger isr");
    sc89890h_get_status(sc_device, &state);
    if (state.vbus_gd) {
        dev_info(sc_device->dev, "vbus good");
        if (!sc_device->state.vbus_stat) {
            sc_device->state = state;
            dev_info(sc_device->dev, "state changed");
            if (sc_device->state.vbus_stat > SC89890H_USB_NON &&
                sc_device->state.vbus_stat < SC89890H_USB_OTG) {
                dev_info(sc_device->dev, "schedule irq work");
                schedule_delayed_work(&sc_device->irq_work, msecs_to_jiffies(200));
                schedule_delayed_work(&sc_device->charge_work, msecs_to_jiffies(3000));
            }
        }
    } else {
        sc_device->ibat = 3000;
        sc_device->voltage_max = 0;
        sc_device->current_max = 0;
        sc_device->state.vbus_stat = 0;
        sc_device->state.chrg_stat = 0;
        sc_device->temp_state = SC89890H_STATE_NORMAL;
        sc89890h_charge_enable_ctrl(sc_device, 1);
        cancel_delayed_work(&sc_device->irq_work);
        cancel_delayed_work(&sc_device->charge_work);
        cancel_delayed_work(&sc_device->hvdcp_work);
        sc89890h_update_bits(sc_device,
            SC89890H_CHRG_CTRL_1, SC89890H_DPM_VSEL_MASK,
            SC89890H_DP_HIZ | SC89890H_DM_HIZ);
        dev_info(sc_device->dev, "cancel irq work");
    }

    power_supply_changed(sc_device->ac_psy);
    power_supply_changed(sc_device->usb_psy);
    power_supply_changed(sc_device->charger_psy);

    return IRQ_HANDLED;
}

static void sc89890h_charge_irq_work(
    struct work_struct *work)
{
    struct sc89890h_device *sc_device = container_of(
            work, struct sc89890h_device, irq_work.work);

    dev_info(sc_device->dev, "irq work");
    if (sc_device->state.vbus_stat == SC89890H_USB_DCP) {
        dev_info(sc_device->dev, "dcp detected");
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1000000;
        sc89890h_set_usbin_current_limit(sc_device, 2000);
        // Vdp keep 0.6V for 1.5s, charger disconnect dpdm
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DP_VSEL_MASK, SC89890H_DP_0V6);
        schedule_delayed_work(&sc_device->hvdcp_work, msecs_to_jiffies(1500));
    } else if (sc_device->state.vbus_stat == SC89890H_USB_SDP) {
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 500000;
        dev_info(sc_device->dev, "sdp detected");
    } else if (sc_device->state.vbus_stat == SC89890H_USB_CDP) {
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1500000;
        dev_info(sc_device->dev, "cdp detected");
    } else if (sc_device->state.vbus_stat == SC89890H_USB_UNKNOWN ||
        sc_device->state.vbus_stat == SC89890H_USB_NSTANDA) {
        sc_device->voltage_max = 5000000;
        sc_device->current_max = 1000000;
        sc89890h_set_usbin_current_limit(sc_device, 1000);
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DP_VSEL_MASK, SC89890H_DP_0V6);
        schedule_delayed_work(&sc_device->hvdcp_work, msecs_to_jiffies(1500));
        dev_info(sc_device->dev, "unknown charge detected");
    }

    power_supply_changed(sc_device->ac_psy);
    power_supply_changed(sc_device->usb_psy);
    power_supply_changed(sc_device->charger_psy);
}

static int sc89890h_charger_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret, value;
    struct sc89890h_device *sc_device = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_PARALLEL;
        dev_info(sc_device->dev, "charger psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (sc_device->state.vbus_gd)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sc_device->dev, "charger online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        val->intval = sc89890h_charge_type[sc_device->state.chrg_stat];
        dev_info(sc_device->dev, "charger psy charge type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_STATUS:
        value = sc_device->state.chrg_stat;
        if (value == SC89890H_PRECHARGE || value == SC89890H_FAST_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
        else if (!sc_device->state.vbus_gd)
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (sc_device->state.vbus_gd && !sc_device->chg_en)
            val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if (value == SC89890H_TERM_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_FULL;
        else
            val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
        dev_info(sc_device->dev, "charger psy prop status: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
        // 1:SDP/2:CDP/3:DCP/0,4,5,6,7:unknown
        if (sc_device->state.vbus_stat > 0 && sc_device->state.vbus_stat < 4)
            val->intval = sc89890h_usb_type[sc_device->state.vbus_stat - 1];
        else
            val->intval = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        dev_info(sc_device->dev, "charger usb type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        ret = iio_read_channel_processed(sc_device->vbus, &value);
        if (ret < 0) {
            dev_err(sc_device->dev, "get charger voltage failed");
            return -EINVAL;
        }
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        val->intval = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
        dev_info(sc_device->dev, "charger voltage: %d", val->intval);
        break;
    default:
        dev_err(sc_device->dev, "get charger prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply_desc sc89890h_charger_desc = {
    .name = "mtk_charger_type",
    .type = POWER_SUPPLY_TYPE_PARALLEL,
    .usb_types = sc89890h_usb_type,
    .num_usb_types = ARRAY_SIZE(sc89890h_usb_type),
    .properties = sc89890h_charger_props,
    .num_properties = ARRAY_SIZE(sc89890h_charger_props),
    .get_property = sc89890h_charger_get_property,
};

static char *sc89890h_charger_supplied_to[] = {
    "battery",
    "mtk-master-charger",
};

static struct power_supply *sc89890h_register_charger_psy(
    struct sc89890h_device *sc_device)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sc_device,
        .of_node = sc_device->dev->of_node,
    };

    psy_cfg.supplied_to = sc89890h_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(sc89890h_charger_supplied_to);
    return power_supply_register(sc_device->dev, &sc89890h_charger_desc, &psy_cfg);
}

static int sc89890h_ac_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct sc89890h_device *sc_device = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        if (sc_device->state.vbus_stat == SC89890H_USB_DCP || 
            sc_device->state.vbus_stat == SC89890H_USB_UNKNOWN || 
            sc_device->state.vbus_stat == SC89890H_USB_NSTANDA)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sc_device->dev, "ac online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = sc_device->current_max;
        dev_info(sc_device->dev, "ac current maxium: %d", sc_device->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = sc_device->voltage_max;
        dev_info(sc_device->dev, "ac voltage maxium: %d", sc_device->voltage_max);
        break;
    default:
        dev_err(sc_device->dev, "get ac prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply_desc sc89890h_ac_desc = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .properties = sc89890h_ac_props,
    .num_properties = ARRAY_SIZE(sc89890h_ac_props),
    .get_property = sc89890h_ac_get_property,
};

static struct power_supply *sc89890h_register_ac_psy(
    struct sc89890h_device *sc_device)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sc_device,
        .of_node = sc_device->dev->of_node,
    };

    return power_supply_register(sc_device->dev, &sc89890h_ac_desc, &psy_cfg);
}

static int sc89890h_usb_writeable(
    struct power_supply *psy, enum power_supply_property psp)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        return 1;
    default:
        return 0;
    }
}

static int sc89890h_usb_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct sc89890h_device *sc_device = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_USB;
        dev_info(sc_device->dev, "usb psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        val->intval = sc_device->chg_en;
        dev_info(sc_device->dev, "usb psy prop charge enable: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (sc_device->state.vbus_stat == SC89890H_USB_SDP ||
            sc_device->state.vbus_stat == SC89890H_USB_CDP)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sc_device->dev, "usb online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = sc_device->current_max;
        dev_info(sc_device->dev, "usb current maxium: %d", sc_device->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = sc_device->voltage_max;
        dev_info(sc_device->dev, "usb voltage maxium: %d", sc_device->voltage_max);
        break;
    default:
        dev_err(sc_device->dev, "get usb prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static int sc89890h_usb_set_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    const union power_supply_propval *val)
{
    struct sc89890h_device *sc_device = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        sc89890h_charge_enable_ctrl(sc_device, !!val->intval);
        schedule_delayed_work(&sc_device->psy_work, msecs_to_jiffies(100));
        return 0;
    default:
        return -EPERM;
    }
}

static struct power_supply_desc sc89890h_usb_desc = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .properties = sc89890h_usb_props,
    .num_properties = ARRAY_SIZE(sc89890h_usb_props),
    .get_property = sc89890h_usb_get_property,
    .set_property = sc89890h_usb_set_property,
    .property_is_writeable = sc89890h_usb_writeable,
};

static struct power_supply *sc89890h_register_usb_psy(
    struct sc89890h_device *sc_device)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sc_device,
        .of_node = sc_device->dev->of_node,
    };

    return power_supply_register(sc_device->dev, &sc89890h_usb_desc, &psy_cfg);
}

static int sc89890h_enable_vbus(struct regulator_dev *rdev)
{
    struct sc89890h_device *sc_device = rdev_get_drvdata(rdev);

    sc_device->otg_mode = 1;
    dev_info(sc_device->dev, "sc89890h enable otg");
    return sc89890h_update_bits(sc_device,
            SC89890H_CHRG_CTRL_3, SC89890H_OTG_CHG_MASK, SC89890H_OTG_ENABLE);
}

static int sc89890h_disable_vbus(struct regulator_dev *rdev)
{
    struct sc89890h_device *sc_device = rdev_get_drvdata(rdev);

    sc_device->otg_mode = 0;
    dev_info(sc_device->dev, "sc89890h dibable otg");
    return sc89890h_update_bits(sc_device,
        SC89890H_CHRG_CTRL_3, SC89890H_OTG_CHG_MASK,
        sc_device->chg_en ? SC89890H_CHRG_EN : 0);
}

static int sc89890h_vbus_state(struct regulator_dev *rdev)
{
    u8 otg_en;
    struct sc89890h_device *sc_device = rdev_get_drvdata(rdev);

    sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_3, &otg_en);
    otg_en = !!(otg_en & SC89890H_OTG_ENABLE);
    dev_info(sc_device->dev, "sc89890h otg state: %d", otg_en);

    return otg_en;
}

static struct regulator_ops sc89890h_vbus_ops = {
    .enable = sc89890h_enable_vbus,
    .disable = sc89890h_disable_vbus,
    .is_enabled = sc89890h_vbus_state,
};

static const struct regulator_desc sc89890h_otg_rdesc = {
    .name = "sc89890h-vbus",
    .of_match = "sc89890h-otg-vbus",
    .ops = &sc89890h_vbus_ops,
    .owner = THIS_MODULE,
    .type = REGULATOR_VOLTAGE,
};

static struct regulator_dev *sc89890h_register_regulator(
    struct sc89890h_device *sc_device)
{
    struct regulator_config config = {
        .dev = sc_device->dev,
        .driver_data = sc_device,
    };

    return regulator_register(&sc89890h_otg_rdesc, &config);
}

static int sc89890h_gpio_config(struct sc89890h_device *sc_device)
{
    int ret, irqn;

    sc_device->chg_en_gpio = of_get_named_gpio(sc_device->dev->of_node, "sc,chg-en-gpio", 0);
    if (!gpio_is_valid(sc_device->chg_en_gpio)) {
        dev_err(sc_device->dev, "get sc,chg-en-gpio failed");
        return -EINVAL;
    }
    ret = gpio_request_one(sc_device->chg_en_gpio, GPIOF_OUT_INIT_LOW, "sc_chg_en_pin");
    if (ret) {
        dev_err(sc_device->dev, "request sc,chg-en-gpio failed");
        return -EPERM;
    }

    sc_device->otg_en_gpio = of_get_named_gpio(sc_device->dev->of_node, "sc,otg-en-gpio", 0);
    if (!gpio_is_valid(sc_device->otg_en_gpio)) {
        dev_err(sc_device->dev, "get sc,otg-en-gpio failed");
        goto err_otg_gpio;
    }
    ret = gpio_request_one(sc_device->otg_en_gpio, GPIOF_OUT_INIT_HIGH, "sc_otg_en_pin");
    if (ret) {
        dev_err(sc_device->dev, "request sc,otg-en-gpio failed");
        goto err_otg_gpio;
    }

    sc_device->irq_gpio = of_get_named_gpio(sc_device->dev->of_node, "sc,irq-gpio", 0);
    if (!gpio_is_valid(sc_device->irq_gpio)) {
        dev_err(sc_device->dev, "get sc,irq-gpio failed\n");
        goto err_irq_gpio;
    }
    irqn = gpio_to_irq(sc_device->irq_gpio);
    if (irqn > 0)
        sc_device->client->irq = irqn;
    else {
        dev_err(sc_device->dev, "map sc,irq-gpio to irq failed");
        goto err_irq_gpio;
    }

    ret = gpio_request_one(sc_device->irq_gpio, GPIOF_DIR_IN, "sc89890h_irq_pin");
    if (ret) {
        dev_err(sc_device->dev, "request sc_device,irq-gpio failed\n");
        goto err_irq_gpio;
    }

    return 0;

err_irq_gpio:
    gpio_free(sc_device->otg_en_gpio);
err_otg_gpio:
    gpio_free(sc_device->chg_en_gpio);
    
    return -EINVAL;
}

static ssize_t sc89890h_reg_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    int i, ret;
    char *reg_val[2];
    ulong addr, val;
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    if (strstr(buf, ":")) {
        for (i = 0; i < ARRAY_SIZE(reg_val); i++)
            reg_val[i] = strsep((char **)&buf, ":");

        ret = kstrtoul(reg_val[0], 16, &addr);
        if (ret < 0) {
            dev_err(dev, "get reg addr failed\n");
            return -EINVAL;
        }

        ret = kstrtoul(reg_val[1], 16, &val);
        if (ret < 0) {
            dev_err(dev, "get reg val failed\n");
            return -EINVAL;
        }
        ret = i2c_smbus_write_byte_data(sc_device->client, addr, val);
        if (ret < 0) {
            dev_err(dev, "write reg[%x]: %x failed\n",
                (uint32_t)addr, (uint32_t)val);
            return -EIO;
        }
    } else {
        ret = kstrtoul(buf, 16, &val);
        if (ret) {
            dev_err(dev, "kstrtoul [%s] failed\n", buf);
            return -EINVAL;
        }
        sc_device->reg_addr = val;
    }

    return size;
}

static struct device_attribute dev_attr_reg =
    __ATTR(reg, S_IWUSR, NULL, sc89890h_reg_store);

static ssize_t sc89890h_reg_dump(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int i, ret;
    ssize_t len = 0;
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);
    u8 reg_index[21] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14,
    };

    for (i = 0; i < ARRAY_SIZE(reg_index); i++) {
        ret = i2c_smbus_read_byte_data(sc_device->client, reg_index[i]);
        if (ret < 0) {
            dev_err(dev, "read reg [%#x] failed\n", sc_device->reg_addr);
            return -EINVAL;
        }
        len += snprintf(buf + len, PAGE_SIZE, "%#04x: %#04x\n", reg_index[i], ret);
    }

    return len;
}

static struct device_attribute dev_attr_dump =
    __ATTR(dump, S_IRUSR, sc89890h_reg_dump, NULL);

static ssize_t sc89890h_charge_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    if (!strncmp(buf, "1", 1)) {
        sc_device->qc_force = QC_TYPE_FORCE_1P0;
        dev_info(dev, "enable qc1.0");
    } else if (!strncmp(buf, "2", 1)) {
        sc_device->qc_force = QC_TYPE_FORCE_2P0;
        dev_info(dev, "enable qc2.0");
    } else if (!strncmp(buf, "3", 1)) {
        sc_device->qc_force = QC_TYPE_FORCE_3P0;
        dev_info(dev, "enable qc3.0");
    } else {
        dev_err(dev, "invalid parameter: %s", buf);
        return -EINVAL;
    }

    return size;
}

static struct device_attribute dev_attr_charge =
    __ATTR(charge, S_IWUSR, NULL, sc89890h_charge_store);

static ssize_t sc89890h_dpdm_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    if (!strncmp(buf, "dp", 2)) {
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DP_VSEL_MASK, SC89890H_DP_3V3);
        usleep_range(100, 100);
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DP_VSEL_MASK, SC89890H_DP_0V6);
    } else if (!strncmp(buf, "dm", 2)) {
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DM_VSEL_MASK, SC89890H_DM_0V6);
        usleep_range(1000, 1000);
        sc89890h_update_bits(sc_device, SC89890H_CHRG_CTRL_1,
            SC89890H_DM_VSEL_MASK, SC89890H_DM_3V3);
    } else {
        dev_err(dev, "error params");
        return -EINVAL;
    }

    return size;
}

static struct device_attribute dev_attr_dpdm =
    __ATTR(dpdm, S_IWUSR, NULL, sc89890h_dpdm_store);

static ssize_t sc89890h_qc3v0_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    sscanf(buf, "%d %d\n", &sc_device->chg_volt, &sc_device->chg_curr);
    dev_info(dev, "qc3.0 voltage: %d, current: %d",
        sc_device->chg_volt, sc_device->chg_curr);

    return size;
}

static struct device_attribute dev_attr_qc3v0 =
    __ATTR(qc3v0, S_IWUSR, NULL, sc89890h_qc3v0_store);

static ssize_t sc89890h_vtemp_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    sscanf(buf, "%d\n", &sc_device->vtemp);
    sc89890h_vtemp = sc_device->vtemp;
    schedule_delayed_work(&sc_device->psy_work, msecs_to_jiffies(100));
    dev_info(dev, "virtual temperature %d", sc_device->vtemp);

    return size;
}

static struct device_attribute dev_attr_vtemp =
    __ATTR(vtemp, S_IWUSR, NULL, sc89890h_vtemp_store);

static int sc89890h_charging_switch(
    struct charger_device *chg_dev, bool enable)
{
    return 0;
}

static int sc89890h_set_ichrg_curr(
    struct charger_device *chg_dev, unsigned int chrg_curr)
{
    return 0;
}

static int sc89890h_get_input_curr_lim(
    struct charger_device *chg_dev, unsigned int *ilim)
{
    int ret;
    u8 reg_val;
    struct sc89890h_device *sc_device = charger_get_data(chg_dev);

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_0, &reg_val);
    if (ret)
        return -EINVAL;

    *ilim = ((reg_val & SC89890H_IINDPM_I_MASK)
            * SC89890H_IINDPM_STEP_MA
            + SC89890H_IINDPM_I_MIN_MA) * 1000;

    return 0;
}

static int sc89890h_set_input_curr_lim(
    struct charger_device *chg_dev, unsigned int iindpm)
{
    return 0;
}

static int sc89890h_get_chrg_volt(
    struct charger_device *chg_dev, unsigned int *volt)
{
    int ret;
    u8 vreg_val;
    struct sc89890h_device *sc_device = charger_get_data(chg_dev);

    ret = sc89890h_read_reg(sc_device, SC89890H_CHRG_CTRL_6, &vreg_val);
    if (ret)
        return ret;

    vreg_val = (vreg_val & SC89890H_VBAT_VREG_MASK) >> 2;
    *volt = (vreg_val * SC89890H_VBAT_VREG_STEP_MV
            + SC89890H_VBAT_VREG_MIN_MV) * 1000;

    return 0;
}

static int sc89890h_set_chrg_volt(
    struct charger_device *chg_dev, unsigned int chrg_volt)
{
    return 0;
}

static int sc89890h_reset_watch_dog_timer(
    struct charger_device *chg_dev)
{
    return 0;
}

static int sc89890h_set_input_volt_lim(
    struct charger_device *chg_dev, unsigned int vindpm)
{
    return 0;
}

static int sc89890h_get_charging_status(
    struct charger_device *chg_dev, bool *is_done)
{
    struct sc89890h_device *sc_device = charger_get_data(chg_dev);

    if (sc_device->state.chrg_stat == SC89890H_TERM_CHARGE)
        *is_done = true;
    else
        *is_done = false;

    return 0;
}

static int sc89890h_enable_safetytimer(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sc89890h_get_is_safetytimer_enable(
    struct charger_device *chg_dev, bool *en)
{    
    *en = false;

    return 0;
}

static int sc89890h_enable_otg(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sc89890h_set_boost_current_limit(
    struct charger_device *chg_dev, u32 uA)
{
    return 0;
}

static int sc89890h_do_event(
    struct charger_device *chg_dev, u32 event,
                u32 args)
{
    if (chg_dev == NULL)
        return -EINVAL;

    switch (event) {
    case CHARGER_DEV_NOTIFY_EOC:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
        break;
    case CHARGER_DEV_NOTIFY_RECHG:
        charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
        break;
    default:
        break;
    }

    return 0;
}

static int sc89890h_en_pe_current_partern(
    struct charger_device *chg_dev, bool is_up)
{
    return 0;
}

static struct charger_ops sc89890h_chg_ops = {
    .enable = sc89890h_charging_switch,
    .get_charging_current = NULL,
    .set_charging_current = sc89890h_set_ichrg_curr,
    .get_input_current = sc89890h_get_input_curr_lim,
    .set_input_current = sc89890h_set_input_curr_lim,
    .get_constant_voltage = sc89890h_get_chrg_volt,
    .set_constant_voltage = sc89890h_set_chrg_volt,
    .kick_wdt = sc89890h_reset_watch_dog_timer,
    .set_mivr = sc89890h_set_input_volt_lim,
    .is_charging_done = sc89890h_get_charging_status,
    .enable_safety_timer = sc89890h_enable_safetytimer,
    .is_safety_timer_enabled = sc89890h_get_is_safetytimer_enable,
    .enable_otg = sc89890h_enable_otg,
    .set_boost_current_limit = sc89890h_set_boost_current_limit,
    .event = sc89890h_do_event,
    .send_ta_current_pattern = sc89890h_en_pe_current_partern,
    .set_pe20_efficiency_table = NULL,
    .send_ta20_current_pattern = NULL,
    .enable_cable_drop_comp = NULL,
};

static const struct charger_properties sc89890h_chg_props = {
    .alias_name = "sc89890h",
};

static int sc89890h_create_sysfs_file(
    struct sc89890h_device *sc_device)
{
    int ret;

    ret = device_create_file(sc_device->dev, &dev_attr_reg);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file reg failed\n");
        goto err_create_reg;
    }

    ret = device_create_file(sc_device->dev, &dev_attr_dump);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file dump failed\n");
        goto err_create_dump;
    }

    ret = device_create_file(sc_device->dev, &dev_attr_charge);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file hvdcp_enable failed\n");
        goto err_create_charge;
    }

    ret = device_create_file(sc_device->dev, &dev_attr_dpdm);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file dpdm failed\n");
        goto err_create_dpdm;
    }

    ret = device_create_file(sc_device->dev, &dev_attr_qc3v0);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file dpdm failed\n");
        goto err_create_qc3v0;
    }

    ret = device_create_file(sc_device->dev, &dev_attr_vtemp);
    if (ret) {
        dev_err(sc_device->dev, "device_create_file vtemp failed\n");
        goto err_create_vtemp;
    }

    return 0;

err_create_vtemp:
    device_remove_file(sc_device->dev, &dev_attr_qc3v0);
err_create_qc3v0:
    device_remove_file(sc_device->dev, &dev_attr_dpdm);
err_create_dpdm:
    device_remove_file(sc_device->dev, &dev_attr_charge);
err_create_charge:
    device_remove_file(sc_device->dev, &dev_attr_dump);
err_create_dump:
    device_remove_file(sc_device->dev, &dev_attr_reg);
err_create_reg:
    return -EPERM;
}

static int sc89890h_driver_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    struct sc89890h_device *sc_device;

    sc_device = devm_kzalloc(&client->dev, sizeof(*sc_device), GFP_KERNEL);
    if (!sc_device) {
        dev_err(&client->dev, "allocate memory for sc89890h failed\n");
        return -ENOMEM;
    }

    sc_device->chg_en = 1;
    sc_device->ibat = 3000;
    sc_device->chg_volt = 5600;
    sc_device->chg_curr = 3000;
    sc_device->vtemp = SC89890H_TEMP_INVALID;
    sc_device->qc_type = QUICK_CHARGE_1P0;
    sc_device->qc_force = QC_TYPE_AUTO_CHECK;
    sc_device->temp_state = SC89890H_STATE_NORMAL;
    sc_device->client = client;
    sc_device->dev = &client->dev;
    sc_device->chg_desc = &sc89890h_charger_desc;
    i2c_set_clientdata(client, sc_device);
    INIT_DELAYED_WORK(&sc_device->irq_work, sc89890h_charge_irq_work);
    INIT_DELAYED_WORK(&sc_device->psy_work, sc89890h_psy_change_work);
    INIT_DELAYED_WORK(&sc_device->hvdcp_work, sc89890h_hvdcp_detect_work);
    INIT_DELAYED_WORK(&sc_device->charge_work, sc89890h_charge_online_work);

    sc_device->vbus = devm_iio_channel_get(sc_device->dev, "pmic_vbus");
    if (IS_ERR_OR_NULL(sc_device->vbus)) {
        dev_err(sc_device->dev, "sc89890h get vbus failed\n");
        return -EPROBE_DEFER;
    }

    ret = sc89890h_chipid_detect(sc_device);
    if (ret != SC89890H_PN_ID) {
        dev_err(sc_device->dev, "sc89890h not founded");
        devm_iio_channel_release(sc_device->dev, sc_device->vbus);
        devm_kfree(sc_device->dev, sc_device);
        return -ENODEV;
    }

    ret = sc89890h_gpio_config(sc_device);
    if (ret < 0) {
        dev_err(sc_device->dev, "sc89890h config gpio failed\n");
        goto err_gpio_config;
    }

    ret = sc89890h_hardware_init(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "sc89890h hardware init failed");
        goto err_hardware_init;
    }

    /* Register charger device */
    sc_device->chg_dev = charger_device_register("primary_chg",
                        &client->dev, sc_device,
                        &sc89890h_chg_ops,
                        &sc89890h_chg_props);
    if (IS_ERR_OR_NULL(sc_device->chg_dev)) {
        dev_err(sc_device->dev, "register charger device failed\n");
        ret = PTR_ERR(sc_device->chg_dev);
        goto err_hardware_init;
    }

    ret = devm_request_threaded_irq(sc_device->dev, client->irq,
                NULL, sc89890h_irq_handler_thread,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                dev_name(sc_device->dev), sc_device);
    if (ret) {
        dev_err(sc_device->dev, "sc89890h request interrupt failed\n");
        goto err_request_irq;
    }
    ret = enable_irq_wake(client->irq);
    if (ret) {
        dev_err(sc_device->dev, "sc89890h enable irq wake failed\n");
        goto err_irq_wake;
    }

    ret = sc89890h_get_initial_state(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "get initial state failed\n");
        goto err_irq_wake;
    }

    sc_device->charger_psy = sc89890h_register_charger_psy(sc_device);
    if (IS_ERR_OR_NULL(sc_device->charger_psy)) {
        dev_err(sc_device->dev, "register charger power supply failed\n");
        goto err_register_charger_psy;
    }

    sc_device->usb_psy = sc89890h_register_usb_psy(sc_device);
    if (IS_ERR_OR_NULL(sc_device->usb_psy)) {
        dev_err(sc_device->dev, "register usb power supply failed\n");
        goto err_register_usb_psy;
    }

    sc_device->ac_psy = sc89890h_register_ac_psy(sc_device);
    if (IS_ERR_OR_NULL(sc_device->ac_psy)) {
        dev_err(sc_device->dev, "register ac power supply failed\n");
        goto err_register_ac_psy;
    }

    sc_device->otg_rdev = sc89890h_register_regulator(sc_device);
    if (IS_ERR_OR_NULL(sc_device->otg_rdev)) {
        dev_err(sc_device->dev, "sc89890h register regulator failed\n");
        goto err_register_rdev;
    }

    ret = sc89890h_create_sysfs_file(sc_device);
    if (ret) {
        dev_err(sc_device->dev, "sc89890h create sysfs failed\n");
        goto err_create_sysfs;
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    scc_init_speed_current_map("mediatek,mt6357-gauge");
    scc_create_file(sc_device->dev);
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    dev_info(sc_device->dev, "sc89890h probe finished");

    return 0;

err_create_sysfs:
    regulator_unregister(sc_device->otg_rdev);
err_register_rdev:
    power_supply_unregister(sc_device->ac_psy);
err_register_ac_psy:
    power_supply_unregister(sc_device->usb_psy);
err_register_usb_psy:
    power_supply_unregister(sc_device->charger_psy);
err_register_charger_psy:
    disable_irq_wake(client->irq);
err_irq_wake:
    free_irq(client->irq, sc_device);
err_request_irq:
    charger_device_unregister(sc_device->chg_dev);
err_hardware_init:
    gpio_free(sc_device->irq_gpio);
    gpio_free(sc_device->otg_en_gpio);
    gpio_free(sc_device->chg_en_gpio);
err_gpio_config:
    devm_iio_channel_release(sc_device->dev, sc_device->vbus);
    devm_kfree(sc_device->dev, sc_device);
    return -EBUSY;
}

static void sc89890h_remove_sysfs_file(
    struct sc89890h_device *sc_device)
{
    device_remove_file(sc_device->dev, &dev_attr_vtemp);
    device_remove_file(sc_device->dev, &dev_attr_qc3v0);
    device_remove_file(sc_device->dev, &dev_attr_dpdm);
    device_remove_file(sc_device->dev, &dev_attr_charge);
    device_remove_file(sc_device->dev, &dev_attr_dump);
    device_remove_file(sc_device->dev, &dev_attr_reg);
}

static int sc89890h_charger_remove(struct i2c_client *client)
{
    struct sc89890h_device *sc_device = i2c_get_clientdata(client);

    sc89890h_remove_sysfs_file(sc_device);
    regulator_unregister(sc_device->otg_rdev);
    power_supply_unregister(sc_device->usb_psy);
    power_supply_unregister(sc_device->ac_psy);
    power_supply_unregister(sc_device->charger_psy);
    disable_irq_wake(client->irq);
    free_irq(client->irq, sc_device);
    charger_device_unregister(sc_device->chg_dev);
    gpio_free(sc_device->irq_gpio);
    gpio_free(sc_device->otg_en_gpio);
    gpio_free(sc_device->chg_en_gpio);
    devm_iio_channel_release(sc_device->dev, sc_device->vbus);
    devm_kfree(sc_device->dev, sc_device);

    return 0;
}

static int sc89890h_device_suspend(struct device *dev)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    if (sc_device->state.vbus_gd) {
        cancel_delayed_work(&sc_device->charge_work);
        dev_info(sc_device->dev, "cancel charge work");
    }

    return 0;
}

static int sc89890h_device_resume(struct device *dev)
{
    struct sc89890h_device *sc_device = dev_get_drvdata(dev);

    if (sc_device->state.vbus_gd) {
        schedule_delayed_work(&sc_device->charge_work, msecs_to_jiffies(100));
        dev_info(sc_device->dev, "resume charge work");
    }

    return 0;
}

static struct dev_pm_ops sc89890h_pm_osp = {
    .suspend = sc89890h_device_suspend,
    .resume = sc89890h_device_resume,
};

static const struct i2c_device_id sc89890h_i2c_ids[] = {
    { "sc89890h", 0 }, { },
};

static const struct of_device_id sc89890h_of_match[] = {
    { .compatible = "southchip,sc89890h", }, { },
};

static struct i2c_driver sc89890h_driver = {
    .driver = {
        .name = "sc89890h-charger",
        .of_match_table = sc89890h_of_match,
        .pm = &sc89890h_pm_osp,
    },
    .probe = sc89890h_driver_probe,
    .remove = sc89890h_charger_remove,
    .id_table = sc89890h_i2c_ids,
};

module_i2c_driver(sc89890h_driver);
MODULE_LICENSE("GPL v2");

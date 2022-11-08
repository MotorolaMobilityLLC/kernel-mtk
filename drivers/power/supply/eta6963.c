#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/reboot.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include "eta6963.h"
#if defined (CONFIG_TINNO_SCC_SUPPORT)
#include <linux/scc_drv.h>
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

#undef dev_fmt
#define dev_fmt(fmt) "202206271430: " fmt

extern int mt6357_irq_flag;
extern int current_max_sgm7220;
extern int current_max_wusb3801;

int eta6963_vtemp = 250;
EXPORT_SYMBOL(eta6963_vtemp);

int eta6963_irq_flag = 0;
EXPORT_SYMBOL(eta6963_irq_flag);

extern int demomode_enable;
extern int demomode_max_soc;
extern int demomode_min_soc;

static char *eta6963_charge_state[] = {
    "charge-disable",
    "pre-charge",
    "fast-charge",
    "charge-terminated",
};

static enum power_supply_usb_type eta6963_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
};

static enum power_supply_usb_type eta6963_charge_type[] = {
    POWER_SUPPLY_CHARGE_TYPE_NONE,
    POWER_SUPPLY_CHARGE_TYPE_TRICKLE,
    POWER_SUPPLY_CHARGE_TYPE_FAST,
    POWER_SUPPLY_CHARGE_TYPE_TAPER,
};

static enum power_supply_property eta6963_charger_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_USB_TYPE,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static enum power_supply_property eta6963_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static enum power_supply_property eta6963_usb_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CHARGE_ENABLED,
};

static int eta6963_read_reg(
    struct eta6963_device *eta, u8 reg, u8 *data)
{
    int ret;

    ret = i2c_smbus_read_byte_data(eta->client, reg);
    if (ret < 0) {
        dev_err(eta->dev, "read reg: %#x failed", reg);
        return -EINVAL;
    } else
        *data = ret;

    return 0;
}

static int eta6963_write_reg(
    struct eta6963_device *eta, u8 reg, u8 value)
{
    int ret;

    ret = i2c_smbus_write_byte_data(eta->client, reg, value);
    if (ret < 0) {
        dev_err(eta->dev, "write reg: %#x failed", reg);
        return -EINVAL;
    }

    return 0;
}

static int eta6963_update_bits(
    struct eta6963_device *eta, u8 reg, u8 mask, u8 val)
{
    u8  tmp;
    int ret;

    ret = eta6963_read_reg(eta, reg, &tmp);
    if (ret)
        return ret;

    tmp &= ~mask;
    tmp |= val & mask;

    ret = eta6963_write_reg(eta, reg, tmp);
    if (ret)
        return ret;

    return 0;
}

static inline int eta6963_reset_register(
    struct eta6963_device *eta)
{
    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_B,
        ETA6963_REG_RESET, ETA6963_REG_RESET);
}

static int eta6963_disable_watchdog_timer(
    struct eta6963_device *eta)
{
    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_5,
        ETA6963_WDT_TIMER_MASK, ETA6963_WDT_TIMER_DISABLE);
}

static int eta6963_disable_iindpm_irq(
    struct eta6963_device *eta)
{
    return eta6963_update_bits(eta,
        ETA6963_CHRG_CTRL_A, ETA6963_DPM_INT_MASK, ETA6963_IINDPM_INT_MASK);
}

static void eta6963_charge_enable_ctrl(
    struct eta6963_device *eta, int en)
{   
    eta->chg_en = !!en;
    eta6963_update_bits(eta, ETA6963_CHRG_CTRL_1,
        ETA6963_CHRG_EN, en ? ETA6963_CHRG_EN : 0);
}

static inline int eta6963_get_charge_enable_status(
    struct eta6963_device *eta)
{
    u8 status;

    eta6963_read_reg(eta, ETA6963_CHRG_CTRL_1, &status);

    return !!(status & ETA6963_CHRG_EN);
}

static void eta6963_set_usbin_current_limit(
    struct eta6963_device *eta , unsigned int iindpm)
{
    u8 reg_val;

    reg_val = (iindpm - ETA6963_IINDPM_I_MIN_MA)
        / ETA6963_IINDPM_STEP_MA;
    eta6963_update_bits(eta, ETA6963_CHRG_CTRL_0,
        ETA6963_IINDPM_I_MASK, reg_val);
}

#if defined (CONFIG_TINNO_SCC_SUPPORT)
static int eta6963_get_cc_ibat_value(
    struct eta6963_device *eta)
{
    int ret;
    int ichg;
    u8 reg_val;

    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_2, &reg_val);
    if (ret < 0)
        ichg = -EINVAL;
    else
        ichg = reg_val & ETA6963_ICHRG_CUR_MASK;

    return ichg;
}
#endif /* CONFIG_TINNO_SCC_SUPPORT */

static int eta6963_set_fast_charge_current(
    struct eta6963_device *eta, unsigned int chrg_curr)
{
    u8 reg_val;

    reg_val = chrg_curr / ETA6963_ICHRG_CURRENT_STEP_MA;

    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_2,
        ETA6963_ICHRG_CUR_MASK, reg_val);
}

static int eta6963_set_taper_current(
    struct eta6963_device *eta, int term_current)
{
    u8 reg_val;
    
    reg_val = (term_current - ETA6963_TERMCHRG_I_MIN_MA) /
		ETA6963_TERMCHRG_CURRENT_STEP_MA;
 
    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_3,
        ETA6963_TERMCHRG_CUR_MASK, reg_val);
}

static int eta6963_set_prechg_current(
    struct eta6963_device *eta, int prechg_current)
{
    u8 reg_val;

    reg_val = (prechg_current - ETA6963_PRECHG_I_MIN_MA) /
		ETA6963_PRECHG_CURRENT_STEP_MA;
    reg_val <<= 4;
    
    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_3,
        ETA6963_PRECHG_CUR_MASK, reg_val);
}

static int eta6963_set_battery_voltage(
    struct eta6963_device *eta, unsigned int chrg_volt)
{
    u8 reg_val;

    reg_val = (chrg_volt - ETA6963_VREG_V_MIN_MV)
                / ETA6963_VREG_V_STEP_MV;
    reg_val = reg_val << 3;

    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_4,
        ETA6963_VREG_V_MASK, reg_val);
}

static int eta6963_recharge_voltage(
    struct eta6963_device *eta, unsigned int rechg_mv)
{
    u8 reg_val;

    if (rechg_mv == 120)
        reg_val = ETA6963_RECHARGE_VOLTAGE_120MV;
    else
        reg_val = ETA6963_RECHARGE_VOLTAGE_240MV;

    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_4,
        ETA6963_RECHARGE_VOLTAGE_MASK, reg_val);
}

static int eta6963_vindpm_track_set(
    struct eta6963_device *eta, unsigned int vindpm_mv)
{
    u8 reg_val;

    if (vindpm_mv == 200)
        reg_val = ETA6963_VINDPM_TRACK_SET_200MV;
    else if (vindpm_mv == 250)
        reg_val = ETA6963_VINDPM_TRACK_SET_250MV;
    else if (vindpm_mv == 300)
        reg_val = ETA6963_VINDPM_TRACK_SET_300MV;
    else
        reg_val = ETA6963_VINDPM_TRACK_SET_DISABLE;

    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_7,
        ETA6963_VINDPM_TRACK_SET_MASK, reg_val);
}

static int eta6963_set_boost_current(
    struct eta6963_device *eta, int boost_current)
{
    u8 reg_val;

    if (boost_current >= 1200)
        reg_val = ETA6963_BOOST_CUR_1A2;
    else
        reg_val = ETA6963_BOOST_CUR_0A5;

    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_2,
        ETA6963_BOOST_MASK, reg_val);
}

static int eta6963_set_vindpm(
    struct eta6963_device *eta, int vindpm)
{
    u8 reg_val;

    reg_val = (vindpm - 3900) / 100;
    return eta6963_update_bits(eta, ETA6963_CHRG_CTRL_6,
        ETA6963_VINDPM_V_MASK, reg_val);
}

static int eta6963_chipid_detect(struct eta6963_device *eta)
{
    int ret = 0;
    u8 val = 0;

    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_B, &val);
    if (ret < 0) {
        dev_err(eta->dev, "read ETA6963_CHRG_CTRL_B failed");
        return -EINVAL;
    }
    val = val & ETA6963_PN_MASK;
    dev_info(eta->dev, "read chipid reg value: %#02x", val);
    
    return val;
}

static int eta6963_hardware_init(struct eta6963_device *eta)
{
    int ret;

    ret = eta6963_set_taper_current(eta, 240);
    if (ret) {
        dev_err(eta->dev, "set eta6963 taper current failed");
        return -EPERM;
    }

    ret = eta6963_set_prechg_current(eta, 360);
    if (ret) {
        dev_err(eta->dev, "set eta6963 precharge current failed");
        return -EPERM;
    }

    ret = eta6963_set_battery_voltage(eta, 4456);
    if (ret) {
        dev_err(eta->dev, "set eta6963 charge voltage failed");
        return -EPERM;
    }
   
    ret = eta6963_recharge_voltage(eta, 240);
    if (ret) {
        dev_err(eta->dev, "set eta6963 recharge voltage failed");
        return -EPERM;
    }

    ret = eta6963_vindpm_track_set(eta, 0);
    if (ret) {
        dev_err(eta->dev, "set eta6963 vindpm track failed");
        return -EPERM;
    }

    ret = eta6963_set_fast_charge_current(eta, 2400);
    if (ret) {
        dev_err(eta->dev, "set eta6963 fast charge current failed");
        return -EPERM;
    }

    ret = eta6963_set_boost_current(eta, 1200);
    if (ret) {
            dev_err(eta->dev, "set eta6963 boost current failed");
            return -EPERM;
    }

    ret = eta6963_disable_watchdog_timer(eta);
    if (ret) {
        dev_err(eta->dev, "disable eta6963 watchdog timer failed");
        return -EPERM;
    }

    ret = eta6963_set_vindpm(eta, 4200);
    if (ret) {
        dev_err(eta->dev, "set eta6963 minum vindpm failed");
        return -EPERM;
    }

    ret = eta6963_disable_iindpm_irq(eta);
    if (ret) {
        dev_err(eta->dev, "disable eta6963 iindpm irq failed");
        return -EPERM;
    }

    return 0;
}

static int eta6963_get_vbus_voltage(struct eta6963_device *eta)
{
    int ret, value;

    ret = iio_read_channel_processed(eta->vbus, &value);
    if (ret < 0) {
        dev_err(eta->dev, "get charger voltage failed");
        return -EINVAL;
    }
    // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
    eta->vbus_voltage = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
    dev_info(eta->dev, "vbus voltage: %d", eta->vbus_voltage);

    return eta->vbus_voltage;
}

static int eta6963_vbus_online(struct eta6963_device *eta)
{
    int ret, online;
    
    ret = eta6963_get_vbus_voltage(eta);
    if (ret < 0) {
        dev_err(eta->dev, "vbus voltage invalid");
        return -EINVAL; 
    }
    online = eta->vbus_voltage >= VBUS_MINUM_LEVEL ? 1 : 0;
    dev_info(eta->dev, "vbus online: %d", online);

    return online;
}

static int eta6963_run_apsd(struct eta6963_device *eta, int on)
{
    int ret;
    union power_supply_propval psy_val;

    psy_val.intval = on;
    dev_info(eta->dev, "run apsd: %d", on);
    ret = power_supply_set_property(eta->aca_psy,
                POWER_SUPPLY_PROP_ONLINE, &psy_val);
    if (ret) {
        dev_err(eta->dev, "set aca psy online prop failed");
        return -EPERM;
    }

    ret = power_supply_get_property(eta->aca_psy,
                POWER_SUPPLY_PROP_USB_TYPE, &psy_val);
    if (ret) {
        dev_err(eta->dev, "get aca psy usb type prop failed");
        return -EPERM;
    }
    eta->state.vbus_stat = psy_val.intval;
    dev_info(eta->dev, "bc1.2 usb type: %d", eta->state.vbus_stat);

    return 0;
}

static void eta6963_charger_plug_out(struct eta6963_device *eta)
{
    eta->ibat = 2400;
    eta->voltage_max = 0;
    eta->current_max = 0;
    eta->state.vbus_stat = 0;
    eta->state.chrg_stat = 0;
    eta->vbus_online = 0;
    eta->temp_state = ETA6963_STATE_NORMAL;
    eta6963_charge_enable_ctrl(eta, 1);
    cancel_delayed_work(&eta->irq_work);
    cancel_delayed_work(&eta->curr_work);
    cancel_delayed_work(&eta->charge_work);
    dev_info(eta->dev, "charger plug out detected");
}

static int eta6963_get_charge_state(
    struct eta6963_device *eta)
{
    int ret;
    u8 status;

    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_8, &status);
    if (ret) {
        dev_err(eta->dev, "get eta6963 status register failed");
        return -EINVAL;
    }

    return status;
}

static int eta6963_get_status(
    struct eta6963_device *eta, struct eta6963_state *state)
{
    int ret;
    u8 status, fault, ctrl;

    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_8, &status);
    if (ret) {
        dev_err(eta->dev, "read eta6963 0x08 register failed");
        return -EINVAL;
    }
    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_9, &fault);
    if (ret) {
        dev_err(eta->dev, "read eta6963 0x09 register failed");
        return -EINVAL;
    }
    ret = eta6963_read_reg(eta, ETA6963_CHRG_CTRL_A, &ctrl);
    if (ret) {
        dev_err(eta->dev, "read eta6963 0x0A register failed");
        return -EINVAL;
    }

    state->pg_stat = (status & ETA6963_PG_GOOD_MASK) >> 2;
    state->chrg_stat = (status & ETA6963_CHG_STAT_MASK) >> 3;
    state->vbus_stat = (status & ETA6963_VBUS_STAT_MASK) >> 5;

    dev_info(eta->dev, "[08]: %#x, [09]: %#x, [0A]: %#x",
        status, fault, ctrl);

    return 0;
}

static int eta6963_get_initial_state(
    struct eta6963_device *eta)
{
    int ret, online;
    struct eta6963_state state;

    eta6963_get_status(eta, &state);
    online = eta6963_vbus_online(eta);
    if (online > 0) {
        eta->vbus_online = 1;
        dev_info(eta->dev, "charger online during boot\n");
        ret = eta6963_run_apsd(eta, 1);
        if (ret) {
            dev_err(eta->dev, "charger run apsd failed during boot\n");
            return -EINVAL;
        }
        schedule_delayed_work(&eta->curr_work, msecs_to_jiffies(1000));
        schedule_delayed_work(&eta->charge_work, msecs_to_jiffies(3000));
    } else {
        if (strstr(boot_command_line, "androidboot.bootreason=usb")) {
            dev_info(eta->dev, "charger off line, power off machine after 10s\n");
            schedule_delayed_work(&eta->poff_work, msecs_to_jiffies(10000));
       }
    }

    return 0;
}

static int eta6963_get_battery_temperature(
    struct eta6963_device *eta)
{
    int ret;
    int charge_state;
    union power_supply_propval psy_prop;

    ret = power_supply_get_property(eta->battery_psy,
            POWER_SUPPLY_PROP_TEMP, &psy_prop);
    if (ret) {
        dev_err(eta->dev, "get battery temp failed");
        return -ENODEV;
    }

    // set virtual temperature
    if (eta->vtemp != ETA6963_TEMP_INVALID) {
        psy_prop.intval = eta->vtemp;
        dev_info(eta->dev, "set vtemp: %d", eta->vtemp);
    }

    eta->temp = psy_prop.intval;
    dev_info(eta->dev, "battery temp: %d", eta->temp);

    if (eta->temp <= -170 || eta->temp > 570)
        charge_state = ETA6963_CHG_DISABLE;
    else if (eta->temp > -170 && eta->temp < 0)
        charge_state = ETA6963_STATE_COLD;
    else if (eta->temp >= 0 && eta->temp < 150)
        charge_state = ETA6963_STATE_COOL;
    else if (eta->temp > 150 && eta->temp <= 450)
        charge_state = ETA6963_STATE_NORMAL;
    else if (eta->temp > 450 && eta->temp <= 570)
        charge_state = ETA6963_STATE_WARM;

    return charge_state;
}

static void eta6963_set_charge_voltage_current(
    struct eta6963_device *eta)
{
    switch (eta->temp_state) {
    case ETA6963_STATE_COLD:
        dev_info(eta->dev, "cold state");
        eta->ibat = 960;
        eta6963_set_battery_voltage(eta, 4200);
        eta6963_set_fast_charge_current(eta, 960);
        break;
    case ETA6963_STATE_COOL:
        dev_info(eta->dev, "cool state");
        eta6963_set_battery_voltage(eta, 4456);
        break;
    case ETA6963_STATE_NORMAL:
        dev_info(eta->dev, "normal state");
        eta->ibat = 2400;
        eta6963_set_battery_voltage(eta, 4456);
        eta6963_set_fast_charge_current(eta, 2400);
        break;
    case ETA6963_STATE_WARM:
        dev_info(eta->dev, "warm state");
        eta->ibat = 1980;
        eta6963_set_battery_voltage(eta, 4200);
        eta6963_set_fast_charge_current(eta, 1980);
        break;
    case ETA6963_CHG_DISABLE:
        dev_info(eta->dev, "charge disabled state");
        break;
    default:
        dev_err(eta->dev, "invalid temp state: %d", eta->temp_state);
        break;
    }
}

static void eta6963_charge_online_work(
    struct work_struct *work)
{
    int ret, err = 0;
    int online, new_temp, chg_state;
#if defined (CONFIG_TINNO_SCC_SUPPORT)
    int ichg_o, ichg_n, scc_value;
#endif  /* CONFIG_TINNO_SCC_SUPPORT */
    union power_supply_propval psy_prop;

    struct eta6963_device *eta = container_of(
        work, struct eta6963_device, charge_work.work);

    if (!eta->battery_psy) {
        eta->battery_psy = power_supply_get_by_name("battery");
        if (!eta->battery_psy) {
            err = -ENODEV;
            dev_err(eta->dev, "get battery power supply failed");
            goto out;
        }
    }

    new_temp = eta6963_get_battery_temperature(eta);
    if (new_temp < 0) {
        err = -ENODEV;
        dev_err(eta->dev, "get charge state failed");
        goto out;
    }
    if (eta->temp_state != new_temp) {
        eta->temp_state = new_temp;
        dev_info(eta->dev, "temp status changed");
        if (eta->chg_en && eta->temp_state == ETA6963_CHG_DISABLE) {
            eta6963_charge_enable_ctrl(eta, 0);
            power_supply_changed(eta->charger_psy);
            dev_info(eta->dev, "disable charge, temperature: %d", eta->temp);
        } else if (!eta->chg_en) {
            eta6963_charge_enable_ctrl(eta, 1);
            power_supply_changed(eta->charger_psy);
            dev_info(eta->dev, "enable charge, temperature: %d", eta->temp);
        }
        eta6963_set_charge_voltage_current(eta);
    }

    if (eta->temp_state == ETA6963_STATE_COOL) {
        ret = power_supply_get_property(eta->battery_psy,
                POWER_SUPPLY_PROP_VOLTAGE_NOW, &psy_prop);
        if (ret) {
            dev_err(eta->dev, "get battery voltage failed");
            return;
        }
        if (psy_prop.intval < 4200000) {
            eta->ibat = 1980;
            eta6963_set_fast_charge_current(eta, eta->ibat);
            dev_info(eta->dev,
                "battery voltage: %d, set charge current to 1980",
                psy_prop.intval);
        } else {
            eta->ibat = 960;
            eta6963_set_fast_charge_current(eta, eta->ibat);
            dev_info(eta->dev,
                "battery voltage: %d, set charge current to 960",
                psy_prop.intval);
        }
    }
    if (eta->temp <= -100 || eta->temp >= 550) {
        power_supply_changed(eta->charger_psy);
        dev_info(eta->dev, "device shutdown, temperature: %d", eta->temp);  
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    if (eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_DCP) {
        scc_value = scc_get_current();
        scc_value /= 1000;
        if (scc_value > eta->ibat)
            scc_value = eta->ibat;
        dev_info(eta->dev, "scc current = %d, jeita current: %d",
            scc_value, eta->ibat);
        ichg_o = eta6963_get_cc_ibat_value(eta);
        if (ichg_o < 0) {
            err = -EINVAL;
            dev_err(eta->dev, "get cc ibat value failed");
            goto out;
        }
        ichg_n = (scc_value / 60) & ETA6963_ICHRG_CUR_MASK;
        if (ichg_n != ichg_o) {
            eta6963_set_fast_charge_current(eta, scc_value);
            dev_info(eta->dev, "scc set cc current = %d", scc_value);
        }
    }
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    if (demomode_enable) {
        ret = power_supply_get_property(eta->battery_psy,
                POWER_SUPPLY_PROP_CAPACITY, &psy_prop);
        if (ret) {
            err = -EINVAL;
            dev_err(eta->dev, "get battery capacity failed");
            goto out;
        }
        if (psy_prop.intval >= demomode_max_soc && eta->chg_en) {
            eta6963_charge_enable_ctrl(eta, 0);
            power_supply_changed(eta->charger_psy);
            dev_info(eta->dev, "disable charge due to max_soc");
        } else if (psy_prop.intval <= demomode_min_soc && !eta->chg_en) {
            eta6963_charge_enable_ctrl(eta, 1);
            power_supply_changed(eta->charger_psy);
            dev_info(eta->dev, "enable charge due to min_soc");
        }
    }

    ret = eta6963_get_charge_state(eta);
    if (ret < 0) {
        err = -EINVAL;
        dev_err(eta->dev, "get charge state failed");
        goto out;
    }
    chg_state = (ret & ETA6963_CHG_STAT_MASK) >> 3;
    if (eta->state.chrg_stat != chg_state) {
        eta->state.chrg_stat = chg_state;
        dev_info(eta->dev, "charge state: %s",
            eta6963_charge_state[chg_state]);
        power_supply_changed(eta->charger_psy);
    }

    online = eta6963_vbus_online(eta);

out:
    if (err || online == 0)
        cancel_delayed_work(&eta->charge_work);
    else
        schedule_delayed_work(&eta->charge_work, msecs_to_jiffies(1000));
}

static void eta6963_psy_changed_work(
    struct work_struct *work)
{
    struct eta6963_device *eta = container_of(
        work, struct eta6963_device, psy_work.work);

    power_supply_changed(eta->charger_psy);
}

static irqreturn_t eta6963_irq_handler_thread(
    int irq, void *private)
{
    struct eta6963_state state;
    struct eta6963_device *eta = private;

    dev_info(eta->dev, "charger isr");
	eta6963_irq_flag = 1;
	if (!mt6357_irq_flag) {
		mdelay(100);
		dev_info(eta->dev, "wait ic stable");
	}
    //pm_stay_awake(eta->dev);
    eta6963_get_status(eta, &state);
    eta6963_get_status(eta, &state);
    //schedule_delayed_work(&eta->irq_work, msecs_to_jiffies(0));
    schedule_work(&eta->irq_work.work);

    return IRQ_HANDLED;
}

static void eta6963_charge_irq_work(
    struct work_struct *work)
{
    int ret, online;
    struct eta6963_device *eta = container_of(
            work, struct eta6963_device, irq_work.work);

    dev_info(eta->dev, "irq work");

	if (!gpio_get_value(eta->id_gpio)) {
		dev_info(eta->dev, "usb host mode");
		goto out;
	}

    online = eta6963_vbus_online(eta);
    if (online > 0 && !eta->vbus_online) {
        dev_info(eta->dev, "charger online detected");
        eta->vbus_online = 1;
        ret = eta6963_run_apsd(eta, 1);
        if (ret) {
            dev_err(eta->dev, "charger run apsd failed in irq work");
            goto out;
        }
        schedule_delayed_work(&eta->curr_work, msecs_to_jiffies(2000));
        schedule_delayed_work(&eta->charge_work, msecs_to_jiffies(3000));
    } else if (online == 0 && eta->vbus_online) {
        dev_info(eta->dev, "charger offline detected");
        eta->vbus_online = 0;
        eta6963_charger_plug_out(eta);
        ret = eta6963_run_apsd(eta, 0);
        if (ret) {
            dev_err(eta->dev, "charger set aca psy offline failed");
            goto out;
        }
    } else {
        dev_info(eta->dev, "repeated state, vbus online: %d", online);
        goto out;
    }

    power_supply_changed(eta->charger_psy);

out:
	mt6357_irq_flag = 0;
	eta6963_irq_flag = 0;
    //pm_relax(eta->dev);
}

static void eta6963_charge_current_work(
    struct work_struct *work)
{
    struct eta6963_device *eta = container_of(
            work, struct eta6963_device, curr_work.work);

    if (eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_DCP) {
        dev_info(eta->dev, "dcp detected");
        eta->voltage_max = 5000000;
        eta->current_max = 2000000;
        eta6963_set_usbin_current_limit(eta, 2000);
    } else if (eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_SDP) {
        eta->voltage_max = 5000000;
        eta->current_max = 500000;
        eta6963_set_usbin_current_limit(eta, 500);
        dev_info(eta->dev, "sdp detected");
    } else if (eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_CDP) {
        eta->voltage_max = 5000000;
        eta->current_max = 1500000;
        eta6963_set_usbin_current_limit(eta, 1500);
        dev_info(eta->dev, "cdp detected");
    } else if (eta->vbus_online) {
        eta->voltage_max = 5000000;
        eta->current_max = 1000000;
        eta6963_set_usbin_current_limit(eta, 1000);
        dev_info(eta->dev, "unknown charge detected");
    }

    power_supply_changed(eta->charger_psy);
}

static void eta6963_power_off_work(
    struct work_struct *work)
{
	struct eta6963_device *eta = container_of(
            work, struct eta6963_device, poff_work.work);

	dev_info(eta->dev, "shutdown device now");
	kernel_power_off();
}

static int eta6963_charger_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int value;
    struct eta6963_device *eta = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_PARALLEL;
        dev_info(eta->dev, "charger psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (eta->vbus_online)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(eta->dev, "charger online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        val->intval = eta6963_charge_type[eta->state.chrg_stat];
        dev_info(eta->dev, "charger psy charge type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_STATUS:
        value = eta->state.chrg_stat;
        if (value == ETA6963_PRECHARGE || value == ETA6963_FAST_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
        else if (!eta->vbus_online)
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (eta->vbus_online && !eta->chg_en)
            val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if (value == ETA6963_TERM_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_FULL;
        else
            val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
        dev_info(eta->dev, "charger psy prop status: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
        val->intval = eta->state.vbus_stat;
        dev_info(eta->dev, "charger usb type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = eta->vbus_voltage;
        dev_info(eta->dev, "charger voltage: %d", val->intval);
        break;
    default:
        dev_err(eta->dev, "get charger prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static void eta6963_charger_power_changed(
	struct power_supply *psy)
{
	struct eta6963_device *eta = power_supply_get_drvdata(psy);

	dev_err(eta->dev, "external power changed");
	schedule_work(&eta->irq_work.work);
}

static struct power_supply_desc eta6963_charger_desc = {
    .name = "mtk_charger_type",
    .type = POWER_SUPPLY_TYPE_PARALLEL,
    .usb_types = eta6963_usb_type,
    .num_usb_types = ARRAY_SIZE(eta6963_usb_type),
    .properties = eta6963_charger_props,
    .num_properties = ARRAY_SIZE(eta6963_charger_props),
    .get_property = eta6963_charger_get_property,
    .external_power_changed = eta6963_charger_power_changed,
};

static char *eta6963_charger_supplied_to[] = { "battery" };

static struct power_supply *eta6963_register_charger_psy(
    struct eta6963_device *eta)
{
    struct power_supply_config psy_cfg = {
        .drv_data = eta,
        .of_node = eta->dev->of_node,
    };

    psy_cfg.supplied_to = eta6963_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(eta6963_charger_supplied_to);
    return power_supply_register(eta->dev, &eta6963_charger_desc, &psy_cfg);
}

static int eta6963_ac_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct eta6963_device *eta = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        if (eta->vbus_online &&
           (eta->state.vbus_stat != POWER_SUPPLY_USB_TYPE_SDP &&
            eta->state.vbus_stat != POWER_SUPPLY_USB_TYPE_CDP))
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(eta->dev, "ac online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = eta->current_max;
        dev_info(eta->dev, "ac current maxium: %d", eta->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = eta->voltage_max;
        dev_info(eta->dev, "ac voltage maxium: %d", eta->voltage_max);
        break;
    default:
        dev_err(eta->dev, "get ac prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply_desc eta6963_ac_desc = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .properties = eta6963_ac_props,
    .num_properties = ARRAY_SIZE(eta6963_ac_props),
    .get_property = eta6963_ac_get_property,
};

static struct power_supply *eta6963_register_ac_psy(
    struct eta6963_device *eta)
{
    struct power_supply_config psy_cfg = {
        .drv_data = eta,
        .of_node = eta->dev->of_node,
    };

    return power_supply_register(eta->dev, &eta6963_ac_desc, &psy_cfg);
}

static int eta6963_usb_writeable(
    struct power_supply *psy, enum power_supply_property psp)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        return 1;
    default:
        return 0;
    }
}

static int eta6963_usb_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct eta6963_device *eta = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_USB;
        dev_info(eta->dev, "usb psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        val->intval = eta->chg_en;
        dev_info(eta->dev, "usb psy prop charge enable: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_SDP ||
            eta->state.vbus_stat == POWER_SUPPLY_USB_TYPE_CDP)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(eta->dev, "usb online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = eta->current_max;
        dev_info(eta->dev, "usb current maxium: %d", eta->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = eta->voltage_max;
        dev_info(eta->dev, "usb voltage maxium: %d", eta->voltage_max);
        break;
    default:
        dev_err(eta->dev, "get usb prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static int eta6963_usb_set_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    const union power_supply_propval *val)
{
    struct eta6963_device *eta = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        eta6963_charge_enable_ctrl(eta, !!val->intval);
        schedule_delayed_work(&eta->psy_work, msecs_to_jiffies(100));
        dev_info(eta->dev, "set charge enable: %d", !!val->intval);
        return 0;
    default:
        return -EPERM;
    }
}

static struct power_supply_desc eta6963_usb_desc = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .properties = eta6963_usb_props,
    .num_properties = ARRAY_SIZE(eta6963_usb_props),
    .get_property = eta6963_usb_get_property,
    .set_property = eta6963_usb_set_property,
    .property_is_writeable = eta6963_usb_writeable,
};

static struct power_supply *eta6963_register_usb_psy(
    struct eta6963_device *eta)
{
    struct power_supply_config psy_cfg = {
        .drv_data = eta,
        .of_node = eta->dev->of_node,
    };

    return power_supply_register(eta->dev, &eta6963_usb_desc, &psy_cfg);
}

static int eta6963_enable_vbus(struct regulator_dev *rdev)
{
    struct eta6963_device *eta = rdev_get_drvdata(rdev);

    eta->otg_mode = 1;
    dev_info(eta->dev, "enable otg");
    return eta6963_update_bits(eta,
            ETA6963_CHRG_CTRL_1, ETA6963_OTG_EN, ETA6963_OTG_EN);
}

static int eta6963_disable_vbus(struct regulator_dev *rdev)
{
    struct eta6963_device *eta = rdev_get_drvdata(rdev);

    eta->otg_mode = 0;
    dev_info(eta->dev, "dibable otg");
    return eta6963_update_bits(eta,
        ETA6963_CHRG_CTRL_1, ETA6963_OTG_EN, 0);
}

static int eta6963_vbus_state(struct regulator_dev *rdev)
{
    u8 otg_en;
    struct eta6963_device *eta = rdev_get_drvdata(rdev);

    eta6963_read_reg(eta, ETA6963_CHRG_CTRL_1, &otg_en);
    otg_en = !!(otg_en & ETA6963_OTG_EN);
    dev_info(eta->dev, "otg state: %d", otg_en);

    return otg_en;
}

static struct regulator_ops eta6963_vbus_ops = {
    .enable = eta6963_enable_vbus,
    .disable = eta6963_disable_vbus,
    .is_enabled = eta6963_vbus_state,
};

static const struct regulator_desc eta6963_otg_rdesc = {
    .name = "eta6963-vbus",
    .of_match = "eta6963-otg-vbus",
    .ops = &eta6963_vbus_ops,
    .owner = THIS_MODULE,
    .type = REGULATOR_VOLTAGE,
};

static struct regulator_dev *eta6963_register_regulator(
    struct eta6963_device *eta)
{
    struct regulator_config config = {
        .dev = eta->dev,
        .driver_data = eta,
    };

    return regulator_register(&eta6963_otg_rdesc, &config);
}

static int eta6963_gpio_config(struct eta6963_device *eta)
{
    int ret, irqn;

    eta->en_gpio = of_get_named_gpio(eta->dev->of_node, "eta,chg-en-gpio", 0);
    if (!gpio_is_valid(eta->en_gpio)) {
        dev_err(eta->dev, "get eta,chg-en-gpio failed");
        return -EINVAL;
    }
    ret = gpio_request_one(eta->en_gpio, GPIOF_OUT_INIT_LOW, "eta_chg_en_pin");
    if (ret) {
        dev_err(eta->dev, "request eta,chg-en-gpio failed");
        return -EPERM;
    }

    eta->irq_gpio = of_get_named_gpio(eta->dev->of_node, "eta,irq-gpio", 0);
    if (!gpio_is_valid(eta->irq_gpio)) {
        dev_err(eta->dev, "get eta,irq-gpio failed\n");
        goto err_en_gpio;
    }
    irqn = gpio_to_irq(eta->irq_gpio);
    if (irqn > 0)
        eta->client->irq = irqn;
    else {
        dev_err(eta->dev, "map eta,irq-gpio to irq failed");
        goto err_en_gpio;
    }

    ret = gpio_request_one(eta->irq_gpio, GPIOF_DIR_IN, "eta6963_irq_pin");
    if (ret) {
        dev_err(eta->dev, "request eta,irq-gpio failed\n");
        goto err_en_gpio;
    }

	eta->id_gpio = of_get_named_gpio(eta->dev->of_node, "id-gpio", 0);
    if (!gpio_is_valid(eta->id_gpio)) {
        dev_err(eta->dev, "get id-gpio failed");
        goto err_id_gpio;
    }

    return 0;

err_id_gpio:
	gpio_free(eta->irq_gpio);
err_en_gpio:
    gpio_free(eta->en_gpio);
    return -EINVAL;
}

static ssize_t eta6963_reg_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    int i, ret;
    char *reg_val[2];
    ulong addr, val;
    struct eta6963_device *eta = dev_get_drvdata(dev);

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
        ret = i2c_smbus_write_byte_data(eta->client, addr, val);
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
        eta->reg_addr = val;
    }

    return size;
}

static struct device_attribute dev_attr_reg =
    __ATTR(reg, S_IWUSR, NULL, eta6963_reg_store);

static ssize_t eta6963_reg_dump(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int i, ret;
    ssize_t len = 0;
    struct eta6963_device *eta = dev_get_drvdata(dev);
    u8 reg_index[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 
    };

    for (i = 0; i < ARRAY_SIZE(reg_index); i++) {
        ret = i2c_smbus_read_byte_data(eta->client, reg_index[i]);
        if (ret < 0) {
            dev_err(dev, "read reg [%#x] failed\n", eta->reg_addr);
            return -EINVAL;
        }
        len += snprintf(buf + len, PAGE_SIZE, "%#04x: %#04x\n", reg_index[i], ret);
    }

    return len;
}

static struct device_attribute dev_attr_dump =
    __ATTR(dump, S_IRUSR, eta6963_reg_dump, NULL);

static ssize_t eta6963_vtemp_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct eta6963_device *eta = dev_get_drvdata(dev);

    sscanf(buf, "%d\n", &eta->vtemp);
    eta6963_vtemp = eta->vtemp;
    schedule_delayed_work(&eta->psy_work, msecs_to_jiffies(100));
    dev_info(dev, "virtual temperature %d", eta->vtemp);

    return size;
}

static struct device_attribute dev_attr_vtemp =
    __ATTR(vtemp, S_IWUSR, NULL, eta6963_vtemp_store);

static int eta6963_create_sysfs_file(
    struct eta6963_device *eta)
{
    int ret;

    ret = device_create_file(eta->dev, &dev_attr_reg);
    if (ret) {
        dev_err(eta->dev, "device_create_file reg failed\n");
        goto err_create_reg;
    }

    ret = device_create_file(eta->dev, &dev_attr_dump);
    if (ret) {
        dev_err(eta->dev, "device_create_file dump failed\n");
        goto err_create_dump;
    }

    ret = device_create_file(eta->dev, &dev_attr_vtemp);
    if (ret) {
        dev_err(eta->dev, "device_create_file vtemp failed\n");
        goto err_create_vtemp;
    }

    return 0;

err_create_vtemp:
    device_remove_file(eta->dev, &dev_attr_dump);
err_create_dump:
    device_remove_file(eta->dev, &dev_attr_reg);
err_create_reg:
    return -EPERM;
}

static int eta6963_driver_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    struct eta6963_device *eta;

    eta = devm_kzalloc(&client->dev, sizeof(*eta), GFP_KERNEL);
    if (!eta) {
        dev_err(&client->dev, "allocate memory for eta6963 failed");
        return -ENOMEM;
    }

    eta->vbus = devm_iio_channel_get(&client->dev, "pmic_vbus");
    if (IS_ERR_OR_NULL(eta->vbus)) {
        dev_err(&client->dev, "get vbus failed\n");
        devm_kfree(&client->dev, eta);
        return -EPROBE_DEFER;
    }

    eta->aca_psy = power_supply_get_by_name("mtk_charger_aca");
    if (!eta->aca_psy) {
        dev_err(&client->dev, "get mtk_charger_aca psy failed");
        devm_iio_channel_release(&client->dev, eta->vbus);
        devm_kfree(&client->dev, eta);
        return -EPROBE_DEFER;
    }

    eta->chg_en = 1;
    eta->ibat = 2400;
    eta->vbus_online = 0;
    eta->vtemp = ETA6963_TEMP_INVALID;
    eta->temp_state = ETA6963_STATE_NORMAL;
    eta->client = client;
    eta->dev = &client->dev;

    ret = eta6963_chipid_detect(eta);
    if (ret != ETA6963_PN_ID) {
        dev_err(&client->dev, "detect eta6963 failed");
        devm_iio_channel_release(&client->dev, eta->vbus);
        devm_kfree(&client->dev, eta);
        return -ENODEV;
    }

    i2c_set_clientdata(client, eta);
    //device_init_wakeup(eta->dev, true);
    INIT_DELAYED_WORK(&eta->irq_work, eta6963_charge_irq_work);
    INIT_DELAYED_WORK(&eta->psy_work, eta6963_psy_changed_work);
    INIT_DELAYED_WORK(&eta->charge_work, eta6963_charge_online_work);
    INIT_DELAYED_WORK(&eta->curr_work, eta6963_charge_current_work);
	INIT_DELAYED_WORK(&eta->poff_work, eta6963_power_off_work);

    ret = eta6963_gpio_config(eta);
    if (ret < 0) {
        dev_err(eta->dev, "config gpio failed");
        goto err_gpio_config;
    }

    ret = eta6963_hardware_init(eta);
    if (ret) {
        dev_err(eta->dev, "hardware init failed");
        goto err_hardware_init;
    }

    ret = devm_request_threaded_irq(eta->dev, client->irq,
                NULL, eta6963_irq_handler_thread,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                dev_name(eta->dev), eta);
    if (ret) {
        dev_err(eta->dev, "request interrupt failed");
        goto err_hardware_init;
    }
    ret = enable_irq_wake(client->irq);
    if (ret) {
        dev_err(eta->dev, "enable irq wake failed");
        goto err_irq_wake;
    }

    eta->charger_psy = eta6963_register_charger_psy(eta);
    if (IS_ERR_OR_NULL(eta->charger_psy)) {
        dev_err(eta->dev, "register charger power supply failed");
        goto err_register_charger_psy;
    }

    eta->usb_psy = eta6963_register_usb_psy(eta);
    if (IS_ERR_OR_NULL(eta->usb_psy)) {
        dev_err(eta->dev, "register usb power supply failed");
        goto err_register_usb_psy;
    }

    eta->ac_psy = eta6963_register_ac_psy(eta);
    if (IS_ERR_OR_NULL(eta->ac_psy)) {
        dev_err(eta->dev, "register ac power supply failed");
        goto err_register_ac_psy;
    }

    eta->otg_rdev = eta6963_register_regulator(eta);
    if (IS_ERR_OR_NULL(eta->otg_rdev)) {
        dev_err(eta->dev, "register regulator failed");
        goto err_register_rdev;
    }
    
    ret = eta6963_get_initial_state(eta);
    if (ret) {
        dev_err(eta->dev, "get initial state failed");
        goto err_get_state;
    }

    ret = eta6963_create_sysfs_file(eta);
    if (ret) {
        dev_err(eta->dev, "create sysfs failed\n");
        goto err_get_state;
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    scc_init_speed_current_map("mediatek,mt6357-gauge");
    scc_create_file(eta->dev);
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    dev_info(eta->dev, "eta6963_driver_probe finished");

    return 0;

err_get_state:
    regulator_unregister(eta->otg_rdev);
err_register_rdev:
    power_supply_unregister(eta->ac_psy);
err_register_ac_psy:
    power_supply_unregister(eta->usb_psy);
err_register_usb_psy:
    power_supply_unregister(eta->charger_psy);
err_register_charger_psy:
    disable_irq_wake(client->irq);
err_irq_wake:
    free_irq(client->irq, eta);
err_hardware_init:
    gpio_free(eta->irq_gpio);
    gpio_free(eta->en_gpio);
err_gpio_config:
    devm_iio_channel_release(eta->dev, eta->vbus);
    devm_kfree(eta->dev, eta);
    return -EBUSY;
}

static void eta6963_remove_sysfs_file(
    struct eta6963_device *eta)
{
    device_remove_file(eta->dev, &dev_attr_vtemp);
    device_remove_file(eta->dev, &dev_attr_dump);
    device_remove_file(eta->dev, &dev_attr_reg);
}

static int eta6963_charger_remove(struct i2c_client *client)
{
    struct eta6963_device *eta = i2c_get_clientdata(client);

    eta6963_remove_sysfs_file(eta);
    regulator_unregister(eta->otg_rdev);
    power_supply_unregister(eta->charger_psy);
    power_supply_unregister(eta->usb_psy);
    power_supply_unregister(eta->ac_psy);
    disable_irq_wake(client->irq);
    free_irq(client->irq, eta);
    gpio_free(eta->irq_gpio);
    gpio_free(eta->en_gpio);
    devm_iio_channel_release(eta->dev, eta->vbus);
    devm_kfree(eta->dev, eta);

    return 0;
}

static int eta6963_device_suspend(struct device *dev)
{
    struct eta6963_device *eta = dev_get_drvdata(dev);

    if (eta->vbus_online) {
        cancel_delayed_work(&eta->charge_work);
        dev_info(eta->dev, "cancel charge work");
    }

    return 0;
}

static int eta6963_device_resume(struct device *dev)
{
    struct eta6963_device *eta = dev_get_drvdata(dev);

    if (eta->vbus_online) {
        schedule_delayed_work(&eta->charge_work, msecs_to_jiffies(100));
        dev_info(eta->dev, "resume charge work");
    }

    return 0;
}

static struct dev_pm_ops eta6963_pm_osp = {
    .suspend = eta6963_device_suspend,
    .resume = eta6963_device_resume,
};

static const struct i2c_device_id eta6963_i2c_ids[] = {
    { "eta6963", 0 }, { },
};

static const struct of_device_id eta6963_of_match[] = {
    { .compatible = "eta,eta6963", }, { },
};

static struct i2c_driver eta6963_driver = {
    .driver = {
        .name = "eta6963-charger",
        .of_match_table = eta6963_of_match,
        .pm = &eta6963_pm_osp,
    },
    .probe = eta6963_driver_probe,
    .remove = eta6963_charger_remove,
    .id_table = eta6963_i2c_ids,
};

static int eta6963_init(void)
{
    return i2c_add_driver(&eta6963_driver);
}

static void eta6963_exit(void)
{
    i2c_del_driver(&eta6963_driver);
}

module_init(eta6963_init);
module_exit(eta6963_exit);
MODULE_LICENSE("GPL v2");

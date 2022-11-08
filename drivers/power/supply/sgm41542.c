#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/string.h>
#include "sgm41542.h"
#include "charger_class.h"
#if defined (CONFIG_TINNO_SCC_SUPPORT)
#include <linux/scc_drv.h>
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

#undef dev_fmt
#define dev_fmt(fmt) "202204061700: " fmt

extern int current_max_sgm7220;
extern int current_max_wusb3801;

int sgm41542_vtemp = 250;
EXPORT_SYMBOL(sgm41542_vtemp);

extern int demomode_enable;
extern int demomode_max_soc;
extern int demomode_min_soc;

static char *sgm41542_charge_state[] = {
    "charge-disable",
    "pre-charge",
    "fast-charge",
    "charge-terminated",
};

static enum power_supply_usb_type sgm41542_usb_type[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_CDP,
    POWER_SUPPLY_USB_TYPE_DCP,
    POWER_SUPPLY_USB_TYPE_UNKNOWN,
};

static enum power_supply_usb_type sgm41542_charge_type[] = {
    POWER_SUPPLY_CHARGE_TYPE_NONE,
    POWER_SUPPLY_CHARGE_TYPE_TRICKLE,
    POWER_SUPPLY_CHARGE_TYPE_FAST,
    POWER_SUPPLY_CHARGE_TYPE_TAPER,
};

static enum power_supply_property sgm41542_charger_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_USB_TYPE,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
};

static enum power_supply_property sgm41542_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static enum power_supply_property sgm41542_usb_props[] = {
    POWER_SUPPLY_PROP_TYPE,
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CHARGE_ENABLED,
};

static int sgm41542_read_reg(
    struct sgm41542_device *sgm, u8 reg, u8 *data)
{
    int ret;

    ret = i2c_smbus_read_byte_data(sgm->client, reg);
    if (ret < 0) {
        dev_err(sgm->dev, "read reg: %#x failed", reg);
        return -EINVAL;
    } else
        *data = ret;

    return 0;
}

static int sgm41542_write_reg(
    struct sgm41542_device *sgm, u8 reg, u8 value)
{
    int ret;

    ret = i2c_smbus_write_byte_data(sgm->client, reg, value);
    if (ret < 0) {
        dev_err(sgm->dev, "write reg: %#x failed", reg);
        return -EINVAL;
    }

    return 0;
}

static int sgm41542_update_bits(
    struct sgm41542_device *sgm, u8 reg, u8 mask, u8 val)
{
    u8  tmp;
    int ret;

    ret = sgm41542_read_reg(sgm, reg, &tmp);
    if (ret)
        return ret;

    tmp &= ~mask;
    tmp |= val & mask;

    ret = sgm41542_write_reg(sgm, reg, tmp);
    if (ret)
        return ret;

    return 0;
}

static inline int sgm41542_reset_register(
    struct sgm41542_device *sgm)
{
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_B,
        SGM41542_REG_RESET, SGM41542_REG_RESET);
}

static int sgm41542_disable_watchdog_timer(
    struct sgm41542_device *sgm)
{
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_5,
        SGM41542_WDT_TIMER_MASK, SGM41542_WDT_TIMER_DISABLE);
}

static int sgm41542_disable_dpm_irq(
    struct sgm41542_device *sgm)
{
    return sgm41542_update_bits(sgm,
        SGM41542_CHRG_CTRL_A, SGM41542_DPM_INT_MASK,
        SGM41542_IINDPM_INT_MASK | SGM41542_VINDPM_INT_MASK);
}

static void sgm41542_charge_enable_ctrl(
    struct sgm41542_device *sgm, int en)
{   
    sgm->chg_en = !!en;
    sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_1,
        SGM41542_CHRG_EN, en ? SGM41542_CHRG_EN : 0);
}

static inline int sgm41542_get_charge_enable_status(
    struct sgm41542_device *sgm)
{
    u8 status;

    sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_1, &status);

    return !!(status & SGM41542_CHRG_EN);
}

static void sgm41542_set_usbin_current_limit(
    struct sgm41542_device *sgm , unsigned int iindpm)
{
    u8 reg_val;

    if (iindpm > 3100)
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_0,
            SGM41542_IINDPM_I_MASK, SGM41542_IINDPM_I_MASK);
    else {
        reg_val = (iindpm - SGM41542_IINDPM_I_MIN_MA)
            / SGM41542_IINDPM_STEP_MA;
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_0,
            SGM41542_IINDPM_I_MASK, reg_val);
    }
}

#if defined (CONFIG_TINNO_SCC_SUPPORT)
static int sgm41542_get_cc_ibat_value(
    struct sgm41542_device *sgm)
{
    int ret;
    int ichg;
    u8 reg_val;

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_2, &reg_val);
    if (ret < 0)
        ichg = -EINVAL;
    else
        ichg = reg_val & SGM41542_ICHRG_CUR_MASK;

    return ichg;
}
#endif /* CONFIG_TINNO_SCC_SUPPORT */

static int sgm41542_set_fast_charge_current(
    struct sgm41542_device *sgm, unsigned int chrg_curr)
{
    u8 reg_val;

    reg_val = chrg_curr / SGM41542_ICHRG_CURRENT_STEP_MA;
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_2,
        SGM41542_ICHRG_CUR_MASK, reg_val);
}

static int sgm41542_set_taper_current(
    struct sgm41542_device *sgm, int term_current)
{
    u8 reg_val;

    reg_val = (term_current - SGM41542_TERMCHRG_I_MIN_MA)
                / SGM41542_TERMCHRG_CURRENT_STEP_MA;
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_3,
        SGM41542_TERMCHRG_CUR_MASK, reg_val);
}

static int sgm41542_set_prechg_current(
    struct sgm41542_device *sgm, int prechg_current)
{
    u8 reg_val;

    reg_val = (prechg_current - SGM41542_PRECHG_I_MIN_MA)
                / SGM41542_PRECHG_CURRENT_STEP_MA;
    reg_val = reg_val << 4;
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_3,
        SGM41542_PRECHG_CUR_MASK, reg_val);
}

static int sgm41542_set_battery_voltage(
    struct sgm41542_device *sgm, unsigned int chrg_volt)
{
    u8 reg_val;

    reg_val = (chrg_volt - SGM41542_VREG_V_MIN_MV)
                / SGM41542_VREG_V_STEP_MV;
    reg_val = reg_val << 3;
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_4,
        SGM41542_VREG_V_MASK, reg_val);
}

static int sgm41542_vreg_fine_tune(
    struct sgm41542_device *sgm, int tune_mv)
{
    u8 reg_val;

    if (tune_mv == 8)
        reg_val = SGM41542_VREG_FINE_TUNE_PLUS_8MV;
    else if (tune_mv == -8)
        reg_val = SGM41542_VREG_FINE_TUNE_MINUS_8MV;
    else if (tune_mv == -16)
        reg_val = SGM41542_VREG_FINE_TUNE_MINUS_16MV;
    else
        reg_val = SGM41542_VREG_FINE_TUNE_DISABLE;

    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_F,
        SGM41542_VREG_FINE_TUNE_MASK, reg_val);
    //return 0;
}

static int sgm41542_recharge_voltage(
    struct sgm41542_device *sgm, unsigned int rechg_mv)
{
    u8 reg_val;

    if (rechg_mv == 100)
        reg_val = SGM41542_RECHARGE_VOLTAGE_100MV;
    else
        reg_val = SGM41542_RECHARGE_VOLTAGE_200MV;

    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_4,
        SGM41542_RECHARGE_VOLTAGE_MASK, reg_val);
}

static int sgm41542_vindpm_track_set(
    struct sgm41542_device *sgm, unsigned int vindpm_mv)
{
    u8 reg_val;

    if (vindpm_mv == 200)
        reg_val = SGM41542_VINDPM_TRACK_SET_200MV;
    else if (vindpm_mv == 250)
        reg_val = SGM41542_VINDPM_TRACK_SET_250MV;
    else if (vindpm_mv == 300)
        reg_val = SGM41542_VINDPM_TRACK_SET_300MV;
    else
        reg_val = SGM41542_VINDPM_TRACK_SET_DISABLE;

    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_7,
        SGM41542_VINDPM_TRACK_SET_MASK, reg_val);
}

static int sgm41542_set_boost_current(
    struct sgm41542_device *sgm, int boost_current)
{
    u8 reg_val;

    if (boost_current >= 2000)
        reg_val = SGM41542_BOOST_CUR_2A;
    else
        reg_val = SGM41542_BOOST_CUR_1A2;

    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_2,
        SGM41542_BOOST_CUR_2A, reg_val);
}

static int sgm41542_chipid_detect(struct sgm41542_device *sgm)
{
    int ret = 0;
    u8 val = 0;

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_B, &val);
    if (ret < 0) {
        dev_err(sgm->dev, "read SGM41542_CHRG_CTRL_B failed");
        sgm->client->addr = 0x0b;
        ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_B, &val);
        if (ret < 0) {
            dev_err(sgm->dev, "read SGM41543_CHRG_CTRL_B failed");
            return -EINVAL;
        }
    }

    val = val & SGM41542_PN_MASK;
    return val;
}

static int sgm41542_hardware_init(struct sgm41542_device *sgm)
{
    int ret;

    ret = sgm41542_set_taper_current(sgm, 300);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 taper current failed");
        return -EPERM;
    }

    ret = sgm41542_set_prechg_current(sgm, 360);
   if (ret) {
       dev_err(sgm->dev, "set sgm41542 precharge current failed");
       return -EPERM;
   }

    ret = sgm41542_set_battery_voltage(sgm, 4464);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 charge voltage failed");
        return -EPERM;
    }

    ret = sgm41542_vreg_fine_tune(sgm, -16);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 vreg fine tune 16mv failed");
        return -EPERM;
    }

    ret = sgm41542_recharge_voltage(sgm, 200);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 recharge voltage failed");
        return -EPERM;
    }

    ret = sgm41542_vindpm_track_set(sgm, 200);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 vindpm track failed");
        return -EPERM;
    }

    ret = sgm41542_set_fast_charge_current(sgm, 3000);
    if (ret) {
        dev_err(sgm->dev, "set sgm41542 fast charge current failed");
        return -EPERM;
    }

    ret = sgm41542_set_boost_current(sgm, 1200);
    if (ret) {
            dev_err(sgm->dev, "set sgm41542 boost current failed");
            return -EPERM;
    }

    ret = sgm41542_disable_watchdog_timer(sgm);
    if (ret) {
        dev_err(sgm->dev, "disable sgm41542 watchdog timer failed");
        return -EPERM;
    }

    ret = sgm41542_disable_dpm_irq(sgm);
    if (ret) {
        dev_err(sgm->dev, "disable sgm41542 iindpm irq failed");
        return -EPERM;
    }

    return 0;
}

static int sgm41542_get_charge_state(
    struct sgm41542_device *sgm)
{
    int ret;
    u8 status;

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_8, &status);
    if (ret) {
        dev_err(sgm->dev, "get sgm41542 status register failed");
        return -EINVAL;
    }

    return status;
}

static int sgm41542_get_status(
    struct sgm41542_device *sgm, struct sgm41542_state *state)
{
    int ret;
    u8 status, fault, ctrl;

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_8, &status);
    if (ret) {
        dev_err(sgm->dev, "read sgm41542 0x08 register failed");
        return -EINVAL;
    }
    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_9, &fault);
    if (ret) {
        dev_err(sgm->dev, "read sgm41542 0x09 register failed");
        return -EINVAL;
    }
    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_A, &ctrl);
    if (ret) {
        dev_err(sgm->dev, "read sgm41542 0x0A register failed");
        return -EINVAL;
    }

    state->pg_stat = (status & SGM41542_PG_GOOD_MASK) >> 2;
    state->chrg_stat = (status & SGM41542_CHG_STAT_MASK) >> 3;
    state->vbus_stat = (status & SGM41542_VBUS_STAT_MASK) >> 5;

    dev_info(sgm->dev, "[08]: %#x, [09]: %#x, [0A]: %#x",
        status, fault, ctrl);

    return 0;
}

static int sgm41542_get_initial_state(
    struct sgm41542_device *sgm)
{
    int chg_state;

    chg_state = sgm41542_get_charge_state(sgm);
    if (chg_state < 0) {
        dev_err(sgm->dev, "get charger state failed\n");
        return -EINVAL;
    }

    if (chg_state & SGM41542_PG_GOOD_MASK) {
        sgm->voltage_max = 5000000;
        sgm->current_max = 1000000;
        sgm->state.chrg_stat = SGM41542_FAST_CHARGE;
        sgm->state.pg_stat = (chg_state & SGM41542_PG_GOOD_MASK) >> 2;
        sgm->state.vbus_stat = (chg_state & SGM41542_VBUS_STAT_MASK) >> 5;
        // release dp dm
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D, SGM41542_DPM_VSEL_MASK,
                             SGM41542_DP_HIZ | SGM41542_DM_HIZ);
        schedule_delayed_work(&sgm->irq_work, msecs_to_jiffies(1000));
        schedule_delayed_work(&sgm->charge_work, msecs_to_jiffies(30000));
        dev_info(sgm->dev, "charger online during power up\n");
    }

    return 0;
}

static int sgm41542_get_battery_temperature(
    struct sgm41542_device *sgm)
{
    int ret;
    int charge_state;
    union power_supply_propval psy_prop;

    ret = power_supply_get_property(sgm->battery_psy,
            POWER_SUPPLY_PROP_TEMP, &psy_prop);
    if (ret) {
        dev_err(sgm->dev, "get battery temp failed");
        return -ENODEV;
    }

    // set virtual temperature
    if (sgm->vtemp != SGM41542_TEMP_INVALID) {
        psy_prop.intval = sgm->vtemp;
        dev_info(sgm->dev, "set vtemp: %d", sgm->vtemp);
    }

    sgm->temp = psy_prop.intval;
    dev_info(sgm->dev, "battery temp: %d", sgm->temp);

    if (sgm->temp <= -170 || sgm->temp > 570)
        charge_state = SGM41542_CHG_DISABLE;
    else if (sgm->temp > -170 && sgm->temp < 0)
        charge_state = SGM41542_STATE_COLD;
    else if (sgm->temp >= 0 && sgm->temp < 150)
        charge_state = SGM41542_STATE_COOL;
    else if (sgm->temp > 150 && sgm->temp <= 450)
        charge_state = SGM41542_STATE_NORMAL;
    else if (sgm->temp > 450 && sgm->temp <= 570)
        charge_state = SGM41542_STATE_WARM;

    return charge_state;
}

static void sgm41542_set_charge_voltage_current(
    struct sgm41542_device *sgm)
{
    switch (sgm->temp_state) {
    case SGM41542_STATE_COLD:
        dev_info(sgm->dev, "cold state");
        sgm->ibat = 960;
        sgm41542_set_battery_voltage(sgm, 4208);
        sgm41542_vreg_fine_tune(sgm, -8);
        break;
    case SGM41542_STATE_COOL:
        dev_info(sgm->dev, "cool state");
        sgm41542_set_battery_voltage(sgm, 4464);
        sgm41542_vreg_fine_tune(sgm, -16);
        break;
    case SGM41542_STATE_NORMAL:
        dev_info(sgm->dev, "normal state");
        sgm->ibat = 3000;
        sgm41542_set_battery_voltage(sgm, 4464);
        sgm41542_vreg_fine_tune(sgm, -16);
        break;
    case SGM41542_STATE_WARM:
        dev_info(sgm->dev, "warm state");
        sgm->ibat = 1980;
        sgm41542_set_battery_voltage(sgm, 4208);
        sgm41542_vreg_fine_tune(sgm, -8);
        break;
    case SGM41542_CHG_DISABLE:
        dev_info(sgm->dev, "charge disabled state");
        break;
    default:
        dev_err(sgm->dev, "invalid temp state: %d", sgm->temp_state);
        break;
    }
}

static inline int sgm41542_set_cvcharge_5v(
    struct sgm41542_device *sgm)
{
    int ret;
    union power_supply_propval psy_prop;

    ret = power_supply_get_property(sgm->battery_psy,
            POWER_SUPPLY_PROP_CURRENT_NOW, &psy_prop);
    if (ret) {
        dev_err(sgm->dev, "get battery charge current failed");
        return -ENODEV;
    }

    // dpdm should only be controlled in dcp
    // dpdm should be HIZ state in sdp/cdp
    if (sgm->state.vbus_stat == SGM41542_USB_DCP) {
        if (psy_prop.intval > 0 && psy_prop.intval < 1500000) {
            // enhable 5V
            dev_info(sgm->dev, "enable dp dm for dcp 5v");
            sgm->qc_type = QUICK_CHARGE_1P0;
            sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
                SGM41542_DP_VSEL_MASK | SGM41542_DM_VSEL_MASK,
                SGM41542_DP_0V6 | SGM41542_DM_HIZ);
        }
    }

    return 0;
}

static inline void sgm41542_get_thermal_temp(
    struct sgm41542_device *sgm)
{
    int temp;
    sgm->tz_ap = thermal_zone_get_zone_by_name("mtktsAP");
    if (!IS_ERR(sgm->tz_ap) && sgm->tz_ap->ops &&
             sgm->tz_ap->ops->get_temp) {
        sgm->tz_ap->ops->get_temp(sgm->tz_ap, &temp);
        dev_info(sgm->dev, "mtktsAP temp: %d", temp / 1000);
    }

    sgm->tz_rf = thermal_zone_get_zone_by_name("mtktsbtsmdpa");
    if (!IS_ERR(sgm->tz_rf) && sgm->tz_rf->ops &&
             sgm->tz_rf->ops->get_temp) {
        sgm->tz_rf->ops->get_temp(sgm->tz_rf, &temp);
        dev_info(sgm->dev, "mtktsbtsmdpa temp: %d", temp / 1000);
    }

    sgm->tz_chg = thermal_zone_get_zone_by_name("mtktscharger");
    if (!IS_ERR(sgm->tz_chg) && sgm->tz_chg->ops &&
             sgm->tz_chg->ops->get_temp) {
        sgm->tz_chg->ops->get_temp(sgm->tz_chg, &temp);
        dev_info(sgm->dev, "mtktscharger temp: %d", temp / 1000);
    }

    //lizhihua-20220122-add
    sgm->tz_batt = thermal_zone_get_zone_by_name("mtktsbattery");
    if (!IS_ERR(sgm->tz_batt) && sgm->tz_batt->ops &&
        sgm->tz_batt->ops->get_temp) {
        sgm->tz_batt->ops->get_temp(sgm->tz_batt, &temp);
        dev_info(sgm->dev, "mtktsbattery temp: %d", temp / 1000);
    }
    //end
}

static void sgm41542_charge_online_work(
    struct work_struct *work)
{
    int ret, err = 0;
    int new_temp;
    int chg_state;
    struct device_node *firmware;
    static int is_p412 = 0;
#if defined (CONFIG_TINNO_SCC_SUPPORT)
    int ichg_o, ichg_n, scc_value;
#endif  /* CONFIG_TINNO_SCC_SUPPORT */
	union power_supply_propval psy_prop;

    struct sgm41542_device *sgm = container_of(
        work, struct sgm41542_device, charge_work.work);

    if (!sgm->battery_psy) {
        sgm->battery_psy = power_supply_get_by_name("battery");
        if (!sgm->battery_psy) {
            err = -ENODEV;
            dev_err(sgm->dev, "get battery power supply failed");
            goto out;
        }
    }

    sgm41542_get_thermal_temp(sgm);

    /*if (sgm->qc_type != QUICK_CHARGE_1P0) {
        ret = sgm41542_set_cvcharge_5v(sgm);
        if (ret) {
            err = -EPERM;
            dev_err(sgm->dev, "set constant charge voltage to 5v failed");
            goto out;
        }
    }*/

    new_temp = sgm41542_get_battery_temperature(sgm);
    if (new_temp < 0) {
        err = -ENODEV;
        dev_err(sgm->dev, "get charge state failed");
        goto out;
    }
    if (sgm->temp_state != new_temp) {
        sgm->temp_state = new_temp;
        dev_info(sgm->dev, "temp status changed");
        if (sgm->chg_en && sgm->temp_state == SGM41542_CHG_DISABLE) {
            sgm41542_charge_enable_ctrl(sgm, 0);
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev, "disable charge, temperature: %d", sgm->temp);
        } else if (!sgm->chg_en) {
            sgm41542_charge_enable_ctrl(sgm, 1);
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev, "enable charge, temperature: %d", sgm->temp);
        }
        sgm41542_set_charge_voltage_current(sgm);
    }
    if (sgm->temp_state == SGM41542_STATE_COOL) {
        ret = power_supply_get_property(sgm->battery_psy,
                POWER_SUPPLY_PROP_VOLTAGE_NOW, &psy_prop);
        if (ret) {
            dev_err(sgm->dev, "get battery voltage failed");
            return;
        }
        if (psy_prop.intval < 4200000) {
            sgm->ibat = 1980;
            sgm41542_set_fast_charge_current(sgm, sgm->ibat);
            dev_info(sgm->dev,
                "battery voltage: %d, set charge current to 1980",
                psy_prop.intval);
        } else {
            sgm->ibat = 960;
            sgm41542_set_fast_charge_current(sgm, sgm->ibat);
            dev_info(sgm->dev,
                "battery voltage: %d, set charge current to 960",
                psy_prop.intval);
        }
    }
    if (sgm->temp <= -100 || sgm->temp >= 550) {
        power_supply_changed(sgm->charger_psy);
        dev_info(sgm->dev, "device shutdown, temperature: %d", sgm->temp);  
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    if (sgm->state.vbus_stat == SGM41542_USB_DCP) {
        scc_value = scc_get_current();
        scc_value /= 1000;
        if (scc_value > sgm->ibat)
            scc_value = sgm->ibat;
        dev_info(sgm->dev, "scc current = %d, jeita current: %d",
            scc_value, sgm->ibat);
        ichg_o = sgm41542_get_cc_ibat_value(sgm);
        if (ichg_o < 0) {
            err = -EINVAL;
            dev_err(sgm->dev, "get cc ibat value failed");
            goto out;
        }
        ichg_n = (scc_value / 60) & SGM41542_ICHRG_CUR_MASK;
        if (ichg_n != ichg_o) {
            sgm41542_set_fast_charge_current(sgm, scc_value);
            dev_info(sgm->dev, "scc set cc current = %d", scc_value);
        }
    }
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    if (demomode_enable) {
        if (!is_p412){
            firmware = of_find_node_by_name(NULL, "android");
            if (firmware) {
                const char *sku = of_get_property(firmware, "software.sku", NULL);
                of_node_put(firmware);

                if (!strcmp(sku, "RF2-SKU10")
                    || !strcmp(sku, "RF2-SKU11")) {
                    is_p412 = 1;
                   demomode_min_soc = 40;
          	   demomode_max_soc = 70;
	  	}
                pr_info("software.sku=%s is_p412=%d\n", sku, is_p412);
            }
        }
	ret = power_supply_get_property(sgm->battery_psy,
                POWER_SUPPLY_PROP_CAPACITY, &psy_prop);
        if (ret) {
            err = -EINVAL;
            dev_err(sgm->dev, "get battery capacity failed");
            goto out;
        }
	if (psy_prop.intval >= demomode_max_soc && sgm->chg_en) {
            sgm41542_charge_enable_ctrl(sgm, 0);
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev, "disable charge due to max_soc");
        } else if (psy_prop.intval <= demomode_min_soc && !sgm->chg_en) {
            sgm41542_charge_enable_ctrl(sgm, 1);
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev, "enable charge due to min_soc");
        }
    }

    ret = sgm41542_get_charge_state(sgm);
    if (ret < 0) {
        err = -EINVAL;
        dev_err(sgm->dev, "get charge state failed");
        goto out;
    }
    chg_state = (ret & SGM41542_CHG_STAT_MASK) >> 3;
    if (sgm->state.chrg_stat != chg_state) {
        sgm->state.chrg_stat = chg_state;
        dev_info(sgm->dev, "charge state: %s",
            sgm41542_charge_state[chg_state]);
        power_supply_changed(sgm->charger_psy);
    }

out:
    if (err || !sgm->state.pg_stat)
        cancel_delayed_work(&sgm->charge_work);
    else
        schedule_delayed_work(&sgm->charge_work, msecs_to_jiffies(1000));
}

static void sgm41542_request_qc3p0(struct sgm41542_device *sgm)
{
    int i, ret, value, vbus_volt_old, vbus_volt_new, time_out = 0;

    ret = iio_read_channel_processed(sgm->vbus, &value);
    if (ret < 0) {
        dev_err(sgm->dev, "get vbus voltage failed before qc3.0 check");
        return;
    } else {
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        vbus_volt_old = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
    }

    // qc3.0 commubication
    dev_info(sgm->dev, "hvdcp qc3.0 checking");
    // enter continuous mode
    sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
        SGM41542_DP_VSEL_MASK | SGM41542_DM_VSEL_MASK,
        SGM41542_DP_0V6 | SGM41542_DM_3V3);
    msleep(100);
    for (i = 0; i < 20; i++) {
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DM_VSEL_MASK, SGM41542_DM_0V6);
        usleep_range(100, 100);
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DM_VSEL_MASK, SGM41542_DM_3V3);
        // check vbus voltage until vbus stable
        while (time_out < 100) {
            msleep(10);
            ret = iio_read_channel_processed(sgm->vbus, &value);
            if (ret < 0) {
                dev_err(sgm->dev, "check qc3.0 vbus failed, try count: %d", i);
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
            dev_info(sgm->dev, "qc3.0 not supported");
            return;
        }
        if (vbus_volt_new < sgm->chg_volt) {
            sgm->qc_type = QUICK_CHARGE_3P0;
            sgm->voltage_max = 5600000;
            sgm->current_max = 3000000;
            sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
                SGM41542_DP_VSEL_MASK, SGM41542_DP_3V3);
            usleep_range(100, 100);
            sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
                SGM41542_DP_VSEL_MASK, SGM41542_DP_0V6);
            sgm41542_set_usbin_current_limit(sgm, sgm->chg_curr);
            msleep(100);
            ret = iio_read_channel_processed(sgm->vbus, &value);
            if (ret < 0) {
                dev_err(sgm->dev, "get vbus voltage failed after increase vbus voltage");
                return;
            }
            vbus_volt_new = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev,
                "hvdcp qc3.0 detected, vbus voltage: %d", vbus_volt_new);
            break;
        }
    }
}

static void sgm41542_request_qc2p0(struct sgm41542_device *sgm)
{
    int ret, value, vbus_volt, time_out = 0;

    // qc2.0 commubication
    dev_info(sgm->dev, "hvdcp qc2.0 checking");
    sgm41542_set_usbin_current_limit(sgm, 2000);
    sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
        SGM41542_DP_VSEL_MASK | SGM41542_DM_VSEL_MASK,
        SGM41542_DP_3V3 | SGM41542_DM_0V6);
    // set qc2.0 timeout to 3s, wait vbus stable
    while (time_out < 30) {
        time_out++;
        msleep(100);
        ret = iio_read_channel_processed(sgm->vbus, &value);
        if (ret < 0) {
            dev_err(sgm->dev, "get vbus voltage failed when check qc2.0");
            return;
        }
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        vbus_volt = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
        if (vbus_volt >= 8200 && vbus_volt <= 9200) {
            sgm->qc_type = QUICK_CHARGE_2P0;
            sgm->voltage_max = 9000000;
            sgm->current_max = 2000000;
            //sgm41542_set_usbin_current_limit(sgm, 2000);
            power_supply_changed(sgm->charger_psy);
            dev_info(sgm->dev,
                "hvdcp qc2.0 detected, vbus voltage: %d", vbus_volt);
            break;
        }
    }
}

static void sgm41542_hvdcp_detect_work(
    struct work_struct *work)
{
    struct sgm41542_state state;
    struct sgm41542_device *sgm = container_of(
        work, struct sgm41542_device, hvdcp_work.work);

    dev_info(sgm->dev, "hvdcp work started");
    if (sgm->qc_force == QC_TYPE_FORCE_1P0) {
        dev_info(sgm->dev, "force qc1.0");
        sgm41542_set_usbin_current_limit(sgm, 3000);
    } else if (sgm->qc_force == QC_TYPE_FORCE_2P0) {
        dev_info(sgm->dev, "force qc2.0");
        sgm41542_request_qc2p0(sgm);
    } else if (sgm->qc_force == QC_TYPE_FORCE_3P0) {
        dev_info(sgm->dev, "force qc3.0");
        sgm41542_request_qc3p0(sgm);
    } else { // sgm->qc_force == QC_TYPE_AUTO_CHECK
        dev_info(sgm->dev, "auto check qc type");
        sgm->qc_type = QUICK_CHARGE_1P0; // default mode qc1.0
        sgm->voltage_max = 5000000;
        sgm->current_max = 1000000;
        sgm41542_request_qc2p0(sgm);
        if (sgm->qc_type == QUICK_CHARGE_2P0)
            sgm41542_request_qc3p0(sgm);
    }

    if (sgm->qc_type == QUICK_CHARGE_1P0) {
        // qc1.0 set current to 1A
        sgm->qc_type = QUICK_CHARGE_1P0;
        sgm->voltage_max = 5000000;
        sgm->current_max = 1000000;
        sgm41542_set_usbin_current_limit(sgm, 3000);
        power_supply_changed(sgm->charger_psy);
        dev_info(sgm->dev, "set charge mode to qc1.0");
    }

    if (current_max_sgm7220 == 1500 ||
        current_max_wusb3801 == 1500) {
        sgm41542_set_usbin_current_limit(sgm, 1500);
        dev_info(sgm->dev, "change charge current to 1500ma");
    }

    sgm41542_get_status(sgm, &state);
    if (!state.pg_stat) {
        sgm->voltage_max = 0;
        sgm->current_max = 0;
        sgm->state.vbus_stat = 0;
        sgm->state.chrg_stat = 0;
        power_supply_changed(sgm->charger_psy);
        dev_info(sgm->dev, "charger offline in hvdcp work");
    }
}

static void sgm41542_psy_change_work(
    struct work_struct *work)
{
    struct sgm41542_device *sgm = container_of(
        work, struct sgm41542_device, psy_work.work);

    power_supply_changed(sgm->charger_psy);
}

static irqreturn_t sgm41542_irq_handler_thread(
    int irq, void *private)
{
    struct sgm41542_state state;
    struct sgm41542_device *sgm = private;

    dev_info(sgm->dev, "charger isr");
    sgm41542_get_status(sgm, &state);
    // status not changed
    if (sgm->state.pg_stat == state.pg_stat) {
        dev_info(sgm->dev, "state not changed");
        return IRQ_HANDLED;
    }
    sgm->state = state;
    if (sgm->state.pg_stat) {
        sgm->state.chrg_stat = SGM41542_PRECHARGE;
        schedule_delayed_work(&sgm->irq_work, msecs_to_jiffies(100));
        schedule_delayed_work(&sgm->charge_work, msecs_to_jiffies(3000));
        dev_info(sgm->dev, "schedule irq work");
    } else {
        sgm->ibat = 3000;
        sgm->voltage_max = 0;
        sgm->current_max = 0;
        sgm->state.vbus_stat = 0;
        sgm->state.chrg_stat = 0;
        sgm->temp_state = SGM41542_STATE_NORMAL;
        sgm41542_charge_enable_ctrl(sgm, 1);
        cancel_delayed_work(&sgm->irq_work);
        cancel_delayed_work(&sgm->charge_work);
        cancel_delayed_work(&sgm->hvdcp_work);
        dev_info(sgm->dev, "cancel irq work");
    }

    power_supply_changed(sgm->charger_psy);

    return IRQ_HANDLED;
}

static void sgm41542_charge_irq_work(
    struct work_struct *work)
{
    struct sgm41542_state state;
    struct sgm41542_device *sgm = container_of(
            work, struct sgm41542_device, irq_work.work);

    dev_info(sgm->dev, "irq work");

    sgm41542_get_status(sgm, &state);
    sgm->state.vbus_stat = state.vbus_stat;
    if (state.vbus_stat == SGM41542_USB_DCP) {
        dev_info(sgm->dev, "dcp detected");
        sgm->voltage_max = 5000000;
        sgm->current_max = 1000000;
        // Vdp keep 0.6V for 1.5s, charger disconnect dpdm
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DP_VSEL_MASK, SGM41542_DP_0V6);
        schedule_delayed_work(&sgm->hvdcp_work, msecs_to_jiffies(1500));
    } else if (state.vbus_stat == SGM41542_USB_SDP) {
        sgm->voltage_max = 5000000;
        sgm->current_max = 500000;
        dev_info(sgm->dev, "sdp detected");
    } else if (sgm->state.vbus_stat == SGM41542_USB_CDP) {
        sgm->voltage_max = 5000000;
        sgm->current_max = 1500000;
        dev_info(sgm->dev, "cdp detected");
    } else if (sgm->state.vbus_stat == SGM41542_USB_UNKNOWN ||
        sgm->state.vbus_stat == SGM41542_USB_NSTANDA) {
        sgm->voltage_max = 5000000;
        sgm->current_max = 1000000;
        sgm41542_set_usbin_current_limit(sgm, 1000);
        dev_info(sgm->dev, "unknown charge detected");
    }

    power_supply_changed(sgm->charger_psy);
}

static int sgm41542_charger_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    int ret, value;
    struct sgm41542_device *sgm = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_PARALLEL;
        dev_info(sgm->dev, "charger psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (sgm->state.pg_stat)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sgm->dev, "charger online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        val->intval = sgm41542_charge_type[sgm->state.chrg_stat];
        dev_info(sgm->dev, "charger psy charge type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_STATUS:
        value = sgm->state.chrg_stat;
        if (value == SGM41542_PRECHARGE || value == SGM41542_FAST_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
        else if (!sgm->state.pg_stat)
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (sgm->state.pg_stat && !sgm->chg_en)
            val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if (value == SGM41542_TERM_CHARGE)
            val->intval = POWER_SUPPLY_STATUS_FULL;
        else
            val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
        dev_info(sgm->dev, "charger psy prop status: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_USB_TYPE:
        // 1:SDP/2:CDP/3:DCP/0,4,5,6,7:unknown
        if (sgm->state.vbus_stat > 0 && sgm->state.vbus_stat < 4)
            val->intval = sgm41542_usb_type[sgm->state.vbus_stat - 1];
        else
            val->intval = POWER_SUPPLY_USB_TYPE_UNKNOWN;
        dev_info(sgm->dev, "charger usb type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        ret = iio_read_channel_processed(sgm->vbus, &value);
        if (ret < 0) {
            dev_err(sgm->dev, "get charger voltage failed");
            return -EINVAL;
        }
        // vbus voltage = Vr2 + Vr2 / Rr2 * Rr1
        val->intval = value + R_VBUS_CHARGER_1 * value / R_VBUS_CHARGER_2;
        dev_info(sgm->dev, "charger voltage: %d", val->intval);
        break;
    default:
        dev_err(sgm->dev, "get charger prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply_desc sgm41542_charger_desc = {
    .name = "mtk_charger_type",
    .type = POWER_SUPPLY_TYPE_PARALLEL,
    .usb_types = sgm41542_usb_type,
    .num_usb_types = ARRAY_SIZE(sgm41542_usb_type),
    .properties = sgm41542_charger_props,
    .num_properties = ARRAY_SIZE(sgm41542_charger_props),
    .get_property = sgm41542_charger_get_property,
};

static char *sgm41542_charger_supplied_to[] = {
    "battery",
    "mtk-master-charger",
};

static struct power_supply *sgm41542_register_charger_psy(
    struct sgm41542_device *sgm)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sgm,
        .of_node = sgm->dev->of_node,
    };

    psy_cfg.supplied_to = sgm41542_charger_supplied_to;
    psy_cfg.num_supplicants = ARRAY_SIZE(sgm41542_charger_supplied_to);
    return power_supply_register(sgm->dev, &sgm41542_charger_desc, &psy_cfg);
}

static int sgm41542_ac_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct sgm41542_device *sgm = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        if (sgm->state.vbus_stat == SGM41542_USB_DCP ||
            sgm->state.vbus_stat == SGM41542_USB_UNKNOWN ||
            sgm->state.vbus_stat == SGM41542_USB_NSTANDA)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sgm->dev, "ac online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = sgm->current_max;
        dev_info(sgm->dev, "ac current maxium: %d", sgm->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = sgm->voltage_max;
        dev_info(sgm->dev, "ac voltage maxium: %d", sgm->voltage_max);
        break;
    default:
        dev_err(sgm->dev, "get ac prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static struct power_supply_desc sgm41542_ac_desc = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .properties = sgm41542_ac_props,
    .num_properties = ARRAY_SIZE(sgm41542_ac_props),
    .get_property = sgm41542_ac_get_property,
};

static struct power_supply *sgm41542_register_ac_psy(
    struct sgm41542_device *sgm)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sgm,
        .of_node = sgm->dev->of_node,
    };

    return power_supply_register(sgm->dev, &sgm41542_ac_desc, &psy_cfg);
}

static int sgm41542_usb_writeable(
    struct power_supply *psy, enum power_supply_property psp)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        return 1;
    default:
        return 0;
    }
}

static int sgm41542_usb_get_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    union power_supply_propval *val)
{
    struct sgm41542_device *sgm = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_TYPE:
        val->intval = POWER_SUPPLY_TYPE_USB;
        dev_info(sgm->dev, "usb psy prop type: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        val->intval = sgm->chg_en;
        dev_info(sgm->dev, "usb psy prop charge enable: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_ONLINE:
        if (sgm->state.vbus_stat == SGM41542_USB_SDP ||
            sgm->state.vbus_stat == SGM41542_USB_CDP)
            val->intval = 1;
        else
            val->intval = 0;
        dev_info(sgm->dev, "usb online: %d", val->intval);
        break;
    case POWER_SUPPLY_PROP_CURRENT_MAX:
        val->intval = sgm->current_max;
        dev_info(sgm->dev, "usb current maxium: %d", sgm->current_max);
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX:
        val->intval = sgm->voltage_max;
        dev_info(sgm->dev, "usb voltage maxium: %d", sgm->voltage_max);
        break;
    default:
        dev_err(sgm->dev, "get usb prop: %u is not supported", psp);
        return -EINVAL;
    }

    return 0;
}

static int sgm41542_usb_set_property(
    struct power_supply *psy,
    enum power_supply_property psp,
    const union power_supply_propval *val)
{
    struct sgm41542_device *sgm = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_CHARGE_ENABLED:
        sgm41542_charge_enable_ctrl(sgm, !!val->intval);
        schedule_delayed_work(&sgm->psy_work, msecs_to_jiffies(100));
        return 0;
    default:
        return -EPERM;
    }
}

static struct power_supply_desc sgm41542_usb_desc = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .properties = sgm41542_usb_props,
    .num_properties = ARRAY_SIZE(sgm41542_usb_props),
    .get_property = sgm41542_usb_get_property,
    .set_property = sgm41542_usb_set_property,
    .property_is_writeable = sgm41542_usb_writeable,
};

static struct power_supply *sgm41542_register_usb_psy(
    struct sgm41542_device *sgm)
{
    struct power_supply_config psy_cfg = {
        .drv_data = sgm,
        .of_node = sgm->dev->of_node,
    };

    return power_supply_register(sgm->dev, &sgm41542_usb_desc, &psy_cfg);
}

static int sgm41542_enable_vbus(struct regulator_dev *rdev)
{
    struct sgm41542_device *sgm = rdev_get_drvdata(rdev);

    sgm->otg_mode = 1;
    dev_info(sgm->dev, "enable otg");
    return sgm41542_update_bits(sgm,
            SGM41542_CHRG_CTRL_1, SGM41542_OTG_EN, SGM41542_OTG_EN);
}

static int sgm41542_disable_vbus(struct regulator_dev *rdev)
{
    struct sgm41542_device *sgm = rdev_get_drvdata(rdev);

    sgm->otg_mode = 0;
    dev_info(sgm->dev, "dibable otg");
    return sgm41542_update_bits(sgm,
        SGM41542_CHRG_CTRL_1, SGM41542_OTG_EN, 0);
}

static int sgm41542_vbus_state(struct regulator_dev *rdev)
{
    u8 otg_en;
    struct sgm41542_device *sgm = rdev_get_drvdata(rdev);

    sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_1, &otg_en);
    otg_en = !!(otg_en & SGM41542_OTG_EN);
    dev_info(sgm->dev, "otg state: %d", otg_en);

    return otg_en;
}

static struct regulator_ops sgm41542_vbus_ops = {
    .enable = sgm41542_enable_vbus,
    .disable = sgm41542_disable_vbus,
    .is_enabled = sgm41542_vbus_state,
};

static const struct regulator_desc sgm41542_otg_rdesc = {
    .name = "sgm41542-vbus",
    .of_match = "sgm41542-otg-vbus",
    .ops = &sgm41542_vbus_ops,
    .owner = THIS_MODULE,
    .type = REGULATOR_VOLTAGE,
};

static struct regulator_dev *sgm41542_register_regulator(
    struct sgm41542_device *sgm)
{
    struct regulator_config config = {
        .dev = sgm->dev,
        .driver_data = sgm,
    };

    return regulator_register(&sgm41542_otg_rdesc, &config);
}

static int sgm41542_gpio_config(struct sgm41542_device *sgm)
{
    int ret, irqn;

    sgm->en_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,chg-en-gpio", 0);
    if (!gpio_is_valid(sgm->en_gpio)) {
        dev_err(sgm->dev, "get sgm,chg-en-gpio failed");
        return -EINVAL;
    }
    ret = gpio_request_one(sgm->en_gpio, GPIOF_OUT_INIT_LOW, "sgm_chg_en_pin");
    if (ret) {
        dev_err(sgm->dev, "request sgm,chg-en-gpio failed");
        return -EPERM;
    }

    sgm->irq_gpio = of_get_named_gpio(sgm->dev->of_node, "sgm,irq-gpio", 0);
    if (!gpio_is_valid(sgm->irq_gpio)) {
        dev_err(sgm->dev, "get sgm,irq-gpio failed\n");
        goto err_out;
    }
    irqn = gpio_to_irq(sgm->irq_gpio);
    if (irqn > 0)
        sgm->client->irq = irqn;
    else {
        dev_err(sgm->dev, "map sgm,irq-gpio to irq failed");
        goto err_out;
    }

    ret = gpio_request_one(sgm->irq_gpio, GPIOF_DIR_IN, "sgm41542_irq_pin");
    if (ret) {
        dev_err(sgm->dev, "request sgm,irq-gpio failed\n");
        goto err_out;
    }

    return 0;

err_out:
    gpio_free(sgm->en_gpio);
    return -EINVAL;
}

static ssize_t sgm41542_reg_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    int i, ret;
    char *reg_val[2];
    ulong addr, val;
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

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
        ret = i2c_smbus_write_byte_data(sgm->client, addr, val);
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
        sgm->reg_addr = val;
    }

    return size;
}

static struct device_attribute dev_attr_reg =
    __ATTR(reg, S_IWUSR, NULL, sgm41542_reg_store);

static ssize_t sgm41542_reg_dump(
    struct device *dev, struct device_attribute *attr, char *buf)
{
    int i, ret;
    ssize_t len = 0;
    struct sgm41542_device *sgm = dev_get_drvdata(dev);
    u8 reg_index[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };

    for (i = 0; i < ARRAY_SIZE(reg_index); i++) {
        ret = i2c_smbus_read_byte_data(sgm->client, reg_index[i]);
        if (ret < 0) {
            dev_err(dev, "read reg [%#x] failed\n", sgm->reg_addr);
            return -EINVAL;
        }
        len += snprintf(buf + len, PAGE_SIZE, "%#04x: %#04x\n", reg_index[i], ret);
    }

    return len;
}

static struct device_attribute dev_attr_dump =
    __ATTR(dump, S_IRUSR, sgm41542_reg_dump, NULL);

static ssize_t sgm41542_charge_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    if (!strncmp(buf, "1", 1)) {
        sgm->qc_force = QC_TYPE_FORCE_1P0;
        dev_info(dev, "enable qc1.0");
    } else if (!strncmp(buf, "2", 1)) {
        sgm->qc_force = QC_TYPE_FORCE_2P0;
        dev_info(dev, "enable qc2.0");
    } else if (!strncmp(buf, "3", 1)) {
        sgm->qc_force = QC_TYPE_FORCE_3P0;
        dev_info(dev, "enable qc3.0");
    } else {
        dev_err(dev, "invalid parameter: %s", buf);
        return -EINVAL;
    }

    return size;
}

static struct device_attribute dev_attr_charge =
    __ATTR(charge, S_IWUSR, NULL, sgm41542_charge_store);

static ssize_t sgm41542_dpdm_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    if (!strncmp(buf, "dp", 2)) {
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DP_VSEL_MASK, SGM41542_DP_3V3);
        usleep_range(100, 100);
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DP_VSEL_MASK, SGM41542_DP_0V6);
    } else if (!strncmp(buf, "dm", 2)) {
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DM_VSEL_MASK, SGM41542_DM_0V6);
        usleep_range(1000, 1000);
        sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_D,
            SGM41542_DM_VSEL_MASK, SGM41542_DM_3V3);
    } else {
        dev_err(dev, "error params");
        return -EINVAL;
    }

    return size;
}

static struct device_attribute dev_attr_dpdm =
    __ATTR(dpdm, S_IWUSR, NULL, sgm41542_dpdm_store);

static ssize_t sgm41542_qc3v0_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    sscanf(buf, "%d %d\n", &sgm->chg_volt, &sgm->chg_curr);
    dev_info(dev, "qc3.0 voltage: %d, current: %d",
        sgm->chg_volt, sgm->chg_curr);

    return size;
}

static struct device_attribute dev_attr_qc3v0 =
    __ATTR(qc3v0, S_IWUSR, NULL, sgm41542_qc3v0_store);

static ssize_t sgm41542_vtemp_store(
    struct device *dev, struct device_attribute *attr,
    const char *buf, size_t size)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    sscanf(buf, "%d\n", &sgm->vtemp);
    sgm41542_vtemp = sgm->vtemp;
    schedule_delayed_work(&sgm->psy_work, msecs_to_jiffies(100));
    dev_info(dev, "virtual temperature %d", sgm->vtemp);

    return size;
}

static struct device_attribute dev_attr_vtemp =
    __ATTR(vtemp, S_IWUSR, NULL, sgm41542_vtemp_store);

static int sgm41542_charging_switch(
    struct charger_device *chg_dev, bool enable)
{
    return 0;
}

static int sgm41542_set_ichrg_curr(
    struct charger_device *chg_dev, unsigned int chrg_curr)
{
    return 0;
}

static int sgm41542_get_input_curr_lim(
    struct charger_device *chg_dev, unsigned int *ilim)
{
    int ret;
    u8 reg_val;
    struct sgm41542_device *sgm = charger_get_data(chg_dev);

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_0, &reg_val);
    if (ret)
        return -EINVAL;
    if (SGM41542_IINDPM_I_MASK == (reg_val & SGM41542_IINDPM_I_MASK))
        *ilim =  SGM41542_IINDPM_I_MAX_MA;
    else
        *ilim = ((reg_val & SGM41542_IINDPM_I_MASK)
                * SGM41542_IINDPM_STEP_MA
                + SGM41542_IINDPM_I_MIN_MA) * 1000;

    return 0;
}

static int sgm41542_set_input_curr_lim(
    struct charger_device *chg_dev, unsigned int iindpm)
{
    return 0;
}

static int sgm41542_get_chrg_volt(
    struct charger_device *chg_dev, unsigned int *volt)
{
    int ret;
    u8 vreg_val;
    struct sgm41542_device *sgm = charger_get_data(chg_dev);

    ret = sgm41542_read_reg(sgm, SGM41542_CHRG_CTRL_4, &vreg_val);
    if (ret)
        return ret;

    vreg_val = (vreg_val & SGM41542_VREG_V_MASK) >> 3;

    if (15 == vreg_val)
        *volt = 4352000; //default
    else if (vreg_val < 25)
        *volt = (vreg_val * SGM41542_VREG_V_STEP_MV
                + SGM41542_VREG_V_MIN_MV) * 1000;

    return 0;
}

static int sgm41542_set_chrg_volt(
    struct charger_device *chg_dev, unsigned int chrg_volt)
{
    return 0;
}

static int sgm41542_set_wdt_rst(
    struct sgm41542_device *sgm, bool is_rst)
{
    u8 val;

    if (is_rst)
        val = SGM41542_WDT_RST_MASK;
    else
        val = 0;

    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_1,
            SGM41542_WDT_RST_MASK, val);
}

static int sgm41542_reset_watch_dog_timer(
    struct charger_device *chg_dev)
{
    struct sgm41542_device *sgm = charger_get_data(chg_dev);

    return sgm41542_set_wdt_rst(sgm, 0x1);
}

static int sgm41542_set_input_volt_lim(
    struct charger_device *chg_dev, unsigned int vindpm)
{
    return 0;
}

static int sgm41542_get_charging_status(
    struct charger_device *chg_dev, bool *is_done)
{
    struct sgm41542_device *sgm = charger_get_data(chg_dev);

    if (sgm->state.chrg_stat == SGM41542_TERM_CHARGE)
        *is_done = true;
    else
        *is_done = false;

    return 0;
}

static inline int sgm41542_set_en_timer(
    struct sgm41542_device *sgm)
{
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_5,
                SGM41542_SAFETY_TIMER_EN, SGM41542_SAFETY_TIMER_EN);
}

static inline int sgm41542_set_disable_timer(
    struct sgm41542_device *sgm)
{
    return sgm41542_update_bits(sgm, SGM41542_CHRG_CTRL_5,
                SGM41542_SAFETY_TIMER_EN, 0);
}

static int sgm41542_enable_safetytimer(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sgm41542_get_is_safetytimer_enable(
    struct charger_device *chg_dev, bool *en)
{
    int ret = 0;
    u8 val = 0;

    struct sgm41542_device *sgm = charger_get_data(chg_dev);

    ret = sgm41542_read_reg(sgm,SGM41542_CHRG_CTRL_5,&val);
    if (ret < 0) {
        pr_info("[%s] read SGM4154x_CHRG_CTRL_5 fail\n", __func__);
        return ret;
    } else
        *en = !!(val & SGM41542_SAFETY_TIMER_EN);

    return 0;
}

static int sgm41542_enable_otg(
    struct charger_device *chg_dev, bool en)
{
    return 0;
}

static int sgm41542_set_boost_current_limit(
    struct charger_device *chg_dev, u32 uA)
{
    return 0;
}

static int sgm41542_do_event(
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

static int sgm41542_en_pe_current_partern(
    struct charger_device *chg_dev, bool is_up)
{
    return 0;
}

static struct charger_ops sgm41542_chg_ops = {
    .enable = sgm41542_charging_switch,
    .get_charging_current = NULL,
    .set_charging_current = sgm41542_set_ichrg_curr,
    .get_input_current = sgm41542_get_input_curr_lim,
    .set_input_current = sgm41542_set_input_curr_lim,
    .get_constant_voltage = sgm41542_get_chrg_volt,
    .set_constant_voltage = sgm41542_set_chrg_volt,
    .kick_wdt = sgm41542_reset_watch_dog_timer,
    .set_mivr = sgm41542_set_input_volt_lim,
    .is_charging_done = sgm41542_get_charging_status,
    .enable_safety_timer = sgm41542_enable_safetytimer,
    .is_safety_timer_enabled = sgm41542_get_is_safetytimer_enable,
    .enable_otg = sgm41542_enable_otg,
    .set_boost_current_limit = sgm41542_set_boost_current_limit,
    .event = sgm41542_do_event,
    .send_ta_current_pattern = sgm41542_en_pe_current_partern,
    .set_pe20_efficiency_table = NULL,
    .send_ta20_current_pattern = NULL,
    .enable_cable_drop_comp = NULL,
};

static const struct charger_properties sgm41542_chg_props = {
    .alias_name = "sgm41542",
};

static int sgm41542_create_sysfs_file(
    struct sgm41542_device *sgm)
{
    int ret;

    ret = device_create_file(sgm->dev, &dev_attr_reg);
    if (ret) {
        dev_err(sgm->dev, "device_create_file reg failed\n");
        goto err_create_reg;
    }

    ret = device_create_file(sgm->dev, &dev_attr_dump);
    if (ret) {
        dev_err(sgm->dev, "device_create_file dump failed\n");
        goto err_create_dump;
    }

    ret = device_create_file(sgm->dev, &dev_attr_charge);
    if (ret) {
        dev_err(sgm->dev, "device_create_file hvdcp_enable failed\n");
        goto err_create_charge;
    }

    ret = device_create_file(sgm->dev, &dev_attr_dpdm);
    if (ret) {
        dev_err(sgm->dev, "device_create_file dpdm failed\n");
        goto err_create_dpdm;
    }

    ret = device_create_file(sgm->dev, &dev_attr_qc3v0);
    if (ret) {
        dev_err(sgm->dev, "device_create_file dpdm failed\n");
        goto err_create_qc3v0;
    }

    ret = device_create_file(sgm->dev, &dev_attr_vtemp);
    if (ret) {
        dev_err(sgm->dev, "device_create_file vtemp failed\n");
        goto err_create_vtemp;
    }

    return 0;

err_create_vtemp:
    device_remove_file(sgm->dev, &dev_attr_qc3v0);
err_create_qc3v0:
    device_remove_file(sgm->dev, &dev_attr_dpdm);
err_create_dpdm:
    device_remove_file(sgm->dev, &dev_attr_charge);
err_create_charge:
    device_remove_file(sgm->dev, &dev_attr_dump);
err_create_dump:
    device_remove_file(sgm->dev, &dev_attr_reg);
err_create_reg:
    return -EPERM;
}

static int sgm41542_driver_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    struct sgm41542_device *sgm;

    sgm = devm_kzalloc(&client->dev, sizeof(*sgm), GFP_KERNEL);
    if (!sgm) {
        dev_err(&client->dev, "allocate memory for sgm41542 failed\n");
        return -ENOMEM;
    }

    sgm->chg_en = 1;
    sgm->ibat = 3000;
    sgm->chg_volt = 5600;
    sgm->chg_curr = 3000;
    sgm->vtemp = SGM41542_TEMP_INVALID;
    sgm->qc_type = QUICK_CHARGE_1P0;
    sgm->qc_force = QC_TYPE_AUTO_CHECK;
    sgm->temp_state = SGM41542_STATE_NORMAL;
    sgm->client = client;
    sgm->dev = &client->dev;
    sgm->chg_desc = &sgm41542_charger_desc;
    i2c_set_clientdata(client, sgm);
    INIT_DELAYED_WORK(&sgm->irq_work, sgm41542_charge_irq_work);
    INIT_DELAYED_WORK(&sgm->psy_work, sgm41542_psy_change_work);
    INIT_DELAYED_WORK(&sgm->hvdcp_work, sgm41542_hvdcp_detect_work);
    INIT_DELAYED_WORK(&sgm->charge_work, sgm41542_charge_online_work);

    sgm->vbus = devm_iio_channel_get(sgm->dev, "pmic_vbus");
    if (IS_ERR_OR_NULL(sgm->vbus)) {
        dev_err(sgm->dev, "get vbus failed\n");
        return -EPROBE_DEFER;
    }

    ret = sgm41542_chipid_detect(sgm);
    if (ret != SGM41542_PN_ID && ret != SGM41543_PN_ID ) {
        dev_err(sgm->dev, "charger ic not founded");
        devm_iio_channel_release(sgm->dev, sgm->vbus);
        devm_kfree(sgm->dev, sgm);
        return -ENODEV;;
    }

    ret = sgm41542_gpio_config(sgm);
    if (ret < 0) {
        dev_err(sgm->dev, "config gpio failed\n");
        goto err_gpio_config;
    }

    ret = sgm41542_hardware_init(sgm);
    if (ret) {
        dev_err(sgm->dev, "hardware init failed");
        goto err_hardware_init;
    }

    /* Register charger device */
    sgm->chg_dev = charger_device_register("primary_chg",
                        &client->dev, sgm,
                        &sgm41542_chg_ops,
                        &sgm41542_chg_props);
    if (IS_ERR_OR_NULL(sgm->chg_dev)) {
        dev_err(sgm->dev, "register charger device failed\n");
        ret = PTR_ERR(sgm->chg_dev);
        goto err_hardware_init;
    }

    ret = devm_request_threaded_irq(sgm->dev, client->irq,
                NULL, sgm41542_irq_handler_thread,
                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                dev_name(sgm->dev), sgm);
    if (ret) {
        dev_err(sgm->dev, "request interrupt failed\n");
        goto err_request_irq;
    }
    ret = enable_irq_wake(client->irq);
    if (ret) {
        dev_err(sgm->dev, "enable irq wake failed\n");
        goto err_irq_wake;
    }

    ret = sgm41542_get_initial_state(sgm);
    if (ret) {
        dev_err(sgm->dev, "get initial state failed\n");
        goto err_irq_wake;
    }

    sgm->charger_psy = sgm41542_register_charger_psy(sgm);
    if (IS_ERR_OR_NULL(sgm->charger_psy)) {
        dev_err(sgm->dev, "register charger power supply failed\n");
        goto err_register_charger_psy;
    }

    sgm->usb_psy = sgm41542_register_usb_psy(sgm);
    if (IS_ERR_OR_NULL(sgm->usb_psy)) {
        dev_err(sgm->dev, "register usb power supply failed\n");
        goto err_register_usb_psy;
    }

    sgm->ac_psy = sgm41542_register_ac_psy(sgm);
    if (IS_ERR_OR_NULL(sgm->ac_psy)) {
        dev_err(sgm->dev, "register ac power supply failed\n");
        goto err_register_ac_psy;
    }

    sgm->otg_rdev = sgm41542_register_regulator(sgm);
    if (IS_ERR_OR_NULL(sgm->otg_rdev)) {
        dev_err(sgm->dev, "register regulator failed\n");
        goto err_register_rdev;
    }

    ret = sgm41542_create_sysfs_file(sgm);
    if (ret) {
        dev_err(sgm->dev, "create sysfs failed\n");
        goto err_create_sysfs;
    }

#if defined (CONFIG_TINNO_SCC_SUPPORT)
    scc_init_speed_current_map("mediatek,mt6357-gauge");
    scc_create_file(sgm->dev);
#endif  /* CONFIG_TINNO_SCC_SUPPORT */

    dev_info(sgm->dev, "probe finished");

    return 0;

err_create_sysfs:
    regulator_unregister(sgm->otg_rdev);
err_register_rdev:
    power_supply_unregister(sgm->usb_psy);
err_register_usb_psy:
    power_supply_unregister(sgm->ac_psy);
err_register_ac_psy:
    power_supply_unregister(sgm->charger_psy);
err_register_charger_psy:
    disable_irq_wake(client->irq);
err_irq_wake:
    free_irq(client->irq, sgm);
err_request_irq:
    charger_device_unregister(sgm->chg_dev);
err_hardware_init:
    gpio_free(sgm->irq_gpio);
    gpio_free(sgm->en_gpio);
err_gpio_config:
    devm_iio_channel_release(sgm->dev, sgm->vbus);
    devm_kfree(sgm->dev, sgm);
    return -EBUSY;
}

static void sgm41542_remove_sysfs_file(
    struct sgm41542_device *sgm)
{
    device_remove_file(sgm->dev, &dev_attr_vtemp);
    device_remove_file(sgm->dev, &dev_attr_qc3v0);
    device_remove_file(sgm->dev, &dev_attr_dpdm);
    device_remove_file(sgm->dev, &dev_attr_charge);
    device_remove_file(sgm->dev, &dev_attr_dump);
    device_remove_file(sgm->dev, &dev_attr_reg);
}

static int sgm41542_charger_remove(struct i2c_client *client)
{
    struct sgm41542_device *sgm = i2c_get_clientdata(client);

    sgm41542_remove_sysfs_file(sgm);
    regulator_unregister(sgm->otg_rdev);
    power_supply_unregister(sgm->usb_psy);
    power_supply_unregister(sgm->ac_psy);
    power_supply_unregister(sgm->charger_psy);
    disable_irq_wake(client->irq);
    free_irq(client->irq, sgm);
    charger_device_unregister(sgm->chg_dev);
    gpio_free(sgm->irq_gpio);
    gpio_free(sgm->en_gpio);
    devm_iio_channel_release(sgm->dev, sgm->vbus);
    devm_kfree(sgm->dev, sgm);

    return 0;
}

static int sgm41542_device_suspend(struct device *dev)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    if (sgm->state.pg_stat) {
        cancel_delayed_work(&sgm->charge_work);
        dev_info(sgm->dev, "cancel charge work");
    }

    return 0;
}

static int sgm41542_device_resume(struct device *dev)
{
    struct sgm41542_device *sgm = dev_get_drvdata(dev);

    if (sgm->state.pg_stat) {
        schedule_delayed_work(&sgm->charge_work, msecs_to_jiffies(100));
        dev_info(sgm->dev, "resume charge work");
    }

    return 0;
}

static struct dev_pm_ops sgm41542_pm_osp = {
    .suspend = sgm41542_device_suspend,
    .resume = sgm41542_device_resume,
};

static const struct i2c_device_id sgm41542_i2c_ids[] = {
    { "sgm41542", 0 }, { },
};

static const struct of_device_id sgm41542_of_match[] = {
    { .compatible = "sgm,sgm41542", }, { },
};

static struct i2c_driver sgm41542_driver = {
    .driver = {
        .name = "sgm41542-charger",
        .of_match_table = sgm41542_of_match,
        .pm = &sgm41542_pm_osp,
    },
    .probe = sgm41542_driver_probe,
    .remove = sgm41542_charger_remove,
    .id_table = sgm41542_i2c_ids,
};

module_i2c_driver(sgm41542_driver);
MODULE_LICENSE("GPL v2");

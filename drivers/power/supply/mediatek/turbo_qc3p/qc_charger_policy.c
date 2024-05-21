#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <mt-plat/v1/charger_class.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include "qc_charger_policy.h"

#define BATT_MAX_CHG_VOLT       4450
#define BATT_FAST_CHG_CURR      10000
#define BUS_OVP_THRESHOLD       9500

#define BUS_VOLT_INIT_UP        300

#define BAT_VOLT_LOOP_LMT       BATT_MAX_CHG_VOLT
#define BAT_CURR_LOOP_LMT       BATT_FAST_CHG_CURR
#define BUS_VOLT_LOOP_LMT       BUS_OVP_THRESHOLD

#define PM_WORK_RUN_INTERVAL    200
#define DEMO_SOC_LIMIT          70

extern int charger_set_qc_type(int type);

enum {
    PM_ALGO_RET_OK,
    PM_ALGO_RET_THERM_FAULT,
    PM_ALGO_RET_OTHER_FAULT,
    PM_ALGO_RET_CHG_DISABLED,
    PM_ALGO_RET_TAPER_DONE,
};

extern void hvdcp3_chgstat_notify(void);
extern bool g_capacity_level_limited_flag;
extern bool g_battery_protect_enable_flag;
static struct charger_consumer *pcurr_limit;

static const struct qcpm_config qcpm_config = {
    .bat_volt_lp_lmt        = BAT_VOLT_LOOP_LMT,
    .bat_curr_lp_lmt        = BAT_CURR_LOOP_LMT,
    .bus_volt_lp_lmt        = BUS_VOLT_LOOP_LMT,
    .bus_curr_lp_lmt        = (BAT_CURR_LOOP_LMT >> 1),

    .fc2_taper_current      = 20000,    //20mA
    .fc2_steps              = 1,

    .min_adapter_volt_required  = 11000,
    .min_adapter_curr_required  = 5000,

    .min_vbat_for_cp        = 3500,

    .cp_sec_enable          = false,
    .fc2_disable_sw         = false,
};

static struct usbqc_pm *__qcpm;

static int heartbeat_delay_ms = 100000;
static bool ignore_hysteresis_degc = false;
static int g_in_flag = 0;

extern int wt6670f_get_charger_type(void);
extern int wt6670f_reset_charger_type(void);
extern int wt6670f_set_volt_count(int count);
extern void wt6670f_reset_chg_type(void);
extern int  charger_set_thread_cv(u32 uV);
extern int  set_probe_down_flag(bool flag);
/*******************************QC API******************************/
/*
 * Set AICR & ICHG of switching charger
 *
 * @aicr: setting of AICR
 * @ichg: setting of ICHG
 */
static void usbqc_check_pca_chg_swchg(struct usbqc_pm *qcpm)
{
    if (!qcpm->sw_chg) {
        qcpm->sw_chg = get_charger_by_name("primary_chg");
        if (!qcpm->sw_chg) {
            pr_err("get primary_chg fail\n");
        }
    }
}

static void usbqc_check_charger_psy(struct usbqc_pm *qcpm)
{
    if (!qcpm->usb_psy) {
        qcpm->usb_psy = power_supply_get_by_name("sgm4154x-charger");
        if (!qcpm->usb_psy) {
            qcpm->usb_psy = power_supply_get_by_name("sy697x-charger");
            if (!qcpm->usb_psy)
                pr_err("usb psy not found!\n");
        }
    }
    if (qcpm->usb_psy) {
        pr_info("%s: the usb_psy is %s\n", __func__, qcpm->usb_psy->desc->name);
    }
}

static void usbqc_check_cp_psy(struct usbqc_pm *qcpm)
{
    if (!qcpm->cp_psy) {
        qcpm->cp_psy = power_supply_get_by_name("sc854x-standalone");
        if (!qcpm->cp_psy) {
            qcpm->cp_psy = power_supply_get_by_name("hl7139-standalone");
            if (!qcpm->cp_psy)
                pr_err("%s: cp_psy not found\n",__func__);
        }
    }

    if (qcpm->cp_psy) {
        pr_info("%s: the cp_psy is %s\n", __func__, qcpm->cp_psy->desc->name);
    }
}

static void usbqc_check_cp_sec_psy(struct usbqc_pm *qcpm)
{
    if (!qcpm->cp_sec_psy) {
        qcpm->cp_sec_psy = power_supply_get_by_name("sc8549-slave");
        if (!qcpm->cp_sec_psy)
            pr_err("cp_sec_psy not found\n");
    }
}

static void usbqc_check_batt_psy(struct usbqc_pm *qcpm)
{
    if (!qcpm->batt_psy) {
        qcpm->batt_psy = power_supply_get_by_name("battery");
        if (!qcpm->batt_psy)
            pr_err("batt psy not found\n");
    }

    if (qcpm->batt_psy) {
        pr_err("%s: the batt_psy is %s\n", __func__, qcpm->batt_psy->desc->name);
    }
}
#if 0
static int usbqc_pm_set_swchg_cap(struct usbqc_pm *qcpm, u32 aicr)
{
    int ret;
    u32 ichg;

    ret = charger_dev_set_input_current(qcpm->sw_chg, aicr * 1000);
    if (ret < 0) {
        pr_err("set aicr fail(%d)\n", ret);
        return ret;
    }

    //set ichg
    /* 90% charging efficiency */

    ichg = (90 * qcpm->cp.vbus_volt * aicr / 100) / qcpm->cp.vbat_volt;
#if 0
    ret = charger_dev_set_charging_current(qcpm->sw_chg, ichg * 1000);
    if (ret < 0) {
        pr_err("set_ichg fail(%d)\n", ret);
        return ret;
    }
#endif
    pr_info("AICR = %dmA, ICHG = %dmA\n", aicr, ichg);
    return 0;

}
#endif

static int usbqc_pm_set_curr_limit_sw(int limit)
{
    int ret;

    ret = charger_manager_set_qc_input_current_limit(pcurr_limit, 0, limit);
    if (ret < 0) {
        pr_err("%s: set sw input current limit fail\n", __func__);
    }
    ret = charger_manager_set_qc_charging_current_limit(pcurr_limit, 0, limit);
    if (ret < 0) {
        pr_err("%s: set sw current limit fail\n", __func__);
        return ret;
    }
    return 0;
}

/*
 * Enable charging of switching charger
 * For divide by two algorithm, according to swchg_ichg to decide enable or not
 *
 * @en: enable/disable
 */
static int usbqc_pm_enable_sw(struct usbqc_pm *qcpm, bool en)
{
    int ret;

    pr_info("%s: en = %d\n", __func__, en);
    if (en) {
        ret = charger_dev_enable(qcpm->sw_chg, true);
        if (ret < 0) {
            pr_err("en swchg fail(%d)\n", ret);
            return ret;
        }
    } else {
        ret = charger_dev_enable(qcpm->sw_chg, false);
        if (ret < 0) {
            pr_err("en swchg fail(%d)\n", ret);
            return ret;
        }
    }
    //qcpm->sw.charge_enabled = en;
    return 0;
}

static usbqc_pm_enable_term_sw(struct usbqc_pm *qcpm, bool en) {
    int ret = 0;

    if (en) {
        ret = charger_dev_enable_termination(qcpm->sw_chg, true);
        if(ret < 0) {
            pr_err("enable termination failed\n");
        }
    } else {
        ret = charger_dev_enable_termination(qcpm->sw_chg, false);
        if(ret < 0) {
            pr_err("disable termination failed\n");
        }
    }
    return ret;
}

/*
 * Get ibus current of switching charger
 *
*/
static int usbqc_pm_update_sw_status(struct usbqc_pm *qcpm)
{
    union power_supply_propval val = {0,};
    int ret;

    usbqc_check_charger_psy(qcpm);

    ret = power_supply_get_property(qcpm->usb_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
    if (!ret) {
        qcpm->sw.charge_enabled = val.intval;
    }
    pr_err("%s: charge_enabled is %d\n", __func__, val.intval);
    return ret;
}

static int usbqc_get_input_voltage_settled(struct usbqc_pm *qcpm, u32 *vbus_volt)
{
    int ret;
    union power_supply_propval val = {0,};


    usbqc_check_cp_psy(qcpm);

    if (!qcpm->cp_psy)
        return -ENODEV;

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BUS_VOLTAGE, &val);
    if (!ret)
        *vbus_volt = val.intval;

    return ret;
}

static int usbqc_get_input_current(struct usbqc_pm *qcpm, u32 *ibus_pump)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_psy(qcpm);
    if (!qcpm->cp_psy)
        return -ENODEV;

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BUS_CURRENT, &val);
    if (!ret)
        *ibus_pump = val.intval;

    return ret;
}

static void usbqc_pm_update_cp_status(struct usbqc_pm *qcpm)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_psy(qcpm);
    usbqc_check_batt_psy(qcpm);

    if (!qcpm->cp_psy )
        return;

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE, &val);
    if (!ret)
        qcpm->cp.vbat_volt = val.intval;
    pr_err("%s: vbat_volt %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BATTERY_CURRENT, &val);
    if (!ret)
        qcpm->cp.ibat_curr = val.intval;
    pr_err("%s: ibat_curr %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BUS_VOLTAGE, &val);
    if (!ret)
        qcpm->cp.vbus_volt = val.intval;
    pr_err("%s: vbus_volt %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BUS_CURRENT, &val);
    if (!ret)
        qcpm->cp.ibus_curr = val.intval;
    pr_err("%s: ibus_curr %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_VBUS_ERROR_STATUS, &val);
    if (!ret)
    {
        pr_err("%s: vbus error state : %02x\n", __func__, val.intval);
        qcpm->cp.vbus_error_low = (val.intval >> 5) & 0x01;
        qcpm->cp.vbus_error_high = (val.intval >> 4) & 0x01;
    }

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BUS_TEMPERATURE, &val);
    if (!ret)
        qcpm->cp.bus_temp = val.intval;
    pr_err("%s: bus_temp %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BATTERY_TEMPERATURE, &val);
    if (!ret)
        qcpm->cp.bat_temp = val.intval;
    pr_err("%s: bat_temp %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_DIE_TEMPERATURE, &val);
    if (!ret)
        qcpm->cp.die_temp = val.intval;
    pr_err("%s: die_temp %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_BATTERY_PRESENT, &val);
    if (!ret)
        qcpm->cp.batt_pres = val.intval;
    pr_err("%s: batt_pres %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_VBUS_PRESENT, &val);
    if (!ret)
        qcpm->cp.vbus_pres = val.intval;
    pr_err("%s: vbus_pres %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
    if (!ret)
        qcpm->cp.charge_enabled = val.intval;
    pr_err("%s: charge_enabled %d\n", __func__, val.intval);

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_ALARM_STATUS, &val);
    if (!ret) {
        qcpm->cp.bat_ovp_alarm = !!(val.intval & BAT_OVP_ALARM_MASK);
        qcpm->cp.bat_ocp_alarm = !!(val.intval & BAT_OCP_ALARM_MASK);
        qcpm->cp.bus_ovp_alarm = !!(val.intval & BUS_OVP_ALARM_MASK);
        qcpm->cp.bus_ocp_alarm = !!(val.intval & BUS_OCP_ALARM_MASK);
        qcpm->cp.bat_ucp_alarm = !!(val.intval & BAT_UCP_ALARM_MASK);
        qcpm->cp.bat_therm_alarm = !!(val.intval & BAT_THERM_ALARM_MASK);
        qcpm->cp.bus_therm_alarm = !!(val.intval & BUS_THERM_ALARM_MASK);
        qcpm->cp.die_therm_alarm = !!(val.intval & DIE_THERM_ALARM_MASK);
    }

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_SC_FAULT_STATUS, &val);
    if (!ret) {
        qcpm->cp.bat_ovp_fault = !!(val.intval & BAT_OVP_FAULT_MASK);
        qcpm->cp.bat_ocp_fault = !!(val.intval & BAT_OCP_FAULT_MASK);
        qcpm->cp.bus_ovp_fault = !!(val.intval & BUS_OVP_FAULT_MASK);
        qcpm->cp.bus_ocp_fault = !!(val.intval & BUS_OCP_FAULT_MASK);
        qcpm->cp.bat_therm_fault = !!(val.intval & BAT_THERM_FAULT_MASK);
        qcpm->cp.bus_therm_fault = !!(val.intval & BUS_THERM_FAULT_MASK);
        qcpm->cp.die_therm_fault = !!(val.intval & DIE_THERM_FAULT_MASK);
    }

}
static void usbqc_pm_update_cp_sec_status(struct usbqc_pm *qcpm)
{
    int ret;
    union power_supply_propval val = {0,};

    if (!qcpm_config.cp_sec_enable)
        return;

    usbqc_check_cp_sec_psy(qcpm);

    if (!qcpm->cp_sec_psy)
        return;

    ret = power_supply_get_property(qcpm->cp_sec_psy,
            POWER_SUPPLY_PROP_SC_BUS_CURRENT, &val);
    if (!ret)
        qcpm->cp_sec.ibus_curr = val.intval;

    ret = power_supply_get_property(qcpm->cp_sec_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
    if (!ret)
        qcpm->cp_sec.charge_enabled = val.intval;
}

static int usbqc_pm_enable_cp(struct usbqc_pm *qcpm, bool enable)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_psy(qcpm);

    if (!qcpm->cp_psy)
        return -ENODEV;

    val.intval = enable;
    ret = power_supply_set_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);

    return ret;
}

static int usbqc_pm_enable_cp_sec(struct usbqc_pm *qcpm, bool enable)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_sec_psy(qcpm);

    if (!qcpm->cp_sec_psy)
        return -ENODEV;

    val.intval = enable;
    ret = power_supply_set_property(qcpm->cp_sec_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);

    return ret;
}

static int usbqc_pm_check_cp_enabled(struct usbqc_pm *qcpm)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_psy(qcpm);

    if (!qcpm->cp_psy)
        return -ENODEV;

    ret = power_supply_get_property(qcpm->cp_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
    if (!ret)
        qcpm->cp.charge_enabled = !!val.intval;


    return ret;
}

static int usbqc_pm_check_cp_sec_enabled(struct usbqc_pm *qcpm)
{
    int ret;
    union power_supply_propval val = {0,};

    usbqc_check_cp_sec_psy(qcpm);

    if (!qcpm->cp_sec_psy)
        return -ENODEV;

    ret = power_supply_get_property(qcpm->cp_sec_psy,
            POWER_SUPPLY_PROP_CHARGING_ENABLED, &val);
    if (!ret)
        qcpm->cp_sec.charge_enabled = !!val.intval;

    return ret;
}

static void usbqc_pm_move_state(struct usbqc_pm *qcpm, enum pm_sm_state_t state)
{
#if 1
    pr_err("state change:%s -> %s\n",
    pm_sm_state_str[qcpm->state], pm_sm_state_str[state]);
#endif
    qcpm->state = state;
}

void usbqc_set_pps_result_history(struct usbqc_pm *qcpm, int pps_result)
{
    if(qcpm->pps_result_history_idx >= PPS_RET_HISTORY_SIZE - 1)
        qcpm->pps_result_history_idx = 0;

    if(pps_result < 0)
        qcpm->pps_result_history_history[qcpm->pps_result_history_idx] = 1;
    else
        qcpm->pps_result_history_history[qcpm->pps_result_history_idx] = 0;
    qcpm->pps_result_history_idx++;
    qcpm->pps_result = pps_result;
}

int usbqc_get_pps_result_history(struct usbqc_pm *qcpm)
{
    return qcpm->pps_result_history_history[qcpm->pps_result_history_idx];
}

void usbqc_clear_pps_result_history(struct usbqc_pm *qcpm)
{
    u32 idx;
    for(idx = 0; idx < PPS_RET_HISTORY_SIZE; idx++)
    {
        qcpm->pps_result_history_history[idx] = 0;
    }
    qcpm->pps_result = 0;
}

static bool qc_find_temp_zone(struct usbqc_pm *qcpm, int temp_c)
{
    int i;
    u32 num_zones;
    int max_temp;
    u32 prev_zone;
    int hotter_t,colder_t;
    int hotter_fcc,colder_fcc;
    struct qc_chrg_step_zone *zones;

    zones = qcpm->temp_zones;
    num_zones = qcpm->qc_temp_zones_num;
    prev_zone = qcpm->pres_temp_zone;

    pr_info("%s : prev zone %d, temp_c %d\n", __func__, prev_zone, temp_c);
    max_temp = zones[num_zones - 1].temp_c;

    if(prev_zone == ZONE_NONE) {
        for(i = num_zones - 1; i >= 0; i--) {
            if(temp_c >= zones[i].temp_c) {
                if(i == num_zones - 1)
                    qcpm->pres_temp_zone = ZONE_HOT;
                else
                    qcpm->pres_temp_zone = i + 1;
                return true;
            }
        }
        qcpm->pres_temp_zone = ZONE_COLD;
        return true;
    }

    if(prev_zone == ZONE_COLD) {
        if(temp_c  >= MIN_TEMP_C + HYSTERESIS_DEGC)
            qcpm->pres_temp_zone = ZONE_FIRST;
    } else if(prev_zone == ZONE_HOT) {
        if(temp_c <= max_temp - HYSTERESIS_DEGC)
            qcpm->pres_temp_zone = num_zones - 1;
    } else {
        if(prev_zone == ZONE_FIRST) {
            hotter_t = zones[prev_zone].temp_c;
            colder_t = MIN_TEMP_C;
            hotter_fcc = zones[prev_zone + 1].chrg_step_power->chrg_step_curr;
            colder_fcc = 0;
        } else if(prev_zone == num_zones - 1) {
            hotter_t = zones[prev_zone].temp_c;
            colder_t = zones[prev_zone - 1].temp_c;
            hotter_fcc = 0;
            colder_fcc = zones[prev_zone - 1].chrg_step_power->chrg_step_curr;
        } else {
            hotter_t = zones[prev_zone].temp_c;
            colder_t = zones[prev_zone - 1].temp_c;
            hotter_fcc = zones[prev_zone + 1].chrg_step_power->chrg_step_curr;
            colder_fcc = zones[prev_zone - 1].chrg_step_power->chrg_step_curr;
        }

        if(!ignore_hysteresis_degc) {
            if(zones[prev_zone].chrg_step_power->chrg_step_curr < hotter_fcc)
                hotter_t += HYSTERESIS_DEGC;
            if(zones[prev_zone].chrg_step_power->chrg_step_curr < colder_fcc)
                colder_t -= HYSTERESIS_DEGC;
        }

        if(temp_c <= MIN_TEMP_C)
            qcpm->pres_temp_zone = ZONE_COLD;
        else if(temp_c >= max_temp)
            qcpm->pres_temp_zone = ZONE_HOT;
        else if(temp_c >= hotter_t)
            qcpm->pres_temp_zone++;
        else if(temp_c < colder_t)
            qcpm->pres_temp_zone--;
    }
    pr_info("%s: batt temp_c %d, prev zone %d, pres zone %d, "
                "hotter_fcc %dmA, colder_fcc %dmA, "
                "hotter_t %dC, colder_t %dC\n", __func__,
                temp_c, prev_zone, qcpm->pres_temp_zone,
                hotter_fcc, colder_fcc, hotter_t, colder_t);
    if(prev_zone != qcpm->pres_temp_zone) {
        pr_info("%s: Entered Temp Zone %d!\n", __func__, qcpm->pres_temp_zone);
        return true;
    }
    return false;
}


bool qc_find_chrg_step(struct usbqc_pm *qcpm, int temp_zone, int vbatt_volt)
{
    int batt_volt, i;
    bool find_step = false;

    struct qc_chrg_step_zone *zone;
    struct qc_chrg_step_power *chrg_steps;
    struct qc_chrg_step_info chrg_step_inline;
    struct qc_chrg_step_info prev_step;

    if(!qcpm) {
        pr_err("%s called before qcpm valid!\n", __func__);
        return false;
    }

    if(qcpm->pres_temp_zone == ZONE_HOT ||
        qcpm->pres_temp_zone == ZONE_COLD ||
        qcpm->pres_temp_zone < ZONE_FIRST) {
            pr_err("%s : pres temp zone is HOT or COLD,"
                       " cannot find chrg step\n", __func__);
            return false;
    }

    zone = &qcpm->temp_zones[qcpm->pres_temp_zone];
    chrg_steps = zone->chrg_step_power;
    prev_step = qcpm->chrg_step;

    batt_volt = vbatt_volt;
    chrg_step_inline.temp_c = zone->temp_c;

    for(i = 0; i < qcpm->chrg_step_nums; i++) {
        if(chrg_steps[i].chrg_step_volt > 0 && batt_volt < chrg_steps[i].chrg_step_volt) {
            if((i + 1) < qcpm->chrg_step_nums && chrg_steps[i + 1].chrg_step_volt > 0) {
                chrg_step_inline.chrg_step_cv_tapper_curr = chrg_steps[i + 1].chrg_step_curr;
            } else {
                chrg_step_inline.chrg_step_cv_tapper_curr = chrg_steps[i].chrg_step_curr;
            }
            chrg_step_inline.chrg_step_cc_curr = chrg_steps[i].chrg_step_curr;
            chrg_step_inline.chrg_step_cv_volt = chrg_steps[i].chrg_step_volt;
            chrg_step_inline.pres_chrg_step = i;
            find_step = true;
            pr_info("%s: find chrg step\n", __func__);
            break;
        }
    }

    if(find_step) {
        pr_info("%s: chrg step %d, step cc curr %d, step cv volt %d, "
                    "step cv tapper curr %d\n", __func__,
                    chrg_step_inline.pres_chrg_step,
                    chrg_step_inline.chrg_step_cc_curr,
                    chrg_step_inline.chrg_step_cv_volt,
                    chrg_step_inline.chrg_step_cv_tapper_curr);
        qcpm->chrg_step = chrg_step_inline;
    } else {
        for(i = 0; i < qcpm->chrg_step_nums; i++) {
            if(chrg_steps[i].chrg_step_volt > 0 && batt_volt > chrg_steps[i].chrg_step_volt) {
                if((i + 1) < qcpm->chrg_step_nums && chrg_steps[i + 1].chrg_step_volt > 0) {
                    chrg_step_inline.chrg_step_cv_tapper_curr = chrg_steps[i + 1].chrg_step_curr;
                } else {
                    chrg_step_inline.chrg_step_cv_tapper_curr = chrg_steps[i].chrg_step_curr;
                }
                chrg_step_inline.chrg_step_cc_curr = chrg_steps[i].chrg_step_curr;
                chrg_step_inline.chrg_step_cv_volt = chrg_steps[i].chrg_step_volt;
                chrg_step_inline.pres_chrg_step = i;
                find_step = true;
                pr_info("%s: find chrg step\n", __func__);
            }
        }

        if(find_step) {
            pr_info("%s: chrg step %d, "
                            "step cc curr %d, step cv volt %d, "
                            "step cv tapper curr %d\n", __func__,
                            chrg_step_inline.pres_chrg_step,
                            chrg_step_inline.chrg_step_cc_curr,
                            chrg_step_inline.chrg_step_cv_volt,
                            chrg_step_inline.chrg_step_cv_tapper_curr);
            qcpm->chrg_step = chrg_step_inline;
        }
    }

    if(find_step) {
        if(qcpm->chrg_step.chrg_step_cc_curr == qcpm->chrg_step.chrg_step_cv_tapper_curr)
            qcpm->chrg_step.last_step = true;
        else
            qcpm->chrg_step.last_step = false;

        pr_info("%s: Temp zone:%d, "
                    "select chrg step %d, step cc curr %d, "
                    "step cv volt %d, step cv tapper curr %d, "
                    "is the last chrg step %d\n", __func__,
                    qcpm->pres_temp_zone,
                    qcpm->chrg_step.pres_chrg_step,
                    qcpm->chrg_step.chrg_step_cc_curr,
                    qcpm->chrg_step.chrg_step_cv_volt,
                    qcpm->chrg_step.chrg_step_cv_tapper_curr,
                    qcpm->chrg_step.last_step);

        if(prev_step.pres_chrg_step != qcpm->chrg_step.pres_chrg_step) {
            pr_info("%s: Find the next chrg step\n", __func__);
            return true;
        }
    }
    return false;
}

void usbqc_select_pdo(struct usbqc_pm *qcpm, int target_uv, int target_ua)
{
    int ret, vbus_val;
    union power_supply_propval prop = {0,};
    int ibatt_curr = 0, vbatt_volt = 0, vbus_volt = 0, ibus_curr = 0, ibus_pump = 0;
    int req_volt_inc_step = 0, req_volt_dec_step = 0;
    int req_curr_inc_step = 0, req_curr_dec_step = 0;
    int real_inc_step = 0,     real_dec_step = 0;
    int calculated_vbus;
    int retry_count;
    int target_vbus_volt = 0;
    int count = 0;
    int ibus_usb = 200;
    int vbus_volt_new;

    pr_err(" %s: target voltage:%d, target current:%d\n", __func__, target_uv, target_ua);
    ret = power_supply_get_property(qcpm->cp_psy, POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE, &prop);
    //ret = power_supply_get_property(qcpm->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
    if(!ret)
        vbatt_volt = prop.intval;

    pr_err(" %s: battry voltage now:%d\n", __func__, vbatt_volt);
    ret = power_supply_get_property(qcpm->batt_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &prop);
    if(!ret)
        ibatt_curr = prop.intval;
    pr_err(" %s: battry current now:%d\n", __func__, ibatt_curr);

    ret = usbqc_get_input_voltage_settled(qcpm, &vbus_volt);
    pr_err(" %s: input voltage now:%d\n", __func__, vbus_volt);

    ret = usbqc_get_input_current(qcpm, &ibus_pump);
    pr_err(" %s: input current now:%d\n", __func__, ibus_pump);

    if(ibus_pump < 0)
        ibus_pump = 0;

    if(target_ua > qcpm->qc_request_curr_prev) {
        req_curr_inc_step = (target_ua - qcpm->qc_request_curr_prev) / 2;
        if(req_curr_inc_step < 0)
            req_curr_inc_step = 0;
    } else {
        req_curr_dec_step = (qcpm->qc_request_curr_prev - target_ua) / 2;
        if(req_curr_dec_step < 0)
            req_curr_dec_step = 0;
    }
    pr_err(" %s: req_curr_inc_step:%d, req_curr_dec_step:%d\n", __func__, req_curr_inc_step, req_curr_dec_step);

    if(target_uv > qcpm->qc_request_volt_prev) {
        req_volt_inc_step = target_uv - qcpm->qc_request_volt_prev;
        if(req_volt_inc_step < 0)
            req_volt_inc_step = 0;
    } else {
        req_volt_dec_step = qcpm->qc_request_volt_prev - target_uv;
        if(req_volt_dec_step < 0)
            req_volt_dec_step = 0;
    }
    pr_err(" %s: target_uv:%d, req_volt_inc_step:%d, req_volt_dec_step:%d\n", __func__, target_uv, req_volt_inc_step, req_volt_dec_step);

    if(target_ua > qcpm->qc_curr_max || target_uv > qcpm->qc_volt_max) {
        pr_err("%s: ibus > max curr or vbus, skip inc volt\n", __func__);
        req_volt_inc_step = 0;
        req_curr_inc_step = 0;
    }

    pr_err(" %s: prev_caltulated, current inc:%d,dec:%d, voltage inc:%d,dec:%d\n", __func__, req_curr_inc_step, req_curr_dec_step,
                                                                                        req_volt_inc_step, req_volt_dec_step);
    real_inc_step = max(req_volt_inc_step, req_curr_inc_step);
    real_dec_step = max(req_volt_dec_step, req_curr_dec_step);
    pr_err(" %s: real_inc_step:%d, real_dec_step:%d\n", __func__, real_inc_step, real_dec_step);

    ibus_curr = ibus_pump + ibus_usb;
    calculated_vbus =  vbus_volt + (ibus_curr * 200) / 1000;

    if(real_dec_step > 0) {
        target_vbus_volt = vbus_volt - real_dec_step;
        if(target_vbus_volt < 4500000) {
            count = 0;
        } else {
            count -= real_dec_step / 20000;
            if(real_dec_step % 20000 > 0) {
                count--;
            }
        }
        pr_err(" %s:vbus descend to volt: %d, vbus_now = %d, count = %d\n", __func__, target_vbus_volt, vbus_volt, count);
    } else if(real_inc_step > 0 && (calculated_vbus <= qcpm->qc_volt_max)
        && ibus_curr <= qcpm->qc_curr_max) {
        target_vbus_volt = vbus_volt + real_inc_step;
        count += real_inc_step / 20000;
        if(real_dec_step % 20000 > 0) {
            count++;
        }
        pr_err(" %s:vbus increase to volt: %d, vbus_now = %d, count = %d\n", __func__, target_vbus_volt, vbus_volt, count);
    }

    for(retry_count = 0; retry_count < 3; retry_count++) {
        ret = wt6670f_set_volt_count(count);
        if(ret != 0) {
            pr_err("%s, wt6670f_set_volt_count fail,switch to main charger!\n", __func__);
            usbqc_pm_move_state(qcpm, PM_STATE_STOP_CHARGE);
        } else {
            udelay(10000 * abs(count));

            ret = usbqc_get_input_voltage_settled(qcpm, &vbus_val);
            if(!ret) {
                vbus_volt_new = vbus_val;
                pr_err("%s, vbus_volt_new :%d, vbus_volt: %d", __func__, vbus_volt_new, vbus_volt);
                if(vbus_volt_new > 0 && ((count > 0 && (vbus_volt_new > vbus_volt)) ||
                                        (count < 0 && (vbus_volt_new < vbus_volt)))) {
                    pr_err("%s, vbus modify successfully", __func__);
                    break;
                }
                #if 0
                else if(vbus_volt_new > 0) {
                    pr_err("%s, vbus modify failed, retry!\n", __func__);
                    count =(count > 0)? ( count + (vbus_volt - vbus_volt_new) / 20):
                            (count - (vbus_volt_new - vbus_volt) / 20);
                    vbus_volt = vbus_volt_new;
                }
                #endif
                else {
                    break;
                }
            } else {
                pr_err("%s, vbus read failed, exit\n", __func__);
                break;
            }
        }
    }
    pr_err("%s, pdo select over\n", __func__);
    ret = usbqc_get_input_current(qcpm, &ibus_pump);
    qcpm->qc_request_volt_prev = vbus_volt_new;
    qcpm->qc_request_volt = vbus_volt_new;
    qcpm->qc_request_curr_prev = ibus_pump;
    qcpm->qc_request_curr      = ibus_pump;
    pr_err("%s: after adjust vbus volt pd_request_volt = %d target_vbus = %d pd_request_curr = %d target_cur = %d\n",
                __func__, vbus_volt_new, qcpm->qc_request_volt_prev, ibus_pump, qcpm->qc_request_curr);
}

static int usbqc_chrg_sm_work_func(struct usbqc_pm *qcpm)
{
    int ret;
    int rc = 0;
    int vbatt_volt,batt_soc;
    int ibatt_curr,ibus_curr;
    int volt_change;
    ///bool qc_pps_balance;
    int chrg_cv_delta_volt;
    static int chrg_cv_taper_tunning_cnt = 0;
    static int chrg_cc_power_tuning_cnt = 0;
    struct qc_chrg_step_info chrg_step;
    union power_supply_propval prop = {0,};
    int state = qcpm->state;
    pr_err("%s: vbus_vol %d vbat_vol %d vout %d\n", __func__, qcpm->cp.vbus_volt, qcpm->cp.vbat_volt, qcpm->cp.vout_volt);
    pr_err("%s: ibus_curr %d ibat_curr %d\n", __func__, qcpm->cp.ibus_curr + qcpm->cp_sec.ibus_curr, qcpm->cp.ibat_curr);

    vbatt_volt = qcpm->cp.vout_volt;
    ibus_curr = qcpm->cp.ibus_curr;

    if(qcpm->pres_temp_zone == ZONE_COLD || qcpm->pres_temp_zone == ZONE_HOT
        || (vbatt_volt > qcpm->batt_ovp_lmt && !qcpm->sw.charge_enabled)) {
        usbqc_pm_move_state(qcpm, PM_STATE_STOP_CHARGE);
    }

    chrg_step = qcpm->chrg_step;
    ret = power_supply_get_property(qcpm->cp_psy, POWER_SUPPLY_PROP_SC_BATTERY_VOLTAGE, &prop);
    //ret = power_supply_get_property(qcpm->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
    if(!ret)
        vbatt_volt = prop.intval;
    pr_err(" %s: battry voltage now:%d\n", __func__, vbatt_volt);
    ret = power_supply_get_property(qcpm->batt_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &prop);
    if(!ret)
        ibatt_curr = prop.intval;
    pr_err(" %s: battry current now:%d\n", __func__, ibatt_curr);

    ret = power_supply_get_property(qcpm->batt_psy, POWER_SUPPLY_PROP_CAPACITY, &prop);
    if(!ret)
        batt_soc = prop.intval;
    pr_err(" %s: battry soc now:%d\n", __func__, batt_soc);

    switch (qcpm->state) {
    case PM_STATE_DISCONNECT:
         pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);

        qcpm->qc_request_volt = 0;
        pr_info("batt_volt-%d is ok, start flash charging\n",
                qcpm->cp.vbat_volt);
        qc_find_chrg_step(qcpm,
                            qcpm->pres_temp_zone, vbatt_volt);
        usbqc_pm_move_state(qcpm, PM_STATE_CHRG_PUMP_ENTRY);
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        qcpm->sys_therm_cooling = false;
        break;

    case PM_STATE_ENTRY:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if(qcpm->qc_pps_support && batt_soc < CP_CHRG_SOC_LIMIT) {
            qc_find_chrg_step(qcpm,
                            qcpm->pres_temp_zone, vbatt_volt);
            //charger_dev_enable_termination(qcpm->sw_chg, true);
            charger_dev_enable_vbus_ovp(qcpm->sw_chg, false);
            if (!qcpm->cp.charge_enabled) {
                usbqc_pm_enable_cp(qcpm, true);
                usbqc_pm_check_cp_enabled(qcpm);
                usbqc_pm_move_state(qcpm, PM_STATE_CHRG_PUMP_ENTRY);
            } else {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;
    case PM_STATE_SW_ENTRY:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        qcpm->qc_request_volt = 5000000;
        qcpm->qc_request_curr = ibus_curr;
        usbqc_pm_enable_term_sw(qcpm, true);
        usbqc_pm_set_curr_limit_sw(-1);
        usbqc_pm_move_state(qcpm, PM_STATE_SW_LOOP);
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;
    case PM_STATE_CHRG_PUMP_ENTRY:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if (qcpm_config.fc2_disable_sw) {
            usbqc_pm_enable_sw(qcpm, false);
            usbqc_pm_set_curr_limit_sw(500000); // chan 门限
            charger_dev_dump_registers(qcpm->sw_chg);
            if (!qcpm->sw.charge_enabled)
                usbqc_pm_move_state(qcpm, PM_STATE_SINGLE_CP_ENTRY);
        } else {
            usbqc_pm_enable_term_sw(qcpm, false);
            usbqc_pm_set_curr_limit_sw(500000); // chan 门限
            usbqc_pm_move_state(qcpm, PM_STATE_SINGLE_CP_ENTRY);
        }

        if (g_capacity_level_limited_flag) {
            pr_info("%s:Demo vesion is : %d\n", __func__, g_capacity_level_limited_flag);
            if (!qcpm->sw.charge_enabled && (batt_soc >= DEMO_SOC_LIMIT)) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        if (g_battery_protect_enable_flag) {
            pr_info("%s:battery protct mode is : %d\n", __func__, g_battery_protect_enable_flag);
            if (!qcpm->sw.charge_enabled) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        qcpm->batt_thermal_cooling = false;
        qcpm->sys_therm_cooling = false;
        charger_set_qc_type(1);
        charger_set_thread_cv(4496000);
        charger_dev_set_constant_voltage(qcpm->sw_chg, 4496000);  //qc battery cv
        charger_dev_set_eoc_current(qcpm->sw_chg, 600000); //qc cut-off current

        if (qcpm_config.cp_sec_enable) {
            qcpm->qc_request_volt = qcpm->cp.vbat_volt * 2 + BUS_VOLT_INIT_UP * 2;
        } else {
            qcpm->qc_request_volt = (2 * vbatt_volt) % 20000;
            qcpm->qc_request_volt = 2 * vbatt_volt - qcpm->qc_request_volt + qcpm->pps_volt_comp; //2 * vbatt + 500mv;
        }

        qcpm->qc_request_curr = min(qcpm->qc_curr_max, qcpm->qc_charging_curr_min);
        pr_info("%s pps init , volt %duV, curr %duA, volt comp %dmv\n",
                __func__, qcpm->qc_request_volt, qcpm->qc_request_curr, qcpm->pps_volt_comp);
        qcpm->qc_request_curr_prev = qcpm->qc_request_curr;
        qcpm->qc_request_volt_prev = 5000000;//5V
        //qcpm->qc_request_volt_prev = qcpm->cp.vbus_volt;//5V
        qcpm->qc_vbatt_volt_prev = vbatt_volt;
        qcpm->qc_ibatt_curr_prev = ibatt_curr;
        qcpm->qc_therm_loop_cn = 0;
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;

    case PM_STATE_SINGLE_CP_ENTRY:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if (!qcpm->cp.charge_enabled) {
            usbqc_pm_enable_cp(qcpm, true);
            usbqc_pm_check_cp_enabled(qcpm);
        }
        usbqc_pm_move_state(qcpm, PM_STATE_PPS_TUNNING_CURR);
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;

    case PM_STATE_PPS_TUNNING_CURR:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
        pr_info("%s: Temp zone:%d,select chrg step %d, step cc curr %d, "
                    "step cv volt %d, step cv tapper curr %d, "
                    "is the last chrg step %d\n", __func__,
                    qcpm->pres_temp_zone,
                    qcpm->chrg_step.pres_chrg_step,
                    qcpm->chrg_step.chrg_step_cc_curr,
                    qcpm->chrg_step.chrg_step_cv_volt,
                    qcpm->chrg_step.chrg_step_cv_tapper_curr,
                    qcpm->chrg_step.last_step);

        if (!qcpm->cp.charge_enabled) {
            qcpm->pps_volt_comp = DEFAULT_PPS_VOLT_COMP;
            usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
        } else if(vbatt_volt > qcpm->chrg_step.chrg_step_cv_volt) {
            if(qcpm->qc_request_curr - qcpm->pps_curr_steps  > qcpm->typec_middle_current || ibatt_curr - qcpm->pps_curr_steps > qcpm->chrg_step.chrg_step_cc_curr) {
                qcpm->qc_request_curr -= qcpm->pps_curr_steps;
            }
            usbqc_pm_move_state(qcpm, PM_STATE_CP_CC_LOOP);
            pr_info("%s During the going up curr process, "
                        "the chrg step was changed, "
                        "stop increase  pps curr and Enter into CC stage as soon\n", __func__);
        } else if(qcpm->pps_result  < 0) {
            if(usbqc_get_pps_result_history(qcpm) != 0) {
                usbqc_pm_move_state(qcpm, PM_STATE_CP_CC_LOOP);
                pr_info("%s Too many pdo request failed, Enter into CC stage direct\n", __func__);
                usbqc_clear_pps_result_history(qcpm);
            }
            qcpm->qc_request_curr = qcpm->qc_request_curr_prev;
            goto schedule;
        } else if(qcpm->qc_request_curr + qcpm->pps_curr_steps <= qcpm->qc_curr_max && vbatt_volt < qcpm->chrg_step.chrg_step_cv_volt && ibatt_curr < chrg_step.chrg_step_cc_curr && 
                 (qcpm->system_thermal_level == -1 || (qcpm->system_thermal_level != -1 && ibatt_curr < qcpm->system_thermal_level))) {
            pr_info("%s: qc request curr: %dua, qc curr max: %dua, chrg_step_cc_curr: %ua, pps_curr_steps: %d\n", __func__, qcpm->qc_request_curr, qcpm->qc_curr_max, qcpm->chrg_step.chrg_step_cc_curr,
                    qcpm->pps_curr_steps);
            volt_change = qcpm->pps_curr_steps;
            qcpm->qc_request_curr += min(volt_change, qcpm->chrg_step.chrg_step_cc_curr - ibatt_curr);
            pr_info("%s: Increase pps curr: %d, volt_change：%d\n", __func__, qcpm->qc_request_curr, volt_change);
            volt_change = 0;
        } else {
            if(ibatt_curr > qcpm->chrg_step.chrg_step_cc_curr) {
                volt_change = ((ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr) % 20000 == 0) ?
                    (ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr) :
                    (ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr + 20000);
                qcpm->qc_request_curr -= volt_change;
                pr_info("%s Reduce pps curr %d, volt_change %d\n", __func__, qcpm->qc_request_curr, volt_change);
                volt_change = 0;
                usbqc_pm_move_state(qcpm, PM_STATE_PPS_TUNNING_VOLT);
            } else if (qcpm->system_thermal_level != -1 && ibatt_curr > qcpm->system_thermal_level) {
                qcpm->qc_request_curr -= qcpm->pps_curr_steps;
            }
        }

        if (g_capacity_level_limited_flag) {
            if (!qcpm->sw.charge_enabled && (batt_soc >= DEMO_SOC_LIMIT)) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        if (g_battery_protect_enable_flag) {
            if (!qcpm->sw.charge_enabled) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;

    case PM_STATE_PPS_TUNNING_VOLT:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;

        if (!qcpm->cp.charge_enabled) {
            pr_info("%s: CP MASTER was disabled, "
                            "Enter into SW directly\n", __func__);
            qcpm->pps_volt_comp = DEFAULT_PPS_VOLT_STEPS;
            usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
        } else if (vbatt_volt > qcpm->chrg_step.chrg_step_cv_volt) {
            qcpm->qc_request_volt -= qcpm->pps_volt_steps;
            usbqc_pm_move_state(qcpm,
                        PM_STATE_CP_CC_LOOP);
            pr_info("%s: Duing the volt going up process, "
                        "the chrg step was changed, "
                        "stop increase pps volt and "
                        "Enter into CC stage as soon!\n", __func__);
        } else if (qcpm->pps_result < 0) {
            if (usbqc_get_pps_result_history(qcpm) != 0) {
                usbqc_pm_move_state(qcpm,
                        PM_STATE_CP_CC_LOOP);
                usbqc_clear_pps_result_history(qcpm);
                pr_info("%s: Too many pdo request failed, "
                         "Enter into CC stage directly!\n", __func__);
            }
            qcpm->qc_request_volt = qcpm->qc_request_volt_prev;
            goto schedule;
        } else if (qcpm->qc_request_volt + qcpm->pps_volt_steps
                    <= qcpm->qc_volt_max
                    && vbatt_volt < qcpm->chrg_step.chrg_step_cv_volt
                    && ibatt_curr < ((qcpm->chrg_step.pres_chrg_step == STEP_FIRST) ?
                    qcpm->chrg_step.chrg_step_cc_curr + qcpm->step_first_current_comp:
                    qcpm->chrg_step.chrg_step_cc_curr)) {
                    pr_info(" %s: qc_request_volt: %duv, qc_volt_max: %duv, chrg_step_cv_volt: %d, "
                            "pps_volt_steps: %d \n", __func__, qcpm->qc_request_volt, qcpm->qc_volt_max,
                            qcpm->chrg_step.chrg_step_cv_volt, qcpm->pps_volt_steps);
                    if(qcpm->qc_request_volt + 3 * qcpm->pps_volt_steps <= qcpm->qc_volt_max) {
                        volt_change = 3 * qcpm->pps_volt_steps;
                    } else {
                        volt_change = 1 * qcpm->pps_volt_steps;
                    }

                    ibus_curr = (qcpm->chrg_step.pres_chrg_step == STEP_FIRST) ?
                                qcpm->chrg_step.chrg_step_cc_curr + qcpm->step_first_current_comp:
                                qcpm->chrg_step.chrg_step_cc_curr;

                    qcpm->qc_request_volt += min(volt_change, ibus_curr - ibatt_curr);
                    pr_info("%s: Increase pps volt %d, volt_change: %d, ibus_curr: %d \n", __func__, qcpm->qc_request_volt, volt_change, ibus_curr);
        } else {
            if(ibatt_curr > qcpm->chrg_step.chrg_step_cc_curr){
                volt_change = ((ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr) % 20000 == 0) ?
                                        (ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr) :
                                        (ibatt_curr - qcpm->chrg_step.chrg_step_cc_curr + 20000);

                volt_change = min(volt_change, qcpm->pps_curr_steps);
                qcpm->qc_request_curr -= volt_change;

                pr_info("%s: Reduce pps curr %d, volt_change: %d \n", __func__, qcpm->qc_request_curr, volt_change);
                volt_change = 0;
            }

            pr_info("%s: Enter into CC loop stage !\n", __func__);
            usbqc_pm_move_state(qcpm, PM_STATE_CP_CC_LOOP);
        }
        break;
    case PM_STATE_CP_CC_LOOP:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        pr_info("%s: Temp zone:%d, vbatt_volt %d, select chrg step %d, step cc curr %d, "
                    "step cv volt %d, step cv tapper curr %d, "
                    "is the last chrg step %d\n", __func__,
                    qcpm->pres_temp_zone, vbatt_volt,
                    qcpm->chrg_step.pres_chrg_step,
                    qcpm->chrg_step.chrg_step_cc_curr,
                    qcpm->chrg_step.chrg_step_cv_volt,
                    qcpm->chrg_step.chrg_step_cv_tapper_curr,
                    qcpm->chrg_step.last_step);

        if (qcpm->cp_psy&& (!qcpm->cp.charge_enabled)) {
            qcpm->pps_volt_comp = DEFAULT_PPS_VOLT_COMP;
            usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            qcpm->qc_cc_loop_stage = false;
            heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
            goto schedule;
        }
#if 0
        if (qcpm->pps_result < 0) {
            pr_err("%s: Last select pdo failed\n", __func__);
            qcpm->pps_result = usbqc_get_pps_result_history(qcpm);
            switch(qcpm->pps_result) {
                case BLANCE_POWER:
                    qc_pps_balance = true;
                    usbqc_clear_pps_result_history(qcpm);
                    break;
                case RESET_POWER:
                    usbqc_pm_move_state(qcpm, PM_STATE_ENTRY);
                    usbqc_clear_pps_result_history(qcpm);
                    break;
                default:
                    break;
            }
            qcpm->qc_request_volt = qcpm->qc_request_volt_prev;
            goto schedule;
        }
#endif
#if 0
        if(((!qcpm->sys_therm_cooling) || (qcpm->systerm_thermal_level >= chrg_step.chrg_step_cc_curr))
            && !qcpm->batt_therm_cooling) {
            if(qcpm->qc_request_curr - qcpm->pps_curr_steps > qcpm->typec_middle_current) {
                qcpm->qc_request_curr -= qcpm->pps_curr_steps;
                qcpm->qc_request_volt += 0;// chan need mmi_calculate_delta_volt(qcpm->qc_request_volt_prev，qcpm->qc_request_curr, qcpm->pps_curr_steps);
            } else {
                if(qcpm->qc_request_curr + qcpm->pps_curr_steps < qcpm->qc_curr_max) {
                    qcpm->qc_request_curr += qcpm->pps_curr_steps;
                } else if(qcpm->qc_request_volt + qcpm->pps_volt_steps < qcpm->qc_volt_max) {
                    qcpm->qc_request_volt += qcpm->pps_volt_steps;
                }
            }
            chrg_cc_power_tuning_cnt = 0;
        } else if(ibatt_curr < chrg_step.chrg_step_cc_curr && qcpm->qc_request_volt < qcpm->qc_volt_max) {
            chrg_cc_power_tuning_cnt++;
        } else if(ibatt_curr > chrg_step.chrg_step_cc_curr + CC_CURR_DEBOUNCE) {
            qcpm->qc_request_volt -= qcpm->pps_volt_steps;
        } else {
            chrg_cc_power_tuning_cnt = 0;
            if(ibatt_curr < chrg_step.chrg_step_cc_curr) {
                heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
            }
        }
#endif
        if(!qcpm->qc_cc_loop_stage) {
            qcpm->qc_cc_loop_stage = true;
            qcpm->qc_vbatt_volt_prev = vbatt_volt;
        }

        if(ibatt_curr < qcpm->chrg_step.chrg_step_cc_curr) {
            qcpm->qc_request_curr += qcpm->pps_curr_steps;
            pr_err("%s: CC_LOOP_STEP_1\n", __func__);
        } else if(ibatt_curr > qcpm->chrg_step.chrg_step_cc_curr + CC_CURR_DEBOUNCE) {
            if(vbatt_volt > qcpm->qc_vbatt_volt_prev + 1000) {
                qcpm->qc_vbatt_volt_prev = vbatt_volt;
                qcpm->qc_request_curr -= qcpm->pps_curr_steps;
            } else {
                pr_info("%s: CC loop work well,continue\n", __func__);
            }
            pr_err("%s: CC_LOOP_STEP_2\n", __func__);
        } else {
            chrg_cc_power_tuning_cnt = 0;
            pr_err("%s: CC_LOOP_STEP_3\n", __func__);
        }
        pr_err("%s: ibatt_curr is %d，cc curr:%d,taper_tunning_count:%d\n", __func__, ibatt_curr,
                qcpm->chrg_step.chrg_step_cc_curr, chrg_cc_power_tuning_cnt);

        pr_err("%s: vbatt_volt is %d，cv volt:%d,taper_tunning_count:%d\n", __func__, vbatt_volt,
                qcpm->chrg_step.chrg_step_cv_volt, chrg_cv_taper_tunning_cnt);

        if(vbatt_volt >= qcpm->chrg_step.chrg_step_cv_volt) {
            if(chrg_cv_taper_tunning_cnt > CV_TAPPER_COUNT) {
                usbqc_pm_move_state(qcpm, PM_STATE_CP_CV_LOOP);
                qcpm->qc_cc_loop_stage = false;
                chrg_cv_taper_tunning_cnt = 0;
                chrg_cv_delta_volt = CV_DELTA_VOLT;
            } else {
                chrg_cv_taper_tunning_cnt++;
            }
        } else {
            chrg_cv_taper_tunning_cnt = 0;
        }

        if (g_capacity_level_limited_flag) {
            if (!qcpm->sw.charge_enabled && (batt_soc >= DEMO_SOC_LIMIT)) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        if (g_battery_protect_enable_flag) {
            if (!qcpm->sw.charge_enabled) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
        break;

    case PM_STATE_CP_CV_LOOP:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if (qcpm->cp_psy && (!qcpm->cp.charge_enabled)) {
                qcpm->pps_volt_comp = DEFAULT_PPS_VOLT_COMP;
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
                heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
                goto schedule;
        }
#if 0
        if (qcpm->pps_result < 0) {
            pr_err("%s: Last select pdo failed\n", __func__);
            qcpm->pps_result = usbqc_get_pps_result_history(qcpm);
            switch (qcpm->pps_result) {
            case BLANCE_POWER:
                    qcpm->qc_request_curr -=
                    qcpm->pps_curr_steps;
                    usbqc_clear_pps_result_history(qcpm);
                break;
            case RESET_POWER:
                usbqc_pm_move_state(qcpm, PM_STATE_ENTRY);
                usbqc_clear_pps_result_history(qcpm);
                break;
            default:
                break;
            }
            qcpm->qc_request_volt = qcpm->qc_request_volt_prev;
            goto schedule;
        }
#endif
#if 0
        if (vbatt_volt >= chrg_step.chrg_step_cv_volt
            && (!chrg_step.last_step &&
            ibatt_curr < chrg_step.chrg_step_cv_tapper_curr)) {
            //|| ibatt_curr < qcpm->cp.charging_current) {
            if (chrg_cv_taper_tunning_cnt >= CV_TAPPER_COUNT) {
                if (ibatt_curr < qcpm->qc_charging_curr_min) {
                    qc_find_chrg_step(qcpm,
                            qcpm->pres_temp_zone, vbatt_volt);
                    usbqc_pm_move_state(qcpm, PM_STATE_CP_QUIT);
                    heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
                } else {
                    if (qc_find_chrg_step(qcpm,
                            qcpm->pres_temp_zone, vbatt_volt)) {
                        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
                        usbqc_pm_move_state(qcpm,
                                    PM_STATE_CP_CC_LOOP);
                    } else {
                        usbqc_pm_move_state(qcpm, PM_STATE_CP_QUIT);
                    }
                }
                chrg_cv_taper_tunning_cnt = 0;
            } else {
                chrg_cv_taper_tunning_cnt++;
            }
        } else if (!qcpm->sys_therm_cooling
            && !qcpm->batt_therm_cooling)  {
            chrg_cv_taper_tunning_cnt = 0;
            if (vbatt_volt > chrg_step.chrg_step_cv_volt + 10000) {
                if (chrg_cv_delta_volt > 20000)
                    qcpm->qc_request_volt -= chrg_cv_delta_volt;
                else
                    qcpm->qc_request_volt -= 20000;
            } else if (vbatt_volt < chrg_step.chrg_step_cv_volt - 10000) {
                chrg_cv_delta_volt -= 20000;
                qcpm->qc_request_volt += 20000;
            } else {
                pr_info("%s: CV loop work well\n", __func__);
            }
        } else {
            if (vbatt_volt > (chrg_step.chrg_step_cv_volt + 10000)) {
                if (chrg_cv_delta_volt > 20000)
                    qcpm->qc_request_volt -= chrg_cv_delta_volt;
                else
                    qcpm->qc_request_volt -= 20000;
            } else {
                pr_info("%s ：CV loop work well\n", __func__);
            }
        }
#endif
     pr_info("%s: Temp zone:%d,vbatt_volt %d select chrg step %d, step cc curr %d, "
        "step cv volt %d, step cv tapper curr %d, "
        "is the last chrg step %d\n", __func__,
            vbatt_volt, qcpm->pres_temp_zone,
            qcpm->chrg_step.pres_chrg_step,
            qcpm->chrg_step.chrg_step_cc_curr,
            qcpm->chrg_step.chrg_step_cv_volt,
            qcpm->chrg_step.chrg_step_cv_tapper_curr,
            qcpm->chrg_step.last_step);

        if(vbatt_volt >= chrg_step.chrg_step_cv_volt && (!chrg_step.last_step)) {
            qc_find_chrg_step(qcpm,qcpm->pres_temp_zone, vbatt_volt);
            if(ibatt_curr > qcpm->chrg_step.chrg_step_cv_tapper_curr + 10000) {
                heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
                usbqc_pm_move_state(qcpm, PM_STATE_CP_CC_LOOP);
            }
        } else {
            if(qcpm->chrg_step.last_step && (ibatt_curr < qcpm->qc_charging_curr_min)) {
                if(chrg_cv_taper_tunning_cnt > CV_TAPPER_COUNT) {
                    qc_find_chrg_step(qcpm, qcpm->pres_temp_zone, vbatt_volt);
                    usbqc_pm_move_state(qcpm, PM_STATE_CP_QUIT);
                    heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
                    chrg_cv_taper_tunning_cnt = 0;
                } else {
                    chrg_cv_taper_tunning_cnt++;
                }
            } else {
                if(vbatt_volt > qcpm->chrg_step.chrg_step_cv_volt + 10000) {
                    qcpm->qc_request_volt -= 20000;
                } else if(vbatt_volt < qcpm->chrg_step.chrg_step_cv_volt - 10000) {
                    qcpm->qc_request_volt += 20000;
                } else {
                    pr_info("%s: CV loop work well, continue\n", __func__);
                }
            }
        }
        pr_info("%s: CV Level qc_request_curr: %d, qc_request_volt: %d\n", __func__,
            qcpm->qc_request_curr, qcpm->qc_request_volt);

        if (g_capacity_level_limited_flag) {
            if (!qcpm->sw.charge_enabled && (batt_soc >= DEMO_SOC_LIMIT)) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        if (g_battery_protect_enable_flag) {
            if (!qcpm->sw.charge_enabled) {
                usbqc_pm_move_state(qcpm, PM_STATE_SW_ENTRY);
            }
        }

        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
        break;


    case PM_STATE_CP_QUIT:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if (qcpm->cp.charge_enabled) {
            usbqc_pm_enable_cp(qcpm, false);
            usbqc_pm_check_cp_enabled(qcpm);
        }

        if (!qcpm->sw.charge_enabled) {
            usbqc_pm_enable_sw(qcpm, true);
        }

        qcpm->sys_therm_cooling = false;
        qcpm->batt_therm_cooling = false;
        qcpm->batt_therm_cooling_cnt = 0;
        usbqc_pm_move_state(qcpm, PM_STATE_RECOVERY_SW);
        heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
        break;

    case PM_STATE_RECOVERY_SW:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        usbqc_pm_enable_term_sw(qcpm, true);
        usbqc_pm_set_curr_limit_sw(-1); //disable qc thermal
        charger_dev_set_charging_current(qcpm->sw_chg, 2200000);
        qcpm->qc_request_curr = qcpm->qc_request_curr_prev;
        if(ibatt_curr > qcpm->chrg_step.chrg_step_cc_curr) {
            qcpm->qc_request_volt -= CV_DELTA_VOLT;
            chrg_cv_taper_tunning_cnt = 0;
        } else {
            qcpm->qc_request_volt = 5000000;
            chrg_cv_taper_tunning_cnt++;
        }

        if(chrg_cv_taper_tunning_cnt > CV_TAPPER_COUNT) {
            heartbeat_delay_ms = HEARTBEAT_SHORT_DELAY_MS;
            charger_dev_enable_vbus_ovp(qcpm->sw_chg, true);
            usbqc_pm_move_state(qcpm, PM_STATE_SW_LOOP);
        }
        pr_err("%s:qc_pps_support:%d, pres_chrg_step:%d, qc_request_volt:%d, chrg_step_cc_curr:%d\n",
            __func__, qcpm->qc_pps_support, qcpm->chrg_step.pres_chrg_step, qcpm->qc_request_volt,
            qcpm->qc_request_curr);
        heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
        break;

    case PM_STATE_SW_LOOP:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        pr_err("%s: qc_pps_support:%d, pres_chrg_step:%d, vbatt_volt:%d, chrg_step_cc_curr:%d, "
                "batt_soc:%d\n", __func__, qcpm->qc_pps_support, qcpm->chrg_step.pres_chrg_step,
                vbatt_volt, qcpm->chrg_step.chrg_step_cc_curr, batt_soc);
        if (qcpm->qc_pps_support && qcpm->chrg_step.pres_chrg_step!= (qcpm->chrg_step_nums - 1)
            && vbatt_volt > qcpm->pl_chrg_vbatt_min
            && qcpm->chrg_step.chrg_step_cc_curr > qcpm->qc_charging_curr_min
            && batt_soc < CP_CHRG_SOC_LIMIT && qcpm->system_thermal_level > qcpm->thermal_min_level) {
            usbqc_pm_move_state(qcpm, PM_STATE_CHRG_PUMP_ENTRY);
        } else if (!qcpm->sw.charge_enabled) {
            usbqc_pm_move_state(qcpm, PM_STATE_STOP_CHARGE);
        }

        if ((ibatt_curr < 900000) && (g_in_flag == 0)) { // Switch battery cv when the current is less than 900ma
            g_in_flag = 1;
            pr_err("%s: into ibatt_curr < 900000\n", __func__);
            charger_set_thread_cv(4488000);
            charger_dev_set_constant_voltage(qcpm->sw_chg, 4488000);
        } else if (ibatt_curr > 916000) {             //Switch battery cv when the current is greater than 900ma
            pr_err("%s: into ibatt_curr > 916000\n", __func__);
            g_in_flag = 0;
        }
        heartbeat_delay_ms = HEARTBEAT_SW_DELAY_MS;
        break;
    case PM_STATE_STOP_CHARGE:
        pr_err("%s: current state is : %s\n", __func__, pm_sm_state_str[qcpm->state]);
        if (qcpm->cp.charge_enabled) {
            usbqc_pm_enable_cp(qcpm, false);
            usbqc_pm_check_cp_enabled(qcpm);
        }
        if(qcpm->sw.charge_enabled) {
            usbqc_pm_enable_sw(qcpm, false);
        }
        //usbqc_pm_set_swchg_cap(qcpm, 2500);
        qc_find_chrg_step(qcpm,qcpm->pres_temp_zone, vbatt_volt);
        if(qcpm->pres_temp_zone != ZONE_COLD && qcpm->pres_temp_zone != ZONE_HOT \
        && qcpm->cp.charge_enabled && qcpm->chrg_step.chrg_step_cc_curr  > 0) {
            usbqc_pm_move_state(qcpm, PM_STATE_ENTRY);
            heartbeat_delay_ms = HEARTBEAT_NEXT_STATE_MS;
        }
        //qcpm->qc_request_curr = 3000000;
        qcpm->qc_request_volt = 5000000;
        heartbeat_delay_ms = HEARTBEAT_SW_DELAY_MS;
        break;
    default:
        break;
    }

schedule:
    qcpm->qc_request_volt = min(qcpm->qc_request_volt, qcpm->qc_volt_max);
    qcpm->qc_request_curr = min(qcpm->qc_request_curr, qcpm->qc_curr_max);

    pr_info("%s: For Thermal into schedule level is %d\n", __func__, qcpm->system_thermal_level);
    if(qcpm->system_thermal_level == THERMAL_NOT_LIMIT) {
        pr_info("%s: For Thermal level: %d, min Level: %d State: %d, middle: %d\n",
                __func__, qcpm->system_thermal_level, qcpm->thermal_min_level,
                qcpm->state,  qcpm->typec_middle_current);
        qcpm->sys_therm_cooling = false;
        qcpm->batt_thermal_cooling = false;
    } else if(qcpm->system_thermal_level <= qcpm->thermal_min_level && qcpm->batt_thermal_cooling) {
            qcpm->batt_thermal_cooling = false;
            qcpm->sys_therm_cooling = false;
            usbqc_pm_move_state(qcpm, PM_STATE_CP_QUIT);
            pr_info("%s: For Thermal is the highest : %d,"
                    "Force enter into single  pmic charging !\n",
                    __func__, qcpm->system_thermal_level);
    } else if(qcpm->system_thermal_level > qcpm->thermal_min_level &&
            (qcpm->state == PM_STATE_CP_CC_LOOP || qcpm->state == PM_STATE_CP_CV_LOOP)) {
        pr_info("%s: For Thermal level is %d\n", __func__, qcpm->system_thermal_level);
        if(!qcpm->sys_therm_cooling) {
            qcpm->sys_therm_cooling = true;
            qcpm->qc_sys_therm_volt = qcpm->qc_request_volt_prev;
            qcpm->qc_sys_therm_curr = qcpm->qc_request_curr_prev;
            qcpm->qc_ibatt_curr_prev = ibatt_curr;
        }
        qcpm->batt_thermal_cooling = true;
        //qcpm->sys_therm_cooling = false;
        if(ibatt_curr > qcpm->system_thermal_level + CC_CURR_DEBOUNCE) {
            if(qcpm->qc_sys_therm_curr - THERMAL_TUNNING_CURR >= qcpm->typec_middle_current - 200000) {
                if (ibatt_curr < qcpm->qc_ibatt_curr_prev - 33000) {
                    qcpm->qc_ibatt_curr_prev = ibatt_curr;
                    qcpm->qc_sys_therm_curr -= THERMAL_TUNNING_CURR;
                    qcpm->qc_therm_loop_cn = 0;
                    pr_info("%s: into ibatt_curr < qcpm->qc_ibatt_curr_prev - 20000\n", __func__);
                } else {
                    qcpm->qc_therm_loop_cn++;
                    pr_info("%s: qc_therm_loop_cn is %d\n", __func__, qcpm->qc_therm_loop_cn);
                    if (qcpm->qc_therm_loop_cn == 10) {
                        qcpm->qc_sys_therm_curr -= THERMAL_TUNNING_CURR;
                        qcpm->qc_therm_loop_cn = 0;
                    }
                    pr_info("%s: thermal decrease current loop well,qcpm->qc_sys_therm_curr is %d, continue\n", __func__, qcpm->qc_therm_loop_cn);
                }
                //qcpm->qc_request_curr = qcpm->qc_request_curr_prev - THERMAL_TUNNING_CURR;//= qcpm->qc_sys_therm_curr;
                pr_info("%s: For thermal, decrease pps curr %d\n", __func__, qcpm->qc_sys_therm_curr);
            } else {
                pr_info("%s: For Thermal qc_sys_therm_curr %dmA was less than %dmA, "
                                            "Give up thermal mitigation!\n",
                                            __func__, qcpm->qc_sys_therm_curr - THERMAL_TUNNING_CURR,
                                            qcpm->typec_middle_current);
            }
        } else if(ibatt_curr < qcpm->system_thermal_level - CC_CURR_DEBOUNCE) {
            if(ibatt_curr + THERMAL_TUNNING_CURR <= qcpm->chrg_step.chrg_step_cc_curr) {
                qcpm->qc_sys_therm_curr += THERMAL_TUNNING_CURR;
                //qcpm->qc_request_curr += THERMAL_TUNNING_CURR;//qcpm->qc_sys_therm_curr;
                pr_info("%s: For Thermal, increase pps curr %d\n", __func__, qcpm->qc_sys_therm_curr);
            }
        }
        heartbeat_delay_ms = THERMAL_DELAY_LESS_MS;
    } else if (qcpm->state == PM_STATE_CP_CC_LOOP || qcpm->state == PM_STATE_CP_CV_LOOP) {
        qcpm->batt_thermal_cooling = true;
    }/*else if(qcpm->system_thermal_level != THERMAL_NOT_LIMIT
        && qcpm->system_thermal_level > qcpm->thermal_min_level && qcpm->state == PM_STATE_SW_LOOP) {
            usbqc_pm_move_state(qcpm, PM_STATE_ENTRY);
    }*/
    if (qcpm->sys_therm_cooling)
        qcpm->qc_request_curr = min(qcpm->qc_request_curr, qcpm->qc_sys_therm_curr);

    if (state != PM_STATE_DISCONNECT &&  state != PM_STATE_SW_LOOP) {
        if (g_capacity_level_limited_flag) {
            if (!qcpm->sw.charge_enabled && (batt_soc >= DEMO_SOC_LIMIT)) {
                qcpm->qc_request_volt = 5000000;
            }
        }
        if (g_battery_protect_enable_flag) {
            if (!qcpm->sw.charge_enabled) {
                qcpm->qc_request_volt = 5000000;
            }
        }
        usbqc_select_pdo(qcpm,qcpm->qc_request_volt,
                            qcpm->qc_request_curr);
    }

    return rc;
}

static void usbqc_pm_workfunc(struct work_struct *work)
{
    union power_supply_propval val = {0,};
    struct usbqc_pm *qcpm = container_of(work, struct usbqc_pm, pm_work.work);
    int batt_temp;
    unsigned long flags = 0;
    int ret;
    mutex_lock(&qcpm->charger_lock);
    spin_lock_irqsave(&qcpm->slock, flags);
    if (!qcpm->charger_wakelock->active)
        __pm_stay_awake(qcpm->charger_wakelock);
    spin_unlock_irqrestore(&qcpm->slock, flags);
    usbqc_pm_update_sw_status(qcpm);
    usbqc_pm_update_cp_status(qcpm);
    usbqc_pm_update_cp_sec_status(qcpm);

    ret = power_supply_get_property(qcpm->batt_psy,
            POWER_SUPPLY_PROP_TEMP, &val);
    if (!ret)
        batt_temp = val.intval;
    batt_temp /= 10;

    qc_find_temp_zone(qcpm, batt_temp);

    if (!usbqc_chrg_sm_work_func(qcpm) && qcpm->qc_active) {
        pr_err("schedule SM work, sm state %s\n", pm_sm_state_str[qcpm->state]);
        schedule_delayed_work(&qcpm->pm_work,
                msecs_to_jiffies(heartbeat_delay_ms));
    }

    spin_lock_irqsave(&qcpm->slock, flags);
    __pm_relax(qcpm->charger_wakelock);
    spin_unlock_irqrestore(&qcpm->slock, flags);
    mutex_unlock(&qcpm->charger_lock);
}

void usbqc_suspend_notifier(bool suspend)
{
    struct usbqc_pm *qcpm = __qcpm;
    if(suspend) {
        cancel_delayed_work(&qcpm->pm_work);
    } else {
        if(qcpm->qc_active)
            schedule_delayed_work(&qcpm->pm_work, 0);
    }
}
EXPORT_SYMBOL(usbqc_suspend_notifier);

static void usbqc_pm_disconnect(struct usbqc_pm *qcpm)
{
    int chrg_type = 0;

    pr_info("%s start\n", __func__);
    usbqc_pm_enable_cp(qcpm, false);
    usbqc_pm_check_cp_enabled(qcpm);
    if (qcpm_config.cp_sec_enable) {
        usbqc_pm_enable_cp_sec(qcpm, false);
        usbqc_pm_check_cp_sec_enabled(qcpm);
    }
    cancel_delayed_work(&qcpm->pm_work);

    if (qcpm->sw.charge_enabled) {
        usbqc_pm_enable_sw(qcpm, false);
    }

    qcpm->qc_request_volt = 0;
    qcpm->qc_request_curr = 0;
    qcpm->qc_target_volt = 0;
    qcpm->qc_target_curr = 0;
    qcpm->qc_pps_support = false;
    qcpm->qc_volt_max = 0;
    qcpm->qc_curr_max = 0;

    chrg_type = wt6670f_reset_charger_type();
    if (chrg_type != 0) {
        pr_err("%s wt6670f_reset_charger_type fail\n", __func__);
    }

    usbqc_pm_set_curr_limit_sw(-1);
    charger_set_qc_type(0);
    charger_set_thread_cv(4450000);
    charger_dev_set_constant_voltage(qcpm->sw_chg, 4450000); //Default battery cv
    charger_dev_set_eoc_current(qcpm->sw_chg, 243000); //sw cut-off current
    usbqc_pm_enable_term_sw(qcpm, true);

    usbqc_pm_move_state(qcpm, PM_STATE_DISCONNECT);
}

static void usbqc_pd_contact(struct usbqc_pm *qcpm, int chrg_type, bool connected)
{
    pr_err("[SC manager] >> qc_active %d\n",
            qcpm->qc_active);

    if (connected) {
        pr_err("[SC manager] >>start cp charging pps support %d\n", qcpm->qc_pps_support);
        if (qcpm->qc_pps_support) {
            if(chrg_type == 0x09) {
                qcpm->qc_active = connected;
                hvdcp3_chgstat_notify();
                schedule_delayed_work(&qcpm->pm_work, 0);
            }
        } else {
            qcpm->qc_active = false;
        }
    } else {
        qcpm->qc_active = connected;
        usbqc_pm_disconnect(qcpm);
    }
}

#if 0
static void kick_sm(struct usbqc_pm *qcpm, u32 delay)
{
    if(!qcpm->sm_work_running) {
        schedule_delayed_work(&qcpm->pm_work, delay);
    }
    pr_info("%s\n", __func__);
}
#endif

static void usb_psy_change_work(struct work_struct *work)
{
    union power_supply_propval val = {0,};
    struct usbqc_pm *qcpm = container_of(work, struct usbqc_pm,
                    usb_psy_change_work.work);
    int chrg_type;
    int ret;

    ret = power_supply_get_property(qcpm->usb_psy,
            POWER_SUPPLY_PROP_ONLINE, &val);
    qcpm->vbus_present = val.intval;

    if(qcpm->vbus_present) {
        chrg_type = wt6670f_get_charger_type();
        switch(chrg_type) {
        case 0x8:
            qcpm->qc_volt_max = 9500000;
            qcpm->qc_curr_max = 2000000;
            qcpm->qc_pps_support = true;
            break;
        case 0x9:
            qcpm->qc_volt_max = 10000000;
            qcpm->qc_curr_max =  3100000;
            qcpm->qc_pps_support = true;
            break;
        }
    } else {
        wt6670f_reset_chg_type();
        pr_info("%s: Reset WT6670F charger Type\n", __func__);
    }

    pr_info("vbus present %d, qc pps support %d, "
            "pps max voltage %d， pps max current %d\n",
            qcpm->vbus_present, qcpm->qc_pps_support,
            qcpm->qc_volt_max, qcpm->qc_curr_max);

    if(!qcpm->qc_active && val.intval)
        usbqc_pd_contact(qcpm, chrg_type, true);
    else if (qcpm->qc_active && !val.intval)
        usbqc_pd_contact(qcpm, chrg_type, false);

    schedule_delayed_work(&qcpm->usb_psy_change_work, msecs_to_jiffies(3000));
}

static void cp_psy_change_work(struct work_struct *work)
{
    //int ret = 0;
    //union power_supply_propval propval;
    struct usbqc_pm *qcpm = container_of(work, struct usbqc_pm,
                    cp_psy_change_work.work);

    qcpm->sm_work_running = false;
}

#if 0
static int usbqc_check_plugout(struct usbqc_pm *qcpm)
{
    int ret;
    union power_supply_propval val = {0,};

    ret = power_supply_get_property(qcpm->usb_psy,
            POWER_SUPPLY_PROP_ONLINE, &val);
    if (!ret) {
        if (!val.intval) {
            usbqc_pm_enable_cp(qcpm, false);
            usbqc_pm_check_cp_enabled(qcpm);
            if (qcpm_config.cp_sec_enable) {
                usbqc_pm_enable_cp_sec(qcpm, false);
                usbqc_pm_check_cp_sec_enabled(qcpm);
            }
        }
    }

    return ret;
}
#endif
static int qcpm_psy_notifier_cb(struct notifier_block *nb,
            unsigned long event, void *data)
{
    struct usbqc_pm *qcpm = container_of(nb, struct usbqc_pm, nb);
    struct power_supply *psy = data;
    unsigned long flags;

    pr_err("%s: start\n",__func__);
    if (event != PSY_EVENT_PROP_CHANGED) {
        pr_err("%s: start with event：%d\n", __func__, event);
        return NOTIFY_OK;
    }

    usbqc_check_cp_psy(qcpm);
    if (qcpm_config.cp_sec_enable) {
        usbqc_check_cp_sec_psy(qcpm);
    }
    usbqc_check_charger_psy(qcpm);
    usbqc_check_pca_chg_swchg(qcpm);

    if (!qcpm->cp_psy || !qcpm->usb_psy || !qcpm->sw_chg) {
        pr_err("%s: cp_psy, usb_psy or sw_chg is null\n", __func__);
        return NOTIFY_OK;
    }

    //usbqc_check_plugout(qcpm);
    if (psy == qcpm->cp_psy || psy == qcpm->usb_psy) {
        spin_lock_irqsave(&qcpm->psy_change_lock, flags);
        pr_err("[SC manager] >>>qcpm->sm_work_running : %d\n", qcpm->sm_work_running);
        if(1)  {//(!qcpm->sm_work_running) {
            qcpm->sm_work_running = true;
            if (psy == qcpm->cp_psy) {
                schedule_delayed_work(&qcpm->cp_psy_change_work, 0);
            } else {
                cancel_delayed_work(&qcpm->usb_psy_change_work);
                schedule_delayed_work(&qcpm->usb_psy_change_work, 0);
            }
        }
        spin_unlock_irqrestore(&qcpm->psy_change_lock, flags);
    } else {
        pr_err("%s: the cp_psy is %s\n", __func__, psy->desc->name);
    }

    return NOTIFY_OK;
}

static void usbqc_charger_parse_dt(struct usbqc_pm *qcpm,
                                struct device *dev)
{
    struct device_node *np = dev->of_node;
    u32 val;
    u32 args[5];
    int idx;
    if (of_property_read_u32(np, "qc,pps-volt-steps", &val) >= 0)
        qcpm->pps_volt_steps = val;
    else {
        pr_err("use default pps-volt-steps:%d\n", DEFAULT_PPS_VOLT_STEPS);
        qcpm->pps_volt_steps = DEFAULT_PPS_VOLT_STEPS;
    }
    pr_err("%s: of find property pps-volt-steps:%d\n", __func__, qcpm->pps_volt_steps);

    if (of_property_read_u32(np, "qc,pps-curr-steps", &val) >= 0)
        qcpm->pps_curr_steps = val;
    else {
        pr_err("use default pps-curr-steps:%d\n", DEFAULT_PPS_CURR_STEPS);
        qcpm->pps_curr_steps = DEFAULT_PPS_CURR_STEPS;
    }
    pr_err("%s: of find property pps-curr-steps:%d\n", __func__, qcpm->pps_curr_steps);

     if (of_property_read_u32(np, "qc,qc-volt-max", &val) >= 0)
        qcpm->qc_volt_max = val;
    else {
        pr_err("use default pps-volt-steps:%d\n", DEFAULT_QC_VOLT_MAX);
        qcpm->qc_volt_max = DEFAULT_QC_VOLT_MAX;
    }
    pr_err("%s: of find property qc-volt-max:%d\n", __func__, qcpm->qc_volt_max);

    if (of_property_read_u32(np, "qc,qc-curr-max", &val) >= 0)
        qcpm->qc_curr_max = val;
    else {
        pr_err("use default qc-curr-max:%d\n", DEFAULT_QC_CURR_MAX);
        qcpm->qc_curr_max = DEFAULT_QC_CURR_MAX;
    }
    pr_err("%s: of find property qc-curr-max:%d\n", __func__, qcpm->qc_curr_max);

    if (of_property_read_u32(np, "qc,batt-curr-boost", &val) >= 0)
        qcpm->batt_curr_boost = val;
    else {
        pr_err("use default batt-curr-boost:%d\n", DEFAULT_BATT_CURR_BOOST);
        qcpm->batt_curr_boost = DEFAULT_BATT_CURR_BOOST;
    }
    pr_err("%s: of find property batt-curr-boost:%d\n", __func__, qcpm->batt_curr_boost);

    if (of_property_read_u32(np, "qc.batt-ovp-limit", &val) >= 0)
        qcpm->batt_ovp_limit = val;
    else {
        pr_err("use default batt-ovp-limits:%d\n", DEFAULT_BATT_OVP_LIMIT);
        qcpm->batt_ovp_limit = DEFAULT_BATT_OVP_LIMIT;
    }
    pr_err("%s: of find property batt-ovp-limits:%d\n", __func__, qcpm->batt_ovp_limit);

    if (of_property_read_u32(np, "qc,pl-chrg-vbatt-min", &val) >= 0)
        qcpm->pl_chrg_vbatt_min = val;
    else {
        pr_err("use default pl-chrg-vbatt-min:%d\n", DEFAULT_PL_CHRG_VBATT_MIN);
        qcpm->pl_chrg_vbatt_min = DEFAULT_PL_CHRG_VBATT_MIN;
    }
    pr_err("%s: of find property pl-chrg-vbatt-min:%d\n", __func__, qcpm->pl_chrg_vbatt_min);

    if (of_property_read_u32(np, "qc,typec-middle-current", &val) >= 0)
        qcpm->typec_middle_current = val;
    else {
        pr_err("use default typec-middle-current:%d\n", DEFAULT_TYPEC_MIDDLE_CURRENT);
        qcpm->typec_middle_current = DEFAULT_TYPEC_MIDDLE_CURRENT;
    }
    pr_err("%s: of find property typec-middle-current:%d\n", __func__, qcpm->typec_middle_current);

    if (of_property_read_u32(np, "qc,step-first-current-comp", &val) >= 0)
        qcpm->step_first_current_comp = val;
    else {
        pr_err("use default qc,step-first-current-comp:%d\n", DEFAULT_STEP_FIRST_CURR_COMP);
        qcpm->step_first_current_comp = DEFAULT_STEP_FIRST_CURR_COMP;
    }
    pr_err("%s: of find property step-first-current-comp:%d\n", __func__, qcpm->step_first_current_comp);

    qcpm->dont_rerun_aicl = of_property_read_bool(np, "qc,dont-rerun-aicl");
    pr_err("%s: of find property dont-rerun-aicl:%d\n", __func__, qcpm->dont_rerun_aicl);

    if (of_property_read_u32(np, "qc,qc-temp-zones-num", &val) >= 0)
        qcpm->qc_temp_zones_num = val;
    else {
        pr_err("use default qc,qc-temp-zones-num:%d\n", DEFAULT_TEMP_ZONES_NUM);
        qcpm->qc_temp_zones_num = DEFAULT_TEMP_ZONES_NUM;
    }
    pr_err("%s: of find property qc-temp-zones-num:%d\n", __func__, qcpm->qc_temp_zones_num);

    qcpm->temp_zones = (struct qc_chrg_step_zone*)kzalloc(sizeof(struct qc_chrg_step_zone) * (qcpm->qc_temp_zones_num), GFP_KERNEL);

    for(idx = 0; idx < qcpm->qc_temp_zones_num; idx++) {
        of_property_read_u32_index(np, "qc,qc-chrg-temp-zones",  5 * idx,     &args[0]);
        of_property_read_u32_index(np, "qc,qc-chrg-temp-zones",  5 * idx + 1, &args[1]);
        of_property_read_u32_index(np, "qc,qc-chrg-temp-zones",  5 * idx + 2, &args[2]);
        of_property_read_u32_index(np, "qc,qc-chrg-temp-zones",  5 * idx + 3, &args[3]);
        of_property_read_u32_index(np, "qc,qc-chrg-temp-zones",  5 * idx + 4, &args[4]);
        qcpm->temp_zones[idx].temp_c = args[0];
        qcpm->temp_zones[idx].chrg_step_power[0].chrg_step_volt = args[1] * 1000;
        qcpm->temp_zones[idx].chrg_step_power[0].chrg_step_curr = args[2] * 1000;
        qcpm->temp_zones[idx].chrg_step_power[1].chrg_step_volt = args[3] * 1000;
        qcpm->temp_zones[idx].chrg_step_power[1].chrg_step_curr = args[4] * 1000;
        pr_err("qc temp zone idx: %d, temp_c :%d, 0_chrg_step_volt:%d, 0_chrg_step_curr:%d, 1_chrg_step_volt:%d, 1_chrg_step_curr:%d\n", idx, args[0], args[1], args[2], args[3], args[4]);
    }

    if (of_property_read_u32(np, "qc,thermal-min-level", &val) >= 0)
        qcpm->thermal_min_level = val;
    else {
        pr_err("use default qc,thermal-min-level:%d\n", DEFAULT_THERMAL_MIN_LEVEL);
        qcpm->thermal_min_level = DEFAULT_THERMAL_MIN_LEVEL;
    }
    pr_err("%s: of find property thermal-min-level:%d\n", __func__, qcpm->thermal_min_level);

    if (of_property_read_u32(np, "qc,sw-charging-curr-limited", &val) >= 0)
        qcpm->sw_charging_curr_limited = val;
    else {
        pr_err("use default qc,sw-charging-curr-limited:%d\n", DEFAULT_SW_CURR_LIMITED);
        qcpm->sw_charging_curr_limited = DEFAULT_SW_CURR_LIMITED;
    }
    pr_err("%s: of find property sw-charging-curr-limited:%d\n", __func__, qcpm->sw_charging_curr_limited);

    if (of_property_read_u32(np, "qc,qc-charging-curr-min", &val) >= 0)
        qcpm->qc_charging_curr_min = val;
    else {
        pr_err("use default qc,charging-curr-min:%d\n", DEFAULT_CHARGING_CURR_MIN);
        qcpm->qc_charging_curr_min = DEFAULT_CHARGING_CURR_MIN;
    }
    pr_err("%s: of find property charging-curr-min:%d\n", __func__, qcpm->qc_charging_curr_min);

    if (of_property_read_u32(np, "qc,pps-volt-comp", &val) >= 0)
        qcpm->pps_volt_comp = val;
    else {
        pr_err("use default pps-volt-comp:%d\n", DEFAULT_PPS_VOLT_COMP);
        qcpm->pps_volt_comp = DEFAULT_PPS_VOLT_COMP;
    }
    pr_err("%s: of find property pps-volt-comp:%d\n", __func__, qcpm->pps_volt_comp);
}

#if 1
static enum power_supply_property usbqc_power_supply_props[] = {
    POWER_SUPPLY_PROP_SYSTEM_THERMAL_LEVEL,
};

static int usbqc_property_is_writeable(struct power_supply *psy,
        enum power_supply_property prop)
{
    switch (prop) {
        case POWER_SUPPLY_PROP_SYSTEM_THERMAL_LEVEL:
            return true;
        default:
            return false;
    }
}

static int usbqc_charger_set_property(struct power_supply *psy,
        enum power_supply_property prop,
        const union power_supply_propval *val)
{
    struct usbqc_pm *qcpm = power_supply_get_drvdata(psy);
    int ret = 0;

    switch (prop) {
        case POWER_SUPPLY_PROP_SYSTEM_THERMAL_LEVEL:
            qcpm->system_thermal_level = val->intval;
            pr_err("%s: set qcpm->system_thermal_level:%d\n", __func__, qcpm->system_thermal_level);
            break;
        default:
            return -EINVAL;
    }

    return ret;
}

static int usbqc_charger_get_property(struct power_supply *psy,
        enum power_supply_property prop,
        union power_supply_propval *val)
{
    struct usbqc_pm *qcpm = power_supply_get_drvdata(psy);
    int ret = 0;

    switch (prop) {
        case POWER_SUPPLY_PROP_SYSTEM_THERMAL_LEVEL:
            val->intval = qcpm->system_thermal_level;
            pr_err("%s: get qcpm->system_thermal_level:%d\n", __func__, qcpm->system_thermal_level);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

static struct power_supply_desc uqbqc_power_supply_desc = {
    .name = "usbqc",
    .type = POWER_SUPPLY_TYPE_UNKNOWN,
    .properties = usbqc_power_supply_props,
    .num_properties = ARRAY_SIZE(usbqc_power_supply_props),
    .get_property = usbqc_charger_get_property,
    .set_property = usbqc_charger_set_property,
    .property_is_writeable = usbqc_property_is_writeable,
};

static int usbqc_power_supply_init(struct usbqc_pm *qcpm,
        struct device *dev)
{
    struct power_supply_config psy_cfg = { .drv_data = qcpm,
        .of_node = dev->of_node, };

    qcpm->qc_psy = power_supply_register(dev,
            &uqbqc_power_supply_desc,
            &psy_cfg);
    if (IS_ERR(qcpm->qc_psy)) {
        pr_err("%s: Failed to register power supply\n", __func__);
        return -EINVAL;
    }

    return 0;
}
#endif

static int qc_charger_probe(struct platform_device *pdev)
{
    struct usbqc_pm *qcpm = NULL;

    pr_err("%s: starts\n", __func__);
    qcpm = kzalloc(sizeof(*qcpm), GFP_KERNEL);
    if (!qcpm)
        return -ENOMEM;

    platform_set_drvdata(pdev, qcpm);
    qcpm->pdev = pdev;

    usbqc_charger_parse_dt(qcpm, &pdev->dev);

    __qcpm = qcpm;

    INIT_DELAYED_WORK(&qcpm->cp_psy_change_work, cp_psy_change_work);
    INIT_DELAYED_WORK(&qcpm->usb_psy_change_work, usb_psy_change_work);

    spin_lock_init(&qcpm->psy_change_lock);
    spin_lock_init(&qcpm->slock);
    mutex_init(&qcpm->charger_lock);

    usbqc_check_cp_psy(qcpm);
    if (qcpm_config.cp_sec_enable) {
        usbqc_check_cp_sec_psy(qcpm);
    }

    usbqc_check_charger_psy(qcpm);
    usbqc_check_pca_chg_swchg(qcpm);

#if 1
    usbqc_power_supply_init(qcpm, &pdev->dev);
#endif

    INIT_DELAYED_WORK(&qcpm->pm_work, usbqc_pm_workfunc);
    qcpm->charger_wakelock = wakeup_source_register(NULL, "charger suspend wakelock");
    qcpm->pps_result = 0;
    qcpm->qc_pps_support = false;
    qcpm->pres_temp_zone = ZONE_NONE;
    qcpm->qc_cc_loop_stage = false;
    qcpm->chrg_step_nums = 2;
    qcpm->system_thermal_level = THERMAL_NOT_LIMIT;
    qcpm->nb.notifier_call = qcpm_psy_notifier_cb;
    power_supply_reg_notifier(&qcpm->nb);
    pcurr_limit = charger_manager_get_by_name(&pdev->dev, "charger");

    set_probe_down_flag(true);
    pr_err("%s: probe successfully\n", __func__);
    return 0;
}

static int qc_charger_remove(struct platform_device *dev)
{
    struct usbqc_pm *qcpm = platform_get_drvdata(dev);

    power_supply_unreg_notifier(&qcpm->nb);
    cancel_delayed_work(&qcpm->pm_work);
    cancel_delayed_work(&qcpm->cp_psy_change_work);
    cancel_delayed_work(&qcpm->usb_psy_change_work);
    return 0;
}

static void qc_charger_shutdown(struct platform_device *dev)
{

}

static const struct of_device_id mtk_charger_of_match[] = {
    {.compatible = "qc,chrg-manager",},
    {},
};

MODULE_DEVICE_TABLE(of, mtk_charger_of_match);

struct platform_device mtk_charger_device = {
    .name = "chrg-manager",
    .id = -1,
};

static struct platform_driver mtk_qc3p_driver = {
    .probe = qc_charger_probe,
    .remove = qc_charger_remove,
    .shutdown = qc_charger_shutdown,
    .driver = {
               .name = "chrg-manager",
               .of_match_table = mtk_charger_of_match,
    },
};

static int __init charger_qc_policy_init(void)
{
    pr_err("%s: start\n", __func__);
    return platform_driver_register(&mtk_qc3p_driver);
}
late_initcall(charger_qc_policy_init);

static void __exit charger_qc_policy_exit(void)
{
    platform_driver_unregister(&mtk_qc3p_driver);
}
module_exit(charger_qc_policy_exit);

MODULE_AUTHOR("chan.miao <chan.miao@huaqin@huaqin.com>");
MODULE_DESCRIPTION("Huaqin QC3+ Charger Driver");
MODULE_LICENSE("GPL");


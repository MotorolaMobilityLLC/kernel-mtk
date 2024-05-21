/*
 *  charger_policy.h
 *
 *  Created on: Mar 27, 2017
 *      Author: a0220433
 */

#ifndef SRC_PDLIB_USB_PD_POLICY_MANAGER_H_
#define SRC_PDLIB_USB_PD_POLICY_MANAGER_H_
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/usb/tcpm.h>
#include <mt-plat/v1/mtk_charger.h>
#include <mt-plat/v1/charger_class.h>

enum pm_sm_state_t{
    PM_STATE_DISCONNECT,
    PM_STATE_ENTRY,
    PM_STATE_SW_ENTRY,
    PM_STATE_SW_LOOP,
    PM_STATE_CHRG_PUMP_ENTRY,
    PM_STATE_SINGLE_CP_ENTRY,
    PM_STATE_PPS_TUNNING_CURR,
    PM_STATE_PPS_TUNNING_VOLT,
    PM_STATE_CP_CC_LOOP,
    PM_STATE_CP_CV_LOOP,
    PM_STATE_CP_QUIT,
    PM_STATE_RECOVERY_SW,
    PM_STATE_STOP_CHARGE,
    PM_STATE_COOLING_LOOP,
    PM_STATE_POWER_LIMIT_LOOP,
};

static const unsigned char *pm_sm_state_str[] = {
    "PM_STATE_DISCONNECT",
    "PM_STATE_ENTRY",
    "PM_STATE_SW_ENTRY",
    "PM_STATE_SW_LOOP",
    "PM_STATE_CHRG_PUMP_ENTRY",
    "PM_STATE_SINGLE_CP_ENTRY",
    "PM_STATE_PPS_TUNNING_CURR",
    "PM_STATE_PPS_TUNNING_VOLT",
    "PM_STATE_CP_CC_LOOP",
    "PM_STATE_CP_CV_LOOP",
    "PM_STATE_CP_QUIT",
    "PM_STATE_RECOVERY_SW",
    "PM_STATE_STOP_CHARGE",
    "PM_STATE_COOLING_LOOP",
    "PM_STATE_POWER_LIMIT_LOOP",
};

enum temp_zone{
    ZONE_COLD,
    ZONE_FIRST = 0,

    ZONE_HOT = 10,
    ZONE_NONE,
};

enum {
    BLANCE_POWER,
    RESET_POWER,
};


#define MIN_TEMP_C      0
#define HYSTERESIS_DEGC 2

#define BAT_OVP_FAULT_SHIFT         0
#define BAT_OCP_FAULT_SHIFT         1
#define BUS_OVP_FAULT_SHIFT         2
#define BUS_OCP_FAULT_SHIFT         3
#define BAT_THERM_FAULT_SHIFT           4
#define BUS_THERM_FAULT_SHIFT           5
#define DIE_THERM_FAULT_SHIFT           6

#define BAT_OVP_FAULT_MASK      (1 << BAT_OVP_FAULT_SHIFT)
#define BAT_OCP_FAULT_MASK      (1 << BAT_OCP_FAULT_SHIFT)
#define BUS_OVP_FAULT_MASK      (1 << BUS_OVP_FAULT_SHIFT)
#define BUS_OCP_FAULT_MASK      (1 << BUS_OCP_FAULT_SHIFT)
#define BAT_THERM_FAULT_MASK        (1 << BAT_THERM_FAULT_SHIFT)
#define BUS_THERM_FAULT_MASK        (1 << BUS_THERM_FAULT_SHIFT)
#define DIE_THERM_FAULT_MASK        (1 << DIE_THERM_FAULT_SHIFT)

#define BAT_OVP_ALARM_SHIFT         0
#define BAT_OCP_ALARM_SHIFT         1
#define BUS_OVP_ALARM_SHIFT         2
#define BUS_OCP_ALARM_SHIFT         3
#define BAT_THERM_ALARM_SHIFT           4
#define BUS_THERM_ALARM_SHIFT           5
#define DIE_THERM_ALARM_SHIFT           6
#define BAT_UCP_ALARM_SHIFT         7

#define BAT_OVP_ALARM_MASK      (1 << BAT_OVP_ALARM_SHIFT)
#define BAT_OCP_ALARM_MASK      (1 << BAT_OCP_ALARM_SHIFT)
#define BUS_OVP_ALARM_MASK      (1 << BUS_OVP_ALARM_SHIFT)
#define BUS_OCP_ALARM_MASK      (1 << BUS_OCP_ALARM_SHIFT)
#define BAT_THERM_ALARM_MASK        (1 << BAT_THERM_ALARM_SHIFT)
#define BUS_THERM_ALARM_MASK        (1 << BUS_THERM_ALARM_SHIFT)
#define DIE_THERM_ALARM_MASK        (1 << DIE_THERM_ALARM_SHIFT)
#define BAT_UCP_ALARM_MASK      (1 << BAT_UCP_ALARM_SHIFT)

#define VBAT_REG_STATUS_SHIFT           0
#define IBAT_REG_STATUS_SHIFT           1

#define VBAT_REG_STATUS_MASK        (1 << VBAT_REG_STATUS_SHIFT)
#define IBAT_REG_STATUS_MASK        (1 << VBAT_REG_STATUS_SHIFT)


#define DEFAULT_PPS_VOLT_STEPS           20000
#define DEFAULT_PPS_CURR_STEPS           40000
#define DEFAULT_QC_VOLT_MAX              12000000
#define DEFAULT_QC_CURR_MAX              3000000
#define DEFAULT_BATT_CURR_BOOST          300000
#define DEFAULT_BATT_OVP_LIMIT           4550000
#define DEFAULT_TYPEC_MIDDLE_CURRENT     1500000
#define DEFAULT_PL_CHRG_VBATT_MIN        3600000
#define DEFAULT_STEP_FIRST_CURR_COMP     0
#define DEFAULT_TEMP_ZONES_NUM           5
#define DEFAULT_THERMAL_MIN_LEVEL        1500000
#define DEFAULT_CHARGING_CURR_MIN        1500000
#define DEFAULT_SW_CURR_LIMITED          500000
#define DEFAULT_PPS_VOLT_COMP            500000
#define CV_TAPPER_COUNT                  3
#define CV_DELTA_VOLT                    100000
#define QC3P_V_STEP                      20000
#define CP_CHRG_SOC_LIMIT                100
#define THERMAL_TUNNING_CURR             40000
#define THERMAL_NOT_LIMIT                -1

#define CC_CURR_DEBOUNCE                 20000
#define HEARTBEAT_NEXT_STATE_MS          1000
#define THERMAL_DELAY_MS                 300
#define THERMAL_DELAY_LESS_MS            100
#define HEARTBEAT_SHORT_DELAY_MS         100
#define HEARTBEAT_SW_DELAY_MS            3000
#define STEP_FIRST                       0

#define PPS_RET_HISTORY_SIZE             5
#define TEMP_ZONE_NUMS 5
struct qc_chrg_step_power
{
    int chrg_step_curr;
    int chrg_step_volt;
};

struct qc_chrg_step_zone
{
    int temp_c;
    struct qc_chrg_step_power chrg_step_power[2];
};

struct qc_chrg_step_info
{
    int temp_c;
    int chrg_step_cv_tapper_curr;
    int chrg_step_cc_curr;
    int chrg_step_cv_volt;
    int pres_chrg_step;
    bool last_step;
};

struct sw_device {
    bool charge_enabled;
    int  ibus_curr;
};

struct cp_device {
    bool charge_enabled;

    bool batt_pres;
    bool vbus_pres;

    /* alarm/fault status */
    bool bat_ovp_fault;
    bool bat_ocp_fault;
    bool bus_ovp_fault;
    bool bus_ocp_fault;

    bool bat_ovp_alarm;
    bool bat_ocp_alarm;
    bool bus_ovp_alarm;
    bool bus_ocp_alarm;

    bool bat_ucp_alarm;

    bool bat_therm_alarm;
    bool bus_therm_alarm;
    bool die_therm_alarm;

    bool bat_therm_fault;
    bool bus_therm_fault;
    bool die_therm_fault;

    bool therm_shutdown_flag;
    bool therm_shutdown_stat;

    bool vbat_reg;
    bool ibat_reg;

    int  vout_volt;
    int  vbat_volt;
    int  vbus_volt;
    int  ibat_curr;
    int  ibus_curr;

    int vbus_error_low;
    int vbus_error_high;

    int  bat_temp;
    int  bus_temp;
    int  die_temp;
};

struct usbqc_pm {
    struct tcpc_device *tcpc;
    struct notifier_block tcp_nb;
    struct charger_device *sw_chg;
    struct platform_device *pdev;

    u32 batt_curr_boost;
    u32 batt_ovp_limit;
    u32 step_first_current_comp;
    u32 dont_rerun_aicl;

    struct qc_chrg_step_zone *temp_zones;

    u32 thermal_min_level;
    u32 sw_charging_curr_limited;

    bool is_pps_en_unlock;
    int hrst_cnt;

    struct cp_device cp;
    struct cp_device cp_sec;
    struct sw_device sw;

    bool   cp_sec_stopped;

    bool qc_active;
    bool qc_pps_support;

    int qc_volt_max;
    int qc_curr_max;

    struct delayed_work pm_work;
    struct notifier_block nb;

    bool   psy_change_running;
    struct delayed_work cp_psy_change_work;
    struct delayed_work usb_psy_change_work;

    spinlock_t psy_change_lock;
    struct mutex charger_lock;
    spinlock_t slock;

    int vbus_present;
    int qc_request_curr;
    int qc_request_volt;
    int pps_volt_comp;
    int typec_middle_current;
    int pres_temp_zone;
    int pps_volt_steps;
    int pps_curr_steps;
    int batt_therm_cooling_cnt;
    bool sm_work_running;
    enum pm_sm_state_t state;
    int batt_ovp_lmt;
    int qc_pps_balance;
    int qc_charging_curr_min;
    int qc_request_volt_prev;
    int qc_request_curr_prev;

    int qc_target_volt;
    int qc_target_curr;

    int qc_sys_therm_volt;
    int qc_sys_therm_curr;
    bool sys_therm_cooling;
    bool batt_therm_cooling;
    bool batt_thermal_cooling;
    int system_thermal_level;
    int pps_result;
    int qc_vbatt_volt_prev;
    int qc_ibatt_curr_prev;
    int qc_therm_loop_cn;
    bool qc_cc_loop_stage;

    int pl_chrg_vbatt_min;
    u32 qc_temp_zones_num;
    u32 chrg_step_nums;
    struct qc_chrg_step_info chrg_step;

    int pps_result_history_idx;
    int pps_result_history_history[PPS_RET_HISTORY_SIZE];

    struct wakeup_source *charger_wakelock;

    struct power_supply *qc_psy;
    struct power_supply *cp_psy;
    struct power_supply *cp_sec_psy;
    struct power_supply *usb_psy;
    struct power_supply *batt_psy;
};

struct qcpm_config {
    int bat_volt_lp_lmt; /*bat volt loop limit*/
    int bat_curr_lp_lmt;
    int bus_volt_lp_lmt;
    int bus_curr_lp_lmt;

    int fc2_taper_current;
    int fc2_steps;

    int min_adapter_volt_required;
    int min_adapter_curr_required;

    int min_vbat_for_cp;

    bool cp_sec_enable;
    bool fc2_disable_sw;        /* disable switching charger during flash charge*/

};

#endif /* SRC_PDLIB_USB_QC_POLICY_H_ */


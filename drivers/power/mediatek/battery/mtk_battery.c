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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    mtk_battery.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines functions of the Anroid Battery service for updating the battery status
 *
 * Author:
 * -------
 * Weiching Lin
 *
 ****************************************************************************/
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/wait.h>		/* For wait queue*/
#include <linux/sched.h>	/* For wait queue*/
#include <linux/kthread.h>	/* For Kthread_run */
#include <linux/platform_device.h>	/* platform device */
#include <linux/time.h>
#include <linux/wakelock.h>

#include <linux/netlink.h>	/* netlink */
#include <linux/kernel.h>
#include <linux/socket.h>	/* netlink */
#include <linux/skbuff.h>	/* netlink */
#include <net/sock.h>		/* netlink */
#include <linux/cdev.h>		/* cdev */

#include <linux/err.h>	/* IS_ERR, PTR_ERR */
#include <linux/reboot.h>	/*kernel_power_off*/
#include <linux/proc_fs.h>

#include <linux/vmalloc.h>

#include <mt-plat/charger_type.h>
#include <mt-plat/mtk_charger.h>
#include <mt-plat/mtk_battery.h>
#include <mach/mtk_battery_property.h>
#include <mach/mtk_battery_table.h>	/* BATTERY_PROFILE_STRUCT */
#include <mt-plat/upmu_common.h> /*pmic & ptim */
#include <mt-plat/mtk_auxadc_intf.h>
#include <mt-plat/mtk_boot.h>
#include <linux/of_fdt.h>	/*of_dt API*/

#include <mtk_gauge_class.h>
#include "mtk_battery_internal.h"

/* ============================================================ */
/* define */
/* ============================================================ */
#define NETLINK_FGD 26
#define FGD_CHECK_VERSION 0x100001

#define BAT_PSEUDO_THREAD

/* ============================================================ */
/* global variable */
/* ============================================================ */
#ifdef BAT_PSEUDO_THREAD
static struct hrtimer fg_drv_thread_hrtimer;
#endif
static DECLARE_WAIT_QUEUE_HEAD(fg_update_wq);
struct PMU_ChargerStruct BMT_status;
struct gauge_hw_status FG_status;
static struct BAT_EC_Struct Bat_EC_ctrl;

static unsigned int fg_update_flag;
static struct sock *daemo_nl_sk;
static u_int g_fgd_pid;
static unsigned int g_fgd_version = -1;
static bool init_flag;
static int BAT_EC_cmd;
static int BAT_EC_param;

static int fg_bat_int1_gap;
static int fg_bat_int1_ht = 0xffff;
static int fg_bat_int1_lt = 0xffff;

static int fg_bat_int2_ht = 0xffff;
static int fg_bat_int2_lt = 0xffff;
static int fg_bat_int2_ht_en = 0xffff;
static int fg_bat_int2_lt_en = 0xffff;

static int fg_bat_tmp_int_gap;
static int fg_bat_tmp_c_int_gap;
static int fg_bat_tmp_ht = 0xffff;
static int fg_bat_tmp_lt = 0xffff;
static int fg_bat_tmp_c_ht = 0xffff;
static int fg_bat_tmp_c_lt = 0xffff;
static int fg_bat_tmp_int_ht = 0xffff;
static int fg_bat_tmp_int_lt = 0xffff;

static int bat_cycle_thr;

static signed int ptim_lk_v;
static signed int ptim_lk_i;
static int pl_bat_vol;
static int pl_shutdown_time;
static u32 pl_two_sec_reboot;


static struct gtimer tracking_timer;

static struct gauge_consumer coulomb_plus;
static struct gauge_consumer coulomb_minus;
static struct gauge_consumer soc_plus;
static struct gauge_consumer soc_minus;

struct timespec sw_iavg_time;
int sw_iavg_car;
int sw_iavg;

#ifdef GAUGE_COULOMB_INTERRUPT_TEST
static struct gauge_consumer coulomb_test1;
static struct gauge_consumer coulomb_test2;
static struct gauge_consumer coulomb_test3;
static struct gauge_consumer coulomb_test4;

int coulomb_test1_handler(struct gauge_consumer *consumer)
{
	gauge_coulomb_start(&coulomb_test1, -5);
	gauge_coulomb_start(&coulomb_test2, -10);
	return 0;
}
#endif

#ifdef GAUGE_TIMER_INTERRUPT_TEST
static struct gtimer g1, g2, g3, g4, g5;
#endif

static bool gDisableGM30;
static bool ntc_disable_nafg;

static struct gauge_device *gauge_dev;

bool is_fg_disable(void)
{
	return gDisableGM30;
}


static int Enable_BATDRV_LOG = 3;	/* Todo: charging.h use it, should removed */

static bool is_init_done;

static bool is_reset_battery_cycle;
static int fixed_bat_tmp = 0xffff;
static struct charger_consumer *pbat_consumer;
static struct notifier_block bat_nb;
static DEFINE_MUTEX(bat_mutex);
/************ adc_cali *******************/
#define ADC_CALI_DEVNAME "MT_pmic_adc_cali"
#define TEST_ADC_CALI_PRINT _IO('k', 0)
#define SET_ADC_CALI_Slop _IOW('k', 1, int)
#define SET_ADC_CALI_Offset _IOW('k', 2, int)
#define SET_ADC_CALI_Cal _IOW('k', 3, int)
#define ADC_CHANNEL_READ _IOW('k', 4, int)
#define BAT_STATUS_READ _IOW('k', 5, int)
#define Set_Charger_Current _IOW('k', 6, int)
/* add for meta tool----------------------------------------- */
#define Get_META_BAT_VOL _IOW('k', 10, int)
#define Get_META_BAT_SOC _IOW('k', 11, int)
#define Get_META_BAT_CAR_TUNE_VALUE _IOW('k', 12, int)
#define Set_META_BAT_CAR_TUNE_VALUE _IOW('k', 13, int)
/* add for meta tool----------------------------------------- */

static struct class *adc_cali_class;
static int adc_cali_major;
static dev_t adc_cali_devno;
static struct cdev *adc_cali_cdev;

static int adc_cali_slop[14] = {
	1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000
};
static int adc_cali_offset[14] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int adc_cali_cal[1] = { 0 };
static int battery_in_data[1] = { 0 };
static int battery_out_data[1] = { 0 };
static bool g_ADC_Cali;

static signed int gFG_daemon_log_level = BM_DAEMON_DEFAULT_LOG_LEVEL;

static enum power_supply_property battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	/* Add for Battery Service */
	POWER_SUPPLY_PROP_batt_vol,
	POWER_SUPPLY_PROP_batt_temp,
	/* Add for EM */
	POWER_SUPPLY_PROP_TemperatureR,
	POWER_SUPPLY_PROP_TempBattVoltage,
	POWER_SUPPLY_PROP_InstatVolt,
	POWER_SUPPLY_PROP_BatteryAverageCurrent,
	POWER_SUPPLY_PROP_BatterySenseVoltage,
	POWER_SUPPLY_PROP_ISenseVoltage,
	POWER_SUPPLY_PROP_ChargerVoltage,
	/* Dual battery */
	POWER_SUPPLY_PROP_status_smb,
	POWER_SUPPLY_PROP_capacity_smb,
	POWER_SUPPLY_PROP_present_smb,
	/* ADB CMD Discharging */
	POWER_SUPPLY_PROP_adjust_power,
};

bool is_battery_init_done(void)
{
	return is_init_done;
}

/* ============================================================ */
/* gauge hal interface */
/* ============================================================ */

bool gauge_get_current(int *bat_current)
{
	bool is_charging = false;

#if defined(CONFIG_POWER_EXT)
	*bat_current = 0;
#else
	if (Bat_EC_ctrl.debug_fg_curr_en == 1) {
		*bat_current = Bat_EC_ctrl.debug_fg_curr_value;
		return false;
	}

	if (is_battery_init_done() == false) {
		*bat_current = 0;
		return false;
	}

	gauge_dev_get_current(gauge_dev, &is_charging, bat_current);
#endif
	return is_charging;
}

int gauge_get_average_current(bool *valid)
{
	int iavg = 0;

#if defined(CONFIG_POWER_EXT)
	iavg = 0;
#else
	gauge_dev_get_average_current(gauge_dev, &iavg, valid);
#endif
	return iavg;
}

int gauge_get_coulomb(void)
{
	int columb = 0;
#if defined(CONFIG_POWER_EXT)
#else
	gauge_dev_get_coulomb(gauge_dev, &columb);
#endif
	return columb;
}

int gauge_reset_hw(void)
{
#if defined(CONFIG_POWER_EXT)
#else
	gauge_coulomb_before_reset();
	gauge_dev_reset_hw(gauge_dev);
	gauge_coulomb_after_reset();
	get_monotonic_boottime(&sw_iavg_time);
	sw_iavg_car = gauge_get_coulomb();
#endif
	return 0;
}

int gauge_get_hwocv(void)
{
	int hwocv = 37000;
#if defined(CONFIG_POWER_EXT)
	hwocv = 37000;
#else
	gauge_dev_get_hwocv(gauge_dev, &hwocv);
#endif
	return hwocv;
}

int gauge_set_coulomb_interrupt1_ht(int car)
{
	return gauge_dev_set_coulomb_interrupt1_ht(gauge_dev, car);
}

int gauge_set_coulomb_interrupt1_lt(int car)
{
	return gauge_dev_set_coulomb_interrupt1_lt(gauge_dev, car);
}

int gauge_get_ptim_current(int *ptim_current, bool *is_charging)
{
	gauge_dev_get_ptim_current(gauge_dev, ptim_current, is_charging);
	return 0;
}

int gauge_get_zcv_current(int *zcv_current)
{
	gauge_dev_get_zcv_current(gauge_dev, zcv_current);
	return 0;
}

int gauge_get_zcv(int *zcv)
{
	gauge_dev_get_zcv(gauge_dev, zcv);
	return 0;
}

int gauge_set_nag_en(int nafg_zcv_en)
{
	gauge_dev_enable_nag_interrupt(gauge_dev, nafg_zcv_en);
	return 0;
}

int gauge_set_zcv_interrupt_en(int zcv_intr_en)
{
	gauge_dev_enable_zcv_interrupt(gauge_dev, zcv_intr_en);
	return 0;
}

int gauge_get_hw_version(void)
{
	return gauge_dev_get_hw_version(gauge_dev);
}

/* ============================================================ */
/* external interface */
/* ============================================================ */
static int ptim_vol;
static int ptim_curr;

static void _do_ptim(void)
{
	int ret;
	bool is_charging = false;

	ret = do_ptim_gauge(false, &ptim_vol, &ptim_curr, &is_charging);

	if ((is_charging == false) && (ptim_curr >= 0))
		ptim_curr = 0 - ptim_curr;
}

static int _get_ptim_bat_vol(void)
{
	int vbat;

	if (Bat_EC_ctrl.debug_ptim_v_en == 1)
		vbat = Bat_EC_ctrl.debug_ptim_v_value;
	else
		vbat = ptim_vol;

	return vbat;
}

static int _get_ptim_R_curr(void)
{
	int cur;

	if (Bat_EC_ctrl.debug_ptim_r_en == 1)
		cur = Bat_EC_ctrl.debug_ptim_r_value;
	else
		cur = ptim_curr;

	return cur;
}

static int _get_ptim_rac_val(void)
{
	int rac;

	if (Bat_EC_ctrl.debug_rac_en == 1)
		rac = Bat_EC_ctrl.debug_rac_value;
	else
		rac = get_rac();

	return rac;
}



/* ============================================================ */
/* functions */
/* ============================================================ */

static void disable_fg(void)
{
	pmic_enable_interrupt(FG_BAT1_INT_L_NO, 0, "GM30");
	pmic_enable_interrupt(FG_BAT1_INT_H_NO, 0, "GM30");

	pmic_enable_interrupt(FG_BAT0_INT_L_NO, 0, "GM30");
	pmic_enable_interrupt(FG_BAT0_INT_H_NO, 0, "GM30");

	pmic_enable_interrupt(FG_N_CHARGE_L_NO, 0, "GM30");

	pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");

	pmic_enable_interrupt(FG_ZCV_NO, 0, "GM30");

	pmic_enable_interrupt(FG_BAT_PLUGOUT_NO, 0, "GM30");
	pmic_enable_interrupt(FG_RG_INT_EN_NAG_C_DLTV, 0, "GM30");

	pmic_enable_interrupt(FG_RG_INT_EN_BAT_TEMP_H, 0, "GM30");
	pmic_enable_interrupt(FG_RG_INT_EN_BAT_TEMP_L, 0, "GM30");

	pmic_enable_interrupt(FG_RG_INT_EN_BAT2_H, 0, "GM30");
	pmic_enable_interrupt(FG_RG_INT_EN_BAT2_L, 0, "GM30");
	gDisableGM30 = 1;
}

signed int battery_meter_get_tempR(signed int dwVolt)
{
#if defined(CONFIG_POWER_EXT)
	return 0;
#else
	int TRes;

	TRes = (RBAT_PULL_UP_R * dwVolt) / (RBAT_PULL_UP_VOLT - dwVolt);

	return TRes;
#endif
}

signed int battery_meter_get_tempV(void)
{
#if defined(CONFIG_POWER_EXT)
	return 0;
#else
	int val = 0;

	val = 1;		/* set avg times */
	val = pmic_get_v_bat_temp();
	return val;
#endif
}

signed int battery_meter_get_VSense(void)
{
#if defined(CONFIG_POWER_EXT)
	return 0;
#else
	return pmic_get_ibus();
#endif
}

void battery_update_psd(struct battery_data *bat_data)
{
	bat_data->BAT_batt_vol = battery_get_bat_voltage();
	bat_data->BAT_InstatVolt = bat_data->BAT_batt_vol;
	bat_data->BAT_BatterySenseVoltage = bat_data->BAT_batt_vol;
	bat_data->BAT_batt_temp = battery_get_bat_temperature();
	bat_data->BAT_TempBattVoltage = battery_meter_get_tempV();
	bat_data->BAT_TemperatureR = battery_meter_get_tempR(bat_data->BAT_TempBattVoltage);
	bat_data->BAT_BatteryAverageCurrent = battery_get_ibus();
	bat_data->BAT_ISenseVoltage = battery_meter_get_VSense();
	bat_data->BAT_ChargerVoltage = battery_get_vbus();
}

static int battery_get_property(struct power_supply *psy,
				enum power_supply_property psp, union power_supply_propval *val)
{
	int ret = 0;
	struct battery_data *data = container_of(psy->desc, struct battery_data, psd);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = data->BAT_STATUS;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = data->BAT_HEALTH;/* do not change before*/
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = data->BAT_PRESENT;/* do not change before*/
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = data->BAT_TECHNOLOGY;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = data->BAT_CAPACITY;
		break;
	case POWER_SUPPLY_PROP_batt_vol:
		val->intval = data->BAT_batt_vol;
		break;
	case POWER_SUPPLY_PROP_batt_temp:
		val->intval = data->BAT_batt_temp * 10;
		break;
	case POWER_SUPPLY_PROP_TemperatureR:
		val->intval = data->BAT_TemperatureR;
		break;
	case POWER_SUPPLY_PROP_TempBattVoltage:
		val->intval = data->BAT_TempBattVoltage;
		break;
	case POWER_SUPPLY_PROP_InstatVolt:
		val->intval = data->BAT_InstatVolt;
		break;
	case POWER_SUPPLY_PROP_BatteryAverageCurrent:
		val->intval = data->BAT_BatteryAverageCurrent;
		break;
	case POWER_SUPPLY_PROP_BatterySenseVoltage:
		val->intval = data->BAT_BatterySenseVoltage;
		break;
	case POWER_SUPPLY_PROP_ISenseVoltage:
		val->intval = data->BAT_ISenseVoltage;
		break;
	case POWER_SUPPLY_PROP_ChargerVoltage:
		val->intval = data->BAT_ChargerVoltage;
		break;
		/* Dual battery */
	case POWER_SUPPLY_PROP_status_smb:
		val->intval = data->status_smb;
		break;
	case POWER_SUPPLY_PROP_capacity_smb:
		val->intval = data->capacity_smb;
		break;
	case POWER_SUPPLY_PROP_present_smb:
		val->intval = data->present_smb;
		break;
	case POWER_SUPPLY_PROP_adjust_power:
		val->intval = data->adjust_power;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* battery_data initialization */
static struct battery_data battery_main = {
	.psd = {
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = battery_props,
		.num_properties = ARRAY_SIZE(battery_props),
		.get_property = battery_get_property,
		},

#if defined(CONFIG_POWER_EXT)
	.BAT_STATUS = POWER_SUPPLY_STATUS_FULL,
	.BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD,
	.BAT_PRESENT = 1,
	.BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION,
	.BAT_CAPACITY = 100,
	.BAT_batt_vol = 4200,
	.BAT_batt_temp = 22,
	/* Dual battery */
	.status_smb = POWER_SUPPLY_STATUS_NOT_CHARGING,
	.capacity_smb = 50,
	.present_smb = 0,
	/* ADB CMD discharging */
	.adjust_power = -1,
#else
	.BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING,
	.BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD,
	.BAT_PRESENT = 1,
	.BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION,
	.BAT_CAPACITY = -1,
	.BAT_batt_vol = 0,
	.BAT_batt_temp = 0,
	/* Dual battery */
	.status_smb = POWER_SUPPLY_STATUS_NOT_CHARGING,
	.capacity_smb = 50,
	.present_smb = 0,
	/* ADB CMD discharging */
	.adjust_power = -1,
#endif
};

static void battery_update(struct battery_data *bat_data)
{
	struct power_supply *bat_psy = bat_data->psy;

	battery_update_psd(&battery_main);
	bat_data->BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION;
	bat_data->BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD;
	bat_data->BAT_PRESENT = 1;

#if defined(CONFIG_MTK_DISABLE_GAUGE)
	return;
#endif

#if defined(CONFIG_POWER_EXT)
	bat_data->BAT_CAPACITY = 50;
#endif
	power_supply_changed(bat_psy);
}

unsigned int bat_is_kpoc(void)
{
#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	int boot_mode = get_boot_mode();

	if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
	    || boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
		return true;
	}
#endif
	return false;
}

int get_hw_ocv(void)
{
	return FG_status.hw_ocv;
}

int get_sw_ocv(void)
{
	return FG_status.sw_ocv;
}

void set_hw_ocv_unreliable(bool _flag_unreliable)
{
	FG_status.flag_hw_ocv_unreliable = _flag_unreliable;
}

int bat_get_debug_level(void)
{
	return Enable_BATDRV_LOG;
}

/* ============================================================ */
/* function prototype */
/* ============================================================ */
static void nl_send_to_user(u32 pid, int seq, struct fgd_nl_msg_t *reply_msg);

struct fuel_gauge_custom_data fg_cust_data;
struct fuel_gauge_table_custom_data fg_table_cust_data;


/* ============================================================ */
/* extern function */
/* ============================================================ */

static int proc_cmd_id;
static unsigned int proc_subcmd;
static unsigned int proc_subcmd_para1;
static char proc_log[4096];

static void proc_dump_log(struct seq_file *m)
{
	seq_printf(m, "subcmd:%d para1:%d\n", proc_subcmd, proc_subcmd_para1);
	seq_printf(m, "%s\n", proc_log);
}

static void proc_dump_dtsi(struct seq_file *m)
{
	seq_puts(m, "********** dump DTSI **********\n");
	seq_printf(m, "g_FG_PSEUDO100_T0 = %d\n", fg_cust_data.pseudo100_t0);
	seq_printf(m, "g_FG_PSEUDO100_T1 = %d\n", fg_cust_data.pseudo100_t1);
	seq_printf(m, "g_FG_PSEUDO100_T2 = %d\n", fg_cust_data.pseudo100_t2);
	seq_printf(m, "g_FG_PSEUDO100_T3 = %d\n", fg_cust_data.pseudo100_t3);
	seq_printf(m, "g_FG_PSEUDO100_T4 = %d\n", fg_cust_data.pseudo100_t4);

	seq_printf(m, "DIFFERENCE_FULLOCV_ITH = %d\n", fg_cust_data.difference_fullocv_ith);
	seq_printf(m, "Q_MAX_SYS_VOLTAGE_BAT = %d\n", fg_cust_data.q_max_sys_voltage);

	seq_printf(m, "SHUTDOWN_1_TIME = %d\n", fg_cust_data.shutdown_1_time);
	seq_printf(m, "KEEP_100_PERCENT = %d\n", fg_cust_data.keep_100_percent);
	seq_printf(m, "R_FG_VALUE = %d\n", fg_cust_data.r_fg_value);
	seq_printf(m, "TEMPERATURE_T0 = %d\n", fg_cust_data.temperature_t0);
	seq_printf(m, "TEMPERATURE_T1 = %d\n", fg_cust_data.temperature_t1);
	seq_printf(m, "TEMPERATURE_T2 = %d\n", fg_cust_data.temperature_t2);
	seq_printf(m, "TEMPERATURE_T3 = %d\n", fg_cust_data.temperature_t3);
	seq_printf(m, "TEMPERATURE_T4 = %d\n", fg_cust_data.temperature_t4);

	seq_printf(m, "EMBEDDED_SEL = %d\n", fg_cust_data.embedded_sel);
	seq_printf(m, "PMIC_SHUTDOWN_CURRENT = %d\n", fg_cust_data.pmic_shutdown_current);
	seq_printf(m, "FG_METER_RESISTANCE = %d\n", fg_cust_data.fg_meter_resistance);
	seq_printf(m, "CAR_TUNE_VALUE = %d\n", fg_cust_data.car_tune_value);

	seq_printf(m, "pl_two_sec_reboot = %d\n", pl_two_sec_reboot);
#ifdef SHUTDOWN_CONDITION_LOW_BAT_VOLT
	seq_puts(m, "SHUTDOWN_CONDITION_LOW_BAT_VOLT = 1\n");
	seq_printf(m, "lbat_def: %d %d %d\n", VBAT2_DET_VOLTAGE1, VBAT2_DET_VOLTAGE2, VBAT2_DET_VOLTAGE3);
	seq_printf(m, "lbat: %d %d %d\n", fg_cust_data.vbat2_det_voltage1,
		fg_cust_data.vbat2_det_voltage2, fg_cust_data.vbat2_det_voltage3);
#else
	seq_puts(m, "SHUTDOWN_CONDITION_LOW_BAT_VOLT = 0\n");
#endif

#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
	seq_puts(m, "CONFIG_MTK_ADDITIONAL_BATTERY_TABLE is defined\n");
#endif
	seq_printf(m, "hw_version = %d\n", gauge_get_hw_version());


}

static void dump_kernel_table(struct seq_file *m)
{
	int i;

	seq_printf(m, "table size:%d\n", fg_table_cust_data.fg_profile_t0_size);
	seq_puts(m, "idx: maH, voltage, R, percentage\n");
	for (i = 0; i < 100; i++) {
		seq_printf(m, "%d: %d %d %d %d\n",
			i,
			fg_table_cust_data.fg_profile_t0[i].mah,
			fg_table_cust_data.fg_profile_t0[i].voltage,
			fg_table_cust_data.fg_profile_t0[i].resistance,
			fg_table_cust_data.fg_profile_t0[i].percentage);
	}
}


static int proc_dump_log_show(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "********** Gauge Dump **********\n");

	seq_puts(m, "Command Table list\n");
	seq_puts(m, "0: dump dtsi\n");
	seq_puts(m, "1: dump v-mode table\n");
	seq_puts(m, "2: dump gauge hw register\n");
	seq_puts(m, "3: kernel table\n");
	seq_puts(m, "4: send CHR FULL\n");
	seq_printf(m, "current command:%d\n", proc_cmd_id);

	switch (proc_cmd_id) {
	case 0:
		proc_dump_dtsi(m);
		break;
	case 1:
		wakeup_fg_algo_cmd(FG_INTR_KERNEL_CMD, FG_KERNEL_CMD_DUMP_LOG, 1);
		for (i = 0; i < 5; i++) {
			msleep(500);
			if (proc_subcmd_para1 == 1)
				break;
		}
		proc_dump_log(m);

		break;

	case 2:
		gauge_dev_dump(gauge_dev, m);
		break;
	case 3:
		dump_kernel_table(m);
		break;
	case 4:
		wakeup_fg_algo(FG_INTR_CHR_FULL);
		break;
	default:
		seq_printf(m, "do not support command:%d\n", proc_cmd_id);
		wakeup_fg_algo_cmd(FG_INTR_KERNEL_CMD, FG_KERNEL_CMD_DUMP_LOG, proc_cmd_id);
		for (i = 0; i < 5; i++) {
			msleep(500);
			if (proc_subcmd_para1 == proc_cmd_id)
				break;
		}
		proc_dump_log(m);

		break;
	}



	/*battery_dump_info(m);*/

	return 0;
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *f_pos)
{
	int cmd = 0;
	char num[10];

	memset(num, 0, 10);

	bm_err("proc_write %s %d\n", buffer, (int)count);

	if (!count)
		return 0;

	if (count > (sizeof(num) - 1))
		return -EINVAL;

	if (copy_from_user(num, buffer, count))
		return -EFAULT;

	if (kstrtoint(num, 10, &cmd) == 0)
		proc_cmd_id = cmd;
	else {
		proc_cmd_id = 0;
		return -EFAULT;
	}

	bm_err("proc_write success %d\n", cmd);
	return count;
}


static int proc_dump_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dump_log_show, NULL);
}

static const struct file_operations battery_dump_log_proc_fops = {
	.open = proc_dump_log_open,
	.read = seq_read,
	.llseek	= seq_lseek,
	.write = proc_write,
};

void battery_debug_init(void)
{
	struct proc_dir_entry *battery_dir;

	battery_dir = proc_mkdir("battery", NULL);
	if (!battery_dir) {
		bm_err("fail to mkdir /proc/battery\n");
		return;
	}

	proc_create("dump_log", 0644, battery_dir, &battery_dump_log_proc_fops);
	/*proc_create("dump_dtsi", S_IRUGO | S_IWUSR, battery_dir, &battery_dump_dtsi_proc_fops);*/
}

static ssize_t show_Battery_Temperature(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	bm_err("show_Battery_Temperature: %d %d\n", battery_main.BAT_batt_temp, fixed_bat_tmp);
	return sprintf(buf, "%d\n", fixed_bat_tmp);
}

static void fg_bat_temp_int_internal(void);
static ssize_t store_Battery_Temperature(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	signed int temp;

	if (kstrtoint(buf, 10, &temp) == 0) {

		fixed_bat_tmp = temp;
		if (fixed_bat_tmp == 0xffff)
			fg_bat_temp_int_internal();
		else {
			gauge_dev_enable_battery_tmp_lt_interrupt(gauge_dev, 0, 0);
			gauge_dev_enable_battery_tmp_ht_interrupt(gauge_dev, 0, 0);
		}
		battery_main.BAT_batt_temp = force_get_tbat(true);
		bm_err("store_Battery_Temperature: fixed_bat_tmp:%d ,tmp:%d!\n", temp, battery_main.BAT_batt_temp);
		battery_update(&battery_main);
	} else {
		bm_err("store_Battery_Temperature: format error!\n");
	}
	return size;
}

static DEVICE_ATTR(Battery_Temperature, 0664, show_Battery_Temperature,
		   store_Battery_Temperature);


/* ============================================================ */
/* Internal function */
/* ============================================================ */
int g_fg_battery_id;

#ifdef MTK_GET_BATTERY_ID_BY_AUXADC
void fgauge_get_profile_id(void)
{
	int id_volt = 0;
	int id = 0;
	int ret = 0;

	ret = IMM_GetOneChannelValue_Cali(BATTERY_ID_CHANNEL_NUM, &id_volt);
	if (ret != 0)
		bm_debug("[fgauge_get_profile_id]id_volt read fail\n");
	else
		bm_debug("[fgauge_get_profile_id]id_volt = %d\n", id_volt);

	if ((sizeof(g_battery_id_voltage) / sizeof(int)) != TOTAL_BATTERY_NUMBER) {
		bm_debug("[fgauge_get_profile_id]error! voltage range incorrect!\n");
		return;
	}

	for (id = 0; id < TOTAL_BATTERY_NUMBER; id++) {
		if (id_volt < g_battery_id_voltage[id]) {
			g_fg_battery_id = id;
			break;
		} else if (g_battery_id_voltage[id] == -1) {
			g_fg_battery_id = TOTAL_BATTERY_NUMBER - 1;
		}
	}

	bm_debug("[fgauge_get_profile_id]Battery id (%d)\n", g_fg_battery_id);
}
#elif defined(MTK_GET_BATTERY_ID_BY_GPIO)
void fgauge_get_profile_id(void)
{
	g_fg_battery_id = 0;
}
#else
void fgauge_get_profile_id(void)
{
	if (Bat_EC_ctrl.debug_bat_id_en == 1)
		g_fg_battery_id = Bat_EC_ctrl.debug_bat_id_value;
	else
		g_fg_battery_id = BATTERY_PROFILE_ID;
}
#endif


void fg_custom_init_from_header(void)
{
	fgauge_get_profile_id();

	fg_cust_data.versionID1 = FG_DAEMON_CMD_FROM_USER_NUMBER;
	fg_cust_data.versionID2 = sizeof(fg_cust_data);
	fg_cust_data.versionID3 = FG_KERNEL_CMD_FROM_USER_NUMBER;

	fg_cust_data.q_max_L_current = Q_MAX_L_CURRENT;
	fg_cust_data.q_max_H_current = Q_MAX_H_CURRENT;
	fg_cust_data.q_max_t0 = g_Q_MAX_T0[g_fg_battery_id];
	fg_cust_data.q_max_t1 = g_Q_MAX_T1[g_fg_battery_id];
	fg_cust_data.q_max_t2 = g_Q_MAX_T2[g_fg_battery_id];
	fg_cust_data.q_max_t3 = g_Q_MAX_T3[g_fg_battery_id];
	fg_cust_data.q_max_t4 = g_Q_MAX_T4[g_fg_battery_id];
	fg_cust_data.q_max_t0_h_current = g_Q_MAX_T0_H_CURRENT[g_fg_battery_id];
	fg_cust_data.q_max_t1_h_current = g_Q_MAX_T1_H_CURRENT[g_fg_battery_id];
	fg_cust_data.q_max_t2_h_current = g_Q_MAX_T2_H_CURRENT[g_fg_battery_id];
	fg_cust_data.q_max_t3_h_current = g_Q_MAX_T3_H_CURRENT[g_fg_battery_id];
	fg_cust_data.q_max_t4_h_current = g_Q_MAX_T4_H_CURRENT[g_fg_battery_id];
	fg_cust_data.q_max_sys_voltage = UNIT_TRANS_10 * g_Q_MAX_SYS_VOLTAGE[g_fg_battery_id];

	fg_cust_data.pseudo1_t0 = UNIT_TRANS_100 * g_FG_PSEUDO1_T0[g_fg_battery_id];
	fg_cust_data.pseudo1_t1 = UNIT_TRANS_100 * g_FG_PSEUDO1_T1[g_fg_battery_id];
	fg_cust_data.pseudo1_t2 = UNIT_TRANS_100 * g_FG_PSEUDO1_T2[g_fg_battery_id];
	fg_cust_data.pseudo1_t3 = UNIT_TRANS_100 * g_FG_PSEUDO1_T3[g_fg_battery_id];
	fg_cust_data.pseudo1_t4 = UNIT_TRANS_100 * g_FG_PSEUDO1_T4[g_fg_battery_id];
	fg_cust_data.pseudo100_t0 = UNIT_TRANS_100 * g_FG_PSEUDO100_T0[g_fg_battery_id];
	fg_cust_data.pseudo100_t1 = UNIT_TRANS_100 * g_FG_PSEUDO100_T1[g_fg_battery_id];
	fg_cust_data.pseudo100_t2 = UNIT_TRANS_100 * g_FG_PSEUDO100_T2[g_fg_battery_id];
	fg_cust_data.pseudo100_t3 = UNIT_TRANS_100 * g_FG_PSEUDO100_T3[g_fg_battery_id];
	fg_cust_data.pseudo100_t4 = UNIT_TRANS_100 * g_FG_PSEUDO100_T4[g_fg_battery_id];

	fg_cust_data.pseudo1_en = PSEUDO1_EN;
	fg_cust_data.pseudo100_en = PSEUDO100_EN;
	fg_cust_data.pseudo100_en_dis = PSEUDO100_EN_DIS;
	fg_cust_data.pseudo1_iq_offset = UNIT_TRANS_100 * g_FG_PSEUDO1_OFFSET[g_fg_battery_id];

	/* iboot related */
	fg_cust_data.qmax_sel = QMAX_SEL;
	fg_cust_data.iboot_sel = IBOOT_SEL;
	fg_cust_data.poweron_system_iboot = UNIT_TRANS_10 * POWERON_SYSTEM_IBOOT;
	fg_cust_data.shutdown_system_iboot = SHUTDOWN_SYSTEM_IBOOT;
	fg_cust_data.pmic_min_vol = PMIC_MIN_VOL;

	/*hw related */
	fg_cust_data.car_tune_value = UNIT_TRANS_10 * CAR_TUNE_VALUE;
	fg_cust_data.fg_meter_resistance = FG_METER_RESISTANCE;
	fg_cust_data.r_fg_value = UNIT_TRANS_10 * R_FG_VALUE;

	/* Aging Compensation */
	fg_cust_data.aging_one_en = AGING_ONE_EN;
	fg_cust_data.aging1_update_soc = UNIT_TRANS_100 * AGING1_UPDATE_SOC;
	fg_cust_data.aging1_load_soc = UNIT_TRANS_100 * AGING1_LOAD_SOC;
	fg_cust_data.aging_temp_diff = AGING_TEMP_DIFF;
	fg_cust_data.aging_100_en = AGING_100_EN;
	fg_cust_data.difference_voltage_update = DIFFERENCE_VOLTAGE_UPDATE;
	fg_cust_data.aging_factor_min = UNIT_TRANS_100 * AGING_FACTOR_MIN;
	fg_cust_data.aging_factor_diff = UNIT_TRANS_100 * AGING_FACTOR_DIFF;
	/* Aging Compensation 2*/
	fg_cust_data.aging_two_en = AGING_TWO_EN;
	/* Aging Compensation 3*/
	fg_cust_data.aging_third_en = AGING_THIRD_EN;

	/* ui_soc related */
	fg_cust_data.diff_soc_setting = DIFF_SOC_SETTING;
	fg_cust_data.keep_100_percent = UNIT_TRANS_100 * KEEP_100_PERCENT;
	fg_cust_data.difference_full_cv = DIFFERENCE_FULL_CV;
	fg_cust_data.diff_bat_temp_setting = DIFF_BAT_TEMP_SETTING;
	fg_cust_data.diff_bat_temp_setting_c = DIFF_BAT_TEMP_SETTING_C;
	fg_cust_data.discharge_tracking_time = DISCHARGE_TRACKING_TIME;
	fg_cust_data.charge_tracking_time = CHARGE_TRACKING_TIME;
	fg_cust_data.difference_fullocv_vth = DIFFERENCE_FULLOCV_VTH;
	fg_cust_data.difference_fullocv_ith = UNIT_TRANS_10 * DIFFERENCE_FULLOCV_ITH;
	fg_cust_data.charge_pseudo_full_level = CHARGE_PSEUDO_FULL_LEVEL;

	/* pre tracking */
	fg_cust_data.fg_pre_tracking_en = FG_PRE_TRACKING_EN;
	fg_cust_data.vbat2_det_time = VBAT2_DET_TIME;
	fg_cust_data.vbat2_det_counter = VBAT2_DET_COUNTER;
	fg_cust_data.vbat2_det_voltage1 = VBAT2_DET_VOLTAGE1;
	fg_cust_data.vbat2_det_voltage2 = VBAT2_DET_VOLTAGE2;
	fg_cust_data.vbat2_det_voltage3 = VBAT2_DET_VOLTAGE3;

	/* sw fg */
	fg_cust_data.difference_fgc_fgv_th1 = DIFFERENCE_FGC_FGV_TH1;
	fg_cust_data.difference_fgc_fgv_th2 = DIFFERENCE_FGC_FGV_TH2;
	fg_cust_data.difference_fgc_fgv_th3 = DIFFERENCE_FGC_FGV_TH3;
	fg_cust_data.difference_fgc_fgv_th_soc1 = DIFFERENCE_FGC_FGV_TH_SOC1;
	fg_cust_data.difference_fgc_fgv_th_soc2 = DIFFERENCE_FGC_FGV_TH_SOC2;
	fg_cust_data.nafg_time_setting = NAFG_TIME_SETTING;
	fg_cust_data.nafg_ratio = NAFG_RATIO;
	fg_cust_data.nafg_ratio_en = NAFG_RATIO_EN;
	fg_cust_data.nafg_resistance = NAFG_RESISTANCE;

	/* ADC resistor  */
	fg_cust_data.r_charger_1 = R_CHARGER_1;
	fg_cust_data.r_charger_2 = R_CHARGER_2;

	/* mode select */
	fg_cust_data.pmic_shutdown_current = PMIC_SHUTDOWN_CURRENT;
	fg_cust_data.pmic_shutdown_sw_en = PMIC_SHUTDOWN_SW_EN;
	fg_cust_data.force_vc_mode = FORCE_VC_MODE;
	fg_cust_data.embedded_sel = EMBEDDED_SEL;
	fg_cust_data.loading_1_en = LOADING_1_EN;
	fg_cust_data.loading_2_en = LOADING_2_EN;
	fg_cust_data.diff_iavg_th = DIFF_IAVG_TH;

	fg_cust_data.shutdown_gauge0 = SHUTDOWN_GAUGE0;
	fg_cust_data.shutdown_1_time = SHUTDOWN_1_TIME;
	fg_cust_data.shutdown_gauge1_xmins = SHUTDOWN_GAUGE1_XMINS;
	fg_cust_data.shutdown_gauge0_voltage = SHUTDOWN_GAUGE0_VOLTAGE;

	/* ZCV update */
	fg_cust_data.zcv_suspend_time = ZCV_SUSPEND_TIME;
	fg_cust_data.sleep_current_avg = SLEEP_CURRENT_AVG;
	fg_cust_data.zcv_car_gap_percentage = ZCV_CAR_GAP_PERCENTAGE;

	/* dod_init */
	fg_cust_data.hwocv_oldocv_diff = HWOCV_OLDOCV_DIFF;
	fg_cust_data.hwocv_swocv_diff = HWOCV_SWOCV_DIFF;
	fg_cust_data.hwocv_swocv_diff_lt = HWOCV_SWOCV_DIFF_LT;
	fg_cust_data.hwocv_swocv_diff_lt_temp = HWOCV_SWOCV_DIFF_LT_TEMP;
	fg_cust_data.swocv_oldocv_diff = SWOCV_OLDOCV_DIFF;
	fg_cust_data.vbat_oldocv_diff = VBAT_OLDOCV_DIFF;
	fg_cust_data.swocv_oldocv_diff_emb = SWOCV_OLDOCV_DIFF_EMB;

	fg_cust_data.pmic_shutdown_time = UNIT_TRANS_60 * PMIC_SHUTDOWN_TIME;
	fg_cust_data.tnew_told_pon_diff = TNEW_TOLD_PON_DIFF;
	fg_cust_data.tnew_told_pon_diff2 = TNEW_TOLD_PON_DIFF2;

	fg_cust_data.dc_ratio_sel = DC_RATIO_SEL;
	fg_cust_data.dc_r_cnt = DC_R_CNT;

	/* shutdown_hl_zcv */
	fg_cust_data.shutdown_hl_zcv_t0 = g_SHUTDOWN_HL_ZCV_T0[g_fg_battery_id];
	fg_cust_data.shutdown_hl_zcv_t1 = g_SHUTDOWN_HL_ZCV_T1[g_fg_battery_id];
	fg_cust_data.shutdown_hl_zcv_t2 = g_SHUTDOWN_HL_ZCV_T2[g_fg_battery_id];
	fg_cust_data.shutdown_hl_zcv_t3 = g_SHUTDOWN_HL_ZCV_T3[g_fg_battery_id];
	fg_cust_data.shutdown_hl_zcv_t4 = g_SHUTDOWN_HL_ZCV_T4[g_fg_battery_id];

	fg_cust_data.pseudo1_sel = PSEUDO1_SEL;
	fg_cust_data.additional_battery_table_en = ADDITIONAL_BATTERY_TABLE_EN;
	fg_cust_data.d0_sel = D0_SEL;
	fg_cust_data.aging_sel = AGING_SEL;
	fg_cust_data.bat_par_i = BAT_PAR_I;

	fg_cust_data.fg_tracking_current = FG_TRACKING_CURRENT;
	fg_cust_data.fg_tracking_current_iboot_en = FG_TRACKING_CURRENT_IBOOT_EN;
	fg_cust_data.ui_fast_tracking_en = UI_FAST_TRACKING_EN;
	fg_cust_data.ui_fast_tracking_gap = UI_FAST_TRACKING_GAP;

	fg_cust_data.bat_plug_out_time = BAT_PLUG_OUT_TIME;
	fg_cust_data.keep_100_percent_minsoc = KEEP_100_PERCENT_MINSOC;

/* table */
	fg_cust_data.additional_battery_table_en = ADDITIONAL_BATTERY_TABLE_EN;
	fg_cust_data.temperature_t0 = TEMPERATURE_T0;
	fg_cust_data.temperature_t1 = TEMPERATURE_T1;
	fg_cust_data.temperature_t2 = TEMPERATURE_T2;
	fg_cust_data.temperature_t3 = TEMPERATURE_T3;
	fg_cust_data.temperature_t4 = TEMPERATURE_T4;
	fg_cust_data.temperature_tb0 = TEMPERATURE_TB0;
	fg_cust_data.temperature_tb1 = TEMPERATURE_TB1;

	fg_cust_data.battery_tmp_to_disable_gm30 = BATTERY_TMP_TO_DISABLE_GM30;
	fg_cust_data.battery_tmp_to_disable_nafg = BATTERY_TMP_TO_DISABLE_NAFG;
	fg_cust_data.battery_tmp_to_enable_nafg = BATTERY_TMP_TO_ENABLE_NAFG;

#if defined(GM30_DISABLE_NAFG)
		fg_cust_data.disable_nafg = 1;
#else
		fg_cust_data.disable_nafg = 0;
#endif

	fg_table_cust_data.fg_profile_t0_size =
		sizeof(fg_profile_t0[g_fg_battery_id]) / sizeof(struct FUELGAUGE_PROFILE_STRUCT);

	memcpy(&fg_table_cust_data.fg_profile_t0,
			&fg_profile_t0[g_fg_battery_id],
			sizeof(fg_profile_t0[g_fg_battery_id]));

	fg_table_cust_data.fg_profile_t1_size =
		sizeof(fg_profile_t1[g_fg_battery_id]) / sizeof(struct FUELGAUGE_PROFILE_STRUCT);

	memcpy(&fg_table_cust_data.fg_profile_t1,
			&fg_profile_t1[g_fg_battery_id],
			sizeof(fg_profile_t1[g_fg_battery_id]));

	fg_table_cust_data.fg_profile_t2_size =
		sizeof(fg_profile_t2[g_fg_battery_id]) / sizeof(struct FUELGAUGE_PROFILE_STRUCT);

	memcpy(&fg_table_cust_data.fg_profile_t2,
			&fg_profile_t2[g_fg_battery_id],
			sizeof(fg_profile_t2[g_fg_battery_id]));

	fg_table_cust_data.fg_profile_t3_size =
		sizeof(fg_profile_t3[g_fg_battery_id]) / sizeof(struct FUELGAUGE_PROFILE_STRUCT);

	memcpy(&fg_table_cust_data.fg_profile_t3,
			&fg_profile_t3[g_fg_battery_id],
			sizeof(fg_profile_t3[g_fg_battery_id]));

	fg_table_cust_data.fg_profile_t4_size =
		sizeof(fg_profile_t4[g_fg_battery_id]) / sizeof(struct FUELGAUGE_PROFILE_STRUCT);

	memcpy(&fg_table_cust_data.fg_profile_t4,
			&fg_profile_t4[g_fg_battery_id],
			sizeof(fg_profile_t4[g_fg_battery_id]));


	/* fg_custom_init_dump(); */

}

#ifdef CONFIG_OF
static void fg_custom_parse_table(const struct device_node *np,
				const char *node_srting, struct FUELGAUGE_PROFILE_STRUCT *profile_struct)
{
	int mah, voltage, resistance, idx, saddles;
	struct FUELGAUGE_PROFILE_STRUCT *profile_p;

	profile_p = profile_struct;

	saddles = fg_table_cust_data.fg_profile_t0_size;
	idx = 0;
	bm_debug("fg_custom_parse_table: %s, %d\n", node_srting, saddles);

	while (!of_property_read_u32_index(np, node_srting, idx, &mah)) {
		idx++;
		if (!of_property_read_u32_index(np, node_srting, idx, &voltage)) {
			/*bm_err("fg_custom_parse_table: mah: %d, voltage: %d\n", mah, voltage);*/
			/*bm_err("fg_custom_parse_table: mah: %d, voltage: %d\n", mah, voltage);*/
		}
		idx++;
		if (!of_property_read_u32_index(np, node_srting, idx, &resistance)) {
			bm_debug("fg_custom_parse_table: mah: %d, voltage: %d, resistance: %d\n",
				    mah, voltage, resistance);
		}

		profile_p->mah = mah;
		profile_p->voltage = voltage;
		profile_p->resistance = resistance;

		/* dump parsing data */
		#if 0
		msleep(20);
		bm_print(BM_LOG_CRTI, "__batt_meter_parse_table>> %s[%d]: <%d, %d>\n",
				node_srting, (idx/2), profile_p->percentage, profile_p->voltage);
		#endif

		profile_p++;
		if ((idx++) >= (saddles * 3))
			break;
	}

	/* error handle */
	if (idx == 0) {
		bm_err("[%s] cannot find %s in dts\n", __func__, node_srting);
		return;
	}

	/* use last data to fill with the rest array */
	/* if raw data is less than temp array */
	/* error handle */
	profile_p--;

	while (idx < (100 * 3)) {
		profile_p++;
		profile_p->mah = mah;
		profile_p->voltage = voltage;
		profile_p->resistance = resistance;
		idx = idx + 3;
	}
}


void fg_custom_init_from_dts(struct platform_device *dev)
{
	struct device_node *np = dev->dev.of_node;
	unsigned int val;
	unsigned int val_0, val_1, val_2, val_3;
	int ret, ret0, ret1, ret2, ret3;
	int bat_id;
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
	unsigned int val_4;
	int ret4;
#endif

	fgauge_get_profile_id();
	bat_id = g_fg_battery_id;

	bm_err("fg_custom_init_from_dts\n");

	if (!of_property_read_u32(np, "g_FG_PSEUDO100_T0", &val)) {
		fg_cust_data.pseudo100_t0 = (int)val * UNIT_TRANS_100;
		bm_debug("Get g_FG_PSEUDO100_T0: %d\n",
			 fg_cust_data.pseudo100_t0);
	} else {
		bm_err("Get g_FG_PSEUDO100_T0 failed\n");
	}

	if (!of_property_read_u32(np, "g_FG_PSEUDO100_T1", &val)) {
		fg_cust_data.pseudo100_t1 = (int)val * UNIT_TRANS_100;
		bm_debug("Get g_FG_PSEUDO100_T1: %d\n",
			 fg_cust_data.pseudo100_t1);
	} else {
		bm_err("Get g_FG_PSEUDO100_T1 failed\n");
	}

	if (!of_property_read_u32(np, "g_FG_PSEUDO100_T2", &val)) {
		fg_cust_data.pseudo100_t2 = (int)val * UNIT_TRANS_100;
		bm_debug("Get g_FG_PSEUDO100_T2: %d\n",
			 fg_cust_data.pseudo100_t2);
	} else {
		bm_err("Get g_FG_PSEUDO100_T2 failed\n");
	}

	if (!of_property_read_u32(np, "g_FG_PSEUDO100_T3", &val)) {
		fg_cust_data.pseudo100_t3 = (int)val * UNIT_TRANS_100;
		bm_debug("Get g_FG_PSEUDO100_T3: %d\n",
			 fg_cust_data.pseudo100_t3);
	} else {
		bm_err("Get g_FG_PSEUDO100_T3 failed\n");
	}

	if (!of_property_read_u32(np, "g_FG_PSEUDO100_T4", &val)) {
		fg_cust_data.pseudo100_t4 = (int)val * UNIT_TRANS_100;
		bm_debug("Get g_FG_PSEUDO100_T4: %d\n",
			 fg_cust_data.pseudo100_t4);
	} else {
		bm_err("Get g_FG_PSEUDO100_T4 failed\n");
	}

	if (!of_property_read_u32(np, "DIFFERENCE_FULLOCV_ITH", &val)) {
		fg_cust_data.difference_fullocv_ith = (int)val * UNIT_TRANS_10;
		bm_debug("Get DIFFERENCE_FULLOCV_ITH: %d\n",
			 fg_cust_data.difference_fullocv_ith);
	} else {
		bm_err("Get DIFFERENCE_FULLOCV_ITH failed\n");
	}

	if (!of_property_read_u32(np, "MTK_CHR_EXIST", &val)) {
		fg_cust_data.mtk_chr_exist = (int)val;
		bm_debug("Get MTK_CHR_EXIST: %d\n",
			 fg_cust_data.mtk_chr_exist);
	} else {
		bm_err("Get MTK_CHR_EXIST failed\n");
	}

		switch (bat_id) {
		case 0:
			ret = !of_property_read_u32(np, "Q_MAX_SYS_VOLTAGE_BAT0", &val);
			break;
		case 1:
			ret = !of_property_read_u32(np, "Q_MAX_SYS_VOLTAGE_BAT1", &val);
			break;
		case 2:
			ret = !of_property_read_u32(np, "Q_MAX_SYS_VOLTAGE_BAT2", &val);
			break;
		case 3:
			ret = !of_property_read_u32(np, "Q_MAX_SYS_VOLTAGE_BAT3", &val);
			break;
		default:
			ret = 0;
			break;
		}
	if (ret) {
		fg_cust_data.q_max_sys_voltage = (int)val * UNIT_TRANS_10;
		bm_debug("Get Q_MAX_SYS_VOLTAGE BAT%d: %d\n",
				 bat_id, fg_cust_data.q_max_sys_voltage);
	} else {
		bm_err("Get Q_MAX_SYS_VOLTAGE BAT%d failed\n", bat_id);
	}

		switch (bat_id) {
		case 0:
			ret = !of_property_read_u32(np, "PSEUDO1_IQ_OFFSET_BAT0", &val);
			break;
		case 1:
			ret = !of_property_read_u32(np, "PSEUDO1_IQ_OFFSET_BAT1", &val);
			break;
		case 2:
			ret = !of_property_read_u32(np, "PSEUDO1_IQ_OFFSET_BAT2", &val);
			break;
		case 3:
			ret = !of_property_read_u32(np, "PSEUDO1_IQ_OFFSET_BAT3", &val);
			break;
		default:
			ret = 0;
			break;
		}
	if (ret) {
		fg_cust_data.pseudo1_iq_offset = (int)val * UNIT_TRANS_100;
		bm_err("Get PSEUDO1_IQ_OFFSET BAT%d: %d\n",
				 bat_id, fg_cust_data.pseudo1_iq_offset);
	} else {
		bm_err("Get PSEUDO1_IQ_OFFSET BAT%d failed\n", bat_id);
	}

	if (!of_property_read_u32(np, "SHUTDOWN_1_TIME", &val)) {
		fg_cust_data.shutdown_1_time = (int)val;
		bm_debug("Get SHUTDOWN_1_TIME: %d\n",
			 fg_cust_data.shutdown_1_time);
	} else {
		bm_err("Get SHUTDOWN_1_TIME failed\n");
	}

	if (!of_property_read_u32(np, "KEEP_100_PERCENT", &val)) {
		fg_cust_data.keep_100_percent = (int)val * UNIT_TRANS_100;
		bm_debug("Get KEEP_100_PERCENT: %d\n",
			 fg_cust_data.keep_100_percent);
	} else {
		bm_err("Get KEEP_100_PERCENT failed\n");
	}

	if (!of_property_read_u32(np, "R_FG_VALUE", &val)) {
		fg_cust_data.r_fg_value = (int)val * UNIT_TRANS_10;
		bm_debug("Get R_FG_VALUE: %d\n",
			 fg_cust_data.r_fg_value);
	} else {
		bm_err("Get R_FG_VALUE failed\n");
	}

	if (!of_property_read_u32(np, "TEMPERATURE_T0", &val)) {
		fg_cust_data.temperature_t0 = (int)val;
		bm_debug("Get TEMPERATURE_T0: %d\n",
			 fg_cust_data.temperature_t0);
	} else {
		bm_err("Get TEMPERATURE_T0 failed\n");
	}

	if (!of_property_read_u32(np, "TEMPERATURE_T1", &val)) {
		fg_cust_data.temperature_t1 = (int)val;
		bm_debug("Get TEMPERATURE_T1: %d\n",
			 fg_cust_data.temperature_t1);
	} else {
		bm_err("Get TEMPERATURE_T1 failed\n");
	}

	if (!of_property_read_u32(np, "TEMPERATURE_T2", &val)) {
		fg_cust_data.temperature_t2 = (int)val;
		bm_debug("Get TEMPERATURE_T2: %d\n",
			 fg_cust_data.temperature_t2);
	} else {
		bm_err("Get TEMPERATURE_T2 failed\n");
	}

	if (!of_property_read_u32(np, "TEMPERATURE_T3", &val)) {
		fg_cust_data.temperature_t3 = (int)val;
		bm_debug("Get TEMPERATURE_T3: %d\n",
			 fg_cust_data.temperature_t3);
	} else {
		bm_err("Get TEMPERATURE_T3 failed\n");
	}

	if (!of_property_read_u32(np, "TEMPERATURE_T4", &val)) {
		fg_cust_data.temperature_t4 = (int)val;
		bm_debug("Get TEMPERATURE_T4: %d\n",
			 fg_cust_data.temperature_t4);
	} else {
		bm_err("Get TEMPERATURE_T4 failed\n");
	}

	if (!of_property_read_u32(np, "EMBEDDED_SEL", &val)) {
		fg_cust_data.embedded_sel = (int)val;
		bm_debug("Get EMBEDDED_SEL: %d\n",
			 fg_cust_data.embedded_sel);
	} else {
		bm_err("Get EMBEDDED_SEL failed\n");
	}

	if (!of_property_read_u32(np, "PMIC_SHUTDOWN_CURRENT", &val)) {
		fg_cust_data.pmic_shutdown_current = (int)val;
		bm_debug("Get PMIC_SHUTDOWN_CURRENT: %d\n",
			 fg_cust_data.pmic_shutdown_current);
	} else {
		bm_err("Get PMIC_SHUTDOWN_CURRENT failed\n");
	}

	if (!of_property_read_u32(np, "CAR_TUNE_VALUE", &val)) {
		fg_cust_data.car_tune_value = (int)val * UNIT_TRANS_10;
		bm_debug("Get CAR_TUNE_VALUE: %d\n",
			 fg_cust_data.car_tune_value);
	} else {
		bm_err("Get CAR_TUNE_VALUE failed\n");
	}

	if (!of_property_read_u32(np, "FG_METER_RESISTANCE", &val)) {
		fg_cust_data.fg_meter_resistance = (int)val;
		bm_debug("Get FG_METER_RESISTANCE: %d\n",
			 fg_cust_data.fg_meter_resistance);
	} else {
		bm_err("Get FG_METER_RESISTANCE failed\n");
	}


		switch (bat_id) {
		case 0:
			ret0 = !of_property_read_u32(np, "battery0_profile_t0_num", &val_0);
			ret1 = !of_property_read_u32(np, "battery0_profile_t1_num", &val_1);
			ret2 = !of_property_read_u32(np, "battery0_profile_t2_num", &val_2);
			ret3 = !of_property_read_u32(np, "battery0_profile_t3_num", &val_3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			ret4 = !of_property_read_u32(np, "battery0_profile_t4_num", &val_4);
#endif
			break;
		case 1:
			ret0 = !of_property_read_u32(np, "battery1_profile_t0_num", &val_0);
			ret1 = !of_property_read_u32(np, "battery1_profile_t1_num", &val_1);
			ret2 = !of_property_read_u32(np, "battery1_profile_t2_num", &val_2);
			ret3 = !of_property_read_u32(np, "battery1_profile_t3_num", &val_3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			ret4 = !of_property_read_u32(np, "battery1_profile_t4_num", &val_4);
#endif
			break;
		case 2:
			ret0 = !of_property_read_u32(np, "battery2_profile_t0_num", &val_0);
			ret1 = !of_property_read_u32(np, "battery2_profile_t1_num", &val_1);
			ret2 = !of_property_read_u32(np, "battery2_profile_t2_num", &val_2);
			ret3 = !of_property_read_u32(np, "battery2_profile_t3_num", &val_3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			ret4 = !of_property_read_u32(np, "battery2_profile_t4_num", &val_4);
#endif
			break;
		case 3:
			ret0 = !of_property_read_u32(np, "battery3_profile_t0_num", &val_0);
			ret1 = !of_property_read_u32(np, "battery3_profile_t1_num", &val_1);
			ret2 = !of_property_read_u32(np, "battery3_profile_t2_num", &val_2);
			ret3 = !of_property_read_u32(np, "battery3_profile_t3_num", &val_3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			ret4 = !of_property_read_u32(np, "battery3_profile_t4_num", &val_4);
#endif
			break;
		default:
			ret0 = 0;
			ret1 = 0;
			ret2 = 0;
			ret3 = 0;
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			ret4 = 0;
#endif
			break;
		}
	if (ret0) {
		bm_debug("Get battery%d_profile_t0_num: %d\n", bat_id, val_0);
		fg_table_cust_data.fg_profile_t0_size = val_0;
	} else
		bm_err("Get battery%d_profile_t0_num failed\n", bat_id);

	if (ret1) {
		bm_debug("Get battery%d_profile_t1_num: %d\n", bat_id, val_1);
		fg_table_cust_data.fg_profile_t1_size = val_1;
	} else
		bm_err("Get battery%d_profile_t1_num failed\n", bat_id);

	if (ret2) {
		bm_debug("Get battery%d_profile_t2_num: %d\n", bat_id, val_2);
		fg_table_cust_data.fg_profile_t2_size = val_2;
	} else
		bm_err("Get battery%d_profile_t2_num failed\n", bat_id);

	if (ret3) {
		bm_debug("Get battery%d_profile_t3_num: %d\n", bat_id, val_3);
		fg_table_cust_data.fg_profile_t3_size = val_3;
	} else
		bm_err("Get battery%d_profile_t3_num failed\n", bat_id);

#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
	if (ret4) {
		bm_debug("Get battery%d_profile_t4_num: %d\n", bat_id, val_4);
		fg_table_cust_data.fg_profile_t4_size = val_4;
	} else
		bm_err("Get battery%d_profile_t4_num failed\n", bat_id);
#endif

		switch (bat_id) {
		case 0:
			fg_custom_parse_table(np, "battery0_profile_t0",
				fg_table_cust_data.fg_profile_t0);
			fg_custom_parse_table(np, "battery0_profile_t1",
				fg_table_cust_data.fg_profile_t1);
			fg_custom_parse_table(np, "battery0_profile_t2",
				fg_table_cust_data.fg_profile_t2);
			fg_custom_parse_table(np, "battery0_profile_t3",
				fg_table_cust_data.fg_profile_t3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			fg_custom_parse_table(np, "battery0_profile_t4",
				fg_table_cust_data.fg_profile_t4);
#endif
			break;

		case 1:
			fg_custom_parse_table(np, "battery1_profile_t0",
				fg_table_cust_data.fg_profile_t0);
			fg_custom_parse_table(np, "battery1_profile_t1",
				fg_table_cust_data.fg_profile_t1);
			fg_custom_parse_table(np, "battery1_profile_t2",
				fg_table_cust_data.fg_profile_t2);
			fg_custom_parse_table(np, "battery1_profile_t3",
				fg_table_cust_data.fg_profile_t3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			fg_custom_parse_table(np, "battery1_profile_t4",
				fg_table_cust_data.fg_profile_t4);
#endif
			break;

		case 2:
			fg_custom_parse_table(np, "battery2_profile_t0",
				fg_table_cust_data.fg_profile_t0);
			fg_custom_parse_table(np, "battery2_profile_t1",
				fg_table_cust_data.fg_profile_t1);
			fg_custom_parse_table(np, "battery2_profile_t2",
				fg_table_cust_data.fg_profile_t2);
			fg_custom_parse_table(np, "battery2_profile_t3",
				fg_table_cust_data.fg_profile_t3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			fg_custom_parse_table(np, "battery2_profile_t4",
				fg_table_cust_data.fg_profile_t4);
#endif
			break;

		case 3:
			fg_custom_parse_table(np, "battery3_profile_t0",
				fg_table_cust_data.fg_profile_t0);
			fg_custom_parse_table(np, "battery3_profile_t1",
				fg_table_cust_data.fg_profile_t1);
			fg_custom_parse_table(np, "battery3_profile_t2",
				fg_table_cust_data.fg_profile_t2);
			fg_custom_parse_table(np, "battery3_profile_t3",
				fg_table_cust_data.fg_profile_t3);
#ifdef CONFIG_MTK_ADDITIONAL_BATTERY_TABLE
			fg_custom_parse_table(np, "battery3_profile_t4",
				fg_table_cust_data.fg_profile_t4);
#endif
			break;

		default:
			break;
		}

}
#endif

#if 0
void fg_custom_init(struct platform_device *dev)
{
	fg_custom_init_from_header();
	fg_custom_init_from_dts(dev);
}
#endif

int interpolation(int i1, int b1, int i2, int b2, int i)
{
	int ret;

	ret = (b2 - b1) * (i - i1) / (i2 - i1) + b1;

	/*FGLOG_ERROR("[FGADC] interpolation : %d %d %d %d %d %d!\r\n", i1, b1, i2, b2, i, ret);*/
	return ret;
}

unsigned int TempConverBattThermistor(int temp)
{
	int RES1 = 0, RES2 = 0;
	int TMP1 = 0, TMP2 = 0;
	int i;
	unsigned int TBatt_R_Value = 0xffff;

	if (temp >= Fg_Temperature_Table[20].BatteryTemp) {
		TBatt_R_Value = Fg_Temperature_Table[20].TemperatureR;
	} else if (temp <= Fg_Temperature_Table[0].BatteryTemp) {
		TBatt_R_Value = Fg_Temperature_Table[0].TemperatureR;
	} else {
		RES1 = Fg_Temperature_Table[0].TemperatureR;
		TMP1 = Fg_Temperature_Table[0].BatteryTemp;

		for (i = 0; i <= 20; i++) {
			if (temp <= Fg_Temperature_Table[i].BatteryTemp) {
				RES2 = Fg_Temperature_Table[i].TemperatureR;
				TMP2 = Fg_Temperature_Table[i].BatteryTemp;
				break;
			}
			{	/* hidden else */
				RES1 = Fg_Temperature_Table[i].TemperatureR;
				TMP1 = Fg_Temperature_Table[i].BatteryTemp;
			}
		}


		TBatt_R_Value = interpolation(TMP1, RES1, TMP2, RES2, temp);
	}

	bm_warn("[TempConverBattThermistor] [%d] %d %d %d %d %d\n", TBatt_R_Value, TMP1, RES1, TMP2, RES2, temp);

	return TBatt_R_Value;
}

int BattThermistorConverTemp(int Res)
{
	int i = 0;
	int RES1 = 0, RES2 = 0;
	int TBatt_Value = -200, TMP1 = 0, TMP2 = 0;

	if (Res >= Fg_Temperature_Table[0].TemperatureR) {
		TBatt_Value = -40;
	} else if (Res <= Fg_Temperature_Table[20].TemperatureR) {
		TBatt_Value = 60;
	} else {
		RES1 = Fg_Temperature_Table[0].TemperatureR;
		TMP1 = Fg_Temperature_Table[0].BatteryTemp;

		for (i = 0; i <= 20; i++) {
			if (Res >= Fg_Temperature_Table[i].TemperatureR) {
				RES2 = Fg_Temperature_Table[i].TemperatureR;
				TMP2 = Fg_Temperature_Table[i].BatteryTemp;
				break;
			}
			{	/* hidden else */
				RES1 = Fg_Temperature_Table[i].TemperatureR;
				TMP1 = Fg_Temperature_Table[i].BatteryTemp;
			}
		}

		TBatt_Value = (((Res - RES2) * TMP1) + ((RES1 - Res) * TMP2)) / (RES1 - RES2);
	}
	bm_trace("[BattThermistorConverTemp] %d %d %d %d %d %d\n", RES1, RES2, Res, TMP1, TMP2, TBatt_Value);

	return TBatt_Value;
}

unsigned int TempToBattVolt(int temp, int update)
{
	unsigned int R_NTC = TempConverBattThermistor(temp);
	long long Vin = 0;
	long long V_IR_comp = 0;
	/*int vbif28 = pmic_get_auxadc_value(AUXADC_LIST_VBIF);*/
	int vbif28 = RBAT_PULL_UP_VOLT;
	static int fg_current_temp;
	static bool fg_current_state;
	int fg_r_value = fg_cust_data.r_fg_value;
	int	fg_meter_res_value = fg_cust_data.fg_meter_resistance;

#ifdef RBAT_PULL_UP_VOLT_BY_BIF
	vbif28 = pmic_get_vbif28_volt();
#endif
	Vin = (long long)R_NTC * vbif28 * 10;	/* 0.1 mV */
	do_div(Vin, (R_NTC + RBAT_PULL_UP_R));

	if (update == true)
		fg_current_state = gauge_get_current(&fg_current_temp);

	if (fg_current_state == true) {
		V_IR_comp = Vin;
		V_IR_comp += ((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
	} else {
		V_IR_comp = Vin;
		V_IR_comp -= ((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
	}

	bm_notice("[TempToBattVolt] temp %d R_NTC %d V(%lld %lld) I %d CHG %d\n",
		temp, R_NTC, Vin, V_IR_comp, fg_current_temp, fg_current_state);

	return (unsigned int) V_IR_comp;
}

int BattVoltToTemp(int dwVolt, int volt_cali)
{
	long long TRes_temp;
	long long TRes;
	int sBaTTMP = -100;
	/*int vbif28 = pmic_get_auxadc_value(AUXADC_LIST_VBIF);*/
	int vbif28 = RBAT_PULL_UP_VOLT;	/* 2 side: BattVoltToTemp, TempToBattVolt */
	int delta_v;

	/* TRes_temp = ((long long)RBAT_PULL_UP_R*(long long)dwVolt) / (RBAT_PULL_UP_VOLT-dwVolt); */
	/* TRes = (TRes_temp * (long long)RBAT_PULL_DOWN_R)/((long long)RBAT_PULL_DOWN_R - TRes_temp); */

	TRes_temp = (RBAT_PULL_UP_R * (long long) dwVolt);
#ifdef RBAT_PULL_UP_VOLT_BY_BIF
	vbif28 = pmic_get_vbif28_volt() + volt_cali;
	delta_v = abs(vbif28 - dwVolt);
	if (delta_v == 0)
		delta_v = 1;

	do_div(TRes_temp, delta_v);
	if (vbif28 > 3000 || vbif28 < 2500)
		bm_err("[RBAT_PULL_UP_VOLT_BY_BIF] vbif28:%d\n", pmic_get_vbif28_volt());
#else
	delta_v = abs(RBAT_PULL_UP_VOLT - dwVolt);
	if (delta_v == 0)
		delta_v = 1;

	do_div(TRes_temp, delta_v);
#endif

#ifdef RBAT_PULL_DOWN_R
	TRes = (TRes_temp * RBAT_PULL_DOWN_R);
	do_div(TRes, abs(RBAT_PULL_DOWN_R - TRes_temp));
#else
	TRes = TRes_temp;
#endif

	/* convert register to temperature */
	if (!pmic_is_bif_exist())
		sBaTTMP = BattThermistorConverTemp((int)TRes);
	else
		sBaTTMP = BattThermistorConverTemp((int)TRes - BIF_NTC_R);

	bm_notice("[BattVoltToTemp] %d %d %d %d\n", dwVolt, RBAT_PULL_UP_R, vbif28, volt_cali);
	return sBaTTMP;
}

int force_get_tbat_internal(bool update)
{
	int bat_temperature_volt = 0;
	int bat_temperature_val = 0;
	static int pre_bat_temperature_val = -1;
	int fg_r_value = 0;
	int fg_meter_res_value = 0;
	int fg_current_temp = 0;
	bool fg_current_state = false;
	int bat_temperature_volt_temp = 0;
	int vol_cali = 0;

	static int pre_bat_temperature_volt_temp, pre_bat_temperature_volt;
	static int pre_fg_current_temp;
	static int pre_fg_current_state;
	static int pre_fg_r_value;
	static int pre_bat_temperature_val2;
	static struct timespec pre_time;
	struct timespec ctime, dtime;

	if (is_battery_init_done() == false)
		return 25;

	if (fixed_bat_tmp != 0xffff)
		return fixed_bat_tmp;

	if (Bat_EC_ctrl.fixed_temp_en)
		return Bat_EC_ctrl.fixed_temp_value;

	if (update == true || pre_bat_temperature_val == -1) {
		/* Get V_BAT_Temperature */
		bat_temperature_volt = 2;
		bat_temperature_volt = pmic_get_v_bat_temp();

		if (bat_temperature_volt != 0) {
			fg_r_value = fg_cust_data.r_fg_value;
			fg_meter_res_value = fg_cust_data.fg_meter_resistance;


			gauge_dev_get_current(gauge_dev, &fg_current_state, &fg_current_temp);
			fg_current_temp = fg_current_temp / 10;

			if (fg_current_state == true) {
				bat_temperature_volt_temp = bat_temperature_volt;
				bat_temperature_volt =
				bat_temperature_volt - ((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
				vol_cali = -((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
			} else {
				bat_temperature_volt_temp = bat_temperature_volt;
				bat_temperature_volt =
				bat_temperature_volt + ((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
				vol_cali = ((fg_current_temp * (fg_meter_res_value + fg_r_value)) / 10000);
			}

			bat_temperature_val = BattVoltToTemp(bat_temperature_volt, vol_cali);
		}

#ifdef CONFIG_MTK_BIF_SUPPORT
		/*	battery_charging_control(CHARGING_CMD_GET_BIF_TBAT, &bat_temperature_val); */
#endif
		bm_notice("[force_get_tbat] %d,%d,%d,%d,%d,%d r:%d %d\n",
		bat_temperature_volt_temp, bat_temperature_volt, fg_current_state, fg_current_temp,
		fg_r_value, bat_temperature_val, fg_meter_res_value, fg_r_value);

		if (pre_bat_temperature_val2 == 0) {
			pre_bat_temperature_volt_temp = bat_temperature_volt_temp;
			pre_bat_temperature_volt = bat_temperature_volt;
			pre_fg_current_temp = fg_current_temp;
			pre_fg_current_state = fg_current_state;
			pre_fg_r_value = fg_r_value;
			pre_bat_temperature_val2 = bat_temperature_val;
			get_monotonic_boottime(&pre_time);
		} else {
			get_monotonic_boottime(&ctime);
			dtime = timespec_sub(ctime, pre_time);

			if (((dtime.tv_sec <= 20) &&
				(abs(pre_bat_temperature_val2 - bat_temperature_val) >= 5)) ||
				bat_temperature_val >= 58) {
				bm_err("[force_get_tbat][err] current:%d,%d,%d,%d,%d,%d pre:%d,%d,%d,%d,%d,%d\n",
					bat_temperature_volt_temp, bat_temperature_volt, fg_current_state,
					fg_current_temp, fg_r_value, bat_temperature_val,
					pre_bat_temperature_volt_temp, pre_bat_temperature_volt, pre_fg_current_state,
					pre_fg_current_temp, pre_fg_r_value, pre_bat_temperature_val2);
				/*pmic_auxadc_debug(1);*/
				WARN_ON(1);
			}

			pre_bat_temperature_volt_temp = bat_temperature_volt_temp;
			pre_bat_temperature_volt = bat_temperature_volt;
			pre_fg_current_temp = fg_current_temp;
			pre_fg_current_state = fg_current_state;
			pre_fg_r_value = fg_r_value;
			pre_bat_temperature_val2 = bat_temperature_val;
			pre_time = ctime;
			bm_trace("[force_get_tbat] current:%d,%d,%d,%d,%d,%d pre:%d,%d,%d,%d,%d,%d time:%d\n",
			bat_temperature_volt_temp, bat_temperature_volt, fg_current_state, fg_current_temp,
			fg_r_value, bat_temperature_val, pre_bat_temperature_volt_temp, pre_bat_temperature_volt,
			pre_fg_current_state, pre_fg_current_temp, pre_fg_r_value,
			pre_bat_temperature_val2, (int)dtime.tv_sec);
		}
	} else {
		bat_temperature_val = pre_bat_temperature_val;
	}

	return bat_temperature_val;
}

int force_get_tbat(bool update)
{
#if defined(CONFIG_POWER_EXT) || defined(FIXED_TBAT_25)
	bm_debug("[force_get_tbat] fixed TBAT=25 t\n");
	return 25;
#else
	int bat_temperature_val = 0;

	bat_temperature_val = force_get_tbat_internal(update);

	if (bat_temperature_val <= BATTERY_TMP_TO_DISABLE_GM30 && gDisableGM30 == false) {
		bm_err("battery temperature is too low %d and disable GM3.0\n", bat_temperature_val);
		disable_fg();
		if (gDisableGM30 == true)
			battery_main.BAT_CAPACITY = 50;
		battery_update(&battery_main);
	}

	if (bat_temperature_val <= BATTERY_TMP_TO_DISABLE_NAFG) {
		ntc_disable_nafg = true;
		bm_err("[force_get_tbat] ntc_disable_nafg %d %d\n", bat_temperature_val,
			DEFAULT_BATTERY_TMP_WHEN_DISABLE_NAFG);
		return DEFAULT_BATTERY_TMP_WHEN_DISABLE_NAFG;
	}

	ntc_disable_nafg = false;

	return bat_temperature_val;
#endif
}

unsigned int battery_meter_get_fg_time(void)
{
	unsigned int time;

	gauge_dev_get_time(gauge_dev, &time);
	return time;
}

unsigned int battery_meter_enable_time_interrupt(unsigned int sec)
{
	return gauge_dev_enable_time_interrupt(gauge_dev, sec);
}

/* ============================================================ */
/* Customized function */
/* ============================================================ */
int get_customized_aging_factor(int orig_af)
{
	return (orig_af + 100);
}

int get_customized_d0_c_soc(int origin_d0_c_soc)
{
	int val;

	if (Bat_EC_ctrl.debug_d0_c_en == 1)
		val = Bat_EC_ctrl.debug_d0_c_value;
	else
		val = (origin_d0_c_soc + 0);

	bm_err("[get_customized_d0_c_soc] EC_en %d EC_value %d original %d val %d\n",
		Bat_EC_ctrl.debug_d0_c_en, Bat_EC_ctrl.debug_d0_c_value, origin_d0_c_soc, val);

	return val;
}

int get_customized_d0_v_soc(int origin_d0_v_soc)
{
	int val;

	if (Bat_EC_ctrl.debug_d0_v_en == 1)
		val = Bat_EC_ctrl.debug_d0_v_value;
	else
		val = (origin_d0_v_soc + 0);

	bm_err("[get_customized_d0_c_soc] EC_en %d EC_value %d original %d val %d\n",
		Bat_EC_ctrl.debug_d0_v_en, Bat_EC_ctrl.debug_d0_v_value, origin_d0_v_soc, val);

	return val;
}

int get_customized_uisoc(int origin_uisoc)
{
	int val;

	if (Bat_EC_ctrl.debug_uisoc_en == 1)
		val = Bat_EC_ctrl.debug_uisoc_value;
	else
		val = (origin_uisoc + 0);

	bm_err("[get_customized_d0_c_soc] EC_en %d EC_value %d original %d val %d\n",
		Bat_EC_ctrl.debug_uisoc_en, Bat_EC_ctrl.debug_uisoc_value, origin_uisoc, val);

	return val;
}


/* ============================================================ */
/* Internal function */
/* ============================================================ */
void update_fg_dbg_tool_value(void)
{
	/* Todo: backup variables */
}

int fg_get_system_sec(void)
{
	struct timespec time;

	time.tv_sec = 0;
	time.tv_nsec = 0;
	get_monotonic_boottime(&time);
	return (int)time.tv_sec;
}

void bmd_ctrl_cmd_from_user(void *nl_data, struct fgd_nl_msg_t *ret_msg)
{
	struct fgd_nl_msg_t *msg;
	static int ptim_vbat, ptim_i;

	msg = nl_data;
	ret_msg->fgd_cmd = msg->fgd_cmd;

	switch (msg->fgd_cmd) {

	case FG_DAEMON_CMD_IS_BAT_PLUGOUT:
		{
			int is_bat_plugout = 0;
			int bat_plugout_time = 0;

			gauge_dev_get_boot_battery_plug_out_status(gauge_dev, &is_bat_plugout, &bat_plugout_time);
			ret_msg->fgd_data_len += sizeof(is_bat_plugout);
			memcpy(ret_msg->fgd_data, &is_bat_plugout, sizeof(is_bat_plugout));

			bm_debug("[fg_res] BATTERY_METER_CMD_GET_BOOT_BATTERY_PLUG_STATUS = %d\n", is_bat_plugout);
		}
		break;
	case FG_DAEMON_CMD_IS_BAT_EXIST:
		{
			int is_bat_exist = 0;

			is_bat_exist = pmic_is_battery_exist();

			ret_msg->fgd_data_len += sizeof(is_bat_exist);
			memcpy(ret_msg->fgd_data, &is_bat_exist, sizeof(is_bat_exist));

			bm_debug("[fg_res] FG_DAEMON_CMD_IS_BAT_EXIST = %d\n", is_bat_exist);
		}
		break;

	case FG_DAEMON_CMD_IS_BAT_CHARGING:
		{
			int is_bat_charging = 0;
			int bat_current = 0;

			/* BAT_DISCHARGING = 0 */
			/* BAT_CHARGING = 1 */
			is_bat_charging = gauge_get_current(&bat_current);

			FG_status.is_bat_charging = is_bat_charging;

			ret_msg->fgd_data_len += sizeof(is_bat_charging);
			memcpy(ret_msg->fgd_data, &is_bat_charging, sizeof(is_bat_charging));

			bm_debug("[fg_res] FG_DAEMON_CMD_IS_BAT_CHARGING = %d\n", is_bat_charging);
		}
		break;

	case FG_DAEMON_CMD_GET_CHARGER_STATUS:
		{
			int charger_status = 0;

			/* charger status need charger API */
			/* CHR_ERR = -1 */
			/* CHR_NORMAL = 0 */
			if (battery_main.BAT_STATUS == POWER_SUPPLY_STATUS_NOT_CHARGING)
				charger_status = -1;
			else
				charger_status = 0;

			ret_msg->fgd_data_len += sizeof(charger_status);
			memcpy(ret_msg->fgd_data, &charger_status, sizeof(charger_status));

			bm_debug("[fg_res] FG_DAEMON_CMD_GET_CHARGER_STATUS = %d\n", charger_status);
		}
		break;

	case FG_DAEMON_CMD_GET_FG_HW_CAR:
		{
			int fg_coulomb = 0;

			fg_coulomb = gauge_get_coulomb();
			ret_msg->fgd_data_len += sizeof(fg_coulomb);
			memcpy(ret_msg->fgd_data, &fg_coulomb, sizeof(fg_coulomb));

			bm_debug("[fg_res] BATTERY_METER_CMD_GET_FG_HW_CAR = %d\n", fg_coulomb);
		}
		break;
	case FG_DAEMON_CMD_IS_CHARGER_EXIST:
		{
			int is_charger_exist = 0;

			if (mt_get_charger_type() == CHARGER_UNKNOWN)
				is_charger_exist = false;
			else
				is_charger_exist = true;

			ret_msg->fgd_data_len += sizeof(is_charger_exist);
			memcpy(ret_msg->fgd_data, &is_charger_exist, sizeof(is_charger_exist));

			bm_debug("[fg_res] FG_DAEMON_CMD_IS_CHARGER_EXIST = %d\n", is_charger_exist);
		}
		break;


	case FG_DAEMON_CMD_GET_INIT_FLAG:
		{
			ret_msg->fgd_data_len += sizeof(init_flag);
			memcpy(ret_msg->fgd_data, &init_flag, sizeof(init_flag));
			bm_debug("[fg_res] FG_DAEMON_CMD_GET_INIT_FLAG = %d\n", init_flag);
		}
		break;

	case FG_DAEMON_CMD_SET_INIT_FLAG:
		{
			memcpy(&init_flag, &msg->fgd_data[0], sizeof(init_flag));

			bm_debug("[fg_res] FG_DAEMON_CMD_SET_INIT_FLAG = %d\n", init_flag);
		}
		break;

	case FG_DAEMON_CMD_GET_TEMPERTURE:	/* fix me */
		{
			bool update;
			int temperture = 0;

			memcpy(&update, &msg->fgd_data[0], sizeof(update));
			temperture = force_get_tbat(update);
			bm_debug("[fg_res] FG_DAEMON_CMD_GET_TEMPERTURE update = %d tmp:%d\n",
				update, temperture);
			ret_msg->fgd_data_len += sizeof(temperture);
			memcpy(ret_msg->fgd_data, &temperture, sizeof(temperture));
			/* gFG_temp = temperture; */
		}
	break;

	case FG_DAEMON_CMD_GET_RAC:
		{
			int rac;

			rac = _get_ptim_rac_val();
			ret_msg->fgd_data_len += sizeof(rac);
			memcpy(ret_msg->fgd_data, &rac, sizeof(rac));
			bm_debug("[fg_res] FG_DAEMON_CMD_GET_RAC = %d\n", rac);
		}
		break;

	case FG_DAEMON_CMD_GET_DISABLE_NAFG:
		{
			int ret = 0;

			if (ntc_disable_nafg == true)
				ret = 1;
			else
				ret = 0;
			ret_msg->fgd_data_len += sizeof(ret);
			memcpy(ret_msg->fgd_data, &ret, sizeof(ret));
			bm_debug("[fg_res] FG_DAEMON_CMD_GET_DISABLE_NAFG = %d\n", ret);
		}
		break;

	case FG_DAEMON_CMD_SET_DAEMON_PID:
		{
			memcpy(&g_fgd_pid, &msg->fgd_data[0], sizeof(g_fgd_pid));
			bm_debug("[fg_res] FG_DAEMON_CMD_SET_DAEMON_PID = %d\n", g_fgd_pid);
		}
		break;

	case FG_DAEMON_CMD_CHECK_FG_DAEMON_VERSION:
		{
			memcpy(&g_fgd_version, &msg->fgd_data[0], sizeof(g_fgd_version));
			bm_debug("[fg_res] FG_DAEMON_CMD_CHECK_FG_DAEMON_VERSION = %d\n", g_fgd_version);
			if (g_fgd_version != FGD_CHECK_VERSION)
				bm_debug("bad FG_DAEMON_VERSION 0x%x, 0x%x\n", FGD_CHECK_VERSION, g_fgd_version);
			else
				bm_debug("FG_DAEMON_VERSION OK\n");
		}
		break;

	case FG_DAEMON_CMD_GET_FG_SHUTDOWN_COND:
		{
			unsigned int shutdown_cond = get_shutdown_cond();

			ret_msg->fgd_data_len += sizeof(shutdown_cond);
			memcpy(ret_msg->fgd_data, &shutdown_cond, sizeof(shutdown_cond));

			bm_debug("[fg_res] shutdown_cond = %d\n", shutdown_cond);
		}
		break;

	case FG_DAEMON_CMD_FGADC_RESET:
		{
			bm_err("[fg_res] fgadc_reset\n");
			gauge_reset_hw();
		}
		break;

	case FG_DAEMON_CMD_GET_CUSTOM_SETTING:
		{
			memcpy(ret_msg->fgd_data, &fg_cust_data, sizeof(fg_cust_data));
			ret_msg->fgd_data_len += sizeof(fg_cust_data);

			memcpy(&ret_msg->fgd_data[ret_msg->fgd_data_len],
			       &fg_table_cust_data, sizeof(fg_table_cust_data));
			ret_msg->fgd_data_len += sizeof(fg_table_cust_data);

			bm_debug("[fg_res] data len:%d %d custom data length = %d\n",
				(int)sizeof(fg_cust_data),
				(int)sizeof(fg_table_cust_data),
				ret_msg->fgd_data_len);
		}
		break;

	case FG_DAEMON_CMD_GET_PTIM_VBAT:
		{
			unsigned int ptim_bat_vol = 0;
			signed int ptim_R_curr = 0;

			if (init_flag == 1) {
				_do_ptim();
				ptim_bat_vol = _get_ptim_bat_vol();
				ptim_R_curr = _get_ptim_R_curr();
				bm_warn("[fg_res] PTIM V %d I %d\n",
					ptim_bat_vol, ptim_R_curr);
			} else {
				ptim_bat_vol = ptim_lk_v;
				ptim_R_curr = ptim_lk_i;
				bm_warn("[fg_res] PTIM_LK V %d I %d\n",
					ptim_bat_vol, ptim_R_curr);
			}
			ptim_vbat = ptim_bat_vol;
			ptim_i = ptim_R_curr;
			ret_msg->fgd_data_len += sizeof(ptim_vbat);
			memcpy(ret_msg->fgd_data, &ptim_vbat, sizeof(ptim_vbat));
		}
		break;

	case FG_DAEMON_CMD_GET_PTIM_I:
		{
			ret_msg->fgd_data_len += sizeof(ptim_i);
			memcpy(ret_msg->fgd_data, &ptim_i, sizeof(ptim_i));
			bm_debug("[fg_res] FG_DAEMON_CMD_GET_PTIM_I = %d\n", ptim_i);
		}
		break;

	case FG_DAEMON_CMD_GET_HW_OCV:
	{
		int voltage = 0;

		battery_main.BAT_batt_temp = force_get_tbat(true);
		voltage = gauge_get_hwocv();
		FG_status.hw_ocv = voltage;

		ret_msg->fgd_data_len += sizeof(voltage);
		memcpy(ret_msg->fgd_data, &voltage, sizeof(voltage));

		bm_debug("[fg_res] FG_DAEMON_CMD_GET_HW_OCV = %d\n", voltage);
	}
	break;

	case FG_DAEMON_CMD_SET_SW_OCV:
	{
		int _sw_ocv;

		memcpy(&_sw_ocv, &msg->fgd_data[0], sizeof(_sw_ocv));
		FG_status.sw_ocv = _sw_ocv;

		bm_debug("[fg_res] FG_DAEMON_CMD_SET_SW_OCV = %d\n", _sw_ocv);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_BAT_INT1_GAP:
	{
		int fg_coulomb = 0;

		fg_coulomb = gauge_get_coulomb();

		memcpy(&fg_bat_int1_gap, &msg->fgd_data[0], sizeof(fg_bat_int1_gap));

		fg_bat_int1_ht = fg_coulomb + fg_bat_int1_gap;
		fg_bat_int1_lt = fg_coulomb - fg_bat_int1_gap;
		gauge_coulomb_start(&coulomb_plus, fg_bat_int1_gap);
		gauge_coulomb_start(&coulomb_minus, -fg_bat_int1_gap);

		bm_err("[fg_res] FG_DAEMON_CMD_SET_FG_BAT_INT1_GAP = %d car:%d\n",
			fg_bat_int1_gap, fg_coulomb);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_BAT_INT2_HT_GAP:
	{
		memcpy(&fg_bat_int2_ht, &msg->fgd_data[0], sizeof(fg_bat_int2_ht));
		gauge_coulomb_start(&soc_plus, fg_bat_int2_ht);
		bm_err("[fg_res][fg_bat_int2] FG_DAEMON_CMD_SET_FG_BAT_INT2_HT_GAP = %d\n", fg_bat_int2_ht);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_BAT_INT2_LT_GAP:
	{
		memcpy(&fg_bat_int2_lt, &msg->fgd_data[0], sizeof(fg_bat_int2_lt));
		gauge_coulomb_start(&soc_minus, -fg_bat_int2_lt);
		bm_err("[fg_res][fg_bat_int2] FG_DAEMON_CMD_SET_FG_BAT_INT2_LT_GAP = %d\n", fg_bat_int2_lt);
	}
	break;

	case FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_HT:
	{
		memcpy(&fg_bat_int2_ht_en, &msg->fgd_data[0], sizeof(fg_bat_int2_ht_en));
		if (fg_bat_int2_ht_en == 0)
			gauge_coulomb_stop(&soc_plus);
		bm_debug("[fg_res][fg_bat_int2] FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_HT = %d\n", fg_bat_int2_ht_en);
	}
	break;

	case FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_LT:
	{
		memcpy(&fg_bat_int2_lt_en, &msg->fgd_data[0], sizeof(fg_bat_int2_lt_en));
		if (fg_bat_int2_lt_en == 0)
			gauge_coulomb_stop(&soc_minus);

		bm_debug("[fg_res][fg_bat_int2] FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_LT = %d\n", fg_bat_int2_lt_en);
	}

	break;


	case FG_DAEMON_CMD_SET_FG_BAT_TMP_C_GAP:
	{
		int tmp = force_get_tbat(true);

		memcpy(&fg_bat_tmp_c_int_gap, &msg->fgd_data[0], sizeof(fg_bat_tmp_c_int_gap));

		fg_bat_tmp_c_ht = tmp + fg_bat_tmp_c_int_gap;
		fg_bat_tmp_c_lt = tmp - fg_bat_tmp_c_int_gap;

		bm_warn("[fg_res][FG_TEMP_INT] FG_DAEMON_CMD_SET_FG_BAT_TMP_C_GAP = %d ht:%d lt:%d\n",
			fg_bat_tmp_c_int_gap, fg_bat_tmp_c_ht, fg_bat_tmp_c_lt);

	}
	break;

	case FG_DAEMON_CMD_SET_FG_BAT_TMP_GAP:
	{
		int tmp = force_get_tbat(true);

		memcpy(&fg_bat_tmp_int_gap, &msg->fgd_data[0], sizeof(fg_bat_tmp_int_gap));

		fg_bat_tmp_ht = tmp + fg_bat_tmp_int_gap;
		fg_bat_tmp_lt = tmp - fg_bat_tmp_int_gap;

		bm_warn("[fg_res][FG_TEMP_INT] FG_DAEMON_CMD_SET_FG_BAT_TMP_GAP = %d ht:%d lt:%d\n", fg_bat_tmp_int_gap,
			fg_bat_tmp_ht, fg_bat_tmp_lt);

	}
	break;

	case FG_DAEMON_CMD_GET_SHUTDOWN_DURATION_TIME:
	{
		signed int time = 0;

		time = pl_shutdown_time;

		ret_msg->fgd_data_len += sizeof(time);
		memcpy(ret_msg->fgd_data, &time, sizeof(time));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_SHUTDOWN_DURATION_TIME = %d\n", time);
	}
	break;

	case FG_DAEMON_CMD_GET_BAT_PLUG_OUT_TIME:
	{
		int p1, p2;
		unsigned int time = 0;

		gauge_dev_get_boot_battery_plug_out_status(gauge_dev, &p1, &p2);
		time = p2;

		ret_msg->fgd_data_len += sizeof(time);
		memcpy(ret_msg->fgd_data, &time, sizeof(time));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_BAT_PLUG_OUT_TIME = %d\n", time);
	}
	break;

	case FG_DAEMON_CMD_GET_VBAT:
	{
		unsigned int vbat = 0;

		vbat = battery_get_bat_voltage() * 10;
		ret_msg->fgd_data_len += sizeof(vbat);
		memcpy(ret_msg->fgd_data, &vbat, sizeof(vbat));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_VBAT = %d\n", vbat);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_RESET_RTC_STATUS:
	{
		int fg_reset_rtc;

		memcpy(&fg_reset_rtc, &msg->fgd_data[0], sizeof(fg_reset_rtc));

		gauge_dev_set_reset_status(gauge_dev, fg_reset_rtc);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_FG_RESET_RTC_STATUS = %d\n", fg_reset_rtc);
	}
	break;

	case FG_DAEMON_CMD_SET_IS_FG_INITIALIZED:
	{
		int fg_reset;

		memcpy(&fg_reset, &msg->fgd_data[0], sizeof(fg_reset));
		gauge_dev_set_gauge_initialized(gauge_dev, fg_reset);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_FG_RESET_STATUS = %d\n", fg_reset);
	}
	break;


	case FG_DAEMON_CMD_GET_IS_FG_INITIALIZED:
	{
		int fg_reset;

		gauge_dev_is_gauge_initialized(gauge_dev, &fg_reset);
		ret_msg->fgd_data_len += sizeof(fg_reset);
		memcpy(ret_msg->fgd_data, &fg_reset, sizeof(fg_reset));
		bm_debug("[fg_res] BATTERY_METER_CMD_GET_FG_RESET_STATUS = %d\n", fg_reset);
	}
	break;

	case FG_DAEMON_CMD_IS_HWOCV_UNRELIABLE:
	{
		int is_hwocv_unreliable;

		is_hwocv_unreliable = FG_status.flag_hw_ocv_unreliable;
		ret_msg->fgd_data_len += sizeof(is_hwocv_unreliable);
		memcpy(ret_msg->fgd_data, &is_hwocv_unreliable, sizeof(is_hwocv_unreliable));
		bm_debug("[fg_res] FG_DAEMON_CMD_IS_HWOCV_UNRELIABLE = %d\n", is_hwocv_unreliable);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_TIME:
	{
		int secs;

		memcpy(&secs, &msg->fgd_data[0], sizeof(secs));

		if (secs != 0)
			gtimer_start(&tracking_timer, secs);
		else
			gtimer_stop(&tracking_timer);

		bm_debug("[fg_res] FG_DAEMON_CMD_SET_FG_TIME = %d\n", secs);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_TIME:
	{
		int now_time_secs;

		now_time_secs = fg_get_system_sec();
		ret_msg->fgd_data_len += sizeof(now_time_secs);
		memcpy(ret_msg->fgd_data, &now_time_secs, sizeof(now_time_secs));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_NOW_TIME = %d\n", now_time_secs);
	}
	break;

	case FG_DAEMON_CMD_GET_ZCV:
	{
		int zcv = 0;

		gauge_get_zcv(&zcv);
		ret_msg->fgd_data_len += sizeof(zcv);
		memcpy(ret_msg->fgd_data, &zcv, sizeof(zcv));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_ZCV = %d\n", zcv);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_CNT:
	{
		int cnt = 0;
		int update;

		memcpy(&update, &msg->fgd_data[0], sizeof(update));

		if (update == 1)
			gauge_dev_get_nag_cnt(gauge_dev, &cnt);
		else
			cnt = FG_status.sw_car_nafg_cnt;

		ret_msg->fgd_data_len += sizeof(cnt);
		memcpy(ret_msg->fgd_data, &cnt, sizeof(cnt));

		bm_debug("[fg_res] BATTERY_METER_CMD_GET_SW_CAR_NAFG_CNT = %d\n", cnt);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_DLTV:
	{
		int dltv = 0;
		int update;

		memcpy(&update, &msg->fgd_data[0], sizeof(update));

		if (update == 1)
			gauge_dev_get_nag_dltv(gauge_dev, &dltv);
		else
			dltv = FG_status.sw_car_nafg_dltv;

		ret_msg->fgd_data_len += sizeof(dltv);
		memcpy(ret_msg->fgd_data, &dltv, sizeof(dltv));

		bm_debug("[fg_res] BATTERY_METER_CMD_GET_SW_CAR_NAFG_DLTV = %d\n", dltv);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_C_DLTV:
	{
		int c_dltv = 0;
		int update;

		memcpy(&update, &msg->fgd_data[0], sizeof(update));

		if (update == 1)
			gauge_dev_get_nag_c_dltv(gauge_dev, &c_dltv);
		else
			c_dltv = FG_status.sw_car_nafg_c_dltv;

		ret_msg->fgd_data_len += sizeof(c_dltv);
		memcpy(ret_msg->fgd_data, &c_dltv, sizeof(c_dltv));

		bm_debug("[fg_res] BATTERY_METER_CMD_GET_SW_CAR_NAFG_C_DLTV = %d\n", c_dltv);
	}
	break;

	case FG_DAEMON_CMD_SET_NAG_ZCV_EN:
	{
		int nafg_zcv_en;

		memcpy(&nafg_zcv_en, &msg->fgd_data[0], sizeof(nafg_zcv_en));

		gauge_set_nag_en(nafg_zcv_en);

		bm_debug("[fg_res] FG_DAEMON_CMD_SET_NAG_ZCV_EN = %d\n", nafg_zcv_en);
	}
	break;

	case FG_DAEMON_CMD_SET_NAG_ZCV:
	{
		int nafg_zcv;

		memcpy(&nafg_zcv, &msg->fgd_data[0], sizeof(nafg_zcv));

		gauge_dev_set_nag_zcv(gauge_dev, nafg_zcv);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_NAG_ZCV = %d\n", nafg_zcv);
	}
	break;

	case FG_DAEMON_CMD_SET_NAG_C_DLTV:
	{
		int nafg_c_dltv;

		memcpy(&nafg_c_dltv, &msg->fgd_data[0], sizeof(nafg_c_dltv));

		gauge_dev_set_nag_c_dltv(gauge_dev, nafg_c_dltv);
		gauge_set_nag_en(1);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_NAG_C_DLTV = %d\n", nafg_c_dltv);
	}
	break;


	case FG_DAEMON_CMD_SET_ZCV_INTR:
	{
		int fg_zcv_car_th;

		memcpy(&fg_zcv_car_th, &msg->fgd_data[0], sizeof(fg_zcv_car_th));

		gauge_dev_set_zcv_interrupt_threshold(gauge_dev, fg_zcv_car_th);
		gauge_set_zcv_interrupt_en(1);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_ZCV_INTR = %d\n", fg_zcv_car_th);
	}
	break;

	case FG_DAEMON_CMD_SET_BAT_PLUGOUT_INTR:
	{
		int fg_bat_plugout_en;

		memcpy(&fg_bat_plugout_en, &msg->fgd_data[0], sizeof(fg_bat_plugout_en));

		gauge_dev_enable_bat_plugout_interrupt(gauge_dev, fg_bat_plugout_en);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_BAT_PLUGOUT_INTR_EN = %d\n", fg_bat_plugout_en);
	}
	break;

	case FG_DAEMON_CMD_SET_IAVG_INTR:
	{
		int fg_iavg_thr;

		memcpy(&fg_iavg_thr, &msg->fgd_data[0], sizeof(fg_iavg_thr));

		gauge_dev_enable_iavg_interrupt(gauge_dev, fg_iavg_thr);

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_SET_IAVG_INTR = %d\n", fg_iavg_thr);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_QUSE:/*useless*/
	{
		int fg_quse;

		memcpy(&fg_quse, &msg->fgd_data[0], sizeof(fg_quse));

		bm_debug("[fg_res] FG_DAEMON_CMD_SET_FG_QUSE = %d\n", fg_quse);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_DC_RATIO:
	{
		int fg_dc_ratio;

		memcpy(&fg_dc_ratio, &msg->fgd_data[0], sizeof(fg_dc_ratio));

		bm_debug("[fg_res] BATTERY_METER_CMD_SET_FG_DC_RATIO = %d\n", fg_dc_ratio);
	}
	break;

	case FG_DAEMON_CMD_SET_BATTERY_CYCLE_THRESHOLD:
	{
		memcpy(&bat_cycle_thr, &msg->fgd_data[0], sizeof(bat_cycle_thr));

		gauge_dev_set_battery_cycle_interrupt(gauge_dev, bat_cycle_thr);

		bm_err("[fg_res] FG_DAEMON_CMD_SET_BATTERY_CYCLE_THRESHOLD = %d\n", bat_cycle_thr);
	}
	break;

	case FG_DAEMON_CMD_SOFF_RESET:
	{
		gauge_dev_reset_shutdown_time(gauge_dev);
		bm_debug("[fg_res] BATTERY_METER_CMD_SOFF_RESET\n");
	}
	break;

	case FG_DAEMON_CMD_NCAR_RESET:
	{
		gauge_dev_reset_ncar(gauge_dev);
		bm_debug("[fg_res] BATTERY_METER_CMD_NCAR_RESET\n");
	}
	break;

	case FG_DAEMON_CMD_GET_IMIX:
	{
		int imix = UNIT_TRANS_10 * get_imix();

		ret_msg->fgd_data_len += sizeof(imix);
		memcpy(ret_msg->fgd_data, &imix, sizeof(imix));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_IMIX = %d\n", imix);
	}
	break;

	case FG_DAEMON_CMD_IS_BATTERY_CYCLE_RESET:
	{
		int reset = is_reset_battery_cycle;

		ret_msg->fgd_data_len += sizeof(reset);
		memcpy(ret_msg->fgd_data, &reset, sizeof(reset));
		bm_debug("[fg_res] FG_DAEMON_CMD_IS_BATTERY_CYCLE_RESET = %d\n", reset);
		is_reset_battery_cycle = false;
	}
	break;

	case FG_DAEMON_CMD_GET_AGING_FACTOR_CUST:
	{
		int aging_factor_cust = 0;
		int origin_aging_factor;

		memcpy(&origin_aging_factor, &msg->fgd_data[0], sizeof(origin_aging_factor));
		aging_factor_cust = get_customized_aging_factor(origin_aging_factor);

		ret_msg->fgd_data_len += sizeof(aging_factor_cust);
		memcpy(ret_msg->fgd_data, &aging_factor_cust, sizeof(aging_factor_cust));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_AGING_FACTOR_CUST = %d\n", aging_factor_cust);
	}
	break;

	case FG_DAEMON_CMD_GET_D0_C_SOC_CUST:
	{
		int d0_c_cust = 0;
		int origin_d0_c;

		memcpy(&origin_d0_c, &msg->fgd_data[0], sizeof(origin_d0_c));
		d0_c_cust = get_customized_d0_c_soc(origin_d0_c);

		ret_msg->fgd_data_len += sizeof(d0_c_cust);
		memcpy(ret_msg->fgd_data, &d0_c_cust, sizeof(d0_c_cust));
		bm_err("[fg_res] FG_DAEMON_CMD_GET_D0_C_CUST = %d\n", d0_c_cust);
	}
	break;

	case FG_DAEMON_CMD_GET_D0_V_SOC_CUST:
	{
		int d0_v_cust = 0;
		int origin_d0_v;

		memcpy(&origin_d0_v, &msg->fgd_data[0], sizeof(origin_d0_v));
		d0_v_cust = get_customized_d0_v_soc(origin_d0_v);

		ret_msg->fgd_data_len += sizeof(d0_v_cust);
		memcpy(ret_msg->fgd_data, &d0_v_cust, sizeof(d0_v_cust));
		bm_err("[fg_res] FG_DAEMON_CMD_GET_D0_V_CUST = %d\n", d0_v_cust);
	}
	break;

	case FG_DAEMON_CMD_GET_UISOC_CUST:
	{
		int uisoc_cust = 0;
		int origin_uisoc;

		memcpy(&origin_uisoc, &msg->fgd_data[0], sizeof(origin_uisoc));
		uisoc_cust = get_customized_uisoc(origin_uisoc);

		ret_msg->fgd_data_len += sizeof(uisoc_cust);
		memcpy(ret_msg->fgd_data, &uisoc_cust, sizeof(uisoc_cust));
		bm_err("[fg_res] FG_DAEMON_CMD_GET_UISOC_CUST = %d\n", uisoc_cust);
	}
	break;

	case FG_DAEMON_CMD_IS_KPOC:
	{
		int is_kpoc = bat_is_kpoc();

		ret_msg->fgd_data_len += sizeof(is_kpoc);
		memcpy(ret_msg->fgd_data, &is_kpoc, sizeof(is_kpoc));
		bm_debug("[fg_res] FG_DAEMON_CMD_IS_KPOC = %d\n", is_kpoc);
	}
	break;

	case FG_DAEMON_CMD_GET_NAFG_VBAT:
	{
		int nafg_vbat;

		gauge_dev_get_nag_vbat(gauge_dev, &nafg_vbat);
		ret_msg->fgd_data_len += sizeof(nafg_vbat);
		memcpy(ret_msg->fgd_data, &nafg_vbat, sizeof(nafg_vbat));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_NAFG_VBAT = %d\n", nafg_vbat);

		/*fg_update_nag_vbat_avg(nafg_vbat);*/
		FG_status.nafg_vbat = nafg_vbat;
	}
	break;

	case FG_DAEMON_CMD_GET_HW_INFO:
	{
		int intr_no;

		memcpy(&intr_no, &msg->fgd_data[0], sizeof(intr_no));

		intr_no = gauge_dev_get_hw_status(gauge_dev, &FG_status, intr_no);
		bm_trace("[fg_res] FG_DAEMON_CMD_GET_HW_INFO = %d\n", intr_no);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_CURRENT_AVG:
	{
		int fg_current_iavg;
		bool valid;

		fg_current_iavg = gauge_get_average_current(&valid);

		ret_msg->fgd_data_len += sizeof(fg_current_iavg);
		memcpy(ret_msg->fgd_data, &fg_current_iavg, sizeof(fg_current_iavg));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_FG_CURRENT_AVG = %d\n", fg_current_iavg);
	}
	break;

	case FG_DAEMON_CMD_GET_FG_CURRENT_IAVG_VALID:
	{
		bool valid = false;
		int iavg_valid;

		gauge_get_average_current(&valid);
		iavg_valid = valid;

		ret_msg->fgd_data_len += sizeof(iavg_valid);
		memcpy(ret_msg->fgd_data, &iavg_valid, sizeof(iavg_valid));
		bm_err("[fg_res] FG_DAEMON_CMD_GET_FG_CURRENT_IAVG_VALID = %d\n", iavg_valid);
	}
	break;


	case FG_DAEMON_CMD_SET_KERNEL_SOC:
	{
		int daemon_soc;

		memcpy(&daemon_soc, &msg->fgd_data[0], sizeof(daemon_soc));
		FG_status.soc = (daemon_soc + 50) / 100;
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_KERNEL_SOC = %d %d\n",
			daemon_soc, FG_status.soc);
	}
	break;

	case FG_DAEMON_CMD_SET_KERNEL_UISOC:
	{
		int daemon_ui_soc;

		memcpy(&daemon_ui_soc, &msg->fgd_data[0], sizeof(daemon_ui_soc));
		FG_status.ui_soc = (daemon_ui_soc + 50) / 100;
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_KERNEL_UISOC = %d %d\n",
			daemon_ui_soc, FG_status.ui_soc);

		battery_main.BAT_CAPACITY = FG_status.ui_soc;
		/* ac_update(&ac_main); */
		battery_update(&battery_main);
	}
	break;

	case FG_DAEMON_CMD_SET_KERNEL_INIT_VBAT:
	{
		int daemon_init_vbat;

		memcpy(&daemon_init_vbat, &msg->fgd_data[0], sizeof(daemon_init_vbat));
		FG_status.nafg_vbat = daemon_init_vbat;
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_KERNEL_INIT_VBAT = %d\n", daemon_init_vbat);
	}
	break;


	case FG_DAEMON_CMD_SET_FG_SHUTDOWN_COND:
	{
		int shutdown_cond;

		memcpy(&shutdown_cond, &msg->fgd_data[0], sizeof(shutdown_cond));
		set_shutdown_cond(shutdown_cond);
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_FG_SHUTDOWN_COND = %d\n", shutdown_cond);

	}
	break;

	case FG_DAEMON_CMD_ENABLE_FG_VBAT_L_INT:
	{
		int fg_vbat_l_en;

		memcpy(&fg_vbat_l_en, &msg->fgd_data[0], sizeof(fg_vbat_l_en));
		gauge_dev_enable_vbat_low_interrupt(gauge_dev, fg_vbat_l_en);
		bm_debug("[fg_res] FG_DAEMON_CMD_ENABLE_FG_VBAT_L_INT = %d\n", fg_vbat_l_en);
	}
	break;

	case FG_DAEMON_CMD_ENABLE_FG_VBAT_H_INT:
	{
		int fg_vbat_h_en;

		memcpy(&fg_vbat_h_en, &msg->fgd_data[0], sizeof(fg_vbat_h_en));
		gauge_dev_enable_vbat_high_interrupt(gauge_dev, fg_vbat_h_en);
		bm_debug("[fg_res] FG_DAEMON_CMD_ENABLE_FG_VBAT_H_INT = %d\n", fg_vbat_h_en);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_VBAT_L_TH:
	{
		int fg_vbat_l_thr;

		memcpy(&fg_vbat_l_thr, &msg->fgd_data[0], sizeof(fg_vbat_l_thr));
		gauge_dev_enable_vbat_low_threshold(gauge_dev, fg_vbat_l_thr);
		set_shutdown_vbat_lt(fg_vbat_l_thr, fg_cust_data.vbat2_det_voltage1);
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_FG_VBAT_L_TH = %d\n", fg_vbat_l_thr);
	}
	break;

	case FG_DAEMON_CMD_SET_FG_VBAT_H_TH:
	{
		int fg_vbat_h_thr;

		memcpy(&fg_vbat_h_thr, &msg->fgd_data[0], sizeof(fg_vbat_h_thr));
		gauge_dev_enable_vbat_high_threshold(gauge_dev, fg_vbat_h_thr);
		bm_debug("[fg_res] FG_DAEMON_CMD_SET_FG_VBAT_H_TH = %d\n", fg_vbat_h_thr);
	}
	break;

	case FG_DAEMON_CMD_SET_CAR_TUNE_VALUE:
	{
		signed int cali_car_tune;

		memcpy(&cali_car_tune, &msg->fgd_data[0], sizeof(cali_car_tune));
#ifdef CALIBRATE_CAR_TUNE_VALUE_BY_META_TOOL
		bm_notice("[fg_res] cali_car_tune = %d, default = %d, Use [cali_car_tune]\n",
			cali_car_tune, fg_cust_data.car_tune_value);
		fg_cust_data.car_tune_value = cali_car_tune;
#else
		bm_notice("[fg_res] cali_car_tune = %d, default = %d, Use [default]\n",
			cali_car_tune, fg_cust_data.car_tune_value);
#endif
	}
	break;

	case FG_DAEMON_CMD_PRINT_LOG:
	{
		bm_err("%s", &msg->fgd_data[0]);
	}
	break;

	case FG_DAEMON_CMD_DUMP_LOG:
	{
		proc_subcmd = msg->fgd_subcmd;
		proc_subcmd_para1 = msg->fgd_subcmd_para1;
		memset(proc_log, 0, 4096);
		strncpy(proc_log, &msg->fgd_data[0], strlen(&msg->fgd_data[0]));
		bm_err("[fg_res] FG_DAEMON_CMD_DUMP_LOG %d %d %d\n", msg->fgd_subcmd, msg->fgd_subcmd_para1,
			(int)strlen(&msg->fgd_data[0]));
	}
	break;

	case FG_DAEMON_CMD_GET_RTC_UI_SOC:
	{
		int rtc_ui_soc;

		gauge_dev_get_rtc_ui_soc(gauge_dev, &rtc_ui_soc);

		ret_msg->fgd_data_len += sizeof(rtc_ui_soc);
		memcpy(ret_msg->fgd_data, &rtc_ui_soc, sizeof(rtc_ui_soc));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_RTC_UI_SOC = %d\n", rtc_ui_soc);
	}
	break;

	case FG_DAEMON_CMD_SET_RTC_UI_SOC:
	{
		int rtc_ui_soc;

		memcpy(&rtc_ui_soc, &msg->fgd_data[0], sizeof(rtc_ui_soc));
		gauge_dev_set_rtc_ui_soc(gauge_dev, rtc_ui_soc);
		bm_debug("[fg_res] BATTERY_METER_CMD_SET_RTC_UI_SOC = %d\n", rtc_ui_soc);
	}
	break;

	case FG_DAEMON_CMD_GET_RTC_TWO_SEC_REBOOT:
	{
		int two_sec_reboot_flag;

		two_sec_reboot_flag = pl_two_sec_reboot;
		ret_msg->fgd_data_len += sizeof(two_sec_reboot_flag);
		memcpy(ret_msg->fgd_data, &two_sec_reboot_flag, sizeof(two_sec_reboot_flag));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_RTC_TWO_SEC_REBOOT = %d\n", two_sec_reboot_flag);
	}
	break;

	case FG_DAEMON_CMD_GET_RTC_INVALID:
	{
		int rtc_invalid;

		gauge_dev_is_rtc_invalid(gauge_dev, &rtc_invalid);

		ret_msg->fgd_data_len += sizeof(rtc_invalid);
		memcpy(ret_msg->fgd_data, &rtc_invalid, sizeof(rtc_invalid));
		bm_debug("[fg_res] FG_DAEMON_CMD_GET_RTC_INVALID = %d\n", rtc_invalid);
	}
	break;

	default:
		bm_err("bad FG_DAEMON_CTRL_CMD_FROM_USER 0x%x\n", msg->fgd_cmd);
		break;
	}			/* switch() */

}

static void nl_send_to_user(u32 pid, int seq, struct fgd_nl_msg_t *reply_msg)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	/* int size=sizeof(struct fgd_nl_msg_t); */
	int size = reply_msg->fgd_data_len + FGD_NL_MSG_T_HDR_LEN;
	int len = NLMSG_SPACE(size);
	void *data;
	int ret;

	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb)
		return;

	nlh = nlmsg_put(skb, pid, seq, 0, size, 0);
	data = NLMSG_DATA(nlh);
	memcpy(data, reply_msg, size);
	NETLINK_CB(skb).portid = 0;	/* from kernel */
	NETLINK_CB(skb).dst_group = 0;	/* unicast */

	ret = netlink_unicast(daemo_nl_sk, skb, pid, MSG_DONTWAIT);
	if (ret < 0) {
		bm_err("[Netlink] send failed %d\n", ret);
		return;
	}
	/*bm_debug("[Netlink] reply_user: netlink_unicast- ret=%d\n", ret); */


}


static void nl_data_handler(struct sk_buff *skb)
{
	u32 pid;
	kuid_t uid;
	int seq;
	void *data;
	struct nlmsghdr *nlh;
	struct fgd_nl_msg_t *fgd_msg, *fgd_ret_msg;
	int size = 0;

	nlh = (struct nlmsghdr *)skb->data;
	pid = NETLINK_CREDS(skb)->pid;
	uid = NETLINK_CREDS(skb)->uid;
	seq = nlh->nlmsg_seq;

	/*bm_debug("[Netlink] recv skb from user space uid:%d pid:%d seq:%d\n",uid,pid,seq); */
	data = NLMSG_DATA(nlh);

	fgd_msg = (struct fgd_nl_msg_t *)data;

	size = fgd_msg->fgd_ret_data_len + FGD_NL_MSG_T_HDR_LEN;

	fgd_ret_msg = vmalloc(size);
	if (!fgd_ret_msg) {
		/* bm_err("Error: nl_data_handler() vmalloc fail!!!\n"); */
		return;
	}

	memset(fgd_ret_msg, 0, size);

	bmd_ctrl_cmd_from_user(data, fgd_ret_msg);
	nl_send_to_user(pid, seq, fgd_ret_msg);

	vfree(fgd_ret_msg);
}

int wakeup_fg_algo(unsigned int flow_state)
{
	update_fg_dbg_tool_value();

	if (gDisableGM30) {
		bm_err("FG daemon is disabled\n");
		return -1;
	}

	if (g_fgd_pid != 0) {
		struct fgd_nl_msg_t *fgd_msg;
		int size = FGD_NL_MSG_T_HDR_LEN + sizeof(flow_state);

		fgd_msg = vmalloc(size);
		if (!fgd_msg) {
			/* bm_err("Error: wakeup_fg_algo() vmalloc fail!!!\n"); */
			return -1;
		}

		bm_debug("[wakeup_fg_algo] malloc size=%d pid=%d cmd:%d\n", size, g_fgd_pid, flow_state);
		memset(fgd_msg, 0, size);
		fgd_msg->fgd_cmd = FG_DAEMON_CMD_NOTIFY_DAEMON;
		memcpy(fgd_msg->fgd_data, &flow_state, sizeof(flow_state));
		fgd_msg->fgd_data_len += sizeof(flow_state);
		nl_send_to_user(g_fgd_pid, 0, fgd_msg);
		vfree(fgd_msg);
		return 0;
	} else {
		return -1;
	}
}

int wakeup_fg_algo_cmd(unsigned int flow_state, int cmd, int para1)
{
	update_fg_dbg_tool_value();

	if (gDisableGM30) {
		bm_err("FG daemon is disabled\n");
		return -1;
	}

	if (g_fgd_pid != 0) {
		struct fgd_nl_msg_t *fgd_msg;
		int size = FGD_NL_MSG_T_HDR_LEN + sizeof(flow_state);

		fgd_msg = vmalloc(size);
		if (!fgd_msg) {
			/* bm_err("Error: wakeup_fg_algo() vmalloc fail!!!\n"); */
			return -1;
		}

		bm_debug("[wakeup_fg_algo] malloc size=%d pid=%d cmd:%d\n", size, g_fgd_pid, flow_state);
		memset(fgd_msg, 0, size);
		fgd_msg->fgd_cmd = FG_DAEMON_CMD_NOTIFY_DAEMON;
		fgd_msg->fgd_subcmd = cmd;
		fgd_msg->fgd_subcmd_para1 = para1;
		memcpy(fgd_msg->fgd_data, &flow_state, sizeof(flow_state));
		fgd_msg->fgd_data_len += sizeof(flow_state);
		nl_send_to_user(g_fgd_pid, 0, fgd_msg);
		vfree(fgd_msg);
		return 0;
	} else {
		return -1;
	}
}

int wakeup_fg_algo_atomic(unsigned int flow_state)
{
	update_fg_dbg_tool_value();

	if (gDisableGM30) {
		bm_err("FG daemon is disabled\n");
		return -1;
	}

	if (g_fgd_pid != 0) {
		struct fgd_nl_msg_t *fgd_msg;
		int size = FGD_NL_MSG_T_HDR_LEN + sizeof(flow_state);

		fgd_msg = kmalloc(size, GFP_ATOMIC);
		if (!fgd_msg) {
			/* bm_err("Error: wakeup_fg_algo() vmalloc fail!!!\n"); */
			return -1;
		}

		bm_debug("[wakeup_fg_algo] malloc size=%d pid=%d cmd:%d\n", size, g_fgd_pid, flow_state);
		memset(fgd_msg, 0, size);
		fgd_msg->fgd_cmd = FG_DAEMON_CMD_NOTIFY_DAEMON;
		memcpy(fgd_msg->fgd_data, &flow_state, sizeof(flow_state));
		fgd_msg->fgd_data_len += sizeof(flow_state);
		nl_send_to_user(g_fgd_pid, 0, fgd_msg);
		kfree(fgd_msg);
		return 0;
	} else {
		return -1;
	}
}


int fg_get_battery_temperature_for_zcv(void)
{
	return battery_main.BAT_batt_temp;
}


void fg_nafg_int_handler(void)
{
	int nafg_en = 0;
	signed int nafg_cnt = 0;
	signed int nafg_dltv = 0;
	signed int nafg_c_dltv = 0;

	/* 1. Get SW Car value */
	gauge_dev_get_nag_cnt(gauge_dev, &nafg_cnt);
	gauge_dev_get_nag_dltv(gauge_dev, &nafg_dltv);
	gauge_dev_get_nag_c_dltv(gauge_dev, &nafg_c_dltv);

	FG_status.sw_car_nafg_cnt = nafg_cnt;
	FG_status.sw_car_nafg_dltv = nafg_dltv;
	FG_status.sw_car_nafg_c_dltv = nafg_c_dltv;

	bm_err("[fg_nafg_int_handler][fg_bat_nafg] [%d:%d:%d]\n", nafg_cnt, nafg_dltv, nafg_c_dltv);
	battery_dump_nag();

	/* 2. Stop HW interrupt*/
	gauge_set_nag_en(nafg_en);

	/* 3. Notify fg daemon */
	wakeup_fg_algo(FG_INTR_NAG_C_DLTV);
}

void fg_bat_temp_int_init(void)
{
#if defined(CONFIG_MTK_DISABLE_GAUGE) || defined(CONFIG_POWER_EXT) || defined(FIXED_TBAT_25)
	return;
#else
	int tmp = force_get_tbat(true);
	int fg_bat_new_ht, fg_bat_new_lt;

	fg_bat_new_ht = TempToBattVolt(tmp + 1, 1);
	fg_bat_new_lt = TempToBattVolt(tmp - 1, 0);

	gauge_dev_enable_battery_tmp_lt_interrupt(gauge_dev, false, 0);
	gauge_dev_enable_battery_tmp_ht_interrupt(gauge_dev, false, 0);
	gauge_dev_enable_battery_tmp_lt_interrupt(gauge_dev, true, fg_bat_new_lt);
	gauge_dev_enable_battery_tmp_ht_interrupt(gauge_dev, true, fg_bat_new_ht);
#endif
}

void fg_bat_temp_int_internal(void)
{
#if defined(CONFIG_MTK_DISABLE_GAUGE) || defined(CONFIG_POWER_EXT) || defined(FIXED_TBAT_25)
	battery_main.BAT_batt_temp = 25;
	battery_update(&battery_main);
	return;
#else
	int tmp = force_get_tbat(true);
	int fg_bat_new_ht, fg_bat_new_lt;

	gauge_dev_enable_battery_tmp_lt_interrupt(gauge_dev, false, 0);
	gauge_dev_enable_battery_tmp_ht_interrupt(gauge_dev, false, 0);


	if (tmp >= fg_bat_tmp_c_ht)
		wakeup_fg_algo(FG_INTR_BAT_TMP_C_HT);
	else if (tmp <= fg_bat_tmp_c_lt)
		wakeup_fg_algo(FG_INTR_BAT_TMP_C_LT);

	if (tmp >= fg_bat_tmp_ht)
		wakeup_fg_algo(FG_INTR_BAT_TMP_HT);
	else if (tmp <= fg_bat_tmp_lt)
		wakeup_fg_algo(FG_INTR_BAT_TMP_LT);

	fg_bat_new_ht = TempToBattVolt(tmp + 1, 1);
	fg_bat_new_lt = TempToBattVolt(tmp - 1, 0);

	if (fixed_bat_tmp == 0xffff) {
		gauge_dev_enable_battery_tmp_lt_interrupt(gauge_dev, true, fg_bat_new_lt);
		gauge_dev_enable_battery_tmp_ht_interrupt(gauge_dev, true, fg_bat_new_ht);
	}
	bm_err("[fg_bat_temp_int_internal][FG_TEMP_INT] T[%d] V[%d %d] C[%d %d] h[%d %d]\n",
		tmp, fg_bat_tmp_ht, fg_bat_tmp_lt, fg_bat_tmp_c_ht, fg_bat_tmp_c_lt,
		fg_bat_new_lt, fg_bat_new_ht);

	battery_main.BAT_batt_temp = tmp;
	battery_update(&battery_main);
#endif
}

void fg_bat_temp_int_l_handler(void)
{
	bm_err("[fg_bat_temp_int_l_handler]\n");
	fg_bat_temp_int_internal();
}

void fg_bat_temp_int_h_handler(void)
{
	bm_err("[fg_bat_temp_int_h_handler]\n");
	fg_bat_temp_int_internal();
}

void fg_bat_sw_temp_int_l_handler(void)
{
	bm_err("[fg_bat_sw_temp_int_l_handler]\n");
	fg_bat_temp_int_internal();
}

void fg_bat_sw_temp_int_h_handler(void)
{
	bm_err("[fg_bat_sw_temp_int_h_handler]\n");
	fg_bat_temp_int_internal();
}

void fg_bat_temp_int_sw_check(void)
{
	int tmp = force_get_tbat(true);

	bm_err("[fg_bat_temp_int_sw_check] tmp %d lt %d ht %d\n", tmp, fg_bat_tmp_lt, fg_bat_tmp_ht);

	if (tmp >= fg_bat_tmp_ht)
		fg_bat_sw_temp_int_h_handler();
	else if (tmp <= fg_bat_tmp_lt)
		fg_bat_sw_temp_int_l_handler();
}

void fg_update_iavg(void)
{
	struct timespec now_time, diff;
	int fg_coulomb;

	get_monotonic_boottime(&now_time);

	diff = timespec_sub(now_time, sw_iavg_time);
	bm_debug("[fg_update_iavg]diff time:%ld\n", diff.tv_sec);
	if (diff.tv_sec >= 60) {
		fg_coulomb = gauge_get_coulomb();
		bm_debug("[fg_update_iavg]time:%ld car:%d %d iavg:%d\n",
			diff.tv_sec, fg_coulomb, sw_iavg_car, sw_iavg);
		sw_iavg = (fg_coulomb - sw_iavg_car) * 60;
		sw_iavg_time = now_time;
		sw_iavg_car = fg_coulomb;
	}
}

void fg_drv_update_hw_status(void)
{
	static bool fg_current_state;
	signed int chr_vol;
	int fg_current, fg_coulomb, bat_vol, hwocv, plugout_status, tmp, bat_plugout_time;
	int fg_current_iavg;
	bool valid = false;

	bm_debug("[fg_drv_update_hw_status]=>\n");

#ifdef GAUGE_COULOMB_INTERRUPT_TEST
		if (list_empty(&coulomb_test1.list) == true)
			gauge_coulomb_start(&coulomb_test1, -5);

		if (list_empty(&coulomb_test2.list) == true)
			gauge_coulomb_start(&coulomb_test2, -10);
#endif
	fg_update_iavg();

	gauge_dev_get_boot_battery_plug_out_status(gauge_dev, &plugout_status, &bat_plugout_time);

	fg_current_state = gauge_get_current(&fg_current);
	fg_coulomb = gauge_get_coulomb();
	bat_vol = pmic_get_battery_voltage();
	chr_vol = pmic_get_vbus();
	hwocv = gauge_get_hwocv();
	tmp = force_get_tbat(true);

	bm_err("car[%d,%ld,%ld,%ld,%ld] c:%d %d vbat:%d vbus:%d hwocv:%d soc:%d %d gm3:%d %d %d\n",
		fg_coulomb, coulomb_plus.end, coulomb_minus.end, soc_plus.end, soc_minus.end,
		fg_current_state, fg_current, bat_vol, chr_vol, hwocv,
		battery_get_bat_soc(), battery_get_bat_uisoc(),
		gDisableGM30, fg_cust_data.disable_nafg, ntc_disable_nafg);

	if (bat_get_debug_level() >= 7) {
		gauge_coulomb_dump_list();
		gtimer_dump_list();
	}

	fg_current_iavg = gauge_get_average_current(&valid);


	bm_err("tmp:%d %d %d hcar2:%d lcar2:%d time:%d sw_iavg:%d %d %d\n",
		tmp, fg_bat_tmp_int_ht, fg_bat_tmp_int_lt,
		fg_bat_int2_ht, fg_bat_int2_lt,
		fg_get_system_sec(), sw_iavg, fg_current_iavg, valid);


	wakeup_fg_algo_cmd(FG_INTR_KERNEL_CMD, FG_KERNEL_CMD_DUMP_REGULAR_LOG, 0);

}

void fg_iavg_int_ht_handler(void)
{
	FG_status.iavg_intr_flag = 0;
	pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_H, 0);
	pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_L, 0);
	pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");
	bm_err("[FGADC_intr_end][fg_iavg_int_ht_handler] iavg_intr_flag %d\n",
		FG_status.iavg_intr_flag);

	wakeup_fg_algo(FG_INTR_IAVG);

	fg_bat_temp_int_sw_check();
}

void fg_iavg_int_lt_handler(void)
{
	FG_status.iavg_intr_flag = 0;
	pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_H, 0);
	pmic_set_register_value(PMIC_RG_INT_EN_FG_IAVG_L, 0);
	pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");
	bm_err("[FGADC_intr_end][fg_iavg_int_lt_handler] iavg_intr_flag %d\n",
		FG_status.iavg_intr_flag);

	wakeup_fg_algo(FG_INTR_IAVG);

	fg_bat_temp_int_sw_check();
}

void fg_cycle_int_handler(void)
{
	wakeup_fg_algo(FG_INTR_BAT_CYCLE);
	fg_bat_temp_int_sw_check();
}

int fg_bat_int1_h_handler(struct gauge_consumer *consumer)
{
	int fg_coulomb = 0;

	fg_coulomb = gauge_get_coulomb();

	fg_bat_int1_ht = fg_coulomb + fg_bat_int1_gap;
	fg_bat_int1_lt = fg_coulomb - fg_bat_int1_gap;

	gauge_coulomb_start(&coulomb_plus, fg_bat_int1_gap);
	gauge_coulomb_start(&coulomb_minus, -fg_bat_int1_gap);

	bm_err("[fg_bat_int1_h_handler] car:%d ht:%d lt:%d gap:%d\n",
		fg_coulomb, fg_bat_int1_ht, fg_bat_int1_lt, fg_bat_int1_gap);

	wakeup_fg_algo(FG_INTR_BAT_INT1_HT);

	fg_bat_temp_int_sw_check();
	return 0;
}

int fg_bat_int1_l_handler(struct gauge_consumer *consumer)
{
	int fg_coulomb = 0;

	fg_coulomb = gauge_get_coulomb();

	fg_bat_int1_ht = fg_coulomb + fg_bat_int1_gap;
	fg_bat_int1_lt = fg_coulomb - fg_bat_int1_gap;

	gauge_coulomb_start(&coulomb_plus, fg_bat_int1_gap);
	gauge_coulomb_start(&coulomb_minus, -fg_bat_int1_gap);

	bm_err("[fg_bat_int1_l_handler] car:%d ht:%d lt:%d gap:%d\n",
		fg_coulomb, fg_bat_int1_ht, fg_bat_int1_lt, fg_bat_int1_gap);
	wakeup_fg_algo(FG_INTR_BAT_INT1_LT);

	fg_bat_temp_int_sw_check();
	return 0;
}

int fg_bat_int2_h_handler(struct gauge_consumer *consumer)
{
	int fg_coulomb = 0;

	pmic_get_register_value(0x2004);
	fg_coulomb = gauge_get_coulomb();
	bm_err("[fg_bat_int2_h_handler] car:%d ht:%d\n",
		fg_coulomb, fg_bat_int2_ht);

	bm_debug("[fg_bat_int2_h_handler] FGADC_CON2 0x%x INT_CON5 0x%x\n",
		upmu_get_reg_value(0x2004),
		upmu_get_reg_value(0x081E));
	bm_debug("[fg_bat_int2_h_handler] car31 0x%x car16 0x%x\n",
		upmu_get_reg_value(0x2016),
		upmu_get_reg_value(0x2018));
	bm_debug("[fg_bat_int2_h_handler] LTH31 0x%x LTH16 0x%x\n",
		upmu_get_reg_value(0x201A),
		upmu_get_reg_value(0x201C));
	bm_debug("[fg_bat_int2_h_handler] HTH31 0x%x HTH16 0x%x\n",
		upmu_get_reg_value(0x201E),
		upmu_get_reg_value(0x2020));


	wakeup_fg_algo(FG_INTR_BAT_INT2_HT);

	fg_bat_temp_int_sw_check();
	return 0;
}

int fg_bat_int2_l_handler(struct gauge_consumer *consumer)
{
	int fg_coulomb = 0;

	fg_coulomb = gauge_get_coulomb();
	bm_err("[fg_bat_int2_l_handler] car:%d lt:%d\n",
		fg_coulomb, fg_bat_int2_lt);

	bm_debug("[fg_bat_int2_l_handler] FGADC_CON2 0x%x INT_CON5 0x%x\n",
		upmu_get_reg_value(0x2004),
		upmu_get_reg_value(0x081E));
	bm_debug("[fg_bat_int2_l_handler] car31 0x%x car16 0x%x\n",
		upmu_get_reg_value(0x2016),
		upmu_get_reg_value(0x2018));
	bm_debug("[fg_bat_int2_l_handler] LTH31 0x%x LTH16 0x%x\n",
		upmu_get_reg_value(0x201A),
		upmu_get_reg_value(0x201C));
	bm_debug("[fg_bat_int2_l_handler] HTH31 0x%x HTH16 0x%x\n",
		upmu_get_reg_value(0x201E),
		upmu_get_reg_value(0x2020));

	wakeup_fg_algo(FG_INTR_BAT_INT2_LT);

	fg_bat_temp_int_sw_check();
	return 0;
}


void fg_zcv_int_handler(void)
{
	int fg_coulomb = 0;
	int zcv_intr_en = 0;
	int zcv_intr_curr = 0;
	int zcv;

	fg_coulomb = gauge_get_coulomb();
	gauge_get_zcv_current(&zcv_intr_curr);
	gauge_get_zcv(&zcv);
	bm_err("[fg_zcv_int_handler] car:%d zcv_curr:%d zcv:%d\n",	fg_coulomb, zcv_intr_curr, zcv);

	if (abs(zcv_intr_curr) < SLEEP_CURRENT_AVG) {
		wakeup_fg_algo(FG_INTR_FG_ZCV);
		zcv_intr_en = 0;
		gauge_set_zcv_interrupt_en(zcv_intr_en);
	}

	bm_debug("[fg_zcv_int_handler] DET_IV %d 15_00 0x%x 30_16 0x%x\n",
		pmic_get_register_value(PMIC_FG_ZCV_DET_IV),
		pmic_get_register_value(PMIC_FG_ZCV_CAR_TH_15_00),
		pmic_get_register_value(PMIC_FG_ZCV_CAR_TH_30_16));

	fg_bat_temp_int_sw_check();
}

void fg_bat_plugout_int_handler(void)
{
	int is_bat_exist;

	bm_err("[fg_bat_plugout_int_handler]\n");
	battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_UNKNOWN;
	wakeup_fg_algo(FG_INTR_BAT_PLUGOUT);
	battery_update(&battery_main);

	fg_bat_temp_int_sw_check();

	is_bat_exist = pmic_is_battery_exist();
	bm_err("[fg_bat_plugout_int_handler] is_bat_exist %d\n", is_bat_exist);

	if (is_bat_exist == 0)
		kernel_power_off();
}

static CHARGER_TYPE chr_type;

void fg_charger_in_handler(void)
{
	CHARGER_TYPE current_chr_type;

	current_chr_type = mt_get_charger_type();

	bm_debug("[fg_charger_in_handler] notify daemon %d %d\n", chr_type, current_chr_type);

	if (current_chr_type != CHARGER_UNKNOWN) {
		if (chr_type == CHARGER_UNKNOWN)
			wakeup_fg_algo_atomic(FG_INTR_CHARGER_IN);
	}
	chr_type = current_chr_type;
}

void fg_vbat2_l_int_handler(void)
{
	int lt_ht_en = 0;

	bm_err("[fg_vbat2_l_int_handler]\n");
	gauge_dev_enable_vbat_high_interrupt(gauge_dev, lt_ht_en);
	gauge_dev_enable_vbat_low_interrupt(gauge_dev, lt_ht_en);
	wakeup_fg_algo(FG_INTR_VBAT2_L);

	fg_bat_temp_int_sw_check();
}

void fg_vbat2_h_int_handler(void)
{
	int lt_ht_en = 0;

	bm_err("[fg_vbat2_h_int_handler]\n");
	gauge_dev_enable_vbat_high_interrupt(gauge_dev, lt_ht_en);
	gauge_dev_enable_vbat_low_interrupt(gauge_dev, lt_ht_en);
	disable_shutdown_cond(LOW_BAT_VOLT);
	wakeup_fg_algo(FG_INTR_VBAT2_H);

	fg_bat_temp_int_sw_check();
}

void fg_chr_full_int_handler(void)
{
	bm_err("[fg_chr_full_int_handler]\n");
	wakeup_fg_algo(FG_INTR_CHR_FULL);

	fg_bat_temp_int_sw_check();
}

void fg_shutdown_int_handler(void)
{
	bm_err("[fg_shutdown_int_handler]\n");
	wakeup_fg_algo(FG_INTR_SHUTDOWN);
}

void fg_dlpt_sd_handler(void)
{
	bm_err("[fg_dlpt_sd_handler]\n");
	wakeup_fg_algo(FG_INTR_DLPT_SD);
}

/* Extern to external usage */
void notify_fg_dlpt_sd(void)
{
	fg_dlpt_sd_handler();
}

void notify_fg_shutdown(void)
{
	fg_shutdown_int_handler();
}

void notify_fg_chr_full(void)
{
	fg_chr_full_int_handler();
}

int tracking_timer_callback(struct gtimer *gtimer)
{
	wakeup_fg_algo(FG_INTR_FG_TIME);
	return 0;
}

int battery_get_charger_zcv(void)
{
	u32 zcv = 0;

	charger_manager_get_zcv(pbat_consumer, MAIN_CHARGER, &zcv);
	return zcv;
}

#ifdef BAT_PSEUDO_THREAD
int battery_update_routine(void *x)
{
	ktime_t ktime = ktime_set(10, 0);
	int temp_intr_toggle = 0;

	battery_update_psd(&battery_main);

	while (1) {
		wait_event(fg_update_wq, (fg_update_flag > 0));

		fg_drv_update_hw_status();

		/*********************/

		if (temp_intr_toggle > 10) {
			temp_intr_toggle = 0;
		} else {
			/*fg_drv_send_intr(FG_INTR_FG_TIME);*/
			temp_intr_toggle++;
		}
		/*********************/
		fg_update_flag = 0;

		if (bat_get_debug_level() >= BMLOG_DEBUG_LEVEL)
			ktime = ktime_set(10, 0);
		else
			ktime = ktime_set(60, 0);

		hrtimer_start(&fg_drv_thread_hrtimer, ktime, HRTIMER_MODE_REL);
	}
}

void fg_update_routine_wakeup(void)
{
	fg_update_flag = 1;
	wake_up(&fg_update_wq);
}

enum hrtimer_restart fg_drv_thread_hrtimer_func(struct hrtimer *timer)
{
	fg_update_routine_wakeup();

	return HRTIMER_NORESTART;
}

void fg_drv_thread_hrtimer_init(void)
{
	ktime_t ktime = ktime_set(10, 0);

	hrtimer_init(&fg_drv_thread_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fg_drv_thread_hrtimer.function = fg_drv_thread_hrtimer_func;
	hrtimer_start(&fg_drv_thread_hrtimer, ktime, HRTIMER_MODE_REL);
}
#endif

void exec_BAT_EC(int cmd, int param)
{
	bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d\n", cmd, param);
	switch (cmd) {
	case 101:
		{
			/* Force Temperature, force_get_tbat*/
			if (param == 0xff) {
				Bat_EC_ctrl.fixed_temp_en = 0;
				Bat_EC_ctrl.fixed_temp_value = 25;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.fixed_temp_en = 1;
				if (param >= 100)
					Bat_EC_ctrl.fixed_temp_value = 0 - (param - 100);
				else
					Bat_EC_ctrl.fixed_temp_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 102:
		{
			/* force PTIM RAC */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_rac_en = 0;
				Bat_EC_ctrl.debug_rac_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_rac_en = 1;
				Bat_EC_ctrl.debug_rac_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 103:
		{
			/* force PTIM V */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_ptim_v_en = 0;
				Bat_EC_ctrl.debug_ptim_v_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_ptim_v_en = 1;
				Bat_EC_ctrl.debug_ptim_v_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}

		}
		break;
	case 104:
		{
			/* force PTIM R_current */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_ptim_r_en = 0;
				Bat_EC_ctrl.debug_ptim_r_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_ptim_r_en = 1;
				Bat_EC_ctrl.debug_ptim_r_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 105:
		{
			/* force interrupt trigger */
			switch (param) {
			case 1:
				{
					wakeup_fg_algo(FG_INTR_TIMER_UPDATE);
				}
				break;
			case 4096:
				{
					wakeup_fg_algo(FG_INTR_NAG_C_DLTV);
				}
				break;
			case 8192:
				{
					wakeup_fg_algo(FG_INTR_FG_ZCV);
				}
				break;
			case 32768:
				{
					wakeup_fg_algo(FG_INTR_RESET_NVRAM);
				}
				break;
			case 65536:
				{
					wakeup_fg_algo(FG_INTR_BAT_PLUGOUT);
				}
				break;


			default:
				{

				}
				break;
			}
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, force interrupt\n", cmd, param);
		}
		break;
	case 106:
		{
			/* force FG Current */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_fg_curr_en = 0;
				Bat_EC_ctrl.debug_fg_curr_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_fg_curr_en = 1;
				Bat_EC_ctrl.debug_fg_curr_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 107:
		{
			/* force Battery ID */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_bat_id_en = 0;
				Bat_EC_ctrl.debug_bat_id_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);
			} else {
				Bat_EC_ctrl.debug_bat_id_en = 1;
				Bat_EC_ctrl.debug_bat_id_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
				fg_custom_init_from_header();
			}
		}
		break;
	case 108:
		{
			/* Set D0_C_CUST */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_d0_c_en = 0;
				Bat_EC_ctrl.debug_d0_c_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_d0_c_en = 1;
				Bat_EC_ctrl.debug_d0_c_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 109:
		{
			/* Set D0_V_CUST */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_d0_v_en = 0;
				Bat_EC_ctrl.debug_d0_v_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_d0_v_en = 1;
				Bat_EC_ctrl.debug_d0_v_value = param;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;
	case 110:
		{
			/* Set UISOC_CUST */
			if (param == 0xff) {
				Bat_EC_ctrl.debug_uisoc_en = 0;
				Bat_EC_ctrl.debug_uisoc_value = 0;
				bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, disable\n", cmd, param);

			} else {
				Bat_EC_ctrl.debug_uisoc_en = 1;
				Bat_EC_ctrl.debug_uisoc_value = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, enable\n", cmd, param);
			}
		}
		break;

	case 701:
		{
			fg_cust_data.pseudo1_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, pseudo1_en\n", cmd, param);
		}
		break;
	case 702:
		{
			fg_cust_data.pseudo100_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, pseudo100_en\n", cmd, param);
		}
		break;
	case 703:
		{
			fg_cust_data.qmax_sel = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, qmax_sel\n", cmd, param);
		}
		break;
	case 704:
		{
			fg_cust_data.iboot_sel = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, iboot_sel\n", cmd, param);
		}
		break;
	case 705:
		{
			fg_cust_data.pmic_min_vol = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, pmic_min_vol\n", cmd, param);
		}
		break;
	case 706:
		{
			fg_cust_data.poweron_system_iboot = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, poweron_system_iboot\n", cmd, param);
		}
		break;
	case 707:
		{
			fg_cust_data.shutdown_system_iboot = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, shutdown_system_iboot\n", cmd, param);
		}
		break;
	case 708:
		{
			fg_cust_data.fg_meter_resistance = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, fg_meter_resistance\n", cmd, param);
		}
		break;
	case 709:
		{
			fg_cust_data.r_fg_value = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, r_fg_value\n", cmd, param);
		}
		break;
	case 710:
		{
			fg_cust_data.q_max_sys_voltage = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, q_max_sys_voltage\n", cmd, param);
		}
		break;
	case 711:
		{
			fg_cust_data.loading_1_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, loading_1_en\n", cmd, param);
		}
		break;
	case 712:
		{
			fg_cust_data.loading_2_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, loading_2_en\n", cmd, param);
		}
		break;
	case 713:
		{
			fg_cust_data.aging_temp_diff = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, aging_temp_diff\n", cmd, param);
		}
		break;
	case 714:
		{
			fg_cust_data.aging1_load_soc = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, aging1_load_soc\n", cmd, param);
		}
		break;
	case 715:
		{
			fg_cust_data.aging1_update_soc = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, aging1_update_soc\n", cmd, param);
		}
		break;
	case 716:
		{
			fg_cust_data.aging_100_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, aging_100_en\n", cmd, param);
		}
		break;
	case 717:
		{
			fg_cust_data.additional_battery_table_en = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, additional_battery_table_en\n", cmd, param);
		}
		break;
	case 718:
		{
			fg_cust_data.d0_sel = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, d0_sel\n", cmd, param);
		}
		break;
	case 719:
		{
			fg_cust_data.zcv_car_gap_percentage = param;
			bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, zcv_car_gap_percentage\n", cmd, param);
		}
		break;

	default:
		bm_err("[FG_IT] exe_BAT_EC cmd %d, param %d, default\n", cmd, param);
		break;
	}

}

static ssize_t show_FG_daemon_disable(struct device *dev, struct device_attribute *attr, char *buf)
{
	bm_trace("[FG] show FG disable : %d\n", gDisableGM30);
	return sprintf(buf, "%d\n", gDisableGM30);
}

static ssize_t store_FG_daemon_disable(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	bm_err("[disable FG daemon]\n");
	disable_fg();
	if (gDisableGM30 == true)
		battery_main.BAT_CAPACITY = 50;
	battery_update(&battery_main);
	return size;
}
static DEVICE_ATTR(FG_daemon_disable, 0664, show_FG_daemon_disable, store_FG_daemon_disable);

static ssize_t show_FG_nafg_disable(struct device *dev, struct device_attribute *attr, char *buf)
{
	bm_trace("[FG] show nafg disable : %d\n", gDisableGM30);
	return sprintf(buf, "%d\n", gDisableGM30);
}

static ssize_t store_FG_nafg_disable(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val = 0;
	int ret;

	bm_err("[store_FG_nafg_disable]\n");

	if (buf != NULL && size != 0) {
		bm_err("[store_FG_nafg_disable] buf is %s\n", buf);
		ret = kstrtoul(buf, 10, &val);
		if (val < 0) {
			bm_err("[store_FG_nafg_disable] val is %d ??\n", (int)val);
			val = 0;
		}

		wakeup_fg_algo_cmd(FG_INTR_KERNEL_CMD, FG_KERNEL_CMD_DISABLE_NAFG, val);

		bm_err("[store_FG_nafg_disable] FG_nafg_disable = %d\n", (int)val);
	}


	return size;
}
static DEVICE_ATTR(disable_nafg, 0664, show_FG_nafg_disable, store_FG_nafg_disable);


static ssize_t show_FG_daemon_log_level(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	bm_err("[FG] show FG_daemon_log_level : %d\n", gFG_daemon_log_level);
	return sprintf(buf, "%d\n", gFG_daemon_log_level);
}

static ssize_t store_FG_daemon_log_level(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t size)
{
	unsigned long val = 0;
	int ret;

	bm_err("[FG_daemon_log_level]\n");

	if (buf != NULL && size != 0) {
		bm_err("[FG_daemon_log_level] buf is %s\n", buf);
		ret = kstrtoul(buf, 10, &val);
		if (val < 0) {
			bm_err("[FG_daemon_log_level] val is %d ??\n", (int)val);
			val = 0;
		}
		gFG_daemon_log_level = val;
		Enable_BATDRV_LOG = val;
		if (val >= 7) {
			gtimer_set_log_level(3);
			gauge_coulomb_set_log_level(3);
		} else {
			gtimer_set_log_level(0);
			gauge_coulomb_set_log_level(0);
		}
		bm_err("[FG_daemon_log_level] gFG_daemon_log_level=%d\n", gFG_daemon_log_level);
	}
	return size;
}
static DEVICE_ATTR(FG_daemon_log_level, 0664, show_FG_daemon_log_level, store_FG_daemon_log_level);

static ssize_t show_shutdown_cond_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	bm_trace("[FG] show_shutdown_cond_enable : %d\n", get_shutdown_cond_flag());
	return sprintf(buf, "%d\n", get_shutdown_cond_flag());
}

static ssize_t store_shutdown_cond_enable(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val = 0;
	int ret;

	bm_err("[store_shutdown_cond_enable]\n");
	if (buf != NULL && size != 0) {
		bm_err("[store_shutdown_cond_enable] buf is %s\n", buf);
		ret = kstrtoul(buf, 10, &val);
		if (val < 0) {
			bm_err("[store_shutdown_cond_enable] val is %d ??\n", (int)val);
			val = 0;
		}

		set_shutdown_cond_flag(val);

		bm_err("[store_shutdown_cond_enable] shutdown_cond_enabled=%d\n", get_shutdown_cond_flag());
	}

	return size;
}
static DEVICE_ATTR(shutdown_condition_enable, 0664, show_shutdown_cond_enable, store_shutdown_cond_enable);

static ssize_t show_reset_battery_cycle(struct device *dev, struct device_attribute *attr, char *buf)
{
	bm_trace("[FG] show_reset_battery_cycle : %d\n", is_reset_battery_cycle);
	return sprintf(buf, "%d\n", is_reset_battery_cycle);
}

static ssize_t store_reset_battery_cycle(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val = 0;
	int ret;

	bm_err("[store_reset_battery_cycle]\n");
	if (buf != NULL && size != 0) {
		bm_err("[store_reset_battery_cycle] buf is %s\n", buf);
		ret = kstrtoul(buf, 10, &val);
		if (val < 0) {
			bm_err("[store_reset_battery_cycle] val is %d ??\n", (int)val);
			val = 0;
		}
		if (val == 0)
			is_reset_battery_cycle = false;
		else {
			is_reset_battery_cycle = true;
			wakeup_fg_algo(FG_INTR_BAT_CYCLE);
		}
		bm_err("[store_reset_battery_cycle] store_reset_battery_cycle=%d\n", is_reset_battery_cycle);
	}

	return size;
}
static DEVICE_ATTR(reset_battery_cycle, 0664, show_reset_battery_cycle, store_reset_battery_cycle);

static ssize_t show_BAT_EC(struct device *dev, struct device_attribute *attr, char *buf)
{
	bm_err("[FG_IT] show_BAT_EC\n");

	return sprintf(buf, "%d:%d\n", BAT_EC_cmd, BAT_EC_param);
}

static ssize_t store_BAT_EC(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	char cmd_buf[4], param_buf[16];

	bm_err("[FG_IT] store_BAT_EC\n");
	if (size > 20) {
		bm_err("[FG_IT] store_BAT_EC error, %Zu\n", size);
		return -1;
	}

	if (buf != NULL && size != 0) {
		bm_err("[FG_IT] buf is %s and size is %Zu\n", buf, size);

		cmd_buf[0] = buf[0];
		cmd_buf[1] = buf[1];
		cmd_buf[2] = buf[2];
		cmd_buf[3] = '\0';
		strncpy(param_buf, buf + 4, size - 4);
		if ((size - 4) > 0)
			param_buf[size - 4 - 1] = '\0';

		bm_err("[FG_IT] cmd_buf %s, param_buf %s\n", cmd_buf, param_buf);

		ret = kstrtouint(cmd_buf, 10, &BAT_EC_cmd);
		ret = kstrtouint(param_buf, 10, &BAT_EC_param);

		bm_err("[FG_IT] store cmd %d, param %d\n", BAT_EC_cmd, BAT_EC_param);
	}

	exec_BAT_EC(BAT_EC_cmd, BAT_EC_param);

	return size;
}
static DEVICE_ATTR(BAT_EC, 0664, show_BAT_EC, store_BAT_EC);

static ssize_t show_FG_Battery_CurrentConsumption(struct device *dev, struct device_attribute *attr,
						  char *buf)
{
	int ret_value = 8888;

	ret_value = battery_get_bat_avg_current();
	bm_err("[EM] FG_Battery_CurrentConsumption : %d mA\n", ret_value);
	return sprintf(buf, "%d\n", ret_value);
}

static ssize_t store_FG_Battery_CurrentConsumption(struct device *dev,
						   struct device_attribute *attr, const char *buf,
						   size_t size)
{
	bm_err("[EM] Not Support Write Function\n");
	return size;
}

static DEVICE_ATTR(FG_Battery_CurrentConsumption, 0664, show_FG_Battery_CurrentConsumption,
		   store_FG_Battery_CurrentConsumption);

/* ///////////////////////////////////////////////////////////////////////////////////////// */
/* // Create File For EM : Power_On_Voltage */
/* ///////////////////////////////////////////////////////////////////////////////////////// */
static ssize_t show_Power_On_Voltage(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_value = 1;

	ret_value = 3450;
	bm_err("[EM] Power_On_Voltage : %d\n", ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_Power_On_Voltage(struct device *dev, struct device_attribute *attr,
				      const char *buf, size_t size)
{
	bm_err("[EM] Not Support Write Function\n");
	return size;
}

static DEVICE_ATTR(Power_On_Voltage, 0664, show_Power_On_Voltage, store_Power_On_Voltage);

/* ///////////////////////////////////////////////////////////////////////////////////////// */
/* // Create File For EM : Power_Off_Voltage */
/* ///////////////////////////////////////////////////////////////////////////////////////// */
static ssize_t show_Power_Off_Voltage(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret_value = 1;

	ret_value = 3400;
	bm_err("[EM] Power_Off_Voltage : %d\n", ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_Power_Off_Voltage(struct device *dev, struct device_attribute *attr,
				       const char *buf, size_t size)
{
	bm_err("[EM] Not Support Write Function\n");
	return size;
}

static DEVICE_ATTR(Power_Off_Voltage, 0664, show_Power_Off_Voltage, store_Power_Off_Voltage);


static int battery_callback(struct notifier_block *nb, unsigned long event, void *v)
{
	bm_err("battery_callback:%ld\n", event);
	switch (event) {
	case CHARGER_NOTIFY_EOC:
		{
/* CHARGING FULL */
			notify_fg_chr_full();
		}
		break;
	case CHARGER_NOTIFY_START_CHARGING:
		{
/* START CHARGING */
			battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
			battery_update(&battery_main);
		}
		break;
	case CHARGER_NOTIFY_STOP_CHARGING:
		{
/* STOP CHARGING */
			battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;
			battery_update(&battery_main);
		}
		break;
	case CHARGER_NOTIFY_ERROR:
		{
/* charging enter error state */
		battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;
		battery_update(&battery_main);
		}
		break;
	case CHARGER_NOTIFY_NORMAL:
		{
/* charging leave error state */
		battery_main.BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
		battery_update(&battery_main);

		}
		break;

	default:
		{
		}
		break;
	}

	return NOTIFY_DONE;
}

/********** adc_cdev*******************/
signed int battery_meter_meta_tool_cali_car_tune(int meta_current)
{
	int cali_car_tune = 0;
	int ret = 0;

	if (meta_current == 0)
		return fg_cust_data.car_tune_value * 10;

	ret = gauge_dev_enable_car_tune_value_calibration(gauge_dev, meta_current, &cali_car_tune);

	return cali_car_tune;		/* 1000 base */
}

static long adc_cali_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int *user_data_addr;
	int *naram_data_addr;
	int i = 0;
	int ret = 0;
	int adc_in_data[2] = { 1, 1 };
	int adc_out_data[2] = { 1, 1 };
	int temp_car_tune;

	mutex_lock(&bat_mutex);

	switch (cmd) {
	case TEST_ADC_CALI_PRINT:
		g_ADC_Cali = false;
		break;

	case SET_ADC_CALI_Slop:
		naram_data_addr = (int *)arg;
		ret = copy_from_user(adc_cali_slop, naram_data_addr, 36);
		g_ADC_Cali = false;	/* enable calibration after setting ADC_CALI_Cal */
		/* Protection */
		for (i = 0; i < 14; i++) {
			if ((*(adc_cali_slop + i) == 0) || (*(adc_cali_slop + i) == 1))
				*(adc_cali_slop + i) = 1000;
		}
		for (i = 0; i < 14; i++)
			bm_notice("adc_cali_slop[%d] = %d\n", i,
				    *(adc_cali_slop + i));
		bm_notice("**** unlocked_ioctl : SET_ADC_CALI_Slop Done!\n");
		break;

	case SET_ADC_CALI_Offset:
		naram_data_addr = (int *)arg;
		ret = copy_from_user(adc_cali_offset, naram_data_addr, 36);
		g_ADC_Cali = false;	/* enable calibration after setting ADC_CALI_Cal */
		for (i = 0; i < 14; i++)
			bm_notice("adc_cali_offset[%d] = %d\n", i,
				    *(adc_cali_offset + i));
		bm_notice("**** unlocked_ioctl : SET_ADC_CALI_Offset Done!\n");
		break;

	case SET_ADC_CALI_Cal:
		naram_data_addr = (int *)arg;
		ret = copy_from_user(adc_cali_cal, naram_data_addr, 4);
		g_ADC_Cali = true;
		if (adc_cali_cal[0] == 1)
			g_ADC_Cali = true;
		else
			g_ADC_Cali = false;

		for (i = 0; i < 1; i++)
			bm_notice("adc_cali_cal[%d] = %d\n", i,
				    *(adc_cali_cal + i));
		bm_notice("**** unlocked_ioctl : SET_ADC_CALI_Cal Done!\n");
		break;

	case ADC_CHANNEL_READ:
		/* g_ADC_Cali = KAL_FALSE; *//* 20100508 Infinity */
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);	/* 2*int = 2*4 */

		if (adc_in_data[0] == 0) {
			/* I_SENSE */
			adc_out_data[0] = battery_get_bat_voltage() * adc_in_data[1];
		} else if (adc_in_data[0] == 1) {
			/* BAT_SENSE */
			adc_out_data[0] = battery_get_bat_voltage() * adc_in_data[1];
		} else if (adc_in_data[0] == 3) {
			/* V_Charger */
			adc_out_data[0] = battery_meter_get_charger_voltage() * adc_in_data[1];
			/* adc_out_data[0] = adc_out_data[0] / 100; */
		} else if (adc_in_data[0] == 30) {
			/* V_Bat_temp magic number */
			adc_out_data[0] = battery_meter_get_battery_temperature() * adc_in_data[1];
		} else if (adc_in_data[0] == 66) {
			adc_out_data[0] = (battery_get_bat_current()) / 10;

		if (battery_get_bat_current_sign() == true)
			adc_out_data[0] = 0 - adc_out_data[0];	/* charging */

		} else {
			bm_notice("unknown channel(%d,%d)\n",
				    adc_in_data[0], adc_in_data[1]);
		}

		if (adc_out_data[0] < 0)
			adc_out_data[1] = 1;	/* failed */
		else
			adc_out_data[1] = 0;	/* success */

		if (adc_in_data[0] == 30)
			adc_out_data[1] = 0;	/* success */

		if (adc_in_data[0] == 66)
			adc_out_data[1] = 0;	/* success */

		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		bm_notice(
			    "**** unlocked_ioctl : Channel %d * %d times = %d\n",
			    adc_in_data[0], adc_in_data[1], adc_out_data[0]);
		break;

	case BAT_STATUS_READ:
		user_data_addr = (int *)arg;
		ret = copy_from_user(battery_in_data, user_data_addr, 4);
		/* [0] is_CAL */
		if (g_ADC_Cali)
			battery_out_data[0] = 1;
		else
			battery_out_data[0] = 0;
		ret = copy_to_user(user_data_addr, battery_out_data, 4);
		bm_notice("**** unlocked_ioctl : CAL:%d\n", battery_out_data[0]);
		break;
#if 0
	case Set_Charger_Current:	/* For Factory Mode */
		user_data_addr = (int *)arg;
		ret = copy_from_user(charging_level_data, user_data_addr, 4);
		g_ftm_battery_flag = KAL_TRUE;
		if (charging_level_data[0] == 0)
			charging_level_data[0] = CHARGE_CURRENT_70_00_MA;
		else if (charging_level_data[0] == 1)
			charging_level_data[0] = CHARGE_CURRENT_200_00_MA;
		else if (charging_level_data[0] == 2)
			charging_level_data[0] = CHARGE_CURRENT_400_00_MA;
		else if (charging_level_data[0] == 3)
			charging_level_data[0] = CHARGE_CURRENT_450_00_MA;
		else if (charging_level_data[0] == 4)
			charging_level_data[0] = CHARGE_CURRENT_550_00_MA;
		else if (charging_level_data[0] == 5)
			charging_level_data[0] = CHARGE_CURRENT_650_00_MA;
		else if (charging_level_data[0] == 6)
			charging_level_data[0] = CHARGE_CURRENT_700_00_MA;
		else if (charging_level_data[0] == 7)
			charging_level_data[0] = CHARGE_CURRENT_800_00_MA;
		else if (charging_level_data[0] == 8)
			charging_level_data[0] = CHARGE_CURRENT_900_00_MA;
		else if (charging_level_data[0] == 9)
			charging_level_data[0] = CHARGE_CURRENT_1000_00_MA;
		else if (charging_level_data[0] == 10)
			charging_level_data[0] = CHARGE_CURRENT_1100_00_MA;
		else if (charging_level_data[0] == 11)
			charging_level_data[0] = CHARGE_CURRENT_1200_00_MA;
		else if (charging_level_data[0] == 12)
			charging_level_data[0] = CHARGE_CURRENT_1300_00_MA;
		else if (charging_level_data[0] == 13)
			charging_level_data[0] = CHARGE_CURRENT_1400_00_MA;
		else if (charging_level_data[0] == 14)
			charging_level_data[0] = CHARGE_CURRENT_1500_00_MA;
		else if (charging_level_data[0] == 15)
			charging_level_data[0] = CHARGE_CURRENT_1600_00_MA;
		else
			charging_level_data[0] = CHARGE_CURRENT_450_00_MA;
		wake_up_bat();
		bm_notice("**** unlocked_ioctl : set_Charger_Current:%d\n",
			    charging_level_data[0]);
		break;
#endif
		/* add for meta tool------------------------------- */
	case Get_META_BAT_VOL:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		BMT_status.bat_vol = battery_get_bat_avg_voltage() / 10;
		adc_out_data[0] = BMT_status.bat_vol;
		ret = copy_to_user(user_data_addr, adc_out_data, 8);

		break;
	case Get_META_BAT_SOC:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		BMT_status.UI_SOC2 = battery_get_bat_uisoc();
		adc_out_data[0] = BMT_status.UI_SOC2;
		ret = copy_to_user(user_data_addr, adc_out_data, 8);

		break;

	case Get_META_BAT_CAR_TUNE_VALUE:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = fg_cust_data.car_tune_value;
		bm_notice("Get_BAT_CAR_TUNE_VALUE, res=%d\n", adc_out_data[0]);
		ret = copy_to_user(user_data_addr, adc_out_data, 8);

		break;

	case Set_META_BAT_CAR_TUNE_VALUE:

		/* meta tool input: adc_in_data[1] (mA)*/
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);

		/* Send cali_current to hal to calculate car_tune_value*/
		temp_car_tune = battery_meter_meta_tool_cali_car_tune(adc_in_data[1]);

		/* return car_tune_value to meta tool in adc_out_data[0] */
		fg_cust_data.car_tune_value = temp_car_tune;
		adc_out_data[0] = temp_car_tune;
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		bm_notice("Set_BAT_CAR_TUNE_VALUE[%d], res=%d, ret=%d\n",
			adc_in_data[1], adc_out_data[0], ret);

		break;
		/* add bing meta tool------------------------------- */

	default:
		g_ADC_Cali = false;
		break;
	}

	mutex_unlock(&bat_mutex);

	return 0;
}

static int adc_cali_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int adc_cali_release(struct inode *inode, struct file *file)
{
	return 0;
}

static unsigned long bat_node;

static int fb_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	bat_node = node;
	return 1;
}


static const struct file_operations adc_cali_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = adc_cali_ioctl,
	.open = adc_cali_open,
	.release = adc_cali_release,
};

/*************************************/
struct wake_lock battery_lock;
static int __init battery_probe(struct platform_device *dev)
{
	int ret_device_file = 0;
	int ret;
	struct class_device *class_dev = NULL;
	const char *fg_swocv_v = NULL;
	char fg_swocv_v_tmp[10];
	int fg_swocv_v_len = 0;
	const char *fg_swocv_i = NULL;
	char fg_swocv_i_tmp[10];
	int fg_swocv_i_len = 0;
	const char *shutdown_time = NULL;
	char shutdown_time_tmp[10];
	int shutdown_time_len = 0;
	const char *boot_voltage = NULL;
	char boot_voltage_tmp[10];
	int boot_voltage_len = 0;

/********* adc_cdev **********/
	ret = alloc_chrdev_region(&adc_cali_devno, 0, 1, ADC_CALI_DEVNAME);
	if (ret)
		bm_notice("Error: Can't Get Major number for adc_cali\n");
	adc_cali_cdev = cdev_alloc();
	adc_cali_cdev->owner = THIS_MODULE;
	adc_cali_cdev->ops = &adc_cali_fops;
	ret = cdev_add(adc_cali_cdev, adc_cali_devno, 1);
	if (ret)
		bm_notice("adc_cali Error: cdev_add\n");
	adc_cali_major = MAJOR(adc_cali_devno);
	adc_cali_class = class_create(THIS_MODULE, ADC_CALI_DEVNAME);
	class_dev = (struct class_device *)device_create(adc_cali_class,
							 NULL,
							 adc_cali_devno, NULL, ADC_CALI_DEVNAME);
/*****************************/

	gauge_dev = get_gauge_by_name("gauge");
	if (gauge_dev != NULL) {
		gauge_dev->fg_cust_data = &fg_cust_data;
		gauge_dev_initial(gauge_dev);
	} else
		bm_err("gauge_dev is NULL\n");

	gauge_coulomb_service_init();
	coulomb_plus.callback = fg_bat_int1_h_handler;
	gauge_coulomb_consumer_init(&coulomb_plus, &dev->dev, "car+1%");
	coulomb_minus.callback = fg_bat_int1_l_handler;
	gauge_coulomb_consumer_init(&coulomb_minus, &dev->dev, "car-1%");

	gauge_coulomb_consumer_init(&soc_plus, &dev->dev, "soc+1%");
	soc_plus.callback = fg_bat_int2_h_handler;
	gauge_coulomb_consumer_init(&soc_minus, &dev->dev, "soc-1%");
	soc_minus.callback = fg_bat_int2_l_handler;

#ifdef GAUGE_COULOMB_INTERRUPT_TEST
	gauge_coulomb_consumer_init(&coulomb_test1, &dev->dev, "test1");
	coulomb_test1.callback = coulomb_test1_handler;
	gauge_coulomb_consumer_init(&coulomb_test2, &dev->dev, "test2");
	gauge_coulomb_consumer_init(&coulomb_test3, &dev->dev, "test3");
	gauge_coulomb_consumer_init(&coulomb_test4, &dev->dev, "test4");
#endif

#ifdef GAUGE_TIMER_INTERRUPT_TEST
	gtimer_init(&g1, &dev->dev, "test1");
	gtimer_init(&g2, &dev->dev, "test2");
	gtimer_init(&g3, &dev->dev, "test3");
	gtimer_init(&g4, &dev->dev, "test4");
	gtimer_init(&g5, &dev->dev, "test5");
#endif

	fg_custom_init_from_header();

#ifdef BAT_PSEUDO_THREAD
	kthread_run(battery_update_routine, NULL, "battery_thread");
	fg_drv_thread_hrtimer_init();
#endif
	/* Power supply class */
#if !defined(CONFIG_MTK_DISABLE_GAUGE)
	battery_main.psy = power_supply_register(&(dev->dev), &battery_main.psd, NULL);
	if (IS_ERR(battery_main.psy)) {
		bm_err("[BAT_probe] power_supply_register Battery Fail !!\n");
		ret = PTR_ERR(battery_main.psy);
		return ret;
	}
	bm_err("[BAT_probe] power_supply_register Battery Success !!\n");
#endif
	ret = device_create_file(&(dev->dev), &dev_attr_Battery_Temperature);

	wake_lock_init(&battery_lock, WAKE_LOCK_SUSPEND, "battery wakelock");
	wake_lock(&battery_lock);

	if (gauge_get_hw_version() >= GAUGE_HW_V1000) {
		/* SW FG nafg */
		pmic_register_interrupt_callback(FG_RG_INT_EN_NAG_C_DLTV, fg_nafg_int_handler);

		/* init ZCV INT */
		pmic_register_interrupt_callback(FG_ZCV_NO, fg_zcv_int_handler);
	}

	if (gauge_get_hw_version() >= GAUGE_HW_V2000) {
		/* init  cycle int */
		pmic_register_interrupt_callback(FG_N_CHARGE_L_NO, fg_cycle_int_handler);

		/* init  IAVG int */
		pmic_register_interrupt_callback(FG_IAVG_H_NO, fg_iavg_int_ht_handler);
		pmic_register_interrupt_callback(FG_IAVG_L_NO, fg_iavg_int_lt_handler);

		/* init BAT PLUGOUT INT */
		pmic_register_interrupt_callback(FG_BAT_PLUGOUT_NO, fg_bat_plugout_int_handler);

		/* TEMPRATURE INT */
		pmic_register_interrupt_callback(FG_RG_INT_EN_BAT_TEMP_H, fg_bat_temp_int_h_handler);
		pmic_register_interrupt_callback(FG_RG_INT_EN_BAT_TEMP_L, fg_bat_temp_int_l_handler);

		/* VBAT2 L/H */
		pmic_register_interrupt_callback(FG_RG_INT_EN_BAT2_H, fg_vbat2_h_int_handler);
		pmic_register_interrupt_callback(FG_RG_INT_EN_BAT2_L, fg_vbat2_l_int_handler);
	}

	/* sysfs node */
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_disable_nafg);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_FG_daemon_log_level);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_FG_daemon_disable);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_BAT_EC);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_FG_Battery_CurrentConsumption);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_Power_On_Voltage);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_Power_Off_Voltage);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_shutdown_condition_enable);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_reset_battery_cycle);

	gtimer_init(&tracking_timer, &dev->dev, "tracking_timer");
	tracking_timer.callback = tracking_timer_callback;

	fg_bat_temp_int_init();
	mtk_power_misc_init(dev);

	pbat_consumer = charger_manager_get_by_name(&(dev->dev), "charger");
	if (pbat_consumer != NULL) {
		bat_nb.notifier_call = battery_callback;
		register_charger_manager_notifier(pbat_consumer, &bat_nb);
	}

	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0)
		fg_swocv_v = of_get_flat_dt_prop(bat_node, "atag,fg_swocv_v", &fg_swocv_v_len);
	if (fg_swocv_v == NULL) {
		bm_err(" fg_swocv_v == NULL len = %d\n", fg_swocv_v_len);
	} else {
		snprintf(fg_swocv_v_tmp, (fg_swocv_v_len + 1), "%s", fg_swocv_v);
		ret = kstrtoint(fg_swocv_v_tmp, 10, &ptim_lk_v);
		bm_err(" fg_swocv_v = %s len %d fg_swocv_v_tmp %s ptim_lk_v[%d]\n",
			fg_swocv_v, fg_swocv_v_len, fg_swocv_v_tmp, ptim_lk_v);
	}

	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0)
		fg_swocv_i = of_get_flat_dt_prop(bat_node, "atag,fg_swocv_i", &fg_swocv_i_len);
	if (fg_swocv_i == NULL) {
		bm_err(" fg_swocv_i == NULL len = %d\n", fg_swocv_i_len);
	} else {
		snprintf(fg_swocv_i_tmp, (fg_swocv_i_len + 1), "%s", fg_swocv_i);
		ret = kstrtoint(fg_swocv_i_tmp, 10, &ptim_lk_i);

		bm_err(" fg_swocv_i = %s len %d fg_swocv_i_tmp %s ptim_lk_i[%d]\n",
			fg_swocv_i, fg_swocv_i_len, fg_swocv_i_tmp, ptim_lk_i);
	}

	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0)
		shutdown_time = of_get_flat_dt_prop(bat_node, "atag,shutdown_time", &shutdown_time_len);
	if (shutdown_time == NULL) {
		bm_err(" shutdown_time == NULL len = %d\n", shutdown_time_len);
	} else {
		snprintf(shutdown_time_tmp, (shutdown_time_len + 1), "%s", shutdown_time);
		ret = kstrtoint(shutdown_time_tmp, 10, &pl_shutdown_time);

		bm_err(" shutdown_time = %s len %d shutdown_time_tmp %s pl_shutdown_time[%d]\n",
			shutdown_time, shutdown_time_len, shutdown_time_tmp, pl_shutdown_time);
	}

	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0)
		boot_voltage = of_get_flat_dt_prop(bat_node, "atag,boot_voltage", &boot_voltage_len);
	if (boot_voltage == NULL) {
		bm_err(" boot_voltage == NULL len = %d\n", boot_voltage_len);
	} else {
		snprintf(boot_voltage_tmp, (boot_voltage_len + 1), "%s", boot_voltage);
		ret = kstrtoint(boot_voltage_tmp, 10, &pl_bat_vol);

		bm_err(" boot_voltage = %s len %d boot_voltage_tmp %s pl_bat_vol[%d]\n",
			boot_voltage, boot_voltage_len, boot_voltage_tmp, pl_bat_vol);
	}

	gauge_dev_get_info(gauge_dev, GAUGE_2SEC_REBOOT, &pl_two_sec_reboot);
	gauge_dev_set_info(gauge_dev, GAUGE_2SEC_REBOOT, 0);

	battery_debug_init();

	wake_unlock(&battery_lock);

#if defined(CONFIG_MTK_DISABLE_GAUGE) || defined(CONFIG_POWER_EXT)
	bm_err("disable GM 3.0\n");
	disable_fg();
#endif
	is_init_done = true;

	get_monotonic_boottime(&sw_iavg_time);
	sw_iavg_car = gauge_get_coulomb();

	return 0;
}

struct platform_device battery_device = {
	.name = "battery",
	.id = -1,
};

#ifdef CONFIG_OF
static int battery_dts_probe(struct platform_device *dev)
{
	int ret = 0;

	bm_err("******** battery_dts_probe!! ********\n");
	battery_device.dev.of_node = dev->dev.of_node;
	ret = platform_device_register(&battery_device);
	if (ret) {
		bm_err("****[battery_dts_probe] Unable to register device (%d)\n", ret);
		return ret;
	}

	fg_custom_init_from_dts(dev);

	return 0;
}
#endif

#ifdef CONFIG_OF
static const struct of_device_id mtk_bat_of_match[] = {
	{.compatible = "mediatek,bat_gm30",},
	{},
};
MODULE_DEVICE_TABLE(of, mtk_bat_of_match);
#endif

static int battery_suspend(struct platform_device *dev, pm_message_t state)
{
	bm_err("******** battery_suspend!! ********\n");
	pmic_enable_interrupt(FG_IAVG_H_NO, 0, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 0, "GM30");
	return 0;
}

static int battery_resume(struct platform_device *dev)
{
	bm_err("******** battery_resume!! ********\n");
	pmic_enable_interrupt(FG_IAVG_H_NO, 1, "GM30");
	pmic_enable_interrupt(FG_IAVG_L_NO, 1, "GM30");
	fg_update_iavg();
	return 0;
}

static struct platform_driver battery_dts_driver_probe = {
	.probe = battery_dts_probe,
	.remove = NULL,
	.shutdown = NULL,
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		.name = "battery-dts",
#ifdef CONFIG_OF
		.of_match_table = mtk_bat_of_match,
#endif
	},
};

static struct platform_driver battery_driver_probe = {
	.probe = battery_probe,
	.remove = NULL,
	.shutdown = NULL,
	.suspend = battery_suspend,
	.resume = battery_resume,
	.driver = {
		   .name = "battery",
	},
};

static int __init battery_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input = nl_data_handler,
	};

	int ret;

	daemo_nl_sk = netlink_kernel_create(&init_net, NETLINK_FGD, &cfg);
	bm_err("netlink_kernel_create protol= %d\n", NETLINK_FGD);

	if (daemo_nl_sk == NULL) {
		bm_err("netlink_kernel_create error\n");
		return -1;
	}
	bm_err("netlink_kernel_create ok\n");

#ifdef CONFIG_OF
	/* register battery_device by DTS */
#else
	ret = platform_device_register(&battery_device);
#endif

	ret = platform_driver_register(&battery_driver_probe);
	ret = platform_driver_register(&battery_dts_driver_probe);

	bm_err("[battery_init] Initialization : DONE\n");

	return 0;
}

static void __exit battery_exit(void)
{

}
module_init(battery_init);
module_exit(battery_exit);

MODULE_AUTHOR("Weiching Lin");
MODULE_DESCRIPTION("Battery Device Driver");
MODULE_LICENSE("GPL");

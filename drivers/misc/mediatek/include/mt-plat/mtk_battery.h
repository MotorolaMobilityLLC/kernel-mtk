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

#ifndef _MTK_BATTERY_H
#define _MTK_BATTERY_H

#include <linux/types.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>
#include <mach/mtk_charger_init.h>

/* ============================================================ */
/* typedef */
/* ============================================================ */

/* battery notify charger_consumer */
enum {
	EVENT_BATTERY_PLUG_OUT,
};

struct FUELGAUGE_TEMPERATURE {
	signed int BatteryTemp;
	signed int TemperatureR;
};

struct FUELGAUGE_PROFILE_STRUCT {
	unsigned int mah;
	unsigned short voltage;
	unsigned short resistance; /* Ohm*/
	unsigned short percentage;
};


/* coulomb service */
struct gauge_consumer {
	char *name;
	struct device *dev;
	long start;
	long end;
	int variable;

	int (*callback)(struct gauge_consumer *);
	struct list_head list;
};

extern void gauge_coulomb_service_init(void);
extern void gauge_coulomb_consumer_init(struct gauge_consumer *coulomb, struct device *dev, char *name);
extern void gauge_coulomb_start(struct gauge_consumer *coulomb, int car);
extern void gauge_coulomb_stop(struct gauge_consumer *coulomb);
extern void gauge_coulomb_dump_list(void);
extern void gauge_coulomb_before_reset(void);
extern void gauge_coulomb_after_reset(void);
extern void gauge_coulomb_set_log_level(int x);
/* coulomb sub system end */

struct gtimer {
	char *name;
	struct device *dev;
	struct timespec endtime;
	int interval;

	int (*callback)(struct gtimer *);
	struct list_head list;
};

extern void gtimer_init(struct gtimer *timer, struct device *dev, char *name);
extern void gtimer_start(struct gtimer *timer, int sec);
extern void gtimer_stop(struct gtimer *timer);
extern void gtimer_dump_list(void);
extern void gtimer_set_log_level(int x);
/* time service end */



/* ============================================================ */
/* Prototype */
/* ============================================================ */
int fg_core_routine(void *x);

/* ============================================================ */
/* Extern */
/* ============================================================ */
extern int get_hw_ocv(void);
extern int get_sw_ocv(void);
extern void set_hw_ocv_unreliable(bool);
extern void battery_dump_info(struct seq_file *m);
extern void battery_dump_nag(void);

extern signed int battery_meter_get_battery_temperature(void);
extern unsigned int battery_meter_get_fg_time(void);
extern unsigned int battery_meter_set_fg_timer_interrupt(unsigned int sec);
extern void fg_charger_in_handler(void);

extern void do_chrdet_int_task(void);
extern int bat_get_debug_level(void);
extern int force_get_tbat(bool update);
/************** Old Interface *******************/
#ifdef CONFIG_MTK_SMART_BATTERY
	extern void wake_up_bat(void);
	extern unsigned long BAT_Get_Battery_Voltage(int polling_mode);
#else
	#define wake_up_bat()			do {} while (0)
	#define BAT_Get_Battery_Voltage(polling_mode)	({ 0; })
#endif
extern unsigned int bat_get_ui_percentage(void);
extern signed int battery_meter_get_battery_current(void);
extern signed int battery_meter_get_charger_voltage(void);
extern int get_soc(void);
extern int get_ui_soc(void);
/************** End Old Interface *******************/

/************** New Interface *******************/
extern bool battery_get_bat_current_sign(void);
extern signed int battery_get_bat_current(void);
extern signed int battery_get_bat_current_mA(void);
extern signed int battery_get_bat_avg_current(void);
extern signed int battery_get_bat_voltage(void);
extern signed int battery_get_bat_avg_voltage(void);
extern signed int battery_get_bat_soc(void);
extern signed int battery_get_bat_uisoc(void);
extern signed int battery_get_bat_temperature(void);
extern signed int battery_get_ibus(void);
extern signed int battery_get_vbus(void);
extern unsigned int battery_get_is_kpoc(void);
extern unsigned int bat_is_kpoc(void);
extern bool battery_is_battery_exist(void);
/************** End New Interface *******************/

extern int battery_get_charger_zcv(void);
extern int fg_get_battery_temperature_for_zcv(void);

/* pmic battery adc service */
extern int pmic_get_vbus(void);
extern int pmic_get_battery_voltage(void);
extern int pmic_get_v_bat_temp(void);
extern int pmic_is_bif_exist(void);
extern int pmic_get_bif_battery_voltage(int *vbat);
extern int pmic_get_bif_battery_temperature(int *tmp);
extern int pmic_get_ibus(void);
extern int pmic_get_charging_current(void);
extern bool pmic_is_battery_exist(void);

/* temp */
extern bool battery_meter_get_battery_current_sign(void);


extern bool is_fg_disable(void);

/* mtk_power_misc.c */
extern void mtk_power_misc_init(struct platform_device *pdev);
extern void notify_fg_shutdown(void);
extern int set_shutdown_cond(int shutdown_cond);
extern int get_shutdown_cond(void);
extern void set_shutdown_vbat_lt(int, int);
extern void set_shutdown_cond_flag(int);
extern int get_shutdown_cond_flag(void);
/* end mtk_power_misc.c */

extern void notify_fg_dlpt_sd(void);
extern void fg_charger_in_handler(void);
extern bool is_battery_init_done(void);

/* gauge interface */
extern bool gauge_get_current(int *bat_current);
extern int gauge_get_average_current(bool *valid);
extern int gauge_get_coulomb(void);
extern int gauge_set_coulomb_interrupt1_ht(int car);
extern int gauge_set_coulomb_interrupt1_lt(int car);
extern int gauge_get_ptim_current(int *ptim_current, bool *is_charging);

extern unsigned int battery_meter_enable_time_interrupt(unsigned int sec);

extern int register_battery_notifier(struct notifier_block *nb);
extern int unregister_battery_notifier(struct notifier_block *nb);

#endif /* End of _FUEL_GAUGE_GM_30_H */

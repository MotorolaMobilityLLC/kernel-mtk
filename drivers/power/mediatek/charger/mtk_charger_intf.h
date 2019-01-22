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
#include <mt-plat/charger_type.h>
#include <mt-plat/charger_class.h>
#include <mt-plat/mtk_charger.h>

#ifndef __MTK_CHARGER_INTF_H__
#define __MTK_CHARGER_INTF_H__

#include <linux/ktime.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/hrtimer.h>
#include <linux/wakelock.h>
#include <linux/spinlock.h>
#include <mt-plat/mtk_battery.h>

struct charger_manager;
#include "mtk_pe20_intf.h"
#include "mtk_pe30_intf.h"

/* charger_dev notify charger_manager */
enum {
	CHARGER_DEV_NOTIFY_VBUS_OVP,
	CHARGER_DEV_NOTIFY_BAT_OVP,
	CHARGER_DEV_NOTIFY_EOC,
	CHARGER_DEV_NOTIFY_RECHG,
	CHARGER_DEV_NOTIFY_SAFETY_TIMEOUT,
};

/*
*Software Jeita
*T0:-10
*T1:0
*T2:10
*T3:45
*T4:50
*/
typedef enum {
	TEMP_BELOW_T0 = 0,
	TEMP_T0_TO_T1,
	TEMP_T1_TO_T2,
	TEMP_T2_TO_T3,
	TEMP_T3_TO_T4,
	TEMP_ABOVE_T4
} sw_jeita_state_enum;

struct sw_jeita_data {
	int sm;
	int cv;
	bool charging;
	bool error_recovery_flag;
};

/* battery thermal protection */
typedef enum {
	BAT_TEMP_LOW = 0,
	BAT_TEMP_NORMAL,
	BAT_TEMP_HIGH
} bat_temp_state_enum;

struct battery_thermal_protection_data {
	int sm;
	bool enable_min_charge_temperature;
	int min_charge_temperature;
	int min_charge_temperature_plus_x_degree;
	int max_charge_temperature;
	int max_charge_temperature_minus_x_degree;
};

struct charger_custom_data {
	int usb_charger_current_suspend;
	int usb_charger_current_unconfigured;
	int usb_charger_current_configured;
	int usb_charger_current;
	int ac_charger_current;
	int ac_charger_input_current;
	int non_std_ac_charger_current;
	int charging_host_charger_current;
	int ta_ac_charger_current;

	int max_charger_voltage;
	int pe20_ichg_level_threshold;
	int ta_start_battery_soc;
	int ta_stop_battery_soc;
};

struct charger_data {
	int force_charging_current;
	int thermal_input_current_limit;
	int thermal_charging_current_limit;
	int input_current_limit;
	int charging_current_limit;
};

struct charger_manager {
	const char *algorithm_name;
	struct platform_device *pdev;
	void	*algorithm_data;
	int usb_state;
	bool usb_unlimited;

	struct charger_device *chg1_dev;
	struct notifier_block chg1_nb;
	struct charger_data chg1_data;

	struct charger_device *chg2_dev;
	struct notifier_block chg2_nb;
	struct charger_data chg2_data;

	CHARGER_TYPE chr_type;
	bool can_charging;

	int (*do_algorithm)(struct charger_manager *);
	int (*plug_in)(struct charger_manager *);
	int (*plug_out)(struct charger_manager *);
	int (*do_charging)(struct charger_manager *, bool en);
	int (*do_event)(struct notifier_block *nb, unsigned long event, void *v);
	int (*change_current_setting)(struct charger_manager *);

	/*notify charger user*/
	struct srcu_notifier_head evt_nh;
	/*receive from battery*/
	struct notifier_block psy_nb;

	/* common info */
	int battery_temperature;

	/* sw jeita */
	bool enable_sw_jeita;
	struct sw_jeita_data sw_jeita;

	/* battery warning */
	unsigned int notify_code;
	unsigned int notify_test_mode;

	/* battery thermal protection */
	struct battery_thermal_protection_data thermal;

	/* dtsi custom data */
	struct charger_custom_data data;

	bool enable_sw_safety_timer;
	bool enable_pe_plus;

	/* pe 2.0 */
	bool enable_pe_2;
	struct mtk_pe20 pe2;

	/* pe 3.0 */
	bool enable_pe_3;
	struct mtk_pe30 pe3;
	struct charger_device *dc_chg;

	/* thread related */
	struct hrtimer charger_kthread_timer;
	struct fgtimer charger_kthread_fgtimer;

	struct wake_lock charger_wakelock;
	struct mutex charger_lock;
	spinlock_t slock;
	unsigned int polling_interval;
	bool charger_thread_timeout;
	wait_queue_head_t  wait_que;
	bool charger_thread_polling;
};

/* charger related module interface */
extern int charger_manager_notifier(struct charger_manager *info, int event);
extern int mtk_switch_charging_init(struct charger_manager *);
extern void _wake_up_charger(struct charger_manager *);
extern int mtk_get_dynamic_cv(struct charger_manager *info, unsigned int *cv);

#endif /* __MTK_CHARGER_INTF_H__ */


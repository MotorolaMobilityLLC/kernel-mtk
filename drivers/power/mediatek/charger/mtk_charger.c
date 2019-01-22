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

/*
 *
 * Filename:
 * ---------
 *    mtk_charger.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines functions of Battery charging
 *
 * Author:
 * -------
 * Wy Chuang
 *
 */
#include <linux/init.h>		/* For init/exit macros */
#include <linux/module.h>	/* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/scatterlist.h>
#include <linux/suspend.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <mt-plat/charger_class.h>
#include <mt-plat/mtk_charger.h>
#include <mach/mtk_charger_init.h>
#include <mt-plat/charger_type.h>

#include <mt-plat/mtk_battery.h>
#include <musb_core.h>


struct charger_info *pinfo;
#define USE_FG_TIMER 1

int mtk_chr_is_charger_exist(unsigned char *exist)
{
	*exist = 1;
	return 0;
}

bool upmu_is_chr_det(void)
{
	return 0;
}
/*EXPORT_SYMBOL(upmu_is_chr_det);*/

void BATTERY_SetUSBState(int usb_state_value)
{
	if (IS_ENABLED(CONFIG_POWER_EXT)) {
		pr_err("[BATTERY_SetUSBState] in FPGA/EVB, no service\r\n");
	} else {
		if ((usb_state_value < USB_SUSPEND) || ((usb_state_value > USB_CONFIGURED))) {
			pr_err("[BATTERY] BAT_SetUSBState Fail! Restore to default value\r\n");
			usb_state_value = USB_UNCONFIGURED;
		} else {
			pr_err("[BATTERY] BAT_SetUSBState Success! Set %d\r\n",
					usb_state_value);
			pinfo->usb_state = usb_state_value;
		}
	}
}

unsigned int set_chr_input_current_limit(int current_limit)
{
	return 500;
}

int get_chr_temperature(int *min_temp, int *max_temp)
{
	*min_temp = 25;
	*max_temp = 30;

	return 0;
}

int set_chr_boost_current_limit(unsigned int current_limit)
{
	return 0;
}

int set_chr_enable_otg(unsigned int enable)
{
	return 0;
}

int mtk_chr_get_tchr(int *min_temp, int *max_temp)
{
	return 0;
}
/*=============== fix me==================*/
#ifdef EXP_DISABLE
int charger_psy_event(struct notifier_block *nb, unsigned long event, void *v)
{
	struct charger_info *info = container_of(nb, struct charger_info, psy_nb);
	struct power_supply *psy = v;
	CHARGER_TYPE chr_type;

	pr_err("charger psy_event %ld %s type:%d", event, psy->name, mt_get_charger_type());
	return NOTIFY_DONE;
	if ((strcmp(psy->name, "mt-chr") == 0) ||
		(strcmp(psy->name, "mt-usb") == 0)
		) {
		chr_type = mt_get_charger_type();
		if (chr_type == CHARGER_UNKNOWN) {
			if (info->chr_type != CHARGER_UNKNOWN) {
				if (info->plug_out != NULL)
					info->plug_out(info);
				info->chr_type = chr_type;
				info->charger_thread_polling = false;
				pr_err("charger_psy_event plug out\n");
			}
		} else {
			if (info->chr_type == CHARGER_UNKNOWN) {
				if (info->plug_in != NULL)
					info->plug_in(info);
				info->chr_type = chr_type;
				info->charger_thread_polling = true;
				pr_err("charger_psy_event plug in, tyupe:%d\n", chr_type);
			}
		}
	}

	pr_err("charger_psy_event %ld %s", event, psy->name);

	return NOTIFY_DONE;
}
#endif

static void wake_up_charger(struct charger_info *info)
{
	mutex_lock(&info->charger_lock);
	if (wake_lock_active(&info->charger_wakelock) == 0)
		wake_lock(&info->charger_wakelock);
	info->charger_thread_timeout = true;
	mutex_unlock(&info->charger_lock);
	wake_up(&info->wait_que);
}

int charger_dev_event(struct notifier_block *nb, unsigned long event, void *v)
{
	struct charger_info *info = container_of(nb, struct charger_info, charger_dev_nb);

	pr_err("charger_dev_event %ld", event);
	wake_up_charger(info);

	return NOTIFY_DONE;
}

void do_sw_jeita_state_machine(struct charger_info *info)
{
	struct sw_jeita_data *sw_jeita;
	int pre_sm;

	sw_jeita = &info->sw_jeita;
	pre_sm = sw_jeita->sm;
	sw_jeita->charging = true;
	/* JEITA battery temp Standard */

	if (info->battery_temperature >= TEMP_T4_THRESHOLD) {
		pr_err("[SW_JEITA] Battery Over high Temperature(%d) !!\n\r",
			    TEMP_T4_THRESHOLD);

		sw_jeita->sm = TEMP_ABOVE_T4;
		sw_jeita->charging = false;
	} else if (info->battery_temperature > TEMP_T3_THRESHOLD) {	/* control 45c to normal behavior */
		if ((sw_jeita->sm == TEMP_ABOVE_T4)
		    && (info->battery_temperature >= TEMP_T4_THRES_MINUS_X_DEGREE)) {
			pr_err("[SW_JEITA] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
				    TEMP_T4_THRES_MINUS_X_DEGREE, TEMP_T4_THRESHOLD);

			sw_jeita->charging = false;
		} else {
			pr_err("[SW_JEITA] Battery Temperature between %d and %d !!\n\r",
				    TEMP_T3_THRESHOLD, TEMP_T4_THRESHOLD);

			sw_jeita->sm = TEMP_T3_TO_T4;
		}
	} else if (info->battery_temperature >= TEMP_T2_THRESHOLD) {
		if (((sw_jeita->sm == TEMP_T3_TO_T4)
		     && (info->battery_temperature >= TEMP_T3_THRES_MINUS_X_DEGREE))
		    || ((sw_jeita->sm == TEMP_T1_TO_T2)
			&& (info->battery_temperature <= TEMP_T2_THRES_PLUS_X_DEGREE))) {
			pr_err("[SW_JEITA] Battery Temperature not recovery to normal temperature charging mode yet!!\n\r");
		} else {
			pr_err("[SW_JEITA] Battery Normal Temperature between %d and %d !!\n\r",
				    TEMP_T2_THRESHOLD, TEMP_T3_THRESHOLD);
			sw_jeita->sm = TEMP_T2_TO_T3;
		}
	} else if (info->battery_temperature >= TEMP_T1_THRESHOLD) {
		if ((sw_jeita->sm == TEMP_T0_TO_T1 || sw_jeita->sm == TEMP_BELOW_T0)
		    && (info->battery_temperature <= TEMP_T1_THRES_PLUS_X_DEGREE)) {
			if (sw_jeita->sm == TEMP_T0_TO_T1) {
				pr_err("[SW_JEITA] Battery Temperature between %d and %d !!\n\r",
					    TEMP_T1_THRES_PLUS_X_DEGREE, TEMP_T2_THRESHOLD);
			}
			if (sw_jeita->sm == TEMP_BELOW_T0) {
				pr_err("[SW_JEITA] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
					    TEMP_T1_THRESHOLD, TEMP_T1_THRES_PLUS_X_DEGREE);
				sw_jeita->charging = false;
			}
		} else {
			pr_err("[SW_JEITA] Battery Temperature between %d and %d !!\n\r",
				    TEMP_T1_THRESHOLD, TEMP_T2_THRESHOLD);

			sw_jeita->sm = TEMP_T1_TO_T2;
		}
	} else if (info->battery_temperature >= TEMP_T0_THRESHOLD) {
		if ((sw_jeita->sm == TEMP_BELOW_T0)
		    && (info->battery_temperature <= TEMP_T0_THRES_PLUS_X_DEGREE)) {
			pr_err("[SW_JEITA] Battery Temperature between %d and %d,not allow charging yet!!\n\r",
				    TEMP_T0_THRESHOLD, TEMP_T0_THRES_PLUS_X_DEGREE);

			sw_jeita->charging = false;
		} else {
			pr_err("[SW_JEITA] Battery Temperature between %d and %d !!\n\r",
				    TEMP_T0_THRESHOLD, TEMP_T1_THRESHOLD);

			sw_jeita->sm = TEMP_T0_TO_T1;
		}
	} else {
		pr_err("[SW_JEITA] Battery below low Temperature(%d) !!\n\r",
			    TEMP_T0_THRESHOLD);
		sw_jeita->sm = TEMP_BELOW_T0;
		sw_jeita->charging = false;
	}

	/* set CV after temperature changed */
	/* In normal range, we adjust CV dynamically */
	if (sw_jeita->sm != TEMP_T2_TO_T3) {
		if (sw_jeita->sm == TEMP_ABOVE_T4)
			sw_jeita->cv = JEITA_TEMP_ABOVE_T4_CV_VOLTAGE;
		else if (sw_jeita->sm == TEMP_T3_TO_T4)
			sw_jeita->cv = JEITA_TEMP_T3_TO_T4_CV_VOLTAGE;
		else if (sw_jeita->sm == TEMP_T2_TO_T3)
			sw_jeita->cv = 0;
		else if (sw_jeita->sm == TEMP_T1_TO_T2)
			sw_jeita->cv = JEITA_TEMP_T1_TO_T2_CV_VOLTAGE;
		else if (sw_jeita->sm == TEMP_T0_TO_T1)
			sw_jeita->cv = JEITA_TEMP_T0_TO_T1_CV_VOLTAGE;
		else if (sw_jeita->sm == TEMP_BELOW_T0)
			sw_jeita->cv = JEITA_TEMP_BELOW_T0_CV_VOLTAGE;
		else
			sw_jeita->cv = BATTERY_CV;
	}

	pr_err("[SW_JEITA]preState:%d newState:%d tmp:%d cv:%d\n\r",
		pre_sm, sw_jeita->sm, info->battery_temperature, sw_jeita->cv);
}

void mtk_charger_int_handler(void)
{
	struct charger_info *info = pinfo;

	pr_err("mtk_charger_int_handler ,type:%d\n", mt_get_charger_type());

	if (pinfo == NULL)
		return;

	wake_up_charger(info);
}

static int mtk_charger_plug_in(struct charger_info *info, CHARGER_TYPE chr_type)
{
	info->chr_type = chr_type;
	info->charger_thread_polling = true;


	info->can_charging = true;
	info->thermal_charging_current_limit = -1;
	info->thermal_input_current_limit = -1;

	pr_err("mtk_is_charger_on plug in, tyupe:%d\n", chr_type);
	if (info->plug_in != NULL)
		info->plug_in(info);

	charger_dev_plug_in(info->primary_chg);
	return 0;
}

static int mtk_charger_plug_out(struct charger_info *info)
{
	pr_err("mtk_charger_plug_out\n");
	info->chr_type = CHARGER_UNKNOWN;
	info->charger_thread_polling = false;

	if (info->plug_out != NULL)
		info->plug_out(info);

	charger_dev_plug_out(info->primary_chg);
	return 0;
}

static bool mtk_is_charger_on(struct charger_info *info)
{
	CHARGER_TYPE chr_type;

	chr_type = mt_get_charger_type();
	pr_err("mtk_is_charger_on %d %d", chr_type, info->chr_type);
	if (chr_type == CHARGER_UNKNOWN) {
		if (info->chr_type != CHARGER_UNKNOWN)
			mtk_charger_plug_out(info);
	} else {
		if (info->chr_type == CHARGER_UNKNOWN)
			mtk_charger_plug_in(info, chr_type);
	}

	if (chr_type == CHARGER_UNKNOWN)
		return false;

	return true;
}

static void charger_update_data(struct charger_info *info)
{
	info->battery_temperature = battery_meter_get_battery_temperature();
}

static void charger_check_status(struct charger_info *info)
{
	bool charging = true;
	int temperature;
	struct battery_thermal_protection_data *thermal;

	temperature = info->battery_temperature;
	thermal = &info->thermal;

	if (info->enable_sw_jeita == true) {
		do_sw_jeita_state_machine(info);
		if (info->sw_jeita.charging == false) {
			charging = false;
			goto stop_charging;
		}
	}

	if (thermal->enable_min_charge_temperature) {
		if (temperature < thermal->min_charge_temperature) {
			pr_err("[BATTERY] Battery Under Temperature or NTC fail %d %d!!\n\r", temperature,
				thermal->min_charge_temperature);
			thermal->sm = BAT_TEMP_LOW;
			charging = false;
			goto stop_charging;
		} else if (thermal->sm == BAT_TEMP_LOW) {
			if (temperature >= thermal->min_charge_temperature_plus_x_degree) {
				pr_err("[BATTERY] Battery Temperature raise from %d to %d(%d), allow charging!!\n\r",
						thermal->min_charge_temperature,
						temperature,
						thermal->min_charge_temperature_plus_x_degree);
				thermal->sm = BAT_TEMP_NORMAL;
			} else {
				charging = false;
				goto stop_charging;
			}
		}
	}

	if (temperature >= thermal->max_charge_temperature) {
		pr_err("[BATTERY] Battery over Temperature or NTC fail %d %d!!\n\r", temperature,
			thermal->min_charge_temperature);
		thermal->sm = BAT_TEMP_HIGH;
		charging = false;
		goto stop_charging;
	} else if (thermal->sm == BAT_TEMP_HIGH) {
		if (temperature < thermal->max_charge_temperature_minus_x_degree) {
			pr_err("[BATTERY] Battery Temperature raise from %d to %d(%d), allow charging!!\n\r",
					thermal->max_charge_temperature,
					temperature,
					thermal->max_charge_temperature_minus_x_degree);
			thermal->sm = BAT_TEMP_NORMAL;
		} else {
			charging = false;
			goto stop_charging;
		}
	}

stop_charging:

	pr_err("temperature check %d %d", temperature, thermal->sm);
	if (charging != info->can_charging) {
		if (info->do_charging)
			info->do_charging(info, charging);
	}

	info->can_charging = charging;

}

static void charger_select_charging_current_limit(struct charger_info *info)
{

	if (info->usb_unlimited && (info->chr_type == STANDARD_HOST || info->chr_type == CHARGING_HOST)) {
		info->input_current_limit = info->data.ac_charger_input_current;
		info->charging_current_limit = info->data.ac_charger_current;
		goto done;
	}

	if (info->chr_type == STANDARD_HOST) {
		if (IS_ENABLED(CONFIG_USBIF_COMPLIANCE)) {
			if (info->usb_state == USB_SUSPEND)
				info->input_current_limit = info->data.usb_charger_current_suspend;
			else if (info->usb_state == USB_UNCONFIGURED)
				info->input_current_limit = info->data.usb_charger_current_unconfigured;
			else if (info->usb_state == USB_CONFIGURED)
				info->input_current_limit = info->data.usb_charger_current_configured;
			else
				info->input_current_limit = info->data.usb_charger_current_unconfigured;

				info->charging_current_limit = info->input_current_limit;
		} else {
			info->input_current_limit = info->data.usb_charger_current;
			info->charging_current_limit = info->data.usb_charger_current;	/* it can be larger */
		}
	} else if (info->chr_type == NONSTANDARD_CHARGER) {
		info->input_current_limit = info->data.non_std_ac_charger_current;
		info->charging_current_limit = info->data.non_std_ac_charger_current;
	} else if (info->chr_type == STANDARD_CHARGER) {
		info->input_current_limit = info->data.ac_charger_input_current;
		info->charging_current_limit = info->data.ac_charger_current;
	} else if (info->chr_type == CHARGING_HOST) {
		info->input_current_limit = info->data.charging_host_charger_current;
		info->charging_current_limit = info->data.charging_host_charger_current;
	}

	if (info->enable_sw_jeita) {
		if (IS_ENABLED(CONFIG_USBIF_COMPLIANCE) && info->chr_type == STANDARD_HOST)
			pr_debug("USBIF & STAND_HOST skip current check\n");
		else {
			if (info->sw_jeita.sm == TEMP_T0_TO_T1) {
				info->input_current_limit = 500000;
				info->charging_current_limit = 350000;
			}
		}
	}

	if (info->thermal_charging_current_limit != -1)
		if (info->thermal_charging_current_limit < info->charging_current_limit)
			info->charging_current_limit = info->thermal_charging_current_limit;

	if (info->thermal_input_current_limit != -1)
		if (info->thermal_input_current_limit < info->input_current_limit)
			info->input_current_limit = info->thermal_input_current_limit;
done:

	pr_err("thermal:%d %d setting:%d %d type:%d usb_unlimited:%d usbif:%d usbsm:%d\n",
		info->thermal_input_current_limit, info->thermal_charging_current_limit,
		info->input_current_limit, info->charging_current_limit,
		info->chr_type, info->usb_unlimited,
		IS_ENABLED(CONFIG_USBIF_COMPLIANCE), info->usb_state);

}


enum hrtimer_restart charger_kthread_hrtimer_func(struct hrtimer *timer)
{
	struct charger_info *info = container_of(timer, struct charger_info, charger_kthread_timer);

	wake_up_charger(info);
	return HRTIMER_NORESTART;
}

int charger_kthread_fgtimer_func(struct fgtimer *data)
{
	struct charger_info *info = container_of(data, struct charger_info, charger_kthread_fgtimer);

	wake_up_charger(info);
	return 0;
}

static void mtk_charger_init_timer(struct charger_info *info)
{
	if (IS_ENABLED(USE_FG_TIMER)) {
		fgtimer_init(&info->charger_kthread_fgtimer, &info->pdev->dev, "charger_thread");
		fgtimer_start(&info->charger_kthread_fgtimer, info->polling_interval);
		info->charger_kthread_fgtimer.callback = charger_kthread_fgtimer_func;
	} else {
		ktime_t ktime = ktime_set(info->polling_interval, 0);

		hrtimer_init(&info->charger_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		info->charger_kthread_timer.function = charger_kthread_hrtimer_func;
		hrtimer_start(&info->charger_kthread_timer, ktime, HRTIMER_MODE_REL);
	}
}

static void mtk_charger_start_timer(struct charger_info *info)
{
	if (IS_ENABLED(USE_FG_TIMER)) {
		pr_err("fg start timer");
		fgtimer_start(&info->charger_kthread_fgtimer, info->polling_interval);
	} else {
		ktime_t ktime = ktime_set(info->polling_interval, 0);

		pr_err("hrtimer start timer");
		hrtimer_start(&info->charger_kthread_timer, ktime, HRTIMER_MODE_REL);
	}
}

void mtk_charger_stop_timer(struct charger_info *info)
{
	if (IS_ENABLED(USE_FG_TIMER))
		fgtimer_stop(&info->charger_kthread_fgtimer);
}

static int charger_routine_thread(void *arg)
{
	struct charger_info *info = arg;

	while (1) {
		pr_err("charger_routine_thread [%s]\n", info->primary_chg->props.alias_name);
		wait_event(info->wait_que, (info->charger_thread_timeout == true));

		mutex_lock(&info->charger_lock);
		info->charger_thread_timeout = false;
		pr_err("charger_routine_thread wake up %d\n", mtk_is_charger_on(info));

		charger_update_data(info);
		charger_check_status(info);

		if (mtk_is_charger_on(info) == true) {
			charger_select_charging_current_limit(info);
			if (info->do_algorithm)
				info->do_algorithm(info);
		}

		if (info->charger_thread_polling == true)
			mtk_charger_start_timer(info);
		wake_unlock(&info->charger_wakelock);
		mutex_unlock(&info->charger_lock);
	}

	return 0;
}

static int mtk_charger_parse_dt(struct charger_info *info, struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 val;

	pr_err("%s: starts\n", __func__);

	if (!np) {
		pr_err("%s: no device node\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "algorithm_name",
		&info->algorithm_name) < 0) {
		pr_err("%s: no algorithm_name name\n", __func__);
		info->algorithm_name = "SwitchCharging";
	}

	if (strcmp(info->algorithm_name, "SwitchCharging") == 0) {
		pr_err("found SwitchCharging\n");
		mtk_switch_charging_init(info);
	}

	info->enable_sw_safety_timer = of_property_read_bool(np, "enable_sw_safety_timer");
	info->enable_sw_jeita = of_property_read_bool(np, "enable_sw_jeita");
	info->enable_pe_plus = of_property_read_bool(np, "enable_pe_plus");
	info->enable_pe_2 = of_property_read_bool(np, "enable_pe_2");
	info->enable_pe_3 = of_property_read_bool(np, "enable_pe_3");

	/* battery thermal */
	info->thermal.enable_min_charge_temperature = of_property_read_bool(np,
		"enable_min_charge_temperature");

	if (of_property_read_u32(np, "min_charge_temperature", &val) >= 0) {
		info->thermal.min_charge_temperature = val;
	} else {
		pr_err(
			"use default MIN_CHARGE_TEMPERATURE:%d\n", MIN_CHARGE_TEMPERATURE);
		info->thermal.min_charge_temperature = MIN_CHARGE_TEMPERATURE;
	}

	if (of_property_read_u32(np, "min_charge_temperature_plus_x_degree", &val) >= 0) {
		info->thermal.min_charge_temperature_plus_x_degree = val;
	} else {
		pr_err(
			"use default MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE:%d\n",
			MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE);
		info->thermal.min_charge_temperature_plus_x_degree = MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "max_charge_temperature", &val) >= 0) {
		info->thermal.max_charge_temperature = val;
	} else {
		pr_err(
			"use default MAX_CHARGE_TEMPERATURE:%d\n", MAX_CHARGE_TEMPERATURE);
		info->thermal.max_charge_temperature = MAX_CHARGE_TEMPERATURE;
	}

	if (of_property_read_u32(np, "max_charge_temperature_minus_x_degree", &val) >= 0) {
		info->thermal.max_charge_temperature_minus_x_degree = val;
	} else {
		pr_err(
			"use default MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE:%d\n",
			MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE);
		info->thermal.max_charge_temperature_minus_x_degree = MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE;
	}

	/* charging current */
	if (of_property_read_u32(np, "max_charge_temperature_minus_x_degree", &val) >= 0) {
		info->thermal.max_charge_temperature_minus_x_degree = val;
	} else {
		pr_err(
			"use default MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE:%d\n",
			MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE);
		info->thermal.max_charge_temperature_minus_x_degree = MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE;
	}

	/*need to add dtsi*/
	info->data.usb_charger_current_suspend = USB_CHARGER_CURRENT_SUSPEND;
	info->data.usb_charger_current_unconfigured = USB_CHARGER_CURRENT_UNCONFIGURED;
	info->data.usb_charger_current_configured = USB_CHARGER_CURRENT_CONFIGURED;
	info->data.usb_charger_current = USB_CHARGER_CURRENT;
	info->data.ac_charger_current = AC_CHARGER_CURRENT;
	info->data.ac_charger_input_current = AC_CHARGER_INPUT_CURRENT;
	info->data.non_std_ac_charger_current = NON_STD_AC_CHARGER_CURRENT;
	info->data.charging_host_charger_current = CHARGING_HOST_CHARGER_CURRENT;

	pr_err("algorithm name:%s\n",
		info->algorithm_name);

	return 0;
}

static int mtk_charger_probe(struct platform_device *pdev)
{
	struct charger_info *info = NULL;

	pr_err("%s: starts\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(struct charger_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	pinfo = info;

	platform_set_drvdata(pdev, info);
	info->pdev = pdev;
	mtk_charger_parse_dt(info, &pdev->dev);

	mutex_init(&info->charger_lock);
	wake_lock_init(&info->charger_wakelock, WAKE_LOCK_SUSPEND, "charger suspend wakelock");

	/* init thread */
	init_waitqueue_head(&info->wait_que);
	info->polling_interval = 20;

	mtk_charger_init_timer(info);

	kthread_run(charger_routine_thread, info, "charger_thread");

	if (info->primary_chg != NULL) {
		info->charger_dev_nb.notifier_call = charger_dev_event;
		register_charger_device_notifier(info->primary_chg, &info->charger_dev_nb);
	}

#ifdef EXP_DISABLE
	info->nb.notifier_call = charger_psy_event;
	power_supply_reg_notifier(&info->psy_nb);
#endif
	return 0;
}

static int mtk_charger_remove(struct platform_device *dev)
{
	return 0;
}

static void mtk_charger_shutdown(struct platform_device *dev)
{

}

static const struct of_device_id mtk_charger_of_match[] = {
	{.compatible = "mediatek,charger",},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_charger_of_match);

struct platform_device charger_device = {
	.name = "charger",
	.id = -1,
};

static struct platform_driver charger_driver = {
	.probe = mtk_charger_probe,
	.remove = mtk_charger_remove,
	.shutdown = mtk_charger_shutdown,
	.driver = {
		   .name = "charger",
		   .of_match_table = mtk_charger_of_match,
		   },
};

static int __init mtk_charger_init(void)
{
	return platform_driver_register(&charger_driver);
}
late_initcall(mtk_charger_init);

static void __exit mtk_charger_exit(void)
{
	platform_driver_unregister(&charger_driver);
}
module_exit(mtk_charger_exit);


MODULE_AUTHOR("wy.chuang <wy.chuang@mediatek.com>");
MODULE_DESCRIPTION("MTK Charger Driver");
MODULE_LICENSE("GPL");

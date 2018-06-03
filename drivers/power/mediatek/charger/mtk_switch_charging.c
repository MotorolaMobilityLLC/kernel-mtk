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
 *    mtk_switch_charging.c
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

#include <musb_core.h>
#include "mtk_switch_charging.h"
#include <mt-plat/charger_class.h>
#include <mt-plat/mtk_charger.h>
#include "mtk_pe20_intf.h"

static void swchg_select_charging_current_limit(struct charger_manager *info)
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
		mtk_pe20_set_charging_current(info, &info->charging_current_limit, &info->input_current_limit);
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

	charger_dev_set_input_current(info->primary_chg, info->input_current_limit);
	charger_dev_set_charging_current(info->primary_chg, info->charging_current_limit);

	/* If AICR < 300mA, stop PE+/PE+20 */
	if (info->input_current_limit < 300000) {
		if (mtk_pe20_get_is_enable(info)) {
			mtk_pe20_set_is_enable(info, false);
			if (mtk_pe20_get_is_connect(info))
				mtk_pe20_reset_ta_vchr(info);
		}
	}
	if (info->input_current_limit > 0 && info->charging_current_limit > 0)
		charger_dev_enable(info->primary_chg);
}

static void swchg_select_cv(struct charger_manager *info)
{
	int constant_voltage;

	if (info->enable_sw_jeita)
		if (info->sw_jeita.cv != 0) {
			charger_dev_set_constant_voltage(info->primary_chg, info->sw_jeita.cv);
			return;
		}

	/* dynamic cv*/
	constant_voltage = BATTERY_CV;

	charger_dev_set_constant_voltage(info->primary_chg, constant_voltage);
}

static void swchg_turn_on_charging(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;
	bool charging_enable = true;

	if (swchgalg->state == CHR_ERROR) {
		charging_enable = false;
		pr_err("[charger]Charger Error, turn OFF charging !\n");
	} else if ((get_boot_mode() == META_BOOT) || ((get_boot_mode() == ADVMETA_BOOT))) {
		charging_enable = false;
		pr_err("[charger]In meta or advanced meta mode, disable charging.\n");
	} else {
		mtk_pe20_start_algorithm(info);

		swchg_select_charging_current_limit(info);
		if (info->input_current_limit == 0 || info->charging_current_limit == 0) {
			charging_enable = false;
			pr_err("[charger]charging current is set 0mA, turn off charging !\r\n");
		} else {
			swchg_select_cv(info);
		}
	}

	if (charging_enable == true)
		charger_dev_enable(info->primary_chg);
	else
		charger_dev_disable(info->primary_chg);

}

static int mtk_switch_charging_plug_in(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;

	swchgalg->state = CHR_CC;
	swchgalg->disable_charging = false;
	charger_manager_notifier(info, CHARGER_NOTIFY_START_CHARGING);

	return 0;
}

static int mtk_switch_charging_plug_out(struct charger_manager *info)
{
	mtk_pe20_set_is_cable_out_occur(info, true);
	charger_manager_notifier(info, CHARGER_NOTIFY_STOP_CHARGING);
	return 0;
}

static int mtk_switch_charging_do_charging(struct charger_manager *info, bool en)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;

	pr_err("mtk_switch_charging_do_charging en:%d %s\n", en, info->algorithm_name);
	if (en) {
		swchgalg->disable_charging = false;
		swchgalg->state = CHR_CC;
	} else {
		swchgalg->disable_charging = true;
		swchgalg->state = CHR_ERROR;
	}
	return 0;
}

static int mtk_switch_chr_cc(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;

	/* check bif */
	if (IS_ENABLED(CONFIG_MTK_BIF_SUPPORT)) {
		if (pmic_is_bif_exist() != 1)
			swchgalg->state = CHR_ERROR;
	}

	/* turn on LED */

	swchg_turn_on_charging(info);

	if (charger_dev_get_charging_status(info->primary_chg) == 1) {
		swchgalg->state = CHR_BATFULL;
		pr_err("battery full!\n");
	}

	/* If it is not disabled by throttling,
	 * enable PE+/PE+20, if it is disabled
	 */
	if (info->thermal_input_current_limit != -1 &&
		info->thermal_input_current_limit < 300)
		return 0;

	if (!mtk_pe20_get_is_enable(info)) {
		mtk_pe20_set_is_enable(info, true);
		mtk_pe20_set_to_check_chr_type(info, true);
	}

	return 0;
}

int mtk_switch_chr_err(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;

	if (info->enable_sw_jeita) {
		if ((info->sw_jeita.sm == TEMP_BELOW_T0) || (info->sw_jeita.sm == TEMP_ABOVE_T4))
			info->sw_jeita.error_recovery_flag = false;

		if ((info->sw_jeita.error_recovery_flag == false) &&
			(info->sw_jeita.sm != TEMP_BELOW_T0) && (info->sw_jeita.sm == TEMP_ABOVE_T4)) {
			info->sw_jeita.error_recovery_flag = true;
			swchgalg->state = CHR_CC;
		}
	}
	charger_dev_disable(info->primary_chg);

	if (mtk_pe20_get_is_enable(info)) {
		mtk_pe20_set_is_enable(info, false);
		if (mtk_pe20_get_is_connect(info))
			mtk_pe20_reset_ta_vchr(info);
	}

	return 0;
}

int mtk_switch_chr_full(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;

	/* turn off LED */

	/*
	 * If CV is set to lower value by JEITA,
	 * Reset CV to normal value if temperture is in normal zone
	 */
	swchg_select_cv(info);

	if (charger_dev_get_charging_status(info->primary_chg) == 1) {
		swchgalg->state = CHR_CC;

		mtk_pe20_set_to_check_chr_type(info, true);
		pr_err("battery recharging!\n");
	}

	return 0;
}



static int mtk_switch_charging_run(struct charger_manager *info)
{
	struct switch_charging_alg_data *swchgalg = info->algorithm_data;
	int ret = 10;

	pr_err("mtk_switch_charging_run [%d]\n", swchgalg->state);

	mtk_pe20_check_charger(info);

	switch (swchgalg->state) {
	case CHR_CC:
		ret = mtk_switch_chr_cc(info);
		break;

	case CHR_BATFULL:
		ret = mtk_switch_chr_full(info);
		break;

	case CHR_ERROR:
		ret = mtk_switch_chr_err(info);
		break;

	case CHR_PEP30:
		break;
	}

	charger_dev_dump_registers(info->primary_chg);
	return 0;
}

int charger_dev_event(struct notifier_block *nb, unsigned long event, void *v)
{
	struct charger_manager *info = container_of(nb, struct charger_manager, charger_dev_nb);
	/*struct charger_device *charger_dev = v;*/

	pr_err("charger_dev_event %ld", event);

	if (event == CHARGER_DEV_NOTIFY_EOC) {
		charger_manager_notifier(info, CHARGER_NOTIFY_EOC);
		wake_up_charger(info);
	}
	return NOTIFY_DONE;
}


int mtk_switch_charging_init(struct charger_manager *info)
{
	struct switch_charging_alg_data *swch_alg;

	swch_alg = devm_kzalloc(&info->pdev->dev, sizeof(struct switch_charging_alg_data), GFP_KERNEL);
	if (!swch_alg)
		return -ENOMEM;

	info->primary_chg = get_charger_by_name("PrimarySWCHG");
	if (info->primary_chg)
		pr_err("Found primary charger [%s]\n", info->primary_chg->props.alias_name);
	else
		pr_err("*** Error : can't find primary charger [%s]***\n", "PrimarySWCHG");

	info->algorithm_data = swch_alg;
	info->do_algorithm = mtk_switch_charging_run;
	info->plug_in = mtk_switch_charging_plug_in;
	info->plug_out = mtk_switch_charging_plug_out;
	info->do_charging = mtk_switch_charging_do_charging;
	info->do_event = charger_dev_event;

	return 0;
}





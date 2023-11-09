// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*
 *
 * Filename:
 * ---------
 *    mtk_wlc.c
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
#include <linux/init.h> /* For init/exit macros */
#include <linux/module.h> /* For MODULE_ marcros  */
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
#include <linux/pm_wakeup.h>
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
#include <linux/reboot.h>
#include "mtk_charger.h"
#include "moto_wlc.h"
#include "mtk_charger_algorithm_class.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/thermal.h>

static int wlc_dbg_level = WLC_DEBUG_LEVEL;
struct moto_wls_chg_ops *wls_chg_ops = NULL;

#define CHARGER_STATE_NUM 8
static int wlc_state_to_current_limit[CHARGER_STATE_NUM] = {
	-1, 2500000, 2000000, 1500000, 1200000, 1000000, 700000, 500000
};

int wlc_get_debug_level(void)
{
	return wlc_dbg_level;
}

void moto_wlc_control_gpio(struct chg_alg_device *alg, bool enable)
{
	struct mtk_wlc *wlc;

	wlc = dev_get_drvdata(&alg->dev);
	if (gpio_is_valid(wlc->wls_control_en))
		gpio_set_value(wlc->wls_control_en, enable);
}

static int wlc_plugout_reset(struct chg_alg_device *alg)
{
	int ret = 0, cnt = 0;
	struct mtk_wlc *wlc;

	wlc_dbg("%s\n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	switch (wlc->state) {
	case WLC_HW_UNINIT:
	case WLC_HW_FAIL:
	case WLC_HW_READY:
		break;
	case WLC_TA_NOT_SUPPORT:
		wlc->state = WLC_HW_READY;
		break;
	case WLC_RUN:
		/* set flag to end WLC thread asap */
		mutex_lock(&wlc->cable_out_lock);
		wlc->is_cable_out_occur = true;
		//moto_wlc_control_gpio(alg, true);
		mutex_unlock(&wlc->cable_out_lock);
		while (mutex_trylock(&wlc->access_lock) == 0) {
			wlc_err("%s:wlc is running state:%d cnt:%d\n", __func__,
				wlc->state, cnt);
			cnt++;
			msleep(100);
		}
		wlc_hal_vbat_mon_en(alg, CHG1, false);
		wlc->old_cv = 0;
		wlc->is_cable_out_occur = false;
		wlc->stop_6pin_re_en = 0;

		/* Enable OVP */
		//ret = wlc_hal_enable_vbus_ovp(alg, true);
		//if (ret < 0)
			//wlc_err("%s:enable vbus ovp fail, ret:%d\n", __func__,
				//ret);

		/* Set MIVR for vbus 5V */
		ret = wlc_hal_set_mivr(alg, CHG1,
				       wlc->min_charger_voltage); /* uV */
		if (ret < 0)
			wlc_err("%s:set mivr fail, ret:%d\n", __func__, ret);

		wlc_dbg("%s: OK\n", __func__);
		wlc->state = WLC_HW_READY;
		mutex_unlock(&wlc->access_lock);
		break;
	default:
		wlc_dbg("%s: error state:%d\n", __func__, wlc->state);
		break;
	}
	return 0;
}

int wlc_reset_ta_vchr(struct chg_alg_device *alg)
{
	int ret;
	struct mtk_wlc *wlc;

	wlc_dbg("%s: starts\n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	ret = wlc_hal_set_mivr(alg, CHG1, wlc->min_charger_voltage);
	if (ret < 0)
		wlc_err("%s:set mivr fail, ret:%d\n", __func__, ret);

	wlc_hal_vbat_mon_en(alg, CHG1, false);
	wlc->old_cv = 0;
	wlc->stop_6pin_re_en = 0;

	wlc_dbg("%s: OK\n", __func__);

	return ret;
}

static int wlc_leave(struct chg_alg_device *alg)
{
	int ret = 0;
	struct mtk_wlc *wlc;

	wlc_dbg("%s: starts\n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	ret = wlc_reset_ta_vchr(alg);
	if (ret != 0) {
		wlc_err("%s: failed, state = %d, ret = %d\n", __func__,
			wlc->state, ret);
	}

	//ret = wlc_hal_enable_vbus_ovp(alg, true);
	//if (ret != 0) {
	//	wlc_err("%s: enable vbus ovp fai,ret:%d\n", __func__, ret);
	//}

	wlc_hal_set_mivr(alg, CHG1, wlc->min_charger_voltage);
	if (ret != 0) {
		wlc_err("%s:set mivr fail,ret:%d\n", __func__, ret);
	}

	wlc_dbg("%s: OK\n", __func__);
	return ret;
}

static int __wlc_check_charger(struct chg_alg_device *alg)
{
	int ret = 0, ret_value = 0;
	struct mtk_wlc *wlc;

	wlc = dev_get_drvdata(&alg->dev);

	wlc_dbg("%s type:%d", __func__, wlc_hal_get_charger_type(alg));

	if (wlc_hal_get_charger_type(alg) != POWER_SUPPLY_TYPE_WIRELESS) {
		ret_value = ALG_TA_NOT_SUPPORT;
		goto out;
	}

	if (wlc->is_cable_out_occur)
		goto out;

	ret = wlc_reset_ta_vchr(alg);
	if (ret != 0)
		goto out;

	if (wlc->is_cable_out_occur)
		goto out;

	wlc_dbg("%s: OK, state = %d\n", __func__, wlc->state);

	return ret;
out:
	if (ret_value == 0)
		ret_value = ALG_TA_NOT_SUPPORT;
	wlc_dbg("%s,state:%d,plugout:%d \n", __func__,
		wlc->state, wlc->is_cable_out_occur);
	return ret_value;
}

static int _wlc_init_algo(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	int log_level;

	wlc = dev_get_drvdata(&alg->dev);
	mutex_lock(&wlc->access_lock);
	if (wlc_hal_init_hardware(alg) != 0) {
		wlc->state = WLC_HW_FAIL;
		wlc_err("%s:init hw fail\n", __func__);
	} else
		wlc->state = WLC_HW_READY;

	wlc_hal_vbat_mon_en(alg, CHG1, false);
	wlc->old_cv = 0;
	wlc->stop_6pin_re_en = 0;

	alg->config = SINGLE_CHARGER;

	log_level = wlc_hal_get_log_level(alg);
	pr_notice("%s: log_level=%d", __func__, log_level);
	if (log_level > 0)
		wlc_dbg_level = log_level;

	mutex_unlock(&wlc->access_lock);
	wlc_dbg("%s config:%d\n", __func__, alg->config);
	return 0;
}

static char *wlc_state_to_str(int state)
{
	switch (state) {
	case WLC_HW_UNINIT:
		return "WLC_HW_UNINIT";
	case WLC_HW_FAIL:
		return "WLC_HW_FAIL";
	case WLC_HW_READY:
		return "WLC_HW_READY";
	case WLC_TA_NOT_SUPPORT:
		return "WLC_TA_NOT_SUPPORT";
	case WLC_RUN:
		return "WLC_RUN";
	case WLC_DONE:
		return "WLC_DONE";
	default:
		break;
	}
	wlc_err("%s unknown state:%d\n", __func__, state);
	return "WLC_UNKNOWN";
}

static int _wlc_is_algo_ready(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	int ret_value;

	wlc = dev_get_drvdata(&alg->dev);

	mutex_lock(&wlc->access_lock);
	__pm_stay_awake(wlc->suspend_lock);
	wlc_dbg("%s state:%s\n", __func__, wlc_state_to_str(wlc->state));

	switch (wlc->state) {
	case WLC_HW_UNINIT:
	case WLC_HW_FAIL:
		ret_value = ALG_INIT_FAIL;
		break;
	case WLC_HW_READY:
		if (wlc_hal_get_charger_type(alg) != POWER_SUPPLY_TYPE_WIRELESS)
			ret_value = ALG_TA_NOT_SUPPORT;
		else
			ret_value = ALG_READY;
		break;
	case WLC_TA_NOT_SUPPORT:
		ret_value = ALG_TA_NOT_SUPPORT;
		break;
	case WLC_RUN:
		ret_value = ALG_RUNNING;
		break;
	default:
		ret_value = ALG_INIT_FAIL;
		break;
	}
	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);
	wlc_dbg("%s ret_value:%d\n", __func__, ret_value);
	return ret_value;
}

#define VBUS_DEFAULT_MV 5000
static int wlc_sc_set_charger(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	int ichg1_min = -1, aicr1_min = -1;
	int ret;
	int charging_current, input_current, vbus, input_thermal_limit;
	bool cable_ready = false;
	int temp = 0, batt_cv = 0;
	bool cv_auto = true;
	wlc_dbg("%s \n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	if (wlc->input_current_limit1 == 0 ||
	    wlc->charging_current_limit1 == 0) {
		pr_notice("input/charging current is 0, end WLC\n");
		return -1;
	}
	mutex_lock(&wlc->data_lock);

	input_current = wlc->wireless_charger_max_input_current;
	vbus = VBUS_DEFAULT_MV;
	charging_current = wlc->wireless_charger_max_current;

	if (NULL != wls_chg_ops)
		wls_chg_ops->wls_current_select(&input_current, &vbus,
						&cable_ready);

	wlc->cable_ready = cable_ready;
	if (!cable_ready) {
		mutex_unlock(&wlc->data_lock);
		wlc_hal_set_charging_current(alg, CHG1, 3150000);
		temp = wlc_hal_get_batt_temp(alg);
		if (temp > 15 && temp <= 45)
			cv_auto = false;
	} else {
		cv_auto = true;
		if (wlc->charging_current_limit1 != -1) {
			if (wlc->charging_current_limit1 < charging_current)
				wlc->charging_current1 =
					wlc->charging_current_limit1;
			ret = wlc_hal_get_min_charging_current(alg, CHG1,
							       &ichg1_min);
			if (ret != -EOPNOTSUPP &&
			    wlc->charging_current_limit1 < ichg1_min)
				wlc->charging_current1 = 0;

			input_thermal_limit =
				wlc->charging_current_limit1 / 1000;
			input_thermal_limit *= VBUS_DEFAULT_MV;
			input_thermal_limit /= vbus;
			input_thermal_limit *= 1000;
		} else {
			wlc->charging_current1 = charging_current;
			input_thermal_limit = -1;
		}
		wlc->charging_current1 = wlc->charging_current1 < wlc->mmi_fcc ?
						       wlc->charging_current1 :
						       wlc->mmi_fcc;

		if (input_thermal_limit != -1 &&
		    input_thermal_limit < input_current) {
			wlc->input_current1 = input_thermal_limit;
			ret = wlc_hal_get_min_input_current(alg, CHG1,
							    &aicr1_min);
			if (ret != -EOPNOTSUPP &&
			    input_thermal_limit < aicr1_min)
				wlc->input_current1 = 0;
			wlc_info("%s input_thermal_limit:%d aicr1_min:%d\n",
					__func__, input_thermal_limit, aicr1_min);
		} else
			wlc->input_current1 = input_current;

		if (NULL != wls_chg_ops &&
			NULL != wls_chg_ops->wls_notify_thermal_icl)
			wls_chg_ops->wls_notify_thermal_icl(input_thermal_limit);

		wlc_info("%s input current = %d:%d:%d:%d, vout = %d\n",
			 __func__, wlc->input_current1, input_current,
			 input_thermal_limit, wlc->charging_current_limit1,
			 vbus);
		mutex_unlock(&wlc->data_lock);

		if (wlc->input_current1 == 0 || wlc->charging_current1 == 0) {
			wlc_err("current is zero %d %d\n", wlc->input_current1,
				wlc->charging_current1);
			return -1;
		}

		wlc_hal_set_charging_current(alg, CHG1, wlc->charging_current1);
		wlc_hal_set_input_current(alg, CHG1, wlc->input_current1);
	}
	if (wlc->old_cv == 0 || (wlc->old_cv != wlc->cv) ||
	    wlc->wlc_6pin_en == 0) {
		wlc_hal_vbat_mon_en(alg, CHG1, false);
		batt_cv = wlc_hal_get_batt_cv(alg);
		if(!cv_auto)
			wlc_hal_set_cv(alg, CHG1, batt_cv);
		else
			wlc_hal_set_cv(alg, CHG1, wlc->cv);
		if (wlc->wlc_6pin_en && wlc->stop_6pin_re_en != 1)
			wlc_hal_vbat_mon_en(alg, CHG1, true);

		wlc_dbg("%s old_cv=%d, new_cv=%d, wlc_6pin_en=%d\n", __func__,
			wlc->old_cv, wlc->cv, wlc->wlc_6pin_en);

		wlc->old_cv = wlc->cv;
	} else {
		if (wlc->wlc_6pin_en && wlc->stop_6pin_re_en != 1) {
			wlc->stop_6pin_re_en = 1;
			wlc_hal_vbat_mon_en(alg, CHG1, true);
		}
	}

	wlc_dbg("%s m:%d s:%d cv:%d chg1:%d,%d,%d, min:%d:%d, 6pin_en:%d, 6pin_re_en=%d\n",
		__func__, alg->config, wlc->state, wlc->cv, wlc->input_current1,
		wlc->charging_current1, wlc->mmi_fcc, ichg1_min, aicr1_min,
		wlc->wlc_6pin_en, wlc->stop_6pin_re_en);

	return 0;
}

static int __wlc_run(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	int ret = 0, ret_value = 0, uisoc = 0;
	wlc_dbg("%s \n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	if (wlc_hal_get_charger_type(alg) != POWER_SUPPLY_TYPE_WIRELESS) {
		ret_value = ALG_TA_NOT_SUPPORT;
		goto out;
	}

	uisoc = wlc_hal_get_uisoc(alg);
	if((NULL != wls_chg_ops) && true == wlc->cable_ready && uisoc == 100)
		wls_chg_ops->wls_set_battery_soc(uisoc);

	if (wlc_sc_set_charger(alg) != 0) {
		ret = wlc_leave(alg);
		ret_value = ALG_DONE;
		goto out;
	}

	wlc_dbg("%s cv:%d chg1:%d,%d uisoc %d\n", __func__, wlc->cv, wlc->input_current1,
		wlc->charging_current1, uisoc);

out:
	wlc_dbg("%s: plugout=%d\n", __func__, wlc->is_cable_out_occur);

	wlc_dbg("%s: state = %s, ret = %d:%d\n", __func__,
		wlc_state_to_str(wlc->state),ret, ret_value);
	return ret_value;
}

static int _wlc_start_algo(struct chg_alg_device *alg)
{
	int ret = 0, ret_value = 0;
	struct mtk_wlc *wlc;
	bool again;
	wlc_dbg("%s \n", __func__);
	wlc = dev_get_drvdata(&alg->dev);
	mutex_lock(&wlc->access_lock);
	__pm_stay_awake(wlc->suspend_lock);

	do {
		wlc_info("%s state:%d %s %d\n", __func__, wlc->state,
			 wlc_state_to_str(wlc->state), again);

		again = false;
		switch (wlc->state) {
		case WLC_HW_UNINIT:
		case WLC_HW_FAIL:
			ret_value = ALG_INIT_FAIL;
			break;
		case WLC_HW_READY:
			ret = __wlc_check_charger(alg);
			if (ret == 0) {
				wlc->state = WLC_RUN;
				ret_value = ALG_READY;
				again = true;
			} else {
				wlc->state = WLC_TA_NOT_SUPPORT;
				ret_value = ALG_TA_NOT_SUPPORT;
			}
			break;
		case WLC_TA_NOT_SUPPORT:
			ret_value = ALG_TA_NOT_SUPPORT;
			break;
		case WLC_RUN:
			ret = __wlc_run(alg);
			if (ret == ALG_TA_NOT_SUPPORT)
				wlc->state = WLC_TA_NOT_SUPPORT;
			else if (ret == ALG_DONE)
				wlc->state = WLC_HW_READY;
			ret_value = ret;
			break;
		default:
			wlc_err("WLC unknown state:%d\n", wlc->state);
			ret_value = ALG_INIT_FAIL;
			break;
		}
	} while (again == true);
	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);

	return ret_value;
}

static bool _wlc_is_algo_running(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;

	wlc_dbg("%s\n", __func__);
	wlc = dev_get_drvdata(&alg->dev);

	if (wlc->state == WLC_RUN)
		return true;

	return false;
}

static int _wlc_stop_algo(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;

	wlc = dev_get_drvdata(&alg->dev);

	wlc_dbg("%s %d\n", __func__, wlc->state);
	if (wlc->state == WLC_RUN) {
		if (NULL != wls_chg_ops &&
			true == wlc->cable_ready &&
			NULL != wls_chg_ops->wls_stop_epp) {
			wls_chg_ops->wls_stop_epp();
		}
		wlc_reset_ta_vchr(alg);
		wlc->state = WLC_HW_READY;
	}

	return 0;
}

static int wlc_full_event(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	int ret_value = 0;
	wlc_dbg("%s \n", __func__);
	wlc = dev_get_drvdata(&alg->dev);
	switch (wlc->state) {
	case WLC_HW_UNINIT:
	case WLC_HW_FAIL:
	case WLC_HW_READY:
	case WLC_TA_NOT_SUPPORT:
		break;
	case WLC_RUN:
		if (wlc->state == WLC_RUN) {
			wlc_err("%s evt full\n", __func__);
			wlc_leave(alg);
			wlc->state = WLC_DONE;
		}
		break;
	default:
		wlc_dbg("%s: error state:%d\n", __func__, wlc->state);
		break;
	}

	return ret_value;
}

static int _wlc_notifier_call(struct chg_alg_device *alg,
			      struct chg_alg_notify *notify)
{
	struct mtk_wlc *wlc;
	int ret_value = 0;

	wlc = dev_get_drvdata(&alg->dev);
	wlc_err("%s evt:%d state:%s\n", __func__, notify->evt,
		wlc_state_to_str(wlc->state));

	switch (notify->evt) {
	case EVT_PLUG_OUT:
		wlc->stop_6pin_re_en = 0;
		ret_value = wlc_plugout_reset(alg);
		break;
	case EVT_FULL:
		wlc->stop_6pin_re_en = 1;
		ret_value = wlc_full_event(alg);
		break;
	case EVT_BATPRO_DONE:
		wlc->wlc_6pin_en = 0;
		ret_value = 0;
		break;
	case EVT_RECHARGE:
		if (wlc->state == WLC_DONE) {
			wlc_err("%s evt recharge\n", __func__);
			moto_wlc_control_gpio(alg, false);
			wlc->state = WLC_HW_READY;
		}
		break;
	default:
		ret_value = -EINVAL;
	}
	return ret_value;
}

static void mtk_wlc_parse_dt(struct mtk_wlc *wlc, struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 val;
	int i;
	wlc_dbg("%s \n", __func__);
	wlc->wls_control_en = of_get_named_gpio(np, "mmi,wls_control_en", 0);
	if (!gpio_is_valid(wlc->wls_control_en))
		pr_err("wls_control_en is %d invalid\n", wlc->wls_control_en);

	if (of_property_read_u32(np, "min_charger_voltage", &val) >= 0)
		wlc->min_charger_voltage = val;
	else {
		pr_notice("use default V_CHARGER_MIN:%d\n", WLC_V_CHARGER_MIN);
		wlc->min_charger_voltage = WLC_V_CHARGER_MIN;
	}
	if (of_property_read_u32(np, "wlc_max_charger_current", &val) >= 0) {
		wlc->wireless_charger_max_current = val;
	} else {
		pr_err("use default WIRELESS_CHARGER_MAX_CURRENT:%d\n",
			WIRELESS_CHARGER_MAX_CURRENT);
		wlc->wireless_charger_max_current =
			WIRELESS_CHARGER_MAX_CURRENT;
	}

	if (of_property_read_u32(np, "wlc_max_input_current", &val) >= 0) {
		wlc->wireless_charger_max_input_current = val;
	} else {
		pr_err("use default WIRELESS_CHARGER_MAX_INPUT_CURRENT:%d\n",
			WIRELESS_CHARGER_MAX_INPUT_CURRENT);
		wlc->wireless_charger_max_input_current = WIRELESS_CHARGER_MAX_INPUT_CURRENT;
	}

	if (of_property_read_u32_array(np, "mmi,wlc-rx-mitigation",
			wlc_state_to_current_limit, CHARGER_STATE_NUM))
		pr_info("Not define wlc rx thermal table, use defaut table\n");

	for (i = 0; i < CHARGER_STATE_NUM; i++) {
		pr_info("mmi wlc rx table: table %d, current %d mA\n",
			i, wlc_state_to_current_limit[i]);
	}
}

int _wlc_get_prop(struct chg_alg_device *alg,
		enum chg_alg_props s, int *value)
{

	pr_notice("%s\n", __func__);
	if (s == ALG_MAX_VBUS)
		*value = 12000;
	else
		pr_notice("%s does not support prop:%d\n", __func__, s);
	return 0;
}

int _wlc_set_prop(struct chg_alg_device *alg,
		enum chg_alg_props s, int value)
{
	struct mtk_wlc *wlc;
	struct power_supply *psy = NULL;
	union  power_supply_propval chip_state;
	int ret = 0;

	pr_notice("%s %d %d\n", __func__, s, value);

	wlc = dev_get_drvdata(&alg->dev);

	switch (s) {
	case ALG_LOG_LEVEL:
		wlc_dbg_level = value;
		break;
	case ALG_REF_VBAT:
		wlc->ref_vbat = value;
		break;
	case ALG_WLC_STATE:
		moto_wlc_control_gpio(alg, !value);
		if (!value) {
			wlc_plugout_reset(alg);
		}
		if (NULL != wls_chg_ops) {
			psy = power_supply_get_by_name("wireless");
			if (!IS_ERR_OR_NULL(psy)) {
				chip_state.intval = value;
				ret = power_supply_set_property(
					psy,
					POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
					&chip_state);
				if (ret < 0)
					chr_err("set wlc chip state fail\n");
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int _wlc_set_setting(struct chg_alg_device *alg_dev,
		     struct chg_limit_setting *setting)
{
	struct mtk_wlc *wlc;

	wlc = dev_get_drvdata(&alg_dev->dev);

	//wlc_dbg("%s cv:%d icl:%d,%d cc:%d,%d, mmi_fcc:%d, 6pin_en:%d\n",
	//	__func__, setting->cv, setting->input_current_limit1,
	//	setting->input_current_limit2, setting->charging_current_limit1,
	//	setting->charging_current_limit2, setting->mmi_fcc_limit,
	//	setting->vbat_mon_en);

	mutex_lock(&wlc->access_lock);
	__pm_stay_awake(wlc->suspend_lock);
	wlc->cv = setting->cv;
	wlc->wlc_6pin_en = setting->vbat_mon_en;
	wlc->input_current_limit1 = setting->input_current_limit1;
	wlc->input_current_limit2 = setting->input_current_limit2;
	wlc->charging_current_limit2 = setting->charging_current_limit2;
	//wlc->mmi_fcc = setting->mmi_fcc_limit;

	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);

	return 0;
}

static struct chg_alg_ops wlc_alg_ops = {
	.init_algo = _wlc_init_algo,
	.is_algo_ready = _wlc_is_algo_ready,
	.start_algo = _wlc_start_algo,
	.is_algo_running = _wlc_is_algo_running,
	.stop_algo = _wlc_stop_algo,
	.notifier_call = _wlc_notifier_call,
	.get_prop = _wlc_get_prop,
	.set_prop = _wlc_set_prop,
	.set_current_limit = _wlc_set_setting,
};

int moto_wireless_chg_ops_register(struct moto_wls_chg_ops *ops)
{
	if (!ops) {
		pr_err("%s invalide wls chg ops(null)\n", __func__);
		return -EINVAL;
	}

	wls_chg_ops = ops;

	return 0;
}
EXPORT_SYMBOL(moto_wireless_chg_ops_register);

static int wlc_tcd_get_max_state(struct thermal_cooling_device *tcd,
	unsigned long *state)
{
	struct mtk_wlc *wlc = tcd->devdata;

	*state = wlc->max_state;

	return 0;
}

static int wlc_tcd_get_cur_state(struct thermal_cooling_device *tcd,
	unsigned long *state)
{
	struct mtk_wlc *wlc = tcd->devdata;

	*state = wlc->cur_state;

	return 0;
}

static int wlc_tcd_set_cur_state(struct thermal_cooling_device *tcd,
	unsigned long state)
{
	struct mtk_wlc *wlc = tcd->devdata;

	if (state > wlc->max_state)
		return -EINVAL;

	wlc->charging_current_limit1 = wlc_state_to_current_limit[state];
	wlc->cur_state = state;

	if (NULL != wls_chg_ops &&
		NULL != wls_chg_ops->wls_notify_cur_state)
		wls_chg_ops->wls_notify_cur_state(state, wlc->charging_current_limit1);

	wlc_info("%s cur state = %d, config state = %ld, cur limt = %d\n",
		__func__, wlc->cur_state, state, wlc->charging_current_limit1);

	return 0;
}

static const struct thermal_cooling_device_ops wlc_tcd_ops = {
	.get_max_state = wlc_tcd_get_max_state,
	.get_cur_state = wlc_tcd_get_cur_state,
	.set_cur_state = wlc_tcd_set_cur_state,
};

static int mtk_wlc_probe(struct platform_device *pdev)
{
	struct mtk_wlc *wlc = NULL;
	int rc;

	pr_notice("%s: starts\n", __func__);

	wlc = devm_kzalloc(&pdev->dev, sizeof(*wlc), GFP_KERNEL);
	if (!wlc)
		return -ENOMEM;
	platform_set_drvdata(pdev, wlc);
	wlc->pdev = pdev;

	wlc->suspend_lock =
		wakeup_source_register(NULL, "WLC suspend wakelock");

	mutex_init(&wlc->access_lock);
	mutex_init(&wlc->cable_out_lock);
	mutex_init(&wlc->data_lock);

	wlc->state = WLC_HW_UNINIT;
	mtk_wlc_parse_dt(wlc, &pdev->dev);

	if(gpio_is_valid(wlc->wls_control_en)) {
		rc  = devm_gpio_request_one(&wlc->pdev->dev, wlc->wls_control_en,
				  GPIOF_OUT_INIT_LOW, "wls_control_en");
		if (rc  < 0)
			pr_err(" [%s] Failed to request wls_control_en gpio, ret:%d", __func__, rc);
	}

	wlc->alg = chg_alg_device_register("wlc", &pdev->dev, wlc, &wlc_alg_ops,
					   NULL);

	/* Register thermal zone cooling device */
	wlc->charging_current_limit1 = -1;
	wlc->max_state = CHARGER_STATE_NUM - 1;
	wlc->tcd = thermal_of_cooling_device_register(dev_of_node(&pdev->dev),
		"moto_wlc", wlc, &wlc_tcd_ops);

	return 0;
}

static int mtk_wlc_remove(struct platform_device *dev)
{
	struct mtk_wlc *wlc;

	wlc = (struct mtk_wlc  *)platform_get_drvdata(dev);
	if(wlc->tcd)
		thermal_cooling_device_unregister(wlc->tcd);

	return 0;
}

static void mtk_wlc_shutdown(struct platform_device *dev)
{
}

static const struct of_device_id mtk_wlc_of_match[] = {
	{
		.compatible = "moto,charger,wlc",
	},
	{},
};

MODULE_DEVICE_TABLE(of, mtk_wlc_of_match);

struct platform_device wlc_device = {
	.name = "wlc",
	.id = -1,
};

static struct platform_driver wlc_driver = {
	.probe = mtk_wlc_probe,
	.remove = mtk_wlc_remove,
	.shutdown = mtk_wlc_shutdown,
	.driver = {
		   .name = "wlc",
		   .of_match_table = mtk_wlc_of_match,
	},
};

static int __init mtk_wlc_init(void)
{
	return platform_driver_register(&wlc_driver);
}
module_init(mtk_wlc_init);

static void __exit mtk_wlc_exit(void)
{
	platform_driver_unregister(&wlc_driver);
}
module_exit(mtk_wlc_exit);

MODULE_DESCRIPTION("moto wlc algorithm Driver");
MODULE_LICENSE("GPL");

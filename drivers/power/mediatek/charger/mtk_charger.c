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

#include "mtk_charger_intf.h"
#include <mt-plat/charger_type.h>
#include <mt-plat/mtk_battery.h>
#include <musb_core.h>

static struct charger_manager *pinfo;
static struct list_head consumer_head = LIST_HEAD_INIT(consumer_head);
static DEFINE_MUTEX(consumer_mutex);

#define USE_FG_TIMER 1


/*=============== fix me==================*/


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

int mtk_chr_is_charger_exist(unsigned char *exist)
{
	if (mt_get_charger_type() == CHARGER_UNKNOWN)
		*exist = 0;
	else
		*exist = 1;
	return 0;
}

/*=============== fix me==================*/

/* log */
#include <linux/string.h>

char chargerlog[1000];
#define LOG_LENGTH 500
int chargerlog_level = 10;
int chargerlogIdx;

int charger_get_debug_level(void)
{
	return chargerlog_level;
}

void charger_log(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsprintf(chargerlog + chargerlogIdx, fmt, args);
	va_end(args);
	chargerlogIdx = strlen(chargerlog);
	if (chargerlogIdx >= LOG_LENGTH) {
		pr_err("%s", chargerlog);
		chargerlogIdx = 0;
		memset(chargerlog, 0, 1000);
	}
}

void charger_log_flash(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsprintf(chargerlog + chargerlogIdx, fmt, args);
	va_end(args);
	pr_err("%s", chargerlog);
	chargerlogIdx = 0;
	memset(chargerlog, 0, 1000);
}

/* log */
void _wake_up_charger(struct charger_manager *info)
{
	unsigned long flags;

	if (info == NULL)
		return;

	spin_lock_irqsave(&info->slock, flags);
	if (wake_lock_active(&info->charger_wakelock) == 0)
		wake_lock(&info->charger_wakelock);
	spin_unlock_irqrestore(&info->slock, flags);
	info->charger_thread_timeout = true;
	wake_up(&info->wait_que);
}

/* charger_manager ops  */
static int _mtk_charger_change_current_setting(struct charger_manager *info)
{
	if (info != NULL && info->change_current_setting)
		return info->change_current_setting(info);

	return 0;
}

static int _mtk_charger_do_charging(struct charger_manager *info, bool en)
{
	if (info != NULL && info->do_charging) {
		info->do_charging(info, en);
	}
	return 0;
}
/* charger_manager ops end */


/* user interface */
struct charger_consumer *charger_manager_get_by_name(struct device *dev,
	const char *name)
{
	struct charger_consumer *puser;

	puser = kzalloc(sizeof(struct charger_consumer), GFP_KERNEL);
	if (puser == NULL)
		return NULL;

	mutex_lock(&consumer_mutex);
	puser->dev = dev;

	if (pinfo == NULL)
		list_add(&puser->list, &consumer_head);
	else
	puser->cm = pinfo;
	mutex_unlock(&consumer_mutex);

	return puser;
}

static int _charger_manager_enable_charging(struct charger_consumer *consumer,
	int idx, bool en)
{
	struct charger_manager *info = consumer->cm;

	pr_err("%s: dev:%s idx:%d en:%d\n", __func__, dev_name(consumer->dev),
		idx, en);

	if (info != NULL) {
		struct charger_data *pdata;

		if (idx == 0)
			pdata = &info->chg1_data;
		else if (idx == 1)
			pdata = &info->chg2_data;
		else
			return -ENOTSUPP;

		if (en == false) {
			if (pdata->disable_charging_count == 0)
				_mtk_charger_do_charging(info, en);

			pdata->disable_charging_count++;
		} else {
			if (pdata->disable_charging_count == 1) {
				_mtk_charger_do_charging(info, en);
				pdata->disable_charging_count = 0;
			} else if (pdata->disable_charging_count > 1)
				pdata->disable_charging_count--;
		}
		pr_err("%s: dev:%s idx:%d en:%d cnt:%d\n", __func__, dev_name(consumer->dev),
			idx, en, pdata->disable_charging_count);

		return 0;
	}
	return -EBUSY;

}

int charger_manager_enable_charging(struct charger_consumer *consumer,
	int idx, bool en)
{
	struct charger_manager *info = consumer->cm;
	int ret = 0;

	mutex_lock(&info->charger_lock);
	ret = _charger_manager_enable_charging(consumer, idx, en);
	mutex_unlock(&info->charger_lock);
	return ret;
}

int charger_manager_set_input_current_limit(struct charger_consumer *consumer,
	int idx, int input_current)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
		struct charger_data *pdata;

		if (idx == 0)
			pdata = &info->chg1_data;
		else if (idx == 1)
			pdata = &info->chg2_data;
		else
			return -ENOTSUPP;

		pdata->thermal_input_current_limit = input_current;
		_mtk_charger_change_current_setting(info);
		_wake_up_charger(info);
		return 0;
	}
	return -EBUSY;
}

int charger_manager_set_charging_current_limit(struct charger_consumer *consumer,
	int idx, int charging_current)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
		struct charger_data *pdata;

		if (idx == 0)
			pdata = &info->chg1_data;
		else if (idx == 1)
			pdata = &info->chg2_data;
		else
			return -ENOTSUPP;

		pdata->thermal_charging_current_limit = charging_current;
		_mtk_charger_change_current_setting(info);
		_wake_up_charger(info);
		return 0;
	}
	return -EBUSY;
}

int charger_manager_force_charging_current(struct charger_consumer *consumer,
	int idx, int charging_current)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
		struct charger_data *pdata;

		if (idx == 0)
			pdata = &info->chg1_data;
		else if (idx == 1)
			pdata = &info->chg2_data;
		else
			return -ENOTSUPP;

		pdata->force_charging_current = charging_current;
		_mtk_charger_change_current_setting(info);
		_wake_up_charger(info);
		return 0;
	}
	return -EBUSY;
}

int charger_manager_set_pe30_input_current_limit(struct charger_consumer *consumer,
	int idx, int input_current)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
		mtk_pe30_set_charging_current_limit(info, input_current / 1000);
		return 0;
	}
	return -EBUSY;
}

int charger_manager_get_pe30_input_current_limit(struct charger_consumer *consumer,
	int idx, int *input_current, int *min_current, int *max_current)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
		*input_current = mtk_pe30_get_charging_current_limit(info) * 1000;
		*min_current = CC_END * 1000;
		*max_current = CC_INIT * 1000;
		return 0;
	}
	return -EBUSY;
}

int charger_manager_get_current_charging_type(struct charger_consumer *consumer)
{
	struct charger_manager *info = consumer->cm;

	if (info != NULL) {
	if (mtk_pe20_get_is_connect(info))
		return 2;

	if (mtk_is_pe30_running(info))
		return 3;
	}

	return 0;
}


int register_charger_manager_notifier(struct charger_consumer *consumer,
	struct notifier_block *nb)
{
	int ret = 0;
	struct charger_manager *info = consumer->cm;


	mutex_lock(&consumer_mutex);
	if (info != NULL)
	ret = srcu_notifier_chain_register(&info->evt_nh, nb);
	else
		consumer->pnb = nb;
	mutex_unlock(&consumer_mutex);

	return ret;
}

int unregister_charger_manager__notifier(struct charger_consumer *consumer,
				struct notifier_block *nb)
{
	int ret = 0;
	struct charger_manager *info = consumer->cm;

	mutex_lock(&consumer_mutex);
	if (info != NULL)
		ret = srcu_notifier_chain_unregister(&info->evt_nh, nb);
	else
		consumer->pnb = NULL;
	mutex_unlock(&consumer_mutex);

	return ret;
}

/* user interface end*/

/* sw jeita */
void do_sw_jeita_state_machine(struct charger_manager *info)
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
	} else {
		sw_jeita->cv = 0;
	}

	pr_err("[SW_JEITA]preState:%d newState:%d tmp:%d cv:%d\n\r",
		pre_sm, sw_jeita->sm, info->battery_temperature, sw_jeita->cv);
}

static ssize_t show_sw_jeita(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;

	pr_err("show_sw_jeita: %d\n", pinfo->enable_sw_jeita);
	return sprintf(buf, "%d\n", pinfo->enable_sw_jeita);
}

static ssize_t store_sw_jeita(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct charger_manager *pinfo = dev->driver_data;
	signed int temp;

	if (kstrtoint(buf, 10, &temp) == 0) {
		if (temp == 0)
			pinfo->enable_sw_jeita = false;
		else
			pinfo->enable_sw_jeita = true;

	} else {
		pr_err("store_sw_jeita: format error!\n");
	}
	return size;
}

static DEVICE_ATTR(sw_jeita, 0664, show_sw_jeita,
		   store_sw_jeita);
/* sw jeita end*/

/* pump express series */
bool mtk_is_pep_series_connect(struct charger_manager *info)
{
	if (mtk_is_pe30_running(info) || mtk_pe20_get_is_connect(info))
		return true;

	return false;
}

static ssize_t show_pe20(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;

	pr_err("show_pe20: %d\n", pinfo->enable_pe_2);
	return sprintf(buf, "%d\n", pinfo->enable_pe_2);
}

static ssize_t store_pe20(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct charger_manager *pinfo = dev->driver_data;
	signed int temp;

	if (kstrtoint(buf, 10, &temp) == 0) {
		if (temp == 0)
			pinfo->enable_pe_2 = false;
		else
			pinfo->enable_pe_2 = true;

	} else {
		pr_err("store_pe20: format error!\n");
	}
	return size;
}

static DEVICE_ATTR(pe20, 0664, show_pe20,
		   store_pe20);

static ssize_t show_pe30(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;

	pr_err("show_pe30: %d\n", pinfo->enable_pe_3);
	return sprintf(buf, "%d\n", pinfo->enable_pe_3);
}

static ssize_t store_pe30(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	struct charger_manager *pinfo = dev->driver_data;
	signed int temp;

	if (kstrtoint(buf, 10, &temp) == 0) {
		if (temp == 0)
			pinfo->enable_pe_3 = false;
		else
			pinfo->enable_pe_3 = true;

	} else {
		pr_err("store_pe30: format error!\n");
	}
	return size;
}

static DEVICE_ATTR(pe30, 0664, show_pe30,
		   store_pe30);


/* pump express series end*/

int mtk_get_dynamic_cv(struct charger_manager *info, unsigned int *cv)
{
	int ret = 0;
	u32 _cv, _cv_temp;
	unsigned int vbat_threshold[4] = {3400000, 0, 0, 0};
	u32 vbat_bif = 0, vbat_auxadc = 0, vbat = 0;
	u32 retry_cnt = 0;

	if (pmic_is_bif_exist()) {
		if (!info->enable_dynamic_cv) {
			_cv = BIF_CV;
			goto _out;
		}

		do {
			vbat_auxadc = battery_get_bat_voltage() * 1000;
			ret = pmic_get_bif_battery_voltage(&vbat_bif);
			vbat_bif = vbat_bif * 1000;
			if (ret >= 0 && vbat_bif != 0 && vbat_bif < vbat_auxadc) {
				vbat = vbat_bif;
				pr_err("%s: use BIF vbat = %duV, dV to auxadc = %duV\n",
					__func__, vbat, vbat_auxadc - vbat_bif);
				break;
			}
			retry_cnt++;
		} while (retry_cnt < 5);

		if (retry_cnt == 5) {
			ret = 0;
			vbat = vbat_auxadc;
			pr_err("%s: use AUXADC vbat = %duV, since BIF vbat = %duV\n",
				__func__, vbat_auxadc, vbat_bif);
		}

		/* Adjust CV according to the obtained vbat */
		vbat_threshold[1] = BIF_THRESHOLD1;
		vbat_threshold[2] = BIF_THRESHOLD2;
		_cv_temp = BIF_CV_UNDER_THRESHOLD2;

		if (vbat >= vbat_threshold[0] && vbat < vbat_threshold[1])
			_cv = 4608000;
		else if (vbat >= vbat_threshold[1] && vbat < vbat_threshold[2])
			_cv = _cv_temp;
		else {
			_cv = BIF_CV;
			info->enable_dynamic_cv = false;
		}
_out:
		*cv = _cv;
		pr_err("%s: CV = %duV, enable_dynamic_cv = %d\n",
			__func__, _cv, info->enable_dynamic_cv);
	} else
		ret = -ENOTSUPP;

	return ret;
}

int charger_manager_notifier(struct charger_manager *info, int event)
{
	return srcu_notifier_call_chain(
		&info->evt_nh, event, NULL);
}

int charger_psy_event(struct notifier_block *nb, unsigned long event, void *v)
{
	struct charger_manager *info = container_of(nb, struct charger_manager, psy_nb);
	struct power_supply *psy = v;
	union power_supply_propval val;
	int ret;
	int tmp = 0;

	if (strcmp(psy->desc->name, "battery") == 0) {
		ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_batt_temp, &val);
		if (!ret) {
			tmp = val.intval / 10;
			if (info->battery_temperature != tmp && mt_get_charger_type() != CHARGER_UNKNOWN) {
				_wake_up_charger(info);
				pr_err("charger_psy_event %ld %s tmp:%d %d chr:%d\n", event, psy->desc->name, tmp,
					info->battery_temperature, mt_get_charger_type());
			}
		}
	}


	return NOTIFY_DONE;
}

void mtk_charger_int_handler(void)
{
	struct charger_manager *info = pinfo;

	pr_err("mtk_charger_int_handler ,type:%d\n", mt_get_charger_type());

	if (pinfo == NULL)
		return;

	_wake_up_charger(info);
}

static int mtk_charger_plug_in(struct charger_manager *info, CHARGER_TYPE chr_type)
{
	info->chr_type = chr_type;
	info->charger_thread_polling = true;


	info->can_charging = true;
	info->chg1_data.thermal_charging_current_limit = -1;
	info->chg1_data.thermal_input_current_limit = -1;

	pr_err("mtk_is_charger_on plug in, tyupe:%d\n", chr_type);
	if (info->plug_in != NULL)
		info->plug_in(info);

	charger_dev_plug_in(info->chg1_dev);
	return 0;
}

static int mtk_charger_plug_out(struct charger_manager *info)
{
	struct charger_data *pdata1 = &info->chg1_data;
	struct charger_data *pdata2 = &info->chg2_data;

	pr_err("mtk_charger_plug_out\n");
	info->chr_type = CHARGER_UNKNOWN;
	info->charger_thread_polling = false;

	pdata1->disable_charging_count = 0;
	pdata2->disable_charging_count = 0;

	if (info->plug_out != NULL)
		info->plug_out(info);

	charger_dev_plug_out(info->chg1_dev);
	return 0;
}

static bool mtk_is_charger_on(struct charger_manager *info)
{
	CHARGER_TYPE chr_type;

	chr_type = mt_get_charger_type();
	pr_err("mtk_is_charger_on %d %d\n", chr_type, info->chr_type);
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

static void charger_update_data(struct charger_manager *info)
{
	info->battery_temperature = battery_meter_get_battery_temperature();
}

static void mtk_battery_notify_VCharger_check(struct charger_manager *info)
{
#if defined(BATTERY_NOTIFY_CASE_0001_VCHARGER)
	int vchr = 0;

	vchr = pmic_get_vbus();
	if (vchr > info->data.max_charger_voltage) {
		info->notify_code |= 0x0001;
		pr_err("[BATTERY] charger_vol(%d) > %d mV\n",
			vchr, info->data.max_charger_voltage);
	} else {
		info->notify_code &= ~(0x0001);
	}
	if (info->notify_code != 0x0000)
		pr_err("[BATTERY] BATTERY_NOTIFY_CASE_0001_VCHARGER (%x)\n",
			info->notify_code);
#endif
}

static void mtk_battery_notify_VBatTemp_check(struct charger_manager *info)
{
#if defined(BATTERY_NOTIFY_CASE_0002_VBATTEMP)
	if (info->battery_temperature >= info->thermal.max_charge_temperature) {
		info->notify_code |= 0x0002;
		pr_err("[BATTERY] bat_temp(%d) out of range(too high)\n",
			info->battery_temperature);
	}

	if (info->enable_sw_jeita == true) {
		if (info->battery_temperature < TEMP_NEG_10_THRESHOLD) {
			info->notify_code |= 0x0020;
			pr_err("[BATTERY] bat_temp(%d) out of range(too low)\n",
				info->battery_temperature);
		}
	} else {
#ifdef BAT_LOW_TEMP_PROTECT_ENABLE
		if (info->battery_temperature < info->thermal.min_charge_temperature) {
			info->notify_code |= 0x0020;
			pr_err("[BATTERY] bat_temp(%d) out of range(too low)\n",
				info->battery_temperature);
		}
#endif
	}
#endif
}

static void mtk_battery_notify_UI_test(struct charger_manager *info)
{
	if (info->notify_test_mode == 0x0001) {
		info->notify_code = 0x0001;
		pr_debug("[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0001_VCHARGER\n");
	} else if (info->notify_test_mode == 0x0002) {
		info->notify_code = 0x0002;
		pr_debug("[BATTERY_TestMode] BATTERY_NOTIFY_CASE_0002_VBATTEMP\n");
	} else {
		pr_debug("[BATTERY] Unknown BN_TestMode Code : %x\n",
			info->notify_test_mode);
	}
}

static void mtk_battery_notify_check(struct charger_manager *info)
{
	info->notify_code = 0x0000;

	if (info->notify_test_mode == 0x0000) {
		mtk_battery_notify_VCharger_check(info);
		mtk_battery_notify_VBatTemp_check(info);
	} else {
		mtk_battery_notify_UI_test(info);
	}
}

static void charger_check_status(struct charger_manager *info)
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
	} else {

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
	}

	if (info->cmd_discharging)
		charging = false;

stop_charging:
	mtk_battery_notify_check(info);

	pr_err("tmp:%d (jeita:%d sm:%d cv:%d en:%d) (sm:%d) en:%d\n", temperature,
		info->enable_sw_jeita, info->sw_jeita.sm, info->sw_jeita.cv,
		info->sw_jeita.charging,
		thermal->sm, charging);

	if (charging != info->can_charging)
		_charger_manager_enable_charging(info->chg1_consumer, 0, charging);

	info->can_charging = charging;

}

enum hrtimer_restart charger_kthread_hrtimer_func(struct hrtimer *timer)
{
	struct charger_manager *info = container_of(timer, struct charger_manager, charger_kthread_timer);

	_wake_up_charger(info);
	return HRTIMER_NORESTART;
}

int charger_kthread_fgtimer_func(struct fgtimer *data)
{
	struct charger_manager *info = container_of(data, struct charger_manager, charger_kthread_fgtimer);

	_wake_up_charger(info);
	return 0;
}

static void mtk_charger_init_timer(struct charger_manager *info)
{
	if (IS_ENABLED(USE_FG_TIMER)) {
		fgtimer_init(&info->charger_kthread_fgtimer, &info->pdev->dev, "charger_thread");
		info->charger_kthread_fgtimer.callback = charger_kthread_fgtimer_func;
		fgtimer_start(&info->charger_kthread_fgtimer, info->polling_interval);
	} else {
		ktime_t ktime = ktime_set(info->polling_interval, 0);

		hrtimer_init(&info->charger_kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		info->charger_kthread_timer.function = charger_kthread_hrtimer_func;
		hrtimer_start(&info->charger_kthread_timer, ktime, HRTIMER_MODE_REL);
	}
}

static void mtk_charger_start_timer(struct charger_manager *info)
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

void mtk_charger_stop_timer(struct charger_manager *info)
{
	if (IS_ENABLED(USE_FG_TIMER)) {
		fgtimer_stop(&info->charger_kthread_fgtimer);
	}
}

static int charger_routine_thread(void *arg)
{
	struct charger_manager *info = arg;
	static int i;
	unsigned long flags;
	bool curr_sign, is_charger_on;

	while (1) {
		pr_err("charger_routine_thread [%s]\n", info->chg1_dev->props.alias_name);
		wait_event(info->wait_que, (info->charger_thread_timeout == true));

		mutex_lock(&info->charger_lock);
		info->charger_thread_timeout = false;
		i++;
		curr_sign = battery_get_bat_current_sign();
		pr_err("Vbat=%d,I=%d,VChr=%d,T=%d,Soc=%d:%d\n", battery_get_bat_voltage(),
			curr_sign ? battery_get_bat_current() :
					-1 * battery_get_bat_current(),
			battery_get_vbus(), battery_get_bat_temperature(),
			battery_get_bat_soc(), battery_get_bat_uisoc());

		is_charger_on = mtk_is_charger_on(info);

		charger_update_data(info);
		charger_check_status(info);

		if (is_charger_on == true) {
			if (info->do_algorithm)
				info->do_algorithm(info);
		}

		if (info->charger_thread_polling == true)
			mtk_charger_start_timer(info);

		spin_lock_irqsave(&info->slock, flags);
		wake_unlock(&info->charger_wakelock);
		spin_unlock_irqrestore(&info->slock, flags);

		mutex_unlock(&info->charger_lock);
	}

	return 0;
}

static int mtk_charger_parse_dt(struct charger_manager *info, struct device *dev)
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

#ifdef CONFIG_MTK_DUAL_CHARGER_SUPPORT
	if (strcmp(info->algorithm_name, "DualSwitchCharging") == 0) {
		pr_debug("found DualSwitchCharging\n");
		mtk_dual_switch_charging_init(info);
	}
#endif

	info->enable_sw_safety_timer = of_property_read_bool(np, "enable_sw_safety_timer");
	info->enable_sw_jeita = of_property_read_bool(np, "enable_sw_jeita");
	info->enable_pe_plus = of_property_read_bool(np, "enable_pe_plus");
	info->enable_pe_2 = of_property_read_bool(np, "enable_pe_2");
	info->enable_pe_3 = of_property_read_bool(np, "enable_pe_3");

	/* common */
	if (of_property_read_u32(np, "battery_cv", &val) >= 0) {
		info->data.battery_cv = val;
	} else {
		pr_err(
			"use default BATTERY_CV:%d\n",
			BATTERY_CV);
		info->data.battery_cv = BATTERY_CV;
	}

	if (of_property_read_u32(np, "max_charger_voltage", &val) >= 0) {
		info->data.max_charger_voltage = val;
	} else {
		pr_err(
			"use default V_CHARGER_MAX:%d\n",
			V_CHARGER_MAX);
		info->data.max_charger_voltage = V_CHARGER_MAX;
	}

	/* charging current */
	if (of_property_read_u32(np, "usb_charger_current_suspend", &val) >= 0) {
		info->data.usb_charger_current_suspend = val;
	} else {
		pr_err(
			"use default USB_CHARGER_CURRENT_SUSPEND:%d\n",
			USB_CHARGER_CURRENT_SUSPEND);
		info->data.usb_charger_current_suspend = USB_CHARGER_CURRENT_SUSPEND;
	}

	if (of_property_read_u32(np, "usb_charger_current_unconfigured", &val) >= 0) {
		info->data.usb_charger_current_unconfigured = val;
	} else {
		pr_err(
			"use default USB_CHARGER_CURRENT_UNCONFIGURED:%d\n",
			USB_CHARGER_CURRENT_UNCONFIGURED);
		info->data.usb_charger_current_unconfigured = USB_CHARGER_CURRENT_UNCONFIGURED;
	}

	if (of_property_read_u32(np, "usb_charger_current_configured", &val) >= 0) {
		info->data.usb_charger_current_configured = val;
	} else {
		pr_err(
			"use default USB_CHARGER_CURRENT_CONFIGURED:%d\n",
			USB_CHARGER_CURRENT_CONFIGURED);
		info->data.usb_charger_current_configured = USB_CHARGER_CURRENT_CONFIGURED;
	}

	if (of_property_read_u32(np, "usb_charger_current", &val) >= 0) {
		info->data.usb_charger_current = val;
	} else {
		pr_err(
			"use default USB_CHARGER_CURRENT:%d\n",
			USB_CHARGER_CURRENT);
		info->data.usb_charger_current = USB_CHARGER_CURRENT;
	}

	if (of_property_read_u32(np, "ac_charger_current", &val) >= 0) {
		info->data.ac_charger_current = val;
	} else {
		pr_err(
			"use default AC_CHARGER_CURRENT:%d\n",
			AC_CHARGER_CURRENT);
		info->data.ac_charger_current = AC_CHARGER_CURRENT;
	}

	if (of_property_read_u32(np, "ac_charger_input_current", &val) >= 0) {
		info->data.ac_charger_input_current = val;
	} else {
		pr_err(
			"use default AC_CHARGER_INPUT_CURRENT:%d\n",
			AC_CHARGER_INPUT_CURRENT);
		info->data.ac_charger_input_current = AC_CHARGER_INPUT_CURRENT;
	}

	if (of_property_read_u32(np, "non_std_ac_charger_current", &val) >= 0) {
		info->data.non_std_ac_charger_current = val;
	} else {
		pr_err(
			"use default NON_STD_AC_CHARGER_CURRENT:%d\n",
			NON_STD_AC_CHARGER_CURRENT);
		info->data.non_std_ac_charger_current = NON_STD_AC_CHARGER_CURRENT;
	}

	if (of_property_read_u32(np, "charging_host_charger_current", &val) >= 0) {
		info->data.charging_host_charger_current = val;
	} else {
		pr_err(
			"use default CHARGING_HOST_CHARGER_CURRENT:%d\n",
			CHARGING_HOST_CHARGER_CURRENT);
		info->data.charging_host_charger_current = CHARGING_HOST_CHARGER_CURRENT;
	}

	if (of_property_read_u32(np, "ta_ac_charger_current", &val) >= 0) {
		info->data.ta_ac_charger_current = val;
	} else {
		pr_err(
			"use default TA_AC_CHARGING_CURRENT:%d\n",
			TA_AC_CHARGING_CURRENT);
		info->data.ta_ac_charger_current = TA_AC_CHARGING_CURRENT;
	}

	/* sw jeita */
	if (of_property_read_u32(np, "jeita_temp_above_t4_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_above_t4_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_ABOVE_T4_CV_VOLTAGE:%d\n", JEITA_TEMP_ABOVE_T4_CV_VOLTAGE);
		info->data.jeita_temp_above_t4_cv_voltage = JEITA_TEMP_ABOVE_T4_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "jeita_temp_t3_to_t4_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_t3_to_t4_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_T3_TO_T4_CV_VOLTAGE:%d\n", JEITA_TEMP_T3_TO_T4_CV_VOLTAGE);
		info->data.jeita_temp_t3_to_t4_cv_voltage = JEITA_TEMP_T3_TO_T4_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "jeita_temp_t2_to_t3_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_t2_to_t3_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_T2_TO_T3_CV_VOLTAGE:%d\n", JEITA_TEMP_T2_TO_T3_CV_VOLTAGE);
		info->data.jeita_temp_t2_to_t3_cv_voltage = JEITA_TEMP_T2_TO_T3_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "jeita_temp_t1_to_t2_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_t1_to_t2_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_T1_TO_T2_CV_VOLTAGE:%d\n", JEITA_TEMP_T1_TO_T2_CV_VOLTAGE);
		info->data.jeita_temp_t1_to_t2_cv_voltage = JEITA_TEMP_T1_TO_T2_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "jeita_temp_t0_to_t1_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_t0_to_t1_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_T0_TO_T1_CV_VOLTAGE:%d\n", JEITA_TEMP_T0_TO_T1_CV_VOLTAGE);
		info->data.jeita_temp_t0_to_t1_cv_voltage = JEITA_TEMP_T0_TO_T1_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "jeita_temp_below_t0_cv_voltage", &val) >= 0) {
		info->data.jeita_temp_below_t0_cv_voltage = val;
	} else {
		pr_err(
			"use default JEITA_TEMP_BELOW_T0_CV_VOLTAGE:%d\n", JEITA_TEMP_BELOW_T0_CV_VOLTAGE);
		info->data.jeita_temp_below_t0_cv_voltage = JEITA_TEMP_BELOW_T0_CV_VOLTAGE;
	}

	if (of_property_read_u32(np, "temp_t4_threshold", &val) >= 0) {
		info->data.temp_t4_threshold = val;
	} else {
		pr_err(
			"use default TEMP_T4_THRESHOLD:%d\n", TEMP_T4_THRESHOLD);
		info->data.temp_t4_threshold = TEMP_T4_THRESHOLD;
	}

	if (of_property_read_u32(np, "temp_t4_thres_minus_x_degree", &val) >= 0) {
		info->data.temp_t4_thres_minus_x_degree = val;
	} else {
		pr_err(
			"use default TEMP_T4_THRES_MINUS_X_DEGREE:%d\n", TEMP_T4_THRES_MINUS_X_DEGREE);
		info->data.temp_t4_thres_minus_x_degree = TEMP_T4_THRES_MINUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "temp_t3_threshold", &val) >= 0) {
		info->data.temp_t3_threshold = val;
	} else {
		pr_err(
			"use default TEMP_T3_THRESHOLD:%d\n", TEMP_T3_THRESHOLD);
		info->data.temp_t3_threshold = TEMP_T3_THRESHOLD;
	}

	if (of_property_read_u32(np, "temp_t3_thres_minus_x_degree", &val) >= 0) {
		info->data.temp_t3_thres_minus_x_degree = val;
	} else {
		pr_err(
			"use default TEMP_T3_THRES_MINUS_X_DEGREE:%d\n", TEMP_T3_THRES_MINUS_X_DEGREE);
		info->data.temp_t3_threshold = TEMP_T3_THRES_MINUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "temp_t2_threshold", &val) >= 0) {
		info->data.temp_t2_threshold = val;
	} else {
		pr_err(
			"use default TEMP_T2_THRESHOLD:%d\n", TEMP_T2_THRESHOLD);
		info->data.temp_t2_threshold = TEMP_T2_THRESHOLD;
	}

	if (of_property_read_u32(np, "temp_t2_thres_plus_x_degree", &val) >= 0) {
		info->data.temp_t2_thres_plus_x_degree = val;
	} else {
		pr_err(
			"use default TEMP_T2_THRES_PLUS_X_DEGREE:%d\n", TEMP_T2_THRES_PLUS_X_DEGREE);
		info->data.temp_t2_thres_plus_x_degree = TEMP_T2_THRES_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "temp_t1_threshold", &val) >= 0) {
		info->data.temp_t1_threshold = val;
	} else {
		pr_err(
			"use default TEMP_T1_THRESHOLD:%d\n", TEMP_T1_THRESHOLD);
		info->data.temp_t1_threshold = TEMP_T1_THRESHOLD;
	}

	if (of_property_read_u32(np, "temp_t1_thres_plus_x_degree", &val) >= 0) {
		info->data.temp_t1_thres_plus_x_degree = val;
	} else {
		pr_err(
			"use default TEMP_T1_THRES_PLUS_X_DEGREE:%d\n", TEMP_T1_THRES_PLUS_X_DEGREE);
		info->data.temp_t1_thres_plus_x_degree = TEMP_T1_THRES_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "temp_t0_threshold", &val) >= 0) {
		info->data.temp_t0_threshold = val;
	} else {
		pr_err(
			"use default TEMP_T0_THRESHOLD:%d\n", TEMP_T0_THRESHOLD);
		info->data.temp_t0_threshold = TEMP_T0_THRESHOLD;
	}

	if (of_property_read_u32(np, "temp_t0_thres_plus_x_degree", &val) >= 0) {
		info->data.temp_t0_thres_plus_x_degree = val;
	} else {
		pr_err(
			"use default TEMP_T0_THRES_PLUS_X_DEGREE:%d\n", TEMP_T0_THRES_PLUS_X_DEGREE);
		info->data.temp_t0_thres_plus_x_degree = TEMP_T0_THRES_PLUS_X_DEGREE;
	}

	if (of_property_read_u32(np, "temp_neg_10_threshold", &val) >= 0) {
		info->data.temp_neg_10_threshold = val;
	} else {
		pr_err(
			"use default TEMP_NEG_10_THRESHOLD:%d\n", TEMP_NEG_10_THRESHOLD);
		info->data.temp_neg_10_threshold = TEMP_NEG_10_THRESHOLD;
	}

	/* battery temperature protection */
	info->thermal.sm = BAT_TEMP_NORMAL;
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

	/* PE 2.0 */
	if (of_property_read_u32(np, "pe20_ichg_level_threshold", &val) >= 0) {
		info->data.pe20_ichg_level_threshold = val;
	} else {
		pr_err(
			"use default PE20_ICHG_LEAVE_THRESHOLD:%d\n",
			PE20_ICHG_LEAVE_THRESHOLD);
		info->data.pe20_ichg_level_threshold = PE20_ICHG_LEAVE_THRESHOLD;
	}

	if (of_property_read_u32(np, "ta_start_battery_soc", &val) >= 0) {
		info->data.ta_start_battery_soc = val;
	} else {
		pr_err(
			"use default TA_START_BATTERY_SOC:%d\n",
			TA_START_BATTERY_SOC);
		info->data.ta_start_battery_soc = TA_START_BATTERY_SOC;
	}

	if (of_property_read_u32(np, "ta_stop_battery_soc", &val) >= 0) {
		info->data.ta_stop_battery_soc = val;
	} else {
		pr_err(
			"use default TA_STOP_BATTERY_SOC:%d\n",
			TA_STOP_BATTERY_SOC);
		info->data.ta_stop_battery_soc = TA_STOP_BATTERY_SOC;
	}

	/* PE 3.0 */
	if (of_property_read_u32(np, "cv_limit", &val) >= 0) {
		info->data.cv_limit = val;
	} else {
		pr_err(
			"use default CV_LIMIT:%d\n",
			CV_LIMIT);
		info->data.cv_limit = CV_LIMIT;
	}

	if (of_property_read_u32(np, "bat_upper_bound", &val) >= 0) {
		info->data.bat_upper_bound = val;
	} else {
		pr_err(
			"use default BAT_UPPER_BOUND:%d\n",
			BAT_UPPER_BOUND);
		info->data.bat_upper_bound = BAT_UPPER_BOUND;
	}

	if (of_property_read_u32(np, "bat_lower_bound", &val) >= 0) {
		info->data.bat_lower_bound = val;
	} else {
		pr_err(
			"use default BAT_LOWER_BOUND:%d\n",
			BAT_LOWER_BOUND);
		info->data.bat_lower_bound = BAT_LOWER_BOUND;
	}

	if (of_property_read_u32(np, "cc_ss_init", &val) >= 0) {
		info->data.cc_ss_init = val;
	} else {
		pr_err(
			"use default CC_SS_INIT:%d\n",
			CC_SS_INIT);
		info->data.cc_ss_init = CC_SS_INIT;
	}

	if (of_property_read_u32(np, "cc_init", &val) >= 0) {
		info->data.cc_init = val;
	} else {
		pr_err(
			"use default CC_INIT:%d\n",
			CC_INIT);
		info->data.cc_init = CC_INIT;
	}

	if (of_property_read_u32(np, "cc_init_bad_cable1", &val) >= 0) {
		info->data.cc_init_bad_cable1 = val;
	} else {
		pr_err(
			"use default CC_INIT_BAD_CABLE1:%d\n",
			CC_INIT_BAD_CABLE1);
		info->data.cc_init_bad_cable1 = CC_INIT_BAD_CABLE1;
	}

	if (of_property_read_u32(np, "cc_init_bad_cable2", &val) >= 0) {
		info->data.cc_init_bad_cable2 = val;
	} else {
		pr_err(
			"use default CC_INIT_BAD_CABLE2:%d\n",
			CC_INIT_BAD_CABLE2);
		info->data.cc_init_bad_cable2 = CC_INIT_BAD_CABLE2;
	}

	if (of_property_read_u32(np, "cc_init_r", &val) >= 0) {
		info->data.bat_lower_bound = val;
	} else {
		pr_err(
			"use default CC_INIT_R:%d\n",
			CC_INIT_R);
		info->data.bat_lower_bound = CC_INIT_R;
	}

	if (of_property_read_u32(np, "cc_init_bad_cable1_r", &val) >= 0) {
		info->data.cc_init_bad_cable1_r = val;
	} else {
		pr_err(
			"use default CC_INIT_BAD_CABLE1_R:%d\n",
			CC_INIT_BAD_CABLE1_R);
		info->data.cc_init_bad_cable1_r = CC_INIT_BAD_CABLE1_R;
	}

	if (of_property_read_u32(np, "cc_init_bad_cable2_r", &val) >= 0) {
		info->data.cc_init_bad_cable2_r = val;
	} else {
		pr_err(
			"use default CC_INIT_BAD_CABLE2_R:%d\n",
			CC_INIT_BAD_CABLE2_R);
		info->data.cc_init_bad_cable2_r = CC_INIT_BAD_CABLE2_R;
	}

#ifdef CONFIG_MTK_DUAL_CHARGER_SUPPORT
	/* dual charger */
	if (of_property_read_u32(np, "chg1_ta_ac_charger_current", &val) >= 0) {
		info->data.chg1_ta_ac_charger_current = val;
	} else {
		pr_err(
			"use default TA_AC_MASTER_CHARGING_CURRENT:%d\n",
			TA_AC_MASTER_CHARGING_CURRENT);
		info->data.chg1_ta_ac_charger_current = TA_AC_MASTER_CHARGING_CURRENT;
	}

	if (of_property_read_u32(np, "chg2_ta_ac_charger_current", &val) >= 0) {
		info->data.chg2_ta_ac_charger_current = val;
	} else {
		pr_err(
			"use default TA_AC_SLAVE_CHARGING_CURRENT:%d\n",
			TA_AC_SLAVE_CHARGING_CURRENT);
		info->data.chg2_ta_ac_charger_current = TA_AC_SLAVE_CHARGING_CURRENT;
	}
#endif

	/* cable measurement impedance */
	if (of_property_read_u32(np, "cable_imp_threshold", &val) >= 0) {
		info->data.cable_imp_threshold = val;
	} else {
		pr_err(
			"use default CABLE_IMP_THRESHOLD:%d\n",
			CABLE_IMP_THRESHOLD);
		info->data.cable_imp_threshold = CABLE_IMP_THRESHOLD;
	}

	if (of_property_read_u32(np, "vbat_cable_imp_threshold", &val) >= 0) {
		info->data.vbat_cable_imp_threshold = val;
	} else {
		pr_err(
			"use default VBAT_CABLE_IMP_THRESHOLD:%d\n",
			VBAT_CABLE_IMP_THRESHOLD);
		info->data.vbat_cable_imp_threshold = VBAT_CABLE_IMP_THRESHOLD;
	}

	/* bif */
	if (of_property_read_u32(np, "bif_threshold1", &val) >= 0) {
		info->data.bif_threshold1 = val;
	} else {
		pr_err(
			"use default BIF_THRESHOLD1:%d\n",
			BIF_THRESHOLD1);
		info->data.bif_threshold1 = BIF_THRESHOLD1;
	}

	if (of_property_read_u32(np, "bif_threshold2", &val) >= 0) {
		info->data.bif_threshold2 = val;
	} else {
		pr_err(
			"use default BIF_THRESHOLD2:%d\n",
			BIF_THRESHOLD2);
		info->data.bif_threshold2 = BIF_THRESHOLD2;
	}

	if (of_property_read_u32(np, "bif_cv_under_threshold2", &val) >= 0) {
		info->data.bif_cv_under_threshold2 = val;
	} else {
		pr_err(
			"use default BIF_CV_UNDER_THRESHOLD2:%d\n",
			BIF_CV_UNDER_THRESHOLD2);
		info->data.bif_cv_under_threshold2 = BIF_CV_UNDER_THRESHOLD2;
	}

	info->data.usb_charger_current_suspend = USB_CHARGER_CURRENT_SUSPEND;
	info->data.usb_charger_current_unconfigured = USB_CHARGER_CURRENT_UNCONFIGURED;
	info->data.usb_charger_current_configured = USB_CHARGER_CURRENT_CONFIGURED;
	info->data.usb_charger_current = USB_CHARGER_CURRENT;
	info->data.ac_charger_current = AC_CHARGER_CURRENT;
	info->data.ac_charger_input_current = AC_CHARGER_INPUT_CURRENT;
	info->data.non_std_ac_charger_current = NON_STD_AC_CHARGER_CURRENT;
	info->data.charging_host_charger_current = CHARGING_HOST_CHARGER_CURRENT;
	info->data.ta_ac_charger_current = TA_AC_CHARGING_CURRENT;
	info->data.max_charger_voltage = V_CHARGER_MAX;
	info->data.pe20_ichg_level_threshold = PE20_ICHG_LEAVE_THRESHOLD;
	info->data.ta_start_battery_soc = TA_START_BATTERY_SOC;
	info->data.ta_stop_battery_soc = TA_STOP_BATTERY_SOC;


	pr_err("algorithm name:%s\n", info->algorithm_name);

	return 0;
}


static ssize_t show_Pump_Express(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;
	int is_ta_detected = 0;

	pr_debug("[%s] chr_type:%d UISOC:%d startsoc:%d stopsoc:%d\n", __func__,
		mt_get_charger_type(), get_ui_soc(),
		pinfo->data.ta_start_battery_soc,
		pinfo->data.ta_stop_battery_soc);

	if (IS_ENABLED(CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT)) {
		/* Is PE+20 connect */
		if (mtk_pe20_get_is_connect(pinfo))
			is_ta_detected = 1;
		pr_debug("%s: pe20_is_connect = %d\n",
			__func__, mtk_pe20_get_is_connect(pinfo));
#if 0 /* TODO: Add PE+ */
		/* Is PE+ connect */
		if (mtk_pe_get_is_connect(pinfo))
			is_ta_detected = 1;
		pr_err("%s: pe_is_connect = %d\n",
			__func__, mtk_pep_get_is_connect(pinfo));
#endif
	}

	if (mtk_is_TA_support_pe30(pinfo) == true)
		is_ta_detected = 1;

	pr_debug("%s: detected = %d\n", __func__, is_ta_detected);

	return sprintf(buf, "%u\n", is_ta_detected);
}

static DEVICE_ATTR(Pump_Express, 0444, show_Pump_Express, NULL);

static ssize_t show_BatteryNotify(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;

	pr_debug("[Battery] show_BatteryNotify: %x\n", pinfo->notify_code);

	return sprintf(buf, "%u\n", pinfo->notify_code);
}

static ssize_t store_BatteryNotify(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct charger_manager *pinfo = dev->driver_data;
	unsigned int reg = 0;
	int ret;

	pr_debug("[Battery] store_BatteryNotify\n");
	if (buf != NULL && size != 0) {
		pr_debug("[Battery] buf is %s and size is %Zu\n", buf, size);
		ret = kstrtouint(buf, 16, &reg);
		pinfo->notify_code = reg;
		pr_debug("[Battery] store code: %x\n", pinfo->notify_code);
	}
	return size;
}

static DEVICE_ATTR(BatteryNotify, 0664, show_BatteryNotify, store_BatteryNotify);

static ssize_t show_BN_TestMode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct charger_manager *pinfo = dev->driver_data;

	pr_debug("[Battery] show_BN_TestMode : %x\n", pinfo->notify_test_mode);
	return sprintf(buf, "%u\n", pinfo->notify_test_mode);
}

static ssize_t store_BN_TestMode(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct charger_manager *pinfo = dev->driver_data;
	unsigned int reg = 0;
	int ret;

	pr_debug("[Battery] store_BN_TestMode\n");
	if (buf != NULL && size != 0) {
		pr_debug("[Battery] buf is %s and size is %Zu\n", buf, size);
		ret = kstrtouint(buf, 16, &reg);
		pinfo->notify_test_mode = reg;
		pr_debug("[Battery] store mode: %x\n", pinfo->notify_test_mode);
	}
	return size;
}
static DEVICE_ATTR(BN_TestMode, 0664, show_BN_TestMode, store_BN_TestMode);

static ssize_t show_ADC_Charger_Voltage(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	int vbus = battery_get_vbus();

	pr_debug("[%s]: %d\n", __func__, vbus);
	return sprintf(buf, "%d\n", vbus);
}

static DEVICE_ATTR(ADC_Charger_Voltage, 0444, show_ADC_Charger_Voltage, NULL);

/* procfs */
static int mtk_charger_current_cmd_show(struct seq_file *m, void *data)
{
	struct charger_manager *pinfo = m->private;

	seq_printf(m, "%d %d\n", pinfo->usb_unlimited, pinfo->cmd_discharging);
	return 0;
}

static ssize_t mtk_charger_current_cmd_write(struct file *file, const char *buffer,
						size_t count, loff_t *data)
{
	int len = 0;
	char desc[32];
	int cmd_current_unlimited = 0;
	int cmd_discharging = 0;
	struct charger_manager *info = PDE_DATA(file_inode(file));

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %d", &cmd_current_unlimited, &cmd_discharging) == 2) {
		info->usb_unlimited = cmd_current_unlimited;
		if (cmd_discharging == 1) {
			info->cmd_discharging = true;
			charger_dev_enable(info->chg1_dev, false);
			charger_manager_notifier(info, CHARGER_NOTIFY_STOP_CHARGING);
		} else if (cmd_discharging == 0) {
			info->cmd_discharging = false;
			charger_dev_enable(info->chg1_dev, true);
			charger_manager_notifier(info, CHARGER_NOTIFY_START_CHARGING);
		}

		pr_debug("%s cmd_current_unlimited=%d, cmd_discharging=%d\n",
			__func__, cmd_current_unlimited, cmd_discharging);
		return count;
	}

	pr_err("bad argument, echo [usb_unlimited] [disable] > current_cmd\n");
	return count;
}

static int mtk_charger_en_power_path_show(struct seq_file *m, void *data)
{
	struct charger_manager *pinfo = m->private;
	bool power_path_en = true;

	charger_dev_is_powerpath_enabled(pinfo->chg1_dev, &power_path_en);
	seq_printf(m, "%d\n", power_path_en);

	return 0;
}

static ssize_t mtk_charger_en_power_path_write(struct file *file, const char *buffer,
	size_t count, loff_t *data)
{
	int len = 0, ret = 0;
	char desc[32];
	unsigned int enable = 0;
	struct charger_manager *info = PDE_DATA(file_inode(file));


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	ret = kstrtou32(desc, 10, &enable);
	if (ret == 0) {
		charger_dev_enable_powerpath(info->chg1_dev, enable);
		pr_debug("%s: enable power path = %d\n", __func__, enable);
		return count;
	}

	pr_err("bad argument, echo [enable] > en_power_path\n");
	return count;
}

static int mtk_charger_en_safety_timer_show(struct seq_file *m, void *data)
{
	struct charger_manager *pinfo = m->private;
	bool safety_timer_en;

	charger_dev_is_safety_timer_enabled(pinfo->chg1_dev, &safety_timer_en);
	seq_printf(m, "%d\n", safety_timer_en);

	return 0;
}

static ssize_t mtk_charger_en_safety_timer_write(struct file *file, const char *buffer,
	size_t count, loff_t *data)
{
	int len = 0, ret = 0;
	char desc[32];
	unsigned int enable = 0;
	struct charger_manager *info = PDE_DATA(file_inode(file));

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	ret = kstrtou32(desc, 10, &enable);
	if (ret == 0) {
		charger_dev_enable_safety_timer(info->chg1_dev, enable);
		pr_debug("%s: enable safety timer = %d\n", __func__, enable);
		return count;
	}

	pr_err("bad argument, echo [enable] > en_safety_timer\n");
	return count;
}

/* PROC_FOPS_RW(battery_cmd); */
/* PROC_FOPS_RW(discharging_cmd); */
PROC_FOPS_RW(current_cmd);
PROC_FOPS_RW(en_power_path);
PROC_FOPS_RW(en_safety_timer);

/* Create sysfs and procfs attributes */
static int mtk_charger_setup_files(struct platform_device *pdev)
{
	int ret = 0;
	struct proc_dir_entry *battery_dir = NULL;
	struct charger_manager *info = platform_get_drvdata(pdev);
	/* struct charger_device *chg_dev; */

	ret = device_create_file(&(pdev->dev), &dev_attr_sw_jeita);
	if (ret)
		goto _out;
	ret = device_create_file(&(pdev->dev), &dev_attr_pe20);
	if (ret)
		goto _out;
	ret = device_create_file(&(pdev->dev), &dev_attr_pe30);
	if (ret)
		goto _out;
	/* Battery warning */
	ret = device_create_file(&(pdev->dev), &dev_attr_BatteryNotify);
	if (ret)
		goto _out;
	ret = device_create_file(&(pdev->dev), &dev_attr_BN_TestMode);
	if (ret)
		goto _out;
	/* Pump express */
	ret = device_create_file(&(pdev->dev), &dev_attr_Pump_Express);
	if (ret)
		goto _out;

	ret = device_create_file(&(pdev->dev), &dev_attr_ADC_Charger_Voltage);
	if (ret)
		goto _out;

	battery_dir = proc_mkdir("mtk_battery_cmd", NULL);
	if (!battery_dir) {
		pr_err("[%s]: mkdir /proc/mtk_battery_cmd failed\n", __func__);
		return -ENOMEM;
	}

#if 0
	proc_create_data("battery_cmd", S_IRUGO | S_IWUSR, battery_dir,
			&battery_cmd_proc_fops, info);
	proc_create_data("discharging_cmd", S_IRUGO | S_IWUSR, battery_dir,
			&discharging_cmd_proc_fops, info);
#endif
	proc_create_data("current_cmd", S_IRUGO | S_IWUSR, battery_dir,
			&mtk_charger_current_cmd_fops, info);
	proc_create_data("en_power_path", S_IRUGO | S_IWUSR, battery_dir,
			&mtk_charger_en_power_path_fops, info);
	proc_create_data("en_safety_timer", S_IRUGO | S_IWUSR, battery_dir,
			&mtk_charger_en_safety_timer_fops, info);

_out:
	return ret;
}

static int mtk_charger_probe(struct platform_device *pdev)
{
	struct charger_manager *info = NULL;
	struct list_head *pos;
	struct list_head *phead = &consumer_head;
	struct charger_consumer *ptr;
	int ret;

	pr_err("%s: starts\n", __func__);

	info = devm_kzalloc(&pdev->dev, sizeof(struct charger_manager), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	pinfo = info;

	platform_set_drvdata(pdev, info);
	info->pdev = pdev;
	mtk_charger_parse_dt(info, &pdev->dev);

	mutex_init(&info->charger_lock);
	wake_lock_init(&info->charger_wakelock, WAKE_LOCK_SUSPEND, "charger suspend wakelock");
	spin_lock_init(&info->slock);

	/* init thread */
	init_waitqueue_head(&info->wait_que);
	info->polling_interval = 10;
	info->enable_dynamic_cv = true;

	mtk_charger_init_timer(info);

	kthread_run(charger_routine_thread, info, "charger_thread");

	if (info->chg1_dev != NULL && info->do_event != NULL) {
		info->chg1_nb.notifier_call = info->do_event;
		register_charger_device_notifier(info->chg1_dev, &info->chg1_nb);
		charger_dev_set_drvdata(info->chg1_dev, info);
	}

	info->psy_nb.notifier_call = charger_psy_event;
	power_supply_reg_notifier(&info->psy_nb);

	srcu_init_notifier_head(&info->evt_nh);
	ret = mtk_charger_setup_files(pdev);
	if (ret)
		pr_err("Error creating sysfs interface\n");

	if (mtk_pe20_init(info) < 0)
		info->enable_pe_2 = false;

	if (mtk_pe30_init(info) == false)
		info->enable_pe_3 = false;

	mutex_lock(&consumer_mutex);
	list_for_each(pos, phead) {
		ptr = container_of(pos, struct charger_consumer, list);
		ptr->cm = info;
		if (ptr->pnb != NULL) {
			srcu_notifier_chain_register(&info->evt_nh, ptr->pnb);
			ptr->pnb = NULL;
		}
	}
	mutex_unlock(&consumer_mutex);
	info->chg1_consumer = charger_manager_get_by_name(&pdev->dev, "charger1");

	return 0;
}

static int mtk_charger_remove(struct platform_device *dev)
{
	return 0;
}

static void mtk_charger_shutdown(struct platform_device *dev)
{
	struct charger_manager *info = platform_get_drvdata(dev);

	if (mtk_pe20_get_is_connect(info)) {
		charger_dev_set_input_current(info->chg1_dev, 100000);
		charger_dev_set_mivr(info->chg1_dev, 4500000);
		pr_debug("%s: reset TA before shutdown\n", __func__);
	}
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

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
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
/* necessary header */
#include "moto_wlc.h"
#include "charger_class.h"

/* dependent on platform */
#include "mtk_charger.h"

struct wlc_hal {
	struct charger_device *chg1_dev;
	struct charger_device *chg2_dev;
};

int wlc_hal_init_hardware(struct chg_alg_device *alg)
{
	struct mtk_wlc *wlc;
	struct wlc_hal *hal;

	wlc_dbg("%s\n", __func__);
	if (alg == NULL) {
		wlc_err("%s: alg is null\n", __func__);
		return -EINVAL;
	}

	wlc = dev_get_drvdata(&alg->dev);
	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (hal == NULL) {
		hal = devm_kzalloc(&wlc->pdev->dev, sizeof(*hal), GFP_KERNEL);
		if (!hal)
			return -ENOMEM;
		chg_alg_dev_set_drv_hal_data(alg, hal);
	}

	hal->chg1_dev = get_charger_by_name("primary_chg");
	if (hal->chg1_dev)
		wlc_dbg("%s: Found primary charger\n", __func__);
	else {
		wlc_err("%s: Error : can't find primary charger\n",
			__func__);
		return -ENODEV;
	}

	hal->chg2_dev = get_charger_by_name("secondary_chg");
	if (hal->chg2_dev)
		wlc_dbg("%s: Found secondary charger\n", __func__);
	else
		wlc_err("%s: Error : can't find secondary charger\n",
			__func__);

	return 0;
}

int wlc_hal_get_boot_mode(struct chg_alg_device *alg)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		ret = -EINVAL;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		if (info == NULL)
			ret = -EINVAL;
		else
			ret = info->bootmode;
	}

	wlc_dbg("%s boot mode:%d\n", __func__, ret);
	return ret;
}

int wlc_hal_get_uisoc(struct chg_alg_device *alg)
{
	union power_supply_propval prop;
	struct power_supply *bat_psy = NULL;
	int ret;
	struct mtk_wlc *wlc;

	if (alg == NULL)
		return -EINVAL;

	wlc = dev_get_drvdata(&alg->dev);
	bat_psy = wlc->bat_psy;

	if (IS_ERR_OR_NULL(bat_psy)) {
		pr_notice("%s retry to get wlc->bat_psy\n", __func__);
		bat_psy = devm_power_supply_get_by_phandle(&wlc->pdev->dev, "gauge");
		wlc->bat_psy = bat_psy;
	}

	if (IS_ERR_OR_NULL(bat_psy)) {
		pr_notice("%s Couldn't get bat_psy\n", __func__);
		ret = 50;
	} else {
		ret = power_supply_get_property(bat_psy,
			POWER_SUPPLY_PROP_CAPACITY, &prop);
		ret = prop.intval;
	}

	wlc_dbg("%s:%d\n", __func__,
		ret);
	return ret;
}

int wlc_hal_get_charger_type(struct chg_alg_device *alg)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		ret = -EINVAL;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		if (info == NULL)
			ret = -EINVAL;
		else
			ret = info->chr_type;
	}

	wlc_dbg("%s type:%d\n", __func__, ret);
	return info->chr_type;
}

int wlc_hal_get_batt_temp(struct chg_alg_device *alg)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		ret = -EINVAL;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		if (info == NULL)
			ret = -EINVAL;
		else
			ret = info->battery_temp;
	}

	wlc_dbg("%s battery_temp:%d\n", __func__, ret);
	return info->battery_temp;
}

int wlc_hal_get_batt_cv(struct chg_alg_device *alg)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		ret = -EINVAL;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		if (info == NULL)
			ret = -EINVAL;
		else
			ret = info->data.battery_cv;
	}

	wlc_dbg("%s battery_cv:%d\n", __func__, ret);
	return info->data.battery_cv;
}

int wlc_hal_set_mivr(struct chg_alg_device *alg, enum chg_idx chgidx, int uV)
{
	int ret = 0;
	struct mtk_wlc *wlc;
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	wlc = dev_get_drvdata(&alg->dev);
	hal = chg_alg_dev_get_drv_hal_data(alg);

	ret = charger_dev_set_mivr(hal->chg1_dev, uV);
	if (ret < 0)
		wlc_err("%s: failed, ret = %d\n", __func__, ret);

	return ret;
}

int wlc_hal_get_charger_cnt(struct chg_alg_device *alg)
{
	struct wlc_hal *hal;
	int cnt = 0;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (hal->chg1_dev != NULL)
		cnt++;
	if (hal->chg2_dev != NULL)
		cnt++;

	return cnt;
}

bool wlc_hal_is_chip_enable(struct chg_alg_device *alg, enum chg_idx chgidx)
{
	struct wlc_hal *hal;
	bool is_chip_enable = false;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1)
		charger_dev_is_chip_enabled(hal->chg1_dev,
		&is_chip_enable);
	else if (chgidx == CHG2)
		charger_dev_is_chip_enabled(hal->chg2_dev,
		&is_chip_enable);

	return is_chip_enable;
}

int wlc_hal_enable_charger(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool en)
{
	struct wlc_hal *hal;
	int ret;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1  && hal->chg1_dev != NULL)
		ret = charger_dev_enable(hal->chg1_dev, en);
	else if (chgidx == CHG2  && hal->chg2_dev != NULL)
		ret = charger_dev_enable(hal->chg2_dev, en);

	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, en);
	return 0;
}

int wlc_hal_is_charger_enable(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool *en)
{
	struct wlc_hal *hal;
	int ret;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1  && hal->chg1_dev != NULL)
		ret = charger_dev_is_enabled(hal->chg1_dev, en);
	else if (chgidx == CHG2  && hal->chg2_dev != NULL)
		ret = charger_dev_is_enabled(hal->chg2_dev, en);

	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, *en);
	return 0;
}

int wlc_hal_reset_ta(struct chg_alg_device *alg, enum chg_idx chgidx)
{
	struct wlc_hal *hal;
	int ret;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	ret = charger_dev_reset_ta(hal->chg1_dev);
	if (ret != 0) {
		wlc_err("%s: fail ,ret=%d\n", __func__, ret);
		return -1;
	}
	return 0;
}

static int get_pmic_vbus(int *vchr)
{
	union power_supply_propval prop;
	static struct power_supply *chg_psy;
	int ret;

	if (chg_psy == NULL)
		chg_psy = power_supply_get_by_name("mtk_charger_type");
	if (chg_psy == NULL || IS_ERR(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		ret = -1;
	} else {
		ret = power_supply_get_property(chg_psy,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	}
	*vchr = prop.intval * 1000;

	wlc_dbg("%s vbus:%d\n", __func__,
		prop.intval);
	return ret;
}

int wlc_hal_get_vbus(struct chg_alg_device *alg)
{
	int ret = 0;
	int vchr = 0;
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;
	hal = chg_alg_dev_get_drv_hal_data(alg);

	ret = charger_dev_get_vbus(hal->chg1_dev, &vchr);
	if (ret < 0) {
		ret = get_pmic_vbus(&vchr);
		if (ret < 0)
			wlc_err("%s: get vbus failed: %d\n", __func__, ret);
	}

	return vchr;
}

int wlc_hal_get_vbat(struct chg_alg_device *alg)
{
	union power_supply_propval prop;
	struct power_supply *bat_psy = NULL;
	int ret;
	struct mtk_wlc *wlc;

	if (alg == NULL)
		return -EINVAL;

	wlc = dev_get_drvdata(&alg->dev);

	bat_psy = wlc->bat_psy;

	if (bat_psy == NULL || IS_ERR(bat_psy)) {
		pr_notice("%s retry to get wlc->bat_psy\n", __func__);
		bat_psy = devm_power_supply_get_by_phandle(&wlc->pdev->dev, "gauge");
		wlc->bat_psy = bat_psy;
	}

	if (bat_psy == NULL || IS_ERR(bat_psy)) {
		pr_notice("%s Couldn't get bat_psy\n", __func__);
		ret = 3999;
	} else {
		ret = power_supply_get_property(bat_psy,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
		ret = prop.intval;
	}

	pr_debug("%s:%d\n", __func__,
		ret);
	return ret;
}

int wlc_hal_get_ibat(struct chg_alg_device *alg)
{
	union power_supply_propval prop;
	struct power_supply *bat_psy = NULL;
	int ret;
	struct mtk_wlc *wlc;

	if (alg == NULL)
		return -EINVAL;

	wlc = dev_get_drvdata(&alg->dev);
	bat_psy = wlc->bat_psy;

	if (bat_psy == NULL || IS_ERR(bat_psy)) {
		pr_notice("%s retry to get wlc->bat_psy\n", __func__);
		bat_psy = devm_power_supply_get_by_phandle(&wlc->pdev->dev, "gauge");
		wlc->bat_psy = bat_psy;
	}

	if (bat_psy == NULL || IS_ERR(bat_psy)) {
		pr_notice("%s Couldn't get bat_psy\n", __func__);
		ret = 0;
	} else {
		ret = power_supply_get_property(bat_psy,
			POWER_SUPPLY_PROP_CURRENT_NOW, &prop);
		ret = prop.intval;
	}

	pr_debug("%s:%d\n", __func__,
		ret);
	return ret;
}


int wlc_hal_enable_cable_drop_comp(struct chg_alg_device *alg,
	bool en)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	return charger_dev_enable_cable_drop_comp(hal->chg1_dev, false);
}

int wlc_hal_vbat_mon_en(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool en)
{

	struct wlc_hal *hal;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);

	ret = charger_dev_enable_6pin_battery_charging(
		hal->chg1_dev, en);

	wlc_err("%s en=%d ret=%d\n", __func__, en, ret);

	return ret;
}

int wlc_hal_set_cv(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 uv)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		charger_dev_set_constant_voltage(hal->chg1_dev,
								uv);
	else if (chgidx == CHG2 && hal->chg2_dev != NULL)
		charger_dev_set_constant_voltage(hal->chg2_dev,
								uv);
	return 0;
}

int wlc_hal_set_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 ua)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		charger_dev_set_charging_current(hal->chg1_dev, ua);
	else if (chgidx == CHG2 && hal->chg2_dev != NULL)
		charger_dev_set_charging_current(hal->chg2_dev, ua);
	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, ua);

	return 0;
}

int wlc_hal_get_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *ua)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		charger_dev_get_charging_current(hal->chg1_dev, ua);
	else if (chgidx == CHG2 && hal->chg2_dev != NULL)
		charger_dev_get_charging_current(hal->chg2_dev, ua);
	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, *ua);

	return 0;
}

int wlc_hal_set_input_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 ua)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		charger_dev_set_input_current(hal->chg1_dev, ua);
	else if (chgidx == CHG2 && hal->chg2_dev != NULL)
		charger_dev_set_input_current(hal->chg2_dev, ua);
	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, ua);

	return 0;
}

int wlc_hal_get_mivr_state(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool *in_loop)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		charger_dev_get_mivr_state(hal->chg1_dev, in_loop);
	else if (chgidx == CHG2 && hal->chg2_dev != NULL)
		charger_dev_get_mivr_state(hal->chg2_dev, in_loop);

	return 0;
}

int wlc_hal_send_ta20_current_pattern(struct chg_alg_device *alg,
					  u32 uV)
{
	struct wlc_hal *hal;

	if (alg == NULL)
		return -EINVAL;
	hal = chg_alg_dev_get_drv_hal_data(alg);
	return charger_dev_send_ta20_current_pattern(hal->chg1_dev,
			uV);
}

#if 0
int wlc_hal_enable_vbus_ovp(struct chg_alg_device *alg, bool enable)
{
	mtk_chg_enable_vbus_ovp(enable);

	return 0;
}
#endif

int wlc_hal_get_min_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *uA)
{
	struct wlc_hal *hal;
	int ret;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		ret = charger_dev_get_min_charging_current(hal->chg1_dev, uA);
	if (chgidx == CHG2 && hal->chg2_dev != NULL)
		ret = charger_dev_get_min_charging_current(hal->chg2_dev, uA);
	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, *uA);
	return 0;
}

int wlc_hal_get_min_input_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *uA)
{
	struct wlc_hal *hal;
	int ret;

	if (alg == NULL)
		return -EINVAL;

	hal = chg_alg_dev_get_drv_hal_data(alg);
	if (chgidx == CHG1 && hal->chg1_dev != NULL)
		ret = charger_dev_get_min_input_current(hal->chg1_dev, uA);
	if (chgidx == CHG2 && hal->chg2_dev != NULL)
		ret = charger_dev_get_min_input_current(hal->chg2_dev, uA);
	wlc_dbg("%s idx:%d %d\n", __func__, chgidx, *uA);
	return 0;
}


int wlc_hal_get_log_level(struct chg_alg_device *alg)
{
	struct mtk_charger *info = NULL;
	struct power_supply *chg_psy = NULL;
	int ret = 0;

	if (alg == NULL)
		return -EINVAL;

	chg_psy = power_supply_get_by_name("mtk-master-charger");
	if (IS_ERR_OR_NULL(chg_psy)) {
		wlc_err("%s Couldn't get chg_psy\n", __func__);
		return -1;
	} else {
		info = (struct mtk_charger *)power_supply_get_drvdata(chg_psy);
		if (IS_ERR_OR_NULL(info)) {
			wlc_err("%s info is NULL\n", __func__);
			return -1;
		}
		ret = info->log_level;
	}

	return ret;
}

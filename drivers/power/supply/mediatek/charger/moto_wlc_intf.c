/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 Motorola Inc.
*/

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mutex.h>

#include <upmu_common.h>
#include "mtk_charger_intf.h"
#include "mtk_charger_init.h"
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/thermal.h>

static int wlc_set_mivr(struct charger_manager *pinfo, int uV);
#define CHARGER_STATE_NUM 8
struct moto_wls_chg_ops *wls_chg_ops = NULL;
static int wlc_state_to_current_limit[CHARGER_STATE_NUM] = {
	-1, 2500000, 2000000, 1500000, 1200000, 1000000, 700000, 500000
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

/* Unit of the following functions are uV, uA */
static inline u32 wlc_get_vbus(void)
{
	return battery_get_vbus() * 1000;
}

static inline u32 wlc_get_vbat(void)
{
	return pmic_get_battery_voltage() * 1000;
}

static inline u32 wlc_get_ibat(void)
{
	return battery_get_bat_current() * 100;
}
static inline u32 wlc_get_soc(void)
{
	return battery_get_uisoc();
}
static bool cancel_wlc(struct charger_manager *pinfo)
{
//	if (mtk_pdc_check_charger(pinfo) || mtk_is_TA_support_pd_pps(pinfo))
//		return true;
	return false;
}

int mtk_wlc_reset_ta_vchr(struct charger_manager *pinfo)
{
	int ret = 0, chr_volt = 0;
	u32 retry_cnt = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;
//	bool chg2_chip_enabled = false;

	chr_info("%s: starts\n", __func__);

	wlc_set_mivr(pinfo, pinfo->wlc.min_charger_voltage);

	/* Reset TA's charging voltage */
	do {
/*
		if (pinfo->chg2_dev) {
			charger_dev_is_chip_enabled(pinfo->chg2_dev,
				&chg2_chip_enabled);
			if (chg2_chip_enabled)
				charger_dev_enable(pinfo->chg2_dev, false);
		}
*/
		ret = charger_dev_reset_ta(pinfo->chg1_dev);
		msleep(250);

		/* Check charger's voltage */
		chr_volt = wlc_get_vbus();
		if (abs(chr_volt - wlc->ta_vchr_org) <= 1000000) {
			wlc->vbus = chr_volt;
			wlc->idx = -1;
			wlc->is_connect = false;
			break;
		}

		retry_cnt++;
	} while (retry_cnt < 3);

	if (wlc->is_connect) {
		chr_err("%s: failed, ret = %d\n", __func__, ret);
		wlc_set_mivr(pinfo, wlc->min_charger_voltage);
		return ret;
	}

	/* Measure VBAT */
	wlc->vbat_orig = wlc_get_vbat();

	chr_info("%s: OK\n", __func__);

	return ret;
}

/* Enable/Disable HW & SW VBUS OVP */
static int wlc_enable_vbus_ovp(struct charger_manager *pinfo, bool enable)
{
	int ret = 0;
	return ret;

	/* Enable/Disable HW(PMIC) OVP */
	ret = pmic_enable_hw_vbus_ovp(enable);
	if (ret < 0) {
		chr_err("%s: failed, ret = %d\n", __func__, ret);
		return ret;
	}

	charger_enable_vbus_ovp(pinfo, enable);

	return ret;
}

static int wlc_set_mivr(struct charger_manager *pinfo, int uV)
{
	int ret = 0;
//	bool chg2_chip_enabled = false;

	chr_info("%s: starts,uV:%d\n", __func__, uV);

	ret = charger_dev_set_mivr(pinfo->chg1_dev, uV);
	if (ret < 0)
		chr_err("%s: failed, ret = %d\n", __func__, ret);

/*	if (pinfo->chg2_dev) {
		charger_dev_is_chip_enabled(pinfo->chg2_dev,
			&chg2_chip_enabled);
		if (chg2_chip_enabled) {
			ret = charger_dev_set_mivr(pinfo->chg2_dev,
				uV + pinfo->data.slave_mivr_diff);
			if (ret < 0)
				pr_info("%s: chg2 failed, ret = %d\n", __func__,
					ret);
		}
	}
*/
	return ret;
}

static int wlc_leave(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	chr_info("%s: starts\n", __func__);
	ret = mtk_wlc_reset_ta_vchr(pinfo);
	if (ret < 0 || wlc->is_connect) {
		chr_err("%s: failed, is_connect = %d, ret = %d\n",
			__func__, wlc->is_connect, ret);
		return ret;
	}

	wlc_enable_vbus_ovp(pinfo, true);
	wlc_set_mivr(pinfo, pinfo->data.min_charger_voltage);
	chr_info("%s: OK\n", __func__);
	return ret;
}

static int wlc_check_leave_status(struct charger_manager *pinfo)
{
	int ret = 0;
	int ichg = 0, vchr = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	chr_info("%s: starts\n", __func__);

	/* PE+ leaves unexpectedly */
	vchr = wlc_get_vbus();
	if (abs(vchr - wlc->ta_vchr_org) < 1000000) {
		chr_err("%s: WLC leave unexpectedly, recheck TA\n", __func__);
		wlc->to_check_chr_type = true;
		ret = wlc_leave(pinfo);
		if (ret < 0 || wlc->is_connect)
			goto _err;

		return ret;
	}

	ichg = wlc_get_ibat();

	/* Check SOC & Ichg */
	if (battery_get_soc() > pinfo->data.ta_stop_battery_soc &&
	    ichg > 0 && ichg < pinfo->data.wlc_ichg_level_threshold) {
		ret = wlc_leave(pinfo);
		if (ret < 0 || wlc->is_connect)
			goto _err;
		chr_info("%s: OK, SOC = (%d,%d), stop WLC\n", __func__,
			battery_get_soc(), pinfo->data.ta_stop_battery_soc);
	}

	return ret;

_err:
	chr_err("%s: failed, is_connect = %d, ret = %d\n",
		__func__, wlc->is_connect, ret);
	return ret;
}

static int __wlc_set_ta_vchr(struct charger_manager *pinfo, u32 chr_volt)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;
//	bool chg2_chip_enabled = false;

	chr_info("%s: starts, chr_volt:%d\n", __func__, chr_volt);

	/* Not to set chr volt if cable is plugged out */
	if (wlc->is_cable_out_occur) {
		chr_err("%s: failed, cable out\n", __func__);
		return -EIO;
	}
/*
	if (pinfo->chg2_dev) {
		charger_dev_is_chip_enabled(pinfo->chg2_dev,
			&chg2_chip_enabled);
		if (chg2_chip_enabled)
			charger_dev_enable(pinfo->chg2_dev, false);
	}
*/
//	ret = charger_dev_send_ta20_current_pattern(pinfo->chg1_dev, chr_volt);
//	if (ret < 0) {
//		chr_err("%s: failed, ret = %d\n", __func__, ret);
//		return ret;
//	}

//	chr_info("%s: OK\n", __func__);

	return ret;
}

static int wlc_set_ta_vchr(struct charger_manager *pinfo, u32 chr_volt)
{
	int ret = 0;
	int vchr_before, vchr_after, vchr_delta;
	const u32 sw_retry_cnt_max = 3;
	const u32 retry_cnt_max = 5;
	u32 sw_retry_cnt = 0, retry_cnt = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	chr_info("%s: starts, chr_volt:%d\n", __func__, chr_volt);

	do {
		vchr_before = wlc_get_vbus();
		ret = __wlc_set_ta_vchr(pinfo, chr_volt);
		vchr_after = wlc_get_vbus();

		vchr_delta = abs(vchr_after - chr_volt);

		/*
		 * It is successful if VBUS difference to target is
		 * less than 500mV.
		 */
		if (vchr_delta < 500000 && ret == 0) {
			chr_info("%s: OK, vchr = (%d, %d), vchr_target = %dmV\n",
				__func__, vchr_before / 1000, vchr_after / 1000,
				chr_volt / 1000);
			return ret;
		}

		if (ret == 0 || sw_retry_cnt >= sw_retry_cnt_max)
			retry_cnt++;
		else
			sw_retry_cnt++;

		wlc_set_mivr(pinfo, pinfo->wlc.min_charger_voltage);

		chr_err("%s: retry_cnt = (%d, %d), vchr = (%d, %d), vchr_target = %dmV\n",
			__func__, sw_retry_cnt, retry_cnt, vchr_before / 1000,
			vchr_after / 1000, chr_volt / 1000);

	} while (!wlc->is_cable_out_occur &&
		 mt_get_charger_type() != CHARGER_UNKNOWN &&
		 retry_cnt < retry_cnt_max && /*pinfo->enable_hv_charging &&*/
		 cancel_wlc(pinfo) != true);

	ret = -EIO;
	chr_err("%s: failed, vchr_org = %dmV, vchr_after = %dmV, target_vchr = %dmV\n",
		__func__, wlc->ta_vchr_org / 1000, vchr_after / 1000,
		chr_volt / 1000);

	return ret;
}

static void mtk_wlc_check_cable_impedance(struct charger_manager *pinfo)
{
	int ret = 0;
	int vchr1, vchr2, cable_imp;
	unsigned int aicr_value;
	bool mivr_state = false;
	struct timespec ptime[2], diff;
	struct mtk_wlc *wlc = &pinfo->wlc;
	return;
	chr_info("%s: starts\n", __func__);

	if (wlc->vbat_orig > pinfo->data.vbat_cable_imp_threshold) {
		chr_err("VBAT > %dmV, directly set aicr to %dmA\n",
			pinfo->data.vbat_cable_imp_threshold / 1000,
			pinfo->data.ac_charger_input_current / 1000);
		wlc->aicr_cable_imp = pinfo->data.ac_charger_input_current;
		goto end;
	}

	/* Disable cable drop compensation */
	charger_dev_enable_cable_drop_comp(pinfo->chg1_dev, false);

	get_monotonic_boottime(&ptime[0]);

	/* Set ichg = 2500mA, set MIVR */
	charger_dev_set_charging_current(pinfo->chg1_dev, 2500000);
	mdelay(240);
	wlc_set_mivr(pinfo, pinfo->wlc.min_charger_voltage);

	get_monotonic_boottime(&ptime[1]);
	diff = timespec_sub(ptime[1], ptime[0]);

	aicr_value = 800000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);

	/* To wait for soft-start */
	msleep(150);

	ret = charger_dev_get_mivr_state(pinfo->chg1_dev, &mivr_state);
	if (ret != -ENOTSUPP && mivr_state) {
		wlc->aicr_cable_imp = 1000000;
		goto end;
	}

	vchr1 = wlc_get_vbus();

	aicr_value = 500000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);
	msleep(20);

	vchr2 = wlc_get_vbus();

	/*
	 * Calculate cable impedance (|V1 - V2|) / (|I2 - I1|)
	 * m_ohm = (mv * 10 * 1000) / (mA * 10)
	 * m_ohm = (uV * 10) / (mA * 10)
	 */
	cable_imp = (abs(vchr1 - vchr2) * 10) / (7400 - 4625);

	chr_info("%s: cable_imp:%d mohm, vchr1:%d, vchr2:%d, time:%ld\n",
		__func__, cable_imp, vchr1 / 1000, vchr2 / 1000,
		diff.tv_nsec);

	/* Recover cable drop compensation */
	aicr_value = 100000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);
	msleep(250);

	if (cable_imp < pinfo->data.cable_imp_threshold) {
		wlc->aicr_cable_imp = pinfo->data.ac_charger_input_current;
		chr_info("Normal cable\n");
	} else {
		wlc->aicr_cable_imp = 1000000; /* uA */
		chr_info("Bad cable\n");
	}
	charger_dev_set_input_current(pinfo->chg1_dev, wlc->aicr_cable_imp);

	pr_info("%s: set aicr:%dmA, vbat:%dmV, mivr_state:%d\n",
		__func__, wlc->aicr_cable_imp / 1000,
		wlc->vbat_orig / 1000, mivr_state);
	return;

end:
	chr_err("%s not started: set aicr:%dmA, vbat:%dmV, mivr_state:%d\n",
		__func__, wlc->aicr_cable_imp / 1000,
		wlc->vbat_orig / 1000, mivr_state);
}

static int wlc_detect_ta(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	chr_info("%s: starts\n", __func__);
//	return ret;
//	wlc->ta_vchr_org = wlc_get_vbus();

	/* Disable OVP */
	ret = wlc_enable_vbus_ovp(pinfo, false);
	if (ret < 0)
		goto err;

//	if (abs(wlc->ta_vchr_org - 8500000) > 500000)
//		ret = wlc_set_ta_vchr(pinfo, 8500000);
//	else
//		ret = wlc_set_ta_vchr(pinfo, 6500000);

//	if (ret < 0) {
//		wlc->to_check_chr_type = false;
//		goto err;
//	}
	if(mtk_wlc_check_charger_avail() == true)
		wlc->is_connect = true;
	chr_info("%s: OK\n", __func__);

	return ret;
err:
	wlc->is_connect = false;
	wlc_enable_vbus_ovp(pinfo, true);
	chr_err("%s: failed, ret = %d\n", __func__, ret);
	return ret;
}

static int wlc_init_ta(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	wlc->is_connect = false;
	wlc->is_cable_out_occur = false;

	chr_info("%s: starts\n", __func__);

#ifdef WLC_HW_INIT /* need check */
	ret = battery_charging_control(CHARGING_CMD_INIT, NULL);
	if (ret < 0) {
		chr_err("%s: failed, ret = %d\n", __func__, ret);
		return ret;
	}
#endif /*wlc_HW_INIT*/

	chr_info("%s: OK\n", __func__);
	return ret;
}
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

	chr_info("%s cur state = %d, config state = %d, cur limt = %d\n",
		__func__, wlc->cur_state, state, wlc->charging_current_limit1);

	return 0;
}
static const struct thermal_cooling_device_ops wlc_tcd_ops = {
	.get_max_state = wlc_tcd_get_max_state,
	.get_cur_state = wlc_tcd_get_cur_state,
	.set_cur_state = wlc_tcd_set_cur_state,
};

#define VBUS_DEFAULT_MV 5000
int mtk_wlc_set_charging_current(struct charger_manager *pinfo,
	unsigned int *ichg, unsigned int *aicr)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;
//	int ichg1_min = -1, aicr1_min = -1;
	int charging_current, input_current, vbus, input_thermal_limit;
	bool cable_ready = false;
	*aicr = 0;
	*ichg = 0;

	chr_info("%s: starts\n", __func__);

	if (!wlc->is_connect)
		return -ENOTSUPP;

//	if (wlc->input_current_limit1 == 0 ||
//	    wlc->charging_current_limit1 == 0) {
//		pr_notice("input/charging current is 0, end WLC\n");
//		return -1;
//	}
	mutex_lock(&wlc->access_lock);

	input_current = wlc->wireless_charger_max_input_current;
	vbus = VBUS_DEFAULT_MV;
	charging_current = wlc->wireless_charger_max_current;

	if(NULL != wls_chg_ops) {
		wls_chg_ops->wls_current_select(&input_current, &vbus,&cable_ready);
		if(wlc_get_soc() == 100)
			wls_chg_ops->wls_set_battery_soc(100);
	}

	if(!cable_ready) {
		chr_info("%s: cable_ready:%d\n", __func__, cable_ready);
//		input_current = 0;
	}

	if (wlc->charging_current_limit1 != -1) {
		if (wlc->charging_current_limit1 < charging_current)
			wlc->charging_current1 = wlc->charging_current_limit1;
//		ret = wlc_hal_get_min_charging_current(alg, CHG1, &ichg1_min);
//		if (ret != -EOPNOTSUPP &&
//		    wlc->charging_current_limit1 < ichg1_min)
//			wlc->charging_current1 = 0;

		input_thermal_limit = wlc->charging_current_limit1 / 1000;
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
//		ret = wlc_hal_get_min_input_current(alg, CHG1, &aicr1_min);
//		if (ret != -EOPNOTSUPP && input_thermal_limit < aicr1_min)
//			wlc->input_current1 = 0;
	} else
		wlc->input_current1 = input_current;

	chr_info("%s input current = %d:%d:%d:%d, vout = %d\n", __func__, wlc->input_current1,
			input_current, input_thermal_limit, wlc->charging_current_limit1, vbus);
	mutex_unlock(&wlc->access_lock);

	if (wlc->input_current1 == 0 /*|| wlc->charging_current1 == 0*/) {
		chr_info("current is zero %d %d\n", wlc->input_current1,
			wlc->charging_current1);
		return -1;
	}

	chr_info("%s: starts, ichg:%d\n", __func__, *ichg);
	*aicr = wlc->charging_current1;
	*ichg = wlc->input_current1;
	chr_info("%s: OK, ichg = %dmA, AICR = %dmA\n",
		__func__, *ichg / 1000, *aicr / 1000);

	return ret;
}

int mtk_wlc_plugout_reset(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;

	chr_info("%s: starts\n", __func__);

	wlc->is_connect = false;
	wlc->is_cable_out_occur = false;
	wlc->to_check_chr_type = true;
	wlc->idx = -1;
	wlc->vbus = 5000000; /* mV */

	/* Enable OVP */
	ret = wlc_enable_vbus_ovp(pinfo, true);
	if (ret < 0)
		goto err;

	/* Set MIVR for vbus 5V */
	ret = wlc_set_mivr(pinfo, pinfo->wlc.min_charger_voltage); /* uV */
	if (ret < 0)
		goto err;

	chr_info("%s: OK\n", __func__);

	return ret;
err:
	chr_err("%s: failed, ret = %d\n", __func__, ret);

	return ret;
}

bool mtk_wlc_check_charger_avail(void){
	chr_info("%s: starts\n", __func__);

	if(mt_get_charger_type() != WIRELESS_CHARGER)
		return false;
	else
		return true;
}

int mtk_wlc_check_charger(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_wlc *wlc = &pinfo->wlc;
/*
	if (!pinfo->enable_hv_charging) {
		pr_info("%s: hv charging is disabled\n", __func__);
		if (wlc->is_connect) {
			wlc_leave(pinfo);
			wlc->to_check_chr_type = true;
		}
		return ret;
	}
*/
	chr_info("%s: starts\n", __func__);

	if (pinfo->enable_wlc != true)
		return -ENOTSUPP;

	if (!wlc->is_enabled) {
		pr_debug("%s: stop, WLC is disabled\n", __func__);
		return ret;
	}

	mutex_lock(&wlc->access_lock);
	__pm_stay_awake(wlc->suspend_lock);

	chr_info("%s\n", __func__);

	if (wlc->is_cable_out_occur)
		mtk_wlc_plugout_reset(pinfo);

	/* Not to check charger type or
	 * Not standard charger or
	 * SOC is not in range
	 */
	if (!wlc->to_check_chr_type ||
	    mt_get_charger_type() != WIRELESS_CHARGER)
		goto out;

	ret = wlc_init_ta(pinfo);
	if (ret < 0)
		goto out;

	if (cancel_wlc(pinfo))
		goto out;

	ret = mtk_wlc_reset_ta_vchr(pinfo);
	if (ret < 0)
		goto out;

	if (cancel_wlc(pinfo))
		goto out;

	mtk_wlc_check_cable_impedance(pinfo);

	if (cancel_wlc(pinfo))
		goto out;

	ret = wlc_detect_ta(pinfo);
	if (ret < 0)
		goto out;

	wlc->to_check_chr_type = false;

/*
	if (wlc->is_connect == true) {
		ret = wlc_set_ta_vchr(pinfo, 5500000);
		if (ret == 0)
			wlc_set_mivr(pinfo, pinfo->wlc->min_charger_voltage);
		else
			wlc_leave(pinfo);
	}
*/
	chr_info("%s: OK, to_check_chr_type = %d\n",
		__func__, wlc->to_check_chr_type);
	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);

	return ret;
out:

	chr_info("%s: stop, SOC = (%d), to_check_chr_type = %d, chr_type = %d, ret = %d\n",
		__func__, battery_get_soc(),
		wlc->to_check_chr_type,
		mt_get_charger_type(), ret);

	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);

	return ret;
}


int mtk_wlc_start_algorithm(struct charger_manager *pinfo)
{
	int ret = 0;
	int i;
	int vbat, vbus, ichg;
	int pre_vbus, pre_idx;
	int tune = 0, pes = 0; /* For log, to know the state of WLC */
	u32 size;
	struct mtk_wlc *wlc = &pinfo->wlc;
/*
	if (!pinfo->enable_hv_charging) {
		chr_info("%s: hv charging is disabled\n", __func__);
		if (wlc->is_connect) {
			wlc_leave(pinfo);
			wlc->to_check_chr_type = true;
		}
		return ret;
	}
*/
	chr_info("%s: starts\n", __func__);

	if (!wlc->is_enabled) {
		chr_info("%s: stop, WLC is disabled\n",
			__func__);
		return ret;
	}

	mutex_lock(&wlc->access_lock);
	__pm_stay_awake(wlc->suspend_lock);
	chr_info("%s\n", __func__);

	if (wlc->is_cable_out_occur)
		mtk_wlc_plugout_reset(pinfo);

	if (!wlc->is_connect) {
		ret = -EIO;
		chr_info("%s: stop, WLC is not connected\n",
			__func__);
		__pm_relax(wlc->suspend_lock);
		mutex_unlock(&wlc->access_lock);
		return ret;
	}

	vbat = wlc_get_vbat();
	vbus = wlc_get_vbus();
	ichg = wlc_get_ibat();

	pre_vbus = wlc->vbus;
	pre_idx = wlc->idx;

	ret = wlc_check_leave_status(pinfo);
	if (!wlc->is_connect || ret < 0) {
		pes = 1;
		goto out;
	}

	size = ARRAY_SIZE(wlc->profile);
	for (i = 0; i < size; i++) {
		tune = 0;

		/* Exceed this level, check next level */
		if (vbat > (wlc->profile[i].vbat + 100000))
			continue;

		/* If vbat is still 30mV larger than the lower level
		 * Do not down grade
		 */
		if (i < wlc->idx && vbat > (wlc->profile[i].vbat + 30000))
			continue;

		if (wlc->vbus != wlc->profile[i].vchr)
			tune = 1;

		wlc->vbus = wlc->profile[i].vchr;
		wlc->idx = i;

		if (abs(vbus - wlc->vbus) >= 1000000)
			tune = 2;

		if (tune != 0) {
			ret = wlc_set_ta_vchr(pinfo, wlc->vbus);
			if (ret == 0)
				wlc_set_mivr(pinfo, wlc->min_charger_voltage);
			else
				wlc_leave(pinfo);
		} else
			wlc_set_mivr(pinfo, wlc->min_charger_voltage);
		break;
	}
	pes = 2;

out:
	chr_info("%s: vbus = (%d, %d), idx = (%d, %d), I = %d\n",
		__func__, pre_vbus / 1000, wlc->vbus / 1000, pre_idx,
		wlc->idx, ichg / 1000);

	chr_info("%s: SOC = %d, is_connect = %d, tune = %d, pes = %d, vbat = %d, ret = %d\n",
		__func__, battery_get_soc(), wlc->is_connect, tune, pes,
		vbat / 1000, ret);
	__pm_relax(wlc->suspend_lock);
	mutex_unlock(&wlc->access_lock);

	return ret;
}

void mtk_wlc_set_to_check_chr_type(struct charger_manager *pinfo, bool check)
{
	mutex_lock(&pinfo->wlc.access_lock);
	__pm_stay_awake(pinfo->wlc.suspend_lock);

	chr_info("%s: check = %d\n", __func__, check);
	pinfo->wlc.to_check_chr_type = check;

	__pm_relax(pinfo->wlc.suspend_lock);
	mutex_unlock(&pinfo->wlc.access_lock);
}

void mtk_wlc_set_is_enable(struct charger_manager *pinfo, bool enable)
{
	mutex_lock(&pinfo->wlc.access_lock);
	__pm_stay_awake(pinfo->wlc.suspend_lock);

	chr_info("%s: enable = %d\n", __func__, enable);
	pinfo->wlc.is_enabled = enable;
	pinfo->wlc.is_connect = enable;

	__pm_relax(pinfo->wlc.suspend_lock);
	mutex_unlock(&pinfo->wlc.access_lock);
}

void mtk_wlc_set_is_cable_out_occur(struct charger_manager *pinfo, bool out)
{
	chr_info("%s: out = %d\n", __func__, out);
	mutex_lock(&pinfo->wlc.pmic_sync_lock);
	pinfo->wlc.is_cable_out_occur = out;
	mutex_unlock(&pinfo->wlc.pmic_sync_lock);
}

bool mtk_wlc_get_to_check_chr_type(struct charger_manager *pinfo)
{
	return pinfo->wlc.to_check_chr_type;
}

bool mtk_wlc_get_is_connect(struct charger_manager *pinfo)
{
	/*
	 * Cable out is occurred,
	 * but not execute plugout_reset yet
	 */
	if (pinfo->wlc.is_cable_out_occur)
		return false;

	return pinfo->wlc.is_connect;
}

bool mtk_wlc_get_is_enable(struct charger_manager *pinfo)
{
	return pinfo->wlc.is_enabled;
}
static void mtk_wlc_parse_dt(struct charger_manager *info,
				struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 val;
	int i;

	chr_err("%s: starts\n", __func__);

	if (!np) {
		chr_err("%s: no device node\n", __func__);
		return;
	}
	info->wlc.wls_control_en = of_get_named_gpio(np, "mmi,wls_control_en", 0);
	if (!gpio_is_valid(info->wlc.wls_control_en))
		pr_err("wls_control_en is %d invalid\n", info->wlc.wls_control_en);

	if (of_property_read_u32(np, "min_charger_voltage", &val) >= 0)
		info->wlc.min_charger_voltage = val;
	else {
		pr_notice("use default V_CHARGER_MIN:%d\n", WLC_V_CHARGER_MIN);
		info->wlc.min_charger_voltage = WLC_V_CHARGER_MIN;
	}
	if (of_property_read_u32(np, "wlc_max_charger_current", &val) >= 0) {
		info->wlc.wireless_charger_max_current = val;
	} else {
		pr_err("use default WIRELESS_CHARGER_MAX_CURRENT:%d\n",
			WIRELESS_CHARGER_MAX_CURRENT);
		info->wlc.wireless_charger_max_current =
			WIRELESS_CHARGER_MAX_CURRENT;
	}

	if (of_property_read_u32(np, "wlc_max_input_current", &val) >= 0) {
		info->wlc.wireless_charger_max_input_current = val;
	} else {
		pr_err("use default WIRELESS_CHARGER_MAX_INPUT_CURRENT:%d\n",
			WIRELESS_CHARGER_MAX_INPUT_CURRENT);
		info->wlc.wireless_charger_max_input_current = WIRELESS_CHARGER_MAX_INPUT_CURRENT;
	}

	if (of_property_read_u32_array(np, "mmi,wlc-rx-mitigation",
			wlc_state_to_current_limit, CHARGER_STATE_NUM))
		pr_info("Not define wlc rx thermal table, use defaut table\n");

	for (i = 0; i < CHARGER_STATE_NUM; i++) {
		pr_info("mmi wlc rx table: table %d, current %d mA\n",
			i, wlc_state_to_current_limit[i]);
	}

}

int mtk_wlc_remove(struct charger_manager *pinfo)
{
//	struct mtk_wlc *wlc;

//	wlc = (struct mtk_wlc  *)platform_get_drvdata(dev);
//	if(pinfo->wlc.tcd)
//		thermal_cooling_device_unregister(pinfo->wlc.tcd);

	return 0;
}

int mtk_wlc_init(struct charger_manager *pinfo,struct device *dev)
{
//	int ret = 0;

	//wakeup_source_init(pinfo->wlc.suspend_lock, "WLC suspend wakelock");
	pinfo->wlc.suspend_lock = wakeup_source_register(NULL, "WLC suspend wakelock");
	mutex_init(&pinfo->wlc.access_lock);
	mutex_init(&pinfo->wlc.pmic_sync_lock);

	pinfo->wlc.ta_vchr_org = 5000000;
	pinfo->wlc.idx = -1;
	pinfo->wlc.vbus = 5000000;
	pinfo->wlc.to_check_chr_type = true;
	pinfo->wlc.is_enabled = true;

	pinfo->wlc.profile[0].vbat = 3400000;
	pinfo->wlc.profile[1].vbat = 3500000;
	pinfo->wlc.profile[2].vbat = 3600000;
	pinfo->wlc.profile[3].vbat = 3700000;
	pinfo->wlc.profile[4].vbat = 3800000;
	pinfo->wlc.profile[5].vbat = 3900000;
	pinfo->wlc.profile[6].vbat = 4000000;
	pinfo->wlc.profile[7].vbat = 4100000;
	pinfo->wlc.profile[8].vbat = 4200000;
	pinfo->wlc.profile[9].vbat = 4300000;

	pinfo->wlc.profile[0].vchr = 8000000;
	pinfo->wlc.profile[1].vchr = 8500000;
	pinfo->wlc.profile[2].vchr = 8500000;
	pinfo->wlc.profile[3].vchr = 9000000;
	pinfo->wlc.profile[4].vchr = 9000000;
	pinfo->wlc.profile[5].vchr = 9000000;
	pinfo->wlc.profile[6].vchr = 9500000;
	pinfo->wlc.profile[7].vchr = 9500000;
	pinfo->wlc.profile[8].vchr = 10000000;
	pinfo->wlc.profile[9].vchr = 10000000;

	mtk_wlc_parse_dt(pinfo, dev);
//	ret = charger_dev_set_pe20_efficiency_table(pinfo->chg1_dev);
//	if (ret != 0)
//		chr_err("%s: use default table, %d\n", __func__, ret);
	pinfo->wlc.charging_current_limit1 = -1;
	pinfo->wlc.max_state = CHARGER_STATE_NUM - 1;
//	pinfo->wlc.tcd = thermal_of_cooling_device_register(dev_of_node(dev),
//		"moto_wlc", &pinfo->wlc, &wlc_tcd_ops);

	return 0;
}

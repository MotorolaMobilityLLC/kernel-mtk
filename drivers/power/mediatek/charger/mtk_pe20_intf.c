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

#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/delay.h>

#include "mtk_charger_intf.h"
#include <mt-plat/charger_type.h>
#include <mt-plat/mtk_battery.h>
#include <upmu_common.h>

static int pe20_set_mivr(struct charger_manager *pinfo, int uV);

/* Unit of the following functions are uV, uA */
static inline u32 pe20_get_vbus(void)
{
	return pmic_get_vbus() * 1000;
}

static inline u32 pe20_get_vbat(void)
{
	return pmic_get_battery_voltage() * 1000;
}

static inline u32 pe20_get_ibat(void)
{
	return battery_meter_get_battery_current() * 100;
}

int mtk_pe20_reset_ta_vchr(struct charger_manager *pinfo)
{
	int ret = 0, chr_volt = 0;
	u32 retry_cnt = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);

	/* Reset TA's charging voltage */
	do {
		if (pinfo->chg2_dev)
			charger_dev_enable(pinfo->chg2_dev, false);

		ret = charger_dev_set_ta20_reset(pinfo->chg1_dev);
		msleep(250);

		/* Check charger's voltage */
		chr_volt = pe20_get_vbus();
		if (abs(chr_volt - pe20->ta_vchr_org) <= 1000000) {
			pe20->vbus = chr_volt;
			pe20->idx = -1;
			pe20->is_connect = false;
			break;
		}

		retry_cnt++;
	} while (retry_cnt < 3);

	if (pe20->is_connect) {
		pr_err("%s: failed, ret = %d\n",
			__func__, ret);
		return ret;
	}

	pe20_set_mivr(pinfo, 4500000);

	/* Measure VBAT */
	pe20->vbat_orig = pe20_get_vbat();

	pr_err("%s: OK\n", __func__);

	return ret;
}

static int pe20_enable_hw_vbus_ovp(struct charger_manager *pinfo, bool enable)
{
	int ret = 0;

	ret = pmic_set_register_value(PMIC_RG_VCDT_HV_EN, enable);
	if (ret != 0)
		pr_err("%s: failed, ret = %d\n", __func__, ret);

	return ret;
}

/* Enable/Disable HW */
static int pe20_enable_vbus_ovp(struct charger_manager *pinfo, bool enable)
{
	int ret = 0;

	/* Enable/Disable HW(PMIC) OVP */
	ret = pe20_enable_hw_vbus_ovp(pinfo, enable);
	if (ret < 0) {
		pr_err("%s: failed, ret = %d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static int pe20_set_mivr(struct charger_manager *pinfo, int uV)
{
	int ret = 0;

	ret = charger_dev_set_mivr(pinfo->chg1_dev, uV);
	if (ret < 0)
		pr_err("%s: failed, ret = %d\n", __func__, ret);
	return ret;
}

static int pe20_leave(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);
	ret = mtk_pe20_reset_ta_vchr(pinfo);
	if (ret < 0 || pe20->is_connect) {
		pr_err("%s: failed, is_connect = %d, ret = %d\n",
			__func__, pe20->is_connect, ret);
		return ret;
	}

	pe20_enable_vbus_ovp(pinfo, true);
	pe20_set_mivr(pinfo, 4500000);
	pr_err("%s: OK\n", __func__);
	return ret;
}

static int pe20_check_leave_status(struct charger_manager *pinfo)
{
	int ret = 0;
	int ichg = 0, vchr = 0;
	bool current_sign;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);

	/* PE+ leaves unexpectedly */
	vchr = pe20_get_vbus();
	if (abs(vchr - pe20->ta_vchr_org) < 1000000) {
		pr_err("%s: PE+20 leave unexpectedly, recheck TA\n", __func__);
		pe20->to_check_chr_type = true;
		ret = pe20_leave(pinfo);
		if (ret < 0 || pe20->is_connect)
			goto _err;

		return ret;
	}

	ichg = pe20_get_ibat();
	current_sign = battery_meter_get_battery_current_sign();

	/* Check SOC & Ichg */
	if (battery_get_bat_soc() > pinfo->data.ta_stop_battery_soc &&
	    current_sign && ichg < pinfo->data.pe20_ichg_level_threshold * 1000) {
		ret = pe20_leave(pinfo);
		if (ret < 0 || pe20->is_connect)
			goto _err;
		pr_err("%s: OK, SOC = (%d,%d), stop PE+20\n", __func__,
			battery_get_bat_soc(), pinfo->data.ta_stop_battery_soc);
	}

	return ret;

_err:
	pr_err("%s: failed, is_connect = %d, ret = %d\n",
		__func__, pe20->is_connect, ret);
	return ret;
}

static int __pe20_set_ta_vchr(struct charger_manager *pinfo, u32 chr_volt)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);

	/* Not to set chr volt if cable is plugged out */
	if (pe20->is_cable_out_occur) {
		pr_err("%s: failed, cable out\n", __func__);
		return -EIO;
	}

	if (pinfo->chg2_dev)
		charger_dev_enable(pinfo->chg2_dev, false);

	ret = charger_dev_send_ta20_current_pattern(pinfo->chg1_dev, chr_volt);
	if (ret < 0) {
		pr_err("%s: failed, ret = %d\n", __func__, ret);
		return ret;
	}

	pr_err("%s: OK\n", __func__);

	return ret;
}

static int pe20_set_ta_vchr(struct charger_manager *pinfo, u32 chr_volt)
{
	int ret = 0;
	int vchr_before, vchr_after, vchr_delta;
	const u32 sw_retry_cnt_max = 3;
	const u32 retry_cnt_max = 5;
	u32 sw_retry_cnt = 0, retry_cnt = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);

	do {
		vchr_before = pe20_get_vbus();
		ret = __pe20_set_ta_vchr(pinfo, chr_volt);
		vchr_after = pe20_get_vbus();

		vchr_delta = abs(vchr_after - chr_volt);

		/* It is successful
		 * if difference to target is less than 500mA
		 */
		if (vchr_delta < 500000 && ret == 0) {
			pr_err("%s: OK, vchr = (%d, %d), vchr_target = %dmV\n",
				__func__, vchr_before / 1000, vchr_after / 1000,
				chr_volt / 1000);
			return ret;
		}

		if (ret == 0 || sw_retry_cnt >= sw_retry_cnt_max)
			retry_cnt++;
		else
			sw_retry_cnt++;

		pe20_set_mivr(pinfo, 4500000);

		pr_err("%s: retry_cnt = (%d, %d), vchr = (%d, %d), vchr_target = %dmV\n",
			__func__, sw_retry_cnt, retry_cnt, vchr_before / 1000,
			vchr_after / 1000, chr_volt / 1000);

	} while (!pe20->is_cable_out_occur && mt_get_charger_type() != CHARGER_UNKNOWN
		&& retry_cnt < retry_cnt_max && pinfo->enable_hv_charging);

	ret = -EIO;
	pr_err("%s: failed, vchr_org = %dmV, vchr_after = %dmV, target_vchr = %dmV\n",
		__func__, pe20->ta_vchr_org / 1000, vchr_after / 1000,
		chr_volt / 1000);

	return ret;
}

static void mtk_pe20_check_cable_impedance(struct charger_manager *pinfo)
{
	int ret = 0;
	int vchr1, vchr2, cable_imp;
	unsigned int aicr_value;
	bool mivr_state = false;
	struct timespec ptime[2], diff;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_debug("%s: starts\n", __func__);

	if (pe20->vbat_orig > pinfo->data.vbat_cable_imp_threshold * 1000) {
		pr_err("VBAT > %dmV, directly set aicr to %dmA\n",
			pinfo->data.vbat_cable_imp_threshold,
			pinfo->data.ac_charger_input_current);
		pe20->aicr_cable_imp = pinfo->data.ac_charger_input_current;
		goto end;
	}
#if 0
	/* Trigger adapter WDT to drop VBUS to 5V */
	aicr_value = 100000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);
	mdelay(240);
#endif
	/* Disable cable drop compensation */
	charger_dev_enable_cable_drop_comp(pinfo->chg1_dev, false);

	get_monotonic_boottime(&ptime[0]);

	/* Set ichg = 2500mA, set MIVR=4.5V */
	charger_dev_set_charging_current(pinfo->chg1_dev, 2500000);
	mdelay(240);
	pe20_set_mivr(pinfo, 4500000);
	/* pe20_set_mivr(pinfo, 4300000); */

	get_monotonic_boottime(&ptime[1]);
	diff = timespec_sub(ptime[1], ptime[0]);

	aicr_value = 800000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);

	/* To wait for soft-start */
	msleep(150);

	ret = charger_dev_get_mivr_state(pinfo->chg1_dev, &mivr_state);
	if (ret != -ENOTSUPP && mivr_state) {
		pe20->aicr_cable_imp = 1000000;
		goto end;
	}

	vchr1 = pe20_get_vbus();

	aicr_value = 500000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);
	msleep(20);

	vchr2 = pe20_get_vbus();

	/*
	 * Calculate cable impedance (|V1 - V2|) / (|I2 - I1|)
	 * m_ohm = (mv * 10 * 1000) / (mA * 10)
	 * m_ohm = (uV * 10) / (mA * 10)
	 */
	cable_imp = (abs(vchr1 - vchr2) * 10) / (7400 - 4625);

	pr_err("%s: cable_imp:%d mohm, vchr1:%d, vchr2:%d, time:%ld\n",
		    __func__, cable_imp, vchr1 / 1000, vchr2 / 1000, diff.tv_nsec);

	/* Recover cable drop compensation */
	aicr_value = 100000;
	charger_dev_set_input_current(pinfo->chg1_dev, aicr_value);
	msleep(250);

	if (cable_imp < pinfo->data.cable_imp_threshold) {
		pe20->aicr_cable_imp = 3200000;
		pr_err("Normal cable\n");
	} else {
		pe20->aicr_cable_imp = 1000000;
		pr_err("Bad cable\n");
	}

	pr_info("%s: set aicr:%dmA, vbat:%dmV, mivr_state:%d\n",
		__func__, pe20->aicr_cable_imp / 1000,
		pe20->vbat_orig / 1000, mivr_state);
	return;

end:
	pr_err("%s not started: set aicr:%dmA, vbat:%dmV, mivr_state:%d\n",
		__func__, pe20->aicr_cable_imp / 1000,
		pe20->vbat_orig / 1000, mivr_state);
}

static int pe20_detect_ta(struct charger_manager *pinfo)
{
	int ret;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);
	pe20->ta_vchr_org = pe20_get_vbus();

	/* Disable OVP */
	ret = pe20_enable_vbus_ovp(pinfo, false);
	if (ret < 0)
		goto err;

	if (abs(pe20->ta_vchr_org - 8500000) > 500000)
		ret = pe20_set_ta_vchr(pinfo, 8500000);
	else
		ret = pe20_set_ta_vchr(pinfo, 6500000);

	if (ret < 0) {
		pe20->to_check_chr_type = false;
		goto err;
	}

	pe20->is_connect = true;
	/*mt_charger_enable_DP_voltage(1);*/
	pr_err("%s: OK\n", __func__);

	return ret;
err:
	pe20->is_connect = false;
	pe20_enable_vbus_ovp(pinfo, true);
	pr_err("%s: failed, ret = %d\n", __func__, ret);
	return ret;
}

static int pe20_init_ta(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pe20->is_connect = false;
	pe20->is_cable_out_occur = false;

	pr_err("%s: starts\n", __func__);

#ifdef PE20_HW_INIT /* need check */
	ret = battery_charging_control(CHARGING_CMD_INIT, NULL);
	if (ret < 0) {
		pr_err("%s: failed, ret = %d\n", __func__, ret);
		return ret;
	}
#endif /*PE20_HW_INIT*/

	pr_err("%s: OK\n", __func__);
	return ret;
}

int mtk_pe20_set_charging_current(struct charger_manager *pinfo,
	unsigned int *ichg, unsigned int *aicr)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	if (!pe20->is_connect)
		return -ENOTSUPP;

	pr_err("%s: starts\n", __func__);
	/* *aicr = 3200000; */
	*aicr = pe20->aicr_cable_imp;
	*ichg = pinfo->data.ta_ac_charger_current;
	pr_err("%s: OK, ichg = %dmA, AICR = %dmA\n",
		__func__, *ichg / 1000, *aicr / 1000);

	return ret;
}

int mtk_pe20_plugout_reset(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	pr_err("%s: starts\n", __func__);

	pe20->is_connect = false;
	pe20->is_cable_out_occur = false;
	pe20->to_check_chr_type = true;
	pe20->idx = -1;
	pe20->vbus = 5000000; /* mV */

	/* Enable OVP */
	ret = pe20_enable_vbus_ovp(pinfo, true);
	if (ret < 0)
		goto err;

	/* Set MIVR to 4.5V for vbus 5V */
	ret = pe20_set_mivr(pinfo, 4500000); /* uV */
	if (ret < 0)
		goto err;

	/*mt_charger_enable_DP_voltage(0);*/
	pr_err("%s: OK\n", __func__);

	return ret;
err:
	pr_err("%s: failed, ret = %d\n", __func__, ret);

	return ret;
}

int mtk_pe20_check_charger(struct charger_manager *pinfo)
{
	int ret = 0;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	if (!pinfo->enable_hv_charging) {
		pr_info("%s: hv charging is disabled\n", __func__);
		if (pe20->is_connect) {
			pe20_leave(pinfo);
			pe20->to_check_chr_type = true;
		}
		return ret;
	}

	if (pinfo->enable_pe_2 != true)
		return -ENOTSUPP;

	if (!pe20->is_enabled) {
		pr_debug("%s: stop, PE+20 is disabled\n", __func__);
		return ret;
	}

	mutex_lock(&pe20->access_lock);
	wake_lock(&pe20->suspend_lock);

	pr_info("%s\n", __func__);

	if (pe20->is_cable_out_occur)
		mtk_pe20_plugout_reset(pinfo);

	/* Not to check charger type or
	 * Not standard charger or
	 * SOC is not in range
	 */
	if (!pe20->to_check_chr_type ||
	    mt_get_charger_type() != STANDARD_CHARGER ||
	    battery_get_bat_soc() < pinfo->data.ta_start_battery_soc ||
	    battery_get_bat_soc() >= pinfo->data.ta_stop_battery_soc)
		goto out;

	ret = pe20_init_ta(pinfo);
	if (ret < 0)
		goto out;

	ret = mtk_pe20_reset_ta_vchr(pinfo);
	if (ret < 0)
		goto out;

	mtk_pe20_check_cable_impedance(pinfo);

	ret = pe20_detect_ta(pinfo);
	if (ret < 0)
		goto out;

	pe20->to_check_chr_type = false;

	pr_err("%s: OK, to_check_chr_type = %d\n",
		__func__, pe20->to_check_chr_type);
	wake_unlock(&pe20->suspend_lock);
	mutex_unlock(&pe20->access_lock);

	return ret;
out:

	pr_err("%s: stop, SOC = (%d, %d, %d), to_check_chr_type = %d, chr_type = %d, ret = %d\n",
		__func__, battery_get_bat_soc(), pinfo->data.ta_start_battery_soc,
		pinfo->data.ta_stop_battery_soc, pe20->to_check_chr_type,
		mt_get_charger_type(), ret);

	wake_unlock(&pe20->suspend_lock);
	mutex_unlock(&pe20->access_lock);

	return ret;
}


int mtk_pe20_start_algorithm(struct charger_manager *pinfo)
{
	int ret = 0;
	int i;
	int vbat, vbus, ichg;
	int pre_vbus, pre_idx;
	int tune = 0, pes = 0; /* For log, to know the state of PE+20 */
	bool current_sign;
	u32 size;
	struct mtk_pe20 *pe20 = &pinfo->pe2;

	if (!pinfo->enable_hv_charging) {
		pr_info("%s: hv charging is disabled\n", __func__);
		if (pe20->is_connect) {
			pe20_leave(pinfo);
			pe20->to_check_chr_type = true;
		}
		return ret;
	}

	if (!pe20->is_enabled) {
		pr_debug("%s: stop, PE+20 is disabled\n",
			__func__);
		return ret;
	}

	mutex_lock(&pe20->access_lock);
	wake_lock(&pe20->suspend_lock);
	pr_info("%s\n", __func__);

	if (pe20->is_cable_out_occur)
		mtk_pe20_plugout_reset(pinfo);

	if (!pe20->is_connect) {
		ret = -EIO;
		pr_debug("%s: stop, PE+20 is not connected\n",
			__func__);
		wake_unlock(&pe20->suspend_lock);
		mutex_unlock(&pe20->access_lock);
		return ret;
	}

	vbat = pe20_get_vbat();
	vbus = pe20_get_vbus();
	ichg = pe20_get_ibat();
	current_sign = battery_meter_get_battery_current_sign();

	pre_vbus = pe20->vbus;
	pre_idx = pe20->idx;

	ret = pe20_check_leave_status(pinfo);
	if (!pe20->is_connect || ret < 0) {
		pes = 1;
		goto out;
	}

	size = ARRAY_SIZE(pe20->profile);
	for (i = 0; i < size; i++) {
		tune = 0;

		/* Exceed this level, check next level */
		if (vbat > (pe20->profile[i].vbat + 100000))
			continue;

		/* If vbat is still 30mV larger than the lower level
		 * Do not down grade
		 */
		if (i < pe20->idx && vbat > (pe20->profile[i].vbat + 30000))
			continue;

		if (pe20->vbus != pe20->profile[i].vchr)
			tune = 1;

		pe20->vbus = pe20->profile[i].vchr;
		pe20->idx = i;

		if (abs(vbus - pe20->vbus) >= 1000000)
			tune = 2;

		if (tune != 0) {
			ret = pe20_set_ta_vchr(pinfo, pe20->vbus);
			if (ret == 0)
				pe20_set_mivr(pinfo, pe20->vbus - 500000);
			else
				pe20_leave(pinfo);
		}
		break;
	}
	pes = 2;

out:
	pr_err("%s: vbus = (%d, %d), idx = (%d, %d), I = (%d, %d)\n",
		__func__, pre_vbus / 1000, pe20->vbus / 1000, pre_idx, pe20->idx,
		(int)current_sign, (int)ichg / 1000);

	pr_err("%s: SOC = %d, is_connect = %d, tune = %d, pes = %d, vbat = %d, ret = %d\n",
		__func__, battery_get_bat_soc(), pe20->is_connect, tune, pes,
		vbat / 1000, ret);
	wake_unlock(&pe20->suspend_lock);
	mutex_unlock(&pe20->access_lock);

	return ret;
}

void mtk_pe20_set_to_check_chr_type(struct charger_manager *pinfo, bool check)
{
	mutex_lock(&pinfo->pe2.access_lock);
	wake_lock(&pinfo->pe2.suspend_lock);

	pr_err("%s: check = %d\n", __func__, check);
	pinfo->pe2.to_check_chr_type = check;

	wake_unlock(&pinfo->pe2.suspend_lock);
	mutex_unlock(&pinfo->pe2.access_lock);
}

void mtk_pe20_set_is_enable(struct charger_manager *pinfo, bool enable)
{
	mutex_lock(&pinfo->pe2.access_lock);
	wake_lock(&pinfo->pe2.suspend_lock);

	pr_err("%s: enable = %d\n", __func__, enable);
	pinfo->pe2.is_enabled = enable;

	wake_unlock(&pinfo->pe2.suspend_lock);
	mutex_unlock(&pinfo->pe2.access_lock);
}

void mtk_pe20_set_is_cable_out_occur(struct charger_manager *pinfo, bool out)
{
	pr_err("%s: out = %d\n", __func__, out);
	mutex_lock(&pinfo->pe2.pmic_sync_lock);
	pinfo->pe2.is_cable_out_occur = out;
	mutex_unlock(&pinfo->pe2.pmic_sync_lock);
}

bool mtk_pe20_get_to_check_chr_type(struct charger_manager *pinfo)
{
	return pinfo->pe2.to_check_chr_type;
}

bool mtk_pe20_get_is_connect(struct charger_manager *pinfo)
{
	/*
	 * Cable out is occurred,
	 * but not execute plugout_reset yet
	 */
	if (pinfo->pe2.is_cable_out_occur)
		return false;

	return pinfo->pe2.is_connect;
}

bool mtk_pe20_get_is_enable(struct charger_manager *pinfo)
{
	return pinfo->pe2.is_enabled;
}

int mtk_pe20_init(struct charger_manager *pinfo)
{
	int ret = 0;

	wake_lock_init(&pinfo->pe2.suspend_lock, WAKE_LOCK_SUSPEND,
		"PE+20 TA charger suspend wakelock");
	mutex_init(&pinfo->pe2.access_lock);
	mutex_init(&pinfo->pe2.pmic_sync_lock);

	pinfo->pe2.ta_vchr_org = 5000000;
	pinfo->pe2.idx = -1;
	pinfo->pe2.vbus = 5000000;
	pinfo->pe2.to_check_chr_type = true;
	pinfo->pe2.is_enabled = true;

	pinfo->pe2.profile[0].vbat = 3400000;
	pinfo->pe2.profile[1].vbat = 3500000;
	pinfo->pe2.profile[2].vbat = 3600000;
	pinfo->pe2.profile[3].vbat = 3700000;
	pinfo->pe2.profile[4].vbat = 3800000;
	pinfo->pe2.profile[5].vbat = 3900000;
	pinfo->pe2.profile[6].vbat = 4000000;
	pinfo->pe2.profile[7].vbat = 4100000;
	pinfo->pe2.profile[8].vbat = 4200000;
	pinfo->pe2.profile[9].vbat = 4300000;

	pinfo->pe2.profile[0].vchr = 8000000;
	pinfo->pe2.profile[1].vchr = 8500000;
	pinfo->pe2.profile[2].vchr = 8500000;
	pinfo->pe2.profile[3].vchr = 9000000;
	pinfo->pe2.profile[4].vchr = 9000000;
	pinfo->pe2.profile[5].vchr = 9000000;
	pinfo->pe2.profile[6].vchr = 9500000;
	pinfo->pe2.profile[7].vchr = 9500000;
	pinfo->pe2.profile[8].vchr = 10000000;
	pinfo->pe2.profile[9].vchr = 10000000;

	ret = charger_dev_set_pe20_efficiency_table(pinfo->chg1_dev);
	if (ret != 0)
		pr_err("%s: use default table, %d\n", __func__, ret);

	return 0;
}

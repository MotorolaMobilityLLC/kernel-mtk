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
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <linux/suspend.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <linux/reboot.h>
#include "mtk_charger_intf.h"

#define  DC_INIT                        (0x00)
#define  DC_MEASURE_R					(0x01)
#define  DC_SOFT_START                  (0x02)
#define  DC_CC							(0x03)
#define  DC_STOP	                    (0x04)
#define BAT_MS_TO_NS(x) (x * 1000 * 1000)

static int mtk_pe30_set_ta_boundary_cap(struct charger_manager *info, int cur, int voltage)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	struct mtk_vdm_ta_cap cap;
	int ret = 0, cnt = 0;

	cap.cur = cur;
	cap.vol = voltage;

	do {
		ret = mtk_set_ta_boundary_cap(pe3->tcpc, &cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	return ret;
}

static int mtk_pe30_get_ta_boundary_cap(struct charger_manager *info, struct mtk_pe30_cap *cap)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_boundary_cap(pe3->tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pe30_set_ta_cap(struct charger_manager *info, int cur, int voltage)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	int ret = 0, cnt = 0;

	pe3->current_cap.cur = cur;
	pe3->current_cap.vol = voltage;

	do {
		ret = mtk_set_ta_cap(pe3->tcpc, &pe3->current_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	return ret;
}

static int mtk_pe30_get_ta_cap(struct charger_manager *info, struct mtk_pe30_cap *cap)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_cap(pe3->tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pe30_get_ta_current_cap(struct charger_manager *info, struct mtk_pe30_cap *cap)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_current_cap(pe3->tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pe30_get_ta_tmp_wdt(struct charger_manager *info, struct mtk_pe30_cap *cap)
{
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;
	struct mtk_pe30 *pe3 = &info->pe3;

	do {
		ret = mtk_monitor_ta_inform(pe3->tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;
	cap->temp = ta_cap.temp;

	return ret;
}

static int mtk_pe30_enable_direct_charge(struct charger_manager *info, bool en)
{
	int ret = 0, ret2 = 0;
	struct mtk_pe30_cap cap;
	struct pd_ta_stat pdstat;
	struct mtk_pe30 *pe3 = &info->pe3;

	if (en == true) {
		charger_dev_set_direct_charging_vbusov(info->dc_chg, 6500000);
		charger_dev_set_direct_charging_ibusoc(info->dc_chg, info->data.cc_max);
		ret = mtk_enable_direct_charge(pe3->tcpc, en);
		if (ret != 0)
			ret2 = ret;
		mtk_pe30_get_ta_tmp_wdt(info, &cap);
		ret = mtk_get_ta_charger_status(pe3->tcpc, &pdstat);
		if ((ret == MTK_VDM_SUCCESS) && pdstat.ping_chk_fail)
			mtk_clr_ta_pingcheck_fault(pe3->tcpc);
		ret = mtk_enable_ta_dplus_dect(pe3->tcpc, true, 4000);
		if (ret != 0)
			ret2 = ret;
	} else {
		ret = mtk_enable_direct_charge(pe3->tcpc, en);
		if (ret != 0)
			ret2 = ret;
		ret = mtk_enable_ta_dplus_dect(pe3->tcpc, false, 4000);
		if (ret != 0)
			ret2 = ret;
		charger_dev_set_direct_charging_vbusov(info->dc_chg, 5500000);
		charger_dev_set_direct_charging_ibusoc(info->dc_chg, info->data.cc_normal);
	}
	charger_dev_enable_direct_charging(info->dc_chg, en);

	return ret2;
}

int mtk_pe30_ta_hard_reset(struct charger_manager *info)
{
	int ret = 0, cnt = 0;
	struct mtk_pe30 *pe3 = &info->pe3;

	do {
		ret = tcpm_hard_reset(pe3->tcpc);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);


	return ret;
}

static bool mtk_pe30_get_ta_charger_status(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;
	struct pd_ta_stat pdstat;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_charger_status(pe3->tcpc, &pdstat);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	if (ret != 0)
		goto _fail;

	if (pdstat.ovp == 1 || pdstat.otp == 1 || pdstat.uvp == 1 || pdstat.rvs_cur == 1
		|| pdstat.ping_chk_fail == 1)
		goto _fail;


	pr_err("[mtk_pe30_get_ta_charger_status]%d %d %d %d %d %d %d %d %d %d\n",
		ret,
		pdstat.chg_mode, pdstat.dc_en, pdstat.dpc_en, pdstat.pc_en,
		pdstat.ovp, pdstat.otp, pdstat.uvp,
		pdstat.rvs_cur, pdstat.ping_chk_fail);

	return true;

_fail:

	pr_err("[mtk_pe30_get_ta_charger_status]fail %d %d %d %d %d %d %d %d %d %d\n",
		ret,
		pdstat.chg_mode, pdstat.dc_en, pdstat.dpc_en, pdstat.pc_en,
		pdstat.ovp, pdstat.otp, pdstat.uvp,
		pdstat.rvs_cur, pdstat.ping_chk_fail);

	return false;

}

static void wake_up_pe30_thread(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	pe3->mtk_charger_pe30_thread_flag = true;
	wake_up_interruptible(&pe3->mtk_charger_pe30_thread_waiter);

	pr_err("[wake_up_pe30_thread]\n");
}

static int mtk_pe30_get_cv(struct charger_manager *info)
{
	int cv = info->data.bat_upper_bound;

	return cv;
}

static int mtk_pe30_get_cali_vbus(struct charger_manager *info, int charging_current)
{
	int vbus = mtk_pe30_get_cv(info) + 70 + 110 * charging_current / 100 * info->pe3.r_total / 1000;

	return vbus;
}

static bool is_mtk_pe30_rdy(struct charger_manager *info)
{
	int vbat = 0;
	int ret = false;
	struct mtk_pe30 *pe3 = &info->pe3;

	if (pmic_is_bif_exist() == false)
		goto _fail;

	if (info->enable_pe_3 == false)
		goto _fail;

	if (pe3->is_pe30_done == true)
		goto _fail;

	if (pe3->pe30_charging_state != DC_STOP)
		goto _fail;

	if (mtk_cooler_is_abcct_unlimit() != 1)
		goto _fail;

	if (mtk_is_pep30_en_unlock() == true)
		pe3->is_vdm_rdy = true;
	else {
		pe3->is_vdm_rdy = false;
		goto _fail;
	}

	vbat = pmic_get_battery_voltage();
	if (vbat < info->data.bat_upper_bound && vbat > info->data.bat_lower_bound)
		ret = true;

	pr_err(
		"[is_mtk_pe30_rdy]pe3:%d:%d vbat=%d max/min=%d %d ret=%d is_pe30_done:%d is_pd_rdy:%d is_vdm_rdy:%d %d th:%d bif:%d\n",
		info->enable_pe_3, (info->dc_chg == NULL),
		vbat, info->data.bat_upper_bound, info->data.bat_lower_bound, ret, pe3->is_pe30_done,
		mtk_is_pd_chg_ready(),
		pe3->is_vdm_rdy, mtk_is_pep30_en_unlock(), mtk_cooler_is_abcct_unlimit(), pmic_is_bif_exist());

	return ret;

_fail:

	pr_err(
		"[is_mtk_pe30_rdy]pe3:%d:%d vbat=%d max/min=%d %d ret=%d is_pe30_done:%d is_pd_rdy:%d is_vdm_rdy:%d %d th:%d bif:%d\n",
		info->enable_pe_3, (info->dc_chg == NULL),
		vbat, info->data.bat_upper_bound, info->data.bat_lower_bound, ret, pe3->is_pe30_done,
		mtk_is_pd_chg_ready(),
		pe3->is_vdm_rdy, mtk_is_pep30_en_unlock(), mtk_cooler_is_abcct_unlimit(), pmic_is_bif_exist());



	return ret;
}

static void mtk_pe30_start(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	mutex_lock(&pe3->pe30_mutex);
	if (wake_lock_active(&pe3->pe30_wakelock) == 0)
		wake_lock(&pe3->pe30_wakelock);

	pe3->charging_current_limit = info->data.cc_init;
	pe3->is_pe30_done = true;
	pe3->pe30_charging_state = DC_INIT;
	get_monotonic_boottime(&pe3->startTime);
	wake_up_pe30_thread(info);
	charger_dev_enable_chip(info->dc_chg, true);
	mutex_unlock(&pe3->pe30_mutex);
}

bool mtk_is_TA_support_pe30(struct charger_manager *info)
{
	return info->pe3.is_vdm_rdy;
}

bool mtk_is_pe30_running(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	if (pe3->pe30_charging_state == DC_STOP)
		return false;
		return true;
}

void mtk_pe30_set_charging_current_limit(struct charger_manager *info, int cur)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	WARN(1, "mtk_pe30_set_charging_current_limit cur:%d\n", cur);
	mutex_lock(&pe3->pe30_mutex);
	pe3->charging_current_limit = cur;
	mutex_unlock(&pe3->pe30_mutex);
}

int mtk_pe30_get_charging_current_limit(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	if (pe3->charging_current_limit_by_r < pe3->charging_current_limit) {
		pr_err("[mtk_pe30_get_charging_current_limit]limit:%d limit_r:%d\n",
			pe3->charging_current_limit, pe3->charging_current_limit_by_r);
		return pe3->charging_current_limit_by_r;
	}
	return pe3->charging_current_limit;
}

void mtk_pe30_plugout_reset(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	mutex_lock(&pe3->pe30_mutex);
	pr_err("[mtk_pe30_plugout_reset]state = %d\n", pe3->pe30_charging_state);

	if (pe3->pe30_charging_state != DC_STOP) {
		mtk_pe30_set_ta_cap(info, 3000, 5000);
		tcpm_set_direct_charge_en(pe3->tcpc, false);
		mtk_pe30_enable_direct_charge(info, false);
	}

	pe3->is_pe30_done = false;
	pe3->is_vdm_rdy = false;
	pe3->pe30_charging_state = DC_STOP;

	if (wake_lock_active(&pe3->pe30_wakelock) != 0)
		wake_unlock(&pe3->pe30_wakelock);

	mutex_unlock(&pe3->pe30_mutex);
	mtk_pe30_set_charging_current_limit(info, info->data.cc_init);

	_wake_up_charger(info);
}

void mtk_pe30_end(struct charger_manager *info)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	mutex_lock(&pe3->pe30_mutex);
	pr_err("[mtk_pe30_end]state:%d\n",
		pe3->pe30_charging_state);

	if (pe3->pe30_charging_state != DC_STOP) {
		mtk_pe30_set_ta_cap(info, 3000, 5000);
		tcpm_set_direct_charge_en(pe3->tcpc, false);
		mtk_pe30_enable_direct_charge(info, false);
		pe3->pe30_charging_state = DC_STOP;
		_wake_up_charger(info);
		charger_dev_enable_chip(info->dc_chg, false);
		if (wake_lock_active(&pe3->pe30_wakelock) != 0)
			wake_unlock(&pe3->pe30_wakelock);
	}
	mutex_unlock(&pe3->pe30_mutex);

}

bool mtk_pe30_get_is_enable(struct charger_manager *pinfo)
{
	return !pinfo->pe3.is_pe30_done;
}

void mtk_pe30_set_is_enable(struct charger_manager *pinfo, bool enable)
{
	struct mtk_pe30 *pe3 = &pinfo->pe3;

	mutex_lock(&pe3->pe30_mutex);
	if (enable == true)
		pinfo->pe3.is_pe30_done = false;
	else
		pinfo->pe3.is_pe30_done = true;
	mutex_unlock(&pe3->pe30_mutex);
}

bool mtk_pe30_check_charger(struct charger_manager *info)
{
	if (is_mtk_pe30_rdy(info)) {
		charger_dev_enable(info->chg1_dev, false);
		mtk_pe30_start(info);
		return true;
	}
	return false;
}

static void _mtk_pe30_end(struct charger_manager *info, bool reset)
{
	struct mtk_pe30 *pe3 = &info->pe3;

	pr_err("[_mtk_pe30_end]state:%d reset:%d\n",
		pe3->pe30_charging_state, reset);

	if (pe3->pe30_charging_state != DC_STOP) {
		mtk_pe30_set_ta_cap(info, 3000, 5000);
		tcpm_set_direct_charge_en(pe3->tcpc, false);
		mtk_pe30_enable_direct_charge(info, false);
		pe3->pe30_charging_state = DC_STOP;
		if (reset == true)
			mtk_pe30_ta_hard_reset(info);
		_wake_up_charger(info);
		charger_dev_enable_chip(info->dc_chg, false);
		if (wake_lock_active(&pe3->pe30_wakelock) != 0)
			wake_unlock(&pe3->pe30_wakelock);
	}
}

static enum hrtimer_restart mtk_charger_pe30_timer_callback(struct hrtimer *timer)
{
	struct mtk_pe30 *pe3 = container_of(timer, struct mtk_pe30, mtk_charger_pe30_timer);

	pe3->mtk_charger_pe30_thread_flag = true;
	wake_up_interruptible(&pe3->mtk_charger_pe30_thread_waiter);

	return HRTIMER_NORESTART;
}

static void mtk_pe30_DC_init(struct charger_manager *info)
{
	int vbat;
	int i;
	int avgcur = 0;
	struct mtk_pe30_cap cap_now;
	unsigned int inputcur = 0;
	signed int fgcur = 0;
	bool sign;
	bool reset = true;
	int ret = 0;
	struct mtk_pe30 *pe3 = &info->pe3;

	tcpm_set_direct_charge_en(pe3->tcpc, true);

	vbat = pmic_get_battery_voltage();

	pr_err("[mtk_pe30_DC_init]state = %d ,vbat = %d\n", pe3->pe30_charging_state, vbat);

	charger_dev_enable_powerpath(info->chg1_dev, false);
	msleep(500);

	charger_dev_get_ibus(info->dc_chg, &inputcur);
	fgcur = battery_get_bat_current();
	sign = battery_get_bat_current_sign();

	for (i = 0; i < 10; i++) {
		ret = mtk_pe30_get_ta_current_cap(info, &cap_now);
		if (ret != 0) {
			pr_err("[mtk_pe30_DC_init]err0 = %d\n", ret);
			pe30_chr_enable_power_path(info, true);
			goto _fail;
		}
		avgcur += cap_now.cur;
		pr_err("[mtk_pe30_DC_init]cur:%d vol:%d\n", cap_now.cur, cap_now.vol);
	}
	avgcur = avgcur / 10;

	pe3->pmic_vbus = pmic_get_vbus();
	pe3->TA_vbus = cap_now.vol;
	pe3->vbus_cali = pe3->TA_vbus - pe3->pmic_vbus;

	pr_err("[mtk_pe30_DC_init]FOD cur:%d vol:%d fgc:%d:%d inputcur:%d vbus:%d cali:%d\n",
		avgcur, cap_now.vol, sign, fgcur, inputcur, pe3->pmic_vbus, pe3->vbus_cali);

	pe30_chr_enable_power_path(info, true);

	if (avgcur > info->data.fod_current) {
		reset = false;
		goto _fail;
	}

	ret = mtk_enable_direct_charge(pe3->tcpc, true);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_init]err0 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pe30_set_ta_boundary_cap(info, info->data.cc_init, vbat + 50);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_init]err1 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pe30_set_ta_cap(info, info->data.cc_ss_init, vbat + 50);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_init]err2 = %d\n", ret);
		goto _fail;
	}

	msleep(info->data.cc_ss_blanking);

	ret = mtk_pe30_enable_direct_charge(info, true);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_init]enable fail = %d\n", ret);
		goto _fail;
	}
	pe3->pe30_charging_state = DC_MEASURE_R;
	pe3->ktime = ktime_set(0, BAT_MS_TO_NS(info->data.cc_ss_blanking));

	return;
_fail:
	_mtk_pe30_end(info, reset);

}

static void mtk_pe30_DC_measure_R(struct charger_manager *info)
{
	struct mtk_pe30_cap cap1, cap2;
	int vbus1, vbat1, vbus2, vbat2, bifvbat1 = 0, bifvbat2 = 0;
	int fgcur1, fgcur2;
	int ret;
	bool reset = true;
	unsigned char emark;
	struct mtk_pe30 *pe3 = &info->pe3;

	pr_err("[mtk_pe30_DC_measure_R]vbus = %d\n", pmic_get_vbus());

	/* set boundary */
	ret = mtk_pe30_set_ta_boundary_cap(info, info->data.cc_init, info->data.cv_limit);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_measure_R]err1 = %d\n", ret);
		goto _fail;
	}

	/* measure 1 */
	ret = mtk_pe30_set_ta_cap(info, 1500, info->data.cv_limit);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_measure_R]err2 = %d\n", ret);
		goto _fail;
	}
	msleep(300);

	vbus1 = pmic_get_vbus();
	vbat1 = pmic_get_battery_voltage();
	pmic_get_bif_battery_voltage(&bifvbat1);
	fgcur1 = battery_get_bat_current() / 10;
	ret = mtk_pe30_get_ta_current_cap(info, &cap1);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_measure_R]err3 = %d\n", ret);
		goto _fail;
	}

	if (bifvbat1 != 0 && bifvbat1 >= mtk_pe30_get_cv(info)) {
		pr_err("[mtk_pe30_DC_measure_R]end bifvbat1=%d cur:%d vol:%d\n",
		bifvbat1, cap1.cur, cap1.vol);
		reset = false;
		goto _fail;
	}

	/* measure 2 */
	ret = mtk_pe30_set_ta_cap(info, 2000, info->data.cv_limit);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_measure_R]err2 = %d\n", ret);
		goto _fail;
	}
	msleep(300);

	vbus2 = pmic_get_vbus();
	vbat2 = pmic_get_battery_voltage();
	pmic_get_bif_battery_voltage(&bifvbat2);
	fgcur2 = battery_get_bat_current() / 10;
	ret = mtk_pe30_get_ta_current_cap(info, &cap2);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_measure_R]err3 = %d\n", ret);
		goto _fail;
	}

	if (bifvbat2 != 0 && bifvbat2 >= mtk_pe30_get_cv(info)) {
		pr_err("[mtk_pe30_DC_measure_R]end bifvbat2=%d cur:%d vol:%d\n",
		bifvbat2, cap2.cur, cap2.vol);
		reset = false;
		goto _fail;
	}

	if (cap2.cur == cap1.cur)
		return;

	if (bifvbat1 != 0 && bifvbat2 != 0)
		pe3->r_vbat = abs((vbat2 - bifvbat2) - (vbat1 - bifvbat1)) * 1000 / abs(fgcur2 - fgcur1);

	pe3->r_sw = abs((vbus2 - vbus1) - (vbat2 - vbat1)) * 1000 / abs(cap2.cur - cap1.cur);
	pe3->r_cable_1 = abs((cap2.vol - cap1.vol)-(vbus2 - vbus1)) * 1000 / abs(cap2.cur - cap1.cur);

	pe3->r_cable = abs(cap2.vol - pe3->vbus_cali - vbus2) * 1000 / abs(cap2.cur);
	pe3->r_cable_2 = abs(cap1.vol - pe3->vbus_cali - vbus1) * 1000 / abs(cap1.cur);

	if (pe3->r_sw < info->data.r_sw_min)
		pe3->r_sw = info->data.r_sw_min;

	if (pe3->r_vbat < info->data.r_vbat_min)
		pe3->r_vbat = info->data.r_vbat_min;

	pe3->r_total = pe3->r_cable + pe3->r_sw + pe3->r_vbat;

	if (pe3->r_cable < info->data.cc_init_r)
		pe3->charging_current_limit_by_r = info->data.cc_init;
	else if (pe3->r_cable < info->data.cc_init_bad_cable1_r)
		pe3->charging_current_limit_by_r = info->data.cc_init_bad_cable1;
	else if (pe3->r_cable < info->data.cc_init_bad_cable2_r)
		pe3->charging_current_limit_by_r = info->data.cc_init_bad_cable2;
	else {
		pr_err("[mtk_pe30_DC_measure_R]r_cable:%d is too hight , end pe30\n", pe3->r_cable);

		pr_err(
		"[mtk_pe30_DC_measure_R]Tibus:%d %d Tvbus:%d %d vbus:%d %d vbat:%d %d bifvbat:%d %d fgcur:%d %d r_c:%d r_sw:%d r_vbat:%d %d\n",
		cap2.cur, cap1.cur, cap2.vol, cap1.vol, vbus2, vbus1, vbat2,
		vbat1, bifvbat2, bifvbat1, fgcur2, fgcur1, pe3->r_cable, pe3->r_sw, pe3->r_vbat,
		pe3->charging_current_limit_by_r);

		pr_err(
		"[mtk_pe30_DC_measure_R]r_cable(2A):%d r_cable1:%d r_cable2(1.5A):%d cali:%d\n",
		pe3->r_cable, pe3->r_cable_1, pe3->r_cable_2, pe3->vbus_cali);

		reset = false;
		goto _fail;
	}

	tcpm_get_cable_capability(pe3->tcpc, &emark);
#ifdef PE30_CHECK_EMARK
	if (emark == 2) {
		if (pe3->charging_current_limit_by_r > 5000) {
			pe3->charging_current_limit_by_r = 5000;
			pr_err("[mtk_pe30_DC_measure_R]emark:%d reset current:%d\n",
				emark, pe3->charging_current_limit_by_r);
		}
	} else if (emark == 1) {
		if (pe3->charging_current_limit_by_r > 3000) {
			pe3->charging_current_limit_by_r = 3000;
			pr_err("[mtk_pe30_DC_measure_R]emark:%d reset current:%d\n",
				emark, pe3->charging_current_limit_by_r);
		}
	} else {
		reset = false;
		pr_err("[mtk_pe30_DC_measure_R]emark:%d end pe30\n", emark);
		goto _fail;
	}
#endif

	pr_err(
	"[mtk_pe30_DC_measure_R]Tibus:%d %d Tvbus:%d %d vbus:%d %d vbat:%d %d bifvbat:%d %d fgcur:%d %d r_c:%d r_sw:%d r_vbat:%d %d\n",
	cap2.cur, cap1.cur, cap2.vol, cap1.vol, vbus2, vbus1, vbat2,
	vbat1, bifvbat2, bifvbat1, fgcur2, fgcur1, pe3->r_cable, pe3->r_sw,
	pe3->r_vbat, pe3->charging_current_limit_by_r);

	pr_err(
	"[mtk_pe30_DC_measure_R]r_cable(2A):%d r_cable1:%d r_cable2(1.5A):%d cali:%d\n",
	pe3->r_cable, pe3->r_cable_1, pe3->r_cable_2, pe3->vbus_cali);

	pe3->pe30_charging_state = DC_SOFT_START;

	return;
_fail:
	_mtk_pe30_end(info, reset);

}


static void mtk_pe30_DC_soft_start(struct charger_manager *info)
{
	int ret;
	int vbat;
	int cv;
	struct mtk_pe30_cap setting, cap_now;
	int current_setting;
	bool reset = true;
	struct mtk_pe30 *pe3 = &info->pe3;


	pr_err("[mtk_pe30_DC_soft_start]vbus = %d\n", pmic_get_vbus());

	ret = mtk_pe30_get_ta_current_cap(info, &cap_now);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_soft_start]err1 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pe30_get_ta_cap(info, &setting);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_soft_start]err2 = %d\n", ret);
		goto _fail;
	}

	current_setting = setting.cur;

	vbat = pmic_get_battery_voltage();

	if (vbat >= mtk_pe30_get_cv(info)) {

		if (cap_now.cur >= info->data.cc_end) {
			current_setting = current_setting - info->data.cc_ss_step;
			cv = mtk_pe30_get_cali_vbus(info, current_setting);
			if (cv >= info->data.cv_limit)
				cv = info->data.cv_limit;
			ret = mtk_pe30_set_ta_cap(info, current_setting, cv);
			if (ret != 0) {
				pr_err("[mtk_pe30_DC_soft_start]err6 = %d\n", ret);
				goto _fail;
			}

			pr_err(
				"[mtk_pe30_DC_soft_start]go to CV vbat:%d current:%d setting current:%d\n",
				vbat, cap_now.cur, current_setting);
			pe3->pe30_charging_state = DC_CC;
			pe3->ktime = ktime_set(0, BAT_MS_TO_NS(info->data.cc_blanking));
			return;
		}
		pr_err("[mtk_pe30_DC_soft_start]end vbat:%d current:%d setting current:%d\n",
			vbat, cap_now.cur, current_setting);
		reset = false;
		goto _fail;

	}

	if (setting.cur >= info->data.cc_init) {
		pe3->pe30_charging_state = DC_CC;
		pr_err("[mtk_pe30_DC_soft_start]go to CV2 vbat:%d current:%d setting current:%d\n",
			vbat, cap_now.cur, current_setting);
		pe3->ktime = ktime_set(0, BAT_MS_TO_NS(info->data.cc_blanking));
		return;
	}

	if (vbat >= info->data.cv_ss_step2)
		current_setting = current_setting + info->data.cc_ss_step2;
	else if (vbat >= info->data.cv_ss_step3)
		current_setting = current_setting + info->data.cc_ss_step3;
	else
		current_setting = current_setting + info->data.cc_ss_step;

	cv = mtk_pe30_get_cali_vbus(info, current_setting);

	pr_err(
		"[mtk_pe30_DC_soft_start]vbat:%d cv = %d cur:%d r:%d cap_now.cur:%d last_cur:%d\n",
		vbat, cv, current_setting, pe3->r_total,
		cap_now.cur, setting.cur);

	ret = mtk_pe30_set_ta_boundary_cap(info, info->data.cc_init, info->data.cv_limit);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_soft_start]err4 = %d\n", ret);
		goto _fail;
	}

	if (cv >= info->data.cv_limit)
		cv = info->data.cv_limit;

	ret = mtk_pe30_set_ta_cap(info, current_setting, cv);


	pr_err("[mtk_pe30_DC_soft_start]cur = %d vol = %d\n", current_setting, cv);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_soft_start]err5 = %d\n", ret);
		goto _fail;
	}

	pr_err("[mtk_pe30_DC_soft_start]setting:%d %d now:%d %d boundary:%d %d cap:%d %d\n",
		setting.cur, setting.vol,
		cap_now.cur, cap_now.vol, info->data.cc_init, info->data.cv_limit, current_setting, cv);


	return;
_fail:
	_mtk_pe30_end(info, reset);


}

static void mtk_pe30_DC_cc(struct charger_manager *info)
{
	struct mtk_pe30_cap setting, cap_now;
	int vbat = 0;
	int ret;
	int TAVbus, current_setting, cv;
	int tmp;
	int tmp_min = 0, tmp_max = 0;
	bool reset = true;
	struct mtk_pe30 *pe3 = &info->pe3;

	pr_err("[mtk_pe30_DC_CC]state = %d\n", pe3->pe30_charging_state);

	ret = pmic_get_bif_battery_voltage(&vbat);
	if (ret < 0)
		vbat = pmic_get_battery_voltage();

	ret = mtk_pe30_get_ta_current_cap(info, &cap_now);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_CC]err1 = %d\n", ret);
		goto _fail;
	}

	if (cap_now.cur <= info->data.cc_end) {
		if (pe3->charging_current_limit < info->data.cc_end) {
			pr_err(
				"[mtk_pe30_DC_cc]cur<%d , reset is_pe30_done\n", info->data.cc_end);
			pe3->is_pe30_done = false;
		}
		pr_err("[mtk_pe30_DC_CC]pe30 end , current:%d\n", cap_now.cur);
		reset = false;
		goto _fail;
	}

	ret = mtk_pe30_get_ta_cap(info, &setting);
	if (ret != 0) {
		pr_err("[mtk_pe30_DC_CC]err2 = %d\n", ret);
		goto _fail;
	}

	current_setting = setting.cur;
	tmp = battery_meter_get_battery_temperature();

	ret = charger_dev_get_temperature(info->dc_chg, &tmp_min, &tmp_max);
	if (ret < 0) {
		pr_err("[mtk_pe30_DC_CC]err3 = %d\n", ret);
		goto _fail;
	}

	cv = mtk_pe30_get_cv(info);
	if (vbat >= cv || tmp_max >= info->data.charger_temp_max) {

		current_setting -= info->data.cc_step;
		TAVbus = mtk_pe30_get_cali_vbus(info, current_setting);

		if (TAVbus >= info->data.cv_limit)
			TAVbus = info->data.cv_limit;

		if (TAVbus <= 5500)
			TAVbus = 5500;

		ret = mtk_pe30_set_ta_boundary_cap(info, info->data.cc_init, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pe30_DC_CC]err6 = %d\n", ret);
			goto _fail;
		}

		if (current_setting >= mtk_pe30_get_charging_current_limit(info)) {
			pr_err("[mtk_pe30_DC_cc]over limit target:%d limit:%d\n",
			current_setting, mtk_pe30_get_charging_current_limit(info));
			current_setting = mtk_pe30_get_charging_current_limit(info);
		}

		ret = mtk_pe30_set_ta_cap(info, current_setting, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pe30_DC_CC]err7 = %d\n", ret);
			goto _fail;
		}

		pr_err("[mtk_pe30_DC_CC]- vbat:%d current:%d:%d r:%d TAVBUS:%d cv:%d tmp_max:%d\n",
			vbat, current_setting, mtk_pe30_get_charging_current_limit(info), pe3->r_total,
			TAVbus, cv, tmp_max);
	} else {

		current_setting += CC_STEP;
		TAVbus = mtk_pe30_get_cali_vbus(info, current_setting);

		if (TAVbus >= info->data.cv_limit)
			TAVbus = info->data.cv_limit;

		if (TAVbus <= 5500)
			TAVbus = 5500;

		ret = mtk_pe30_set_ta_boundary_cap(info, info->data.cc_init, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pe30_DC_CC]err8 = %d\n", ret);
			goto _fail;
		}

		if (current_setting >= mtk_pe30_get_charging_current_limit(info)) {
			pr_err("[mtk_pe30_DC_cc]over limit target:%d limit:%d\n",
			current_setting, mtk_pe30_get_charging_current_limit(info));
			current_setting = mtk_pe30_get_charging_current_limit(info);
		}

		ret = mtk_pe30_set_ta_cap(info, current_setting, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pe30_DC_CC]err9 = %d\n", ret);
			goto _fail;
		}

		pr_err("[mtk_pe30_DC_CC]+ vbat:%d current:%d:%d r:%d TAVBUS:%d cv:%d\n",
			vbat, current_setting, mtk_pe30_get_charging_current_limit(info), pe3->r_total, TAVbus, cv);
	}

	return;

_fail:
	_mtk_pe30_end(info, reset);

}

bool mtk_pe30_safety_check(struct charger_manager *info)
{
	int vbus = 0, vbus_cv, bifvbat = 0;
	int pmic_vbat = 0, pmic_vbat_ovp;
	struct mtk_pe30_cap cap, setting, cap_b, tmp;
	int ret;
	signed int cur;
	bool sign;
	unsigned int chrCur = 0;
	unsigned char emark = 0;
	int biftmp = 0;
	bool reset = true;
	struct mtk_pe30 *pe3 = &info->pe3;

	setting.vol = 0;
	setting.cur = 0;
	cap.vol = 0;
	cap.cur = 0;
	tmp.temp = 0;
	cur = battery_get_bat_current();
	sign = battery_get_bat_current_sign();
	ret = mtk_pe30_get_ta_boundary_cap(info, &cap_b);
	if (ret < 0) {
		pr_err("[%s]get ta boundary cap fail ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	if (mtk_pe30_get_ta_charger_status(info) == false) {
		pr_err("[%s]get ta status ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	/*vbus ov: auxadc by PMIC */
	vbus = pmic_get_vbus();
	if (pe3->pe30_charging_state != DC_MEASURE_R && pe3->pe30_charging_state != DC_INIT) {
		vbus_cv = info->data.bat_upper_bound + 70 + 110 * info->data.cc_init / 100 * pe3->r_cable;
		if (vbus > vbus_cv) {
			pr_err("[%s]%d vbus %d > vbus_cv %d , r_cable:%d end pe30\n",
			__func__, pe3->pe30_charging_state, vbus, vbus_cv, pe3->r_cable);
			reset = false;
			goto _fail;
		}
	}

	/* ibus oc: RT9468 current */
	ret = charger_dev_get_ibus(info->dc_chg, &chrCur);
	if (ret < 0) {
		pr_err("[%s]get current from chr fail ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	if (chrCur >= (info->data.cc_max / 1000)) {
		pr_err("[%s]current is too high from chr :%d\n",
		__func__, chrCur);
		goto _fail;
	}

	/* ibus oc:TA ADC current*/
	mtk_pe30_get_ta_current_cap(info, &cap);
	mtk_pe30_get_ta_cap(info, &setting);

	if (cap.cur >= (info->data.cc_max / 1000)) {
		pr_err("[%s]cur %d >= CC_MAX %d , end pe30\n", __func__, cap.cur, (info->data.cc_max / 1000));
		goto _fail;
	}

#ifdef PE30_CHECK_TA
	if ((cap.vol - setting.vol) > info->data.vbus_ov_gap) {
		pr_err("[%s]vbus:%d > vbuus_b:%d gap:%d, end pe30\n", __func__, cap.vol,
		setting.vol, VBUS_OV_GAP);
		goto _fail;
	}
#endif


	/*vbat ovp*/
	pmic_vbat = pmic_get_battery_voltage();
	ret = pmic_get_bif_battery_voltage(&bifvbat);
	if (pe3->pe30_charging_state != DC_MEASURE_R && pe3->pe30_charging_state != DC_INIT) {
		if (ret >= 0)
			pmic_vbat_ovp = info->data.bat_upper_bound + 50 + 110 *
			pe3->current_cap.cur / 100 * pe3->r_vbat;
		else
			pmic_vbat_ovp = info->data.bat_upper_bound + 50;
		/*pmic adc vbat */
		if (pmic_vbat > pmic_vbat_ovp) {
			pr_err("[%s]pmic_vbat:%d pmic_vbat_ovp:%d vbatCv:%d current:%d r_vbat:%d\n",
			__func__, pmic_vbat, pmic_vbat_ovp, info->data.bat_upper_bound,
			pe3->current_cap.cur, pe3->r_vbat);
			goto _fail;
		}

		/*bif adc vbat */
		if (ret >= 0 && bifvbat >= info->data.bat_upper_bound + 50) {
			pr_err("[%s]bifvbat %d >= BAT_UPPER_BOUND:%d + 50\n",
			__func__, bifvbat, info->data.bat_upper_bound + 50);
			goto _fail;
		}
	}

	if (battery_is_battery_exist() == false) {
		pr_err("[%s]no battery, end pe30\n",
		__func__);
		reset = false;
		goto _fail;
	}

	if (pe3->batteryTemperature >= info->data.battery_temp_max) {
		pr_err("[%s]battery temperature is too high %d, end pe30\n",
		__func__, pe3->batteryTemperature);
		reset = false;
		goto _fail;
	}

	if (pe3->batteryTemperature <= info->data.battery_temp_min) {
		pr_err("[%s]battery temperature is too low %d, end pe30\n",
		__func__, pe3->batteryTemperature);
		reset = false;
		goto _fail;
	}

	/* TA thermal*/
	mtk_pe30_get_ta_tmp_wdt(info, &tmp);
	if (tmp.temp >= info->data.ta_temp_max) {
		pr_err("[%s]TA temperature is too high %d, end pe30\n",
		__func__, tmp.temp);
		goto _fail;
	}

	charger_dev_kick_direct_charging_wdt(info->dc_chg);
	tcpm_get_cable_capability(pe3->tcpc, &emark);

	pmic_get_bif_battery_temperature(&biftmp);

	pr_err(
	"fg:%d:%d:%d:dif:%d:[cur]:%d:%d:%d:[vol]:%d:%d:vbus:%d:vbat:%d:bifvbat:%d:soc:%d:%d:tmp:%d:%d:%d:b:%d:%d:m:%d:%d:%d\n",
		pe3->pe30_charging_state, sign, cur, cap.cur - cur/10,
		cap.cur, setting.cur, chrCur,
		cap.vol, setting.vol, vbus, pmic_vbat, bifvbat,
		battery_get_bat_soc(), battery_get_bat_uisoc(),
		pe3->batteryTemperature, biftmp, tmp.temp, cap_b.cur, cap_b.vol,
		emark, pe3->charging_current_limit, pe3->charging_current_limit_by_r);

	if (pe3->charging_current_limit < info->data.cc_end) {
		pr_err(
			"[mtk_pe30_set_charging_current_limit]cur<%d , reset is_pe30_done\n", info->data.cc_end);
		_mtk_pe30_end(info, false);
		pe3->is_pe30_done = false;
	}

	return true;

_fail:

	pr_err(
	"fg:%d:%d:%d:dif:%d:[cur]:%d:%d:%d:[vol]:%d:%d:vbus:%d:vbat:%d:bifvbat:%d:soc:%d:uisoc2:%d:tmp:%d:%d:b:%d:%d:m:%d\n",
		pe3->pe30_charging_state, sign, cur, cap.cur - cur/10,
		cap.cur, setting.cur, chrCur,
		cap.vol, setting.vol, vbus, pmic_vbat, bifvbat,
		battery_get_bat_soc(), battery_get_bat_uisoc(),
		pe3->batteryTemperature, tmp.temp, cap_b.cur, cap_b.vol,
		emark);

	_mtk_pe30_end(info, reset);

	return false;
}

static int mtk_charger_pe30_thread_handler(void *arg)
{
	struct timespec time;
	struct charger_manager *info = arg;
	struct mtk_pe30 *pe3 = &info->pe3;


	pe3->ktime = ktime_set(0, BAT_MS_TO_NS(500));
	do {
		wait_event_interruptible(pe3->mtk_charger_pe30_thread_waiter,
					 (pe3->mtk_charger_pe30_thread_flag == true));

		pe3->mtk_charger_pe30_thread_flag = false;

		mutex_lock(&pe3->pe30_mutex);
		get_monotonic_boottime(&pe3->endTime);
		time = timespec_sub(pe3->endTime, pe3->startTime);

		pe3->batteryTemperature = battery_meter_get_battery_temperature();

		pr_err("[mtk_charger_pe30_thread_handler]state = %d tmp:%d time:%d wakelock:%d\n",
		pe3->pe30_charging_state, pe3->batteryTemperature, (int)time.tv_sec,
		wake_lock_active(&pe3->pe30_wakelock));

		if (time.tv_sec >= info->data.pe30_max_charging_time) {
			pr_err("[mtk_charger_pe30_thread_handler]pe30 charging timeout %d:%d\n",
			(int)time.tv_sec, info->data.pe30_max_charging_time);
			_mtk_pe30_end(info, false);
		}

		switch (pe3->pe30_charging_state) {
		case DC_INIT:
			mtk_pe30_DC_init(info);
			break;

		case DC_MEASURE_R:
			mtk_pe30_DC_measure_R(info);
			break;

		case DC_SOFT_START:
			mtk_pe30_DC_soft_start(info);
			break;

		case DC_CC:
			mtk_pe30_DC_cc(info);
			break;

		case DC_STOP:
			break;
		}

		/*charger_dev_dump_registers(info->dc_chg);*/

		if (pe3->pe30_charging_state != DC_STOP) {
			mtk_pe30_safety_check(info);
			hrtimer_start(&pe3->mtk_charger_pe30_timer, pe3->ktime, HRTIMER_MODE_REL);
		}
		mutex_unlock(&pe3->pe30_mutex);
	} while (!kthread_should_stop());

	return 0;
}

int mtk_pe30_parse_dt(struct charger_manager *info, struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 val;

	pr_err("%s: starts\n", __func__);

	if (!np) {
		pr_err("%s: no device node\n", __func__);
		return -EINVAL;
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
		info->data.cc_init_r = val;
	} else {
		pr_err(
			"use default CC_INIT_R:%d\n",
			CC_INIT_R);
		info->data.cc_init_r = CC_INIT_R;
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

	if (of_property_read_u32(np, "cc_normal", &val) >= 0) {
		info->data.cc_normal = val;
	} else {
		pr_err(
			"use default CC_NORMAL:%d\n",
			CC_NORMAL);
		info->data.cc_normal = CC_NORMAL;
	}

	if (of_property_read_u32(np, "cc_max", &val) >= 0) {
		info->data.cc_max = val;
	} else {
		pr_err(
			"use default CC_MAX:%d\n",
			CC_MAX);
		info->data.cc_max = CC_MAX;
	}

	if (of_property_read_u32(np, "cc_end", &val) >= 0) {
		info->data.cc_end = val;
	} else {
		pr_err(
			"use default CC_END:%d\n",
			CC_END);
		info->data.cc_end = CC_END;
	}

	if (of_property_read_u32(np, "cc_step", &val) >= 0) {
		info->data.cc_step = val;
	} else {
		pr_err(
			"use default CC_STEP:%d\n",
			CC_STEP);
		info->data.cc_step = CC_STEP;
	}

	if (of_property_read_u32(np, "cc_ss_step", &val) >= 0) {
		info->data.cc_ss_step = val;
	} else {
		pr_err(
			"use default CC_SS_STEP:%d\n",
			CC_SS_STEP);
		info->data.cc_ss_step = CC_SS_STEP;
	}

	if (of_property_read_u32(np, "cc_ss_step2", &val) >= 0) {
		info->data.cc_ss_step2 = val;
	} else {
		pr_err(
			"use default CC_SS_STEP2:%d\n",
			CC_SS_STEP2);
		info->data.cc_ss_step2 = CC_SS_STEP2;
	}

	if (of_property_read_u32(np, "cc_ss_step3", &val) >= 0) {
		info->data.cc_ss_step3 = val;
	} else {
		pr_err(
			"use default CC_SS_STEP3:%d\n",
			CC_SS_STEP3);
		info->data.cc_ss_step3 = CC_SS_STEP3;
	}

	if (of_property_read_u32(np, "cv_ss_step2", &val) >= 0) {
		info->data.cv_ss_step2 = val;
	} else {
		pr_err(
			"use default CV_SS_STEP2:%d\n",
			CV_SS_STEP2);
		info->data.cv_ss_step2 = CV_SS_STEP2;
	}

	if (of_property_read_u32(np, "cv_ss_step3", &val) >= 0) {
		info->data.cv_ss_step3 = val;
	} else {
		pr_err(
			"use default CV_SS_STEP3:%d\n",
			CV_SS_STEP3);
		info->data.cv_ss_step3 = CV_SS_STEP3;
	}

	if (of_property_read_u32(np, "cc_ss_blanking", &val) >= 0) {
		info->data.cc_ss_blanking = val;
	} else {
		pr_err(
			"use default CC_SS_BLANKING:%d\n",
			CC_SS_BLANKING);
		info->data.cc_ss_blanking = CC_SS_BLANKING;
	}

	if (of_property_read_u32(np, "cc_blanking", &val) >= 0) {
		info->data.cc_blanking = val;
	} else {
		pr_err(
			"use default CC_BLANKING:%d\n",
			CC_BLANKING);
		info->data.cc_blanking = CC_BLANKING;
	}

	if (of_property_read_u32(np, "charger_temp_max", &val) >= 0) {
		info->data.charger_temp_max = val;
	} else {
		pr_err(
			"use default CHARGER_TEMP_MAX:%d\n",
			CHARGER_TEMP_MAX);
		info->data.charger_temp_max = CHARGER_TEMP_MAX;
	}

	if (of_property_read_u32(np, "ta_temp_max", &val) >= 0) {
		info->data.ta_temp_max = val;
	} else {
		pr_err(
			"use default TA_TEMP_MAX:%d\n",
			TA_TEMP_MAX);
		info->data.ta_temp_max = TA_TEMP_MAX;
	}

	if (of_property_read_u32(np, "vbus_ov_gap", &val) >= 0) {
		info->data.vbus_ov_gap = val;
	} else {
		pr_err(
			"use default VBUS_OV_GAP:%d\n",
			VBUS_OV_GAP);
		info->data.vbus_ov_gap = VBUS_OV_GAP;
	}

	if (of_property_read_u32(np, "fod_current", &val) >= 0) {
		info->data.fod_current = val;
	} else {
		pr_err(
			"use default FOD_CURRENT:%d\n",
			FOD_CURRENT);
		info->data.fod_current = FOD_CURRENT;
	}

	if (of_property_read_u32(np, "r_vbat_min", &val) >= 0) {
		info->data.r_vbat_min = val;
	} else {
		pr_err(
			"use default R_VBAT_MIN:%d\n",
			R_VBAT_MIN);
		info->data.r_vbat_min = R_VBAT_MIN;
	}

	if (of_property_read_u32(np, "r_sw_min", &val) >= 0) {
		info->data.r_sw_min = val;
	} else {
		pr_err(
			"use default R_SW_MIN:%d\n",
			R_SW_MIN);
		info->data.r_sw_min = R_SW_MIN;
	}

	if (of_property_read_u32(np, "pe30_max_charging_time", &val) >= 0) {
		info->data.pe30_max_charging_time = val;
	} else {
		pr_err(
			"use default PE30_MAX_CHARGING_TIME:%d\n",
			PE30_MAX_CHARGING_TIME);
		info->data.pe30_max_charging_time = PE30_MAX_CHARGING_TIME;
	}

	if (info->enable_sw_jeita == false) {
		if (of_property_read_u32(np, "battery_temp_min", &val) >= 0) {
			info->data.battery_temp_min = val;
		} else {
			pr_err(
				"use default BATTERY_TEMP_MIN:%d\n",
				BATTERY_TEMP_MIN);
			info->data.battery_temp_min = BATTERY_TEMP_MIN;
		}

		if (of_property_read_u32(np, "battery_temp_max", &val) >= 0) {
			info->data.battery_temp_max = val;
		} else {
			pr_err(
				"use default BATTERY_TEMP_MAX:%d\n",
				BATTERY_TEMP_MAX);
			info->data.battery_temp_max = BATTERY_TEMP_MAX;
		}
	} else {
		info->data.battery_temp_min = info->data.temp_t2_threshold;
		info->data.battery_temp_max = info->data.temp_t3_threshold;
	}

	return 0;
}

bool mtk_pe30_init(struct charger_manager *info)
{
	ktime_t ktime;
	struct mtk_pe30 *pe3 = &info->pe3;

	mutex_init(&pe3->pe30_mutex);
	init_waitqueue_head(&pe3->mtk_charger_pe30_thread_waiter);

	mtk_pe30_parse_dt(info, &info->pdev->dev);

	pe3->pe30_charging_state = DC_STOP;
	pe3->batteryTemperature = -1000;
	pe3->charging_current_limit = info->data.cc_init;
	pe3->charging_current_limit_by_r = info->data.cc_init;
	ktime = ktime_set(0, BAT_MS_TO_NS(2000));

	wake_lock_init(&pe3->pe30_wakelock, WAKE_LOCK_SUSPEND, "pe30 wakelock");
	info->dc_chg = get_charger_by_name("load_switch");
	if (info->dc_chg == NULL)
		return false;

	hrtimer_init(&pe3->mtk_charger_pe30_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pe3->mtk_charger_pe30_timer.function = mtk_charger_pe30_timer_callback;
	pe3->mtk_charger_pe30_thread =
	    kthread_run(mtk_charger_pe30_thread_handler, info,
			"charger_pe30");
	if (IS_ERR(pe3->mtk_charger_pe30_thread)) {
		pr_err(
			    "[%s]: failed to create mtk_charger_pe30 thread\n",
			    __func__);
		return false;
	}

	pr_err("mtk_charger_pe30 : done\n");

	return true;

}

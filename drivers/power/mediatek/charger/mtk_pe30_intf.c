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

#include <mtk_direct_charge_vdm.h>
#include "mtk_charger_intf.h"

#define  DC_INIT                        (0x00)
#define  DC_MEASURE_R					(0x01)
#define  DC_SOFT_START                  (0x02)
#define  DC_CC							(0x03)
#define  DC_STOP	                    (0x04)


#define BAT_MS_TO_NS(x) (x * 1000 * 1000)


static DEFINE_MUTEX(pe30_mutex);


static struct hrtimer mtk_charger_pep30_timer;
static struct task_struct *mtk_charger_pep30_thread;
static bool mtk_charger_pep30_thread_flag;
static DECLARE_WAIT_QUEUE_HEAD(mtk_charger_pep30_thread_waiter);

static bool is_pep30_done; /* one plug in only do pep30 once */
static int pep30_charging_state = DC_STOP;
static struct tcpc_device *tcpc;
static ktime_t ktime;
static int r_total, r_cable, r_sw, r_vbat;
static int batteryTemperature = -1000;
static int r_cable_1, r_cable_2;

static bool is_vdm_rdy;

struct mtk_vdm_ta_cap current_cap;

static int charging_current_limit = CC_INIT;
static int charging_current_limit_by_r = CC_INIT;

static struct timespec startTime, endTime;

static int mtk_pep30_set_ta_boundary_cap(int cur, int voltage)
{
	struct mtk_vdm_ta_cap cap;
	int ret = 0, cnt = 0;

	cap.cur = cur;
	cap.vol = voltage;

	do {
		ret = mtk_set_ta_boundary_cap(tcpc, &cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	return ret;
}

static int mtk_pep30_get_ta_boundary_cap(struct mtk_pep30_cap *cap)
{
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_boundary_cap(tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pep30_set_ta_cap(int cur, int voltage)
{
	int ret = 0, cnt = 0;

	current_cap.cur = cur;
	current_cap.vol = voltage;

	do {
		ret = mtk_set_ta_cap(tcpc, &current_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	return ret;
}

static int mtk_pep30_get_ta_cap(struct mtk_pep30_cap *cap)
{
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_cap(tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pep30_get_ta_current_cap(struct mtk_pep30_cap *cap)
{
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_current_cap(tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;

	return ret;
}

static int mtk_pep30_get_ta_tmp_wdt(struct mtk_pep30_cap *cap)
{
	struct mtk_vdm_ta_cap ta_cap;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_monitor_ta_inform(tcpc, &ta_cap);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	cap->cur = ta_cap.cur;
	cap->vol = ta_cap.vol;
	cap->temp = ta_cap.temp;

	return ret;
}

static int mtk_pep30_enable_direct_charge(bool en)
{
	int ret = 0, ret2 = 0;
	struct mtk_pep30_cap cap;
	struct pd_ta_stat pdstat;

	if (en == true) {
		mtk_chr_set_direct_charge_ibus_oc(CC_MAX);
		ret = mtk_enable_direct_charge(tcpc, en);
		if (ret != 0)
			ret2 = ret;
		mtk_pep30_get_ta_tmp_wdt(&cap);
		ret = mtk_get_ta_charger_status(tcpc, &pdstat);
		if ((ret == MTK_VDM_SUCCESS) && pdstat.ping_chk_fail)
			mtk_clr_ta_pingcheck_fault(tcpc);
		ret = mtk_enable_ta_dplus_dect(tcpc, true, 4000);
		if (ret != 0)
			ret2 = ret;
	} else {
		mtk_chr_set_direct_charge_ibus_oc(CC_NORMAL);
		ret = mtk_enable_direct_charge(tcpc, en);
		if (ret != 0)
			ret2 = ret;
		ret = mtk_enable_ta_dplus_dect(tcpc, false, 4000);
		if (ret != 0)
			ret2 = ret;
	}
	mtk_chr_enable_direct_charge(en);

	return ret2;
}

int mtk_pep30_ta_hard_reset(void)
{
	int ret = 0, cnt = 0;

	do {
		ret = tcpm_hard_reset(tcpc);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);


	return ret;
}

static bool mtk_pep30_get_ta_charger_status(void)
{
	struct pd_ta_stat pdstat;
	int ret = 0, cnt = 0;

	do {
		ret = mtk_get_ta_charger_status(tcpc, &pdstat);
		cnt++;
		if (ret == 1)
			msleep(100);
	} while (ret != 0 && cnt <= 3);

	if (ret != 0)
		goto _fail;

	if (pdstat.ovp == 1 || pdstat.otp == 1 || pdstat.uvp == 1 || pdstat.rvs_cur == 1
		|| pdstat.ping_chk_fail == 1)
		goto _fail;


	pr_err("[mtk_pep30_get_ta_charger_status]%d %d %d %d %d %d %d %d %d %d\n",
		ret,
		pdstat.chg_mode, pdstat.dc_en, pdstat.dpc_en, pdstat.pc_en,
		pdstat.ovp, pdstat.otp, pdstat.uvp,
		pdstat.rvs_cur, pdstat.ping_chk_fail);

	return true;

_fail:

	pr_err("[mtk_pep30_get_ta_charger_status]fail %d %d %d %d %d %d %d %d %d %d\n",
		ret,
		pdstat.chg_mode, pdstat.dc_en, pdstat.dpc_en, pdstat.pc_en,
		pdstat.ovp, pdstat.otp, pdstat.uvp,
		pdstat.rvs_cur, pdstat.ping_chk_fail);

	return false;

}

static void wake_up_pep30_thread(void)
{
	mtk_charger_pep30_thread_flag = true;
	wake_up_interruptible(&mtk_charger_pep30_thread_waiter);

	pr_err("[wake_up_pep30_thread]\n");
}

static int mtk_pep30_get_cv(void)
{
	int cv = BAT_UPPER_BOUND;

	return cv;
}

static int mtk_pep30_get_cali_vbus(int charging_current)
{
	int vbus = mtk_pep30_get_cv() + 70 + 110 * charging_current / 100 * r_total / 1000;

	return vbus;
}

static bool is_mtk_pep30_rdy(void)
{
	int vbat = 0;
	int ret = false;

	if (is_pep30_done == true)
		goto _fail;

	if (pep30_charging_state != DC_STOP)
		goto _fail;

	if (mtk_cooler_is_abcct_unlimit() != 1)
		goto _fail;

	if (mtk_is_pep30_en_unlock() == true)
		is_vdm_rdy = true;
	else {
		is_vdm_rdy = false;
		goto _fail;
	}

	vbat = pmic_get_battery_voltage();
	if (vbat < BAT_UPPER_BOUND && vbat > BAT_LOWER_BOUND)
		ret = true;

	pr_err(
		"[is_mtk_pep30_rdy]vbat=%d max/min=%d %d ret=%d is_pep30_done:%d is_pd_rdy:%d is_vdm_rdy:%d %d thermal:%d\n",
		vbat, BAT_UPPER_BOUND, BAT_LOWER_BOUND, ret, is_pep30_done, mtk_is_pd_chg_ready(), is_vdm_rdy,
		mtk_is_pep30_en_unlock(), mtk_cooler_is_abcct_unlimit());

	return ret;

_fail:

	pr_err(
	"[is_mtk_pep30_rdy]vbat=%d max/min=%d %d ret=%d is_pep30_done:%d is_pd_rdy:%d is_vdm_rdy:%d %d thermal:%d\n",
		vbat, BAT_UPPER_BOUND, BAT_LOWER_BOUND, ret, is_pep30_done, mtk_is_pd_chg_ready(), is_vdm_rdy,
		mtk_is_pep30_en_unlock(), mtk_cooler_is_abcct_unlimit());


	return ret;
}

static void mtk_pep30_start(void)
{
	mutex_lock(&pe30_mutex);
	charging_current_limit = CC_INIT;
	is_pep30_done = true;
	pep30_charging_state = DC_INIT;
	get_monotonic_boottime(&startTime);
	wake_up_pep30_thread();
	mutex_unlock(&pe30_mutex);

}

bool mtk_is_TA_support_pep30(void)
{
	return is_vdm_rdy;
}

bool mtk_is_pep30_running(void)
{
	if (pep30_charging_state == DC_STOP)
		return false;
		return true;
}

void mtk_pep30_set_charging_current_limit(int cur)
{
	WARN(1, "mtk_pep30_set_charging_current_limit cur:%d\n", cur);
	mutex_lock(&pe30_mutex);
	charging_current_limit = cur;
	mutex_unlock(&pe30_mutex);
}

int mtk_pep30_get_charging_current_limit(void)
{
	if (charging_current_limit_by_r < charging_current_limit) {
		pr_err("[mtk_pep30_get_charging_current_limit]limit:%d limit_r:%d\n",
			charging_current_limit, charging_current_limit_by_r);
		return charging_current_limit_by_r;
	}
	return charging_current_limit;
}

void mtk_pep30_plugout_reset(void)
{
	mutex_lock(&pe30_mutex);
	pr_err("[mtk_pep30_plugout_reset]state = %d\n", pep30_charging_state);

	if (pep30_charging_state != DC_STOP) {
		mtk_pep30_set_ta_cap(3000, 5000);
		tcpm_set_direct_charge_en(tcpc, false);
		mtk_pep30_enable_direct_charge(false);
	}

	is_pep30_done = false;
	is_vdm_rdy = false;
	pep30_charging_state = DC_STOP;
	mutex_unlock(&pe30_mutex);
	mtk_pep30_set_charging_current_limit(CC_INIT);

	wake_up_bat3();
}

static void _mtk_pep30_end(bool reset)
{
	mutex_lock(&pe30_mutex);

	pr_err("[_mtk_pep30_end]state:%d reset:%d\n",
		pep30_charging_state, reset);

	if (pep30_charging_state != DC_STOP) {
		mtk_pep30_set_ta_cap(3000, 5000);
		tcpm_set_direct_charge_en(tcpc, false);
		mtk_pep30_enable_direct_charge(false);
		pep30_charging_state = DC_STOP;
		if (reset == true)
			mtk_pep30_ta_hard_reset();
		wake_up_bat3();
	}
	mutex_unlock(&pe30_mutex);
}

void mtk_pep30_end(bool retry)
{
	_mtk_pep30_end(false);
	if (retry == true)
		is_pep30_done = false;
}

bool mtk_pep30_check_charger(void)
{
	if (is_mtk_pep30_rdy()) {
		mtk_chr_enable_charge(false);
		mtk_pep30_start();
		return true;
	}
	return false;
}


static enum hrtimer_restart mtk_charger_pep30_timer_callback(struct hrtimer *timer)
{
	mtk_charger_pep30_thread_flag = true;
	wake_up_interruptible(&mtk_charger_pep30_thread_waiter);

	return HRTIMER_NORESTART;
}


int pmic_vbus, TA_vbus;
int vbus_cali;

static void mtk_pep30_DC_init(void)
{
	int vbat;
	int i;
	int avgcur = 0;
	struct mtk_pep30_cap cap_now;
	unsigned int inputcur = 0;
	signed int fgcur = 0;
	bool sign;
	bool reset = true;
	int ret = 0;

	tcpm_set_direct_charge_en(tcpc, true);

	vbat = pmic_get_battery_voltage();

	pr_err("[mtk_pep30_DC_init]state = %d ,vbat = %d\n", pep30_charging_state, vbat);

	mtk_chr_enable_power_path(false);
	msleep(500);

	mtk_chr_get_input_current(&inputcur);
	fgcur = battery_meter_get_battery_current();
	sign = battery_meter_get_battery_current_sign();

	for (i = 0; i < 10; i++) {
		ret = mtk_pep30_get_ta_current_cap(&cap_now);
		if (ret != 0) {
			pr_err("[mtk_pep30_DC_init]err0 = %d\n", ret);
			mtk_chr_enable_power_path(true);
			goto _fail;
		}
		avgcur += cap_now.cur;
		pr_err("[mtk_pep30_DC_init]cur:%d vol:%d\n", cap_now.cur, cap_now.vol);
	}
	avgcur = avgcur / 10;

	pmic_vbus = battery_meter_get_charger_voltage();
	TA_vbus = cap_now.vol;
	vbus_cali = TA_vbus - pmic_vbus;

	pr_err("[mtk_pep30_DC_init]FOD cur:%d vol:%d fgc:%d:%d inputcur:%d vbus:%d cali:%d\n",
		avgcur, cap_now.vol, sign, fgcur, inputcur, pmic_vbus, vbus_cali);

	mtk_chr_enable_power_path(true);

	if (avgcur > FOD_CURRENT) {
		reset = false;
		goto _fail;
	}

	ret = mtk_enable_direct_charge(tcpc, true);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_init]err0 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pep30_set_ta_boundary_cap(CC_INIT, vbat + 50);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_init]err1 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pep30_set_ta_cap(CC_SS_INIT, vbat + 50);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_init]err2 = %d\n", ret);
		goto _fail;
	}

	msleep(CC_SS_BLANKING);

	ret = mtk_pep30_enable_direct_charge(true);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_init]enable fail = %d\n", ret);
		goto _fail;
	}
	pep30_charging_state = DC_MEASURE_R;
	ktime = ktime_set(0, BAT_MS_TO_NS(CC_SS_BLANKING));

	return;
_fail:
	_mtk_pep30_end(reset);

}

static void mtk_pep30_DC_measure_R(void)
{
	struct mtk_pep30_cap cap1, cap2;
	int vbus1, vbat1, vbus2, vbat2, bifvbat1 = 0, bifvbat2 = 0;
	int fgcur1, fgcur2;
	int ret;
	bool reset = true;
	unsigned char emark;

	pr_err("[mtk_pep30_DC_measure_R]vbus = %d\n", battery_meter_get_charger_voltage());

	/* set boundary */
	ret = mtk_pep30_set_ta_boundary_cap(CC_INIT, CV_LIMIT);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_measure_R]err1 = %d\n", ret);
		goto _fail;
	}

	/* measure 1 */
	ret = mtk_pep30_set_ta_cap(1500, CV_LIMIT);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_measure_R]err2 = %d\n", ret);
		goto _fail;
	}
	msleep(300);

	vbus1 = battery_meter_get_charger_voltage();
	vbat1 = pmic_get_battery_voltage();
	pmic_get_bif_battery_voltage(&bifvbat1);
	fgcur1 = battery_meter_get_battery_current() / 10;
	ret = mtk_pep30_get_ta_current_cap(&cap1);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_measure_R]err3 = %d\n", ret);
		goto _fail;
	}

	if (bifvbat1 != 0 && bifvbat1 >= mtk_pep30_get_cv()) {
		pr_err("[mtk_pep30_DC_measure_R]end bifvbat1=%d cur:%d vol:%d\n",
		bifvbat1, cap1.cur, cap1.vol);
		reset = false;
		goto _fail;
	}

	/* measure 2 */
	ret = mtk_pep30_set_ta_cap(2000, CV_LIMIT);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_measure_R]err2 = %d\n", ret);
		goto _fail;
	}
	msleep(300);

	vbus2 = battery_meter_get_charger_voltage();
	vbat2 = pmic_get_battery_voltage();
	pmic_get_bif_battery_voltage(&bifvbat2);
	fgcur2 = battery_meter_get_battery_current() / 10;
	ret = mtk_pep30_get_ta_current_cap(&cap2);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_measure_R]err3 = %d\n", ret);
		goto _fail;
	}

	if (bifvbat2 != 0 && bifvbat2 >= mtk_pep30_get_cv()) {
		pr_err("[mtk_pep30_DC_measure_R]end bifvbat2=%d cur:%d vol:%d\n",
		bifvbat2, cap2.cur, cap2.vol);
		reset = false;
		goto _fail;
	}

	if (cap2.cur == cap1.cur)
		return;

	if (bifvbat1 != 0 && bifvbat2 != 0)
		r_vbat = abs((vbat2 - bifvbat2) - (vbat1 - bifvbat1)) * 1000 / abs(fgcur2 - fgcur1);

	r_sw = abs((vbus2 - vbus1) - (vbat2 - vbat1)) * 1000 / abs(cap2.cur - cap1.cur);
	r_cable_1 = abs((cap2.vol - cap1.vol)-(vbus2 - vbus1)) * 1000 / abs(cap2.cur - cap1.cur);

	r_cable = abs(cap2.vol - vbus_cali - vbus2) * 1000 / abs(cap2.cur);
	r_cable_2 = abs(cap1.vol - vbus_cali - vbus1) * 1000 / abs(cap1.cur);

	if (r_sw < R_SW_MIN)
		r_sw = R_SW_MIN;

	if (r_vbat < R_VBAT_MIN)
		r_vbat = R_VBAT_MIN;

	r_total = r_cable + r_sw + r_vbat;

	if (r_cable < CC_INIT_R)
		charging_current_limit_by_r = CC_INIT;
	else if (r_cable < CC_INIT_BAD_CABLE1_R)
		charging_current_limit_by_r = CC_INIT_BAD_CABLE1;
	else if (r_cable < CC_INIT_BAD_CABLE2_R)
		charging_current_limit_by_r = CC_INIT_BAD_CABLE2;
	else {
		pr_err("[mtk_pep30_DC_measure_R]r_cable:%d is too hight , end Pep30\n", r_cable);

		pr_err(
		"[mtk_pep30_DC_measure_R]Tibus:%d %d Tvbus:%d %d vbus:%d %d vbat:%d %d bifvbat:%d %d fgcur:%d %d r_c:%d r_sw:%d r_vbat:%d %d\n",
		cap2.cur, cap1.cur, cap2.vol, cap1.vol, vbus2, vbus1, vbat2,
		vbat1, bifvbat2, bifvbat1, fgcur2, fgcur1, r_cable, r_sw, r_vbat, charging_current_limit_by_r);

		pr_err(
		"[mtk_pep30_DC_measure_R]r_cable(2A):%d r_cable1:%d r_cable2(1.5A):%d cali:%d\n",
		r_cable, r_cable_1, r_cable_2, vbus_cali);

		reset = false;
		goto _fail;
	}

	tcpm_get_cable_capability(tcpc, &emark);
#ifdef PE30_CHECK_EMARK
	if (emark == 2) {
		if (charging_current_limit_by_r > 5000) {
			charging_current_limit_by_r = 5000;
			pr_err("[mtk_pep30_DC_measure_R]emark:%d reset current:%d\n",
				emark, charging_current_limit_by_r);
		}
	} else if (emark == 1) {
		if (charging_current_limit_by_r > 3000) {
			charging_current_limit_by_r = 3000;
			pr_err("[mtk_pep30_DC_measure_R]emark:%d reset current:%d\n",
				emark, charging_current_limit_by_r);
		}
	} else {
		reset = false;
		pr_err("[mtk_pep30_DC_measure_R]emark:%d end pep30\n", emark);
		goto _fail;
	}
#endif

	pr_err(
	"[mtk_pep30_DC_measure_R]Tibus:%d %d Tvbus:%d %d vbus:%d %d vbat:%d %d bifvbat:%d %d fgcur:%d %d r_c:%d r_sw:%d r_vbat:%d %d\n",
	cap2.cur, cap1.cur, cap2.vol, cap1.vol, vbus2, vbus1, vbat2,
	vbat1, bifvbat2, bifvbat1, fgcur2, fgcur1, r_cable, r_sw, r_vbat, charging_current_limit_by_r);

	pr_err(
	"[mtk_pep30_DC_measure_R]r_cable(2A):%d r_cable1:%d r_cable2(1.5A):%d cali:%d\n",
	r_cable, r_cable_1, r_cable_2, vbus_cali);

	pep30_charging_state = DC_SOFT_START;

	return;
_fail:
	_mtk_pep30_end(reset);

}


static void mtk_pep30_DC_soft_start(void)
{
	int ret;
	int vbat;
	int cv;
	struct mtk_pep30_cap setting, cap_now;
	int current_setting;
	bool reset = true;

	pr_err("[mtk_pep30_DC_soft_start]vbus = %d\n", battery_meter_get_charger_voltage());

	ret = mtk_pep30_get_ta_current_cap(&cap_now);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_soft_start]err1 = %d\n", ret);
		goto _fail;
	}

	ret = mtk_pep30_get_ta_cap(&setting);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_soft_start]err2 = %d\n", ret);
		goto _fail;
	}

	current_setting = setting.cur;

	vbat = pmic_get_battery_voltage();

	if (vbat >= mtk_pep30_get_cv()) {

		if (cap_now.cur >= CC_END) {
			current_setting = current_setting - CC_SS_STEP;
			cv = mtk_pep30_get_cali_vbus(current_setting);
			if (cv >= CV_LIMIT)
				cv = CV_LIMIT;
			ret = mtk_pep30_set_ta_cap(current_setting, cv);
			if (ret != 0) {
				pr_err("[mtk_pep30_DC_soft_start]err6 = %d\n", ret);
				goto _fail;
			}

			pr_err(
				"[mtk_pep30_DC_soft_start]go to CV vbat:%d current:%d setting current:%d\n",
				vbat, cap_now.cur, current_setting);
			pep30_charging_state = DC_CC;
			ktime = ktime_set(0, BAT_MS_TO_NS(CC_BLANKING));
			return;
		}
		pr_err("[mtk_pep30_DC_soft_start]end vbat:%d current:%d setting current:%d\n",
			vbat, cap_now.cur, current_setting);
		reset = false;
		goto _fail;

	}

	if (setting.cur >= CC_INIT) {
		pep30_charging_state = DC_CC;
		pr_err("[mtk_pep30_DC_soft_start]go to CV2 vbat:%d current:%d setting current:%d\n",
			vbat, cap_now.cur, current_setting);
		ktime = ktime_set(0, BAT_MS_TO_NS(CC_BLANKING));
		return;
	}

	if (vbat >= CV_SS_STEP2)
		current_setting = current_setting + CC_SS_STEP2;
	else if (vbat >= CV_SS_STEP3)
		current_setting = current_setting + CC_SS_STEP3;
	else
		current_setting = current_setting + CC_SS_STEP;

	cv = mtk_pep30_get_cali_vbus(current_setting);

	pr_err(
		"[mtk_pep30_DC_soft_start]vbat:%d cv = %d cur:%d r:%d cap_now.cur:%d last_cur:%d\n",
		vbat, cv, current_setting, r_total,
		cap_now.cur, setting.cur);

	ret = mtk_pep30_set_ta_boundary_cap(CC_INIT, CV_LIMIT);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_soft_start]err4 = %d\n", ret);
		goto _fail;
	}

	if (cv >= CV_LIMIT)
		cv = CV_LIMIT;

	ret = mtk_pep30_set_ta_cap(current_setting, cv);


	pr_err("[mtk_pep30_DC_soft_start]cur = %d vol = %d\n", current_setting, cv);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_soft_start]err5 = %d\n", ret);
		goto _fail;
	}

	pr_err("[mtk_pep30_DC_soft_start]setting:%d %d now:%d %d boundary:%d %d cap:%d %d\n",
		setting.cur, setting.vol,
		cap_now.cur, cap_now.vol, CC_INIT, CV_LIMIT, current_setting, cv);


	return;
_fail:
	_mtk_pep30_end(reset);


}

static void mtk_pep30_DC_cc(void)
{
	struct mtk_pep30_cap setting, cap_now;
	int vbat = 0;
	int ret;
	int TAVbus, current_setting, cv;
	int tmp;
	int tmp_min = 0, tmp_max = 0;
	bool reset = true;

	pr_err("[mtk_pep30_DC_CC]state = %d\n", pep30_charging_state);

	ret = pmic_get_bif_battery_voltage(&vbat);
	if (ret < 0)
		vbat = pmic_get_battery_voltage();

	ret = mtk_pep30_get_ta_current_cap(&cap_now);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_CC]err1 = %d\n", ret);
		goto _fail;
	}

	if (cap_now.cur <= CC_END) {
		if (charging_current_limit < CC_END) {
			pr_err(
				"[mtk_pep30_DC_cc]cur<%d , reset is_pep30_done\n", CC_END);
			mutex_lock(&pe30_mutex);
			is_pep30_done = false;
			mutex_unlock(&pe30_mutex);
		}
		pr_err("[mtk_pep30_DC_CC]pe30 end , current:%d\n", cap_now.cur);
		reset = false;
		goto _fail;
	}

	ret = mtk_pep30_get_ta_cap(&setting);
	if (ret != 0) {
		pr_err("[mtk_pep30_DC_CC]err2 = %d\n", ret);
		goto _fail;
	}

	current_setting = setting.cur;
	tmp = battery_meter_get_battery_temperature();

	ret = mtk_chr_get_tchr(&tmp_min, &tmp_max);
	if (ret < 0) {
		pr_err("[mtk_pep30_DC_CC]err3 = %d\n", ret);
		goto _fail;
	}

	cv = mtk_pep30_get_cv();
	if (vbat >= cv || tmp_max >= CHARGER_TEMP_MAX) {

		current_setting -= CC_STEP;
		TAVbus = mtk_pep30_get_cali_vbus(current_setting);

		if (TAVbus >= CV_LIMIT)
			TAVbus = CV_LIMIT;

		if (TAVbus <= 5500)
			TAVbus = 5500;

		ret = mtk_pep30_set_ta_boundary_cap(CC_INIT, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pep30_DC_CC]err6 = %d\n", ret);
			goto _fail;
		}

		if (current_setting >= mtk_pep30_get_charging_current_limit()) {
			pr_err("[mtk_pep30_DC_cc]over limit target:%d limit:%d\n",
			current_setting, mtk_pep30_get_charging_current_limit());
			current_setting = mtk_pep30_get_charging_current_limit();
		}

		ret = mtk_pep30_set_ta_cap(current_setting, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pep30_DC_CC]err7 = %d\n", ret);
			goto _fail;
		}

		pr_err("[mtk_pep30_DC_CC]- vbat:%d current:%d:%d r:%d TAVBUS:%d cv:%d tmp_max:%d\n",
			vbat, current_setting, mtk_pep30_get_charging_current_limit(), r_total, TAVbus, cv, tmp_max);
	} else {

		current_setting += CC_STEP;
		TAVbus = mtk_pep30_get_cali_vbus(current_setting);

		if (TAVbus >= CV_LIMIT)
			TAVbus = CV_LIMIT;

		if (TAVbus <= 5500)
			TAVbus = 5500;

		ret = mtk_pep30_set_ta_boundary_cap(CC_INIT, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pep30_DC_CC]err8 = %d\n", ret);
			goto _fail;
		}

		if (current_setting >= mtk_pep30_get_charging_current_limit()) {
			pr_err("[mtk_pep30_DC_cc]over limit target:%d limit:%d\n",
			current_setting, mtk_pep30_get_charging_current_limit());
			current_setting = mtk_pep30_get_charging_current_limit();
		}

		ret = mtk_pep30_set_ta_cap(current_setting, TAVbus);
		if (ret != 0) {
			pr_err("[mtk_pep30_DC_CC]err9 = %d\n", ret);
			goto _fail;
		}

		pr_err("[mtk_pep30_DC_CC]+ vbat:%d current:%d:%d r:%d TAVBUS:%d cv:%d\n",
			vbat, current_setting, mtk_pep30_get_charging_current_limit(), r_total, TAVbus, cv);
	}

	return;

_fail:
	_mtk_pep30_end(reset);

}

bool mtk_pep30_safety_check(void)
{
	int vbus = 0, vbus_cv, bifvbat = 0;
	int pmic_vbat = 0, pmic_vbat_ovp;
	struct mtk_pep30_cap cap, setting, cap_b, tmp;
	int ret;
	signed int cur;
	bool sign;
	unsigned int chrCur = 0;
	unsigned char emark = 0;
	int biftmp = 0;
	bool reset = true;

	setting.vol = 0;
	setting.cur = 0;
	cap.vol = 0;
	cap.cur = 0;
	tmp.temp = 0;
	cur = battery_meter_get_battery_current();
	sign = battery_meter_get_battery_current_sign();
	ret = mtk_pep30_get_ta_boundary_cap(&cap_b);
	if (ret < 0) {
		pr_err("[%s]get ta boundary cap fail ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	if (mtk_pep30_get_ta_charger_status() == false) {
		pr_err("[%s]get ta status ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	/*vbus ov: auxadc by PMIC */
	vbus = battery_meter_get_charger_voltage();
	if (pep30_charging_state != DC_MEASURE_R && pep30_charging_state != DC_INIT) {
		vbus_cv = BAT_UPPER_BOUND + 70 + 110 * CC_INIT / 100 * r_cable;
		if (vbus > vbus_cv) {
			pr_err("[%s]%d vbus %d > vbus_cv %d , r_cable:%d end pep30\n",
			__func__, pep30_charging_state, vbus, vbus_cv, r_cable);
			reset = false;
			goto _fail;
		}
	}

	/* ibus oc: RT9468 current */
	ret = mtk_chr_get_input_current(&chrCur);
	if (ret < 0) {
		pr_err("[%s]get current from chr fail ret:%d\n",
		__func__, ret);
		goto _fail;
	}

	if (chrCur >= CC_MAX) {
		pr_err("[%s]current is too high from chr :%d\n",
		__func__, chrCur);
		goto _fail;
	}

	/* ibus oc:TA ADC current*/
	mtk_pep30_get_ta_current_cap(&cap);
	mtk_pep30_get_ta_cap(&setting);

	if (cap.cur >= CC_MAX) {
		pr_err("[%s]cur %d >= CC_MAX %d , end pep30\n", __func__, cap.cur, CC_MAX);
		goto _fail;
	}

#ifdef PE30_CHECK_TA
	if ((cap.vol - setting.vol) > VBUS_OV_GAP) {
		pr_err("[%s]vbus:%d > vbuus_b:%d gap:%d, end pep30\n", __func__, cap.vol,
		setting.vol, VBUS_OV_GAP);
		goto _fail;
	}
#endif


	/*vbat ovp*/
	pmic_vbat = pmic_get_battery_voltage();
	ret = pmic_get_bif_battery_voltage(&bifvbat);
	if (pep30_charging_state != DC_MEASURE_R && pep30_charging_state != DC_INIT) {
		if (ret >= 0)
			pmic_vbat_ovp = BAT_UPPER_BOUND + 50 + 110 * current_cap.cur / 100 * r_vbat;
		else
			pmic_vbat_ovp = BAT_UPPER_BOUND + 50;
		/*pmic adc vbat */
		if (pmic_vbat > pmic_vbat_ovp) {
			pr_err("[%s]pmic_vbat:%d pmic_vbat_ovp:%d vbatCv:%d current:%d r_vbat:%d\n",
			__func__, pmic_vbat, pmic_vbat_ovp, BAT_UPPER_BOUND, current_cap.cur, r_vbat);
			goto _fail;
		}

		/*bif adc vbat */
		if (ret >= 0 && bifvbat >= BAT_UPPER_BOUND + 50) {
			pr_err("[%s]bifvbat %d >= BAT_UPPER_BOUND:%d + 50\n",
			__func__, bifvbat, BAT_UPPER_BOUND + 50);
			goto _fail;
		}
	}

	if (battery_meter_get_battery_status() == false) {
		pr_err("[%s]no battery, end pep30\n",
		__func__);
		reset = false;
		goto _fail;
	}

	if (batteryTemperature >= BATTERY_TEMP_MAX) {
		pr_err("[%s]battery temperature is too high %d, end pep30\n",
		__func__, batteryTemperature);
		reset = false;
		goto _fail;
	}

	if (batteryTemperature <= BATTERY_TEMP_MIN) {
		pr_err("[%s]battery temperature is too low %d, end pep30\n",
		__func__, batteryTemperature);
		reset = false;
		goto _fail;
	}

	/* TA thermal*/
	mtk_pep30_get_ta_tmp_wdt(&tmp);
	if (tmp.temp >= TA_TEMP_MAX) {
		pr_err("[%s]TA temperature is too high %d, end pep30\n",
		__func__, tmp.temp);
		goto _fail;
	}

	mtk_chr_kick_direct_charge_wdt();
	tcpm_get_cable_capability(tcpc, &emark);

	pmic_get_bif_battery_temperature(&biftmp);

	pr_err(
	"fg:%d:%d:%d:dif:%d:[cur]:%d:%d:%d:[vol]:%d:%d:vbus:%d:vbat:%d:bifvbat:%d:soc:%d:%d:tmp:%d:%d:%d:b:%d:%d:m:%d:%d:%d\n",
		pep30_charging_state, sign, cur, cap.cur - cur/10,
		cap.cur, setting.cur, chrCur,
		cap.vol, setting.vol, vbus, pmic_vbat, bifvbat,
		BMT_status.SOC, BMT_status.UI_SOC2,
		batteryTemperature, biftmp, tmp.temp, cap_b.cur, cap_b.vol,
		emark, charging_current_limit, charging_current_limit_by_r);

	if (charging_current_limit < CC_END) {
		pr_err(
			"[mtk_pep30_set_charging_current_limit]cur<%d , reset is_pep30_done\n", CC_END);
		_mtk_pep30_end(false);
		mutex_lock(&pe30_mutex);
		is_pep30_done = false;
		mutex_unlock(&pe30_mutex);
	}

	return true;

_fail:

	pr_err(
	"fg:%d:%d:%d:dif:%d:[cur]:%d:%d:%d:[vol]:%d:%d:vbus:%d:vbat:%d:bifvbat:%d:soc:%d:uisoc2:%d:tmp:%d:%d:b:%d:%d:m:%d\n",
		pep30_charging_state, sign, cur, cap.cur - cur/10,
		cap.cur, setting.cur, chrCur,
		cap.vol, setting.vol, vbus, pmic_vbat, bifvbat,
		BMT_status.SOC, BMT_status.UI_SOC2,
		batteryTemperature, tmp.temp, cap_b.cur, cap_b.vol,
		emark);

	_mtk_pep30_end(reset);

	return false;
}

static int mtk_charger_pep30_thread_handler(void *unused)
{
	struct timespec time;

	ktime = ktime_set(0, BAT_MS_TO_NS(500));
	do {
		wait_event_interruptible(mtk_charger_pep30_thread_waiter,
					 (mtk_charger_pep30_thread_flag == true));

		mtk_charger_pep30_thread_flag = false;

		get_monotonic_boottime(&endTime);
		time = timespec_sub(endTime, startTime);

		batteryTemperature = battery_meter_get_battery_temperature();

		pr_err("[mtk_charger_pep30_thread_handler]state = %d tmp:%d time:%d\n",
		pep30_charging_state, batteryTemperature, (int)time.tv_sec);

		if (time.tv_sec >= PE30_MAX_CHARGING_TIME) {
			pr_err("[mtk_charger_pep30_thread_handler]pe30 charging timeout %d:%d\n",
			(int)time.tv_sec, PE30_MAX_CHARGING_TIME);
			_mtk_pep30_end(false);
		}

		switch (pep30_charging_state) {
		case DC_INIT:
			mtk_pep30_DC_init();
			break;

		case DC_MEASURE_R:
			mtk_pep30_DC_measure_R();
			break;

		case DC_SOFT_START:
			mtk_pep30_DC_soft_start();
			break;

		case DC_CC:
			mtk_pep30_DC_cc();
			break;

		case DC_STOP:
			break;
		}

		if (pep30_charging_state != DC_STOP) {
			mtk_pep30_safety_check();
			hrtimer_start(&mtk_charger_pep30_timer, ktime, HRTIMER_MODE_REL);
		}
	} while (!kthread_should_stop());

	return 0;
}

bool mtk_pep30_init(void)
{
	ktime_t ktime;

	tcpc = NULL;
	ktime = ktime_set(0, BAT_MS_TO_NS(2000));
	hrtimer_init(&mtk_charger_pep30_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mtk_charger_pep30_timer.function = mtk_charger_pep30_timer_callback;

	mtk_charger_pep30_thread =
	    kthread_run(mtk_charger_pep30_thread_handler, 0,
			"bat_mtk_charger_pep30");
	if (IS_ERR(mtk_charger_pep30_thread)) {
		pr_err(
			    "[%s]: failed to create mtk_charger_pep30 thread\n",
			    __func__);
	}

	pr_err("mtk_charger_pep30 : done\n");

	return true;

}

/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <mach/mtk_pmic_wrap.h>
#include <linux/ratelimit.h>
#include <linux/timekeeping.h>

#include "include/pmic.h"
#include "include/pmic_auxadc.h"
#include "mtk_idle.h"
#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_auxadc_intf.h>

#define AEE_DBG 1

static int count_time_out = 100;
static int battmp;
#if AEE_DBG
static unsigned int aee_count;
#endif
static unsigned int dbg_count;
static unsigned char dbg_flag;

static struct wake_lock  pmic_auxadc_wake_lock;
static struct mutex pmic_adc_mutex;
static DEFINE_MUTEX(auxadc_ch3_mutex);

#define DBG_REG_SIZE 264
struct pmic_adc_dbg_st {
	int ktime_sec;
	unsigned short reg[DBG_REG_SIZE];
};
static unsigned int adc_dbg_addr[DBG_REG_SIZE];
static struct pmic_adc_dbg_st pmic_adc_dbg[4];
static unsigned char dbg_stamp;

/*
 * The vbif28 variable and function are defined in upmu_regulator
 * to prevent from building error, define weak attribute here
 */
unsigned int __attribute__ ((weak)) g_pmic_pad_vbif28_vol = 1;
unsigned int __attribute__ ((weak)) pmic_get_vbif28_volt(void)
{
	return 0;
}

void wk_auxadc_bgd_ctrl(unsigned char en)
{
	if (en) {
		/*--BAT TEMP MAX DET EN--*/
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX, 1);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MAX, 1);
		/*--BAT TEMP MIN DET EN--*/
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN, 1);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MIN, 1);
		/*--BAT TEMP DET EN--*/
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_H, 1);
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_L, 1);
	} else {
		/*--BAT TEMP MAX DET EN--*/
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MAX, 0);
		/*--BAT TEMP MIN DET EN--*/
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN, 0);
		pmic_set_register_value(PMIC_AUXADC_BAT_TEMP_EN_MIN, 0);
		/*--BAT TEMP DET EN--*/
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_H, 0);
		pmic_set_register_value(PMIC_RG_INT_EN_BAT_TEMP_L, 0);
	}
}

static int pmic_auxadc_notify_call(struct notifier_block *nb, unsigned long id, void *unused)
{
#if 0
	/* Already move to sspm */
	switch (id) {
	case NOTIFY_DPIDLE_ENTER:
	case NOTIFY_SOIDLE_ENTER:
	case NOTIFY_SOIDLE3_ENTER:
		/* Turn off AUXADC BAT TEMP Background detection, because VBIF28 will be disabled */
		wk_auxadc_bgd_ctrl(0);
		break;
	case NOTIFY_DPIDLE_LEAVE:
	case NOTIFY_SOIDLE_LEAVE:
	case NOTIFY_SOIDLE3_LEAVE:
		/* Turn on AUXADC BAT TEMP Background detection for normal mode */
		wk_auxadc_bgd_ctrl(1);
		break;
	default:
		break;
	}
#endif
	return NOTIFY_OK;
}

static struct notifier_block pmic_auxadc_nb = {
	.notifier_call = pmic_auxadc_notify_call,
};

void pmic_auxadc_suspend(void)
{
	wk_auxadc_bgd_ctrl(0);
	battmp = 0;
}

void pmic_auxadc_resume(void)
{
	wk_auxadc_bgd_ctrl(1);
}

void wk_auxadc_bgd_ctrl_dbg(void)
{
	pr_err("EN_BAT_TEMP_L: %d\n", pmic_get_register_value(PMIC_RG_INT_EN_BAT_TEMP_L));
	pr_err("EN_BAT_TEMP_H: %d\n", pmic_get_register_value(PMIC_RG_INT_EN_BAT_TEMP_H));
	pr_err("BAT_TEMP_IRQ_EN_MAX: %d\n", pmic_get_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX));
	pr_err("BAT_TEMP_IRQ_EN_MIN: %d\n", pmic_get_register_value(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN));
	pr_err("BAT_TEMP_EN_MAX: %d\n", pmic_get_register_value(PMIC_AUXADC_BAT_TEMP_EN_MAX));
	pr_err("BAT_TEMP_EN_MIN: %d\n", pmic_get_register_value(PMIC_AUXADC_BAT_TEMP_EN_MIN));
	pr_err("BATON_TDET_EN: %d\n", pmic_get_register_value(PMIC_BATON_TDET_EN));
}

int bat_temp_filter(int *arr, unsigned short size)
{
	unsigned char i, i_max, i_min = 0;
	int arr_max = 0, arr_min = arr[0];
	int sum = 0;

	for (i = 0; i < size; i++) {
		sum += arr[i];
		if (arr[i] > arr_max) {
			arr_max = arr[i];
			i_max = i;
		} else if (arr[i] < arr_min) {
			arr_min = arr[i];
			i_min = i;
		}
	}
	sum = sum - arr_max - arr_min;
	return (sum/(size - 2));
}

void wk_auxadc_dbg_dump(void)
{
	unsigned char reg_log[861] = "", reg_str[21] = "";
	unsigned char i;
	unsigned short j;

	for (i = 0; i < 4; i++) {
		if (pmic_adc_dbg[dbg_stamp].ktime_sec == 0) {
			dbg_stamp++;
			if (dbg_stamp >= 4)
				dbg_stamp = 0;
			continue;
		}
		for (j = 0; adc_dbg_addr[j] != 0; j++) {
			if (j % 43 == 0) {
				pr_err("%d %s\n", pmic_adc_dbg[dbg_stamp].ktime_sec, reg_log);
				strncpy(reg_log, "", 860);
			}
			snprintf(reg_str, 20, "Reg[0x%x]=0x%x, ", adc_dbg_addr[j], pmic_adc_dbg[dbg_stamp].reg[j]);
			strncat(reg_log, reg_str, 860);
		}
		pr_err("[%s] %d %d %s\n", __func__, dbg_stamp, pmic_adc_dbg[dbg_stamp].ktime_sec, reg_log);
		strncpy(reg_log, "", 860);
		dbg_stamp++;
		if (dbg_stamp >= 4)
			dbg_stamp = 0;
	}
}

int wk_auxadc_battmp_dbg(int bat_temp)
{
	unsigned short i;
	int vbif = 0, bat_temp2 = 0, bat_temp3 = 0, bat_id = 0;
	int arr_bat_temp[5];

	if (dbg_flag)
		pr_err("Another dbg is running\n");
	dbg_flag = 1;
	for (i = 0; adc_dbg_addr[i] != 0; i++)
		pmic_adc_dbg[dbg_stamp].reg[i] = upmu_get_reg_value(adc_dbg_addr[i]);
	pmic_adc_dbg[dbg_stamp].ktime_sec = (int)get_monotonic_coarse().tv_sec;
	dbg_stamp++;
	if (dbg_stamp >= 4)
		dbg_stamp = 0;
	/* Re-read AUXADC */
	bat_temp2 = pmic_get_auxadc_value(AUXADC_LIST_BATTEMP);
	vbif = pmic_get_auxadc_value(AUXADC_LIST_VBIF);
	bat_temp3 = pmic_get_auxadc_value(AUXADC_LIST_BATTEMP);
	bat_id = pmic_get_auxadc_value(AUXADC_LIST_BATID);
	pr_err("BAT_TEMP1: %d, BAT_TEMP2:%d, VBIF:%d, BAT_TEMP3:%d, BATID:%d, DA_VBIF28_STB:%d\n",
		bat_temp, bat_temp2, vbif, bat_temp3, bat_id, pmic_get_register_value(PMIC_DA_VBIF28_STB));

	if (bat_temp < 200 ||
		(battmp != 0 && (bat_temp - battmp > 100 || battmp - bat_temp > 100))) {
		wk_auxadc_dbg_dump();
		for (i = 0; i < 5; i++)
			arr_bat_temp[i] = pmic_get_auxadc_value(AUXADC_LIST_BATTEMP);
		bat_temp = bat_temp_filter(arr_bat_temp, 5);
	}
	dbg_flag = 0;
	return bat_temp;
}

void pmic_auxadc_lock(void)
{
	wake_lock(&pmic_auxadc_wake_lock);
	mutex_lock(&pmic_adc_mutex);
}

void lockadcch3(void)
{
	mutex_lock(&auxadc_ch3_mutex);
}

void unlockadcch3(void)
{
	mutex_unlock(&auxadc_ch3_mutex);
}


void pmic_auxadc_unlock(void)
{
	mutex_unlock(&pmic_adc_mutex);
	wake_unlock(&pmic_auxadc_wake_lock);
}

struct pmic_auxadc_channel_new {
	u8 resolution;
	u8 r_val;
	u8 ch_num;
	unsigned int channel_rqst;
	unsigned int channel_rdy;
	unsigned int channel_out;
};

struct pmic_auxadc_channel_new mt6356_auxadc_channel[] = {
	{15, 3, 0, PMIC_AUXADC_RQST_CH0, /* BATADC */
		PMIC_AUXADC_ADC_RDY_CH0_BY_AP, PMIC_AUXADC_ADC_OUT_CH0_BY_AP},
	{12, 1, 2, PMIC_AUXADC_RQST_CH2, /* VCDT */
		PMIC_AUXADC_ADC_RDY_CH2, PMIC_AUXADC_ADC_OUT_CH2},
	{12, 2, 3, PMIC_AUXADC_RQST_CH3, /* BAT TEMP */
		PMIC_AUXADC_ADC_RDY_CH3, PMIC_AUXADC_ADC_OUT_CH3},
	{12, 2, 3, PMIC_AUXADC_RQST_BATID, /* BATID */
		PMIC_AUXADC_ADC_RDY_BATID, PMIC_AUXADC_ADC_OUT_BATID},
	{12, 2, 11, PMIC_AUXADC_RQST_CH11, /* VBIF */
		PMIC_AUXADC_ADC_RDY_CH11, PMIC_AUXADC_ADC_OUT_CH11},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4, /* CHIP TEMP */
		PMIC_AUXADC_ADC_RDY_CH4, PMIC_AUXADC_ADC_OUT_CH4},
	{15, 1, 4, PMIC_AUXADC_RQST_CH4, /* DCXO */
		PMIC_AUXADC_ADC_RDY_DCXO_BY_AP, PMIC_AUXADC_ADC_OUT_DCXO_BY_AP},
	{12, 1, 5, PMIC_AUXADC_RQST_CH5, /* ACCDET MULTI-KEY */
		PMIC_AUXADC_ADC_RDY_CH5, PMIC_AUXADC_ADC_OUT_CH5},
	{15, 1, 7, PMIC_AUXADC_RQST_CH7, /* TSX */
		PMIC_AUXADC_ADC_RDY_CH7_BY_AP, PMIC_AUXADC_ADC_OUT_CH7_BY_AP},
	{15, 1, 9, PMIC_AUXADC_RQST_CH9, /* HP OFFSET CAL */
		PMIC_AUXADC_ADC_RDY_CH9, PMIC_AUXADC_ADC_OUT_CH9},
	{15, 3, 1, PMIC_AUXADC_RQST_CH1, /* ISENSE */
		PMIC_AUXADC_ADC_RDY_CH1_BY_AP, PMIC_AUXADC_ADC_OUT_CH1_BY_AP},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4_BY_THR1, /* TS_BUCK1 */
		PMIC_AUXADC_ADC_RDY_CH4_BY_THR1, PMIC_AUXADC_ADC_OUT_CH4_BY_THR1},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4_BY_THR2, /* TS_BUCK2 */
		PMIC_AUXADC_ADC_RDY_CH4_BY_THR2, PMIC_AUXADC_ADC_OUT_CH4_BY_THR2},
};
#define MT6356_AUXADC_CHANNEL_MAX	ARRAY_SIZE(mt6356_auxadc_channel)

static unsigned int mts_count;
static unsigned int mts_adc;
/*--Monitor MTS reg--*/
void mt6356_auxadc_monitor_mts_regs(void)
{
	int count = 0;
	unsigned int mts_adc_tmp = 0;

	if (mts_adc == 0)
		return;
	mts_adc_tmp = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
	pr_notice("[MTS_ADC] OLD = 0x%x, NOW = 0x%x, CNT = %d\n", mts_adc, mts_adc_tmp, mts_count);

	if (mts_adc ==  mts_adc_tmp)
		mts_count++;
	else
		mts_count = 0;

	if (mts_count >= 8 && mts_count < 10) {
		pwrap_dump_all_register();
		/* AUXADC_ADC_RDY_MDRT & AUXADC_ADC_OUT_MDRT */
		pr_notice("AUXADC_ADC36 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_ADC36));
		pr_notice("AUXADC_MDRT_0 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_0));
		pr_notice("AUXADC_MDRT_1 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_1));
		pr_notice("AUXADC_MDRT_2 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_2));
		pr_notice("AUXADC_MDRT_3 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_3));
		pr_notice("AUXADC_MDRT_4 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_4));
		/*--AUXADC CLK--*/
		pr_notice("RG_AUXADC_CK_PDN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN));
		pr_notice("RG_AUXADC_CK_PDN_HWEN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN));
		/*--AUXADC CH7--*/
		pr_notice("[MTS_ADC] trigger TSX by AP: %d\n", pmic_get_auxadc_value(AUXADC_LIST_TSX));
		pmic_set_register_value(PMIC_AUXADC_RQST_CH7_BY_GPS, 1);
		udelay(10);
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_GPS) != 1) {
			usleep_range(1300, 1500);
			if ((count++) > count_time_out)
				break;
		}
		pmic_set_register_value(PMIC_AUXADC_RQST_CH7_BY_MD, 1);
		udelay(10);
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_MD) != 1) {
			usleep_range(1300, 1500);
			if ((count++) > count_time_out)
				break;
		}
		pr_notice("AUXADC_ADC16 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_ADC16));
		pr_notice("AUXADC_ADC17 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_ADC17));
	}
	if (mts_count == 12) {
		pmic_set_register_value(PMIC_AUXADC_MDRT_DET_EN, 0);
		udelay(10);
		pmic_set_register_value(PMIC_AUXADC_MDRT_DET_EN, 1);
	}
	if (mts_count > 15) {
		aee_kernel_warning("PMIC AUXADC:MDRT", "MDRT");
		pwrap_dump_all_register();
		pr_notice("DEW_READ_TEST = 0x%x\n", pmic_get_register_value(PMIC_DEW_READ_TEST));
		/*--AUXADC CH7--*/
		pr_notice("AUXADC_LIST_TSX = %d\n", pmic_get_auxadc_value(AUXADC_LIST_TSX));
		/*--AUXADC--*/
		/* AUXADC_ADC_RDY_MDRT & AUXADC_ADC_OUT_MDRT */
		pr_notice("AUXADC_ADC36 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_ADC36));
		pr_notice("AUXADC_MDRT_0 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_0));
		pr_notice("AUXADC_MDRT_1 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_1));
		pr_notice("AUXADC_MDRT_2 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_2));
		pr_notice("AUXADC_MDRT_3 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_3));
		pr_notice("AUXADC_MDRT_4 = 0x%x\n", upmu_get_reg_value(MT6356_AUXADC_MDRT_4));
		/*--AUXADC CLK--*/
		pr_notice("RG_AUXADC_CK_PDN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN));
		pr_notice("RG_AUXADC_CK_PDN_HWEN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN));
		mts_count = 0;
	}
	mts_adc = mts_adc_tmp;
}

int mt6356_get_auxadc_value(u8 channel)
{
	int count = 0, tdet_tmp = 0;
	signed int adc_result = 0, reg_val = 0;
	struct pmic_auxadc_channel_new *auxadc_channel;
	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 5);

	if (channel - AUXADC_LIST_MT6356_START < 0 ||
			channel - AUXADC_LIST_MT6356_END > 0) {
		pr_err("[%s] Invalid channel(%d)\n", __func__, channel);
		return -EINVAL;
	}
	auxadc_channel =
		&mt6356_auxadc_channel[channel-AUXADC_LIST_MT6356_START];

	pmic_auxadc_lock();

	if (channel == AUXADC_LIST_DCXO)
		pmic_set_register_value(PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL, 1);
	if (channel == AUXADC_LIST_MT6356_CHIP_TEMP)
		pmic_set_register_value(PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL, 0);
	if (channel == AUXADC_LIST_VBIF) {
		if (pmic_get_register_value(PMIC_BATON_TDET_EN)) {
			tdet_tmp = 1;
			pr_err("pmic auxadc baton_tdet_en should be zero\n");
			pmic_set_register_value(PMIC_BATON_TDET_EN, 0);
		}
	}
	if (channel == AUXADC_LIST_BATTEMP)
		mutex_lock(&auxadc_ch3_mutex);

	pmic_set_register_value(auxadc_channel->channel_rqst, 1);
	udelay(10);

	while (pmic_get_register_value(auxadc_channel->channel_rdy) != 1) {
		usleep_range(1300, 1500);
		if ((count++) > count_time_out) {
			pr_err("[%s] (%d) Time out!\n", __func__, channel);
			pr_err("[%s] Reg[0x%x]=0x%x, RG_AUXADC_CK_PDN_HWEN=%d, RG_AUXADC_CK_TSTSEL=%d, RG_SMPS_CK_TSTSEL=%d\n",
				__func__,
				MT6356_STRUP_CON6, upmu_get_reg_value(MT6356_STRUP_CON6),
				pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN),
				pmic_get_register_value(PMIC_RG_AUXADC_CK_TSTSEL),
				pmic_get_register_value(PMIC_RG_SMPS_CK_TSTSEL));
			break;
		}
	}

	reg_val = pmic_get_register_value(auxadc_channel->channel_out);

	if (channel == AUXADC_LIST_BATTEMP)
		mutex_unlock(&auxadc_ch3_mutex);

	pmic_auxadc_unlock();

	if (auxadc_channel->resolution == 12)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 4096;
	else if (auxadc_channel->resolution == 15)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 32768;

	if (__ratelimit(&ratelimit)) {
		pr_err("[%s] ch_idx = %d, channel = %d, reg_val = 0x%x, adc_result = %d\n",
			__func__, channel, auxadc_channel->ch_num, reg_val, adc_result);
	}

	if (channel == AUXADC_LIST_BATTEMP && dbg_flag == 0) {
		dbg_count++;
		if (battmp != 0 &&
		    (adc_result < 200 || ((adc_result - battmp) > 100) || ((battmp - adc_result) > 100))) {
			pr_err("VBIF28_OC_RAW_STATUS:%d\n",
				pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC));
			if (pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC) == 1)
				pmic_set_register_value(PMIC_RG_INT_STATUS_VBIF28_OC, 1);
			/* dump debug log when VBAT being abnormal */
			pr_err("old: %d, new: %d\n", battmp, adc_result);
			adc_result = wk_auxadc_battmp_dbg(adc_result);
			pr_err("VBIF28_OC_RAW_STATUS:%d\n",
				pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC));
			if (pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC) == 1)
				pmic_set_register_value(PMIC_RG_INT_STATUS_VBIF28_OC, 1);
#if AEE_DBG
			if (aee_count < 2)
				aee_kernel_warning("PMIC AUXADC:BAT TEMP", "BAT TEMP");
			else
				pr_err("aee_count=%d\n", aee_count);
			aee_count++;
#endif
		} else if (dbg_count % 50 == 0)
			/* dump debug log in normal case */
			wk_auxadc_battmp_dbg(adc_result);
		battmp = adc_result;
	}
	/*--Monitor MTS Thread--*/
	if (channel == AUXADC_LIST_BATADC)
		mt6356_auxadc_monitor_mts_regs();
	/* Audio request HPOFS to return raw data */
	if (channel == AUXADC_LIST_HPOFS_CAL)
		return reg_val * auxadc_channel->r_val;
	else
		return adc_result;
}

void adc_dbg_init(void)
{
	unsigned short i;
	unsigned int addr = 0x740;

	/* All of AUXADC */
	for (i = 0; addr <= 0x934; i++) {
		adc_dbg_addr[i] = addr;
		addr += 0x2;
	}
	/* Clock related */
	adc_dbg_addr[i++] = 0x70A;
	adc_dbg_addr[i++] = 0x70C;
	/* RST related */
	adc_dbg_addr[i++] = 0x70E;
	/* Others */
	adc_dbg_addr[i++] = 0x606;
	adc_dbg_addr[i++] = 0x8FA;
	adc_dbg_addr[i] = 0x648;
}

void mt6356_auxadc_init(void)
{
	unsigned char i;

	pr_err("%s\n", __func__);
	wake_lock_init(&pmic_auxadc_wake_lock,
			WAKE_LOCK_SUSPEND, "MT6356 AuxADC wakelock");
	mutex_init(&pmic_adc_mutex);

	/* set channel 0, 7 as 15 bits, others = 12 bits  000001000001*/
	pmic_set_register_value(PMIC_RG_STRUP_AUXADC_RSTB_SEL, 1);
	pmic_set_register_value(PMIC_RG_STRUP_AUXADC_RSTB_SW, 1);

#if 0 /* de-featured in MT6356*/
	pmic_set_register_value(PMIC_AUXADC_MDBG_DET_EN, 0);
	pmic_set_register_value(PMIC_AUXADC_MDBG_DET_PRD, 0x40);
#endif
#if 0
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_PRD, 0x40);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_WKUP_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_SRCLKEN_IND, 0);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_START_SEL, 1);
#else
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_EN, 0);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_START_SEL, 0);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_RDY_ST_PRD, 0xFA);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_RDY_ST_EN, 1);
#endif
	pmic_set_register_value(PMIC_AUXADC_CK_AON, 0);
	pmic_set_register_value(PMIC_AUXADC_DATA_REUSE_SEL, 0);
	pmic_set_register_value(PMIC_AUXADC_DATA_REUSE_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_TRIM_CH0_SEL, 0);

	/* Check VBIF28 OC status */
	pmic_mask_interrupt(INT_VBIF28_OC, "PMIC_AUXADC");
	pmic_enable_interrupt(INT_VBIF28_OC, 1, "PMIC_AUXADC");
	/* update VBIF28 by AUXADC */
	g_pmic_pad_vbif28_vol = pmic_get_auxadc_value(AUXADC_LIST_VBIF);
	if (g_pmic_pad_vbif28_vol < 2500) {
		for (i = 0; i < 5; i++) {
			pr_err("VBIF28_OC:%d\n", pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC));
			if (pmic_get_register_value(PMIC_RG_INT_RAW_STATUS_VBIF28_OC) != 1)
				break;
			pmic_set_register_value(PMIC_RG_INT_STATUS_VBIF28_OC, 1);
			mdelay(1);
		}
		if (i >= 5) {
			/* AEE warning is not ready */
			WARN_ON(1);
		} else
			g_pmic_pad_vbif28_vol = pmic_get_auxadc_value(AUXADC_LIST_VBIF);
	}
	battmp = pmic_get_auxadc_value(AUXADC_LIST_BATTEMP);
	mts_adc = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
	pr_err("****[%s] VBIF28 = %d, BAT TEMP = %d, MTS_ADC = 0x%x\n",
		__func__, pmic_get_vbif28_volt(), battmp, mts_adc);
	mtk_idle_notifier_register(&pmic_auxadc_nb);
	adc_dbg_init();
	pr_info("****[%s] DONE\n", __func__);
}
EXPORT_SYMBOL(mt6356_auxadc_init);

#define MT6356_AUXADC_DEBUG(_reg)                                       \
{                                                                       \
	value = pmic_get_register_value(_reg);				\
	snprintf(buf+strlen(buf), 1024, "%s = 0x%x\n", #_reg, value);	\
	pr_err("[%s] %s = 0x%x\n", __func__, #_reg,			\
		pmic_get_register_value(_reg));			\
}

void mt6356_auxadc_dump_regs(char *buf)
{
	int value;

	snprintf(buf+strlen(buf), 1024, "====| %s |====\n", __func__);
	MT6356_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_RSTB_SEL);
	MT6356_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_RSTB_SW);
	MT6356_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_START_SEL);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_EN);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_PRD);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_WKUP_EN);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_SRCLKEN_IND);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_CK_AON);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_DATA_REUSE_SEL);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_DATA_REUSE_EN);
	MT6356_AUXADC_DEBUG(PMIC_AUXADC_TRIM_CH0_SEL);
}
EXPORT_SYMBOL(mt6356_auxadc_dump_regs);

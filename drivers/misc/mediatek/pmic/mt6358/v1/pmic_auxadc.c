/*
 * Copyright (C) 2017 MediaTek Inc.

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
#include <linux/ratelimit.h>
#include <linux/timekeeping.h>
#include <linux/math64.h>
#include <linux/of_address.h>

#include "include/pmic.h"
#include "include/pmic_auxadc.h"
#include <mt-plat/aee.h>
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_auxadc_intf.h>

#ifdef CONFIG_MTK_PMIC_WRAP_HAL
#include <mach/mtk_pmic_wrap.h>
#endif

#if (CONFIG_MTK_GAUGE_VERSION == 30)
#include <mt-plat/mtk_battery.h>
#endif

#define AEE_DBG 0

static unsigned int count_time_out = 100;
static unsigned int total_time_out;
static int battmp;
#if AEE_DBG
static unsigned int aee_count;
#endif
static unsigned int dbg_count;
static unsigned char dbg_flag;

static struct wake_lock  pmic_auxadc_wake_lock;
static struct mutex pmic_adc_mutex;
static DEFINE_MUTEX(auxadc_ch3_mutex);

#define DBG_REG_SIZE 384
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


static unsigned int g_DEGC;
static unsigned int g_O_VTS;
static unsigned int g_O_SLOPE_SIGN;
static unsigned int g_O_SLOPE;
static unsigned int g_CALI_FROM_EFUSE_EN;
static unsigned int g_GAIN_AUX;
static unsigned int g_SIGN_AUX;
static unsigned int g_GAIN_BGRL;
static unsigned int g_SIGN_BGRL;
static unsigned int g_TEMP_L_CALI;
static unsigned int g_GAIN_BGRH;
static unsigned int g_SIGN_BGRH;
static unsigned int g_TEMP_H_CALI;
static unsigned int g_AUXCALI_EN;
static unsigned int g_BGRCALI_EN;

static int wk_aux_cali(int T_curr, int vbat_out)
{
	signed long long coeff_gain_aux = 0;
	signed long long vbat_cali = 0;

	coeff_gain_aux = (317220 + 11960 * (signed long long)g_GAIN_AUX);
	vbat_cali = div_s64((vbat_out * (T_curr - 250) * coeff_gain_aux), 255);
	vbat_cali = div_s64(vbat_cali, 1000000000);
	if (g_SIGN_AUX == 0)
		vbat_out += vbat_cali;
	else
		vbat_out -= vbat_cali;
	return vbat_out;
}

static int wk_bgr_cali(int T_curr, int vbat_out)
{
	signed long long coeff_gain_bgr = 0;
	signed int T_L = -100 + g_TEMP_L_CALI * 25;
	signed int T_H = 600 + g_TEMP_H_CALI * 25;

	if (T_curr < T_L) {
		coeff_gain_bgr = (127 + 8 * (signed long long)g_GAIN_BGRL);
		if (g_SIGN_BGRL == 0)
			vbat_out += div_s64((vbat_out * (T_curr - T_L) * coeff_gain_bgr), 127000000);
		else
			vbat_out -= div_s64((vbat_out * (T_curr - T_L) * coeff_gain_bgr), 127000000);
	} else if (T_curr > T_H) {
		coeff_gain_bgr = (127 + 8 * (signed long long)g_GAIN_BGRH);
		if (g_SIGN_BGRH == 0)
			vbat_out -= div_s64((vbat_out * (T_curr - T_H) * coeff_gain_bgr), 127000000);
		else
			vbat_out += div_s64((vbat_out * (T_curr - T_H) * coeff_gain_bgr), 127000000);
	}

	return vbat_out;
}

/* vbat_out unit is 0.1mV, vthr unit is mV */
int wk_vbat_cali(int vbat_out, int vthr)
{
	int mV_diff = 0;
	int T_curr = 0; /* unit: 0.1 degrees C*/
	int vbat_out_old = vbat_out;

	mV_diff = vthr - g_O_VTS * 1800 / 4096;
	if (g_O_SLOPE_SIGN == 0)
		T_curr = mV_diff * 10000 / (signed int)(1681 + g_O_SLOPE * 10);
	else
		T_curr = mV_diff * 10000 / (signed int)(1681 - g_O_SLOPE * 10);
	T_curr = (g_DEGC * 10 / 2) - T_curr;
	/*pr_info("%d\n", T_curr);*/

	if (g_AUXCALI_EN == 1) {
		vbat_out = wk_aux_cali(T_curr, vbat_out);
		/*pr_info("vbat_out_auxcali = %d\n", vbat_out);*/
	}

	if (g_BGRCALI_EN == 1) {
		vbat_out = wk_bgr_cali(T_curr, vbat_out);
		/*pr_info("vbat_out_bgrcali = %d\n", vbat_out);*/
	}

	if (abs(vbat_out - vbat_out_old) > 1000) {
		pr_notice("vbat_out_old=%d, vthr=%d, T_curr=%d, vbat_out=%d\n",
			vbat_out_old, vthr, T_curr, vbat_out);
		pr_notice("%d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			g_DEGC, g_O_VTS, g_O_SLOPE_SIGN, g_O_SLOPE,
			g_SIGN_AUX, g_SIGN_BGRL, g_SIGN_BGRH,
			g_AUXCALI_EN, g_BGRCALI_EN,
			g_GAIN_AUX, g_GAIN_BGRL, g_GAIN_BGRH,
			g_TEMP_L_CALI, g_TEMP_H_CALI);
		aee_kernel_warning("PMIC AUXADC CALI", "VBAT CALI");
	}

	return vbat_out;
}

void wk_auxadc_bgd_ctrl(unsigned char en)
{
	pmic_config_interface(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX_ADDR, en,
		PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX_MASK,
		PMIC_AUXADC_BAT_TEMP_IRQ_EN_MAX_SHIFT);
	pmic_config_interface(PMIC_AUXADC_BAT_TEMP_EN_MAX_ADDR, en,
		PMIC_AUXADC_BAT_TEMP_EN_MAX_MASK,
		PMIC_AUXADC_BAT_TEMP_EN_MAX_SHIFT);

	pmic_config_interface(PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN_ADDR, en,
		PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN_MASK,
		PMIC_AUXADC_BAT_TEMP_IRQ_EN_MIN_SHIFT);
	pmic_config_interface(PMIC_AUXADC_BAT_TEMP_EN_MIN_ADDR, en,
		PMIC_AUXADC_BAT_TEMP_EN_MIN_MASK,
		PMIC_AUXADC_BAT_TEMP_EN_MIN_SHIFT);

	pmic_config_interface(PMIC_RG_INT_EN_BAT_TEMP_H_ADDR, en,
		PMIC_RG_INT_EN_BAT_TEMP_H_MASK,
		PMIC_RG_INT_EN_BAT_TEMP_H_SHIFT);
	pmic_config_interface(PMIC_RG_INT_EN_BAT_TEMP_L_ADDR, en,
		PMIC_RG_INT_EN_BAT_TEMP_L_MASK,
		PMIC_RG_INT_EN_BAT_TEMP_L_SHIFT);
}

void pmic_auxadc_suspend(void)
{
	wk_auxadc_bgd_ctrl(0);
	battmp = 0;
}

void pmic_auxadc_resume(void)
{
	wk_auxadc_bgd_ctrl(1);
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
				HKLOG("%d %s\n", pmic_adc_dbg[dbg_stamp].ktime_sec, reg_log);
				strncpy(reg_log, "", 860);
			}
			snprintf(reg_str, 20, "Reg[0x%x]=0x%x, ", adc_dbg_addr[j], pmic_adc_dbg[dbg_stamp].reg[j]);
			strncat(reg_log, reg_str, 860);
		}
		pr_notice("[%s] %d %d %s\n", __func__, dbg_stamp, pmic_adc_dbg[dbg_stamp].ktime_sec, reg_log);
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
		HKLOG("Another dbg is running\n");
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
	pr_notice("BAT_TEMP1: %d, BAT_TEMP2:%d, VBIF:%d, BAT_TEMP3:%d, BATID:%d\n",
		bat_temp, bat_temp2, vbif, bat_temp3, bat_id);

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

struct pmic_auxadc_channel_new mt6358_auxadc_channel[] = {
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
	{15, 1, 10, PMIC_AUXADC_RQST_CH10, /* DCXO */
		PMIC_AUXADC_ADC_RDY_DCXO_BY_AP, PMIC_AUXADC_ADC_OUT_DCXO_BY_AP},
	{12, 1, 5, PMIC_AUXADC_RQST_CH5, /* ACCDET MULTI-KEY */
		PMIC_AUXADC_ADC_RDY_CH5, PMIC_AUXADC_ADC_OUT_CH5},
	{15, 1, 7, PMIC_AUXADC_RQST_CH7, /* TSX */
		PMIC_AUXADC_ADC_RDY_CH7_BY_AP, PMIC_AUXADC_ADC_OUT_CH7_BY_AP},
	{15, 1, 9, PMIC_AUXADC_RQST_CH9, /* HP OFFSET CAL */
		PMIC_AUXADC_ADC_RDY_CH9, PMIC_AUXADC_ADC_OUT_CH9},
	{15, 3, 1, PMIC_AUXADC_RQST_CH1, /* ISENSE */
		PMIC_AUXADC_ADC_RDY_CH1_BY_AP, PMIC_AUXADC_ADC_OUT_CH1_BY_AP},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4_BY_THR1, /* VCORE_TEMP */
		PMIC_AUXADC_ADC_RDY_CH4_BY_THR1, PMIC_AUXADC_ADC_OUT_CH4_BY_THR1},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4_BY_THR2, /* VPROC_TEMP */
		PMIC_AUXADC_ADC_RDY_CH4_BY_THR2, PMIC_AUXADC_ADC_OUT_CH4_BY_THR2},
	{12, 1, 4, PMIC_AUXADC_RQST_CH4_BY_THR3, /* VGPU_TEMP */
		PMIC_AUXADC_ADC_RDY_CH4_BY_THR3, PMIC_AUXADC_ADC_OUT_CH4_BY_THR3},
	{12, 1.5, 6, PMIC_AUXADC_RQST_CH6, /* DCXO voltage */
		PMIC_AUXADC_ADC_RDY_CH6, PMIC_AUXADC_ADC_OUT_CH6},
};

static bool mts_enable = true;
static int mts_timestamp;
static unsigned int mts_count;
static unsigned int mts_adc;
static struct wake_lock mts_monitor_wake_lock;
static struct mutex mts_monitor_mutex;
static struct task_struct *mts_thread_handle;
/*--wake up thread to polling MTS data--*/
void wake_up_mts_monitor(void)
{
	HKLOG("[%s]\n", __func__);
	if (mts_thread_handle != NULL) {
		wake_lock(&mts_monitor_wake_lock);
		wake_up_process(mts_thread_handle);
	} else
		pr_notice(PMICTAG "[%s] mts_thread_handle not ready\n", __func__);
}

/*--Monitor MTS reg--*/
static void mt6358_mts_reg_dump(void)
{
	/* AUXADC_ADC_RDY_MDRT & AUXADC_ADC_OUT_MDRT */
	pr_notice("AUXADC_ADC15 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC15));
	pr_notice("AUXADC_ADC16 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC16));
	pr_notice("AUXADC_ADC17 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC17));
	pr_notice("AUXADC_ADC31 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC31));
	pr_notice("AUXADC_MDRT_0 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_MDRT_0));
	pr_notice("AUXADC_MDRT_1 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_MDRT_1));
	pr_notice("AUXADC_MDRT_2 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_MDRT_2));
	pr_notice("AUXADC_MDRT_3 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_MDRT_3));
	pr_notice("AUXADC_MDRT_4 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_MDRT_4));
	/*--AUXADC CLK--*/
	pr_notice("RG_AUXADC_CK_PDN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN));
	pr_notice("RG_AUXADC_CK_PDN_HWEN = 0x%x\n", pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN));
}

void mt6358_auxadc_monitor_mts_regs(void)
{
	unsigned int count = 0;
	int mts_timestamp_cur = 0;
	unsigned int mts_adc_tmp = 0;

	if (mts_adc == 0)
		return;
	mts_timestamp_cur = (int)get_monotonic_coarse().tv_sec;
	if ((mts_timestamp_cur - mts_timestamp) < 5)
		return;
	mts_timestamp = mts_timestamp_cur;
	mts_adc_tmp = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
	pr_notice("[MTS_ADC] OLD = 0x%x, NOW = 0x%x, CNT = %d\n", mts_adc, mts_adc_tmp, mts_count);

	if (mts_adc ==  mts_adc_tmp)
		mts_count++;
	else
		mts_count = 0;

	if (mts_count >= 7 && mts_count < 9) {
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
		pwrap_dump_all_register();
#endif
		mt6358_mts_reg_dump();
		/*--AUXADC CH7--*/
		pmic_get_auxadc_value(AUXADC_LIST_TSX);
		pmic_set_hk_reg_value(PMIC_AUXADC_RQST_CH7_BY_GPS, 1);
		udelay(10);
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_GPS) != 1) {
			usleep_range(1300, 1500);
			if ((count++) > count_time_out)
				break;
		}
		pmic_set_hk_reg_value(PMIC_AUXADC_RQST_CH7_BY_MD, 1);
		udelay(10);
		while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_CH7_BY_MD) != 1) {
			usleep_range(1300, 1500);
			if ((count++) > count_time_out)
				break;
		}
		pr_notice("AUXADC_ADC15 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC15));
		pr_notice("AUXADC_ADC16 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC16));
		pr_notice("AUXADC_ADC17 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC17));
	}
	if (mts_count > 30) {
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
		pwrap_dump_all_register();
#endif
		pr_notice("DEW_READ_TEST = 0x%x\n", pmic_get_register_value(PMIC_DEW_READ_TEST));
		/*--AUXADC--*/
		mt6358_mts_reg_dump();
		/*--AUXADC CH7--*/
		pmic_get_auxadc_value(AUXADC_LIST_TSX);
		pr_notice("AUXADC_ADC15 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC15));
		pr_notice("AUXADC_ADC16 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC16));
		pr_notice("AUXADC_ADC17 = 0x%x\n", upmu_get_reg_value(MT6358_AUXADC_ADC17));
		mts_count = 0;
		wake_up_mts_monitor();
	}
	mts_adc = mts_adc_tmp;
}

int mts_kthread(void *x)
{
	unsigned int i = 0, j = 0;
	unsigned int polling_cnt = 0;
	unsigned int ktrhead_mts_adc = 0;

	/* Run on a process content */
	while (1) {
		mutex_lock(&mts_monitor_mutex);
		polling_cnt = 0;
		ktrhead_mts_adc = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
		while (mts_adc == ktrhead_mts_adc) {
			i = 0;
			j = 0;
			while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_MDRT) == 1) {
				if (i > 1000) {
					pr_notice("[MTS_ADC] no trigger\n");
					break;
				}
				i++;
				mdelay(1);
			}
			while (pmic_get_register_value(PMIC_AUXADC_ADC_RDY_MDRT) == 0) {
				if (j > count_time_out) {
					pr_notice("[MTS_ADC] no ready\n");
					break;
				}
				j++;
				mdelay(1);
			}
			ktrhead_mts_adc = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
			if (polling_cnt % 20 == 0) {
				pr_notice("[MTS_ADC] (%d) Ready at %d(%d)ms, Reg_MDRT=0x%x, MDRT_OUT=%d\n",
					polling_cnt, j, i,
					upmu_get_reg_value(PMIC_AUXADC_ADC_OUT_MDRT_ADDR),
					ktrhead_mts_adc);
			}
			if (polling_cnt == 156) { /* 156 * 32ms ~= 5s*/
				pr_notice("[MTS_ADC] (%d) reset AUXADC\n",
					polling_cnt);
				pmic_set_register_value(PMIC_RG_AUXADC_RST, 1);
				pmic_set_register_value(PMIC_RG_AUXADC_RST, 0);
			}
			if (polling_cnt >= 624) { /* 624 * 32ms ~= 20s*/
				mt6358_mts_reg_dump();
				aee_kernel_warning("PMIC AUXADC:MDRT", "MDRT");
				break;
			}
			polling_cnt++;
		}
		mutex_unlock(&mts_monitor_mutex);
		wake_unlock(&mts_monitor_wake_lock);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return 0;
}
/*--Monitor MTS reg End--*/

static void mt6358_auxadc_timeout_dump(unsigned short ch_num)
{
	pr_notice("[%s] (%d) Time out! STA0=0x%x, STA1=0x%x, STA2=0x%x\n",
		__func__,
		ch_num,
		upmu_get_reg_value(MT6358_AUXADC_STA0),
		upmu_get_reg_value(MT6358_AUXADC_STA1),
		upmu_get_reg_value(MT6358_AUXADC_STA2));
	pr_notice("[%s] Reg[0x%x]=0x%x, RG_AUXADC_CK_PDN_HWEN=%d, RG_AUXADC_CK_TSTSEL=%d, RG_SMPS_CK_TSTSEL=%d\n",
		__func__,
		MT6358_STRUP_CON6, upmu_get_reg_value(MT6358_STRUP_CON6),
		pmic_get_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN),
		pmic_get_register_value(PMIC_RG_AUXADC_CK_TSTSEL),
		pmic_get_register_value(PMIC_RG_SMPS_CK_TSTSEL));
}

static int get_device_adc_out(unsigned short channel,
	struct pmic_auxadc_channel_new *auxadc_channel)
{
	int tdet_tmp = 0;
	unsigned int count = 0;
	unsigned short reg_val = 0;
	bool is_timeout = false;

	switch (channel) {
	case AUXADC_LIST_VBIF:
		tdet_tmp = 1;
		HKLOG("pmic auxadc baton_tdet_en should be zero\n");
		pmic_set_hk_reg_value(PMIC_RG_BATON_TDET_EN, 0);
		break;
	/* No need to set AUXADC_DCXO_CH4_MUX_AP_SEL from MT6358 */
	default:
		break;
	}

	pmic_set_hk_reg_value(auxadc_channel->channel_rqst, 1);
	udelay(10);

	while (pmic_get_register_value(auxadc_channel->channel_rdy) != 1) {
		usleep_range(1300, 1500);
		if ((count++) > count_time_out) {
			mt6358_auxadc_timeout_dump(auxadc_channel->ch_num);
			is_timeout = true;
			break;
		}
	}
	if (is_timeout) {
		total_time_out++;
		if (total_time_out > 10 && total_time_out < 13) {
			pmic_set_hk_reg_value(PMIC_AUXADC_DATA_REUSE_EN, 1);
			pr_notice("AUXADC timeout, enable DATA REUSE\n");
			aee_kernel_warning("PMIC AUXADC:TIMEOUT", "");
		}
	} else if (total_time_out != 0) {
		total_time_out = 0;
		pr_notice("AUXADC timeout resolved, disable DATA REUSE\n");
		pmic_set_hk_reg_value(PMIC_AUXADC_DATA_REUSE_EN, 0);
	}

	reg_val = pmic_get_register_value(auxadc_channel->channel_out);
	return (int)reg_val;
}

int mt6358_get_auxadc_nolock(u8 channel)
{
	signed int adc_result = 0, reg_val = 0;
	struct pmic_auxadc_channel_new *auxadc_channel;

	if (channel - AUXADC_LIST_MT6358_START < 0 ||
			channel - AUXADC_LIST_MT6358_END > 0) {
		pr_notice("[%s] Invalid channel(%d)\n", __func__, channel);
		return -EINVAL;
	}
	auxadc_channel =
		&mt6358_auxadc_channel[channel-AUXADC_LIST_MT6358_START];

	/*
	 * be careful when access AUXADC_LIST_BATTEMP
	 * because this API don't hold auxadc_ch3_mutex
	 */
	reg_val = get_device_adc_out(channel, auxadc_channel);

	/* Audio request HPOFS to return raw data */
	if (channel == AUXADC_LIST_HPOFS_CAL)
		adc_result =  reg_val;
	else if (auxadc_channel->resolution == 12)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 4096;
	else if (auxadc_channel->resolution == 15)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 32768;
	pr_notice("[%s] ch_idx = %d, channel = %d, reg_val = 0x%x, adc_result = %d\n",
		__func__, channel, auxadc_channel->ch_num,
		reg_val, adc_result);

	return adc_result;
}

int mt6358_get_auxadc_value(u8 channel)
{
	int bat_cur = 0, is_charging = 0;
	signed int adc_result = 0, reg_val = 0;
	signed int vbat_cali = 0, vthr = 0;
	struct pmic_auxadc_channel_new *auxadc_channel;
	static DEFINE_RATELIMIT_STATE(ratelimit, 1 * HZ, 5);

	if (channel - AUXADC_LIST_MT6358_START < 0 ||
			channel - AUXADC_LIST_MT6358_END > 0) {
		pr_notice("[%s] Invalid channel(%d)\n", __func__, channel);
		return -EINVAL;
	}
	auxadc_channel =
		&mt6358_auxadc_channel[channel-AUXADC_LIST_MT6358_START];

	pmic_auxadc_lock();
	if (channel == AUXADC_LIST_BATTEMP)
		mutex_lock(&auxadc_ch3_mutex);

	reg_val = get_device_adc_out(channel, auxadc_channel);

	if (channel == AUXADC_LIST_BATTEMP)
		mutex_unlock(&auxadc_ch3_mutex);
	pmic_auxadc_unlock();

	/* Audio request HPOFS to return raw data */
	if (channel == AUXADC_LIST_HPOFS_CAL)
		adc_result =  reg_val;
	else if (auxadc_channel->resolution == 12)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 4096;
	else if (auxadc_channel->resolution == 15)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 32768;

	if (channel != AUXADC_LIST_VCORE_TEMP &&
	    channel != AUXADC_LIST_VPROC_TEMP &&
	    channel != AUXADC_LIST_VGPU_TEMP &&
	    __ratelimit(&ratelimit)) {
		if (channel == AUXADC_LIST_BATTEMP) {
#if (CONFIG_MTK_GAUGE_VERSION == 30)
			is_charging = gauge_get_current(&bat_cur);
#endif
			if (is_charging == 0)
				bat_cur = 0 - bat_cur;
			pr_notice("[%s] ch_idx = %d, channel = %d, bat_cur = %d, reg_val = 0x%x, adc_result = %d\n",
				__func__, channel, auxadc_channel->ch_num,
				bat_cur, reg_val, adc_result);
		} else if (channel == AUXADC_LIST_BATADC) {
			vthr = pmic_get_auxadc_value(AUXADC_LIST_CHIP_TEMP);
			vbat_cali = DIV_ROUND_CLOSEST(wk_vbat_cali(adc_result * 10, vthr), 10);
			pr_notice("[%s] ch_idx = %d, channel = %d, reg_val = 0x%x, old_vbat = %d, vthr = %d, adc_result = %d\n",
				__func__, channel, auxadc_channel->ch_num,
				reg_val, adc_result, vthr, vbat_cali);
			adc_result = vbat_cali;
		} else {
			pr_notice("[%s] ch_idx = %d, channel = %d, reg_val = 0x%x, adc_result = %d\n",
				__func__, channel, auxadc_channel->ch_num,
				reg_val, adc_result);
		}
	} else {
		HKLOG("[%s] ch_idx = %d, channel = %d, reg_val = 0x%x, adc_result = %d\n",
			__func__, channel, auxadc_channel->ch_num,
			reg_val, adc_result);
	}

	if (channel == AUXADC_LIST_BATTEMP && dbg_flag == 0) {
		dbg_count++;
		if (battmp != 0 &&
		    (adc_result < 200 || ((adc_result - battmp) > 100) || ((battmp - adc_result) > 100))) {
			/* dump debug log when VBAT being abnormal */
			pr_notice("old: %d, new: %d\n", battmp, adc_result);
			adc_result = wk_auxadc_battmp_dbg(adc_result);
#if AEE_DBG
			if (aee_count < 2)
				aee_kernel_warning("PMIC AUXADC:BAT TEMP", "BAT TEMP");
			else
				pr_notice("aee_count=%d\n", aee_count);
			aee_count++;
#endif
		} else if (dbg_count % 50 == 0)
			/* dump debug log in normal case */
			wk_auxadc_battmp_dbg(adc_result);
		battmp = adc_result;
	}

	/*--Monitor MTS Thread--*/
	if ((channel == AUXADC_LIST_BATADC) && mts_enable)
		mt6358_auxadc_monitor_mts_regs();

	return adc_result;
}

void parsing_cust_setting(void)
{
	struct device_node *np;
	unsigned int disable_modem;

	/* check customer setting */
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt-pmic-custom-setting");
	if (!np) {
		HKLOG("[%s]Failed to find device-tree node\n", __func__);
		return;
	}

	if (!of_property_read_u32(np, "disable-modem", &disable_modem)) {
		if (disable_modem)
			mts_enable = false;
	}

	of_node_put(np);
}

void adc_dbg_init(void)
{
	unsigned short i;
	unsigned int addr = 0x1000;

	/* All of AUXADC */
	for (i = 0; addr <= 0x1266; i++) {
		adc_dbg_addr[i] = addr;
		addr += 0x2;
	}
	/* Clock related */
	adc_dbg_addr[i++] = MT6358_HK_TOP_CLK_CON0;
	adc_dbg_addr[i++] = MT6358_HK_TOP_CLK_CON1;
	/* RST related */
	adc_dbg_addr[i++] = MT6358_HK_TOP_RST_CON0;
	/* Others */
	adc_dbg_addr[i++] = MT6358_BATON_ANA_CON0;
	adc_dbg_addr[i++] = MT6358_PCHR_VREF_ELR_0;
	adc_dbg_addr[i++] = MT6358_PCHR_VREF_ELR_1;
}

static void mts_thread_init(void)
{
	mts_thread_handle = kthread_create(mts_kthread, (void *)NULL, "mts_thread");
	if (IS_ERR(mts_thread_handle)) {
		mts_thread_handle = NULL;
		pr_notice(PMICTAG "[adc_kthread] creation fails\n");
	} else {
		HKLOG("[adc_kthread] kthread_create Done\n");
	}
}

void mt6358_adc_cali_init(void)
{
	unsigned int efuse = 0;

	if (pmic_get_register_value(PMIC_AUXADC_EFUSE_ADC_CALI_EN) == 1) {
		g_DEGC = pmic_get_register_value(PMIC_AUXADC_EFUSE_DEGC_CALI);
		if (g_DEGC < 38 || g_DEGC > 60)
			g_DEGC = 53;
		g_O_VTS = pmic_get_register_value(PMIC_AUXADC_EFUSE_O_VTS);
		g_O_SLOPE_SIGN = pmic_get_register_value(PMIC_AUXADC_EFUSE_O_SLOPE_SIGN);
		g_O_SLOPE = pmic_get_register_value(PMIC_AUXADC_EFUSE_O_SLOPE);
	} else {
		g_DEGC = 50;
		g_O_VTS = 1600;
	}

	efuse = pmic_Read_Efuse_HPOffset(39);
	g_CALI_FROM_EFUSE_EN = (efuse >> 2) & 0x1;
	if (g_CALI_FROM_EFUSE_EN == 1) {
		g_SIGN_AUX = (efuse >> 3) & 0x1;
		g_AUXCALI_EN = (efuse >> 6) & 0x1;
		g_GAIN_AUX = (efuse >> 8) & 0xFF;
	} else {
		g_SIGN_AUX = 0;
		g_AUXCALI_EN = 1;
		g_GAIN_AUX = 106;
	}
	g_SIGN_BGRL = (efuse >> 4) & 0x1;
	g_SIGN_BGRH = (efuse >> 5) & 0x1;
	g_BGRCALI_EN = (efuse >> 7) & 0x1;

	efuse = pmic_Read_Efuse_HPOffset(40);
	g_GAIN_BGRL = (efuse >> 9) & 0x7F;
	efuse = pmic_Read_Efuse_HPOffset(41);
	g_GAIN_BGRH = (efuse >> 9) & 0x7F;

	efuse = pmic_Read_Efuse_HPOffset(42);
	g_TEMP_L_CALI = (efuse >> 10) & 0x7;
	g_TEMP_H_CALI = (efuse >> 13) & 0x7;

	pr_info("%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		g_DEGC, g_O_VTS, g_O_SLOPE_SIGN, g_O_SLOPE,
		g_CALI_FROM_EFUSE_EN, g_SIGN_AUX, g_SIGN_BGRL, g_SIGN_BGRH,
		g_AUXCALI_EN, g_BGRCALI_EN,
		g_GAIN_AUX, g_GAIN_BGRL, g_GAIN_BGRH,
		g_TEMP_L_CALI, g_TEMP_H_CALI);
}

void mt6358_auxadc_init(void)
{
	HKLOG("%s\n", __func__);
	wake_lock_init(&pmic_auxadc_wake_lock,
			WAKE_LOCK_SUSPEND, "PMIC AuxADC wakelock");
	mutex_init(&pmic_adc_mutex);

	parsing_cust_setting();
	if (mts_enable) {
		wake_lock_init(&mts_monitor_wake_lock,
			WAKE_LOCK_SUSPEND, "PMIC MTS Monitor wakelock");
		mutex_init(&mts_monitor_mutex);
		mts_adc = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_MDRT);
		mts_thread_init();
	}

	/* Remove register setting which is set by PMIC initial setting in PL */
	battmp = pmic_get_auxadc_value(AUXADC_LIST_BATTEMP);

	/* update VBIF28 by AUXADC */
	g_pmic_pad_vbif28_vol = pmic_get_auxadc_value(AUXADC_LIST_VBIF);
	pr_info("****[%s] VBIF28 = %d, BAT TEMP = %d, MTS_ADC = 0x%x\n",
		__func__, pmic_get_vbif28_volt(), battmp, mts_adc);

	adc_dbg_init();
	pr_info("****[%s] DONE, BAT TEMP = %d, MTS_ADC = 0x%x\n",
		__func__, battmp, mts_adc);

	mt6358_adc_cali_init();
}
EXPORT_SYMBOL(mt6358_auxadc_init);

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

#include "include/pmic.h"
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_auxadc_intf.h>

static int count_time_out = 100;

static struct wake_lock  mt6355_auxadc_wake_lock;
static struct mutex mt6355_adc_mutex;
static DEFINE_MUTEX(auxadc_ch3_mutex);

unsigned int wk_auxadc_vsen_tdet_ctrl(unsigned char en_check)
{
	if (en_check) {
		if ((!pmic_get_register_value(PMIC_RG_ADCIN_VSEN_MUX_EN)) &&
			(pmic_get_register_value(PMIC_BATON_TDET_EN))) {
			PMICLOG("[%s] vbif non 1th %d\n", __func__, g_pmic_pad_vbif28_vol);
			return g_pmic_pad_vbif28_vol;
		}
		pr_err("[%s] vbif 1th a!\n", __func__);
	} else {
		pmic_set_register_value(PMIC_RG_ADCIN_VSEN_MUX_EN, 0);
		pmic_set_register_value(PMIC_BATON_TDET_EN, 1);
		pr_err("[%s] vbif 1th b!\n", __func__);
	}
	return 0;
}

void wk_auxadc_bgd_ctrl(unsigned char en)
{
	if (en) {
		/*--Enable BATON TDET EN--*/
		pmic_config_interface_nolock(MT6355_PCHR_VREF_ANA_DA0, 0x1, 0x1, 2);
		/*--BAT TEMP MAX DET EN--*/
		pmic_config_interface_nolock(MT6355_AUXADC_BAT_TEMP_4, 0x3, 0x3, 12);
		/*--BAT TEMP MIN DET EN--*/
		pmic_config_interface_nolock(MT6355_AUXADC_BAT_TEMP_5, 0x3, 0x3, 12);
		/*--BAT TEMP DET EN--*/
		pmic_config_interface_nolock(MT6355_INT_CON1_SET, 0x00C0, 0xffff, 0);
	} else {
		/*--BAT TEMP MAX DET EN--*/
		pmic_config_interface_nolock(MT6355_AUXADC_BAT_TEMP_4, 0x0, 0x3, 12);
		/*--BAT TEMP MIN DET EN--*/
		pmic_config_interface_nolock(MT6355_AUXADC_BAT_TEMP_5, 0x0, 0x3, 12);
		/*--BAT TEMP DET EN--*/
		pmic_config_interface_nolock(MT6355_INT_CON1_CLR, 0x00C0, 0xffff, 0);
		/*--Disable BATON TDET EN--*/
		pmic_config_interface_nolock(MT6355_PCHR_VREF_ANA_DA0, 0x0, 0x1, 2);
	}
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

void mt6355_auxadc_lock(void)
{
	wake_lock(&mt6355_auxadc_wake_lock);
	mutex_lock(&mt6355_adc_mutex);
}

void lockadcch3(void)
{
	mutex_lock(&auxadc_ch3_mutex);
}

void unlockadcch3(void)
{
	mutex_unlock(&auxadc_ch3_mutex);
}


void mt6355_auxadc_unlock(void)
{
	mutex_unlock(&mt6355_adc_mutex);
	wake_unlock(&mt6355_auxadc_wake_lock);
}

struct pmic_auxadc_channel mt6355_auxadc_channel[] = {
	{15, 3, PMIC_AUXADC_RQST_CH0, /* BATADC */
		PMIC_AUXADC_ADC_RDY_CH0_BY_AP, PMIC_AUXADC_ADC_OUT_CH0_BY_AP},
	{12, 1, PMIC_AUXADC_RQST_CH2, /* VCDT */
		PMIC_AUXADC_ADC_RDY_CH2, PMIC_AUXADC_ADC_OUT_CH2},
	{12, 2, PMIC_AUXADC_RQST_CH3, /* BAT TEMP */
		PMIC_AUXADC_ADC_RDY_CH3, PMIC_AUXADC_ADC_OUT_CH3},
	{12, 2, PMIC_AUXADC_RQST_BATID, /* BATID */
		PMIC_AUXADC_ADC_RDY_BATID, PMIC_AUXADC_ADC_OUT_BATID},
	{12, 2, PMIC_AUXADC_RQST_CH11, /* VBIF */
		PMIC_AUXADC_ADC_RDY_CH11, PMIC_AUXADC_ADC_OUT_CH11},
	{12, 1, PMIC_AUXADC_RQST_CH4, /* CHIP TEMP */
		PMIC_AUXADC_ADC_RDY_CH4, PMIC_AUXADC_ADC_OUT_CH4},
	{12, 1, PMIC_AUXADC_RQST_CH4, /* DCXO */
		PMIC_AUXADC_ADC_RDY_CH4, PMIC_AUXADC_ADC_OUT_CH4},
	{12, 1, PMIC_AUXADC_RQST_CH5, /* ACCDET MULTI-KEY */
		PMIC_AUXADC_ADC_RDY_CH5, PMIC_AUXADC_ADC_OUT_CH5},
	{15, 1, PMIC_AUXADC_RQST_CH7, /* TSX */
		PMIC_AUXADC_ADC_RDY_CH7_BY_AP, PMIC_AUXADC_ADC_OUT_CH7_BY_AP},
	{15, 1, PMIC_AUXADC_RQST_CH9, /* HP OFFSET CAL */
		PMIC_AUXADC_ADC_RDY_CH9, PMIC_AUXADC_ADC_OUT_CH9},
};
#define MT6355_AUXADC_CHANNEL_MAX	ARRAY_SIZE(mt6355_auxadc_channel)

int mt6355_get_auxadc_value(u8 channel)
{
	int count = 0;
	signed int adc_result = 0, reg_val = 0;
	struct pmic_auxadc_channel *auxadc_channel;

	if (channel - AUXADC_LIST_MT6355_START < 0 ||
			channel - AUXADC_LIST_MT6355_END > 0) {
		pr_err("[%s] Invalid channel(%d)\n", __func__, channel);
		return -EINVAL;
	}
	auxadc_channel =
		&mt6355_auxadc_channel[channel-AUXADC_LIST_MT6355_START];

	if (channel == AUXADC_LIST_VBIF) {
		if (wk_auxadc_vsen_tdet_ctrl(1))
			return g_pmic_pad_vbif28_vol;
	}

	mt6355_auxadc_lock();

	if (channel == AUXADC_LIST_DCXO)
		pmic_set_register_value(PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL, 1);
	if (channel == AUXADC_LIST_MT6355_CHIP_TEMP)
		pmic_set_register_value(PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL, 0);
	if (channel == AUXADC_LIST_BATTEMP)
		mutex_lock(&auxadc_ch3_mutex);

	pmic_set_register_value(auxadc_channel->channel_rqst, 1);
	udelay(10);

	while (pmic_get_register_value(auxadc_channel->channel_rdy) != 1) {
		usleep_range(1300, 1500);
		if ((count++) > count_time_out) {
			pr_err("[%s] (%d) Time out!\n", __func__, channel);
			break;
		}
	}
	reg_val = pmic_get_register_value(auxadc_channel->channel_out);

	if (channel == AUXADC_LIST_BATTEMP)
		mutex_unlock(&auxadc_ch3_mutex);

	mt6355_auxadc_unlock();

	if (channel == AUXADC_LIST_VBIF)
		wk_auxadc_vsen_tdet_ctrl(0);

	if (auxadc_channel->resolution == 12)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 4096;
	else if (auxadc_channel->resolution == 15)
		adc_result = (reg_val * auxadc_channel->r_val *
					VOLTAGE_FULL_RANGE) / 32768;

	pr_info("[%s] ch = %d, reg_val = 0x%x, adc_result = %d\n",
				__func__, channel, reg_val, adc_result);

	/* Audio request HPOPS to return raw data */
	if (channel == AUXADC_LIST_HPOFS_CAL)
		return reg_val * auxadc_channel->r_val;
	else
		return adc_result;
}

void mt6355_auxadc_init(void)
{
	pr_err("%s\n", __func__);
	wake_lock_init(&mt6355_auxadc_wake_lock,
			WAKE_LOCK_SUSPEND, "MT6355 AuxADC wakelock");
	mutex_init(&mt6355_adc_mutex);

	/* set channel 0, 7 as 15 bits, others = 12 bits  000001000001*/
	pmic_set_register_value(PMIC_RG_STRUP_AUXADC_RSTB_SEL, 1);
	pmic_set_register_value(PMIC_RG_STRUP_AUXADC_RSTB_SW, 1);

	/* 4/11, Ricky, Remove initial setting due to MT6353 issue */
	/* pmic_set_register_value(PMIC_RG_STRUP_AUXADC_START_SEL, 1); */

	pmic_set_register_value(PMIC_AUXADC_MDBG_DET_EN, 0);
	pmic_set_register_value(PMIC_AUXADC_MDBG_DET_PRD, 0x40);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_PRD, 0x40);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_WKUP_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_SRCLKEN_IND, 0);
	pmic_set_register_value(PMIC_AUXADC_MDRT_DET_START_SEL, 1);
	pmic_set_register_value(PMIC_AUXADC_CK_AON, 0);
	pmic_set_register_value(PMIC_AUXADC_DATA_REUSE_SEL, 0);
	pmic_set_register_value(PMIC_AUXADC_DATA_REUSE_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_TRIM_CH0_SEL, 0);

	pr_info("****[%s] DONE\n", __func__);

	/* update VBIF28 by AUXADC */
	g_pmic_pad_vbif28_vol = mt6355_get_auxadc_value(AUXADC_LIST_VBIF);
	pr_info("****[%s] VBIF28 = %d\n", __func__, pmic_get_vbif28_volt());
}
EXPORT_SYMBOL(mt6355_auxadc_init);

#define MT6355_AUXADC_DEBUG(_reg)                                       \
{                                                                       \
	value = pmic_get_register_value(_reg);				\
	snprintf(buf+strlen(buf), 1024, "%s = 0x%x\n", #_reg, value);	\
	pr_err("[%s] %s = 0x%x\n", __func__, #_reg,			\
		pmic_get_register_value(_reg));			\
}

void mt6355_auxadc_dump_regs(char *buf)
{
	int value;

	snprintf(buf+strlen(buf), 1024, "====| %s |====\n", __func__);
	MT6355_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_RSTB_SEL);
	MT6355_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_RSTB_SW);
	MT6355_AUXADC_DEBUG(PMIC_RG_STRUP_AUXADC_START_SEL);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_EN);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_PRD);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_WKUP_EN);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_MDRT_DET_SRCLKEN_IND);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_CK_AON);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_DATA_REUSE_SEL);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_DATA_REUSE_EN);
	MT6355_AUXADC_DEBUG(PMIC_AUXADC_TRIM_CH0_SEL);
}
EXPORT_SYMBOL(mt6355_auxadc_dump_regs);

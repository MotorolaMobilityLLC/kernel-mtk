/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include <mt6336/mt6336.h>
#include "flashlight.h"
#include "flashlight-dt.h"

/* device tree should be defined in flashlight-dt.h */
#ifndef MT6336_DTNAME
#define MT6336_DTNAME "mediatek,flashlights_mt6336"
#endif

#define MT6336_NAME "flashlights-mt6336"

/* define ct, level */
#define MT6336_CT_NUM 2
#define MT6336_CT_HT 0
#define MT6336_CT_LT 1

#define MT6336_NONE (-1)
#define MT6336_DISABLE 0
#define MT6336_ENABLE 1
#define MT6336_ENABLE_TORCH 1
#define MT6336_ENABLE_FLASH 2

#define MT6336_LEVEL_NUM 34
#define MT6336_LEVEL_TORCH 8
#define MT6336_WDT_TIMEOUT 600 /* ms */

/* define mutex, work queue and timer */
static DEFINE_MUTEX(mt6336_mutex);
static DEFINE_MUTEX(mt6336_enable_mutex);
static DEFINE_MUTEX(mt6336_disable_mutex);
static struct work_struct mt6336_work_ht;
static struct work_struct mt6336_work_lt;
static struct hrtimer fl_timer_ht;
static struct hrtimer fl_timer_lt;
static unsigned int mt6336_timeout_ms[MT6336_CT_NUM];

/* define usage count */
static int use_count;

/* mt6336 pmic control handler */
struct mt6336_ctrl *flashlight_ctrl;

/******************************************************************************
 * mt6336 operations
 *****************************************************************************/
static const unsigned char mt6336_level[MT6336_LEVEL_NUM] = {
	0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 0x13, 0x17,
	0x1B, 0x1F, 0x23, 0x27, 0x2B, 0x2F, 0x33, 0x37, 0x3B, 0x3F,
	0x43, 0x47, 0x4B, 0x4F, 0x53, 0x57, 0x5B, 0x5F, 0x63, 0x67,
	0x6B, 0x6F, 0x73, 0x77
};

static int is_preenable;
static int mt6336_en_ht;
static int mt6336_en_lt;
static int mt6336_level_ht;
static int mt6336_level_lt;

static int mt6336_is_torch(int level)
{
	if (level >= MT6336_LEVEL_TORCH)
		return -1;

	return 0;
}

#if 0
static int mt6336_is_torch_by_timeout(int timeout)
{
	if (!timeout)
		return 0;

	if (timeout >= MT6336_WDT_TIMEOUT)
		return 0;

	return -1;
}
#endif

static int mt6336_verify_level(int level)
{
	if (level < 0)
		level = 0;
	else if (level >= MT6336_LEVEL_NUM)
		level = MT6336_LEVEL_NUM - 1;

	return level;
}

/* pre-enable steps before enable flashlight */
static int mt6336_preenable(void)
{
	int ret;

	ret = mt6336_set_register_value(0x0519, 0x0D);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_EN, 0x06);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_CLAMP_EN, 0x01);
	 */

	ret = mt6336_set_register_value(0x0520, 0x00);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_TUNE_SYS_MSB, 0x00);
	 */

	ret = mt6336_set_register_value(0x055A, 0x00);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_SWCHR_RSV_TRIM_MSB, 0x01);
	 */

	ret = mt6336_set_register_value(0x0455, 0x00);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_EN_HW_GAIN_SET, 0x00);
	 */

	ret = mt6336_set_register_value(0x03C9, 0x00);
	/* mt6336_set_flag_register_value(
	 * MT6336_AUXADC_HWGAIN_EN, 0x00);
	 */

	ret = mt6336_set_register_value(0x0553, 0x54);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_DTC, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_SRCEH, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_SRC, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_VTHSEL, 0x00);
	 */

	ret = mt6336_set_register_value(0x055F, 0xE0);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_FTR_RC, 0x03);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_FTR_DROP, 0x04);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_FTR_SHOOT_EN, 0x00);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_FTR_DROP_EN, 0x00);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_SWCHR_ZCD_TRIM_EN, 0x00);
	 */

	ret = mt6336_set_register_value(0x053D, 0x45);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_VRAMP_SLP, 0x04);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_VRAMP_DCOS, 0x05);
	 */

	/* TODO: only way to verify register is to get again */
	ret = 0;

	return ret;
}

/* post-enable steps after enable flashlight */
static int mt6336_postenable(void)
{
	int ret;

	ret = mt6336_set_register_value(0x052A, 0x88);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_RSV_LSB, 0x88);
	 */

	ret = mt6336_set_register_value(0x0553, 0x14);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_DTC, 0x00);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_SRCEH, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_SRC, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_PWR_LG_VTHSEL, 0x00);
	 */

	ret = mt6336_set_register_value(0x0519, 0x3F);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_EN, 0x1F);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_CLAMP_EN, 0x01);
	 */

	ret = mt6336_set_register_value(0x051E, 0x02);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_TUNE_ICHIN_MSB, 0x02);
	 */

	ret = mt6336_set_register_value(0x0520, 0x04);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_TUNE_SYS_MSB, 0x04);
	 */

	ret = mt6336_set_register_value(0x055A, 0x00);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_SWCHR_RSV_TRIM_MSB, 0x00);
	 */

	ret = mt6336_set_register_value(0x0455, 0x01);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_EN_HW_GAIN_SET, 0x01);
	 */

	ret = mt6336_set_register_value(0x03C9, 0x10);
	/* mt6336_set_flag_register_value(
	 * MT6336_AUXADC_HWGAIN_EN, 0x01);
	 */

	ret = mt6336_set_register_value(0x03CF, 0x03);
	/* mt6336_set_flag_register_value(
	 * MT6336_AUXADC_HWGAIN_DET_PRD_M, 0x03);
	 */

	ret = mt6336_set_register_value(0x0402, 0x03);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_DIS_REVFET, 0x00);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_T_TERM_EXT, 0x00);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_EN_TERM, 0x01);
	 * mt6336_set_flag_register_value(
	 * MT6336_RG_EN_RECHARGE, 0x01);
	 */

	ret = mt6336_set_register_value(0x0529, 0x88);
	/* mt6336_set_flag_register_value(
	 * MT6336_RG_A_LOOP_GM_RSV_MSB, 0x88);
	 */

	/* TODO: only way to verify register is to get again */
	ret = 0;

	return ret;
}

/* flashlight enable function */
static int mt6336_enable(void)
{
	mt6336_ctrl_enable(flashlight_ctrl);

	if ((mt6336_en_ht == MT6336_ENABLE_FLASH)
			|| (mt6336_en_lt == MT6336_ENABLE_FLASH)) {
		/* flash mode */
		if (!mt6336_get_flag_register_value(
					MT6336_DA_QI_OTG_MODE_MUX)) {

			/* disable charging */
			mt6336_set_register_value(0x0400, 0x00);
			/* mt6336_set_flag_register_value(
			 * MT6336_RG_EN_BUCK, 0x00);
			 * mt6336_set_flag_register_value(
			 * MT6336_RG_EN_CHARGE, 0x00);
			 */

			/* pre-enable steps */
			mt6336_preenable();
			is_preenable = 1;

			/* enable flash mode, source current */
			if (mt6336_en_ht)
				mt6336_set_flag_register_value(
						MT6336_RG_EN_IFLA1, 0x01);
			if (mt6336_en_lt)
				mt6336_set_flag_register_value(
						MT6336_RG_EN_IFLA2, 0x01);
			mt6336_set_flag_register_value(
					MT6336_RG_EN_LEDCS, 0x01);

			/* Enable flash and enable charger here for robust.
			 *
			 * When flash mode and charging mode are both enable,
			 * flash will get first priority in chip.
			 */
			mt6336_set_register_value(0x0400, 0x13);

		} else {
			/* TODO: action in OTG mode */
			fl_info("Failed to turn on flash mode since in OTG mode.\n");
		}

	} else {
		/* torch mode */

		/* pre-enable steps */
		if (!mt6336_get_flag_register_value(
					MT6336_DA_QI_OTG_MODE_MUX) &&
				!mt6336_get_flag_register_value(
					MT6336_DA_QI_VBUS_PLUGIN_MUX)) {
			mt6336_preenable();
			is_preenable = 1;
		}

		/* enable torch mode, source current and enable torch */
		if (mt6336_en_ht)
			mt6336_set_flag_register_value(
					MT6336_RG_EN_ITOR1, 0x01);
		if (mt6336_en_lt)
			mt6336_set_flag_register_value(
					MT6336_RG_EN_ITOR2, 0x01);
		mt6336_set_flag_register_value(MT6336_RG_EN_LEDCS, 0x01);
		mt6336_set_flag_register_value(MT6336_RG_EN_TORCH, 0x01);
	}

	mt6336_ctrl_disable(flashlight_ctrl);

	mt6336_en_ht = MT6336_NONE;
	mt6336_en_lt = MT6336_NONE;

	return 0;
}

/* flashlight disable function */
static int mt6336_disable(void)
{
	mt6336_ctrl_enable(flashlight_ctrl);

	/* disable torch/flash mode */
	mt6336_set_flag_register_value(MT6336_RG_EN_ITOR1, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_ITOR2, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_IFLA1, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_IFLA2, 0x00);

	/* pre-enable steps */
	if (is_preenable) {
		mt6336_postenable();
		is_preenable = 0;
	}

	/* disable source current and disable torch/flash */
	mt6336_set_flag_register_value(MT6336_RG_EN_LEDCS, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_TORCH, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_FLASH, 0x00);

	mt6336_ctrl_disable(flashlight_ctrl);

	mt6336_en_ht = MT6336_NONE;
	mt6336_en_lt = MT6336_NONE;

	return 0;
}

/* set flashlight level */
static int mt6336_set_level_ht(int level)
{
	level = mt6336_verify_level(level);
	mt6336_level_ht = level;

	/* set brightness level */
	mt6336_ctrl_enable(flashlight_ctrl);
	mt6336_set_flag_register_value(MT6336_RG_ITOR1, mt6336_level[level]);
	mt6336_set_flag_register_value(MT6336_RG_IFLA1, mt6336_level[level]);
	mt6336_ctrl_disable(flashlight_ctrl);

	return 0;
}

int mt6336_set_level_lt(int level)
{
	level = mt6336_verify_level(level);
	mt6336_level_lt = level;

	/* set brightness level */
	mt6336_ctrl_enable(flashlight_ctrl);
	mt6336_set_flag_register_value(MT6336_RG_ITOR2, mt6336_level[level]);
	mt6336_set_flag_register_value(MT6336_RG_IFLA2, mt6336_level[level]);
	mt6336_ctrl_disable(flashlight_ctrl);

	return 0;
}

static int mt6336_set_level(int ct_index, int level)
{
	if (ct_index == MT6336_CT_HT)
		mt6336_set_level_ht(level);
	else if (ct_index == MT6336_CT_LT)
		mt6336_set_level_lt(level);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

/* flashlight init */
int mt6336_init(void)
{
	int ret = 0;

	mt6336_ctrl_enable(flashlight_ctrl);

	/* clear flash/torch mode enable register */
	mt6336_set_flag_register_value(MT6336_RG_EN_IFLA1, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_IFLA2, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_ITOR1, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_ITOR2, 0x00);

	/* clear current source register */
	mt6336_set_flag_register_value(MT6336_RG_EN_LEDCS, 0x00);

	/* clear flash/torch enable register */
	mt6336_set_flag_register_value(MT6336_RG_EN_FLASH, 0x00);
	mt6336_set_flag_register_value(MT6336_RG_EN_TORCH, 0x00);

	/* setup flash mode output regulation voltage (default: 5.0 V) */
	mt6336_set_flag_register_value(MT6336_RG_VFLA, 0x05);

	/* setup flash mode current step up/down timing (default: 10 us) */
	mt6336_set_flag_register_value(MT6336_RG_TSTEP_ILED, 0x01);

	/* setup flash/torch watchdog timeout (default: 600 ms) */
	mt6336_set_flag_register_value(MT6336_RG_FLA_WDT, 0x10);

	/* enable flash watchdog (default) */
	mt6336_set_flag_register_value(MT6336_RG_EN_FLA_WDT, 0x01);

	/* disable torch watchdog (default) */
	mt6336_set_flag_register_value(MT6336_RG_EN_TOR_WDT, 0x00);

	mt6336_ctrl_disable(flashlight_ctrl);

	is_preenable = 0;
	mt6336_en_ht = MT6336_NONE;
	mt6336_en_lt = MT6336_NONE;

	return ret;
}

/* flashlight uninit */
int mt6336_uninit(void)
{
	mt6336_disable();

	return 0;
}


/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
void mt6336_isr_short_ht(void)
{
	schedule_work(&mt6336_work_ht);
}

void mt6336_isr_short_lt(void)
{
	schedule_work(&mt6336_work_lt);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static void mt6336_work_disable_ht(struct work_struct *data)
{
	fl_dbg("ht work queue callback\n");
	mt6336_disable();
}

static void mt6336_work_disable_lt(struct work_struct *data)
{
	fl_dbg("lt work queue callback\n");
	mt6336_disable();
}

static enum hrtimer_restart fl_timer_func_ht(struct hrtimer *timer)
{
	schedule_work(&mt6336_work_ht);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart fl_timer_func_lt(struct hrtimer *timer)
{
	schedule_work(&mt6336_work_lt);
	return HRTIMER_NORESTART;
}

int mt6336_timer_start(int ct_index, ktime_t ktime)
{
	if (ct_index == MT6336_CT_HT)
		hrtimer_start(&fl_timer_ht, ktime, HRTIMER_MODE_REL);
	else if (ct_index == MT6336_CT_LT)
		hrtimer_start(&fl_timer_lt, ktime, HRTIMER_MODE_REL);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

int mt6336_timer_cancel(int ct_index)
{
	if (ct_index == MT6336_CT_HT)
		hrtimer_cancel(&fl_timer_ht);
	else if (ct_index == MT6336_CT_LT)
		hrtimer_cancel(&fl_timer_lt);
	else {
		fl_err("Error ct index\n");
		return -1;
	}

	return 0;
}

/******************************************************************************
 * Flashlight operation wrapper function
 *****************************************************************************/
static int mt6336_operate(int ct_index, int enable)
{
	ktime_t ktime;

	/* setup enable/disable */
	if (ct_index == MT6336_CT_HT) {
		mt6336_en_ht = enable;
		if (mt6336_en_ht) {
			if (mt6336_is_torch(mt6336_level_ht))
				mt6336_en_ht = MT6336_ENABLE_FLASH;
		}
	} else if (ct_index == MT6336_CT_LT) {
		mt6336_en_lt = enable;
		if (mt6336_en_lt) {
			if (mt6336_is_torch(mt6336_level_lt))
				mt6336_en_lt = MT6336_ENABLE_FLASH;
		}
	} else {
		fl_err("Error ct index\n");
		return -1;
	}

	/* TODO: add clear state mechanism by timeout */

	/* operate flashlight and setup timer */
	if ((mt6336_en_ht != MT6336_NONE) &&
			(mt6336_en_lt != MT6336_NONE)) {
		if ((mt6336_en_ht == MT6336_DISABLE) &&
				(mt6336_en_lt == MT6336_DISABLE)) {
			mt6336_disable();
			mt6336_timer_cancel(MT6336_CT_HT);
			mt6336_timer_cancel(MT6336_CT_LT);
		} else {
			if (mt6336_timeout_ms[MT6336_CT_HT]) {
				ktime = ktime_set(
					mt6336_timeout_ms[MT6336_CT_HT] / 1000,
					(mt6336_timeout_ms[MT6336_CT_HT] % 1000) * 1000000);
				mt6336_timer_start(MT6336_CT_HT, ktime);
			}
			if (mt6336_timeout_ms[MT6336_CT_LT]) {
				ktime = ktime_set(
					mt6336_timeout_ms[MT6336_CT_LT] / 1000,
					(mt6336_timeout_ms[MT6336_CT_LT] % 1000) * 1000000);
				mt6336_timer_start(MT6336_CT_LT, ktime);
			}
			mt6336_enable();
		}
	}

	return 0;
}

/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int mt6336_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_user_arg *fl_arg;
	int ct_index;

	fl_arg = (struct flashlight_user_arg *)arg;
	ct_index = fl_get_ct_index(fl_arg->ct_id);
	if (flashlight_ct_index_verify(ct_index)) {
		fl_err("Failed with error index\n");
		return -EINVAL;
	}

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		fl_dbg("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		mt6336_timeout_ms[ct_index] = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		fl_dbg("FLASH_IOC_SET_DUTY(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		mt6336_set_level(ct_index, fl_arg->arg);
		break;

	case FLASH_IOC_SET_STEP:
		fl_dbg("FLASH_IOC_SET_STEP(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		fl_dbg("FLASH_IOC_SET_ONOFF(%d): %d\n",
				ct_index, (int)fl_arg->arg);
		mt6336_operate(ct_index, fl_arg->arg);
		break;

	default:
		fl_info("No such command and arg(%d): (%d, %d)\n",
				ct_index, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int mt6336_open(void *pArg)
{
	/* Actual behavior move to set driver since power saving issue */
	return 0;
}

static int mt6336_release(void *pArg)
{
	/* uninit chip and clear usage count */
	mutex_lock(&mt6336_mutex);
	use_count--;
	if (!use_count)
		mt6336_uninit();
	if (use_count < 0)
		use_count = 0;
	mutex_unlock(&mt6336_mutex);

	fl_dbg("Release: %d\n", use_count);

	return 0;
}

static int mt6336_set_driver(int scenario)
{
	/* init chip and set usage count */
	mutex_lock(&mt6336_mutex);
	if (!use_count)
		mt6336_init();
	use_count++;
	mutex_unlock(&mt6336_mutex);

	fl_dbg("Set driver: %d\n", use_count);

	return 0;
}

static ssize_t mt6336_strobe_store(struct flashlight_arg arg)
{
	mt6336_set_driver(FLASHLIGHT_SCENARIO_CAMERA);
	mt6336_set_level(arg.ct, arg.level);

	if (arg.level < 0)
		mt6336_operate(arg.ct, MT6336_DISABLE);
	else
		mt6336_operate(arg.ct, MT6336_ENABLE);

	msleep(arg.dur);
	mt6336_operate(arg.ct, MT6336_DISABLE);
	mt6336_release(NULL);

	return 0;
}

static struct flashlight_operations mt6336_ops = {
	mt6336_open,
	mt6336_release,
	mt6336_ioctl,
	mt6336_strobe_store,
	mt6336_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int mt6336_probe(struct platform_device *dev)
{
	fl_dbg("Probe start.\n");

	/* init work queue */
	INIT_WORK(&mt6336_work_ht, mt6336_work_disable_ht);
	INIT_WORK(&mt6336_work_lt, mt6336_work_disable_lt);

	/* init timer */
	hrtimer_init(&fl_timer_ht, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fl_timer_ht.function = fl_timer_func_ht;
	hrtimer_init(&fl_timer_lt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fl_timer_lt.function = fl_timer_func_lt;
	mt6336_timeout_ms[MT6336_CT_HT] = 100;
	mt6336_timeout_ms[MT6336_CT_LT] = 100;

	/* register flashlight operations */
	if (flashlight_dev_register(MT6336_NAME, &mt6336_ops))
		return -EFAULT;

	/* get mt6336 pmic control handler */
	flashlight_ctrl = mt6336_ctrl_get("mt6336_flashlight");

	/* register and enable mt6336 pmic ISR */
	mt6336_ctrl_enable(flashlight_ctrl);
	mt6336_register_interrupt_callback(MT6336_INT_LED1_SHORT, mt6336_isr_short_ht);
	mt6336_register_interrupt_callback(MT6336_INT_LED2_SHORT, mt6336_isr_short_lt);
	/* mt6336_register_interrupt_callback(MT6336_INT_LED1_OPEN, NULL); */
	/* mt6336_register_interrupt_callback(MT6336_INT_LED2_OPEN, NULL); */
	mt6336_enable_interrupt(MT6336_INT_LED1_SHORT, "flashlight");
	mt6336_enable_interrupt(MT6336_INT_LED2_SHORT, "flashlight");
	/* mt6336_enable_interrupt(MT6336_INT_LED1_OPEN, "flashlight"); */
	/* mt6336_enable_interrupt(MT6336_INT_LED2_OPEN, "flashlight"); */
	mt6336_ctrl_disable(flashlight_ctrl);

	/* clear usage count */
	use_count = 0;

	fl_dbg("Probe done.\n");

	return 0;
}

static int mt6336_remove(struct platform_device *dev)
{
	fl_dbg("Remove start.\n");

	/* flush work queue */
	flush_work(&mt6336_work_ht);
	flush_work(&mt6336_work_lt);

	/* unregister flashlight operations */
	flashlight_dev_unregister(MT6336_NAME);

	/* disable mt6336 pmic ISR */
	mt6336_ctrl_enable(flashlight_ctrl);
	mt6336_disable_interrupt(MT6336_INT_LED1_SHORT, "flashlight");
	mt6336_disable_interrupt(MT6336_INT_LED2_SHORT, "flashlight");
	/* mt6336_disable_interrupt(MT6336_INT_LED1_OPEN, "flashlight"); */
	/* mt6336_disable_interrupt(MT6336_INT_LED2_OPEN, "flashlight"); */
	mt6336_ctrl_disable(flashlight_ctrl);

	fl_dbg("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt6336_of_match[] = {
	{.compatible = MT6336_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, mt6336_of_match);
#else
static struct platform_device mt6336_platform_device[] = {
	{
		.name = MT6336_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, mt6336_platform_device);
#endif

static struct platform_driver mt6336_platform_driver = {
	.probe = mt6336_probe,
	.remove = mt6336_remove,
	.driver = {
		.name = MT6336_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mt6336_of_match,
#endif
	},
};

static int __init flashlight_mt6336_init(void)
{
	int ret;

	fl_dbg("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&mt6336_platform_device);
	if (ret) {
		fl_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&mt6336_platform_driver);
	if (ret) {
		fl_err("Failed to register platform driver\n");
		return ret;
	}

	fl_dbg("Init done.\n");

	return 0;
}

static void __exit flashlight_mt6336_exit(void)
{
	fl_dbg("Exit start.\n");

	platform_driver_unregister(&mt6336_platform_driver);

	fl_dbg("Exit done.\n");
}

/* replace module_init() since conflict in kernel init process */
late_initcall(flashlight_mt6336_init);
module_exit(flashlight_mt6336_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight MT6336 Driver");


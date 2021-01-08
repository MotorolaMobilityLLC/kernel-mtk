/*
 * aw881xx_monitor.c monitor_module
 *
 * Version: v0.1.14
 *
 * Copyright (c) 2019 AWINIC Technology CO., LTD
 *
 *  Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>
#include <linux/debugfs.h>
#include <asm/ioctls.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include "aw881xx.h"
#include "aw881xx_reg.h"
#include "aw881xx_monitor.h"


static int aw881xx_monitor_set_gain_value(struct aw881xx *aw881xx,
					uint16_t gain_value)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t read_reg_val;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_HAGCCFG7, &reg_val);
	if (ret)
		return ret;

	/* check gain */
	read_reg_val = reg_val;
	read_reg_val = read_reg_val >> AW881XX_HAGC_GAIN_SHIFT;
	if (read_reg_val == gain_value) {
		aw_dev_dbg(aw881xx->dev, "%s: gain_value=0x%x, no change\n",
			   __func__, gain_value);
		return ret;
	}

	reg_val &= AW881XX_HAGC_GAIN_MASK;
	reg_val |= gain_value << AW881XX_HAGC_GAIN_SHIFT;

	ret = aw881xx_reg_write(aw881xx, AW881XX_REG_HAGCCFG7, reg_val);
	if (ret < 0)
		return ret;

	aw_dev_dbg(aw881xx->dev, "%s: set reg_val=0x%x, gain_value=0x%x\n",
		__func__, reg_val, gain_value);

	return ret;
}

static int aw881xx_monitor_set_bst_ipeak(struct aw881xx *aw881xx,
					uint16_t bst_ipeak)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t read_reg_val;

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_SYSCTRL2, &reg_val);
	if (ret < 0)
		return ret;

	/* check ipeak */
	read_reg_val = reg_val;
	read_reg_val = read_reg_val & (~AW881XX_BIT_SYSCTRL2_BST_IPEAK_MASK);
	if (read_reg_val == bst_ipeak) {
		aw_dev_dbg(aw881xx->dev, "%s: ipeak=0x%x, no change\n",
			__func__, bst_ipeak);
		return ret;
	}

	reg_val &= AW881XX_BIT_SYSCTRL2_BST_IPEAK_MASK;
	reg_val |= bst_ipeak;

	ret = aw881xx_reg_write(aw881xx, AW881XX_REG_SYSCTRL2, reg_val);
	if (ret < 0)
		return ret;

	aw_dev_dbg(aw881xx->dev, "%s: set reg_val=0x%x, bst_ipeak=0x%x\n",
		__func__, reg_val, bst_ipeak);

	return ret;
}

static int aw881xx_monitor_vmax_check(struct aw881xx *aw881xx)
{
	int ret = -1;

	ret = aw881xx_get_iis_status(aw881xx);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: no iis signal\n", __func__);
		return ret;
	}

	ret = aw881xx_get_dsp_status(aw881xx);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: dsp not work\n", __func__);
		return ret;
	}

	return 0;
}

static int aw881xx_monitor_set_vmax(struct aw881xx *aw881xx, uint16_t vmax)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw881xx_monitor_vmax_check(aw881xx);
	if (ret) {
		aw_dev_err(aw881xx->dev, "%s: vamx_check fail, ret=%d\n",
			__func__, ret);
		return ret;
	}

	ret = aw881xx_dsp_read(aw881xx, AW881XX_DSP_REG_VMAX, &reg_val);
	if (ret)
		return ret;

	if (reg_val == vmax) {
		aw_dev_info(aw881xx->dev, "%s: read vmax=0x%x\n",
			__func__, reg_val);
		return ret;
	}

	ret = aw881xx_dsp_write(aw881xx, AW881XX_DSP_REG_VMAX, vmax);
	if (ret)
		return ret;

	return ret;
}

static int aw881xx_monitor_get_voltage(struct aw881xx *aw881xx,
					uint16_t *voltage)
{
	int ret = -1;

	if (voltage == NULL) {
		aw_dev_err(aw881xx->dev, "%s: voltage=%p\n",
			__func__, voltage);
		return ret;
	}

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_VBAT, voltage);
	if (ret < 0)
		return ret;

	*voltage = (*voltage) * AW881XX_VBAT_RANGE /
		AW881XX_VBAT_COEFF_INT_10BIT;

	aw_dev_dbg(aw881xx->dev, "%s: chip voltage=%dmV\n", __func__, *voltage);

	return ret;
}

static int aw881xx_monitor_get_temperature(struct aw881xx *aw881xx,
					int16_t *temp)
{
	int ret = -1;
	uint16_t reg_val = 0;

	if (temp == NULL) {
		aw_dev_err(aw881xx->dev, "%s:temp=%p\n",
			__func__, temp);
		return ret;
	}

	ret = aw881xx_reg_read(aw881xx, AW881XX_REG_TEMP, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val & AW881XX_TEMP_SIGN_MASK)
		reg_val |= AW881XX_TEMP_NEG_MASK;

	*temp = (int)reg_val;

	aw_dev_dbg(aw881xx->dev, "%s: chip_temperature=%d\n", __func__, *temp);

	return ret;
}

static int aw881xx_monitor_check_voltage(struct aw881xx *aw881xx,
					uint16_t *bst_ipeak,
					uint16_t *gain_db)
{
	int ret = -1;
	uint16_t voltage = 0;
	struct aw881xx_monitor *monitor = &aw881xx->monitor;

	if ((bst_ipeak == NULL) || (gain_db == NULL)) {
		aw_dev_err(aw881xx->dev, "bst_ipeak is %p, gain_db is %p\n",
			bst_ipeak, gain_db);
		return ret;
	}

	ret = aw881xx_monitor_get_voltage(aw881xx, &voltage);
	if (ret < 0)
		return ret;

	if (voltage > AW881XX_VOL_LIMIT_40) {
		*bst_ipeak = AW881XX_IPEAK_350;
		*gain_db = AW881XX_GAIN_00DB;
		monitor->pre_vol_bst_ipeak = *bst_ipeak;
		monitor->pre_vol_gain_db = *gain_db;
	} else if (voltage < AW881XX_VOL_LIMIT_39 &&
		   voltage > AW881XX_VOL_LIMIT_38) {
		*bst_ipeak = AW881XX_IPEAK_300;
		*gain_db = AW881XX_GAIN_NEG_05DB;
		monitor->pre_vol_bst_ipeak = *bst_ipeak;
		monitor->pre_vol_gain_db = *gain_db;
	} else if (voltage < AW881XX_VOL_LIMIT_37 &&
		   voltage > AW881XX_VOL_LIMIT_36) {
		*bst_ipeak = AW881XX_IPEAK_275;
		*gain_db = AW881XX_GAIN_NEG_10DB;
		monitor->pre_vol_bst_ipeak = *bst_ipeak;
		monitor->pre_vol_gain_db = *gain_db;
	} else if (voltage < AW881XX_VOL_LIMIT_35) {
		*bst_ipeak = AW881XX_IPEAK_250;
		*gain_db = AW881XX_GAIN_NEG_15DB;
		monitor->pre_vol_bst_ipeak = *bst_ipeak;
		monitor->pre_vol_gain_db = *gain_db;
	} else {
		*bst_ipeak = monitor->pre_vol_bst_ipeak;
		*gain_db = monitor->pre_vol_gain_db;
	}
	aw_dev_info(aw881xx->dev, "%s: bst_ipeak=0x%x, gain_db=0x%x\n",
			__func__, *bst_ipeak, *gain_db);

	return ret;
}

static void aw881xx_monitor_check_temperature_deglitch(uint16_t *bst_ipeak,
				uint16_t *gain_db, uint16_t *vmax,
				int16_t temperature, int16_t pre_temp)
{
	if (temperature <= pre_temp) {
		if (temperature <= AW881XX_TEMP_LIMIT_7 &&
			temperature >= AW881XX_TEMP_LIMIT_5) {
			*bst_ipeak = AW881XX_IPEAK_350;
			*gain_db = AW881XX_GAIN_00DB;
			*vmax = AW881XX_VMAX_80;
		} else if (temperature <= AW881XX_TEMP_LIMIT_2 &&
			temperature >= AW881XX_TEMP_LIMIT_0) {
			*bst_ipeak = AW881XX_IPEAK_300;
			*gain_db = AW881XX_GAIN_NEG_30DB;
			*vmax = AW881XX_VMAX_70;
		} else if (temperature <= AW881XX_TEMP_LIMIT_NEG_2 &&
			   temperature >= AW881XX_TEMP_LIMIT_NEG_5) {
			*bst_ipeak = AW881XX_IPEAK_275;
			*gain_db = AW881XX_GAIN_NEG_45DB;
			*vmax = AW881XX_VMAX_60;
		}
	} else {
		if (temperature <= AW881XX_TEMP_LIMIT_7 &&
			temperature >= AW881XX_TEMP_LIMIT_5) {
			*bst_ipeak = AW881XX_IPEAK_300;
			*gain_db = AW881XX_GAIN_NEG_30DB;
			*vmax = AW881XX_VMAX_70;
		} else if (temperature <= AW881XX_TEMP_LIMIT_2 &&
			temperature >= AW881XX_TEMP_LIMIT_0) {
			*bst_ipeak = AW881XX_IPEAK_275;
			*gain_db = AW881XX_GAIN_NEG_45DB;
			*vmax = AW881XX_VMAX_60;
		} else if (temperature <= AW881XX_TEMP_LIMIT_NEG_2 &&
			temperature >= AW881XX_TEMP_LIMIT_NEG_5) {
			*bst_ipeak = AW881XX_IPEAK_250;
			*gain_db = AW881XX_GAIN_NEG_60DB;
			*vmax = AW881XX_VMAX_50;
		}
	}
}

static int aw881xx_monitor_check_temperature(struct aw881xx *aw881xx,
			uint16_t *bst_ipeak, uint16_t *gain_db, uint16_t *vmax)
{
	int ret = -1;
	int16_t temperature = 0;
	int16_t pre_temp;
	struct aw881xx_monitor *monitor = &aw881xx->monitor;

	if ((bst_ipeak == NULL) || (gain_db == NULL) || (vmax == NULL)) {
		aw_dev_err(aw881xx->dev,
			"%s: bst_ipeak=%p, gain_db=%p, vmax=%p\n",
			__func__, bst_ipeak, gain_db, vmax);
		return ret;
	}
	pre_temp = aw881xx->monitor.pre_temp;

	ret = aw881xx_monitor_get_temperature(aw881xx, &temperature);
	if (ret < 0)
		return ret;

	aw881xx->monitor.pre_temp = temperature;
	if (temperature > AW881XX_TEMP_LIMIT_7) {
		*bst_ipeak = AW881XX_IPEAK_350;
		*gain_db = AW881XX_GAIN_00DB;
		*vmax = AW881XX_VMAX_80;
	} else if (temperature < AW881XX_TEMP_LIMIT_5 &&
		temperature > AW881XX_TEMP_LIMIT_2) {
		*bst_ipeak = AW881XX_IPEAK_300;
		*gain_db = AW881XX_GAIN_NEG_30DB;
		*vmax = AW881XX_VMAX_70;
	} else if (temperature < AW881XX_TEMP_LIMIT_0 &&
		temperature > AW881XX_TEMP_LIMIT_NEG_2) {
		*bst_ipeak = AW881XX_IPEAK_275;
		*gain_db = AW881XX_GAIN_NEG_45DB;
		*vmax = AW881XX_VMAX_60;
	} else if (temperature < AW881XX_TEMP_LIMIT_NEG_5) {
		*bst_ipeak = AW881XX_IPEAK_250;
		*gain_db = AW881XX_GAIN_NEG_60DB;
		*vmax = AW881XX_VMAX_50;
	} else {
		if (temperature == pre_temp) {
			*bst_ipeak = monitor->pre_temp_bst_ipeak;
			*gain_db = monitor->pre_temp_gain_db;
			*vmax = monitor->pre_temp_vmax;
		} else {
			aw881xx_monitor_check_temperature_deglitch(bst_ipeak,
					gain_db, vmax, temperature, pre_temp);
		}
	}
	monitor->pre_temp_bst_ipeak = *bst_ipeak;
	monitor->pre_temp_gain_db = *gain_db;
	monitor->pre_temp_vmax = *vmax;

	aw_dev_info(aw881xx->dev,
		"%s: bst_ipeak=0x%x, gain_db=0x%x, vmax=0x%0x\n",
		__func__, *bst_ipeak, *gain_db, *vmax);

	return ret;
}

static int aw881xx_monitor_check_sysint(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t sysint = 0;

	ret = aw881xx_get_sysint(aw881xx, &sysint);
	if (ret < 0)
		aw_dev_err(aw881xx->dev, "%s: get_sysint fail, ret=%d\n",
			__func__, ret);
	else
		aw_dev_info(aw881xx->dev, "%s: get_sysint=0x%04x\n",
			__func__, sysint);

	return ret;
}

static void aw881xx_monitor_work(struct aw881xx *aw881xx)
{
	int ret = -1;
	uint16_t vol_ipeak = 0;
	uint16_t vol_gain = 0;
	uint16_t temp_ipeak = 0;
	uint16_t temp_gain = 0;
	uint16_t vmax = 0;
	uint16_t real_ipeak;
	uint16_t real_gain;

	/* get ipeak and gain value by voltage and temperature */
	ret = aw881xx_monitor_check_voltage(aw881xx, &vol_ipeak, &vol_gain);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: check voltage failed\n",
			__func__);
		return;
	}
	ret = aw881xx_monitor_check_temperature(aw881xx,
					&temp_ipeak, &temp_gain, &vmax);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev, "%s: check temperature failed\n",
			   __func__);
		return;
	}
	/* get min Ipeak */
	if (vol_ipeak == AW881XX_IPEAK_NONE && temp_ipeak == AW881XX_IPEAK_NONE)
		real_ipeak = AW881XX_IPEAK_NONE;
	else
		real_ipeak = (vol_ipeak < temp_ipeak ? vol_ipeak : temp_ipeak);

	/* get min gain */
	if (vol_gain == AW881XX_GAIN_NONE || temp_gain == AW881XX_GAIN_NONE) {
		if (vol_gain == AW881XX_GAIN_NONE &&
			temp_gain == AW881XX_GAIN_NONE)
			real_gain = AW881XX_GAIN_NONE;
		else if (vol_gain == AW881XX_GAIN_NONE)
			real_gain = temp_gain;
		else
			real_gain = vol_gain;
	} else {
		real_gain = (vol_gain > temp_gain ? vol_gain : temp_gain);
	}

	if (real_ipeak != AW881XX_IPEAK_NONE)
		aw881xx_monitor_set_bst_ipeak(aw881xx, real_ipeak);

	if (real_gain != AW881XX_GAIN_NONE)
		aw881xx_monitor_set_gain_value(aw881xx, real_gain);

	if (vmax != AW881XX_VMAX_NONE)
		aw881xx_monitor_set_vmax(aw881xx, vmax);

	aw881xx_monitor_check_sysint(aw881xx);
}

void aw881xx_monitor_stop(struct aw881xx_monitor *monitor)
{
	struct aw881xx *aw881xx =
		container_of(monitor, struct aw881xx, monitor);

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);

	if (hrtimer_active(&monitor->monitor_timer)) {
		aw_dev_info(aw881xx->dev, "%s: cancel monitor\n", __func__);
		hrtimer_cancel(&monitor->monitor_timer);
	}
}

void aw881xx_monitor_start(struct aw881xx_monitor *monitor)
{
	int timer_val = monitor->monitor_timer_val;
	struct aw881xx *aw881xx =
		container_of(monitor, struct aw881xx, monitor);

	aw_dev_info(aw881xx->dev, "%s enter\n", __func__);

	if (!hrtimer_active(&monitor->monitor_timer)) {
		aw_dev_info(aw881xx->dev, "%s: start monitor\n", __func__);
		hrtimer_start(&monitor->monitor_timer,
					ktime_set(timer_val / 1000,
					(timer_val % 1000) * 1000000),
					HRTIMER_MODE_REL);
	}
}

static enum hrtimer_restart aw881xx_monitor_timer_func(struct hrtimer *timer)
{
	struct aw881xx_monitor *monitor =
		container_of(timer, struct aw881xx_monitor, monitor_timer);
	struct aw881xx *aw881xx =
		container_of(monitor, struct aw881xx, monitor);

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	if (monitor->monitor_flag)
		schedule_work(&monitor->monitor_work);

	return HRTIMER_NORESTART;
}

static void aw881xx_monitor_work_routine(struct work_struct *work)
{
	struct aw881xx_monitor *monitor =
		container_of(work, struct aw881xx_monitor, monitor_work);
	struct aw881xx *aw881xx =
		container_of(monitor, struct aw881xx, monitor);

	aw_dev_dbg(aw881xx->dev, "%s: enter\n", __func__);

	mutex_lock(&aw881xx->lock);
	if (!aw881xx_get_hmute(aw881xx)) {
		aw881xx_monitor_work(aw881xx);
		aw881xx_monitor_start(monitor);
	}
	mutex_unlock(&aw881xx->lock);
}

static ssize_t aw881xx_monitor_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	int ret = -1;
	unsigned int databuf[2] = { 0 };

	ret = kstrtouint(buf, 0, &databuf[0]);
	if (ret < 0)
		return ret;

	mutex_lock(&aw881xx->lock);
	if (aw881xx->monitor.monitor_flag == databuf[0]) {
		mutex_unlock(&aw881xx->lock);
		return count;
	}
	aw881xx->monitor.monitor_flag = databuf[0];
	if (databuf[0])
		schedule_work(&aw881xx->monitor.monitor_work);
	mutex_unlock(&aw881xx->lock);

	return count;
}

static ssize_t aw881xx_monitor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"aw881xx monitor_flag=%d\n",
			aw881xx->monitor.monitor_flag);

	return len;
}

static DEVICE_ATTR(monitor, S_IWUSR | S_IRUGO,
		aw881xx_monitor_show, aw881xx_monitor_store);

static struct attribute *aw881xx_monitor_attr[] = {
	&dev_attr_monitor.attr,
	NULL
};

static struct attribute_group aw881xx_monitor_attr_group = {
	.attrs = aw881xx_monitor_attr
};

int aw881xx_monitor_init(struct aw881xx_monitor *monitor)
{
	int ret = -1;
	struct aw881xx *aw881xx =
		container_of(monitor, struct aw881xx, monitor);

	aw_dev_info(aw881xx->dev, "%s: enter\n", __func__);
	hrtimer_init(&monitor->monitor_timer, CLOCK_MONOTONIC,
			HRTIMER_MODE_REL);
	monitor->monitor_timer.function = aw881xx_monitor_timer_func;
	INIT_WORK(&monitor->monitor_work, aw881xx_monitor_work_routine);

	ret = sysfs_create_group(&aw881xx->dev->kobj,
				 &aw881xx_monitor_attr_group);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev,
			"%s error creating sysfs attr files\n", __func__);
		return ret;
	}

	return 0;
}

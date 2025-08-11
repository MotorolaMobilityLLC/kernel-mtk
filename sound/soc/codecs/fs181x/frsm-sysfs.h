/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-08-03 File created.
 */

#ifndef __FRSM_SYSFS_H__
#define __FRSM_SYSFS_H__

#include <linux/debugfs.h>
#include "internal.h"

static uint8_t g_reg_addr;

static ssize_t dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	uint16_t val;
	ssize_t len;
	uint8_t reg;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	for (len = 0, reg = 0; reg <= FRSM_REG_MAX; reg++) {
		ret = frsm_reg_read(frsm_dev, reg, &val);
		if (ret)
			return ret;
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%02X:%04X%c", reg, val,
				(reg & 0x7) == 0x7 ? '\n' : ' ');
	}
	buf[len - 1] = '\n';

	return len;
}

static ssize_t reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	uint16_t val;
	ssize_t len;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	ret = frsm_reg_read(frsm_dev, g_reg_addr, &val);
	if (ret)
		return ret;

	len = snprintf(buf, PAGE_SIZE, "%02x:%04x\n", g_reg_addr, val);

	return len;
}

static ssize_t reg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	unsigned long data;
	uint16_t *buf16;
	uint8_t addr;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	ret = kstrtoul(buf, 0, &data);
	if (ret)
		return ret;

	buf16 = (uint16_t *)&data;
	if (data <= 0xFF) {
		addr = buf16[0] & 0xFF;
	} else if (data <= 0xFFFFFF) {
		addr = buf16[1] & 0xFF;
		ret = frsm_reg_write(frsm_dev, addr, buf16[0]);
	} else if (data <= 0xFFFFFFFFFF) {
		addr = buf16[2] & 0xFF;
		ret = frsm_reg_update_bits(frsm_dev, addr, buf16[1], buf16[0]);
	} else {
		dev_err(frsm_dev->dev, "Invalid data:%ld\n", data);
		return -EINVAL;
	}

	g_reg_addr = addr;
	if (ret)
		return ret;

	return count;
}

static ssize_t tuning_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	ssize_t len;
	int state;

	if (frsm_dev == NULL)
		return -EINVAL;

	state = test_bit(STATE_TUNING, &frsm_dev->state);
	len = snprintf(buf, PAGE_SIZE, "%s\n", state ? "On" : "Off");

	return len;
}

static ssize_t tuning_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	int state;
	int ret;

	if (frsm_dev == NULL)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &state);
	if (ret)
		return ret;

	if (state)
		set_bit(STATE_TUNING, &frsm_dev->state);
	else
		clear_bit(STATE_TUNING, &frsm_dev->state);

	return count;
}

static ssize_t reload_fw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	const char *name;
	ssize_t len;
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	name = frsm_dev->pdata->fwm_name;
	frsm_dev->force_init = true;
	ret = frsm_firmware_init_sync(frsm_dev, name);
	len = snprintf(buf, PAGE_SIZE, "%s\n", !ret ? "OK" : "FAIL");

	return len;
}

static ssize_t mntr_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	ssize_t len;
	bool on;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	on = frsm_dev->pdata->mntr_enable;

	len = snprintf(buf, PAGE_SIZE, "%s\n", on ? "On" : "Off");

	return len;
}

static ssize_t mntr_en_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	int turn_on;
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	if (count <= 0)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &turn_on);
	if (ret)
		return ret;

	if (turn_on) {
		frsm_dev->pdata->mntr_enable = true;
		ret = frsm_mntr_switch(frsm_dev, 1);
	} else {
		ret = frsm_mntr_switch(frsm_dev, 0);
		frsm_dev->pdata->mntr_enable = false;
	}

	if (ret)
		return ret;

	return count;
}

static ssize_t mntr_period_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	ssize_t len;
	int period;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	period = frsm_dev->pdata->mntr_period;

	len = snprintf(buf, PAGE_SIZE, "%d(ms)\n", period);

	return len;
}

static ssize_t mntr_period_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct frsm_dev *frsm_dev = dev_get_drvdata(dev);
	int period;
	int ret;

	if (frsm_dev == NULL || frsm_dev->pdata == NULL)
		return -EINVAL;

	if (count <= 0)
		return -EINVAL;

	ret = kstrtoint(buf, 0, &period);
	if (ret)
		return ret;

	frsm_dev->pdata->mntr_period = period;

	return count;
}

static DEVICE_ATTR_RO(dump);
static DEVICE_ATTR_RW(reg);
static DEVICE_ATTR_RW(tuning);
static DEVICE_ATTR_RO(reload_fw);
static DEVICE_ATTR_RW(mntr_en);
static DEVICE_ATTR_RW(mntr_period);

static struct attribute *frsm_i2c_attributes[] = {
	&dev_attr_dump.attr,
	&dev_attr_reg.attr,
	&dev_attr_tuning.attr,
	&dev_attr_reload_fw.attr,
	&dev_attr_mntr_en.attr,
	&dev_attr_mntr_period.attr,
	NULL
};

static struct attribute_group frsm_i2c_attr_group = {
	.attrs = frsm_i2c_attributes,
};

static int frsm_sysfs_init(struct frsm_dev *frsm_dev)
{
	int ret;

	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return -EINVAL;

	ret = sysfs_create_group(&frsm_dev->dev->kobj, &frsm_i2c_attr_group);

	FRSM_FUNC_EXIT(frsm_dev->dev, ret);
	return ret;
}

static void frsm_sysfs_deinit(struct frsm_dev *frsm_dev)
{
	if (frsm_dev == NULL || frsm_dev->dev == NULL)
		return;

	sysfs_remove_group(&frsm_dev->dev->kobj, &frsm_i2c_attr_group);
}

#endif // __FRSM_SYSFS_H__

/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2024-02-04 File created.
 */

#ifndef __FRSM_AMP_CLASS_H__
#define __FRSM_AMP_CLASS_H__

#include <linux/debugfs.h>
#include "frsm-amp-drv.h"

static ssize_t ndev_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;
	int ndev;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	ndev = frsm_amp->spkinfo.ndev;

	return snprintf(buf, PAGE_SIZE, "%d\n", ndev);
}

static ssize_t ndev_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	int data;
	int ret;

	ret = kstrtoint(buf, 0, &data);
	if (ret)
		return ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	frsm_amp->spkinfo.ndev = data;

	return len;
}

static ssize_t func_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	return snprintf(buf, PAGE_SIZE, "%lx\n", frsm_amp->func);
}

static ssize_t func_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	unsigned long data;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	ret = kstrtoul(buf, 0, &data);
	if (ret)
		return ret;

	frsm_amp->func = data;

	return len;
}

static ssize_t regs_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp_reg *amp_reg;
	struct frsm_amp *frsm_amp;
	struct frsm_argv argv;
	int len, size;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	size = sizeof(struct frsm_amp_reg) + sizeof(int);
	amp_reg = kzalloc(size, GFP_KERNEL);
	if (amp_reg == NULL)
		return -ENOMEM;

	amp_reg->addr = frsm_amp->addr;
	amp_reg->size = sizeof(char);
	amp_reg->buf[0] = frsm_amp->reg;
	argv.buf = amp_reg;
	argv.size = sizeof(struct frsm_amp_reg) + amp_reg->size;
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_REG_WRITE, &argv);
	if (ret == amp_reg->size) {
		amp_reg->size = sizeof(uint16_t);
		argv.size = sizeof(struct frsm_amp_reg) + amp_reg->size;
		ret = frsm_amp_notify(frsm_amp, EVENT_AMP_REG_READ, &argv);
	}

	if (ret == amp_reg->size)
		len = snprintf(buf, PAGE_SIZE, "%02x %02x:%02x%02x\n",
				amp_reg->addr, frsm_amp->reg,
				amp_reg->buf[0], amp_reg->buf[1]);
	else
		len = -EFAULT;
	kfree(amp_reg);

	return len;
}

static ssize_t regs_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp_reg *amp_reg;
	struct frsm_amp *frsm_amp;
	struct frsm_argv argv;
	int data, size;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	ret = kstrtoint(buf, 0, &data);
	if (ret)
		return ret;

	if (data >= 0 && data <= 0xFFFF) { // addr8 reg8
		frsm_amp->addr = HIGH8(data);
		frsm_amp->reg = LOW8(data);
		return len;
	}

	size = sizeof(struct frsm_amp_reg) + sizeof(int);
	amp_reg = kzalloc(size, GFP_KERNEL);
	if (amp_reg == NULL)
		return -ENOMEM;

	amp_reg->addr = LOW8(data >> 24);
	amp_reg->buf[0] = LOW8(data >> 16);
	amp_reg->buf[1] = LOW8(data >> 8);
	amp_reg->buf[2] = LOW8(data);
	frsm_amp->addr = amp_reg->addr;
	frsm_amp->reg = amp_reg->buf[0];
	amp_reg->size = sizeof(char) + sizeof(uint16_t);
	argv.buf = amp_reg;
	argv.size = sizeof(struct frsm_amp_reg) + amp_reg->size;
	ret = frsm_amp_notify(frsm_amp, EVENT_AMP_REG_WRITE, &argv);
	if (ret != amp_reg->size)
		len = -EFAULT;
	kfree(amp_reg);

	return len;
}

static ssize_t monitor_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	return snprintf(buf, PAGE_SIZE, "%s\n",
			frsm_amp->mntr_en ? "Enabled" : "Disabled");
}

static ssize_t monitor_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	int state;
	int ret;

	ret = kstrtoint(buf, 0, &state);
	if (ret)
		return ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	frsm_amp->mntr_en = !!state;
	dev_info(frsm_amp->dev, "monitor:%d\n", frsm_amp->mntr_en);
	ret = frsm_amp_mntr_switch(frsm_amp, frsm_amp->mntr_en);
	if (ret)
		return ret;

	if (!frsm_amp->mntr_en && frsm_amp->stream_on)
		frsm_amp_prot_battery(frsm_amp, true);

	return len;
}

static ssize_t batt_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;
	int len;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	len = snprintf(buf, PAGE_SIZE, "batv:%d cap:%d tempr:%d\n",
			frsm_amp->batt.batv,
			frsm_amp->batt.cap,
			frsm_amp->batt.tempr);

	return len;
}

static ssize_t batt_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	int ret;

	if (len < sizeof(struct frsm_batt))
		return -EFAULT;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	memcpy(&frsm_amp->batt, buf, sizeof(struct frsm_batt));

	ret = frsm_amp_prot_battery(frsm_amp, false);
	if (ret)
		return ret;

	return len;
}

static ssize_t tunings_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	struct frsm_argv argv;
	char tuning;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	if (sizeof(tuning) != len)
		return -EFAULT;

	tuning = *buf;
	argv.buf = &tuning;
	argv.size = len;
	ret = frsm_amp_set_tuning(frsm_amp, &argv);
	if (ret)
		return ret;

	return len;
}

static ssize_t calre_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_amp *frsm_amp;
	struct spkr_info *info;
	struct frsm_argv argv;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);

	if (len > sizeof(struct spkr_info))
		return -EFAULT;

	info = kzalloc(sizeof(struct spkr_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	memcpy(info, buf, len);
	argv.buf = info;
	argv.size = sizeof(struct spkr_info);
	ret = frsm_amp_set_calre(frsm_amp, &argv);
	kfree(info);
	if (ret)
		return ret;

	return len;
}

static ssize_t livedata_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;
	struct live_data *data;
	struct frsm_argv argv;
	struct spkr_info info;
	int i, len;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	if (!test_bit(EVENT_STREAM_ON, &frsm_amp->state))
		return snprintf(buf, PAGE_SIZE, "Off\n");

	info.ndev = frsm_amp->spkinfo.ndev;
	argv.buf = &info;
	argv.size = sizeof(info);
	ret = frsm_amp_get_livedata(frsm_amp, &argv);
	if (ret)
		return ret;

	for (i = 0, len = 0; i < info.ndev; i++) {
		data = info.data + i;
		len += snprintf(buf + len, PAGE_SIZE - len,
				"spk%d:%d,%d,%d,%d,%d\n", i + 1, data->spkre,
				data->spkr0, data->spkt0,
				data->spkf0, data->spkQ);
	}

	buf[len - 1] = '\n';

	return len;
}

static ssize_t fsalgo_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_adsp_pkg *pkg;
	struct frsm_amp *frsm_amp;
	int ret;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	pkg = kzalloc(len, GFP_KERNEL);
	if (pkg == NULL)
		return -ENOMEM;

	memcpy(pkg, buf, len);
	dev_info(frsm_amp->dev, "mid:%04x pid:%04x size:%d\n",
			pkg->module_id, pkg->param_id, pkg->size);
	ret = frsm_amp_set_fsalgo(frsm_amp, pkg);
	kfree(pkg);
	if (ret)
		return ret;

	return len;
}

static ssize_t init_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_mode_params *params;

	int ret;

	params = kzalloc(sizeof(struct frsm_mode_params), GFP_KERNEL);
	if (params == NULL)
		return -ENOMEM;

	memcpy(params, buf, len);
	ret = frsm_amp_init_dev(params->spkid, !!params->mode);
	kfree(params);
	if (ret)
		return ret;

	return len;
}

static ssize_t scene_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_mode_params *params;

	int ret;

	params = kzalloc(sizeof(struct frsm_mode_params), GFP_KERNEL);
	if (params == NULL)
		return -ENOMEM;

	memcpy(params, buf, len);
	ret = frsm_amp_set_scene(params->spkid, params->mode);
	kfree(params);
	if (ret)
		return ret;

	return len;
}

static ssize_t spkon_store(const_t struct class *class,
		const_t struct class_attribute *attr,
		const char *buf, size_t len)
{
	struct frsm_mode_params *params;

	int ret;

	params = kzalloc(sizeof(struct frsm_mode_params), GFP_KERNEL);
	if (params == NULL)
		return -ENOMEM;

	memcpy(params, buf, len);
	ret = frsm_amp_spk_switch(params->spkid, params->mode);
	kfree(params);
	if (ret)
		return ret;

	return len;
}

static ssize_t state_show(const_t struct class *class,
		const_t struct class_attribute *attr, char *buf)
{
	struct frsm_amp *frsm_amp;
	bool amp_on;
	int len;

	frsm_amp = container_of(class, struct frsm_amp, class_dev);
	amp_on = test_bit(EVENT_STREAM_ON, &frsm_amp->state);
	len = snprintf(buf, PAGE_SIZE, "%d\n", amp_on);

	return len;
}

static CLASS_ATTR_RW(ndev);
static CLASS_ATTR_RW(func);
static CLASS_ATTR_RW(regs);
static CLASS_ATTR_RW(monitor);
static CLASS_ATTR_RW(batt);
static CLASS_ATTR_WO(tunings);
static CLASS_ATTR_WO(calre);
static CLASS_ATTR_RO(livedata);
static CLASS_ATTR_WO(fsalgo);
static CLASS_ATTR_WO(init);
static CLASS_ATTR_WO(scene);
static CLASS_ATTR_WO(spkon);
static CLASS_ATTR_RO(state);

static struct attribute *frsm_amp_class_attrs[] = {
	&class_attr_ndev.attr,
	&class_attr_func.attr,
	&class_attr_regs.attr,
	&class_attr_tunings.attr,
	&class_attr_calre.attr,
	&class_attr_livedata.attr,
	&class_attr_fsalgo.attr,
	&class_attr_monitor.attr,
	&class_attr_batt.attr,
	&class_attr_init.attr,
	&class_attr_scene.attr,
	&class_attr_spkon.attr,
	&class_attr_state.attr,
	NULL,
};

ATTRIBUTE_GROUPS(frsm_amp_class);

static int frsm_amp_class_init(struct frsm_amp *frsm_amp)
{
	int ret;

	if (frsm_amp == NULL || frsm_amp->dev == NULL)
		return -EINVAL;

	if (!of_property_read_bool(frsm_amp->dev->of_node,
			"frsm,class-enable"))
		return 0;

	frsm_amp->class_dev.name = FRSM_AMP_NAME;
	frsm_amp->class_dev.class_groups = frsm_amp_class_groups;

	ret = class_register(&frsm_amp->class_dev);
	if (ret) {
		dev_err(frsm_amp->dev, "Failed to init classdev:%d\n", ret);
		return ret;
	}

	set_bit(FRSM_HAS_CLASS, &frsm_amp->func);

	return 0;
}

static void frsm_amp_class_deinit(struct frsm_amp *frsm_amp)
{
	if (frsm_amp && test_bit(FRSM_HAS_CLASS, &frsm_amp->func))
		class_unregister(&frsm_amp->class_dev);
}

#endif // __FRSM_AMP_CLASS_H__

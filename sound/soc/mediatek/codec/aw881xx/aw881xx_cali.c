/*
 * aw881xx_cali.c cali_module
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
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include "aw881xx.h"
#include "aw881xx_reg.h"
#include "aw881xx_cali.h"
#include "aw881xx_monitor.h"

/******************************************************
 *
 * aw881xx cali store
 *
 ******************************************************/

/*write cali to persist file example */
#ifdef AW_CALI_STORE_EXAMPLE

#define AWINIC_CALI_FILE  "/mnt/vendor/persist/factory/audio/aw_cali.bin"
#define AW_INT_DEC_DIGIT 10
static int aw881xx_write_cali_re_to_file(uint32_t cali_re, char *name_suffix)
{
	struct file *fp = NULL;
	char buf[50] = { 0 };
	loff_t pos = 0;
	mm_segment_t fs;
	char *channel = NULL;

	if (name_suffix == NULL) {
		channel = "mono";
	} else if (!strcmp(name_suffix, "l")) {
		channel = "left";
	} else if (!strcmp(name_suffix, "r")) {
		channel = "right";
		pos = AW_INT_DEC_DIGIT;
	} else {
		pr_err("%s: name_suffix:%s error\n", __func__, channel);
		return -EINVAL;
	}

	fp = filp_open(AWINIC_CALI_FILE, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		pr_err("%s:channel:%s open %s failed!\n",
			__func__, channel, AWINIC_CALI_FILE);
		return -EINVAL;
	}
	cali_re = ((cali_re * 1000) >> 12);
	snprintf(buf, sizeof(buf), "%10d", cali_re);

	fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(fp, buf, strlen(buf), &pos);

	set_fs(fs);

	pr_info("%s: channel:%s buf:%s cali_re:%d\n",
		__func__, channel, buf, cali_re);

	filp_close(fp, NULL);
	return 0;
}

static int aw881xx_get_cali_re_from_file(uint32_t *cali_re, char *name_suffix)
{
	struct file *fp = NULL;
	int f_size;
	char *buf = NULL;
	int32_t int_cali_re = 0;
	loff_t pos = 0;
	mm_segment_t fs;
	char *channel = NULL;

	if (name_suffix == NULL) {
		channel = "mono";
	} else if (!strcmp(name_suffix, "l")) {
		channel = "left";
	} else if (!strcmp(name_suffix, "r")) {
		channel = "right";
		pos = AW_INT_DEC_DIGIT;
	} else {
		pr_err("%s: name_suffix:%s error\n", __func__, channel);
		return -EINVAL;
	}

	fp = filp_open(AWINIC_CALI_FILE, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("%s:channel:%s open %s failed!\n",
			__func__, channel, AWINIC_CALI_FILE);
		return -EINVAL;
	}

	f_size = AW_INT_DEC_DIGIT;

	buf = kzalloc(f_size + 1, GFP_ATOMIC);
	if (!buf) {
		pr_err("%s: channel:%s malloc mem %d failed!\n",
			__func__, channel, f_size);
		filp_close(fp, NULL);
		return -EINVAL;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_read(fp, buf, f_size, &pos);

	set_fs(fs);

	if (sscanf(buf, "%d", &int_cali_re) == 1)
		*cali_re = ((int_cali_re << 12) / 1000);
	else
		*cali_re = AW_ERRO_CALI_VALUE;

	pr_info("%s: channel:%s buf:%s int_cali_re: %d\n",
		__func__, channel, buf, int_cali_re);

	kfree(buf);
	buf = NULL;
	filp_close(fp, NULL);

	return 0;

}
#endif


static int aw881xx_get_cali_re_from_phone(struct aw881xx *aw881xx)
{
	/* customer add, get re from nv or persist or cali file */
#ifdef AW_CALI_STORE_EXAMPLE
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;

	return aw881xx_get_cali_re_from_file(&cali_attr->cali_re,
					aw881xx->name_suffix);
#else
	return -EBUSY;
#endif
}

void aw881xx_get_cali_re(struct aw881xx_cali_attr *cali_attr)
{
	int ret;
	struct aw881xx *aw881xx =
		container_of(cali_attr, struct aw881xx, cali_attr);

	ret = aw881xx_get_cali_re_from_phone(aw881xx);
	if (ret < 0) {
		cali_attr->cali_re = AW_ERRO_CALI_VALUE;
		aw_dev_dbg(aw881xx->dev, "%s: find no re\n", __func__);
	}
}

static int aw881xx_set_cali_re_to_phone(struct aw881xx *aw881xx)
{
	/* customer add, set re to nv or persist or cali file */
#ifdef AW_CALI_STORE_EXAMPLE
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;

	return aw881xx_write_cali_re_to_file(cali_attr->cali_re, aw881xx->name_suffix);
#else
	return -EBUSY;
#endif
}

void aw881xx_set_cali_re_to_dsp(struct aw881xx_cali_attr *cali_attr)
{
	uint16_t cali_re = 0;
	uint16_t dsp_ra = 0;
	struct aw881xx *aw881xx =
		container_of(cali_attr, struct aw881xx, cali_attr);

	aw881xx_dsp_read(aw881xx, AW881XX_DSP_REG_CFG_ADPZ_RA, &dsp_ra);

	cali_re = cali_attr->cali_re + dsp_ra;

	/* set cali re to aw881xx */
	aw881xx_dsp_write(aw881xx, AW881XX_DSP_REG_CFG_ADPZ_RE,
			cali_re);

}

static void aw881xx_set_cali_re(struct aw881xx_cali_attr *cali_attr)
{
	struct aw881xx *aw881xx =
		container_of(cali_attr, struct aw881xx, cali_attr);

	aw881xx_set_cali_re_to_phone(aw881xx);

	/* set cali re to aw881xx */
	aw881xx_set_cali_re_to_dsp(cali_attr);

}

static ssize_t aw881xx_cali_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	int ret = -1;
	unsigned int databuf[2] = { 0 };

	ret = kstrtouint(buf, 0, &databuf[0]);
	if (ret < 0)
		return ret;

	cali_attr->cali_re = databuf[0] * (1 << AW881XX_DSP_RE_SHIFT) / 1000;
	aw881xx_set_cali_re(cali_attr);

	return count;
}

static ssize_t aw881xx_cali_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"cali_re=%dmohm\n",
			(uint32_t) (cali_attr->cali_re * 1000) /
			(1 << AW881XX_DSP_RE_SHIFT));

	return len;
}

static ssize_t aw881xx_re_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	int ret = -1;
	unsigned int databuf[2] = { 0 };

	ret = kstrtouint(buf, 0, &databuf[0]);
	if (ret < 0)
		return ret;

	cali_attr->re = databuf[0];

	return count;
}

static ssize_t aw881xx_re_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"re=%dmohm\n", cali_attr->re);

	return len;
}

static ssize_t aw881xx_f0_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	int ret = -1;
	unsigned int databuf[2] = { 0 };

	ret = kstrtouint(buf, 0, &databuf[0]);
	if (ret < 0)
		return ret;

	cali_attr->f0 = databuf[0];

	return count;
}

static ssize_t aw881xx_f0_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct aw881xx *aw881xx = dev_get_drvdata(dev);
	struct aw881xx_cali_attr *cali_attr = &aw881xx->cali_attr;
	ssize_t len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "f0=%dHz\n", cali_attr->f0);

	return len;
}

static DEVICE_ATTR(cali, S_IWUSR | S_IRUGO,
			aw881xx_cali_show, aw881xx_cali_store);
static DEVICE_ATTR(re, S_IWUSR | S_IRUGO,
			aw881xx_re_show, aw881xx_re_store);
static DEVICE_ATTR(f0, S_IWUSR | S_IRUGO,
			aw881xx_f0_show, aw881xx_f0_store);

static struct attribute *aw881xx_cali_attr[] = {
	&dev_attr_cali.attr,
	&dev_attr_re.attr,
	&dev_attr_f0.attr,
	NULL
};

static struct attribute_group aw881xx_cali_attr_group = {
	.attrs = aw881xx_cali_attr
};

void aw881xx_cali_init(struct aw881xx_cali_attr *cali_attr)
{
	int ret = -1;
	struct aw881xx *aw881xx =
		container_of(cali_attr, struct aw881xx, cali_attr);

	ret = sysfs_create_group(&aw881xx->dev->kobj, &aw881xx_cali_attr_group);
	if (ret < 0) {
		aw_dev_err(aw881xx->dev,
			"%s error creating sysfs attr files\n", __func__);
	}
}

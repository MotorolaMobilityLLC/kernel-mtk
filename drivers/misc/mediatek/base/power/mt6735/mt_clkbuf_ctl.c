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

/*
 * @file    mt_clk_buf_ctl.c
 * @brief   Driver for RF clock buffer control
 */

#define __MT_CLK_BUF_CTL_C__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/of.h>

#include "mt_spm.h"
#include "mt_spm_sleep.h"
#include "mt_clkbuf_ctl.h"

DEFINE_MUTEX(clk_buf_ctrl_lock);

#define DEFINE_ATTR_RO(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define DEFINE_ATTR_RW(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define __ATTR_OF(_name)	(&_name##_attr.attr)

static CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_DISABLE,
	CLK_BUF_SW_DISABLE,
	CLK_BUF_SW_DISABLE
};

#define BSI_CW_CNT		30 /* bits w DATA/READ bit */
#define BSI_CLK_HALF_PERIOD	1  /* in us */
#define BSI_CLK_PERIOD		(BSI_CLK_HALF_PERIOD << 1) /* in us */
#define BSI_DATA_BIT		0  /* 0: not tx data; 1: tx data */
#define BSI_READ_BIT		0  /* 0: write; 1: read */
#define BSI_CLK_MASK		0x7
#define BSI_CW_ADDR		252
#define BSI_CW_DEFAULT		0x01E8F

#define CLK_BUF_BSI_PAD_NUM	5
static unsigned int clk_buf_spm_cfg[CLK_BUF_BSI_PAD_NUM] = {
	0x00000000, /* BSI_EN_SR */
	0x00000000, /* BSI_CLK_SR */
	0x00000000, /* BSI_D0_SR */
	0x00000000, /* BSI_D1_SR */
	0x00000000, /* BSI_D2_SR */
};
static unsigned int clk_buf_bit_ctrl;

static unsigned int clk_buf_CW(CLK_BUF_SWCTRL_STATUS_T *status)
{
	unsigned int bsi_clk_setting, bsi_cw_data;

	bsi_clk_setting = ((status[3] << 2) | (status[2] << 1) | (status[1] << 0));
	bsi_cw_data = ((BSI_CW_DEFAULT & (~(BSI_CLK_MASK << 4))) |
			((bsi_clk_setting & BSI_CLK_MASK) << 4));

	return ((BSI_DATA_BIT & 0x1) << 29) | ((BSI_READ_BIT & 0x1) << 28) |
		((BSI_CW_ADDR & 0xFF) << 20) | ((bsi_cw_data & 0xFFFFF) << 0);
}


static void clk_buf_Send_BSI_CW(CLK_BUF_SWCTRL_STATUS_T *status)
{
	int  i;
	unsigned int cw;
	bool d0, d1, d2;

	clk_buf_spm_cfg[BSI_EN_SR] = 0x20000000;
	clk_buf_spm_cfg[BSI_CLK_SR] = 0x00000000;
	clk_buf_spm_cfg[BSI_D0_SR] = 0x00000000;
	clk_buf_spm_cfg[BSI_D1_SR] = 0x00000000;
	clk_buf_spm_cfg[BSI_D2_SR] = 0x00000000;
	clk_buf_bit_ctrl = 29;

	cw = clk_buf_CW(status);

	for (i = (BSI_CW_CNT-1); i >= 0; i = i - 3) {
		d0 = ((cw >> (i - 2)) & 0x1) ? 1 : 0;
		d1 = ((cw >> (i - 1)) & 0x1) ? 1 : 0;
		d2 = ((cw >> (i - 0)) & 0x1) ? 1 : 0;

		clk_buf_bit_ctrl -= 1;
		clk_buf_spm_cfg[BSI_EN_SR] |= (1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_CLK_SR] &= ~(1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D0_SR] |= (d0 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D1_SR] |= (d1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D2_SR] |= (d2 << clk_buf_bit_ctrl);

		clk_buf_bit_ctrl -= 1;
		clk_buf_spm_cfg[BSI_EN_SR] |= (1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_CLK_SR] |= (1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D0_SR] |= (d0 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D1_SR] |= (d1 << clk_buf_bit_ctrl);
		clk_buf_spm_cfg[BSI_D2_SR] |= (d2 << clk_buf_bit_ctrl);
	}

	clk_buf_bit_ctrl -= 1;
	clk_buf_spm_cfg[BSI_EN_SR] |= (1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_CLK_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D0_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D1_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D2_SR] &= ~(1 << clk_buf_bit_ctrl);

	clk_buf_bit_ctrl -= 1;
	clk_buf_spm_cfg[BSI_EN_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_CLK_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D0_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D1_SR] &= ~(1 << clk_buf_bit_ctrl);
	clk_buf_spm_cfg[BSI_D2_SR] &= ~(1 << clk_buf_bit_ctrl);

	spm_ap_bsi_gen(clk_buf_spm_cfg);
}


static void spm_clk_buf_ctrl(CLK_BUF_SWCTRL_STATUS_T *status)
{
	u32 spm_val;
	int i;

	spm_ap_mdsrc_req(1);

	spm_val = spm_read(SPM_SLEEP_MDBSI_CON) & ~0x7;

	for (i = 1; i < CLKBUF_NUM; i++)
		spm_val |= status[i] << (i-1);

	spm_write(SPM_SLEEP_MDBSI_CON, spm_val);

	udelay(2);

	spm_ap_mdsrc_req(0);
}

#define SPM_PWR_STATUS_MD	(1U << 0)
bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{
	unsigned int CLK_BUF1_STATUS, CLK_BUF2_STATUS, CLK_BUF3_STATUS, CLK_BUF4_STATUS;
	struct device_node *node;
	u32 vals[4];

	node = of_find_compatible_node(NULL, NULL, "mediatek,rf_clock_buffer");
	if (node) {
		of_property_read_u32_array(node, "mediatek,clkbuf-config", vals, 4);
		CLK_BUF1_STATUS = vals[0];
		CLK_BUF2_STATUS = vals[1];
		CLK_BUF3_STATUS = vals[2];
		CLK_BUF4_STATUS = vals[3];
	} else {
		pr_err("%s can't find compatible node\n", __func__);
		BUG();
	}

	if (id >= CLK_BUF_INVALID)
		return false;

	if ((id == CLK_BUF_BB_MD) && (CLK_BUF1_STATUS == CLOCK_BUFFER_HW_CONTROL))
		return false;

	if ((id == CLK_BUF_CONN) && (CLK_BUF2_STATUS == CLOCK_BUFFER_HW_CONTROL))
		return false;

	if ((id == CLK_BUF_NFC) && (CLK_BUF3_STATUS == CLOCK_BUFFER_HW_CONTROL))
		return false;

	if ((id == CLK_BUF_AUDIO) && (CLK_BUF4_STATUS == CLOCK_BUFFER_HW_CONTROL))
		return false;

	mutex_lock(&clk_buf_ctrl_lock);

	clk_buf_swctrl[id] = onoff;
	if ((spm_read(SPM_PWR_STATUS) & SPM_PWR_STATUS_MD) && (spm_read(SPM_PWR_STATUS_2ND) & SPM_PWR_STATUS_MD))
		spm_clk_buf_ctrl(clk_buf_swctrl);
	else

	clk_buf_Send_BSI_CW(clk_buf_swctrl);

	mutex_unlock(&clk_buf_ctrl_lock);

	return true;
}


void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status)
{
	int i;

	for (i = 0; i < CLKBUF_NUM; i++)
		status[i] = clk_buf_swctrl[i];
}

static ssize_t clk_buf_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	u32 clk_buf_en[CLKBUF_NUM], i;
	char cmd[32];

	if (sscanf(buf, "%s %x %x %x %x", cmd, &clk_buf_en[0], &clk_buf_en[1], &clk_buf_en[2], &clk_buf_en[3]) != 5)
		return -EPERM;

	for (i = 0; i < CLKBUF_NUM; i++)
		clk_buf_swctrl[i] = clk_buf_en[i];

	if (!strcmp(cmd, "bsi"))
		spm_clk_buf_ctrl(clk_buf_swctrl);
	else if (!strcmp(cmd, "apbsi"))
		clk_buf_Send_BSI_CW(clk_buf_swctrl);
	else
		return -EINVAL;

	return count;
}

static ssize_t clk_buf_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	char *p = buf;
	unsigned int CLK_BUF1_STATUS = 0, CLK_BUF2_STATUS = 0, CLK_BUF3_STATUS = 0, CLK_BUF4_STATUS = 0;
	struct device_node *node;
	u32 vals[4];

	node = of_find_compatible_node(NULL, NULL, "mediatek,rf_clock_buffer");
	if (node) {
		of_property_read_u32_array(node, "mediatek,clkbuf-config", vals, 4);
		CLK_BUF1_STATUS = vals[0];
		CLK_BUF2_STATUS = vals[1];
		CLK_BUF3_STATUS = vals[2];
		CLK_BUF4_STATUS = vals[3];
	} else {
		pr_err("%s can't find compatible node\n", __func__);
	}

	p += sprintf(p, "********** clock buffer state **********\n");
	p += sprintf(p, "CKBUF1 SW(1)/HW(2) CTL: %d, Disable(0)/Enable(1): %d\n", CLK_BUF1_STATUS, clk_buf_swctrl[0]);
	p += sprintf(p, "CKBUF2 SW(1)/HW(2) CTL: %d, Disable(0)/Enable(1): %d\n", CLK_BUF2_STATUS, clk_buf_swctrl[1]);
	p += sprintf(p, "CKBUF3 SW(1)/HW(2) CTL: %d, Disable(0)/Enable(1): %d\n", CLK_BUF3_STATUS, clk_buf_swctrl[2]);
	p += sprintf(p, "CKBUF4 SW(1)/HW(2) CTL: %d, Disable(0)/Enable(1): %d\n", CLK_BUF4_STATUS, clk_buf_swctrl[3]);
	p += sprintf(p, "\n********** clock buffer command help **********\n");
	p += sprintf(p, "BSI  switch on/off: echo bsi en1 en2 en3 en4 > /sys/power/clk_buf/clk_buf_ctrl\n");
	p += sprintf(p, "APBSI switch on/off: echo apbsi en1 en2 en3 en4 > /sys/power/clk_buf/clk_buf_ctrl\n");
	p += sprintf(p, "BB   :en1\n");
	p += sprintf(p, "6605 :en2\n");
	p += sprintf(p, "5193 :en3\n");
	p += sprintf(p, "AUDIO:en4\n");

	return p - buf;
}

DEFINE_ATTR_RW(clk_buf_ctrl);

static struct attribute *clk_buf_attrs[] = {
	__ATTR_OF(clk_buf_ctrl),
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name = "clk_buf",
	.attrs = clk_buf_attrs,
};

static int clk_buf_fs_init(void)
{
	int r;

#if defined(CONFIG_PM)
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
		pr_err("FAILED TO CREATE /sys/power/clk_buf (%d)\n", r);
	return r;
#endif
}

bool clk_buf_init(void)
{
	if (clk_buf_fs_init())
		return 0;
	return 1;
}

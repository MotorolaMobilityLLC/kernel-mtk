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

/**
 * @file    mt_clk_buf_ctl.c
 * @brief   Driver for RF clock buffer control
 *
 */

#define __MT_CLK_BUF_CTL_C__

/*=============================================================*/
/* Include files */
/*=============================================================*/

/* #define RF_CLK_BUF_BRING_UP // test for bring up */

#ifdef __KERNEL__		/* for kernel */

/* system includes */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/module.h>

#include <mt_spm.h>
#include <mach/mt_clkmgr.h>
#include "mt_clkbuf_ctl_internal.h"

#ifndef CONFIG_MTK_LEGACY
#include <linux/of.h>
#endif				/* ! CONFIG_MTK_LEGACY */

#define clk_buf_err(fmt, args...)       \
	pr_debug(fmt, ##args)
#define clk_buf_warn(fmt, args...)      \
	pr_debug(fmt, ##args)
#define clk_buf_info(fmt, args...)      \
	pr_debug(fmt, ##args)
#define clk_buf_dbg(fmt, args...)       \
	pr_debug(fmt, ##args)
#define clk_buf_ver(fmt, args...)       \
	pr_debug(fmt, ##args)

/*  */
/* LOCK */
/*  */
static DEFINE_SPINLOCK(clk_buf_ctrl_lock);

#define clkbuf_lock(flags) spin_lock_irqsave(&clk_buf_ctrl_lock, flags)
#define clkbuf_unlock(flags) spin_unlock_irqrestore(&clk_buf_ctrl_lock, flags)

#define clkbuf_read(addr)            __raw_readl(IOMEM(addr))
#define clkbuf_write(addr, val)      mt_reg_sync_writel(val, addr)

#else				/* for preloader */

#ifdef MTKDRV_CLKBUF_CTL	/* for CTP */

#include <kernel_to_ctp.h>
#include "mt_clkbuf_ctl.h"

#define clk_buf_emerg(fmt, args...)     must_print("[clk_buf] " fmt, ##args)
#define clk_buf_alert(fmt, args...)     must_print("[clk_buf] " fmt, ##args)
#define clk_buf_crit(fmt, args...)      must_print("[clk_buf] " fmt, ##args)
#define clk_buf_err(fmt, args...)	must_print("[clk_buf] " fmt, ##args)
#define clk_buf_warn(fmt, args...)	must_print("[clk_buf] " fmt, ##args)
#define clk_buf_notice(fmt, args...)    must_print("[clk_buf] " fmt, ##args)
#define clk_buf_info(fmt, args...)      opt_print("[clk_buf] " fmt, ##args)
#define clk_buf_dbg(fmt, args...)	opt_print("[clk_buf] " fmt, ##args)

#else
#include <platform.h>
#include <spm.h>
#include <spm_mtcmos.h>
#include <spm_mtcmos_internal.h>
#include <timer.h>		/* udelay */
#include <clkbuf_ctl.h>

#define clk_buf_err(fmt, args...)	print("[clk_buf] " fmt, ##args)
#define clk_buf_warn(fmt, args...)      print("[clk_buf] " fmt, ##args)
#define clk_buf_info(fmt, args...)      print("[clk_buf] " fmt, ##args)
#define clk_buf_dbg(fmt, args...)       print("[clk_buf] " fmt, ##args)
#define clk_buf_ver(fmt, args...)       print("[clk_buf] " fmt, ##args)

#endif

#define clkbuf_read(addr)            DRV_Reg32(addr)
#define clkbuf_write(addr, val)      DRV_WriteReg32(val, addr)

#endif

#if 0				/* Rainier FIXME */
static CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_DISABLE,
	CLK_BUF_SW_DISABLE
};
#else
static CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE
};

static CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl_default[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_CONNSRC,
#ifdef CONFIG_MTK_NFC
	CLK_BUF_SW_NFC_NFCSRC,
#else
	CLK_BUF_SW_DISABLE,
#endif
	CLK_BUF_SW_DISABLE
};
#endif

static void check_clk_buf_regs(void)
{
	unsigned int value;
	unsigned int tmp;

	value = clkbuf_read(RF_CK_BUF_REG);

	/* get value from cksel2_sel */
	tmp = clk_buf_swctrl[1] = (value >> 8) & 0x3;
	/* get value from cksel2_reg if cksel2_sel = 1 */
	if (tmp == 1)
		clk_buf_swctrl[1] = (value >> 15) & 0x1;
	else if (tmp == 0)
		clk_buf_swctrl[1] = CLK_BUF_HW_SPM;

	/* get value from cksel3_sel */
	tmp = clk_buf_swctrl[2] = (value >> 16) & 0x3;
	/* get value from cksel3_reg if cksel3_sel = 1 */
	if (tmp == 1)
		clk_buf_swctrl[2] = (value >> 23) & 0x1;
	else if (tmp == 0)
		clk_buf_swctrl[2] = CLK_BUF_HW_SPM;

	/* get value from cksel4_sel */
	tmp = clk_buf_swctrl[3] = (value >> 24) & 0xf;
	/* get value from cksel4_reg if cksel4_sel = 1 */
	if (tmp == 1)
		clk_buf_swctrl[3] = (value >> 31) & 0x1;
	else if (tmp == 0)
		clk_buf_swctrl[3] = CLK_BUF_HW_SPM;
}

#ifndef CONFIG_MTK_LEGACY
unsigned int CLK_BUF1_STATUS, CLK_BUF2_STATUS, CLK_BUF3_STATUS, CLK_BUF4_STATUS;
#endif				/* ! CONFIG_MTK_LEGACY */

static int audio_ref_count = 1;
static void clk_buf_ctrl_buf1(CLK_BUF_SWCTRL_STATUS_T *status)
{
	/* clk_buf_dbg("#@# CLKBUF %d %d %d %d\n", status[0], status[1], status[2], status[3]); */
#ifndef RF_CLK_BUF_BRING_UP	/* test for bring up */
	if (CLK_BUF2_STATUS != CLOCK_BUFFER_HW_CONTROL) {
		if (status[1] == CLK_BUF_SW_DISABLE) {
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_CONN_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_CONN_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_CONN_GRP_SET(1));
		} else if (status[1] == CLK_BUF_SW_ENABLE) {
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_CONN_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_CONN_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_CONN_GRP_SET(1));
		} else if (status[1] == CLK_BUF_SW_CONNSRC) {
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_CONN_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_CONN_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_CONN_GRP_SET(status[1]));
		}
	}
#endif
}

static void clk_buf_ctrl_buf2(CLK_BUF_SWCTRL_STATUS_T *status)
{
	/* clk_buf_dbg("#@# CLKBUF %d %d %d %d\n", status[0], status[1], status[2], status[3]); */
#ifndef RF_CLK_BUF_BRING_UP	/* test for bring up */
	if (CLK_BUF3_STATUS != CLOCK_BUFFER_HW_CONTROL) {
		if (status[2] == CLK_BUF_SW_DISABLE) {
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_NFC_GRP_SET(1));
		} else if (status[2] == CLK_BUF_SW_ENABLE) {
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_NFC_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_NFC_GRP_SET(1));
		} else if (status[2] == CLK_BUF_SW_NFCSRC) {
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_SEL);
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_NFC_GRP_SET(status[2]));
		} else if (status[2] == CLK_BUF_SW_NFC_NFCSRC) {
			clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_NFC_GRP_MASK);
			clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_NFC_GRP_SET(status[2]));
		}
	}
#endif
}

static void clk_buf_ctrl_buf3(CLK_BUF_SWCTRL_STATUS_T *status)
{
	/* clk_buf_dbg("#@# CLKBUF %d %d %d %d\n", status[0], status[1], status[2], status[3]); */
#ifndef RF_CLK_BUF_BRING_UP	/* test for bring up */
	if (CLK_BUF4_STATUS != CLOCK_BUFFER_HW_CONTROL) {
		if (status[3] == CLK_BUF_SW_DISABLE) {
			audio_ref_count--;
			if (audio_ref_count == 0) {
				clkbuf_write(RF_CK_BUF_REG_CLR, CLK_BUF_AUDIO_SEL);
				clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_AUDIO_GRP_SET(1));
			}
		} else if (status[3] == CLK_BUF_SW_ENABLE) {
			if (audio_ref_count == 0) {
				clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_AUDIO_SEL);
				clkbuf_write(RF_CK_BUF_REG_SET, CLK_BUF_AUDIO_GRP_SET(1));
			}
			audio_ref_count++;
		}
	}
#endif
}


static void clk_buf_ctrl_by_reg(CLK_BUF_SWCTRL_STATUS_T *status)
{
	/* CLK_BUF_CONN */
	clk_buf_ctrl_buf1(status);
	/* CLK_BUF_NFC */
	clk_buf_ctrl_buf2(status);
	/* CLK_BUF_AUDIO */
	clk_buf_ctrl_buf3(status);
}

#ifdef __KERNEL__		/* for kernel */

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{
	int swctrl_id = 0;
	int swctrl_val = 0;
	unsigned long flags;

	clkbuf_lock(flags);
	check_clk_buf_regs();

	switch (id) {
	case CLK_BUF_BB:
		swctrl_id = 0;
		swctrl_val = CLK_BUF_BB_GRP_GET(id);
		if (CLK_BUF1_STATUS == CLOCK_BUFFER_HW_CONTROL) {
			clkbuf_unlock(flags);
			return false;
		}
		break;
	case CLK_BUF_CONN:
	case CLK_BUF_CONNSRC:
		swctrl_id = 1;
		swctrl_val = CLK_BUF_CONN_GRP_GET(id);
		if (CLK_BUF2_STATUS == CLOCK_BUFFER_HW_CONTROL) {
			clkbuf_unlock(flags);
			return false;
		}
		if (onoff)
			clk_buf_swctrl[swctrl_id] = swctrl_val;
		else
			clk_buf_swctrl[swctrl_id] = CLK_BUF_SW_DISABLE;
		clk_buf_ctrl_buf1(clk_buf_swctrl);
		break;
	case CLK_BUF_NFC:
	case CLK_BUF_NFCSRC:
	case CLK_BUF_NFC_NFCSRC:
		swctrl_id = 2;
		swctrl_val = CLK_BUF_NFC_GRP_GET(id);
		if (CLK_BUF3_STATUS == CLOCK_BUFFER_HW_CONTROL) {
			clkbuf_unlock(flags);
			return false;
		}
		if (onoff)
			clk_buf_swctrl[swctrl_id] = swctrl_val;
		else
			clk_buf_swctrl[swctrl_id] = CLK_BUF_SW_DISABLE;
		clk_buf_ctrl_buf2(clk_buf_swctrl);
		break;
	case CLK_BUF_AUDIO:
		swctrl_id = 3;
		swctrl_val = CLK_BUF_AUDIO_GRP_GET(id);
		if (CLK_BUF4_STATUS == CLOCK_BUFFER_HW_CONTROL) {
			clkbuf_unlock(flags);
			return false;
		}
		if (onoff)
			clk_buf_swctrl[swctrl_id] = swctrl_val;
		else
			clk_buf_swctrl[swctrl_id] = CLK_BUF_SW_DISABLE;
		clk_buf_ctrl_buf3(clk_buf_swctrl);
		break;
	default:
		clkbuf_unlock(flags);
		return false;
	}
	clkbuf_unlock(flags);
	return true;
}


void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status)
{
	int i;
	unsigned long flags;

	clkbuf_lock(flags);
	check_clk_buf_regs();

	for (i = 0; i < CLKBUF_NUM; i++)
		status[i] = clk_buf_swctrl[i];
	clkbuf_unlock(flags);
}


#if defined(CONFIG_PM)
static ssize_t clk_buf_ctrl_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	/*design for BSI wrapper command or by GPIO wavefrom */
	u32 clk_buf_en[CLKBUF_NUM], i;
	char cmd[32];
	unsigned long flags;

	if (sscanf
	    (buf, "%31s %x %x %x %x", cmd, &clk_buf_en[0], &clk_buf_en[1], &clk_buf_en[2],
	     &clk_buf_en[3]) != 5)
		return -EPERM;

	clkbuf_lock(flags);
	check_clk_buf_regs();
	for (i = 0; i < CLKBUF_NUM; i++)
		clk_buf_swctrl[i] = clk_buf_en[i];

	if (!strcmp(cmd, "reg")) {
		clk_buf_dbg("#@# calling clk_buf_ctrl_by_reg() CLKBUF %d %d %d %d\n",
				clk_buf_swctrl[0], clk_buf_swctrl[1], clk_buf_swctrl[2], clk_buf_swctrl[3]);
		clk_buf_ctrl_by_reg(clk_buf_swctrl);
		clkbuf_unlock(flags);
		return count;
	}

	clkbuf_unlock(flags);
	return -EINVAL;
}

static ssize_t clk_buf_ctrl_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int len = 0;
	char *p = (char *)buf;

	check_clk_buf_regs();

	p += sprintf(p, "********** clock buffer state **********\n");
	p += sprintf(p, "CKBUF1 SW(1)/HW(2) CTL: %d, SPM(-1)/Disable(0)/Enable(1): %d\n",
		     CLK_BUF1_STATUS, clk_buf_swctrl[0]);
	p += sprintf(p, "CKBUF2 SW(1)/HW(2) CTL: %d, SPM(-1)/Disable(0)/Enable(1): %d\n",
		     CLK_BUF2_STATUS, clk_buf_swctrl[1]);
	p += sprintf(p, "CKBUF3 SW(1)/HW(2) CTL: %d, SPM(-1)/Disable(0)/Enable(1): %d\n",
		     CLK_BUF3_STATUS, clk_buf_swctrl[2]);
	p += sprintf(p, "CKBUF4 SW(1)/HW(2) CTL: %d, SPM(-1)/Disable(0)/Enable(1): %d\n",
		     CLK_BUF4_STATUS, clk_buf_swctrl[3]);
	p += sprintf(p, "\n********** clock buffer command help **********\n");
	p += sprintf(p,
		     "REG switch on/off: echo reg en1 en2 en3 en4 > /sys/power/clk_buf/clk_buf_ctrl\n");
	p += sprintf(p, "BB   :en1\n");
	p += sprintf(p, "CONN :en2\n");
	p += sprintf(p, "NFC  :en3\n");
	p += sprintf(p, "AUDIO:en4\n");

	len = p - buf;

	return len;
}

static struct kobj_attribute clk_buf_ctrl_attr =
	__ATTR(clk_buf_ctrl, 0644, clk_buf_ctrl_show, clk_buf_ctrl_store);

static struct attribute *clk_buf_attrs[] = {

	/* for clock buffer control */
	&clk_buf_ctrl_attr.attr,

	/* must */
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name = "clk_buf",
	.attrs = clk_buf_attrs,
};


static int clk_buf_fs_init(void)
{
	int r;

	/* create /sys/power/clk_buf/xxx */
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
		clk_buf_err("FAILED TO CREATE /sys/power/clk_buf (%d)\n", r);
	return r;
}
#endif


bool clk_buf_init(void)
{
	unsigned long flags;
#ifndef CONFIG_MTK_LEGACY
	struct device_node *node;
	static u32 vals[4];

	node = of_find_compatible_node(NULL, NULL, "mediatek,rf_clock_buffer");
	if (node) {
		int err;

		err = of_property_read_u32_array(node, "mediatek,clkbuf-config", vals, 4);
		if (err) {
			clk_buf_err("%s can't find array of mediatek,clkbuf-config\n", __func__);
			vals[0] = CLOCK_BUFFER_HW_CONTROL;
			vals[1] = CLOCK_BUFFER_SW_CONTROL;
			vals[2] = CLOCK_BUFFER_SW_CONTROL;
			vals[3] = CLOCK_BUFFER_SW_CONTROL;
		}
		CLK_BUF1_STATUS = vals[0];
		CLK_BUF2_STATUS = vals[1];
		CLK_BUF3_STATUS = vals[2];
		CLK_BUF4_STATUS = vals[3];
	} else {
		clk_buf_err("%s can't find compatible node\n", __func__);
	}
#endif				/* ! CONFIG_MTK_LEGACY */

#if defined(CONFIG_PM)
	if (clk_buf_fs_init())
		return 0;
#endif

	clkbuf_lock(flags);
	clk_buf_dbg("#@# calling clk_buf_ctrl_by_reg() CLKBUF %d %d %d %d\n",
			clk_buf_swctrl[0], clk_buf_swctrl[1], clk_buf_swctrl[2], clk_buf_swctrl[3]);
	clk_buf_ctrl_by_reg(clk_buf_swctrl_default);
	clkbuf_unlock(flags);
	return 1;
}

#else				/* for preloader */

void clk_buf_all_on(void)
{
	clk_buf_ctrl_by_reg(clk_buf_swctrl);
}

void clk_buf_all_default(void)
{
	clk_buf_ctrl_by_reg(clk_buf_swctrl_default);
}

#endif

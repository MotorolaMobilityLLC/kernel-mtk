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
 * @file    mtk_clk_buf_ctl.c
 * @brief   Driver for clock buffer control
 *
 */

#define __MTK_CLK_BUF_CTL_C__

/*
 * Include files
 */
#ifdef __KERNEL__
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
#include <linux/of_address.h>
#include <linux/ratelimit.h>
#include <linux/workqueue.h>

#include <mt-plat/sync_write.h>
#include <mt-plat/upmu_common.h>

#else /* LK */
#include <debug.h>
#include <mt_typedefs.h>
#include <sync_write.h>
#include <mt_reg_base.h>
#include <mt_pmic.h>
#include <upmu_hw.h>

#endif /* __KERNEL__ */

#include <mtk_clkbuf_ctl.h>

#define TAG     "[Power/clkbuf]"

#ifdef __KERNEL__
#define clk_buf_err(fmt, args...)		pr_err(TAG fmt, ##args)
#define clk_buf_warn(fmt, args...)		pr_warn(TAG fmt, ##args)
#define clk_buf_warn_limit(fmt, args...)	pr_warn_ratelimited(TAG fmt, ##args)
#define clk_buf_dbg(fmt, args...)			\
	do {						\
		if (clkbuf_debug)			\
			pr_warn(TAG fmt, ##args);	\
	} while (0)

#define clkbuf_readl(addr)			__raw_readl(addr)
#define clkbuf_writel(addr, val)	mt_reg_sync_writel(val, addr)

DEFINE_MUTEX(clk_buf_ctrl_lock);

#ifdef CONFIG_PM
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
#endif /* CONFIG_PM */

static void __iomem *pwrap_base;

#define PWRAP_REG(ofs)		(pwrap_base + ofs)

#else /* LK */
#define clk_buf_err(fmt, args...)		dprintf(CRITICAL, fmt, ##args)
#define clk_buf_warn(fmt, args...)		dprintf(ALWAYS, fmt, ##args)
#define clk_buf_warn_limit(fmt, args...)	dprintf(ALWAYS, fmt, ##args)
#define clk_buf_dbg(fmt, args...)		dprintf(INFO, fmt, ##args)

#define clkbuf_readl(addr)			DRV_Reg32(addr)
#define clkbuf_writel(addr, val)		DRV_WriteReg32(addr, val)

#define late_initcall(a)

#define mutex_lock(a)
#define mutex_unlock(a)

#define PWRAP_REG(ofs)		(PWRAP_BASE + ofs)

#define false 0
#define true 1
typedef int bool;
#endif /* __KERNEL__ */

/* PMICWRAP Reg */
#define DCXO_ENABLE		PWRAP_REG(0x188)
#define DCXO_CONN_ADR0		PWRAP_REG(0x18C)
#define DCXO_CONN_WDATA0	PWRAP_REG(0x190)
#define DCXO_CONN_ADR1		PWRAP_REG(0x194)
#define DCXO_CONN_WDATA1	PWRAP_REG(0x198)
#define DCXO_NFC_ADR0		PWRAP_REG(0x19C)
#define DCXO_NFC_WDATA0		PWRAP_REG(0x1A0)
#define DCXO_NFC_ADR1		PWRAP_REG(0x1A4)
#define DCXO_NFC_WDATA1		PWRAP_REG(0x1A8)

#define	DCXO_CONN_ENABLE	(0x1 << 1)
#define	DCXO_NFC_ENABLE		(0x1 << 0)

#define PMIC_REG_MASK				0xFFFF
#define PMIC_REG_SHIFT				0

#define PMIC_CW00_ADDR				0x7000 /* only for MT6351 */
#define PMIC_CW00_INIT_VAL			0x4FFD
#define PMIC_CW00_XO_EXTBUF2_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF2_MODE_SHIFT		3
#define PMIC_CW00_XO_EXTBUF2_EN_M_MASK		0x1
#define PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT		5
#define PMIC_CW00_XO_EXTBUF3_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF3_MODE_SHIFT		6
#define PMIC_CW00_XO_BB_LPM_EN_MASK		0x1
#define PMIC_CW00_XO_BB_LPM_EN_SHIFT		12
#define PMIC_CW02_INIT_VAL			0xB86A
#define PMIC_CW11_INIT_VAL			0xA400

/*
 * 0x701A	CW13	Code Word 13
 * 15:14	RG_XO_EXTBUF4_ISET	XO Control Signal of Current on EXTBUF4
 * 13:12	RG_XO_EXTBUF4_HD	XO Control Signal of EXTBUF4 Output driving Strength
 * 11:10	RG_XO_EXTBUF3_ISET	XO Control Signal of Current on EXTBUF3
 * 9:8		RG_XO_EXTBUF3_HD	XO Control Signal of EXTBUF3 Output driving Strength
 * 7:6		RG_XO_EXTBUF2_ISET	XO Control Signal of Current on EXTBUF2
 * 5:4		RG_XO_EXTBUF2_HD	XO Control Signal of EXTBUF2 Output driving Strength
 * 3:2		RG_XO_EXTBUF1_ISET	XO Control Signal of Current on EXTBUF1
 * 1:0		RG_XO_EXTBUF1_HD	XO Control Signal of EXTBUF1 Output driving Strength
 */
#define PMIC_CW13_ADDR				0x701A
#define PMIC_CW13_SUGGEST_VAL			0x4666
#define PMIC_CW13_XO_EXTBUF_HD_VAL		((0x2 << 0) | (0x2 << 4) \
						 | (0x2 << 8) | (0 << 12))

#define PMIC_CW14_ADDR				0x701C
#define PMIC_CW14_XO_EXTBUF3_EN_M_MASK		0x1
#define PMIC_CW14_XO_EXTBUF3_EN_M_SHIFT		11

#define PMIC_CW15_ADDR				0x701E
#define PMIC_CW15_DCXO_STATIC_AUXOUT_EN_MASK	0x1
#define PMIC_CW15_DCXO_STATIC_AUXOUT_EN_SHIFT	0
#define PMIC_CW15_DCXO_STATIC_AUXOUT_SEL_MASK	0xF
#define PMIC_CW15_DCXO_STATIC_AUXOUT_SEL_SHIFT	1

/* TODO: marked this after driver is ready */
#define CLKBUF_BRINGUP

static bool is_clkbuf_initiated;
static bool g_is_flightmode_on;
static bool clkbuf_debug;

/* false: rf_clkbuf, true: pmic_clkbuf */
static bool is_pmic_clkbuf = true;

static unsigned int bblpm_cnt;

static int g_xo_drv_curr_val = -1;
/* static unsigned short clkbuf_ctrl_stat; */
#define CLKBUF_CTRL_STAT_MASK ((0x1 << CLK_BUF_BB_MD) | (0x1 << CLK_BUF_CONN) | \
			       (0x1 << CLK_BUF_NFC) | (0x1 << CLK_BUF_RF) | \
			       (0x1 << CLK_BUF_AUDIO) | (0x1 << CLK_BUF_CHG) | \
			       (0x1 << CLK_BUF_UFS))
#define CLKBUF_CTRL_STAT_XO_PD_MASK ((0x1 << CLK_BUF_AUDIO) | (0x1 << CLK_BUF_CHG))

/* FIXME: Before MP, using suggested driving current to test. */
/* #define TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP */

#ifdef __KERNEL__

#if !defined(CONFIG_MTK_LEGACY)
static unsigned int CLK_BUF1_STATUS_PMIC = CLOCK_BUFFER_HW_CONTROL,
		    CLK_BUF2_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL,
		    CLK_BUF3_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL,
		    CLK_BUF4_STATUS_PMIC = CLOCK_BUFFER_HW_CONTROL,
		    CLK_BUF5_STATUS_PMIC = CLOCK_BUFFER_DISABLE,
		    CLK_BUF6_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL,
		    CLK_BUF7_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL;
static int PMIC_CLK_BUF1_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF2_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF3_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF4_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF5_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF6_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K,
	   PMIC_CLK_BUF7_DRIVING_CURR = CLK_BUF_DRIVING_CURR_AUTO_K;
#endif /* CONFIG_MTK_LEGACY */

#ifndef CLKBUF_BRINGUP
static CLK_BUF_SWCTRL_STATUS_T  pmic_clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_DISABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE
};
#else /* For Bring-up */
static CLK_BUF_SWCTRL_STATUS_T  pmic_clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE
};
#endif

static void pmic_clk_buf_ctrl_wcn(short on)
{
	if (on)
		pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF2_EN_M_MASK,
				      PMIC_XO_EXTBUF2_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF2_EN_M_MASK,
				      PMIC_XO_EXTBUF2_EN_M_SHIFT);
}

static void pmic_clk_buf_ctrl(CLK_BUF_SWCTRL_STATUS_T *status)
{
	u32 pmic_cw00 = 0, pmic_cw11 = 0;

	if (!is_clkbuf_initiated)
		return;

	pmic_clk_buf_ctrl_wcn(status[XO_WCN] % 2);

	if (status[XO_NFC] % 2)
		pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF3_EN_M_MASK,
				      PMIC_XO_EXTBUF3_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF3_EN_M_MASK,
				      PMIC_XO_EXTBUF3_EN_M_SHIFT);
	if (status[XO_AUD] % 2)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF5_EN_M_MASK,
				      PMIC_XO_EXTBUF5_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF5_EN_M_MASK,
				      PMIC_XO_EXTBUF5_EN_M_SHIFT);
	if (status[XO_PD] % 2)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF6_EN_M_MASK,
				      PMIC_XO_EXTBUF6_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF6_EN_M_MASK,
				      PMIC_XO_EXTBUF6_EN_M_SHIFT);
	if (status[XO_EXT] % 2)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF7_EN_M_MASK,
				      PMIC_XO_EXTBUF7_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF7_EN_M_MASK,
				      PMIC_XO_EXTBUF7_EN_M_SHIFT);

	pmic_read_interface(MT6335_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW11=0x%x, clk_buf_swctrl=[%u %u %u %u %u %u %u]\n",
		     __func__, pmic_cw00, pmic_cw11, status[0], status[1],
		     status[2], status[3], status[4], status[5], status[6]);
}

/*
 * Baseband Low Power Mode (BBLPM) for PMIC clkbuf
 * Condition: TBD
 * Caller: deep idle
 */
void clk_buf_control_bblpm(bool on)
{
/* TODO: Disable BBLPM before conditions are confirmed */
#if 0
	u32 cw00 = 0;

	if (!is_clkbuf_initiated)
		return;

	if (!is_pmic_clkbuf ||
	    (pmic_clk_buf_swctrl[CLK_BUF_NFC] == CLK_BUF_SW_ENABLE))
		return;

	if (on) /* FPM -> BBLPM */
		pmic_config_interface_nolock(PMIC_DCXO_CW00_SET_ADDR, 0x1,
				      PMIC_XO_BB_LPM_EN_MASK,
				      PMIC_XO_BB_LPM_EN_SHIFT);
	else /* BBLPM -> FPM */
		pmic_config_interface_nolock(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
				      PMIC_XO_BB_LPM_EN_MASK,
				      PMIC_XO_BB_LPM_EN_SHIFT);

	pmic_read_interface_nolock(MT6335_DCXO_CW00, &cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);

	bblpm_cnt++;

	clk_buf_dbg("%s(%u): CW00=0x%x, bblpm_cnt=%u\n", __func__, (on ? 1 : 0),
		    cw00, bblpm_cnt);
#endif
}

/* for spm driver use */
bool is_clk_buf_under_flightmode(void)
{
	return g_is_flightmode_on;
}

/* for ccci driver to notify this */
void clk_buf_set_by_flightmode(bool is_flightmode_on)
{
}

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{
	if (!is_clkbuf_initiated)
		return false;

	if (!is_pmic_clkbuf)
		return false;

	if (id >= CLK_BUF_INVALID) /* TODO: need check DCT tool for CLK BUF SW control */
		return false;

	if ((id == CLK_BUF_BB_MD) && (CLK_BUF1_STATUS_PMIC == CLOCK_BUFFER_HW_CONTROL))
		return false;

	if ((id == CLK_BUF_RF) && (CLK_BUF4_STATUS_PMIC == CLOCK_BUFFER_HW_CONTROL))
		return false;

	clk_buf_dbg("%s: id=%d, onoff=%d\n", __func__, id, onoff);

	mutex_lock(&clk_buf_ctrl_lock);

	/* TODO: control clock buffer for SW mode */
	/* record the status of NFC from caller for checking BBLPM */
	if (id == CLK_BUF_NFC)
		pmic_clk_buf_swctrl[id] = onoff;

	mutex_unlock(&clk_buf_ctrl_lock);

	return true;
}

void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status)
{
	if (!is_clkbuf_initiated)
		return;

	if (is_pmic_clkbuf)
		return;
}

/*
 * Let caller get driving current setting of RF clock buffer
 * Caller: ccci & ccci will send it to modem
 */
void clk_buf_get_rf_drv_curr(void *rf_drv_curr)
{
	if (!is_clkbuf_initiated)
		return;

	if (is_pmic_clkbuf)
		return;
}

/* Called by ccci driver to keep afcdac value sent from modem */
void clk_buf_save_afc_val(unsigned int afcdac)
{
	if (is_pmic_clkbuf)
		return;
}

/* Called by suspend driver to write afcdac into SPM register */
void clk_buf_write_afcdac(void)
{
	if (!is_clkbuf_initiated)
		return;

	if (is_pmic_clkbuf)
		return;
}

#ifdef CONFIG_PM
static ssize_t clk_buf_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	u32 clk_buf_en[CLKBUF_NUM], i;
	char cmd[32];

	if (sscanf(buf, "%31s %x %x %x %x %x %x %x", cmd, &clk_buf_en[0],
		   &clk_buf_en[1], &clk_buf_en[2], &clk_buf_en[3],
		   &clk_buf_en[4], &clk_buf_en[5], &clk_buf_en[6])
	    != (CLKBUF_NUM + 1))
		return -EPERM;

	if (!strcmp(cmd, "pmic")) {
		if (!is_pmic_clkbuf)
			return -EINVAL;

		mutex_lock(&clk_buf_ctrl_lock);

		for (i = 0; i < CLKBUF_NUM; i++)
			pmic_clk_buf_swctrl[i] = clk_buf_en[i];

		pmic_clk_buf_ctrl(pmic_clk_buf_swctrl);

		mutex_unlock(&clk_buf_ctrl_lock);

		return count;
	}

	return -EINVAL;
}

static ssize_t clk_buf_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	int len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,
			"********** PMIC clock buffer state (%s) **********\n",
			(is_pmic_clkbuf ? "on" : "off"));
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF1_BB   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF1_STATUS_PMIC, pmic_clk_buf_swctrl[0]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF2_CONN SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF2_STATUS_PMIC, pmic_clk_buf_swctrl[1]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF3_NFC  SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF3_STATUS_PMIC, pmic_clk_buf_swctrl[2]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF4_RF   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF4_STATUS_PMIC, pmic_clk_buf_swctrl[3]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF5_RESERVED   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF5_STATUS_PMIC, pmic_clk_buf_swctrl[4]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF6_AUD_CHG SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF6_STATUS_PMIC, pmic_clk_buf_swctrl[5]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"CKBUF7_UFS  SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF7_STATUS_PMIC, pmic_clk_buf_swctrl[6]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n********** clock buffer command help **********\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"PMIC switch on/off: echo pmic en1 en2 en3 en4 en5 en6 en7 > /sys/power/clk_buf/clk_buf_ctrl\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"g_xo_drv_curr_val=0x%x, bblpm_cnt=%u\n",
			g_xo_drv_curr_val, bblpm_cnt);
	len += snprintf(buf+len, PAGE_SIZE-len, "pmic_drv_curr_vals=%d %d %d %d %d %d %d\n",
			PMIC_CLK_BUF1_DRIVING_CURR, PMIC_CLK_BUF2_DRIVING_CURR,
			PMIC_CLK_BUF3_DRIVING_CURR, PMIC_CLK_BUF4_DRIVING_CURR,
			PMIC_CLK_BUF5_DRIVING_CURR, PMIC_CLK_BUF6_DRIVING_CURR,
			PMIC_CLK_BUF7_DRIVING_CURR);

	return len;
}

static ssize_t clk_buf_debug_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	int debug = 0;

	if (!kstrtoint(buf, 10, &debug)) {
		if (debug == 0)
			clkbuf_debug = false;
		else if (debug == 1)
			clkbuf_debug = true;
		else
			clk_buf_warn("bad argument!! should be 0 or 1 [0: disable, 1: enable]\n");
	} else
		return -EPERM;

	return count;
}

static ssize_t clk_buf_debug_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	int len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "clkbuf_debug=%d\n", clkbuf_debug);

	return len;
}

DEFINE_ATTR_RW(clk_buf_ctrl);
DEFINE_ATTR_RW(clk_buf_debug);

static struct attribute *clk_buf_attrs[] = {
	/* for clock buffer control */
	__ATTR_OF(clk_buf_ctrl),
	__ATTR_OF(clk_buf_debug),

	/* must */
	NULL,
};

static struct attribute_group clk_buf_attr_group = {
	.name	= "clk_buf",
	.attrs	= clk_buf_attrs,
};
#endif /* CONFIG_PM */

bool is_clk_buf_from_pmic(void)
{
	return true;
};

static int clk_buf_fs_init(void)
{
	int r = 0;

#ifdef CONFIG_PM
	/* create /sys/power/clk_buf/xxx */
	r = sysfs_create_group(power_kobj, &clk_buf_attr_group);
	if (r)
		clk_buf_err("FAILED TO CREATE /sys/power/clk_buf (%d)\n", r);
#endif

	return r;
}

#if defined(CONFIG_OF)
static int clk_buf_dts_map(void)
{
	struct device_node *node;
#if !defined(CONFIG_MTK_LEGACY)
	u32 vals[CLKBUF_NUM] = {0, 0, 0, 0, 0, 0, 0};
	int ret = -1;

	node = of_find_compatible_node(NULL, NULL, "mediatek,pmic_clock_buffer");
	if (node) {
		ret = of_property_read_u32_array(node, "mediatek,clkbuf-config",
						 vals, CLKBUF_NUM);
		if (!ret) {
			CLK_BUF1_STATUS_PMIC = vals[0];
			CLK_BUF2_STATUS_PMIC = vals[1];
			CLK_BUF3_STATUS_PMIC = vals[2];
			CLK_BUF4_STATUS_PMIC = vals[3];
			CLK_BUF5_STATUS_PMIC = vals[4];
			CLK_BUF6_STATUS_PMIC = vals[5];
			CLK_BUF7_STATUS_PMIC = vals[6];
		}
		ret = of_property_read_u32_array(node, "mediatek,clkbuf-driving-current",
						 vals, CLKBUF_NUM);
		if (!ret) {
			PMIC_CLK_BUF1_DRIVING_CURR = vals[0];
			PMIC_CLK_BUF2_DRIVING_CURR = vals[1];
			PMIC_CLK_BUF3_DRIVING_CURR = vals[2];
			PMIC_CLK_BUF4_DRIVING_CURR = vals[3];
			PMIC_CLK_BUF5_DRIVING_CURR = vals[4];
			PMIC_CLK_BUF6_DRIVING_CURR = vals[5];
			PMIC_CLK_BUF7_DRIVING_CURR = vals[6];
		}
	} else {
		clk_buf_err("%s can't find compatible node for pmic_clock_buffer\n", __func__);
		return -1;
	}
#endif
	node = of_find_compatible_node(NULL, NULL, "mediatek,pwrap");
	if (node)
		pwrap_base = of_iomap(node, 0);
	else {
		clk_buf_err("%s can't find compatible node for pwrap\n",
		       __func__);
		return -1;
	}
}
#else
static int clk_buf_dts_map(void)
{
	return 0;
}
#endif

#endif /* __KERNEL__ */

static void clk_buf_dump_dts_log(void)
{
	clk_buf_warn("%s: PMIC_CLK_BUF?_STATUS=%d %d %d %d %d %d %d\n", __func__,
		     CLK_BUF1_STATUS_PMIC, CLK_BUF2_STATUS_PMIC,
		     CLK_BUF3_STATUS_PMIC, CLK_BUF4_STATUS_PMIC,
		     CLK_BUF5_STATUS_PMIC, CLK_BUF6_STATUS_PMIC,
		     CLK_BUF7_STATUS_PMIC);
	clk_buf_warn("%s: PMIC_CLK_BUF?_DRV_CURR=%d %d %d %d %d %d %d\n", __func__,
		     PMIC_CLK_BUF1_DRIVING_CURR,
		     PMIC_CLK_BUF2_DRIVING_CURR,
		     PMIC_CLK_BUF3_DRIVING_CURR,
		     PMIC_CLK_BUF4_DRIVING_CURR,
		     PMIC_CLK_BUF5_DRIVING_CURR,
		     PMIC_CLK_BUF6_DRIVING_CURR,
		     PMIC_CLK_BUF7_DRIVING_CURR);
}

static void clk_buf_gen_drv_curr_val(void)
{
/* FIXME */
#if 0
#ifdef TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP
	g_xo_drv_curr_val = PMIC_CW13_SUGGEST_VAL;
#else
	g_xo_drv_curr_val = PMIC_CW13_XO_EXTBUF_HD_VAL |
			     (PMIC_CLK_BUF5_DRIVING_CURR << 2) |
			     (PMIC_CLK_BUF6_DRIVING_CURR << 6) |
			     (PMIC_CLK_BUF7_DRIVING_CURR << 10) |
			     (PMIC_CLK_BUF8_DRIVING_CURR << 14);
#endif

	clk_buf_warn("%s: g_xo_drv_curr_val=0x%x, pmic_drv_curr_vals=%d %d %d %d\n",
		     __func__, g_xo_drv_curr_val,
		     PMIC_CLK_BUF5_DRIVING_CURR,
		     PMIC_CLK_BUF6_DRIVING_CURR,
		     PMIC_CLK_BUF7_DRIVING_CURR,
		     PMIC_CLK_BUF8_DRIVING_CURR);
#endif
}

static void clk_buf_pmic_wrap_init(void)
{
	u32 pmic_cw00 = 0, pmic_cw02 = 0, pmic_cw11 = 0;

	clk_buf_gen_drv_curr_val();

	/* Dump registers before setting */
	pmic_read_interface(MT6335_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW02, &pmic_cw02,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW02=0x%x, CW11=0x%x\n",
		     __func__, pmic_cw00, pmic_cw02, pmic_cw11);

	/* Setup initial PMIC clock buffer setting */
	pmic_config_interface(MT6335_DCXO_CW02, PMIC_CW02_INIT_VAL,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(MT6335_DCXO_CW00, PMIC_CW00_INIT_VAL,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(MT6335_DCXO_CW11, PMIC_CW11_INIT_VAL,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);

	/* Check if the setting is ok */
	pmic_read_interface(MT6335_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW02, &pmic_cw02,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(MT6335_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW02=0x%x, CW11=0x%x\n",
		     __func__, pmic_cw00, pmic_cw02, pmic_cw11);

	/* Setup PMIC_WRAP setting for XO2 & XO3 */
	clkbuf_writel(DCXO_CONN_ADR0, PMIC_DCXO_CW00_CLR_ADDR);
	clkbuf_writel(DCXO_CONN_WDATA0,
		      PMIC_XO_EXTBUF2_EN_M_MASK << PMIC_XO_EXTBUF2_EN_M_SHIFT);	/* bit5 = 0 */
	clkbuf_writel(DCXO_CONN_ADR1, PMIC_DCXO_CW00_SET_ADDR);
	clkbuf_writel(DCXO_CONN_WDATA1,
		      PMIC_XO_EXTBUF2_EN_M_MASK << PMIC_XO_EXTBUF2_EN_M_SHIFT);	/* bit5 = 1 */
	clkbuf_writel(DCXO_NFC_ADR0, PMIC_DCXO_CW00_CLR_ADDR);
	clkbuf_writel(DCXO_NFC_WDATA0,
		      PMIC_XO_EXTBUF3_EN_M_MASK << PMIC_XO_EXTBUF3_EN_M_SHIFT);	/* bit8 = 0 */
	clkbuf_writel(DCXO_NFC_ADR1, PMIC_DCXO_CW00_SET_ADDR);
	clkbuf_writel(DCXO_NFC_WDATA1,
		      PMIC_XO_EXTBUF3_EN_M_MASK << PMIC_XO_EXTBUF3_EN_M_SHIFT);	/* bit8 = 1 */

	clkbuf_writel(DCXO_ENABLE, DCXO_CONN_ENABLE | DCXO_NFC_ENABLE);

	clk_buf_warn("%s: DCXO_CONN_ADR0/WDATA0/ADR1/WDATA1/EN=0x%x/%x/%x/%x/%x\n",
		     __func__, clkbuf_readl(DCXO_CONN_ADR0),
		     clkbuf_readl(DCXO_CONN_WDATA0),
		     clkbuf_readl(DCXO_CONN_ADR1),
		     clkbuf_readl(DCXO_CONN_WDATA1),
		     clkbuf_readl(DCXO_ENABLE));
	clk_buf_warn("%s: DCXO_NFC_ADR0/WDATA0/ADR1/WDATA1=0x%x/%x/%x/%x\n",
		     __func__, clkbuf_readl(DCXO_NFC_ADR0),
		     clkbuf_readl(DCXO_NFC_WDATA0),
		     clkbuf_readl(DCXO_NFC_ADR1),
		     clkbuf_readl(DCXO_NFC_WDATA1));
}

int clk_buf_init(void)
{
#ifdef CLKBUF_BRINGUP
	clk_buf_warn("clk_buf_ctrl is initialized for bring-up\n");

	return 0;
#endif /* CLKBUF_BRINGUP */

	if (is_clkbuf_initiated)
		return -1;

	if (clk_buf_dts_map())
		return -1;

	clk_buf_dump_dts_log();

	if (clk_buf_fs_init())
		return -1;

	/* Co-TSX @PMIC */
	if (is_clk_buf_from_pmic()) {
		is_pmic_clkbuf = true;

		clk_buf_pmic_wrap_init();
	}

	is_clkbuf_initiated = true;

	return 0;
}
late_initcall(clk_buf_init);


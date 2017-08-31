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
 *
 */

#define __MT_CLK_BUF_CTL_C__

/*
 * Include files
 */

/* system includes */
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

/* #include <mach/mt_gpio_core.h> */
#include <mtk_clkbuf_ctl.h>

#define TAG     "[Power/clkbuf]"

#define clk_buf_err(fmt, args...)		pr_err(TAG fmt, ##args)
#define clk_buf_warn(fmt, args...)		pr_warn(TAG fmt, ##args)
#define clk_buf_warn_limit(fmt, args...)	pr_warn_ratelimited(TAG fmt, ##args)
#define clk_buf_dbg(fmt, args...)			\
	do {						\
		if (clkbuf_debug)			\
			pr_warn(TAG fmt, ##args);	\
	} while (0)


/*
 * LOCK
 */
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

#define clkbuf_readl(addr)			__raw_readl(addr)
#define clkbuf_writel(addr, val)	mt_reg_sync_writel(val, addr)

#define PWRAP_REG(ofs)		(pwrap_base + ofs)

#define DCXO_ENABLE		PWRAP_REG(0x18C)
#define DCXO_CONN_ADR0		PWRAP_REG(0x190)
#define DCXO_CONN_WDATA0	PWRAP_REG(0x194)
#define DCXO_CONN_ADR1		PWRAP_REG(0x198)
#define DCXO_CONN_WDATA1	PWRAP_REG(0x19C)
#define DCXO_NFC_ADR0		PWRAP_REG(0x1A0)
#define DCXO_NFC_WDATA0		PWRAP_REG(0x1A4)
#define DCXO_NFC_ADR1		PWRAP_REG(0x1A8)
#define DCXO_NFC_WDATA1		PWRAP_REG(0x1AC)
#define HARB_SLEEP_GATED_CTRL	PWRAP_REG(0x1F0)

#define	DCXO_CONN_ENABLE	(1U << 1)
#define	DCXO_NFC_ENABLE		(1U << 0)
#define HARB_SLEEP_GATED_EN	(1U << 0)

#define	DCXO_EXTBUF_EN_M	0
#define	DCXO_EN_BB		1
#define	DCXO_CLK_SEL		2
#define	DCXO_EN_BB_OR_CLK_SEL	3

#define PMIC_REG_MASK				0xFFFF
#define PMIC_REG_SHIFT				0

#define PMIC_CW00_ADDR				0x7000
#define PMIC_CW00_INIT_VAL			0x4DFD
#define PMIC_CW00_INIT_VAL_MAN		0x4925
/*
 * PMIC_CW00_INIT_VAL_MAN 0x4925: MANUAL pmic clkbuf2,3,4
 * PMIC_CW00_INIT_VAL_MAN 0x4F25: MANUAL pmic clkbuf2,3
*/
#define PMIC_CW00_XO_EXTBUF1_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF1_MODE_SHIFT		0
#define PMIC_CW00_XO_EXTBUF1_EN_M_MASK		0x1
#define PMIC_CW00_XO_EXTBUF1_EN_M_SHIFT		2
#define PMIC_CW00_XO_EXTBUF2_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF2_MODE_SHIFT		3
#define PMIC_CW00_XO_EXTBUF2_EN_M_MASK		0x1
#define PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT		5
#define PMIC_CW00_XO_EXTBUF3_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF3_MODE_SHIFT		6
#define PMIC_CW00_XO_EXTBUF4_MODE_MASK		0x3
#define PMIC_CW00_XO_EXTBUF4_MODE_SHIFT		9
#define PMIC_CW00_XO_EXTBUF4_EN_M_MASK		0x1
#define PMIC_CW00_XO_EXTBUF4_EN_M_SHIFT		11


#define PMIC_CW00_XO_BB_LPM_EN_MASK		0x1
#define PMIC_CW00_XO_BB_LPM_EN_SHIFT		12

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
#define PMIC_CW13_DEFAULT_VAL			0x8AAA
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

/* FIXME: only for bring up Co-TSX before DT is ready */
/* #define CLKBUF_COTSX_BRINGUP */

/* FIXME: Before MP, using suggested driving current to test. */
/* #define TEST_SUGGEST_RF_DRIVING_CURR_BEFORE_MP */
#define TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP

/* disable clkbuf3 in mt6757 because NFC have it's own XTAL */
#define DISABLE_PMIC_CLKBUF3

/*
 * Baseband Low Power Mode (BBLPM) for PMIC clkbuf
 * Condition: flightmode on + conn mtcmos off + NFC clkbuf off + enter deepidle
 * Caller: deep idle
 */
void clk_buf_control_bblpm(bool on)
{

}


/* for spm driver use */
bool is_clk_buf_under_flightmode(void)
{
	return false;
}

/* for ccci driver to notify this */
void clk_buf_set_by_flightmode(bool is_flightmode_on)
{

}

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{
	return true;
}

void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status)
{

}

/*
 * Let caller get driving current setting of RF clock buffer
 * Caller: ccci & ccci will send it to modem
 */
void clk_buf_get_rf_drv_curr(void *rf_drv_curr)
{

}

/* Called by ccci driver to keep afcdac value sent from modem */
void clk_buf_save_afc_val(unsigned int afcdac)
{

}

/* Called by suspend driver to write afcdac into SPM register */
void clk_buf_write_afcdac(void)
{

}

bool is_clk_buf_from_pmic(void)
{
	bool ret = true;

	return ret;

};


bool clk_buf_init(void)
{
	return true;
}


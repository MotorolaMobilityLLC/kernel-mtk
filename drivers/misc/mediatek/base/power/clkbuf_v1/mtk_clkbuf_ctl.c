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
#include <mtk_spm.h>

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

/* PMICWRAP Reg */
#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)
#define DCXO_ENABLE		PWRAP_REG(0x18C)
#define DCXO_CONN_ADR0		PWRAP_REG(0x190)
#define DCXO_CONN_WDATA0	PWRAP_REG(0x194)
#define DCXO_CONN_ADR1		PWRAP_REG(0x198)
#define DCXO_CONN_WDATA1	PWRAP_REG(0x19C)
#define DCXO_NFC_ADR0		PWRAP_REG(0x1A0)
#define DCXO_NFC_WDATA0		PWRAP_REG(0x1A4)
#define DCXO_NFC_ADR1		PWRAP_REG(0x1A8)
#define DCXO_NFC_WDATA1		PWRAP_REG(0x1AC)
#elif defined(CONFIG_MACH_ELBRUS)
#define DCXO_ENABLE		PWRAP_REG(0x188)
#define DCXO_CONN_ADR0		PWRAP_REG(0x18C)
#define DCXO_CONN_WDATA0	PWRAP_REG(0x190)
#define DCXO_CONN_ADR1		PWRAP_REG(0x194)
#define DCXO_CONN_WDATA1	PWRAP_REG(0x198)
#define DCXO_NFC_ADR0		PWRAP_REG(0x19C)
#define DCXO_NFC_WDATA0		PWRAP_REG(0x1A0)
#define DCXO_NFC_ADR1		PWRAP_REG(0x1A4)
#define DCXO_NFC_WDATA1		PWRAP_REG(0x1A8)
#else
#error NO corresponding project can be found!!!
#endif

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
#define PMIC_DCXO_CW00		MT6355_DCXO_CW00
#define PMIC_DCXO_CW00_SET	MT6355_DCXO_CW00_SET
#define PMIC_DCXO_CW00_CLR	MT6355_DCXO_CW00_CLR
#define PMIC_DCXO_CW01		MT6355_DCXO_CW01
#define PMIC_DCXO_CW02		MT6355_DCXO_CW02
#define PMIC_DCXO_CW03		MT6355_DCXO_CW03
#define PMIC_DCXO_CW04		MT6355_DCXO_CW04
#define PMIC_DCXO_CW05		MT6355_DCXO_CW05
#define PMIC_DCXO_CW06		MT6355_DCXO_CW06
#define PMIC_DCXO_CW07		MT6355_DCXO_CW07
#define PMIC_DCXO_CW08		MT6355_DCXO_CW08
#define PMIC_DCXO_CW09		MT6355_DCXO_CW09
#define PMIC_DCXO_CW10		MT6355_DCXO_CW10
#define PMIC_DCXO_CW11		MT6355_DCXO_CW11
#define PMIC_DCXO_CW11_SET	MT6355_DCXO_CW11_SET
#define PMIC_DCXO_CW11_CLR	MT6355_DCXO_CW11_CLR
#define PMIC_DCXO_CW12		MT6355_DCXO_CW12
#define PMIC_DCXO_CW13		MT6355_DCXO_CW13
#define PMIC_DCXO_CW14		MT6355_DCXO_CW14
#define PMIC_DCXO_CW15		MT6355_DCXO_CW15
#define PMIC_DCXO_CW16		MT6355_DCXO_CW16
#define PMIC_DCXO_CW17		MT6355_DCXO_CW17
#define PMIC_DCXO_CW18		MT6355_DCXO_CW18
#define PMIC_DCXO_CW19		MT6355_DCXO_CW19
#else /* for MT6335 */
#define PMIC_DCXO_CW00		MT6335_DCXO_CW00
#define PMIC_DCXO_CW00_SET	MT6335_DCXO_CW00_SET
#define PMIC_DCXO_CW00_CLR	MT6335_DCXO_CW00_CLR
#define PMIC_DCXO_CW01		MT6335_DCXO_CW01
#define PMIC_DCXO_CW02		MT6335_DCXO_CW02
#define PMIC_DCXO_CW03		MT6335_DCXO_CW03
#define PMIC_DCXO_CW04		MT6335_DCXO_CW04
#define PMIC_DCXO_CW05		MT6335_DCXO_CW05
#define PMIC_DCXO_CW06		MT6335_DCXO_CW06
#define PMIC_DCXO_CW07		MT6335_DCXO_CW07
#define PMIC_DCXO_CW08		MT6335_DCXO_CW08
#define PMIC_DCXO_CW09		MT6335_DCXO_CW09
#define PMIC_DCXO_CW10		MT6335_DCXO_CW10
#define PMIC_DCXO_CW11		MT6335_DCXO_CW11
#define PMIC_DCXO_CW11_SET	MT6335_DCXO_CW11_SET
#define PMIC_DCXO_CW11_CLR	MT6335_DCXO_CW11_CLR
#define PMIC_DCXO_CW12		MT6335_DCXO_CW12
#define PMIC_DCXO_CW13		MT6335_DCXO_CW13
#define PMIC_DCXO_CW14		MT6335_DCXO_CW14
#define PMIC_DCXO_CW15		MT6335_DCXO_CW15
#define PMIC_DCXO_CW16		MT6335_DCXO_CW16
#define PMIC_DCXO_CW17		MT6335_DCXO_CW17
#define PMIC_DCXO_CW18		MT6335_DCXO_CW18
#define PMIC_DCXO_CW19		MT6335_DCXO_CW19
#endif

#define	DCXO_CONN_ENABLE	(0x1 << 1)
#define	DCXO_NFC_ENABLE		(0x1 << 0)

#define PMIC_REG_MASK				0xFFFF
#define PMIC_REG_SHIFT				0

#define PMIC_CW00_INIT_VAL			0x4FFD
#define PMIC_CW11_INIT_VAL			0xA400

/* TODO: marked this after driver is ready */
#if defined(CONFIG_MACH_MT6759)
#define CLKBUF_BRINGUP
#endif

/* Debug only */
/* #define CLKBUF_TWAM_DEBUG */

static bool is_clkbuf_initiated;

/* false: rf_clkbuf, true: pmic_clkbuf */
static bool is_pmic_clkbuf = true;

/* FIXME: Before MP, using suggested driving current to test. */
/* #define TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP */

static bool g_is_flightmode_on;
static bool clkbuf_debug;
static unsigned int bblpm_cnt;

#ifdef CLKBUF_TWAM_DEBUG
struct delayed_work clkbuf_twam_dbg_work;
static bool clkbuf_twam_dbg_flag;
#endif

static unsigned int clkbuf_ctrl_stat;
#define CLKBUF_CTRL_STAT_MASK ((0x1 << CLK_BUF_BB_MD) | (0x1 << CLK_BUF_CONN) | \
			       (0x1 << CLK_BUF_NFC) | (0x1 << CLK_BUF_RF) | \
			       (0x1 << CLK_BUF_AUDIO) | (0x1 << CLK_BUF_CHG) | \
			       (0x1 << CLK_BUF_UFS))
#define CLKBUF_CTRL_STAT_XO_PD_MASK ((0x1 << CLK_BUF_AUDIO) | (0x1 << CLK_BUF_CHG))

static unsigned int pwrap_dcxo_en_flag = (DCXO_CONN_ENABLE | DCXO_NFC_ENABLE);

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

static int8_t clkbuf_drv_curr_auxout[CLKBUF_NUM];

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

static void pmic_clk_buf_ctrl_nfc(short on)
{
	if (on)
		pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF3_EN_M_MASK,
				      PMIC_XO_EXTBUF3_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF3_EN_M_MASK,
				      PMIC_XO_EXTBUF3_EN_M_SHIFT);
}

static void pmic_clk_buf_ctrl_aud(short on)
{
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
#else /* for MT6335 */
	if (on)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF5_EN_M_MASK,
				      PMIC_XO_EXTBUF5_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF5_EN_M_MASK,
				      PMIC_XO_EXTBUF5_EN_M_SHIFT);
#endif
}

static void pmic_clk_buf_ctrl_pd(short on)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (on)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF6_EN_M_MASK,
				      PMIC_XO_EXTBUF6_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF6_EN_M_MASK,
				      PMIC_XO_EXTBUF6_EN_M_SHIFT);
#endif
}

static void pmic_clk_buf_ctrl_ext(short on)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
	if (on)
		pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
				      PMIC_XO_EXTBUF7_EN_M_MASK,
				      PMIC_XO_EXTBUF7_EN_M_SHIFT);
	else
		pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
				      PMIC_XO_EXTBUF7_EN_M_MASK,
				      PMIC_XO_EXTBUF7_EN_M_SHIFT);
#endif
}

static void pmic_clk_buf_ctrl_multi(void)
{
	clk_buf_dbg("%s: clkbuf_ctrl_stat=0x%x\n", __func__, clkbuf_ctrl_stat);
	if (!!(clkbuf_ctrl_stat & CLKBUF_CTRL_STAT_XO_PD_MASK)) {
		if (pmic_clk_buf_swctrl[XO_PD] == 0) {
			pmic_clk_buf_ctrl_pd(1);
			pmic_clk_buf_swctrl[XO_PD] = 1;
		}
	} else {
		if (pmic_clk_buf_swctrl[XO_PD] == 1) {
			pmic_clk_buf_ctrl_pd(0);
			pmic_clk_buf_swctrl[XO_PD] = 0;
		}
	}
}

static void pmic_clk_buf_ctrl(CLK_BUF_SWCTRL_STATUS_T *status)
{
	u32 pmic_cw00 = 0, pmic_cw11 = 0;

	if (!is_clkbuf_initiated)
		return;

	pmic_clk_buf_ctrl_wcn(status[XO_WCN] % 2);
	pmic_clk_buf_ctrl_nfc(status[XO_NFC] % 2);
	pmic_clk_buf_ctrl_aud(status[XO_AUD] % 2);
	pmic_clk_buf_ctrl_pd(status[XO_PD] % 2);
	pmic_clk_buf_ctrl_ext(status[XO_EXT] % 2);

	pmic_read_interface(PMIC_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW11=0x%x, clk_buf_swctrl=[%u %u %u %u %u %u %u]\n",
		     __func__, pmic_cw00, pmic_cw11, status[0], status[1],
		     status[2], status[3], status[4], status[5], status[6]);
}

void clk_buf_control_bblpm(bool on)
{
#ifdef CLKBUF_USE_BBLPM
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	u32 cw00 = 0;

	if (!is_clkbuf_initiated || !is_pmic_clkbuf)
		return;

	if (on) /* FPM -> BBLPM */
		pmic_config_interface_nolock(PMIC_DCXO_CW00_SET_ADDR, 0x1,
				      PMIC_XO_BB_LPM_EN_MASK,
				      PMIC_XO_BB_LPM_EN_SHIFT);
	else /* BBLPM -> FPM */
		pmic_config_interface_nolock(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
				      PMIC_XO_BB_LPM_EN_MASK,
				      PMIC_XO_BB_LPM_EN_SHIFT);

	pmic_read_interface_nolock(PMIC_DCXO_CW00, &cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);

	clk_buf_dbg("%s(%u): CW00=0x%x\n", __func__, (on ? 1 : 0), cw00);
#endif
#endif
}

/*
 * Baseband Low Power Mode (BBLPM) for PMIC clkbuf
 * Condition: XO_CELL/XO_NFC/XO_WCN/XO_EXT OFF
 * Caller: deep idle, SODI2.5
 * Return: 0 if all conditions are matched & ready to enter BBLPM
 */
u32 clk_buf_bblpm_enter_cond(void)
{
	u32 bblpm_cond = 0;

#ifdef CLKBUF_USE_BBLPM
	if (!is_clkbuf_initiated || !is_pmic_clkbuf) {
		bblpm_cond |= BBLPM_COND_SKIP;
		return bblpm_cond;
	}

	if ((clkbuf_readl(MD1_PWR_CON) & 0x4) || (clkbuf_readl(C2K_PWR_CON) & 0x4))
		bblpm_cond |= BBLPM_COND_CEL;

	if (pmic_clk_buf_swctrl[XO_NFC] == CLK_BUF_SW_ENABLE)
		bblpm_cond |= BBLPM_COND_NFC;

#if 0 /* Check by caller in SSPM */
#ifdef CONFIG_MTK_UFS_BOOTING
	if (ufs_mtk_deepidle_hibern8_check())
		bblpm_cond |= BBLPM_COND_EXT;
#endif
#endif

	if (!bblpm_cond)
		bblpm_cnt++;
#else /* !CLKBUF_USE_BBLPM */
	bblpm_cond |= BBLPM_COND_SKIP;
#endif
	return bblpm_cond;
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

static void clk_buf_ctrl_internal(enum clk_buf_id id, bool onoff)
{
	if (!is_pmic_clkbuf)
		return;

	mutex_lock(&clk_buf_ctrl_lock);

	switch (id) {
	case CLK_BUF_CONN:
		if (onoff) {
			pmic_config_interface(PMIC_DCXO_CW00_SET, 0x3,
					      PMIC_XO_EXTBUF2_MODE_MASK,
					      PMIC_XO_EXTBUF2_MODE_SHIFT);
			pmic_clk_buf_ctrl_wcn(1);
			pmic_clk_buf_swctrl[XO_WCN] = 1;

			pwrap_dcxo_en_flag |= DCXO_CONN_ENABLE;
			clkbuf_writel(DCXO_ENABLE, pwrap_dcxo_en_flag);
		} else {
			pwrap_dcxo_en_flag &= ~DCXO_CONN_ENABLE;
			clkbuf_writel(DCXO_ENABLE, pwrap_dcxo_en_flag);

			pmic_config_interface(PMIC_DCXO_CW00_CLR, 0x3,
					      PMIC_XO_EXTBUF2_MODE_MASK,
					      PMIC_XO_EXTBUF2_MODE_SHIFT);
			pmic_clk_buf_ctrl_wcn(0);
			pmic_clk_buf_swctrl[XO_WCN] = 0;
		}
		clk_buf_warn("%s: id=%d, onoff=%d, DCXO_ENABLE=0x%x, pwrap_dcxo_en_flag=0x%x\n",
			     __func__, id, onoff, clkbuf_readl(DCXO_ENABLE),
			     pwrap_dcxo_en_flag);
		break;
	case CLK_BUF_NFC:
		if (onoff) {
			pmic_config_interface(PMIC_DCXO_CW00_SET, 0x3,
					      PMIC_XO_EXTBUF3_MODE_MASK,
					      PMIC_XO_EXTBUF3_MODE_SHIFT);
			pmic_clk_buf_ctrl_nfc(1);
			pmic_clk_buf_swctrl[XO_NFC] = 1;

			pwrap_dcxo_en_flag |= DCXO_NFC_ENABLE;
			clkbuf_writel(DCXO_ENABLE, pwrap_dcxo_en_flag);
		} else {
			pwrap_dcxo_en_flag &= ~DCXO_NFC_ENABLE;
			clkbuf_writel(DCXO_ENABLE, pwrap_dcxo_en_flag);

			pmic_config_interface(PMIC_DCXO_CW00_CLR, 0x3,
					      PMIC_XO_EXTBUF3_MODE_MASK,
					      PMIC_XO_EXTBUF3_MODE_SHIFT);
			pmic_clk_buf_ctrl_nfc(0);
			pmic_clk_buf_swctrl[XO_NFC] = 0;
		}
		clk_buf_warn("%s: id=%d, onoff=%d, DCXO_ENABLE=0x%x, pwrap_dcxo_en_flag=0x%x\n",
			     __func__, id, onoff, clkbuf_readl(DCXO_ENABLE),
			     pwrap_dcxo_en_flag);
		break;
	case CLK_BUF_UFS:
		if (onoff) {
			pmic_clk_buf_ctrl_ext(1);
			pmic_clk_buf_swctrl[XO_EXT] = 1;
		} else {
			pmic_clk_buf_ctrl_ext(0);
			pmic_clk_buf_swctrl[XO_EXT] = 0;
		}
		clk_buf_warn("%s: id=%d, onoff=%d\n", __func__, id, onoff);
		break;
	default:
		clk_buf_err("%s: id=%d isn't supported\n", __func__, id);
		break;
	}

	mutex_unlock(&clk_buf_ctrl_lock);
}

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{
	short ret = 0, no_lock = 0;

	if (!is_clkbuf_initiated)
		return false;

	if (!is_pmic_clkbuf)
		return false;

	clk_buf_dbg("%s: id=%d, onoff=%d, clkbuf_ctrl_stat=0x%x\n", __func__,
		    id, onoff, clkbuf_ctrl_stat);

	if (preempt_count() > 0 || irqs_disabled() || system_state != SYSTEM_RUNNING || oops_in_progress)
		no_lock = 1;

	if (!no_lock)
		mutex_lock(&clk_buf_ctrl_lock);

	switch (id) {
	case CLK_BUF_BB_MD:
		if (CLK_BUF1_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		break;
	case CLK_BUF_CONN:
		if (CLK_BUF2_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		if (!(pwrap_dcxo_en_flag & DCXO_CONN_ENABLE)) {
			ret = -1;
			clk_buf_err("%s: id=%d skip due to non co-clock for CONN\n", __func__, id);
			pmic_clk_buf_ctrl_wcn(0);
			pmic_clk_buf_swctrl[XO_WCN] = 0;
			break;
		}
		/* record the status of CONN from caller for checking BBLPM */
		pmic_clk_buf_swctrl[XO_WCN] = onoff;
		break;
	case CLK_BUF_NFC:
		if (CLK_BUF3_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		/* record the status of NFC from caller for checking BBLPM */
		pmic_clk_buf_swctrl[XO_NFC] = onoff;
		break;
	case CLK_BUF_RF:
		if (CLK_BUF4_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		break;
	case CLK_BUF_AUDIO:
		if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		if (onoff)
			clkbuf_ctrl_stat |= (0x1 << CLK_BUF_AUDIO);
		else
			clkbuf_ctrl_stat &= ~(0x1 << CLK_BUF_AUDIO);

		pmic_clk_buf_ctrl_multi();
		break;
	case CLK_BUF_CHG:
		if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		if (onoff)
			clkbuf_ctrl_stat |= (0x1 << CLK_BUF_CHG);
		else
			clkbuf_ctrl_stat &= ~(0x1 << CLK_BUF_CHG);

		pmic_clk_buf_ctrl_multi();
		break;
	case CLK_BUF_UFS:
		if (CLK_BUF7_STATUS_PMIC != CLOCK_BUFFER_SW_CONTROL) {
			ret = -1;
			clk_buf_err("%s: id=%d isn't controlled by SW\n", __func__, id);
			break;
		}
		pmic_clk_buf_ctrl_ext(onoff);
		pmic_clk_buf_swctrl[XO_EXT] = onoff;
		break;
	default:
		ret = -1;
		clk_buf_err("%s: id=%d isn't supported\n", __func__, id);
		break;
	}

	if (!no_lock)
		mutex_unlock(&clk_buf_ctrl_lock);

	if (ret)
		return false;
	else
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

#ifdef CLKBUF_TWAM_DEBUG
static void clkbuf_twam_dbg_worker(struct work_struct *work)
{
	while (clkbuf_twam_dbg_flag) {
		if ((spm_read(SPM_IRQ_STA) & (0x4)) == 0x4) {
			clk_buf_warn("SPM_TWAM_LAST_STA0/1/2/3=0x%x/0x%x/0x%x/0x%x\n",
				     spm_read(SPM_TWAM_LAST_STA0),
				     spm_read(SPM_TWAM_LAST_STA1),
				     spm_read(SPM_TWAM_LAST_STA2),
				     spm_read(SPM_TWAM_LAST_STA3));
			clk_buf_warn("SPM_TWAM_CURR_STA0/1/2/3=0x%x/0x%x/0x%x/0x%x\n",
				     spm_read(SPM_TWAM_CURR_STA0),
				     spm_read(SPM_TWAM_CURR_STA1),
				     spm_read(SPM_TWAM_CURR_STA2),
				     spm_read(SPM_TWAM_CURR_STA3));

			mutex_lock(&clk_buf_ctrl_lock);
			spm_write(SPM_IRQ_STA, (spm_read(SPM_IRQ_STA) & ~(0x4)) | (0x1 << 2));
			mutex_unlock(&clk_buf_ctrl_lock);
		}
	}
}
#endif

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

static void clk_buf_calc_drv_curr_auxout(void)
{
	u32 pmic_cw19 = 0;

	pmic_config_interface(PMIC_DCXO_CW18, 42,
			      PMIC_XO_STATIC_AUXOUT_SEL_MASK,
			      PMIC_XO_STATIC_AUXOUT_SEL_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW19, &pmic_cw19,
			    PMIC_XO_STATIC_AUXOUT_MASK,
			    PMIC_XO_STATIC_AUXOUT_SHIFT);
	clk_buf_dbg("%s: pmic_cw19=0x%x, flag_swbufldok=%u\n", __func__,
		    pmic_cw19, ((pmic_cw19 & (0x3 << 2)) >> 2));
	if (((pmic_cw19 & (0x3 << 2)) >> 2) != 3)
		return;

	pmic_config_interface(PMIC_DCXO_CW18, 5,
			      PMIC_XO_STATIC_AUXOUT_SEL_MASK,
			      PMIC_XO_STATIC_AUXOUT_SEL_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW19, &pmic_cw19,
			    PMIC_XO_STATIC_AUXOUT_MASK,
			    PMIC_XO_STATIC_AUXOUT_SHIFT);
	clk_buf_dbg("%s: sel io_dbg4: pmic_cw19=0x%x\n", __func__, pmic_cw19);
	clkbuf_drv_curr_auxout[XO_SOC] = (pmic_cw19 & (0x3 << 1)) >> 1;
	clkbuf_drv_curr_auxout[XO_WCN] = (pmic_cw19 & (0x3 << 7)) >> 7;

	pmic_config_interface(PMIC_DCXO_CW18, 6,
			      PMIC_XO_STATIC_AUXOUT_SEL_MASK,
			      PMIC_XO_STATIC_AUXOUT_SEL_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW19, &pmic_cw19,
			    PMIC_XO_STATIC_AUXOUT_MASK,
			    PMIC_XO_STATIC_AUXOUT_SHIFT);
	clk_buf_dbg("%s: sel io_dbg5: pmic_cw19=0x%x\n", __func__, pmic_cw19);
	clkbuf_drv_curr_auxout[XO_NFC] = (pmic_cw19 & (0x3 << 1)) >> 1;
	clkbuf_drv_curr_auxout[XO_CEL] = (pmic_cw19 & (0x3 << 7)) >> 7;

	pmic_config_interface(PMIC_DCXO_CW18, 7,
			      PMIC_XO_STATIC_AUXOUT_SEL_MASK,
			      PMIC_XO_STATIC_AUXOUT_SEL_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW19, &pmic_cw19,
			    PMIC_XO_STATIC_AUXOUT_MASK,
			    PMIC_XO_STATIC_AUXOUT_SHIFT);
	clk_buf_dbg("%s: sel io_dbg6: pmic_cw19=0x%x\n", __func__, pmic_cw19);
	clkbuf_drv_curr_auxout[XO_AUD] = (pmic_cw19 & (0x3 << 1)) >> 1;
	clkbuf_drv_curr_auxout[XO_PD] = (pmic_cw19 & (0x3 << 7)) >> 7;
	clkbuf_drv_curr_auxout[XO_EXT] = (pmic_cw19 & (0x3 << 12)) >> 12;

	clk_buf_warn("%s: PMIC_CLK_BUF?_DRV_CURR_AUXOUT=%d %d %d %d %d %d %d\n", __func__,
		     clkbuf_drv_curr_auxout[XO_SOC],
		     clkbuf_drv_curr_auxout[XO_WCN],
		     clkbuf_drv_curr_auxout[XO_NFC],
		     clkbuf_drv_curr_auxout[XO_CEL],
		     clkbuf_drv_curr_auxout[XO_AUD],
		     clkbuf_drv_curr_auxout[XO_PD],
		     clkbuf_drv_curr_auxout[XO_EXT]);
}

#ifdef CONFIG_PM
static ssize_t clk_buf_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	u32 clk_buf_en[CLKBUF_NUM], i;
	char cmd[32];

	if (sscanf(buf, "%31s %x %x %x %x %x %x %x", cmd, &clk_buf_en[XO_SOC],
		   &clk_buf_en[XO_WCN], &clk_buf_en[XO_NFC], &clk_buf_en[XO_CEL],
		   &clk_buf_en[XO_AUD], &clk_buf_en[XO_PD], &clk_buf_en[XO_EXT])
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

	if (!strcmp(cmd, "pwrap")) {
		if (!is_pmic_clkbuf)
			return -EINVAL;

		mutex_lock(&clk_buf_ctrl_lock);

		for (i = 0; i < CLKBUF_NUM; i++) {
			if (i == XO_WCN) {
				if (clk_buf_en[i])
					pwrap_dcxo_en_flag |= DCXO_CONN_ENABLE;
				else
					pwrap_dcxo_en_flag &= ~DCXO_CONN_ENABLE;
			} else if (i == XO_NFC) {
				if (clk_buf_en[i])
					pwrap_dcxo_en_flag |= DCXO_NFC_ENABLE;
				else
					pwrap_dcxo_en_flag &= ~DCXO_NFC_ENABLE;
			}
		}

		clkbuf_writel(DCXO_ENABLE, pwrap_dcxo_en_flag);
		clk_buf_warn("%s: DCXO_ENABLE=0x%x, pwrap_dcxo_en_flag=0x%x\n", __func__,
			     clkbuf_readl(DCXO_ENABLE), pwrap_dcxo_en_flag);

		mutex_unlock(&clk_buf_ctrl_lock);

		return count;
	}

	return -EINVAL;
}

static ssize_t clk_buf_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	int len = 0;
	u32 pmic_cw00 = 0, pmic_cw02 = 0, pmic_cw11 = 0, pmic_cw14 = 0,
	    pmic_cw16 = 0;

	clk_buf_calc_drv_curr_auxout();

	len += snprintf(buf+len, PAGE_SIZE-len,
			"********** PMIC clock buffer state (%s) **********\n",
			(is_pmic_clkbuf ? "on" : "off"));
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_SOC   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF1_STATUS_PMIC, pmic_clk_buf_swctrl[XO_SOC]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_WCN   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF2_STATUS_PMIC, pmic_clk_buf_swctrl[XO_WCN]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_NFC   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF3_STATUS_PMIC, pmic_clk_buf_swctrl[XO_NFC]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_CEL   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF4_STATUS_PMIC, pmic_clk_buf_swctrl[XO_CEL]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_AUD   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF5_STATUS_PMIC, pmic_clk_buf_swctrl[XO_AUD]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_PD    SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF6_STATUS_PMIC, pmic_clk_buf_swctrl[XO_PD]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"XO_EXT   SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
			CLK_BUF7_STATUS_PMIC, pmic_clk_buf_swctrl[XO_EXT]);
	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n********** clock buffer command help **********\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"PMIC switch on/off: echo pmic en1 en2 en3 en4 en5 en6 en7 > /sys/power/clk_buf/clk_buf_ctrl\n");
	len += snprintf(buf+len, PAGE_SIZE-len,
			"\n********** clock buffer debug info **********\n");
	len += snprintf(buf+len, PAGE_SIZE-len, "pmic_drv_curr_vals=%d %d %d %d %d %d %d\n",
			PMIC_CLK_BUF1_DRIVING_CURR, PMIC_CLK_BUF2_DRIVING_CURR,
			PMIC_CLK_BUF3_DRIVING_CURR, PMIC_CLK_BUF4_DRIVING_CURR,
			PMIC_CLK_BUF5_DRIVING_CURR, PMIC_CLK_BUF6_DRIVING_CURR,
			PMIC_CLK_BUF7_DRIVING_CURR);
	len += snprintf(buf+len, PAGE_SIZE-len, "clkbuf_drv_curr_auxout=%d %d %d %d %d %d %d\n",
		     clkbuf_drv_curr_auxout[XO_SOC],
		     clkbuf_drv_curr_auxout[XO_WCN],
		     clkbuf_drv_curr_auxout[XO_NFC],
		     clkbuf_drv_curr_auxout[XO_CEL],
		     clkbuf_drv_curr_auxout[XO_AUD],
		     clkbuf_drv_curr_auxout[XO_PD],
		     clkbuf_drv_curr_auxout[XO_EXT]);

	len += snprintf(buf+len, PAGE_SIZE-len, "clkbuf_ctrl_stat=0x%x, pwrap_dcxo_en_flag=0x%x\n",
			clkbuf_ctrl_stat, pwrap_dcxo_en_flag);

	pmic_read_interface_nolock(PMIC_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface_nolock(PMIC_DCXO_CW02, &pmic_cw02,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface_nolock(PMIC_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface_nolock(PMIC_DCXO_CW14, &pmic_cw14,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface_nolock(PMIC_DCXO_CW16, &pmic_cw16,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	len += snprintf(buf+len, PAGE_SIZE-len, "DCXO_CW00/CW02/CW11/CW14/CW16=0x%x/0x%x/0x%x/0x%x/0x%x\n",
			pmic_cw00, pmic_cw02, pmic_cw11, pmic_cw14, pmic_cw16);

	len += snprintf(buf+len, PAGE_SIZE-len, "DCXO_CONN_ADR0/WDATA0/ADR1/WDATA1/EN=0x%x/%x/%x/%x/%x\n",
		     clkbuf_readl(DCXO_CONN_ADR0),
		     clkbuf_readl(DCXO_CONN_WDATA0),
		     clkbuf_readl(DCXO_CONN_ADR1),
		     clkbuf_readl(DCXO_CONN_WDATA1),
		     clkbuf_readl(DCXO_ENABLE));
	len += snprintf(buf+len, PAGE_SIZE-len, "DCXO_NFC_ADR0/WDATA0/ADR1/WDATA1=0x%x/%x/%x/%x\n",
		     clkbuf_readl(DCXO_NFC_ADR0),
		     clkbuf_readl(DCXO_NFC_WDATA0),
		     clkbuf_readl(DCXO_NFC_ADR1),
		     clkbuf_readl(DCXO_NFC_WDATA1));

#if defined(CONFIG_MACH_MT6759)
	len += snprintf(buf+len, PAGE_SIZE-len, "bblpm_cnt=%u\n", bblpm_cnt);
#else
	len += snprintf(buf+len, PAGE_SIZE-len, "bblpm_cnt=%u, MD1/C2K_PWR_CON=0x%x/%x\n",
			bblpm_cnt, clkbuf_readl(MD1_PWR_CON),
			clkbuf_readl(C2K_PWR_CON));
#endif

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
		else if (debug == 2)
			clk_buf_ctrl(CLK_BUF_AUDIO, true);
		else if (debug == 3)
			clk_buf_ctrl(CLK_BUF_AUDIO, false);
		else if (debug == 4)
			clk_buf_ctrl(CLK_BUF_CHG, true);
		else if (debug == 5)
			clk_buf_ctrl(CLK_BUF_CHG, false);
		else if (debug == 6)
			clk_buf_ctrl(CLK_BUF_UFS, true);
		else if (debug == 7)
			clk_buf_ctrl(CLK_BUF_UFS, false);
#ifdef CLKBUF_TWAM_DEBUG
		else if (debug == 8) {
			clk_buf_warn("%s: clkbuf_twam_dbg_work is enabled\n", __func__);
			clkbuf_twam_dbg_flag = true;
			schedule_delayed_work(&clkbuf_twam_dbg_work, 0);
		} else if (debug == 9) {
			clkbuf_twam_dbg_flag = false;
			clk_buf_warn("%s: clkbuf_twam_dbg_work is disabled\n", __func__);
		}
#endif
		else if (debug == 10)
			clk_buf_ctrl_internal(CLK_BUF_CONN, false);
		else if (debug == 11)
			clk_buf_ctrl_internal(CLK_BUF_CONN, true);
		else if (debug == 12)
			clk_buf_ctrl_internal(CLK_BUF_NFC, false);
		else if (debug == 13)
			clk_buf_ctrl_internal(CLK_BUF_NFC, true);
		else if (debug == 14)
			clk_buf_ctrl_internal(CLK_BUF_UFS, false);
		else if (debug == 15)
			clk_buf_ctrl_internal(CLK_BUF_UFS, true);
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

static int clk_buf_fs_init(void)
{
	int r = 0;

	/* create /sys/power/clk_buf/xxx */
	r = sysfs_create_group(power_kobj, &clk_buf_attr_group);
	if (r)
		clk_buf_err("FAILED TO CREATE /sys/power/clk_buf (%d)\n", r);

	return r;
}
#else /* !CONFIG_PM */
static int clk_buf_fs_init(void)
{
	return 0;
}
#endif /* CONFIG_PM */

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

	return 0;
}
#else /* !CONFIG_OF */
static int clk_buf_dts_map(void)
{
	return 0;
}
#endif

bool is_clk_buf_from_pmic(void)
{
	return true;
};

static void clk_buf_pmic_wrap_init(void)
{
	u32 pmic_cw00 = 0, pmic_cw02 = 0, pmic_cw11 = 0, pmic_cw14 = 0,
	    pmic_cw16 = 0;

	/* Dump registers before setting */
	pmic_read_interface(PMIC_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW02, &pmic_cw02,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW14, &pmic_cw14,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW16, &pmic_cw16,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW02=0x%x, CW11=0x%x, CW14=0x%x, CW16=0x%x\n",
		     __func__, pmic_cw00, pmic_cw02, pmic_cw11, pmic_cw14, pmic_cw16);

#ifndef __KERNEL__
	/* Setup initial PMIC clock buffer setting */
	pmic_config_interface(PMIC_DCXO_CW02, 0,
			    PMIC_XO_BUFLDOK_EN_MASK, PMIC_XO_BUFLDOK_EN_SHIFT);
	udelay(100);
	pmic_config_interface(PMIC_DCXO_CW02, 1,
			    PMIC_XO_BUFLDOK_EN_MASK, PMIC_XO_BUFLDOK_EN_SHIFT);
	mdelay(1);
	pmic_config_interface(PMIC_DCXO_CW00, PMIC_CW00_INIT_VAL,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(PMIC_DCXO_CW11, PMIC_CW11_INIT_VAL,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(PMIC_DCXO_CW14, 0x4,
			    PMIC_RG_XO_RESERVED0_MASK, PMIC_RG_XO_RESERVED0_SHIFT);

	/* Check if the setting is ok */
	pmic_read_interface(PMIC_DCXO_CW00, &pmic_cw00,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW02, &pmic_cw02,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW11, &pmic_cw11,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW14, &pmic_cw14,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_DCXO_CW16, &pmic_cw16,
			    PMIC_REG_MASK, PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW02=0x%x, CW11=0x%x, CW14=0x%x, CW16=0x%x\n",
		     __func__, pmic_cw00, pmic_cw02, pmic_cw11, pmic_cw14, pmic_cw16);
#endif /* #ifndef __KERNEL__ */

#ifdef __KERNEL__
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
#endif /* #ifdef __KERNEL__ */

	clk_buf_calc_drv_curr_auxout();

#ifdef CLKBUF_TWAM_DEBUG
	spm_write(SPM_TWAM_WINDOW_LEN, 0x5690);
	spm_write(SPM_TWAM_IDLE_SEL, 0x0);
	clk_buf_warn("%s: SPM_TWAM_CON=0x%x\n", __func__, spm_read(SPM_TWAM_CON));
	spm_write(SPM_TWAM_CON, (spm_read(SPM_TWAM_CON) & ~(0x1F000)) | (0x4 << 12));
	spm_write(SPM_TWAM_CON, (spm_read(SPM_TWAM_CON) & ~(0x30)) | (0x2 << 4));
	spm_write(SPM_TWAM_CON, (spm_read(SPM_TWAM_CON) & ~(0x1)) | (0x1 << 0));
	clk_buf_warn("%s: SPM_TWAM_CON=0x%x\n", __func__, spm_read(SPM_TWAM_CON));
#endif
}

int clk_buf_init(void)
{
#ifdef CLKBUF_BRINGUP
	clk_buf_warn("%s: is skipped for bring-up\n", __func__);

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

#ifdef CLKBUF_TWAM_DEBUG
	INIT_DELAYED_WORK(&clkbuf_twam_dbg_work, clkbuf_twam_dbg_worker);
#endif

#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT6759)
	/* no need to use XO_CONN in this platform */
	clk_buf_ctrl_internal(CLK_BUF_CONN, false);
#ifndef CONFIG_MTK_NFC_SUPPORT
	/* no need to use XO_NFC if no NFC chip */
	clk_buf_ctrl_internal(CLK_BUF_NFC, false);
#endif
#ifndef CONFIG_MTK_UFS_BOOTING
	/* no need to use XO_EXT if storage is emmc */
	clk_buf_ctrl_internal(CLK_BUF_UFS, false);
#endif
#endif

	is_clkbuf_initiated = true;

	return 0;
}
late_initcall(clk_buf_init);


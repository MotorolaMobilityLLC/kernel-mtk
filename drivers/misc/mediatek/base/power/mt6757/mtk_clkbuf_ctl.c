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
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/ratelimit.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>

#include <mt-plat/sync_write.h>
#include <mtk_spm.h>
/* #include <mach/mt_gpio_core.h> */
#include <mt-plat/upmu_common.h>
#include <mtk_clkbuf_ctl.h>

#define TAG "[Power/clkbuf]"

#define clk_buf_err(fmt, args...) pr_debug(TAG fmt, ##args)
#define clk_buf_warn(fmt, args...) pr_debug(TAG fmt, ##args)
#define clk_buf_warn_limit(fmt, args...) pr_debug(TAG fmt, ##args)
#define clk_buf_dbg(fmt, args...)					       \
	do {								   \
	if (clkbuf_debug)					       \
	    pr_debug(TAG fmt, ##args);				   \
	} while (0)

/*
 * LOCK
 */
DEFINE_MUTEX(clk_buf_ctrl_lock);

#define DEFINE_ATTR_RO(_name)						\
	static struct kobj_attribute _name##_attr = {			\
	.attr =								\
	{							       \
	    .name = #_name, .mode = 0444,			       \
	},							       \
	.show = _name##_show,						\
	}

#define DEFINE_ATTR_RW(_name)						       \
	static struct kobj_attribute _name##_attr = {			   \
	.attr =								   \
	{							       \
	    .name = #_name, .mode = 0644,			       \
	},							       \
	.show = _name##_show,						   \
	.store = _name##_store,						   \
	}

#define __ATTR_OF(_name) (&_name##_attr.attr)

#define clkbuf_readl(addr) __raw_readl(addr)
#define clkbuf_writel(addr, val) mt_reg_sync_writel(val, addr)

static void __iomem *pwrap_base;

#define PWRAP_REG(ofs) (pwrap_base + ofs)

#define DCXO_ENABLE PWRAP_REG(0x18C)
#define DCXO_CONN_ADR0 PWRAP_REG(0x190)
#define DCXO_CONN_WDATA0 PWRAP_REG(0x194)
#define DCXO_CONN_ADR1 PWRAP_REG(0x198)
#define DCXO_CONN_WDATA1 PWRAP_REG(0x19C)
#define DCXO_NFC_ADR0 PWRAP_REG(0x1A0)
#define DCXO_NFC_WDATA0 PWRAP_REG(0x1A4)
#define DCXO_NFC_ADR1 PWRAP_REG(0x1A8)
#define DCXO_NFC_WDATA1 PWRAP_REG(0x1AC)
#define HARB_SLEEP_GATED_CTRL PWRAP_REG(0x1F0)

#define DCXO_CONN_ENABLE (1U << 1)
#define DCXO_NFC_ENABLE (1U << 0)
#define HARB_SLEEP_GATED_EN (1U << 0)

#define DCXO_EXTBUF_EN_M 0
#define DCXO_EN_BB 1
#define DCXO_CLK_SEL 2
#define DCXO_EN_BB_OR_CLK_SEL 3

#define PMIC_REG_MASK 0xFFFF
#define PMIC_REG_SHIFT 0

#define PMIC_CW00_ADDR 0x7000
#define PMIC_CW00_INIT_VAL 0x4F3D
#define PMIC_CW00_INIT_VAL_MAN 0x4925
/*
 * PMIC_CW00_INIT_VAL_MAN 0x4925: MANUAL pmic clkbuf2,3,4
 * PMIC_CW00_INIT_VAL_MAN 0x4F25: MANUAL pmic clkbuf2,3
 */
#define PMIC_CW00_XO_EXTBUF1_MODE_MASK 0x3
#define PMIC_CW00_XO_EXTBUF1_MODE_SHIFT 0
#define PMIC_CW00_XO_EXTBUF1_EN_M_MASK 0x1
#define PMIC_CW00_XO_EXTBUF1_EN_M_SHIFT 2
#define PMIC_CW00_XO_EXTBUF2_MODE_MASK 0x3
#define PMIC_CW00_XO_EXTBUF2_MODE_SHIFT 3
#define PMIC_CW00_XO_EXTBUF2_EN_M_MASK 0x1
#define PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT 5
#define PMIC_CW00_XO_EXTBUF3_MODE_MASK 0x3
#define PMIC_CW00_XO_EXTBUF3_MODE_SHIFT 6
#define PMIC_CW00_XO_EXTBUF4_MODE_MASK 0x3
#define PMIC_CW00_XO_EXTBUF4_MODE_SHIFT 9
#define PMIC_CW00_XO_EXTBUF4_EN_M_MASK 0x1
#define PMIC_CW00_XO_EXTBUF4_EN_M_SHIFT 11

#define PMIC_CW00_XO_BB_LPM_EN_MASK 0x1
#define PMIC_CW00_XO_BB_LPM_EN_SHIFT 12

/*
 * 0x701A   CW13    Code Word 13
 * 15:14    RG_XO_EXTBUF4_ISET	XO Control Signal of Current on EXTBUF4
 * 13:12    RG_XO_EXTBUF4_HD	XO Control Signal of EXTBUF4 Output driving
 * Strength
 * 11:10    RG_XO_EXTBUF3_ISET	XO Control Signal of Current on EXTBUF3
 * 9:8	    RG_XO_EXTBUF3_HD	XO Control Signal of EXTBUF3 Output driving
 * Strength
 * 7:6	    RG_XO_EXTBUF2_ISET	XO Control Signal of Current on EXTBUF2
 * 5:4	    RG_XO_EXTBUF2_HD	XO Control Signal of EXTBUF2 Output driving
 * Strength
 * 3:2	    RG_XO_EXTBUF1_ISET	XO Control Signal of Current on EXTBUF1
 * 1:0	    RG_XO_EXTBUF1_HD	XO Control Signal of EXTBUF1 Output driving
 * Strength
 */
#define PMIC_CW13_ADDR 0x701A
#define PMIC_CW13_SUGGEST_VAL 0x4666
#define PMIC_CW13_DEFAULT_VAL 0x8AAA
#define PMIC_CW13_XO_EXTBUF_HD_VAL					\
	((0x2 << 0) | (0x2 << 4) | (0x2 << 8) | (0 << 12))

#define PMIC_CW14_ADDR 0x701C
#define PMIC_CW14_XO_EXTBUF3_EN_M_MASK 0x1
#define PMIC_CW14_XO_EXTBUF3_EN_M_SHIFT 11
#define PMIC_CW14_XO_RG_XO_RESERVED0_MASK 0xF
#define PMIC_CW14_XO_RG_XO_RESERVED0_SHIFT 6

#define PMIC_CW15_ADDR 0x701E
#define PMIC_CW15_DCXO_STATIC_AUXOUT_EN_MASK 0x1
#define PMIC_CW15_DCXO_STATIC_AUXOUT_EN_SHIFT 0
#define PMIC_CW15_DCXO_STATIC_AUXOUT_SEL_MASK 0xF
#define PMIC_CW15_DCXO_STATIC_AUXOUT_SEL_SHIFT 1

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
/* dynamic enable pmic clkbuf2 for CONNSYS co-tsx/tcxo co-load request */
#define DYNAMIC_ENABLE_PMIC_CLKBU2

/*CW00=0x4DFD is from mt6335*/
/*#define MT6355_PMIC_CW00_INIT_VAL	0x4DFD from mt6335 */
/*#define MT6355_PMIC_CW00_INIT_VAL	0x4F35	XO3_mode=00,
 * XO2_mode=10
 */
#define MT6355_PMIC_CW00_INIT_VAL 0x4FFD     /*XO3_mode=11, XO2_mode=11*/
#define MT6355_PMIC_CW00_INIT_VAL_OFF 0x4000 /*XO234_mode=00, XO1_mode=00*/
#define MT6355_PMIC_CW00_INIT_VAL_MAN 0x4925 /*XO234_mode=00, XO1_mode=01*/
/*#define MT6355_PMIC_CW02_INIT_VAL 0xB86A porting from Elbrus, need this!?
 */
#define MT6355_PMIC_CW11_INIT_VAL 0x8000 /*XO7_mode=00, XO6_mode=00*/
#define MT6355_PMIC_CW16_SUGGEST_VAL 0x9455

#endif /* CONFIG_MTK_PMIC_CHIP_MT6355 */

/* #define CLKBUF_BRINGUP */

static bool is_clkbuf_initiated;
static bool g_is_flightmode_on;
static bool clkbuf_debug;

struct delayed_work clkbuf_delayed_work;

/* false: rf_clkbuf, true: pmic_clkbuf */
static bool is_pmic_clkbuf = true;

static unsigned int afcdac_val = 0x2A80008; /* afc_default=4096 */
static bool is_clkbuf_afcdac_updated;
static unsigned int bblpm_cnt;

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
static unsigned int g_pmic_cw00_rg_val = MT6355_PMIC_CW00_INIT_VAL;
static unsigned int g_pmic_cw16_rg_val = MT6355_PMIC_CW16_SUGGEST_VAL;
#else
static unsigned int g_pmic_cw00_rg_val = PMIC_CW00_INIT_VAL;
static unsigned int g_pmic_cw13_rg_val = PMIC_CW13_DEFAULT_VAL;
#endif

/* FIXME: Before MP, using suggested driving current to test. */
/* #define TEST_SUGGEST_RF_DRIVING_CURR_BEFORE_MP */
#define TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP

/* disable clkbuf3 in mt6757 because NFC have it's own XTAL */
#define DISABLE_PMIC_CLKBUF3

#if !defined(CONFIG_MTK_LEGACY)
static unsigned int CLK_BUF1_STATUS, CLK_BUF2_STATUS, CLK_BUF3_STATUS,
	CLK_BUF4_STATUS, CLK_BUF5_STATUS_PMIC = CLOCK_BUFFER_HW_CONTROL,
	     CLK_BUF6_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL,
	     CLK_BUF7_STATUS_PMIC = CLOCK_BUFFER_SW_CONTROL,
	     CLK_BUF8_STATUS_PMIC = CLOCK_BUFFER_HW_CONTROL;
static int RF_CLK_BUF1_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   RF_CLK_BUF2_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   RF_CLK_BUF3_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   RF_CLK_BUF4_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;
static int PMIC_CLK_BUF5_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   PMIC_CLK_BUF6_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   PMIC_CLK_BUF7_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   PMIC_CLK_BUF8_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
static int PMIC_CLK_BUF9_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   PMIC_CLK_BUF10_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2,
	   PMIC_CLK_BUF11_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;
#endif /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */
#else /* CONFIG_MTK_LEGACY */
/* FIXME: can be removed after DCT tool gen is ready */
#if !defined(RF_CLK_BUF1_DRIVING_CURR)
#define RF_CLK_BUF1_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define RF_CLK_BUF2_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define RF_CLK_BUF3_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define RF_CLK_BUF4_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#endif
#if !defined(PMIC_CLK_BUF5_DRIVING_CURR)
#define PMIC_CLK_BUF5_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF6_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF7_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF8_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF9_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF10_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#define PMIC_CLK_BUF11_DRIVING_CURR CLK_BUF_DRIVING_CURR_2
#endif

#endif /* CONFIG_MTK_LEGACY */

#ifndef CLKBUF_BRINGUP
static enum CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_DISABLE};

static enum CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl_modem_on[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE};
#else /* For Bring-up */
static enum CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE};

static enum CLK_BUF_SWCTRL_STATUS_T clk_buf_swctrl_modem_on[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE};
#endif

static enum CLK_BUF_SWCTRL_STATUS_T pmic_clk_buf_swctrl[CLKBUF_NUM] = {
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE,
	CLK_BUF_SW_ENABLE, CLK_BUF_SW_ENABLE
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	,
	CLK_BUF_SW_DISABLE, CLK_BUF_SW_DISABLE, CLK_BUF_SW_DISABLE
#endif

};

static void spm_clk_buf_ctrl(enum CLK_BUF_SWCTRL_STATUS_T *status)
{
	u32 spm_val;
	int i;

	spm_val = spm_read(SPM_MDBSI_CON) & ~0x7;

	for (i = 1; i < RF_CLKBUF_NUM; i++)
	spm_val |= status[i] << (i - 1);

	spm_write(SPM_MDBSI_CON, spm_val);

	udelay(2);
}

static void pmic_clk_buf_ctrl(enum CLK_BUF_SWCTRL_STATUS_T *status)
{
#if !defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	u32 conn_conf = 0, nfc_conf = 0;
#endif

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	if (status[PMIC_CLK_BUF_BB_MD] >= 2) {
	pmic_config_interface(MT6355_DCXO_CW00, g_pmic_cw00_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(MT6355_DCXO_CW11,
		      MT6355_PMIC_CW11_INIT_VAL, PMIC_REG_MASK,
		      PMIC_REG_SHIFT);
	} else {
	pmic_config_interface(MT6355_DCXO_CW00,
		      MT6355_PMIC_CW00_INIT_VAL_MAN,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(MT6355_DCXO_CW11,
		      MT6355_PMIC_CW11_INIT_VAL, PMIC_REG_MASK,
		      PMIC_REG_SHIFT);

	g_pmic_cw16_rg_val = MT6355_PMIC_CW16_SUGGEST_VAL;
	pmic_config_interface(MT6355_DCXO_CW16, g_pmic_cw16_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);

	if (status[PMIC_CLK_BUF_CONN] % 2)
	    pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
			  PMIC_XO_EXTBUF2_EN_M_MASK,
			  PMIC_XO_EXTBUF2_EN_M_SHIFT);
	else
	    pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
			  PMIC_XO_EXTBUF2_EN_M_MASK,
			  PMIC_XO_EXTBUF2_EN_M_SHIFT);

	if (status[PMIC_CLK_BUF_NFC] % 2)
	    pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
			  PMIC_XO_EXTBUF3_EN_M_MASK,
			  PMIC_XO_EXTBUF3_EN_M_SHIFT);
	else
	    pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
			  PMIC_XO_EXTBUF3_EN_M_MASK,
			  PMIC_XO_EXTBUF3_EN_M_SHIFT);

	if (status[PMIC_CLK_BUF_RF] % 2)
	    pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR, 0x1,
			  PMIC_XO_EXTBUF4_EN_M_MASK,
			  PMIC_XO_EXTBUF4_EN_M_SHIFT);
	else
	    pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
			  PMIC_XO_EXTBUF4_EN_M_MASK,
			  PMIC_XO_EXTBUF4_EN_M_SHIFT);
#if 0
	pmic_config_interface(MT6355_DCXO_CW11, MT6355_DCXO_CW00_SUGGEST,
	    PMIC_REG_MASK, PMIC_REG_SHIFT);

	if (status[PMIC_CLK_BUF_AUD] % 2)
	    pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
			PMIC_XO_EXTBUF6_EN_M_MASK,
			PMIC_XO_EXTBUF6_EN_M_SHIFT);
	else
	    pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
			PMIC_XO_EXTBUF6_EN_M_MASK,
			PMIC_XO_EXTBUF6_EN_M_SHIFT);

	if (status[PMIC_CLK_BUF_EXT] % 2)
	    pmic_config_interface(PMIC_DCXO_CW11_SET_ADDR, 0x1,
			PMIC_XO_EXTBUF4_EN_M_MASK,
			PMIC_XO_EXTBUF4_EN_M_SHIFT);
	else
	    pmic_config_interface(PMIC_DCXO_CW11_CLR_ADDR, 0x1,
			PMIC_XO_EXTBUF4_EN_M_MASK,
			PMIC_XO_EXTBUF4_EN_M_SHIFT);
#endif
	}
#else  /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */

	if (status[PMIC_CLK_BUF_BB_MD] >= 2) {
	pmic_config_interface(PMIC_CW00_ADDR, g_pmic_cw00_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);
	} else {
	pmic_config_interface(PMIC_CW00_ADDR, PMIC_CW00_INIT_VAL_MAN,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);
	pmic_config_interface(PMIC_CW00_ADDR,
		      (status[PMIC_CLK_BUF_CONN] % 2),
		      PMIC_CW00_XO_EXTBUF2_EN_M_MASK,
		      PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT);
	pmic_config_interface(PMIC_CW14_ADDR,
		      (status[PMIC_CLK_BUF_NFC] % 2),
		      PMIC_CW14_XO_EXTBUF3_EN_M_MASK,
		      PMIC_CW14_XO_EXTBUF3_EN_M_SHIFT);
	pmic_config_interface(PMIC_CW00_ADDR,
		      (status[PMIC_CLK_BUF_RF] % 2),
		      PMIC_CW00_XO_EXTBUF4_EN_M_MASK,
		      PMIC_CW00_XO_EXTBUF4_EN_M_SHIFT);
	}

	pmic_read_interface(PMIC_CW00_ADDR, &conn_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW14_ADDR, &nfc_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	clk_buf_warn(
	"%s: CW00=0x%x, CW14=0x%x, clkbuf2=%u, clkbuf3=%u, clkbuf4=%u\n",
	__func__, conn_conf, nfc_conf, status[PMIC_CLK_BUF_CONN],
	status[PMIC_CLK_BUF_NFC], status[PMIC_CLK_BUF_RF]);
#endif /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */
}

/*
 * Baseband Low Power Mode (BBLPM) for PMIC clkbuf
 * Condition: flightmode on + conn mtcmos off + NFC clkbuf off + enter deepidle
 * Caller: deep idle
 */
void clk_buf_control_bblpm(bool on)
{
	u32 cw00 = 0;

	if (!is_pmic_clkbuf)
	return;

#if !defined(DISABLE_PMIC_CLKBUF3)
	if (pmic_clk_buf_swctrl[PMIC_CLK_BUF_NFC] == CLK_BUF_SW_ENABLE)
	return;
#endif

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	if (on) {
	pmic_config_interface_nolock(PMIC_DCXO_CW00_SET_ADDR, 0x1,
			 PMIC_XO_BB_LPM_EN_MASK,
			 PMIC_XO_BB_LPM_EN_SHIFT);
	} else {
	pmic_config_interface_nolock(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
			 PMIC_XO_BB_LPM_EN_MASK,
			 PMIC_XO_BB_LPM_EN_SHIFT);
	}

	pmic_read_interface_nolock(MT6355_DCXO_CW00, &cw00, PMIC_REG_MASK,
		   PMIC_REG_SHIFT);

#else

	pmic_config_interface_nolock(PMIC_CW00_ADDR, (on ? 1 : 0),
		     PMIC_CW00_XO_BB_LPM_EN_MASK,
		     PMIC_CW00_XO_BB_LPM_EN_SHIFT);

	pmic_read_interface_nolock(PMIC_CW00_ADDR, &cw00, PMIC_REG_MASK,
		   PMIC_REG_SHIFT);

#endif
	bblpm_cnt++;

	clk_buf_dbg("%s(%u): CW00=0x%x\n", __func__, (on ? 1 : 0), cw00);
}

static void clkbuf_delayed_worker(struct work_struct *work)
{
	bool srcclkena_o1 = false;

	srcclkena_o1 = !!(spm_read(PCM_REG13_DATA) & R13_MD1_VRF18_REQ);
	clk_buf_warn(
	"%s: g_is_flightmode_on=%d, srcclkena_o1=%u, pcm_reg13=0x%x\n",
	__func__, g_is_flightmode_on, srcclkena_o1,
	spm_read(PCM_REG13_DATA));

	WARN_ON(clkbuf_debug && g_is_flightmode_on && srcclkena_o1);

	mutex_lock(&clk_buf_ctrl_lock);

	if (g_is_flightmode_on)
	spm_clk_buf_ctrl(clk_buf_swctrl);

	mutex_unlock(&clk_buf_ctrl_lock);
}

/* for spm driver use */
bool is_clk_buf_under_flightmode(void) { return g_is_flightmode_on; }

/* for ccci driver to notify this */
void clk_buf_set_by_flightmode(bool is_flightmode_on)
{
	bool srcclkena_o1 = false;

	srcclkena_o1 = !!(spm_read(PCM_REG13_DATA) & R13_MD1_VRF18_REQ);
	clk_buf_warn(
	"%s: g/is_flightmode_on=%d->%d, srcclkena_o1=%u, pcm_reg13=0x%x\n",
	__func__, g_is_flightmode_on, is_flightmode_on, srcclkena_o1,
	spm_read(PCM_REG13_DATA));

	mutex_lock(&clk_buf_ctrl_lock);

	if (g_is_flightmode_on == is_flightmode_on) {
	mutex_unlock(&clk_buf_ctrl_lock);
	return;
	}

	g_is_flightmode_on = is_flightmode_on;

	if (!is_pmic_clkbuf) {
	if (g_is_flightmode_on) {
	    schedule_delayed_work(&clkbuf_delayed_work,
			  msecs_to_jiffies(2000));
	    /* spm_clk_buf_ctrl(clk_buf_swctrl); */
	} else {
	    cancel_delayed_work(&clkbuf_delayed_work);
	    spm_clk_buf_ctrl(clk_buf_swctrl_modem_on);
	}
	}

	mutex_unlock(&clk_buf_ctrl_lock);
}

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff)
{

	if (is_pmic_clkbuf) {
	mutex_lock(&clk_buf_ctrl_lock);

	/* record the status of NFC from caller for checking BBLPM */
	if (id == CLK_BUF_NFC)
	    pmic_clk_buf_swctrl[id] = onoff;

	mutex_unlock(&clk_buf_ctrl_lock);

	clk_buf_dbg("%s: id=%d, onoff=%d, is_flightmode_on=%d\n",
		__func__, id, onoff, g_is_flightmode_on);

	return false;
	}

	if (id >= CLK_BUF_INVALID)
	  /* TODO: need check DCT tool for CLK BUF SW control */
	return false;

	if ((id == CLK_BUF_BB_MD) &&
	(CLK_BUF1_STATUS == CLOCK_BUFFER_HW_CONTROL ||
	 CLK_BUF1_STATUS == CLOCK_BUFFER_DISABLE))
	return false;

	if ((id == CLK_BUF_CONN) &&
	(CLK_BUF2_STATUS == CLOCK_BUFFER_HW_CONTROL ||
	 CLK_BUF2_STATUS == CLOCK_BUFFER_DISABLE))
	return false;

	if ((id == CLK_BUF_NFC) &&
	(CLK_BUF3_STATUS == CLOCK_BUFFER_HW_CONTROL ||
	 CLK_BUF3_STATUS == CLOCK_BUFFER_DISABLE))
	return false;

	if ((id == CLK_BUF_AUDIO) &&
	(CLK_BUF4_STATUS == CLOCK_BUFFER_HW_CONTROL ||
	 CLK_BUF4_STATUS == CLOCK_BUFFER_DISABLE))
	return false;

#if 0
	/* for bring-up */
	clk_buf_warn("%s is disabled for bring-up\n", __func__);
	return false;
#endif
	clk_buf_dbg("%s: id=%d, onoff=%d, is_flightmode_on=%d\n", __func__, id,
	    onoff, g_is_flightmode_on);

	mutex_lock(&clk_buf_ctrl_lock);

	clk_buf_swctrl[id] = onoff;

	if (g_is_flightmode_on)
	spm_clk_buf_ctrl(clk_buf_swctrl);

	mutex_unlock(&clk_buf_ctrl_lock);

	return true;
}
EXPORT_SYMBOL(clk_buf_ctrl);

void clk_buf_get_swctrl_status(enum CLK_BUF_SWCTRL_STATUS_T *status)
{
	int i;

	clk_buf_warn(
	"%s: is_flightmode_on=%d, swctrl:clkbuf3=%d/%d, clkbuf4=%d/%d\n",
	__func__, g_is_flightmode_on, clk_buf_swctrl[2],
	clk_buf_swctrl_modem_on[2], clk_buf_swctrl[3],
	clk_buf_swctrl_modem_on[3]);

	for (i = 0; i < RF_CLKBUF_NUM; i++) {
	if (g_is_flightmode_on)
	    status[i] = clk_buf_swctrl[i];
	else
	    status[i] = clk_buf_swctrl_modem_on[i];
	}
}

/*
 * Let caller get driving current setting of RF clock buffer
 * Caller: ccci & ccci will send it to modem
 */
void clk_buf_get_rf_drv_curr(void *rf_drv_curr)
{

#ifdef TEST_SUGGEST_RF_DRIVING_CURR_BEFORE_MP
	RF_CLK_BUF1_DRIVING_CURR = CLK_BUF_DRIVING_CURR_0_9MA,
	RF_CLK_BUF2_DRIVING_CURR = CLK_BUF_DRIVING_CURR_0_9MA,
	RF_CLK_BUF3_DRIVING_CURR = CLK_BUF_DRIVING_CURR_0_9MA,
	RF_CLK_BUF4_DRIVING_CURR = CLK_BUF_DRIVING_CURR_0_9MA;
#endif

	((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[0] =
	RF_CLK_BUF1_DRIVING_CURR;
	((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[1] =
	RF_CLK_BUF2_DRIVING_CURR;
	((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[2] =
	RF_CLK_BUF3_DRIVING_CURR;
	((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[3] =
	RF_CLK_BUF4_DRIVING_CURR;

	clk_buf_warn("%s: rf_drv_curr_vals=%d %d %d %d\n", __func__,
	     ((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[0],
	     ((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[1],
	     ((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[2],
	     ((enum MTK_CLK_BUF_DRIVING_CURR *)rf_drv_curr)[3]);
}

/* Called by ccci driver to keep afcdac value sent from modem */
void clk_buf_save_afc_val(unsigned int afcdac)
{

	if (is_pmic_clkbuf)
	return;

	afcdac_val = afcdac;

	if (!is_clkbuf_afcdac_updated) {
	spm_write(SPM_BSI_EN_SR, afcdac_val);
	is_clkbuf_afcdac_updated = true;
	}

	clk_buf_dbg("%s: afcdac=0x%x, SPM_BSI_EN_SR=0x%x\n", __func__,
	    afcdac_val, spm_read(SPM_BSI_EN_SR));
}

/* Called by suspend driver to write afcdac into SPM register */
void clk_buf_write_afcdac(void)
{
	if (is_pmic_clkbuf)
	return;

	spm_write(SPM_BSI_EN_SR, afcdac_val);
	clk_buf_dbg("%s: afcdac=0x%x, SPM_BSI_EN_SR=0x%x, afcdac_updated=%d\n",
	    __func__, afcdac_val, spm_read(SPM_BSI_EN_SR),
	    is_clkbuf_afcdac_updated);
}

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
void clk_buf_init_connsys_clkbuf(bool onoff)
{
	u32 pmic_cw00;

	if (!is_pmic_clkbuf)
	return;

#if defined(DYNAMIC_ENABLE_PMIC_CLKBU2) && !defined(CLKBUF_BRINGUP)
	if (onoff == 0) {
	pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x3,
		      PMIC_XO_EXTBUF2_MODE_MASK,
		      PMIC_XO_EXTBUF2_MODE_SHIFT);

	pmic_config_interface(PMIC_DCXO_CW00_CLR_ADDR, 0x1,
		      PMIC_XO_EXTBUF2_EN_M_MASK,
		      PMIC_XO_EXTBUF2_EN_M_SHIFT);

	clkbuf_writel(DCXO_CONN_ADR0, 0);
	clkbuf_writel(DCXO_CONN_WDATA0, 0);
	clkbuf_writel(DCXO_CONN_ADR1, 0);
	clkbuf_writel(DCXO_CONN_WDATA1, 0);

	clkbuf_writel(DCXO_ENABLE,
		  clkbuf_readl(DCXO_ENABLE) & ~DCXO_CONN_ENABLE);

	} else {
#if 0
	/* restore connsys clkbuf */
	pmic_config_interface(PMIC_DCXO_CW00_SET_ADDR,
		(MT6355_PMIC_CW00_INIT_VAL >> PMIC_XO_EXTBUF2_MODE_SHIFT)
						& PMIC_XO_EXTBUF2_MODE_MASK,
		PMIC_XO_EXTBUF2_MODE_MASK,
		PMIC_XO_EXTBUF2_MODE_SHIFT);

	clkbuf_writel(DCXO_CONN_ADR0, MT6355_DCXO_CW00_CLR);
	clkbuf_writel(DCXO_CONN_WDATA0,
		PMIC_XO_EXTBUF2_EN_M_MASK << PMIC_XO_EXTBUF2_EN_M_SHIFT);
	clkbuf_writel(DCXO_CONN_ADR1, MT6355_DCXO_CW00_SET);
	clkbuf_writel(DCXO_CONN_WDATA1,
		PMIC_XO_EXTBUF2_EN_M_MASK << PMIC_XO_EXTBUF2_EN_M_SHIFT);

	clkbuf_writel(DCXO_ENABLE, clkbuf_readl(DCXO_ENABLE) | DCXO_CONN_ENABLE);
#endif
	}

	pmic_read_interface(MT6355_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK,
		PMIC_REG_SHIFT);

	clk_buf_warn("%s: on=%d CW00=0x%x DCXO_CONN_ADR0/WDATA0/ADR1/WDATA1/EN= 0x%x/%x/%x/%x/%x\n",
	     __func__, onoff, pmic_cw00, clkbuf_readl(DCXO_CONN_ADR0),
	     clkbuf_readl(DCXO_CONN_WDATA0),
	     clkbuf_readl(DCXO_CONN_ADR1),
	     clkbuf_readl(DCXO_CONN_WDATA1), clkbuf_readl(DCXO_ENABLE));
#endif /* !defined(DYNAMIC_ENABLE_PMIC_CLKBU2)	&& !defined(CLKBUF_BRINGUP)*/
}
#endif /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */

static ssize_t clk_buf_ctrl_store(struct kobject *kobj,
		  struct kobj_attribute *attr, const char *buf,
		  size_t count)
{
	/* design for BSI or PMIC wrapper command */
	u32 clk_buf_en[CLKBUF_NUM], i;
	char cmd[32];

	/*TODO : if PMIC6355 use CLKBUF6/7, it must be fixed */
	if (sscanf(buf, "%31s %x %x %x %x", cmd, &clk_buf_en[0], &clk_buf_en[1],
	   &clk_buf_en[2], &clk_buf_en[3]) != 5)
	return -EPERM;

	if (!strcmp(cmd, "bsi")) {
	if (is_pmic_clkbuf)
		return -EINVAL;

	mutex_lock(&clk_buf_ctrl_lock);

	for (i = 0; i < RF_CLKBUF_NUM; i++)
	    clk_buf_swctrl[i] = clk_buf_en[i];

	if (g_is_flightmode_on)
	    spm_clk_buf_ctrl(clk_buf_swctrl);

	mutex_unlock(&clk_buf_ctrl_lock);

	return count;
	}

	if (!strcmp(cmd, "pmic")) {
	if (!is_pmic_clkbuf)
	    return -EINVAL;

	for (i = 0; i < CLKBUF_NUM; i++)
	    pmic_clk_buf_swctrl[i] = clk_buf_en[i];

	pmic_clk_buf_ctrl(pmic_clk_buf_swctrl);

	return count;
	}

	return -EINVAL;
}

static ssize_t clk_buf_ctrl_show(struct kobject *kobj,
		 struct kobj_attribute *attr, char *buf)
{
	int len = 0;
	bool srcclkena_o1 = false;
	int buf2_status = -1, buf3_status = -1, buf4_status = -1;
	u32 buf1_mode, buf2_mode, buf3_mode, buf4_mode, buf2_en_m, buf3_en_m,
	buf4_en_m, buf24_en;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	u32 pmic_cw00, pmic_cw11, pmic_cw12, pmic_cw14, pmic_cw16;
	u32 buf6_mode, buf7_mode, buf6_en_m, buf7_en_m;
	int buf6_status = -1, buf7_status = -1;
	u32 xo_extbuf2_clksel_man, xo_extbuf4_clksel_man;
#else  /* defined (CONFIG_MTK_PMIC_CHIP_MT6355) */
	u32 pmic_cw00, pmic_cw13, pmic_cw14;
	u32 rg_xo_reserved0_0;
#endif /* defined (CONFIG_MTK_PMIC_CHIP_MT6355) */

	len += snprintf(buf + len, PAGE_SIZE - len,
	    "********** RF clock buffer state (%s) "
	    "flightmode(FM)=%d **********\n",
	    (is_pmic_clkbuf ? "off" : "on"), g_is_flightmode_on);
	len += snprintf(
	buf + len, PAGE_SIZE - len, "CKBUF1_BB	 SW(1)/HW(2) CTL: %d, Dis(0)/En(1) of FM=1:%d, FM=0:%d\n",
	CLK_BUF1_STATUS, clk_buf_swctrl[0], clk_buf_swctrl_modem_on[0]);
	len += snprintf(
	buf + len, PAGE_SIZE - len, "CKBUF2_NONE SW(1)/HW(2) CTL: %d, Dis(0)/En(1) of FM=1:%d, FM=0:%d\n",
	CLK_BUF2_STATUS, clk_buf_swctrl[1], clk_buf_swctrl_modem_on[1]);
	len += snprintf(
	buf + len, PAGE_SIZE - len, "CKBUF3_NFC	 SW(1)/HW(2) CTL: %d,Dis(0)/En(1) of FM=1:%d, FM=0:%d\n",
	CLK_BUF3_STATUS, clk_buf_swctrl[2], clk_buf_swctrl_modem_on[2]);
	len += snprintf(
	buf + len, PAGE_SIZE - len, "CKBUF4_AUD	 SW(1)/HW(2) CTL: %d, Dis(0)/En(1) of FM=1:%d, FM=0:%d\n",
	CLK_BUF4_STATUS, clk_buf_swctrl[3], clk_buf_swctrl_modem_on[3]);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "********** PMIC clock buffer state (%s) **********\n",
	    (is_pmic_clkbuf ? "on" : "off"));
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "CKBUF1_BB	 SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
	    CLK_BUF5_STATUS_PMIC, pmic_clk_buf_swctrl[0]);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "CKBUF2_CONN SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
	    CLK_BUF6_STATUS_PMIC, pmic_clk_buf_swctrl[1]);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "CKBUF3_NFC	 SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
	    CLK_BUF7_STATUS_PMIC, pmic_clk_buf_swctrl[2]);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "CKBUF4_RF	 SW(1)/HW(2) CTL: %d, Dis(0)/En(1): %d\n",
	    CLK_BUF8_STATUS_PMIC, pmic_clk_buf_swctrl[3]);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "\n********** clock buffer command help **********\n");
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "BSI  switch on/off: echo bsi en1 en2 en3 en4 > /sys/power/clk_buf/clk_buf_ctrl\n");
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "PMIC switch on/off: echo pmic en1 en2 en3 en4 > /sys/power/clk_buf/clk_buf_ctrl\n");
	if (is_pmic_clkbuf) {

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	len += snprintf(buf + len, PAGE_SIZE - len,
		"g_pmic_cw16_rg_val=0x%x, bblpm_cnt=%u\n",
		g_pmic_cw16_rg_val, bblpm_cnt);

	srcclkena_o1 = !!(spm_read(PCM_REG13_DATA) & R13_MD1_VRF18_REQ);

	pmic_read_interface(MT6355_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW11, &pmic_cw11, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW12, &pmic_cw12, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW14, &pmic_cw14, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW16, &pmic_cw16, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);

	buf1_mode = (pmic_cw00 >> PMIC_XO_EXTBUF1_MODE_SHIFT) &
		PMIC_XO_EXTBUF1_MODE_MASK;
	buf2_mode = (pmic_cw00 >> PMIC_XO_EXTBUF2_MODE_SHIFT) &
		PMIC_XO_EXTBUF2_MODE_MASK;
	buf3_mode = (pmic_cw00 >> PMIC_XO_EXTBUF3_MODE_SHIFT) &
		PMIC_XO_EXTBUF3_MODE_MASK;
	buf4_mode = (pmic_cw00 >> PMIC_XO_EXTBUF4_MODE_SHIFT) &
		PMIC_XO_EXTBUF4_MODE_MASK;
	buf6_mode = (pmic_cw11 >> PMIC_XO_EXTBUF6_MODE_SHIFT) &
		PMIC_XO_EXTBUF6_MODE_MASK;
	buf7_mode = (pmic_cw11 >> PMIC_XO_EXTBUF7_MODE_SHIFT) &
		PMIC_XO_EXTBUF7_MODE_MASK;

	buf2_en_m = (pmic_cw00 >> PMIC_XO_EXTBUF2_EN_M_SHIFT) &
		PMIC_XO_EXTBUF2_EN_M_MASK;
	buf3_en_m = (pmic_cw00 >> PMIC_XO_EXTBUF3_EN_M_SHIFT) &
		PMIC_XO_EXTBUF3_EN_M_MASK;
	buf4_en_m = (pmic_cw00 >> PMIC_XO_EXTBUF4_EN_M_SHIFT) &
		PMIC_XO_EXTBUF4_EN_M_MASK;
	buf6_en_m = (pmic_cw11 >> PMIC_XO_EXTBUF6_EN_M_SHIFT) &
		PMIC_XO_EXTBUF6_EN_M_MASK;
	buf7_en_m = (pmic_cw11 >> PMIC_XO_EXTBUF7_EN_M_SHIFT) &
		PMIC_XO_EXTBUF7_EN_M_MASK;

	xo_extbuf4_clksel_man =
	    (pmic_cw12 >> PMIC_XO_EXTBUF4_CLKSEL_MAN_SHIFT) &
	    PMIC_XO_EXTBUF4_CLKSEL_MAN_MASK;
	xo_extbuf2_clksel_man =
	    (pmic_cw14 >> PMIC_XO_EXTBUF2_CLKSEL_MAN_SHIFT) &
	    PMIC_XO_EXTBUF2_CLKSEL_MAN_MASK;

	buf24_en = ((srcclkena_o1 & ~(xo_extbuf4_clksel_man)) ||
		(buf4_en_m & xo_extbuf4_clksel_man)) ||
	       ((srcclkena_o1 & ~(xo_extbuf2_clksel_man)) ||
		(buf2_en_m & xo_extbuf2_clksel_man));
	/* TODO: fix "srcclkena_o1" to srclken_conn */

	if (buf2_mode == 0x3)
	    buf2_status = buf24_en;
	else if (buf2_mode == 0x0)
	    buf2_status = buf2_en_m;

	if (buf3_mode == 0x00)
	    buf3_status = buf3_en_m;

	if (buf4_mode == 0x3)
	    buf4_status = buf24_en;
	else if (buf4_mode == 0x2)
	    buf4_status = srcclkena_o1;
	else if (buf4_mode == 0x00)
	    buf4_status = buf4_en_m;

	if (buf6_mode == 0x0)
	    buf6_status = buf6_en_m;

	if (buf7_mode == 0x0)
	    buf7_status = buf7_en_m;

	len += snprintf(buf + len, PAGE_SIZE - len,
		"buf2/3/4 mode = 0x%x/0x%x/0x%x, buf2/3/4 status =%d/%d/%d\n",
		buf2_mode, buf3_mode, buf4_mode, buf2_status,
		buf3_status, buf4_status);

#else  /* defined (CONFIG_MTK_PMIC_CHIP_MT6355) */
	len += snprintf(buf + len, PAGE_SIZE - len,
		"g_pmic_cw13_rg_val=0x%x, bblpm_cnt=%u\n",
		g_pmic_cw13_rg_val, bblpm_cnt);

	srcclkena_o1 = !!(spm_read(PCM_REG13_DATA) & R13_MD1_VRF18_REQ);

	pmic_read_interface(PMIC_CW00_ADDR, &pmic_cw00, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW13_ADDR, &pmic_cw13, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW14_ADDR, &pmic_cw14, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);

	rg_xo_reserved0_0 =
	    (pmic_cw14 >> PMIC_CW14_XO_RG_XO_RESERVED0_SHIFT) & 0x1;

	buf1_mode = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF1_MODE_SHIFT) &
		PMIC_CW00_XO_EXTBUF1_MODE_MASK;
	buf2_mode = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF2_MODE_SHIFT) &
		PMIC_CW00_XO_EXTBUF2_MODE_MASK;
	buf3_mode = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF3_MODE_SHIFT) &
		PMIC_CW00_XO_EXTBUF3_MODE_MASK;
	buf4_mode = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF4_MODE_SHIFT) &
		PMIC_CW00_XO_EXTBUF4_MODE_MASK;

	buf2_en_m = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT) &
		PMIC_CW00_XO_EXTBUF2_EN_M_MASK;
	buf3_en_m = (pmic_cw14 >> PMIC_CW14_XO_EXTBUF3_EN_M_SHIFT) &
		PMIC_CW14_XO_EXTBUF3_EN_M_MASK;
	buf4_en_m = (pmic_cw00 >> PMIC_CW00_XO_EXTBUF4_EN_M_SHIFT) &
		PMIC_CW00_XO_EXTBUF4_EN_M_MASK;

	buf24_en = ((srcclkena_o1 & ~(rg_xo_reserved0_0)) ||
		(buf4_en_m & rg_xo_reserved0_0)) ||
	       buf2_en_m;

	if (buf2_mode == 0x3)
	    buf2_status = buf24_en;
	else if (buf2_mode == 0x0)
	    buf2_status = buf2_en_m;

	if (buf3_mode == 0x00)
	    buf3_status = buf3_en_m;

	if (buf4_mode == 0x3)
	    buf4_status = buf24_en;
	else if (buf4_mode == 0x2)
	    buf4_status = srcclkena_o1;
	else if (buf4_mode == 0x00)
	    buf4_status = buf4_en_m;

	len += snprintf(buf + len, PAGE_SIZE - len,
		"buf2/3/4 mode = 0x%x/0x%x/0x%x, buf2/3/4 status =%d/%d/%d\n",
		buf2_mode, buf3_mode, buf4_mode, buf2_status,
		buf3_status, buf4_status);
	/*
	 * p += sprintf(p, "rg_xo_reserved0_0=%d, ,buf2_en_m=%d,
	 * buf3_en_m=%d, buf4_en_m=%d\n",
	 *     rg_xo_reserved0_0, buf2_en_m, buf3_en_m, buf4_en_m);
	 */
	len += snprintf(buf + len, PAGE_SIZE - len,
		"buf24_en=%d srcclkena_o1=%d\n", buf24_en,
		srcclkena_o1);
#endif /* defined (CONFIG_MTK_PMIC_CHIP_MT6355) */

	} else
	len += snprintf(buf + len, PAGE_SIZE - len,
		"afcdac=0x%x, is_afcdac_updated=%d\n",
		afcdac_val, is_clkbuf_afcdac_updated);

	len += snprintf(
	buf + len, PAGE_SIZE - len,
	"rf_drv_curr_vals=%d %d %d %d, pmic_drv_curr_vals=%d %d %d %d\n",
	RF_CLK_BUF1_DRIVING_CURR, RF_CLK_BUF2_DRIVING_CURR,
	RF_CLK_BUF3_DRIVING_CURR, RF_CLK_BUF4_DRIVING_CURR,
	PMIC_CLK_BUF5_DRIVING_CURR, PMIC_CLK_BUF6_DRIVING_CURR,
	PMIC_CLK_BUF7_DRIVING_CURR, PMIC_CLK_BUF8_DRIVING_CURR);
	srcclkena_o1 = !!(spm_read(PCM_REG13_DATA) & R13_MD1_VRF18_REQ);
	len += snprintf(buf + len, PAGE_SIZE - len,
	    "srcclkena_o1=%u, pcm_reg13=0x%x, MD1_PWR_CON=0x%x, C2K_PWR_CON=0x%x\n",
	    srcclkena_o1, spm_read(PCM_REG13_DATA),
	    spm_read(MD1_PWR_CON), spm_read(C2K_PWR_CON));

	return len;
}

static ssize_t clk_buf_debug_store(struct kobject *kobj,
		   struct kobj_attribute *attr, const char *buf,
		   size_t count)
{
	int debug = 0;

	if (!kstrtoint(buf, 10, &debug)) {
	if (debug == 0)
	    clkbuf_debug = false;
	else if (debug == 1)
	    clkbuf_debug = true;
	else
	    clk_buf_warn("bad argument!! should be 0 or 1 [0: "
		     "disable, 1: enable]\n");
	} else
	return -EPERM;

	return count;
}

static ssize_t clk_buf_debug_show(struct kobject *kobj,
		  struct kobj_attribute *attr, char *buf)
{
	int len = 0;

	len += snprintf(buf + len, PAGE_SIZE - len, "clkbuf_debug=%d\n",
	    clkbuf_debug);

	return len;
}

DEFINE_ATTR_RW(clk_buf_ctrl);
DEFINE_ATTR_RW(clk_buf_debug);

static struct attribute *clk_buf_attrs[] = {
	/* for clock buffer control */
	__ATTR_OF(clk_buf_ctrl), __ATTR_OF(clk_buf_debug),

	/* must */
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name = "clk_buf", .attrs = clk_buf_attrs,
};

bool is_clk_buf_from_pmic(void)
{
#if !defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	unsigned int reg = 0;
#endif
	bool ret = true;

	if (is_clkbuf_initiated)
	return is_pmic_clkbuf;

/*   copy from rtc:
 *  1.	TEST_CON0(0x208)[12:8] = 0x11
 *  2.	DA_XMODE =  TEST_OUT(0x206)[0]
 *  3.	DA_DCXO32K_EN =	 TEST_OUT(0x206)[1]
 *  when  DCXO32K_EN=1 and XMODE=0 => clkbuf is from RF,  otherwise clkbuf
 *is from pmic
 */

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)

#ifdef CLKBUF_COVCTCXO_MODE
	ret = false;
#else
	ret = true;
#endif

#else  /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */
	pmic_config_interface(0x208, 0x11, 0x1F, 8);
	pmic_read_interface(0x206, &reg, PMIC_REG_MASK, PMIC_REG_SHIFT);

	if ((reg & 0x3) == 0x2) {
	clk_buf_warn("clkbuf is from RF, 0x206=0x%x\n", reg);
	ret = false;
	} else {
	clk_buf_warn("clkbuf is from PMIC, 0x206=0x%x\n", reg);
	ret = true;
	}
#endif /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */

	return ret;
};

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
static void gen_pmic_cw16_rg_val(void)
{
#ifdef TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP
	g_pmic_cw16_rg_val = MT6355_PMIC_CW16_SUGGEST_VAL;
#else
	if (PMIC_CLK_BUF5_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF5_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF6_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF6_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF7_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF7_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF8_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF8_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF9_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF9_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF10_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF10_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	if (PMIC_CLK_BUF11_DRIVING_CURR == CLK_BUF_DRIVING_CURR_AUTO_K)
	PMIC_CLK_BUF11_DRIVING_CURR = CLK_BUF_DRIVING_CURR_2;

	g_pmic_cw16_rg_val = MT6355_PMIC_CW16_SUGGEST_VAL |
		 (PMIC_CLK_BUF5_DRIVING_CURR << 0) |
		 (PMIC_CLK_BUF6_DRIVING_CURR << 2) |
		 (PMIC_CLK_BUF7_DRIVING_CURR << 4) |
		 (PMIC_CLK_BUF8_DRIVING_CURR << 6) |
		 (PMIC_CLK_BUF9_DRIVING_CURR << 8) |
		 (PMIC_CLK_BUF10_DRIVING_CURR << 10) |
		 (PMIC_CLK_BUF11_DRIVING_CURR << 12);
#endif
	clk_buf_warn("%s: g_pmic_cw16_rg_val=0x%x, pmic_drv_curr_vals=%d %d %d %d %d %d %d\n",
	     __func__, g_pmic_cw16_rg_val, PMIC_CLK_BUF5_DRIVING_CURR,
	     PMIC_CLK_BUF6_DRIVING_CURR, PMIC_CLK_BUF7_DRIVING_CURR,
	     PMIC_CLK_BUF8_DRIVING_CURR, PMIC_CLK_BUF9_DRIVING_CURR,
	     PMIC_CLK_BUF10_DRIVING_CURR, PMIC_CLK_BUF11_DRIVING_CURR);
}
#else /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */
static void gen_pmic_cw13_rg_val(void)
{
#ifdef TEST_SUGGEST_PMIC_DRIVING_CURR_BEFORE_MP
	g_pmic_cw13_rg_val = PMIC_CW13_SUGGEST_VAL;
#else
	g_pmic_cw13_rg_val = PMIC_CW13_XO_EXTBUF_HD_VAL |
		 (PMIC_CLK_BUF5_DRIVING_CURR << 2) |
		 (PMIC_CLK_BUF6_DRIVING_CURR << 6) |
		 (PMIC_CLK_BUF7_DRIVING_CURR << 10) |
		 (PMIC_CLK_BUF8_DRIVING_CURR << 14);
#endif

	clk_buf_warn(
	"%s: g_pmic_cw13_rg_val=0x%x, pmic_drv_curr_vals=%d %d %d %d\n",
	__func__, g_pmic_cw13_rg_val, PMIC_CLK_BUF5_DRIVING_CURR,
	PMIC_CLK_BUF6_DRIVING_CURR, PMIC_CLK_BUF7_DRIVING_CURR,
	PMIC_CLK_BUF8_DRIVING_CURR);
}
#endif /* !defined(CONFIG_MTK_PMIC_CHIP_MT6355) */

static void clk_buf_pmic_wrap_init(void)
{
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	u32 pmic_cw00 = 0, pmic_cw11 = 0, pmic_cw14 = 0, pmic_cw16 = 0;
#else
	u32 conn_conf = 0, nfc_conf = 0;
	u32 pmic_cw13 = 0;
#endif
#if 0 /* config GPIO for debug */
	mt_set_gpio_mode(MT_GPIO_99, GPIO_MODE_01);
	mt_set_gpio_pull_enable(MT_GPIO_99, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(MT_GPIO_99, GPIO_DIR_IN);
#endif

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)

	gen_pmic_cw16_rg_val();

	/* Dump registers before setting */
	pmic_read_interface(MT6355_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW11, &pmic_cw11, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW14, &pmic_cw14, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW16, &pmic_cw16, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW11=0x%x, CW14=0x%x, CW16=0x%x\n",
	     __func__, pmic_cw00, pmic_cw11, pmic_cw14, pmic_cw16);

	g_pmic_cw00_rg_val = MT6355_PMIC_CW00_INIT_VAL;

	if (CLK_BUF6_STATUS_PMIC == CLOCK_BUFFER_DISABLE) {
	g_pmic_cw00_rg_val =
	    g_pmic_cw00_rg_val &
	    ~(PMIC_XO_EXTBUF2_MODE_MASK
	      << PMIC_XO_EXTBUF2_MODE_SHIFT); /*clkbuf2 mode = 00*/

	g_pmic_cw00_rg_val =
	    g_pmic_cw00_rg_val &
	    ~(PMIC_XO_EXTBUF2_EN_M_MASK
	      << PMIC_XO_EXTBUF2_EN_M_SHIFT); /*clkbuf2 en_m = 0*/
	}

#ifdef DISABLE_PMIC_CLKBUF3
	g_pmic_cw00_rg_val =
	g_pmic_cw00_rg_val &
	~(PMIC_XO_EXTBUF3_MODE_MASK
	  << PMIC_XO_EXTBUF3_MODE_SHIFT); /*clkbuf3 mode = 00*/

	g_pmic_cw00_rg_val =
	g_pmic_cw00_rg_val &
	~(PMIC_XO_EXTBUF3_EN_M_MASK
	  << PMIC_XO_EXTBUF3_EN_M_SHIFT); /*clkbuf3 en_m = 0*/
#endif

	if (is_pmic_clkbuf == true) {
	/* Setup initial PMIC clock buffer setting */
	pmic_config_interface(MT6355_DCXO_CW00, g_pmic_cw00_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);

	pmic_config_interface(MT6355_DCXO_CW11,
		      MT6355_PMIC_CW11_INIT_VAL, PMIC_REG_MASK,
		      PMIC_REG_SHIFT);

	/* set XO_EXTBUF2_CLKSEL_MAN=1 for controlling CLKBUF2 by
	 * PMIC_XO_EXTBUF2_EN_M
	 */
	pmic_config_interface(MT6355_DCXO_CW14, 0x1,
		      PMIC_XO_EXTBUF2_CLKSEL_MAN_MASK,
		      PMIC_XO_EXTBUF2_CLKSEL_MAN_SHIFT);

	pmic_config_interface(MT6355_DCXO_CW16, g_pmic_cw16_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);

	/* Check if the setting is ok */
	pmic_read_interface(MT6355_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW11, &pmic_cw11, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW14, &pmic_cw14, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW16, &pmic_cw16, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	clk_buf_warn(
	    "%s DCXO_CW00=0x%x, CW11=0x%x, CW14=0x%x, CW16=0x%x\n",
	    __func__, pmic_cw00, pmic_cw11, pmic_cw14, pmic_cw16);

	/* Setup PMIC_WRAP setting for XO2 & XO3 */
	if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_DISABLE) {
	    clkbuf_writel(DCXO_CONN_ADR0, MT6355_DCXO_CW00_CLR);
	    clkbuf_writel(
		DCXO_CONN_WDATA0,
		PMIC_XO_EXTBUF2_EN_M_MASK
		<< PMIC_XO_EXTBUF2_EN_M_SHIFT); /* bit5 = 0 */
	    clkbuf_writel(DCXO_CONN_ADR1, MT6355_DCXO_CW00_SET);
	    clkbuf_writel(
		DCXO_CONN_WDATA1,
		PMIC_XO_EXTBUF2_EN_M_MASK
		<< PMIC_XO_EXTBUF2_EN_M_SHIFT); /* bit5 = 1 */

	    clkbuf_writel(DCXO_ENABLE, clkbuf_readl(DCXO_ENABLE) |
			       DCXO_CONN_ENABLE);
	}

#if !defined(DISABLE_PMIC_CLKBUF3)
	clkbuf_writel(DCXO_NFC_ADR0, MT6355_DCXO_CW00_CLR);
	clkbuf_writel(DCXO_NFC_WDATA0,
		  PMIC_XO_EXTBUF3_EN_M_MASK
		  << PMIC_XO_EXTBUF3_EN_M_SHIFT); /* bit8 = 0 */
	clkbuf_writel(DCXO_NFC_ADR1, MT6355_DCXO_CW00_SET);
	clkbuf_writel(DCXO_NFC_WDATA1,
		  PMIC_XO_EXTBUF3_EN_M_MASK
		  << PMIC_XO_EXTBUF3_EN_M_SHIFT); /* bit8 = 1 */

	clkbuf_writel(DCXO_ENABLE,
		  clkbuf_readl(DCXO_ENABLE) | DCXO_NFC_ENABLE);
#endif /* !defined(DYNAMIC_ENABLE_PMIC_CLKBU2) */
	} else {
	/* turn off all pmic clock buffer */
	g_pmic_cw00_rg_val = MT6355_PMIC_CW00_INIT_VAL_OFF;

	pmic_config_interface(MT6355_DCXO_CW00, g_pmic_cw00_rg_val,
		      PMIC_REG_MASK, PMIC_REG_SHIFT);

	pmic_config_interface(MT6355_DCXO_CW11,
		      MT6355_PMIC_CW11_INIT_VAL, PMIC_REG_MASK,
		      PMIC_REG_SHIFT);

	/* Check if the setting is ok */
	pmic_read_interface(MT6355_DCXO_CW00, &pmic_cw00, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW11, &pmic_cw11, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW14, &pmic_cw14, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	pmic_read_interface(MT6355_DCXO_CW16, &pmic_cw16, PMIC_REG_MASK,
		    PMIC_REG_SHIFT);
	clk_buf_warn("%s DCXO_CW00=0x%x, CW11=0x%x, CW14=0x%x, CW16=0x%x because pmic clock buffer = 0\n",
		 __func__, pmic_cw00, pmic_cw11, pmic_cw14,
		 pmic_cw16);
	}

	clk_buf_warn(
	"%s: DCXO_CONN_ADR0/WDATA0/ADR1/WDATA1/EN=0x%x/%x/%x/%x/%x\n",
	__func__, clkbuf_readl(DCXO_CONN_ADR0),
	clkbuf_readl(DCXO_CONN_WDATA0), clkbuf_readl(DCXO_CONN_ADR1),
	clkbuf_readl(DCXO_CONN_WDATA1), clkbuf_readl(DCXO_ENABLE));
	clk_buf_warn("%s: DCXO_NFC_ADR0/WDATA0/ADR1/WDATA1=0x%x/%x/%x/%x\n",
	     __func__, clkbuf_readl(DCXO_NFC_ADR0),
	     clkbuf_readl(DCXO_NFC_WDATA0), clkbuf_readl(DCXO_NFC_ADR1),
	     clkbuf_readl(DCXO_NFC_WDATA1));

#else /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */

	gen_pmic_cw13_rg_val();

#if 0 /* debug only */
	/* Switch clkbuf2,3,4 to S/W mode control */
	pmic_config_interface(PMIC_CW00_ADDR, 0,
		  PMIC_CW00_XO_EXTBUF2_MODE_MASK,
		  PMIC_CW00_XO_EXTBUF2_MODE_SHIFT); /* XO_WCN */
	pmic_config_interface(PMIC_CW00_ADDR, 0,
		  PMIC_CW00_XO_EXTBUF3_MODE_MASK,
		  PMIC_CW00_XO_EXTBUF3_MODE_SHIFT); /* XO_NFC */
	pmic_config_interface(PMIC_CW00_ADDR, 0,
		  PMIC_CW00_XO_EXTBUF4_MODE_MASK,
		  PMIC_CW00_XO_EXTBUF4_MODE_SHIFT); /* XO_CEL */
#else

	g_pmic_cw00_rg_val = PMIC_CW00_INIT_VAL;

	if (CLK_BUF6_STATUS_PMIC == CLOCK_BUFFER_DISABLE) {
	pmic_config_interface(PMIC_CW00_ADDR, 0,
		      PMIC_CW00_XO_EXTBUF2_EN_M_MASK,
		      PMIC_CW00_XO_EXTBUF2_EN_M_SHIFT);
	g_pmic_cw00_rg_val = g_pmic_cw00_rg_val &
		     ~(PMIC_CW00_XO_EXTBUF2_MODE_MASK
		       << PMIC_CW00_XO_EXTBUF2_MODE_SHIFT);
	pmic_config_interface(PMIC_CW00_ADDR, 0,
		      PMIC_CW00_XO_EXTBUF2_MODE_MASK,
		      PMIC_CW00_XO_EXTBUF2_MODE_SHIFT);
	}

#ifdef DISABLE_PMIC_CLKBUF3
	pmic_config_interface(PMIC_CW14_ADDR, 0, PMIC_CW14_XO_EXTBUF3_EN_M_MASK,
		  PMIC_CW14_XO_EXTBUF3_EN_M_SHIFT);
	g_pmic_cw00_rg_val = g_pmic_cw00_rg_val &
		 ~(PMIC_CW00_XO_EXTBUF3_MODE_MASK
		   << PMIC_CW00_XO_EXTBUF3_MODE_SHIFT);
	pmic_config_interface(PMIC_CW00_ADDR, 0, PMIC_CW00_XO_EXTBUF3_MODE_MASK,
		  PMIC_CW00_XO_EXTBUF3_MODE_SHIFT);
#endif
	/* Setup initial PMIC clock buffer setting */
	pmic_read_interface(PMIC_CW00_ADDR, &conn_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW14_ADDR, &nfc_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW13_ADDR, &pmic_cw13, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	clk_buf_warn("%s PMIC_CW00_ADDR=0x%x, PMIC_CW14_ADDR=0x%x, PMIC_CW13_ADDR=0x%x\n",
	     __func__, conn_conf, nfc_conf, pmic_cw13);
	pmic_config_interface(PMIC_CW00_ADDR, g_pmic_cw00_rg_val, PMIC_REG_MASK,
		  PMIC_REG_SHIFT);
	pmic_config_interface(PMIC_CW13_ADDR, g_pmic_cw13_rg_val, PMIC_REG_MASK,
		  PMIC_REG_SHIFT);

#endif

	/* Check if the setting is ok */
	pmic_read_interface(PMIC_CW00_ADDR, &conn_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW14_ADDR, &nfc_conf, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	pmic_read_interface(PMIC_CW13_ADDR, &pmic_cw13, PMIC_REG_MASK,
		PMIC_REG_SHIFT);
	clk_buf_warn("%s PMIC_CW00_ADDR=0x%x, PMIC_CW14_ADDR=0x%x, PMIC_CW13_ADDR=0x%x\n",
	     __func__, conn_conf, nfc_conf, pmic_cw13);

	if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_DISABLE) {
	clkbuf_writel(DCXO_CONN_ADR0, PMIC_CW00_ADDR);
	clkbuf_writel(DCXO_CONN_WDATA0,
		  conn_conf & 0xFFDF); /* bit5 = 0 */
	clkbuf_writel(DCXO_CONN_ADR1, PMIC_CW00_ADDR);
	clkbuf_writel(DCXO_CONN_WDATA1,
		  conn_conf | 0x0020); /* bit5 = 1 */
	}
#ifdef DISABLE_PMIC_CLKBUF3
#else
	clkbuf_writel(DCXO_NFC_ADR0, PMIC_CW14_ADDR);
	clkbuf_writel(DCXO_NFC_WDATA0, nfc_conf & 0xF7FF); /* bit11 = 0 */
	clkbuf_writel(DCXO_NFC_ADR1, PMIC_CW14_ADDR);
	clkbuf_writel(DCXO_NFC_WDATA1, nfc_conf | 0x0800); /* bit11 = 1 */
#endif

	/* Enable pmic_wrap sleep gated control */
	clkbuf_writel(HARB_SLEEP_GATED_CTRL, HARB_SLEEP_GATED_EN);

#ifdef DISABLE_PMIC_CLKBUF3
	if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_DISABLE)
	clkbuf_writel(DCXO_ENABLE, DCXO_CONN_ENABLE);
#else
	if (CLK_BUF6_STATUS_PMIC != CLOCK_BUFFER_DISABLE)
	clkbuf_writel(DCXO_ENABLE, DCXO_CONN_ENABLE | DCXO_NFC_ENABLE);
	else
	clkbuf_writel(DCXO_ENABLE, DCXO_NFC_ENABLE);
#endif

	clk_buf_warn(
	"%s: "
	"DCXO_CONN_ADR0/WDATA0/ADR1/WDATA1/EN=0x%x/%x/%x/%x/%x,HSGC=%x\n",
	__func__, clkbuf_readl(DCXO_CONN_ADR0),
	clkbuf_readl(DCXO_CONN_WDATA0), clkbuf_readl(DCXO_CONN_ADR1),
	clkbuf_readl(DCXO_CONN_WDATA1), clkbuf_readl(DCXO_ENABLE),
	clkbuf_readl(HARB_SLEEP_GATED_CTRL));
	clk_buf_warn("%s: DCXO_NFC_ADR0/WDATA0/ADR1/WDATA1=0x%x/%x/%x/%x\n",
	     __func__, clkbuf_readl(DCXO_NFC_ADR0),
	     clkbuf_readl(DCXO_NFC_WDATA0), clkbuf_readl(DCXO_NFC_ADR1),
	     clkbuf_readl(DCXO_NFC_WDATA1));
#endif /* defined(CONFIG_MTK_PMIC_CHIP_MT6355) */
}

static int clk_buf_fs_init(void)
{
	int r = 0;

#if defined(CONFIG_PM)
	/* create /sys/power/clk_buf/xxx */
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
	clk_buf_err("FAILED TO CREATE /sys/power/clk_buf (%d)\n", r);
#endif

	return r;
}

static void clk_buf_clear_rf_setting(void)
{
	memset(clk_buf_swctrl, 0, sizeof(clk_buf_swctrl));
	memset(clk_buf_swctrl_modem_on, 0, sizeof(clk_buf_swctrl_modem_on));

#if !defined(CONFIG_MTK_LEGACY)
	CLK_BUF1_STATUS = CLOCK_BUFFER_DISABLE;
	CLK_BUF2_STATUS = CLOCK_BUFFER_DISABLE;
	CLK_BUF3_STATUS = CLOCK_BUFFER_DISABLE;
	CLK_BUF4_STATUS = CLOCK_BUFFER_DISABLE;

	clk_buf_warn("%s: RF_CLK_BUF?_STATUS=%d %d %d %d\n", __func__,
	     CLK_BUF1_STATUS, CLK_BUF2_STATUS, CLK_BUF3_STATUS,
	     CLK_BUF4_STATUS);
	clk_buf_warn("%s: PMIC_CLK_BUF?_STATUS=%d %d %d %d\n", __func__,
	     CLK_BUF5_STATUS_PMIC, CLK_BUF6_STATUS_PMIC,
	     CLK_BUF7_STATUS_PMIC, CLK_BUF8_STATUS_PMIC);
#endif
}

bool clk_buf_init(void)
{
	struct device_node *node;
#if !defined(CONFIG_MTK_LEGACY)
	u32 vals[CLKBUF_NUM];
	int ret = -1;

	node = of_find_compatible_node(NULL, NULL, "mediatek,rf_clock_buffer");
	if (node) {
	ret = of_property_read_u32_array(node, "mediatek,clkbuf-config",
			 vals, RF_CLKBUF_NUM);
	if (!ret) {
	    CLK_BUF1_STATUS = vals[0];
	    CLK_BUF2_STATUS = vals[1];
	    CLK_BUF3_STATUS = vals[2];
	    CLK_BUF4_STATUS = vals[3];
	}
	ret = of_property_read_u32_array(
	    node, "mediatek,clkbuf-driving-current", vals,
	    RF_CLKBUF_NUM);
	if (!ret) {
	    RF_CLK_BUF1_DRIVING_CURR = vals[0];
	    RF_CLK_BUF2_DRIVING_CURR = vals[1];
	    RF_CLK_BUF3_DRIVING_CURR = vals[2];
	    RF_CLK_BUF4_DRIVING_CURR = vals[3];
	}

	clk_buf_warn("%s RF clkbuf Status:  %d, %d, %d, %d\n", __func__,
		 CLK_BUF1_STATUS, CLK_BUF2_STATUS, CLK_BUF3_STATUS,
		 CLK_BUF4_STATUS);
	clk_buf_warn("%s RF clkbuf Current: %d, %d, %d, %d\n", __func__,
		 RF_CLK_BUF1_DRIVING_CURR, RF_CLK_BUF2_DRIVING_CURR,
		 RF_CLK_BUF3_DRIVING_CURR,
		 RF_CLK_BUF4_DRIVING_CURR);
	} else {
	clk_buf_err(
	    "%s can't find compatible node for rf_clock_buffer\n",
	    __func__);
	WARN_ON(1);
	}

	node =
	of_find_compatible_node(NULL, NULL, "mediatek,pmic_clock_buffer");
	if (node) {
	ret = of_property_read_u32_array(node, "mediatek,clkbuf-config",
			 vals, CLKBUF_NUM);
	if (!ret) {
	    CLK_BUF5_STATUS_PMIC = vals[0];
	    CLK_BUF6_STATUS_PMIC = vals[1];
	    CLK_BUF7_STATUS_PMIC = vals[2];
	    CLK_BUF8_STATUS_PMIC = vals[3];
	}
	ret = of_property_read_u32_array(
	    node, "mediatek,clkbuf-driving-current", vals, CLKBUF_NUM);
	if (!ret) {
	    PMIC_CLK_BUF5_DRIVING_CURR = vals[0];
	    PMIC_CLK_BUF6_DRIVING_CURR = vals[1];
	    PMIC_CLK_BUF7_DRIVING_CURR = vals[2];
	    PMIC_CLK_BUF8_DRIVING_CURR = vals[3];
#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
	    PMIC_CLK_BUF9_DRIVING_CURR = vals[4];
	    PMIC_CLK_BUF10_DRIVING_CURR = vals[5];
	    PMIC_CLK_BUF11_DRIVING_CURR = vals[6];
#endif
	}
	} else {
	clk_buf_err(
	    "%s can't find compatible node for pmic_clock_buffer\n",
	    __func__);
	WARN_ON(1);
	}
#endif

	if (is_clkbuf_initiated)
	return false;

	if (clk_buf_fs_init())
	return false;

	node = of_find_compatible_node(NULL, NULL, "mediatek,pwrap");
	if (node)
	pwrap_base = of_iomap(node, 0);
	else {
	clk_buf_err("%s can't find compatible node for pwrap\n",
		__func__);
	WARN_ON(1);
	}

	/* Co-TSX @PMIC */
	if (is_clk_buf_from_pmic()) {
	is_pmic_clkbuf = true;

	clk_buf_pmic_wrap_init();
	clk_buf_clear_rf_setting();
	} else { /* VCTCXO @RF */
	is_pmic_clkbuf = false;

	if (CLK_BUF2_STATUS == CLOCK_BUFFER_DISABLE) {
	    clk_buf_swctrl_modem_on[CLK_BUF_CONN] =
		CLK_BUF_SW_DISABLE;
	    clk_buf_swctrl[CLK_BUF_CONN] = CLK_BUF_SW_DISABLE;
	}

	if (CLK_BUF3_STATUS == CLOCK_BUFFER_DISABLE) {
	    clk_buf_swctrl_modem_on[CLK_BUF_NFC] =
		CLK_BUF_SW_DISABLE;
	    clk_buf_swctrl[CLK_BUF_NFC] = CLK_BUF_SW_DISABLE;
	}

	clk_buf_pmic_wrap_init();

	if (g_is_flightmode_on)
	    spm_clk_buf_ctrl(clk_buf_swctrl);
	else
	    spm_clk_buf_ctrl(clk_buf_swctrl_modem_on);

	spm_write(SPM_BSI_EN_SR, afcdac_val);
	clk_buf_warn("%s: afcdac=0x%x, SPM_BSI_EN_SR=0x%x\n", __func__,
		 afcdac_val, spm_read(SPM_BSI_EN_SR));
	}

	INIT_DELAYED_WORK(&clkbuf_delayed_work, clkbuf_delayed_worker);

	is_clkbuf_initiated = true;

	return true;
}

#if !defined(MT_DORMANT_UT)
static int __init mt_clkbuf_late_init(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && !defined(CLKBUF_BRINGUP)
	clk_buf_init();
#else
	is_pmic_clkbuf = true;
	clk_buf_fs_init();
	clk_buf_warn(
	"clk_buf_ctrl is disabled for bring-up or FPGA early porting\n");
#endif
	return 0;
}

late_initcall(mt_clkbuf_late_init);
#endif /* #if !defined (MT_DORMANT_UT) */

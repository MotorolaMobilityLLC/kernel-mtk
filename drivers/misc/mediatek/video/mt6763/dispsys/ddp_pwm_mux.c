/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <ddp_clkmgr.h>
#include <ddp_pwm_mux.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <ddp_reg.h>

#define PWM_MSG(fmt, arg...) pr_debug("[PWM] " fmt "\n", ##arg)
#define PWM_ERR(fmt, arg...) pr_err("[PWM] " fmt "\n", ##arg)

#define BYPASS_CLK_SELECT

/*****************************************************************************
 *
 * dummy function
 *
*****************************************************************************/
#ifdef BYPASS_CLK_SELECT
int disp_pwm_set_pwmmux(unsigned int clk_req)
{
	return 0;
}

int disp_pwm_clksource_enable(int clk_req)
{
	return 0;
}

int disp_pwm_clksource_disable(int clk_req)
{
	return 0;
}

bool disp_pwm_mux_is_osc(void)
{
	return false;
}
#else
/*****************************************************************************
 *
 * variable for get clock node fromdts
 *
*****************************************************************************/
static void __iomem *disp_pmw_mux_base;

#ifndef MUX_DISPPWM_ADDR /* disp pwm source clock select register address */
#define MUX_DISPPWM_ADDR (disp_pmw_mux_base + 0xB0)
#endif
#ifdef HARD_CODE_CONFIG
#ifndef MUX_UPDATE_ADDR /* disp pwm source clock update register address */
#define MUX_UPDATE_ADDR (disp_pmw_mux_base + 0x4)
#endif
#endif
#ifndef OSC_ULPOSC_ADDR /* rosc control register address */
#define OSC_ULPOSC_ADDR (disp_pmw_osc_base + 0x458)
#endif

/* clock hard code access API */
#define DRV_Reg32(addr) INREG32(addr)
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)

static int g_pwm_mux_clock_source = -1;

/*****************************************************************************
 *
 * disp pwm source clock select mux api
 *
*****************************************************************************/
enum DDP_CLK_ID disp_pwm_get_clkid(unsigned int clk_req)
{
	enum DDP_CLK_ID clkid = -1;

	switch (clk_req) {
	case 0:
		clkid = ULPOSC_D8; /* ULPOSC 29M */
		break;
	case 1:
		clkid = ULPOSC_D4; /* ULPOSC 58M */
		break;
	case 2:
		clkid = UNIVPLL2_D4; /* PLL 104M */
		break;
	case 3:
		clkid = CLK26M; /* SYS 26M */
		break;
	default:
		clkid = -1;
		break;
	}

	return clkid;
}

/*****************************************************************************
 *
 * get disp pwm source mux node
 *
*****************************************************************************/
#define DTSI_TOPCKGEN "mediatek,topckgen"
static int disp_pwm_get_muxbase(void)
{
	int ret = 0;
	struct device_node *node;

	if (disp_pmw_mux_base != NULL)
		return 0;

	node = of_find_compatible_node(NULL, NULL, DTSI_TOPCKGEN);
	if (!node) {
		PWM_ERR("DISP find TOPCKGEN node failed\n");
		return -1;
	}
	disp_pmw_mux_base = of_iomap(node, 0);
	if (!disp_pmw_mux_base) {
		PWM_ERR("DISP TOPCKGEN base failed\n");
		return -1;
	}
	PWM_MSG("find TOPCKGEN node");
	return ret;
}

static unsigned int disp_pwm_get_pwmmux(void)
{
	unsigned int regsrc = 0;

	if (MUX_DISPPWM_ADDR != NULL)
		regsrc = clk_readl(MUX_DISPPWM_ADDR);
	else
		PWM_ERR("mux addr illegal");

	return regsrc;
}

/*****************************************************************************
 *
 * disp pwm source clock select mux api
 *
*****************************************************************************/
int disp_pwm_set_pwmmux(unsigned int clk_req)
{
	unsigned int reg_before, reg_after;
	int ret = 0;
	enum DDP_CLK_ID clkid = -1;

	clkid = disp_pwm_get_clkid(clk_req);
	ret = disp_pwm_get_muxbase();
	reg_before = disp_pwm_get_pwmmux();

	PWM_MSG("clk_req=%d clkid=%d", clk_req, clkid);
	if (clkid != -1) {
		ddp_clk_enable(MUX_PWM);
		ddp_clk_set_parent(MUX_PWM, clkid);
		ddp_clk_disable(MUX_PWM);
	}

	reg_after = disp_pwm_get_pwmmux();
	g_pwm_mux_clock_source = reg_after & 0x3;
	PWM_MSG("PWM_MUX %x->%x", reg_before, reg_after);

	return 0;
}

static void __iomem *disp_pmw_osc_base;

/*****************************************************************************
 *
 * get disp pwm source osc
 *
*****************************************************************************/
static int get_ulposc_base(void)
{
	int ret = 0;
	struct device_node *node;

	if (disp_pmw_osc_base != NULL) {
		PWM_MSG("SLEEP node exist");
		return 0;
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node) {
		PWM_ERR("DISP find SLEEP node failed\n");
		return -1;
	}
	disp_pmw_osc_base = of_iomap(node, 0);
	if (!disp_pmw_osc_base) {
		PWM_ERR("DISP find SLEEP base failed\n");
		return -1;
	}

	return ret;
}

static int get_ulposc_status(void)
{
	unsigned int regosc;
	int ret = -1;

	if (get_ulposc_base() == -1) {
		PWM_ERR("get ULPOSC status fail");
		return ret;
	}

	regosc = clk_readl(OSC_ULPOSC_ADDR);
	if ((regosc & 0x5) != 0x5) {
		PWM_MSG("ULPOSC is off (%x)", regosc);
		ret = 0;
	} else {
		PWM_MSG("ULPOSC is on (%x)", regosc);
		ret = 1;
	}

	return ret;
}

/*****************************************************************************
 *
 * hardcode turn on/off ROSC api
 *
*****************************************************************************/
static int ulposc_on(void)
{
	unsigned int regosc;

	if (get_ulposc_base() == -1)
		return -1;

	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x", regosc); */

	/* OSC EN = 1 */
	regosc = regosc | 0x1;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after en", regosc); */
	udelay(11);

	/* OSC RST	*/
	regosc = regosc | 0x2;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after rst 1", regosc); */
	udelay(40);
	regosc = regosc & 0xfffffffd;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after rst 0", regosc); */
	udelay(130);

	/* OSC CG_EN = 1 */
	regosc = regosc | 0x4;
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after cg_en", regosc); */

	return 0;

}

static int ulposc_off(void)
{
	unsigned int regosc;

	if (get_ulposc_base() == -1)
		return -1;

	regosc = clk_readl(OSC_ULPOSC_ADDR);

	/* OSC CG_EN = 0 */
	regosc = regosc & (~0x4);
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after cg_en", regosc); */

	udelay(40);

	/* OSC EN = 0 */
	regosc = regosc & (~0x1);
	clk_writel(OSC_ULPOSC_ADDR, regosc);
	regosc = clk_readl(OSC_ULPOSC_ADDR);
	/* PWM_MSG("ULPOSC config : 0x%08x after en", regosc); */

	return 0;
}

static int ulposc_enable(enum DDP_CLK_ID clkid)
{
	ulposc_on();
	get_ulposc_status();

	return 0;
}

static int ulposc_disable(enum DDP_CLK_ID clkid)
{
	ulposc_off();
	get_ulposc_status();

	return 0;
}

/*****************************************************************************
 *
 * disp pwm clock source power on /power off api
 *
*****************************************************************************/
int disp_pwm_clksource_enable(int clk_req)
{
	int ret = 0;
	enum DDP_CLK_ID clkid = -1;

	clkid = disp_pwm_get_clkid(clk_req);

	switch (clkid) {
	case ULPOSC_D4:
	case ULPOSC_D8:
	/* FIXME: case ULPOSC_D10:*/
		ulposc_enable(clkid);
		break;
	default:
		break;
	}

	return ret;
}

int disp_pwm_clksource_disable(int clk_req)
{
	int ret = 0;
	enum DDP_CLK_ID clkid = -1;

	clkid = disp_pwm_get_clkid(clk_req);

	switch (clkid) {
	case ULPOSC_D4:
	case ULPOSC_D8:
	/* FIXME: case ULPOSC_D10: */
		ulposc_disable(clkid);
		break;
	default:
		break;
	}

	return ret;
}

/*****************************************************************************
 *
 * disp pwm clock source query api
 *
*****************************************************************************/

bool disp_pwm_mux_is_osc(void)
{
	bool is_osc = false;

	if (g_pwm_mux_clock_source == 3 || g_pwm_mux_clock_source == 2)
		is_osc = true;

	return is_osc;
}

/*****************************************************************************
 *
 * ulposc clock source calibration
 *
*****************************************************************************/
static void __iomem *infracfg_ao_base;

/* ULPOSC control register addr */
#define PLL_OSC_CON0		(infracfg_ao_base + 0xB00)
#define PLL_OSC_CON1		(infracfg_ao_base + 0xB04)
#define OSC_ULPOSC_DBG		(disp_pmw_osc_base + 0x000)
#define FQMTR_CK_EN		(disp_pmw_mux_base + 0x220)    /* Enable fqmeter reference clk */
#define SET_DIV_CNT		(disp_pmw_mux_base + 0x104)    /* Set fqmeter div count */
#define AD_OSC_CLK_DBG		(disp_pmw_mux_base + 0x10c)    /* Select debug for AD_OSC_CLK */
#define FQMTR_OUTPUT		(disp_pmw_mux_base + 0x224)    /* Check the result [15:0] */
#define DEFAULT_CALI		(0x00AD6E2B)
#define ULP_FQMTR_MIDDLE	(0x49D8)
#define ULP_FQMTR_CEIL		(0x513B)
#define ULP_FQMTR_FLOOR		(0x4277)

#define DTSI_INFRACFG_AO "mediatek,infracfg_ao"
static int disp_pwm_get_infracfg_ao_base(void)
{
	int ret = 0;
	struct device_node *node;

	if (infracfg_ao_base != NULL)
		return 0;

	node = of_find_compatible_node(NULL, NULL, DTSI_INFRACFG_AO);
	if (!node) {
		PWM_ERR("Find INFRACFG_AO node failed\n");
		return -1;
	}
	infracfg_ao_base = of_iomap(node, 0);
	if (!infracfg_ao_base) {
		PWM_ERR("INFRACFG_AO base failed\n");
		return -1;
	}
	PWM_MSG("find INFRACFG_AO node");
	return ret;
}

static uint32_t disp_pwm_get_ulposc_meter_val(uint32_t cali_val)
{
	uint32_t result = 0, polling_result = 0;
	uint32_t rsv_fqmtr_ck_en, rsv_div_cnt, rsv_osc_con;

	rsv_fqmtr_ck_en = clk_readl(FQMTR_CK_EN);
	rsv_div_cnt = clk_readl(SET_DIV_CNT);

	rsv_osc_con = clk_readl(PLL_OSC_CON0);
	clk_writel(PLL_OSC_CON0, (rsv_osc_con & ~0x3F) | cali_val);
	clk_writel(FQMTR_CK_EN, 0x00001000);
	clk_writel(SET_DIV_CNT, 0x00ffffff);
	clk_writel(AD_OSC_CLK_DBG, 0x00260000);
	clk_writel(FQMTR_CK_EN, 0x00001010);
	do {
		polling_result = clk_readl(FQMTR_CK_EN) & 0x10;
		udelay(50);
	} while (polling_result != 0);
	result = clk_readl(FQMTR_OUTPUT) & 0xffff;

	clk_writel(FQMTR_CK_EN, rsv_fqmtr_ck_en);
	clk_writel(SET_DIV_CNT, rsv_div_cnt);
	PWM_MSG("get_cali_val 0x%x 0x%x\n", cali_val, result);

	return result;
}

static bool disp_pwm_is_frequency_correct(uint32_t meter_val)
{
	if (meter_val > ULP_FQMTR_FLOOR && meter_val < ULP_FQMTR_CEIL)
		return true;
	else
		return false;
}

void disp_pwm_ulposc_cali(void)
{
	uint32_t cali_val = 0, meter_val = 0;
	uint32_t left = 0x1f, right = 0x3f, middle;
	uint32_t diff_left = 0, diff_right = 0xffff;

	if (get_ulposc_base() != 0 || disp_pwm_get_muxbase() != 0 ||
		disp_pwm_get_infracfg_ao_base() != 0) {
		/* print error log */
		PWM_MSG("get base address fail\n");
	}

	clk_writel(PLL_OSC_CON0, DEFAULT_CALI);
	clk_writel(PLL_OSC_CON1, 0x4);

	clk_writel(OSC_ULPOSC_DBG, 0x0b160001);
	clk_writel(OSC_ULPOSC_ADDR, 0x1);
	udelay(30);
	clk_writel(OSC_ULPOSC_ADDR, 0x5);

	cali_val = DEFAULT_CALI & 0x3F;
	meter_val = disp_pwm_get_ulposc_meter_val(cali_val);

	if (disp_pwm_is_frequency_correct(meter_val) == true) {
		PWM_MSG("final cali_val: 0x%x\n", meter_val);
		return;
	}

	do {
		middle = (left + right) / 2;
		if (middle == left)
			break;

		cali_val = middle;
		meter_val = disp_pwm_get_ulposc_meter_val(cali_val);

		if (disp_pwm_is_frequency_correct(meter_val) == true) {
			PWM_MSG("final cali_val: 0x%x\n", meter_val);
			return;
		} else if (meter_val > ULP_FQMTR_MIDDLE)
			right = middle;
		else
			left = middle;
	} while (left <= right);

	cali_val = left;
	meter_val = disp_pwm_get_ulposc_meter_val(cali_val);
	if (meter_val > ULP_FQMTR_MIDDLE)
		diff_left = meter_val - ULP_FQMTR_MIDDLE;
	else
		diff_left = ULP_FQMTR_MIDDLE - meter_val;

	cali_val = right;
	meter_val = disp_pwm_get_ulposc_meter_val(cali_val);
	if (meter_val > ULP_FQMTR_MIDDLE)
		diff_right = meter_val - ULP_FQMTR_MIDDLE;
	else
		diff_right = ULP_FQMTR_MIDDLE - meter_val;

	if (diff_left < diff_right)
		cali_val = left;
	else
		cali_val = right;
	meter_val = disp_pwm_get_ulposc_meter_val(cali_val);

	PWM_MSG("final cali_val: 0x%x\n", meter_val);
}

void disp_pwm_ulposc_query(char *debug_output)
{
	char *temp_buf = debug_output;
	const size_t buf_max_len = 100;
	int buf_offset;
	uint32_t osc_con = 0, current_cali_value = 0, current_meter_value = 0;

	if (get_ulposc_base() != 0 || disp_pwm_get_muxbase() != 0 ||
		disp_pwm_get_infracfg_ao_base() != 0) {
		/* print error log */
		PWM_MSG("get base address fail\n");
	}

	buf_offset = snprintf(temp_buf, buf_max_len,
			  "0x10006458: (0x%08x)\n", clk_readl(OSC_ULPOSC_ADDR));
	temp_buf += buf_offset;
	buf_offset = snprintf(temp_buf, buf_max_len,
			  "0x10006000: (0x%08x)\n", clk_readl(OSC_ULPOSC_DBG));
	temp_buf += buf_offset;

	osc_con = clk_readl(PLL_OSC_CON0);
	buf_offset = snprintf(temp_buf, buf_max_len,
			  "0x10001B00: (0x%08x)\n", osc_con);
	temp_buf += buf_offset;

	buf_offset = snprintf(temp_buf, buf_max_len,
			  "0x10001B04: (0x%08x)\n", clk_readl(PLL_OSC_CON1));
	temp_buf += buf_offset;
	buf_offset = snprintf(temp_buf, buf_max_len,
			  "0x10000220: (0x%08x)\n", clk_readl(FQMTR_CK_EN));
	temp_buf += buf_offset;

	current_cali_value = osc_con & 0x3F;
	current_meter_value = disp_pwm_get_ulposc_meter_val(current_cali_value);
	buf_offset = snprintf(temp_buf, buf_max_len,
			  "current meter value: (0x%08x)\n", current_meter_value);
	temp_buf += buf_offset;
}
#endif		/* BYPASS_CLK_SELECT */

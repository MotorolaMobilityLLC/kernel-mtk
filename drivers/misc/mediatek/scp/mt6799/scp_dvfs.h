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

#ifndef __SCP_DVFS_H__
#define __SCP_DVFS_H__


struct mt_scp_dvfs_table_info {
	unsigned int cur_clk;
	unsigned int cur_volt;
	unsigned int fix_clk;
	bool fix_clk_on;
	unsigned int limited_clk;
	unsigned int limited_volt;
	bool limited_clk_on;
	bool scp_dvfs_disable;
	bool scp_dvfs_sleep;
	bool scp_dvfs_wake;
};

typedef enum {
	IN_DEBUG_IDLE = 1,
	ENTERING_SLEEP = 2,
	IN_SLEEP = 4,
	ENTERING_ACTIVE = 8,
	IN_ACTIVE = 16,
} scp_state_enum;

typedef enum {
	CLK_SEL_SYS     = 0x0,
	CLK_SEL_32K     = 0x1,
	CLK_SEL_HIGH    = 0x2
} clk_sel_val;

enum {
	CLK_SYS_EN_BIT = 0,
	CLK_HIGH_EN_BIT = 1,
	CLK_HIGH_CG_BIT = 2,
	CLK_SYS_IRQ_EN_BIT = 16,
	CLK_HIGH_IRQ_EN_BIT = 17,
};
/*#ifdef CONFIG_PINCTRL_MT6797*/

typedef enum  {
	CLK_OPP0 = 71,
	CLK_OPP1 = 165,
	CLK_OPP2 = 286,
	CLK_OPP3 = 330,
	CLK_OPP4 = 416,
	CLK_OPP_NUM,
} clk_opp_enum;

typedef enum  {
	FREQ_104MHZ = 71,
	FREQ_187MHZ = 165,
	FREQ_286MHZ = 286,
	FREQ_330MHZ = 330,
	FREQ_416MHZ = 416,
	FREQ_32KHZ = 32,
	FREQ_UNKNOWN = 0xffff,
} freq_enum;

typedef enum {
	CLK_DIV_1 = 0,
	CLK_DIV_2 = 1,
	CLK_DIV_4  = 2,
	CLK_DIV_8  = 3,
	CLK_DIV_UNKNOWN,
} clk_div_enum;

typedef enum  {
	SPM_VOLTAGE_800_D = 0,
	SPM_VOLTAGE_800,
	SPM_VOLTAGE_900,
	SPM_VOLTAGE_1000,
	SPM_VOLTAGE_TYPE_NUM,
} voltage_enum;

struct mt_scp_pll_t {
	struct clk *clk_mux;          /* main clock for mfg setting */
	struct clk *clk_pll0; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll1; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll2; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll3; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll4; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll5; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll6; /* substitution clock for scp transient parent setting */
	struct clk *clk_pll7; /* substitution clock for scp transient parent setting */
};

extern int scp_pll_ctrl_set(unsigned int pll_ctrl_flag, unsigned int pll_sel);
extern short  scp_set_pmic_vcore(unsigned int cur_freq);
extern unsigned int scp_get_dvfs_opp(void);
extern uint32_t scp_get_freq(void);
extern int scp_request_freq(void);

/* scp dvfs variable*/
extern unsigned int scp_expected_freq;
extern unsigned int scp_current_freq;
extern spinlock_t scp_awake_spinlock;

#endif  /* __SCP_DVFS_H__ */

/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef ADSP_CLK_H
#define ADSP_CLK_H

enum adsppll_freq {
	ADSPPLL_FREQ_480MHZ     = 480000000,
	ADSPPLL_FREQ_692MHZ     = 692000000,
	ADSPPLL_FREQ_800MHZ     = 800000000
};

enum adsp_clk {
	CLK_INFRA_ADSP,
	CLK_TOP_MUX_ADSP,
	CLK_TOP_ADSPPLL_CK,
	CLK_CLK26M,
	CLK_APMIXED_ADSPPLL,
	ADSP_CLK_NUM
};

extern struct clk *clk_adsp_infra;
extern struct clk *clk_top_mux_adsp;
extern struct clk *clk_top_adsppll_ck;
extern struct clk *clk_adsp_clk26;
extern struct clk *clk_apmixed_adsppll;

int adsppll_mux_setting(bool enable);
void set_adsppll_rate(enum adsppll_freq freq);
int adsp_set_top_mux(bool enable, enum adsp_clk clk);
int adsp_enable_clock(void);
void adsp_disable_clock(void);
#endif /* ADSP_CLK_H */

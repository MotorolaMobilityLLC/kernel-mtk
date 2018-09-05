/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/clk.h>
#include "adsp_clk.h"

int adsp_set_top_mux(bool enable, enum adsp_clk clk)
{
	int ret = 0;

	pr_debug("%s(%d, %x)\n", __func__, enable, clk);
	if (enable) {
		switch (clk) {
		case CLK_TOP_ADSPPLL_CK:
			ret = clk_set_parent(clk_top_mux_adsp,
					     clk_top_adsppll_ck);
			break;
		case CLK_CLK26M:
			ret = clk_set_parent(clk_top_mux_adsp,
					     clk_adsp_clk26);
			break;
		default:
			ret = -1;
			break;
		}
		if (IS_ERR(&ret)) {
			pr_err("[ADSP] %s clk_set_parent(clk_top_mux_adsp-%x) fail %d\n",
			      __func__, clk, ret);
			return ret;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

int adsppll_mux_setting(bool enable)
{
	int ret = 0;

	if (enable) {
		ret = clk_prepare_enable(clk_top_mux_adsp);
		if (IS_ERR(&ret)) {
			pr_err("[ADSP] %s clk_prepare_enable(clk_top_mux_adsp) fail %d\n",
			       __func__, ret);
			goto EXIT;
		} else {
			ret = clk_set_parent(clk_top_mux_adsp,
					     clk_top_adsppll_ck);
			if (IS_ERR(&ret)) {
				pr_err("[ADSP] %s clk_set_parent(clk_top_mux_adsp) fail %d\n",
				       __func__, ret);
				goto EXIT;
			}
		}
	} else {
		ret = clk_set_parent(clk_top_mux_adsp, clk_adsp_clk26);
		if (IS_ERR(&ret)) {
			pr_err("[ADSP] %s clk_set_parentt(clk_top_mux_adsp) fail %d\n",
			       __func__, ret);

			goto EXIT;
		}
		clk_disable_unprepare(clk_top_mux_adsp);
	}
	pr_debug("%s(%d) done\n", __func__, enable);

EXIT:
	return PTR_ERR(&ret);
}

void set_adsppll_rate(enum adsppll_freq freq)
{
	int ret = 0;

	ret = clk_prepare_enable(clk_top_adsppll_ck);
	if (IS_ERR(&ret)) {
		pr_err("[ADSP] %s clk_prepare_enable(clk_top_mux_adsp) fail %d\n",
		       __func__, ret);
		clk_disable_unprepare(clk_top_adsppll_ck);
	}
	clk_set_rate(clk_top_adsppll_ck, freq);
	clk_disable_unprepare(clk_top_adsppll_ck);

	pr_debug("set adsppll freq = %d done\n", freq);
}

int adsp_enable_clock(void)
{
	int ret = 0;

	ret = clk_prepare_enable(clk_adsp_infra);
	if (IS_ERR(&ret)) {
		pr_err("[ADSP] %s clk_prepare_enable(clk_adsp_infra) fail %d\n",
		       __func__, ret);
		return PTR_ERR(&ret);
	}
	return ret;
}

void adsp_disable_clock(void)
{
	clk_disable_unprepare(clk_adsp_infra);
}

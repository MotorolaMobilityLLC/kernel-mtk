/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/clk.h>
#include "imgsensor_common.h"
#include "imgsensor_clk.h"

extern void mipic_26m_en(int en);

char *gimgsensor_mclk_name[IMGSENSOR_MCLK_MAX_NUM] = {
	"TOP_CAMTG_SEL",
	"TOP_CAMTG2_SEL",
	"TOP_CLK26M",
	"TOP_UNIVPLL_D52",
	"TOP_UNIVPLL2_D8",
	"TOP_UNIVPLL_D26",
	"TOP_UNIVPLL2_D16",
	"TOP_UNIVPLL2_D32",
	"TOP_UNIVPLL_D104",
	"TOP_UNIVPLL_D208",
};

extern struct platform_device* gpimgsensor_hw_platform_device;

static inline void imgsensor_clk_check(struct IMGSENSOR_CLK *pclk)
{
	int i;

	for (i = 0; i < IMGSENSOR_MCLK_MAX_NUM; i++)
		WARN_ON(IS_ERR(pclk->mclk_sel[i]));
}

/*******************************************************************************
* Common Clock Framework (CCF)
********************************************************************************/
enum IMGSENSOR_RETURN imgsensor_clk_init(struct IMGSENSOR_CLK *pclk)
{
	int i;
	struct platform_device *pplatform_dev = gpimgsensor_hw_platform_device;

	if (pplatform_dev == NULL) {
		PK_ERR("[%s] pdev is null\n", __func__);
		return IMGSENSOR_RETURN_ERROR;
	}
	/* get all possible using clocks */
	for (i = 0; i < IMGSENSOR_MCLK_MAX_NUM; i++)
		pclk->mclk_sel[i] = devm_clk_get(&pplatform_dev->dev, gimgsensor_mclk_name[i]);

	return IMGSENSOR_RETURN_SUCCESS;
}

int imgsensor_clk_set(struct IMGSENSOR_CLK *pclk, ACDK_SENSOR_MCLK_STRUCT *pmclk)
{
	int ret = 0;

	if (pmclk->TG > IMGSENSOR_MCLK_TOP_CAMTG2_SEL ||
		pmclk->TG < IMGSENSOR_MCLK_TOP_CAMTG_SEL ||
		pmclk->freq > IMGSENSOR_MCLK_MAX_NUM ||
		pmclk->freq < IMGSENSOR_MCLK_TOP_CLK26M) {
		PK_ERR("[CAMERA SENSOR]kdSetSensorMclk out of range, tg=%d, freq= %d\n",
			pmclk->TG, pmclk->freq);
	}

	PK_DBG("[CAMERA SENSOR] CCF kdSetSensorMclk on= %d, freq= %d, TG= %d\n",
		pmclk->on, pmclk->freq, pmclk->TG);

	imgsensor_clk_check(pclk);

	if (pmclk->on) {

		/* Workaround for timestamp: TG1 always ON */
		if (clk_prepare_enable(pclk->mclk_sel[IMGSENSOR_MCLK_TOP_CAMTG_SEL]))
			PK_ERR("[CAMERA SENSOR] failed tg=%d\n", IMGSENSOR_MCLK_TOP_CAMTG_SEL);
		else
			atomic_inc(&pclk->enable_cnt[IMGSENSOR_MCLK_TOP_CAMTG_SEL]);

		if (clk_prepare_enable(pclk->mclk_sel[pmclk->TG]))
			PK_ERR("[CAMERA SENSOR] failed tg=%d\n", pmclk->TG);
		else
			atomic_inc(&pclk->enable_cnt[pmclk->TG]);

		if (clk_prepare_enable(pclk->mclk_sel[pmclk->freq]))
			PK_ERR("[CAMERA SENSOR] failed freq= %d\n", pmclk->freq);
		else
			atomic_inc(&pclk->enable_cnt[pmclk->freq]);

		ret = clk_set_parent(pclk->mclk_sel[pmclk->TG], pclk->mclk_sel[pmclk->freq]);
	} else {

		/* Workaround for timestamp: TG1 always ON */
		clk_disable_unprepare(pclk->mclk_sel[IMGSENSOR_MCLK_TOP_CAMTG_SEL]);
		atomic_dec(&pclk->enable_cnt[IMGSENSOR_MCLK_TOP_CAMTG_SEL]);

		clk_disable_unprepare(pclk->mclk_sel[pmclk->TG]);
		atomic_dec(&pclk->enable_cnt[pmclk->TG]);
		clk_disable_unprepare(pclk->mclk_sel[pmclk->freq]);
		atomic_dec(&pclk->enable_cnt[pmclk->freq]);
	}

	return ret;
}

void imgsensor_clk_enable_all(struct IMGSENSOR_CLK *pclk)
{
	mipic_26m_en(1);
}

void imgsensor_clk_disable_all(struct IMGSENSOR_CLK *pclk)
{
	int i;

	mipic_26m_en(0);

	for (i = 0; i < IMGSENSOR_MCLK_MAX_NUM; i++) {
		for (; atomic_read(&pclk->enable_cnt[i]) > 0; ) {
			clk_disable_unprepare(pclk->mclk_sel[i]);
			atomic_dec(&pclk->enable_cnt[i]);
		}
	}
}


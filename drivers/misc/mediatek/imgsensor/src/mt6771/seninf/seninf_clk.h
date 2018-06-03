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

#ifndef __SENINF_CLK_H__
#define __SENINF_CLK_H__

#include <linux/atomic.h>
#include <linux/platform_device.h>

#include "kd_imgsensor_define.h"

#include "seninf_common.h"

#define SENINF_USE_CCF

enum SENINF_CLK_IDX_SYS {
	SENINF_CLK_IDX_SYS_MIN_NUM = 0,
	SENINF_CLK_IDX_SYS_SCP_SYS_MM0 = SENINF_CLK_IDX_SYS_MIN_NUM,
	SENINF_CLK_IDX_SYS_SCP_SYS_CAM,
	SENINF_CLK_IDX_SYS_CLK_CAM_SENINF,
	SENINF_CLK_IDX_SYS_MAX_NUM
};

enum SENINF_CLK_IDX_TG {
	SENINF_CLK_IDX_TG_MIN_NUM = SENINF_CLK_IDX_SYS_MAX_NUM,
	SENINF_CLK_IDX_TG_TOP_CAMTG_SEL = SENINF_CLK_IDX_TG_MIN_NUM,
	SENINF_CLK_IDX_TG_TOP_CAMTG2_SEL,
	SENINF_CLK_IDX_TG_TOP_CAMTG3_SEL,
	SENINF_CLK_IDX_TG_TOP_CAMTG4_SEL,
	SENINF_CLK_IDX_TG_MAX_NUM
};

enum SENINF_CLK_IDX_FREQ {
	SENINF_CLK_IDX_FREQ_MIN_NUM = SENINF_CLK_IDX_TG_MAX_NUM,
	SENINF_CLK_IDX_FREQ_TOP_UNIVPLL_D416 = SENINF_CLK_IDX_FREQ_MIN_NUM,
	SENINF_CLK_IDX_FREQ_TOP_UNIVPLL_D208,
	SENINF_CLK_IDX_FREQ_TOP_UNIVPLL_D104,
	SENINF_CLK_IDX_FREQ_CLK26M,
	SENINF_CLK_IDX_FREQ_TOP_UNIVPLL2_D52,
	SENINF_CLK_IDX_FREQ_TOP_UNIVPLL2_D16,
	SENINF_CLK_IDX_FREQ_MAX_NUM
};

enum SENINF_CLK_TG {
	SENINF_CLK_TG_0,
	SENINF_CLK_TG_1,
	SENINF_CLK_TG_2,
	SENINF_CLK_TG_3,
	SENINF_CLK_TG_MAX_NUM
};

enum SENINF_CLK_MCLK_FREQ {
	SENINF_CLK_MCLK_FREQ_6MHZ = 6,
	SENINF_CLK_MCLK_FREQ_12MHZ = 12,
	SENINF_CLK_MCLK_FREQ_24MHZ = 24,
	SENINF_CLK_MCLK_FREQ_26MHZ = 26,
	SENINF_CLK_MCLK_FREQ_48MHZ = 48,
	SENINF_CLK_MCLK_FREQ_52MHZ = 52
};

#define SENINF_CLK_MCLK_FREQ_MAX    SENINF_CLK_MCLK_FREQ_52MHZ
#define SENINF_CLK_MCLK_FREQ_MIN    SENINF_CLK_MCLK_FREQ_6MHZ
#define SENINF_CLK_IDX_MAX_NUM      SENINF_CLK_IDX_FREQ_MAX_NUM
#define SENINF_CLK_IDX_FREQ_IDX_NUM (SENINF_CLK_IDX_FREQ_MAX_NUM - SENINF_CLK_IDX_FREQ_MIN_NUM)

struct SENINF_CLK_CTRL {
	char *pctrl;
};

struct SENINF_CLK {
	struct platform_device *pplatform_device;
	struct clk *mclk_sel[SENINF_CLK_IDX_MAX_NUM];
	atomic_t enable_cnt[SENINF_CLK_IDX_MAX_NUM];
#ifndef SENINF_USE_CCF
	void __iomem *pcamsys_base;
#endif
};

enum SENINF_RETURN seninf_clk_init(struct SENINF_CLK *pclk);
int seninf_clk_set(struct SENINF_CLK *pclk, ACDK_SENSOR_MCLK_STRUCT *pmclk);
void seninf_clk_open(struct SENINF_CLK *pclk);
void seninf_clk_release(struct SENINF_CLK *pclk);

#endif


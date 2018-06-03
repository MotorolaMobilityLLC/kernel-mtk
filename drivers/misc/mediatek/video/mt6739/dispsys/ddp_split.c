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

#define LOG_TAG "SPLIT"
#include "ddp_log.h"

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif

#include <linux/delay.h>

#include "ddp_info.h"
#include "ddp_hal.h"
#include "ddp_reg.h"
#include "disp_helper.h"

static char *split_state(unsigned int state)
{
	switch (state) {
	case 1:
		return "idle";
	case 2:
		return "wait";
	case 4:
		return "busy";
	default:
		return "unknown";
	}
	return "unknown";
}

static int split_clock_on(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
	if (module == DISP_MODULE_SPLIT0) {
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_SPLIT0, "split");
#else
		ddp_clk_enable(DISP0_DISP_SPLIT);
#endif
		DDPMSG("Split0 ClockOn CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	}
#endif
	return 0;
}

static int split_clock_off(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
	DDPMSG("Split0 Clockoff CG\n");
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_DISP0_DISP_SPLIT0, "split");
#else
	ddp_clk_disable(DISP0_DISP_SPLIT);
#endif
#endif
	return 0;
}

static int split_init(enum DISP_MODULE_ENUM module, void *handle)
{
	split_clock_on(module, handle);
	return 0;
}

static int split_deinit(enum DISP_MODULE_ENUM module, void *handle)
{
	split_clock_off(module, handle);
	return 0;
}

static int split_start(enum DISP_MODULE_ENUM module, void *handle)
{
	DISP_REG_SET_FIELD(handle, DEBUG_FLD_PIXEL_CNT_EN, DISP_REG_SPLIT_DEBUG, 1);
	DISP_REG_SET(handle, DISP_REG_SPLIT_ENABLE, 0x01);
	return 0;
}

static int split_stop(enum DISP_MODULE_ENUM module, void *handle)
{
	DISP_REG_SET(handle, DISP_REG_SPLIT_ENABLE, 0x0);
	DISP_REG_SET_FIELD(handle, DEBUG_FLD_PIXEL_CNT_EN, DISP_REG_SPLIT_DEBUG, 0);
	return 0;
}

static int split_config(enum DISP_MODULE_ENUM module, struct disp_ddp_path_config *pConfig, void *handle)
{
	DISP_REG_SET(handle, DISP_REG_SPLIT_CFG, 0x00);/* 0: even pixel first, 1: odd pexel first */

	return 0;
}

static int split_busy(enum DISP_MODULE_ENUM module)
{
	unsigned int state = DISP_REG_GET_FIELD(DEBUG_FLD_SPLIT_FSM, DISP_REG_SPLIT_DEBUG);

	return (state & 0x4);
}

static int split_idle(enum DISP_MODULE_ENUM module)
{
	unsigned int state = DISP_REG_GET_FIELD(DEBUG_FLD_SPLIT_FSM, DISP_REG_SPLIT_DEBUG);

	return (state & 0x3);
}

int split_reset(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned int delay_cnt = 0;

	DISP_REG_SET(handle, DISP_REG_SPLIT_SW_RESET, 0x1);
	DISP_REG_SET(handle, DISP_REG_SPLIT_SW_RESET, 0x0);
	/*always use cpu do reset*/
	while ((DISP_REG_GET_FIELD(DEBUG_FLD_SPLIT_FSM, DISP_REG_SPLIT_DEBUG) & 0x3) == 0) {
		delay_cnt++;
		udelay(10);
		if (delay_cnt > 2000) {
			DDPERR("split0_reset() timeout!\n");
			break;
		}
	}
	return 0;
}

static int split_dump_regs(enum DISP_MODULE_ENUM module)
{
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		DDPDUMP("== START: DISP SPLIT REGS ==\n");
		DDPDUMP("SPLIT: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x00, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x00),
			0x04, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x04),
			0x20, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x20),
			0x60, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x60));
		DDPDUMP("SPLIT: 0x%04x=0x%08x\n",
			0x80, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x80));
		DDPDUMP("-- END: DISP SPLIT REGS --\n");
	} else {
		DDPMSG("== DISP SPLIT0 REGS ==\n");
		DDPMSG("(0x000)S_ENABLE=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_ENABLE));
		DDPMSG("(0x004)S_SW_RST=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_SW_RESET));
		DDPMSG("(0x008)S_DEBUG=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_DEBUG));
	}
	return 0;
}

static int split_dump_analysis(enum DISP_MODULE_ENUM module)
{
	unsigned int pixel = DISP_REG_GET_FIELD(DEBUG_FLD_IN_PIXEL_CNT, DISP_REG_SPLIT_DEBUG);
	unsigned int state = DISP_REG_GET_FIELD(DEBUG_FLD_SPLIT_FSM, DISP_REG_SPLIT_DEBUG);

	DDPMSG("== DISP SPLIT0 ANALYSIS ==\n");
	DDPMSG("cur_pixel %u, state %s\n", pixel, split_state(state));
	return 0;
}

static int split_dump(enum DISP_MODULE_ENUM module, int level)
{
	split_dump_analysis(module);
	split_dump_regs(module);

	return 0;
}

struct DDP_MODULE_DRIVER ddp_driver_split = {
	.init		= split_init,
	.deinit		= split_deinit,
	.config		= split_config,
	.start		= split_start,
	.trigger	= NULL,
	.stop		= split_stop,
	.reset		= split_reset,
	.power_on	= split_clock_on,
	.power_off	= split_clock_off,
	.is_idle	= split_idle,
	.is_busy	= split_busy,
	.dump_info	= split_dump,
	.bypass		= NULL,
	.build_cmdq	= NULL,
	.set_lcm_utils	= NULL,
};

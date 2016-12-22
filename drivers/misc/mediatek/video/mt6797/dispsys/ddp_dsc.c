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

#define LOG_TAG "DSC"
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

static bool dsc_enable;

static int dsc_dump(DISP_MODULE_ENUM module, int level)
{
	u32 i;

	DDPDUMP("== DISP DSC REGS ==\n");
	DDPDUMP("(0x000)DSC_START=0x%x\n", DISP_REG_GET(DISP_REG_DSC_CON));
	DDPDUMP("(0x000)DSC_SLICE_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_DSC_SLICE_W));
	DDPDUMP("(0x000)DSC_SLICE_HIGHT=0x%x\n", DISP_REG_GET(DISP_REG_DSC_SLICE_H));
	DDPDUMP("(0x000)DSC_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_DSC_PIC_W));
	DDPDUMP("(0x000)DSC_HEIGHT=0x%x\n", DISP_REG_GET(DISP_REG_DSC_PIC_H));
	DDPDUMP("-- Start dump dsc registers --\n");
	for (i = 0; i < 204; i += 16) {
		DDPDUMP("DSC+%x: 0x%x 0x%x 0x%x	0x%x\n", i, DISP_REG_GET(DISPSYS_DSC_BASE + i),
				DISP_REG_GET(DISPSYS_DSC_BASE + i + 0x4), DISP_REG_GET(DISPSYS_DSC_BASE + i + 0x8),
				DISP_REG_GET(DISPSYS_DSC_BASE + i + 0xc));
	}
	return 0;
}

static int dsc_init(DISP_MODULE_ENUM module, void *handle)
{
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_DISP0_DISP_UFOE, "ufoe");
#else
	ddp_clk_enable(DISP0_DISP_DSC);
#endif
	DDPMSG("dsc_clock on CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return 0;
}

static int dsc_deinit(DISP_MODULE_ENUM module, void *handle)
{
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_DISP0_DISP_DSC, "ufoe");
#else
	ddp_clk_disable(DISP0_DISP_DSC);
#endif
	DDPMSG("dsc_clock off CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return 0;
}

int dsc_start(DISP_MODULE_ENUM module, void *cmdq)
{
	if (dsc_enable)
		DISP_REG_SET_FIELD(cmdq, START_FLD_DISP_DSC_START, DISP_REG_DSC_CON, 1);
	DDPMSG("dsc_start, dsc_start:0x%x\n", DISP_REG_GET(DISP_REG_DSC_CON));
	return 0;
}

int dsc_stop(DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_REG_SET_FIELD(cmdq_handle, START_FLD_DISP_DSC_START, DISP_REG_DSC_CON, 0);
	DDPMSG("dsc_stop, dsc_stop:0x%x\n", DISP_REG_GET(DISP_REG_DSC_CON));
	return 0;
}

static int dsc_config(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *handle)
{
	unsigned int pic_group_width, slice_width, slice_hight, pic_height_ext_num, slice_group_width;
	unsigned int bit_per_pixel, chrunk_size;
	unsigned int init_delay_limit, init_delay_height_min, init_delay_height;
	/* LCM_DSI_PARAMS *lcm_config = &(pConfig->dispif_config.dsi); */
	LCM_DPI_PARAMS *lcm_config = &(pConfig->dispif_config.dpi);

	if (lcm_config->dsc_enable == 1) {
		DDPMSG("dsc_config, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", pConfig->dst_w, pConfig->dst_h,
			lcm_config->dsc_enable, lcm_config->dsc_params.slice_mode, lcm_config->dsc_params.slice_width,
			lcm_config->dsc_params.slice_hight, lcm_config->dsc_params.bit_per_pixel);

	pic_group_width = (pConfig->dst_w + 2)/3;
	slice_width = lcm_config->dsc_params.slice_width;
	slice_hight = lcm_config->dsc_params.slice_hight;
	pic_height_ext_num = (pConfig->dst_h + slice_hight - 1) / slice_hight;
	slice_group_width = (slice_width + 2)/3;
	bit_per_pixel = lcm_config->dsc_params.bit_per_pixel; /*128=1/3,  196=1/2*/
	chrunk_size = (slice_width*bit_per_pixel/8/16);

	DISP_REG_SET_FIELD(handle, START_FLD_DISP_DSC_UFOE_SEL, DISP_REG_DSC_CON, 1);
	DISP_REG_SET_FIELD(handle, START_FLD_DISP_DSC_BYPASS, DISP_REG_DSC_CON, 0); /*disable BYPASS dsc*/

	DISP_REG_SET_FIELD(handle, CFG_FLD_PIC_WIDTH, DISP_REG_DSC_PIC_W, pConfig->dst_w);
	DISP_REG_SET_FIELD(handle, CFG_FLD_PIC_GROUP_WIDTH_M1, DISP_REG_DSC_PIC_W, (pic_group_width-1));

	DISP_REG_SET_FIELD(handle, CFG_FLD_PIC_HEIGHT_M1, DISP_REG_DSC_PIC_H, (pConfig->dst_h - 1));
	DISP_REG_SET_FIELD(handle, CFG_FLD_PIC_HEIGHT_EXT_M1, DISP_REG_DSC_PIC_H, (pic_height_ext_num*slice_hight - 1));

	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_WIDTH, DISP_REG_DSC_SLICE_W, slice_width);
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_GROUP_WIDTH_M1, DISP_REG_DSC_SLICE_W, (slice_group_width - 1));
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_HEIGHT_M1, DISP_REG_DSC_SLICE_H, (slice_hight - 1));
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_NUM_M1, DISP_REG_DSC_SLICE_H, (pic_height_ext_num - 1));
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_WIDTH_MOD3, DISP_REG_DSC_SLICE_H, (slice_width%3));

	DISP_REG_SET_FIELD(handle, CFG_FLD_CHUNK_SIZE, DISP_REG_DSC_CHUNK_SIZE, chrunk_size);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BUF_SIZE, DISP_REG_DSC_BUF_SIZE, chrunk_size*slice_hight);
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_MODE, DISP_REG_DSC_MODE, (lcm_config->dsc_params.slice_mode));
	DISP_REG_SET_FIELD(handle, CFG_FLD_RGB_SWAP, DISP_REG_DSC_MODE, (lcm_config->dsc_params.rgb_swap));

	init_delay_limit = ((128+(lcm_config->dsc_params.xmit_delay+2)/3)*3 + pConfig->dst_w-1)/pConfig->dst_w;
	init_delay_height_min = (init_delay_limit > 15) ? 15 : init_delay_limit;
	init_delay_height = init_delay_height_min;
	DISP_REG_SET_FIELD(handle, CFG_FLD_INIT_DELAY_HEIGHT, DISP_REG_DSC_MODE, init_delay_height);

	DDPMSG("dsc_config, init delay:0x%x\n", DISP_REG_GET(DISP_REG_DSC_MODE));

	DISP_REG_SET_FIELD(handle, CFG_FLD_CFG, DISP_REG_DSC_CFG,
		(lcm_config->dsc_params.dsc_cfg == 0) ? 0x22 : lcm_config->dsc_params.dsc_cfg);
	DISP_REG_SET_FIELD(handle, CFG_FLD_CKSM_CAL_EN, DISP_REG_DSC_DBG_CON, 1);

#if 0
	DISP_REG_SET_FIELD(handle, CFG_FLD_UP_LINE_BUF_DEPTH, DISP_REG_DSC_PPS0,
		(lcm_config->dsc_params.dsc_line_buf_depth == 0) ? 0x9 : lcm_config->dsc_params.dsc_line_buf_depth);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BIT_PER_CHANNEL, DISP_REG_DSC_PPS0,
		(lcm_config->dsc_params.bit_per_channel == 0) ? 0x8 : lcm_config->dsc_params.bit_per_channel);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BIT_PER_PIXEL, DISP_REG_DSC_PPS0,
		(lcm_config->dsc_params.bit_per_pixel == 0) ? 0x80 : lcm_config->dsc_params.bit_per_pixel);
	DISP_REG_SET_FIELD(handle, CFG_FLD_RCT_ON, DISP_REG_DSC_PPS0,
		(lcm_config->dsc_params.rct_on == 0) ? 1 : lcm_config->dsc_params.rct_on); /* Ning */

	DISP_REG_SET_FIELD(handle, CFG_FLD_BP_ENABLE, DISP_REG_DSC_PPS0, lcm_config->dsc_params.bp_enable);
#else
	DISP_REG_SET_FIELD(handle, CFG_FLD_UP_LINE_BUF_DEPTH, DISP_REG_DSC_PPS0, 0x9);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BIT_PER_CHANNEL, DISP_REG_DSC_PPS0, 0x8);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BIT_PER_PIXEL, DISP_REG_DSC_PPS0, 0x80);
	DISP_REG_SET_FIELD(handle, CFG_FLD_RCT_ON, DISP_REG_DSC_PPS0, 1);
	DISP_REG_SET_FIELD(handle, CFG_FLD_BP_ENABLE, DISP_REG_DSC_PPS0, 0);
#endif
	DISP_REG_SET_FIELD(handle, CFG_FLD_INITIAL_DEC_DELAY, DISP_REG_DSC_PPS1,
		(lcm_config->dsc_params.dec_delay == 0) ? 0x268 : lcm_config->dsc_params.dec_delay);
	DISP_REG_SET_FIELD(handle, CFG_FLD_INITIAL_XMIT_DELAY, DISP_REG_DSC_PPS1,
		(lcm_config->dsc_params.xmit_delay == 0) ? 0x200 : lcm_config->dsc_params.xmit_delay);

	DISP_REG_SET_FIELD(handle, CFG_FLD_INITIAL_SCALE_VALUE, DISP_REG_DSC_PPS2,
		(lcm_config->dsc_params.scale_value == 0) ? 0x20 : lcm_config->dsc_params.scale_value);
	DISP_REG_SET_FIELD(handle, CFG_FLD_SCALE_INCREMENT_INTERVAL, DISP_REG_DSC_PPS2,
		(lcm_config->dsc_params.increment_interval == 0) ? 0x387 : lcm_config->dsc_params.increment_interval);

	DISP_REG_SET_FIELD(handle, CFG_FLD_FIRST_LINE_BPG_OFFSET, DISP_REG_DSC_PPS3,
		(lcm_config->dsc_params.line_bpg_offset == 0) ? 0xc : lcm_config->dsc_params.line_bpg_offset);
	DISP_REG_SET_FIELD(handle, CFG_FLD_SCALE_DECREMENT_INTERVAL, DISP_REG_DSC_PPS3,
		(lcm_config->dsc_params.decrement_interval == 0) ? 0xa : lcm_config->dsc_params.decrement_interval);

	DISP_REG_SET_FIELD(handle, CFG_FLD_NFL_BPG_OFFSET, DISP_REG_DSC_PPS4,
		(lcm_config->dsc_params.nfl_bpg_offset == 0) ? 0x319 : lcm_config->dsc_params.nfl_bpg_offset);
	DISP_REG_SET_FIELD(handle, CFG_FLD_SLICE_BPG_OFFSET, DISP_REG_DSC_PPS4,
		(lcm_config->dsc_params.slice_bpg_offset == 0) ? 0x263 : lcm_config->dsc_params.slice_bpg_offset);

	DISP_REG_SET_FIELD(handle, CFG_FLD_INITIAL_OFFSET, DISP_REG_DSC_PPS5,
		(lcm_config->dsc_params.initial_offset == 0) ? 0x1800 : lcm_config->dsc_params.initial_offset);
	DISP_REG_SET_FIELD(handle, CFG_FLD_FINAL_OFFSET, DISP_REG_DSC_PPS5,
		(lcm_config->dsc_params.final_offset == 0) ? 0x10f0 : lcm_config->dsc_params.final_offset);

	DISP_REG_SET_FIELD(handle, CFG_FLD_FLATNESS_MIN_QP, DISP_REG_DSC_PPS6,
		(lcm_config->dsc_params.flatness_minqp == 0) ? 0x3 : lcm_config->dsc_params.flatness_minqp);
	DISP_REG_SET_FIELD(handle, CFG_FLD_FLATNESS_MAX_QP, DISP_REG_DSC_PPS6,
		(lcm_config->dsc_params.flatness_maxqp == 0) ? 0xc : lcm_config->dsc_params.flatness_maxqp);
	DISP_REG_SET_FIELD(handle, CFG_FLD_RC_MODEL_SIZE, DISP_REG_DSC_PPS6,
		(lcm_config->dsc_params.rc_mode1_size == 0) ? 0x2000 : lcm_config->dsc_params.rc_mode1_size);

	DISP_REG_SET(handle, DISP_REG_DSC_PPS6, 0x20000c03);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS7, 0x330b0b06);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS8, 0x382a1c0e);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS9, 0x69625446);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS10, 0x7b797770);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS11, 0x00007e7d);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS12, 0x00800880);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS13, 0xf8c100a1);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS14, 0xe8e3f0e3);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS15, 0xe103e0e3);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS16, 0xd943e123);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS17, 0xd185d965);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS18, 0xd1a7d1a5);
	DISP_REG_SET(handle, DISP_REG_DSC_PPS19, 0x0000d1ed);

	dsc_enable = true;
	DDPMSG("dsc_config, dsc_config:0x%x\n", DISP_REG_GET(DISP_REG_DSC_CON));

	} else {
		DISP_REG_SET_FIELD(handle, START_FLD_DISP_DSC_BYPASS, DISP_REG_DSC_CON, 1); /*enable BYPASS dsc*/
		dsc_enable = false;
	}

	return 0;
}

static int dsc_clock_on(DISP_MODULE_ENUM module, void *handle)
{
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_DISP0_DISP_DSC_ENGINE, "dsc");
#else
	ddp_clk_enable(DISP0_DISP_DSC);
#endif
	DDPMSG("dsc_clock on CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return 0;
}

static int dsc_clock_off(DISP_MODULE_ENUM module, void *handle)
{
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_DISP0_DISP_DSC_ENGINE, "dsc");
#else
	ddp_clk_disable(DISP0_DISP_DSC);
#endif
	DDPMSG("dsc_clock off CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	return 0;
}

static int dsc_reset(DISP_MODULE_ENUM module, void *handle)
{
	DISP_REG_SET_FIELD(handle, START_FLD_DISP_DSC_SW_RESET, DISP_REG_DSC_CON, 1);
	DISP_REG_SET_FIELD(handle, START_FLD_DISP_DSC_SW_RESET, DISP_REG_DSC_CON, 0);
	DDPMSG("dsc reset done\n");
	return 0;
}

int dsc_bypass(DISP_MODULE_ENUM module, int bypass)
{
	DISP_REG_SET_FIELD(NULL, START_FLD_DISP_DSC_BYPASS, DISP_REG_DSC_CON, 1);
	DDPMSG("dsc bypass done\n");
	return 0;
}


DDP_MODULE_DRIVER ddp_driver_dsc = {

	.init            = dsc_init,
	.deinit          = dsc_deinit,
	.config          = dsc_config,
	.start		 = dsc_start,
	.trigger         = NULL,
	.stop	         = dsc_stop,
	.reset           = dsc_reset,
	.power_on        = dsc_clock_on,
	.power_off       = dsc_clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = dsc_dump,
	.bypass          = dsc_bypass,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
	};

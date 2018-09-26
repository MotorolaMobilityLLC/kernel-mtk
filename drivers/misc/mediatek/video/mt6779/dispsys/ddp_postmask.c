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

#define LOG_TAG "POSTMASK"
#include "ddp_log.h"
#include "ddp_clkmgr.h"
#include <linux/delay.h>

#include "ddp_info.h"
#include "ddp_hal.h"
#include "ddp_reg.h"
#include "ddp_postmask.h"

#include "lcm_drv.h"
#include "disp_helper.h"


#define POSTMASK_MASK_MAX_NUM	96
#define POSTMASK_GRAD_MAX_NUM	192

static bool postmask_enable;
static unsigned int postmask_bg_color = 0xff000000;
static unsigned int postmask_mask_color = 0xff000000;

unsigned long postmask_base_addr(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_POSTMASK:
		return DISPSYS_POSTMASK_BASE;
	default:
		DDP_PR_ERR("invalid postmask module=%d\n", module);
		return -1;
	}

	return 0;
}

int postmask_dump_reg(enum DISP_MODULE_ENUM module)
{
	unsigned int i = 0;
	unsigned int num = 0;
	unsigned long base = postmask_base_addr(module);

	DDPDUMP("== DISP %s REGS ==\n", ddp_get_module_name(module));
	DDPDUMP("0x%08x 0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_STATUS + base),
		DISP_REG_GET(DISP_REG_POSTMASK_INPUT_COUNT + base),
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_ADDR + base),
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_LENGTH + base));
	DDPDUMP("0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_EN + base),
		DISP_REG_GET(DISP_REG_POSTMASK_CFG + base),
		DISP_REG_GET(DISP_REG_POSTMASK_SIZE + base));
	DDPDUMP("0x%08x 0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_FIFO_CTRL + base),
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_GMC_SETTING2 + base),
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_BUF_LOW_TH + base),
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_BUF_HIGH_TH + base));
	DDPDUMP("0x%08x 0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_SRAM_CFG + base),
		DISP_REG_GET(DISP_REG_POSTMASK_MASK_SHIFT + base),
		DISP_REG_GET(DISP_REG_POSTMASK_GRAD_SHIFT + base),
		DISP_REG_GET(DISP_REG_POSTMASK_PAUSE_REGION + base));
	DDPDUMP("0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_BLEND_CFG + base),
		DISP_REG_GET(DISP_REG_POSTMASK_ROI_BGCLR + base),
		DISP_REG_GET(DISP_REG_POSTMASK_MASK_CLR + base));

	num = POSTMASK_MASK_MAX_NUM >> 2;
	for (i = 0; i < num; i += 4) {
		unsigned long l_addr = 0;

		l_addr = DISP_REG_POSTMASK_NUM(i) + base;
		DDPDUMP("0x%08x 0x%08x 0x%08x 0x%08x\n",
			DISP_REG_GET(l_addr),
			DISP_REG_GET(l_addr + 0x4),
			DISP_REG_GET(l_addr + 0x8),
			DISP_REG_GET(l_addr + 0xC));
	}

	num = POSTMASK_GRAD_MAX_NUM >> 2;
	for (i = 0; i < num; i += 4) {
		unsigned long l_addr = 0;

		l_addr = DISP_REG_POSTMASK_GRAD_VAL(i) + base;
		DDPDUMP("0x%08x 0x%08x 0x%08x 0x%08x\n",
			DISP_REG_GET(l_addr),
			DISP_REG_GET(l_addr + 0x4),
			DISP_REG_GET(l_addr + 0x8),
			DISP_REG_GET(l_addr + 0xC));
	}

	return 0;
}

int postmask_dump_analysis(enum DISP_MODULE_ENUM module)
{
	unsigned long baddr = postmask_base_addr(module);

	DDPDUMP("== DISP %s ANALYSIS ==\n", ddp_get_module_name(module));
	DDPDUMP("en=%d,cfg=0x%x,size=(%dx%d)\n",
		DISP_REG_GET(DISP_REG_POSTMASK_EN + baddr) & 0x1,
		DISP_REG_GET(DISP_REG_POSTMASK_CFG + baddr),
		(DISP_REG_GET(DISP_REG_POSTMASK_SIZE + baddr) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_POSTMASK_SIZE + baddr) & 0x1fff);
	DDPDUMP("blend_cfg=0x%x,bg=0x%x,mask=0x%x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_BLEND_CFG + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_ROI_BGCLR + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_MASK_CLR + baddr));
	DDPDUMP("fifo_cfg=%d,gmc=0x%x,threshold=(0x%x,0x%x)\n",
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_FIFO_CTRL + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_GMC_SETTING2 + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_BUF_LOW_TH + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_RDMA_BUF_HIGH_TH + baddr));
	DDPDUMP("mem_addr=0x%x,length=0x%x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_ADDR + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_MEM_LENGTH + baddr));
	DDPDUMP("status=0x%x,cur_pos=0x%x\n",
		DISP_REG_GET(DISP_REG_POSTMASK_STATUS + baddr),
		DISP_REG_GET(DISP_REG_POSTMASK_INPUT_COUNT + baddr));

	return 0;
}

static int postmask_dump(enum DISP_MODULE_ENUM module, int level)
{
	postmask_dump_analysis(module);
	postmask_dump_reg(module);

	return 0;
}

static int postmask_clock_on(enum DISP_MODULE_ENUM module, void *handle)
{
	ddp_clk_enable_by_module(module);

	DDPMSG("%s CG:%d(0x%x)\n", __func__,
		(DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1) >> 14) & 0x1,
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));

	return 0;
}

static int postmask_clock_off(enum DISP_MODULE_ENUM module, void *handle)
{
	ddp_clk_disable_by_module(module);

	DDPMSG("%s CG:%d(0x%x)\n", __func__,
		(DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1) >> 14) & 0x1,
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));

	return 0;
}

static int postmask_init(enum DISP_MODULE_ENUM module, void *handle)
{
	return postmask_clock_on(module, handle);
}

static int postmask_deinit(enum DISP_MODULE_ENUM module, void *handle)
{
	return postmask_clock_off(module, handle);
}

int postmask_start(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long base_addr = postmask_base_addr(module);

	if (postmask_enable) {
		DISP_REG_SET_FIELD(handle, EN_FLD_POSTMASK_EN,
					base_addr + DISP_REG_POSTMASK_EN, 1);

		DISP_REG_SET(handle, base_addr + DISP_REG_POSTMASK_INTEN,
			REG_FLD_VAL(PM_INTEN_FLD_PM_IF_FME_END_INTEN, 1) |
			REG_FLD_VAL(PM_INTEN_FLD_PM_FME_CPL_INTEN, 1) |
			REG_FLD_VAL(PM_INTEN_FLD_PM_START_INTEN, 1) |
			REG_FLD_VAL(PM_INTEN_FLD_PM_ABNORMAL_SOF_INTEN, 1) |
			REG_FLD_VAL(PM_INTEN_FLD_RDMA_FME_UND_INTEN, 1) |
			REG_FLD_VAL(PM_INTEN_FLD_RDMA_EOF_ABNORMAL_INTEN, 1));
	}

	DDPMSG("%s en:%d(0x%x)\n", __func__,
		DISP_REG_GET(base_addr + DISP_REG_POSTMASK_EN) & 0x1,
		DISP_REG_GET(base_addr + DISP_REG_POSTMASK_EN));

	return 0;
}

int postmask_stop(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long base_addr = postmask_base_addr(module);

	DISP_REG_SET(handle, base_addr + DISP_REG_POSTMASK_INTEN, 0);
	DISP_REG_SET_FIELD(handle, EN_FLD_POSTMASK_EN,
				base_addr + DISP_REG_POSTMASK_EN, 0);
	DISP_REG_SET(handle, base_addr + DISP_REG_POSTMASK_INTSTA, 0);

	DDPMSG("%s en:%d(0x%x)\n", __func__,
		DISP_REG_GET(base_addr + DISP_REG_POSTMASK_EN) & 0x1,
		DISP_REG_GET(base_addr + DISP_REG_POSTMASK_EN));

	return 0;
}

static int postmask_config(enum DISP_MODULE_ENUM module,
				struct disp_ddp_path_config *pConfig,
				void *handle)
{
	unsigned long base_addr = postmask_base_addr(module);
	unsigned int value = 0;
	unsigned int rc_mode =
			disp_helper_get_option(DISP_OPT_ROUND_CORNER_MODE);
	struct LCM_PARAMS *lcm_param = &(pConfig->dispif_config);


	if (!pConfig->dst_dirty)
		return 0;

	DDPMSG("%s size:(%d,%d), mode:%d, pattern:(%d,%d), mem:(0x%p,%d)\n",
		   __func__, pConfig->dst_w, pConfig->dst_h,
		   disp_helper_get_option(DISP_OPT_ROUND_CORNER_MODE),
		   lcm_param->corner_pattern_width,
		   lcm_param->corner_pattern_height,
		   lcm_param->round_corner_params.lt_addr,
		   lcm_param->round_corner_params.tp_size);

	if (lcm_param->round_corner_en == 1) {
		if (rc_mode != DISP_HELPER_HW_RC) {
			DDP_PR_ERR("unsupport round corner mode:%d\n", rc_mode);
			return -1;
		}

		value = (REG_FLD_VAL((PM_CFG_FLD_RELAY_MODE), 0) |
			 REG_FLD_VAL((PM_CFG_FLD_DRAM_MODE), 1) |
			 REG_FLD_VAL((PM_CFG_FLD_BGCLR_IN_SEL), 0));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_CFG + base_addr, value);

		value = (REG_FLD_VAL((PM_BLEND_CFG_FLD_A_EN), 1) |
			 REG_FLD_VAL((PM_BLEND_CFG_FLD_PARGB_BLD), 0) |
			 REG_FLD_VAL((PM_BLEND_CFG_FLD_CONST_BLD), 0));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_BLEND_CFG +
			 base_addr, value);

		DISP_REG_SET(handle, DISP_REG_POSTMASK_ROI_BGCLR +
			 base_addr, postmask_bg_color);
		DISP_REG_SET(handle, DISP_REG_POSTMASK_MASK_CLR +
			 base_addr, postmask_mask_color);

		value = (REG_FLD_VAL((PM_SIZE_FLD_HSIZE), pConfig->dst_w) |
			 REG_FLD_VAL((PM_SIZE_FLD_VSIZE), pConfig->dst_h));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_SIZE + base_addr, value);

		value = (REG_FLD_VAL((PM_PAUSE_REGION_FLD_RDMA_PAUSE_START),
			 lcm_param->corner_pattern_height) |
			 REG_FLD_VAL((PM_PAUSE_REGION_FLD_RDMA_PAUSE_END),
			 pConfig->dst_h - lcm_param->corner_pattern_height));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_PAUSE_REGION +
			 base_addr, value);

		DISP_REG_SET(handle, DISP_REG_POSTMASK_MEM_ADDR +
			 base_addr,
			 virt_to_phys(lcm_param->round_corner_params.lt_addr));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_MEM_LENGTH +
			 base_addr,
			 lcm_param->round_corner_params.tp_size);

		DDPMSG("%s cfg:0x%x\n", __func__,
			 DISP_REG_GET(DISP_REG_POSTMASK_CFG + base_addr));
		postmask_enable = true;
	} else {
		DISP_REG_SET(handle, DISP_REG_POSTMASK_ROI_BGCLR +
			 base_addr, postmask_bg_color);
		DISP_REG_SET(handle, DISP_REG_POSTMASK_MASK_CLR +
			 base_addr, postmask_mask_color);

		value = (REG_FLD_VAL((PM_SIZE_FLD_HSIZE), pConfig->dst_w) |
			 REG_FLD_VAL((PM_SIZE_FLD_VSIZE), pConfig->dst_h));
		DISP_REG_SET(handle, DISP_REG_POSTMASK_SIZE + base_addr, value);

		/*enable BYPASS postmask*/
		DISP_REG_SET_FIELD(handle, PM_CFG_FLD_RELAY_MODE,
			 DISP_REG_POSTMASK_CFG + base_addr, 1);
		DDPMSG("%s cfg:0x%x\n", __func__,
			 DISP_REG_GET(DISP_REG_POSTMASK_CFG + base_addr));
		postmask_enable = false;
	}

	return 0;
}

int postmask_reset(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long base_addr = postmask_base_addr(module);

	DISP_REG_SET_FIELD(handle, RESET_FLD_POSTMASK_RESET,
		base_addr + DISP_REG_POSTMASK_RESET, 1);
	DISP_REG_SET_FIELD(handle, RESET_FLD_POSTMASK_RESET,
		base_addr + DISP_REG_POSTMASK_RESET, 0);
	DDPMSG("%s done\n", __func__);

	return 0;
}

int postmask_bypass(enum DISP_MODULE_ENUM module, int bypass)
{
	unsigned long base_addr = postmask_base_addr(module);

	DISP_REG_SET_FIELD(NULL, PM_CFG_FLD_RELAY_MODE,
		base_addr + DISP_REG_POSTMASK_CFG, 1);
	DDPMSG("%s done\n", __func__);

	return 0;
}

struct DDP_MODULE_DRIVER ddp_driver_postmask = {
	.init            = postmask_init,
	.deinit          = postmask_deinit,
	.config          = postmask_config,
	.start		 = postmask_start,
	.trigger         = NULL,
	.stop	         = postmask_stop,
	.reset           = postmask_reset,
	.power_on        = postmask_clock_on,
	.power_off       = postmask_clock_off,
	.is_idle         = NULL,
	.is_busy         = NULL,
	.dump_info       = postmask_dump,
	.bypass          = postmask_bypass,
	.build_cmdq      = NULL,
	.set_lcm_utils   = NULL,
};

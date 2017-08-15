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

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include "mt-plat/mt_chip.h"
#include "disp_log.h"
#include "primary_display.h"

#include "mt-plat/mt_boot.h"
#include "disp_helper.h"
#include "disp_drv_platform.h"
#include "primary_display.h"

/* use this magic_code to detect memory corruption */
#define MAGIC_CODE 0xDEADAAA0U

/* CONFIG_MTK_FPGA is used in linux kernel for early porting. */
/* if the macro name changed, please modify the code here too. */
#ifdef CONFIG_MTK_FPGA
static unsigned int disp_global_stage = MAGIC_CODE | DISP_HELPER_STAGE_EARLY_PORTING;
#else
/* please change this to DISP_HELPER_STAGE_NORMAL after bring up done */
/*static unsigned int disp_global_stage = MAGIC_CODE | DISP_HELPER_STAGE_BRING_UP;*/
static unsigned int disp_global_stage = MAGIC_CODE | DISP_HELPER_STAGE_NORMAL;
#endif

static int _is_early_porting_stage(void)
{
	return (disp_global_stage & (~MAGIC_CODE)) == DISP_HELPER_STAGE_EARLY_PORTING;
}

static int _is_bringup_stage(void)
{
	return (disp_global_stage & (~MAGIC_CODE)) == DISP_HELPER_STAGE_BRING_UP;
}

static int _is_normal_stage(void)
{
	return (disp_global_stage & (~MAGIC_CODE)) == DISP_HELPER_STAGE_NORMAL;
}

#if 0
static int _disp_helper_option_value[DISP_OPT_NUM] = { 0 };


const char *disp_helper_option_string[DISP_OPT_NUM] = {
	"DISP_OPT_USE_CMDQ",
	"DISP_OPT_USE_M4U",
	"DISP_OPT_MIPITX_ON_CHIP",
	"DISP_OPT_USE_DEVICE_TREE",
	"DISP_OPT_FAKE_LCM_X",
	"DISP_OPT_FAKE_LCM_Y",
	"DISP_OPT_FAKE_LCM_WIDTH",
	"DISP_OPT_FAKE_LCM_HEIGHT",
	"DISP_OPT_OVL_WARM_RESET",
	"DISP_OPT_DYNAMIC_SWITCH_UNDERFLOW_EN",
	/* Begin: lowpower option*/
	"DISP_OPT_SODI_SUPPORT",
	"DISP_OPT_IDLE_MGR",
	"DISP_OPT_IDLEMGR_SWTCH_DECOUPLE",
	"DISP_OPT_IDLEMGR_ENTER_ULPS",
	"DISP_OPT_SHARE_SRAM",
	"DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK",
	"DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING",
	"DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ",
	"DISP_OPT_MET_LOG", /* for met */
	/* End: lowpower option */
	"DISP_OPT_DECOUPLE_MODE_USE_RGB565",
	"DISP_OPT_NO_LCM_FOR_LOW_POWER_MEASUREMENT",
	"DISP_OPT_NO_LK",
	"DISP_OPT_BYPASS_PQ",
	"DISP_OPT_ESD_CHECK_RECOVERY",
	"DISP_OPT_ESD_CHECK_SWITCH",
	"DISP_OPT_PRESENT_FENCE",
	"DISP_OPT_PERFORMANCE_DEBUG",
	"DISP_OPT_SWITCH_DST_MODE",
	"DISP_OPT_MUTEX_EOF_EN_FOR_CMD_MODE",
	"DISP_OPT_SCREEN_CAP_FROM_DITHER",
	"DISP_OPT_BYPASS_OVL",
	"DISP_OPT_FPS_CALC_WND",
	"DISP_OPT_SMART_OVL",
	"DISP_OPT_DYNAMIC_DEBUG",
	"DISP_OPT_SHOW_VISUAL_DEBUG_INFO",
	"DISP_OPT_RDMA_UNDERFLOW_AEE",
	"DISP_OPT_GMO_OPTIMIZE",
	"DISP_OPT_CV_BYSUSPEND",
	"DISP_OPT_HRT",
};
#endif

#define OPT_COUNT 44
const int help_info_cnt = OPT_COUNT;

struct disp_help_info help_info[OPT_COUNT] = {
	{ DISP_OPT_USE_CMDQ, 1, "DISP_OPT_USE_CMDQ" },
	{ DISP_OPT_USE_M4U, 1, "DISP_OPT_USE_M4U" },
	{ DISP_OPT_MIPITX_ON_CHIP, 0, "DISP_OPT_MIPITX_ON_CHIP" },
	{ DISP_OPT_USE_DEVICE_TREE, 0, "DISP_OPT_USE_DEVICE_TREE" },
	{ DISP_OPT_FAKE_LCM_X, 0, "DISP_OPT_FAKE_LCM_X" },
	{ DISP_OPT_FAKE_LCM_Y, 0, "DISP_OPT_FAKE_LCM_Y" },
	{ DISP_OPT_FAKE_LCM_WIDTH, 0, "DISP_OPT_FAKE_LCM_WIDTH" },
	{ DISP_OPT_FAKE_LCM_HEIGHT, 0, "DISP_OPT_FAKE_LCM_HEIGHT" },
	{ DISP_OPT_OVL_WARM_RESET, 0, "DISP_OPT_OVL_WARM_RESET" },
	{ DISP_OPT_DYNAMIC_SWITCH_UNDERFLOW_EN, 0, "DISP_OPT_DYNAMIC_SWITCH_UNDERFLOW_EN" },
	{ DISP_OPT_SODI_SUPPORT, 1, "DISP_OPT_SODI_SUPPORT" },
	{ DISP_OPT_IDLE_MGR, 1, "DISP_OPT_IDLE_MGR" },
	{ DISP_OPT_IDLEMGR_SWTCH_DECOUPLE, 1, "DISP_OPT_IDLEMGR_SWTCH_DECOUPLE" },
	{ DISP_OPT_IDLEMGR_ENTER_ULPS, 1, "DISP_OPT_IDLEMGR_ENTER_ULPS" },
	{ DISP_OPT_SHARE_SRAM, 1, "DISP_OPT_SHARE_SRAM" },
	{ DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK, 0, "DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK" },
	{ DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING, 1, "DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING" },
	{ DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ, 1, "DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ" },
	{ DISP_OPT_MET_LOG, 1, "DISP_OPT_MET_LOG" },
	{ DISP_OPT_DECOUPLE_MODE_USE_RGB565, 0, "DISP_OPT_DECOUPLE_MODE_USE_RGB565" },
	{ DISP_OPT_NO_LCM_FOR_LOW_POWER_MEASUREMENT, 0, "DISP_OPT_NO_LCM_FOR_LOW_POWER_MEASUREMENT" },
	{ DISP_OPT_NO_LK, 0, "DISP_OPT_NO_LK" },
	{ DISP_OPT_BYPASS_PQ, 0, "DISP_OPT_BYPASS_PQ" },
	{ DISP_OPT_ESD_CHECK_RECOVERY, 1, "DISP_OPT_ESD_CHECK_RECOVERY" },
	{ DISP_OPT_ESD_CHECK_SWITCH, 1, "DISP_OPT_ESD_CHECK_SWITCH" },
	{ DISP_OPT_PRESENT_FENCE, 1, "DISP_OPT_PRESENT_FENCE" },
	{ DISP_OPT_PERFORMANCE_DEBUG, 0, "DISP_OPT_PERFORMANCE_DEBUG" },
	{ DISP_OPT_SWITCH_DST_MODE, 0, "DISP_OPT_SWITCH_DST_MODE" },
	{ DISP_OPT_MUTEX_EOF_EN_FOR_CMD_MODE, 1, "DISP_OPT_MUTEX_EOF_EN_FOR_CMD_MODE" },
	{ DISP_OPT_SCREEN_CAP_FROM_DITHER, 0, "DISP_OPT_SCREEN_CAP_FROM_DITHER" },
	{ DISP_OPT_BYPASS_OVL, 1, "DISP_OPT_BYPASS_OVL" },
	{ DISP_OPT_FPS_CALC_WND, 10, "DISP_OPT_FPS_CALC_WND" },
	{ DISP_OPT_SMART_OVL, 1, "DISP_OPT_SMART_OVL" },
	{ DISP_OPT_DYNAMIC_DEBUG, 0, "DISP_OPT_DYNAMIC_DEBUG" },
	{ DISP_OPT_SHOW_VISUAL_DEBUG_INFO, 0, "DISP_OPT_SHOW_VISUAL_DEBUG_INFO" },
	{ DISP_OPT_RDMA_UNDERFLOW_AEE, 0, "DISP_OPT_RDMA_UNDERFLOW_AEE" },
	{ DISP_OPT_GMO_OPTIMIZE, 0, "DISP_OPT_GMO_OPTIMIZE" },
	{ DISP_OPT_CV_BYSUSPEND, 1, "DISP_OPT_CV_BYSUSPEND" },
	{ DISP_OPT_DETECT_RECOVERY, 0, "DISP_OPT_DETECT_RECOVERY" },
	{ DISP_OPT_DELAYED_TRIGGER, 1, "DISP_OPT_DELAYED_TRIGGER" },
	{ DISP_OPT_OVL_EXT_LAYER, 0, "DISP_OPT_OVL_EXT_LAYER" },
	{ DISP_OPT_RSZ, 0, "DISP_OPT_RSZ" },
	{ DISP_OPT_DUAL_PIPE, 0, "DISP_OPT_DUAL_PIPE" },
	{ DISP_OPT_HRT, 1, "DISP_OPT_HRT" },
};

static int option_to_index(DISP_HELPER_OPT option)
{
	int index = -1;
	int i = 0;

	for (i = 0; i < help_info_cnt; i++) {
		if (option == help_info[i].opt_enum) {
			index = i;
			break;
		}
	}
	return index;
}

const char *disp_helper_option_spy(DISP_HELPER_OPT option)
{
	int index = -1;

	index = option_to_index(option);
	if (index < 0)
		return "unknown option!!";
	return help_info[index].opt_string;
}

DISP_HELPER_OPT disp_helper_name_to_opt(const char *name)
{
	DISP_HELPER_OPT opt = -1;
	int i = 0;

	for (i = 0; i < help_info_cnt; i++) {
		const char *opt_name = help_info[i].opt_string;

		if (strncmp(name, opt_name, strlen(opt_name)) == 0) {
			opt = help_info[i].opt_enum;
			break;
		}
	}
	if (opt == -1)
		pr_err("%s: unknown name: %s\n", __func__, name);
	return opt;
}

int disp_helper_set_option(DISP_HELPER_OPT option, int value)
{
	int index = -1;
	int ret = 0;

	if (option == DISP_OPT_FPS_CALC_WND) {
		/*ret = primary_fps_ctx_set_wnd_sz(value);*/
		if (ret) {
			DISPERR("%s error to set fps_wnd_sz to %d\n", __func__, value);
			return ret;
		}
	}

	index = option_to_index(option);
	if (index >= 0) {
		DISPMSG("Set Option %d(%s) from (%d) to (%d)\n", option,
			  help_info[index].opt_string, help_info[index].opt_value, value);
		help_info[index].opt_value = value;
		DISPMSG("After set (%s) is (%d)\n", help_info[index].opt_string,
			  help_info[index].opt_value);
		ret = 0;
	} else {
		DISPERR("Wrong option: %d\n", option);
		ret = -1;
	}
	return ret;
}

int disp_helper_set_option_by_name(const char *name, int value)
{
	DISP_HELPER_OPT opt;

	opt = disp_helper_name_to_opt(name);
	if (opt < 0)
		return -1;

	return disp_helper_set_option(opt, value);
}

int disp_helper_get_option(DISP_HELPER_OPT option)
{
	int index = -1;
	int ret = 0;

	index = option_to_index(option);
	if (index < 0) {
		DISPERR("%s: option invalid %d\n", __func__, option);
		BUG();
	}

	switch (option) {
	case DISP_OPT_USE_CMDQ:
	case DISP_OPT_SHOW_VISUAL_DEBUG_INFO:
		{
			return help_info[index].opt_value;
		}
	case DISP_OPT_USE_M4U:
		{
			return help_info[index].opt_value;
		}
	case DISP_OPT_MIPITX_ON_CHIP:
		{
			if (_is_normal_stage())
				return 1;
			else if (_is_bringup_stage())
				return 1;
			else if (_is_early_porting_stage())
				return 0;

			BUG_ON(1);
			return 0;
		}
	case DISP_OPT_FAKE_LCM_X:
		{
			int x = 0;
#ifdef CONFIG_CUSTOM_LCM_X
			ret = kstrtoint(CONFIG_CUSTOM_LCM_X, 0, &x);
			if (ret) {
				pr_err("%s error to parse x: %s\n", __func__, CONFIG_CUSTOM_LCM_X);
				x = 0;
			}
#endif
			return x;
		}
	case DISP_OPT_FAKE_LCM_Y:
		{
			int y = 0;
#ifdef CONFIG_CUSTOM_LCM_Y
			ret = kstrtoint(CONFIG_CUSTOM_LCM_Y, 0, &y);
			if (ret) {
				pr_err("%s error to parse x: %s\n", __func__, CONFIG_CUSTOM_LCM_Y);
				y = 0;
			}

#endif
			return y;
		}
	case DISP_OPT_FAKE_LCM_WIDTH:
		{
			int w = primary_display_get_virtual_width();

			if (w == 0)
				w = DISP_GetScreenWidth();
			return w;
		}
	case DISP_OPT_FAKE_LCM_HEIGHT:
		{
			int h = primary_display_get_virtual_height();

			if (h == 0)
				h = DISP_GetScreenHeight();
			return h;
		}
	case DISP_OPT_OVL_WARM_RESET:
		{
			return 0;
		}
	case DISP_OPT_NO_LK:
		{
			if (_is_normal_stage())
				return 0;
			else if (_is_bringup_stage())
				return 0;
			return 1;
		}
	case DISP_OPT_PERFORMANCE_DEBUG:
		{
			if (_is_normal_stage())
				return 0;
			else if (_is_bringup_stage())
				return 0;
			else if (_is_early_porting_stage())
				return 0;
			BUG_ON(1);
		}
	case DISP_OPT_SWITCH_DST_MODE:
		{
			if (_is_normal_stage())
				return 0;
			else if (_is_bringup_stage())
				return 0;
			else if (_is_early_porting_stage())
				return 0;
			BUG_ON(1);
		}
	default:
		{
			return help_info[index].opt_value;
		}
	}

	return	ret;
}

DISP_HELPER_STAGE disp_helper_get_stage(void)
{
	return disp_global_stage & (~MAGIC_CODE);
}

const char *disp_helper_stage_spy(void)
{
	if (disp_helper_get_stage() == DISP_HELPER_STAGE_EARLY_PORTING)
		return "EARLY_PORTING";
	else if (disp_helper_get_stage() == DISP_HELPER_STAGE_BRING_UP)
		return "BRINGUP";
	else if (disp_helper_get_stage() == DISP_HELPER_STAGE_NORMAL)
		return "NORMAL";
	return NULL;
}

void disp_helper_option_init(void)
{
	disp_helper_set_option(DISP_OPT_USE_CMDQ, 1);
	disp_helper_set_option(DISP_OPT_USE_M4U, 1);

	/* test solution for 6795 rdma underflow caused by ufoe LR mode(ufoe fifo is larger than rdma) */
	disp_helper_set_option(DISP_OPT_DYNAMIC_SWITCH_UNDERFLOW_EN, 0);

	/* warm reset ovl before each trigger for cmd mode */
	disp_helper_set_option(DISP_OPT_OVL_WARM_RESET, 0);

	/* ===================Begin: lowpower option setting==================== */
	disp_helper_set_option(DISP_OPT_SODI_SUPPORT, 1);
	disp_helper_set_option(DISP_OPT_IDLE_MGR, 1);

	/* 1. vdo mode + screen idle(need idlemgr) */
	disp_helper_set_option(DISP_OPT_IDLEMGR_SWTCH_DECOUPLE, 1);
	disp_helper_set_option(DISP_OPT_SHARE_SRAM, 1);
	disp_helper_set_option(DISP_OPT_IDLEMGR_DISABLE_ROUTINE_IRQ, 1);

	/* 2. cmd mode + screen idle(need idlemgr) */
	disp_helper_set_option(DISP_OPT_IDLEMGR_ENTER_ULPS, 1);

	/* 3. cmd mode + vdo mode */
	disp_helper_set_option(DISP_OPT_DYNAMIC_SWITCH_MMSYSCLK, 0);
	disp_helper_set_option(DISP_OPT_DYNAMIC_RDMA_GOLDEN_SETTING, 1);


	disp_helper_set_option(DISP_OPT_MET_LOG, 1);
	/* ===================End: lowpower option setting==================== */

	disp_helper_set_option(DISP_OPT_PRESENT_FENCE, 1);

	/* use fake vsync timer for low power measurement */
	disp_helper_set_option(DISP_OPT_NO_LCM_FOR_LOW_POWER_MEASUREMENT, 0);

	/* use RGB565 format for decouple mode intermediate buffer */
	disp_helper_set_option(DISP_OPT_DECOUPLE_MODE_USE_RGB565, 0);

	disp_helper_set_option(DISP_OPT_BYPASS_PQ, 0);
	disp_helper_set_option(DISP_OPT_MUTEX_EOF_EN_FOR_CMD_MODE, 1);
	disp_helper_set_option(DISP_OPT_ESD_CHECK_RECOVERY, 1);
	disp_helper_set_option(DISP_OPT_ESD_CHECK_SWITCH, 1);

	disp_helper_set_option(DISP_OPT_BYPASS_OVL, 0);
	disp_helper_set_option(DISP_OPT_FPS_CALC_WND, 10);
	disp_helper_set_option(DISP_OPT_SMART_OVL, 1);
	disp_helper_set_option(DISP_OPT_GMO_OPTIMIZE, 0);
	disp_helper_set_option(DISP_OPT_CV_BYSUSPEND, 1);
	disp_helper_set_option(DISP_OPT_DYNAMIC_DEBUG, 0);
	disp_helper_set_option(DISP_OPT_DELAYED_TRIGGER, 1);
	/*Detect Hang thread Option*/
	disp_helper_set_option(DISP_OPT_DETECT_RECOVERY, 0);
	disp_helper_set_option(DISP_OPT_HRT, 1);
}

int disp_helper_get_option_list(char *stringbuf, int buf_len)
{
	int len = 0;
	int i = 0;

	for (i = 0; i < help_info_cnt; i++) {
		len +=
		    scnprintf(stringbuf + len, buf_len - len, "Option: [%d][%s] Value: [%d]\n", i,
			      help_info[i].opt_string, help_info[i].opt_value);
	}

	return len;
}

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
/* #include <linux/rtpm_prio.h> */
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include "disp_log.h"

/* #include "mt_boot.h" */
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
static unsigned int disp_global_stage = MAGIC_CODE | DISP_HELPER_STAGE_NORMAL;
#endif

static int _is_early_porting_stage(void)
{
	return (disp_global_stage&(~MAGIC_CODE)) == DISP_HELPER_STAGE_EARLY_PORTING;
}

static int _is_bringup_stage(void)
{
	return (disp_global_stage&(~MAGIC_CODE)) == DISP_HELPER_STAGE_BRING_UP;
}

static int _is_normal_stage(void)
{
	return (disp_global_stage&(~MAGIC_CODE)) == DISP_HELPER_STAGE_NORMAL;
}

static int screen_idle_switch_decouple = 1;
void enable_screen_idle_switch_decouple(void)
{
	screen_idle_switch_decouple = 1;
}
void disable_screen_idle_switch_decouple(void)
{
	screen_idle_switch_decouple = 0;
}

/* FIXME: should include header file here. */
/* extern UINT32 DISP_GetScreenWidth(void); */
/* extern UINT32 DISP_GetScreenHeight(void); */

static int _disp_helper_option_value[DISP_HELPER_OPTION_NUM] = {0};

const char *disp_helper_option_spy(DISP_HELPER_OPTION option)
{
	switch (option) {
	case DISP_HELPER_OPTION_USE_CMDQ:
		return "DISP_HELPER_OPTION_USE_CMDQ";
	case DISP_HELPER_OPTION_USE_M4U:
		return "DISP_HELPER_OPTION_USE_M4U";
	case DISP_HELPER_OPTION_USE_CLKMGR:
		return "DISP_HELPER_OPTION_USE_CLKMGR";
	case DISP_HELPER_OPTION_MIPITX_ON_CHIP:
		return "DISP_HELPER_OPTION_MIPITX_ON_CHIP";
	case DISP_HELPER_OPTION_USE_DEVICE_TREE:
		return "DISP_HELPER_OPTION_USE_DEVICE_TREE";
	case DISP_HELPER_OPTION_FAKE_LCM_X:
		return "DISP_HELPER_OPTION_FAKE_LCM_X";
	case DISP_HELPER_OPTION_FAKE_LCM_Y:
		return "DISP_HELPER_OPTION_FAKE_LCM_Y";
	case DISP_HELPER_OPTION_FAKE_LCM_WIDTH:
		return "DISP_HELPER_OPTION_FAKE_LCM_WIDTH";
	case DISP_HELPER_OPTION_FAKE_LCM_HEIGHT:
		return "DISP_HELPER_OPTION_FAKE_LCM_HEIGHT";
	case DISP_HELPER_OPTION_OVL_WARM_RESET:
		return "DISP_HELPER_OPTION_OVL_WARM_RESET";
	case DISP_HELPER_OPTION_DYNAMIC_SWITCH_UNDERFLOW_EN:
		return "DISP_HELPER_OPTION_DYNAMIC_SWITCH_UNDERFLOW_EN";
	case DISP_HELPER_OPTION_IDLEMGR_SWTCH_DECOUPLE:
		return "DISP_HELPER_OPTION_IDLEMGR_SWTCH_DECOUPLE";
	case DISP_HELPER_OPTION_IDLEMGR_DISABLE_ROUTINE_IRQ:
		return "DISP_HELPER_OPTION_IDLEMGR_DISABLE_ROUTINE_IRQ";
	case DISP_HELPER_OPTION_DECOUPLE_MODE_USE_RGB565:
		return "DISP_HELPER_OPTION_DECOUPLE_MODE_USE_RGB565";
	case DISP_HELPER_OPTION_TWO_PIPE_INTERFACE_PATH:
		return "DISP_HELPER_OPTION_TWO_PIPE_INTERFACE_PATH";
	case DISP_HELPER_OPTION_NO_LCM_FOR_LOW_POWER_MEASUREMENT:
		return "DISP_HELPER_OPTION_NO_LCM_FOR_LOW_POWER_MEASUREMENT";
	default:
		return "unknown";
	}
}

void disp_helper_set_option(DISP_HELPER_OPTION option, int value)
{
	if (option < DISP_HELPER_OPTION_NUM) {
		DISPMSG("Set Option %d(%s) from (%d) to (%d)\n", option,
			  disp_helper_option_spy(option), disp_helper_get_option(option), value);
		_disp_helper_option_value[option] = value;
		DISPMSG("After set (%s) is (%d)\n", disp_helper_option_spy(option), disp_helper_get_option(option));
	} else {
		DISPERR("Wrong option: %d\n", option);
	}
}

int disp_helper_get_option(DISP_HELPER_OPTION option)
{
	int ret = 0;

	/* DISPMSG("stage=0x%08x\n", disp_global_stage); */
	switch (option) {
	case DISP_HELPER_OPTION_USE_CMDQ:
	{
		if (_is_normal_stage())
			return 1;
		else if (_is_bringup_stage())
			return 0;
		else if (_is_early_porting_stage())
			return 0;

		BUG_ON(1);
	}
	case DISP_HELPER_OPTION_USE_M4U:
	{
		if (_is_normal_stage())
			return 1;
		else if (_is_bringup_stage())
			return 0;
		else if (_is_early_porting_stage())
			return 0;

		BUG_ON(1);
	}
	case DISP_HELPER_OPTION_USE_CLKMGR:
	{
		if (_is_normal_stage())
			return 1;
		else if (_is_bringup_stage())
			return 0;
		else if (_is_early_porting_stage())
			return 0;

		BUG_ON(1);
	}
	case DISP_HELPER_OPTION_MIPITX_ON_CHIP:
	{
		if (_is_normal_stage())
			return 1;
		else if (_is_bringup_stage())
			return 1;
		else if (_is_early_porting_stage())
			return 0;

		BUG_ON(1);
	}
	case DISP_HELPER_OPTION_FAKE_LCM_X:
	{
		unsigned long int x = 0;

#ifdef CONFIG_CUSTOM_LCM_X
		ret = kstrtoul(CONFIG_CUSTOM_LCM_X, 0, &x);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
#endif
		return x;
	}
	case DISP_HELPER_OPTION_FAKE_LCM_Y:
	{
		unsigned long int y = 0;

#ifdef CONFIG_CUSTOM_LCM_Y
		ret = kstrtoul(CONFIG_CUSTOM_LCM_Y, 0, &y);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
#endif
		return y;
	}
	case DISP_HELPER_OPTION_FAKE_LCM_WIDTH:
	{
		unsigned long int x = 0;
		int w = DISP_GetScreenWidth();

#ifdef CONFIG_CUSTOM_LCM_X
		ret = kstrtoul(CONFIG_CUSTOM_LCM_X, 0, &x);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (x != 0)
			w = ALIGN_TO(w, 16);
#endif
		return w;
	}
	case DISP_HELPER_OPTION_FAKE_LCM_HEIGHT:
	{
		int h = DISP_GetScreenHeight();

		return h;
	}
	default:
	{
		return _disp_helper_option_value[option];
	}
	}
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

	return "";
}

void disp_helper_option_init(void)
{
	/* test solution for 6795 rdma underflow caused by ufoe LR mode(ufoe fifo is larger than rdma) */
	disp_helper_set_option(DISP_HELPER_OPTION_DYNAMIC_SWITCH_UNDERFLOW_EN, 0);

	/* warm reset ovl before each trigger for cmd mode */
	disp_helper_set_option(DISP_HELPER_OPTION_OVL_WARM_RESET, 1);

	/* switch to decouple mode for screen idle, only for video mode */
	disp_helper_set_option(DISP_HELPER_OPTION_IDLEMGR_SWTCH_DECOUPLE, 1);

	/* disable routine irq for screen idle */
	disp_helper_set_option(DISP_HELPER_OPTION_IDLEMGR_DISABLE_ROUTINE_IRQ, 1);

	/* use rdma0->dsi0, rdma2->dsi1 for 6795 video mode */
	disp_helper_set_option(DISP_HELPER_OPTION_TWO_PIPE_INTERFACE_PATH, 0);

	/* use fake vsync timer for low power measurement */
	disp_helper_set_option(DISP_HELPER_OPTION_NO_LCM_FOR_LOW_POWER_MEASUREMENT, 0);

	/* use RGB565 format for decouple mode intermediate buffer */
	disp_helper_set_option(DISP_HELPER_OPTION_DECOUPLE_MODE_USE_RGB565, 0);
}

int disp_helper_get_option_list(char *stringbuf, int buf_len)
{
	int len = 0;
	int i = 0;

	for (i = 0; i < DISP_HELPER_OPTION_NUM; i++) {
		DISPINFO("Option: [%s] Value: [%d]\n", disp_helper_option_spy(i), disp_helper_get_option(i));
		len += scnprintf(stringbuf + len, buf_len - len, "Option: [%d][%s] Value: [%d]\n",
				 i, disp_helper_option_spy(i), disp_helper_get_option(i));
	}

	return len;
}

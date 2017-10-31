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

/*
 * @file    mt_clk_buf_ctl.c
 * @brief   Driver for RF clock buffer control
 */
#ifndef __MT_CLK_BUF_CTL_H__
#define __MT_CLK_BUF_CTL_H__

#include <linux/kernel.h>
#include <linux/mutex.h>

enum clk_buf_id {
	CLK_BUF_BB_MD		= 0,
	CLK_BUF_CONN		= 1,
	CLK_BUF_NFC			= 2,
	CLK_BUF_AUDIO		= 3,
	CLK_BUF_INVALID		= 4,
};

typedef enum {
	CLK_BUF_DISABLE	= 0,
	CLOCK_BUFFER_SW_CONTROL	= 1,
	CLOCK_BUFFER_HW_CONTROL	= 2,
} CLK_BUF_STATUS;

typedef enum {
	CLK_BUF_SW_DISABLE = 0,
	CLK_BUF_SW_ENABLE  = 1,
} CLK_BUF_SWCTRL_STATUS_T;
#define CLKBUF_NUM         4

#define STA_CLK_ON      1
#define STA_CLK_OFF     0

bool clk_buf_ctrl(enum clk_buf_id id, bool onoff);
void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status);
bool clk_buf_init(void);

extern struct mutex clk_buf_ctrl_lock;
#endif


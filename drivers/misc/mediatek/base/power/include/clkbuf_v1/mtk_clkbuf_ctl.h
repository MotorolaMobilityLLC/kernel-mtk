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

/**
* @file    mtk_clk_buf_ctl.h
* @brief   Driver for clock buffer control
*
*/
#ifndef __MTK_CLK_BUF_CTL_H__
#define __MTK_CLK_BUF_CTL_H__

#include <linux/kernel.h>
#include <linux/mutex.h>
#ifdef CONFIG_MTK_LEGACY
#include <cust_clk_buf.h>
#endif

#ifndef CONFIG_MTK_LEGACY
typedef enum {
	CLOCK_BUFFER_DISABLE,
	CLOCK_BUFFER_SW_CONTROL,
	CLOCK_BUFFER_HW_CONTROL,
} MTK_CLK_BUF_STATUS;

typedef enum {
	CLK_BUF_DRIVING_CURR_AUTO_K = -1,
	CLK_BUF_DRIVING_CURR_0,
	CLK_BUF_DRIVING_CURR_1,
	CLK_BUF_DRIVING_CURR_2,
	CLK_BUF_DRIVING_CURR_3
} MTK_CLK_BUF_DRIVING_CURR;
#endif

typedef enum {
	CLK_BUF_SW_DISABLE = 0,
	CLK_BUF_SW_ENABLE  = 1,
} CLK_BUF_SWCTRL_STATUS_T;

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
/* clk_buf_id: users of clock buffer */
enum clk_buf_id {
	CLK_BUF_BB_MD		= 0,
	CLK_BUF_CONN,
	CLK_BUF_NFC,
	CLK_BUF_RF,
	CLK_BUF_AUDIO,
	CLK_BUF_CHG,
	CLK_BUF_UFS,
	CLK_BUF_INVALID
};

/* xo_id: clock buffer list */
enum xo_id {
	XO_SOC	= 0,
	XO_WCN,
	XO_NFC,
	XO_CEL,
	XO_AUD,		/* Disabled */
	XO_PD,		/* AUDIO_CHG */
	XO_EXT,		/* UFS */
	XO_NUMBER
};

enum {
	BBLPM_COND_SKIP = 1,
	BBLPM_COND_CEL,
	BBLPM_COND_NFC,
	BBLPM_COND_WCN,
	BBLPM_COND_EXT,
};
#else /* for MT6335 */
/* clk_buf_id: users of clock buffer */
enum clk_buf_id {
	CLK_BUF_BB_MD		= 0,
	CLK_BUF_CONN,
	CLK_BUF_NFC,
	CLK_BUF_RF,
	CLK_BUF_AUDIO,
	CLK_BUF_CHG,
	CLK_BUF_UFS,
	CLK_BUF_INVALID
};

/* xo_id: clock buffer list */
enum xo_id {
	XO_SOC	= 0,
	XO_WCN,
	XO_NFC,
	XO_CEL,
	XO_AUD,		/* Disabled */
	XO_PD,		/* AUDIO_CHG */
	XO_EXT,		/* UFS */
	XO_NUMBER
};

enum {
	BBLPM_COND_SKIP = 1,
	BBLPM_COND_CEL,
	BBLPM_COND_NFC,
	BBLPM_COND_WCN,
	BBLPM_COND_EXT,
};
#endif

#define CLKBUF_NUM      XO_NUMBER

#define STA_CLK_ON      1
#define STA_CLK_OFF     0

/* #define CLKBUF_USE_BBLPM */

int clk_buf_init(void);
bool clk_buf_ctrl(enum clk_buf_id id, bool onoff);
void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status);
void clk_buf_get_rf_drv_curr(void *rf_drv_curr);
void clk_buf_set_by_flightmode(bool is_flightmode_on);
void clk_buf_save_afc_val(unsigned int afcdac);
void clk_buf_write_afcdac(void);
void clk_buf_control_bblpm(bool on);
u32 clk_buf_bblpm_enter_cond(void);
bool is_clk_buf_under_flightmode(void);
bool is_clk_buf_from_pmic(void);

extern struct mutex clk_buf_ctrl_lock;

#endif


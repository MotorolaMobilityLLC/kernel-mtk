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
 * @file    mt_clk_buf_ctl.c
 * @brief   Driver for RF clock buffer control
 *
 */
#ifndef __MT_CLK_BUF_CTL_H__
#define __MT_CLK_BUF_CTL_H__

#define CLK_BUF_BB_GRP                  (0x1<<0)
#define CLK_BUF_BB_GRP_MASK             (0x1<<0)
#define CLK_BUF_BB_GRP_GET(x)           (((x)&CLK_BUF_BB_GRP_MASK)>>0)
#define CLK_BUF_BB_GRP_SET(x)           (((x)&0x1)<<0)
#define CLK_BUF_CONN_GRP                (0x1<<8)
#define CLK_BUF_CONN_GRP_MASK           (0x3<<8)
#define CLK_BUF_CONN_GRP_GET(x)         (((x)&CLK_BUF_CONN_GRP_MASK)>>8)
#define CLK_BUF_CONN_GRP_SET(x)         (((x)&0x3)<<8)
#define CLK_BUF_CONN_SEL                (0x80<<8)
#define CLK_BUF_NFC_GRP                 (0x1<<16)
#define CLK_BUF_NFC_GRP_MASK            (0x3<<16)
#define CLK_BUF_NFC_GRP_GET(x)          (((x)&CLK_BUF_NFC_GRP_MASK)>>16)
#define CLK_BUF_NFC_GRP_SET(x)          (((x)&0x3)<<16)
#define CLK_BUF_NFC_SEL                 (0x80<<16)
#define CLK_BUF_AUDIO_GRP               (0x1<<24)
#define CLK_BUF_AUDIO_GRP_MASK          (0x1<<24)
#define CLK_BUF_AUDIO_GRP_GET(x)        (((x)&CLK_BUF_AUDIO_GRP_MASK)>>24)
#define CLK_BUF_AUDIO_GRP_SET(x)        (((x)&0x1)<<24)
#define CLK_BUF_AUDIO_SEL               (0x80<<24)

enum clk_buf_id {
	CLK_BUF_BB              = 0,
	CLK_BUF_CONN            = CLK_BUF_CONN_GRP_SET(1),
	CLK_BUF_CONNSRC         = CLK_BUF_CONN_GRP_SET(2),
	CLK_BUF_NFC             = CLK_BUF_NFC_GRP_SET(1),
	CLK_BUF_NFCSRC          = CLK_BUF_NFC_GRP_SET(2),
	CLK_BUF_NFC_NFCSRC      = CLK_BUF_NFC_GRP_SET(3),
	CLK_BUF_AUDIO           = CLK_BUF_AUDIO_GRP_SET(1),
};

typedef enum {
	CLK_BUF_HW_SPM          = -1,
	CLK_BUF_SW_DISABLE      = 0,
	CLK_BUF_SW_ENABLE       = 1,
	CLK_BUF_SW_CONNSRC      = 2, /* for RF_CLK_CFG bit 9 */
	CLK_BUF_SW_NFCSRC       = 2, /* for RF_CLK_CFG bit 17 */
	CLK_BUF_SW_NFC_NFCSRC   = 3,
} CLK_BUF_SWCTRL_STATUS_T;

#ifdef __KERNEL__
bool clk_buf_ctrl(enum clk_buf_id id, bool onoff);
void clk_buf_get_swctrl_status(CLK_BUF_SWCTRL_STATUS_T *status);
bool clk_buf_init(void);

#else
void clk_buf_all_on(void);
void clk_buf_all_default(void);
#endif
#endif


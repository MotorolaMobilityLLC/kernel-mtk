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

#ifndef __MT_OTP_H__
#define __MT_OTP_H__


#ifdef __KERNEL__
#include <linux/kernel.h>
#include <mt-plat/sync_write.h>
#endif

#define	TARGET_TEMP		(95)

#define OTP_ENABLE		(1)
#define PID_EN_enable		(1)

#define OTP_TIMER_INTERRUPT	(0)
/**
 * OTP control register PID_CTL0
 */
#define OTP_T_TARGET		(27:16)
#define OTP_INTERRUPT_CLR	(10:10)
#define OTP_INTERRUPT_ST	(9:9)
#define OTP_INTERRUPT_EN	(8:8)
#define OTP_ERROR_SUM_MODE	(7:6)
#define OTP_FIFO_SIZE		(5:4)
#define OTP_ADAPT_SIZE		(3:3)
#define OTP_ERROR_SUM_CLAMP	(2:2)
#define OTP_FIFO_CLEAN		(1:1)
#define OTP_PID_EN		(0:0)

/**
 * OTP control register PID_ADAPT
 */
#define OTP_POS_SUM_THOLD	(31:16)
#define OTP_NEG_ERR_THOLD	(7:4)
#define OTP_POS_ERR_FIFO_SIZEi	(1:0)


/**
 * OTP control register PID_KP
 */
#define OTP_KP_STEP		(31:16)
#define OTP_KP			(15:0)


/**
 * OTP control register PID_KI
 */
#define OTP_KI_STEP		(31:16)
#define OTP_KI			(15:0)


/**
 * OTP control register PID_KD
 */
#define OTP_KD_STEP		(31:16)
#define OTP_KD			(15:0)


/**
 * OTP control register PID_SCORE0
 */
#define OTP_SCORE_S		(31:16)
#define OTP_SCORE_M		(15:0)


/**
 * OTP control register PID_SHT
 */
#define OTP_FREQ_SHT		(20:16)
#define OTP_CALC_SHT		(12:8)
#define OTP_SCORE_SHT		(4:0)


/**
 * OTP control register PID_FREQ0
 */
#define OTP_FREQ_S		(31:16)
#define OTP_FREQ_M		(15:0)



/**
 * OTP control register PID_MUX
 */
#define OTP_NTH_DEBUG_MUX	(23:20)
#define OTP_POS_ERR_FIFO_MUX	(18:16)
#define OTP_ERR_FIFO_MUX	(12:8)
#define OTP_ERROR_SUM_FIFO_MUX	(4:0)

/**
 * OTP register setting
 */
struct OTP_config_data {
	unsigned int T_TARGET;
	unsigned int ERROR_SUM_MODE;
	unsigned int FIFO_SIZE;
	unsigned int ADAPT_MODE;
	unsigned int ERROR_SUM_CLAMP;
	unsigned int FIFO_CLEAN;
	unsigned int PID_EN;
	unsigned int POS_SUM_THOLD;
	unsigned int NEG_ERR_THOLD;
	unsigned int POS_ERR_FIFO_SIZE;
};

struct OTP_debug_data {
	unsigned int INTERRUPT_CLR;
	unsigned int INTERRUPT_ST;
	unsigned int INTERRUPT_EN;
	unsigned int NTH_DEBUG_MUX;
	unsigned int POS_ERR_FIFO_MUX;
	unsigned int ERR_FIFO_MUX;
	unsigned int ERROR_SUM_FIFO_MUX;
};

struct OTP_ctrl_data {
	unsigned int PIDERRMAX;
	unsigned int PIDERRMIN;
	unsigned int KP_STEP;
	unsigned int KP;
	unsigned int KI_STEP;
	unsigned int KI;
	unsigned int KD_STEP;
	unsigned int KD;
};

struct OTP_score_data {
	unsigned int PID_SCORE_S;
	unsigned int PID_SCORE_M;
	unsigned int PID_FREQ_SHT;
	unsigned int PID_CALC_SHT;
	unsigned int PID_SCORE_SHT;
	unsigned int PID_FREQ_S;
	unsigned int PID_FREQ_M;
};

struct OTP_info_data {
	u32 score;
	u32 freq;
	int channel_status;
};

enum otp_policy {
	NORMAL_MODE,
	MANUAL_MODE
		/* CONSERVATIVE_MODE, */
		/* AGGRESSIVE_MODE */
};

typedef struct {
	int data[32];
} TempInfo;

extern int BigOTPISRHandler(void);
extern TempInfo BigOTPGetTemp(void);
extern unsigned int BigOTPGetiDVFSFreqpct(void);
extern unsigned int BigOTPGetFreqpct(void);
extern void BigOTPEnable(void);
extern void BigOTPDisable(void);
extern int BigOTPThermIRQ(int status);
#endif  /* __MT_OTP_H__ */


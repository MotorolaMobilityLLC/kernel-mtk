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

#define __MT_OTP_C__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/cpumask.h>
#include <linux/workqueue.h>

/* project includes */
#include "mt_otp.h"
#include "mt_idvfs.h"
#include "mt_cpufreq.h"

#ifdef CONFIG_OF
	#include <linux/of.h>
	#include <linux/of_irq.h>
	#include <linux/of_address.h>
	#include <linux/of_fdt.h>
	#include <mt-plat/aee.h>
	#if defined(CONFIG_MTK_CLKMGR)
		#include <mach/mt_clkmgr.h>
	#else
		#include <linux/clk.h>
	#endif
#endif

#ifdef __KERNEL__
	#include <mt-plat/mt_chip.h>
	#include <mt-plat/mt_gpio.h>
	#include "mt-plat/upmu_common.h"
	#include "../../../include/mt-plat/mt6797/include/mach/mt_thermal.h"
	#include "mach/mt_ppm_api.h"
	#include "mt_gpufreq.h"
	#include "../../../power/mt6797/mt6311.h"
#else
	#include "mach/mt_ptpslt.h"
	#include "kernel2ctp.h"
	#include "otp.h"
	#include "upmu_common.h"
	#include "mt6311.h"
	#ifndef EARLY_PORTING
		#include "gpu_dvfs.h"
		#include "thermal.h"
		/* #include "gpio_const.h" */
	#endif
#endif

#ifdef __KERNEL__
#define OTP_TAG     "[OTP]"
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define otp_emerg(fmt, args...)     pr_err(ANDROID_LOG_ERROR, OTP_TAG, fmt, ##args)
	#define otp_alert(fmt, args...)     pr_err(ANDROID_LOG_ERROR, OTP_TAG, fmt, ##args)
	#define otp_crit(fmt, args...)      pr_err(ANDROID_LOG_ERROR, OTP_TAG, fmt, ##args)
	#define otp_error(fmt, args...)     pr_err(ANDROID_LOG_ERROR, OTP_TAG, fmt, ##args)
	#define otp_warning(fmt, args...)   pr_warn(ANDROID_LOG_WARN, OTP_TAG, fmt, ##args)
	#define otp_notice(fmt, args...)    pr_info(ANDROID_LOG_INFO, OTP_TAG, fmt, ##args)
	#define otp_info(fmt, args...)      pr_info(ANDROID_LOG_INFO, OTP_TAG, fmt, ##args)
	#define otp_debug(fmt, args...)     pr_debug(ANDROID_LOG_DEBUG, OTP_TAG, fmt, ##args)
#else
	#define otp_emerg(fmt, args...)     pr_emerg(OTP_TAG fmt, ##args)
	#define otp_alert(fmt, args...)     pr_alert(OTP_TAG fmt, ##args)
	#define otp_crit(fmt, args...)      pr_crit(OTP_TAG fmt, ##args)
	#define otp_error(fmt, args...)     pr_err(OTP_TAG fmt, ##args)
	#define otp_warning(fmt, args...)   pr_warn(OTP_TAG fmt, ##args)
	#define otp_notice(fmt, args...)    pr_notice(OTP_TAG fmt, ##args)
	#define otp_info(fmt, args...)      pr_info(OTP_TAG fmt, ##args)
	#define otp_debug(fmt, args...)     pr_debug(OTP_TAG fmt, ##args)
#endif
#endif

#ifdef CONFIG_OF
void __iomem *otp_base; /* 0x10222000 */
#endif

#define OTP_BASEADDR		(otp_base + 0x440)
#define OTP_PID_CTL0		(OTP_BASEADDR + 0x00)
#define OTP_PID_ERRMAX		(OTP_BASEADDR + 0x04)
#define OTP_PID_ERRMIN		(OTP_BASEADDR + 0x08)
#define OTP_PID_ADAPT		(OTP_BASEADDR + 0x0C)
#define OTP_PID_KP		(OTP_BASEADDR + 0x10)
#define OTP_PID_KI		(OTP_BASEADDR + 0x14)
#define OTP_PID_KD		(OTP_BASEADDR + 0x18)
#define OTP_PID_SCORE0		(OTP_BASEADDR + 0x1C)
#define OTP_PID_SHT		(OTP_BASEADDR + 0x20)
#define OTP_PID_FREQ0		(OTP_BASEADDR + 0x24)
#define OTP_PID_DEBUGOUT	(OTP_BASEADDR + 0x28)
#define OTP_PID_MUX		(OTP_BASEADDR + 0x2C)


#undef  BIT
#define BIT(_bit_)		(unsigned)(1 << (_bit_))
#define BITS(_bits_, _val_)	\
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define BITMASK(_bits_)		\
	(((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

#define otp_read(addr) DRV_Reg32(addr)
#define otp_write(addr, val)    mt_reg_sync_writel(val, addr)
#define otp_write_field(addr, range, val)    otp_write(addr, (otp_read(addr) & ~(BITMASK(range))) | BITS(range, val))

#define DEBUG   0
#define DUMP_FULL_LOG   0

#define BIG_CORE_BANK   0
#define MAX_TARGET_TEMP 130
#define MIN_TARGET_TEMP 50

#define BIG_CPU_NUM0	8
#define BIG_CPU_NUM1	9

/* overflow bound */
#define UPPER_BOUND	0x0030D4
#define LOWER_BOUND	0x002CEC

static int dump_debug_log;

struct task_struct *thread;

static void OTPThermHCheck(struct work_struct *);
static void OTPThermLCheck(struct work_struct *);

static struct workqueue_struct *otp_workqueue;
static DECLARE_WORK(therm_H_check, OTPThermHCheck);
static DECLARE_WORK(therm_L_check, OTPThermLCheck);

static struct OTP_config_data otp_config_data;
static struct OTP_debug_data otp_debug_data;
static struct OTP_ctrl_data otp_ctrl_data;
static struct OTP_score_data otp_score_data;

struct OTP_info_data otp_info_data;

static DEFINE_MUTEX(debug_mutex);
static DEFINE_MUTEX(timer_mutex);

#ifdef ENABLE_IDVFS
int otp_enable = OTP_ENABLE;
#else
int otp_enable = 0;
#endif

int MODE = 0;

unsigned int MTS;
unsigned int BTS;

int CurTempPT = 0;
int CurTempPT_next = 0;

int CurTemp;

#if DUMP_FULL_LOG
static unsigned int otp_reg_dump_addr_off[] = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C};
unsigned int otp_reg_dump_data[ARRAY_SIZE(otp_reg_dump_addr_off)];
#endif

#define OTP_INTERVAL		(5LL)
static wait_queue_head_t wq;
static int condition;
static int timer_enabled;
static int sw_channel_status;
static struct hrtimer otp_timer;
unsigned int otp_debug_dump_data[10];
/*
void BigOTPEnable(void);
void BigOTPDisable(void);
int BigOTPSetTempTarget (int TempTarget);
int BigOTPSetTempPolicy (enum otp_policy TempPolicy);
struct TempInfo BigOTPGetTemp f(void)
int BigOTPGetCurrTemp (void);
int BigOTPSetTempUpdate (int TempTarget);
*/

/* otp_config_data */
/* unsigned int t_target = 110; */
unsigned int t_target = TARGET_TEMP;

unsigned int error_sum_mode = 0x0;
unsigned int fifo_size = 0x3;
unsigned int adapt_mode = 0x0;
unsigned int error_sum_clamp = 0x1;
unsigned int fifo_clean = 0x0;
unsigned int pid_en = 0x0;
unsigned int pos_sum_thold = 0x04;
unsigned int neg_err_thold = 0x0;
unsigned int pos_err_fifo_size = 0x3;

/* otp_ctrl_data */
unsigned int piderrmax = 0x00020000;
unsigned int piderrmin = 0xFFFFF060;
unsigned int kp_step = 0x0;
unsigned int kp = 0xFF9C;
unsigned int ki_step = 0x0;
unsigned int ki = 0xFFFE;
unsigned int kd_step = 0x0;
unsigned int kd = 0x0;

/* otp_score_data */
unsigned int pid_score_s = 0x2;
unsigned int pid_score_m = 0x7FFF;
unsigned int pid_freq_sht = 0x0;
unsigned int pid_calc_sht = 0x0;
unsigned int pid_score_sht = 0x2;
unsigned int pid_freq_s = 0x000D;
unsigned int pid_freq_m = 0x0;

/* otp_debug_data */
unsigned int interrupt_clr = 0x0;
unsigned int interrupt_en = 0x1;
unsigned int nth_debug_mux = 0x0;
unsigned int pos_err_fifo_mux = 0x0;
unsigned int err_fifo_mux = 0x0;
unsigned int error_sum_fifo_mux = 0x0;


static void otp_config_data_reset(struct OTP_config_data *otp_config_data)
{
	memset((void *)otp_config_data, 0, sizeof(struct OTP_config_data));
}

static void otp_debug_data_reset(struct OTP_debug_data *otp_debug_data)
{
	memset((void *)otp_debug_data, 0, sizeof(struct OTP_debug_data));
}

static void otp_ctrl_data_reset(struct OTP_ctrl_data *otp_ctrl_data)
{
	memset((void *)otp_ctrl_data, 0, sizeof(struct OTP_ctrl_data));
}

static void otp_score_data_reset(struct OTP_score_data *otp_score_data)
{
	memset((void *)otp_score_data, 0, sizeof(struct OTP_score_data));
}

static void otp_info_data_reset(struct OTP_info_data *otp_info_data)
{
	memset((void *)otp_info_data, 0, sizeof(struct OTP_info_data));
}

int get_piden_status(void)
{
	volatile unsigned int val = 0;

	val = ((otp_read(OTP_PID_CTL0) & 0x1));
	if (val != 0)
		return 1;
	return 0;
}

static int otp_set_T_TARGET(struct OTP_config_data *otp_config_data, unsigned int T_TARGET)
{
	if (T_TARGET & ~(0xFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"11\"\n");
		return -1;
	}
	otp_config_data->T_TARGET = T_TARGET;

	return 0;
}

static int otp_set_ERROR_SUM_MODE(struct OTP_config_data *otp_config_data, unsigned int ERROR_SUM_MODE)
{
	if (ERROR_SUM_MODE & ~(0x3)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}

	otp_config_data->ERROR_SUM_MODE = ERROR_SUM_MODE;

	return 0;
}

static int otp_set_FIFO_SIZE(struct OTP_config_data *otp_config_data, unsigned int FIFO_SIZE)
{
	if (FIFO_SIZE & ~(0x3)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"1\"\n");
		return -1;
	}

	otp_config_data->FIFO_SIZE = FIFO_SIZE;

	return 0;
}

static int otp_set_ADAPT_MODE(struct OTP_config_data *otp_config_data, unsigned int ADAPT_MODE)
{
	if (ADAPT_MODE & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"0\"\n");
		return -1;
	}

	otp_config_data->ADAPT_MODE = ADAPT_MODE;

	return 0;
}

static int otp_set_ERROR_SUM_CLAMP(struct OTP_config_data *otp_config_data, unsigned int ERROR_SUM_CLAMP)
{
	if (ERROR_SUM_CLAMP & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"0\"\n");
		return -1;
	}

	otp_config_data->ERROR_SUM_CLAMP = ERROR_SUM_CLAMP;

	return 0;
}

static int otp_set_FIFO_CLEAN(struct OTP_config_data *otp_config_data, unsigned int FIFO_CLEAN)
{
	if (FIFO_CLEAN & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"0\"\n");
		return -1;
	}

	otp_config_data->FIFO_CLEAN = FIFO_CLEAN;

	return 0;
}


static int otp_set_PID_EN(struct OTP_config_data *otp_config_data, unsigned int PID_EN)
{
	if (PID_EN & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"0\"\n");
		return -1;
	}

	otp_config_data->PID_EN = PID_EN;

	return 0;
}

static int otp_set_PIDERRMAX(struct OTP_ctrl_data *otp_ctrl_data, unsigned long PIDERRMAX)
{
	if (PIDERRMAX & ~(0xFFFFFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"31\"\n");
		return -1;
	}

	otp_ctrl_data->PIDERRMAX = PIDERRMAX;

	return 0;
}

static int otp_set_PIDERRMIN(struct OTP_ctrl_data *otp_ctrl_data, unsigned long PIDERRMIN)
{
	if (PIDERRMIN & ~(0xFFFFFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"31\"\n");
		return -1;
	}

	otp_ctrl_data->PIDERRMIN = PIDERRMIN;

	return 0;
}

static int otp_set_POS_SUM_THOLD(struct OTP_config_data *otp_config_data, unsigned int POS_SUM_THOLD)
{
	if (POS_SUM_THOLD & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_config_data->POS_SUM_THOLD = POS_SUM_THOLD;

	return 0;
}

static int otp_set_NEG_ERR_THOLD(struct OTP_config_data *otp_config_data, unsigned int NEG_ERR_THOLD)
{
	if (NEG_ERR_THOLD & ~(0xF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"3\"\n");
		return -1;
	}

	otp_config_data->NEG_ERR_THOLD = NEG_ERR_THOLD;

	return 0;
}

static int otp_set_POS_ERR_FIFO_SIZE(struct OTP_config_data *otp_config_data, unsigned int POS_ERR_FIFO_SIZE)
{
	if (POS_ERR_FIFO_SIZE & ~(0x3)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"2\"\n");
		return -1;
	}

	otp_config_data->POS_ERR_FIFO_SIZE = POS_ERR_FIFO_SIZE;

	return 0;
}

static int otp_set_KP_STEP(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KP_STEP)
{
	if (KP_STEP & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_ctrl_data->KP_STEP = KP_STEP;

	return 0;
}

static int otp_set_KP(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KP)
{
	if (KP & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_ctrl_data->KP = KP;

	return 0;
}

static int otp_set_KI_STEP(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KI_STEP)
{
	if (KI_STEP & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_ctrl_data->KI_STEP = KI_STEP;

	return 0;
}

static int otp_set_KI(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KI)
{
	if (KI & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_ctrl_data->KI = KI;

	return 0;
}

static int otp_set_KD_STEP(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KD_STEP)
{
	if (KD_STEP&~(0xFFFF)) {
		otp_error("badargument!!argumentshouldbe\"0\"~\"15\"\n");
		return -1;
	}

	otp_ctrl_data->KD_STEP = KD_STEP;

	return 0;
}

static int otp_set_KD(struct OTP_ctrl_data *otp_ctrl_data, unsigned int KD)
{
	if (KD & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_ctrl_data->KD = KD;

	return 0;
}

static int otp_set_PID_SCORE_S(struct OTP_score_data *otp_score_data, unsigned int PID_SCORE_S)
{
	if (PID_SCORE_S & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_score_data->PID_SCORE_S = PID_SCORE_S;

	return 0;
}

static int otp_set_PID_SCORE_M(struct OTP_score_data *otp_score_data, unsigned int PID_SCORE_M)
{
	if (PID_SCORE_M & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_score_data->PID_SCORE_M = PID_SCORE_M;

	return 0;
}

static int otp_set_PID_FREQ_SHT(struct OTP_score_data *otp_score_data, unsigned int PID_FREQ_SHT)
{
	if (PID_FREQ_SHT & ~(0x1F)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"4\"\n");
		return -1;
	}

	otp_score_data->PID_FREQ_SHT = PID_FREQ_SHT;

	return 0;
}

static int otp_set_PID_CALC_SHT(struct OTP_score_data *otp_score_data, unsigned int PID_CALC_SHT)
{
	if (PID_CALC_SHT & ~(0x1F)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"4\"\n");
		return -1;
	}

	otp_score_data->PID_CALC_SHT = PID_CALC_SHT;

	return 0;
}

static int otp_set_PID_SCORE_SHT(struct OTP_score_data *otp_score_data, unsigned int PID_SCORE_SHT)
{
	if (PID_SCORE_SHT & ~(0x1F)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"4\"\n");
		return -1;
	}

	otp_score_data->PID_SCORE_SHT = PID_SCORE_SHT;

	return 0;
}

static int otp_set_PID_FREQ_S(struct OTP_score_data *otp_score_data, unsigned int PID_FREQ_S)
{
	if (PID_FREQ_S & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_score_data->PID_FREQ_S = PID_FREQ_S;

	return 0;
}

static int otp_set_PID_FREQ_M(struct OTP_score_data *otp_score_data, unsigned int PID_FREQ_M)
{
	if (PID_FREQ_M & ~(0xFFFF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_score_data->PID_FREQ_M = PID_FREQ_M;

	return 0;
}

static int otp_set_INTERRUPT_CLR(struct OTP_debug_data *otp_debug_data, unsigned int INTERRUPT_CLR)
{
	if (INTERRUPT_CLR & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_debug_data->INTERRUPT_CLR = INTERRUPT_CLR;

	return 0;
}

static int otp_set_INTERRUPT_EN(struct OTP_debug_data *otp_debug_data, unsigned int INTERRUPT_EN)
{
	if (INTERRUPT_EN & ~(0x1)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_debug_data->INTERRUPT_EN = INTERRUPT_EN;

	return 0;
}

static int otp_set_NTH_DEBUG_MUX(struct OTP_debug_data *otp_debug_data, unsigned int NTH_DEBUG_MUX)
{
	if (NTH_DEBUG_MUX & ~(0xF)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"15\"\n");
		return -1;
	}

	otp_debug_data->NTH_DEBUG_MUX = NTH_DEBUG_MUX;

	return 0;
}

static int otp_set_POS_ERR_FIFO_MUX(struct OTP_debug_data *otp_debug_data, unsigned int POS_ERR_FIFO_MUX)
{
	if (POS_ERR_FIFO_MUX & ~(0x7)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"3\"\n");
		return -1;
	}

	otp_debug_data->POS_ERR_FIFO_MUX = POS_ERR_FIFO_MUX;

	return 0;
}

static int otp_set_ERR_FIFO_MUX(struct OTP_debug_data *otp_debug_data, unsigned int ERR_FIFO_MUX)
{
	if (ERR_FIFO_MUX & ~(0x1F)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"4\"\n");
		return -1;
	}

	otp_debug_data->ERR_FIFO_MUX = ERR_FIFO_MUX;

	return 0;
}

static int otp_set_ERROR_SUM_FIFO_MUX(struct OTP_debug_data *otp_debug_data, unsigned int ERR_FIFO_MUX)
{
	if (ERR_FIFO_MUX & ~(0x1F)) {
		otp_error("bad argument!! argument should be \"0\" ~ \"4\"\n");
		return -1;
	}

	otp_debug_data->ERR_FIFO_MUX = ERR_FIFO_MUX;

	return 0;
}

static void otp_PID_EN_apply(struct OTP_config_data *otp_config_data)
{
	otp_write(OTP_PID_CTL0, ((otp_read(OTP_PID_CTL0) & 0xFFFFFFFE) |
	(otp_config_data->PID_EN & 0x1)));
}

static void otp_T_TARGET_apply(struct OTP_config_data *otp_config_data)
{
	otp_write(OTP_PID_CTL0, ((otp_read(OTP_PID_CTL0) & 0x0000FFFF) |
	((otp_config_data->T_TARGET << 16) & 0xffff0000)));
}

static void otp_ERR_FIFO_MUX_apply(struct OTP_debug_data *otp_debug_data)
{
	otp_write(OTP_PID_MUX, ((otp_read(OTP_PID_MUX) & 0xFFFFE0FF) |
	((otp_debug_data->ERR_FIFO_MUX << 8) & 0x00001F00)));
}

static void otp_NTH_DEBUG_MUX_apply(struct OTP_debug_data *otp_debug_data)
{
	otp_write(OTP_PID_MUX, ((otp_read(OTP_PID_MUX) & 0xFE0FFFFF) |
	((otp_debug_data->NTH_DEBUG_MUX << 20) & 0x01f00000)));
}

static void otp_INTERRUPT_CLR_apply(struct OTP_debug_data *otp_debug_data)
{
	otp_write(OTP_PID_CTL0, ((otp_read(OTP_PID_CTL0) & 0xFFFFFBFF) |
	((otp_debug_data->INTERRUPT_CLR << 10) & 0x00000400)));
}

static void otp_config_data_apply(struct OTP_config_data *otp_config_data)
{
	otp_write(OTP_PID_CTL0, ((otp_config_data->T_TARGET << 16) & 0xffff0000) |
	((otp_config_data->ERROR_SUM_MODE << 6) & 0x000000C0) |
	((otp_config_data->FIFO_SIZE << 4)     & 0x00000030) |
	((otp_config_data->ADAPT_MODE << 3)   & 0x00000008) |
	((otp_config_data->ERROR_SUM_CLAMP << 2) & 0x00000004) |
	((otp_config_data->FIFO_CLEAN << 1)   & 0x00000002));

	otp_write(OTP_PID_ADAPT,
	((otp_config_data->POS_SUM_THOLD << 16)  & 0xffff0000) |
	((otp_config_data->NEG_ERR_THOLD <<  4)  & 0x000000f0) |
	(otp_config_data->POS_ERR_FIFO_SIZE      & 0x3));
}

static void otp_ctrl_data_apply(struct OTP_ctrl_data *otp_ctrl_data)
{

	otp_write(OTP_PID_ERRMAX, otp_ctrl_data->PIDERRMAX);
	otp_write(OTP_PID_ERRMIN, otp_ctrl_data->PIDERRMIN);

	otp_write(OTP_PID_KP,
	((otp_ctrl_data->KP_STEP << 16) & 0xffff0000) |
	(otp_ctrl_data->KP       & 0x0000ffff));

	otp_write(OTP_PID_KI,
	((otp_ctrl_data->KI_STEP << 16) & 0xffff0000) |
	(otp_ctrl_data->KI       & 0x0000ffff));

	otp_write(OTP_PID_KD,
	((otp_ctrl_data->KD_STEP << 16) & 0xffff0000) |
	(otp_ctrl_data->KD       & 0x0000ffff));
}

static void otp_score_data_apply(struct OTP_score_data *otp_score_data)
{
	otp_write(OTP_PID_SCORE0,
	((otp_score_data->PID_SCORE_S << 16) & 0xffff0000) |
	(otp_score_data->PID_SCORE_M     & 0x0000ffff));

	otp_write(OTP_PID_SHT,
	((otp_score_data->PID_FREQ_SHT << 16) & 0x001f0000) |
	((otp_score_data->PID_CALC_SHT <<  8) & 0x00001f00) |
	(otp_score_data->PID_SCORE_SHT       & 0x0000001f));

	otp_write(OTP_PID_FREQ0,
	((otp_score_data->PID_FREQ_S << 16) & 0xffff0000) |
	(otp_score_data->PID_FREQ_M     & 0x0000ffff));
}

static void otp_debug_data_apply(struct OTP_debug_data *otp_debug_data)
{
	otp_write(OTP_PID_CTL0,
	(otp_read(OTP_PID_CTL0) & 0xFFFFFAFF) |
	((otp_debug_data->INTERRUPT_CLR << 10) & 0x00000400)|
	((otp_debug_data->INTERRUPT_EN << 8)  & 0x00000100));

	otp_write(OTP_PID_MUX,
	((otp_debug_data->NTH_DEBUG_MUX << 20)   & 0x01F00000)|
	((otp_debug_data->POS_ERR_FIFO_MUX << 16) & 0x00070000)|
	((otp_debug_data->ERR_FIFO_MUX << 8)    & 0x00001F00)|
	(otp_debug_data->ERROR_SUM_FIFO_MUX    & 0x0000000F));
}

static void set_otp_config_data(
	unsigned int t_target,
	unsigned int error_sum_mode,
	unsigned int fifo_size,
	unsigned int adapt_mode,
	unsigned int error_sum_clamp,
	unsigned int fifo_clean,
	unsigned int pos_sum_thold,
	unsigned int neg_err_thold,
	unsigned int pos_err_fifo_size
)
{

	otp_set_T_TARGET(&otp_config_data, t_target);
	otp_set_ERROR_SUM_MODE(&otp_config_data, error_sum_mode);
	otp_set_FIFO_SIZE(&otp_config_data, fifo_size);
	otp_set_ADAPT_MODE(&otp_config_data, adapt_mode);
	otp_set_ERROR_SUM_CLAMP(&otp_config_data, error_sum_clamp);
	otp_set_FIFO_CLEAN(&otp_config_data, fifo_clean);
	otp_set_POS_SUM_THOLD(&otp_config_data, pos_sum_thold);
	otp_set_NEG_ERR_THOLD(&otp_config_data, neg_err_thold);
	otp_set_POS_ERR_FIFO_SIZE(&otp_config_data, pos_err_fifo_size);

	otp_config_data_apply(&otp_config_data);
}

static void set_otp_ctrl_data(
	unsigned int piderrmax,
	unsigned int piderrmin,
	unsigned int kp_step,
	unsigned int kp,
	unsigned int ki_step,
	unsigned int ki,
	unsigned int kd_step,
	unsigned int kd
)
{
	otp_ctrl_data_reset(&otp_ctrl_data);
	otp_set_PIDERRMAX(&otp_ctrl_data, piderrmax);
	otp_set_PIDERRMIN(&otp_ctrl_data, piderrmin);
	otp_set_KP_STEP(&otp_ctrl_data, kp_step);
	otp_set_KP(&otp_ctrl_data, kp);
	otp_set_KI_STEP(&otp_ctrl_data, ki_step);
	otp_set_KI(&otp_ctrl_data, ki);
	otp_set_KD_STEP(&otp_ctrl_data, kd_step);
	otp_set_KD(&otp_ctrl_data, kd);

	otp_ctrl_data_apply(&otp_ctrl_data);
}

static void set_otp_debug_data(
	unsigned int interrupt_clr,
	unsigned int interrupt_en,
	unsigned int nth_debug_mux,
	unsigned int pos_err_fifo_mux,
	unsigned int err_fifo_mux,
	unsigned int error_sum_fifo_mux
)
{
	otp_debug_data_reset(&otp_debug_data);
	otp_set_INTERRUPT_CLR(&otp_debug_data, interrupt_clr);
	otp_set_INTERRUPT_EN(&otp_debug_data, interrupt_en);
	otp_set_NTH_DEBUG_MUX(&otp_debug_data, nth_debug_mux);
	otp_set_POS_ERR_FIFO_MUX(&otp_debug_data, pos_err_fifo_mux);
	otp_set_ERR_FIFO_MUX(&otp_debug_data, err_fifo_mux);
	otp_set_ERROR_SUM_FIFO_MUX(&otp_debug_data, error_sum_fifo_mux);
	otp_debug_data_apply(&otp_debug_data);
}

static void set_otp_score_data(
	unsigned int pid_score_s,
	unsigned int pid_score_m,
	unsigned int pid_freq_sht,
	unsigned int pid_calc_sht,
	unsigned int pid_score_sht,
	unsigned int pid_freq_s,
	unsigned int pid_freq_m
)
{
	otp_score_data_reset(&otp_score_data);
	otp_set_PID_SCORE_S(&otp_score_data, pid_score_s);
	otp_set_PID_SCORE_M(&otp_score_data, pid_score_m);
	otp_set_PID_FREQ_SHT(&otp_score_data, pid_freq_sht);
	otp_set_PID_CALC_SHT(&otp_score_data, pid_calc_sht);
	otp_set_PID_SCORE_SHT(&otp_score_data, pid_score_sht);
	otp_set_PID_FREQ_S(&otp_score_data, pid_freq_s);
	otp_set_PID_FREQ_M(&otp_score_data, pid_freq_m);
	otp_score_data_apply(&otp_score_data);
}

static void Normal_Mode_Setting(void)
{

	/* derrmax, piderrmin, kp_step, kp, ki_step, ki, kd_step, kd */
	set_otp_ctrl_data(0x00020000, 0xFFFFF060, 0x0, 0xFF9C, 0x0, 0xFFFE, 0x0, 0x0);
}

void getTHslope(void)
{
#ifndef EARLY_PORTING
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;
#endif

#if 1
	ts_bank = BIG_CORE_BANK;
	get_thermal_slope_intercept(&ts_info, ts_bank);
	MTS = ts_info.ts_MTS;
	BTS = ts_info.ts_BTS;
#else
	MTS =  0x2cf; /* (2048 * TS_SLOPE) + 2048; */
	BTS =  0x80E; /* 4 * TS_INTERCEPT; */
#endif
}


int BigOTPSetTempUpdate(void)
{
	thermal_set_big_core_speed(0xC, 0x00010032, 0x30D);
	return 0;
}


int ADC2Temp(int ADC)
{
	int temp;

	temp = (((BTS << 4) - ((ADC * MTS) >> 6)) & 0x00003FFF) * 1000 >> 6;
	temp = temp + 25000;

	return temp;
}

static int Temp2ADC(int Temp)
{
	int adc;

	Temp = Temp - 25;
	adc = (int)(((((BTS << 4) - (((unsigned int)Temp) << 6)) & 0x0000FFFF) << 6) / MTS);

/*
	otp_info("[OTP] MTS = 0x%x, BTS = 0x%x\n", MTS, BTS);
	otp_info("[OTP] temp %d = adc 0x%x\n", Temp+25, adc);
*/
	return adc;
}

int BigOTPSetTempPolicy(enum otp_policy TempPolicy)
{
	int policy_valid;

	switch (TempPolicy) {
	case NORMAL_MODE:
		Normal_Mode_Setting();
		policy_valid = 0;
	break;
	default:
		policy_valid = -1;
	break;
	}

	return policy_valid;
}

static int otp_thread_handler(void *arg)
{

	unsigned int score_status;

	do {
		wait_event_interruptible(wq, condition);
		if (((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1)) &&
		(get_piden_status() == 1)) {
			mutex_lock(&debug_mutex);

			otp_set_NTH_DEBUG_MUX(&otp_debug_data, 5);
			otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
			otp_info_data.score = (otp_read(OTP_PID_DEBUGOUT) & 0xFFFFFF);
			otp_set_NTH_DEBUG_MUX(&otp_debug_data, 6);
			otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
			otp_info_data.freq = (otp_read(OTP_PID_DEBUGOUT) & 0x7FFFF);

			mutex_unlock(&debug_mutex);

			otp_info_data.channel_status = BigiDVFSChannelGet(2);

			score_status = (otp_info_data.score >> 23) & 0x1;

			if (otp_info_data.channel_status == 0) {
				if ((otp_info_data.score != 0x0) &&
				((otp_info_data.score < LOWER_BOUND) || (score_status == 0x1))) {
					if (dump_debug_log) {
						otp_info("Big_Temp=%d\tscore=0x%06x\tfreq=0x%05x\tchannel=%d\n",
						get_immediate_big_wrap(), otp_info_data.score, otp_info_data.freq,
						otp_info_data.channel_status);
						otp_info(" Channel enable\n");
					}
					BigiDVFSChannel(2, 1);
					otp_info_data.channel_status = 1;

				}
			} else {
				if ((otp_info_data.score > UPPER_BOUND) && (score_status == 0x0)) {
					if (dump_debug_log) {
						otp_info("Big_Temp=%d\tscore=0x%06x\tfreq=0x%05x\tchannel=%d\n",
						get_immediate_big_wrap(), otp_info_data.score, otp_info_data.freq,
						otp_info_data.channel_status);
						otp_info(" Channel disable\n");
					}
					BigiDVFSChannel(2, 0);
					otp_info_data.channel_status = 0;

				}
			}
		}
		condition = 0;
		/* msleep(5); */
	} while (!kthread_should_stop());

	return 0;
}

static enum hrtimer_restart otp_timer_func(struct hrtimer *timer)
{
	condition = 1;
	wake_up_interruptible(&wq);
	hrtimer_forward_now(timer, ms_to_ktime(OTP_INTERVAL));

	return HRTIMER_RESTART;
}

static void otp_monitor_ctrl(void)
{
	if (1) {
		init_waitqueue_head(&wq);
		thread = kthread_run(otp_thread_handler, NULL, "otp info");
	}

	hrtimer_init(&otp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	otp_timer.function = otp_timer_func;
	/* hrtimer_start(&otp_timer, ms_to_ktime(OTP_INTERVAL), HRTIMER_MODE_REL); */
}

void enable_otp_hrtimer(void)
{
	if (timer_enabled == 0) {
		hrtimer_start(&otp_timer, ms_to_ktime(OTP_INTERVAL), HRTIMER_MODE_REL);
		timer_enabled = 1;
		if (dump_debug_log)
			otp_info("enable timer\n");
	}
}

void disable_otp_hrtimer(void)
{
	if (timer_enabled == 1) {
		hrtimer_cancel(&otp_timer);
		timer_enabled = 0;
		if (dump_debug_log)
			otp_info("disable timer\n");
	}
}

static void enable_OTP(void)
{
	if (dump_debug_log)
		otp_info(" Enabled\n");

	getTHslope();

	BigOTPSetTempUpdate();

	set_otp_config_data(
		Temp2ADC(t_target), error_sum_mode, fifo_size, adapt_mode, error_sum_clamp, fifo_clean,
		pos_sum_thold, neg_err_thold, pos_err_fifo_size);
	if (BigOTPSetTempPolicy(MODE) != 0)
		set_otp_ctrl_data(piderrmax, piderrmin, kp_step, kp, ki_step, ki, kd_step, kd);
	set_otp_score_data(
		pid_score_s, pid_score_m, pid_freq_sht, pid_calc_sht, pid_score_sht, pid_freq_s, pid_freq_m);
	set_otp_debug_data(
		interrupt_clr, interrupt_en, nth_debug_mux, pos_err_fifo_mux, err_fifo_mux, error_sum_fifo_mux);
	#if PID_EN_enable
	pid_en = 0x1;
	#endif

	otp_set_PID_EN(&otp_config_data, pid_en);
	otp_PID_EN_apply(&otp_config_data);

	/* otp_info_data_reset(&otp_info_data); */

#if OTP_TIMER_INTERRUPT
	if (get_immediate_big_wrap() < 70000)
		sw_channel_status = 0;
	else
		sw_channel_status = 1;

	if (dump_debug_log)
		otp_info(" Configuration finished, temp = %d, sw_channel_status = %d, pid_en = %d\n",
		get_immediate_big_wrap(), sw_channel_status, pid_en);

	mutex_lock(&timer_mutex);
	if (sw_channel_status == 1)
		enable_otp_hrtimer();
	mutex_unlock(&timer_mutex);
#else
	if (dump_debug_log)
		otp_info(" Configuration finished, pid_en = %d\n", pid_en);

	if (get_piden_status() == 1)
		enable_otp_hrtimer();
#endif
}

static void disable_OTP(void)
{
	if (dump_debug_log)
		otp_info(" Disabled\n");

	BigiDVFSChannel(2, 0);
	otp_info_data_reset(&otp_info_data);

#if OTP_TIMER_INTERRUPT
	mutex_lock(&timer_mutex);
	disable_otp_hrtimer();
	mutex_unlock(&timer_mutex);
#else
	disable_otp_hrtimer();
#endif

	pid_en = 0x0;
	otp_set_PID_EN(&otp_config_data, pid_en);
	otp_PID_EN_apply(&otp_config_data);

	set_otp_config_data(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
	set_otp_ctrl_data(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
	set_otp_score_data(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
	set_otp_debug_data(0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
}

int BigOTPThermIRQ(int status)
{
	if (0 == otp_enable)
		return 0;
#if OTP_TIMER_INTERRUPT
	if (otp_workqueue) {
		if (dump_debug_log)
			otp_info("Thermal IRQ, schedule queue\n");

		if (status == 1)
			queue_work(otp_workqueue, &therm_H_check);
		else
			queue_work(otp_workqueue, &therm_L_check);
	} else
		otp_info("Thermal IRQ before OTP init ready, ignore\n");
#endif
	return 0;
}

static void OTPThermHCheck(struct work_struct *ws)
{
	if (dump_debug_log)
		otp_info("get H IRQ start\n");

	mutex_lock(&timer_mutex);
	if (sw_channel_status == 0) {
		sw_channel_status = 1;
		if ((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1))
			enable_otp_hrtimer();
	}
	mutex_unlock(&timer_mutex);

}

static void OTPThermLCheck(struct work_struct *ws)
{
	if (dump_debug_log)
		otp_info("get L IRQ start\n");

	mutex_lock(&timer_mutex);
	if (sw_channel_status == 1) {
		sw_channel_status = 0;
		if ((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1))
			disable_otp_hrtimer();

	}
	mutex_unlock(&timer_mutex);
}

void BigOTPEnable(void)
{
	if (0 == otp_enable)
		return;

	enable_OTP();
}

void BigOTPDisable(void)
{
	if (0 == otp_enable)
		return;

	disable_OTP();
}

int BigOTPSetTempTarget(int TempTarget)
{
	int target_adc;

	if (unlikely((TempTarget > MAX_TARGET_TEMP) || (TempTarget < MIN_TARGET_TEMP)))
		return -1;

	target_adc = Temp2ADC(TempTarget);
	otp_set_T_TARGET(&otp_config_data, (unsigned int)target_adc);
	otp_T_TARGET_apply(&otp_config_data);

	return 0;
}

unsigned int get_freq_pct(u32 freq)
{
	unsigned int freqpct_x100;
	int i;

	freqpct_x100 = ((freq & 0x7F000) >> 12) * 100;
	for (i = 0 ; i < 6; i++) {
		if (freq & (1 << (11-i)))
			freqpct_x100 += (50 >> i);
	}

	return freqpct_x100;
}

unsigned int BigOTPGetiDVFSFreqpct(void)
{
	unsigned int freqpct_x100;
	u32 freq;

	freq = otp_info_data.freq;

	freqpct_x100 = get_freq_pct(freq);

	if (cpu_online(BIG_CPU_NUM0) || cpu_online(BIG_CPU_NUM1))
		return freqpct_x100;

	return 0;
}

unsigned int BigOTPGetFreqpct(void)
{
	unsigned int freqpct_x100;
	u32 freq;

	freq = otp_info_data.freq;

	freqpct_x100 = get_freq_pct(freq);

	if ((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1)) {
		if (otp_info_data.channel_status == 1)
			return freqpct_x100;
	}

	return 0;
}

TempInfo BigOTPGetTemp(void)
{
	int i;
	int index_len = 32;
	TempInfo TempArray;
	int TempErr;
	int Temp_ADC;
	int Temp;

	memset(&TempArray, -1, sizeof(TempInfo));

	if ((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1)) {
		for (i = 0; i < index_len; i++) {
			mutex_lock(&debug_mutex);
			otp_set_ERR_FIFO_MUX(&otp_debug_data, i);
			otp_ERR_FIFO_MUX_apply(&otp_debug_data);
			otp_set_NTH_DEBUG_MUX(&otp_debug_data, 0x1);
			otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
			mutex_unlock(&debug_mutex);

			TempErr = (((otp_read(OTP_PID_DEBUGOUT) & 0x0000FFFF) ^ 0x8000) - 0x8000);
			Temp_ADC = Temp2ADC(t_target) - TempErr;
			Temp = ADC2Temp((int)Temp_ADC);
			TempArray.data[i] = Temp;
		}
	}

	return TempArray;
}

/*
int BigOTPGetCurrTemp(void)
{
	unsigned int CurTempErr;
	unsigned int CurTemp_ADC;
	int CurTemp;

	otp_set_ERR_FIFO_MUX(&otp_debug_data, CurTempPT);
	otp_ERR_FIFO_MUX_apply(&otp_debug_data);
	otp_set_NTH_DEBUG_MUX(&otp_debug_data, 0x1);
	otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
	CurTempErr = otp_read(OTP_PID_DEBUGOUT) & 0x0000FFFF;
	CurTemp_ADC = t_target - CurTempErr;
	CurTemp = ADC2Temp((int)CurTemp_ADC);

	return CurTemp;
}
*/

int BigOTPGetCurrTemp(void)
{
	int CurTemp;

	CurTemp = get_immediate_big_wrap();

	return CurTemp;
}

int BigOTPISRHandler(void)
{
	unsigned int freq_intb_value;

#if DEBUG
	TempInfo Temparray;
	int i;
#endif
	mutex_lock(&debug_mutex);

	otp_set_NTH_DEBUG_MUX(&otp_debug_data, 6);
	otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
	freq_intb_value = (otp_read(OTP_PID_DEBUGOUT) & BIT(31)) >> 31;
	otp_info(" initial freq_intb_value = 0x%x\n", freq_intb_value);

	mutex_unlock(&debug_mutex);

	if (!freq_intb_value) {
		otp_info(" ISR\n");
		#if DEBUG
			otp_info(" Temp from TC, Temp=%d\n", get_immediate_big_wrap());
			Temparray = BigOTPGetTemp();
			for (i = 0; i < 32; i++)
				otp_info(" TempArray <%0d> %d\n", i, Temparray.data[i]);
		#endif
		otp_set_INTERRUPT_CLR(&otp_debug_data, 0x1);
		otp_INTERRUPT_CLR_apply(&otp_debug_data);

	}

	return 0;
}

static int otp_remove(struct platform_device *pdev)
{
	return 0;
}



static int otp_probe(struct platform_device *pdev)
{
	return 0;
}



static int otp_suspend(struct platform_device *pdev, pm_message_t state)
{

	return 0;
}



static int otp_resume(struct platform_device *pdev)
{

	return 0;
}



#ifdef CONFIG_OF
static const struct of_device_id mt_otp_of_match[] = {
	{ .compatible = "mediatek,ptpotp", },
	{},
};
#endif
static struct platform_driver otp_driver = {
	.remove     = otp_remove,
	.shutdown   = NULL,
	.probe      = otp_probe,
	.suspend    = otp_suspend,
	.resume     = otp_resume,
	.driver     = {
	.name   = "mt-otp",
	#ifdef CONFIG_OF
	.of_match_table = mt_otp_of_match,
	#endif
	},
};



#ifdef CONFIG_PROC_FS
static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}

/* otp debug */
static int otp_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "otp_debug = %d\n", dump_debug_log);

	return 0;
}



static ssize_t otp_debug_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	int val = 0;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &val)) {
		if (val == 0)
			dump_debug_log = 0;
		else
			dump_debug_log = 1;
	}
	free_page((unsigned long)buf);

	return count;
}


/* otp_enable */
static int otp_enable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "otp_enable = %d\n", otp_enable);
	seq_printf(m, "pid_en = %d\n", pid_en);

	return 0;
}



static ssize_t otp_enable_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d", &otp_enable, &pid_en) == 2) {
		if (otp_enable == 1)
			enable_OTP();
		else
			disable_OTP();
	}
	free_page((unsigned long)buf);

	return count;
}

/* otp_policy */
static int otp_policy_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "otp_policy = %d\n", MODE);
	return 0;
}

static ssize_t otp_policy_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);
	int val = 0;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &val)) {
		if (val == 0)
			MODE = 0;
		else
			MODE = 1;
	}

	free_page((unsigned long)buf);

	return count;
}
/* otp_config_data */
static int otp_config_data_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "t_target           = %d\n", t_target);
	seq_printf(m, "error_sum_mode     = 0x%x\n", error_sum_mode);
	seq_printf(m, "fifo_size          = 0x%x\n", fifo_size);
	seq_printf(m, "adapt_mode         = 0x%x\n", adapt_mode);
	seq_printf(m, "error_sum_clamp    = 0x%x\n", error_sum_clamp);
	seq_printf(m, "fifo_clean         = 0x%x\n", fifo_clean);
	seq_printf(m, "pos_sum_thold      = 0x%x\n", pos_sum_thold);
	seq_printf(m, "neg_err_thold      = 0x%x\n", neg_err_thold);
	seq_printf(m, "pos_err_fifo_size  = 0x%x\n\n", pos_err_fifo_size);
	seq_printf(m, "[reg] OTP_PID_CTL0       = 0x%x\n", (otp_read(OTP_PID_CTL0) & 0xFFFFFFFF));
	seq_printf(m, "[reg] OTP_PID_ADAPT      = 0x%x\n", (otp_read(OTP_PID_ADAPT) & 0xFFFFFFFF));
	seq_printf(m, "[reg] t_target           = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 16) & 0xFFF);
	seq_printf(m, "[reg] error_sum_mode     = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 6) & 0x3);
	seq_printf(m, "[reg] fifo_size          = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 4) & 0x3);
	seq_printf(m, "[reg] adapt_mode         = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 3) & 0x1);
	seq_printf(m, "[reg] error_sum_clamp    = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 2) & 0x1);
	seq_printf(m, "[reg] fifo_clean         = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 1) & 0x1);
	seq_printf(m, "[reg] pos_sum_thold      = 0x%x\n", (otp_read(OTP_PID_ADAPT) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] neg_err_thold      = 0x%x\n", (otp_read(OTP_PID_ADAPT) >> 4) & 0xF);
	seq_printf(m, "[reg] pos_err_fifo_size  = 0x%x\n", (otp_read(OTP_PID_ADAPT)) & 0x3);

	return 0;
}

static ssize_t otp_config_data_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %x %x %x %x %x %x %x %x",
		&t_target, &error_sum_mode, &fifo_size, &adapt_mode, &error_sum_clamp,
		&fifo_clean, &pos_sum_thold, &neg_err_thold, &pos_err_fifo_size) == 9)

		set_otp_config_data(
			Temp2ADC(t_target) & 0xFFF,
			error_sum_mode & 0x03,
			fifo_size & 0x03,
			adapt_mode & 0x1,
			error_sum_clamp & 0x1,
			fifo_clean & 0x1,
			pos_sum_thold & 0xFF,
			neg_err_thold & 0x01,
			pos_err_fifo_size & 0x3
			);

	free_page((unsigned long)buf);

	return count;
}

/* otp_ctrl_data */
static int otp_ctrl_data_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "piderrmax  = 0x%x\n", piderrmax);
	seq_printf(m, "piderrmin  = 0x%x\n", piderrmin);
	seq_printf(m, "kp_step    = 0x%x\n", kp_step);
	seq_printf(m, "kp         = 0x%x\n", kp);
	seq_printf(m, "ki_step    = 0x%x\n", ki_step);
	seq_printf(m, "ki         = 0x%x\n", ki);
	seq_printf(m, "kd_step    = 0x%x\n", kd_step);
	seq_printf(m, "kd         = 0x%x\n\n", kd);
	seq_printf(m, "[reg] piderrmax  = 0x%x\n", (otp_read(OTP_PID_ERRMAX)));
	seq_printf(m, "[reg] piderrmin  = 0x%x\n", (otp_read(OTP_PID_ERRMIN)));
	seq_printf(m, "[reg] kp_step    = 0x%x\n", (otp_read(OTP_PID_KP) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] kp         = 0x%x\n", (otp_read(OTP_PID_KP) & 0xFFFF));
	seq_printf(m, "[reg] ki_step    = 0x%x\n", (otp_read(OTP_PID_KI) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] ki         = 0x%x\n", (otp_read(OTP_PID_KI) & 0xFFFF));
	seq_printf(m, "[reg] kd_step    = 0x%x\n", (otp_read(OTP_PID_KD) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] kd         = 0x%x\n", (otp_read(OTP_PID_KD) & 0xFFFF));

	return 0;
}

static ssize_t otp_ctrl_data_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%x %x %x %x %x %x %x %x",
		&piderrmax, &piderrmin, &kp_step, &kp, &ki_step, &ki, &kd_step, &kd) == 8)
		set_otp_ctrl_data(
		piderrmax & 0xFFFFFFFF,
		piderrmin & 0xFFFFFFFF,
		kp_step & 0xFFFF,
		kp & 0xFFFF,
		ki_step & 0xFFFF,
		ki & 0xFFFF,
		kd & 0xFFFF,
		kd_step & 0xFFFF);

	free_page((unsigned long)buf);

	return count;
}

/* otp_score_data */
static int otp_score_data_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "pid_score_s    = 0x%x\n", pid_score_s);
	seq_printf(m, "pid_score_m    = 0x%x\n", pid_score_m);
	seq_printf(m, "pid_freq_sht   = 0x%x\n", pid_freq_sht);
	seq_printf(m, "pid_calc_sht   = 0x%x\n", pid_calc_sht);
	seq_printf(m, "pid_score_sht  = 0x%x\n", pid_score_sht);
	seq_printf(m, "pid_freq_s     = 0x%x\n", pid_freq_s);
	seq_printf(m, "pid_freq_m     = 0x%x\n\n", pid_freq_m);
	seq_printf(m, "[reg] pid_score_s    = 0x%x\n", (otp_read(OTP_PID_SCORE0) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] pid_score_m    = 0x%x\n", (otp_read(OTP_PID_SCORE0) & 0xFFFF));
	seq_printf(m, "[reg] pid_freq_sht   = 0x%x\n", (otp_read(OTP_PID_SHT) >> 16) & 0x1F);
	seq_printf(m, "[reg] pid_calc_sht   = 0x%x\n", (otp_read(OTP_PID_SHT) >> 8) & 0x1F);
	seq_printf(m, "[reg] pid_score_sht  = 0x%x\n", (otp_read(OTP_PID_SHT) & 0x1F));
	seq_printf(m, "[reg] pid_freq_s     = 0x%x\n", (otp_read(OTP_PID_FREQ0) >> 16) & 0xFFFF);
	seq_printf(m, "[reg] pid_freq_m     = 0x%x\n", (otp_read(OTP_PID_FREQ0) & 0xFFFF));

	return 0;
}

static ssize_t otp_score_data_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%x %x %x %x %x %x %x",
		&pid_score_s, &pid_score_m, &pid_freq_sht, &pid_calc_sht,
		&pid_score_sht, &pid_freq_s, &pid_freq_m) == 7)

		set_otp_score_data(
			pid_score_s & 0xFFFF,
			pid_score_m & 0xFFFF,
			pid_freq_sht & 0x1F,
			pid_calc_sht & 0x1F,
			pid_score_sht & 0x1F,
			pid_freq_s & 0xFFFF,
			pid_freq_m & 0xFFFF);

	free_page((unsigned long)buf);

	return count;
}

/* otp_debug_data */
static int otp_debug_data_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "interrupt_clr      = 0x%x\n", interrupt_clr);
	seq_printf(m, "interrupt_en       = 0x%x\n", interrupt_en);
	seq_printf(m, "nth_debug_mux      = 0x%x\n", nth_debug_mux);
	seq_printf(m, "pos_err_fifo_mux   = 0x%x\n", pos_err_fifo_mux);
	seq_printf(m, "err_fifo_mux       = 0x%x\n", err_fifo_mux);
	seq_printf(m, "error_sum_fifo_mux = 0x%x\n\n", error_sum_fifo_mux);
	seq_printf(m, "[reg] interrupt_clr      = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 10) & 0x1);
	seq_printf(m, "[reg] interrupt_en       = 0x%x\n", (otp_read(OTP_PID_CTL0) >> 8) & 0x1);
	seq_printf(m, "[reg] nth_debug_mux      = 0x%x\n", (otp_read(OTP_PID_MUX) >> 20) & 0x7);
	seq_printf(m, "[reg] pos_err_fifo_mux   = 0x%x\n", (otp_read(OTP_PID_MUX) >> 16) & 0x7);
	seq_printf(m, "[reg] err_fifo_mux       = 0x%x\n", (otp_read(OTP_PID_MUX) >> 8) & 0x1F);
	seq_printf(m, "[reg] error_sum_fifo_mux = 0x%x\n", (otp_read(OTP_PID_MUX) & 0x1F));

	return 0;
}

static ssize_t otp_debug_data_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%x %x %x %x %x %x",
		&interrupt_clr, &interrupt_en, &nth_debug_mux,
		&pos_err_fifo_mux, &err_fifo_mux, &error_sum_fifo_mux) == 6
)
		set_otp_debug_data(
			interrupt_clr & 0x1,
			interrupt_en & 0x1,
			nth_debug_mux & 0x1F,
			pos_err_fifo_mux & 0x7,
			err_fifo_mux & 0x1F,
			error_sum_fifo_mux & 0x1F);

	free_page((unsigned long)buf);

	return count;
}

/* otp_curr_temp */
static int otp_curr_temp_proc_show(struct seq_file *m, void *v)
{

	unsigned int freqpct_x100_internal;
	unsigned int freqpct_x100;
	unsigned int freqpct_x100_real;
	u32 score;
	u32 freq;
	u32 info_freq;

	info_freq = otp_info_data.freq;
	freqpct_x100 = get_freq_pct(info_freq);

	if ((cpu_online(BIG_CPU_NUM0) == 1) || (cpu_online(BIG_CPU_NUM1) == 1)) {
		mutex_lock(&debug_mutex);

		otp_set_NTH_DEBUG_MUX(&otp_debug_data, 5);
		otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
		score = (otp_read(OTP_PID_DEBUGOUT) & 0xFFFFFF);
		otp_set_NTH_DEBUG_MUX(&otp_debug_data, 6);
		otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
		freq = (otp_read(OTP_PID_DEBUGOUT) & 0x7FFFF);

		mutex_unlock(&debug_mutex);

		freqpct_x100_internal = get_freq_pct(freq);

		if (otp_info_data.channel_status == 1)
			freqpct_x100_real = freqpct_x100;
		else
			freqpct_x100_real = 0;
	} else {
		freqpct_x100_real = 0;
		score = 0;
		freq = 0;
		freqpct_x100_internal = 0;
	}

	seq_printf(m, "%d,0x%06x,0x%05x,%u,%d,0x%06x,0x%05x,%u,%u\n",
	get_immediate_big_wrap(), score, freq, freqpct_x100_internal,
	otp_info_data.channel_status, otp_info_data.score, info_freq,
	freqpct_x100, freqpct_x100_real);

	return 0;
}

/* otp_history_temp */
static int otp_history_temp_proc_show(struct seq_file *m, void *v)
{
	TempInfo Temparray;
	int i;

	Temparray = BigOTPGetTemp();

	for (i = 0; i < 32; i++)
		seq_printf(m, "%d\t", Temparray.data[i]);

	return 0;
}

/* otp_debug_status */
static int otp_debug_status_proc_show(struct seq_file *m, void *v)
{
	int i;

	mutex_lock(&debug_mutex);

	for (i = 0; i < 10; i++) {
		otp_set_NTH_DEBUG_MUX(&otp_debug_data, i);
		otp_NTH_DEBUG_MUX_apply(&otp_debug_data);
		otp_debug_dump_data[i] = otp_read(OTP_PID_DEBUGOUT);
		seq_printf(m, "otp_debug_dump_data[%d] = 0x%08x\n", i, otp_debug_dump_data[i]);
	}

	mutex_unlock(&debug_mutex);
	return 0;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

	PROC_FOPS_RW(otp_enable);
	PROC_FOPS_RW(otp_debug);
	PROC_FOPS_RW(otp_policy);
	PROC_FOPS_RW(otp_config_data);
	PROC_FOPS_RW(otp_ctrl_data);
	PROC_FOPS_RW(otp_score_data);
	PROC_FOPS_RW(otp_debug_data);
	PROC_FOPS_RO(otp_curr_temp);
	PROC_FOPS_RO(otp_history_temp);
	PROC_FOPS_RO(otp_debug_status);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {

	PROC_ENTRY(otp_enable),
	PROC_ENTRY(otp_debug),
	PROC_ENTRY(otp_policy),
	PROC_ENTRY(otp_config_data),
	PROC_ENTRY(otp_ctrl_data),
	PROC_ENTRY(otp_score_data),
	PROC_ENTRY(otp_debug_data),
	PROC_ENTRY(otp_curr_temp),
	PROC_ENTRY(otp_history_temp),
	PROC_ENTRY(otp_debug_status),
	};

	dir = proc_mkdir("otp", NULL);

	if (!dir) {
		otp_error("fail to create /proc/otp @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			otp_error("%s(), create /proc/otp/%s failed\n", __func__, entries[i].name);
	}

	return 0;
}

#endif /* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init otp_init(void)
{
	int err = 0;

	struct device_node *node = NULL;

	otp_info(" INIT\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,ptpotp");

	if (!node) {
		otp_error(" find node failed\n");
	} else {
		/* Setup IO addresses */
		otp_base = of_iomap(node, 0);
	}

	err = platform_driver_register(&otp_driver);

	if (err) {
		otp_error("%s(), OTP driver callback register failed..\n", __func__);

		return err;
	}
	/* BigOTPEnable(); */
	otp_config_data_reset(&otp_config_data);
	otp_info_data_reset(&otp_info_data);

	otp_workqueue = create_workqueue("otp_wq");

	otp_monitor_ctrl();

#if OTP_TIMER_INTERRUPT
	if (get_immediate_big_wrap() < 70000)
		sw_channel_status = 0;
	else
		sw_channel_status = 1;

	mutex_lock(&timer_mutex);
	if (sw_channel_status == 1)
		enable_otp_hrtimer();
	mutex_unlock(&timer_mutex);

	if (dump_debug_log) {
		otp_info("Big Temp = %d, sw_channel_status = %d\n",
		get_immediate_big_wrap(), sw_channel_status);
	}
#endif

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (_create_procfs()) {
		err = -ENOMEM;
		goto out;
	}
#endif /* CONFIG_PROC_FS */

out:
	return err;
}

static void __exit otp_exit(void)
{
	destroy_workqueue(otp_workqueue);
}

late_initcall(otp_init);
module_exit(otp_exit);

MODULE_DESCRIPTION("MediaTek OTP Driver v0.1");
MODULE_LICENSE("GPL");

#undef __MT_OTP_C__

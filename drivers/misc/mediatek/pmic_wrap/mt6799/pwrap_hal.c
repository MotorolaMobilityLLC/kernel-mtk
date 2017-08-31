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
/******************************************************************************
 * MTK PMIC Wrapper Driver
 *
 * Copyright 2016 MediaTek Co.,Ltd.
 *
 * DESCRIPTION:
 *     This file provides API for other drivers to access PMIC registers
 *
 ******************************************************************************/

#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/io.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif
#include <mt-plat/mtk_gpio.h>
#include <mach/mtk_pmic_wrap.h>
#include "pwrap_hal.h"
#undef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include "sspm_ipi.h"
#endif
#define PMIC_WRAP_DEVICE "pmic_wrap"


#ifdef CONFIG_OF
void __iomem *pwrap_base;
static void __iomem *topckgen_base;
static void __iomem *infracfg_ao_base;
#endif
static struct mt_pmic_wrap_driver *mt_wrp;

static spinlock_t	wrp_lock = __SPIN_LOCK_UNLOCKED(lock);

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#define WRITE_CMD   1
#define READ_CMD	 0
#define WRITE_PMIC   1
#define WRITE_PMIC_WRAP   0

static u32 pwrap_recv_data[4] = {0};
static s32 pwrap_wacs2_ipi(u32  adr, u32 rdata, u32 flag);
static int pwrap_ipi_register(void);
#endif

/*********************start ---internal API**************************************/
static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata);
static s32 _pwrap_reset_spislv(void);
static s32 _pwrap_init_dio(u32 dio_en);
static s32 _pwrap_init_cipher(void);
static s32 _pwrap_init_reg_clock(u32 regck_sel);
static bool _pwrap_timeout_ns(u64 start_time_ns, u64 timeout_time_ns);
static u64 _pwrap_get_current_time(void);
static u64 _pwrap_time2ns(u64 time_us);
static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata);
static void _pwrap_enable(void);
static void _pwrap_starve_set(void);
static s32 pwrap_wacs2_hal(u32 write, u32 adr, u32 wdata, u32 *rdata);
static u32 pwrap_write_test(void);
static u32 pwrap_read_test(void);
static s32 pwrap_read_nochk(u32  adr, u32 *rdata);
static s32 pwrap_write_nochk(u32  adr, u32  wdata);
static inline void pwrap_dump_ap_register(void);
/************* end--internal API************************************************/


#ifdef CONFIG_OF
static int pwrap_of_iomap(void);
static void pwrap_of_iounmap(void);
#endif

/*********pwrap timeout API******************/
/*#define PWRAP_TIMEOUT*/
#ifdef PWRAP_TIMEOUT
static u64 _pwrap_get_current_time(void)
{
	return sched_clock();
}

static bool _pwrap_timeout_ns(u64 start_time_ns, u64 timeout_time_ns)
{
	u64 cur_time = 0;
	u64 elapse_time = 0;

	/* get current tick */
	cur_time = _pwrap_get_current_time();	/* ns */

	/* avoid timer over flow exiting in FPGA env */
	if (cur_time < start_time_ns) {
		PWRAPERR("@@@@Timer overflow! start%lld cur timer%lld\n", start_time_ns, cur_time);
		start_time_ns = cur_time;
		timeout_time_ns = 2000 * 1000;	/* 2000us */
		PWRAPERR("@@@@reset timer! start%lld setting%lld\n", start_time_ns,
			 timeout_time_ns);
	}

	elapse_time = cur_time - start_time_ns;

	/* check if timeout */
	if (timeout_time_ns <= elapse_time) {
		/* timeout */
		PWRAPERR("@@@@Timeout: elapse time%lld,start%lld setting timer%lld\n",
			 elapse_time, start_time_ns, timeout_time_ns);
		return 1;
	}
	return 0;
}

static u64 _pwrap_time2ns(u64 time_us)
{
	return time_us * 1000;
}

#else
static u64 _pwrap_get_current_time(void)
{
	return 0;
}

static bool _pwrap_timeout_ns(u64 start_time_ns, u64 elapse_time)
{
	return 0;
}

static u64 _pwrap_time2ns(u64 time_us)
{
	return 0;
}

#endif




static void pwrap_ut(u32 ut_test)
{
	switch (ut_test) {
	case 1:
		pwrap_write_test();
		break;
	case 2:
		pwrap_read_test();
		break;
	case 3:
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		pwrap_wacs2_ipi(0x10010000 + 0xD8, 0xffffffff, (WRITE_CMD | WRITE_PMIC_WRAP));
		break;
#endif
	default:
		PWRAPREG("default test.\n");
		break;
	}
}
/* ##################################################################### */
/* define macro and inline function (for do while loop) */
/* ##################################################################### */
typedef u32(*loop_condition_fp) (u32);	/* define a function pointer */

static inline u32 wait_for_fsm_idle(u32 x)
{
	return GET_WACS2_FSM(x) != WACS_FSM_IDLE;
}

static inline u32 wait_for_fsm_vldclr(u32 x)
{
	return GET_WACS2_FSM(x) != WACS_FSM_WFVLDCLR;
}

static inline u32 wait_for_sync(u32 x)
{
	return GET_WACS2_SYNC_IDLE2(x) != WACS_SYNC_IDLE;
}

static inline u32 wait_for_idle_and_sync(u32 x)
{
	return (GET_WACS2_FSM(x) != WACS_FSM_IDLE) || (GET_WACS2_SYNC_IDLE2(x) != WACS_SYNC_IDLE);
}

static inline u32 wait_for_wrap_idle(u32 x)
{
	return (GET_WRAP_FSM(x) != 0x0) || (GET_WRAP_CH_DLE_RESTCNT(x) != 0x0);
}

static inline u32 wait_for_wrap_state_idle(u32 x)
{
	return GET_WRAP_AG_DLE_RESTCNT(x) != 0;
}

static inline u32 wait_for_man_idle_and_noreq(u32 x)
{
	return (GET_MAN_REQ(x) != MAN_FSM_NO_REQ) || (GET_MAN_FSM(x) != MAN_FSM_IDLE);
}

static inline u32 wait_for_man_vldclr(u32 x)
{
	return GET_MAN_FSM(x) != MAN_FSM_WFVLDCLR;
}

static inline u32 wait_for_cipher_ready(u32 x)
{
	return x != 3;
}

static inline u32 wait_for_stdupd_idle(u32 x)
{
	return GET_STAUPD_FSM(x) != 0x0;
}

static inline u32 wait_for_state_ready_init(loop_condition_fp fp, u32 timeout_us,
						void *wacs_register, u32 *read_reg)
{

	u64 start_time_ns = 0, timeout_ns = 0;
	u32 reg_rdata = 0x0;

	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(timeout_us);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait_for_state_ready_init timeout when waiting for idle\n");
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);
	} while (fp(reg_rdata));

	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline u32 wait_for_state_idle_init(loop_condition_fp fp, u32 timeout_us,
					   void *wacs_register, void *wacs_vldclr_register,
					   u32 *read_reg)
{

	u64 start_time_ns = 0, timeout_ns = 0;
	u32 reg_rdata;

	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(timeout_us);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait_for_state_idle_init timeout when waiting for idle\n");
			pwrap_dump_ap_register();
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);
		/* if last read command timeout,clear vldclr bit
		 * read command state machine:
		 * FSM_REQ-->wfdle-->WFVLDCLR;write:FSM_REQ-->idle
		*/
		switch (GET_WACS0_FSM(reg_rdata)) {
		case WACS_FSM_WFVLDCLR:
			WRAP_WR32(wacs_vldclr_register, 1);
			PWRAPERR("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
			break;
		case WACS_FSM_WFDLE:
			PWRAPERR("WACS_FSM = WACS_FSM_WFDLE\n");
			break;
		case WACS_FSM_REQ:
			PWRAPERR("WACS_FSM = WACS_FSM_REQ\n");
			break;
		default:
			break;
		}
	} while (fp(reg_rdata));
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline u32 wait_for_state_idle(loop_condition_fp fp, u32 timeout_us, void *wacs_register,
					  void *wacs_vldclr_register, u32 *read_reg)
{

	u64 start_time_ns = 0, timeout_ns = 0;
	u32 reg_rdata;

	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(timeout_us);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait_for_state_idle timeout when waiting for idle\n");
			pwrap_dump_ap_register();
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);
		if (GET_WACS2_INIT_DONE2(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
		switch (GET_WACS2_FSM(reg_rdata)) {
		case WACS_FSM_WFVLDCLR:
			WRAP_WR32(wacs_vldclr_register, 1);
			PWRAPERR("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
			break;
		case WACS_FSM_WFDLE:
			PWRAPERR("WACS_FSM = WACS_FSM_WFDLE\n");
			break;
		case WACS_FSM_REQ:
			PWRAPERR("WACS_FSM = WACS_FSM_REQ\n");
			break;
		default:
			break;
		}
	} while (fp(reg_rdata));
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline u32 wait_for_state_ready(loop_condition_fp fp, u32 timeout_us, void *wacs_register,
					   u32 *read_reg)
{

	u64 start_time_ns = 0, timeout_ns = 0;
	u32 reg_rdata;

	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(timeout_us);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("timeout when waiting for idle\n");
			pwrap_dump_ap_register();
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);

		if (GET_WACS2_INIT_DONE2(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
	} while (fp(reg_rdata));
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

/******************************************************
 * Function : pwrap_wacs2_hal()
 * Description :
 * Parameter :
 * Return :
 ******************************************************/
static s32 pwrap_wacs2_hal(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	u32 reg_rdata = 0;
	u32 wacs_write = 0;
	u32 wacs_adr = 0;
	u32 wacs_cmd = 0;
	u32 return_value = 0;
	unsigned long flags = 0;

	/* Check argument validation */
	if ((write & ~(0x1)) != 0)
		return E_PWR_INVALID_RW;
	if ((adr & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;
	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	spin_lock_irqsave(&wrp_lock, flags);

	/* Check IDLE & INIT_DONE in advance */
	return_value =
		wait_for_state_idle(wait_for_fsm_idle, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA,
				PMIC_WRAP_WACS2_VLDCLR, 0);
	if (return_value != 0) {
		PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n", return_value);
		goto FAIL;
	}
	wacs_write = write << 31;
	wacs_adr = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;

	WRAP_WR32(PMIC_WRAP_WACS2_CMD, wacs_cmd);
	if (write == 0) {
		if (rdata == NULL) {
			PWRAPERR("rdata is a NULL pointer\n");
			return_value = E_PWR_INVALID_ARG;
			goto FAIL;
		}
		return_value =
			wait_for_state_ready(wait_for_fsm_vldclr, TIMEOUT_READ, PMIC_WRAP_WACS2_RDATA,
					 &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n", return_value);
			return_value += 1;
			goto FAIL;
		}
		*rdata = GET_WACS2_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR, 1);
	}

FAIL:
	spin_unlock_irqrestore(&wrp_lock, flags);
	if (return_value != 0) {
		PWRAPERR("pwrap_wacs2_hal fail,return_value=%d\n", return_value);
		PWRAPERR("timeout:BUG_ON here\n");
	}

	return return_value;
}


/*********************internal API for pwrap_init***************************/

/**********************************
 * Function : _pwrap_wacs2_nochk()
 * Description :
 * Parameter :
 * Return :
 ***********************************/
s32 pwrap_read_nochk(u32 adr, u32 *rdata)
{
	return _pwrap_wacs2_nochk(0, adr, 0, rdata);
}

s32 pwrap_write_nochk(u32 adr, u32 wdata)
{
	return _pwrap_wacs2_nochk(1, adr, wdata, 0);
}

static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	u32 reg_rdata = 0x0;
	u32 wacs_write = 0x0;
	u32 wacs_adr = 0x0;
	u32 wacs_cmd = 0x0;
	u32 return_value = 0x0;

	/* Check argument validation */
	if ((write & ~(0x1)) != 0)
		return E_PWR_INVALID_RW;
	if ((adr & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;
	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	/* Check IDLE */
	return_value =
		wait_for_state_ready_init(wait_for_fsm_idle, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_wacs2_nochk write command fail,return_value=%x\n", return_value);
		return return_value;
	}

	wacs_write = write << 31;
	wacs_adr = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;
	WRAP_WR32(PMIC_WRAP_WACS2_CMD, wacs_cmd);

	if (write == 0) {
		if (rdata == NULL) {
			PWRAPERR("rdata is a NULL pointer\n");
			return_value = E_PWR_INVALID_ARG;
			return return_value;
		}
		return_value =
			wait_for_state_ready_init(wait_for_fsm_vldclr, TIMEOUT_READ,
						  PMIC_WRAP_WACS2_RDATA, &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n", return_value);
			return_value += 1;
			return return_value;
		}
		*rdata = GET_WACS2_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR, 1);
	}
	return 0;
}
static void __pwrap_soft_reset(void)
{
	PWRAPLOG("start reset wrapper\n");
	WRAP_WR32(INFRA_GLOBALCON_RST2_SET, 0x1);
	WRAP_WR32(INFRA_GLOBALCON_RST2_CLR, 0x1);
}

static void __pwrap_spi_clk_set(void)
{
	PWRAPLOG("spi clk set ....\n");
	WRAP_WR32(CLK_CFG_6_CLR, 0x00ff0000);
	/* Select ULPOSC/16 Clock as SYS, TMR and SPI Clocks in Suspend Mode */
	WRAP_WR32(MODULE_CLK_SEL, WRAP_RD32(MODULE_CLK_SEL) | 0x100);
	/*sys_ck cg enable, turn off clock*/
	WRAP_WR32(MODULE_SW_CG_0_SET, 0x0000000F);
	/* toggle PMIC_WRAP and pwrap_spictl reset */
	__pwrap_soft_reset();
	PWRAPLOG("pwrap_spictl reset ok\n");
	/* turn on clock*/
	WRAP_WR32(MODULE_SW_CG_0_CLR, 0x0000000F);
}

/************************************************
 * Function : _pwrap_init_dio()
 * Description :call it in pwrap_init,mustn't check init done
 * Parameter :
 * Return :
 ************************************************/
static s32 _pwrap_init_dio(u32 dio_en)
{
	u32 rdata = 0x0;

	pwrap_write_nochk(DEW_DIO_EN, dio_en);
#ifdef DUAL_PMICS
	pwrap_write_nochk(EXT_DEW_DIO_EN, dio_en);
#endif

	do {
		rdata = WRAP_RD32(PMIC_WRAP_WACS2_RDATA);
	} while ((GET_WACS2_FSM(rdata) != 0x0) || (GET_WACS2_SYNC_IDLE2(rdata) != 0x1));

#ifndef DUAL_PMICS
	WRAP_WR32(PMIC_WRAP_DIO_EN, 0x1);
#else
	WRAP_WR32(PMIC_WRAP_DIO_EN, 0x3);
#endif


	return 0;
}

/***************************************************
 * Function : _pwrap_init_cipher()
 * Description :
 * Parameter :
 * Return :
 ****************************************************/
static s32 _pwrap_init_cipher(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	u32 start_time_ns = 0, timeout_ns = 0;

	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 0);
	WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL, 2);
	WRAP_WR32(PMIC_WRAP_CIPHER_EN, 1);
	/* Config CIPHER @ PMIC */
	pwrap_write_nochk(DEW_CIPHER_SWRST, 0x1);
	pwrap_write_nochk(DEW_CIPHER_SWRST, 0x0);
	pwrap_write_nochk(DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(DEW_CIPHER_IV_SEL,  0x2);
	pwrap_write_nochk(DEW_CIPHER_EN,  0x1);
	PWRAPLOG("[_pwrap_init_cipher]Config CIPHER of PMIC 0 ok\n");
#ifdef DUAL_PMICS
	/* Config CIPHER of PMIC 1 */
	pwrap_write_nochk(EXT_DEW_CIPHER_SWRST, 0x1);
	pwrap_write_nochk(EXT_DEW_CIPHER_SWRST, 0x0);
	pwrap_write_nochk(EXT_DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(EXT_DEW_CIPHER_IV_SEL,  0x2);
	pwrap_write_nochk(EXT_DEW_CIPHER_EN,  0x1);
	PWRAPLOG("[_pwrap_init_cipher]Config CIPHER of PMIC 1 ok\n");
#endif
	/*wait for cipher data ready@AP */
	return_value = wait_for_state_ready_init(wait_for_cipher_ready, TIMEOUT_WAIT_IDLE, PMIC_WRAP_CIPHER_RDY, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher data ready@AP fail,return_value=%x\n", return_value);
		return return_value;
	}
	PWRAPLOG("wait for cipher 0 and 1 to be ready ok\n");

	/* wait for cipher 0 data ready@PMIC */
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
			PWRAPERR("wait for cipher 0 data ready@PMIC\n");

		pwrap_read_nochk(DEW_CIPHER_RDY, &rdata);
	 } while (rdata != 0x1); /* cipher_ready */

	return_value = pwrap_write_nochk(DEW_CIPHER_MODE, 0x1);
	if (return_value != 0) {
		PWRAPERR("write DEW_CIPHER_MODE fail,return_value=%x\n", return_value);
		return return_value;
	}
	PWRAPLOG("wait for cipher0 data ready ok\n");
#ifdef DUAL_PMICS
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
			PWRAPERR("wait for cipher 1 data ready@PMIC\n");
		pwrap_read_nochk(EXT_DEW_CIPHER_RDY, &rdata);
	 } while (rdata != 0x1); /* cipher_ready */

	return_value = pwrap_write_nochk(EXT_DEW_CIPHER_MODE, 0x1);
	if (return_value != 0) {
		PWRAPERR("write EXT_DEW_CIPHER_MODE fail,return_value=%x\n", return_value);
		return return_value;
	}
#endif
	PWRAPLOG("wait for cipher 1 data ready@PMIC ok\n");

	/* wait for cipher mode idle */
	return_value = wait_for_state_ready_init(wait_for_idle_and_sync, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher mode idle fail,return_value=%x\n", return_value);
		return return_value;
	}
	WRAP_WR32(PMIC_WRAP_CIPHER_MODE, 1);

	/* Read Test */
	pwrap_read_nochk(DEW_READ_TEST, &rdata);
	if (rdata != MT6335_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	}

#ifdef DUAL_PMICS
	pwrap_read_nochk(EXT_DEW_READ_TEST, &rdata);
	if (rdata != MT6337_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	}
#endif

	return 0;
}

static void _pwrap_InitStaUpd(void)
{

	WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN, 0x19f);
#if 0
	/* CRC */
#ifdef DUAL_PMICS
	pwrap_write_nochk(DEW_CRC_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_CRC_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_SIG_ADR, DEW_CRC_VAL);
#else
	pwrap_write_nochk(DEW_CRC_EN, 0x1);
	pwrap_write_nochk(EXT_DEW_CRC_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_CRC_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_SIG_ADR, (EXT_DEW_CRC_VAL << 16 | DEW_CRC_VAL));
#endif
#endif
    /* Signature */
#ifndef DUAL_PMICS
	WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x1);
	WRAP_WR32(PMIC_WRAP_SIG_ADR, DEW_CRC_VAL);
	WRAP_WR32(PMIC_WRAP_SIG_VALUE, 0x83);
#else
	WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x3);
	WRAP_WR32(PMIC_WRAP_SIG_ADR, (EXT_DEW_CRC_VAL << 16) | DEW_CRC_VAL);
	WRAP_WR32(PMIC_WRAP_SIG_VALUE, (0x83 << 16) | 0x83);
#endif

	WRAP_WR32(PMIC_WRAP_EINT_STA0_ADR, INT_STA);
#ifdef DUAL_PMICS
	WRAP_WR32(PMIC_WRAP_EINT_STA1_ADR, EXT_INT_STA);
#endif

	/* MD ADC Interface */
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST, AUXADC_ADC36);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_WP, AUXADC_MDBG_1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR0, AUXADC_BUF0);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR1, AUXADC_BUF1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR2, AUXADC_BUF2);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR3, AUXADC_BUF3);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR4, AUXADC_BUF4);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR5, AUXADC_BUF5);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR6, AUXADC_BUF6);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR7, AUXADC_BUF7);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR8, AUXADC_BUF8);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR9, AUXADC_BUF9);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR10, AUXADC_BUF10);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR11, AUXADC_BUF11);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR12, AUXADC_BUF12);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR13, AUXADC_BUF13);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR14, AUXADC_BUF14);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR15, AUXADC_BUF15);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR16, AUXADC_BUF16);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR17, AUXADC_BUF17);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR18, AUXADC_BUF18);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR19, AUXADC_BUF19);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR20, AUXADC_BUF20);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR21, AUXADC_BUF21);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR22, AUXADC_BUF22);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR23, AUXADC_BUF23);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR24, AUXADC_BUF24);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR25, AUXADC_BUF25);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR26, AUXADC_BUF26);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR27, AUXADC_BUF27);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR28, AUXADC_BUF28);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR29, AUXADC_BUF29);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR30, AUXADC_BUF30);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR31, AUXADC_BUF31);

	/* GPS Interface */
	/* change to external connsys gps
	* WRAP_WR32(PMIC_WRAP_ADC_CMD_ADDR, AUXADC_RQST1_SET);
	* WRAP_WR32(PMIC_WRAP_PWRAP_ADC_CMD, 0x0100);
	* WRAP_WR32(PMIC_WRAP_ADC_RDATA_ADDR, AUXADC_ADC16);
	*/
	WRAP_WR32(PMIC_WRAP_GPS_ADC_RDATA_ADDR, AUXADC_ADC36);
	return;
}

static void _pwrap_starve_set(void)
{
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 0x0007);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_0, 0x0402);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_1, 0x0402);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_2, 0x0403);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_3, 0x040f);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_4, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_5, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_6, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_7, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_8, 0x0428);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_9, 0x0428);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_10, 0x0417);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_11, 0x0563);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_12, 0x047c);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_13, 0x0740);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_14, 0x0740);

	return;

}

static void _pwrap_enable(void)
{
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0x7fff);
	WRAP_WR32(PMIC_WRAP_WACS0_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_WACS1_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_WACS2_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_WACS3_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_STAUPD_CTRL, 0x5);
	WRAP_WR32(PMIC_WRAP_WDT_UNIT, 0xf);
	WRAP_WR32(PMIC_WRAP_WDT_SRC_EN, 0xffffffff);
	WRAP_WR32(PMIC_WRAP_TIMER_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_INT0_EN, 0xffffffff);
	WRAP_WR32(PMIC_WRAP_INT1_EN, 0xffffffff);

}

/************************************************
 * Function : _pwrap_init_sistrobe()
 * scription : Initialize SI_CK_CON and SIDLY
 * Parameter :
 * Return :
 ************************************************/
static s32 _pwrap_init_sistrobe(int dual_si_sample_settings)
{
	int rdata;
	int si_en_sel, si_ck_sel, si_dly, si_sample_ctrl, reserved = 0;
	char result_faulty = 0;
	char found;
	int test_data[30] = {0x6996, 0x9669, 0x6996, 0x9669, 0x6996, 0x9669, 0x6996, 0x9669, 0x6996, 0x9669,
		0x5AA5, 0xA55A, 0x5AA5, 0xA55A, 0x5AA5, 0xA55A, 0x5AA5, 0xA55A, 0x5AA5, 0xA55A,
		0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27};
	int i;
	int error = 0;

	/* TINFO = "[DrvPWRAP_InitSiStrobe] SI Strobe Calibration For PMIC 0............" */
	/* TINFO = "[DrvPWRAP_InitSiStrobe] Scan For The First Valid Sampling Clock Edge......" */
	found = 0;
	for (si_en_sel = 0; si_en_sel < 3; si_en_sel++) {
		for (si_ck_sel = 0; si_ck_sel < 2; si_ck_sel++) {
			si_sample_ctrl = (si_en_sel << 6) | (si_ck_sel << 5);
			WRAP_WR32(PMIC_WRAP_SI_SAMPLE_CTRL, si_sample_ctrl);

			pwrap_read_nochk(DEW_READ_TEST, &rdata);
			if (rdata == 0x5aa5) {
				PWRAPLOG("[DrvPWRAP_InitSiStrobe]The First Valid Sampling Clock Edge Is Found !!!\n");
				PWRAPLOG("si_en_sel = %x, si_ck_sel = %x, si_sample_ctrl = %x, rdata = %x\n",
						si_en_sel, si_ck_sel, si_sample_ctrl, rdata);
				found = 1;
				break;
			}
			PWRAPERR("si_en_sel = %x, si_ck_sel = %x, *PMIC_WRAP_SI_SAMPLE_CTRL = %x, rdata = %x\n",
					si_en_sel, si_ck_sel, si_sample_ctrl, rdata);
		}
		if (found == 1)
			break;
	}
	if (found == 0) {
		result_faulty |= 0x1;
		PWRAPERR("result_faulty = %d\n", result_faulty);
	}
	/* TINFO = "[DrvPWRAP_InitSiStrobe] Search For The Data Boundary......" */
	for (si_dly = 0; si_dly < 8; si_dly++) {
		pwrap_write_nochk(RG_SPI_CON2, si_dly);

		/*pwrap_read_nochk(DEW_READ_TEST, &rdata);*/

		error = 0;
		for (i = 0; i < 30; i++) {
			pwrap_write_nochk(DEW_WRITE_TEST, test_data[i]);
			pwrap_read_nochk(DEW_WRITE_TEST, &rdata);
			if ((rdata & 0x7fff) != (test_data[i] & 0x7fff)) {
				PWRAPERR("[DrvPWRAP_InitSiStrobe] (%x, %x, %x) Data Boundary Is Found !!\n",
					si_dly, reserved, rdata);
				PWRAPERR("[DrvPWRAP_InitSiStrobe] (%x, %x, %x) Data Boundary Is Found !!\n",
					si_dly, si_dly, rdata);
				error = 1;
				break;
			}
		}
		if (error == 1)
			break;
		PWRAPLOG("si_dly = %x, *PMIC_WRAP_RESERVED = %x, rdata = %x\n",
				si_dly, reserved, rdata);
		PWRAPLOG("si_dly = %x, *RG_SPI_CON2 = %x, rdata = %x\n",
				si_dly, si_dly, rdata);
	}

	/* TINFO = "[DrvPWRAP_InitSiStrobe] Change The Sampling Clock Edge To The Next One." */
	/*elbrus si_sample_ctrl = (((si_en_sel << 1) | si_ck_sel) + 1) << 2;*/
	si_sample_ctrl = si_sample_ctrl + 0x20;
	WRAP_WR32(PMIC_WRAP_SI_SAMPLE_CTRL, si_sample_ctrl);
	if (si_dly == 8) {
		PWRAPLOG("SI Strobe Calibration For PMIC 0 Done, (%x, si_dly%x)\n", si_sample_ctrl, si_dly);
		si_dly--;
	}
	PWRAPLOG("SI Strobe Calibration For PMIC 0 Done, (%x, %x)\n", si_sample_ctrl, reserved);
	PWRAPLOG("SI Strobe Calibration For PMIC 0 Done, (%x, %x)\n", si_sample_ctrl, si_dly);

#ifdef DUAL_PMICS
	if (dual_si_sample_settings == 1) {
		si_sample_ctrl = si_sample_ctrl | 0x400;
		/* TINFO = "[DrvPWRAP_InitSiStrobe] SI Strobe Calibration For PMIC 1............" */
		/* TINFO = "[DrvPWRAP_InitSiStrobe] Scan For The First Valid Sampling Clock Edge." */
		found = 0;
		for (si_en_sel = 0; si_en_sel < 3; si_en_sel++) {
			for (si_ck_sel = 0; si_ck_sel < 2; si_ck_sel++) {
			/*elbrus si_sample_ctrl = (si_sample_ctrl & 0xfffffc1f) |
			*(((si_en_sel << 3) | (si_ck_sel << 2)) << 5);
			*/
				si_sample_ctrl = (si_sample_ctrl & 0xfffc01ff) | (si_en_sel << 15) | (si_ck_sel << 14);
				WRAP_WR32(PMIC_WRAP_SI_SAMPLE_CTRL, si_sample_ctrl);

				pwrap_read_nochk(DEW_READ_TEST, &rdata);
				if (rdata == 0x5aa5) {
					PWRAPLOG("si_en_sel = %x, si_ck_sel = %x, *si_sample_ctrl = %x, rdata = %x\n",
						si_en_sel, si_ck_sel, si_sample_ctrl, rdata);
					found = 1;
					break;
				}
				PWRAPERR("si_en_sel = %x, si_ck_sel = %x, *si_sample_ctrl = %x, rdata = %x\n",
					si_en_sel, si_ck_sel, si_sample_ctrl, rdata);
			}
			if (found == 1)
				break;
		}

		if (found == 0) {
			result_faulty |= 0x2;
			PWRAPERR("2.result_faulty = %d\n", result_faulty);
		}
		/* TINFO = "[DrvPWRAP_InitSiStrobe] Search For The Data Boundary." */
		for (si_dly = 0; si_dly < 8; si_dly++) {
			pwrap_write_nochk(EXT_RG_SPI_CON2, si_dly);

			/*pwrap_read_nochk(EXT_DEW_READ_TEST, &rdata);*/

			error = 0;
			for (i = 0; i < 30; i++) {
				pwrap_write_nochk(EXT_DEW_WRITE_TEST, test_data[i]);
				pwrap_read_nochk(EXT_DEW_WRITE_TEST, &rdata);
				if ((rdata & 0x7fff) != (test_data[i] & 0x7fff)) {
					PWRAPERR("si_dly = %x, *PMIC_WRAP_RESERVED = %x, rdata = %x\n",
						si_dly, reserved, rdata);
					PWRAPERR("si_dly = %x, *EXT_RG_SPI_CON2 = %x, rdata = %x\n",
						si_dly, si_dly, rdata);
					error = 1;
					break;
				}
			}
			if (error == 1)
				break;

			PWRAPLOG("si_dly = %x, *PMIC_WRAP_RESERVED = %x, rdata = %x\n", si_dly, reserved, rdata);
			PWRAPLOG("si_dly = %x, *EXT_RG_SPI_CON2 = %x, rdata = %x\n", si_dly, si_dly, rdata);
		}

		/* TINFO = "[DrvPWRAP_InitSiStrobe] Change The Sampling Clock Edge To The Next One." */
		/*si_sample_ctrl = (si_sample_ctrl & 0xfffffc1f) | ((((si_en_sel << 1) | si_ck_sel) + 1) << 7);*/
		si_sample_ctrl = si_sample_ctrl + 0x4000;
		WRAP_WR32(PMIC_WRAP_SI_SAMPLE_CTRL, si_sample_ctrl);
		if (si_dly == 8)
			si_dly--;
		PWRAPLOG("For PMIC 1 Done, *PMIC_WRAP_SI_SAMPLE_CTRL = %x, *PMIC_WRAP_RESERVED = %x\n",
			si_sample_ctrl, reserved);
		PWRAPLOG("For PMIC 1 Done, *PMIC_WRAP_SI_SAMPLE_CTRL = %x, *EXT_RG_SPI_CON2 = %x\n",
			si_sample_ctrl, si_dly);
	}

	if (result_faulty != 0)
		return result_faulty;

	/* Read Test */
	pwrap_read_nochk(DEW_READ_TEST, &rdata);
	if (rdata != 0x5aa5) {
		PWRAPERR("[_pwrap_init_dio] Read Test Failed, DEW_READ_TEST rdata = %x, exp = 0x5aa5\n",
			rdata);
		return 0x10;
	}
#ifdef DUAL_PMICS
	pwrap_read_nochk(EXT_DEW_READ_TEST, &rdata);
	if (rdata != 0x5aa5) {
		PWRAPERR("[_pwrap_init_dio] Read Test Failed, EXT_DEW_READ_TEST rdata = %x, exp = 0x5aa5\n",
			rdata);
		return 0x11;
	}
#endif

#endif

	return 0;
}


static int __pwrap_InitSPISLV(void)
{
	pwrap_write_nochk(FILTER_CON0, 0xf); /* turn on IO filter function */
	pwrap_write_nochk(TOP_CKHWEN_CON0_SET, 0x80); /* turn on SPI slave DCM */
	pwrap_write_nochk(SMT_CON1, 0xf); /* turn on IO SMT function to improve noise immunity */
	pwrap_write_nochk(GPIO_PULLEN0_CLR, 0x3c0); /* turn off IO pull function for power saving */
	#ifdef DUAL_PMICS
	pwrap_write_nochk(EXT_FILTER_CON0, 0xf); /* turn on IO filter function */
	pwrap_write_nochk(EXT_TOP_CKHWEN_CON0_SET, 0x80); /* turn on SPI slave DCM */
	pwrap_write_nochk(EXT_SMT_CON1, 0xf); /* turn on IO SMT function to improve noise immunity */
	#endif

	return 0;
}

/******************************************************
 * Function : _pwrap_reset_spislv()
 * Description :
 * Parameter :
 * Return :
 ******************************************************/
static s32 _pwrap_reset_spislv(void)
{
	u32 ret = 0;
	u32 return_value = 0;

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, DISABLE_ALL);
	WRAP_WR32(PMIC_WRAP_WRAP_EN, DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL, MANUAL_MODE);
	WRAP_WR32(PMIC_WRAP_MAN_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_DIO_EN, DISABLE);

	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_CSL << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_CSH << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));
	return_value =
		wait_for_state_ready_init(wait_for_sync, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);

	if (return_value != 0) {
		PWRAPERR("_pwrap_reset_spislv fail,return_value=%x\n", return_value);
		ret = E_PWR_TIMEOUT;
		goto timeout;
	}

	WRAP_WR32(PMIC_WRAP_MAN_EN, DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL, WRAPPER_MODE);

timeout:
	WRAP_WR32(PMIC_WRAP_MAN_EN, DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL, WRAPPER_MODE);
	return ret;
}

static s32 _pwrap_init_reg_clock(u32 regck_sel)
{
	u32 rdata;

#ifndef SLV_CLK_1M
#ifndef DUAL_PMICS
		/* Set Read Dummy Cycle Number (Slave Clock is 18MHz) */
		_pwrap_wacs2_nochk(1, DEW_RDDMY_NO, 0x8, &rdata);
		WRAP_WR32(PMIC_WRAP_RDDMY, 0x8);
		PWRAPLOG("NO_SLV_CLK_1M Set Read Dummy Cycle\n");
#else
		_pwrap_wacs2_nochk(1, DEW_RDDMY_NO, 0x8, &rdata);
		_pwrap_wacs2_nochk(1, EXT_DEW_RDDMY_NO, 0x8, &rdata);
		WRAP_WR32(PMIC_WRAP_RDDMY, 0x0808);
		PWRAPLOG("NO_SLV_CLK_1M Set Read Dummy Cycle dual_pmics\n");
#endif
#else
#ifndef DUAL_PMICS
		/* Set Read Dummy Cycle Number (Slave Clock is 1MHz) */
		_pwrap_wacs2_nochk(1, DEW_RDDMY_NO, 0x68, &rdata);
		WRAP_WR32(PMIC_WRAP_RDDMY, 0x68);
		PWRAPLOG("SLV_CLK_1M Set Read Dummy Cycle\n");
#else
		_pwrap_wacs2_nochk(1, DEW_RDDMY_NO, 0x68, &rdata);
		_pwrap_wacs2_nochk(1, EXT_DEW_RDDMY_NO, 0x68, &rdata);
		WRAP_WR32(PMIC_WRAP_RDDMY, 0x6868);
		PWRAPLOG("SLV_CLK_1M Set Read Dummy Cycle dual_pmics\n");
#endif
#endif

		/* Config SPI Waveform according to reg clk */
		if (regck_sel == 1) { /* Slave Clock is 18MHz */
			/* wait data written into register => 4T_PMIC:
			* CSHEXT_WRITE_START+EXT_CK+CSHEXT_WRITE_END+CSLEXT_START
*/
			WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0x0202);
			WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0x0);
			WRAP_WR32(PMIC_WRAP_CSLEXT_WRITE, 0x200);
			WRAP_WR32(PMIC_WRAP_CSLEXT_READ, 0x0);  /* for PMIC site synchronizer use*/
		} else { /*Safe Mode*/
			WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0x0f0f);
			WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0x0f0f);
			WRAP_WR32(PMIC_WRAP_CSLEXT_WRITE, 0x0f0f);
			WRAP_WR32(PMIC_WRAP_CSLEXT_READ, 0x0f0f);
		}

		return 0;

}

static int _pwrap_wacs2_write_test(int pmic_no)
{
	u32 rdata;

	if (pmic_no == 0) {
		pwrap_write_nochk(DEW_WRITE_TEST, 0xa55a);
		pwrap_read_nochk(DEW_WRITE_TEST, &rdata);
		if (rdata != 0xa55a) {
			PWRAPREG("Error: Write Test Failed. DEW_WRITE_TEST rdata = %x, exp = 0xa55a\n", rdata);
			return E_PWR_WRITE_TEST_FAIL;
		}
	}

	#ifdef DUAL_PMICS
	if (pmic_no == 1) {
		pwrap_write_nochk(DEW_WRITE_TEST, 0xa55a);
		pwrap_read_nochk(DEW_WRITE_TEST, &rdata);
		if (rdata != 0xa55a) {
			PWRAPREG("Error: Write Test Failed. EXT_DEW_WRITE_TEST rdata = %x, exp = 0xa55a\n", rdata);
			return E_PWR_WRITE_TEST_FAIL;
		}
	}
	#endif

	return 0;
}
static u32 pwrap_read_test(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	/* Read Test */
	return_value = pwrap_wacs2_read(DEW_WRITE_TEST, &rdata);
	if (rdata != MT6335_DEFAULT_VALUE_READ_TEST) {
		PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata, return_value);
		return E_PWR_READ_TEST_FAIL;
	}
	PWRAPREG("Read Test pass,return_value=%d\n", return_value);

	return 0;
}
static u32 pwrap_write_test(void)
{
	u32 rdata = 0;
	u32 sub_return = 0;
	u32 sub_return1 = 0;

	/* Write test using WACS2 */
	PWRAPREG("start pwrap_write\n");
	sub_return = pwrap_wacs2_write(DEW_WRITE_TEST, MT6335_DEFAULT_VALUE_READ_TEST);
	PWRAPREG("after pwrap_write\n");
	sub_return1 = pwrap_wacs2_read(DEW_WRITE_TEST, &rdata);
	if ((rdata != MT6335_DEFAULT_VALUE_READ_TEST) || (sub_return != 0) || (sub_return1 != 0)) {
		PWRAPREG("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n", rdata, sub_return,
			sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
	PWRAPREG("write Test pass\n");

	return 0;
}
s32 pwrap_init(void)
{
	s32 sub_return = 0;
	u32 rdata = 0x0;

	PWRAPFUC();
#ifdef CONFIG_OF
	sub_return = pwrap_of_iomap();
	if (sub_return)
		return sub_return;
#endif

	PWRAPLOG("pwrap_init start!!!!!!!!!!!!!\n");

	__pwrap_spi_clk_set();

	PWRAPLOG("__pwrap_spi_clk_set ok\n");

	/* Enable DCM */
#ifndef PWRAP_DCM_ALL_ON
	WRAP_WR32(PMIC_WRAP_DCM_EN, 1);
#else
	WRAP_WR32(PMIC_WRAP_DCM_EN, 0x3fff);
#endif

	/* no debounce */
	WRAP_WR32(PMIC_WRAP_DCM_DBC_PRD, 0);
	PWRAPLOG("Enable DCM ok\n");

	/* Reset SPISLV */
	sub_return = _pwrap_reset_spislv();
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_reset_spislv fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_RESET_SPI;
	}
	PWRAPLOG("Reset SPISLV ok\n");

	/* Enable WRAP */
	WRAP_WR32(PMIC_WRAP_WRAP_EN, ENABLE);
	PWRAPLOG("Enable WRAP  ok\n");

	/* Enable WACS2 */
	WRAP_WR32(PMIC_WRAP_WACS2_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0x2000); /* ONLY WACS2 */
	PWRAPLOG("Enable WACSW2  ok\n");

	/* SPI Slave Configuration */
	sub_return = __pwrap_InitSPISLV();
	if (sub_return != 0) {
		PWRAPERR("Error: DrvPWRAP_InitSPISLV Failed, sub_return = %x", sub_return);
		return -1;
	}


	/* SPI Waveform Configuration. 0:safe mode, 1:18MHz */
	sub_return = _pwrap_init_reg_clock(1);
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_init_reg_clock fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_REG_CLOCK;
	}
	PWRAPLOG("_pwrap_init_reg_clock ok\n");

	/* Enable DIO mode */
	sub_return = _pwrap_init_dio(1);
	if (sub_return != 0) {
		PWRAPERR("_pwrap_init_dio test error,error code=%x, sub_return=%x\n", 0x11, sub_return);
		return E_PWR_INIT_DIO;
	}
	PWRAPLOG("_pwrap_init_dio ok\n");

	/* Input data calibration flow; */
	sub_return = _pwrap_init_sistrobe(1);
	if (sub_return != 0) {
		PWRAPERR("error,DrvPWRAP_InitSiStrobe fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_SIDLY;
	}
	PWRAPLOG("_pwrap_init_sistrobe ok\n");

	/* Enable Encryption */
	sub_return = _pwrap_init_cipher();
	if (sub_return != 0) {
		PWRAPERR("Enable Encryption fail, return=%x\n", sub_return);
		return E_PWR_INIT_CIPHER;
	}
	PWRAPLOG("_pwrap_init_cipher ok\n");

	/*  Write test using WACS2.  check Wtiet test default value */
	sub_return = _pwrap_wacs2_write_test(0);
	if (rdata != 0) {
		PWRAPERR("[_pwrap_wacs2_write_test(0)] test fail\n");
		return E_PWR_INIT_WRITE_TEST;
	}
#ifdef DUAL_PMICS
	sub_return = _pwrap_wacs2_write_test(1);
	if (rdata != 0) {
		PWRAPERR("[_pwrap_wacs2_write_test(1)] test fail\n");
		return E_PWR_INIT_WRITE_TEST;
	}
#endif
	PWRAPLOG("_pwrap_wacs2_write_test ok\n");

	/* Status update function initialization
	* 1. Signature Checking using CRC (CRC 0 only)
	* 2. EINT update
	* 3. Read back Auxadc thermal data for GPS
	*/
	_pwrap_InitStaUpd();
	PWRAPLOG("_pwrap_InitStaUpd ok\n");

	/* PMIC_WRAP starvation setting */
	_pwrap_starve_set();
	PWRAPLOG("_pwrap_starve_set ok\n");

	/* PMIC_WRAP enables */
	_pwrap_enable();
	PWRAPLOG("_pwrap_enable ok\n");

	/* Backward Compatible Settings */
	/* WRAP_WR32(PMIC_WRAP_BWC_OPTIONS,  WRAP_RD32(PMIC_WRAP_BWC_OPTIONS) | 0x8); */

	/* Initialization Done */
	WRAP_WR32(PMIC_WRAP_INIT_DONE0, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE1, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE2, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE3, 0x1);

	PWRAPLOG("pwrap_init Done!!!!!!!!!\n");

#ifdef CONFIG_OF
	pwrap_of_iounmap();
#endif

/* for simulation runtime ipi test */
#if 0
	mdelay(20000);
	for (i = 0; i < 5; i++) {
		pwrap_ut(1);
		pwrap_ut(2);
		pwrap_ut(3);
	}
#endif
	return 0;
}

/*-------------------pwrap debug---------------------*/
static inline void pwrap_dump_ap_register(void)
{
	u32 i = 0;
	u32 *reg_addr;
	u32 reg_value = 0;

	PWRAPREG("dump pwrap register\n");
	for (i = 0; i <= PMIC_WRAP_REG_RANGE; i++) {
		reg_addr = (PMIC_WRAP_BASE + i * 4);
		reg_value = WRAP_RD32(((unsigned int *) (PMIC_WRAP_BASE + i * 4)));

		PWRAPREG("reg_addr:0x%p = 0x%x\n", reg_addr, reg_value);
	}
}

static inline void pwrap_dump_all_register(void)
{
	pwrap_dump_ap_register();
}

/*********************** external API for pmic_wrap user ************************/
s32 pwrap_wacs2_read(u32  adr, u32 *rdata)
{
	pwrap_wacs2_hal(0, adr, 0, rdata);
	return 0;
}

/* Provide PMIC write API */
s32 pwrap_wacs2_write(u32  adr, u32  wdata)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	u32 flag;

	flag = WRITE_CMD | (1 << WRITE_PMIC);
	pwrap_wacs2_ipi(adr, wdata, flag);
#else
	pwrap_wacs2_hal(1, adr, wdata, 0);
#endif
	return 0;
}

s32 pwrap_wacs2_audio_read(u32  adr, u32 *rdata)
{
	pwrap_wacs2_hal(0, adr, 0, rdata);
	return 0;
}

s32 pwrap_wacs2_audio_write(u32  adr, u32  wdata)
{
	pwrap_wacs2_hal(1, adr, wdata, 0);
	return 0;
}

/*---------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static int pwrap_of_iomap(void)
{
	/*
	 * Map the address of the following register base:
	 * INFRACFG_AO, TOPCKGEN, SCP_CLK_CTRL, SCP_PMICWP2P
	 */

	struct device_node *infracfg_ao_node;
	struct device_node *topckgen_node;

	infracfg_ao_node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (!infracfg_ao_node) {
		pr_warn("get INFRACFG_AO failed\n");
		return -ENODEV;
	}

	infracfg_ao_base = of_iomap(infracfg_ao_node, 0);
	if (!infracfg_ao_base) {
		pr_warn("INFRACFG_AO iomap failed\n");
		return -ENOMEM;
	}

	topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (!topckgen_node) {
		pr_warn("get TOPCKGEN failed\n");
		return -ENODEV;
	}

	topckgen_base = of_iomap(topckgen_node, 0);
	if (!topckgen_base) {
		pr_warn("TOPCKGEN iomap failed\n");
		return -ENOMEM;
	}
	return 0;
}

static void pwrap_of_iounmap(void)
{
	iounmap(topckgen_base);
}
#endif

static int is_pwrap_init_done(void)
{
	int ret = 0;

	ret = WRAP_RD32(PMIC_WRAP_INIT_DONE2);
	PWRAPLOG("is_pwrap_init_done %d\n", ret);
	if (ret != 0)
		return 0;

	ret = pwrap_init();
	if (ret != 0) {
		PWRAPERR("init error (%d)\n", ret);
		pwrap_dump_all_register();
		return ret;
	}
	PWRAPLOG("init successfully done (%d)\n\n", ret);
	return ret;
}


#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
/* pmic wrap used 4*4Byte IPI data buf,
 * buf0 - adr,
 * buf1 - bit0: 0 read,  1 write; bit1: 0 pmic_wrap, 1 pmic.
 * buf3 - rdata, buf4 reserved
*/
static s32 pwrap_wacs2_ipi(u32  adr, u32 wdata, u32 flag)
{
	int ipi_data_ret = 0, err;
	unsigned int ipi_buf[32];

	mutex_lock(&pwrap_lock);
	ipi_buf[0] = adr;
	ipi_buf[1] = flag;
	ipi_buf[2] = wdata;

	err = sspm_ipi_send_sync(IPI_ID_PMIC, 1, (void *)ipi_buf, 0, &ipi_data_ret);
	if (err != 0)
		PWRAPERR("ipi_write error: %d\n", err);
	else
		PWRAPLOG("ipi_write success: %x\n", ipi_data_ret);

	mutex_unlock(&pwrap_lock);
	return 0;
}

static int pwrap_ipi_register(void)
{
	int ret;
	int retry = 0;
	ipi_action_t pwrap_isr;

	pwrap_isr.data = (void *)pwrap_recv_data;

	do {
		retry++;
		ret = sspm_ipi_recv_registration(IPI_ID_PMIC, &pwrap_isr);
	} while ((ret != 0) && (retry < 10));
	if (retry >= 10)
		PWRAPERR("pwrap_ipi_register fail\n");
	return 0;
}
#endif

/*Interrupt handler function*/
static int g_wrap_wdt_irq_count;
static int g_case_flag;
static irqreturn_t mt_pmic_wrap_irq(int irqno, void *dev_id)
{
	unsigned long flags = 0;

	PWRAPFUC();
	if ((WRAP_RD32(PMIC_WRAP_INT0_FLG) & 0x01) == 0x01) {
		g_wrap_wdt_irq_count++;
		g_case_flag = 0;
		PWRAPREG("g_wrap_wdt_irq_count=%d\n", g_wrap_wdt_irq_count);

	} else {
		g_case_flag = 1;
	}
/* Check INT1_FLA Status, if wrong status, dump all register info */
	if ((WRAP_RD32(PMIC_WRAP_INT1_FLG) & 0xffffff) != 0) {
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		pwrap_wacs2_ipi(0x10010000 + 0xD8, 0xffffffff, (WRITE_CMD | WRITE_PMIC_WRAP));
#else
		WRAP_WR32(PMIC_WRAP_INT1_CLR, 0xffffffff);
#endif
		PWRAPREG("INT1_FLG status Wrong,value=0x%x\n", WRAP_RD32(PMIC_WRAP_INT1_FLG));
	}
	spin_lock_irqsave(&wrp_lock, flags);
	pwrap_dump_all_register();

	/* clear interrupt flag */
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
			pwrap_wacs2_ipi(0x10010000 + 0xc8, 0xffffffff, (WRITE_CMD | WRITE_PMIC_WRAP));
#else
			WRAP_WR32(PMIC_WRAP_INT0_CLR, 0xffffffff);
#endif

	PWRAPREG("INT0 flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT0_EN));
	if (g_wrap_wdt_irq_count == 10 || g_case_flag == 1)
		WARN_ON(1);

	spin_unlock_irqrestore(&wrp_lock, flags);

	return IRQ_HANDLED;
}

static void pwrap_int_test(void)
{
	u32 rdata1 = 0;
	u32 rdata2 = 0;

	while (1) {
		rdata1 = WRAP_RD32(PMIC_WRAP_EINT_STA);
		pwrap_read(INT_STA, &rdata2);
		PWRAPREG
			("Pwrap INT status check,PMIC_WRAP_EINT_STA=0x%x,INT_STA[0x01B4]=0x%x\n",
			 rdata1, rdata2);
		msleep(500);
	}
}

/*---------------------------------------------------------------------------*/
static s32 mt_pwrap_show_hal(char *buf)
{
	PWRAPFUC();
	return snprintf(buf, PAGE_SIZE, "%s\n", "no implement");
}

/*---------------------------------------------------------------------------*/
static s32 mt_pwrap_store_hal(const char *buf, size_t count)
{
	u32 reg_value = 0;
	u32 reg_addr = 0;
	u32 return_value = 0;
	u32 ut_test = 0;

	if (!strncmp(buf, "-h", 2)) {
		PWRAPREG
	("PWRAP debug: [-dump_reg][-trace_wacs2][-init][-rdap][-wrap][-rdpmic][-wrpmic][-readtest][-writetest]\n");
		PWRAPREG("PWRAP UT: [1][2]\n");
	} else if (!strncmp(buf, "-dump_reg", 9)) {
		pwrap_dump_all_register();
	} else if (!strncmp(buf, "-trace_wacs2", 12)) {
		/* pwrap_trace_wacs2(); */
	} else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	} else if (!strncmp(buf, "-rdap", 5) && (sscanf(buf + 5, "%x", &reg_addr) == 1)) {
		/* pwrap_read_reg_on_ap(reg_addr); */
	} else if (!strncmp(buf, "-wrap", 5)
		   && (sscanf(buf + 5, "%x %x", &reg_addr, &reg_value) == 2)) {
		/* pwrap_write_reg_on_ap(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-rdpmic", 7) && (sscanf(buf + 7, "%x", &reg_addr) == 1)) {
		/* pwrap_read_reg_on_pmic(reg_addr); */
	} else if (!strncmp(buf, "-wrpmic", 7)
		   && (sscanf(buf + 7, "%x %x", &reg_addr, &reg_value) == 2)) {
		/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-readtest", 9)) {
		pwrap_read_test();
	} else if (!strncmp(buf, "-writetest", 10)) {
		pwrap_write_test();
	} else if (!strncmp(buf, "-int", 4)) {
		pwrap_int_test();
	} else if (!strncmp(buf, "-ut", 3) && (sscanf(buf + 3, "%d", &ut_test) == 1)) {
		pwrap_ut(ut_test);
	} else {
		PWRAPREG("wrong parameter\n");
	}
	return count;
}

static int __init pwrap_hal_init(void)
{
	s32 ret = 0;
#ifdef CONFIG_OF
	u32 pwrap_irq;
	struct device_node *pwrap_node;

	PWRAPLOG("mt_pwrap_init++++\n");
	pwrap_node = of_find_compatible_node(NULL, NULL, "mediatek,pwrap");
	if (!pwrap_node) {
		pr_warn("PWRAP get node failed\n");
		return -ENODEV;
	}

	pwrap_base = of_iomap(pwrap_node, 0);
	if (!pwrap_base) {
		pr_warn("PWRAP iomap failed\n");
		return -ENOMEM;
	}

	pwrap_irq = irq_of_parse_and_map(pwrap_node, 0);
	if (!pwrap_irq) {
		pr_warn("PWRAP get irq fail\n");
		return -ENODEV;
	}
	pr_warn("PWRAP reg: 0x%p,  irq: %d\n", pwrap_base, pwrap_irq);
#endif
	mt_wrp = get_mt_pmic_wrap_drv();
	mt_wrp->store_hal = mt_pwrap_store_hal;
	mt_wrp->show_hal = mt_pwrap_show_hal;
	mt_wrp->wacs2_hal = pwrap_wacs2_hal;

	pwrap_of_iomap();
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		pwrap_ipi_register();
#endif

	if (is_pwrap_init_done() == 0) {
#ifdef PMIC_WRAP_NO_PMIC
#else
		ret =
			request_irq(MT_PMIC_WRAP_IRQ_ID, mt_pmic_wrap_irq, IRQF_TRIGGER_HIGH,
				PMIC_WRAP_DEVICE, 0);
#endif
		if (ret) {
			PWRAPERR("register IRQ failed (%d)\n", ret);
			return ret;
		}
	} else {
		PWRAPERR("not init (%d)\n", ret);
	}

	PWRAPLOG("mt_pwrap_init----\n");
	return ret;
}

/********************************************************************************************/
/* extern API for PMIC driver, INT related control, this INT is for PMIC chip to AP */
/********************************************************************************************/

u32 mt_pmic_wrap_eint_status(void)
{
	return WRAP_RD32(PMIC_WRAP_EINT_STA);
}

void mt_pmic_wrap_eint_clr(int offset)
{
	if ((offset < 0) || (offset > 3))
		PWRAPERR("clear EINT flag error, only 0-3 bit\n");
	else
		WRAP_WR32(PMIC_WRAP_EINT_CLR, (1 << offset));
}


postcore_initcall(pwrap_hal_init);

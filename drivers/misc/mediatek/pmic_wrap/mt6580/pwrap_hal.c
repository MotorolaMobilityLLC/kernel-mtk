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

/******************************************************************************
 * pwrap_hal.c - Linux pmic_wrapper Driver,hardware_dependent driver
 *
 *
 * DESCRIPTION:
 *     This file provid the other drivers PMIC wrapper relative functions
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
#include <mach/mt_pmic_wrap.h>
#include "pwrap_hal.h"


#define PMIC_WRAP_DEVICE "pmic_wrap"

/*-----start-- global variable-------------------------------------------------*/
#ifdef CONFIG_OF
void __iomem *pwrap_base;
void __iomem *topckgen_base;
void __iomem *toprgu_reg_base;
#endif

static struct mt_pmic_wrap_driver *mt_wrp;

static DEFINE_SPINLOCK(wrp_lock);
static DEFINE_SPINLOCK(wrp_lock_isr);
/* ----------interral API ------------------------ */
static s32 _pwrap_init_dio(u32 dio_en);
static s32 _pwrap_init_cipher(void);
static s32 _pwrap_init_reg_clock(u32 regck_sel);

static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata);
static s32 pwrap_write_nochk(u32 adr, u32 wdata);
static s32 pwrap_read_nochk(u32 adr, u32 *rdata);
#ifdef CONFIG_OF
static int pwrap_of_iomap(void);
static void pwrap_of_iounmap(void);
#endif

#ifdef PMIC_WRAP_NO_PMIC
/*-pwrap debug--------------------------------------------------------------------------*/
static inline void pwrap_dump_all_register(void)
{
}

/********************************************************************************************/
/* extern API for PMIC driver, INT related control, this INT is for PMIC chip to AP */
/********************************************************************************************/
u32 mt_pmic_wrap_eint_status(void)
{
	return 0;
}

void mt_pmic_wrap_eint_clr(int offset)
{
}

/* -------------------------------------------------------- */
/* Function : pwrap_wacs2_hal() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 pwrap_wacs2_hal(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	PWRAPERR("there is no PMIC real chip,PMIC_WRAP do Nothing\n");
	return 0;
}

/*
 *pmic_wrap init,init wrap interface
 *
 */
s32 pwrap_init(void)
{
	PWRAPLOG("There is no PMIC real chip, PMIC_WRAP not init\n");
	return 0;
}

/* EXPORT_SYMBOL(pwrap_init); */
/*Interrupt handler function*/
static irqreturn_t mt_pmic_wrap_irq(int irqno, void *dev_id)
{
	unsigned long flags = 0;

	PWRAPFUC();
	PWRAPREG("dump pwrap register\n");
	spin_lock_irqsave(&wrp_lock, flags);
	/* ----------------------------------------------------------------------- */
	pwrap_dump_all_register();
	/* raise the priority of WACS2 for AP */
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 1 << 3);

	/* ----------------------------------------------------------------------- */
	/* clear interrupt flag */
	WRAP_WR32(PMIC_WRAP_INT_CLR, 0xffffffff);
	PWRAPREG("INT flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT_EN));
	/* BUG_ON(1); */
	spin_unlock_irqrestore(&wrp_lock, flags);
	return IRQ_HANDLED;
}

static u32 pwrap_read_test(void)
{
	return 0;
}

static u32 pwrap_write_test(void)
{
	return 0;
}

static void pwrap_ut(u32 ut_test)
{
	switch (ut_test) {
	case 1:
		/* pwrap_wacs2_para_test(); */
		pwrap_write_test();
		break;
	case 2:
		/* pwrap_wacs2_para_test(); */
		pwrap_read_test();
		break;

	default:
		PWRAPREG("default test.\n");
		break;
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
		    ("PWRAP debug: [-dump_reg][-trace_wacs2][-init][-rdpmic][-wrpmic][-readtest][-writetest]\n");
		PWRAPREG("PWRAP UT: [1][2]\n");
	}
	/* ----------pwrap debug---------- */
	else if (!strncmp(buf, "-dump_reg", 9)) {
		pwrap_dump_all_register();
	} else if (!strncmp(buf, "-trace_wacs2", 12)) {
		/* pwrap_trace_wacs2(); */
	} else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	} else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf + 7, "%x", &reg_addr))) {
		/* pwrap_read_reg_on_pmic(reg_addr); */
	} else if (!strncmp(buf, "-wrpmic", 7)
		   && (2 == sscanf(buf + 7, "%x %x", &reg_addr, &reg_value))) {
		/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-readtest", 9)) {
		pwrap_read_test();
	} else if (!strncmp(buf, "-writetest", 10)) {
		pwrap_write_test();
	}
	/* ----------pwrap UT---------- */
	else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf + 3, "%d", &ut_test))) {
		pwrap_ut(ut_test);
	} else {
		PWRAPREG("wrong parameter\n");
	}
	return count;
}

/*---------------------------------------------------------------------------*/
#else
/*-pwrap debug--------------------------------------------------------------------------*/
static inline void pwrap_dump_ap_register(void)
{
	u32 i = 0;

	PWRAPREG("dump pwrap register, base=0x%p\n", PMIC_WRAP_BASE);
	PWRAPREG("address     :   3 2 1 0    7 6 5 4    B A 9 8    F E D C\n");
	for (i = 0; i <= PMIC_WRAP_REG_RANGE; i += 4) {
		PWRAPREG("offset 0x%.3x:0x%.8x 0x%.8x 0x%.8x 0x%.8x\n", i * 4,
			 WRAP_RD32(PMIC_WRAP_BASE + (i * 4) + 0),
			 WRAP_RD32(PMIC_WRAP_BASE + (i * 4) + 0x4),
			 WRAP_RD32(PMIC_WRAP_BASE + (i * 4) + 0x8),
			 WRAP_RD32(PMIC_WRAP_BASE + (i * 4) + 0xC));
	}
	/* PWRAPREG("elapse_time=%llx(ns)\n",elapse_time); */
}

static inline void pwrap_dump_pmic_register(void)
{
#if 0
	u32 i = 0;
	u32 reg_addr = 0;
	u32 reg_value = 0;

	PWRAPREG("dump dewrap register\n");
	for (i = 0; i <= 14; i++) {
		reg_addr = (DEW_BASE + i * 4);
		reg_value = pwrap_read_nochk(reg_addr, &reg_value);
		PWRAPREG("0x%x=0x%x\n", reg_addr, reg_value);
	}
#endif
}

static inline void pwrap_dump_all_register(void)
{
	pwrap_dump_ap_register();
	pwrap_dump_pmic_register();
}

#if 0
static void __pwrap_soft_reset(void)
{
	PWRAPLOG("start reset wrapper\n");
	PWRAP_SOFT_RESET;
	PWRAPLOG("the reset register =%x\n", WRAP_RD32(INFRA_GLOBALCON_RST0));
	PWRAPLOG("PMIC_WRAP_STAUPD_GRPEN =0x%x,it should be equal to 0xc\n",
		 WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN));
	/* clear reset bit */
	PWRAP_CLEAR_SOFT_RESET_BIT;
}
#endif
/******************************************************************************
  wrapper timeout
 ******************************************************************************/
#define PWRAP_TIMEOUT
#ifdef PWRAP_TIMEOUT
static u64 _pwrap_get_current_time(void)
{
	return sched_clock();	/* /TODO: fix me */
}

/* u64 elapse_time=0; */

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
		timeout_time_ns = 10000 * 1000;	/* 10000us */
		PWRAPERR("@@@@reset timer! start%lld setting%lld\n", start_time_ns,
			 timeout_time_ns);
	}

	elapse_time = cur_time - start_time_ns;

	/* check if timeout */
	if (timeout_time_ns <= elapse_time) {
		/* timeout */
		PWRAPERR("@@@@Timeout: elapse time%lld,start%lld setting timer%lld\n",
			 elapse_time, start_time_ns, timeout_time_ns);
		return true;
	}
	return false;
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
{				/* ,u64 timeout_ns) */
	return false;
}

static u64 _pwrap_time2ns(u64 time_us)
{
	return 0;
}

#endif
/* ##################################################################### */
/* define macro and inline function (for do while loop) */
/* ##################################################################### */
typedef u32(*loop_condition_fp) (u32);	/* define a function pointer */

static inline u32 wait_for_fsm_idle(u32 x)
{
	return (GET_WACS0_FSM(x) != WACS_FSM_IDLE);
}

static inline u32 wait_for_fsm_vldclr(u32 x)
{
	return (GET_WACS0_FSM(x) != WACS_FSM_WFVLDCLR);
}

static inline u32 wait_for_sync(u32 x)
{
	return (GET_SYNC_IDLE0(x) != WACS_SYNC_IDLE);
}

static inline u32 wait_for_idle_and_sync(u32 x)
{
	return ((GET_WACS2_FSM(x) != WACS_FSM_IDLE) || (GET_SYNC_IDLE2(x) != WACS_SYNC_IDLE));
}

static inline u32 wait_for_wrap_idle(u32 x)
{
	return ((GET_WRAP_FSM(x) != 0x0) || (GET_WRAP_CH_DLE_RESTCNT(x) != 0x0));
}

static inline u32 wait_for_wrap_state_idle(u32 x)
{
	return (GET_WRAP_AG_DLE_RESTCNT(x) != 0);
}

static inline u32 wait_for_man_idle_and_noreq(u32 x)
{
	return ((GET_MAN_REQ(x) != MAN_FSM_NO_REQ) || (GET_MAN_FSM(x) != MAN_FSM_IDLE));
}

static inline u32 wait_for_man_vldclr(u32 x)
{
	return (GET_MAN_FSM(x) != MAN_FSM_WFVLDCLR);
}

static inline u32 wait_for_cipher_ready(u32 x)
{
	return (x != 1);
}

static inline u32 wait_for_stdupd_idle(u32 x)
{
	return (GET_STAUPD_FSM(x) != 0x0);
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
	} while (fp(reg_rdata));	/* IDLE State */
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
			/* pwrap_trace_wacs2(); */
			/* BUG_ON(1); */
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);
		/* if last read command timeout,clear vldclr bit */
		/* read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;write:FSM_REQ-->idle */
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
	} while (fp(reg_rdata));	/* IDLE State */
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
			/* pwrap_trace_wacs2(); */
			/* BUG_ON(1); */
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);
		if (GET_INIT_DONE0(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
		/* if last read command timeout,clear vldclr bit */
		/* read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;write:FSM_REQ-->idle */
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
	} while (fp(reg_rdata));	/* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

/********************************************************************************************/
/* extern API for PMIC driver, INT related control, this INT is for PMIC chip to AP */
/********************************************************************************************/
u32 mt_pmic_wrap_eint_status(void)
{
	return 0;
}

void mt_pmic_wrap_eint_clr(int offset)
{
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
			/* pwrap_trace_wacs2(); */
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		reg_rdata = WRAP_RD32(wacs_register);

		if (GET_INIT_DONE0(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
	} while (fp(reg_rdata));	/* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

/* -------------------------------------------------------- */
/* Function : pwrap_wacs2_hal() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 pwrap_wacs2_hal(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	/* u64 wrap_access_time=0x0; */
	u32 reg_rdata = 0;
	u32 wacs_write = 0;
	u32 wacs_adr = 0;
	u32 wacs_cmd = 0;
	u32 return_value = 0;
	unsigned long flags = 0;
	/* PWRAPFUC(); */
	/* #ifndef CONFIG_MTK_LDVT_PMIC_WRAP */
	/* PWRAPLOG("wrapper access,write=%x,add=%x,wdata=%x,rdata=%x\n",write,adr,wdata,rdata); */
	/* #endif */
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
		if (NULL == rdata) {
			PWRAPERR("rdata is a NULL pointer\n");
			return_value = E_PWR_INVALID_ARG;
			goto FAIL;
		}
		return_value =
		    wait_for_state_ready(wait_for_fsm_vldclr, TIMEOUT_READ, PMIC_WRAP_WACS2_RDATA,
					 &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n", return_value);
			return_value += 1;	/* E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ */
			goto FAIL;
		}
		*rdata = GET_WACS0_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR, 1);
	}
	/* spin_unlock_irqrestore(&wrp_lock,flags); */
FAIL:
	spin_unlock_irqrestore(&wrp_lock, flags);
	if (return_value != 0) {
		PWRAPERR("pwrap_wacs2_hal fail,return_value=%d\n", return_value);
		PWRAPERR("timeout:BUG_ON here\n");
		/* BUG_ON(1); */
	}
	/* wrap_access_time=sched_clock(); */
	/* pwrap_trace(wrap_access_time,return_value,write, adr, wdata,(u32)rdata); */
	return return_value;
}

/* s32 pwrap_wacs2( u32  write, u32  adr, u32  wdata, u32 *rdata ) */
/* { */
/* return pwrap_wacs2_hal(write, adr,wdata,rdata ); */
/* } */
/* EXPORT_SYMBOL(pwrap_wacs2); */
/* s32 pwrap_read( u32  adr, u32 *rdata ) */
/* { */
/* return pwrap_wacs2( PWRAP_READ, adr,0,rdata ); */
/* } */
/* EXPORT_SYMBOL(pwrap_read); */
/*  */
/* s32 pwrap_write( u32  adr, u32  wdata ) */
/* { */
/* return pwrap_wacs2( PWRAP_WRITE, adr,wdata,0 ); */
/* } */
/* EXPORT_SYMBOL(pwrap_write); */
/* ****************************************************************************** */
/* --internal API for pwrap_init------------------------------------------------- */
/* ****************************************************************************** */
/* -------------------------------------------------------- */
/* Function : _pwrap_wacs2_nochk() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */

/* static s32 pwrap_read_nochk( u32  adr, u32 *rdata ) */
static s32 pwrap_read_nochk(u32 adr, u32 *rdata)
{
	return _pwrap_wacs2_nochk(0, adr, 0, rdata);
}

static s32 pwrap_write_nochk(u32 adr, u32 wdata)
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
	/* PWRAPFUC(); */
	/* Check argument validation */
	if ((write & ~(0x1)) != 0)
		return E_PWR_INVALID_RW;
	if ((adr & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;
	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	/* Check IDLE */
	return_value =
	    wait_for_state_ready_init(wait_for_fsm_idle, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA,
				      0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_wacs2_nochk write command fail,return_value=%x\n", return_value);
		return return_value;
	}

	wacs_write = write << 31;
	wacs_adr = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;
	WRAP_WR32(PMIC_WRAP_WACS2_CMD, wacs_cmd);

	if (write == 0) {
		if (NULL == rdata)
			return E_PWR_INVALID_ARG;
		/* wait for read data ready */
		return_value =
		    wait_for_state_ready_init(wait_for_fsm_vldclr, TIMEOUT_WAIT_IDLE,
					      PMIC_WRAP_WACS2_RDATA, &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("_pwrap_wacs2_nochk read fail,return_value=%x\n", return_value);
			return return_value;
		}
		*rdata = GET_WACS0_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR, 1);
	}
	return 0;
}

/* -------------------------------------------------------- */
/* Function : _pwrap_init_dio() */
/* Description :call it in pwrap_init,mustn't check init done */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 _pwrap_init_dio(u32 dio_en)
{
	u32 arb_en_backup = 0x0;
	u32 rdata = 0x0;
	u32 return_value = 0;

	/* PWRAPFUC(); */
	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);	/* only WACS2 */

#ifdef SLV_6350
	pwrap_write_nochk(MT6350_DEW_DIO_EN, dio_en);
#endif

	/* Check IDLE & INIT_DONE in advance */
	return_value =
	    wait_for_state_ready_init(wait_for_idle_and_sync, TIMEOUT_WAIT_IDLE,
				      PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_init_dio fail,return_value=%x\n", return_value);
		return return_value;
	}
	WRAP_WR32(PMIC_WRAP_DIO_EN, dio_en);
	/* Read Test */
#ifdef SLV_6350
	pwrap_read_nochk(MT6350_DEW_READ_TEST, &rdata);
	if (rdata != MT6350_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("[Dio_mode][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=0x5aa5\n",
			 dio_en, rdata);
		return E_PWR_READ_TEST_FAIL;
	}
#endif

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);
	return 0;
}

/* -------------------------------------------------------- */
/* Function : _pwrap_init_cipher() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 _pwrap_init_cipher(void)
{
	u32 arb_en_backup = 0;
	u32 rdata = 0;
	u32 return_value = 0;
	u32 start_time_ns = 0, timeout_ns = 0;
	/* PWRAPFUC(); */
	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);	/* only WACS0 */

	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 0);
	WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL, 2);
	WRAP_WR32(PMIC_WRAP_CIPHER_EN, 1);

	/* Config CIPHER @ PMIC */
#ifdef SLV_6350
	pwrap_write_nochk(MT6350_DEW_CIPHER_SWRST, 0x1);
	pwrap_write_nochk(MT6350_DEW_CIPHER_SWRST, 0x0);
	pwrap_write_nochk(MT6350_DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(MT6350_DEW_CIPHER_IV_SEL, 0x2);
	pwrap_write_nochk(MT6350_DEW_CIPHER_EN, 0x1);
#endif

	/* wait for cipher data ready@AP */
	return_value =
	    wait_for_state_ready_init(wait_for_cipher_ready, TIMEOUT_WAIT_IDLE,
				      PMIC_WRAP_CIPHER_RDY, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher data ready@AP fail,return_value=%x\n", return_value);
		return return_value;
	}
	/* wait for cipher data ready@PMIC */
#ifdef SLV_6350
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait for cipher data ready@PMIC\n");
			/* pwrap_dump_all_register(); */
			/* return E_PWR_WAIT_IDLE_TIMEOUT; */
		}
		pwrap_read_nochk(MT6350_DEW_CIPHER_RDY, &rdata);
	} while (rdata != 0x1);	/* cipher_ready */

	pwrap_write_nochk(MT6350_DEW_CIPHER_MODE, 0x1);
#endif

	/* wait for cipher mode idle */
	return_value =
	    wait_for_state_ready_init(wait_for_idle_and_sync, TIMEOUT_WAIT_IDLE,
				      PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher mode idle fail,return_value=%x\n", return_value);
		return return_value;
	}
	WRAP_WR32(PMIC_WRAP_CIPHER_MODE, 1);

	/* Read Test */
#ifdef SLV_6350
	pwrap_read_nochk(MT6350_DEW_READ_TEST, &rdata);
	if (rdata != MT6350_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	}
#endif

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);
	return 0;
}

/* -------------------------------------------------------- */
/* Function : _pwrap_init_sistrobe() */
/* Description : Initialize SI_CK_CON and SIDLY */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 _pwrap_init_sistrobe(void)
{
	u32 arb_en_backup = 0;
	u32 rdata = 0;
	u32 i = 0;
	s32 ind = 0;
	u32 tmp1 = 0;
	u32 tmp2 = 0;
	u32 result_faulty = 0;
	u32 result[2] = { 0, 0 };
	s32 leading_one[2] = { -1, -1 };
	s32 tailing_one[2] = { -1, -1 };

	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);	/* only WACS2 */

	/* -------------------------------------------------------- */
	/* Scan all possible input strobe by READ_TEST              */
	/* -------------------------------------------------------- */
	for (ind = 0; ind < 24; ind++) {	/* 24 sampling clock edge */
		WRAP_WR32(PMIC_WRAP_SI_CK_CON, (ind >> 2) & 0x7);
		WRAP_WR32(PMIC_WRAP_SIDLY, 0x3 - (ind & 0x3));
#ifdef SLV_6350
		_pwrap_wacs2_nochk(0, MT6350_DEW_READ_TEST, 0, &rdata);
		if (rdata == MT6350_DEFAULT_VALUE_READ_TEST) {
			PWRAPLOG
			    ("_pwrap_init_sistrobe [Read Test of MT6350] pass,index=%d rdata=%x\n",
			     ind, rdata);
			result[0] |= (0x1 << ind);
		} else {
			PWRAPLOG
			    ("_pwrap_init_sistrobe [Read Test of MT6350] tuning,index=%d rdata=%x\n",
			     ind, rdata);
		}
#endif
	}
#ifndef SLV_6350
	result[0] = result[1];
#endif
#ifndef SLV_6332
	result[1] = result[0];
#endif
	/* -------------------------------------------------------- */
	/* Locate the leading one and trailing one of PMIC 1/2     */
	/* -------------------------------------------------------- */
	for (ind = 23; ind >= 0; ind--) {
		if ((result[0] & (0x1 << ind)) && leading_one[0] == -1)
			leading_one[0] = ind;
		if (leading_one[0] > 0)
			break;
	}
	for (ind = 23; ind >= 0; ind--) {
		if ((result[1] & (0x1 << ind)) && leading_one[1] == -1)
			leading_one[1] = ind;
		if (leading_one[1] > 0)
			break;
	}

	for (ind = 0; ind < 24; ind++) {
		if ((result[0] & (0x1 << ind)) && tailing_one[0] == -1)
			tailing_one[0] = ind;
		if (tailing_one[0] > 0)
			break;
	}
	for (ind = 0; ind < 24; ind++) {
		if ((result[1] & (0x1 << ind)) && tailing_one[1] == -1)
			tailing_one[1] = ind;
		if (tailing_one[1] > 0)
			break;
	}

	/* -------------------------------------------------------- */
	/* Check the continuity of pass range                       */
	/* -------------------------------------------------------- */
	for (i = 0; i < 2; i++) {
		tmp1 = (0x1 << (leading_one[i] + 1)) - 1;
		tmp2 = (0x1 << tailing_one[i]) - 1;
		if ((tmp1 - tmp2) != result[i]) {
			/*TERR = "[DrvPWRAP_InitSiStrobe] Fail at PMIC %d, result = %x, leading_one:%d, tailing_one:%d"
			   , i+1, result[i], leading_one[i], tailing_one[i] */
			PWRAPERR
			    ("_pwrap_init_sistrobe Fail at PMIC %d, result = %x, leading_one:%d, tailing_one:%d\n",
			     i + 1, result[i], leading_one[i], tailing_one[i]);
			result_faulty = 0x1;
		}
	}

	/* -------------------------------------------------------- */
	/* Config SICK and SIDLY to the middle point of pass range  */
	/* -------------------------------------------------------- */
	if (result_faulty == 0) {
		/* choose the best point in the interaction of PMIC1's pass range and PMIC2's pass range */
		ind =
		    ((leading_one[0] + tailing_one[0]) / 2 +
		     (leading_one[1] + tailing_one[1]) / 2) / 2;
		/*TINFO = "The best point in the interaction area is %d, ind" */
		WRAP_WR32(PMIC_WRAP_SI_CK_CON, (ind >> 2) & 0x7);
		WRAP_WR32(PMIC_WRAP_SIDLY, 0x3 - (ind & 0x3));
		/* -------------------------------------------------------- */
		/* Restore */
		/* -------------------------------------------------------- */
		WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);
		return 0;
	}
	PWRAPERR("_pwrap_init_sistrobe Fail,result_faulty=%x\n", result_faulty);
	return result_faulty;
}

/* -------------------------------------------------------- */
/* Function : _pwrap_reset_spislv() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */

static s32 _pwrap_reset_spislv(void)
{
	u32 ret = 0;
	u32 return_value = 0;
	/* PWRAPFUC(); */
	/* This driver does not using _pwrap_switch_mux */
	/* because the remaining requests are expected to fail anyway */

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, DISABLE_ALL);
	WRAP_WR32(PMIC_WRAP_WRAP_EN, DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL, MANUAL_MODE);
	WRAP_WR32(PMIC_WRAP_MAN_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_DIO_EN, DISABLE);

	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_CSL << 8));	/* 0x2100 */
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_OUTS << 8));	/* 0x2800//to reset counter */
	WRAP_WR32(PMIC_WRAP_MAN_CMD, (OP_WR << 13) | (OP_CSH << 8));	/* 0x2000 */
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
	u32 wdata = 0;
	u32 rdata = 0;
	u32 return_value = 0;

	PWRAPFUC();

	if (regck_sel == 1)	/* not supported in 6323!! */
		return_value = E_PWR_INIT_REG_CLOCK;
	else
		wdata = 0x3;

	pwrap_write_nochk(MT6350_TOP_CKCON1_CLR, wdata);
	pwrap_read_nochk(MT6350_TOP_CKCON1, &rdata);
#if defined SLV_6350
	if ((rdata & 0x3) != 0) {
		PWRAPERR("_pwrap_init_reg_clock,MT6350_TOP_CKCON1 Write [15]=1 Fail, rdata=%x\n",
			 rdata);
		return E_PWR_WRITE_TEST_FAIL;
	}
	pwrap_write_nochk(MT6350_DEW_RDDMY_NO, 0x8);
	WRAP_WR32(PMIC_WRAP_RDDMY, 0x8);
#endif

	/* Config SPI Waveform according to reg clk */
	if (regck_sel == 1) {	/* 6MHz in 6323  => no support ; 18MHz in 6320 */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0x4);
		/* wait data written into register => 3T_PMIC: consists of CSLEXT_END(1T) + CSHEXT(6T) */
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0x5);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START, 0x0);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END, 0x0);
	} else if (regck_sel == 2) {	/* 12MHz in 6323; 36MHz in 6320 */
#if defined SLV_6350
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0x0);
		/* wait data written into register => 3T_PMIC: consists of CSLEXT_END(1T) + CSHEXT(6T) */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0x7);
#endif
		WRAP_WR32(PMIC_WRAP_CSLEXT_START, 0x0);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END, 0x0);
	} else {		/* Safe mode */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0xf);
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0xf);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START, 0xf);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END, 0xf);
	}

	return return_value;
}

/*
 * pmic_wrap init,init wrap interface
 */
s32 pwrap_init(void)
{
	s32 sub_return = 0;
	s32 sub_return1 = 0;
	u32 clk_sel = 0;
	/* s32 ret=0; */
	u32 rdata = 0x0;
	/* u32 timeout=0; */
	u32 cg_mask = 0;
	u32 backup = 0;

	PWRAPFUC();
#ifdef CONFIG_OF
	sub_return = pwrap_of_iomap();
	if (sub_return)
		return sub_return;
#endif
	/* ############################### */
	/* toggle PMIC_WRAP and pwrap_spictl reset */
	/* ############################### */
	/* WRAP_SET_BIT(0x80,INFRA_GLOBALCON_RST0); */
	/* WRAP_CLR_BIT(0x80,INFRA_GLOBALCON_RST0); */
	/* __pwrap_soft_reset(); */

	/* Turn off module clock */
	cg_mask = ((1 << 20) | (1 << 27) | (1 << 28) | (1 << 29));
	backup = (~WRAP_RD32(CLK_SWCG_1)) & cg_mask;
	WRAP_WR32(CLK_SETCG_1, cg_mask);
	/* dummy read to add latency (to wait clock turning off) */
	rdata = WRAP_RD32(PMIC_WRAP_SWRST);

	/* Toggle module reset */
	WRAP_WR32(PMIC_WRAP_SWRST, ENABLE);

	rdata = WRAP_RD32(WDT_FAST_SWSYSRST);
	WRAP_WR32(WDT_FAST_SWSYSRST, (rdata | (0x1 << 11)) | (0x88 << 24));
	WRAP_WR32(WDT_FAST_SWSYSRST, (rdata & (~(0x1 << 11))) | (0x88 << 24));
	WRAP_WR32(PMIC_WRAP_SWRST, DISABLE);

	/* Turn on module clock */
	WRAP_WR32(CLK_CLRCG_1, backup | (1 << 20));	/* ensure cg for AP is off; */

	/* Turn on module clock dcm (in global_con) */
	/* WHQA_00014186: set PMIC bclk DCM default off due to HW issue */
	WRAP_WR32(CLK_SETCG_3, (1 << 2) | (1 << 1));
	/* WRAP_WR32(CLK_SETCG_3, (1 << 2)); */

	/* ############################### */
	/* Set SPI_CK_freq = 26MHz */
	/* ############################### */
#ifndef CONFIG_MTK_FPGA
	clk_sel = WRAP_RD32(CLK_SEL_0);
	WRAP_WR32(CLK_SEL_0, clk_sel | (0x3 << 24));
#endif

	/* ############################### */
	/* Enable DCM */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_DCM_EN, 1);	/* enable CRC DCM and PMIC_WRAP DCM */
	WRAP_WR32(PMIC_WRAP_DCM_DBC_PRD, DISABLE);	/* no debounce */

	/* ############################### */
	/* Reset SPISLV */
	/* ############################### */
	sub_return = _pwrap_reset_spislv();
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_reset_spislv fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_RESET_SPI;
	}
	/* ############################### */
	/* Enable WACS2 */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_WRAP_EN, ENABLE);	/* enable wrap */
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);	/* Only WACS2 */
	WRAP_WR32(PMIC_WRAP_WACS2_EN, ENABLE);

	/* ############################### */
	/* Set Dummy cycle to make the cycle is the same at both AP and PMIC sides */
	/* ############################### */
	/* (default value of 6320 dummy cycle is already 0x8) */
#ifdef SLV_6350
	WRAP_WR32(PMIC_WRAP_RDDMY, 0xF);
#endif

	/* ############################### */
	/* Input data calibration flow; */
	/* ############################### */
	sub_return = _pwrap_init_sistrobe();
	if (sub_return != 0) {
		PWRAPERR("error,DrvPWRAP_InitSiStrobe fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_SIDLY;
	}
	/* ############################### */
	/* SPI Waveform Configuration */
	/* ############################### */
	/* 0:safe mode, 1:18MHz */
	sub_return = _pwrap_init_reg_clock(2);
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_init_reg_clock fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_REG_CLOCK;
	}
	/* ############################### */
	/* Enable DIO mode */
	/* ############################### */
	/* PMIC2 dual io not ready */

	sub_return = _pwrap_init_dio(1);
	if (sub_return != 0) {
		PWRAPERR("_pwrap_init_dio test error,error code=%x, sub_return=%x\n", 0x11,
			 sub_return);
		return E_PWR_INIT_DIO;
	}
	/* ############################### */
	/* Enable Encryption */
	/* ############################### */

	sub_return = _pwrap_init_cipher();
	if (sub_return != 0) {
		PWRAPERR("Enable Encryption fail, return=%x\n", sub_return);
		return E_PWR_INIT_CIPHER;
	}
	/* ############################### */
	/* Write test using WACS2 */
	/* ############################### */
	/* check Wtiet test default value */

#ifdef SLV_6350
	sub_return = pwrap_write_nochk(MT6350_DEW_WRITE_TEST, MT6350_WRITE_TEST_VALUE);
	sub_return1 = pwrap_read_nochk(MT6350_DEW_WRITE_TEST, &rdata);
	if (rdata != MT6350_WRITE_TEST_VALUE) {
		PWRAPERR
		    ("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n",
		     rdata, sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
#endif

	/* ############################### */
	/* Signature Checking - Using CRC */
	/* should be the last to modify WRITE_TEST */
	/* ############################### */
	sub_return = pwrap_write_nochk(MT6350_DEW_CRC_EN, ENABLE);
	if (sub_return != 0) {
		PWRAPERR("Enable CRC fail, return=%x\n", sub_return);
		return E_PWR_INIT_ENABLE_CRC;
	}
	WRAP_WR32(PMIC_WRAP_CRC_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_SIG_MODE, CHECK_CRC);
	WRAP_WR32(PMIC_WRAP_SIG_ADR, MT6350_DEW_CRC_VAL);

	/* ############################### */
	/* PMIC_WRAP enables */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0x1ff);
	WRAP_WR32(PMIC_WRAP_WACS0_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_WACS1_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_STAUPD_PRD, 0x5);	/* 0x1:20us,for concurrence test,MP:0x5;  //100us */
	WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN, 0xff);
	WRAP_WR32(PMIC_WRAP_WDT_UNIT, 0xf);
	WRAP_WR32(PMIC_WRAP_WDT_SRC_EN, 0xffffffff);
	WRAP_WR32(PMIC_WRAP_TIMER_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_INT_EN, 0x7ffffffd);

	/* ############################### */
	/* GPS Initialization */
	/* set address of ready bit register and thermal data register */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_ADC_CMD_ADDR, MT6350_AUXADC_CON21);
	WRAP_WR32(PMIC_WRAP_PWRAP_ADC_CMD, 0x8000);
	WRAP_WR32(PMIC_WRAP_ADC_RDY_ADDR, MT6350_AUXADC_ADC12);
	WRAP_WR32(PMIC_WRAP_ADC_RDATA_ADDR1, MT6350_AUXADC_ADC13);
	WRAP_WR32(PMIC_WRAP_ADC_RDATA_ADDR2, MT6350_AUXADC_ADC14);

	/* ############################### */
	/* Initialization Done */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_INIT_DONE2, ENABLE);

	/* ############################### */
	/* TBD: Should be configured by MD MCU */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_INIT_DONE0, ENABLE);
	WRAP_WR32(PMIC_WRAP_INIT_DONE1, ENABLE);

#ifdef CONFIG_OF
	pwrap_of_iounmap();
#endif

	return 0;
}

/* EXPORT_SYMBOL(pwrap_init); */
/*Interrupt handler function*/
static irqreturn_t mt_pmic_wrap_irq(int irqno, void *dev_id)
{
	unsigned long flags = 0;

	PWRAPFUC();
	PWRAPREG("dump pwrap register\n");
	spin_lock_irqsave(&wrp_lock_isr, flags);
	/* *----------------------------------------------------------------------- */
	pwrap_dump_all_register();
	/* raise the priority of WACS2 for AP */
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 1 << 3);

	/* *----------------------------------------------------------------------- */
	/* clear interrupt flag */
	WRAP_WR32(PMIC_WRAP_INT_CLR, 0xffffffff);
	PWRAPREG("INT flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT_EN));
	BUG_ON(1);
	spin_unlock_irqrestore(&wrp_lock_isr, flags);
	return IRQ_HANDLED;
}

static u32 pwrap_read_test(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	/* Read Test */
#ifdef SLV_6350
	return_value = pwrap_read(MT6350_DEW_READ_TEST, &rdata);
	if (rdata != MT6350_DEFAULT_VALUE_READ_TEST) {
		PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata,
			 return_value);
		return_value =  E_PWR_READ_TEST_FAIL;
	} else {
		PWRAPREG("Read Test pass,return_value=%d\n", return_value);
		/* return 0; */
	}
#endif
	return return_value;
}

static u32 pwrap_write_test(void)
{
	u32 rdata = 0;
	u32 sub_return = 0;
	u32 sub_return1 = 0;
	/* ############################### */
	/* Write test using WACS2 */
	/* ############################### */
#ifdef SLV_6350
	sub_return = pwrap_write(MT6350_DEW_WRITE_TEST, MT6350_WRITE_TEST_VALUE);
	PWRAPREG("after MT6350 pwrap_write\n");
	sub_return1 = pwrap_read(MT6350_DEW_WRITE_TEST, &rdata);
	if ((rdata != MT6350_WRITE_TEST_VALUE) || (sub_return != 0) || (sub_return1 != 0)) {
		PWRAPREG
		    ("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n",
		     rdata, sub_return, sub_return1);
		sub_return =  E_PWR_INIT_WRITE_TEST;
	} else {
		PWRAPREG("write MT6350 Test pass\n");
		/* return 0; */
	}
#endif
	return sub_return;
}

#ifndef USER_BUILD_KERNEL
static void pwrap_read_reg_on_pmic(u32 reg_addr)
{
	u32 reg_value = 0;
	u32 return_value = 0;
	/* PWRAPFUC(); */
	return_value = pwrap_read(reg_addr, &reg_value);
	PWRAPREG("0x%x=0x%x,return_value=%x\n", reg_addr, reg_value, return_value);
}

static void pwrap_write_reg_on_pmic(u32 reg_addr, u32 reg_value)
{
	u32 return_value = 0;

	PWRAPREG("write 0x%x to register 0x%x\n", reg_value, reg_addr);
	return_value = pwrap_write(reg_addr, reg_value);
	return_value = pwrap_read(reg_addr, &reg_value);
	/* PWRAPFUC(); */
	PWRAPREG("the result:0x%x=0x%x,return_value=%x\n", reg_addr, reg_value, return_value);
}
#endif

static void pwrap_ut(u32 ut_test)
{
	switch (ut_test) {
	case 1:
		/* pwrap_wacs2_para_test(); */
		pwrap_write_test();
		break;
	case 2:
		/* pwrap_wacs2_para_test(); */
		pwrap_read_test();
		break;

	default:
		PWRAPREG("default test.\n");
		break;
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
#ifndef USER_BUILD_KERNEL
	u32 reg_value = 0;
	u32 reg_addr = 0;
#endif
	u32 return_value = 0;
	u32 ut_test = 0;

	if (!strncmp(buf, "-h", 2)) {
		PWRAPREG
		    ("PWRAP debug: [-dump_reg][-trace_wacs2][-init][-rdpmic][-wrpmic][-readtest][-writetest]\n");
		PWRAPREG("PWRAP UT: [1][2]\n");
	}
	/* ----------pwrap debug---------- */
	else if (!strncmp(buf, "-dump_reg", 9)) {
		pwrap_dump_all_register();
	} else if (!strncmp(buf, "-trace_wacs2", 12)) {
		/* pwrap_trace_wacs2(); */
	} else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	}
#ifndef USER_BUILD_KERNEL
	else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf + 7, "%x", &reg_addr))) {
		pwrap_read_reg_on_pmic(reg_addr);
	} else if (!strncmp(buf, "-wrpmic", 7)
		   && (2 == sscanf(buf + 7, "%x %x", &reg_addr, &reg_value))) {
		pwrap_write_reg_on_pmic(reg_addr, reg_value);
	}
#endif
	else if (!strncmp(buf, "-readtest", 9)) {
		pwrap_read_test();
	} else if (!strncmp(buf, "-writetest", 10)) {
		pwrap_write_test();
	} else if (!strncmp(buf, "-int", 4)) {
		/* pwrap_int_test(); */
	}
	/* ----------pwrap UT---------- */
	else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf + 3, "%d", &ut_test))) {
		pwrap_ut(ut_test);
	} else {
		PWRAPREG("wrong parameter\n");
	}
	return count;
}

/*---------------------------------------------------------------------------*/
#endif				/* endif PMIC_WRAP_NO_PMIC */

#ifdef CONFIG_OF
static int pwrap_of_iomap(void)
{
	/*
	 * Map the address of the following register base:
	 * INFRACFG_AO, TOPCKGEN, SCP_CLK_CTRL, SCP_PMICWP2P
	 */

	struct device_node *toprgu_node;
	struct device_node *topckgen_node;

	toprgu_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPRGU");
	if (!toprgu_node) {
		pr_info("get TOPRGU failed\n");
		return -ENODEV;
	}

	toprgu_reg_base = of_iomap(toprgu_node, 0);
	if (!toprgu_reg_base) {
		pr_info("TOPRGU iomap failed\n");
		return -ENOMEM;
	}

	topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPCKGEN");
	if (!topckgen_node) {
		pr_info("get TOPCKGEN failed\n");
		return -ENODEV;
	}

	topckgen_base = of_iomap(topckgen_node, 0);
	if (!topckgen_base) {
		pr_info("TOPCKGEN iomap failed\n");
		return -ENOMEM;
	}

	pr_info("TOPRGU reg: 0x%p\n", toprgu_reg_base);
	pr_info("TOPCKGEN reg: 0x%p\n", topckgen_base);
	return 0;
}

static void pwrap_of_iounmap(void)
{
	iounmap(toprgu_reg_base);
	iounmap(topckgen_base);
}
#endif

#define VERSION     "Revision"
static int is_pwrap_init_done(void)
{
	int ret = 0;

	ret = WRAP_RD32(PMIC_WRAP_INIT_DONE2);
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

static int __init pwrap_hal_init(void)
{
	s32 ret = 0;
#ifdef CONFIG_OF
	u32 pwrap_irq;
	struct device_node *pwrap_node;
	struct device_node *toprgu_node;
	struct device_node *topckgen_node;

	toprgu_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPRGU");
	if (!toprgu_node) {
		pr_info("get TOPRGU failed\n");
		return -ENODEV;
	}

	toprgu_reg_base = of_iomap(toprgu_node, 0);
	if (!toprgu_reg_base) {
		pr_info("TOPRGU iomap failed\n");
		return -ENOMEM;
	}

	topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPCKGEN");
	if (!topckgen_node) {
		pr_info("get TOPCKGEN failed\n");
		return -ENODEV;
	}

	topckgen_base = of_iomap(topckgen_node, 0);
	if (!topckgen_base) {
		pr_info("TOPCKGEN iomap failed\n");
		return -ENOMEM;
	}

	pwrap_node = of_find_compatible_node(NULL, NULL, "mediatek,PWRAP");
	if (!pwrap_node) {
		pr_info("PWRAP get node failed\n");
		return -ENODEV;
	}

	pwrap_base = of_iomap(pwrap_node, 0);
	if (!pwrap_base) {
		pr_info("PWRAP iomap failed\n");
		return -ENOMEM;
	}

	pwrap_irq = irq_of_parse_and_map(pwrap_node, 0);
	if (!pwrap_irq) {
		pr_info("PWRAP get irq fail\n");
		return -ENODEV;
	}
	pr_info("PWRAP reg: 0x%p,  irq: %d\n", pwrap_base, pwrap_irq);
#endif

	PWRAPLOG("HAL init: version %s\n", VERSION);
	mt_wrp = get_mt_pmic_wrap_drv();
	mt_wrp->store_hal = mt_pwrap_store_hal;
	mt_wrp->show_hal = mt_pwrap_show_hal;
	mt_wrp->wacs2_hal = pwrap_wacs2_hal;

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
	/* PWRAPERR("not init (%x)\n", is_pwrap_init_done); */

	return ret;
}
postcore_initcall(pwrap_hal_init);

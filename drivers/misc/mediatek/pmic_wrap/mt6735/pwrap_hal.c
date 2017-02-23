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
static void __iomem *topckgen_base;
static void __iomem *infracfg_ao_base;
#endif
static struct mt_pmic_wrap_driver *mt_wrp;

static spinlock_t	wrp_lock = __SPIN_LOCK_UNLOCKED(lock);
/* spinlock_t	wrp_lock = __SPIN_LOCK_UNLOCKED(lock); */
/* ----------interral API ------------------------ */
static s32	_pwrap_init_dio(u32 dio_en);
static s32	_pwrap_init_cipher(void);
static s32	_pwrap_init_reg_clock(u32 regck_sel);

static s32	_pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata);


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
	PWRAPLOG("[PMIC2]first of all PMIC_WRAP_STAUPD_GRPEN=0x%x\n", WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN));
	PWRAPLOG("[PMIC2]first of all PMIC_WRAP_EINT_STA=0x%x\n", WRAP_RD32(PMIC_WRAP_EINT_STA));
	return WRAP_RD32(PMIC_WRAP_EINT_STA);
}

void mt_pmic_wrap_eint_clr(int offset)
{
	if ((offset < 0) || (offset > 3))
		PWRAPERR("clear EINT flag error, only 0-3 bit\n");
	else
		WRAP_WR32(PMIC_WRAP_EINT_CLR, (1<<offset));
}

/* -------------------------------------------------------- */
/* Function : pwrap_wacs2_hal() */
/* Description : */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
static s32 pwrap_wacs2_hal(u32  write, u32  adr, u32  wdata, u32 *rdata)
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
	/* *----------------------------------------------------------------------- */
	pwrap_dump_all_register();
	/* raise the priority of WACS2 for AP */
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 1<<3);

	/* *----------------------------------------------------------------------- */
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
		PWRAPREG("PWRAP debug: [-dump_reg]"
			\"[-trace_wacs2][-init]"
			\"[-rdap][-wrap][-rdpmic][-wrpmic]"
			\"[-readtest][-writetest]\n");
		PWRAPREG("PWRAP UT: [1][2]\n");
	}
	/* -------pwrap debug----------- */
	else if (!strncmp(buf, "-dump_reg", 9))
		pwrap_dump_all_register();
	else if (!strncmp(buf, "-trace_wacs2", 12))
		;/* pwrap_trace_wacs2(); */
	else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	} else if (!strncmp(buf, "-rdap", 5) && (1 == sscanf(buf+5, "%x", &reg_addr))) {
		/* pwrap_read_reg_on_ap(reg_addr); */
	} else if (!strncmp(buf, "-wrap", 5) && (2 == sscanf(buf+5, "%x %x", &reg_addr, &reg_value))) {
		/* pwrap_write_reg_on_ap(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf+7, "%x", &reg_addr))) {
		/* pwrap_read_reg_on_pmic(reg_addr); */
	} else if (!strncmp(buf, "-wrpmic", 7) && (2 == sscanf(buf+7, "%x %x", &reg_addr, &reg_value))) {
		/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-readtest", 9)) {
		pwrap_read_test();
	} else if (!strncmp(buf, "-writetest", 10)) {
		pwrap_write_test();
	}
	/* ----------------pwrap UT---------- */
	else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf+3, "%d", &ut_test))) {
		pwrap_ut(ut_test);
	} else{
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
	#if defined(CONFIG_ARCH_MT6735M)
	for (i = 0; i <= 0x234; i += 16) {
		PWRAPREG("offset 0x%.3x:0x%.8x 0x%.8x 0x%.8x 0x%.8x\n", i,
				WRAP_RD32(PMIC_WRAP_BASE+i+0),
				WRAP_RD32(PMIC_WRAP_BASE+i+4),
				WRAP_RD32(PMIC_WRAP_BASE+i+8),
				WRAP_RD32(PMIC_WRAP_BASE+i+12));
	}
	i = 0x234;
		PWRAPREG("offset 0x%.3x:0x%.8x 0x%.8x 0x%.8x 0x%.8x\n", i ,
				WRAP_RD32(PMIC_WRAP_BASE+i+0),
				WRAP_RD32(PMIC_WRAP_BASE+i+4),
				WRAP_RD32(PMIC_WRAP_BASE+i+8),
				WRAP_RD32(PMIC_WRAP_BASE+i+12));

	#else
	for (i = 0; i <= 0x248; i += 16) {
		PWRAPREG("offset 0x%.3x:0x%.8x 0x%.8x 0x%.8x 0x%.8x\n", i,
				WRAP_RD32(PMIC_WRAP_BASE+i+0),
				WRAP_RD32(PMIC_WRAP_BASE+i+4),
				WRAP_RD32(PMIC_WRAP_BASE+i+8),
				WRAP_RD32(PMIC_WRAP_BASE+i+12));
	}
	PWRAPREG("infra clock1 0x10000048 =0x%x\n", WRAP_RD32(infracfg_ao_base+0x48));
	PWRAPREG("infra clock2 0x10210080 =0x%x\n", WRAP_RD32(topckgen_base+0x80));
	#endif
}
static inline void pwrap_dump_pmic_register(void)
{
	/* u32 i=0; */
	/* u32 reg_addr=0; */
	/* u32 reg_value=0; */
	/*  */
	/* PWRAPREG("dump dewrap register\n"); */
	/* for(i=0;i<=14;i++) */
	/* { */
	/* reg_addr=(DEW_BASE+i*4); */
	/* reg_value=pwrap_read_nochk(reg_addr,&reg_value); */
	/* PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value); */
	/* } */
}
static inline void pwrap_dump_all_register(void)
{
	pwrap_dump_ap_register();
	pwrap_dump_pmic_register();
}
static void __pwrap_soft_reset(void)
{
	PWRAPLOG("start reset wrapper\n");
	/* PWRAP_SOFT_RESET; */
	WRAP_WR32(INFRA_GLOBALCON_RST0, 0x80);

	PWRAPLOG("the reset register =%x\n", WRAP_RD32(INFRA_GLOBALCON_RST0));
	PWRAPLOG("PMIC_WRAP_STAUPD_GRPEN =0x%x,it should be equal to 0xc\n", WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN));
	/* clear reset bit */
	/* PWRAP_CLEAR_SOFT_RESET_BIT; */
	WRAP_WR32(INFRA_GLOBALCON_RST1, 0x80);
}
/******************************************************************************
  wrapper timeout
 ******************************************************************************/
#define PWRAP_TIMEOUT
#ifdef PWRAP_TIMEOUT
static u64 _pwrap_get_current_time(void)
{
	return sched_clock();   /* /TODO: fix me */
}
/* u64 elapse_time=0; */

static bool _pwrap_timeout_ns(u64 start_time_ns, u64 timeout_time_ns)
{
	u64 cur_time = 0;
	u64 elapse_time = 0;

	/* get current tick */
	cur_time = _pwrap_get_current_time();/* ns */

	/* avoid timer over flow exiting in FPGA env */
	if (cur_time < start_time_ns) {
		PWRAPERR("@@@@Timer overflow! start%lld cur timer%lld\n", start_time_ns, cur_time);
		start_time_ns = cur_time;
		timeout_time_ns = 2000*1000;
		PWRAPERR("@@@@reset timer! start%lld setting%lld\n", start_time_ns, timeout_time_ns);
	}

	elapse_time = cur_time-start_time_ns;

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
	return time_us*1000;
}

#else
static u64 _pwrap_get_current_time(void)
{
	return 0;
}
static bool _pwrap_timeout_ns(u64 start_time_ns, u64 elapse_time)/* ,u64 timeout_ns) */
{
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
typedef u32 (*loop_condition_fp)(u32);/* define a function pointer */

static inline u32 wait_for_fsm_idle(u32 x)
{
	return GET_WACS0_FSM(x) != WACS_FSM_IDLE;
}
static inline u32 wait_for_fsm_vldclr(u32 x)
{
	return GET_WACS0_FSM(x) != WACS_FSM_WFVLDCLR;
}
static inline u32 wait_for_sync(u32 x)
{
	return GET_SYNC_IDLE0(x) != WACS_SYNC_IDLE;
}
static inline u32 wait_for_idle_and_sync(u32 x)
{
	return (GET_WACS2_FSM(x) != WACS_FSM_IDLE) || (GET_SYNC_IDLE2(x) != WACS_SYNC_IDLE);
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
	return  GET_MAN_FSM(x) != MAN_FSM_WFVLDCLR;
}
static inline u32 wait_for_cipher_ready(u32 x)
{
	return x != 3;
}
static inline u32 wait_for_stdupd_idle(u32 x)
{
	return GET_STAUPD_FSM(x) != 0x0;
}

static inline u32 wait_for_state_ready_init(loop_condition_fp fp, u32 timeout_us, void *wacs_register, u32 *read_reg)
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
	} while (fp(reg_rdata)); /* IDLE State */

	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline u32 wait_for_state_idle_init(loop_condition_fp fp, u32 timeout_us,
	void *wacs_register, void *wacs_vldclr_register, u32 *read_reg)
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
				WRAP_WR32(wacs_vldclr_register , 1);
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
	} while (fp(reg_rdata)); /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}
static inline u32 wait_for_state_idle(loop_condition_fp fp, u32 timeout_us,
	void *wacs_register, void *wacs_vldclr_register, u32 *read_reg)
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
				WRAP_WR32(wacs_vldclr_register , 1);
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
	} while (fp(reg_rdata)); /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
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
	if (offset < 0 || offset > 3)
		PWRAPERR("clear EINT flag error, only 0-3 bit\n");
	else
		WRAP_WR32(PMIC_WRAP_EINT_CLR, (1<<offset));
	PWRAPREG("clear EINT flag mt_pmic_wrap_eint_status=0x%x\n", WRAP_RD32(PMIC_WRAP_EINT_STA));
}

static inline u32 wait_for_state_ready(loop_condition_fp fp, u32 timeout_us, void *wacs_register, u32 *read_reg)
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
	} while (fp(reg_rdata)); /* IDLE State */
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
static s32 pwrap_wacs2_hal(u32  write, u32  adr, u32  wdata, u32 *rdata)
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
	if ((write & ~(0x1))    != 0)
		return E_PWR_INVALID_RW;
	if ((adr   & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;
	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	spin_lock_irqsave(&wrp_lock, flags);
	/* check pmicaddr 0xa bit11 & bit10 ,bit 11 only can write1 bit 10 only can write 0 request by Wy Chuang */
	if (0 != write && 0xa == adr) {

		if (0 == (wdata & (1<<11)) || 1 == ((wdata>>10) & 0x1)) {
			PWRAPERR(" pwrap_wacs2_hal check 0xa err pid=%d, wdata=0x%x\n", current->pid, wdata);
			BUG_ON(1);
		}
	}

	/* Check IDLE & INIT_DONE in advance */
	return_value = wait_for_state_idle(wait_for_fsm_idle, TIMEOUT_WAIT_IDLE,
	PMIC_WRAP_WACS2_RDATA, PMIC_WRAP_WACS2_VLDCLR, 0);
	if (return_value != 0) {
		PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n", return_value);
		goto FAIL;
	}
	wacs_write  = write << 31;
	wacs_adr    = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;

	WRAP_WR32(PMIC_WRAP_WACS2_CMD, wacs_cmd);
	if (write == 0) {
		if (NULL == rdata) {
			PWRAPERR("rdata is a NULL pointer\n");
			return_value = E_PWR_INVALID_ARG;
			goto FAIL;
		}
		return_value = wait_for_state_ready(wait_for_fsm_vldclr,
			TIMEOUT_READ, PMIC_WRAP_WACS2_RDATA, &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n", return_value);
			return_value += 1;
			goto FAIL;
		}
		*rdata = GET_WACS0_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR , 1);
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
s32 pwrap_read_nochk(u32  adr, u32 *rdata)
{
	return _pwrap_wacs2_nochk(0, adr,  0, rdata);
}
/*EXPORT_SYMBOL(pwrap_read_nochk);*/

s32 pwrap_write_nochk(u32  adr, u32  wdata)
{
	return _pwrap_wacs2_nochk(1, adr, wdata, 0);
}
/*EXPORT_SYMBOL(pwrap_write_nochk);*/

static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	u32 reg_rdata = 0x0;
	u32 wacs_write = 0x0;
	u32 wacs_adr = 0x0;
	u32 wacs_cmd = 0x0;
	u32 return_value = 0x0;
	/* PWRAPFUC(); */
	/* Check argument validation */

	if (0 != write && 0xa == adr) {

		if (0 == (wdata & (1<<11)) || 1 == ((wdata>>10) & 0x01)) {
			PWRAPERR("_pwrap_wacs2_nochk check 0xa err pid=%d, wdata=0x%x\n", current->pid, wdata);
			BUG_ON(1);
		}
	}

	if ((write & ~(0x1))    != 0)
		return E_PWR_INVALID_RW;
	if ((adr   & ~(0xffff)) != 0)
		return E_PWR_INVALID_ADDR;
	if ((wdata & ~(0xffff)) != 0)
		return E_PWR_INVALID_WDAT;

	/* Check IDLE */
	return_value = wait_for_state_ready_init(wait_for_fsm_idle,
	TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_wacs2_nochk write command fail,return_value=%x\n", return_value);
		return return_value;
	}

	wacs_write  = write << 31;
	wacs_adr    = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;
	WRAP_WR32(PMIC_WRAP_WACS2_CMD, wacs_cmd);

	if (write == 0) {
		if (NULL == rdata)
			return E_PWR_INVALID_ARG;
		/* wait for read data ready */
		return_value = wait_for_state_ready_init(wait_for_fsm_vldclr,
		TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, &reg_rdata);
		if (return_value != 0) {
			PWRAPERR("_pwrap_wacs2_nochk read fail,return_value=%x\n", return_value);
			return return_value;
		}
		*rdata = GET_WACS0_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR , 1);
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
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , WACS2); /* only WACS2 */
#ifdef SLV_6328
	  pwrap_write_nochk(MT6328_DEW_DIO_EN, (dio_en));
#endif
#ifdef SLV_6332
	  pwrap_write_nochk(MT6332_DEW_DIO_EN, (dio_en>>1));
#endif

	/* Check IDLE & INIT_DONE in advance */
	return_value = wait_for_state_ready_init(wait_for_idle_and_sync,
	TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_init_dio fail,return_value=%x\n", return_value);
		return return_value;
	}
	/* enable AP DIO mode */
	WRAP_WR32(PMIC_WRAP_DIO_EN , dio_en);
	/* Read Test */
#ifdef SLV_6328
	pwrap_read_nochk(MT6328_DEW_READ_TEST, &rdata);
	if (rdata != MT6328_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("[Dio_mode][Read Test] fail,dio_en = %x", dio_en);
		PWRAPERR("READ_TEST rdata=%x, exp=0x5aa5\n", rdata);
		return E_PWR_READ_TEST_FAIL;
	  }
#endif
#ifdef SLV_6332
	pwrap_read_nochk(MT6332_DEW_READ_TEST, &rdata);
	if (rdata != MT6332_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("[Dio_mode][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=0xa55a\n", dio_en, rdata);
		return E_PWR_READ_TEST_FAIL;
	}
#endif
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
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

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , WACS2);

	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST ,  1);
	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST ,  0);
	WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL , 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL  , 2);
	WRAP_WR32(PMIC_WRAP_CIPHER_EN   , 1);

	/* Config CIPHER @ PMIC */
#ifdef SLV_6328
	pwrap_write_nochk(MT6328_DEW_CIPHER_SWRST, 0x1);
	pwrap_write_nochk(MT6328_DEW_CIPHER_SWRST, 0x0);
	pwrap_write_nochk(MT6328_DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(MT6328_DEW_CIPHER_IV_SEL,  0x2);
	pwrap_write_nochk(MT6328_DEW_CIPHER_EN,  0x1);
#endif
#ifdef SLV_6332
	pwrap_write_nochk(MT6332_DEW_CIPHER_SWRST,   0x1);
	pwrap_write_nochk(MT6332_DEW_CIPHER_SWRST,   0x0);
	pwrap_write_nochk(MT6332_DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(MT6332_DEW_CIPHER_IV_SEL,  0x2);
	pwrap_write_nochk(MT6332_DEW_CIPHER_EN,	0x1);
#endif

	PWRAPLOG("mt_pwrap_init---- debug7\n");

	/* wait for cipher data ready@AP */
	return_value = wait_for_state_ready_init(wait_for_cipher_ready, TIMEOUT_WAIT_IDLE, PMIC_WRAP_CIPHER_RDY, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher data ready@AP fail,return_value=%x\n", return_value);
		return return_value;
	}
	PWRAPLOG("mt_pwrap_init---- debug8\n");
	/* wait for cipher data ready@PMIC */
#ifdef SLV_6328
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait for cipher data ready@PMIC\n");
			/* pwrap_dump_all_register(); */
			/* return E_PWR_WAIT_IDLE_TIMEOUT; */
		}
		pwrap_read_nochk(MT6328_DEW_CIPHER_RDY, &rdata);
	} while (rdata != 0x1); /* cipher_ready */

	pwrap_write_nochk(MT6328_DEW_CIPHER_MODE, 0x1);
	PWRAPLOG("mt_pwrap_init---- debug9\n");
#endif
#ifdef SLV_6332
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns)) {
			PWRAPERR("wait for cipher data ready@PMIC\n");
			/* pwrap_dump_all_register(); */
			/* return E_PWR_WAIT_IDLE_TIMEOUT; */
		}
		pwrap_read_nochk(MT6332_DEW_CIPHER_RDY, &rdata);
	} while (rdata != 0x1); /* cipher_ready */

	pwrap_write_nochk(MT6332_DEW_CIPHER_MODE, 0x1);
#endif
	/* wait for cipher mode idle */
	return_value = wait_for_state_ready_init(wait_for_idle_and_sync, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher mode idle fail,return_value=%x\n", return_value);
		return return_value;
	}
	WRAP_WR32(PMIC_WRAP_CIPHER_MODE , 1);

	/* Read Test */
#ifdef SLV_6328
	  pwrap_read_nochk(MT6328_DEW_READ_TEST, &rdata);
	  if (rdata != MT6328_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	  }
#endif
#ifdef SLV_6332
	  pwrap_read_nochk(MT6332_DEW_READ_TEST, &rdata);
	  if (rdata != MT6332_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	  }
#endif

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
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
	u32 result[2] = {0, 0};
	s32 leading_one[2] = {-1, -1};
	s32 tailing_one[2] = {-1, -1};

	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , WACS2);

	/* --------------------------------------------------------------------- */
	/* Scan all possible input strobe by READ_TEST */
	/* --------------------------------------------------------------------- */
	for (ind = 0; ind < 24; ind++) {
		WRAP_WR32(PMIC_WRAP_SI_CK_CON , (ind >> 2) & 0x7);
		WRAP_WR32(PMIC_WRAP_SIDLY , 0x3 - (ind & 0x3));
#ifdef SLV_6328
		_pwrap_wacs2_nochk(0, MT6328_DEW_READ_TEST, 0, &rdata);
		if (rdata == MT6328_DEFAULT_VALUE_READ_TEST) {
			PWRAPLOG("_pwrap_init_sistrobe [Read Test of MT6328] pass");
			PWRAPLOG("index=%d rdata=%x\n", ind, rdata);
			result[0] |= (0x1 << ind);
		} else{
			PWRAPLOG("_pwrap_init_sistrobe [Read Test of MT6328]");
			PWRAPLOG("tuning,index=%d rdata=%x\n", ind, rdata);
		}
#endif
#ifdef SLV_6332
			_pwrap_wacs2_nochk(0, MT6332_DEW_READ_TEST, 0, &rdata);
			if (rdata == MT6332_DEFAULT_VALUE_READ_TEST) {
				PWRAPLOG("_pwrap_init_sistrobe [Read Test of MT6332] pass");
				PWRAPLOG("index=%d rdata=%x\n", ind, rdata);
				result[1] |= (0x1 << ind);
			} else {
				PWRAPLOG("_pwrap_init_sistrobe");
				PWRAPLOG("[Read Test of MT6332] tuning,index=%d rdata=%x\n", ind, rdata);
			}
#endif
	 }
#ifndef SLV_6328
	  result[0] = result[1];
#endif
#ifndef SLV_6332
	  result[1] = result[0];
#endif
	 /* --------------------------------------------------------------------- */
	/* Locate the leading one and trailing one of PMIC 1/2 */
	/* --------------------------------------------------------------------- */
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

	/* --------------------------------------------------------------------- */
	/* Check the continuity of pass range */
	/* --------------------------------------------------------------------- */
	for (i = 0; i < 2; i++) {
		tmp1 = (0x1 << (leading_one[i]+1)) - 1;
		tmp2 = (0x1 << tailing_one[i]) - 1;
		if ((tmp1 - tmp2) != result[i]) {
			PWRAPERR("_pwrap_init_sistrobe Fail at PMIC %d, result = %x", i+1, result[i]);
			PWRAPERR("leading_one:%d, tailing_one:%d\n", leading_one[i], tailing_one[i]);
		result_faulty = 0x1;
		}
	}

	/* --------------------------------------------------------------------- */
	/* Config SICK and SIDLY to the middle point of pass range */
	/* --------------------------------------------------------------------- */
	if (result_faulty == 0) {
		/* choose the best point in the interaction of PMIC1's pass range and PMIC2's pass range */
		ind = ((leading_one[0] + tailing_one[0])/2 + (leading_one[1] + tailing_one[1])/2)/2;
		/*TINFO = "The best point in the interaction area is %d, ind"*/
		WRAP_WR32(PMIC_WRAP_SI_CK_CON , (ind >> 2) & 0x7);
		WRAP_WR32(PMIC_WRAP_SIDLY , 0x3 - (ind & 0x3));
		/* --------------------------------------------------------------------- */
		/* Restore */
		/* --------------------------------------------------------------------- */
		WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
		return 0;
	}
	return 0;
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

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , DISABLE_ALL);
	WRAP_WR32(PMIC_WRAP_WRAP_EN , DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL , MANUAL_MODE);
	WRAP_WR32(PMIC_WRAP_MAN_EN , ENABLE);
	WRAP_WR32(PMIC_WRAP_DIO_EN , DISABLE);

	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_CSL  << 8));/* 0x2100 */
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8)); /* 0x2800//to reset counter */
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_CSH  << 8));/* 0x2000 */
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
	WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));

	return_value = wait_for_state_ready_init(wait_for_sync, TIMEOUT_WAIT_IDLE, PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_reset_spislv fail,return_value=%x\n", return_value);
		ret = E_PWR_TIMEOUT;
		goto timeout;
	}

	WRAP_WR32(PMIC_WRAP_MAN_EN , DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL , WRAPPER_MODE);

timeout:
	WRAP_WR32(PMIC_WRAP_MAN_EN , DISABLE);
	WRAP_WR32(PMIC_WRAP_MUX_SEL , WRAPPER_MODE);
	return ret;
}

static s32 _pwrap_init_reg_clock(u32 regck_sel)
{
	/* u32 wdata=0; */
	/* u32 rdata=0; */
	PWRAPFUC();

	/* Set Dummy cycle 6328 and 6332 (assume 12MHz) */
#ifdef SLV_6328
	  pwrap_write_nochk(MT6328_DEW_RDDMY_NO, 0x8);
#endif
#ifdef SLV_6332
	  pwrap_write_nochk(MT6332_DEW_RDDMY_NO, 0x8);
#endif
	WRAP_WR32(PMIC_WRAP_RDDMY , 0x88);

	/* Config SPI Waveform according to reg clk */
	if (regck_sel == 1) { /* 6MHz in 6323  => no support ; 18MHz in 6320 */
		/* for 6320, slave need enough time (4T of PMIC reg_ck) to back idle state */
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ    , 0x0);
		/* wait data written into register => 3T_PMIC: consists of CSLEXT_END(1T) + CSHEXT(6T) */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE   , 0x6);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START   , 0x0);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END   , 0x0);

	} else { /* Safe mode */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE   , 0xff);
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ    , 0xff);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START   , 0xf);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END   , 0xf);
	}

	return 0;
}

/* -------------------------------------------------------- */
/* Function : DrvPWRAP_Switch_Strobe_Setting() */
/* Description : used to switch input data calibration setting before system sleep or after system wakeup */
/* no use since SPI_CK (26MHz) is always kept unchanged */
/* Parameter : */
/* Return : */
/* -------------------------------------------------------- */
/* void DrvPWRAP_Switch_Strobe_Setting (int si_ck_con, int sidly) */
/* { */
/* int reg_rdata; */
/* // turn off spi_wrap */
/* *PMIC_WRAP_WRAP_EN = 0; */
/* // wait for WRAP to be in idle state */
/* // and no remaining rdata to be received */
/* do */
/* { */
/* reg_rdata = *PMIC_WRAP_WRAP_STA; */
/* } while ( (GET_WRAP_FSM(reg_rdata) != 0) || */
/* (GET_WRAP_CH_DLE_RESTCNT(reg_rdata)) != 0 ); */
/*  */
/* *PMIC_WRAP_SI_CK_CON = si_ck_con; */
/* *PMIC_WRAP_SIDLY = sidly; */
/*  */
/* // turn on spi_wrap */
/* *PMIC_WRAP_WRAP_EN = 1; */
/* } */

#if 0
static s32 _pwrap_init_signature(U8 path)
{
	int ret;
	u32 rdata = 0x0;

	PWRAPFUC();

	if (path == 1) {
		/* ############################### */
		/* Signature Checking - Using Write Test Register */
		/* should be the last to modify WRITE_TEST */
		/* ############################### */
		_pwrap_wacs2_nochk(1, MT6328_DEW_WRITE_TEST, 0x5678, &rdata);
		WRAP_WR32(PMIC_WRAP_SIG_ADR, MT6328_DEW_WRITE_TEST);
		WRAP_WR32(PMIC_WRAP_SIG_VALUE, 0x5678);
		WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x1);
	} else{
		/* ############################### */
		 /* Signature Checking using CRC and EINT update */
		/* should be the last to modify WRITE_TEST */
		/* ############################### */
#ifdef SLV_6328
		  ret = pwrap_write_nochk(MT6328_DEW_CRC_EN, ENABLE);
		  if (ret != 0) {
			PWRAPERR("MT6328 enable CRC fail,ret=%x\n", ret);
			return E_PWR_INIT_ENABLE_CRC;
		  }
		  WRAP_WR32(PMIC_WRAP_SIG_ADR, WRAP_RD32(PMIC_WRAP_SIG_ADR)|MT6328_DEW_CRC_VAL);
		  /* WRAP_WR32(PMIC_WRAP_EINT_STA0_ADR,MT6328_INT_STA); */
		  WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN, WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN)|0x35);
#endif
#ifdef SLV_6332
		  ret = pwrap_write_nochk(MT6332_DEW_CRC_EN, ENABLE);
		  if (ret != 0) {
				PWRAPERR("MT6332 enable CRC fail,ret=%x\n", ret);
				return E_PWR_INIT_ENABLE_CRC;
		  }
		  WRAP_WR32(PMIC_WRAP_SIG_ADR, WRAP_RD32(PMIC_WRAP_SIG_ADR)|(MT6332_DEW_CRC_VAL<<16));
		  WRAP_WR32(PMIC_WRAP_EINT_STA1_ADR, MT6332_INT_STA);
		  WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN, WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN)|0xa);
#endif
	}
	WRAP_WR32(PMIC_WRAP_CRC_EN, ENABLE);
	return 0;
}
#endif
/*
 *pmic_wrap init,init wrap interface
 *
 */
s32 pwrap_init(void)
{
	s32 sub_return = 0;
	s32 sub_return1 = 0;
	/* s32 ret=0; */
	u32 rdata = 0x0;
	/* u32 regValue; */
	/* u32 timeout=0; */
	/* u32 cg_mask = 0; */
	/* u32 backup = 0; */
	PWRAPFUC();
#ifdef CONFIG_OF
	sub_return = pwrap_of_iomap();
	if (sub_return)
		return sub_return;
#endif
	PWRAPLOG("mt_pwrap_init---- debug2\n");

	/* ############################### */
	/* toggle PMIC_WRAP and pwrap_spictl reset */
	/* ############################### */
	/* WRAP_SET_BIT(0x80,INFRA_GLOBALCON_RST0); */
	/* WRAP_CLR_BIT(0x80,INFRA_GLOBALCON_RST0); */
	__pwrap_soft_reset();
	PWRAPLOG("mt_pwrap_init---- debug3\n");

	/* ############################### */
	/* Set SPI_CK_freq = 26MHz */
	/* ############################### */
	WRAP_WR32(CLK_CFG_4_CLR, CLK_SPI_CK_26M);

	/* ############################### */
	/* toggle PERI_PWRAP_BRIDGE reset */
	/* ############################### */
	/* WRAP_SET_BIT(0x04,PERI_GLOBALCON_RST1); */
	/* WRAP_CLR_BIT(0x04,PERI_GLOBALCON_RST1); */

	/* ############################### */
	/* Enable DCM */
	/* ############################### */
	/* WRAP_WR32(PMIC_WRAP_DCM_EN , ENABLE); */
	WRAP_WR32(PMIC_WRAP_DCM_EN , 3);/* enable CRC DCM and Pwrap DCM */
	WRAP_WR32(PMIC_WRAP_DCM_DBC_PRD , DISABLE);
	PWRAPLOG("mt_pwrap_init---- debug4\n");

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
	WRAP_WR32(PMIC_WRAP_WRAP_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);
	WRAP_WR32(PMIC_WRAP_WACS2_EN, ENABLE);
	PWRAPLOG("mt_pwrap_init---- debug5\n");
	/* ############################### */
	/* Input data calibration flow; */
	/* ############################### */
	sub_return = _pwrap_init_sistrobe();
	if (sub_return != 0) {
		PWRAPERR("error,DrvPWRAP_InitSiStrobe fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_SIDLY;
	}
	PWRAPLOG("mt_pwrap_init---- debug6\n");
	/* ############################### */
	/* SPI Waveform Configuration */
	/* ############################### */
	/* 0:safe mode, 1:18MHz */
	sub_return = _pwrap_init_reg_clock(1);
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_init_reg_clock fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_REG_CLOCK;
	}

	/* ############################### */
	/* Enable DIO mode */
	/* ############################### */
	/* PMIC2 dual io not ready */
	/* /TODO: Fix me //for early porting */
#if 1
	sub_return = _pwrap_init_dio(1);
	if (sub_return != 0) {
		PWRAPERR("_pwrap_init_dio test error,error code=%x, sub_return=%x\n", 0x11, sub_return);
		return E_PWR_INIT_DIO;
	}
#endif
/* ############################### */
	/* Enable Encryption */
	/* ############################### */
	PWRAPLOG("have cipher\n");
	sub_return = _pwrap_init_cipher();
	if (sub_return != 0) {
		PWRAPERR("Enable Encryption fail, return=%x\n", sub_return);
		return E_PWR_INIT_CIPHER;
	}

	/* ############################### */
	/* Write test using WACS2 */
	/* ############################### */
	/* check Wtiet test default value */

#ifdef SLV_6328
	  sub_return = pwrap_write_nochk(MT6328_DEW_WRITE_TEST, MT6328_WRITE_TEST_VALUE);
	  sub_return1 = pwrap_read_nochk(MT6328_DEW_WRITE_TEST, &rdata);
	  if (rdata != MT6328_WRITE_TEST_VALUE)  {
			PWRAPERR("write test error,rdata=0x%x,exp=0xa55a", rdata);
			PWRAPERR("sub_return=0x%x,sub_return1=0x%x\n", sub_return, sub_return1);
			return E_PWR_INIT_WRITE_TEST;
	  }
#endif
#ifdef SLV_6332
	  sub_return = pwrap_write_nochk(MT6332_DEW_WRITE_TEST, MT6332_WRITE_TEST_VALUE);
	  sub_return1 = pwrap_read_nochk(MT6332_DEW_WRITE_TEST, &rdata);
	  if (rdata != MT6332_WRITE_TEST_VALUE)  {
			PWRAPERR("write test error,rdata=0x%x,exp=0xa55a", rdata);
			PWRAPERR("sub_return=0x%x,sub_return1=0x%x\n", sub_return, sub_return1);
			return E_PWR_INIT_WRITE_TEST;
	  }
#endif

	/* ############################### */
	/* Signature Checking - Using CRC */
	/* should be the last to modify WRITE_TEST */
	/* ############################### */
	 PWRAPLOG("mt_pwrap_init---- debug10\n");
/* /TODO: Fix me //for early porting  really no need? */

#if 0
	sub_return = _pwrap_init_signature(0);
	if (sub_return != 0) {
		PWRAPERR("Enable CRC fail, return=%x\n", sub_return);
		return E_PWR_INIT_ENABLE_CRC;
	}
#endif

#if 1
    /* ADC init */
	WRAP_WR32(PMIC_WRAP_ADC_CMD_ADDR, MT6328_AUXADC_RQST1_SET);
	WRAP_WR32(PMIC_WRAP_PWRAP_ADC_CMD, 0x0100);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST , MT6328_AUXADC_ADC32);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_WP     , MT6328_AUXADC_MDBG_1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR0       , MT6328_AUXADC_BUF0);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR1       , MT6328_AUXADC_BUF1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR2       , MT6328_AUXADC_BUF2);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR3       , MT6328_AUXADC_BUF3);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR4       , MT6328_AUXADC_BUF4);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR5       , MT6328_AUXADC_BUF5);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR6       , MT6328_AUXADC_BUF6);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR7       , MT6328_AUXADC_BUF7);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR8       , MT6328_AUXADC_BUF8);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR9       , MT6328_AUXADC_BUF9);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR10      , MT6328_AUXADC_BUF10);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR11      , MT6328_AUXADC_BUF11);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR12      , MT6328_AUXADC_BUF12);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR13      , MT6328_AUXADC_BUF13);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR14      , MT6328_AUXADC_BUF14);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR15      , MT6328_AUXADC_BUF15);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR16      , MT6328_AUXADC_BUF16);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR17      , MT6328_AUXADC_BUF17);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR18      , MT6328_AUXADC_BUF18);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR19      , MT6328_AUXADC_BUF19);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR20      , MT6328_AUXADC_BUF20);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR21      , MT6328_AUXADC_BUF21);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR22      , MT6328_AUXADC_BUF22);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR23      , MT6328_AUXADC_BUF23);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR24      , MT6328_AUXADC_BUF24);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR25      , MT6328_AUXADC_BUF25);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR26      , MT6328_AUXADC_BUF26);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR27      , MT6328_AUXADC_BUF27);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR28      , MT6328_AUXADC_BUF28);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR29      , MT6328_AUXADC_BUF29);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR30      , MT6328_AUXADC_BUF30);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR31      , MT6328_AUXADC_BUF31);


#endif

    /* adc 6328 setting */
	pwrap_write_nochk(MT6328_AUXADC_MDBG_0, (0x40|(1<<15)));
	pwrap_write_nochk(MT6328_AUXADC_MDRT_0, (0x40|(1<<15)));
	pwrap_write_nochk(MT6328_AUXADC_MDRT_2, 0x04);
	PWRAPLOG("mt_pwrap_init---- debug11 with adc &6328 adc\n");
	/* ############################### */
	/* PMIC_WRAP enables */
	/* ############################### */
  #if defined(CONFIG_ARCH_MT6735M)
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0xff);
	PWRAPLOG("mt_pwrap_init---- use D2\n");
  #else
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0x3ff);
  #endif
	WRAP_WR32(PMIC_WRAP_WACS0_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_WACS1_EN, ENABLE);
  #if defined(CONFIG_ARCH_MT6735M)
  /* not enable wacs3    D2 change */
  #else
	WRAP_WR32(PMIC_WRAP_WACS3_EN, ENABLE);
  #endif
	PWRAPLOG("mt_pwrap_init---- debug12 ok\n");
	WRAP_WR32(PMIC_WRAP_STAUPD_PRD, 0x5);  /* 0x1:20us,for concurrence test,MP:0x5;  //100us */
	WRAP_WR32(PMIC_WRAP_WDT_UNIT, 0xf);
	WRAP_WR32(PMIC_WRAP_WDT_SRC_EN, 0xfffffbff);
	WRAP_WR32(PMIC_WRAP_TIMER_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_INT_EN, 0xfffffbf9);
	PWRAPLOG("mt_pwrap_init---- debug12++\n");
	/* no ok */

	/* ############################### */
	/* Initialization Done */
	/* ############################### */
	WRAP_WR32(PMIC_WRAP_INIT_DONE0 , ENABLE);
	WRAP_WR32(PMIC_WRAP_INIT_DONE2 , ENABLE);
	PWRAPLOG("mt_pwrap_init---- debug13\n");
	#if defined(CONFIG_ARCH_MT6735M)
	/* not enable wacs3    D2 change */
	#else
	WRAP_WR32(PMIC_WRAP_INIT_DONE3 , ENABLE);
	#endif


	/* WRAP_WR32(PMIC_WRAP_INIT_DONE1 , ENABLE); */

	PWRAPLOG("mt_pwrap_init---- debug14\n");
	WRAP_WR32(PMIC_WRAP_EINT_STA0_ADR, MT6328_INT_STA);
#ifdef CONFIG_OF
	pwrap_of_iounmap();
#endif
	PWRAPLOG("mt_pwrap_init---- debug15,%x\n", WRAP_RD32(PMIC_WRAP_EINT_STA0_ADR));
	return 0;
}
/* EXPORT_SYMBOL(pwrap_init); */
/*Interrupt handler function*/
static int g_wrap_wdt_irq_count;
static int g_case_flag;
static irqreturn_t mt_pmic_wrap_irq(int irqno, void *dev_id)
{
	unsigned long flags = 0;

	PWRAPFUC();
	PWRAPREG("dump pwrap register\n");
	if ((WRAP_RD32(PMIC_WRAP_INT_FLG)&0x01) == 0x01) {
		g_wrap_wdt_irq_count++;
		g_case_flag = 0;
		PWRAPREG("g_wrap_wdt_irq_count=%d\n", g_wrap_wdt_irq_count);
	} else {
		g_case_flag = 1;
	}

	spin_lock_irqsave(&wrp_lock, flags);
	/* *----------------------------------------------------------------------- */
	PWRAPREG("infra clock1=0x%x\n", WRAP_RD32(infracfg_ao_base+0x48));
	pwrap_dump_all_register();
	/* raise the priority of WACS2 for AP */
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 1<<3);

	/* *----------------------------------------------------------------------- */
	/* clear interrupt flag */
	WRAP_WR32(PMIC_WRAP_INT_CLR, 0xffffffff);
	PWRAPREG("INT flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT_EN));
	PWRAPREG("infra clock2=0x%x\n", WRAP_RD32(infracfg_ao_base+0x48));
	if (10 == g_wrap_wdt_irq_count || 1 == g_case_flag)
		BUG_ON(1);
	spin_unlock_irqrestore(&wrp_lock, flags);
	return IRQ_HANDLED;
}

static u32 pwrap_read_test(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	/* Read Test */
#ifdef SLV_6328
	return_value = pwrap_read(MT6328_DEW_READ_TEST, &rdata);
	if (rdata != MT6328_DEFAULT_VALUE_READ_TEST) {
		PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata, return_value);
		return E_PWR_READ_TEST_FAIL;
	}
#endif
#ifdef SLV_6332
	return_value = pwrap_read(MT6332_DEW_READ_TEST, &rdata);
	if (rdata != MT6332_DEFAULT_VALUE_READ_TEST) {
		PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata, return_value);
		return E_PWR_READ_TEST_FAIL;
	}
#endif
	return 0;
}
static u32 pwrap_write_test(void)
{
	u32 rdata = 0;
	u32 sub_return = 0;
	u32 sub_return1 = 0;
	/* ############################### */
	/* Write test using WACS2 */
	/* ############################### */
#ifdef SLV_6328
	sub_return = pwrap_write(MT6328_DEW_WRITE_TEST, MT6328_WRITE_TEST_VALUE);
	PWRAPREG("after MT6328 pwrap_write\n");
	sub_return1 = pwrap_read(MT6328_DEW_WRITE_TEST, &rdata);
	if ((rdata != MT6328_WRITE_TEST_VALUE) || (sub_return != 0) || (sub_return1 != 0)) {
		PWRAPREG("write test error,rdata=0x%x,exp=0xa55a", rdata);
		PWRAPREG("sub_return=0x%x,sub_return1=0x%x\n", sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
#endif
#ifdef SLV_6332
	sub_return = pwrap_write(MT6332_DEW_WRITE_TEST, MT6332_WRITE_TEST_VALUE);
	PWRAPREG("after MT6332 pwrap_write\n");
	sub_return1 = pwrap_read(MT6332_DEW_WRITE_TEST, &rdata);
	if ((rdata != MT6332_WRITE_TEST_VALUE) || (sub_return != 0) || (sub_return1 != 0)) {
		PWRAPREG("write test error,rdata=0x%x", rdata);
		PWRAPREG("exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n", sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
#endif
	return 0;
}
static void pwrap_int_test(void)
{
	u32 rdata1 = 0;
	u32 rdata2 = 0;

	while (1) {
	   #ifdef SLV_6328
		rdata1 = WRAP_RD32(PMIC_WRAP_EINT_STA);
		pwrap_read(MT6328_INT_STA, &rdata2);
		PWRAPREG("Pwrap INT status check,PMIC_WRAP_EINT_STA=0x%x", rdata1);
		PWRAPREG("MT6328_INT_STA[0x01B4]=0x%x\n", rdata2);
	   #endif
	   #ifdef SLV_6332
		rdata1 = WRAP_RD32(PMIC_WRAP_EINT_STA);
		pwrap_read(MT6332_INT_STA, &rdata2);
		PWRAPREG("Pwrap INT status check,PMIC_WRAP_EINT_STA=0x%x", rdata1);
		PWRAPREG("MT6332_INT_STA[0x8112]=0x%x\n", rdata2);
	   #endif
	   msleep(500);
	}
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
		PWRAPREG("PWRAP debug: [-dump_reg][-trace_wacs2][-init]");
		PWRAPREG("[-rdap][-wrap][-rdpmic][-wrpmic][-readtest][-writetest]\n");
		PWRAPREG("PWRAP UT: [1][2]\n");
	}
	/* ------------pwrap debug-------------------- */
	else if (!strncmp(buf, "-dump_reg", 9))
		pwrap_dump_all_register();
	else if (!strncmp(buf, "-trace_wacs2", 12))
		;/* pwrap_trace_wacs2(); */
	else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	} else if (!strncmp(buf, "-rdap", 5) && (1 == sscanf(buf+5, "%x", &reg_addr)))
		;/* pwrap_read_reg_on_ap(reg_addr); */
	else if (!strncmp(buf, "-wrap", 5) && (2 == sscanf(buf+5, "%x %x", &reg_addr, &reg_value)))
		;/* pwrap_write_reg_on_ap(reg_addr,reg_value); */
	else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf+7, "%x", &reg_addr)))
		;/* pwrap_read_reg_on_pmic(reg_addr); */
	else if (!strncmp(buf, "-wrpmic", 7) && (2 == sscanf(buf+7, "%x %x", &reg_addr, &reg_value)))
		;/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	else if (!strncmp(buf, "-readtest", 9))
		pwrap_read_test();
	else if (!strncmp(buf, "-writetest", 10))
		pwrap_write_test();
	else if (!strncmp(buf, "-int", 4))
		pwrap_int_test();
	/* ----------pwrap UT---------------- */
	else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf+3, "%d", &ut_test)))
		pwrap_ut(ut_test);
	else
		PWRAPREG("wrong parameter\n");
	return count;
}
/*---------------------------------------------------------------------------*/
#endif /* endif PMIC_WRAP_NO_PMIC */
#ifdef CONFIG_OF
static int pwrap_of_iomap(void)
{
	/*
	 * Map the address of the following register base:
	 * INFRACFG_AO, TOPCKGEN, SCP_CLK_CTRL, SCP_PMICWP2P
	 */

	struct device_node *infracfg_ao_node;
	struct device_node *topckgen_node;

	infracfg_ao_node =
		of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	if (!infracfg_ao_node) {
		pr_warn("get INFRACFG_AO failed\n");
		return -ENODEV;
	}

	infracfg_ao_base = of_iomap(infracfg_ao_node, 0);
	if (!infracfg_ao_base) {
		pr_warn("INFRACFG_AO iomap failed\n");
		return -ENOMEM;
	}

	topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,CKSYS");
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
	/* iounmap(infracfg_ao_base); */
	iounmap(topckgen_base);
}
#endif

#define VERSION     "Revision"
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
static int __init pwrap_hal_init(void)
{
	s32 ret = 0;
#ifdef CONFIG_OF
	u32 pwrap_irq;
	struct device_node *pwrap_node;

	PWRAPLOG("mt_pwrap_init\n");
	g_wrap_wdt_irq_count = 0;
	g_case_flag = 0;
	pwrap_node = of_find_compatible_node(NULL, NULL, "mediatek,PWRAP");
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
	PWRAPLOG("real init: version %s\n", VERSION);
	mt_wrp = get_mt_pmic_wrap_drv();
	mt_wrp->store_hal = mt_pwrap_store_hal;
	mt_wrp->show_hal = mt_pwrap_show_hal;
	mt_wrp->wacs2_hal = pwrap_wacs2_hal;

	PWRAPLOG("mt_pwrap_init---- debug1\n");
	pwrap_of_iomap();



	if (is_pwrap_init_done() == 0) {
	  #ifdef PMIC_WRAP_NO_PMIC
	  #else
		ret = request_irq(MT_PMIC_WRAP_IRQ_ID, mt_pmic_wrap_irq, IRQF_TRIGGER_HIGH, PMIC_WRAP_DEVICE, 0);
	  #endif
		if (ret) {
			PWRAPERR("register IRQ failed (%d)\n", ret);
			return ret;
		}
	} else{
		PWRAPERR("not init (%d)\n", ret);
	}
	pwrap_ut(1);
	pwrap_ut(2);
	PWRAPLOG("mt_pwrap_init----\n");
	return ret;
}
postcore_initcall(pwrap_hal_init);

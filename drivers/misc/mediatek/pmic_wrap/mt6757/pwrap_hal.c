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
#include <mt-plat/mt_gpio.h>
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
static s32 _pwrap_init_dio(u32 dio_en);
static s32 _pwrap_init_cipher(void);
static s32 _pwrap_init_reg_clock(u32 regck_sel);

static s32 _pwrap_wacs2_nochk(u32 write, u32 adr, u32 wdata, u32 *rdata);
static void _pwrap_enable(void);
static void _pwrap_starve_set(void);
static void _pwrap_adc_set(void);

#ifdef CONFIG_OF
static int pwrap_of_iomap(void);
static void pwrap_of_iounmap(void);
#endif

/****************** For test *********************************/
static u32 pwrap_read_test(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	/* Read Test */
#ifdef SLV_6351
	return_value = pwrap_read(MT6351_DEW_READ_TEST, &rdata);
	if (rdata != MT6351_DEFAULT_VALUE_READ_TEST) {
		PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata,
			 return_value);
		return E_PWR_READ_TEST_FAIL;
	}
	PWRAPREG("Read MT6351 Test pass,return_value=%d\n", return_value);
#endif

	return 0;
}

static u32 pwrap_write_test(void)
{
	u32 rdata = 0;
	u32 sub_return = 0;
	u32 sub_return1 = 0;

	/* Write test using WACS2 */
#ifdef SLV_6351
	sub_return = pwrap_write(MT6351_DEW_WRITE_TEST, MT6351_WRITE_TEST_VALUE);
	PWRAPREG("after MT6351 pwrap_write\n");
	sub_return1 = pwrap_read(MT6351_DEW_WRITE_TEST, &rdata);
	if ((rdata != MT6351_WRITE_TEST_VALUE) || (sub_return != 0) || (sub_return1 != 0)) {
		PWRAPREG
		    ("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n",
		     rdata, sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
	PWRAPREG("write MT6351 Test pass\n");
#endif

	return 0;
}

static void pwrap_ut(u32 ut_test)
{
	switch (ut_test) {
	case 1:
		pwrap_write_test();
		break;
	case 2:
		pwrap_read_test();
		break;
	default:
		PWRAPREG("default test.\n");
		break;
	}
}

/****************************************************************/

#ifdef PMIC_WRAP_NO_PMIC
/*-pwrap debug--------------------------------------------------------------------------*/
static inline void pwrap_dump_all_register(void)
{
	PWRAPLOG("Nothing to do\n");
}

/********************************************************************************************/
/* extern API for PMIC driver, INT related control, this INT is for PMIC chip to AP */
/********************************************************************************************/
u32 mt_pmic_wrap_eint_status(void)
{
	PWRAPLOG("[PMIC2]first of all PMIC_WRAP_STAUPD_GRPEN=0x%x\n",
		 WRAP_RD32(PMIC_WRAP_STAUPD_GRPEN));
	PWRAPLOG("[PMIC2]first of all PMIC_WRAP_EINT_STA=0x%x\n", WRAP_RD32(PMIC_WRAP_EINT_STA));
	return WRAP_RD32(PMIC_WRAP_EINT_STA);
}

void mt_pmic_wrap_eint_clr(int offset)
{
	if ((offset < 0) || (offset > 3))
		PWRAPERR("clear EINT flag error, only 0-3 bit\n");
	else
		WRAP_WR32(PMIC_WRAP_EINT_CLR, (1 << offset));
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
	pwrap_dump_all_register();
	/* raise the priority of WACS2 for AP */
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 1 << 3);

	/* clear interrupt flag */
	WRAP_WR32(PMIC_WRAP_INT0_CLR, 0xffffffff);
	PWRAPREG("INT flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT0_EN));
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
	} else if (!strncmp(buf, "-dump_reg", 9))
		pwrap_dump_all_register();
	else if (!strncmp(buf, "-trace_wacs2", 12))
		/* pwrap_trace_wacs2(); */
	else if (!strncmp(buf, "-init", 5)) {
		return_value = pwrap_init();
		if (return_value == 0)
			PWRAPREG("pwrap_init pass,return_value=%d\n", return_value);
		else
			PWRAPREG("pwrap_init fail,return_value=%d\n", return_value);
	} else if (!strncmp(buf, "-rdap", 5) && (1 == sscanf(buf+5, "%x", &reg_addr)))
		/* pwrap_read_reg_on_ap(reg_addr); */
	else if (!strncmp(buf, "-wrap", 5) && (2 == sscanf(buf+5, "%x %x", &reg_addr, &reg_value)))
		/* pwrap_write_reg_on_ap(reg_addr,reg_value); */
	else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf+7, "%x", &reg_addr)))
		/* pwrap_read_reg_on_pmic(reg_addr); */
	else if (!strncmp(buf, "-wrpmic", 7) && (2 == sscanf(buf+7, "%x %x", &reg_addr, &reg_value)))
		/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	else if (!strncmp(buf, "-readtest", 9))
		pwrap_read_test();
	else if (!strncmp(buf, "-writetest", 10))
		pwrap_write_test();
	else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf+3, "%d", &ut_test)))
		pwrap_ut(ut_test);
	else
		PWRAPREG("wrong parameter\n");

	return count;
}

/*---------------------------------------------------------------------------*/
#else
/*-pwrap debug--------------------------------------------------------------------------*/
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

static inline void pwrap_dump_pmic_register(void)
{
}

static inline void pwrap_dump_all_register(void)
{
	pwrap_dump_ap_register();
	pwrap_dump_pmic_register();
}

static void __pwrap_soft_reset(void)
{
	PWRAPLOG("start reset wrapper\n");
	WRAP_WR32(INFRA_GLOBALCON_RST0, 0x1);
	WRAP_WR32(INFRA_GLOBALCON_RST1, 0x1);
}

#ifndef CONFIG_MTK_FPGA
static void __pwrap_spi_clk_set(void)
{
	PWRAPLOG("spi clk set ....\n");
	WRAP_WR32(CLK_CFG_5_CLR, CLK_SPI_CK_26M);
	/*sys_ck cg enable */
	WRAP_WR32(MODULE_SW_CG_0_SET, 0x0000000F);
	WRAP_WR32(MODULE_SW_CG_0_CLR, 0x0000000F);
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

static BOOL _pwrap_timeout_ns(u64 start_time_ns, u64 elapse_time)	/* ,u64 timeout_ns) */
{
	return 0;
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
		 *read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;write:FSM_REQ-->idle */
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
		if (GET_INIT_DONE0(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
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

		if (GET_INIT_DONE0(reg_rdata) != WACS_INIT_DONE) {
			PWRAPERR("initialization isn't finished\n");
			return E_PWR_NOT_INIT_DONE;
		}
	} while (fp(reg_rdata));
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

/***********************************
 * Function : pwrap_wacs2_hal()
 * Description :
 * Parameter :
 * Return :
 ***********************************/
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
			return_value += 1;
			goto FAIL;
		}
		*rdata = GET_WACS0_RDATA(reg_rdata);
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


/**********************************
 * Function : _pwrap_wacs2_nochk()
 * Description : For external user API
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
		if (NULL == rdata) {
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
		*rdata = GET_WACS0_RDATA(reg_rdata);
		WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR, 1);
	}

	return 0;
}


/************************************************
 * Function : _pwrap_init_dio()
 * Description :call it in pwrap_init,mustn't check init done
 * Parameter :
 * Return :
 ************************************************/
static s32 _pwrap_init_dio(u32 dio_en)
{
	u32 arb_en_backup = 0x0;
	/* u32 rdata = 0x0; */
	u32 return_value = 0;

	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);
#ifdef SLV_6351
	pwrap_write_nochk(MT6351_DEW_DIO_EN, (dio_en));
#endif

	/* Check IDLE & INIT_DONE in advance */
	return_value =
	    wait_for_state_ready_init(wait_for_idle_and_sync, TIMEOUT_WAIT_IDLE,
				      PMIC_WRAP_WACS2_RDATA, 0);
	if (return_value != 0) {
		PWRAPERR("_pwrap_init_dio fail,return_value=%x\n", return_value);
		return return_value;
	}
	/* enable AP DIO mode */
	WRAP_WR32(PMIC_WRAP_DIO_EN, dio_en);
	/* Read Test */
/* #ifdef SLV_6351
	pwrap_read_nochk(MT6351_DEW_READ_TEST, &rdata);
	if (rdata != MT6351_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("[Dio_mode][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=%x\n",
			 dio_en, rdata, MT6351_DEFAULT_VALUE_READ_TEST);
		return E_PWR_READ_TEST_FAIL;
	}
#endif */
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);

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
	u32 arb_en_backup = 0;
	u32 rdata = 0;
	u32 return_value = 0;
	u32 start_time_ns = 0, timeout_ns = 0;

	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);
	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_SWRST, 0);
	WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL, 1);
	WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL, 2);
	WRAP_WR32(PMIC_WRAP_CIPHER_EN, 1);
	/* Config CIPHER @ PMIC */
#ifdef SLV_6351
	pwrap_write_nochk(MT6351_DEW_CIPHER_SWRST, 0x1);
	pwrap_write_nochk(MT6351_DEW_CIPHER_SWRST, 0x0);
	pwrap_write_nochk(MT6351_DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(MT6351_DEW_CIPHER_IV_SEL, 0x2);
	pwrap_write_nochk(MT6351_DEW_CIPHER_EN, 0x1);
#endif

	/*wait for cipher data ready@AP */
	return_value =
	    wait_for_state_ready_init(wait_for_cipher_ready, TIMEOUT_WAIT_IDLE,
				      PMIC_WRAP_CIPHER_RDY, 0);
	if (return_value != 0) {
		PWRAPERR("wait for cipher data ready@AP fail,return_value=%x\n", return_value);
		return return_value;
	}
	PWRAPLOG("cipher data ready@AP\n");

	/* wait for cipher data ready@PMIC */
#ifdef SLV_6351
	start_time_ns = _pwrap_get_current_time();
	timeout_ns = _pwrap_time2ns(0xFFFFFF);
	do {
		if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
			PWRAPERR("wait for cipher data ready@PMIC\n");
		pwrap_read_nochk(MT6351_DEW_CIPHER_RDY, &rdata);
	} while (rdata != 0x1);	/* cipher_ready */

	return_value = pwrap_write_nochk(MT6351_DEW_CIPHER_MODE, 0x1);
	if (return_value != 0) {
		PWRAPERR("write MT6351_DEW_CIPHER_MODE fail,return_value=%x\n", return_value);
		return return_value;
	}
#endif
	PWRAPLOG("cipher data ready@PMIC\n");

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
#ifdef SLV_6351
	pwrap_read_nochk(MT6351_DEW_READ_TEST, &rdata);
	if (rdata != MT6351_DEFAULT_VALUE_READ_TEST) {
		PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
		return E_PWR_READ_TEST_FAIL;
	}
#endif

	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);

	return 0;
}

static void _pwrap_adc_set(void)
{
	/*pwrap_write_nochk(MT6351_DEW_CRC_EN, 0x1);*/

	WRAP_WR32(PMIC_WRAP_SIG_ADR, MT6351_DEW_CRC_VAL);
	WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN, 0x75);
	/*WRAP_WR32(PMIC_WRAP_CRC_EN, 0x1);*/
	WRAP_WR32(PMIC_WRAP_EINT_STA0_ADR, MT6351_INT_STA);

	WRAP_WR32(PMIC_WRAP_ADC_CMD_ADDR, MT6351_AUXADC_RQST1_SET);
	WRAP_WR32(PMIC_WRAP_PWRAP_ADC_CMD, 0x0100);
	WRAP_WR32(PMIC_WRAP_ADC_RDATA_ADDR, MT6351_AUXADC_ADC16);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST, MT6351_AUXADC_ADC32);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR_WP, MT6351_AUXADC_MDBG_1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR0, MT6351_AUXADC_BUF0);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR1, MT6351_AUXADC_BUF1);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR2, MT6351_AUXADC_BUF2);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR3, MT6351_AUXADC_BUF3);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR4, MT6351_AUXADC_BUF4);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR5, MT6351_AUXADC_BUF5);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR6, MT6351_AUXADC_BUF6);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR7, MT6351_AUXADC_BUF7);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR8, MT6351_AUXADC_BUF8);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR9, MT6351_AUXADC_BUF9);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR10, MT6351_AUXADC_BUF10);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR11, MT6351_AUXADC_BUF11);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR12, MT6351_AUXADC_BUF12);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR13, MT6351_AUXADC_BUF13);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR14, MT6351_AUXADC_BUF14);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR15, MT6351_AUXADC_BUF15);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR16, MT6351_AUXADC_BUF16);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR17, MT6351_AUXADC_BUF17);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR18, MT6351_AUXADC_BUF18);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR19, MT6351_AUXADC_BUF19);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR20, MT6351_AUXADC_BUF20);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR21, MT6351_AUXADC_BUF21);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR22, MT6351_AUXADC_BUF22);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR23, MT6351_AUXADC_BUF23);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR24, MT6351_AUXADC_BUF24);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR25, MT6351_AUXADC_BUF25);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR26, MT6351_AUXADC_BUF26);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR27, MT6351_AUXADC_BUF27);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR28, MT6351_AUXADC_BUF28);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR29, MT6351_AUXADC_BUF29);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR30, MT6351_AUXADC_BUF30);
	WRAP_WR32(PMIC_WRAP_MD_ADC_RDATA_ADDR31, MT6351_AUXADC_BUF31);

}

static void _pwrap_starve_set(void)
{
	WRAP_WR32(PMIC_WRAP_HARB_HPRIO, 0x0007);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_0, 0x0402);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_1, 0x0402);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_2, 0x0403);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_3, 0x0414);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_4, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_5, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_6, 0x0420);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_7, 0x0428);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_8, 0x0428);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_9, 0x0417);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_10, 0x0563);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_11, 0x047C);
	WRAP_WR32(PMIC_WRAP_STARV_COUNTER_12, 0x0740);

}

static void _pwrap_enable(void)
{
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, 0x01fff);
	WRAP_WR32(PMIC_WRAP_WACS0_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_WACS1_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_WACS3_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_STAUPD_PRD, 0x5);
	WRAP_WR32(PMIC_WRAP_WDT_UNIT, 0xf);
	WRAP_WR32(PMIC_WRAP_WDT_SRC_EN, 0xffffffff);
	WRAP_WR32(PMIC_WRAP_TIMER_EN, 0x1);
	WRAP_WR32(PMIC_WRAP_INT0_EN, 0xfffffffd);
	WRAP_WR32(PMIC_WRAP_INT1_EN, 0x0001ffff);

}

/************************************************
 * Function : _pwrap_init_sistrobe()
 * scription : Initialize SI_CK_CON and SIDLY
 * Parameter :
 * Return :
 ************************************************/
static s32 _pwrap_init_sistrobe(void)
{

	u32 arb_en_backup = 0;
	u32 rdata = 0;
	s32 ind = 0, sidly = 0;
	u32 result_faulty = 0;
	u64 result = 0, tmp1 = 0, tmp2 = 0;
	s32 leading_one = 0;
	s32 tailing_one = 0;
	s32 i = 0, fail_cnt = 0, ret = 0;
	u32 test_data[30] = {0x6996, 0x9669, 0x6996, 0x9669, 0x6996,
						 0x9669, 0x6996, 0x9669, 0x6996, 0x9669,
						 0x5AA5, 0xA55A, 0x5AA5, 0xA55A, 0x5AA5,
						 0xA55A, 0x5AA5, 0xA55A, 0x5AA5, 0xA55A,
						 0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27,
		0x1B27, 0x1B27, 0x1B27, 0x1B27, 0x1B27
	};

	arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);	/* only WACS2 */

	/* --------------------------------------------------------------------- */
	/* Scan all possible input strobe by READ_TEST */
	/* --------------------------------------------------------------------- */
	for (ind = 0; ind < 24; ind++) {	/* 24 sampling clock edge */
		WRAP_WR32(PMIC_WRAP_SI_CK_CON, (ind >> 2) & 0x7);
		WRAP_WR32(PMIC_WRAP_SIDLY, 0x3 - (ind & 0x3));
#ifdef SLV_6351
		fail_cnt = 0;
		for (i = 0; i < 30; i++) {
			_pwrap_wacs2_nochk(1, MT6351_DEW_WRITE_TEST, test_data[i], &rdata);
			_pwrap_wacs2_nochk(0, MT6351_DEW_WRITE_TEST, 0, &rdata);

			if (rdata != test_data[i]) {
				fail_cnt++;
				PWRAPLOG("_pwrap_init_sistrobe tuning, index=%d rdata =%x\n", ind,
					 rdata);
				break;
			} else {
				PWRAPLOG("_pwrap_init_sistrobe pass,index=%d rdata=%x\n", ind,
					 rdata);
			}
		}
		if (fail_cnt == 0)
			result |= (0x1 << ind);
#endif
	}

	/* --------------------------------------------------------------------- */
	/* Locate the leading one and trailing one of PMIC 1/2 */
	/* --------------------------------------------------------------------- */
	for (ind = 23; ind >= 0; ind--) {
		if (result & (0x1 << ind))
			break;

	}
	leading_one = ind;

	for (ind = 0; ind < 24; ind++) {
		if (result & (0x1 << ind))
			break;
	}
	tailing_one = ind;

	/* --------------------------------------------------------------------- */
	/* Check the continuity of pass range */
	/* --------------------------------------------------------------------- */
	tmp1 = (0x1 << (leading_one + 1)) - 1;
	tmp2 = (0x1 << tailing_one) - 1;
	if ((tmp1 - tmp2) != result) {
		PWRAPERR
		    ("_pwrap_init_sistrobe Fail, result = %llx, leading_one:%d, tailing_one:%d\n",
				result, leading_one, tailing_one);
			result_faulty = 0x1;
		}
	/* --------------------------------------------------------------------- */
	/* Config SICK and SIDLY to the middle point of pass range */
	/* --------------------------------------------------------------------- */
	if (result_faulty == 0) {
		ind = (leading_one + tailing_one) / 2;
		WRAP_WR32(PMIC_WRAP_SI_CK_CON, (ind >> 2) & 0x7);
		sidly = 0x3 - (ind & 0x3);
		if (sidly != 0)
			sidly--;
		WRAP_WR32(PMIC_WRAP_SIDLY , sidly);
		/* --------------------------------------------------------------------- */
		/* Restore */
		/* --------------------------------------------------------------------- */
		WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, arb_en_backup);
	} else {
		PWRAPERR("_pwrap_init_sistrobe Fail,result_faulty=%x\n", result_faulty);
		ret = result_faulty;
	}
	return ret;
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
/* Set Dummy cycle 6351 (assume 18MHz) */
#ifdef SLV_6351
	pwrap_write_nochk(MT6351_DEW_RDDMY_NO, 0x8);
#endif

	WRAP_WR32(PMIC_WRAP_RDDMY, 0x8);

	/* Config SPI Waveform according to reg clk */
	if (regck_sel == 1) {
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0x0);
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0x33);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START, 0x0);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END, 0x0);
	} else {		/* Safe mode */
		WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE, 0xff);
		WRAP_WR32(PMIC_WRAP_CSHEXT_READ, 0xff);
		WRAP_WR32(PMIC_WRAP_CSLEXT_START, 0xf);
		WRAP_WR32(PMIC_WRAP_CSLEXT_END, 0xf);
	}
	return 0;
}


/*
 *pmic_wrap init,init wrap interface
 *
 */
s32 pwrap_init(void)
{
	s32 sub_return = 0;
	s32 sub_return1 = 0;
	u32 rdata = 0x0;

	PWRAPFUC();
#ifdef CONFIG_OF
	sub_return = pwrap_of_iomap();
	if (sub_return)
		return sub_return;
#endif

	PWRAPLOG("kernel pwrap_init start!!!!!!!!!!!!!\n");

	/* toggle PMIC_WRAP and pwrap_spictl reset */
	__pwrap_soft_reset();
	PWRAPLOG("soft reset ok\n");

	/* Set SPI_CK_freq = 26MHz. It can marked when use fpga verify, because it has no address on Everest fpga */
#ifdef CONFIG_MTK_FPGA
#else
	__pwrap_spi_clk_set();
#endif
	PWRAPLOG("spi clk set ok\n");

	/* Enable DCM */
	WRAP_WR32(PMIC_WRAP_DCM_EN, 3);
	/* no debounce */
	WRAP_WR32(PMIC_WRAP_DCM_DBC_PRD, DISABLE);
	PWRAPLOG("enable DCM ok\n");

	/* Reset SPISLV */
	sub_return = _pwrap_reset_spislv();
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_reset_spislv fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_RESET_SPI;
	}
	PWRAPLOG("reset spislv ok\n");

	/* Enable WACS2 */
	WRAP_WR32(PMIC_WRAP_WRAP_EN, ENABLE);
	WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN, WACS2);
	WRAP_WR32(PMIC_WRAP_WACS2_EN, ENABLE);
	PWRAPLOG("enable WACS2 ok\n");

	/* SPI Waveform Configuration. 0:safe mode, 1:18MHz */
	sub_return = _pwrap_init_reg_clock(1);
	if (sub_return != 0) {
		PWRAPERR("error,_pwrap_init_reg_clock fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_REG_CLOCK;
	}
	PWRAPLOG("set reg clock ok\n");

	/* Enable DIO mode */
	sub_return = _pwrap_init_dio(1);
	if (sub_return != 0) {
		PWRAPERR("_pwrap_init_dio test error,error code=%x, sub_return=%x\n", 0x11,
			 sub_return);
		return E_PWR_INIT_DIO;
	}
	PWRAPLOG("enable dio ok\n");

	/* Input data calibration flow; */
	sub_return = _pwrap_init_sistrobe();
	if (sub_return != 0) {
		PWRAPERR("error,DrvPWRAP_InitSiStrobe fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_SIDLY;
	}
	PWRAPLOG("sistrobe ok\n");

	/* Enable Encryption */
	sub_return = _pwrap_init_cipher();
	if (sub_return != 0) {
		PWRAPERR("Enable Encryption fail, return=%x\n", sub_return);
		return E_PWR_INIT_CIPHER;
	}
	PWRAPLOG("enable cipher ok\n");

	/*  Write test using WACS2.  check Wtiet test default value */
#ifdef SLV_6351
	sub_return = pwrap_write_nochk(MT6351_DEW_WRITE_TEST, MT6351_WRITE_TEST_VALUE);
	sub_return1 = pwrap_read_nochk(MT6351_DEW_WRITE_TEST, &rdata);
	if (rdata != MT6351_WRITE_TEST_VALUE) {
		PWRAPERR
		    ("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n",
		     rdata, sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}
#endif
	PWRAPLOG("write test ok\n");

	/* Status update function initialization
	 * 1. Signature Checking using CRC (CRC 0 only)
	 * 2. EINT update
	 * 3. Read back Auxadc thermal data for GPD
	 * 4. Read back Auxadc thermal data for LTE
	 */
	_pwrap_adc_set();
	PWRAPLOG("adc set ok\n");

	/* PMIC WRAP priority adjust */
	WRAP_WR32(PMIC_WRAP_PRIORITY_USER_SEL_0, 0x6543C210);
	WRAP_WR32(PMIC_WRAP_PRIORITY_USER_SEL_1, 0xFEDBA987);
	WRAP_WR32(PMIC_WRAP_ARBITER_OUT_SEL_0, 0x87654210);
	WRAP_WR32(PMIC_WRAP_ARBITER_OUT_SEL_1, 0xFED3CBA9);

	/* PMIC_WRAP starvation setting */
	_pwrap_starve_set();
	PWRAPLOG("starve set ok\n");

	/* PMIC_WRAP enables */
	_pwrap_enable();
	PWRAPLOG("enable ok\n");

	/* Initialization Done */
	WRAP_WR32(PMIC_WRAP_INIT_DONE0, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE1, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE2, 0x1);
	WRAP_WR32(PMIC_WRAP_INIT_DONE3, 0x1);

	PWRAPLOG("kernel pwrap_init Done!!!!!!!!!\n");

#ifdef CONFIG_OF
	pwrap_of_iounmap();
#endif

	return 0;
}

/*Interrupt handler function*/
static int g_wrap_wdt_irq_count;
static int g_case_flag;
static irqreturn_t mt_pmic_wrap_irq(int irqno, void *dev_id)
{
	unsigned long flags = 0;

	PWRAPFUC();
	PWRAPREG("dump pwrap register\n");
	if ((WRAP_RD32(PMIC_WRAP_INT0_FLG) & 0x01) == 0x01) {
		g_wrap_wdt_irq_count++;
		g_case_flag = 0;
		PWRAPREG("g_wrap_wdt_irq_count=%d\n", g_wrap_wdt_irq_count);

	} else {
		g_case_flag = 1;
	}
/* Check INT1_FLA Status, if wrong status, dump all register info */
	if ((WRAP_RD32(PMIC_WRAP_INT1_FLG) & 0xffffff) != 0) {
		WRAP_WR32(PMIC_WRAP_INT1_CLR, 0xffffffff);
		PWRAPREG("INT1_FLG status Wrong,value=0x%x\n", WRAP_RD32(PMIC_WRAP_INT1_FLG));
	}
	spin_lock_irqsave(&wrp_lock, flags);
	PWRAPREG("infra clock status=0x%x\n", WRAP_RD32(MODULE_SW_CG_0_STA));
	PWRAPREG("pdn_pmicspi clock status=0x%x\n", WRAP_RD32(CLK_CFG_5_STA));
	pwrap_dump_all_register();

	/* clear interrupt flag */
	WRAP_WR32(PMIC_WRAP_INT0_CLR, 0xffffffff);
	PWRAPREG("INT0 flag 0x%x\n", WRAP_RD32(PMIC_WRAP_INT0_EN));
	if (10 == g_wrap_wdt_irq_count || 1 == g_case_flag)
		BUG_ON(1);

	spin_unlock_irqrestore(&wrp_lock, flags);

	return IRQ_HANDLED;
}

static void pwrap_int_test(void)
{
	u32 rdata1 = 0;
	u32 rdata2 = 0;

	while (1) {
#ifdef SLV_6351
		rdata1 = WRAP_RD32(PMIC_WRAP_EINT_STA);
		pwrap_read(MT6351_INT_STA, &rdata2);
		PWRAPREG
		    ("Pwrap INT status check,PMIC_WRAP_EINT_STA=0x%x,MT6351_INT_STA[0x01B4]=0x%x\n",
		     rdata1, rdata2);
#endif
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
		PWRAPREG("PWRAP debug: [-dump_reg][-trace_wacs2][-init][-rdap][-wrap][-rdpmic]");
		PWRAPREG("[-wrpmic][-readtest][-writetest]\n");
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
	} else if (!strncmp(buf, "-rdap", 5) && (1 == sscanf(buf + 5, "%x", &reg_addr))) {
		/* pwrap_read_reg_on_ap(reg_addr); */
	} else if (!strncmp(buf, "-wrap", 5)
		   && (2 == sscanf(buf + 5, "%x %x", &reg_addr, &reg_value))) {
		/* pwrap_write_reg_on_ap(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf + 7, "%x", &reg_addr))) {
		/* pwrap_read_reg_on_pmic(reg_addr); */
	} else if (!strncmp(buf, "-wrpmic", 7)
		   && (2 == sscanf(buf + 7, "%x %x", &reg_addr, &reg_value))) {
		/* pwrap_write_reg_on_pmic(reg_addr,reg_value); */
	} else if (!strncmp(buf, "-readtest", 9)) {
		pwrap_read_test();
	} else if (!strncmp(buf, "-writetest", 10)) {
		pwrap_write_test();
	} else if (!strncmp(buf, "-int", 4)) {
		pwrap_int_test();
	} else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf + 3, "%d", &ut_test))) {
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

	struct device_node *infracfg_ao_node;
	struct device_node *topckgen_node;

	infracfg_ao_node = of_find_compatible_node(NULL, NULL, "mediatek,INFRACFG_AO");
	if (!infracfg_ao_node) {
		pr_warn("get INFRACFG_AO failed\n");
		return -ENODEV;
	}

	infracfg_ao_base = of_iomap(infracfg_ao_node, 0);
	if (!infracfg_ao_base) {
		pr_warn("INFRACFG_AO iomap failed\n");
		return -ENOMEM;
	}

	topckgen_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPCKGEN");
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

static int __init pwrap_hal_init(void)
{
	s32 ret = 0;
#ifdef CONFIG_OF
	u32 pwrap_irq;
	struct device_node *pwrap_node;

	PWRAPLOG("mt_pwrap_init++++\n");
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
	mt_wrp = get_mt_pmic_wrap_drv();
	mt_wrp->store_hal = mt_pwrap_store_hal;
	mt_wrp->show_hal = mt_pwrap_show_hal;
	mt_wrp->wacs2_hal = pwrap_wacs2_hal;

	PWRAPLOG("mt_pwrap_init---- debug1\n");
	pwrap_of_iomap();

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

	pwrap_ut(1);
	pwrap_ut(2);
	PWRAPLOG("mt_pwrap_init----\n");
	return ret;
}

postcore_initcall(pwrap_hal_init);

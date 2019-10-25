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
#ifndef __PMIC_WRAP_INIT_H__
#define __PMIC_WRAP_INIT_H__

/****** SW ENV define *************************************/
#define PMIC_WRAP_PRELOADER 0
#define PMIC_WRAP_LK 0
#define PMIC_WRAP_KERNEL 1
#define PMIC_WRAP_SCP 0
#define PMIC_WRAP_CTP 0

#define PMIC_WRAP_DEBUG
#define PMIC_WRAP_SUPPORT
/****** For BringUp. if BringUp doesn't had PMIC, need open this ***********/
#if (PMIC_WRAP_PRELOADER)
#if CFG_FPGA_PLATFORM
#define PMIC_WRAP_NO_PMIC
#else
#undef PMIC_WRAP_NO_PMIC
/* #define PWRAP_TIMEOUT */
#endif
#elif (PMIC_WRAP_LK)
#if defined(MACH_FPGA)
#define PMIC_WRAP_NO_PMIC
#else
#undef PMIC_WRAP_NO_PMIC
/* #define PWRAP_TIMEOUT */
#endif
#elif (PMIC_WRAP_KERNEL)
#if defined(CONFIG_MTK_FPGA) || defined(CONFIG_FPGA_EARLY_PORTING)
#define PMIC_WRAP_NO_PMIC
#else
#undef PMIC_WRAP_NO_PMIC
/* #define PWRAP_TIMEOUT */
#endif
#elif (PMIC_WRAP_CTP)
#if defined(CONFIG_MTK_FPGA)
#define PMIC_WRAP_NO_PMIC
#else
#undef PMIC_WRAP_NO_PMIC
/* #define PWRAP_TIMEOUT */
#endif
#else
#undef PMIC_WRAP_NO_PMIC
/* #define PWRAP_TIMEOUT */
#endif

#define MTK_PLATFORM_MT6757 1
#define MTK_PLATFORM_MT6757_OE2 1

/******  SW ENV header define *****************************/
#if (PMIC_WRAP_PRELOADER)
#include <gpio.h>
#include <mt6757.h>
#include <sync_write.h>
#include <typedefs.h>
#elif (PMIC_WRAP_LK)
#include <debug.h>
#include <platform/mt_gpt.h>
#include <platform/mt_irq.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_typedefs.h>
#include <platform/sync_write.h>
#include <platform/upmu_hw.h>
#include <sys/types.h>
#elif (PMIC_WRAP_KERNEL)
#ifndef CONFIG_OF
#include <mach/mtk_irq.h>
#include <mach/mtk_reg_base.h>
#endif
#include "mt-plat/sync_write.h"
#elif (PMIC_WRAP_SCP)
#include "FreeRTOS.h"
#include "stdio.h"
#include <string.h>
#elif (PMIC_WRAP_CTP)
#include <reg_base.H>
#include <sync_write.h>
#include <typedefs.h>
#else
### Compile error, check SW ENV define
#endif

/*******************start ---external API********************************/
extern signed int pwrap_read(unsigned int adr, unsigned int *rdata);
extern signed int pwrap_write(unsigned int adr, unsigned int wdata);
extern signed int pwrap_write_nochk(unsigned int adr, unsigned int wdata);
extern signed int pwrap_read_nochk(unsigned int adr, unsigned int *rdata);
extern signed int pwrap_wacs2(unsigned int write, unsigned int adr,
			      unsigned int wdata, unsigned int *rdata);
extern signed int pwrap_wacs2_read(unsigned int adr, unsigned int *rdata);
extern signed int pwrap_wacs2_write(unsigned int adr, unsigned int wdata);
extern void pwrap_dump_all_register(void);
extern signed int pwrap_init_preloader(void);
extern signed int pwrap_init_lk(void);
extern signed int pwrap_init_scp(void);
extern signed int pwrap_init(void);

/******  DEBUG marco define *******************************/
#define PWRAPTAG "[PWRAP] "
#if (PMIC_WRAP_PRELOADER)
#ifdef PMIC_WRAP_DEBUG
#define PWRAPFUC(fmt, arg...) print(PWRAPTAG "%s\n", __func__)
#define PWRAPLOG(fmt, arg...) print(PWRAPTAG fmt, ##arg)
#else
#define PWRAPFUC(fmt, arg...)
#define PWRAPLOG(fmt, arg...)
#endif /* end of #ifdef PMIC_WRAP_DEBUG */
#define PWRAPERR(fmt, arg...)                                                  \
	print(PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#elif (PMIC_WRAP_LK)
#ifdef PMIC_WRAP_DEBUG
#define PWRAPFUC(fmt, arg...) dprintf(CRITICAL, PWRAPTAG "%s\n", __func__)
#define PWRAPLOG(fmt, arg...) dprintf(CRITICAL, PWRAPTAG fmt, ##arg)
#else
#define PWRAPFUC(fmt, arg...)
#define PWRAPLOG(fmt, arg...)
#endif /* end of #ifdef PMIC_WRAP_DEBUG */
#define PWRAPERR(fmt, arg...)                                                  \
	dprintf(CRITICAL, PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#elif (PMIC_WRAP_KERNEL)
#ifdef PMIC_WRAP_DEBUG
#define PWRAPDEB(fmt, arg...)                                                  \
	pr_debug(PWRAPTAG "cpuid=%d," fmt, raw_smp_processor_id(), ##arg)
#define PWRAPLOG(fmt, arg...) pr_debug(PWRAPTAG fmt, ##arg)
#define PWRAPFUC(fmt, arg...)                                                  \
	pr_debug(PWRAPTAG "cpuid=%d,%s\n", raw_smp_processor_id(), __func__)
#define PWRAPREG(fmt, arg...) pr_debug(PWRAPTAG fmt, ##arg)
#else
#define PWRAPDEB(fmt, arg...)
#define PWRAPLOG(fmt, arg...)
#define PWRAPFUC(fmt, arg...)
#define PWRAPREG(fmt, arg...)
#endif /* end of #ifdef PMIC_WRAP_DEBUG */
#define PWRAPERR(fmt, arg...)                                                  \
	pr_info(PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#elif (PMIC_WRAP_SCP)
#ifdef PMIC_WRAP_DEBUG
#define PWRAPFUC(fmt, arg...) PRINTF_D(PWRAPTAG "%s\n", __func__)
#define PWRAPLOG(fmt, arg...) PRINTF_D(PWRAPTAG fmt, ##arg)
#else
#define PWRAPFUC(fmt, arg...) /*PRINTF_D(PWRAPTAG "%s\n", __FUNCTION__)*/
#define PWRAPLOG(fmt, arg...) /*PRINTF_D(PWRAPTAG fmt, ##arg)*/
#endif			      /* end of #ifdef PMIC_WRAP_DEBUG */
#define PWRAPERR(fmt, arg...)                                                  \
	PRINTF_E(PWRAPTAG "ERROR, line=%d " fmt, __LINE__, ##arg)
#elif (PMIC_WRAP_CTP)
#ifdef PMIC_WRAP_DEBUG
#define PWRAPFUC(fmt, arg...) dbg_print(PWRAPTAG "%s\n", __func__)
#define PWRAPLOG(fmt, arg...) dbg_print(PWRAPTAG fmt, ##arg)
#else
#define PWRAPFUC(fmt, arg...) dbg_print(PWRAPTAG "%s\n", __func__)
#define PWRAPLOG(fmt, arg...) dbg_print(PWRAPTAG fmt, ##arg)
#endif /* end of #ifdef PMIC_WRAP_DEBUG */
#define PWRAPERR(fmt, arg...)                                                  \
	dbg_print(PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#else
## #Compile error, check SW ENV define
#endif
/**********************************************************/

/***********  platform info, PMIC info ********************/
#define PMIC_WRAP_REG_RANGE (354)

#define DEFAULT_VALUE_READ_TEST (0x5aa5)
#define PWRAP_WRITE_TEST_VALUE (0x1234)

#ifdef CONFIG_OF
extern void __iomem *pwrap_base;
#define PMIC_WRAP_BASE (pwrap_base)
#define MT_PMIC_WRAP_IRQ_ID (pwrap_irq)
#define INFRACFG_AO_REG_BASE (infracfg_ao_base)
#define TOPCKGEN_BASE (topckgen_base)
#define SCP_CLK_CTRL_BASE (scp_clk_ctrl_base)
#define PMIC_WRAP_P2P_BASE (pwrap_p2p_base)
#else
#define PMIC_WRAP_BASE (PWRAP_BASE)
#define MT_PMIC_WRAP_IRQ_ID (PMIC_WRAP_ERR_IRQ_BIT_ID)
#define INFRACFG_AO_BASE (0x10001000)
#define INFRACFG_AO_REG_BASE (INFRACFG_AO_BASE)
#define CKSYS_BASE (0x10000000)
#define TOPCKGEN_BASE (CKSYS_BASE)
#define SCP_CLK_CTRL_BASE (0x10059000)
#define PMIC_WRAP_P2P_BASE (0x1005E000)
#endif

#define UINT32 unsigned int
#define UINT32P unsigned int *

/**********************************************************/

#define ENABLE (1)
#define DISABLE (0)
#define DISABLE_ALL (0)

/* HIPRIS_ARB */
#define MDINF (1 << 0)
#define WACS0 (1 << 1)
#define WACS1 (1 << 2)
#define WACS2 (1 << 11)
#define DVFSINF (1 << 3)
#define STAUPD (1 << 5)
#define GPSINF (1 << 6)

/* MUX SEL */
#define WRAPPER_MODE (0)
#define MANUAL_MODE (1)

/* macro for MAN_RDATA  FSM */
#define MAN_FSM_NO_REQ (0x00)
#define MAN_FSM_IDLE (0x00)
#define MAN_FSM_REQ (0x02)
#define MAN_FSM_WFDLE (0x04) /* wait for dle,wait for read data done, */
#define MAN_FSM_WFVLDCLR (0x06)

/* macro for WACS_FSM */
#define WACS_FSM_IDLE (0x00)
#define WACS_FSM_REQ (0x02)   /* request in process */
#define WACS_FSM_WFDLE (0x04) /* wait for dle,wait for read data done, */
#define WACS_FSM_WFVLDCLR                                                      \
	(0x06) /* finish read data , wait for valid flag clearing */
#define WACS_INIT_DONE (0x01)
#define WACS_SYNC_IDLE (0x01)
#define WACS_SYNC_BUSY (0x00)

/**** timeout time, unit :us ***********/
#define TIMEOUT_RESET (0xFF)
#define TIMEOUT_READ (0xFF)
#define TIMEOUT_WAIT_IDLE (0xFF)

/*-----macro for manual commnd ---------------------------------*/
#define OP_WR (0x1)
#define OP_RD (0x0)
#define OP_CSH (0x0)
#define OP_CSL (0x1)
#define OP_CK (0x2)
#define OP_OUTS (0x8)
#define OP_OUTD (0x9)
#define OP_OUTQ (0xA)
#define OP_INS (0xC)
#define OP_INS0 (0xD)
#define OP_IND (0xE)
#define OP_INQ (0xF)
#define OP_OS2IS (0x10)
#define OP_OS2ID (0x11)
#define OP_OS2IQ (0x12)
#define OP_OD2IS (0x13)
#define OP_OD2ID (0x14)
#define OP_OD2IQ (0x15)
#define OP_OQ2IS (0x16)
#define OP_OQ2ID (0x17)
#define OP_OQ2IQ (0x18)
#define OP_OSNIS (0x19)
#define OP_ODNID (0x1A)

/******************Error handle *****************************/
#define E_PWR_INVALID_ARG (1)
#define E_PWR_INVALID_RW (2)
#define E_PWR_INVALID_ADDR (3)
#define E_PWR_INVALID_WDAT (4)
#define E_PWR_INVALID_OP_MANUAL (5)
#define E_PWR_NOT_IDLE_STATE (6)
#define E_PWR_NOT_INIT_DONE (7)
#define E_PWR_NOT_INIT_DONE_READ (8)
#define E_PWR_WAIT_IDLE_TIMEOUT (9)
#define E_PWR_WAIT_IDLE_TIMEOUT_READ (10)
#define E_PWR_INIT_SIDLY_FAIL (11)
#define E_PWR_RESET_TIMEOUT (12)
#define E_PWR_TIMEOUT (13)
#define E_PWR_INIT_RESET_SPI (20)
#define E_PWR_INIT_SIDLY (21)
#define E_PWR_INIT_REG_CLOCK (22)
#define E_PWR_INIT_ENABLE_PMIC (23)
#define E_PWR_INIT_DIO (24)
#define E_PWR_INIT_CIPHER (25)
#define E_PWR_INIT_WRITE_TEST (26)
#define E_PWR_INIT_ENABLE_CRC (27)
#define E_PWR_INIT_ENABLE_DEWRAP (28)
#define E_PWR_READ_TEST_FAIL (30)
#define E_PWR_WRITE_TEST_FAIL (31)
#define E_PWR_SWITCH_DIO (32)

/*-----macro for read/write register -------------------------------------*/
#define WRAP_RD32(addr) __raw_readl((void *)addr)
#define WRAP_WR32(addr, val) mt_reg_sync_writel((val), ((void *)addr))

#define WRAP_SET_BIT(BS, REG)                                                  \
	mt_reg_sync_writel((__raw_readl((void *)REG) | (u32)(BS)),             \
			   ((void *)REG))
#define WRAP_CLR_BIT(BS, REG)                                                  \
	mt_reg_sync_writel((__raw_readl((void *)REG) & (~(u32)(BS))),          \
			   ((void *)REG))

/**************** end ---external API***********************************/

/************* macro for spi clock config ******************************/
#if (MTK_PLATFORM_MT6757) || (MTK_PLATFORM_MT6757_OE2)
#define CLK_CFG_4_SET (TOPCKGEN_BASE + 0x084)
#define CLK_CFG_4_CLR (TOPCKGEN_BASE + 0x088)
#define CLK_CFG_5_CLR (TOPCKGEN_BASE + 0x098)

#define CLK_SPI_CK_26M 0x1
#define INFRA_GLOBALCON_RST0 (INFRACFG_AO_REG_BASE + 0x140)
#define INFRA_GLOBALCON_RST1 (INFRACFG_AO_REG_BASE + 0x144)
#define PMIC_CLOCK_DCM (INFRACFG_AO_REG_BASE + 0x074)
#define APB_CLOCK_GATING (INFRACFG_AO_REG_BASE + 0xF0C)
#endif /* end of MTK_PLATFORM_MT6757 */
#define MODULE_CLK_SEL (INFRACFG_AO_REG_BASE + 0x098)
#define MODULE_SW_CG_0_SET (INFRACFG_AO_REG_BASE + 0x080)
#define MODULE_SW_CG_0_CLR (INFRACFG_AO_REG_BASE + 0x084)
#define INFRA_GLOBALCON_RST2_SET (INFRACFG_AO_REG_BASE + 0x140)
#define INFRA_GLOBALCON_RST2_CLR (INFRACFG_AO_REG_BASE + 0x144)

/*****************************************************************/
#define PMIC_WRAP_MUX_SEL ((UINT32P)(PMIC_WRAP_BASE + 0x0))
#define PMIC_WRAP_WRAP_EN ((UINT32P)(PMIC_WRAP_BASE + 0x4))
#define PMIC_WRAP_DIO_EN ((UINT32P)(PMIC_WRAP_BASE + 0x8))
#define PMIC_WRAP_SIDLY ((UINT32P)(PMIC_WRAP_BASE + 0xC))
#define PMIC_WRAP_RDDMY ((UINT32P)(PMIC_WRAP_BASE + 0x10))
#define PMIC_WRAP_SI_CK_CON ((UINT32P)(PMIC_WRAP_BASE + 0x14))
#define PMIC_WRAP_CSHEXT_WRITE ((UINT32P)(PMIC_WRAP_BASE + 0x18))
#define PMIC_WRAP_CSHEXT_READ ((UINT32P)(PMIC_WRAP_BASE + 0x1C))
#define PMIC_WRAP_CSLEXT_START ((UINT32P)(PMIC_WRAP_BASE + 0x20))
#define PMIC_WRAP_CSLEXT_END ((UINT32P)(PMIC_WRAP_BASE + 0x24))
#define PMIC_WRAP_STAUPD_PRD ((UINT32P)(PMIC_WRAP_BASE + 0x28))
#define PMIC_WRAP_STAUPD_GRPEN ((UINT32P)(PMIC_WRAP_BASE + 0x2C))
#define PMIC_WRAP_EINT_STA0_ADR ((UINT32P)(PMIC_WRAP_BASE + 0x30))
#define PMIC_WRAP_EINT_STA1_ADR ((UINT32P)(PMIC_WRAP_BASE + 0x34))
#define PMIC_WRAP_EINT_STA ((UINT32P)(PMIC_WRAP_BASE + 0x38))
#define PMIC_WRAP_EINT_CLR ((UINT32P)(PMIC_WRAP_BASE + 0x3C))
#define PMIC_WRAP_STAUPD_MAN_TRIG ((UINT32P)(PMIC_WRAP_BASE + 0x40))
#define PMIC_WRAP_STAUPD_STA ((UINT32P)(PMIC_WRAP_BASE + 0x44))
#define PMIC_WRAP_WRAP_STA ((UINT32P)(PMIC_WRAP_BASE + 0x48))
#define PMIC_WRAP_HARB_INIT ((UINT32P)(PMIC_WRAP_BASE + 0x4C))
#define PMIC_WRAP_HARB_HPRIO ((UINT32P)(PMIC_WRAP_BASE + 0x50))
#define PMIC_WRAP_HIPRIO_ARB_EN ((UINT32P)(PMIC_WRAP_BASE + 0x54))
#define PMIC_WRAP_HARB_STA0 ((UINT32P)(PMIC_WRAP_BASE + 0x58))
#define PMIC_WRAP_HARB_STA1 ((UINT32P)(PMIC_WRAP_BASE + 0x5C))
#define PMIC_WRAP_MAN_EN ((UINT32P)(PMIC_WRAP_BASE + 0x60))
#define PMIC_WRAP_MAN_CMD ((UINT32P)(PMIC_WRAP_BASE + 0x64))
#define PMIC_WRAP_MAN_RDATA ((UINT32P)(PMIC_WRAP_BASE + 0x68))
#define PMIC_WRAP_MAN_VLDCLR ((UINT32P)(PMIC_WRAP_BASE + 0x6C))
#define PMIC_WRAP_WACS0_EN ((UINT32P)(PMIC_WRAP_BASE + 0x70))
#define PMIC_WRAP_INIT_DONE0 ((UINT32P)(PMIC_WRAP_BASE + 0x74))
#define PMIC_WRAP_WACS0_CMD ((UINT32P)(PMIC_WRAP_BASE + 0x78))
#define PMIC_WRAP_WACS0_RDATA ((UINT32P)(PMIC_WRAP_BASE + 0x7C))
#define PMIC_WRAP_WACS0_VLDCLR ((UINT32P)(PMIC_WRAP_BASE + 0x80))
#define PMIC_WRAP_WACS1_EN ((UINT32P)(PMIC_WRAP_BASE + 0x84))
#define PMIC_WRAP_INIT_DONE1 ((UINT32P)(PMIC_WRAP_BASE + 0x88))
#define PMIC_WRAP_WACS1_CMD ((UINT32P)(PMIC_WRAP_BASE + 0x8C))
#define PMIC_WRAP_WACS1_RDATA ((UINT32P)(PMIC_WRAP_BASE + 0x90))
#define PMIC_WRAP_WACS1_VLDCLR ((UINT32P)(PMIC_WRAP_BASE + 0x94))
#define PMIC_WRAP_WACS2_EN ((UINT32P)(PMIC_WRAP_BASE + 0x98))
#define PMIC_WRAP_INIT_DONE2 ((UINT32P)(PMIC_WRAP_BASE + 0x9C))
#define PMIC_WRAP_WACS2_CMD ((UINT32P)(PMIC_WRAP_BASE + 0xA0))
#define PMIC_WRAP_WACS2_RDATA ((UINT32P)(PMIC_WRAP_BASE + 0xA4))
#define PMIC_WRAP_WACS2_VLDCLR ((UINT32P)(PMIC_WRAP_BASE + 0xA8))
#define PMIC_WRAP_WACS3_EN ((UINT32P)(PMIC_WRAP_BASE + 0xAC))
#define PMIC_WRAP_INIT_DONE3 ((UINT32P)(PMIC_WRAP_BASE + 0xB0))
#define PMIC_WRAP_WACS3_CMD ((UINT32P)(PMIC_WRAP_BASE + 0xB4))
#define PMIC_WRAP_WACS3_RDATA ((UINT32P)(PMIC_WRAP_BASE + 0xB8))
#define PMIC_WRAP_WACS3_VLDCLR ((UINT32P)(PMIC_WRAP_BASE + 0xBC))
#define PMIC_WRAP_INT0_EN ((UINT32P)(PMIC_WRAP_BASE + 0xC0))
#define PMIC_WRAP_INT0_FLG_RAW ((UINT32P)(PMIC_WRAP_BASE + 0xC4))
#define PMIC_WRAP_INT0_FLG ((UINT32P)(PMIC_WRAP_BASE + 0xC8))
#define PMIC_WRAP_INT0_CLR ((UINT32P)(PMIC_WRAP_BASE + 0xCC))
#define PMIC_WRAP_INT1_EN ((UINT32P)(PMIC_WRAP_BASE + 0xD0))
#define PMIC_WRAP_INT1_FLG_RAW ((UINT32P)(PMIC_WRAP_BASE + 0xD4))
#define PMIC_WRAP_INT1_FLG ((UINT32P)(PMIC_WRAP_BASE + 0xD8))
#define PMIC_WRAP_INT1_CLR ((UINT32P)(PMIC_WRAP_BASE + 0xDC))
#define PMIC_WRAP_SIG_ADR ((UINT32P)(PMIC_WRAP_BASE + 0xE0))
#define PMIC_WRAP_SIG_MODE ((UINT32P)(PMIC_WRAP_BASE + 0xE4))
#define PMIC_WRAP_SIG_VALUE ((UINT32P)(PMIC_WRAP_BASE + 0xE8))
#define PMIC_WRAP_SIG_ERRVAL ((UINT32P)(PMIC_WRAP_BASE + 0xEC))
#define PMIC_WRAP_CRC_EN ((UINT32P)(PMIC_WRAP_BASE + 0xF0))
#define PMIC_WRAP_TIMER_EN ((UINT32P)(PMIC_WRAP_BASE + 0xF4))
#define PMIC_WRAP_TIMER_STA ((UINT32P)(PMIC_WRAP_BASE + 0xF8))
#define PMIC_WRAP_WDT_UNIT ((UINT32P)(PMIC_WRAP_BASE + 0xFC))
#define PMIC_WRAP_WDT_SRC_EN ((UINT32P)(PMIC_WRAP_BASE + 0x100))
#define PMIC_WRAP_WDT_FLG ((UINT32P)(PMIC_WRAP_BASE + 0x104))
#define PMIC_WRAP_DEBUG_INT_SEL ((UINT32P)(PMIC_WRAP_BASE + 0x108))
#define PMIC_WRAP_DVFS_ADR0 ((UINT32P)(PMIC_WRAP_BASE + 0x10C))
#define PMIC_WRAP_DVFS_WDATA0 ((UINT32P)(PMIC_WRAP_BASE + 0x110))
#define PMIC_WRAP_DVFS_ADR1 ((UINT32P)(PMIC_WRAP_BASE + 0x114))
#define PMIC_WRAP_DVFS_WDATA1 ((UINT32P)(PMIC_WRAP_BASE + 0x118))
#define PMIC_WRAP_DVFS_ADR2 ((UINT32P)(PMIC_WRAP_BASE + 0x11C))
#define PMIC_WRAP_DVFS_WDATA2 ((UINT32P)(PMIC_WRAP_BASE + 0x120))
#define PMIC_WRAP_DVFS_ADR3 ((UINT32P)(PMIC_WRAP_BASE + 0x124))
#define PMIC_WRAP_DVFS_WDATA3 ((UINT32P)(PMIC_WRAP_BASE + 0x128))
#define PMIC_WRAP_DVFS_ADR4 ((UINT32P)(PMIC_WRAP_BASE + 0x12C))
#define PMIC_WRAP_DVFS_WDATA4 ((UINT32P)(PMIC_WRAP_BASE + 0x130))
#define PMIC_WRAP_DVFS_ADR5 ((UINT32P)(PMIC_WRAP_BASE + 0x134))
#define PMIC_WRAP_DVFS_WDATA5 ((UINT32P)(PMIC_WRAP_BASE + 0x138))
#define PMIC_WRAP_DVFS_ADR6 ((UINT32P)(PMIC_WRAP_BASE + 0x13C))
#define PMIC_WRAP_DVFS_WDATA6 ((UINT32P)(PMIC_WRAP_BASE + 0x140))
#define PMIC_WRAP_DVFS_ADR7 ((UINT32P)(PMIC_WRAP_BASE + 0x144))
#define PMIC_WRAP_DVFS_WDATA7 ((UINT32P)(PMIC_WRAP_BASE + 0x148))
#define PMIC_WRAP_DVFS_ADR8 ((UINT32P)(PMIC_WRAP_BASE + 0x14C))
#define PMIC_WRAP_DVFS_WDATA8 ((UINT32P)(PMIC_WRAP_BASE + 0x150))
#define PMIC_WRAP_DVFS_ADR9 ((UINT32P)(PMIC_WRAP_BASE + 0x154))
#define PMIC_WRAP_DVFS_WDATA9 ((UINT32P)(PMIC_WRAP_BASE + 0x158))
#define PMIC_WRAP_DVFS_ADR10 ((UINT32P)(PMIC_WRAP_BASE + 0x15C))
#define PMIC_WRAP_DVFS_WDATA10 ((UINT32P)(PMIC_WRAP_BASE + 0x160))
#define PMIC_WRAP_DVFS_ADR11 ((UINT32P)(PMIC_WRAP_BASE + 0x164))
#define PMIC_WRAP_DVFS_WDATA11 ((UINT32P)(PMIC_WRAP_BASE + 0x168))
#define PMIC_WRAP_DVFS_ADR12 ((UINT32P)(PMIC_WRAP_BASE + 0x16C))
#define PMIC_WRAP_DVFS_WDATA12 ((UINT32P)(PMIC_WRAP_BASE + 0x170))
#define PMIC_WRAP_DVFS_ADR13 ((UINT32P)(PMIC_WRAP_BASE + 0x174))
#define PMIC_WRAP_DVFS_WDATA13 ((UINT32P)(PMIC_WRAP_BASE + 0x178))
#define PMIC_WRAP_DVFS_ADR14 ((UINT32P)(PMIC_WRAP_BASE + 0x17C))
#define PMIC_WRAP_DVFS_WDATA14 ((UINT32P)(PMIC_WRAP_BASE + 0x180))
#define PMIC_WRAP_DVFS_ADR15 ((UINT32P)(PMIC_WRAP_BASE + 0x184))
#define PMIC_WRAP_DVFS_WDATA15 ((UINT32P)(PMIC_WRAP_BASE + 0x188))
#define PMIC_WRAP_DCXO_ENABLE ((UINT32P)(PMIC_WRAP_BASE + 0x18C))
#define PMIC_WRAP_DCXO_CONN_ADR0 ((UINT32P)(PMIC_WRAP_BASE + 0x190))
#define PMIC_WRAP_DCXO_CONN_WDATA0 ((UINT32P)(PMIC_WRAP_BASE + 0x194))
#define PMIC_WRAP_DCXO_CONN_ADR1 ((UINT32P)(PMIC_WRAP_BASE + 0x198))
#define PMIC_WRAP_DCXO_CONN_WDATA1 ((UINT32P)(PMIC_WRAP_BASE + 0x19C))
#define PMIC_WRAP_DCXO_NFC_ADR0 ((UINT32P)(PMIC_WRAP_BASE + 0x1A0))
#define PMIC_WRAP_DCXO_NFC_WDATA0 ((UINT32P)(PMIC_WRAP_BASE + 0x1A4))
#define PMIC_WRAP_DCXO_NFC_ADR1 ((UINT32P)(PMIC_WRAP_BASE + 0x1A8))
#define PMIC_WRAP_DCXO_NFC_WDATA1 ((UINT32P)(PMIC_WRAP_BASE + 0x1AC))
#define PMIC_WRAP_SPMINF_STA ((UINT32P)(PMIC_WRAP_BASE + 0x1B0))
#define PMIC_WRAP_CIPHER_KEY_SEL ((UINT32P)(PMIC_WRAP_BASE + 0x1B4))
#define PMIC_WRAP_CIPHER_IV_SEL ((UINT32P)(PMIC_WRAP_BASE + 0x1B8))
#define PMIC_WRAP_CIPHER_EN ((UINT32P)(PMIC_WRAP_BASE + 0x1BC))
#define PMIC_WRAP_CIPHER_RDY ((UINT32P)(PMIC_WRAP_BASE + 0x1C0))
#define PMIC_WRAP_CIPHER_MODE ((UINT32P)(PMIC_WRAP_BASE + 0x1C4))
#define PMIC_WRAP_CIPHER_SWRST ((UINT32P)(PMIC_WRAP_BASE + 0x1C8))
#define PMIC_WRAP_DCM_EN ((UINT32P)(PMIC_WRAP_BASE + 0x1CC))
#define PMIC_WRAP_DCM_SPI_DBC_PRD ((UINT32P)(PMIC_WRAP_BASE + 0x1D0))
#define PMIC_WRAP_DCM_DBC_PRD ((UINT32P)(PMIC_WRAP_BASE + 0x1D4))
#define PMIC_WRAP_EXT_CK ((UINT32P)(PMIC_WRAP_BASE + 0x1D8))
#define PMIC_WRAP_ADC_CMD_ADDR ((UINT32P)(PMIC_WRAP_BASE + 0x1DC))
#define PMIC_WRAP_PWRAP_ADC_CMD ((UINT32P)(PMIC_WRAP_BASE + 0x1E0))
#define PMIC_WRAP_ADC_RDATA_ADDR ((UINT32P)(PMIC_WRAP_BASE + 0x1E4))
#define PMIC_WRAP_GPS_STA ((UINT32P)(PMIC_WRAP_BASE + 0x1E8))
#define PMIC_WRAP_SWRST ((UINT32P)(PMIC_WRAP_BASE + 0x1EC))
#define PMIC_WRAP_HARB_SLEEP_GATED_CTRL ((UINT32P)(PMIC_WRAP_BASE + 0x1F0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST ((UINT32P)(PMIC_WRAP_BASE + 0x1F4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_WP ((UINT32P)(PMIC_WRAP_BASE + 0x1F8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR0 ((UINT32P)(PMIC_WRAP_BASE + 0x1FC))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR1 ((UINT32P)(PMIC_WRAP_BASE + 0x200))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR2 ((UINT32P)(PMIC_WRAP_BASE + 0x204))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR3 ((UINT32P)(PMIC_WRAP_BASE + 0x208))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR4 ((UINT32P)(PMIC_WRAP_BASE + 0x20C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR5 ((UINT32P)(PMIC_WRAP_BASE + 0x210))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR6 ((UINT32P)(PMIC_WRAP_BASE + 0x214))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR7 ((UINT32P)(PMIC_WRAP_BASE + 0x218))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR8 ((UINT32P)(PMIC_WRAP_BASE + 0x21C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR9 ((UINT32P)(PMIC_WRAP_BASE + 0x220))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR10 ((UINT32P)(PMIC_WRAP_BASE + 0x224))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR11 ((UINT32P)(PMIC_WRAP_BASE + 0x228))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR12 ((UINT32P)(PMIC_WRAP_BASE + 0x22C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR13 ((UINT32P)(PMIC_WRAP_BASE + 0x230))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR14 ((UINT32P)(PMIC_WRAP_BASE + 0x234))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR15 ((UINT32P)(PMIC_WRAP_BASE + 0x238))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR16 ((UINT32P)(PMIC_WRAP_BASE + 0x23C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR17 ((UINT32P)(PMIC_WRAP_BASE + 0x240))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR18 ((UINT32P)(PMIC_WRAP_BASE + 0x244))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR19 ((UINT32P)(PMIC_WRAP_BASE + 0x248))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR20 ((UINT32P)(PMIC_WRAP_BASE + 0x24C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR21 ((UINT32P)(PMIC_WRAP_BASE + 0x250))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR22 ((UINT32P)(PMIC_WRAP_BASE + 0x254))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR23 ((UINT32P)(PMIC_WRAP_BASE + 0x258))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR24 ((UINT32P)(PMIC_WRAP_BASE + 0x25C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR25 ((UINT32P)(PMIC_WRAP_BASE + 0x260))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR26 ((UINT32P)(PMIC_WRAP_BASE + 0x264))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR27 ((UINT32P)(PMIC_WRAP_BASE + 0x268))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR28 ((UINT32P)(PMIC_WRAP_BASE + 0x26C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR29 ((UINT32P)(PMIC_WRAP_BASE + 0x270))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR30 ((UINT32P)(PMIC_WRAP_BASE + 0x274))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR31 ((UINT32P)(PMIC_WRAP_BASE + 0x278))
#define PMIC_WRAP_MD_ADC_MODE_SEL ((UINT32P)(PMIC_WRAP_BASE + 0x27C))
#define PMIC_WRAP_MD_ADC_STA0 ((UINT32P)(PMIC_WRAP_BASE + 0x280))
#define PMIC_WRAP_MD_ADC_STA1 ((UINT32P)(PMIC_WRAP_BASE + 0x284))
#define PMIC_WRAP_PRIORITY_USER_SEL_0 ((UINT32P)(PMIC_WRAP_BASE + 0x288))
#define PMIC_WRAP_PRIORITY_USER_SEL_1 ((UINT32P)(PMIC_WRAP_BASE + 0x28C))
#define PMIC_WRAP_ARBITER_OUT_SEL_0 ((UINT32P)(PMIC_WRAP_BASE + 0x290))
#define PMIC_WRAP_ARBITER_OUT_SEL_1 ((UINT32P)(PMIC_WRAP_BASE + 0x294))
#define PMIC_WRAP_STARV_COUNTER_0 ((UINT32P)(PMIC_WRAP_BASE + 0x298))
#define PMIC_WRAP_STARV_COUNTER_1 ((UINT32P)(PMIC_WRAP_BASE + 0x29C))
#define PMIC_WRAP_STARV_COUNTER_2 ((UINT32P)(PMIC_WRAP_BASE + 0x2A0))
#define PMIC_WRAP_STARV_COUNTER_3 ((UINT32P)(PMIC_WRAP_BASE + 0x2A4))
#define PMIC_WRAP_STARV_COUNTER_4 ((UINT32P)(PMIC_WRAP_BASE + 0x2A8))
#define PMIC_WRAP_STARV_COUNTER_5 ((UINT32P)(PMIC_WRAP_BASE + 0x2AC))
#define PMIC_WRAP_STARV_COUNTER_6 ((UINT32P)(PMIC_WRAP_BASE + 0x2B0))
#define PMIC_WRAP_STARV_COUNTER_7 ((UINT32P)(PMIC_WRAP_BASE + 0x2B4))
#define PMIC_WRAP_STARV_COUNTER_8 ((UINT32P)(PMIC_WRAP_BASE + 0x2B8))
#define PMIC_WRAP_STARV_COUNTER_9 ((UINT32P)(PMIC_WRAP_BASE + 0x2BC))
#define PMIC_WRAP_STARV_COUNTER_10 ((UINT32P)(PMIC_WRAP_BASE + 0x2C0))
#define PMIC_WRAP_STARV_COUNTER_11 ((UINT32P)(PMIC_WRAP_BASE + 0x2C4))
#define PMIC_WRAP_STARV_COUNTER_12 ((UINT32P)(PMIC_WRAP_BASE + 0x2C8))
#define PMIC_WRAP_STARV_COUNTER_13 ((UINT32P)(PMIC_WRAP_BASE + 0x2CC))
#define PMIC_WRAP_STARV_COUNTER_14 ((UINT32P)(PMIC_WRAP_BASE + 0x2D0))
#define PMIC_WRAP_STARV_COUNTER_15 ((UINT32P)(PMIC_WRAP_BASE + 0x2D4))
#define PMIC_WRAP_STARV_COUNTER_0_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2D8))
#define PMIC_WRAP_STARV_COUNTER_1_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2DC))
#define PMIC_WRAP_STARV_COUNTER_2_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2E0))
#define PMIC_WRAP_STARV_COUNTER_3_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2E4))
#define PMIC_WRAP_STARV_COUNTER_4_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2E8))
#define PMIC_WRAP_STARV_COUNTER_5_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2EC))
#define PMIC_WRAP_STARV_COUNTER_6_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2F0))
#define PMIC_WRAP_STARV_COUNTER_7_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2F4))
#define PMIC_WRAP_STARV_COUNTER_8_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2F8))
#define PMIC_WRAP_STARV_COUNTER_9_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x2FC))
#define PMIC_WRAP_STARV_COUNTER_10_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x300))
#define PMIC_WRAP_STARV_COUNTER_11_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x304))
#define PMIC_WRAP_STARV_COUNTER_12_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x308))
#define PMIC_WRAP_STARV_COUNTER_13_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x30C))
#define PMIC_WRAP_STARV_COUNTER_14_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x310))
#define PMIC_WRAP_STARV_COUNTER_15_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x314))
#define PMIC_WRAP_STARV_COUNTER_CLR ((UINT32P)(PMIC_WRAP_BASE + 0x318))
#define PMIC_WRAP_STARV_PRIO_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x31C))
#define PMIC_WRAP_DEBUG_STATUS ((UINT32P)(PMIC_WRAP_BASE + 0x320))
#define PMIC_WRAP_DEBUG_SQUENCE_0 ((UINT32P)(PMIC_WRAP_BASE + 0x324))
#define PMIC_WRAP_DEBUG_SQUENCE_1 ((UINT32P)(PMIC_WRAP_BASE + 0x328))
#define PMIC_WRAP_DEBUG_SQUENCE_2 ((UINT32P)(PMIC_WRAP_BASE + 0x32C))
#define PMIC_WRAP_DEBUG_SQUENCE_3 ((UINT32P)(PMIC_WRAP_BASE + 0x330))
#define PMIC_WRAP_DEBUG_SQUENCE_4 ((UINT32P)(PMIC_WRAP_BASE + 0x334))
#define PMIC_WRAP_DEBUG_SQUENCE_5 ((UINT32P)(PMIC_WRAP_BASE + 0x338))
#define PMIC_WRAP_DEBUG_SQUENCE_6 ((UINT32P)(PMIC_WRAP_BASE + 0x33C))
#define PMIC_WRAP_DEBUG_SW_DRIVER_0 ((UINT32P)(PMIC_WRAP_BASE + 0x340))
#define PMIC_WRAP_DEBUG_SW_DRIVER_1 ((UINT32P)(PMIC_WRAP_BASE + 0x344))
#define PMIC_WRAP_DEBUG_SW_DRIVER_2 ((UINT32P)(PMIC_WRAP_BASE + 0x348))
#define PMIC_WRAP_DEBUG_SW_DRIVER_3 ((UINT32P)(PMIC_WRAP_BASE + 0x34C))
#define PMIC_WRAP_DEBUG_SW_DRIVER_4 ((UINT32P)(PMIC_WRAP_BASE + 0x350))
#define PMIC_WRAP_DEBUG_SW_DRIVER_5 ((UINT32P)(PMIC_WRAP_BASE + 0x354))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST_1 ((UINT32P)(PMIC_WRAP_BASE + 0x358))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_WP_1 ((UINT32P)(PMIC_WRAP_BASE + 0x35C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR0_1 ((UINT32P)(PMIC_WRAP_BASE + 0x360))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR1_1 ((UINT32P)(PMIC_WRAP_BASE + 0x364))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR2_1 ((UINT32P)(PMIC_WRAP_BASE + 0x368))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR3_1 ((UINT32P)(PMIC_WRAP_BASE + 0x36C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR4_1 ((UINT32P)(PMIC_WRAP_BASE + 0x370))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR5_1 ((UINT32P)(PMIC_WRAP_BASE + 0x374))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR6_1 ((UINT32P)(PMIC_WRAP_BASE + 0x378))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR7_1 ((UINT32P)(PMIC_WRAP_BASE + 0x37C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR8_1 ((UINT32P)(PMIC_WRAP_BASE + 0x380))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR9_1 ((UINT32P)(PMIC_WRAP_BASE + 0x384))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR10_1 ((UINT32P)(PMIC_WRAP_BASE + 0x388))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR11_1 ((UINT32P)(PMIC_WRAP_BASE + 0x38C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR12_1 ((UINT32P)(PMIC_WRAP_BASE + 0x390))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR13_1 ((UINT32P)(PMIC_WRAP_BASE + 0x394))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR14_1 ((UINT32P)(PMIC_WRAP_BASE + 0x398))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR15_1 ((UINT32P)(PMIC_WRAP_BASE + 0x39C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR16_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3A0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR17_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3A4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR18_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3A8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR19_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3AC))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR20_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3B0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR21_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3B4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR22_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3B8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR23_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3BC))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR24_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3C0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR25_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3C4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR26_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3C8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR27_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3CC))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR28_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3D0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR29_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3D4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR30_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3D8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR31_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3DC))
#define PMIC_WRAP_MD_ADC_MODE_SEL_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3E0))
#define PMIC_WRAP_MD_ADC_STA0_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3E4))
#define PMIC_WRAP_MD_ADC_STA1_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3E8))
#define PMIC_WRAP_ADC_CMD_ADDR_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3EC))
#define PMIC_WRAP_PWRAP_ADC_CMD_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3F0))
#define PMIC_WRAP_ADC_RDATA_ADDR_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3F4))
#define PMIC_WRAP_GPS_STA_1 ((UINT32P)(PMIC_WRAP_BASE + 0x3F8))
#define PMIC_WRAP_HARB_STA2 ((UINT32P)(PMIC_WRAP_BASE + 0x3FC))

#define GET_STAUPD_DLE_CNT(x) ((x >> 0) & 0x00000007)
#define GET_STAUPD_ALE_CNT(x) ((x >> 3) & 0x00000007)
#define GET_STAUPD_FSM(x) ((x >> 6) & 0x00000007)
#define GET_STAUPD_DLE_CNT_3(x) ((x >> 9) & 0x00000001)
#define GET_STAUPD_ALE_CNT_3(x) ((x >> 10) & 0x00000001)
#define GET_WRAP_CH_DLE_RESTCNT(x) ((x >> 0) & 0x00000007)
#define GET_WRAP_CH_ALE_RESTCNT(x) ((x >> 3) & 0x00000003)
#define GET_WRAP_AG_DLE_RESTCNT(x) ((x >> 5) & 0x00000003)
#define GET_WRAP_CH_W(x) ((x >> 7) & 0x00000001)
#define GET_WRAP_CH_REQ(x) ((x >> 8) & 0x00000001)
#define GET_AG_WRAP_W(x) ((x >> 9) & 0x00000001)
#define GET_AG_WRAP_REQ(x) ((x >> 10) & 0x00000001)
#define GET_WRAP_FSM(x) ((x >> 11) & 0x0000000f)
#define GET_HARB_WRAP_WDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_HARB_WRAP_ADR(x) ((x >> 16) & 0x00007fff)
#define GET_HARB_WRAP_REQ(x) ((x >> 31) & 0x00000001)
#define GET_HARB_DLE_EMPTY(x) ((x >> 0) & 0x00000001)
#define GET_HARB_DLE_FULL(x) ((x >> 1) & 0x00000001)
#define GET_HARB_VLD(x) ((x >> 2) & 0x00000001)
#define GET_HARB_DLE_OWN(x) ((x >> 3) & 0x0000000f)
#define GET_HARB_OWN(x) ((x >> 7) & 0x0000000f)
#define GET_HARB_DLE_RESTCNT(x) ((x >> 11) & 0x0000000f)
#define GET_AG_HARB_REQ(x) ((x >> 15) & 0x00003fff)
#define GET_HARB_WRAP_W(x) ((x >> 29) & 0x00000001)
#define GET_HARB_WRAP_REQ0(x) ((x >> 30) & 0x00000001)
#define GET_SPI_WDATA(x) ((x >> 0) & 0x000000ff)
#define GET_SPI_OP(x) ((x >> 8) & 0x0000001f)
#define GET_SPI_W(x) ((x >> 13) & 0x00000001)
#define GET_MAN_RDATA(x) ((x >> 0) & 0x000000ff)
#define GET_MAN_FSM(x) ((x >> 8) & 0x00000007)
#define GET_MAN_REQ(x) ((x >> 11) & 0x00000001)
#define GET_WACS0_WDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS0_ADR(x) ((x >> 16) & 0x00007fff)
#define GET_WACS0_WRITE(x) ((x >> 31) & 0x00000001)
#define GET_WACS0_RDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS0_FSM(x) ((x >> 16) & 0x00000007)
#define GET_WACS0_REQ(x) ((x >> 19) & 0x00000001)
#define GET_SYNC_IDLE0(x) ((x >> 20) & 0x00000001)
#define GET_INIT_DONE0(x) ((x >> 21) & 0x00000001)
#define GET_SYS_IDLE0(x) ((x >> 22) & 0x00000001)
#define GET_WACS0_FIFO_FILLCNT(x) ((x >> 24) & 0x0000000f)
#define GET_WACS0_FIFO_FREECNT(x) ((x >> 28) & 0x0000000f)
#define GET_WACS1_WDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS1_ADR(x) ((x >> 16) & 0x00007fff)
#define GET_WACS1_WRITE(x) ((x >> 31) & 0x00000001)
#define GET_WACS1_RDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS1_FSM(x) ((x >> 16) & 0x00000007)
#define GET_WACS1_REQ(x) ((x >> 19) & 0x00000001)
#define GET_SYNC_IDLE1(x) ((x >> 20) & 0x00000001)
#define GET_INIT_DONE1(x) ((x >> 21) & 0x00000001)
#define GET_SYS_IDLE1(x) ((x >> 22) & 0x00000001)
#define GET_WACS1_FIFO_FILLCNT(x) ((x >> 24) & 0x0000000f)
#define GET_WACS1_FIFO_FREECNT(x) ((x >> 28) & 0x0000000f)
#define GET_WACS2_WDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS2_ADR(x) ((x >> 16) & 0x00007fff)
#define GET_WACS2_WRITE(x) ((x >> 31) & 0x00000001)
#define GET_WACS2_RDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS2_FSM(x) ((x >> 16) & 0x00000007)
#define GET_WACS2_REQ(x) ((x >> 19) & 0x00000001)
#define GET_SYNC_IDLE2(x) ((x >> 20) & 0x00000001)
#define GET_INIT_DONE2(x) ((x >> 21) & 0x00000001)
#define GET_SYS_IDLE2(x) ((x >> 22) & 0x00000001)
#define GET_WACS2_FIFO_FILLCNT(x) ((x >> 24) & 0x0000000f)
#define GET_WACS2_FIFO_FREECNT(x) ((x >> 28) & 0x0000000f)
#define GET_WACS3_WDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS3_ADR(x) ((x >> 16) & 0x00007fff)
#define GET_WACS3_WRITE(x) ((x >> 31) & 0x00000001)
#define GET_WACS3_RDATA(x) ((x >> 0) & 0x0000ffff)
#define GET_WACS3_FSM(x) ((x >> 16) & 0x00000007)
#define GET_WACS3_REQ(x) ((x >> 19) & 0x00000001)
#define GET_SYNC_IDLE3(x) ((x >> 20) & 0x00000001)
#define GET_INIT_DONE3(x) ((x >> 21) & 0x00000001)
#define GET_SYS_IDLE3(x) ((x >> 22) & 0x00000001)
#define GET_WACS3_FIFO_FILLCNT(x) ((x >> 24) & 0x0000000f)
#define GET_WACS3_FIFO_FREECNT(x) ((x >> 28) & 0x0000000f)
#define GET_PWRAP_GPS_ACK(x) ((x >> 0) & 0x00000001)
#define GET_GPS_PWRAP_REQ(x) ((x >> 1) & 0x00000001)
#define GET_GPSINF_DLE_CNT(x) ((x >> 4) & 0x00000003)
#define GET_GPSINF_ALE_CNT(x) ((x >> 6) & 0x00000003)
#define GET_GPS_INF_FSM(x) ((x >> 8) & 0x00000007)
#define GET_PWRAP_GPS_WDATA(x) ((x >> 17) & 0x00007fff)
#define GET_PWRAP_MD_ADC_TEMP_DATA(x) ((x >> 16) & 0x0000ffff)
#define GET_PWRAP_MD_ADC_TEMP_DATA_1(x) ((x >> 16) & 0x0000ffff)
#define GET_PWRAP_GPS_ACK_1(x) ((x >> 0) & 0x00000001)
#define GET_GPS_PWRAP_REQ_1(x) ((x >> 1) & 0x00000001)
#define GET_GPSINF_DLE_CNT_1(x) ((x >> 4) & 0x00000003)
#define GET_GPSINF_ALE_CNT_1(x) ((x >> 6) & 0x00000003)
#define GET_GPS_INF_FSM_1(x) ((x >> 8) & 0x00000007)
#define GET_PWRAP_GPS_WDATA_1(x) ((x >> 17) & 0x00007fff)
#define GET_AG_HARB_REQ_14_15(x) ((x >> 0) & 0x00000003)
/*******************macro for  regsister@PMIC *******************************/
#if (PMIC_WRAP_KERNEL)
#include <mach/upmu_hw.h>
#else
#include <upmu_hw.h>
#endif
#endif

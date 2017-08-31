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

#ifndef __PMIC_WRAP_REGS_H__
#define __PMIC_WRAP_REGS_H__
#ifndef CONFIG_OF
#include <mach/mtk_reg_base.h>
#include <mach/mtk_irq.h>
#endif
#include "mt-plat/sync_write.h"


#define PMIC_WRAP_DEBUG
#define PWRAPTAG                "[PWRAP] "
#ifdef PMIC_WRAP_DEBUG
#define PWRAPDEB(fmt, arg...)     printk(PWRAPTAG "cpuid=%d," fmt, raw_smp_processor_id(), ##arg)
#define PWRAPFUC(fmt, arg...)     printk(PWRAPTAG "cpuid=%d,%s\n", raw_smp_processor_id(), __func__)
#endif
#define PWRAPLOG(fmt, arg...)   printk(PWRAPTAG fmt, ##arg)
#define PWRAPERR(fmt, arg...)   printk(PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#define PWRAPREG(fmt, arg...)   printk(PWRAPTAG fmt, ##arg)

/*if BringUp doesn't had PMIC, need open this */
/* #define PMIC_WRAP_NO_PMIC */

/******************** Everest platform info, PMIC info ************/
#define SLV_6351
#define	PMIC_CHIP_VERSION                       "MT6351"
#define PMIC_WRAP_REG_RANGE                     207
#define MT6351_DEFAULT_VALUE_READ_TEST          0x5aa5
#define MT6351_WRITE_TEST_VALUE                 0x1234

#ifdef CONFIG_OF
extern void __iomem *pwrap_base;
#define PMIC_WRAP_BASE		(pwrap_base)
#define MT_PMIC_WRAP_IRQ_ID	(pwrap_irq)
#define INFRACFG_AO_REG_BASE	(infracfg_ao_base)
#define TOPCKGEN_BASE		(topckgen_base)
#define SCP_CLK_CTRL_BASE       (scp_clk_ctrl_base)
#define PMIC_WRAP_P2P_BASE      (pwrap_p2p_base)
#else
#define PMIC_WRAP_BASE          (PWRAP_BASE)
#define MT_PMIC_WRAP_IRQ_ID     (PMIC_WRAP_ERR_IRQ_BIT_ID)
#define INFRACFG_AO_BASE        (0x10001000)
#define INFRACFG_AO_REG_BASE    (INFRACFG_AO_BASE)
#define CKSYS_BASE              (0x10000000)
#define TOPCKGEN_BASE           (CKSYS_BASE)
#define SCP_CLK_CTRL_BASE       (0x10059000)
#define PMIC_WRAP_P2P_BASE      (0x1005E000)
#endif
/******************************************************/

#define ENABLE	1
#define DISABLE 0
#define DISABLE_ALL 0

/* HIPRIS_ARB */
#define MDINF		(1 << 0)
#define WACS0		(1 << 1)
#define WACS1		(1 << 2)
#define WACS2		(1 << 11)
#define DVFSINF		(1 << 3)
#define STAUPD		(1 << 5)
#define GPSINF		(1 << 6)

/* MUX SEL */
#define	WRAPPER_MODE	0
#define	MANUAL_MODE		1


/* macro for MAN_RDATA  FSM */
#define MAN_FSM_NO_REQ             (0x00)
#define MAN_FSM_IDLE               (0x00)
#define MAN_FSM_REQ                (0x02)
#define MAN_FSM_WFDLE              (0x04) /* wait for dle,wait for read data done, */
#define MAN_FSM_WFVLDCLR           (0x06)

/* macro for WACS_FSM */
#define WACS_FSM_IDLE               (0x00)
#define WACS_FSM_REQ                (0x02)
#define WACS_FSM_WFDLE              (0x04) /* wait for dle,wait for read data done, */
#define WACS_FSM_WFVLDCLR           (0x06) /* finish read data , wait for valid flag clearing */
#define WACS_INIT_DONE              (0x01)
#define WACS_SYNC_IDLE              (0x01)
#define WACS_SYNC_BUSY              (0x00)

/**** timeout time, unit :us ***********/
#define TIMEOUT_RESET	        0xFF
#define TIMEOUT_READ	        0xFF
#define TIMEOUT_WAIT_IDLE       0xFF

/*-----macro for manual commnd ---------------------------------*/
#define OP_WR    (0x1)
#define OP_RD    (0x0)
#define OP_CSH   (0x0)
#define OP_CSL   (0x1)
#define OP_CK    (0x2)
#define OP_OUTS  (0x8)
#define OP_OUTD  (0x9)
#define OP_OUTQ  (0xA)
#define OP_INS   (0xC)
#define OP_INS0  (0xD)
#define OP_IND   (0xE)
#define OP_INQ   (0xF)
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
#define E_PWR_INVALID_ARG               1
#define E_PWR_INVALID_RW                2
#define E_PWR_INVALID_ADDR              3
#define E_PWR_INVALID_WDAT              4
#define E_PWR_INVALID_OP_MANUAL         5
#define E_PWR_NOT_IDLE_STATE            6
#define E_PWR_NOT_INIT_DONE             7
#define E_PWR_NOT_INIT_DONE_READ        8
#define E_PWR_WAIT_IDLE_TIMEOUT         9
#define E_PWR_WAIT_IDLE_TIMEOUT_READ    10
#define E_PWR_INIT_SIDLY_FAIL           11
#define E_PWR_RESET_TIMEOUT             12
#define E_PWR_TIMEOUT                   13
#define E_PWR_INIT_RESET_SPI            20
#define E_PWR_INIT_SIDLY                21
#define E_PWR_INIT_REG_CLOCK            22
#define E_PWR_INIT_ENABLE_PMIC          23
#define E_PWR_INIT_DIO                  24
#define E_PWR_INIT_CIPHER               25
#define E_PWR_INIT_WRITE_TEST           26
#define E_PWR_INIT_ENABLE_CRC           27
#define E_PWR_INIT_ENABLE_DEWRAP        28
#define E_PWR_READ_TEST_FAIL            30
#define E_PWR_WRITE_TEST_FAIL           31
#define E_PWR_SWITCH_DIO                32


/*-----macro for read/write register -------------------------------------*/
#define WRAP_RD32(addr)            __raw_readl((void *)addr)
#define WRAP_WR32(addr, val)        mt_reg_sync_writel((val), ((void *)addr))

#define WRAP_SET_BIT(BS, REG)       mt_reg_sync_writel((__raw_readl((void *)REG) | (u32)(BS)), ((void *)REG))
#define WRAP_CLR_BIT(BS, REG)       mt_reg_sync_writel((__raw_readl((void *)REG) & (~(u32)(BS))), ((void *)REG))


/*******************start ---external API********************************/
s32 pwrap_write_nochk(u32 adr, u32 wdata);
s32 pwrap_read_nochk(u32 adr, u32 *rdata);

s32 pwrap_read(u32  adr, u32 *rdata);
s32 pwrap_write(u32  adr, u32  wdata);
s32 pwrap_wacs2(u32  write, u32  adr, u32  wdata, u32 *rdata);
s32 pwrap_init(void);
s32 pwrap_init_lk(void);

/**************** end ---external API***********************************/

/************* macro for spi clock config ******************************/
#define CLK_CFG_4_CLR						(TOPCKGEN_BASE+0x088)
#define CLK_CFG_5_STA						(TOPCKGEN_BASE+0x090)
#define CLK_CFG_5_CLR						(TOPCKGEN_BASE+0x098)
#define CLK_SPI_CK_26M						0x1
#define MODULE_SW_CG_0_SET					(INFRACFG_AO_REG_BASE+0x080)
#define MODULE_SW_CG_0_CLR					(INFRACFG_AO_REG_BASE+0x084)
#define MODULE_SW_CG_0_STA					(INFRACFG_AO_REG_BASE+0x090)
#define INFRA_GLOBALCON_RST0                (INFRACFG_AO_REG_BASE+0x140)
#define INFRA_GLOBALCON_RST1                (INFRACFG_AO_REG_BASE+0x144)
/*****************************************************************/
/* For Init CM4 PMIC Wrap P2P */
#define PMIC_WRAP_WACS_P2P_EN			((unsigned int *)(0x100AB000 + 0x200))
#define PMIC_WRAP_INIT_DONE_P2P			((unsigned int *)(0x100AB000 + 0x204))

#define PMIC_WRAP_MUX_SEL               ((unsigned int *)(PMIC_WRAP_BASE+0x0))
#define PMIC_WRAP_WRAP_EN               ((unsigned int *)(PMIC_WRAP_BASE+0x4))
#define PMIC_WRAP_DIO_EN                ((unsigned int *)(PMIC_WRAP_BASE+0x8))
#define PMIC_WRAP_SIDLY                 ((unsigned int *)(PMIC_WRAP_BASE+0xC))
#define PMIC_WRAP_RDDMY                 ((unsigned int *)(PMIC_WRAP_BASE+0x10))
#define PMIC_WRAP_SI_CK_CON             ((unsigned int *)(PMIC_WRAP_BASE+0x14))
#define PMIC_WRAP_CSHEXT_WRITE          ((unsigned int *)(PMIC_WRAP_BASE+0x18))
#define PMIC_WRAP_CSHEXT_READ           ((unsigned int *)(PMIC_WRAP_BASE+0x1C))
#define PMIC_WRAP_CSLEXT_START          ((unsigned int *)(PMIC_WRAP_BASE+0x20))
#define PMIC_WRAP_CSLEXT_END            ((unsigned int *)(PMIC_WRAP_BASE+0x24))
#define PMIC_WRAP_STAUPD_PRD            ((unsigned int *)(PMIC_WRAP_BASE+0x28))
#define PMIC_WRAP_STAUPD_GRPEN          ((unsigned int *)(PMIC_WRAP_BASE+0x2C))
#define PMIC_WRAP_EINT_STA0_ADR         ((unsigned int *)(PMIC_WRAP_BASE+0x30))
#define PMIC_WRAP_EINT_STA1_ADR         ((unsigned int *)(PMIC_WRAP_BASE+0x34))
#define PMIC_WRAP_EINT_STA              ((unsigned int *)(PMIC_WRAP_BASE+0x38))
#define PMIC_WRAP_EINT_CLR              ((unsigned int *)(PMIC_WRAP_BASE+0x3C))
#define PMIC_WRAP_STAUPD_MAN_TRIG       ((unsigned int *)(PMIC_WRAP_BASE+0x40))
#define PMIC_WRAP_STAUPD_STA            ((unsigned int *)(PMIC_WRAP_BASE+0x44))
#define PMIC_WRAP_WRAP_STA              ((unsigned int *)(PMIC_WRAP_BASE+0x48))
#define PMIC_WRAP_HARB_INIT             ((unsigned int *)(PMIC_WRAP_BASE+0x4C))
#define PMIC_WRAP_HARB_HPRIO            ((unsigned int *)(PMIC_WRAP_BASE+0x50))
#define PMIC_WRAP_HIPRIO_ARB_EN         ((unsigned int *)(PMIC_WRAP_BASE+0x54))
#define PMIC_WRAP_HARB_STA0             ((unsigned int *)(PMIC_WRAP_BASE+0x58))
#define PMIC_WRAP_HARB_STA1             ((unsigned int *)(PMIC_WRAP_BASE+0x5C))
#define PMIC_WRAP_MAN_EN                ((unsigned int *)(PMIC_WRAP_BASE+0x60))
#define PMIC_WRAP_MAN_CMD               ((unsigned int *)(PMIC_WRAP_BASE+0x64))
#define PMIC_WRAP_MAN_RDATA             ((unsigned int *)(PMIC_WRAP_BASE+0x68))
#define PMIC_WRAP_MAN_VLDCLR            ((unsigned int *)(PMIC_WRAP_BASE+0x6C))
#define PMIC_WRAP_WACS0_EN              ((unsigned int *)(PMIC_WRAP_BASE+0x70))
#define PMIC_WRAP_INIT_DONE0            ((unsigned int *)(PMIC_WRAP_BASE+0x74))
#define PMIC_WRAP_WACS0_CMD             ((unsigned int *)(PMIC_WRAP_BASE+0x78))
#define PMIC_WRAP_WACS0_RDATA           ((unsigned int *)(PMIC_WRAP_BASE+0x7C))
#define PMIC_WRAP_WACS0_VLDCLR          ((unsigned int *)(PMIC_WRAP_BASE+0x80))
#define PMIC_WRAP_WACS1_EN              ((unsigned int *)(PMIC_WRAP_BASE+0x84))
#define PMIC_WRAP_INIT_DONE1            ((unsigned int *)(PMIC_WRAP_BASE+0x88))
#define PMIC_WRAP_WACS1_CMD             ((unsigned int *)(PMIC_WRAP_BASE+0x8C))
#define PMIC_WRAP_WACS1_RDATA           ((unsigned int *)(PMIC_WRAP_BASE+0x90))
#define PMIC_WRAP_WACS1_VLDCLR          ((unsigned int *)(PMIC_WRAP_BASE+0x94))
#define PMIC_WRAP_WACS2_EN              ((unsigned int *)(PMIC_WRAP_BASE+0x98))
#define PMIC_WRAP_INIT_DONE2            ((unsigned int *)(PMIC_WRAP_BASE+0x9C))
#define PMIC_WRAP_WACS2_CMD             ((unsigned int *)(PMIC_WRAP_BASE+0xA0))
#define PMIC_WRAP_WACS2_RDATA           ((unsigned int *)(PMIC_WRAP_BASE+0xA4))
#define PMIC_WRAP_WACS2_VLDCLR          ((unsigned int *)(PMIC_WRAP_BASE+0xA8))
#define PMIC_WRAP_WACS3_EN              ((unsigned int *)(PMIC_WRAP_BASE+0xAC))
#define PMIC_WRAP_INIT_DONE3            ((unsigned int *)(PMIC_WRAP_BASE+0xB0))
#define PMIC_WRAP_WACS3_CMD             ((unsigned int *)(PMIC_WRAP_BASE+0xB4))
#define PMIC_WRAP_WACS3_RDATA           ((unsigned int *)(PMIC_WRAP_BASE+0xB8))
#define PMIC_WRAP_WACS3_VLDCLR          ((unsigned int *)(PMIC_WRAP_BASE+0xBC))
#define PMIC_WRAP_INT0_EN               ((unsigned int *)(PMIC_WRAP_BASE+0xC0))
#define PMIC_WRAP_INT0_FLG_RAW          ((unsigned int *)(PMIC_WRAP_BASE+0xC4))
#define PMIC_WRAP_INT0_FLG              ((unsigned int *)(PMIC_WRAP_BASE+0xC8))
#define PMIC_WRAP_INT0_CLR              ((unsigned int *)(PMIC_WRAP_BASE+0xCC))
#define PMIC_WRAP_INT1_EN               ((unsigned int *)(PMIC_WRAP_BASE+0xD0))
#define PMIC_WRAP_INT1_FLG_RAW          ((unsigned int *)(PMIC_WRAP_BASE+0xD4))
#define PMIC_WRAP_INT1_FLG              ((unsigned int *)(PMIC_WRAP_BASE+0xD8))
#define PMIC_WRAP_INT1_CLR              ((unsigned int *)(PMIC_WRAP_BASE+0xDC))
#define PMIC_WRAP_SIG_ADR               ((unsigned int *)(PMIC_WRAP_BASE+0xE0))
#define PMIC_WRAP_SIG_MODE              ((unsigned int *)(PMIC_WRAP_BASE+0xE4))
#define PMIC_WRAP_SIG_VALUE             ((unsigned int *)(PMIC_WRAP_BASE+0xE8))
#define PMIC_WRAP_SIG_ERRVAL            ((unsigned int *)(PMIC_WRAP_BASE+0xEC))
#define PMIC_WRAP_CRC_EN                ((unsigned int *)(PMIC_WRAP_BASE+0xF0))
#define PMIC_WRAP_TIMER_EN              ((unsigned int *)(PMIC_WRAP_BASE+0xF4))
#define PMIC_WRAP_TIMER_STA             ((unsigned int *)(PMIC_WRAP_BASE+0xF8))
#define PMIC_WRAP_WDT_UNIT              ((unsigned int *)(PMIC_WRAP_BASE+0xFC))
#define PMIC_WRAP_WDT_SRC_EN            ((unsigned int *)(PMIC_WRAP_BASE+0x100))
#define PMIC_WRAP_WDT_FLG               ((unsigned int *)(PMIC_WRAP_BASE+0x104))
#define PMIC_WRAP_DEBUG_INT_SEL         ((unsigned int *)(PMIC_WRAP_BASE+0x108))
#define PMIC_WRAP_DVFS_ADR0             ((unsigned int *)(PMIC_WRAP_BASE+0x10C))
#define PMIC_WRAP_DVFS_WDATA0           ((unsigned int *)(PMIC_WRAP_BASE+0x110))
#define PMIC_WRAP_DVFS_ADR1             ((unsigned int *)(PMIC_WRAP_BASE+0x114))
#define PMIC_WRAP_DVFS_WDATA1           ((unsigned int *)(PMIC_WRAP_BASE+0x118))
#define PMIC_WRAP_DVFS_ADR2             ((unsigned int *)(PMIC_WRAP_BASE+0x11C))
#define PMIC_WRAP_DVFS_WDATA2           ((unsigned int *)(PMIC_WRAP_BASE+0x120))
#define PMIC_WRAP_DVFS_ADR3             ((unsigned int *)(PMIC_WRAP_BASE+0x124))
#define PMIC_WRAP_DVFS_WDATA3           ((unsigned int *)(PMIC_WRAP_BASE+0x128))
#define PMIC_WRAP_DVFS_ADR4             ((unsigned int *)(PMIC_WRAP_BASE+0x12C))
#define PMIC_WRAP_DVFS_WDATA4           ((unsigned int *)(PMIC_WRAP_BASE+0x130))
#define PMIC_WRAP_DVFS_ADR5             ((unsigned int *)(PMIC_WRAP_BASE+0x134))
#define PMIC_WRAP_DVFS_WDATA5           ((unsigned int *)(PMIC_WRAP_BASE+0x138))
#define PMIC_WRAP_DVFS_ADR6             ((unsigned int *)(PMIC_WRAP_BASE+0x13C))
#define PMIC_WRAP_DVFS_WDATA6           ((unsigned int *)(PMIC_WRAP_BASE+0x140))
#define PMIC_WRAP_DVFS_ADR7             ((unsigned int *)(PMIC_WRAP_BASE+0x144))
#define PMIC_WRAP_DVFS_WDATA7           ((unsigned int *)(PMIC_WRAP_BASE+0x148))
#define PMIC_WRAP_DVFS_ADR8             ((unsigned int *)(PMIC_WRAP_BASE+0x14C))
#define PMIC_WRAP_DVFS_WDATA8           ((unsigned int *)(PMIC_WRAP_BASE+0x150))
#define PMIC_WRAP_DVFS_ADR9             ((unsigned int *)(PMIC_WRAP_BASE+0x154))
#define PMIC_WRAP_DVFS_WDATA9           ((unsigned int *)(PMIC_WRAP_BASE+0x158))
#define PMIC_WRAP_DVFS_ADR10            ((unsigned int *)(PMIC_WRAP_BASE+0x15C))
#define PMIC_WRAP_DVFS_WDATA10          ((unsigned int *)(PMIC_WRAP_BASE+0x160))
#define PMIC_WRAP_DVFS_ADR11            ((unsigned int *)(PMIC_WRAP_BASE+0x164))
#define PMIC_WRAP_DVFS_WDATA11          ((unsigned int *)(PMIC_WRAP_BASE+0x168))
#define PMIC_WRAP_DVFS_ADR12            ((unsigned int *)(PMIC_WRAP_BASE+0x16C))
#define PMIC_WRAP_DVFS_WDATA12          ((unsigned int *)(PMIC_WRAP_BASE+0x170))
#define PMIC_WRAP_DVFS_ADR13            ((unsigned int *)(PMIC_WRAP_BASE+0x174))
#define PMIC_WRAP_DVFS_WDATA13          ((unsigned int *)(PMIC_WRAP_BASE+0x178))
#define PMIC_WRAP_DVFS_ADR14            ((unsigned int *)(PMIC_WRAP_BASE+0x17C))
#define PMIC_WRAP_DVFS_WDATA14          ((unsigned int *)(PMIC_WRAP_BASE+0x180))
#define PMIC_WRAP_DVFS_ADR15            ((unsigned int *)(PMIC_WRAP_BASE+0x184))
#define PMIC_WRAP_DVFS_WDATA15          ((unsigned int *)(PMIC_WRAP_BASE+0x188))
#define PMIC_WRAP_DCXO_ENABLE           ((unsigned int *)(PMIC_WRAP_BASE+0x18C))
#define PMIC_WRAP_DCXO_CONN_ADR0        ((unsigned int *)(PMIC_WRAP_BASE+0x190))
#define PMIC_WRAP_DCXO_CONN_WDATA0      ((unsigned int *)(PMIC_WRAP_BASE+0x194))
#define PMIC_WRAP_DCXO_CONN_ADR1        ((unsigned int *)(PMIC_WRAP_BASE+0x198))
#define PMIC_WRAP_DCXO_CONN_WDATA1      ((unsigned int *)(PMIC_WRAP_BASE+0x19C))
#define PMIC_WRAP_DCXO_NFC_ADR0         ((unsigned int *)(PMIC_WRAP_BASE+0x1A0))
#define PMIC_WRAP_DCXO_NFC_WDATA0       ((unsigned int *)(PMIC_WRAP_BASE+0x1A4))
#define PMIC_WRAP_DCXO_NFC_ADR1         ((unsigned int *)(PMIC_WRAP_BASE+0x1A8))
#define PMIC_WRAP_DCXO_NFC_WDATA1       ((unsigned int *)(PMIC_WRAP_BASE+0x1AC))
#define PMIC_WRAP_SPMINF_STA            ((unsigned int *)(PMIC_WRAP_BASE+0x1B0))
#define PMIC_WRAP_CIPHER_KEY_SEL        ((unsigned int *)(PMIC_WRAP_BASE+0x1B4))
#define PMIC_WRAP_CIPHER_IV_SEL         ((unsigned int *)(PMIC_WRAP_BASE+0x1B8))
#define PMIC_WRAP_CIPHER_EN             ((unsigned int *)(PMIC_WRAP_BASE+0x1BC))
#define PMIC_WRAP_CIPHER_RDY            ((unsigned int *)(PMIC_WRAP_BASE+0x1C0))
#define PMIC_WRAP_CIPHER_MODE           ((unsigned int *)(PMIC_WRAP_BASE+0x1C4))
#define PMIC_WRAP_CIPHER_SWRST          ((unsigned int *)(PMIC_WRAP_BASE+0x1C8))
#define PMIC_WRAP_DCM_EN                ((unsigned int *)(PMIC_WRAP_BASE+0x1CC))
#define PMIC_WRAP_DCM_SPI_DBC_PRD       ((unsigned int *)(PMIC_WRAP_BASE+0x1D0))
#define PMIC_WRAP_DCM_DBC_PRD           ((unsigned int *)(PMIC_WRAP_BASE+0x1D4))
#define PMIC_WRAP_EXT_CK                ((unsigned int *)(PMIC_WRAP_BASE+0x1D8))
#define PMIC_WRAP_ADC_CMD_ADDR          ((unsigned int *)(PMIC_WRAP_BASE+0x1DC))
#define PMIC_WRAP_PWRAP_ADC_CMD         ((unsigned int *)(PMIC_WRAP_BASE+0x1E0))
#define PMIC_WRAP_ADC_RDATA_ADDR        ((unsigned int *)(PMIC_WRAP_BASE+0x1E4))
#define PMIC_WRAP_GPS_STA               ((unsigned int *)(PMIC_WRAP_BASE+0x1E8))
#define PMIC_WRAP_SWRST                 ((unsigned int *)(PMIC_WRAP_BASE+0x1EC))
#define PMIC_WRAP_HARB_SLEEP_GATED_CTRL  ((unsigned int *)(PMIC_WRAP_BASE+0x1F0))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_LATEST  ((unsigned int *)(PMIC_WRAP_BASE+0x1F4))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR_WP  ((unsigned int *)(PMIC_WRAP_BASE+0x1F8))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR0    ((unsigned int *)(PMIC_WRAP_BASE+0x1FC))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR1    ((unsigned int *)(PMIC_WRAP_BASE+0x200))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR2    ((unsigned int *)(PMIC_WRAP_BASE+0x204))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR3    ((unsigned int *)(PMIC_WRAP_BASE+0x208))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR4    ((unsigned int *)(PMIC_WRAP_BASE+0x20C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR5    ((unsigned int *)(PMIC_WRAP_BASE+0x210))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR6    ((unsigned int *)(PMIC_WRAP_BASE+0x214))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR7    ((unsigned int *)(PMIC_WRAP_BASE+0x218))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR8    ((unsigned int *)(PMIC_WRAP_BASE+0x21C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR9    ((unsigned int *)(PMIC_WRAP_BASE+0x220))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR10   ((unsigned int *)(PMIC_WRAP_BASE+0x224))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR11   ((unsigned int *)(PMIC_WRAP_BASE+0x228))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR12   ((unsigned int *)(PMIC_WRAP_BASE+0x22C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR13   ((unsigned int *)(PMIC_WRAP_BASE+0x230))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR14   ((unsigned int *)(PMIC_WRAP_BASE+0x234))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR15   ((unsigned int *)(PMIC_WRAP_BASE+0x238))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR16   ((unsigned int *)(PMIC_WRAP_BASE+0x23C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR17   ((unsigned int *)(PMIC_WRAP_BASE+0x240))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR18   ((unsigned int *)(PMIC_WRAP_BASE+0x244))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR19   ((unsigned int *)(PMIC_WRAP_BASE+0x248))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR20   ((unsigned int *)(PMIC_WRAP_BASE+0x24C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR21   ((unsigned int *)(PMIC_WRAP_BASE+0x250))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR22   ((unsigned int *)(PMIC_WRAP_BASE+0x254))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR23   ((unsigned int *)(PMIC_WRAP_BASE+0x258))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR24   ((unsigned int *)(PMIC_WRAP_BASE+0x25C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR25   ((unsigned int *)(PMIC_WRAP_BASE+0x260))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR26   ((unsigned int *)(PMIC_WRAP_BASE+0x264))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR27   ((unsigned int *)(PMIC_WRAP_BASE+0x268))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR28   ((unsigned int *)(PMIC_WRAP_BASE+0x26C))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR29   ((unsigned int *)(PMIC_WRAP_BASE+0x270))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR30   ((unsigned int *)(PMIC_WRAP_BASE+0x274))
#define PMIC_WRAP_MD_ADC_RDATA_ADDR31   ((unsigned int *)(PMIC_WRAP_BASE+0x278))
#define PMIC_WRAP_MD_ADC_MODE_SEL       ((unsigned int *)(PMIC_WRAP_BASE+0x27C))
#define PMIC_WRAP_MD_ADC_STA0           ((unsigned int *)(PMIC_WRAP_BASE+0x280))
#define PMIC_WRAP_MD_ADC_STA1           ((unsigned int *)(PMIC_WRAP_BASE+0x284))
#define PMIC_WRAP_PRIORITY_USER_SEL_0   ((unsigned int *)(PMIC_WRAP_BASE+0x288))
#define PMIC_WRAP_PRIORITY_USER_SEL_1   ((unsigned int *)(PMIC_WRAP_BASE+0x28C))
#define PMIC_WRAP_ARBITER_OUT_SEL_0     ((unsigned int *)(PMIC_WRAP_BASE+0x290))
#define PMIC_WRAP_ARBITER_OUT_SEL_1     ((unsigned int *)(PMIC_WRAP_BASE+0x294))
#define PMIC_WRAP_STARV_COUNTER_0       ((unsigned int *)(PMIC_WRAP_BASE+0x298))
#define PMIC_WRAP_STARV_COUNTER_1       ((unsigned int *)(PMIC_WRAP_BASE+0x29C))
#define PMIC_WRAP_STARV_COUNTER_2       ((unsigned int *)(PMIC_WRAP_BASE+0x2A0))
#define PMIC_WRAP_STARV_COUNTER_3       ((unsigned int *)(PMIC_WRAP_BASE+0x2A4))
#define PMIC_WRAP_STARV_COUNTER_4       ((unsigned int *)(PMIC_WRAP_BASE+0x2A8))
#define PMIC_WRAP_STARV_COUNTER_5       ((unsigned int *)(PMIC_WRAP_BASE+0x2AC))
#define PMIC_WRAP_STARV_COUNTER_6       ((unsigned int *)(PMIC_WRAP_BASE+0x2B0))
#define PMIC_WRAP_STARV_COUNTER_7       ((unsigned int *)(PMIC_WRAP_BASE+0x2B4))
#define PMIC_WRAP_STARV_COUNTER_8       ((unsigned int *)(PMIC_WRAP_BASE+0x2B8))
#define PMIC_WRAP_STARV_COUNTER_9       ((unsigned int *)(PMIC_WRAP_BASE+0x2BC))
#define PMIC_WRAP_STARV_COUNTER_10      ((unsigned int *)(PMIC_WRAP_BASE+0x2C0))
#define PMIC_WRAP_STARV_COUNTER_11      ((unsigned int *)(PMIC_WRAP_BASE+0x2C4))
#define PMIC_WRAP_STARV_COUNTER_12      ((unsigned int *)(PMIC_WRAP_BASE+0x2C8))
#define PMIC_WRAP_STARV_COUNTER_13      ((unsigned int *)(PMIC_WRAP_BASE+0x2CC))
#define PMIC_WRAP_STARV_COUNTER_14      ((unsigned int *)(PMIC_WRAP_BASE+0x2D0))
#define PMIC_WRAP_STARV_COUNTER_15      ((unsigned int *)(PMIC_WRAP_BASE+0x2D4))
#define PMIC_WRAP_STARV_COUNTER_0_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2D8))
#define PMIC_WRAP_STARV_COUNTER_1_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2DC))
#define PMIC_WRAP_STARV_COUNTER_2_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2E0))
#define PMIC_WRAP_STARV_COUNTER_3_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2E4))
#define PMIC_WRAP_STARV_COUNTER_4_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2E8))
#define PMIC_WRAP_STARV_COUNTER_5_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2EC))
#define PMIC_WRAP_STARV_COUNTER_6_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2F0))
#define PMIC_WRAP_STARV_COUNTER_7_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2F4))
#define PMIC_WRAP_STARV_COUNTER_8_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2F8))
#define PMIC_WRAP_STARV_COUNTER_9_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x2FC))
#define PMIC_WRAP_STARV_COUNTER_10_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x300))
#define PMIC_WRAP_STARV_COUNTER_11_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x304))
#define PMIC_WRAP_STARV_COUNTER_12_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x308))
#define PMIC_WRAP_STARV_COUNTER_13_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x30C))
#define PMIC_WRAP_STARV_COUNTER_14_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x310))
#define PMIC_WRAP_STARV_COUNTER_15_STATUS  ((unsigned int *)(PMIC_WRAP_BASE+0x314))
#define PMIC_WRAP_STARV_COUNTER_CLR     ((unsigned int *)(PMIC_WRAP_BASE+0x318))
#define PMIC_WRAP_STARV_PRIO_STATUS     ((unsigned int *)(PMIC_WRAP_BASE+0x31C))
#define PMIC_WRAP_DEBUG_STATUS          ((unsigned int *)(PMIC_WRAP_BASE+0x320))
#define PMIC_WRAP_DEBUG_SQUENCE_0       ((unsigned int *)(PMIC_WRAP_BASE+0x324))
#define PMIC_WRAP_DEBUG_SQUENCE_1       ((unsigned int *)(PMIC_WRAP_BASE+0x328))
#define PMIC_WRAP_DEBUG_SQUENCE_2       ((unsigned int *)(PMIC_WRAP_BASE+0x32C))
#define PMIC_WRAP_DEBUG_SQUENCE_3       ((unsigned int *)(PMIC_WRAP_BASE+0x330))
#define PMIC_WRAP_DEBUG_SQUENCE_4       ((unsigned int *)(PMIC_WRAP_BASE+0x334))
#define PMIC_WRAP_DEBUG_SQUENCE_5       ((unsigned int *)(PMIC_WRAP_BASE+0x338))
#define PMIC_WRAP_DEBUG_SQUENCE_6       ((unsigned int *)(PMIC_WRAP_BASE+0x33C))


/*************** macro for wrapper  regsister ************************/
#define GET_STAUPD_DLE_CNT(x)        ((x>>0)  & 0x00000007)
#define GET_STAUPD_ALE_CNT(x)        ((x>>3)  & 0x00000007)
#define GET_STAUPD_FSM(x)            ((x>>6)  & 0x00000007)
#define GET_WRAP_CH_DLE_RESTCNT(x)   ((x>>0)  & 0x00000007)
#define GET_WRAP_CH_ALE_RESTCNT(x)   ((x>>3)  & 0x00000003)
#define GET_WRAP_AG_DLE_RESTCNT(x)   ((x>>5)  & 0x00000003)
#define GET_WRAP_CH_W(x)             ((x>>7)  & 0x00000001)
#define GET_WRAP_CH_REQ(x)           ((x>>8)  & 0x00000001)
#define GET_AG_WRAP_W(x)             ((x>>9)  & 0x00000001)
#define GET_AG_WRAP_REQ(x)           ((x>>10) & 0x00000001)
#define GET_WRAP_FSM(x)              ((x>>11) & 0x0000000f)
#define GET_HARB_WRAP_WDATA(x)       ((x>>0)  & 0x0000ffff)
#define GET_HARB_WRAP_ADR(x)         ((x>>16) & 0x00007fff)
#define GET_HARB_WRAP_REQ(x)         ((x>>31) & 0x00000001)
#define GET_HARB_DLE_EMPTY(x)        ((x>>0)  & 0x00000001)
#define GET_HARB_DLE_FULL(x)         ((x>>1)  & 0x00000001)
#define GET_HARB_VLD(x)              ((x>>2)  & 0x00000001)
#define GET_HARB_DLE_OWN(x)          ((x>>3)  & 0x0000000f)
#define GET_HARB_OWN(x)              ((x>>7)  & 0x0000000f)
#define GET_HARB_DLE_RESTCNT(x)      ((x>>11) & 0x0000000f)
#define GET_AG_HARB_REQ(x)           ((x>>15) & 0x00003fff)
#define GET_HARB_WRAP_W(x)           ((x>>29) & 0x00000001)
#define GET_HARB_WRAP_REQ0(x)        ((x>>30) & 0x00000001)
#define GET_SPI_WDATA(x)             ((x>>0)  & 0x000000ff)
#define GET_SPI_OP(x)                ((x>>8)  & 0x0000001f)
#define GET_SPI_W(x)                 ((x>>13) & 0x00000001)
#define GET_MAN_RDATA(x)             ((x>>0)  & 0x000000ff)
#define GET_MAN_FSM(x)               ((x>>8)  & 0x00000007)
#define GET_MAN_REQ(x)               ((x>>11) & 0x00000001)
#define GET_WACS0_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS0_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS0_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS0_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS0_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS0_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE0(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE0(x)            ((x>>21) & 0x00000001)
#define GET_SYS_IDLE0(x)             ((x>>22) & 0x00000001)
#define GET_WACS0_FIFO_FILLCNT(x)    ((x>>24) & 0x0000000f)
#define GET_WACS0_FIFO_FREECNT(x)    ((x>>28) & 0x0000000f)
#define GET_WACS1_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS1_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS1_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS1_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS1_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS1_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE1(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE1(x)            ((x>>21) & 0x00000001)
#define GET_SYS_IDLE1(x)             ((x>>22) & 0x00000001)
#define GET_WACS1_FIFO_FILLCNT(x)    ((x>>24) & 0x0000000f)
#define GET_WACS1_FIFO_FREECNT(x)    ((x>>28) & 0x0000000f)
#define GET_WACS2_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS2_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS2_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS2_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS2_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS2_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE2(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE2(x)            ((x>>21) & 0x00000001)
#define GET_SYS_IDLE2(x)             ((x>>22) & 0x00000001)
#define GET_WACS2_FIFO_FILLCNT(x)    ((x>>24) & 0x0000000f)
#define GET_WACS2_FIFO_FREECNT(x)    ((x>>28) & 0x0000000f)
#define GET_WACS3_WDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS3_ADR(x)             ((x>>16) & 0x00007fff)
#define GET_WACS3_WRITE(x)           ((x>>31) & 0x00000001)
#define GET_WACS3_RDATA(x)           ((x>>0)  & 0x0000ffff)
#define GET_WACS3_FSM(x)             ((x>>16) & 0x00000007)
#define GET_WACS3_REQ(x)             ((x>>19) & 0x00000001)
#define GET_SYNC_IDLE3(x)            ((x>>20) & 0x00000001)
#define GET_INIT_DONE3(x)            ((x>>21) & 0x00000001)
#define GET_SYS_IDLE3(x)             ((x>>22) & 0x00000001)
#define GET_WACS3_FIFO_FILLCNT(x)    ((x>>24) & 0x0000000f)
#define GET_WACS3_FIFO_FREECNT(x)    ((x>>28) & 0x0000000f)
#define GET_PWRAP_GPS_ACK(x)         ((x>>0)  & 0x00000001)
#define GET_GPS_PWRAP_REQ(x)         ((x>>1)  & 0x00000001)
#define GET_GPSINF_DLE_CNT(x)        ((x>>4)  & 0x00000003)
#define GET_GPSINF_ALE_CNT(x)        ((x>>6)  & 0x00000003)
#define GET_GPS_INF_FSM(x)           ((x>>8)  & 0x00000007)
#define GET_PWRAP_GPS_WDATA(x)       ((x>>17) & 0x00007fff)
#define GET_PWRAP_MD_ADC_TEMP_DATA(x)  ((x>>16) & 0x0000ffff)

#endif

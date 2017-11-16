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

#ifndef __CCCI_PLATFORM_CFG_H__
#define __CCCI_PLATFORM_CFG_H__
#include <linux/version.h>
#include <mt-plat/mt_boot.h>
#include <mt-plat/sync_write.h>
/* -------------ccci driver configure------------------------*/
#define MD1_DEV_MAJOR	(184)
#define MD2_DEV_MAJOR	(169)
/* #define CCCI_PLATFORM_L	0x3536544D */
/* #define CCCI_PLATFORM_H	0x31453537 */
#define CCCI_PLATFORM			"MT6580E1"
#define CCCI1_DRIVER_VER		0x20121001
#define CCCI2_DRIVER_VER		0x20121001
/*  Note: must sync with sec lib, if ccci and sec has dependency change */
#define CURR_SEC_CCCI_SYNC_VER		(1)
#define CCMNI_V1			(1)
#define CCMNI_V2			(2)
#define MD_HEADER_VER_NO		(2)
#define GFH_HEADER_VER_NO		(1)
/* -------------md share memory configure----------------*/
/* Common configuration */
#define MD_EX_LOG_SIZE				(2*1024)
#define CCCI_MISC_INFO_SMEM_SIZE	(1*1024)
/*  2M align for share memory */
#define CCCI_SHARED_MEM_SIZE		UL(0x200000)
#define MD_IMG_DUMP_SIZE			(1<<8)
#define DSP_IMG_DUMP_SIZE			(1<<9)
/* For V1 CCMNI */
#define CCMNI_V1_PORT_NUM			(3)
/*  For V2 CCMNI */
#define CCMNI_V2_PORT_NUM			(3)
/*  MD SYS1 configuration */
/*  PCM */
#define CCCI1_PCM_SMEM_SIZE			(16 * 2 * 1024)
/*  MD Log */
#define CCCI1_MD_LOG_SIZE			(137*1024*4+64*1024+112*1024)
#define RPC1_MAX_BUF_SIZE			2048
/* support 2 concurrently request*/
#define RPC1_REQ_BUF_NUM			2
#define CCCI1_TTY_BUF_SIZE			(16 * 1024)
#define CCCI1_CCMNI_BUF_SIZE			(16*1024)
#define CCCI1_TTY_PORT_NUM			(3)
/* #define CCCI1_CCMNI_V1_PORT_NUM		(3)*/
/*  MD SYS2 configuration */
#define CCCI2_PCM_SMEM_SIZE			(16 * 2 * 1024)
/*  MD Log */
#define CCCI2_MD_LOG_SIZE			(137*1024*4+64*1024+112*1024)

/* support 2 concurrently request */
#define RPC2_MAX_BUF_SIZE			2048
#define RPC2_REQ_BUF_NUM			2
#define CCCI2_TTY_BUF_SIZE			(16 * 1024)
#define CCCI2_CCMNI_BUF_SIZE			(16*1024)
#define CCCI2_TTY_PORT_NUM			(3)
/*  For V1 CCMNI */
/* #define CCCI2_CCMNI_V1_PORT_NUM		(3)*/
/* -------------feature enable/disable configure----------------*/
/******security feature configure******/
/* enable debug log for SECURE_ALGO_OP, always disable*/
/* #define  ENCRYPT_DEBUG */
/* disable for bring up, need enable by security owner after security feature ready */
/*  #define  ENABLE_MD_IMG_SECURITY_FEATURE */
/******share memory configure******/
/* using ioremap to allocate share memory, not dma_alloc_coherent */
#define CCCI_STATIC_SHARED_MEM
/* md region can be adjusted by 2G/3G, ex, 2G: 10MB for md+dsp, 3G: 22MB for md+dsp */
/* #define  MD_IMG_SIZE_ADJUST_BY_VER */
/******md header check configure******/
/* #define  ENABLE_CHIP_VER_CHECK */
#define  ENABLE_2G_3G_CHECK
#define  ENABLE_MEM_SIZE_CHECK
/******EMI MPU protect configure******/
/* disable for bring up            */
#define  ENABLE_EMI_PROTECTION
/******md memory remap configure******/
#define  ENABLE_MEM_REMAP_HW
/******md wake up workaround******/
/* only enable for mt6589 platform */
/* #define  ENABLE_MD_WAKE_UP */
/* ******other feature configure******/
/* #define  ENABLE_LOCK_MD_SLP_FEATURE */
/* disable for bring up */
#define ENABLE_32K_CLK_LESS
/* disable for bring up for md not enable wdt at bring up */
#define  ENABLE_MD_WDT_PROCESS
/* disable for bring up */
#define ENABLE_AEE_MD_EE
/* awlays enable for bring up */
#define  ENABLE_DRAM_API
#define ENABLE_SW_MEM_REMAP
#define ENABLE_DUMP_MD_REGISTER
#define ENABLE_DUMP_MD_REG
#ifdef CONFIG_FW_LOADER_USER_HELPER
#define ENABLE_LOAD_IMG_BY_REQUEST_FIRMWARE
#endif

/*#define FEATURE_GET_TD_EINT_NUM*/
#define FEATURE_GET_MD_GPIO_NUM
#define FEATURE_GET_MD_GPIO_VAL
#define FEATURE_GET_MD_ADC_NUM
#define FEATURE_GET_MD_ADC_VAL
#define FEATURE_GET_MD_EINT_ATTR	/*disable for bring up  */
#define FEATURE_GET_MD_EINT_ATTR_DTS
/*#define FEATURE_GET_DRAM_TYPE_CLK*/
/*#define FEATURE_MD_FAST_DORMANCY*/
#define FEATURE_GET_MD_BAT_VOL
#define FEATURE_PM_IPO_H	/*disable for bring up */
/*#define FEATURE_DFO_EN					  Always bring up*/

/*******************AP CCIF register define**********************/
#define CCIF_BASE			(AP_CCIF_BASE)
#define CCIF_CON(addr)			((addr) + 0x0100)
#define CCIF_BUSY(addr)			((addr) + 0x0104)
#define CCIF_START(addr)		((addr) + 0x0108)
#define CCIF_TCHNUM(addr)		((addr) + 0x010C)
#define CCIF_RCHNUM(addr)		((addr) + 0x0110)
#define CCIF_ACK(addr)			((addr) + 0x0114)
/* for CHDATA, the first half space belongs to AP and the remaining space belongs to MD */
#define CCIF_TXCHDATA(addr)		((addr) + 0x0200)
#define CCIF_RXCHDATA(addr)		((addr) + 0x0200 + 128)
/* Modem CCIF */
#define MD_CCIF_CON(base)		((base) + 0x0120)
#define MD_CCIF_BUSY(base)		((base) + 0x0124)
#define MD_CCIF_START(base)		((base) + 0x128)
#define MD_CCIF_TCHNUM(base)		((base) + 0x012C)
#define MD_CCIF_RCHNUM(base)		((base) + 0x0130)
#define MD_CCIF_ACK(base)		((base) + 0x0134)
/* define constant */
#define CCIF_CON_SEQ			0x00	/* sequential */
#define CCIF_CON_ARB			0x01	/* arbitration */
/* #define CCIF_IRQ_CODE MT_AP_CCIF_IRQ_ID */
/*  CCIF HW specific macro definition */
#define CCIF_STD_V1_MAX_CH_NUM				(8)
#define CCIF_STD_V1_RUN_TIME_DATA_OFFSET	(0x240)
#define CCIF_STD_V1_RUM_TIME_MEM_MAX_LEN	(256-64)
/*******************other register define**********************/
/* modem debug register and bit */
/*  #define MD_DEBUG_MODE	(DBGAPB_BASE+0x1A010)*/
#define MD_DBG_JTAG_BIT		(1<<0)
/************************* define function macro **************/
#define CCIF_MASK(irq)		disable_irq(irq)
#define CCIF_UNMASK(irq)	enable_irq(irq)
#define CCIF_CLEAR_PHY(pc)	(*CCIF_ACK(pc) = 0xFFFFFFFF)
#define ccci_write32(a, v)	mt_reg_sync_writel(v, a)
#define ccci_write16(a, v)	mt_reg_sync_writew(v, a)
#define ccci_write8(a, v)	mt_reg_sync_writeb(v, a)
#define ccci_read32(a)		(*((volatile unsigned int *)a))
#define ccci_read16(a)		(*((volatile unsigned short *)a))
#define ccci_read8(a)		(*((volatile unsigned char *)a))
#endif	/*  __CCCI_PLATFORM_CFG_H__ */

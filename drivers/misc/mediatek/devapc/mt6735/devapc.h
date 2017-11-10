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

#ifndef _DAPC_H
#define _DAPC_H
#include <linux/types.h>
#define MOD_NO_IN_1_DEVAPC                  16
#define DEVAPC_TAG                          "DEVAPC"

/* 1: Force to enable enhanced one-core violation debugging
 * 0: Enhanced one-core violation debugging can be enabled dynamically
 * Notice: You should only use one core to debug
 * (Please note it may trigger PRINTK too much)  */
#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	0

#if defined(CONFIG_ARCH_MT6735)
	/*For EMI API DEVAPC0_D0_VIO_STA_3, idx:124*/
	#define ABORT_EMI                0x10000000
#elif defined(CONFIG_ARCH_MT6735M)
	/*For EMI API DEVAPC0_D0_VIO_STA_3, idx:115*/
	#define ABORT_EMI                0x80000
#elif defined(CONFIG_ARCH_MT6753)
	/*For EMI API DEVAPC0_D0_VIO_STA_3, idx:125*/
	#define ABORT_EMI                0x20000000
#else

#error "Wrong Config type"

#endif

#define DAPC_INPUT_TYPE_DEBUG_ON	200
#define DAPC_INPUT_TYPE_DEBUG_OFF	100

/*Define constants*/
#define DEVAPC_DEVICE_NUMBER    140  /* This number must be bigger than total slave */
#define DEVAPC_DOMAIN_AP        0
#define DEVAPC_DOMAIN_MD1       1
#define DEVAPC_DOMAIN_CONN      2
#define DEVAPC_DOMAIN_MD32      3

#if defined(CONFIG_ARCH_MT6735)

#define DEVAPC_DOMAIN_MM        4
#define DEVAPC_DOMAIN_MD3       5
#define DEVAPC_DOMAIN_MFG       6

#define VIO_DBG_MSTID 0x0003FFF
#define VIO_DBG_DMNID 0x000C000
#define VIO_DBG_RW    0x3000000
#define VIO_DBG_CLR   0x80000000

#elif defined(CONFIG_ARCH_MT6735M)

#define VIO_DBG_MSTID 0x0003FFF
#define VIO_DBG_DMNID 0x000C000
#define VIO_DBG_RW    0x3000000
#define VIO_DBG_CLR   0x80000000

#elif defined(CONFIG_ARCH_MT6753)

#define DEVAPC_DOMAIN_MM        4
#define DEVAPC_DOMAIN_MD3       5
#define DEVAPC_DOMAIN_MFG       6

#define VIO_DBG_MSTID 0x00000FFF
#define VIO_DBG_DMNID 0x0000F000
#define VIO_DBG_RW    0x30000000
#define VIO_DBG_CLR   0x80000000

#else

#error "Wrong MACH type"

#endif


/******************************************************************************
*REGISTER ADDRESS DEFINATION
******************************************************************************/
#define DEVAPC0_D0_APC_0            (devapc_ao_base+0x0000)
#define DEVAPC0_D1_APC_0            (devapc_ao_base+0x0100)
#define DEVAPC0_D2_APC_0            (devapc_ao_base+0x0200)
#define DEVAPC0_D3_APC_0            (devapc_ao_base+0x0300)

#if defined(CONFIG_ARCH_MT6735)

#define DEVAPC0_D4_APC_0            (devapc_ao_base+0x0400)
#define DEVAPC0_D5_APC_0            (devapc_ao_base+0x0500)
#define DEVAPC0_D6_APC_0            (devapc_ao_base+0x0600)
#define DEVAPC0_D7_APC_0            (devapc_ao_base+0x0700)

#elif defined(CONFIG_ARCH_MT6735M)

#elif defined(CONFIG_ARCH_MT6753)

#define DEVAPC0_D4_APC_0            (devapc_ao_base+0x0400)
#define DEVAPC0_D5_APC_0            (devapc_ao_base+0x0500)
#define DEVAPC0_D6_APC_0            (devapc_ao_base+0x0600)
#define DEVAPC0_D7_APC_0            (devapc_ao_base+0x0700)

#else

#error "Wrong Config type"

#endif


#define DEVAPC0_APC_CON             ((unsigned int *)(devapc_ao_base+0x0F00))
#define DEVAPC0_APC_LOCK_0          ((unsigned int *)(devapc_ao_base+0x0F04))
#define DEVAPC0_APC_LOCK_1          ((unsigned int *)(devapc_ao_base+0x0F08))
#define DEVAPC0_APC_LOCK_2          ((unsigned int *)(devapc_ao_base+0x0F0C))
#define DEVAPC0_APC_LOCK_3          ((unsigned int *)(devapc_ao_base+0x0F10))
#define DEVAPC0_APC_LOCK_4          ((unsigned int *)(devapc_ao_base+0x0F14))

#define DEVAPC0_PD_APC_CON          ((unsigned int *)(devapc_pd_base+0x0F00))
#define DEVAPC0_D0_VIO_MASK_0       ((unsigned int *)(devapc_pd_base+0x0000))
#define DEVAPC0_D0_VIO_MASK_1       ((unsigned int *)(devapc_pd_base+0x0004))
#define DEVAPC0_D0_VIO_MASK_2       ((unsigned int *)(devapc_pd_base+0x0008))
#define DEVAPC0_D0_VIO_MASK_3       ((unsigned int *)(devapc_pd_base+0x000C))
#define DEVAPC0_D0_VIO_MASK_4       ((unsigned int *)(devapc_pd_base+0x0010))
#define DEVAPC0_D0_VIO_STA_0        ((unsigned int *)(devapc_pd_base+0x0400))
#define DEVAPC0_D0_VIO_STA_1        ((unsigned int *)(devapc_pd_base+0x0404))
#define DEVAPC0_D0_VIO_STA_2        ((unsigned int *)(devapc_pd_base+0x0408))
#define DEVAPC0_D0_VIO_STA_3        ((unsigned int *)(devapc_pd_base+0x040C))
#define DEVAPC0_D0_VIO_STA_4        ((unsigned int *)(devapc_pd_base+0x0410))
#define DEVAPC0_VIO_DBG0            ((unsigned int *)(devapc_pd_base+0x0900))
#define DEVAPC0_VIO_DBG1            ((unsigned int *)(devapc_pd_base+0x0904))
#define DEVAPC0_DEC_ERR_CON         ((unsigned int *)(devapc_pd_base+0x0F80))
#define DEVAPC0_DEC_ERR_ADDR        ((unsigned int *)(devapc_pd_base+0x0F84))
#define DEVAPC0_DEC_ERR_ID          ((unsigned int *)(devapc_pd_base+0x0F88))

struct DEVICE_INFO {
	const char      *device;
};

struct DOMAIN_INFO {
	const char      *name;
};


#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

#endif


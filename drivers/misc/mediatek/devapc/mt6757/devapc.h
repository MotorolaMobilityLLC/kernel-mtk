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

/* 1: Force to enable enhanced one-core violation debugging */
/* 0: Enhanced one-core violation debugging can be enabled dynamically */
/* Notice: You should only use one core to debug */
/* (Please note it may trigger PRINTK too much)  */
#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	0

/* Uncomment to enable AEE  */
/* #define DEVAPC_ENABLE_AEE  */

#if defined(CONFIG_MTK_AEE_FEATURE) && defined(DEVAPC_ENABLE_AEE)

/* This is necessary for AEE */
#define DEVAPC_TOTAL_SLAVES                 221

/* AEE trigger threshold for each module.
 * Remember: NEVER set it to 1
 */
#define DEVAPC_VIO_AEE_TRIGGER_TIMES        10

/* AEE trigger frequency for each module (ms) */
#define DEVAPC_VIO_AEE_TRIGGER_FREQUENCY    1000

/* Maximum populating AEE times for all the modules */
#define DEVAPC_VIO_MAX_TOTAL_MODULE_AEE_TRIGGER_TIMES        3

#endif

/*For EMI API(DRAM violation) DEVAPC0_D0_VIO_STA_7, idx:228*/
#define ABORT_EMI                0x00000010
#define INDEX_EMI                228

/*For EMI_MPU(APB violation) API DEVAPC0_D0_VIO_STA_7, idx:229*/
#define ABORT_EMI_MPU            0x00000020
#define INDEX_EMI_MPU            229

#define DAPC_INPUT_TYPE_DEBUG_ON	200
#define DAPC_INPUT_TYPE_DEBUG_OFF	100

/*Define constants*/

#define VIO_DBG_MSTID 0x0000FFFF
#define VIO_DBG_DMNID 0x000F0000
#define VIO_DBG_W     0x10000000
#define VIO_DBG_CLR   0x80000000

/******************************************************************************
*REGISTER ADDRESS DEFINATION
******************************************************************************/

/* Device APC PD */
#define DEVAPC0_D0_VIO_MASK_0       ((volatile unsigned int *)(devapc_pd_base+0x0000))
#define DEVAPC0_D0_VIO_MASK_1       ((volatile unsigned int *)(devapc_pd_base+0x0004))
#define DEVAPC0_D0_VIO_MASK_2       ((volatile unsigned int *)(devapc_pd_base+0x0008))
#define DEVAPC0_D0_VIO_MASK_3       ((volatile unsigned int *)(devapc_pd_base+0x000C))
#define DEVAPC0_D0_VIO_MASK_4       ((volatile unsigned int *)(devapc_pd_base+0x0010))
#define DEVAPC0_D0_VIO_MASK_5       ((volatile unsigned int *)(devapc_pd_base+0x0014))
#define DEVAPC0_D0_VIO_MASK_6       ((volatile unsigned int *)(devapc_pd_base+0x0018))
#define DEVAPC0_D0_VIO_MASK_7       ((volatile unsigned int *)(devapc_pd_base+0x001C))

#define DEVAPC0_D0_VIO_STA_0        ((volatile unsigned int *)(devapc_pd_base+0x0400))
#define DEVAPC0_D0_VIO_STA_1        ((volatile unsigned int *)(devapc_pd_base+0x0404))
#define DEVAPC0_D0_VIO_STA_2        ((volatile unsigned int *)(devapc_pd_base+0x0408))
#define DEVAPC0_D0_VIO_STA_3        ((volatile unsigned int *)(devapc_pd_base+0x040C))
#define DEVAPC0_D0_VIO_STA_4        ((volatile unsigned int *)(devapc_pd_base+0x0410))
#define DEVAPC0_D0_VIO_STA_5        ((volatile unsigned int *)(devapc_pd_base+0x0414))
#define DEVAPC0_D0_VIO_STA_6        ((volatile unsigned int *)(devapc_pd_base+0x0418))
#define DEVAPC0_D0_VIO_STA_7        ((volatile unsigned int *)(devapc_pd_base+0x041C))

#define DEVAPC0_VIO_DBG0            ((volatile unsigned int *)(devapc_pd_base+0x0900))
#define DEVAPC0_VIO_DBG1            ((volatile unsigned int *)(devapc_pd_base+0x0904))

#define DEVAPC0_PD_APC_CON          ((volatile unsigned int *)(devapc_pd_base+0x0F00))

#define DEVAPC0_DEC_ERR_CON         ((volatile unsigned int *)(devapc_pd_base+0x0F80))
#define DEVAPC0_DEC_ERR_ADDR        ((volatile unsigned int *)(devapc_pd_base+0x0F84))
#define DEVAPC0_DEC_ERR_ID          ((volatile unsigned int *)(devapc_pd_base+0x0F88))


struct DEVICE_INFO {
	const char      *device;
	bool            enable_vio_irq;
};

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

#endif


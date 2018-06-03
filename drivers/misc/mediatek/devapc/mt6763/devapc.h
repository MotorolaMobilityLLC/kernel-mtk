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

#ifndef __DAPC_H__
#define __DAPC_H__

#include <linux/types.h>

/******************************************************************************
* CONSTANT DEFINATION
******************************************************************************/

#define MOD_NO_IN_1_DEVAPC                  16
#define DEVAPC_TAG                          "DEVAPC"

/* 1: Force to enable enhanced one-core violation debugging */
/* 0: Enhanced one-core violation debugging can be enabled dynamically */
/* Notice: You should only use one core to debug */
/* (Please note it may trigger PRINTK too much)  */
#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	1

#define DAPC_INPUT_TYPE_DEBUG_ON	200
#define DAPC_INPUT_TYPE_DEBUG_OFF	100

#define DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX    0

/* For Infra VIO_DBG */
#define INFRA_VIO_DBG_MSTID             0x0000FFFF
#define INFRA_VIO_DBG_MSTID_START_BIT   0
#define INFRA_VIO_DBG_DMNID             0x003F0000
#define INFRA_VIO_DBG_DMNID_START_BIT   16
#define INFRA_VIO_DBG_W_VIO             0x00400000
#define INFRA_VIO_DBG_W_VIO_START_BIT   22
#define INFRA_VIO_DBG_R_VIO             0x00800000
#define INFRA_VIO_DBG_R_VIO_START_BIT   23
#define INFRA_VIO_ADDR_HIGH             0x0F000000
#define INFRA_VIO_ADDR_HIGH_START_BIT   24

/******************************************************************************
* REGISTER ADDRESS DEFINATION
******************************************************************************/

/* Device APC PD */
#define PD_INFRA_VIO_SHIFT_MAX_BIT      21
#define PD_INFRA_VIO_SHIFT_MAX_INDEX    295
#define PD_INFRA_VIO_MASK_MAX_INDEX     307
#define PD_INFRA_VIO_STA_MAX_INDEX      307

#define DEVAPC_PD_INFRA_VIO_MASK(index)    ((volatile unsigned int *)(devapc_pd_infra_base + 0x4 * index))
#define DEVAPC_PD_INFRA_VIO_STA(index)     ((volatile unsigned int *)(devapc_pd_infra_base + 0x400 + 0x4 * index))

#define DEVAPC_PD_INFRA_VIO_DBG0           ((volatile unsigned int *)(devapc_pd_infra_base+0x900))
#define DEVAPC_PD_INFRA_VIO_DBG1           ((volatile unsigned int *)(devapc_pd_infra_base+0x904))

#define DEVAPC_PD_INFRA_APC_CON            ((volatile unsigned int *)(devapc_pd_infra_base+0xF00))

#define DEVAPC_PD_INFRA_VIO_SHIFT_STA      ((volatile unsigned int *)(devapc_pd_infra_base+0xF10))
#define DEVAPC_PD_INFRA_VIO_SHIFT_SEL      ((volatile unsigned int *)(devapc_pd_infra_base+0xF14))
#define DEVAPC_PD_INFRA_VIO_SHIFT_CON      ((volatile unsigned int *)(devapc_pd_infra_base+0xF20))


struct DEVICE_INFO {
	const char      *device;
	bool            enable_vio_irq;
};

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

#endif /* __DAPC_H__ */

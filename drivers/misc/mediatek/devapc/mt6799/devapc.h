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
#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	0

#define DAPC_INPUT_TYPE_DEBUG_ON	200
#define DAPC_INPUT_TYPE_DEBUG_OFF	100

#define DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX    0
#define DAPC_DEVICE_TREE_NODE_PD_PERI_INDEX     1

/* For Infra VIO_DBG */
#define INFRA_VIO_DBG_MSTID             0x0000FFFF
#define INFRA_VIO_DBG_MSTID_START_BIT   0
#define INFRA_VIO_DBG_DMNID             0x00FF0000
#define INFRA_VIO_DBG_DMNID_START_BIT   16
#define INFRA_VIO_ADDR_HIGH             0x0F000000
#define INFRA_VIO_ADDR_HIGH_START_BIT   24
#define INFRA_VIO_DBG_W                 0x10000000
#define INFRA_VIO_DBG_W_START_BIT       28
#define INFRA_VIO_DBG_R                 0x20000000
#define INFRA_VIO_DBG_R_START_BIT       29
#define INFRA_VIO_DBG_CLR               0x80000000
#define INFRA_VIO_DBG_CLR_START_BIT     31

/* For Peri VIO_DBG */
#define PERI_VIO_DBG_MSTID              0x0003FFFF
#define PERI_VIO_DBG_MSTID_START_BIT    0
#define PERI_VIO_DBG_DMNID              0x03FC0000
#define PERI_VIO_DBG_DMNID_START_BIT    18
#define PERI_VIO_ADDR_HIGH              0x3C000000
#define PERI_VIO_ADDR_HIGH_START_BIT    26
#define PERI_VIO_DBG_W                  0x40000000
#define PERI_VIO_DBG_W_START_BIT        30
/* PERI_VIO_DBG_R: write bit[31] to clear; read bit[31] to get value READ violation */
#define PERI_VIO_DBG_R                  0x80000000
#define PERI_VIO_DBG_R_START_BIT        31
#define PERI_VIO_DBG_CLR                0x80000000
#define PERI_VIO_DBG_CLR_START_BIT      31

/******************************************************************************
* REGISTER ADDRESS DEFINATION
******************************************************************************/

/* Device APC PD */
#define DEVAPC0_PD_INFRA_VIO_MASK(index)    ((unsigned int *)(devapc_pd_infra_base + 0x4 * index))
#define DEVAPC0_PD_INFRA_VIO_STA(index)     ((unsigned int *)(devapc_pd_infra_base + 0x0400 + 0x4 * index))

#define DEVAPC0_PD_INFRA_VIO_DBG0           ((unsigned int *)(devapc_pd_infra_base+0x0900))
#define DEVAPC0_PD_INFRA_VIO_DBG1           ((unsigned int *)(devapc_pd_infra_base+0x0904))

#define DEVAPC0_PD_INFRA_APC_CON            ((unsigned int *)(devapc_pd_infra_base+0x0F00))

#define PD_INFRA_VIO_MASK_MAX_REG_INDEX     10
#define PD_INFRA_VIO_STA_MAX_REG_INDEX      10


/* Device APC PD PERI */
#define DEVAPC0_PD_PERI_VIO_MASK(index)     ((unsigned int *)(devapc_pd_peri_base + 0x4 * index))
#define DEVAPC0_PD_PERI_VIO_STA(index)      ((unsigned int *)(devapc_pd_peri_base + 0x0400 + 0x4 * index))

#define DEVAPC0_PD_PERI_VIO_DBG0            ((unsigned int *)(devapc_pd_peri_base+0x0900))
#define DEVAPC0_PD_PERI_VIO_DBG1            ((unsigned int *)(devapc_pd_peri_base+0x0904))

#define DEVAPC0_PD_PERI_APC_CON             ((unsigned int *)(devapc_pd_peri_base+0x0F00))

#define PD_PERI_VIO_MASK_MAX_REG_INDEX      2
#define PD_PERI_VIO_STA_MAX_REG_INDEX       2



struct DEVICE_INFO {
	const char      *device;
	bool            enable_vio_irq;
};

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

#endif /* __DAPC_H__ */


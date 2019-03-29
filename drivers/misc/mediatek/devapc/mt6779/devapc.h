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

/* Debug message event */
#define DEVAPC_LOG_NONE        0x00000000
#define DEVAPC_LOG_INFO        0x00000001
#define DEVAPC_LOG_DBG         0x00000002

#define DEBUG
#ifdef DEBUG
#define DEVAPC_LOG_LEVEL	(DEVAPC_LOG_DBG)
#else
#define DEVAPC_LOG_LEVEL	(DEVAPC_LOG_NONE)
#endif

#define DEVAPC_DBG_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_LOG_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
		} \
	} while (0)


#define DEVAPC_VIO_LEVEL      (DEVAPC_LOG_INFO)

#define DEVAPC_MSG(fmt, args...) \
	do {    \
		if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_DBG) { \
			pr_debug(fmt, ##args); \
		} else if (DEVAPC_VIO_LEVEL & DEVAPC_LOG_INFO) { \
			pr_info(fmt, ##args); \
		} \
	} while (0)


#define MOD_NO_IN_1_DEVAPC                  16
#define DEVAPC_TAG                          "DEVAPC"

/* 1: Force to enable enhanced one-core violation debugging */
/* 0: Enhanced one-core violation debugging can be enabled dynamically */
/* Notice: You should only use one core to debug */
#ifdef DBG_ENABLE
	#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	1
#else
	#define DEVAPC_ENABLE_ONE_CORE_VIOLATION_DEBUG	0
#endif

#define DAPC_DEVICE_TREE_NODE_PD_INFRA_INDEX    0
#define DAPC_DEVICE_TREE_NODE_AO_INFRA_INDEX    1

/* Uncomment to enable AEE  */
#define DEVAPC_ENABLE_AEE			1


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
#define PD_INFRA_VIO_SHIFT_MAX_BIT      25
#define PD_INFRA_VIO_MASK_MAX_INDEX     525
#define PD_INFRA_VIO_STA_MAX_INDEX      525

#define DEVAPC_PD_INFRA_VIO_MASK(index) \
	((unsigned int *)(devapc_pd_infra_base + 0x4 * index))
#define DEVAPC_PD_INFRA_VIO_STA(index) \
	((unsigned int *)(devapc_pd_infra_base + 0x400 + 0x4 * index))

#define DEVAPC_PD_INFRA_VIO_DBG0 \
	((unsigned int *)(devapc_pd_infra_base+0x900))
#define DEVAPC_PD_INFRA_VIO_DBG1 \
	((unsigned int *)(devapc_pd_infra_base+0x904))

#define DEVAPC_PD_INFRA_APC_CON \
	((unsigned int *)(devapc_pd_infra_base+0xF00))

#define DEVAPC_PD_INFRA_VIO_SHIFT_STA \
	((unsigned int *)(devapc_pd_infra_base+0xF10))
#define DEVAPC_PD_INFRA_VIO_SHIFT_SEL \
	((unsigned int *)(devapc_pd_infra_base+0xF14))
#define DEVAPC_PD_INFRA_VIO_SHIFT_CON \
	((unsigned int *)(devapc_pd_infra_base+0xF20))

#define DEVAPC_TOTAL_SLAVES		478
#define DEVAPC_SRAMROM_VIO_INDEX	511
#define DEVAPC_MD_VIO			231
#define DEVAPC_MM_VIO_START		232

/******************************************************************************
 * DATA STRUCTURE & FUNCTION DEFINATION
 ******************************************************************************/

struct DEVICE_INFO {
	int		DEVAPC_SLAVE_TYPE;
	int		config_index;
	const char	*device;
	bool		enable_vio_irq;
};

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

enum DEVAPC_SLAVE_TYPE {
	E_DAPC_INFRA_PERI_SLAVE = 0,
	E_DAPC_MM_SLAVE,
	E_DAPC_MD_SLAVE,
	E_DAPC_PERI_SLAVE,
	E_DAPC_MM2ND_SLAVE,
	E_DAPC_OTHERS_SLAVE,
	E_DAPC_SLAVE_TYPE_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum E_MASK_DOM {
	E_DOMAIN_0 = 0,
	E_DOMAIN_1,
	E_DOMAIN_2,
	E_DOMAIN_3,
	E_DOMAIN_4,
	E_DOMAIN_5,
	E_DOMAIN_6,
	E_DOMAIN_7,
	E_DOMAIN_8,
	E_DOMAIN_9,
	E_DOMAIN_10,
	E_DOMAIN_11,
	E_DOMAIN_12,
	E_DOMAIN_13,
	E_DOMAIN_14,
	E_DOMAIN_15,
	E_DOMAIN_OTHERS,
	E_MASK_DOM_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

#endif /* __DAPC_H__ */

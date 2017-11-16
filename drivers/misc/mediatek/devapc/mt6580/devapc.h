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
/*For EMI API DEVAPC0_D0_VIO_STA_3, idx:103*/
#define ABORT_EMI                0x80
/*Define constants*/
#define DEVAPC_DEVICE_NUMBER    100
#define DEVAPC_DOMAIN_AP        0
#define DEVAPC_DOMAIN_MD        1
#define DEVAPC_DOMAIN_CONN      2
#define DEVAPC_DOMAIN_MM        3
#define VIO_DBG_MSTID 0x000001FF
#define VIO_DBG_DMNID 0x0000C000
#define VIO_DBG_RW    0x3000000
#define VIO_DBG_CLR   0x80000000

/******************************************************************************
*REGISTER ADDRESS DEFINATION
******************************************************************************/
#define DEVAPC0_D0_APC_0            (devapc_ao_base+0x0000)

#define DEVAPC0_D1_APC_0            (devapc_ao_base+0x0100)

#define DEVAPC0_D2_APC_0            (devapc_ao_base+0x0200)

#define DEVAPC0_D3_APC_0            (devapc_ao_base+0x0300)

#define DEVAPC0_MAS_DOM_0           ((unsigned int *)(devapc_ao_base+0x0400))
#define DEVAPC0_MAS_DOM_1           ((unsigned int *)(devapc_ao_base+0x0404))
#define DEVAPC0_MAS_SEC             ((unsigned int *)(devapc_ao_base+0x0500))
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
#define DEVAPC0_D0_VIO_STA_0        ((unsigned int *)(devapc_pd_base+0x0400))
#define DEVAPC0_D0_VIO_STA_1        ((unsigned int *)(devapc_pd_base+0x0404))
#define DEVAPC0_D0_VIO_STA_2        ((unsigned int *)(devapc_pd_base+0x0408))
#define DEVAPC0_D0_VIO_STA_3        ((unsigned int *)(devapc_pd_base+0x040C))
#define DEVAPC0_VIO_DBG0            ((unsigned int *)(devapc_pd_base+0x0900))
#define DEVAPC0_VIO_DBG1            ((unsigned int *)(devapc_pd_base+0x0904))
#define DEVAPC0_DEC_ERR_CON         ((unsigned int *)(devapc_pd_base+0x0F80))
#define DEVAPC0_DEC_ERR_ADDR        ((unsigned int *)(devapc_pd_base+0x0F84))
#define DEVAPC0_DEC_ERR_ID          ((unsigned int *)(devapc_pd_base+0x0F88))

struct DEVICE_INFO {
	const char      *device;
	bool            forbidden;
};

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
#endif

#endif


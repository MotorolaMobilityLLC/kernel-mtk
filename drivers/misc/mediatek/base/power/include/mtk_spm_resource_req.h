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

#ifndef __MTK_SPM_RESOURCE_REQ_H__
#define __MTK_SPM_RESOURCE_REQ_H__

/* SPM resource request APIs: public */

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6797)

#elif defined(CONFIG_ARCH_MT6735) || defined(CONFIG_ARCH_MT6735M) || defined(CONFIG_ARCH_MT6753)

#elif defined(CONFIG_ARCH_MT6580)

#elif defined(CONFIG_MACH_KIBOPLUS) || defined(CONFIG_MACH_MT6757)

#include "spm_v2/mtk_spm_resource_req.h"

#elif defined(CONFIG_MACH_MT6799) \
	|| defined(CONFIG_MACH_MT6758) \
	|| defined(CONFIG_MACH_MT6759) \
	|| defined(CONFIG_MACH_MT6775)

#include "spm_v3/mtk_spm_resource_req.h"

#elif defined(CONFIG_MACH_MT6763) || defined(CONFIG_MACH_MT6739)

#include "spm_v4/mtk_spm_resource_req.h"

#else

/* SPM resource request APIs: for bring up */

enum {
	SPM_RESOURCE_RELEASE = 0,
	SPM_RESOURCE_MAINPLL = 1 << 0,
	SPM_RESOURCE_DRAM    = 1 << 1,
	SPM_RESOURCE_CK_26M  = 1 << 2,
	SPM_RESOURCE_AXI_BUS = 1 << 3,
	SPM_RESOURCE_CPU     = 1 << 4,
	NF_SPM_RESOURCE = 5,

	SPM_RESOURCE_ALL = (1 << NF_SPM_RESOURCE) - 1
};

enum {
	SPM_RESOURCE_USER_SPM = 0,
	SPM_RESOURCE_USER_UFS,
	SPM_RESOURCE_USER_SSUSB,
	SPM_RESOURCE_USER_AUDIO,
	SPM_RESOURCE_USER_UART,
	SPM_RESOURCE_USER_CONN,
	SPM_RESOURCE_USER_MSDC,
	NF_SPM_RESOURCE_USER
};

static inline bool spm_resource_req(unsigned int user, unsigned int req_mask)
{
	return true;
}

#endif

#endif /* __MTK_SPM_RESOURCE_REQ_H__ */

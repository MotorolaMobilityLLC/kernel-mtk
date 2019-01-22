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
#if !defined(__ION_PROFILE_H__)
#define __ION_PROFILE_H__

enum ION_PROFILE_TYPE {
	PROFILE_ALLOC = 0,
	PROFILE_FREE,
	PROFILE_SHARE,
	PROFILE_IMPORT,
	PROFILE_MAP_KERNEL,
	PROFILE_UNMAP_KERNEL,
	PROFILE_MAP_USER,
	PROFILE_UNMAP_USER,
	PROFILE_CUSTOM_IOC,
	PROFILE_GET_PHYS,
	PROFILE_DMA_CLEAN_RANGE,
	PROFILE_DMA_FLUSH_RANGE,
	PROFILE_DMA_INVALID_RANGE,
	PROFILE_DMA_CLEAN_ALL,
	PROFILE_DMA_FLUSH_ALL,
	PROFILE_DMA_INVALID_ALL,
	PROFILE_MAX,
};

#define ION_PROFILE

#ifndef ION_PROFILE
/*
* avoid CamelCase, will modify in a letter
*#define MMProfileLogEx(...)
*#define MMProfileEnable(...)
*#define MMProfileStart(...)
*#define MMP_Event unsigned int
*#define MMP_RootEvent 1
*/
#else
#include <mmprofile.h>

void mmprofile_enable(int enable);
void mmprofile_start(int start);

void ion_profile_init(void);
#endif

/*extern MMP_Event ION_MMP_Events[PROFILE_MAX];*/

#endif

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

#ifndef __ION_DRV_PRIV_H__
#define __ION_DRV_PRIV_H__

#include "ion_priv.h"

/* STRUCT ION_HEAP *G_ION_HEAPS[ION_HEAP_IDX_MAX]; */

/* Import from multimedia heap */
long ion_mm_ioctl(struct ion_client *client, unsigned int cmd,
		unsigned long arg, int from_kernel);

void smp_inner_dcache_flush_all(void);


#ifdef ION_HISTORY_RECORD
extern int ion_history_init(void);
#else
static inline int ion_history_init(void)
{
	return 0;
}
#endif

int ion_mm_heap_for_each_pool(int (*fn)(int high, int order, int cache, size_t size));
struct ion_heap *ion_drv_get_heap(struct ion_device *dev, int heap_id, int need_lock);
int ion_drv_create_heap(struct ion_platform_heap *heap_data);

#endif

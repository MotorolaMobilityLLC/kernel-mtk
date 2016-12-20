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

#ifndef __ION_SEC_HEAP_H__
#define __ION_SEC_HEAP_H__

struct ion_sec_buffer_info {
	struct mutex lock;/*mutex lock on secure buffer*/
	int module_id;
	unsigned int security;
	unsigned int coherent;
	void *VA;
	unsigned int MVA;
	ion_phys_addr_t priv_phys;
	struct ion_mm_buf_debug_info dbg_info;
	struct ion_mm_sf_buf_info sf_buf_info;
};

#endif

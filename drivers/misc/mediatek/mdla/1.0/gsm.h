/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __GSM_H__
#define __GSM_H__

#include <linux/types.h>
#include <linux/sizes.h>

#define GSM_BASE (0x1d000000)
#define GSM_MVA_BASE (0x80000000)

void *gsm_alloc(size_t size);
void *gsm_mva_to_virt(u32 mva);
u32 gsm_virt_to_mva(void *vaddr);
int gsm_release(void *vaddr, size_t size);

#define is_gsm_addr(a) \
	(likely(((a) >= GSM_MVA_BASE) && ((a) < GSM_MVA_BASE) + SZ_1M))

#endif /* KMOD_GSM_H_ */


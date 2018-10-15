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

#ifndef __DEVMPU_MT_H__
#define __DEVMPU_MT_H__

/* platform config. */
#define DEVMPU_DRAM_BASE     0x40000000UL
#define DEVMPU_DRAM_SIZE     0x200000000ULL  /* protect 8GB DRAM */
#define DEVMPU_PAGE_SIZE     0x00200000UL
#define DEVMPU_PAGE_NUM      (DEVMPU_DRAM_SIZE / DEVMPU_PAGE_SIZE)
#define DEVMPU_ALIGN_BITS    __builtin_ctz(DEVMPU_PAGE_SIZE)

#endif

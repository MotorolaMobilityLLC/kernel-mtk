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

#ifndef __MTK_MEMINFO_H__
#define __MTK_MEMINFO_H__

/* physical offset */
extern phys_addr_t get_phys_offset(void);
/* physical DRAM size */
extern phys_addr_t get_max_DRAM_size(void);
/* DRAM size controlled by kernel */
extern phys_addr_t get_memory_size(void);
extern phys_addr_t mtk_get_max_DRAM_size(void);
extern phys_addr_t get_zone_movable_cma_base(void);
extern phys_addr_t get_zone_movable_cma_size(void);
#ifdef CONFIG_MTK_MEMORY_LOWPOWER
extern phys_addr_t memory_lowpower_cma_base(void);
extern phys_addr_t memory_lowpower_cma_size(void);
#endif /* end CONFIG_MTK_MEMORY_LOWPOWER */

#endif /* end __MTK_MEMINFO_H__ */

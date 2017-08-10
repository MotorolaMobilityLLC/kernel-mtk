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
enum dcs_status {
	DCS_NORMAL,
	DCS_LOWPOWER,
	DCS_BUSY,
	DCS_NR_STATUS,
};
#ifdef CONFIG_MTK_DCS
extern int dcs_dram_channel_switch(enum dcs_status status);
extern int dcs_get_channel_num_trylock(int *num);
extern void dcs_get_channel_num_unlock(void);
extern bool dcs_initialied(void);
#else
static inline int dcs_dram_channel_switch(enum dcs_status status) { return 0; }
static inline int dcs_get_channel_num_trylock(int *num) { return 0; }
static inline void dcs_get_channel_num_unlock(void) {}
static inline bool dcs_initialied(void) { return true; }
#endif /* CONFIG_MTK_DCS */

#endif /* end __MTK_MEMINFO_H__ */

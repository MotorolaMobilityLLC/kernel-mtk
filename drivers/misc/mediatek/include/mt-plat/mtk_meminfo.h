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
#include <linux/cma.h>

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
extern struct single_cma_registration memory_lowpower_registration;
#endif /* end CONFIG_MTK_MEMORY_LOWPOWER */

#ifdef CONFIG_ZONE_MOVABLE_CMA
extern phys_addr_t zmc_base(void);
extern struct page *zmc_cma_alloc(struct cma *cma, int count, unsigned int align);
extern bool zmc_cma_release(struct cma *cma, struct page *pages, int count);

extern int zmc_register_client(struct notifier_block *nb);
extern int zmc_unregister_client(struct notifier_block *nb);
extern int zmc_notifier_call_chain(unsigned long val, void *v);

struct single_cma_registration {
	phys_addr_t size;
	phys_addr_t align;
	const char *name;
	void (*init)(struct cma *);
};

#define ZMC_EVENT_ALLOC_MOVABLE 0x01
#endif

#ifdef CONFIG_MTK_SVP
extern phys_addr_t memory_ssvp_cma_base(void);
extern phys_addr_t memory_ssvp_cma_size(void);
extern struct single_cma_registration memory_ssvp_registration;
#endif /* end CONFIG_MTK_MEMORY_LOWPOWER */

enum dcs_status {
	DCS_NORMAL,
	DCS_LOWPOWER,
	DCS_BUSY,
	DCS_NR_STATUS,
};
#ifdef CONFIG_MTK_DCS
extern int dcs_dram_channel_switch(enum dcs_status status);
extern int dcs_get_dcs_status_lock(int *ch, enum dcs_status *status);
extern int dcs_get_dcs_status_trylock(int *ch, enum dcs_status *status);
extern void dcs_get_dcs_status_unlock(void);
extern bool dcs_initialied(void);
#else
static inline int dcs_dram_channel_switch(enum dcs_status status) { return 0; }
static inline int dcs_get_dcs_status_lock(int *ch, enum dcs_status *status)
{
	*ch = -1;
	*status = DCS_BUSY;
	return -EBUSY;
}
static inline int dcs_get_dcs_status_trylock(int *ch, enum dcs_status *status)
{
	*ch = -1;
	*status = DCS_BUSY;
	return -EBUSY;
}
static inline void dcs_get_dcs_status_unlock(void) {}
static inline bool dcs_initialied(void) { return true; }
#endif /* CONFIG_MTK_DCS */

#endif /* end __MTK_MEMINFO_H__ */

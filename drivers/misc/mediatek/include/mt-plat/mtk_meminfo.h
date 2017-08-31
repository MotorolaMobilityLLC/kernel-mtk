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
#define ZMC_ALLOC_ALL 0x01 /* allocate all memory reserved from dts */

/* Priority of ZONE_MOVABLE_CMA users */
enum zmc_prio {
	ZMC_SSVP,
	ZMC_CHECK_MEM_STAT,
	ZMC_MLP = ZMC_CHECK_MEM_STAT,
	NR_ZMC_OWNER,
};

struct single_cma_registration {
	phys_addr_t size;
	phys_addr_t align;
	unsigned long flag;
	const char *name;
	void (*init)(struct cma *);
	enum zmc_prio prio;
	bool reserve_fail;
};

extern phys_addr_t zmc_base(void);
extern struct page *zmc_cma_alloc(struct cma *cma, int count, unsigned int align, struct single_cma_registration *p);
extern bool zmc_cma_release(struct cma *cma, struct page *pages, int count);

extern int zmc_register_client(struct notifier_block *nb);
extern int zmc_unregister_client(struct notifier_block *nb);
extern int zmc_notifier_call_chain(unsigned long val, void *v);


#define ZMC_EVENT_ALLOC_MOVABLE 0x01
#endif

#ifdef CONFIG_MTK_SVP
extern phys_addr_t memory_ssvp_cma_base(void);
extern phys_addr_t memory_ssvp_cma_size(void);
extern struct single_cma_registration memory_ssvp_registration;
#endif /* end CONFIG_MTK_MEMORY_LOWPOWER */

#ifdef CONFIG_MTK_DCS
enum dcs_status {
	DCS_NORMAL,
	DCS_LOWPOWER,
	DCS_BUSY,
	DCS_NR_STATUS,
};
enum dcs_kicker {
	DCS_KICKER_MHL,
	DCS_KICKER_PERF,
	DCS_KICKER_DEBUG,
	DCS_NR_KICKER,
};
extern int dcs_enter_perf(enum dcs_kicker kicker);
extern int dcs_exit_perf(enum dcs_kicker kicker);
extern int dcs_get_dcs_status_lock(int *ch, enum dcs_status *status);
extern int dcs_get_dcs_status_trylock(int *ch, enum dcs_status *status);
extern void dcs_get_dcs_status_unlock(void);
extern bool dcs_initialied(void);
extern int dcs_full_init(void);
extern char * const dcs_status_name(enum dcs_status status);
extern int dcs_set_lbw_region(u64 start, u64 end);
extern int dcs_mpu_protection(int enable);
/* DO _NOT_ USE APIS below UNLESS YOU KNOW HOW TO USE THEM */
extern int __dcs_get_dcs_status(int *ch, enum dcs_status *dcs_status);
extern int dcs_switch_to_lowpower(void);
#endif /* end CONFIG_MTK_DCS */
#endif /* end __MTK_MEMINFO_H__ */

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

#ifndef __MT_HIBERNATE_H__
#define __MT_HIBERNATE_H__

#include <linux/suspend.h>

#define POWERMODE_HIBERNATE 0xadfeefda

int notrace swsusp_arch_save_image(unsigned long unused);

extern void mt_save_generic_timer(unsigned int *container, int sw);
extern void mt_restore_generic_timer(unsigned int *container, int sw);
extern void mt_save_banked_registers(unsigned int *container);
extern void mt_restore_banked_registers(unsigned int *container);
#ifdef CONFIG_MTK_ETM
extern void trace_start_dormant(void);
#endif

extern void __disable_dcache__inner_flush_dcache_L1__inner_flush_dcache_L2(void);
extern void call_with_stack(void (*fn)(void *), void *arg, void *sp);

#ifdef CONFIG_MTK_HIBERNATION
extern bool system_is_hibernating;
extern int mtk_hibernate_via_autosleep(suspend_state_t *autoslp_state);
extern void notrace mtk_save_processor_state(void);
extern void notrace mtk_restore_processor_state(void);
extern void notrace mtk_arch_restore_image(void);
#endif

#endif /* __MT_HIBERNATE_H__ */

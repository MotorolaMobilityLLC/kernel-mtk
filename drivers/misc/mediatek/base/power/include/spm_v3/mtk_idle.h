/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __MTK_IDLE_H__
#define __MTK_IDLE_H__

#include <linux/notifier.h>

enum idle_lock_spm_id {
	IDLE_SPM_LOCK_VCORE_DVFS = 0,
};

#ifdef CONFIG_CPU_IDLE
extern void idle_lock_spm(enum idle_lock_spm_id id);
extern void idle_unlock_spm(enum idle_lock_spm_id id);

extern void enable_dpidle_by_bit(int id);
extern void disable_dpidle_by_bit(int id);

extern void enable_soidle3_by_bit(int id);
extern void disable_soidle3_by_bit(int id);

extern void enable_soidle_by_bit(int id);
extern void disable_soidle_by_bit(int id);
#else
void idle_lock_spm(enum idle_lock_spm_id id) {}
void idle_unlock_spm(enum idle_lock_spm_id id) {}

void enable_dpidle_by_bit(int id) {}
void disable_dpidle_by_bit(int id) {}

void enable_soidle3_by_bit(int id) {}
void disable_soidle3_by_bit(int id) {}

void enable_soidle_by_bit(int id) {}
void disable_soidle_by_bit(int id) {}
#endif

extern void enable_mcsodi_by_bit(int id);
extern void disable_mcsodi_by_bit(int id);

#define DPIDLE_START    1
#define DPIDLE_END      2
#define SOIDLE_START    3
#define SOIDLE_END      4
extern int mtk_idle_notifier_register(struct notifier_block *n);
extern void mtk_idle_notifier_unregister(struct notifier_block *n);

extern void idle_lock_by_ufs(unsigned int lock);
extern void idle_lock_by_gpu(unsigned int lock);

#endif

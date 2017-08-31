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

#ifndef __MTK_CPUIDLE_H__
#define __MTK_CPUIDLE_H__

#include <asm/arch_timer.h>

enum mtk_cpuidle_mode {
	MTK_MCDI_MODE = 1,
	MTK_SODI_MODE,
	MTK_SODI3_MODE,
	MTK_DPIDLE_MODE,
	MTK_SUSPEND_MODE,
};

int mtk_cpuidle_init(void);
int mtk_enter_idle_state(int idx);

#define MTK_CPUIDLE_TIME_PROFILING 0

#ifdef CONFIG_MTK_RAM_CONSOLE
#define aee_addr(cpu) (mtk_cpuidle_aee_virt_addr + (cpu << 2))
#define mtk_cpuidle_footprint_log(cpu, idx) (				\
{									\
	writel_relaxed(readl_relaxed(aee_addr(cpu)) | (1 << idx),	\
		       aee_addr(cpu));					\
}									\
)
#define mtk_cpuidle_footprint_clr(cpu) (				\
{									\
	writel_relaxed(0, aee_addr(cpu));				\
}									\
)
#else
#define mtk_cpuidle_footprint(cpu, idx)
#define mtk_cpuidle_footprint_clr(cpu)
#endif

#if MTK_CPUIDLE_TIME_PROFILING
#define MTK_CPUIDLE_TIMESTAMP_COUNT 16

#define mtk_cpuidle_timestamp_log(cpu, idx) (				\
{									\
	mtk_cpuidle_timestamp[cpu][idx] = arch_counter_get_cntvct();	\
}									\
)
#else
#define mtk_cpuidle_timestamp_log(cpu, idx)
#endif

void __weak switch_armpll_ll_hwmode(int enable) { }
void __weak switch_armpll_l_hwmode(int enable) { }
void mt_save_generic_timer(unsigned int *container, int sw);
void mt_restore_generic_timer(unsigned int *container, int sw);
void write_cntpctl(int cntpctl);
int read_cntpctl(void);
extern unsigned long *aee_rr_rec_mtk_cpuidle_footprint_va(void);
extern unsigned long *aee_rr_rec_mtk_cpuidle_footprint_pa(void);

#endif

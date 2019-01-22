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

#ifndef __MTK_IDLE_INTERNAL_H__
#define __MTK_IDLE_INTERNAL_H__
#include <linux/io.h>

/*
 * Chip specific declaratinos
 */
#if defined(CONFIG_MACH_MT6799)
#include "mtk_idle_mt6799.h"
#elif defined(CONFIG_MACH_MT6759)
#include "mtk_idle_mt6759.h"
#endif

/*
 * Common declarations
 */
enum mt_idle_mode {
	MT_DPIDLE = 0,
	MT_SOIDLE,
	MT_MCIDLE,
	MT_SLIDLE
};

/* IDLE_TYPE is used for idle_switch in mt_idle.c */
enum {
	IDLE_TYPE_DP = 0,
	IDLE_TYPE_SO3,
	IDLE_TYPE_SO,
	IDLE_TYPE_MC,
	IDLE_TYPE_MCSO,
	IDLE_TYPE_SL,
	IDLE_TYPE_RG,
	NR_TYPES,
};

/* CPUIDLE_STATE is used to represent CPUidle C States */
enum {
	CPUIDLE_STATE_RG = 0,
	CPUIDLE_STATE_SL,
	CPUIDLE_STATE_MCSO,
	CPUIDLE_STATE_MC,
	CPUIDLE_STATE_SO,
	CPUIDLE_STATE_DP,
	CPUIDLE_STATE_SO3,
	NR_CPUIDLE_STATE
};

enum {
	BY_CPU = 0,
	BY_CLK,
	BY_TMR,
	BY_OTH,
	BY_VTG,
	BY_FRM,
	BY_PLL,
	BY_PWM,
	BY_DCS,
	BY_UFS,
	BY_GPU,
	BY_SSPM_IPI,
	NR_REASONS,
};

#define INVALID_GRP_ID(grp)     (grp < 0 || grp >= NR_GRPS)
#define INVALID_IDLE_ID(id)     (id < 0 || id >= NR_TYPES)
#define INVALID_REASON_ID(id)   (id < 0 || id >= NR_REASONS)

#define idle_readl(addr)	    __raw_readl(addr)

extern unsigned int dpidle_blocking_stat[NR_GRPS][32];
extern int idle_switch[NR_TYPES];

extern unsigned int idle_condition_mask[NR_TYPES][NR_GRPS];

extern unsigned int soidle3_pll_condition_mask[NR_PLLS];

/*
 * Function Declarations
 */
const char *mtk_get_idle_name(int id);
const char *mtk_get_reason_name(int);
const char *mtk_get_cg_group_name(int id);
const char *mtk_get_pll_group_name(int id);

bool mtk_idle_check_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD]);
bool mtk_idle_check_secure_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD]);
bool mtk_idle_check_pll(unsigned int *condition_mask, unsigned int *block_mask);
bool mtk_idle_check_clkmux(int idle_type,
							unsigned int block_mask[NR_TYPES][NF_CLK_CFG]);

void __init iomap_init(void);

bool mtk_idle_disp_is_pwm_rosc(void);

#endif /* __MTK_IDLE_INTERNAL_H__ */


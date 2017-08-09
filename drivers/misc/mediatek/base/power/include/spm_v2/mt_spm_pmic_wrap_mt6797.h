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

#ifndef __MT_SPM_PMIC_WRAP_H__
#define __MT_SPM_PMIC_WRAP_H__
/* Phase */
enum pmic_wrap_phase_id {
	PMIC_WRAP_PHASE_NORMAL,	/* as VCORE_DVFS */
	PMIC_WRAP_PHASE_SUSPEND,
	PMIC_WRAP_PHASE_DEEPIDLE,
	NR_PMIC_WRAP_PHASE,
};

/* IDX mapping */
enum {
	IDX_NM_VCORE_HPM,		/* 0 */		/* PMIC_WRAP_PHASE_NORMAL */
	IDX_NM_VCORE_TRANS4,		/* 1 */
	IDX_NM_VCORE_TRANS3,		/* 2 */
	IDX_NM_VCORE_LPM,		/* 3 */
	IDX_NM_VCORE_TRANS2,		/* 4 */
	IDX_NM_VCORE_TRANS1,		/* 5 */
	IDX_NM_VCORE_SCREEN_OFF_MODE,	/* 6 */
	IDX_NM_VSRAM_CORE_0P9V = 10,	/* A */
	IDX_NM_VSRAM_CORE_1P0V,		/* B */
	IDX_NM_VSRAM_CORE_1P1V,		/* C */
	IDX_NM_PWM_MODE,		/* D */
	IDX_NM_AUTO_MODE,		/* E */
	NR_IDX_NM,
};
enum {
	IDX_SP_VCORE_HPM,		/* 0 */		/* PMIC_WRAP_PHASE_SUSPEND */
	IDX_SP_VCORE_TRANS4,		/* 1 */
	IDX_SP_VCORE_TRANS3,		/* 2 */
	IDX_SP_VCORE_LPM,		/* 3 */
	IDX_SP_VCORE_TRANS2,		/* 4 */
	IDX_SP_VCORE_TRANS1,		/* 5 */
	IDX_SP_VCORE_SCREEN_OFF_MODE,	/* 6 */
	IDX_SP_VSRAM_CORE_0P9V = 10,	/* A */
	IDX_SP_VSRAM_CORE_1P0V,		/* B */
	IDX_SP_VSRAM_CORE_1P1V,		/* C */
	IDX_SP_VCORE_LQ_EN,		/* D */
	IDX_SP_VCORE_LQ_DIS,		/* E */
	NR_IDX_SP,
};
enum {
	IDX_DI_VCORE_HPM,		/* 0 */		/* PMIC_WRAP_PHASE_DEEPIDLE */
	IDX_DI_VCORE_TRANS4,		/* 1 */
	IDX_DI_VCORE_TRANS3,		/* 2 */
	IDX_DI_VCORE_LPM,		/* 3 */
	IDX_DI_VCORE_TRANS2,		/* 4 */
	IDX_DI_VCORE_TRANS1,		/* 5 */
	IDX_DI_VCORE_SCREEN_OFF_MODE,	/* 6 */
	IDX_DI_SRCCLKEN_IN2_NORMAL,	/* 7 */
	IDX_DI_SRCCLKEN_IN2_SLEEP,	/* 8 */
	IDX_DI_VCORE_DPIDLE,  /* 9 */
	IDX_DI_VSRAM_CORE_0P9V = 10,	/* A */
	IDX_DI_VSRAM_CORE_1P0V,		/* B */
	IDX_DI_VSRAM_CORE_1P1V,		/* C */
	IDX_DI_VCORE_LQ_EN,		/* D */
	IDX_DI_VCORE_LQ_DIS,		/* E */
	NR_IDX_DI,
};

#endif				/* __MT_SPM_PMIC_WRAP_H__ */

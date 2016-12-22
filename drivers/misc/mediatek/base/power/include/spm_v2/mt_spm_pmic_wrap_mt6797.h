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
	IDX_NM_VCORE_TRANS1,		/* 1 */
	IDX_NM_VCORE_TRANS2,		/* 2 */
	IDX_NM_VCORE_TRANS3,		/* 3 */
	IDX_NM_VCORE_TRANS4,		/* 4 */
	IDX_NM_VCORE_TRANS5,		/* 5 */
	IDX_NM_VCORE_LPM,		/* 6 */
	IDX_NM_VCORE_TRANS6,		/* 7 */
	IDX_NM_VCORE_TRANS7,		/* 8 */
	IDX_NM_VCORE_ULPM,		/* 9 */
	IDX_NM_PWM_MODE,		/* A */
	IDX_NM_AUTO_MODE,		/* B */
	IDX_NM_PWM_BEF1,		/* C */
	IDX_NM_PWM_BEF2,		/* D */
	IDX_NM_AUTO_AFT1,		/* E */
	IDX_NM_AUTO_AFT2,		/* F */
	NR_IDX_NM,
};
enum {
	IDX_SP_VCORE_HPM,		/* 0 */		/* PMIC_WRAP_PHASE_SUSPEND */
	IDX_SP_VCORE_TRANS1,		/* 1 */
	IDX_SP_VCORE_TRANS2,		/* 2 */
	IDX_SP_VCORE_TRANS3,		/* 3 */
	IDX_SP_VCORE_TRANS4,		/* 4 */
	IDX_SP_VCORE_TRANS5,		/* 5 */
	IDX_SP_VCORE_LPM,		/* 6 */
	IDX_SP_VCORE_VOSEL_0P6V,	/* 7*/
	IDX_SP_VCORE_VSLEEP_SEL_0P6V,	/* 8 */
	IDX_SP_VCORE_DPIDLE_SODI,	/* 9 */
	IDX_SP_VSRAM_CORE_1P0V,		/* A */
	IDX_SP_VSRAM_CORE_1P1V,		/* B */
	IDX_SP_VCORE_LQ_EN,		/* C */
	IDX_SP_VCORE_LQ_DIS,		/* D */
	IDX_SP_VCORE_VOSEL_0P7V,	/* E */
	IDX_SP_VCORE_VSLEEP_SEL_0P7V,	/* F */
	NR_IDX_SP,
};
enum {
	IDX_DI_VCORE_HPM,		/* 0 */		/* PMIC_WRAP_PHASE_DEEPIDLE */
	IDX_DI_VCORE_TRANS1,		/* 1 */
	IDX_DI_VCORE_TRANS2,		/* 2 */
	IDX_DI_VCORE_TRANS3,		/* 3 */
	IDX_DI_VCORE_TRANS4,		/* 4 */
	IDX_DI_VCORE_TRANS5,		/* 5 */
	IDX_DI_VCORE_LPM,		/* 6 */
	IDX_DI_SRCCLKEN_IN2_NORMAL,	/* 7 */
	IDX_DI_SRCCLKEN_IN2_SLEEP,	/* 8 */
	IDX_DI_VCORE_DPIDLE_SODI,	/* 9 */
	IDX_DI_VSRAM_CORE_1P0V,		/* A */
	IDX_DI_VSRAM_CORE_1P1V,		/* B */
	IDX_DI_VCORE_LQ_EN,		/* C */
	IDX_DI_VCORE_LQ_DIS,		/* D */
	NR_IDX_DI,
};

void mt_spm_update_pmic_wrap(void);

#endif				/* __MT_SPM_PMIC_WRAP_H__ */

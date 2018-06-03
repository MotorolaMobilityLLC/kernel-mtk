
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

/**
 * @file	mtk_eem_internal.c
 * @brief   Driver for EEM
 *
 */
#define __MTK_EEM_INTERNAL_C__

#include "mtk_eem_config.h"
#include "mtk_eem.h"

#if !(EEM_ENABLE_TINYSYS_SSPM)
	#include "mtk_eem_internal_ap.h"
#else
	#include "mtk_eem_internal_sspm.h"
#endif

#include "mtk_eem_internal.h"

/**
 * EEM controllers
 */
struct eem_ctrl eem_ctrls[NR_EEM_CTRL] = {
	[EEM_CTRL_BIG] = {
		.name = __stringify(EEM_CTRL_BIG),
		.det_id = EEM_DET_BIG,
	},

	[EEM_CTRL_CCI] = {
		.name = __stringify(EEM_CTRL_CCI),
		.det_id = EEM_DET_CCI,
	},

	[EEM_CTRL_GPU] = {
		.name = __stringify(EEM_CTRL_GPU),
		.det_id = EEM_DET_GPU,
	},

	[EEM_CTRL_2L] = {
		.name = __stringify(EEM_CTRL_2L),
		.det_id = EEM_DET_2L,
	},

	[EEM_CTRL_L] = {
		.name = __stringify(EEM_CTRL_L),
		.det_id = EEM_DET_L,
	},
	#if EEM_BANK_SOC
	[EEM_CTRL_SOC] = {
		.name = __stringify(EEM_CTRL_SOC),
		.det_id = EEM_DET_SOC,
	}
	#endif

};

#define BASE_OP(fn)	.fn = base_ops_ ## fn
struct eem_det_ops eem_det_base_ops = {
	#if !(EEM_ENABLE_TINYSYS_SSPM)
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_table),
	BASE_OP(get_orig_volt_table),
	#endif
	/* platform independent code */
	BASE_OP(volt_2_pmic),
	BASE_OP(volt_2_eem),
	BASE_OP(pmic_2_volt),
	BASE_OP(eem_2_pmic),
};

struct eem_det eem_detectors[NR_EEM_DET] = {
	[EEM_DET_BIG] = {
		.name		= __stringify(EEM_DET_BIG),
		.ops		= &big_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset	= 0,
		#endif
		.ctrl_id	= EEM_CTRL_BIG,
		#ifdef CONFIG_BIG_OFF
		.features	= 0,
		#else
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		#endif
		.max_freq_khz	= 2548000,/* 2496Mhz */
		.VBOOT		= VBOOT_VAL,
		.VMAX		= VMAX_VAL,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= BIG_EEM_BASE,
		.eem_step	= BIG_EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,
	},

	[EEM_DET_CCI] = {
		.name		= __stringify(EEM_DET_CCI),
		.ops		= &cci_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset = 0,
		#endif
		.ctrl_id	= EEM_CTRL_CCI,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1391000,/* 1118 MHz */
		.VBOOT		= VBOOT_VAL, /* 10uV */
		.VMAX		= VMAX_VAL,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,
	},

	[EEM_DET_GPU] = {
		.name		= __stringify(EEM_DET_GPU),
		.ops		= &gpu_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset	= 0,
		#endif
		.ctrl_id	= EEM_CTRL_GPU,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 850000,/* 850 MHz */
		.VBOOT		= VBOOT_VAL, /* 10uV */
		.VMAX		= VMAX_VAL_GPU,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= EEM_V_BASE,
		.eem_step	= EEM_STEP,
		.pmic_base	= GPU_PMIC_BASE,
		.pmic_step	= GPU_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL_GPU,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,

	},

	[EEM_DET_2L] = {
		.name		= __stringify(EEM_DET_2L),
		.ops		= &dual_little_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset	= 0,
		#endif
		.ctrl_id	= EEM_CTRL_2L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1638000,/* 1599 MHz */
		.VBOOT		= VBOOT_VAL, /* 10uV */
		.VMAX		= VMAX_VAL,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,
	},

	[EEM_DET_L] = {
		.name		= __stringify(EEM_DET_L),
		.ops		= &little_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset	= 0,
		#endif
		.ctrl_id	= EEM_CTRL_L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2340000,/* 2249 MHz */
		.VBOOT		= VBOOT_VAL, /* 10uV */
		.VMAX		= VMAX_VAL,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= CPU_PMIC_BASE,
		.pmic_step	= CPU_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,
	},

	#if EEM_BANK_SOC
	[EEM_DET_SOC] = {
		.name		= __stringify(EEM_DET_SOC),
		.ops		= &soc_det_ops,
		#if !(EEM_ENABLE_TINYSYS_SSPM)
		.volt_offset	= 0,
		#endif
		.ctrl_id	= EEM_CTRL_SOC,
		.features	= FEA_INIT01 | FEA_INIT02,
		.max_freq_khz	= 100,/* 800 MHz */
		.VBOOT		= VBOOT_VAL_SOC, /* 10uV */
		.VMAX		= VMAX_VAL_SOC,
		.VMIN		= VMIN_VAL,
		.eem_v_base	= EEM_V_BASE,
		.eem_step   = EEM_STEP,
		.pmic_base	= VCORE_PMIC_BASE,
		.pmic_step	= VCORE_PMIC_STEP,
		.DETWINDOW	= DETWINDOW_VAL,
		.DTHI		= DTHI_VAL,
		.DTLO		= DTLO_VAL,
		.DETMAX		= DETMAX_VAL,
		.AGECONFIG	= AGECONFIG_VAL,
		.AGEM		= AGEM_VAL,
		.DVTFIXED	= DVTFIXED_VAL,
		.VCO		= VCO_VAL,
		.DCCONFIG	= DCCONFIG_VAL,
	}
	#endif

};
#undef __MT_EEM_INTERNAL_C__

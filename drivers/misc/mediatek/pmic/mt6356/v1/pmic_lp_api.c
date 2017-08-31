/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
#include <mt-plat/upmu_common.h>
#include "include/pmic.h"
#include "include/pmic_api_buck.h"

/* ---------------------------------------------------------------  */
/* pmic_<type>_<name>_lp (<user>, <op_en>, <op_cfg>)                */
/* parameter                                                        */
/* <type>   : BUCK / LDO                                            */
/* <name>   : BUCK name / LDO name                                  */
/* <user>   : SRCLKEN0 / SRCLKEN1 / SRCLKEN2 / SRCLKEN3 / SW / SPM  */
/* <op_en>  : user control enable                                   */
/* <op_cfg> :                                                       */
/*     HW mode :                                                    */
/*            0 : define as ON/OFF control                          */
/*            1 : define as LP/No LP control                        */
/*     SW/SPM :                                                     */
/*            0 : OFF                                               */
/*            1 : ON                                                */
/* ---------------------------------------------------------------  */
#define PMIC_LP_BUCK1_ENTRY(reg) {reg, MT6356_BUCK_##reg##_OP_EN, MT6356_BUCK_##reg##_OP_CFG, MT6356_BUCK_##reg##_CON0}
#define PMIC_LP_LDO1_ENTRY(reg) {reg, MT6356_LDO_##reg##_OP_EN, MT6356_LDO_##reg##_OP_CFG, MT6356_LDO_##reg##_CON0}
#define PMIC_LP_LDO2_ENTRY(reg) {reg, MT6356_LDO_##reg##_OP_EN, MT6356_LDO_##reg##_OP_CFG \
				    , MT6356_LDO_##reg##_CON0_0, MT6356_LDO_##reg##_CON0_1 \
				}
#define PMIC_LP_BUCK3_ENTRY(reg) {reg, MT6356_BUCK_##reg##_CON0}

const PMU_LP_TABLE1_ENTRY pmu_lp_table1[] = {
	PMIC_LP_BUCK1_ENTRY(VPROC),
	PMIC_LP_BUCK1_ENTRY(VCORE),
	PMIC_LP_BUCK1_ENTRY(VMODEM),
	PMIC_LP_BUCK1_ENTRY(VS1),
	PMIC_LP_BUCK1_ENTRY(VS2),
	PMIC_LP_LDO1_ENTRY(VSRAM_PROC),
	PMIC_LP_LDO1_ENTRY(VSRAM_GPU),
	PMIC_LP_LDO1_ENTRY(VSRAM_OTHERS),
	PMIC_LP_LDO1_ENTRY(VFE28),
	PMIC_LP_LDO1_ENTRY(VXO22),
	PMIC_LP_LDO1_ENTRY(VRF18),
	PMIC_LP_LDO1_ENTRY(VRF12),
	PMIC_LP_LDO1_ENTRY(VMIPI),
	PMIC_LP_LDO1_ENTRY(VCN28),
	PMIC_LP_LDO1_ENTRY(VCN18),
	PMIC_LP_LDO1_ENTRY(VCAMA),
	PMIC_LP_LDO1_ENTRY(VCAMD),
	PMIC_LP_LDO1_ENTRY(VCAMIO),
	PMIC_LP_LDO1_ENTRY(VA12),
	PMIC_LP_LDO1_ENTRY(VAUX18),
	PMIC_LP_LDO1_ENTRY(VAUD28),
	PMIC_LP_LDO1_ENTRY(VIO28),
	PMIC_LP_LDO1_ENTRY(VIO18),
	PMIC_LP_LDO1_ENTRY(VDRAM),
	PMIC_LP_LDO1_ENTRY(VMC),
	PMIC_LP_LDO1_ENTRY(VMCH),
	PMIC_LP_LDO1_ENTRY(VEMC),
	PMIC_LP_LDO1_ENTRY(VSIM1),
	PMIC_LP_LDO1_ENTRY(VSIM2),
	PMIC_LP_LDO1_ENTRY(VIBR),
	PMIC_LP_LDO1_ENTRY(VBIF28),
};

const PMU_LP_TABLE2_ENTRY pmu_lp_table2[] = {
	PMIC_LP_LDO2_ENTRY(VCN33),
	PMIC_LP_LDO2_ENTRY(VLDO28),
	PMIC_LP_LDO2_ENTRY(VUSB),
};

const PMU_LP_TABLE3_ENTRY pmu_lp_table3[] = {
	PMIC_LP_BUCK3_ENTRY(VPA),
};

#define pmic_set_sw_en(addr, val)       pmic_config_interface_nolock(addr, val, 1, 0)
#define pmic_set_sw_lp(addr, val)       pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_sw_op_en(addr, val)        pmic_config_interface_nolock(addr, val, 1, 0)
#define pmic_set_hw0_op_en(addr, val)       pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_hw1_op_en(addr, val)       pmic_config_interface_nolock(addr, val, 1, 2)
#define pmic_set_hw2_op_en(addr, val)       pmic_config_interface_nolock(addr, val, 1, 3)
#define pmic_set_hw3_op_en(addr, val)       pmic_config_interface_nolock(addr, val, 1, 4)
#define pmic_set_hw0_op_cfg(addr, val)      pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_hw1_op_cfg(addr, val)      pmic_config_interface_nolock(addr, val, 1, 2)
#define pmic_set_hw2_op_cfg(addr, val)      pmic_config_interface_nolock(addr, val, 1, 3)
#define pmic_set_hw3_op_cfg(addr, val)      pmic_config_interface_nolock(addr, val, 1, 4)
#define pmic_get_sw_op_en(addr, pval)       pmic_read_interface_nolock(addr, pval, 1, 0)
#define pmic_get_hw0_op_en(addr, pval)      pmic_read_interface_nolock(addr, pval, 1, 1)
#define pmic_get_hw1_op_en(addr, pval)      pmic_read_interface_nolock(addr, pval, 1, 2)
#define pmic_get_hw2_op_en(addr, pval)      pmic_read_interface_nolock(addr, pval, 1, 3)
#define pmic_get_hw3_op_en(addr, pval)      pmic_read_interface_nolock(addr, pval, 1, 4)
#define pmic_get_hw0_op_cfg(addr, pval)     pmic_read_interface_nolock(addr, pval, 1, 1)
#define pmic_get_hw1_op_cfg(addr, pval)     pmic_read_interface_nolock(addr, pval, 1, 2)
#define pmic_get_hw2_op_cfg(addr, pval)     pmic_read_interface_nolock(addr, pval, 1, 3)
#define pmic_get_hw3_op_cfg(addr, pval)     pmic_read_interface_nolock(addr, pval, 1, 4)

static int pmic_lp_type1_set(PMU_LP_TABLE1_ENUM pname, BUCK_LDO_EN_USER user, unsigned char op_en,
			     unsigned char op_cfg)
{
	const PMU_LP_TABLE1_ENTRY *pFlag = &pmu_lp_table1[pname];
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		PMICLOG("pmic_lp_type1 op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_lp_type1\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp, en_cfg);
		pmic_set_sw_lp((unsigned int)pFlag->en_lp, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_sw_op_en((unsigned int)pFlag->op_en, &rb_en);
		PMICLOG("sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp, en_cfg);
		pmic_set_sw_lp((unsigned int)pFlag->en_lp, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_sw_op_en((unsigned int)pFlag->op_en, &rb_en);
		PMICLOG("spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw0_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw0_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw0_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw1_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw1_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw1_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw2_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw2_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw2_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN3:
		if (pname == VCN28) {
			pmic_set_hw3_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
			pmic_set_hw3_op_en((unsigned int)pFlag->op_en, op_en);
			pmic_get_hw3_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
			pmic_get_hw3_op_en((unsigned int)pFlag->op_en, &rb_en);
			(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
			PMICLOG("srclken3 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		} else
			PMICLOG("only vcn28 have srclken3 lp op en = %d, op cfg = %d\n", rb_en,
				rb_cfg);
		break;
	default:
		PMICLOG("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

static int pmic_lp_type2_set(PMU_LP_TABLE2_ENUM pname, BUCK_LDO_EN_USER user, unsigned char op_en,
			     unsigned char op_cfg)
{
	const PMU_LP_TABLE2_ENTRY *pFlag = &pmu_lp_table2[pname];
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		PMICLOG("pmic_lp_type2 op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_lp_type2\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp_0, en_cfg);
		pmic_set_sw_en((unsigned int)pFlag->en_lp_1, en_cfg);
		pmic_set_sw_lp((unsigned int)pFlag->en_lp_0, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_sw_op_en((unsigned int)pFlag->op_en, &rb_en);
		PMICLOG("sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp_0, en_cfg);
		pmic_set_sw_en((unsigned int)pFlag->en_lp_1, en_cfg);
		pmic_set_sw_lp((unsigned int)pFlag->en_lp_0, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_sw_op_en((unsigned int)pFlag->op_en, &rb_en);
		PMICLOG("sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw0_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw0_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw0_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw1_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw1_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw1_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg((unsigned int)pFlag->op_cfg, op_cfg);
		pmic_set_hw2_op_en((unsigned int)pFlag->op_en, op_en);
		pmic_get_hw2_op_cfg((unsigned int)pFlag->op_cfg, &rb_cfg);
		pmic_get_hw2_op_en((unsigned int)pFlag->op_en, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		PMICLOG("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

static int pmic_lp_type3_set(PMU_LP_TABLE3_ENUM pname, BUCK_LDO_EN_USER user, unsigned char op_en,
			     unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		PMICLOG("pmic_lp_type3 op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_lp_type3\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		const PMU_LP_TABLE3_ENTRY *pFlag = &pmu_lp_table3[pname];
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp, en_cfg);
#endif				/* LP_GOLDEN_SETTING */
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		const PMU_LP_TABLE3_ENTRY *pFlag = &pmu_lp_table3[pname];
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en((unsigned int)pFlag->en_lp, en_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		break;
	default:
		PMICLOG("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vproc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vproc_lp\n");
	return pmic_lp_type1_set(VPROC, user, op_en, op_cfg);
}

int pmic_buck_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vcore_lp\n");
	return pmic_lp_type1_set(VCORE, user, op_en, op_cfg);
}

int pmic_buck_vmodem_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vmodem_lp\n");
	return pmic_lp_type1_set(VMODEM, user, op_en, op_cfg);
}

int pmic_buck_vs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vs1_lp\n");
	return pmic_lp_type1_set(VS1, user, op_en, op_cfg);
}

int pmic_buck_vs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vs2_lp\n");
	return pmic_lp_type1_set(VS2, user, op_en, op_cfg);
}

int pmic_buck_vpa_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vpa_lp\n");
	return pmic_lp_type3_set(VPA, user, op_en, op_cfg);
}

int pmic_ldo_vsram_proc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vsram_proc_lp\n");
	return pmic_lp_type1_set(VSRAM_PROC, user, op_en, op_cfg);
}

int pmic_ldo_vsram_gpu_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vsram_gpu_lp\n");
	return pmic_lp_type1_set(VSRAM_GPU, user, op_en, op_cfg);
}

int pmic_ldo_vsram_others_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vsram_others_lp\n");
	return pmic_lp_type1_set(VSRAM_OTHERS, user, op_en, op_cfg);
}

int pmic_ldo_vfe28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vfe28_lp\n");
	return pmic_lp_type1_set(VFE28, user, op_en, op_cfg);
}

int pmic_ldo_vxo22_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vxo22_lp\n");
	return pmic_lp_type1_set(VXO22, user, op_en, op_cfg);
}

int pmic_ldo_vrf18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vrf18_lp\n");
	return pmic_lp_type1_set(VRF18, user, op_en, op_cfg);
}

int pmic_ldo_vrf12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vrf12_lp\n");
	return pmic_lp_type1_set(VRF12, user, op_en, op_cfg);
}

int pmic_ldo_vmipi_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vmipi_lp\n");
	return pmic_lp_type1_set(VMIPI, user, op_en, op_cfg);
}

int pmic_ldo_vcn33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcn33_lp\n");
	return pmic_lp_type2_set(VCN33, user, op_en, op_cfg);
}

int pmic_ldo_vcn28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcn28_lp\n");
	return pmic_lp_type1_set(VCN28, user, op_en, op_cfg);
}

int pmic_ldo_vcn18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcn18_lp\n");
	return pmic_lp_type1_set(VCN18, user, op_en, op_cfg);
}

int pmic_ldo_vcama_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcama_lp\n");
	return pmic_lp_type1_set(VCAMA, user, op_en, op_cfg);
}

int pmic_ldo_vcamd_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcamd_lp\n");
	return pmic_lp_type1_set(VCAMD, user, op_en, op_cfg);
}

int pmic_ldo_vcamio_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vcamio_lp\n");
	return pmic_lp_type1_set(VCAMIO, user, op_en, op_cfg);
}

int pmic_ldo_vldo28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vldo28_lp\n");
	return pmic_lp_type2_set(VLDO28, user, op_en, op_cfg);
}

int pmic_ldo_va12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_va12_lp\n");
	return pmic_lp_type1_set(VA12, user, op_en, op_cfg);
}

int pmic_ldo_vaux18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vaux18_lp\n");
	return pmic_lp_type1_set(VAUX18, user, op_en, op_cfg);
}

int pmic_ldo_vaud28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vaud28_lp\n");
	return pmic_lp_type1_set(VAUD28, user, op_en, op_cfg);
}

int pmic_ldo_vio28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vio28_lp\n");
	return pmic_lp_type1_set(VIO28, user, op_en, op_cfg);
}

int pmic_ldo_vio18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vio18_lp\n");
	return pmic_lp_type1_set(VIO18, user, op_en, op_cfg);
}

int pmic_ldo_vdram_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vdram_lp\n");
	return pmic_lp_type1_set(VDRAM, user, op_en, op_cfg);
}

int pmic_ldo_vmc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vmc_lp\n");
	return pmic_lp_type1_set(VMC, user, op_en, op_cfg);
}

int pmic_ldo_vmch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vmch_lp\n");
	return pmic_lp_type1_set(VMCH, user, op_en, op_cfg);
}

int pmic_ldo_vemc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vemc_lp\n");
	return pmic_lp_type1_set(VEMC, user, op_en, op_cfg);
}

int pmic_ldo_vsim1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vsim1_lp\n");
	return pmic_lp_type1_set(VSIM1, user, op_en, op_cfg);
}

int pmic_ldo_vsim2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vsim2_lp\n");
	return pmic_lp_type1_set(VSIM2, user, op_en, op_cfg);
}

int pmic_ldo_vibr_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vibr_lp\n");
	return pmic_lp_type1_set(VIBR, user, op_en, op_cfg);
}

int pmic_ldo_vusb_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vusb_lp\n");
	return pmic_lp_type2_set(VUSB, user, op_en, op_cfg);
}

int pmic_ldo_vbif28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_ldo_vbif28_lp\n");
	return pmic_lp_type1_set(VBIF28, user, op_en, op_cfg);
}

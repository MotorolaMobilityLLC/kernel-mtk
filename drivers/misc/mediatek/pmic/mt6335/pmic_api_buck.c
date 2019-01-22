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

#define pmic_set_sw_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 0)
#define pmic_set_sw_lp(addr, val)		pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_sw_op_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 0)
#define pmic_set_hw0_op_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_hw1_op_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 2)
#define pmic_set_hw2_op_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 3)
#define pmic_set_hw3_op_en(addr, val)		pmic_config_interface_nolock(addr, val, 1, 4)
#define pmic_set_hw0_op_cfg(addr, val)		pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_hw1_op_cfg(addr, val)		pmic_config_interface_nolock(addr, val, 1, 2)
#define pmic_set_hw2_op_cfg(addr, val)		pmic_config_interface_nolock(addr, val, 1, 3)
#define pmic_set_hw3_op_cfg(addr, val)		pmic_config_interface_nolock(addr, val, 1, 4)
#define pmic_get_sw_op_en(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 0)
#define pmic_get_hw0_op_en(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 1)
#define pmic_get_hw1_op_en(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 2)
#define pmic_get_hw2_op_en(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 3)
#define pmic_get_hw3_op_en(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 4)
#define pmic_get_hw0_op_cfg(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 1)
#define pmic_get_hw1_op_cfg(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 2)
#define pmic_get_hw2_op_cfg(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 3)
#define pmic_get_hw3_op_cfg(addr, pval)		pmic_read_interface_nolock(addr, pval, 1, 4)

int pmic_buck_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vcore_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vcore_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VCORE_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VCORE_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VCORE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VCORE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vcore sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VCORE_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VCORE_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VCORE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VCORE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vcore spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VCORE_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VCORE_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VCORE_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VCORE_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vcore srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VCORE_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VCORE_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VCORE_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VCORE_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vcore srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VCORE_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VCORE_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VCORE_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VCORE_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vcore srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vdram_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vdram_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vdram_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VDRAM_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VDRAM_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VDRAM_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VDRAM_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vdram sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VDRAM_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VDRAM_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VDRAM_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VDRAM_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vdram spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VDRAM_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VDRAM_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VDRAM_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VDRAM_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vdram srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VDRAM_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VDRAM_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VDRAM_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VDRAM_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vdram srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VDRAM_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VDRAM_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VDRAM_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VDRAM_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vdram srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vmodem_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vmodem_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vmodem_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VMODEM_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VMODEM_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VMODEM_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VMODEM_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vmodem sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VMODEM_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VMODEM_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VMODEM_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VMODEM_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vmodem spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VMODEM_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VMODEM_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VMODEM_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VMODEM_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmodem srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VMODEM_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VMODEM_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VMODEM_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VMODEM_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmodem srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VMODEM_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VMODEM_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VMODEM_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VMODEM_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmodem srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vmd1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vmd1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vmd1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VMD1_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VMD1_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VMD1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VMD1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vmd1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VMD1_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VMD1_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VMD1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VMD1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vmd1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VMD1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VMD1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VMD1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VMD1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmd1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VMD1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VMD1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VMD1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VMD1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmd1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VMD1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VMD1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VMD1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VMD1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vmd1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vs1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vs1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VS1_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VS1_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VS1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VS1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vs1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VS1_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VS1_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VS1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VS1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vs1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VS1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VS1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VS1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VS1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VS1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VS1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VS1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VS1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VS1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VS1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VS1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VS1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vs2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vs2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VS2_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VS2_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VS2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VS2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vs2 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VS2_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VS2_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VS2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VS2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vs2 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VS2_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VS2_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VS2_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VS2_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs2 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VS2_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VS2_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VS2_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VS2_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs2 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VS2_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VS2_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VS2_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VS2_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vs2 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vpa1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vpa1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vpa1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VPA1_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING */
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VPA1_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_buck_vimvo_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_buck_vimvo_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_buck_vimvo_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VIMVO_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VIMVO_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VIMVO_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VIMVO_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vimvo sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_BUCK_VIMVO_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_BUCK_VIMVO_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_BUCK_VIMVO_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_BUCK_VIMVO_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("buck vimvo spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_BUCK_VIMVO_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_BUCK_VIMVO_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_BUCK_VIMVO_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_BUCK_VIMVO_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vimvo srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_BUCK_VIMVO_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_BUCK_VIMVO_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_BUCK_VIMVO_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_BUCK_VIMVO_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vimvo srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_BUCK_VIMVO_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_BUCK_VIMVO_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_BUCK_VIMVO_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_BUCK_VIMVO_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("buck vimvo srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsram_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsram_vcore_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsram_vcore_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VCORE_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VCORE_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VCORE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VCORE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vcore sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VCORE_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VCORE_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VCORE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VCORE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vcore spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSRAM_VCORE_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSRAM_VCORE_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSRAM_VCORE_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSRAM_VCORE_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vcore srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSRAM_VCORE_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSRAM_VCORE_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSRAM_VCORE_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSRAM_VCORE_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vcore srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSRAM_VCORE_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSRAM_VCORE_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSRAM_VCORE_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSRAM_VCORE_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vcore srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsram_dvfs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsram_dvfs1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsram_dvfs1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_DVFS1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_DVFS1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_DVFS1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_DVFS1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_dvfs1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_DVFS1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_DVFS1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_DVFS1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_DVFS1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_dvfs1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSRAM_DVFS1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSRAM_DVFS1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSRAM_DVFS1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSRAM_DVFS1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSRAM_DVFS1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSRAM_DVFS1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSRAM_DVFS1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSRAM_DVFS1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSRAM_DVFS1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSRAM_DVFS1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSRAM_DVFS1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSRAM_DVFS1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsram_dvfs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsram_dvfs2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsram_dvfs2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_DVFS2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_DVFS2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_DVFS2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_DVFS2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_dvfs2 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_DVFS2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_DVFS2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_DVFS2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_DVFS2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_dvfs2 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSRAM_DVFS2_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSRAM_DVFS2_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSRAM_DVFS2_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSRAM_DVFS2_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs2 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSRAM_DVFS2_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSRAM_DVFS2_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSRAM_DVFS2_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSRAM_DVFS2_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs2 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSRAM_DVFS2_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSRAM_DVFS2_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSRAM_DVFS2_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSRAM_DVFS2_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_dvfs2 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsram_vgpu_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsram_vgpu_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsram_vgpu_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VGPU_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VGPU_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VGPU_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VGPU_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vgpu sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VGPU_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VGPU_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VGPU_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VGPU_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vgpu spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSRAM_VGPU_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSRAM_VGPU_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSRAM_VGPU_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSRAM_VGPU_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vgpu srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSRAM_VGPU_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSRAM_VGPU_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSRAM_VGPU_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSRAM_VGPU_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vgpu srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSRAM_VGPU_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSRAM_VGPU_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSRAM_VGPU_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSRAM_VGPU_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vgpu srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsram_vmd_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsram_vmd_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsram_vmd_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VMD_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VMD_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VMD_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VMD_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vmd sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSRAM_VMD_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSRAM_VMD_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSRAM_VMD_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSRAM_VMD_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsram_vmd spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSRAM_VMD_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSRAM_VMD_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSRAM_VMD_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSRAM_VMD_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vmd srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSRAM_VMD_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSRAM_VMD_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSRAM_VMD_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSRAM_VMD_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vmd srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSRAM_VMD_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSRAM_VMD_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSRAM_VMD_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSRAM_VMD_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsram_vmd srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vrf18_1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vrf18_1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vrf18_1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF18_1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF18_1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VRF18_1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF18_1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf18_1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF18_1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF18_1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VRF18_1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF18_1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf18_1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VRF18_1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VRF18_1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VRF18_1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VRF18_1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VRF18_1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VRF18_1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VRF18_1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VRF18_1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VRF18_1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VRF18_1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VRF18_1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VRF18_1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vrf18_2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vrf18_2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vrf18_2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF18_2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF18_2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VRF18_2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF18_2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf18_2 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF18_2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF18_2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VRF18_2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF18_2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf18_2 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VRF18_2_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VRF18_2_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VRF18_2_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VRF18_2_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_2 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VRF18_2_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VRF18_2_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VRF18_2_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VRF18_2_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_2 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VRF18_2_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VRF18_2_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VRF18_2_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VRF18_2_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf18_2 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vrf12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vrf12_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vrf12_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF12_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF12_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VRF12_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF12_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf12 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VRF12_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VRF12_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VRF12_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VRF12_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vrf12 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VRF12_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VRF12_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VRF12_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VRF12_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf12 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VRF12_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VRF12_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VRF12_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VRF12_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf12 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VRF12_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VRF12_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VRF12_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VRF12_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vrf12 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcn33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcn33_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcn33_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN33_SW_EN_BT_ADDR, en_cfg);
		pmic_set_sw_en(PMIC_RG_VCN33_SW_EN_WIFI_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN33_SW_LP_BT_ADDR, lp_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN33_SW_LP_WIFI_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_BT_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_BT_ADDR, &rb_en);
		PMICLOG("ldo vcn33 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		pmic_set_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_WIFI_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_WIFI_ADDR, &rb_en);
		PMICLOG("ldo vcn33 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN33_SW_EN_BT_ADDR, en_cfg);
		pmic_set_sw_en(PMIC_RG_VCN33_SW_EN_WIFI_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN33_SW_LP_BT_ADDR, lp_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN33_SW_LP_WIFI_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_BT_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_BT_ADDR, &rb_en);
		PMICLOG("ldo vcn33 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		pmic_set_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_WIFI_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN33_SW_OP_EN_WIFI_ADDR, &rb_en);
		PMICLOG("ldo vcn33 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCN33_HW0_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCN33_HW0_OP_EN_BT_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCN33_HW0_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCN33_HW0_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw0_op_cfg(PMIC_RG_VCN33_HW0_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCN33_HW0_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCN33_HW0_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCN33_HW0_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCN33_HW1_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCN33_HW1_OP_EN_BT_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCN33_HW1_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCN33_HW1_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw1_op_cfg(PMIC_RG_VCN33_HW1_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCN33_HW1_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCN33_HW1_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCN33_HW1_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCN33_HW2_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCN33_HW2_OP_EN_BT_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCN33_HW2_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCN33_HW2_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw2_op_cfg(PMIC_RG_VCN33_HW2_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCN33_HW2_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCN33_HW2_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCN33_HW2_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn33 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcn28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcn28_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcn28_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCN28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcn28 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCN28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcn28 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCN28_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCN28_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCN28_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCN28_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn28 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCN28_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCN28_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCN28_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCN28_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn28 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCN28_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCN28_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCN28_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCN28_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn28 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN3:
		pmic_set_hw3_op_cfg(PMIC_RG_VCN28_HW3_OP_CFG_ADDR, op_cfg);
		pmic_set_hw3_op_en(PMIC_RG_VCN28_HW3_OP_EN_ADDR, op_en);
		pmic_get_hw3_op_cfg(PMIC_RG_VCN28_HW3_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw3_op_en(PMIC_RG_VCN28_HW3_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn28 srclken3 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcn18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcn18_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcn18_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN18_SW_EN_BT_ADDR, en_cfg);
		pmic_set_sw_en(PMIC_RG_VCN18_SW_EN_WIFI_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN18_SW_LP_BT_ADDR, lp_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN18_SW_LP_WIFI_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_BT_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_BT_ADDR, &rb_en);
		PMICLOG("ldo vcn18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		pmic_set_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_WIFI_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_WIFI_ADDR, &rb_en);
		PMICLOG("ldo vcn18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCN18_SW_EN_BT_ADDR, en_cfg);
		pmic_set_sw_en(PMIC_RG_VCN18_SW_EN_WIFI_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN18_SW_LP_BT_ADDR, lp_cfg);
		pmic_set_sw_lp(PMIC_RG_VCN18_SW_LP_WIFI_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_BT_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_BT_ADDR, &rb_en);
		PMICLOG("ldo vcn18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		pmic_set_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_WIFI_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCN18_SW_OP_EN_WIFI_ADDR, &rb_en);
		PMICLOG("ldo vcn18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCN18_HW0_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCN18_HW0_OP_EN_BT_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCN18_HW0_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCN18_HW0_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw0_op_cfg(PMIC_RG_VCN18_HW0_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCN18_HW0_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCN18_HW0_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCN18_HW0_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCN18_HW1_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCN18_HW1_OP_EN_BT_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCN18_HW1_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCN18_HW1_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw1_op_cfg(PMIC_RG_VCN18_HW1_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCN18_HW1_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCN18_HW1_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCN18_HW1_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCN18_HW2_OP_CFG_BT_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCN18_HW2_OP_EN_BT_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCN18_HW2_OP_CFG_BT_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCN18_HW2_OP_EN_BT_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		pmic_set_hw2_op_cfg(PMIC_RG_VCN18_HW2_OP_CFG_WIFI_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCN18_HW2_OP_EN_WIFI_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCN18_HW2_OP_CFG_WIFI_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCN18_HW2_OP_EN_WIFI_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcn18 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcama1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcama1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcama1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMA1_SW_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING */
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMA1_SW_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcama2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcama2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcama2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMA2_SW_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING */
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMA2_SW_EN_ADDR, en_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcamd1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcamd1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcamd1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMD1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMD1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCAMD1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMD1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamd1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMD1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMD1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCAMD1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMD1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamd1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCAMD1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCAMD1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCAMD1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCAMD1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCAMD1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCAMD1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCAMD1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCAMD1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCAMD1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCAMD1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCAMD1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCAMD1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcamd2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcamd2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcamd2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMD2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMD2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCAMD2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMD2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamd2 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMD2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMD2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCAMD2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMD2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamd2 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCAMD2_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCAMD2_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCAMD2_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCAMD2_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd2 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCAMD2_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCAMD2_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCAMD2_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCAMD2_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd2 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCAMD2_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCAMD2_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCAMD2_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCAMD2_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamd2 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcamio_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcamio_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcamio_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMIO_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMIO_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCAMIO_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMIO_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamio sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMIO_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMIO_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCAMIO_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMIO_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamio spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCAMIO_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCAMIO_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCAMIO_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCAMIO_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamio srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCAMIO_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCAMIO_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCAMIO_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCAMIO_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamio srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCAMIO_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCAMIO_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCAMIO_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCAMIO_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamio srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vcamaf_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vcamaf_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vcamaf_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMAF_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMAF_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VCAMAF_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMAF_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamaf sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VCAMAF_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VCAMAF_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VCAMAF_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VCAMAF_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vcamaf spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VCAMAF_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VCAMAF_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VCAMAF_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VCAMAF_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamaf srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VCAMAF_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VCAMAF_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VCAMAF_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VCAMAF_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamaf srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VCAMAF_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VCAMAF_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VCAMAF_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VCAMAF_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vcamaf srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_va10_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_va10_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_va10_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA10_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA10_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VA10_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA10_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va10 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA10_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA10_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VA10_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA10_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va10 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VA10_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VA10_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VA10_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VA10_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va10 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VA10_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VA10_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VA10_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VA10_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va10 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VA10_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VA10_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VA10_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VA10_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va10 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_va12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_va12_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_va12_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA12_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA12_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VA12_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA12_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va12 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA12_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA12_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VA12_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA12_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va12 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VA12_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VA12_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VA12_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VA12_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va12 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VA12_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VA12_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VA12_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VA12_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va12 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VA12_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VA12_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VA12_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VA12_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va12 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_va18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_va18_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_va18_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VA18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VA18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VA18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VA18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VA18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo va18 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VA18_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VA18_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VA18_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VA18_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va18 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VA18_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VA18_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VA18_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VA18_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va18 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VA18_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VA18_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VA18_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VA18_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo va18 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsim2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsim2_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsim2_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSIM2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSIM2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSIM2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSIM2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsim2 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSIM2_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSIM2_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSIM2_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSIM2_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsim2 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSIM2_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSIM2_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSIM2_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSIM2_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim2 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSIM2_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSIM2_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSIM2_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSIM2_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim2 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSIM2_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSIM2_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSIM2_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSIM2_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim2 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vsim1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vsim1_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vsim1_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSIM1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSIM1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VSIM1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSIM1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsim1 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VSIM1_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VSIM1_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VSIM1_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VSIM1_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vsim1 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VSIM1_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VSIM1_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VSIM1_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VSIM1_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim1 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VSIM1_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VSIM1_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VSIM1_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VSIM1_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim1 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VSIM1_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VSIM1_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VSIM1_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VSIM1_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vsim1 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vtouch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vtouch_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vtouch_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VTOUCH_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VTOUCH_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VTOUCH_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VTOUCH_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vtouch sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VTOUCH_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VTOUCH_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VTOUCH_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VTOUCH_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vtouch spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VTOUCH_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VTOUCH_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VTOUCH_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VTOUCH_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vtouch srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VTOUCH_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VTOUCH_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VTOUCH_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VTOUCH_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vtouch srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VTOUCH_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VTOUCH_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VTOUCH_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VTOUCH_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vtouch srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vmc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vmc_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vmc_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMC_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMC_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VMC_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMC_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmc sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMC_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMC_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VMC_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMC_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmc spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VMC_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VMC_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VMC_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VMC_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmc srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VMC_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VMC_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VMC_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VMC_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmc srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VMC_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VMC_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VMC_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VMC_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmc srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vmch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vmch_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vmch_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMCH_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMCH_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VMCH_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMCH_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmch sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMCH_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMCH_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VMCH_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMCH_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmch spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VMCH_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VMCH_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VMCH_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VMCH_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmch srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VMCH_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VMCH_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VMCH_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VMCH_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmch srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VMCH_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VMCH_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VMCH_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VMCH_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmch srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vemc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vemc_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vemc_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VEMC_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VEMC_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VEMC_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VEMC_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vemc sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VEMC_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VEMC_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VEMC_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VEMC_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vemc spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VEMC_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VEMC_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VEMC_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VEMC_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vemc srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VEMC_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VEMC_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VEMC_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VEMC_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vemc srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VEMC_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VEMC_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VEMC_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VEMC_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vemc srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vufs18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vufs18_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vufs18_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VUFS18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VUFS18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VUFS18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VUFS18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vufs18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VUFS18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VUFS18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VUFS18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VUFS18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vufs18 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VUFS18_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VUFS18_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VUFS18_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VUFS18_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vufs18 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VUFS18_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VUFS18_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VUFS18_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VUFS18_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vufs18 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VUFS18_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VUFS18_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VUFS18_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VUFS18_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vufs18 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vefuse_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vefuse_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vefuse_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VEFUSE_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VEFUSE_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VEFUSE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VEFUSE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vefuse sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VEFUSE_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VEFUSE_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VEFUSE_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VEFUSE_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vefuse spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VEFUSE_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VEFUSE_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VEFUSE_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VEFUSE_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vefuse srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VEFUSE_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VEFUSE_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VEFUSE_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VEFUSE_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vefuse srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VEFUSE_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VEFUSE_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VEFUSE_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VEFUSE_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vefuse srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vusb33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vusb33_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vusb33_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VUSB33_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VUSB33_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VUSB33_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VUSB33_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vusb33 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VUSB33_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VUSB33_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VUSB33_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VUSB33_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vusb33 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VUSB33_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VUSB33_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VUSB33_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VUSB33_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vusb33 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VUSB33_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VUSB33_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VUSB33_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VUSB33_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vusb33 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VUSB33_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VUSB33_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VUSB33_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VUSB33_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vusb33 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vio18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vio18_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vio18_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIO18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIO18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VIO18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIO18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vio18 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIO18_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIO18_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VIO18_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIO18_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vio18 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VIO18_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VIO18_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VIO18_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VIO18_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio18 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VIO18_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VIO18_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VIO18_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VIO18_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio18 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VIO18_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VIO18_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VIO18_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VIO18_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio18 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vio28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vio28_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vio28_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIO28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIO28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VIO28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIO28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vio28 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIO28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIO28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VIO28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIO28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vio28 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VIO28_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VIO28_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VIO28_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VIO28_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio28 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VIO28_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VIO28_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VIO28_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VIO28_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio28 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VIO28_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VIO28_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VIO28_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VIO28_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vio28 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vbif28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vbif28_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vbif28_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VBIF28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VBIF28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VBIF28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VBIF28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vbif28 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VBIF28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VBIF28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VBIF28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VBIF28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vbif28 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VBIF28_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VBIF28_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VBIF28_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VBIF28_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vbif28 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VBIF28_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VBIF28_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VBIF28_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VBIF28_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vbif28 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VBIF28_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VBIF28_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VBIF28_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VBIF28_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vbif28 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vmipi_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vmipi_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vmipi_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMIPI_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMIPI_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VMIPI_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMIPI_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmipi sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VMIPI_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VMIPI_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VMIPI_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VMIPI_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vmipi spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VMIPI_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VMIPI_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VMIPI_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VMIPI_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmipi srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VMIPI_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VMIPI_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VMIPI_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VMIPI_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmipi srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VMIPI_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VMIPI_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VMIPI_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VMIPI_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vmipi srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vgp3_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vgp3_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vgp3_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VGP3_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VGP3_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VGP3_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VGP3_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vgp3 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VGP3_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VGP3_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VGP3_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VGP3_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vgp3 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VGP3_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VGP3_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VGP3_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VGP3_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vgp3 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VGP3_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VGP3_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VGP3_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VGP3_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vgp3 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VGP3_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VGP3_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VGP3_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VGP3_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vgp3 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vibr_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vibr_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vibr_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIBR_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIBR_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VIBR_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIBR_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vibr sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VIBR_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VIBR_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VIBR_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VIBR_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vibr spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VIBR_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VIBR_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VIBR_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VIBR_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vibr srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VIBR_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VIBR_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VIBR_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VIBR_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vibr srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VIBR_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VIBR_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VIBR_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VIBR_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vibr srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vxo22_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vxo22_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vxo22_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VXO22_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VXO22_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VXO22_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VXO22_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vxo22 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VXO22_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VXO22_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VXO22_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VXO22_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vxo22 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VXO22_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VXO22_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VXO22_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VXO22_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vxo22 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VXO22_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VXO22_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VXO22_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VXO22_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vxo22 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VXO22_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VXO22_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VXO22_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VXO22_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vxo22 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

int pmic_ldo_vfe28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en, rb_cfg, max_cfg = 1;
	int ret = 0;

	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("pmic_ldo_vfe28_lp op_en/op_cfg should be 0 or %d\n", max_cfg);
		return -1;
	}

	PMICLOG("pmic_ldo_vfe28_lp\n");

	switch (user) {
	case SW:
#ifdef LP_GOLDEN_SETTING
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VFE28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VFE28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING */
		pmic_set_sw_op_en(PMIC_RG_VFE28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VFE28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vfe28 sw lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SPM:
#ifdef LP_GOLDEN_SETTING_W_SPM
		unsigned int en_cfg, lp_cfg;

		en_cfg = op_cfg & 0x1;
		lp_cfg = (op_cfg >> 1) & 0x1;
		pmic_set_sw_en(PMIC_RG_VFE28_SW_EN_ADDR, en_cfg);
		pmic_set_sw_lp(PMIC_RG_VFE28_SW_LP_ADDR, lp_cfg);
#endif				/* LP_GOLDEN_SETTING_W_SPM */
		pmic_set_sw_op_en(PMIC_RG_VFE28_SW_OP_EN_ADDR, op_en);
		pmic_get_sw_op_en(PMIC_RG_VFE28_SW_OP_EN_ADDR, &rb_en);
		PMICLOG("ldo vfe28 spm lp op en = %d\n", rb_en);
		(rb_en == op_en) ? (ret = 0) : (ret = -1);
		break;
	case SRCLKEN0:
		pmic_set_hw0_op_cfg(PMIC_RG_VFE28_HW0_OP_CFG_ADDR, op_cfg);
		pmic_set_hw0_op_en(PMIC_RG_VFE28_HW0_OP_EN_ADDR, op_en);
		pmic_get_hw0_op_cfg(PMIC_RG_VFE28_HW0_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw0_op_en(PMIC_RG_VFE28_HW0_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vfe28 srclken0 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN1:
		pmic_set_hw1_op_cfg(PMIC_RG_VFE28_HW1_OP_CFG_ADDR, op_cfg);
		pmic_set_hw1_op_en(PMIC_RG_VFE28_HW1_OP_EN_ADDR, op_en);
		pmic_get_hw1_op_cfg(PMIC_RG_VFE28_HW1_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw1_op_en(PMIC_RG_VFE28_HW1_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vfe28 srclken1 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	case SRCLKEN2:
		pmic_set_hw2_op_cfg(PMIC_RG_VFE28_HW2_OP_CFG_ADDR, op_cfg);
		pmic_set_hw2_op_en(PMIC_RG_VFE28_HW2_OP_EN_ADDR, op_en);
		pmic_get_hw2_op_cfg(PMIC_RG_VFE28_HW2_OP_CFG_ADDR, &rb_cfg);
		pmic_get_hw2_op_en(PMIC_RG_VFE28_HW2_OP_EN_ADDR, &rb_en);
		(rb_en == op_en && rb_cfg == op_cfg) ? (ret = 0) : (ret = -1);
		PMICLOG("ldo vfe28 srclken2 lp op en = %d, op cfg = %d\n", rb_en, rb_cfg);
		break;
	default:
		pr_debug("unknown user\n");
		ret = -1;
		break;
	}
	return ret;
}

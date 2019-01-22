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

#define pmic_set_sw_en(addr, val)             pmic_config_interface_nolock(addr, val, 1, 0)
#define pmic_set_sw_lp(addr, val)             pmic_config_interface_nolock(addr, val, 1, 1)
#define pmic_set_op_en(user, addr, val)       pmic_config_interface_nolock(addr, val, 1, user)
#define pmic_set_op_cfg(user, addr, val)      pmic_config_interface_nolock(addr, val, 1, user)
#define pmic_get_op_en(user, addr, pval)      pmic_read_interface_nolock(addr, pval, 1, user)
#define pmic_get_op_cfg(user, addr, pval)     pmic_read_interface_nolock(addr, pval, 1, user)

#define en_cfg_shift  0x6

#if defined(LGS) || defined(LGSWS)

const struct PMU_LP_TABLE_ENTRY pmu_lp_table[] = {
	PMIC_LP_BUCK_ENTRY(VPROC),
};

static int pmic_lp_golden_set(unsigned int en_adr, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int en_cfg = 0, lp_cfg = 0;

    /*--op_cfg 0:SW_OFF, 1:SW_EN, 3: SW_LP (SPM)--*/
	if (op_en > 1 || op_cfg > 3) {
		pr_err("p\n");
		return -1;
	}

	en_cfg = op_cfg & 0x1;
	lp_cfg = (op_cfg >> 1) & 0x1;
	pmic_set_sw_en(en_adr, en_cfg);
	pmic_set_sw_lp(en_adr, lp_cfg);
}
#endif

static int pmic_lp_type_set(unsigned short en_cfg_adr, enum PMU_LP_TABLE_ENUM name,
			    enum BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	unsigned int rb_en = 0, rb_cfg = 0, max_cfg = 1;
	unsigned short op_en_adr = 0, op_cfg_adr = 0;
	int ret = 0, ret_en = 0, ret_cfg = 0;

	if (en_cfg_adr) {
		op_en_adr = en_cfg_adr;
		op_cfg_adr = (unsigned short)(en_cfg_adr + en_cfg_shift);
	}
	  /*--else keep default adr = 0--*/
	if (user == SW || user == SPM) {
		max_cfg = 3;
		rb_cfg = 0;
		rb_en = 0;
	}

	if (op_en > 1 || op_cfg > max_cfg) {
		pr_err("p\n");
		return -1;
	}

	PMICLOG("0x%x,type %d\n", en_cfg_adr, user);

#if defined(LGS) || defined(LGSWS)
	const PMU_LP_TABLE_ENTRY *pFlag = &pmu_lp_table[name];

	if (user == SW || user == SPM)
		pmic_lp_golden_set((unsigned int)pFlag->en_adr, op_en, op_cfg);

#endif

	if (op_cfg_adr && op_en_adr) {
		pmic_set_op_en(user, op_en_adr, op_en);
		pmic_get_op_en(user, op_en_adr, &rb_en);
		PMICLOG("user = %d, op en = %d\n", user, rb_en);
		(rb_en == op_en) ? (ret_en = 0) : (ret_en = -1);
		if (user != SW && user != SPM) {
			pmic_set_op_cfg(user, op_cfg_adr, op_cfg);
			pmic_get_op_cfg(user, op_cfg_adr, &rb_cfg);
			(rb_cfg == op_cfg) ? (ret_cfg = 0) : (ret_cfg - 1);
			PMICLOG("user = %d, op cfg = %d\n", user, rb_cfg);
		}
	}

	((!ret_en) && (!ret_cfg)) ? (ret = 0) : (ret = -1);
	if (ret)
		pr_err("%d, %d, %d\n", user, ret_en, ret_cfg);
	return ret;
}

int pmic_buck_vproc_lp(enum BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg)
{
	PMICLOG("pmic_buck_vproc_lp\n");
	return pmic_lp_type_set(MT6357_BUCK_VPROC_OP_EN, VPROC, user, op_en, op_cfg);
}


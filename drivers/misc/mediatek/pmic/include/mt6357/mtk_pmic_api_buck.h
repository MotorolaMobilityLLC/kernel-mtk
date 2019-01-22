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

#ifndef _MT_PMIC_API_BUCK_H_
#define _MT_PMIC_API_BUCK_H_

void vmd1_pmic_setting_on(void);
void vmd1_pmic_setting_off(void);
int vcore_pmic_set_mode(unsigned char mode);
int vproc_pmic_set_mode(unsigned char mode);
void wk_auxadc_bgd_ctrl(unsigned char en);
void wk_auxadc_bgd_ctrl_dbg(void);

#ifdef LP_GOLDEN_SETTING
#define LGS
#endif

#ifdef LP_GOLDEN_SETTING_W_SPM
#define LGSWS
#endif

#if defined(LGS) || defined(LGSWS)
#define PMIC_LP_BUCK_ENTRY(reg) {reg, MT6357_BUCK_##reg##_CON0}
#define PMIC_LP_LDO_ENTRY(reg) {reg, MT6357_LDO_##reg##_CON0}
#define PMIC_LP_LDO_VCN33_0_ENTRY(reg) {reg, MT6357_LDO_VCN33_CON0_0}
#define PMIC_LP_LDO_VCN33_1_ENTRY(reg) {reg, MT6357_LDO_VCN33_CON0_1}
#define PMIC_LP_LDO_VLDO28_0_ENTRY(reg) {reg, MT6357_LDO_VLDO28_CON0_0}
#define PMIC_LP_LDO_VLDO28_1_ENTRY(reg) {reg, MT6357_LDO_VLDO28_CON0_1}
#define PMIC_LP_LDO_VUSB33_0_ENTRY(reg) {reg, MT6357_LDO_VUSB33_CON0_0}
#define PMIC_LP_LDO_VUSB33_1_ENTRY(reg) {reg, MT6357_LDO_VUSB33_CON0_1}
#endif

enum BUCK_LDO_EN_USER {
	SW,
	SPM = SW,
	SRCLKEN0,
	SRCLKEN1,
	SRCLKEN2,
	SRCLKEN3,
};

#define HW_OFF	0
#define HW_LP	1
#define SW_OFF	0
#define SW_ON	1
#define SW_LP	3
#define SPM_OFF	0
#define SPM_ON	1
#define SPM_LP	3

enum PMU_LP_TABLE_ENUM {
	VPROC,
	TABLE_COUNT_END
};

struct PMU_LP_TABLE_ENTRY {
	enum PMU_LP_TABLE_ENUM flagname;
#if defined(LGS) || defined(LGSWS)
	unsigned short en_adr;
#endif
};

extern int pmic_buck_vproc_lp(enum BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);

#endif

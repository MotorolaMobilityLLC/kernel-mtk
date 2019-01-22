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
#define PMIC_LP_BUCK_ENTRY(reg) {reg, MT6356_BUCK_##reg##_CON0}
#define PMIC_LP_LDO_ENTRY(reg) {reg, MT6356_LDO_##reg##_CON0}
#define PMIC_LP_LDO_VCN33_0_ENTRY(reg) {reg, MT6356_LDO_VCN33_CON0_0}
#define PMIC_LP_LDO_VCN33_1_ENTRY(reg) {reg, MT6356_LDO_VCN33_CON0_1}
#define PMIC_LP_LDO_VLDO28_0_ENTRY(reg) {reg, MT6356_LDO_VLDO28_CON0_0}
#define PMIC_LP_LDO_VLDO28_1_ENTRY(reg) {reg, MT6356_LDO_VLDO28_CON0_1}
#define PMIC_LP_LDO_VUSB_0_ENTRY(reg) {reg, MT6356_LDO_VUSB_CON0_0}
#define PMIC_LP_LDO_VUSB_1_ENTRY(reg) {reg, MT6356_LDO_VUSB_CON0_1}
#endif

typedef enum {
	SW,
	SPM = SW,
	SRCLKEN0,
	SRCLKEN1,
	SRCLKEN2,
	SRCLKEN3,
} BUCK_LDO_EN_USER;

#define HW_OFF	0
#define HW_LP	1
#define SW_OFF	0
#define SW_ON	1
#define SW_LP	3
#define SPM_OFF	0
#define SPM_ON	1
#define SPM_LP	3

typedef enum {
	VPROC,
	VCORE,
	VMODEM,
	VS1,
	VS2,
	VSRAM_PROC,
	VSRAM_GPU,
	VSRAM_OTHERS,
	VFE28,
	VXO22,
	VRF18,
	VRF12,
	VMIPI,
	VCN28,
	VCN18,
	VCAMA,
	VCAMD,
	VCAMIO,
	VA12,
	VAUX18,
	VAUD28,
	VIO28,
	VIO18,
	VDRAM,
	VMC,
	VMCH,
	VEMC,
	VSIM1,
	VSIM2,
	VIBR,
	VBIF28,
	VCN33_0,
	VCN33_1,
	VLDO28_0,
	VLDO28_1,
	VUSB_0,
	VUSB_1,
	VPA,
	TABLE_COUNT_END
} PMU_LP_TABLE_ENUM;

typedef struct {
	PMU_LP_TABLE_ENUM flagname;
#if defined(LGS) || defined(LGSWS)
	unsigned short en_adr;
#endif
} PMU_LP_TABLE_ENTRY;

extern int pmic_buck_vproc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vmodem_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vpa_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsram_proc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsram_gpu_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsram_others_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vfe28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vxo22_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vrf18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vrf12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmipi_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcama_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamd_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamio_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vldo28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_va12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vaux18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vaud28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vio28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vio18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vdram_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vemc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsim1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsim2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vibr_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vusb_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vbif28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);

#endif				/* _MT_PMIC_API_BUCK_H_ */

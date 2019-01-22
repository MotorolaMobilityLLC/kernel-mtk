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
void wk_auxadc_bgd_ctrl(unsigned char en);
void wk_auxadc_bgd_ctrl_dbg(void);

typedef enum {
	SW,
	SPM,
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
	TABLE1_COUNT_END
} PMU_LP_TABLE1_ENUM;

typedef enum {
	VCN33,
	VLDO28,
	VUSB,
	TABLE2_COUNT_END
} PMU_LP_TABLE2_ENUM;

typedef enum {
	VPA,
	TABLE3_COUNT_END
} PMU_LP_TABLE3_ENUM;

typedef struct {
	PMU_LP_TABLE1_ENUM flagname;
	unsigned short op_en;
	unsigned short op_cfg;
	unsigned short en_lp;
} PMU_LP_TABLE1_ENTRY;

typedef struct {
	PMU_LP_TABLE2_ENUM flagname;
	unsigned short op_en;
	unsigned short op_cfg;
	unsigned short en_lp_0;
	unsigned short en_lp_1;
} PMU_LP_TABLE2_ENTRY;

typedef struct {
	PMU_LP_TABLE3_ENUM flagname;
	unsigned short en_lp;
} PMU_LP_TABLE3_ENTRY;

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

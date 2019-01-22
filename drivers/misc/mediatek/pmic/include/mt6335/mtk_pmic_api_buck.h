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

#ifdef LP_GOLDEN_SETTING
#define LGS
#endif

#ifdef LP_GOLDEN_SETTING_W_SPM
#define LGSWS
#endif

#if defined(LGS) || defined(LGSWS)

#define PMIC_LP_BUCK_ENTRY(reg) {reg, MT6335_BUCK_##reg##_CON0}
#define PMIC_LP_LDO_ENTRY(reg) {reg, MT6335_LDO_##reg##_CON0}
#define PMIC_LP_LDO_VCN33_0_ENTRY(reg) {reg, MT6335_LDO_VCN33_CON0_BT}
#define PMIC_LP_LDO_VCN33_1_ENTRY(reg) {reg, MT6335_LDO_VCN33_CON0_WIFI}
#define PMIC_LP_LDO_VCN18_0_ENTRY(reg) {reg, MT6335_LDO_VCN18_CON0_BT}
#define PMIC_LP_LDO_VCN18_1_ENTRY(reg) {reg, MT6335_LDO_VCN18_CON0_WIFI}
#endif

typedef enum {
	SW,
	SPM = SW,
	SRCLKEN0,
	SRCLKEN1,
	SRCLKEN2,
	SRCLKEN3,
} BUCK_LDO_EN_USER;

#define HW_OFF   0
#define HW_LP   1
#define SW_OFF  0
#define SW_ON   1
#define SW_LP   3
#define SPM_OFF 0
#define SPM_ON  1
#define SPM_LP  3

typedef enum {
	VCORE,
	VDRAM,
	VMODEM,
	VMD1,
	VS1,
	VS2,
	VIMVO,
	VSRAM_VCORE,
	VSRAM_DVFS1,
	VSRAM_DVFS2,
	VSRAM_VGPU,
	VSRAM_VMD,
	VRF18_1,
	VRF18_2,
	VRF12,
	VCN28,
	VCAMD1,
	VCAMD2,
	VCAMIO,
	VCAMAF,
	VA10,
	VA12,
	VA18,
	VSIM2,
	VSIM1,
	VTOUCH,
	VMC,
	VMCH,
	VEMC,
	VUFS18,
	VEFUSE,
	VUSB33,
	VIO18,
	VIO28,
	VBIF28,
	VMIPI,
	VGP3,
	VIBR,
	VXO22,
	VFE28,
	VCN33_0,
	VCN33_1,
	VCN18_0,
	VCN18_1,
	VPA1,
	VCAMA1,
	VCAMA2,
	TABLE_COUNT_END
} PMU_LP_TABLE_ENUM;

typedef struct {
	PMU_LP_TABLE_ENUM flagname;
#if defined(LGS) || defined(LGSWS)
	unsigned short en_adr;
#endif
} PMU_LP_TABLE_ENTRY;

extern int pmic_buck_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vdram_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vmodem_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vmd1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vpa1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_buck_vimvo_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsram_vcore_lp(BUCK_LDO_EN_USER user, unsigned char op_en,
				   unsigned char op_cfg);
extern int pmic_ldo_vsram_dvfs1_lp(BUCK_LDO_EN_USER user, unsigned char op_en,
				   unsigned char op_cfg);
extern int pmic_ldo_vsram_dvfs2_lp(BUCK_LDO_EN_USER user, unsigned char op_en,
				   unsigned char op_cfg);
extern int pmic_ldo_vsram_vgpu_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsram_vmd_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vrf18_1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vrf18_2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vrf12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcn18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcama1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcama2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamd1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamd2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamio_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vcamaf_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_va10_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_va12_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_va18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsim2_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vsim1_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vtouch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmch_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vemc_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vufs18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vefuse_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vusb33_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vio18_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vio28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vbif28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vmipi_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vgp3_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vibr_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vxo22_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);
extern int pmic_ldo_vfe28_lp(BUCK_LDO_EN_USER user, unsigned char op_en, unsigned char op_cfg);

#endif				/* _MT_PMIC_API_BUCK_H_ */

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

void pmic_ldo_vio28_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VIO28; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VIO28_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VIO28_EN);
	PMICLOG("ldo : VIO28; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vio18_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VIO18; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VIO18_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VIO18_EN);
	PMICLOG("ldo : VIO18; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vufs18_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VUFS18; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VUFS18_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VUFS18_EN);
	PMICLOG("ldo : VUFS18; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_va10_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VA10; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VA10_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VA10_EN);
	PMICLOG("ldo : VA10; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_va12_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VA12; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VA12_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VA12_EN);
	PMICLOG("ldo : VA12; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_va18_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VA18; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VA18_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VA18_EN);
	PMICLOG("ldo : VA18; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vusb33_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VUSB33; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VUSB33_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VUSB33_EN);
	PMICLOG("ldo : VUSB33; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vemc_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VEMC; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VEMC_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VEMC_EN);
	PMICLOG("ldo : VEMC; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vxo22_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VXO22; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VXO22_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VXO22_EN);
	PMICLOG("ldo : VXO22; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vefuse_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VEFUSE; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VEFUSE_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VEFUSE_EN);
	PMICLOG("ldo : VEFUSE; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsim1_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSIM1; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSIM1_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSIM1_EN);
	PMICLOG("ldo : VSIM1; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsim2_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSIM2; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSIM2_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSIM2_EN);
	PMICLOG("ldo : VSIM2; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcamaf_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMAF; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMAF_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMAF_EN);
	PMICLOG("ldo : VCAMAF; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vtouch_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VTOUCH; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VTOUCH_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VTOUCH_EN);
	PMICLOG("ldo : VTOUCH; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcamd1_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMD1; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMD1_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMD1_EN);
	PMICLOG("ldo : VCAMD1; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcamd2_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMD2; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMD2_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMD2_EN);
	PMICLOG("ldo : VCAMD2; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcamio_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMIO; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMIO_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMIO_EN);
	PMICLOG("ldo : VCAMIO; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vmipi_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VMIPI; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VMIPI_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VMIPI_EN);
	PMICLOG("ldo : VMIPI; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vgp3_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VGP3; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VGP3_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VGP3_EN);
	PMICLOG("ldo : VGP3; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcn33_bt_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCN33_BT; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCN33_SW_EN_BT, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCN33_EN);
	PMICLOG("ldo : VCN33_BT; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcn33_wifi_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCN33_WIFI; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCN33_SW_EN_WIFI, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCN33_EN);
	PMICLOG("ldo : VCN33_WIFI; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcn18_bt_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCN18_BT; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCN18_SW_EN_BT, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCN18_EN);
	PMICLOG("ldo : VCN18_BT; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcn18_wifi_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCN18_WIFI; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCN18_SW_EN_WIFI, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCN18_EN);
	PMICLOG("ldo : VCN18_WIFI; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcn28_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCN28; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCN28_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCN28_EN);
	PMICLOG("ldo : VCN28; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vibr_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VIBR; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VIBR_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VIBR_EN);
	PMICLOG("ldo : VIBR; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vbif28_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VBIF28; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VBIF28_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VBIF28_EN);
	PMICLOG("ldo : VBIF28; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vfe28_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VFE28; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VFE28_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VFE28_EN);
	PMICLOG("ldo : VFE28; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vmch_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VMCH; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VMCH_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VMCH_EN);
	PMICLOG("ldo : VMCH; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vmc_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VMC; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VMC_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VMC_EN);
	PMICLOG("ldo : VMC; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vrf18_1_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VRF18_1; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VRF18_1_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VRF18_1_EN);
	PMICLOG("ldo : VRF18_1; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vrf18_2_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VRF18_2; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VRF18_2_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VRF18_2_EN);
	PMICLOG("ldo : VRF18_2; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vrf12_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VRF12; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VRF12_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VRF12_EN);
	PMICLOG("ldo : VRF12; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcama1_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMA1; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMA1_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMA1_EN);
	PMICLOG("ldo : VCAMA1; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vcama2_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VCAMA2; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VCAMA2_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VCAMA2_EN);
	PMICLOG("ldo : VCAMA2; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsram_dvfs1_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSRAM_DVFS1; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSRAM_DVFS1_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSRAM_DVFS1_EN);
	PMICLOG("ldo : VSRAM_DVFS1; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsram_dvfs2_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSRAM_DVFS2; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSRAM_DVFS2_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSRAM_DVFS2_EN);
	PMICLOG("ldo : VSRAM_DVFS2; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsram_vgpu_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSRAM_VGPU; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSRAM_VGPU_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSRAM_VGPU_EN);
	PMICLOG("ldo : VSRAM_VGPU; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsram_vcore_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSRAM_VCORE; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSRAM_VCORE_EN);
	PMICLOG("ldo : VSRAM_VCORE; sw en_value readback = %d\n", read_back);
}
void pmic_ldo_vsram_vmd_sw_en(int en_value)
{
	unsigned short read_back;

	PMICLOG("ldo : VSRAM_VMD; sw enable = %d\n", en_value);
	pmic_set_register_value(PMIC_RG_VSRAM_VMD_SW_EN, en_value);

	/* read back check */
	read_back =  pmic_get_register_value(PMIC_DA_QI_VSRAM_VMD_EN);
	PMICLOG("ldo : VSRAM_VMD; sw en_value readback = %d\n", read_back);
}

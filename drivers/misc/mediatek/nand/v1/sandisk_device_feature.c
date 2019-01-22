/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include "mtk_nand.h"
#include <linux/io.h>
#include "mtk_nand_util.h"
#include "mtk_nand_device_feature.h"

#ifdef SUPPORT_SANDISK_DEVICE

int g_sandisk_retry_case;	/*for new read retry table case 1, 2, 3, 4 */

/*sandisk 19nm read retry*/
u16 sandisk_19nm_rr_table[18] = {
	0x0000,
	0xFF0F, 0xEEFE, 0xDDFD, 0x11EE,
	0x22ED, 0x33DF, 0xCDDE, 0x01DD,
	0x0211, 0x1222, 0xBD21, 0xAD32,
	0x9DF0, 0xBCEF, 0xACDC, 0x9CFF,
	0x0000
};

u32 sandisk_tlc_rrtbl_12h[40] = {
	0x00000000, 0x08000004, 0x00000404, 0x04040408,
	0x08040408, 0x0004080C, 0x04040810, 0x0C0C0C00,
	0x0E0E0E00, 0x10101000, 0x12121200, 0x080808FC,
	0xFC08FCF8, 0x0000FBF6, 0x0408FBF4, 0xFEFCF8FA,
	0xFCF8F4EC, 0xF8F8F8EC, 0x0002FCE4, 0xFCFEFEFE,
	0xFFFC00FD, 0xFEFB00FC, 0xFEFAFEFA, 0xFDF9FDFA,
	0xFBF8FBFA, 0xF9F7FAF8, 0xF8F6F9F4, 0xF5F4F8F2,
	0xF4F2F6EE, 0xF0F0F4E8, 0xECECF0E6, 0x020400FA,
	0x00FEFFF8, 0xFEFEFDF6, 0xFDFDFCF4, 0xFBFCFCF2,
	0xF9FBFBF0, 0xF8F9F9EE, 0xF6F8F8ED, 0xF4F7F6EA,
};

u32 sandisk_tlc_rrtbl_13h[40] = {
	0x00000000, 0x00040800, 0x00080004, 0x00020404,
	0x00040800, 0x00080000, 0x00FC0000, 0x000C0C0C,
	0x000E0E0E, 0x00101010, 0x00141414, 0x000008FC,
	0x0004FCF8, 0x00FC00F6, 0x00FC0404, 0x00FCFE08,
	0x00FCFC00, 0x00F8F8FA, 0x000000F4, 0x00FAFC02,
	0x00F8FF00, 0x00F6FDFE, 0x00F4FBFC, 0x00F2F9FA,
	0x00F0F7F8, 0x00EEF5F6, 0x00ECF3F4, 0x00EAF1F2,
	0x00E8ECEE, 0x00E0E4E8, 0x00DAE0E2, 0x00000000,
	0x00FEFEFE, 0x00FBFCFC, 0x00F9FAFA, 0x00F7F8F8,
	0x00F5F6F6, 0x00F3F4F4, 0x00F1F2F2, 0x00EFF0EF,
};

u32 sandisk_tlc_rrtbl_14h[11] = {
	0x00000000, 0x00000010, 0x00000020, 0x00000030,
	0x00000040, 0x00000050, 0x00000060, 0x000000F0,
	0x000000E0, 0x000000D0, 0x000000C0,
};

u32 special_rrtry_setting[37] = {
	0x00000000, 0x7C00007C, 0x787C0004, 0x74780078,
	0x7C007C08, 0x787C7C00, 0x74787C7C, 0x70747C00,
	0x7C007800, 0x787C7800, 0x74787800, 0x70747800,
	0x6C707800, 0x00040400, 0x7C000400, 0x787C040C,
	0x7478040C, 0x7C000810, 0x00040810, 0x04040C0C,
	0x00040C10, 0x00081014, 0x000C1418, 0x7C040C0C,
	0x74787478, 0x70747478, 0x6C707478, 0x686C7478,
	0x74787078, 0x70747078, 0x686C7078, 0x6C707078,
	0x6C706C78, 0x686C6C78, 0x64686C78, 0x686C6874,
	0x64686874,
};

u32 special_mlcslc_rrtry_setting[23] = {
	0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x7C, 0x78,
	0x74, 0x18, 0x1C, 0x20, 0x70, 0x6C, 0x68, 0x24,
	0x28, 0x2C, 0x64, 0x60, 0x5C, 0x58, 0x54,
};

inline void sandisk_retry_case_inc(void)
{
	g_sandisk_retry_case++;
}

inline void sandisk_retry_case_reset(void)
{
	g_sandisk_retry_case = 0;
}


inline int get_sandisk_retry_case(void)
{
	return g_sandisk_retry_case;
}

u32 SANDISK_TRANSFER(u32 pageNo)
{
	if (pageNo == 0)
		return pageNo;
	else
		return pageNo+pageNo-1;
}

u32 sandisk_pairpage_mapping(u32 page, bool high_to_low)
{
	if (high_to_low == TRUE) {
		if (page == 255)
			return page-2;
		if ((page == 0) || (1 == (page%2)))
			return page;
		if (page == 2)
			return 0;
		return page - 3;
	}
	if (high_to_low != TRUE) {
		if ((page != 0) && (0 == (page%2)))
			return page;
		if (page == 255)
			return page;
		if (page == 0 || page == 253)
			return page + 2;
		return page+3;
	}
	return page + 3;
}

void mtk_nand_sandisk_tlc_1ynm_rrtry(struct mtd_info *mtd,
					struct flashdev_info_t deviceinfo, u32 feature, bool defValue)
{
	u16 reg_val = 0;
	u32	timeout = TIMEOUT_3;
	u32 value1, value2, value3;

	if ((feature > 1) || defValue) {
		mtk_nand_reset();
		reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

		mtk_nand_set_command(0x55);
		mtk_nand_set_address(0, 0, 1, 0);
		DRV_WriteReg32(NFI_CON_REG32, 1 << CON_NFI_SEC_SHIFT);
		NFI_SET_REG32(NFI_CON_REG32, CON_NFI_BRD | CON_NFI_BWR);
		DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);
		WAIT_NFI_PIO_READY(timeout);
		if (timeout == 0)
			MSG(INIT, "mtk_nand_sandisk_tlc_1ynm_rrtry: timeout\n");

		DRV_WriteReg32(NFI_DATAW_REG32, 0);

		mtk_nand_device_reset();
		mtk_nand_interface_async();
		/*should do NAND DEVICE interface change under sync mode */
	}

	if (gn_devinfo.tlcControl.slcopmodeEn) {
		value3 = sandisk_tlc_rrtbl_14h[feature];
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x14, (u8 *)&value3, 4);
	} else {
		value1 = sandisk_tlc_rrtbl_12h[feature];
		value2 = sandisk_tlc_rrtbl_13h[feature];
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x12, (u8 *)&value1, 4);
		mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd, 0x13, (u8 *)&value2, 4);
	}

	if (defValue == FALSE) {
		mtk_nand_reset();
		reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
		DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

		mtk_nand_set_command(0x5D);

		mtk_nand_reset();
	}
}
EXPORT_SYMBOL(mtk_nand_sandisk_tlc_1ynm_rrtry);


static void mtk_nand_sandisk_rrtry_set(struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
				bool defValue)
{
	if (defValue == FALSE)
		mtk_nand_reset();
	else {
		mtk_nand_device_reset();
		mtk_nand_interface_async();
		mtk_nand_reset();
		/*should do NAND DEVICE interface change under sync mode */
	}

	mtk_nand_SetFeature(mtd, deviceinfo.feature_set.FeatureSet.sfeatureCmd,
				deviceinfo.feature_set.FeatureSet.readRetryAddress,
				(u8 *) &feature, 4);
	if (defValue == FALSE) {
		if (g_sandisk_retry_case > 1) {
			if (g_sandisk_retry_case == 3) {
				u32 timeout = TIMEOUT_3;

				mtk_nand_reset();
				DRV_WriteReg16(NFI_CNFG_REG16, (CNFG_OP_CUST | CNFG_BYTE_RW));
				mtk_nand_set_command(0x5C);
				mtk_nand_set_command(0xC5);
				mtk_nand_set_command(0x55);
				mtk_nand_set_address(0x00, 0, 1, 0);	/* test mode entry */
				mtk_nand_status_ready(STA_NFI_OP_MASK);
				DRV_WriteReg32(NFI_CON_REG32, 1 << CON_NFI_SEC_SHIFT);
				NFI_SET_REG32(NFI_CON_REG32, CON_NFI_BRD | CON_NFI_BWR);
				DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);
				WAIT_NFI_PIO_READY(timeout);
				DRV_WriteReg8(NFI_DATAW_REG32, 0x01);
				while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN)
					&& (timeout--))
					;
				mtk_nand_reset();
				timeout = TIMEOUT_3;
				mtk_nand_set_command(0x55);
				mtk_nand_set_address(0x23, 0, 1, 0);
				/*changing parameter LMFLGFIX_NEXT = 1 to all die */
				mtk_nand_status_ready(STA_NFI_OP_MASK);
				DRV_WriteReg32(NFI_CON_REG32, 1 << CON_NFI_SEC_SHIFT);
				NFI_SET_REG32(NFI_CON_REG32, CON_NFI_BRD | CON_NFI_BWR);
				DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);
				WAIT_NFI_PIO_READY(timeout);
				DRV_WriteReg8(NFI_DATAW_REG32, 0xC0);
				while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN)
					&& (timeout--))
					;
				mtk_nand_reset();
				/* pr_debug("Case3# Set LMFLGFIX_NEXT = 1\n"); */
			}
			mtk_nand_set_command(0x25);
			/* pr_debug("Case2#3# Set cmd 25\n"); */
		}
		mtk_nand_set_command(deviceinfo.feature_set.FeatureSet.readRetryPreCmd);
	}
}

void mtk_nand_sandisk_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 feature,
				bool defValue)
{
	if (gn_devinfo.tlcControl.slcopmodeEn)
		mtk_nand_sandisk_rrtry_set(mtd, deviceinfo, special_mlcslc_rrtry_setting[feature], defValue);
	else
		mtk_nand_sandisk_rrtry_set(mtd, deviceinfo, special_rrtry_setting[feature], defValue);
}
EXPORT_SYMBOL(mtk_nand_sandisk_rrtry);

void sandisk_19nm_rr_init(void)
{
	u32		  reg_val		 = 0;
	u32		count	  = 0;
	u32		  timeout = 0xffff;
	u32 acccon;

	acccon = DRV_Reg32(NFI_ACCCON_REG32);

	mtk_nand_reset();

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
	mtk_nand_set_command(0x3B);
	mtk_nand_set_command(0xB9);

	for (count = 0; count < 9; count++) {
		mtk_nand_set_command(0x53);
		mtk_nand_set_address((0x04 + count), 0, 1, 0);
		DRV_WriteReg32(NFI_CON_REG32, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg16(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		DRV_WriteReg32(NFI_DATAW_REG32, 0x00);
		mtk_nand_reset();
	}

}

void sandisk_19nm_rr_loading(u32 retryCount, bool defValue)
{
	u32 reg_val = 0;
	u32 timeout = 0xffff;
	u32 acccon;
	u8 count;
	u8 cmd_reg[4] = { 0x4, 0x5, 0x7 };

	acccon = DRV_Reg32(NFI_ACCCON_REG32);

	mtk_nand_reset();

	reg_val = (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

	if ((retryCount != 0) || defValue)
		mtk_nand_set_command(0xD6);

	mtk_nand_set_command(0x3B);
	mtk_nand_set_command(0xB9);
	for (count = 0; count < 3; count++) {
		mtk_nand_set_command(0x53);
		mtk_nand_set_address(cmd_reg[count], 0, 1, 0);
		DRV_WriteReg32(NFI_CON_REG32, (CON_NFI_BWR | (1 << CON_NFI_SEC_SHIFT)));
		DRV_WriteReg16(NFI_STRDATA_REG16, 1);
		timeout = 0xffff;
		WAIT_NFI_PIO_READY(timeout);
		if (count == 0)
			DRV_WriteReg32(NFI_DATAW_REG32,
					(((sandisk_19nm_rr_table[retryCount] & 0xF000) >> 8) |
					((sandisk_19nm_rr_table[retryCount] & 0x00F0) >> 4)));
		else if (count == 1)
			DRV_WriteReg32(NFI_DATAW_REG32,
					((sandisk_19nm_rr_table[retryCount] & 0x000F) << 4));
		else if (count == 2)
			DRV_WriteReg32(NFI_DATAW_REG32,
					((sandisk_19nm_rr_table[retryCount] & 0x0F00) >> 4));

		mtk_nand_reset();
	}

	if (!defValue)
		mtk_nand_set_command(0xB6);

}

void mtk_nand_sandisk_19nm_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo,
					u32 retryCount, bool defValue)
{
	if ((retryCount == 0) && (!defValue))
		sandisk_19nm_rr_init();
	sandisk_19nm_rr_loading(retryCount, defValue);
}
EXPORT_SYMBOL(mtk_nand_sandisk_19nm_rrtry);

#endif


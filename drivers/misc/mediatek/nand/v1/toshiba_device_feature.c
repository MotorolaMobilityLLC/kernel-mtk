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

#ifdef SUPPORT_TOSHIBA_DEVICE
static const u8 tsb_mlc[8][5] = {
	{0x04, 0x04, 0x7C, 0x7E, 0x00},
	{0x00, 0x7C, 0x78, 0x78, 0x00},
	{0x7C, 0x76, 0x74, 0x72, 0x00},
	{0x08, 0x08, 0x00, 0x00, 0x00},
	{0x0B, 0x7E, 0x76, 0x74, 0x00},
	{0x10, 0x76, 0x72, 0x70, 0x00},
	{0x02, 0x7C, 0x7E, 0x70, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0x00}
};

static const u8 tsb_tlc_pslc_1y[8][2] = {
	{0xF0, 0x00},
	{0xE0, 0x00},
	{0xD0, 0x00},
	{0xC0, 0x00},
	{0x20, 0x00},
	{0x30, 0x00},
	{0x40, 0x00},
	{0x00, 0x00}
};

static const u8 tb_tlc_tlc_1y[31][7] = {
		{0xFE, 0x03, 0x02, 0x02, 0xFF, 0xFC, 0xFD},
		{0xFE, 0x02, 0x01, 0x01, 0xFE, 0xFA, 0xFB},
		{0xFE, 0x00, 0x00, 0xFF, 0xFC, 0xF8, 0xF9},
		{0xFD, 0xFF, 0xFE, 0xFE, 0xFA, 0xF6, 0xF7},
		{0xFD, 0xFE, 0xFD, 0xFC, 0xF8, 0xF4, 0xF5},
		{0xFD, 0xFD, 0xFC, 0xFB, 0xF6, 0xF2, 0xF2},
		{0xFD, 0xFB, 0xFB, 0xF9, 0xF5, 0xF0, 0xF0},
		{0xFD, 0xFA, 0xF9, 0xF8, 0xF3, 0xEE, 0xEE},
		{0xFD, 0xF9, 0xF8, 0xF6, 0xF1, 0xEC, 0xEC},
		{0xFD, 0xF8, 0xF7, 0xF5, 0xEF, 0xEA, 0xE9},
		{0xFC, 0xF6, 0xF6, 0xF3, 0xEE, 0xE8, 0xE7},
		{0xFA, 0xFA, 0xFB, 0xFA, 0xFB, 0xFA, 0xFA},
		{0xFA, 0xFA, 0xFA, 0xF9, 0xFA, 0xF8, 0xF8},
		{0xFA, 0xFA, 0xFA, 0xF8, 0xF9, 0xF6, 0xF5},
		{0xFB, 0xFA, 0xF9, 0xF7, 0xF7, 0xF4, 0xF3},
		{0xFB, 0xFB, 0xF9, 0xF6, 0xF6, 0xF2, 0xF0},
		{0xFB, 0xFB, 0xF8, 0xF5, 0xF5, 0xF0, 0xEE},
		{0xFB, 0xFB, 0xF8, 0xF5, 0xF4, 0xEE, 0xEB},
		{0xFC, 0xFB, 0xF7, 0xF4, 0xF2, 0xEC, 0xE9},
		{0xFC, 0xFE, 0xFE, 0xF9, 0xFA, 0xF8, 0xF8},
		{0xFD, 0xFE, 0xFD, 0xF7, 0xF7, 0xF4, 0xF3},
		{0xFD, 0xFF, 0xFC, 0xF5, 0xF5, 0xF0, 0xEE},
		{0xFE, 0x03, 0x03, 0x04, 0x01, 0xFF, 0x01},
		{0xFC, 0x00, 0x00, 0x01, 0xFE, 0xFC, 0xFE},
		{0xFA, 0xFA, 0xFC, 0xFC, 0xFA, 0xF7, 0xFA},
		{0x00, 0x03, 0x02, 0x03, 0xFF, 0xFC, 0xFE},
		{0x04, 0x03, 0x03, 0x03, 0x00, 0xFC, 0xFD},
		{0x08, 0x04, 0x03, 0x04, 0x00, 0xFC, 0xFC},
		{0xFC, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08},
		{0xF8, 0x00, 0x00, 0x00, 0x08, 0x08, 0x10},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

u32 TOSHIBA_TRANSFER(u32 pageNo)
{
	if (pageNo == 0)
		return pageNo;
	else
		return pageNo+pageNo-1;
}

u32 toshiba_pairpage_mapping(u32 page, bool high_to_low)
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

static void mtk_nand_modeentry_rrtry(void)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5C);
	mtk_nand_set_command(0xC5);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_rren_rrtry(bool needB3)
{
	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if (needB3)
		mtk_nand_set_command(0xB3);
	mtk_nand_set_command(0x26);
	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);
}

static void mtk_nand_sprmset_rrtry(u32 addr, u32 data)
{
	u16 reg_val = 0;
	u32 timeout = TIMEOUT_3;	/*0xffff; */

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);
	mtk_nand_set_command(0x55);
	mtk_nand_set_address(addr, 0, 1, 0);
	mtk_nand_status_ready(STA_NFI_OP_MASK);
	DRV_WriteReg32(NFI_CON_REG32, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG32, CON_NFI_BRD | CON_NFI_BWR);
	DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);
	WAIT_NFI_PIO_READY(timeout);
	timeout = TIMEOUT_3;
	DRV_WriteReg8(NFI_DATAW_REG32, data);

	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;
}

void mtk_nand_toshiba_rrtry(struct mtd_info *mtd, flashdev_info deviceinfo, u32 retryCount,
				bool defValue)
{
	u32 acccon;
	u8 cnt = 0;
	u8 add_reg[6] = {0x04, 0x05, 0x06, 0x07, 0x0D};

	acccon = DRV_Reg32(NFI_ACCCON_REG32);

	if (retryCount == 0)
		mtk_nand_modeentry_rrtry();

	for (cnt = 0; cnt < 5; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], tsb_mlc[retryCount][cnt]);

	if (retryCount == 3)
		mtk_nand_rren_rrtry(TRUE);
	else if (retryCount < 6)
		mtk_nand_rren_rrtry(FALSE);

	if (retryCount == 7) {
		mtk_nand_device_reset();
		mtk_nand_reset();
	}

}
EXPORT_SYMBOL(mtk_nand_toshiba_rrtry);


static void mtk_nand_modeentry_tlc_rrtry(void)
{
	u32	reg_val = 0;
	u32	timeout = TIMEOUT_3;

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5C);
	mtk_nand_set_command(0xC5);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	mtk_nand_reset();

	reg_val |= (CNFG_OP_CUST | CNFG_BYTE_RW);
	DRV_WriteReg16(NFI_CNFG_REG16, reg_val);

	mtk_nand_set_command(0x55);
	mtk_nand_set_address(0, 0, 1, 0);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

	DRV_WriteReg32(NFI_CON_REG32, 1 << CON_NFI_SEC_SHIFT);
	NFI_SET_REG32(NFI_CON_REG32, CON_NFI_BRD | CON_NFI_BWR);
	DRV_WriteReg16(NFI_STRDATA_REG16, 0x1);


	WAIT_NFI_PIO_READY(timeout);
	timeout = TIMEOUT_3;
	DRV_WriteReg8(NFI_DATAW_REG32, 0x01);

	while (!(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY_RETURN) && (timeout--))
		;
}


static void mtk_nand_toshiba_tlc_1y_rrtry(flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	u8 add_reg[7] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
	u8 cnt = 0;

	if (defValue == TRUE) {
		for (cnt = 0; cnt < 7; cnt++)
			mtk_nand_sprmset_rrtry(add_reg[cnt], tb_tlc_tlc_1y[30][cnt]);
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND(NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
		return;
	}

	if (retryCount == 0)
		mtk_nand_modeentry_tlc_rrtry();

	for (cnt = 0; cnt < 7; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], tb_tlc_tlc_1y[retryCount][cnt]);

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	if (retryCount == 31)
		mtk_nand_set_command(0xB3);
	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

}

static void mtk_nand_toshiba_slc_1y_rrtry(flashdev_info deviceinfo, u32 retryCount, bool defValue)
{
	u8 add_reg[2] = {0x0B, 0x0D};
	u8 cnt = 0;

	if (defValue == TRUE) {
		for (cnt = 0; cnt < 2; cnt++)
			mtk_nand_sprmset_rrtry(add_reg[cnt], tsb_tlc_pslc_1y[7][cnt]);
		mtk_nand_set_mode(CNFG_OP_RESET);
		NFI_ISSUE_COMMAND(NAND_CMD_RESET, 0, 0, 0, 0);
		mtk_nand_reset();
	}

	if (retryCount == 0)
		mtk_nand_modeentry_tlc_rrtry();

	for (cnt = 0; cnt < 2; cnt++)
		mtk_nand_sprmset_rrtry(add_reg[cnt], tsb_tlc_pslc_1y[retryCount][cnt]);

	mtk_nand_reset();

	mtk_nand_set_mode(CNFG_OP_CUST);

	mtk_nand_set_command(0x5D);

	mtk_nand_status_ready(STA_NFI_OP_MASK);

}

void mtk_nand_toshiba_tlc_rrtry(struct mtd_info *mtd, struct flashdev_info_t deviceinfo,
				u32 retryCount, bool defValue)
{
	if (gn_devinfo.tlcControl.slcopmodeEn)
		mtk_nand_toshiba_slc_1y_rrtry(deviceinfo, retryCount, defValue);
	else
		mtk_nand_toshiba_tlc_1y_rrtry(deviceinfo, retryCount, defValue);
}
EXPORT_SYMBOL(mtk_nand_toshiba_tlc_rrtry);

#endif


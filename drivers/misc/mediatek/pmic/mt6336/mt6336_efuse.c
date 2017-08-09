/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/delay.h>
#include <linux/mutex.h>

#include "include/mt6336/mt6336.h"
#include "include/mt6336/mt6336_upmu_hw.h"

/*****************************************************************************
 * MT6336 EFUSE read
 ******************************************************************************/
static DEFINE_MUTEX(mt6336_efuse_lock_mutex);
unsigned short mt6336_Read_Efuse_HPOffset(int i)
{
	unsigned short efusevalue;

	pr_debug("mt6336_Read_Efuse_HPOffset(+)\n");
	mutex_lock(&mt6336_efuse_lock_mutex);
	/*1. enable efuse ctrl engine clock */
	mt6336_set_flag_register_value(MT6336_PMIC_CLK_CKPDN_HWEN_CON0_CLR,
		MT6336_CON_BIT(MT6336_CLK_EFUSE_CK_PDN_HWEN));
	mt6336_set_flag_register_value(MT6336_PMIC_CLK_CKPDN_CON1_CLR,
		MT6336_CON_BIT(MT6336_CLK_EFUSE_CK_PDN));
	/*2. */
	mt6336_set_flag_register_value(MT6336_RG_OTP_RD_SW, 1);
	/*3. set row to read */
	mt6336_set_flag_register_value(MT6336_RG_OTP_PA, i*2);
	/*4. Toggle */
	udelay(100);
	if (mt6336_get_flag_register_value(MT6336_RG_OTP_RD_TRIG) == 0)
		mt6336_set_flag_register_value(MT6336_RG_OTP_RD_TRIG, 1);
	else
		mt6336_set_flag_register_value(MT6336_RG_OTP_RD_TRIG, 0);
	/*5. polling Reg[0x61A] */
	udelay(300);
	while (mt6336_get_flag_register_value(MT6336_RG_OTP_RD_BUSY) == 1)
		;
	udelay(1000);		/*Need to delay at least 1ms for 0x61A and than can read 0xC18 */
	/*6. read data */
	efusevalue = mt6336_get_flag_register_value(MT6336_RG_OTP_DOUT_SW);
	pr_debug("HPoffset : efuse=0x%x\n", efusevalue);
	/*7. Disable efuse ctrl engine clock */
	mt6336_set_flag_register_value(MT6336_PMIC_CLK_CKPDN_HWEN_CON0_SET,
		MT6336_CON_BIT(MT6336_CLK_EFUSE_CK_PDN_HWEN));
	mt6336_set_flag_register_value(MT6336_PMIC_CLK_CKPDN_CON1_SET,
		MT6336_CON_BIT(MT6336_CLK_EFUSE_CK_PDN));
	mutex_unlock(&mt6336_efuse_lock_mutex);
	return efusevalue;
}

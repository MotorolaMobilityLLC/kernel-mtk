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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    pmic.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines PMIC functions
 *
 * Author:
 * -------
 * Argus Lin
 *
 ****************************************************************************/
#include "linux/delay.h"
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_chip.h>
#ifdef CONFIG_OF
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#endif
#define PMIC_32K_LESS_DETECT_V1      1
#define PMIC_CO_TSX_V1               1

#define PMIC_READ_REGISTER_UINT32(reg)	(*(volatile uint32_t *const)(reg))
#define PMIC_INREG32(x)	PMIC_READ_REGISTER_UINT32((uint32_t *)((void *)(x)))
#define PMIC_WRITE_REGISTER_UINT32(reg, val)	((*(volatile uint32_t *const)(reg)) = (val))
#define PMIC_OUTREG32(x, y)	PMIC_WRITE_REGISTER_UINT32((uint32_t *)((void *)(x)), (uint32_t)(y))

#define PMIC_DRV_Reg32(addr)             PMIC_INREG32(addr)
#define PMIC_DRV_WriteReg32(addr, data)  PMIC_OUTREG32(addr, data)

int PMIC_MD_INIT_SETTING_V1(void)
{
	unsigned int ret = 0;
	unsigned int pmic_reg = 0;
#if PMIC_CO_TSX_V1
	struct device_node *modem_temp_node = NULL;
	void __iomem *modem_temp_base = NULL;
#endif

#if PMIC_32K_LESS_DETECT_V1

	/* 32k less crystal auto detect start */
	ret |= pmic_config_interface(0x701E, 0x1, 0x1, 0);
	ret |= pmic_config_interface(0x701E, 0x3, 0xF, 1);
	ret = pmic_read_interface(0x7000, &pmic_reg, 0xffff, 0);
	ret |= pmic_config_interface(0x701E, 0x0, 0x1, 0);
	ret = pmic_config_interface(0xA04, 0x1, 0x1, 3);
	if ((pmic_reg & 0x200) == 0x200) {
		/* VCTCXO on MT6176 , OFF XO on MT6351
		    HW control, use srclken_0 */

		ret = pmic_config_interface(0xA04, 0x0, 0x7, 11);
		pr_err("[PMIC] VCTCXO on MT6176 , OFF XO on MT6351\n");
	} else {
		/*  HW control, use srclken_1, for LP */
		ret = pmic_config_interface(0xA04, 0x1, 0x1, 4);
		ret = pmic_config_interface(0xA04, 0x1, 0x7, 11);
		pr_err("[PMIC] VCTCXO 0x7000=0x%x\n", pmic_reg);
	}
#endif

#if PMIC_CO_TSX_V1
	modem_temp_node = of_find_compatible_node(NULL, NULL, "mediatek,MODEM_TEMP_SHARE");

	if (modem_temp_node == NULL) {
		pr_err("PMIC get modem_temp_node failed\n");
		modem_temp_base = 0;
	} else {
		modem_temp_base = of_iomap(modem_temp_node, 0);
	}
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base, 0x011f);
	pr_err("[PMIC] TEMP_SHARE_CTRL:0x%x\n", PMIC_DRV_Reg32(modem_temp_base));
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base + 0x04, 0x013f);
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base, 0x0);
	pr_err("[PMIC] TEMP_SHARE_CTRL:0x%x _RATIO:0x%x\n", PMIC_DRV_Reg32(modem_temp_base),
		PMIC_DRV_Reg32(modem_temp_base + 0x04));
#endif
	return ret;
}

void PMIC_upmu_set_rg_baton_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface((unsigned int)(MT6351_PMIC_RG_BATON_EN_ADDR),
		(unsigned int)(val),
		(unsigned int)(MT6351_PMIC_RG_BATON_EN_MASK),
		(unsigned int)(MT6351_PMIC_RG_BATON_EN_SHIFT)
		);
}

void PMIC_upmu_set_baton_tdet_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface((unsigned int)(MT6351_PMIC_BATON_TDET_EN_ADDR),
		(unsigned int)(val),
		(unsigned int)(MT6351_PMIC_BATON_TDET_EN_MASK),
		(unsigned int)(MT6351_PMIC_BATON_TDET_EN_SHIFT)
	);
}

unsigned int PMIC_upmu_get_rgs_baton_undet(void)
{
	unsigned int ret = 0;
	unsigned int val = 0;

	ret = pmic_read_interface((unsigned int)(MT6351_PMIC_RGS_BATON_UNDET_ADDR),
		(&val),
		(unsigned int)(MT6351_PMIC_RGS_BATON_UNDET_MASK),
		(unsigned int)(MT6351_PMIC_RGS_BATON_UNDET_SHIFT)
	);
	return val;
}

int PMIC_check_battery(void)
{
	unsigned int val = 0;

	/* ask shin-shyu programming guide */
	PMIC_upmu_set_rg_baton_en(1);
	PMIC_upmu_set_baton_tdet_en(1);
	val = PMIC_upmu_get_rgs_baton_undet();
	if (val == 0) {
		pr_debug("bat is exist.\n");
		return 1;
	}
	pr_debug("bat NOT exist.\n");
	return 0;
}

void PMIC_INIT_SETTING_V1(void)
{
	unsigned int chip_version = 0;
	unsigned int ret = 0;

	chip_version = pmic_get_register_value(PMIC_SWCID);

	/* This flag is used for fg to judge if battery even removed manually */
	is_battery_remove = !pmic_get_register_value(PMIC_STRUP_PWROFF_SEQ_EN);
	is_wdt_reboot_pmic = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	pmic_set_register_value(PMIC_TOP_RST_MISC_SET,  0x8);
	is_wdt_reboot_pmic_chk = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	pmic_set_register_value(PMIC_TOP_RST_MISC_CLR,  0x8);
	/*--------------------------------------------------------*/

	pr_err("[PMIC] 6351 PMIC Chip = 0x%x\n",  chip_version);
	pr_err("[PMIC] 2015-06-17...\n");
	pr_err("[PMIC] is_battery_remove =%d is_wdt_reboot=%d\n",
	is_battery_remove,  is_wdt_reboot_pmic);
	pr_err("[PMIC] is_wdt_reboot_chk=%d\n",
	is_wdt_reboot_pmic_chk);

	pmic_set_register_value(PMIC_TOP_RST_MISC_SET,  0x1);

ret = pmic_config_interface(0x8, 0x1, 0x1, 0);
ret = pmic_config_interface(0xA, 0x1, 0x1, 1);
ret = pmic_config_interface(0xA, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA, 0x1, 0x1, 3);
ret = pmic_config_interface(0xA, 0x1, 0x1, 4);
ret = pmic_config_interface(0xA, 0x1, 0x1, 5);
ret = pmic_config_interface(0xA, 0x1, 0x1, 7);
ret = pmic_config_interface(0xA, 0x1, 0x1, 8);
ret = pmic_config_interface(0xA, 0x1, 0x1, 10);
ret = pmic_config_interface(0xA, 0x1, 0x1, 11);
ret = pmic_config_interface(0xA, 0x1, 0x1, 12);
ret = pmic_config_interface(0xA, 0x1, 0x1, 13);
ret = pmic_config_interface(0xA, 0x1, 0x1, 14);
ret = pmic_config_interface(0xA, 0x1, 0x1, 15);
ret = pmic_config_interface(0x18, 0x1, 0x1, 5);
ret = pmic_config_interface(0x1C, 0x1, 0x1, 7);
ret = pmic_config_interface(0x1E, 0x1, 0x1, 0);
ret = pmic_config_interface(0x1E, 0x1, 0x1, 1);
ret = pmic_config_interface(0x2C, 0x1, 0x1, 15);
ret = pmic_config_interface(0x32, 0x1, 0x1, 2);
ret = pmic_config_interface(0x32, 0x1, 0x1, 3);
ret = pmic_config_interface(0x204, 0x1, 0x1, 4);
ret = pmic_config_interface(0x204, 0x1, 0x1, 5);
ret = pmic_config_interface(0x204, 0x1, 0x1, 6);
ret = pmic_config_interface(0x226, 0x1, 0x1, 0);
ret = pmic_config_interface(0x228, 0x1, 0x1, 0);
ret = pmic_config_interface(0x228, 0x1, 0x1, 1);
ret = pmic_config_interface(0x228, 0x1, 0x1, 2);
ret = pmic_config_interface(0x228, 0x1, 0x1, 3);
ret = pmic_config_interface(0x23A, 0x0, 0x1, 9);
ret = pmic_config_interface(0x23A, 0x1, 0x1, 11);
ret = pmic_config_interface(0x240, 0x1, 0x1, 2);
ret = pmic_config_interface(0x240, 0x1, 0x1, 3);
ret = pmic_config_interface(0x240, 0x0, 0x1, 10);
ret = pmic_config_interface(0x246, 0x1, 0x1, 13);
ret = pmic_config_interface(0x246, 0x1, 0x1, 14);
ret = pmic_config_interface(0x24C, 0x0, 0x1, 2);
ret = pmic_config_interface(0x258, 0x0, 0x1, 5);
ret = pmic_config_interface(0x258, 0x1, 0x1, 8);
ret = pmic_config_interface(0x258, 0x0, 0x1, 14);
ret = pmic_config_interface(0x25E, 0x1, 0x1, 9);
ret = pmic_config_interface(0x25E, 0x0, 0x1, 10);
ret = pmic_config_interface(0x282, 0x0, 0x1, 3);
ret = pmic_config_interface(0x282, 0x0, 0x1, 4);
ret = pmic_config_interface(0x282, 0x0, 0x1, 10);
ret = pmic_config_interface(0x282, 0x0, 0x1, 11);
ret = pmic_config_interface(0x28E, 0x0, 0x1, 4);
ret = pmic_config_interface(0x410, 0x8, 0x3F, 8);
ret = pmic_config_interface(0x414, 0x3, 0x3, 4);
ret = pmic_config_interface(0x422, 0x1, 0x1, 7);
ret = pmic_config_interface(0x436, 0x0, 0x3, 2);
ret = pmic_config_interface(0x450, 0xF, 0xF, 11);
ret = pmic_config_interface(0x452, 0x1, 0x1, 3);
ret = pmic_config_interface(0x456, 0x1, 0x1, 7);
ret = pmic_config_interface(0x45C, 0x1, 0x1, 1);
ret = pmic_config_interface(0x45E, 0x400, 0xFFFF, 0);
ret = pmic_config_interface(0x464, 0xF, 0xF, 11);
ret = pmic_config_interface(0x466, 0x1, 0x1, 3);
ret = pmic_config_interface(0x466, 0x5, 0x7, 9);
ret = pmic_config_interface(0x46A, 0x1, 0x1, 7);
ret = pmic_config_interface(0x470, 0x1, 0x1, 1);
ret = pmic_config_interface(0x472, 0x400, 0xFFFF, 0);
ret = pmic_config_interface(0x478, 0xF, 0xF, 11);
ret = pmic_config_interface(0x47A, 0x2, 0x7, 6);
ret = pmic_config_interface(0x47E, 0x1, 0x1, 1);
ret = pmic_config_interface(0x480, 0x1, 0x7, 4);
ret = pmic_config_interface(0x484, 0x1, 0x1, 1);
ret = pmic_config_interface(0x48C, 0xF, 0xF, 11);
ret = pmic_config_interface(0x48E, 0x2, 0x7, 6);
ret = pmic_config_interface(0x492, 0x1, 0x1, 1);
ret = pmic_config_interface(0x494, 0x1, 0x7, 4);
ret = pmic_config_interface(0x498, 0x1, 0x1, 1);
ret = pmic_config_interface(0x4A0, 0xF, 0xF, 11);
ret = pmic_config_interface(0x4A2, 0x2, 0x7, 6);
ret = pmic_config_interface(0x4A6, 0x1, 0x1, 1);
ret = pmic_config_interface(0x4A8, 0x1, 0x7, 4);
ret = pmic_config_interface(0x4AC, 0x1, 0x1, 1);
ret = pmic_config_interface(0x4B6, 0x6, 0x7, 6);
ret = pmic_config_interface(0x4C2, 0x10, 0xFFFF, 0);
ret = pmic_config_interface(0x4C8, 0xF, 0xF, 11);
ret = pmic_config_interface(0x4CA, 0x5, 0x7, 6);
ret = pmic_config_interface(0x4CE, 0x1, 0x1, 1);
ret = pmic_config_interface(0x4D0, 0x1, 0x7, 4);
ret = pmic_config_interface(0x4DC, 0x3, 0x3, 0);
ret = pmic_config_interface(0x4DC, 0x2, 0x3, 4);
ret = pmic_config_interface(0x4DC, 0x0, 0x1, 10);
ret = pmic_config_interface(0x4DC, 0x1, 0x3, 14);
ret = pmic_config_interface(0x4E0, 0x0, 0x3, 14);
ret = pmic_config_interface(0x4E2, 0x88, 0xFF, 8);
ret = pmic_config_interface(0x600, 0x1, 0x1, 1);
ret = pmic_config_interface(0x606, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x606, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x60C, 0x10, 0x7F, 0);
ret = pmic_config_interface(0x612, 0x3, 0x3, 0);
ret = pmic_config_interface(0x612, 0x1, 0x1, 8);
ret = pmic_config_interface(0x61A, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x61A, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x620, 0x10, 0x7F, 0);
ret = pmic_config_interface(0x626, 0x3, 0x3, 0);
ret = pmic_config_interface(0x626, 0x1, 0x3, 4);
ret = pmic_config_interface(0x62E, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x62E, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x634, 0x10, 0x7F, 0);
ret = pmic_config_interface(0x63A, 0x3, 0x3, 0);
ret = pmic_config_interface(0x63A, 0x1, 0x3, 4);
ret = pmic_config_interface(0x63A, 0x1, 0x1, 8);
ret = pmic_config_interface(0x642, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x642, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x646, 0x30, 0x7F, 0);
ret = pmic_config_interface(0x648, 0x10, 0x7F, 0);
ret = pmic_config_interface(0x64E, 0x3, 0x3, 0);
ret = pmic_config_interface(0x64E, 0x1, 0x3, 4);
ret = pmic_config_interface(0x64E, 0x1, 0x1, 8);
ret = pmic_config_interface(0x656, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x656, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x65C, 0x10, 0x7F, 0);
ret = pmic_config_interface(0x662, 0x3, 0x3, 0);
ret = pmic_config_interface(0x662, 0x1, 0x3, 4);
ret = pmic_config_interface(0x662, 0x1, 0x1, 8);
ret = pmic_config_interface(0x676, 0x1, 0x1, 8);
ret = pmic_config_interface(0x68A, 0x1, 0x1, 8);
ret = pmic_config_interface(0x692, 0x0, 0x7F, 0);
ret = pmic_config_interface(0x692, 0x1, 0x7F, 8);
ret = pmic_config_interface(0x69E, 0x0, 0x3, 0);
ret = pmic_config_interface(0x6A0, 0x1, 0x1, 1);
ret = pmic_config_interface(0x6A6, 0x11, 0x7F, 0);
ret = pmic_config_interface(0x6A6, 0x4, 0x7F, 8);
ret = pmic_config_interface(0x6AC, 0x35, 0x7F, 0);
ret = pmic_config_interface(0x6B2, 0x3, 0x3, 0);
ret = pmic_config_interface(0x6B2, 0x1, 0x3, 4);
ret = pmic_config_interface(0x6B2, 0x0, 0x1, 8);
ret = pmic_config_interface(0xA00, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA00, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA02, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA04, 0x1, 0x1, 3);
ret = pmic_config_interface(0xA04, 0x0, 0x7, 11);
ret = pmic_config_interface(0xA06, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA08, 0x1, 0x1, 3);
ret = pmic_config_interface(0xA08, 0x1, 0x7, 11);
ret = pmic_config_interface(0xA0A, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA0E, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA14, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA16, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA16, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA18, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA1E, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA24, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA2A, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA30, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA34, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA34, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA36, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA3C, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA46, 0x1, 0x1, 3);
ret = pmic_config_interface(0xA46, 0x1, 0x7, 11);
ret = pmic_config_interface(0xA48, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA4C, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA4C, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA4E, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA54, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA5A, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA66, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA68, 0x1, 0x1, 3);
ret = pmic_config_interface(0xA68, 0x1, 0x7, 11);
ret = pmic_config_interface(0xA6E, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA6E, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA74, 0x1, 0x1, 1);
ret = pmic_config_interface(0xA74, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA74, 0x0, 0x1, 3);
ret = pmic_config_interface(0xA74, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA7C, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA82, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA86, 0x0, 0x1, 3);
ret = pmic_config_interface(0xA88, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA8E, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA94, 0x1, 0x1, 9);
ret = pmic_config_interface(0xA9C, 0x1, 0x1, 2);
ret = pmic_config_interface(0xA9C, 0x0, 0x7, 5);
ret = pmic_config_interface(0xA9E, 0x1, 0x1, 9);
ret = pmic_config_interface(0xAAC, 0x1, 0x1, 9);
ret = pmic_config_interface(0xB10, 0x2, 0x7, 8);
ret = pmic_config_interface(0xCC4, 0x1, 0x1, 8);
ret = pmic_config_interface(0xCC4, 0x1, 0x1, 9);
ret = pmic_config_interface(0xCC8, 0x1F, 0xFFFF, 0);
ret = pmic_config_interface(0xCCA, 0x14, 0xFF, 0);
ret = pmic_config_interface(0xCCC, 0xFF, 0xFF, 8);
ret = pmic_config_interface(0xCE2, 0x1, 0x7FFF, 0);
ret = pmic_config_interface(0xCE4, 0xBCAC, 0xFFFF, 0);
ret = pmic_config_interface(0xEA2, 0x0, 0x1, 13);
ret = pmic_config_interface(0xEA2, 0x0, 0x1, 14);
ret = pmic_config_interface(0xEA2, 0x0, 0x1, 15);
ret = pmic_config_interface(0xEAA, 0x83, 0xFFF, 0);
ret = pmic_config_interface(0xEAA, 0x0, 0x1, 13);
ret = pmic_config_interface(0xEAA, 0x1, 0x1, 15);
ret = pmic_config_interface(0xEB2, 0x1, 0x3, 4);
ret = pmic_config_interface(0xEB2, 0x3, 0x3, 6);
ret = pmic_config_interface(0xEB2, 0x1, 0x3, 8);
ret = pmic_config_interface(0xEB2, 0x1, 0x3, 10);
ret = pmic_config_interface(0xEB2, 0x1, 0x3, 12);
ret = pmic_config_interface(0xEB2, 0x2, 0x3, 14);
ret = pmic_config_interface(0xEB4, 0x1, 0x3, 0);
ret = pmic_config_interface(0xEB4, 0x1, 0x3, 2);
ret = pmic_config_interface(0xEB4, 0x1, 0x3, 4);
ret = pmic_config_interface(0xEB4, 0x3, 0x3, 6);
ret = pmic_config_interface(0xEC6, 0x1, 0x1, 14);
ret = pmic_config_interface(0xF16, 0xC, 0x3FF, 0);
ret = pmic_config_interface(0xF16, 0x0, 0x1, 15);
ret = pmic_config_interface(0xF1C, 0xC, 0x3FF, 0);
ret = pmic_config_interface(0xF1C, 0x1, 0x1, 15);
ret = pmic_config_interface(0xF20, 0x1, 0x1, 2);
ret = pmic_config_interface(0xF7A, 0xB, 0xF, 4);
ret = pmic_config_interface(0xF84, 0x4, 0xF, 1);
ret = pmic_config_interface(0xF92, 0x3, 0xF, 0);
ret = pmic_config_interface(0xFA0, 0x1, 0x1, 1);
ret = pmic_config_interface(0xFA4, 0x0, 0x7, 4);
ret = pmic_config_interface(0xFAA, 0x1, 0x1, 2);
ret = pmic_config_interface(0xFAA, 0x1, 0x1, 6);
ret = pmic_config_interface(0xFAA, 0x1, 0x1, 7);

ret = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH11, 5, 0);

/*****************************************************
 * below programming is used for MD setting
 *****************************************************/
	PMIC_MD_INIT_SETTING_V1();
}

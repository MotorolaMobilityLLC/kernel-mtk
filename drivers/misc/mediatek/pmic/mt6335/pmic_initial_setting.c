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

#include "linux/delay.h"
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_chip.h>
#ifdef CONFIG_OF
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#endif
#define PMIC_32K_LESS_DETECT_V1      0
#define PMIC_CO_TSX_V1               0

#define PMIC_READ_REGISTER_UINT32(reg)	(*(volatile uint32_t *const)(reg))
#define PMIC_INREG32(x)	PMIC_READ_REGISTER_UINT32((uint32_t *)((void *)(x)))
#define PMIC_WRITE_REGISTER_UINT32(reg, val)	((*(volatile uint32_t *const)(reg)) = (val))
#define PMIC_OUTREG32(x, y)	PMIC_WRITE_REGISTER_UINT32((uint32_t *)((void *)(x)), (uint32_t)(y))

#define PMIC_DRV_Reg32(addr)             PMIC_INREG32(addr)
#define PMIC_DRV_WriteReg32(addr, data)  PMIC_OUTREG32(addr, data)

int PMIC_MD_INIT_SETTING_V1(void)
{
	unsigned int ret = 0;
#if PMIC_32K_LESS_DETECT_V1
	unsigned int pmic_reg = 0;
#endif

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
		/* VCTCXO on MT6176 , OFF XO on MT6353
		   HW control, use srclken_0 */

		ret = pmic_config_interface(0xA04, 0x0, 0x7, 11);
		pr_err("[PMIC] VCTCXO on MT6176 , OFF XO on MT6353\n");
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
		if (modem_temp_base)
			iounmap(modem_temp_base);
		modem_temp_base = 0;
		return ret;
	}
/* -- MT6353 TBD Start -- */
	modem_temp_base = of_iomap(modem_temp_node, 0);
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base, 0x011f);
	pr_err("[PMIC] TEMP_SHARE_CTRL:0x%x\n", PMIC_DRV_Reg32(modem_temp_base));
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base + 0x04, 0x013f);
	/* modem temp */
	PMIC_DRV_WriteReg32(modem_temp_base, 0x0);
	pr_err("[PMIC] TEMP_SHARE_CTRL:0x%x _RATIO:0x%x\n", PMIC_DRV_Reg32(modem_temp_base),
	       PMIC_DRV_Reg32(modem_temp_base + 0x04));
/* -- MT6353 TBD End -- */

#endif
	return ret;
}

void PMIC_upmu_set_rg_baton_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface((unsigned int)(PMIC_RG_BATON_EN_ADDR),
				    (unsigned int)(val),
				    (unsigned int)(PMIC_RG_BATON_EN_MASK),
				    (unsigned int)(PMIC_RG_BATON_EN_SHIFT)
	    );
}

void PMIC_upmu_set_baton_tdet_en(unsigned int val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface((unsigned int)(PMIC_BATON_TDET_EN_ADDR),
				    (unsigned int)(val),
				    (unsigned int)(PMIC_BATON_TDET_EN_MASK),
				    (unsigned int)(PMIC_BATON_TDET_EN_SHIFT)
	    );
}

unsigned int PMIC_upmu_get_rgs_baton_undet(void)
{
	unsigned int ret = 0;
	unsigned int val = 0;

	ret = pmic_read_interface((unsigned int)(PMIC_RGS_BATON_UNDET_ADDR),
				  (&val),
				  (unsigned int)(PMIC_RGS_BATON_UNDET_MASK),
				  (unsigned int)(PMIC_RGS_BATON_UNDET_SHIFT)
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
#ifdef MT6351
	unsigned int ret = 0;
#endif

	chip_version = pmic_get_register_value(PMIC_SWCID);

	/*1.UVLO off*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] TOP_RST_STATUS Reg[0x%x]=0x%x\n",
		MT6335_TOP_RST_STATUS, upmu_get_reg_value(MT6335_TOP_RST_STATUS));
	/*2.thermal shutdown 150*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] THERMALSTATUS Reg[0x%x]=0x%x\n",
		MT6335_THERMALSTATUS, upmu_get_reg_value(MT6335_THERMALSTATUS));
	/*3.power not good*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] PGSTATUS0 Reg[0x%x]=0x%x\n",
		MT6335_PGSTATUS0, upmu_get_reg_value(MT6335_PGSTATUS0));
	pr_debug("[PMIC_KERNEL][pmic_boot_status] PGSTATUS1 Reg[0x%x]=0x%x\n",
		MT6335_PGSTATUS1, upmu_get_reg_value(MT6335_PGSTATUS1));
	/*4.buck oc*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] PSOCSTATUS Reg[0x%x]=0x%x\n",
		MT6335_PSOCSTATUS, upmu_get_reg_value(MT6335_PSOCSTATUS));
	/*5.long press shutdown*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] STRUP_CON4 Reg[0x%x]=0x%x\n",
		MT6335_STRUP_CON4, upmu_get_reg_value(MT6335_STRUP_CON4));
	/*6.WDTRST*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] TOP_RST_MISC Reg[0x%x]=0x%x\n",
		MT6335_TOP_RST_MISC, upmu_get_reg_value(MT6335_TOP_RST_MISC));
	/*7.CLK TRIM*/
	pr_debug("[PMIC_KERNEL][pmic_boot_status] TOP_CLK_TRIM  Reg[0x%x]=0x%x\n",
		MT6335_TOP_CLK_TRIM, upmu_get_reg_value(MT6335_TOP_CLK_TRIM));

	is_battery_remove = !PMIC_check_battery();
	is_wdt_reboot_pmic = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	pmic_set_register_value(PMIC_TOP_RST_MISC_SET, 0x8);
	is_wdt_reboot_pmic_chk = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	pmic_set_register_value(PMIC_TOP_RST_MISC_CLR, 0x8);
	/*--------------------------------------------------------*/
	if (!(upmu_get_reg_value(MT6335_PPCCTL0)))
		pmic_set_register_value(PMIC_RG_PWRHOLD, 0x1);
	pr_err("[PMIC] 6335 PowerHold = 0x%x\n",  upmu_get_reg_value(MT6335_PPCCTL0));
	pr_err("[PMIC] 6335 PMIC Chip = 0x%x\n", chip_version);
	pr_err("[PMIC] 2016-02-25...\n");
	pr_err("[PMIC] is_battery_remove =%d is_wdt_reboot=%d\n",
	       is_battery_remove, is_wdt_reboot_pmic);
	pr_err("[PMIC] is_wdt_reboot_chk=%d\n", is_wdt_reboot_pmic_chk);

	pmic_set_register_value(PMIC_TOP_RST_MISC_SET, 0x1);


#ifdef MT6351
	ret = PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH11, 5, 0);
#endif

/*****************************************************
 * below programming is used for MD setting
 *****************************************************/
	PMIC_MD_INIT_SETTING_V1();
}

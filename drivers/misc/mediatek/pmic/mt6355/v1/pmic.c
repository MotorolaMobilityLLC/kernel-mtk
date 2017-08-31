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

#include <generated/autoconf.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/preempt.h>
#include <linux/uaccess.h>

#include <mt-plat/upmu_common.h>
#include <mach/mtk_pmic.h>
#include "include/pmic.h"
#include "include/pmic_irq.h"
#include "include/pmic_throttling_dlpt.h"
#include "include/pmic_debugfs.h"
#include "pwrap_hal.h"

#ifdef CONFIG_MTK_AUXADC_INTF
#include <mt-plat/mtk_auxadc_intf.h>
#endif /* CONFIG_MTK_AUXADC_INTF */

#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mach/mtk_battery_meter.h>
#endif

#if defined(CONFIG_MTK_EXTBUCK)
#include "include/extbuck/fan53526.h"
#endif

void vmd1_pmic_setting_on(void)
{
#ifdef CONFIG_MACH_MT6759
	/* 1.Call PMIC driver API configure VMODEM voltage as 0.75V (0.4+0.00625*step)*/
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL, 0x38); /* set to 0.75V */
	/* 2.Call PMIC driver API configure VSRAM_MD voltage as 0.88125V (0.51875+0.00625*step)*/
	pmic_set_register_value(PMIC_RG_LDO_VSRAM_MD_VOSEL, 0x3A); /* set to 0.88125V */
#else
	/* 1.Call PMIC driver API configure VMODEM voltage as 0.8V */
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL, 0x40); /* set to 0.8V */
	/* 2.Call PMIC driver API configure VSRAM_MD voltage as 0.93125V */
	pmic_set_register_value(PMIC_RG_LDO_VSRAM_MD_VOSEL, 0x42); /* set to 0.93125V */
#endif
	/* Enable FPFM before enable BUCK, SW workaround to avoid VMODEM overshoot */
	pmic_config_interface(0x128E, 0x1, 0x1, 12);	/* 0x128E[12] = 1 */
	PMICLOG("vmd1_pmic_setting_on vmodem fpfm %d\n",
		((pmic_get_register_value(PMIC_RG_VMODEM_TRAN_BST) & 0x20) >> 5));
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_EN, 1);
	pmic_set_register_value(PMIC_RG_LDO_VSRAM_MD_EN, 1);
	udelay(220);

	/* Disable FPFM after enable BUCK, SW workaround to avoid VMODME overshoot */
	pmic_config_interface(0x128E, 0x0, 0x1, 12);	/* 0x128E[12] = 0 */
	PMICLOG("vmd1_pmic_setting_on vmodem fpfm %d\n",
		((pmic_get_register_value(PMIC_RG_VMODEM_TRAN_BST) & 0x20) >> 5));

#ifdef CONFIG_MACH_MT6759
	if (pmic_get_register_value(PMIC_DA_VMODEM_VOSEL) != 0x38)
#else
	if (pmic_get_register_value(PMIC_DA_VMODEM_VOSEL) != 0x40)
#endif
		pr_err("vmd1_pmic_setting_on vmodem vosel = 0x%x, da_vosel = 0x%x",
			pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL),
			pmic_get_register_value(PMIC_DA_VMODEM_VOSEL));

#ifdef CONFIG_MACH_MT6759
	if (pmic_get_register_value(PMIC_DA_QI_VSRAM_MD_VOSEL) != 0x3A)
#else
	if (pmic_get_register_value(PMIC_DA_QI_VSRAM_MD_VOSEL) != 0x42)
#endif
		pr_err("vmd1_pmic_setting_on vsram_md vosel = 0x%x, da_vosel = 0x%x",
			pmic_get_register_value(PMIC_RG_LDO_VSRAM_MD_VOSEL),
			pmic_get_register_value(PMIC_DA_QI_VSRAM_MD_VOSEL));

}

void vmd1_pmic_setting_off(void)
{
	/* 1.Call PMIC driver API configure VMODEM off */
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_EN, 0);
	/* 2.Call PMIC driver API configure VSRAM_MD off */
	pmic_set_register_value(PMIC_RG_LDO_VSRAM_MD_EN, 0);
	PMICLOG("vmd1_pmic_setting_off vmodem en %d\n",
		(pmic_get_register_value(PMIC_DA_VMODEM_EN) & 0x1));
	PMICLOG("vmd1_pmic_setting_off vsram_md en %d\n",
		(pmic_get_register_value(PMIC_DA_QI_VSRAM_MD_EN) & 0x1));
}


/*****************************************************************************
 * upmu_interrupt_chrdet_int_en
 ******************************************************************************/
#if 0 /* May not used, marked by Jeter */
void upmu_interrupt_chrdet_int_en(unsigned int val)
{
	PMICLOG("[upmu_interrupt_chrdet_int_en] val=%d\n", val);
	pmic_enable_interrupt(INT_CHRDET, val, "PMIC");
}
EXPORT_SYMBOL(upmu_interrupt_chrdet_int_en);
#endif

/*****************************************************************************
 * PMIC charger detection
 ******************************************************************************/
unsigned int upmu_get_rgs_chrdet(void)
{
	unsigned int val = 0;

	val = pmic_get_register_value(PMIC_RGS_CHRDET);
	PMICLOG("[upmu_get_rgs_chrdet] CHRDET status = %d\n", val);

	return val;
}

int pmic_rdy;
int usb_rdy;
void pmic_enable_charger_detection_int(int x)
{

	if (x == 0) {
		pmic_rdy = 1;
		PMICLOG("[pmic_enable_charger_detection_int] PMIC\n");
	} else if (x == 1) {
		usb_rdy = 1;
		PMICLOG("[pmic_enable_charger_detection_int] USB\n");
	}

	PMICLOG("[pmic_enable_charger_detection_int] pmic_rdy=%d usb_rdy=%d\n", pmic_rdy, usb_rdy);
	if (pmic_rdy == 1 && usb_rdy == 1) {
#if defined(CONFIG_MTK_SMART_BATTERY)
		wake_up_bat();
#endif
		PMICLOG("[pmic_enable_charger_detection_int] enable charger detection interrupt\n");
	}
}


bool is_charger_detection_rdy(void)
{

	if (pmic_rdy == 1 && usb_rdy == 1)
		return true;
	else
		return false;
}

int is_ext_buck2_exist(void)
{
#if defined(CONFIG_MTK_EXTBUCK)
	if ((is_fan53526_exist() == 1))
		return 1;
	else
		return 0;
#else
	return 0;
#endif /* End of #if defined(CONFIG_MTK_EXTBUCK) */
}

int is_ext_vbat_boost_exist(void)
{
	return 0;
}

int is_ext_swchr_exist(void)
{
	return 0;
}


/*****************************************************************************
 * Enternal SWCHR
 ******************************************************************************/

/*****************************************************************************
 * Enternal VBAT Boost status
 ******************************************************************************/

/*****************************************************************************
 * Enternal BUCK status
 ******************************************************************************/

int get_ext_buck_i2c_ch_num(void)
{
#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
	if (is_mt6313_exist() == 1)
		return get_mt6313_i2c_ch_num();
#endif
		return -1;
}

int is_ext_buck_sw_ready(void)
{
#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
	if ((is_mt6313_sw_ready() == 1))
		return 1;
#endif
		return 0;
}

int is_ext_buck_exist(void)
{
#if defined(CONFIG_MTK_PMIC_CHIP_MT6313)
	if ((is_mt6313_exist() == 1))
		return 1;
#endif
		return 0;
}

int is_ext_buck_gpio_exist(void)
{
	return pmic_get_register_value(PMIC_RG_STRUP_EXT_PMIC_EN);
}

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

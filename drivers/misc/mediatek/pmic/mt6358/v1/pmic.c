/*
 * Copyright (C) 2017 MediaTek Inc.

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

#include "mtk_devinfo.h"

#include <mt-plat/upmu_common.h>
#include <mach/mtk_pmic.h>
#include "include/pmic.h"
#include "include/pmic_irq.h"
#include "include/pmic_throttling_dlpt.h"
#include "include/pmic_debugfs.h"

#ifdef CONFIG_MTK_AUXADC_INTF
#include <mt-plat/mtk_auxadc_intf.h>
#endif /* CONFIG_MTK_AUXADC_INTF */

void record_md_vosel(void)
{
	g_vmodem_vosel = pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL);
	pr_info("[%s] vmodem_vosel = 0x%x\n", __func__, g_vmodem_vosel);
}

void vmd1_pmic_setting_on(void)
{
#if 0 /*TBD*/
	unsigned int segment = get_devinfo_with_index(28);
	unsigned char vmodem_segment = (unsigned char)((segment & 0x08000000) >> 27);

	if (!vmodem_segment)
		g_vmodem_vosel = 0x6F;/* VMODEM 1.19375V: 0x6F */
	else
		g_vmodem_vosel = 0x68;/* VMODEM 1.15V: 0x68 */
#endif
	/* 1.Call PMIC driver API configure VMODEM voltage */
	if (g_vmodem_vosel != 0) {
		pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL,
			g_vmodem_vosel);
	} else {
		pr_notice("[%s] vmodem vosel has not recorded!\n", __func__);
		g_vmodem_vosel =
			pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL);
		pr_info("[%s] vmodem_vosel = 0x%x\n",
			__func__, g_vmodem_vosel);
	}
	if (pmic_get_register_value(PMIC_DA_VMODEM_VOSEL) != g_vmodem_vosel)
		pr_notice("[%s] vmodem vosel = 0x%x, da_vosel = 0x%x",
			__func__,
			pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL),
			pmic_get_register_value(PMIC_DA_VMODEM_VOSEL));
}

void vmd1_pmic_setting_off(void)
{
	PMICLOG("vmd1_pmic_setting_off\n");
}

int vcore_pmic_set_mode(unsigned char mode)
{
	unsigned char ret = 0;

	pmic_set_register_value(PMIC_RG_VCORE_FPWM, mode);

	ret = pmic_get_register_value(PMIC_RG_VCORE_FPWM);

	return (ret == mode) ? (0) : (-1);
}

/* [Export API] */

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

int is_ext_vbat_boost_exist(void)
{
	return 0;
}

int is_ext_swchr_exist(void)
{
	return 0;
}

/*****************************************************************************
 * Enternal BUCK status
 ******************************************************************************/

int is_ext_buck_gpio_exist(void)
{
	return pmic_get_register_value(PMIC_RG_STRUP_EXT_PMIC_EN);
}

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");
